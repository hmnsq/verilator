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

#define شجرة_SKIP_VISIT(...)
#define TREEOP1(...)
#define TREEOPA(...)
#define TREEOP(...)
#define TREEOPS(...)
#define TREEOPC(...)
#define TREEOPV(...)

//######################################################################
// Utilities

ساكن منطقي isConst(ثابت Ast_عقدة* nodep, uint64_t v) {
    ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
    أرجع constp && constp->toUQuad() == v;
}

template <typename T>
ساكن typename std::enable_if<std::is_integral<T>::قيمة, منطقي>::type isPow2(T val) {
    أرجع (val & (val - 1)) == 0;
}

ساكن صحيح عدد_Trailing_Zeroes(uint64_t val) {
    UASSERT(val, "countTrailingZeroes argument must be non-zero");
#إذا defined(__GNUC__) && !defined(VL_NO_BUILTINS)
    أرجع __builtin_ctzll(val);
#وإلا
    صحيح bit = 0;
    val = ~val;
    طالما (val & 1) {
        ++bit;
        val >>= 1;
    }
    أرجع bit;
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
    size_t operator()(Ast_عقدة* nodep) {
        إذا (!m_inuser4p) m_inuser4p = std::make_unique<VNUser4InUse>();  // Claim on first use
        إذا (!nodep->user4p()) nodep->user4p(reinterpret_cast<فراغ*>(++m_nextId));
        أرجع reinterpret_cast<size_t>(nodep->user4p());
    }
};

