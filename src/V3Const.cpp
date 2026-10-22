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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Const.h"

#شامل "V3Ast.h"
#شامل "V3Global.h"
#شامل "V3Simulate.h"
#شامل "V3Stats.h"
#شامل "V3String.h"
#شامل "V3UniqueNames.h"
#شامل "V3Width.h"

#شامل <algorithm>
#شامل <memory>
#شامل <type_traits>
#شامل <unordered_map>
#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

#define درخت_SKIP_VISIT(...)
#define TREEOP1(...)
#define TREEOPA(...)
#define TREEOP(...)
#define TREEOPS(...)
#define TREEOPC(...)
#define TREEOPV(...)

//######################################################################
// Utilities

ایستا منطقی isConst(ثابت Ast_گره* nodep, uint64_t v) {
    ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
    بازگشت constp && constp->toUQuad() == v;
}

template <typename T>
ایستا typename std::enable_if<std::is_integral<T>::مقدار, منطقی>::type isPow2(T val) {
    بازگشت (val & (val - 1)) == 0;
}

ایستا صحیح شمار_Trailing_Zeroes(uint64_t val) {
    UASSERT(val, "countTrailingZeroes argument must be non-zero");
#اگر defined(__GNUC__) && !defined(VL_NO_BUILTINS)
    بازگشت __builtin_ctzll(val);
#وگرنه
    صحیح bit = 0;
    val = ~val;
    وقتی (val & 1) {
        ++bit;
        val >>= 1;
    }
    بازگشت bit;
#endif
}

// Allocates a unique id for each AstNode it is applied to, held in user4.
رده VNIdAllocator final {
    // NODE STATE
    // AstNode::user4p -> size_t. Unique id of the node (0/nullptr if not allocated yet)

    // MEMBERS
    // Claimed lazily on first use TODO: fix conflict with V3Param user4 slot
    std::unique_ptr<VNUser4InUse> m_inuser4p;
    size_t m_nextId = 0;  // Id allocated most recently

عمومی:
    // Return the unique id of the given node, allocating a new one if it has none yet
    size_t operator()(Ast_گره* nodep) {
        اگر (!m_inuser4p) m_inuser4p = std::make_unique<VNUser4InUse>();  // Claim on first use
        اگر (!nodep->user4p()) nodep->user4p(reinterpret_cast<تهی*>(++m_nextId));
        بازگشت reinterpret_cast<size_t>(nodep->user4p());
    }
};

// This visitor can be used in the post-expanded Ast from V3Expand, where the Ast satisfies:
// - Constants are 64 bit at most (because words are accessed via AstWordSel)
// - Variables are scoped.
رده Const_Bit_Op_درخت_Visitor final : عمومی VNVisitorConst {
    // TYPES

    // Holds a node to be added as a term in the reduction tree, it's equivalent op count, and a
    // bool indicating if the term is clean (0/1 value, or if the top bits might be dirty)
    using ResultTerm = std::tuple<Ast_گره_عبارت*, unsigned, منطقی>;

    رده LeafInfo final {  // Leaf node (either AstConst or AstVarRef)
        // MEMBERS
        منطقی m_polarity = درست;  // Invert result due to NOT
        صحیح m_lsb = 0;  // LSB of actually used bit of m_refp->varp()
        صحیح m_msb = 0;  // MSB of actually used bit of m_refp->varp()
        صحیح m_wordIdx = -1;  // -1 means AstWordSel is not used.
        AstVarRef* m_refp = nullptr;  // Leaf's variable reference
        ثابت AstConst* m_constp = nullptr;  // Leaf's constant

    عمومی:
        // CONSTRUCTORS
        LeafInfo() = پیشفرض;
        LeafInfo(ثابت LeafInfo& other) = پیشفرض;
        LeafInfo& operator=(ثابت LeafInfo& other) = پیشفرض;
        LeafInfo(LeafInfo&& other) = پیشفرض;
        LeafInfo& operator=(LeafInfo&& other) = پیشفرض;
        explicit LeafInfo(صحیح lsb)
            : m_lsb{lsb} {}

        // METHODS
        تهی setLeaf(AstVarRef* refp) {
            UASSERT_OBJ(!m_refp && !m_constp, refp, "Must be called just once");
            m_refp = refp;
            m_msb = refp->varp()->widthMin() - 1;
        }
        تهی setLeaf(ثابت AstConst* constp) {
            UASSERT_OBJ(!m_refp && !m_constp, constp, "Must be called just once");
            m_constp = constp;
            m_msb = constp->widthMin() - 1;
        }
        // updateBitRange(), limitBitRangeToLsb(), and polarity() must be called during ascending
        // back to the root.
        تهی updateBitRange(صحیح newLsb, صحیح newMsb) {
            اگر ((m_lsb <= m_msb && newLsb > newMsb) || (m_lsb > m_msb && m_lsb < newLsb)) {
                // When the new bit range is out of m_refp, clear polarity because nodes below is
                // shifted out to zero.
                // This kind of clear may happen several times. e.g. (!(1'b1 >> 1)) >> 1
                polarity(درست);
            }
            m_lsb = newLsb;
            m_msb = newMsb;
        }
        تهی updateBitRange(ثابت AstCCast* castp) {
            updateBitRange(m_lsb, std::min(m_msb, m_lsb + castp->width() - 1));
        }
        تهی updateBitRange(ثابت AstShiftR* shiftp) {
            updateBitRange(m_lsb + VN_AS(shiftp->rhsp(), Const)->toUInt(), m_msb);
        }
        تهی limitBitRangeToLsb() { updateBitRange(m_lsb, std::min(m_msb, m_lsb)); }
        صحیح wordIdx() ثابت { بازگشت m_wordIdx; }
        تهی wordIdx(صحیح i) { m_wordIdx = i; }
        منطقی polarity() ثابت { بازگشت m_polarity; }
        تهی polarity(منطقی p) { m_polarity = p; }

        AstVarRef* refp() ثابت { بازگشت m_refp; }
        ثابت AstConst* constp() ثابت { بازگشت m_constp; }
        منطقی missingWordSel() ثابت {
            // When V3Expand is skipped, WordSel is not inserted.
            بازگشت m_refp->isWide() && m_wordIdx == -1;
        }
        صحیح lsb() ثابت { بازگشت m_lsb; }

        صحیح msb() ثابت { بازگشت std::min(m_msb, varWidth() - 1); }
        صحیح varWidth() ثابت {
            UASSERT(m_refp, "m_refp should be set");
            ثابت صحیح width = m_refp->varp()->widthMin();
            اگر (!m_refp->isWide()) {
                UASSERT_OBJ(m_wordIdx == -1, m_refp, "Bad word index into non-wide");
                بازگشت width;
            } وگرنه {
                اگر (missingWordSel()) بازگشت width;
                UASSERT_OBJ(m_wordIdx >= 0, m_refp, "Bad word index into wide");
                ثابت صحیح bitsInMSW = VL_BITBIT_E(width) ? VL_BITBIT_E(width) : VL_EDATASIZE;
                بازگشت m_wordIdx == m_refp->width_واژگان() - 1 ? bitsInMSW : VL_EDATASIZE;
            }
        }
    };

    ساختار BitPolarityEntry final {  // Found bit polarity during iterate()
        LeafInfo m_info;  // Leaf (variable or constant) bit polarity was found on
        منطقی m_polarity = نادرست;  // Polarity the bit must have to match
        صحیح m_bit = 0;  // Bit index within the leaf that was tested
        BitPolarityEntry(ثابت LeafInfo& info, منطقی pol, صحیح bit)
            : m_info{info}
            , m_polarity{pol}
            , m_bit{bit} {}
        BitPolarityEntry() = پیشفرض;
    };

    ساختار Frozen_گره_Info final {  // Context when a frozen node is found
        منطقی m_polarity;  // Polarity the frozen node must match
        صحیح m_lsb;  // LSB position of the frozen node
        منطقی operator<(ثابت Frozen_گره_Info& other) ثابت {
            اگر (m_lsb != other.m_lsb) بازگشت m_lsb < other.m_lsb;
            بازگشت m_polarity < other.m_polarity;
        }
    };

    رده Restorer final {  // Restore the original state unless disableRestore() is called
        Const_Bit_Op_درخت_Visitor& m_visitor;  // Visitor whose state is saved and restored
        ثابت size_t m_polaritiesSize;  // Saved m_visitor.m_bitPolarities size to truncate back to
        ثابت size_t m_frozenSize;  // Saved m_visitor.m_frozenNodes size to truncate back to
        ثابت unsigned m_ops;  // Saved m_visitor.m_ops to restore
        ثابت منطقی m_polarity;  // Saved m_visitor.m_polarity to restore
        منطقی m_restore = درست;  // Whether the destructor still needs to restore state

    عمومی:
        explicit Restorer(Const_Bit_Op_درخت_Visitor& visitor)
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
            اگر (m_restore) restoreNow();
        }
        تهی disableRestore() { m_restore = نادرست; }
        تهی restoreNow() {
            UASSERT(m_restore, "Can be called just once");
            m_visitor.m_bitPolarities.resize(m_polaritiesSize);
            m_visitor.m_frozenNodes.resize(m_frozenSize);
            m_visitor.m_ops = m_ops;
            m_visitor.m_polarity = m_polarity;
            m_restore = نادرست;
        }
    };
    // Collect information for each Variable to transform as below
    رده VarInfo final {
        // MEMBERS
        صحیح m_knownResult = -1;  // -1: result is not known, 0 or 1: result of this tree
        ثابت Const_Bit_Op_درخت_Visitor* ثابت
            m_parentp;  // ConstBitOpTreeVisitor holding this VarInfo
        AstVarRef* ثابت m_refp;  // Points the variable that this VarInfo covers
        ثابت صحیح m_width;  // Width of term this VarInfo refers to
        V3_عدد m_bitPolarity;  // Coefficient of each bit

    عمومی:
        // METHODS
        منطقی hasConstResult() ثابت { بازگشت m_knownResult >= 0 || m_bitPolarity.isAllX(); }
        // The constant result. Only valid if hasConstResult() returned true.
        منطقی getConstResult() ثابت {
            // Note that this condition covers m_knownResult == -1 but m_bitPolarity.isAllX(),
            // in which case the result is 0
            بازگشت m_knownResult == 1;
        }
        ثابت AstVarRef* refp() ثابت { بازگشت m_refp; }
        منطقی sameVarAs(ثابت Ast_گره_Var_Ref* otherp) ثابت { بازگشت m_refp->same_گره(otherp); }
        تهی setPolarity(منطقی compBit, صحیح bit) {
            // Ignore if already determined a known reduction
            اگر (m_knownResult >= 0) بازگشت;
            UASSERT_OBJ(bit < m_width, m_refp,
                        "Bit index out of range: " << bit << " width: " << m_width);
            اگر (m_bitPolarity.bitIsX(bit)) {  // The bit is not yet marked with either polarity
                m_bitPolarity.setBit(bit, compBit);
            } وگرنه {  // The bit has already been marked with some polarity
                ثابت منطقی sameFlag = m_bitPolarity.bitIs1(bit) == compBit;
                اگر (m_parentp->is_Xor_درخت()) {
                    UASSERT_OBJ(compBit && sameFlag, m_refp, "Only true is set in Xor tree");
                    // a ^ a ^ b == b so we can ignore a
                    m_bitPolarity.setBit(bit, 'x');
                } وگرنه {  // And, Or
                    // Can ignore this nodep as the bit is already marked with the same polarity
                    اگر (sameFlag) بازگشت;  // a & a == a, b | b == b
                    // Otherwise result is constant (a & ~a == 0) or (a | ~a == 1)
                    m_knownResult = m_parentp->is_And_درخت() ? 0 : 1;
                    m_bitPolarity.setAllBitsX();  // The variable is not referred anymore
                }
            }
        }

        // Return reduction term for this VarInfo, together with the number of ops in the term,
        // and a boolean indicating if the term is clean (1-bit vs multi-bit value)
        ResultTerm getResultTerm() ثابت {
            UASSERT_OBJ(!hasConstResult(), m_refp, "getTerm on reduction that yields constant");
            File_سطر* ثابت fl = m_refp->fileline();

            // Get the term we are referencing (the WordSel, if wide, otherwise just the VarRef)
            Ast_گره_عبارت* srcp = VN_CAST(m_refp->backp(), WordSel);
            اگر (!srcp) srcp = m_refp;
            srcp = srcp->clone_درخت(نادرست);

            // Signed variables might have redundant sign bits that need masking.
            ثابت منطقی hasRedundantSignBits
                = m_refp->varp()->dtypep()->isSigned()
                  && (m_refp->isWide() ? (m_width != VL_EDATASIZE)
                                       : (m_width < 8 || !isPow2(m_width)));

            // Get the mask that selects the bits that are relevant in this term
            V3_عدد mask_رقم{srcp, m_width, 0};
            mask_رقم.opBitsNonXZ(m_bitPolarity);  // 'x' -> 0, 0->1, 1->1
            ثابت uint64_t maskVal = mask_رقم.toUQuad();
            UASSERT_OBJ(maskVal != 0, m_refp,
                        "Should have been recognized as having const 0 result");

            // Parts of the return value
            Ast_گره_عبارت* resultp = srcp;  // The tree for this term
            unsigned ops = 0;  // Number of ops in this term
            منطقی clean = نادرست;  // Whether the term is clean (has value 0 or 1)

            اگر (isPow2(maskVal)) {
                // If we only want a single bit, shift it out instead of a masked compare. Shifts
                // don't go through the flags register on x86 and are hence faster. This is also
                // always fewer or same ops as mask and compare, but with shorter instructions on
                // x86.

                // Find the index of the bit we want.
                ثابت صحیح bit = شمار_Trailing_Zeroes(maskVal);
                // If we want something other than the bottom bit, shift it out
                اگر (bit != 0) {
                    resultp = جدید AstShiftR{fl, resultp,
                                            جدید AstConst{fl, static_cast<uint32_t>(bit)}, m_width};
                    ++ops;
                }
                // Negate it if necessary
                ثابت منطقی negate = m_bitPolarity.bitIs0(bit);
                اگر (negate) {
                    resultp = جدید AstNot{fl, resultp};
                    ++ops;
                }
                // Clean if MSB of unsigned value, and not negated
                clean = (bit == m_width - 1) && !hasRedundantSignBits && !negate;
            } وگرنه {
                // We want multiple bits. Go ahead and extract them.

                // Check if masking is required, and if so apply it
                ثابت منطقی needsMasking = maskVal != VL_MASK_Q(m_width) || hasRedundantSignBits;
                اگر (needsMasking) {
                    resultp = جدید AstAnd{fl, جدید AstConst{fl, mask_رقم}, resultp};
                    ++ops;
                }

                // Create the sub-expression for this term
                اگر (m_parentp->is_Xor_درخت()) {
                    اگر (needsMasking) {
                        // Reduce the masked term to the minimum known width,
                        // to use the smallest RedXor formula
                        ثابت صحیح widthMin = mask_رقم.widthToFit();
                        resultp->dtypeChgWidth(widthMin, widthMin);
                    }
                    resultp = جدید AstRedXor{fl, resultp};
                    ++ops;
                    clean = نادرست;
                    // VL_REDXOR_* returns IData, set width accordingly to avoid unnecessary casts
                    resultp->dtypeChgWidth(VL_IDATASIZE, 1);
                } وگرنه اگر (m_parentp->is_And_درخت()) {
                    V3_عدد comp_رقم{srcp, m_width, 0};
                    comp_رقم.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    resultp = جدید AstEq{fl, جدید AstConst{fl, comp_رقم}, resultp};
                    ++ops;
                    clean = درست;
                } وگرنه {  // Or
                    V3_عدد comp_رقم{srcp, m_width, 0};
                    comp_رقم.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    comp_رقم.opXor(V3_عدد{comp_رقم}, mask_رقم);
                    resultp = جدید AstNeq{fl, جدید AstConst{fl, comp_رقم}, resultp};
                    ++ops;
                    clean = درست;
                }
            }

            بازگشت ResultTerm{resultp, ops, clean};
        }

        // CONSTRUCTORS
        VarInfo(Const_Bit_Op_درخت_Visitor* parent, AstVarRef* refp, صحیح width)
            : m_parentp{parent}
            , m_refp{refp}
            , m_width{width}
            , m_bitPolarity{refp, m_width} {
            m_bitPolarity.setAllBitsX();
        }
    };

    // MEMBERS
    منطقی m_failed = نادرست;
    منطقی m_polarity = درست;  // Flip when AstNot comes
    unsigned m_ops;  // Number of operations such as And, Or, Xor, Sel...
    صحیح m_lsb = 0;  // Current LSB
    LeafInfo* m_leafp = nullptr;  // AstConst or AstVarRef that currently looking for
    ثابت Ast_گره_عبارت* ثابت m_rootp;  // Root of this AST subtree

    std::vector<std::pair<Ast_گره_عبارت*, Frozen_گره_Info>>
        m_frozenNodes;  // Nodes that cannot be optimized
    std::vector<BitPolarityEntry> m_bitPolarities;  // Polarity of bits found during iterate()
    std::vector<std::unique_ptr<VarInfo>> m_varInfos;  // VarInfo for each variable, [0] is nullptr
    VNIdAllocator& m_ids;  // Node id allocator, owned by ConstVisitor
    // Base index of m_varInfos that points VarInfo, keyed by AstVarScope/AstVarRef id,
    // zero means not set yet.
    std::unordered_map<size_t, صحیح> m_baseIdxs;

    // METHODS

    منطقی is_And_درخت() ثابت { بازگشت VN_IS(m_rootp, And); }
    منطقی is_Or_درخت() ثابت { بازگشت VN_IS(m_rootp, Or); }
    منطقی is_Xor_درخت() ثابت { بازگشت VN_IS(m_rootp, Xor) || VN_IS(m_rootp, RedXor); }

