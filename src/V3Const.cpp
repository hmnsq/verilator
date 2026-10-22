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

#تعریف_کړه ونه_SKIP_لیدنه(...)
#تعریف_کړه TREEOP1(...)
#تعریف_کړه TREEOPA(...)
#تعریف_کړه TREEOP(...)
#تعریف_کړه TREEOPS(...)
#تعریف_کړه TREEOPC(...)
#تعریف_کړه TREEOPV(...)

//######################################################################
// Utilities

ساكن منطقي isConst(ثابت Ast_غوټه* nodep, uint64_t v) {
    ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
    أرجع constp && constp->منزل_UQuad() == v;
}

کينډۍ <ډول_نوم T>
ساكن ډول_نوم std::enable_if<std::is_integral<T>::ارزښت, منطقي>::type isPow2(T val) {
    أرجع (val & (val - 1)) == 0;
}

ساكن صحيح شمېر_Trailing_Zeroes(uint64_t val) {
    UASSERT(val, "countTrailingZeroes argument must be non-zero");
#إذا defined(__GNUC__) && !defined(VL_NO_BUILTINS)
    أرجع __builtin_ctzll(val);
#وإلا
    صحيح بټ_2 = 0;
    val = ~val;
    طالما (val & 1) {
        ++بټ_2;
        val >>= 1;
    }
    أرجع بټ_2;
#د_که_پای
}

// Allocates a unique id for each AstNode it is applied to, held in user4.
صنف VNIdAllocator final {
    // NODE STATE
    // AstNode::user4p -> size_t. Unique id of the node (0/nullptr if not allocated yet)

    // MEMBERS
    // Claimed lazily on first use TODO: fix conflict with V3Param user4 slot
    std::unique_ptr<VNUser4_دننه_Use> m_inuser4p;
    اندازه_t m_بل_Id = 0;  // Id allocated most recently

عام:
    // Return the unique id of the given node, allocating a new one if it has none yet
    اندازه_t چلوونکی()(Ast_غوټه* nodep) {
        إذا (!m_inuser4p) m_inuser4p = std::make_unique<VNUser4_دننه_Use>();  // Claim on first use
        إذا (!nodep->user4p()) nodep->user4p(reinterpret_cast<فراغ*>(++m_بل_Id));
        أرجع reinterpret_cast<اندازه_t>(nodep->user4p());
    }
};

// This visitor can be used in the post-expanded Ast from V3Expand, where the Ast satisfies:
// - Constants are 64 bit at most (because words are accessed via AstWordSel)
// - Variables are scoped.
صنف Const_بټ_Op_ونه_Visitor final : عام VNVisitorConst {
    // TYPES

    // Holds a node to be added as a term in the reduction tree, it's equivalent op count, and a
    // bool indicating if the term is clean (0/1 value, or if the top bits might be dirty)
    کارول پایله_غړی = std::tuple<Ast_غوټه_بیان*, بینښه, منطقي>;

    صنف ونې_پاڼه_Info final {  // Leaf node (either AstConst or AstVarRef)
        // MEMBERS
        منطقي m_polarity = ریښتیا;  // Invert result due to NOT
        صحيح m_lsb = 0;  // LSB of actually used bit of m_refp->varp()
        صحيح m_msb = 0;  // MSB of actually used bit of m_refp->varp()
        صحيح m_کلمه_Idx = -1;  // -1 means AstWordSel is not used.
        AstVarRef* m_refp = تش_اشاره;  // Leaf's variable reference
        ثابت AstConst* m_constp = تش_اشاره;  // Leaf's constant

    عام:
        // CONSTRUCTORS
        ونې_پاڼه_Info() = افتراضي;
        ونې_پاڼه_Info(ثابت ونې_پاڼه_Info& other) = افتراضي;
        ونې_پاڼه_Info& چلوونکی=(ثابت ونې_پاڼه_Info& other) = افتراضي;
        ونې_پاڼه_Info(ونې_پاڼه_Info&& other) = افتراضي;
        ونې_پاڼه_Info& چلوونکی=(ونې_پاڼه_Info&& other) = افتراضي;
        څرګند ونې_پاڼه_Info(صحيح lsb)
            : m_lsb{lsb} {}

        // METHODS
        فراغ سېټ_ونې_پاڼه(AstVarRef* refp) {
            UASSERT_OBJ(!m_refp && !m_constp, refp, "Must be called just once");
            m_refp = refp;
            m_msb = refp->varp()->پلنوالی_Min() - 1;
        }
        فراغ سېټ_ونې_پاڼه(ثابت AstConst* constp) {
            UASSERT_OBJ(!m_refp && !m_constp, constp, "Must be called just once");
            m_constp = constp;
            m_msb = constp->پلنوالی_Min() - 1;
        }
        // updateBitRange(), limitBitRangeToLsb(), and polarity() must be called during ascending
        // back to the root.
        فراغ update_بټ_Range(صحيح newLsb, صحيح newMsb) {
            إذا ((m_lsb <= m_msb && newLsb > newMsb) || (m_lsb > m_msb && m_lsb < newLsb)) {
                // When the new bit range is out of m_refp, clear polarity because nodes below is
                // shifted out to zero.
                // This kind of clear may happen several times. e.g. (!(1'b1 >> 1)) >> 1
                polarity(ریښتیا);
            }
            m_lsb = newLsb;
            m_msb = newMsb;
        }
        فراغ update_بټ_Range(ثابت AstCCast* castp) {
            update_بټ_Range(m_lsb, std::min(m_msb, m_lsb + castp->پلنوالی() - 1));
        }
        فراغ update_بټ_Range(ثابت Ast_ښويدنه_R* shiftp) {
            update_بټ_Range(m_lsb + VN_AS(shiftp->rhsp(), Const)->منزل_UInt(), m_msb);
        }
        فراغ حد_بټ_Range_منزل_Lsb() { update_بټ_Range(m_lsb, std::min(m_msb, m_lsb)); }
        صحيح کلمه_Idx() ثابت { أرجع m_کلمه_Idx; }
        فراغ کلمه_Idx(صحيح i) { m_کلمه_Idx = i; }
        منطقي polarity() ثابت { أرجع m_polarity; }
        فراغ polarity(منطقي p) { m_polarity = p; }

        AstVarRef* refp() ثابت { أرجع m_refp; }
        ثابت AstConst* constp() ثابت { أرجع m_constp; }
        منطقي missing_کلمه_Sel() ثابت {
            // When V3Expand is skipped, WordSel is not inserted.
            أرجع m_refp->isWide() && m_کلمه_Idx == -1;
        }
        صحيح lsb() ثابت { أرجع m_lsb; }

        صحيح msb() ثابت { أرجع std::min(m_msb, var_پلنوالی() - 1); }
        صحيح var_پلنوالی() ثابت {
            UASSERT(m_refp, "m_refp should be set");
            ثابت صحيح پلنوالی = m_refp->varp()->پلنوالی_Min();
            إذا (!m_refp->isWide()) {
                UASSERT_OBJ(m_کلمه_Idx == -1, m_refp, "Bad word index into non-wide");
                أرجع پلنوالی;
            } وإلا {
                إذا (missing_کلمه_Sel()) أرجع پلنوالی;
                UASSERT_OBJ(m_کلمه_Idx >= 0, m_refp, "Bad word index into wide");
                ثابت صحيح بټونه_دننه_MSW = VL_BITBIT_E(پلنوالی) ? VL_BITBIT_E(پلنوالی) : VL_EDATASIZE;
                أرجع m_کلمه_Idx == m_refp->پلنوالی_ټکي() - 1 ? بټونه_دننه_MSW : VL_EDATASIZE;
            }
        }
    };

    بنية بټ_Polarity_ثبت final {  // Found bit polarity during iterate()
        ونې_پاڼه_Info m_info;  // Leaf (variable or constant) bit polarity was found on
        منطقي m_polarity = خطأ;  // Polarity the bit must have to match
        صحيح m_بټ = 0;  // Bit index within the leaf that was tested
        بټ_Polarity_ثبت(ثابت ونې_پاڼه_Info& info, منطقي pol, صحيح بټ_2)
            : m_info{info}
            , m_polarity{pol}
            , m_بټ{بټ_2} {}
        بټ_Polarity_ثبت() = افتراضي;
    };

    بنية Frozen_غوټه_Info final {  // Context when a frozen node is found
        منطقي m_polarity;  // Polarity the frozen node must match
        صحيح m_lsb;  // LSB position of the frozen node
        منطقي چلوونکی<(ثابت Frozen_غوټه_Info& other) ثابت {
            إذا (m_lsb != other.m_lsb) أرجع m_lsb < other.m_lsb;
            أرجع m_polarity < other.m_polarity;
        }
    };

    صنف Restorer final {  // Restore the original state unless disableRestore() is called
        Const_بټ_Op_ونه_Visitor& m_visitor;  // Visitor whose state is saved and restored
        ثابت اندازه_t m_polarities_اندازه;  // Saved m_visitor.m_bitPolarities size to truncate back to
        ثابت اندازه_t m_frozen_اندازه;  // Saved m_visitor.m_frozenNodes size to truncate back to
        ثابت بینښه m_ops;  // Saved m_visitor.m_ops to restore
        ثابت منطقي m_polarity;  // Saved m_visitor.m_polarity to restore
        منطقي m_restore = ریښتیا;  // Whether the destructor still needs to restore state

    عام:
        څرګند Restorer(Const_بټ_Op_ونه_Visitor& visitor)
            : m_visitor{visitor}
            , m_polarities_اندازه{visitor.m_بټ_Polarities.اندازه()}
            , m_frozen_اندازه{visitor.m_frozenNodes.اندازه()}
            , m_ops{visitor.m_ops}
            , m_polarity{visitor.m_polarity} {}
        ~Restorer() {
            UASSERT(m_visitor.m_بټ_Polarities.اندازه() >= m_polarities_اندازه,
                    "m_bitPolarities must grow monotonically");
            UASSERT(m_visitor.m_frozenNodes.اندازه() >= m_frozen_اندازه,
                    "m_frozenNodes must grow monotonically");
            إذا (m_restore) restoreNow();
        }
        فراغ disableRestore() { m_restore = خطأ; }
        فراغ restoreNow() {
            UASSERT(m_restore, "Can be called just once");
            m_visitor.m_بټ_Polarities.resize(m_polarities_اندازه);
            m_visitor.m_frozenNodes.resize(m_frozen_اندازه);
            m_visitor.m_ops = m_ops;
            m_visitor.m_polarity = m_polarity;
            m_restore = خطأ;
        }
    };
    // Collect information for each Variable to transform as below
    صنف VarInfo final {
        // MEMBERS
        صحيح m_معلوم_پایله = -1;  // -1: result is not known, 0 or 1: result of this tree
        ثابت Const_بټ_Op_ونه_Visitor* ثابت
            m_parentp;  // ConstBitOpTreeVisitor holding this VarInfo
        AstVarRef* ثابت m_refp;  // Points the variable that this VarInfo covers
        ثابت صحيح m_پلنوالی;  // Width of term this VarInfo refers to
        V3_شمېره m_بټ_Polarity;  // Coefficient of each bit

    عام:
        // METHODS
        منطقي has_Const_پایله() ثابت { أرجع m_معلوم_پایله >= 0 || m_بټ_Polarity.isAllX(); }
        // The constant result. Only valid if hasConstResult() returned true.
        منطقي get_Const_پایله() ثابت {
            // Note that this condition covers m_knownResult == -1 but m_bitPolarity.isAllX(),
            // in which case the result is 0
            أرجع m_معلوم_پایله == 1;
        }
        ثابت AstVarRef* refp() ثابت { أرجع m_refp; }
        منطقي sameVarAs(ثابت Ast_غوټه_Var_Ref* otherp) ثابت { أرجع m_refp->same_غوټه(otherp); }
        فراغ سېټ_Polarity(منطقي comp_بټ, صحيح بټ_2) {
            // Ignore if already determined a known reduction
            إذا (m_معلوم_پایله >= 0) أرجع;
            UASSERT_OBJ(بټ_2 < m_پلنوالی, m_refp,
                        "Bit index out of range: " << بټ_2 << " width: " << m_پلنوالی);
            إذا (m_بټ_Polarity.بټ_Is_X(بټ_2)) {  // The bit is not yet marked with either polarity
                m_بټ_Polarity.سېټ_بټ(بټ_2, comp_بټ);
            } وإلا {  // The bit has already been marked with some polarity
                ثابت منطقي sameFlag = m_بټ_Polarity.بټ_Is1(بټ_2) == comp_بټ;
                إذا (m_parentp->is_Xor_ونه()) {
                    UASSERT_OBJ(comp_بټ && sameFlag, m_refp, "Only true is set in Xor tree");
                    // a ^ a ^ b == b so we can ignore a
                    m_بټ_Polarity.سېټ_بټ(بټ_2, 'x');
                } وإلا {  // And, Or
                    // Can ignore this nodep as the bit is already marked with the same polarity
                    إذا (sameFlag) أرجع;  // a & a == a, b | b == b
                    // Otherwise result is constant (a & ~a == 0) or (a | ~a == 1)
                    m_معلوم_پایله = m_parentp->is_And_ونه() ? 0 : 1;
                    m_بټ_Polarity.سېټ_All_بټونه_X();  // The variable is not referred anymore
                }
            }
        }

        // Return reduction term for this VarInfo, together with the number of ops in the term,
        // and a boolean indicating if the term is clean (1-bit vs multi-bit value)
        پایله_غړی get_پایله_غړی() ثابت {
            UASSERT_OBJ(!has_Const_پایله(), m_refp, "getTerm on reduction that yields constant");
            File_کرښه* ثابت fl = m_refp->fileline();

            // Get the term we are referencing (the WordSel, if wide, otherwise just the VarRef)
            Ast_غوټه_بیان* srcp = VN_CAST(m_refp->backp(), کلمه_Sel);
            إذا (!srcp) srcp = m_refp;
            srcp = srcp->clone_ونه(خطأ);

            // Signed variables might have redundant sign bits that need masking.
            ثابت منطقي has_Redundant_Sign_بټونه
                = m_refp->varp()->dtypep()->isSigned()
                  && (m_refp->isWide() ? (m_پلنوالی != VL_EDATASIZE)
                                       : (m_پلنوالی < 8 || !isPow2(m_پلنوالی)));

            // Get the mask that selects the bits that are relevant in this term
            V3_شمېره ماسک_عدد{srcp, m_پلنوالی, 0};
            ماسک_عدد.op_بټونه_Non_XZ(m_بټ_Polarity);  // 'x' -> 0, 0->1, 1->1
            ثابت uint64_t ماسک_Val = ماسک_عدد.منزل_UQuad();
            UASSERT_OBJ(ماسک_Val != 0, m_refp,
                        "Should have been recognized as having const 0 result");

            // Parts of the return value
            Ast_غوټه_بیان* resultp = srcp;  // The tree for this term
            بینښه ops = 0;  // Number of ops in this term
            منطقي clean = خطأ;  // Whether the term is clean (has value 0 or 1)

            إذا (isPow2(ماسک_Val)) {
                // If we only want a single bit, shift it out instead of a masked compare. Shifts
                // don't go through the flags register on x86 and are hence faster. This is also
                // always fewer or same ops as mask and compare, but with shorter instructions on
                // x86.

                // Find the index of the bit we want.
                ثابت صحيح بټ_2 = شمېر_Trailing_Zeroes(ماسک_Val);
                // If we want something other than the bottom bit, shift it out
                إذا (بټ_2 != 0) {
                    resultp = جديد Ast_ښويدنه_R{fl, resultp,
                                            جديد AstConst{fl, static_cast<uint32_t>(بټ_2)}, m_پلنوالی};
                    ++ops;
                }
                // Negate it if necessary
                ثابت منطقي negate = m_بټ_Polarity.بټ_Is0(بټ_2);
                إذا (negate) {
                    resultp = جديد AstNot{fl, resultp};
                    ++ops;
                }
                // Clean if MSB of unsigned value, and not negated
                clean = (بټ_2 == m_پلنوالی - 1) && !has_Redundant_Sign_بټونه && !negate;
            } وإلا {
                // We want multiple bits. Go ahead and extract them.

                // Check if masking is required, and if so apply it
                ثابت منطقي needsMasking = ماسک_Val != VL_ماسک_Q(m_پلنوالی) || has_Redundant_Sign_بټونه;
                إذا (needsMasking) {
                    resultp = جديد AstAnd{fl, جديد AstConst{fl, ماسک_عدد}, resultp};
                    ++ops;
                }

                // Create the sub-expression for this term
                إذا (m_parentp->is_Xor_ونه()) {
                    إذا (needsMasking) {
                        // Reduce the masked term to the minimum known width,
                        // to use the smallest RedXor formula
                        ثابت صحيح پلنوالی_Min = ماسک_عدد.پلنوالی_منزل_Fit();
                        resultp->dtype_Chg_پلنوالی(پلنوالی_Min, پلنوالی_Min);
                    }
                    resultp = جديد AstRedXor{fl, resultp};
                    ++ops;
                    clean = خطأ;
                    // VL_REDXOR_* returns IData, set width accordingly to avoid unnecessary casts
                    resultp->dtype_Chg_پلنوالی(VL_IDATASIZE, 1);
                } وإلا إذا (m_parentp->is_And_ونه()) {
                    V3_شمېره comp_عدد{srcp, m_پلنوالی, 0};
                    comp_عدد.op_بټونه_One(m_بټ_Polarity);  // 'x'->0, 0->0, 1->1
                    resultp = جديد AstEq{fl, جديد AstConst{fl, comp_عدد}, resultp};
                    ++ops;
                    clean = ریښتیا;
                } وإلا {  // Or
                    V3_شمېره comp_عدد{srcp, m_پلنوالی, 0};
                    comp_عدد.op_بټونه_One(m_بټ_Polarity);  // 'x'->0, 0->0, 1->1
                    comp_عدد.opXor(V3_شمېره{comp_عدد}, ماسک_عدد);
                    resultp = جديد AstNeq{fl, جديد AstConst{fl, comp_عدد}, resultp};
                    ++ops;
                    clean = ریښتیا;
                }
            }

            أرجع پایله_غړی{resultp, ops, clean};
        }

        // CONSTRUCTORS
        VarInfo(Const_بټ_Op_ونه_Visitor* پلار, AstVarRef* refp, صحيح پلنوالی)
            : m_parentp{پلار}
            , m_refp{refp}
            , m_پلنوالی{پلنوالی}
            , m_بټ_Polarity{refp, m_پلنوالی} {
            m_بټ_Polarity.سېټ_All_بټونه_X();
        }
    };

    // MEMBERS
    منطقي m_failed = خطأ;
    منطقي m_polarity = ریښتیا;  // Flip when AstNot comes
    بینښه m_ops;  // Number of operations such as And, Or, Xor, Sel...
    صحيح m_lsb = 0;  // Current LSB
    ونې_پاڼه_Info* m_leafp = تش_اشاره;  // AstConst or AstVarRef that currently looking for
    ثابت Ast_غوټه_بیان* ثابت m_rootp;  // Root of this AST subtree

    std::ویکټور<std::pair<Ast_غوټه_بیان*, Frozen_غوټه_Info>>
        m_frozenNodes;  // Nodes that cannot be optimized
    std::ویکټور<بټ_Polarity_ثبت> m_بټ_Polarities;  // Polarity of bits found during iterate()
    std::ویکټور<std::unique_ptr<VarInfo>> m_varInfos;  // VarInfo for each variable, [0] is nullptr
    VNIdAllocator& m_ids;  // Node id allocator, owned by ConstVisitor
    // Base index of m_varInfos that points VarInfo, keyed by AstVarScope/AstVarRef id,
    // zero means not set yet.
    std::unordered_map<اندازه_t, صحيح> m_بنسټ_Idxs;

    // METHODS

    منطقي is_And_ونه() ثابت { أرجع VN_IS(m_rootp, And); }
    منطقي is_Or_ونه() ثابت { أرجع VN_IS(m_rootp, Or); }
    منطقي is_Xor_ونه() ثابت { أرجع VN_IS(m_rootp, Xor) || VN_IS(m_rootp, RedXor); }

#تعریف_کړه CONST_BITOP_RETURN_IF(cond, nodep) \
    إذا (سېټ_Failed(cond, #cond, nodep, __LINE__)) أرجع

#تعریف_کړه CONST_BITOP_سېټ_FAILED(reason, nodep) سېټ_Failed(ریښتیا, reason, nodep, __LINE__)

    منطقي سېټ_Failed(منطقي fail, ثابت حرف* reason, ثابت Ast_غوټه* nodep, صحيح کرښه) {
        إذا (fail && !m_failed) {
            UINFO(9, "cannot optimize " << m_rootp << " reason:" << reason << " called from line:"
                                        << کرښه << " when checking:" << nodep);
            // UINFOTREE(9, m_rootp, "", "root");
            m_failed = ریښتیا;
        }
        أرجع m_failed;
    }
    فراغ incrOps(ثابت Ast_غوټه* nodep, صحيح کرښه) {
        ++m_ops;
        UINFO(9, "Increment to " << m_ops << " " << nodep << " called from line " << کرښه);
    }
    VarInfo& getVarInfo(ثابت ونې_پاڼه_Info& ref) {
        UASSERT_OBJ(ref.refp(), m_rootp, "null varref in And/Or/Xor optimization");
        Ast_غوټه* nodep = ref.refp()->varScopep();
        إذا (!nodep) nodep = ref.refp()->varp();  // Not scoped
        صحيح& بنسټ_Idxr = m_بنسټ_Idxs[m_ids(nodep)];
        إذا (بنسټ_Idxr == 0) {  // Not set yet
            بنسټ_Idxr = m_varInfos.اندازه();
            ثابت صحيح عدد_ټکي
                = ref.refp()->dtypep()->isWide() ? ref.refp()->dtypep()->پلنوالی_ټکي() : 1;
            m_varInfos.resize(m_varInfos.اندازه() + عدد_ټکي);
        }
        ثابت اندازه_t idx = بنسټ_Idxr + std::max(0, ref.کلمه_Idx());
        VarInfo* varInfop = m_varInfos[idx].get();
        إذا (!varInfop) {
            varInfop = جديد VarInfo{هذا, ref.refp(), ref.var_پلنوالی()};
            m_varInfos[idx].reset(varInfop);
            إذا (ref.missing_کلمه_Sel()) {
                // ConstBitOpTreeVisitor makes some constants for masks and its type is uint64_t.
                // That's why V3Expand, that inserts WordSel, is needed.
                CONST_BITOP_سېټ_FAILED("V3Expand is skipped", ref.refp());
            }
        } وإلا {
            إذا (!varInfop->sameVarAs(ref.refp()))
                CONST_BITOP_سېټ_FAILED("different var (scope?)", ref.refp());
        }
        أرجع *varInfop;
    }

    // Traverse down to see AstConst or AstVarRef
    ونې_پاڼه_Info پیدا_کړه_ونې_پاڼه(Ast_غوټه* nodep, منطقي expectConst) {
        إذا (!nodep->dtypep()->skipRefp()->isIntegralOrPacked()) أرجع ونې_پاڼه_Info{};
        ونې_پاڼه_Info info{m_lsb};
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
        أرجع ok ? info : ونې_پاڼه_Info{};
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه* nodep) override { CONST_BITOP_سېټ_FAILED("Hit unexpected op", nodep); }
    فراغ لیدنه(AstCCast* nodep) override {
        iterateChildrenConst(nodep);
        إذا (m_leafp) m_leafp->update_بټ_Range(nodep);
    }
    فراغ لیدنه(Ast_ښويدنه_R* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ثابت AstConst* ثابت constp = VN_CAST(nodep->rhsp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->rhsp());
        m_lsb += constp->منزل_UInt();
        incrOps(nodep, __LINE__);
        iterateConst(nodep->lhsp());
        m_leafp->update_بټ_Range(nodep);
        m_lsb -= constp->منزل_UInt();
    }
    فراغ لیدنه(AstNot* nodep) override {
        CONST_BITOP_RETURN_IF(nodep->پلنوالی_Min() != 1, nodep);
        Ast_غوټه* lhsp = nodep->lhsp();
        AstCCast* ثابت castp = VN_CAST(lhsp, CCast);
        إذا (castp) lhsp = castp->lhsp();
        CONST_BITOP_RETURN_IF(!is_Xor_ونه() && !VN_IS(lhsp, VarRef) && !VN_IS(lhsp, ښويدنه_R), lhsp);
        incrOps(nodep, __LINE__);
        m_polarity = !m_polarity;
        iterateChildrenConst(nodep);
        // Don't restore m_polarity for Xor as it counts parity of the entire tree
        إذا (!is_Xor_ونه()) m_polarity = !m_polarity;
        إذا (m_leafp && castp) m_leafp->update_بټ_Range(castp);
        إذا (m_leafp) m_leafp->polarity(!m_leafp->polarity());
    }
    فراغ لیدنه(Ast_کلمه_Sel* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ثابت AstConst* ثابت constp = VN_CAST(nodep->bitp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->bitp());
        UASSERT_OBJ(m_leafp->کلمه_Idx() == -1, nodep, "Unexpected nested WordSel");
        m_leafp->کلمه_Idx(constp->منزل_SInt());
        iterateConst(nodep->fromp());
    }
    فراغ لیدنه(AstVarRef* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->سېټ_ونې_پاڼه(nodep);
    }
    فراغ لیدنه(AstConst* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->سېټ_ونې_پاڼه(nodep);
    }

    فراغ لیدنه(AstRedXor* nodep) override {
        Restorer restorer{*هذا};
        CONST_BITOP_RETURN_IF(!VN_IS(m_rootp, Xor), nodep);
        Ast_غوټه* lhsp = nodep->lhsp();
        ثابت AstCCast* ثابت castp = VN_CAST(lhsp, CCast);
        إذا (castp) lhsp = castp->lhsp();
        إذا (ثابت AstAnd* ثابت andp = VN_CAST(lhsp, And)) {  // '^(mask & leaf)'
            CONST_BITOP_RETURN_IF(!andp, lhsp);

            ثابت ونې_پاڼه_Info& ماسک = پیدا_کړه_ونې_پاڼه(andp->lhsp(), ریښتیا);
            CONST_BITOP_RETURN_IF(!ماسک.constp() || ماسک.lsb() != 0, andp->lhsp());

            ونې_پاڼه_Info ref = پیدا_کړه_ونې_پاڼه(andp->rhsp(), خطأ);
            CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());
            إذا (castp) ref.update_بټ_Range(castp);

            restorer.disableRestore();  // Now all subtree succeeded

            ثابت V3_شمېره& ماسک_عدد = ماسک.constp()->عدد();

            incrOps(nodep, __LINE__);
            incrOps(andp, __LINE__);

            // Mark all bits checked in this reduction
            ثابت صحيح max_بټ_Idx = std::min(ref.lsb() + ماسک_عدد.پلنوالی(), ref.msb() + 1);
            لكل (صحيح بټ_Idx = ref.lsb(); بټ_Idx < max_بټ_Idx; ++بټ_Idx) {
                ثابت صحيح ماسک_Idx = بټ_Idx - ref.lsb();
                إذا (ماسک_عدد.بټ_Is0(ماسک_Idx)) استمر;
                // Set true, m_polarity takes care of the entire parity
                m_بټ_Polarities.emplace_شا(ref, ریښتیا, بټ_Idx);
            }
        } وإلا {  // '^leaf'
            ونې_پاڼه_Info ref = پیدا_کړه_ونې_پاڼه(lhsp, خطأ);
            CONST_BITOP_RETURN_IF(!ref.refp(), lhsp);
            إذا (castp) ref.update_بټ_Range(castp);

            restorer.disableRestore();  // Now all checks passed

            incrOps(nodep, __LINE__);

            // Mark all bits checked by this comparison
            لكل (صحيح بټ_Idx = ref.lsb(); بټ_Idx <= ref.msb(); ++بټ_Idx) {
                m_بټ_Polarities.emplace_شا(ref, ریښتیا, بټ_Idx);
            }
        }
    }

    فراغ لیدنه(Ast_غوټه_Biop* nodep) override {
        إذا (VN_IS(nodep, And) && isConst(nodep->lhsp(), 1)) {  // 1 & _
            // Always reach past a plain making AND
            Restorer restorer{*هذا};
            incrOps(nodep, __LINE__);
            iterateConst(nodep->rhsp());
            إذا (m_leafp) m_leafp->حد_بټ_Range_منزل_Lsb();
            CONST_BITOP_RETURN_IF(m_failed, nodep->rhsp());
            restorer.disableRestore();  // Now all checks passed
        } وإلا إذا (nodep->type() == m_rootp->type()) {  // And, Or, Xor
            // subtree under NOT can be optimized only in XOR tree.
            CONST_BITOP_RETURN_IF(!m_polarity && !is_Xor_ونه(), nodep);
            incrOps(nodep, __LINE__);
            لكل (ثابت منطقي ښی : {خطأ, ریښتیا}) {
                VL_RESTORER(m_leafp);
                Restorer restorer{*هذا};
                ونې_پاڼه_Info ونې_پاڼه_Info_2{m_lsb};
                // cppcheck-suppress danglingLifetime
                m_leafp = &ونې_پاڼه_Info_2;
                Ast_غوټه_بیان* opp = ښی ? nodep->rhsp() : nodep->lhsp();
                ثابت منطقي origFailed = m_failed;
                iterateConst(opp);
                إذا (ونې_پاڼه_Info_2.constp() || m_failed) {
                    // Revert changes in leaf
                    restorer.restoreNow();
                    // Reach past a cast then add to frozen nodes to be added to final reduction
                    إذا (ثابت AstCCast* ثابت castp = VN_CAST(opp, CCast)) opp = castp->lhsp();
                    ثابت منطقي pol = is_Xor_ونه() || m_polarity;  // Only AND/OR tree needs polarity
                    UASSERT_OBJ(pol, nodep, "AND/OR tree expects m_polarity==true");
                    m_frozenNodes.emplace_شا(opp, Frozen_غوټه_Info{pol, m_lsb});
                    m_failed = origFailed;
                    استمر;
                }
                restorer.disableRestore();  // Now all checks passed
                إذا (ونې_پاڼه_Info_2.refp()) {
                    // The conditional on the lsb being in range is necessary for some degenerate
                    // case, e.g.: (IData)((QData)wide[0] >> 32), or <1-bit-var> >> 1, which is
                    // just zero
                    إذا (ونې_پاڼه_Info_2.lsb() <= ونې_پاڼه_Info_2.msb()) {
                        m_بټ_Polarities.emplace_شا(ونې_پاڼه_Info_2, is_Xor_ونه() || ونې_پاڼه_Info_2.polarity(),
                                                     ونې_پاڼه_Info_2.lsb());
                    } وإلا إذا ((is_And_ونه() && ونې_پاڼه_Info_2.polarity())
                               || (is_Or_ونه() && !ونې_پاڼه_Info_2.polarity())) {
                        // If there is a constant 0 term in an And tree or 1 term in an Or tree, we
                        // must include it. Fudge this by adding a bit with both polarities, which
                        // will simplify to zero or one respectively.
                        // Note that Xor tree does not need this kind of care, polarity of Xor tree
                        // is already cared when visitin AstNot. Taking xor with 1'b0 is nop.
                        m_بټ_Polarities.emplace_شا(ونې_پاڼه_Info_2, ریښتیا, 0);
                        m_بټ_Polarities.emplace_شا(ونې_پاڼه_Info_2, خطأ, 0);
                    }
                }
            }
        } وإلا إذا ((is_And_ونه() && VN_IS(nodep, Eq)) || (is_Or_ونه() && VN_IS(nodep, Neq))) {
            Restorer restorer{*هذا};
            CONST_BITOP_RETURN_IF(!m_polarity, nodep);
            CONST_BITOP_RETURN_IF(m_lsb, nodep);  // the result of EQ/NE is 1 bit width
            ثابت Ast_غوټه* lhsp = nodep->lhsp();
            إذا (ثابت AstCCast* ثابت castp = VN_CAST(lhsp, CCast)) lhsp = castp->lhsp();
            ثابت AstConst* ثابت constp = VN_CAST(lhsp, Const);
            CONST_BITOP_RETURN_IF(!constp, nodep->lhsp());

            ثابت V3_شمېره& comp_عدد = constp->عدد();

            خپلکاری سېټ_Polarities = [هذا, &comp_عدد](ثابت ونې_پاڼه_Info& ref, ثابت V3_شمېره* maskp) {
                ثابت منطقي ماسک_Flip = is_And_ونه() ^ ref.polarity();
                صحيح constant_پلنوالی = comp_عدد.پلنوالی();
                إذا (maskp) constant_پلنوالی = std::max(constant_پلنوالی, maskp->پلنوالی());
                ثابت صحيح max_بټ_Idx = std::max(ref.lsb() + constant_پلنوالی, ref.msb() + 1);
                // Mark all bits checked by this comparison
                لكل (صحيح بټ_Idx = ref.lsb(); بټ_Idx < max_بټ_Idx; ++بټ_Idx) {
                    ثابت صحيح ماسک_Idx = بټ_Idx - ref.lsb();
                    ثابت منطقي mask0 = maskp && maskp->بټ_Is0(ماسک_Idx);
                    ثابت منطقي بهر_د_Range = بټ_Idx > ref.msb();
                    إذا (mask0 || بهر_د_Range) {  // RHS is 0
                        إذا (comp_عدد.بټ_Is1(ماسک_Idx)) {
                            // LHS is 1
                            // And tree: 1 == 0 => always false, set v && !v
                            // Or tree : 1 != 0 => always true, set v || !v
                            m_بټ_Polarities.emplace_شا(ref, ریښتیا, 0);
                            m_بټ_Polarities.emplace_شا(ref, خطأ, 0);
                            اكسر;
                        } وإلا {  // This bitIdx is irrelevant
                            استمر;
                        }
                    }
                    ثابت منطقي polarity = comp_عدد.بټ_Is1(ماسک_Idx) != ماسک_Flip;
                    m_بټ_Polarities.emplace_شا(ref, polarity, بټ_Idx);
                }
            };

            إذا (ثابت AstAnd* ثابت andp = VN_CAST(nodep->rhsp(), And)) {  // comp == (mask & v)
                ثابت ونې_پاڼه_Info& ماسک = پیدا_کړه_ونې_پاڼه(andp->lhsp(), ریښتیا);
                CONST_BITOP_RETURN_IF(!ماسک.constp() || ماسک.lsb() != 0, andp->lhsp());

                ثابت ونې_پاڼه_Info& ref = پیدا_کړه_ونې_پاڼه(andp->rhsp(), خطأ);
                CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());

                restorer.disableRestore();  // Now all checks passed

                ثابت V3_شمېره& ماسک_عدد = ماسک.constp()->عدد();

                incrOps(nodep, __LINE__);
                incrOps(andp, __LINE__);

                سېټ_Polarities(ref, &ماسک_عدد);
            } وإلا {  // comp == v
                ثابت ونې_پاڼه_Info& ref = پیدا_کړه_ونې_پاڼه(nodep->rhsp(), خطأ);
                CONST_BITOP_RETURN_IF(!ref.refp(), nodep->rhsp());

                restorer.disableRestore();  // Now all checks passed

                incrOps(nodep, __LINE__);

                سېټ_Polarities(ref, تش_اشاره);
            }
        } وإلا {
            CONST_BITOP_سېټ_FAILED("Mixture of different ops cannot be optimized", nodep);
        }
    }

    // CONSTRUCTORS
    Const_بټ_Op_ونه_Visitor(Ast_غوټه_بیان* nodep, بینښه externalOps, VNIdAllocator& ids)
        : m_ops{externalOps}
        , m_rootp{nodep}
        , m_ids{ids} {
        // Fill nullptr at [0] because a base index of 0 means not set yet
        m_varInfos.ټېل_شا(تش_اشاره);
        CONST_BITOP_RETURN_IF(!is_And_ونه() && !is_Or_ونه() && !is_Xor_ونه(), nodep);
        إذا (Ast_غوټه_Biop* ثابت biopp = VN_CAST(nodep, غوټه_Biop)) {
            iterateConst(biopp);
        } وإلا {
            UASSERT_OBJ(VN_IS(nodep, RedXor), nodep, "Must be RedXor");
            incrOps(nodep, __LINE__);
            iterateChildrenConst(nodep);
        }
        لكل (خپلکاری&& ثبت : m_بټ_Polarities) {
            getVarInfo(ثبت.m_info).سېټ_Polarity(ثبت.m_polarity, ثبت.m_بټ);
        }
        UASSERT_OBJ(is_Xor_ونه() || m_polarity, nodep, "must be the original polarity");
    }
    مجازي ~Const_بټ_Op_ونه_Visitor() = افتراضي;
#تعریف_لغوه CONST_BITOP_RETURN_IF
#تعریف_لغوه CONST_BITOP_سېټ_FAILED

عام:
    // Transform as below.
    // v[0] & v[1] => 2'b11 == (2'b11 & v)
    // v[0] | v[1] => 2'b00 != (2'b11 & v)
    // v[0] ^ v[1] => ^{2'b11 & v}
    // (3'b011 == (3'b011 & v)) & v[2]  => 3'b111 == (3'b111 & v)
    // (3'b000 != (3'b011 & v)) | v[2]  => 3'b000 != (3'b111 & v)
    // Reduction ops are transformed in the same way.
    // &{v[0], v[1]} => 2'b11 == (2'b11 & v)
    ساكن Ast_غوټه_بیان* simplify(Ast_غوټه_بیان* nodep, صحيح پایله_پلنوالی, بینښه externalOps,
                                 VDouble0& reduction, VNIdAllocator& ids) {
        UASSERT_OBJ(1 <= پایله_پلنوالی && پایله_پلنوالی <= 64, nodep, "resultWidth out of range");

        // Walk tree, gathering all terms referenced in expression
        ثابت Const_بټ_Op_ونه_Visitor visitor{nodep, externalOps, ids};

        // If failed on root node is not optimizable, or there are no variable terms, then done
        إذا (visitor.m_failed || visitor.m_varInfos.اندازه() == 1) أرجع تش_اشاره;

        // FileLine used for constructing all new nodes in this function
        File_کرښه* ثابت fl = nodep->fileline();

        // Get partial result each term referenced, count total number of ops and keep track of
        // whether we have clean/dirty terms. visitor.m_varInfos appears in deterministic order,
        // so the optimized tree is deterministic as well.

        std::ویکټور<Ast_غوټه_بیان*> termps;
        termps.reserve(visitor.m_varInfos.اندازه() - 1);
        بینښه پایله_Ops = 0;
        منطقي has_Clean_غړی = خطأ;
        منطقي has_Dirty_غړی = خطأ;

        لكل (خپلکاری&& v : visitor.m_varInfos) {
            إذا (!v) استمر;  // Skip nullptr at m_varInfos[0]
            إذا (v->has_Const_پایله()) {
                // If a constant term is known, we can either drop it or the whole tree is constant
                Ast_غوټه_بیان* resultp = تش_اشاره;
                إذا (v->get_Const_پایله()) {
                    UASSERT_OBJ(visitor.is_Or_ونه(), nodep,
                                "Only OR tree can yield known 1 result");
                    UINFO(9, "OR tree with const 1 term: " << v->refp());
                    // Known 1 bit in OR tree, whole result is 1
                    resultp = جديد AstConst{fl, AstConst::بټ_True{}};
                } وإلا إذا (visitor.is_And_ونه()) {
                    UINFO(9, "AND tree with const 0 term: " << v->refp());
                    // Known 0 bit in AND tree, whole result is 0
                    resultp = جديد AstConst{fl, AstConst::بټ_False{}};
                } وإلا {
                    // Known 0 bit in OR or XOR tree. Ignore it.
                    استمر;
                }
                // Set width and widthMin precisely
                resultp->dtype_Chg_پلنوالی(پایله_پلنوالی, 1);
                لكل (Ast_غوټه* ثابت termp : termps) VL_DO_DANGLING(termp->delete_ونه(), termp);
                أرجع resultp;
            }
            ثابت پایله_غړی پایله = v->get_پایله_غړی();
            termps.ټېل_شا(std::get<0>(پایله));
            پایله_Ops += std::get<1>(پایله);
            إذا (std::get<2>(پایله)) {
                has_Clean_غړی = ریښتیا;
                UINFO(9, "Clean term: " << termps.شا());
            } وإلا {
                has_Dirty_غړی = ریښتیا;
                UINFO(9, "Dirty term: " << termps.شا());
            }
        }

        // Group by FrozenNodeInfo
        std::map<Frozen_غوټه_Info, std::ویکټور<Ast_غوټه_بیان*>> frozenNodes;
        // Check if frozen terms are clean or not
        لكل (ثابت خپلکاری& frozenInfo : visitor.m_frozenNodes) {
            Ast_غوټه_بیان* ثابت termp = frozenInfo.لومړی;
            // Comparison operators are clean
            إذا ((VN_IS(termp, Eq) || VN_IS(termp, Neq) || VN_IS(termp, Lt) || VN_IS(termp, Lte)
                 || VN_IS(termp, Gt) || VN_IS(termp, Gte))
                && frozenInfo.دوهم.m_lsb == 0) {
                has_Clean_غړی = ریښتیا;
            } وإلا {
                // Otherwise, conservatively assume the frozen term is dirty
                has_Dirty_غړی = ریښتیا;
                UINFO(9, "Dirty frozen term: " << termp);
            }
            frozenNodes[frozenInfo.دوهم].ټېل_شا(termp);
        }

        // Figure out if a final negation is required
        ثابت منطقي needsFlip = visitor.is_Xor_ونه() && !visitor.m_polarity;

        // Figure out if the final tree needs cleaning
        ثابت منطقي needsCleaning = visitor.is_And_ونه() ? !has_Clean_غړی : has_Dirty_غړی;

        // Add size of reduction tree to op count
        پایله_Ops += termps.اندازه() - 1;
        لكل (ثابت خپلکاری& lsbAndNodes : frozenNodes) {
            إذا (lsbAndNodes.لومړی.m_lsb > 0) ++پایله_Ops;  // Needs AstShiftR
            إذا (!lsbAndNodes.لومړی.m_polarity) ++پایله_Ops;  // Needs AstNot
            پایله_Ops += lsbAndNodes.دوهم.اندازه();
        }
        // Add final polarity flip in Xor tree
        إذا (needsFlip) ++پایله_Ops;
        // Add final cleaning AND
        إذا (needsCleaning) ++پایله_Ops;

        إذا (debug() >= 9) {  // LCOV_EXCL_START
            وتنه << "-  Bitop tree considered:\n";
            لكل (ثابت Ast_غوټه_بیان* ثابت termp : termps) termp->dump_ونه("-  Reduced term: ");
            لكل (ثابت std::pair<Ast_غوټه_بیان*, Frozen_غوټه_Info>& termp : visitor.m_frozenNodes) {
                termp.لومړی->dump_ونه("-  Frozen term with lsb "
                                      + std::منزل_string(termp.دوهم.m_lsb) + " polarity "
                                      + std::منزل_string(termp.دوهم.m_polarity) + ": ");
            }
            وتنه << "-  Needs flipping: " << needsFlip << "\n";
            وتنه << "-  Needs cleaning: " << needsCleaning << "\n";
            وتنه << "-  Size: " << پایله_Ops << " input size: " << visitor.m_ops << "\n";
        }  // LCOV_EXCL_STOP

        // Sometimes we have no terms left after ignoring redundant terms
        // (all of which were zeroes)
        إذا (termps.تشه() && visitor.m_frozenNodes.تشه()) {
            reduction += visitor.m_ops;
            Ast_غوټه_بیان* ثابت resultp = needsFlip ? جديد AstConst{fl, AstConst::بټ_True{}}
                                                   : جديد AstConst{fl, AstConst::بټ_False{}};
            resultp->dtype_Chg_پلنوالی(پایله_پلنوالی, 1);
            أرجع resultp;
        }

        // Only substitute the result if beneficial as determined by operation count
        إذا (visitor.m_ops <= پایله_Ops) {
            لكل (Ast_غوټه* ثابت termp : termps) VL_DO_DANGLING(termp->delete_ونه(), termp);
            أرجع تش_اشاره;
        }

        // Update statistics
        reduction += visitor.m_ops - پایله_Ops;

        // Reduction op to combine terms
        ثابت خپلکاری reduce = [&visitor, fl](Ast_غوټه_بیان* lhsp, Ast_غوټه_بیان* rhsp) -> Ast_غوټه_بیان* {
            إذا (!lhsp) أرجع rhsp;
            إذا (visitor.is_And_ونه()) {
                أرجع جديد AstAnd{fl, lhsp, rhsp};
            } وإلا إذا (visitor.is_Or_ونه()) {
                أرجع جديد AstOr{fl, lhsp, rhsp};
            } وإلا {
                أرجع جديد AstXor{fl, lhsp, rhsp};
            }
        };

        // Compute result by reducing all terms
        Ast_غوټه_بیان* resultp = تش_اشاره;
        لكل (Ast_غوټه_بیان* ثابت termp : termps) {  //
            resultp = reduce(resultp, termp);
        }
        // Add any frozen terms to the reduction
        لكل (خپلکاری&& nodes : frozenNodes) {
            // nodes.second has same lsb and polarity
            Ast_غوټه_بیان* termp = تش_اشاره;
            لكل (Ast_غوټه_بیان* ثابت itemp : nodes.دوهم) {
                termp = reduce(termp, itemp->unlink_Fr_شا());
            }
            إذا (nodes.لومړی.m_lsb > 0) {  // LSB is not 0, so shiftR
                Ast_غوټه_DType* ثابت dtypep = termp->dtypep();
                termp = جديد Ast_ښويدنه_R{termp->fileline(), termp,
                                      جديد AstConst(termp->fileline(), AstConst::Widthed_ارزښت{},
                                                   termp->پلنوالی(), nodes.لومړی.m_lsb)};
                termp->dtypep(dtypep);
            }
            إذا (!nodes.لومړی.m_polarity) {  // Polarity is inverted, so append Not
                Ast_غوټه_DType* ثابت dtypep = termp->dtypep();
                termp = جديد AstNot{termp->fileline(), termp};
                termp->dtypep(dtypep);
            }
            resultp = reduce(resultp, termp);
        }

        // Set width of masks to expected result width. This is required to prevent later removal
        // of the masking node e.g. by the "AND with all ones" rule. If the result width happens
        // to be 1, we still need to ensure the AstAnd is not dropped, so use a wider mask in this
        // special case.
        ثابت صحيح ماسک_پلنوالی
            = std::max(resultp->پلنوالی(), پایله_پلنوالی == 1 ? VL_IDATASIZE : پایله_پلنوالی);

        // Apply final polarity flip
        إذا (needsFlip) {
            إذا (needsCleaning) {
                // Cleaning will be added below. Use a NOT which is a byte shorter on x86
                resultp = جديد AstNot{fl, resultp};
            } وإلا {
                // Keep result clean by using XOR(1, _)
                AstConst* ثابت maskp = جديد AstConst{fl, AstConst::Widthed_ارزښت{}, ماسک_پلنوالی, 1};
                resultp = جديد AstXor{fl, maskp, resultp};
            }
        }

        // Apply final cleaning
        إذا (needsCleaning) {
            AstConst* ثابت maskp = جديد AstConst{fl, AstConst::Widthed_ارزښت{}, ماسک_پلنوالی, 1};
            resultp = جديد AstAnd{fl, maskp, resultp};
        }

        // Cast back to original size if required
        إذا (resultp->پلنوالی() != پایله_پلنوالی) {
            resultp = جديد AstCCast{fl, resultp, پایله_پلنوالی, 1};
        }

        // Set width and widthMin precisely
        resultp->dtype_Chg_پلنوالی(پایله_پلنوالی, 1);

        أرجع resultp;
    }
};

