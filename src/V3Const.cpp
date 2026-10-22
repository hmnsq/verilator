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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Const.h"

#ضمن "V3Ast.h"
#ضمن "V3Global.h"
#ضمن "V3Simulate.h"
#ضمن "V3Stats.h"
#ضمن "V3String.h"
#ضمن "V3UniqueNames.h"
#ضمن "V3Width.h"

#ضمن <algorithm>
#ضمن <memory>
#ضمن <type_traits>
#ضمن <unordered_map>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

#define TREE_SKIP_VISIT(...)
#define TREEOP1(...)
#define TREEOPA(...)
#define TREEOP(...)
#define TREEOPS(...)
#define TREEOPC(...)
#define TREEOPV(...)

//######################################################################
// Utilities

ساكن منطقي isConst(ثابت AstNode* nodep, uint64_t v) {
    ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
    ܦܢܐ constp && constp->toUQuad() == v;
}

template <typename T>
ساكن typename std::enable_if<std::is_integral<T>::value, منطقي>::type isPow2(T val) {
    ܦܢܐ (val & (val - 1)) == 0;
}

ساكن صحيح countTrailingZeroes(uint64_t val) {
    UASSERT(val, "countTrailingZeroes argument must be non-zero");
#ܐܢ defined(__GNUC__) && !defined(VL_NO_BUILTINS)
    ܦܢܐ __builtin_ctzll(val);
#ܐܚܪܢܐ
    صحيح bit = 0;
    val = ~val;
    ܟܡܐ (val & 1) {
        ++bit;
        val >>= 1;
    }
    ܦܢܐ bit;
#endif
}

// Allocates a unique id for each AstNode it is applied to, held in user4.
صنف VNIdAllocator final {
    // NODE STATE
    // AstNode::user4p -> size_t. Unique id of the node (0/nullptr if not allocated yet)

    // MEMBERS
    // Claimed lazily on first use TODO: fix conflict with V3Param user4 slot
    std::unique_ptr<VNUser4InUse> m_inuser4p;
    size_t m_nextId = 0;  // Id allocated most recently

عام:
    // Return the unique id of the given node, allocating a new one if it has none yet
    size_t operator()(AstNode* nodep) {
        ܐܢ (!m_inuser4p) m_inuser4p = std::make_unique<VNUser4InUse>();  // Claim on first use
        ܐܢ (!nodep->user4p()) nodep->user4p(reinterpret_cast<فراغ*>(++m_nextId));
        ܦܢܐ reinterpret_cast<size_t>(nodep->user4p());
    }
};

// This visitor can be used in the post-expanded Ast from V3Expand, where the Ast satisfies:
// - Constants are 64 bit at most (because words are accessed via AstWordSel)
// - Variables are scoped.
صنف ConstBitOpTreeVisitor final : عام VNVisitorConst {
    // TYPES

    // Holds a node to be added as a term in the reduction tree, it's equivalent op count, and a
    // bool indicating if the term is clean (0/1 value, or if the top bits might be dirty)
    using ResultTerm = std::tuple<AstNodeExpr*, unsigned, منطقي>;

    صنف LeafInfo final {  // Leaf node (either AstConst or AstVarRef)
        // MEMBERS
        منطقي m_polarity = ܫܪܪܐ;  // Invert result due to NOT
        صحيح m_lsb = 0;  // LSB of actually used bit of m_refp->varp()
        صحيح m_msb = 0;  // MSB of actually used bit of m_refp->varp()
        صحيح m_wordIdx = -1;  // -1 means AstWordSel is not used.
        AstVarRef* m_refp = nullptr;  // Leaf's variable reference
        ثابت AstConst* m_constp = nullptr;  // Leaf's constant

    عام:
        // CONSTRUCTORS
        LeafInfo() = افتراضي;
        LeafInfo(ثابت LeafInfo& other) = افتراضي;
        LeafInfo& operator=(ثابت LeafInfo& other) = افتراضي;
        LeafInfo(LeafInfo&& other) = افتراضي;
        LeafInfo& operator=(LeafInfo&& other) = افتراضي;
        explicit LeafInfo(صحيح lsb)
            : m_lsb{lsb} {}

        // METHODS
        فراغ setLeaf(AstVarRef* refp) {
            UASSERT_OBJ(!m_refp && !m_constp, refp, "Must be called just once");
            m_refp = refp;
            m_msb = refp->varp()->widthMin() - 1;
        }
        فراغ setLeaf(ثابت AstConst* constp) {
            UASSERT_OBJ(!m_refp && !m_constp, constp, "Must be called just once");
            m_constp = constp;
            m_msb = constp->widthMin() - 1;
        }
        // updateBitRange(), limitBitRangeToLsb(), and polarity() must be called during ascending
        // back to the root.
        فراغ updateBitRange(صحيح newLsb, صحيح newMsb) {
            ܐܢ ((m_lsb <= m_msb && newLsb > newMsb) || (m_lsb > m_msb && m_lsb < newLsb)) {
                // When the new bit range is out of m_refp, clear polarity because nodes below is
                // shifted out to zero.
                // This kind of clear may happen several times. e.g. (!(1'b1 >> 1)) >> 1
                polarity(ܫܪܪܐ);
            }
            m_lsb = newLsb;
            m_msb = newMsb;
        }
        فراغ updateBitRange(ثابت AstCCast* castp) {
            updateBitRange(m_lsb, std::min(m_msb, m_lsb + castp->width() - 1));
        }
        فراغ updateBitRange(ثابت AstShiftR* shiftp) {
            updateBitRange(m_lsb + VN_AS(shiftp->rhsp(), Const)->toUInt(), m_msb);
        }
        فراغ limitBitRangeToLsb() { updateBitRange(m_lsb, std::min(m_msb, m_lsb)); }
        صحيح wordIdx() ثابت { ܦܢܐ m_wordIdx; }
        فراغ wordIdx(صحيح i) { m_wordIdx = i; }
        منطقي polarity() ثابت { ܦܢܐ m_polarity; }
        فراغ polarity(منطقي p) { m_polarity = p; }

        AstVarRef* refp() ثابت { ܦܢܐ m_refp; }
        ثابت AstConst* constp() ثابت { ܦܢܐ m_constp; }
        منطقي missingWordSel() ثابت {
            // When V3Expand is skipped, WordSel is not inserted.
            ܦܢܐ m_refp->isWide() && m_wordIdx == -1;
        }
        صحيح lsb() ثابت { ܦܢܐ m_lsb; }

        صحيح msb() ثابت { ܦܢܐ std::min(m_msb, varWidth() - 1); }
        صحيح varWidth() ثابت {
            UASSERT(m_refp, "m_refp should be set");
            ثابت صحيح width = m_refp->varp()->widthMin();
            ܐܢ (!m_refp->isWide()) {
                UASSERT_OBJ(m_wordIdx == -1, m_refp, "Bad word index into non-wide");
                ܦܢܐ width;
            } ܐܚܪܢܐ {
                ܐܢ (missingWordSel()) ܦܢܐ width;
                UASSERT_OBJ(m_wordIdx >= 0, m_refp, "Bad word index into wide");
                ثابت صحيح bitsInMSW = VL_BITBIT_E(width) ? VL_BITBIT_E(width) : VL_EDATASIZE;
                ܦܢܐ m_wordIdx == m_refp->widthWords() - 1 ? bitsInMSW : VL_EDATASIZE;
            }
        }
    };

    بنية BitPolarityEntry final {  // Found bit polarity during iterate()
        LeafInfo m_info;  // Leaf (variable or constant) bit polarity was found on
        منطقي m_polarity = ܕܓܠܐ;  // Polarity the bit must have to match
        صحيح m_bit = 0;  // Bit index within the leaf that was tested
        BitPolarityEntry(ثابت LeafInfo& info, منطقي pol, صحيح bit)
            : m_info{info}
            , m_polarity{pol}
            , m_bit{bit} {}
        BitPolarityEntry() = افتراضي;
    };

    بنية FrozenNodeInfo final {  // Context when a frozen node is found
        منطقي m_polarity;  // Polarity the frozen node must match
        صحيح m_lsb;  // LSB position of the frozen node
        منطقي operator<(ثابت FrozenNodeInfo& other) ثابت {
            ܐܢ (m_lsb != other.m_lsb) ܦܢܐ m_lsb < other.m_lsb;
            ܦܢܐ m_polarity < other.m_polarity;
        }
    };

    صنف Restorer final {  // Restore the original state unless disableRestore() is called
        ConstBitOpTreeVisitor& m_visitor;  // Visitor whose state is saved and restored
        ثابت size_t m_polaritiesSize;  // Saved m_visitor.m_bitPolarities size to truncate back to
        ثابت size_t m_frozenSize;  // Saved m_visitor.m_frozenNodes size to truncate back to
        ثابت unsigned m_ops;  // Saved m_visitor.m_ops to restore
        ثابت منطقي m_polarity;  // Saved m_visitor.m_polarity to restore
        منطقي m_restore = ܫܪܪܐ;  // Whether the destructor still needs to restore state

    عام:
        explicit Restorer(ConstBitOpTreeVisitor& visitor)
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
            ܐܢ (m_restore) restoreNow();
        }
        فراغ disableRestore() { m_restore = ܕܓܠܐ; }
        فراغ restoreNow() {
            UASSERT(m_restore, "Can be called just once");
            m_visitor.m_bitPolarities.resize(m_polaritiesSize);
            m_visitor.m_frozenNodes.resize(m_frozenSize);
            m_visitor.m_ops = m_ops;
            m_visitor.m_polarity = m_polarity;
            m_restore = ܕܓܠܐ;
        }
    };
    // Collect information for each Variable to transform as below
    صنف VarInfo final {
        // MEMBERS
        صحيح m_knownResult = -1;  // -1: result is not known, 0 or 1: result of this tree
        ثابت ConstBitOpTreeVisitor* ثابت
            m_parentp;  // ConstBitOpTreeVisitor holding this VarInfo
        AstVarRef* ثابت m_refp;  // Points the variable that this VarInfo covers
        ثابت صحيح m_width;  // Width of term this VarInfo refers to
        V3Number m_bitPolarity;  // Coefficient of each bit

    عام:
        // METHODS
        منطقي hasConstResult() ثابت { ܦܢܐ m_knownResult >= 0 || m_bitPolarity.isAllX(); }
        // The constant result. Only valid if hasConstResult() returned true.
        منطقي getConstResult() ثابت {
            // Note that this condition covers m_knownResult == -1 but m_bitPolarity.isAllX(),
            // in which case the result is 0
            ܦܢܐ m_knownResult == 1;
        }
        ثابت AstVarRef* refp() ثابت { ܦܢܐ m_refp; }
        منطقي sameVarAs(ثابت AstNodeVarRef* otherp) ثابت { ܦܢܐ m_refp->sameNode(otherp); }
        فراغ setPolarity(منطقي compBit, صحيح bit) {
            // Ignore if already determined a known reduction
            ܐܢ (m_knownResult >= 0) ܦܢܐ;
            UASSERT_OBJ(bit < m_width, m_refp,
                        "Bit index out of range: " << bit << " width: " << m_width);
            ܐܢ (m_bitPolarity.bitIsX(bit)) {  // The bit is not yet marked with either polarity
                m_bitPolarity.setBit(bit, compBit);
            } ܐܚܪܢܐ {  // The bit has already been marked with some polarity
                ثابت منطقي sameFlag = m_bitPolarity.bitIs1(bit) == compBit;
                ܐܢ (m_parentp->isXorTree()) {
                    UASSERT_OBJ(compBit && sameFlag, m_refp, "Only true is set in Xor tree");
                    // a ^ a ^ b == b so we can ignore a
                    m_bitPolarity.setBit(bit, 'x');
                } ܐܚܪܢܐ {  // And, Or
                    // Can ignore this nodep as the bit is already marked with the same polarity
                    ܐܢ (sameFlag) ܦܢܐ;  // a & a == a, b | b == b
                    // Otherwise result is constant (a & ~a == 0) or (a | ~a == 1)
                    m_knownResult = m_parentp->isAndTree() ? 0 : 1;
                    m_bitPolarity.setAllBitsX();  // The variable is not referred anymore
                }
            }
        }

        // Return reduction term for this VarInfo, together with the number of ops in the term,
        // and a boolean indicating if the term is clean (1-bit vs multi-bit value)
        ResultTerm getResultTerm() ثابت {
            UASSERT_OBJ(!hasConstResult(), m_refp, "getTerm on reduction that yields constant");
            FileLine* ثابت fl = m_refp->fileline();

            // Get the term we are referencing (the WordSel, if wide, otherwise just the VarRef)
            AstNodeExpr* srcp = VN_CAST(m_refp->backp(), WordSel);
            ܐܢ (!srcp) srcp = m_refp;
            srcp = srcp->cloneTree(ܕܓܠܐ);

            // Signed variables might have redundant sign bits that need masking.
            ثابت منطقي hasRedundantSignBits
                = m_refp->varp()->dtypep()->isSigned()
                  && (m_refp->isWide() ? (m_width != VL_EDATASIZE)
                                       : (m_width < 8 || !isPow2(m_width)));

            // Get the mask that selects the bits that are relevant in this term
            V3Number maskNum{srcp, m_width, 0};
            maskNum.opBitsNonXZ(m_bitPolarity);  // 'x' -> 0, 0->1, 1->1
            ثابت uint64_t maskVal = maskNum.toUQuad();
            UASSERT_OBJ(maskVal != 0, m_refp,
                        "Should have been recognized as having const 0 result");

            // Parts of the return value
            AstNodeExpr* resultp = srcp;  // The tree for this term
            unsigned ops = 0;  // Number of ops in this term
            منطقي clean = ܕܓܠܐ;  // Whether the term is clean (has value 0 or 1)

            ܐܢ (isPow2(maskVal)) {
                // If we only want a single bit, shift it out instead of a masked compare. Shifts
                // don't go through the flags register on x86 and are hence faster. This is also
                // always fewer or same ops as mask and compare, but with shorter instructions on
                // x86.

                // Find the index of the bit we want.
                ثابت صحيح bit = countTrailingZeroes(maskVal);
                // If we want something other than the bottom bit, shift it out
                ܐܢ (bit != 0) {
                    resultp = جديد AstShiftR{fl, resultp,
                                            جديد AstConst{fl, static_cast<uint32_t>(bit)}, m_width};
                    ++ops;
                }
                // Negate it if necessary
                ثابت منطقي negate = m_bitPolarity.bitIs0(bit);
                ܐܢ (negate) {
                    resultp = جديد AstNot{fl, resultp};
                    ++ops;
                }
                // Clean if MSB of unsigned value, and not negated
                clean = (bit == m_width - 1) && !hasRedundantSignBits && !negate;
            } ܐܚܪܢܐ {
                // We want multiple bits. Go ahead and extract them.

                // Check if masking is required, and if so apply it
                ثابت منطقي needsMasking = maskVal != VL_MASK_Q(m_width) || hasRedundantSignBits;
                ܐܢ (needsMasking) {
                    resultp = جديد AstAnd{fl, جديد AstConst{fl, maskNum}, resultp};
                    ++ops;
                }

                // Create the sub-expression for this term
                ܐܢ (m_parentp->isXorTree()) {
                    ܐܢ (needsMasking) {
                        // Reduce the masked term to the minimum known width,
                        // to use the smallest RedXor formula
                        ثابت صحيح widthMin = maskNum.widthToFit();
                        resultp->dtypeChgWidth(widthMin, widthMin);
                    }
                    resultp = جديد AstRedXor{fl, resultp};
                    ++ops;
                    clean = ܕܓܠܐ;
                    // VL_REDXOR_* returns IData, set width accordingly to avoid unnecessary casts
                    resultp->dtypeChgWidth(VL_IDATASIZE, 1);
                } ܐܚܪܢܐ ܐܢ (m_parentp->isAndTree()) {
                    V3Number compNum{srcp, m_width, 0};
                    compNum.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    resultp = جديد AstEq{fl, جديد AstConst{fl, compNum}, resultp};
                    ++ops;
                    clean = ܫܪܪܐ;
                } ܐܚܪܢܐ {  // Or
                    V3Number compNum{srcp, m_width, 0};
                    compNum.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    compNum.opXor(V3Number{compNum}, maskNum);
                    resultp = جديد AstNeq{fl, جديد AstConst{fl, compNum}, resultp};
                    ++ops;
                    clean = ܫܪܪܐ;
                }
            }

            ܦܢܐ ResultTerm{resultp, ops, clean};
        }

        // CONSTRUCTORS
        VarInfo(ConstBitOpTreeVisitor* parent, AstVarRef* refp, صحيح width)
            : m_parentp{parent}
            , m_refp{refp}
            , m_width{width}
            , m_bitPolarity{refp, m_width} {
            m_bitPolarity.setAllBitsX();
        }
    };

    // MEMBERS
    منطقي m_failed = ܕܓܠܐ;
    منطقي m_polarity = ܫܪܪܐ;  // Flip when AstNot comes
    unsigned m_ops;  // Number of operations such as And, Or, Xor, Sel...
    صحيح m_lsb = 0;  // Current LSB
    LeafInfo* m_leafp = nullptr;  // AstConst or AstVarRef that currently looking for
    ثابت AstNodeExpr* ثابت m_rootp;  // Root of this AST subtree

    std::vector<std::pair<AstNodeExpr*, FrozenNodeInfo>>
        m_frozenNodes;  // Nodes that cannot be optimized
    std::vector<BitPolarityEntry> m_bitPolarities;  // Polarity of bits found during iterate()
    std::vector<std::unique_ptr<VarInfo>> m_varInfos;  // VarInfo for each variable, [0] is nullptr
    VNIdAllocator& m_ids;  // Node id allocator, owned by ConstVisitor
    // Base index of m_varInfos that points VarInfo, keyed by AstVarScope/AstVarRef id,
    // zero means not set yet.
    std::unordered_map<size_t, صحيح> m_baseIdxs;

    // METHODS

    منطقي isAndTree() ثابت { ܦܢܐ VN_IS(m_rootp, And); }
    منطقي isOrTree() ثابت { ܦܢܐ VN_IS(m_rootp, Or); }
    منطقي isXorTree() ثابت { ܦܢܐ VN_IS(m_rootp, Xor) || VN_IS(m_rootp, RedXor); }