#define CONST_BITOP_RETURN_IF(cond, nodep) \
    اگر (setFailed(cond, #cond, nodep, __LINE__)) بازگشت

#define CONST_BITOP_SET_FAILED(reason, nodep) setFailed(درست, reason, nodep, __LINE__)

    منطقی setFailed(منطقی fail, ثابت نویسه* reason, ثابت Ast_گره* nodep, صحیح سطر) {
        اگر (fail && !m_failed) {
            UINFO(9, "cannot optimize " << m_rootp << " reason:" << reason << " called from line:"
                                        << سطر << " when checking:" << nodep);
            // UINFOTREE(9, m_rootp, "", "root");
            m_failed = درست;
        }
        بازگشت m_failed;
    }
    تهی incrOps(ثابت Ast_گره* nodep, صحیح سطر) {
        ++m_ops;
        UINFO(9, "Increment to " << m_ops << " " << nodep << " called from line " << سطر);
    }
    VarInfo& getVarInfo(ثابت LeafInfo& ref) {
        UASSERT_OBJ(ref.refp(), m_rootp, "null varref in And/Or/Xor optimization");
        Ast_گره* nodep = ref.refp()->varScopep();
        اگر (!nodep) nodep = ref.refp()->varp();  // Not scoped
        صحیح& baseIdxr = m_baseIdxs[m_ids(nodep)];
        اگر (baseIdxr == 0) {  // Not set yet
            baseIdxr = m_varInfos.size();
            ثابت صحیح رقم_واژگان
                = ref.refp()->dtypep()->isWide() ? ref.refp()->dtypep()->width_واژگان() : 1;
            m_varInfos.resize(m_varInfos.size() + رقم_واژگان);
        }
        ثابت size_t idx = baseIdxr + std::max(0, ref.wordIdx());
        VarInfo* varInfop = m_varInfos[idx].get();
        اگر (!varInfop) {
            varInfop = جدید VarInfo{این, ref.refp(), ref.varWidth()};
            m_varInfos[idx].reset(varInfop);
            اگر (ref.missingWordSel()) {
                // ConstBitOpTreeVisitor makes some constants for masks and its type is uint64_t.
                // That's why V3Expand, that inserts WordSel, is needed.
                CONST_BITOP_SET_FAILED("V3Expand is skipped", ref.refp());
            }
        } وگرنه {
            اگر (!varInfop->sameVarAs(ref.refp()))
                CONST_BITOP_SET_FAILED("different var (scope?)", ref.refp());
        }
        بازگشت *varInfop;
    }

    // Traverse down to see AstConst or AstVarRef
    LeafInfo findLeaf(Ast_گره* nodep, منطقی expectConst) {
        اگر (!nodep->dtypep()->skipRefp()->isIntegralOrPacked()) بازگشت LeafInfo{};
        LeafInfo info{m_lsb};
        {
            VL_RESTORER(m_leafp);
            m_leafp = &info;
            iterateConst(nodep);
        }

        منطقی ok = !m_failed;
        اگر (expectConst) {
            ok &= !info.refp() && info.constp();
        } وگرنه {
            ok &= info.refp() && !info.constp();
        }
        بازگشت ok ? info : LeafInfo{};
    }

    // VISITORS
    تهی visit(Ast_گره* nodep) override { CONST_BITOP_SET_FAILED("Hit unexpected op", nodep); }
    تهی visit(AstCCast* nodep) override {
        iterateChildrenConst(nodep);
        اگر (m_leafp) m_leafp->updateBitRange(nodep);
    }
    تهی visit(AstShiftR* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ثابت AstConst* ثابت constp = VN_CAST(nodep->rhsp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->rhsp());
        m_lsb += constp->toUInt();
        incrOps(nodep, __LINE__);
        iterateConst(nodep->lhsp());
        m_leafp->updateBitRange(nodep);
        m_lsb -= constp->toUInt();
    }
    تهی visit(AstNot* nodep) override {
        CONST_BITOP_RETURN_IF(nodep->widthMin() != 1, nodep);
        Ast_گره* lhsp = nodep->lhsp();
        AstCCast* ثابت castp = VN_CAST(lhsp, CCast);
        اگر (castp) lhsp = castp->lhsp();
        CONST_BITOP_RETURN_IF(!is_Xor_درخت() && !VN_IS(lhsp, VarRef) && !VN_IS(lhsp, ShiftR), lhsp);
        incrOps(nodep, __LINE__);
        m_polarity = !m_polarity;
        iterateChildrenConst(nodep);
        // Don't restore m_polarity for Xor as it counts parity of the entire tree
        اگر (!is_Xor_درخت()) m_polarity = !m_polarity;
        اگر (m_leafp && castp) m_leafp->updateBitRange(castp);
        اگر (m_leafp) m_leafp->polarity(!m_leafp->polarity());
    }
    تهی visit(AstWordSel* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ثابت AstConst* ثابت constp = VN_CAST(nodep->bitp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->bitp());
        UASSERT_OBJ(m_leafp->wordIdx() == -1, nodep, "Unexpected nested WordSel");
        m_leafp->wordIdx(constp->toSInt());
        iterateConst(nodep->fromp());
    }
    تهی visit(AstVarRef* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }
    تهی visit(AstConst* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }

    تهی visit(AstRedXor* nodep) override {
        Restorer restorer{*این};
        CONST_BITOP_RETURN_IF(!VN_IS(m_rootp, Xor), nodep);
        Ast_گره* lhsp = nodep->lhsp();
        ثابت AstCCast* ثابت castp = VN_CAST(lhsp, CCast);
        اگر (castp) lhsp = castp->lhsp();
        اگر (ثابت AstAnd* ثابت andp = VN_CAST(lhsp, And)) {  // '^(mask & leaf)'
            CONST_BITOP_RETURN_IF(!andp, lhsp);

            ثابت LeafInfo& mask = findLeaf(andp->lhsp(), درست);
            CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

            LeafInfo ref = findLeaf(andp->rhsp(), نادرست);
            CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());
            اگر (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all subtree succeeded

            ثابت V3_عدد& mask_رقم = mask.constp()->رقم();

            incrOps(nodep, __LINE__);
            incrOps(andp, __LINE__);

            // Mark all bits checked in this reduction
            ثابت صحیح maxBitIdx = std::min(ref.lsb() + mask_رقم.width(), ref.msb() + 1);
            برای (صحیح bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                ثابت صحیح maskIdx = bitIdx - ref.lsb();
                اگر (mask_رقم.bitIs0(maskIdx)) ادامه;
                // Set true, m_polarity takes care of the entire parity
                m_bitPolarities.emplace_back(ref, درست, bitIdx);
            }
        } وگرنه {  // '^leaf'
            LeafInfo ref = findLeaf(lhsp, نادرست);
            CONST_BITOP_RETURN_IF(!ref.refp(), lhsp);
            اگر (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all checks passed

            incrOps(nodep, __LINE__);

            // Mark all bits checked by this comparison
            برای (صحیح bitIdx = ref.lsb(); bitIdx <= ref.msb(); ++bitIdx) {
                m_bitPolarities.emplace_back(ref, درست, bitIdx);
            }
        }
    }

    تهی visit(Ast_گره_Biop* nodep) override {
        اگر (VN_IS(nodep, And) && isConst(nodep->lhsp(), 1)) {  // 1 & _
            // Always reach past a plain making AND
            Restorer restorer{*این};
            incrOps(nodep, __LINE__);
            iterateConst(nodep->rhsp());
            اگر (m_leafp) m_leafp->limitBitRangeToLsb();
            CONST_BITOP_RETURN_IF(m_failed, nodep->rhsp());
            restorer.disableRestore();  // Now all checks passed
        } وگرنه اگر (nodep->type() == m_rootp->type()) {  // And, Or, Xor
            // subtree under NOT can be optimized only in XOR tree.
            CONST_BITOP_RETURN_IF(!m_polarity && !is_Xor_درخت(), nodep);
            incrOps(nodep, __LINE__);
            برای (ثابت منطقی راست : {نادرست, درست}) {
                VL_RESTORER(m_leafp);
                Restorer restorer{*این};
                LeafInfo leafInfo{m_lsb};
                // cppcheck-suppress danglingLifetime
                m_leafp = &leafInfo;
                Ast_گره_عبارت* opp = راست ? nodep->rhsp() : nodep->lhsp();
                ثابت منطقی origFailed = m_failed;
                iterateConst(opp);
                اگر (leafInfo.constp() || m_failed) {
                    // Revert changes in leaf
                    restorer.restoreNow();
                    // Reach past a cast then add to frozen nodes to be added to final reduction
                    اگر (ثابت AstCCast* ثابت castp = VN_CAST(opp, CCast)) opp = castp->lhsp();
                    ثابت منطقی pol = is_Xor_درخت() || m_polarity;  // Only AND/OR tree needs polarity
                    UASSERT_OBJ(pol, nodep, "AND/OR tree expects m_polarity==true");
                    m_frozenNodes.emplace_back(opp, Frozen_گره_Info{pol, m_lsb});
                    m_failed = origFailed;
                    ادامه;
                }
                restorer.disableRestore();  // Now all checks passed
                اگر (leafInfo.refp()) {
                    // The conditional on the lsb being in range is necessary for some degenerate
                    // case, e.g.: (IData)((QData)wide[0] >> 32), or <1-bit-var> >> 1, which is
                    // just zero
                    اگر (leafInfo.lsb() <= leafInfo.msb()) {
                        m_bitPolarities.emplace_back(leafInfo, is_Xor_درخت() || leafInfo.polarity(),
                                                     leafInfo.lsb());
                    } وگرنه اگر ((is_And_درخت() && leafInfo.polarity())
                               || (is_Or_درخت() && !leafInfo.polarity())) {
                        // If there is a constant 0 term in an And tree or 1 term in an Or tree, we
                        // must include it. Fudge this by adding a bit with both polarities, which
                        // will simplify to zero or one respectively.
                        // Note that Xor tree does not need this kind of care, polarity of Xor tree
                        // is already cared when visitin AstNot. Taking xor with 1'b0 is nop.
                        m_bitPolarities.emplace_back(leafInfo, درست, 0);
                        m_bitPolarities.emplace_back(leafInfo, نادرست, 0);
                    }
                }
            }
        } وگرنه اگر ((is_And_درخت() && VN_IS(nodep, Eq)) || (is_Or_درخت() && VN_IS(nodep, Neq))) {
            Restorer restorer{*این};
            CONST_BITOP_RETURN_IF(!m_polarity, nodep);
            CONST_BITOP_RETURN_IF(m_lsb, nodep);  // the result of EQ/NE is 1 bit width
            ثابت Ast_گره* lhsp = nodep->lhsp();
            اگر (ثابت AstCCast* ثابت castp = VN_CAST(lhsp, CCast)) lhsp = castp->lhsp();
            ثابت AstConst* ثابت constp = VN_CAST(lhsp, Const);
            CONST_BITOP_RETURN_IF(!constp, nodep->lhsp());

            ثابت V3_عدد& comp_رقم = constp->رقم();

            auto setPolarities = [این, &comp_رقم](ثابت LeafInfo& ref, ثابت V3_عدد* maskp) {
                ثابت منطقی maskFlip = is_And_درخت() ^ ref.polarity();
                صحیح constantWidth = comp_رقم.width();
                اگر (maskp) constantWidth = std::max(constantWidth, maskp->width());
                ثابت صحیح maxBitIdx = std::max(ref.lsb() + constantWidth, ref.msb() + 1);
                // Mark all bits checked by this comparison
                برای (صحیح bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                    ثابت صحیح maskIdx = bitIdx - ref.lsb();
                    ثابت منطقی mask0 = maskp && maskp->bitIs0(maskIdx);
                    ثابت منطقی outOfRange = bitIdx > ref.msb();
                    اگر (mask0 || outOfRange) {  // RHS is 0
                        اگر (comp_رقم.bitIs1(maskIdx)) {
                            // LHS is 1
                            // And tree: 1 == 0 => always false, set v && !v
                            // Or tree : 1 != 0 => always true, set v || !v
                            m_bitPolarities.emplace_back(ref, درست, 0);
                            m_bitPolarities.emplace_back(ref, نادرست, 0);
                            بشکن;
                        } وگرنه {  // This bitIdx is irrelevant
                            ادامه;
                        }
                    }
                    ثابت منطقی polarity = comp_رقم.bitIs1(maskIdx) != maskFlip;
                    m_bitPolarities.emplace_back(ref, polarity, bitIdx);
                }
            };

            اگر (ثابت AstAnd* ثابت andp = VN_CAST(nodep->rhsp(), And)) {  // comp == (mask & v)
                ثابت LeafInfo& mask = findLeaf(andp->lhsp(), درست);
                CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

                ثابت LeafInfo& ref = findLeaf(andp->rhsp(), نادرست);
                CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());

                restorer.disableRestore();  // Now all checks passed

                ثابت V3_عدد& mask_رقم = mask.constp()->رقم();

                incrOps(nodep, __LINE__);
                incrOps(andp, __LINE__);

                setPolarities(ref, &mask_رقم);
            } وگرنه {  // comp == v
                ثابت LeafInfo& ref = findLeaf(nodep->rhsp(), نادرست);
                CONST_BITOP_RETURN_IF(!ref.refp(), nodep->rhsp());

                restorer.disableRestore();  // Now all checks passed

                incrOps(nodep, __LINE__);

                setPolarities(ref, nullptr);
            }
        } وگرنه {
            CONST_BITOP_SET_FAILED("Mixture of different ops cannot be optimized", nodep);
        }
    }

    // CONSTRUCTORS
    Const_Bit_Op_درخت_Visitor(Ast_گره_عبارت* nodep, unsigned externalOps, VNIdAllocator& ids)
        : m_ops{externalOps}
        , m_rootp{nodep}
        , m_ids{ids} {
        // Fill nullptr at [0] because a base index of 0 means not set yet
        m_varInfos.افزودن_back(nullptr);
        CONST_BITOP_RETURN_IF(!is_And_درخت() && !is_Or_درخت() && !is_Xor_درخت(), nodep);
        اگر (Ast_گره_Biop* ثابت biopp = VN_CAST(nodep, گره_Biop)) {
            iterateConst(biopp);
        } وگرنه {
            UASSERT_OBJ(VN_IS(nodep, RedXor), nodep, "Must be RedXor");
            incrOps(nodep, __LINE__);
            iterateChildrenConst(nodep);
        }
        برای (auto&& entry : m_bitPolarities) {
            getVarInfo(entry.m_info).setPolarity(entry.m_polarity, entry.m_bit);
        }
        UASSERT_OBJ(is_Xor_درخت() || m_polarity, nodep, "must be the original polarity");
    }
    مجازی ~Const_Bit_Op_درخت_Visitor() = پیشفرض;
#undef CONST_BITOP_RETURN_IF
#undef CONST_BITOP_SET_FAILED

عمومی:
    // Transform as below.
    // v[0] & v[1] => 2'b11 == (2'b11 & v)
    // v[0] | v[1] => 2'b00 != (2'b11 & v)
    // v[0] ^ v[1] => ^{2'b11 & v}
    // (3'b011 == (3'b011 & v)) & v[2]  => 3'b111 == (3'b111 & v)
    // (3'b000 != (3'b011 & v)) | v[2]  => 3'b000 != (3'b111 & v)
    // Reduction ops are transformed in the same way.
    // &{v[0], v[1]} => 2'b11 == (2'b11 & v)
    ایستا Ast_گره_عبارت* simplify(Ast_گره_عبارت* nodep, صحیح resultWidth, unsigned externalOps,
                                 VDouble0& reduction, VNIdAllocator& ids) {
        UASSERT_OBJ(1 <= resultWidth && resultWidth <= 64, nodep, "resultWidth out of range");

        // Walk tree, gathering all terms referenced in expression
        ثابت Const_Bit_Op_درخت_Visitor visitor{nodep, externalOps, ids};

        // If failed on root node is not optimizable, or there are no variable terms, then done
        اگر (visitor.m_failed || visitor.m_varInfos.size() == 1) بازگشت nullptr;

        // FileLine used for constructing all new nodes in this function
        File_سطر* ثابت fl = nodep->fileline();

        // Get partial result each term referenced, count total number of ops and keep track of
        // whether we have clean/dirty terms. visitor.m_varInfos appears in deterministic order,
        // so the optimized tree is deterministic as well.

        std::vector<Ast_گره_عبارت*> termps;
        termps.reserve(visitor.m_varInfos.size() - 1);
        unsigned resultOps = 0;
        منطقی hasCleanTerm = نادرست;
        منطقی hasDirtyTerm = نادرست;

        برای (auto&& v : visitor.m_varInfos) {
            اگر (!v) ادامه;  // Skip nullptr at m_varInfos[0]
            اگر (v->hasConstResult()) {
                // If a constant term is known, we can either drop it or the whole tree is constant
                Ast_گره_عبارت* resultp = nullptr;
                اگر (v->getConstResult()) {
                    UASSERT_OBJ(visitor.is_Or_درخت(), nodep,
                                "Only OR tree can yield known 1 result");
                    UINFO(9, "OR tree with const 1 term: " << v->refp());
                    // Known 1 bit in OR tree, whole result is 1
                    resultp = جدید AstConst{fl, AstConst::BitTrue{}};
                } وگرنه اگر (visitor.is_And_درخت()) {
                    UINFO(9, "AND tree with const 0 term: " << v->refp());
                    // Known 0 bit in AND tree, whole result is 0
                    resultp = جدید AstConst{fl, AstConst::BitFalse{}};
                } وگرنه {
                    // Known 0 bit in OR or XOR tree. Ignore it.
                    ادامه;
                }
                // Set width and widthMin precisely
                resultp->dtypeChgWidth(resultWidth, 1);
                برای (Ast_گره* ثابت termp : termps) VL_DO_DANGLING(termp->delete_درخت(), termp);
                بازگشت resultp;
            }
            ثابت ResultTerm result = v->getResultTerm();
            termps.افزودن_back(std::get<0>(result));
            resultOps += std::get<1>(result);
            اگر (std::get<2>(result)) {
                hasCleanTerm = درست;
                UINFO(9, "Clean term: " << termps.back());
            } وگرنه {
                hasDirtyTerm = درست;
                UINFO(9, "Dirty term: " << termps.back());
            }
        }

        // Group by FrozenNodeInfo
        std::map<Frozen_گره_Info, std::vector<Ast_گره_عبارت*>> frozenNodes;
        // Check if frozen terms are clean or not
        برای (ثابت auto& frozenInfo : visitor.m_frozenNodes) {
            Ast_گره_عبارت* ثابت termp = frozenInfo.first;
            // Comparison operators are clean
            اگر ((VN_IS(termp, Eq) || VN_IS(termp, Neq) || VN_IS(termp, Lt) || VN_IS(termp, Lte)
                 || VN_IS(termp, Gt) || VN_IS(termp, Gte))
                && frozenInfo.second.m_lsb == 0) {
                hasCleanTerm = درست;
            } وگرنه {
                // Otherwise, conservatively assume the frozen term is dirty
                hasDirtyTerm = درست;
                UINFO(9, "Dirty frozen term: " << termp);
            }
            frozenNodes[frozenInfo.second].افزودن_back(termp);
        }

        // Figure out if a final negation is required
        ثابت منطقی needsFlip = visitor.is_Xor_درخت() && !visitor.m_polarity;

        // Figure out if the final tree needs cleaning
        ثابت منطقی needsCleaning = visitor.is_And_درخت() ? !hasCleanTerm : hasDirtyTerm;

        // Add size of reduction tree to op count
        resultOps += termps.size() - 1;
        برای (ثابت auto& lsbAndNodes : frozenNodes) {
            اگر (lsbAndNodes.first.m_lsb > 0) ++resultOps;  // Needs AstShiftR
            اگر (!lsbAndNodes.first.m_polarity) ++resultOps;  // Needs AstNot
            resultOps += lsbAndNodes.second.size();
        }
        // Add final polarity flip in Xor tree
        اگر (needsFlip) ++resultOps;
        // Add final cleaning AND
        اگر (needsCleaning) ++resultOps;

        اگر (debug() >= 9) {  // LCOV_EXCL_START
            cout << "-  Bitop tree considered:\n";
            برای (ثابت Ast_گره_عبارت* ثابت termp : termps) termp->dump_درخت("-  Reduced term: ");
            برای (ثابت std::pair<Ast_گره_عبارت*, Frozen_گره_Info>& termp : visitor.m_frozenNodes) {
                termp.first->dump_درخت("-  Frozen term with lsb "
                                      + std::to_string(termp.second.m_lsb) + " polarity "
                                      + std::to_string(termp.second.m_polarity) + ": ");
            }
            cout << "-  Needs flipping: " << needsFlip << "\n";
            cout << "-  Needs cleaning: " << needsCleaning << "\n";
            cout << "-  Size: " << resultOps << " input size: " << visitor.m_ops << "\n";
        }  // LCOV_EXCL_STOP

        // Sometimes we have no terms left after ignoring redundant terms
        // (all of which were zeroes)
        اگر (termps.empty() && visitor.m_frozenNodes.empty()) {
            reduction += visitor.m_ops;
            Ast_گره_عبارت* ثابت resultp = needsFlip ? جدید AstConst{fl, AstConst::BitTrue{}}
                                                   : جدید AstConst{fl, AstConst::BitFalse{}};
            resultp->dtypeChgWidth(resultWidth, 1);
            بازگشت resultp;
        }

        // Only substitute the result if beneficial as determined by operation count
        اگر (visitor.m_ops <= resultOps) {
            برای (Ast_گره* ثابت termp : termps) VL_DO_DANGLING(termp->delete_درخت(), termp);
            بازگشت nullptr;
        }

        // Update statistics
        reduction += visitor.m_ops - resultOps;

        // Reduction op to combine terms
        ثابت auto reduce = [&visitor, fl](Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp) -> Ast_گره_عبارت* {
            اگر (!lhsp) بازگشت rhsp;
            اگر (visitor.is_And_درخت()) {
                بازگشت جدید AstAnd{fl, lhsp, rhsp};
            } وگرنه اگر (visitor.is_Or_درخت()) {
                بازگشت جدید AstOr{fl, lhsp, rhsp};
            } وگرنه {
                بازگشت جدید AstXor{fl, lhsp, rhsp};
            }
        };

        // Compute result by reducing all terms
        Ast_گره_عبارت* resultp = nullptr;
        برای (Ast_گره_عبارت* ثابت termp : termps) {  //
            resultp = reduce(resultp, termp);
        }
        // Add any frozen terms to the reduction
        برای (auto&& nodes : frozenNodes) {
            // nodes.second has same lsb and polarity
            Ast_گره_عبارت* termp = nullptr;
            برای (Ast_گره_عبارت* ثابت itemp : nodes.second) {
                termp = reduce(termp, itemp->unlinkFrBack());
            }
            اگر (nodes.first.m_lsb > 0) {  // LSB is not 0, so shiftR
                Ast_گره_DType* ثابت dtypep = termp->dtypep();
                termp = جدید AstShiftR{termp->fileline(), termp,
                                      جدید AstConst(termp->fileline(), AstConst::Widthed_مقدار{},
                                                   termp->width(), nodes.first.m_lsb)};
                termp->dtypep(dtypep);
            }
            اگر (!nodes.first.m_polarity) {  // Polarity is inverted, so append Not
                Ast_گره_DType* ثابت dtypep = termp->dtypep();
                termp = جدید AstNot{termp->fileline(), termp};
                termp->dtypep(dtypep);
            }
            resultp = reduce(resultp, termp);
        }

        // Set width of masks to expected result width. This is required to prevent later removal
        // of the masking node e.g. by the "AND with all ones" rule. If the result width happens
        // to be 1, we still need to ensure the AstAnd is not dropped, so use a wider mask in this
        // special case.
        ثابت صحیح maskWidth
            = std::max(resultp->width(), resultWidth == 1 ? VL_IDATASIZE : resultWidth);

        // Apply final polarity flip
        اگر (needsFlip) {
            اگر (needsCleaning) {
                // Cleaning will be added below. Use a NOT which is a byte shorter on x86
                resultp = جدید AstNot{fl, resultp};
            } وگرنه {
                // Keep result clean by using XOR(1, _)
                AstConst* ثابت maskp = جدید AstConst{fl, AstConst::Widthed_مقدار{}, maskWidth, 1};
                resultp = جدید AstXor{fl, maskp, resultp};
            }
        }

        // Apply final cleaning
        اگر (needsCleaning) {
            AstConst* ثابت maskp = جدید AstConst{fl, AstConst::Widthed_مقدار{}, maskWidth, 1};
            resultp = جدید AstAnd{fl, maskp, resultp};
        }

        // Cast back to original size if required
        اگر (resultp->width() != resultWidth) {
            resultp = جدید AstCCast{fl, resultp, resultWidth, 1};
        }

        // Set width and widthMin precisely
        resultp->dtypeChgWidth(resultWidth, 1);

        بازگشت resultp;
    }
};

//######################################################################
// Const state, as a visitor of each AstNode

رده ConstVisitor final : عمومی VNVisitor {
    // CONSTANTS
    ایستا constexpr unsigned CONCAT_MERGABLE_MAX_DEPTH = 10;  // Limit alg recursion
    // Largest right-hand side that is copied to each part of a split concatenation assignment
    ایستا constexpr صحیح CONCAT_ASSIGN_MAX_RHS_NODES = 16;

    // NODE STATE
    // See VNIdAllocator. All per node state below is held in side tables keyed on the
    // node ids it hands out, as node pointers are not stable keys within V3Const.

    // STATE
    VNIdAllocator m_ids;  // Node id allocator
    منطقی m_params = نادرست;  // If true, propagate parameterized and true numbers only
    منطقی m_required = نادرست;  // If true, must become a constant
    منطقی m_wremove = درست;  // Inside scope, no assignw removal
    منطقی m_warn = نادرست;  // Output warnings
    منطقی m_doExpensive = نادرست;  // Enable computationally expensive optimizations
    منطقی m_doCpp = نادرست;  // Enable late-stage C++ optimizations
    منطقی m_doNConst = نادرست;  // Enable non-constant-child simplifications
    منطقی m_doV = نادرست;  // Verilog, not C++ conversion
    منطقی m_doGenerate = نادرست;  // Postpone width checking inside generate
    منطقی m_convertLogicToBit = نادرست;  // Convert logical operators to bitwise
    منطقی m_hasJumpDelay = نادرست;  // JumpGo or Delay under this loop
    منطقی m_hasLoopTest = نادرست;  // Contains AstLoopTest
    منطقی m_underRecFunc = نادرست;  // Under a recursive function
    Ast_گره_Module* m_modp = nullptr;  // Current module
    ثابت AstArraySel* m_selp = nullptr;  // Current select
    ثابت AstScope* m_scopep = nullptr;  // Current scope
    ثابت AstAttrOf* m_attrp = nullptr;  // Current attribute
    VDouble0 m_statBitOpReduction;  // Ops reduced in ConstBitOpTreeVisitor
    VDouble0 m_statConcatMerge;  // Concat merges
    VDouble0 m_stat_Cond_عبارت_Redundant;  // Conditional repeated expressions
    VDouble0 m_stat_If_Cond_عبارت_Redundant;  // Conditional repeated expressions
    VDouble0 m_statMemberAccessVisits;  // Nodes visited by containsMemberAccessRecurse
    ثابت منطقی m_globalPass;  // ConstVisitor invoked as a global pass
    ایستا uint32_t s_global_Pass_رقم;  // Counts number of times ConstVisitor invoked as global pass
    V3UniqueNames m_concswapNames;  // For generating unique temporary variable names
    // Caches results of matchBiopToBitwise, keyed on node id
    std::unordered_map<size_t, منطقی> m_containsMemberAccess;
    // Items of enum currently being recursed into. Cannot use VNIdAllocator, as this runs in
    // every mode, including those invoked under a pass holding user4 (V3Param). Node pointers
    // are safe as keys here, unlike elsewhere in V3Const: folding the item value below does
    // free nodes whose addresses are then reused, but AstEnumItem is only ever constructed
    // while parsing (verilog.y and V3LinkParse), so a reused address can never become one,
    // and every key looked up is an AstEnumItem.
    std::unordered_set<ثابت AstEnumItem*> m_recursing_Enum_اقلام;
    std::unordered_set<AstJumpBlock*> m_usedJumpBlocks;  // JumpBlocks used by some JumpGo

    // METHODS

    // Return true if statements may be deleted. Until statics are lifted, a variable declared
    // in a statement might be referenced from elsewhere (e.g. hierarchically).
    ایستا منطقی canDeleteStatements() { بازگشت v3Global.staticsLifted(); }

    تهی deleteVarScopesUnder(Ast_گره* subtreep) {
        اگر (!subtreep) بازگشت;
        اگر (!m_scopep) بازگشت;
        std::unordered_set<AstVar*> varps;
        subtreep->foreachAndNext([&](AstVar* varp) { varps.درج(varp); });
        اگر (varps.empty()) بازگشت;
        برای (AstVarScope *vscp = m_scopep->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            اگر (varps.find(vscp->varp()) != varps.end()) {
                VL_DO_DANGLING(افزودن_Deletep(vscp->unlinkFrBack()), vscp);
            }
        }
    }

    V3_عدد const_رقم_V(Ast_گره* nodep) {
        // Contract C width to V width (if needed, else just direct copy)
        // The upper zeros in the C representation can otherwise cause
        // wrong results in some operations, e.g. MulS
        ثابت V3_عدد& numc = VN_AS(nodep, Const)->رقم();
        بازگشت !numc.is_عدد() ? numc : V3_عدد{nodep, nodep->widthMinV(), numc};
    }
    V3_عدد to_رقم_C(Ast_گره* nodep, ثابت V3_عدد& numv) {
        // Extend V width back to C width for given node
        بازگشت !numv.is_عدد() ? numv : V3_عدد{nodep, nodep->width(), numv};
    }

    ایستا منطقی lowerAsFixedAggregate(ثابت Ast_گره_DType* ثابت dtypep) {
        بازگشت dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }

    AstStructSel* newStructSel(Ast_گره_عبارت* ثابت fromp, ثابت AstMemberDType* ثابت itemp) {
        AstStructSel* ثابت selp = جدید AstStructSel{fromp->fileline(), fromp, itemp->name()};
        selp->dtypeFrom(itemp->dtypep());
        بازگشت selp;
    }

    تهی collectFixedAggregateTerms(Ast_گره_عبارت* ثابت fromp, std::vector<Ast_گره_عبارت*>& termps,
                                    ثابت منطقی packReal) {
        ثابت Ast_گره_DType* ثابت dtypep = fromp->dtypep()->skipRefp();
        اگر (ثابت AstUnpackArrayDType* ثابت unpackDtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            برای (ثابت صحیح idx : unpackDtypep->declRange().seq_چپ_To_راست()) {
                AstArraySel* ثابت selp
                    = جدید AstArraySel{fromp->fileline(), fromp->clone_درخت_Pure(نادرست), idx};
                collectFixedAggregateTerms(selp, termps, packReal);
            }
            VL_DO_DANGLING(افزودن_Deletep(fromp), fromp);
        } وگرنه اگر (ثابت Ast_گره_UOr_Struct_DType* ثابت sdtypep
                   = VN_CAST(dtypep, گره_UOr_Struct_DType)) {
            اگر (sdtypep->packed()) {
                termps.افزودن_back(fromp);
                بازگشت;
            }
            برای (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                collectFixedAggregateTerms(newStructSel(fromp->clone_درخت_Pure(نادرست), itemp),
                                           termps, packReal);
            }
            VL_DO_DANGLING(افزودن_Deletep(fromp), fromp);
        } وگرنه اگر (packReal && dtypep->isDouble()) {
            termps.افزودن_back(جدید AstRealToBits{fromp->fileline(), fromp});
        } وگرنه {
            termps.افزودن_back(fromp);
        }
    }

    Ast_گره_عبارت* packFixedAggregate(Ast_گره_عبارت* ثابت fromp) {
        std::vector<Ast_گره_عبارت*> termps;
        collectFixedAggregateTerms(fromp, termps, درست);
        UASSERT(!termps.empty(), "No stream terms");
        Ast_گره_عبارت* resultp = termps[0];
        برای (size_t i = 1; i < termps.size(); ++i) {
            resultp = جدید AstConcat{resultp->fileline(), resultp, termps[i]};
        }
        بازگشت resultp;
    }

    تهی replaceAssignToFixedAggregate(Ast_گره_Assign* ثابت nodep, Ast_گره_عبارت* ثابت dstp,
                                       Ast_گره_عبارت* srcp) {
        ثابت صحیح dstWidth = dstp->dtypep()->widthStream();
        std::vector<Ast_گره_عبارت*> termps;
        collectFixedAggregateTerms(dstp, termps, نادرست);
        صحیح srcWidth = srcp->width();
        اگر (srcWidth < dstWidth) {
            AstExtend* ثابت extendp = جدید AstExtend{srcp->fileline(), srcp, dstWidth};
            srcp = جدید AstShiftL{
                extendp->fileline(), extendp,
                جدید AstConst{extendp->fileline(), static_cast<uint32_t>(dstWidth - srcWidth)},
                dstWidth};
            srcWidth = dstWidth;
        }
        Ast_گره_Assign* newp = nullptr;
        صحیح offset = 0;
        برای (size_t i = 0; i < termps.size(); ++i) {
            Ast_گره_عبارت* ثابت termp = termps[i];
            ثابت صحیح width = termp->dtypep()->widthStream();
            ثابت صحیح lsb = srcWidth - offset - width;
            offset += width;
            Ast_گره_عبارت* rhsp
                = جدید AstSel{srcp->fileline(), srcp->clone_درخت_Pure(نادرست), lsb, width};
            اگر (termp->dtypep()->skipRefp()->isDouble()) {
                rhsp = جدید AstBitsToRealD{rhsp->fileline(), rhsp};
            }
            Ast_گره_Assign* ثابت assignp = nodep->cloneType(termp, rhsp);
            assignp->dtypeFrom(termp);
            newp = Ast_گره::addNext(newp, assignp);
        }
        nodep->addNextHere(newp);
        VL_DO_DANGLING(افزودن_Deletep(srcp), srcp);
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }

    منطقی operandConst(ثابت Ast_گره* nodep) { بازگشت VN_IS(nodep, Const); }
    منطقی operandAsvConst(ثابت Ast_گره* nodep) {
        // BIASV(CONST, BIASV(CONST,...)) -> BIASV( BIASV_CONSTED(a,b), ...)
        ثابت Ast_گره_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, گره_Bi_Com_Asv);
        اگر (!bnodep) بازگشت نادرست;
        اگر (!VN_IS(bnodep->lhsp(), Const)) بازگشت نادرست;
        ثابت Ast_گره_Bi_Com_Asv* ثابت rnodep = VN_CAST(bnodep->rhsp(), گره_Bi_Com_Asv);
        اگر (!rnodep) بازگشت نادرست;
        اگر (rnodep->type() != bnodep->type()) بازگشت نادرست;
        اگر (rnodep->width() != bnodep->width()) بازگشت نادرست;
        اگر (rnodep->lhsp()->width() != bnodep->lhsp()->width()) بازگشت نادرست;
        اگر (!VN_IS(rnodep->lhsp(), Const)) بازگشت نادرست;
        بازگشت درست;
    }
    منطقی operandAsvSame(ثابت Ast_گره* nodep) {
        // BIASV(SAMEa, BIASV(SAMEb,...)) -> BIASV( BIASV(SAMEa,SAMEb), ...)
        ثابت Ast_گره_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, گره_Bi_Com_Asv);
        اگر (!bnodep) بازگشت نادرست;
        ثابت Ast_گره_Bi_Com_Asv* ثابت rnodep = VN_CAST(bnodep->rhsp(), گره_Bi_Com_Asv);
        اگر (!rnodep) بازگشت نادرست;
        اگر (rnodep->type() != bnodep->type()) بازگشت نادرست;
        اگر (rnodep->width() != bnodep->width()) بازگشت نادرست;
        بازگشت operandsSame(bnodep->lhsp(), rnodep->lhsp());
    }
    منطقی operandAsvLUp(ثابت Ast_گره* nodep) {
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
        ثابت Ast_گره_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, گره_Bi_Com_Asv);
        اگر (!bnodep) بازگشت نادرست;
        ثابت Ast_گره_Bi_Com_Asv* ثابت lnodep = VN_CAST(bnodep->lhsp(), گره_Bi_Com_Asv);
        اگر (!lnodep) بازگشت نادرست;
        اگر (lnodep->type() != bnodep->type()) بازگشت نادرست;
        اگر (lnodep->width() != bnodep->width()) بازگشت نادرست;
        بازگشت VN_IS(lnodep->lhsp(), Const);
    }
    منطقی operandAsvRUp(ثابت Ast_گره* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr)) ?
        ثابت Ast_گره_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, گره_Bi_Com_Asv);
        اگر (!bnodep) بازگشت نادرست;
        ثابت Ast_گره_Bi_Com_Asv* ثابت rnodep = VN_CAST(bnodep->rhsp(), گره_Bi_Com_Asv);
        اگر (!rnodep) بازگشت نادرست;
        اگر (rnodep->type() != bnodep->type()) بازگشت نادرست;
        اگر (rnodep->width() != bnodep->width()) بازگشت نادرست;
        بازگشت VN_IS(rnodep->lhsp(), Const);
    }
    ایستا منطقی operandSubAdd(ثابت Ast_گره* nodep) {
        // SUB( ADD(CONSTx,y), CONSTz) -> ADD(SUB(CONSTx,CONSTz), y)
        ثابت Ast_گره_Biop* ثابت np = VN_CAST(nodep, گره_Biop);
        ثابت Ast_گره_Biop* ثابت lp = VN_CAST(np->lhsp(), گره_Biop);
        بازگشت (lp && VN_IS(lp->lhsp(), Const) && VN_IS(np->rhsp(), Const)
                && lp->width() == np->width());
    }
    منطقی matchRedundantClean(AstAnd* andp) {
        // Remove And with constant one inserted by V3Clean
        // 1 & (a == b)  -> (IData)(a == b)
        // When bool is casted to int, the value is either 0 or 1
        AstConst* ثابت constp = VN_AS(andp->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isOne(), andp->lhsp(), "TRREEOPC must meet this condition");
        Ast_گره_عبارت* ثابت rhsp = andp->rhsp();
        AstCCast* ccastp = nullptr;
        ثابت auto isEqOrNeq = [](ثابت Ast_گره* nodep) -> منطقی {  //
            بازگشت VN_IS(nodep, Eq) || VN_IS(nodep, Neq);
        };
        اگر (isEqOrNeq(rhsp)) {
            ccastp = جدید AstCCast{andp->fileline(), rhsp->unlinkFrBack(), andp};
        } وگرنه اگر (AstCCast* ثابت tmpp = VN_CAST(rhsp, CCast)) {
            اگر (isEqOrNeq(tmpp->lhsp())) {
                اگر (tmpp->width() == andp->width()) {
                    tmpp->unlinkFrBack();
                    ccastp = tmpp;
                } وگرنه {
                    ccastp = جدید AstCCast{andp->fileline(), tmpp->lhsp()->unlinkFrBack(), andp};
                }
            }
        }
        اگر (ccastp) {
            andp->replaceWithKeepDType(ccastp);
            VL_DO_DANGLING(افزودن_Deletep(andp), andp);
            بازگشت درست;
        }
        بازگشت نادرست;
    }

    ایستا منطقی operandAndOrSame(ثابت Ast_گره* nodep) {
        // OR( AND(VAL,x), AND(VAL,y)) -> AND(VAL,OR(x,y))
        // OR( AND(x,VAL), AND(y,VAL)) -> AND(OR(x,y),VAL)
        ثابت Ast_گره_Biop* ثابت np = VN_CAST(nodep, گره_Biop);
        ثابت Ast_گره_Biop* ثابت lp = VN_CAST(np->lhsp(), گره_Biop);
        ثابت Ast_گره_Biop* ثابت rp = VN_CAST(np->rhsp(), گره_Biop);
        بازگشت (lp && rp && lp->width() == rp->width() && lp->type() == rp->type()
                && (operandsSame(lp->lhsp(), rp->lhsp()) || operandsSame(lp->rhsp(), rp->rhsp())));
    }
    منطقی matchOrAndNot(Ast_گره_Biop* nodep) {
        // AstOr{$a, AstAnd{AstNot{$b}, $c}} if $a.width1, $a==$b => AstOr{$a,$c}
        // Someday we'll sort the biops completely and this can be simplified
        // This often results from our simplified clock generation:
        // if (rst) ... else if (enable)... -> OR(rst,AND(!rst,enable))
        ثابت Ast_گره_عبارت* ap;
        Ast_گره_Biop* andp;
        اگر (VN_IS(nodep->lhsp(), And)) {
            andp = VN_AS(nodep->lhsp(), And);
            ap = nodep->rhsp();
        } وگرنه اگر (VN_IS(nodep->rhsp(), And)) {
            andp = VN_AS(nodep->rhsp(), And);
            ap = nodep->lhsp();
        } وگرنه {
            بازگشت نادرست;
        }
        ثابت Ast_گره_Uniop* notp;
        Ast_گره_عبارت* cp;
        اگر (VN_IS(andp->lhsp(), Not)) {
            notp = VN_AS(andp->lhsp(), Not);
            cp = andp->rhsp();
        } وگرنه اگر (VN_IS(andp->rhsp(), Not)) {
            notp = VN_AS(andp->rhsp(), Not);
            cp = andp->lhsp();
        } وگرنه {
            بازگشت نادرست;
        }
        ثابت Ast_گره_عبارت* ثابت bp = notp->lhsp();
        اگر (!operandsSame(ap, bp)) بازگشت نادرست;
        // Do it
        cp->unlinkFrBack();
        VL_DO_DANGLING(افزودن_Deletep(andp->unlinkFrBack()), andp);
        VL_DANGLING(notp);
        // Replace whichever branch is now dangling
        اگر (nodep->rhsp()) {
            nodep->lhsp(cp);
        } وگرنه {
            nodep->rhsp(cp);
        }
        بازگشت درست;
    }
    منطقی matchAndCond(AstAnd* nodep) {
        // Push down a AND into conditional, when one side of conditional is constant
        // (otherwise we'd be trading one operation for two operations)
        // V3Clean often makes this pattern, as it postpones the AND until
        // as high as possible, which is usually the right choice, except for this.
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstCond* ثابت condp = VN_CAST(nodep->rhsp(), Cond);
        اگر (!condp) بازگشت نادرست;
        اگر (!VN_IS(condp->thenp(), Const) && !VN_IS(condp->elsep(), Const)) بازگشت نادرست;
        AstConst* ثابت maskp = VN_CAST(nodep->lhsp(), Const);
        اگر (!maskp) بازگشت نادرست;
        UINFO(4, "AND(CONSTm, CONDcond(c, i, e))->CONDcond(c, AND(m,i), AND(m, e)) " << nodep);
        AstCond* ثابت newp = جدید AstCond{
            condp->fileline(), condp->condp()->unlinkFrBack(),
            جدید AstAnd{nodep->fileline(), maskp->clone_درخت(نادرست), condp->thenp()->unlinkFrBack()},
            جدید AstAnd{nodep->fileline(), maskp->clone_درخت(نادرست),
                       condp->elsep()->unlinkFrBack()}};
        newp->thenp()->dtypeFrom(nodep);  // As And might have been to change widths
        newp->elsep()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        بازگشت درست;
    }

    منطقی matchCondCond(ثابت AstCond* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        اگر (VN_IS(nodep->backp(), Cond)) بازگشت نادرست;  // Was checked when visited parent
        اگر (!VN_IS(nodep->thenp(), Cond) && !VN_IS(nodep->elsep(), Cond))
            بازگشت نادرست;  // Short circuit
        std::vector<Ast_گره_عبارت*> truesp;
        std::vector<Ast_گره_عبارت*> falsesp;
        matchCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
        بازگشت نادرست;  // Can optimize further
    }
    تهی matchCondCondRecurse(ثابت AstCond* nodep, std::vector<Ast_گره_عبارت*>& truesp,
                              std::vector<Ast_گره_عبارت*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        اگر (truesp.size() > 4 || falsesp.size() > 4) بازگشت;
        اگر (!nodep->condp()->isPure()) بازگشت;
        منطقی replaced = نادرست;
        برای (ثابت Ast_گره_عبارت* condp : truesp) {
            اگر (replaced) بشکن;
            اگر (!operandsSame(nodep->condp(), condp)) ادامه;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_رقم(nodep->condp(), 1);
            replaced = درست;
            ++m_stat_Cond_عبارت_Redundant;
        }
        برای (ثابت Ast_گره_عبارت* condp : falsesp) {
            اگر (replaced) بشکن;
            اگر (!operandsSame(nodep->condp(), condp)) ادامه;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = درست;
            ++m_stat_Cond_عبارت_Redundant;
        }
        اگر (ثابت AstCond* subCondp = VN_CAST(nodep->thenp(), Cond)) {
            اگر (!replaced) truesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            اگر (!replaced) truesp.برداشت_back();
        }
        اگر (ثابت AstCond* subCondp = VN_CAST(nodep->elsep(), Cond)) {
            اگر (!replaced) falsesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            اگر (!replaced) falsesp.برداشت_back();
        }
    }
    تهی matchIfCondCond(ثابت Ast_گره_If* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        اگر (VN_IS(nodep->backp(), If)) بازگشت;  // Was checked when visited parent
        اگر (!VN_IS(nodep->thensp(), If) && !VN_IS(nodep->elsesp(), If)) بازگشت;  // Short circuit
        std::vector<Ast_گره_عبارت*> truesp;
        std::vector<Ast_گره_عبارت*> falsesp;
        matchIfCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
    }
    تهی matchIfCondCondRecurse(ثابت Ast_گره_If* nodep, std::vector<Ast_گره_عبارت*>& truesp,
                                std::vector<Ast_گره_عبارت*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        اگر (truesp.size() > 4 || falsesp.size() > 4) بازگشت;
        اگر (!nodep->condp()->isPure()) بازگشت;
        // A constant condition is folded by visit(AstNodeIf), once statements can be deleted
        اگر (VN_IS(nodep->condp(), Const)) بازگشت;
        منطقی replaced = نادرست;
        برای (ثابت Ast_گره_عبارت* condp : truesp) {
            اگر (replaced) بشکن;
            اگر (!operandsSame(nodep->condp(), condp)) ادامه;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_رقم(nodep->condp(), 1);
            replaced = درست;
            ++m_stat_If_Cond_عبارت_Redundant;
        }
        برای (ثابت Ast_گره_عبارت* condp : falsesp) {
            اگر (replaced) بشکن;
            اگر (!operandsSame(nodep->condp(), condp)) ادامه;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = درست;
            ++m_stat_If_Cond_عبارت_Redundant;
        }
        // We only check the first statement of parent IF is an If
        // So we don't need to check for effects in the executing thensp/elsesp
        // altering the child't condition.  e.g. 'if (x) begin x=1; if (x) end'
        اگر (ثابت Ast_گره_If* subIfp = VN_CAST(nodep->thensp(), گره_If)) {
            اگر (!replaced) truesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            اگر (!replaced) truesp.برداشت_back();
        }
        اگر (ثابت Ast_گره_If* subIfp = VN_CAST(nodep->elsesp(), گره_If)) {
            اگر (!replaced) falsesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            اگر (!replaced) falsesp.برداشت_back();
        }
    }

    تهی matchIfSameLast(Ast_گره_If* nodep) {
        اگر (!nodep->thensp()) بازگشت;
        اگر (!nodep->elsesp()) بازگشت;
        اگر (!canDeleteStatements()) بازگشت;
        // Iterate bodies in reverse
        Ast_گره* tp = nodep->thensp()->lastp();
        Ast_گره* ep = nodep->elsesp()->lastp();
        وقتی (tp && ep && tp->same_درخت(ep)) {
            UASSERT_OBJ(!tp->nextp(), tp, "Expected no nextp");
            UASSERT_OBJ(!ep->nextp(), ep, "Expected no nextp");
            Ast_گره* ثابت tPrevp = tp->prevp();
            Ast_گره* ثابت ePrevp = ep->prevp();
            nodep->addNextHere(tp->unlinkFrBack());
            VL_DO_DANGLING(افزودن_Deletep(ep->unlinkFrBack()), ep);
            tp = tPrevp;
            ep = ePrevp;
        }
    }

    منطقی matchMaskedOr(AstAnd* nodep) {
        // Masking an OR with terms that have no bits set under the mask is replaced with masking
        // only the remaining terms. Canonical example as generated by V3Expand is:
        // 0xff & (a << 8 | b >> 24) --> 0xff & (b >> 24)

        // Compute how many significant bits are in the mask
        ثابت AstConst* ثابت constp = VN_AS(nodep->lhsp(), Const);
        ثابت uint32_t significantBits = constp->رقم().widthToFit();

        AstOr* ثابت orp = VN_AS(nodep->rhsp(), Or);

        // Predicate for checking whether the bottom 'significantBits' bits of the given expression
        // are all zeroes.
        ثابت auto checkBottomClear = [=](ثابت Ast_گره* nodep) -> منطقی {
            اگر (ثابت AstShiftL* ثابت shiftp = VN_CAST(nodep, ShiftL)) {
                اگر (ثابت AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                    بازگشت scp->رقم().toUInt() >= significantBits;
                }
            }
            بازگشت نادرست;
        };

        ثابت منطقی orLIsRedundant = checkBottomClear(orp->lhsp());
        ثابت منطقی orRIsRedundant = checkBottomClear(orp->rhsp());

        اگر (orLIsRedundant && orRIsRedundant) {
            nodep->replaceWithKeepDType(
                جدید AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت درست;
        } وگرنه اگر (orLIsRedundant) {
            orp->replaceWithKeepDType(orp->rhsp()->unlinkFrBack());
            VL_DO_DANGLING(افزودن_Deletep(orp), orp);
            بازگشت نادرست;  // input node is still valid, keep going
        } وگرنه اگر (orRIsRedundant) {
            orp->replaceWithKeepDType(orp->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(افزودن_Deletep(orp), orp);
            بازگشت نادرست;  // input node is still valid, keep going
        } وگرنه {
            بازگشت نادرست;
        }
    }
    منطقی matchMaskedShift(AstAnd* nodep) {
        // Drop redundant masking of right shift result. E.g: 0xff & ((uint32_t)a >> 24). This
        // commonly appears after V3Expand and the simplification in matchMaskedOr. Similarly,
        // drop redundant masking of left shift result. E.g.: 0xff000000 & ((uint32_t)a << 24).

        ثابت auto checkMask = [nodep, این](ثابت V3_عدد& mask) -> منطقی {
            ثابت AstConst* ثابت constp = VN_AS(nodep->lhsp(), Const);
            اگر (constp->رقم().isCaseEq(mask)) {
                Ast_گره* ثابت rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                nodep->replaceWithKeepDType(rhsp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت درست;
            }
            بازگشت نادرست;
        };

        // Check if masking is redundant
        اگر (ثابت AstShiftR* ثابت shiftp = VN_CAST(nodep->rhsp(), ShiftR)) {
            اگر (ثابت AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_عدد maskLo{nodep, nodep->width()};
                maskLo.setMask(nodep->width() - scp->رقم().toUInt());
                بازگشت checkMask(maskLo);
            }
        } وگرنه اگر (ثابت AstShiftL* ثابت shiftp = VN_CAST(nodep->rhsp(), ShiftL)) {
            اگر (ثابت AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_عدد mask{nodep, nodep->width()};
                ثابت uint32_t shiftAmount = scp->رقم().toUInt();
                mask.setMask(nodep->width() - shiftAmount, shiftAmount);
                بازگشت checkMask(mask);
            }
        }
        بازگشت نادرست;
    }

    منطقی matchMaskedZero(ثابت AstAnd* nodep) {
        // Turn masking of known zero bits into constant zero. Commonly appears after V3Expand.
        ثابت AstConst* ثابت maskp = VN_AS(nodep->lhsp(), Const);
        ثابت Ast_گره_عبارت* ثابت rhsp = nodep->rhsp();
        ثابت uint32_t msbP1 = maskp->رقم().mostSetBitP1();
        اگر (!msbP1) بازگشت نادرست;  // Don't rewrite, separate rule matches for this
        ثابت uint32_t msb = msbP1 - 1;
        ثابت uint32_t lsb = maskp->رقم().leastSetBitP1() - 1;

        اگر (ثابت AstShiftL* ثابت shiftp = VN_CAST(rhsp, ShiftL)) {
            // 'a << S' forces the low S bits to zero
            اگر (AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                بازگشت scp->رقم().fitsInUInt()  //
                       && (scp->رقم().toUInt() > msb);
            }
        }
        اگر (ثابت AstShiftR* ثابت shiftp = VN_CAST(rhsp, ShiftR)) {
            // 'a >> S' forces the high S bits to zero. Check against the width of the shifted
            // operand, V3Expand can create shifts wider than their inputs. Must use width(),
            // not widthMin(), as bits above widthMin() are not guaranteed to be zero.
            اگر (AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                بازگشت scp->رقم().fitsInUInt()
                       && (lsb + scp->رقم().toUInt()
                           >= static_cast<uint32_t>(shiftp->lhsp()->width()));
            }
        }
        اگر (ثابت AstMul* ثابت mulp = VN_CAST(rhsp, Mul)) {
            // 'C * a' forces the low N bits to zero where 'C' has low zero bits
            اگر (AstConst* ثابت cp = VN_CAST(mulp->lhsp(), Const)) {
                بازگشت cp->رقم().leastSetBitP1() > msb + 1;
            }
        }
        اگر (ثابت AstExtend* ثابت extendp = VN_CAST(rhsp, Extend)) {
            // Zero-extension forces the bits above the source width to zero
            بازگشت lsb >= static_cast<uint32_t>(extendp->lhsp()->width());
        }

        بازگشت نادرست;
    }

    منطقی match_Bit_Op_درخت(Ast_گره_عبارت* nodep) {
        اگر (nodep->widthMin() != 1) بازگشت نادرست;
        اگر (!v3Global.opt.f_Const_Bit_Op_درخت()) بازگشت نادرست;

        ثابت صحیح width = nodep->width();
        Ast_گره_عبارت* rootp = nodep;
        unsigned externalOps = 0;
        // Reach past a plain making AND
        اگر (ثابت AstAnd* ثابت andp = VN_CAST(nodep, And)) {
            اگر (isConst(andp->lhsp(), 1)) {
                rootp = andp->rhsp();
                externalOps = 1;
            }
        }

        // Only optimize if rootp is in fact the root of a tree of identical
        // operations, otherwise analysis on an eventually unoptimizable unablanced
        // tree can go O(N^2) as we would re-analyze every time as we move up the
        // tree, repeatedly re-discover the sub-tree is not optimizable.
        اگر (rootp->type() == nodep->backp()->type()) بازگشت نادرست;

        std::string debugPrefix;
        اگر (debug() >= 9) {  // LCOV_EXCL_START
            ایستا صحیح s_c = 0;
            debugPrefix = "-  matchBitOpTree[" + std::to_string(++s_c) + "] ";
            nodep->dump_درخت(debugPrefix + "INPUT: ");
        }  // LCOV_EXCL_STOP

        Ast_گره_عبارت* ثابت newp = Const_Bit_Op_درخت_Visitor::simplify(rootp, width, externalOps,
                                                                  m_statBitOpReduction, m_ids);

        اگر (newp) {
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Transformed leaf of bit tree to " << newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }

        اگر (debug() >= 9) {  // LCOV_EXCL_START
            اگر (newp) {
                newp->dump_درخت(debugPrefix + "RESULT: ");
            } وگرنه {
                cout << debugPrefix << "not replaced\n";
            }
        }  // LCOV_EXCL_STOP

        بازگشت newp;
    }
    ایستا منطقی operandShiftSame(ثابت Ast_گره* nodep) {
        ثابت Ast_گره_Biop* ثابت np = VN_AS(nodep, گره_Biop);
        {
            ثابت AstShiftL* ثابت lp = VN_CAST(np->lhsp(), ShiftL);
            ثابت AstShiftL* ثابت rp = VN_CAST(np->rhsp(), ShiftL);
            اگر (lp && rp) {
                بازگشت (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        {
            ثابت AstShiftR* ثابت lp = VN_CAST(np->lhsp(), ShiftR);
            ثابت AstShiftR* ثابت rp = VN_CAST(np->rhsp(), ShiftR);
            اگر (lp && rp) {
                بازگشت (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        بازگشت نادرست;
    }
    منطقی operandHugeShiftL(ثابت Ast_گره_Biop* nodep) {
        بازگشت (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->رقم().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->رقم().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->width())))
                && nodep->lhsp()->isPure());
    }
    منطقی operandHugeShiftR(ثابت Ast_گره_Biop* nodep) {
        بازگشت (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->رقم().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->رقم().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->lhsp()->width())))
                && nodep->lhsp()->isPure()
                && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)));
    }
    منطقی operandIsTwo(ثابت Ast_گره* nodep) {
        ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
        اگر (!constp) بازگشت نادرست;  // not constant
        اگر (constp->رقم().isFourState()) بازگشت نادرست;  // four-state
        اگر (nodep->width() > VL_QUADSIZE) بازگشت نادرست;  // too wide
        اگر (nodep->isSigned() && constp->رقم().isNegative()) بازگشت نادرست;  // signed and negative
        بازگشت constp->toUQuad() == 2;
    }
    منطقی operandIsTwostate(ثابت Ast_گره* nodep) {
        بازگشت (VN_IS(nodep, Const) && !VN_AS(nodep, Const)->رقم().isFourState());
    }
    منطقی operandIsPowTwo(ثابت Ast_گره* nodep) {
        اگر (!operandIsTwostate(nodep)) بازگشت نادرست;
        بازگشت (1 == VN_AS(nodep, Const)->رقم().شمار_Ones());
    }
    منطقی operandShiftOp(ثابت Ast_گره_Biop* nodep) {
        اگر (!VN_IS(nodep->rhsp(), Const)) بازگشت نادرست;
        ثابت Ast_گره_Biop* ثابت lhsp = VN_CAST(nodep->lhsp(), گره_Biop);
        اگر (!lhsp || !(VN_IS(lhsp, And) || VN_IS(lhsp, Or) || VN_IS(lhsp, Xor))) بازگشت نادرست;
        اگر (nodep->width() != lhsp->width()) بازگشت نادرست;
        اگر (nodep->width() != lhsp->lhsp()->width()) بازگشت نادرست;
        اگر (nodep->width() != lhsp->rhsp()->width()) بازگشت نادرست;

        // Only do it if we can fruther reduce one of the sides if pulling the shift through:
        // - Either operands are constants
        اگر (VN_IS(lhsp->lhsp(), Const)) بازگشت درست;
        اگر (VN_IS(lhsp->rhsp(), Const)) بازگشت درست;
        // - Const shift on either side
        اگر (Ast_گره_Biop* ثابت llp = VN_CAST(lhsp->lhsp(), گره_Biop)) {
            بازگشت (VN_IS(llp, ShiftL) || VN_IS(llp, ShiftR)) && VN_IS(llp->rhsp(), Const);
        }
        اگر (Ast_گره_Biop* ثابت lrp = VN_CAST(lhsp->rhsp(), گره_Biop)) {
            بازگشت (VN_IS(lrp, ShiftL) || VN_IS(lrp, ShiftR)) && VN_IS(lrp->rhsp(), Const);
        }
        // Otherwise we would increase logic size
        بازگشت نادرست;
    }
    منطقی operandShiftShift(ثابت Ast_گره_Biop* nodep) {
        // We could add a AND though.
        ثابت Ast_گره_Biop* ثابت lhsp = VN_CAST(nodep->lhsp(), گره_Biop);
        اگر (!lhsp || !(VN_IS(lhsp, ShiftL) || VN_IS(lhsp, ShiftR))) بازگشت نادرست;
        // We can only get rid of a<<b>>c or a<<b<<c, with constant b & c
        // because bits may be masked in that process, or (b+c) may exceed the word width.
        اگر (!(VN_IS(nodep->rhsp(), Const) && VN_IS(lhsp->rhsp(), Const))) بازگشت نادرست;
        اگر (VN_AS(nodep->rhsp(), Const)->رقم().isFourState()
            || VN_AS(lhsp->rhsp(), Const)->رقم().isFourState())
            بازگشت نادرست;
        اگر (nodep->width() != lhsp->width()) بازگشت نادرست;
        اگر (nodep->width() != lhsp->lhsp()->width()) بازگشت نادرست;
        بازگشت درست;
    }
    منطقی operandWordOOB(ثابت AstWordSel* nodep) {
        // V3Expand may make a arraysel that exceeds the bounds of the array
        // It was an expression, then got constified.  In reality, the WordSel
        // must be wrapped in a Cond, that will be false.
        بازگشت (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), گره_Var_Ref)
                && VN_AS(nodep->fromp(), گره_Var_Ref)->access().isReadOnly()
                && (static_cast<صحیح>(VN_AS(nodep->bitp(), Const)->toUInt())
                    >= VN_AS(nodep->fromp(), گره_Var_Ref)->varp()->width_واژگان()));
    }
    منطقی operandSelFull(ثابت AstSel* nodep) {
        بازگشت (VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0
                && static_cast<صحیح>(nodep->widthConst()) == nodep->fromp()->width());
    }
    منطقی operandSelExtend(AstSel* nodep) {
        اگر (!m_doV) بازگشت نادرست;
        AstExtend* ثابت extendp = VN_CAST(nodep->fromp(), Extend);
        اگر (!extendp) بازگشت نادرست;
        AstConst* ثابت lsbp = VN_CAST(nodep->lsbp(), Const);
        اگر (!lsbp) بازگشت نادرست;
        ثابت صحیح width = nodep->widthConst();
        ثابت صحیح lsb = lsbp->toSInt();
        ثابت صحیح msb = lsb + width - 1;
        Ast_گره_عبارت* ثابت lhsp = extendp->lhsp();
        اگر (!lhsp->isPure()) بازگشت نادرست;

        // Selecting the entire extended expression, replace with that
        اگر (lsb == 0 && msb == lhsp->width() - 1) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(lhsp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت درست;
        }
        // Select entirely in the extended part - replace with zero
        اگر (lsb >= lhsp->width()) {
            replaceZero(nodep);
            بازگشت درست;
        }
        // Select entirely in the extended expression - replace with select from that
        اگر (msb < lhsp->width()) {
            lhsp->unlinkFrBack();
            lsbp->unlinkFrBack();
            nodep->replaceWithKeepDType(جدید AstSel{nodep->fileline(), lhsp, lsbp, width});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت درست;
        }
        // Select straddles both sides, but is just a shorter extend
        اگر (lsb == 0) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(جدید AstExtend{nodep->fileline(), lhsp});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت درست;
        }

        بازگشت نادرست;
    }
    منطقی operandSelBiLower(AstSel* nodep) {
        // SEL(ADD(a,b),(width-1),0) -> ADD(SEL(a),SEL(b))
        // Add or any operation which doesn't care if we discard top bits
        Ast_گره_Biop* ثابت bip = VN_CAST(nodep->fromp(), گره_Biop);
        اگر (!(m_doV && bip && VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0)) بازگشت نادرست;
        UINFOTREE(9, nodep, "", "SEL(BI)-in");
        Ast_گره_عبارت* ثابت bilhsp = bip->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت birhsp = bip->rhsp()->unlinkFrBack();
        bip->lhsp(جدید AstSel{nodep->fileline(), bilhsp, 0, nodep->widthConst()});
        bip->rhsp(جدید AstSel{nodep->fileline(), birhsp, 0, nodep->widthConst()});
        UINFOTREE(9, bip, "", "SEL(BI)-ou");
        VL_DO_DANGLING(replaceWChild(nodep, bip), nodep);
        بازگشت درست;
    }
    منطقی operandSelShiftLower(AstSel* nodep) {
        // AND({a}, SHIFTR({b}, {c})) is often shorthand in C for Verilog {b}[{c} :+ {a}]
        // becomes thought other optimizations
        // SEL(SHIFTR({a},{b}),{lsb},{width}) -> SEL({a},{lsb+b},{width})
        ثابت AstShiftR* ثابت shiftp = VN_CAST(nodep->fromp(), ShiftR);
        اگر (!(m_doV && shiftp && VN_IS(shiftp->rhsp(), Const) && VN_IS(nodep->lsbp(), Const))) {
            بازگشت نادرست;
        }
        Ast_گره_عبارت* ثابت ap = shiftp->lhsp();
        ثابت AstConst* ثابت bp = VN_AS(shiftp->rhsp(), Const);
        ثابت AstConst* ثابت lp = VN_AS(nodep->lsbp(), Const);
        اگر (bp->isWide() || bp->رقم().isFourState() || bp->رقم().isNegative() || lp->isWide()
            || lp->رقم().isFourState() || lp->رقم().isNegative()) {
            بازگشت نادرست;
        }
        ثابت صحیح newLsb = lp->toSInt() + bp->toSInt();
        اگر (newLsb + nodep->widthConst() > ap->width()) بازگشت نادرست;
        //
        UINFO(9, "SEL(SHIFTR(a,b),l,w) -> SEL(a,l+b,w)");
        UINFOTREE(9, nodep, "", "SEL(SH)-in");
        AstSel* ثابت newp
            = جدید AstSel{nodep->fileline(), ap->unlinkFrBack(), newLsb, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        UINFOTREE(9, newp, "", "SEL(SH)-ou");
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        بازگشت درست;
    }

    منطقی operandBiExtendConstShrink(Ast_گره_Biop* nodep) {
        // Loop unrolling favors standalone compares
        // EQ(const{width32}, EXTEND(xx{width3})) -> EQ(const{3}, xx{3})
        // The constant must have zero bits (+ 1 if signed) or compare
        // would be incorrect. See also operandBiExtendConst
        AstExtend* ثابت extendp = VN_CAST(nodep->rhsp(), Extend);
        اگر (!extendp) بازگشت نادرست;
        Ast_گره_عبارت* ثابت smallerp = extendp->lhsp();
        ثابت صحیح subsize = smallerp->width();
        AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
        اگر (!constp) بازگشت نادرست;
        اگر (!constp->رقم().isBitsZero(constp->width() - 1, subsize)) بازگشت نادرست;
        //
        UINFOTREE(9, nodep, "", "BI(EXTEND)-in");
        smallerp->unlinkFrBack();
        VL_DO_DANGLING(افزودن_Deletep(extendp->unlinkFrBack()), extendp);  // aka nodep->lhsp.
        nodep->rhsp(smallerp);

        constp->unlinkFrBack();
        ثابت V3_عدد رقم{constp, subsize, constp->رقم()};
        nodep->lhsp(جدید AstConst{constp->fileline(), رقم});
        VL_DO_DANGLING(افزودن_Deletep(constp), constp);
        بازگشت نادرست;  // input node is still valid, keep going
    }
    منطقی operandBiExtendConstOver(ثابت Ast_گره_Biop* nodep) {
        // EQ(const{width32}, EXTEND(xx{width3})) -> constant
        // When the constant has non-zero bits above the extend it's a constant.
        // Avoids compiler warning
        ثابت AstExtend* ثابت extendp = VN_CAST(nodep->rhsp(), Extend);
        اگر (!extendp) بازگشت نادرست;
        ثابت Ast_گره* ثابت smallerp = extendp->lhsp();
        ثابت صحیح subsize = smallerp->width();
        ثابت AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
        اگر (!constp) بازگشت نادرست;
        اگر (constp->رقم().isBitsZero(constp->width() - 1, subsize)) بازگشت نادرست;
        بازگشت درست;
    }

    // Extraction checks
    منطقی warnSelect(AstSel* nodep) {
        اگر (m_doGenerate) {
            // Never checked yet
            V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);  // May need "constifying"
        }
        // Find range of dtype we are selecting from
        // Similar code in V3Unknown::AstSel
        ثابت منطقی doit = درست;
        اگر (m_warn && VN_IS(nodep->lsbp(), Const) && doit) {
            ثابت صحیح maxDeclBit = nodep->declRange().hiMaxSelect() * nodep->declElWidth()
                                   + (nodep->declElWidth() - 1);
            اگر (VN_AS(nodep->lsbp(), Const)->رقم().isFourState()) {
                nodep->v3error("Selection index is constantly unknown or tristated: "
                               "lsb="
                               << nodep->lsbp()->name() << " width=" << nodep->widthConst());
                // Replacing nodep will make a mess above, so we replace the offender
                replaceZero(nodep->lsbp());
            } وگرنه اگر (nodep->declRange().ranged()
                       && (nodep->msbConst() > maxDeclBit || nodep->lsbConst() > maxDeclBit)) {
                // See also warning in V3Width
                // Must adjust by element width as declRange() is in number of elements
                string msbLsbProtected;
                اگر (nodep->declElWidth() == 0) {
                    msbLsbProtected = "(nodep->declElWidth() == 0) "
                                      + std::to_string(nodep->msbConst()) + ":"
                                      + std::to_string(nodep->lsbConst());
                } وگرنه {
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
        بازگشت نادرست;  // Not a transform, so NOP
    }

    ایستا منطقی operandsSame(ثابت Ast_گره* node1p, ثابت Ast_گره* node2p) {
        // For now we just detect constants & simple vars, though it could be more generic
        اگر (ثابت AstConst* ثابت const1p = VN_CAST(node1p, Const)) {
            اگر (ثابت AstConst* ثابت const2p = VN_CAST(node2p, Const)) {
                بازگشت V3_عدد{node1p->fileline(), 1, 0}
                    .opEq(const1p->رقم(), const2p->رقم())
                    .isEqOne();
            }
        }
        اگر (VN_IS(node1p, VarRef) && VN_IS(node2p, VarRef)) {
            // Avoid comparing widthMin's, which results in lost optimization attempts
            // If cleanup sameGateTree to be smarter, this can be restored.
            // return node1p->sameGateTree(node2p);
            بازگشت node1p->isSame(node2p);
        }
        // Pattern created by coverage-line/branch; avoid compiler tautological-compare warning
        اگر (ثابت AstAnd* ثابت and1p = VN_CAST(node1p, And)) {
            اگر (ثابت AstAnd* ثابت and2p = VN_CAST(node2p, And)) {
                اگر (VN_IS(and1p->lhsp(), Const) && VN_IS(and1p->rhsp(), گره_Var_Ref)
                    && VN_IS(and2p->lhsp(), Const) && VN_IS(and2p->rhsp(), گره_Var_Ref))
                    بازگشت node1p->same_Gate_درخت(node2p);
            }
        }
        بازگشت نادرست;
    }
    منطقی ifSameAssign(ثابت Ast_گره_If* nodep) {
        ثابت Ast_گره_Assign* ثابت thensp = VN_CAST(nodep->thensp(), گره_Assign);
        ثابت Ast_گره_Assign* ثابت elsesp = VN_CAST(nodep->elsesp(), گره_Assign);
        اگر (!thensp || thensp->nextp()) بازگشت نادرست;  // Must be SINGLE statement
        اگر (!elsesp || elsesp->nextp()) بازگشت نادرست;
        اگر (thensp->type() != elsesp->type()) بازگشت نادرست;  // Can't mix an assigndly with assign
        اگر (!thensp->lhsp()->same_Gate_درخت(elsesp->lhsp())) بازگشت نادرست;
        اگر (!thensp->rhsp()->gate_درخت()) بازگشت نادرست;
        اگر (!elsesp->rhsp()->gate_درخت()) بازگشت نادرست;
        // Must not create an expression with unpacked array type
        اگر (VN_IS(thensp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) بازگشت نادرست;
        اگر (VN_IS(elsesp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) بازگشت نادرست;
        اگر (m_underRecFunc) بازگشت نادرست;  // This optimization may lead to infinite recursion
        اگر (m_doCpp) {  // Do not combine ifs cretaed by V3Premit
            اگر (VN_IS(thensp->lhsp(), VarRef) && thensp->lhsp()->isWide()) بازگشت نادرست;
        }
        // Only do it if not calls and both pure, otherwise undoes V3LiftExpr
        بازگشت !VN_IS(thensp->rhsp(), گره_FTask_Ref)  //
               && !VN_IS(elsesp->rhsp(), گره_FTask_Ref)  //
               && thensp->rhsp()->isPure()  //
               && elsesp->rhsp()->isPure();
    }
    منطقی operandIfIf(ثابت Ast_گره_If* nodep) {
        اگر (nodep->elsesp()) بازگشت نادرست;
        ثابت Ast_گره_If* ثابت lowerIfp = VN_CAST(nodep->thensp(), گره_If);
        اگر (!lowerIfp || lowerIfp->nextp()) بازگشت نادرست;
        اگر (nodep->type() != lowerIfp->type()) بازگشت نادرست;
        اگر (Ast_گره::afterCommentp(lowerIfp->elsesp())) بازگشت نادرست;
        بازگشت درست;
    }
    منطقی ifConcatMergeableBiop(ثابت Ast_گره* nodep) {
        بازگشت (VN_IS(nodep, And) || VN_IS(nodep, Or) || VN_IS(nodep, Xor));
    }
    منطقی ifAdjacentSel(ثابت AstSel* lhsp, ثابت AstSel* rhsp) {
        اگر (!v3Global.opt.fAssemble()) بازگشت نادرست;  // opt disabled
        اگر (!lhsp || !rhsp) بازگشت نادرست;
        ثابت Ast_گره* ثابت lfromp = lhsp->fromp();
        ثابت Ast_گره* ثابت rfromp = rhsp->fromp();
        اگر (!lfromp || !rfromp || !lfromp->same_Gate_درخت(rfromp)) بازگشت نادرست;
        ثابت AstConst* ثابت lstart = VN_CAST(lhsp->lsbp(), Const);
        ثابت AstConst* ثابت rstart = VN_CAST(rhsp->lsbp(), Const);
        اگر (!lstart || !rstart) بازگشت نادرست;  // too complicated
        ثابت صحیح rend = (rstart->toSInt() + rhsp->widthConst());
        بازگشت (rend == lstart->toSInt());
    }
    منطقی ifMergeAdjacent(ثابت Ast_گره_عبارت* lhsp, ثابت Ast_گره_عبارت* rhsp) {
        // Called by concatmergeable to determine if {lhsp, rhsp} make sense
        اگر (!v3Global.opt.fAssemble()) بازگشت نادرست;  // opt disabled
        // Two same varref
        اگر (operandsSame(lhsp, rhsp)) بازگشت درست;
        ثابت AstSel* ثابت lselp = VN_CAST(lhsp, Sel);
        ثابت AstSel* ثابت rselp = VN_CAST(rhsp, Sel);
        اگر (!lselp && !rselp) بازگشت نادرست;
        اگر (lselp && !VN_IS(lselp->lsbp(), Const)) بازگشت نادرست;
        اگر (rselp && !VN_IS(rselp->lsbp(), Const)) بازگشت نادرست;
        // a[i:j] {a[j-1:k], b}
        اگر (lselp && !rselp && VN_IS(rhsp, Concat))
            بازگشت ifMergeAdjacent(lhsp, VN_AS(rhsp, Concat)->lhsp());
        // {b, a[j:k]} a[k-1:i]
        اگر (rselp && !lselp && VN_IS(lhsp, Concat))
            بازگشت ifMergeAdjacent(VN_AS(lhsp, Concat)->rhsp(), rhsp);
        // a a[msb:j]
        ثابت Ast_گره_عبارت* ثابت lfromp = lselp                                ? lselp->fromp()
                                          : lhsp->same_Gate_درخت(rselp->fromp()) ? lhsp
                                                                               : nullptr;
        // a[i:0] a
        ثابت Ast_گره_عبارت* ثابت rfromp = rselp                                ? rselp->fromp()
                                          : rhsp->same_Gate_درخت(lselp->fromp()) ? rhsp
                                                                               : nullptr;
        اگر (!lfromp || !rfromp || !lfromp->same_Gate_درخت(rfromp)) بازگشت نادرست;

        ثابت int32_t lstart = lselp ? lselp->lsbConst() : 0;
        ثابت int32_t rstart = rselp ? rselp->lsbConst() : 0;
        ثابت int32_t rend = rstart + (rselp ? rselp->widthConst() : rhsp->width());
        // a[i:j] a[j-1:k]
        اگر (rend == lstart) بازگشت درست;
        // a[i:0] a[msb:j]
        اگر (rend == rfromp->width() && lstart == 0) بازگشت درست;
        بازگشت نادرست;
    }
    منطقی concatMergeable(ثابت Ast_گره_عبارت* lhsp, ثابت Ast_گره_عبارت* rhsp, unsigned depth) {
        // Determine if {a OP b, c OP d} => {a, c} OP {b, d} is advantageous
        اگر (!v3Global.opt.fAssemble()) بازگشت نادرست;  // opt disabled
        اگر (lhsp->type() != rhsp->type()) بازگشت نادرست;
        اگر (!ifConcatMergeableBiop(lhsp)) بازگشت نادرست;
        اگر (depth > CONCAT_MERGABLE_MAX_DEPTH) بازگشت نادرست;  // As worse case O(n^2) algorithm

        ثابت Ast_گره_Biop* ثابت lp = VN_CAST(lhsp, گره_Biop);
        ثابت Ast_گره_Biop* ثابت rp = VN_CAST(rhsp, گره_Biop);
        اگر (!lp || !rp) بازگشت نادرست;
        // {a[]&b[], a[]&b[]}
        ثابت منطقی lad = ifMergeAdjacent(lp->lhsp(), rp->lhsp());
        ثابت منطقی rad = ifMergeAdjacent(lp->rhsp(), rp->rhsp());
        اگر (lad && rad) بازگشت درست;
        // {a[] & b[]&c[], a[] & b[]&c[]}
        اگر (lad && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) بازگشت درست;
        // {a[]&b[] & c[], a[]&b[] & c[]}
        اگر (rad && concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)) بازگشت درست;
        // {(a[]&b[])&(c[]&d[]), (a[]&b[])&(c[]&d[])}
        اگر (concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)
            && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) {
            بازگشت درست;
        }
        بازگشت نادرست;
    }
    ایستا منطقی operandsSameWidth(ثابت Ast_گره* lhsp, ثابت Ast_گره* rhsp) {
        بازگشت lhsp->width() == rhsp->width();
    }

    //----------------------------------------
    // Constant Replacement functions.
    // These all take a node, delete its tree, and replaces it with a constant

    تهی replace_رقم(Ast_گره* oldp, ثابت V3_عدد& رقم, ثابت string& origParamName = "") {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        UASSERT_OBJ(!(VN_IS(oldp, Const) && !VN_AS(oldp, Const)->رقم().isFourState()), oldp,
                    "Already constant??");
        AstConst* ثابت newp = جدید AstConst{oldp->fileline(), رقم};
        اگر (!origParamName.empty()) newp->origParamName(origParamName);
        oldp->replaceWithKeepDType(newp);
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        VL_DO_DANGLING(افزودن_Deletep(oldp), oldp);
    }
    تهی replace_رقم(Ast_گره* nodep, uint32_t val) {
        ثابت V3_عدد رقم{nodep, nodep->width(), val};
        VL_DO_DANGLING(replace_رقم(nodep, رقم), nodep);
    }
    تهی replace_رقم_Signed(Ast_گره_Biop* nodep, uint32_t val) {
        // We allow both sides to be constant, as one may have come from
        // parameter propagation, etc.
        اگر (m_warn && !(VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const))) {
            nodep->v3warn(UNSIGNED, "Comparison is constant due to unsigned arithmetic");
        }
        VL_DO_DANGLING(replace_رقم(nodep, val), nodep);
    }
    تهی replace_رقم_Limited(Ast_گره_Biop* nodep, uint32_t val) {
        // Avoids gcc warning about same
        اگر (m_warn) nodep->v3warn(CMPCONST, "Comparison is constant due to limited range");
        VL_DO_DANGLING(replace_رقم(nodep, val), nodep);
    }
    تهی replaceZero(Ast_گره* nodep) { VL_DO_DANGLING(replace_رقم(nodep, 0), nodep); }
    تهی replaceZeroChkPure(Ast_گره* nodep, Ast_گره_عبارت* checkp) {
        // For example, "0 * n" -> 0 if n has no side effects
        // Else strength reduce it to 0 & n.
        // If ever change the operation note AstAnd rule specially ignores this created pattern
        اگر (checkp->isPure()) {
            VL_DO_DANGLING(replace_رقم(nodep, 0), nodep);
        } وگرنه {
            Ast_گره* ثابت newp = جدید AstAnd{nodep->fileline(), جدید AstConst{nodep->fileline(), 0},
                                             checkp->unlinkFrBack()};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    تهی replaceAllOnes(Ast_گره* nodep) {
        V3_عدد ones{nodep, nodep->width(), 0};
        ones.setMask(nodep->width());
        VL_DO_DANGLING(replace_رقم(nodep, ones), nodep);
    }
    تهی replaceConst(Ast_گره_Uniop* nodep) {
        V3_عدد numv{nodep, nodep->widthMinV()};
        nodep->عدد_Operate(numv, const_رقم_V(nodep->lhsp()));
        ثابت V3_عدد& رقم = to_رقم_C(nodep, numv);
        UINFO(4, "UNICONST -> " << رقم);
        VL_DO_DANGLING(replace_رقم(nodep, رقم), nodep);
    }
    تهی replaceConst(Ast_گره_Biop* nodep) {
        V3_عدد numv{nodep, nodep->widthMinV()};
        nodep->عدد_Operate(numv, const_رقم_V(nodep->lhsp()), const_رقم_V(nodep->rhsp()));
        ثابت V3_عدد& رقم = to_رقم_C(nodep, numv);
        UINFO(4, "BICONST -> " << رقم);
        VL_DO_DANGLING(replace_رقم(nodep, رقم), nodep);
    }
    تهی replaceConst(Ast_گره_Triop* nodep) {
        V3_عدد numv{nodep, nodep->widthMinV()};
        nodep->عدد_Operate(numv, const_رقم_V(nodep->lhsp()), const_رقم_V(nodep->rhsp()),
                             const_رقم_V(nodep->thsp()));
        ثابت V3_عدد& رقم = to_رقم_C(nodep, numv);
        UINFO(4, "TRICONST -> " << رقم);
        VL_DO_DANGLING(replace_رقم(nodep, رقم), nodep);
    }
    تهی replaceConst(Ast_گره_Quadop* nodep) {
        V3_عدد numv{nodep, nodep->widthMinV()};
        nodep->عدد_Operate(numv, const_رقم_V(nodep->lhsp()), const_رقم_V(nodep->rhsp()),
                             const_رقم_V(nodep->thsp()), const_رقم_V(nodep->fhsp()));
        ثابت V3_عدد& رقم = to_رقم_C(nodep, numv);
        UINFO(4, "QUADCONST -> " << رقم);
        VL_DO_DANGLING(replace_رقم(nodep, رقم), nodep);
    }

    تهی replaceConstString(Ast_گره* oldp, ثابت string& رقم) {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        Ast_گره* ثابت newp = جدید AstConst{oldp->fileline(), AstConst::String{}, رقم};
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        oldp->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(oldp), oldp);
    }
    //----------------------------------------
    // Replacement functions.
    // These all take a node and replace it with something else

    تهی replaceWChild(Ast_گره* nodep, Ast_گره_عبارت* childp) {
        // NODE(..., CHILD(...)) -> CHILD(...)
        childp->unlinkFrBackWithNext();
        // If replacing a SEL for example, the data type comes from the parent (is less wide).
        // This may adversely affect the operation of the node being replaced.
        nodep->replaceWithKeepDType(childp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی replaceWChildBool(Ast_گره* nodep, Ast_گره_عبارت* childp) {
        // NODE(..., CHILD(...)) -> REDOR(CHILD(...))
        childp->unlinkFrBack();
        اگر (childp->width1()) {
            nodep->replaceWithKeepDType(childp);
        } وگرنه {
            nodep->replaceWithKeepDType(جدید AstRedOr{childp->fileline(), childp});
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    //! Replace a ternary node with its RHS after iterating
    //! Used with short-circuiting, where the RHS has not yet been iterated.
    تهی replaceWIteratedRhs(Ast_گره_Triop* nodep) {
        اگر (Ast_گره* ثابت rhsp = nodep->rhsp()) iterateAndNextNull(rhsp);
        replaceWChild(nodep, nodep->rhsp());  // May have changed
    }

    //! Replace a ternary node with its THS after iterating
    //! Used with short-circuiting, where the THS has not yet been iterated.
    تهی replaceWIteratedThs(Ast_گره_Triop* nodep) {
        اگر (Ast_گره* ثابت thsp = nodep->thsp()) iterateAndNextNull(thsp);
        replaceWChild(nodep, nodep->thsp());  // May have changed
    }
    تهی replaceWLhs(Ast_گره_Uniop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    تهی replaceWLhs(Ast_گره_Biop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    تهی replaceWRhs(Ast_گره_Biop* nodep) {
        // Keep RHS, remove LHS
        replaceWChild(nodep, nodep->rhsp());
    }
    تهی replaceWLhsBool(Ast_گره_Biop* nodep) { replaceWChildBool(nodep, nodep->lhsp()); }
    تهی replaceWRhsBool(Ast_گره_Biop* nodep) { replaceWChildBool(nodep, nodep->rhsp()); }
    تهی replaceAsv(Ast_گره_Biop* nodep) {
        // BIASV(CONSTa, BIASV(CONSTb, c)) -> BIASV( BIASV_CONSTED(a,b), c)
        // BIASV(SAMEa,  BIASV(SAMEb, c))  -> BIASV( BIASV(SAMEa,SAMEb), c)
        // UINFOTREE(1, nodep, "", "repAsvConst_old");
        Ast_گره_عبارت* ثابت ap = nodep->lhsp();
        Ast_گره_Biop* ثابت rp = VN_AS(nodep->rhsp(), گره_Biop);
        Ast_گره_عبارت* ثابت bp = rp->lhsp();
        Ast_گره_عبارت* ثابت cp = rp->rhsp();
        ap->unlinkFrBack();
        bp->unlinkFrBack();
        cp->unlinkFrBack();
        rp->unlinkFrBack();
        nodep->lhsp(rp);
        nodep->rhsp(cp);
        rp->lhsp(ap);
        rp->rhsp(bp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        اگر (VN_IS(rp->lhsp(), Const) && VN_IS(rp->rhsp(), Const)) replaceConst(rp);
        iterate(nodep);  // Proceed to fixed point
    }
    تهی replaceAsvLUp(Ast_گره_Biop* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r))
        Ast_گره_Biop* ثابت lp = VN_AS(nodep->lhsp()->unlinkFrBack(), گره_Biop);
        Ast_گره_عبارت* ثابت llp = lp->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت lrp = lp->rhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت rp = nodep->rhsp()->unlinkFrBack();
        nodep->lhsp(llp);
        nodep->rhsp(lp);
        lp->lhsp(lrp);
        lp->rhsp(rp);
        lp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    تهی replaceAsvRUp(Ast_گره_Biop* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr))
        Ast_گره_عبارت* ثابت lp = nodep->lhsp()->unlinkFrBack();
        Ast_گره_Biop* ثابت rp = VN_AS(nodep->rhsp()->unlinkFrBack(), گره_Biop);
        Ast_گره_عبارت* ثابت rlp = rp->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت rrp = rp->rhsp()->unlinkFrBack();
        nodep->lhsp(rlp);
        nodep->rhsp(rp);
        rp->lhsp(lp);
        rp->rhsp(rrp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    تهی replaceAndOr(Ast_گره_Biop* nodep) {
        //  OR  (AND (CONSTll,lr), AND(CONSTrl==ll,rr))    -> AND (CONSTll, OR(lr,rr))
        //  OR  (AND (CONSTll,lr), AND(CONSTrl,    rr=lr)) -> AND (OR(ll,rl), rr)
        // nodep ^lp  ^llp   ^lrp  ^rp  ^rlp       ^rrp
        // (Or/And may also be reversed)
        Ast_گره_Biop* ثابت lp = VN_AS(nodep->lhsp()->unlinkFrBack(), گره_Biop);
        Ast_گره_عبارت* ثابت llp = lp->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت lrp = lp->rhsp()->unlinkFrBack();
        Ast_گره_Biop* ثابت rp = VN_AS(nodep->rhsp()->unlinkFrBack(), گره_Biop);
        Ast_گره_عبارت* ثابت rlp = rp->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        اگر (operandsSame(llp, rlp)) {
            lp->lhsp(llp);
            lp->rhsp(nodep);
            lp->dtypeFrom(nodep);
            nodep->lhsp(lrp);
            nodep->rhsp(rrp);
            VL_DO_DANGLING(افزودن_Deletep(rp), rp);
            VL_DO_DANGLING(افزودن_Deletep(rlp), rlp);
        } وگرنه اگر (operandsSame(lrp, rrp)) {
            lp->lhsp(nodep);
            lp->rhsp(rrp);
            lp->dtypeFrom(nodep);
            nodep->lhsp(llp);
            nodep->rhsp(rlp);
            VL_DO_DANGLING(افزودن_Deletep(rp), rp);
            VL_DO_DANGLING(افزودن_Deletep(lrp), lrp);
        } وگرنه {
            nodep->v3fatalSrc("replaceAndOr on something operandAndOrSame shouldn't have matched");
        }
        // UINFOTREE(1, nodep, "", "repAndOr_new");
    }
    تهی replaceShiftSame(Ast_گره_Biop* nodep) {
        // Or(Shift(ll,CONSTlr),Shift(rl,CONSTrr==lr)) -> Shift(Or(ll,rl),CONSTlr)
        // (Or/And may also be reversed)
        Ast_گره_Biop* ثابت lp = VN_AS(nodep->lhsp()->unlinkFrBack(), گره_Biop);
        Ast_گره_عبارت* ثابت llp = lp->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت lrp = lp->rhsp()->unlinkFrBack();
        Ast_گره_Biop* ثابت rp = VN_AS(nodep->rhsp()->unlinkFrBack(), گره_Biop);
        Ast_گره_عبارت* ثابت rlp = rp->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        lp->lhsp(nodep);
        lp->rhsp(lrp);
        nodep->lhsp(llp);
        nodep->rhsp(rlp);
        nodep->dtypep(llp->dtypep());  // dtype of Biop is before shift.
        VL_DO_DANGLING(افزودن_Deletep(rp), rp);
        VL_DO_DANGLING(افزودن_Deletep(rrp), rrp);
        // UINFOTREE(1, nodep, "", "repShiftSame_new");
    }
    تهی replaceConcatSel(AstConcat* nodep) {
        // {a[1], a[0]} -> a[1:0]
        AstSel* ثابت lselp = VN_AS(nodep->lhsp()->unlinkFrBack(), Sel);
        AstSel* ثابت rselp = VN_AS(nodep->rhsp()->unlinkFrBack(), Sel);
        ثابت صحیح lstart = lselp->lsbConst();
        ثابت صحیح lwidth = lselp->widthConst();
        ثابت صحیح rstart = rselp->lsbConst();
        ثابت صحیح rwidth = rselp->widthConst();

        UASSERT_OBJ((rstart + rwidth) == lstart, nodep,
                    "tried to merge two selects which are not adjacent");
        AstSel* ثابت newselp = جدید AstSel{
            lselp->fromp()->fileline(), rselp->fromp()->unlinkFrBack(), rstart, lwidth + rwidth};
        UINFO(5, "merged two adjacent sel " << lselp << " and " << rselp << " to one " << newselp);

        nodep->replaceWithKeepDType(newselp);
        VL_DO_DANGLING(افزودن_Deletep(lselp), lselp);
        VL_DO_DANGLING(افزودن_Deletep(rselp), rselp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی replaceConcatMerge(AstConcat* nodep) {
        // {llp OP lrp, rlp OP rrp} => {llp, rlp} OP {lrp, rrp}, where OP = AND/OR/XOR
        Ast_گره_Biop* ثابت lp = VN_AS(nodep->lhsp(), گره_Biop);
        Ast_گره_Biop* ثابت rp = VN_AS(nodep->rhsp(), گره_Biop);
        اگر (concatMergeable(lp, rp, 0)) {
            Ast_گره_عبارت* ثابت llp = lp->lhsp();
            Ast_گره_عبارت* ثابت lrp = lp->rhsp();
            Ast_گره_عبارت* ثابت rlp = rp->lhsp();
            Ast_گره_عبارت* ثابت rrp = rp->rhsp();
            AstConcat* ثابت newlp = جدید AstConcat{rlp->fileline(), llp->clone_درخت_Pure(نادرست),
                                                   rlp->clone_درخت_Pure(نادرست)};
            AstConcat* ثابت newrp = جدید AstConcat{rrp->fileline(), lrp->clone_درخت_Pure(نادرست),
                                                   rrp->clone_درخت_Pure(نادرست)};
            // use the lhs to replace the parent concat
            llp->replaceWith(newlp);
            VL_DO_DANGLING(افزودن_Deletep(llp), llp);
            lrp->replaceWith(newrp);
            VL_DO_DANGLING(افزودن_Deletep(lrp), lrp);
            lp->dtypeChgWidthSigned(newlp->width(), newlp->width(), VSigning::UNSIGNED);
            UINFO(5, "merged " << nodep);
            ++m_statConcatMerge;
            VL_DO_DANGLING(افزودن_Deletep(rp->unlinkFrBack()), rp);
            nodep->replaceWithKeepDType(lp->unlinkFrBack());
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            iterate(lp->lhsp());
            iterate(lp->rhsp());
        } وگرنه {
            nodep->v3fatalSrc("tried to merge two Concat which are not adjacent");
        }
    }
    تهی replaceExtend(Ast_گره* nodep, Ast_گره_عبارت* arg0p) {
        // -> EXTEND(nodep)
        // like a AstExtend{$rhsp}, but we need to set the width correctly from base node
        arg0p->unlinkFrBack();
        Ast_گره_عبارت* ثابت newp
            = (VN_IS(nodep, ExtendS)
                   ? static_cast<Ast_گره_عبارت*>(جدید AstExtendS{nodep->fileline(), arg0p})
                   : static_cast<Ast_گره_عبارت*>(جدید AstExtend{nodep->fileline(), arg0p}));
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی replacePowShift(Ast_گره_Biop* nodep) {  // Pow or PowS
        UINFO(5, "POW(2,b)->SHIFTL(1,b) " << nodep);
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* ثابت newp
            = جدید AstShiftL{nodep->fileline(), جدید AstConst{nodep->fileline(), 1}, rhsp};
        newp->lhsp()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی replaceMulShift(AstMul* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "MUL(2^n,b)->SHIFTL(b,n) " << nodep);
        ثابت صحیح amount = VN_AS(nodep->lhsp(), Const)->رقم().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_گره_عبارت* ثابت opp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* ثابت newp
            = جدید AstShiftL{nodep->fileline(), opp, جدید AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی replaceDivShift(AstDiv* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "DIV(b,2^n)->SHIFTR(b,n) " << nodep);
        ثابت صحیح amount = VN_AS(nodep->rhsp(), Const)->رقم().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_گره_عبارت* ثابت opp = nodep->lhsp()->unlinkFrBack();
        AstShiftR* ثابت newp
            = جدید AstShiftR{nodep->fileline(), opp, جدید AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی replaceModAnd(AstModDiv* nodep) {  // Mod, but not ModS as not simple shift
        UINFO(5, "MOD(b,2^n)->AND(b,2^n-1) " << nodep);
        ثابت صحیح amount = VN_AS(nodep->rhsp(), Const)->رقم().mostSetBitP1() - 1;  // 2^n->n+1
        V3_عدد mask{nodep, nodep->width()};
        mask.setMask(amount);
        Ast_گره_عبارت* ثابت opp = nodep->lhsp()->unlinkFrBack();
        AstAnd* ثابت newp
            = جدید AstAnd{nodep->fileline(), opp, جدید AstConst{nodep->fileline(), mask}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی replaceShiftOp(Ast_گره_Biop* nodep) {
        UINFO(5, "SHIFT(AND(a,b),CONST) with a/b special ->AND(SHIFT(a,CONST),SHIFT(b,CONST)) "
                     << nodep);
        ثابت صحیح width = nodep->width();
        ثابت صحیح widthMin = nodep->widthMin();
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        Ast_گره_Biop* ثابت lhsp = VN_AS(nodep->lhsp(), گره_Biop);
        lhsp->unlinkFrBack();
        Ast_گره_عبارت* ثابت shiftp = nodep->rhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت ap = lhsp->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت bp = lhsp->rhsp()->unlinkFrBack();
        Ast_گره_Biop* ثابت shift1p = nodep;
        Ast_گره_Biop* ثابت shift2p = nodep->clone_درخت(درست);
        shift1p->lhsp(ap);
        shift1p->rhsp(shiftp->clone_درخت_Pure(درست));
        shift2p->lhsp(bp);
        shift2p->rhsp(shiftp);
        Ast_گره_Biop* ثابت newp = lhsp;
        newp->lhsp(shift1p);
        newp->rhsp(shift2p);
        newp->dtypeChgWidth(width, widthMin);  // The new AND must have width of the original SHIFT
        handle.relink(newp);
        iterate(newp);  // Further reduce, either node may have more reductions.
    }
    تهی replaceShiftShift(Ast_گره_Biop* nodep) {
        UINFO(4, "SHIFT(SHIFT(a,s1),s2)->SHIFT(a,ADD(s1,s2)) " << nodep);
        UINFOTREE(9, nodep, "", "repShiftShift_old");
        Ast_گره_Biop* ثابت lhsp = VN_AS(nodep->lhsp(), گره_Biop);
        lhsp->unlinkFrBack();
        Ast_گره_عبارت* ثابت ap = lhsp->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت shift1p = lhsp->rhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت shift2p = nodep->rhsp()->unlinkFrBack();
        // Shift1p and shift2p may have different sizes, both are
        // self-determined so sum with infinite width
        اگر (nodep->type() == lhsp->type()) {
            ثابت صحیح shift1 = VN_AS(shift1p, Const)->toUInt();
            ثابت صحیح shift2 = VN_AS(shift2p, Const)->toUInt();
            ثابت صحیح newshift = shift1 + shift2;
            VL_DO_DANGLING(افزودن_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(افزودن_Deletep(shift2p), shift2p);
            nodep->lhsp(ap);
            nodep->rhsp(جدید AstConst(nodep->fileline(), newshift));
            iterate(nodep);  // Further reduce, either node may have more reductions.
        } وگرنه {
            // We know shift amounts are constant, but might be a mixed left/right shift
            صحیح shift1 = VN_AS(shift1p, Const)->toUInt();
            اگر (VN_IS(lhsp, ShiftR)) shift1 = -shift1;
            صحیح shift2 = VN_AS(shift2p, Const)->toUInt();
            اگر (VN_IS(nodep, ShiftR)) shift2 = -shift2;
            ثابت صحیح newshift = shift1 + shift2;
            VL_DO_DANGLING(افزودن_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(افزودن_Deletep(shift2p), shift2p);
            Ast_گره_عبارت* newp;
            V3_عدد mask1{nodep, nodep->width()};
            V3_عدد ones{nodep, nodep->width()};
            ones.setMask(nodep->width());
            اگر (shift1 < 0) {
                mask1.opShiftR(ones, V3_عدد(nodep, VL_IDATASIZE, -shift1));
            } وگرنه {
                mask1.opShiftL(ones, V3_عدد(nodep, VL_IDATASIZE, shift1));
            }
            V3_عدد mask{nodep, nodep->width()};
            اگر (shift2 < 0) {
                mask.opShiftR(mask1, V3_عدد(nodep, VL_IDATASIZE, -shift2));
            } وگرنه {
                mask.opShiftL(mask1, V3_عدد(nodep, VL_IDATASIZE, shift2));
            }
            اگر (newshift < 0) {
                newp = جدید AstShiftR{nodep->fileline(), ap,
                                     جدید AstConst(nodep->fileline(), -newshift)};
            } وگرنه {
                newp = جدید AstShiftL{nodep->fileline(), ap,
                                     جدید AstConst(nodep->fileline(), newshift)};
            }
            newp->dtypeFrom(nodep);
            newp = جدید AstAnd{nodep->fileline(), newp, جدید AstConst{nodep->fileline(), mask}};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            // UINFOTREE(1, newp, "", "repShiftShift_new");
            iterate(newp);  // Further reduce, either node may have more reductions.
        }
        VL_DO_DANGLING(افزودن_Deletep(lhsp), lhsp);
    }

    منطقی replaceAssignMultiSel(Ast_گره_Assign* nodep) {
        // Multiple assignments to sequential bits can be concated
        // ASSIGN(SEL(a),aq), ASSIGN(SEL(a+1),bq) -> ASSIGN(SEL(a:b),CONCAT(aq,bq)
        // ie. assign var[2]=a, assign var[3]=b -> assign var[3:2]={b,a}

        // Skip if we're not const'ing an entire module (IE doing only one assign, etc)
        اگر (!m_modp) بازگشت نادرست;
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ثابت sel1p = VN_CAST(nodep->lhsp(), Sel);
        اگر (!sel1p) بازگشت نادرست;
        Ast_گره_Assign* ثابت nextp = VN_CAST(nodep->nextp(), گره_Assign);
        اگر (!nextp) بازگشت نادرست;
        اگر (nodep->type() != nextp->type()) بازگشت نادرست;
        ثابت AstSel* ثابت sel2p = VN_CAST(nextp->lhsp(), Sel);
        اگر (!sel2p) بازگشت نادرست;
        AstVarRef* ثابت varref1p = VN_CAST(sel1p->fromp(), VarRef);
        اگر (!varref1p) بازگشت نادرست;
        ثابت AstVarRef* ثابت varref2p = VN_CAST(sel2p->fromp(), VarRef);
        اگر (!varref2p) بازگشت نادرست;
        اگر (!varref1p->same_Gate_درخت(varref2p)) بازگشت نادرست;
        ثابت AstConst* ثابت con1p = VN_CAST(sel1p->lsbp(), Const);
        اگر (!con1p) بازگشت نادرست;
        ثابت AstConst* ثابت con2p = VN_CAST(sel2p->lsbp(), Const);
        اگر (!con2p) بازگشت نادرست;
        // We need to make sure there's no self-references involved in either
        // assignment.  For speed, we only look 3 deep, then give up.
        اگر (!varNotReferenced(nodep->rhsp(), varref1p->varp())) بازگشت نادرست;
        اگر (!varNotReferenced(nextp->rhsp(), varref2p->varp())) بازگشت نادرست;
        // If a variable is marked split_var, access to the variable should not be merged.
        اگر (varref1p->varp()->attrSplitVar() || varref2p->varp()->attrSplitVar()) بازگشت نادرست;
        // Swap?
        اگر ((con1p->toSInt() != con2p->toSInt() + sel2p->width())
            && (con2p->toSInt() != con1p->toSInt() + sel1p->width())) {
            بازگشت نادرست;
        }
        ثابت منطقی lsbFirstAssign = (con1p->toUInt() < con2p->toUInt());
        UINFO(4, "replaceAssignMultiSel " << nodep);
        UINFO(4, "                   && " << nextp);
        // UINFOTREE(1, nodep, "", "comb1");
        // UINFOTREE(1, nextp, "", "comb2");
        Ast_گره_عبارت* ثابت rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت rhs2p = nextp->rhsp()->unlinkFrBack();
        Ast_گره_Assign* newp;
        اگر (lsbFirstAssign) {
            newp = nodep->cloneType(جدید AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel1p->lsbConst(), sel1p->width() + sel2p->width()},
                                    جدید AstConcat{rhs1p->fileline(), rhs2p, rhs1p});
        } وگرنه {
            newp = nodep->cloneType(جدید AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel2p->lsbConst(), sel1p->width() + sel2p->width()},
                                    جدید AstConcat{rhs1p->fileline(), rhs1p, rhs2p});
        }
        // UINFOTREE(1, pnewp, "", "conew");
        nodep->replaceWith(newp);  // dypep intentionally changing
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        VL_DO_DANGLING(افزودن_Deletep(nextp->unlinkFrBack()), nextp);
        بازگشت درست;
    }

    منطقی replaceAssignSFormatF(Ast_گره_Assign* nodep) {
        // Rewrite 'x = sformatf(...)' into 'sformat(x, ...)', which is more efficient
        // at the call site as it does not need to check if 'x' needs to be freed first.
        // This is a somewhat common pattern after lowering assertions.
        اگر (!VN_IS(nodep, Assign)) بازگشت نادرست;
        AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef);
        اگر (!lhsp) بازگشت نادرست;
        AstSFormatF* ثابت fmtp = VN_CAST(nodep->rhsp(), SFormatF);
        اگر (!fmtp) بازگشت نادرست;
        lhsp->unlinkFrBack();
        fmtp->unlinkFrBack();
        nodep->replaceWith(جدید AstSFormat{nodep->fileline(), fmtp, lhsp});
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        بازگشت درست;
    }

    منطقی varNotReferenced(Ast_گره* nodep, AstVar* varp, صحیح level = 0) {
        // Return true if varp never referenced under node.
        // Return false if referenced, or tree too deep to be worth it, or side effects
        اگر (!nodep) بازگشت درست;
        اگر (level > 2) بازگشت نادرست;
        اگر (!nodep->isPure()) بازگشت نادرست;  // For example a $fgetc can't be reordered
        اگر (VN_IS(nodep, گره_Var_Ref) && VN_AS(nodep, گره_Var_Ref)->varp() == varp) بازگشت نادرست;
        بازگشت (varNotReferenced(nodep->nextp(), varp, level + 1)
                && varNotReferenced(nodep->op1p(), varp, level + 1)
                && varNotReferenced(nodep->op2p(), varp, level + 1)
                && varNotReferenced(nodep->op3p(), varp, level + 1)
                && varNotReferenced(nodep->op4p(), varp, level + 1));
    }

    منطقی replace_گره_Assign(Ast_گره_Assign* nodep) {
        اگر (VN_IS(nodep->lhsp(), VarRef) && VN_IS(nodep->rhsp(), VarRef)
            && VN_AS(nodep->lhsp(), VarRef)->sameNoLvalue(VN_AS(nodep->rhsp(), VarRef))
            && !VN_IS(nodep, AssignDly)) {
            // X = X.  Quite pointless, though X <= X may override another earlier assignment
            اگر (VN_IS(nodep, AssignW)) {
                اگر (!v3Global.opt.pedantic())
                    nodep->v3error(
                        "Wire inputs its own output, creating circular logic (wire x=x)");
                بازگشت نادرست;  // Don't delete the assign, or V3Gate will freak out
            } وگرنه {
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                بازگشت درست;
            }
        } وگرنه اگر (m_doV && VN_IS(nodep->lhsp(), Concat)) {
            منطقی need_temp = نادرست;
            // A blocking assignment with an intra-assignment timing control evaluates its value,
            // then waits once (IEEE 1800-2023 9.4.5), here to assign a temporary for the parts
            ثابت منطقی waits = VN_IS(nodep, Assign) && nodep->timingControlp();
            // Splitting copies the RHS once per part, so a large RHS would make the tree
            // grow with the product of the number of parts and the RHS size
            ثابت منطقی needTempPure = !nodep->rhsp()->isPure() || waits
                                      || nodep->rhsp()->isLargerThan(CONCAT_ASSIGN_MAX_RHS_NODES);
            اگر (m_warn && !VN_IS(nodep, AssignDly)
                && !needTempPure) {  // Is same var on LHS and RHS?
                // If the rhs is not pure, we need a temporary variable anyway
                std::unordered_set<size_t> lhsVarIds;
                nodep->lhsp()->foreach([&](ثابت AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    lhsVarIds.emplace(m_ids(refp->varp()));
                });
                nodep->rhsp()->foreach([&](ثابت AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    اگر (lhsVarIds.شمار(m_ids(refp->varp()))) need_temp = درست;
                });
            }
            اگر (needTempPure) {
                // if the RHS is impure or large we need to create a temporary variable for it,
                // because further handling involves copying of the RHS.
                UINFO(4, "  ASSITEMPPURE " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp,rhs),
                //                                ASSIGN(lc1,SEL(temp,{size1})),
                //                                ASSIGN(lc2,SEL(temp,{size2}))

                Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
                AstVar* ثابت tempPurep = جدید AstVar{rhsp->fileline(), VVarType::BLOCKTEMP,
                                                     m_concswapNames.get(rhsp), rhsp->dtypep()};
                // Static, as read right after written, also in a fork
                tempPurep->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempPurep);
                AstVarRef* ثابت tempPureRefp
                    = جدید AstVarRef{rhsp->fileline(), tempPurep, VAccess::WRITE};
                Ast_گره_Assign* ثابت asnp
                    = VN_IS(nodep, AssignDly)
                          ? جدید AstAssign{nodep->fileline(), tempPureRefp, rhsp}
                          : nodep->cloneType(tempPureRefp, rhsp);
                nodep->addHereThisAsNext(asnp);
                nodep->rhsp(جدید AstVarRef{rhsp->fileline(), tempPurep, VAccess::READ});
                // Only the assignment of the temporary waits
                اگر (waits) افزودن_Deletep(nodep->timingControlp()->unlinkFrBack());
            } وگرنه اگر (need_temp) {
                // The first time we constify, there may be the same variable on the LHS
                // and RHS.  In that case, we must use temporaries, or {a,b}={b,a} will break.
                UINFO(4, "  ASSITEMP " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp1,SEL(rhs,{size})),
                //                                ASSIGN(temp2,SEL(newrhs,{size}))
                //                                ASSIGN(lc1,temp1),
                //                                ASSIGN(lc2,temp2)
            } وگرنه {
                UINFO(4, "  ASSI " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(lc1,SEL(rhs,{size})),
                //                                ASSIGN(lc2,SEL(newrhs,{size}))
            }
            // The NBA evaluates its intra-assignment delay as it executes (IEEE 1800-2023 4.9.4),
            // so once for the NBAs to all parts
            AstDelay* ثابت delayp
                = VN_IS(nodep, AssignDly) ? VN_CAST(nodep->timingControlp(), Delay) : nullptr;
            اگر (delayp && !delayp->lhsp()->isPure()) {
                File_سطر* ثابت flp = nodep->fileline();
                Ast_گره_عبارت* ثابت valuep = delayp->lhsp()->unlinkFrBack();
                AstVar* ثابت tempp = جدید AstVar{flp, VVarType::BLOCKTEMP,
                                                 m_concswapNames.get(valuep), valuep->dtypep()};
                // Static, as read right after written, when the NBAs to the parts start waiting
                tempp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempp);
                nodep->addHereThisAsNext(
                    جدید AstAssign{flp, جدید AstVarRef{flp, tempp, VAccess::WRITE}, valuep});
                delayp->lhsp(جدید AstVarRef{flp, tempp, VAccess::READ});
            }
            UINFOTREE(9, nodep, "", "Ass_old");
            // Unlink the stuff
            Ast_گره_عبارت* ثابت lc1p = VN_AS(nodep->lhsp(), Concat)->lhsp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت lc2p = VN_AS(nodep->lhsp(), Concat)->rhsp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت conp = VN_AS(nodep->lhsp(), Concat)->unlinkFrBack();
            Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت rhs2p = rhsp->clone_درخت_Pure(نادرست);
            // Calc widths
            ثابت صحیح lsb2 = 0;
            ثابت صحیح msb2 = lsb2 + lc2p->width() - 1;
            ثابت صحیح lsb1 = msb2 + 1;
            ثابت صحیح msb1 = lsb1 + lc1p->width() - 1;
            UASSERT_OBJ(msb1 == (conp->width() - 1), nodep, "Width calc mismatch");
            // Form ranges
            AstSel* ثابت sel1p = جدید AstSel{conp->fileline(), rhsp, lsb1, msb1 - lsb1 + 1};
            AstSel* ثابت sel2p = جدید AstSel{conp->fileline(), rhs2p, lsb2, msb2 - lsb2 + 1};
            // Make new assigns of same flavor as old one
            //*** Not cloneTree; just one node.
            Ast_گره_Assign* newp = nullptr;
            اگر (!need_temp) {
                Ast_گره_Assign* ثابت asn1ap = nodep->cloneType(lc1p, sel1p);
                Ast_گره_Assign* ثابت asn2ap = nodep->cloneType(lc2p, sel2p);
                asn1ap->dtypeFrom(sel1p);
                asn2ap->dtypeFrom(sel2p);
                newp = Ast_گره::addNext(newp, asn1ap);
                newp = Ast_گره::addNext(newp, asn2ap);
            } وگرنه {
                UASSERT_OBJ(m_modp, nodep, "Not under module");
                UASSERT_OBJ(m_globalPass, nodep,
                            "Should not reach here when not invoked on whole AstNetlist");
                // We could create just one temp variable, but we'll get better optimization
                // if we make one per term.
                AstVar* ثابت temp1p
                    = جدید AstVar{sel1p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel1p), VFlagLogicPacked{}, msb1 - lsb1 + 1};
                AstVar* ثابت temp2p
                    = جدید AstVar{sel2p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel2p), VFlagLogicPacked{}, msb2 - lsb2 + 1};
                m_modp->addStmtsp(temp1p);
                m_modp->addStmtsp(temp2p);
                Ast_گره_Assign* ثابت asn1ap = nodep->cloneType(
                    جدید AstVarRef{sel1p->fileline(), temp1p, VAccess::WRITE}, sel1p);
                Ast_گره_Assign* ثابت asn2ap = nodep->cloneType(
                    جدید AstVarRef{sel2p->fileline(), temp2p, VAccess::WRITE}, sel2p);
                Ast_گره_Assign* ثابت asn1bp = nodep->cloneType(
                    lc1p, جدید AstVarRef{sel1p->fileline(), temp1p, VAccess::READ});
                Ast_گره_Assign* ثابت asn2bp = nodep->cloneType(
                    lc2p, جدید AstVarRef{sel2p->fileline(), temp2p, VAccess::READ});
                asn1ap->dtypeFrom(temp1p);
                asn1bp->dtypeFrom(temp1p);
                asn2ap->dtypeFrom(temp2p);
                asn2bp->dtypeFrom(temp2p);
                // This order matters
                newp = Ast_گره::addNext(newp, asn1ap);
                newp = Ast_گره::addNext(newp, asn2ap);
                newp = Ast_گره::addNext(newp, asn1bp);
                newp = Ast_گره::addNext(newp, asn2bp);
            }
            اگر (debug() >= 9 && newp) newp->dump_درخت_And_Next(cout, "-     _new: ");
            nodep->addNextHere(newp);
            // Cleanup
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            VL_DO_DANGLING(افزودن_Deletep(conp), conp);
            // Further reduce, either node may have more reductions.
            بازگشت درست;
        } وگرنه اگر (m_doV && VN_IS(nodep->rhsp(), CvtPackedToArray)
                   && lowerAsFixedAggregate(nodep->lhsp()->dtypep())) {
            AstCvtPackedToArray* ثابت cvtp
                = VN_AS(nodep->rhsp(), CvtPackedToArray)->unlinkFrBack();
            Ast_گره_عبارت* srcp = cvtp->fromp()->unlinkFrBack();
            اگر (lowerAsFixedAggregate(srcp->dtypep())) {
                srcp = packFixedAggregate(srcp);
            } وگرنه اگر (Ast_گره_Stream* ثابت streamp = VN_CAST(srcp, گره_Stream)) {
                Ast_گره_عبارت* ثابت streamSrcp = streamp->lhsp();
                اگر (lowerAsFixedAggregate(streamSrcp->dtypep())) {
                    Ast_گره_عبارت* ثابت packedp = packFixedAggregate(streamSrcp->unlinkFrBack());
                    streamp->lhsp(packedp);
                    streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                       VSigning::UNSIGNED,
                                                       packedp->isFourstateOrNoDtype());
                }
            }
            VL_DO_DANGLING(افزودن_Deletep(cvtp), cvtp);
            replaceAssignToFixedAggregate(nodep, nodep->lhsp()->unlinkFrBack(), srcp);
            بازگشت درست;
        } وگرنه اگر (m_doV && VN_IS(nodep->rhsp(), StreamR)
                   && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            // The right-streaming operator on rhs of assignment does not
            // change the order of bits. Eliminate stream but keep its lhsp.
            // Add a cast if needed.
            AstStreamR* ثابت streamp = VN_AS(nodep->rhsp(), StreamR)->unlinkFrBack();
            Ast_گره_عبارت* srcp = streamp->lhsp()->unlinkFrBack();
            Ast_گره_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            ثابت Ast_گره_DType* ثابت dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            اگر (lowerAsFixedAggregate(srcDTypep)) {
                srcp = packFixedAggregate(srcp);
            } وگرنه اگر (VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)) {
                اگر (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    صحیح srcElementBits = 0;
                    اگر (ثابت Ast_گره_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    صحیح dstElementBits = 0;
                    اگر (ثابت Ast_گره_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    srcp = جدید AstCvtArrayToArray{
                        srcp->fileline(), srcp,          nodep->dtypep(), نادرست, 1,
                        dstElementBits,   srcElementBits};
                } وگرنه {
                    srcp = جدید AstCvtArrayToPacked{srcp->fileline(), srcp, nodep->dtypep()};
                }
            } وگرنه اگر (VN_IS(srcDTypep, UnpackArrayDType)) {
                srcp = جدید AstCvtArrayToPacked{srcp->fileline(), srcp, srcDTypep};
                // Handling the case where lhs is wider than rhs by inserting zeros. StreamL does
                // not require this, since the left streaming operator implicitly handles this.
                ثابت صحیح packedBits = nodep->lhsp()->widthMin();
                ثابت صحیح unpackBits
                    = srcDTypep->arrayUnpackedElements() * srcDTypep->subDTypep()->widthMin();
                ثابت uint32_t offset = packedBits > unpackBits ? packedBits - unpackBits : 0;
                srcp = جدید AstShiftL{srcp->fileline(), srcp,
                                     جدید AstConst{srcp->fileline(), offset}, packedBits};
            }
            اگر (!VN_IS(dstDTypep, UnpackArrayDType) && !VN_IS(dstDTypep, QueueDType)
                && !VN_IS(dstDTypep, DynArrayDType)) {
                ثابت صحیح sWidth = srcp->width();
                ثابت صحیح dWidth = nodep->lhsp()->width();
                اگر (sWidth < dWidth) {
                    AstExtend* ثابت extendp = جدید AstExtend{srcp->fileline(), srcp, dWidth};
                    srcp = جدید AstShiftL{
                        srcp->fileline(), extendp,
                        جدید AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                        dWidth};
                }
            }
            nodep->rhsp(srcp);
            VL_DO_DANGLING(افزودن_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            بازگشت درست;
        } وگرنه اگر (m_doV && VN_IS(nodep->lhsp(), StreamL)) {
            // Push the stream operator to the rhs of the assignment statement
            Ast_گره_عبارت* streamp = nodep->lhsp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت dstp = VN_AS(streamp, StreamL)->lhsp()->unlinkFrBack();
            Ast_گره_DType* ثابت dstDTypep = dstp->dtypep()->skipRefp();
            Ast_گره_عبارت* srcp = nodep->rhsp()->unlinkFrBack();
            ثابت Ast_گره_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamL reverses, so reverse=true)
            اگر ((VN_IS(srcDTypep, UnpackArrayDType) || VN_IS(srcDTypep, QueueDType)
                 || VN_IS(srcDTypep, DynArrayDType))
                && (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType))) {
                صحیح blockSize = 1;
                اگر (ثابت AstConst* ثابت constp
                    = VN_CAST(VN_AS(streamp, StreamL)->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    اگر (VL_UNLIKELY(blockSize <= 0)) blockSize = 1;
                }
                صحیح srcElementBits = 0;
                اگر (ثابت Ast_گره_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                صحیح dstElementBits = 0;
                اگر (ثابت Ast_گره_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                nodep->lhsp(dstp);
                nodep->rhsp(جدید AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, درست,
                                                   blockSize, dstElementBits, srcElementBits});
                nodep->dtypep(dstDTypep);
                VL_DO_DANGLING(افزودن_Deletep(streamp), streamp);
                بازگشت درست;
            }
            اگر (lowerAsFixedAggregate(srcDTypep)) srcp = packFixedAggregate(srcp);
            ثابت صحیح sWidth = srcp->width();
            ثابت صحیح dWidth = dstp->width();
            // Connect the rhs to the stream operator and update its width
            VN_AS(streamp, StreamL)->lhsp(srcp);
            اگر (VN_IS(srcDTypep, DynArrayDType) || VN_IS(srcDTypep, QueueDType)
                || VN_IS(srcDTypep, UnpackArrayDType)) {
                streamp->dtypeSetStream();
            } وگرنه {
                streamp->dtypeSetBitOrLogicUnsized(srcp->width(), srcp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   srcp->isFourstateOrNoDtype());
            }
            اگر (VN_IS(dstDTypep, UnpackArrayDType)) {
                streamp = جدید AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
            } وگرنه {
                اگر (dWidth == 0) {
                    streamp = جدید AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
                } وگرنه اگر (sWidth >= dWidth) {
                    streamp = جدید AstSel{streamp->fileline(), streamp, sWidth - dWidth, dWidth};
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(streamp);
            nodep->dtypep(dstDTypep);
            بازگشت درست;
        } وگرنه اگر (m_doV && VN_IS(nodep->lhsp(), StreamR)) {
            // The right stream operator on lhs of assignment statement does
            // not reorder bits. However, if the rhs is wider than the lhs,
            // then we select bits from the left-most, not the right-most.
            Ast_گره_عبارت* ثابت streamp = nodep->lhsp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت dstp = VN_AS(streamp, StreamR)->lhsp()->unlinkFrBack();
            Ast_گره_DType* ثابت dstDTypep = dstp->dtypep()->skipRefp();
            Ast_گره_عبارت* srcp = nodep->rhsp()->unlinkFrBack();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamR does not reverse, so reverse=false).
            // V3Width may have wrapped the source in CvtArrayToPacked; unwrap it.
            اگر (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                Ast_گره_عبارت* origSrcp = srcp;
                اگر (AstCvtArrayToPacked* ثابت cvtp = VN_CAST(srcp, CvtArrayToPacked)) {
                    origSrcp = cvtp->fromp();
                }
                ثابت Ast_گره_DType* ثابت origSrcDTypep = origSrcp->dtypep()->skipRefp();
                اگر (VN_IS(origSrcDTypep, UnpackArrayDType) || VN_IS(origSrcDTypep, QueueDType)
                    || VN_IS(origSrcDTypep, DynArrayDType)) {
                    صحیح srcElementBits = 0;
                    اگر (ثابت Ast_گره_DType* ثابت elemDtp = origSrcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    صحیح dstElementBits = 0;
                    اگر (ثابت Ast_گره_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    اگر (VN_IS(srcp, CvtArrayToPacked)) {
                        origSrcp = VN_AS(srcp, CvtArrayToPacked)->fromp()->unlinkFrBack();
                        VL_DO_DANGLING(افزودن_Deletep(srcp), srcp);
                        srcp = origSrcp;
                    }
                    // Descending unpacked arrays need element reversal
                    منطقی reverse = نادرست;
                    اگر (ثابت AstUnpackArrayDType* ثابت unpackDtp
                        = VN_CAST(origSrcDTypep, UnpackArrayDType)) {
                        reverse = !unpackDtp->declRange().ascending();
                    }
                    nodep->lhsp(dstp);
                    nodep->rhsp(جدید AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, reverse,
                                                       1, dstElementBits, srcElementBits});
                    nodep->dtypep(dstDTypep);
                    VL_DO_DANGLING(افزودن_Deletep(streamp), streamp);
                    بازگشت درست;
                }
            }
            ثابت صحیح sWidth = srcp->width();
            ثابت صحیح dWidth = dstp->width();
            اگر (VN_IS(dstDTypep, UnpackArrayDType)) {
                ثابت صحیح dstBitWidth
                    = dWidth * VN_AS(dstDTypep, UnpackArrayDType)->arrayUnpackedElements();
                // Handling the case where rhs is wider than lhs. StreamL does not require this
                // since the combination of the left streaming operation and the implicit
                // truncation in VL_ASSIGN_UNPACK automatically selects the left-most bits.
                اگر (sWidth > dstBitWidth) {
                    srcp
                        = جدید AstSel{streamp->fileline(), srcp, sWidth - dstBitWidth, dstBitWidth};
                }
                srcp = جدید AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
            } وگرنه {
                اگر (dWidth == 0) {
                    srcp = جدید AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
                } وگرنه اگر (sWidth >= dWidth) {
                    srcp = جدید AstSel{streamp->fileline(), srcp, sWidth - dWidth, dWidth};
                } وگرنه {
                    // Source narrower than destination: left-justify by shifting left.
                    // The right stream operator packs left-to-right, so remaining
                    // LSBs are zero-filled (IEEE 1800-2023 11.4.14.3).
                    اگر (!VN_IS(srcp->dtypep()->skipRefp(), QueueDType)) {
                        AstExtend* ثابت extendp = جدید AstExtend{srcp->fileline(), srcp, dWidth};
                        srcp = جدید AstShiftL{
                            srcp->fileline(), extendp,
                            جدید AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                            dWidth};
                    } وگرنه {  // if it is a queue we dont need do do a shiftL
                        srcp = جدید AstExtend{srcp->fileline(), srcp};
                    }
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(srcp);
            nodep->dtypep(dstDTypep);
            VL_DO_DANGLING(افزودن_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            بازگشت درست;
        } وگرنه اگر (m_doV && VN_IS(nodep->rhsp(), StreamL)) {
            AstStreamL* streamp = VN_AS(nodep->rhsp(), StreamL);
            Ast_گره_عبارت* srcp = streamp->lhsp();
            ثابت Ast_گره_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            Ast_گره_DType* ثابت dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            اگر (lowerAsFixedAggregate(srcDTypep)) {
                Ast_گره_عبارت* ثابت packedp = packFixedAggregate(srcp->unlinkFrBack());
                streamp->lhsp(packedp);
                streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   packedp->isFourstateOrNoDtype());
                srcp = packedp;
            } وگرنه اگر ((VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)
                        || VN_IS(srcDTypep, UnpackArrayDType))) {
                اگر (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    صحیح blockSize = 1;
                    اگر (ثابت AstConst* ثابت constp = VN_CAST(streamp->rhsp(), Const)) {
                        blockSize = constp->toSInt();
                        اگر (VL_UNLIKELY(blockSize <= 0)) {
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
                    صحیح srcElementBits = 0;
                    اگر (ثابت Ast_گره_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    صحیح dstElementBits = 0;
                    اگر (ثابت Ast_گره_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    streamp->unlinkFrBack();
                    srcp = جدید AstCvtArrayToArray{
                        srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     درست,
                        blockSize,        dstElementBits,       srcElementBits};
                    nodep->rhsp(srcp);
                    VL_DO_DANGLING(افزودن_Deletep(streamp), streamp);
                } وگرنه {
                    streamp->lhsp(جدید AstCvtArrayToPacked{srcp->fileline(), srcp->unlinkFrBack(),
                                                          dstDTypep});
                    streamp->dtypeFrom(dstDTypep);
                }
            }
        } وگرنه اگر (m_doV && replaceAssignMultiSel(nodep)) {
            بازگشت درست;
        } وگرنه اگر (replaceAssignSFormatF(nodep)) {
            بازگشت درست;
        }
        بازگشت نادرست;
    }

    // Boolean replacements
    منطقی operandBoolShift(ثابت Ast_گره* nodep) {
        // boolean test of AND(const,SHIFTR(x,const)) -> test of AND(SHIFTL(x,const), x)
        اگر (!VN_IS(nodep, And)) بازگشت نادرست;
        اگر (!VN_IS(VN_AS(nodep, And)->lhsp(), Const)) بازگشت نادرست;
        اگر (!VN_IS(VN_AS(nodep, And)->rhsp(), ShiftR)) بازگشت نادرست;
        ثابت AstShiftR* ثابت shiftp = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR);
        اگر (!VN_IS(shiftp->rhsp(), Const)) بازگشت نادرست;
        اگر (static_cast<uint32_t>(nodep->width()) <= VN_AS(shiftp->rhsp(), Const)->toUInt()) {
            بازگشت نادرست;
        }
        بازگشت درست;
    }
    تهی replaceBoolShift(Ast_گره* nodep) {
        UINFOTREE(9, nodep, "", "bshft_old");
        AstConst* ثابت andConstp = VN_AS(VN_AS(nodep, And)->lhsp(), Const);
        Ast_گره_عبارت* ثابت fromp
            = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->lhsp()->unlinkFrBack();
        AstConst* ثابت shiftConstp
            = VN_AS(VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->rhsp(), Const);
        V3_عدد val{andConstp, andConstp->width()};
        val.opShiftL(andConstp->رقم(), shiftConstp->رقم());
        AstAnd* ثابت newp
            = جدید AstAnd{nodep->fileline(), جدید AstConst{nodep->fileline(), val}, fromp};
        // widthMin no longer applicable if different C-expanded width
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        UINFOTREE(9, newp, "", "_new");
    }

    تهی replaceWithSimulation(Ast_گره* nodep) {
        SimulateVisitor simvis;
        // Run it - may be unoptimizable due to large for loop, etc
        simvis.mainParamEmulate(nodep);
        اگر (!simvis.optimizable()) {
            ثابت Ast_گره* errorp = simvis.whyNotNodep();
            اگر (!errorp) errorp = nodep;
            nodep->v3error("Expecting expression to be constant, but can't determine constant for "
                           << nodep->prettyTypeName() << '\n'
                           << errorp->warnOther() << "... Location of non-constant "
                           << errorp->prettyTypeName() << ": " << simvis.whyNotMessage());
            VL_DO_DANGLING(replaceZero(nodep), nodep);
        } وگرنه {
            // Fetch the result
            Ast_گره* ثابت valuep = simvis.fetch_مقدار_Null(nodep);  // valuep is owned by Simulate
            UASSERT_OBJ(valuep, nodep, "No value returned from simulation");
            // Replace it
            Ast_گره* ثابت newp = valuep->clone_درخت(نادرست);
            newp->fileline(nodep->fileline());
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Simulate->" << newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }

    //----------------------------------------

    // VISITORS
    تهی visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        iterateChildrenBackwardsConst(nodep);
    }
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_concswapNames.reset();
        iterateChildren(nodep);
    }
    تهی visit(AstCFunc* nodep) override {
        // No ASSIGNW removals under funcs, we've long eliminated INITIALs
        // (We should perhaps rename the assignw's to just assigns)
        VL_RESTORER(m_wremove);
        m_wremove = نادرست;
        iterateChildren(nodep);
    }
    تهی visit(AstCLocalScope* nodep) override {
        iterateChildren(nodep);
        اگر (!nodep->stmtsp()) {
            nodep->unlinkFrBack();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    تهی visit(AstScope* nodep) override {
        // No ASSIGNW removals under scope, we've long eliminated INITIALs
        VL_RESTORER(m_wremove);
        VL_RESTORER(m_scopep);
        m_wremove = نادرست;
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    تهی swapSides(Ast_گره_Bi_Com* nodep) {
        // COMMUTATIVE({a},CONST) -> COMMUTATIVE(CONST,{a})
        // This simplifies later optimizations
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBackWithNext();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBackWithNext();
        nodep->lhsp(rhsp);
        nodep->rhsp(lhsp);
        iterate(nodep);  // Again?
    }

    منطقی containsMemberAccessRecurse(Ast_گره* ثابت nodep) {
        اگر (!nodep) بازگشت نادرست;
        ثابت size_t id = m_ids(nodep);
        ثابت auto it = m_containsMemberAccess.find(id);
        اگر (it != m_containsMemberAccess.end()) بازگشت it->second;
        ++m_statMemberAccessVisits;
        منطقی result = نادرست;
        اگر (VN_IS(nodep, MemberSel) || VN_IS(nodep, MethodCall) || VN_IS(nodep, CMethodCall)) {
            result = درست;
        } وگرنه اگر (ثابت Ast_گره_FTask_Ref* ثابت funcRefp = VN_CAST(nodep, گره_FTask_Ref)) {
            اگر (containsMemberAccessRecurse(funcRefp->taskp())) result = درست;
        } وگرنه اگر (ثابت Ast_گره_CCall* ثابت funcRefp = VN_CAST(nodep, گره_CCall)) {
            اگر (containsMemberAccessRecurse(funcRefp->funcp())) result = درست;
        } وگرنه اگر (ثابت Ast_گره_FTask* ثابت funcp = VN_CAST(nodep, گره_FTask)) {
            // Assume that it has a member access
            اگر (funcp->recursive()) result = درست;
        } وگرنه اگر (ثابت AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc)) {
            اگر (funcp->recursive()) result = درست;
        }
        اگر (!result) {
            result = containsMemberAccessRecurse(nodep->op1p())
                     || containsMemberAccessRecurse(nodep->op2p())
                     || containsMemberAccessRecurse(nodep->op3p())
                     || containsMemberAccessRecurse(nodep->op4p());
        }
        اگر (!result && !VN_IS(nodep, گره_FTask)
            && !VN_IS(nodep, CFunc)  // don't enter into next function
            && containsMemberAccessRecurse(nodep->nextp())) {
            result = درست;
        }
        m_containsMemberAccess.emplace(id, result);
        بازگشت result;
    }

    منطقی matchBiopToBitwise(Ast_گره_Biop* ثابت nodep) {
        اگر (!m_convertLogicToBit) بازگشت نادرست;
        اگر (!nodep->lhsp()->width1()) بازگشت نادرست;
        اگر (!nodep->rhsp()->width1()) بازگشت نادرست;
        اگر (!nodep->isPure()) بازگشت نادرست;
        اگر (containsMemberAccessRecurse(nodep)) بازگشت نادرست;
        بازگشت درست;
    }
    منطقی matchConcatRand(AstConcat* nodep) {
        //    CONCAT(RAND, RAND) - created by Chisel code
        AstRand* ثابت aRandp = VN_CAST(nodep->lhsp(), Rand);
        ثابت AstRand* ثابت bRandp = VN_CAST(nodep->rhsp(), Rand);
        اگر (!aRandp || !bRandp) بازگشت نادرست;
        اگر (!aRandp->combinable(bRandp)) بازگشت نادرست;
        UINFO(4, "Concat(Rand,Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        بازگشت درست;
    }
    منطقی matchSelRand(AstSel* nodep) {
        //    SEL(RAND) - created by Chisel code
        AstRand* ثابت aRandp = VN_CAST(nodep->fromp(), Rand);
        اگر (!aRandp) بازگشت نادرست;
        اگر (aRandp->seedp()) بازگشت نادرست;
        UINFO(4, "Sel(Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        بازگشت درست;
    }
    منطقی matchToStringNConst(AstToStringN* nodep) {
        iterateChildren(nodep);
        اگر (ثابت AstConsArray* ثابت consp = VN_CAST(nodep->lhsp(), ConsArray)) {
            اگر (!(m_doExpensive || m_params)) بازگشت نادرست;
            اگر (!consp->is_Constant_درخت()) بازگشت نادرست;
        } وگرنه اگر (!VN_IS(nodep->lhsp(), Const)) {
            بازگشت نادرست;
        }
        replaceWithSimulation(nodep);
        بازگشت درست;
    }
    صحیح operandConcatMove(ثابت AstConcat* nodep) {
        //    CONCAT under concat  (See moveConcat)
        // Return value: true indicates to do it; 2 means move to LHS
        ثابت AstConcat* ثابت abConcp = VN_CAST(nodep->lhsp(), Concat);
        ثابت AstConcat* ثابت bcConcp = VN_CAST(nodep->rhsp(), Concat);
        اگر (!abConcp && !bcConcp) بازگشت 0;
        اگر (bcConcp) {
            ثابت Ast_گره_عبارت* ثابت ap = nodep->lhsp();
            ثابت Ast_گره_عبارت* ثابت bp = bcConcp->lhsp();
            // If a+b == 32,64,96 etc, then we want to have a+b together on LHS
            اگر (VL_BITBIT_I(ap->width() + bp->width()) == 0) بازگشت 2;  // Transform 2: to abConc
        } وگرنه {  // abConcp
            // Unless lhs is already 32 bits due to above, reorder it
            اگر (VL_BITBIT_I(nodep->lhsp()->width()) != 0) بازگشت 1;  // Transform 1: to bcConc
        }
        بازگشت 0;  // ok
    }
    تهی moveConcat(AstConcat* nodep) {
        //    1: CONCAT(CONCAT({a},{b}),{c})  -> CONCAT({a},CONCAT({b}, {c}))
        // or 2: CONCAT({a}, CONCAT({b},{c})) -> CONCAT(CONCAT({a},{b}),{c})
        // Because the lhs of a concat needs a shift but the rhs doesn't,
        // putting additional CONCATs on the RHS leads to fewer assembler operations.
        // However, we'll end up with lots of wide moves if we make huge trees
        // like that, so on 32 bit boundaries, we'll do the opposite form.
        UINFO(4, "Move concat: " << nodep);
        اگر (operandConcatMove(nodep) > 1) {
            Ast_گره_عبارت* ثابت ap = nodep->lhsp()->unlinkFrBack();
            AstConcat* ثابت bcConcp = VN_AS(nodep->rhsp(), Concat);
            bcConcp->unlinkFrBack();
            Ast_گره_عبارت* ثابت bp = bcConcp->lhsp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت cp = bcConcp->rhsp()->unlinkFrBack();
            AstConcat* ثابت abConcp = جدید AstConcat{bcConcp->fileline(), ap, bp};
            nodep->lhsp(abConcp);
            nodep->rhsp(cp);
            // If bp was a concat, then we have this exact same form again!
            // Recurse rather then calling node->iterate to prevent 2^n recursion!
            اگر (operandConcatMove(abConcp)) moveConcat(abConcp);
            VL_DO_DANGLING(افزودن_Deletep(bcConcp), bcConcp);
        } وگرنه {
            AstConcat* ثابت abConcp = VN_AS(nodep->lhsp(), Concat);
            abConcp->unlinkFrBack();
            Ast_گره_عبارت* ثابت ap = abConcp->lhsp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت bp = abConcp->rhsp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت cp = nodep->rhsp()->unlinkFrBack();
            AstConcat* ثابت bcConcp = جدید AstConcat{abConcp->fileline(), bp, cp};
            nodep->lhsp(ap);
            nodep->rhsp(bcConcp);
            اگر (operandConcatMove(bcConcp)) moveConcat(bcConcp);
            VL_DO_DANGLING(افزودن_Deletep(abConcp), abConcp);
        }
    }

    // Special cases
    تهی visit(AstConst*) override {}  // Already constant

    تهی visit(AstCell* nodep) override {
        اگر (m_params) {
            iterateAndNextNull(nodep->paramsp());
        } وگرنه {
            iterateChildren(nodep);
        }
    }
    تهی visit(AstClassOrPackageRef* nodep) override { iterateChildren(nodep); }

    تهی visit(AstMatchMasked* nodep) override {
        // Do not iterate the tables, they must be constant pool entries
        iterate(nodep->lhsp());
        اگر (AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const)) {
            replace_رقم(nodep, AstMatchMasked::fold(constp->رقم(), nodep->matchp()->varp()));
        }
    }

    تهی visit(AstPin* nodep) override { iterateChildren(nodep); }

    تهی replaceLogEq(AstLogEq* nodep) {
        // LOGEQ(a,b) => AstLogAnd{AstLogOr{AstLogNot{a},b},AstLogOr{AstLogNot{b},a}}
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        // Do exactly as IEEE says, might result in extra terms, so in future may do differently
        AstLogAnd* ثابت newp = جدید AstLogAnd{
            nodep->fileline(),
            جدید AstLogOr{nodep->fileline(), جدید AstLogNot{nodep->fileline(), lhsp}, rhsp},
            جدید AstLogOr{nodep->fileline(),
                         جدید AstLogNot{nodep->fileline(), rhsp->clone_درخت_Pure(نادرست)},
                         lhsp->clone_درخت_Pure(نادرست)}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی replaceSelSel(AstSel* nodep) {
        // SEL(SEL({x},a,b),c,d) => SEL({x},a+c,d)
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ثابت belowp = VN_AS(nodep->fromp(), Sel);
        Ast_گره_عبارت* ثابت fromp = belowp->fromp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت lsb1p = nodep->lsbp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت lsb2p = belowp->lsbp()->unlinkFrBack();
        // Eliminate lower range
        UINFO(4, "Elim Lower range: " << nodep);
        Ast_گره_عبارت* newlsbp;
        اگر (VN_IS(lsb1p, Const) && VN_IS(lsb2p, Const)) {
            newlsbp = جدید AstConst{lsb1p->fileline(),
                                   VN_AS(lsb1p, Const)->toUInt() + VN_AS(lsb2p, Const)->toUInt()};
            VL_DO_DANGLING(افزودن_Deletep(lsb1p), lsb1p);
            VL_DO_DANGLING(افزودن_Deletep(lsb2p), lsb2p);
        } وگرنه {
            // Width is important, we need the width of the fromp's expression, not the
            // potentially smaller lsb1p's width, but don't insert a redundant AstExtend.
            // Note that due to some sloppiness in earlier passes, lsb1p might actually be wider,
            // so extend to the wider type.
            ثابت Ast_گره_عبارت* ثابت widep = lsb1p->width() > lsb2p->width() ? lsb1p : lsb2p;
            Ast_گره_عبارت* ثابت lhsp = widep->width() > lsb2p->width()
                                          ? جدید AstExtend{lsb2p->fileline(), lsb2p}
                                          : lsb2p;
            Ast_گره_عبارت* ثابت rhsp = widep->width() > lsb1p->width()
                                          ? جدید AstExtend{lsb1p->fileline(), lsb1p}
                                          : lsb1p;
            lhsp->dtypeFrom(widep);
            rhsp->dtypeFrom(widep);
            newlsbp = جدید AstAdd{lsb1p->fileline(), lhsp, rhsp};
            newlsbp->dtypeFrom(widep);
        }
        AstSel* ثابت newp = جدید AstSel{nodep->fileline(), fromp, newlsbp, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی replaceSelConcat(AstSel* nodep) {
        // SEL(CONCAT(a,b),c,d) => SEL(a or b, . .)
        AstConcat* ثابت conp = VN_AS(nodep->fromp(), Concat);
        Ast_گره_عبارت* ثابت conLhsp = conp->lhsp();
        Ast_گره_عبارت* ثابت conRhsp = conp->rhsp();
        اگر (static_cast<صحیح>(nodep->lsbConst()) >= conRhsp->width()) {
            conLhsp->unlinkFrBack();
            AstSel* ثابت newp
                = جدید AstSel{nodep->fileline(), conLhsp, nodep->lsbConst() - conRhsp->width(),
                             nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } وگرنه اگر (static_cast<صحیح>(nodep->msbConst()) < conRhsp->width()) {
            conRhsp->unlinkFrBack();
            AstSel* ثابت newp
                = جدید AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(), nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } وگرنه {
            // Yuk, split between the two
            conRhsp->unlinkFrBack();
            conLhsp->unlinkFrBack();
            AstConcat* ثابت newp
                = جدید AstConcat{nodep->fileline(),
                                جدید AstSel{nodep->fileline(), conLhsp, 0,
                                           nodep->msbConst() - conRhsp->width() + 1},
                                جدید AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(),
                                           conRhsp->width() - nodep->lsbConst()}};
            nodep->replaceWithKeepDType(newp);
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    منطقی operandSelReplicate(AstSel* nodep) {
        // SEL(REPLICATE(from,rep),lsb,width) => SEL(from,0,width) as long
        // as SEL's width <= b's width
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ثابت repp = VN_AS(nodep->fromp(), Replicate);
        Ast_گره_عبارت* ثابت fromp = repp->srcp();
        ثابت AstConst* ثابت lsbp = VN_CAST(nodep->lsbp(), Const);
        اگر (!lsbp) بازگشت نادرست;
        UASSERT_OBJ(fromp->width(), nodep, "Not widthed");
        اگر ((lsbp->toUInt() / fromp->width())
            != ((lsbp->toUInt() + nodep->width() - 1) / fromp->width())) {
            بازگشت نادرست;
        }
        //
        fromp->unlinkFrBack();
        AstSel* ثابت newp = جدید AstSel{
            nodep->fileline(), fromp,
            جدید AstConst{lsbp->fileline(), lsbp->toUInt() % fromp->width()}, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        بازگشت درست;
    }
    منطقی operandRepRep(AstReplicate* nodep) {
        // REPLICATE(REPLICATE2(from2,cnt2),cnt1) => REPLICATE(from2,(cnt1+cnt2))
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ثابت rep2p = VN_AS(nodep->srcp(), Replicate);
        Ast_گره_عبارت* ثابت from2p = rep2p->srcp();
        ثابت AstConst* ثابت cnt1p = VN_CAST(nodep->countp(), Const);
        اگر (!cnt1p) بازگشت نادرست;
        ثابت AstConst* ثابت cnt2p = VN_CAST(rep2p->countp(), Const);
        اگر (!cnt2p) بازگشت نادرست;
        //
        from2p->unlinkFrBack();
        AstReplicate* ثابت newp
            = جدید AstReplicate{nodep->fileline(), from2p, cnt1p->toUInt() * cnt2p->toUInt()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        بازگشت درست;
    }
    منطقی operandConcatSame(AstConcat* nodep) {
        // CONCAT(fromp,fromp) -> REPLICATE(fromp,1+1)
        // CONCAT(REP(fromp,cnt1),fromp) -> REPLICATE(fromp,cnt1+1)
        // CONCAT(fromp,REP(fromp,cnt1)) -> REPLICATE(fromp,1+cnt1)
        // CONCAT(REP(fromp,cnt1),REP(fromp,cnt2)) -> REPLICATE(fromp,cnt1+cnt2)
        Ast_گره_عبارت* from1p = nodep->lhsp();
        uint32_t cnt1 = 1;
        Ast_گره_عبارت* from2p = nodep->rhsp();
        uint32_t cnt2 = 1;
        اگر (VN_IS(from1p, Replicate)) {
            ثابت AstConst* ثابت cnt1p = VN_CAST(VN_CAST(from1p, Replicate)->countp(), Const);
            اگر (!cnt1p) بازگشت نادرست;
            from1p = VN_AS(from1p, Replicate)->srcp();
            cnt1 = cnt1p->toUInt();
        }
        اگر (VN_IS(from2p, Replicate)) {
            ثابت AstConst* ثابت cnt2p = VN_CAST(VN_CAST(from2p, Replicate)->countp(), Const);
            اگر (!cnt2p) بازگشت نادرست;
            from2p = VN_AS(from2p, Replicate)->srcp();
            cnt2 = cnt2p->toUInt();
        }
        اگر (!operandsSame(from1p, from2p)) بازگشت نادرست;
        //
        from1p->unlinkFrBack();
        AstReplicate* ثابت newp = جدید AstReplicate{nodep->fileline(), from1p, cnt1 + cnt2};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        بازگشت درست;
    }
    تهی replaceSelIntoBiop(AstSel* nodep) {
        // SEL(BUFIF1(a,b),1,bit) => BUFIF1(SEL(a,1,bit),SEL(b,1,bit))
        Ast_گره_Biop* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), گره_Biop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_گره_عبارت* ثابت lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_گره_عبارت* ثابت bilhsp = fromp->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت birhsp = fromp->rhsp()->unlinkFrBack();
        //
        fromp->lhsp(
            جدید AstSel{nodep->fileline(), bilhsp, lsbp->clone_درخت_Pure(درست), nodep->widthConst()});
        fromp->rhsp(جدید AstSel{nodep->fileline(), birhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی replaceSelIntoUniop(AstSel* nodep) {
        // SEL(NOT(a),1,bit) => NOT(SEL(a,bit))
        Ast_گره_Uniop* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), گره_Uniop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_گره_عبارت* ثابت lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_گره_عبارت* ثابت bilhsp = fromp->lhsp()->unlinkFrBack();
        //
        fromp->lhsp(جدید AstSel{nodep->fileline(), bilhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        iterateChildren(nodep);
    }

    تهی visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->bitp());
        اگر (VN_IS(nodep->bitp(), Const)
            && VN_IS(nodep->fromp(), VarRef)
            // Need to make sure it's an array object so don't mis-allow a constant (bug509.)
            && VN_AS(nodep->fromp(), VarRef)->varp()
            && VN_IS(VN_AS(nodep->fromp(), VarRef)->varp()->valuep(), ConsArray)) {
            m_selp = nodep;  // Ask visit(AstVarRef) to replace varref with const
        }
        iterateAndNextNull(nodep->fromp());
        اگر (VN_IS(nodep->fromp(), Const)) {  // It did.
            اگر (!m_selp) {
                nodep->v3error("Illegal assignment of constant to unpacked array");
            } وگرنه {
                Ast_گره* ثابت fromp = nodep->fromp()->unlinkFrBack();
                nodep->replaceWithKeepDType(fromp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
        }
        // Handle ARRAYSEL directly on ConsArray (not through VarRef)
        وگرنه اگر (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ConsArray)) {
            ثابت AstConsArray* ثابت consp = VN_AS(nodep->fromp(), ConsArray);
            ثابت uint32_t bit = VN_AS(nodep->bitp(), Const)->toUInt();
            Ast_گره_عبارت* ثابت itemp = consp->valuep(bit);
            اگر (VN_IS(itemp, Const)) {
                ثابت V3_عدد& رقم = VN_AS(itemp, Const)->رقم();
                VL_DO_DANGLING(replace_رقم(nodep, رقم), nodep);
            } وگرنه اگر (m_params && itemp) {
                // An aggregate element, e.g. a structure in an array parameter
                nodep->replaceWithKeepDType(itemp->clone_درخت(نادرست));
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
        }
        m_selp = nullptr;
    }

    // Evaluate a slice of an unpacked array.  If constantification is
    // required (m_required=true), call replaceWithSimulation() to compute
    // the slice via simulation.  Otherwise just iterate the children.
    تهی visit(AstSliceSel* nodep) override {
        // First constify or width any child nodes
        iterateChildren(nodep);
        اگر (!m_required) بازگشت;  // Do nothing unless we are in parameter mode
        // Fallback to simulation: this will invoke SimulateVisitor::visit(AstSliceSel*)
        replaceWithSimulation(nodep);
    }

    تهی visit(AstStructSel* nodep) override {
        iterateChildren(nodep);

        اگر (ثابت AstConsStruct* ثابت consp = VN_CAST(nodep->fromp(), ConsStruct)) {
            Ast_گره_عبارت* ثابت valuep = consp->memberValuep(nodep->name());
            اگر (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
                VL_DO_DANGLING(replace_رقم(nodep, constp->رقم()), nodep);
            } وگرنه {
                nodep->replaceWith(valuep->clone_درخت(نادرست));
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
        }
    }

    تهی visit(AstCAwait* nodep) override {
        m_hasJumpDelay = درست;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Not linked");
        منطقی did = نادرست;
        اگر (m_doV && (!nodep->varp()->constPoolEntry() || m_selp) && nodep->varp()->valuep()
            && !m_attrp) {
            // UINFOTREE(1, valuep, "", "visitvaref");
            iterateAndNextNull(nodep->varp()->valuep());  // May change nodep->varp()->valuep()
            Ast_گره* ثابت valuep = nodep->varp()->valuep();
            // The selected element, when under a constant select of an array
            ثابت AstConsArray* ثابت consp = VN_CAST(valuep, ConsArray);
            ثابت Ast_گره* ثابت selItemp
                = (m_selp && consp) ? consp->valuep(m_selp->bitConst()) : nullptr;
            اگر (nodep->access().isReadOnly() && valuep
                && ((!m_params  // Can reduce constant wires into equations
                     && m_doNConst && v3Global.opt.fConst() && nodep->varp()->isConst())
                    || nodep->varp()->isParam())) {
                اگر (operandConst(valuep)) {
                    ثابت V3_عدد& رقم = VN_AS(valuep, Const)->رقم();
                    // UINFO(2, "constVisit " << cvtToHex(valuep) << " " << num);
                    ثابت string origParamName
                        = nodep->varp()->isParam() ? nodep->varp()->name() : "";
                    VL_DO_DANGLING(replace_رقم(nodep, رقم, origParamName), nodep);
                    did = درست;
                } وگرنه اگر (ثابت AstConst* ثابت itemp = VN_CAST(selItemp, Const)) {
                    VL_DO_DANGLING(replace_رقم(nodep, itemp->رقم()), nodep);
                    did = درست;
                } وگرنه اگر (m_params && consp) {
                    // Allow parameters to pass arrays
                    // Earlier recursion of ConsArray made sure each array value is constant
                    // This exception is fairly fragile, i.e. doesn't
                    // support arrays of arrays or other stuff
                    Ast_گره* ثابت newp = valuep->clone_درخت(نادرست);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    did = درست;
                } وگرنه اگر (m_params && VN_IS(valuep, ConsStruct)) {
                    Ast_گره* ثابت newp = valuep->clone_درخت(نادرست);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    did = درست;
                } وگرنه اگر (nodep->varp()->isParam() && VN_IS(valuep, Unbounded)) {
                    Ast_گره* ثابت newp = valuep->clone_درخت(نادرست);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    did = درست;
                }
            }
        }
        اگر (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but variable isn't const: "
                           << nodep->varp()->prettyNameQ());
        }
    }
    تهی visit(Ast_عبارت_Stmt* nodep) override {
        iterateChildren(nodep);
        اگر (!Ast_گره::afterCommentp(nodep->stmtsp())) {
            deleteVarScopesUnder(nodep->stmtsp());
            UINFO(8, "ExprStmt(...) " << nodep << " " << nodep->resultp());
            nodep->replaceWith(nodep->resultp()->unlinkFrBack());
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            // Removing the ExprStmt might have made something impure above now pure
        }
    }
    تهی visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        منطقی did = نادرست;
        اگر (nodep->itemp()->valuep()) {
            // UINFOTREE(1, nodep->itemp()->valuep(), "", "visitvaref");
            ثابت AstEnumItem* ثابت itemp = nodep->itemp();
            اگر (m_recursing_Enum_اقلام.شمار(itemp)) {
                nodep->v3error("Recursive enum value: " << nodep->itemp()->prettyNameQ());
            } وگرنه {
                m_recursing_Enum_اقلام.emplace(itemp);
                iterateAndNextNull(nodep->itemp()->valuep());
                m_recursing_Enum_اقلام.erase(itemp);
            }
            اگر (AstConst* ثابت valuep = VN_CAST(nodep->itemp()->valuep(), Const)) {
                ثابت V3_عدد& رقم = valuep->رقم();
                VL_DO_DANGLING(replace_رقم(nodep, رقم), nodep);
                did = درست;
            }
        }
        اگر (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but enum value isn't const: "
                           << nodep->itemp()->prettyNameQ());
        }
    }

    منطقی only_Sen_Item_In_Sen_درخت(ثابت AstSenItem* nodep) {
        // Only one if it's not in a list
        بازگشت (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }
    تهی visit(AstSenItem* nodep) override {
        iterateChildren(nodep);
        اگر (m_doNConst
            && !v3Global.opt.timing().isSetTrue()  // If --timing, V3Sched would turn this into an
                                                   // infinite loop. See #5080
            && (VN_IS(nodep->sensp(), Const) || VN_IS(nodep->sensp(), EnumItemRef)
                || (nodep->varrefp() && nodep->varrefp()->varp()->isParam()))) {
            // Constants in sensitivity lists may be removed (we'll simplify later)
            اگر (nodep->isClocked()) {  // A constant can never get a pos/negedge
                اگر (only_Sen_Item_In_Sen_درخت(nodep)) {
                    اگر (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                        // TODO: This really is dodgy, as strictly compliant simulators will not
                        //       execute this block, but but t_func_check relies on it
                        nodep->replaceWith(
                            جدید AstSenItem{nodep->fileline(), AstSenItem::Initial{}});
                    } وگرنه {
                        nodep->replaceWith(جدید AstSenItem{nodep->fileline(), AstSenItem::Never{}});
                    }
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                } وگرنه {
                    VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } وگرنه {  // Otherwise it may compute a result that needs to settle out
                nodep->replaceWith(جدید AstSenItem{nodep->fileline(), AstSenItem::Combo{}});
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
        } وگرنه اگر (m_doNConst && VN_IS(nodep->sensp(), Not)) {
            // V3Gate may propagate NOTs into clocks... Just deal with it
            Ast_گره* ثابت sensp = nodep->sensp();
            Ast_گره* lastSensp = sensp;
            منطقی invert = نادرست;
            وقتی (VN_IS(lastSensp, Not)) {
                lastSensp = VN_AS(lastSensp, Not)->lhsp();
                invert = !invert;
            }
            UINFO(8, "senItem(NOT...) " << nodep << " " << invert);
            اگر (invert) nodep->edgeType(nodep->edgeType().invert());
            sensp->replaceWith(lastSensp->unlinkFrBack());
            VL_DO_DANGLING(افزودن_Deletep(sensp), sensp);
        }
    }

    رده SenItemCmp final {
        ایستا صحیح cmp(ثابت Ast_گره_عبارت* ap, ثابت Ast_گره_عبارت* bp) {
            ثابت VNType aType = ap->type();
            ثابت VNType bType = bp->type();
            اگر (aType != bType) بازگشت static_cast<صحیح>(bType) - static_cast<صحیح>(aType);

            اگر (ثابت AstVarRef* ثابت aRefp = VN_CAST(ap, VarRef)) {
                ثابت AstVarRef* ثابت bRefp = VN_AS(bp, VarRef);
                // Looks visually better if we keep sorted by name
                اگر (aRefp->name() < bRefp->name()) بازگشت -1;
                اگر (aRefp->name() > bRefp->name()) بازگشت 1;
                // But might be same name with different scopes
                اگر (aRefp->varScopep() < bRefp->varScopep()) بازگشت -1;
                اگر (aRefp->varScopep() > bRefp->varScopep()) بازگشت 1;
                // Or rarely, different data types
                اگر (aRefp->dtypep() < bRefp->dtypep()) بازگشت -1;
                اگر (aRefp->dtypep() > bRefp->dtypep()) بازگشت 1;
                بازگشت 0;
            }

            اگر (ثابت AstConst* ثابت aConstp = VN_CAST(ap, Const)) {
                ثابت AstConst* ثابت bConstp = VN_AS(bp, Const);
                اگر (aConstp->رقم().isLtXZ(bConstp->رقم())) بازگشت -1;
                اگر (bConstp->رقم().isLtXZ(aConstp->رقم())) بازگشت 1;
                بازگشت 0;
            }

            اگر (ثابت Ast_گره_Biop* ثابت aBiOpp = VN_CAST(ap, گره_Biop)) {
                ثابت Ast_گره_Biop* ثابت bBiOpp = VN_AS(bp, گره_Biop);
                // Compare RHSs first, as LHS might be const
                اگر (ثابت صحیح c = cmp(aBiOpp->rhsp(), bBiOpp->rhsp())) بازگشت c;
                بازگشت cmp(aBiOpp->lhsp(), bBiOpp->lhsp());
            }

            اگر (ثابت AstCMethodHard* ثابت aCallp = VN_CAST(ap, CMethodHard)) {
                ثابت AstCMethodHard* ثابت bCallp = VN_AS(bp, CMethodHard);
                اگر (aCallp->method() < bCallp->method()) بازگشت -1;
                اگر (aCallp->method() > bCallp->method()) بازگشت 1;
                اگر (ثابت صحیح c = cmp(aCallp->fromp(), bCallp->fromp())) بازگشت c;
                ثابت Ast_گره_عبارت* aPinsp = aCallp->pinsp();
                ثابت Ast_گره_عبارت* bPinsp = bCallp->pinsp();
                وقتی (aPinsp && bPinsp) {
                    اگر (ثابت صحیح c = cmp(aPinsp, bPinsp)) بازگشت c;
                    aPinsp = VN_AS(aPinsp->nextp(), گره_عبارت);
                    bPinsp = VN_AS(bPinsp->nextp(), گره_عبارت);
                }
                بازگشت aPinsp ? -1 : bPinsp ? 1 : 0;
            }

            بازگشت 0;
        }

    عمومی:
        منطقی operator()(ثابت AstSenItem* lhsp, ثابت AstSenItem* rhsp) ثابت {
            ثابت Ast_گره_عبارت* ثابت lSensp = lhsp->sensp();
            ثابت Ast_گره_عبارت* ثابت rSensp = rhsp->sensp();
            اگر (lSensp && rSensp) {
                // If both terms have sensitivity expressions, recursively compare them
                اگر (ثابت صحیح c = cmp(lSensp, rSensp)) بازگشت c < 0;
            } وگرنه اگر (lSensp || rSensp) {
                // Terms with sensitivity expressions come after those without
                بازگشت rSensp;
            }
            // Finally sort by edge, AFTER variable, as we want multiple edges for same var
            // adjacent. note the SenTree optimizer requires this order (more general first,
            // less general last)
            بازگشت lhsp->edgeType() < rhsp->edgeType();
        }
    };

    تهی visit(Ast_Sen_درخت* nodep) override {
        iterateChildren(nodep);
        اگر (m_doExpensive) {
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
            برای (AstSenItem *nextp, *senp = nodep->sensesp(); senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                // cppcheck-suppress unassignedVariable  // cppcheck bug
                ثابت SenItemCmp cmp;
                اگر (nextp && !cmp(senp, nextp)) {
                    // Something's out of order, sort it
                    senp = nullptr;
                    std::vector<AstSenItem*> vec;
                    برای (AstSenItem* senp = nodep->sensesp(); senp;
                         senp = VN_AS(senp->nextp(), SenItem)) {
                        vec.افزودن_back(senp);
                    }
                    stable_sort(vec.begin(), vec.end(), SenItemCmp());
                    برای (ثابت auto& ip : vec) ip->unlinkFrBack();
                    برای (ثابت auto& ip : vec) nodep->addSensesp(ip);
                    بشکن;
                }
            }

            // Pass 2, remove duplicates and simplify adjacent terms if possible
            برای (AstSenItem *senp = nodep->sensesp(), *nextp; senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                اگر (!nextp) بشکن;
                AstSenItem* ثابت lItemp = senp;
                AstSenItem* ثابت rItemp = nextp;
                ثابت Ast_گره_عبارت* ثابت lSenp = lItemp->sensp();
                ثابت Ast_گره_عبارت* ثابت rSenp = rItemp->sensp();
                اگر (!lSenp || !rSenp) ادامه;

                اگر (lSenp->same_Gate_درخت(rSenp)) {
                    // POSEDGE or NEGEDGE -> BOTHEDGE. (We've sorted POSEDGE, before NEGEDGE, so we
                    // do not need to test for the opposite orders.)
                    اگر (lItemp->edgeType() == VEdgeType::ET_POSEDGE
                        && rItemp->edgeType() == VEdgeType::ET_NEGEDGE) {
                        // Make both terms BOTHEDGE, the second will be removed below
                        lItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                        rItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                    }

                    // Remove identical expressions
                    اگر (lItemp->edgeType() == rItemp->edgeType()) {
                        VL_DO_DANGLING(افزودن_Deletep(rItemp->unlinkFrBack()), rItemp);
                        nextp = lItemp;
                    }
                }
            }
        }
    }

    //-----
    // Zero elimination
    تهی visit(Ast_گره_Assign* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->timingControlp()) m_hasJumpDelay = درست;
        اگر (m_doNConst && replace_گره_Assign(nodep)) بازگشت;
    }
    تهی visit(AstAlias* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    تهی visit(AstAliasScope* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    تهی visit(AstAssignW* nodep) override {
        iterateChildren(nodep);
        اگر (m_doNConst && replace_گره_Assign(nodep)) بازگشت;
        // Process containing this AssignW as single body statement
        AstAlways* ثابت procp = VN_CAST(nodep->backp(), Always);
        اگر (!procp || procp->stmtsp() != nodep || nodep->nextp()) بازگشت;
        // Not VarXRef, as different refs may set different values to each hierarchy
        Ast_گره_Var_Ref* ثابت varrefp = VN_CAST(nodep->lhsp(), VarRef);
        اگر (m_wremove && !m_params && m_doNConst && m_modp && operandConst(nodep->rhsp())
            && !VN_AS(nodep->rhsp(), Const)->رقم().isFourState()
            && varrefp  // Don't do messes with BITREFs/ARRAYREFs
            && !varrefp->varp()->hasStrengthAssignment()  // Strengths are resolved in V3Tristate
            && !varrefp->varp()->valuep()  // Not already constified
            && !varrefp->varScopep()  // Not scoped (or each scope may have different initial val.)
            && !varrefp->varp()->isForced()  // Not forced (not really a constant)
        ) {
            // ASSIGNW (VARREF, const) -> INITIAL ( ASSIGN (VARREF, const) )
            UINFO(4, "constAssignW " << nodep);
            // Make a initial assignment
            Ast_گره_عبارت* ثابت exprp = nodep->rhsp()->unlinkFrBack();
            varrefp->unlinkFrBack();
            File_سطر* ثابت flp = nodep->fileline();
            AstInitial* ثابت newinitp = جدید AstInitial{flp, جدید AstAssign{flp, varrefp, exprp}};
            procp->replaceWith(newinitp);
            VL_DO_DANGLING(افزودن_Deletep(procp), procp);
            // Set the initial value right in the variable so we can constant propagate
            Ast_گره* ثابت initvaluep = exprp->clone_درخت(نادرست);
            varrefp->varp()->isConst(درست);
            varrefp->varp()->valuep(initvaluep);
        }
    }
    تهی visit(AstCReset* nodep) override {
        iterateChildren(nodep);
        اگر (!m_doNConst) بازگشت;
        ثابت AstBasicDType* ثابت bdtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType);
        اگر (!bdtypep) بازگشت;
        اگر (!bdtypep->isZeroInit()) بازگشت;
        AstConst* ثابت newp = جدید AstConst{nodep->fileline(), V3_عدد{nodep, bdtypep}};
        UINFO(9, "CRESET(0) => CONST(0) " << nodep);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstCvtUnpackToUnpack* nodep) override {
        iterateChildren(nodep);
        AstConsArray* ثابت fromp = VN_CAST(nodep->fromp(), ConsArray);
        اگر (!fromp) بازگشت;
        // Fold the outermost dimension, converting the elements for any dimension under it
        ثابت AstUnpackArrayDType* ثابت toArrayp
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت AstUnpackArrayDType* ثابت fromArrayp
            = VN_AS(fromp->dtypep()->skipRefp(), UnpackArrayDType);
        // This dimension may not be reversed, only one under it
        ثابت منطقی reverse
            = toArrayp->declRange().ascending() != fromArrayp->declRange().ascending();
        ثابت uint64_t lastIndex = toArrayp->elementsConst() - 1;
        Ast_گره_DType* ثابت subDTypep = toArrayp->subDTypep();
        ثابت AstUnpackArrayDType* ثابت subArrayp
            = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
        ثابت auto convert = [&](Ast_گره_عبارت* valuep) -> Ast_گره_عبارت* {
            valuep->unlinkFrBack();
            اگر (!subArrayp || !subArrayp->isReverseOf(valuep->dtypep())) بازگشت valuep;
            بازگشت جدید AstCvtUnpackToUnpack{valuep->fileline(), valuep, subDTypep};
        };
        AstConsArray* ثابت newp
            = جدید AstConsArray{fromp->fileline(), nodep->dtypep(),
                               fromp->defaultp() ? convert(fromp->defaultp()) : nullptr};
        برای (ثابت AstConsArrayItem* itemp = fromp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            newp->valuep(reverse ? lastIndex - itemp->index() : itemp->index(),
                         convert(itemp->valuep()));
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        iterate(newp);  // Fold the conversions of the elements
    }
    تهی visit(AstCvtArrayToArray* nodep) override {
        iterateChildren(nodep);
        // Handle the case where we have a stream operation inside a cast conversion
        // Check for both StreamL and StreamR operations
        Ast_گره_Stream* streamp = nullptr;
        منطقی isReverse = نادرست;
        اگر (AstStreamL* ثابت streamLp = VN_CAST(nodep->fromp(), StreamL)) {
            streamp = streamLp;
            isReverse = درست;  // StreamL reverses the operation
        } وگرنه اگر (AstStreamR* ثابت streamRp = VN_CAST(nodep->fromp(), StreamR)) {
            streamp = streamRp;
            isReverse = نادرست;  // StreamR doesn't reverse the operation
        }
        اگر (streamp) {
            Ast_گره_عبارت* srcp = streamp->lhsp();
            ثابت Ast_گره_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            Ast_گره_DType* ثابت dstDTypep = nodep->dtypep()->skipRefp();
            اگر (VN_IS(srcDTypep, QueueDType) && VN_IS(dstDTypep, QueueDType)) {
                صحیح blockSize = 1;
                اگر (ثابت AstConst* ثابت constp = VN_CAST(streamp->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    اگر (VL_UNLIKELY(blockSize <= 0)) {
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
                صحیح srcElementBits = 0;
                اگر (ثابت Ast_گره_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                صحیح dstElementBits = 0;
                اگر (ثابت Ast_گره_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                streamp->unlinkFrBack();
                Ast_گره_عبارت* newp = جدید AstCvtArrayToArray{
                    srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     isReverse,
                    blockSize,        dstElementBits,       srcElementBits};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(streamp), streamp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت;
            }
        }
    }
    تهی visit(AstRelease* nodep) override {
        // cppcheck-suppress constVariablePointer // children unlinked below
        اگر (AstConcat* ثابت concatp = VN_CAST(nodep->lhsp(), Concat)) {
            File_سطر* ثابت flp = nodep->fileline();
            AstRelease* ثابت newLp = جدید AstRelease{flp, concatp->lhsp()->unlinkFrBack()};
            AstRelease* ثابت newRp = جدید AstRelease{flp, concatp->rhsp()->unlinkFrBack()};
            nodep->replaceWith(newLp);
            newLp->addNextHere(newRp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            visit(newLp);
            visit(newRp);
        }
    }

    تهی visit(Ast_گره_If* nodep) override {
        iterateChildren(nodep);
        اگر (m_doNConst) {
            اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep->condp(), Const)) {
                Ast_گره* keepp = nullptr;
                Ast_گره* delp = nullptr;
                اگر (constp->isZero()) {
                    UINFO(4, "IF(0,{any},{x}) => {x}: " << nodep);
                    keepp = nodep->elsesp();
                    delp = nodep->thensp();
                } وگرنه اگر (!m_doV || constp->isNeqZero()) {  // Might be X in Verilog
                    UINFO(4, "IF(!0,{x},{any}) => {x}: " << nodep);
                    keepp = nodep->thensp();
                    delp = nodep->elsesp();
                } وگرنه {
                    UINFO(4, "IF condition is X, retaining: " << nodep);
                    بازگشت;
                }
                اگر (!canDeleteStatements()) {
                    UINFO(4, "IF retained until statics lifted: " << nodep);
                    بازگشت;
                }

                // If we delete a branch that contains variable declarations, also delete the
                // corresponding varscopes so we don't leave dangling AstVarScope::m_varp pointers.
                deleteVarScopesUnder(delp);

                اگر (keepp) {
                    keepp->unlinkFrBackWithNext();
                    nodep->replaceWith(keepp);
                } وگرنه {
                    nodep->unlinkFrBack();
                    deleteVarScopesUnder(nodep->thensp());
                    deleteVarScopesUnder(nodep->elsesp());
                }
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);

                // Removed branch could contain only impurity within a function and the
                // function could be purified
                VIsCached::clear_Cache_درخت();
            } وگرنه اگر (!Ast_گره::afterCommentp(nodep->thensp())
                       && !Ast_گره::afterCommentp(nodep->elsesp())) {
                اگر (!nodep->condp()->isPure()) {
                    // Condition has side effect - leave, but don't need return value
                    UINFO(4, "IF({x}) ; => STMTEXPR({x}): " << nodep);
                    nodep->fileline()->warnOff(V3ErrorCode::IGNOREDRETURN, درست);
                    Ast_گره_Stmt* ثابت newp
                        = جدید Ast_Stmt_عبارت{nodep->fileline(), nodep->condp()->unlinkFrBack()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                } وگرنه {
                    // Empty block, remove it
                    UINFO(4, "IF({x}) ; => ; : " << nodep);
                    VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } وگرنه اگر (!Ast_گره::afterCommentp(nodep->thensp())) {
                UINFO(4, "IF({x}) nullptr {...} => IF(NOT{x}}: " << nodep);
                Ast_گره_عبارت* ثابت condp = nodep->condp();
                Ast_گره* ثابت elsesp = nodep->elsesp();
                condp->unlinkFrBackWithNext();
                elsesp->unlinkFrBackWithNext();
                اگر (nodep->thensp()) {  // Must have been comment
                    افزودن_Deletep(nodep->thensp()->unlinkFrBackWithNext());
                }
                nodep->condp(جدید AstLogNot{condp->fileline(),
                                           condp});  // LogNot, as C++ optimization also possible
                nodep->addThensp(elsesp);
                nodep->branchPred(nodep->branchPred().invert());
            } وگرنه اگر (((VN_IS(nodep->condp(), Not) && nodep->condp()->width() == 1)
                        || VN_IS(nodep->condp(), LogNot))
                       && nodep->thensp() && nodep->elsesp()) {
                UINFO(4, "IF(NOT {x})  => IF(x) swapped if/else" << nodep);
                Ast_گره_عبارت* ثابت condp
                    = VN_AS(nodep->condp(), گره_Uniop)->lhsp()->unlinkFrBackWithNext();
                Ast_گره* ثابت thensp = nodep->thensp()->unlinkFrBackWithNext();
                Ast_گره* ثابت elsesp = nodep->elsesp()->unlinkFrBackWithNext();
                AstIf* ثابت ifp = جدید AstIf{nodep->fileline(), condp, elsesp, thensp};
                ifp->isBoundsCheck(nodep->isBoundsCheck());  // Copy bounds check info
                ifp->branchPred(nodep->branchPred().invert());
                nodep->replaceWith(ifp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وگرنه اگر (ifSameAssign(nodep)) {
                UINFO(4,
                      "IF({a}) ASSIGN({b},{c}) else ASSIGN({b},{d}) => ASSIGN({b}, {a}?{c}:{d})");
                Ast_گره_Assign* ثابت thensp = VN_AS(nodep->thensp(), گره_Assign);
                // cppcheck-suppress constVariablePointer // children unlinked below
                Ast_گره_Assign* ثابت elsesp = VN_AS(nodep->elsesp(), گره_Assign);
                thensp->unlinkFrBack();
                Ast_گره_عبارت* ثابت condp = nodep->condp()->unlinkFrBack();
                Ast_گره_عبارت* ثابت truep = thensp->rhsp()->unlinkFrBack();
                Ast_گره_عبارت* ثابت falsep = elsesp->rhsp()->unlinkFrBack();
                thensp->rhsp(جدید AstCond{truep->fileline(), condp, truep, falsep});
                nodep->replaceWith(thensp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وگرنه اگر (نادرست  // Disabled, as vpm assertions are faster
                              // without due to short-circuiting
                       && operandIfIf(nodep)) {
                UINFO(9, "IF({a}) IF({b}) => IF({a} && {b})");
                Ast_گره_If* ثابت lowerIfp = VN_AS(nodep->thensp(), گره_If);
                Ast_گره_عبارت* ثابت condp = nodep->condp()->unlinkFrBack();
                Ast_گره* ثابت lowerThensp = lowerIfp->thensp()->unlinkFrBackWithNext();
                Ast_گره_عبارت* ثابت lowerCondp = lowerIfp->condp()->unlinkFrBackWithNext();
                nodep->condp(جدید AstLogAnd{lowerIfp->fileline(), condp, lowerCondp});
                lowerIfp->replaceWith(lowerThensp);
                VL_DO_DANGLING(افزودن_Deletep(lowerIfp), lowerIfp);
            } وگرنه {
                // Optimizations that don't reform the IF itself
                اگر (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
                matchIfSameLast(nodep);
                matchIfCondCond(nodep);
            }
        }
    }

    تهی visit(AstDisplay* nodep) override {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        iterateChildren(nodep);
        اگر (stmtDisplayDisplay(nodep)) بازگشت;
    }
    منطقی stmtDisplayDisplay(AstDisplay* nodep) {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        اگر (!m_modp) بازگشت نادرست;  // Don't optimize under single statement
        AstDisplay* ثابت prevp = VN_CAST(nodep->backp(), Display);
        اگر (!prevp) بازگشت نادرست;
        اگر (!((prevp->displayType() == nodep->displayType())
              || (prevp->displayType() == VDisplayType::DT_WRITE
                  && nodep->displayType() == VDisplayType::DT_DISPLAY)
              || (prevp->displayType() == VDisplayType::DT_DISPLAY
                  && nodep->displayType() == VDisplayType::DT_WRITE)))
            بازگشت نادرست;
        اگر ((prevp->filep() && !nodep->filep()) || (!prevp->filep() && nodep->filep())
            || (prevp->filep() && nodep->filep() && !prevp->filep()->same_درخت(nodep->filep())))
            بازگشت نادرست;
        اگر (!prevp->fmtp() || prevp->fmtp()->nextp() || !nodep->fmtp() || nodep->fmtp()->nextp())
            بازگشت نادرست;
        AstSFormatF* ثابت pformatp = prevp->fmtp();
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSFormatF* ثابت nformatp = nodep->fmtp();
        // We don't merge scopeNames as can have only one and might be different scopes (late in
        // process) Also rare for real code to print %m multiple times in same message
        اگر (nformatp->scopeNamep() && pformatp->scopeNamep()) بازگشت نادرست;
        // We don't early merge arguments as might need to later print warnings with
        // right line numbers, nor scopeNames as might be different scopes (late in process)
        اگر (!m_doCpp && pformatp->exprsp()) بازگشت نادرست;
        اگر (!m_doCpp && nformatp->exprsp()) بازگشت نادرست;
        اگر (pformatp->exprsp() && !pformatp->exprsp()->isPureAndNext()) بازگشت نادرست;
        اگر (nformatp->exprsp() && !nformatp->exprsp()->isPureAndNext()) بازگشت نادرست;
        // Avoid huge merges
        ایستا constexpr صحیح DISPLAY_MAX_MERGE_LENGTH = 500;
        اگر (pformatp->text().length() + nformatp->text().length() > DISPLAY_MAX_MERGE_LENGTH)
            بازگشت نادرست;
        //
        UINFO(9, "DISPLAY(SF({a})) DISPLAY(SF({b})) -> DISPLAY(SF({a}+{b}))");
        // Convert DT_DISPLAY to DT_WRITE as may allow later optimizations
        اگر (prevp->displayType() == VDisplayType::DT_DISPLAY) {
            prevp->displayType(VDisplayType::DT_WRITE);
            pformatp->text(pformatp->text() + "\n");
        }
        // We can't replace prev() as the edit tracking iterators will get confused.
        // So instead we edit the prev note itself.
        اگر (prevp->addNewline()) pformatp->text(pformatp->text() + "\n");
        pformatp->text(pformatp->text() + nformatp->text());
        اگر (!prevp->addNewline() && nodep->addNewline()) pformatp->text(pformatp->text() + "\n");
        اگر (nformatp->exprsp()) pformatp->addExprsp(nformatp->exprsp()->unlinkFrBackWithNext());
        اگر (AstScopeName* ثابت scopeNamep = nformatp->scopeNamep()) {
            scopeNamep->unlinkFrBackWithNext();
            pformatp->scopeNamep(scopeNamep);
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
        بازگشت درست;
    }
    تهی visit(AstSFormatArg* nodep) override {
        // Skip namep(): requiring its runtime lookup to be constant can reject valid
        // enum-valued constant-function calls in parameters. displayedEnum() resolves
        // the name from the folded exprp() value and enum dtype instead.
        iterateAndNextNull(nodep->exprp());
    }
    تهی visit(AstSFormatF* nodep) override {
        // Substitute constants into displays.  The main point of this is to
        // simplify assertion methodologies which call functions with display's.
        // This eliminates a pile of wide temps, and makes the C a whole lot more readable.
        iterateChildren(nodep);
        اگر (nodep->عبارت_Format_2()) {
            // Upconvert to text-based format?
            // Similar code in V3LinkResolve::visit(AstSFormatF)
            UASSERT_OBJ(nodep->text() == "", nodep,
                        "Non-format $sformatf should have text format == \"\"");
            اگر (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->رقم().isFromString()) {
                AstConst* ثابت fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->رقم().toString());
                nodep->عبارت_Format_2(نادرست);
                VL_DO_DANGLING(افزودن_Deletep(fmtp), fmtp);
            }
        }
        اگر (nodep->عبارت_Format_2()) بازگشت;  // Runtime, unfortunately  (unless constify format later)
        منطقی anyconst = نادرست;
        برای (Ast_گره* argp = nodep->exprsp(); argp; argp = argp->nextp()) {
            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
            Ast_گره* ثابت subargp = fargp ? fargp->exprp() : argp;
            اگر (VN_IS(subargp, Const)) {
                anyconst = درست;
                بشکن;
            }
        }
        اگر (m_doNConst && anyconst) {
            UINFO(9, "  Display in  " << nodep->text());
            string newFormat;
            string fmt;
            منطقی inPct = نادرست;
            Ast_گره* argp = nodep->exprsp();
            ثابت string text = nodep->text();
            برای (نویسه ch : text) {
                اگر (!inPct && ch == '%') {
                    inPct = درست;
                    fmt = ch;
                } وگرنه اگر (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } وگرنه اگر (inPct) {
                    inPct = نادرست;
                    fmt += ch;
                    ch = std::tolower(ch);
                    انتخاب (ch) {
                    حالت '%': بشکن;  // %% - still %%
                    حالت 'm': بشکن;  // %m - still %m - auto insert "name"
                    حالت 'l': بشکن;  // %l - still %l - auto insert "library"
                    حالت 't':  // %t - don't know $timeformat so can't constify
                        اگر (argp) argp = argp->nextp();
                        بشکن;
                    پیشفرض:  // Most operators, just move to next argument
                        اگر (argp) {
                            Ast_گره* ثابت nextp = argp->nextp();
                            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                            Ast_گره* ثابت subargp = fargp ? fargp->exprp() : argp;
                            ثابت VFormatAttr formatAttr
                                = argp
                                      ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                                      : VFormatAttr{};
                            اگر (VN_IS(subargp, Const)) {  // Convert it
                                ثابت string out
                                    = formatAttr.isEnum()
                                          ? const_رقم_V(subargp).displayedEnum(fargp, fmt)
                                          : const_رقم_V(subargp).displayed(nodep, fmt, formatAttr);
                                UINFO(9, "     DispConst: " << fmt << " -> " << out << "  for "
                                                            << subargp);
                                // fmt = out w/ replace % with %% as it must later when
                                // used as a format output as literal.
                                fmt = VString::quotePercent(out);
                                VL_DO_DANGLING(افزودن_Deletep(argp->unlinkFrBack()), argp);
                            }
                            argp = nextp;
                        }
                        بشکن;
                    }  // switch
                    newFormat += fmt;
                } وگرنه {
                    newFormat += ch;
                }
            }
            اگر (newFormat != nodep->text()) {
                nodep->text(newFormat);
                UINFO(9, "  Display out " << nodep);
            }
        }
        اگر (!nodep->exprsp() && nodep->text().find('%') == string::npos && !nodep->hidden()) {
            // Just a simple constant string - the formatting is pointless
            UINFO(9, "replaceConstStr");
            VL_DO_DANGLING(replaceConstString(nodep, nodep->text()), nodep);
            بازگشت;
        }
        اگر (VN_IS(nodep->backp(), گره_Assign)  // Can't remove under Display etc
            && nodep->text() == "%s" && VN_IS(nodep->exprsp(), SFormatArg)
            && VN_AS(nodep->exprsp(), SFormatArg)->formatAttr().isString()
            && !nodep->exprsp()->nextp()) {
            UINFO(9, "  Display(%p, expr) -> constant expr " << nodep);
            nodep->replaceWith(VN_AS(nodep->exprsp(), SFormatArg)->exprp()->unlinkFrBack());
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }
    }
    تهی visit(AstSFormat* nodep) override {
        iterateChildren(nodep);
        اگر (!m_doNConst) بازگشت;
        اگر (m_doNConst) {
            // If it's a constant string written to a string variable, inline it as an assignment
            AstSFormatF* ثابت fmtp = nodep->fmtp();
            Ast_گره_عبارت* ثابت lhsp = nodep->lhsp();
            AstBasicDType* ثابت basicp = VN_CAST(lhsp->dtypep()->skipRefp(), BasicDType);
            اگر (basicp && basicp->isString() && !fmtp->exprsp()
                && fmtp->text().find('%') == string::npos) {
                AstConst* ثابت strp
                    = جدید AstConst{fmtp->fileline(), AstConst::String{}, fmtp->text()};
                lhsp->unlinkFrBack();
                nodep->replaceWith(جدید AstAssign{nodep->fileline(), lhsp, strp});
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت;
            }
        }
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_underRecFunc);
        اگر (nodep->recursive()) m_underRecFunc = درست;
        iterateChildren(nodep);
    }

    تهی visit(Ast_گره_CCall* nodep) override {
        iterateChildren(nodep);
        m_hasJumpDelay = درست;  // As don't analyze inside tasks for timing controls
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        // Note excludes AstFuncRef as other visitor below
        iterateChildren(nodep);
        m_hasJumpDelay = درست;  // As don't analyze inside tasks for timing controls
    }
    تهی visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_گره_FTask_Ref*>(nodep));
        اگر (m_params) {  // Only parameters force us to do constant function call propagation
            replaceWithSimulation(nodep);
        }
    }
    تهی visit(AstArg* nodep) override {
        // replaceWithSimulation on the Arg's parent FuncRef replaces these
        iterateChildren(nodep);
    }
    تهی visit(AstLoop* nodep) override {
        VL_RESTORER(m_hasLoopTest);
        ثابت منطقی oldHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = نادرست;
        m_hasLoopTest = نادرست;
        iterateChildren(nodep);
        ثابت منطقی thisLoopHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = thisLoopHasJumpDelay || oldHasJumpDelay;
        // If the first statement always break, the loop is useless
        اگر (ثابت AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            اگر (testp->condp()->isZero() && (!testp->nextp() || canDeleteStatements())) {
                nodep->v3warn(UNUSEDLOOP, "Loop condition is always false");
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                بازگشت;
            }
        }
        // If last statement always breaks, repalce loop with body
        اگر (Ast_گره* lastp = nodep->stmtsp()) {
            وقتی (Ast_گره* ثابت nextp = lastp->nextp()) lastp = nextp;
            اگر (AstLoopTest* ثابت testp = VN_CAST(lastp, LoopTest)) {
                اگر (testp->condp()->isZero()) {
                    VL_DO_DANGLING2(افزودن_Deletep(testp->unlinkFrBack()), testp, lastp);
                    nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                }
            }
        }
        // Warn on infinite loop
        اگر (!m_hasLoopTest && !thisLoopHasJumpDelay) {
            nodep->v3warn(INFINITELOOP, "Infinite loop (condition always true)");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::INFINITELOOP, درست);  // Complain once
        }
    }
    تهی visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);

        // If never breaks, remove
        اگر (nodep->condp()->isNeqZero()) {
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }

        m_hasLoopTest = درست;

        // If always breaks, subsequent statements are dead code, delete them
        اگر (nodep->condp()->isZero()) {
            Ast_گره* ثابت nextp = nodep->nextp();
            اگر (nextp && canDeleteStatements()) {
                VL_DO_DANGLING(افزودن_Deletep(nextp->unlinkFrBackWithNext()), nodep);
            }
            بازگشت;
        }

        اگر (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
    }

    تهی visit(AstConsArray* nodep) override { iterateChildren(nodep); }
    تهی visit(AstConsStruct* nodep) override { iterateChildren(nodep); }
    تهی visit(AstConsStructItem* nodep) override { iterateChildren(nodep); }
    تهی visit(AstConsArrayItem* nodep) override { iterateChildren(nodep); }
    تهی visit(AstUnbounded* nodep) override { iterateChildren(nodep); }
    // These are converted by V3Param.  Don't constify as we don't want the
    // from() VARREF to disappear, if any.
    // If output of a presel didn't get consted, chances are V3Param didn't visit properly
    تهی visit(Ast_گره_Pre_Sel*) override {}

    // Ignored, can eliminate early
    تهی visit(AstSysIgnore* nodep) override {
        iterateChildren(nodep);
        اگر (m_doNConst) VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }

    تهی visit(Ast_Stmt_عبارت* nodep) override {
        iterateChildren(nodep);
        // Malformed due to child being deleted, remove here
        اگر (!nodep->exprp()) {
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }
        // Remove if expression is trivially pure
        اگر (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), VarRef)) {
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }
        // Remove calls to empty functions
        اگر (ثابت AstCCall* ثابت callp = VN_CAST(nodep->exprp(), CCall)) {
            ثابت AstCFunc* ثابت funcp = callp->funcp();
            اگر (!callp->argsp() && funcp->emptyBody()) {
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                بازگشت;
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
    تهی visit(AstBasicDType* nodep) override {
        iterateChildren(nodep);
        nodep->cvtRangeConst();
    }

    //-----
    // Jump elimination

    تهی visit(AstJumpGo* nodep) override {
        // Any statements following the JumpGo (at this statement level) never execute, delete
        اگر (nodep->nextp() && canDeleteStatements()) {
            افزودن_Deletep(nodep->nextp()->unlinkFrBackWithNext());
        }

        // JumpGo as last statement in target JumpBlock (including last in a last sub-list),
        // is a no-op, remove it.
        اگر (!nodep->nextp()) {
            برای (Ast_گره* abovep = nodep->abovep(); abovep; abovep = abovep->abovep()) {
                اگر (abovep == nodep->blockp()) {
                    VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                    بازگشت;
                }
                // Stop if not doing expensive, or if the above node is not the last in its list,
                // ... or if it's not an 'if' TODO: it would be enough if it was not a branch.
                اگر (!m_doExpensive || abovep->nextp() || !VN_IS(abovep, If)) بشکن;
            }
        }
        // Mark JumpBlock as used
        m_usedJumpBlocks.emplace(nodep->blockp());
        m_hasJumpDelay = درست;
    }

    تهی visit(AstJumpBlock* nodep) override {
        iterateChildren(nodep);

        // If first statement is an AstLoopTest, pull it before the jump block
        اگر (AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            nodep->addHereThisAsNext(testp->unlinkFrBack());
        }

        // Remove if empty
        اگر (!nodep->stmtsp()) {
            UINFO(4, "JUMPLABEL => empty " << nodep);
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }

        // If no JumpGo points to this node, replace it with its body
        اگر (!m_usedJumpBlocks.شمار(nodep)) {
            UINFO(4, "JUMPLABEL => unused " << nodep);
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }

    //-----
    // Below lines are magic expressions processed by astgen
    //  TREE_SKIP_VISIT("AstNODETYPE")    # Rename normal visit to visitGen and don't iterate
    //-----
    // clang-format off
    درخت_SKIP_VISIT("ArraySel");
    درخت_SKIP_VISIT("CAwait");

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
    تهی visit(Ast_گره* nodep) override {
        // Default: Just iterate
        اگر (m_required) {
            اگر (VN_IS(nodep, گره_DType) || VN_IS(nodep, Range) || VN_IS(nodep, SliceSel)
                || VN_IS(nodep, Dot) || VN_IS(nodep, Text)) {
                // ignore
            } وگرنه اگر (VN_IS(nodep, Pattern) || VN_IS(nodep, PatMember)) {
                // A parameter override pattern is typed later, so only fold its members
                iterateChildren(nodep);
            } وگرنه اگر (AstCellRef* ثابت crp = VN_CAST(nodep, CellRef)) {
                iterate(crp->exprp());
                اگر (Ast_گره* ثابت newp = crp->exprp()) {
                    crp->replaceWithKeepDType(newp->unlinkFrBack());
                    VL_DO_DANGLING(افزودن_Deletep(crp), crp);
                }
                بازگشت;
            } وگرنه {
                nodep->v3error("Expecting expression to be constant, but can't convert a "
                               << nodep->prettyTypeName() << " to constant.");
            }
        } وگرنه {
            اگر (nodep->isTimingControl()) m_hasJumpDelay = درست;
            // Calculate the width of this operation
            اگر (m_params && !nodep->width()) nodep = V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);
        }
    }

عمومی:
    // Processing Mode Enum
    شمارش ProcMode : uint8_t {
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
    ConstVisitor(ProcMode pmode, منطقی globalPass)
        : m_globalPass{globalPass}
        , m_concswapNames{globalPass ? ("__Vconcswap_" + cvtToStr(s_global_Pass_رقم++)) : ""} {
        // clang-format off
        انتخاب (pmode) {
        حالت PROC_PARAMS_NOWARN:  m_doV = درست;  m_doNConst = درست; m_params = درست;
                                  m_required = نادرست; بشکن;
        حالت PROC_PARAMS:         m_doV = درست;  m_doNConst = درست; m_params = درست;
                                  m_required = درست; بشکن;
        حالت PROC_GENERATE:       m_doV = درست;  m_doNConst = درست; m_params = درست;
                                  m_required = درست; m_doGenerate = درست; بشکن;
        حالت PROC_LIVE:           بشکن;
        حالت PROC_V_WARN:         m_doV = درست;  m_doNConst = درست; m_warn = درست; m_convertLogicToBit = درست; بشکن;
        حالت PROC_V_NOWARN:       m_doV = درست;  m_doNConst = درست; بشکن;
        حالت PROC_V_EXPENSIVE:    m_doV = درست;  m_doNConst = درست; m_doExpensive = درست; بشکن;
        حالت PROC_CPP:            m_doV = نادرست; m_doNConst = درست; m_doCpp = درست; بشکن;
        پیشفرض:                  v3fatalSrc("Bad case"); بشکن;
        }
        // clang-format on
    }
    ~ConstVisitor() override {
        اگر (m_doCpp) {
            اگر (m_globalPass) {
                V3Stats::addStat("Optimizations, Const bit op reduction", m_statBitOpReduction);
            } وگرنه {
                V3Stats::addStatSum("Optimizations, Const bit op reduction", m_statBitOpReduction);
            }
        }
        V3Stats::addStatSum("Optimizations, Cond redundant expressions", m_stat_Cond_عبارت_Redundant);
        V3Stats::addStatSum("Optimizations, If cond redundant expressions",
                            m_stat_If_Cond_عبارت_Redundant);
        V3Stats::addStatSum("Optimizations, Concat merges", m_statConcatMerge);
        V3Stats::addStatSum("Optimizations, Member access predicate node visits",
                            m_statMemberAccessVisits);
    }

    Ast_گره* mainAcceptEdit(Ast_گره* nodep) {
        VIsCached::clear_Cache_درخت();  // Avoid using any stale isPure
        // Operate starting at a random place
        بازگشت iterateSubtreeReturnEdits(nodep);
    }
};

uint32_t ConstVisitor::s_global_Pass_رقم = 0;

//######################################################################
// Const class functions

//! Force this cell node's parameter list to become a constant
تهی V3Const::constifyParamsEdit(Ast_گره* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS, /* globalPass: */ نادرست};
    // cppcheck-suppress constVariablePointer // edited below
    اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        اگر (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } وگرنه {
        (تهی)visitor.mainAcceptEdit(nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump if have next line
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

//! Constify this cell node's parameter list if possible
تهی V3Const::constifyParamsNoWarnEdit(Ast_گره* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS_NOWARN, /* globalPass: */ نادرست};
    // cppcheck-suppress constVariablePointer // edited below
    اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        اگر (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } وگرنه {
        (تهی)visitor.mainAcceptEdit(nodep);
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
تهی V3Const::constifyGenerateParamsEdit(Ast_گره* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE:");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized
    // yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthGenerateParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_GENERATE, /* globalPass: */ نادرست};
    // cppcheck-suppress constVariablePointer // edited below
    اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        اگر (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } وگرنه {
        VL_DO_DANGLING(visitor.mainAcceptEdit(nodep), nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump this.
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

تهی V3Const::constifyAllLint(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_WARN, /* globalPass: */ درست};
        (تهی)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("const", 0, dump_درخت_Either_Level() >= 3);
}

تهی V3Const::constifyCpp(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ درست};
        (تهی)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("const_cpp", 0, dump_درخت_Either_Level() >= 3);
}

Ast_گره* V3Const::constifyEdit(Ast_گره* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_NOWARN, /* globalPass: */ نادرست};
    nodep = visitor.mainAcceptEdit(nodep);
    بازگشت nodep;
}

Ast_گره* V3Const::constifyEditCpp(Ast_گره* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ نادرست};
    nodep = visitor.mainAcceptEdit(nodep);
    بازگشت nodep;
}

تهی V3Const::constifyAllLive(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    // This only pushes constants up, doesn't make any other edits
    // IE doesn't prune dead statements, as we need to do some usability checks after this
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_LIVE, /* globalPass: */ درست};
        (تهی)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("const", 0, dump_درخت_Either_Level() >= 3);
}

تهی V3Const::constifyAll(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ درست};
        (تهی)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("const", 0, dump_درخت_Either_Level() >= 3);
}

Ast_گره* V3Const::constifyExpensiveEdit(Ast_گره* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ نادرست};
    nodep = visitor.mainAcceptEdit(nodep);
    بازگشت nodep;
}
