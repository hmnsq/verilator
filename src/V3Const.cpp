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

#وصف_ڪريو وڻ_SKIP_گهمو(...)
#وصف_ڪريو TREEOP1(...)
#وصف_ڪريو TREEOPA(...)
#وصف_ڪريو TREEOP(...)
#وصف_ڪريو TREEOPS(...)
#وصف_ڪريو TREEOPC(...)
#وصف_ڪريو TREEOPV(...)

//######################################################################
// Utilities

ساكن منطقي isConst(ثابت Ast_ڳنڍ* nodep, uint64_t v) {
    ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
    أرجع constp && constp->منزل_UQuad() == v;
}

سانچو <قسم_نالو T>
ساكن قسم_نالو std::enable_if<std::is_integral<T>::قدر, منطقي>::type isPow2(T val) {
    أرجع (val & (val - 1)) == 0;
}

ساكن صحيح ڳڻپ_Trailing_Zeroes(uint64_t val) {
    UASSERT(val, "countTrailingZeroes argument must be non-zero");
#إذا defined(__GNUC__) && !defined(VL_NO_BUILTINS)
    أرجع __builtin_ctzll(val);
#وإلا
    صحيح ٻٽو_انگ_2 = 0;
    val = ~val;
    طالما (val & 1) {
        ++ٻٽو_انگ_2;
        val >>= 1;
    }
    أرجع ٻٽو_انگ_2;
#جيڪڏهن_پورو
}

// Allocates a unique id for each AstNode it is applied to, held in user4.
صنف VNIdAllocator final {
    // NODE STATE
    // AstNode::user4p -> size_t. Unique id of the node (0/nullptr if not allocated yet)

    // MEMBERS
    // Claimed lazily on first use TODO: fix conflict with V3Param user4 slot
    std::unique_ptr<VNUser4_اندريون_Use> m_inuser4p;
    سائيز_t m_ايندڙ_Id = 0;  // Id allocated most recently

عام:
    // Return the unique id of the given node, allocating a new one if it has none yet
    سائيز_t عامل()(Ast_ڳنڍ* nodep) {
        إذا (!m_inuser4p) m_inuser4p = std::make_unique<VNUser4_اندريون_Use>();  // Claim on first use
        إذا (!nodep->user4p()) nodep->user4p(reinterpret_cast<فراغ*>(++m_ايندڙ_Id));
        أرجع reinterpret_cast<سائيز_t>(nodep->user4p());
    }
};

// This visitor can be used in the post-expanded Ast from V3Expand, where the Ast satisfies:
// - Constants are 64 bit at most (because words are accessed via AstWordSel)
// - Variables are scoped.
صنف Const_ٻٽو_انگ_Op_وڻ_Visitor final : عام VNVisitorConst {
    // TYPES

    // Holds a node to be added as a term in the reduction tree, it's equivalent op count, and a
    // bool indicating if the term is clean (0/1 value, or if the top bits might be dirty)
    استعمال نتيجو_رڪن = std::tuple<Ast_ڳنڍ_اظهار*, بنانشان, منطقي>;

    صنف پن_Info final {  // Leaf node (either AstConst or AstVarRef)
        // MEMBERS
        منطقي m_polarity = سچ;  // Invert result due to NOT
        صحيح m_lsb = 0;  // LSB of actually used bit of m_refp->varp()
        صحيح m_msb = 0;  // MSB of actually used bit of m_refp->varp()
        صحيح m_لفظ_Idx = -1;  // -1 means AstWordSel is not used.
        AstVarRef* m_refp = خالي_اشارو;  // Leaf's variable reference
        ثابت AstConst* m_constp = خالي_اشارو;  // Leaf's constant

    عام:
        // CONSTRUCTORS
        پن_Info() = افتراضي;
        پن_Info(ثابت پن_Info& other) = افتراضي;
        پن_Info& عامل=(ثابت پن_Info& other) = افتراضي;
        پن_Info(پن_Info&& other) = افتراضي;
        پن_Info& عامل=(پن_Info&& other) = افتراضي;
        واضح پن_Info(صحيح lsb)
            : m_lsb{lsb} {}

        // METHODS
        فراغ سيٽ_پن(AstVarRef* refp) {
            UASSERT_OBJ(!m_refp && !m_constp, refp, "Must be called just once");
            m_refp = refp;
            m_msb = refp->varp()->ويڪر_Min() - 1;
        }
        فراغ سيٽ_پن(ثابت AstConst* constp) {
            UASSERT_OBJ(!m_refp && !m_constp, constp, "Must be called just once");
            m_constp = constp;
            m_msb = constp->ويڪر_Min() - 1;
        }
        // updateBitRange(), limitBitRangeToLsb(), and polarity() must be called during ascending
        // back to the root.
        فراغ update_ٻٽو_انگ_Range(صحيح newLsb, صحيح newMsb) {
            إذا ((m_lsb <= m_msb && newLsb > newMsb) || (m_lsb > m_msb && m_lsb < newLsb)) {
                // When the new bit range is out of m_refp, clear polarity because nodes below is
                // shifted out to zero.
                // This kind of clear may happen several times. e.g. (!(1'b1 >> 1)) >> 1
                polarity(سچ);
            }
            m_lsb = newLsb;
            m_msb = newMsb;
        }
        فراغ update_ٻٽو_انگ_Range(ثابت AstCCast* castp) {
            update_ٻٽو_انگ_Range(m_lsb, std::min(m_msb, m_lsb + castp->ويڪر() - 1));
        }
        فراغ update_ٻٽو_انگ_Range(ثابت Ast_سرڪايو_R* shiftp) {
            update_ٻٽو_انگ_Range(m_lsb + VN_AS(shiftp->rhsp(), Const)->منزل_UInt(), m_msb);
        }
        فراغ حد_بندي_ٻٽو_انگ_Range_منزل_Lsb() { update_ٻٽو_انگ_Range(m_lsb, std::min(m_msb, m_lsb)); }
        صحيح لفظ_Idx() ثابت { أرجع m_لفظ_Idx; }
        فراغ لفظ_Idx(صحيح i) { m_لفظ_Idx = i; }
        منطقي polarity() ثابت { أرجع m_polarity; }
        فراغ polarity(منطقي p) { m_polarity = p; }

        AstVarRef* refp() ثابت { أرجع m_refp; }
        ثابت AstConst* constp() ثابت { أرجع m_constp; }
        منطقي missing_لفظ_Sel() ثابت {
            // When V3Expand is skipped, WordSel is not inserted.
            أرجع m_refp->isWide() && m_لفظ_Idx == -1;
        }
        صحيح lsb() ثابت { أرجع m_lsb; }

        صحيح msb() ثابت { أرجع std::min(m_msb, var_ويڪر() - 1); }
        صحيح var_ويڪر() ثابت {
            UASSERT(m_refp, "m_refp should be set");
            ثابت صحيح ويڪر = m_refp->varp()->ويڪر_Min();
            إذا (!m_refp->isWide()) {
                UASSERT_OBJ(m_لفظ_Idx == -1, m_refp, "Bad word index into non-wide");
                أرجع ويڪر;
            } وإلا {
                إذا (missing_لفظ_Sel()) أرجع ويڪر;
                UASSERT_OBJ(m_لفظ_Idx >= 0, m_refp, "Bad word index into wide");
                ثابت صحيح بٽ_اندريون_MSW = VL_BITBIT_E(ويڪر) ? VL_BITBIT_E(ويڪر) : VL_EDATASIZE;
                أرجع m_لفظ_Idx == m_refp->ويڪر_لفظ() - 1 ? بٽ_اندريون_MSW : VL_EDATASIZE;
            }
        }
    };

    بنية ٻٽو_انگ_Polarity_داخلا final {  // Found bit polarity during iterate()
        پن_Info m_info;  // Leaf (variable or constant) bit polarity was found on
        منطقي m_polarity = خطأ;  // Polarity the bit must have to match
        صحيح m_ٻٽو_انگ = 0;  // Bit index within the leaf that was tested
        ٻٽو_انگ_Polarity_داخلا(ثابت پن_Info& info, منطقي pol, صحيح ٻٽو_انگ_2)
            : m_info{info}
            , m_polarity{pol}
            , m_ٻٽو_انگ{ٻٽو_انگ_2} {}
        ٻٽو_انگ_Polarity_داخلا() = افتراضي;
    };

    بنية Frozen_ڳنڍ_Info final {  // Context when a frozen node is found
        منطقي m_polarity;  // Polarity the frozen node must match
        صحيح m_lsb;  // LSB position of the frozen node
        منطقي عامل<(ثابت Frozen_ڳنڍ_Info& other) ثابت {
            إذا (m_lsb != other.m_lsb) أرجع m_lsb < other.m_lsb;
            أرجع m_polarity < other.m_polarity;
        }
    };

    صنف Restorer final {  // Restore the original state unless disableRestore() is called
        Const_ٻٽو_انگ_Op_وڻ_Visitor& m_visitor;  // Visitor whose state is saved and restored
        ثابت سائيز_t m_polarities_سائيز;  // Saved m_visitor.m_bitPolarities size to truncate back to
        ثابت سائيز_t m_frozen_سائيز;  // Saved m_visitor.m_frozenNodes size to truncate back to
        ثابت بنانشان m_ops;  // Saved m_visitor.m_ops to restore
        ثابت منطقي m_polarity;  // Saved m_visitor.m_polarity to restore
        منطقي m_restore = سچ;  // Whether the destructor still needs to restore state

    عام:
        واضح Restorer(Const_ٻٽو_انگ_Op_وڻ_Visitor& visitor)
            : m_visitor{visitor}
            , m_polarities_سائيز{visitor.m_ٻٽو_انگ_Polarities.سائيز()}
            , m_frozen_سائيز{visitor.m_frozenNodes.سائيز()}
            , m_ops{visitor.m_ops}
            , m_polarity{visitor.m_polarity} {}
        ~Restorer() {
            UASSERT(m_visitor.m_ٻٽو_انگ_Polarities.سائيز() >= m_polarities_سائيز,
                    "m_bitPolarities must grow monotonically");
            UASSERT(m_visitor.m_frozenNodes.سائيز() >= m_frozen_سائيز,
                    "m_frozenNodes must grow monotonically");
            إذا (m_restore) restoreNow();
        }
        فراغ disableRestore() { m_restore = خطأ; }
        فراغ restoreNow() {
            UASSERT(m_restore, "Can be called just once");
            m_visitor.m_ٻٽو_انگ_Polarities.resize(m_polarities_سائيز);
            m_visitor.m_frozenNodes.resize(m_frozen_سائيز);
            m_visitor.m_ops = m_ops;
            m_visitor.m_polarity = m_polarity;
            m_restore = خطأ;
        }
    };
    // Collect information for each Variable to transform as below
    صنف VarInfo final {
        // MEMBERS
        صحيح m_معلوم_نتيجو = -1;  // -1: result is not known, 0 or 1: result of this tree
        ثابت Const_ٻٽو_انگ_Op_وڻ_Visitor* ثابت
            m_parentp;  // ConstBitOpTreeVisitor holding this VarInfo
        AstVarRef* ثابت m_refp;  // Points the variable that this VarInfo covers
        ثابت صحيح m_ويڪر;  // Width of term this VarInfo refers to
        V3_عدد m_ٻٽو_انگ_Polarity;  // Coefficient of each bit

    عام:
        // METHODS
        منطقي has_Const_نتيجو() ثابت { أرجع m_معلوم_نتيجو >= 0 || m_ٻٽو_انگ_Polarity.isAllX(); }
        // The constant result. Only valid if hasConstResult() returned true.
        منطقي get_Const_نتيجو() ثابت {
            // Note that this condition covers m_knownResult == -1 but m_bitPolarity.isAllX(),
            // in which case the result is 0
            أرجع m_معلوم_نتيجو == 1;
        }
        ثابت AstVarRef* refp() ثابت { أرجع m_refp; }
        منطقي sameVarAs(ثابت Ast_ڳنڍ_Var_Ref* otherp) ثابت { أرجع m_refp->same_ڳنڍ(otherp); }
        فراغ سيٽ_Polarity(منطقي comp_ٻٽو_انگ, صحيح ٻٽو_انگ_2) {
            // Ignore if already determined a known reduction
            إذا (m_معلوم_نتيجو >= 0) أرجع;
            UASSERT_OBJ(ٻٽو_انگ_2 < m_ويڪر, m_refp,
                        "Bit index out of range: " << ٻٽو_انگ_2 << " width: " << m_ويڪر);
            إذا (m_ٻٽو_انگ_Polarity.ٻٽو_انگ_Is_X(ٻٽو_انگ_2)) {  // The bit is not yet marked with either polarity
                m_ٻٽو_انگ_Polarity.سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, comp_ٻٽو_انگ);
            } وإلا {  // The bit has already been marked with some polarity
                ثابت منطقي sameFlag = m_ٻٽو_انگ_Polarity.ٻٽو_انگ_Is1(ٻٽو_انگ_2) == comp_ٻٽو_انگ;
                إذا (m_parentp->is_Xor_وڻ()) {
                    UASSERT_OBJ(comp_ٻٽو_انگ && sameFlag, m_refp, "Only true is set in Xor tree");
                    // a ^ a ^ b == b so we can ignore a
                    m_ٻٽو_انگ_Polarity.سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 'x');
                } وإلا {  // And, Or
                    // Can ignore this nodep as the bit is already marked with the same polarity
                    إذا (sameFlag) أرجع;  // a & a == a, b | b == b
                    // Otherwise result is constant (a & ~a == 0) or (a | ~a == 1)
                    m_معلوم_نتيجو = m_parentp->is_And_وڻ() ? 0 : 1;
                    m_ٻٽو_انگ_Polarity.سيٽ_All_بٽ_X();  // The variable is not referred anymore
                }
            }
        }

        // Return reduction term for this VarInfo, together with the number of ops in the term,
        // and a boolean indicating if the term is clean (1-bit vs multi-bit value)
        نتيجو_رڪن get_نتيجو_رڪن() ثابت {
            UASSERT_OBJ(!has_Const_نتيجو(), m_refp, "getTerm on reduction that yields constant");
            File_سٽ* ثابت fl = m_refp->fileline();

            // Get the term we are referencing (the WordSel, if wide, otherwise just the VarRef)
            Ast_ڳنڍ_اظهار* srcp = VN_CAST(m_refp->backp(), لفظ_Sel);
            إذا (!srcp) srcp = m_refp;
            srcp = srcp->clone_وڻ(خطأ);

            // Signed variables might have redundant sign bits that need masking.
            ثابت منطقي has_Redundant_Sign_بٽ
                = m_refp->varp()->dtypep()->isSigned()
                  && (m_refp->isWide() ? (m_ويڪر != VL_EDATASIZE)
                                       : (m_ويڪر < 8 || !isPow2(m_ويڪر)));

            // Get the mask that selects the bits that are relevant in this term
            V3_عدد نقاب_انگ{srcp, m_ويڪر, 0};
            نقاب_انگ.op_بٽ_Non_XZ(m_ٻٽو_انگ_Polarity);  // 'x' -> 0, 0->1, 1->1
            ثابت uint64_t نقاب_Val = نقاب_انگ.منزل_UQuad();
            UASSERT_OBJ(نقاب_Val != 0, m_refp,
                        "Should have been recognized as having const 0 result");

            // Parts of the return value
            Ast_ڳنڍ_اظهار* resultp = srcp;  // The tree for this term
            بنانشان ops = 0;  // Number of ops in this term
            منطقي clean = خطأ;  // Whether the term is clean (has value 0 or 1)

            إذا (isPow2(نقاب_Val)) {
                // If we only want a single bit, shift it out instead of a masked compare. Shifts
                // don't go through the flags register on x86 and are hence faster. This is also
                // always fewer or same ops as mask and compare, but with shorter instructions on
                // x86.

                // Find the index of the bit we want.
                ثابت صحيح ٻٽو_انگ_2 = ڳڻپ_Trailing_Zeroes(نقاب_Val);
                // If we want something other than the bottom bit, shift it out
                إذا (ٻٽو_انگ_2 != 0) {
                    resultp = جديد Ast_سرڪايو_R{fl, resultp,
                                            جديد AstConst{fl, static_cast<uint32_t>(ٻٽو_انگ_2)}, m_ويڪر};
                    ++ops;
                }
                // Negate it if necessary
                ثابت منطقي negate = m_ٻٽو_انگ_Polarity.ٻٽو_انگ_Is0(ٻٽو_انگ_2);
                إذا (negate) {
                    resultp = جديد AstNot{fl, resultp};
                    ++ops;
                }
                // Clean if MSB of unsigned value, and not negated
                clean = (ٻٽو_انگ_2 == m_ويڪر - 1) && !has_Redundant_Sign_بٽ && !negate;
            } وإلا {
                // We want multiple bits. Go ahead and extract them.

                // Check if masking is required, and if so apply it
                ثابت منطقي needsMasking = نقاب_Val != VL_نقاب_Q(m_ويڪر) || has_Redundant_Sign_بٽ;
                إذا (needsMasking) {
                    resultp = جديد AstAnd{fl, جديد AstConst{fl, نقاب_انگ}, resultp};
                    ++ops;
                }

                // Create the sub-expression for this term
                إذا (m_parentp->is_Xor_وڻ()) {
                    إذا (needsMasking) {
                        // Reduce the masked term to the minimum known width,
                        // to use the smallest RedXor formula
                        ثابت صحيح ويڪر_Min = نقاب_انگ.ويڪر_منزل_Fit();
                        resultp->dtype_Chg_ويڪر(ويڪر_Min, ويڪر_Min);
                    }
                    resultp = جديد AstRedXor{fl, resultp};
                    ++ops;
                    clean = خطأ;
                    // VL_REDXOR_* returns IData, set width accordingly to avoid unnecessary casts
                    resultp->dtype_Chg_ويڪر(VL_IDATASIZE, 1);
                } وإلا إذا (m_parentp->is_And_وڻ()) {
                    V3_عدد comp_انگ{srcp, m_ويڪر, 0};
                    comp_انگ.op_بٽ_One(m_ٻٽو_انگ_Polarity);  // 'x'->0, 0->0, 1->1
                    resultp = جديد AstEq{fl, جديد AstConst{fl, comp_انگ}, resultp};
                    ++ops;
                    clean = سچ;
                } وإلا {  // Or
                    V3_عدد comp_انگ{srcp, m_ويڪر, 0};
                    comp_انگ.op_بٽ_One(m_ٻٽو_انگ_Polarity);  // 'x'->0, 0->0, 1->1
                    comp_انگ.opXor(V3_عدد{comp_انگ}, نقاب_انگ);
                    resultp = جديد AstNeq{fl, جديد AstConst{fl, comp_انگ}, resultp};
                    ++ops;
                    clean = سچ;
                }
            }

            أرجع نتيجو_رڪن{resultp, ops, clean};
        }

        // CONSTRUCTORS
        VarInfo(Const_ٻٽو_انگ_Op_وڻ_Visitor* والد, AstVarRef* refp, صحيح ويڪر)
            : m_parentp{والد}
            , m_refp{refp}
            , m_ويڪر{ويڪر}
            , m_ٻٽو_انگ_Polarity{refp, m_ويڪر} {
            m_ٻٽو_انگ_Polarity.سيٽ_All_بٽ_X();
        }
    };

    // MEMBERS
    منطقي m_failed = خطأ;
    منطقي m_polarity = سچ;  // Flip when AstNot comes
    بنانشان m_ops;  // Number of operations such as And, Or, Xor, Sel...
    صحيح m_lsb = 0;  // Current LSB
    پن_Info* m_leafp = خالي_اشارو;  // AstConst or AstVarRef that currently looking for
    ثابت Ast_ڳنڍ_اظهار* ثابت m_rootp;  // Root of this AST subtree

    std::ويڪٽر<std::pair<Ast_ڳنڍ_اظهار*, Frozen_ڳنڍ_Info>>
        m_frozenNodes;  // Nodes that cannot be optimized
    std::ويڪٽر<ٻٽو_انگ_Polarity_داخلا> m_ٻٽو_انگ_Polarities;  // Polarity of bits found during iterate()
    std::ويڪٽر<std::unique_ptr<VarInfo>> m_varInfos;  // VarInfo for each variable, [0] is nullptr
    VNIdAllocator& m_ids;  // Node id allocator, owned by ConstVisitor
    // Base index of m_varInfos that points VarInfo, keyed by AstVarScope/AstVarRef id,
    // zero means not set yet.
    std::unordered_map<سائيز_t, صحيح> m_بنياد_Idxs;

    // METHODS

    منطقي is_And_وڻ() ثابت { أرجع VN_IS(m_rootp, And); }
    منطقي is_Or_وڻ() ثابت { أرجع VN_IS(m_rootp, Or); }
    منطقي is_Xor_وڻ() ثابت { أرجع VN_IS(m_rootp, Xor) || VN_IS(m_rootp, RedXor); }

#وصف_ڪريو CONST_BITOP_RETURN_IF(cond, nodep) \
    إذا (سيٽ_Failed(cond, #cond, nodep, __LINE__)) أرجع

#وصف_ڪريو CONST_BITOP_سيٽ_FAILED(reason, nodep) سيٽ_Failed(سچ, reason, nodep, __LINE__)

    منطقي سيٽ_Failed(منطقي fail, ثابت حرف* reason, ثابت Ast_ڳنڍ* nodep, صحيح سٽ) {
        إذا (fail && !m_failed) {
            UINFO(9, "cannot optimize " << m_rootp << " reason:" << reason << " called from line:"
                                        << سٽ << " when checking:" << nodep);
            // UINFOTREE(9, m_rootp, "", "root");
            m_failed = سچ;
        }
        أرجع m_failed;
    }
    فراغ incrOps(ثابت Ast_ڳنڍ* nodep, صحيح سٽ) {
        ++m_ops;
        UINFO(9, "Increment to " << m_ops << " " << nodep << " called from line " << سٽ);
    }
    VarInfo& getVarInfo(ثابت پن_Info& ref) {
        UASSERT_OBJ(ref.refp(), m_rootp, "null varref in And/Or/Xor optimization");
        Ast_ڳنڍ* nodep = ref.refp()->varScopep();
        إذا (!nodep) nodep = ref.refp()->varp();  // Not scoped
        صحيح& بنياد_Idxr = m_بنياد_Idxs[m_ids(nodep)];
        إذا (بنياد_Idxr == 0) {  // Not set yet
            بنياد_Idxr = m_varInfos.سائيز();
            ثابت صحيح انگ_لفظ
                = ref.refp()->dtypep()->isWide() ? ref.refp()->dtypep()->ويڪر_لفظ() : 1;
            m_varInfos.resize(m_varInfos.سائيز() + انگ_لفظ);
        }
        ثابت سائيز_t idx = بنياد_Idxr + std::max(0, ref.لفظ_Idx());
        VarInfo* varInfop = m_varInfos[idx].get();
        إذا (!varInfop) {
            varInfop = جديد VarInfo{هذا, ref.refp(), ref.var_ويڪر()};
            m_varInfos[idx].reset(varInfop);
            إذا (ref.missing_لفظ_Sel()) {
                // ConstBitOpTreeVisitor makes some constants for masks and its type is uint64_t.
                // That's why V3Expand, that inserts WordSel, is needed.
                CONST_BITOP_سيٽ_FAILED("V3Expand is skipped", ref.refp());
            }
        } وإلا {
            إذا (!varInfop->sameVarAs(ref.refp()))
                CONST_BITOP_سيٽ_FAILED("different var (scope?)", ref.refp());
        }
        أرجع *varInfop;
    }

    // Traverse down to see AstConst or AstVarRef
    پن_Info ڳوليو_پن(Ast_ڳنڍ* nodep, منطقي expectConst) {
        إذا (!nodep->dtypep()->skipRefp()->isIntegralOrPacked()) أرجع پن_Info{};
        پن_Info info{m_lsb};
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
        أرجع ok ? info : پن_Info{};
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ* nodep) override { CONST_BITOP_سيٽ_FAILED("Hit unexpected op", nodep); }
    فراغ گهمو(AstCCast* nodep) override {
        iterateChildrenConst(nodep);
        إذا (m_leafp) m_leafp->update_ٻٽو_انگ_Range(nodep);
    }
    فراغ گهمو(Ast_سرڪايو_R* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ثابت AstConst* ثابت constp = VN_CAST(nodep->rhsp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->rhsp());
        m_lsb += constp->منزل_UInt();
        incrOps(nodep, __LINE__);
        iterateConst(nodep->lhsp());
        m_leafp->update_ٻٽو_انگ_Range(nodep);
        m_lsb -= constp->منزل_UInt();
    }
    فراغ گهمو(AstNot* nodep) override {
        CONST_BITOP_RETURN_IF(nodep->ويڪر_Min() != 1, nodep);
        Ast_ڳنڍ* lhsp = nodep->lhsp();
        AstCCast* ثابت castp = VN_CAST(lhsp, CCast);
        إذا (castp) lhsp = castp->lhsp();
        CONST_BITOP_RETURN_IF(!is_Xor_وڻ() && !VN_IS(lhsp, VarRef) && !VN_IS(lhsp, سرڪايو_R), lhsp);
        incrOps(nodep, __LINE__);
        m_polarity = !m_polarity;
        iterateChildrenConst(nodep);
        // Don't restore m_polarity for Xor as it counts parity of the entire tree
        إذا (!is_Xor_وڻ()) m_polarity = !m_polarity;
        إذا (m_leafp && castp) m_leafp->update_ٻٽو_انگ_Range(castp);
        إذا (m_leafp) m_leafp->polarity(!m_leafp->polarity());
    }
    فراغ گهمو(Ast_لفظ_Sel* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ثابت AstConst* ثابت constp = VN_CAST(nodep->bitp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->bitp());
        UASSERT_OBJ(m_leafp->لفظ_Idx() == -1, nodep, "Unexpected nested WordSel");
        m_leafp->لفظ_Idx(constp->منزل_SInt());
        iterateConst(nodep->fromp());
    }
    فراغ گهمو(AstVarRef* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->سيٽ_پن(nodep);
    }
    فراغ گهمو(AstConst* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->سيٽ_پن(nodep);
    }

    فراغ گهمو(AstRedXor* nodep) override {
        Restorer restorer{*هذا};
        CONST_BITOP_RETURN_IF(!VN_IS(m_rootp, Xor), nodep);
        Ast_ڳنڍ* lhsp = nodep->lhsp();
        ثابت AstCCast* ثابت castp = VN_CAST(lhsp, CCast);
        إذا (castp) lhsp = castp->lhsp();
        إذا (ثابت AstAnd* ثابت andp = VN_CAST(lhsp, And)) {  // '^(mask & leaf)'
            CONST_BITOP_RETURN_IF(!andp, lhsp);

            ثابت پن_Info& نقاب = ڳوليو_پن(andp->lhsp(), سچ);
            CONST_BITOP_RETURN_IF(!نقاب.constp() || نقاب.lsb() != 0, andp->lhsp());

            پن_Info ref = ڳوليو_پن(andp->rhsp(), خطأ);
            CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());
            إذا (castp) ref.update_ٻٽو_انگ_Range(castp);

            restorer.disableRestore();  // Now all subtree succeeded

            ثابت V3_عدد& نقاب_انگ = نقاب.constp()->انگ();

            incrOps(nodep, __LINE__);
            incrOps(andp, __LINE__);

            // Mark all bits checked in this reduction
            ثابت صحيح max_ٻٽو_انگ_Idx = std::min(ref.lsb() + نقاب_انگ.ويڪر(), ref.msb() + 1);
            لكل (صحيح ٻٽو_انگ_Idx = ref.lsb(); ٻٽو_انگ_Idx < max_ٻٽو_انگ_Idx; ++ٻٽو_انگ_Idx) {
                ثابت صحيح نقاب_Idx = ٻٽو_انگ_Idx - ref.lsb();
                إذا (نقاب_انگ.ٻٽو_انگ_Is0(نقاب_Idx)) استمر;
                // Set true, m_polarity takes care of the entire parity
                m_ٻٽو_انگ_Polarities.emplace_پٺ(ref, سچ, ٻٽو_انگ_Idx);
            }
        } وإلا {  // '^leaf'
            پن_Info ref = ڳوليو_پن(lhsp, خطأ);
            CONST_BITOP_RETURN_IF(!ref.refp(), lhsp);
            إذا (castp) ref.update_ٻٽو_انگ_Range(castp);

            restorer.disableRestore();  // Now all checks passed

            incrOps(nodep, __LINE__);

            // Mark all bits checked by this comparison
            لكل (صحيح ٻٽو_انگ_Idx = ref.lsb(); ٻٽو_انگ_Idx <= ref.msb(); ++ٻٽو_انگ_Idx) {
                m_ٻٽو_انگ_Polarities.emplace_پٺ(ref, سچ, ٻٽو_انگ_Idx);
            }
        }
    }

    فراغ گهمو(Ast_ڳنڍ_Biop* nodep) override {
        إذا (VN_IS(nodep, And) && isConst(nodep->lhsp(), 1)) {  // 1 & _
            // Always reach past a plain making AND
            Restorer restorer{*هذا};
            incrOps(nodep, __LINE__);
            iterateConst(nodep->rhsp());
            إذا (m_leafp) m_leafp->حد_بندي_ٻٽو_انگ_Range_منزل_Lsb();
            CONST_BITOP_RETURN_IF(m_failed, nodep->rhsp());
            restorer.disableRestore();  // Now all checks passed
        } وإلا إذا (nodep->type() == m_rootp->type()) {  // And, Or, Xor
            // subtree under NOT can be optimized only in XOR tree.
            CONST_BITOP_RETURN_IF(!m_polarity && !is_Xor_وڻ(), nodep);
            incrOps(nodep, __LINE__);
            لكل (ثابت منطقي ساڄي : {خطأ, سچ}) {
                VL_RESTORER(m_leafp);
                Restorer restorer{*هذا};
                پن_Info پن_Info_2{m_lsb};
                // cppcheck-suppress danglingLifetime
                m_leafp = &پن_Info_2;
                Ast_ڳنڍ_اظهار* opp = ساڄي ? nodep->rhsp() : nodep->lhsp();
                ثابت منطقي origFailed = m_failed;
                iterateConst(opp);
                إذا (پن_Info_2.constp() || m_failed) {
                    // Revert changes in leaf
                    restorer.restoreNow();
                    // Reach past a cast then add to frozen nodes to be added to final reduction
                    إذا (ثابت AstCCast* ثابت castp = VN_CAST(opp, CCast)) opp = castp->lhsp();
                    ثابت منطقي pol = is_Xor_وڻ() || m_polarity;  // Only AND/OR tree needs polarity
                    UASSERT_OBJ(pol, nodep, "AND/OR tree expects m_polarity==true");
                    m_frozenNodes.emplace_پٺ(opp, Frozen_ڳنڍ_Info{pol, m_lsb});
                    m_failed = origFailed;
                    استمر;
                }
                restorer.disableRestore();  // Now all checks passed
                إذا (پن_Info_2.refp()) {
                    // The conditional on the lsb being in range is necessary for some degenerate
                    // case, e.g.: (IData)((QData)wide[0] >> 32), or <1-bit-var> >> 1, which is
                    // just zero
                    إذا (پن_Info_2.lsb() <= پن_Info_2.msb()) {
                        m_ٻٽو_انگ_Polarities.emplace_پٺ(پن_Info_2, is_Xor_وڻ() || پن_Info_2.polarity(),
                                                     پن_Info_2.lsb());
                    } وإلا إذا ((is_And_وڻ() && پن_Info_2.polarity())
                               || (is_Or_وڻ() && !پن_Info_2.polarity())) {
                        // If there is a constant 0 term in an And tree or 1 term in an Or tree, we
                        // must include it. Fudge this by adding a bit with both polarities, which
                        // will simplify to zero or one respectively.
                        // Note that Xor tree does not need this kind of care, polarity of Xor tree
                        // is already cared when visitin AstNot. Taking xor with 1'b0 is nop.
                        m_ٻٽو_انگ_Polarities.emplace_پٺ(پن_Info_2, سچ, 0);
                        m_ٻٽو_انگ_Polarities.emplace_پٺ(پن_Info_2, خطأ, 0);
                    }
                }
            }
        } وإلا إذا ((is_And_وڻ() && VN_IS(nodep, Eq)) || (is_Or_وڻ() && VN_IS(nodep, Neq))) {
            Restorer restorer{*هذا};
            CONST_BITOP_RETURN_IF(!m_polarity, nodep);
            CONST_BITOP_RETURN_IF(m_lsb, nodep);  // the result of EQ/NE is 1 bit width
            ثابت Ast_ڳنڍ* lhsp = nodep->lhsp();
            إذا (ثابت AstCCast* ثابت castp = VN_CAST(lhsp, CCast)) lhsp = castp->lhsp();
            ثابت AstConst* ثابت constp = VN_CAST(lhsp, Const);
            CONST_BITOP_RETURN_IF(!constp, nodep->lhsp());

            ثابت V3_عدد& comp_انگ = constp->انگ();

            خودڪار سيٽ_Polarities = [هذا, &comp_انگ](ثابت پن_Info& ref, ثابت V3_عدد* maskp) {
                ثابت منطقي نقاب_Flip = is_And_وڻ() ^ ref.polarity();
                صحيح constant_ويڪر = comp_انگ.ويڪر();
                إذا (maskp) constant_ويڪر = std::max(constant_ويڪر, maskp->ويڪر());
                ثابت صحيح max_ٻٽو_انگ_Idx = std::max(ref.lsb() + constant_ويڪر, ref.msb() + 1);
                // Mark all bits checked by this comparison
                لكل (صحيح ٻٽو_انگ_Idx = ref.lsb(); ٻٽو_انگ_Idx < max_ٻٽو_انگ_Idx; ++ٻٽو_انگ_Idx) {
                    ثابت صحيح نقاب_Idx = ٻٽو_انگ_Idx - ref.lsb();
                    ثابت منطقي mask0 = maskp && maskp->ٻٽو_انگ_Is0(نقاب_Idx);
                    ثابت منطقي ٻاهريون_جو_Range = ٻٽو_انگ_Idx > ref.msb();
                    إذا (mask0 || ٻاهريون_جو_Range) {  // RHS is 0
                        إذا (comp_انگ.ٻٽو_انگ_Is1(نقاب_Idx)) {
                            // LHS is 1
                            // And tree: 1 == 0 => always false, set v && !v
                            // Or tree : 1 != 0 => always true, set v || !v
                            m_ٻٽو_انگ_Polarities.emplace_پٺ(ref, سچ, 0);
                            m_ٻٽو_انگ_Polarities.emplace_پٺ(ref, خطأ, 0);
                            اكسر;
                        } وإلا {  // This bitIdx is irrelevant
                            استمر;
                        }
                    }
                    ثابت منطقي polarity = comp_انگ.ٻٽو_انگ_Is1(نقاب_Idx) != نقاب_Flip;
                    m_ٻٽو_انگ_Polarities.emplace_پٺ(ref, polarity, ٻٽو_انگ_Idx);
                }
            };

            إذا (ثابت AstAnd* ثابت andp = VN_CAST(nodep->rhsp(), And)) {  // comp == (mask & v)
                ثابت پن_Info& نقاب = ڳوليو_پن(andp->lhsp(), سچ);
                CONST_BITOP_RETURN_IF(!نقاب.constp() || نقاب.lsb() != 0, andp->lhsp());

                ثابت پن_Info& ref = ڳوليو_پن(andp->rhsp(), خطأ);
                CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());

                restorer.disableRestore();  // Now all checks passed

                ثابت V3_عدد& نقاب_انگ = نقاب.constp()->انگ();

                incrOps(nodep, __LINE__);
                incrOps(andp, __LINE__);

                سيٽ_Polarities(ref, &نقاب_انگ);
            } وإلا {  // comp == v
                ثابت پن_Info& ref = ڳوليو_پن(nodep->rhsp(), خطأ);
                CONST_BITOP_RETURN_IF(!ref.refp(), nodep->rhsp());

                restorer.disableRestore();  // Now all checks passed

                incrOps(nodep, __LINE__);

                سيٽ_Polarities(ref, خالي_اشارو);
            }
        } وإلا {
            CONST_BITOP_سيٽ_FAILED("Mixture of different ops cannot be optimized", nodep);
        }
    }

    // CONSTRUCTORS
    Const_ٻٽو_انگ_Op_وڻ_Visitor(Ast_ڳنڍ_اظهار* nodep, بنانشان externalOps, VNIdAllocator& ids)
        : m_ops{externalOps}
        , m_rootp{nodep}
        , m_ids{ids} {
        // Fill nullptr at [0] because a base index of 0 means not set yet
        m_varInfos.ڌڪو_پٺ(خالي_اشارو);
        CONST_BITOP_RETURN_IF(!is_And_وڻ() && !is_Or_وڻ() && !is_Xor_وڻ(), nodep);
        إذا (Ast_ڳنڍ_Biop* ثابت biopp = VN_CAST(nodep, ڳنڍ_Biop)) {
            iterateConst(biopp);
        } وإلا {
            UASSERT_OBJ(VN_IS(nodep, RedXor), nodep, "Must be RedXor");
            incrOps(nodep, __LINE__);
            iterateChildrenConst(nodep);
        }
        لكل (خودڪار&& داخلا_2 : m_ٻٽو_انگ_Polarities) {
            getVarInfo(داخلا_2.m_info).سيٽ_Polarity(داخلا_2.m_polarity, داخلا_2.m_ٻٽو_انگ);
        }
        UASSERT_OBJ(is_Xor_وڻ() || m_polarity, nodep, "must be the original polarity");
    }
    مجازي ~Const_ٻٽو_انگ_Op_وڻ_Visitor() = افتراضي;