// This visitor can be used in the post-expanded Ast from V3Expand, where the Ast satisfies:
// - Constants are 64 bit at most (because words are accessed via AstWordSel)
// - Variables are scoped.
صنف Const_Bit_Op_شجرة_Visitor final : عام VNVisitorConst {
    // TYPES

    // Holds a node to be added as a term in the reduction tree, it's equivalent op count, and a
    // bool indicating if the term is clean (0/1 value, or if the top bits might be dirty)
    using ResultTerm = std::tuple<Ast_عقدة_تعبير*, unsigned, منطقي>;

    صنف LeafInfo final {  // Leaf node (either AstConst or AstVarRef)
        // MEMBERS
        منطقي m_polarity = true;  // Invert result due to NOT
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
            إذا ((m_lsb <= m_msb && newLsb > newMsb) || (m_lsb > m_msb && m_lsb < newLsb)) {
                // When the new bit range is out of m_refp, clear polarity because nodes below is
                // shifted out to zero.
                // This kind of clear may happen several times. e.g. (!(1'b1 >> 1)) >> 1
                polarity(true);
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
        صحيح wordIdx() ثابت { أرجع m_wordIdx; }
        فراغ wordIdx(صحيح i) { m_wordIdx = i; }
        منطقي polarity() ثابت { أرجع m_polarity; }
        فراغ polarity(منطقي p) { m_polarity = p; }

        AstVarRef* refp() ثابت { أرجع m_refp; }
        ثابت AstConst* constp() ثابت { أرجع m_constp; }
        منطقي missingWordSel() ثابت {
            // When V3Expand is skipped, WordSel is not inserted.
            أرجع m_refp->isWide() && m_wordIdx == -1;
        }
        صحيح lsb() ثابت { أرجع m_lsb; }

        صحيح msb() ثابت { أرجع std::min(m_msb, varWidth() - 1); }
        صحيح varWidth() ثابت {
            UASSERT(m_refp, "m_refp should be set");
            ثابت صحيح width = m_refp->varp()->widthMin();
            إذا (!m_refp->isWide()) {
                UASSERT_OBJ(m_wordIdx == -1, m_refp, "Bad word index into non-wide");
                أرجع width;
            } وإلا {
                إذا (missingWordSel()) أرجع width;
                UASSERT_OBJ(m_wordIdx >= 0, m_refp, "Bad word index into wide");
                ثابت صحيح bitsInMSW = VL_BITBIT_E(width) ? VL_BITBIT_E(width) : VL_EDATASIZE;
                أرجع m_wordIdx == m_refp->width_كلمات() - 1 ? bitsInMSW : VL_EDATASIZE;
            }
        }
    };

    بنية BitPolarityEntry final {  // Found bit polarity during iterate()
        LeafInfo m_info;  // Leaf (variable or constant) bit polarity was found on
        منطقي m_polarity = خطأ;  // Polarity the bit must have to match
        صحيح m_bit = 0;  // Bit index within the leaf that was tested
        BitPolarityEntry(ثابت LeafInfo& info, منطقي pol, صحيح bit)
            : m_info{info}
            , m_polarity{pol}
            , m_bit{bit} {}
        BitPolarityEntry() = افتراضي;
    };

    بنية Frozen_عقدة_Info final {  // Context when a frozen node is found
        منطقي m_polarity;  // Polarity the frozen node must match
        صحيح m_lsb;  // LSB position of the frozen node
        منطقي operator<(ثابت Frozen_عقدة_Info& other) ثابت {
            إذا (m_lsb != other.m_lsb) أرجع m_lsb < other.m_lsb;
            أرجع m_polarity < other.m_polarity;
        }
    };

    صنف Restorer final {  // Restore the original state unless disableRestore() is called
        Const_Bit_Op_شجرة_Visitor& m_visitor;  // Visitor whose state is saved and restored
        ثابت size_t m_polaritiesSize;  // Saved m_visitor.m_bitPolarities size to truncate back to
        ثابت size_t m_frozenSize;  // Saved m_visitor.m_frozenNodes size to truncate back to
        ثابت unsigned m_ops;  // Saved m_visitor.m_ops to restore
        ثابت منطقي m_polarity;  // Saved m_visitor.m_polarity to restore
        منطقي m_restore = true;  // Whether the destructor still needs to restore state

    عام:
        explicit Restorer(Const_Bit_Op_شجرة_Visitor& visitor)
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
            إذا (m_restore) restoreNow();
        }
        فراغ disableRestore() { m_restore = خطأ; }
        فراغ restoreNow() {
            UASSERT(m_restore, "Can be called just once");
            m_visitor.m_bitPolarities.resize(m_polaritiesSize);
            m_visitor.m_frozenNodes.resize(m_frozenSize);
            m_visitor.m_ops = m_ops;
            m_visitor.m_polarity = m_polarity;
            m_restore = خطأ;
        }
    };
    // Collect information for each Variable to transform as below
    صنف VarInfo final {
        // MEMBERS
        صحيح m_knownResult = -1;  // -1: result is not known, 0 or 1: result of this tree
        ثابت Const_Bit_Op_شجرة_Visitor* ثابت
            m_parentp;  // ConstBitOpTreeVisitor holding this VarInfo
        AstVarRef* ثابت m_refp;  // Points the variable that this VarInfo covers
        ثابت صحيح m_width;  // Width of term this VarInfo refers to
        V3_رقم m_bitPolarity;  // Coefficient of each bit

    عام:
        // METHODS
        منطقي hasConstResult() ثابت { أرجع m_knownResult >= 0 || m_bitPolarity.isAllX(); }
        // The constant result. Only valid if hasConstResult() returned true.
        منطقي getConstResult() ثابت {
            // Note that this condition covers m_knownResult == -1 but m_bitPolarity.isAllX(),
            // in which case the result is 0
            أرجع m_knownResult == 1;
        }
        ثابت AstVarRef* refp() ثابت { أرجع m_refp; }
        منطقي sameVarAs(ثابت Ast_عقدة_Var_Ref* otherp) ثابت { أرجع m_refp->same_عقدة(otherp); }
        فراغ setPolarity(منطقي compBit, صحيح bit) {
            // Ignore if already determined a known reduction
            إذا (m_knownResult >= 0) أرجع;
            UASSERT_OBJ(bit < m_width, m_refp,
                        "Bit index out of range: " << bit << " width: " << m_width);
            إذا (m_bitPolarity.bitIsX(bit)) {  // The bit is not yet marked with either polarity
                m_bitPolarity.setBit(bit, compBit);
            } وإلا {  // The bit has already been marked with some polarity
                ثابت منطقي sameFlag = m_bitPolarity.bitIs1(bit) == compBit;
                إذا (m_parentp->is_Xor_شجرة()) {
                    UASSERT_OBJ(compBit && sameFlag, m_refp, "Only true is set in Xor tree");
                    // a ^ a ^ b == b so we can ignore a
                    m_bitPolarity.setBit(bit, 'x');
                } وإلا {  // And, Or
                    // Can ignore this nodep as the bit is already marked with the same polarity
                    إذا (sameFlag) أرجع;  // a & a == a, b | b == b
                    // Otherwise result is constant (a & ~a == 0) or (a | ~a == 1)
                    m_knownResult = m_parentp->is_And_شجرة() ? 0 : 1;
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
            Ast_عقدة_تعبير* srcp = VN_CAST(m_refp->backp(), WordSel);
            إذا (!srcp) srcp = m_refp;
            srcp = srcp->clone_شجرة(خطأ);

            // Signed variables might have redundant sign bits that need masking.
            ثابت منطقي hasRedundantSignBits
                = m_refp->varp()->dtypep()->isSigned()
                  && (m_refp->isWide() ? (m_width != VL_EDATASIZE)
                                       : (m_width < 8 || !isPow2(m_width)));

            // Get the mask that selects the bits that are relevant in this term
            V3_رقم mask_رقم{srcp, m_width, 0};
            mask_رقم.opBitsNonXZ(m_bitPolarity);  // 'x' -> 0, 0->1, 1->1
            ثابت uint64_t maskVal = mask_رقم.toUQuad();
            UASSERT_OBJ(maskVal != 0, m_refp,
                        "Should have been recognized as having const 0 result");

            // Parts of the return value
            Ast_عقدة_تعبير* resultp = srcp;  // The tree for this term
            unsigned ops = 0;  // Number of ops in this term
            منطقي clean = خطأ;  // Whether the term is clean (has value 0 or 1)

            إذا (isPow2(maskVal)) {
                // If we only want a single bit, shift it out instead of a masked compare. Shifts
                // don't go through the flags register on x86 and are hence faster. This is also
                // always fewer or same ops as mask and compare, but with shorter instructions on
                // x86.

                // Find the index of the bit we want.
                ثابت صحيح bit = عدد_Trailing_Zeroes(maskVal);
                // If we want something other than the bottom bit, shift it out
                إذا (bit != 0) {
                    resultp = جديد AstShiftR{fl, resultp,
                                            جديد AstConst{fl, static_cast<uint32_t>(bit)}, m_width};
                    ++ops;
                }
                // Negate it if necessary
                ثابت منطقي negate = m_bitPolarity.bitIs0(bit);
                إذا (negate) {
                    resultp = جديد AstNot{fl, resultp};
                    ++ops;
                }
                // Clean if MSB of unsigned value, and not negated
                clean = (bit == m_width - 1) && !hasRedundantSignBits && !negate;
            } وإلا {
                // We want multiple bits. Go ahead and extract them.

                // Check if masking is required, and if so apply it
                ثابت منطقي needsMasking = maskVal != VL_MASK_Q(m_width) || hasRedundantSignBits;
                إذا (needsMasking) {
                    resultp = جديد AstAnd{fl, جديد AstConst{fl, mask_رقم}, resultp};
                    ++ops;
                }

                // Create the sub-expression for this term
                إذا (m_parentp->is_Xor_شجرة()) {
                    إذا (needsMasking) {
                        // Reduce the masked term to the minimum known width,
                        // to use the smallest RedXor formula
                        ثابت صحيح widthMin = mask_رقم.widthToFit();
                        resultp->dtypeChgWidth(widthMin, widthMin);
                    }
                    resultp = جديد AstRedXor{fl, resultp};
                    ++ops;
                    clean = خطأ;
                    // VL_REDXOR_* returns IData, set width accordingly to avoid unnecessary casts
                    resultp->dtypeChgWidth(VL_IDATASIZE, 1);
                } وإلا إذا (m_parentp->is_And_شجرة()) {
                    V3_رقم comp_رقم{srcp, m_width, 0};
                    comp_رقم.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    resultp = جديد AstEq{fl, جديد AstConst{fl, comp_رقم}, resultp};
                    ++ops;
                    clean = true;
                } وإلا {  // Or
                    V3_رقم comp_رقم{srcp, m_width, 0};
                    comp_رقم.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    comp_رقم.opXor(V3_رقم{comp_رقم}, mask_رقم);
                    resultp = جديد AstNeq{fl, جديد AstConst{fl, comp_رقم}, resultp};
                    ++ops;
                    clean = true;
                }
            }

            أرجع ResultTerm{resultp, ops, clean};
        }

        // CONSTRUCTORS
        VarInfo(Const_Bit_Op_شجرة_Visitor* parent, AstVarRef* refp, صحيح width)
            : m_parentp{parent}
            , m_refp{refp}
            , m_width{width}
            , m_bitPolarity{refp, m_width} {
            m_bitPolarity.setAllBitsX();
        }
    };

    // MEMBERS
    منطقي m_failed = خطأ;
    منطقي m_polarity = true;  // Flip when AstNot comes
    unsigned m_ops;  // Number of operations such as And, Or, Xor, Sel...
    صحيح m_lsb = 0;  // Current LSB
    LeafInfo* m_leafp = nullptr;  // AstConst or AstVarRef that currently looking for
    ثابت Ast_عقدة_تعبير* ثابت m_rootp;  // Root of this AST subtree

    std::vector<std::pair<Ast_عقدة_تعبير*, Frozen_عقدة_Info>>
        m_frozenNodes;  // Nodes that cannot be optimized
    std::vector<BitPolarityEntry> m_bitPolarities;  // Polarity of bits found during iterate()
    std::vector<std::unique_ptr<VarInfo>> m_varInfos;  // VarInfo for each variable, [0] is nullptr
    VNIdAllocator& m_ids;  // Node id allocator, owned by ConstVisitor
    // Base index of m_varInfos that points VarInfo, keyed by AstVarScope/AstVarRef id,
    // zero means not set yet.
    std::unordered_map<size_t, صحيح> m_baseIdxs;

    // METHODS

    منطقي is_And_شجرة() ثابت { أرجع VN_IS(m_rootp, And); }
    منطقي is_Or_شجرة() ثابت { أرجع VN_IS(m_rootp, Or); }
    منطقي is_Xor_شجرة() ثابت { أرجع VN_IS(m_rootp, Xor) || VN_IS(m_rootp, RedXor); }

#define CONST_BITOP_RETURN_IF(cond, nodep) \
    إذا (setFailed(cond, #cond, nodep, __LINE__)) أرجع

#define CONST_BITOP_SET_FAILED(reason, nodep) setFailed(true, reason, nodep, __LINE__)

    منطقي setFailed(منطقي fail, ثابت حرف* reason, ثابت Ast_عقدة* nodep, صحيح سطر) {
        إذا (fail && !m_failed) {
            UINFO(9, "cannot optimize " << m_rootp << " reason:" << reason << " called from line:"
                                        << سطر << " when checking:" << nodep);
            // UINFOTREE(9, m_rootp, "", "root");
            m_failed = true;
        }
        أرجع m_failed;
    }
    فراغ incrOps(ثابت Ast_عقدة* nodep, صحيح سطر) {
        ++m_ops;
        UINFO(9, "Increment to " << m_ops << " " << nodep << " called from line " << سطر);
    }
    VarInfo& getVarInfo(ثابت LeafInfo& ref) {
        UASSERT_OBJ(ref.refp(), m_rootp, "null varref in And/Or/Xor optimization");
        Ast_عقدة* nodep = ref.refp()->varScopep();
        إذا (!nodep) nodep = ref.refp()->varp();  // Not scoped
        صحيح& baseIdxr = m_baseIdxs[m_ids(nodep)];
        إذا (baseIdxr == 0) {  // Not set yet
            baseIdxr = m_varInfos.size();
            ثابت صحيح رقم_كلمات
                = ref.refp()->dtypep()->isWide() ? ref.refp()->dtypep()->width_كلمات() : 1;
            m_varInfos.resize(m_varInfos.size() + رقم_كلمات);
        }
        ثابت size_t idx = baseIdxr + std::max(0, ref.wordIdx());
        VarInfo* varInfop = m_varInfos[idx].get();
        إذا (!varInfop) {
            varInfop = جديد VarInfo{هذا, ref.refp(), ref.varWidth()};
            m_varInfos[idx].reset(varInfop);
            إذا (ref.missingWordSel()) {
                // ConstBitOpTreeVisitor makes some constants for masks and its type is uint64_t.
                // That's why V3Expand, that inserts WordSel, is needed.
                CONST_BITOP_SET_FAILED("V3Expand is skipped", ref.refp());
            }
        } وإلا {
            إذا (!varInfop->sameVarAs(ref.refp()))
                CONST_BITOP_SET_FAILED("different var (scope?)", ref.refp());
        }
        أرجع *varInfop;
    }

    // Traverse down to see AstConst or AstVarRef
    LeafInfo findLeaf(Ast_عقدة* nodep, منطقي expectConst) {
        إذا (!nodep->dtypep()->skipRefp()->isIntegralOrPacked()) أرجع LeafInfo{};
        LeafInfo info{m_lsb};
        {
            VL_RESTORER(m_leafp);
            m_leafp = &info;
            iterateConst(nodep);
        }

        منطقي ok = !m_failed;
        إذا (expectConst) {
            ok &= !info.refp() && info.constp();
        } وإلا {
            ok &= info.refp() && !info.constp();
        }
        أرجع ok ? info : LeafInfo{};
    }

    // VISITORS
    فراغ visit(Ast_عقدة* nodep) override { CONST_BITOP_SET_FAILED("Hit unexpected op", nodep); }
    فراغ visit(AstCCast* nodep) override {
        iterateChildrenConst(nodep);
        إذا (m_leafp) m_leafp->updateBitRange(nodep);
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
        Ast_عقدة* lhsp = nodep->lhsp();
        AstCCast* ثابت castp = VN_CAST(lhsp, CCast);
        إذا (castp) lhsp = castp->lhsp();
        CONST_BITOP_RETURN_IF(!is_Xor_شجرة() && !VN_IS(lhsp, VarRef) && !VN_IS(lhsp, ShiftR), lhsp);
        incrOps(nodep, __LINE__);
        m_polarity = !m_polarity;
        iterateChildrenConst(nodep);
        // Don't restore m_polarity for Xor as it counts parity of the entire tree
        إذا (!is_Xor_شجرة()) m_polarity = !m_polarity;
        إذا (m_leafp && castp) m_leafp->updateBitRange(castp);
        إذا (m_leafp) m_leafp->polarity(!m_leafp->polarity());
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
        Ast_عقدة* lhsp = nodep->lhsp();
        ثابت AstCCast* ثابت castp = VN_CAST(lhsp, CCast);
        إذا (castp) lhsp = castp->lhsp();
        إذا (ثابت AstAnd* ثابت andp = VN_CAST(lhsp, And)) {  // '^(mask & leaf)'
            CONST_BITOP_RETURN_IF(!andp, lhsp);

            ثابت LeafInfo& mask = findLeaf(andp->lhsp(), true);
            CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

            LeafInfo ref = findLeaf(andp->rhsp(), خطأ);
            CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());
            إذا (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all subtree succeeded

            ثابت V3_رقم& mask_رقم = mask.constp()->رقم();

            incrOps(nodep, __LINE__);
            incrOps(andp, __LINE__);

            // Mark all bits checked in this reduction
            ثابت صحيح maxBitIdx = std::min(ref.lsb() + mask_رقم.width(), ref.msb() + 1);
            لكل (صحيح bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                ثابت صحيح maskIdx = bitIdx - ref.lsb();
                إذا (mask_رقم.bitIs0(maskIdx)) استمر;
                // Set true, m_polarity takes care of the entire parity
                m_bitPolarities.emplace_back(ref, true, bitIdx);
            }
        } وإلا {  // '^leaf'
            LeafInfo ref = findLeaf(lhsp, خطأ);
            CONST_BITOP_RETURN_IF(!ref.refp(), lhsp);
            إذا (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all checks passed

            incrOps(nodep, __LINE__);

            // Mark all bits checked by this comparison
            لكل (صحيح bitIdx = ref.lsb(); bitIdx <= ref.msb(); ++bitIdx) {
                m_bitPolarities.emplace_back(ref, true, bitIdx);
            }
        }
    }

    فراغ visit(Ast_عقدة_Biop* nodep) override {
        إذا (VN_IS(nodep, And) && isConst(nodep->lhsp(), 1)) {  // 1 & _
            // Always reach past a plain making AND
            Restorer restorer{*هذا};
            incrOps(nodep, __LINE__);
            iterateConst(nodep->rhsp());
            إذا (m_leafp) m_leafp->limitBitRangeToLsb();
            CONST_BITOP_RETURN_IF(m_failed, nodep->rhsp());
            restorer.disableRestore();  // Now all checks passed
        } وإلا إذا (nodep->type() == m_rootp->type()) {  // And, Or, Xor
            // subtree under NOT can be optimized only in XOR tree.
            CONST_BITOP_RETURN_IF(!m_polarity && !is_Xor_شجرة(), nodep);
            incrOps(nodep, __LINE__);
            لكل (ثابت منطقي يمين : {خطأ, true}) {
                VL_RESTORER(m_leafp);
                Restorer restorer{*هذا};
                LeafInfo leafInfo{m_lsb};
                // cppcheck-suppress danglingLifetime
                m_leafp = &leafInfo;
                Ast_عقدة_تعبير* opp = يمين ? nodep->rhsp() : nodep->lhsp();
                ثابت منطقي origFailed = m_failed;
                iterateConst(opp);
                إذا (leafInfo.constp() || m_failed) {
                    // Revert changes in leaf
                    restorer.restoreNow();
                    // Reach past a cast then add to frozen nodes to be added to final reduction
                    إذا (ثابت AstCCast* ثابت castp = VN_CAST(opp, CCast)) opp = castp->lhsp();
                    ثابت منطقي pol = is_Xor_شجرة() || m_polarity;  // Only AND/OR tree needs polarity
                    UASSERT_OBJ(pol, nodep, "AND/OR tree expects m_polarity==true");
                    m_frozenNodes.emplace_back(opp, Frozen_عقدة_Info{pol, m_lsb});
                    m_failed = origFailed;
                    استمر;
                }
                restorer.disableRestore();  // Now all checks passed
                إذا (leafInfo.refp()) {
                    // The conditional on the lsb being in range is necessary for some degenerate
                    // case, e.g.: (IData)((QData)wide[0] >> 32), or <1-bit-var> >> 1, which is
                    // just zero
                    إذا (leafInfo.lsb() <= leafInfo.msb()) {
                        m_bitPolarities.emplace_back(leafInfo, is_Xor_شجرة() || leafInfo.polarity(),
                                                     leafInfo.lsb());
                    } وإلا إذا ((is_And_شجرة() && leafInfo.polarity())
                               || (is_Or_شجرة() && !leafInfo.polarity())) {
                        // If there is a constant 0 term in an And tree or 1 term in an Or tree, we
                        // must include it. Fudge this by adding a bit with both polarities, which
                        // will simplify to zero or one respectively.
                        // Note that Xor tree does not need this kind of care, polarity of Xor tree
                        // is already cared when visitin AstNot. Taking xor with 1'b0 is nop.
                        m_bitPolarities.emplace_back(leafInfo, true, 0);
                        m_bitPolarities.emplace_back(leafInfo, خطأ, 0);
                    }
                }
            }
        } وإلا إذا ((is_And_شجرة() && VN_IS(nodep, Eq)) || (is_Or_شجرة() && VN_IS(nodep, Neq))) {
            Restorer restorer{*هذا};
            CONST_BITOP_RETURN_IF(!m_polarity, nodep);
            CONST_BITOP_RETURN_IF(m_lsb, nodep);  // the result of EQ/NE is 1 bit width
            ثابت Ast_عقدة* lhsp = nodep->lhsp();
            إذا (ثابت AstCCast* ثابت castp = VN_CAST(lhsp, CCast)) lhsp = castp->lhsp();
            ثابت AstConst* ثابت constp = VN_CAST(lhsp, Const);
            CONST_BITOP_RETURN_IF(!constp, nodep->lhsp());

            ثابت V3_رقم& comp_رقم = constp->رقم();

            auto setPolarities = [هذا, &comp_رقم](ثابت LeafInfo& ref, ثابت V3_رقم* maskp) {
                ثابت منطقي maskFlip = is_And_شجرة() ^ ref.polarity();
                صحيح constantWidth = comp_رقم.width();
                إذا (maskp) constantWidth = std::max(constantWidth, maskp->width());
                ثابت صحيح maxBitIdx = std::max(ref.lsb() + constantWidth, ref.msb() + 1);
                // Mark all bits checked by this comparison
                لكل (صحيح bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                    ثابت صحيح maskIdx = bitIdx - ref.lsb();
                    ثابت منطقي mask0 = maskp && maskp->bitIs0(maskIdx);
                    ثابت منطقي outOfRange = bitIdx > ref.msb();
                    إذا (mask0 || outOfRange) {  // RHS is 0
                        إذا (comp_رقم.bitIs1(maskIdx)) {
                            // LHS is 1
                            // And tree: 1 == 0 => always false, set v && !v
                            // Or tree : 1 != 0 => always true, set v || !v
                            m_bitPolarities.emplace_back(ref, true, 0);
                            m_bitPolarities.emplace_back(ref, خطأ, 0);
                            اكسر;
                        } وإلا {  // This bitIdx is irrelevant
                            استمر;
                        }
                    }
                    ثابت منطقي polarity = comp_رقم.bitIs1(maskIdx) != maskFlip;
                    m_bitPolarities.emplace_back(ref, polarity, bitIdx);
                }
            };

            إذا (ثابت AstAnd* ثابت andp = VN_CAST(nodep->rhsp(), And)) {  // comp == (mask & v)
                ثابت LeafInfo& mask = findLeaf(andp->lhsp(), true);
                CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

                ثابت LeafInfo& ref = findLeaf(andp->rhsp(), خطأ);
                CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());

                restorer.disableRestore();  // Now all checks passed

                ثابت V3_رقم& mask_رقم = mask.constp()->رقم();

                incrOps(nodep, __LINE__);
                incrOps(andp, __LINE__);

                setPolarities(ref, &mask_رقم);
            } وإلا {  // comp == v
                ثابت LeafInfo& ref = findLeaf(nodep->rhsp(), خطأ);
                CONST_BITOP_RETURN_IF(!ref.refp(), nodep->rhsp());

                restorer.disableRestore();  // Now all checks passed

                incrOps(nodep, __LINE__);

                setPolarities(ref, nullptr);
            }
        } وإلا {
            CONST_BITOP_SET_FAILED("Mixture of different ops cannot be optimized", nodep);
        }
    }

    // CONSTRUCTORS
    Const_Bit_Op_شجرة_Visitor(Ast_عقدة_تعبير* nodep, unsigned externalOps, VNIdAllocator& ids)
        : m_ops{externalOps}
        , m_rootp{nodep}
        , m_ids{ids} {
        // Fill nullptr at [0] because a base index of 0 means not set yet
        m_varInfos.ادفع_back(nullptr);
        CONST_BITOP_RETURN_IF(!is_And_شجرة() && !is_Or_شجرة() && !is_Xor_شجرة(), nodep);
        إذا (Ast_عقدة_Biop* ثابت biopp = VN_CAST(nodep, عقدة_Biop)) {
            iterateConst(biopp);
        } وإلا {
            UASSERT_OBJ(VN_IS(nodep, RedXor), nodep, "Must be RedXor");
            incrOps(nodep, __LINE__);
            iterateChildrenConst(nodep);
        }
        لكل (auto&& entry : m_bitPolarities) {
            getVarInfo(entry.m_info).setPolarity(entry.m_polarity, entry.m_bit);
        }
        UASSERT_OBJ(is_Xor_شجرة() || m_polarity, nodep, "must be the original polarity");
    }
    virtual ~Const_Bit_Op_شجرة_Visitor() = افتراضي;
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
    ساكن Ast_عقدة_تعبير* simplify(Ast_عقدة_تعبير* nodep, صحيح resultWidth, unsigned externalOps,
                                 VDouble0& reduction, VNIdAllocator& ids) {
        UASSERT_OBJ(1 <= resultWidth && resultWidth <= 64, nodep, "resultWidth out of range");

        // Walk tree, gathering all terms referenced in expression
        ثابت Const_Bit_Op_شجرة_Visitor visitor{nodep, externalOps, ids};

        // If failed on root node is not optimizable, or there are no variable terms, then done
        إذا (visitor.m_failed || visitor.m_varInfos.size() == 1) أرجع nullptr;

        // FileLine used for constructing all new nodes in this function
        File_سطر* ثابت fl = nodep->fileline();

        // Get partial result each term referenced, count total number of ops and keep track of
        // whether we have clean/dirty terms. visitor.m_varInfos appears in deterministic order,
        // so the optimized tree is deterministic as well.

        std::vector<Ast_عقدة_تعبير*> termps;
        termps.reserve(visitor.m_varInfos.size() - 1);
        unsigned resultOps = 0;
        منطقي hasCleanTerm = خطأ;
        منطقي hasDirtyTerm = خطأ;

        لكل (auto&& v : visitor.m_varInfos) {
            إذا (!v) استمر;  // Skip nullptr at m_varInfos[0]
            إذا (v->hasConstResult()) {
                // If a constant term is known, we can either drop it or the whole tree is constant
                Ast_عقدة_تعبير* resultp = nullptr;
                إذا (v->getConstResult()) {
                    UASSERT_OBJ(visitor.is_Or_شجرة(), nodep,
                                "Only OR tree can yield known 1 result");
                    UINFO(9, "OR tree with const 1 term: " << v->refp());
                    // Known 1 bit in OR tree, whole result is 1
                    resultp = جديد AstConst{fl, AstConst::BitTrue{}};
                } وإلا إذا (visitor.is_And_شجرة()) {
                    UINFO(9, "AND tree with const 0 term: " << v->refp());
                    // Known 0 bit in AND tree, whole result is 0
                    resultp = جديد AstConst{fl, AstConst::BitFalse{}};
                } وإلا {
                    // Known 0 bit in OR or XOR tree. Ignore it.
                    استمر;
                }
                // Set width and widthMin precisely
                resultp->dtypeChgWidth(resultWidth, 1);
                لكل (Ast_عقدة* ثابت termp : termps) VL_DO_DANGLING(termp->delete_شجرة(), termp);
                أرجع resultp;
            }
            ثابت ResultTerm result = v->getResultTerm();
            termps.ادفع_back(std::get<0>(result));
            resultOps += std::get<1>(result);
            إذا (std::get<2>(result)) {
                hasCleanTerm = true;
                UINFO(9, "Clean term: " << termps.back());
            } وإلا {
                hasDirtyTerm = true;
                UINFO(9, "Dirty term: " << termps.back());
            }
        }

        // Group by FrozenNodeInfo
        std::map<Frozen_عقدة_Info, std::vector<Ast_عقدة_تعبير*>> frozenNodes;
        // Check if frozen terms are clean or not
        لكل (ثابت auto& frozenInfo : visitor.m_frozenNodes) {
            Ast_عقدة_تعبير* ثابت termp = frozenInfo.first;
            // Comparison operators are clean
            إذا ((VN_IS(termp, Eq) || VN_IS(termp, Neq) || VN_IS(termp, Lt) || VN_IS(termp, Lte)
                 || VN_IS(termp, Gt) || VN_IS(termp, Gte))
                && frozenInfo.second.m_lsb == 0) {
                hasCleanTerm = true;
            } وإلا {
                // Otherwise, conservatively assume the frozen term is dirty
                hasDirtyTerm = true;
                UINFO(9, "Dirty frozen term: " << termp);
            }
            frozenNodes[frozenInfo.second].ادفع_back(termp);
        }

        // Figure out if a final negation is required
        ثابت منطقي needsFlip = visitor.is_Xor_شجرة() && !visitor.m_polarity;

        // Figure out if the final tree needs cleaning
        ثابت منطقي needsCleaning = visitor.is_And_شجرة() ? !hasCleanTerm : hasDirtyTerm;

        // Add size of reduction tree to op count
        resultOps += termps.size() - 1;
        لكل (ثابت auto& lsbAndNodes : frozenNodes) {
            إذا (lsbAndNodes.first.m_lsb > 0) ++resultOps;  // Needs AstShiftR
            إذا (!lsbAndNodes.first.m_polarity) ++resultOps;  // Needs AstNot
            resultOps += lsbAndNodes.second.size();
        }
        // Add final polarity flip in Xor tree
        إذا (needsFlip) ++resultOps;
        // Add final cleaning AND
        إذا (needsCleaning) ++resultOps;

        إذا (debug() >= 9) {  // LCOV_EXCL_START
            cout << "-  Bitop tree considered:\n";
            لكل (ثابت Ast_عقدة_تعبير* ثابت termp : termps) termp->dump_شجرة("-  Reduced term: ");
            لكل (ثابت std::pair<Ast_عقدة_تعبير*, Frozen_عقدة_Info>& termp : visitor.m_frozenNodes) {
                termp.first->dump_شجرة("-  Frozen term with lsb "
                                      + std::to_string(termp.second.m_lsb) + " polarity "
                                      + std::to_string(termp.second.m_polarity) + ": ");
            }
            cout << "-  Needs flipping: " << needsFlip << "\n";
            cout << "-  Needs cleaning: " << needsCleaning << "\n";
            cout << "-  Size: " << resultOps << " input size: " << visitor.m_ops << "\n";
        }  // LCOV_EXCL_STOP

        // Sometimes we have no terms left after ignoring redundant terms
        // (all of which were zeroes)
        إذا (termps.empty() && visitor.m_frozenNodes.empty()) {
            reduction += visitor.m_ops;
            Ast_عقدة_تعبير* ثابت resultp = needsFlip ? جديد AstConst{fl, AstConst::BitTrue{}}
                                                   : جديد AstConst{fl, AstConst::BitFalse{}};
            resultp->dtypeChgWidth(resultWidth, 1);
            أرجع resultp;
        }

        // Only substitute the result if beneficial as determined by operation count
        إذا (visitor.m_ops <= resultOps) {
            لكل (Ast_عقدة* ثابت termp : termps) VL_DO_DANGLING(termp->delete_شجرة(), termp);
            أرجع nullptr;
        }

        // Update statistics
        reduction += visitor.m_ops - resultOps;

        // Reduction op to combine terms
        ثابت auto reduce = [&visitor, fl](Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp) -> Ast_عقدة_تعبير* {
            إذا (!lhsp) أرجع rhsp;
            إذا (visitor.is_And_شجرة()) {
                أرجع جديد AstAnd{fl, lhsp, rhsp};
            } وإلا إذا (visitor.is_Or_شجرة()) {
                أرجع جديد AstOr{fl, lhsp, rhsp};
            } وإلا {
                أرجع جديد AstXor{fl, lhsp, rhsp};
            }
        };

        // Compute result by reducing all terms
        Ast_عقدة_تعبير* resultp = nullptr;
        لكل (Ast_عقدة_تعبير* ثابت termp : termps) {  //
            resultp = reduce(resultp, termp);
        }
        // Add any frozen terms to the reduction
        لكل (auto&& nodes : frozenNodes) {
            // nodes.second has same lsb and polarity
            Ast_عقدة_تعبير* termp = nullptr;
            لكل (Ast_عقدة_تعبير* ثابت itemp : nodes.second) {
                termp = reduce(termp, itemp->unlinkFrBack());
            }
            إذا (nodes.first.m_lsb > 0) {  // LSB is not 0, so shiftR
                Ast_عقدة_DType* ثابت dtypep = termp->dtypep();
                termp = جديد AstShiftR{termp->fileline(), termp,
                                      جديد AstConst(termp->fileline(), AstConst::Widthed_قيمة{},
                                                   termp->width(), nodes.first.m_lsb)};
                termp->dtypep(dtypep);
            }
            إذا (!nodes.first.m_polarity) {  // Polarity is inverted, so append Not
                Ast_عقدة_DType* ثابت dtypep = termp->dtypep();
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
        إذا (needsFlip) {
            إذا (needsCleaning) {
                // Cleaning will be added below. Use a NOT which is a byte shorter on x86
                resultp = جديد AstNot{fl, resultp};
            } وإلا {
                // Keep result clean by using XOR(1, _)
                AstConst* ثابت maskp = جديد AstConst{fl, AstConst::Widthed_قيمة{}, maskWidth, 1};
                resultp = جديد AstXor{fl, maskp, resultp};
            }
        }

        // Apply final cleaning
        إذا (needsCleaning) {
            AstConst* ثابت maskp = جديد AstConst{fl, AstConst::Widthed_قيمة{}, maskWidth, 1};
            resultp = جديد AstAnd{fl, maskp, resultp};
        }

        // Cast back to original size if required
        إذا (resultp->width() != resultWidth) {
            resultp = جديد AstCCast{fl, resultp, resultWidth, 1};
        }

        // Set width and widthMin precisely
        resultp->dtypeChgWidth(resultWidth, 1);

        أرجع resultp;
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
    منطقي m_params = خطأ;  // If true, propagate parameterized and true numbers only
    منطقي m_required = خطأ;  // If true, must become a constant
    منطقي m_wremove = true;  // Inside scope, no assignw removal
    منطقي m_warn = خطأ;  // Output warnings
    منطقي m_doExpensive = خطأ;  // Enable computationally expensive optimizations
    منطقي m_doCpp = خطأ;  // Enable late-stage C++ optimizations
    منطقي m_doNConst = خطأ;  // Enable non-constant-child simplifications
    منطقي m_doV = خطأ;  // Verilog, not C++ conversion
    منطقي m_doGenerate = خطأ;  // Postpone width checking inside generate
    منطقي m_convertLogicToBit = خطأ;  // Convert logical operators to bitwise
    منطقي m_hasJumpDelay = خطأ;  // JumpGo or Delay under this loop
    منطقي m_hasLoopTest = خطأ;  // Contains AstLoopTest
    منطقي m_underRecFunc = خطأ;  // Under a recursive function
    Ast_عقدة_Module* m_modp = nullptr;  // Current module
    ثابت AstArraySel* m_selp = nullptr;  // Current select
    ثابت AstScope* m_scopep = nullptr;  // Current scope
    ثابت AstAttrOf* m_attrp = nullptr;  // Current attribute
    VDouble0 m_statBitOpReduction;  // Ops reduced in ConstBitOpTreeVisitor
    VDouble0 m_statConcatMerge;  // Concat merges
    VDouble0 m_stat_Cond_تعبير_Redundant;  // Conditional repeated expressions
    VDouble0 m_stat_If_Cond_تعبير_Redundant;  // Conditional repeated expressions
    VDouble0 m_statMemberAccessVisits;  // Nodes visited by containsMemberAccessRecurse
    ثابت منطقي m_globalPass;  // ConstVisitor invoked as a global pass
    ساكن uint32_t s_global_Pass_رقم;  // Counts number of times ConstVisitor invoked as global pass
    V3UniqueNames m_concswapNames;  // For generating unique temporary variable names
    // Caches results of matchBiopToBitwise, keyed on node id
    std::unordered_map<size_t, منطقي> m_containsMemberAccess;
    // Items of enum currently being recursed into. Cannot use VNIdAllocator, as this runs in
    // every mode, including those invoked under a pass holding user4 (V3Param). Node pointers
    // are safe as keys here, unlike elsewhere in V3Const: folding the item value below does
    // free nodes whose addresses are then reused, but AstEnumItem is only ever constructed
    // while parsing (verilog.y and V3LinkParse), so a reused address can never become one,
    // and every key looked up is an AstEnumItem.
    std::unordered_set<ثابت AstEnumItem*> m_recursing_Enum_عناصر;
    std::unordered_set<AstJumpBlock*> m_usedJumpBlocks;  // JumpBlocks used by some JumpGo

    // METHODS

    // Return true if statements may be deleted. Until statics are lifted, a variable declared
    // in a statement might be referenced from elsewhere (e.g. hierarchically).
    ساكن منطقي canDeleteStatements() { أرجع v3Global.staticsLifted(); }

    فراغ deleteVarScopesUnder(Ast_عقدة* subtreep) {
        إذا (!subtreep) أرجع;
        إذا (!m_scopep) أرجع;
        std::unordered_set<AstVar*> varps;
        subtreep->foreachAndNext([&](AstVar* varp) { varps.أدرج(varp); });
        إذا (varps.empty()) أرجع;
        لكل (AstVarScope *vscp = m_scopep->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            إذا (varps.find(vscp->varp()) != varps.end()) {
                VL_DO_DANGLING(ادفع_Deletep(vscp->unlinkFrBack()), vscp);
            }
        }
    }

    V3_رقم const_رقم_V(Ast_عقدة* nodep) {
        // Contract C width to V width (if needed, else just direct copy)
        // The upper zeros in the C representation can otherwise cause
        // wrong results in some operations, e.g. MulS
        ثابت V3_رقم& numc = VN_AS(nodep, Const)->رقم();
        أرجع !numc.is_رقم() ? numc : V3_رقم{nodep, nodep->widthMinV(), numc};
    }
    V3_رقم to_رقم_C(Ast_عقدة* nodep, ثابت V3_رقم& numv) {
        // Extend V width back to C width for given node
        أرجع !numv.is_رقم() ? numv : V3_رقم{nodep, nodep->width(), numv};
    }

    ساكن منطقي lowerAsFixedAggregate(ثابت Ast_عقدة_DType* ثابت dtypep) {
        أرجع dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }

    AstStructSel* newStructSel(Ast_عقدة_تعبير* ثابت fromp, ثابت AstMemberDType* ثابت itemp) {
        AstStructSel* ثابت selp = جديد AstStructSel{fromp->fileline(), fromp, itemp->name()};
        selp->dtypeFrom(itemp->dtypep());
        أرجع selp;
    }

    فراغ collectFixedAggregateTerms(Ast_عقدة_تعبير* ثابت fromp, std::vector<Ast_عقدة_تعبير*>& termps,
                                    ثابت منطقي packReal) {
        ثابت Ast_عقدة_DType* ثابت dtypep = fromp->dtypep()->skipRefp();
        إذا (ثابت AstUnpackArrayDType* ثابت unpackDtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            لكل (ثابت صحيح idx : unpackDtypep->declRange().seq_يسار_To_يمين()) {
                AstArraySel* ثابت selp
                    = جديد AstArraySel{fromp->fileline(), fromp->clone_شجرة_Pure(خطأ), idx};
                collectFixedAggregateTerms(selp, termps, packReal);
            }
            VL_DO_DANGLING(ادفع_Deletep(fromp), fromp);
        } وإلا إذا (ثابت Ast_عقدة_UOr_Struct_DType* ثابت sdtypep
                   = VN_CAST(dtypep, عقدة_UOr_Struct_DType)) {
            إذا (sdtypep->packed()) {
                termps.ادفع_back(fromp);
                أرجع;
            }
            لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                collectFixedAggregateTerms(newStructSel(fromp->clone_شجرة_Pure(خطأ), itemp),
                                           termps, packReal);
            }
            VL_DO_DANGLING(ادفع_Deletep(fromp), fromp);
        } وإلا إذا (packReal && dtypep->isDouble()) {
            termps.ادفع_back(جديد AstRealToBits{fromp->fileline(), fromp});
        } وإلا {
            termps.ادفع_back(fromp);
        }
    }

    Ast_عقدة_تعبير* packFixedAggregate(Ast_عقدة_تعبير* ثابت fromp) {
        std::vector<Ast_عقدة_تعبير*> termps;
        collectFixedAggregateTerms(fromp, termps, true);
        UASSERT(!termps.empty(), "No stream terms");
        Ast_عقدة_تعبير* resultp = termps[0];
        لكل (size_t i = 1; i < termps.size(); ++i) {
            resultp = جديد AstConcat{resultp->fileline(), resultp, termps[i]};
        }
        أرجع resultp;
    }

    فراغ replaceAssignToFixedAggregate(Ast_عقدة_Assign* ثابت nodep, Ast_عقدة_تعبير* ثابت dstp,
                                       Ast_عقدة_تعبير* srcp) {
        ثابت صحيح dstWidth = dstp->dtypep()->widthStream();
        std::vector<Ast_عقدة_تعبير*> termps;
        collectFixedAggregateTerms(dstp, termps, خطأ);
        صحيح srcWidth = srcp->width();
        إذا (srcWidth < dstWidth) {
            AstExtend* ثابت extendp = جديد AstExtend{srcp->fileline(), srcp, dstWidth};
            srcp = جديد AstShiftL{
                extendp->fileline(), extendp,
                جديد AstConst{extendp->fileline(), static_cast<uint32_t>(dstWidth - srcWidth)},
                dstWidth};
            srcWidth = dstWidth;
        }
        Ast_عقدة_Assign* newp = nullptr;
        صحيح offset = 0;
        لكل (size_t i = 0; i < termps.size(); ++i) {
            Ast_عقدة_تعبير* ثابت termp = termps[i];
            ثابت صحيح width = termp->dtypep()->widthStream();
            ثابت صحيح lsb = srcWidth - offset - width;
            offset += width;
            Ast_عقدة_تعبير* rhsp
                = جديد AstSel{srcp->fileline(), srcp->clone_شجرة_Pure(خطأ), lsb, width};
            إذا (termp->dtypep()->skipRefp()->isDouble()) {
                rhsp = جديد AstBitsToRealD{rhsp->fileline(), rhsp};
            }
            Ast_عقدة_Assign* ثابت assignp = nodep->cloneType(termp, rhsp);
            assignp->dtypeFrom(termp);
            newp = Ast_عقدة::addNext(newp, assignp);
        }
        nodep->addNextHere(newp);
        VL_DO_DANGLING(ادفع_Deletep(srcp), srcp);
        VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
    }

    منطقي operandConst(ثابت Ast_عقدة* nodep) { أرجع VN_IS(nodep, Const); }
    منطقي operandAsvConst(ثابت Ast_عقدة* nodep) {
        // BIASV(CONST, BIASV(CONST,...)) -> BIASV( BIASV_CONSTED(a,b), ...)
        ثابت Ast_عقدة_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, عقدة_Bi_Com_Asv);
        إذا (!bnodep) أرجع خطأ;
        إذا (!VN_IS(bnodep->lhsp(), Const)) أرجع خطأ;
        ثابت Ast_عقدة_Bi_Com_Asv* ثابت rnodep = VN_CAST(bnodep->rhsp(), عقدة_Bi_Com_Asv);
        إذا (!rnodep) أرجع خطأ;
        إذا (rnodep->type() != bnodep->type()) أرجع خطأ;
        إذا (rnodep->width() != bnodep->width()) أرجع خطأ;
        إذا (rnodep->lhsp()->width() != bnodep->lhsp()->width()) أرجع خطأ;
        إذا (!VN_IS(rnodep->lhsp(), Const)) أرجع خطأ;
        أرجع true;
    }
    منطقي operandAsvSame(ثابت Ast_عقدة* nodep) {
        // BIASV(SAMEa, BIASV(SAMEb,...)) -> BIASV( BIASV(SAMEa,SAMEb), ...)
        ثابت Ast_عقدة_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, عقدة_Bi_Com_Asv);
        إذا (!bnodep) أرجع خطأ;
        ثابت Ast_عقدة_Bi_Com_Asv* ثابت rnodep = VN_CAST(bnodep->rhsp(), عقدة_Bi_Com_Asv);
        إذا (!rnodep) أرجع خطأ;
        إذا (rnodep->type() != bnodep->type()) أرجع خطأ;
        إذا (rnodep->width() != bnodep->width()) أرجع خطأ;
        أرجع operandsSame(bnodep->lhsp(), rnodep->lhsp());
    }
    منطقي operandAsvLUp(ثابت Ast_عقدة* nodep) {
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
        ثابت Ast_عقدة_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, عقدة_Bi_Com_Asv);
        إذا (!bnodep) أرجع خطأ;
        ثابت Ast_عقدة_Bi_Com_Asv* ثابت lnodep = VN_CAST(bnodep->lhsp(), عقدة_Bi_Com_Asv);
        إذا (!lnodep) أرجع خطأ;
        إذا (lnodep->type() != bnodep->type()) أرجع خطأ;
        إذا (lnodep->width() != bnodep->width()) أرجع خطأ;
        أرجع VN_IS(lnodep->lhsp(), Const);
    }
    منطقي operandAsvRUp(ثابت Ast_عقدة* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr)) ?
        ثابت Ast_عقدة_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, عقدة_Bi_Com_Asv);
        إذا (!bnodep) أرجع خطأ;
        ثابت Ast_عقدة_Bi_Com_Asv* ثابت rnodep = VN_CAST(bnodep->rhsp(), عقدة_Bi_Com_Asv);
        إذا (!rnodep) أرجع خطأ;
        إذا (rnodep->type() != bnodep->type()) أرجع خطأ;
        إذا (rnodep->width() != bnodep->width()) أرجع خطأ;
        أرجع VN_IS(rnodep->lhsp(), Const);
    }
    ساكن منطقي operandSubAdd(ثابت Ast_عقدة* nodep) {
        // SUB( ADD(CONSTx,y), CONSTz) -> ADD(SUB(CONSTx,CONSTz), y)
        ثابت Ast_عقدة_Biop* ثابت np = VN_CAST(nodep, عقدة_Biop);
        ثابت Ast_عقدة_Biop* ثابت lp = VN_CAST(np->lhsp(), عقدة_Biop);
        أرجع (lp && VN_IS(lp->lhsp(), Const) && VN_IS(np->rhsp(), Const)
                && lp->width() == np->width());
    }
    منطقي matchRedundantClean(AstAnd* andp) {
        // Remove And with constant one inserted by V3Clean
        // 1 & (a == b)  -> (IData)(a == b)
        // When bool is casted to int, the value is either 0 or 1
        AstConst* ثابت constp = VN_AS(andp->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isOne(), andp->lhsp(), "TRREEOPC must meet this condition");
        Ast_عقدة_تعبير* ثابت rhsp = andp->rhsp();
        AstCCast* ccastp = nullptr;
        ثابت auto isEqOrNeq = [](ثابت Ast_عقدة* nodep) -> منطقي {  //
            أرجع VN_IS(nodep, Eq) || VN_IS(nodep, Neq);
        };
        إذا (isEqOrNeq(rhsp)) {
            ccastp = جديد AstCCast{andp->fileline(), rhsp->unlinkFrBack(), andp};
        } وإلا إذا (AstCCast* ثابت tmpp = VN_CAST(rhsp, CCast)) {
            إذا (isEqOrNeq(tmpp->lhsp())) {
                إذا (tmpp->width() == andp->width()) {
                    tmpp->unlinkFrBack();
                    ccastp = tmpp;
                } وإلا {
                    ccastp = جديد AstCCast{andp->fileline(), tmpp->lhsp()->unlinkFrBack(), andp};
                }
            }
        }
        إذا (ccastp) {
            andp->replaceWithKeepDType(ccastp);
            VL_DO_DANGLING(ادفع_Deletep(andp), andp);
            أرجع true;
        }
        أرجع خطأ;
    }

    ساكن منطقي operandAndOrSame(ثابت Ast_عقدة* nodep) {
        // OR( AND(VAL,x), AND(VAL,y)) -> AND(VAL,OR(x,y))
        // OR( AND(x,VAL), AND(y,VAL)) -> AND(OR(x,y),VAL)
        ثابت Ast_عقدة_Biop* ثابت np = VN_CAST(nodep, عقدة_Biop);
        ثابت Ast_عقدة_Biop* ثابت lp = VN_CAST(np->lhsp(), عقدة_Biop);
        ثابت Ast_عقدة_Biop* ثابت rp = VN_CAST(np->rhsp(), عقدة_Biop);
        أرجع (lp && rp && lp->width() == rp->width() && lp->type() == rp->type()
                && (operandsSame(lp->lhsp(), rp->lhsp()) || operandsSame(lp->rhsp(), rp->rhsp())));
    }
    منطقي matchOrAndNot(Ast_عقدة_Biop* nodep) {
        // AstOr{$a, AstAnd{AstNot{$b}, $c}} if $a.width1, $a==$b => AstOr{$a,$c}
        // Someday we'll sort the biops completely and this can be simplified
        // This often results from our simplified clock generation:
        // if (rst) ... else if (enable)... -> OR(rst,AND(!rst,enable))
        ثابت Ast_عقدة_تعبير* ap;
        Ast_عقدة_Biop* andp;
        إذا (VN_IS(nodep->lhsp(), And)) {
            andp = VN_AS(nodep->lhsp(), And);
            ap = nodep->rhsp();
        } وإلا إذا (VN_IS(nodep->rhsp(), And)) {
            andp = VN_AS(nodep->rhsp(), And);
            ap = nodep->lhsp();
        } وإلا {
            أرجع خطأ;
        }
        ثابت Ast_عقدة_Uniop* notp;
        Ast_عقدة_تعبير* cp;
        إذا (VN_IS(andp->lhsp(), Not)) {
            notp = VN_AS(andp->lhsp(), Not);
            cp = andp->rhsp();
        } وإلا إذا (VN_IS(andp->rhsp(), Not)) {
            notp = VN_AS(andp->rhsp(), Not);
            cp = andp->lhsp();
        } وإلا {
            أرجع خطأ;
        }
        ثابت Ast_عقدة_تعبير* ثابت bp = notp->lhsp();
        إذا (!operandsSame(ap, bp)) أرجع خطأ;
        // Do it
        cp->unlinkFrBack();
        VL_DO_DANGLING(ادفع_Deletep(andp->unlinkFrBack()), andp);
        VL_DANGLING(notp);
        // Replace whichever branch is now dangling
        إذا (nodep->rhsp()) {
            nodep->lhsp(cp);
        } وإلا {
            nodep->rhsp(cp);
        }
        أرجع true;
    }
    منطقي matchAndCond(AstAnd* nodep) {
        // Push down a AND into conditional, when one side of conditional is constant
        // (otherwise we'd be trading one operation for two operations)
        // V3Clean often makes this pattern, as it postpones the AND until
        // as high as possible, which is usually the right choice, except for this.
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstCond* ثابت condp = VN_CAST(nodep->rhsp(), Cond);
        إذا (!condp) أرجع خطأ;
        إذا (!VN_IS(condp->thenp(), Const) && !VN_IS(condp->elsep(), Const)) أرجع خطأ;
        AstConst* ثابت maskp = VN_CAST(nodep->lhsp(), Const);
        إذا (!maskp) أرجع خطأ;
        UINFO(4, "AND(CONSTm, CONDcond(c, i, e))->CONDcond(c, AND(m,i), AND(m, e)) " << nodep);
        AstCond* ثابت newp = جديد AstCond{
            condp->fileline(), condp->condp()->unlinkFrBack(),
            جديد AstAnd{nodep->fileline(), maskp->clone_شجرة(خطأ), condp->thenp()->unlinkFrBack()},
            جديد AstAnd{nodep->fileline(), maskp->clone_شجرة(خطأ),
                       condp->elsep()->unlinkFrBack()}};
        newp->thenp()->dtypeFrom(nodep);  // As And might have been to change widths
        newp->elsep()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        أرجع true;
    }

    منطقي matchCondCond(ثابت AstCond* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        إذا (VN_IS(nodep->backp(), Cond)) أرجع خطأ;  // Was checked when visited parent
        إذا (!VN_IS(nodep->thenp(), Cond) && !VN_IS(nodep->elsep(), Cond))
            أرجع خطأ;  // Short circuit
        std::vector<Ast_عقدة_تعبير*> truesp;
        std::vector<Ast_عقدة_تعبير*> falsesp;
        matchCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
        أرجع خطأ;  // Can optimize further
    }
    فراغ matchCondCondRecurse(ثابت AstCond* nodep, std::vector<Ast_عقدة_تعبير*>& truesp,
                              std::vector<Ast_عقدة_تعبير*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        إذا (truesp.size() > 4 || falsesp.size() > 4) أرجع;
        إذا (!nodep->condp()->isPure()) أرجع;
        منطقي replaced = خطأ;
        لكل (ثابت Ast_عقدة_تعبير* condp : truesp) {
            إذا (replaced) اكسر;
            إذا (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_رقم(nodep->condp(), 1);
            replaced = true;
            ++m_stat_Cond_تعبير_Redundant;
        }
        لكل (ثابت Ast_عقدة_تعبير* condp : falsesp) {
            إذا (replaced) اكسر;
            إذا (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = true;
            ++m_stat_Cond_تعبير_Redundant;
        }
        إذا (ثابت AstCond* subCondp = VN_CAST(nodep->thenp(), Cond)) {
            إذا (!replaced) truesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            إذا (!replaced) truesp.اسحب_back();
        }
        إذا (ثابت AstCond* subCondp = VN_CAST(nodep->elsep(), Cond)) {
            إذا (!replaced) falsesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            إذا (!replaced) falsesp.اسحب_back();
        }
    }
    فراغ matchIfCondCond(ثابت Ast_عقدة_If* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        إذا (VN_IS(nodep->backp(), If)) أرجع;  // Was checked when visited parent
        إذا (!VN_IS(nodep->thensp(), If) && !VN_IS(nodep->elsesp(), If)) أرجع;  // Short circuit
        std::vector<Ast_عقدة_تعبير*> truesp;
        std::vector<Ast_عقدة_تعبير*> falsesp;
        matchIfCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
    }
    فراغ matchIfCondCondRecurse(ثابت Ast_عقدة_If* nodep, std::vector<Ast_عقدة_تعبير*>& truesp,
                                std::vector<Ast_عقدة_تعبير*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        إذا (truesp.size() > 4 || falsesp.size() > 4) أرجع;
        إذا (!nodep->condp()->isPure()) أرجع;
        // A constant condition is folded by visit(AstNodeIf), once statements can be deleted
        إذا (VN_IS(nodep->condp(), Const)) أرجع;
        منطقي replaced = خطأ;
        لكل (ثابت Ast_عقدة_تعبير* condp : truesp) {
            إذا (replaced) اكسر;
            إذا (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_رقم(nodep->condp(), 1);
            replaced = true;
            ++m_stat_If_Cond_تعبير_Redundant;
        }
        لكل (ثابت Ast_عقدة_تعبير* condp : falsesp) {
            إذا (replaced) اكسر;
            إذا (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = true;
            ++m_stat_If_Cond_تعبير_Redundant;
        }
        // We only check the first statement of parent IF is an If
        // So we don't need to check for effects in the executing thensp/elsesp
        // altering the child't condition.  e.g. 'if (x) begin x=1; if (x) end'
        إذا (ثابت Ast_عقدة_If* subIfp = VN_CAST(nodep->thensp(), عقدة_If)) {
            إذا (!replaced) truesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            إذا (!replaced) truesp.اسحب_back();
        }
        إذا (ثابت Ast_عقدة_If* subIfp = VN_CAST(nodep->elsesp(), عقدة_If)) {
            إذا (!replaced) falsesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            إذا (!replaced) falsesp.اسحب_back();
        }
    }

    فراغ matchIfSameLast(Ast_عقدة_If* nodep) {
        إذا (!nodep->thensp()) أرجع;
        إذا (!nodep->elsesp()) أرجع;
        إذا (!canDeleteStatements()) أرجع;
        // Iterate bodies in reverse
        Ast_عقدة* tp = nodep->thensp()->lastp();
        Ast_عقدة* ep = nodep->elsesp()->lastp();
        طالما (tp && ep && tp->same_شجرة(ep)) {
            UASSERT_OBJ(!tp->nextp(), tp, "Expected no nextp");
            UASSERT_OBJ(!ep->nextp(), ep, "Expected no nextp");
            Ast_عقدة* ثابت tPrevp = tp->prevp();
            Ast_عقدة* ثابت ePrevp = ep->prevp();
            nodep->addNextHere(tp->unlinkFrBack());
            VL_DO_DANGLING(ادفع_Deletep(ep->unlinkFrBack()), ep);
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
        ثابت uint32_t significantBits = constp->رقم().widthToFit();

        AstOr* ثابت orp = VN_AS(nodep->rhsp(), Or);

        // Predicate for checking whether the bottom 'significantBits' bits of the given expression
        // are all zeroes.
        ثابت auto checkBottomClear = [=](ثابت Ast_عقدة* nodep) -> منطقي {
            إذا (ثابت AstShiftL* ثابت shiftp = VN_CAST(nodep, ShiftL)) {
                إذا (ثابت AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                    أرجع scp->رقم().toUInt() >= significantBits;
                }
            }
            أرجع خطأ;
        };

        ثابت منطقي orLIsRedundant = checkBottomClear(orp->lhsp());
        ثابت منطقي orRIsRedundant = checkBottomClear(orp->rhsp());

        إذا (orLIsRedundant && orRIsRedundant) {
            nodep->replaceWithKeepDType(
                جديد AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            أرجع true;
        } وإلا إذا (orLIsRedundant) {
            orp->replaceWithKeepDType(orp->rhsp()->unlinkFrBack());
            VL_DO_DANGLING(ادفع_Deletep(orp), orp);
            أرجع خطأ;  // input node is still valid, keep going
        } وإلا إذا (orRIsRedundant) {
            orp->replaceWithKeepDType(orp->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(ادفع_Deletep(orp), orp);
            أرجع خطأ;  // input node is still valid, keep going
        } وإلا {
            أرجع خطأ;
        }
    }
    منطقي matchMaskedShift(AstAnd* nodep) {
        // Drop redundant masking of right shift result. E.g: 0xff & ((uint32_t)a >> 24). This
        // commonly appears after V3Expand and the simplification in matchMaskedOr. Similarly,
        // drop redundant masking of left shift result. E.g.: 0xff000000 & ((uint32_t)a << 24).

        ثابت auto checkMask = [nodep, هذا](ثابت V3_رقم& mask) -> منطقي {
            ثابت AstConst* ثابت constp = VN_AS(nodep->lhsp(), Const);
            إذا (constp->رقم().isCaseEq(mask)) {
                Ast_عقدة* ثابت rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                nodep->replaceWithKeepDType(rhsp);
                VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
                أرجع true;
            }
            أرجع خطأ;
        };

        // Check if masking is redundant
        إذا (ثابت AstShiftR* ثابت shiftp = VN_CAST(nodep->rhsp(), ShiftR)) {
            إذا (ثابت AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_رقم maskLo{nodep, nodep->width()};
                maskLo.setMask(nodep->width() - scp->رقم().toUInt());
                أرجع checkMask(maskLo);
            }
        } وإلا إذا (ثابت AstShiftL* ثابت shiftp = VN_CAST(nodep->rhsp(), ShiftL)) {
            إذا (ثابت AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_رقم mask{nodep, nodep->width()};
                ثابت uint32_t shiftAmount = scp->رقم().toUInt();
                mask.setMask(nodep->width() - shiftAmount, shiftAmount);
                أرجع checkMask(mask);
            }
        }
        أرجع خطأ;
    }

    منطقي matchMaskedZero(ثابت AstAnd* nodep) {
        // Turn masking of known zero bits into constant zero. Commonly appears after V3Expand.
        ثابت AstConst* ثابت maskp = VN_AS(nodep->lhsp(), Const);
        ثابت Ast_عقدة_تعبير* ثابت rhsp = nodep->rhsp();
        ثابت uint32_t msbP1 = maskp->رقم().mostSetBitP1();
        إذا (!msbP1) أرجع خطأ;  // Don't rewrite, separate rule matches for this
        ثابت uint32_t msb = msbP1 - 1;
        ثابت uint32_t lsb = maskp->رقم().leastSetBitP1() - 1;

        إذا (ثابت AstShiftL* ثابت shiftp = VN_CAST(rhsp, ShiftL)) {
            // 'a << S' forces the low S bits to zero
            إذا (AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                أرجع scp->رقم().fitsInUInt()  //
                       && (scp->رقم().toUInt() > msb);
            }
        }
        إذا (ثابت AstShiftR* ثابت shiftp = VN_CAST(rhsp, ShiftR)) {
            // 'a >> S' forces the high S bits to zero. Check against the width of the shifted
            // operand, V3Expand can create shifts wider than their inputs. Must use width(),
            // not widthMin(), as bits above widthMin() are not guaranteed to be zero.
            إذا (AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                أرجع scp->رقم().fitsInUInt()
                       && (lsb + scp->رقم().toUInt()
                           >= static_cast<uint32_t>(shiftp->lhsp()->width()));
            }
        }
        إذا (ثابت AstMul* ثابت mulp = VN_CAST(rhsp, Mul)) {
            // 'C * a' forces the low N bits to zero where 'C' has low zero bits
            إذا (AstConst* ثابت cp = VN_CAST(mulp->lhsp(), Const)) {
                أرجع cp->رقم().leastSetBitP1() > msb + 1;
            }
        }
        إذا (ثابت AstExtend* ثابت extendp = VN_CAST(rhsp, Extend)) {
            // Zero-extension forces the bits above the source width to zero
            أرجع lsb >= static_cast<uint32_t>(extendp->lhsp()->width());
        }

        أرجع خطأ;
    }

    منطقي match_Bit_Op_شجرة(Ast_عقدة_تعبير* nodep) {
        إذا (nodep->widthMin() != 1) أرجع خطأ;
        إذا (!v3Global.opt.f_Const_Bit_Op_شجرة()) أرجع خطأ;

        ثابت صحيح width = nodep->width();
        Ast_عقدة_تعبير* rootp = nodep;
        unsigned externalOps = 0;
        // Reach past a plain making AND
        إذا (ثابت AstAnd* ثابت andp = VN_CAST(nodep, And)) {
            إذا (isConst(andp->lhsp(), 1)) {
                rootp = andp->rhsp();
                externalOps = 1;
            }
        }

        // Only optimize if rootp is in fact the root of a tree of identical
        // operations, otherwise analysis on an eventually unoptimizable unablanced
        // tree can go O(N^2) as we would re-analyze every time as we move up the
        // tree, repeatedly re-discover the sub-tree is not optimizable.
        إذا (rootp->type() == nodep->backp()->type()) أرجع خطأ;

        std::string debugPrefix;
        إذا (debug() >= 9) {  // LCOV_EXCL_START
            ساكن صحيح s_c = 0;
            debugPrefix = "-  matchBitOpTree[" + std::to_string(++s_c) + "] ";
            nodep->dump_شجرة(debugPrefix + "INPUT: ");
        }  // LCOV_EXCL_STOP

        Ast_عقدة_تعبير* ثابت newp = Const_Bit_Op_شجرة_Visitor::simplify(rootp, width, externalOps,
                                                                  m_statBitOpReduction, m_ids);

        إذا (newp) {
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Transformed leaf of bit tree to " << newp);
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        }

        إذا (debug() >= 9) {  // LCOV_EXCL_START
            إذا (newp) {
                newp->dump_شجرة(debugPrefix + "RESULT: ");
            } وإلا {
                cout << debugPrefix << "not replaced\n";
            }
        }  // LCOV_EXCL_STOP

        أرجع newp;
    }
    ساكن منطقي operandShiftSame(ثابت Ast_عقدة* nodep) {
        ثابت Ast_عقدة_Biop* ثابت np = VN_AS(nodep, عقدة_Biop);
        {
            ثابت AstShiftL* ثابت lp = VN_CAST(np->lhsp(), ShiftL);
            ثابت AstShiftL* ثابت rp = VN_CAST(np->rhsp(), ShiftL);
            إذا (lp && rp) {
                أرجع (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        {
            ثابت AstShiftR* ثابت lp = VN_CAST(np->lhsp(), ShiftR);
            ثابت AstShiftR* ثابت rp = VN_CAST(np->rhsp(), ShiftR);
            إذا (lp && rp) {
                أرجع (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        أرجع خطأ;
    }
    منطقي operandHugeShiftL(ثابت Ast_عقدة_Biop* nodep) {
        أرجع (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->رقم().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->رقم().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->width())))
                && nodep->lhsp()->isPure());
    }
    منطقي operandHugeShiftR(ثابت Ast_عقدة_Biop* nodep) {
        أرجع (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->رقم().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->رقم().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->lhsp()->width())))
                && nodep->lhsp()->isPure()
                && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)));
    }
    منطقي operandIsTwo(ثابت Ast_عقدة* nodep) {
        ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
        إذا (!constp) أرجع خطأ;  // not constant
        إذا (constp->رقم().isFourState()) أرجع خطأ;  // four-state
        إذا (nodep->width() > VL_QUADSIZE) أرجع خطأ;  // too wide
        إذا (nodep->isSigned() && constp->رقم().isNegative()) أرجع خطأ;  // signed and negative
        أرجع constp->toUQuad() == 2;
    }
    منطقي operandIsTwostate(ثابت Ast_عقدة* nodep) {
        أرجع (VN_IS(nodep, Const) && !VN_AS(nodep, Const)->رقم().isFourState());
    }
    منطقي operandIsPowTwo(ثابت Ast_عقدة* nodep) {
        إذا (!operandIsTwostate(nodep)) أرجع خطأ;
        أرجع (1 == VN_AS(nodep, Const)->رقم().عدد_Ones());
    }
    منطقي operandShiftOp(ثابت Ast_عقدة_Biop* nodep) {
        إذا (!VN_IS(nodep->rhsp(), Const)) أرجع خطأ;
        ثابت Ast_عقدة_Biop* ثابت lhsp = VN_CAST(nodep->lhsp(), عقدة_Biop);
        إذا (!lhsp || !(VN_IS(lhsp, And) || VN_IS(lhsp, Or) || VN_IS(lhsp, Xor))) أرجع خطأ;
        إذا (nodep->width() != lhsp->width()) أرجع خطأ;
        إذا (nodep->width() != lhsp->lhsp()->width()) أرجع خطأ;
        إذا (nodep->width() != lhsp->rhsp()->width()) أرجع خطأ;

        // Only do it if we can fruther reduce one of the sides if pulling the shift through:
        // - Either operands are constants
        إذا (VN_IS(lhsp->lhsp(), Const)) أرجع true;
        إذا (VN_IS(lhsp->rhsp(), Const)) أرجع true;
        // - Const shift on either side
        إذا (Ast_عقدة_Biop* ثابت llp = VN_CAST(lhsp->lhsp(), عقدة_Biop)) {
            أرجع (VN_IS(llp, ShiftL) || VN_IS(llp, ShiftR)) && VN_IS(llp->rhsp(), Const);
        }
        إذا (Ast_عقدة_Biop* ثابت lrp = VN_CAST(lhsp->rhsp(), عقدة_Biop)) {
            أرجع (VN_IS(lrp, ShiftL) || VN_IS(lrp, ShiftR)) && VN_IS(lrp->rhsp(), Const);
        }
        // Otherwise we would increase logic size
        أرجع خطأ;
    }
    منطقي operandShiftShift(ثابت Ast_عقدة_Biop* nodep) {
        // We could add a AND though.
        ثابت Ast_عقدة_Biop* ثابت lhsp = VN_CAST(nodep->lhsp(), عقدة_Biop);
        إذا (!lhsp || !(VN_IS(lhsp, ShiftL) || VN_IS(lhsp, ShiftR))) أرجع خطأ;
        // We can only get rid of a<<b>>c or a<<b<<c, with constant b & c
        // because bits may be masked in that process, or (b+c) may exceed the word width.
        إذا (!(VN_IS(nodep->rhsp(), Const) && VN_IS(lhsp->rhsp(), Const))) أرجع خطأ;
        إذا (VN_AS(nodep->rhsp(), Const)->رقم().isFourState()
            || VN_AS(lhsp->rhsp(), Const)->رقم().isFourState())
            أرجع خطأ;
        إذا (nodep->width() != lhsp->width()) أرجع خطأ;
        إذا (nodep->width() != lhsp->lhsp()->width()) أرجع خطأ;
        أرجع true;
    }
    منطقي operandWordOOB(ثابت AstWordSel* nodep) {
        // V3Expand may make a arraysel that exceeds the bounds of the array
        // It was an expression, then got constified.  In reality, the WordSel
        // must be wrapped in a Cond, that will be false.
        أرجع (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), عقدة_Var_Ref)
                && VN_AS(nodep->fromp(), عقدة_Var_Ref)->access().isReadOnly()
                && (static_cast<صحيح>(VN_AS(nodep->bitp(), Const)->toUInt())
                    >= VN_AS(nodep->fromp(), عقدة_Var_Ref)->varp()->width_كلمات()));
    }
    منطقي operandSelFull(ثابت AstSel* nodep) {
        أرجع (VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0
                && static_cast<صحيح>(nodep->widthConst()) == nodep->fromp()->width());
    }
    منطقي operandSelExtend(AstSel* nodep) {
        إذا (!m_doV) أرجع خطأ;
        AstExtend* ثابت extendp = VN_CAST(nodep->fromp(), Extend);
        إذا (!extendp) أرجع خطأ;
        AstConst* ثابت lsbp = VN_CAST(nodep->lsbp(), Const);
        إذا (!lsbp) أرجع خطأ;
        ثابت صحيح width = nodep->widthConst();
        ثابت صحيح lsb = lsbp->toSInt();
        ثابت صحيح msb = lsb + width - 1;
        Ast_عقدة_تعبير* ثابت lhsp = extendp->lhsp();
        إذا (!lhsp->isPure()) أرجع خطأ;

        // Selecting the entire extended expression, replace with that
        إذا (lsb == 0 && msb == lhsp->width() - 1) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(lhsp);
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            أرجع true;
        }
        // Select entirely in the extended part - replace with zero
        إذا (lsb >= lhsp->width()) {
            replaceZero(nodep);
            أرجع true;
        }
        // Select entirely in the extended expression - replace with select from that
        إذا (msb < lhsp->width()) {
            lhsp->unlinkFrBack();
            lsbp->unlinkFrBack();
            nodep->replaceWithKeepDType(جديد AstSel{nodep->fileline(), lhsp, lsbp, width});
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            أرجع true;
        }
        // Select straddles both sides, but is just a shorter extend
        إذا (lsb == 0) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(جديد AstExtend{nodep->fileline(), lhsp});
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            أرجع true;
        }

        أرجع خطأ;
    }
    منطقي operandSelBiLower(AstSel* nodep) {
        // SEL(ADD(a,b),(width-1),0) -> ADD(SEL(a),SEL(b))
        // Add or any operation which doesn't care if we discard top bits
        Ast_عقدة_Biop* ثابت bip = VN_CAST(nodep->fromp(), عقدة_Biop);
        إذا (!(m_doV && bip && VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0)) أرجع خطأ;
        UINFOTREE(9, nodep, "", "SEL(BI)-in");
        Ast_عقدة_تعبير* ثابت bilhsp = bip->lhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت birhsp = bip->rhsp()->unlinkFrBack();
        bip->lhsp(جديد AstSel{nodep->fileline(), bilhsp, 0, nodep->widthConst()});
        bip->rhsp(جديد AstSel{nodep->fileline(), birhsp, 0, nodep->widthConst()});
        UINFOTREE(9, bip, "", "SEL(BI)-ou");
        VL_DO_DANGLING(replaceWChild(nodep, bip), nodep);
        أرجع true;
    }
    منطقي operandSelShiftLower(AstSel* nodep) {
        // AND({a}, SHIFTR({b}, {c})) is often shorthand in C for Verilog {b}[{c} :+ {a}]
        // becomes thought other optimizations
        // SEL(SHIFTR({a},{b}),{lsb},{width}) -> SEL({a},{lsb+b},{width})
        ثابت AstShiftR* ثابت shiftp = VN_CAST(nodep->fromp(), ShiftR);
        إذا (!(m_doV && shiftp && VN_IS(shiftp->rhsp(), Const) && VN_IS(nodep->lsbp(), Const))) {
            أرجع خطأ;
        }
        Ast_عقدة_تعبير* ثابت ap = shiftp->lhsp();
        ثابت AstConst* ثابت bp = VN_AS(shiftp->rhsp(), Const);
        ثابت AstConst* ثابت lp = VN_AS(nodep->lsbp(), Const);
        إذا (bp->isWide() || bp->رقم().isFourState() || bp->رقم().isNegative() || lp->isWide()
            || lp->رقم().isFourState() || lp->رقم().isNegative()) {
            أرجع خطأ;
        }
        ثابت صحيح newLsb = lp->toSInt() + bp->toSInt();
        إذا (newLsb + nodep->widthConst() > ap->width()) أرجع خطأ;
        //
        UINFO(9, "SEL(SHIFTR(a,b),l,w) -> SEL(a,l+b,w)");
        UINFOTREE(9, nodep, "", "SEL(SH)-in");
        AstSel* ثابت newp
            = جديد AstSel{nodep->fileline(), ap->unlinkFrBack(), newLsb, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        UINFOTREE(9, newp, "", "SEL(SH)-ou");
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        أرجع true;
    }

    منطقي operandBiExtendConstShrink(Ast_عقدة_Biop* nodep) {
        // Loop unrolling favors standalone compares
        // EQ(const{width32}, EXTEND(xx{width3})) -> EQ(const{3}, xx{3})
        // The constant must have zero bits (+ 1 if signed) or compare
        // would be incorrect. See also operandBiExtendConst
        AstExtend* ثابت extendp = VN_CAST(nodep->rhsp(), Extend);
        إذا (!extendp) أرجع خطأ;
        Ast_عقدة_تعبير* ثابت smallerp = extendp->lhsp();
        ثابت صحيح subsize = smallerp->width();
        AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
        إذا (!constp) أرجع خطأ;
        إذا (!constp->رقم().isBitsZero(constp->width() - 1, subsize)) أرجع خطأ;
        //
        UINFOTREE(9, nodep, "", "BI(EXTEND)-in");
        smallerp->unlinkFrBack();
        VL_DO_DANGLING(ادفع_Deletep(extendp->unlinkFrBack()), extendp);  // aka nodep->lhsp.
        nodep->rhsp(smallerp);

        constp->unlinkFrBack();
        ثابت V3_رقم رقم{constp, subsize, constp->رقم()};
        nodep->lhsp(جديد AstConst{constp->fileline(), رقم});
        VL_DO_DANGLING(ادفع_Deletep(constp), constp);
        أرجع خطأ;  // input node is still valid, keep going
    }
    منطقي operandBiExtendConstOver(ثابت Ast_عقدة_Biop* nodep) {
        // EQ(const{width32}, EXTEND(xx{width3})) -> constant
        // When the constant has non-zero bits above the extend it's a constant.
        // Avoids compiler warning
        ثابت AstExtend* ثابت extendp = VN_CAST(nodep->rhsp(), Extend);
        إذا (!extendp) أرجع خطأ;
        ثابت Ast_عقدة* ثابت smallerp = extendp->lhsp();
        ثابت صحيح subsize = smallerp->width();
        ثابت AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
        إذا (!constp) أرجع خطأ;
        إذا (constp->رقم().isBitsZero(constp->width() - 1, subsize)) أرجع خطأ;
        أرجع true;
    }

    // Extraction checks
    منطقي warnSelect(AstSel* nodep) {
        إذا (m_doGenerate) {
            // Never checked yet
            V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);  // May need "constifying"
        }
        // Find range of dtype we are selecting from
        // Similar code in V3Unknown::AstSel
        ثابت منطقي doit = true;
        إذا (m_warn && VN_IS(nodep->lsbp(), Const) && doit) {
            ثابت صحيح maxDeclBit = nodep->declRange().hiMaxSelect() * nodep->declElWidth()
                                   + (nodep->declElWidth() - 1);
            إذا (VN_AS(nodep->lsbp(), Const)->رقم().isFourState()) {
                nodep->v3error("Selection index is constantly unknown or tristated: "
                               "lsb="
                               << nodep->lsbp()->name() << " width=" << nodep->widthConst());
                // Replacing nodep will make a mess above, so we replace the offender
                replaceZero(nodep->lsbp());
            } وإلا إذا (nodep->declRange().ranged()
                       && (nodep->msbConst() > maxDeclBit || nodep->lsbConst() > maxDeclBit)) {
                // See also warning in V3Width
                // Must adjust by element width as declRange() is in number of elements
                string msbLsbProtected;
                إذا (nodep->declElWidth() == 0) {
                    msbLsbProtected = "(nodep->declElWidth() == 0) "
                                      + std::to_string(nodep->msbConst()) + ":"
                                      + std::to_string(nodep->lsbConst());
                } وإلا {
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
        أرجع خطأ;  // Not a transform, so NOP
    }

    ساكن منطقي operandsSame(ثابت Ast_عقدة* node1p, ثابت Ast_عقدة* node2p) {
        // For now we just detect constants & simple vars, though it could be more generic
        إذا (ثابت AstConst* ثابت const1p = VN_CAST(node1p, Const)) {
            إذا (ثابت AstConst* ثابت const2p = VN_CAST(node2p, Const)) {
                أرجع V3_رقم{node1p->fileline(), 1, 0}
                    .opEq(const1p->رقم(), const2p->رقم())
                    .isEqOne();
            }
        }
        إذا (VN_IS(node1p, VarRef) && VN_IS(node2p, VarRef)) {
            // Avoid comparing widthMin's, which results in lost optimization attempts
            // If cleanup sameGateTree to be smarter, this can be restored.
            // return node1p->sameGateTree(node2p);
            أرجع node1p->isSame(node2p);
        }
        // Pattern created by coverage-line/branch; avoid compiler tautological-compare warning
        إذا (ثابت AstAnd* ثابت and1p = VN_CAST(node1p, And)) {
            إذا (ثابت AstAnd* ثابت and2p = VN_CAST(node2p, And)) {
                إذا (VN_IS(and1p->lhsp(), Const) && VN_IS(and1p->rhsp(), عقدة_Var_Ref)
                    && VN_IS(and2p->lhsp(), Const) && VN_IS(and2p->rhsp(), عقدة_Var_Ref))
                    أرجع node1p->same_Gate_شجرة(node2p);
            }
        }
        أرجع خطأ;
    }
    منطقي ifSameAssign(ثابت Ast_عقدة_If* nodep) {
        ثابت Ast_عقدة_Assign* ثابت thensp = VN_CAST(nodep->thensp(), عقدة_Assign);
        ثابت Ast_عقدة_Assign* ثابت elsesp = VN_CAST(nodep->elsesp(), عقدة_Assign);
        إذا (!thensp || thensp->nextp()) أرجع خطأ;  // Must be SINGLE statement
        إذا (!elsesp || elsesp->nextp()) أرجع خطأ;
        إذا (thensp->type() != elsesp->type()) أرجع خطأ;  // Can't mix an assigndly with assign
        إذا (!thensp->lhsp()->same_Gate_شجرة(elsesp->lhsp())) أرجع خطأ;
        إذا (!thensp->rhsp()->gate_شجرة()) أرجع خطأ;
        إذا (!elsesp->rhsp()->gate_شجرة()) أرجع خطأ;
        // Must not create an expression with unpacked array type
        إذا (VN_IS(thensp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) أرجع خطأ;
        إذا (VN_IS(elsesp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) أرجع خطأ;
        إذا (m_underRecFunc) أرجع خطأ;  // This optimization may lead to infinite recursion
        إذا (m_doCpp) {  // Do not combine ifs cretaed by V3Premit
            إذا (VN_IS(thensp->lhsp(), VarRef) && thensp->lhsp()->isWide()) أرجع خطأ;
        }
        // Only do it if not calls and both pure, otherwise undoes V3LiftExpr
        أرجع !VN_IS(thensp->rhsp(), عقدة_FTask_Ref)  //
               && !VN_IS(elsesp->rhsp(), عقدة_FTask_Ref)  //
               && thensp->rhsp()->isPure()  //
               && elsesp->rhsp()->isPure();
    }
    منطقي operandIfIf(ثابت Ast_عقدة_If* nodep) {
        إذا (nodep->elsesp()) أرجع خطأ;
        ثابت Ast_عقدة_If* ثابت lowerIfp = VN_CAST(nodep->thensp(), عقدة_If);
        إذا (!lowerIfp || lowerIfp->nextp()) أرجع خطأ;
        إذا (nodep->type() != lowerIfp->type()) أرجع خطأ;
        إذا (Ast_عقدة::afterCommentp(lowerIfp->elsesp())) أرجع خطأ;
        أرجع true;
    }
    منطقي ifConcatMergeableBiop(ثابت Ast_عقدة* nodep) {
        أرجع (VN_IS(nodep, And) || VN_IS(nodep, Or) || VN_IS(nodep, Xor));
    }
    منطقي ifAdjacentSel(ثابت AstSel* lhsp, ثابت AstSel* rhsp) {
        إذا (!v3Global.opt.fAssemble()) أرجع خطأ;  // opt disabled
        إذا (!lhsp || !rhsp) أرجع خطأ;
        ثابت Ast_عقدة* ثابت lfromp = lhsp->fromp();
        ثابت Ast_عقدة* ثابت rfromp = rhsp->fromp();
        إذا (!lfromp || !rfromp || !lfromp->same_Gate_شجرة(rfromp)) أرجع خطأ;
        ثابت AstConst* ثابت lstart = VN_CAST(lhsp->lsbp(), Const);
        ثابت AstConst* ثابت rstart = VN_CAST(rhsp->lsbp(), Const);
        إذا (!lstart || !rstart) أرجع خطأ;  // too complicated
        ثابت صحيح rend = (rstart->toSInt() + rhsp->widthConst());
        أرجع (rend == lstart->toSInt());
    }
    منطقي ifMergeAdjacent(ثابت Ast_عقدة_تعبير* lhsp, ثابت Ast_عقدة_تعبير* rhsp) {
        // Called by concatmergeable to determine if {lhsp, rhsp} make sense
        إذا (!v3Global.opt.fAssemble()) أرجع خطأ;  // opt disabled
        // Two same varref
        إذا (operandsSame(lhsp, rhsp)) أرجع true;
        ثابت AstSel* ثابت lselp = VN_CAST(lhsp, Sel);
        ثابت AstSel* ثابت rselp = VN_CAST(rhsp, Sel);
        إذا (!lselp && !rselp) أرجع خطأ;
        إذا (lselp && !VN_IS(lselp->lsbp(), Const)) أرجع خطأ;
        إذا (rselp && !VN_IS(rselp->lsbp(), Const)) أرجع خطأ;
        // a[i:j] {a[j-1:k], b}
        إذا (lselp && !rselp && VN_IS(rhsp, Concat))
            أرجع ifMergeAdjacent(lhsp, VN_AS(rhsp, Concat)->lhsp());
        // {b, a[j:k]} a[k-1:i]
        إذا (rselp && !lselp && VN_IS(lhsp, Concat))
            أرجع ifMergeAdjacent(VN_AS(lhsp, Concat)->rhsp(), rhsp);
        // a a[msb:j]
        ثابت Ast_عقدة_تعبير* ثابت lfromp = lselp                                ? lselp->fromp()
                                          : lhsp->same_Gate_شجرة(rselp->fromp()) ? lhsp
                                                                               : nullptr;
        // a[i:0] a
        ثابت Ast_عقدة_تعبير* ثابت rfromp = rselp                                ? rselp->fromp()
                                          : rhsp->same_Gate_شجرة(lselp->fromp()) ? rhsp
                                                                               : nullptr;
        إذا (!lfromp || !rfromp || !lfromp->same_Gate_شجرة(rfromp)) أرجع خطأ;

        ثابت int32_t lstart = lselp ? lselp->lsbConst() : 0;
        ثابت int32_t rstart = rselp ? rselp->lsbConst() : 0;
        ثابت int32_t rend = rstart + (rselp ? rselp->widthConst() : rhsp->width());
        // a[i:j] a[j-1:k]
        إذا (rend == lstart) أرجع true;
        // a[i:0] a[msb:j]
        إذا (rend == rfromp->width() && lstart == 0) أرجع true;
        أرجع خطأ;
    }
    منطقي concatMergeable(ثابت Ast_عقدة_تعبير* lhsp, ثابت Ast_عقدة_تعبير* rhsp, unsigned depth) {
        // Determine if {a OP b, c OP d} => {a, c} OP {b, d} is advantageous
        إذا (!v3Global.opt.fAssemble()) أرجع خطأ;  // opt disabled
        إذا (lhsp->type() != rhsp->type()) أرجع خطأ;
        إذا (!ifConcatMergeableBiop(lhsp)) أرجع خطأ;
        إذا (depth > CONCAT_MERGABLE_MAX_DEPTH) أرجع خطأ;  // As worse case O(n^2) algorithm

        ثابت Ast_عقدة_Biop* ثابت lp = VN_CAST(lhsp, عقدة_Biop);
        ثابت Ast_عقدة_Biop* ثابت rp = VN_CAST(rhsp, عقدة_Biop);
        إذا (!lp || !rp) أرجع خطأ;
        // {a[]&b[], a[]&b[]}
        ثابت منطقي lad = ifMergeAdjacent(lp->lhsp(), rp->lhsp());
        ثابت منطقي rad = ifMergeAdjacent(lp->rhsp(), rp->rhsp());
        إذا (lad && rad) أرجع true;
        // {a[] & b[]&c[], a[] & b[]&c[]}
        إذا (lad && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) أرجع true;
        // {a[]&b[] & c[], a[]&b[] & c[]}
        إذا (rad && concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)) أرجع true;
        // {(a[]&b[])&(c[]&d[]), (a[]&b[])&(c[]&d[])}
        إذا (concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)
            && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) {
            أرجع true;
        }
        أرجع خطأ;
    }
    ساكن منطقي operandsSameWidth(ثابت Ast_عقدة* lhsp, ثابت Ast_عقدة* rhsp) {
        أرجع lhsp->width() == rhsp->width();
    }

    //----------------------------------------
    // Constant Replacement functions.
    // These all take a node, delete its tree, and replaces it with a constant

    فراغ replace_رقم(Ast_عقدة* oldp, ثابت V3_رقم& رقم, ثابت string& origParamName = "") {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        UASSERT_OBJ(!(VN_IS(oldp, Const) && !VN_AS(oldp, Const)->رقم().isFourState()), oldp,
                    "Already constant??");
        AstConst* ثابت newp = جديد AstConst{oldp->fileline(), رقم};
        إذا (!origParamName.empty()) newp->origParamName(origParamName);
        oldp->replaceWithKeepDType(newp);
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        VL_DO_DANGLING(ادفع_Deletep(oldp), oldp);
    }
    فراغ replace_رقم(Ast_عقدة* nodep, uint32_t val) {
        ثابت V3_رقم رقم{nodep, nodep->width(), val};
        VL_DO_DANGLING(replace_رقم(nodep, رقم), nodep);
    }
    فراغ replace_رقم_Signed(Ast_عقدة_Biop* nodep, uint32_t val) {
        // We allow both sides to be constant, as one may have come from
        // parameter propagation, etc.
        إذا (m_warn && !(VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const))) {
            nodep->v3warn(UNSIGNED, "Comparison is constant due to unsigned arithmetic");
        }
        VL_DO_DANGLING(replace_رقم(nodep, val), nodep);
    }
    فراغ replace_رقم_Limited(Ast_عقدة_Biop* nodep, uint32_t val) {
        // Avoids gcc warning about same
        إذا (m_warn) nodep->v3warn(CMPCONST, "Comparison is constant due to limited range");
        VL_DO_DANGLING(replace_رقم(nodep, val), nodep);
    }
    فراغ replaceZero(Ast_عقدة* nodep) { VL_DO_DANGLING(replace_رقم(nodep, 0), nodep); }
    فراغ replaceZeroChkPure(Ast_عقدة* nodep, Ast_عقدة_تعبير* checkp) {
        // For example, "0 * n" -> 0 if n has no side effects
        // Else strength reduce it to 0 & n.
        // If ever change the operation note AstAnd rule specially ignores this created pattern
        إذا (checkp->isPure()) {
            VL_DO_DANGLING(replace_رقم(nodep, 0), nodep);
        } وإلا {
            Ast_عقدة* ثابت newp = جديد AstAnd{nodep->fileline(), جديد AstConst{nodep->fileline(), 0},
                                             checkp->unlinkFrBack()};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        }
    }
    فراغ replaceAllOnes(Ast_عقدة* nodep) {
        V3_رقم ones{nodep, nodep->width(), 0};
        ones.setMask(nodep->width());
        VL_DO_DANGLING(replace_رقم(nodep, ones), nodep);
    }
    فراغ replaceConst(Ast_عقدة_Uniop* nodep) {
        V3_رقم numv{nodep, nodep->widthMinV()};
        nodep->رقم_Operate(numv, const_رقم_V(nodep->lhsp()));
        ثابت V3_رقم& رقم = to_رقم_C(nodep, numv);
        UINFO(4, "UNICONST -> " << رقم);
        VL_DO_DANGLING(replace_رقم(nodep, رقم), nodep);
    }
    فراغ replaceConst(Ast_عقدة_Biop* nodep) {
        V3_رقم numv{nodep, nodep->widthMinV()};
        nodep->رقم_Operate(numv, const_رقم_V(nodep->lhsp()), const_رقم_V(nodep->rhsp()));
        ثابت V3_رقم& رقم = to_رقم_C(nodep, numv);
        UINFO(4, "BICONST -> " << رقم);
        VL_DO_DANGLING(replace_رقم(nodep, رقم), nodep);
    }
    فراغ replaceConst(Ast_عقدة_Triop* nodep) {
        V3_رقم numv{nodep, nodep->widthMinV()};
        nodep->رقم_Operate(numv, const_رقم_V(nodep->lhsp()), const_رقم_V(nodep->rhsp()),
                             const_رقم_V(nodep->thsp()));
        ثابت V3_رقم& رقم = to_رقم_C(nodep, numv);
        UINFO(4, "TRICONST -> " << رقم);
        VL_DO_DANGLING(replace_رقم(nodep, رقم), nodep);
    }
    فراغ replaceConst(Ast_عقدة_Quadop* nodep) {
        V3_رقم numv{nodep, nodep->widthMinV()};
        nodep->رقم_Operate(numv, const_رقم_V(nodep->lhsp()), const_رقم_V(nodep->rhsp()),
                             const_رقم_V(nodep->thsp()), const_رقم_V(nodep->fhsp()));
        ثابت V3_رقم& رقم = to_رقم_C(nodep, numv);
        UINFO(4, "QUADCONST -> " << رقم);
        VL_DO_DANGLING(replace_رقم(nodep, رقم), nodep);
    }

    فراغ replaceConstString(Ast_عقدة* oldp, ثابت string& رقم) {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        Ast_عقدة* ثابت newp = جديد AstConst{oldp->fileline(), AstConst::String{}, رقم};
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        oldp->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ادفع_Deletep(oldp), oldp);
    }
    //----------------------------------------
    // Replacement functions.
    // These all take a node and replace it with something else

    فراغ replaceWChild(Ast_عقدة* nodep, Ast_عقدة_تعبير* childp) {
        // NODE(..., CHILD(...)) -> CHILD(...)
        childp->unlinkFrBackWithNext();
        // If replacing a SEL for example, the data type comes from the parent (is less wide).
        // This may adversely affect the operation of the node being replaced.
        nodep->replaceWithKeepDType(childp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }

    فراغ replaceWChildBool(Ast_عقدة* nodep, Ast_عقدة_تعبير* childp) {
        // NODE(..., CHILD(...)) -> REDOR(CHILD(...))
        childp->unlinkFrBack();
        إذا (childp->width1()) {
            nodep->replaceWithKeepDType(childp);
        } وإلا {
            nodep->replaceWithKeepDType(جديد AstRedOr{childp->fileline(), childp});
        }
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }

    //! Replace a ternary node with its RHS after iterating
    //! Used with short-circuiting, where the RHS has not yet been iterated.
    فراغ replaceWIteratedRhs(Ast_عقدة_Triop* nodep) {
        إذا (Ast_عقدة* ثابت rhsp = nodep->rhsp()) iterateAndNextNull(rhsp);
        replaceWChild(nodep, nodep->rhsp());  // May have changed
    }

    //! Replace a ternary node with its THS after iterating
    //! Used with short-circuiting, where the THS has not yet been iterated.
    فراغ replaceWIteratedThs(Ast_عقدة_Triop* nodep) {
        إذا (Ast_عقدة* ثابت thsp = nodep->thsp()) iterateAndNextNull(thsp);
        replaceWChild(nodep, nodep->thsp());  // May have changed
    }
    فراغ replaceWLhs(Ast_عقدة_Uniop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    فراغ replaceWLhs(Ast_عقدة_Biop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    فراغ replaceWRhs(Ast_عقدة_Biop* nodep) {
        // Keep RHS, remove LHS
        replaceWChild(nodep, nodep->rhsp());
    }
    فراغ replaceWLhsBool(Ast_عقدة_Biop* nodep) { replaceWChildBool(nodep, nodep->lhsp()); }
    فراغ replaceWRhsBool(Ast_عقدة_Biop* nodep) { replaceWChildBool(nodep, nodep->rhsp()); }
    فراغ replaceAsv(Ast_عقدة_Biop* nodep) {
        // BIASV(CONSTa, BIASV(CONSTb, c)) -> BIASV( BIASV_CONSTED(a,b), c)
        // BIASV(SAMEa,  BIASV(SAMEb, c))  -> BIASV( BIASV(SAMEa,SAMEb), c)
        // UINFOTREE(1, nodep, "", "repAsvConst_old");
        Ast_عقدة_تعبير* ثابت ap = nodep->lhsp();
        Ast_عقدة_Biop* ثابت rp = VN_AS(nodep->rhsp(), عقدة_Biop);
        Ast_عقدة_تعبير* ثابت bp = rp->lhsp();
        Ast_عقدة_تعبير* ثابت cp = rp->rhsp();
        ap->unlinkFrBack();
        bp->unlinkFrBack();
        cp->unlinkFrBack();
        rp->unlinkFrBack();
        nodep->lhsp(rp);
        nodep->rhsp(cp);
        rp->lhsp(ap);
        rp->rhsp(bp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        إذا (VN_IS(rp->lhsp(), Const) && VN_IS(rp->rhsp(), Const)) replaceConst(rp);
        iterate(nodep);  // Proceed to fixed point
    }
    فراغ replaceAsvLUp(Ast_عقدة_Biop* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r))
        Ast_عقدة_Biop* ثابت lp = VN_AS(nodep->lhsp()->unlinkFrBack(), عقدة_Biop);
        Ast_عقدة_تعبير* ثابت llp = lp->lhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت lrp = lp->rhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت rp = nodep->rhsp()->unlinkFrBack();
        nodep->lhsp(llp);
        nodep->rhsp(lp);
        lp->lhsp(lrp);
        lp->rhsp(rp);
        lp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    فراغ replaceAsvRUp(Ast_عقدة_Biop* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr))
        Ast_عقدة_تعبير* ثابت lp = nodep->lhsp()->unlinkFrBack();
        Ast_عقدة_Biop* ثابت rp = VN_AS(nodep->rhsp()->unlinkFrBack(), عقدة_Biop);
        Ast_عقدة_تعبير* ثابت rlp = rp->lhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت rrp = rp->rhsp()->unlinkFrBack();
        nodep->lhsp(rlp);
        nodep->rhsp(rp);
        rp->lhsp(lp);
        rp->rhsp(rrp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    فراغ replaceAndOr(Ast_عقدة_Biop* nodep) {
        //  OR  (AND (CONSTll,lr), AND(CONSTrl==ll,rr))    -> AND (CONSTll, OR(lr,rr))
        //  OR  (AND (CONSTll,lr), AND(CONSTrl,    rr=lr)) -> AND (OR(ll,rl), rr)
        // nodep ^lp  ^llp   ^lrp  ^rp  ^rlp       ^rrp
        // (Or/And may also be reversed)
        Ast_عقدة_Biop* ثابت lp = VN_AS(nodep->lhsp()->unlinkFrBack(), عقدة_Biop);
        Ast_عقدة_تعبير* ثابت llp = lp->lhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت lrp = lp->rhsp()->unlinkFrBack();
        Ast_عقدة_Biop* ثابت rp = VN_AS(nodep->rhsp()->unlinkFrBack(), عقدة_Biop);
        Ast_عقدة_تعبير* ثابت rlp = rp->lhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        إذا (operandsSame(llp, rlp)) {
            lp->lhsp(llp);
            lp->rhsp(nodep);
            lp->dtypeFrom(nodep);
            nodep->lhsp(lrp);
            nodep->rhsp(rrp);
            VL_DO_DANGLING(ادفع_Deletep(rp), rp);
            VL_DO_DANGLING(ادفع_Deletep(rlp), rlp);
        } وإلا إذا (operandsSame(lrp, rrp)) {
            lp->lhsp(nodep);
            lp->rhsp(rrp);
            lp->dtypeFrom(nodep);
            nodep->lhsp(llp);
            nodep->rhsp(rlp);
            VL_DO_DANGLING(ادفع_Deletep(rp), rp);
            VL_DO_DANGLING(ادفع_Deletep(lrp), lrp);
        } وإلا {
            nodep->v3fatalSrc("replaceAndOr on something operandAndOrSame shouldn't have matched");
        }
        // UINFOTREE(1, nodep, "", "repAndOr_new");
    }
    فراغ replaceShiftSame(Ast_عقدة_Biop* nodep) {
        // Or(Shift(ll,CONSTlr),Shift(rl,CONSTrr==lr)) -> Shift(Or(ll,rl),CONSTlr)
        // (Or/And may also be reversed)
        Ast_عقدة_Biop* ثابت lp = VN_AS(nodep->lhsp()->unlinkFrBack(), عقدة_Biop);
        Ast_عقدة_تعبير* ثابت llp = lp->lhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت lrp = lp->rhsp()->unlinkFrBack();
        Ast_عقدة_Biop* ثابت rp = VN_AS(nodep->rhsp()->unlinkFrBack(), عقدة_Biop);
        Ast_عقدة_تعبير* ثابت rlp = rp->lhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        lp->lhsp(nodep);
        lp->rhsp(lrp);
        nodep->lhsp(llp);
        nodep->rhsp(rlp);
        nodep->dtypep(llp->dtypep());  // dtype of Biop is before shift.
        VL_DO_DANGLING(ادفع_Deletep(rp), rp);
        VL_DO_DANGLING(ادفع_Deletep(rrp), rrp);
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
        VL_DO_DANGLING(ادفع_Deletep(lselp), lselp);
        VL_DO_DANGLING(ادفع_Deletep(rselp), rselp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ replaceConcatMerge(AstConcat* nodep) {
        // {llp OP lrp, rlp OP rrp} => {llp, rlp} OP {lrp, rrp}, where OP = AND/OR/XOR
        Ast_عقدة_Biop* ثابت lp = VN_AS(nodep->lhsp(), عقدة_Biop);
        Ast_عقدة_Biop* ثابت rp = VN_AS(nodep->rhsp(), عقدة_Biop);
        إذا (concatMergeable(lp, rp, 0)) {
            Ast_عقدة_تعبير* ثابت llp = lp->lhsp();
            Ast_عقدة_تعبير* ثابت lrp = lp->rhsp();
            Ast_عقدة_تعبير* ثابت rlp = rp->lhsp();
            Ast_عقدة_تعبير* ثابت rrp = rp->rhsp();
            AstConcat* ثابت newlp = جديد AstConcat{rlp->fileline(), llp->clone_شجرة_Pure(خطأ),
                                                   rlp->clone_شجرة_Pure(خطأ)};
            AstConcat* ثابت newrp = جديد AstConcat{rrp->fileline(), lrp->clone_شجرة_Pure(خطأ),
                                                   rrp->clone_شجرة_Pure(خطأ)};
            // use the lhs to replace the parent concat
            llp->replaceWith(newlp);
            VL_DO_DANGLING(ادفع_Deletep(llp), llp);
            lrp->replaceWith(newrp);
            VL_DO_DANGLING(ادفع_Deletep(lrp), lrp);
            lp->dtypeChgWidthSigned(newlp->width(), newlp->width(), VSigning::UNSIGNED);
            UINFO(5, "merged " << nodep);
            ++m_statConcatMerge;
            VL_DO_DANGLING(ادفع_Deletep(rp->unlinkFrBack()), rp);
            nodep->replaceWithKeepDType(lp->unlinkFrBack());
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            iterate(lp->lhsp());
            iterate(lp->rhsp());
        } وإلا {
            nodep->v3fatalSrc("tried to merge two Concat which are not adjacent");
        }
    }
    فراغ replaceExtend(Ast_عقدة* nodep, Ast_عقدة_تعبير* arg0p) {
        // -> EXTEND(nodep)
        // like a AstExtend{$rhsp}, but we need to set the width correctly from base node
        arg0p->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت newp
            = (VN_IS(nodep, ExtendS)
                   ? static_cast<Ast_عقدة_تعبير*>(جديد AstExtendS{nodep->fileline(), arg0p})
                   : static_cast<Ast_عقدة_تعبير*>(جديد AstExtend{nodep->fileline(), arg0p}));
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ replacePowShift(Ast_عقدة_Biop* nodep) {  // Pow or PowS
        UINFO(5, "POW(2,b)->SHIFTL(1,b) " << nodep);
        Ast_عقدة_تعبير* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* ثابت newp
            = جديد AstShiftL{nodep->fileline(), جديد AstConst{nodep->fileline(), 1}, rhsp};
        newp->lhsp()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ replaceMulShift(AstMul* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "MUL(2^n,b)->SHIFTL(b,n) " << nodep);
        ثابت صحيح amount = VN_AS(nodep->lhsp(), Const)->رقم().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_عقدة_تعبير* ثابت opp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* ثابت newp
            = جديد AstShiftL{nodep->fileline(), opp, جديد AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ replaceDivShift(AstDiv* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "DIV(b,2^n)->SHIFTR(b,n) " << nodep);
        ثابت صحيح amount = VN_AS(nodep->rhsp(), Const)->رقم().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_عقدة_تعبير* ثابت opp = nodep->lhsp()->unlinkFrBack();
        AstShiftR* ثابت newp
            = جديد AstShiftR{nodep->fileline(), opp, جديد AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ replaceModAnd(AstModDiv* nodep) {  // Mod, but not ModS as not simple shift
        UINFO(5, "MOD(b,2^n)->AND(b,2^n-1) " << nodep);
        ثابت صحيح amount = VN_AS(nodep->rhsp(), Const)->رقم().mostSetBitP1() - 1;  // 2^n->n+1
        V3_رقم mask{nodep, nodep->width()};
        mask.setMask(amount);
        Ast_عقدة_تعبير* ثابت opp = nodep->lhsp()->unlinkFrBack();
        AstAnd* ثابت newp
            = جديد AstAnd{nodep->fileline(), opp, جديد AstConst{nodep->fileline(), mask}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ replaceShiftOp(Ast_عقدة_Biop* nodep) {
        UINFO(5, "SHIFT(AND(a,b),CONST) with a/b special ->AND(SHIFT(a,CONST),SHIFT(b,CONST)) "
                     << nodep);
        ثابت صحيح width = nodep->width();
        ثابت صحيح widthMin = nodep->widthMin();
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        Ast_عقدة_Biop* ثابت lhsp = VN_AS(nodep->lhsp(), عقدة_Biop);
        lhsp->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت shiftp = nodep->rhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت ap = lhsp->lhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت bp = lhsp->rhsp()->unlinkFrBack();
        Ast_عقدة_Biop* ثابت shift1p = nodep;
        Ast_عقدة_Biop* ثابت shift2p = nodep->clone_شجرة(true);
        shift1p->lhsp(ap);
        shift1p->rhsp(shiftp->clone_شجرة_Pure(true));
        shift2p->lhsp(bp);
        shift2p->rhsp(shiftp);
        Ast_عقدة_Biop* ثابت newp = lhsp;
        newp->lhsp(shift1p);
        newp->rhsp(shift2p);
        newp->dtypeChgWidth(width, widthMin);  // The new AND must have width of the original SHIFT
        handle.relink(newp);
        iterate(newp);  // Further reduce, either node may have more reductions.
    }
    فراغ replaceShiftShift(Ast_عقدة_Biop* nodep) {
        UINFO(4, "SHIFT(SHIFT(a,s1),s2)->SHIFT(a,ADD(s1,s2)) " << nodep);
        UINFOTREE(9, nodep, "", "repShiftShift_old");
        Ast_عقدة_Biop* ثابت lhsp = VN_AS(nodep->lhsp(), عقدة_Biop);
        lhsp->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت ap = lhsp->lhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت shift1p = lhsp->rhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت shift2p = nodep->rhsp()->unlinkFrBack();
        // Shift1p and shift2p may have different sizes, both are
        // self-determined so sum with infinite width
        إذا (nodep->type() == lhsp->type()) {
            ثابت صحيح shift1 = VN_AS(shift1p, Const)->toUInt();
            ثابت صحيح shift2 = VN_AS(shift2p, Const)->toUInt();
            ثابت صحيح newshift = shift1 + shift2;
            VL_DO_DANGLING(ادفع_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ادفع_Deletep(shift2p), shift2p);
            nodep->lhsp(ap);
            nodep->rhsp(جديد AstConst(nodep->fileline(), newshift));
            iterate(nodep);  // Further reduce, either node may have more reductions.
        } وإلا {
            // We know shift amounts are constant, but might be a mixed left/right shift
            صحيح shift1 = VN_AS(shift1p, Const)->toUInt();
            إذا (VN_IS(lhsp, ShiftR)) shift1 = -shift1;
            صحيح shift2 = VN_AS(shift2p, Const)->toUInt();
            إذا (VN_IS(nodep, ShiftR)) shift2 = -shift2;
            ثابت صحيح newshift = shift1 + shift2;
            VL_DO_DANGLING(ادفع_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ادفع_Deletep(shift2p), shift2p);
            Ast_عقدة_تعبير* newp;
            V3_رقم mask1{nodep, nodep->width()};
            V3_رقم ones{nodep, nodep->width()};
            ones.setMask(nodep->width());
            إذا (shift1 < 0) {
                mask1.opShiftR(ones, V3_رقم(nodep, VL_IDATASIZE, -shift1));
            } وإلا {
                mask1.opShiftL(ones, V3_رقم(nodep, VL_IDATASIZE, shift1));
            }
            V3_رقم mask{nodep, nodep->width()};
            إذا (shift2 < 0) {
                mask.opShiftR(mask1, V3_رقم(nodep, VL_IDATASIZE, -shift2));
            } وإلا {
                mask.opShiftL(mask1, V3_رقم(nodep, VL_IDATASIZE, shift2));
            }
            إذا (newshift < 0) {
                newp = جديد AstShiftR{nodep->fileline(), ap,
                                     جديد AstConst(nodep->fileline(), -newshift)};
            } وإلا {
                newp = جديد AstShiftL{nodep->fileline(), ap,
                                     جديد AstConst(nodep->fileline(), newshift)};
            }
            newp->dtypeFrom(nodep);
            newp = جديد AstAnd{nodep->fileline(), newp, جديد AstConst{nodep->fileline(), mask}};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            // UINFOTREE(1, newp, "", "repShiftShift_new");
            iterate(newp);  // Further reduce, either node may have more reductions.
        }
        VL_DO_DANGLING(ادفع_Deletep(lhsp), lhsp);
    }

    منطقي replaceAssignMultiSel(Ast_عقدة_Assign* nodep) {
        // Multiple assignments to sequential bits can be concated
        // ASSIGN(SEL(a),aq), ASSIGN(SEL(a+1),bq) -> ASSIGN(SEL(a:b),CONCAT(aq,bq)
        // ie. assign var[2]=a, assign var[3]=b -> assign var[3:2]={b,a}

        // Skip if we're not const'ing an entire module (IE doing only one assign, etc)
        إذا (!m_modp) أرجع خطأ;
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ثابت sel1p = VN_CAST(nodep->lhsp(), Sel);
        إذا (!sel1p) أرجع خطأ;
        Ast_عقدة_Assign* ثابت nextp = VN_CAST(nodep->nextp(), عقدة_Assign);
        إذا (!nextp) أرجع خطأ;
        إذا (nodep->type() != nextp->type()) أرجع خطأ;
        ثابت AstSel* ثابت sel2p = VN_CAST(nextp->lhsp(), Sel);
        إذا (!sel2p) أرجع خطأ;
        AstVarRef* ثابت varref1p = VN_CAST(sel1p->fromp(), VarRef);
        إذا (!varref1p) أرجع خطأ;
        ثابت AstVarRef* ثابت varref2p = VN_CAST(sel2p->fromp(), VarRef);
        إذا (!varref2p) أرجع خطأ;
        إذا (!varref1p->same_Gate_شجرة(varref2p)) أرجع خطأ;
        ثابت AstConst* ثابت con1p = VN_CAST(sel1p->lsbp(), Const);
        إذا (!con1p) أرجع خطأ;
        ثابت AstConst* ثابت con2p = VN_CAST(sel2p->lsbp(), Const);
        إذا (!con2p) أرجع خطأ;
        // We need to make sure there's no self-references involved in either
        // assignment.  For speed, we only look 3 deep, then give up.
        إذا (!varNotReferenced(nodep->rhsp(), varref1p->varp())) أرجع خطأ;
        إذا (!varNotReferenced(nextp->rhsp(), varref2p->varp())) أرجع خطأ;
        // If a variable is marked split_var, access to the variable should not be merged.
        إذا (varref1p->varp()->attrSplitVar() || varref2p->varp()->attrSplitVar()) أرجع خطأ;
        // Swap?
        إذا ((con1p->toSInt() != con2p->toSInt() + sel2p->width())
            && (con2p->toSInt() != con1p->toSInt() + sel1p->width())) {
            أرجع خطأ;
        }
        ثابت منطقي lsbFirstAssign = (con1p->toUInt() < con2p->toUInt());
        UINFO(4, "replaceAssignMultiSel " << nodep);
        UINFO(4, "                   && " << nextp);
        // UINFOTREE(1, nodep, "", "comb1");
        // UINFOTREE(1, nextp, "", "comb2");
        Ast_عقدة_تعبير* ثابت rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت rhs2p = nextp->rhsp()->unlinkFrBack();
        Ast_عقدة_Assign* newp;
        إذا (lsbFirstAssign) {
            newp = nodep->cloneType(جديد AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel1p->lsbConst(), sel1p->width() + sel2p->width()},
                                    جديد AstConcat{rhs1p->fileline(), rhs2p, rhs1p});
        } وإلا {
            newp = nodep->cloneType(جديد AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel2p->lsbConst(), sel1p->width() + sel2p->width()},
                                    جديد AstConcat{rhs1p->fileline(), rhs1p, rhs2p});
        }
        // UINFOTREE(1, pnewp, "", "conew");
        nodep->replaceWith(newp);  // dypep intentionally changing
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        VL_DO_DANGLING(ادفع_Deletep(nextp->unlinkFrBack()), nextp);
        أرجع true;
    }

    منطقي replaceAssignSFormatF(Ast_عقدة_Assign* nodep) {
        // Rewrite 'x = sformatf(...)' into 'sformat(x, ...)', which is more efficient
        // at the call site as it does not need to check if 'x' needs to be freed first.
        // This is a somewhat common pattern after lowering assertions.
        إذا (!VN_IS(nodep, Assign)) أرجع خطأ;
        AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef);
        إذا (!lhsp) أرجع خطأ;
        AstSFormatF* ثابت fmtp = VN_CAST(nodep->rhsp(), SFormatF);
        إذا (!fmtp) أرجع خطأ;
        lhsp->unlinkFrBack();
        fmtp->unlinkFrBack();
        nodep->replaceWith(جديد AstSFormat{nodep->fileline(), fmtp, lhsp});
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        أرجع true;
    }

    منطقي varNotReferenced(Ast_عقدة* nodep, AstVar* varp, صحيح level = 0) {
        // Return true if varp never referenced under node.
        // Return false if referenced, or tree too deep to be worth it, or side effects
        إذا (!nodep) أرجع true;
        إذا (level > 2) أرجع خطأ;
        إذا (!nodep->isPure()) أرجع خطأ;  // For example a $fgetc can't be reordered
        إذا (VN_IS(nodep, عقدة_Var_Ref) && VN_AS(nodep, عقدة_Var_Ref)->varp() == varp) أرجع خطأ;
        أرجع (varNotReferenced(nodep->nextp(), varp, level + 1)
                && varNotReferenced(nodep->op1p(), varp, level + 1)
                && varNotReferenced(nodep->op2p(), varp, level + 1)
                && varNotReferenced(nodep->op3p(), varp, level + 1)
                && varNotReferenced(nodep->op4p(), varp, level + 1));
    }

    منطقي replace_عقدة_Assign(Ast_عقدة_Assign* nodep) {
        إذا (VN_IS(nodep->lhsp(), VarRef) && VN_IS(nodep->rhsp(), VarRef)
            && VN_AS(nodep->lhsp(), VarRef)->sameNoLvalue(VN_AS(nodep->rhsp(), VarRef))
            && !VN_IS(nodep, AssignDly)) {
            // X = X.  Quite pointless, though X <= X may override another earlier assignment
            إذا (VN_IS(nodep, AssignW)) {
                إذا (!v3Global.opt.pedantic())
                    nodep->v3error(
                        "Wire inputs its own output, creating circular logic (wire x=x)");
                أرجع خطأ;  // Don't delete the assign, or V3Gate will freak out
            } وإلا {
                VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
                أرجع true;
            }
        } وإلا إذا (m_doV && VN_IS(nodep->lhsp(), Concat)) {
            منطقي need_temp = خطأ;
            // A blocking assignment with an intra-assignment timing control evaluates its value,
            // then waits once (IEEE 1800-2023 9.4.5), here to assign a temporary for the parts
            ثابت منطقي waits = VN_IS(nodep, Assign) && nodep->timingControlp();
            // Splitting copies the RHS once per part, so a large RHS would make the tree
            // grow with the product of the number of parts and the RHS size
            ثابت منطقي needTempPure = !nodep->rhsp()->isPure() || waits
                                      || nodep->rhsp()->isLargerThan(CONCAT_ASSIGN_MAX_RHS_NODES);
            إذا (m_warn && !VN_IS(nodep, AssignDly)
                && !needTempPure) {  // Is same var on LHS and RHS?
                // If the rhs is not pure, we need a temporary variable anyway
                std::unordered_set<size_t> lhsVarIds;
                nodep->lhsp()->foreach([&](ثابت AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    lhsVarIds.emplace(m_ids(refp->varp()));
                });
                nodep->rhsp()->foreach([&](ثابت AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    إذا (lhsVarIds.عدد(m_ids(refp->varp()))) need_temp = true;
                });
            }
            إذا (needTempPure) {
                // if the RHS is impure or large we need to create a temporary variable for it,
                // because further handling involves copying of the RHS.
                UINFO(4, "  ASSITEMPPURE " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp,rhs),
                //                                ASSIGN(lc1,SEL(temp,{size1})),
                //                                ASSIGN(lc2,SEL(temp,{size2}))

                Ast_عقدة_تعبير* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
                AstVar* ثابت tempPurep = جديد AstVar{rhsp->fileline(), VVarType::BLOCKTEMP,
                                                     m_concswapNames.get(rhsp), rhsp->dtypep()};
                // Static, as read right after written, also in a fork
                tempPurep->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempPurep);
                AstVarRef* ثابت tempPureRefp
                    = جديد AstVarRef{rhsp->fileline(), tempPurep, VAccess::WRITE};
                Ast_عقدة_Assign* ثابت asnp
                    = VN_IS(nodep, AssignDly)
                          ? جديد AstAssign{nodep->fileline(), tempPureRefp, rhsp}
                          : nodep->cloneType(tempPureRefp, rhsp);
                nodep->addHereThisAsNext(asnp);
                nodep->rhsp(جديد AstVarRef{rhsp->fileline(), tempPurep, VAccess::READ});
                // Only the assignment of the temporary waits
                إذا (waits) ادفع_Deletep(nodep->timingControlp()->unlinkFrBack());
            } وإلا إذا (need_temp) {
                // The first time we constify, there may be the same variable on the LHS
                // and RHS.  In that case, we must use temporaries, or {a,b}={b,a} will break.
                UINFO(4, "  ASSITEMP " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp1,SEL(rhs,{size})),
                //                                ASSIGN(temp2,SEL(newrhs,{size}))
                //                                ASSIGN(lc1,temp1),
                //                                ASSIGN(lc2,temp2)
            } وإلا {
                UINFO(4, "  ASSI " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(lc1,SEL(rhs,{size})),
                //                                ASSIGN(lc2,SEL(newrhs,{size}))
            }
            // The NBA evaluates its intra-assignment delay as it executes (IEEE 1800-2023 4.9.4),
            // so once for the NBAs to all parts
            AstDelay* ثابت delayp
                = VN_IS(nodep, AssignDly) ? VN_CAST(nodep->timingControlp(), Delay) : nullptr;
            إذا (delayp && !delayp->lhsp()->isPure()) {
                File_سطر* ثابت flp = nodep->fileline();
                Ast_عقدة_تعبير* ثابت valuep = delayp->lhsp()->unlinkFrBack();
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
            Ast_عقدة_تعبير* ثابت lc1p = VN_AS(nodep->lhsp(), Concat)->lhsp()->unlinkFrBack();
            Ast_عقدة_تعبير* ثابت lc2p = VN_AS(nodep->lhsp(), Concat)->rhsp()->unlinkFrBack();
            Ast_عقدة_تعبير* ثابت conp = VN_AS(nodep->lhsp(), Concat)->unlinkFrBack();
            Ast_عقدة_تعبير* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_عقدة_تعبير* ثابت rhs2p = rhsp->clone_شجرة_Pure(خطأ);
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
            Ast_عقدة_Assign* newp = nullptr;
            إذا (!need_temp) {
                Ast_عقدة_Assign* ثابت asn1ap = nodep->cloneType(lc1p, sel1p);
                Ast_عقدة_Assign* ثابت asn2ap = nodep->cloneType(lc2p, sel2p);
                asn1ap->dtypeFrom(sel1p);
                asn2ap->dtypeFrom(sel2p);
                newp = Ast_عقدة::addNext(newp, asn1ap);
                newp = Ast_عقدة::addNext(newp, asn2ap);
            } وإلا {
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
                Ast_عقدة_Assign* ثابت asn1ap = nodep->cloneType(
                    جديد AstVarRef{sel1p->fileline(), temp1p, VAccess::WRITE}, sel1p);
                Ast_عقدة_Assign* ثابت asn2ap = nodep->cloneType(
                    جديد AstVarRef{sel2p->fileline(), temp2p, VAccess::WRITE}, sel2p);
                Ast_عقدة_Assign* ثابت asn1bp = nodep->cloneType(
                    lc1p, جديد AstVarRef{sel1p->fileline(), temp1p, VAccess::READ});
                Ast_عقدة_Assign* ثابت asn2bp = nodep->cloneType(
                    lc2p, جديد AstVarRef{sel2p->fileline(), temp2p, VAccess::READ});
                asn1ap->dtypeFrom(temp1p);
                asn1bp->dtypeFrom(temp1p);
                asn2ap->dtypeFrom(temp2p);
                asn2bp->dtypeFrom(temp2p);
                // This order matters
                newp = Ast_عقدة::addNext(newp, asn1ap);
                newp = Ast_عقدة::addNext(newp, asn2ap);
                newp = Ast_عقدة::addNext(newp, asn1bp);
                newp = Ast_عقدة::addNext(newp, asn2bp);
            }
            إذا (debug() >= 9 && newp) newp->dump_شجرة_And_Next(cout, "-     _new: ");
            nodep->addNextHere(newp);
            // Cleanup
            VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
            VL_DO_DANGLING(ادفع_Deletep(conp), conp);
            // Further reduce, either node may have more reductions.
            أرجع true;
        } وإلا إذا (m_doV && VN_IS(nodep->rhsp(), CvtPackedToArray)
                   && lowerAsFixedAggregate(nodep->lhsp()->dtypep())) {
            AstCvtPackedToArray* ثابت cvtp
                = VN_AS(nodep->rhsp(), CvtPackedToArray)->unlinkFrBack();
            Ast_عقدة_تعبير* srcp = cvtp->fromp()->unlinkFrBack();
            إذا (lowerAsFixedAggregate(srcp->dtypep())) {
                srcp = packFixedAggregate(srcp);
            } وإلا إذا (Ast_عقدة_Stream* ثابت streamp = VN_CAST(srcp, عقدة_Stream)) {
                Ast_عقدة_تعبير* ثابت streamSrcp = streamp->lhsp();
                إذا (lowerAsFixedAggregate(streamSrcp->dtypep())) {
                    Ast_عقدة_تعبير* ثابت packedp = packFixedAggregate(streamSrcp->unlinkFrBack());
                    streamp->lhsp(packedp);
                    streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                       VSigning::UNSIGNED,
                                                       packedp->isFourstateOrNoDtype());
                }
            }
            VL_DO_DANGLING(ادفع_Deletep(cvtp), cvtp);
            replaceAssignToFixedAggregate(nodep, nodep->lhsp()->unlinkFrBack(), srcp);
            أرجع true;
        } وإلا إذا (m_doV && VN_IS(nodep->rhsp(), StreamR)
                   && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            // The right-streaming operator on rhs of assignment does not
            // change the order of bits. Eliminate stream but keep its lhsp.
            // Add a cast if needed.
            AstStreamR* ثابت streamp = VN_AS(nodep->rhsp(), StreamR)->unlinkFrBack();
            Ast_عقدة_تعبير* srcp = streamp->lhsp()->unlinkFrBack();
            Ast_عقدة_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            ثابت Ast_عقدة_DType* ثابت dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            إذا (lowerAsFixedAggregate(srcDTypep)) {
                srcp = packFixedAggregate(srcp);
            } وإلا إذا (VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)) {
                إذا (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    صحيح srcElementBits = 0;
                    إذا (ثابت Ast_عقدة_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    صحيح dstElementBits = 0;
                    إذا (ثابت Ast_عقدة_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    srcp = جديد AstCvtArrayToArray{
                        srcp->fileline(), srcp,          nodep->dtypep(), خطأ, 1,
                        dstElementBits,   srcElementBits};
                } وإلا {
                    srcp = جديد AstCvtArrayToPacked{srcp->fileline(), srcp, nodep->dtypep()};
                }
            } وإلا إذا (VN_IS(srcDTypep, UnpackArrayDType)) {
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
            إذا (!VN_IS(dstDTypep, UnpackArrayDType) && !VN_IS(dstDTypep, QueueDType)
                && !VN_IS(dstDTypep, DynArrayDType)) {
                ثابت صحيح sWidth = srcp->width();
                ثابت صحيح dWidth = nodep->lhsp()->width();
                إذا (sWidth < dWidth) {
                    AstExtend* ثابت extendp = جديد AstExtend{srcp->fileline(), srcp, dWidth};
                    srcp = جديد AstShiftL{
                        srcp->fileline(), extendp,
                        جديد AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                        dWidth};
                }
            }
            nodep->rhsp(srcp);
            VL_DO_DANGLING(ادفع_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            أرجع true;
        } وإلا إذا (m_doV && VN_IS(nodep->lhsp(), StreamL)) {
            // Push the stream operator to the rhs of the assignment statement
            Ast_عقدة_تعبير* streamp = nodep->lhsp()->unlinkFrBack();
            Ast_عقدة_تعبير* ثابت dstp = VN_AS(streamp, StreamL)->lhsp()->unlinkFrBack();
            Ast_عقدة_DType* ثابت dstDTypep = dstp->dtypep()->skipRefp();
            Ast_عقدة_تعبير* srcp = nodep->rhsp()->unlinkFrBack();
            ثابت Ast_عقدة_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamL reverses, so reverse=true)
            إذا ((VN_IS(srcDTypep, UnpackArrayDType) || VN_IS(srcDTypep, QueueDType)
                 || VN_IS(srcDTypep, DynArrayDType))
                && (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType))) {
                صحيح blockSize = 1;
                إذا (ثابت AstConst* ثابت constp
                    = VN_CAST(VN_AS(streamp, StreamL)->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    إذا (VL_UNLIKELY(blockSize <= 0)) blockSize = 1;
                }
                صحيح srcElementBits = 0;
                إذا (ثابت Ast_عقدة_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                صحيح dstElementBits = 0;
                إذا (ثابت Ast_عقدة_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                nodep->lhsp(dstp);
                nodep->rhsp(جديد AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, true,
                                                   blockSize, dstElementBits, srcElementBits});
                nodep->dtypep(dstDTypep);
                VL_DO_DANGLING(ادفع_Deletep(streamp), streamp);
                أرجع true;
            }
            إذا (lowerAsFixedAggregate(srcDTypep)) srcp = packFixedAggregate(srcp);
            ثابت صحيح sWidth = srcp->width();
            ثابت صحيح dWidth = dstp->width();
            // Connect the rhs to the stream operator and update its width
            VN_AS(streamp, StreamL)->lhsp(srcp);
            إذا (VN_IS(srcDTypep, DynArrayDType) || VN_IS(srcDTypep, QueueDType)
                || VN_IS(srcDTypep, UnpackArrayDType)) {
                streamp->dtypeSetStream();
            } وإلا {
                streamp->dtypeSetBitOrLogicUnsized(srcp->width(), srcp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   srcp->isFourstateOrNoDtype());
            }
            إذا (VN_IS(dstDTypep, UnpackArrayDType)) {
                streamp = جديد AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
            } وإلا {
                إذا (dWidth == 0) {
                    streamp = جديد AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
                } وإلا إذا (sWidth >= dWidth) {
                    streamp = جديد AstSel{streamp->fileline(), streamp, sWidth - dWidth, dWidth};
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(streamp);
            nodep->dtypep(dstDTypep);
            أرجع true;
        } وإلا إذا (m_doV && VN_IS(nodep->lhsp(), StreamR)) {
            // The right stream operator on lhs of assignment statement does
            // not reorder bits. However, if the rhs is wider than the lhs,
            // then we select bits from the left-most, not the right-most.
            Ast_عقدة_تعبير* ثابت streamp = nodep->lhsp()->unlinkFrBack();
            Ast_عقدة_تعبير* ثابت dstp = VN_AS(streamp, StreamR)->lhsp()->unlinkFrBack();
            Ast_عقدة_DType* ثابت dstDTypep = dstp->dtypep()->skipRefp();
            Ast_عقدة_تعبير* srcp = nodep->rhsp()->unlinkFrBack();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamR does not reverse, so reverse=false).
            // V3Width may have wrapped the source in CvtArrayToPacked; unwrap it.
            إذا (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                Ast_عقدة_تعبير* origSrcp = srcp;
                إذا (AstCvtArrayToPacked* ثابت cvtp = VN_CAST(srcp, CvtArrayToPacked)) {
                    origSrcp = cvtp->fromp();
                }
                ثابت Ast_عقدة_DType* ثابت origSrcDTypep = origSrcp->dtypep()->skipRefp();
                إذا (VN_IS(origSrcDTypep, UnpackArrayDType) || VN_IS(origSrcDTypep, QueueDType)
                    || VN_IS(origSrcDTypep, DynArrayDType)) {
                    صحيح srcElementBits = 0;
                    إذا (ثابت Ast_عقدة_DType* ثابت elemDtp = origSrcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    صحيح dstElementBits = 0;
                    إذا (ثابت Ast_عقدة_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    إذا (VN_IS(srcp, CvtArrayToPacked)) {
                        origSrcp = VN_AS(srcp, CvtArrayToPacked)->fromp()->unlinkFrBack();
                        VL_DO_DANGLING(ادفع_Deletep(srcp), srcp);
                        srcp = origSrcp;
                    }
                    // Descending unpacked arrays need element reversal
                    منطقي reverse = خطأ;
                    إذا (ثابت AstUnpackArrayDType* ثابت unpackDtp
                        = VN_CAST(origSrcDTypep, UnpackArrayDType)) {
                        reverse = !unpackDtp->declRange().ascending();
                    }
                    nodep->lhsp(dstp);
                    nodep->rhsp(جديد AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, reverse,
                                                       1, dstElementBits, srcElementBits});
                    nodep->dtypep(dstDTypep);
                    VL_DO_DANGLING(ادفع_Deletep(streamp), streamp);
                    أرجع true;
                }
            }
            ثابت صحيح sWidth = srcp->width();
            ثابت صحيح dWidth = dstp->width();
            إذا (VN_IS(dstDTypep, UnpackArrayDType)) {
                ثابت صحيح dstBitWidth
                    = dWidth * VN_AS(dstDTypep, UnpackArrayDType)->arrayUnpackedElements();
                // Handling the case where rhs is wider than lhs. StreamL does not require this
                // since the combination of the left streaming operation and the implicit
                // truncation in VL_ASSIGN_UNPACK automatically selects the left-most bits.
                إذا (sWidth > dstBitWidth) {
                    srcp
                        = جديد AstSel{streamp->fileline(), srcp, sWidth - dstBitWidth, dstBitWidth};
                }
                srcp = جديد AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
            } وإلا {
                إذا (dWidth == 0) {
                    srcp = جديد AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
                } وإلا إذا (sWidth >= dWidth) {
                    srcp = جديد AstSel{streamp->fileline(), srcp, sWidth - dWidth, dWidth};
                } وإلا {
                    // Source narrower than destination: left-justify by shifting left.
                    // The right stream operator packs left-to-right, so remaining
                    // LSBs are zero-filled (IEEE 1800-2023 11.4.14.3).
                    إذا (!VN_IS(srcp->dtypep()->skipRefp(), QueueDType)) {
                        AstExtend* ثابت extendp = جديد AstExtend{srcp->fileline(), srcp, dWidth};
                        srcp = جديد AstShiftL{
                            srcp->fileline(), extendp,
                            جديد AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                            dWidth};
                    } وإلا {  // if it is a queue we dont need do do a shiftL
                        srcp = جديد AstExtend{srcp->fileline(), srcp};
                    }
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(srcp);
            nodep->dtypep(dstDTypep);
            VL_DO_DANGLING(ادفع_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            أرجع true;
        } وإلا إذا (m_doV && VN_IS(nodep->rhsp(), StreamL)) {
            AstStreamL* streamp = VN_AS(nodep->rhsp(), StreamL);
            Ast_عقدة_تعبير* srcp = streamp->lhsp();
            ثابت Ast_عقدة_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            Ast_عقدة_DType* ثابت dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            إذا (lowerAsFixedAggregate(srcDTypep)) {
                Ast_عقدة_تعبير* ثابت packedp = packFixedAggregate(srcp->unlinkFrBack());
                streamp->lhsp(packedp);
                streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   packedp->isFourstateOrNoDtype());
                srcp = packedp;
            } وإلا إذا ((VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)
                        || VN_IS(srcDTypep, UnpackArrayDType))) {
                إذا (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    صحيح blockSize = 1;
                    إذا (ثابت AstConst* ثابت constp = VN_CAST(streamp->rhsp(), Const)) {
                        blockSize = constp->toSInt();
                        إذا (VL_UNLIKELY(blockSize <= 0)) {
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
                    إذا (ثابت Ast_عقدة_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    صحيح dstElementBits = 0;
                    إذا (ثابت Ast_عقدة_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    streamp->unlinkFrBack();
                    srcp = جديد AstCvtArrayToArray{
                        srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     true,
                        blockSize,        dstElementBits,       srcElementBits};
                    nodep->rhsp(srcp);
                    VL_DO_DANGLING(ادفع_Deletep(streamp), streamp);
                } وإلا {
                    streamp->lhsp(جديد AstCvtArrayToPacked{srcp->fileline(), srcp->unlinkFrBack(),
                                                          dstDTypep});
                    streamp->dtypeFrom(dstDTypep);
                }
            }
        } وإلا إذا (m_doV && replaceAssignMultiSel(nodep)) {
            أرجع true;
        } وإلا إذا (replaceAssignSFormatF(nodep)) {
            أرجع true;
        }
        أرجع خطأ;
    }

    // Boolean replacements
    منطقي operandBoolShift(ثابت Ast_عقدة* nodep) {
        // boolean test of AND(const,SHIFTR(x,const)) -> test of AND(SHIFTL(x,const), x)
        إذا (!VN_IS(nodep, And)) أرجع خطأ;
        إذا (!VN_IS(VN_AS(nodep, And)->lhsp(), Const)) أرجع خطأ;
        إذا (!VN_IS(VN_AS(nodep, And)->rhsp(), ShiftR)) أرجع خطأ;
        ثابت AstShiftR* ثابت shiftp = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR);
        إذا (!VN_IS(shiftp->rhsp(), Const)) أرجع خطأ;
        إذا (static_cast<uint32_t>(nodep->width()) <= VN_AS(shiftp->rhsp(), Const)->toUInt()) {
            أرجع خطأ;
        }
        أرجع true;
    }
    فراغ replaceBoolShift(Ast_عقدة* nodep) {
        UINFOTREE(9, nodep, "", "bshft_old");
        AstConst* ثابت andConstp = VN_AS(VN_AS(nodep, And)->lhsp(), Const);
        Ast_عقدة_تعبير* ثابت fromp
            = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->lhsp()->unlinkFrBack();
        AstConst* ثابت shiftConstp
            = VN_AS(VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->rhsp(), Const);
        V3_رقم val{andConstp, andConstp->width()};
        val.opShiftL(andConstp->رقم(), shiftConstp->رقم());
        AstAnd* ثابت newp
            = جديد AstAnd{nodep->fileline(), جديد AstConst{nodep->fileline(), val}, fromp};
        // widthMin no longer applicable if different C-expanded width
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        UINFOTREE(9, newp, "", "_new");
    }

    فراغ replaceWithSimulation(Ast_عقدة* nodep) {
        SimulateVisitor simvis;
        // Run it - may be unoptimizable due to large for loop, etc
        simvis.mainParamEmulate(nodep);
        إذا (!simvis.optimizable()) {
            ثابت Ast_عقدة* errorp = simvis.whyNotNodep();
            إذا (!errorp) errorp = nodep;
            nodep->v3error("Expecting expression to be constant, but can't determine constant for "
                           << nodep->prettyTypeName() << '\n'
                           << errorp->warnOther() << "... Location of non-constant "
                           << errorp->prettyTypeName() << ": " << simvis.whyNotMessage());
            VL_DO_DANGLING(replaceZero(nodep), nodep);
        } وإلا {
            // Fetch the result
            Ast_عقدة* ثابت valuep = simvis.fetch_قيمة_Null(nodep);  // valuep is owned by Simulate
            UASSERT_OBJ(valuep, nodep, "No value returned from simulation");
            // Replace it
            Ast_عقدة* ثابت newp = valuep->clone_شجرة(خطأ);
            newp->fileline(nodep->fileline());
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Simulate->" << newp);
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        }
    }

    //----------------------------------------

    // VISITORS
    فراغ visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        iterateChildrenBackwardsConst(nodep);
    }
    فراغ visit(Ast_عقدة_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_concswapNames.reset();
        iterateChildren(nodep);
    }
    فراغ visit(AstCFunc* nodep) override {
        // No ASSIGNW removals under funcs, we've long eliminated INITIALs
        // (We should perhaps rename the assignw's to just assigns)
        VL_RESTORER(m_wremove);
        m_wremove = خطأ;
        iterateChildren(nodep);
    }
    فراغ visit(AstCLocalScope* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->stmtsp()) {
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        }
    }
    فراغ visit(AstScope* nodep) override {
        // No ASSIGNW removals under scope, we've long eliminated INITIALs
        VL_RESTORER(m_wremove);
        VL_RESTORER(m_scopep);
        m_wremove = خطأ;
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    فراغ swapSides(Ast_عقدة_Bi_Com* nodep) {
        // COMMUTATIVE({a},CONST) -> COMMUTATIVE(CONST,{a})
        // This simplifies later optimizations
        Ast_عقدة_تعبير* ثابت lhsp = nodep->lhsp()->unlinkFrBackWithNext();
        Ast_عقدة_تعبير* ثابت rhsp = nodep->rhsp()->unlinkFrBackWithNext();
        nodep->lhsp(rhsp);
        nodep->rhsp(lhsp);
        iterate(nodep);  // Again?
    }

    منطقي containsMemberAccessRecurse(Ast_عقدة* ثابت nodep) {
        إذا (!nodep) أرجع خطأ;
        ثابت size_t id = m_ids(nodep);
        ثابت auto it = m_containsMemberAccess.find(id);
        إذا (it != m_containsMemberAccess.end()) أرجع it->second;
        ++m_statMemberAccessVisits;
        منطقي result = خطأ;
        إذا (VN_IS(nodep, MemberSel) || VN_IS(nodep, MethodCall) || VN_IS(nodep, CMethodCall)) {
            result = true;
        } وإلا إذا (ثابت Ast_عقدة_FTask_Ref* ثابت funcRefp = VN_CAST(nodep, عقدة_FTask_Ref)) {
            إذا (containsMemberAccessRecurse(funcRefp->taskp())) result = true;
        } وإلا إذا (ثابت Ast_عقدة_CCall* ثابت funcRefp = VN_CAST(nodep, عقدة_CCall)) {
            إذا (containsMemberAccessRecurse(funcRefp->funcp())) result = true;
        } وإلا إذا (ثابت Ast_عقدة_FTask* ثابت funcp = VN_CAST(nodep, عقدة_FTask)) {
            // Assume that it has a member access
            إذا (funcp->recursive()) result = true;
        } وإلا إذا (ثابت AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc)) {
            إذا (funcp->recursive()) result = true;
        }
        إذا (!result) {
            result = containsMemberAccessRecurse(nodep->op1p())
                     || containsMemberAccessRecurse(nodep->op2p())
                     || containsMemberAccessRecurse(nodep->op3p())
                     || containsMemberAccessRecurse(nodep->op4p());
        }
        إذا (!result && !VN_IS(nodep, عقدة_FTask)
            && !VN_IS(nodep, CFunc)  // don't enter into next function
            && containsMemberAccessRecurse(nodep->nextp())) {
            result = true;
        }
        m_containsMemberAccess.emplace(id, result);
        أرجع result;
    }

    منطقي matchBiopToBitwise(Ast_عقدة_Biop* ثابت nodep) {
        إذا (!m_convertLogicToBit) أرجع خطأ;
        إذا (!nodep->lhsp()->width1()) أرجع خطأ;
        إذا (!nodep->rhsp()->width1()) أرجع خطأ;
        إذا (!nodep->isPure()) أرجع خطأ;
        إذا (containsMemberAccessRecurse(nodep)) أرجع خطأ;
        أرجع true;
    }
    منطقي matchConcatRand(AstConcat* nodep) {
        //    CONCAT(RAND, RAND) - created by Chisel code
        AstRand* ثابت aRandp = VN_CAST(nodep->lhsp(), Rand);
        ثابت AstRand* ثابت bRandp = VN_CAST(nodep->rhsp(), Rand);
        إذا (!aRandp || !bRandp) أرجع خطأ;
        إذا (!aRandp->combinable(bRandp)) أرجع خطأ;
        UINFO(4, "Concat(Rand,Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        أرجع true;
    }
    منطقي matchSelRand(AstSel* nodep) {
        //    SEL(RAND) - created by Chisel code
        AstRand* ثابت aRandp = VN_CAST(nodep->fromp(), Rand);
        إذا (!aRandp) أرجع خطأ;
        إذا (aRandp->seedp()) أرجع خطأ;
        UINFO(4, "Sel(Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        أرجع true;
    }
    منطقي matchToStringNConst(AstToStringN* nodep) {
        iterateChildren(nodep);
        إذا (ثابت AstConsArray* ثابت consp = VN_CAST(nodep->lhsp(), ConsArray)) {
            إذا (!(m_doExpensive || m_params)) أرجع خطأ;
            إذا (!consp->is_Constant_شجرة()) أرجع خطأ;
        } وإلا إذا (!VN_IS(nodep->lhsp(), Const)) {
            أرجع خطأ;
        }
        replaceWithSimulation(nodep);
        أرجع true;
    }
    صحيح operandConcatMove(ثابت AstConcat* nodep) {
        //    CONCAT under concat  (See moveConcat)
        // Return value: true indicates to do it; 2 means move to LHS
        ثابت AstConcat* ثابت abConcp = VN_CAST(nodep->lhsp(), Concat);
        ثابت AstConcat* ثابت bcConcp = VN_CAST(nodep->rhsp(), Concat);
        إذا (!abConcp && !bcConcp) أرجع 0;
        إذا (bcConcp) {
            ثابت Ast_عقدة_تعبير* ثابت ap = nodep->lhsp();
            ثابت Ast_عقدة_تعبير* ثابت bp = bcConcp->lhsp();
            // If a+b == 32,64,96 etc, then we want to have a+b together on LHS
            إذا (VL_BITBIT_I(ap->width() + bp->width()) == 0) أرجع 2;  // Transform 2: to abConc
        } وإلا {  // abConcp
            // Unless lhs is already 32 bits due to above, reorder it
            إذا (VL_BITBIT_I(nodep->lhsp()->width()) != 0) أرجع 1;  // Transform 1: to bcConc
        }
        أرجع 0;  // ok
    }
    فراغ moveConcat(AstConcat* nodep) {
        //    1: CONCAT(CONCAT({a},{b}),{c})  -> CONCAT({a},CONCAT({b}, {c}))
        // or 2: CONCAT({a}, CONCAT({b},{c})) -> CONCAT(CONCAT({a},{b}),{c})
        // Because the lhs of a concat needs a shift but the rhs doesn't,
        // putting additional CONCATs on the RHS leads to fewer assembler operations.
        // However, we'll end up with lots of wide moves if we make huge trees
        // like that, so on 32 bit boundaries, we'll do the opposite form.
        UINFO(4, "Move concat: " << nodep);
        إذا (operandConcatMove(nodep) > 1) {
            Ast_عقدة_تعبير* ثابت ap = nodep->lhsp()->unlinkFrBack();
            AstConcat* ثابت bcConcp = VN_AS(nodep->rhsp(), Concat);
            bcConcp->unlinkFrBack();
            Ast_عقدة_تعبير* ثابت bp = bcConcp->lhsp()->unlinkFrBack();
            Ast_عقدة_تعبير* ثابت cp = bcConcp->rhsp()->unlinkFrBack();
            AstConcat* ثابت abConcp = جديد AstConcat{bcConcp->fileline(), ap, bp};
            nodep->lhsp(abConcp);
            nodep->rhsp(cp);
            // If bp was a concat, then we have this exact same form again!
            // Recurse rather then calling node->iterate to prevent 2^n recursion!
            إذا (operandConcatMove(abConcp)) moveConcat(abConcp);
            VL_DO_DANGLING(ادفع_Deletep(bcConcp), bcConcp);
        } وإلا {
            AstConcat* ثابت abConcp = VN_AS(nodep->lhsp(), Concat);
            abConcp->unlinkFrBack();
            Ast_عقدة_تعبير* ثابت ap = abConcp->lhsp()->unlinkFrBack();
            Ast_عقدة_تعبير* ثابت bp = abConcp->rhsp()->unlinkFrBack();
            Ast_عقدة_تعبير* ثابت cp = nodep->rhsp()->unlinkFrBack();
            AstConcat* ثابت bcConcp = جديد AstConcat{abConcp->fileline(), bp, cp};
            nodep->lhsp(ap);
            nodep->rhsp(bcConcp);
            إذا (operandConcatMove(bcConcp)) moveConcat(bcConcp);
            VL_DO_DANGLING(ادفع_Deletep(abConcp), abConcp);
        }
    }

    // Special cases
    فراغ visit(AstConst*) override {}  // Already constant

    فراغ visit(AstCell* nodep) override {
        إذا (m_params) {
            iterateAndNextNull(nodep->paramsp());
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ visit(AstClassOrPackageRef* nodep) override { iterateChildren(nodep); }

    فراغ visit(AstMatchMasked* nodep) override {
        // Do not iterate the tables, they must be constant pool entries
        iterate(nodep->lhsp());
        إذا (AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const)) {
            replace_رقم(nodep, AstMatchMasked::fold(constp->رقم(), nodep->matchp()->varp()));
        }
    }

    فراغ visit(AstPin* nodep) override { iterateChildren(nodep); }

    فراغ replaceLogEq(AstLogEq* nodep) {
        // LOGEQ(a,b) => AstLogAnd{AstLogOr{AstLogNot{a},b},AstLogOr{AstLogNot{b},a}}
        Ast_عقدة_تعبير* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        // Do exactly as IEEE says, might result in extra terms, so in future may do differently
        AstLogAnd* ثابت newp = جديد AstLogAnd{
            nodep->fileline(),
            جديد AstLogOr{nodep->fileline(), جديد AstLogNot{nodep->fileline(), lhsp}, rhsp},
            جديد AstLogOr{nodep->fileline(),
                         جديد AstLogNot{nodep->fileline(), rhsp->clone_شجرة_Pure(خطأ)},
                         lhsp->clone_شجرة_Pure(خطأ)}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }

    فراغ replaceSelSel(AstSel* nodep) {
        // SEL(SEL({x},a,b),c,d) => SEL({x},a+c,d)
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ثابت belowp = VN_AS(nodep->fromp(), Sel);
        Ast_عقدة_تعبير* ثابت fromp = belowp->fromp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت lsb1p = nodep->lsbp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت lsb2p = belowp->lsbp()->unlinkFrBack();
        // Eliminate lower range
        UINFO(4, "Elim Lower range: " << nodep);
        Ast_عقدة_تعبير* newlsbp;
        إذا (VN_IS(lsb1p, Const) && VN_IS(lsb2p, Const)) {
            newlsbp = جديد AstConst{lsb1p->fileline(),
                                   VN_AS(lsb1p, Const)->toUInt() + VN_AS(lsb2p, Const)->toUInt()};
            VL_DO_DANGLING(ادفع_Deletep(lsb1p), lsb1p);
            VL_DO_DANGLING(ادفع_Deletep(lsb2p), lsb2p);
        } وإلا {
            // Width is important, we need the width of the fromp's expression, not the
            // potentially smaller lsb1p's width, but don't insert a redundant AstExtend.
            // Note that due to some sloppiness in earlier passes, lsb1p might actually be wider,
            // so extend to the wider type.
            ثابت Ast_عقدة_تعبير* ثابت widep = lsb1p->width() > lsb2p->width() ? lsb1p : lsb2p;
            Ast_عقدة_تعبير* ثابت lhsp = widep->width() > lsb2p->width()
                                          ? جديد AstExtend{lsb2p->fileline(), lsb2p}
                                          : lsb2p;
            Ast_عقدة_تعبير* ثابت rhsp = widep->width() > lsb1p->width()
                                          ? جديد AstExtend{lsb1p->fileline(), lsb1p}
                                          : lsb1p;
            lhsp->dtypeFrom(widep);
            rhsp->dtypeFrom(widep);
            newlsbp = جديد AstAdd{lsb1p->fileline(), lhsp, rhsp};
            newlsbp->dtypeFrom(widep);
        }
        AstSel* ثابت newp = جديد AstSel{nodep->fileline(), fromp, newlsbp, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }

    فراغ replaceSelConcat(AstSel* nodep) {
        // SEL(CONCAT(a,b),c,d) => SEL(a or b, . .)
        AstConcat* ثابت conp = VN_AS(nodep->fromp(), Concat);
        Ast_عقدة_تعبير* ثابت conLhsp = conp->lhsp();
        Ast_عقدة_تعبير* ثابت conRhsp = conp->rhsp();
        إذا (static_cast<صحيح>(nodep->lsbConst()) >= conRhsp->width()) {
            conLhsp->unlinkFrBack();
            AstSel* ثابت newp
                = جديد AstSel{nodep->fileline(), conLhsp, nodep->lsbConst() - conRhsp->width(),
                             nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } وإلا إذا (static_cast<صحيح>(nodep->msbConst()) < conRhsp->width()) {
            conRhsp->unlinkFrBack();
            AstSel* ثابت newp
                = جديد AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(), nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } وإلا {
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
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    منطقي operandSelReplicate(AstSel* nodep) {
        // SEL(REPLICATE(from,rep),lsb,width) => SEL(from,0,width) as long
        // as SEL's width <= b's width
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ثابت repp = VN_AS(nodep->fromp(), Replicate);
        Ast_عقدة_تعبير* ثابت fromp = repp->srcp();
        ثابت AstConst* ثابت lsbp = VN_CAST(nodep->lsbp(), Const);
        إذا (!lsbp) أرجع خطأ;
        UASSERT_OBJ(fromp->width(), nodep, "Not widthed");
        إذا ((lsbp->toUInt() / fromp->width())
            != ((lsbp->toUInt() + nodep->width() - 1) / fromp->width())) {
            أرجع خطأ;
        }
        //
        fromp->unlinkFrBack();
        AstSel* ثابت newp = جديد AstSel{
            nodep->fileline(), fromp,
            جديد AstConst{lsbp->fileline(), lsbp->toUInt() % fromp->width()}, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        أرجع true;
    }
    منطقي operandRepRep(AstReplicate* nodep) {
        // REPLICATE(REPLICATE2(from2,cnt2),cnt1) => REPLICATE(from2,(cnt1+cnt2))
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ثابت rep2p = VN_AS(nodep->srcp(), Replicate);
        Ast_عقدة_تعبير* ثابت from2p = rep2p->srcp();
        ثابت AstConst* ثابت cnt1p = VN_CAST(nodep->countp(), Const);
        إذا (!cnt1p) أرجع خطأ;
        ثابت AstConst* ثابت cnt2p = VN_CAST(rep2p->countp(), Const);
        إذا (!cnt2p) أرجع خطأ;
        //
        from2p->unlinkFrBack();
        AstReplicate* ثابت newp
            = جديد AstReplicate{nodep->fileline(), from2p, cnt1p->toUInt() * cnt2p->toUInt()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        أرجع true;
    }
    منطقي operandConcatSame(AstConcat* nodep) {
        // CONCAT(fromp,fromp) -> REPLICATE(fromp,1+1)
        // CONCAT(REP(fromp,cnt1),fromp) -> REPLICATE(fromp,cnt1+1)
        // CONCAT(fromp,REP(fromp,cnt1)) -> REPLICATE(fromp,1+cnt1)
        // CONCAT(REP(fromp,cnt1),REP(fromp,cnt2)) -> REPLICATE(fromp,cnt1+cnt2)
        Ast_عقدة_تعبير* from1p = nodep->lhsp();
        uint32_t cnt1 = 1;
        Ast_عقدة_تعبير* from2p = nodep->rhsp();
        uint32_t cnt2 = 1;
        إذا (VN_IS(from1p, Replicate)) {
            ثابت AstConst* ثابت cnt1p = VN_CAST(VN_CAST(from1p, Replicate)->countp(), Const);
            إذا (!cnt1p) أرجع خطأ;
            from1p = VN_AS(from1p, Replicate)->srcp();
            cnt1 = cnt1p->toUInt();
        }
        إذا (VN_IS(from2p, Replicate)) {
            ثابت AstConst* ثابت cnt2p = VN_CAST(VN_CAST(from2p, Replicate)->countp(), Const);
            إذا (!cnt2p) أرجع خطأ;
            from2p = VN_AS(from2p, Replicate)->srcp();
            cnt2 = cnt2p->toUInt();
        }
        إذا (!operandsSame(from1p, from2p)) أرجع خطأ;
        //
        from1p->unlinkFrBack();
        AstReplicate* ثابت newp = جديد AstReplicate{nodep->fileline(), from1p, cnt1 + cnt2};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        أرجع true;
    }
    فراغ replaceSelIntoBiop(AstSel* nodep) {
        // SEL(BUFIF1(a,b),1,bit) => BUFIF1(SEL(a,1,bit),SEL(b,1,bit))
        Ast_عقدة_Biop* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), عقدة_Biop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_عقدة_تعبير* ثابت lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_عقدة_تعبير* ثابت bilhsp = fromp->lhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت birhsp = fromp->rhsp()->unlinkFrBack();
        //
        fromp->lhsp(
            جديد AstSel{nodep->fileline(), bilhsp, lsbp->clone_شجرة_Pure(true), nodep->widthConst()});
        fromp->rhsp(جديد AstSel{nodep->fileline(), birhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ replaceSelIntoUniop(AstSel* nodep) {
        // SEL(NOT(a),1,bit) => NOT(SEL(a,bit))
        Ast_عقدة_Uniop* ثابت fromp = VN_AS(nodep->fromp()->unlinkFrBack(), عقدة_Uniop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_عقدة_تعبير* ثابت lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_عقدة_تعبير* ثابت bilhsp = fromp->lhsp()->unlinkFrBack();
        //
        fromp->lhsp(جديد AstSel{nodep->fileline(), bilhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }

    فراغ visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        iterateChildren(nodep);
    }

    فراغ visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->bitp());
        إذا (VN_IS(nodep->bitp(), Const)
            && VN_IS(nodep->fromp(), VarRef)
            // Need to make sure it's an array object so don't mis-allow a constant (bug509.)
            && VN_AS(nodep->fromp(), VarRef)->varp()
            && VN_IS(VN_AS(nodep->fromp(), VarRef)->varp()->valuep(), ConsArray)) {
            m_selp = nodep;  // Ask visit(AstVarRef) to replace varref with const
        }
        iterateAndNextNull(nodep->fromp());
        إذا (VN_IS(nodep->fromp(), Const)) {  // It did.
            إذا (!m_selp) {
                nodep->v3error("Illegal assignment of constant to unpacked array");
            } وإلا {
                Ast_عقدة* ثابت fromp = nodep->fromp()->unlinkFrBack();
                nodep->replaceWithKeepDType(fromp);
                VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            }
        }
        // Handle ARRAYSEL directly on ConsArray (not through VarRef)
        وإلا إذا (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ConsArray)) {
            ثابت AstConsArray* ثابت consp = VN_AS(nodep->fromp(), ConsArray);
            ثابت uint32_t bit = VN_AS(nodep->bitp(), Const)->toUInt();
            Ast_عقدة_تعبير* ثابت itemp = consp->valuep(bit);
            إذا (VN_IS(itemp, Const)) {
                ثابت V3_رقم& رقم = VN_AS(itemp, Const)->رقم();
                VL_DO_DANGLING(replace_رقم(nodep, رقم), nodep);
            } وإلا إذا (m_params && itemp) {
                // An aggregate element, e.g. a structure in an array parameter
                nodep->replaceWithKeepDType(itemp->clone_شجرة(خطأ));
                VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
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
        إذا (!m_required) أرجع;  // Do nothing unless we are in parameter mode
        // Fallback to simulation: this will invoke SimulateVisitor::visit(AstSliceSel*)
        replaceWithSimulation(nodep);
    }

    فراغ visit(AstStructSel* nodep) override {
        iterateChildren(nodep);

        إذا (ثابت AstConsStruct* ثابت consp = VN_CAST(nodep->fromp(), ConsStruct)) {
            Ast_عقدة_تعبير* ثابت valuep = consp->memberValuep(nodep->name());
            إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
                VL_DO_DANGLING(replace_رقم(nodep, constp->رقم()), nodep);
            } وإلا {
                nodep->replaceWith(valuep->clone_شجرة(خطأ));
                VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            }
        }
    }

    فراغ visit(AstCAwait* nodep) override {
        m_hasJumpDelay = true;
        iterateChildren(nodep);
    }
    فراغ visit(Ast_عقدة_Var_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Not linked");
        منطقي did = خطأ;
        إذا (m_doV && (!nodep->varp()->constPoolEntry() || m_selp) && nodep->varp()->valuep()
            && !m_attrp) {
            // UINFOTREE(1, valuep, "", "visitvaref");
            iterateAndNextNull(nodep->varp()->valuep());  // May change nodep->varp()->valuep()
            Ast_عقدة* ثابت valuep = nodep->varp()->valuep();
            // The selected element, when under a constant select of an array
            ثابت AstConsArray* ثابت consp = VN_CAST(valuep, ConsArray);
            ثابت Ast_عقدة* ثابت selItemp
                = (m_selp && consp) ? consp->valuep(m_selp->bitConst()) : nullptr;
            إذا (nodep->access().isReadOnly() && valuep
                && ((!m_params  // Can reduce constant wires into equations
                     && m_doNConst && v3Global.opt.fConst() && nodep->varp()->isConst())
                    || nodep->varp()->isParam())) {
                إذا (operandConst(valuep)) {
                    ثابت V3_رقم& رقم = VN_AS(valuep, Const)->رقم();
                    // UINFO(2, "constVisit " << cvtToHex(valuep) << " " << num);
                    ثابت string origParamName
                        = nodep->varp()->isParam() ? nodep->varp()->name() : "";
                    VL_DO_DANGLING(replace_رقم(nodep, رقم, origParamName), nodep);
                    did = true;
                } وإلا إذا (ثابت AstConst* ثابت itemp = VN_CAST(selItemp, Const)) {
                    VL_DO_DANGLING(replace_رقم(nodep, itemp->رقم()), nodep);
                    did = true;
                } وإلا إذا (m_params && consp) {
                    // Allow parameters to pass arrays
                    // Earlier recursion of ConsArray made sure each array value is constant
                    // This exception is fairly fragile, i.e. doesn't
                    // support arrays of arrays or other stuff
                    Ast_عقدة* ثابت newp = valuep->clone_شجرة(خطأ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
                    did = true;
                } وإلا إذا (m_params && VN_IS(valuep, ConsStruct)) {
                    Ast_عقدة* ثابت newp = valuep->clone_شجرة(خطأ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
                    did = true;
                } وإلا إذا (nodep->varp()->isParam() && VN_IS(valuep, Unbounded)) {
                    Ast_عقدة* ثابت newp = valuep->clone_شجرة(خطأ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
                    did = true;
                }
            }
        }
        إذا (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but variable isn't const: "
                           << nodep->varp()->prettyNameQ());
        }
    }
    فراغ visit(Ast_تعبير_Stmt* nodep) override {
        iterateChildren(nodep);
        إذا (!Ast_عقدة::afterCommentp(nodep->stmtsp())) {
            deleteVarScopesUnder(nodep->stmtsp());
            UINFO(8, "ExprStmt(...) " << nodep << " " << nodep->resultp());
            nodep->replaceWith(nodep->resultp()->unlinkFrBack());
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            // Removing the ExprStmt might have made something impure above now pure
        }
    }
    فراغ visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        منطقي did = خطأ;
        إذا (nodep->itemp()->valuep()) {
            // UINFOTREE(1, nodep->itemp()->valuep(), "", "visitvaref");
            ثابت AstEnumItem* ثابت itemp = nodep->itemp();
            إذا (m_recursing_Enum_عناصر.عدد(itemp)) {
                nodep->v3error("Recursive enum value: " << nodep->itemp()->prettyNameQ());
            } وإلا {
                m_recursing_Enum_عناصر.emplace(itemp);
                iterateAndNextNull(nodep->itemp()->valuep());
                m_recursing_Enum_عناصر.erase(itemp);
            }
            إذا (AstConst* ثابت valuep = VN_CAST(nodep->itemp()->valuep(), Const)) {
                ثابت V3_رقم& رقم = valuep->رقم();
                VL_DO_DANGLING(replace_رقم(nodep, رقم), nodep);
                did = true;
            }
        }
        إذا (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but enum value isn't const: "
                           << nodep->itemp()->prettyNameQ());
        }
    }

    منطقي only_Sen_Item_In_Sen_شجرة(ثابت AstSenItem* nodep) {
        // Only one if it's not in a list
        أرجع (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }
    فراغ visit(AstSenItem* nodep) override {
        iterateChildren(nodep);
        إذا (m_doNConst
            && !v3Global.opt.timing().isSetTrue()  // If --timing, V3Sched would turn this into an
                                                   // infinite loop. See #5080
            && (VN_IS(nodep->sensp(), Const) || VN_IS(nodep->sensp(), EnumItemRef)
                || (nodep->varrefp() && nodep->varrefp()->varp()->isParam()))) {
            // Constants in sensitivity lists may be removed (we'll simplify later)
            إذا (nodep->isClocked()) {  // A constant can never get a pos/negedge
                إذا (only_Sen_Item_In_Sen_شجرة(nodep)) {
                    إذا (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                        // TODO: This really is dodgy, as strictly compliant simulators will not
                        //       execute this block, but but t_func_check relies on it
                        nodep->replaceWith(
                            جديد AstSenItem{nodep->fileline(), AstSenItem::Initial{}});
                    } وإلا {
                        nodep->replaceWith(جديد AstSenItem{nodep->fileline(), AstSenItem::Never{}});
                    }
                    VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
                } وإلا {
                    VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } وإلا {  // Otherwise it may compute a result that needs to settle out
                nodep->replaceWith(جديد AstSenItem{nodep->fileline(), AstSenItem::Combo{}});
                VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            }
        } وإلا إذا (m_doNConst && VN_IS(nodep->sensp(), Not)) {
            // V3Gate may propagate NOTs into clocks... Just deal with it
            Ast_عقدة* ثابت sensp = nodep->sensp();
            Ast_عقدة* lastSensp = sensp;
            منطقي invert = خطأ;
            طالما (VN_IS(lastSensp, Not)) {
                lastSensp = VN_AS(lastSensp, Not)->lhsp();
                invert = !invert;
            }
            UINFO(8, "senItem(NOT...) " << nodep << " " << invert);
            إذا (invert) nodep->edgeType(nodep->edgeType().invert());
            sensp->replaceWith(lastSensp->unlinkFrBack());
            VL_DO_DANGLING(ادفع_Deletep(sensp), sensp);
        }
    }

    صنف SenItemCmp final {
        ساكن صحيح cmp(ثابت Ast_عقدة_تعبير* ap, ثابت Ast_عقدة_تعبير* bp) {
            ثابت VNType aType = ap->type();
            ثابت VNType bType = bp->type();
            إذا (aType != bType) أرجع static_cast<صحيح>(bType) - static_cast<صحيح>(aType);

            إذا (ثابت AstVarRef* ثابت aRefp = VN_CAST(ap, VarRef)) {
                ثابت AstVarRef* ثابت bRefp = VN_AS(bp, VarRef);
                // Looks visually better if we keep sorted by name
                إذا (aRefp->name() < bRefp->name()) أرجع -1;
                إذا (aRefp->name() > bRefp->name()) أرجع 1;
                // But might be same name with different scopes
                إذا (aRefp->varScopep() < bRefp->varScopep()) أرجع -1;
                إذا (aRefp->varScopep() > bRefp->varScopep()) أرجع 1;
                // Or rarely, different data types
                إذا (aRefp->dtypep() < bRefp->dtypep()) أرجع -1;
                إذا (aRefp->dtypep() > bRefp->dtypep()) أرجع 1;
                أرجع 0;
            }

            إذا (ثابت AstConst* ثابت aConstp = VN_CAST(ap, Const)) {
                ثابت AstConst* ثابت bConstp = VN_AS(bp, Const);
                إذا (aConstp->رقم().isLtXZ(bConstp->رقم())) أرجع -1;
                إذا (bConstp->رقم().isLtXZ(aConstp->رقم())) أرجع 1;
                أرجع 0;
            }

            إذا (ثابت Ast_عقدة_Biop* ثابت aBiOpp = VN_CAST(ap, عقدة_Biop)) {
                ثابت Ast_عقدة_Biop* ثابت bBiOpp = VN_AS(bp, عقدة_Biop);
                // Compare RHSs first, as LHS might be const
                إذا (ثابت صحيح c = cmp(aBiOpp->rhsp(), bBiOpp->rhsp())) أرجع c;
                أرجع cmp(aBiOpp->lhsp(), bBiOpp->lhsp());
            }

            إذا (ثابت AstCMethodHard* ثابت aCallp = VN_CAST(ap, CMethodHard)) {
                ثابت AstCMethodHard* ثابت bCallp = VN_AS(bp, CMethodHard);
                إذا (aCallp->method() < bCallp->method()) أرجع -1;
                إذا (aCallp->method() > bCallp->method()) أرجع 1;
                إذا (ثابت صحيح c = cmp(aCallp->fromp(), bCallp->fromp())) أرجع c;
                ثابت Ast_عقدة_تعبير* aPinsp = aCallp->pinsp();
                ثابت Ast_عقدة_تعبير* bPinsp = bCallp->pinsp();
                طالما (aPinsp && bPinsp) {
                    إذا (ثابت صحيح c = cmp(aPinsp, bPinsp)) أرجع c;
                    aPinsp = VN_AS(aPinsp->nextp(), عقدة_تعبير);
                    bPinsp = VN_AS(bPinsp->nextp(), عقدة_تعبير);
                }
                أرجع aPinsp ? -1 : bPinsp ? 1 : 0;
            }

            أرجع 0;
        }

    عام:
        منطقي operator()(ثابت AstSenItem* lhsp, ثابت AstSenItem* rhsp) ثابت {
            ثابت Ast_عقدة_تعبير* ثابت lSensp = lhsp->sensp();
            ثابت Ast_عقدة_تعبير* ثابت rSensp = rhsp->sensp();
            إذا (lSensp && rSensp) {
                // If both terms have sensitivity expressions, recursively compare them
                إذا (ثابت صحيح c = cmp(lSensp, rSensp)) أرجع c < 0;
            } وإلا إذا (lSensp || rSensp) {
                // Terms with sensitivity expressions come after those without
                أرجع rSensp;
            }
            // Finally sort by edge, AFTER variable, as we want multiple edges for same var
            // adjacent. note the SenTree optimizer requires this order (more general first,
            // less general last)
            أرجع lhsp->edgeType() < rhsp->edgeType();
        }
    };

    فراغ visit(Ast_Sen_شجرة* nodep) override {
        iterateChildren(nodep);
        إذا (m_doExpensive) {
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
                إذا (nextp && !cmp(senp, nextp)) {
                    // Something's out of order, sort it
                    senp = nullptr;
                    std::vector<AstSenItem*> vec;
                    لكل (AstSenItem* senp = nodep->sensesp(); senp;
                         senp = VN_AS(senp->nextp(), SenItem)) {
                        vec.ادفع_back(senp);
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
                إذا (!nextp) اكسر;
                AstSenItem* ثابت lItemp = senp;
                AstSenItem* ثابت rItemp = nextp;
                ثابت Ast_عقدة_تعبير* ثابت lSenp = lItemp->sensp();
                ثابت Ast_عقدة_تعبير* ثابت rSenp = rItemp->sensp();
                إذا (!lSenp || !rSenp) استمر;

                إذا (lSenp->same_Gate_شجرة(rSenp)) {
                    // POSEDGE or NEGEDGE -> BOTHEDGE. (We've sorted POSEDGE, before NEGEDGE, so we
                    // do not need to test for the opposite orders.)
                    إذا (lItemp->edgeType() == VEdgeType::ET_POSEDGE
                        && rItemp->edgeType() == VEdgeType::ET_NEGEDGE) {
                        // Make both terms BOTHEDGE, the second will be removed below
                        lItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                        rItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                    }

                    // Remove identical expressions
                    إذا (lItemp->edgeType() == rItemp->edgeType()) {
                        VL_DO_DANGLING(ادفع_Deletep(rItemp->unlinkFrBack()), rItemp);
                        nextp = lItemp;
                    }
                }
            }
        }
    }

    //-----
    // Zero elimination
    فراغ visit(Ast_عقدة_Assign* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->timingControlp()) m_hasJumpDelay = true;
        إذا (m_doNConst && replace_عقدة_Assign(nodep)) أرجع;
    }
    فراغ visit(AstAlias* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    فراغ visit(AstAliasScope* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    فراغ visit(AstAssignW* nodep) override {
        iterateChildren(nodep);
        إذا (m_doNConst && replace_عقدة_Assign(nodep)) أرجع;
        // Process containing this AssignW as single body statement
        AstAlways* ثابت procp = VN_CAST(nodep->backp(), Always);
        إذا (!procp || procp->stmtsp() != nodep || nodep->nextp()) أرجع;
        // Not VarXRef, as different refs may set different values to each hierarchy
        Ast_عقدة_Var_Ref* ثابت varrefp = VN_CAST(nodep->lhsp(), VarRef);
        إذا (m_wremove && !m_params && m_doNConst && m_modp && operandConst(nodep->rhsp())
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
            Ast_عقدة_تعبير* ثابت exprp = nodep->rhsp()->unlinkFrBack();
            varrefp->unlinkFrBack();
            File_سطر* ثابت flp = nodep->fileline();
            AstInitial* ثابت newinitp = جديد AstInitial{flp, جديد AstAssign{flp, varrefp, exprp}};
            procp->replaceWith(newinitp);
            VL_DO_DANGLING(ادفع_Deletep(procp), procp);
            // Set the initial value right in the variable so we can constant propagate
            Ast_عقدة* ثابت initvaluep = exprp->clone_شجرة(خطأ);
            varrefp->varp()->isConst(true);
            varrefp->varp()->valuep(initvaluep);
        }
    }
    فراغ visit(AstCReset* nodep) override {
        iterateChildren(nodep);
        إذا (!m_doNConst) أرجع;
        ثابت AstBasicDType* ثابت bdtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType);
        إذا (!bdtypep) أرجع;
        إذا (!bdtypep->isZeroInit()) أرجع;
        AstConst* ثابت newp = جديد AstConst{nodep->fileline(), V3_رقم{nodep, bdtypep}};
        UINFO(9, "CRESET(0) => CONST(0) " << nodep);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ visit(AstCvtUnpackToUnpack* nodep) override {
        iterateChildren(nodep);
        AstConsArray* ثابت fromp = VN_CAST(nodep->fromp(), ConsArray);
        إذا (!fromp) أرجع;
        // Fold the outermost dimension, converting the elements for any dimension under it
        ثابت AstUnpackArrayDType* ثابت toArrayp
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت AstUnpackArrayDType* ثابت fromArrayp
            = VN_AS(fromp->dtypep()->skipRefp(), UnpackArrayDType);
        // This dimension may not be reversed, only one under it
        ثابت منطقي reverse
            = toArrayp->declRange().ascending() != fromArrayp->declRange().ascending();
        ثابت uint64_t lastIndex = toArrayp->elementsConst() - 1;
        Ast_عقدة_DType* ثابت subDTypep = toArrayp->subDTypep();
        ثابت AstUnpackArrayDType* ثابت subArrayp
            = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
        ثابت auto convert = [&](Ast_عقدة_تعبير* valuep) -> Ast_عقدة_تعبير* {
            valuep->unlinkFrBack();
            إذا (!subArrayp || !subArrayp->isReverseOf(valuep->dtypep())) أرجع valuep;
            أرجع جديد AstCvtUnpackToUnpack{valuep->fileline(), valuep, subDTypep};
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
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        iterate(newp);  // Fold the conversions of the elements
    }
    فراغ visit(AstCvtArrayToArray* nodep) override {
        iterateChildren(nodep);
        // Handle the case where we have a stream operation inside a cast conversion
        // Check for both StreamL and StreamR operations
        Ast_عقدة_Stream* streamp = nullptr;
        منطقي isReverse = خطأ;
        إذا (AstStreamL* ثابت streamLp = VN_CAST(nodep->fromp(), StreamL)) {
            streamp = streamLp;
            isReverse = true;  // StreamL reverses the operation
        } وإلا إذا (AstStreamR* ثابت streamRp = VN_CAST(nodep->fromp(), StreamR)) {
            streamp = streamRp;
            isReverse = خطأ;  // StreamR doesn't reverse the operation
        }
        إذا (streamp) {
            Ast_عقدة_تعبير* srcp = streamp->lhsp();
            ثابت Ast_عقدة_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            Ast_عقدة_DType* ثابت dstDTypep = nodep->dtypep()->skipRefp();
            إذا (VN_IS(srcDTypep, QueueDType) && VN_IS(dstDTypep, QueueDType)) {
                صحيح blockSize = 1;
                إذا (ثابت AstConst* ثابت constp = VN_CAST(streamp->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    إذا (VL_UNLIKELY(blockSize <= 0)) {
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
                إذا (ثابت Ast_عقدة_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                صحيح dstElementBits = 0;
                إذا (ثابت Ast_عقدة_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                streamp->unlinkFrBack();
                Ast_عقدة_تعبير* newp = جديد AstCvtArrayToArray{
                    srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     isReverse,
                    blockSize,        dstElementBits,       srcElementBits};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ادفع_Deletep(streamp), streamp);
                VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
                أرجع;
            }
        }
    }
    فراغ visit(AstRelease* nodep) override {
        // cppcheck-suppress constVariablePointer // children unlinked below
        إذا (AstConcat* ثابت concatp = VN_CAST(nodep->lhsp(), Concat)) {
            File_سطر* ثابت flp = nodep->fileline();
            AstRelease* ثابت newLp = جديد AstRelease{flp, concatp->lhsp()->unlinkFrBack()};
            AstRelease* ثابت newRp = جديد AstRelease{flp, concatp->rhsp()->unlinkFrBack()};
            nodep->replaceWith(newLp);
            newLp->addNextHere(newRp);
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            visit(newLp);
            visit(newRp);
        }
    }

    فراغ visit(Ast_عقدة_If* nodep) override {
        iterateChildren(nodep);
        إذا (m_doNConst) {
            إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->condp(), Const)) {
                Ast_عقدة* keepp = nullptr;
                Ast_عقدة* delp = nullptr;
                إذا (constp->isZero()) {
                    UINFO(4, "IF(0,{any},{x}) => {x}: " << nodep);
                    keepp = nodep->elsesp();
                    delp = nodep->thensp();
                } وإلا إذا (!m_doV || constp->isNeqZero()) {  // Might be X in Verilog
                    UINFO(4, "IF(!0,{x},{any}) => {x}: " << nodep);
                    keepp = nodep->thensp();
                    delp = nodep->elsesp();
                } وإلا {
                    UINFO(4, "IF condition is X, retaining: " << nodep);
                    أرجع;
                }
                إذا (!canDeleteStatements()) {
                    UINFO(4, "IF retained until statics lifted: " << nodep);
                    أرجع;
                }

                // If we delete a branch that contains variable declarations, also delete the
                // corresponding varscopes so we don't leave dangling AstVarScope::m_varp pointers.
                deleteVarScopesUnder(delp);

                إذا (keepp) {
                    keepp->unlinkFrBackWithNext();
                    nodep->replaceWith(keepp);
                } وإلا {
                    nodep->unlinkFrBack();
                    deleteVarScopesUnder(nodep->thensp());
                    deleteVarScopesUnder(nodep->elsesp());
                }
                VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);

                // Removed branch could contain only impurity within a function and the
                // function could be purified
                VIsCached::clear_Cache_شجرة();
            } وإلا إذا (!Ast_عقدة::afterCommentp(nodep->thensp())
                       && !Ast_عقدة::afterCommentp(nodep->elsesp())) {
                إذا (!nodep->condp()->isPure()) {
                    // Condition has side effect - leave, but don't need return value
                    UINFO(4, "IF({x}) ; => STMTEXPR({x}): " << nodep);
                    nodep->fileline()->warnOff(V3ErrorCode::IGNOREDRETURN, true);
                    Ast_عقدة_Stmt* ثابت newp
                        = جديد Ast_Stmt_تعبير{nodep->fileline(), nodep->condp()->unlinkFrBack()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
                } وإلا {
                    // Empty block, remove it
                    UINFO(4, "IF({x}) ; => ; : " << nodep);
                    VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } وإلا إذا (!Ast_عقدة::afterCommentp(nodep->thensp())) {
                UINFO(4, "IF({x}) nullptr {...} => IF(NOT{x}}: " << nodep);
                Ast_عقدة_تعبير* ثابت condp = nodep->condp();
                Ast_عقدة* ثابت elsesp = nodep->elsesp();
                condp->unlinkFrBackWithNext();
                elsesp->unlinkFrBackWithNext();
                إذا (nodep->thensp()) {  // Must have been comment
                    ادفع_Deletep(nodep->thensp()->unlinkFrBackWithNext());
                }
                nodep->condp(جديد AstLogNot{condp->fileline(),
                                           condp});  // LogNot, as C++ optimization also possible
                nodep->addThensp(elsesp);
                nodep->branchPred(nodep->branchPred().invert());
            } وإلا إذا (((VN_IS(nodep->condp(), Not) && nodep->condp()->width() == 1)
                        || VN_IS(nodep->condp(), LogNot))
                       && nodep->thensp() && nodep->elsesp()) {
                UINFO(4, "IF(NOT {x})  => IF(x) swapped if/else" << nodep);
                Ast_عقدة_تعبير* ثابت condp
                    = VN_AS(nodep->condp(), عقدة_Uniop)->lhsp()->unlinkFrBackWithNext();
                Ast_عقدة* ثابت thensp = nodep->thensp()->unlinkFrBackWithNext();
                Ast_عقدة* ثابت elsesp = nodep->elsesp()->unlinkFrBackWithNext();
                AstIf* ثابت ifp = جديد AstIf{nodep->fileline(), condp, elsesp, thensp};
                ifp->isBoundsCheck(nodep->isBoundsCheck());  // Copy bounds check info
                ifp->branchPred(nodep->branchPred().invert());
                nodep->replaceWith(ifp);
                VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            } وإلا إذا (ifSameAssign(nodep)) {
                UINFO(4,
                      "IF({a}) ASSIGN({b},{c}) else ASSIGN({b},{d}) => ASSIGN({b}, {a}?{c}:{d})");
                Ast_عقدة_Assign* ثابت thensp = VN_AS(nodep->thensp(), عقدة_Assign);
                // cppcheck-suppress constVariablePointer // children unlinked below
                Ast_عقدة_Assign* ثابت elsesp = VN_AS(nodep->elsesp(), عقدة_Assign);
                thensp->unlinkFrBack();
                Ast_عقدة_تعبير* ثابت condp = nodep->condp()->unlinkFrBack();
                Ast_عقدة_تعبير* ثابت truep = thensp->rhsp()->unlinkFrBack();
                Ast_عقدة_تعبير* ثابت falsep = elsesp->rhsp()->unlinkFrBack();
                thensp->rhsp(جديد AstCond{truep->fileline(), condp, truep, falsep});
                nodep->replaceWith(thensp);
                VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            } وإلا إذا (خطأ  // Disabled, as vpm assertions are faster
                              // without due to short-circuiting
                       && operandIfIf(nodep)) {
                UINFO(9, "IF({a}) IF({b}) => IF({a} && {b})");
                Ast_عقدة_If* ثابت lowerIfp = VN_AS(nodep->thensp(), عقدة_If);
                Ast_عقدة_تعبير* ثابت condp = nodep->condp()->unlinkFrBack();
                Ast_عقدة* ثابت lowerThensp = lowerIfp->thensp()->unlinkFrBackWithNext();
                Ast_عقدة_تعبير* ثابت lowerCondp = lowerIfp->condp()->unlinkFrBackWithNext();
                nodep->condp(جديد AstLogAnd{lowerIfp->fileline(), condp, lowerCondp});
                lowerIfp->replaceWith(lowerThensp);
                VL_DO_DANGLING(ادفع_Deletep(lowerIfp), lowerIfp);
            } وإلا {
                // Optimizations that don't reform the IF itself
                إذا (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
                matchIfSameLast(nodep);
                matchIfCondCond(nodep);
            }
        }
    }

    فراغ visit(AstDisplay* nodep) override {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        iterateChildren(nodep);
        إذا (stmtDisplayDisplay(nodep)) أرجع;
    }
    منطقي stmtDisplayDisplay(AstDisplay* nodep) {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        إذا (!m_modp) أرجع خطأ;  // Don't optimize under single statement
        AstDisplay* ثابت prevp = VN_CAST(nodep->backp(), Display);
        إذا (!prevp) أرجع خطأ;
        إذا (!((prevp->displayType() == nodep->displayType())
              || (prevp->displayType() == VDisplayType::DT_WRITE
                  && nodep->displayType() == VDisplayType::DT_DISPLAY)
              || (prevp->displayType() == VDisplayType::DT_DISPLAY
                  && nodep->displayType() == VDisplayType::DT_WRITE)))
            أرجع خطأ;
        إذا ((prevp->filep() && !nodep->filep()) || (!prevp->filep() && nodep->filep())
            || (prevp->filep() && nodep->filep() && !prevp->filep()->same_شجرة(nodep->filep())))
            أرجع خطأ;
        إذا (!prevp->fmtp() || prevp->fmtp()->nextp() || !nodep->fmtp() || nodep->fmtp()->nextp())
            أرجع خطأ;
        AstSFormatF* ثابت pformatp = prevp->fmtp();
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSFormatF* ثابت nformatp = nodep->fmtp();
        // We don't merge scopeNames as can have only one and might be different scopes (late in
        // process) Also rare for real code to print %m multiple times in same message
        إذا (nformatp->scopeNamep() && pformatp->scopeNamep()) أرجع خطأ;
        // We don't early merge arguments as might need to later print warnings with
        // right line numbers, nor scopeNames as might be different scopes (late in process)
        إذا (!m_doCpp && pformatp->exprsp()) أرجع خطأ;
        إذا (!m_doCpp && nformatp->exprsp()) أرجع خطأ;
        إذا (pformatp->exprsp() && !pformatp->exprsp()->isPureAndNext()) أرجع خطأ;
        إذا (nformatp->exprsp() && !nformatp->exprsp()->isPureAndNext()) أرجع خطأ;
        // Avoid huge merges
        ساكن constexpr صحيح DISPLAY_MAX_MERGE_LENGTH = 500;
        إذا (pformatp->text().length() + nformatp->text().length() > DISPLAY_MAX_MERGE_LENGTH)
            أرجع خطأ;
        //
        UINFO(9, "DISPLAY(SF({a})) DISPLAY(SF({b})) -> DISPLAY(SF({a}+{b}))");
        // Convert DT_DISPLAY to DT_WRITE as may allow later optimizations
        إذا (prevp->displayType() == VDisplayType::DT_DISPLAY) {
            prevp->displayType(VDisplayType::DT_WRITE);
            pformatp->text(pformatp->text() + "\n");
        }
        // We can't replace prev() as the edit tracking iterators will get confused.
        // So instead we edit the prev note itself.
        إذا (prevp->addNewline()) pformatp->text(pformatp->text() + "\n");
        pformatp->text(pformatp->text() + nformatp->text());
        إذا (!prevp->addNewline() && nodep->addNewline()) pformatp->text(pformatp->text() + "\n");
        إذا (nformatp->exprsp()) pformatp->addExprsp(nformatp->exprsp()->unlinkFrBackWithNext());
        إذا (AstScopeName* ثابت scopeNamep = nformatp->scopeNamep()) {
            scopeNamep->unlinkFrBackWithNext();
            pformatp->scopeNamep(scopeNamep);
        }
        VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
        أرجع true;
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
        إذا (nodep->تعبير_Format_2()) {
            // Upconvert to text-based format?
            // Similar code in V3LinkResolve::visit(AstSFormatF)
            UASSERT_OBJ(nodep->text() == "", nodep,
                        "Non-format $sformatf should have text format == \"\"");
            إذا (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->رقم().isFromString()) {
                AstConst* ثابت fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->رقم().toString());
                nodep->تعبير_Format_2(خطأ);
                VL_DO_DANGLING(ادفع_Deletep(fmtp), fmtp);
            }
        }
        إذا (nodep->تعبير_Format_2()) أرجع;  // Runtime, unfortunately  (unless constify format later)
        منطقي anyconst = خطأ;
        لكل (Ast_عقدة* argp = nodep->exprsp(); argp; argp = argp->nextp()) {
            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
            Ast_عقدة* ثابت subargp = fargp ? fargp->exprp() : argp;
            إذا (VN_IS(subargp, Const)) {
                anyconst = true;
                اكسر;
            }
        }
        إذا (m_doNConst && anyconst) {
            UINFO(9, "  Display in  " << nodep->text());
            string newFormat;
            string fmt;
            منطقي inPct = خطأ;
            Ast_عقدة* argp = nodep->exprsp();
            ثابت string text = nodep->text();
            لكل (حرف ch : text) {
                إذا (!inPct && ch == '%') {
                    inPct = true;
                    fmt = ch;
                } وإلا إذا (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } وإلا إذا (inPct) {
                    inPct = خطأ;
                    fmt += ch;
                    ch = std::tolower(ch);
                    بدل (ch) {
                    حالة '%': اكسر;  // %% - still %%
                    حالة 'm': اكسر;  // %m - still %m - auto insert "name"
                    حالة 'l': اكسر;  // %l - still %l - auto insert "library"
                    حالة 't':  // %t - don't know $timeformat so can't constify
                        إذا (argp) argp = argp->nextp();
                        اكسر;
                    افتراضي:  // Most operators, just move to next argument
                        إذا (argp) {
                            Ast_عقدة* ثابت nextp = argp->nextp();
                            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                            Ast_عقدة* ثابت subargp = fargp ? fargp->exprp() : argp;
                            ثابت VFormatAttr formatAttr
                                = argp
                                      ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                                      : VFormatAttr{};
                            إذا (VN_IS(subargp, Const)) {  // Convert it
                                ثابت string out
                                    = formatAttr.isEnum()
                                          ? const_رقم_V(subargp).displayedEnum(fargp, fmt)
                                          : const_رقم_V(subargp).displayed(nodep, fmt, formatAttr);
                                UINFO(9, "     DispConst: " << fmt << " -> " << out << "  for "
                                                            << subargp);
                                // fmt = out w/ replace % with %% as it must later when
                                // used as a format output as literal.
                                fmt = VString::quotePercent(out);
                                VL_DO_DANGLING(ادفع_Deletep(argp->unlinkFrBack()), argp);
                            }
                            argp = nextp;
                        }
                        اكسر;
                    }  // switch
                    newFormat += fmt;
                } وإلا {
                    newFormat += ch;
                }
            }
            إذا (newFormat != nodep->text()) {
                nodep->text(newFormat);
                UINFO(9, "  Display out " << nodep);
            }
        }
        إذا (!nodep->exprsp() && nodep->text().find('%') == string::npos && !nodep->hidden()) {
            // Just a simple constant string - the formatting is pointless
            UINFO(9, "replaceConstStr");
            VL_DO_DANGLING(replaceConstString(nodep, nodep->text()), nodep);
            أرجع;
        }
        إذا (VN_IS(nodep->backp(), عقدة_Assign)  // Can't remove under Display etc
            && nodep->text() == "%s" && VN_IS(nodep->exprsp(), SFormatArg)
            && VN_AS(nodep->exprsp(), SFormatArg)->formatAttr().isString()
            && !nodep->exprsp()->nextp()) {
            UINFO(9, "  Display(%p, expr) -> constant expr " << nodep);
            nodep->replaceWith(VN_AS(nodep->exprsp(), SFormatArg)->exprp()->unlinkFrBack());
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            أرجع;
        }
    }
    فراغ visit(AstSFormat* nodep) override {
        iterateChildren(nodep);
        إذا (!m_doNConst) أرجع;
        إذا (m_doNConst) {
            // If it's a constant string written to a string variable, inline it as an assignment
            AstSFormatF* ثابت fmtp = nodep->fmtp();
            Ast_عقدة_تعبير* ثابت lhsp = nodep->lhsp();
            AstBasicDType* ثابت basicp = VN_CAST(lhsp->dtypep()->skipRefp(), BasicDType);
            إذا (basicp && basicp->isString() && !fmtp->exprsp()
                && fmtp->text().find('%') == string::npos) {
                AstConst* ثابت strp
                    = جديد AstConst{fmtp->fileline(), AstConst::String{}, fmtp->text()};
                lhsp->unlinkFrBack();
                nodep->replaceWith(جديد AstAssign{nodep->fileline(), lhsp, strp});
                VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
                أرجع;
            }
        }
    }
    فراغ visit(Ast_عقدة_FTask* nodep) override {
        VL_RESTORER(m_underRecFunc);
        إذا (nodep->recursive()) m_underRecFunc = true;
        iterateChildren(nodep);
    }

    فراغ visit(Ast_عقدة_CCall* nodep) override {
        iterateChildren(nodep);
        m_hasJumpDelay = true;  // As don't analyze inside tasks for timing controls
    }
    فراغ visit(Ast_عقدة_FTask_Ref* nodep) override {
        // Note excludes AstFuncRef as other visitor below
        iterateChildren(nodep);
        m_hasJumpDelay = true;  // As don't analyze inside tasks for timing controls
    }
    فراغ visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_عقدة_FTask_Ref*>(nodep));
        إذا (m_params) {  // Only parameters force us to do constant function call propagation
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
        m_hasJumpDelay = خطأ;
        m_hasLoopTest = خطأ;
        iterateChildren(nodep);
        ثابت منطقي thisLoopHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = thisLoopHasJumpDelay || oldHasJumpDelay;
        // If the first statement always break, the loop is useless
        إذا (ثابت AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            إذا (testp->condp()->isZero() && (!testp->nextp() || canDeleteStatements())) {
                nodep->v3warn(UNUSEDLOOP, "Loop condition is always false");
                VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
                أرجع;
            }
        }
        // If last statement always breaks, repalce loop with body
        إذا (Ast_عقدة* lastp = nodep->stmtsp()) {
            طالما (Ast_عقدة* ثابت nextp = lastp->nextp()) lastp = nextp;
            إذا (AstLoopTest* ثابت testp = VN_CAST(lastp, LoopTest)) {
                إذا (testp->condp()->isZero()) {
                    VL_DO_DANGLING2(ادفع_Deletep(testp->unlinkFrBack()), testp, lastp);
                    nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
                    أرجع;
                }
            }
        }
        // Warn on infinite loop
        إذا (!m_hasLoopTest && !thisLoopHasJumpDelay) {
            nodep->v3warn(INFINITELOOP, "Infinite loop (condition always true)");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::INFINITELOOP, true);  // Complain once
        }
    }
    فراغ visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);

        // If never breaks, remove
        إذا (nodep->condp()->isNeqZero()) {
            VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }

        m_hasLoopTest = true;

        // If always breaks, subsequent statements are dead code, delete them
        إذا (nodep->condp()->isZero()) {
            Ast_عقدة* ثابت nextp = nodep->nextp();
            إذا (nextp && canDeleteStatements()) {
                VL_DO_DANGLING(ادفع_Deletep(nextp->unlinkFrBackWithNext()), nodep);
            }
            أرجع;
        }

        إذا (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
    }

    فراغ visit(AstConsArray* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstConsStruct* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstConsStructItem* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstConsArrayItem* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstUnbounded* nodep) override { iterateChildren(nodep); }
    // These are converted by V3Param.  Don't constify as we don't want the
    // from() VARREF to disappear, if any.
    // If output of a presel didn't get consted, chances are V3Param didn't visit properly
    فراغ visit(Ast_عقدة_Pre_Sel*) override {}

    // Ignored, can eliminate early
    فراغ visit(AstSysIgnore* nodep) override {
        iterateChildren(nodep);
        إذا (m_doNConst) VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
    }

    فراغ visit(Ast_Stmt_تعبير* nodep) override {
        iterateChildren(nodep);
        // Malformed due to child being deleted, remove here
        إذا (!nodep->exprp()) {
            VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }
        // Remove if expression is trivially pure
        إذا (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), VarRef)) {
            VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }
        // Remove calls to empty functions
        إذا (ثابت AstCCall* ثابت callp = VN_CAST(nodep->exprp(), CCall)) {
            ثابت AstCFunc* ثابت funcp = callp->funcp();
            إذا (!callp->argsp() && funcp->emptyBody()) {
                VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
                أرجع;
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
        إذا (nodep->nextp() && canDeleteStatements()) {
            ادفع_Deletep(nodep->nextp()->unlinkFrBackWithNext());
        }

        // JumpGo as last statement in target JumpBlock (including last in a last sub-list),
        // is a no-op, remove it.
        إذا (!nodep->nextp()) {
            لكل (Ast_عقدة* abovep = nodep->abovep(); abovep; abovep = abovep->abovep()) {
                إذا (abovep == nodep->blockp()) {
                    VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
                    أرجع;
                }
                // Stop if not doing expensive, or if the above node is not the last in its list,
                // ... or if it's not an 'if' TODO: it would be enough if it was not a branch.
                إذا (!m_doExpensive || abovep->nextp() || !VN_IS(abovep, If)) اكسر;
            }
        }
        // Mark JumpBlock as used
        m_usedJumpBlocks.emplace(nodep->blockp());
        m_hasJumpDelay = true;
    }

    فراغ visit(AstJumpBlock* nodep) override {
        iterateChildren(nodep);

        // If first statement is an AstLoopTest, pull it before the jump block
        إذا (AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            nodep->addHereThisAsNext(testp->unlinkFrBack());
        }

        // Remove if empty
        إذا (!nodep->stmtsp()) {
            UINFO(4, "JUMPLABEL => empty " << nodep);
            VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }

        // If no JumpGo points to this node, replace it with its body
        إذا (!m_usedJumpBlocks.عدد(nodep)) {
            UINFO(4, "JUMPLABEL => unused " << nodep);
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        }
    }

    //-----
    // Below lines are magic expressions processed by astgen
    //  TREE_SKIP_VISIT("AstNODETYPE")    # Rename normal visit to visitGen and don't iterate
    //-----
    // clang-format off
    شجرة_SKIP_VISIT("ArraySel");
    شجرة_SKIP_VISIT("CAwait");

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
    فراغ visit(Ast_عقدة* nodep) override {
        // Default: Just iterate
        إذا (m_required) {
            إذا (VN_IS(nodep, عقدة_DType) || VN_IS(nodep, Range) || VN_IS(nodep, SliceSel)
                || VN_IS(nodep, Dot) || VN_IS(nodep, Text)) {
                // ignore
            } وإلا إذا (VN_IS(nodep, Pattern) || VN_IS(nodep, PatMember)) {
                // A parameter override pattern is typed later, so only fold its members
                iterateChildren(nodep);
            } وإلا إذا (AstCellRef* ثابت crp = VN_CAST(nodep, CellRef)) {
                iterate(crp->exprp());
                إذا (Ast_عقدة* ثابت newp = crp->exprp()) {
                    crp->replaceWithKeepDType(newp->unlinkFrBack());
                    VL_DO_DANGLING(ادفع_Deletep(crp), crp);
                }
                أرجع;
            } وإلا {
                nodep->v3error("Expecting expression to be constant, but can't convert a "
                               << nodep->prettyTypeName() << " to constant.");
            }
        } وإلا {
            إذا (nodep->isTimingControl()) m_hasJumpDelay = true;
            // Calculate the width of this operation
            إذا (m_params && !nodep->width()) nodep = V3Width::widthParamsEdit(nodep);
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
        , m_concswapNames{globalPass ? ("__Vconcswap_" + cvtToStr(s_global_Pass_رقم++)) : ""} {
        // clang-format off
        بدل (pmode) {
        حالة PROC_PARAMS_NOWARN:  m_doV = true;  m_doNConst = true; m_params = true;
                                  m_required = خطأ; اكسر;
        حالة PROC_PARAMS:         m_doV = true;  m_doNConst = true; m_params = true;
                                  m_required = true; اكسر;
        حالة PROC_GENERATE:       m_doV = true;  m_doNConst = true; m_params = true;
                                  m_required = true; m_doGenerate = true; اكسر;
        حالة PROC_LIVE:           اكسر;
        حالة PROC_V_WARN:         m_doV = true;  m_doNConst = true; m_warn = true; m_convertLogicToBit = true; اكسر;
        حالة PROC_V_NOWARN:       m_doV = true;  m_doNConst = true; اكسر;
        حالة PROC_V_EXPENSIVE:    m_doV = true;  m_doNConst = true; m_doExpensive = true; اكسر;
        حالة PROC_CPP:            m_doV = خطأ; m_doNConst = true; m_doCpp = true; اكسر;
        افتراضي:                  v3fatalSrc("Bad case"); اكسر;
        }
        // clang-format on
    }
    ~ConstVisitor() override {
        إذا (m_doCpp) {
            إذا (m_globalPass) {
                V3Stats::addStat("Optimizations, Const bit op reduction", m_statBitOpReduction);
            } وإلا {
                V3Stats::addStatSum("Optimizations, Const bit op reduction", m_statBitOpReduction);
            }
        }
        V3Stats::addStatSum("Optimizations, Cond redundant expressions", m_stat_Cond_تعبير_Redundant);
        V3Stats::addStatSum("Optimizations, If cond redundant expressions",
                            m_stat_If_Cond_تعبير_Redundant);
        V3Stats::addStatSum("Optimizations, Concat merges", m_statConcatMerge);
        V3Stats::addStatSum("Optimizations, Member access predicate node visits",
                            m_statMemberAccessVisits);
    }

    Ast_عقدة* mainAcceptEdit(Ast_عقدة* nodep) {
        VIsCached::clear_Cache_شجرة();  // Avoid using any stale isPure
        // Operate starting at a random place
        أرجع iterateSubtreeReturnEdits(nodep);
    }
};

uint32_t ConstVisitor::s_global_Pass_رقم = 0;

//######################################################################
// Const class functions

//! Force this cell node's parameter list to become a constant
فراغ V3Const::constifyParamsEdit(Ast_عقدة* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS, /* globalPass: */ خطأ};
    // cppcheck-suppress constVariablePointer // edited below
    إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        إذا (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } وإلا {
        (فراغ)visitor.mainAcceptEdit(nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump if have next line
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

//! Constify this cell node's parameter list if possible
فراغ V3Const::constifyParamsNoWarnEdit(Ast_عقدة* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS_NOWARN, /* globalPass: */ خطأ};
    // cppcheck-suppress constVariablePointer // edited below
    إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        إذا (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } وإلا {
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
فراغ V3Const::constifyGenerateParamsEdit(Ast_عقدة* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE:");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized
    // yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthGenerateParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_GENERATE, /* globalPass: */ خطأ};
    // cppcheck-suppress constVariablePointer // edited below
    إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        إذا (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } وإلا {
        VL_DO_DANGLING(visitor.mainAcceptEdit(nodep), nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump this.
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

فراغ V3Const::constifyAllLint(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_WARN, /* globalPass: */ true};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_شجرة("const", 0, dump_شجرة_Either_Level() >= 3);
}

فراغ V3Const::constifyCpp(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ true};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_شجرة("const_cpp", 0, dump_شجرة_Either_Level() >= 3);
}

Ast_عقدة* V3Const::constifyEdit(Ast_عقدة* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_NOWARN, /* globalPass: */ خطأ};
    nodep = visitor.mainAcceptEdit(nodep);
    أرجع nodep;
}

Ast_عقدة* V3Const::constifyEditCpp(Ast_عقدة* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ خطأ};
    nodep = visitor.mainAcceptEdit(nodep);
    أرجع nodep;
}

فراغ V3Const::constifyAllLive(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    // This only pushes constants up, doesn't make any other edits
    // IE doesn't prune dead statements, as we need to do some usability checks after this
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_LIVE, /* globalPass: */ true};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_شجرة("const", 0, dump_شجرة_Either_Level() >= 3);
}

فراغ V3Const::constifyAll(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ true};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_شجرة("const", 0, dump_شجرة_Either_Level() >= 3);
}

Ast_عقدة* V3Const::constifyExpensiveEdit(Ast_عقدة* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ خطأ};
    nodep = visitor.mainAcceptEdit(nodep);
    أرجع nodep;
}