//######################################################################
// Const state, as a visitor of each AstNode

صنف ConstVisitor final : عام VNVisitor {
    // CONSTANTS
    ساكن constexpr بینښه CONCAT_MERGABLE_MAX_ژوروالی = 10;  // Limit alg recursion
    // Largest right-hand side that is copied to each part of a split concatenation assignment
    ساكن constexpr صحيح CONCAT_ټاکل_MAX_RHS_NODES = 16;

    // NODE STATE
    // See VNIdAllocator. All per node state below is held in side tables keyed on the
    // node ids it hands out, as node pointers are not stable keys within V3Const.

    // STATE
    VNIdAllocator m_ids;  // Node id allocator
    منطقي m_params = خطأ;  // If true, propagate parameterized and true numbers only
    منطقي m_required = خطأ;  // If true, must become a constant
    منطقي m_wremove = ریښتیا;  // Inside scope, no assignw removal
    منطقي m_warn = خطأ;  // Output warnings
    منطقي m_doExpensive = خطأ;  // Enable computationally expensive optimizations
    منطقي m_doCpp = خطأ;  // Enable late-stage C++ optimizations
    منطقي m_doNConst = خطأ;  // Enable non-constant-child simplifications
    منطقي m_doV = خطأ;  // Verilog, not C++ conversion
    منطقي m_doGenerate = خطأ;  // Postpone width checking inside generate
    منطقي m_convert_Logic_منزل_بټ = خطأ;  // Convert logical operators to bitwise
    منطقي m_hasJumpDelay = خطأ;  // JumpGo or Delay under this loop
    منطقي m_hasLoopTest = خطأ;  // Contains AstLoopTest
    منطقي m_underRecFunc = خطأ;  // Under a recursive function
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    ثابت AstArraySel* m_selp = تش_اشاره;  // Current select
    ثابت AstScope* m_scopep = تش_اشاره;  // Current scope
    ثابت Ast_Attr_د* m_attrp = تش_اشاره;  // Current attribute
    VDouble0 m_stat_بټ_Op_Reduction;  // Ops reduced in ConstBitOpTreeVisitor
    VDouble0 m_stat_Concat_یوځای;  // Concat merges
    VDouble0 m_stat_Cond_بیان_Redundant;  // Conditional repeated expressions
    VDouble0 m_stat_If_Cond_بیان_Redundant;  // Conditional repeated expressions
    VDouble0 m_statMemberAccessVisits;  // Nodes visited by containsMemberAccessRecurse
    ثابت منطقي m_globalPass;  // ConstVisitor invoked as a global pass
    ساكن uint32_t s_global_Pass_عدد;  // Counts number of times ConstVisitor invoked as global pass
    V3_Unique_نومونه m_concswap_نومونه;  // For generating unique temporary variable names
    // Caches results of matchBiopToBitwise, keyed on node id
    std::unordered_map<اندازه_t, منطقي> m_لري_Member_Access;
    // Items of enum currently being recursed into. Cannot use VNIdAllocator, as this runs in
    // every mode, including those invoked under a pass holding user4 (V3Param). Node pointers
    // are safe as keys here, unlike elsewhere in V3Const: folding the item value below does
    // free nodes whose addresses are then reused, but AstEnumItem is only ever constructed
    // while parsing (verilog.y and V3LinkParse), so a reused address can never become one,
    // and every key looked up is an AstEnumItem.
    std::unordered_سېټ<ثابت Ast_Enum_توکی*> m_recursing_Enum_توکي;
    std::unordered_سېټ<Ast_Jump_بلاک*> m_کارول_شوی_Jump_بلاکونه;  // JumpBlocks used by some JumpGo

    // METHODS

    // Return true if statements may be deleted. Until statics are lifted, a variable declared
    // in a statement might be referenced from elsewhere (e.g. hierarchically).
    ساكن منطقي canDeleteStatements() { أرجع v3Global.staticsLifted(); }

    فراغ deleteVarScopesUnder(Ast_غوټه* subtreep) {
        إذا (!subtreep) أرجع;
        إذا (!m_scopep) أرجع;
        std::unordered_سېټ<AstVar*> varps;
        subtreep->foreach_And_بل([&](AstVar* varp) { varps.ورننباسه(varp); });
        إذا (varps.تشه()) أرجع;
        لكل (AstVarScope *vscp = m_scopep->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            إذا (varps.پیدا_کړه(vscp->varp()) != varps.پای_ټکی()) {
                VL_DO_DANGLING(ټېل_Deletep(vscp->unlink_Fr_شا()), vscp);
            }
        }
    }

    V3_شمېره const_عدد_V(Ast_غوټه* nodep) {
        // Contract C width to V width (if needed, else just direct copy)
        // The upper zeros in the C representation can otherwise cause
        // wrong results in some operations, e.g. MulS
        ثابت V3_شمېره& numc = VN_AS(nodep, Const)->عدد();
        أرجع !numc.is_شمېره() ? numc : V3_شمېره{nodep, nodep->پلنوالی_Min_V(), numc};
    }
    V3_شمېره منزل_عدد_C(Ast_غوټه* nodep, ثابت V3_شمېره& numv) {
        // Extend V width back to C width for given node
        أرجع !numv.is_شمېره() ? numv : V3_شمېره{nodep, nodep->پلنوالی(), numv};
    }

    ساكن منطقي ښکتنی_As_Fixed_Aggregate(ثابت Ast_غوټه_DType* ثابت dtypep) {
        أرجع dtypep->isStreamableFixedAggregate() && dtypep->لري_Unpacked_Struct();
    }

    AstStructSel* newStructSel(Ast_غوټه_بیان* ثابت fromp, ثابت AstMemberDType* ثابت itemp) {
        AstStructSel* ثابت selp = جديد AstStructSel{fromp->fileline(), fromp, itemp->name()};
        selp->dtypeFrom(itemp->dtypep());
        أرجع selp;
    }

    فراغ collectFixedAggregateTerms(Ast_غوټه_بیان* ثابت fromp, std::ویکټور<Ast_غوټه_بیان*>& termps,
                                    ثابت منطقي packReal) {
        ثابت Ast_غوټه_DType* ثابت dtypep = fromp->dtypep()->skipRefp();
        إذا (ثابت AstUnpackArrayDType* ثابت unpackDtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            لكل (ثابت صحيح idx : unpackDtypep->declRange().seq_کیڼ_منزل_ښی()) {
                AstArraySel* ثابت selp
                    = جديد AstArraySel{fromp->fileline(), fromp->clone_ونه_خالص(خطأ), idx};
                collectFixedAggregateTerms(selp, termps, packReal);
            }
            VL_DO_DANGLING(ټېل_Deletep(fromp), fromp);
        } وإلا إذا (ثابت Ast_غوټه_UOr_Struct_DType* ثابت sdtypep
                   = VN_CAST(dtypep, غوټه_UOr_Struct_DType)) {
            إذا (sdtypep->packed()) {
                termps.ټېل_شا(fromp);
                أرجع;
            }
            لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                collectFixedAggregateTerms(newStructSel(fromp->clone_ونه_خالص(خطأ), itemp),
                                           termps, packReal);
            }
            VL_DO_DANGLING(ټېل_Deletep(fromp), fromp);
        } وإلا إذا (packReal && dtypep->isDouble()) {
            termps.ټېل_شا(جديد Ast_Real_منزل_بټونه{fromp->fileline(), fromp});
        } وإلا {
            termps.ټېل_شا(fromp);
        }
    }

    Ast_غوټه_بیان* packFixedAggregate(Ast_غوټه_بیان* ثابت fromp) {
        std::ویکټور<Ast_غوټه_بیان*> termps;
        collectFixedAggregateTerms(fromp, termps, ریښتیا);
        UASSERT(!termps.تشه(), "No stream terms");
        Ast_غوټه_بیان* resultp = termps[0];
        لكل (اندازه_t i = 1; i < termps.اندازه(); ++i) {
            resultp = جديد AstConcat{resultp->fileline(), resultp, termps[i]};
        }
        أرجع resultp;
    }

    فراغ replace_ټاکل_منزل_Fixed_Aggregate(Ast_غوټه_ټاکل* ثابت nodep, Ast_غوټه_بیان* ثابت dstp,
                                       Ast_غوټه_بیان* srcp) {
        ثابت صحيح dst_پلنوالی = dstp->dtypep()->پلنوالی_بهیر();
        std::ویکټور<Ast_غوټه_بیان*> termps;
        collectFixedAggregateTerms(dstp, termps, خطأ);
        صحيح src_پلنوالی = srcp->پلنوالی();
        إذا (src_پلنوالی < dst_پلنوالی) {
            AstExtend* ثابت extendp = جديد AstExtend{srcp->fileline(), srcp, dst_پلنوالی};
            srcp = جديد Ast_ښويدنه_L{
                extendp->fileline(), extendp,
                جديد AstConst{extendp->fileline(), static_cast<uint32_t>(dst_پلنوالی - src_پلنوالی)},
                dst_پلنوالی};
            src_پلنوالی = dst_پلنوالی;
        }
        Ast_غوټه_ټاکل* newp = تش_اشاره;
        صحيح دننه_واټن = 0;
        لكل (اندازه_t i = 0; i < termps.اندازه(); ++i) {
            Ast_غوټه_بیان* ثابت termp = termps[i];
            ثابت صحيح پلنوالی = termp->dtypep()->پلنوالی_بهیر();
            ثابت صحيح lsb = src_پلنوالی - دننه_واټن - پلنوالی;
            دننه_واټن += پلنوالی;
            Ast_غوټه_بیان* rhsp
                = جديد AstSel{srcp->fileline(), srcp->clone_ونه_خالص(خطأ), lsb, پلنوالی};
            إذا (termp->dtypep()->skipRefp()->isDouble()) {
                rhsp = جديد Ast_بټونه_منزل_Real_D{rhsp->fileline(), rhsp};
            }
            Ast_غوټه_ټاکل* ثابت assignp = nodep->cloneType(termp, rhsp);
            assignp->dtypeFrom(termp);
            newp = Ast_غوټه::جمع_کړه_بل(newp, assignp);
        }
        nodep->جمع_کړه_بل_دلته(newp);
        VL_DO_DANGLING(ټېل_Deletep(srcp), srcp);
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }

    منطقي عملوند_Const(ثابت Ast_غوټه* nodep) { أرجع VN_IS(nodep, Const); }
    منطقي عملوند_Asv_Const(ثابت Ast_غوټه* nodep) {
        // BIASV(CONST, BIASV(CONST,...)) -> BIASV( BIASV_CONSTED(a,b), ...)
        ثابت Ast_غوټه_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, غوټه_Bi_Com_Asv);
        إذا (!bnodep) أرجع خطأ;
        إذا (!VN_IS(bnodep->lhsp(), Const)) أرجع خطأ;
        ثابت Ast_غوټه_Bi_Com_Asv* ثابت rnodep = VN_CAST(bnodep->rhsp(), غوټه_Bi_Com_Asv);
        إذا (!rnodep) أرجع خطأ;
        إذا (rnodep->type() != bnodep->type()) أرجع خطأ;
        إذا (rnodep->پلنوالی() != bnodep->پلنوالی()) أرجع خطأ;
        إذا (rnodep->lhsp()->پلنوالی() != bnodep->lhsp()->پلنوالی()) أرجع خطأ;
        إذا (!VN_IS(rnodep->lhsp(), Const)) أرجع خطأ;
        أرجع ریښتیا;
    }
    منطقي عملوند_Asv_Same(ثابت Ast_غوټه* nodep) {
        // BIASV(SAMEa, BIASV(SAMEb,...)) -> BIASV( BIASV(SAMEa,SAMEb), ...)
        ثابت Ast_غوټه_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, غوټه_Bi_Com_Asv);
        إذا (!bnodep) أرجع خطأ;
        ثابت Ast_غوټه_Bi_Com_Asv* ثابت rnodep = VN_CAST(bnodep->rhsp(), غوټه_Bi_Com_Asv);
        إذا (!rnodep) أرجع خطأ;
        إذا (rnodep->type() != bnodep->type()) أرجع خطأ;
        إذا (rnodep->پلنوالی() != bnodep->پلنوالی()) أرجع خطأ;
        أرجع operandsSame(bnodep->lhsp(), rnodep->lhsp());
    }
    منطقي عملوند_Asv_LUp(ثابت Ast_غوټه* nodep) {
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
        ثابت Ast_غوټه_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, غوټه_Bi_Com_Asv);
        إذا (!bnodep) أرجع خطأ;
        ثابت Ast_غوټه_Bi_Com_Asv* ثابت lnodep = VN_CAST(bnodep->lhsp(), غوټه_Bi_Com_Asv);
        إذا (!lnodep) أرجع خطأ;
        إذا (lnodep->type() != bnodep->type()) أرجع خطأ;
        إذا (lnodep->پلنوالی() != bnodep->پلنوالی()) أرجع خطأ;
        أرجع VN_IS(lnodep->lhsp(), Const);
    }
    منطقي عملوند_Asv_RUp(ثابت Ast_غوټه* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr)) ?
        ثابت Ast_غوټه_Bi_Com_Asv* ثابت bnodep = VN_CAST(nodep, غوټه_Bi_Com_Asv);
        إذا (!bnodep) أرجع خطأ;
        ثابت Ast_غوټه_Bi_Com_Asv* ثابت rnodep = VN_CAST(bnodep->rhsp(), غوټه_Bi_Com_Asv);
        إذا (!rnodep) أرجع خطأ;
        إذا (rnodep->type() != bnodep->type()) أرجع خطأ;
        إذا (rnodep->پلنوالی() != bnodep->پلنوالی()) أرجع خطأ;
        أرجع VN_IS(rnodep->lhsp(), Const);
    }
    ساكن منطقي عملوند_Sub_جمع_کړه(ثابت Ast_غوټه* nodep) {
        // SUB( ADD(CONSTx,y), CONSTz) -> ADD(SUB(CONSTx,CONSTz), y)
        ثابت Ast_غوټه_Biop* ثابت np = VN_CAST(nodep, غوټه_Biop);
        ثابت Ast_غوټه_Biop* ثابت lp = VN_CAST(np->lhsp(), غوټه_Biop);
        أرجع (lp && VN_IS(lp->lhsp(), Const) && VN_IS(np->rhsp(), Const)
                && lp->پلنوالی() == np->پلنوالی());
    }
    منطقي سمون_Redundant_Clean(AstAnd* andp) {
        // Remove And with constant one inserted by V3Clean
        // 1 & (a == b)  -> (IData)(a == b)
        // When bool is casted to int, the value is either 0 or 1
        AstConst* ثابت constp = VN_AS(andp->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isOne(), andp->lhsp(), "TRREEOPC must meet this condition");
        Ast_غوټه_بیان* ثابت rhsp = andp->rhsp();
        AstCCast* ccastp = تش_اشاره;
        ثابت خپلکاری isEqOrNeq = [](ثابت Ast_غوټه* nodep) -> منطقي {  //
            أرجع VN_IS(nodep, Eq) || VN_IS(nodep, Neq);
        };
        إذا (isEqOrNeq(rhsp)) {
            ccastp = جديد AstCCast{andp->fileline(), rhsp->unlink_Fr_شا(), andp};
        } وإلا إذا (AstCCast* ثابت tmpp = VN_CAST(rhsp, CCast)) {
            إذا (isEqOrNeq(tmpp->lhsp())) {
                إذا (tmpp->پلنوالی() == andp->پلنوالی()) {
                    tmpp->unlink_Fr_شا();
                    ccastp = tmpp;
                } وإلا {
                    ccastp = جديد AstCCast{andp->fileline(), tmpp->lhsp()->unlink_Fr_شا(), andp};
                }
            }
        }
        إذا (ccastp) {
            andp->replaceWithKeepDType(ccastp);
            VL_DO_DANGLING(ټېل_Deletep(andp), andp);
            أرجع ریښتیا;
        }
        أرجع خطأ;
    }

    ساكن منطقي عملوند_And_Or_Same(ثابت Ast_غوټه* nodep) {
        // OR( AND(VAL,x), AND(VAL,y)) -> AND(VAL,OR(x,y))
        // OR( AND(x,VAL), AND(y,VAL)) -> AND(OR(x,y),VAL)
        ثابت Ast_غوټه_Biop* ثابت np = VN_CAST(nodep, غوټه_Biop);
        ثابت Ast_غوټه_Biop* ثابت lp = VN_CAST(np->lhsp(), غوټه_Biop);
        ثابت Ast_غوټه_Biop* ثابت rp = VN_CAST(np->rhsp(), غوټه_Biop);
        أرجع (lp && rp && lp->پلنوالی() == rp->پلنوالی() && lp->type() == rp->type()
                && (operandsSame(lp->lhsp(), rp->lhsp()) || operandsSame(lp->rhsp(), rp->rhsp())));
    }
    منطقي سمون_Or_And_Not(Ast_غوټه_Biop* nodep) {
        // AstOr{$a, AstAnd{AstNot{$b}, $c}} if $a.width1, $a==$b => AstOr{$a,$c}
        // Someday we'll sort the biops completely and this can be simplified
        // This often results from our simplified clock generation:
        // if (rst) ... else if (enable)... -> OR(rst,AND(!rst,enable))
        ثابت Ast_غوټه_بیان* ap;
        Ast_غوټه_Biop* andp;
        إذا (VN_IS(nodep->lhsp(), And)) {
            andp = VN_AS(nodep->lhsp(), And);
            ap = nodep->rhsp();
        } وإلا إذا (VN_IS(nodep->rhsp(), And)) {
            andp = VN_AS(nodep->rhsp(), And);
            ap = nodep->lhsp();
        } وإلا {
            أرجع خطأ;
        }
        ثابت Ast_غوټه_Uniop* notp;
        Ast_غوټه_بیان* cp;
        إذا (VN_IS(andp->lhsp(), Not)) {
            notp = VN_AS(andp->lhsp(), Not);
            cp = andp->rhsp();
        } وإلا إذا (VN_IS(andp->rhsp(), Not)) {
            notp = VN_AS(andp->rhsp(), Not);
            cp = andp->lhsp();
        } وإلا {
            أرجع خطأ;
        }
        ثابت Ast_غوټه_بیان* ثابت bp = notp->lhsp();
        إذا (!operandsSame(ap, bp)) أرجع خطأ;
        // Do it
        cp->unlink_Fr_شا();
        VL_DO_DANGLING(ټېل_Deletep(andp->unlink_Fr_شا()), andp);
        VL_DANGLING(notp);
        // Replace whichever branch is now dangling
        إذا (nodep->rhsp()) {
            nodep->lhsp(cp);
        } وإلا {
            nodep->rhsp(cp);
        }
        أرجع ریښتیا;
    }
    منطقي سمون_And_Cond(AstAnd* nodep) {
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
            condp->fileline(), condp->condp()->unlink_Fr_شا(),
            جديد AstAnd{nodep->fileline(), maskp->clone_ونه(خطأ), condp->thenp()->unlink_Fr_شا()},
            جديد AstAnd{nodep->fileline(), maskp->clone_ونه(خطأ),
                       condp->elsep()->unlink_Fr_شا()}};
        newp->thenp()->dtypeFrom(nodep);  // As And might have been to change widths
        newp->elsep()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        أرجع ریښتیا;
    }

    منطقي سمون_Cond_Cond(ثابت AstCond* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        إذا (VN_IS(nodep->backp(), Cond)) أرجع خطأ;  // Was checked when visited parent
        إذا (!VN_IS(nodep->thenp(), Cond) && !VN_IS(nodep->elsep(), Cond))
            أرجع خطأ;  // Short circuit
        std::ویکټور<Ast_غوټه_بیان*> truesp;
        std::ویکټور<Ast_غوټه_بیان*> falsesp;
        سمون_Cond_Cond_Recurse(nodep, truesp /*ref*/, falsesp /*ref*/);
        أرجع خطأ;  // Can optimize further
    }
    فراغ سمون_Cond_Cond_Recurse(ثابت AstCond* nodep, std::ویکټور<Ast_غوټه_بیان*>& truesp,
                              std::ویکټور<Ast_غوټه_بیان*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        إذا (truesp.اندازه() > 4 || falsesp.اندازه() > 4) أرجع;
        إذا (!nodep->condp()->is_خالص()) أرجع;
        منطقي replaced = خطأ;
        لكل (ثابت Ast_غوټه_بیان* condp : truesp) {
            إذا (replaced) اكسر;
            إذا (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_عدد(nodep->condp(), 1);
            replaced = ریښتیا;
            ++m_stat_Cond_بیان_Redundant;
        }
        لكل (ثابت Ast_غوټه_بیان* condp : falsesp) {
            إذا (replaced) اكسر;
            إذا (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = ریښتیا;
            ++m_stat_Cond_بیان_Redundant;
        }
        إذا (ثابت AstCond* subCondp = VN_CAST(nodep->thenp(), Cond)) {
            إذا (!replaced) truesp.emplace_شا(nodep->condp());
            سمون_Cond_Cond_Recurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            إذا (!replaced) truesp.راوباسه_شا();
        }
        إذا (ثابت AstCond* subCondp = VN_CAST(nodep->elsep(), Cond)) {
            إذا (!replaced) falsesp.emplace_شا(nodep->condp());
            سمون_Cond_Cond_Recurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            إذا (!replaced) falsesp.راوباسه_شا();
        }
    }
    فراغ سمون_If_Cond_Cond(ثابت Ast_غوټه_If* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        إذا (VN_IS(nodep->backp(), If)) أرجع;  // Was checked when visited parent
        إذا (!VN_IS(nodep->thensp(), If) && !VN_IS(nodep->elsesp(), If)) أرجع;  // Short circuit
        std::ویکټور<Ast_غوټه_بیان*> truesp;
        std::ویکټور<Ast_غوټه_بیان*> falsesp;
        سمون_If_Cond_Cond_Recurse(nodep, truesp /*ref*/, falsesp /*ref*/);
    }
    فراغ سمون_If_Cond_Cond_Recurse(ثابت Ast_غوټه_If* nodep, std::ویکټور<Ast_غوټه_بیان*>& truesp,
                                std::ویکټور<Ast_غوټه_بیان*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        إذا (truesp.اندازه() > 4 || falsesp.اندازه() > 4) أرجع;
        إذا (!nodep->condp()->is_خالص()) أرجع;
        // A constant condition is folded by visit(AstNodeIf), once statements can be deleted
        إذا (VN_IS(nodep->condp(), Const)) أرجع;
        منطقي replaced = خطأ;
        لكل (ثابت Ast_غوټه_بیان* condp : truesp) {
            إذا (replaced) اكسر;
            إذا (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_عدد(nodep->condp(), 1);
            replaced = ریښتیا;
            ++m_stat_If_Cond_بیان_Redundant;
        }
        لكل (ثابت Ast_غوټه_بیان* condp : falsesp) {
            إذا (replaced) اكسر;
            إذا (!operandsSame(nodep->condp(), condp)) استمر;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = ریښتیا;
            ++m_stat_If_Cond_بیان_Redundant;
        }
        // We only check the first statement of parent IF is an If
        // So we don't need to check for effects in the executing thensp/elsesp
        // altering the child't condition.  e.g. 'if (x) begin x=1; if (x) end'
        إذا (ثابت Ast_غوټه_If* subIfp = VN_CAST(nodep->thensp(), غوټه_If)) {
            إذا (!replaced) truesp.emplace_شا(nodep->condp());
            سمون_If_Cond_Cond_Recurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            إذا (!replaced) truesp.راوباسه_شا();
        }
        إذا (ثابت Ast_غوټه_If* subIfp = VN_CAST(nodep->elsesp(), غوټه_If)) {
            إذا (!replaced) falsesp.emplace_شا(nodep->condp());
            سمون_If_Cond_Cond_Recurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            إذا (!replaced) falsesp.راوباسه_شا();
        }
    }

    فراغ سمون_If_Same_وروستی(Ast_غوټه_If* nodep) {
        إذا (!nodep->thensp()) أرجع;
        إذا (!nodep->elsesp()) أرجع;
        إذا (!canDeleteStatements()) أرجع;
        // Iterate bodies in reverse
        Ast_غوټه* tp = nodep->thensp()->lastp();
        Ast_غوټه* ep = nodep->elsesp()->lastp();
        طالما (tp && ep && tp->same_ونه(ep)) {
            UASSERT_OBJ(!tp->nextp(), tp, "Expected no nextp");
            UASSERT_OBJ(!ep->nextp(), ep, "Expected no nextp");
            Ast_غوټه* ثابت tPrevp = tp->prevp();
            Ast_غوټه* ثابت ePrevp = ep->prevp();
            nodep->جمع_کړه_بل_دلته(tp->unlink_Fr_شا());
            VL_DO_DANGLING(ټېل_Deletep(ep->unlink_Fr_شا()), ep);
            tp = tPrevp;
            ep = ePrevp;
        }
    }

    منطقي سمون_Masked_Or(AstAnd* nodep) {
        // Masking an OR with terms that have no bits set under the mask is replaced with masking
        // only the remaining terms. Canonical example as generated by V3Expand is:
        // 0xff & (a << 8 | b >> 24) --> 0xff & (b >> 24)

        // Compute how many significant bits are in the mask
        ثابت AstConst* ثابت constp = VN_AS(nodep->lhsp(), Const);
        ثابت uint32_t significant_بټونه = constp->عدد().پلنوالی_منزل_Fit();

        AstOr* ثابت orp = VN_AS(nodep->rhsp(), Or);

        // Predicate for checking whether the bottom 'significantBits' bits of the given expression
        // are all zeroes.
        ثابت خپلکاری checkBottomClear = [=](ثابت Ast_غوټه* nodep) -> منطقي {
            إذا (ثابت Ast_ښويدنه_L* ثابت shiftp = VN_CAST(nodep, ښويدنه_L)) {
                إذا (ثابت AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                    أرجع scp->عدد().منزل_UInt() >= significant_بټونه;
                }
            }
            أرجع خطأ;
        };

        ثابت منطقي orLIsRedundant = checkBottomClear(orp->lhsp());
        ثابت منطقي orRIsRedundant = checkBottomClear(orp->rhsp());

        إذا (orLIsRedundant && orRIsRedundant) {
            nodep->replaceWithKeepDType(
                جديد AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع ریښتیا;
        } وإلا إذا (orLIsRedundant) {
            orp->replaceWithKeepDType(orp->rhsp()->unlink_Fr_شا());
            VL_DO_DANGLING(ټېل_Deletep(orp), orp);
            أرجع خطأ;  // input node is still valid, keep going
        } وإلا إذا (orRIsRedundant) {
            orp->replaceWithKeepDType(orp->lhsp()->unlink_Fr_شا());
            VL_DO_DANGLING(ټېل_Deletep(orp), orp);
            أرجع خطأ;  // input node is still valid, keep going
        } وإلا {
            أرجع خطأ;
        }
    }
    منطقي سمون_Masked_ښويدنه(AstAnd* nodep) {
        // Drop redundant masking of right shift result. E.g: 0xff & ((uint32_t)a >> 24). This
        // commonly appears after V3Expand and the simplification in matchMaskedOr. Similarly,
        // drop redundant masking of left shift result. E.g.: 0xff000000 & ((uint32_t)a << 24).

        ثابت خپلکاری check_ماسک = [nodep, هذا](ثابت V3_شمېره& ماسک) -> منطقي {
            ثابت AstConst* ثابت constp = VN_AS(nodep->lhsp(), Const);
            إذا (constp->عدد().isCaseEq(ماسک)) {
                Ast_غوټه* ثابت rhsp = nodep->rhsp();
                rhsp->unlink_Fr_شا();
                nodep->replaceWithKeepDType(rhsp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع ریښتیا;
            }
            أرجع خطأ;
        };

        // Check if masking is redundant
        إذا (ثابت Ast_ښويدنه_R* ثابت shiftp = VN_CAST(nodep->rhsp(), ښويدنه_R)) {
            إذا (ثابت AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_شمېره ماسک_Lo{nodep, nodep->پلنوالی()};
                ماسک_Lo.سېټ_ماسک(nodep->پلنوالی() - scp->عدد().منزل_UInt());
                أرجع check_ماسک(ماسک_Lo);
            }
        } وإلا إذا (ثابت Ast_ښويدنه_L* ثابت shiftp = VN_CAST(nodep->rhsp(), ښويدنه_L)) {
            إذا (ثابت AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_شمېره ماسک{nodep, nodep->پلنوالی()};
                ثابت uint32_t ښويدنه_Amount = scp->عدد().منزل_UInt();
                ماسک.سېټ_ماسک(nodep->پلنوالی() - ښويدنه_Amount, ښويدنه_Amount);
                أرجع check_ماسک(ماسک);
            }
        }
        أرجع خطأ;
    }

    منطقي سمون_Masked_Zero(ثابت AstAnd* nodep) {
        // Turn masking of known zero bits into constant zero. Commonly appears after V3Expand.
        ثابت AstConst* ثابت maskp = VN_AS(nodep->lhsp(), Const);
        ثابت Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp();
        ثابت uint32_t msbP1 = maskp->عدد().most_سېټ_بټ_P1();
        إذا (!msbP1) أرجع خطأ;  // Don't rewrite, separate rule matches for this
        ثابت uint32_t msb = msbP1 - 1;
        ثابت uint32_t lsb = maskp->عدد().لږترلږه_سېټ_بټ_P1() - 1;

        إذا (ثابت Ast_ښويدنه_L* ثابت shiftp = VN_CAST(rhsp, ښويدنه_L)) {
            // 'a << S' forces the low S bits to zero
            إذا (AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                أرجع scp->عدد().ځاییږي_دننه_UInt()  //
                       && (scp->عدد().منزل_UInt() > msb);
            }
        }
        إذا (ثابت Ast_ښويدنه_R* ثابت shiftp = VN_CAST(rhsp, ښويدنه_R)) {
            // 'a >> S' forces the high S bits to zero. Check against the width of the shifted
            // operand, V3Expand can create shifts wider than their inputs. Must use width(),
            // not widthMin(), as bits above widthMin() are not guaranteed to be zero.
            إذا (AstConst* ثابت scp = VN_CAST(shiftp->rhsp(), Const)) {
                أرجع scp->عدد().ځاییږي_دننه_UInt()
                       && (lsb + scp->عدد().منزل_UInt()
                           >= static_cast<uint32_t>(shiftp->lhsp()->پلنوالی()));
            }
        }
        إذا (ثابت AstMul* ثابت mulp = VN_CAST(rhsp, Mul)) {
            // 'C * a' forces the low N bits to zero where 'C' has low zero bits
            إذا (AstConst* ثابت cp = VN_CAST(mulp->lhsp(), Const)) {
                أرجع cp->عدد().لږترلږه_سېټ_بټ_P1() > msb + 1;
            }
        }
        إذا (ثابت AstExtend* ثابت extendp = VN_CAST(rhsp, Extend)) {
            // Zero-extension forces the bits above the source width to zero
            أرجع lsb >= static_cast<uint32_t>(extendp->lhsp()->پلنوالی());
        }

        أرجع خطأ;
    }

    منطقي سمون_بټ_Op_ونه(Ast_غوټه_بیان* nodep) {
        إذا (nodep->پلنوالی_Min() != 1) أرجع خطأ;
        إذا (!v3Global.opt.f_Const_بټ_Op_ونه()) أرجع خطأ;

        ثابت صحيح پلنوالی = nodep->پلنوالی();
        Ast_غوټه_بیان* rootp = nodep;
        بینښه externalOps = 0;
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

        std::string debug_مختاړی;
        إذا (debug() >= 9) {  // LCOV_EXCL_START
            ساكن صحيح s_c = 0;
            debug_مختاړی = "-  matchBitOpTree[" + std::منزل_string(++s_c) + "] ";
            nodep->dump_ونه(debug_مختاړی + "INPUT: ");
        }  // LCOV_EXCL_STOP

        Ast_غوټه_بیان* ثابت newp = Const_بټ_Op_ونه_Visitor::simplify(rootp, پلنوالی, externalOps,
                                                                  m_stat_بټ_Op_Reduction, m_ids);

        إذا (newp) {
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Transformed leaf of bit tree to " << newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }

        إذا (debug() >= 9) {  // LCOV_EXCL_START
            إذا (newp) {
                newp->dump_ونه(debug_مختاړی + "RESULT: ");
            } وإلا {
                وتنه << debug_مختاړی << "not replaced\n";
            }
        }  // LCOV_EXCL_STOP

        أرجع newp;
    }
    ساكن منطقي عملوند_ښويدنه_Same(ثابت Ast_غوټه* nodep) {
        ثابت Ast_غوټه_Biop* ثابت np = VN_AS(nodep, غوټه_Biop);
        {
            ثابت Ast_ښويدنه_L* ثابت lp = VN_CAST(np->lhsp(), ښويدنه_L);
            ثابت Ast_ښويدنه_L* ثابت rp = VN_CAST(np->rhsp(), ښويدنه_L);
            إذا (lp && rp) {
                أرجع (lp->پلنوالی() == rp->پلنوالی() && lp->lhsp()->پلنوالی() == rp->lhsp()->پلنوالی()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        {
            ثابت Ast_ښويدنه_R* ثابت lp = VN_CAST(np->lhsp(), ښويدنه_R);
            ثابت Ast_ښويدنه_R* ثابت rp = VN_CAST(np->rhsp(), ښويدنه_R);
            إذا (lp && rp) {
                أرجع (lp->پلنوالی() == rp->پلنوالی() && lp->lhsp()->پلنوالی() == rp->lhsp()->پلنوالی()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        أرجع خطأ;
    }
    منطقي عملوند_Huge_ښويدنه_L(ثابت Ast_غوټه_Biop* nodep) {
        أرجع (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->عدد().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->عدد().ځاییږي_دننه_UInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->منزل_UInt()
                        >= static_cast<uint32_t>(nodep->پلنوالی())))
                && nodep->lhsp()->is_خالص());
    }
    منطقي عملوند_Huge_ښويدنه_R(ثابت Ast_غوټه_Biop* nodep) {
        أرجع (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->عدد().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->عدد().ځاییږي_دننه_UInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->منزل_UInt()
                        >= static_cast<uint32_t>(nodep->lhsp()->پلنوالی())))
                && nodep->lhsp()->is_خالص()
                && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), بهیر_DType)));
    }
    منطقي عملوند_Is_Two(ثابت Ast_غوټه* nodep) {
        ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
        إذا (!constp) أرجع خطأ;  // not constant
        إذا (constp->عدد().isFourState()) أرجع خطأ;  // four-state
        إذا (nodep->پلنوالی() > VL_QUADSIZE) أرجع خطأ;  // too wide
        إذا (nodep->isSigned() && constp->عدد().isNegative()) أرجع خطأ;  // signed and negative
        أرجع constp->منزل_UQuad() == 2;
    }
    منطقي عملوند_Is_Twostate(ثابت Ast_غوټه* nodep) {
        أرجع (VN_IS(nodep, Const) && !VN_AS(nodep, Const)->عدد().isFourState());
    }
    منطقي عملوند_Is_Pow_Two(ثابت Ast_غوټه* nodep) {
        إذا (!عملوند_Is_Twostate(nodep)) أرجع خطأ;
        أرجع (1 == VN_AS(nodep, Const)->عدد().شمېر_Ones());
    }
    منطقي عملوند_ښويدنه_Op(ثابت Ast_غوټه_Biop* nodep) {
        إذا (!VN_IS(nodep->rhsp(), Const)) أرجع خطأ;
        ثابت Ast_غوټه_Biop* ثابت lhsp = VN_CAST(nodep->lhsp(), غوټه_Biop);
        إذا (!lhsp || !(VN_IS(lhsp, And) || VN_IS(lhsp, Or) || VN_IS(lhsp, Xor))) أرجع خطأ;
        إذا (nodep->پلنوالی() != lhsp->پلنوالی()) أرجع خطأ;
        إذا (nodep->پلنوالی() != lhsp->lhsp()->پلنوالی()) أرجع خطأ;
        إذا (nodep->پلنوالی() != lhsp->rhsp()->پلنوالی()) أرجع خطأ;

        // Only do it if we can fruther reduce one of the sides if pulling the shift through:
        // - Either operands are constants
        إذا (VN_IS(lhsp->lhsp(), Const)) أرجع ریښتیا;
        إذا (VN_IS(lhsp->rhsp(), Const)) أرجع ریښتیا;
        // - Const shift on either side
        إذا (Ast_غوټه_Biop* ثابت llp = VN_CAST(lhsp->lhsp(), غوټه_Biop)) {
            أرجع (VN_IS(llp, ښويدنه_L) || VN_IS(llp, ښويدنه_R)) && VN_IS(llp->rhsp(), Const);
        }
        إذا (Ast_غوټه_Biop* ثابت lrp = VN_CAST(lhsp->rhsp(), غوټه_Biop)) {
            أرجع (VN_IS(lrp, ښويدنه_L) || VN_IS(lrp, ښويدنه_R)) && VN_IS(lrp->rhsp(), Const);
        }
        // Otherwise we would increase logic size
        أرجع خطأ;
    }
    منطقي عملوند_ښويدنه_ښويدنه(ثابت Ast_غوټه_Biop* nodep) {
        // We could add a AND though.
        ثابت Ast_غوټه_Biop* ثابت lhsp = VN_CAST(nodep->lhsp(), غوټه_Biop);
        إذا (!lhsp || !(VN_IS(lhsp, ښويدنه_L) || VN_IS(lhsp, ښويدنه_R))) أرجع خطأ;
        // We can only get rid of a<<b>>c or a<<b<<c, with constant b & c
        // because bits may be masked in that process, or (b+c) may exceed the word width.
        إذا (!(VN_IS(nodep->rhsp(), Const) && VN_IS(lhsp->rhsp(), Const))) أرجع خطأ;
        إذا (VN_AS(nodep->rhsp(), Const)->عدد().isFourState()
            || VN_AS(lhsp->rhsp(), Const)->عدد().isFourState())
            أرجع خطأ;
        إذا (nodep->پلنوالی() != lhsp->پلنوالی()) أرجع خطأ;
        إذا (nodep->پلنوالی() != lhsp->lhsp()->پلنوالی()) أرجع خطأ;
        أرجع ریښتیا;
    }
    منطقي عملوند_کلمه_OOB(ثابت Ast_کلمه_Sel* nodep) {
        // V3Expand may make a arraysel that exceeds the bounds of the array
        // It was an expression, then got constified.  In reality, the WordSel
        // must be wrapped in a Cond, that will be false.
        أرجع (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), غوټه_Var_Ref)
                && VN_AS(nodep->fromp(), غوټه_Var_Ref)->access().isReadOnly()
                && (static_cast<صحيح>(VN_AS(nodep->bitp(), Const)->منزل_UInt())
                    >= VN_AS(nodep->fromp(), غوټه_Var_Ref)->varp()->پلنوالی_ټکي()));
    }
    منطقي عملوند_Sel_ډک(ثابت AstSel* nodep) {
        أرجع (VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0
                && static_cast<صحيح>(nodep->پلنوالی_Const()) == nodep->fromp()->پلنوالی());
    }
    منطقي عملوند_Sel_Extend(AstSel* nodep) {
        إذا (!m_doV) أرجع خطأ;
        AstExtend* ثابت extendp = VN_CAST(nodep->fromp(), Extend);
        إذا (!extendp) أرجع خطأ;
        AstConst* ثابت lsbp = VN_CAST(nodep->lsbp(), Const);
        إذا (!lsbp) أرجع خطأ;
        ثابت صحيح پلنوالی = nodep->پلنوالی_Const();
        ثابت صحيح lsb = lsbp->منزل_SInt();
        ثابت صحيح msb = lsb + پلنوالی - 1;
        Ast_غوټه_بیان* ثابت lhsp = extendp->lhsp();
        إذا (!lhsp->is_خالص()) أرجع خطأ;

        // Selecting the entire extended expression, replace with that
        إذا (lsb == 0 && msb == lhsp->پلنوالی() - 1) {
            lhsp->unlink_Fr_شا();
            nodep->replaceWithKeepDType(lhsp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع ریښتیا;
        }
        // Select entirely in the extended part - replace with zero
        إذا (lsb >= lhsp->پلنوالی()) {
            replaceZero(nodep);
            أرجع ریښتیا;
        }
        // Select entirely in the extended expression - replace with select from that
        إذا (msb < lhsp->پلنوالی()) {
            lhsp->unlink_Fr_شا();
            lsbp->unlink_Fr_شا();
            nodep->replaceWithKeepDType(جديد AstSel{nodep->fileline(), lhsp, lsbp, پلنوالی});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع ریښتیا;
        }
        // Select straddles both sides, but is just a shorter extend
        إذا (lsb == 0) {
            lhsp->unlink_Fr_شا();
            nodep->replaceWithKeepDType(جديد AstExtend{nodep->fileline(), lhsp});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع ریښتیا;
        }

        أرجع خطأ;
    }
    منطقي عملوند_Sel_Bi_ښکتنی(AstSel* nodep) {
        // SEL(ADD(a,b),(width-1),0) -> ADD(SEL(a),SEL(b))
        // Add or any operation which doesn't care if we discard top bits
        Ast_غوټه_Biop* ثابت bip = VN_CAST(nodep->fromp(), غوټه_Biop);
        إذا (!(m_doV && bip && VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0)) أرجع خطأ;
        UINFOTREE(9, nodep, "", "SEL(BI)-in");
        Ast_غوټه_بیان* ثابت bilhsp = bip->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت birhsp = bip->rhsp()->unlink_Fr_شا();
        bip->lhsp(جديد AstSel{nodep->fileline(), bilhsp, 0, nodep->پلنوالی_Const()});
        bip->rhsp(جديد AstSel{nodep->fileline(), birhsp, 0, nodep->پلنوالی_Const()});
        UINFOTREE(9, bip, "", "SEL(BI)-ou");
        VL_DO_DANGLING(replaceWChild(nodep, bip), nodep);
        أرجع ریښتیا;
    }
    منطقي عملوند_Sel_ښويدنه_ښکتنی(AstSel* nodep) {
        // AND({a}, SHIFTR({b}, {c})) is often shorthand in C for Verilog {b}[{c} :+ {a}]
        // becomes thought other optimizations
        // SEL(SHIFTR({a},{b}),{lsb},{width}) -> SEL({a},{lsb+b},{width})
        ثابت Ast_ښويدنه_R* ثابت shiftp = VN_CAST(nodep->fromp(), ښويدنه_R);
        إذا (!(m_doV && shiftp && VN_IS(shiftp->rhsp(), Const) && VN_IS(nodep->lsbp(), Const))) {
            أرجع خطأ;
        }
        Ast_غوټه_بیان* ثابت ap = shiftp->lhsp();
        ثابت AstConst* ثابت bp = VN_AS(shiftp->rhsp(), Const);
        ثابت AstConst* ثابت lp = VN_AS(nodep->lsbp(), Const);
        إذا (bp->isWide() || bp->عدد().isFourState() || bp->عدد().isNegative() || lp->isWide()
            || lp->عدد().isFourState() || lp->عدد().isNegative()) {
            أرجع خطأ;
        }
        ثابت صحيح newLsb = lp->منزل_SInt() + bp->منزل_SInt();
        إذا (newLsb + nodep->پلنوالی_Const() > ap->پلنوالی()) أرجع خطأ;
        //
        UINFO(9, "SEL(SHIFTR(a,b),l,w) -> SEL(a,l+b,w)");
        UINFOTREE(9, nodep, "", "SEL(SH)-in");
        AstSel* ثابت newp
            = جديد AstSel{nodep->fileline(), ap->unlink_Fr_شا(), newLsb, nodep->پلنوالی_Const()};
        nodep->replaceWithKeepDType(newp);
        UINFOTREE(9, newp, "", "SEL(SH)-ou");
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        أرجع ریښتیا;
    }

    منطقي عملوند_Bi_Extend_Const_Shrink(Ast_غوټه_Biop* nodep) {
        // Loop unrolling favors standalone compares
        // EQ(const{width32}, EXTEND(xx{width3})) -> EQ(const{3}, xx{3})
        // The constant must have zero bits (+ 1 if signed) or compare
        // would be incorrect. See also operandBiExtendConst
        AstExtend* ثابت extendp = VN_CAST(nodep->rhsp(), Extend);
        إذا (!extendp) أرجع خطأ;
        Ast_غوټه_بیان* ثابت smallerp = extendp->lhsp();
        ثابت صحيح subsize = smallerp->پلنوالی();
        AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
        إذا (!constp) أرجع خطأ;
        إذا (!constp->عدد().is_بټونه_Zero(constp->پلنوالی() - 1, subsize)) أرجع خطأ;
        //
        UINFOTREE(9, nodep, "", "BI(EXTEND)-in");
        smallerp->unlink_Fr_شا();
        VL_DO_DANGLING(ټېل_Deletep(extendp->unlink_Fr_شا()), extendp);  // aka nodep->lhsp.
        nodep->rhsp(smallerp);

        constp->unlink_Fr_شا();
        ثابت V3_شمېره عدد{constp, subsize, constp->عدد()};
        nodep->lhsp(جديد AstConst{constp->fileline(), عدد});
        VL_DO_DANGLING(ټېل_Deletep(constp), constp);
        أرجع خطأ;  // input node is still valid, keep going
    }
    منطقي عملوند_Bi_Extend_Const_Over(ثابت Ast_غوټه_Biop* nodep) {
        // EQ(const{width32}, EXTEND(xx{width3})) -> constant
        // When the constant has non-zero bits above the extend it's a constant.
        // Avoids compiler warning
        ثابت AstExtend* ثابت extendp = VN_CAST(nodep->rhsp(), Extend);
        إذا (!extendp) أرجع خطأ;
        ثابت Ast_غوټه* ثابت smallerp = extendp->lhsp();
        ثابت صحيح subsize = smallerp->پلنوالی();
        ثابت AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
        إذا (!constp) أرجع خطأ;
        إذا (constp->عدد().is_بټونه_Zero(constp->پلنوالی() - 1, subsize)) أرجع خطأ;
        أرجع ریښتیا;
    }

    // Extraction checks
    منطقي warnSelect(AstSel* nodep) {
        إذا (m_doGenerate) {
            // Never checked yet
            V3_پلنوالی::پلنوالی_Params_Edit(nodep);
            iterateChildren(nodep);  // May need "constifying"
        }
        // Find range of dtype we are selecting from
        // Similar code in V3Unknown::AstSel
        ثابت منطقي doit = ریښتیا;
        إذا (m_warn && VN_IS(nodep->lsbp(), Const) && doit) {
            ثابت صحيح max_Decl_بټ = nodep->declRange().hiMaxSelect() * nodep->decl_El_پلنوالی()
                                   + (nodep->decl_El_پلنوالی() - 1);
            إذا (VN_AS(nodep->lsbp(), Const)->عدد().isFourState()) {
                nodep->v3error("Selection index is constantly unknown or tristated: "
                               "lsb="
                               << nodep->lsbp()->name() << " width=" << nodep->پلنوالی_Const());
                // Replacing nodep will make a mess above, so we replace the offender
                replaceZero(nodep->lsbp());
            } وإلا إذا (nodep->declRange().ranged()
                       && (nodep->msbConst() > max_Decl_بټ || nodep->lsbConst() > max_Decl_بټ)) {
                // See also warning in V3Width
                // Must adjust by element width as declRange() is in number of elements
                string msbLsbProtected;
                إذا (nodep->decl_El_پلنوالی() == 0) {
                    msbLsbProtected = "(nodep->declElWidth() == 0) "
                                      + std::منزل_string(nodep->msbConst()) + ":"
                                      + std::منزل_string(nodep->lsbConst());
                } وإلا {
                    msbLsbProtected = std::منزل_string(nodep->msbConst() / nodep->decl_El_پلنوالی())
                                      + ":"
                                      + std::منزل_string(nodep->lsbConst() / nodep->decl_El_پلنوالی());
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

    ساكن منطقي operandsSame(ثابت Ast_غوټه* node1p, ثابت Ast_غوټه* node2p) {
        // For now we just detect constants & simple vars, though it could be more generic
        إذا (ثابت AstConst* ثابت const1p = VN_CAST(node1p, Const)) {
            إذا (ثابت AstConst* ثابت const2p = VN_CAST(node2p, Const)) {
                أرجع V3_شمېره{node1p->fileline(), 1, 0}
                    .opEq(const1p->عدد(), const2p->عدد())
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
                إذا (VN_IS(and1p->lhsp(), Const) && VN_IS(and1p->rhsp(), غوټه_Var_Ref)
                    && VN_IS(and2p->lhsp(), Const) && VN_IS(and2p->rhsp(), غوټه_Var_Ref))
                    أرجع node1p->same_Gate_ونه(node2p);
            }
        }
        أرجع خطأ;
    }
    منطقي if_Same_ټاکل(ثابت Ast_غوټه_If* nodep) {
        ثابت Ast_غوټه_ټاکل* ثابت thensp = VN_CAST(nodep->thensp(), غوټه_ټاکل);
        ثابت Ast_غوټه_ټاکل* ثابت elsesp = VN_CAST(nodep->elsesp(), غوټه_ټاکل);
        إذا (!thensp || thensp->nextp()) أرجع خطأ;  // Must be SINGLE statement
        إذا (!elsesp || elsesp->nextp()) أرجع خطأ;
        إذا (thensp->type() != elsesp->type()) أرجع خطأ;  // Can't mix an assigndly with assign
        إذا (!thensp->lhsp()->same_Gate_ونه(elsesp->lhsp())) أرجع خطأ;
        إذا (!thensp->rhsp()->gate_ونه()) أرجع خطأ;
        إذا (!elsesp->rhsp()->gate_ونه()) أرجع خطأ;
        // Must not create an expression with unpacked array type
        إذا (VN_IS(thensp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) أرجع خطأ;
        إذا (VN_IS(elsesp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) أرجع خطأ;
        إذا (m_underRecFunc) أرجع خطأ;  // This optimization may lead to infinite recursion
        إذا (m_doCpp) {  // Do not combine ifs cretaed by V3Premit
            إذا (VN_IS(thensp->lhsp(), VarRef) && thensp->lhsp()->isWide()) أرجع خطأ;
        }
        // Only do it if not calls and both pure, otherwise undoes V3LiftExpr
        أرجع !VN_IS(thensp->rhsp(), غوټه_FTask_Ref)  //
               && !VN_IS(elsesp->rhsp(), غوټه_FTask_Ref)  //
               && thensp->rhsp()->is_خالص()  //
               && elsesp->rhsp()->is_خالص();
    }
    منطقي عملوند_If_If(ثابت Ast_غوټه_If* nodep) {
        إذا (nodep->elsesp()) أرجع خطأ;
        ثابت Ast_غوټه_If* ثابت ښکتنی_Ifp = VN_CAST(nodep->thensp(), غوټه_If);
        إذا (!ښکتنی_Ifp || ښکتنی_Ifp->nextp()) أرجع خطأ;
        إذا (nodep->type() != ښکتنی_Ifp->type()) أرجع خطأ;
        إذا (Ast_غوټه::afterCommentp(ښکتنی_Ifp->elsesp())) أرجع خطأ;
        أرجع ریښتیا;
    }
    منطقي ifConcatMergeableBiop(ثابت Ast_غوټه* nodep) {
        أرجع (VN_IS(nodep, And) || VN_IS(nodep, Or) || VN_IS(nodep, Xor));
    }
    منطقي ifAdjacentSel(ثابت AstSel* lhsp, ثابت AstSel* rhsp) {
        إذا (!v3Global.opt.fAssemble()) أرجع خطأ;  // opt disabled
        إذا (!lhsp || !rhsp) أرجع خطأ;
        ثابت Ast_غوټه* ثابت lfromp = lhsp->fromp();
        ثابت Ast_غوټه* ثابت rfromp = rhsp->fromp();
        إذا (!lfromp || !rfromp || !lfromp->same_Gate_ونه(rfromp)) أرجع خطأ;
        ثابت AstConst* ثابت lstart = VN_CAST(lhsp->lsbp(), Const);
        ثابت AstConst* ثابت rstart = VN_CAST(rhsp->lsbp(), Const);
        إذا (!lstart || !rstart) أرجع خطأ;  // too complicated
        ثابت صحيح rend = (rstart->منزل_SInt() + rhsp->پلنوالی_Const());
        أرجع (rend == lstart->منزل_SInt());
    }
    منطقي if_یوځای_Adjacent(ثابت Ast_غوټه_بیان* lhsp, ثابت Ast_غوټه_بیان* rhsp) {
        // Called by concatmergeable to determine if {lhsp, rhsp} make sense
        إذا (!v3Global.opt.fAssemble()) أرجع خطأ;  // opt disabled
        // Two same varref
        إذا (operandsSame(lhsp, rhsp)) أرجع ریښتیا;
        ثابت AstSel* ثابت lselp = VN_CAST(lhsp, Sel);
        ثابت AstSel* ثابت rselp = VN_CAST(rhsp, Sel);
        إذا (!lselp && !rselp) أرجع خطأ;
        إذا (lselp && !VN_IS(lselp->lsbp(), Const)) أرجع خطأ;
        إذا (rselp && !VN_IS(rselp->lsbp(), Const)) أرجع خطأ;
        // a[i:j] {a[j-1:k], b}
        إذا (lselp && !rselp && VN_IS(rhsp, Concat))
            أرجع if_یوځای_Adjacent(lhsp, VN_AS(rhsp, Concat)->lhsp());
        // {b, a[j:k]} a[k-1:i]
        إذا (rselp && !lselp && VN_IS(lhsp, Concat))
            أرجع if_یوځای_Adjacent(VN_AS(lhsp, Concat)->rhsp(), rhsp);
        // a a[msb:j]
        ثابت Ast_غوټه_بیان* ثابت lfromp = lselp                                ? lselp->fromp()
                                          : lhsp->same_Gate_ونه(rselp->fromp()) ? lhsp
                                                                               : تش_اشاره;
        // a[i:0] a
        ثابت Ast_غوټه_بیان* ثابت rfromp = rselp                                ? rselp->fromp()
                                          : rhsp->same_Gate_ونه(lselp->fromp()) ? rhsp
                                                                               : تش_اشاره;
        إذا (!lfromp || !rfromp || !lfromp->same_Gate_ونه(rfromp)) أرجع خطأ;

        ثابت int32_t lstart = lselp ? lselp->lsbConst() : 0;
        ثابت int32_t rstart = rselp ? rselp->lsbConst() : 0;
        ثابت int32_t rend = rstart + (rselp ? rselp->پلنوالی_Const() : rhsp->پلنوالی());
        // a[i:j] a[j-1:k]
        إذا (rend == lstart) أرجع ریښتیا;
        // a[i:0] a[msb:j]
        إذا (rend == rfromp->پلنوالی() && lstart == 0) أرجع ریښتیا;
        أرجع خطأ;
    }
    منطقي concatMergeable(ثابت Ast_غوټه_بیان* lhsp, ثابت Ast_غوټه_بیان* rhsp, بینښه ژوروالی) {
        // Determine if {a OP b, c OP d} => {a, c} OP {b, d} is advantageous
        إذا (!v3Global.opt.fAssemble()) أرجع خطأ;  // opt disabled
        إذا (lhsp->type() != rhsp->type()) أرجع خطأ;
        إذا (!ifConcatMergeableBiop(lhsp)) أرجع خطأ;
        إذا (ژوروالی > CONCAT_MERGABLE_MAX_ژوروالی) أرجع خطأ;  // As worse case O(n^2) algorithm

        ثابت Ast_غوټه_Biop* ثابت lp = VN_CAST(lhsp, غوټه_Biop);
        ثابت Ast_غوټه_Biop* ثابت rp = VN_CAST(rhsp, غوټه_Biop);
        إذا (!lp || !rp) أرجع خطأ;
        // {a[]&b[], a[]&b[]}
        ثابت منطقي lad = if_یوځای_Adjacent(lp->lhsp(), rp->lhsp());
        ثابت منطقي rad = if_یوځای_Adjacent(lp->rhsp(), rp->rhsp());
        إذا (lad && rad) أرجع ریښتیا;
        // {a[] & b[]&c[], a[] & b[]&c[]}
        إذا (lad && concatMergeable(lp->rhsp(), rp->rhsp(), ژوروالی + 1)) أرجع ریښتیا;
        // {a[]&b[] & c[], a[]&b[] & c[]}
        إذا (rad && concatMergeable(lp->lhsp(), rp->lhsp(), ژوروالی + 1)) أرجع ریښتیا;
        // {(a[]&b[])&(c[]&d[]), (a[]&b[])&(c[]&d[])}
        إذا (concatMergeable(lp->lhsp(), rp->lhsp(), ژوروالی + 1)
            && concatMergeable(lp->rhsp(), rp->rhsp(), ژوروالی + 1)) {
            أرجع ریښتیا;
        }
        أرجع خطأ;
    }
    ساكن منطقي operands_Same_پلنوالی(ثابت Ast_غوټه* lhsp, ثابت Ast_غوټه* rhsp) {
        أرجع lhsp->پلنوالی() == rhsp->پلنوالی();
    }

    //----------------------------------------
    // Constant Replacement functions.
    // These all take a node, delete its tree, and replaces it with a constant

    فراغ replace_عدد(Ast_غوټه* oldp, ثابت V3_شمېره& عدد, ثابت string& origParamName = "") {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        UASSERT_OBJ(!(VN_IS(oldp, Const) && !VN_AS(oldp, Const)->عدد().isFourState()), oldp,
                    "Already constant??");
        AstConst* ثابت newp = جديد AstConst{oldp->fileline(), عدد};
        إذا (!origParamName.تشه()) newp->origParamName(origParamName);
        oldp->replaceWithKeepDType(newp);
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        VL_DO_DANGLING(ټېل_Deletep(oldp), oldp);
    }
    فراغ replace_عدد(Ast_غوټه* nodep, uint32_t val) {
        ثابت V3_شمېره عدد{nodep, nodep->پلنوالی(), val};
        VL_DO_DANGLING(replace_عدد(nodep, عدد), nodep);
    }
    فراغ replace_عدد_Signed(Ast_غوټه_Biop* nodep, uint32_t val) {
        // We allow both sides to be constant, as one may have come from
        // parameter propagation, etc.
        إذا (m_warn && !(VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const))) {
            nodep->v3warn(UNSIGNED, "Comparison is constant due to unsigned arithmetic");
        }
        VL_DO_DANGLING(replace_عدد(nodep, val), nodep);
    }
    فراغ replace_عدد_Limited(Ast_غوټه_Biop* nodep, uint32_t val) {
        // Avoids gcc warning about same
        إذا (m_warn) nodep->v3warn(CMPCONST, "Comparison is constant due to limited range");
        VL_DO_DANGLING(replace_عدد(nodep, val), nodep);
    }
    فراغ replaceZero(Ast_غوټه* nodep) { VL_DO_DANGLING(replace_عدد(nodep, 0), nodep); }
    فراغ replace_Zero_Chk_خالص(Ast_غوټه* nodep, Ast_غوټه_بیان* checkp) {
        // For example, "0 * n" -> 0 if n has no side effects
        // Else strength reduce it to 0 & n.
        // If ever change the operation note AstAnd rule specially ignores this created pattern
        إذا (checkp->is_خالص()) {
            VL_DO_DANGLING(replace_عدد(nodep, 0), nodep);
        } وإلا {
            Ast_غوټه* ثابت newp = جديد AstAnd{nodep->fileline(), جديد AstConst{nodep->fileline(), 0},
                                             checkp->unlink_Fr_شا()};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
    }
    فراغ replaceAllOnes(Ast_غوټه* nodep) {
        V3_شمېره ones{nodep, nodep->پلنوالی(), 0};
        ones.سېټ_ماسک(nodep->پلنوالی());
        VL_DO_DANGLING(replace_عدد(nodep, ones), nodep);
    }
    فراغ replaceConst(Ast_غوټه_Uniop* nodep) {
        V3_شمېره numv{nodep, nodep->پلنوالی_Min_V()};
        nodep->شمېره_عمل_کړه(numv, const_عدد_V(nodep->lhsp()));
        ثابت V3_شمېره& عدد = منزل_عدد_C(nodep, numv);
        UINFO(4, "UNICONST -> " << عدد);
        VL_DO_DANGLING(replace_عدد(nodep, عدد), nodep);
    }
    فراغ replaceConst(Ast_غوټه_Biop* nodep) {
        V3_شمېره numv{nodep, nodep->پلنوالی_Min_V()};
        nodep->شمېره_عمل_کړه(numv, const_عدد_V(nodep->lhsp()), const_عدد_V(nodep->rhsp()));
        ثابت V3_شمېره& عدد = منزل_عدد_C(nodep, numv);
        UINFO(4, "BICONST -> " << عدد);
        VL_DO_DANGLING(replace_عدد(nodep, عدد), nodep);
    }
    فراغ replaceConst(Ast_غوټه_Triop* nodep) {
        V3_شمېره numv{nodep, nodep->پلنوالی_Min_V()};
        nodep->شمېره_عمل_کړه(numv, const_عدد_V(nodep->lhsp()), const_عدد_V(nodep->rhsp()),
                             const_عدد_V(nodep->thsp()));
        ثابت V3_شمېره& عدد = منزل_عدد_C(nodep, numv);
        UINFO(4, "TRICONST -> " << عدد);
        VL_DO_DANGLING(replace_عدد(nodep, عدد), nodep);
    }
    فراغ replaceConst(Ast_غوټه_Quadop* nodep) {
        V3_شمېره numv{nodep, nodep->پلنوالی_Min_V()};
        nodep->شمېره_عمل_کړه(numv, const_عدد_V(nodep->lhsp()), const_عدد_V(nodep->rhsp()),
                             const_عدد_V(nodep->thsp()), const_عدد_V(nodep->fhsp()));
        ثابت V3_شمېره& عدد = منزل_عدد_C(nodep, numv);
        UINFO(4, "QUADCONST -> " << عدد);
        VL_DO_DANGLING(replace_عدد(nodep, عدد), nodep);
    }

    فراغ replaceConstString(Ast_غوټه* oldp, ثابت string& عدد) {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        Ast_غوټه* ثابت newp = جديد AstConst{oldp->fileline(), AstConst::String{}, عدد};
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        oldp->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(oldp), oldp);
    }
    //----------------------------------------
    // Replacement functions.
    // These all take a node and replace it with something else

    فراغ replaceWChild(Ast_غوټه* nodep, Ast_غوټه_بیان* childp) {
        // NODE(..., CHILD(...)) -> CHILD(...)
        childp->unlink_Fr_شا_With_بل();
        // If replacing a SEL for example, the data type comes from the parent (is less wide).
        // This may adversely affect the operation of the node being replaced.
        nodep->replaceWithKeepDType(childp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ replaceWChildBool(Ast_غوټه* nodep, Ast_غوټه_بیان* childp) {
        // NODE(..., CHILD(...)) -> REDOR(CHILD(...))
        childp->unlink_Fr_شا();
        إذا (childp->width1()) {
            nodep->replaceWithKeepDType(childp);
        } وإلا {
            nodep->replaceWithKeepDType(جديد AstRedOr{childp->fileline(), childp});
        }
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    //! Replace a ternary node with its RHS after iterating
    //! Used with short-circuiting, where the RHS has not yet been iterated.
    فراغ replaceWIteratedRhs(Ast_غوټه_Triop* nodep) {
        إذا (Ast_غوټه* ثابت rhsp = nodep->rhsp()) iterate_And_بل_Null(rhsp);
        replaceWChild(nodep, nodep->rhsp());  // May have changed
    }

    //! Replace a ternary node with its THS after iterating
    //! Used with short-circuiting, where the THS has not yet been iterated.
    فراغ replaceWIteratedThs(Ast_غوټه_Triop* nodep) {
        إذا (Ast_غوټه* ثابت thsp = nodep->thsp()) iterate_And_بل_Null(thsp);
        replaceWChild(nodep, nodep->thsp());  // May have changed
    }
    فراغ replaceWLhs(Ast_غوټه_Uniop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    فراغ replaceWLhs(Ast_غوټه_Biop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    فراغ replaceWRhs(Ast_غوټه_Biop* nodep) {
        // Keep RHS, remove LHS
        replaceWChild(nodep, nodep->rhsp());
    }
    فراغ replaceWLhsBool(Ast_غوټه_Biop* nodep) { replaceWChildBool(nodep, nodep->lhsp()); }
    فراغ replaceWRhsBool(Ast_غوټه_Biop* nodep) { replaceWChildBool(nodep, nodep->rhsp()); }
    فراغ replaceAsv(Ast_غوټه_Biop* nodep) {
        // BIASV(CONSTa, BIASV(CONSTb, c)) -> BIASV( BIASV_CONSTED(a,b), c)
        // BIASV(SAMEa,  BIASV(SAMEb, c))  -> BIASV( BIASV(SAMEa,SAMEb), c)
        // UINFOTREE(1, nodep, "", "repAsvConst_old");
        Ast_غوټه_بیان* ثابت ap = nodep->lhsp();
        Ast_غوټه_Biop* ثابت rp = VN_AS(nodep->rhsp(), غوټه_Biop);
        Ast_غوټه_بیان* ثابت bp = rp->lhsp();
        Ast_غوټه_بیان* ثابت cp = rp->rhsp();
        ap->unlink_Fr_شا();
        bp->unlink_Fr_شا();
        cp->unlink_Fr_شا();
        rp->unlink_Fr_شا();
        nodep->lhsp(rp);
        nodep->rhsp(cp);
        rp->lhsp(ap);
        rp->rhsp(bp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        إذا (VN_IS(rp->lhsp(), Const) && VN_IS(rp->rhsp(), Const)) replaceConst(rp);
        iterate(nodep);  // Proceed to fixed point
    }
    فراغ replaceAsvLUp(Ast_غوټه_Biop* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r))
        Ast_غوټه_Biop* ثابت lp = VN_AS(nodep->lhsp()->unlink_Fr_شا(), غوټه_Biop);
        Ast_غوټه_بیان* ثابت llp = lp->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت lrp = lp->rhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت rp = nodep->rhsp()->unlink_Fr_شا();
        nodep->lhsp(llp);
        nodep->rhsp(lp);
        lp->lhsp(lrp);
        lp->rhsp(rp);
        lp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    فراغ replaceAsvRUp(Ast_غوټه_Biop* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr))
        Ast_غوټه_بیان* ثابت lp = nodep->lhsp()->unlink_Fr_شا();
        Ast_غوټه_Biop* ثابت rp = VN_AS(nodep->rhsp()->unlink_Fr_شا(), غوټه_Biop);
        Ast_غوټه_بیان* ثابت rlp = rp->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت rrp = rp->rhsp()->unlink_Fr_شا();
        nodep->lhsp(rlp);
        nodep->rhsp(rp);
        rp->lhsp(lp);
        rp->rhsp(rrp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    فراغ replaceAndOr(Ast_غوټه_Biop* nodep) {
        //  OR  (AND (CONSTll,lr), AND(CONSTrl==ll,rr))    -> AND (CONSTll, OR(lr,rr))
        //  OR  (AND (CONSTll,lr), AND(CONSTrl,    rr=lr)) -> AND (OR(ll,rl), rr)
        // nodep ^lp  ^llp   ^lrp  ^rp  ^rlp       ^rrp
        // (Or/And may also be reversed)
        Ast_غوټه_Biop* ثابت lp = VN_AS(nodep->lhsp()->unlink_Fr_شا(), غوټه_Biop);
        Ast_غوټه_بیان* ثابت llp = lp->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت lrp = lp->rhsp()->unlink_Fr_شا();
        Ast_غوټه_Biop* ثابت rp = VN_AS(nodep->rhsp()->unlink_Fr_شا(), غوټه_Biop);
        Ast_غوټه_بیان* ثابت rlp = rp->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت rrp = rp->rhsp()->unlink_Fr_شا();
        nodep->replaceWithKeepDType(lp);
        إذا (operandsSame(llp, rlp)) {
            lp->lhsp(llp);
            lp->rhsp(nodep);
            lp->dtypeFrom(nodep);
            nodep->lhsp(lrp);
            nodep->rhsp(rrp);
            VL_DO_DANGLING(ټېل_Deletep(rp), rp);
            VL_DO_DANGLING(ټېل_Deletep(rlp), rlp);
        } وإلا إذا (operandsSame(lrp, rrp)) {
            lp->lhsp(nodep);
            lp->rhsp(rrp);
            lp->dtypeFrom(nodep);
            nodep->lhsp(llp);
            nodep->rhsp(rlp);
            VL_DO_DANGLING(ټېل_Deletep(rp), rp);
            VL_DO_DANGLING(ټېل_Deletep(lrp), lrp);
        } وإلا {
            nodep->v3fatalSrc("replaceAndOr on something operandAndOrSame shouldn't have matched");
        }
        // UINFOTREE(1, nodep, "", "repAndOr_new");
    }
    فراغ replace_ښويدنه_Same(Ast_غوټه_Biop* nodep) {
        // Or(Shift(ll,CONSTlr),Shift(rl,CONSTrr==lr)) -> Shift(Or(ll,rl),CONSTlr)
        // (Or/And may also be reversed)
        Ast_غوټه_Biop* ثابت lp = VN_AS(nodep->lhsp()->unlink_Fr_شا(), غوټه_Biop);
        Ast_غوټه_بیان* ثابت llp = lp->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت lrp = lp->rhsp()->unlink_Fr_شا();
        Ast_غوټه_Biop* ثابت rp = VN_AS(nodep->rhsp()->unlink_Fr_شا(), غوټه_Biop);
        Ast_غوټه_بیان* ثابت rlp = rp->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت rrp = rp->rhsp()->unlink_Fr_شا();
        nodep->replaceWithKeepDType(lp);
        lp->lhsp(nodep);
        lp->rhsp(lrp);
        nodep->lhsp(llp);
        nodep->rhsp(rlp);
        nodep->dtypep(llp->dtypep());  // dtype of Biop is before shift.
        VL_DO_DANGLING(ټېل_Deletep(rp), rp);
        VL_DO_DANGLING(ټېل_Deletep(rrp), rrp);
        // UINFOTREE(1, nodep, "", "repShiftSame_new");
    }
    فراغ replaceConcatSel(AstConcat* nodep) {
        // {a[1], a[0]} -> a[1:0]
        AstSel* ثابت lselp = VN_AS(nodep->lhsp()->unlink_Fr_شا(), Sel);
        AstSel* ثابت rselp = VN_AS(nodep->rhsp()->unlink_Fr_شا(), Sel);
        ثابت صحيح lstart = lselp->lsbConst();
        ثابت صحيح lwidth = lselp->پلنوالی_Const();
        ثابت صحيح rstart = rselp->lsbConst();
        ثابت صحيح rwidth = rselp->پلنوالی_Const();

        UASSERT_OBJ((rstart + rwidth) == lstart, nodep,
                    "tried to merge two selects which are not adjacent");
        AstSel* ثابت newselp = جديد AstSel{
            lselp->fromp()->fileline(), rselp->fromp()->unlink_Fr_شا(), rstart, lwidth + rwidth};
        UINFO(5, "merged two adjacent sel " << lselp << " and " << rselp << " to one " << newselp);

        nodep->replaceWithKeepDType(newselp);
        VL_DO_DANGLING(ټېل_Deletep(lselp), lselp);
        VL_DO_DANGLING(ټېل_Deletep(rselp), rselp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ replace_Concat_یوځای(AstConcat* nodep) {
        // {llp OP lrp, rlp OP rrp} => {llp, rlp} OP {lrp, rrp}, where OP = AND/OR/XOR
        Ast_غوټه_Biop* ثابت lp = VN_AS(nodep->lhsp(), غوټه_Biop);
        Ast_غوټه_Biop* ثابت rp = VN_AS(nodep->rhsp(), غوټه_Biop);
        إذا (concatMergeable(lp, rp, 0)) {
            Ast_غوټه_بیان* ثابت llp = lp->lhsp();
            Ast_غوټه_بیان* ثابت lrp = lp->rhsp();
            Ast_غوټه_بیان* ثابت rlp = rp->lhsp();
            Ast_غوټه_بیان* ثابت rrp = rp->rhsp();
            AstConcat* ثابت newlp = جديد AstConcat{rlp->fileline(), llp->clone_ونه_خالص(خطأ),
                                                   rlp->clone_ونه_خالص(خطأ)};
            AstConcat* ثابت newrp = جديد AstConcat{rrp->fileline(), lrp->clone_ونه_خالص(خطأ),
                                                   rrp->clone_ونه_خالص(خطأ)};
            // use the lhs to replace the parent concat
            llp->replaceWith(newlp);
            VL_DO_DANGLING(ټېل_Deletep(llp), llp);
            lrp->replaceWith(newrp);
            VL_DO_DANGLING(ټېل_Deletep(lrp), lrp);
            lp->dtype_Chg_پلنوالی_Signed(newlp->پلنوالی(), newlp->پلنوالی(), VSigning::UNSIGNED);
            UINFO(5, "merged " << nodep);
            ++m_stat_Concat_یوځای;
            VL_DO_DANGLING(ټېل_Deletep(rp->unlink_Fr_شا()), rp);
            nodep->replaceWithKeepDType(lp->unlink_Fr_شا());
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            iterate(lp->lhsp());
            iterate(lp->rhsp());
        } وإلا {
            nodep->v3fatalSrc("tried to merge two Concat which are not adjacent");
        }
    }
    فراغ replaceExtend(Ast_غوټه* nodep, Ast_غوټه_بیان* arg0p) {
        // -> EXTEND(nodep)
        // like a AstExtend{$rhsp}, but we need to set the width correctly from base node
        arg0p->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت newp
            = (VN_IS(nodep, ExtendS)
                   ? static_cast<Ast_غوټه_بیان*>(جديد AstExtendS{nodep->fileline(), arg0p})
                   : static_cast<Ast_غوټه_بیان*>(جديد AstExtend{nodep->fileline(), arg0p}));
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ replace_Pow_ښويدنه(Ast_غوټه_Biop* nodep) {  // Pow or PowS
        UINFO(5, "POW(2,b)->SHIFTL(1,b) " << nodep);
        Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
        Ast_ښويدنه_L* ثابت newp
            = جديد Ast_ښويدنه_L{nodep->fileline(), جديد AstConst{nodep->fileline(), 1}, rhsp};
        newp->lhsp()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ replace_Mul_ښويدنه(AstMul* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "MUL(2^n,b)->SHIFTL(b,n) " << nodep);
        ثابت صحيح amount = VN_AS(nodep->lhsp(), Const)->عدد().most_سېټ_بټ_P1() - 1;  // 2^n->n+1
        Ast_غوټه_بیان* ثابت opp = nodep->rhsp()->unlink_Fr_شا();
        Ast_ښويدنه_L* ثابت newp
            = جديد Ast_ښويدنه_L{nodep->fileline(), opp, جديد AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ replace_Div_ښويدنه(AstDiv* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "DIV(b,2^n)->SHIFTR(b,n) " << nodep);
        ثابت صحيح amount = VN_AS(nodep->rhsp(), Const)->عدد().most_سېټ_بټ_P1() - 1;  // 2^n->n+1
        Ast_غوټه_بیان* ثابت opp = nodep->lhsp()->unlink_Fr_شا();
        Ast_ښويدنه_R* ثابت newp
            = جديد Ast_ښويدنه_R{nodep->fileline(), opp, جديد AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ replaceModAnd(AstModDiv* nodep) {  // Mod, but not ModS as not simple shift
        UINFO(5, "MOD(b,2^n)->AND(b,2^n-1) " << nodep);
        ثابت صحيح amount = VN_AS(nodep->rhsp(), Const)->عدد().most_سېټ_بټ_P1() - 1;  // 2^n->n+1
        V3_شمېره ماسک{nodep, nodep->پلنوالی()};
        ماسک.سېټ_ماسک(amount);
        Ast_غوټه_بیان* ثابت opp = nodep->lhsp()->unlink_Fr_شا();
        AstAnd* ثابت newp
            = جديد AstAnd{nodep->fileline(), opp, جديد AstConst{nodep->fileline(), ماسک}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ replace_ښويدنه_Op(Ast_غوټه_Biop* nodep) {
        UINFO(5, "SHIFT(AND(a,b),CONST) with a/b special ->AND(SHIFT(a,CONST),SHIFT(b,CONST)) "
                     << nodep);
        ثابت صحيح پلنوالی = nodep->پلنوالی();
        ثابت صحيح پلنوالی_Min = nodep->پلنوالی_Min();
        VNRelinker handle;
        nodep->unlink_Fr_شا(&handle);
        Ast_غوټه_Biop* ثابت lhsp = VN_AS(nodep->lhsp(), غوټه_Biop);
        lhsp->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت shiftp = nodep->rhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت ap = lhsp->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت bp = lhsp->rhsp()->unlink_Fr_شا();
        Ast_غوټه_Biop* ثابت shift1p = nodep;
        Ast_غوټه_Biop* ثابت shift2p = nodep->clone_ونه(ریښتیا);
        shift1p->lhsp(ap);
        shift1p->rhsp(shiftp->clone_ونه_خالص(ریښتیا));
        shift2p->lhsp(bp);
        shift2p->rhsp(shiftp);
        Ast_غوټه_Biop* ثابت newp = lhsp;
        newp->lhsp(shift1p);
        newp->rhsp(shift2p);
        newp->dtype_Chg_پلنوالی(پلنوالی, پلنوالی_Min);  // The new AND must have width of the original SHIFT
        handle.relink(newp);
        iterate(newp);  // Further reduce, either node may have more reductions.
    }
    فراغ replace_ښويدنه_ښويدنه(Ast_غوټه_Biop* nodep) {
        UINFO(4, "SHIFT(SHIFT(a,s1),s2)->SHIFT(a,ADD(s1,s2)) " << nodep);
        UINFOTREE(9, nodep, "", "repShiftShift_old");
        Ast_غوټه_Biop* ثابت lhsp = VN_AS(nodep->lhsp(), غوټه_Biop);
        lhsp->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت ap = lhsp->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت shift1p = lhsp->rhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت shift2p = nodep->rhsp()->unlink_Fr_شا();
        // Shift1p and shift2p may have different sizes, both are
        // self-determined so sum with infinite width
        إذا (nodep->type() == lhsp->type()) {
            ثابت صحيح shift1 = VN_AS(shift1p, Const)->منزل_UInt();
            ثابت صحيح shift2 = VN_AS(shift2p, Const)->منزل_UInt();
            ثابت صحيح newshift = shift1 + shift2;
            VL_DO_DANGLING(ټېل_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ټېل_Deletep(shift2p), shift2p);
            nodep->lhsp(ap);
            nodep->rhsp(جديد AstConst(nodep->fileline(), newshift));
            iterate(nodep);  // Further reduce, either node may have more reductions.
        } وإلا {
            // We know shift amounts are constant, but might be a mixed left/right shift
            صحيح shift1 = VN_AS(shift1p, Const)->منزل_UInt();
            إذا (VN_IS(lhsp, ښويدنه_R)) shift1 = -shift1;
            صحيح shift2 = VN_AS(shift2p, Const)->منزل_UInt();
            إذا (VN_IS(nodep, ښويدنه_R)) shift2 = -shift2;
            ثابت صحيح newshift = shift1 + shift2;
            VL_DO_DANGLING(ټېل_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ټېل_Deletep(shift2p), shift2p);
            Ast_غوټه_بیان* newp;
            V3_شمېره mask1{nodep, nodep->پلنوالی()};
            V3_شمېره ones{nodep, nodep->پلنوالی()};
            ones.سېټ_ماسک(nodep->پلنوالی());
            إذا (shift1 < 0) {
                mask1.op_ښويدنه_R(ones, V3_شمېره(nodep, VL_IDATASIZE, -shift1));
            } وإلا {
                mask1.op_ښويدنه_L(ones, V3_شمېره(nodep, VL_IDATASIZE, shift1));
            }
            V3_شمېره ماسک{nodep, nodep->پلنوالی()};
            إذا (shift2 < 0) {
                ماسک.op_ښويدنه_R(mask1, V3_شمېره(nodep, VL_IDATASIZE, -shift2));
            } وإلا {
                ماسک.op_ښويدنه_L(mask1, V3_شمېره(nodep, VL_IDATASIZE, shift2));
            }
            إذا (newshift < 0) {
                newp = جديد Ast_ښويدنه_R{nodep->fileline(), ap,
                                     جديد AstConst(nodep->fileline(), -newshift)};
            } وإلا {
                newp = جديد Ast_ښويدنه_L{nodep->fileline(), ap,
                                     جديد AstConst(nodep->fileline(), newshift)};
            }
            newp->dtypeFrom(nodep);
            newp = جديد AstAnd{nodep->fileline(), newp, جديد AstConst{nodep->fileline(), ماسک}};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            // UINFOTREE(1, newp, "", "repShiftShift_new");
            iterate(newp);  // Further reduce, either node may have more reductions.
        }
        VL_DO_DANGLING(ټېل_Deletep(lhsp), lhsp);
    }

    منطقي replace_ټاکل_Multi_Sel(Ast_غوټه_ټاکل* nodep) {
        // Multiple assignments to sequential bits can be concated
        // ASSIGN(SEL(a),aq), ASSIGN(SEL(a+1),bq) -> ASSIGN(SEL(a:b),CONCAT(aq,bq)
        // ie. assign var[2]=a, assign var[3]=b -> assign var[3:2]={b,a}

        // Skip if we're not const'ing an entire module (IE doing only one assign, etc)
        إذا (!m_modp) أرجع خطأ;
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ثابت sel1p = VN_CAST(nodep->lhsp(), Sel);
        إذا (!sel1p) أرجع خطأ;
        Ast_غوټه_ټاکل* ثابت nextp = VN_CAST(nodep->nextp(), غوټه_ټاکل);
        إذا (!nextp) أرجع خطأ;
        إذا (nodep->type() != nextp->type()) أرجع خطأ;
        ثابت AstSel* ثابت sel2p = VN_CAST(nextp->lhsp(), Sel);
        إذا (!sel2p) أرجع خطأ;
        AstVarRef* ثابت varref1p = VN_CAST(sel1p->fromp(), VarRef);
        إذا (!varref1p) أرجع خطأ;
        ثابت AstVarRef* ثابت varref2p = VN_CAST(sel2p->fromp(), VarRef);
        إذا (!varref2p) أرجع خطأ;
        إذا (!varref1p->same_Gate_ونه(varref2p)) أرجع خطأ;
        ثابت AstConst* ثابت con1p = VN_CAST(sel1p->lsbp(), Const);
        إذا (!con1p) أرجع خطأ;
        ثابت AstConst* ثابت con2p = VN_CAST(sel2p->lsbp(), Const);
        إذا (!con2p) أرجع خطأ;
        // We need to make sure there's no self-references involved in either
        // assignment.  For speed, we only look 3 deep, then give up.
        إذا (!varNotReferenced(nodep->rhsp(), varref1p->varp())) أرجع خطأ;
        إذا (!varNotReferenced(nextp->rhsp(), varref2p->varp())) أرجع خطأ;
        // If a variable is marked split_var, access to the variable should not be merged.
        إذا (varref1p->varp()->attr_وېشل_Var() || varref2p->varp()->attr_وېشل_Var()) أرجع خطأ;
        // Swap?
        إذا ((con1p->منزل_SInt() != con2p->منزل_SInt() + sel2p->پلنوالی())
            && (con2p->منزل_SInt() != con1p->منزل_SInt() + sel1p->پلنوالی())) {
            أرجع خطأ;
        }
        ثابت منطقي lsb_لومړی_ټاکل = (con1p->منزل_UInt() < con2p->منزل_UInt());
        UINFO(4, "replaceAssignMultiSel " << nodep);
        UINFO(4, "                   && " << nextp);
        // UINFOTREE(1, nodep, "", "comb1");
        // UINFOTREE(1, nextp, "", "comb2");
        Ast_غوټه_بیان* ثابت rhs1p = nodep->rhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت rhs2p = nextp->rhsp()->unlink_Fr_شا();
        Ast_غوټه_ټاکل* newp;
        إذا (lsb_لومړی_ټاکل) {
            newp = nodep->cloneType(جديد AstSel{sel1p->fileline(), varref1p->unlink_Fr_شا(),
                                               sel1p->lsbConst(), sel1p->پلنوالی() + sel2p->پلنوالی()},
                                    جديد AstConcat{rhs1p->fileline(), rhs2p, rhs1p});
        } وإلا {
            newp = nodep->cloneType(جديد AstSel{sel1p->fileline(), varref1p->unlink_Fr_شا(),
                                               sel2p->lsbConst(), sel1p->پلنوالی() + sel2p->پلنوالی()},
                                    جديد AstConcat{rhs1p->fileline(), rhs1p, rhs2p});
        }
        // UINFOTREE(1, pnewp, "", "conew");
        nodep->replaceWith(newp);  // dypep intentionally changing
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        VL_DO_DANGLING(ټېل_Deletep(nextp->unlink_Fr_شا()), nextp);
        أرجع ریښتیا;
    }

    منطقي replace_ټاکل_SFormat_F(Ast_غوټه_ټاکل* nodep) {
        // Rewrite 'x = sformatf(...)' into 'sformat(x, ...)', which is more efficient
        // at the call site as it does not need to check if 'x' needs to be freed first.
        // This is a somewhat common pattern after lowering assertions.
        إذا (!VN_IS(nodep, ټاکل)) أرجع خطأ;
        AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef);
        إذا (!lhsp) أرجع خطأ;
        AstSFormatF* ثابت fmtp = VN_CAST(nodep->rhsp(), SFormatF);
        إذا (!fmtp) أرجع خطأ;
        lhsp->unlink_Fr_شا();
        fmtp->unlink_Fr_شا();
        nodep->replaceWith(جديد AstSFormat{nodep->fileline(), fmtp, lhsp});
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        أرجع ریښتیا;
    }

    منطقي varNotReferenced(Ast_غوټه* nodep, AstVar* varp, صحيح level = 0) {
        // Return true if varp never referenced under node.
        // Return false if referenced, or tree too deep to be worth it, or side effects
        إذا (!nodep) أرجع ریښتیا;
        إذا (level > 2) أرجع خطأ;
        إذا (!nodep->is_خالص()) أرجع خطأ;  // For example a $fgetc can't be reordered
        إذا (VN_IS(nodep, غوټه_Var_Ref) && VN_AS(nodep, غوټه_Var_Ref)->varp() == varp) أرجع خطأ;
        أرجع (varNotReferenced(nodep->nextp(), varp, level + 1)
                && varNotReferenced(nodep->op1p(), varp, level + 1)
                && varNotReferenced(nodep->op2p(), varp, level + 1)
                && varNotReferenced(nodep->op3p(), varp, level + 1)
                && varNotReferenced(nodep->op4p(), varp, level + 1));
    }

    منطقي replace_غوټه_ټاکل(Ast_غوټه_ټاکل* nodep) {
        إذا (VN_IS(nodep->lhsp(), VarRef) && VN_IS(nodep->rhsp(), VarRef)
            && VN_AS(nodep->lhsp(), VarRef)->sameNoLvalue(VN_AS(nodep->rhsp(), VarRef))
            && !VN_IS(nodep, ټاکل_Dly)) {
            // X = X.  Quite pointless, though X <= X may override another earlier assignment
            إذا (VN_IS(nodep, ټاکل_W)) {
                إذا (!v3Global.opt.pedantic())
                    nodep->v3error(
                        "Wire inputs its own output, creating circular logic (wire x=x)");
                أرجع خطأ;  // Don't delete the assign, or V3Gate will freak out
            } وإلا {
                VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                أرجع ریښتیا;
            }
        } وإلا إذا (m_doV && VN_IS(nodep->lhsp(), Concat)) {
            منطقي need_لنډمهاله = خطأ;
            // A blocking assignment with an intra-assignment timing control evaluates its value,
            // then waits once (IEEE 1800-2023 9.4.5), here to assign a temporary for the parts
            ثابت منطقي waits = VN_IS(nodep, ټاکل) && nodep->timingControlp();
            // Splitting copies the RHS once per part, so a large RHS would make the tree
            // grow with the product of the number of parts and the RHS size
            ثابت منطقي need_لنډمهاله_خالص = !nodep->rhsp()->is_خالص() || waits
                                      || nodep->rhsp()->isLargerThan(CONCAT_ټاکل_MAX_RHS_NODES);
            إذا (m_warn && !VN_IS(nodep, ټاکل_Dly)
                && !need_لنډمهاله_خالص) {  // Is same var on LHS and RHS?
                // If the rhs is not pure, we need a temporary variable anyway
                std::unordered_سېټ<اندازه_t> lhsVarIds;
                nodep->lhsp()->foreach([&](ثابت AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    lhsVarIds.emplace(m_ids(refp->varp()));
                });
                nodep->rhsp()->foreach([&](ثابت AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    إذا (lhsVarIds.شمېر(m_ids(refp->varp()))) need_لنډمهاله = ریښتیا;
                });
            }
            إذا (need_لنډمهاله_خالص) {
                // if the RHS is impure or large we need to create a temporary variable for it,
                // because further handling involves copying of the RHS.
                UINFO(4, "  ASSITEMPPURE " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp,rhs),
                //                                ASSIGN(lc1,SEL(temp,{size1})),
                //                                ASSIGN(lc2,SEL(temp,{size2}))

                Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
                AstVar* ثابت لنډمهاله_Purep = جديد AstVar{rhsp->fileline(), VVarType::BLOCKTEMP,
                                                     m_concswap_نومونه.get(rhsp), rhsp->dtypep()};
                // Static, as read right after written, also in a fork
                لنډمهاله_Purep->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کړه_Stmtsp(لنډمهاله_Purep);
                AstVarRef* ثابت لنډمهاله_خالص_Refp
                    = جديد AstVarRef{rhsp->fileline(), لنډمهاله_Purep, VAccess::WRITE};
                Ast_غوټه_ټاکل* ثابت asnp
                    = VN_IS(nodep, ټاکل_Dly)
                          ? جديد Ast_ټاکل{nodep->fileline(), لنډمهاله_خالص_Refp, rhsp}
                          : nodep->cloneType(لنډمهاله_خالص_Refp, rhsp);
                nodep->جمع_کړه_دلته_This_As_بل(asnp);
                nodep->rhsp(جديد AstVarRef{rhsp->fileline(), لنډمهاله_Purep, VAccess::READ});
                // Only the assignment of the temporary waits
                إذا (waits) ټېل_Deletep(nodep->timingControlp()->unlink_Fr_شا());
            } وإلا إذا (need_لنډمهاله) {
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
                = VN_IS(nodep, ټاکل_Dly) ? VN_CAST(nodep->timingControlp(), Delay) : تش_اشاره;
            إذا (delayp && !delayp->lhsp()->is_خالص()) {
                File_کرښه* ثابت flp = nodep->fileline();
                Ast_غوټه_بیان* ثابت valuep = delayp->lhsp()->unlink_Fr_شا();
                AstVar* ثابت tempp = جديد AstVar{flp, VVarType::BLOCKTEMP,
                                                 m_concswap_نومونه.get(valuep), valuep->dtypep()};
                // Static, as read right after written, when the NBAs to the parts start waiting
                tempp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کړه_Stmtsp(tempp);
                nodep->جمع_کړه_دلته_This_As_بل(
                    جديد Ast_ټاکل{flp, جديد AstVarRef{flp, tempp, VAccess::WRITE}, valuep});
                delayp->lhsp(جديد AstVarRef{flp, tempp, VAccess::READ});
            }
            UINFOTREE(9, nodep, "", "Ass_old");
            // Unlink the stuff
            Ast_غوټه_بیان* ثابت lc1p = VN_AS(nodep->lhsp(), Concat)->lhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت lc2p = VN_AS(nodep->lhsp(), Concat)->rhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت conp = VN_AS(nodep->lhsp(), Concat)->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت rhs2p = rhsp->clone_ونه_خالص(خطأ);
            // Calc widths
            ثابت صحيح lsb2 = 0;
            ثابت صحيح msb2 = lsb2 + lc2p->پلنوالی() - 1;
            ثابت صحيح lsb1 = msb2 + 1;
            ثابت صحيح msb1 = lsb1 + lc1p->پلنوالی() - 1;
            UASSERT_OBJ(msb1 == (conp->پلنوالی() - 1), nodep, "Width calc mismatch");
            // Form ranges
            AstSel* ثابت sel1p = جديد AstSel{conp->fileline(), rhsp, lsb1, msb1 - lsb1 + 1};
            AstSel* ثابت sel2p = جديد AstSel{conp->fileline(), rhs2p, lsb2, msb2 - lsb2 + 1};
            // Make new assigns of same flavor as old one
            //*** Not cloneTree; just one node.
            Ast_غوټه_ټاکل* newp = تش_اشاره;
            إذا (!need_لنډمهاله) {
                Ast_غوټه_ټاکل* ثابت asn1ap = nodep->cloneType(lc1p, sel1p);
                Ast_غوټه_ټاکل* ثابت asn2ap = nodep->cloneType(lc2p, sel2p);
                asn1ap->dtypeFrom(sel1p);
                asn2ap->dtypeFrom(sel2p);
                newp = Ast_غوټه::جمع_کړه_بل(newp, asn1ap);
                newp = Ast_غوټه::جمع_کړه_بل(newp, asn2ap);
            } وإلا {
                UASSERT_OBJ(m_modp, nodep, "Not under module");
                UASSERT_OBJ(m_globalPass, nodep,
                            "Should not reach here when not invoked on whole AstNetlist");
                // We could create just one temp variable, but we'll get better optimization
                // if we make one per term.
                AstVar* ثابت temp1p
                    = جديد AstVar{sel1p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswap_نومونه.get(sel1p), VFlagLogicPacked{}, msb1 - lsb1 + 1};
                AstVar* ثابت temp2p
                    = جديد AstVar{sel2p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswap_نومونه.get(sel2p), VFlagLogicPacked{}, msb2 - lsb2 + 1};
                m_modp->جمع_کړه_Stmtsp(temp1p);
                m_modp->جمع_کړه_Stmtsp(temp2p);
                Ast_غوټه_ټاکل* ثابت asn1ap = nodep->cloneType(
                    جديد AstVarRef{sel1p->fileline(), temp1p, VAccess::WRITE}, sel1p);
                Ast_غوټه_ټاکل* ثابت asn2ap = nodep->cloneType(
                    جديد AstVarRef{sel2p->fileline(), temp2p, VAccess::WRITE}, sel2p);
                Ast_غوټه_ټاکل* ثابت asn1bp = nodep->cloneType(
                    lc1p, جديد AstVarRef{sel1p->fileline(), temp1p, VAccess::READ});
                Ast_غوټه_ټاکل* ثابت asn2bp = nodep->cloneType(
                    lc2p, جديد AstVarRef{sel2p->fileline(), temp2p, VAccess::READ});
                asn1ap->dtypeFrom(temp1p);
                asn1bp->dtypeFrom(temp1p);
                asn2ap->dtypeFrom(temp2p);
                asn2bp->dtypeFrom(temp2p);
                // This order matters
                newp = Ast_غوټه::جمع_کړه_بل(newp, asn1ap);
                newp = Ast_غوټه::جمع_کړه_بل(newp, asn2ap);
                newp = Ast_غوټه::جمع_کړه_بل(newp, asn1bp);
                newp = Ast_غوټه::جمع_کړه_بل(newp, asn2bp);
            }
            إذا (debug() >= 9 && newp) newp->dump_ونه_And_بل(وتنه, "-     _new: ");
            nodep->جمع_کړه_بل_دلته(newp);
            // Cleanup
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            VL_DO_DANGLING(ټېل_Deletep(conp), conp);
            // Further reduce, either node may have more reductions.
            أرجع ریښتیا;
        } وإلا إذا (m_doV && VN_IS(nodep->rhsp(), Cvt_Packed_منزل_Array)
                   && ښکتنی_As_Fixed_Aggregate(nodep->lhsp()->dtypep())) {
            Ast_Cvt_Packed_منزل_Array* ثابت cvtp
                = VN_AS(nodep->rhsp(), Cvt_Packed_منزل_Array)->unlink_Fr_شا();
            Ast_غوټه_بیان* srcp = cvtp->fromp()->unlink_Fr_شا();
            إذا (ښکتنی_As_Fixed_Aggregate(srcp->dtypep())) {
                srcp = packFixedAggregate(srcp);
            } وإلا إذا (Ast_غوټه_بهیر* ثابت streamp = VN_CAST(srcp, غوټه_بهیر)) {
                Ast_غوټه_بیان* ثابت بهیر_Srcp = streamp->lhsp();
                إذا (ښکتنی_As_Fixed_Aggregate(بهیر_Srcp->dtypep())) {
                    Ast_غوټه_بیان* ثابت packedp = packFixedAggregate(بهیر_Srcp->unlink_Fr_شا());
                    streamp->lhsp(packedp);
                    streamp->dtype_سېټ_بټ_Or_Logic_Unsized(packedp->پلنوالی(), packedp->پلنوالی_Min(),
                                                       VSigning::UNSIGNED,
                                                       packedp->isFourstateOrNoDtype());
                }
            }
            VL_DO_DANGLING(ټېل_Deletep(cvtp), cvtp);
            replace_ټاکل_منزل_Fixed_Aggregate(nodep, nodep->lhsp()->unlink_Fr_شا(), srcp);
            أرجع ریښتیا;
        } وإلا إذا (m_doV && VN_IS(nodep->rhsp(), بهیر_R)
                   && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), کتار_DType)) {
            // The right-streaming operator on rhs of assignment does not
            // change the order of bits. Eliminate stream but keep its lhsp.
            // Add a cast if needed.
            Ast_بهیر_R* ثابت streamp = VN_AS(nodep->rhsp(), بهیر_R)->unlink_Fr_شا();
            Ast_غوټه_بیان* srcp = streamp->lhsp()->unlink_Fr_شا();
            Ast_غوټه_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            ثابت Ast_غوټه_DType* ثابت dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            إذا (ښکتنی_As_Fixed_Aggregate(srcDTypep)) {
                srcp = packFixedAggregate(srcp);
            } وإلا إذا (VN_IS(srcDTypep, کتار_DType) || VN_IS(srcDTypep, DynArrayDType)) {
                إذا (VN_IS(dstDTypep, کتار_DType) || VN_IS(dstDTypep, DynArrayDType)) {
                    صحيح src_Element_بټونه = 0;
                    إذا (ثابت Ast_غوټه_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                        src_Element_بټونه = elemDtp->پلنوالی();
                    }
                    صحيح dst_Element_بټونه = 0;
                    إذا (ثابت Ast_غوټه_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_بټونه = elemDtp->پلنوالی();
                    }
                    srcp = جديد Ast_Cvt_Array_منزل_Array{
                        srcp->fileline(), srcp,          nodep->dtypep(), خطأ, 1,
                        dst_Element_بټونه,   src_Element_بټونه};
                } وإلا {
                    srcp = جديد Ast_Cvt_Array_منزل_Packed{srcp->fileline(), srcp, nodep->dtypep()};
                }
            } وإلا إذا (VN_IS(srcDTypep, UnpackArrayDType)) {
                srcp = جديد Ast_Cvt_Array_منزل_Packed{srcp->fileline(), srcp, srcDTypep};
                // Handling the case where lhs is wider than rhs by inserting zeros. StreamL does
                // not require this, since the left streaming operator implicitly handles this.
                ثابت صحيح packed_بټونه = nodep->lhsp()->پلنوالی_Min();
                ثابت صحيح unpack_بټونه
                    = srcDTypep->arrayUnpackedElements() * srcDTypep->subDTypep()->پلنوالی_Min();
                ثابت uint32_t دننه_واټن = packed_بټونه > unpack_بټونه ? packed_بټونه - unpack_بټونه : 0;
                srcp = جديد Ast_ښويدنه_L{srcp->fileline(), srcp,
                                     جديد AstConst{srcp->fileline(), دننه_واټن}, packed_بټونه};
            }
            إذا (!VN_IS(dstDTypep, UnpackArrayDType) && !VN_IS(dstDTypep, کتار_DType)
                && !VN_IS(dstDTypep, DynArrayDType)) {
                ثابت صحيح s_پلنوالی = srcp->پلنوالی();
                ثابت صحيح d_پلنوالی = nodep->lhsp()->پلنوالی();
                إذا (s_پلنوالی < d_پلنوالی) {
                    AstExtend* ثابت extendp = جديد AstExtend{srcp->fileline(), srcp, d_پلنوالی};
                    srcp = جديد Ast_ښويدنه_L{
                        srcp->fileline(), extendp,
                        جديد AstConst{srcp->fileline(), static_cast<uint32_t>(d_پلنوالی - s_پلنوالی)},
                        d_پلنوالی};
                }
            }
            nodep->rhsp(srcp);
            VL_DO_DANGLING(ټېل_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            أرجع ریښتیا;
        } وإلا إذا (m_doV && VN_IS(nodep->lhsp(), بهیر_L)) {
            // Push the stream operator to the rhs of the assignment statement
            Ast_غوټه_بیان* streamp = nodep->lhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت dstp = VN_AS(streamp, بهیر_L)->lhsp()->unlink_Fr_شا();
            Ast_غوټه_DType* ثابت dstDTypep = dstp->dtypep()->skipRefp();
            Ast_غوټه_بیان* srcp = nodep->rhsp()->unlink_Fr_شا();
            ثابت Ast_غوټه_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamL reverses, so reverse=true)
            إذا ((VN_IS(srcDTypep, UnpackArrayDType) || VN_IS(srcDTypep, کتار_DType)
                 || VN_IS(srcDTypep, DynArrayDType))
                && (VN_IS(dstDTypep, کتار_DType) || VN_IS(dstDTypep, DynArrayDType))) {
                صحيح بلاک_اندازه = 1;
                إذا (ثابت AstConst* ثابت constp
                    = VN_CAST(VN_AS(streamp, بهیر_L)->rhsp(), Const)) {
                    بلاک_اندازه = constp->منزل_SInt();
                    إذا (VL_UNLIKELY(بلاک_اندازه <= 0)) بلاک_اندازه = 1;
                }
                صحيح src_Element_بټونه = 0;
                إذا (ثابت Ast_غوټه_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                    src_Element_بټونه = elemDtp->پلنوالی();
                }
                صحيح dst_Element_بټونه = 0;
                إذا (ثابت Ast_غوټه_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                    dst_Element_بټونه = elemDtp->پلنوالی();
                }
                nodep->lhsp(dstp);
                nodep->rhsp(جديد Ast_Cvt_Array_منزل_Array{srcp->fileline(), srcp, dstDTypep, ریښتیا,
                                                   بلاک_اندازه, dst_Element_بټونه, src_Element_بټونه});
                nodep->dtypep(dstDTypep);
                VL_DO_DANGLING(ټېل_Deletep(streamp), streamp);
                أرجع ریښتیا;
            }
            إذا (ښکتنی_As_Fixed_Aggregate(srcDTypep)) srcp = packFixedAggregate(srcp);
            ثابت صحيح s_پلنوالی = srcp->پلنوالی();
            ثابت صحيح d_پلنوالی = dstp->پلنوالی();
            // Connect the rhs to the stream operator and update its width
            VN_AS(streamp, بهیر_L)->lhsp(srcp);
            إذا (VN_IS(srcDTypep, DynArrayDType) || VN_IS(srcDTypep, کتار_DType)
                || VN_IS(srcDTypep, UnpackArrayDType)) {
                streamp->dtype_سېټ_بهیر();
            } وإلا {
                streamp->dtype_سېټ_بټ_Or_Logic_Unsized(srcp->پلنوالی(), srcp->پلنوالی_Min(),
                                                   VSigning::UNSIGNED,
                                                   srcp->isFourstateOrNoDtype());
            }
            إذا (VN_IS(dstDTypep, UnpackArrayDType)) {
                streamp = جديد Ast_Cvt_Packed_منزل_Array{nodep->fileline(), streamp, dstDTypep};
            } وإلا {
                إذا (d_پلنوالی == 0) {
                    streamp = جديد Ast_Cvt_Packed_منزل_Array{nodep->fileline(), streamp, dstDTypep};
                } وإلا إذا (s_پلنوالی >= d_پلنوالی) {
                    streamp = جديد AstSel{streamp->fileline(), streamp, s_پلنوالی - d_پلنوالی, d_پلنوالی};
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(streamp);
            nodep->dtypep(dstDTypep);
            أرجع ریښتیا;
        } وإلا إذا (m_doV && VN_IS(nodep->lhsp(), بهیر_R)) {
            // The right stream operator on lhs of assignment statement does
            // not reorder bits. However, if the rhs is wider than the lhs,
            // then we select bits from the left-most, not the right-most.
            Ast_غوټه_بیان* ثابت streamp = nodep->lhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت dstp = VN_AS(streamp, بهیر_R)->lhsp()->unlink_Fr_شا();
            Ast_غوټه_DType* ثابت dstDTypep = dstp->dtypep()->skipRefp();
            Ast_غوټه_بیان* srcp = nodep->rhsp()->unlink_Fr_شا();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamR does not reverse, so reverse=false).
            // V3Width may have wrapped the source in CvtArrayToPacked; unwrap it.
            إذا (VN_IS(dstDTypep, کتار_DType) || VN_IS(dstDTypep, DynArrayDType)) {
                Ast_غوټه_بیان* origSrcp = srcp;
                إذا (Ast_Cvt_Array_منزل_Packed* ثابت cvtp = VN_CAST(srcp, Cvt_Array_منزل_Packed)) {
                    origSrcp = cvtp->fromp();
                }
                ثابت Ast_غوټه_DType* ثابت origSrcDTypep = origSrcp->dtypep()->skipRefp();
                إذا (VN_IS(origSrcDTypep, UnpackArrayDType) || VN_IS(origSrcDTypep, کتار_DType)
                    || VN_IS(origSrcDTypep, DynArrayDType)) {
                    صحيح src_Element_بټونه = 0;
                    إذا (ثابت Ast_غوټه_DType* ثابت elemDtp = origSrcDTypep->subDTypep()) {
                        src_Element_بټونه = elemDtp->پلنوالی();
                    }
                    صحيح dst_Element_بټونه = 0;
                    إذا (ثابت Ast_غوټه_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_بټونه = elemDtp->پلنوالی();
                    }
                    إذا (VN_IS(srcp, Cvt_Array_منزل_Packed)) {
                        origSrcp = VN_AS(srcp, Cvt_Array_منزل_Packed)->fromp()->unlink_Fr_شا();
                        VL_DO_DANGLING(ټېل_Deletep(srcp), srcp);
                        srcp = origSrcp;
                    }
                    // Descending unpacked arrays need element reversal
                    منطقي سرچپه = خطأ;
                    إذا (ثابت AstUnpackArrayDType* ثابت unpackDtp
                        = VN_CAST(origSrcDTypep, UnpackArrayDType)) {
                        سرچپه = !unpackDtp->declRange().ascending();
                    }
                    nodep->lhsp(dstp);
                    nodep->rhsp(جديد Ast_Cvt_Array_منزل_Array{srcp->fileline(), srcp, dstDTypep, سرچپه,
                                                       1, dst_Element_بټونه, src_Element_بټونه});
                    nodep->dtypep(dstDTypep);
                    VL_DO_DANGLING(ټېل_Deletep(streamp), streamp);
                    أرجع ریښتیا;
                }
            }
            ثابت صحيح s_پلنوالی = srcp->پلنوالی();
            ثابت صحيح d_پلنوالی = dstp->پلنوالی();
            إذا (VN_IS(dstDTypep, UnpackArrayDType)) {
                ثابت صحيح dst_بټ_پلنوالی
                    = d_پلنوالی * VN_AS(dstDTypep, UnpackArrayDType)->arrayUnpackedElements();
                // Handling the case where rhs is wider than lhs. StreamL does not require this
                // since the combination of the left streaming operation and the implicit
                // truncation in VL_ASSIGN_UNPACK automatically selects the left-most bits.
                إذا (s_پلنوالی > dst_بټ_پلنوالی) {
                    srcp
                        = جديد AstSel{streamp->fileline(), srcp, s_پلنوالی - dst_بټ_پلنوالی, dst_بټ_پلنوالی};
                }
                srcp = جديد Ast_Cvt_Packed_منزل_Array{nodep->fileline(), srcp, dstDTypep};
            } وإلا {
                إذا (d_پلنوالی == 0) {
                    srcp = جديد Ast_Cvt_Packed_منزل_Array{nodep->fileline(), srcp, dstDTypep};
                } وإلا إذا (s_پلنوالی >= d_پلنوالی) {
                    srcp = جديد AstSel{streamp->fileline(), srcp, s_پلنوالی - d_پلنوالی, d_پلنوالی};
                } وإلا {
                    // Source narrower than destination: left-justify by shifting left.
                    // The right stream operator packs left-to-right, so remaining
                    // LSBs are zero-filled (IEEE 1800-2023 11.4.14.3).
                    إذا (!VN_IS(srcp->dtypep()->skipRefp(), کتار_DType)) {
                        AstExtend* ثابت extendp = جديد AstExtend{srcp->fileline(), srcp, d_پلنوالی};
                        srcp = جديد Ast_ښويدنه_L{
                            srcp->fileline(), extendp,
                            جديد AstConst{srcp->fileline(), static_cast<uint32_t>(d_پلنوالی - s_پلنوالی)},
                            d_پلنوالی};
                    } وإلا {  // if it is a queue we dont need do do a shiftL
                        srcp = جديد AstExtend{srcp->fileline(), srcp};
                    }
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(srcp);
            nodep->dtypep(dstDTypep);
            VL_DO_DANGLING(ټېل_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            أرجع ریښتیا;
        } وإلا إذا (m_doV && VN_IS(nodep->rhsp(), بهیر_L)) {
            Ast_بهیر_L* streamp = VN_AS(nodep->rhsp(), بهیر_L);
            Ast_غوټه_بیان* srcp = streamp->lhsp();
            ثابت Ast_غوټه_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            Ast_غوټه_DType* ثابت dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            إذا (ښکتنی_As_Fixed_Aggregate(srcDTypep)) {
                Ast_غوټه_بیان* ثابت packedp = packFixedAggregate(srcp->unlink_Fr_شا());
                streamp->lhsp(packedp);
                streamp->dtype_سېټ_بټ_Or_Logic_Unsized(packedp->پلنوالی(), packedp->پلنوالی_Min(),
                                                   VSigning::UNSIGNED,
                                                   packedp->isFourstateOrNoDtype());
                srcp = packedp;
            } وإلا إذا ((VN_IS(srcDTypep, کتار_DType) || VN_IS(srcDTypep, DynArrayDType)
                        || VN_IS(srcDTypep, UnpackArrayDType))) {
                إذا (VN_IS(dstDTypep, کتار_DType) || VN_IS(dstDTypep, DynArrayDType)) {
                    صحيح بلاک_اندازه = 1;
                    إذا (ثابت AstConst* ثابت constp = VN_CAST(streamp->rhsp(), Const)) {
                        بلاک_اندازه = constp->منزل_SInt();
                        إذا (VL_UNLIKELY(بلاک_اندازه <= 0)) {
                            // Not reachable due to higher level checks when parsing stream
                            // operators commented out to not fail v3error-coverage-checks.
                            // nodep->v3error("Stream block size must be positive, got " <<
                            // blockSize); nodep->v3error("Stream block size must be positive, got
                            // " << blockSize);
                            بلاک_اندازه = 1;
                        }
                    }
                    // Not reachable due to higher level checks when parsing stream operators
                    // commented out to not fail v3error-coverage-checks.
                    // else {
                    // nodep->v3error("Stream block size must be constant (got " <<
                    // streamp->rhsp()->prettyTypeName() << ")");
                    // }
                    صحيح src_Element_بټونه = 0;
                    إذا (ثابت Ast_غوټه_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                        src_Element_بټونه = elemDtp->پلنوالی();
                    }
                    صحيح dst_Element_بټونه = 0;
                    إذا (ثابت Ast_غوټه_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_بټونه = elemDtp->پلنوالی();
                    }
                    streamp->unlink_Fr_شا();
                    srcp = جديد Ast_Cvt_Array_منزل_Array{
                        srcp->fileline(), srcp->unlink_Fr_شا(), dstDTypep,     ریښتیا,
                        بلاک_اندازه,        dst_Element_بټونه,       src_Element_بټونه};
                    nodep->rhsp(srcp);
                    VL_DO_DANGLING(ټېل_Deletep(streamp), streamp);
                } وإلا {
                    streamp->lhsp(جديد Ast_Cvt_Array_منزل_Packed{srcp->fileline(), srcp->unlink_Fr_شا(),
                                                          dstDTypep});
                    streamp->dtypeFrom(dstDTypep);
                }
            }
        } وإلا إذا (m_doV && replace_ټاکل_Multi_Sel(nodep)) {
            أرجع ریښتیا;
        } وإلا إذا (replace_ټاکل_SFormat_F(nodep)) {
            أرجع ریښتیا;
        }
        أرجع خطأ;
    }

    // Boolean replacements
    منطقي عملوند_Bool_ښويدنه(ثابت Ast_غوټه* nodep) {
        // boolean test of AND(const,SHIFTR(x,const)) -> test of AND(SHIFTL(x,const), x)
        إذا (!VN_IS(nodep, And)) أرجع خطأ;
        إذا (!VN_IS(VN_AS(nodep, And)->lhsp(), Const)) أرجع خطأ;
        إذا (!VN_IS(VN_AS(nodep, And)->rhsp(), ښويدنه_R)) أرجع خطأ;
        ثابت Ast_ښويدنه_R* ثابت shiftp = VN_AS(VN_AS(nodep, And)->rhsp(), ښويدنه_R);
        إذا (!VN_IS(shiftp->rhsp(), Const)) أرجع خطأ;
        إذا (static_cast<uint32_t>(nodep->پلنوالی()) <= VN_AS(shiftp->rhsp(), Const)->منزل_UInt()) {
            أرجع خطأ;
        }
        أرجع ریښتیا;
    }
    فراغ replace_Bool_ښويدنه(Ast_غوټه* nodep) {
        UINFOTREE(9, nodep, "", "bshft_old");
        AstConst* ثابت andConstp = VN_AS(VN_AS(nodep, And)->lhsp(), Const);
        Ast_غوټه_بیان* ثابت fromp
            = VN_AS(VN_AS(nodep, And)->rhsp(), ښويدنه_R)->lhsp()->unlink_Fr_شا();
        AstConst* ثابت ښويدنه_Constp
            = VN_AS(VN_AS(VN_AS(nodep, And)->rhsp(), ښويدنه_R)->rhsp(), Const);
        V3_شمېره val{andConstp, andConstp->پلنوالی()};
        val.op_ښويدنه_L(andConstp->عدد(), ښويدنه_Constp->عدد());
        AstAnd* ثابت newp
            = جديد AstAnd{nodep->fileline(), جديد AstConst{nodep->fileline(), val}, fromp};
        // widthMin no longer applicable if different C-expanded width
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        UINFOTREE(9, newp, "", "_new");
    }

    فراغ replaceWithSimulation(Ast_غوټه* nodep) {
        SimulateVisitor simvis;
        // Run it - may be unoptimizable due to large for loop, etc
        simvis.mainParamEmulate(nodep);
        إذا (!simvis.optimizable()) {
            ثابت Ast_غوټه* errorp = simvis.whyNotNodep();
            إذا (!errorp) errorp = nodep;
            nodep->v3error("Expecting expression to be constant, but can't determine constant for "
                           << nodep->prettyTypeName() << '\n'
                           << errorp->warnOther() << "... Location of non-constant "
                           << errorp->prettyTypeName() << ": " << simvis.why_Not_پیغام());
            VL_DO_DANGLING(replaceZero(nodep), nodep);
        } وإلا {
            // Fetch the result
            Ast_غوټه* ثابت valuep = simvis.fetch_ارزښت_Null(nodep);  // valuep is owned by Simulate
            UASSERT_OBJ(valuep, nodep, "No value returned from simulation");
            // Replace it
            Ast_غوټه* ثابت newp = valuep->clone_ونه(خطأ);
            newp->fileline(nodep->fileline());
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Simulate->" << newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
    }

    //----------------------------------------

    // VISITORS
    فراغ لیدنه(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        iterateChildrenBackwardsConst(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_concswap_نومونه.reset();
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCFunc* nodep) override {
        // No ASSIGNW removals under funcs, we've long eliminated INITIALs
        // (We should perhaps rename the assignw's to just assigns)
        VL_RESTORER(m_wremove);
        m_wremove = خطأ;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCLocalScope* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->stmtsp()) {
            nodep->unlink_Fr_شا();
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
    }
    فراغ لیدنه(AstScope* nodep) override {
        // No ASSIGNW removals under scope, we've long eliminated INITIALs
        VL_RESTORER(m_wremove);
        VL_RESTORER(m_scopep);
        m_wremove = خطأ;
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    فراغ بدلون_Sides(Ast_غوټه_Bi_Com* nodep) {
        // COMMUTATIVE({a},CONST) -> COMMUTATIVE(CONST,{a})
        // This simplifies later optimizations
        Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا_With_بل();
        Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا_With_بل();
        nodep->lhsp(rhsp);
        nodep->rhsp(lhsp);
        iterate(nodep);  // Again?
    }

    منطقي لري_Member_Access_Recurse(Ast_غوټه* ثابت nodep) {
        إذا (!nodep) أرجع خطأ;
        ثابت اندازه_t id = m_ids(nodep);
        ثابت خپلکاری it = m_لري_Member_Access.پیدا_کړه(id);
        إذا (it != m_لري_Member_Access.پای_ټکی()) أرجع it->دوهم;
        ++m_statMemberAccessVisits;
        منطقي پایله = خطأ;
        إذا (VN_IS(nodep, MemberSel) || VN_IS(nodep, لاره_Call) || VN_IS(nodep, CMethodCall)) {
            پایله = ریښتیا;
        } وإلا إذا (ثابت Ast_غوټه_FTask_Ref* ثابت funcRefp = VN_CAST(nodep, غوټه_FTask_Ref)) {
            إذا (لري_Member_Access_Recurse(funcRefp->taskp())) پایله = ریښتیا;
        } وإلا إذا (ثابت Ast_غوټه_CCall* ثابت funcRefp = VN_CAST(nodep, غوټه_CCall)) {
            إذا (لري_Member_Access_Recurse(funcRefp->funcp())) پایله = ریښتیا;
        } وإلا إذا (ثابت Ast_غوټه_FTask* ثابت funcp = VN_CAST(nodep, غوټه_FTask)) {
            // Assume that it has a member access
            إذا (funcp->recursive()) پایله = ریښتیا;
        } وإلا إذا (ثابت AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc)) {
            إذا (funcp->recursive()) پایله = ریښتیا;
        }
        إذا (!پایله) {
            پایله = لري_Member_Access_Recurse(nodep->op1p())
                     || لري_Member_Access_Recurse(nodep->op2p())
                     || لري_Member_Access_Recurse(nodep->op3p())
                     || لري_Member_Access_Recurse(nodep->op4p());
        }
        إذا (!پایله && !VN_IS(nodep, غوټه_FTask)
            && !VN_IS(nodep, CFunc)  // don't enter into next function
            && لري_Member_Access_Recurse(nodep->nextp())) {
            پایله = ریښتیا;
        }
        m_لري_Member_Access.emplace(id, پایله);
        أرجع پایله;
    }

    منطقي سمون_Biop_منزل_Bitwise(Ast_غوټه_Biop* ثابت nodep) {
        إذا (!m_convert_Logic_منزل_بټ) أرجع خطأ;
        إذا (!nodep->lhsp()->width1()) أرجع خطأ;
        إذا (!nodep->rhsp()->width1()) أرجع خطأ;
        إذا (!nodep->is_خالص()) أرجع خطأ;
        إذا (لري_Member_Access_Recurse(nodep)) أرجع خطأ;
        أرجع ریښتیا;
    }
    منطقي سمون_Concat_Rand(AstConcat* nodep) {
        //    CONCAT(RAND, RAND) - created by Chisel code
        AstRand* ثابت aRandp = VN_CAST(nodep->lhsp(), Rand);
        ثابت AstRand* ثابت bRandp = VN_CAST(nodep->rhsp(), Rand);
        إذا (!aRandp || !bRandp) أرجع خطأ;
        إذا (!aRandp->combinable(bRandp)) أرجع خطأ;
        UINFO(4, "Concat(Rand,Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlink_Fr_شا());
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        أرجع ریښتیا;
    }
    منطقي سمون_Sel_Rand(AstSel* nodep) {
        //    SEL(RAND) - created by Chisel code
        AstRand* ثابت aRandp = VN_CAST(nodep->fromp(), Rand);
        إذا (!aRandp) أرجع خطأ;
        إذا (aRandp->seedp()) أرجع خطأ;
        UINFO(4, "Sel(Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlink_Fr_شا());
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        أرجع ریښتیا;
    }
    منطقي سمون_منزل_String_NConst(Ast_منزل_String_N* nodep) {
        iterateChildren(nodep);
        إذا (ثابت AstConsArray* ثابت consp = VN_CAST(nodep->lhsp(), ConsArray)) {
            إذا (!(m_doExpensive || m_params)) أرجع خطأ;
            إذا (!consp->is_Constant_ونه()) أرجع خطأ;
        } وإلا إذا (!VN_IS(nodep->lhsp(), Const)) {
            أرجع خطأ;
        }
        replaceWithSimulation(nodep);
        أرجع ریښتیا;
    }
    صحيح عملوند_Concat_Move(ثابت AstConcat* nodep) {
        //    CONCAT under concat  (See moveConcat)
        // Return value: true indicates to do it; 2 means move to LHS
        ثابت AstConcat* ثابت abConcp = VN_CAST(nodep->lhsp(), Concat);
        ثابت AstConcat* ثابت bcConcp = VN_CAST(nodep->rhsp(), Concat);
        إذا (!abConcp && !bcConcp) أرجع 0;
        إذا (bcConcp) {
            ثابت Ast_غوټه_بیان* ثابت ap = nodep->lhsp();
            ثابت Ast_غوټه_بیان* ثابت bp = bcConcp->lhsp();
            // If a+b == 32,64,96 etc, then we want to have a+b together on LHS
            إذا (VL_BITBIT_I(ap->پلنوالی() + bp->پلنوالی()) == 0) أرجع 2;  // Transform 2: to abConc
        } وإلا {  // abConcp
            // Unless lhs is already 32 bits due to above, reorder it
            إذا (VL_BITBIT_I(nodep->lhsp()->پلنوالی()) != 0) أرجع 1;  // Transform 1: to bcConc
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
        إذا (عملوند_Concat_Move(nodep) > 1) {
            Ast_غوټه_بیان* ثابت ap = nodep->lhsp()->unlink_Fr_شا();
            AstConcat* ثابت bcConcp = VN_AS(nodep->rhsp(), Concat);
            bcConcp->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت bp = bcConcp->lhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت cp = bcConcp->rhsp()->unlink_Fr_شا();
            AstConcat* ثابت abConcp = جديد AstConcat{bcConcp->fileline(), ap, bp};
            nodep->lhsp(abConcp);
            nodep->rhsp(cp);
            // If bp was a concat, then we have this exact same form again!
            // Recurse rather then calling node->iterate to prevent 2^n recursion!
            إذا (عملوند_Concat_Move(abConcp)) moveConcat(abConcp);
            VL_DO_DANGLING(ټېل_Deletep(bcConcp), bcConcp);
        } وإلا {
            AstConcat* ثابت abConcp = VN_AS(nodep->lhsp(), Concat);
            abConcp->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت ap = abConcp->lhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت bp = abConcp->rhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت cp = nodep->rhsp()->unlink_Fr_شا();
            AstConcat* ثابت bcConcp = جديد AstConcat{abConcp->fileline(), bp, cp};
            nodep->lhsp(ap);
            nodep->rhsp(bcConcp);
            إذا (عملوند_Concat_Move(bcConcp)) moveConcat(bcConcp);
            VL_DO_DANGLING(ټېل_Deletep(abConcp), abConcp);
        }
    }

    // Special cases
    فراغ لیدنه(AstConst*) override {}  // Already constant

    فراغ لیدنه(AstCell* nodep) override {
        إذا (m_params) {
            iterate_And_بل_Null(nodep->paramsp());
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ لیدنه(AstClassOrPackageRef* nodep) override { iterateChildren(nodep); }

    فراغ لیدنه(Ast_سمون_Masked* nodep) override {
        // Do not iterate the tables, they must be constant pool entries
        iterate(nodep->lhsp());
        إذا (AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const)) {
            replace_عدد(nodep, Ast_سمون_Masked::fold(constp->عدد(), nodep->matchp()->varp()));
        }
    }

    فراغ لیدنه(AstPin* nodep) override { iterateChildren(nodep); }

    فراغ replaceLogEq(AstLogEq* nodep) {
        // LOGEQ(a,b) => AstLogAnd{AstLogOr{AstLogNot{a},b},AstLogOr{AstLogNot{b},a}}
        Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
        // Do exactly as IEEE says, might result in extra terms, so in future may do differently
        AstLogAnd* ثابت newp = جديد AstLogAnd{
            nodep->fileline(),
            جديد AstLogOr{nodep->fileline(), جديد AstLogNot{nodep->fileline(), lhsp}, rhsp},
            جديد AstLogOr{nodep->fileline(),
                         جديد AstLogNot{nodep->fileline(), rhsp->clone_ونه_خالص(خطأ)},
                         lhsp->clone_ونه_خالص(خطأ)}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ replaceSelSel(AstSel* nodep) {
        // SEL(SEL({x},a,b),c,d) => SEL({x},a+c,d)
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ثابت belowp = VN_AS(nodep->fromp(), Sel);
        Ast_غوټه_بیان* ثابت fromp = belowp->fromp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت lsb1p = nodep->lsbp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت lsb2p = belowp->lsbp()->unlink_Fr_شا();
        // Eliminate lower range
        UINFO(4, "Elim Lower range: " << nodep);
        Ast_غوټه_بیان* newlsbp;
        إذا (VN_IS(lsb1p, Const) && VN_IS(lsb2p, Const)) {
            newlsbp = جديد AstConst{lsb1p->fileline(),
                                   VN_AS(lsb1p, Const)->منزل_UInt() + VN_AS(lsb2p, Const)->منزل_UInt()};
            VL_DO_DANGLING(ټېل_Deletep(lsb1p), lsb1p);
            VL_DO_DANGLING(ټېل_Deletep(lsb2p), lsb2p);
        } وإلا {
            // Width is important, we need the width of the fromp's expression, not the
            // potentially smaller lsb1p's width, but don't insert a redundant AstExtend.
            // Note that due to some sloppiness in earlier passes, lsb1p might actually be wider,
            // so extend to the wider type.
            ثابت Ast_غوټه_بیان* ثابت widep = lsb1p->پلنوالی() > lsb2p->پلنوالی() ? lsb1p : lsb2p;
            Ast_غوټه_بیان* ثابت lhsp = widep->پلنوالی() > lsb2p->پلنوالی()
                                          ? جديد AstExtend{lsb2p->fileline(), lsb2p}
                                          : lsb2p;
            Ast_غوټه_بیان* ثابت rhsp = widep->پلنوالی() > lsb1p->پلنوالی()
                                          ? جديد AstExtend{lsb1p->fileline(), lsb1p}
                                          : lsb1p;
            lhsp->dtypeFrom(widep);
            rhsp->dtypeFrom(widep);
            newlsbp = جديد Ast_جمع_کړه{lsb1p->fileline(), lhsp, rhsp};
            newlsbp->dtypeFrom(widep);
        }
        AstSel* ثابت newp = جديد AstSel{nodep->fileline(), fromp, newlsbp, nodep->پلنوالی_Const()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ replaceSelConcat(AstSel* nodep) {
        // SEL(CONCAT(a,b),c,d) => SEL(a or b, . .)
        AstConcat* ثابت conp = VN_AS(nodep->fromp(), Concat);
        Ast_غوټه_بیان* ثابت conLhsp = conp->lhsp();
        Ast_غوټه_بیان* ثابت conRhsp = conp->rhsp();
        إذا (static_cast<صحيح>(nodep->lsbConst()) >= conRhsp->پلنوالی()) {
            conLhsp->unlink_Fr_شا();
            AstSel* ثابت newp
                = جديد AstSel{nodep->fileline(), conLhsp, nodep->lsbConst() - conRhsp->پلنوالی(),
                             nodep->پلنوالی_Const()};
            nodep->replaceWithKeepDType(newp);
        } وإلا إذا (static_cast<صحيح>(nodep->msbConst()) < conRhsp->پلنوالی()) {
            conRhsp->unlink_Fr_شا();
            AstSel* ثابت newp
                = جديد AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(), nodep->پلنوالی_Const()};
            nodep->replaceWithKeepDType(newp);
        } وإلا {
            // Yuk, split between the two
            conRhsp->unlink_Fr_شا();
            conLhsp->unlink_Fr_شا();
            AstConcat* ثابت newp
                = جديد AstConcat{nodep->fileline(),
                                جديد AstSel{nodep->fileline(), conLhsp, 0,
                                           nodep->msbConst() - conRhsp->پلنوالی() + 1},
                                جديد AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(),
                                           conRhsp->پلنوالی() - nodep->lsbConst()}};
            nodep->replaceWithKeepDType(newp);
        }
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    منطقي عملوند_Sel_Replicate(AstSel* nodep) {
        // SEL(REPLICATE(from,rep),lsb,width) => SEL(from,0,width) as long
        // as SEL's width <= b's width
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ثابت repp = VN_AS(nodep->fromp(), Replicate);
        Ast_غوټه_بیان* ثابت fromp = repp->srcp();
        ثابت AstConst* ثابت lsbp = VN_CAST(nodep->lsbp(), Const);
        إذا (!lsbp) أرجع خطأ;
        UASSERT_OBJ(fromp->پلنوالی(), nodep, "Not widthed");
        إذا ((lsbp->منزل_UInt() / fromp->پلنوالی())
            != ((lsbp->منزل_UInt() + nodep->پلنوالی() - 1) / fromp->پلنوالی())) {
            أرجع خطأ;
        }
        //
        fromp->unlink_Fr_شا();
        AstSel* ثابت newp = جديد AstSel{
            nodep->fileline(), fromp,
            جديد AstConst{lsbp->fileline(), lsbp->منزل_UInt() % fromp->پلنوالی()}, nodep->پلنوالی_Const()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        أرجع ریښتیا;
    }
    منطقي عملوند_Rep_Rep(AstReplicate* nodep) {
        // REPLICATE(REPLICATE2(from2,cnt2),cnt1) => REPLICATE(from2,(cnt1+cnt2))
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ثابت rep2p = VN_AS(nodep->srcp(), Replicate);
        Ast_غوټه_بیان* ثابت from2p = rep2p->srcp();
        ثابت AstConst* ثابت cnt1p = VN_CAST(nodep->countp(), Const);
        إذا (!cnt1p) أرجع خطأ;
        ثابت AstConst* ثابت cnt2p = VN_CAST(rep2p->countp(), Const);
        إذا (!cnt2p) أرجع خطأ;
        //
        from2p->unlink_Fr_شا();
        AstReplicate* ثابت newp
            = جديد AstReplicate{nodep->fileline(), from2p, cnt1p->منزل_UInt() * cnt2p->منزل_UInt()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        أرجع ریښتیا;
    }
    منطقي عملوند_Concat_Same(AstConcat* nodep) {
        // CONCAT(fromp,fromp) -> REPLICATE(fromp,1+1)
        // CONCAT(REP(fromp,cnt1),fromp) -> REPLICATE(fromp,cnt1+1)
        // CONCAT(fromp,REP(fromp,cnt1)) -> REPLICATE(fromp,1+cnt1)
        // CONCAT(REP(fromp,cnt1),REP(fromp,cnt2)) -> REPLICATE(fromp,cnt1+cnt2)
        Ast_غوټه_بیان* from1p = nodep->lhsp();
        uint32_t cnt1 = 1;
        Ast_غوټه_بیان* from2p = nodep->rhsp();
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
        from1p->unlink_Fr_شا();
        AstReplicate* ثابت newp = جديد AstReplicate{nodep->fileline(), from1p, cnt1 + cnt2};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        أرجع ریښتیا;
    }
    فراغ replaceSelIntoBiop(AstSel* nodep) {
        // SEL(BUFIF1(a,b),1,bit) => BUFIF1(SEL(a,1,bit),SEL(b,1,bit))
        Ast_غوټه_Biop* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_شا(), غوټه_Biop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_غوټه_بیان* ثابت lsbp = nodep->lsbp()->unlink_Fr_شا();
        //
        Ast_غوټه_بیان* ثابت bilhsp = fromp->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت birhsp = fromp->rhsp()->unlink_Fr_شا();
        //
        fromp->lhsp(
            جديد AstSel{nodep->fileline(), bilhsp, lsbp->clone_ونه_خالص(ریښتیا), nodep->پلنوالی_Const()});
        fromp->rhsp(جديد AstSel{nodep->fileline(), birhsp, lsbp, nodep->پلنوالی_Const()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ replaceSelIntoUniop(AstSel* nodep) {
        // SEL(NOT(a),1,bit) => NOT(SEL(a,bit))
        Ast_غوټه_Uniop* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_شا(), غوټه_Uniop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_غوټه_بیان* ثابت lsbp = nodep->lsbp()->unlink_Fr_شا();
        //
        Ast_غوټه_بیان* ثابت bilhsp = fromp->lhsp()->unlink_Fr_شا();
        //
        fromp->lhsp(جديد AstSel{nodep->fileline(), bilhsp, lsbp, nodep->پلنوالی_Const()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ لیدنه(Ast_Attr_د* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstArraySel* nodep) override {
        iterate_And_بل_Null(nodep->bitp());
        إذا (VN_IS(nodep->bitp(), Const)
            && VN_IS(nodep->fromp(), VarRef)
            // Need to make sure it's an array object so don't mis-allow a constant (bug509.)
            && VN_AS(nodep->fromp(), VarRef)->varp()
            && VN_IS(VN_AS(nodep->fromp(), VarRef)->varp()->valuep(), ConsArray)) {
            m_selp = nodep;  // Ask visit(AstVarRef) to replace varref with const
        }
        iterate_And_بل_Null(nodep->fromp());
        إذا (VN_IS(nodep->fromp(), Const)) {  // It did.
            إذا (!m_selp) {
                nodep->v3error("Illegal assignment of constant to unpacked array");
            } وإلا {
                Ast_غوټه* ثابت fromp = nodep->fromp()->unlink_Fr_شا();
                nodep->replaceWithKeepDType(fromp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            }
        }
        // Handle ARRAYSEL directly on ConsArray (not through VarRef)
        وإلا إذا (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ConsArray)) {
            ثابت AstConsArray* ثابت consp = VN_AS(nodep->fromp(), ConsArray);
            ثابت uint32_t بټ_2 = VN_AS(nodep->bitp(), Const)->منزل_UInt();
            Ast_غوټه_بیان* ثابت itemp = consp->valuep(بټ_2);
            إذا (VN_IS(itemp, Const)) {
                ثابت V3_شمېره& عدد = VN_AS(itemp, Const)->عدد();
                VL_DO_DANGLING(replace_عدد(nodep, عدد), nodep);
            } وإلا إذا (m_params && itemp) {
                // An aggregate element, e.g. a structure in an array parameter
                nodep->replaceWithKeepDType(itemp->clone_ونه(خطأ));
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            }
        }
        m_selp = تش_اشاره;
    }

    // Evaluate a slice of an unpacked array.  If constantification is
    // required (m_required=true), call replaceWithSimulation() to compute
    // the slice via simulation.  Otherwise just iterate the children.
    فراغ لیدنه(Ast_ټوټه_Sel* nodep) override {
        // First constify or width any child nodes
        iterateChildren(nodep);
        إذا (!m_required) أرجع;  // Do nothing unless we are in parameter mode
        // Fallback to simulation: this will invoke SimulateVisitor::visit(AstSliceSel*)
        replaceWithSimulation(nodep);
    }

    فراغ لیدنه(AstStructSel* nodep) override {
        iterateChildren(nodep);

        إذا (ثابت AstConsStruct* ثابت consp = VN_CAST(nodep->fromp(), ConsStruct)) {
            Ast_غوټه_بیان* ثابت valuep = consp->memberValuep(nodep->name());
            إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
                VL_DO_DANGLING(replace_عدد(nodep, constp->عدد()), nodep);
            } وإلا {
                nodep->replaceWith(valuep->clone_ونه(خطأ));
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            }
        }
    }

    فراغ لیدنه(AstCAwait* nodep) override {
        m_hasJumpDelay = ریښتیا;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Var_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Not linked");
        منطقي did = خطأ;
        إذا (m_doV && (!nodep->varp()->const_Pool_ثبت() || m_selp) && nodep->varp()->valuep()
            && !m_attrp) {
            // UINFOTREE(1, valuep, "", "visitvaref");
            iterate_And_بل_Null(nodep->varp()->valuep());  // May change nodep->varp()->valuep()
            Ast_غوټه* ثابت valuep = nodep->varp()->valuep();
            // The selected element, when under a constant select of an array
            ثابت AstConsArray* ثابت consp = VN_CAST(valuep, ConsArray);
            ثابت Ast_غوټه* ثابت selItemp
                = (m_selp && consp) ? consp->valuep(m_selp->بټ_Const()) : تش_اشاره;
            إذا (nodep->access().isReadOnly() && valuep
                && ((!m_params  // Can reduce constant wires into equations
                     && m_doNConst && v3Global.opt.fConst() && nodep->varp()->isConst())
                    || nodep->varp()->isParam())) {
                إذا (عملوند_Const(valuep)) {
                    ثابت V3_شمېره& عدد = VN_AS(valuep, Const)->عدد();
                    // UINFO(2, "constVisit " << cvtToHex(valuep) << " " << num);
                    ثابت string origParamName
                        = nodep->varp()->isParam() ? nodep->varp()->name() : "";
                    VL_DO_DANGLING(replace_عدد(nodep, عدد, origParamName), nodep);
                    did = ریښتیا;
                } وإلا إذا (ثابت AstConst* ثابت itemp = VN_CAST(selItemp, Const)) {
                    VL_DO_DANGLING(replace_عدد(nodep, itemp->عدد()), nodep);
                    did = ریښتیا;
                } وإلا إذا (m_params && consp) {
                    // Allow parameters to pass arrays
                    // Earlier recursion of ConsArray made sure each array value is constant
                    // This exception is fairly fragile, i.e. doesn't
                    // support arrays of arrays or other stuff
                    Ast_غوټه* ثابت newp = valuep->clone_ونه(خطأ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    did = ریښتیا;
                } وإلا إذا (m_params && VN_IS(valuep, ConsStruct)) {
                    Ast_غوټه* ثابت newp = valuep->clone_ونه(خطأ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    did = ریښتیا;
                } وإلا إذا (nodep->varp()->isParam() && VN_IS(valuep, Unbounded)) {
                    Ast_غوټه* ثابت newp = valuep->clone_ونه(خطأ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    did = ریښتیا;
                }
            }
        }
        إذا (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but variable isn't const: "
                           << nodep->varp()->prettyNameQ());
        }
    }
    فراغ لیدنه(Ast_بیان_Stmt* nodep) override {
        iterateChildren(nodep);
        إذا (!Ast_غوټه::afterCommentp(nodep->stmtsp())) {
            deleteVarScopesUnder(nodep->stmtsp());
            UINFO(8, "ExprStmt(...) " << nodep << " " << nodep->resultp());
            nodep->replaceWith(nodep->resultp()->unlink_Fr_شا());
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            // Removing the ExprStmt might have made something impure above now pure
        }
    }
    فراغ لیدنه(Ast_Enum_توکی_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        منطقي did = خطأ;
        إذا (nodep->itemp()->valuep()) {
            // UINFOTREE(1, nodep->itemp()->valuep(), "", "visitvaref");
            ثابت Ast_Enum_توکی* ثابت itemp = nodep->itemp();
            إذا (m_recursing_Enum_توکي.شمېر(itemp)) {
                nodep->v3error("Recursive enum value: " << nodep->itemp()->prettyNameQ());
            } وإلا {
                m_recursing_Enum_توکي.emplace(itemp);
                iterate_And_بل_Null(nodep->itemp()->valuep());
                m_recursing_Enum_توکي.erase(itemp);
            }
            إذا (AstConst* ثابت valuep = VN_CAST(nodep->itemp()->valuep(), Const)) {
                ثابت V3_شمېره& عدد = valuep->عدد();
                VL_DO_DANGLING(replace_عدد(nodep, عدد), nodep);
                did = ریښتیا;
            }
        }
        إذا (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but enum value isn't const: "
                           << nodep->itemp()->prettyNameQ());
        }
    }

    منطقي only_Sen_توکی_دننه_Sen_ونه(ثابت Ast_Sen_توکی* nodep) {
        // Only one if it's not in a list
        أرجع (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }
    فراغ لیدنه(Ast_Sen_توکی* nodep) override {
        iterateChildren(nodep);
        إذا (m_doNConst
            && !v3Global.opt.timing().is_سېټ_True()  // If --timing, V3Sched would turn this into an
                                                   // infinite loop. See #5080
            && (VN_IS(nodep->sensp(), Const) || VN_IS(nodep->sensp(), Enum_توکی_Ref)
                || (nodep->varrefp() && nodep->varrefp()->varp()->isParam()))) {
            // Constants in sensitivity lists may be removed (we'll simplify later)
            إذا (nodep->isClocked()) {  // A constant can never get a pos/negedge
                إذا (only_Sen_توکی_دننه_Sen_ونه(nodep)) {
                    إذا (nodep->څنډه_Type() == VEdgeType::ET_بدل_شوی) {
                        // TODO: This really is dodgy, as strictly compliant simulators will not
                        //       execute this block, but but t_func_check relies on it
                        nodep->replaceWith(
                            جديد Ast_Sen_توکی{nodep->fileline(), Ast_Sen_توکی::Initial{}});
                    } وإلا {
                        nodep->replaceWith(جديد Ast_Sen_توکی{nodep->fileline(), Ast_Sen_توکی::Never{}});
                    }
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                } وإلا {
                    VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                }
            } وإلا {  // Otherwise it may compute a result that needs to settle out
                nodep->replaceWith(جديد Ast_Sen_توکی{nodep->fileline(), Ast_Sen_توکی::Combo{}});
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            }
        } وإلا إذا (m_doNConst && VN_IS(nodep->sensp(), Not)) {
            // V3Gate may propagate NOTs into clocks... Just deal with it
            Ast_غوټه* ثابت sensp = nodep->sensp();
            Ast_غوټه* وروستی_Sensp = sensp;
            منطقي invert = خطأ;
            طالما (VN_IS(وروستی_Sensp, Not)) {
                وروستی_Sensp = VN_AS(وروستی_Sensp, Not)->lhsp();
                invert = !invert;
            }
            UINFO(8, "senItem(NOT...) " << nodep << " " << invert);
            إذا (invert) nodep->څنډه_Type(nodep->څنډه_Type().invert());
            sensp->replaceWith(وروستی_Sensp->unlink_Fr_شا());
            VL_DO_DANGLING(ټېل_Deletep(sensp), sensp);
        }
    }

    صنف Sen_توکی_Cmp final {
        ساكن صحيح cmp(ثابت Ast_غوټه_بیان* ap, ثابت Ast_غوټه_بیان* bp) {
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
                إذا (aConstp->عدد().isLtXZ(bConstp->عدد())) أرجع -1;
                إذا (bConstp->عدد().isLtXZ(aConstp->عدد())) أرجع 1;
                أرجع 0;
            }

            إذا (ثابت Ast_غوټه_Biop* ثابت aBiOpp = VN_CAST(ap, غوټه_Biop)) {
                ثابت Ast_غوټه_Biop* ثابت bBiOpp = VN_AS(bp, غوټه_Biop);
                // Compare RHSs first, as LHS might be const
                إذا (ثابت صحيح c = cmp(aBiOpp->rhsp(), bBiOpp->rhsp())) أرجع c;
                أرجع cmp(aBiOpp->lhsp(), bBiOpp->lhsp());
            }

            إذا (ثابت AstCMethodHard* ثابت aCallp = VN_CAST(ap, CMethodHard)) {
                ثابت AstCMethodHard* ثابت bCallp = VN_AS(bp, CMethodHard);
                إذا (aCallp->لاره() < bCallp->لاره()) أرجع -1;
                إذا (aCallp->لاره() > bCallp->لاره()) أرجع 1;
                إذا (ثابت صحيح c = cmp(aCallp->fromp(), bCallp->fromp())) أرجع c;
                ثابت Ast_غوټه_بیان* aPinsp = aCallp->pinsp();
                ثابت Ast_غوټه_بیان* bPinsp = bCallp->pinsp();
                طالما (aPinsp && bPinsp) {
                    إذا (ثابت صحيح c = cmp(aPinsp, bPinsp)) أرجع c;
                    aPinsp = VN_AS(aPinsp->nextp(), غوټه_بیان);
                    bPinsp = VN_AS(bPinsp->nextp(), غوټه_بیان);
                }
                أرجع aPinsp ? -1 : bPinsp ? 1 : 0;
            }

            أرجع 0;
        }

    عام:
        منطقي چلوونکی()(ثابت Ast_Sen_توکی* lhsp, ثابت Ast_Sen_توکی* rhsp) ثابت {
            ثابت Ast_غوټه_بیان* ثابت lSensp = lhsp->sensp();
            ثابت Ast_غوټه_بیان* ثابت rSensp = rhsp->sensp();
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
            أرجع lhsp->څنډه_Type() < rhsp->څنډه_Type();
        }
    };

    فراغ لیدنه(Ast_Sen_ونه* nodep) override {
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
            لكل (Ast_Sen_توکی *nextp, *senp = nodep->sensesp(); senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), Sen_توکی);
                // cppcheck-suppress unassignedVariable  // cppcheck bug
                ثابت Sen_توکی_Cmp cmp;
                إذا (nextp && !cmp(senp, nextp)) {
                    // Something's out of order, sort it
                    senp = تش_اشاره;
                    std::ویکټور<Ast_Sen_توکی*> vec;
                    لكل (Ast_Sen_توکی* senp = nodep->sensesp(); senp;
                         senp = VN_AS(senp->nextp(), Sen_توکی)) {
                        vec.ټېل_شا(senp);
                    }
                    stable_ترتیبول(vec.begin(), vec.پای_ټکی(), Sen_توکی_Cmp());
                    لكل (ثابت خپلکاری& ip : vec) ip->unlink_Fr_شا();
                    لكل (ثابت خپلکاری& ip : vec) nodep->جمع_کړه_Sensesp(ip);
                    اكسر;
                }
            }

            // Pass 2, remove duplicates and simplify adjacent terms if possible
            لكل (Ast_Sen_توکی *senp = nodep->sensesp(), *nextp; senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), Sen_توکی);
                إذا (!nextp) اكسر;
                Ast_Sen_توکی* ثابت lItemp = senp;
                Ast_Sen_توکی* ثابت rItemp = nextp;
                ثابت Ast_غوټه_بیان* ثابت lSenp = lItemp->sensp();
                ثابت Ast_غوټه_بیان* ثابت rSenp = rItemp->sensp();
                إذا (!lSenp || !rSenp) استمر;

                إذا (lSenp->same_Gate_ونه(rSenp)) {
                    // POSEDGE or NEGEDGE -> BOTHEDGE. (We've sorted POSEDGE, before NEGEDGE, so we
                    // do not need to test for the opposite orders.)
                    إذا (lItemp->څنډه_Type() == VEdgeType::ET_POSEDGE
                        && rItemp->څنډه_Type() == VEdgeType::ET_NEGEDGE) {
                        // Make both terms BOTHEDGE, the second will be removed below
                        lItemp->څنډه_Type(VEdgeType::ET_BOTHEDGE);
                        rItemp->څنډه_Type(VEdgeType::ET_BOTHEDGE);
                    }

                    // Remove identical expressions
                    إذا (lItemp->څنډه_Type() == rItemp->څنډه_Type()) {
                        VL_DO_DANGLING(ټېل_Deletep(rItemp->unlink_Fr_شا()), rItemp);
                        nextp = lItemp;
                    }
                }
            }
        }
    }

    //-----
    // Zero elimination
    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->timingControlp()) m_hasJumpDelay = ریښتیا;
        إذا (m_doNConst && replace_غوټه_ټاکل(nodep)) أرجع;
    }
    فراغ لیدنه(AstAlias* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    فراغ لیدنه(AstAliasScope* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    فراغ لیدنه(Ast_ټاکل_W* nodep) override {
        iterateChildren(nodep);
        إذا (m_doNConst && replace_غوټه_ټاکل(nodep)) أرجع;
        // Process containing this AssignW as single body statement
        AstAlways* ثابت procp = VN_CAST(nodep->backp(), Always);
        إذا (!procp || procp->stmtsp() != nodep || nodep->nextp()) أرجع;
        // Not VarXRef, as different refs may set different values to each hierarchy
        Ast_غوټه_Var_Ref* ثابت varrefp = VN_CAST(nodep->lhsp(), VarRef);
        إذا (m_wremove && !m_params && m_doNConst && m_modp && عملوند_Const(nodep->rhsp())
            && !VN_AS(nodep->rhsp(), Const)->عدد().isFourState()
            && varrefp  // Don't do messes with BITREFs/ARRAYREFs
            && !varrefp->varp()->hasStrengthAssignment()  // Strengths are resolved in V3Tristate
            && !varrefp->varp()->valuep()  // Not already constified
            && !varrefp->varScopep()  // Not scoped (or each scope may have different initial val.)
            && !varrefp->varp()->isForced()  // Not forced (not really a constant)
        ) {
            // ASSIGNW (VARREF, const) -> INITIAL ( ASSIGN (VARREF, const) )
            UINFO(4, "constAssignW " << nodep);
            // Make a initial assignment
            Ast_غوټه_بیان* ثابت exprp = nodep->rhsp()->unlink_Fr_شا();
            varrefp->unlink_Fr_شا();
            File_کرښه* ثابت flp = nodep->fileline();
            AstInitial* ثابت newinitp = جديد AstInitial{flp, جديد Ast_ټاکل{flp, varrefp, exprp}};
            procp->replaceWith(newinitp);
            VL_DO_DANGLING(ټېل_Deletep(procp), procp);
            // Set the initial value right in the variable so we can constant propagate
            Ast_غوټه* ثابت initvaluep = exprp->clone_ونه(خطأ);
            varrefp->varp()->isConst(ریښتیا);
            varrefp->varp()->valuep(initvaluep);
        }
    }
    فراغ لیدنه(AstCReset* nodep) override {
        iterateChildren(nodep);
        إذا (!m_doNConst) أرجع;
        ثابت AstBasicDType* ثابت bdtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType);
        إذا (!bdtypep) أرجع;
        إذا (!bdtypep->isZeroInit()) أرجع;
        AstConst* ثابت newp = جديد AstConst{nodep->fileline(), V3_شمېره{nodep, bdtypep}};
        UINFO(9, "CRESET(0) => CONST(0) " << nodep);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(Ast_Cvt_Unpack_منزل_Unpack* nodep) override {
        iterateChildren(nodep);
        AstConsArray* ثابت fromp = VN_CAST(nodep->fromp(), ConsArray);
        إذا (!fromp) أرجع;
        // Fold the outermost dimension, converting the elements for any dimension under it
        ثابت AstUnpackArrayDType* ثابت منزل_Arrayp
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت AstUnpackArrayDType* ثابت fromArrayp
            = VN_AS(fromp->dtypep()->skipRefp(), UnpackArrayDType);
        // This dimension may not be reversed, only one under it
        ثابت منطقي سرچپه
            = منزل_Arrayp->declRange().ascending() != fromArrayp->declRange().ascending();
        ثابت uint64_t وروستی_شاخص = منزل_Arrayp->elementsConst() - 1;
        Ast_غوټه_DType* ثابت subDTypep = منزل_Arrayp->subDTypep();
        ثابت AstUnpackArrayDType* ثابت subArrayp
            = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
        ثابت خپلکاری convert = [&](Ast_غوټه_بیان* valuep) -> Ast_غوټه_بیان* {
            valuep->unlink_Fr_شا();
            إذا (!subArrayp || !subArrayp->is_سرچپه_د(valuep->dtypep())) أرجع valuep;
            أرجع جديد Ast_Cvt_Unpack_منزل_Unpack{valuep->fileline(), valuep, subDTypep};
        };
        AstConsArray* ثابت newp
            = جديد AstConsArray{fromp->fileline(), nodep->dtypep(),
                               fromp->defaultp() ? convert(fromp->defaultp()) : تش_اشاره};
        لكل (ثابت Ast_Cons_Array_توکی* itemp = fromp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_توکی)) {
            newp->valuep(سرچپه ? وروستی_شاخص - itemp->شاخص() : itemp->شاخص(),
                         convert(itemp->valuep()));
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        iterate(newp);  // Fold the conversions of the elements
    }
    فراغ لیدنه(Ast_Cvt_Array_منزل_Array* nodep) override {
        iterateChildren(nodep);
        // Handle the case where we have a stream operation inside a cast conversion
        // Check for both StreamL and StreamR operations
        Ast_غوټه_بهیر* streamp = تش_اشاره;
        منطقي is_سرچپه = خطأ;
        إذا (Ast_بهیر_L* ثابت بهیر_Lp = VN_CAST(nodep->fromp(), بهیر_L)) {
            streamp = بهیر_Lp;
            is_سرچپه = ریښتیا;  // StreamL reverses the operation
        } وإلا إذا (Ast_بهیر_R* ثابت بهیر_Rp = VN_CAST(nodep->fromp(), بهیر_R)) {
            streamp = بهیر_Rp;
            is_سرچپه = خطأ;  // StreamR doesn't reverse the operation
        }
        إذا (streamp) {
            Ast_غوټه_بیان* srcp = streamp->lhsp();
            ثابت Ast_غوټه_DType* ثابت srcDTypep = srcp->dtypep()->skipRefp();
            Ast_غوټه_DType* ثابت dstDTypep = nodep->dtypep()->skipRefp();
            إذا (VN_IS(srcDTypep, کتار_DType) && VN_IS(dstDTypep, کتار_DType)) {
                صحيح بلاک_اندازه = 1;
                إذا (ثابت AstConst* ثابت constp = VN_CAST(streamp->rhsp(), Const)) {
                    بلاک_اندازه = constp->منزل_SInt();
                    إذا (VL_UNLIKELY(بلاک_اندازه <= 0)) {
                        // Not reachable due to higher level checks when parsing stream
                        // operators commented out to not fail v3error-coverage-checks.
                        // nodep->v3error("Stream block size must be positive, got " <<
                        // blockSize);
                        بلاک_اندازه = 1;
                    }
                }
                // Not reachable due to higher level checks when parsing stream operators
                // commented out to not fail v3error-coverage-checks.
                // else {
                //    nodep->v3error("Stream block size must be constant (got " <<
                //    streamp->rhsp()->prettyTypeName() << ")");
                // }
                صحيح src_Element_بټونه = 0;
                إذا (ثابت Ast_غوټه_DType* ثابت elemDtp = srcDTypep->subDTypep()) {
                    src_Element_بټونه = elemDtp->پلنوالی();
                }
                صحيح dst_Element_بټونه = 0;
                إذا (ثابت Ast_غوټه_DType* ثابت elemDtp = dstDTypep->subDTypep()) {
                    dst_Element_بټونه = elemDtp->پلنوالی();
                }
                streamp->unlink_Fr_شا();
                Ast_غوټه_بیان* newp = جديد Ast_Cvt_Array_منزل_Array{
                    srcp->fileline(), srcp->unlink_Fr_شا(), dstDTypep,     is_سرچپه,
                    بلاک_اندازه,        dst_Element_بټونه,       src_Element_بټونه};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(streamp), streamp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع;
            }
        }
    }
    فراغ لیدنه(AstRelease* nodep) override {
        // cppcheck-suppress constVariablePointer // children unlinked below
        إذا (AstConcat* ثابت concatp = VN_CAST(nodep->lhsp(), Concat)) {
            File_کرښه* ثابت flp = nodep->fileline();
            AstRelease* ثابت newLp = جديد AstRelease{flp, concatp->lhsp()->unlink_Fr_شا()};
            AstRelease* ثابت newRp = جديد AstRelease{flp, concatp->rhsp()->unlink_Fr_شا()};
            nodep->replaceWith(newLp);
            newLp->جمع_کړه_بل_دلته(newRp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            لیدنه(newLp);
            لیدنه(newRp);
        }
    }

    فراغ لیدنه(Ast_غوټه_If* nodep) override {
        iterateChildren(nodep);
        إذا (m_doNConst) {
            إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->condp(), Const)) {
                Ast_غوټه* keepp = تش_اشاره;
                Ast_غوټه* delp = تش_اشاره;
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
                    keepp->unlink_Fr_شا_With_بل();
                    nodep->replaceWith(keepp);
                } وإلا {
                    nodep->unlink_Fr_شا();
                    deleteVarScopesUnder(nodep->thensp());
                    deleteVarScopesUnder(nodep->elsesp());
                }
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);

                // Removed branch could contain only impurity within a function and the
                // function could be purified
                VIsCached::clear_Cache_ونه();
            } وإلا إذا (!Ast_غوټه::afterCommentp(nodep->thensp())
                       && !Ast_غوټه::afterCommentp(nodep->elsesp())) {
                إذا (!nodep->condp()->is_خالص()) {
                    // Condition has side effect - leave, but don't need return value
                    UINFO(4, "IF({x}) ; => STMTEXPR({x}): " << nodep);
                    nodep->fileline()->warnOff(V3_Error_کوډ::IGNOREDRETURN, ریښتیا);
                    Ast_غوټه_Stmt* ثابت newp
                        = جديد Ast_Stmt_بیان{nodep->fileline(), nodep->condp()->unlink_Fr_شا()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                } وإلا {
                    // Empty block, remove it
                    UINFO(4, "IF({x}) ; => ; : " << nodep);
                    VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                }
            } وإلا إذا (!Ast_غوټه::afterCommentp(nodep->thensp())) {
                UINFO(4, "IF({x}) nullptr {...} => IF(NOT{x}}: " << nodep);
                Ast_غوټه_بیان* ثابت condp = nodep->condp();
                Ast_غوټه* ثابت elsesp = nodep->elsesp();
                condp->unlink_Fr_شا_With_بل();
                elsesp->unlink_Fr_شا_With_بل();
                إذا (nodep->thensp()) {  // Must have been comment
                    ټېل_Deletep(nodep->thensp()->unlink_Fr_شا_With_بل());
                }
                nodep->condp(جديد AstLogNot{condp->fileline(),
                                           condp});  // LogNot, as C++ optimization also possible
                nodep->جمع_کړه_Thensp(elsesp);
                nodep->branchPred(nodep->branchPred().invert());
            } وإلا إذا (((VN_IS(nodep->condp(), Not) && nodep->condp()->پلنوالی() == 1)
                        || VN_IS(nodep->condp(), LogNot))
                       && nodep->thensp() && nodep->elsesp()) {
                UINFO(4, "IF(NOT {x})  => IF(x) swapped if/else" << nodep);
                Ast_غوټه_بیان* ثابت condp
                    = VN_AS(nodep->condp(), غوټه_Uniop)->lhsp()->unlink_Fr_شا_With_بل();
                Ast_غوټه* ثابت thensp = nodep->thensp()->unlink_Fr_شا_With_بل();
                Ast_غوټه* ثابت elsesp = nodep->elsesp()->unlink_Fr_شا_With_بل();
                AstIf* ثابت ifp = جديد AstIf{nodep->fileline(), condp, elsesp, thensp};
                ifp->isBoundsCheck(nodep->isBoundsCheck());  // Copy bounds check info
                ifp->branchPred(nodep->branchPred().invert());
                nodep->replaceWith(ifp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            } وإلا إذا (if_Same_ټاکل(nodep)) {
                UINFO(4,
                      "IF({a}) ASSIGN({b},{c}) else ASSIGN({b},{d}) => ASSIGN({b}, {a}?{c}:{d})");
                Ast_غوټه_ټاکل* ثابت thensp = VN_AS(nodep->thensp(), غوټه_ټاکل);
                // cppcheck-suppress constVariablePointer // children unlinked below
                Ast_غوټه_ټاکل* ثابت elsesp = VN_AS(nodep->elsesp(), غوټه_ټاکل);
                thensp->unlink_Fr_شا();
                Ast_غوټه_بیان* ثابت condp = nodep->condp()->unlink_Fr_شا();
                Ast_غوټه_بیان* ثابت truep = thensp->rhsp()->unlink_Fr_شا();
                Ast_غوټه_بیان* ثابت falsep = elsesp->rhsp()->unlink_Fr_شا();
                thensp->rhsp(جديد AstCond{truep->fileline(), condp, truep, falsep});
                nodep->replaceWith(thensp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            } وإلا إذا (خطأ  // Disabled, as vpm assertions are faster
                              // without due to short-circuiting
                       && عملوند_If_If(nodep)) {
                UINFO(9, "IF({a}) IF({b}) => IF({a} && {b})");
                Ast_غوټه_If* ثابت ښکتنی_Ifp = VN_AS(nodep->thensp(), غوټه_If);
                Ast_غوټه_بیان* ثابت condp = nodep->condp()->unlink_Fr_شا();
                Ast_غوټه* ثابت ښکتنی_Thensp = ښکتنی_Ifp->thensp()->unlink_Fr_شا_With_بل();
                Ast_غوټه_بیان* ثابت ښکتنی_Condp = ښکتنی_Ifp->condp()->unlink_Fr_شا_With_بل();
                nodep->condp(جديد AstLogAnd{ښکتنی_Ifp->fileline(), condp, ښکتنی_Condp});
                ښکتنی_Ifp->replaceWith(ښکتنی_Thensp);
                VL_DO_DANGLING(ټېل_Deletep(ښکتنی_Ifp), ښکتنی_Ifp);
            } وإلا {
                // Optimizations that don't reform the IF itself
                إذا (عملوند_Bool_ښويدنه(nodep->condp())) replace_Bool_ښويدنه(nodep->condp());
                سمون_If_Same_وروستی(nodep);
                سمون_If_Cond_Cond(nodep);
            }
        }
    }

    فراغ لیدنه(AstDisplay* nodep) override {
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
            || (prevp->filep() && nodep->filep() && !prevp->filep()->same_ونه(nodep->filep())))
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
        إذا (pformatp->exprsp() && !pformatp->exprsp()->is_خالص_And_بل()) أرجع خطأ;
        إذا (nformatp->exprsp() && !nformatp->exprsp()->is_خالص_And_بل()) أرجع خطأ;
        // Avoid huge merges
        ساكن constexpr صحيح DISPLAY_MAX_یوځای_اوږدتیا = 500;
        إذا (pformatp->متن().اوږدتیا() + nformatp->متن().اوږدتیا() > DISPLAY_MAX_یوځای_اوږدتیا)
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
        إذا (prevp->جمع_کړه_Newline()) pformatp->متن(pformatp->متن() + "\n");
        pformatp->متن(pformatp->متن() + nformatp->متن());
        إذا (!prevp->جمع_کړه_Newline() && nodep->جمع_کړه_Newline()) pformatp->متن(pformatp->متن() + "\n");
        إذا (nformatp->exprsp()) pformatp->جمع_کړه_Exprsp(nformatp->exprsp()->unlink_Fr_شا_With_بل());
        إذا (AstScopeName* ثابت scopeNamep = nformatp->scopeNamep()) {
            scopeNamep->unlink_Fr_شا_With_بل();
            pformatp->scopeNamep(scopeNamep);
        }
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
        أرجع ریښتیا;
    }
    فراغ لیدنه(AstSFormatArg* nodep) override {
        // Skip namep(): requiring its runtime lookup to be constant can reject valid
        // enum-valued constant-function calls in parameters. displayedEnum() resolves
        // the name from the folded exprp() value and enum dtype instead.
        iterate_And_بل_Null(nodep->exprp());
    }
    فراغ لیدنه(AstSFormatF* nodep) override {
        // Substitute constants into displays.  The main point of this is to
        // simplify assertion methodologies which call functions with display's.
        // This eliminates a pile of wide temps, and makes the C a whole lot more readable.
        iterateChildren(nodep);
        إذا (nodep->بیان_Format_2()) {
            // Upconvert to text-based format?
            // Similar code in V3LinkResolve::visit(AstSFormatF)
            UASSERT_OBJ(nodep->متن() == "", nodep,
                        "Non-format $sformatf should have text format == \"\"");
            إذا (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->عدد().isFromString()) {
                AstConst* ثابت fmtp = VN_AS(nodep->exprsp()->unlink_Fr_شا(), Const);
                nodep->متن(fmtp->عدد().منزل_String());
                nodep->بیان_Format_2(خطأ);
                VL_DO_DANGLING(ټېل_Deletep(fmtp), fmtp);
            }
        }
        إذا (nodep->بیان_Format_2()) أرجع;  // Runtime, unfortunately  (unless constify format later)
        منطقي anyconst = خطأ;
        لكل (Ast_غوټه* argp = nodep->exprsp(); argp; argp = argp->nextp()) {
            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
            Ast_غوټه* ثابت subargp = fargp ? fargp->exprp() : argp;
            إذا (VN_IS(subargp, Const)) {
                anyconst = ریښتیا;
                اكسر;
            }
        }
        إذا (m_doNConst && anyconst) {
            UINFO(9, "  Display in  " << nodep->متن());
            string newFormat;
            string fmt;
            منطقي دننه_Pct = خطأ;
            Ast_غوټه* argp = nodep->exprsp();
            ثابت string متن = nodep->متن();
            لكل (حرف ch : متن) {
                إذا (!دننه_Pct && ch == '%') {
                    دننه_Pct = ریښتیا;
                    fmt = ch;
                } وإلا إذا (دننه_Pct && (std::که_عدد(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } وإلا إذا (دننه_Pct) {
                    دننه_Pct = خطأ;
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
                            Ast_غوټه* ثابت nextp = argp->nextp();
                            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                            Ast_غوټه* ثابت subargp = fargp ? fargp->exprp() : argp;
                            ثابت VFormatAttr formatAttr
                                = argp
                                      ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                                      : VFormatAttr{};
                            إذا (VN_IS(subargp, Const)) {  // Convert it
                                ثابت string بهر
                                    = formatAttr.isEnum()
                                          ? const_عدد_V(subargp).displayedEnum(fargp, fmt)
                                          : const_عدد_V(subargp).displayed(nodep, fmt, formatAttr);
                                UINFO(9, "     DispConst: " << fmt << " -> " << بهر << "  for "
                                                            << subargp);
                                // fmt = out w/ replace % with %% as it must later when
                                // used as a format output as literal.
                                fmt = VString::quotePercent(بهر);
                                VL_DO_DANGLING(ټېل_Deletep(argp->unlink_Fr_شا()), argp);
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
        إذا (!nodep->exprsp() && nodep->متن().پیدا_کړه('%') == string::npos && !nodep->hidden()) {
            // Just a simple constant string - the formatting is pointless
            UINFO(9, "replaceConstStr");
            VL_DO_DANGLING(replaceConstString(nodep, nodep->متن()), nodep);
            أرجع;
        }
        إذا (VN_IS(nodep->backp(), غوټه_ټاکل)  // Can't remove under Display etc
            && nodep->متن() == "%s" && VN_IS(nodep->exprsp(), SFormatArg)
            && VN_AS(nodep->exprsp(), SFormatArg)->formatAttr().isString()
            && !nodep->exprsp()->nextp()) {
            UINFO(9, "  Display(%p, expr) -> constant expr " << nodep);
            nodep->replaceWith(VN_AS(nodep->exprsp(), SFormatArg)->exprp()->unlink_Fr_شا());
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }
    }
    فراغ لیدنه(AstSFormat* nodep) override {
        iterateChildren(nodep);
        إذا (!m_doNConst) أرجع;
        إذا (m_doNConst) {
            // If it's a constant string written to a string variable, inline it as an assignment
            AstSFormatF* ثابت fmtp = nodep->fmtp();
            Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp();
            AstBasicDType* ثابت basicp = VN_CAST(lhsp->dtypep()->skipRefp(), BasicDType);
            إذا (basicp && basicp->isString() && !fmtp->exprsp()
                && fmtp->متن().پیدا_کړه('%') == string::npos) {
                AstConst* ثابت strp
                    = جديد AstConst{fmtp->fileline(), AstConst::String{}, fmtp->متن()};
                lhsp->unlink_Fr_شا();
                nodep->replaceWith(جديد Ast_ټاکل{nodep->fileline(), lhsp, strp});
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع;
            }
        }
    }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        VL_RESTORER(m_underRecFunc);
        إذا (nodep->recursive()) m_underRecFunc = ریښتیا;
        iterateChildren(nodep);
    }

    فراغ لیدنه(Ast_غوټه_CCall* nodep) override {
        iterateChildren(nodep);
        m_hasJumpDelay = ریښتیا;  // As don't analyze inside tasks for timing controls
    }
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        // Note excludes AstFuncRef as other visitor below
        iterateChildren(nodep);
        m_hasJumpDelay = ریښتیا;  // As don't analyze inside tasks for timing controls
    }
    فراغ لیدنه(AstFuncRef* nodep) override {
        لیدنه(static_cast<Ast_غوټه_FTask_Ref*>(nodep));
        إذا (m_params) {  // Only parameters force us to do constant function call propagation
            replaceWithSimulation(nodep);
        }
    }
    فراغ لیدنه(AstArg* nodep) override {
        // replaceWithSimulation on the Arg's parent FuncRef replaces these
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstLoop* nodep) override {
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
                VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                أرجع;
            }
        }
        // If last statement always breaks, repalce loop with body
        إذا (Ast_غوټه* lastp = nodep->stmtsp()) {
            طالما (Ast_غوټه* ثابت nextp = lastp->nextp()) lastp = nextp;
            إذا (AstLoopTest* ثابت testp = VN_CAST(lastp, LoopTest)) {
                إذا (testp->condp()->isZero()) {
                    VL_DO_DANGLING2(ټېل_Deletep(testp->unlink_Fr_شا()), testp, lastp);
                    nodep->replaceWith(nodep->stmtsp()->unlink_Fr_شا_With_بل());
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                }
            }
        }
        // Warn on infinite loop
        إذا (!m_hasLoopTest && !thisLoopHasJumpDelay) {
            nodep->v3warn(INFINITELOOP, "Infinite loop (condition always true)");
            nodep->fileline()->modifyWarnOff(V3_Error_کوډ::INFINITELOOP, ریښتیا);  // Complain once
        }
    }
    فراغ لیدنه(AstLoopTest* nodep) override {
        iterateChildren(nodep);

        // If never breaks, remove
        إذا (nodep->condp()->isNeqZero()) {
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }

        m_hasLoopTest = ریښتیا;

        // If always breaks, subsequent statements are dead code, delete them
        إذا (nodep->condp()->isZero()) {
            Ast_غوټه* ثابت nextp = nodep->nextp();
            إذا (nextp && canDeleteStatements()) {
                VL_DO_DANGLING(ټېل_Deletep(nextp->unlink_Fr_شا_With_بل()), nodep);
            }
            أرجع;
        }

        إذا (عملوند_Bool_ښويدنه(nodep->condp())) replace_Bool_ښويدنه(nodep->condp());
    }

    فراغ لیدنه(AstConsArray* nodep) override { iterateChildren(nodep); }
    فراغ لیدنه(AstConsStruct* nodep) override { iterateChildren(nodep); }
    فراغ لیدنه(Ast_Cons_Struct_توکی* nodep) override { iterateChildren(nodep); }
    فراغ لیدنه(Ast_Cons_Array_توکی* nodep) override { iterateChildren(nodep); }
    فراغ لیدنه(AstUnbounded* nodep) override { iterateChildren(nodep); }
    // These are converted by V3Param.  Don't constify as we don't want the
    // from() VARREF to disappear, if any.
    // If output of a presel didn't get consted, chances are V3Param didn't visit properly
    فراغ لیدنه(Ast_غوټه_Pre_Sel*) override {}

    // Ignored, can eliminate early
    فراغ لیدنه(AstSysIgnore* nodep) override {
        iterateChildren(nodep);
        إذا (m_doNConst) VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }

    فراغ لیدنه(Ast_Stmt_بیان* nodep) override {
        iterateChildren(nodep);
        // Malformed due to child being deleted, remove here
        إذا (!nodep->exprp()) {
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }
        // Remove if expression is trivially pure
        إذا (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), VarRef)) {
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }
        // Remove calls to empty functions
        إذا (ثابت AstCCall* ثابت callp = VN_CAST(nodep->exprp(), CCall)) {
            ثابت AstCFunc* ثابت funcp = callp->funcp();
            إذا (!callp->argsp() && funcp->تشه_Body()) {
                VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
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
    فراغ لیدنه(AstBasicDType* nodep) override {
        iterateChildren(nodep);
        nodep->cvtRangeConst();
    }

    //-----
    // Jump elimination

    فراغ لیدنه(AstJumpGo* nodep) override {
        // Any statements following the JumpGo (at this statement level) never execute, delete
        إذا (nodep->nextp() && canDeleteStatements()) {
            ټېل_Deletep(nodep->nextp()->unlink_Fr_شا_With_بل());
        }

        // JumpGo as last statement in target JumpBlock (including last in a last sub-list),
        // is a no-op, remove it.
        إذا (!nodep->nextp()) {
            لكل (Ast_غوټه* abovep = nodep->abovep(); abovep; abovep = abovep->abovep()) {
                إذا (abovep == nodep->blockp()) {
                    VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                    أرجع;
                }
                // Stop if not doing expensive, or if the above node is not the last in its list,
                // ... or if it's not an 'if' TODO: it would be enough if it was not a branch.
                إذا (!m_doExpensive || abovep->nextp() || !VN_IS(abovep, If)) اكسر;
            }
        }
        // Mark JumpBlock as used
        m_کارول_شوی_Jump_بلاکونه.emplace(nodep->blockp());
        m_hasJumpDelay = ریښتیا;
    }

    فراغ لیدنه(Ast_Jump_بلاک* nodep) override {
        iterateChildren(nodep);

        // If first statement is an AstLoopTest, pull it before the jump block
        إذا (AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            nodep->جمع_کړه_دلته_This_As_بل(testp->unlink_Fr_شا());
        }

        // Remove if empty
        إذا (!nodep->stmtsp()) {
            UINFO(4, "JUMPLABEL => empty " << nodep);
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }

        // If no JumpGo points to this node, replace it with its body
        إذا (!m_کارول_شوی_Jump_بلاکونه.شمېر(nodep)) {
            UINFO(4, "JUMPLABEL => unused " << nodep);
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_شا_With_بل());
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
    }

    //-----
    // Below lines are magic expressions processed by astgen
    //  TREE_SKIP_VISIT("AstNODETYPE")    # Rename normal visit to visitGen and don't iterate
    //-----
    // clang-format off
    ونه_SKIP_لیدنه("ArraySel");
    ونه_SKIP_لیدنه("CAwait");

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
    //مزي_ګانې
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
    فراغ لیدنه(Ast_غوټه* nodep) override {
        // Default: Just iterate
        إذا (m_required) {
            إذا (VN_IS(nodep, غوټه_DType) || VN_IS(nodep, Range) || VN_IS(nodep, ټوټه_Sel)
                || VN_IS(nodep, Dot) || VN_IS(nodep, متن_2)) {
                // ignore
            } وإلا إذا (VN_IS(nodep, نمونه) || VN_IS(nodep, PatMember)) {
                // A parameter override pattern is typed later, so only fold its members
                iterateChildren(nodep);
            } وإلا إذا (AstCellRef* ثابت crp = VN_CAST(nodep, CellRef)) {
                iterate(crp->exprp());
                إذا (Ast_غوټه* ثابت newp = crp->exprp()) {
                    crp->replaceWithKeepDType(newp->unlink_Fr_شا());
                    VL_DO_DANGLING(ټېل_Deletep(crp), crp);
                }
                أرجع;
            } وإلا {
                nodep->v3error("Expecting expression to be constant, but can't convert a "
                               << nodep->prettyTypeName() << " to constant.");
            }
        } وإلا {
            إذا (nodep->isTimingControl()) m_hasJumpDelay = ریښتیا;
            // Calculate the width of this operation
            إذا (m_params && !nodep->پلنوالی()) nodep = V3_پلنوالی::پلنوالی_Params_Edit(nodep);
            iterateChildren(nodep);
        }
    }

عام:
    // Processing Mode Enum
    تعداد Proc_ډېرپېښ : uint8_t {
        PROC_PARAMS_NOWARN,
        PROC_PARAMS,
        PROC_GENERATE,
        PROC_ژوندی,
        PROC_V_WARN,
        PROC_V_NOWARN,
        PROC_V_EXPENSIVE,
        PROC_CPP
    };

    // CONSTRUCTORS
    ConstVisitor(Proc_ډېرپېښ pmode, منطقي globalPass)
        : m_globalPass{globalPass}
        , m_concswap_نومونه{globalPass ? ("__Vconcswap_" + cvt_منزل_Str(s_global_Pass_عدد++)) : ""} {
        // clang-format off
        بدل (pmode) {
        حالة PROC_PARAMS_NOWARN:  m_doV = ریښتیا;  m_doNConst = ریښتیا; m_params = ریښتیا;
                                  m_required = خطأ; اكسر;
        حالة PROC_PARAMS:         m_doV = ریښتیا;  m_doNConst = ریښتیا; m_params = ریښتیا;
                                  m_required = ریښتیا; اكسر;
        حالة PROC_GENERATE:       m_doV = ریښتیا;  m_doNConst = ریښتیا; m_params = ریښتیا;
                                  m_required = ریښتیا; m_doGenerate = ریښتیا; اكسر;
        حالة PROC_ژوندی:           اكسر;
        حالة PROC_V_WARN:         m_doV = ریښتیا;  m_doNConst = ریښتیا; m_warn = ریښتیا; m_convert_Logic_منزل_بټ = ریښتیا; اكسر;
        حالة PROC_V_NOWARN:       m_doV = ریښتیا;  m_doNConst = ریښتیا; اكسر;
        حالة PROC_V_EXPENSIVE:    m_doV = ریښتیا;  m_doNConst = ریښتیا; m_doExpensive = ریښتیا; اكسر;
        حالة PROC_CPP:            m_doV = خطأ; m_doNConst = ریښتیا; m_doCpp = ریښتیا; اكسر;
        افتراضي:                  v3fatalSrc("Bad case"); اكسر;
        }
        // clang-format on
    }
    ~ConstVisitor() override {
        إذا (m_doCpp) {
            إذا (m_globalPass) {
                V3Stats::جمع_کړه_Stat("Optimizations, Const bit op reduction", m_stat_بټ_Op_Reduction);
            } وإلا {
                V3Stats::جمع_کړه_Stat_مجموعه("Optimizations, Const bit op reduction", m_stat_بټ_Op_Reduction);
            }
        }
        V3Stats::جمع_کړه_Stat_مجموعه("Optimizations, Cond redundant expressions", m_stat_Cond_بیان_Redundant);
        V3Stats::جمع_کړه_Stat_مجموعه("Optimizations, If cond redundant expressions",
                            m_stat_If_Cond_بیان_Redundant);
        V3Stats::جمع_کړه_Stat_مجموعه("Optimizations, Concat merges", m_stat_Concat_یوځای);
        V3Stats::جمع_کړه_Stat_مجموعه("Optimizations, Member access predicate node visits",
                            m_statMemberAccessVisits);
    }

    Ast_غوټه* mainAcceptEdit(Ast_غوټه* nodep) {
        VIsCached::clear_Cache_ونه();  // Avoid using any stale isPure
        // Operate starting at a random place
        أرجع iterateSubtreeReturnEdits(nodep);
    }
};

uint32_t ConstVisitor::s_global_Pass_عدد = 0;

//######################################################################
// Const class functions

//! Force this cell node's parameter list to become a constant
فراغ V3Const::constifyParamsEdit(Ast_غوټه* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3_پلنوالی::پلنوالی_Params_Edit(nodep);
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
فراغ V3Const::constifyParamsNoWarnEdit(Ast_غوټه* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3_پلنوالی::پلنوالی_Params_Edit(nodep);
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
فراغ V3Const::constifyGenerateParamsEdit(Ast_غوټه* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE:");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized
    // yet.

    // Make sure we've sized everything first
    nodep = V3_پلنوالی::پلنوالی_Generate_Params_Edit(nodep);
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
        ConstVisitor visitor{ConstVisitor::PROC_V_WARN, /* globalPass: */ ریښتیا};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("const", 0, dump_ونه_Either_Level() >= 3);
}

فراغ V3Const::constifyCpp(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ ریښتیا};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("const_cpp", 0, dump_ونه_Either_Level() >= 3);
}

Ast_غوټه* V3Const::constifyEdit(Ast_غوټه* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_NOWARN, /* globalPass: */ خطأ};
    nodep = visitor.mainAcceptEdit(nodep);
    أرجع nodep;
}

Ast_غوټه* V3Const::constifyEditCpp(Ast_غوټه* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ خطأ};
    nodep = visitor.mainAcceptEdit(nodep);
    أرجع nodep;
}

فراغ V3Const::constify_All_ژوندی(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    // This only pushes constants up, doesn't make any other edits
    // IE doesn't prune dead statements, as we need to do some usability checks after this
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_ژوندی, /* globalPass: */ ریښتیا};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("const", 0, dump_ونه_Either_Level() >= 3);
}

فراغ V3Const::constifyAll(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ ریښتیا};
        (فراغ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("const", 0, dump_ونه_Either_Level() >= 3);
}

Ast_غوټه* V3Const::constifyExpensiveEdit(Ast_غوټه* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ خطأ};
    nodep = visitor.mainAcceptEdit(nodep);
    أرجع nodep;
}