#وصف_رد CONST_BITOP_RETURN_IF
#وصف_رد CONST_BITOP_سيٽ_FAILED

عام:
    // Transform as below.
    // v[0] & v[1] => 2'b11 == (2'b11 & v)
    // v[0] | v[1] => 2'b00 != (2'b11 & v)
    // v[0] ^ v[1] => ^{2'b11 & v}
    // (3'b011 == (3'b011 & v)) & v[2]  => 3'b111 == (3'b111 & v)
    // (3'b000 != (3'b011 & v)) | v[2]  => 3'b000 != (3'b111 & v)
    // Reduction ops are transformed in the same way.
    // &{v[0], v[1]} => 2'b11 == (2'b11 & v)
    ساكن Ast_ڳنڍ_اظهار* simplify(Ast_ڳنڍ_اظهار* nodep, صحيح نتيجو_ويڪر, بنانشان externalOps,
                                 VDouble0& reduction, VNIdAllocator& ids) {
        UASSERT_OBJ(1 <= نتيجو_ويڪر && نتيجو_ويڪر <= 64, nodep, "resultWidth out of range");

        // Walk tree, gathering all terms referenced in expression
        ثابت Const_ٻٽو_انگ_Op_وڻ_Visitor visitor{nodep, externalOps, ids};

        // If failed on root node is not optimizable, or there are no variable terms, then done
        إذا (visitor.m_failed || visitor.m_varInfos.سائيز() == 1) أرجع خالي_اشارو;

        // FileLine used for constructing all new nodes in this function
        File_سٽ* ثابت fl = nodep->fileline();

        // Get partial result each term referenced, count total number of ops and keep track of
        // whether we have clean/dirty terms. visitor.m_varInfos appears in deterministic order,
        // so the optimized tree is deterministic as well.

        std::ويڪٽر<Ast_ڳنڍ_اظهار*> termps;
        termps.reserve(visitor.m_varInfos.سائيز() - 1);
        بنانشان نتيجو_Ops = 0;
        منطقي has_Clean_رڪن = خطأ;
        منطقي has_Dirty_رڪن = خطأ;

        لكل (خودڪار&& v : visitor.m_varInfos) {
            إذا (!v) استمر;  // Skip nullptr at m_varInfos[0]
            إذا (v->has_Const_نتيجو()) {
                // If a constant term is known, we can either drop it or the whole tree is constant
                Ast_ڳنڍ_اظهار* resultp = خالي_اشارو;
                إذا (v->get_Const_نتيجو()) {
                    UASSERT_OBJ(visitor.is_Or_وڻ(), nodep,
                                "Only OR tree can yield known 1 result");
                    UINFO(9, "OR tree with const 1 term: " << v->refp());
                    // Known 1 bit in OR tree, whole result is 1
                    resultp = جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}};
                } وإلا إذا (visitor.is_And_وڻ()) {
                    UINFO(9, "AND tree with const 0 term: " << v->refp());
                    // Known 0 bit in AND tree, whole result is 0
                    resultp = جديد AstConst{fl, AstConst::ٻٽو_انگ_False{}};
                } وإلا {
                    // Known 0 bit in OR or XOR tree. Ignore it.
                    استمر;
                }
                // Set width and widthMin precisely
                resultp->dtype_Chg_ويڪر(نتيجو_ويڪر, 1);
                لكل (Ast_ڳنڍ* ثابت termp : termps) VL_DO_DANGLING(termp->delete_وڻ(), termp);
                أرجع resultp;
            }
            ثابت نتيجو_رڪن نتيجو = v->get_نتيجو_رڪن();
            termps.ڌڪو_پٺ(std::get<0>(نتيجو));
            نتيجو_Ops += std::get<1>(نتيجو);
            إذا (std::get<2>(نتيجو)) {
                has_Clean_رڪن = سچ;
                UINFO(9, "Clean term: " << termps.پٺ());
            } وإلا {
                has_Dirty_رڪن = سچ;
                UINFO(9, "Dirty term: " << termps.پٺ());
            }
        }

        // Group by FrozenNodeInfo
        std::map<Frozen_ڳنڍ_Info, std::ويڪٽر<Ast_ڳنڍ_اظهار*>> frozenNodes;
        // Check if frozen terms are clean or not
        لكل (ثابت خودڪار& frozenInfo : visitor.m_frozenNodes) {
            Ast_ڳنڍ_اظهار* ثابت termp = frozenInfo.پهريون;
            // Comparison operators are clean
            إذا ((VN_IS(termp, Eq) || VN_IS(termp, Neq) || VN_IS(termp, Lt) || VN_IS(termp, Lte)
                 || VN_IS(termp, Gt) || VN_IS(termp, Gte))
                && frozenInfo.ٻيو.m_lsb == 0) {
                has_Clean_رڪن = سچ;
            } وإلا {
                // Otherwise, conservatively assume the frozen term is dirty
                has_Dirty_رڪن = سچ;
                UINFO(9, "Dirty frozen term: " << termp);
            }
            frozenNodes[frozenInfo.ٻيو].ڌڪو_پٺ(termp);
        }

        // Figure out if a final negation is required
        ثابت منطقي needsFlip = visitor.is_Xor_وڻ() && !visitor.m_polarity;

        // Figure out if the final tree needs cleaning
        ثابت منطقي needsCleaning = visitor.is_And_وڻ() ? !has_Clean_رڪن : has_Dirty_رڪن;

        // Add size of reduction tree to op count
        نتيجو_Ops += termps.سائيز() - 1;
        لكل (ثابت خودڪار& lsbAndNodes : frozenNodes) {
            إذا (lsbAndNodes.پهريون.m_lsb > 0) ++نتيجو_Ops;  // Needs AstShiftR
            إذا (!lsbAndNodes.پهريون.m_polarity) ++نتيجو_Ops;  // Needs AstNot
            نتيجو_Ops += lsbAndNodes.ٻيو.سائيز();
        }
        // Add final polarity flip in Xor tree
        إذا (needsFlip) ++نتيجو_Ops;
        // Add final cleaning AND
        إذا (needsCleaning) ++نتيجو_Ops;

        إذا (debug() >= 9) {  // LCOV_EXCL_START
            ٻاهر << "-  Bitop tree considered:\n";
            لكل (ثابت Ast_ڳنڍ_اظهار* ثابت termp : termps) termp->dump_وڻ("-  Reduced term: ");
            لكل (ثابت std::pair<Ast_ڳنڍ_اظهار*, Frozen_ڳنڍ_Info>& termp : visitor.m_frozenNodes) {
                termp.پهريون->dump_وڻ("-  Frozen term with lsb "
                                      + std::منزل_string(termp.ٻيو.m_lsb) + " polarity "
                                      + std::منزل_string(termp.ٻيو.m_polarity) + ": ");
            }
            ٻاهر << "-  Needs flipping: " << needsFlip << "\n";
            ٻاهر << "-  Needs cleaning: " << needsCleaning << "\n";
            ٻاهر << "-  Size: " << نتيجو_Ops << " input size: " << visitor.m_ops << "\n";
        }  // LCOV_EXCL_STOP

        // Sometimes we have no terms left after ignoring redundant terms
        // (all of which were zeroes)
        إذا (termps.خالي_جايون() && visitor.m_frozenNodes.خالي_جايون()) {
            reduction += visitor.m_ops;
            Ast_ڳنڍ_اظهار* ثابت resultp = needsFlip ? جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}}
                                                   : جديد AstConst{fl, AstConst::ٻٽو_انگ_False{}};
            resultp->dtype_Chg_ويڪر(نتيجو_ويڪر, 1);
            أرجع resultp;
        }

        // Only substitute the result if beneficial as determined by operation count
        إذا (visitor.m_ops <= نتيجو_Ops) {
            لكل (Ast_ڳنڍ* ثابت termp : termps) VL_DO_DANGLING(termp->delete_وڻ(), termp);
            أرجع خالي_اشارو;
        }

        // Update statistics
        reduction += visitor.m_ops - نتيجو_Ops;

        // Reduction op to combine terms
        ثابت خودڪار reduce = [&visitor, fl](Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp) -> Ast_ڳنڍ_اظهار* {
            إذا (!lhsp) أرجع rhsp;
            إذا (visitor.is_And_وڻ()) {
                أرجع جديد AstAnd{fl, lhsp, rhsp};
            } وإلا إذا (visitor.is_Or_وڻ()) {
                أرجع جديد AstOr{fl, lhsp, rhsp};
            } وإلا {
                أرجع جديد AstXor{fl, lhsp, rhsp};
            }
        };

        // Compute result by reducing all terms
        Ast_ڳنڍ_اظهار* resultp = خالي_اشارو;
        لكل (Ast_ڳنڍ_اظهار* ثابت termp : termps) {  //
            resultp = reduce(resultp, termp);
        }
        // Add any frozen terms to the reduction
        لكل (خودڪار&& nodes : frozenNodes) {
            // nodes.second has same lsb and polarity
            Ast_ڳنڍ_اظهار* termp = خالي_اشارو;
            لكل (Ast_ڳنڍ_اظهار* ثابت itemp : nodes.ٻيو) {
                termp = reduce(termp, itemp->unlink_Fr_پٺ());
            }
            إذا (nodes.پهريون.m_lsb > 0) {  // LSB is not 0, so shiftR
                Ast_ڳنڍ_DType* ثابت dtypep = termp->dtypep();
                termp = جديد Ast_سرڪايو_R{termp->fileline(), termp,
                                      جديد AstConst(termp->fileline(), AstConst::Widthed_قدر{},
                                                   termp->ويڪر(), nodes.پهريون.m_lsb)};
                termp->dtypep(dtypep);
            }
            إذا (!nodes.پهريون.m_polarity) {  // Polarity is inverted, so append Not
                Ast_ڳنڍ_DType* ثابت dtypep = termp->dtypep();
                termp = جديد AstNot{termp->fileline(), termp};
                termp->dtypep(dtypep);
            }
            resultp = reduce(resultp, termp);
        }

        // Set width of masks to expected result width. This is required to prevent later removal
        // of the masking node e.g. by the "AND with all ones" rule. If the result width happens
        // to be 1, we still need to ensure the AstAnd is not dropped, so use a wider mask in this
        // special case.
        ثابت صحيح نقاب_ويڪر
            = std::max(resultp->ويڪر(), نتيجو_ويڪر == 1 ? VL_IDATASIZE : نتيجو_ويڪر);

        // Apply final polarity flip
        إذا (needsFlip) {
            إذا (needsCleaning) {
                // Cleaning will be added below. Use a NOT which is a byte shorter on x86
                resultp = جديد AstNot{fl, resultp};
            } وإلا {
                // Keep result clean by using XOR(1, _)
                AstConst* ثابت maskp = جديد AstConst{fl, AstConst::Widthed_قدر{}, نقاب_ويڪر, 1};
                resultp = جديد AstXor{fl, maskp, resultp};
            }
        }

        // Apply final cleaning
        إذا (needsCleaning) {
            AstConst* ثابت maskp = جديد AstConst{fl, AstConst::Widthed_قدر{}, نقاب_ويڪر, 1};
            resultp = جديد AstAnd{fl, maskp, resultp};
        }

        // Cast back to original size if required
        إذا (resultp->ويڪر() != نتيجو_ويڪر) {
            resultp = جديد AstCCast{fl, resultp, نتيجو_ويڪر, 1};
        }

        // Set width and widthMin precisely
        resultp->dtype_Chg_ويڪر(نتيجو_ويڪر, 1);

        أرجع resultp;
    }
};

//######################################################################
// Const state, as a visitor of each AstNode