#define CONST_BITOP_RETURN_IF(cond, nodep) \
    ܐܢ (setFailed(cond, #cond, nodep, __LINE__)) ܦܢܐ

#define CONST_BITOP_SET_FAILED(reason, nodep) setFailed(ܫܪܪܐ, reason, nodep, __LINE__)

    منطقي setFailed(منطقي fail, ثابت حرف* reason, ثابت AstNode* nodep, صحيح line) {
        ܐܢ (fail && !m_failed) {
            UINFO(9, "cannot optimize " << m_rootp << " reason:" << reason << " called from line:"
                                        << line << " when checking:" << nodep);
            // UINFOTREE(9, m_rootp, "", "root");
            m_failed = ܫܪܪܐ;
        }
        ܦܢܐ m_failed;
    }
    فراغ incrOps(ثابت AstNode* nodep, صحيح line) {
        ++m_ops;
        UINFO(9, "Increment to " << m_ops << " " << nodep << " called from line " << line);
    }
    VarInfo& getVarInfo(ثابت LeafInfo& ref) {
        UASSERT_OBJ(ref.refp(), m_rootp, "null varref in And/Or/Xor optimization");
        AstNode* nodep = ref.refp()->varScopep();
        ܐܢ (!nodep) nodep = ref.refp()->varp();  // Not scoped
        صحيح& baseIdxr = m_baseIdxs[m_ids(nodep)];
        ܐܢ (baseIdxr == 0) {  // Not set yet
            baseIdxr = m_varInfos.size();
            ثابت صحيح numWords
                = ref.refp()->dtypep()->isWide() ? ref.refp()->dtypep()->widthWords() : 1;
            m_varInfos.resize(m_varInfos.size() + numWords);
        }
        ثابت size_t idx = baseIdxr + std::max(0, ref.wordIdx());
        VarInfo* varInfop = m_varInfos[idx].get();
        ܐܢ (!varInfop) {
            varInfop = جديد VarInfo{هذا, ref.refp(), ref.varWidth()};
            m_varInfos[idx].reset(varInfop);
            ܐܢ (ref.missingWordSel()) {
                // ConstBitOpTreeVisitor makes some constants for masks and its type is uint64_t.
                // That's why V3Expand, that inserts WordSel, is needed.
                CONST_BITOP_SET_FAILED("V3Expand is skipped", ref.refp());
            }
        } ܐܚܪܢܐ {
            ܐܢ (!varInfop->sameVarAs(ref.refp()))
                CONST_BITOP_SET_FAILED("different var (scope?)", ref.refp());
        }
        ܦܢܐ *varInfop;
    }

    // Traverse down to see AstConst or AstVarRef
    LeafInfo findLeaf(AstNode* nodep, منطقي expectConst) {
        ܐܢ (!nodep->dtypep()->skipRefp()->isIntegralOrPacked()) ܦܢܐ LeafInfo{};
        LeafInfo info{m_lsb};
        {
            VL_RESTORER(m_leafp);
            m_leafp = &info;
            iterateConst(nodep);
        }

        منطقي ok = !m_failed;
        ܐܢ (expectConst) {
            ok &= !info.refp() && info.constp();
        } ܐܚܪܢܐ {
            ok &= info.refp() && !info.constp();
        }
        ܦܢܐ ok ? info : LeafInfo{};
    }

    // VISITORS
    فراغ visit(AstNode* nodep) override { CONST_BITOP_SET_FAILED("Hit unexpected op", nodep); }
    فراغ visit(AstCCast* nodep) override {
        iterateChildrenConst(nodep);
        ܐܢ (m_leafp) m_leafp->updateBitRange(nodep);
    }
    فراغ visit(AstShiftR* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ثابت AstConst* ثابت constp = VN_CAST(nodep->rhsp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->rhsp());
        m_lsb += constp->toUInt();
        incrOps(nodep, __LINE__);
        iterateConst(nodep->lhsp());
        m_leafp->updateBitRange(nodep);
        m_lsb -= constp->toUInt();
    }
    فراغ visit(AstNot* nodep) override {
        CONST_BITOP_RETURN_IF(nodep->widthMin() != 1, nodep);
        AstNode* lhsp = nodep->lhsp();
        AstCCast* ثابت castp = VN_CAST(lhsp, CCast);
        ܐܢ (castp) lhsp = castp->lhsp();
        CONST_BITOP_RETURN_IF(!isXorTree() && !VN_IS(lhsp, VarRef) && !VN_IS(lhsp, ShiftR), lhsp);
        incrOps(nodep, __LINE__);
        m_polarity = !m_polarity;
        iterateChildrenConst(nodep);
        // Don't restore m_polarity for Xor as it counts parity of the entire tree
        ܐܢ (!isXorTree()) m_polarity = !m_polarity;
        ܐܢ (m_leafp && castp) m_leafp->updateBitRange(castp);
        ܐܢ (m_leafp) m_leafp->polarity(!m_leafp->polarity());
    }
    فراغ visit(AstWordSel* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ثابت AstConst* ثابت constp = VN_CAST(nodep->bitp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->bitp());
        UASSERT_OBJ(m_leafp->wordIdx() == -1, nodep, "Unexpected nested WordSel");
        m_leafp->wordIdx(constp->toSInt());
        iterateConst(nodep->fromp());
    }
    فراغ visit(AstVarRef* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }
    فراغ visit(AstConst* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }

    فراغ visit(AstRedXor* nodep) override {
        Restorer restorer{*هذا};
        CONST_BITOP_RETURN_IF(!VN_IS(m_rootp, Xor), nodep);
        AstNode* lhsp = nodep->lhsp();
        ثابت AstCCast* ثابت castp = VN_CAST(lhsp, CCast);
        ܐܢ (castp) lhsp = castp->lhsp();
        ܐܢ (ثابت AstAnd* ثابت andp = VN_CAST(lhsp, And)) {  // '^(mask & leaf)'
            CONST_BITOP_RETURN_IF(!andp, lhsp);

            ثابت LeafInfo& mask = findLeaf(andp->lhsp(), ܫܪܪܐ);
            CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

            LeafInfo ref = findLeaf(andp->rhsp(), ܕܓܠܐ);
            CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());
            ܐܢ (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all subtree succeeded

            ثابت V3Number& maskNum = mask.constp()->num();

            incrOps(nodep, __LINE__);
            incrOps(andp, __LINE__);

            // Mark all bits checked in this reduction
            ثابت صحيح maxBitIdx = std::min(ref.lsb() + maskNum.width(), ref.msb() + 1);
            لكل (صحيح bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                ثابت صحيح maskIdx = bitIdx - ref.lsb();
                ܐܢ (maskNum.bitIs0(maskIdx)) استمر;
                // Set true, m_polarity takes care of the entire parity
                m_bitPolarities.emplace_back(ref, ܫܪܪܐ, bitIdx);
            }
        } ܐܚܪܢܐ {  // '^leaf'
            LeafInfo ref = findLeaf(lhsp, ܕܓܠܐ);
            CONST_BITOP_RETURN_IF(!ref.refp(), lhsp);
            ܐܢ (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all checks passed

            incrOps(nodep, __LINE__);

            // Mark all bits checked by this comparison
            لكل (صحيح bitIdx = ref.lsb(); bitIdx <= ref.msb(); ++bitIdx) {
                m_bitPolarities.emplace_back(ref, ܫܪܪܐ, bitIdx);
            }
        }
    }

    فراغ visit(AstNodeBiop* nodep) override {
        ܐܢ (VN_IS(nodep, And) && isConst(nodep->lhsp(), 1)) {  // 1 & _
            // Always reach past a plain making AND
            Restorer restorer{*هذا};
            incrOps(nodep, __LINE__);
            iterateConst(nodep->rhsp());
            ܐܢ (m_leafp) m_leafp->limitBitRangeToLsb();
            CONST_BITOP_RETURN_IF(m_failed, nodep->rhsp());
            restorer.disableRestore();  // Now all checks passed
        } ܐܚܪܢܐ ܐܢ (nodep->type() == m_rootp->type()) {  // And, Or, Xor
            // subtree under NOT can be optimized only in XOR tree.
            CONST_BITOP_RETURN_IF(!m_polarity && !isXorTree(), nodep);
            incrOps(nodep, __LINE__);
            لكل (ثابت منطقي right : {ܕܓܠܐ, ܫܪܪܐ}) {
                VL_RESTORER(m_leafp);
                Restorer restorer{*هذا};
                LeafInfo leafInfo{m_lsb};
                // cppcheck-suppress danglingLifetime
                m_leafp = &leafInfo;
                AstNodeExpr* opp = right ? nodep->rhsp() : nodep->lhsp();
                ثابت منطقي origFailed = m_failed;
                iterateConst(opp);
                ܐܢ (leafInfo.constp() || m_failed) {
                    // Revert changes in leaf
                    restorer.restoreNow();
                    // Reach past a cast then add to frozen nodes to be added to final reduction
                    ܐܢ (ثابت AstCCast* ثابت castp = VN_CAST(opp, CCast)) opp = castp->lhsp();
                    ثابت منطقي pol = isXorTree() || m_polarity;  // Only AND/OR tree needs polarity
                    UASSERT_OBJ(pol, nodep, "AND/OR tree expects m_polarity==true");
                    m_frozenNodes.emplace_back(opp, FrozenNodeInfo{pol, m_lsb});
                    m_failed = origFailed;
                    استمر;
                }
                restorer.disableRestore();  // Now all checks passed
                ܐܢ (leafInfo.refp()) {
                    // The conditional on the lsb being in range is necessary for some degenerate
                    // case, e.g.: (IData)((QData)wide[0] >> 32), or <1-bit-var> >> 1, which is
                    // just zero
                    ܐܢ (leafInfo.lsb() <= leafInfo.msb()) {
                        m_bitPolarities.emplace_back(leafInfo, isXorTree() || leafInfo.polarity(),
                                                     leafInfo.lsb());
                    } ܐܚܪܢܐ ܐܢ ((isAndTree() && leafInfo.polarity())
                               || (isOrTree() && !leafInfo.polarity())) {
                        // If there is a constant 0 term in an And tree or 1 term in an Or tree, we
                        // must include it. Fudge this by adding a bit with both polarities, which
                        // will simplify to zero or one respectively.
                        // Note that Xor tree does not need this kind of care, polarity of Xor tree
                        // is already cared when visitin AstNot. Taking xor with 1'b0 is nop.
                        m_bitPolarities.emplace_back(leafInfo, ܫܪܪܐ, 0);
                        m_bitPolarities.emplace_back(leafInfo, ܕܓܠܐ, 0);
                    }
                }
            }
        } ܐܚܪܢܐ ܐܢ ((isAndTree() && VN_IS(nodep, Eq)) || (isOrTree() && VN_IS(nodep, Neq))) {
            Restorer restorer{*هذا};
            CONST_BITOP_RETURN_IF(!m_polarity, nodep);
            CONST_BITOP_RETURN_IF(m_lsb, nodep);  // the result of EQ/NE is 1 bit width
            ثابت AstNode* lhsp = nodep->lhsp();
            ܐܢ (ثابت AstCCast* ثابت castp = VN_CAST(lhsp, CCast)) lhsp = castp->lhsp();
            ثابت AstConst* ثابت constp = VN_CAST(lhsp, Const);
            CONST_BITOP_RETURN_IF(!constp, nodep->lhsp());

            ثابت V3Number& compNum = constp->num();

            auto setPolarities = [هذا, &compNum](ثابت LeafInfo& ref, ثابت V3Number* maskp) {
                ثابت منطقي maskFlip = isAndTree() ^ ref.polarity();
                صحيح constantWidth = compNum.width();
                ܐܢ (maskp) constantWidth = std::max(constantWidth, maskp->width());
                ثابت صحيح maxBitIdx = std::max(ref.lsb() + constantWidth, ref.msb() + 1);
                // Mark all bits checked by this comparison
                لكل (صحيح bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                    ثابت صحيح maskIdx = bitIdx - ref.lsb();
                    ثابت منطقي mask0 = maskp && maskp->bitIs0(maskIdx);
                    ثابت منطقي outOfRange = bitIdx > ref.msb();
                    ܐܢ (mask0 || outOfRange) {  // RHS is 0
                        ܐܢ (compNum.bitIs1(maskIdx)) {
                            // LHS is 1
                            // And tree: 1 == 0 => always false, set v && !v
                            // Or tree : 1 != 0 => always true, set v || !v
                            m_bitPolarities.emplace_back(ref, ܫܪܪܐ, 0);
                            m_bitPolarities.emplace_back(ref, ܕܓܠܐ, 0);
                            اكسر;
                        } ܐܚܪܢܐ {  // This bitIdx is irrelevant
                            استمر;
                        }
                    }
                    ثابت منطقي polarity = compNum.bitIs1(maskIdx) != maskFlip;
                    m_bitPolarities.emplace_back(ref, polarity, bitIdx);
                }
            };

            ܐܢ (ثابت AstAnd* ثابت andp = VN_CAST(nodep->rhsp(), And)) {  // comp == (mask & v)
                ثابت LeafInfo& mask = findLeaf(andp->lhsp(), ܫܪܪܐ);
                CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

                ثابت LeafInfo& ref = findLeaf(andp->rhsp(), ܕܓܠܐ);
                CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());

                restorer.disableRestore();  // Now all checks passed

                ثابت V3Number& maskNum = mask.constp()->num();

                incrOps(nodep, __LINE__);
                incrOps(andp, __LINE__);

                setPolarities(ref, &maskNum);
            } ܐܚܪܢܐ {  // comp == v
                ثابت LeafInfo& ref = findLeaf(nodep->rhsp(), ܕܓܠܐ);
                CONST_BITOP_RETURN_IF(!ref.refp(), nodep->rhsp());

                restorer.disableRestore();  // Now all checks passed

                incrOps(nodep, __LINE__);

                setPolarities(ref, nullptr);
            }
        } ܐܚܪܢܐ {
            CONST_BITOP_SET_FAILED("Mixture of different ops cannot be optimized", nodep);
        }
    }

    // CONSTRUCTORS
    ConstBitOpTreeVisitor(AstNodeExpr* nodep, unsigned externalOps, VNIdAllocator& ids)
        : m_ops{externalOps}
        , m_rootp{nodep}
        , m_ids{ids} {
        // Fill nullptr at [0] because a base index of 0 means not set yet
        m_varInfos.push_back(nullptr);
        CONST_BITOP_RETURN_IF(!isAndTree() && !isOrTree() && !isXorTree(), nodep);
        ܐܢ (AstNodeBiop* ثابت biopp = VN_CAST(nodep, NodeBiop)) {
            iterateConst(biopp);
        } ܐܚܪܢܐ {
            UASSERT_OBJ(VN_IS(nodep, RedXor), nodep, "Must be RedXor");
            incrOps(nodep, __LINE__);
            iterateChildrenConst(nodep);
        }
        لكل (auto&& entry : m_bitPolarities) {
            getVarInfo(entry.m_info).setPolarity(entry.m_polarity, entry.m_bit);
        }
        UASSERT_OBJ(isXorTree() || m_polarity, nodep, "must be the original polarity");
    }
    virtual ~ConstBitOpTreeVisitor() = افتراضي;
#undef CONST_BITOP_RETURN_IF
#undef CONST_BITOP_SET_FAILED

عام:
    // Transform as below.
    // v[0] & v[1] => 2'b11 == (2'b11 & v)
    // v[0] | v[1] => 2'b00 != (2'b11 & v)
    // v[0] ^ v[1] => ^{2'b11 & v}
    // (3'b011 == (3'b011 & v)) & v[2]  => 3'b111 == (3'b111 & v)
    // (3'b000 != (3'b011 & v)) | v[2]  => 3'b000 != (3'b111 & v)
    // Reduction ops are transformed in the same way.
    // &{v[0], v[1]} => 2'b11 == (2'b11 & v)
    ساكن AstNodeExpr* simplify(AstNodeExpr* nodep, صحيح resultWidth, unsigned externalOps,
                                 VDouble0& reduction, VNIdAllocator& ids) {
        UASSERT_OBJ(1 <= resultWidth && resultWidth <= 64, nodep, "resultWidth out of range");

        // Walk tree, gathering all terms referenced in expression
        ثابت ConstBitOpTreeVisitor visitor{nodep, externalOps, ids};

        // If failed on root node is not optimizable, or there are no variable terms, then done
        ܐܢ (visitor.m_failed || visitor.m_varInfos.size() == 1) ܦܢܐ nullptr;

        // FileLine used for constructing all new nodes in this function
        FileLine* ثابت fl = nodep->fileline();

        // Get partial result each term referenced, count total number of ops and keep track of
        // whether we have clean/dirty terms. visitor.m_varInfos appears in deterministic order,
        // so the optimized tree is deterministic as well.

        std::vector<AstNodeExpr*> termps;
        termps.reserve(visitor.m_varInfos.size() - 1);
        unsigned resultOps = 0;
        منطقي hasCleanTerm = ܕܓܠܐ;
        منطقي hasDirtyTerm = ܕܓܠܐ;

        لكل (auto&& v : visitor.m_varInfos) {
            ܐܢ (!v) استمر;  // Skip nullptr at m_varInfos[0]
            ܐܢ (v->hasConstResult()) {
                // If a constant term is known, we can either drop it or the whole tree is constant
                AstNodeExpr* resultp = nullptr;
                ܐܢ (v->getConstResult()) {
                    UASSERT_OBJ(visitor.isOrTree(), nodep,
                                "Only OR tree can yield known 1 result");
                    UINFO(9, "OR tree with const 1 term: " << v->refp());
                    // Known 1 bit in OR tree, whole result is 1
                    resultp = جديد AstConst{fl, AstConst::BitTrue{}};
                } ܐܚܪܢܐ ܐܢ (visitor.isAndTree()) {
                    UINFO(9, "AND tree with const 0 term: " << v->refp());
                    // Known 0 bit in AND tree, whole result is 0
                    resultp = جديد AstConst{fl, AstConst::BitFalse{}};
                } ܐܚܪܢܐ {
                    // Known 0 bit in OR or XOR tree. Ignore it.
                    استمر;
                }
                // Set width and widthMin precisely
                resultp->dtypeChgWidth(resultWidth, 1);
                لكل (AstNode* ثابت termp : termps) VL_DO_DANGLING(termp->deleteTree(), termp);
                ܦܢܐ resultp;
            }
            ثابت ResultTerm result = v->getResultTerm();
            termps.push_back(std::get<0>(result));
            resultOps += std::get<1>(result);
            ܐܢ (std::get<2>(result)) {
                hasCleanTerm = ܫܪܪܐ;
                UINFO(9, "Clean term: " << termps.back());
            } ܐܚܪܢܐ {
                hasDirtyTerm = ܫܪܪܐ;
                UINFO(9, "Dirty term: " << termps.back());
            }
        }

        // Group by FrozenNodeInfo
        std::map<FrozenNodeInfo, std::vector<AstNodeExpr*>> frozenNodes;
        // Check if frozen terms are clean or not
        لكل (ثابت auto& frozenInfo : visitor.m_frozenNodes) {
            AstNodeExpr* ثابت termp = frozenInfo.first;
            // Comparison operators are clean
            ܐܢ ((VN_IS(termp, Eq) || VN_IS(termp, Neq) || VN_IS(termp, Lt) || VN_IS(termp, Lte)
                 || VN_IS(termp, Gt) || VN_IS(termp, Gte))
                && frozenInfo.second.m_lsb == 0) {
                hasCleanTerm = ܫܪܪܐ;
            } ܐܚܪܢܐ {
                // Otherwise, conservatively assume the frozen term is dirty
                hasDirtyTerm = ܫܪܪܐ;
                UINFO(9, "Dirty frozen term: " << termp);
            }
            frozenNodes[frozenInfo.second].push_back(termp);
        }

        // Figure out if a final negation is required
        ثابت منطقي needsFlip = visitor.isXorTree() && !visitor.m_polarity;

        // Figure out if the final tree needs cleaning
        ثابت منطقي needsCleaning = visitor.isAndTree() ? !hasCleanTerm : hasDirtyTerm;

        // Add size of reduction tree to op count
        resultOps += termps.size() - 1;
        لكل (ثابت auto& lsbAndNodes : frozenNodes) {
            ܐܢ (lsbAndNodes.first.m_lsb > 0) ++resultOps;  // Needs AstShiftR
            ܐܢ (!lsbAndNodes.first.m_polarity) ++resultOps;  // Needs AstNot
            resultOps += lsbAndNodes.second.size();
        }
        // Add final polarity flip in Xor tree
        ܐܢ (needsFlip) ++resultOps;
        // Add final cleaning AND
        ܐܢ (needsCleaning) ++resultOps;

        ܐܢ (debug() >= 9) {  // LCOV_EXCL_START
            cout << "-  Bitop tree considered:\n";
            لكل (ثابت AstNodeExpr* ثابت termp : termps) termp->dumpTree("-  Reduced term: ");
            لكل (ثابت std::pair<AstNodeExpr*, FrozenNodeInfo>& termp : visitor.m_frozenNodes) {
                termp.first->dumpTree("-  Frozen term with lsb "
                                      + std::to_string(termp.second.m_lsb) + " polarity "
                                      + std::to_string(termp.second.m_polarity) + ": ");
            }
            cout << "-  Needs flipping: " << needsFlip << "\n";
            cout << "-  Needs cleaning: " << needsCleaning << "\n";
            cout << "-  Size: " << resultOps << " input size: " << visitor.m_ops << "\n";
        }  // LCOV_EXCL_STOP

        // Sometimes we have no terms left after ignoring redundant terms
        // (all of which were zeroes)
        ܐܢ (termps.empty() && visitor.m_frozenNodes.empty()) {
            reduction += visitor.m_ops;
            AstNodeExpr* ثابت resultp = needsFlip ? جديد AstConst{fl, AstConst::BitTrue{}}
                                                   : جديد AstConst{fl, AstConst::BitFalse{}};
            resultp->dtypeChgWidth(resultWidth, 1);
            ܦܢܐ resultp;
        }

        // Only substitute the result if beneficial as determined by operation count
        ܐܢ (visitor.m_ops <= resultOps) {
            لكل (AstNode* ثابت termp : termps) VL_DO_DANGLING(termp->deleteTree(), termp);
            ܦܢܐ nullptr;
        }

        // Update statistics
        reduction += visitor.m_ops - resultOps;

        // Reduction op to combine terms
        ثابت auto reduce = [&visitor, fl](AstNodeExpr* lhsp, AstNodeExpr* rhsp) -> AstNodeExpr* {
            ܐܢ (!lhsp) ܦܢܐ rhsp;
            ܐܢ (visitor.isAndTree()) {
                ܦܢܐ جديد AstAnd{fl, lhsp, rhsp};
            } ܐܚܪܢܐ ܐܢ (visitor.isOrTree()) {
                ܦܢܐ جديد AstOr{fl, lhsp, rhsp};
            } ܐܚܪܢܐ {
                ܦܢܐ جديد AstXor{fl, lhsp, rhsp};
            }
        };

        // Compute result by reducing all terms
        AstNodeExpr* resultp = nullptr;
        لكل (AstNodeExpr* ثابت termp : termps) {  //
            resultp = reduce(resultp, termp);
        }
        // Add any frozen terms to the reduction
        لكل (auto&& nodes : frozenNodes) {
            // nodes.second has same lsb and polarity
            AstNodeExpr* termp = nullptr;
            لكل (AstNodeExpr* ثابت itemp : nodes.second) {
                termp = reduce(termp, itemp->unlinkFrBack());
            }
            ܐܢ (nodes.first.m_lsb > 0) {  // LSB is not 0, so shiftR
                AstNodeDType* ثابت dtypep = termp->dtypep();
                termp = جديد AstShiftR{termp->fileline(), termp,
                                      جديد AstConst(termp->fileline(), AstConst::WidthedValue{},
                                                   termp->width(), nodes.first.m_lsb)};
                termp->dtypep(dtypep);
            }
            ܐܢ (!nodes.first.m_polarity) {  // Polarity is inverted, so append Not
                AstNodeDType* ثابت dtypep = termp->dtypep();
                termp = جديد AstNot{termp->fileline(), termp};
                termp->dtypep(dtypep);
            }
            resultp = reduce(resultp, termp);
        }

        // Set width of masks to expected result width. This is required to prevent later removal
        // of the masking node e.g. by the "AND with all ones" rule. If the result width happens
        // to be 1, we still need to ensure the AstAnd is not dropped, so use a wider mask in this
        // special case.
        ثابت صحيح maskWidth
            = std::max(resultp->width(), resultWidth == 1 ? VL_IDATASIZE : resultWidth);

        // Apply final polarity flip
        ܐܢ (needsFlip) {
            ܐܢ (needsCleaning) {
                // Cleaning will be added below. Use a NOT which is a byte shorter on x86
                resultp = جديد AstNot{fl, resultp};
            } ܐܚܪܢܐ {
                // Keep result clean by using XOR(1, _)
                AstConst* ثابت maskp = جديد AstConst{fl, AstConst::WidthedValue{}, maskWidth, 1};
                resultp = جديد AstXor{fl, maskp, resultp};
            }
        }

        // Apply final cleaning
        ܐܢ (needsCleaning) {
            AstConst* ثابت maskp = جديد AstConst{fl, AstConst::WidthedValue{}, maskWidth, 1};
            resultp = جديد AstAnd{fl, maskp, resultp};
        }

        // Cast back to original size if required
        ܐܢ (resultp->width() != resultWidth) {
            resultp = جديد AstCCast{fl, resultp, resultWidth, 1};
        }

        // Set width and widthMin precisely
        resultp->dtypeChgWidth(resultWidth, 1);

        ܦܢܐ resultp;
    }
};

//######################################################################
// Const state, as a visitor of each AstNode

صنف ConstVisitor final : عام VNVisitor {
    // CONSTANTS
    ساكن constexpr unsigned CONCAT_MERGABLE_MAX_DEPTH = 10;  // Limit alg recursion
    // Largest right-hand side that is copied to each part of a split concatenation assignment
    ساكن constexpr صحيح CONCAT_ASSIGN_MAX_RHS_NODES = 16;

    // NODE STATE
    // See VNIdAllocator. All per node state below is held in side tables keyed on the
    // node ids it hands out, as node pointers are not stable keys within V3Const.

    // STATE
    VNIdAllocator m_ids;  // Node id allocator
    منطقي m_params = ܕܓܠܐ;  // If true, propagate parameterized and true numbers only
    منطقي m_required = ܕܓܠܐ;  // If true, must become a constant
    منطقي m_wremove = ܫܪܪܐ;  // Inside scope, no assignw removal
    منطقي m_warn = ܕܓܠܐ;  // Output warnings
    منطقي m_doExpensive = ܕܓܠܐ;  // Enable computationally expensive optimizations
    منطقي m_doCpp = ܕܓܠܐ;  // Enable late-stage C++ optimizations
    منطقي m_doNConst = ܕܓܠܐ;  // Enable non-constant-child simplifications
    منطقي m_doV = ܕܓܠܐ;  // Verilog, not C++ conversion
    منطقي m_doGenerate = ܕܓܠܐ;  // Postpone width checking inside generate
    منطقي m_convertLogicToBit = ܕܓܠܐ;  // Convert logical operators to bitwise
    منطقي m_hasJumpDelay = ܕܓܠܐ;  // JumpGo or Delay under this loop
    منطقي m_hasLoopTest = ܕܓܠܐ;  // Contains AstLoopTest
    منطقي m_underRecFunc = ܕܓܠܐ;  // Under a recursive function
    AstNodeModule* m_modp = nullptr;  // Current module
    ثابت AstArraySel* m_selp = nullptr;  // Current select
    ثابت AstScope* m_scopep = nullptr;  // Current scope
    ثابت AstAttrOf* m_attrp = nullptr;  // Current attribute
    VDouble0 m_statBitOpReduction;  // Ops reduced in ConstBitOpTreeVisitor
    VDouble0 m_statConcatMerge;  // Concat merges
    VDouble0 m_statCondExprRedundant;  // Conditional repeated expressions
    VDouble0 m_statIfCondExprRedundant;  // Conditional repeated expressions
    VDouble0 m_statMemberAccessVisits;  // Nodes visited by containsMemberAccessRecurse
    ثابت منطقي m_globalPass;  // ConstVisitor invoked as a global pass
    ساكن uint32_t s_globalPassNum;  // Counts number of times ConstVisitor invoked as global pass
    V3UniqueNames m_concswapNames;  // For generating unique temporary variable names
    // Caches results of matchBiopToBitwise, keyed on node id
    std::unordered_map<size_t, منطقي> m_containsMemberAccess;
    // Items of enum currently being recursed into. Cannot use VNIdAllocator, as this runs in
    // every mode, including those invoked under a pass holding user4 (V3Param). Node pointers
    // are safe as keys here, unlike elsewhere in V3Const: folding the item value below does
    // free nodes whose addresses are then reused, but AstEnumItem is only ever constructed
    // while parsing (verilog.y and V3LinkParse), so a reused address can never become one,
    // and every key looked up is an AstEnumItem.
    std::unordered_set<ثابت AstEnumItem*> m_recursingEnumItems;
    std::unordered_set<AstJumpBlock*> m_usedJumpBlocks;  // JumpBlocks used by some JumpGo

    // METHODS

    // Return true if statements may be deleted. Until statics are lifted, a variable declared
    // in a statement might be referenced from elsewhere (e.g. hierarchically).
    ساكن منطقي canDeleteStatements() { ܦܢܐ v3Global.staticsLifted(); }

    فراغ deleteVarScopesUnder(AstNode* subtreep) {
        ܐܢ (!subtreep) ܦܢܐ;
        ܐܢ (!m_scopep) ܦܢܐ;
        std::unordered_set<AstVar*> varps;
        subtreep->foreachAndNext([&](AstVar* varp) { varps.insert(varp); });
        ܐܢ (varps.empty()) ܦܢܐ;
        لكل (AstVarScope *vscp = m_scopep->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            ܐܢ (varps.find(vscp->varp()) != varps.end()) {
                VL_DO_DANGLING(pushDeletep(vscp->unlinkFrBack()), vscp);
            }
        }
    }

    V3Number constNumV(AstNode* nodep) {
        // Contract C width to V width (if needed, else just direct copy)
        // The upper zeros in the C representation can otherwise cause
        // wrong results in some operations, e.g. MulS
        ثابت V3Number& numc = VN_AS(nodep, Const)->num();
        ܦܢܐ !numc.isNumber() ? numc : V3Number{nodep, nodep->widthMinV(), numc};
    }
    V3Number toNumC(AstNode* nodep, ثابت V3Number& numv) {
        // Extend V width back to C width for given node
        ܦܢܐ !numv.isNumber() ? numv : V3Number{nodep, nodep->width(), numv};
    }

    ساكن منطقي lowerAsFixedAggregate(ثابت AstNodeDType* ثابت dtypep) {
        ܦܢܐ dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }

    AstStructSel* newStructSel(AstNodeExpr* ثابت fromp, ثابت AstMemberDType* ثابت itemp) {
        AstStructSel* ثابت selp = جديد AstStructSel{fromp->fileline(), fromp, itemp->name()};
        selp->dtypeFrom(itemp->dtypep());
        ܦܢܐ selp;
    }

    فراغ collectFixedAggregateTerms(AstNodeExpr* ثابت fromp, std::vector<AstNodeExpr*>& termps,
                                    ثابت منطقي packReal) {
        ثابت AstNodeDType* ثابت dtypep = fromp->dtypep()->skipRefp();
        ܐܢ (ثابت AstUnpackArrayDType* ثابت unpackDtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            لكل (ثابت صحيح idx : unpackDtypep->declRange().seqLeftToRight()) {
                AstArraySel* ثابت selp
                    = جديد AstArraySel{fromp->fileline(), fromp->cloneTreePure(ܕܓܠܐ), idx};
                collectFixedAggregateTerms(selp, termps, packReal);
            }
            VL_DO_DANGLING(pushDeletep(fromp), fromp);
        } ܐܚܪܢܐ ܐܢ (ثابت AstNodeUOrStructDType* ثابت sdtypep
                   = VN_CAST(dtypep, NodeUOrStructDType)) {
            ܐܢ (sdtypep->packed()) {
                termps.push_back(fromp);
                ܦܢܐ;
            }
            لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                collectFixedAggregateTerms(newStructSel(fromp->cloneTreePure(ܕܓܠܐ), itemp),
                                           termps, packReal);
            }
            VL_DO_DANGLING(pushDeletep(fromp), fromp);
        } ܐܚܪܢܐ ܐܢ (packReal && dtypep->isDouble()) {
            termps.push_back(جديد AstRealToBits{fromp->fileline(), fromp});
        } ܐܚܪܢܐ {
            termps.push_back(fromp);
        }
    }

    AstNodeExpr* packFixedAggregate(AstNodeExpr* ثابت fromp) {
        std::vector<AstNodeExpr*> termps;
        collectFixedAggregateTerms(fromp, termps, ܫܪܪܐ);
        UASSERT(!termps.empty(), "No stream terms");
        AstNodeExpr* resultp = termps[0];
        لكل (size_t i = 1; i < termps.size(); ++i) {
            resultp = جديد AstConcat{resultp->fileline(), resultp, termps[i]};
        }
        ܦܢܐ resultp;
    }

    فراغ replaceAssignToFixedAggregate(AstNodeAssign* ثابت nodep, AstNodeExpr* ثابت dstp,
                                       AstNodeExpr* srcp) {
        ثابت صحيح dstWidth = dstp->dtypep()->widthStream();
        std::vector<AstNodeExpr*> termps;
        collectFixedAggregateTerms(dstp, termps, ܕܓܠܐ);
        صحيح srcWidth = srcp->width();
        ܐܢ (srcWidth < dstWidth) {
            AstExtend* ثابت extendp = جديد AstExtend{srcp->fileline(), srcp, dstWidth};
            srcp = جديد AstShiftL{
                extendp->fileline(), extendp,
                جديد AstConst{extendp->fileline(), static_cast<uint32_t>(dstWidth - srcWidth)},
                dstWidth};
            srcWidth = dstWidth;
        }
        AstNodeAssign* newp = nullptr;
        صحيح offset = 0;
        لكل (size_t i = 0; i < termps.size(); ++i) {
            AstNodeExpr* ثابت termp = termps[i];
            ثابت صحيح width = termp->dtypep()->widthStream();
            ثابت صحيح lsb = srcWidth - offset - width;
            offset += width;
            AstNodeExpr* rhsp
                = جديد AstSel{srcp->fileline(), srcp->cloneTreePure(ܕܓܠܐ), lsb, width};
            ܐܢ (termp->dtypep()->skipRefp()->isDouble()) {
                rhsp = جديد AstBitsToRealD{rhsp->fileline(), rhsp};
            }
            AstNodeAssign* ثابت assignp = nodep->cloneType(termp, rhsp);
            assignp->dtypeFrom(termp);
            newp = AstNode::addNext(newp, assignp);
        }
        nodep->addNextHere(newp);
        VL_DO_DANGLING(pushDeletep(srcp), srcp);
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }

    منطقي operandConst(ثابت AstNode* nodep) { ܦܢܐ VN_IS(nodep, Const); }
    منطقي operandAsvConst(ثابت AstNode* nodep) {
        // BIASV(CONST, BIASV(CONST,...)) -> BIASV( BIASV_CONSTED(a,b), ...)
        ثابت AstNodeBiComAsv* ثابت bnodep = VN_CAST(nodep, NodeBiComAsv);
        ܐܢ (!bnodep) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!VN_IS(bnodep->lhsp(), Const)) ܦܢܐ ܕܓܠܐ;
        ثابت AstNodeBiComAsv* ثابت rnodep = VN_CAST(bnodep->rhsp(), NodeBiComAsv);
        ܐܢ (!rnodep) ܦܢܐ ܕܓܠܐ;
        ܐܢ (rnodep->type() != bnodep->type()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (rnodep->width() != bnodep->width()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (rnodep->lhsp()->width() != bnodep->lhsp()->width()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!VN_IS(rnodep->lhsp(), Const)) ܦܢܐ ܕܓܠܐ;
        ܦܢܐ ܫܪܪܐ;
    }
    منطقي operandAsvSame(ثابت AstNode* nodep) {
        // BIASV(SAMEa, BIASV(SAMEb,...)) -> BIASV( BIASV(SAMEa,SAMEb), ...)
        ثابت AstNodeBiComAsv* ثابت bnodep = VN_CAST(nodep, NodeBiComAsv);
        ܐܢ (!bnodep) ܦܢܐ ܕܓܠܐ;
        ثابت AstNodeBiComAsv* ثابت rnodep = VN_CAST(bnodep->rhsp(), NodeBiComAsv);
        ܐܢ (!rnodep) ܦܢܐ ܕܓܠܐ;
        ܐܢ (rnodep->type() != bnodep->type()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (rnodep->width() != bnodep->width()) ܦܢܐ ܕܓܠܐ;
        ܦܢܐ operandsSame(bnodep->lhsp(), rnodep->lhsp());
    }
    منطقي operandAsvLUp(ثابت AstNode* nodep) {
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
        ثابت AstNodeBiComAsv* ثابت bnodep = VN_CAST(nodep, NodeBiComAsv);
        ܐܢ (!bnodep) ܦܢܐ ܕܓܠܐ;
        ثابت AstNodeBiComAsv* ثابت lnodep = VN_CAST(bnodep->lhsp(), NodeBiComAsv);
        ܐܢ (!lnodep) ܦܢܐ ܕܓܠܐ;
        ܐܢ (lnodep->type() != bnodep->type()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (lnodep->width() != bnodep->width()) ܦܢܐ ܕܓܠܐ;
        ܦܢܐ VN_IS(lnodep->lhsp(), Const);
    }
    منطقي operandAsvRUp(ثابت AstNode* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr)) ?
        ثابت AstNodeBiComAsv* ثابت bnodep = VN_CAST(nodep, NodeBiComAsv);
        ܐܢ (!bnodep) ܦܢܐ ܕܓܠܐ;
        ثابت AstNodeBiComAsv* ثابت rnodep = VN_CAST(bnodep->rhsp(), NodeBiComAsv);
        ܐܢ (!rnodep) ܦܢܐ ܕܓܠܐ;
        ܐܢ (rnodep->type() != bnodep->type()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (rnodep->width() != bnodep->width()) ܦܢܐ ܕܓܠܐ;
        ܦܢܐ VN_IS(rnodep->lhsp(), Const);
    }
    ساكن منطقي operandSubAdd(ثابت AstNode* nodep) {
        // SUB( ADD(CONSTx,y), CONSTz) -> ADD(SUB(CONSTx,CONSTz), y)
        ثابت AstNodeBiop* ثابت np = VN_CAST(nodep, NodeBiop);
        ثابت AstNodeBiop* ثابت lp = VN_CAST(np->lhsp(), NodeBiop);
        ܦܢܐ (lp && VN_IS(lp->lhsp(), Const) && VN_IS(np->rhsp(), Const)
                && lp->width() == np->width());
    }
    منطقي matchRedundantClean(AstAnd* andp) {
        // Remove And with constant one inserted by V3Clean
        // 1 & (a == b)  -> (IData)(a == b)
        // When bool is casted to int, the value is either 0 or 1
        AstConst* ثابت constp = VN_AS(andp->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isOne(), andp->lhsp(), "TRREEOPC must meet this condition");
        AstNodeExpr* ثابت rhsp = andp->rhsp();
        AstCCast* ccastp = nullptr;
        ثابت auto isEqOrNeq = [](ثابت AstNode* nodep) -> منطقي {  //
            ܦܢܐ VN_IS(nodep, Eq) || VN_IS(nodep, Neq);
        };
        ܐܢ (isEqOrNeq(rhsp)) {
            ccastp = جديد AstCCast{andp->fileline(), rhsp->unlinkFrBack(), andp};
        } ܐܚܪܢܐ ܐܢ (AstCCast* ثابت tmpp = VN_CAST(rhsp, CCast)) {
            ܐܢ (isEqOrNeq(tmpp->lhsp())) {
                ܐܢ (tmpp->width() == andp->width()) {
                    tmpp->unlinkFrBack();
                    ccastp = tmpp;
                } ܐܚܪܢܐ {
                    ccastp = جديد AstCCast{andp->fileline(), tmpp->lhsp()->unlinkFrBack(), andp};
                }
            }
        }
        ܐܢ (ccastp) {
            andp->replaceWithKeepDType(ccastp);
            VL_DO_DANGLING(pushDeletep(andp), andp);
            ܦܢܐ ܫܪܪܐ;
        }
        ܦܢܐ ܕܓܠܐ;
    }

    ساكن منطقي operandAndOrSame(ثابت AstNode* nodep) {
        // OR( AND(VAL,x), AND(VAL,y)) -> AND(VAL,OR(x,y))
        // OR( AND(x,VAL), AND(y,VAL)) -> AND(OR(x,y),VAL)
        ثابت AstNodeBiop* ثابت np = VN_CAST(nodep, NodeBiop);
        ثابت AstNodeBiop* ثابت lp = VN_CAST(np->lhsp(), NodeBiop);
        ثابت AstNodeBiop* ثابت rp = VN_CAST(np->rhsp(), NodeBiop);
        ܦܢܐ (lp && rp && lp->width() == rp->width() && lp->type() == rp->type()
                && (operandsSame(lp->lhsp(), rp->lhsp()) || operandsSame(lp->rhsp(), rp->rhsp())));
    }
    منطقي matchOrAndNot(AstNodeBiop* nodep) {
        // AstOr{$a, AstAnd{AstNot{$b}, $c}} if $a.width1, $a==$b => AstOr{$a,$c}
        // Someday we'll sort the biops completely and this can be simplified
        // This often results from our simplified clock generation:
        // if (rst) ... else if (enable)... -> OR(rst,AND(!rst,enable))
        ثابت AstNodeExpr* ap;
        AstNodeBiop* andp;
        ܐܢ (VN_IS(nodep->lhsp(), And)) {
            andp = VN_AS(nodep->lhsp(), And);
            ap = nodep->rhsp();
        } ܐܚܪܢܐ ܐܢ (VN_IS(nodep->rhsp(), And)) {
            andp = VN_AS(nodep->rhsp(), And);
            ap = nodep->lhsp();
        } ܐܚܪܢܐ {
            ܦܢܐ ܕܓܠܐ;
        }
        ثابت AstNodeUniop* notp;
        AstNodeExpr* cp;
        ܐܢ (VN_IS(andp->lhsp(), Not)) {
            notp = VN_AS(andp->lhsp(), Not);
            cp = andp->rhsp();
        } ܐܚܪܢܐ ܐܢ (VN_IS(andp->rhsp(), Not)) {
            notp = VN_AS(andp->rhsp(), Not);
            cp = andp->lhsp();
        } ܐܚܪܢܐ {
            ܦܢܐ ܕܓܠܐ;
        }
        ثابت AstNodeExpr* ثابت bp = notp->lhsp();
        ܐܢ (!operandsSame(ap, bp)) ܦܢܐ ܕܓܠܐ;
        // Do it
        cp->unlinkFrBack();
        VL_DO_DANGLING(pushDeletep(andp->unlinkFrBack()), andp);
        VL_DANGLING(notp);
        // Replace whichever branch is now dangling
        ܐܢ (nodep->rhsp()) {
            nodep->lhsp(cp);
        } ܐܚܪܢܐ {
            nodep->rhsp(cp);
        }
        ܦܢܐ ܫܪܪܐ;
    }
    منطقي matchAndCond(AstAnd* nodep) {
        // Push down a AND into conditional, when one side of conditional is constant
        // (otherwise we'd be trading one operation for two operations)
        // V3Clean often makes this pattern, as it postpones the AND until
        // as high as possible, which is usually the right choice, except for this.
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstCond* ثابت condp = VN_CAST(nodep->rhsp(), Cond);
        ܐܢ (!condp) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!VN_IS(condp->thenp(), Const) && !VN_IS(condp->elsep(), Const)) ܦܢܐ ܕܓܠܐ;
        AstConst* ثابت maskp = VN_CAST(nodep->lhsp(), Const);
        ܐܢ (!maskp) ܦܢܐ ܕܓܠܐ;
        UINFO(4, "AND(CONSTm, CONDcond(c, i, e))->CONDcond(c, AND(m,i), AND(m, e)) " << nodep);
        AstCond* ثابت newp = جديد AstCond{
            condp->fileline(), condp->condp()->unlinkFrBack(),
            جديد AstAnd{nodep->fileline(), maskp->cloneTree(ܕܓܠܐ), condp->thenp()->unlinkFrBack()},
            جديد AstAnd{nodep->fileline(), maskp->cloneTree(ܕܓܠܐ),
                       condp->elsep()->unlinkFrBack()}};
        newp->thenp()->dtypeFrom(nodep);  // As And might have been to change widths
        newp->elsep()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        ܦܢܐ ܫܪܪܐ;
    }

    منطقي matchCondCond(ثابت AstCond* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        ܐܢ (VN_IS(nodep->backp(), Cond)) ܦܢܐ ܕܓܠܐ;  // Was checked when visited parent
        ܐܢ (!VN_IS(nodep->thenp(), Cond) && !VN_IS(nodep->elsep(), Cond))
            ܦܢܐ ܕܓܠܐ;  // Short circuit
        std::vector<AstNodeExpr*> truesp;
        std::vector<AstNodeExpr*> falsesp;
        matchCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
        ܦܢܐ ܕܓܠܐ;  // Can optimize further
    }
    فراغ matchCondCondRecurse(ثابت AstCond* nodep, std::vector<AstNodeExpr*>& truesp,
                              std::vector<AstNodeExpr*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        ܐܢ (truesp.size() > 4 || falsesp.size() > 4) ܦܢܐ;
        ܐܢ (!nodep->condp()->isPure()) ܦܢܐ;
        منطقي replaced = ܕܓܠܐ;
        لكل (ثابت AstNodeExpr* condp : truesp) {
            ܐܢ (replaced) اكسر;
            ܐܢ (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replaceNum(nodep->condp(), 1);
            replaced = ܫܪܪܐ;
            ++m_statCondExprRedundant;
        }
        لكل (ثابت AstNodeExpr* condp : falsesp) {
            ܐܢ (replaced) اكسر;
            ܐܢ (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = ܫܪܪܐ;
            ++m_statCondExprRedundant;
        }
        ܐܢ (ثابت AstCond* subCondp = VN_CAST(nodep->thenp(), Cond)) {
            ܐܢ (!replaced) truesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            ܐܢ (!replaced) truesp.pop_back();
        }
        ܐܢ (ثابت AstCond* subCondp = VN_CAST(nodep->elsep(), Cond)) {
            ܐܢ (!replaced) falsesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            ܐܢ (!replaced) falsesp.pop_back();
        }
    }
    فراغ matchIfCondCond(ثابت AstNodeIf* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        ܐܢ (VN_IS(nodep->backp(), If)) ܦܢܐ;  // Was checked when visited parent
        ܐܢ (!VN_IS(nodep->thensp(), If) && !VN_IS(nodep->elsesp(), If)) ܦܢܐ;  // Short circuit
        std::vector<AstNodeExpr*> truesp;
        std::vector<AstNodeExpr*> falsesp;
        matchIfCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
    }
    فراغ matchIfCondCondRecurse(ثابت AstNodeIf* nodep, std::vector<AstNodeExpr*>& truesp,
                                std::vector<AstNodeExpr*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        ܐܢ (truesp.size() > 4 || falsesp.size() > 4) ܦܢܐ;
        ܐܢ (!nodep->condp()->isPure()) ܦܢܐ;
        // A constant condition is folded by visit(AstNodeIf), once statements can be deleted
        ܐܢ (VN_IS(nodep->condp(), Const)) ܦܢܐ;
        منطقي replaced = ܕܓܠܐ;
        لكل (ثابت AstNodeExpr* condp : truesp) {
            ܐܢ (replaced) اكسر;
            ܐܢ (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replaceNum(nodep->condp(), 1);
            replaced = ܫܪܪܐ;
            ++m_statIfCondExprRedundant;
        }
        لكل (ثابت AstNodeExpr* condp : falsesp) {
            ܐܢ (replaced) اكسر;
            ܐܢ (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = ܫܪܪܐ;
            ++m_statIfCondExprRedundant;
        }
        // We only check the first statement of parent IF is an If
        // So we don't need to check for effects in the executing thensp/elsesp
        // altering the child't condition.  e.g. 'if (x) begin x=1; if (x) end'
        ܐܢ (ثابت AstNodeIf* subIfp = VN_CAST(nodep->thensp(), NodeIf)) {
            ܐܢ (!replaced) truesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            ܐܢ (!replaced) truesp.pop_back();
        }
        ܐܢ (ثابت AstNodeIf* subIfp = VN_CAST(nodep->elsesp(), NodeIf)) {
            ܐܢ (!replaced) falsesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            ܐܢ (!replaced) falsesp.pop_back();
        }
    }

    فراغ matchIfSameLast(AstNodeIf* nodep) {
        ܐܢ (!nodep->thensp()) ܦܢܐ;
        ܐܢ (!nodep->elsesp()) ܦܢܐ;
        ܐܢ (!canDeleteStatements()) ܦܢܐ;
        // Iterate bodies in reverse
        AstNode* tp = nodep->thensp()->lastp();
        AstNode* ep = nodep->elsesp()->lastp();
        ܟܡܐ (tp && ep && tp->sameTree(ep)) {
            UASSERT_OBJ(!tp->nextp(), tp, "Expected no nextp");
            UASSERT_OBJ(!ep->nextp(), ep, "Expected no nextp");
            AstNode* ثابت tPrevp = tp->prevp();
            AstNode* ثابت ePrevp = ep->prevp();
            nodep->addNextHere(tp->unlinkFrBack());
            VL_DO_DANGLING(pushDeletep(ep->unlinkFrBack()), ep);
            tp = tPrevp;
            ep = ePrevp;
        }
    }

    منطقي matchMaskedOr(AstAnd* nodep) {
        // Masking an OR with terms that have no bits set under the mask is replaced with masking
        // only the remaining terms. Canonical example as generated by V3Expand is:
        // 0xff & (a << 8 | b >> 24) --> 0xff & (b >> 24)

        // Compute how many significant bits are in the mask
        ثابت AstConst* ثابت constp = VN_AS(nodep->lhsp(), Const);
        ثابت uint32_t significantBits = constp->num().widthToFit();

        AstOr* ثابت orp = VN_AS(nodep->rhsp(), Or);

        // Predicate for checking whether the bottom 'significantBits' bits of the given expression
        // are all zeroes.
        ثابت auto checkBottomClear = [=](ثابت AstNode* nodep) -> منطقي {
            ܐܢ (ثابت AstShiftL* ثابت shiftp = VN_CAST(nodep, ShiftL)) {
                ܐܢ (ثابت AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                    ܦܢܐ scp->num().toUInt() >= significantBits;
                }
            }
            ܦܢܐ ܕܓܠܐ;
        };

        ثابت منطقي orLIsRedundant = checkBottomClear(orp->lhsp());
        ثابت منطقي orRIsRedundant = checkBottomClear(orp->rhsp());

        ܐܢ (orLIsRedundant && orRIsRedundant) {
            nodep->replaceWithKeepDType(
                جديد AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            ܦܢܐ ܫܪܪܐ;
        } ܐܚܪܢܐ ܐܢ (orLIsRedundant) {
            orp->replaceWithKeepDType(orp->rhsp()->unlinkFrBack());
            VL_DO_DANGLING(pushDeletep(orp), orp);
            ܦܢܐ ܕܓܠܐ;  // input node is still valid, keep going
        } ܐܚܪܢܐ ܐܢ (orRIsRedundant) {
            orp->replaceWithKeepDType(orp->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(pushDeletep(orp), orp);
            ܦܢܐ ܕܓܠܐ;  // input node is still valid, keep going
        } ܐܚܪܢܐ {
            ܦܢܐ ܕܓܠܐ;
        }
    }
    منطقي matchMaskedShift(AstAnd* nodep) {
        // Drop redundant masking of right shift result. E.g: 0xff & ((uint32_t)a >> 24). This
        // commonly appears after V3Expand and the simplification in matchMaskedOr. Similarly,
        // drop redundant masking of left shift result. E.g.: 0xff000000 & ((uint32_t)a << 24).

        ثابت auto checkMask = [nodep, هذا](ثابت V3Number& mask) -> منطقي {
            ثابت AstConst* ثابت constp = VN_AS(nodep->lhsp(), Const);
            ܐܢ (constp->num().isCaseEq(mask)) {
                AstNode* ثابت rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                nodep->replaceWithKeepDType(rhsp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ܦܢܐ ܫܪܪܐ;
            }
            ܦܢܐ ܕܓܠܐ;
        };

        // Check if masking is redundant
        ܐܢ (ثابت AstShiftR* ثابت shiftp = VN_CAST(nodep->rhsp(), ShiftR)) {
            ܐܢ (ثابت AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3Number maskLo{nodep, nodep->width()};
                maskLo.setMask(nodep->width() - scp->num().toUInt());
                ܦܢܐ checkMask(maskLo);
            }
        } ܐܚܪܢܐ ܐܢ (ثابت AstShiftL* ثابت shiftp = VN_CAST(nodep->rhsp(), ShiftL)) {
            ܐܢ (ثابت AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3Number mask{nodep, nodep->width()};
                ثابت uint32_t shiftAmount = scp->num().toUInt();
                mask.setMask(nodep->width() - shiftAmount, shiftAmount);
                ܦܢܐ checkMask(mask);
            }
        }
        ܦܢܐ ܕܓܠܐ;
    }

    منطقي matchMaskedZero(ثابت AstAnd* nodep) {
        // Turn masking of known zero bits into constant zero. Commonly appears after V3Expand.
        ثابت AstConst* ثابت maskp = VN_AS(nodep->lhsp(), Const);
        ثابت AstNodeExpr* ثابت rhsp = nodep->rhsp();
        ثابت uint32_t msbP1 = maskp->num().mostSetBitP1();
        ܐܢ (!msbP1) ܦܢܐ ܕܓܠܐ;  // Don't rewrite, separate rule matches for this
        ثابت uint32_t msb = msbP1 - 1;
        ثابت uint32_t lsb = maskp->num().leastSetBitP1() - 1;

        ܐܢ (ثابت AstShiftL* ثابت shiftp = VN_CAST(rhsp, ShiftL)) {
            // 'a << S' forces the low S bits to zero
            ܐܢ (AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                ܦܢܐ scp->num().fitsInUInt()  //
                       && (scp->num().toUInt() > msb);
            }
        }
        ܐܢ (ثابت AstShiftR* ثابت shiftp = VN_CAST(rhsp, ShiftR)) {
            // 'a >> S' forces the high S bits to zero. Check against the width of the shifted
            // operand, V3Expand can create shifts wider than their inputs. Must use width(),
            // not widthMin(), as bits above widthMin() are not guaranteed to be zero.
            ܐܢ (AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                ܦܢܐ scp->num().fitsInUInt()
                       && (lsb + scp->num().toUInt()
                           >= static_cast<uint32_t>(shiftp->lhsp()->width()));
            }
        }
        ܐܢ (ثابت AstMul* ثابت mulp = VN_CAST(rhsp, Mul)) {
            // 'C * a' forces the low N bits to zero where 'C' has low zero bits
            ܐܢ (AstConst* ثابت cp = VN_CAST(mulp->lhsp(), Const)) {
                ܦܢܐ cp->num().leastSetBitP1() > msb + 1;
            }
        }
        ܐܢ (ثابت AstExtend* ثابت extendp = VN_CAST(rhsp, Extend)) {
            // Zero-extension forces the bits above the source width to zero
            ܦܢܐ lsb >= static_cast<uint32_t>(extendp->lhsp()->width());
        }

        ܦܢܐ ܕܓܠܐ;
    }

    منطقي matchBitOpTree(AstNodeExpr* nodep) {
        ܐܢ (nodep->widthMin() != 1) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!v3Global.opt.fConstBitOpTree()) ܦܢܐ ܕܓܠܐ;

        ثابت صحيح width = nodep->width();
        AstNodeExpr* rootp = nodep;
        unsigned externalOps = 0;
        // Reach past a plain making AND
        ܐܢ (ثابت AstAnd* ثابت andp = VN_CAST(nodep, And)) {
            ܐܢ (isConst(andp->lhsp(), 1)) {
                rootp = andp->rhsp();
                externalOps = 1;
            }
        }

        // Only optimize if rootp is in fact the root of a tree of identical
        // operations, otherwise analysis on an eventually unoptimizable unablanced
        // tree can go O(N^2) as we would re-analyze every time as we move up the
        // tree, repeatedly re-discover the sub-tree is not optimizable.
        ܐܢ (rootp->type() == nodep->backp()->type()) ܦܢܐ ܕܓܠܐ;

        std::string debugPrefix;
        ܐܢ (debug() >= 9) {  // LCOV_EXCL_START
            ساكن صحيح s_c = 0;
            debugPrefix = "-  matchBitOpTree[" + std::to_string(++s_c) + "] ";
            nodep->dumpTree(debugPrefix + "INPUT: ");
        }  // LCOV_EXCL_STOP

        AstNodeExpr* ثابت newp = ConstBitOpTreeVisitor::simplify(rootp, width, externalOps,
                                                                  m_statBitOpReduction, m_ids);

        ܐܢ (newp) {
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Transformed leaf of bit tree to " << newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }

        ܐܢ (debug() >= 9) {  // LCOV_EXCL_START
            ܐܢ (newp) {
                newp->dumpTree(debugPrefix + "RESULT: ");
            } ܐܚܪܢܐ {
                cout << debugPrefix << "not replaced\n";
            }
        }  // LCOV_EXCL_STOP

        ܦܢܐ newp;
    }
    ساكن منطقي operandShiftSame(ثابت AstNode* nodep) {
        ثابت AstNodeBiop* ثابت np = VN_AS(nodep, NodeBiop);
        {
            ثابت AstShiftL* ثابت lp = VN_CAST(np->lhsp(), ShiftL);
            ثابت AstShiftL* ثابت rp = VN_CAST(np->rhsp(), ShiftL);
            ܐܢ (lp && rp) {
                ܦܢܐ (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        {
            ثابت AstShiftR* ثابت lp = VN_CAST(np->lhsp(), ShiftR);
            ثابت AstShiftR* ثابت rp = VN_CAST(np->rhsp(), ShiftR);
            ܐܢ (lp && rp) {
                ܦܢܐ (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        ܦܢܐ ܕܓܠܐ;
    }
    منطقي operandHugeShiftL(ثابت AstNodeBiop* nodep) {
        ܦܢܐ (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->num().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->num().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->width())))
                && nodep->lhsp()->isPure());
    }
    منطقي operandHugeShiftR(ثابت AstNodeBiop* nodep) {
        ܦܢܐ (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->num().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->num().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->lhsp()->width())))
                && nodep->lhsp()->isPure()
                && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)));
    }
    منطقي operandIsTwo(ثابت AstNode* nodep) {
        ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
        ܐܢ (!constp) ܦܢܐ ܕܓܠܐ;  // not constant
        ܐܢ (constp->num().isFourState()) ܦܢܐ ܕܓܠܐ;  // four-state
        ܐܢ (nodep->width() > VL_QUADSIZE) ܦܢܐ ܕܓܠܐ;  // too wide
        ܐܢ (nodep->isSigned() && constp->num().isNegative()) ܦܢܐ ܕܓܠܐ;  // signed and negative
        ܦܢܐ constp->toUQuad() == 2;
    }
    منطقي operandIsTwostate(ثابت AstNode* nodep) {
        ܦܢܐ (VN_IS(nodep, Const) && !VN_AS(nodep, Const)->num().isFourState());
    }
    منطقي operandIsPowTwo(ثابت AstNode* nodep) {
        ܐܢ (!operandIsTwostate(nodep)) ܦܢܐ ܕܓܠܐ;
        ܦܢܐ (1 == VN_AS(nodep, Const)->num().countOnes());
    }
    منطقي operandShiftOp(ثابت AstNodeBiop* nodep) {
        ܐܢ (!VN_IS(nodep->rhsp(), Const)) ܦܢܐ ܕܓܠܐ;
        ثابت AstNodeBiop* ثابت lhsp = VN_CAST(nodep->lhsp(), NodeBiop);
        ܐܢ (!lhsp || !(VN_IS(lhsp, And) || VN_IS(lhsp, Or) || VN_IS(lhsp, Xor))) ܦܢܐ ܕܓܠܐ;
        ܐܢ (nodep->width() != lhsp->width()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (nodep->width() != lhsp->lhsp()->width()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (nodep->width() != lhsp->rhsp()->width()) ܦܢܐ ܕܓܠܐ;

        // Only do it if we can fruther reduce one of the sides if pulling the shift through:
        // - Either operands are constants
        ܐܢ (VN_IS(lhsp->lhsp(), Const)) ܦܢܐ ܫܪܪܐ;
        ܐܢ (VN_IS(lhsp->rhsp(), Const)) ܦܢܐ ܫܪܪܐ;
        // - Const shift on either side
        ܐܢ (AstNodeBiop* ثابت llp = VN_CAST(lhsp->lhsp(), NodeBiop)) {
            ܦܢܐ (VN_IS(llp, ShiftL) || VN_IS(llp, ShiftR)) && VN_IS(llp->rhsp(), Const);
        }
        ܐܢ (AstNodeBiop* ثابت lrp = VN_CAST(lhsp->rhsp(), NodeBiop)) {
            ܦܢܐ (VN_IS(lrp, ShiftL) || VN_IS(lrp, ShiftR)) && VN_IS(lrp->rhsp(), Const);
        }
        // Otherwise we would increase logic size
        ܦܢܐ ܕܓܠܐ;
    }
    منطقي operandShiftShift(ثابت AstNodeBiop* nodep) {
        // We could add a AND though.
        ثابت AstNodeBiop* ثابت lhsp = VN_CAST(nodep->lhsp(), NodeBiop);
        ܐܢ (!lhsp || !(VN_IS(lhsp, ShiftL) || VN_IS(lhsp, ShiftR))) ܦܢܐ ܕܓܠܐ;
        // We can only get rid of a<<b>>c or a<<b<<c, with constant b & c
        // because bits may be masked in that process, or (b+c) may exceed the word width.
        ܐܢ (!(VN_IS(nodep->rhsp(), Const) && VN_IS(lhsp->rhsp(), Const))) ܦܢܐ ܕܓܠܐ;
        ܐܢ (VN_AS(nodep->rhsp(), Const)->num().isFourState()
            || VN_AS(lhsp->rhsp(), Const)->num().isFourState())
            ܦܢܐ ܕܓܠܐ;
        ܐܢ (nodep->width() != lhsp->width()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (nodep->width() != lhsp->lhsp()->width()) ܦܢܐ ܕܓܠܐ;
        ܦܢܐ ܫܪܪܐ;
    }
    منطقي operandWordOOB(ثابت AstWordSel* nodep) {
        // V3Expand may make a arraysel that exceeds the bounds of the array
        // It was an expression, then got constified.  In reality, the WordSel
        // must be wrapped in a Cond, that will be false.
        ܦܢܐ (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), NodeVarRef)
                && VN_AS(nodep->fromp(), NodeVarRef)->access().isReadOnly()
                && (static_cast<صحيح>(VN_AS(nodep->bitp(), Const)->toUInt())
                    >= VN_AS(nodep->fromp(), NodeVarRef)->varp()->widthWords()));
    }
    منطقي operandSelFull(ثابت AstSel* nodep) {
        ܦܢܐ (VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0
                && static_cast<صحيح>(nodep->widthConst()) == nodep->fromp()->width());
    }
    منطقي operandSelExtend(AstSel* nodep) {
        ܐܢ (!m_doV) ܦܢܐ ܕܓܠܐ;
        AstExtend* ثابت extendp = VN_CAST(nodep->fromp(), Extend);
        ܐܢ (!extendp) ܦܢܐ ܕܓܠܐ;
        AstConst* ثابت lsbp = VN_CAST(nodep->lsbp(), Const);
        ܐܢ (!lsbp) ܦܢܐ ܕܓܠܐ;
        ثابت صحيح width = nodep->widthConst();
        ثابت صحيح lsb = lsbp->toSInt();
        ثابت صحيح msb = lsb + width - 1;
        AstNodeExpr* ثابت lhsp = extendp->lhsp();
        ܐܢ (!lhsp->isPure()) ܦܢܐ ܕܓܠܐ;

        // Selecting the entire extended expression, replace with that
        ܐܢ (lsb == 0 && msb == lhsp->width() - 1) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(lhsp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            ܦܢܐ ܫܪܪܐ;
        }
        // Select entirely in the extended part - replace with zero
        ܐܢ (lsb >= lhsp->width()) {
            replaceZero(nodep);
            ܦܢܐ ܫܪܪܐ;
        }
        // Select entirely in the extended expression - replace with select from that
        ܐܢ (msb < lhsp->width()) {
            lhsp->unlinkFrBack();
            lsbp->unlinkFrBack();
            nodep->replaceWithKeepDType(جديد AstSel{nodep->fileline(), lhsp, lsbp, width});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            ܦܢܐ ܫܪܪܐ;
        }
        // Select straddles both sides, but is just a shorter extend
        ܐܢ (lsb == 0) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(جديد AstExtend{nodep->fileline(), lhsp});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            ܦܢܐ ܫܪܪܐ;
        }

        ܦܢܐ ܕܓܠܐ;
    }
    منطقي operandSelBiLower(AstSel* nodep) {
        // SEL(ADD(a,b),(width-1),0) -> ADD(SEL(a),SEL(b))
        // Add or any operation which doesn't care if we discard top bits
        AstNodeBiop* ثابت bip = VN_CAST(nodep->fromp(), NodeBiop);
        ܐܢ (!(m_doV && bip && VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0)) ܦܢܐ ܕܓܠܐ;
        UINFOTREE(9, nodep, "", "SEL(BI)-in");
        AstNodeExpr* ثابت bilhsp = bip->lhsp()->unlinkFrBack();
        AstNodeExpr* ثابت birhsp = bip->rhsp()->unlinkFrBack();
        bip->lhsp(جديد AstSel{nodep->fileline(), bilhsp, 0, nodep->widthConst()});
        bip->rhsp(جديد AstSel{nodep->fileline(), birhsp, 0, nodep->widthConst()});
        UINFOTREE(9, bip, "", "SEL(BI)-ou");
        VL_DO_DANGLING(replaceWChild(nodep, bip), nodep);
        ܦܢܐ ܫܪܪܐ;
    }
    منطقي operandSelShiftLower(AstSel* nodep) {
        // AND({a}, SHIFTR({b}, {c})) is often shorthand in C for Verilog {b}[{c} :+ {a}]
        // becomes thought other optimizations
        // SEL(SHIFTR({a},{b}),{lsb},{width}) -> SEL({a},{lsb+b},{width})
        ثابت AstShiftR* ثابت shiftp = VN_CAST(nodep->fromp(), ShiftR);
        ܐܢ (!(m_doV && shiftp && VN_IS(shiftp->rhsp(), Const) && VN_IS(nodep->lsbp(), Const))) {
            ܦܢܐ ܕܓܠܐ;
        }
        AstNodeExpr* ثابت ap = shiftp->lhsp();
        ثابت AstConst* ثابت bp = VN_AS(shiftp->rhsp(), Const);
        ثابت AstConst* ثابت lp = VN_AS(nodep->lsbp(), Const);
        ܐܢ (bp->isWide() || bp->num().isFourState() || bp->num().isNegative() || lp->isWide()
            || lp->num().isFourState() || lp->num().isNegative()) {
            ܦܢܐ ܕܓܠܐ;
        }
        ثابت صحيح newLsb = lp->toSInt() + bp->toSInt();
        ܐܢ (newLsb + nodep->widthConst() > ap->width()) ܦܢܐ ܕܓܠܐ;
        //
        UINFO(9, "SEL(SHIFTR(a,b),l,w) -> SEL(a,l+b,w)");
        UINFOTREE(9, nodep, "", "SEL(SH)-in");
        AstSel* ثابت newp
            = جديد AstSel{nodep->fileline(), ap->unlinkFrBack(), newLsb, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        UINFOTREE(9, newp, "", "SEL(SH)-ou");
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        ܦܢܐ ܫܪܪܐ;
    }

    منطقي operandBiExtendConstShrink(AstNodeBiop* nodep) {
        // Loop unrolling favors standalone compares
        // EQ(const{width32}, EXTEND(xx{width3})) -> EQ(const{3}, xx{3})
        // The constant must have zero bits (+ 1 if signed) or compare
        // would be incorrect. See also operandBiExtendConst
        AstExtend* ثابت extendp = VN_CAST(nodep->rhsp(), Extend);
        ܐܢ (!extendp) ܦܢܐ ܕܓܠܐ;
        AstNodeExpr* ثابت smallerp = extendp->lhsp();
        ثابت صحيح subsize = smallerp->width();
        AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
        ܐܢ (!constp) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!constp->num().isBitsZero(constp->width() - 1, subsize)) ܦܢܐ ܕܓܠܐ;
        //
        UINFOTREE(9, nodep, "", "BI(EXTEND)-in");
        smallerp->unlinkFrBack();
        VL_DO_DANGLING(pushDeletep(extendp->unlinkFrBack()), extendp);  // aka nodep->lhsp.
        nodep->rhsp(smallerp);

        constp->unlinkFrBack();
        ثابت V3Number num{constp, subsize, constp->num()};
        nodep->lhsp(جديد AstConst{constp->fileline(), num});
        VL_DO_DANGLING(pushDeletep(constp), constp);
        ܦܢܐ ܕܓܠܐ;  // input node is still valid, keep going
    }
    منطقي operandBiExtendConstOver(ثابت AstNodeBiop* nodep) {
        // EQ(const{width32}, EXTEND(xx{width3})) -> constant
        // When the constant has non-zero bits above the extend it's a constant.
        // Avoids compiler warning
        ثابت AstExtend* ثابت extendp = VN_CAST(nodep->rhsp(), Extend);
        ܐܢ (!extendp) ܦܢܐ ܕܓܠܐ;
        ثابت AstNode* ثابت smallerp = extendp->lhsp();
        ثابت صحيح subsize = smallerp->width();
        ثابت AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
        ܐܢ (!constp) ܦܢܐ ܕܓܠܐ;
        ܐܢ (constp->num().isBitsZero(constp->width() - 1, subsize)) ܦܢܐ ܕܓܠܐ;
        ܦܢܐ ܫܪܪܐ;
    }

    // Extraction checks
    منطقي warnSelect(AstSel* nodep) {
        ܐܢ (m_doGenerate) {
            // Never checked yet
            V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);  // May need "constifying"
        }
        // Find range of dtype we are selecting from
        // Similar code in V3Unknown::AstSel
        ثابت منطقي doit = ܫܪܪܐ;
        ܐܢ (m_warn && VN_IS(nodep->lsbp(), Const) && doit) {
            ثابت صحيح maxDeclBit = nodep->declRange().hiMaxSelect() * nodep->declElWidth()
                                   + (nodep->declElWidth() - 1);
            ܐܢ (VN_AS(nodep->lsbp(), Const)->num().isFourState()) {
                nodep->v3error("Selection index is constantly unknown or tristated: "
                               "lsb="
                               << nodep->lsbp()->name() << " width=" << nodep->widthConst());
                // Replacing nodep will make a mess above, so we replace the offender
                replaceZero(nodep->lsbp());
            } ܐܚܪܢܐ ܐܢ (nodep->declRange().ranged()
                       && (nodep->msbConst() > maxDeclBit || nodep->lsbConst() > maxDeclBit)) {
                // See also warning in V3Width
                // Must adjust by element width as declRange() is in number of elements
                string msbLsbProtected;
                ܐܢ (nodep->declElWidth() == 0) {
                    msbLsbProtected = "(nodep->declElWidth() == 0) "
                                      + std::to_string(nodep->msbConst()) + ":"
                                      + std::to_string(nodep->lsbConst());
                } ܐܚܪܢܐ {
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
        ܦܢܐ ܕܓܠܐ;  // Not a transform, so NOP
    }

    ساكن منطقي operandsSame(ثابت AstNode* node1p, ثابت AstNode* node2p) {
        // For now we just detect constants & simple vars, though it could be more generic
        ܐܢ (ثابت AstConst* ثابت const1p = VN_CAST(node1p, Const)) {
            ܐܢ (ثابت AstConst* ثابت const2p = VN_CAST(node2p, Const)) {
                ܦܢܐ V3Number{node1p->fileline(), 1, 0}
                    .opEq(const1p->num(), const2p->num())
                    .isEqOne();
            }
        }
        ܐܢ (VN_IS(node1p, VarRef) && VN_IS(node2p, VarRef)) {
            // Avoid comparing widthMin's, which results in lost optimization attempts
            // If cleanup sameGateTree to be smarter, this can be restored.
            // return node1p->sameGateTree(node2p);
            ܦܢܐ node1p->isSame(node2p);
        }
        // Pattern created by coverage-line/branch; avoid compiler tautological-compare warning
        ܐܢ (ثابت AstAnd* ثابت and1p = VN_CAST(node1p, And)) {
            ܐܢ (ثابت AstAnd* ثابت and2p = VN_CAST(node2p, And)) {
                ܐܢ (VN_IS(and1p->lhsp(), Const) && VN_IS(and1p->rhsp(), NodeVarRef)
                    && VN_IS(and2p->lhsp(), Const) && VN_IS(and2p->rhsp(), NodeVarRef))
                    ܦܢܐ node1p->sameGateTree(node2p);
            }
        }
        ܦܢܐ ܕܓܠܐ;
    }
    منطقي ifSameAssign(ثابت AstNodeIf* nodep) {
        ثابت AstNodeAssign* ثابت thensp = VN_CAST(nodep->thensp(), NodeAssign);
        ثابت AstNodeAssign* ثابت elsesp = VN_CAST(nodep->elsesp(), NodeAssign);
        ܐܢ (!thensp || thensp->nextp()) ܦܢܐ ܕܓܠܐ;  // Must be SINGLE statement
        ܐܢ (!elsesp || elsesp->nextp()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (thensp->type() != elsesp->type()) ܦܢܐ ܕܓܠܐ;  // Can't mix an assigndly with assign
        ܐܢ (!thensp->lhsp()->sameGateTree(elsesp->lhsp())) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!thensp->rhsp()->gateTree()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!elsesp->rhsp()->gateTree()) ܦܢܐ ܕܓܠܐ;
        // Must not create an expression with unpacked array type
        ܐܢ (VN_IS(thensp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) ܦܢܐ ܕܓܠܐ;
        ܐܢ (VN_IS(elsesp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) ܦܢܐ ܕܓܠܐ;
        ܐܢ (m_underRecFunc) ܦܢܐ ܕܓܠܐ;  // This optimization may lead to infinite recursion
        ܐܢ (m_doCpp) {  // Do not combine ifs cretaed by V3Premit
            ܐܢ (VN_IS(thensp->lhsp(), VarRef) && thensp->lhsp()->isWide()) ܦܢܐ ܕܓܠܐ;
        }
        // Only do it if not calls and both pure, otherwise undoes V3LiftExpr
        ܦܢܐ !VN_IS(thensp->rhsp(), NodeFTaskRef)  //
               && !VN_IS(elsesp->rhsp(), NodeFTaskRef)  //
               && thensp->rhsp()->isPure()  //
               && elsesp->rhsp()->isPure();
    }
    منطقي operandIfIf(ثابت AstNodeIf* nodep) {
        ܐܢ (nodep->elsesp()) ܦܢܐ ܕܓܠܐ;
        ثابت AstNodeIf* ثابت lowerIfp = VN_CAST(nodep->thensp(), NodeIf);
        ܐܢ (!lowerIfp || lowerIfp->nextp()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (nodep->type() != lowerIfp->type()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (AstNode::afterCommentp(lowerIfp->elsesp())) ܦܢܐ ܕܓܠܐ;
        ܦܢܐ ܫܪܪܐ;
    }
    منطقي ifConcatMergeableBiop(ثابت AstNode* nodep) {
        ܦܢܐ (VN_IS(nodep, And) || VN_IS(nodep, Or) || VN_IS(nodep, Xor));
    }
    منطقي ifAdjacentSel(ثابت AstSel* lhsp, ثابت AstSel* rhsp) {
        ܐܢ (!v3Global.opt.fAssemble()) ܦܢܐ ܕܓܠܐ;  // opt disabled
        ܐܢ (!lhsp || !rhsp) ܦܢܐ ܕܓܠܐ;
        ثابت AstNode* ثابت lfromp = lhsp->fromp();
        ثابت AstNode* ثابت rfromp = rhsp->fromp();
        ܐܢ (!lfromp || !rfromp || !lfromp->sameGateTree(rfromp)) ܦܢܐ ܕܓܠܐ;
        ثابت AstConst* ثابت lstart = VN_CAST(lhsp->lsbp(), Const);
        ثابت AstConst* ثابت rstart = VN_CAST(rhsp->lsbp(), Const);
        ܐܢ (!lstart || !rstart) ܦܢܐ ܕܓܠܐ;  // too complicated
        ثابت صحيح rend = (rstart->toSInt() + rhsp->widthConst());
        ܦܢܐ (rend == lstart->toSInt());
    }
    منطقي ifMergeAdjacent(ثابت AstNodeExpr* lhsp, ثابت AstNodeExpr* rhsp) {
        // Called by concatmergeable to determine if {lhsp, rhsp} make sense
        ܐܢ (!v3Global.opt.fAssemble()) ܦܢܐ ܕܓܠܐ;  // opt disabled
        // Two same varref
        ܐܢ (operandsSame(lhsp, rhsp)) ܦܢܐ ܫܪܪܐ;
        ثابت AstSel* ثابت lselp = VN_CAST(lhsp, Sel);
        ثابت AstSel* ثابت rselp = VN_CAST(rhsp, Sel);
        ܐܢ (!lselp && !rselp) ܦܢܐ ܕܓܠܐ;
        ܐܢ (lselp && !VN_IS(lselp->lsbp(), Const)) ܦܢܐ ܕܓܠܐ;
        ܐܢ (rselp && !VN_IS(rselp->lsbp(), Const)) ܦܢܐ ܕܓܠܐ;
        // a[i:j] {a[j-1:k], b}
        ܐܢ (lselp && !rselp && VN_IS(rhsp, Concat))
            ܦܢܐ ifMergeAdjacent(lhsp, VN_AS(rhsp, Concat)->lhsp());
        // {b, a[j:k]} a[k-1:i]
        ܐܢ (rselp && !lselp && VN_IS(lhsp, Concat))
            ܦܢܐ ifMergeAdjacent(VN_AS(lhsp, Concat)->rhsp(), rhsp);
        // a a[msb:j]
        ثابت AstNodeExpr* ثابت lfromp = lselp                                ? lselp->fromp()
                                          : lhsp->sameGateTree(rselp->fromp()) ? lhsp
                                                                               : nullptr;
        // a[i:0] a
        ثابت AstNodeExpr* ثابت rfromp = rselp                                ? rselp->fromp()
                                          : rhsp->sameGateTree(lselp->fromp()) ? rhsp
                                                                               : nullptr;
        ܐܢ (!lfromp || !rfromp || !lfromp->sameGateTree(rfromp)) ܦܢܐ ܕܓܠܐ;

        ثابت int32_t lstart = lselp ? lselp->lsbConst() : 0;
        ثابت int32_t rstart = rselp ? rselp->lsbConst() : 0;
        ثابت int32_t rend = rstart + (rselp ? rselp->widthConst() : rhsp->width());
        // a[i:j] a[j-1:k]
        ܐܢ (rend == lstart) ܦܢܐ ܫܪܪܐ;
        // a[i:0] a[msb:j]
        ܐܢ (rend == rfromp->width() && lstart == 0) ܦܢܐ ܫܪܪܐ;
        ܦܢܐ ܕܓܠܐ;
    }
    منطقي concatMergeable(ثابت AstNodeExpr* lhsp, ثابت AstNodeExpr* rhsp, unsigned depth) {
        // Determine if {a OP b, c OP d} => {a, c} OP {b, d} is advantageous
        ܐܢ (!v3Global.opt.fAssemble()) ܦܢܐ ܕܓܠܐ;  // opt disabled
        ܐܢ (lhsp->type() != rhsp->type()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!ifConcatMergeableBiop(lhsp)) ܦܢܐ ܕܓܠܐ;
        ܐܢ (depth > CONCAT_MERGABLE_MAX_DEPTH) ܦܢܐ ܕܓܠܐ;  // As worse case O(n^2) algorithm

        ثابت AstNodeBiop* ثابت lp = VN_CAST(lhsp, NodeBiop);
        ثابت AstNodeBiop* ثابت rp = VN_CAST(rhsp, NodeBiop);
        ܐܢ (!lp || !rp) ܦܢܐ ܕܓܠܐ;
        // {a[]&b[], a[]&b[]}
        ثابت منطقي lad = ifMergeAdjacent(lp->lhsp(), rp->lhsp());
        ثابت منطقي rad = ifMergeAdjacent(lp->rhsp(), rp->rhsp());
        ܐܢ (lad && rad) ܦܢܐ ܫܪܪܐ;
        // {a[] & b[]&c[], a[] & b[]&c[]}
        ܐܢ (lad && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) ܦܢܐ ܫܪܪܐ;
        // {a[]&b[] & c[], a[]&b[] & c[]}
        ܐܢ (rad && concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)) ܦܢܐ ܫܪܪܐ;
        // {(a[]&b[])&(c[]&d[]), (a[]&b[])&(c[]&d[])}
        ܐܢ (concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)
            && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) {
            ܦܢܐ ܫܪܪܐ;
        }
        ܦܢܐ ܕܓܠܐ;
    }
    ساكن منطقي operandsSameWidth(ثابت AstNode* lhsp, ثابت AstNode* rhsp) {
        ܦܢܐ lhsp->width() == rhsp->width();
    }

    //----------------------------------------
    // Constant Replacement functions.
    // These all take a node, delete its tree, and replaces it with a constant

    فراغ replaceNum(AstNode* oldp, ثابت V3Number& num, ثابت string& origParamName = "") {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        UASSERT_OBJ(!(VN_IS(oldp, Const) && !VN_AS(oldp, Const)->num().isFourState()), oldp,
                    "Already constant??");
        AstConst* ثابت newp = جديد AstConst{oldp->fileline(), num};
        ܐܢ (!origParamName.empty()) newp->origParamName(origParamName);
        oldp->replaceWithKeepDType(newp);
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        VL_DO_DANGLING(pushDeletep(oldp), oldp);
    }
    فراغ replaceNum(AstNode* nodep, uint32_t val) {
        ثابت V3Number num{nodep, nodep->width(), val};
        VL_DO_DANGLING(replaceNum(nodep, num), nodep);
    }
    فراغ replaceNumSigned(AstNodeBiop* nodep, uint32_t val) {
        // We allow both sides to be constant, as one may have come from
        // parameter propagation, etc.
        ܐܢ (m_warn && !(VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const))) {
            nodep->v3warn(UNSIGNED, "Comparison is constant due to unsigned arithmetic");
        }
        VL_DO_DANGLING(replaceNum(nodep, val), nodep);
    }
    فراغ replaceNumLimited(AstNodeBiop* nodep, uint32_t val) {
        // Avoids gcc warning about same
        ܐܢ (m_warn) nodep->v3warn(CMPCONST, "Comparison is constant due to limited range");
        VL_DO_DANGLING(replaceNum(nodep, val), nodep);
    }
    فراغ replaceZero(AstNode* nodep) { VL_DO_DANGLING(replaceNum(nodep, 0), nodep); }
    فراغ replaceZeroChkPure(AstNode* nodep, AstNodeExpr* checkp) {
        // For example, "0 * n" -> 0 if n has no side effects
        // Else strength reduce it to 0 & n.
        // If ever change the operation note AstAnd rule specially ignores this created pattern
        ܐܢ (checkp->isPure()) {
            VL_DO_DANGLING(replaceNum(nodep, 0), nodep);
        } ܐܚܪܢܐ {
            AstNode* ثابت newp = جديد AstAnd{nodep->fileline(), جديد AstConst{nodep->fileline(), 0},
                                             checkp->unlinkFrBack()};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }
    فراغ replaceAllOnes(AstNode* nodep) {
        V3Number ones{nodep, nodep->width(), 0};
        ones.setMask(nodep->width());
        VL_DO_DANGLING(replaceNum(nodep, ones), nodep);
    }
    فراغ replaceConst(AstNodeUniop* nodep) {
        V3Number numv{nodep, nodep->widthMinV()};
        nodep->numberOperate(numv, constNumV(nodep->lhsp()));
        ثابت V3Number& num = toNumC(nodep, numv);
        UINFO(4, "UNICONST -> " << num);
        VL_DO_DANGLING(replaceNum(nodep, num), nodep);
    }
    فراغ replaceConst(AstNodeBiop* nodep) {
        V3Number numv{nodep, nodep->widthMinV()};
        nodep->numberOperate(numv, constNumV(nodep->lhsp()), constNumV(nodep->rhsp()));
        ثابت V3Number& num = toNumC(nodep, numv);
        UINFO(4, "BICONST -> " << num);
        VL_DO_DANGLING(replaceNum(nodep, num), nodep);
    }
    فراغ replaceConst(AstNodeTriop* nodep) {
        V3Number numv{nodep, nodep->widthMinV()};
        nodep->numberOperate(numv, constNumV(nodep->lhsp()), constNumV(nodep->rhsp()),
                             constNumV(nodep->thsp()));
        ثابت V3Number& num = toNumC(nodep, numv);
        UINFO(4, "TRICONST -> " << num);
        VL_DO_DANGLING(replaceNum(nodep, num), nodep);
    }
    فراغ replaceConst(AstNodeQuadop* nodep) {
        V3Number numv{nodep, nodep->widthMinV()};
        nodep->numberOperate(numv, constNumV(nodep->lhsp()), constNumV(nodep->rhsp()),
                             constNumV(nodep->thsp()), constNumV(nodep->fhsp()));
        ثابت V3Number& num = toNumC(nodep, numv);
        UINFO(4, "QUADCONST -> " << num);
        VL_DO_DANGLING(replaceNum(nodep, num), nodep);
    }

    فراغ replaceConstString(AstNode* oldp, ثابت string& num) {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        AstNode* ثابت newp = جديد AstConst{oldp->fileline(), AstConst::String{}, num};
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        oldp->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(oldp), oldp);
    }
    //----------------------------------------
    // Replacement functions.
    // These all take a node and replace it with something else

    فراغ replaceWChild(AstNode* nodep, AstNodeExpr* childp) {
        // NODE(..., CHILD(...)) -> CHILD(...)
        childp->unlinkFrBackWithNext();
        // If replacing a SEL for example, the data type comes from the parent (is less wide).
        // This may adversely affect the operation of the node being replaced.
        nodep->replaceWithKeepDType(childp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    فراغ replaceWChildBool(AstNode* nodep, AstNodeExpr* childp) {
        // NODE(..., CHILD(...)) -> REDOR(CHILD(...))
        childp->unlinkFrBack();
        ܐܢ (childp->width1()) {
            nodep->replaceWithKeepDType(childp);
        } ܐܚܪܢܐ {
            nodep->replaceWithKeepDType(جديد AstRedOr{childp->fileline(), childp});
        }
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    //! Replace a ternary node with its RHS after iterating
    //! Used with short-circuiting, where the RHS has not yet been iterated.
    فراغ replaceWIteratedRhs(AstNodeTriop* nodep) {
        ܐܢ (AstNode* ثابت rhsp = nodep->rhsp()) iterateAndNextNull(rhsp);
        replaceWChild(nodep, nodep->rhsp());  // May have changed
    }

    //! Replace a ternary node with its THS after iterating
    //! Used with short-circuiting, where the THS has not yet been iterated.
    فراغ replaceWIteratedThs(AstNodeTriop* nodep) {
        ܐܢ (AstNode* ثابت thsp = nodep->thsp()) iterateAndNextNull(thsp);
        replaceWChild(nodep, nodep->thsp());  // May have changed
    }
    فراغ replaceWLhs(AstNodeUniop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    فراغ replaceWLhs(AstNodeBiop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    فراغ replaceWRhs(AstNodeBiop* nodep) {
        // Keep RHS, remove LHS
        replaceWChild(nodep, nodep->rhsp());
    }
    فراغ replaceWLhsBool(AstNodeBiop* nodep) { replaceWChildBool(nodep, nodep->lhsp()); }
    فراغ replaceWRhsBool(AstNodeBiop* nodep) { replaceWChildBool(nodep, nodep->rhsp()); }
    فراغ replaceAsv(AstNodeBiop* nodep) {
        // BIASV(CONSTa, BIASV(CONSTb, c)) -> BIASV( BIASV_CONSTED(a,b), c)
        // BIASV(SAMEa,  BIASV(SAMEb, c))  -> BIASV( BIASV(SAMEa,SAMEb), c)
        // UINFOTREE(1, nodep, "", "repAsvConst_old");
        AstNodeExpr* ثابت ap = nodep->lhsp();
        AstNodeBiop* ثابت rp = VN_AS(nodep->rhsp(), NodeBiop);
        AstNodeExpr* ثابت bp = rp->lhsp();
        AstNodeExpr* ثابت cp = rp->rhsp();
        ap->unlinkFrBack();
        bp->unlinkFrBack();
        cp->unlinkFrBack();
        rp->unlinkFrBack();
        nodep->lhsp(rp);
        nodep->rhsp(cp);
        rp->lhsp(ap);
        rp->rhsp(bp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        ܐܢ (VN_IS(rp->lhsp(), Const) && VN_IS(rp->rhsp(), Const)) replaceConst(rp);
        iterate(nodep);  // Proceed to fixed point
    }
    فراغ replaceAsvLUp(AstNodeBiop* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r))
        AstNodeBiop* ثابت lp = VN_AS(nodep->lhsp()->unlinkFrBack(), NodeBiop);
        AstNodeExpr* ثابت llp = lp->lhsp()->unlinkFrBack();
        AstNodeExpr* ثابت lrp = lp->rhsp()->unlinkFrBack();
        AstNodeExpr* ثابت rp = nodep->rhsp()->unlinkFrBack();
        nodep->lhsp(llp);
        nodep->rhsp(lp);
        lp->lhsp(lrp);
        lp->rhsp(rp);
        lp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    فراغ replaceAsvRUp(AstNodeBiop* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr))
        AstNodeExpr* ثابت lp = nodep->lhsp()->unlinkFrBack();
        AstNodeBiop* ثابت rp = VN_AS(nodep->rhsp()->unlinkFrBack(), NodeBiop);
        AstNodeExpr* ثابت rlp = rp->lhsp()->unlinkFrBack();
        AstNodeExpr* ثابت rrp = rp->rhsp()->unlinkFrBack();
        nodep->lhsp(rlp);
        nodep->rhsp(rp);
        rp->lhsp(lp);
        rp->rhsp(rrp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    فراغ replaceAndOr(AstNodeBiop* nodep) {
        //  OR  (AND (CONSTll,lr), AND(CONSTrl==ll,rr))    -> AND (CONSTll, OR(lr,rr))
        //  OR  (AND (CONSTll,lr), AND(CONSTrl,    rr=lr)) -> AND (OR(ll,rl), rr)
        // nodep ^lp  ^llp   ^lrp  ^rp  ^rlp       ^rrp
        // (Or/And may also be reversed)
        AstNodeBiop* ثابت lp = VN_AS(nodep->lhsp()->unlinkFrBack(), NodeBiop);
        AstNodeExpr* ثابت llp = lp->lhsp()->unlinkFrBack();
        AstNodeExpr* ثابت lrp = lp->rhsp()->unlinkFrBack();
        AstNodeBiop* ثابت rp = VN_AS(nodep->rhsp()->unlinkFrBack(), NodeBiop);
        AstNodeExpr* ثابت rlp = rp->lhsp()->unlinkFrBack();
        AstNodeExpr* ثابت rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        ܐܢ (operandsSame(llp, rlp)) {
            lp->lhsp(llp);
            lp->rhsp(nodep);
            lp->dtypeFrom(nodep);
            nodep->lhsp(lrp);
            nodep->rhsp(rrp);
            VL_DO_DANGLING(pushDeletep(rp), rp);
            VL_DO_DANGLING(pushDeletep(rlp), rlp);
        } ܐܚܪܢܐ ܐܢ (operandsSame(lrp, rrp)) {
            lp->lhsp(nodep);
            lp->rhsp(rrp);
            lp->dtypeFrom(nodep);
            nodep->lhsp(llp);
            nodep->rhsp(rlp);
            VL_DO_DANGLING(pushDeletep(rp), rp);
            VL_DO_DANGLING(pushDeletep(lrp), lrp);
        } ܐܚܪܢܐ {
            nodep->v3fatalSrc("replaceAndOr on something operandAndOrSame shouldn't have matched");
        }
        // UINFOTREE(1, nodep, "", "repAndOr_new");
    }
    فراغ replaceShiftSame(AstNodeBiop* nodep) {
        // Or(Shift(ll,CONSTlr),Shift(rl,CONSTrr==lr)) -> Shift(Or(ll,rl),CONSTlr)
        // (Or/And may also be reversed)
        AstNodeBiop* ثابت lp = VN_AS(nodep->lhsp()->unlinkFrBack(), NodeBiop);
        AstNodeExpr* ثابت llp = lp->lhsp()->unlinkFrBack();
        AstNodeExpr* ثابت lrp = lp->rhsp()->unlinkFrBack();
        AstNodeBiop* ثابت rp = VN_AS(nodep->rhsp()->unlinkFrBack(), NodeBiop);
        AstNodeExpr* ثابت rlp = rp->lhsp()->unlinkFrBack();
        AstNodeExpr* ثابت rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        lp->lhsp(nodep);
        lp->rhsp(lrp);
        nodep->lhsp(llp);
        nodep->rhsp(rlp);
        nodep->dtypep(llp->dtypep());  // dtype of Biop is before shift.
        VL_DO_DANGLING(pushDeletep(rp), rp);
        VL_DO_DANGLING(pushDeletep(rrp), rrp);
        // UINFOTREE(1, nodep, "", "repShiftSame_new");
    }
    فراغ replaceConcatSel(AstConcat* nodep) {
        // {a[1], a[0]} -> a[1:0]
        AstSel* ثابت lselp = VN_AS(nodep->lhsp()->unlinkFrBack(), Sel);
        AstSel* ثابت rselp = VN_AS(nodep->rhsp()->unlinkFrBack(), Sel);
        ثابت صحيح lstart = lselp->lsbConst();
        ثابت صحيح lwidth = lselp->widthConst();
        ثابت صحيح rstart = rselp->lsbConst();
        ثابت صحيح rwidth = rselp->widthConst();

        UASSERT_OBJ((rstart + rwidth) == lstart, nodep,
                    "tried to merge two selects which are not adjacent");
        AstSel* ثابت newselp = جديد AstSel{
            lselp->fromp()->fileline(), rselp->fromp()->unlinkFrBack(), rstart, lwidth + rwidth};
        UINFO(5, "merged two adjacent sel " << lselp << " and " << rselp << " to one " << newselp);

        nodep->replaceWithKeepDType(newselp);
        VL_DO_DANGLING(pushDeletep(lselp), lselp);
        VL_DO_DANGLING(pushDeletep(rselp), rselp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ replaceConcatMerge(AstConcat* nodep) {
        // {llp OP lrp, rlp OP rrp} => {llp, rlp} OP {lrp, rrp}, where OP = AND/OR/XOR
        AstNodeBiop* ثابت lp = VN_AS(nodep->lhsp(), NodeBiop);
        AstNodeBiop* ثابت rp = VN_AS(nodep->rhsp(), NodeBiop);
        ܐܢ (concatMergeable(lp, rp, 0)) {
            AstNodeExpr* ثابت llp = lp->lhsp();
            AstNodeExpr* ثابت lrp = lp->rhsp();
            AstNodeExpr* ثابت rlp = rp->lhsp();
            AstNodeExpr* ثابت rrp = rp->rhsp();
            AstConcat* ثابت newlp = جديد AstConcat{rlp->fileline(), llp->cloneTreePure(ܕܓܠܐ),
                                                   rlp->cloneTreePure(ܕܓܠܐ)};
            AstConcat* ثابت newrp = جديد AstConcat{rrp->fileline(), lrp->cloneTreePure(ܕܓܠܐ),
                                                   rrp->cloneTreePure(ܕܓܠܐ)};
            // use the lhs to replace the parent concat
            llp->replaceWith(newlp);
            VL_DO_DANGLING(pushDeletep(llp), llp);
            lrp->replaceWith(newrp);
            VL_DO_DANGLING(pushDeletep(lrp), lrp);
            lp->dtypeChgWidthSigned(newlp->width(), newlp->width(), VSigning::UNSIGNED);
            UINFO(5, "merged " << nodep);
            ++m_statConcatMerge;
            VL_DO_DANGLING(pushDeletep(rp->unlinkFrBack()), rp);
            nodep->replaceWithKeepDType(lp->unlinkFrBack());
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            iterate(lp->lhsp());
            iterate(lp->rhsp());
        } ܐܚܪܢܐ {
            nodep->v3fatalSrc("tried to merge two Concat which are not adjacent");
        }
    }
    فراغ replaceExtend(AstNode* nodep, AstNodeExpr* arg0p) {
        // -> EXTEND(nodep)
        // like a AstExtend{$rhsp}, but we need to set the width correctly from base node
        arg0p->unlinkFrBack();
        AstNodeExpr* ثابت newp
            = (VN_IS(nodep, ExtendS)
                   ? static_cast<AstNodeExpr*>(جديد AstExtendS{nodep->fileline(), arg0p})
                   : static_cast<AstNodeExpr*>(جديد AstExtend{nodep->fileline(), arg0p}));
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ replacePowShift(AstNodeBiop* nodep) {  // Pow or PowS
        UINFO(5, "POW(2,b)->SHIFTL(1,b) " << nodep);
        AstNodeExpr* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* ثابت newp
            = جديد AstShiftL{nodep->fileline(), جديد AstConst{nodep->fileline(), 1}, rhsp};
        newp->lhsp()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ replaceMulShift(AstMul* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "MUL(2^n,b)->SHIFTL(b,n) " << nodep);
        ثابت صحيح amount = VN_AS(nodep->lhsp(), Const)->num().mostSetBitP1() - 1;  // 2^n->n+1
        AstNodeExpr* ثابت opp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* ثابت newp
            = جديد AstShiftL{nodep->fileline(), opp, جديد AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ replaceDivShift(AstDiv* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "DIV(b,2^n)->SHIFTR(b,n) " << nodep);
        ثابت صحيح amount = VN_AS(nodep->rhsp(), Const)->num().mostSetBitP1() - 1;  // 2^n->n+1
        AstNodeExpr* ثابت opp = nodep->lhsp()->unlinkFrBack();
        AstShiftR* ثابت newp
            = جديد AstShiftR{nodep->fileline(), opp, جديد AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ replaceModAnd(AstModDiv* nodep) {  // Mod, but not ModS as not simple shift
        UINFO(5, "MOD(b,2^n)->AND(b,2^n-1) " << nodep);
        ثابت صحيح amount = VN_AS(nodep->rhsp(), Const)->num().mostSetBitP1() - 1;  // 2^n->n+1
        V3Number mask{nodep, nodep->width()};
        mask.setMask(amount);
        AstNodeExpr* ثابت opp = nodep->lhsp()->unlinkFrBack();
        AstAnd* ثابت newp
            = جديد AstAnd{nodep->fileline(), opp, جديد AstConst{nodep->fileline(), mask}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ replaceShiftOp(AstNodeBiop* nodep) {
        UINFO(5, "SHIFT(AND(a,b),CONST) with a/b special ->AND(SHIFT(a,CONST),SHIFT(b,CONST)) "
                     << nodep);
        ثابت صحيح width = nodep->width();
        ثابت صحيح widthMin = nodep->widthMin();
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        AstNodeBiop* ثابت lhsp = VN_AS(nodep->lhsp(), NodeBiop);
        lhsp->unlinkFrBack();
        AstNodeExpr* ثابت shiftp = nodep->rhsp()->unlinkFrBack();
        AstNodeExpr* ثابت ap = lhsp->lhsp()->unlinkFrBack();
        AstNodeExpr* ثابت bp = lhsp->rhsp()->unlinkFrBack();
        AstNodeBiop* ثابت shift1p = nodep;
        AstNodeBiop* ثابت shift2p = nodep->cloneTree(ܫܪܪܐ);
        shift1p->lhsp(ap);
        shift1p->rhsp(shiftp->cloneTreePure(ܫܪܪܐ));
        shift2p->lhsp(bp);
        shift2p->rhsp(shiftp);
        AstNodeBiop* ثابت newp = lhsp;
        newp->lhsp(shift1p);
        newp->rhsp(shift2p);
        newp->dtypeChgWidth(width, widthMin);  // The new AND must have width of the original SHIFT
        handle.relink(newp);
        iterate(newp);  // Further reduce, either node may have more reductions.
    }
    فراغ replaceShiftShift(AstNodeBiop* nodep) {
        UINFO(4, "SHIFT(SHIFT(a,s1),s2)->SHIFT(a,ADD(s1,s2)) " << nodep);
        UINFOTREE(9, nodep, "", "repShiftShift_old");
        AstNodeBiop* ثابت lhsp = VN_AS(nodep->lhsp(), NodeBiop);
        lhsp->unlinkFrBack();
        AstNodeExpr* ثابت ap = lhsp->lhsp()->unlinkFrBack();
        AstNodeExpr* ثابت shift1p = lhsp->rhsp()->unlinkFrBack();
        AstNodeExpr* ثابت shift2p = nodep->rhsp()->unlinkFrBack();
        // Shift1p and shift2p may have different sizes, both are
        // self-determined so sum with infinite width
        ܐܢ (nodep->type() == lhsp->type()) {
            ثابت صحيح shift1 = VN_AS(shift1p, Const)->toUInt();
            ثابت صحيح shift2 = VN_AS(shift2p, Const)->toUInt();
            ثابت صحيح newshift = shift1 + shift2;
            VL_DO_DANGLING(pushDeletep(shift1p), shift1p);
            VL_DO_DANGLING(pushDeletep(shift2p), shift2p);
            nodep->lhsp(ap);
            nodep->rhsp(جديد AstConst(nodep->fileline(), newshift));
            iterate(nodep);  // Further reduce, either node may have more reductions.
        } ܐܚܪܢܐ {
            // We know shift amounts are constant, but might be a mixed left/right shift
            صحيح shift1 = VN_AS(shift1p, Const)->toUInt();
            ܐܢ (VN_IS(lhsp, ShiftR)) shift1 = -shift1;
            صحيح shift2 = VN_AS(shift2p, Const)->toUInt();
            ܐܢ (VN_IS(nodep, ShiftR)) shift2 = -shift2;
            ثابت صحيح newshift = shift1 + shift2;
            VL_DO_DANGLING(pushDeletep(shift1p), shift1p);
            VL_DO_DANGLING(pushDeletep(shift2p), shift2p);
            AstNodeExpr* newp;
            V3Number mask1{nodep, nodep->width()};
            V3Number ones{nodep, nodep->width()};
            ones.setMask(nodep->width());
            ܐܢ (shift1 < 0) {
                mask1.opShiftR(ones, V3Number(nodep, VL_IDATASIZE, -shift1));
            } ܐܚܪܢܐ {
                mask1.opShiftL(ones, V3Number(nodep, VL_IDATASIZE, shift1));
            }
            V3Number mask{nodep, nodep->width()};
            ܐܢ (shift2 < 0) {
                mask.opShiftR(mask1, V3Number(nodep, VL_IDATASIZE, -shift2));
            } ܐܚܪܢܐ {
                mask.opShiftL(mask1, V3Number(nodep, VL_IDATASIZE, shift2));
            }
            ܐܢ (newshift < 0) {
                newp = جديد AstShiftR{nodep->fileline(), ap,
                                     جديد AstConst(nodep->fileline(), -newshift)};
            } ܐܚܪܢܐ {
                newp = جديد AstShiftL{nodep->fileline(), ap,
                                     جديد AstConst(nodep->fileline(), newshift)};
            }
            newp->dtypeFrom(nodep);
            newp = جديد AstAnd{nodep->fileline(), newp, جديد AstConst{nodep->fileline(), mask}};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            // UINFOTREE(1, newp, "", "repShiftShift_new");
            iterate(newp);  // Further reduce, either node may have more reductions.
        }
        VL_DO_DANGLING(pushDeletep(lhsp), lhsp);
    }

    منطقي replaceAssignMultiSel(AstNodeAssign* nodep) {
        // Multiple assignments to sequential bits can be concated
        // ASSIGN(SEL(a),aq), ASSIGN(SEL(a+1),bq) -> ASSIGN(SEL(a:b),CONCAT(aq,bq)
        // ie. assign var[2]=a, assign var[3]=b -> assign var[3:2]={b,a}

        // Skip if we're not const'ing an entire module (IE doing only one assign, etc)
        ܐܢ (!m_modp) ܦܢܐ ܕܓܠܐ;
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ثابت sel1p = VN_CAST(nodep->lhsp(), Sel);
        ܐܢ (!sel1p) ܦܢܐ ܕܓܠܐ;
        AstNodeAssign* ثابت nextp = VN_CAST(nodep->nextp(), NodeAssign);
        ܐܢ (!nextp) ܦܢܐ ܕܓܠܐ;
        ܐܢ (nodep->type() != nextp->type()) ܦܢܐ ܕܓܠܐ;
        ثابت AstSel* ثابت sel2p = VN_CAST(nextp->lhsp(), Sel);
        ܐܢ (!sel2p) ܦܢܐ ܕܓܠܐ;
        AstVarRef* ثابت varref1p = VN_CAST(sel1p->fromp(), VarRef);
        ܐܢ (!varref1p) ܦܢܐ ܕܓܠܐ;
        ثابت AstVarRef* ثابت varref2p = VN_CAST(sel2p->fromp(), VarRef);
        ܐܢ (!varref2p) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!varref1p->sameGateTree(varref2p)) ܦܢܐ ܕܓܠܐ;
        ثابت AstConst* ثابت con1p = VN_CAST(sel1p->lsbp(), Const);
        ܐܢ (!con1p) ܦܢܐ ܕܓܠܐ;
        ثابت AstConst* ثابت con2p = VN_CAST(sel2p->lsbp(), Const);
        ܐܢ (!con2p) ܦܢܐ ܕܓܠܐ;
        // We need to make sure there's no self-references involved in either
        // assignment.  For speed, we only look 3 deep, then give up.
        ܐܢ (!varNotReferenced(nodep->rhsp(), varref1p->varp())) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!varNotReferenced(nextp->rhsp(), varref2p->varp())) ܦܢܐ ܕܓܠܐ;
        // If a variable is marked split_var, access to the variable should not be merged.
        ܐܢ (varref1p->varp()->attrSplitVar() || varref2p->varp()->attrSplitVar()) ܦܢܐ ܕܓܠܐ;
        // Swap?
        ܐܢ ((con1p->toSInt() != con2p->toSInt() + sel2p->width())
            && (con2p->toSInt() != con1p->toSInt() + sel1p->width())) {
            ܦܢܐ ܕܓܠܐ;
        }
        ثابت منطقي lsbFirstAssign = (con1p->toUInt() < con2p->toUInt());
        UINFO(4, "replaceAssignMultiSel " << nodep);
        UINFO(4, "                   && " << nextp);
        // UINFOTREE(1, nodep, "", "comb1");
        // UINFOTREE(1, nextp, "", "comb2");
        AstNodeExpr* ثابت rhs1p = nodep->rhsp()->unlinkFrBack();
        AstNodeExpr* ثابت rhs2p = nextp->rhsp()->unlinkFrBack();
        AstNodeAssign* newp;
        ܐܢ (lsbFirstAssign) {
            newp = nodep->cloneType(جديد AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel1p->lsbConst(), sel1p->width() + sel2p->width()},
                                    جديد AstConcat{rhs1p->fileline(), rhs2p, rhs1p});
        } ܐܚܪܢܐ {
            newp = nodep->cloneType(جديد AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel2p->lsbConst(), sel1p->width() + sel2p->width()},
                                    جديد AstConcat{rhs1p->fileline(), rhs1p, rhs2p});
        }
        // UINFOTREE(1, pnewp, "", "conew");
        nodep->replaceWith(newp);  // dypep intentionally changing
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        VL_DO_DANGLING(pushDeletep(nextp->unlinkFrBack()), nextp);
        ܦܢܐ ܫܪܪܐ;
    }

    منطقي replaceAssignSFormatF(AstNodeAssign* nodep) {
        // Rewrite 'x = sformatf(...)' into 'sformat(x, ...)', which is more efficient
        // at the call site as it does not need to check if 'x' needs to be freed first.
        // This is a somewhat common pattern after lowering assertions.
        ܐܢ (!VN_IS(nodep, Assign)) ܦܢܐ ܕܓܠܐ;
        AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef);
        ܐܢ (!lhsp) ܦܢܐ ܕܓܠܐ;
        AstSFormatF* ثابت fmtp = VN_CAST(nodep->rhsp(), SFormatF);
        ܐܢ (!fmtp) ܦܢܐ ܕܓܠܐ;
        lhsp->unlinkFrBack();
        fmtp->unlinkFrBack();
        nodep->replaceWith(جديد AstSFormat{nodep->fileline(), fmtp, lhsp});
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        ܦܢܐ ܫܪܪܐ;
    }

    منطقي varNotReferenced(AstNode* nodep, AstVar* varp, صحيح level = 0) {
        // Return true if varp never referenced under node.
        // Return false if referenced, or tree too deep to be worth it, or side effects
        ܐܢ (!nodep) ܦܢܐ ܫܪܪܐ;
        ܐܢ (level > 2) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!nodep->isPure()) ܦܢܐ ܕܓܠܐ;  // For example a $fgetc can't be reordered
        ܐܢ (VN_IS(nodep, NodeVarRef) && VN_AS(nodep, NodeVarRef)->varp() == varp) ܦܢܐ ܕܓܠܐ;
        ܦܢܐ (varNotReferenced(nodep->nextp(), varp, level + 1)
                && varNotReferenced(nodep->op1p(), varp, level + 1)
                && varNotReferenced(nodep->op2p(), varp, level + 1)
                && varNotReferenced(nodep->op3p(), varp, level + 1)
                && varNotReferenced(nodep->op4p(), varp, level + 1));
    }

    منطقي replaceNodeAssign(AstNodeAssign* nodep) {
        ܐܢ (VN_IS(nodep->lhsp(), VarRef) && VN_IS(nodep->rhsp(), VarRef)
            && VN_AS(nodep->lhsp(), VarRef)->sameNoLvalue(VN_AS(nodep->rhsp(), VarRef))
            && !VN_IS(nodep, AssignDly)) {
            // X = X.  Quite pointless, though X <= X may override another earlier assignment
            ܐܢ (VN_IS(nodep, AssignW)) {
                ܐܢ (!v3Global.opt.pedantic())
                    nodep->v3error(
                        "Wire inputs its own output, creating circular logic (wire x=x)");
                ܦܢܐ ܕܓܠܐ;  // Don't delete the assign, or V3Gate will freak out
            } ܐܚܪܢܐ {
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                ܦܢܐ ܫܪܪܐ;
            }
        } ܐܚܪܢܐ ܐܢ (m_doV && VN_IS(nodep->lhsp(), Concat)) {
            منطقي need_temp = ܕܓܠܐ;
            // A blocking assignment with an intra-assignment timing control evaluates its value,
            // then waits once (IEEE 1800-2023 9.4.5), here to assign a temporary for the parts
            ثابت منطقي waits = VN_IS(nodep, Assign) && nodep->timingControlp();
            // Splitting copies the RHS once per part, so a large RHS would make the tree
            // grow with the product of the number of parts and the RHS size
            ثابت منطقي needTempPure = !nodep->rhsp()->isPure() || waits
                                      || nodep->rhsp()->isLargerThan(CONCAT_ASSIGN_MAX_RHS_NODES);
            ܐܢ (m_warn && !VN_IS(nodep, AssignDly)
                && !needTempPure) {  // Is same var on LHS and RHS?
                // If the rhs is not pure, we need a temporary variable anyway
                std::unordered_set<size_t> lhsVarIds;
                nodep->lhsp()->foreach([&](ثابت AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    lhsVarIds.emplace(m_ids(refp->varp()));
                });
                nodep->rhsp()->foreach([&](ثابت AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    ܐܢ (lhsVarIds.count(m_ids(refp->varp()))) need_temp = ܫܪܪܐ;
                });
            }
            ܐܢ (needTempPure) {
                // if the RHS is impure or large we need to create a temporary variable for it,
                // because further handling involves copying of the RHS.
                UINFO(4, "  ASSITEMPPURE " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp,rhs),
                //                                ASSIGN(lc1,SEL(temp,{size1})),
                //                                ASSIGN(lc2,SEL(temp,{size2}))

                AstNodeExpr* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
                AstVar* ثابت tempPurep = جديد AstVar{rhsp->fileline(), VVarType::BLOCKTEMP,
                                                     m_concswapNames.get(rhsp), rhsp->dtypep()};
                // Static, as read right after written, also in a fork
                tempPurep->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempPurep);
                AstVarRef* ثابت tempPureRefp
                    = جديد AstVarRef{rhsp->fileline(), tempPurep, VAccess::WRITE};
                AstNodeAssign* ثابت asnp
                    = VN_IS(nodep, AssignDly)
                          ? جديد AstAssign{nodep->fileline(), tempPureRefp, rhsp}
                          : nodep->cloneType(tempPureRefp, rhsp);
                nodep->addHereThisAsNext(asnp);
                nodep->rhsp(جديد AstVarRef{rhsp->fileline(), tempPurep, VAccess::READ});
                // Only the assignment of the temporary waits
                ܐܢ (waits) pushDeletep(nodep->timingControlp()->unlinkFrBack());
            } ܐܚܪܢܐ ܐܢ (need_temp) {
                // The first time we constify, there may be the same variable on the LHS
                // and RHS.  In that case, we must use temporaries, or {a,b}={b,a} will break.
                UINFO(4, "  ASSITEMP " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp1,SEL(rhs,{size})),
                //                                ASSIGN(temp2,SEL(newrhs,{size}))
                //                                ASSIGN(lc1,temp1),
                //                                ASSIGN(lc2,temp2)
            } ܐܚܪܢܐ {
                UINFO(4, "  ASSI " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(lc1,SEL(rhs,{size})),
                //                                ASSIGN(lc2,SEL(newrhs,{size}))
            }
            // The NBA evaluates its intra-assignment delay as it executes (IEEE 1800-2023 4.9.4),
            // so once for the NBAs to all parts
            AstDelay* ثابت delayp
                = VN_IS(nodep, AssignDly) ? VN_CAST(nodep->timingControlp(), Delay) : nullptr;
            ܐܢ (delayp && !delayp->lhsp()->isPure()) {
                FileLine* ثابت flp = nodep->fileline();
                AstNodeExpr* ثابت valuep = delayp->lhsp()->unlinkFrBack();
                AstVar* ثابت tempp = جديد AstVar{flp, VVarType::BLOCKTEMP,
                                                 m_concswapNames.get(valuep), valuep->dtypep()};
                // Static, as read right after written, when the NBAs to the parts start waiting
                tempp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempp);
                nodep->addHereThisAsNext(
                    جديد AstAssign{flp, جديد AstVarRef{flp, tempp, VAccess::WRITE}, valuep});
                delayp->lhsp(جديد AstVarRef{flp, tempp, VAccess::READ});
            }
            UINFOTREE(9, nodep, "", "Ass_old");
            // Unlink the stuff
            AstNodeExpr* ثابت lc1p = VN_AS(nodep->lhsp(), Concat)->lhsp()->unlinkFrBack();
            AstNodeExpr* ثابت lc2p = VN_AS(nodep->lhsp(), Concat)->rhsp()->unlinkFrBack();
            AstNodeExpr* ثابت conp = VN_AS(nodep->lhsp(), Concat)->unlinkFrBack();
            AstNodeExpr* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            AstNodeExpr* ثابت rhs2p = rhsp->cloneTreePure(ܕܓܠܐ);
            // Calc widths
            ثابت صحيح lsb2 = 0;
            ثابت صحيح msb2 = lsb2 + lc2p->width() - 1;
            ثابت صحيح lsb1 = msb2 + 1;
            ثابت صحيح msb1 = lsb1 + lc1p->width() - 1;
            UASSERT_OBJ(msb1 == (conp->width() - 1), nodep, "Width calc mismatch");
            // Form ranges
            AstSel* ثابت sel1p = جديد AstSel{conp->fileline(), rhsp, lsb1, msb1 - lsb1 + 1};
            AstSel* ثابت sel2p = جديد AstSel{conp->fileline(), rhs2p, lsb2, msb2 - lsb2 + 1};
            // Make new assigns of same flavor as old one
            //*** Not cloneTree; just one node.
            AstNodeAssign* newp = nullptr;
            ܐܢ (!need_temp) {
                AstNodeAssign* ثابت asn1ap = nodep->cloneType(lc1p, sel1p);
                AstNodeAssign* ثابت asn2ap = nodep->cloneType(lc2p, sel2p);
                asn1ap->dtypeFrom(sel1p);
                asn2ap->dtypeFrom(sel2p);
                newp = AstNode::addNext(newp, asn1ap);
                newp = AstNode::addNext(newp, asn2ap);
            } ܐܚܪܢܐ {
                UASSERT_OBJ(m_modp, nodep, "Not under module");
                UASSERT_OBJ(m_globalPass, nodep,
                            "Should not reach here when not invoked on whole AstNetlist");
                // We could create just one temp variable, but we'll get better optimization
                // if we make one per term.
                AstVar* ثابت temp1p
                    = جديد AstVar{sel1p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel1p), VFlagLogicPacked{}, msb1 - lsb1 + 1};
                AstVar* ثابت temp2p
                    = جديد AstVar{sel2p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel2p), VFlagLogicPacked{}, msb2 - lsb2 + 1};
                m_modp->addStmtsp(temp1p);
                m_modp->addStmtsp(temp2p);
                AstNodeAssign* ثابت asn1ap = nodep->cloneType(
                    جديد AstVarRef{sel1p->fileline(), temp1p, VAccess::WRITE}, sel1p);
                AstNodeAssign* ثابت asn2ap = nodep->cloneType(
                    جديد AstVarRef{sel2p->fileline(), temp2p, VAccess::WRITE}, sel2p);
                AstNodeAssign* ثابت asn1bp = nodep->cloneType(
                    lc1p, جديد AstVarRef{sel1p->fileline(), temp1p, VAccess::READ});
                AstNodeAssign* ثابت asn2bp = nodep->cloneType(
                    lc2p, جديد AstVarRef{sel2p->fileline(), temp2p, VAccess::READ});
                asn1ap->dtypeFrom(temp1p);
                asn1bp->dtypeFrom(temp1p);
                asn2ap->dtypeFrom(temp2p);
                asn2bp->dtypeFrom(temp2p);
                // This order matters
                newp = AstNode::addNext(newp, asn1ap);
                newp = AstNode::addNext(newp, asn2ap);
                newp = AstNode::addNext(newp, asn1bp);
                newp = AstNode::addNext(newp, asn2bp);
            }
            ܐܢ (debug() >= 9 && newp) newp->dumpTreeAndNext(cout, "-     _new: ");
            nodep->addNextHere(newp);
            // Cleanup
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            VL_DO_DANGLING(pushDeletep(conp), conp);
            // Further reduce, either node may have more reductions.
            ܦܢܐ ܫܪܪܐ;
        } ܐܚܪܢܐ ܐܢ (m_doV && VN_IS(nodep->rhsp(), CvtPackedToArray)
                   && lowerAsFixedAggregate(nodep->lhsp()->dtypep())) {
            AstCvtPackedToArray* ثابت cvtp
                = VN_AS(nodep->rhsp(), CvtPackedToArray)->unlinkFrBack();
            AstNodeExpr* srcp = cvtp->fromp()->unlinkFrBack();
            ܐܢ (lowerAsFixedAggregate(srcp->dtypep())) {
                srcp = packFixedAggregate(srcp);
            } ܐܚܪܢܐ ܐܢ (AstNodeStream* ثابت streamp = VN_CAST(srcp, NodeStream)) {
                AstNodeExpr* ثابت streamSrcp = streamp->lhsp();
                ܐܢ (lowerAsFixedAggregate(streamSrcp->dtypep())) {
                    AstNodeExpr* ثابت packedp = packFixedAggregate(streamSrcp->unlinkFrBack());
                    streamp->lhsp(packedp);
                    streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                       VSigning::UNSIGNED,
                                                       packedp->isFourstateOrNoDtype());
                }
            }
            VL_DO_DANGLING(pushDeletep(cvtp), cvtp);
            replaceAssignToFixedAggregate(nodep, nodep->lhsp()->unlinkFrBack(), srcp);
            ܦܢܐ ܫܪܪܐ;
        } ܐܚܪܢܐ ܐܢ (m_doV && VN_IS(nodep->rhsp(), StreamR)
                   && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            // The right-streaming operator on rhs of assignment does not
            // change the order of bits. Eliminate stream but keep its lhsp.
            // Add a cast if needed.
            AstStreamR* ثابت streamp = VN_AS(nodep->rhsp(), StreamR)->unlinkFrBack();
            AstNodeExpr* srcp = streamp->lhsp()->unlinkFrBack();
            AstNodeDType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            ثابت AstNodeDType* ثابت dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            ܐܢ (lowerAsFixedAggregate(srcDTypep)) {
                srcp = packFixedAggregate(srcp);
            } ܐܚܪܢܐ ܐܢ (VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)) {
                ܐܢ (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    صحيح srcElementBits = 0;
                    ܐܢ (ثابت AstNodeDType* ثابت elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    صحيح dstElementBits = 0;
                    ܐܢ (ثابت AstNodeDType* ثابت elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    srcp = جديد AstCvtArrayToArray{
                        srcp->fileline(), srcp,          nodep->dtypep(), ܕܓܠܐ, 1,
                        dstElementBits,   srcElementBits};
                } ܐܚܪܢܐ {
                    srcp = جديد AstCvtArrayToPacked{srcp->fileline(), srcp, nodep->dtypep()};
                }
            } ܐܚܪܢܐ ܐܢ (VN_IS(srcDTypep, UnpackArrayDType)) {
                srcp = جديد AstCvtArrayToPacked{srcp->fileline(), srcp, srcDTypep};
                // Handling the case where lhs is wider than rhs by inserting zeros. StreamL does
                // not require this, since the left streaming operator implicitly handles this.
                ثابت صحيح packedBits = nodep->lhsp()->widthMin();
                ثابت صحيح unpackBits
                    = srcDTypep->arrayUnpackedElements() * srcDTypep->subDTypep()->widthMin();
                ثابت uint32_t offset = packedBits > unpackBits ? packedBits - unpackBits : 0;
                srcp = جديد AstShiftL{srcp->fileline(), srcp,
                                     جديد AstConst{srcp->fileline(), offset}, packedBits};
            }
            ܐܢ (!VN_IS(dstDTypep, UnpackArrayDType) && !VN_IS(dstDTypep, QueueDType)
                && !VN_IS(dstDTypep, DynArrayDType)) {
                ثابت صحيح sWidth = srcp->width();
                ثابت صحيح dWidth = nodep->lhsp()->width();
                ܐܢ (sWidth < dWidth) {
                    AstExtend* ثابت extendp = جديد AstExtend{srcp->fileline(), srcp, dWidth};
                    srcp = جديد AstShiftL{
                        srcp->fileline(), extendp,
                        جديد AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                        dWidth};
                }
            }
            nodep->rhsp(srcp);
            VL_DO_DANGLING(pushDeletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            ܦܢܐ ܫܪܪܐ;
        } ܐܚܪܢܐ ܐܢ (m_doV && VN_IS(nodep->lhsp(), StreamL)) {
            // Push the stream operator to the rhs of the assignment statement
            AstNodeExpr* streamp = nodep->lhsp()->unlinkFrBack();
            AstNodeExpr* ثابت dstp = VN_AS(streamp, StreamL)->lhsp()->unlinkFrBack();
            AstNodeDType* ثابت dstDTypep = dstp->dtypep()->skipRefp();
            AstNodeExpr* srcp = nodep->rhsp()->unlinkFrBack();
            ثابت AstNodeDType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamL reverses, so reverse=true)
            ܐܢ ((VN_IS(srcDTypep, UnpackArrayDType) || VN_IS(srcDTypep, QueueDType)
                 || VN_IS(srcDTypep, DynArrayDType))
                && (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType))) {
                صحيح blockSize = 1;
                ܐܢ (ثابت AstConst* ثابت constp
                    = VN_CAST(VN_AS(streamp, StreamL)->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    ܐܢ (VL_UNLIKELY(blockSize <= 0)) blockSize = 1;
                }
                صحيح srcElementBits = 0;
                ܐܢ (ثابت AstNodeDType* ثابت elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                صحيح dstElementBits = 0;
                ܐܢ (ثابت AstNodeDType* ثابت elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                nodep->lhsp(dstp);
                nodep->rhsp(جديد AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, ܫܪܪܐ,
                                                   blockSize, dstElementBits, srcElementBits});
                nodep->dtypep(dstDTypep);
                VL_DO_DANGLING(pushDeletep(streamp), streamp);
                ܦܢܐ ܫܪܪܐ;
            }
            ܐܢ (lowerAsFixedAggregate(srcDTypep)) srcp = packFixedAggregate(srcp);
            ثابت صحيح sWidth = srcp->width();
            ثابت صحيح dWidth = dstp->width();
            // Connect the rhs to the stream operator and update its width
            VN_AS(streamp, StreamL)->lhsp(srcp);
            ܐܢ (VN_IS(srcDTypep, DynArrayDType) || VN_IS(srcDTypep, QueueDType)
                || VN_IS(srcDTypep, UnpackArrayDType)) {
                streamp->dtypeSetStream();
            } ܐܚܪܢܐ {
                streamp->dtypeSetBitOrLogicUnsized(srcp->width(), srcp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   srcp->isFourstateOrNoDtype());
            }
            ܐܢ (VN_IS(dstDTypep, UnpackArrayDType)) {
                streamp = جديد AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
            } ܐܚܪܢܐ {
                ܐܢ (dWidth == 0) {
                    streamp = جديد AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
                } ܐܚܪܢܐ ܐܢ (sWidth >= dWidth) {
                    streamp = جديد AstSel{streamp->fileline(), streamp, sWidth - dWidth, dWidth};
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(streamp);
            nodep->dtypep(dstDTypep);
            ܦܢܐ ܫܪܪܐ;
        } ܐܚܪܢܐ ܐܢ (m_doV && VN_IS(nodep->lhsp(), StreamR)) {
            // The right stream operator on lhs of assignment statement does
            // not reorder bits. However, if the rhs is wider than the lhs,
            // then we select bits from the left-most, not the right-most.
            AstNodeExpr* ثابت streamp = nodep->lhsp()->unlinkFrBack();
            AstNodeExpr* ثابت dstp = VN_AS(streamp, StreamR)->lhsp()->unlinkFrBack();
            AstNodeDType* ثابت dstDTypep = dstp->dtypep()->skipRefp();
            AstNodeExpr* srcp = nodep->rhsp()->unlinkFrBack();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamR does not reverse, so reverse=false).
            // V3Width may have wrapped the source in CvtArrayToPacked; unwrap it.
            ܐܢ (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                AstNodeExpr* origSrcp = srcp;
                ܐܢ (AstCvtArrayToPacked* ثابت cvtp = VN_CAST(srcp, CvtArrayToPacked)) {
                    origSrcp = cvtp->fromp();
                }
                ثابت AstNodeDType* ثابت origSrcDTypep = origSrcp->dtypep()->skipRefp();
                ܐܢ (VN_IS(origSrcDTypep, UnpackArrayDType) || VN_IS(origSrcDTypep, QueueDType)
                    || VN_IS(origSrcDTypep, DynArrayDType)) {
                    صحيح srcElementBits = 0;
                    ܐܢ (ثابت AstNodeDType* ثابت elemDtp = origSrcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    صحيح dstElementBits = 0;
                    ܐܢ (ثابت AstNodeDType* ثابت elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    ܐܢ (VN_IS(srcp, CvtArrayToPacked)) {
                        origSrcp = VN_AS(srcp, CvtArrayToPacked)->fromp()->unlinkFrBack();
                        VL_DO_DANGLING(pushDeletep(srcp), srcp);
                        srcp = origSrcp;
                    }
                    // Descending unpacked arrays need element reversal
                    منطقي reverse = ܕܓܠܐ;
                    ܐܢ (ثابت AstUnpackArrayDType* ثابت unpackDtp
                        = VN_CAST(origSrcDTypep, UnpackArrayDType)) {
                        reverse = !unpackDtp->declRange().ascending();
                    }
                    nodep->lhsp(dstp);
                    nodep->rhsp(جديد AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, reverse,
                                                       1, dstElementBits, srcElementBits});
                    nodep->dtypep(dstDTypep);
                    VL_DO_DANGLING(pushDeletep(streamp), streamp);
                    ܦܢܐ ܫܪܪܐ;
                }
            }
            ثابت صحيح sWidth = srcp->width();
            ثابت صحيح dWidth = dstp->width();
            ܐܢ (VN_IS(dstDTypep, UnpackArrayDType)) {
                ثابت صحيح dstBitWidth
                    = dWidth * VN_AS(dstDTypep, UnpackArrayDType)->arrayUnpackedElements();
                // Handling the case where rhs is wider than lhs. StreamL does not require this
                // since the combination of the left streaming operation and the implicit
                // truncation in VL_ASSIGN_UNPACK automatically selects the left-most bits.
                ܐܢ (sWidth > dstBitWidth) {
                    srcp
                        = جديد AstSel{streamp->fileline(), srcp, sWidth - dstBitWidth, dstBitWidth};
                }
                srcp = جديد AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
            } ܐܚܪܢܐ {
                ܐܢ (dWidth == 0) {
                    srcp = جديد AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
                } ܐܚܪܢܐ ܐܢ (sWidth >= dWidth) {
                    srcp = جديد AstSel{streamp->fileline(), srcp, sWidth - dWidth, dWidth};
                } ܐܚܪܢܐ {
                    // Source narrower than destination: left-justify by shifting left.
                    // The right stream operator packs left-to-right, so remaining
                    // LSBs are zero-filled (IEEE 1800-2023 11.4.14.3).
                    ܐܢ (!VN_IS(srcp->dtypep()->skipRefp(), QueueDType)) {
                        AstExtend* ثابت extendp = جديد AstExtend{srcp->fileline(), srcp, dWidth};
                        srcp = جديد AstShiftL{
                            srcp->fileline(), extendp,
                            جديد AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                            dWidth};
                    } ܐܚܪܢܐ {  // if it is a queue we dont need do do a shiftL
                        srcp = جديد AstExtend{srcp->fileline(), srcp};
                    }
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(srcp);
            nodep->dtypep(dstDTypep);
            VL_DO_DANGLING(pushDeletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            ܦܢܐ ܫܪܪܐ;
        } ܐܚܪܢܐ ܐܢ (m_doV && VN_IS(nodep->rhsp(), StreamL)) {
            AstStreamL* streamp = VN_AS(nodep->rhsp(), StreamL);
            AstNodeExpr* srcp = streamp->lhsp();
            ثابت AstNodeDType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            AstNodeDType* ثابت dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            ܐܢ (lowerAsFixedAggregate(srcDTypep)) {
                AstNodeExpr* ثابت packedp = packFixedAggregate(srcp->unlinkFrBack());
                streamp->lhsp(packedp);
                streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   packedp->isFourstateOrNoDtype());
                srcp = packedp;
            } ܐܚܪܢܐ ܐܢ ((VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)
                        || VN_IS(srcDTypep, UnpackArrayDType))) {
                ܐܢ (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    صحيح blockSize = 1;
                    ܐܢ (ثابت AstConst* ثابت constp = VN_CAST(streamp->rhsp(), Const)) {
                        blockSize = constp->toSInt();
                        ܐܢ (VL_UNLIKELY(blockSize <= 0)) {
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
                    صحيح srcElementBits = 0;
                    ܐܢ (ثابت AstNodeDType* ثابت elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    صحيح dstElementBits = 0;
                    ܐܢ (ثابت AstNodeDType* ثابت elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    streamp->unlinkFrBack();
                    srcp = جديد AstCvtArrayToArray{
                        srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     ܫܪܪܐ,
                        blockSize,        dstElementBits,       srcElementBits};
                    nodep->rhsp(srcp);
                    VL_DO_DANGLING(pushDeletep(streamp), streamp);
                } ܐܚܪܢܐ {
                    streamp->lhsp(جديد AstCvtArrayToPacked{srcp->fileline(), srcp->unlinkFrBack(),
                                                          dstDTypep});
                    streamp->dtypeFrom(dstDTypep);
                }
            }
        } ܐܚܪܢܐ ܐܢ (m_doV && replaceAssignMultiSel(nodep)) {
            ܦܢܐ ܫܪܪܐ;
        } ܐܚܪܢܐ ܐܢ (replaceAssignSFormatF(nodep)) {
            ܦܢܐ ܫܪܪܐ;
        }
        ܦܢܐ ܕܓܠܐ;
    }

    // Boolean replacements
    منطقي operandBoolShift(ثابت AstNode* nodep) {
        // boolean test of AND(const,SHIFTR(x,const)) -> test of AND(SHIFTL(x,const), x)
        ܐܢ (!VN_IS(nodep, And)) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!VN_IS(VN_AS(nodep, And)->lhsp(), Const)) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!VN_IS(VN_AS(nodep, And)->rhsp(), ShiftR)) ܦܢܐ ܕܓܠܐ;
        ثابت AstShiftR* ثابت shiftp = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR);
        ܐܢ (!VN_IS(shiftp->rhsp(), Const)) ܦܢܐ ܕܓܠܐ;
        ܐܢ (static_cast<uint32_t>(nodep->width()) <= VN_AS(shiftp->rhsp(), Const)->toUInt()) {
            ܦܢܐ ܕܓܠܐ;
        }
        ܦܢܐ ܫܪܪܐ;
    }
    فراغ replaceBoolShift(AstNode* nodep) {
        UINFOTREE(9, nodep, "", "bshft_old");
        AstConst* ثابت andConstp = VN_AS(VN_AS(nodep, And)->lhsp(), Const);
        AstNodeExpr* ثابت fromp
            = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->lhsp()->unlinkFrBack();
        AstConst* ثابت shiftConstp
            = VN_AS(VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->rhsp(), Const);
        V3Number val{andConstp, andConstp->width()};
        val.opShiftL(andConstp->num(), shiftConstp->num());
        AstAnd* ثابت newp
            = جديد AstAnd{nodep->fileline(), جديد AstConst{nodep->fileline(), val}, fromp};
        // widthMin no longer applicable if different C-expanded width
        nodep->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        UINFOTREE(9, newp, "", "_new");
    }

    فراغ replaceWithSimulation(AstNode* nodep) {
        SimulateVisitor simvis;
        // Run it - may be unoptimizable due to large for loop, etc
        simvis.mainParamEmulate(nodep);
        ܐܢ (!simvis.optimizable()) {
            ثابت AstNode* errorp = simvis.whyNotNodep();
            ܐܢ (!errorp) errorp = nodep;
            nodep->v3error("Expecting expression to be constant, but can't determine constant for "
                           << nodep->prettyTypeName() << '\n'
                           << errorp->warnOther() << "... Location of non-constant "
                           << errorp->prettyTypeName() << ": " << simvis.whyNotMessage());
            VL_DO_DANGLING(replaceZero(nodep), nodep);
        } ܐܚܪܢܐ {
            // Fetch the result
            AstNode* ثابت valuep = simvis.fetchValueNull(nodep);  // valuep is owned by Simulate
            UASSERT_OBJ(valuep, nodep, "No value returned from simulation");
            // Replace it
            AstNode* ثابت newp = valuep->cloneTree(ܕܓܠܐ);
            newp->fileline(nodep->fileline());
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Simulate->" << newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }

    //----------------------------------------

    // VISITORS
    فراغ visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        iterateChildrenBackwardsConst(nodep);
    }
    فراغ visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_concswapNames.reset();
        iterateChildren(nodep);
    }
    فراغ visit(AstCFunc* nodep) override {
        // No ASSIGNW removals under funcs, we've long eliminated INITIALs
        // (We should perhaps rename the assignw's to just assigns)
        VL_RESTORER(m_wremove);
        m_wremove = ܕܓܠܐ;
        iterateChildren(nodep);
    }
    فراغ visit(AstCLocalScope* nodep) override {
        iterateChildren(nodep);
        ܐܢ (!nodep->stmtsp()) {
            nodep->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }
    فراغ visit(AstScope* nodep) override {
        // No ASSIGNW removals under scope, we've long eliminated INITIALs
        VL_RESTORER(m_wremove);
        VL_RESTORER(m_scopep);
        m_wremove = ܕܓܠܐ;
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    فراغ swapSides(AstNodeBiCom* nodep) {
        // COMMUTATIVE({a},CONST) -> COMMUTATIVE(CONST,{a})
        // This simplifies later optimizations
        AstNodeExpr* ثابت lhsp = nodep->lhsp()->unlinkFrBackWithNext();
        AstNodeExpr* ثابت rhsp = nodep->rhsp()->unlinkFrBackWithNext();
        nodep->lhsp(rhsp);
        nodep->rhsp(lhsp);
        iterate(nodep);  // Again?
    }

    منطقي containsMemberAccessRecurse(AstNode* ثابت nodep) {
        ܐܢ (!nodep) ܦܢܐ ܕܓܠܐ;
        ثابت size_t id = m_ids(nodep);
        ثابت auto it = m_containsMemberAccess.find(id);
        ܐܢ (it != m_containsMemberAccess.end()) ܦܢܐ it->second;
        ++m_statMemberAccessVisits;
        منطقي result = ܕܓܠܐ;
        ܐܢ (VN_IS(nodep, MemberSel) || VN_IS(nodep, MethodCall) || VN_IS(nodep, CMethodCall)) {
            result = ܫܪܪܐ;
        } ܐܚܪܢܐ ܐܢ (ثابت AstNodeFTaskRef* ثابت funcRefp = VN_CAST(nodep, NodeFTaskRef)) {
            ܐܢ (containsMemberAccessRecurse(funcRefp->taskp())) result = ܫܪܪܐ;
        } ܐܚܪܢܐ ܐܢ (ثابت AstNodeCCall* ثابت funcRefp = VN_CAST(nodep, NodeCCall)) {
            ܐܢ (containsMemberAccessRecurse(funcRefp->funcp())) result = ܫܪܪܐ;
        } ܐܚܪܢܐ ܐܢ (ثابت AstNodeFTask* ثابت funcp = VN_CAST(nodep, NodeFTask)) {
            // Assume that it has a member access
            ܐܢ (funcp->recursive()) result = ܫܪܪܐ;
        } ܐܚܪܢܐ ܐܢ (ثابت AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc)) {
            ܐܢ (funcp->recursive()) result = ܫܪܪܐ;
        }
        ܐܢ (!result) {
            result = containsMemberAccessRecurse(nodep->op1p())
                     || containsMemberAccessRecurse(nodep->op2p())
                     || containsMemberAccessRecurse(nodep->op3p())
                     || containsMemberAccessRecurse(nodep->op4p());
        }
        ܐܢ (!result && !VN_IS(nodep, NodeFTask)
            && !VN_IS(nodep, CFunc)  // don't enter into next function
            && containsMemberAccessRecurse(nodep->nextp())) {
            result = ܫܪܪܐ;
        }
        m_containsMemberAccess.emplace(id, result);
        ܦܢܐ result;
    }

    منطقي matchBiopToBitwise(AstNodeBiop* ثابت nodep) {
        ܐܢ (!m_convertLogicToBit) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!nodep->lhsp()->width1()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!nodep->rhsp()->width1()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!nodep->isPure()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (containsMemberAccessRecurse(nodep)) ܦܢܐ ܕܓܠܐ;
        ܦܢܐ ܫܪܪܐ;
    }
    منطقي matchConcatRand(AstConcat* nodep) {
        //    CONCAT(RAND, RAND) - created by Chisel code
        AstRand* ثابت aRandp = VN_CAST(nodep->lhsp(), Rand);
        ثابت AstRand* ثابت bRandp = VN_CAST(nodep->rhsp(), Rand);
        ܐܢ (!aRandp || !bRandp) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!aRandp->combinable(bRandp)) ܦܢܐ ܕܓܠܐ;
        UINFO(4, "Concat(Rand,Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        ܦܢܐ ܫܪܪܐ;
    }
    منطقي matchSelRand(AstSel* nodep) {
        //    SEL(RAND) - created by Chisel code
        AstRand* ثابت aRandp = VN_CAST(nodep->fromp(), Rand);
        ܐܢ (!aRandp) ܦܢܐ ܕܓܠܐ;
        ܐܢ (aRandp->seedp()) ܦܢܐ ܕܓܠܐ;
        UINFO(4, "Sel(Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        ܦܢܐ ܫܪܪܐ;
    }
    منطقي matchToStringNConst(AstToStringN* nodep) {
        iterateChildren(nodep);
        ܐܢ (ثابت AstConsArray* ثابت consp = VN_CAST(nodep->lhsp(), ConsArray)) {
            ܐܢ (!(m_doExpensive || m_params)) ܦܢܐ ܕܓܠܐ;
            ܐܢ (!consp->isConstantTree()) ܦܢܐ ܕܓܠܐ;
        } ܐܚܪܢܐ ܐܢ (!VN_IS(nodep->lhsp(), Const)) {
            ܦܢܐ ܕܓܠܐ;
        }
        replaceWithSimulation(nodep);
        ܦܢܐ ܫܪܪܐ;
    }
    صحيح operandConcatMove(ثابت AstConcat* nodep) {
        //    CONCAT under concat  (See moveConcat)
        // Return value: true indicates to do it; 2 means move to LHS
        ثابت AstConcat* ثابت abConcp = VN_CAST(nodep->lhsp(), Concat);
        ثابت AstConcat* ثابت bcConcp = VN_CAST(nodep->rhsp(), Concat);
        ܐܢ (!abConcp && !bcConcp) ܦܢܐ 0;
        ܐܢ (bcConcp) {
            ثابت AstNodeExpr* ثابت ap = nodep->lhsp();
            ثابت AstNodeExpr* ثابت bp = bcConcp->lhsp();
            // If a+b == 32,64,96 etc, then we want to have a+b together on LHS
            ܐܢ (VL_BITBIT_I(ap->width() + bp->width()) == 0) ܦܢܐ 2;  // Transform 2: to abConc
        } ܐܚܪܢܐ {  // abConcp
            // Unless lhs is already 32 bits due to above, reorder it
            ܐܢ (VL_BITBIT_I(nodep->lhsp()->width()) != 0) ܦܢܐ 1;  // Transform 1: to bcConc
        }
        ܦܢܐ 0;  // ok
    }
    فراغ moveConcat(AstConcat* nodep) {
        //    1: CONCAT(CONCAT({a},{b}),{c})  -> CONCAT({a},CONCAT({b}, {c}))
        // or 2: CONCAT({a}, CONCAT({b},{c})) -> CONCAT(CONCAT({a},{b}),{c})
        // Because the lhs of a concat needs a shift but the rhs doesn't,
        // putting additional CONCATs on the RHS leads to fewer assembler operations.
        // However, we'll end up with lots of wide moves if we make huge trees
        // like that, so on 32 bit boundaries, we'll do the opposite form.
        UINFO(4, "Move concat: " << nodep);
        ܐܢ (operandConcatMove(nodep) > 1) {
            AstNodeExpr* ثابت ap = nodep->lhsp()->unlinkFrBack();
            AstConcat* ثابت bcConcp = VN_AS(nodep->rhsp(), Concat);
            bcConcp->unlinkFrBack();
            AstNodeExpr* ثابت bp = bcConcp->lhsp()->unlinkFrBack();
            AstNodeExpr* ثابت cp = bcConcp->rhsp()->unlinkFrBack();
            AstConcat* ثابت abConcp = جديد AstConcat{bcConcp->fileline(), ap, bp};
            nodep->lhsp(abConcp);
            nodep->rhsp(cp);
            // If bp was a concat, then we have this exact same form again!
            // Recurse rather then calling node->iterate to prevent 2^n recursion!
            ܐܢ (operandConcatMove(abConcp)) moveConcat(abConcp);
            VL_DO_DANGLING(pushDeletep(bcConcp), bcConcp);
        } ܐܚܪܢܐ {
            AstConcat* ثابت abConcp = VN_AS(nodep->lhsp(), Concat);
            abConcp->unlinkFrBack();
            AstNodeExpr* ثابت ap = abConcp->lhsp()->unlinkFrBack();
            AstNodeExpr* ثابت bp = abConcp->rhsp()->unlinkFrBack();
            AstNodeExpr* ثابت cp = nodep->rhsp()->unlinkFrBack();
            AstConcat* ثابت bcConcp = جديد AstConcat{abConcp->fileline(), bp, cp};
            nodep->lhsp(ap);
            nodep->rhsp(bcConcp);
            ܐܢ (operandConcatMove(bcConcp)) moveConcat(bcConcp);
            VL_DO_DANGLING(pushDeletep(abConcp), abConcp);
        }
    }

    // Special cases
    فراغ visit(AstConst*) override {}  // Already constant

    فراغ visit(AstCell* nodep) override {
        ܐܢ (m_params) {
            iterateAndNextNull(nodep->paramsp());
        } ܐܚܪܢܐ {
            iterateChildren(nodep);
        }
    }
    فراغ visit(AstClassOrPackageRef* nodep) override { iterateChildren(nodep); }

    فراغ visit(AstMatchMasked* nodep) override {
        // Do not iterate the tables, they must be constant pool entries
        iterate(nodep->lhsp());
        ܐܢ (AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const)) {
            replaceNum(nodep, AstMatchMasked::fold(constp->num(), nodep->matchp()->varp()));
        }
    }

    فراغ visit(AstPin* nodep) override { iterateChildren(nodep); }

    فراغ replaceLogEq(AstLogEq* nodep) {
        // LOGEQ(a,b) => AstLogAnd{AstLogOr{AstLogNot{a},b},AstLogOr{AstLogNot{b},a}}
        AstNodeExpr* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        AstNodeExpr* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        // Do exactly as IEEE says, might result in extra terms, so in future may do differently
        AstLogAnd* ثابت newp = جديد AstLogAnd{
            nodep->fileline(),
            جديد AstLogOr{nodep->fileline(), جديد AstLogNot{nodep->fileline(), lhsp}, rhsp},
            جديد AstLogOr{nodep->fileline(),
                         جديد AstLogNot{nodep->fileline(), rhsp->cloneTreePure(ܕܓܠܐ)},
                         lhsp->cloneTreePure(ܕܓܠܐ)}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    فراغ replaceSelSel(AstSel* nodep) {
        // SEL(SEL({x},a,b),c,d) => SEL({x},a+c,d)
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ثابت belowp = VN_AS(nodep->fromp(), Sel);
        AstNodeExpr* ثابت fromp = belowp->fromp()->unlinkFrBack();
        AstNodeExpr* ثابت lsb1p = nodep->lsbp()->unlinkFrBack();
        AstNodeExpr* ثابت lsb2p = belowp->lsbp()->unlinkFrBack();
        // Eliminate lower range
        UINFO(4, "Elim Lower range: " << nodep);
        AstNodeExpr* newlsbp;
        ܐܢ (VN_IS(lsb1p, Const) && VN_IS(lsb2p, Const)) {
            newlsbp = جديد AstConst{lsb1p->fileline(),
                                   VN_AS(lsb1p, Const)->toUInt() + VN_AS(lsb2p, Const)->toUInt()};
            VL_DO_DANGLING(pushDeletep(lsb1p), lsb1p);
            VL_DO_DANGLING(pushDeletep(lsb2p), lsb2p);
        } ܐܚܪܢܐ {
            // Width is important, we need the width of the fromp's expression, not the
            // potentially smaller lsb1p's width, but don't insert a redundant AstExtend.
            // Note that due to some sloppiness in earlier passes, lsb1p might actually be wider,
            // so extend to the wider type.
            ثابت AstNodeExpr* ثابت widep = lsb1p->width() > lsb2p->width() ? lsb1p : lsb2p;
            AstNodeExpr* ثابت lhsp = widep->width() > lsb2p->width()
                                          ? جديد AstExtend{lsb2p->fileline(), lsb2p}
                                          : lsb2p;
            AstNodeExpr* ثابت rhsp = widep->width() > lsb1p->width()
                                          ? جديد AstExtend{lsb1p->fileline(), lsb1p}
                                          : lsb1p;
            lhsp->dtypeFrom(widep);
            rhsp->dtypeFrom(widep);
            newlsbp = جديد AstAdd{lsb1p->fileline(), lhsp, rhsp};
            newlsbp->dtypeFrom(widep);
        }
        AstSel* ثابت newp = جديد AstSel{nodep->fileline(), fromp, newlsbp, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    فراغ replaceSelConcat(AstSel* nodep) {
        // SEL(CONCAT(a,b),c,d) => SEL(a or b, . .)
        AstConcat* ثابت conp = VN_AS(nodep->fromp(), Concat);
        AstNodeExpr* ثابت conLhsp = conp->lhsp();
        AstNodeExpr* ثابت conRhsp = conp->rhsp();
        ܐܢ (static_cast<صحيح>(nodep->lsbConst()) >= conRhsp->width()) {
            conLhsp->unlinkFrBack();
            AstSel* ثابت newp
                = جديد AstSel{nodep->fileline(), conLhsp, nodep->lsbConst() - conRhsp->width(),
                             nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } ܐܚܪܢܐ ܐܢ (static_cast<صحيح>(nodep->msbConst()) < conRhsp->width()) {
            conRhsp->unlinkFrBack();
            AstSel* ثابت newp
                = جديد AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(), nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } ܐܚܪܢܐ {
            // Yuk, split between the two
            conRhsp->unlinkFrBack();
            conLhsp->unlinkFrBack();
            AstConcat* ثابت newp
                = جديد AstConcat{nodep->fileline(),
                                جديد AstSel{nodep->fileline(), conLhsp, 0,
                                           nodep->msbConst() - conRhsp->width() + 1},
                                جديد AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(),
                                           conRhsp->width() - nodep->lsbConst()}};
            nodep->replaceWithKeepDType(newp);
        }
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    منطقي operandSelReplicate(AstSel* nodep) {
        // SEL(REPLICATE(from,rep),lsb,width) => SEL(from,0,width) as long
        // as SEL's width <= b's width
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ثابت repp = VN_AS(nodep->fromp(), Replicate);
        AstNodeExpr* ثابت fromp = repp->srcp();
        ثابت AstConst* ثابت lsbp = VN_CAST(nodep->lsbp(), Const);
        ܐܢ (!lsbp) ܦܢܐ ܕܓܠܐ;
        UASSERT_OBJ(fromp->width(), nodep, "Not widthed");
        ܐܢ ((lsbp->toUInt() / fromp->width())
            != ((lsbp->toUInt() + nodep->width() - 1) / fromp->width())) {
            ܦܢܐ ܕܓܠܐ;
        }
        //
        fromp->unlinkFrBack();
        AstSel* ثابت newp = جديد AstSel{
            nodep->fileline(), fromp,
            جديد AstConst{lsbp->fileline(), lsbp->toUInt() % fromp->width()}, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        ܦܢܐ ܫܪܪܐ;
    }
    منطقي operandRepRep(AstReplicate* nodep) {
        // REPLICATE(REPLICATE2(from2,cnt2),cnt1) => REPLICATE(from2,(cnt1+cnt2))
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ثابت rep2p = VN_AS(nodep->srcp(), Replicate);
        AstNodeExpr* ثابت from2p = rep2p->srcp();
        ثابت AstConst* ثابت cnt1p = VN_CAST(nodep->countp(), Const);
        ܐܢ (!cnt1p) ܦܢܐ ܕܓܠܐ;
        ثابت AstConst* ثابت cnt2p = VN_CAST(rep2p->countp(), Const);
        ܐܢ (!cnt2p) ܦܢܐ ܕܓܠܐ;
        //
        from2p->unlinkFrBack();
        AstReplicate* ثابت newp
            = جديد AstReplicate{nodep->fileline(), from2p, cnt1p->toUInt() * cnt2p->toUInt()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        ܦܢܐ ܫܪܪܐ;
    }
    منطقي operandConcatSame(AstConcat* nodep) {
        // CONCAT(fromp,fromp) -> REPLICATE(fromp,1+1)
        // CONCAT(REP(fromp,cnt1),fromp) -> REPLICATE(fromp,cnt1+1)
        // CONCAT(fromp,REP(fromp,cnt1)) -> REPLICATE(fromp,1+cnt1)
        // CONCAT(REP(fromp,cnt1),REP(fromp,cnt2)) -> REPLICATE(fromp,cnt1+cnt2)
        AstNodeExpr* from1p = nodep->lhsp();
        uint32_t cnt1 = 1;
        AstNodeExpr* from2p = nodep->rhsp();
        uint32_t cnt2 = 1;
        ܐܢ (VN_IS(from1p, Replicate)) {
            ثابت AstConst* ثابت cnt1p = VN_CAST(VN_CAST(from1p, Replicate)->countp(), Const);
            ܐܢ (!cnt1p) ܦܢܐ ܕܓܠܐ;
            from1p = VN_AS(from1p, Replicate)->srcp();
            cnt1 = cnt1p->toUInt();
        }
        ܐܢ (VN_IS(from2p, Replicate)) {
            ثابت AstConst* ثابت cnt2p = VN_CAST(VN_CAST(from2p, Replicate)->countp(), Const);
            ܐܢ (!cnt2p) ܦܢܐ ܕܓܠܐ;
            from2p = VN_AS(from2p, Replicate)->srcp();
            cnt2 = cnt2p->toUInt();
        }
        ܐܢ (!operandsSame(from1p, from2p)) ܦܢܐ ܕܓܠܐ;
        //
        from1p->unlinkFrBack();
        AstReplicate* ثابت newp = جديد AstReplicate{nodep->fileline(), from1p, cnt1 + cnt2};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        ܦܢܐ ܫܪܪܐ;
    }
    فراغ replaceSelIntoBiop(AstSel* nodep) {
        // SEL(BUFIF1(a,b),1,bit) => BUFIF1(SEL(a,1,bit),SEL(b,1,bit))
        AstNodeBiop* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), NodeBiop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        AstNodeExpr* ثابت lsbp = nodep->lsbp()->unlinkFrBack();
        //
        AstNodeExpr* ثابت bilhsp = fromp->lhsp()->unlinkFrBack();
        AstNodeExpr* ثابت birhsp = fromp->rhsp()->unlinkFrBack();
        //
        fromp->lhsp(
            جديد AstSel{nodep->fileline(), bilhsp, lsbp->cloneTreePure(ܫܪܪܐ), nodep->widthConst()});
        fromp->rhsp(جديد AstSel{nodep->fileline(), birhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ replaceSelIntoUniop(AstSel* nodep) {
        // SEL(NOT(a),1,bit) => NOT(SEL(a,bit))
        AstNodeUniop* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), NodeUniop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        AstNodeExpr* ثابت lsbp = nodep->lsbp()->unlinkFrBack();
        //
        AstNodeExpr* ثابت bilhsp = fromp->lhsp()->unlinkFrBack();
        //
        fromp->lhsp(جديد AstSel{nodep->fileline(), bilhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    فراغ visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        iterateChildren(nodep);
    }

    فراغ visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->bitp());
        ܐܢ (VN_IS(nodep->bitp(), Const)
            && VN_IS(nodep->fromp(), VarRef)
            // Need to make sure it's an array object so don't mis-allow a constant (bug509.)
            && VN_AS(nodep->fromp(), VarRef)->varp()
            && VN_IS(VN_AS(nodep->fromp(), VarRef)->varp()->valuep(), ConsArray)) {
            m_selp = nodep;  // Ask visit(AstVarRef) to replace varref with const
        }
        iterateAndNextNull(nodep->fromp());
        ܐܢ (VN_IS(nodep->fromp(), Const)) {  // It did.
            ܐܢ (!m_selp) {
                nodep->v3error("Illegal assignment of constant to unpacked array");
            } ܐܚܪܢܐ {
                AstNode* ثابت fromp = nodep->fromp()->unlinkFrBack();
                nodep->replaceWithKeepDType(fromp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            }
        }
        // Handle ARRAYSEL directly on ConsArray (not through VarRef)
        ܐܚܪܢܐ ܐܢ (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ConsArray)) {
            ثابت AstConsArray* ثابت consp = VN_AS(nodep->fromp(), ConsArray);
            ثابت uint32_t bit = VN_AS(nodep->bitp(), Const)->toUInt();
            AstNodeExpr* ثابت itemp = consp->valuep(bit);
            ܐܢ (VN_IS(itemp, Const)) {
                ثابت V3Number& num = VN_AS(itemp, Const)->num();
                VL_DO_DANGLING(replaceNum(nodep, num), nodep);
            } ܐܚܪܢܐ ܐܢ (m_params && itemp) {
                // An aggregate element, e.g. a structure in an array parameter
                nodep->replaceWithKeepDType(itemp->cloneTree(ܕܓܠܐ));
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            }
        }
        m_selp = nullptr;
    }

    // Evaluate a slice of an unpacked array.  If constantification is
    // required (m_required=true), call replaceWithSimulation() to compute
    // the slice via simulation.  Otherwise just iterate the children.
    فراغ visit(AstSliceSel* nodep) override {
        // First constify or width any child nodes
        iterateChildren(nodep);
        ܐܢ (!m_required) ܦܢܐ;  // Do nothing unless we are in parameter mode
        // Fallback to simulation: this will invoke SimulateVisitor::visit(AstSliceSel*)
        replaceWithSimulation(nodep);
    }

    فراغ visit(AstStructSel* nodep) override {
        iterateChildren(nodep);

        ܐܢ (ثابت AstConsStruct* ثابت consp = VN_CAST(nodep->fromp(), ConsStruct)) {
            AstNodeExpr* ثابت valuep = consp->memberValuep(nodep->name());
            ܐܢ (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
                VL_DO_DANGLING(replaceNum(nodep, constp->num()), nodep);
            } ܐܚܪܢܐ {
                nodep->replaceWith(valuep->cloneTree(ܕܓܠܐ));
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            }
        }
    }

    فراغ visit(AstCAwait* nodep) override {
        m_hasJumpDelay = ܫܪܪܐ;
        iterateChildren(nodep);
    }
    فراغ visit(AstNodeVarRef* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Not linked");
        منطقي did = ܕܓܠܐ;
        ܐܢ (m_doV && (!nodep->varp()->constPoolEntry() || m_selp) && nodep->varp()->valuep()
            && !m_attrp) {
            // UINFOTREE(1, valuep, "", "visitvaref");
            iterateAndNextNull(nodep->varp()->valuep());  // May change nodep->varp()->valuep()
            AstNode* ثابت valuep = nodep->varp()->valuep();
            // The selected element, when under a constant select of an array
            ثابت AstConsArray* ثابت consp = VN_CAST(valuep, ConsArray);
            ثابت AstNode* ثابت selItemp
                = (m_selp && consp) ? consp->valuep(m_selp->bitConst()) : nullptr;
            ܐܢ (nodep->access().isReadOnly() && valuep
                && ((!m_params  // Can reduce constant wires into equations
                     && m_doNConst && v3Global.opt.fConst() && nodep->varp()->isConst())
                    || nodep->varp()->isParam())) {
                ܐܢ (operandConst(valuep)) {
                    ثابت V3Number& num = VN_AS(valuep, Const)->num();
                    // UINFO(2, "constVisit " << cvtToHex(valuep) << " " << num);
                    ثابت string origParamName
                        = nodep->varp()->isParam() ? nodep->varp()->name() : "";
                    VL_DO_DANGLING(replaceNum(nodep, num, origParamName), nodep);
                    did = ܫܪܪܐ;
                } ܐܚܪܢܐ ܐܢ (ثابت AstConst* ثابت itemp = VN_CAST(selItemp, Const)) {
                    VL_DO_DANGLING(replaceNum(nodep, itemp->num()), nodep);
                    did = ܫܪܪܐ;
                } ܐܚܪܢܐ ܐܢ (m_params && consp) {
                    // Allow parameters to pass arrays
                    // Earlier recursion of ConsArray made sure each array value is constant
                    // This exception is fairly fragile, i.e. doesn't
                    // support arrays of arrays or other stuff
                    AstNode* ثابت newp = valuep->cloneTree(ܕܓܠܐ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    did = ܫܪܪܐ;
                } ܐܚܪܢܐ ܐܢ (m_params && VN_IS(valuep, ConsStruct)) {
                    AstNode* ثابت newp = valuep->cloneTree(ܕܓܠܐ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    did = ܫܪܪܐ;
                } ܐܚܪܢܐ ܐܢ (nodep->varp()->isParam() && VN_IS(valuep, Unbounded)) {
                    AstNode* ثابت newp = valuep->cloneTree(ܕܓܠܐ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    did = ܫܪܪܐ;
                }
            }
        }
        ܐܢ (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but variable isn't const: "
                           << nodep->varp()->prettyNameQ());
        }
    }
    فراغ visit(AstExprStmt* nodep) override {
        iterateChildren(nodep);
        ܐܢ (!AstNode::afterCommentp(nodep->stmtsp())) {
            deleteVarScopesUnder(nodep->stmtsp());
            UINFO(8, "ExprStmt(...) " << nodep << " " << nodep->resultp());
            nodep->replaceWith(nodep->resultp()->unlinkFrBack());
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            // Removing the ExprStmt might have made something impure above now pure
        }
    }
    فراغ visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        منطقي did = ܕܓܠܐ;
        ܐܢ (nodep->itemp()->valuep()) {
            // UINFOTREE(1, nodep->itemp()->valuep(), "", "visitvaref");
            ثابت AstEnumItem* ثابت itemp = nodep->itemp();
            ܐܢ (m_recursingEnumItems.count(itemp)) {
                nodep->v3error("Recursive enum value: " << nodep->itemp()->prettyNameQ());
            } ܐܚܪܢܐ {
                m_recursingEnumItems.emplace(itemp);
                iterateAndNextNull(nodep->itemp()->valuep());
                m_recursingEnumItems.erase(itemp);
            }
            ܐܢ (AstConst* ثابت valuep = VN_CAST(nodep->itemp()->valuep(), Const)) {
                ثابت V3Number& num = valuep->num();
                VL_DO_DANGLING(replaceNum(nodep, num), nodep);
                did = ܫܪܪܐ;
            }
        }
        ܐܢ (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but enum value isn't const: "
                           << nodep->itemp()->prettyNameQ());
        }
    }

    منطقي onlySenItemInSenTree(ثابت AstSenItem* nodep) {
        // Only one if it's not in a list
        ܦܢܐ (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }
    فراغ visit(AstSenItem* nodep) override {
        iterateChildren(nodep);
        ܐܢ (m_doNConst
            && !v3Global.opt.timing().isSetTrue()  // If --timing, V3Sched would turn this into an
                                                   // infinite loop. See #5080
            && (VN_IS(nodep->sensp(), Const) || VN_IS(nodep->sensp(), EnumItemRef)
                || (nodep->varrefp() && nodep->varrefp()->varp()->isParam()))) {
            // Constants in sensitivity lists may be removed (we'll simplify later)
            ܐܢ (nodep->isClocked()) {  // A constant can never get a pos/negedge
                ܐܢ (onlySenItemInSenTree(nodep)) {
                    ܐܢ (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                        // TODO: This really is dodgy, as strictly compliant simulators will not
                        //       execute this block, but but t_func_check relies on it
                        nodep->replaceWith(
                            جديد AstSenItem{nodep->fileline(), AstSenItem::Initial{}});
                    } ܐܚܪܢܐ {
                        nodep->replaceWith(جديد AstSenItem{nodep->fileline(), AstSenItem::Never{}});
                    }
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                } ܐܚܪܢܐ {
                    VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                }
            } ܐܚܪܢܐ {  // Otherwise it may compute a result that needs to settle out
                nodep->replaceWith(جديد AstSenItem{nodep->fileline(), AstSenItem::Combo{}});
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            }
        } ܐܚܪܢܐ ܐܢ (m_doNConst && VN_IS(nodep->sensp(), Not)) {
            // V3Gate may propagate NOTs into clocks... Just deal with it
            AstNode* ثابت sensp = nodep->sensp();
            AstNode* lastSensp = sensp;
            منطقي invert = ܕܓܠܐ;
            ܟܡܐ (VN_IS(lastSensp, Not)) {
                lastSensp = VN_AS(lastSensp, Not)->lhsp();
                invert = !invert;
            }
            UINFO(8, "senItem(NOT...) " << nodep << " " << invert);
            ܐܢ (invert) nodep->edgeType(nodep->edgeType().invert());
            sensp->replaceWith(lastSensp->unlinkFrBack());
            VL_DO_DANGLING(pushDeletep(sensp), sensp);
        }
    }

    صنف SenItemCmp final {
        ساكن صحيح cmp(ثابت AstNodeExpr* ap, ثابت AstNodeExpr* bp) {
            ثابت VNType aType = ap->type();
            ثابت VNType bType = bp->type();
            ܐܢ (aType != bType) ܦܢܐ static_cast<صحيح>(bType) - static_cast<صحيح>(aType);

            ܐܢ (ثابت AstVarRef* ثابت aRefp = VN_CAST(ap, VarRef)) {
                ثابت AstVarRef* ثابت bRefp = VN_AS(bp, VarRef);
                // Looks visually better if we keep sorted by name
                ܐܢ (aRefp->name() < bRefp->name()) ܦܢܐ -1;
                ܐܢ (aRefp->name() > bRefp->name()) ܦܢܐ 1;
                // But might be same name with different scopes
                ܐܢ (aRefp->varScopep() < bRefp->varScopep()) ܦܢܐ -1;
                ܐܢ (aRefp->varScopep() > bRefp->varScopep()) ܦܢܐ 1;
                // Or rarely, different data types
                ܐܢ (aRefp->dtypep() < bRefp->dtypep()) ܦܢܐ -1;
                ܐܢ (aRefp->dtypep() > bRefp->dtypep()) ܦܢܐ 1;
                ܦܢܐ 0;
            }

            ܐܢ (ثابت AstConst* ثابت aConstp = VN_CAST(ap, Const)) {
                ثابت AstConst* ثابت bConstp = VN_AS(bp, Const);
                ܐܢ (aConstp->num().isLtXZ(bConstp->num())) ܦܢܐ -1;
                ܐܢ (bConstp->num().isLtXZ(aConstp->num())) ܦܢܐ 1;
                ܦܢܐ 0;
            }

            ܐܢ (ثابت AstNodeBiop* ثابت aBiOpp = VN_CAST(ap, NodeBiop)) {
                ثابت AstNodeBiop* ثابت bBiOpp = VN_AS(bp, NodeBiop);
                // Compare RHSs first, as LHS might be const
                ܐܢ (ثابت صحيح c = cmp(aBiOpp->rhsp(), bBiOpp->rhsp())) ܦܢܐ c;
                ܦܢܐ cmp(aBiOpp->lhsp(), bBiOpp->lhsp());
            }

            ܐܢ (ثابت AstCMethodHard* ثابت aCallp = VN_CAST(ap, CMethodHard)) {
                ثابت AstCMethodHard* ثابت bCallp = VN_AS(bp, CMethodHard);
                ܐܢ (aCallp->method() < bCallp->method()) ܦܢܐ -1;
                ܐܢ (aCallp->method() > bCallp->method()) ܦܢܐ 1;
                ܐܢ (ثابت صحيح c = cmp(aCallp->fromp(), bCallp->fromp())) ܦܢܐ c;
                ثابت AstNodeExpr* aPinsp = aCallp->pinsp();
                ثابت AstNodeExpr* bPinsp = bCallp->pinsp();
                ܟܡܐ (aPinsp && bPinsp) {
                    ܐܢ (ثابت صحيح c = cmp(aPinsp, bPinsp)) ܦܢܐ c;
                    aPinsp = VN_AS(aPinsp->nextp(), NodeExpr);
                    bPinsp = VN_AS(bPinsp->nextp(), NodeExpr);
                }
                ܦܢܐ aPinsp ? -1 : bPinsp ? 1 : 0;
            }

            ܦܢܐ 0;
        }

    عام:
        منطقي operator()(ثابت AstSenItem* lhsp, ثابت AstSenItem* rhsp) ثابت {
            ثابت AstNodeExpr* ثابت lSensp = lhsp->sensp();
            ثابت AstNodeExpr* ثابت rSensp = rhsp->sensp();
            ܐܢ (lSensp && rSensp) {
                // If both terms have sensitivity expressions, recursively compare them
                ܐܢ (ثابت صحيح c = cmp(lSensp, rSensp)) ܦܢܐ c < 0;
            } ܐܚܪܢܐ ܐܢ (lSensp || rSensp) {
                // Terms with sensitivity expressions come after those without
                ܦܢܐ rSensp;
            }
            // Finally sort by edge, AFTER variable, as we want multiple edges for same var
            // adjacent. note the SenTree optimizer requires this order (more general first,
            // less general last)
            ܦܢܐ lhsp->edgeType() < rhsp->edgeType();
        }
    };

    فراغ visit(AstSenTree* nodep) override {
        iterateChildren(nodep);
        ܐܢ (m_doExpensive) {
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
            لكل (AstSenItem *nextp, *senp = nodep->sensesp(); senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                // cppcheck-suppress unassignedVariable  // cppcheck bug
                ثابت SenItemCmp cmp;
                ܐܢ (nextp && !cmp(senp, nextp)) {
                    // Something's out of order, sort it
                    senp = nullptr;
                    std::vector<AstSenItem*> vec;
                    لكل (AstSenItem* senp = nodep->sensesp(); senp;
                         senp = VN_AS(senp->nextp(), SenItem)) {
                        vec.push_back(senp);
                    }
                    stable_sort(vec.begin(), vec.end(), SenItemCmp());
                    لكل (ثابت auto& ip : vec) ip->unlinkFrBack();
                    لكل (ثابت auto& ip : vec) nodep->addSensesp(ip);
                    اكسر;
                }
            }

            // Pass 2, remove duplicates and simplify adjacent terms if possible
            لكل (AstSenItem *senp = nodep->sensesp(), *nextp; senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                ܐܢ (!nextp) اكسر;
                AstSenItem* ثابت lItemp = senp;
                AstSenItem* ثابت rItemp = nextp;
                ثابت AstNodeExpr* ثابت lSenp = lItemp->sensp();
                ثابت AstNodeExpr* ثابت rSenp = rItemp->sensp();
                ܐܢ (!lSenp || !rSenp) استمر;

                ܐܢ (lSenp->sameGateTree(rSenp)) {
                    // POSEDGE or NEGEDGE -> BOTHEDGE. (We've sorted POSEDGE, before NEGEDGE, so we
                    // do not need to test for the opposite orders.)
                    ܐܢ (lItemp->edgeType() == VEdgeType::ET_POSEDGE
                        && rItemp->edgeType() == VEdgeType::ET_NEGEDGE) {
                        // Make both terms BOTHEDGE, the second will be removed below
                        lItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                        rItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                    }

                    // Remove identical expressions
                    ܐܢ (lItemp->edgeType() == rItemp->edgeType()) {
                        VL_DO_DANGLING(pushDeletep(rItemp->unlinkFrBack()), rItemp);
                        nextp = lItemp;
                    }
                }
            }
        }
    }

    //-----
    // Zero elimination
    فراغ visit(AstNodeAssign* nodep) override {
        iterateChildren(nodep);
        ܐܢ (nodep->timingControlp()) m_hasJumpDelay = ܫܪܪܐ;
        ܐܢ (m_doNConst && replaceNodeAssign(nodep)) ܦܢܐ;
    }
    فراغ visit(AstAlias* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    فراغ visit(AstAliasScope* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    فراغ visit(AstAssignW* nodep) override {
        iterateChildren(nodep);
        ܐܢ (m_doNConst && replaceNodeAssign(nodep)) ܦܢܐ;
        // Process containing this AssignW as single body statement
        AstAlways* ثابت procp = VN_CAST(nodep->backp(), Always);
        ܐܢ (!procp || procp->stmtsp() != nodep || nodep->nextp()) ܦܢܐ;
        // Not VarXRef, as different refs may set different values to each hierarchy
        AstNodeVarRef* ثابت varrefp = VN_CAST(nodep->lhsp(), VarRef);
        ܐܢ (m_wremove && !m_params && m_doNConst && m_modp && operandConst(nodep->rhsp())
            && !VN_AS(nodep->rhsp(), Const)->num().isFourState()
            && varrefp  // Don't do messes with BITREFs/ARRAYREFs
            && !varrefp->varp()->hasStrengthAssignment()  // Strengths are resolved in V3Tristate
            && !varrefp->varp()->valuep()  // Not already constified
            && !varrefp->varScopep()  // Not scoped (or each scope may have different initial val.)
            && !varrefp->varp()->isForced()  // Not forced (not really a constant)
        ) {
            // ASSIGNW (VARREF, const) -> INITIAL ( ASSIGN (VARREF, const) )
            UINFO(4, "constAssignW " << nodep);
            // Make a initial assignment
            AstNodeExpr* ثابت exprp = nodep->rhsp()->unlinkFrBack();
            varrefp->unlinkFrBack();
            FileLine* ثابت flp = nodep->fileline();
            AstInitial* ثابت newinitp = جديد AstInitial{flp, جديد AstAssign{flp, varrefp, exprp}};
            procp->replaceWith(newinitp);
            VL_DO_DANGLING(pushDeletep(procp), procp);
            // Set the initial value right in the variable so we can constant propagate
            AstNode* ثابت initvaluep = exprp->cloneTree(ܕܓܠܐ);
            varrefp->varp()->isConst(ܫܪܪܐ);
            varrefp->varp()->valuep(initvaluep);
        }
    }
    فراغ visit(AstCReset* nodep) override {
        iterateChildren(nodep);
        ܐܢ (!m_doNConst) ܦܢܐ;
        ثابت AstBasicDType* ثابت bdtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType);
        ܐܢ (!bdtypep) ܦܢܐ;
        ܐܢ (!bdtypep->isZeroInit()) ܦܢܐ;
        AstConst* ثابت newp = جديد AstConst{nodep->fileline(), V3Number{nodep, bdtypep}};
        UINFO(9, "CRESET(0) => CONST(0) " << nodep);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstCvtUnpackToUnpack* nodep) override {
        iterateChildren(nodep);
        AstConsArray* ثابت fromp = VN_CAST(nodep->fromp(), ConsArray);
        ܐܢ (!fromp) ܦܢܐ;
        // Fold the outermost dimension, converting the elements for any dimension under it
        ثابت AstUnpackArrayDType* ثابت toArrayp
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت AstUnpackArrayDType* ثابت fromArrayp
            = VN_AS(fromp->dtypep()->skipRefp(), UnpackArrayDType);
        // This dimension may not be reversed, only one under it
        ثابت منطقي reverse
            = toArrayp->declRange().ascending() != fromArrayp->declRange().ascending();
        ثابت uint64_t lastIndex = toArrayp->elementsConst() - 1;
        AstNodeDType* ثابت subDTypep = toArrayp->subDTypep();
        ثابت AstUnpackArrayDType* ثابت subArrayp
            = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
        ثابت auto convert = [&](AstNodeExpr* valuep) -> AstNodeExpr* {
            valuep->unlinkFrBack();
            ܐܢ (!subArrayp || !subArrayp->isReverseOf(valuep->dtypep())) ܦܢܐ valuep;
            ܦܢܐ جديد AstCvtUnpackToUnpack{valuep->fileline(), valuep, subDTypep};
        };
        AstConsArray* ثابت newp
            = جديد AstConsArray{fromp->fileline(), nodep->dtypep(),
                               fromp->defaultp() ? convert(fromp->defaultp()) : nullptr};
        لكل (ثابت AstConsArrayItem* itemp = fromp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            newp->valuep(reverse ? lastIndex - itemp->index() : itemp->index(),
                         convert(itemp->valuep()));
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        iterate(newp);  // Fold the conversions of the elements
    }
    فراغ visit(AstCvtArrayToArray* nodep) override {
        iterateChildren(nodep);
        // Handle the case where we have a stream operation inside a cast conversion
        // Check for both StreamL and StreamR operations
        AstNodeStream* streamp = nullptr;
        منطقي isReverse = ܕܓܠܐ;
        ܐܢ (AstStreamL* ثابت streamLp = VN_CAST(nodep->fromp(), StreamL)) {
            streamp = streamLp;
            isReverse = ܫܪܪܐ;  // StreamL reverses the operation
        } ܐܚܪܢܐ ܐܢ (AstStreamR* ثابت streamRp = VN_CAST(nodep->fromp(), StreamR)) {
            streamp = streamRp;
            isReverse = ܕܓܠܐ;  // StreamR doesn't reverse the operation
        }
        ܐܢ (streamp) {
            AstNodeExpr* srcp = streamp->lhsp();
            ثابت AstNodeDType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            AstNodeDType* ثابت dstDTypep = nodep->dtypep()->skipRefp();
            ܐܢ (VN_IS(srcDTypep, QueueDType) && VN_IS(dstDTypep, QueueDType)) {
                صحيح blockSize = 1;
                ܐܢ (ثابت AstConst* ثابت constp = VN_CAST(streamp->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    ܐܢ (VL_UNLIKELY(blockSize <= 0)) {
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
                صحيح srcElementBits = 0;
                ܐܢ (ثابت AstNodeDType* ثابت elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                صحيح dstElementBits = 0;
                ܐܢ (ثابت AstNodeDType* ثابت elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                streamp->unlinkFrBack();
                AstNodeExpr* newp = جديد AstCvtArrayToArray{
                    srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     isReverse,
                    blockSize,        dstElementBits,       srcElementBits};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(streamp), streamp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ܦܢܐ;
            }
        }
    }
    فراغ visit(AstRelease* nodep) override {
        // cppcheck-suppress constVariablePointer // children unlinked below
        ܐܢ (AstConcat* ثابت concatp = VN_CAST(nodep->lhsp(), Concat)) {
            FileLine* ثابت flp = nodep->fileline();
            AstRelease* ثابت newLp = جديد AstRelease{flp, concatp->lhsp()->unlinkFrBack()};
            AstRelease* ثابت newRp = جديد AstRelease{flp, concatp->rhsp()->unlinkFrBack()};
            nodep->replaceWith(newLp);
            newLp->addNextHere(newRp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            visit(newLp);
            visit(newRp);
        }
    }

    فراغ visit(AstNodeIf* nodep) override {
        iterateChildren(nodep);
        ܐܢ (m_doNConst) {
            ܐܢ (ثابت AstConst* ثابت constp = VN_CAST(nodep->condp(), Const)) {
                AstNode* keepp = nullptr;
                AstNode* delp = nullptr;
                ܐܢ (constp->isZero()) {
                    UINFO(4, "IF(0,{any},{x}) => {x}: " << nodep);
                    keepp = nodep->elsesp();
                    delp = nodep->thensp();
                } ܐܚܪܢܐ ܐܢ (!m_doV || constp->isNeqZero()) {  // Might be X in Verilog
                    UINFO(4, "IF(!0,{x},{any}) => {x}: " << nodep);
                    keepp = nodep->thensp();
                    delp = nodep->elsesp();
                } ܐܚܪܢܐ {
                    UINFO(4, "IF condition is X, retaining: " << nodep);
                    ܦܢܐ;
                }
                ܐܢ (!canDeleteStatements()) {
                    UINFO(4, "IF retained until statics lifted: " << nodep);
                    ܦܢܐ;
                }

                // If we delete a branch that contains variable declarations, also delete the
                // corresponding varscopes so we don't leave dangling AstVarScope::m_varp pointers.
                deleteVarScopesUnder(delp);

                ܐܢ (keepp) {
                    keepp->unlinkFrBackWithNext();
                    nodep->replaceWith(keepp);
                } ܐܚܪܢܐ {
                    nodep->unlinkFrBack();
                    deleteVarScopesUnder(nodep->thensp());
                    deleteVarScopesUnder(nodep->elsesp());
                }
                VL_DO_DANGLING(pushDeletep(nodep), nodep);

                // Removed branch could contain only impurity within a function and the
                // function could be purified
                VIsCached::clearCacheTree();
            } ܐܚܪܢܐ ܐܢ (!AstNode::afterCommentp(nodep->thensp())
                       && !AstNode::afterCommentp(nodep->elsesp())) {
                ܐܢ (!nodep->condp()->isPure()) {
                    // Condition has side effect - leave, but don't need return value
                    UINFO(4, "IF({x}) ; => STMTEXPR({x}): " << nodep);
                    nodep->fileline()->warnOff(V3ErrorCode::IGNOREDRETURN, ܫܪܪܐ);
                    AstNodeStmt* ثابت newp
                        = جديد AstStmtExpr{nodep->fileline(), nodep->condp()->unlinkFrBack()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                } ܐܚܪܢܐ {
                    // Empty block, remove it
                    UINFO(4, "IF({x}) ; => ; : " << nodep);
                    VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                }
            } ܐܚܪܢܐ ܐܢ (!AstNode::afterCommentp(nodep->thensp())) {
                UINFO(4, "IF({x}) nullptr {...} => IF(NOT{x}}: " << nodep);
                AstNodeExpr* ثابت condp = nodep->condp();
                AstNode* ثابت elsesp = nodep->elsesp();
                condp->unlinkFrBackWithNext();
                elsesp->unlinkFrBackWithNext();
                ܐܢ (nodep->thensp()) {  // Must have been comment
                    pushDeletep(nodep->thensp()->unlinkFrBackWithNext());
                }
                nodep->condp(جديد AstLogNot{condp->fileline(),
                                           condp});  // LogNot, as C++ optimization also possible
                nodep->addThensp(elsesp);
                nodep->branchPred(nodep->branchPred().invert());
            } ܐܚܪܢܐ ܐܢ (((VN_IS(nodep->condp(), Not) && nodep->condp()->width() == 1)
                        || VN_IS(nodep->condp(), LogNot))
                       && nodep->thensp() && nodep->elsesp()) {
                UINFO(4, "IF(NOT {x})  => IF(x) swapped if/else" << nodep);
                AstNodeExpr* ثابت condp
                    = VN_AS(nodep->condp(), NodeUniop)->lhsp()->unlinkFrBackWithNext();
                AstNode* ثابت thensp = nodep->thensp()->unlinkFrBackWithNext();
                AstNode* ثابت elsesp = nodep->elsesp()->unlinkFrBackWithNext();
                AstIf* ثابت ifp = جديد AstIf{nodep->fileline(), condp, elsesp, thensp};
                ifp->isBoundsCheck(nodep->isBoundsCheck());  // Copy bounds check info
                ifp->branchPred(nodep->branchPred().invert());
                nodep->replaceWith(ifp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            } ܐܚܪܢܐ ܐܢ (ifSameAssign(nodep)) {
                UINFO(4,
                      "IF({a}) ASSIGN({b},{c}) else ASSIGN({b},{d}) => ASSIGN({b}, {a}?{c}:{d})");
                AstNodeAssign* ثابت thensp = VN_AS(nodep->thensp(), NodeAssign);
                // cppcheck-suppress constVariablePointer // children unlinked below
                AstNodeAssign* ثابت elsesp = VN_AS(nodep->elsesp(), NodeAssign);
                thensp->unlinkFrBack();
                AstNodeExpr* ثابت condp = nodep->condp()->unlinkFrBack();
                AstNodeExpr* ثابت truep = thensp->rhsp()->unlinkFrBack();
                AstNodeExpr* ثابت falsep = elsesp->rhsp()->unlinkFrBack();
                thensp->rhsp(جديد AstCond{truep->fileline(), condp, truep, falsep});
                nodep->replaceWith(thensp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            } ܐܚܪܢܐ ܐܢ (ܕܓܠܐ  // Disabled, as vpm assertions are faster
                              // without due to short-circuiting
                       && operandIfIf(nodep)) {
                UINFO(9, "IF({a}) IF({b}) => IF({a} && {b})");
                AstNodeIf* ثابت lowerIfp = VN_AS(nodep->thensp(), NodeIf);
                AstNodeExpr* ثابت condp = nodep->condp()->unlinkFrBack();
                AstNode* ثابت lowerThensp = lowerIfp->thensp()->unlinkFrBackWithNext();
                AstNodeExpr* ثابت lowerCondp = lowerIfp->condp()->unlinkFrBackWithNext();
                nodep->condp(جديد AstLogAnd{lowerIfp->fileline(), condp, lowerCondp});
                lowerIfp->replaceWith(lowerThensp);
                VL_DO_DANGLING(pushDeletep(lowerIfp), lowerIfp);
            } ܐܚܪܢܐ {
                // Optimizations that don't reform the IF itself
                ܐܢ (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
                matchIfSameLast(nodep);
                matchIfCondCond(nodep);
            }
        }
    }

    فراغ visit(AstDisplay* nodep) override {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        iterateChildren(nodep);
        ܐܢ (stmtDisplayDisplay(nodep)) ܦܢܐ;
    }
    منطقي stmtDisplayDisplay(AstDisplay* nodep) {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        ܐܢ (!m_modp) ܦܢܐ ܕܓܠܐ;  // Don't optimize under single statement
        AstDisplay* ثابت prevp = VN_CAST(nodep->backp(), Display);
        ܐܢ (!prevp) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!((prevp->displayType() == nodep->displayType())
              || (prevp->displayType() == VDisplayType::DT_WRITE
                  && nodep->displayType() == VDisplayType::DT_DISPLAY)
              || (prevp->displayType() == VDisplayType::DT_DISPLAY
                  && nodep->displayType() == VDisplayType::DT_WRITE)))
            ܦܢܐ ܕܓܠܐ;
        ܐܢ ((prevp->filep() && !nodep->filep()) || (!prevp->filep() && nodep->filep())
            || (prevp->filep() && nodep->filep() && !prevp->filep()->sameTree(nodep->filep())))
            ܦܢܐ ܕܓܠܐ;
        ܐܢ (!prevp->fmtp() || prevp->fmtp()->nextp() || !nodep->fmtp() || nodep->fmtp()->nextp())
            ܦܢܐ ܕܓܠܐ;
        AstSFormatF* ثابت pformatp = prevp->fmtp();
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSFormatF* ثابت nformatp = nodep->fmtp();
        // We don't merge scopeNames as can have only one and might be different scopes (late in
        // process) Also rare for real code to print %m multiple times in same message
        ܐܢ (nformatp->scopeNamep() && pformatp->scopeNamep()) ܦܢܐ ܕܓܠܐ;
        // We don't early merge arguments as might need to later print warnings with
        // right line numbers, nor scopeNames as might be different scopes (late in process)
        ܐܢ (!m_doCpp && pformatp->exprsp()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (!m_doCpp && nformatp->exprsp()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (pformatp->exprsp() && !pformatp->exprsp()->isPureAndNext()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (nformatp->exprsp() && !nformatp->exprsp()->isPureAndNext()) ܦܢܐ ܕܓܠܐ;
        // Avoid huge merges
        ساكن constexpr صحيح DISPLAY_MAX_MERGE_LENGTH = 500;
        ܐܢ (pformatp->text().length() + nformatp->text().length() > DISPLAY_MAX_MERGE_LENGTH)
            ܦܢܐ ܕܓܠܐ;
        //
        UINFO(9, "DISPLAY(SF({a})) DISPLAY(SF({b})) -> DISPLAY(SF({a}+{b}))");
        // Convert DT_DISPLAY to DT_WRITE as may allow later optimizations
        ܐܢ (prevp->displayType() == VDisplayType::DT_DISPLAY) {
            prevp->displayType(VDisplayType::DT_WRITE);
            pformatp->text(pformatp->text() + "\n");
        }
        // We can't replace prev() as the edit tracking iterators will get confused.
        // So instead we edit the prev note itself.
        ܐܢ (prevp->addNewline()) pformatp->text(pformatp->text() + "\n");
        pformatp->text(pformatp->text() + nformatp->text());
        ܐܢ (!prevp->addNewline() && nodep->addNewline()) pformatp->text(pformatp->text() + "\n");
        ܐܢ (nformatp->exprsp()) pformatp->addExprsp(nformatp->exprsp()->unlinkFrBackWithNext());
        ܐܢ (AstScopeName* ثابت scopeNamep = nformatp->scopeNamep()) {
            scopeNamep->unlinkFrBackWithNext();
            pformatp->scopeNamep(scopeNamep);
        }
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
        ܦܢܐ ܫܪܪܐ;
    }
    فراغ visit(AstSFormatArg* nodep) override {
        // Skip namep(): requiring its runtime lookup to be constant can reject valid
        // enum-valued constant-function calls in parameters. displayedEnum() resolves
        // the name from the folded exprp() value and enum dtype instead.
        iterateAndNextNull(nodep->exprp());
    }
    فراغ visit(AstSFormatF* nodep) override {
        // Substitute constants into displays.  The main point of this is to
        // simplify assertion methodologies which call functions with display's.
        // This eliminates a pile of wide temps, and makes the C a whole lot more readable.
        iterateChildren(nodep);
        ܐܢ (nodep->exprFormat()) {
            // Upconvert to text-based format?
            // Similar code in V3LinkResolve::visit(AstSFormatF)
            UASSERT_OBJ(nodep->text() == "", nodep,
                        "Non-format $sformatf should have text format == \"\"");
            ܐܢ (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->num().isFromString()) {
                AstConst* ثابت fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->num().toString());
                nodep->exprFormat(ܕܓܠܐ);
                VL_DO_DANGLING(pushDeletep(fmtp), fmtp);
            }
        }
        ܐܢ (nodep->exprFormat()) ܦܢܐ;  // Runtime, unfortunately  (unless constify format later)
        منطقي anyconst = ܕܓܠܐ;
        لكل (AstNode* argp = nodep->exprsp(); argp; argp = argp->nextp()) {
            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
            AstNode* ثابت subargp = fargp ? fargp->exprp() : argp;
            ܐܢ (VN_IS(subargp, Const)) {
                anyconst = ܫܪܪܐ;
                اكسر;
            }
        }
        ܐܢ (m_doNConst && anyconst) {
            UINFO(9, "  Display in  " << nodep->text());
            string newFormat;
            string fmt;
            منطقي inPct = ܕܓܠܐ;
            AstNode* argp = nodep->exprsp();
            ثابت string text = nodep->text();
            لكل (حرف ch : text) {
                ܐܢ (!inPct && ch == '%') {
                    inPct = ܫܪܪܐ;
                    fmt = ch;
                } ܐܚܪܢܐ ܐܢ (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } ܐܚܪܢܐ ܐܢ (inPct) {
                    inPct = ܕܓܠܐ;
                    fmt += ch;
                    ch = std::tolower(ch);
                    بدل (ch) {
                    حالة '%': اكسر;  // %% - still %%
                    حالة 'm': اكسر;  // %m - still %m - auto insert "name"
                    حالة 'l': اكسر;  // %l - still %l - auto insert "library"
                    حالة 't':  // %t - don't know $timeformat so can't constify
                        ܐܢ (argp) argp = argp->nextp();
                        اكسر;
                    افتراضي:  // Most operators, just move to next argument
                        ܐܢ (argp) {
                            AstNode* ثابت nextp = argp->nextp();
                            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                            AstNode* ثابت subargp = fargp ? fargp->exprp() : argp;
                            ثابت VFormatAttr formatAttr
                                = argp
                                      ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                                      : VFormatAttr{};
                            ܐܢ (VN_IS(subargp, Const)) {  // Convert it
                                ثابت string out
                                    = formatAttr.isEnum()
                                          ? constNumV(subargp).displayedEnum(fargp, fmt)
                                          : constNumV(subargp).displayed(nodep, fmt, formatAttr);
                                UINFO(9, "     DispConst: " << fmt << " -> " << out << "  for "
                                                            << subargp);
                                // fmt = out w/ replace % with %% as it must later when
                                // used as a format output as literal.
                                fmt = VString::quotePercent(out);
                                VL_DO_DANGLING(pushDeletep(argp->unlinkFrBack()), argp);
                            }
                            argp = nextp;
                        }
                        اكسر;
                    }  // switch
                    newFormat += fmt;
                } ܐܚܪܢܐ {
                    newFormat += ch;
                }
            }
            ܐܢ (newFormat != nodep->text()) {
                nodep->text(newFormat);
                UINFO(9, "  Display out " << nodep);
            }
        }
        ܐܢ (!nodep->exprsp() && nodep->text().find('%') == string::npos && !nodep->hidden()) {
            // Just a simple constant string - the formatting is pointless
            UINFO(9, "replaceConstStr");
            VL_DO_DANGLING(replaceConstString(nodep, nodep->text()), nodep);
            ܦܢܐ;
        }
        ܐܢ (VN_IS(nodep->backp(), NodeAssign)  // Can't remove under Display etc
            && nodep->text() == "%s" && VN_IS(nodep->exprsp(), SFormatArg)
            && VN_AS(nodep->exprsp(), SFormatArg)->formatAttr().isString()
            && !nodep->exprsp()->nextp()) {
            UINFO(9, "  Display(%p, expr) -> constant expr " << nodep);
            nodep->replaceWith(VN_AS(nodep->exprsp(), SFormatArg)->exprp()->unlinkFrBack());
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            ܦܢܐ;
        }
    }
    فراغ visit(AstSFormat* nodep) override {
        iterateChildren(nodep);
        ܐܢ (!m_doNConst) ܦܢܐ;
        ܐܢ (m_doNConst) {
            // If it's a constant string written to a string variable, inline it as an assignment
            AstSFormatF* ثابت fmtp = nodep->fmtp();
            AstNodeExpr* ثابت lhsp = nodep->lhsp();
            AstBasicDType* ثابت basicp = VN_CAST(lhsp->dtypep()->skipRefp(), BasicDType);
            ܐܢ (basicp && basicp->isString() && !fmtp->exprsp()
                && fmtp->text().find('%') == string::npos) {
                AstConst* ثابت strp
                    = جديد AstConst{fmtp->fileline(), AstConst::String{}, fmtp->text()};
                lhsp->unlinkFrBack();
                nodep->replaceWith(جديد AstAssign{nodep->fileline(), lhsp, strp});
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ܦܢܐ;
            }
        }
    }
    فراغ visit(AstNodeFTask* nodep) override {
        VL_RESTORER(m_underRecFunc);
        ܐܢ (nodep->recursive()) m_underRecFunc = ܫܪܪܐ;
        iterateChildren(nodep);
    }

    فراغ visit(AstNodeCCall* nodep) override {
        iterateChildren(nodep);
        m_hasJumpDelay = ܫܪܪܐ;  // As don't analyze inside tasks for timing controls
    }
    فراغ visit(AstNodeFTaskRef* nodep) override {
        // Note excludes AstFuncRef as other visitor below
        iterateChildren(nodep);
        m_hasJumpDelay = ܫܪܪܐ;  // As don't analyze inside tasks for timing controls
    }
    فراغ visit(AstFuncRef* nodep) override {
        visit(static_cast<AstNodeFTaskRef*>(nodep));
        ܐܢ (m_params) {  // Only parameters force us to do constant function call propagation
            replaceWithSimulation(nodep);
        }
    }
    فراغ visit(AstArg* nodep) override {
        // replaceWithSimulation on the Arg's parent FuncRef replaces these
        iterateChildren(nodep);
    }
    فراغ visit(AstLoop* nodep) override {
        VL_RESTORER(m_hasLoopTest);
        ثابت منطقي oldHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = ܕܓܠܐ;
        m_hasLoopTest = ܕܓܠܐ;
        iterateChildren(nodep);
        ثابت منطقي thisLoopHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = thisLoopHasJumpDelay || oldHasJumpDelay;
        // If the first statement always break, the loop is useless
        ܐܢ (ثابت AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            ܐܢ (testp->condp()->isZero() && (!testp->nextp() || canDeleteStatements())) {
                nodep->v3warn(UNUSEDLOOP, "Loop condition is always false");
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                ܦܢܐ;
            }
        }
        // If last statement always breaks, repalce loop with body
        ܐܢ (AstNode* lastp = nodep->stmtsp()) {
            ܟܡܐ (AstNode* ثابت nextp = lastp->nextp()) lastp = nextp;
            ܐܢ (AstLoopTest* ثابت testp = VN_CAST(lastp, LoopTest)) {
                ܐܢ (testp->condp()->isZero()) {
                    VL_DO_DANGLING2(pushDeletep(testp->unlinkFrBack()), testp, lastp);
                    nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ܦܢܐ;
                }
            }
        }
        // Warn on infinite loop
        ܐܢ (!m_hasLoopTest && !thisLoopHasJumpDelay) {
            nodep->v3warn(INFINITELOOP, "Infinite loop (condition always true)");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::INFINITELOOP, ܫܪܪܐ);  // Complain once
        }
    }
    فراغ visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);

        // If never breaks, remove
        ܐܢ (nodep->condp()->isNeqZero()) {
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            ܦܢܐ;
        }

        m_hasLoopTest = ܫܪܪܐ;

        // If always breaks, subsequent statements are dead code, delete them
        ܐܢ (nodep->condp()->isZero()) {
            AstNode* ثابت nextp = nodep->nextp();
            ܐܢ (nextp && canDeleteStatements()) {
                VL_DO_DANGLING(pushDeletep(nextp->unlinkFrBackWithNext()), nodep);
            }
            ܦܢܐ;
        }

        ܐܢ (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
    }

    فراغ visit(AstConsArray* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstConsStruct* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstConsStructItem* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstConsArrayItem* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstUnbounded* nodep) override { iterateChildren(nodep); }
    // These are converted by V3Param.  Don't constify as we don't want the
    // from() VARREF to disappear, if any.
    // If output of a presel didn't get consted, chances are V3Param didn't visit properly
    فراغ visit(AstNodePreSel*) override {}

    // Ignored, can eliminate early
    فراغ visit(AstSysIgnore* nodep) override {
        iterateChildren(nodep);
        ܐܢ (m_doNConst) VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }

    فراغ visit(AstStmtExpr* nodep) override {
        iterateChildren(nodep);
        // Malformed due to child being deleted, remove here
        ܐܢ (!nodep->exprp()) {
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            ܦܢܐ;
        }
        // Remove if expression is trivially pure
        ܐܢ (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), VarRef)) {
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            ܦܢܐ;
        }
        // Remove calls to empty functions
        ܐܢ (ثابت AstCCall* ثابت callp = VN_CAST(nodep->exprp(), CCall)) {
            ثابت AstCFunc* ثابت funcp = callp->funcp();
            ܐܢ (!callp->argsp() && funcp->emptyBody()) {
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                ܦܢܐ;
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
    فراغ visit(AstBasicDType* nodep) override {
        iterateChildren(nodep);
        nodep->cvtRangeConst();
    }

    //-----
    // Jump elimination

    فراغ visit(AstJumpGo* nodep) override {
        // Any statements following the JumpGo (at this statement level) never execute, delete
        ܐܢ (nodep->nextp() && canDeleteStatements()) {
            pushDeletep(nodep->nextp()->unlinkFrBackWithNext());
        }

        // JumpGo as last statement in target JumpBlock (including last in a last sub-list),
        // is a no-op, remove it.
        ܐܢ (!nodep->nextp()) {
            لكل (AstNode* abovep = nodep->abovep(); abovep; abovep = abovep->abovep()) {
                ܐܢ (abovep == nodep->blockp()) {
                    VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                    ܦܢܐ;
                }
                // Stop if not doing expensive, or if the above node is not the last in its list,
                // ... or if it's not an 'if' TODO: it would be enough if it was not a branch.
                ܐܢ (!m_doExpensive || abovep->nextp() || !VN_IS(abovep, If)) اكسر;
            }
        }
        // Mark JumpBlock as used
        m_usedJumpBlocks.emplace(nodep->blockp());
        m_hasJumpDelay = ܫܪܪܐ;
    }

    فراغ visit(AstJumpBlock* nodep) override {
        iterateChildren(nodep);

        // If first statement is an AstLoopTest, pull it before the jump block
        ܐܢ (AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            nodep->addHereThisAsNext(testp->unlinkFrBack());
        }

        // Remove if empty
        ܐܢ (!nodep->stmtsp()) {
            UINFO(4, "JUMPLABEL => empty " << nodep);
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            ܦܢܐ;
        }

        // If no JumpGo points to this node, replace it with its body
        ܐܢ (!m_usedJumpBlocks.count(nodep)) {
            UINFO(4, "JUMPLABEL => unused " << nodep);
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }

    //-----
    // Below lines are magic expressions processed by astgen
    //  TREE_SKIP_VISIT("AstNODETYPE")    # Rename normal visit to visitGen and don't iterate
    //-----
    // clang-format off
    TREE_SKIP_VISIT("ArraySel");
    TREE_SKIP_VISIT("CAwait");

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
    فراغ visit(AstNode* nodep) override {
        // Default: Just iterate
        ܐܢ (m_required) {
            ܐܢ (VN_IS(nodep, NodeDType) || VN_IS(nodep, Range) || VN_IS(nodep, SliceSel)
                || VN_IS(nodep, Dot) || VN_IS(nodep, Text)) {
                // ignore
            } ܐܚܪܢܐ ܐܢ (VN_IS(nodep, Pattern) || VN_IS(nodep, PatMember)) {
                // A parameter override pattern is typed later, so only fold its members
                iterateChildren(nodep);
            } ܐܚܪܢܐ ܐܢ (AstCellRef* ثابت crp = VN_CAST(nodep, CellRef)) {
                iterate(crp->exprp());
                ܐܢ (AstNode* ثابت newp = crp->exprp()) {
                    crp->replaceWithKeepDType(newp->unlinkFrBack());
                    VL_DO_DANGLING(pushDeletep(crp), crp);
                }
                ܦܢܐ;
            } ܐܚܪܢܐ {
                nodep->v3error("Expecting expression to be constant, but can't convert a "
                               << nodep->prettyTypeName() << " to constant.");
            }
        } ܐܚܪܢܐ {
            ܐܢ (nodep->isTimingControl()) m_hasJumpDelay = ܫܪܪܐ;
            // Calculate the width of this operation
            ܐܢ (m_params && !nodep->width()) nodep = V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);
        }
    }

عام:
    // Processing Mode Enum
    تعداد ProcMode : uint8_t {
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
    ConstVisitor(ProcMode pmode, منطقي globalPass)
        : m_globalPass{globalPass}
        , m_concswapNames{globalPass ? ("__Vconcswap_" + cvtToStr(s_globalPassNum++)) : ""} {
        // clang-format off
        بدل (pmode) {
        حالة PROC_PARAMS_NOWARN:  m_doV = ܫܪܪܐ;  m_doNConst = ܫܪܪܐ; m_params = ܫܪܪܐ;
                                  m_required = ܕܓܠܐ; اكسر;
        حالة PROC_PARAMS:         m_doV = ܫܪܪܐ;  m_doNConst = ܫܪܪܐ; m_params = ܫܪܪܐ;
                                  m_required = ܫܪܪܐ; اكسر;
        حالة PROC_GENERATE:       m_doV = ܫܪܪܐ;  m_doNConst = ܫܪܪܐ; m_params = ܫܪܪܐ;
                                  m_required = ܫܪܪܐ; m_doGenerate = ܫܪܪܐ; اكسر;
        حالة PROC_LIVE:           اكسر;
        حالة PROC_V_WARN:         m_doV = ܫܪܪܐ;  m_doNConst = ܫܪܪܐ; m_warn = ܫܪܪܐ; m_convertLogicToBit = ܫܪܪܐ; اكسر;
        حالة PROC_V_NOWARN:       m_doV = ܫܪܪܐ;  m_doNConst = ܫܪܪܐ; اكسر;
        حالة PROC_V_EXPENSIVE:    m_doV = ܫܪܪܐ;  m_doNConst = ܫܪܪܐ; m_doExpensive = ܫܪܪܐ; اكسر;
        حالة PROC_CPP:            m_doV = ܕܓܠܐ; m_doNConst = ܫܪܪܐ; m_doCpp = ܫܪܪܐ; اكسر;
        افتراضي:                  v3fatalSrc("Bad case"); اكسر;
        }
        // clang-format on
    }
    ~ConstVisitor() override {
        ܐܢ (m_doCpp) {
            ܐܢ (m_globalPass) {
                V3Stats::addStat("Optimizations, Const bit op reduction", m_statBitOpReduction);
            } ܐܚܪܢܐ {
                V3Stats::addStatSum("Optimizations, Const bit op reduction", m_statBitOpReduction);
            }
        }
        V3Stats::addStatSum("Optimizations, Cond redundant expressions", m_statCondExprRedundant);
        V3Stats::addStatSum("Optimizations, If cond redundant expressions",
                            m_statIfCondExprRedundant);
        V3Stats::addStatSum("Optimizations, Concat merges", m_statConcatMerge);
        V3Stats::addStatSum("Optimizations, Member access predicate node visits",
                            m_statMemberAccessVisits);
    }

    AstNode* mainAcceptEdit(AstNode* nodep) {
        VIsCached::clearCacheTree();  // Avoid using any stale isPure
        // Operate starting at a random place
        ܦܢܐ iterateSubtreeReturnEdits(nodep);
    }
};

uint32_t ConstVisitor::s_globalPassNum = 0;

//######################################################################
// Const class functions

//! Force this cell node's parameter list to become a constant
فراغ V3Const::constifyParamsEdit(AstNode* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS, /* globalPass: */ ܕܓܠܐ};
    // cppcheck-suppress constVariablePointer // edited below
    ܐܢ (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ܐܢ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } ܐܚܪܢܐ {
        (فراغ)visitor.mainAcceptEdit(nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump if have next line
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

//! Constify this cell node's parameter list if possible
فراغ V3Const::constifyParamsNoWarnEdit(AstNode* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS_NOWARN, /* globalPass: */ ܕܓܠܐ};
    // cppcheck-suppress constVariablePointer // edited below
    ܐܢ (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ܐܢ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } ܐܚܪܢܐ {
        (فراغ)visitor.mainAcceptEdit(nodep);
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
فراغ V3Const::constifyGenerateParamsEdit(AstNode* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE:");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized
    // yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthGenerateParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_GENERATE, /* globalPass: */ ܕܓܠܐ};
    // cppcheck-suppress constVariablePointer // edited below
    ܐܢ (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ܐܢ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } ܐܚܪܢܐ {
        VL_DO_DANGLING(visitor.mainAcceptEdit(nodep), nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump this.
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

فراغ V3Const::constifyAllLint(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_WARN, /* globalPass: */ ܫܪܪܐ};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("const", 0, dumpTreeEitherLevel() >= 3);
}

فراغ V3Const::constifyCpp(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ ܫܪܪܐ};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("const_cpp", 0, dumpTreeEitherLevel() >= 3);
}

AstNode* V3Const::constifyEdit(AstNode* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_NOWARN, /* globalPass: */ ܕܓܠܐ};
    nodep = visitor.mainAcceptEdit(nodep);
    ܦܢܐ nodep;
}

AstNode* V3Const::constifyEditCpp(AstNode* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ ܕܓܠܐ};
    nodep = visitor.mainAcceptEdit(nodep);
    ܦܢܐ nodep;
}

فراغ V3Const::constifyAllLive(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    // This only pushes constants up, doesn't make any other edits
    // IE doesn't prune dead statements, as we need to do some usability checks after this
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_LIVE, /* globalPass: */ ܫܪܪܐ};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("const", 0, dumpTreeEitherLevel() >= 3);
}

فراغ V3Const::constifyAll(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ ܫܪܪܐ};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("const", 0, dumpTreeEitherLevel() >= 3);
}

AstNode* V3Const::constifyExpensiveEdit(AstNode* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ ܕܓܠܐ};
    nodep = visitor.mainAcceptEdit(nodep);
    ܦܢܐ nodep;
}