صنف ConstVisitor final : عام VNVisitor {
    // CONSTANTS
    ساكن constexpr بنانشان CONCAT_MERGABLE_MAX_اونهائي = 10;  // Limit alg recursion
    // Largest right-hand side that is copied to each part of a split concatenation assignment
    ساكن constexpr صحيح CONCAT_سونپيو_MAX_RHS_NODES = 16;

    // NODE STATE
    // See VNIdAllocator. All per node state below is held in side tables keyed on the
    // node ids it hands out, as node pointers are not stable keys within V3Const.

    // STATE
    VNIdAllocator m_ids;  // Node id allocator
    منطقي m_params = خطأ;  // If true, propagate parameterized and true numbers only
    منطقي m_required = خطأ;  // If true, must become a constant
    منطقي m_wremove = سچ;  // Inside scope, no assignw removal
    منطقي m_warn = خطأ;  // Output warnings
    منطقي m_doExpensive = خطأ;  // Enable computationally expensive optimizations
    منطقي m_doCpp = خطأ;  // Enable late-stage C++ optimizations
    منطقي m_doNConst = خطأ;  // Enable non-constant-child simplifications
    منطقي m_doV = خطأ;  // Verilog, not C++ conversion
    منطقي m_doGenerate = خطأ;  // Postpone width checking inside generate
    منطقي m_convert_Logic_منزل_ٻٽو_انگ = خطأ;  // Convert logical operators to bitwise
    منطقي m_hasJumpDelay = خطأ;  // JumpGo or Delay under this loop
    منطقي m_hasLoopTest = خطأ;  // Contains AstLoopTest
    منطقي m_underRecFunc = خطأ;  // Under a recursive function
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    ثابت AstArraySel* m_selp = خالي_اشارو;  // Current select
    ثابت AstScope* m_scopep = خالي_اشارو;  // Current scope
    ثابت Ast_Attr_جو* m_attrp = خالي_اشارو;  // Current attribute
    VDouble0 m_stat_ٻٽو_انگ_Op_Reduction;  // Ops reduced in ConstBitOpTreeVisitor
    VDouble0 m_stat_Concat_ملايو;  // Concat merges
    VDouble0 m_stat_Cond_اظهار_Redundant;  // Conditional repeated expressions
    VDouble0 m_stat_If_Cond_اظهار_Redundant;  // Conditional repeated expressions
    VDouble0 m_statMemberAccessVisits;  // Nodes visited by containsMemberAccessRecurse
    ثابت منطقي m_globalPass;  // ConstVisitor invoked as a global pass
    ساكن uint32_t s_global_Pass_انگ;  // Counts number of times ConstVisitor invoked as global pass
    V3_Unique_نالا m_concswap_نالا;  // For generating unique temporary variable names
    // Caches results of matchBiopToBitwise, keyed on node id
    std::unordered_map<سائيز_t, منطقي> m_شامل_آهي_Member_Access;
    // Items of enum currently being recursed into. Cannot use VNIdAllocator, as this runs in
    // every mode, including those invoked under a pass holding user4 (V3Param). Node pointers
    // are safe as keys here, unlike elsewhere in V3Const: folding the item value below does
    // free nodes whose addresses are then reused, but AstEnumItem is only ever constructed
    // while parsing (verilog.y and V3LinkParse), so a reused address can never become one,
    // and every key looked up is an AstEnumItem.
    std::unordered_سيٽ<ثابت Ast_Enum_شي*> m_recursing_Enum_شيون;
    std::unordered_سيٽ<Ast_Jump_بلاڪ*> m_استعمال_ٿيل_Jump_بلاڪون;  // JumpBlocks used by some JumpGo

    // METHODS

    // Return true if statements may be deleted. Until statics are lifted, a variable declared
    // in a statement might be referenced from elsewhere (e.g. hierarchically).
    ساكن منطقي canDeleteStatements() { أرجع v3Global.staticsLifted(); }

    فراغ deleteVarScopesUnder(Ast_ڳنڍ* subtreep) {
        إذا (!subtreep) أرجع;
        إذا (!m_scopep) أرجع;
        std::unordered_سيٽ<AstVar*> varps;
        subtreep->foreach_And_ايندڙ([&](AstVar* varp) { varps.وجهو(varp); });
        إذا (varps.خالي_جايون()) أرجع;
        لكل (AstVarScope *vscp = m_scopep->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            إذا (varps.ڳوليو(vscp->varp()) != varps.پڇاڙي()) {
                VL_DO_DANGLING(ڌڪو_Deletep(vscp->unlink_Fr_پٺ()), vscp);
            }
        }
    }

    V3_عدد const_انگ_V(Ast_ڳنڍ* nodep) {
        // Contract C width to V width (if needed, else just direct copy)
        // The upper zeros in the C representation can otherwise cause
        // wrong results in some operations, e.g. MulS
        ثابت V3_عدد& numc = VN_AS(nodep, Const)->انگ();
        أرجع !numc.is_عدد() ? numc : V3_عدد{nodep, nodep->ويڪر_Min_V(), numc};
    }
    V3_عدد منزل_انگ_C(Ast_ڳنڍ* nodep, ثابت V3_عدد& numv) {
        // Extend V width back to C width for given node
        أرجع !numv.is_عدد() ? numv : V3_عدد{nodep, nodep->ويڪر(), numv};
    }

    ساكن منطقي هيٺاهون_As_Fixed_Aggregate(ثابت Ast_ڳنڍ_DType* ثابت dtypep) {
        أرجع dtypep->isStreamableFixedAggregate() && dtypep->شامل_آهي_Unpacked_Struct();
    }

    AstStructSel* newStructSel(Ast_ڳنڍ_اظهار* ثابت fromp, ثابت AstMemberDType* ثابت itemp) {
        AstStructSel* ثابت selp = جديد AstStructSel{fromp->fileline(), fromp, itemp->name()};
        selp->dtypeFrom(itemp->dtypep());
        أرجع selp;
    }

    فراغ collectFixedAggregateTerms(Ast_ڳنڍ_اظهار* ثابت fromp, std::ويڪٽر<Ast_ڳنڍ_اظهار*>& termps,
                                    ثابت منطقي packReal) {
        ثابت Ast_ڳنڍ_DType* ثابت dtypep = fromp->dtypep()->skipRefp();
        إذا (ثابت AstUnpackArrayDType* ثابت unpackDtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            لكل (ثابت صحيح idx : unpackDtypep->declRange().seq_کاٻو_منزل_ساڄو()) {
                AstArraySel* ثابت selp
                    = جديد AstArraySel{fromp->fileline(), fromp->clone_وڻ_خالص(خطأ), idx};
                collectFixedAggregateTerms(selp, termps, packReal);
            }
            VL_DO_DANGLING(ڌڪو_Deletep(fromp), fromp);
        } وإلا إذا (ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت sdtypep
                   = VN_CAST(dtypep, ڳنڍ_UOr_Struct_DType)) {
            إذا (sdtypep->packed()) {
                termps.ڌڪو_پٺ(fromp);
                أرجع;
            }
            لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                collectFixedAggregateTerms(newStructSel(fromp->clone_وڻ_خالص(خطأ), itemp),
                                           termps, packReal);
            }
            VL_DO_DANGLING(ڌڪو_Deletep(fromp), fromp);
        } وإلا إذا (packReal && dtypep->isDouble()) {
            termps.ڌڪو_پٺ(جديد Ast_Real_منزل_بٽ{fromp->fileline(), fromp});
        } وإلا {
            termps.ڌڪو_پٺ(fromp);
        }
    }

    Ast_ڳنڍ_اظهار* packFixedAggregate(Ast_ڳنڍ_اظهار* ثابت fromp) {
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> termps;
        collectFixedAggregateTerms(fromp, termps, سچ);
        UASSERT(!termps.خالي_جايون(), "No stream terms");
        Ast_ڳنڍ_اظهار* resultp = termps[0];
        لكل (سائيز_t i = 1; i < termps.سائيز(); ++i) {
            resultp = جديد AstConcat{resultp->fileline(), resultp, termps[i]};
        }
        أرجع resultp;
    }

    فراغ replace_سونپيو_منزل_Fixed_Aggregate(Ast_ڳنڍ_سونپيو* ثابت nodep, Ast_ڳنڍ_اظهار* ثابت dstp,
                                       Ast_ڳنڍ_اظهار* srcp) {
        ثابت صحيح dst_ويڪر = dstp->dtypep()->ويڪر_وهڪرو();
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> termps;
        collectFixedAggregateTerms(dstp, termps, خطأ);
        صحيح src_ويڪر = srcp->ويڪر();
        إذا (src_ويڪر < dst_ويڪر) {
            AstExtend* ثابت extendp = جديد AstExtend{srcp->fileline(), srcp, dst_ويڪر};
            srcp = جديد Ast_سرڪايو_L{
                extendp->fileline(), extendp,
                جديد AstConst{extendp->fileline(), static_cast<uint32_t>(dst_ويڪر - src_ويڪر)},
                dst_ويڪر};
            src_ويڪر = dst_ويڪر;
        }
        Ast_ڳنڍ_سونپيو* newp = خالي_اشارو;
        صحيح وٿي = 0;
        لكل (سائيز_t i = 0; i < termps.سائيز(); ++i) {
            Ast_ڳنڍ_اظهار* ثابت termp = termps[i];
            ثابت صحيح ويڪر = termp->dtypep()->ويڪر_وهڪرو();
            ثابت صحيح lsb = src_ويڪر - وٿي - ويڪر;
            وٿي += ويڪر;
            Ast_ڳنڍ_اظهار* rhsp
                = جديد AstSel{srcp->fileline(), srcp->clone_وڻ_خالص(خطأ), lsb, ويڪر};
            إذا (termp->dtypep()->skipRefp()->isDouble()) {
                rhsp = جديد Ast_بٽ_منزل_Real_D{rhsp->fileline(), rhsp};
            }
            Ast_ڳنڍ_سونپيو* ثابت assignp = nodep->cloneType(termp, rhsp);
            assignp->dtypeFrom(termp);
            newp = Ast_ڳنڍ::جوڙيو_ايندڙ(newp, assignp);
        }
        nodep->جوڙيو_ايندڙ_هتي(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(srcp), srcp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
    }

    منطقي عمليو_Const(ثابت Ast_ڳنڍ* nodep) { أرجع VN_IS(nodep, Const); }
    منطقي عمليو_Asv_Const(ثابت Ast_ڳنڍ* nodep) {
        // BIASV(CONST, BIASV(CONST,...)) -> BIASV( BIASV_CONSTED(a,b), ...)
        ثابت Ast_ڳنڍ_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, ڳنڍ_Bi_Com_Asv);
        إذا (!bnodep) أرجع خطأ;
        إذا (!VN_IS(bnodep->lhsp(), Const)) أرجع خطأ;
        ثابت Ast_ڳنڍ_Bi_Com_Asv* ثابت rnodep = VN_CAST(bnodep->rhsp(), ڳنڍ_Bi_Com_Asv);
        إذا (!rnodep) أرجع خطأ;
        إذا (rnodep->type() != bnodep->type()) أرجع خطأ;
        إذا (rnodep->ويڪر() != bnodep->ويڪر()) أرجع خطأ;
        إذا (rnodep->lhsp()->ويڪر() != bnodep->lhsp()->ويڪر()) أرجع خطأ;
        إذا (!VN_IS(rnodep->lhsp(), Const)) أرجع خطأ;
        أرجع سچ;
    }
    منطقي عمليو_Asv_Same(ثابت Ast_ڳنڍ* nodep) {
        // BIASV(SAMEa, BIASV(SAMEb,...)) -> BIASV( BIASV(SAMEa,SAMEb), ...)
        ثابت Ast_ڳنڍ_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, ڳنڍ_Bi_Com_Asv);
        إذا (!bnodep) أرجع خطأ;
        ثابت Ast_ڳنڍ_Bi_Com_Asv* ثابت rnodep = VN_CAST(bnodep->rhsp(), ڳنڍ_Bi_Com_Asv);
        إذا (!rnodep) أرجع خطأ;
        إذا (rnodep->type() != bnodep->type()) أرجع خطأ;
        إذا (rnodep->ويڪر() != bnodep->ويڪر()) أرجع خطأ;
        أرجع operandsSame(bnodep->lhsp(), rnodep->lhsp());
    }
    منطقي عمليو_Asv_LUp(ثابت Ast_ڳنڍ* nodep) {
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
        ثابت Ast_ڳنڍ_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, ڳنڍ_Bi_Com_Asv);
        إذا (!bnodep) أرجع خطأ;
        ثابت Ast_ڳنڍ_Bi_Com_Asv* ثابت lnodep = VN_CAST(bnodep->lhsp(), ڳنڍ_Bi_Com_Asv);
        إذا (!lnodep) أرجع خطأ;
        إذا (lnodep->type() != bnodep->type()) أرجع خطأ;
        إذا (lnodep->ويڪر() != bnodep->ويڪر()) أرجع خطأ;
        أرجع VN_IS(lnodep->lhsp(), Const);
    }
    منطقي عمليو_Asv_RUp(ثابت Ast_ڳنڍ* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr)) ?
        ثابت Ast_ڳنڍ_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, ڳنڍ_Bi_Com_Asv);
        إذا (!bnodep) أرجع خطأ;
        ثابت Ast_ڳنڍ_Bi_Com_Asv* ثابت rnodep = VN_CAST(bnodep->rhsp(), ڳنڍ_Bi_Com_Asv);
        إذا (!rnodep) أرجع خطأ;
        إذا (rnodep->type() != bnodep->type()) أرجع خطأ;
        إذا (rnodep->ويڪر() != bnodep->ويڪر()) أرجع خطأ;
        أرجع VN_IS(rnodep->lhsp(), Const);
    }
    ساكن منطقي عمليو_Sub_جوڙيو(ثابت Ast_ڳنڍ* nodep) {
        // SUB( ADD(CONSTx,y), CONSTz) -> ADD(SUB(CONSTx,CONSTz), y)
        ثابت Ast_ڳنڍ_Biop* ثابت np = VN_CAST(nodep, ڳنڍ_Biop);
        ثابت Ast_ڳنڍ_Biop* ثابت lp = VN_CAST(np->lhsp(), ڳنڍ_Biop);
        أرجع (lp && VN_IS(lp->lhsp(), Const) && VN_IS(np->rhsp(), Const)
                && lp->ويڪر() == np->ويڪر());
    }
    منطقي ميلاپ_Redundant_Clean(AstAnd* andp) {
        // Remove And with constant one inserted by V3Clean
        // 1 & (a == b)  -> (IData)(a == b)
        // When bool is casted to int, the value is either 0 or 1
        AstConst* ثابت constp = VN_AS(andp->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isOne(), andp->lhsp(), "TRREEOPC must meet this condition");
        Ast_ڳنڍ_اظهار* ثابت rhsp = andp->rhsp();
        AstCCast* ccastp = خالي_اشارو;
        ثابت خودڪار isEqOrNeq = [](ثابت Ast_ڳنڍ* nodep) -> منطقي {  //
            أرجع VN_IS(nodep, Eq) || VN_IS(nodep, Neq);
        };
        إذا (isEqOrNeq(rhsp)) {
            ccastp = جديد AstCCast{andp->fileline(), rhsp->unlink_Fr_پٺ(), andp};
        } وإلا إذا (AstCCast* ثابت tmpp = VN_CAST(rhsp, CCast)) {
            إذا (isEqOrNeq(tmpp->lhsp())) {
                إذا (tmpp->ويڪر() == andp->ويڪر()) {
                    tmpp->unlink_Fr_پٺ();
                    ccastp = tmpp;
                } وإلا {
                    ccastp = جديد AstCCast{andp->fileline(), tmpp->lhsp()->unlink_Fr_پٺ(), andp};
                }
            }
        }
        إذا (ccastp) {
            andp->replaceWithKeepDType(ccastp);
            VL_DO_DANGLING(ڌڪو_Deletep(andp), andp);
            أرجع سچ;
        }
        أرجع خطأ;
    }

    ساكن منطقي عمليو_And_Or_Same(ثابت Ast_ڳنڍ* nodep) {
        // OR( AND(VAL,x), AND(VAL,y)) -> AND(VAL,OR(x,y))
        // OR( AND(x,VAL), AND(y,VAL)) -> AND(OR(x,y),VAL)
        ثابت Ast_ڳنڍ_Biop* ثابت np = VN_CAST(nodep, ڳنڍ_Biop);
        ثابت Ast_ڳنڍ_Biop* ثابت lp = VN_CAST(np->lhsp(), ڳنڍ_Biop);
        ثابت Ast_ڳنڍ_Biop* ثابت rp = VN_CAST(np->rhsp(), ڳنڍ_Biop);
        أرجع (lp && rp && lp->ويڪر() == rp->ويڪر() && lp->type() == rp->type()
                && (operandsSame(lp->lhsp(), rp->lhsp()) || operandsSame(lp->rhsp(), rp->rhsp())));
    }
    منطقي ميلاپ_Or_And_Not(Ast_ڳنڍ_Biop* nodep) {
        // AstOr{$a, AstAnd{AstNot{$b}, $c}} if $a.width1, $a==$b => AstOr{$a,$c}
        // Someday we'll sort the biops completely and this can be simplified
        // This often results from our simplified clock generation:
        // if (rst) ... else if (enable)... -> OR(rst,AND(!rst,enable))
        ثابت Ast_ڳنڍ_اظهار* ap;
        Ast_ڳنڍ_Biop* andp;
        إذا (VN_IS(nodep->lhsp(), And)) {
            andp = VN_AS(nodep->lhsp(), And);
            ap = nodep->rhsp();
        } وإلا إذا (VN_IS(nodep->rhsp(), And)) {
            andp = VN_AS(nodep->rhsp(), And);
            ap = nodep->lhsp();
        } وإلا {
            أرجع خطأ;
        }
        ثابت Ast_ڳنڍ_Uniop* notp;
        Ast_ڳنڍ_اظهار* cp;
        إذا (VN_IS(andp->lhsp(), Not)) {
            notp = VN_AS(andp->lhsp(), Not);
            cp = andp->rhsp();
        } وإلا إذا (VN_IS(andp->rhsp(), Not)) {
            notp = VN_AS(andp->rhsp(), Not);
            cp = andp->lhsp();
        } وإلا {
            أرجع خطأ;
        }
        ثابت Ast_ڳنڍ_اظهار* ثابت bp = notp->lhsp();
        إذا (!operandsSame(ap, bp)) أرجع خطأ;
        // Do it
        cp->unlink_Fr_پٺ();
        VL_DO_DANGLING(ڌڪو_Deletep(andp->unlink_Fr_پٺ()), andp);
        VL_DANGLING(notp);
        // Replace whichever branch is now dangling
        إذا (nodep->rhsp()) {
            nodep->lhsp(cp);
        } وإلا {
            nodep->rhsp(cp);
        }
        أرجع سچ;
    }
    منطقي ميلاپ_And_Cond(AstAnd* nodep) {
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
            condp->fileline(), condp->condp()->unlink_Fr_پٺ(),
            جديد AstAnd{nodep->fileline(), maskp->clone_وڻ(خطأ), condp->thenp()->unlink_Fr_پٺ()},
            جديد AstAnd{nodep->fileline(), maskp->clone_وڻ(خطأ),
                       condp->elsep()->unlink_Fr_پٺ()}};
        newp->thenp()->dtypeFrom(nodep);  // As And might have been to change widths
        newp->elsep()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        أرجع سچ;
    }

    منطقي ميلاپ_Cond_Cond(ثابت AstCond* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        إذا (VN_IS(nodep->backp(), Cond)) أرجع خطأ;  // Was checked when visited parent
        إذا (!VN_IS(nodep->thenp(), Cond) && !VN_IS(nodep->elsep(), Cond))
            أرجع خطأ;  // Short circuit
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> truesp;
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> falsesp;
        ميلاپ_Cond_Cond_Recurse(nodep, truesp /*ref*/, falsesp /*ref*/);
        أرجع خطأ;  // Can optimize further
    }
    فراغ ميلاپ_Cond_Cond_Recurse(ثابت AstCond* nodep, std::ويڪٽر<Ast_ڳنڍ_اظهار*>& truesp,
                              std::ويڪٽر<Ast_ڳنڍ_اظهار*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        إذا (truesp.سائيز() > 4 || falsesp.سائيز() > 4) أرجع;
        إذا (!nodep->condp()->is_خالص()) أرجع;
        منطقي replaced = خطأ;
        لكل (ثابت Ast_ڳنڍ_اظهار* condp : truesp) {
            إذا (replaced) اكسر;
            إذا (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_انگ(nodep->condp(), 1);
            replaced = سچ;
            ++m_stat_Cond_اظهار_Redundant;
        }
        لكل (ثابت Ast_ڳنڍ_اظهار* condp : falsesp) {
            إذا (replaced) اكسر;
            إذا (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = سچ;
            ++m_stat_Cond_اظهار_Redundant;
        }
        إذا (ثابت AstCond* subCondp = VN_CAST(nodep->thenp(), Cond)) {
            إذا (!replaced) truesp.emplace_پٺ(nodep->condp());
            ميلاپ_Cond_Cond_Recurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            إذا (!replaced) truesp.ڪڍو_پٺ();
        }
        إذا (ثابت AstCond* subCondp = VN_CAST(nodep->elsep(), Cond)) {
            إذا (!replaced) falsesp.emplace_پٺ(nodep->condp());
            ميلاپ_Cond_Cond_Recurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            إذا (!replaced) falsesp.ڪڍو_پٺ();
        }
    }
    فراغ ميلاپ_If_Cond_Cond(ثابت Ast_ڳنڍ_If* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        إذا (VN_IS(nodep->backp(), If)) أرجع;  // Was checked when visited parent
        إذا (!VN_IS(nodep->thensp(), If) && !VN_IS(nodep->elsesp(), If)) أرجع;  // Short circuit
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> truesp;
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> falsesp;
        ميلاپ_If_Cond_Cond_Recurse(nodep, truesp /*ref*/, falsesp /*ref*/);
    }
    فراغ ميلاپ_If_Cond_Cond_Recurse(ثابت Ast_ڳنڍ_If* nodep, std::ويڪٽر<Ast_ڳنڍ_اظهار*>& truesp,
                                std::ويڪٽر<Ast_ڳنڍ_اظهار*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        إذا (truesp.سائيز() > 4 || falsesp.سائيز() > 4) أرجع;
        إذا (!nodep->condp()->is_خالص()) أرجع;
        // A constant condition is folded by visit(AstNodeIf), once statements can be deleted
        إذا (VN_IS(nodep->condp(), Const)) أرجع;
        منطقي replaced = خطأ;
        لكل (ثابت Ast_ڳنڍ_اظهار* condp : truesp) {
            إذا (replaced) اكسر;
            إذا (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_انگ(nodep->condp(), 1);
            replaced = سچ;
            ++m_stat_If_Cond_اظهار_Redundant;
        }
        لكل (ثابت Ast_ڳنڍ_اظهار* condp : falsesp) {
            إذا (replaced) اكسر;
            إذا (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = سچ;
            ++m_stat_If_Cond_اظهار_Redundant;
        }
        // We only check the first statement of parent IF is an If
        // So we don't need to check for effects in the executing thensp/elsesp
        // altering the child't condition.  e.g. 'if (x) begin x=1; if (x) end'
        إذا (ثابت Ast_ڳنڍ_If* subIfp = VN_CAST(nodep->thensp(), ڳنڍ_If)) {
            إذا (!replaced) truesp.emplace_پٺ(nodep->condp());
            ميلاپ_If_Cond_Cond_Recurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            إذا (!replaced) truesp.ڪڍو_پٺ();
        }
        إذا (ثابت Ast_ڳنڍ_If* subIfp = VN_CAST(nodep->elsesp(), ڳنڍ_If)) {
            إذا (!replaced) falsesp.emplace_پٺ(nodep->condp());
            ميلاپ_If_Cond_Cond_Recurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            إذا (!replaced) falsesp.ڪڍو_پٺ();
        }
    }

    فراغ ميلاپ_If_Same_آخري(Ast_ڳنڍ_If* nodep) {
        إذا (!nodep->thensp()) أرجع;
        إذا (!nodep->elsesp()) أرجع;
        إذا (!canDeleteStatements()) أرجع;
        // Iterate bodies in reverse
        Ast_ڳنڍ* tp = nodep->thensp()->lastp();
        Ast_ڳنڍ* ep = nodep->elsesp()->lastp();
        طالما (tp && ep && tp->same_وڻ(ep)) {
            UASSERT_OBJ(!tp->nextp(), tp, "Expected no nextp");
            UASSERT_OBJ(!ep->nextp(), ep, "Expected no nextp");
            Ast_ڳنڍ* ثابت tPrevp = tp->prevp();
            Ast_ڳنڍ* ثابت ePrevp = ep->prevp();
            nodep->جوڙيو_ايندڙ_هتي(tp->unlink_Fr_پٺ());
            VL_DO_DANGLING(ڌڪو_Deletep(ep->unlink_Fr_پٺ()), ep);
            tp = tPrevp;
            ep = ePrevp;
        }
    }

    منطقي ميلاپ_Masked_Or(AstAnd* nodep) {
        // Masking an OR with terms that have no bits set under the mask is replaced with masking
        // only the remaining terms. Canonical example as generated by V3Expand is:
        // 0xff & (a << 8 | b >> 24) --> 0xff & (b >> 24)

        // Compute how many significant bits are in the mask
        ثابت AstConst* ثابت constp = VN_AS(nodep->lhsp(), Const);
        ثابت uint32_t significant_بٽ = constp->انگ().ويڪر_منزل_Fit();

        AstOr* ثابت orp = VN_AS(nodep->rhsp(), Or);

        // Predicate for checking whether the bottom 'significantBits' bits of the given expression
        // are all zeroes.
        ثابت خودڪار checkBottomClear = [=](ثابت Ast_ڳنڍ* nodep) -> منطقي {
            إذا (ثابت Ast_سرڪايو_L* ثابت shiftp = VN_CAST(nodep, سرڪايو_L)) {
                إذا (ثابت AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                    أرجع scp->انگ().منزل_UInt() >= significant_بٽ;
                }
            }
            أرجع خطأ;
        };

        ثابت منطقي orLIsRedundant = checkBottomClear(orp->lhsp());
        ثابت منطقي orRIsRedundant = checkBottomClear(orp->rhsp());

        إذا (orLIsRedundant && orRIsRedundant) {
            nodep->replaceWithKeepDType(
                جديد AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع سچ;
        } وإلا إذا (orLIsRedundant) {
            orp->replaceWithKeepDType(orp->rhsp()->unlink_Fr_پٺ());
            VL_DO_DANGLING(ڌڪو_Deletep(orp), orp);
            أرجع خطأ;  // input node is still valid, keep going
        } وإلا إذا (orRIsRedundant) {
            orp->replaceWithKeepDType(orp->lhsp()->unlink_Fr_پٺ());
            VL_DO_DANGLING(ڌڪو_Deletep(orp), orp);
            أرجع خطأ;  // input node is still valid, keep going
        } وإلا {
            أرجع خطأ;
        }
    }
    منطقي ميلاپ_Masked_سرڪايو(AstAnd* nodep) {
        // Drop redundant masking of right shift result. E.g: 0xff & ((uint32_t)a >> 24). This
        // commonly appears after V3Expand and the simplification in matchMaskedOr. Similarly,
        // drop redundant masking of left shift result. E.g.: 0xff000000 & ((uint32_t)a << 24).

        ثابت خودڪار check_نقاب = [nodep, هذا](ثابت V3_عدد& نقاب) -> منطقي {
            ثابت AstConst* ثابت constp = VN_AS(nodep->lhsp(), Const);
            إذا (constp->انگ().isCaseEq(نقاب)) {
                Ast_ڳنڍ* ثابت rhsp = nodep->rhsp();
                rhsp->unlink_Fr_پٺ();
                nodep->replaceWithKeepDType(rhsp);
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                أرجع سچ;
            }
            أرجع خطأ;
        };

        // Check if masking is redundant
        إذا (ثابت Ast_سرڪايو_R* ثابت shiftp = VN_CAST(nodep->rhsp(), سرڪايو_R)) {
            إذا (ثابت AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_عدد نقاب_Lo{nodep, nodep->ويڪر()};
                نقاب_Lo.سيٽ_نقاب(nodep->ويڪر() - scp->انگ().منزل_UInt());
                أرجع check_نقاب(نقاب_Lo);
            }
        } وإلا إذا (ثابت Ast_سرڪايو_L* ثابت shiftp = VN_CAST(nodep->rhsp(), سرڪايو_L)) {
            إذا (ثابت AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_عدد نقاب{nodep, nodep->ويڪر()};
                ثابت uint32_t سرڪايو_Amount = scp->انگ().منزل_UInt();
                نقاب.سيٽ_نقاب(nodep->ويڪر() - سرڪايو_Amount, سرڪايو_Amount);
                أرجع check_نقاب(نقاب);
            }
        }
        أرجع خطأ;
    }

    منطقي ميلاپ_Masked_Zero(ثابت AstAnd* nodep) {
        // Turn masking of known zero bits into constant zero. Commonly appears after V3Expand.
        ثابت AstConst* ثابت maskp = VN_AS(nodep->lhsp(), Const);
        ثابت Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp();
        ثابت uint32_t msbP1 = maskp->انگ().most_سيٽ_ٻٽو_انگ_P1();
        إذا (!msbP1) أرجع خطأ;  // Don't rewrite, separate rule matches for this
        ثابت uint32_t msb = msbP1 - 1;
        ثابت uint32_t lsb = maskp->انگ().گهٽ_کان_گهٽ_سيٽ_ٻٽو_انگ_P1() - 1;

        إذا (ثابت Ast_سرڪايو_L* ثابت shiftp = VN_CAST(rhsp, سرڪايو_L)) {
            // 'a << S' forces the low S bits to zero
            إذا (AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                أرجع scp->انگ().سمائي_اندريون_UInt()  //
                       && (scp->انگ().منزل_UInt() > msb);
            }
        }
        إذا (ثابت Ast_سرڪايو_R* ثابت shiftp = VN_CAST(rhsp, سرڪايو_R)) {
            // 'a >> S' forces the high S bits to zero. Check against the width of the shifted
            // operand, V3Expand can create shifts wider than their inputs. Must use width(),
            // not widthMin(), as bits above widthMin() are not guaranteed to be zero.
            إذا (AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                أرجع scp->انگ().سمائي_اندريون_UInt()
                       && (lsb + scp->انگ().منزل_UInt()
                           >= static_cast<uint32_t>(shiftp->lhsp()->ويڪر()));
            }
        }
        إذا (ثابت AstMul* ثابت mulp = VN_CAST(rhsp, Mul)) {
            // 'C * a' forces the low N bits to zero where 'C' has low zero bits
            إذا (AstConst* ثابت cp = VN_CAST(mulp->lhsp(), Const)) {
                أرجع cp->انگ().گهٽ_کان_گهٽ_سيٽ_ٻٽو_انگ_P1() > msb + 1;
            }
        }
        إذا (ثابت AstExtend* ثابت extendp = VN_CAST(rhsp, Extend)) {
            // Zero-extension forces the bits above the source width to zero
            أرجع lsb >= static_cast<uint32_t>(extendp->lhsp()->ويڪر());
        }

        أرجع خطأ;
    }

    منطقي ميلاپ_ٻٽو_انگ_Op_وڻ(Ast_ڳنڍ_اظهار* nodep) {
        إذا (nodep->ويڪر_Min() != 1) أرجع خطأ;
        إذا (!v3Global.opt.f_Const_ٻٽو_انگ_Op_وڻ()) أرجع خطأ;

        ثابت صحيح ويڪر = nodep->ويڪر();
        Ast_ڳنڍ_اظهار* rootp = nodep;
        بنانشان externalOps = 0;
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

        std::string debug_اڳياڙي;
        إذا (debug() >= 9) {  // LCOV_EXCL_START
            ساكن صحيح s_c = 0;
            debug_اڳياڙي = "-  matchBitOpTree[" + std::منزل_string(++s_c) + "] ";
            nodep->dump_وڻ(debug_اڳياڙي + "INPUT: ");
        }  // LCOV_EXCL_STOP

        Ast_ڳنڍ_اظهار* ثابت newp = Const_ٻٽو_انگ_Op_وڻ_Visitor::simplify(rootp, ويڪر, externalOps,
                                                                  m_stat_ٻٽو_انگ_Op_Reduction, m_ids);

        إذا (newp) {
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Transformed leaf of bit tree to " << newp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        }

        إذا (debug() >= 9) {  // LCOV_EXCL_START
            إذا (newp) {
                newp->dump_وڻ(debug_اڳياڙي + "RESULT: ");
            } وإلا {
                ٻاهر << debug_اڳياڙي << "not replaced\n";
            }
        }  // LCOV_EXCL_STOP

        أرجع newp;
    }
    ساكن منطقي عمليو_سرڪايو_Same(ثابت Ast_ڳنڍ* nodep) {
        ثابت Ast_ڳنڍ_Biop* ثابت np = VN_AS(nodep, ڳنڍ_Biop);
        {
            ثابت Ast_سرڪايو_L* ثابت lp = VN_CAST(np->lhsp(), سرڪايو_L);
            ثابت Ast_سرڪايو_L* ثابت rp = VN_CAST(np->rhsp(), سرڪايو_L);
            إذا (lp && rp) {
                أرجع (lp->ويڪر() == rp->ويڪر() && lp->lhsp()->ويڪر() == rp->lhsp()->ويڪر()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        {
            ثابت Ast_سرڪايو_R* ثابت lp = VN_CAST(np->lhsp(), سرڪايو_R);
            ثابت Ast_سرڪايو_R* ثابت rp = VN_CAST(np->rhsp(), سرڪايو_R);
            إذا (lp && rp) {
                أرجع (lp->ويڪر() == rp->ويڪر() && lp->lhsp()->ويڪر() == rp->lhsp()->ويڪر()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        أرجع خطأ;
    }
    منطقي عمليو_Huge_سرڪايو_L(ثابت Ast_ڳنڍ_Biop* nodep) {
        أرجع (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->انگ().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->انگ().سمائي_اندريون_UInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->منزل_UInt()
                        >= static_cast<uint32_t>(nodep->ويڪر())))
                && nodep->lhsp()->is_خالص());
    }
    منطقي عمليو_Huge_سرڪايو_R(ثابت Ast_ڳنڍ_Biop* nodep) {
        أرجع (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->انگ().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->انگ().سمائي_اندريون_UInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->منزل_UInt()
                        >= static_cast<uint32_t>(nodep->lhsp()->ويڪر())))
                && nodep->lhsp()->is_خالص()
                && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), وهڪرو_DType)));
    }
    منطقي عمليو_Is_Two(ثابت Ast_ڳنڍ* nodep) {
        ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
        إذا (!constp) أرجع خطأ;  // not constant
        إذا (constp->انگ().isFourState()) أرجع خطأ;  // four-state
        إذا (nodep->ويڪر() > VL_QUADSIZE) أرجع خطأ;  // too wide
        إذا (nodep->isSigned() && constp->انگ().isNegative()) أرجع خطأ;  // signed and negative
        أرجع constp->منزل_UQuad() == 2;
    }
    منطقي عمليو_Is_Twostate(ثابت Ast_ڳنڍ* nodep) {
        أرجع (VN_IS(nodep, Const) && !VN_AS(nodep, Const)->انگ().isFourState());
    }
    منطقي عمليو_Is_Pow_Two(ثابت Ast_ڳنڍ* nodep) {
        إذا (!عمليو_Is_Twostate(nodep)) أرجع خطأ;
        أرجع (1 == VN_AS(nodep, Const)->انگ().ڳڻپ_Ones());
    }
    منطقي عمليو_سرڪايو_Op(ثابت Ast_ڳنڍ_Biop* nodep) {
        إذا (!VN_IS(nodep->rhsp(), Const)) أرجع خطأ;
        ثابت Ast_ڳنڍ_Biop* ثابت lhsp = VN_CAST(nodep->lhsp(), ڳنڍ_Biop);
        إذا (!lhsp || !(VN_IS(lhsp, And) || VN_IS(lhsp, Or) || VN_IS(lhsp, Xor))) أرجع خطأ;
        إذا (nodep->ويڪر() != lhsp->ويڪر()) أرجع خطأ;
        إذا (nodep->ويڪر() != lhsp->lhsp()->ويڪر()) أرجع خطأ;
        إذا (nodep->ويڪر() != lhsp->rhsp()->ويڪر()) أرجع خطأ;

        // Only do it if we can fruther reduce one of the sides if pulling the shift through:
        // - Either operands are constants
        إذا (VN_IS(lhsp->lhsp(), Const)) أرجع سچ;
        إذا (VN_IS(lhsp->rhsp(), Const)) أرجع سچ;
        // - Const shift on either side
        إذا (Ast_ڳنڍ_Biop* ثابت llp = VN_CAST(lhsp->lhsp(), ڳنڍ_Biop)) {
            أرجع (VN_IS(llp, سرڪايو_L) || VN_IS(llp, سرڪايو_R)) && VN_IS(llp->rhsp(), Const);
        }
        إذا (Ast_ڳنڍ_Biop* ثابت lrp = VN_CAST(lhsp->rhsp(), ڳنڍ_Biop)) {
            أرجع (VN_IS(lrp, سرڪايو_L) || VN_IS(lrp, سرڪايو_R)) && VN_IS(lrp->rhsp(), Const);
        }
        // Otherwise we would increase logic size
        أرجع خطأ;
    }
    منطقي عمليو_سرڪايو_سرڪايو(ثابت Ast_ڳنڍ_Biop* nodep) {
        // We could add a AND though.
        ثابت Ast_ڳنڍ_Biop* ثابت lhsp = VN_CAST(nodep->lhsp(), ڳنڍ_Biop);
        إذا (!lhsp || !(VN_IS(lhsp, سرڪايو_L) || VN_IS(lhsp, سرڪايو_R))) أرجع خطأ;
        // We can only get rid of a<<b>>c or a<<b<<c, with constant b & c
        // because bits may be masked in that process, or (b+c) may exceed the word width.
        إذا (!(VN_IS(nodep->rhsp(), Const) && VN_IS(lhsp->rhsp(), Const))) أرجع خطأ;
        إذا (VN_AS(nodep->rhsp(), Const)->انگ().isFourState()
            || VN_AS(lhsp->rhsp(), Const)->انگ().isFourState())
            أرجع خطأ;
        إذا (nodep->ويڪر() != lhsp->ويڪر()) أرجع خطأ;
        إذا (nodep->ويڪر() != lhsp->lhsp()->ويڪر()) أرجع خطأ;
        أرجع سچ;
    }
    منطقي عمليو_لفظ_OOB(ثابت Ast_لفظ_Sel* nodep) {
        // V3Expand may make a arraysel that exceeds the bounds of the array
        // It was an expression, then got constified.  In reality, the WordSel
        // must be wrapped in a Cond, that will be false.
        أرجع (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ڳنڍ_Var_Ref)
                && VN_AS(nodep->fromp(), ڳنڍ_Var_Ref)->access().isReadOnly()
                && (static_cast<صحيح>(VN_AS(nodep->bitp(), Const)->منزل_UInt())
                    >= VN_AS(nodep->fromp(), ڳنڍ_Var_Ref)->varp()->ويڪر_لفظ()));
    }
    منطقي عمليو_Sel_ڀريل(ثابت AstSel* nodep) {
        أرجع (VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0
                && static_cast<صحيح>(nodep->ويڪر_Const()) == nodep->fromp()->ويڪر());
    }
    منطقي عمليو_Sel_Extend(AstSel* nodep) {
        إذا (!m_doV) أرجع خطأ;
        AstExtend* ثابت extendp = VN_CAST(nodep->fromp(), Extend);
        إذا (!extendp) أرجع خطأ;
        AstConst* ثابت lsbp = VN_CAST(nodep->lsbp(), Const);
        إذا (!lsbp) أرجع خطأ;
        ثابت صحيح ويڪر = nodep->ويڪر_Const();
        ثابت صحيح lsb = lsbp->منزل_SInt();
        ثابت صحيح msb = lsb + ويڪر - 1;
        Ast_ڳنڍ_اظهار* ثابت lhsp = extendp->lhsp();
        إذا (!lhsp->is_خالص()) أرجع خطأ;

        // Selecting the entire extended expression, replace with that
        إذا (lsb == 0 && msb == lhsp->ويڪر() - 1) {
            lhsp->unlink_Fr_پٺ();
            nodep->replaceWithKeepDType(lhsp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع سچ;
        }
        // Select entirely in the extended part - replace with zero
        إذا (lsb >= lhsp->ويڪر()) {
            replaceZero(nodep);
            أرجع سچ;
        }
        // Select entirely in the extended expression - replace with select from that
        إذا (msb < lhsp->ويڪر()) {
            lhsp->unlink_Fr_پٺ();
            lsbp->unlink_Fr_پٺ();
            nodep->replaceWithKeepDType(جديد AstSel{nodep->fileline(), lhsp, lsbp, ويڪر});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع سچ;
        }
        // Select straddles both sides, but is just a shorter extend
        إذا (lsb == 0) {
            lhsp->unlink_Fr_پٺ();
            nodep->replaceWithKeepDType(جديد AstExtend{nodep->fileline(), lhsp});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع سچ;
        }

        أرجع خطأ;
    }
    منطقي عمليو_Sel_Bi_هيٺاهون(AstSel* nodep) {
        // SEL(ADD(a,b),(width-1),0) -> ADD(SEL(a),SEL(b))
        // Add or any operation which doesn't care if we discard top bits
        Ast_ڳنڍ_Biop* ثابت bip = VN_CAST(nodep->fromp(), ڳنڍ_Biop);
        إذا (!(m_doV && bip && VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0)) أرجع خطأ;
        UINFOTREE(9, nodep, "", "SEL(BI)-in");
        Ast_ڳنڍ_اظهار* ثابت bilhsp = bip->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت birhsp = bip->rhsp()->unlink_Fr_پٺ();
        bip->lhsp(جديد AstSel{nodep->fileline(), bilhsp, 0, nodep->ويڪر_Const()});
        bip->rhsp(جديد AstSel{nodep->fileline(), birhsp, 0, nodep->ويڪر_Const()});
        UINFOTREE(9, bip, "", "SEL(BI)-ou");
        VL_DO_DANGLING(replaceWChild(nodep, bip), nodep);
        أرجع سچ;
    }
    منطقي عمليو_Sel_سرڪايو_هيٺاهون(AstSel* nodep) {
        // AND({a}, SHIFTR({b}, {c})) is often shorthand in C for Verilog {b}[{c} :+ {a}]
        // becomes thought other optimizations
        // SEL(SHIFTR({a},{b}),{lsb},{width}) -> SEL({a},{lsb+b},{width})
        ثابت Ast_سرڪايو_R* ثابت shiftp = VN_CAST(nodep->fromp(), سرڪايو_R);
        إذا (!(m_doV && shiftp && VN_IS(shiftp->rhsp(), Const) && VN_IS(nodep->lsbp(), Const))) {
            أرجع خطأ;
        }
        Ast_ڳنڍ_اظهار* ثابت ap = shiftp->lhsp();
        ثابت AstConst* ثابت bp = VN_AS(shiftp->rhsp(), Const);
        ثابت AstConst* ثابت lp = VN_AS(nodep->lsbp(), Const);
        إذا (bp->isWide() || bp->انگ().isFourState() || bp->انگ().isNegative() || lp->isWide()
            || lp->انگ().isFourState() || lp->انگ().isNegative()) {
            أرجع خطأ;
        }
        ثابت صحيح newLsb = lp->منزل_SInt() + bp->منزل_SInt();
        إذا (newLsb + nodep->ويڪر_Const() > ap->ويڪر()) أرجع خطأ;
        //
        UINFO(9, "SEL(SHIFTR(a,b),l,w) -> SEL(a,l+b,w)");
        UINFOTREE(9, nodep, "", "SEL(SH)-in");
        AstSel* ثابت newp
            = جديد AstSel{nodep->fileline(), ap->unlink_Fr_پٺ(), newLsb, nodep->ويڪر_Const()};
        nodep->replaceWithKeepDType(newp);
        UINFOTREE(9, newp, "", "SEL(SH)-ou");
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        أرجع سچ;
    }

    منطقي عمليو_Bi_Extend_Const_Shrink(Ast_ڳنڍ_Biop* nodep) {
        // Loop unrolling favors standalone compares
        // EQ(const{width32}, EXTEND(xx{width3})) -> EQ(const{3}, xx{3})
        // The constant must have zero bits (+ 1 if signed) or compare
        // would be incorrect. See also operandBiExtendConst
        AstExtend* ثابت extendp = VN_CAST(nodep->rhsp(), Extend);
        إذا (!extendp) أرجع خطأ;
        Ast_ڳنڍ_اظهار* ثابت smallerp = extendp->lhsp();
        ثابت صحيح subsize = smallerp->ويڪر();
        AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
        إذا (!constp) أرجع خطأ;
        إذا (!constp->انگ().is_بٽ_Zero(constp->ويڪر() - 1, subsize)) أرجع خطأ;
        //
        UINFOTREE(9, nodep, "", "BI(EXTEND)-in");
        smallerp->unlink_Fr_پٺ();
        VL_DO_DANGLING(ڌڪو_Deletep(extendp->unlink_Fr_پٺ()), extendp);  // aka nodep->lhsp.
        nodep->rhsp(smallerp);

        constp->unlink_Fr_پٺ();
        ثابت V3_عدد انگ{constp, subsize, constp->انگ()};
        nodep->lhsp(جديد AstConst{constp->fileline(), انگ});
        VL_DO_DANGLING(ڌڪو_Deletep(constp), constp);
        أرجع خطأ;  // input node is still valid, keep going
    }
    منطقي عمليو_Bi_Extend_Const_Over(ثابت Ast_ڳنڍ_Biop* nodep) {
        // EQ(const{width32}, EXTEND(xx{width3})) -> constant
        // When the constant has non-zero bits above the extend it's a constant.
        // Avoids compiler warning
        ثابت AstExtend* ثابت extendp = VN_CAST(nodep->rhsp(), Extend);
        إذا (!extendp) أرجع خطأ;
        ثابت Ast_ڳنڍ* ثابت smallerp = extendp->lhsp();
        ثابت صحيح subsize = smallerp->ويڪر();
        ثابت AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
        إذا (!constp) أرجع خطأ;
        إذا (constp->انگ().is_بٽ_Zero(constp->ويڪر() - 1, subsize)) أرجع خطأ;
        أرجع سچ;
    }

    // Extraction checks
    منطقي warnSelect(AstSel* nodep) {
        إذا (m_doGenerate) {
            // Never checked yet
            V3_ويڪر::ويڪر_Params_Edit(nodep);
            iterateChildren(nodep);  // May need "constifying"
        }
        // Find range of dtype we are selecting from
        // Similar code in V3Unknown::AstSel
        ثابت منطقي doit = سچ;
        إذا (m_warn && VN_IS(nodep->lsbp(), Const) && doit) {
            ثابت صحيح max_Decl_ٻٽو_انگ = nodep->declRange().hiMaxSelect() * nodep->decl_El_ويڪر()
                                   + (nodep->decl_El_ويڪر() - 1);
            إذا (VN_AS(nodep->lsbp(), Const)->انگ().isFourState()) {
                nodep->v3error("Selection index is constantly unknown or tristated: "
                               "lsb="
                               << nodep->lsbp()->name() << " width=" << nodep->ويڪر_Const());
                // Replacing nodep will make a mess above, so we replace the offender
                replaceZero(nodep->lsbp());
            } وإلا إذا (nodep->declRange().ranged()
                       && (nodep->msbConst() > max_Decl_ٻٽو_انگ || nodep->lsbConst() > max_Decl_ٻٽو_انگ)) {
                // See also warning in V3Width
                // Must adjust by element width as declRange() is in number of elements
                string msbLsbProtected;
                إذا (nodep->decl_El_ويڪر() == 0) {
                    msbLsbProtected = "(nodep->declElWidth() == 0) "
                                      + std::منزل_string(nodep->msbConst()) + ":"
                                      + std::منزل_string(nodep->lsbConst());
                } وإلا {
                    msbLsbProtected = std::منزل_string(nodep->msbConst() / nodep->decl_El_ويڪر())
                                      + ":"
                                      + std::منزل_string(nodep->lsbConst() / nodep->decl_El_ويڪر());
                }
                nodep->v3warn(SELRANGE,
                              "Selection index out of range: "
                                  << msbLsbProtected << " outside "
                                  << nodep->declRange().hiMaxSelect() << ":0"
                                  << (nodep->declRange().lo() >= 0
                                          ? ""
                                          : (" (adjusted +" + cvt_منزل_Str(-nodep->declRange().lo())
                                             + " to account for negative lsb)")));
                UINFO(1, "    Related Raw index is " << nodep->msbConst() << ":"
                                                     << nodep->lsbConst());
                // Don't replace with zero, we'll do it later
            }
        }
        أرجع خطأ;  // Not a transform, so NOP
    }

    ساكن منطقي operandsSame(ثابت Ast_ڳنڍ* node1p, ثابت Ast_ڳنڍ* node2p) {
        // For now we just detect constants & simple vars, though it could be more generic
        إذا (ثابت AstConst* ثابت const1p = VN_CAST(node1p, Const)) {
            إذا (ثابت AstConst* ثابت const2p = VN_CAST(node2p, Const)) {
                أرجع V3_عدد{node1p->fileline(), 1, 0}
                    .opEq(const1p->انگ(), const2p->انگ())
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
                إذا (VN_IS(and1p->lhsp(), Const) && VN_IS(and1p->rhsp(), ڳنڍ_Var_Ref)
                    && VN_IS(and2p->lhsp(), Const) && VN_IS(and2p->rhsp(), ڳنڍ_Var_Ref))
                    أرجع node1p->same_Gate_وڻ(node2p);
            }
        }
        أرجع خطأ;
    }
    منطقي if_Same_سونپيو(ثابت Ast_ڳنڍ_If* nodep) {
        ثابت Ast_ڳنڍ_سونپيو* ثابت thensp = VN_CAST(nodep->thensp(), ڳنڍ_سونپيو);
        ثابت Ast_ڳنڍ_سونپيو* ثابت elsesp = VN_CAST(nodep->elsesp(), ڳنڍ_سونپيو);
        إذا (!thensp || thensp->nextp()) أرجع خطأ;  // Must be SINGLE statement
        إذا (!elsesp || elsesp->nextp()) أرجع خطأ;
        إذا (thensp->type() != elsesp->type()) أرجع خطأ;  // Can't mix an assigndly with assign
        إذا (!thensp->lhsp()->same_Gate_وڻ(elsesp->lhsp())) أرجع خطأ;
        إذا (!thensp->rhsp()->gate_وڻ()) أرجع خطأ;
        إذا (!elsesp->rhsp()->gate_وڻ()) أرجع خطأ;
        // Must not create an expression with unpacked array type
        إذا (VN_IS(thensp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) أرجع خطأ;
        إذا (VN_IS(elsesp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) أرجع خطأ;
        إذا (m_underRecFunc) أرجع خطأ;  // This optimization may lead to infinite recursion
        إذا (m_doCpp) {  // Do not combine ifs cretaed by V3Premit
            إذا (VN_IS(thensp->lhsp(), VarRef) && thensp->lhsp()->isWide()) أرجع خطأ;
        }
        // Only do it if not calls and both pure, otherwise undoes V3LiftExpr
        أرجع !VN_IS(thensp->rhsp(), ڳنڍ_FTask_Ref)  //
               && !VN_IS(elsesp->rhsp(), ڳنڍ_FTask_Ref)  //
               && thensp->rhsp()->is_خالص()  //
               && elsesp->rhsp()->is_خالص();
    }
    منطقي عمليو_If_If(ثابت Ast_ڳنڍ_If* nodep) {
        إذا (nodep->elsesp()) أرجع خطأ;
        ثابت Ast_ڳنڍ_If* ثابت هيٺاهون_Ifp = VN_CAST(nodep->thensp(), ڳنڍ_If);
        إذا (!هيٺاهون_Ifp || هيٺاهون_Ifp->nextp()) أرجع خطأ;
        إذا (nodep->type() != هيٺاهون_Ifp->type()) أرجع خطأ;
        إذا (Ast_ڳنڍ::afterCommentp(هيٺاهون_Ifp->elsesp())) أرجع خطأ;
        أرجع سچ;
    }
    منطقي ifConcatMergeableBiop(ثابت Ast_ڳنڍ* nodep) {
        أرجع (VN_IS(nodep, And) || VN_IS(nodep, Or) || VN_IS(nodep, Xor));
    }
    منطقي ifAdjacentSel(ثابت AstSel* lhsp, ثابت AstSel* rhsp) {
        إذا (!v3Global.opt.fAssemble()) أرجع خطأ;  // opt disabled
        إذا (!lhsp || !rhsp) أرجع خطأ;
        ثابت Ast_ڳنڍ* ثابت lfromp = lhsp->fromp();
        ثابت Ast_ڳنڍ* ثابت rfromp = rhsp->fromp();
        إذا (!lfromp || !rfromp || !lfromp->same_Gate_وڻ(rfromp)) أرجع خطأ;
        ثابت AstConst* ثابت lstart = VN_CAST(lhsp->lsbp(), Const);
        ثابت AstConst* ثابت rstart = VN_CAST(rhsp->lsbp(), Const);
        إذا (!lstart || !rstart) أرجع خطأ;  // too complicated
        ثابت صحيح rend = (rstart->منزل_SInt() + rhsp->ويڪر_Const());
        أرجع (rend == lstart->منزل_SInt());
    }
    منطقي if_ملايو_Adjacent(ثابت Ast_ڳنڍ_اظهار* lhsp, ثابت Ast_ڳنڍ_اظهار* rhsp) {
        // Called by concatmergeable to determine if {lhsp, rhsp} make sense
        إذا (!v3Global.opt.fAssemble()) أرجع خطأ;  // opt disabled
        // Two same varref
        إذا (operandsSame(lhsp, rhsp)) أرجع سچ;
        ثابت AstSel* ثابت lselp = VN_CAST(lhsp, Sel);
        ثابت AstSel* ثابت rselp = VN_CAST(rhsp, Sel);
        إذا (!lselp && !rselp) أرجع خطأ;
        إذا (lselp && !VN_IS(lselp->lsbp(), Const)) أرجع خطأ;
        إذا (rselp && !VN_IS(rselp->lsbp(), Const)) أرجع خطأ;
        // a[i:j] {a[j-1:k], b}
        إذا (lselp && !rselp && VN_IS(rhsp, Concat))
            أرجع if_ملايو_Adjacent(lhsp, VN_AS(rhsp, Concat)->lhsp());
        // {b, a[j:k]} a[k-1:i]
        إذا (rselp && !lselp && VN_IS(lhsp, Concat))
            أرجع if_ملايو_Adjacent(VN_AS(lhsp, Concat)->rhsp(), rhsp);
        // a a[msb:j]
        ثابت Ast_ڳنڍ_اظهار* ثابت lfromp = lselp                                ? lselp->fromp()
                                          : lhsp->same_Gate_وڻ(rselp->fromp()) ? lhsp
                                                                               : خالي_اشارو;
        // a[i:0] a
        ثابت Ast_ڳنڍ_اظهار* ثابت rfromp = rselp                                ? rselp->fromp()
                                          : rhsp->same_Gate_وڻ(lselp->fromp()) ? rhsp
                                                                               : خالي_اشارو;
        إذا (!lfromp || !rfromp || !lfromp->same_Gate_وڻ(rfromp)) أرجع خطأ;

        ثابت int32_t lstart = lselp ? lselp->lsbConst() : 0;
        ثابت int32_t rstart = rselp ? rselp->lsbConst() : 0;
        ثابت int32_t rend = rstart + (rselp ? rselp->ويڪر_Const() : rhsp->ويڪر());
        // a[i:j] a[j-1:k]
        إذا (rend == lstart) أرجع سچ;
        // a[i:0] a[msb:j]
        إذا (rend == rfromp->ويڪر() && lstart == 0) أرجع سچ;
        أرجع خطأ;
    }
    منطقي concatMergeable(ثابت Ast_ڳنڍ_اظهار* lhsp, ثابت Ast_ڳنڍ_اظهار* rhsp, بنانشان اونهائي) {
        // Determine if {a OP b, c OP d} => {a, c} OP {b, d} is advantageous
        إذا (!v3Global.opt.fAssemble()) أرجع خطأ;  // opt disabled
        إذا (lhsp->type() != rhsp->type()) أرجع خطأ;
        إذا (!ifConcatMergeableBiop(lhsp)) أرجع خطأ;
        إذا (اونهائي > CONCAT_MERGABLE_MAX_اونهائي) أرجع خطأ;  // As worse case O(n^2) algorithm

        ثابت Ast_ڳنڍ_Biop* ثابت lp = VN_CAST(lhsp, ڳنڍ_Biop);
        ثابت Ast_ڳنڍ_Biop* ثابت rp = VN_CAST(rhsp, ڳنڍ_Biop);
        إذا (!lp || !rp) أرجع خطأ;
        // {a[]&b[], a[]&b[]}
        ثابت منطقي lad = if_ملايو_Adjacent(lp->lhsp(), rp->lhsp());
        ثابت منطقي rad = if_ملايو_Adjacent(lp->rhsp(), rp->rhsp());
        إذا (lad && rad) أرجع سچ;
        // {a[] & b[]&c[], a[] & b[]&c[]}
        إذا (lad && concatMergeable(lp->rhsp(), rp->rhsp(), اونهائي + 1)) أرجع سچ;
        // {a[]&b[] & c[], a[]&b[] & c[]}
        إذا (rad && concatMergeable(lp->lhsp(), rp->lhsp(), اونهائي + 1)) أرجع سچ;
        // {(a[]&b[])&(c[]&d[]), (a[]&b[])&(c[]&d[])}
        إذا (concatMergeable(lp->lhsp(), rp->lhsp(), اونهائي + 1)
            && concatMergeable(lp->rhsp(), rp->rhsp(), اونهائي + 1)) {
            أرجع سچ;
        }
        أرجع خطأ;
    }
    ساكن منطقي operands_Same_ويڪر(ثابت Ast_ڳنڍ* lhsp, ثابت Ast_ڳنڍ* rhsp) {
        أرجع lhsp->ويڪر() == rhsp->ويڪر();
    }

    //----------------------------------------
    // Constant Replacement functions.
    // These all take a node, delete its tree, and replaces it with a constant

    فراغ replace_انگ(Ast_ڳنڍ* oldp, ثابت V3_عدد& انگ, ثابت string& origParamName = "") {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        UASSERT_OBJ(!(VN_IS(oldp, Const) && !VN_AS(oldp, Const)->انگ().isFourState()), oldp,
                    "Already constant??");
        AstConst* ثابت newp = جديد AstConst{oldp->fileline(), انگ};
        إذا (!origParamName.خالي_جايون()) newp->origParamName(origParamName);
        oldp->replaceWithKeepDType(newp);
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        VL_DO_DANGLING(ڌڪو_Deletep(oldp), oldp);
    }
    فراغ replace_انگ(Ast_ڳنڍ* nodep, uint32_t val) {
        ثابت V3_عدد انگ{nodep, nodep->ويڪر(), val};
        VL_DO_DANGLING(replace_انگ(nodep, انگ), nodep);
    }
    فراغ replace_انگ_Signed(Ast_ڳنڍ_Biop* nodep, uint32_t val) {
        // We allow both sides to be constant, as one may have come from
        // parameter propagation, etc.
        إذا (m_warn && !(VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const))) {
            nodep->v3warn(UNSIGNED, "Comparison is constant due to unsigned arithmetic");
        }
        VL_DO_DANGLING(replace_انگ(nodep, val), nodep);
    }
    فراغ replace_انگ_Limited(Ast_ڳنڍ_Biop* nodep, uint32_t val) {
        // Avoids gcc warning about same
        إذا (m_warn) nodep->v3warn(CMPCONST, "Comparison is constant due to limited range");
        VL_DO_DANGLING(replace_انگ(nodep, val), nodep);
    }
    فراغ replaceZero(Ast_ڳنڍ* nodep) { VL_DO_DANGLING(replace_انگ(nodep, 0), nodep); }
    فراغ replace_Zero_Chk_خالص(Ast_ڳنڍ* nodep, Ast_ڳنڍ_اظهار* checkp) {
        // For example, "0 * n" -> 0 if n has no side effects
        // Else strength reduce it to 0 & n.
        // If ever change the operation note AstAnd rule specially ignores this created pattern
        إذا (checkp->is_خالص()) {
            VL_DO_DANGLING(replace_انگ(nodep, 0), nodep);
        } وإلا {
            Ast_ڳنڍ* ثابت newp = جديد AstAnd{nodep->fileline(), جديد AstConst{nodep->fileline(), 0},
                                             checkp->unlink_Fr_پٺ()};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        }
    }
    فراغ replaceAllOnes(Ast_ڳنڍ* nodep) {
        V3_عدد ones{nodep, nodep->ويڪر(), 0};
        ones.سيٽ_نقاب(nodep->ويڪر());
        VL_DO_DANGLING(replace_انگ(nodep, ones), nodep);
    }
    فراغ replaceConst(Ast_ڳنڍ_Uniop* nodep) {
        V3_عدد numv{nodep, nodep->ويڪر_Min_V()};
        nodep->عدد_هلايو(numv, const_انگ_V(nodep->lhsp()));
        ثابت V3_عدد& انگ = منزل_انگ_C(nodep, numv);
        UINFO(4, "UNICONST -> " << انگ);
        VL_DO_DANGLING(replace_انگ(nodep, انگ), nodep);
    }
    فراغ replaceConst(Ast_ڳنڍ_Biop* nodep) {
        V3_عدد numv{nodep, nodep->ويڪر_Min_V()};
        nodep->عدد_هلايو(numv, const_انگ_V(nodep->lhsp()), const_انگ_V(nodep->rhsp()));
        ثابت V3_عدد& انگ = منزل_انگ_C(nodep, numv);
        UINFO(4, "BICONST -> " << انگ);
        VL_DO_DANGLING(replace_انگ(nodep, انگ), nodep);
    }
    فراغ replaceConst(Ast_ڳنڍ_Triop* nodep) {
        V3_عدد numv{nodep, nodep->ويڪر_Min_V()};
        nodep->عدد_هلايو(numv, const_انگ_V(nodep->lhsp()), const_انگ_V(nodep->rhsp()),
                             const_انگ_V(nodep->thsp()));
        ثابت V3_عدد& انگ = منزل_انگ_C(nodep, numv);
        UINFO(4, "TRICONST -> " << انگ);
        VL_DO_DANGLING(replace_انگ(nodep, انگ), nodep);
    }
    فراغ replaceConst(Ast_ڳنڍ_Quadop* nodep) {
        V3_عدد numv{nodep, nodep->ويڪر_Min_V()};
        nodep->عدد_هلايو(numv, const_انگ_V(nodep->lhsp()), const_انگ_V(nodep->rhsp()),
                             const_انگ_V(nodep->thsp()), const_انگ_V(nodep->fhsp()));
        ثابت V3_عدد& انگ = منزل_انگ_C(nodep, numv);
        UINFO(4, "QUADCONST -> " << انگ);
        VL_DO_DANGLING(replace_انگ(nodep, انگ), nodep);
    }

    فراغ replaceConstString(Ast_ڳنڍ* oldp, ثابت string& انگ) {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        Ast_ڳنڍ* ثابت newp = جديد AstConst{oldp->fileline(), AstConst::String{}, انگ};
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        oldp->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(oldp), oldp);
    }
    //----------------------------------------
    // Replacement functions.
    // These all take a node and replace it with something else

    فراغ replaceWChild(Ast_ڳنڍ* nodep, Ast_ڳنڍ_اظهار* childp) {
        // NODE(..., CHILD(...)) -> CHILD(...)
        childp->unlink_Fr_پٺ_With_ايندڙ();
        // If replacing a SEL for example, the data type comes from the parent (is less wide).
        // This may adversely affect the operation of the node being replaced.
        nodep->replaceWithKeepDType(childp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    فراغ replaceWChildBool(Ast_ڳنڍ* nodep, Ast_ڳنڍ_اظهار* childp) {
        // NODE(..., CHILD(...)) -> REDOR(CHILD(...))
        childp->unlink_Fr_پٺ();
        إذا (childp->width1()) {
            nodep->replaceWithKeepDType(childp);
        } وإلا {
            nodep->replaceWithKeepDType(جديد AstRedOr{childp->fileline(), childp});
        }
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    //! Replace a ternary node with its RHS after iterating
    //! Used with short-circuiting, where the RHS has not yet been iterated.
    فراغ replaceWIteratedRhs(Ast_ڳنڍ_Triop* nodep) {
        إذا (Ast_ڳنڍ* ثابت rhsp = nodep->rhsp()) iterate_And_ايندڙ_Null(rhsp);
        replaceWChild(nodep, nodep->rhsp());  // May have changed
    }

    //! Replace a ternary node with its THS after iterating
    //! Used with short-circuiting, where the THS has not yet been iterated.
    فراغ replaceWIteratedThs(Ast_ڳنڍ_Triop* nodep) {
        إذا (Ast_ڳنڍ* ثابت thsp = nodep->thsp()) iterate_And_ايندڙ_Null(thsp);
        replaceWChild(nodep, nodep->thsp());  // May have changed
    }
    فراغ replaceWLhs(Ast_ڳنڍ_Uniop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    فراغ replaceWLhs(Ast_ڳنڍ_Biop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    فراغ replaceWRhs(Ast_ڳنڍ_Biop* nodep) {
        // Keep RHS, remove LHS
        replaceWChild(nodep, nodep->rhsp());
    }
    فراغ replaceWLhsBool(Ast_ڳنڍ_Biop* nodep) { replaceWChildBool(nodep, nodep->lhsp()); }
    فراغ replaceWRhsBool(Ast_ڳنڍ_Biop* nodep) { replaceWChildBool(nodep, nodep->rhsp()); }
    فراغ replaceAsv(Ast_ڳنڍ_Biop* nodep) {
        // BIASV(CONSTa, BIASV(CONSTb, c)) -> BIASV( BIASV_CONSTED(a,b), c)
        // BIASV(SAMEa,  BIASV(SAMEb, c))  -> BIASV( BIASV(SAMEa,SAMEb), c)
        // UINFOTREE(1, nodep, "", "repAsvConst_old");
        Ast_ڳنڍ_اظهار* ثابت ap = nodep->lhsp();
        Ast_ڳنڍ_Biop* ثابت rp = VN_AS(nodep->rhsp(), ڳنڍ_Biop);
        Ast_ڳنڍ_اظهار* ثابت bp = rp->lhsp();
        Ast_ڳنڍ_اظهار* ثابت cp = rp->rhsp();
        ap->unlink_Fr_پٺ();
        bp->unlink_Fr_پٺ();
        cp->unlink_Fr_پٺ();
        rp->unlink_Fr_پٺ();
        nodep->lhsp(rp);
        nodep->rhsp(cp);
        rp->lhsp(ap);
        rp->rhsp(bp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        إذا (VN_IS(rp->lhsp(), Const) && VN_IS(rp->rhsp(), Const)) replaceConst(rp);
        iterate(nodep);  // Proceed to fixed point
    }
    فراغ replaceAsvLUp(Ast_ڳنڍ_Biop* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r))
        Ast_ڳنڍ_Biop* ثابت lp = VN_AS(nodep->lhsp()->unlink_Fr_پٺ(), ڳنڍ_Biop);
        Ast_ڳنڍ_اظهار* ثابت llp = lp->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت lrp = lp->rhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت rp = nodep->rhsp()->unlink_Fr_پٺ();
        nodep->lhsp(llp);
        nodep->rhsp(lp);
        lp->lhsp(lrp);
        lp->rhsp(rp);
        lp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    فراغ replaceAsvRUp(Ast_ڳنڍ_Biop* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr))
        Ast_ڳنڍ_اظهار* ثابت lp = nodep->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_Biop* ثابت rp = VN_AS(nodep->rhsp()->unlink_Fr_پٺ(), ڳنڍ_Biop);
        Ast_ڳنڍ_اظهار* ثابت rlp = rp->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت rrp = rp->rhsp()->unlink_Fr_پٺ();
        nodep->lhsp(rlp);
        nodep->rhsp(rp);
        rp->lhsp(lp);
        rp->rhsp(rrp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    فراغ replaceAndOr(Ast_ڳنڍ_Biop* nodep) {
        //  OR  (AND (CONSTll,lr), AND(CONSTrl==ll,rr))    -> AND (CONSTll, OR(lr,rr))
        //  OR  (AND (CONSTll,lr), AND(CONSTrl,    rr=lr)) -> AND (OR(ll,rl), rr)
        // nodep ^lp  ^llp   ^lrp  ^rp  ^rlp       ^rrp
        // (Or/And may also be reversed)
        Ast_ڳنڍ_Biop* ثابت lp = VN_AS(nodep->lhsp()->unlink_Fr_پٺ(), ڳنڍ_Biop);
        Ast_ڳنڍ_اظهار* ثابت llp = lp->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت lrp = lp->rhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_Biop* ثابت rp = VN_AS(nodep->rhsp()->unlink_Fr_پٺ(), ڳنڍ_Biop);
        Ast_ڳنڍ_اظهار* ثابت rlp = rp->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت rrp = rp->rhsp()->unlink_Fr_پٺ();
        nodep->replaceWithKeepDType(lp);
        إذا (operandsSame(llp, rlp)) {
            lp->lhsp(llp);
            lp->rhsp(nodep);
            lp->dtypeFrom(nodep);
            nodep->lhsp(lrp);
            nodep->rhsp(rrp);
            VL_DO_DANGLING(ڌڪو_Deletep(rp), rp);
            VL_DO_DANGLING(ڌڪو_Deletep(rlp), rlp);
        } وإلا إذا (operandsSame(lrp, rrp)) {
            lp->lhsp(nodep);
            lp->rhsp(rrp);
            lp->dtypeFrom(nodep);
            nodep->lhsp(llp);
            nodep->rhsp(rlp);
            VL_DO_DANGLING(ڌڪو_Deletep(rp), rp);
            VL_DO_DANGLING(ڌڪو_Deletep(lrp), lrp);
        } وإلا {
            nodep->v3fatalSrc("replaceAndOr on something operandAndOrSame shouldn't have matched");
        }
        // UINFOTREE(1, nodep, "", "repAndOr_new");
    }
    فراغ replace_سرڪايو_Same(Ast_ڳنڍ_Biop* nodep) {
        // Or(Shift(ll,CONSTlr),Shift(rl,CONSTrr==lr)) -> Shift(Or(ll,rl),CONSTlr)
        // (Or/And may also be reversed)
        Ast_ڳنڍ_Biop* ثابت lp = VN_AS(nodep->lhsp()->unlink_Fr_پٺ(), ڳنڍ_Biop);
        Ast_ڳنڍ_اظهار* ثابت llp = lp->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت lrp = lp->rhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_Biop* ثابت rp = VN_AS(nodep->rhsp()->unlink_Fr_پٺ(), ڳنڍ_Biop);
        Ast_ڳنڍ_اظهار* ثابت rlp = rp->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت rrp = rp->rhsp()->unlink_Fr_پٺ();
        nodep->replaceWithKeepDType(lp);
        lp->lhsp(nodep);
        lp->rhsp(lrp);
        nodep->lhsp(llp);
        nodep->rhsp(rlp);
        nodep->dtypep(llp->dtypep());  // dtype of Biop is before shift.
        VL_DO_DANGLING(ڌڪو_Deletep(rp), rp);
        VL_DO_DANGLING(ڌڪو_Deletep(rrp), rrp);
        // UINFOTREE(1, nodep, "", "repShiftSame_new");
    }
    فراغ replaceConcatSel(AstConcat* nodep) {
        // {a[1], a[0]} -> a[1:0]
        AstSel* ثابت lselp = VN_AS(nodep->lhsp()->unlink_Fr_پٺ(), Sel);
        AstSel* ثابت rselp = VN_AS(nodep->rhsp()->unlink_Fr_پٺ(), Sel);
        ثابت صحيح lstart = lselp->lsbConst();
        ثابت صحيح lwidth = lselp->ويڪر_Const();
        ثابت صحيح rstart = rselp->lsbConst();
        ثابت صحيح rwidth = rselp->ويڪر_Const();

        UASSERT_OBJ((rstart + rwidth) == lstart, nodep,
                    "tried to merge two selects which are not adjacent");
        AstSel* ثابت newselp = جديد AstSel{
            lselp->fromp()->fileline(), rselp->fromp()->unlink_Fr_پٺ(), rstart, lwidth + rwidth};
        UINFO(5, "merged two adjacent sel " << lselp << " and " << rselp << " to one " << newselp);

        nodep->replaceWithKeepDType(newselp);
        VL_DO_DANGLING(ڌڪو_Deletep(lselp), lselp);
        VL_DO_DANGLING(ڌڪو_Deletep(rselp), rselp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ replace_Concat_ملايو(AstConcat* nodep) {
        // {llp OP lrp, rlp OP rrp} => {llp, rlp} OP {lrp, rrp}, where OP = AND/OR/XOR
        Ast_ڳنڍ_Biop* ثابت lp = VN_AS(nodep->lhsp(), ڳنڍ_Biop);
        Ast_ڳنڍ_Biop* ثابت rp = VN_AS(nodep->rhsp(), ڳنڍ_Biop);
        إذا (concatMergeable(lp, rp, 0)) {
            Ast_ڳنڍ_اظهار* ثابت llp = lp->lhsp();
            Ast_ڳنڍ_اظهار* ثابت lrp = lp->rhsp();
            Ast_ڳنڍ_اظهار* ثابت rlp = rp->lhsp();
            Ast_ڳنڍ_اظهار* ثابت rrp = rp->rhsp();
            AstConcat* ثابت newlp = جديد AstConcat{rlp->fileline(), llp->clone_وڻ_خالص(خطأ),
                                                   rlp->clone_وڻ_خالص(خطأ)};
            AstConcat* ثابت newrp = جديد AstConcat{rrp->fileline(), lrp->clone_وڻ_خالص(خطأ),
                                                   rrp->clone_وڻ_خالص(خطأ)};
            // use the lhs to replace the parent concat
            llp->replaceWith(newlp);
            VL_DO_DANGLING(ڌڪو_Deletep(llp), llp);
            lrp->replaceWith(newrp);
            VL_DO_DANGLING(ڌڪو_Deletep(lrp), lrp);
            lp->dtype_Chg_ويڪر_Signed(newlp->ويڪر(), newlp->ويڪر(), VSigning::UNSIGNED);
            UINFO(5, "merged " << nodep);
            ++m_stat_Concat_ملايو;
            VL_DO_DANGLING(ڌڪو_Deletep(rp->unlink_Fr_پٺ()), rp);
            nodep->replaceWithKeepDType(lp->unlink_Fr_پٺ());
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            iterate(lp->lhsp());
            iterate(lp->rhsp());
        } وإلا {
            nodep->v3fatalSrc("tried to merge two Concat which are not adjacent");
        }
    }
    فراغ replaceExtend(Ast_ڳنڍ* nodep, Ast_ڳنڍ_اظهار* arg0p) {
        // -> EXTEND(nodep)
        // like a AstExtend{$rhsp}, but we need to set the width correctly from base node
        arg0p->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت newp
            = (VN_IS(nodep, ExtendS)
                   ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstExtendS{nodep->fileline(), arg0p})
                   : static_cast<Ast_ڳنڍ_اظهار*>(جديد AstExtend{nodep->fileline(), arg0p}));
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ replace_Pow_سرڪايو(Ast_ڳنڍ_Biop* nodep) {  // Pow or PowS
        UINFO(5, "POW(2,b)->SHIFTL(1,b) " << nodep);
        Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٺ();
        Ast_سرڪايو_L* ثابت newp
            = جديد Ast_سرڪايو_L{nodep->fileline(), جديد AstConst{nodep->fileline(), 1}, rhsp};
        newp->lhsp()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ replace_Mul_سرڪايو(AstMul* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "MUL(2^n,b)->SHIFTL(b,n) " << nodep);
        ثابت صحيح amount = VN_AS(nodep->lhsp(), Const)->انگ().most_سيٽ_ٻٽو_انگ_P1() - 1;  // 2^n->n+1
        Ast_ڳنڍ_اظهار* ثابت opp = nodep->rhsp()->unlink_Fr_پٺ();
        Ast_سرڪايو_L* ثابت newp
            = جديد Ast_سرڪايو_L{nodep->fileline(), opp, جديد AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ replace_Div_سرڪايو(AstDiv* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "DIV(b,2^n)->SHIFTR(b,n) " << nodep);
        ثابت صحيح amount = VN_AS(nodep->rhsp(), Const)->انگ().most_سيٽ_ٻٽو_انگ_P1() - 1;  // 2^n->n+1
        Ast_ڳنڍ_اظهار* ثابت opp = nodep->lhsp()->unlink_Fr_پٺ();
        Ast_سرڪايو_R* ثابت newp
            = جديد Ast_سرڪايو_R{nodep->fileline(), opp, جديد AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ replaceModAnd(AstModDiv* nodep) {  // Mod, but not ModS as not simple shift
        UINFO(5, "MOD(b,2^n)->AND(b,2^n-1) " << nodep);
        ثابت صحيح amount = VN_AS(nodep->rhsp(), Const)->انگ().most_سيٽ_ٻٽو_انگ_P1() - 1;  // 2^n->n+1
        V3_عدد نقاب{nodep, nodep->ويڪر()};
        نقاب.سيٽ_نقاب(amount);
        Ast_ڳنڍ_اظهار* ثابت opp = nodep->lhsp()->unlink_Fr_پٺ();
        AstAnd* ثابت newp
            = جديد AstAnd{nodep->fileline(), opp, جديد AstConst{nodep->fileline(), نقاب}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ replace_سرڪايو_Op(Ast_ڳنڍ_Biop* nodep) {
        UINFO(5, "SHIFT(AND(a,b),CONST) with a/b special ->AND(SHIFT(a,CONST),SHIFT(b,CONST)) "
                     << nodep);
        ثابت صحيح ويڪر = nodep->ويڪر();
        ثابت صحيح ويڪر_Min = nodep->ويڪر_Min();
        VNRelinker handle;
        nodep->unlink_Fr_پٺ(&handle);
        Ast_ڳنڍ_Biop* ثابت lhsp = VN_AS(nodep->lhsp(), ڳنڍ_Biop);
        lhsp->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت shiftp = nodep->rhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت ap = lhsp->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت bp = lhsp->rhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_Biop* ثابت shift1p = nodep;
        Ast_ڳنڍ_Biop* ثابت shift2p = nodep->clone_وڻ(سچ);
        shift1p->lhsp(ap);
        shift1p->rhsp(shiftp->clone_وڻ_خالص(سچ));
        shift2p->lhsp(bp);
        shift2p->rhsp(shiftp);
        Ast_ڳنڍ_Biop* ثابت newp = lhsp;
        newp->lhsp(shift1p);
        newp->rhsp(shift2p);
        newp->dtype_Chg_ويڪر(ويڪر, ويڪر_Min);  // The new AND must have width of the original SHIFT
        handle.relink(newp);
        iterate(newp);  // Further reduce, either node may have more reductions.
    }
    فراغ replace_سرڪايو_سرڪايو(Ast_ڳنڍ_Biop* nodep) {
        UINFO(4, "SHIFT(SHIFT(a,s1),s2)->SHIFT(a,ADD(s1,s2)) " << nodep);
        UINFOTREE(9, nodep, "", "repShiftShift_old");
        Ast_ڳنڍ_Biop* ثابت lhsp = VN_AS(nodep->lhsp(), ڳنڍ_Biop);
        lhsp->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت ap = lhsp->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت shift1p = lhsp->rhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت shift2p = nodep->rhsp()->unlink_Fr_پٺ();
        // Shift1p and shift2p may have different sizes, both are
        // self-determined so sum with infinite width
        إذا (nodep->type() == lhsp->type()) {
            ثابت صحيح shift1 = VN_AS(shift1p, Const)->منزل_UInt();
            ثابت صحيح shift2 = VN_AS(shift2p, Const)->منزل_UInt();
            ثابت صحيح newshift = shift1 + shift2;
            VL_DO_DANGLING(ڌڪو_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ڌڪو_Deletep(shift2p), shift2p);
            nodep->lhsp(ap);
            nodep->rhsp(جديد AstConst(nodep->fileline(), newshift));
            iterate(nodep);  // Further reduce, either node may have more reductions.
        } وإلا {
            // We know shift amounts are constant, but might be a mixed left/right shift
            صحيح shift1 = VN_AS(shift1p, Const)->منزل_UInt();
            إذا (VN_IS(lhsp, سرڪايو_R)) shift1 = -shift1;
            صحيح shift2 = VN_AS(shift2p, Const)->منزل_UInt();
            إذا (VN_IS(nodep, سرڪايو_R)) shift2 = -shift2;
            ثابت صحيح newshift = shift1 + shift2;
            VL_DO_DANGLING(ڌڪو_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ڌڪو_Deletep(shift2p), shift2p);
            Ast_ڳنڍ_اظهار* newp;
            V3_عدد mask1{nodep, nodep->ويڪر()};
            V3_عدد ones{nodep, nodep->ويڪر()};
            ones.سيٽ_نقاب(nodep->ويڪر());
            إذا (shift1 < 0) {
                mask1.op_سرڪايو_R(ones, V3_عدد(nodep, VL_IDATASIZE, -shift1));
            } وإلا {
                mask1.op_سرڪايو_L(ones, V3_عدد(nodep, VL_IDATASIZE, shift1));
            }
            V3_عدد نقاب{nodep, nodep->ويڪر()};
            إذا (shift2 < 0) {
                نقاب.op_سرڪايو_R(mask1, V3_عدد(nodep, VL_IDATASIZE, -shift2));
            } وإلا {
                نقاب.op_سرڪايو_L(mask1, V3_عدد(nodep, VL_IDATASIZE, shift2));
            }
            إذا (newshift < 0) {
                newp = جديد Ast_سرڪايو_R{nodep->fileline(), ap,
                                     جديد AstConst(nodep->fileline(), -newshift)};
            } وإلا {
                newp = جديد Ast_سرڪايو_L{nodep->fileline(), ap,
                                     جديد AstConst(nodep->fileline(), newshift)};
            }
            newp->dtypeFrom(nodep);
            newp = جديد AstAnd{nodep->fileline(), newp, جديد AstConst{nodep->fileline(), نقاب}};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            // UINFOTREE(1, newp, "", "repShiftShift_new");
            iterate(newp);  // Further reduce, either node may have more reductions.
        }
        VL_DO_DANGLING(ڌڪو_Deletep(lhsp), lhsp);
    }

    منطقي replace_سونپيو_Multi_Sel(Ast_ڳنڍ_سونپيو* nodep) {
        // Multiple assignments to sequential bits can be concated
        // ASSIGN(SEL(a),aq), ASSIGN(SEL(a+1),bq) -> ASSIGN(SEL(a:b),CONCAT(aq,bq)
        // ie. assign var[2]=a, assign var[3]=b -> assign var[3:2]={b,a}

        // Skip if we're not const'ing an entire module (IE doing only one assign, etc)
        إذا (!m_modp) أرجع خطأ;
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ثابت sel1p = VN_CAST(nodep->lhsp(), Sel);
        إذا (!sel1p) أرجع خطأ;
        Ast_ڳنڍ_سونپيو* ثابت nextp = VN_CAST(nodep->nextp(), ڳنڍ_سونپيو);
        إذا (!nextp) أرجع خطأ;
        إذا (nodep->type() != nextp->type()) أرجع خطأ;
        ثابت AstSel* ثابت sel2p = VN_CAST(nextp->lhsp(), Sel);
        إذا (!sel2p) أرجع خطأ;
        AstVarRef* ثابت varref1p = VN_CAST(sel1p->fromp(), VarRef);
        إذا (!varref1p) أرجع خطأ;
        ثابت AstVarRef* ثابت varref2p = VN_CAST(sel2p->fromp(), VarRef);
        إذا (!varref2p) أرجع خطأ;
        إذا (!varref1p->same_Gate_وڻ(varref2p)) أرجع خطأ;
        ثابت AstConst* ثابت con1p = VN_CAST(sel1p->lsbp(), Const);
        إذا (!con1p) أرجع خطأ;
        ثابت AstConst* ثابت con2p = VN_CAST(sel2p->lsbp(), Const);
        إذا (!con2p) أرجع خطأ;
        // We need to make sure there's no self-references involved in either
        // assignment.  For speed, we only look 3 deep, then give up.
        إذا (!varNotReferenced(nodep->rhsp(), varref1p->varp())) أرجع خطأ;
        إذا (!varNotReferenced(nextp->rhsp(), varref2p->varp())) أرجع خطأ;
        // If a variable is marked split_var, access to the variable should not be merged.
        إذا (varref1p->varp()->attr_ورهايو_Var() || varref2p->varp()->attr_ورهايو_Var()) أرجع خطأ;
        // Swap?
        إذا ((con1p->منزل_SInt() != con2p->منزل_SInt() + sel2p->ويڪر())
            && (con2p->منزل_SInt() != con1p->منزل_SInt() + sel1p->ويڪر())) {
            أرجع خطأ;
        }
        ثابت منطقي lsb_پهريون_سونپيو = (con1p->منزل_UInt() < con2p->منزل_UInt());
        UINFO(4, "replaceAssignMultiSel " << nodep);
        UINFO(4, "                   && " << nextp);
        // UINFOTREE(1, nodep, "", "comb1");
        // UINFOTREE(1, nextp, "", "comb2");
        Ast_ڳنڍ_اظهار* ثابت rhs1p = nodep->rhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت rhs2p = nextp->rhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_سونپيو* newp;
        إذا (lsb_پهريون_سونپيو) {
            newp = nodep->cloneType(جديد AstSel{sel1p->fileline(), varref1p->unlink_Fr_پٺ(),
                                               sel1p->lsbConst(), sel1p->ويڪر() + sel2p->ويڪر()},
                                    جديد AstConcat{rhs1p->fileline(), rhs2p, rhs1p});
        } وإلا {
            newp = nodep->cloneType(جديد AstSel{sel1p->fileline(), varref1p->unlink_Fr_پٺ(),
                                               sel2p->lsbConst(), sel1p->ويڪر() + sel2p->ويڪر()},
                                    جديد AstConcat{rhs1p->fileline(), rhs1p, rhs2p});
        }
        // UINFOTREE(1, pnewp, "", "conew");
        nodep->replaceWith(newp);  // dypep intentionally changing
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        VL_DO_DANGLING(ڌڪو_Deletep(nextp->unlink_Fr_پٺ()), nextp);
        أرجع سچ;
    }

    منطقي replace_سونپيو_SFormat_F(Ast_ڳنڍ_سونپيو* nodep) {
        // Rewrite 'x = sformatf(...)' into 'sformat(x, ...)', which is more efficient
        // at the call site as it does not need to check if 'x' needs to be freed first.
        // This is a somewhat common pattern after lowering assertions.
        إذا (!VN_IS(nodep, سونپيو)) أرجع خطأ;
        AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef);
        إذا (!lhsp) أرجع خطأ;
        AstSFormatF* ثابت fmtp = VN_CAST(nodep->rhsp(), SFormatF);
        إذا (!fmtp) أرجع خطأ;
        lhsp->unlink_Fr_پٺ();
        fmtp->unlink_Fr_پٺ();
        nodep->replaceWith(جديد AstSFormat{nodep->fileline(), fmtp, lhsp});
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        أرجع سچ;
    }

    منطقي varNotReferenced(Ast_ڳنڍ* nodep, AstVar* varp, صحيح level = 0) {
        // Return true if varp never referenced under node.
        // Return false if referenced, or tree too deep to be worth it, or side effects
        إذا (!nodep) أرجع سچ;
        إذا (level > 2) أرجع خطأ;
        إذا (!nodep->is_خالص()) أرجع خطأ;  // For example a $fgetc can't be reordered
        إذا (VN_IS(nodep, ڳنڍ_Var_Ref) && VN_AS(nodep, ڳنڍ_Var_Ref)->varp() == varp) أرجع خطأ;
        أرجع (varNotReferenced(nodep->nextp(), varp, level + 1)
                && varNotReferenced(nodep->op1p(), varp, level + 1)
                && varNotReferenced(nodep->op2p(), varp, level + 1)
                && varNotReferenced(nodep->op3p(), varp, level + 1)
                && varNotReferenced(nodep->op4p(), varp, level + 1));
    }

    منطقي replace_ڳنڍ_سونپيو(Ast_ڳنڍ_سونپيو* nodep) {
        إذا (VN_IS(nodep->lhsp(), VarRef) && VN_IS(nodep->rhsp(), VarRef)
            && VN_AS(nodep->lhsp(), VarRef)->sameNoLvalue(VN_AS(nodep->rhsp(), VarRef))
            && !VN_IS(nodep, سونپيو_Dly)) {
            // X = X.  Quite pointless, though X <= X may override another earlier assignment
            إذا (VN_IS(nodep, سونپيو_W)) {
                إذا (!v3Global.opt.pedantic())
                    nodep->v3error(
                        "Wire inputs its own output, creating circular logic (wire x=x)");
                أرجع خطأ;  // Don't delete the assign, or V3Gate will freak out
            } وإلا {
                VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                أرجع سچ;
            }
        } وإلا إذا (m_doV && VN_IS(nodep->lhsp(), Concat)) {
            منطقي need_عارضي = خطأ;
            // A blocking assignment with an intra-assignment timing control evaluates its value,
            // then waits once (IEEE 1800-2023 9.4.5), here to assign a temporary for the parts
            ثابت منطقي waits = VN_IS(nodep, سونپيو) && nodep->timingControlp();
            // Splitting copies the RHS once per part, so a large RHS would make the tree
            // grow with the product of the number of parts and the RHS size
            ثابت منطقي need_عارضي_خالص = !nodep->rhsp()->is_خالص() || waits
                                      || nodep->rhsp()->isLargerThan(CONCAT_سونپيو_MAX_RHS_NODES);
            إذا (m_warn && !VN_IS(nodep, سونپيو_Dly)
                && !need_عارضي_خالص) {  // Is same var on LHS and RHS?
                // If the rhs is not pure, we need a temporary variable anyway
                std::unordered_سيٽ<سائيز_t> lhsVarIds;
                nodep->lhsp()->foreach([&](ثابت AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    lhsVarIds.emplace(m_ids(refp->varp()));
                });
                nodep->rhsp()->foreach([&](ثابت AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    إذا (lhsVarIds.ڳڻپ(m_ids(refp->varp()))) need_عارضي = سچ;
                });
            }
            إذا (need_عارضي_خالص) {
                // if the RHS is impure or large we need to create a temporary variable for it,
                // because further handling involves copying of the RHS.
                UINFO(4, "  ASSITEMPPURE " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp,rhs),
                //                                ASSIGN(lc1,SEL(temp,{size1})),
                //                                ASSIGN(lc2,SEL(temp,{size2}))

                Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٺ();
                AstVar* ثابت عارضي_Purep = جديد AstVar{rhsp->fileline(), VVarType::BLOCKTEMP,
                                                     m_concswap_نالا.get(rhsp), rhsp->dtypep()};
                // Static, as read right after written, also in a fork
                عارضي_Purep->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڙيو_Stmtsp(عارضي_Purep);
                AstVarRef* ثابت عارضي_خالص_Refp
                    = جديد AstVarRef{rhsp->fileline(), عارضي_Purep, VAccess::WRITE};
                Ast_ڳنڍ_سونپيو* ثابت asnp
                    = VN_IS(nodep, سونپيو_Dly)
                          ? جديد Ast_سونپيو{nodep->fileline(), عارضي_خالص_Refp, rhsp}
                          : nodep->cloneType(عارضي_خالص_Refp, rhsp);
                nodep->جوڙيو_هتي_This_As_ايندڙ(asnp);
                nodep->rhsp(جديد AstVarRef{rhsp->fileline(), عارضي_Purep, VAccess::READ});
                // Only the assignment of the temporary waits
                إذا (waits) ڌڪو_Deletep(nodep->timingControlp()->unlink_Fr_پٺ());
            } وإلا إذا (need_عارضي) {
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
                = VN_IS(nodep, سونپيو_Dly) ? VN_CAST(nodep->timingControlp(), Delay) : خالي_اشارو;
            إذا (delayp && !delayp->lhsp()->is_خالص()) {
                File_سٽ* ثابت flp = nodep->fileline();
                Ast_ڳنڍ_اظهار* ثابت valuep = delayp->lhsp()->unlink_Fr_پٺ();
                AstVar* ثابت tempp = جديد AstVar{flp, VVarType::BLOCKTEMP,
                                                 m_concswap_نالا.get(valuep), valuep->dtypep()};
                // Static, as read right after written, when the NBAs to the parts start waiting
                tempp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڙيو_Stmtsp(tempp);
                nodep->جوڙيو_هتي_This_As_ايندڙ(
                    جديد Ast_سونپيو{flp, جديد AstVarRef{flp, tempp, VAccess::WRITE}, valuep});
                delayp->lhsp(جديد AstVarRef{flp, tempp, VAccess::READ});
            }
            UINFOTREE(9, nodep, "", "Ass_old");
            // Unlink the stuff
            Ast_ڳنڍ_اظهار* ثابت lc1p = VN_AS(nodep->lhsp(), Concat)->lhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت lc2p = VN_AS(nodep->lhsp(), Concat)->rhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت conp = VN_AS(nodep->lhsp(), Concat)->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت rhs2p = rhsp->clone_وڻ_خالص(خطأ);
            // Calc widths
            ثابت صحيح lsb2 = 0;
            ثابت صحيح msb2 = lsb2 + lc2p->ويڪر() - 1;
            ثابت صحيح lsb1 = msb2 + 1;
            ثابت صحيح msb1 = lsb1 + lc1p->ويڪر() - 1;
            UASSERT_OBJ(msb1 == (conp->ويڪر() - 1), nodep, "Width calc mismatch");
            // Form ranges
            AstSel* ثابت sel1p = جديد AstSel{conp->fileline(), rhsp, lsb1, msb1 - lsb1 + 1};
            AstSel* ثابت sel2p = جديد AstSel{conp->fileline(), rhs2p, lsb2, msb2 - lsb2 + 1};
            // Make new assigns of same flavor as old one
            //*** Not cloneTree; just one node.
            Ast_ڳنڍ_سونپيو* newp = خالي_اشارو;
            إذا (!need_عارضي) {
                Ast_ڳنڍ_سونپيو* ثابت asn1ap = nodep->cloneType(lc1p, sel1p);
                Ast_ڳنڍ_سونپيو* ثابت asn2ap = nodep->cloneType(lc2p, sel2p);
                asn1ap->dtypeFrom(sel1p);
                asn2ap->dtypeFrom(sel2p);
                newp = Ast_ڳنڍ::جوڙيو_ايندڙ(newp, asn1ap);
                newp = Ast_ڳنڍ::جوڙيو_ايندڙ(newp, asn2ap);
            } وإلا {
                UASSERT_OBJ(m_modp, nodep, "Not under module");
                UASSERT_OBJ(m_globalPass, nodep,
                            "Should not reach here when not invoked on whole AstNetlist");
                // We could create just one temp variable, but we'll get better optimization
                // if we make one per term.
                AstVar* ثابت temp1p
                    = جديد AstVar{sel1p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswap_نالا.get(sel1p), VFlagLogicPacked{}, msb1 - lsb1 + 1};
                AstVar* ثابت temp2p
                    = جديد AstVar{sel2p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswap_نالا.get(sel2p), VFlagLogicPacked{}, msb2 - lsb2 + 1};
                m_modp->جوڙيو_Stmtsp(temp1p);
                m_modp->جوڙيو_Stmtsp(temp2p);
                Ast_ڳنڍ_سونپيو* ثابت asn1ap = nodep->cloneType(
                    جديد AstVarRef{sel1p->fileline(), temp1p, VAccess::WRITE}, sel1p);
                Ast_ڳنڍ_سونپيو* ثابت asn2ap = nodep->cloneType(
                    جديد AstVarRef{sel2p->fileline(), temp2p, VAccess::WRITE}, sel2p);
                Ast_ڳنڍ_سونپيو* ثابت asn1bp = nodep->cloneType(
                    lc1p, جديد AstVarRef{sel1p->fileline(), temp1p, VAccess::READ});
                Ast_ڳنڍ_سونپيو* ثابت asn2bp = nodep->cloneType(
                    lc2p, جديد AstVarRef{sel2p->fileline(), temp2p, VAccess::READ});
                asn1ap->dtypeFrom(temp1p);
                asn1bp->dtypeFrom(temp1p);
                asn2ap->dtypeFrom(temp2p);
                asn2bp->dtypeFrom(temp2p);
                // This order matters
                newp = Ast_ڳنڍ::جوڙيو_ايندڙ(newp, asn1ap);
                newp = Ast_ڳنڍ::جوڙيو_ايندڙ(newp, asn2ap);
                newp = Ast_ڳنڍ::جوڙيو_ايندڙ(newp, asn1bp);
                newp = Ast_ڳنڍ::جوڙيو_ايندڙ(newp, asn2bp);
            }
            إذا (debug() >= 9 && newp) newp->dump_وڻ_And_ايندڙ(ٻاهر, "-     _new: ");
            nodep->جوڙيو_ايندڙ_هتي(newp);
            // Cleanup
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
            VL_DO_DANGLING(ڌڪو_Deletep(conp), conp);
            // Further reduce, either node may have more reductions.
            أرجع سچ;
        } وإلا إذا (m_doV && VN_IS(nodep->rhsp(), Cvt_Packed_منزل_Array)
                   && هيٺاهون_As_Fixed_Aggregate(nodep->lhsp()->dtypep())) {
            Ast_Cvt_Packed_منزل_Array* ثابت cvtp
                = VN_AS(nodep->rhsp(), Cvt_Packed_منزل_Array)->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* srcp = cvtp->fromp()->unlink_Fr_پٺ();
            إذا (هيٺاهون_As_Fixed_Aggregate(srcp->dtypep())) {
                srcp = packFixedAggregate(srcp);
            } وإلا إذا (Ast_ڳنڍ_وهڪرو* ثابت streamp = VN_CAST(srcp, ڳنڍ_وهڪرو)) {
                Ast_ڳنڍ_اظهار* ثابت وهڪرو_Srcp = streamp->lhsp();
                إذا (هيٺاهون_As_Fixed_Aggregate(وهڪرو_Srcp->dtypep())) {
                    Ast_ڳنڍ_اظهار* ثابت packedp = packFixedAggregate(وهڪرو_Srcp->unlink_Fr_پٺ());
                    streamp->lhsp(packedp);
                    streamp->dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(packedp->ويڪر(), packedp->ويڪر_Min(),
                                                       VSigning::UNSIGNED,
                                                       packedp->isFourstateOrNoDtype());
                }
            }
            VL_DO_DANGLING(ڌڪو_Deletep(cvtp), cvtp);
            replace_سونپيو_منزل_Fixed_Aggregate(nodep, nodep->lhsp()->unlink_Fr_پٺ(), srcp);
            أرجع سچ;
        } وإلا إذا (m_doV && VN_IS(nodep->rhsp(), وهڪرو_R)
                   && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), قطار_DType)) {
            // The right-streaming operator on rhs of assignment does not
            // change the order of bits. Eliminate stream but keep its lhsp.
            // Add a cast if needed.
            Ast_وهڪرو_R* ثابت streamp = VN_AS(nodep->rhsp(), وهڪرو_R)->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* srcp = streamp->lhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            ثابت Ast_ڳنڍ_DType* ثابت dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            إذا (هيٺاهون_As_Fixed_Aggregate(srcDTypep)) {
                srcp = packFixedAggregate(srcp);
            } وإلا إذا (VN_IS(srcDTypep, قطار_DType) || VN_IS(srcDTypep, DynArrayDType)) {
                إذا (VN_IS(dstDTypep, قطار_DType) || VN_IS(dstDTypep, DynArrayDType)) {
                    صحيح src_Element_بٽ = 0;
                    إذا (ثابت Ast_ڳنڍ_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                        src_Element_بٽ = elemDtp->ويڪر();
                    }
                    صحيح dst_Element_بٽ = 0;
                    إذا (ثابت Ast_ڳنڍ_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_بٽ = elemDtp->ويڪر();
                    }
                    srcp = جديد Ast_Cvt_Array_منزل_Array{
                        srcp->fileline(), srcp,          nodep->dtypep(), خطأ, 1,
                        dst_Element_بٽ,   src_Element_بٽ};
                } وإلا {
                    srcp = جديد Ast_Cvt_Array_منزل_Packed{srcp->fileline(), srcp, nodep->dtypep()};
                }
            } وإلا إذا (VN_IS(srcDTypep, UnpackArrayDType)) {
                srcp = جديد Ast_Cvt_Array_منزل_Packed{srcp->fileline(), srcp, srcDTypep};
                // Handling the case where lhs is wider than rhs by inserting zeros. StreamL does
                // not require this, since the left streaming operator implicitly handles this.
                ثابت صحيح packed_بٽ = nodep->lhsp()->ويڪر_Min();
                ثابت صحيح unpack_بٽ
                    = srcDTypep->arrayUnpackedElements() * srcDTypep->subDTypep()->ويڪر_Min();
                ثابت uint32_t وٿي = packed_بٽ > unpack_بٽ ? packed_بٽ - unpack_بٽ : 0;
                srcp = جديد Ast_سرڪايو_L{srcp->fileline(), srcp,
                                     جديد AstConst{srcp->fileline(), وٿي}, packed_بٽ};
            }
            إذا (!VN_IS(dstDTypep, UnpackArrayDType) && !VN_IS(dstDTypep, قطار_DType)
                && !VN_IS(dstDTypep, DynArrayDType)) {
                ثابت صحيح s_ويڪر = srcp->ويڪر();
                ثابت صحيح d_ويڪر = nodep->lhsp()->ويڪر();
                إذا (s_ويڪر < d_ويڪر) {
                    AstExtend* ثابت extendp = جديد AstExtend{srcp->fileline(), srcp, d_ويڪر};
                    srcp = جديد Ast_سرڪايو_L{
                        srcp->fileline(), extendp,
                        جديد AstConst{srcp->fileline(), static_cast<uint32_t>(d_ويڪر - s_ويڪر)},
                        d_ويڪر};
                }
            }
            nodep->rhsp(srcp);
            VL_DO_DANGLING(ڌڪو_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            أرجع سچ;
        } وإلا إذا (m_doV && VN_IS(nodep->lhsp(), وهڪرو_L)) {
            // Push the stream operator to the rhs of the assignment statement
            Ast_ڳنڍ_اظهار* streamp = nodep->lhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت dstp = VN_AS(streamp, وهڪرو_L)->lhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_DType* ثابت dstDTypep = dstp->dtypep()->skipRefp();
            Ast_ڳنڍ_اظهار* srcp = nodep->rhsp()->unlink_Fr_پٺ();
            ثابت Ast_ڳنڍ_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamL reverses, so reverse=true)
            إذا ((VN_IS(srcDTypep, UnpackArrayDType) || VN_IS(srcDTypep, قطار_DType)
                 || VN_IS(srcDTypep, DynArrayDType))
                && (VN_IS(dstDTypep, قطار_DType) || VN_IS(dstDTypep, DynArrayDType))) {
                صحيح بلاڪ_سائيز = 1;
                إذا (ثابت AstConst* ثابت constp
                    = VN_CAST(VN_AS(streamp, وهڪرو_L)->rhsp(), Const)) {
                    بلاڪ_سائيز = constp->منزل_SInt();
                    إذا (VL_UNLIKELY(بلاڪ_سائيز <= 0)) بلاڪ_سائيز = 1;
                }
                صحيح src_Element_بٽ = 0;
                إذا (ثابت Ast_ڳنڍ_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                    src_Element_بٽ = elemDtp->ويڪر();
                }
                صحيح dst_Element_بٽ = 0;
                إذا (ثابت Ast_ڳنڍ_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                    dst_Element_بٽ = elemDtp->ويڪر();
                }
                nodep->lhsp(dstp);
                nodep->rhsp(جديد Ast_Cvt_Array_منزل_Array{srcp->fileline(), srcp, dstDTypep, سچ,
                                                   بلاڪ_سائيز, dst_Element_بٽ, src_Element_بٽ});
                nodep->dtypep(dstDTypep);
                VL_DO_DANGLING(ڌڪو_Deletep(streamp), streamp);
                أرجع سچ;
            }
            إذا (هيٺاهون_As_Fixed_Aggregate(srcDTypep)) srcp = packFixedAggregate(srcp);
            ثابت صحيح s_ويڪر = srcp->ويڪر();
            ثابت صحيح d_ويڪر = dstp->ويڪر();
            // Connect the rhs to the stream operator and update its width
            VN_AS(streamp, وهڪرو_L)->lhsp(srcp);
            إذا (VN_IS(srcDTypep, DynArrayDType) || VN_IS(srcDTypep, قطار_DType)
                || VN_IS(srcDTypep, UnpackArrayDType)) {
                streamp->dtype_سيٽ_وهڪرو();
            } وإلا {
                streamp->dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(srcp->ويڪر(), srcp->ويڪر_Min(),
                                                   VSigning::UNSIGNED,
                                                   srcp->isFourstateOrNoDtype());
            }
            إذا (VN_IS(dstDTypep, UnpackArrayDType)) {
                streamp = جديد Ast_Cvt_Packed_منزل_Array{nodep->fileline(), streamp, dstDTypep};
            } وإلا {
                إذا (d_ويڪر == 0) {
                    streamp = جديد Ast_Cvt_Packed_منزل_Array{nodep->fileline(), streamp, dstDTypep};
                } وإلا إذا (s_ويڪر >= d_ويڪر) {
                    streamp = جديد AstSel{streamp->fileline(), streamp, s_ويڪر - d_ويڪر, d_ويڪر};
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(streamp);
            nodep->dtypep(dstDTypep);
            أرجع سچ;
        } وإلا إذا (m_doV && VN_IS(nodep->lhsp(), وهڪرو_R)) {
            // The right stream operator on lhs of assignment statement does
            // not reorder bits. However, if the rhs is wider than the lhs,
            // then we select bits from the left-most, not the right-most.
            Ast_ڳنڍ_اظهار* ثابت streamp = nodep->lhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت dstp = VN_AS(streamp, وهڪرو_R)->lhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_DType* ثابت dstDTypep = dstp->dtypep()->skipRefp();
            Ast_ڳنڍ_اظهار* srcp = nodep->rhsp()->unlink_Fr_پٺ();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamR does not reverse, so reverse=false).
            // V3Width may have wrapped the source in CvtArrayToPacked; unwrap it.
            إذا (VN_IS(dstDTypep, قطار_DType) || VN_IS(dstDTypep, DynArrayDType)) {
                Ast_ڳنڍ_اظهار* origSrcp = srcp;
                إذا (Ast_Cvt_Array_منزل_Packed* ثابت cvtp = VN_CAST(srcp, Cvt_Array_منزل_Packed)) {
                    origSrcp = cvtp->fromp();
                }
                ثابت Ast_ڳنڍ_DType* ثابت origSrcDTypep = origSrcp->dtypep()->skipRefp();
                إذا (VN_IS(origSrcDTypep, UnpackArrayDType) || VN_IS(origSrcDTypep, قطار_DType)
                    || VN_IS(origSrcDTypep, DynArrayDType)) {
                    صحيح src_Element_بٽ = 0;
                    إذا (ثابت Ast_ڳنڍ_DType* ثابت elemDtp = origSrcDTypep->subDTypep()) {
                        src_Element_بٽ = elemDtp->ويڪر();
                    }
                    صحيح dst_Element_بٽ = 0;
                    إذا (ثابت Ast_ڳنڍ_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_بٽ = elemDtp->ويڪر();
                    }
                    إذا (VN_IS(srcp, Cvt_Array_منزل_Packed)) {
                        origSrcp = VN_AS(srcp, Cvt_Array_منزل_Packed)->fromp()->unlink_Fr_پٺ();
                        VL_DO_DANGLING(ڌڪو_Deletep(srcp), srcp);
                        srcp = origSrcp;
                    }
                    // Descending unpacked arrays need element reversal
                    منطقي ابتو = خطأ;
                    إذا (ثابت AstUnpackArrayDType* ثابت unpackDtp
                        = VN_CAST(origSrcDTypep, UnpackArrayDType)) {
                        ابتو = !unpackDtp->declRange().ascending();
                    }
                    nodep->lhsp(dstp);
                    nodep->rhsp(جديد Ast_Cvt_Array_منزل_Array{srcp->fileline(), srcp, dstDTypep, ابتو,
                                                       1, dst_Element_بٽ, src_Element_بٽ});
                    nodep->dtypep(dstDTypep);
                    VL_DO_DANGLING(ڌڪو_Deletep(streamp), streamp);
                    أرجع سچ;
                }
            }
            ثابت صحيح s_ويڪر = srcp->ويڪر();
            ثابت صحيح d_ويڪر = dstp->ويڪر();
            إذا (VN_IS(dstDTypep, UnpackArrayDType)) {
                ثابت صحيح dst_ٻٽو_انگ_ويڪر
                    = d_ويڪر * VN_AS(dstDTypep, UnpackArrayDType)->arrayUnpackedElements();
                // Handling the case where rhs is wider than lhs. StreamL does not require this
                // since the combination of the left streaming operation and the implicit
                // truncation in VL_ASSIGN_UNPACK automatically selects the left-most bits.
                إذا (s_ويڪر > dst_ٻٽو_انگ_ويڪر) {
                    srcp
                        = جديد AstSel{streamp->fileline(), srcp, s_ويڪر - dst_ٻٽو_انگ_ويڪر, dst_ٻٽو_انگ_ويڪر};
                }
                srcp = جديد Ast_Cvt_Packed_منزل_Array{nodep->fileline(), srcp, dstDTypep};
            } وإلا {
                إذا (d_ويڪر == 0) {
                    srcp = جديد Ast_Cvt_Packed_منزل_Array{nodep->fileline(), srcp, dstDTypep};
                } وإلا إذا (s_ويڪر >= d_ويڪر) {
                    srcp = جديد AstSel{streamp->fileline(), srcp, s_ويڪر - d_ويڪر, d_ويڪر};
                } وإلا {
                    // Source narrower than destination: left-justify by shifting left.
                    // The right stream operator packs left-to-right, so remaining
                    // LSBs are zero-filled (IEEE 1800-2023 11.4.14.3).
                    إذا (!VN_IS(srcp->dtypep()->skipRefp(), قطار_DType)) {
                        AstExtend* ثابت extendp = جديد AstExtend{srcp->fileline(), srcp, d_ويڪر};
                        srcp = جديد Ast_سرڪايو_L{
                            srcp->fileline(), extendp,
                            جديد AstConst{srcp->fileline(), static_cast<uint32_t>(d_ويڪر - s_ويڪر)},
                            d_ويڪر};
                    } وإلا {  // if it is a queue we dont need do do a shiftL
                        srcp = جديد AstExtend{srcp->fileline(), srcp};
                    }
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(srcp);
            nodep->dtypep(dstDTypep);
            VL_DO_DANGLING(ڌڪو_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            أرجع سچ;
        } وإلا إذا (m_doV && VN_IS(nodep->rhsp(), وهڪرو_L)) {
            Ast_وهڪرو_L* streamp = VN_AS(nodep->rhsp(), وهڪرو_L);
            Ast_ڳنڍ_اظهار* srcp = streamp->lhsp();
            ثابت Ast_ڳنڍ_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            Ast_ڳنڍ_DType* ثابت dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            إذا (هيٺاهون_As_Fixed_Aggregate(srcDTypep)) {
                Ast_ڳنڍ_اظهار* ثابت packedp = packFixedAggregate(srcp->unlink_Fr_پٺ());
                streamp->lhsp(packedp);
                streamp->dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(packedp->ويڪر(), packedp->ويڪر_Min(),
                                                   VSigning::UNSIGNED,
                                                   packedp->isFourstateOrNoDtype());
                srcp = packedp;
            } وإلا إذا ((VN_IS(srcDTypep, قطار_DType) || VN_IS(srcDTypep, DynArrayDType)
                        || VN_IS(srcDTypep, UnpackArrayDType))) {
                إذا (VN_IS(dstDTypep, قطار_DType) || VN_IS(dstDTypep, DynArrayDType)) {
                    صحيح بلاڪ_سائيز = 1;
                    إذا (ثابت AstConst* ثابت constp = VN_CAST(streamp->rhsp(), Const)) {
                        بلاڪ_سائيز = constp->منزل_SInt();
                        إذا (VL_UNLIKELY(بلاڪ_سائيز <= 0)) {
                            // Not reachable due to higher level checks when parsing stream
                            // operators commented out to not fail v3error-coverage-checks.
                            // nodep->v3error("Stream block size must be positive, got " <<
                            // blockSize); nodep->v3error("Stream block size must be positive, got
                            // " << blockSize);
                            بلاڪ_سائيز = 1;
                        }
                    }
                    // Not reachable due to higher level checks when parsing stream operators
                    // commented out to not fail v3error-coverage-checks.
                    // else {
                    // nodep->v3error("Stream block size must be constant (got " <<
                    // streamp->rhsp()->prettyTypeName() << ")");
                    // }
                    صحيح src_Element_بٽ = 0;
                    إذا (ثابت Ast_ڳنڍ_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                        src_Element_بٽ = elemDtp->ويڪر();
                    }
                    صحيح dst_Element_بٽ = 0;
                    إذا (ثابت Ast_ڳنڍ_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_بٽ = elemDtp->ويڪر();
                    }
                    streamp->unlink_Fr_پٺ();
                    srcp = جديد Ast_Cvt_Array_منزل_Array{
                        srcp->fileline(), srcp->unlink_Fr_پٺ(), dstDTypep,     سچ,
                        بلاڪ_سائيز,        dst_Element_بٽ,       src_Element_بٽ};
                    nodep->rhsp(srcp);
                    VL_DO_DANGLING(ڌڪو_Deletep(streamp), streamp);
                } وإلا {
                    streamp->lhsp(جديد Ast_Cvt_Array_منزل_Packed{srcp->fileline(), srcp->unlink_Fr_پٺ(),
                                                          dstDTypep});
                    streamp->dtypeFrom(dstDTypep);
                }
            }
        } وإلا إذا (m_doV && replace_سونپيو_Multi_Sel(nodep)) {
            أرجع سچ;
        } وإلا إذا (replace_سونپيو_SFormat_F(nodep)) {
            أرجع سچ;
        }
        أرجع خطأ;
    }

    // Boolean replacements
    منطقي عمليو_Bool_سرڪايو(ثابت Ast_ڳنڍ* nodep) {
        // boolean test of AND(const,SHIFTR(x,const)) -> test of AND(SHIFTL(x,const), x)
        إذا (!VN_IS(nodep, And)) أرجع خطأ;
        إذا (!VN_IS(VN_AS(nodep, And)->lhsp(), Const)) أرجع خطأ;
        إذا (!VN_IS(VN_AS(nodep, And)->rhsp(), سرڪايو_R)) أرجع خطأ;
        ثابت Ast_سرڪايو_R* ثابت shiftp = VN_AS(VN_AS(nodep, And)->rhsp(), سرڪايو_R);
        إذا (!VN_IS(shiftp->rhsp(), Const)) أرجع خطأ;
        إذا (static_cast<uint32_t>(nodep->ويڪر()) <= VN_AS(shiftp->rhsp(), Const)->منزل_UInt()) {
            أرجع خطأ;
        }
        أرجع سچ;
    }
    فراغ replace_Bool_سرڪايو(Ast_ڳنڍ* nodep) {
        UINFOTREE(9, nodep, "", "bshft_old");
        AstConst* ثابت andConstp = VN_AS(VN_AS(nodep, And)->lhsp(), Const);
        Ast_ڳنڍ_اظهار* ثابت fromp
            = VN_AS(VN_AS(nodep, And)->rhsp(), سرڪايو_R)->lhsp()->unlink_Fr_پٺ();
        AstConst* ثابت سرڪايو_Constp
            = VN_AS(VN_AS(VN_AS(nodep, And)->rhsp(), سرڪايو_R)->rhsp(), Const);
        V3_عدد val{andConstp, andConstp->ويڪر()};
        val.op_سرڪايو_L(andConstp->انگ(), سرڪايو_Constp->انگ());
        AstAnd* ثابت newp
            = جديد AstAnd{nodep->fileline(), جديد AstConst{nodep->fileline(), val}, fromp};
        // widthMin no longer applicable if different C-expanded width
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        UINFOTREE(9, newp, "", "_new");
    }

    فراغ replaceWithSimulation(Ast_ڳنڍ* nodep) {
        SimulateVisitor simvis;
        // Run it - may be unoptimizable due to large for loop, etc
        simvis.mainParamEmulate(nodep);
        إذا (!simvis.optimizable()) {
            ثابت Ast_ڳنڍ* errorp = simvis.whyNotNodep();
            إذا (!errorp) errorp = nodep;
            nodep->v3error("Expecting expression to be constant, but can't determine constant for "
                           << nodep->prettyTypeName() << '\n'
                           << errorp->warnOther() << "... Location of non-constant "
                           << errorp->prettyTypeName() << ": " << simvis.why_Not_پيغام());
            VL_DO_DANGLING(replaceZero(nodep), nodep);
        } وإلا {
            // Fetch the result
            Ast_ڳنڍ* ثابت valuep = simvis.fetch_قدر_Null(nodep);  // valuep is owned by Simulate
            UASSERT_OBJ(valuep, nodep, "No value returned from simulation");
            // Replace it
            Ast_ڳنڍ* ثابت newp = valuep->clone_وڻ(خطأ);
            newp->fileline(nodep->fileline());
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Simulate->" << newp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        }
    }

    //----------------------------------------

    // VISITORS
    فراغ گهمو(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        iterateChildrenBackwardsConst(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_concswap_نالا.reset();
        iterateChildren(nodep);
    }
    فراغ گهمو(AstCFunc* nodep) override {
        // No ASSIGNW removals under funcs, we've long eliminated INITIALs
        // (We should perhaps rename the assignw's to just assigns)
        VL_RESTORER(m_wremove);
        m_wremove = خطأ;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstCLocalScope* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->stmtsp()) {
            nodep->unlink_Fr_پٺ();
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        }
    }
    فراغ گهمو(AstScope* nodep) override {
        // No ASSIGNW removals under scope, we've long eliminated INITIALs
        VL_RESTORER(m_wremove);
        VL_RESTORER(m_scopep);
        m_wremove = خطأ;
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    فراغ مٽا_سٽا_Sides(Ast_ڳنڍ_Bi_Com* nodep) {
        // COMMUTATIVE({a},CONST) -> COMMUTATIVE(CONST,{a})
        // This simplifies later optimizations
        Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp()->unlink_Fr_پٺ_With_ايندڙ();
        Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٺ_With_ايندڙ();
        nodep->lhsp(rhsp);
        nodep->rhsp(lhsp);
        iterate(nodep);  // Again?
    }

    منطقي شامل_آهي_Member_Access_Recurse(Ast_ڳنڍ* ثابت nodep) {
        إذا (!nodep) أرجع خطأ;
        ثابت سائيز_t id = m_ids(nodep);
        ثابت خودڪار it = m_شامل_آهي_Member_Access.ڳوليو(id);
        إذا (it != m_شامل_آهي_Member_Access.پڇاڙي()) أرجع it->ٻيو;
        ++m_statMemberAccessVisits;
        منطقي نتيجو = خطأ;
        إذا (VN_IS(nodep, MemberSel) || VN_IS(nodep, طريقو_Call) || VN_IS(nodep, CMethodCall)) {
            نتيجو = سچ;
        } وإلا إذا (ثابت Ast_ڳنڍ_FTask_Ref* ثابت funcRefp = VN_CAST(nodep, ڳنڍ_FTask_Ref)) {
            إذا (شامل_آهي_Member_Access_Recurse(funcRefp->taskp())) نتيجو = سچ;
        } وإلا إذا (ثابت Ast_ڳنڍ_CCall* ثابت funcRefp = VN_CAST(nodep, ڳنڍ_CCall)) {
            إذا (شامل_آهي_Member_Access_Recurse(funcRefp->funcp())) نتيجو = سچ;
        } وإلا إذا (ثابت Ast_ڳنڍ_FTask* ثابت funcp = VN_CAST(nodep, ڳنڍ_FTask)) {
            // Assume that it has a member access
            إذا (funcp->recursive()) نتيجو = سچ;
        } وإلا إذا (ثابت AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc)) {
            إذا (funcp->recursive()) نتيجو = سچ;
        }
        إذا (!نتيجو) {
            نتيجو = شامل_آهي_Member_Access_Recurse(nodep->op1p())
                     || شامل_آهي_Member_Access_Recurse(nodep->op2p())
                     || شامل_آهي_Member_Access_Recurse(nodep->op3p())
                     || شامل_آهي_Member_Access_Recurse(nodep->op4p());
        }
        إذا (!نتيجو && !VN_IS(nodep, ڳنڍ_FTask)
            && !VN_IS(nodep, CFunc)  // don't enter into next function
            && شامل_آهي_Member_Access_Recurse(nodep->nextp())) {
            نتيجو = سچ;
        }
        m_شامل_آهي_Member_Access.emplace(id, نتيجو);
        أرجع نتيجو;
    }

    منطقي ميلاپ_Biop_منزل_Bitwise(Ast_ڳنڍ_Biop* ثابت nodep) {
        إذا (!m_convert_Logic_منزل_ٻٽو_انگ) أرجع خطأ;
        إذا (!nodep->lhsp()->width1()) أرجع خطأ;
        إذا (!nodep->rhsp()->width1()) أرجع خطأ;
        إذا (!nodep->is_خالص()) أرجع خطأ;
        إذا (شامل_آهي_Member_Access_Recurse(nodep)) أرجع خطأ;
        أرجع سچ;
    }
    منطقي ميلاپ_Concat_Rand(AstConcat* nodep) {
        //    CONCAT(RAND, RAND) - created by Chisel code
        AstRand* ثابت aRandp = VN_CAST(nodep->lhsp(), Rand);
        ثابت AstRand* ثابت bRandp = VN_CAST(nodep->rhsp(), Rand);
        إذا (!aRandp || !bRandp) أرجع خطأ;
        إذا (!aRandp->combinable(bRandp)) أرجع خطأ;
        UINFO(4, "Concat(Rand,Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlink_Fr_پٺ());
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        أرجع سچ;
    }
    منطقي ميلاپ_Sel_Rand(AstSel* nodep) {
        //    SEL(RAND) - created by Chisel code
        AstRand* ثابت aRandp = VN_CAST(nodep->fromp(), Rand);
        إذا (!aRandp) أرجع خطأ;
        إذا (aRandp->seedp()) أرجع خطأ;
        UINFO(4, "Sel(Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlink_Fr_پٺ());
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        أرجع سچ;
    }
    منطقي ميلاپ_منزل_String_NConst(Ast_منزل_String_N* nodep) {
        iterateChildren(nodep);
        إذا (ثابت AstConsArray* ثابت consp = VN_CAST(nodep->lhsp(), ConsArray)) {
            إذا (!(m_doExpensive || m_params)) أرجع خطأ;
            إذا (!consp->is_Constant_وڻ()) أرجع خطأ;
        } وإلا إذا (!VN_IS(nodep->lhsp(), Const)) {
            أرجع خطأ;
        }
        replaceWithSimulation(nodep);
        أرجع سچ;
    }
    صحيح عمليو_Concat_Move(ثابت AstConcat* nodep) {
        //    CONCAT under concat  (See moveConcat)
        // Return value: true indicates to do it; 2 means move to LHS
        ثابت AstConcat* ثابت abConcp = VN_CAST(nodep->lhsp(), Concat);
        ثابت AstConcat* ثابت bcConcp = VN_CAST(nodep->rhsp(), Concat);
        إذا (!abConcp && !bcConcp) أرجع 0;
        إذا (bcConcp) {
            ثابت Ast_ڳنڍ_اظهار* ثابت ap = nodep->lhsp();
            ثابت Ast_ڳنڍ_اظهار* ثابت bp = bcConcp->lhsp();
            // If a+b == 32,64,96 etc, then we want to have a+b together on LHS
            إذا (VL_BITBIT_I(ap->ويڪر() + bp->ويڪر()) == 0) أرجع 2;  // Transform 2: to abConc
        } وإلا {  // abConcp
            // Unless lhs is already 32 bits due to above, reorder it
            إذا (VL_BITBIT_I(nodep->lhsp()->ويڪر()) != 0) أرجع 1;  // Transform 1: to bcConc
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
        إذا (عمليو_Concat_Move(nodep) > 1) {
            Ast_ڳنڍ_اظهار* ثابت ap = nodep->lhsp()->unlink_Fr_پٺ();
            AstConcat* ثابت bcConcp = VN_AS(nodep->rhsp(), Concat);
            bcConcp->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت bp = bcConcp->lhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت cp = bcConcp->rhsp()->unlink_Fr_پٺ();
            AstConcat* ثابت abConcp = جديد AstConcat{bcConcp->fileline(), ap, bp};
            nodep->lhsp(abConcp);
            nodep->rhsp(cp);
            // If bp was a concat, then we have this exact same form again!
            // Recurse rather then calling node->iterate to prevent 2^n recursion!
            إذا (عمليو_Concat_Move(abConcp)) moveConcat(abConcp);
            VL_DO_DANGLING(ڌڪو_Deletep(bcConcp), bcConcp);
        } وإلا {
            AstConcat* ثابت abConcp = VN_AS(nodep->lhsp(), Concat);
            abConcp->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت ap = abConcp->lhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت bp = abConcp->rhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت cp = nodep->rhsp()->unlink_Fr_پٺ();
            AstConcat* ثابت bcConcp = جديد AstConcat{abConcp->fileline(), bp, cp};
            nodep->lhsp(ap);
            nodep->rhsp(bcConcp);
            إذا (عمليو_Concat_Move(bcConcp)) moveConcat(bcConcp);
            VL_DO_DANGLING(ڌڪو_Deletep(abConcp), abConcp);
        }
    }

    // Special cases
    فراغ گهمو(AstConst*) override {}  // Already constant

    فراغ گهمو(AstCell* nodep) override {
        إذا (m_params) {
            iterate_And_ايندڙ_Null(nodep->paramsp());
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ گهمو(AstClassOrPackageRef* nodep) override { iterateChildren(nodep); }

    فراغ گهمو(Ast_ميلاپ_Masked* nodep) override {
        // Do not iterate the tables, they must be constant pool entries
        iterate(nodep->lhsp());
        إذا (AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const)) {
            replace_انگ(nodep, Ast_ميلاپ_Masked::fold(constp->انگ(), nodep->matchp()->varp()));
        }
    }

    فراغ گهمو(AstPin* nodep) override { iterateChildren(nodep); }

    فراغ replaceLogEq(AstLogEq* nodep) {
        // LOGEQ(a,b) => AstLogAnd{AstLogOr{AstLogNot{a},b},AstLogOr{AstLogNot{b},a}}
        Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٺ();
        // Do exactly as IEEE says, might result in extra terms, so in future may do differently
        AstLogAnd* ثابت newp = جديد AstLogAnd{
            nodep->fileline(),
            جديد AstLogOr{nodep->fileline(), جديد AstLogNot{nodep->fileline(), lhsp}, rhsp},
            جديد AstLogOr{nodep->fileline(),
                         جديد AstLogNot{nodep->fileline(), rhsp->clone_وڻ_خالص(خطأ)},
                         lhsp->clone_وڻ_خالص(خطأ)}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    فراغ replaceSelSel(AstSel* nodep) {
        // SEL(SEL({x},a,b),c,d) => SEL({x},a+c,d)
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ثابت belowp = VN_AS(nodep->fromp(), Sel);
        Ast_ڳنڍ_اظهار* ثابت fromp = belowp->fromp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت lsb1p = nodep->lsbp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت lsb2p = belowp->lsbp()->unlink_Fr_پٺ();
        // Eliminate lower range
        UINFO(4, "Elim Lower range: " << nodep);
        Ast_ڳنڍ_اظهار* newlsbp;
        إذا (VN_IS(lsb1p, Const) && VN_IS(lsb2p, Const)) {
            newlsbp = جديد AstConst{lsb1p->fileline(),
                                   VN_AS(lsb1p, Const)->منزل_UInt() + VN_AS(lsb2p, Const)->منزل_UInt()};
            VL_DO_DANGLING(ڌڪو_Deletep(lsb1p), lsb1p);
            VL_DO_DANGLING(ڌڪو_Deletep(lsb2p), lsb2p);
        } وإلا {
            // Width is important, we need the width of the fromp's expression, not the
            // potentially smaller lsb1p's width, but don't insert a redundant AstExtend.
            // Note that due to some sloppiness in earlier passes, lsb1p might actually be wider,
            // so extend to the wider type.
            ثابت Ast_ڳنڍ_اظهار* ثابت widep = lsb1p->ويڪر() > lsb2p->ويڪر() ? lsb1p : lsb2p;
            Ast_ڳنڍ_اظهار* ثابت lhsp = widep->ويڪر() > lsb2p->ويڪر()
                                          ? جديد AstExtend{lsb2p->fileline(), lsb2p}
                                          : lsb2p;
            Ast_ڳنڍ_اظهار* ثابت rhsp = widep->ويڪر() > lsb1p->ويڪر()
                                          ? جديد AstExtend{lsb1p->fileline(), lsb1p}
                                          : lsb1p;
            lhsp->dtypeFrom(widep);
            rhsp->dtypeFrom(widep);
            newlsbp = جديد Ast_جوڙيو{lsb1p->fileline(), lhsp, rhsp};
            newlsbp->dtypeFrom(widep);
        }
        AstSel* ثابت newp = جديد AstSel{nodep->fileline(), fromp, newlsbp, nodep->ويڪر_Const()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    فراغ replaceSelConcat(AstSel* nodep) {
        // SEL(CONCAT(a,b),c,d) => SEL(a or b, . .)
        AstConcat* ثابت conp = VN_AS(nodep->fromp(), Concat);
        Ast_ڳنڍ_اظهار* ثابت conLhsp = conp->lhsp();
        Ast_ڳنڍ_اظهار* ثابت conRhsp = conp->rhsp();
        إذا (static_cast<صحيح>(nodep->lsbConst()) >= conRhsp->ويڪر()) {
            conLhsp->unlink_Fr_پٺ();
            AstSel* ثابت newp
                = جديد AstSel{nodep->fileline(), conLhsp, nodep->lsbConst() - conRhsp->ويڪر(),
                             nodep->ويڪر_Const()};
            nodep->replaceWithKeepDType(newp);
        } وإلا إذا (static_cast<صحيح>(nodep->msbConst()) < conRhsp->ويڪر()) {
            conRhsp->unlink_Fr_پٺ();
            AstSel* ثابت newp
                = جديد AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(), nodep->ويڪر_Const()};
            nodep->replaceWithKeepDType(newp);
        } وإلا {
            // Yuk, split between the two
            conRhsp->unlink_Fr_پٺ();
            conLhsp->unlink_Fr_پٺ();
            AstConcat* ثابت newp
                = جديد AstConcat{nodep->fileline(),
                                جديد AstSel{nodep->fileline(), conLhsp, 0,
                                           nodep->msbConst() - conRhsp->ويڪر() + 1},
                                جديد AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(),
                                           conRhsp->ويڪر() - nodep->lsbConst()}};
            nodep->replaceWithKeepDType(newp);
        }
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    منطقي عمليو_Sel_Replicate(AstSel* nodep) {
        // SEL(REPLICATE(from,rep),lsb,width) => SEL(from,0,width) as long
        // as SEL's width <= b's width
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ثابت repp = VN_AS(nodep->fromp(), Replicate);
        Ast_ڳنڍ_اظهار* ثابت fromp = repp->srcp();
        ثابت AstConst* ثابت lsbp = VN_CAST(nodep->lsbp(), Const);
        إذا (!lsbp) أرجع خطأ;
        UASSERT_OBJ(fromp->ويڪر(), nodep, "Not widthed");
        إذا ((lsbp->منزل_UInt() / fromp->ويڪر())
            != ((lsbp->منزل_UInt() + nodep->ويڪر() - 1) / fromp->ويڪر())) {
            أرجع خطأ;
        }
        //
        fromp->unlink_Fr_پٺ();
        AstSel* ثابت newp = جديد AstSel{
            nodep->fileline(), fromp,
            جديد AstConst{lsbp->fileline(), lsbp->منزل_UInt() % fromp->ويڪر()}, nodep->ويڪر_Const()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        أرجع سچ;
    }
    منطقي عمليو_Rep_Rep(AstReplicate* nodep) {
        // REPLICATE(REPLICATE2(from2,cnt2),cnt1) => REPLICATE(from2,(cnt1+cnt2))
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ثابت rep2p = VN_AS(nodep->srcp(), Replicate);
        Ast_ڳنڍ_اظهار* ثابت from2p = rep2p->srcp();
        ثابت AstConst* ثابت cnt1p = VN_CAST(nodep->countp(), Const);
        إذا (!cnt1p) أرجع خطأ;
        ثابت AstConst* ثابت cnt2p = VN_CAST(rep2p->countp(), Const);
        إذا (!cnt2p) أرجع خطأ;
        //
        from2p->unlink_Fr_پٺ();
        AstReplicate* ثابت newp
            = جديد AstReplicate{nodep->fileline(), from2p, cnt1p->منزل_UInt() * cnt2p->منزل_UInt()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        أرجع سچ;
    }
    منطقي عمليو_Concat_Same(AstConcat* nodep) {
        // CONCAT(fromp,fromp) -> REPLICATE(fromp,1+1)
        // CONCAT(REP(fromp,cnt1),fromp) -> REPLICATE(fromp,cnt1+1)
        // CONCAT(fromp,REP(fromp,cnt1)) -> REPLICATE(fromp,1+cnt1)
        // CONCAT(REP(fromp,cnt1),REP(fromp,cnt2)) -> REPLICATE(fromp,cnt1+cnt2)
        Ast_ڳنڍ_اظهار* from1p = nodep->lhsp();
        uint32_t cnt1 = 1;
        Ast_ڳنڍ_اظهار* from2p = nodep->rhsp();
        uint32_t cnt2 = 1;
        إذا (VN_IS(from1p, Replicate)) {
            ثابت AstConst* ثابت cnt1p = VN_CAST(VN_CAST(from1p, Replicate)->countp(), Const);
            إذا (!cnt1p) أرجع خطأ;
            from1p = VN_AS(from1p, Replicate)->srcp();
            cnt1 = cnt1p->منزل_UInt();
        }
        إذا (VN_IS(from2p, Replicate)) {
            ثابت AstConst* ثابت cnt2p = VN_CAST(VN_CAST(from2p, Replicate)->countp(), Const);
            إذا (!cnt2p) أرجع خطأ;
            from2p = VN_AS(from2p, Replicate)->srcp();
            cnt2 = cnt2p->منزل_UInt();
        }
        إذا (!operandsSame(from1p, from2p)) أرجع خطأ;
        //
        from1p->unlink_Fr_پٺ();
        AstReplicate* ثابت newp = جديد AstReplicate{nodep->fileline(), from1p, cnt1 + cnt2};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        أرجع سچ;
    }
    فراغ replaceSelIntoBiop(AstSel* nodep) {
        // SEL(BUFIF1(a,b),1,bit) => BUFIF1(SEL(a,1,bit),SEL(b,1,bit))
        Ast_ڳنڍ_Biop* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_پٺ(), ڳنڍ_Biop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_ڳنڍ_اظهار* ثابت lsbp = nodep->lsbp()->unlink_Fr_پٺ();
        //
        Ast_ڳنڍ_اظهار* ثابت bilhsp = fromp->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت birhsp = fromp->rhsp()->unlink_Fr_پٺ();
        //
        fromp->lhsp(
            جديد AstSel{nodep->fileline(), bilhsp, lsbp->clone_وڻ_خالص(سچ), nodep->ويڪر_Const()});
        fromp->rhsp(جديد AstSel{nodep->fileline(), birhsp, lsbp, nodep->ويڪر_Const()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ replaceSelIntoUniop(AstSel* nodep) {
        // SEL(NOT(a),1,bit) => NOT(SEL(a,bit))
        Ast_ڳنڍ_Uniop* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_پٺ(), ڳنڍ_Uniop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_ڳنڍ_اظهار* ثابت lsbp = nodep->lsbp()->unlink_Fr_پٺ();
        //
        Ast_ڳنڍ_اظهار* ثابت bilhsp = fromp->lhsp()->unlink_Fr_پٺ();
        //
        fromp->lhsp(جديد AstSel{nodep->fileline(), bilhsp, lsbp, nodep->ويڪر_Const()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    فراغ گهمو(Ast_Attr_جو* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        iterateChildren(nodep);
    }

    فراغ گهمو(AstArraySel* nodep) override {
        iterate_And_ايندڙ_Null(nodep->bitp());
        إذا (VN_IS(nodep->bitp(), Const)
            && VN_IS(nodep->fromp(), VarRef)
            // Need to make sure it's an array object so don't mis-allow a constant (bug509.)
            && VN_AS(nodep->fromp(), VarRef)->varp()
            && VN_IS(VN_AS(nodep->fromp(), VarRef)->varp()->valuep(), ConsArray)) {
            m_selp = nodep;  // Ask visit(AstVarRef) to replace varref with const
        }
        iterate_And_ايندڙ_Null(nodep->fromp());
        إذا (VN_IS(nodep->fromp(), Const)) {  // It did.
            إذا (!m_selp) {
                nodep->v3error("Illegal assignment of constant to unpacked array");
            } وإلا {
                Ast_ڳنڍ* ثابت fromp = nodep->fromp()->unlink_Fr_پٺ();
                nodep->replaceWithKeepDType(fromp);
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            }
        }
        // Handle ARRAYSEL directly on ConsArray (not through VarRef)
        وإلا إذا (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ConsArray)) {
            ثابت AstConsArray* ثابت consp = VN_AS(nodep->fromp(), ConsArray);
            ثابت uint32_t ٻٽو_انگ_2 = VN_AS(nodep->bitp(), Const)->منزل_UInt();
            Ast_ڳنڍ_اظهار* ثابت itemp = consp->valuep(ٻٽو_انگ_2);
            إذا (VN_IS(itemp, Const)) {
                ثابت V3_عدد& انگ = VN_AS(itemp, Const)->انگ();
                VL_DO_DANGLING(replace_انگ(nodep, انگ), nodep);
            } وإلا إذا (m_params && itemp) {
                // An aggregate element, e.g. a structure in an array parameter
                nodep->replaceWithKeepDType(itemp->clone_وڻ(خطأ));
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            }
        }
        m_selp = خالي_اشارو;
    }

    // Evaluate a slice of an unpacked array.  If constantification is
    // required (m_required=true), call replaceWithSimulation() to compute
    // the slice via simulation.  Otherwise just iterate the children.
    فراغ گهمو(Ast_ٽڪرو_Sel* nodep) override {
        // First constify or width any child nodes
        iterateChildren(nodep);
        إذا (!m_required) أرجع;  // Do nothing unless we are in parameter mode
        // Fallback to simulation: this will invoke SimulateVisitor::visit(AstSliceSel*)
        replaceWithSimulation(nodep);
    }

    فراغ گهمو(AstStructSel* nodep) override {
        iterateChildren(nodep);

        إذا (ثابت AstConsStruct* ثابت consp = VN_CAST(nodep->fromp(), ConsStruct)) {
            Ast_ڳنڍ_اظهار* ثابت valuep = consp->memberValuep(nodep->name());
            إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
                VL_DO_DANGLING(replace_انگ(nodep, constp->انگ()), nodep);
            } وإلا {
                nodep->replaceWith(valuep->clone_وڻ(خطأ));
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            }
        }
    }

    فراغ گهمو(AstCAwait* nodep) override {
        m_hasJumpDelay = سچ;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Not linked");
        منطقي did = خطأ;
        إذا (m_doV && (!nodep->varp()->const_Pool_داخلا() || m_selp) && nodep->varp()->valuep()
            && !m_attrp) {
            // UINFOTREE(1, valuep, "", "visitvaref");
            iterate_And_ايندڙ_Null(nodep->varp()->valuep());  // May change nodep->varp()->valuep()
            Ast_ڳنڍ* ثابت valuep = nodep->varp()->valuep();
            // The selected element, when under a constant select of an array
            ثابت AstConsArray* ثابت consp = VN_CAST(valuep, ConsArray);
            ثابت Ast_ڳنڍ* ثابت selItemp
                = (m_selp && consp) ? consp->valuep(m_selp->ٻٽو_انگ_Const()) : خالي_اشارو;
            إذا (nodep->access().isReadOnly() && valuep
                && ((!m_params  // Can reduce constant wires into equations
                     && m_doNConst && v3Global.opt.fConst() && nodep->varp()->isConst())
                    || nodep->varp()->isParam())) {
                إذا (عمليو_Const(valuep)) {
                    ثابت V3_عدد& انگ = VN_AS(valuep, Const)->انگ();
                    // UINFO(2, "constVisit " << cvtToHex(valuep) << " " << num);
                    ثابت string origParamName
                        = nodep->varp()->isParam() ? nodep->varp()->name() : "";
                    VL_DO_DANGLING(replace_انگ(nodep, انگ, origParamName), nodep);
                    did = سچ;
                } وإلا إذا (ثابت AstConst* ثابت itemp = VN_CAST(selItemp, Const)) {
                    VL_DO_DANGLING(replace_انگ(nodep, itemp->انگ()), nodep);
                    did = سچ;
                } وإلا إذا (m_params && consp) {
                    // Allow parameters to pass arrays
                    // Earlier recursion of ConsArray made sure each array value is constant
                    // This exception is fairly fragile, i.e. doesn't
                    // support arrays of arrays or other stuff
                    Ast_ڳنڍ* ثابت newp = valuep->clone_وڻ(خطأ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                    did = سچ;
                } وإلا إذا (m_params && VN_IS(valuep, ConsStruct)) {
                    Ast_ڳنڍ* ثابت newp = valuep->clone_وڻ(خطأ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                    did = سچ;
                } وإلا إذا (nodep->varp()->isParam() && VN_IS(valuep, Unbounded)) {
                    Ast_ڳنڍ* ثابت newp = valuep->clone_وڻ(خطأ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                    did = سچ;
                }
            }
        }
        إذا (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but variable isn't const: "
                           << nodep->varp()->prettyNameQ());
        }
    }
    فراغ گهمو(Ast_اظهار_Stmt* nodep) override {
        iterateChildren(nodep);
        إذا (!Ast_ڳنڍ::afterCommentp(nodep->stmtsp())) {
            deleteVarScopesUnder(nodep->stmtsp());
            UINFO(8, "ExprStmt(...) " << nodep << " " << nodep->resultp());
            nodep->replaceWith(nodep->resultp()->unlink_Fr_پٺ());
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            // Removing the ExprStmt might have made something impure above now pure
        }
    }
    فراغ گهمو(Ast_Enum_شي_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        منطقي did = خطأ;
        إذا (nodep->itemp()->valuep()) {
            // UINFOTREE(1, nodep->itemp()->valuep(), "", "visitvaref");
            ثابت Ast_Enum_شي* ثابت itemp = nodep->itemp();
            إذا (m_recursing_Enum_شيون.ڳڻپ(itemp)) {
                nodep->v3error("Recursive enum value: " << nodep->itemp()->prettyNameQ());
            } وإلا {
                m_recursing_Enum_شيون.emplace(itemp);
                iterate_And_ايندڙ_Null(nodep->itemp()->valuep());
                m_recursing_Enum_شيون.erase(itemp);
            }
            إذا (AstConst* ثابت valuep = VN_CAST(nodep->itemp()->valuep(), Const)) {
                ثابت V3_عدد& انگ = valuep->انگ();
                VL_DO_DANGLING(replace_انگ(nodep, انگ), nodep);
                did = سچ;
            }
        }
        إذا (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but enum value isn't const: "
                           << nodep->itemp()->prettyNameQ());
        }
    }

    منطقي only_Sen_شي_اندريون_Sen_وڻ(ثابت Ast_Sen_شي* nodep) {
        // Only one if it's not in a list
        أرجع (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }
    فراغ گهمو(Ast_Sen_شي* nodep) override {
        iterateChildren(nodep);
        إذا (m_doNConst
            && !v3Global.opt.timing().is_سيٽ_True()  // If --timing, V3Sched would turn this into an
                                                   // infinite loop. See #5080
            && (VN_IS(nodep->sensp(), Const) || VN_IS(nodep->sensp(), Enum_شي_Ref)
                || (nodep->varrefp() && nodep->varrefp()->varp()->isParam()))) {
            // Constants in sensitivity lists may be removed (we'll simplify later)
            إذا (nodep->isClocked()) {  // A constant can never get a pos/negedge
                إذا (only_Sen_شي_اندريون_Sen_وڻ(nodep)) {
                    إذا (nodep->ڪنارو_Type() == VEdgeType::ET_بدليو) {
                        // TODO: This really is dodgy, as strictly compliant simulators will not
                        //       execute this block, but but t_func_check relies on it
                        nodep->replaceWith(
                            جديد Ast_Sen_شي{nodep->fileline(), Ast_Sen_شي::Initial{}});
                    } وإلا {
                        nodep->replaceWith(جديد Ast_Sen_شي{nodep->fileline(), Ast_Sen_شي::Never{}});
                    }
                    VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                } وإلا {
                    VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                }
            } وإلا {  // Otherwise it may compute a result that needs to settle out
                nodep->replaceWith(جديد Ast_Sen_شي{nodep->fileline(), Ast_Sen_شي::Combo{}});
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            }
        } وإلا إذا (m_doNConst && VN_IS(nodep->sensp(), Not)) {
            // V3Gate may propagate NOTs into clocks... Just deal with it
            Ast_ڳنڍ* ثابت sensp = nodep->sensp();
            Ast_ڳنڍ* آخري_Sensp = sensp;
            منطقي invert = خطأ;
            طالما (VN_IS(آخري_Sensp, Not)) {
                آخري_Sensp = VN_AS(آخري_Sensp, Not)->lhsp();
                invert = !invert;
            }
            UINFO(8, "senItem(NOT...) " << nodep << " " << invert);
            إذا (invert) nodep->ڪنارو_Type(nodep->ڪنارو_Type().invert());
            sensp->replaceWith(آخري_Sensp->unlink_Fr_پٺ());
            VL_DO_DANGLING(ڌڪو_Deletep(sensp), sensp);
        }
    }

    صنف Sen_شي_Cmp final {
        ساكن صحيح cmp(ثابت Ast_ڳنڍ_اظهار* ap, ثابت Ast_ڳنڍ_اظهار* bp) {
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
                إذا (aConstp->انگ().isLtXZ(bConstp->انگ())) أرجع -1;
                إذا (bConstp->انگ().isLtXZ(aConstp->انگ())) أرجع 1;
                أرجع 0;
            }

            إذا (ثابت Ast_ڳنڍ_Biop* ثابت aBiOpp = VN_CAST(ap, ڳنڍ_Biop)) {
                ثابت Ast_ڳنڍ_Biop* ثابت bBiOpp = VN_AS(bp, ڳنڍ_Biop);
                // Compare RHSs first, as LHS might be const
                إذا (ثابت صحيح c = cmp(aBiOpp->rhsp(), bBiOpp->rhsp())) أرجع c;
                أرجع cmp(aBiOpp->lhsp(), bBiOpp->lhsp());
            }

            إذا (ثابت AstCMethodHard* ثابت aCallp = VN_CAST(ap, CMethodHard)) {
                ثابت AstCMethodHard* ثابت bCallp = VN_AS(bp, CMethodHard);
                إذا (aCallp->طريقو() < bCallp->طريقو()) أرجع -1;
                إذا (aCallp->طريقو() > bCallp->طريقو()) أرجع 1;
                إذا (ثابت صحيح c = cmp(aCallp->fromp(), bCallp->fromp())) أرجع c;
                ثابت Ast_ڳنڍ_اظهار* aPinsp = aCallp->pinsp();
                ثابت Ast_ڳنڍ_اظهار* bPinsp = bCallp->pinsp();
                طالما (aPinsp && bPinsp) {
                    إذا (ثابت صحيح c = cmp(aPinsp, bPinsp)) أرجع c;
                    aPinsp = VN_AS(aPinsp->nextp(), ڳنڍ_اظهار);
                    bPinsp = VN_AS(bPinsp->nextp(), ڳنڍ_اظهار);
                }
                أرجع aPinsp ? -1 : bPinsp ? 1 : 0;
            }

            أرجع 0;
        }

    عام:
        منطقي عامل()(ثابت Ast_Sen_شي* lhsp, ثابت Ast_Sen_شي* rhsp) ثابت {
            ثابت Ast_ڳنڍ_اظهار* ثابت lSensp = lhsp->sensp();
            ثابت Ast_ڳنڍ_اظهار* ثابت rSensp = rhsp->sensp();
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
            أرجع lhsp->ڪنارو_Type() < rhsp->ڪنارو_Type();
        }
    };

    فراغ گهمو(Ast_Sen_وڻ* nodep) override {
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
            لكل (Ast_Sen_شي *nextp, *senp = nodep->sensesp(); senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), Sen_شي);
                // cppcheck-suppress unassignedVariable  // cppcheck bug
                ثابت Sen_شي_Cmp cmp;
                إذا (nextp && !cmp(senp, nextp)) {
                    // Something's out of order, sort it
                    senp = خالي_اشارو;
                    std::ويڪٽر<Ast_Sen_شي*> vec;
                    لكل (Ast_Sen_شي* senp = nodep->sensesp(); senp;
                         senp = VN_AS(senp->nextp(), Sen_شي)) {
                        vec.ڌڪو_پٺ(senp);
                    }
                    stable_ترتيب_ڏيو(vec.begin(), vec.پڇاڙي(), Sen_شي_Cmp());
                    لكل (ثابت خودڪار& ip : vec) ip->unlink_Fr_پٺ();
                    لكل (ثابت خودڪار& ip : vec) nodep->جوڙيو_Sensesp(ip);
                    اكسر;
                }
            }

            // Pass 2, remove duplicates and simplify adjacent terms if possible
            لكل (Ast_Sen_شي *senp = nodep->sensesp(), *nextp; senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), Sen_شي);
                إذا (!nextp) اكسر;
                Ast_Sen_شي* ثابت lItemp = senp;
                Ast_Sen_شي* ثابت rItemp = nextp;
                ثابت Ast_ڳنڍ_اظهار* ثابت lSenp = lItemp->sensp();
                ثابت Ast_ڳنڍ_اظهار* ثابت rSenp = rItemp->sensp();
                إذا (!lSenp || !rSenp) استمر;

                إذا (lSenp->same_Gate_وڻ(rSenp)) {
                    // POSEDGE or NEGEDGE -> BOTHEDGE. (We've sorted POSEDGE, before NEGEDGE, so we
                    // do not need to test for the opposite orders.)
                    إذا (lItemp->ڪنارو_Type() == VEdgeType::ET_POSEDGE
                        && rItemp->ڪنارو_Type() == VEdgeType::ET_NEGEDGE) {
                        // Make both terms BOTHEDGE, the second will be removed below
                        lItemp->ڪنارو_Type(VEdgeType::ET_BOTHEDGE);
                        rItemp->ڪنارو_Type(VEdgeType::ET_BOTHEDGE);
                    }

                    // Remove identical expressions
                    إذا (lItemp->ڪنارو_Type() == rItemp->ڪنارو_Type()) {
                        VL_DO_DANGLING(ڌڪو_Deletep(rItemp->unlink_Fr_پٺ()), rItemp);
                        nextp = lItemp;
                    }
                }
            }
        }
    }

    //-----
    // Zero elimination
    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->timingControlp()) m_hasJumpDelay = سچ;
        إذا (m_doNConst && replace_ڳنڍ_سونپيو(nodep)) أرجع;
    }
    فراغ گهمو(AstAlias* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    فراغ گهمو(AstAliasScope* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    فراغ گهمو(Ast_سونپيو_W* nodep) override {
        iterateChildren(nodep);
        إذا (m_doNConst && replace_ڳنڍ_سونپيو(nodep)) أرجع;
        // Process containing this AssignW as single body statement
        AstAlways* ثابت procp = VN_CAST(nodep->backp(), Always);
        إذا (!procp || procp->stmtsp() != nodep || nodep->nextp()) أرجع;
        // Not VarXRef, as different refs may set different values to each hierarchy
        Ast_ڳنڍ_Var_Ref* ثابت varrefp = VN_CAST(nodep->lhsp(), VarRef);
        إذا (m_wremove && !m_params && m_doNConst && m_modp && عمليو_Const(nodep->rhsp())
            && !VN_AS(nodep->rhsp(), Const)->انگ().isFourState()
            && varrefp  // Don't do messes with BITREFs/ARRAYREFs
            && !varrefp->varp()->hasStrengthAssignment()  // Strengths are resolved in V3Tristate
            && !varrefp->varp()->valuep()  // Not already constified
            && !varrefp->varScopep()  // Not scoped (or each scope may have different initial val.)
            && !varrefp->varp()->isForced()  // Not forced (not really a constant)
        ) {
            // ASSIGNW (VARREF, const) -> INITIAL ( ASSIGN (VARREF, const) )
            UINFO(4, "constAssignW " << nodep);
            // Make a initial assignment
            Ast_ڳنڍ_اظهار* ثابت exprp = nodep->rhsp()->unlink_Fr_پٺ();
            varrefp->unlink_Fr_پٺ();
            File_سٽ* ثابت flp = nodep->fileline();
            AstInitial* ثابت newinitp = جديد AstInitial{flp, جديد Ast_سونپيو{flp, varrefp, exprp}};
            procp->replaceWith(newinitp);
            VL_DO_DANGLING(ڌڪو_Deletep(procp), procp);
            // Set the initial value right in the variable so we can constant propagate
            Ast_ڳنڍ* ثابت initvaluep = exprp->clone_وڻ(خطأ);
            varrefp->varp()->isConst(سچ);
            varrefp->varp()->valuep(initvaluep);
        }
    }
    فراغ گهمو(AstCReset* nodep) override {
        iterateChildren(nodep);
        إذا (!m_doNConst) أرجع;
        ثابت AstBasicDType* ثابت bdtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType);
        إذا (!bdtypep) أرجع;
        إذا (!bdtypep->isZeroInit()) أرجع;
        AstConst* ثابت newp = جديد AstConst{nodep->fileline(), V3_عدد{nodep, bdtypep}};
        UINFO(9, "CRESET(0) => CONST(0) " << nodep);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(Ast_Cvt_Unpack_منزل_Unpack* nodep) override {
        iterateChildren(nodep);
        AstConsArray* ثابت fromp = VN_CAST(nodep->fromp(), ConsArray);
        إذا (!fromp) أرجع;
        // Fold the outermost dimension, converting the elements for any dimension under it
        ثابت AstUnpackArrayDType* ثابت منزل_Arrayp
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت AstUnpackArrayDType* ثابت fromArrayp
            = VN_AS(fromp->dtypep()->skipRefp(), UnpackArrayDType);
        // This dimension may not be reversed, only one under it
        ثابت منطقي ابتو
            = منزل_Arrayp->declRange().ascending() != fromArrayp->declRange().ascending();
        ثابت uint64_t آخري_اشاريو = منزل_Arrayp->elementsConst() - 1;
        Ast_ڳنڍ_DType* ثابت subDTypep = منزل_Arrayp->subDTypep();
        ثابت AstUnpackArrayDType* ثابت subArrayp
            = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
        ثابت خودڪار convert = [&](Ast_ڳنڍ_اظهار* valuep) -> Ast_ڳنڍ_اظهار* {
            valuep->unlink_Fr_پٺ();
            إذا (!subArrayp || !subArrayp->is_ابتو_جو(valuep->dtypep())) أرجع valuep;
            أرجع جديد Ast_Cvt_Unpack_منزل_Unpack{valuep->fileline(), valuep, subDTypep};
        };
        AstConsArray* ثابت newp
            = جديد AstConsArray{fromp->fileline(), nodep->dtypep(),
                               fromp->defaultp() ? convert(fromp->defaultp()) : خالي_اشارو};
        لكل (ثابت Ast_Cons_Array_شي* itemp = fromp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_شي)) {
            newp->valuep(ابتو ? آخري_اشاريو - itemp->اشاريو() : itemp->اشاريو(),
                         convert(itemp->valuep()));
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        iterate(newp);  // Fold the conversions of the elements
    }
    فراغ گهمو(Ast_Cvt_Array_منزل_Array* nodep) override {
        iterateChildren(nodep);
        // Handle the case where we have a stream operation inside a cast conversion
        // Check for both StreamL and StreamR operations
        Ast_ڳنڍ_وهڪرو* streamp = خالي_اشارو;
        منطقي is_ابتو = خطأ;
        إذا (Ast_وهڪرو_L* ثابت وهڪرو_Lp = VN_CAST(nodep->fromp(), وهڪرو_L)) {
            streamp = وهڪرو_Lp;
            is_ابتو = سچ;  // StreamL reverses the operation
        } وإلا إذا (Ast_وهڪرو_R* ثابت وهڪرو_Rp = VN_CAST(nodep->fromp(), وهڪرو_R)) {
            streamp = وهڪرو_Rp;
            is_ابتو = خطأ;  // StreamR doesn't reverse the operation
        }
        إذا (streamp) {
            Ast_ڳنڍ_اظهار* srcp = streamp->lhsp();
            ثابت Ast_ڳنڍ_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            Ast_ڳنڍ_DType* ثابت dstDTypep = nodep->dtypep()->skipRefp();
            إذا (VN_IS(srcDTypep, قطار_DType) && VN_IS(dstDTypep, قطار_DType)) {
                صحيح بلاڪ_سائيز = 1;
                إذا (ثابت AstConst* ثابت constp = VN_CAST(streamp->rhsp(), Const)) {
                    بلاڪ_سائيز = constp->منزل_SInt();
                    إذا (VL_UNLIKELY(بلاڪ_سائيز <= 0)) {
                        // Not reachable due to higher level checks when parsing stream
                        // operators commented out to not fail v3error-coverage-checks.
                        // nodep->v3error("Stream block size must be positive, got " <<
                        // blockSize);
                        بلاڪ_سائيز = 1;
                    }
                }
                // Not reachable due to higher level checks when parsing stream operators
                // commented out to not fail v3error-coverage-checks.
                // else {
                //    nodep->v3error("Stream block size must be constant (got " <<
                //    streamp->rhsp()->prettyTypeName() << ")");
                // }
                صحيح src_Element_بٽ = 0;
                إذا (ثابت Ast_ڳنڍ_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                    src_Element_بٽ = elemDtp->ويڪر();
                }
                صحيح dst_Element_بٽ = 0;
                إذا (ثابت Ast_ڳنڍ_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                    dst_Element_بٽ = elemDtp->ويڪر();
                }
                streamp->unlink_Fr_پٺ();
                Ast_ڳنڍ_اظهار* newp = جديد Ast_Cvt_Array_منزل_Array{
                    srcp->fileline(), srcp->unlink_Fr_پٺ(), dstDTypep,     is_ابتو,
                    بلاڪ_سائيز,        dst_Element_بٽ,       src_Element_بٽ};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڌڪو_Deletep(streamp), streamp);
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                أرجع;
            }
        }
    }
    فراغ گهمو(AstRelease* nodep) override {
        // cppcheck-suppress constVariablePointer // children unlinked below
        إذا (AstConcat* ثابت concatp = VN_CAST(nodep->lhsp(), Concat)) {
            File_سٽ* ثابت flp = nodep->fileline();
            AstRelease* ثابت newLp = جديد AstRelease{flp, concatp->lhsp()->unlink_Fr_پٺ()};
            AstRelease* ثابت newRp = جديد AstRelease{flp, concatp->rhsp()->unlink_Fr_پٺ()};
            nodep->replaceWith(newLp);
            newLp->جوڙيو_ايندڙ_هتي(newRp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            گهمو(newLp);
            گهمو(newRp);
        }
    }

    فراغ گهمو(Ast_ڳنڍ_If* nodep) override {
        iterateChildren(nodep);
        إذا (m_doNConst) {
            إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->condp(), Const)) {
                Ast_ڳنڍ* keepp = خالي_اشارو;
                Ast_ڳنڍ* delp = خالي_اشارو;
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
                    keepp->unlink_Fr_پٺ_With_ايندڙ();
                    nodep->replaceWith(keepp);
                } وإلا {
                    nodep->unlink_Fr_پٺ();
                    deleteVarScopesUnder(nodep->thensp());
                    deleteVarScopesUnder(nodep->elsesp());
                }
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);

                // Removed branch could contain only impurity within a function and the
                // function could be purified
                VIsCached::clear_Cache_وڻ();
            } وإلا إذا (!Ast_ڳنڍ::afterCommentp(nodep->thensp())
                       && !Ast_ڳنڍ::afterCommentp(nodep->elsesp())) {
                إذا (!nodep->condp()->is_خالص()) {
                    // Condition has side effect - leave, but don't need return value
                    UINFO(4, "IF({x}) ; => STMTEXPR({x}): " << nodep);
                    nodep->fileline()->warnOff(V3_Error_ڪوڊ::IGNOREDRETURN, سچ);
                    Ast_ڳنڍ_Stmt* ثابت newp
                        = جديد Ast_Stmt_اظهار{nodep->fileline(), nodep->condp()->unlink_Fr_پٺ()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                } وإلا {
                    // Empty block, remove it
                    UINFO(4, "IF({x}) ; => ; : " << nodep);
                    VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                }
            } وإلا إذا (!Ast_ڳنڍ::afterCommentp(nodep->thensp())) {
                UINFO(4, "IF({x}) nullptr {...} => IF(NOT{x}}: " << nodep);
                Ast_ڳنڍ_اظهار* ثابت condp = nodep->condp();
                Ast_ڳنڍ* ثابت elsesp = nodep->elsesp();
                condp->unlink_Fr_پٺ_With_ايندڙ();
                elsesp->unlink_Fr_پٺ_With_ايندڙ();
                إذا (nodep->thensp()) {  // Must have been comment
                    ڌڪو_Deletep(nodep->thensp()->unlink_Fr_پٺ_With_ايندڙ());
                }
                nodep->condp(جديد AstLogNot{condp->fileline(),
                                           condp});  // LogNot, as C++ optimization also possible
                nodep->جوڙيو_Thensp(elsesp);
                nodep->branchPred(nodep->branchPred().invert());
            } وإلا إذا (((VN_IS(nodep->condp(), Not) && nodep->condp()->ويڪر() == 1)
                        || VN_IS(nodep->condp(), LogNot))
                       && nodep->thensp() && nodep->elsesp()) {
                UINFO(4, "IF(NOT {x})  => IF(x) swapped if/else" << nodep);
                Ast_ڳنڍ_اظهار* ثابت condp
                    = VN_AS(nodep->condp(), ڳنڍ_Uniop)->lhsp()->unlink_Fr_پٺ_With_ايندڙ();
                Ast_ڳنڍ* ثابت thensp = nodep->thensp()->unlink_Fr_پٺ_With_ايندڙ();
                Ast_ڳنڍ* ثابت elsesp = nodep->elsesp()->unlink_Fr_پٺ_With_ايندڙ();
                AstIf* ثابت ifp = جديد AstIf{nodep->fileline(), condp, elsesp, thensp};
                ifp->isBoundsCheck(nodep->isBoundsCheck());  // Copy bounds check info
                ifp->branchPred(nodep->branchPred().invert());
                nodep->replaceWith(ifp);
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            } وإلا إذا (if_Same_سونپيو(nodep)) {
                UINFO(4,
                      "IF({a}) ASSIGN({b},{c}) else ASSIGN({b},{d}) => ASSIGN({b}, {a}?{c}:{d})");
                Ast_ڳنڍ_سونپيو* ثابت thensp = VN_AS(nodep->thensp(), ڳنڍ_سونپيو);
                // cppcheck-suppress constVariablePointer // children unlinked below
                Ast_ڳنڍ_سونپيو* ثابت elsesp = VN_AS(nodep->elsesp(), ڳنڍ_سونپيو);
                thensp->unlink_Fr_پٺ();
                Ast_ڳنڍ_اظهار* ثابت condp = nodep->condp()->unlink_Fr_پٺ();
                Ast_ڳنڍ_اظهار* ثابت truep = thensp->rhsp()->unlink_Fr_پٺ();
                Ast_ڳنڍ_اظهار* ثابت falsep = elsesp->rhsp()->unlink_Fr_پٺ();
                thensp->rhsp(جديد AstCond{truep->fileline(), condp, truep, falsep});
                nodep->replaceWith(thensp);
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            } وإلا إذا (خطأ  // Disabled, as vpm assertions are faster
                              // without due to short-circuiting
                       && عمليو_If_If(nodep)) {
                UINFO(9, "IF({a}) IF({b}) => IF({a} && {b})");
                Ast_ڳنڍ_If* ثابت هيٺاهون_Ifp = VN_AS(nodep->thensp(), ڳنڍ_If);
                Ast_ڳنڍ_اظهار* ثابت condp = nodep->condp()->unlink_Fr_پٺ();
                Ast_ڳنڍ* ثابت هيٺاهون_Thensp = هيٺاهون_Ifp->thensp()->unlink_Fr_پٺ_With_ايندڙ();
                Ast_ڳنڍ_اظهار* ثابت هيٺاهون_Condp = هيٺاهون_Ifp->condp()->unlink_Fr_پٺ_With_ايندڙ();
                nodep->condp(جديد AstLogAnd{هيٺاهون_Ifp->fileline(), condp, هيٺاهون_Condp});
                هيٺاهون_Ifp->replaceWith(هيٺاهون_Thensp);
                VL_DO_DANGLING(ڌڪو_Deletep(هيٺاهون_Ifp), هيٺاهون_Ifp);
            } وإلا {
                // Optimizations that don't reform the IF itself
                إذا (عمليو_Bool_سرڪايو(nodep->condp())) replace_Bool_سرڪايو(nodep->condp());
                ميلاپ_If_Same_آخري(nodep);
                ميلاپ_If_Cond_Cond(nodep);
            }
        }
    }

    فراغ گهمو(AstDisplay* nodep) override {
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
            || (prevp->filep() && nodep->filep() && !prevp->filep()->same_وڻ(nodep->filep())))
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
        إذا (pformatp->exprsp() && !pformatp->exprsp()->is_خالص_And_ايندڙ()) أرجع خطأ;
        إذا (nformatp->exprsp() && !nformatp->exprsp()->is_خالص_And_ايندڙ()) أرجع خطأ;
        // Avoid huge merges
        ساكن constexpr صحيح DISPLAY_MAX_ملايو_ڊيگهائي = 500;
        إذا (pformatp->متن().ڊيگهائي() + nformatp->متن().ڊيگهائي() > DISPLAY_MAX_ملايو_ڊيگهائي)
            أرجع خطأ;
        //
        UINFO(9, "DISPLAY(SF({a})) DISPLAY(SF({b})) -> DISPLAY(SF({a}+{b}))");
        // Convert DT_DISPLAY to DT_WRITE as may allow later optimizations
        إذا (prevp->displayType() == VDisplayType::DT_DISPLAY) {
            prevp->displayType(VDisplayType::DT_WRITE);
            pformatp->متن(pformatp->متن() + "\n");
        }
        // We can't replace prev() as the edit tracking iterators will get confused.
        // So instead we edit the prev note itself.
        إذا (prevp->جوڙيو_Newline()) pformatp->متن(pformatp->متن() + "\n");
        pformatp->متن(pformatp->متن() + nformatp->متن());
        إذا (!prevp->جوڙيو_Newline() && nodep->جوڙيو_Newline()) pformatp->متن(pformatp->متن() + "\n");
        إذا (nformatp->exprsp()) pformatp->جوڙيو_Exprsp(nformatp->exprsp()->unlink_Fr_پٺ_With_ايندڙ());
        إذا (AstScopeName* ثابت scopeNamep = nformatp->scopeNamep()) {
            scopeNamep->unlink_Fr_پٺ_With_ايندڙ();
            pformatp->scopeNamep(scopeNamep);
        }
        VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
        أرجع سچ;
    }
    فراغ گهمو(AstSFormatArg* nodep) override {
        // Skip namep(): requiring its runtime lookup to be constant can reject valid
        // enum-valued constant-function calls in parameters. displayedEnum() resolves
        // the name from the folded exprp() value and enum dtype instead.
        iterate_And_ايندڙ_Null(nodep->exprp());
    }
    فراغ گهمو(AstSFormatF* nodep) override {
        // Substitute constants into displays.  The main point of this is to
        // simplify assertion methodologies which call functions with display's.
        // This eliminates a pile of wide temps, and makes the C a whole lot more readable.
        iterateChildren(nodep);
        إذا (nodep->اظهار_Format_2()) {
            // Upconvert to text-based format?
            // Similar code in V3LinkResolve::visit(AstSFormatF)
            UASSERT_OBJ(nodep->متن() == "", nodep,
                        "Non-format $sformatf should have text format == \"\"");
            إذا (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->انگ().isFromString()) {
                AstConst* ثابت fmtp = VN_AS(nodep->exprsp()->unlink_Fr_پٺ(), Const);
                nodep->متن(fmtp->انگ().منزل_String());
                nodep->اظهار_Format_2(خطأ);
                VL_DO_DANGLING(ڌڪو_Deletep(fmtp), fmtp);
            }
        }
        إذا (nodep->اظهار_Format_2()) أرجع;  // Runtime, unfortunately  (unless constify format later)
        منطقي anyconst = خطأ;
        لكل (Ast_ڳنڍ* argp = nodep->exprsp(); argp; argp = argp->nextp()) {
            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
            Ast_ڳنڍ* ثابت subargp = fargp ? fargp->exprp() : argp;
            إذا (VN_IS(subargp, Const)) {
                anyconst = سچ;
                اكسر;
            }
        }
        إذا (m_doNConst && anyconst) {
            UINFO(9, "  Display in  " << nodep->متن());
            string newFormat;
            string fmt;
            منطقي اندريون_Pct = خطأ;
            Ast_ڳنڍ* argp = nodep->exprsp();
            ثابت string متن = nodep->متن();
            لكل (حرف ch : متن) {
                إذا (!اندريون_Pct && ch == '%') {
                    اندريون_Pct = سچ;
                    fmt = ch;
                } وإلا إذا (اندريون_Pct && (std::انگ_آهي(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } وإلا إذا (اندريون_Pct) {
                    اندريون_Pct = خطأ;
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
                            Ast_ڳنڍ* ثابت nextp = argp->nextp();
                            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                            Ast_ڳنڍ* ثابت subargp = fargp ? fargp->exprp() : argp;
                            ثابت VFormatAttr formatAttr
                                = argp
                                      ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                                      : VFormatAttr{};
                            إذا (VN_IS(subargp, Const)) {  // Convert it
                                ثابت string ٻاهريون
                                    = formatAttr.isEnum()
                                          ? const_انگ_V(subargp).displayedEnum(fargp, fmt)
                                          : const_انگ_V(subargp).displayed(nodep, fmt, formatAttr);
                                UINFO(9, "     DispConst: " << fmt << " -> " << ٻاهريون << "  for "
                                                            << subargp);
                                // fmt = out w/ replace % with %% as it must later when
                                // used as a format output as literal.
                                fmt = VString::quotePercent(ٻاهريون);
                                VL_DO_DANGLING(ڌڪو_Deletep(argp->unlink_Fr_پٺ()), argp);
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
            إذا (newFormat != nodep->متن()) {
                nodep->متن(newFormat);
                UINFO(9, "  Display out " << nodep);
            }
        }
        إذا (!nodep->exprsp() && nodep->متن().ڳوليو('%') == string::npos && !nodep->hidden()) {
            // Just a simple constant string - the formatting is pointless
            UINFO(9, "replaceConstStr");
            VL_DO_DANGLING(replaceConstString(nodep, nodep->متن()), nodep);
            أرجع;
        }
        إذا (VN_IS(nodep->backp(), ڳنڍ_سونپيو)  // Can't remove under Display etc
            && nodep->متن() == "%s" && VN_IS(nodep->exprsp(), SFormatArg)
            && VN_AS(nodep->exprsp(), SFormatArg)->formatAttr().isString()
            && !nodep->exprsp()->nextp()) {
            UINFO(9, "  Display(%p, expr) -> constant expr " << nodep);
            nodep->replaceWith(VN_AS(nodep->exprsp(), SFormatArg)->exprp()->unlink_Fr_پٺ());
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;
        }
    }
    فراغ گهمو(AstSFormat* nodep) override {
        iterateChildren(nodep);
        إذا (!m_doNConst) أرجع;
        إذا (m_doNConst) {
            // If it's a constant string written to a string variable, inline it as an assignment
            AstSFormatF* ثابت fmtp = nodep->fmtp();
            Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp();
            AstBasicDType* ثابت basicp = VN_CAST(lhsp->dtypep()->skipRefp(), BasicDType);
            إذا (basicp && basicp->isString() && !fmtp->exprsp()
                && fmtp->متن().ڳوليو('%') == string::npos) {
                AstConst* ثابت strp
                    = جديد AstConst{fmtp->fileline(), AstConst::String{}, fmtp->متن()};
                lhsp->unlink_Fr_پٺ();
                nodep->replaceWith(جديد Ast_سونپيو{nodep->fileline(), lhsp, strp});
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                أرجع;
            }
        }
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        VL_RESTORER(m_underRecFunc);
        إذا (nodep->recursive()) m_underRecFunc = سچ;
        iterateChildren(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ_CCall* nodep) override {
        iterateChildren(nodep);
        m_hasJumpDelay = سچ;  // As don't analyze inside tasks for timing controls
    }
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        // Note excludes AstFuncRef as other visitor below
        iterateChildren(nodep);
        m_hasJumpDelay = سچ;  // As don't analyze inside tasks for timing controls
    }
    فراغ گهمو(AstFuncRef* nodep) override {
        گهمو(static_cast<Ast_ڳنڍ_FTask_Ref*>(nodep));
        إذا (m_params) {  // Only parameters force us to do constant function call propagation
            replaceWithSimulation(nodep);
        }
    }
    فراغ گهمو(AstArg* nodep) override {
        // replaceWithSimulation on the Arg's parent FuncRef replaces these
        iterateChildren(nodep);
    }
    فراغ گهمو(AstLoop* nodep) override {
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
                VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                أرجع;
            }
        }
        // If last statement always breaks, repalce loop with body
        إذا (Ast_ڳنڍ* lastp = nodep->stmtsp()) {
            طالما (Ast_ڳنڍ* ثابت nextp = lastp->nextp()) lastp = nextp;
            إذا (AstLoopTest* ثابت testp = VN_CAST(lastp, LoopTest)) {
                إذا (testp->condp()->isZero()) {
                    VL_DO_DANGLING2(ڌڪو_Deletep(testp->unlink_Fr_پٺ()), testp, lastp);
                    nodep->replaceWith(nodep->stmtsp()->unlink_Fr_پٺ_With_ايندڙ());
                    VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                    أرجع;
                }
            }
        }
        // Warn on infinite loop
        إذا (!m_hasLoopTest && !thisLoopHasJumpDelay) {
            nodep->v3warn(INFINITELOOP, "Infinite loop (condition always true)");
            nodep->fileline()->modifyWarnOff(V3_Error_ڪوڊ::INFINITELOOP, سچ);  // Complain once
        }
    }
    فراغ گهمو(AstLoopTest* nodep) override {
        iterateChildren(nodep);

        // If never breaks, remove
        إذا (nodep->condp()->isNeqZero()) {
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
            أرجع;
        }

        m_hasLoopTest = سچ;

        // If always breaks, subsequent statements are dead code, delete them
        إذا (nodep->condp()->isZero()) {
            Ast_ڳنڍ* ثابت nextp = nodep->nextp();
            إذا (nextp && canDeleteStatements()) {
                VL_DO_DANGLING(ڌڪو_Deletep(nextp->unlink_Fr_پٺ_With_ايندڙ()), nodep);
            }
            أرجع;
        }

        إذا (عمليو_Bool_سرڪايو(nodep->condp())) replace_Bool_سرڪايو(nodep->condp());
    }

    فراغ گهمو(AstConsArray* nodep) override { iterateChildren(nodep); }
    فراغ گهمو(AstConsStruct* nodep) override { iterateChildren(nodep); }
    فراغ گهمو(Ast_Cons_Struct_شي* nodep) override { iterateChildren(nodep); }
    فراغ گهمو(Ast_Cons_Array_شي* nodep) override { iterateChildren(nodep); }
    فراغ گهمو(AstUnbounded* nodep) override { iterateChildren(nodep); }
    // These are converted by V3Param.  Don't constify as we don't want the
    // from() VARREF to disappear, if any.
    // If output of a presel didn't get consted, chances are V3Param didn't visit properly
    فراغ گهمو(Ast_ڳنڍ_Pre_Sel*) override {}

    // Ignored, can eliminate early
    فراغ گهمو(AstSysIgnore* nodep) override {
        iterateChildren(nodep);
        إذا (m_doNConst) VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
    }

    فراغ گهمو(Ast_Stmt_اظهار* nodep) override {
        iterateChildren(nodep);
        // Malformed due to child being deleted, remove here
        إذا (!nodep->exprp()) {
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
            أرجع;
        }
        // Remove if expression is trivially pure
        إذا (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), VarRef)) {
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
            أرجع;
        }
        // Remove calls to empty functions
        إذا (ثابت AstCCall* ثابت callp = VN_CAST(nodep->exprp(), CCall)) {
            ثابت AstCFunc* ثابت funcp = callp->funcp();
            إذا (!callp->argsp() && funcp->خالي_جايون_Body()) {
                VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
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
    فراغ گهمو(AstBasicDType* nodep) override {
        iterateChildren(nodep);
        nodep->cvtRangeConst();
    }

    //-----
    // Jump elimination

    فراغ گهمو(AstJumpGo* nodep) override {
        // Any statements following the JumpGo (at this statement level) never execute, delete
        إذا (nodep->nextp() && canDeleteStatements()) {
            ڌڪو_Deletep(nodep->nextp()->unlink_Fr_پٺ_With_ايندڙ());
        }

        // JumpGo as last statement in target JumpBlock (including last in a last sub-list),
        // is a no-op, remove it.
        إذا (!nodep->nextp()) {
            لكل (Ast_ڳنڍ* abovep = nodep->abovep(); abovep; abovep = abovep->abovep()) {
                إذا (abovep == nodep->blockp()) {
                    VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                    أرجع;
                }
                // Stop if not doing expensive, or if the above node is not the last in its list,
                // ... or if it's not an 'if' TODO: it would be enough if it was not a branch.
                إذا (!m_doExpensive || abovep->nextp() || !VN_IS(abovep, If)) اكسر;
            }
        }
        // Mark JumpBlock as used
        m_استعمال_ٿيل_Jump_بلاڪون.emplace(nodep->blockp());
        m_hasJumpDelay = سچ;
    }

    فراغ گهمو(Ast_Jump_بلاڪ* nodep) override {
        iterateChildren(nodep);

        // If first statement is an AstLoopTest, pull it before the jump block
        إذا (AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            nodep->جوڙيو_هتي_This_As_ايندڙ(testp->unlink_Fr_پٺ());
        }

        // Remove if empty
        إذا (!nodep->stmtsp()) {
            UINFO(4, "JUMPLABEL => empty " << nodep);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
            أرجع;
        }

        // If no JumpGo points to this node, replace it with its body
        إذا (!m_استعمال_ٿيل_Jump_بلاڪون.ڳڻپ(nodep)) {
            UINFO(4, "JUMPLABEL => unused " << nodep);
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_پٺ_With_ايندڙ());
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        }
    }

    //-----
    // Below lines are magic expressions processed by astgen
    //  TREE_SKIP_VISIT("AstNODETYPE")    # Rename normal visit to visitGen and don't iterate
    //-----
    // clang-format off
    وڻ_SKIP_گهمو("ArraySel");
    وڻ_SKIP_گهمو("CAwait");

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
    //لڙيون
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
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        // Default: Just iterate
        إذا (m_required) {
            إذا (VN_IS(nodep, ڳنڍ_DType) || VN_IS(nodep, Range) || VN_IS(nodep, ٽڪرو_Sel)
                || VN_IS(nodep, Dot) || VN_IS(nodep, متن_2)) {
                // ignore
            } وإلا إذا (VN_IS(nodep, نمونو) || VN_IS(nodep, PatMember)) {
                // A parameter override pattern is typed later, so only fold its members
                iterateChildren(nodep);
            } وإلا إذا (AstCellRef* ثابت crp = VN_CAST(nodep, CellRef)) {
                iterate(crp->exprp());
                إذا (Ast_ڳنڍ* ثابت newp = crp->exprp()) {
                    crp->replaceWithKeepDType(newp->unlink_Fr_پٺ());
                    VL_DO_DANGLING(ڌڪو_Deletep(crp), crp);
                }
                أرجع;
            } وإلا {
                nodep->v3error("Expecting expression to be constant, but can't convert a "
                               << nodep->prettyTypeName() << " to constant.");
            }
        } وإلا {
            إذا (nodep->isTimingControl()) m_hasJumpDelay = سچ;
            // Calculate the width of this operation
            إذا (m_params && !nodep->ويڪر()) nodep = V3_ويڪر::ويڪر_Params_Edit(nodep);
            iterateChildren(nodep);
        }
    }

عام:
    // Processing Mode Enum
    تعداد Proc_گهڻائي : uint8_t {
        PROC_PARAMS_NOWARN,
        PROC_PARAMS,
        PROC_GENERATE,
        PROC_جيئرو,
        PROC_V_WARN,
        PROC_V_NOWARN,
        PROC_V_EXPENSIVE,
        PROC_CPP
    };

    // CONSTRUCTORS
    ConstVisitor(Proc_گهڻائي pmode, منطقي globalPass)
        : m_globalPass{globalPass}
        , m_concswap_نالا{globalPass ? ("__Vconcswap_" + cvt_منزل_Str(s_global_Pass_انگ++)) : ""} {
        // clang-format off
        بدل (pmode) {
        حالة PROC_PARAMS_NOWARN:  m_doV = سچ;  m_doNConst = سچ; m_params = سچ;
                                  m_required = خطأ; اكسر;
        حالة PROC_PARAMS:         m_doV = سچ;  m_doNConst = سچ; m_params = سچ;
                                  m_required = سچ; اكسر;
        حالة PROC_GENERATE:       m_doV = سچ;  m_doNConst = سچ; m_params = سچ;
                                  m_required = سچ; m_doGenerate = سچ; اكسر;
        حالة PROC_جيئرو:           اكسر;
        حالة PROC_V_WARN:         m_doV = سچ;  m_doNConst = سچ; m_warn = سچ; m_convert_Logic_منزل_ٻٽو_انگ = سچ; اكسر;
        حالة PROC_V_NOWARN:       m_doV = سچ;  m_doNConst = سچ; اكسر;
        حالة PROC_V_EXPENSIVE:    m_doV = سچ;  m_doNConst = سچ; m_doExpensive = سچ; اكسر;
        حالة PROC_CPP:            m_doV = خطأ; m_doNConst = سچ; m_doCpp = سچ; اكسر;
        افتراضي:                  v3fatalSrc("Bad case"); اكسر;
        }
        // clang-format on
    }
    ~ConstVisitor() override {
        إذا (m_doCpp) {
            إذا (m_globalPass) {
                V3Stats::جوڙيو_Stat("Optimizations, Const bit op reduction", m_stat_ٻٽو_انگ_Op_Reduction);
            } وإلا {
                V3Stats::جوڙيو_Stat_جوڙ("Optimizations, Const bit op reduction", m_stat_ٻٽو_انگ_Op_Reduction);
            }
        }
        V3Stats::جوڙيو_Stat_جوڙ("Optimizations, Cond redundant expressions", m_stat_Cond_اظهار_Redundant);
        V3Stats::جوڙيو_Stat_جوڙ("Optimizations, If cond redundant expressions",
                            m_stat_If_Cond_اظهار_Redundant);
        V3Stats::جوڙيو_Stat_جوڙ("Optimizations, Concat merges", m_stat_Concat_ملايو);
        V3Stats::جوڙيو_Stat_جوڙ("Optimizations, Member access predicate node visits",
                            m_statMemberAccessVisits);
    }

    Ast_ڳنڍ* mainAcceptEdit(Ast_ڳنڍ* nodep) {
        VIsCached::clear_Cache_وڻ();  // Avoid using any stale isPure
        // Operate starting at a random place
        أرجع iterateSubtreeReturnEdits(nodep);
    }
};

uint32_t ConstVisitor::s_global_Pass_انگ = 0;

//######################################################################
// Const class functions

//! Force this cell node's parameter list to become a constant
فراغ V3Const::constifyParamsEdit(Ast_ڳنڍ* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3_ويڪر::ويڪر_Params_Edit(nodep);
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
فراغ V3Const::constifyParamsNoWarnEdit(Ast_ڳنڍ* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3_ويڪر::ويڪر_Params_Edit(nodep);
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
فراغ V3Const::constifyGenerateParamsEdit(Ast_ڳنڍ* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE:");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized
    // yet.

    // Make sure we've sized everything first
    nodep = V3_ويڪر::ويڪر_Generate_Params_Edit(nodep);
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
        ConstVisitor visitor{ConstVisitor::PROC_V_WARN, /* globalPass: */ سچ};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("const", 0, dump_وڻ_Either_Level() >= 3);
}

فراغ V3Const::constifyCpp(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ سچ};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("const_cpp", 0, dump_وڻ_Either_Level() >= 3);
}

Ast_ڳنڍ* V3Const::constifyEdit(Ast_ڳنڍ* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_NOWARN, /* globalPass: */ خطأ};
    nodep = visitor.mainAcceptEdit(nodep);
    أرجع nodep;
}

Ast_ڳنڍ* V3Const::constifyEditCpp(Ast_ڳنڍ* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ خطأ};
    nodep = visitor.mainAcceptEdit(nodep);
    أرجع nodep;
}

فراغ V3Const::constify_All_جيئرو(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    // This only pushes constants up, doesn't make any other edits
    // IE doesn't prune dead statements, as we need to do some usability checks after this
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_جيئرو, /* globalPass: */ سچ};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("const", 0, dump_وڻ_Either_Level() >= 3);
}

فراغ V3Const::constifyAll(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ سچ};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("const", 0, dump_وڻ_Either_Level() >= 3);
}

Ast_ڳنڍ* V3Const::constifyExpensiveEdit(Ast_ڳنڍ* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ خطأ};
    nodep = visitor.mainAcceptEdit(nodep);
    أرجع nodep;
}
