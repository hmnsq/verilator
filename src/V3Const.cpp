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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Simulate.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Width.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <type_traits>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਰੁੱਖ_SKIP_ਦੇਖੋ(...)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ TREEOP1(...)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ TREEOPA(...)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ TREEOP(...)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ TREEOPS(...)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ TREEOPC(...)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ TREEOPV(...)

//######################################################################
// Utilities

ਸਥਿਰ ਤਾਰਕਿਕ isConst(ਅਚਲ Ast_ਗੰਢ* nodep, uint64_t v) {
    ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep, Const);
    ਵਾਪਸ constp && constp->ਮੰਜ਼ਿਲ_UQuad() == v;
}

ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T>
ਸਥਿਰ ਕਿਸਮ_ਨਾਂ std::enable_if<std::is_integral<T>::ਮੁੱਲ, ਤਾਰਕਿਕ>::type isPow2(T val) {
    ਵਾਪਸ (val & (val - 1)) == 0;
}

ਸਥਿਰ ਪੂਰਨ ਗਿਣਤੀ_Trailing_Zeroes(uint64_t val) {
    UASSERT(val, "countTrailingZeroes argument must be non-zero");
#ਜੇ defined(__GNUC__) && !defined(VL_NO_BUILTINS)
    ਵਾਪਸ __builtin_ctzll(val);
#ਨਹੀਂਤਾਂ
    ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0;
    val = ~val;
    ਜਦੋਂ (val & 1) {
        ++ਦੋਆਧਾਰੀ_ਅੰਕ_2;
        val >>= 1;
    }
    ਵਾਪਸ ਦੋਆਧਾਰੀ_ਅੰਕ_2;
#ਜੇ_ਅੰਤ
}

// Allocates a unique id for each AstNode it is applied to, held in user4.
ਵਰਗ VNIdAllocator final {
    // NODE STATE
    // AstNode::user4p -> size_t. Unique id of the node (0/nullptr if not allocated yet)

    // MEMBERS
    // Claimed lazily on first use TODO: fix conflict with V3Param user4 slot
    std::unique_ptr<VNUser4_ਵਿੱਚ_Use> m_inuser4p;
    ਮਾਪ_t m_ਅਗਲਾ_Id = 0;  // Id allocated most recently

ਜਨਤਕ:
    // Return the unique id of the given node, allocating a new one if it has none yet
    ਮਾਪ_t ਸੰਚਾਲਕ()(Ast_ਗੰਢ* nodep) {
        ਜੇ (!m_inuser4p) m_inuser4p = std::make_unique<VNUser4_ਵਿੱਚ_Use>();  // Claim on first use
        ਜੇ (!nodep->user4p()) nodep->user4p(reinterpret_cast<ਖਾਲੀ*>(++m_ਅਗਲਾ_Id));
        ਵਾਪਸ reinterpret_cast<ਮਾਪ_t>(nodep->user4p());
    }
};

// This visitor can be used in the post-expanded Ast from V3Expand, where the Ast satisfies:
// - Constants are 64 bit at most (because words are accessed via AstWordSel)
// - Variables are scoped.
ਵਰਗ Const_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ_Visitor final : ਜਨਤਕ VNVisitorConst {
    // TYPES

    // Holds a node to be added as a term in the reduction tree, it's equivalent op count, and a
    // bool indicating if the term is clean (0/1 value, or if the top bits might be dirty)
    ਵਰਤੋਂ ਨਤੀਜਾ_ਪਦ = std::tuple<Ast_ਗੰਢ_ਵਿਅੰਜਕ*, ਬਿਨਾਨਿਸ਼ਾਨੀ, ਤਾਰਕਿਕ>;

    ਵਰਗ ਪੱਤੀ_Info final {  // Leaf node (either AstConst or AstVarRef)
        // MEMBERS
        ਤਾਰਕਿਕ m_polarity = ਸੱਚ;  // Invert result due to NOT
        ਪੂਰਨ m_lsb = 0;  // LSB of actually used bit of m_refp->varp()
        ਪੂਰਨ m_msb = 0;  // MSB of actually used bit of m_refp->varp()
        ਪੂਰਨ m_ਸ਼ਬਦ_Idx = -1;  // -1 means AstWordSel is not used.
        AstVarRef* m_refp = ਖਾਲੀ_ਸੂਚਕ;  // Leaf's variable reference
        ਅਚਲ AstConst* m_constp = ਖਾਲੀ_ਸੂਚਕ;  // Leaf's constant

    ਜਨਤਕ:
        // CONSTRUCTORS
        ਪੱਤੀ_Info() = ਮੂਲ;
        ਪੱਤੀ_Info(ਅਚਲ ਪੱਤੀ_Info& other) = ਮੂਲ;
        ਪੱਤੀ_Info& ਸੰਚਾਲਕ=(ਅਚਲ ਪੱਤੀ_Info& other) = ਮੂਲ;
        ਪੱਤੀ_Info(ਪੱਤੀ_Info&& other) = ਮੂਲ;
        ਪੱਤੀ_Info& ਸੰਚਾਲਕ=(ਪੱਤੀ_Info&& other) = ਮੂਲ;
        ਸਪਸ਼ਟ ਪੱਤੀ_Info(ਪੂਰਨ lsb)
            : m_lsb{lsb} {}

        // METHODS
        ਖਾਲੀ ਸੈੱਟ_ਪੱਤੀ(AstVarRef* refp) {
            UASSERT_OBJ(!m_refp && !m_constp, refp, "Must be called just once");
            m_refp = refp;
            m_msb = refp->varp()->ਚੌੜਾਈ_Min() - 1;
        }
        ਖਾਲੀ ਸੈੱਟ_ਪੱਤੀ(ਅਚਲ AstConst* constp) {
            UASSERT_OBJ(!m_refp && !m_constp, constp, "Must be called just once");
            m_constp = constp;
            m_msb = constp->ਚੌੜਾਈ_Min() - 1;
        }
        // updateBitRange(), limitBitRangeToLsb(), and polarity() must be called during ascending
        // back to the root.
        ਖਾਲੀ update_ਦੋਆਧਾਰੀ_ਅੰਕ_Range(ਪੂਰਨ newLsb, ਪੂਰਨ newMsb) {
            ਜੇ ((m_lsb <= m_msb && newLsb > newMsb) || (m_lsb > m_msb && m_lsb < newLsb)) {
                // When the new bit range is out of m_refp, clear polarity because nodes below is
                // shifted out to zero.
                // This kind of clear may happen several times. e.g. (!(1'b1 >> 1)) >> 1
                polarity(ਸੱਚ);
            }
            m_lsb = newLsb;
            m_msb = newMsb;
        }
        ਖਾਲੀ update_ਦੋਆਧਾਰੀ_ਅੰਕ_Range(ਅਚਲ AstCCast* castp) {
            update_ਦੋਆਧਾਰੀ_ਅੰਕ_Range(m_lsb, std::min(m_msb, m_lsb + castp->ਚੌੜਾਈ() - 1));
        }
        ਖਾਲੀ update_ਦੋਆਧਾਰੀ_ਅੰਕ_Range(ਅਚਲ Ast_ਖਿਸਕਾਓ_R* shiftp) {
            update_ਦੋਆਧਾਰੀ_ਅੰਕ_Range(m_lsb + VN_AS(shiftp->rhsp(), Const)->ਮੰਜ਼ਿਲ_UInt(), m_msb);
        }
        ਖਾਲੀ ਹੱਦਬੰਦੀ_ਦੋਆਧਾਰੀ_ਅੰਕ_Range_ਮੰਜ਼ਿਲ_Lsb() { update_ਦੋਆਧਾਰੀ_ਅੰਕ_Range(m_lsb, std::min(m_msb, m_lsb)); }
        ਪੂਰਨ ਸ਼ਬਦ_Idx() ਅਚਲ { ਵਾਪਸ m_ਸ਼ਬਦ_Idx; }
        ਖਾਲੀ ਸ਼ਬਦ_Idx(ਪੂਰਨ i) { m_ਸ਼ਬਦ_Idx = i; }
        ਤਾਰਕਿਕ polarity() ਅਚਲ { ਵਾਪਸ m_polarity; }
        ਖਾਲੀ polarity(ਤਾਰਕਿਕ p) { m_polarity = p; }

        AstVarRef* refp() ਅਚਲ { ਵਾਪਸ m_refp; }
        ਅਚਲ AstConst* constp() ਅਚਲ { ਵਾਪਸ m_constp; }
        ਤਾਰਕਿਕ missing_ਸ਼ਬਦ_Sel() ਅਚਲ {
            // When V3Expand is skipped, WordSel is not inserted.
            ਵਾਪਸ m_refp->isWide() && m_ਸ਼ਬਦ_Idx == -1;
        }
        ਪੂਰਨ lsb() ਅਚਲ { ਵਾਪਸ m_lsb; }

        ਪੂਰਨ msb() ਅਚਲ { ਵਾਪਸ std::min(m_msb, var_ਚੌੜਾਈ() - 1); }
        ਪੂਰਨ var_ਚੌੜਾਈ() ਅਚਲ {
            UASSERT(m_refp, "m_refp should be set");
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = m_refp->varp()->ਚੌੜਾਈ_Min();
            ਜੇ (!m_refp->isWide()) {
                UASSERT_OBJ(m_ਸ਼ਬਦ_Idx == -1, m_refp, "Bad word index into non-wide");
                ਵਾਪਸ ਚੌੜਾਈ;
            } ਨਹੀਂਤਾਂ {
                ਜੇ (missing_ਸ਼ਬਦ_Sel()) ਵਾਪਸ ਚੌੜਾਈ;
                UASSERT_OBJ(m_ਸ਼ਬਦ_Idx >= 0, m_refp, "Bad word index into wide");
                ਅਚਲ ਪੂਰਨ ਬਿੱਟ_ਵਿੱਚ_MSW = VL_BITBIT_E(ਚੌੜਾਈ) ? VL_BITBIT_E(ਚੌੜਾਈ) : VL_EDATASIZE;
                ਵਾਪਸ m_ਸ਼ਬਦ_Idx == m_refp->ਚੌੜਾਈ_ਸ਼ਬਦ() - 1 ? ਬਿੱਟ_ਵਿੱਚ_MSW : VL_EDATASIZE;
            }
        }
    };

    ਬਣਤਰ ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity_ਇੰਦਰਾਜ final {  // Found bit polarity during iterate()
        ਪੱਤੀ_Info m_info;  // Leaf (variable or constant) bit polarity was found on
        ਤਾਰਕਿਕ m_polarity = ਝੂਠ;  // Polarity the bit must have to match
        ਪੂਰਨ m_ਦੋਆਧਾਰੀ_ਅੰਕ = 0;  // Bit index within the leaf that was tested
        ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity_ਇੰਦਰਾਜ(ਅਚਲ ਪੱਤੀ_Info& info, ਤਾਰਕਿਕ pol, ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2)
            : m_info{info}
            , m_polarity{pol}
            , m_ਦੋਆਧਾਰੀ_ਅੰਕ{ਦੋਆਧਾਰੀ_ਅੰਕ_2} {}
        ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity_ਇੰਦਰਾਜ() = ਮੂਲ;
    };

    ਬਣਤਰ Frozen_ਗੰਢ_Info final {  // Context when a frozen node is found
        ਤਾਰਕਿਕ m_polarity;  // Polarity the frozen node must match
        ਪੂਰਨ m_lsb;  // LSB position of the frozen node
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ<(ਅਚਲ Frozen_ਗੰਢ_Info& other) ਅਚਲ {
            ਜੇ (m_lsb != other.m_lsb) ਵਾਪਸ m_lsb < other.m_lsb;
            ਵਾਪਸ m_polarity < other.m_polarity;
        }
    };

    ਵਰਗ Restorer final {  // Restore the original state unless disableRestore() is called
        Const_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ_Visitor& m_visitor;  // Visitor whose state is saved and restored
        ਅਚਲ ਮਾਪ_t m_polarities_ਮਾਪ;  // Saved m_visitor.m_bitPolarities size to truncate back to
        ਅਚਲ ਮਾਪ_t m_frozen_ਮਾਪ;  // Saved m_visitor.m_frozenNodes size to truncate back to
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ m_ops;  // Saved m_visitor.m_ops to restore
        ਅਚਲ ਤਾਰਕਿਕ m_polarity;  // Saved m_visitor.m_polarity to restore
        ਤਾਰਕਿਕ m_restore = ਸੱਚ;  // Whether the destructor still needs to restore state

    ਜਨਤਕ:
        ਸਪਸ਼ਟ Restorer(Const_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ_Visitor& visitor)
            : m_visitor{visitor}
            , m_polarities_ਮਾਪ{visitor.m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarities.ਮਾਪ()}
            , m_frozen_ਮਾਪ{visitor.m_frozenNodes.ਮਾਪ()}
            , m_ops{visitor.m_ops}
            , m_polarity{visitor.m_polarity} {}
        ~Restorer() {
            UASSERT(m_visitor.m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarities.ਮਾਪ() >= m_polarities_ਮਾਪ,
                    "m_bitPolarities must grow monotonically");
            UASSERT(m_visitor.m_frozenNodes.ਮਾਪ() >= m_frozen_ਮਾਪ,
                    "m_frozenNodes must grow monotonically");
            ਜੇ (m_restore) restoreNow();
        }
        ਖਾਲੀ disableRestore() { m_restore = ਝੂਠ; }
        ਖਾਲੀ restoreNow() {
            UASSERT(m_restore, "Can be called just once");
            m_visitor.m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarities.resize(m_polarities_ਮਾਪ);
            m_visitor.m_frozenNodes.resize(m_frozen_ਮਾਪ);
            m_visitor.m_ops = m_ops;
            m_visitor.m_polarity = m_polarity;
            m_restore = ਝੂਠ;
        }
    };
    // Collect information for each Variable to transform as below
    ਵਰਗ VarInfo final {
        // MEMBERS
        ਪੂਰਨ m_ਗਿਆਤ_ਨਤੀਜਾ = -1;  // -1: result is not known, 0 or 1: result of this tree
        ਅਚਲ Const_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ_Visitor* ਅਚਲ
            m_parentp;  // ConstBitOpTreeVisitor holding this VarInfo
        AstVarRef* ਅਚਲ m_refp;  // Points the variable that this VarInfo covers
        ਅਚਲ ਪੂਰਨ m_ਚੌੜਾਈ;  // Width of term this VarInfo refers to
        V3_ਸੰਖਿਆ m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity;  // Coefficient of each bit

    ਜਨਤਕ:
        // METHODS
        ਤਾਰਕਿਕ has_Const_ਨਤੀਜਾ() ਅਚਲ { ਵਾਪਸ m_ਗਿਆਤ_ਨਤੀਜਾ >= 0 || m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity.isAllX(); }
        // The constant result. Only valid if hasConstResult() returned true.
        ਤਾਰਕਿਕ get_Const_ਨਤੀਜਾ() ਅਚਲ {
            // Note that this condition covers m_knownResult == -1 but m_bitPolarity.isAllX(),
            // in which case the result is 0
            ਵਾਪਸ m_ਗਿਆਤ_ਨਤੀਜਾ == 1;
        }
        ਅਚਲ AstVarRef* refp() ਅਚਲ { ਵਾਪਸ m_refp; }
        ਤਾਰਕਿਕ sameVarAs(ਅਚਲ Ast_ਗੰਢ_Var_Ref* otherp) ਅਚਲ { ਵਾਪਸ m_refp->same_ਗੰਢ(otherp); }
        ਖਾਲੀ ਸੈੱਟ_Polarity(ਤਾਰਕਿਕ comp_ਦੋਆਧਾਰੀ_ਅੰਕ, ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
            // Ignore if already determined a known reduction
            ਜੇ (m_ਗਿਆਤ_ਨਤੀਜਾ >= 0) ਵਾਪਸ;
            UASSERT_OBJ(ਦੋਆਧਾਰੀ_ਅੰਕ_2 < m_ਚੌੜਾਈ, m_refp,
                        "Bit index out of range: " << ਦੋਆਧਾਰੀ_ਅੰਕ_2 << " width: " << m_ਚੌੜਾਈ);
            ਜੇ (m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_X(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {  // The bit is not yet marked with either polarity
                m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, comp_ਦੋਆਧਾਰੀ_ਅੰਕ);
            } ਨਹੀਂਤਾਂ {  // The bit has already been marked with some polarity
                ਅਚਲ ਤਾਰਕਿਕ sameFlag = m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2) == comp_ਦੋਆਧਾਰੀ_ਅੰਕ;
                ਜੇ (m_parentp->is_Xor_ਰੁੱਖ()) {
                    UASSERT_OBJ(comp_ਦੋਆਧਾਰੀ_ਅੰਕ && sameFlag, m_refp, "Only true is set in Xor tree");
                    // a ^ a ^ b == b so we can ignore a
                    m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 'x');
                } ਨਹੀਂਤਾਂ {  // And, Or
                    // Can ignore this nodep as the bit is already marked with the same polarity
                    ਜੇ (sameFlag) ਵਾਪਸ;  // a & a == a, b | b == b
                    // Otherwise result is constant (a & ~a == 0) or (a | ~a == 1)
                    m_ਗਿਆਤ_ਨਤੀਜਾ = m_parentp->is_And_ਰੁੱਖ() ? 0 : 1;
                    m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity.ਸੈੱਟ_All_ਬਿੱਟ_X();  // The variable is not referred anymore
                }
            }
        }

        // Return reduction term for this VarInfo, together with the number of ops in the term,
        // and a boolean indicating if the term is clean (1-bit vs multi-bit value)
        ਨਤੀਜਾ_ਪਦ get_ਨਤੀਜਾ_ਪਦ() ਅਚਲ {
            UASSERT_OBJ(!has_Const_ਨਤੀਜਾ(), m_refp, "getTerm on reduction that yields constant");
            File_ਸਤਰ* ਅਚਲ fl = m_refp->fileline();

            // Get the term we are referencing (the WordSel, if wide, otherwise just the VarRef)
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* srcp = VN_CAST(m_refp->backp(), ਸ਼ਬਦ_Sel);
            ਜੇ (!srcp) srcp = m_refp;
            srcp = srcp->clone_ਰੁੱਖ(ਝੂਠ);

            // Signed variables might have redundant sign bits that need masking.
            ਅਚਲ ਤਾਰਕਿਕ has_Redundant_Sign_ਬਿੱਟ
                = m_refp->varp()->dtypep()->isSigned()
                  && (m_refp->isWide() ? (m_ਚੌੜਾਈ != VL_EDATASIZE)
                                       : (m_ਚੌੜਾਈ < 8 || !isPow2(m_ਚੌੜਾਈ)));

            // Get the mask that selects the bits that are relevant in this term
            V3_ਸੰਖਿਆ ਨਕਾਬ_ਅੰਕ{srcp, m_ਚੌੜਾਈ, 0};
            ਨਕਾਬ_ਅੰਕ.op_ਬਿੱਟ_Non_XZ(m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity);  // 'x' -> 0, 0->1, 1->1
            ਅਚਲ uint64_t ਨਕਾਬ_Val = ਨਕਾਬ_ਅੰਕ.ਮੰਜ਼ਿਲ_UQuad();
            UASSERT_OBJ(ਨਕਾਬ_Val != 0, m_refp,
                        "Should have been recognized as having const 0 result");

            // Parts of the return value
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* resultp = srcp;  // The tree for this term
            ਬਿਨਾਨਿਸ਼ਾਨੀ ops = 0;  // Number of ops in this term
            ਤਾਰਕਿਕ clean = ਝੂਠ;  // Whether the term is clean (has value 0 or 1)

            ਜੇ (isPow2(ਨਕਾਬ_Val)) {
                // If we only want a single bit, shift it out instead of a masked compare. Shifts
                // don't go through the flags register on x86 and are hence faster. This is also
                // always fewer or same ops as mask and compare, but with shorter instructions on
                // x86.

                // Find the index of the bit we want.
                ਅਚਲ ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = ਗਿਣਤੀ_Trailing_Zeroes(ਨਕਾਬ_Val);
                // If we want something other than the bottom bit, shift it out
                ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 != 0) {
                    resultp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_R{fl, resultp,
                                            ਨਵਾਂ AstConst{fl, static_cast<uint32_t>(ਦੋਆਧਾਰੀ_ਅੰਕ_2)}, m_ਚੌੜਾਈ};
                    ++ops;
                }
                // Negate it if necessary
                ਅਚਲ ਤਾਰਕਿਕ negate = m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2);
                ਜੇ (negate) {
                    resultp = ਨਵਾਂ AstNot{fl, resultp};
                    ++ops;
                }
                // Clean if MSB of unsigned value, and not negated
                clean = (ਦੋਆਧਾਰੀ_ਅੰਕ_2 == m_ਚੌੜਾਈ - 1) && !has_Redundant_Sign_ਬਿੱਟ && !negate;
            } ਨਹੀਂਤਾਂ {
                // We want multiple bits. Go ahead and extract them.

                // Check if masking is required, and if so apply it
                ਅਚਲ ਤਾਰਕਿਕ needsMasking = ਨਕਾਬ_Val != VL_ਨਕਾਬ_Q(m_ਚੌੜਾਈ) || has_Redundant_Sign_ਬਿੱਟ;
                ਜੇ (needsMasking) {
                    resultp = ਨਵਾਂ AstAnd{fl, ਨਵਾਂ AstConst{fl, ਨਕਾਬ_ਅੰਕ}, resultp};
                    ++ops;
                }

                // Create the sub-expression for this term
                ਜੇ (m_parentp->is_Xor_ਰੁੱਖ()) {
                    ਜੇ (needsMasking) {
                        // Reduce the masked term to the minimum known width,
                        // to use the smallest RedXor formula
                        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ_Min = ਨਕਾਬ_ਅੰਕ.ਚੌੜਾਈ_ਮੰਜ਼ਿਲ_Fit();
                        resultp->dtype_Chg_ਚੌੜਾਈ(ਚੌੜਾਈ_Min, ਚੌੜਾਈ_Min);
                    }
                    resultp = ਨਵਾਂ AstRedXor{fl, resultp};
                    ++ops;
                    clean = ਝੂਠ;
                    // VL_REDXOR_* returns IData, set width accordingly to avoid unnecessary casts
                    resultp->dtype_Chg_ਚੌੜਾਈ(VL_IDATASIZE, 1);
                } ਨਹੀਂਤਾਂ ਜੇ (m_parentp->is_And_ਰੁੱਖ()) {
                    V3_ਸੰਖਿਆ comp_ਅੰਕ{srcp, m_ਚੌੜਾਈ, 0};
                    comp_ਅੰਕ.op_ਬਿੱਟ_One(m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity);  // 'x'->0, 0->0, 1->1
                    resultp = ਨਵਾਂ AstEq{fl, ਨਵਾਂ AstConst{fl, comp_ਅੰਕ}, resultp};
                    ++ops;
                    clean = ਸੱਚ;
                } ਨਹੀਂਤਾਂ {  // Or
                    V3_ਸੰਖਿਆ comp_ਅੰਕ{srcp, m_ਚੌੜਾਈ, 0};
                    comp_ਅੰਕ.op_ਬਿੱਟ_One(m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity);  // 'x'->0, 0->0, 1->1
                    comp_ਅੰਕ.opXor(V3_ਸੰਖਿਆ{comp_ਅੰਕ}, ਨਕਾਬ_ਅੰਕ);
                    resultp = ਨਵਾਂ AstNeq{fl, ਨਵਾਂ AstConst{fl, comp_ਅੰਕ}, resultp};
                    ++ops;
                    clean = ਸੱਚ;
                }
            }

            ਵਾਪਸ ਨਤੀਜਾ_ਪਦ{resultp, ops, clean};
        }

        // CONSTRUCTORS
        VarInfo(Const_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ_Visitor* ਮਾਪਾ, AstVarRef* refp, ਪੂਰਨ ਚੌੜਾਈ)
            : m_parentp{ਮਾਪਾ}
            , m_refp{refp}
            , m_ਚੌੜਾਈ{ਚੌੜਾਈ}
            , m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity{refp, m_ਚੌੜਾਈ} {
            m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity.ਸੈੱਟ_All_ਬਿੱਟ_X();
        }
    };

    // MEMBERS
    ਤਾਰਕਿਕ m_failed = ਝੂਠ;
    ਤਾਰਕਿਕ m_polarity = ਸੱਚ;  // Flip when AstNot comes
    ਬਿਨਾਨਿਸ਼ਾਨੀ m_ops;  // Number of operations such as And, Or, Xor, Sel...
    ਪੂਰਨ m_lsb = 0;  // Current LSB
    ਪੱਤੀ_Info* m_leafp = ਖਾਲੀ_ਸੂਚਕ;  // AstConst or AstVarRef that currently looking for
    ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ m_rootp;  // Root of this AST subtree

    std::ਸਦਿਸ਼<std::pair<Ast_ਗੰਢ_ਵਿਅੰਜਕ*, Frozen_ਗੰਢ_Info>>
        m_frozenNodes;  // Nodes that cannot be optimized
    std::ਸਦਿਸ਼<ਦੋਆਧਾਰੀ_ਅੰਕ_Polarity_ਇੰਦਰਾਜ> m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarities;  // Polarity of bits found during iterate()
    std::ਸਦਿਸ਼<std::unique_ptr<VarInfo>> m_varInfos;  // VarInfo for each variable, [0] is nullptr
    VNIdAllocator& m_ids;  // Node id allocator, owned by ConstVisitor
    // Base index of m_varInfos that points VarInfo, keyed by AstVarScope/AstVarRef id,
    // zero means not set yet.
    std::unordered_map<ਮਾਪ_t, ਪੂਰਨ> m_ਅਧਾਰ_Idxs;

    // METHODS

    ਤਾਰਕਿਕ is_And_ਰੁੱਖ() ਅਚਲ { ਵਾਪਸ VN_IS(m_rootp, And); }
    ਤਾਰਕਿਕ is_Or_ਰੁੱਖ() ਅਚਲ { ਵਾਪਸ VN_IS(m_rootp, Or); }
    ਤਾਰਕਿਕ is_Xor_ਰੁੱਖ() ਅਚਲ { ਵਾਪਸ VN_IS(m_rootp, Xor) || VN_IS(m_rootp, RedXor); }

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ CONST_BITOP_RETURN_IF(cond, nodep) \
    ਜੇ (ਸੈੱਟ_Failed(cond, #cond, nodep, __LINE__)) ਵਾਪਸ

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ CONST_BITOP_ਸੈੱਟ_FAILED(reason, nodep) ਸੈੱਟ_Failed(ਸੱਚ, reason, nodep, __LINE__)

    ਤਾਰਕਿਕ ਸੈੱਟ_Failed(ਤਾਰਕਿਕ fail, ਅਚਲ ਅੱਖਰ* reason, ਅਚਲ Ast_ਗੰਢ* nodep, ਪੂਰਨ ਸਤਰ) {
        ਜੇ (fail && !m_failed) {
            UINFO(9, "cannot optimize " << m_rootp << " reason:" << reason << " called from line:"
                                        << ਸਤਰ << " when checking:" << nodep);
            // UINFOTREE(9, m_rootp, "", "root");
            m_failed = ਸੱਚ;
        }
        ਵਾਪਸ m_failed;
    }
    ਖਾਲੀ incrOps(ਅਚਲ Ast_ਗੰਢ* nodep, ਪੂਰਨ ਸਤਰ) {
        ++m_ops;
        UINFO(9, "Increment to " << m_ops << " " << nodep << " called from line " << ਸਤਰ);
    }
    VarInfo& getVarInfo(ਅਚਲ ਪੱਤੀ_Info& ref) {
        UASSERT_OBJ(ref.refp(), m_rootp, "null varref in And/Or/Xor optimization");
        Ast_ਗੰਢ* nodep = ref.refp()->varScopep();
        ਜੇ (!nodep) nodep = ref.refp()->varp();  // Not scoped
        ਪੂਰਨ& ਅਧਾਰ_Idxr = m_ਅਧਾਰ_Idxs[m_ids(nodep)];
        ਜੇ (ਅਧਾਰ_Idxr == 0) {  // Not set yet
            ਅਧਾਰ_Idxr = m_varInfos.ਮਾਪ();
            ਅਚਲ ਪੂਰਨ ਅੰਕ_ਸ਼ਬਦ
                = ref.refp()->dtypep()->isWide() ? ref.refp()->dtypep()->ਚੌੜਾਈ_ਸ਼ਬਦ() : 1;
            m_varInfos.resize(m_varInfos.ਮਾਪ() + ਅੰਕ_ਸ਼ਬਦ);
        }
        ਅਚਲ ਮਾਪ_t idx = ਅਧਾਰ_Idxr + std::max(0, ref.ਸ਼ਬਦ_Idx());
        VarInfo* varInfop = m_varInfos[idx].get();
        ਜੇ (!varInfop) {
            varInfop = ਨਵਾਂ VarInfo{ਇਹ, ref.refp(), ref.var_ਚੌੜਾਈ()};
            m_varInfos[idx].reset(varInfop);
            ਜੇ (ref.missing_ਸ਼ਬਦ_Sel()) {
                // ConstBitOpTreeVisitor makes some constants for masks and its type is uint64_t.
                // That's why V3Expand, that inserts WordSel, is needed.
                CONST_BITOP_ਸੈੱਟ_FAILED("V3Expand is skipped", ref.refp());
            }
        } ਨਹੀਂਤਾਂ {
            ਜੇ (!varInfop->sameVarAs(ref.refp()))
                CONST_BITOP_ਸੈੱਟ_FAILED("different var (scope?)", ref.refp());
        }
        ਵਾਪਸ *varInfop;
    }

    // Traverse down to see AstConst or AstVarRef
    ਪੱਤੀ_Info ਲੱਭੋ_ਪੱਤੀ(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ expectConst) {
        ਜੇ (!nodep->dtypep()->skipRefp()->isIntegralOrPacked()) ਵਾਪਸ ਪੱਤੀ_Info{};
        ਪੱਤੀ_Info info{m_lsb};
        {
            VL_RESTORER(m_leafp);
            m_leafp = &info;
            iterateConst(nodep);
        }

        ਤਾਰਕਿਕ ok = !m_failed;
        ਜੇ (expectConst) {
            ok &= !info.refp() && info.constp();
        } ਨਹੀਂਤਾਂ {
            ok &= info.refp() && !info.constp();
        }
        ਵਾਪਸ ok ? info : ਪੱਤੀ_Info{};
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { CONST_BITOP_ਸੈੱਟ_FAILED("Hit unexpected op", nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstCCast* nodep) override {
        iterateChildrenConst(nodep);
        ਜੇ (m_leafp) m_leafp->update_ਦੋਆਧਾਰੀ_ਅੰਕ_Range(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਖਿਸਕਾਓ_R* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->rhsp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->rhsp());
        m_lsb += constp->ਮੰਜ਼ਿਲ_UInt();
        incrOps(nodep, __LINE__);
        iterateConst(nodep->lhsp());
        m_leafp->update_ਦੋਆਧਾਰੀ_ਅੰਕ_Range(nodep);
        m_lsb -= constp->ਮੰਜ਼ਿਲ_UInt();
    }
    ਖਾਲੀ ਦੇਖੋ(AstNot* nodep) override {
        CONST_BITOP_RETURN_IF(nodep->ਚੌੜਾਈ_Min() != 1, nodep);
        Ast_ਗੰਢ* lhsp = nodep->lhsp();
        AstCCast* ਅਚਲ castp = VN_CAST(lhsp, CCast);
        ਜੇ (castp) lhsp = castp->lhsp();
        CONST_BITOP_RETURN_IF(!is_Xor_ਰੁੱਖ() && !VN_IS(lhsp, VarRef) && !VN_IS(lhsp, ਖਿਸਕਾਓ_R), lhsp);
        incrOps(nodep, __LINE__);
        m_polarity = !m_polarity;
        iterateChildrenConst(nodep);
        // Don't restore m_polarity for Xor as it counts parity of the entire tree
        ਜੇ (!is_Xor_ਰੁੱਖ()) m_polarity = !m_polarity;
        ਜੇ (m_leafp && castp) m_leafp->update_ਦੋਆਧਾਰੀ_ਅੰਕ_Range(castp);
        ਜੇ (m_leafp) m_leafp->polarity(!m_leafp->polarity());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸ਼ਬਦ_Sel* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->bitp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->bitp());
        UASSERT_OBJ(m_leafp->ਸ਼ਬਦ_Idx() == -1, nodep, "Unexpected nested WordSel");
        m_leafp->ਸ਼ਬਦ_Idx(constp->ਮੰਜ਼ਿਲ_SInt());
        iterateConst(nodep->fromp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->ਸੈੱਟ_ਪੱਤੀ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConst* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->ਸੈੱਟ_ਪੱਤੀ(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstRedXor* nodep) override {
        Restorer restorer{*ਇਹ};
        CONST_BITOP_RETURN_IF(!VN_IS(m_rootp, Xor), nodep);
        Ast_ਗੰਢ* lhsp = nodep->lhsp();
        ਅਚਲ AstCCast* ਅਚਲ castp = VN_CAST(lhsp, CCast);
        ਜੇ (castp) lhsp = castp->lhsp();
        ਜੇ (ਅਚਲ AstAnd* ਅਚਲ andp = VN_CAST(lhsp, And)) {  // '^(mask & leaf)'
            CONST_BITOP_RETURN_IF(!andp, lhsp);

            ਅਚਲ ਪੱਤੀ_Info& ਨਕਾਬ = ਲੱਭੋ_ਪੱਤੀ(andp->lhsp(), ਸੱਚ);
            CONST_BITOP_RETURN_IF(!ਨਕਾਬ.constp() || ਨਕਾਬ.lsb() != 0, andp->lhsp());

            ਪੱਤੀ_Info ref = ਲੱਭੋ_ਪੱਤੀ(andp->rhsp(), ਝੂਠ);
            CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());
            ਜੇ (castp) ref.update_ਦੋਆਧਾਰੀ_ਅੰਕ_Range(castp);

            restorer.disableRestore();  // Now all subtree succeeded

            ਅਚਲ V3_ਸੰਖਿਆ& ਨਕਾਬ_ਅੰਕ = ਨਕਾਬ.constp()->ਅੰਕ();

            incrOps(nodep, __LINE__);
            incrOps(andp, __LINE__);

            // Mark all bits checked in this reduction
            ਅਚਲ ਪੂਰਨ max_ਦੋਆਧਾਰੀ_ਅੰਕ_Idx = std::min(ref.lsb() + ਨਕਾਬ_ਅੰਕ.ਚੌੜਾਈ(), ref.msb() + 1);
            ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_Idx = ref.lsb(); ਦੋਆਧਾਰੀ_ਅੰਕ_Idx < max_ਦੋਆਧਾਰੀ_ਅੰਕ_Idx; ++ਦੋਆਧਾਰੀ_ਅੰਕ_Idx) {
                ਅਚਲ ਪੂਰਨ ਨਕਾਬ_Idx = ਦੋਆਧਾਰੀ_ਅੰਕ_Idx - ref.lsb();
                ਜੇ (ਨਕਾਬ_ਅੰਕ.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਨਕਾਬ_Idx)) ਜਾਰੀ;
                // Set true, m_polarity takes care of the entire parity
                m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarities.emplace_ਪਿੱਠ(ref, ਸੱਚ, ਦੋਆਧਾਰੀ_ਅੰਕ_Idx);
            }
        } ਨਹੀਂਤਾਂ {  // '^leaf'
            ਪੱਤੀ_Info ref = ਲੱਭੋ_ਪੱਤੀ(lhsp, ਝੂਠ);
            CONST_BITOP_RETURN_IF(!ref.refp(), lhsp);
            ਜੇ (castp) ref.update_ਦੋਆਧਾਰੀ_ਅੰਕ_Range(castp);

            restorer.disableRestore();  // Now all checks passed

            incrOps(nodep, __LINE__);

            // Mark all bits checked by this comparison
            ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_Idx = ref.lsb(); ਦੋਆਧਾਰੀ_ਅੰਕ_Idx <= ref.msb(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_Idx) {
                m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarities.emplace_ਪਿੱਠ(ref, ਸੱਚ, ਦੋਆਧਾਰੀ_ਅੰਕ_Idx);
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Biop* nodep) override {
        ਜੇ (VN_IS(nodep, And) && isConst(nodep->lhsp(), 1)) {  // 1 & _
            // Always reach past a plain making AND
            Restorer restorer{*ਇਹ};
            incrOps(nodep, __LINE__);
            iterateConst(nodep->rhsp());
            ਜੇ (m_leafp) m_leafp->ਹੱਦਬੰਦੀ_ਦੋਆਧਾਰੀ_ਅੰਕ_Range_ਮੰਜ਼ਿਲ_Lsb();
            CONST_BITOP_RETURN_IF(m_failed, nodep->rhsp());
            restorer.disableRestore();  // Now all checks passed
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->type() == m_rootp->type()) {  // And, Or, Xor
            // subtree under NOT can be optimized only in XOR tree.
            CONST_BITOP_RETURN_IF(!m_polarity && !is_Xor_ਰੁੱਖ(), nodep);
            incrOps(nodep, __LINE__);
            ਲਈ (ਅਚਲ ਤਾਰਕਿਕ ਸੱਜੇ : {ਝੂਠ, ਸੱਚ}) {
                VL_RESTORER(m_leafp);
                Restorer restorer{*ਇਹ};
                ਪੱਤੀ_Info ਪੱਤੀ_Info_2{m_lsb};
                // cppcheck-suppress danglingLifetime
                m_leafp = &ਪੱਤੀ_Info_2;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* opp = ਸੱਜੇ ? nodep->rhsp() : nodep->lhsp();
                ਅਚਲ ਤਾਰਕਿਕ origFailed = m_failed;
                iterateConst(opp);
                ਜੇ (ਪੱਤੀ_Info_2.constp() || m_failed) {
                    // Revert changes in leaf
                    restorer.restoreNow();
                    // Reach past a cast then add to frozen nodes to be added to final reduction
                    ਜੇ (ਅਚਲ AstCCast* ਅਚਲ castp = VN_CAST(opp, CCast)) opp = castp->lhsp();
                    ਅਚਲ ਤਾਰਕਿਕ pol = is_Xor_ਰੁੱਖ() || m_polarity;  // Only AND/OR tree needs polarity
                    UASSERT_OBJ(pol, nodep, "AND/OR tree expects m_polarity==true");
                    m_frozenNodes.emplace_ਪਿੱਠ(opp, Frozen_ਗੰਢ_Info{pol, m_lsb});
                    m_failed = origFailed;
                    ਜਾਰੀ;
                }
                restorer.disableRestore();  // Now all checks passed
                ਜੇ (ਪੱਤੀ_Info_2.refp()) {
                    // The conditional on the lsb being in range is necessary for some degenerate
                    // case, e.g.: (IData)((QData)wide[0] >> 32), or <1-bit-var> >> 1, which is
                    // just zero
                    ਜੇ (ਪੱਤੀ_Info_2.lsb() <= ਪੱਤੀ_Info_2.msb()) {
                        m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarities.emplace_ਪਿੱਠ(ਪੱਤੀ_Info_2, is_Xor_ਰੁੱਖ() || ਪੱਤੀ_Info_2.polarity(),
                                                     ਪੱਤੀ_Info_2.lsb());
                    } ਨਹੀਂਤਾਂ ਜੇ ((is_And_ਰੁੱਖ() && ਪੱਤੀ_Info_2.polarity())
                               || (is_Or_ਰੁੱਖ() && !ਪੱਤੀ_Info_2.polarity())) {
                        // If there is a constant 0 term in an And tree or 1 term in an Or tree, we
                        // must include it. Fudge this by adding a bit with both polarities, which
                        // will simplify to zero or one respectively.
                        // Note that Xor tree does not need this kind of care, polarity of Xor tree
                        // is already cared when visitin AstNot. Taking xor with 1'b0 is nop.
                        m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarities.emplace_ਪਿੱਠ(ਪੱਤੀ_Info_2, ਸੱਚ, 0);
                        m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarities.emplace_ਪਿੱਠ(ਪੱਤੀ_Info_2, ਝੂਠ, 0);
                    }
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ ((is_And_ਰੁੱਖ() && VN_IS(nodep, Eq)) || (is_Or_ਰੁੱਖ() && VN_IS(nodep, Neq))) {
            Restorer restorer{*ਇਹ};
            CONST_BITOP_RETURN_IF(!m_polarity, nodep);
            CONST_BITOP_RETURN_IF(m_lsb, nodep);  // the result of EQ/NE is 1 bit width
            ਅਚਲ Ast_ਗੰਢ* lhsp = nodep->lhsp();
            ਜੇ (ਅਚਲ AstCCast* ਅਚਲ castp = VN_CAST(lhsp, CCast)) lhsp = castp->lhsp();
            ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(lhsp, Const);
            CONST_BITOP_RETURN_IF(!constp, nodep->lhsp());

            ਅਚਲ V3_ਸੰਖਿਆ& comp_ਅੰਕ = constp->ਅੰਕ();

            ਸਵੈਚਾਲਿਤ ਸੈੱਟ_Polarities = [ਇਹ, &comp_ਅੰਕ](ਅਚਲ ਪੱਤੀ_Info& ref, ਅਚਲ V3_ਸੰਖਿਆ* maskp) {
                ਅਚਲ ਤਾਰਕਿਕ ਨਕਾਬ_Flip = is_And_ਰੁੱਖ() ^ ref.polarity();
                ਪੂਰਨ constant_ਚੌੜਾਈ = comp_ਅੰਕ.ਚੌੜਾਈ();
                ਜੇ (maskp) constant_ਚੌੜਾਈ = std::max(constant_ਚੌੜਾਈ, maskp->ਚੌੜਾਈ());
                ਅਚਲ ਪੂਰਨ max_ਦੋਆਧਾਰੀ_ਅੰਕ_Idx = std::max(ref.lsb() + constant_ਚੌੜਾਈ, ref.msb() + 1);
                // Mark all bits checked by this comparison
                ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_Idx = ref.lsb(); ਦੋਆਧਾਰੀ_ਅੰਕ_Idx < max_ਦੋਆਧਾਰੀ_ਅੰਕ_Idx; ++ਦੋਆਧਾਰੀ_ਅੰਕ_Idx) {
                    ਅਚਲ ਪੂਰਨ ਨਕਾਬ_Idx = ਦੋਆਧਾਰੀ_ਅੰਕ_Idx - ref.lsb();
                    ਅਚਲ ਤਾਰਕਿਕ mask0 = maskp && maskp->ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਨਕਾਬ_Idx);
                    ਅਚਲ ਤਾਰਕਿਕ ਬਾਹਰਲਾ_ਦਾ_Range = ਦੋਆਧਾਰੀ_ਅੰਕ_Idx > ref.msb();
                    ਜੇ (mask0 || ਬਾਹਰਲਾ_ਦਾ_Range) {  // RHS is 0
                        ਜੇ (comp_ਅੰਕ.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਨਕਾਬ_Idx)) {
                            // LHS is 1
                            // And tree: 1 == 0 => always false, set v && !v
                            // Or tree : 1 != 0 => always true, set v || !v
                            m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarities.emplace_ਪਿੱਠ(ref, ਸੱਚ, 0);
                            m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarities.emplace_ਪਿੱਠ(ref, ਝੂਠ, 0);
                            ਰੋਕੋ;
                        } ਨਹੀਂਤਾਂ {  // This bitIdx is irrelevant
                            ਜਾਰੀ;
                        }
                    }
                    ਅਚਲ ਤਾਰਕਿਕ polarity = comp_ਅੰਕ.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਨਕਾਬ_Idx) != ਨਕਾਬ_Flip;
                    m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarities.emplace_ਪਿੱਠ(ref, polarity, ਦੋਆਧਾਰੀ_ਅੰਕ_Idx);
                }
            };

            ਜੇ (ਅਚਲ AstAnd* ਅਚਲ andp = VN_CAST(nodep->rhsp(), And)) {  // comp == (mask & v)
                ਅਚਲ ਪੱਤੀ_Info& ਨਕਾਬ = ਲੱਭੋ_ਪੱਤੀ(andp->lhsp(), ਸੱਚ);
                CONST_BITOP_RETURN_IF(!ਨਕਾਬ.constp() || ਨਕਾਬ.lsb() != 0, andp->lhsp());

                ਅਚਲ ਪੱਤੀ_Info& ref = ਲੱਭੋ_ਪੱਤੀ(andp->rhsp(), ਝੂਠ);
                CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());

                restorer.disableRestore();  // Now all checks passed

                ਅਚਲ V3_ਸੰਖਿਆ& ਨਕਾਬ_ਅੰਕ = ਨਕਾਬ.constp()->ਅੰਕ();

                incrOps(nodep, __LINE__);
                incrOps(andp, __LINE__);

                ਸੈੱਟ_Polarities(ref, &ਨਕਾਬ_ਅੰਕ);
            } ਨਹੀਂਤਾਂ {  // comp == v
                ਅਚਲ ਪੱਤੀ_Info& ref = ਲੱਭੋ_ਪੱਤੀ(nodep->rhsp(), ਝੂਠ);
                CONST_BITOP_RETURN_IF(!ref.refp(), nodep->rhsp());

                restorer.disableRestore();  // Now all checks passed

                incrOps(nodep, __LINE__);

                ਸੈੱਟ_Polarities(ref, ਖਾਲੀ_ਸੂਚਕ);
            }
        } ਨਹੀਂਤਾਂ {
            CONST_BITOP_ਸੈੱਟ_FAILED("Mixture of different ops cannot be optimized", nodep);
        }
    }

    // CONSTRUCTORS
    Const_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ_Visitor(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਬਿਨਾਨਿਸ਼ਾਨੀ externalOps, VNIdAllocator& ids)
        : m_ops{externalOps}
        , m_rootp{nodep}
        , m_ids{ids} {
        // Fill nullptr at [0] because a base index of 0 means not set yet
        m_varInfos.ਧੱਕੋ_ਪਿੱਠ(ਖਾਲੀ_ਸੂਚਕ);
        CONST_BITOP_RETURN_IF(!is_And_ਰੁੱਖ() && !is_Or_ਰੁੱਖ() && !is_Xor_ਰੁੱਖ(), nodep);
        ਜੇ (Ast_ਗੰਢ_Biop* ਅਚਲ biopp = VN_CAST(nodep, ਗੰਢ_Biop)) {
            iterateConst(biopp);
        } ਨਹੀਂਤਾਂ {
            UASSERT_OBJ(VN_IS(nodep, RedXor), nodep, "Must be RedXor");
            incrOps(nodep, __LINE__);
            iterateChildrenConst(nodep);
        }
        ਲਈ (ਸਵੈਚਾਲਿਤ&& ਇੰਦਰਾਜ : m_ਦੋਆਧਾਰੀ_ਅੰਕ_Polarities) {
            getVarInfo(ਇੰਦਰਾਜ.m_info).ਸੈੱਟ_Polarity(ਇੰਦਰਾਜ.m_polarity, ਇੰਦਰਾਜ.m_ਦੋਆਧਾਰੀ_ਅੰਕ);
        }
        UASSERT_OBJ(is_Xor_ਰੁੱਖ() || m_polarity, nodep, "must be the original polarity");
    }
    ਆਭਾਸੀ ~Const_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ_Visitor() = ਮੂਲ;
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ CONST_BITOP_RETURN_IF
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ CONST_BITOP_ਸੈੱਟ_FAILED

ਜਨਤਕ:
    // Transform as below.
    // v[0] & v[1] => 2'b11 == (2'b11 & v)
    // v[0] | v[1] => 2'b00 != (2'b11 & v)
    // v[0] ^ v[1] => ^{2'b11 & v}
    // (3'b011 == (3'b011 & v)) & v[2]  => 3'b111 == (3'b111 & v)
    // (3'b000 != (3'b011 & v)) | v[2]  => 3'b000 != (3'b111 & v)
    // Reduction ops are transformed in the same way.
    // &{v[0], v[1]} => 2'b11 == (2'b11 & v)
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* simplify(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਪੂਰਨ ਨਤੀਜਾ_ਚੌੜਾਈ, ਬਿਨਾਨਿਸ਼ਾਨੀ externalOps,
                                 VDouble0& reduction, VNIdAllocator& ids) {
        UASSERT_OBJ(1 <= ਨਤੀਜਾ_ਚੌੜਾਈ && ਨਤੀਜਾ_ਚੌੜਾਈ <= 64, nodep, "resultWidth out of range");

        // Walk tree, gathering all terms referenced in expression
        ਅਚਲ Const_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ_Visitor visitor{nodep, externalOps, ids};

        // If failed on root node is not optimizable, or there are no variable terms, then done
        ਜੇ (visitor.m_failed || visitor.m_varInfos.ਮਾਪ() == 1) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        // FileLine used for constructing all new nodes in this function
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();

        // Get partial result each term referenced, count total number of ops and keep track of
        // whether we have clean/dirty terms. visitor.m_varInfos appears in deterministic order,
        // so the optimized tree is deterministic as well.

        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> termps;
        termps.reserve(visitor.m_varInfos.ਮਾਪ() - 1);
        ਬਿਨਾਨਿਸ਼ਾਨੀ ਨਤੀਜਾ_Ops = 0;
        ਤਾਰਕਿਕ has_Clean_ਪਦ = ਝੂਠ;
        ਤਾਰਕਿਕ has_Dirty_ਪਦ = ਝੂਠ;

        ਲਈ (ਸਵੈਚਾਲਿਤ&& v : visitor.m_varInfos) {
            ਜੇ (!v) ਜਾਰੀ;  // Skip nullptr at m_varInfos[0]
            ਜੇ (v->has_Const_ਨਤੀਜਾ()) {
                // If a constant term is known, we can either drop it or the whole tree is constant
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* resultp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (v->get_Const_ਨਤੀਜਾ()) {
                    UASSERT_OBJ(visitor.is_Or_ਰੁੱਖ(), nodep,
                                "Only OR tree can yield known 1 result");
                    UINFO(9, "OR tree with const 1 term: " << v->refp());
                    // Known 1 bit in OR tree, whole result is 1
                    resultp = ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
                } ਨਹੀਂਤਾਂ ਜੇ (visitor.is_And_ਰੁੱਖ()) {
                    UINFO(9, "AND tree with const 0 term: " << v->refp());
                    // Known 0 bit in AND tree, whole result is 0
                    resultp = ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
                } ਨਹੀਂਤਾਂ {
                    // Known 0 bit in OR or XOR tree. Ignore it.
                    ਜਾਰੀ;
                }
                // Set width and widthMin precisely
                resultp->dtype_Chg_ਚੌੜਾਈ(ਨਤੀਜਾ_ਚੌੜਾਈ, 1);
                ਲਈ (Ast_ਗੰਢ* ਅਚਲ termp : termps) VL_DO_DANGLING(termp->delete_ਰੁੱਖ(), termp);
                ਵਾਪਸ resultp;
            }
            ਅਚਲ ਨਤੀਜਾ_ਪਦ ਨਤੀਜਾ = v->get_ਨਤੀਜਾ_ਪਦ();
            termps.ਧੱਕੋ_ਪਿੱਠ(std::get<0>(ਨਤੀਜਾ));
            ਨਤੀਜਾ_Ops += std::get<1>(ਨਤੀਜਾ);
            ਜੇ (std::get<2>(ਨਤੀਜਾ)) {
                has_Clean_ਪਦ = ਸੱਚ;
                UINFO(9, "Clean term: " << termps.ਪਿੱਠ());
            } ਨਹੀਂਤਾਂ {
                has_Dirty_ਪਦ = ਸੱਚ;
                UINFO(9, "Dirty term: " << termps.ਪਿੱਠ());
            }
        }

        // Group by FrozenNodeInfo
        std::map<Frozen_ਗੰਢ_Info, std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>> frozenNodes;
        // Check if frozen terms are clean or not
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& frozenInfo : visitor.m_frozenNodes) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ termp = frozenInfo.ਪਹਿਲਾ;
            // Comparison operators are clean
            ਜੇ ((VN_IS(termp, Eq) || VN_IS(termp, Neq) || VN_IS(termp, Lt) || VN_IS(termp, Lte)
                 || VN_IS(termp, Gt) || VN_IS(termp, Gte))
                && frozenInfo.ਦੂਜਾ.m_lsb == 0) {
                has_Clean_ਪਦ = ਸੱਚ;
            } ਨਹੀਂਤਾਂ {
                // Otherwise, conservatively assume the frozen term is dirty
                has_Dirty_ਪਦ = ਸੱਚ;
                UINFO(9, "Dirty frozen term: " << termp);
            }
            frozenNodes[frozenInfo.ਦੂਜਾ].ਧੱਕੋ_ਪਿੱਠ(termp);
        }

        // Figure out if a final negation is required
        ਅਚਲ ਤਾਰਕਿਕ needsFlip = visitor.is_Xor_ਰੁੱਖ() && !visitor.m_polarity;

        // Figure out if the final tree needs cleaning
        ਅਚਲ ਤਾਰਕਿਕ needsCleaning = visitor.is_And_ਰੁੱਖ() ? !has_Clean_ਪਦ : has_Dirty_ਪਦ;

        // Add size of reduction tree to op count
        ਨਤੀਜਾ_Ops += termps.ਮਾਪ() - 1;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& lsbAndNodes : frozenNodes) {
            ਜੇ (lsbAndNodes.ਪਹਿਲਾ.m_lsb > 0) ++ਨਤੀਜਾ_Ops;  // Needs AstShiftR
            ਜੇ (!lsbAndNodes.ਪਹਿਲਾ.m_polarity) ++ਨਤੀਜਾ_Ops;  // Needs AstNot
            ਨਤੀਜਾ_Ops += lsbAndNodes.ਦੂਜਾ.ਮਾਪ();
        }
        // Add final polarity flip in Xor tree
        ਜੇ (needsFlip) ++ਨਤੀਜਾ_Ops;
        // Add final cleaning AND
        ਜੇ (needsCleaning) ++ਨਤੀਜਾ_Ops;

        ਜੇ (debug() >= 9) {  // LCOV_EXCL_START
            ਬਾਹਰ << "-  Bitop tree considered:\n";
            ਲਈ (ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ termp : termps) termp->dump_ਰੁੱਖ("-  Reduced term: ");
            ਲਈ (ਅਚਲ std::pair<Ast_ਗੰਢ_ਵਿਅੰਜਕ*, Frozen_ਗੰਢ_Info>& termp : visitor.m_frozenNodes) {
                termp.ਪਹਿਲਾ->dump_ਰੁੱਖ("-  Frozen term with lsb "
                                      + std::ਮੰਜ਼ਿਲ_string(termp.ਦੂਜਾ.m_lsb) + " polarity "
                                      + std::ਮੰਜ਼ਿਲ_string(termp.ਦੂਜਾ.m_polarity) + ": ");
            }
            ਬਾਹਰ << "-  Needs flipping: " << needsFlip << "\n";
            ਬਾਹਰ << "-  Needs cleaning: " << needsCleaning << "\n";
            ਬਾਹਰ << "-  Size: " << ਨਤੀਜਾ_Ops << " input size: " << visitor.m_ops << "\n";
        }  // LCOV_EXCL_STOP

        // Sometimes we have no terms left after ignoring redundant terms
        // (all of which were zeroes)
        ਜੇ (termps.ਖਾਲੀ_ਥਾਵਾਂ() && visitor.m_frozenNodes.ਖਾਲੀ_ਥਾਵਾਂ()) {
            reduction += visitor.m_ops;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ resultp = needsFlip ? ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}
                                                   : ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
            resultp->dtype_Chg_ਚੌੜਾਈ(ਨਤੀਜਾ_ਚੌੜਾਈ, 1);
            ਵਾਪਸ resultp;
        }

        // Only substitute the result if beneficial as determined by operation count
        ਜੇ (visitor.m_ops <= ਨਤੀਜਾ_Ops) {
            ਲਈ (Ast_ਗੰਢ* ਅਚਲ termp : termps) VL_DO_DANGLING(termp->delete_ਰੁੱਖ(), termp);
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }

        // Update statistics
        reduction += visitor.m_ops - ਨਤੀਜਾ_Ops;

        // Reduction op to combine terms
        ਅਚਲ ਸਵੈਚਾਲਿਤ reduce = [&visitor, fl](Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
            ਜੇ (!lhsp) ਵਾਪਸ rhsp;
            ਜੇ (visitor.is_And_ਰੁੱਖ()) {
                ਵਾਪਸ ਨਵਾਂ AstAnd{fl, lhsp, rhsp};
            } ਨਹੀਂਤਾਂ ਜੇ (visitor.is_Or_ਰੁੱਖ()) {
                ਵਾਪਸ ਨਵਾਂ AstOr{fl, lhsp, rhsp};
            } ਨਹੀਂਤਾਂ {
                ਵਾਪਸ ਨਵਾਂ AstXor{fl, lhsp, rhsp};
            }
        };

        // Compute result by reducing all terms
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* resultp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ termp : termps) {  //
            resultp = reduce(resultp, termp);
        }
        // Add any frozen terms to the reduction
        ਲਈ (ਸਵੈਚਾਲਿਤ&& nodes : frozenNodes) {
            // nodes.second has same lsb and polarity
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* termp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ itemp : nodes.ਦੂਜਾ) {
                termp = reduce(termp, itemp->unlink_Fr_ਪਿੱਠ());
            }
            ਜੇ (nodes.ਪਹਿਲਾ.m_lsb > 0) {  // LSB is not 0, so shiftR
                Ast_ਗੰਢ_DType* ਅਚਲ dtypep = termp->dtypep();
                termp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_R{termp->fileline(), termp,
                                      ਨਵਾਂ AstConst(termp->fileline(), AstConst::Widthed_ਮੁੱਲ{},
                                                   termp->ਚੌੜਾਈ(), nodes.ਪਹਿਲਾ.m_lsb)};
                termp->dtypep(dtypep);
            }
            ਜੇ (!nodes.ਪਹਿਲਾ.m_polarity) {  // Polarity is inverted, so append Not
                Ast_ਗੰਢ_DType* ਅਚਲ dtypep = termp->dtypep();
                termp = ਨਵਾਂ AstNot{termp->fileline(), termp};
                termp->dtypep(dtypep);
            }
            resultp = reduce(resultp, termp);
        }

        // Set width of masks to expected result width. This is required to prevent later removal
        // of the masking node e.g. by the "AND with all ones" rule. If the result width happens
        // to be 1, we still need to ensure the AstAnd is not dropped, so use a wider mask in this
        // special case.
        ਅਚਲ ਪੂਰਨ ਨਕਾਬ_ਚੌੜਾਈ
            = std::max(resultp->ਚੌੜਾਈ(), ਨਤੀਜਾ_ਚੌੜਾਈ == 1 ? VL_IDATASIZE : ਨਤੀਜਾ_ਚੌੜਾਈ);

        // Apply final polarity flip
        ਜੇ (needsFlip) {
            ਜੇ (needsCleaning) {
                // Cleaning will be added below. Use a NOT which is a byte shorter on x86
                resultp = ਨਵਾਂ AstNot{fl, resultp};
            } ਨਹੀਂਤਾਂ {
                // Keep result clean by using XOR(1, _)
                AstConst* ਅਚਲ maskp = ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, ਨਕਾਬ_ਚੌੜਾਈ, 1};
                resultp = ਨਵਾਂ AstXor{fl, maskp, resultp};
            }
        }

        // Apply final cleaning
        ਜੇ (needsCleaning) {
            AstConst* ਅਚਲ maskp = ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, ਨਕਾਬ_ਚੌੜਾਈ, 1};
            resultp = ਨਵਾਂ AstAnd{fl, maskp, resultp};
        }

        // Cast back to original size if required
        ਜੇ (resultp->ਚੌੜਾਈ() != ਨਤੀਜਾ_ਚੌੜਾਈ) {
            resultp = ਨਵਾਂ AstCCast{fl, resultp, ਨਤੀਜਾ_ਚੌੜਾਈ, 1};
        }

        // Set width and widthMin precisely
        resultp->dtype_Chg_ਚੌੜਾਈ(ਨਤੀਜਾ_ਚੌੜਾਈ, 1);

        ਵਾਪਸ resultp;
    }
};

//######################################################################
// Const state, as a visitor of each AstNode

ਵਰਗ ConstVisitor final : ਜਨਤਕ VNVisitor {
    // CONSTANTS
    ਸਥਿਰ constexpr ਬਿਨਾਨਿਸ਼ਾਨੀ CONCAT_MERGABLE_MAX_ਡੂੰਘਾਈ = 10;  // Limit alg recursion
    // Largest right-hand side that is copied to each part of a split concatenation assignment
    ਸਥਿਰ constexpr ਪੂਰਨ CONCAT_ਸੌਂਪੋ_MAX_RHS_NODES = 16;

    // NODE STATE
    // See VNIdAllocator. All per node state below is held in side tables keyed on the
    // node ids it hands out, as node pointers are not stable keys within V3Const.

    // STATE
    VNIdAllocator m_ids;  // Node id allocator
    ਤਾਰਕਿਕ m_params = ਝੂਠ;  // If true, propagate parameterized and true numbers only
    ਤਾਰਕਿਕ m_required = ਝੂਠ;  // If true, must become a constant
    ਤਾਰਕਿਕ m_wremove = ਸੱਚ;  // Inside scope, no assignw removal
    ਤਾਰਕਿਕ m_warn = ਝੂਠ;  // Output warnings
    ਤਾਰਕਿਕ m_doExpensive = ਝੂਠ;  // Enable computationally expensive optimizations
    ਤਾਰਕਿਕ m_doCpp = ਝੂਠ;  // Enable late-stage C++ optimizations
    ਤਾਰਕਿਕ m_doNConst = ਝੂਠ;  // Enable non-constant-child simplifications
    ਤਾਰਕਿਕ m_doV = ਝੂਠ;  // Verilog, not C++ conversion
    ਤਾਰਕਿਕ m_doGenerate = ਝੂਠ;  // Postpone width checking inside generate
    ਤਾਰਕਿਕ m_convert_Logic_ਮੰਜ਼ਿਲ_ਦੋਆਧਾਰੀ_ਅੰਕ = ਝੂਠ;  // Convert logical operators to bitwise
    ਤਾਰਕਿਕ m_hasJumpDelay = ਝੂਠ;  // JumpGo or Delay under this loop
    ਤਾਰਕਿਕ m_hasLoopTest = ਝੂਠ;  // Contains AstLoopTest
    ਤਾਰਕਿਕ m_underRecFunc = ਝੂਠ;  // Under a recursive function
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    ਅਚਲ AstArraySel* m_selp = ਖਾਲੀ_ਸੂਚਕ;  // Current select
    ਅਚਲ AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Current scope
    ਅਚਲ Ast_Attr_ਦਾ* m_attrp = ਖਾਲੀ_ਸੂਚਕ;  // Current attribute
    VDouble0 m_stat_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_Reduction;  // Ops reduced in ConstBitOpTreeVisitor
    VDouble0 m_stat_Concat_ਮਿਲਾਓ;  // Concat merges
    VDouble0 m_stat_Cond_ਵਿਅੰਜਕ_Redundant;  // Conditional repeated expressions
    VDouble0 m_stat_If_Cond_ਵਿਅੰਜਕ_Redundant;  // Conditional repeated expressions
    VDouble0 m_statMemberAccessVisits;  // Nodes visited by containsMemberAccessRecurse
    ਅਚਲ ਤਾਰਕਿਕ m_globalPass;  // ConstVisitor invoked as a global pass
    ਸਥਿਰ uint32_t s_global_Pass_ਅੰਕ;  // Counts number of times ConstVisitor invoked as global pass
    V3_Unique_ਨਾਂ m_concswap_ਨਾਂ;  // For generating unique temporary variable names
    // Caches results of matchBiopToBitwise, keyed on node id
    std::unordered_map<ਮਾਪ_t, ਤਾਰਕਿਕ> m_ਸ਼ਾਮਲ_ਹੈ_Member_Access;
    // Items of enum currently being recursed into. Cannot use VNIdAllocator, as this runs in
    // every mode, including those invoked under a pass holding user4 (V3Param). Node pointers
    // are safe as keys here, unlike elsewhere in V3Const: folding the item value below does
    // free nodes whose addresses are then reused, but AstEnumItem is only ever constructed
    // while parsing (verilog.y and V3LinkParse), so a reused address can never become one,
    // and every key looked up is an AstEnumItem.
    std::unordered_ਸੈੱਟ<ਅਚਲ Ast_Enum_ਵਸਤੂ*> m_recursing_Enum_ਚੀਜ਼ਾਂ;
    std::unordered_ਸੈੱਟ<Ast_Jump_ਖੰਡ*> m_ਵਰਤਿਆ_Jump_ਖੰਡਾਂ;  // JumpBlocks used by some JumpGo

    // METHODS

    // Return true if statements may be deleted. Until statics are lifted, a variable declared
    // in a statement might be referenced from elsewhere (e.g. hierarchically).
    ਸਥਿਰ ਤਾਰਕਿਕ canDeleteStatements() { ਵਾਪਸ v3Global.staticsLifted(); }

    ਖਾਲੀ deleteVarScopesUnder(Ast_ਗੰਢ* subtreep) {
        ਜੇ (!subtreep) ਵਾਪਸ;
        ਜੇ (!m_scopep) ਵਾਪਸ;
        std::unordered_ਸੈੱਟ<AstVar*> varps;
        subtreep->foreach_And_ਅਗਲਾ([&](AstVar* varp) { varps.ਪਾਓ(varp); });
        ਜੇ (varps.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;
        ਲਈ (AstVarScope *vscp = m_scopep->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            ਜੇ (varps.ਲੱਭੋ(vscp->varp()) != varps.ਸਿਰਾ()) {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(vscp->unlink_Fr_ਪਿੱਠ()), vscp);
            }
        }
    }

    V3_ਸੰਖਿਆ const_ਅੰਕ_V(Ast_ਗੰਢ* nodep) {
        // Contract C width to V width (if needed, else just direct copy)
        // The upper zeros in the C representation can otherwise cause
        // wrong results in some operations, e.g. MulS
        ਅਚਲ V3_ਸੰਖਿਆ& numc = VN_AS(nodep, Const)->ਅੰਕ();
        ਵਾਪਸ !numc.is_ਸੰਖਿਆ() ? numc : V3_ਸੰਖਿਆ{nodep, nodep->ਚੌੜਾਈ_Min_V(), numc};
    }
    V3_ਸੰਖਿਆ ਮੰਜ਼ਿਲ_ਅੰਕ_C(Ast_ਗੰਢ* nodep, ਅਚਲ V3_ਸੰਖਿਆ& numv) {
        // Extend V width back to C width for given node
        ਵਾਪਸ !numv.is_ਸੰਖਿਆ() ? numv : V3_ਸੰਖਿਆ{nodep, nodep->ਚੌੜਾਈ(), numv};
    }

    ਸਥਿਰ ਤਾਰਕਿਕ ਹੇਠਲਾ_As_Fixed_Aggregate(ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep) {
        ਵਾਪਸ dtypep->isStreamableFixedAggregate() && dtypep->ਸ਼ਾਮਲ_ਹੈ_Unpacked_Struct();
    }

    AstStructSel* newStructSel(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp, ਅਚਲ AstMemberDType* ਅਚਲ itemp) {
        AstStructSel* ਅਚਲ selp = ਨਵਾਂ AstStructSel{fromp->fileline(), fromp, itemp->name()};
        selp->dtypeFrom(itemp->dtypep());
        ਵਾਪਸ selp;
    }

    ਖਾਲੀ collectFixedAggregateTerms(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp, std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>& termps,
                                    ਅਚਲ ਤਾਰਕਿਕ packReal) {
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = fromp->dtypep()->skipRefp();
        ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ unpackDtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            ਲਈ (ਅਚਲ ਪੂਰਨ idx : unpackDtypep->declRange().seq_ਖੱਬਾ_ਮੰਜ਼ਿਲ_ਸੱਜਾ()) {
                AstArraySel* ਅਚਲ selp
                    = ਨਵਾਂ AstArraySel{fromp->fileline(), fromp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), idx};
                collectFixedAggregateTerms(selp, termps, packReal);
            }
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(fromp), fromp);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ sdtypep
                   = VN_CAST(dtypep, ਗੰਢ_UOr_Struct_DType)) {
            ਜੇ (sdtypep->packed()) {
                termps.ਧੱਕੋ_ਪਿੱਠ(fromp);
                ਵਾਪਸ;
            }
            ਲਈ (ਅਚਲ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                collectFixedAggregateTerms(newStructSel(fromp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), itemp),
                                           termps, packReal);
            }
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(fromp), fromp);
        } ਨਹੀਂਤਾਂ ਜੇ (packReal && dtypep->isDouble()) {
            termps.ਧੱਕੋ_ਪਿੱਠ(ਨਵਾਂ Ast_Real_ਮੰਜ਼ਿਲ_ਬਿੱਟ{fromp->fileline(), fromp});
        } ਨਹੀਂਤਾਂ {
            termps.ਧੱਕੋ_ਪਿੱਠ(fromp);
        }
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* packFixedAggregate(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp) {
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> termps;
        collectFixedAggregateTerms(fromp, termps, ਸੱਚ);
        UASSERT(!termps.ਖਾਲੀ_ਥਾਵਾਂ(), "No stream terms");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* resultp = termps[0];
        ਲਈ (ਮਾਪ_t i = 1; i < termps.ਮਾਪ(); ++i) {
            resultp = ਨਵਾਂ AstConcat{resultp->fileline(), resultp, termps[i]};
        }
        ਵਾਪਸ resultp;
    }

    ਖਾਲੀ replace_ਸੌਂਪੋ_ਮੰਜ਼ਿਲ_Fixed_Aggregate(Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ dstp,
                                       Ast_ਗੰਢ_ਵਿਅੰਜਕ* srcp) {
        ਅਚਲ ਪੂਰਨ dst_ਚੌੜਾਈ = dstp->dtypep()->ਚੌੜਾਈ_ਧਾਰਾ();
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> termps;
        collectFixedAggregateTerms(dstp, termps, ਝੂਠ);
        ਪੂਰਨ src_ਚੌੜਾਈ = srcp->ਚੌੜਾਈ();
        ਜੇ (src_ਚੌੜਾਈ < dst_ਚੌੜਾਈ) {
            AstExtend* ਅਚਲ extendp = ਨਵਾਂ AstExtend{srcp->fileline(), srcp, dst_ਚੌੜਾਈ};
            srcp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{
                extendp->fileline(), extendp,
                ਨਵਾਂ AstConst{extendp->fileline(), static_cast<uint32_t>(dst_ਚੌੜਾਈ - src_ਚੌੜਾਈ)},
                dst_ਚੌੜਾਈ};
            src_ਚੌੜਾਈ = dst_ਚੌੜਾਈ;
        }
        Ast_ਗੰਢ_ਸੌਂਪੋ* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਪੂਰਨ ਵਿੱਥ = 0;
        ਲਈ (ਮਾਪ_t i = 0; i < termps.ਮਾਪ(); ++i) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ termp = termps[i];
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = termp->dtypep()->ਚੌੜਾਈ_ਧਾਰਾ();
            ਅਚਲ ਪੂਰਨ lsb = src_ਚੌੜਾਈ - ਵਿੱਥ - ਚੌੜਾਈ;
            ਵਿੱਥ += ਚੌੜਾਈ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp
                = ਨਵਾਂ AstSel{srcp->fileline(), srcp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), lsb, ਚੌੜਾਈ};
            ਜੇ (termp->dtypep()->skipRefp()->isDouble()) {
                rhsp = ਨਵਾਂ Ast_ਬਿੱਟ_ਮੰਜ਼ਿਲ_Real_D{rhsp->fileline(), rhsp};
            }
            Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assignp = nodep->cloneType(termp, rhsp);
            assignp->dtypeFrom(termp);
            newp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(newp, assignp);
        }
        nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(srcp), srcp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }

    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Const(ਅਚਲ Ast_ਗੰਢ* nodep) { ਵਾਪਸ VN_IS(nodep, Const); }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Asv_Const(ਅਚਲ Ast_ਗੰਢ* nodep) {
        // BIASV(CONST, BIASV(CONST,...)) -> BIASV( BIASV_CONSTED(a,b), ...)
        ਅਚਲ Ast_ਗੰਢ_Bi_Com_Asv* ਅਚਲ bnodep = VN_CAST(nodep, ਗੰਢ_Bi_Com_Asv);
        ਜੇ (!bnodep) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!VN_IS(bnodep->lhsp(), Const)) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_ਗੰਢ_Bi_Com_Asv* ਅਚਲ rnodep = VN_CAST(bnodep->rhsp(), ਗੰਢ_Bi_Com_Asv);
        ਜੇ (!rnodep) ਵਾਪਸ ਝੂਠ;
        ਜੇ (rnodep->type() != bnodep->type()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (rnodep->ਚੌੜਾਈ() != bnodep->ਚੌੜਾਈ()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (rnodep->lhsp()->ਚੌੜਾਈ() != bnodep->lhsp()->ਚੌੜਾਈ()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!VN_IS(rnodep->lhsp(), Const)) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Asv_Same(ਅਚਲ Ast_ਗੰਢ* nodep) {
        // BIASV(SAMEa, BIASV(SAMEb,...)) -> BIASV( BIASV(SAMEa,SAMEb), ...)
        ਅਚਲ Ast_ਗੰਢ_Bi_Com_Asv* ਅਚਲ bnodep = VN_CAST(nodep, ਗੰਢ_Bi_Com_Asv);
        ਜੇ (!bnodep) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_ਗੰਢ_Bi_Com_Asv* ਅਚਲ rnodep = VN_CAST(bnodep->rhsp(), ਗੰਢ_Bi_Com_Asv);
        ਜੇ (!rnodep) ਵਾਪਸ ਝੂਠ;
        ਜੇ (rnodep->type() != bnodep->type()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (rnodep->ਚੌੜਾਈ() != bnodep->ਚੌੜਾਈ()) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ operandsSame(bnodep->lhsp(), rnodep->lhsp());
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Asv_LUp(ਅਚਲ Ast_ਗੰਢ* nodep) {
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
        ਅਚਲ Ast_ਗੰਢ_Bi_Com_Asv* ਅਚਲ bnodep = VN_CAST(nodep, ਗੰਢ_Bi_Com_Asv);
        ਜੇ (!bnodep) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_ਗੰਢ_Bi_Com_Asv* ਅਚਲ lnodep = VN_CAST(bnodep->lhsp(), ਗੰਢ_Bi_Com_Asv);
        ਜੇ (!lnodep) ਵਾਪਸ ਝੂਠ;
        ਜੇ (lnodep->type() != bnodep->type()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (lnodep->ਚੌੜਾਈ() != bnodep->ਚੌੜਾਈ()) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ VN_IS(lnodep->lhsp(), Const);
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Asv_RUp(ਅਚਲ Ast_ਗੰਢ* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr)) ?
        ਅਚਲ Ast_ਗੰਢ_Bi_Com_Asv* ਅਚਲ bnodep = VN_CAST(nodep, ਗੰਢ_Bi_Com_Asv);
        ਜੇ (!bnodep) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_ਗੰਢ_Bi_Com_Asv* ਅਚਲ rnodep = VN_CAST(bnodep->rhsp(), ਗੰਢ_Bi_Com_Asv);
        ਜੇ (!rnodep) ਵਾਪਸ ਝੂਠ;
        ਜੇ (rnodep->type() != bnodep->type()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (rnodep->ਚੌੜਾਈ() != bnodep->ਚੌੜਾਈ()) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ VN_IS(rnodep->lhsp(), Const);
    }
    ਸਥਿਰ ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Sub_ਜੋੜੋ(ਅਚਲ Ast_ਗੰਢ* nodep) {
        // SUB( ADD(CONSTx,y), CONSTz) -> ADD(SUB(CONSTx,CONSTz), y)
        ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ np = VN_CAST(nodep, ਗੰਢ_Biop);
        ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ lp = VN_CAST(np->lhsp(), ਗੰਢ_Biop);
        ਵਾਪਸ (lp && VN_IS(lp->lhsp(), Const) && VN_IS(np->rhsp(), Const)
                && lp->ਚੌੜਾਈ() == np->ਚੌੜਾਈ());
    }
    ਤਾਰਕਿਕ ਮੇਲ_Redundant_Clean(AstAnd* andp) {
        // Remove And with constant one inserted by V3Clean
        // 1 & (a == b)  -> (IData)(a == b)
        // When bool is casted to int, the value is either 0 or 1
        AstConst* ਅਚਲ constp = VN_AS(andp->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isOne(), andp->lhsp(), "TRREEOPC must meet this condition");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = andp->rhsp();
        AstCCast* ccastp = ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ ਸਵੈਚਾਲਿਤ isEqOrNeq = [](ਅਚਲ Ast_ਗੰਢ* nodep) -> ਤਾਰਕਿਕ {  //
            ਵਾਪਸ VN_IS(nodep, Eq) || VN_IS(nodep, Neq);
        };
        ਜੇ (isEqOrNeq(rhsp)) {
            ccastp = ਨਵਾਂ AstCCast{andp->fileline(), rhsp->unlink_Fr_ਪਿੱਠ(), andp};
        } ਨਹੀਂਤਾਂ ਜੇ (AstCCast* ਅਚਲ tmpp = VN_CAST(rhsp, CCast)) {
            ਜੇ (isEqOrNeq(tmpp->lhsp())) {
                ਜੇ (tmpp->ਚੌੜਾਈ() == andp->ਚੌੜਾਈ()) {
                    tmpp->unlink_Fr_ਪਿੱਠ();
                    ccastp = tmpp;
                } ਨਹੀਂਤਾਂ {
                    ccastp = ਨਵਾਂ AstCCast{andp->fileline(), tmpp->lhsp()->unlink_Fr_ਪਿੱਠ(), andp};
                }
            }
        }
        ਜੇ (ccastp) {
            andp->replaceWithKeepDType(ccastp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(andp), andp);
            ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ ਸੰਕਾਰਜ_And_Or_Same(ਅਚਲ Ast_ਗੰਢ* nodep) {
        // OR( AND(VAL,x), AND(VAL,y)) -> AND(VAL,OR(x,y))
        // OR( AND(x,VAL), AND(y,VAL)) -> AND(OR(x,y),VAL)
        ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ np = VN_CAST(nodep, ਗੰਢ_Biop);
        ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ lp = VN_CAST(np->lhsp(), ਗੰਢ_Biop);
        ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ rp = VN_CAST(np->rhsp(), ਗੰਢ_Biop);
        ਵਾਪਸ (lp && rp && lp->ਚੌੜਾਈ() == rp->ਚੌੜਾਈ() && lp->type() == rp->type()
                && (operandsSame(lp->lhsp(), rp->lhsp()) || operandsSame(lp->rhsp(), rp->rhsp())));
    }
    ਤਾਰਕਿਕ ਮੇਲ_Or_And_Not(Ast_ਗੰਢ_Biop* nodep) {
        // AstOr{$a, AstAnd{AstNot{$b}, $c}} if $a.width1, $a==$b => AstOr{$a,$c}
        // Someday we'll sort the biops completely and this can be simplified
        // This often results from our simplified clock generation:
        // if (rst) ... else if (enable)... -> OR(rst,AND(!rst,enable))
        ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ap;
        Ast_ਗੰਢ_Biop* andp;
        ਜੇ (VN_IS(nodep->lhsp(), And)) {
            andp = VN_AS(nodep->lhsp(), And);
            ap = nodep->rhsp();
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep->rhsp(), And)) {
            andp = VN_AS(nodep->rhsp(), And);
            ap = nodep->lhsp();
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ ਝੂਠ;
        }
        ਅਚਲ Ast_ਗੰਢ_Uniop* notp;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* cp;
        ਜੇ (VN_IS(andp->lhsp(), Not)) {
            notp = VN_AS(andp->lhsp(), Not);
            cp = andp->rhsp();
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(andp->rhsp(), Not)) {
            notp = VN_AS(andp->rhsp(), Not);
            cp = andp->lhsp();
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ ਝੂਠ;
        }
        ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bp = notp->lhsp();
        ਜੇ (!operandsSame(ap, bp)) ਵਾਪਸ ਝੂਠ;
        // Do it
        cp->unlink_Fr_ਪਿੱਠ();
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(andp->unlink_Fr_ਪਿੱਠ()), andp);
        VL_DANGLING(notp);
        // Replace whichever branch is now dangling
        ਜੇ (nodep->rhsp()) {
            nodep->lhsp(cp);
        } ਨਹੀਂਤਾਂ {
            nodep->rhsp(cp);
        }
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ ਮੇਲ_And_Cond(AstAnd* nodep) {
        // Push down a AND into conditional, when one side of conditional is constant
        // (otherwise we'd be trading one operation for two operations)
        // V3Clean often makes this pattern, as it postpones the AND until
        // as high as possible, which is usually the right choice, except for this.
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstCond* ਅਚਲ condp = VN_CAST(nodep->rhsp(), Cond);
        ਜੇ (!condp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!VN_IS(condp->thenp(), Const) && !VN_IS(condp->elsep(), Const)) ਵਾਪਸ ਝੂਠ;
        AstConst* ਅਚਲ maskp = VN_CAST(nodep->lhsp(), Const);
        ਜੇ (!maskp) ਵਾਪਸ ਝੂਠ;
        UINFO(4, "AND(CONSTm, CONDcond(c, i, e))->CONDcond(c, AND(m,i), AND(m, e)) " << nodep);
        AstCond* ਅਚਲ newp = ਨਵਾਂ AstCond{
            condp->fileline(), condp->condp()->unlink_Fr_ਪਿੱਠ(),
            ਨਵਾਂ AstAnd{nodep->fileline(), maskp->clone_ਰੁੱਖ(ਝੂਠ), condp->thenp()->unlink_Fr_ਪਿੱਠ()},
            ਨਵਾਂ AstAnd{nodep->fileline(), maskp->clone_ਰੁੱਖ(ਝੂਠ),
                       condp->elsep()->unlink_Fr_ਪਿੱਠ()}};
        newp->thenp()->dtypeFrom(nodep);  // As And might have been to change widths
        newp->elsep()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        ਵਾਪਸ ਸੱਚ;
    }

    ਤਾਰਕਿਕ ਮੇਲ_Cond_Cond(ਅਚਲ AstCond* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        ਜੇ (VN_IS(nodep->backp(), Cond)) ਵਾਪਸ ਝੂਠ;  // Was checked when visited parent
        ਜੇ (!VN_IS(nodep->thenp(), Cond) && !VN_IS(nodep->elsep(), Cond))
            ਵਾਪਸ ਝੂਠ;  // Short circuit
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> truesp;
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> falsesp;
        ਮੇਲ_Cond_Cond_Recurse(nodep, truesp /*ref*/, falsesp /*ref*/);
        ਵਾਪਸ ਝੂਠ;  // Can optimize further
    }
    ਖਾਲੀ ਮੇਲ_Cond_Cond_Recurse(ਅਚਲ AstCond* nodep, std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>& truesp,
                              std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        ਜੇ (truesp.ਮਾਪ() > 4 || falsesp.ਮਾਪ() > 4) ਵਾਪਸ;
        ਜੇ (!nodep->condp()->is_ਸ਼ੁੱਧ()) ਵਾਪਸ;
        ਤਾਰਕਿਕ replaced = ਝੂਠ;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp : truesp) {
            ਜੇ (replaced) ਰੋਕੋ;
            ਜੇ (!operandsSame(nodep->condp(), condp)) ਜਾਰੀ;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_ਅੰਕ(nodep->condp(), 1);
            replaced = ਸੱਚ;
            ++m_stat_Cond_ਵਿਅੰਜਕ_Redundant;
        }
        ਲਈ (ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp : falsesp) {
            ਜੇ (replaced) ਰੋਕੋ;
            ਜੇ (!operandsSame(nodep->condp(), condp)) ਜਾਰੀ;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = ਸੱਚ;
            ++m_stat_Cond_ਵਿਅੰਜਕ_Redundant;
        }
        ਜੇ (ਅਚਲ AstCond* subCondp = VN_CAST(nodep->thenp(), Cond)) {
            ਜੇ (!replaced) truesp.emplace_ਪਿੱਠ(nodep->condp());
            ਮੇਲ_Cond_Cond_Recurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            ਜੇ (!replaced) truesp.ਕੱਢੋ_ਪਿੱਠ();
        }
        ਜੇ (ਅਚਲ AstCond* subCondp = VN_CAST(nodep->elsep(), Cond)) {
            ਜੇ (!replaced) falsesp.emplace_ਪਿੱਠ(nodep->condp());
            ਮੇਲ_Cond_Cond_Recurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            ਜੇ (!replaced) falsesp.ਕੱਢੋ_ਪਿੱਠ();
        }
    }
    ਖਾਲੀ ਮੇਲ_If_Cond_Cond(ਅਚਲ Ast_ਗੰਢ_If* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        ਜੇ (VN_IS(nodep->backp(), If)) ਵਾਪਸ;  // Was checked when visited parent
        ਜੇ (!VN_IS(nodep->thensp(), If) && !VN_IS(nodep->elsesp(), If)) ਵਾਪਸ;  // Short circuit
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> truesp;
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> falsesp;
        ਮੇਲ_If_Cond_Cond_Recurse(nodep, truesp /*ref*/, falsesp /*ref*/);
    }
    ਖਾਲੀ ਮੇਲ_If_Cond_Cond_Recurse(ਅਚਲ Ast_ਗੰਢ_If* nodep, std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>& truesp,
                                std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        ਜੇ (truesp.ਮਾਪ() > 4 || falsesp.ਮਾਪ() > 4) ਵਾਪਸ;
        ਜੇ (!nodep->condp()->is_ਸ਼ੁੱਧ()) ਵਾਪਸ;
        // A constant condition is folded by visit(AstNodeIf), once statements can be deleted
        ਜੇ (VN_IS(nodep->condp(), Const)) ਵਾਪਸ;
        ਤਾਰਕਿਕ replaced = ਝੂਠ;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp : truesp) {
            ਜੇ (replaced) ਰੋਕੋ;
            ਜੇ (!operandsSame(nodep->condp(), condp)) ਜਾਰੀ;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_ਅੰਕ(nodep->condp(), 1);
            replaced = ਸੱਚ;
            ++m_stat_If_Cond_ਵਿਅੰਜਕ_Redundant;
        }
        ਲਈ (ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp : falsesp) {
            ਜੇ (replaced) ਰੋਕੋ;
            ਜੇ (!operandsSame(nodep->condp(), condp)) ਜਾਰੀ;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = ਸੱਚ;
            ++m_stat_If_Cond_ਵਿਅੰਜਕ_Redundant;
        }
        // We only check the first statement of parent IF is an If
        // So we don't need to check for effects in the executing thensp/elsesp
        // altering the child't condition.  e.g. 'if (x) begin x=1; if (x) end'
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_If* subIfp = VN_CAST(nodep->thensp(), ਗੰਢ_If)) {
            ਜੇ (!replaced) truesp.emplace_ਪਿੱਠ(nodep->condp());
            ਮੇਲ_If_Cond_Cond_Recurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            ਜੇ (!replaced) truesp.ਕੱਢੋ_ਪਿੱਠ();
        }
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_If* subIfp = VN_CAST(nodep->elsesp(), ਗੰਢ_If)) {
            ਜੇ (!replaced) falsesp.emplace_ਪਿੱਠ(nodep->condp());
            ਮੇਲ_If_Cond_Cond_Recurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            ਜੇ (!replaced) falsesp.ਕੱਢੋ_ਪਿੱਠ();
        }
    }

    ਖਾਲੀ ਮੇਲ_If_Same_ਆਖਰੀ(Ast_ਗੰਢ_If* nodep) {
        ਜੇ (!nodep->thensp()) ਵਾਪਸ;
        ਜੇ (!nodep->elsesp()) ਵਾਪਸ;
        ਜੇ (!canDeleteStatements()) ਵਾਪਸ;
        // Iterate bodies in reverse
        Ast_ਗੰਢ* tp = nodep->thensp()->lastp();
        Ast_ਗੰਢ* ep = nodep->elsesp()->lastp();
        ਜਦੋਂ (tp && ep && tp->same_ਰੁੱਖ(ep)) {
            UASSERT_OBJ(!tp->nextp(), tp, "Expected no nextp");
            UASSERT_OBJ(!ep->nextp(), ep, "Expected no nextp");
            Ast_ਗੰਢ* ਅਚਲ tPrevp = tp->prevp();
            Ast_ਗੰਢ* ਅਚਲ ePrevp = ep->prevp();
            nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(tp->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(ep->unlink_Fr_ਪਿੱਠ()), ep);
            tp = tPrevp;
            ep = ePrevp;
        }
    }

    ਤਾਰਕਿਕ ਮੇਲ_Masked_Or(AstAnd* nodep) {
        // Masking an OR with terms that have no bits set under the mask is replaced with masking
        // only the remaining terms. Canonical example as generated by V3Expand is:
        // 0xff & (a << 8 | b >> 24) --> 0xff & (b >> 24)

        // Compute how many significant bits are in the mask
        ਅਚਲ AstConst* ਅਚਲ constp = VN_AS(nodep->lhsp(), Const);
        ਅਚਲ uint32_t significant_ਬਿੱਟ = constp->ਅੰਕ().ਚੌੜਾਈ_ਮੰਜ਼ਿਲ_Fit();

        AstOr* ਅਚਲ orp = VN_AS(nodep->rhsp(), Or);

        // Predicate for checking whether the bottom 'significantBits' bits of the given expression
        // are all zeroes.
        ਅਚਲ ਸਵੈਚਾਲਿਤ checkBottomClear = [=](ਅਚਲ Ast_ਗੰਢ* nodep) -> ਤਾਰਕਿਕ {
            ਜੇ (ਅਚਲ Ast_ਖਿਸਕਾਓ_L* ਅਚਲ shiftp = VN_CAST(nodep, ਖਿਸਕਾਓ_L)) {
                ਜੇ (ਅਚਲ AstConst* ਅਚਲ scp = VN_CAST(shiftp->rhsp(), Const)) {
                    ਵਾਪਸ scp->ਅੰਕ().ਮੰਜ਼ਿਲ_UInt() >= significant_ਬਿੱਟ;
                }
            }
            ਵਾਪਸ ਝੂਠ;
        };

        ਅਚਲ ਤਾਰਕਿਕ orLIsRedundant = checkBottomClear(orp->lhsp());
        ਅਚਲ ਤਾਰਕਿਕ orRIsRedundant = checkBottomClear(orp->rhsp());

        ਜੇ (orLIsRedundant && orRIsRedundant) {
            nodep->replaceWithKeepDType(
                ਨਵਾਂ AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (orLIsRedundant) {
            orp->replaceWithKeepDType(orp->rhsp()->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(orp), orp);
            ਵਾਪਸ ਝੂਠ;  // input node is still valid, keep going
        } ਨਹੀਂਤਾਂ ਜੇ (orRIsRedundant) {
            orp->replaceWithKeepDType(orp->lhsp()->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(orp), orp);
            ਵਾਪਸ ਝੂਠ;  // input node is still valid, keep going
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ ਝੂਠ;
        }
    }
    ਤਾਰਕਿਕ ਮੇਲ_Masked_ਖਿਸਕਾਓ(AstAnd* nodep) {
        // Drop redundant masking of right shift result. E.g: 0xff & ((uint32_t)a >> 24). This
        // commonly appears after V3Expand and the simplification in matchMaskedOr. Similarly,
        // drop redundant masking of left shift result. E.g.: 0xff000000 & ((uint32_t)a << 24).

        ਅਚਲ ਸਵੈਚਾਲਿਤ check_ਨਕਾਬ = [nodep, ਇਹ](ਅਚਲ V3_ਸੰਖਿਆ& ਨਕਾਬ) -> ਤਾਰਕਿਕ {
            ਅਚਲ AstConst* ਅਚਲ constp = VN_AS(nodep->lhsp(), Const);
            ਜੇ (constp->ਅੰਕ().isCaseEq(ਨਕਾਬ)) {
                Ast_ਗੰਢ* ਅਚਲ rhsp = nodep->rhsp();
                rhsp->unlink_Fr_ਪਿੱਠ();
                nodep->replaceWithKeepDType(rhsp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ ਸੱਚ;
            }
            ਵਾਪਸ ਝੂਠ;
        };

        // Check if masking is redundant
        ਜੇ (ਅਚਲ Ast_ਖਿਸਕਾਓ_R* ਅਚਲ shiftp = VN_CAST(nodep->rhsp(), ਖਿਸਕਾਓ_R)) {
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_ਸੰਖਿਆ ਨਕਾਬ_Lo{nodep, nodep->ਚੌੜਾਈ()};
                ਨਕਾਬ_Lo.ਸੈੱਟ_ਨਕਾਬ(nodep->ਚੌੜਾਈ() - scp->ਅੰਕ().ਮੰਜ਼ਿਲ_UInt());
                ਵਾਪਸ check_ਨਕਾਬ(ਨਕਾਬ_Lo);
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਖਿਸਕਾਓ_L* ਅਚਲ shiftp = VN_CAST(nodep->rhsp(), ਖਿਸਕਾਓ_L)) {
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_ਸੰਖਿਆ ਨਕਾਬ{nodep, nodep->ਚੌੜਾਈ()};
                ਅਚਲ uint32_t ਖਿਸਕਾਓ_Amount = scp->ਅੰਕ().ਮੰਜ਼ਿਲ_UInt();
                ਨਕਾਬ.ਸੈੱਟ_ਨਕਾਬ(nodep->ਚੌੜਾਈ() - ਖਿਸਕਾਓ_Amount, ਖਿਸਕਾਓ_Amount);
                ਵਾਪਸ check_ਨਕਾਬ(ਨਕਾਬ);
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }

    ਤਾਰਕਿਕ ਮੇਲ_Masked_Zero(ਅਚਲ AstAnd* nodep) {
        // Turn masking of known zero bits into constant zero. Commonly appears after V3Expand.
        ਅਚਲ AstConst* ਅਚਲ maskp = VN_AS(nodep->lhsp(), Const);
        ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp();
        ਅਚਲ uint32_t msbP1 = maskp->ਅੰਕ().most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1();
        ਜੇ (!msbP1) ਵਾਪਸ ਝੂਠ;  // Don't rewrite, separate rule matches for this
        ਅਚਲ uint32_t msb = msbP1 - 1;
        ਅਚਲ uint32_t lsb = maskp->ਅੰਕ().ਘੱਟ_ਤੋਂ_ਘੱਟ_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() - 1;

        ਜੇ (ਅਚਲ Ast_ਖਿਸਕਾਓ_L* ਅਚਲ shiftp = VN_CAST(rhsp, ਖਿਸਕਾਓ_L)) {
            // 'a << S' forces the low S bits to zero
            ਜੇ (AstConst* ਅਚਲ scp = VN_CAST(shiftp->rhsp(), Const)) {
                ਵਾਪਸ scp->ਅੰਕ().ਸਮਾਉਂਦਾ_ਵਿੱਚ_UInt()  //
                       && (scp->ਅੰਕ().ਮੰਜ਼ਿਲ_UInt() > msb);
            }
        }
        ਜੇ (ਅਚਲ Ast_ਖਿਸਕਾਓ_R* ਅਚਲ shiftp = VN_CAST(rhsp, ਖਿਸਕਾਓ_R)) {
            // 'a >> S' forces the high S bits to zero. Check against the width of the shifted
            // operand, V3Expand can create shifts wider than their inputs. Must use width(),
            // not widthMin(), as bits above widthMin() are not guaranteed to be zero.
            ਜੇ (AstConst* ਅਚਲ scp = VN_CAST(shiftp->rhsp(), Const)) {
                ਵਾਪਸ scp->ਅੰਕ().ਸਮਾਉਂਦਾ_ਵਿੱਚ_UInt()
                       && (lsb + scp->ਅੰਕ().ਮੰਜ਼ਿਲ_UInt()
                           >= static_cast<uint32_t>(shiftp->lhsp()->ਚੌੜਾਈ()));
            }
        }
        ਜੇ (ਅਚਲ AstMul* ਅਚਲ mulp = VN_CAST(rhsp, Mul)) {
            // 'C * a' forces the low N bits to zero where 'C' has low zero bits
            ਜੇ (AstConst* ਅਚਲ cp = VN_CAST(mulp->lhsp(), Const)) {
                ਵਾਪਸ cp->ਅੰਕ().ਘੱਟ_ਤੋਂ_ਘੱਟ_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() > msb + 1;
            }
        }
        ਜੇ (ਅਚਲ AstExtend* ਅਚਲ extendp = VN_CAST(rhsp, Extend)) {
            // Zero-extension forces the bits above the source width to zero
            ਵਾਪਸ lsb >= static_cast<uint32_t>(extendp->lhsp()->ਚੌੜਾਈ());
        }

        ਵਾਪਸ ਝੂਠ;
    }

    ਤਾਰਕਿਕ ਮੇਲ_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (nodep->ਚੌੜਾਈ_Min() != 1) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!v3Global.opt.f_Const_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ()) ਵਾਪਸ ਝੂਠ;

        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = nodep->ਚੌੜਾਈ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* rootp = nodep;
        ਬਿਨਾਨਿਸ਼ਾਨੀ externalOps = 0;
        // Reach past a plain making AND
        ਜੇ (ਅਚਲ AstAnd* ਅਚਲ andp = VN_CAST(nodep, And)) {
            ਜੇ (isConst(andp->lhsp(), 1)) {
                rootp = andp->rhsp();
                externalOps = 1;
            }
        }

        // Only optimize if rootp is in fact the root of a tree of identical
        // operations, otherwise analysis on an eventually unoptimizable unablanced
        // tree can go O(N^2) as we would re-analyze every time as we move up the
        // tree, repeatedly re-discover the sub-tree is not optimizable.
        ਜੇ (rootp->type() == nodep->backp()->type()) ਵਾਪਸ ਝੂਠ;

        std::string debug_ਅਗੇਤਰ;
        ਜੇ (debug() >= 9) {  // LCOV_EXCL_START
            ਸਥਿਰ ਪੂਰਨ s_c = 0;
            debug_ਅਗੇਤਰ = "-  matchBitOpTree[" + std::ਮੰਜ਼ਿਲ_string(++s_c) + "] ";
            nodep->dump_ਰੁੱਖ(debug_ਅਗੇਤਰ + "INPUT: ");
        }  // LCOV_EXCL_STOP

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = Const_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ_Visitor::simplify(rootp, ਚੌੜਾਈ, externalOps,
                                                                  m_stat_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_Reduction, m_ids);

        ਜੇ (newp) {
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Transformed leaf of bit tree to " << newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }

        ਜੇ (debug() >= 9) {  // LCOV_EXCL_START
            ਜੇ (newp) {
                newp->dump_ਰੁੱਖ(debug_ਅਗੇਤਰ + "RESULT: ");
            } ਨਹੀਂਤਾਂ {
                ਬਾਹਰ << debug_ਅਗੇਤਰ << "not replaced\n";
            }
        }  // LCOV_EXCL_STOP

        ਵਾਪਸ newp;
    }
    ਸਥਿਰ ਤਾਰਕਿਕ ਸੰਕਾਰਜ_ਖਿਸਕਾਓ_Same(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ np = VN_AS(nodep, ਗੰਢ_Biop);
        {
            ਅਚਲ Ast_ਖਿਸਕਾਓ_L* ਅਚਲ lp = VN_CAST(np->lhsp(), ਖਿਸਕਾਓ_L);
            ਅਚਲ Ast_ਖਿਸਕਾਓ_L* ਅਚਲ rp = VN_CAST(np->rhsp(), ਖਿਸਕਾਓ_L);
            ਜੇ (lp && rp) {
                ਵਾਪਸ (lp->ਚੌੜਾਈ() == rp->ਚੌੜਾਈ() && lp->lhsp()->ਚੌੜਾਈ() == rp->lhsp()->ਚੌੜਾਈ()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        {
            ਅਚਲ Ast_ਖਿਸਕਾਓ_R* ਅਚਲ lp = VN_CAST(np->lhsp(), ਖਿਸਕਾਓ_R);
            ਅਚਲ Ast_ਖਿਸਕਾਓ_R* ਅਚਲ rp = VN_CAST(np->rhsp(), ਖਿਸਕਾਓ_R);
            ਜੇ (lp && rp) {
                ਵਾਪਸ (lp->ਚੌੜਾਈ() == rp->ਚੌੜਾਈ() && lp->lhsp()->ਚੌੜਾਈ() == rp->lhsp()->ਚੌੜਾਈ()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Huge_ਖਿਸਕਾਓ_L(ਅਚਲ Ast_ਗੰਢ_Biop* nodep) {
        ਵਾਪਸ (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->ਅੰਕ().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->ਅੰਕ().ਸਮਾਉਂਦਾ_ਵਿੱਚ_UInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->ਮੰਜ਼ਿਲ_UInt()
                        >= static_cast<uint32_t>(nodep->ਚੌੜਾਈ())))
                && nodep->lhsp()->is_ਸ਼ੁੱਧ());
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Huge_ਖਿਸਕਾਓ_R(ਅਚਲ Ast_ਗੰਢ_Biop* nodep) {
        ਵਾਪਸ (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->ਅੰਕ().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->ਅੰਕ().ਸਮਾਉਂਦਾ_ਵਿੱਚ_UInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->ਮੰਜ਼ਿਲ_UInt()
                        >= static_cast<uint32_t>(nodep->lhsp()->ਚੌੜਾਈ())))
                && nodep->lhsp()->is_ਸ਼ੁੱਧ()
                && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), ਧਾਰਾ_DType)));
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Is_Two(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep, Const);
        ਜੇ (!constp) ਵਾਪਸ ਝੂਠ;  // not constant
        ਜੇ (constp->ਅੰਕ().isFourState()) ਵਾਪਸ ਝੂਠ;  // four-state
        ਜੇ (nodep->ਚੌੜਾਈ() > VL_QUADSIZE) ਵਾਪਸ ਝੂਠ;  // too wide
        ਜੇ (nodep->isSigned() && constp->ਅੰਕ().isNegative()) ਵਾਪਸ ਝੂਠ;  // signed and negative
        ਵਾਪਸ constp->ਮੰਜ਼ਿਲ_UQuad() == 2;
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Is_Twostate(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਵਾਪਸ (VN_IS(nodep, Const) && !VN_AS(nodep, Const)->ਅੰਕ().isFourState());
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Is_Pow_Two(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਜੇ (!ਸੰਕਾਰਜ_Is_Twostate(nodep)) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ (1 == VN_AS(nodep, Const)->ਅੰਕ().ਗਿਣਤੀ_Ones());
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_ਖਿਸਕਾਓ_Op(ਅਚਲ Ast_ਗੰਢ_Biop* nodep) {
        ਜੇ (!VN_IS(nodep->rhsp(), Const)) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ lhsp = VN_CAST(nodep->lhsp(), ਗੰਢ_Biop);
        ਜੇ (!lhsp || !(VN_IS(lhsp, And) || VN_IS(lhsp, Or) || VN_IS(lhsp, Xor))) ਵਾਪਸ ਝੂਠ;
        ਜੇ (nodep->ਚੌੜਾਈ() != lhsp->ਚੌੜਾਈ()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (nodep->ਚੌੜਾਈ() != lhsp->lhsp()->ਚੌੜਾਈ()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (nodep->ਚੌੜਾਈ() != lhsp->rhsp()->ਚੌੜਾਈ()) ਵਾਪਸ ਝੂਠ;

        // Only do it if we can fruther reduce one of the sides if pulling the shift through:
        // - Either operands are constants
        ਜੇ (VN_IS(lhsp->lhsp(), Const)) ਵਾਪਸ ਸੱਚ;
        ਜੇ (VN_IS(lhsp->rhsp(), Const)) ਵਾਪਸ ਸੱਚ;
        // - Const shift on either side
        ਜੇ (Ast_ਗੰਢ_Biop* ਅਚਲ llp = VN_CAST(lhsp->lhsp(), ਗੰਢ_Biop)) {
            ਵਾਪਸ (VN_IS(llp, ਖਿਸਕਾਓ_L) || VN_IS(llp, ਖਿਸਕਾਓ_R)) && VN_IS(llp->rhsp(), Const);
        }
        ਜੇ (Ast_ਗੰਢ_Biop* ਅਚਲ lrp = VN_CAST(lhsp->rhsp(), ਗੰਢ_Biop)) {
            ਵਾਪਸ (VN_IS(lrp, ਖਿਸਕਾਓ_L) || VN_IS(lrp, ਖਿਸਕਾਓ_R)) && VN_IS(lrp->rhsp(), Const);
        }
        // Otherwise we would increase logic size
        ਵਾਪਸ ਝੂਠ;
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_ਖਿਸਕਾਓ_ਖਿਸਕਾਓ(ਅਚਲ Ast_ਗੰਢ_Biop* nodep) {
        // We could add a AND though.
        ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ lhsp = VN_CAST(nodep->lhsp(), ਗੰਢ_Biop);
        ਜੇ (!lhsp || !(VN_IS(lhsp, ਖਿਸਕਾਓ_L) || VN_IS(lhsp, ਖਿਸਕਾਓ_R))) ਵਾਪਸ ਝੂਠ;
        // We can only get rid of a<<b>>c or a<<b<<c, with constant b & c
        // because bits may be masked in that process, or (b+c) may exceed the word width.
        ਜੇ (!(VN_IS(nodep->rhsp(), Const) && VN_IS(lhsp->rhsp(), Const))) ਵਾਪਸ ਝੂਠ;
        ਜੇ (VN_AS(nodep->rhsp(), Const)->ਅੰਕ().isFourState()
            || VN_AS(lhsp->rhsp(), Const)->ਅੰਕ().isFourState())
            ਵਾਪਸ ਝੂਠ;
        ਜੇ (nodep->ਚੌੜਾਈ() != lhsp->ਚੌੜਾਈ()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (nodep->ਚੌੜਾਈ() != lhsp->lhsp()->ਚੌੜਾਈ()) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_ਸ਼ਬਦ_OOB(ਅਚਲ Ast_ਸ਼ਬਦ_Sel* nodep) {
        // V3Expand may make a arraysel that exceeds the bounds of the array
        // It was an expression, then got constified.  In reality, the WordSel
        // must be wrapped in a Cond, that will be false.
        ਵਾਪਸ (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ਗੰਢ_Var_Ref)
                && VN_AS(nodep->fromp(), ਗੰਢ_Var_Ref)->access().isReadOnly()
                && (static_cast<ਪੂਰਨ>(VN_AS(nodep->bitp(), Const)->ਮੰਜ਼ਿਲ_UInt())
                    >= VN_AS(nodep->fromp(), ਗੰਢ_Var_Ref)->varp()->ਚੌੜਾਈ_ਸ਼ਬਦ()));
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Sel_ਭਰਿਆ(ਅਚਲ AstSel* nodep) {
        ਵਾਪਸ (VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0
                && static_cast<ਪੂਰਨ>(nodep->ਚੌੜਾਈ_Const()) == nodep->fromp()->ਚੌੜਾਈ());
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Sel_Extend(AstSel* nodep) {
        ਜੇ (!m_doV) ਵਾਪਸ ਝੂਠ;
        AstExtend* ਅਚਲ extendp = VN_CAST(nodep->fromp(), Extend);
        ਜੇ (!extendp) ਵਾਪਸ ਝੂਠ;
        AstConst* ਅਚਲ lsbp = VN_CAST(nodep->lsbp(), Const);
        ਜੇ (!lsbp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = nodep->ਚੌੜਾਈ_Const();
        ਅਚਲ ਪੂਰਨ lsb = lsbp->ਮੰਜ਼ਿਲ_SInt();
        ਅਚਲ ਪੂਰਨ msb = lsb + ਚੌੜਾਈ - 1;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = extendp->lhsp();
        ਜੇ (!lhsp->is_ਸ਼ੁੱਧ()) ਵਾਪਸ ਝੂਠ;

        // Selecting the entire extended expression, replace with that
        ਜੇ (lsb == 0 && msb == lhsp->ਚੌੜਾਈ() - 1) {
            lhsp->unlink_Fr_ਪਿੱਠ();
            nodep->replaceWithKeepDType(lhsp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ ਸੱਚ;
        }
        // Select entirely in the extended part - replace with zero
        ਜੇ (lsb >= lhsp->ਚੌੜਾਈ()) {
            replaceZero(nodep);
            ਵਾਪਸ ਸੱਚ;
        }
        // Select entirely in the extended expression - replace with select from that
        ਜੇ (msb < lhsp->ਚੌੜਾਈ()) {
            lhsp->unlink_Fr_ਪਿੱਠ();
            lsbp->unlink_Fr_ਪਿੱਠ();
            nodep->replaceWithKeepDType(ਨਵਾਂ AstSel{nodep->fileline(), lhsp, lsbp, ਚੌੜਾਈ});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ ਸੱਚ;
        }
        // Select straddles both sides, but is just a shorter extend
        ਜੇ (lsb == 0) {
            lhsp->unlink_Fr_ਪਿੱਠ();
            nodep->replaceWithKeepDType(ਨਵਾਂ AstExtend{nodep->fileline(), lhsp});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ ਸੱਚ;
        }

        ਵਾਪਸ ਝੂਠ;
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Sel_Bi_ਹੇਠਲਾ(AstSel* nodep) {
        // SEL(ADD(a,b),(width-1),0) -> ADD(SEL(a),SEL(b))
        // Add or any operation which doesn't care if we discard top bits
        Ast_ਗੰਢ_Biop* ਅਚਲ bip = VN_CAST(nodep->fromp(), ਗੰਢ_Biop);
        ਜੇ (!(m_doV && bip && VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0)) ਵਾਪਸ ਝੂਠ;
        UINFOTREE(9, nodep, "", "SEL(BI)-in");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bilhsp = bip->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ birhsp = bip->rhsp()->unlink_Fr_ਪਿੱਠ();
        bip->lhsp(ਨਵਾਂ AstSel{nodep->fileline(), bilhsp, 0, nodep->ਚੌੜਾਈ_Const()});
        bip->rhsp(ਨਵਾਂ AstSel{nodep->fileline(), birhsp, 0, nodep->ਚੌੜਾਈ_Const()});
        UINFOTREE(9, bip, "", "SEL(BI)-ou");
        VL_DO_DANGLING(replaceWChild(nodep, bip), nodep);
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Sel_ਖਿਸਕਾਓ_ਹੇਠਲਾ(AstSel* nodep) {
        // AND({a}, SHIFTR({b}, {c})) is often shorthand in C for Verilog {b}[{c} :+ {a}]
        // becomes thought other optimizations
        // SEL(SHIFTR({a},{b}),{lsb},{width}) -> SEL({a},{lsb+b},{width})
        ਅਚਲ Ast_ਖਿਸਕਾਓ_R* ਅਚਲ shiftp = VN_CAST(nodep->fromp(), ਖਿਸਕਾਓ_R);
        ਜੇ (!(m_doV && shiftp && VN_IS(shiftp->rhsp(), Const) && VN_IS(nodep->lsbp(), Const))) {
            ਵਾਪਸ ਝੂਠ;
        }
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ap = shiftp->lhsp();
        ਅਚਲ AstConst* ਅਚਲ bp = VN_AS(shiftp->rhsp(), Const);
        ਅਚਲ AstConst* ਅਚਲ lp = VN_AS(nodep->lsbp(), Const);
        ਜੇ (bp->isWide() || bp->ਅੰਕ().isFourState() || bp->ਅੰਕ().isNegative() || lp->isWide()
            || lp->ਅੰਕ().isFourState() || lp->ਅੰਕ().isNegative()) {
            ਵਾਪਸ ਝੂਠ;
        }
        ਅਚਲ ਪੂਰਨ newLsb = lp->ਮੰਜ਼ਿਲ_SInt() + bp->ਮੰਜ਼ਿਲ_SInt();
        ਜੇ (newLsb + nodep->ਚੌੜਾਈ_Const() > ap->ਚੌੜਾਈ()) ਵਾਪਸ ਝੂਠ;
        //
        UINFO(9, "SEL(SHIFTR(a,b),l,w) -> SEL(a,l+b,w)");
        UINFOTREE(9, nodep, "", "SEL(SH)-in");
        AstSel* ਅਚਲ newp
            = ਨਵਾਂ AstSel{nodep->fileline(), ap->unlink_Fr_ਪਿੱਠ(), newLsb, nodep->ਚੌੜਾਈ_Const()};
        nodep->replaceWithKeepDType(newp);
        UINFOTREE(9, newp, "", "SEL(SH)-ou");
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        ਵਾਪਸ ਸੱਚ;
    }

    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Bi_Extend_Const_Shrink(Ast_ਗੰਢ_Biop* nodep) {
        // Loop unrolling favors standalone compares
        // EQ(const{width32}, EXTEND(xx{width3})) -> EQ(const{3}, xx{3})
        // The constant must have zero bits (+ 1 if signed) or compare
        // would be incorrect. See also operandBiExtendConst
        AstExtend* ਅਚਲ extendp = VN_CAST(nodep->rhsp(), Extend);
        ਜੇ (!extendp) ਵਾਪਸ ਝੂਠ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ smallerp = extendp->lhsp();
        ਅਚਲ ਪੂਰਨ subsize = smallerp->ਚੌੜਾਈ();
        AstConst* ਅਚਲ constp = VN_CAST(nodep->lhsp(), Const);
        ਜੇ (!constp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!constp->ਅੰਕ().is_ਬਿੱਟ_Zero(constp->ਚੌੜਾਈ() - 1, subsize)) ਵਾਪਸ ਝੂਠ;
        //
        UINFOTREE(9, nodep, "", "BI(EXTEND)-in");
        smallerp->unlink_Fr_ਪਿੱਠ();
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(extendp->unlink_Fr_ਪਿੱਠ()), extendp);  // aka nodep->lhsp.
        nodep->rhsp(smallerp);

        constp->unlink_Fr_ਪਿੱਠ();
        ਅਚਲ V3_ਸੰਖਿਆ ਅੰਕ{constp, subsize, constp->ਅੰਕ()};
        nodep->lhsp(ਨਵਾਂ AstConst{constp->fileline(), ਅੰਕ});
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(constp), constp);
        ਵਾਪਸ ਝੂਠ;  // input node is still valid, keep going
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Bi_Extend_Const_Over(ਅਚਲ Ast_ਗੰਢ_Biop* nodep) {
        // EQ(const{width32}, EXTEND(xx{width3})) -> constant
        // When the constant has non-zero bits above the extend it's a constant.
        // Avoids compiler warning
        ਅਚਲ AstExtend* ਅਚਲ extendp = VN_CAST(nodep->rhsp(), Extend);
        ਜੇ (!extendp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_ਗੰਢ* ਅਚਲ smallerp = extendp->lhsp();
        ਅਚਲ ਪੂਰਨ subsize = smallerp->ਚੌੜਾਈ();
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->lhsp(), Const);
        ਜੇ (!constp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (constp->ਅੰਕ().is_ਬਿੱਟ_Zero(constp->ਚੌੜਾਈ() - 1, subsize)) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ ਸੱਚ;
    }

    // Extraction checks
    ਤਾਰਕਿਕ warnSelect(AstSel* nodep) {
        ਜੇ (m_doGenerate) {
            // Never checked yet
            V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Params_Edit(nodep);
            iterateChildren(nodep);  // May need "constifying"
        }
        // Find range of dtype we are selecting from
        // Similar code in V3Unknown::AstSel
        ਅਚਲ ਤਾਰਕਿਕ doit = ਸੱਚ;
        ਜੇ (m_warn && VN_IS(nodep->lsbp(), Const) && doit) {
            ਅਚਲ ਪੂਰਨ max_Decl_ਦੋਆਧਾਰੀ_ਅੰਕ = nodep->declRange().hiMaxSelect() * nodep->decl_El_ਚੌੜਾਈ()
                                   + (nodep->decl_El_ਚੌੜਾਈ() - 1);
            ਜੇ (VN_AS(nodep->lsbp(), Const)->ਅੰਕ().isFourState()) {
                nodep->v3error("Selection index is constantly unknown or tristated: "
                               "lsb="
                               << nodep->lsbp()->name() << " width=" << nodep->ਚੌੜਾਈ_Const());
                // Replacing nodep will make a mess above, so we replace the offender
                replaceZero(nodep->lsbp());
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->declRange().ranged()
                       && (nodep->msbConst() > max_Decl_ਦੋਆਧਾਰੀ_ਅੰਕ || nodep->lsbConst() > max_Decl_ਦੋਆਧਾਰੀ_ਅੰਕ)) {
                // See also warning in V3Width
                // Must adjust by element width as declRange() is in number of elements
                string msbLsbProtected;
                ਜੇ (nodep->decl_El_ਚੌੜਾਈ() == 0) {
                    msbLsbProtected = "(nodep->declElWidth() == 0) "
                                      + std::ਮੰਜ਼ਿਲ_string(nodep->msbConst()) + ":"
                                      + std::ਮੰਜ਼ਿਲ_string(nodep->lsbConst());
                } ਨਹੀਂਤਾਂ {
                    msbLsbProtected = std::ਮੰਜ਼ਿਲ_string(nodep->msbConst() / nodep->decl_El_ਚੌੜਾਈ())
                                      + ":"
                                      + std::ਮੰਜ਼ਿਲ_string(nodep->lsbConst() / nodep->decl_El_ਚੌੜਾਈ());
                }
                nodep->v3warn(SELRANGE,
                              "Selection index out of range: "
                                  << msbLsbProtected << " outside "
                                  << nodep->declRange().hiMaxSelect() << ":0"
                                  << (nodep->declRange().lo() >= 0
                                          ? ""
                                          : (" (adjusted +" + cvt_ਮੰਜ਼ਿਲ_Str(-nodep->declRange().lo())
                                             + " to account for negative lsb)")));
                UINFO(1, "    Related Raw index is " << nodep->msbConst() << ":"
                                                     << nodep->lsbConst());
                // Don't replace with zero, we'll do it later
            }
        }
        ਵਾਪਸ ਝੂਠ;  // Not a transform, so NOP
    }

    ਸਥਿਰ ਤਾਰਕਿਕ operandsSame(ਅਚਲ Ast_ਗੰਢ* node1p, ਅਚਲ Ast_ਗੰਢ* node2p) {
        // For now we just detect constants & simple vars, though it could be more generic
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ const1p = VN_CAST(node1p, Const)) {
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ const2p = VN_CAST(node2p, Const)) {
                ਵਾਪਸ V3_ਸੰਖਿਆ{node1p->fileline(), 1, 0}
                    .opEq(const1p->ਅੰਕ(), const2p->ਅੰਕ())
                    .isEqOne();
            }
        }
        ਜੇ (VN_IS(node1p, VarRef) && VN_IS(node2p, VarRef)) {
            // Avoid comparing widthMin's, which results in lost optimization attempts
            // If cleanup sameGateTree to be smarter, this can be restored.
            // return node1p->sameGateTree(node2p);
            ਵਾਪਸ node1p->isSame(node2p);
        }
        // Pattern created by coverage-line/branch; avoid compiler tautological-compare warning
        ਜੇ (ਅਚਲ AstAnd* ਅਚਲ and1p = VN_CAST(node1p, And)) {
            ਜੇ (ਅਚਲ AstAnd* ਅਚਲ and2p = VN_CAST(node2p, And)) {
                ਜੇ (VN_IS(and1p->lhsp(), Const) && VN_IS(and1p->rhsp(), ਗੰਢ_Var_Ref)
                    && VN_IS(and2p->lhsp(), Const) && VN_IS(and2p->rhsp(), ਗੰਢ_Var_Ref))
                    ਵਾਪਸ node1p->same_Gate_ਰੁੱਖ(node2p);
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }
    ਤਾਰਕਿਕ if_Same_ਸੌਂਪੋ(ਅਚਲ Ast_ਗੰਢ_If* nodep) {
        ਅਚਲ Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ thensp = VN_CAST(nodep->thensp(), ਗੰਢ_ਸੌਂਪੋ);
        ਅਚਲ Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ elsesp = VN_CAST(nodep->elsesp(), ਗੰਢ_ਸੌਂਪੋ);
        ਜੇ (!thensp || thensp->nextp()) ਵਾਪਸ ਝੂਠ;  // Must be SINGLE statement
        ਜੇ (!elsesp || elsesp->nextp()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (thensp->type() != elsesp->type()) ਵਾਪਸ ਝੂਠ;  // Can't mix an assigndly with assign
        ਜੇ (!thensp->lhsp()->same_Gate_ਰੁੱਖ(elsesp->lhsp())) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!thensp->rhsp()->gate_ਰੁੱਖ()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!elsesp->rhsp()->gate_ਰੁੱਖ()) ਵਾਪਸ ਝੂਠ;
        // Must not create an expression with unpacked array type
        ਜੇ (VN_IS(thensp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (VN_IS(elsesp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (m_underRecFunc) ਵਾਪਸ ਝੂਠ;  // This optimization may lead to infinite recursion
        ਜੇ (m_doCpp) {  // Do not combine ifs cretaed by V3Premit
            ਜੇ (VN_IS(thensp->lhsp(), VarRef) && thensp->lhsp()->isWide()) ਵਾਪਸ ਝੂਠ;
        }
        // Only do it if not calls and both pure, otherwise undoes V3LiftExpr
        ਵਾਪਸ !VN_IS(thensp->rhsp(), ਗੰਢ_FTask_Ref)  //
               && !VN_IS(elsesp->rhsp(), ਗੰਢ_FTask_Ref)  //
               && thensp->rhsp()->is_ਸ਼ੁੱਧ()  //
               && elsesp->rhsp()->is_ਸ਼ੁੱਧ();
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_If_If(ਅਚਲ Ast_ਗੰਢ_If* nodep) {
        ਜੇ (nodep->elsesp()) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_ਗੰਢ_If* ਅਚਲ ਹੇਠਲਾ_Ifp = VN_CAST(nodep->thensp(), ਗੰਢ_If);
        ਜੇ (!ਹੇਠਲਾ_Ifp || ਹੇਠਲਾ_Ifp->nextp()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (nodep->type() != ਹੇਠਲਾ_Ifp->type()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (Ast_ਗੰਢ::afterCommentp(ਹੇਠਲਾ_Ifp->elsesp())) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ ifConcatMergeableBiop(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਵਾਪਸ (VN_IS(nodep, And) || VN_IS(nodep, Or) || VN_IS(nodep, Xor));
    }
    ਤਾਰਕਿਕ ifAdjacentSel(ਅਚਲ AstSel* lhsp, ਅਚਲ AstSel* rhsp) {
        ਜੇ (!v3Global.opt.fAssemble()) ਵਾਪਸ ਝੂਠ;  // opt disabled
        ਜੇ (!lhsp || !rhsp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_ਗੰਢ* ਅਚਲ lfromp = lhsp->fromp();
        ਅਚਲ Ast_ਗੰਢ* ਅਚਲ rfromp = rhsp->fromp();
        ਜੇ (!lfromp || !rfromp || !lfromp->same_Gate_ਰੁੱਖ(rfromp)) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstConst* ਅਚਲ lstart = VN_CAST(lhsp->lsbp(), Const);
        ਅਚਲ AstConst* ਅਚਲ rstart = VN_CAST(rhsp->lsbp(), Const);
        ਜੇ (!lstart || !rstart) ਵਾਪਸ ਝੂਠ;  // too complicated
        ਅਚਲ ਪੂਰਨ rend = (rstart->ਮੰਜ਼ਿਲ_SInt() + rhsp->ਚੌੜਾਈ_Const());
        ਵਾਪਸ (rend == lstart->ਮੰਜ਼ਿਲ_SInt());
    }
    ਤਾਰਕਿਕ if_ਮਿਲਾਓ_Adjacent(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) {
        // Called by concatmergeable to determine if {lhsp, rhsp} make sense
        ਜੇ (!v3Global.opt.fAssemble()) ਵਾਪਸ ਝੂਠ;  // opt disabled
        // Two same varref
        ਜੇ (operandsSame(lhsp, rhsp)) ਵਾਪਸ ਸੱਚ;
        ਅਚਲ AstSel* ਅਚਲ lselp = VN_CAST(lhsp, Sel);
        ਅਚਲ AstSel* ਅਚਲ rselp = VN_CAST(rhsp, Sel);
        ਜੇ (!lselp && !rselp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (lselp && !VN_IS(lselp->lsbp(), Const)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (rselp && !VN_IS(rselp->lsbp(), Const)) ਵਾਪਸ ਝੂਠ;
        // a[i:j] {a[j-1:k], b}
        ਜੇ (lselp && !rselp && VN_IS(rhsp, Concat))
            ਵਾਪਸ if_ਮਿਲਾਓ_Adjacent(lhsp, VN_AS(rhsp, Concat)->lhsp());
        // {b, a[j:k]} a[k-1:i]
        ਜੇ (rselp && !lselp && VN_IS(lhsp, Concat))
            ਵਾਪਸ if_ਮਿਲਾਓ_Adjacent(VN_AS(lhsp, Concat)->rhsp(), rhsp);
        // a a[msb:j]
        ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lfromp = lselp                                ? lselp->fromp()
                                          : lhsp->same_Gate_ਰੁੱਖ(rselp->fromp()) ? lhsp
                                                                               : ਖਾਲੀ_ਸੂਚਕ;
        // a[i:0] a
        ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rfromp = rselp                                ? rselp->fromp()
                                          : rhsp->same_Gate_ਰੁੱਖ(lselp->fromp()) ? rhsp
                                                                               : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!lfromp || !rfromp || !lfromp->same_Gate_ਰੁੱਖ(rfromp)) ਵਾਪਸ ਝੂਠ;

        ਅਚਲ int32_t lstart = lselp ? lselp->lsbConst() : 0;
        ਅਚਲ int32_t rstart = rselp ? rselp->lsbConst() : 0;
        ਅਚਲ int32_t rend = rstart + (rselp ? rselp->ਚੌੜਾਈ_Const() : rhsp->ਚੌੜਾਈ());
        // a[i:j] a[j-1:k]
        ਜੇ (rend == lstart) ਵਾਪਸ ਸੱਚ;
        // a[i:0] a[msb:j]
        ਜੇ (rend == rfromp->ਚੌੜਾਈ() && lstart == 0) ਵਾਪਸ ਸੱਚ;
        ਵਾਪਸ ਝੂਠ;
    }
    ਤਾਰਕਿਕ concatMergeable(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, ਬਿਨਾਨਿਸ਼ਾਨੀ ਡੂੰਘਾਈ) {
        // Determine if {a OP b, c OP d} => {a, c} OP {b, d} is advantageous
        ਜੇ (!v3Global.opt.fAssemble()) ਵਾਪਸ ਝੂਠ;  // opt disabled
        ਜੇ (lhsp->type() != rhsp->type()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!ifConcatMergeableBiop(lhsp)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (ਡੂੰਘਾਈ > CONCAT_MERGABLE_MAX_ਡੂੰਘਾਈ) ਵਾਪਸ ਝੂਠ;  // As worse case O(n^2) algorithm

        ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ lp = VN_CAST(lhsp, ਗੰਢ_Biop);
        ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ rp = VN_CAST(rhsp, ਗੰਢ_Biop);
        ਜੇ (!lp || !rp) ਵਾਪਸ ਝੂਠ;
        // {a[]&b[], a[]&b[]}
        ਅਚਲ ਤਾਰਕਿਕ lad = if_ਮਿਲਾਓ_Adjacent(lp->lhsp(), rp->lhsp());
        ਅਚਲ ਤਾਰਕਿਕ rad = if_ਮਿਲਾਓ_Adjacent(lp->rhsp(), rp->rhsp());
        ਜੇ (lad && rad) ਵਾਪਸ ਸੱਚ;
        // {a[] & b[]&c[], a[] & b[]&c[]}
        ਜੇ (lad && concatMergeable(lp->rhsp(), rp->rhsp(), ਡੂੰਘਾਈ + 1)) ਵਾਪਸ ਸੱਚ;
        // {a[]&b[] & c[], a[]&b[] & c[]}
        ਜੇ (rad && concatMergeable(lp->lhsp(), rp->lhsp(), ਡੂੰਘਾਈ + 1)) ਵਾਪਸ ਸੱਚ;
        // {(a[]&b[])&(c[]&d[]), (a[]&b[])&(c[]&d[])}
        ਜੇ (concatMergeable(lp->lhsp(), rp->lhsp(), ਡੂੰਘਾਈ + 1)
            && concatMergeable(lp->rhsp(), rp->rhsp(), ਡੂੰਘਾਈ + 1)) {
            ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }
    ਸਥਿਰ ਤਾਰਕਿਕ operands_Same_ਚੌੜਾਈ(ਅਚਲ Ast_ਗੰਢ* lhsp, ਅਚਲ Ast_ਗੰਢ* rhsp) {
        ਵਾਪਸ lhsp->ਚੌੜਾਈ() == rhsp->ਚੌੜਾਈ();
    }

    //----------------------------------------
    // Constant Replacement functions.
    // These all take a node, delete its tree, and replaces it with a constant

    ਖਾਲੀ replace_ਅੰਕ(Ast_ਗੰਢ* oldp, ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ, ਅਚਲ string& origParamName = "") {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        UASSERT_OBJ(!(VN_IS(oldp, Const) && !VN_AS(oldp, Const)->ਅੰਕ().isFourState()), oldp,
                    "Already constant??");
        AstConst* ਅਚਲ newp = ਨਵਾਂ AstConst{oldp->fileline(), ਅੰਕ};
        ਜੇ (!origParamName.ਖਾਲੀ_ਥਾਵਾਂ()) newp->origParamName(origParamName);
        oldp->replaceWithKeepDType(newp);
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(oldp), oldp);
    }
    ਖਾਲੀ replace_ਅੰਕ(Ast_ਗੰਢ* nodep, uint32_t val) {
        ਅਚਲ V3_ਸੰਖਿਆ ਅੰਕ{nodep, nodep->ਚੌੜਾਈ(), val};
        VL_DO_DANGLING(replace_ਅੰਕ(nodep, ਅੰਕ), nodep);
    }
    ਖਾਲੀ replace_ਅੰਕ_Signed(Ast_ਗੰਢ_Biop* nodep, uint32_t val) {
        // We allow both sides to be constant, as one may have come from
        // parameter propagation, etc.
        ਜੇ (m_warn && !(VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const))) {
            nodep->v3warn(UNSIGNED, "Comparison is constant due to unsigned arithmetic");
        }
        VL_DO_DANGLING(replace_ਅੰਕ(nodep, val), nodep);
    }
    ਖਾਲੀ replace_ਅੰਕ_Limited(Ast_ਗੰਢ_Biop* nodep, uint32_t val) {
        // Avoids gcc warning about same
        ਜੇ (m_warn) nodep->v3warn(CMPCONST, "Comparison is constant due to limited range");
        VL_DO_DANGLING(replace_ਅੰਕ(nodep, val), nodep);
    }
    ਖਾਲੀ replaceZero(Ast_ਗੰਢ* nodep) { VL_DO_DANGLING(replace_ਅੰਕ(nodep, 0), nodep); }
    ਖਾਲੀ replace_Zero_Chk_ਸ਼ੁੱਧ(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* checkp) {
        // For example, "0 * n" -> 0 if n has no side effects
        // Else strength reduce it to 0 & n.
        // If ever change the operation note AstAnd rule specially ignores this created pattern
        ਜੇ (checkp->is_ਸ਼ੁੱਧ()) {
            VL_DO_DANGLING(replace_ਅੰਕ(nodep, 0), nodep);
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstAnd{nodep->fileline(), ਨਵਾਂ AstConst{nodep->fileline(), 0},
                                             checkp->unlink_Fr_ਪਿੱਠ()};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }
    ਖਾਲੀ replaceAllOnes(Ast_ਗੰਢ* nodep) {
        V3_ਸੰਖਿਆ ones{nodep, nodep->ਚੌੜਾਈ(), 0};
        ones.ਸੈੱਟ_ਨਕਾਬ(nodep->ਚੌੜਾਈ());
        VL_DO_DANGLING(replace_ਅੰਕ(nodep, ones), nodep);
    }
    ਖਾਲੀ replaceConst(Ast_ਗੰਢ_Uniop* nodep) {
        V3_ਸੰਖਿਆ numv{nodep, nodep->ਚੌੜਾਈ_Min_V()};
        nodep->ਸੰਖਿਆ_ਚਲਾਓ(numv, const_ਅੰਕ_V(nodep->lhsp()));
        ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ = ਮੰਜ਼ਿਲ_ਅੰਕ_C(nodep, numv);
        UINFO(4, "UNICONST -> " << ਅੰਕ);
        VL_DO_DANGLING(replace_ਅੰਕ(nodep, ਅੰਕ), nodep);
    }
    ਖਾਲੀ replaceConst(Ast_ਗੰਢ_Biop* nodep) {
        V3_ਸੰਖਿਆ numv{nodep, nodep->ਚੌੜਾਈ_Min_V()};
        nodep->ਸੰਖਿਆ_ਚਲਾਓ(numv, const_ਅੰਕ_V(nodep->lhsp()), const_ਅੰਕ_V(nodep->rhsp()));
        ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ = ਮੰਜ਼ਿਲ_ਅੰਕ_C(nodep, numv);
        UINFO(4, "BICONST -> " << ਅੰਕ);
        VL_DO_DANGLING(replace_ਅੰਕ(nodep, ਅੰਕ), nodep);
    }
    ਖਾਲੀ replaceConst(Ast_ਗੰਢ_Triop* nodep) {
        V3_ਸੰਖਿਆ numv{nodep, nodep->ਚੌੜਾਈ_Min_V()};
        nodep->ਸੰਖਿਆ_ਚਲਾਓ(numv, const_ਅੰਕ_V(nodep->lhsp()), const_ਅੰਕ_V(nodep->rhsp()),
                             const_ਅੰਕ_V(nodep->thsp()));
        ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ = ਮੰਜ਼ਿਲ_ਅੰਕ_C(nodep, numv);
        UINFO(4, "TRICONST -> " << ਅੰਕ);
        VL_DO_DANGLING(replace_ਅੰਕ(nodep, ਅੰਕ), nodep);
    }
    ਖਾਲੀ replaceConst(Ast_ਗੰਢ_Quadop* nodep) {
        V3_ਸੰਖਿਆ numv{nodep, nodep->ਚੌੜਾਈ_Min_V()};
        nodep->ਸੰਖਿਆ_ਚਲਾਓ(numv, const_ਅੰਕ_V(nodep->lhsp()), const_ਅੰਕ_V(nodep->rhsp()),
                             const_ਅੰਕ_V(nodep->thsp()), const_ਅੰਕ_V(nodep->fhsp()));
        ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ = ਮੰਜ਼ਿਲ_ਅੰਕ_C(nodep, numv);
        UINFO(4, "QUADCONST -> " << ਅੰਕ);
        VL_DO_DANGLING(replace_ਅੰਕ(nodep, ਅੰਕ), nodep);
    }

    ਖਾਲੀ replaceConstString(Ast_ਗੰਢ* oldp, ਅਚਲ string& ਅੰਕ) {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstConst{oldp->fileline(), AstConst::String{}, ਅੰਕ};
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        oldp->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(oldp), oldp);
    }
    //----------------------------------------
    // Replacement functions.
    // These all take a node and replace it with something else

    ਖਾਲੀ replaceWChild(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* childp) {
        // NODE(..., CHILD(...)) -> CHILD(...)
        childp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
        // If replacing a SEL for example, the data type comes from the parent (is less wide).
        // This may adversely affect the operation of the node being replaced.
        nodep->replaceWithKeepDType(childp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ replaceWChildBool(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* childp) {
        // NODE(..., CHILD(...)) -> REDOR(CHILD(...))
        childp->unlink_Fr_ਪਿੱਠ();
        ਜੇ (childp->width1()) {
            nodep->replaceWithKeepDType(childp);
        } ਨਹੀਂਤਾਂ {
            nodep->replaceWithKeepDType(ਨਵਾਂ AstRedOr{childp->fileline(), childp});
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    //! Replace a ternary node with its RHS after iterating
    //! Used with short-circuiting, where the RHS has not yet been iterated.
    ਖਾਲੀ replaceWIteratedRhs(Ast_ਗੰਢ_Triop* nodep) {
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ rhsp = nodep->rhsp()) iterate_And_ਅਗਲਾ_Null(rhsp);
        replaceWChild(nodep, nodep->rhsp());  // May have changed
    }

    //! Replace a ternary node with its THS after iterating
    //! Used with short-circuiting, where the THS has not yet been iterated.
    ਖਾਲੀ replaceWIteratedThs(Ast_ਗੰਢ_Triop* nodep) {
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ thsp = nodep->thsp()) iterate_And_ਅਗਲਾ_Null(thsp);
        replaceWChild(nodep, nodep->thsp());  // May have changed
    }
    ਖਾਲੀ replaceWLhs(Ast_ਗੰਢ_Uniop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    ਖਾਲੀ replaceWLhs(Ast_ਗੰਢ_Biop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    ਖਾਲੀ replaceWRhs(Ast_ਗੰਢ_Biop* nodep) {
        // Keep RHS, remove LHS
        replaceWChild(nodep, nodep->rhsp());
    }
    ਖਾਲੀ replaceWLhsBool(Ast_ਗੰਢ_Biop* nodep) { replaceWChildBool(nodep, nodep->lhsp()); }
    ਖਾਲੀ replaceWRhsBool(Ast_ਗੰਢ_Biop* nodep) { replaceWChildBool(nodep, nodep->rhsp()); }
    ਖਾਲੀ replaceAsv(Ast_ਗੰਢ_Biop* nodep) {
        // BIASV(CONSTa, BIASV(CONSTb, c)) -> BIASV( BIASV_CONSTED(a,b), c)
        // BIASV(SAMEa,  BIASV(SAMEb, c))  -> BIASV( BIASV(SAMEa,SAMEb), c)
        // UINFOTREE(1, nodep, "", "repAsvConst_old");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ap = nodep->lhsp();
        Ast_ਗੰਢ_Biop* ਅਚਲ rp = VN_AS(nodep->rhsp(), ਗੰਢ_Biop);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bp = rp->lhsp();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cp = rp->rhsp();
        ap->unlink_Fr_ਪਿੱਠ();
        bp->unlink_Fr_ਪਿੱਠ();
        cp->unlink_Fr_ਪਿੱਠ();
        rp->unlink_Fr_ਪਿੱਠ();
        nodep->lhsp(rp);
        nodep->rhsp(cp);
        rp->lhsp(ap);
        rp->rhsp(bp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        ਜੇ (VN_IS(rp->lhsp(), Const) && VN_IS(rp->rhsp(), Const)) replaceConst(rp);
        iterate(nodep);  // Proceed to fixed point
    }
    ਖਾਲੀ replaceAsvLUp(Ast_ਗੰਢ_Biop* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r))
        Ast_ਗੰਢ_Biop* ਅਚਲ lp = VN_AS(nodep->lhsp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_Biop);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ llp = lp->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lrp = lp->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        nodep->lhsp(llp);
        nodep->rhsp(lp);
        lp->lhsp(lrp);
        lp->rhsp(rp);
        lp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    ਖਾਲੀ replaceAsvRUp(Ast_ਗੰਢ_Biop* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr))
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_Biop* ਅਚਲ rp = VN_AS(nodep->rhsp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_Biop);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rlp = rp->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rrp = rp->rhsp()->unlink_Fr_ਪਿੱਠ();
        nodep->lhsp(rlp);
        nodep->rhsp(rp);
        rp->lhsp(lp);
        rp->rhsp(rrp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    ਖਾਲੀ replaceAndOr(Ast_ਗੰਢ_Biop* nodep) {
        //  OR  (AND (CONSTll,lr), AND(CONSTrl==ll,rr))    -> AND (CONSTll, OR(lr,rr))
        //  OR  (AND (CONSTll,lr), AND(CONSTrl,    rr=lr)) -> AND (OR(ll,rl), rr)
        // nodep ^lp  ^llp   ^lrp  ^rp  ^rlp       ^rrp
        // (Or/And may also be reversed)
        Ast_ਗੰਢ_Biop* ਅਚਲ lp = VN_AS(nodep->lhsp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_Biop);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ llp = lp->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lrp = lp->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_Biop* ਅਚਲ rp = VN_AS(nodep->rhsp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_Biop);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rlp = rp->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rrp = rp->rhsp()->unlink_Fr_ਪਿੱਠ();
        nodep->replaceWithKeepDType(lp);
        ਜੇ (operandsSame(llp, rlp)) {
            lp->lhsp(llp);
            lp->rhsp(nodep);
            lp->dtypeFrom(nodep);
            nodep->lhsp(lrp);
            nodep->rhsp(rrp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(rp), rp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(rlp), rlp);
        } ਨਹੀਂਤਾਂ ਜੇ (operandsSame(lrp, rrp)) {
            lp->lhsp(nodep);
            lp->rhsp(rrp);
            lp->dtypeFrom(nodep);
            nodep->lhsp(llp);
            nodep->rhsp(rlp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(rp), rp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(lrp), lrp);
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("replaceAndOr on something operandAndOrSame shouldn't have matched");
        }
        // UINFOTREE(1, nodep, "", "repAndOr_new");
    }
    ਖਾਲੀ replace_ਖਿਸਕਾਓ_Same(Ast_ਗੰਢ_Biop* nodep) {
        // Or(Shift(ll,CONSTlr),Shift(rl,CONSTrr==lr)) -> Shift(Or(ll,rl),CONSTlr)
        // (Or/And may also be reversed)
        Ast_ਗੰਢ_Biop* ਅਚਲ lp = VN_AS(nodep->lhsp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_Biop);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ llp = lp->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lrp = lp->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_Biop* ਅਚਲ rp = VN_AS(nodep->rhsp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_Biop);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rlp = rp->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rrp = rp->rhsp()->unlink_Fr_ਪਿੱਠ();
        nodep->replaceWithKeepDType(lp);
        lp->lhsp(nodep);
        lp->rhsp(lrp);
        nodep->lhsp(llp);
        nodep->rhsp(rlp);
        nodep->dtypep(llp->dtypep());  // dtype of Biop is before shift.
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(rp), rp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(rrp), rrp);
        // UINFOTREE(1, nodep, "", "repShiftSame_new");
    }
    ਖਾਲੀ replaceConcatSel(AstConcat* nodep) {
        // {a[1], a[0]} -> a[1:0]
        AstSel* ਅਚਲ lselp = VN_AS(nodep->lhsp()->unlink_Fr_ਪਿੱਠ(), Sel);
        AstSel* ਅਚਲ rselp = VN_AS(nodep->rhsp()->unlink_Fr_ਪਿੱਠ(), Sel);
        ਅਚਲ ਪੂਰਨ lstart = lselp->lsbConst();
        ਅਚਲ ਪੂਰਨ lwidth = lselp->ਚੌੜਾਈ_Const();
        ਅਚਲ ਪੂਰਨ rstart = rselp->lsbConst();
        ਅਚਲ ਪੂਰਨ rwidth = rselp->ਚੌੜਾਈ_Const();

        UASSERT_OBJ((rstart + rwidth) == lstart, nodep,
                    "tried to merge two selects which are not adjacent");
        AstSel* ਅਚਲ newselp = ਨਵਾਂ AstSel{
            lselp->fromp()->fileline(), rselp->fromp()->unlink_Fr_ਪਿੱਠ(), rstart, lwidth + rwidth};
        UINFO(5, "merged two adjacent sel " << lselp << " and " << rselp << " to one " << newselp);

        nodep->replaceWithKeepDType(newselp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(lselp), lselp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(rselp), rselp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ replace_Concat_ਮਿਲਾਓ(AstConcat* nodep) {
        // {llp OP lrp, rlp OP rrp} => {llp, rlp} OP {lrp, rrp}, where OP = AND/OR/XOR
        Ast_ਗੰਢ_Biop* ਅਚਲ lp = VN_AS(nodep->lhsp(), ਗੰਢ_Biop);
        Ast_ਗੰਢ_Biop* ਅਚਲ rp = VN_AS(nodep->rhsp(), ਗੰਢ_Biop);
        ਜੇ (concatMergeable(lp, rp, 0)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ llp = lp->lhsp();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lrp = lp->rhsp();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rlp = rp->lhsp();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rrp = rp->rhsp();
            AstConcat* ਅਚਲ newlp = ਨਵਾਂ AstConcat{rlp->fileline(), llp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                                   rlp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
            AstConcat* ਅਚਲ newrp = ਨਵਾਂ AstConcat{rrp->fileline(), lrp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                                   rrp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
            // use the lhs to replace the parent concat
            llp->replaceWith(newlp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(llp), llp);
            lrp->replaceWith(newrp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(lrp), lrp);
            lp->dtype_Chg_ਚੌੜਾਈ_Signed(newlp->ਚੌੜਾਈ(), newlp->ਚੌੜਾਈ(), VSigning::UNSIGNED);
            UINFO(5, "merged " << nodep);
            ++m_stat_Concat_ਮਿਲਾਓ;
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(rp->unlink_Fr_ਪਿੱਠ()), rp);
            nodep->replaceWithKeepDType(lp->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            iterate(lp->lhsp());
            iterate(lp->rhsp());
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("tried to merge two Concat which are not adjacent");
        }
    }
    ਖਾਲੀ replaceExtend(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* arg0p) {
        // -> EXTEND(nodep)
        // like a AstExtend{$rhsp}, but we need to set the width correctly from base node
        arg0p->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp
            = (VN_IS(nodep, ExtendS)
                   ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstExtendS{nodep->fileline(), arg0p})
                   : static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstExtend{nodep->fileline(), arg0p}));
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ replace_Pow_ਖਿਸਕਾਓ(Ast_ਗੰਢ_Biop* nodep) {  // Pow or PowS
        UINFO(5, "POW(2,b)->SHIFTL(1,b) " << nodep);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਖਿਸਕਾਓ_L* ਅਚਲ newp
            = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{nodep->fileline(), ਨਵਾਂ AstConst{nodep->fileline(), 1}, rhsp};
        newp->lhsp()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ replace_Mul_ਖਿਸਕਾਓ(AstMul* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "MUL(2^n,b)->SHIFTL(b,n) " << nodep);
        ਅਚਲ ਪੂਰਨ amount = VN_AS(nodep->lhsp(), Const)->ਅੰਕ().most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() - 1;  // 2^n->n+1
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ opp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਖਿਸਕਾਓ_L* ਅਚਲ newp
            = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{nodep->fileline(), opp, ਨਵਾਂ AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ replace_Div_ਖਿਸਕਾਓ(AstDiv* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "DIV(b,2^n)->SHIFTR(b,n) " << nodep);
        ਅਚਲ ਪੂਰਨ amount = VN_AS(nodep->rhsp(), Const)->ਅੰਕ().most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() - 1;  // 2^n->n+1
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ opp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਖਿਸਕਾਓ_R* ਅਚਲ newp
            = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_R{nodep->fileline(), opp, ਨਵਾਂ AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ replaceModAnd(AstModDiv* nodep) {  // Mod, but not ModS as not simple shift
        UINFO(5, "MOD(b,2^n)->AND(b,2^n-1) " << nodep);
        ਅਚਲ ਪੂਰਨ amount = VN_AS(nodep->rhsp(), Const)->ਅੰਕ().most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() - 1;  // 2^n->n+1
        V3_ਸੰਖਿਆ ਨਕਾਬ{nodep, nodep->ਚੌੜਾਈ()};
        ਨਕਾਬ.ਸੈੱਟ_ਨਕਾਬ(amount);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ opp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        AstAnd* ਅਚਲ newp
            = ਨਵਾਂ AstAnd{nodep->fileline(), opp, ਨਵਾਂ AstConst{nodep->fileline(), ਨਕਾਬ}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ replace_ਖਿਸਕਾਓ_Op(Ast_ਗੰਢ_Biop* nodep) {
        UINFO(5, "SHIFT(AND(a,b),CONST) with a/b special ->AND(SHIFT(a,CONST),SHIFT(b,CONST)) "
                     << nodep);
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = nodep->ਚੌੜਾਈ();
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ_Min = nodep->ਚੌੜਾਈ_Min();
        VNRelinker handle;
        nodep->unlink_Fr_ਪਿੱਠ(&handle);
        Ast_ਗੰਢ_Biop* ਅਚਲ lhsp = VN_AS(nodep->lhsp(), ਗੰਢ_Biop);
        lhsp->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ shiftp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ap = lhsp->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bp = lhsp->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_Biop* ਅਚਲ shift1p = nodep;
        Ast_ਗੰਢ_Biop* ਅਚਲ shift2p = nodep->clone_ਰੁੱਖ(ਸੱਚ);
        shift1p->lhsp(ap);
        shift1p->rhsp(shiftp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ));
        shift2p->lhsp(bp);
        shift2p->rhsp(shiftp);
        Ast_ਗੰਢ_Biop* ਅਚਲ newp = lhsp;
        newp->lhsp(shift1p);
        newp->rhsp(shift2p);
        newp->dtype_Chg_ਚੌੜਾਈ(ਚੌੜਾਈ, ਚੌੜਾਈ_Min);  // The new AND must have width of the original SHIFT
        handle.relink(newp);
        iterate(newp);  // Further reduce, either node may have more reductions.
    }
    ਖਾਲੀ replace_ਖਿਸਕਾਓ_ਖਿਸਕਾਓ(Ast_ਗੰਢ_Biop* nodep) {
        UINFO(4, "SHIFT(SHIFT(a,s1),s2)->SHIFT(a,ADD(s1,s2)) " << nodep);
        UINFOTREE(9, nodep, "", "repShiftShift_old");
        Ast_ਗੰਢ_Biop* ਅਚਲ lhsp = VN_AS(nodep->lhsp(), ਗੰਢ_Biop);
        lhsp->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ap = lhsp->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ shift1p = lhsp->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ shift2p = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        // Shift1p and shift2p may have different sizes, both are
        // self-determined so sum with infinite width
        ਜੇ (nodep->type() == lhsp->type()) {
            ਅਚਲ ਪੂਰਨ shift1 = VN_AS(shift1p, Const)->ਮੰਜ਼ਿਲ_UInt();
            ਅਚਲ ਪੂਰਨ shift2 = VN_AS(shift2p, Const)->ਮੰਜ਼ਿਲ_UInt();
            ਅਚਲ ਪੂਰਨ newshift = shift1 + shift2;
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(shift2p), shift2p);
            nodep->lhsp(ap);
            nodep->rhsp(ਨਵਾਂ AstConst(nodep->fileline(), newshift));
            iterate(nodep);  // Further reduce, either node may have more reductions.
        } ਨਹੀਂਤਾਂ {
            // We know shift amounts are constant, but might be a mixed left/right shift
            ਪੂਰਨ shift1 = VN_AS(shift1p, Const)->ਮੰਜ਼ਿਲ_UInt();
            ਜੇ (VN_IS(lhsp, ਖਿਸਕਾਓ_R)) shift1 = -shift1;
            ਪੂਰਨ shift2 = VN_AS(shift2p, Const)->ਮੰਜ਼ਿਲ_UInt();
            ਜੇ (VN_IS(nodep, ਖਿਸਕਾਓ_R)) shift2 = -shift2;
            ਅਚਲ ਪੂਰਨ newshift = shift1 + shift2;
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(shift2p), shift2p);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp;
            V3_ਸੰਖਿਆ mask1{nodep, nodep->ਚੌੜਾਈ()};
            V3_ਸੰਖਿਆ ones{nodep, nodep->ਚੌੜਾਈ()};
            ones.ਸੈੱਟ_ਨਕਾਬ(nodep->ਚੌੜਾਈ());
            ਜੇ (shift1 < 0) {
                mask1.op_ਖਿਸਕਾਓ_R(ones, V3_ਸੰਖਿਆ(nodep, VL_IDATASIZE, -shift1));
            } ਨਹੀਂਤਾਂ {
                mask1.op_ਖਿਸਕਾਓ_L(ones, V3_ਸੰਖਿਆ(nodep, VL_IDATASIZE, shift1));
            }
            V3_ਸੰਖਿਆ ਨਕਾਬ{nodep, nodep->ਚੌੜਾਈ()};
            ਜੇ (shift2 < 0) {
                ਨਕਾਬ.op_ਖਿਸਕਾਓ_R(mask1, V3_ਸੰਖਿਆ(nodep, VL_IDATASIZE, -shift2));
            } ਨਹੀਂਤਾਂ {
                ਨਕਾਬ.op_ਖਿਸਕਾਓ_L(mask1, V3_ਸੰਖਿਆ(nodep, VL_IDATASIZE, shift2));
            }
            ਜੇ (newshift < 0) {
                newp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_R{nodep->fileline(), ap,
                                     ਨਵਾਂ AstConst(nodep->fileline(), -newshift)};
            } ਨਹੀਂਤਾਂ {
                newp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{nodep->fileline(), ap,
                                     ਨਵਾਂ AstConst(nodep->fileline(), newshift)};
            }
            newp->dtypeFrom(nodep);
            newp = ਨਵਾਂ AstAnd{nodep->fileline(), newp, ਨਵਾਂ AstConst{nodep->fileline(), ਨਕਾਬ}};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            // UINFOTREE(1, newp, "", "repShiftShift_new");
            iterate(newp);  // Further reduce, either node may have more reductions.
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(lhsp), lhsp);
    }

    ਤਾਰਕਿਕ replace_ਸੌਂਪੋ_Multi_Sel(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) {
        // Multiple assignments to sequential bits can be concated
        // ASSIGN(SEL(a),aq), ASSIGN(SEL(a+1),bq) -> ASSIGN(SEL(a:b),CONCAT(aq,bq)
        // ie. assign var[2]=a, assign var[3]=b -> assign var[3:2]={b,a}

        // Skip if we're not const'ing an entire module (IE doing only one assign, etc)
        ਜੇ (!m_modp) ਵਾਪਸ ਝੂਠ;
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ਅਚਲ sel1p = VN_CAST(nodep->lhsp(), Sel);
        ਜੇ (!sel1p) ਵਾਪਸ ਝੂਠ;
        Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ nextp = VN_CAST(nodep->nextp(), ਗੰਢ_ਸੌਂਪੋ);
        ਜੇ (!nextp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (nodep->type() != nextp->type()) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstSel* ਅਚਲ sel2p = VN_CAST(nextp->lhsp(), Sel);
        ਜੇ (!sel2p) ਵਾਪਸ ਝੂਠ;
        AstVarRef* ਅਚਲ varref1p = VN_CAST(sel1p->fromp(), VarRef);
        ਜੇ (!varref1p) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstVarRef* ਅਚਲ varref2p = VN_CAST(sel2p->fromp(), VarRef);
        ਜੇ (!varref2p) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!varref1p->same_Gate_ਰੁੱਖ(varref2p)) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstConst* ਅਚਲ con1p = VN_CAST(sel1p->lsbp(), Const);
        ਜੇ (!con1p) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstConst* ਅਚਲ con2p = VN_CAST(sel2p->lsbp(), Const);
        ਜੇ (!con2p) ਵਾਪਸ ਝੂਠ;
        // We need to make sure there's no self-references involved in either
        // assignment.  For speed, we only look 3 deep, then give up.
        ਜੇ (!varNotReferenced(nodep->rhsp(), varref1p->varp())) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!varNotReferenced(nextp->rhsp(), varref2p->varp())) ਵਾਪਸ ਝੂਠ;
        // If a variable is marked split_var, access to the variable should not be merged.
        ਜੇ (varref1p->varp()->attr_ਵੰਡੋ_Var() || varref2p->varp()->attr_ਵੰਡੋ_Var()) ਵਾਪਸ ਝੂਠ;
        // Swap?
        ਜੇ ((con1p->ਮੰਜ਼ਿਲ_SInt() != con2p->ਮੰਜ਼ਿਲ_SInt() + sel2p->ਚੌੜਾਈ())
            && (con2p->ਮੰਜ਼ਿਲ_SInt() != con1p->ਮੰਜ਼ਿਲ_SInt() + sel1p->ਚੌੜਾਈ())) {
            ਵਾਪਸ ਝੂਠ;
        }
        ਅਚਲ ਤਾਰਕਿਕ lsb_ਪਹਿਲਾ_ਸੌਂਪੋ = (con1p->ਮੰਜ਼ਿਲ_UInt() < con2p->ਮੰਜ਼ਿਲ_UInt());
        UINFO(4, "replaceAssignMultiSel " << nodep);
        UINFO(4, "                   && " << nextp);
        // UINFOTREE(1, nodep, "", "comb1");
        // UINFOTREE(1, nextp, "", "comb2");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhs1p = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhs2p = nextp->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਸੌਂਪੋ* newp;
        ਜੇ (lsb_ਪਹਿਲਾ_ਸੌਂਪੋ) {
            newp = nodep->cloneType(ਨਵਾਂ AstSel{sel1p->fileline(), varref1p->unlink_Fr_ਪਿੱਠ(),
                                               sel1p->lsbConst(), sel1p->ਚੌੜਾਈ() + sel2p->ਚੌੜਾਈ()},
                                    ਨਵਾਂ AstConcat{rhs1p->fileline(), rhs2p, rhs1p});
        } ਨਹੀਂਤਾਂ {
            newp = nodep->cloneType(ਨਵਾਂ AstSel{sel1p->fileline(), varref1p->unlink_Fr_ਪਿੱਠ(),
                                               sel2p->lsbConst(), sel1p->ਚੌੜਾਈ() + sel2p->ਚੌੜਾਈ()},
                                    ਨਵਾਂ AstConcat{rhs1p->fileline(), rhs1p, rhs2p});
        }
        // UINFOTREE(1, pnewp, "", "conew");
        nodep->replaceWith(newp);  // dypep intentionally changing
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nextp->unlink_Fr_ਪਿੱਠ()), nextp);
        ਵਾਪਸ ਸੱਚ;
    }

    ਤਾਰਕਿਕ replace_ਸੌਂਪੋ_SFormat_F(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) {
        // Rewrite 'x = sformatf(...)' into 'sformat(x, ...)', which is more efficient
        // at the call site as it does not need to check if 'x' needs to be freed first.
        // This is a somewhat common pattern after lowering assertions.
        ਜੇ (!VN_IS(nodep, ਸੌਂਪੋ)) ਵਾਪਸ ਝੂਠ;
        AstVarRef* ਅਚਲ lhsp = VN_CAST(nodep->lhsp(), VarRef);
        ਜੇ (!lhsp) ਵਾਪਸ ਝੂਠ;
        AstSFormatF* ਅਚਲ fmtp = VN_CAST(nodep->rhsp(), SFormatF);
        ਜੇ (!fmtp) ਵਾਪਸ ਝੂਠ;
        lhsp->unlink_Fr_ਪਿੱਠ();
        fmtp->unlink_Fr_ਪਿੱਠ();
        nodep->replaceWith(ਨਵਾਂ AstSFormat{nodep->fileline(), fmtp, lhsp});
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        ਵਾਪਸ ਸੱਚ;
    }

    ਤਾਰਕਿਕ varNotReferenced(Ast_ਗੰਢ* nodep, AstVar* varp, ਪੂਰਨ level = 0) {
        // Return true if varp never referenced under node.
        // Return false if referenced, or tree too deep to be worth it, or side effects
        ਜੇ (!nodep) ਵਾਪਸ ਸੱਚ;
        ਜੇ (level > 2) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!nodep->is_ਸ਼ੁੱਧ()) ਵਾਪਸ ਝੂਠ;  // For example a $fgetc can't be reordered
        ਜੇ (VN_IS(nodep, ਗੰਢ_Var_Ref) && VN_AS(nodep, ਗੰਢ_Var_Ref)->varp() == varp) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ (varNotReferenced(nodep->nextp(), varp, level + 1)
                && varNotReferenced(nodep->op1p(), varp, level + 1)
                && varNotReferenced(nodep->op2p(), varp, level + 1)
                && varNotReferenced(nodep->op3p(), varp, level + 1)
                && varNotReferenced(nodep->op4p(), varp, level + 1));
    }

    ਤਾਰਕਿਕ replace_ਗੰਢ_ਸੌਂਪੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) {
        ਜੇ (VN_IS(nodep->lhsp(), VarRef) && VN_IS(nodep->rhsp(), VarRef)
            && VN_AS(nodep->lhsp(), VarRef)->sameNoLvalue(VN_AS(nodep->rhsp(), VarRef))
            && !VN_IS(nodep, ਸੌਂਪੋ_Dly)) {
            // X = X.  Quite pointless, though X <= X may override another earlier assignment
            ਜੇ (VN_IS(nodep, ਸੌਂਪੋ_W)) {
                ਜੇ (!v3Global.opt.pedantic())
                    nodep->v3error(
                        "Wire inputs its own output, creating circular logic (wire x=x)");
                ਵਾਪਸ ਝੂਠ;  // Don't delete the assign, or V3Gate will freak out
            } ਨਹੀਂਤਾਂ {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                ਵਾਪਸ ਸੱਚ;
            }
        } ਨਹੀਂਤਾਂ ਜੇ (m_doV && VN_IS(nodep->lhsp(), Concat)) {
            ਤਾਰਕਿਕ need_ਆਰਜ਼ੀ = ਝੂਠ;
            // A blocking assignment with an intra-assignment timing control evaluates its value,
            // then waits once (IEEE 1800-2023 9.4.5), here to assign a temporary for the parts
            ਅਚਲ ਤਾਰਕਿਕ waits = VN_IS(nodep, ਸੌਂਪੋ) && nodep->timingControlp();
            // Splitting copies the RHS once per part, so a large RHS would make the tree
            // grow with the product of the number of parts and the RHS size
            ਅਚਲ ਤਾਰਕਿਕ need_ਆਰਜ਼ੀ_ਸ਼ੁੱਧ = !nodep->rhsp()->is_ਸ਼ੁੱਧ() || waits
                                      || nodep->rhsp()->isLargerThan(CONCAT_ਸੌਂਪੋ_MAX_RHS_NODES);
            ਜੇ (m_warn && !VN_IS(nodep, ਸੌਂਪੋ_Dly)
                && !need_ਆਰਜ਼ੀ_ਸ਼ੁੱਧ) {  // Is same var on LHS and RHS?
                // If the rhs is not pure, we need a temporary variable anyway
                std::unordered_ਸੈੱਟ<ਮਾਪ_t> lhsVarIds;
                nodep->lhsp()->foreach([&](ਅਚਲ AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    lhsVarIds.emplace(m_ids(refp->varp()));
                });
                nodep->rhsp()->foreach([&](ਅਚਲ AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    ਜੇ (lhsVarIds.ਗਿਣਤੀ_2(m_ids(refp->varp()))) need_ਆਰਜ਼ੀ = ਸੱਚ;
                });
            }
            ਜੇ (need_ਆਰਜ਼ੀ_ਸ਼ੁੱਧ) {
                // if the RHS is impure or large we need to create a temporary variable for it,
                // because further handling involves copying of the RHS.
                UINFO(4, "  ASSITEMPPURE " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp,rhs),
                //                                ASSIGN(lc1,SEL(temp,{size1})),
                //                                ASSIGN(lc2,SEL(temp,{size2}))

                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
                AstVar* ਅਚਲ ਆਰਜ਼ੀ_Purep = ਨਵਾਂ AstVar{rhsp->fileline(), VVarType::BLOCKTEMP,
                                                     m_concswap_ਨਾਂ.get(rhsp), rhsp->dtypep()};
                // Static, as read right after written, also in a fork
                ਆਰਜ਼ੀ_Purep->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ਜੋੜੋ_Stmtsp(ਆਰਜ਼ੀ_Purep);
                AstVarRef* ਅਚਲ ਆਰਜ਼ੀ_ਸ਼ੁੱਧ_Refp
                    = ਨਵਾਂ AstVarRef{rhsp->fileline(), ਆਰਜ਼ੀ_Purep, VAccess::WRITE};
                Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ asnp
                    = VN_IS(nodep, ਸੌਂਪੋ_Dly)
                          ? ਨਵਾਂ Ast_ਸੌਂਪੋ{nodep->fileline(), ਆਰਜ਼ੀ_ਸ਼ੁੱਧ_Refp, rhsp}
                          : nodep->cloneType(ਆਰਜ਼ੀ_ਸ਼ੁੱਧ_Refp, rhsp);
                nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(asnp);
                nodep->rhsp(ਨਵਾਂ AstVarRef{rhsp->fileline(), ਆਰਜ਼ੀ_Purep, VAccess::READ});
                // Only the assignment of the temporary waits
                ਜੇ (waits) ਧੱਕੋ_Deletep(nodep->timingControlp()->unlink_Fr_ਪਿੱਠ());
            } ਨਹੀਂਤਾਂ ਜੇ (need_ਆਰਜ਼ੀ) {
                // The first time we constify, there may be the same variable on the LHS
                // and RHS.  In that case, we must use temporaries, or {a,b}={b,a} will break.
                UINFO(4, "  ASSITEMP " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp1,SEL(rhs,{size})),
                //                                ASSIGN(temp2,SEL(newrhs,{size}))
                //                                ASSIGN(lc1,temp1),
                //                                ASSIGN(lc2,temp2)
            } ਨਹੀਂਤਾਂ {
                UINFO(4, "  ASSI " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(lc1,SEL(rhs,{size})),
                //                                ASSIGN(lc2,SEL(newrhs,{size}))
            }
            // The NBA evaluates its intra-assignment delay as it executes (IEEE 1800-2023 4.9.4),
            // so once for the NBAs to all parts
            AstDelay* ਅਚਲ delayp
                = VN_IS(nodep, ਸੌਂਪੋ_Dly) ? VN_CAST(nodep->timingControlp(), Delay) : ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (delayp && !delayp->lhsp()->is_ਸ਼ੁੱਧ()) {
                File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = delayp->lhsp()->unlink_Fr_ਪਿੱਠ();
                AstVar* ਅਚਲ tempp = ਨਵਾਂ AstVar{flp, VVarType::BLOCKTEMP,
                                                 m_concswap_ਨਾਂ.get(valuep), valuep->dtypep()};
                // Static, as read right after written, when the NBAs to the parts start waiting
                tempp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ਜੋੜੋ_Stmtsp(tempp);
                nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(
                    ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, tempp, VAccess::WRITE}, valuep});
                delayp->lhsp(ਨਵਾਂ AstVarRef{flp, tempp, VAccess::READ});
            }
            UINFOTREE(9, nodep, "", "Ass_old");
            // Unlink the stuff
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lc1p = VN_AS(nodep->lhsp(), Concat)->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lc2p = VN_AS(nodep->lhsp(), Concat)->rhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ conp = VN_AS(nodep->lhsp(), Concat)->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhs2p = rhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            // Calc widths
            ਅਚਲ ਪੂਰਨ lsb2 = 0;
            ਅਚਲ ਪੂਰਨ msb2 = lsb2 + lc2p->ਚੌੜਾਈ() - 1;
            ਅਚਲ ਪੂਰਨ lsb1 = msb2 + 1;
            ਅਚਲ ਪੂਰਨ msb1 = lsb1 + lc1p->ਚੌੜਾਈ() - 1;
            UASSERT_OBJ(msb1 == (conp->ਚੌੜਾਈ() - 1), nodep, "Width calc mismatch");
            // Form ranges
            AstSel* ਅਚਲ sel1p = ਨਵਾਂ AstSel{conp->fileline(), rhsp, lsb1, msb1 - lsb1 + 1};
            AstSel* ਅਚਲ sel2p = ਨਵਾਂ AstSel{conp->fileline(), rhs2p, lsb2, msb2 - lsb2 + 1};
            // Make new assigns of same flavor as old one
            //*** Not cloneTree; just one node.
            Ast_ਗੰਢ_ਸੌਂਪੋ* newp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (!need_ਆਰਜ਼ੀ) {
                Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ asn1ap = nodep->cloneType(lc1p, sel1p);
                Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ asn2ap = nodep->cloneType(lc2p, sel2p);
                asn1ap->dtypeFrom(sel1p);
                asn2ap->dtypeFrom(sel2p);
                newp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(newp, asn1ap);
                newp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(newp, asn2ap);
            } ਨਹੀਂਤਾਂ {
                UASSERT_OBJ(m_modp, nodep, "Not under module");
                UASSERT_OBJ(m_globalPass, nodep,
                            "Should not reach here when not invoked on whole AstNetlist");
                // We could create just one temp variable, but we'll get better optimization
                // if we make one per term.
                AstVar* ਅਚਲ temp1p
                    = ਨਵਾਂ AstVar{sel1p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswap_ਨਾਂ.get(sel1p), VFlagLogicPacked{}, msb1 - lsb1 + 1};
                AstVar* ਅਚਲ temp2p
                    = ਨਵਾਂ AstVar{sel2p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswap_ਨਾਂ.get(sel2p), VFlagLogicPacked{}, msb2 - lsb2 + 1};
                m_modp->ਜੋੜੋ_Stmtsp(temp1p);
                m_modp->ਜੋੜੋ_Stmtsp(temp2p);
                Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ asn1ap = nodep->cloneType(
                    ਨਵਾਂ AstVarRef{sel1p->fileline(), temp1p, VAccess::WRITE}, sel1p);
                Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ asn2ap = nodep->cloneType(
                    ਨਵਾਂ AstVarRef{sel2p->fileline(), temp2p, VAccess::WRITE}, sel2p);
                Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ asn1bp = nodep->cloneType(
                    lc1p, ਨਵਾਂ AstVarRef{sel1p->fileline(), temp1p, VAccess::READ});
                Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ asn2bp = nodep->cloneType(
                    lc2p, ਨਵਾਂ AstVarRef{sel2p->fileline(), temp2p, VAccess::READ});
                asn1ap->dtypeFrom(temp1p);
                asn1bp->dtypeFrom(temp1p);
                asn2ap->dtypeFrom(temp2p);
                asn2bp->dtypeFrom(temp2p);
                // This order matters
                newp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(newp, asn1ap);
                newp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(newp, asn2ap);
                newp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(newp, asn1bp);
                newp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(newp, asn2bp);
            }
            ਜੇ (debug() >= 9 && newp) newp->dump_ਰੁੱਖ_And_ਅਗਲਾ(ਬਾਹਰ, "-     _new: ");
            nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newp);
            // Cleanup
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(conp), conp);
            // Further reduce, either node may have more reductions.
            ਵਾਪਸ ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (m_doV && VN_IS(nodep->rhsp(), Cvt_Packed_ਮੰਜ਼ਿਲ_Array)
                   && ਹੇਠਲਾ_As_Fixed_Aggregate(nodep->lhsp()->dtypep())) {
            Ast_Cvt_Packed_ਮੰਜ਼ਿਲ_Array* ਅਚਲ cvtp
                = VN_AS(nodep->rhsp(), Cvt_Packed_ਮੰਜ਼ਿਲ_Array)->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* srcp = cvtp->fromp()->unlink_Fr_ਪਿੱਠ();
            ਜੇ (ਹੇਠਲਾ_As_Fixed_Aggregate(srcp->dtypep())) {
                srcp = packFixedAggregate(srcp);
            } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_ਧਾਰਾ* ਅਚਲ streamp = VN_CAST(srcp, ਗੰਢ_ਧਾਰਾ)) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਧਾਰਾ_Srcp = streamp->lhsp();
                ਜੇ (ਹੇਠਲਾ_As_Fixed_Aggregate(ਧਾਰਾ_Srcp->dtypep())) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ packedp = packFixedAggregate(ਧਾਰਾ_Srcp->unlink_Fr_ਪਿੱਠ());
                    streamp->lhsp(packedp);
                    streamp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(packedp->ਚੌੜਾਈ(), packedp->ਚੌੜਾਈ_Min(),
                                                       VSigning::UNSIGNED,
                                                       packedp->isFourstateOrNoDtype());
                }
            }
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(cvtp), cvtp);
            replace_ਸੌਂਪੋ_ਮੰਜ਼ਿਲ_Fixed_Aggregate(nodep, nodep->lhsp()->unlink_Fr_ਪਿੱਠ(), srcp);
            ਵਾਪਸ ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (m_doV && VN_IS(nodep->rhsp(), ਧਾਰਾ_R)
                   && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), ਕਤਾਰ_DType)) {
            // The right-streaming operator on rhs of assignment does not
            // change the order of bits. Eliminate stream but keep its lhsp.
            // Add a cast if needed.
            Ast_ਧਾਰਾ_R* ਅਚਲ streamp = VN_AS(nodep->rhsp(), ਧਾਰਾ_R)->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* srcp = streamp->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_DType* ਅਚਲ srcDTypep = srcp->dtypep()->skipRefp();
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            ਜੇ (ਹੇਠਲਾ_As_Fixed_Aggregate(srcDTypep)) {
                srcp = packFixedAggregate(srcp);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(srcDTypep, ਕਤਾਰ_DType) || VN_IS(srcDTypep, DynArrayDType)) {
                ਜੇ (VN_IS(dstDTypep, ਕਤਾਰ_DType) || VN_IS(dstDTypep, DynArrayDType)) {
                    ਪੂਰਨ src_Element_ਬਿੱਟ = 0;
                    ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = srcDTypep->subDTypep()) {
                        src_Element_ਬਿੱਟ = elemDtp->ਚੌੜਾਈ();
                    }
                    ਪੂਰਨ dst_Element_ਬਿੱਟ = 0;
                    ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_ਬਿੱਟ = elemDtp->ਚੌੜਾਈ();
                    }
                    srcp = ਨਵਾਂ Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Array{
                        srcp->fileline(), srcp,          nodep->dtypep(), ਝੂਠ, 1,
                        dst_Element_ਬਿੱਟ,   src_Element_ਬਿੱਟ};
                } ਨਹੀਂਤਾਂ {
                    srcp = ਨਵਾਂ Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Packed{srcp->fileline(), srcp, nodep->dtypep()};
                }
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(srcDTypep, UnpackArrayDType)) {
                srcp = ਨਵਾਂ Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Packed{srcp->fileline(), srcp, srcDTypep};
                // Handling the case where lhs is wider than rhs by inserting zeros. StreamL does
                // not require this, since the left streaming operator implicitly handles this.
                ਅਚਲ ਪੂਰਨ packed_ਬਿੱਟ = nodep->lhsp()->ਚੌੜਾਈ_Min();
                ਅਚਲ ਪੂਰਨ unpack_ਬਿੱਟ
                    = srcDTypep->arrayUnpackedElements() * srcDTypep->subDTypep()->ਚੌੜਾਈ_Min();
                ਅਚਲ uint32_t ਵਿੱਥ = packed_ਬਿੱਟ > unpack_ਬਿੱਟ ? packed_ਬਿੱਟ - unpack_ਬਿੱਟ : 0;
                srcp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{srcp->fileline(), srcp,
                                     ਨਵਾਂ AstConst{srcp->fileline(), ਵਿੱਥ}, packed_ਬਿੱਟ};
            }
            ਜੇ (!VN_IS(dstDTypep, UnpackArrayDType) && !VN_IS(dstDTypep, ਕਤਾਰ_DType)
                && !VN_IS(dstDTypep, DynArrayDType)) {
                ਅਚਲ ਪੂਰਨ s_ਚੌੜਾਈ = srcp->ਚੌੜਾਈ();
                ਅਚਲ ਪੂਰਨ d_ਚੌੜਾਈ = nodep->lhsp()->ਚੌੜਾਈ();
                ਜੇ (s_ਚੌੜਾਈ < d_ਚੌੜਾਈ) {
                    AstExtend* ਅਚਲ extendp = ਨਵਾਂ AstExtend{srcp->fileline(), srcp, d_ਚੌੜਾਈ};
                    srcp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{
                        srcp->fileline(), extendp,
                        ਨਵਾਂ AstConst{srcp->fileline(), static_cast<uint32_t>(d_ਚੌੜਾਈ - s_ਚੌੜਾਈ)},
                        d_ਚੌੜਾਈ};
                }
            }
            nodep->rhsp(srcp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            ਵਾਪਸ ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (m_doV && VN_IS(nodep->lhsp(), ਧਾਰਾ_L)) {
            // Push the stream operator to the rhs of the assignment statement
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* streamp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ dstp = VN_AS(streamp, ਧਾਰਾ_L)->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_DType* ਅਚਲ dstDTypep = dstp->dtypep()->skipRefp();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* srcp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ srcDTypep = srcp->dtypep()->skipRefp();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamL reverses, so reverse=true)
            ਜੇ ((VN_IS(srcDTypep, UnpackArrayDType) || VN_IS(srcDTypep, ਕਤਾਰ_DType)
                 || VN_IS(srcDTypep, DynArrayDType))
                && (VN_IS(dstDTypep, ਕਤਾਰ_DType) || VN_IS(dstDTypep, DynArrayDType))) {
                ਪੂਰਨ ਖੰਡ_ਮਾਪ = 1;
                ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp
                    = VN_CAST(VN_AS(streamp, ਧਾਰਾ_L)->rhsp(), Const)) {
                    ਖੰਡ_ਮਾਪ = constp->ਮੰਜ਼ਿਲ_SInt();
                    ਜੇ (VL_UNLIKELY(ਖੰਡ_ਮਾਪ <= 0)) ਖੰਡ_ਮਾਪ = 1;
                }
                ਪੂਰਨ src_Element_ਬਿੱਟ = 0;
                ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = srcDTypep->subDTypep()) {
                    src_Element_ਬਿੱਟ = elemDtp->ਚੌੜਾਈ();
                }
                ਪੂਰਨ dst_Element_ਬਿੱਟ = 0;
                ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = dstDTypep->subDTypep()) {
                    dst_Element_ਬਿੱਟ = elemDtp->ਚੌੜਾਈ();
                }
                nodep->lhsp(dstp);
                nodep->rhsp(ਨਵਾਂ Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Array{srcp->fileline(), srcp, dstDTypep, ਸੱਚ,
                                                   ਖੰਡ_ਮਾਪ, dst_Element_ਬਿੱਟ, src_Element_ਬਿੱਟ});
                nodep->dtypep(dstDTypep);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(streamp), streamp);
                ਵਾਪਸ ਸੱਚ;
            }
            ਜੇ (ਹੇਠਲਾ_As_Fixed_Aggregate(srcDTypep)) srcp = packFixedAggregate(srcp);
            ਅਚਲ ਪੂਰਨ s_ਚੌੜਾਈ = srcp->ਚੌੜਾਈ();
            ਅਚਲ ਪੂਰਨ d_ਚੌੜਾਈ = dstp->ਚੌੜਾਈ();
            // Connect the rhs to the stream operator and update its width
            VN_AS(streamp, ਧਾਰਾ_L)->lhsp(srcp);
            ਜੇ (VN_IS(srcDTypep, DynArrayDType) || VN_IS(srcDTypep, ਕਤਾਰ_DType)
                || VN_IS(srcDTypep, UnpackArrayDType)) {
                streamp->dtype_ਸੈੱਟ_ਧਾਰਾ();
            } ਨਹੀਂਤਾਂ {
                streamp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(srcp->ਚੌੜਾਈ(), srcp->ਚੌੜਾਈ_Min(),
                                                   VSigning::UNSIGNED,
                                                   srcp->isFourstateOrNoDtype());
            }
            ਜੇ (VN_IS(dstDTypep, UnpackArrayDType)) {
                streamp = ਨਵਾਂ Ast_Cvt_Packed_ਮੰਜ਼ਿਲ_Array{nodep->fileline(), streamp, dstDTypep};
            } ਨਹੀਂਤਾਂ {
                ਜੇ (d_ਚੌੜਾਈ == 0) {
                    streamp = ਨਵਾਂ Ast_Cvt_Packed_ਮੰਜ਼ਿਲ_Array{nodep->fileline(), streamp, dstDTypep};
                } ਨਹੀਂਤਾਂ ਜੇ (s_ਚੌੜਾਈ >= d_ਚੌੜਾਈ) {
                    streamp = ਨਵਾਂ AstSel{streamp->fileline(), streamp, s_ਚੌੜਾਈ - d_ਚੌੜਾਈ, d_ਚੌੜਾਈ};
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(streamp);
            nodep->dtypep(dstDTypep);
            ਵਾਪਸ ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (m_doV && VN_IS(nodep->lhsp(), ਧਾਰਾ_R)) {
            // The right stream operator on lhs of assignment statement does
            // not reorder bits. However, if the rhs is wider than the lhs,
            // then we select bits from the left-most, not the right-most.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ streamp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ dstp = VN_AS(streamp, ਧਾਰਾ_R)->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_DType* ਅਚਲ dstDTypep = dstp->dtypep()->skipRefp();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* srcp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamR does not reverse, so reverse=false).
            // V3Width may have wrapped the source in CvtArrayToPacked; unwrap it.
            ਜੇ (VN_IS(dstDTypep, ਕਤਾਰ_DType) || VN_IS(dstDTypep, DynArrayDType)) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* origSrcp = srcp;
                ਜੇ (Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Packed* ਅਚਲ cvtp = VN_CAST(srcp, Cvt_Array_ਮੰਜ਼ਿਲ_Packed)) {
                    origSrcp = cvtp->fromp();
                }
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ origSrcDTypep = origSrcp->dtypep()->skipRefp();
                ਜੇ (VN_IS(origSrcDTypep, UnpackArrayDType) || VN_IS(origSrcDTypep, ਕਤਾਰ_DType)
                    || VN_IS(origSrcDTypep, DynArrayDType)) {
                    ਪੂਰਨ src_Element_ਬਿੱਟ = 0;
                    ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = origSrcDTypep->subDTypep()) {
                        src_Element_ਬਿੱਟ = elemDtp->ਚੌੜਾਈ();
                    }
                    ਪੂਰਨ dst_Element_ਬਿੱਟ = 0;
                    ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_ਬਿੱਟ = elemDtp->ਚੌੜਾਈ();
                    }
                    ਜੇ (VN_IS(srcp, Cvt_Array_ਮੰਜ਼ਿਲ_Packed)) {
                        origSrcp = VN_AS(srcp, Cvt_Array_ਮੰਜ਼ਿਲ_Packed)->fromp()->unlink_Fr_ਪਿੱਠ();
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(srcp), srcp);
                        srcp = origSrcp;
                    }
                    // Descending unpacked arrays need element reversal
                    ਤਾਰਕਿਕ ਉਲਟ = ਝੂਠ;
                    ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ unpackDtp
                        = VN_CAST(origSrcDTypep, UnpackArrayDType)) {
                        ਉਲਟ = !unpackDtp->declRange().ascending();
                    }
                    nodep->lhsp(dstp);
                    nodep->rhsp(ਨਵਾਂ Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Array{srcp->fileline(), srcp, dstDTypep, ਉਲਟ,
                                                       1, dst_Element_ਬਿੱਟ, src_Element_ਬਿੱਟ});
                    nodep->dtypep(dstDTypep);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(streamp), streamp);
                    ਵਾਪਸ ਸੱਚ;
                }
            }
            ਅਚਲ ਪੂਰਨ s_ਚੌੜਾਈ = srcp->ਚੌੜਾਈ();
            ਅਚਲ ਪੂਰਨ d_ਚੌੜਾਈ = dstp->ਚੌੜਾਈ();
            ਜੇ (VN_IS(dstDTypep, UnpackArrayDType)) {
                ਅਚਲ ਪੂਰਨ dst_ਦੋਆਧਾਰੀ_ਅੰਕ_ਚੌੜਾਈ
                    = d_ਚੌੜਾਈ * VN_AS(dstDTypep, UnpackArrayDType)->arrayUnpackedElements();
                // Handling the case where rhs is wider than lhs. StreamL does not require this
                // since the combination of the left streaming operation and the implicit
                // truncation in VL_ASSIGN_UNPACK automatically selects the left-most bits.
                ਜੇ (s_ਚੌੜਾਈ > dst_ਦੋਆਧਾਰੀ_ਅੰਕ_ਚੌੜਾਈ) {
                    srcp
                        = ਨਵਾਂ AstSel{streamp->fileline(), srcp, s_ਚੌੜਾਈ - dst_ਦੋਆਧਾਰੀ_ਅੰਕ_ਚੌੜਾਈ, dst_ਦੋਆਧਾਰੀ_ਅੰਕ_ਚੌੜਾਈ};
                }
                srcp = ਨਵਾਂ Ast_Cvt_Packed_ਮੰਜ਼ਿਲ_Array{nodep->fileline(), srcp, dstDTypep};
            } ਨਹੀਂਤਾਂ {
                ਜੇ (d_ਚੌੜਾਈ == 0) {
                    srcp = ਨਵਾਂ Ast_Cvt_Packed_ਮੰਜ਼ਿਲ_Array{nodep->fileline(), srcp, dstDTypep};
                } ਨਹੀਂਤਾਂ ਜੇ (s_ਚੌੜਾਈ >= d_ਚੌੜਾਈ) {
                    srcp = ਨਵਾਂ AstSel{streamp->fileline(), srcp, s_ਚੌੜਾਈ - d_ਚੌੜਾਈ, d_ਚੌੜਾਈ};
                } ਨਹੀਂਤਾਂ {
                    // Source narrower than destination: left-justify by shifting left.
                    // The right stream operator packs left-to-right, so remaining
                    // LSBs are zero-filled (IEEE 1800-2023 11.4.14.3).
                    ਜੇ (!VN_IS(srcp->dtypep()->skipRefp(), ਕਤਾਰ_DType)) {
                        AstExtend* ਅਚਲ extendp = ਨਵਾਂ AstExtend{srcp->fileline(), srcp, d_ਚੌੜਾਈ};
                        srcp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{
                            srcp->fileline(), extendp,
                            ਨਵਾਂ AstConst{srcp->fileline(), static_cast<uint32_t>(d_ਚੌੜਾਈ - s_ਚੌੜਾਈ)},
                            d_ਚੌੜਾਈ};
                    } ਨਹੀਂਤਾਂ {  // if it is a queue we dont need do do a shiftL
                        srcp = ਨਵਾਂ AstExtend{srcp->fileline(), srcp};
                    }
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(srcp);
            nodep->dtypep(dstDTypep);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            ਵਾਪਸ ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (m_doV && VN_IS(nodep->rhsp(), ਧਾਰਾ_L)) {
            Ast_ਧਾਰਾ_L* streamp = VN_AS(nodep->rhsp(), ਧਾਰਾ_L);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* srcp = streamp->lhsp();
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ srcDTypep = srcp->dtypep()->skipRefp();
            Ast_ਗੰਢ_DType* ਅਚਲ dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            ਜੇ (ਹੇਠਲਾ_As_Fixed_Aggregate(srcDTypep)) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ packedp = packFixedAggregate(srcp->unlink_Fr_ਪਿੱਠ());
                streamp->lhsp(packedp);
                streamp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(packedp->ਚੌੜਾਈ(), packedp->ਚੌੜਾਈ_Min(),
                                                   VSigning::UNSIGNED,
                                                   packedp->isFourstateOrNoDtype());
                srcp = packedp;
            } ਨਹੀਂਤਾਂ ਜੇ ((VN_IS(srcDTypep, ਕਤਾਰ_DType) || VN_IS(srcDTypep, DynArrayDType)
                        || VN_IS(srcDTypep, UnpackArrayDType))) {
                ਜੇ (VN_IS(dstDTypep, ਕਤਾਰ_DType) || VN_IS(dstDTypep, DynArrayDType)) {
                    ਪੂਰਨ ਖੰਡ_ਮਾਪ = 1;
                    ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(streamp->rhsp(), Const)) {
                        ਖੰਡ_ਮਾਪ = constp->ਮੰਜ਼ਿਲ_SInt();
                        ਜੇ (VL_UNLIKELY(ਖੰਡ_ਮਾਪ <= 0)) {
                            // Not reachable due to higher level checks when parsing stream
                            // operators commented out to not fail v3error-coverage-checks.
                            // nodep->v3error("Stream block size must be positive, got " <<
                            // blockSize); nodep->v3error("Stream block size must be positive, got
                            // " << blockSize);
                            ਖੰਡ_ਮਾਪ = 1;
                        }
                    }
                    // Not reachable due to higher level checks when parsing stream operators
                    // commented out to not fail v3error-coverage-checks.
                    // else {
                    // nodep->v3error("Stream block size must be constant (got " <<
                    // streamp->rhsp()->prettyTypeName() << ")");
                    // }
                    ਪੂਰਨ src_Element_ਬਿੱਟ = 0;
                    ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = srcDTypep->subDTypep()) {
                        src_Element_ਬਿੱਟ = elemDtp->ਚੌੜਾਈ();
                    }
                    ਪੂਰਨ dst_Element_ਬਿੱਟ = 0;
                    ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_ਬਿੱਟ = elemDtp->ਚੌੜਾਈ();
                    }
                    streamp->unlink_Fr_ਪਿੱਠ();
                    srcp = ਨਵਾਂ Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Array{
                        srcp->fileline(), srcp->unlink_Fr_ਪਿੱਠ(), dstDTypep,     ਸੱਚ,
                        ਖੰਡ_ਮਾਪ,        dst_Element_ਬਿੱਟ,       src_Element_ਬਿੱਟ};
                    nodep->rhsp(srcp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(streamp), streamp);
                } ਨਹੀਂਤਾਂ {
                    streamp->lhsp(ਨਵਾਂ Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Packed{srcp->fileline(), srcp->unlink_Fr_ਪਿੱਠ(),
                                                          dstDTypep});
                    streamp->dtypeFrom(dstDTypep);
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (m_doV && replace_ਸੌਂਪੋ_Multi_Sel(nodep)) {
            ਵਾਪਸ ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (replace_ਸੌਂਪੋ_SFormat_F(nodep)) {
            ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // Boolean replacements
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Bool_ਖਿਸਕਾਓ(ਅਚਲ Ast_ਗੰਢ* nodep) {
        // boolean test of AND(const,SHIFTR(x,const)) -> test of AND(SHIFTL(x,const), x)
        ਜੇ (!VN_IS(nodep, And)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!VN_IS(VN_AS(nodep, And)->lhsp(), Const)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!VN_IS(VN_AS(nodep, And)->rhsp(), ਖਿਸਕਾਓ_R)) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_ਖਿਸਕਾਓ_R* ਅਚਲ shiftp = VN_AS(VN_AS(nodep, And)->rhsp(), ਖਿਸਕਾਓ_R);
        ਜੇ (!VN_IS(shiftp->rhsp(), Const)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (static_cast<uint32_t>(nodep->ਚੌੜਾਈ()) <= VN_AS(shiftp->rhsp(), Const)->ਮੰਜ਼ਿਲ_UInt()) {
            ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ ਸੱਚ;
    }
    ਖਾਲੀ replace_Bool_ਖਿਸਕਾਓ(Ast_ਗੰਢ* nodep) {
        UINFOTREE(9, nodep, "", "bshft_old");
        AstConst* ਅਚਲ andConstp = VN_AS(VN_AS(nodep, And)->lhsp(), Const);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp
            = VN_AS(VN_AS(nodep, And)->rhsp(), ਖਿਸਕਾਓ_R)->lhsp()->unlink_Fr_ਪਿੱਠ();
        AstConst* ਅਚਲ ਖਿਸਕਾਓ_Constp
            = VN_AS(VN_AS(VN_AS(nodep, And)->rhsp(), ਖਿਸਕਾਓ_R)->rhsp(), Const);
        V3_ਸੰਖਿਆ val{andConstp, andConstp->ਚੌੜਾਈ()};
        val.op_ਖਿਸਕਾਓ_L(andConstp->ਅੰਕ(), ਖਿਸਕਾਓ_Constp->ਅੰਕ());
        AstAnd* ਅਚਲ newp
            = ਨਵਾਂ AstAnd{nodep->fileline(), ਨਵਾਂ AstConst{nodep->fileline(), val}, fromp};
        // widthMin no longer applicable if different C-expanded width
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        UINFOTREE(9, newp, "", "_new");
    }

    ਖਾਲੀ replaceWithSimulation(Ast_ਗੰਢ* nodep) {
        SimulateVisitor simvis;
        // Run it - may be unoptimizable due to large for loop, etc
        simvis.mainParamEmulate(nodep);
        ਜੇ (!simvis.optimizable()) {
            ਅਚਲ Ast_ਗੰਢ* errorp = simvis.whyNotNodep();
            ਜੇ (!errorp) errorp = nodep;
            nodep->v3error("Expecting expression to be constant, but can't determine constant for "
                           << nodep->prettyTypeName() << '\n'
                           << errorp->warnOther() << "... Location of non-constant "
                           << errorp->prettyTypeName() << ": " << simvis.why_Not_ਸੁਨੇਹਾ());
            VL_DO_DANGLING(replaceZero(nodep), nodep);
        } ਨਹੀਂਤਾਂ {
            // Fetch the result
            Ast_ਗੰਢ* ਅਚਲ valuep = simvis.fetch_ਮੁੱਲ_Null(nodep);  // valuep is owned by Simulate
            UASSERT_OBJ(valuep, nodep, "No value returned from simulation");
            // Replace it
            Ast_ਗੰਢ* ਅਚਲ newp = valuep->clone_ਰੁੱਖ(ਝੂਠ);
            newp->fileline(nodep->fileline());
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Simulate->" << newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }

    //----------------------------------------

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        iterateChildrenBackwardsConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_concswap_ਨਾਂ.reset();
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        // No ASSIGNW removals under funcs, we've long eliminated INITIALs
        // (We should perhaps rename the assignw's to just assigns)
        VL_RESTORER(m_wremove);
        m_wremove = ਝੂਠ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCLocalScope* nodep) override {
        iterateChildren(nodep);
        ਜੇ (!nodep->stmtsp()) {
            nodep->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        // No ASSIGNW removals under scope, we've long eliminated INITIALs
        VL_RESTORER(m_wremove);
        VL_RESTORER(m_scopep);
        m_wremove = ਝੂਠ;
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਅਦਲਾ_ਬਦਲੀ_Sides(Ast_ਗੰਢ_Bi_Com* nodep) {
        // COMMUTATIVE({a},CONST) -> COMMUTATIVE(CONST,{a})
        // This simplifies later optimizations
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
        nodep->lhsp(rhsp);
        nodep->rhsp(lhsp);
        iterate(nodep);  // Again?
    }

    ਤਾਰਕਿਕ ਸ਼ਾਮਲ_ਹੈ_Member_Access_Recurse(Ast_ਗੰਢ* ਅਚਲ nodep) {
        ਜੇ (!nodep) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ ਮਾਪ_t id = m_ids(nodep);
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_ਸ਼ਾਮਲ_ਹੈ_Member_Access.ਲੱਭੋ(id);
        ਜੇ (it != m_ਸ਼ਾਮਲ_ਹੈ_Member_Access.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;
        ++m_statMemberAccessVisits;
        ਤਾਰਕਿਕ ਨਤੀਜਾ = ਝੂਠ;
        ਜੇ (VN_IS(nodep, MemberSel) || VN_IS(nodep, ਵਿਧੀ_Call) || VN_IS(nodep, CMethodCall)) {
            ਨਤੀਜਾ = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_FTask_Ref* ਅਚਲ funcRefp = VN_CAST(nodep, ਗੰਢ_FTask_Ref)) {
            ਜੇ (ਸ਼ਾਮਲ_ਹੈ_Member_Access_Recurse(funcRefp->taskp())) ਨਤੀਜਾ = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_CCall* ਅਚਲ funcRefp = VN_CAST(nodep, ਗੰਢ_CCall)) {
            ਜੇ (ਸ਼ਾਮਲ_ਹੈ_Member_Access_Recurse(funcRefp->funcp())) ਨਤੀਜਾ = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_FTask* ਅਚਲ funcp = VN_CAST(nodep, ਗੰਢ_FTask)) {
            // Assume that it has a member access
            ਜੇ (funcp->recursive()) ਨਤੀਜਾ = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstCFunc* ਅਚਲ funcp = VN_CAST(nodep, CFunc)) {
            ਜੇ (funcp->recursive()) ਨਤੀਜਾ = ਸੱਚ;
        }
        ਜੇ (!ਨਤੀਜਾ) {
            ਨਤੀਜਾ = ਸ਼ਾਮਲ_ਹੈ_Member_Access_Recurse(nodep->op1p())
                     || ਸ਼ਾਮਲ_ਹੈ_Member_Access_Recurse(nodep->op2p())
                     || ਸ਼ਾਮਲ_ਹੈ_Member_Access_Recurse(nodep->op3p())
                     || ਸ਼ਾਮਲ_ਹੈ_Member_Access_Recurse(nodep->op4p());
        }
        ਜੇ (!ਨਤੀਜਾ && !VN_IS(nodep, ਗੰਢ_FTask)
            && !VN_IS(nodep, CFunc)  // don't enter into next function
            && ਸ਼ਾਮਲ_ਹੈ_Member_Access_Recurse(nodep->nextp())) {
            ਨਤੀਜਾ = ਸੱਚ;
        }
        m_ਸ਼ਾਮਲ_ਹੈ_Member_Access.emplace(id, ਨਤੀਜਾ);
        ਵਾਪਸ ਨਤੀਜਾ;
    }

    ਤਾਰਕਿਕ ਮੇਲ_Biop_ਮੰਜ਼ਿਲ_Bitwise(Ast_ਗੰਢ_Biop* ਅਚਲ nodep) {
        ਜੇ (!m_convert_Logic_ਮੰਜ਼ਿਲ_ਦੋਆਧਾਰੀ_ਅੰਕ) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!nodep->lhsp()->width1()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!nodep->rhsp()->width1()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!nodep->is_ਸ਼ੁੱਧ()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (ਸ਼ਾਮਲ_ਹੈ_Member_Access_Recurse(nodep)) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ ਮੇਲ_Concat_Rand(AstConcat* nodep) {
        //    CONCAT(RAND, RAND) - created by Chisel code
        AstRand* ਅਚਲ aRandp = VN_CAST(nodep->lhsp(), Rand);
        ਅਚਲ AstRand* ਅਚਲ bRandp = VN_CAST(nodep->rhsp(), Rand);
        ਜੇ (!aRandp || !bRandp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!aRandp->combinable(bRandp)) ਵਾਪਸ ਝੂਠ;
        UINFO(4, "Concat(Rand,Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlink_Fr_ਪਿੱਠ());
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ ਮੇਲ_Sel_Rand(AstSel* nodep) {
        //    SEL(RAND) - created by Chisel code
        AstRand* ਅਚਲ aRandp = VN_CAST(nodep->fromp(), Rand);
        ਜੇ (!aRandp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (aRandp->seedp()) ਵਾਪਸ ਝੂਠ;
        UINFO(4, "Sel(Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlink_Fr_ਪਿੱਠ());
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ ਮੇਲ_ਮੰਜ਼ਿਲ_String_NConst(Ast_ਮੰਜ਼ਿਲ_String_N* nodep) {
        iterateChildren(nodep);
        ਜੇ (ਅਚਲ AstConsArray* ਅਚਲ consp = VN_CAST(nodep->lhsp(), ConsArray)) {
            ਜੇ (!(m_doExpensive || m_params)) ਵਾਪਸ ਝੂਠ;
            ਜੇ (!consp->is_Constant_ਰੁੱਖ()) ਵਾਪਸ ਝੂਠ;
        } ਨਹੀਂਤਾਂ ਜੇ (!VN_IS(nodep->lhsp(), Const)) {
            ਵਾਪਸ ਝੂਠ;
        }
        replaceWithSimulation(nodep);
        ਵਾਪਸ ਸੱਚ;
    }
    ਪੂਰਨ ਸੰਕਾਰਜ_Concat_Move(ਅਚਲ AstConcat* nodep) {
        //    CONCAT under concat  (See moveConcat)
        // Return value: true indicates to do it; 2 means move to LHS
        ਅਚਲ AstConcat* ਅਚਲ abConcp = VN_CAST(nodep->lhsp(), Concat);
        ਅਚਲ AstConcat* ਅਚਲ bcConcp = VN_CAST(nodep->rhsp(), Concat);
        ਜੇ (!abConcp && !bcConcp) ਵਾਪਸ 0;
        ਜੇ (bcConcp) {
            ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ap = nodep->lhsp();
            ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bp = bcConcp->lhsp();
            // If a+b == 32,64,96 etc, then we want to have a+b together on LHS
            ਜੇ (VL_BITBIT_I(ap->ਚੌੜਾਈ() + bp->ਚੌੜਾਈ()) == 0) ਵਾਪਸ 2;  // Transform 2: to abConc
        } ਨਹੀਂਤਾਂ {  // abConcp
            // Unless lhs is already 32 bits due to above, reorder it
            ਜੇ (VL_BITBIT_I(nodep->lhsp()->ਚੌੜਾਈ()) != 0) ਵਾਪਸ 1;  // Transform 1: to bcConc
        }
        ਵਾਪਸ 0;  // ok
    }
    ਖਾਲੀ moveConcat(AstConcat* nodep) {
        //    1: CONCAT(CONCAT({a},{b}),{c})  -> CONCAT({a},CONCAT({b}, {c}))
        // or 2: CONCAT({a}, CONCAT({b},{c})) -> CONCAT(CONCAT({a},{b}),{c})
        // Because the lhs of a concat needs a shift but the rhs doesn't,
        // putting additional CONCATs on the RHS leads to fewer assembler operations.
        // However, we'll end up with lots of wide moves if we make huge trees
        // like that, so on 32 bit boundaries, we'll do the opposite form.
        UINFO(4, "Move concat: " << nodep);
        ਜੇ (ਸੰਕਾਰਜ_Concat_Move(nodep) > 1) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ap = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
            AstConcat* ਅਚਲ bcConcp = VN_AS(nodep->rhsp(), Concat);
            bcConcp->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bp = bcConcp->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cp = bcConcp->rhsp()->unlink_Fr_ਪਿੱਠ();
            AstConcat* ਅਚਲ abConcp = ਨਵਾਂ AstConcat{bcConcp->fileline(), ap, bp};
            nodep->lhsp(abConcp);
            nodep->rhsp(cp);
            // If bp was a concat, then we have this exact same form again!
            // Recurse rather then calling node->iterate to prevent 2^n recursion!
            ਜੇ (ਸੰਕਾਰਜ_Concat_Move(abConcp)) moveConcat(abConcp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(bcConcp), bcConcp);
        } ਨਹੀਂਤਾਂ {
            AstConcat* ਅਚਲ abConcp = VN_AS(nodep->lhsp(), Concat);
            abConcp->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ap = abConcp->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bp = abConcp->rhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            AstConcat* ਅਚਲ bcConcp = ਨਵਾਂ AstConcat{abConcp->fileline(), bp, cp};
            nodep->lhsp(ap);
            nodep->rhsp(bcConcp);
            ਜੇ (ਸੰਕਾਰਜ_Concat_Move(bcConcp)) moveConcat(bcConcp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(abConcp), abConcp);
        }
    }

    // Special cases
    ਖਾਲੀ ਦੇਖੋ(AstConst*) override {}  // Already constant

    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        ਜੇ (m_params) {
            iterate_And_ਅਗਲਾ_Null(nodep->paramsp());
        } ਨਹੀਂਤਾਂ {
            iterateChildren(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassOrPackageRef* nodep) override { iterateChildren(nodep); }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਮੇਲ_Masked* nodep) override {
        // Do not iterate the tables, they must be constant pool entries
        iterate(nodep->lhsp());
        ਜੇ (AstConst* ਅਚਲ constp = VN_CAST(nodep->lhsp(), Const)) {
            replace_ਅੰਕ(nodep, Ast_ਮੇਲ_Masked::fold(constp->ਅੰਕ(), nodep->matchp()->varp()));
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstPin* nodep) override { iterateChildren(nodep); }

    ਖਾਲੀ replaceLogEq(AstLogEq* nodep) {
        // LOGEQ(a,b) => AstLogAnd{AstLogOr{AstLogNot{a},b},AstLogOr{AstLogNot{b},a}}
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        // Do exactly as IEEE says, might result in extra terms, so in future may do differently
        AstLogAnd* ਅਚਲ newp = ਨਵਾਂ AstLogAnd{
            nodep->fileline(),
            ਨਵਾਂ AstLogOr{nodep->fileline(), ਨਵਾਂ AstLogNot{nodep->fileline(), lhsp}, rhsp},
            ਨਵਾਂ AstLogOr{nodep->fileline(),
                         ਨਵਾਂ AstLogNot{nodep->fileline(), rhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)},
                         lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ replaceSelSel(AstSel* nodep) {
        // SEL(SEL({x},a,b),c,d) => SEL({x},a+c,d)
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ਅਚਲ belowp = VN_AS(nodep->fromp(), Sel);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = belowp->fromp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lsb1p = nodep->lsbp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lsb2p = belowp->lsbp()->unlink_Fr_ਪਿੱਠ();
        // Eliminate lower range
        UINFO(4, "Elim Lower range: " << nodep);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newlsbp;
        ਜੇ (VN_IS(lsb1p, Const) && VN_IS(lsb2p, Const)) {
            newlsbp = ਨਵਾਂ AstConst{lsb1p->fileline(),
                                   VN_AS(lsb1p, Const)->ਮੰਜ਼ਿਲ_UInt() + VN_AS(lsb2p, Const)->ਮੰਜ਼ਿਲ_UInt()};
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(lsb1p), lsb1p);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(lsb2p), lsb2p);
        } ਨਹੀਂਤਾਂ {
            // Width is important, we need the width of the fromp's expression, not the
            // potentially smaller lsb1p's width, but don't insert a redundant AstExtend.
            // Note that due to some sloppiness in earlier passes, lsb1p might actually be wider,
            // so extend to the wider type.
            ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ widep = lsb1p->ਚੌੜਾਈ() > lsb2p->ਚੌੜਾਈ() ? lsb1p : lsb2p;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = widep->ਚੌੜਾਈ() > lsb2p->ਚੌੜਾਈ()
                                          ? ਨਵਾਂ AstExtend{lsb2p->fileline(), lsb2p}
                                          : lsb2p;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = widep->ਚੌੜਾਈ() > lsb1p->ਚੌੜਾਈ()
                                          ? ਨਵਾਂ AstExtend{lsb1p->fileline(), lsb1p}
                                          : lsb1p;
            lhsp->dtypeFrom(widep);
            rhsp->dtypeFrom(widep);
            newlsbp = ਨਵਾਂ Ast_ਜੋੜੋ{lsb1p->fileline(), lhsp, rhsp};
            newlsbp->dtypeFrom(widep);
        }
        AstSel* ਅਚਲ newp = ਨਵਾਂ AstSel{nodep->fileline(), fromp, newlsbp, nodep->ਚੌੜਾਈ_Const()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ replaceSelConcat(AstSel* nodep) {
        // SEL(CONCAT(a,b),c,d) => SEL(a or b, . .)
        AstConcat* ਅਚਲ conp = VN_AS(nodep->fromp(), Concat);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ conLhsp = conp->lhsp();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ conRhsp = conp->rhsp();
        ਜੇ (static_cast<ਪੂਰਨ>(nodep->lsbConst()) >= conRhsp->ਚੌੜਾਈ()) {
            conLhsp->unlink_Fr_ਪਿੱਠ();
            AstSel* ਅਚਲ newp
                = ਨਵਾਂ AstSel{nodep->fileline(), conLhsp, nodep->lsbConst() - conRhsp->ਚੌੜਾਈ(),
                             nodep->ਚੌੜਾਈ_Const()};
            nodep->replaceWithKeepDType(newp);
        } ਨਹੀਂਤਾਂ ਜੇ (static_cast<ਪੂਰਨ>(nodep->msbConst()) < conRhsp->ਚੌੜਾਈ()) {
            conRhsp->unlink_Fr_ਪਿੱਠ();
            AstSel* ਅਚਲ newp
                = ਨਵਾਂ AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(), nodep->ਚੌੜਾਈ_Const()};
            nodep->replaceWithKeepDType(newp);
        } ਨਹੀਂਤਾਂ {
            // Yuk, split between the two
            conRhsp->unlink_Fr_ਪਿੱਠ();
            conLhsp->unlink_Fr_ਪਿੱਠ();
            AstConcat* ਅਚਲ newp
                = ਨਵਾਂ AstConcat{nodep->fileline(),
                                ਨਵਾਂ AstSel{nodep->fileline(), conLhsp, 0,
                                           nodep->msbConst() - conRhsp->ਚੌੜਾਈ() + 1},
                                ਨਵਾਂ AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(),
                                           conRhsp->ਚੌੜਾਈ() - nodep->lsbConst()}};
            nodep->replaceWithKeepDType(newp);
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Sel_Replicate(AstSel* nodep) {
        // SEL(REPLICATE(from,rep),lsb,width) => SEL(from,0,width) as long
        // as SEL's width <= b's width
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ਅਚਲ repp = VN_AS(nodep->fromp(), Replicate);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = repp->srcp();
        ਅਚਲ AstConst* ਅਚਲ lsbp = VN_CAST(nodep->lsbp(), Const);
        ਜੇ (!lsbp) ਵਾਪਸ ਝੂਠ;
        UASSERT_OBJ(fromp->ਚੌੜਾਈ(), nodep, "Not widthed");
        ਜੇ ((lsbp->ਮੰਜ਼ਿਲ_UInt() / fromp->ਚੌੜਾਈ())
            != ((lsbp->ਮੰਜ਼ਿਲ_UInt() + nodep->ਚੌੜਾਈ() - 1) / fromp->ਚੌੜਾਈ())) {
            ਵਾਪਸ ਝੂਠ;
        }
        //
        fromp->unlink_Fr_ਪਿੱਠ();
        AstSel* ਅਚਲ newp = ਨਵਾਂ AstSel{
            nodep->fileline(), fromp,
            ਨਵਾਂ AstConst{lsbp->fileline(), lsbp->ਮੰਜ਼ਿਲ_UInt() % fromp->ਚੌੜਾਈ()}, nodep->ਚੌੜਾਈ_Const()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Rep_Rep(AstReplicate* nodep) {
        // REPLICATE(REPLICATE2(from2,cnt2),cnt1) => REPLICATE(from2,(cnt1+cnt2))
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ਅਚਲ rep2p = VN_AS(nodep->srcp(), Replicate);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ from2p = rep2p->srcp();
        ਅਚਲ AstConst* ਅਚਲ cnt1p = VN_CAST(nodep->countp(), Const);
        ਜੇ (!cnt1p) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstConst* ਅਚਲ cnt2p = VN_CAST(rep2p->countp(), Const);
        ਜੇ (!cnt2p) ਵਾਪਸ ਝੂਠ;
        //
        from2p->unlink_Fr_ਪਿੱਠ();
        AstReplicate* ਅਚਲ newp
            = ਨਵਾਂ AstReplicate{nodep->fileline(), from2p, cnt1p->ਮੰਜ਼ਿਲ_UInt() * cnt2p->ਮੰਜ਼ਿਲ_UInt()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ ਸੰਕਾਰਜ_Concat_Same(AstConcat* nodep) {
        // CONCAT(fromp,fromp) -> REPLICATE(fromp,1+1)
        // CONCAT(REP(fromp,cnt1),fromp) -> REPLICATE(fromp,cnt1+1)
        // CONCAT(fromp,REP(fromp,cnt1)) -> REPLICATE(fromp,1+cnt1)
        // CONCAT(REP(fromp,cnt1),REP(fromp,cnt2)) -> REPLICATE(fromp,cnt1+cnt2)
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* from1p = nodep->lhsp();
        uint32_t cnt1 = 1;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* from2p = nodep->rhsp();
        uint32_t cnt2 = 1;
        ਜੇ (VN_IS(from1p, Replicate)) {
            ਅਚਲ AstConst* ਅਚਲ cnt1p = VN_CAST(VN_CAST(from1p, Replicate)->countp(), Const);
            ਜੇ (!cnt1p) ਵਾਪਸ ਝੂਠ;
            from1p = VN_AS(from1p, Replicate)->srcp();
            cnt1 = cnt1p->ਮੰਜ਼ਿਲ_UInt();
        }
        ਜੇ (VN_IS(from2p, Replicate)) {
            ਅਚਲ AstConst* ਅਚਲ cnt2p = VN_CAST(VN_CAST(from2p, Replicate)->countp(), Const);
            ਜੇ (!cnt2p) ਵਾਪਸ ਝੂਠ;
            from2p = VN_AS(from2p, Replicate)->srcp();
            cnt2 = cnt2p->ਮੰਜ਼ਿਲ_UInt();
        }
        ਜੇ (!operandsSame(from1p, from2p)) ਵਾਪਸ ਝੂਠ;
        //
        from1p->unlink_Fr_ਪਿੱਠ();
        AstReplicate* ਅਚਲ newp = ਨਵਾਂ AstReplicate{nodep->fileline(), from1p, cnt1 + cnt2};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        ਵਾਪਸ ਸੱਚ;
    }
    ਖਾਲੀ replaceSelIntoBiop(AstSel* nodep) {
        // SEL(BUFIF1(a,b),1,bit) => BUFIF1(SEL(a,1,bit),SEL(b,1,bit))
        Ast_ਗੰਢ_Biop* ਅਚਲ fromp = VN_AS(nodep->fromp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_Biop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lsbp = nodep->lsbp()->unlink_Fr_ਪਿੱਠ();
        //
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bilhsp = fromp->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ birhsp = fromp->rhsp()->unlink_Fr_ਪਿੱਠ();
        //
        fromp->lhsp(
            ਨਵਾਂ AstSel{nodep->fileline(), bilhsp, lsbp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ), nodep->ਚੌੜਾਈ_Const()});
        fromp->rhsp(ਨਵਾਂ AstSel{nodep->fileline(), birhsp, lsbp, nodep->ਚੌੜਾਈ_Const()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ replaceSelIntoUniop(AstSel* nodep) {
        // SEL(NOT(a),1,bit) => NOT(SEL(a,bit))
        Ast_ਗੰਢ_Uniop* ਅਚਲ fromp = VN_AS(nodep->fromp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_Uniop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lsbp = nodep->lsbp()->unlink_Fr_ਪਿੱਠ();
        //
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bilhsp = fromp->lhsp()->unlink_Fr_ਪਿੱਠ();
        //
        fromp->lhsp(ਨਵਾਂ AstSel{nodep->fileline(), bilhsp, lsbp, nodep->ਚੌੜਾਈ_Const()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Attr_ਦਾ* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstArraySel* nodep) override {
        iterate_And_ਅਗਲਾ_Null(nodep->bitp());
        ਜੇ (VN_IS(nodep->bitp(), Const)
            && VN_IS(nodep->fromp(), VarRef)
            // Need to make sure it's an array object so don't mis-allow a constant (bug509.)
            && VN_AS(nodep->fromp(), VarRef)->varp()
            && VN_IS(VN_AS(nodep->fromp(), VarRef)->varp()->valuep(), ConsArray)) {
            m_selp = nodep;  // Ask visit(AstVarRef) to replace varref with const
        }
        iterate_And_ਅਗਲਾ_Null(nodep->fromp());
        ਜੇ (VN_IS(nodep->fromp(), Const)) {  // It did.
            ਜੇ (!m_selp) {
                nodep->v3error("Illegal assignment of constant to unpacked array");
            } ਨਹੀਂਤਾਂ {
                Ast_ਗੰਢ* ਅਚਲ fromp = nodep->fromp()->unlink_Fr_ਪਿੱਠ();
                nodep->replaceWithKeepDType(fromp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            }
        }
        // Handle ARRAYSEL directly on ConsArray (not through VarRef)
        ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ConsArray)) {
            ਅਚਲ AstConsArray* ਅਚਲ consp = VN_AS(nodep->fromp(), ConsArray);
            ਅਚਲ uint32_t ਦੋਆਧਾਰੀ_ਅੰਕ_2 = VN_AS(nodep->bitp(), Const)->ਮੰਜ਼ਿਲ_UInt();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ itemp = consp->valuep(ਦੋਆਧਾਰੀ_ਅੰਕ_2);
            ਜੇ (VN_IS(itemp, Const)) {
                ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ = VN_AS(itemp, Const)->ਅੰਕ();
                VL_DO_DANGLING(replace_ਅੰਕ(nodep, ਅੰਕ), nodep);
            } ਨਹੀਂਤਾਂ ਜੇ (m_params && itemp) {
                // An aggregate element, e.g. a structure in an array parameter
                nodep->replaceWithKeepDType(itemp->clone_ਰੁੱਖ(ਝੂਠ));
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            }
        }
        m_selp = ਖਾਲੀ_ਸੂਚਕ;
    }

    // Evaluate a slice of an unpacked array.  If constantification is
    // required (m_required=true), call replaceWithSimulation() to compute
    // the slice via simulation.  Otherwise just iterate the children.
    ਖਾਲੀ ਦੇਖੋ(Ast_ਟੁਕੜਾ_Sel* nodep) override {
        // First constify or width any child nodes
        iterateChildren(nodep);
        ਜੇ (!m_required) ਵਾਪਸ;  // Do nothing unless we are in parameter mode
        // Fallback to simulation: this will invoke SimulateVisitor::visit(AstSliceSel*)
        replaceWithSimulation(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstStructSel* nodep) override {
        iterateChildren(nodep);

        ਜੇ (ਅਚਲ AstConsStruct* ਅਚਲ consp = VN_CAST(nodep->fromp(), ConsStruct)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = consp->memberValuep(nodep->name());
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(valuep, Const)) {
                VL_DO_DANGLING(replace_ਅੰਕ(nodep, constp->ਅੰਕ()), nodep);
            } ਨਹੀਂਤਾਂ {
                nodep->replaceWith(valuep->clone_ਰੁੱਖ(ਝੂਠ));
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstCAwait* nodep) override {
        m_hasJumpDelay = ਸੱਚ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Not linked");
        ਤਾਰਕਿਕ did = ਝੂਠ;
        ਜੇ (m_doV && (!nodep->varp()->const_Pool_ਇੰਦਰਾਜ() || m_selp) && nodep->varp()->valuep()
            && !m_attrp) {
            // UINFOTREE(1, valuep, "", "visitvaref");
            iterate_And_ਅਗਲਾ_Null(nodep->varp()->valuep());  // May change nodep->varp()->valuep()
            Ast_ਗੰਢ* ਅਚਲ valuep = nodep->varp()->valuep();
            // The selected element, when under a constant select of an array
            ਅਚਲ AstConsArray* ਅਚਲ consp = VN_CAST(valuep, ConsArray);
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ selItemp
                = (m_selp && consp) ? consp->valuep(m_selp->ਦੋਆਧਾਰੀ_ਅੰਕ_Const()) : ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (nodep->access().isReadOnly() && valuep
                && ((!m_params  // Can reduce constant wires into equations
                     && m_doNConst && v3Global.opt.fConst() && nodep->varp()->isConst())
                    || nodep->varp()->isParam())) {
                ਜੇ (ਸੰਕਾਰਜ_Const(valuep)) {
                    ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ = VN_AS(valuep, Const)->ਅੰਕ();
                    // UINFO(2, "constVisit " << cvtToHex(valuep) << " " << num);
                    ਅਚਲ string origParamName
                        = nodep->varp()->isParam() ? nodep->varp()->name() : "";
                    VL_DO_DANGLING(replace_ਅੰਕ(nodep, ਅੰਕ, origParamName), nodep);
                    did = ਸੱਚ;
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstConst* ਅਚਲ itemp = VN_CAST(selItemp, Const)) {
                    VL_DO_DANGLING(replace_ਅੰਕ(nodep, itemp->ਅੰਕ()), nodep);
                    did = ਸੱਚ;
                } ਨਹੀਂਤਾਂ ਜੇ (m_params && consp) {
                    // Allow parameters to pass arrays
                    // Earlier recursion of ConsArray made sure each array value is constant
                    // This exception is fairly fragile, i.e. doesn't
                    // support arrays of arrays or other stuff
                    Ast_ਗੰਢ* ਅਚਲ newp = valuep->clone_ਰੁੱਖ(ਝੂਠ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    did = ਸੱਚ;
                } ਨਹੀਂਤਾਂ ਜੇ (m_params && VN_IS(valuep, ConsStruct)) {
                    Ast_ਗੰਢ* ਅਚਲ newp = valuep->clone_ਰੁੱਖ(ਝੂਠ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    did = ਸੱਚ;
                } ਨਹੀਂਤਾਂ ਜੇ (nodep->varp()->isParam() && VN_IS(valuep, Unbounded)) {
                    Ast_ਗੰਢ* ਅਚਲ newp = valuep->clone_ਰੁੱਖ(ਝੂਠ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    did = ਸੱਚ;
                }
            }
        }
        ਜੇ (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but variable isn't const: "
                           << nodep->varp()->prettyNameQ());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਅੰਜਕ_Stmt* nodep) override {
        iterateChildren(nodep);
        ਜੇ (!Ast_ਗੰਢ::afterCommentp(nodep->stmtsp())) {
            deleteVarScopesUnder(nodep->stmtsp());
            UINFO(8, "ExprStmt(...) " << nodep << " " << nodep->resultp());
            nodep->replaceWith(nodep->resultp()->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            // Removing the ExprStmt might have made something impure above now pure
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Enum_ਵਸਤੂ_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        ਤਾਰਕਿਕ did = ਝੂਠ;
        ਜੇ (nodep->itemp()->valuep()) {
            // UINFOTREE(1, nodep->itemp()->valuep(), "", "visitvaref");
            ਅਚਲ Ast_Enum_ਵਸਤੂ* ਅਚਲ itemp = nodep->itemp();
            ਜੇ (m_recursing_Enum_ਚੀਜ਼ਾਂ.ਗਿਣਤੀ_2(itemp)) {
                nodep->v3error("Recursive enum value: " << nodep->itemp()->prettyNameQ());
            } ਨਹੀਂਤਾਂ {
                m_recursing_Enum_ਚੀਜ਼ਾਂ.emplace(itemp);
                iterate_And_ਅਗਲਾ_Null(nodep->itemp()->valuep());
                m_recursing_Enum_ਚੀਜ਼ਾਂ.erase(itemp);
            }
            ਜੇ (AstConst* ਅਚਲ valuep = VN_CAST(nodep->itemp()->valuep(), Const)) {
                ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ = valuep->ਅੰਕ();
                VL_DO_DANGLING(replace_ਅੰਕ(nodep, ਅੰਕ), nodep);
                did = ਸੱਚ;
            }
        }
        ਜੇ (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but enum value isn't const: "
                           << nodep->itemp()->prettyNameQ());
        }
    }

    ਤਾਰਕਿਕ only_Sen_ਵਸਤੂ_ਵਿੱਚ_Sen_ਰੁੱਖ(ਅਚਲ Ast_Sen_ਵਸਤੂ* nodep) {
        // Only one if it's not in a list
        ਵਾਪਸ (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਵਸਤੂ* nodep) override {
        iterateChildren(nodep);
        ਜੇ (m_doNConst
            && !v3Global.opt.timing().is_ਸੈੱਟ_True()  // If --timing, V3Sched would turn this into an
                                                   // infinite loop. See #5080
            && (VN_IS(nodep->sensp(), Const) || VN_IS(nodep->sensp(), Enum_ਵਸਤੂ_Ref)
                || (nodep->varrefp() && nodep->varrefp()->varp()->isParam()))) {
            // Constants in sensitivity lists may be removed (we'll simplify later)
            ਜੇ (nodep->isClocked()) {  // A constant can never get a pos/negedge
                ਜੇ (only_Sen_ਵਸਤੂ_ਵਿੱਚ_Sen_ਰੁੱਖ(nodep)) {
                    ਜੇ (nodep->ਕਿਨਾਰਾ_Type() == VEdgeType::ET_ਬਦਲਿਆ) {
                        // TODO: This really is dodgy, as strictly compliant simulators will not
                        //       execute this block, but but t_func_check relies on it
                        nodep->replaceWith(
                            ਨਵਾਂ Ast_Sen_ਵਸਤੂ{nodep->fileline(), Ast_Sen_ਵਸਤੂ::Initial{}});
                    } ਨਹੀਂਤਾਂ {
                        nodep->replaceWith(ਨਵਾਂ Ast_Sen_ਵਸਤੂ{nodep->fileline(), Ast_Sen_ਵਸਤੂ::Never{}});
                    }
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                } ਨਹੀਂਤਾਂ {
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                }
            } ਨਹੀਂਤਾਂ {  // Otherwise it may compute a result that needs to settle out
                nodep->replaceWith(ਨਵਾਂ Ast_Sen_ਵਸਤੂ{nodep->fileline(), Ast_Sen_ਵਸਤੂ::Combo{}});
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            }
        } ਨਹੀਂਤਾਂ ਜੇ (m_doNConst && VN_IS(nodep->sensp(), Not)) {
            // V3Gate may propagate NOTs into clocks... Just deal with it
            Ast_ਗੰਢ* ਅਚਲ sensp = nodep->sensp();
            Ast_ਗੰਢ* ਆਖਰੀ_Sensp = sensp;
            ਤਾਰਕਿਕ invert = ਝੂਠ;
            ਜਦੋਂ (VN_IS(ਆਖਰੀ_Sensp, Not)) {
                ਆਖਰੀ_Sensp = VN_AS(ਆਖਰੀ_Sensp, Not)->lhsp();
                invert = !invert;
            }
            UINFO(8, "senItem(NOT...) " << nodep << " " << invert);
            ਜੇ (invert) nodep->ਕਿਨਾਰਾ_Type(nodep->ਕਿਨਾਰਾ_Type().invert());
            sensp->replaceWith(ਆਖਰੀ_Sensp->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(sensp), sensp);
        }
    }

    ਵਰਗ Sen_ਵਸਤੂ_Cmp final {
        ਸਥਿਰ ਪੂਰਨ cmp(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ap, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* bp) {
            ਅਚਲ VNType aType = ap->type();
            ਅਚਲ VNType bType = bp->type();
            ਜੇ (aType != bType) ਵਾਪਸ static_cast<ਪੂਰਨ>(bType) - static_cast<ਪੂਰਨ>(aType);

            ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ aRefp = VN_CAST(ap, VarRef)) {
                ਅਚਲ AstVarRef* ਅਚਲ bRefp = VN_AS(bp, VarRef);
                // Looks visually better if we keep sorted by name
                ਜੇ (aRefp->name() < bRefp->name()) ਵਾਪਸ -1;
                ਜੇ (aRefp->name() > bRefp->name()) ਵਾਪਸ 1;
                // But might be same name with different scopes
                ਜੇ (aRefp->varScopep() < bRefp->varScopep()) ਵਾਪਸ -1;
                ਜੇ (aRefp->varScopep() > bRefp->varScopep()) ਵਾਪਸ 1;
                // Or rarely, different data types
                ਜੇ (aRefp->dtypep() < bRefp->dtypep()) ਵਾਪਸ -1;
                ਜੇ (aRefp->dtypep() > bRefp->dtypep()) ਵਾਪਸ 1;
                ਵਾਪਸ 0;
            }

            ਜੇ (ਅਚਲ AstConst* ਅਚਲ aConstp = VN_CAST(ap, Const)) {
                ਅਚਲ AstConst* ਅਚਲ bConstp = VN_AS(bp, Const);
                ਜੇ (aConstp->ਅੰਕ().isLtXZ(bConstp->ਅੰਕ())) ਵਾਪਸ -1;
                ਜੇ (bConstp->ਅੰਕ().isLtXZ(aConstp->ਅੰਕ())) ਵਾਪਸ 1;
                ਵਾਪਸ 0;
            }

            ਜੇ (ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ aBiOpp = VN_CAST(ap, ਗੰਢ_Biop)) {
                ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ bBiOpp = VN_AS(bp, ਗੰਢ_Biop);
                // Compare RHSs first, as LHS might be const
                ਜੇ (ਅਚਲ ਪੂਰਨ c = cmp(aBiOpp->rhsp(), bBiOpp->rhsp())) ਵਾਪਸ c;
                ਵਾਪਸ cmp(aBiOpp->lhsp(), bBiOpp->lhsp());
            }

            ਜੇ (ਅਚਲ AstCMethodHard* ਅਚਲ aCallp = VN_CAST(ap, CMethodHard)) {
                ਅਚਲ AstCMethodHard* ਅਚਲ bCallp = VN_AS(bp, CMethodHard);
                ਜੇ (aCallp->ਵਿਧੀ() < bCallp->ਵਿਧੀ()) ਵਾਪਸ -1;
                ਜੇ (aCallp->ਵਿਧੀ() > bCallp->ਵਿਧੀ()) ਵਾਪਸ 1;
                ਜੇ (ਅਚਲ ਪੂਰਨ c = cmp(aCallp->fromp(), bCallp->fromp())) ਵਾਪਸ c;
                ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* aPinsp = aCallp->pinsp();
                ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* bPinsp = bCallp->pinsp();
                ਜਦੋਂ (aPinsp && bPinsp) {
                    ਜੇ (ਅਚਲ ਪੂਰਨ c = cmp(aPinsp, bPinsp)) ਵਾਪਸ c;
                    aPinsp = VN_AS(aPinsp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ);
                    bPinsp = VN_AS(bPinsp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ);
                }
                ਵਾਪਸ aPinsp ? -1 : bPinsp ? 1 : 0;
            }

            ਵਾਪਸ 0;
        }

    ਜਨਤਕ:
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ Ast_Sen_ਵਸਤੂ* lhsp, ਅਚਲ Ast_Sen_ਵਸਤੂ* rhsp) ਅਚਲ {
            ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lSensp = lhsp->sensp();
            ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rSensp = rhsp->sensp();
            ਜੇ (lSensp && rSensp) {
                // If both terms have sensitivity expressions, recursively compare them
                ਜੇ (ਅਚਲ ਪੂਰਨ c = cmp(lSensp, rSensp)) ਵਾਪਸ c < 0;
            } ਨਹੀਂਤਾਂ ਜੇ (lSensp || rSensp) {
                // Terms with sensitivity expressions come after those without
                ਵਾਪਸ rSensp;
            }
            // Finally sort by edge, AFTER variable, as we want multiple edges for same var
            // adjacent. note the SenTree optimizer requires this order (more general first,
            // less general last)
            ਵਾਪਸ lhsp->ਕਿਨਾਰਾ_Type() < rhsp->ਕਿਨਾਰਾ_Type();
        }
    };

    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਰੁੱਖ* nodep) override {
        iterateChildren(nodep);
        ਜੇ (m_doExpensive) {
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
            ਲਈ (Ast_Sen_ਵਸਤੂ *nextp, *senp = nodep->sensesp(); senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), Sen_ਵਸਤੂ);
                // cppcheck-suppress unassignedVariable  // cppcheck bug
                ਅਚਲ Sen_ਵਸਤੂ_Cmp cmp;
                ਜੇ (nextp && !cmp(senp, nextp)) {
                    // Something's out of order, sort it
                    senp = ਖਾਲੀ_ਸੂਚਕ;
                    std::ਸਦਿਸ਼<Ast_Sen_ਵਸਤੂ*> vec;
                    ਲਈ (Ast_Sen_ਵਸਤੂ* senp = nodep->sensesp(); senp;
                         senp = VN_AS(senp->nextp(), Sen_ਵਸਤੂ)) {
                        vec.ਧੱਕੋ_ਪਿੱਠ(senp);
                    }
                    stable_ਛਾਂਟੋ(vec.begin(), vec.ਸਿਰਾ(), Sen_ਵਸਤੂ_Cmp());
                    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ip : vec) ip->unlink_Fr_ਪਿੱਠ();
                    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ip : vec) nodep->ਜੋੜੋ_Sensesp(ip);
                    ਰੋਕੋ;
                }
            }

            // Pass 2, remove duplicates and simplify adjacent terms if possible
            ਲਈ (Ast_Sen_ਵਸਤੂ *senp = nodep->sensesp(), *nextp; senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), Sen_ਵਸਤੂ);
                ਜੇ (!nextp) ਰੋਕੋ;
                Ast_Sen_ਵਸਤੂ* ਅਚਲ lItemp = senp;
                Ast_Sen_ਵਸਤੂ* ਅਚਲ rItemp = nextp;
                ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lSenp = lItemp->sensp();
                ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rSenp = rItemp->sensp();
                ਜੇ (!lSenp || !rSenp) ਜਾਰੀ;

                ਜੇ (lSenp->same_Gate_ਰੁੱਖ(rSenp)) {
                    // POSEDGE or NEGEDGE -> BOTHEDGE. (We've sorted POSEDGE, before NEGEDGE, so we
                    // do not need to test for the opposite orders.)
                    ਜੇ (lItemp->ਕਿਨਾਰਾ_Type() == VEdgeType::ET_POSEDGE
                        && rItemp->ਕਿਨਾਰਾ_Type() == VEdgeType::ET_NEGEDGE) {
                        // Make both terms BOTHEDGE, the second will be removed below
                        lItemp->ਕਿਨਾਰਾ_Type(VEdgeType::ET_BOTHEDGE);
                        rItemp->ਕਿਨਾਰਾ_Type(VEdgeType::ET_BOTHEDGE);
                    }

                    // Remove identical expressions
                    ਜੇ (lItemp->ਕਿਨਾਰਾ_Type() == rItemp->ਕਿਨਾਰਾ_Type()) {
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(rItemp->unlink_Fr_ਪਿੱਠ()), rItemp);
                        nextp = lItemp;
                    }
                }
            }
        }
    }

    //-----
    // Zero elimination
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        iterateChildren(nodep);
        ਜੇ (nodep->timingControlp()) m_hasJumpDelay = ਸੱਚ;
        ਜੇ (m_doNConst && replace_ਗੰਢ_ਸੌਂਪੋ(nodep)) ਵਾਪਸ;
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlias* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    ਖਾਲੀ ਦੇਖੋ(AstAliasScope* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_W* nodep) override {
        iterateChildren(nodep);
        ਜੇ (m_doNConst && replace_ਗੰਢ_ਸੌਂਪੋ(nodep)) ਵਾਪਸ;
        // Process containing this AssignW as single body statement
        AstAlways* ਅਚਲ procp = VN_CAST(nodep->backp(), Always);
        ਜੇ (!procp || procp->stmtsp() != nodep || nodep->nextp()) ਵਾਪਸ;
        // Not VarXRef, as different refs may set different values to each hierarchy
        Ast_ਗੰਢ_Var_Ref* ਅਚਲ varrefp = VN_CAST(nodep->lhsp(), VarRef);
        ਜੇ (m_wremove && !m_params && m_doNConst && m_modp && ਸੰਕਾਰਜ_Const(nodep->rhsp())
            && !VN_AS(nodep->rhsp(), Const)->ਅੰਕ().isFourState()
            && varrefp  // Don't do messes with BITREFs/ARRAYREFs
            && !varrefp->varp()->hasStrengthAssignment()  // Strengths are resolved in V3Tristate
            && !varrefp->varp()->valuep()  // Not already constified
            && !varrefp->varScopep()  // Not scoped (or each scope may have different initial val.)
            && !varrefp->varp()->isForced()  // Not forced (not really a constant)
        ) {
            // ASSIGNW (VARREF, const) -> INITIAL ( ASSIGN (VARREF, const) )
            UINFO(4, "constAssignW " << nodep);
            // Make a initial assignment
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            varrefp->unlink_Fr_ਪਿੱਠ();
            File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
            AstInitial* ਅਚਲ newinitp = ਨਵਾਂ AstInitial{flp, ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, varrefp, exprp}};
            procp->replaceWith(newinitp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(procp), procp);
            // Set the initial value right in the variable so we can constant propagate
            Ast_ਗੰਢ* ਅਚਲ initvaluep = exprp->clone_ਰੁੱਖ(ਝੂਠ);
            varrefp->varp()->isConst(ਸੱਚ);
            varrefp->varp()->valuep(initvaluep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCReset* nodep) override {
        iterateChildren(nodep);
        ਜੇ (!m_doNConst) ਵਾਪਸ;
        ਅਚਲ AstBasicDType* ਅਚਲ bdtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType);
        ਜੇ (!bdtypep) ਵਾਪਸ;
        ਜੇ (!bdtypep->isZeroInit()) ਵਾਪਸ;
        AstConst* ਅਚਲ newp = ਨਵਾਂ AstConst{nodep->fileline(), V3_ਸੰਖਿਆ{nodep, bdtypep}};
        UINFO(9, "CRESET(0) => CONST(0) " << nodep);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack* nodep) override {
        iterateChildren(nodep);
        AstConsArray* ਅਚਲ fromp = VN_CAST(nodep->fromp(), ConsArray);
        ਜੇ (!fromp) ਵਾਪਸ;
        // Fold the outermost dimension, converting the elements for any dimension under it
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ ਮੰਜ਼ਿਲ_Arrayp
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ fromArrayp
            = VN_AS(fromp->dtypep()->skipRefp(), UnpackArrayDType);
        // This dimension may not be reversed, only one under it
        ਅਚਲ ਤਾਰਕਿਕ ਉਲਟ
            = ਮੰਜ਼ਿਲ_Arrayp->declRange().ascending() != fromArrayp->declRange().ascending();
        ਅਚਲ uint64_t ਆਖਰੀ_ਸੂਚਕਾਂਕ = ਮੰਜ਼ਿਲ_Arrayp->elementsConst() - 1;
        Ast_ਗੰਢ_DType* ਅਚਲ subDTypep = ਮੰਜ਼ਿਲ_Arrayp->subDTypep();
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ subArrayp
            = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
        ਅਚਲ ਸਵੈਚਾਲਿਤ convert = [&](Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
            valuep->unlink_Fr_ਪਿੱਠ();
            ਜੇ (!subArrayp || !subArrayp->is_ਉਲਟ_ਦਾ(valuep->dtypep())) ਵਾਪਸ valuep;
            ਵਾਪਸ ਨਵਾਂ Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack{valuep->fileline(), valuep, subDTypep};
        };
        AstConsArray* ਅਚਲ newp
            = ਨਵਾਂ AstConsArray{fromp->fileline(), nodep->dtypep(),
                               fromp->defaultp() ? convert(fromp->defaultp()) : ਖਾਲੀ_ਸੂਚਕ};
        ਲਈ (ਅਚਲ Ast_Cons_Array_ਵਸਤੂ* itemp = fromp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_ਵਸਤੂ)) {
            newp->valuep(ਉਲਟ ? ਆਖਰੀ_ਸੂਚਕਾਂਕ - itemp->ਸੂਚਕਾਂਕ() : itemp->ਸੂਚਕਾਂਕ(),
                         convert(itemp->valuep()));
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        iterate(newp);  // Fold the conversions of the elements
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Array* nodep) override {
        iterateChildren(nodep);
        // Handle the case where we have a stream operation inside a cast conversion
        // Check for both StreamL and StreamR operations
        Ast_ਗੰਢ_ਧਾਰਾ* streamp = ਖਾਲੀ_ਸੂਚਕ;
        ਤਾਰਕਿਕ is_ਉਲਟ = ਝੂਠ;
        ਜੇ (Ast_ਧਾਰਾ_L* ਅਚਲ ਧਾਰਾ_Lp = VN_CAST(nodep->fromp(), ਧਾਰਾ_L)) {
            streamp = ਧਾਰਾ_Lp;
            is_ਉਲਟ = ਸੱਚ;  // StreamL reverses the operation
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਧਾਰਾ_R* ਅਚਲ ਧਾਰਾ_Rp = VN_CAST(nodep->fromp(), ਧਾਰਾ_R)) {
            streamp = ਧਾਰਾ_Rp;
            is_ਉਲਟ = ਝੂਠ;  // StreamR doesn't reverse the operation
        }
        ਜੇ (streamp) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* srcp = streamp->lhsp();
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ srcDTypep = srcp->dtypep()->skipRefp();
            Ast_ਗੰਢ_DType* ਅਚਲ dstDTypep = nodep->dtypep()->skipRefp();
            ਜੇ (VN_IS(srcDTypep, ਕਤਾਰ_DType) && VN_IS(dstDTypep, ਕਤਾਰ_DType)) {
                ਪੂਰਨ ਖੰਡ_ਮਾਪ = 1;
                ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(streamp->rhsp(), Const)) {
                    ਖੰਡ_ਮਾਪ = constp->ਮੰਜ਼ਿਲ_SInt();
                    ਜੇ (VL_UNLIKELY(ਖੰਡ_ਮਾਪ <= 0)) {
                        // Not reachable due to higher level checks when parsing stream
                        // operators commented out to not fail v3error-coverage-checks.
                        // nodep->v3error("Stream block size must be positive, got " <<
                        // blockSize);
                        ਖੰਡ_ਮਾਪ = 1;
                    }
                }
                // Not reachable due to higher level checks when parsing stream operators
                // commented out to not fail v3error-coverage-checks.
                // else {
                //    nodep->v3error("Stream block size must be constant (got " <<
                //    streamp->rhsp()->prettyTypeName() << ")");
                // }
                ਪੂਰਨ src_Element_ਬਿੱਟ = 0;
                ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = srcDTypep->subDTypep()) {
                    src_Element_ਬਿੱਟ = elemDtp->ਚੌੜਾਈ();
                }
                ਪੂਰਨ dst_Element_ਬਿੱਟ = 0;
                ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = dstDTypep->subDTypep()) {
                    dst_Element_ਬਿੱਟ = elemDtp->ਚੌੜਾਈ();
                }
                streamp->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਨਵਾਂ Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Array{
                    srcp->fileline(), srcp->unlink_Fr_ਪਿੱਠ(), dstDTypep,     is_ਉਲਟ,
                    ਖੰਡ_ਮਾਪ,        dst_Element_ਬਿੱਟ,       src_Element_ਬਿੱਟ};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(streamp), streamp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ;
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstRelease* nodep) override {
        // cppcheck-suppress constVariablePointer // children unlinked below
        ਜੇ (AstConcat* ਅਚਲ concatp = VN_CAST(nodep->lhsp(), Concat)) {
            File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
            AstRelease* ਅਚਲ newLp = ਨਵਾਂ AstRelease{flp, concatp->lhsp()->unlink_Fr_ਪਿੱਠ()};
            AstRelease* ਅਚਲ newRp = ਨਵਾਂ AstRelease{flp, concatp->rhsp()->unlink_Fr_ਪਿੱਠ()};
            nodep->replaceWith(newLp);
            newLp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newRp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਦੇਖੋ(newLp);
            ਦੇਖੋ(newRp);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        iterateChildren(nodep);
        ਜੇ (m_doNConst) {
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->condp(), Const)) {
                Ast_ਗੰਢ* keepp = ਖਾਲੀ_ਸੂਚਕ;
                Ast_ਗੰਢ* delp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (constp->isZero()) {
                    UINFO(4, "IF(0,{any},{x}) => {x}: " << nodep);
                    keepp = nodep->elsesp();
                    delp = nodep->thensp();
                } ਨਹੀਂਤਾਂ ਜੇ (!m_doV || constp->isNeqZero()) {  // Might be X in Verilog
                    UINFO(4, "IF(!0,{x},{any}) => {x}: " << nodep);
                    keepp = nodep->thensp();
                    delp = nodep->elsesp();
                } ਨਹੀਂਤਾਂ {
                    UINFO(4, "IF condition is X, retaining: " << nodep);
                    ਵਾਪਸ;
                }
                ਜੇ (!canDeleteStatements()) {
                    UINFO(4, "IF retained until statics lifted: " << nodep);
                    ਵਾਪਸ;
                }

                // If we delete a branch that contains variable declarations, also delete the
                // corresponding varscopes so we don't leave dangling AstVarScope::m_varp pointers.
                deleteVarScopesUnder(delp);

                ਜੇ (keepp) {
                    keepp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                    nodep->replaceWith(keepp);
                } ਨਹੀਂਤਾਂ {
                    nodep->unlink_Fr_ਪਿੱਠ();
                    deleteVarScopesUnder(nodep->thensp());
                    deleteVarScopesUnder(nodep->elsesp());
                }
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);

                // Removed branch could contain only impurity within a function and the
                // function could be purified
                VIsCached::clear_Cache_ਰੁੱਖ();
            } ਨਹੀਂਤਾਂ ਜੇ (!Ast_ਗੰਢ::afterCommentp(nodep->thensp())
                       && !Ast_ਗੰਢ::afterCommentp(nodep->elsesp())) {
                ਜੇ (!nodep->condp()->is_ਸ਼ੁੱਧ()) {
                    // Condition has side effect - leave, but don't need return value
                    UINFO(4, "IF({x}) ; => STMTEXPR({x}): " << nodep);
                    nodep->fileline()->warnOff(V3_Error_ਕੋਡ::IGNOREDRETURN, ਸੱਚ);
                    Ast_ਗੰਢ_Stmt* ਅਚਲ newp
                        = ਨਵਾਂ Ast_Stmt_ਵਿਅੰਜਕ{nodep->fileline(), nodep->condp()->unlink_Fr_ਪਿੱਠ()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                } ਨਹੀਂਤਾਂ {
                    // Empty block, remove it
                    UINFO(4, "IF({x}) ; => ; : " << nodep);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                }
            } ਨਹੀਂਤਾਂ ਜੇ (!Ast_ਗੰਢ::afterCommentp(nodep->thensp())) {
                UINFO(4, "IF({x}) nullptr {...} => IF(NOT{x}}: " << nodep);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = nodep->condp();
                Ast_ਗੰਢ* ਅਚਲ elsesp = nodep->elsesp();
                condp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                elsesp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                ਜੇ (nodep->thensp()) {  // Must have been comment
                    ਧੱਕੋ_Deletep(nodep->thensp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                }
                nodep->condp(ਨਵਾਂ AstLogNot{condp->fileline(),
                                           condp});  // LogNot, as C++ optimization also possible
                nodep->ਜੋੜੋ_Thensp(elsesp);
                nodep->branchPred(nodep->branchPred().invert());
            } ਨਹੀਂਤਾਂ ਜੇ (((VN_IS(nodep->condp(), Not) && nodep->condp()->ਚੌੜਾਈ() == 1)
                        || VN_IS(nodep->condp(), LogNot))
                       && nodep->thensp() && nodep->elsesp()) {
                UINFO(4, "IF(NOT {x})  => IF(x) swapped if/else" << nodep);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp
                    = VN_AS(nodep->condp(), ਗੰਢ_Uniop)->lhsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                Ast_ਗੰਢ* ਅਚਲ thensp = nodep->thensp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                Ast_ਗੰਢ* ਅਚਲ elsesp = nodep->elsesp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{nodep->fileline(), condp, elsesp, thensp};
                ifp->isBoundsCheck(nodep->isBoundsCheck());  // Copy bounds check info
                ifp->branchPred(nodep->branchPred().invert());
                nodep->replaceWith(ifp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            } ਨਹੀਂਤਾਂ ਜੇ (if_Same_ਸੌਂਪੋ(nodep)) {
                UINFO(4,
                      "IF({a}) ASSIGN({b},{c}) else ASSIGN({b},{d}) => ASSIGN({b}, {a}?{c}:{d})");
                Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ thensp = VN_AS(nodep->thensp(), ਗੰਢ_ਸੌਂਪੋ);
                // cppcheck-suppress constVariablePointer // children unlinked below
                Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ elsesp = VN_AS(nodep->elsesp(), ਗੰਢ_ਸੌਂਪੋ);
                thensp->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = nodep->condp()->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ truep = thensp->rhsp()->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ falsep = elsesp->rhsp()->unlink_Fr_ਪਿੱਠ();
                thensp->rhsp(ਨਵਾਂ AstCond{truep->fileline(), condp, truep, falsep});
                nodep->replaceWith(thensp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            } ਨਹੀਂਤਾਂ ਜੇ (ਝੂਠ  // Disabled, as vpm assertions are faster
                              // without due to short-circuiting
                       && ਸੰਕਾਰਜ_If_If(nodep)) {
                UINFO(9, "IF({a}) IF({b}) => IF({a} && {b})");
                Ast_ਗੰਢ_If* ਅਚਲ ਹੇਠਲਾ_Ifp = VN_AS(nodep->thensp(), ਗੰਢ_If);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = nodep->condp()->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ* ਅਚਲ ਹੇਠਲਾ_Thensp = ਹੇਠਲਾ_Ifp->thensp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਹੇਠਲਾ_Condp = ਹੇਠਲਾ_Ifp->condp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                nodep->condp(ਨਵਾਂ AstLogAnd{ਹੇਠਲਾ_Ifp->fileline(), condp, ਹੇਠਲਾ_Condp});
                ਹੇਠਲਾ_Ifp->replaceWith(ਹੇਠਲਾ_Thensp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(ਹੇਠਲਾ_Ifp), ਹੇਠਲਾ_Ifp);
            } ਨਹੀਂਤਾਂ {
                // Optimizations that don't reform the IF itself
                ਜੇ (ਸੰਕਾਰਜ_Bool_ਖਿਸਕਾਓ(nodep->condp())) replace_Bool_ਖਿਸਕਾਓ(nodep->condp());
                ਮੇਲ_If_Same_ਆਖਰੀ(nodep);
                ਮੇਲ_If_Cond_Cond(nodep);
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstDisplay* nodep) override {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        iterateChildren(nodep);
        ਜੇ (stmtDisplayDisplay(nodep)) ਵਾਪਸ;
    }
    ਤਾਰਕਿਕ stmtDisplayDisplay(AstDisplay* nodep) {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        ਜੇ (!m_modp) ਵਾਪਸ ਝੂਠ;  // Don't optimize under single statement
        AstDisplay* ਅਚਲ prevp = VN_CAST(nodep->backp(), Display);
        ਜੇ (!prevp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!((prevp->displayType() == nodep->displayType())
              || (prevp->displayType() == VDisplayType::DT_WRITE
                  && nodep->displayType() == VDisplayType::DT_DISPLAY)
              || (prevp->displayType() == VDisplayType::DT_DISPLAY
                  && nodep->displayType() == VDisplayType::DT_WRITE)))
            ਵਾਪਸ ਝੂਠ;
        ਜੇ ((prevp->filep() && !nodep->filep()) || (!prevp->filep() && nodep->filep())
            || (prevp->filep() && nodep->filep() && !prevp->filep()->same_ਰੁੱਖ(nodep->filep())))
            ਵਾਪਸ ਝੂਠ;
        ਜੇ (!prevp->fmtp() || prevp->fmtp()->nextp() || !nodep->fmtp() || nodep->fmtp()->nextp())
            ਵਾਪਸ ਝੂਠ;
        AstSFormatF* ਅਚਲ pformatp = prevp->fmtp();
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSFormatF* ਅਚਲ nformatp = nodep->fmtp();
        // We don't merge scopeNames as can have only one and might be different scopes (late in
        // process) Also rare for real code to print %m multiple times in same message
        ਜੇ (nformatp->scopeNamep() && pformatp->scopeNamep()) ਵਾਪਸ ਝੂਠ;
        // We don't early merge arguments as might need to later print warnings with
        // right line numbers, nor scopeNames as might be different scopes (late in process)
        ਜੇ (!m_doCpp && pformatp->exprsp()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!m_doCpp && nformatp->exprsp()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (pformatp->exprsp() && !pformatp->exprsp()->is_ਸ਼ੁੱਧ_And_ਅਗਲਾ()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (nformatp->exprsp() && !nformatp->exprsp()->is_ਸ਼ੁੱਧ_And_ਅਗਲਾ()) ਵਾਪਸ ਝੂਠ;
        // Avoid huge merges
        ਸਥਿਰ constexpr ਪੂਰਨ DISPLAY_MAX_ਮਿਲਾਓ_ਲੰਮਾਈ = 500;
        ਜੇ (pformatp->ਪਾਠ().ਲੰਮਾਈ() + nformatp->ਪਾਠ().ਲੰਮਾਈ() > DISPLAY_MAX_ਮਿਲਾਓ_ਲੰਮਾਈ)
            ਵਾਪਸ ਝੂਠ;
        //
        UINFO(9, "DISPLAY(SF({a})) DISPLAY(SF({b})) -> DISPLAY(SF({a}+{b}))");
        // Convert DT_DISPLAY to DT_WRITE as may allow later optimizations
        ਜੇ (prevp->displayType() == VDisplayType::DT_DISPLAY) {
            prevp->displayType(VDisplayType::DT_WRITE);
            pformatp->ਪਾਠ(pformatp->ਪਾਠ() + "\n");
        }
        // We can't replace prev() as the edit tracking iterators will get confused.
        // So instead we edit the prev note itself.
        ਜੇ (prevp->ਜੋੜੋ_Newline()) pformatp->ਪਾਠ(pformatp->ਪਾਠ() + "\n");
        pformatp->ਪਾਠ(pformatp->ਪਾਠ() + nformatp->ਪਾਠ());
        ਜੇ (!prevp->ਜੋੜੋ_Newline() && nodep->ਜੋੜੋ_Newline()) pformatp->ਪਾਠ(pformatp->ਪਾਠ() + "\n");
        ਜੇ (nformatp->exprsp()) pformatp->ਜੋੜੋ_Exprsp(nformatp->exprsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        ਜੇ (AstScopeName* ਅਚਲ scopeNamep = nformatp->scopeNamep()) {
            scopeNamep->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            pformatp->scopeNamep(scopeNamep);
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
        ਵਾਪਸ ਸੱਚ;
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormatArg* nodep) override {
        // Skip namep(): requiring its runtime lookup to be constant can reject valid
        // enum-valued constant-function calls in parameters. displayedEnum() resolves
        // the name from the folded exprp() value and enum dtype instead.
        iterate_And_ਅਗਲਾ_Null(nodep->exprp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormatF* nodep) override {
        // Substitute constants into displays.  The main point of this is to
        // simplify assertion methodologies which call functions with display's.
        // This eliminates a pile of wide temps, and makes the C a whole lot more readable.
        iterateChildren(nodep);
        ਜੇ (nodep->ਵਿਅੰਜਕ_Format_2()) {
            // Upconvert to text-based format?
            // Similar code in V3LinkResolve::visit(AstSFormatF)
            UASSERT_OBJ(nodep->ਪਾਠ() == "", nodep,
                        "Non-format $sformatf should have text format == \"\"");
            ਜੇ (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->ਅੰਕ().isFromString()) {
                AstConst* ਅਚਲ fmtp = VN_AS(nodep->exprsp()->unlink_Fr_ਪਿੱਠ(), Const);
                nodep->ਪਾਠ(fmtp->ਅੰਕ().ਮੰਜ਼ਿਲ_String());
                nodep->ਵਿਅੰਜਕ_Format_2(ਝੂਠ);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(fmtp), fmtp);
            }
        }
        ਜੇ (nodep->ਵਿਅੰਜਕ_Format_2()) ਵਾਪਸ;  // Runtime, unfortunately  (unless constify format later)
        ਤਾਰਕਿਕ anyconst = ਝੂਠ;
        ਲਈ (Ast_ਗੰਢ* argp = nodep->exprsp(); argp; argp = argp->nextp()) {
            AstSFormatArg* ਅਚਲ fargp = VN_CAST(argp, SFormatArg);
            Ast_ਗੰਢ* ਅਚਲ subargp = fargp ? fargp->exprp() : argp;
            ਜੇ (VN_IS(subargp, Const)) {
                anyconst = ਸੱਚ;
                ਰੋਕੋ;
            }
        }
        ਜੇ (m_doNConst && anyconst) {
            UINFO(9, "  Display in  " << nodep->ਪਾਠ());
            string newFormat;
            string fmt;
            ਤਾਰਕਿਕ ਵਿੱਚ_Pct = ਝੂਠ;
            Ast_ਗੰਢ* argp = nodep->exprsp();
            ਅਚਲ string ਪਾਠ = nodep->ਪਾਠ();
            ਲਈ (ਅੱਖਰ ch : ਪਾਠ) {
                ਜੇ (!ਵਿੱਚ_Pct && ch == '%') {
                    ਵਿੱਚ_Pct = ਸੱਚ;
                    fmt = ch;
                } ਨਹੀਂਤਾਂ ਜੇ (ਵਿੱਚ_Pct && (std::ਅੰਕ_ਹੈ(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } ਨਹੀਂਤਾਂ ਜੇ (ਵਿੱਚ_Pct) {
                    ਵਿੱਚ_Pct = ਝੂਠ;
                    fmt += ch;
                    ch = std::tolower(ch);
                    ਚੋਣ (ch) {
                    ਹਾਲਤ '%': ਰੋਕੋ;  // %% - still %%
                    ਹਾਲਤ 'm': ਰੋਕੋ;  // %m - still %m - auto insert "name"
                    ਹਾਲਤ 'l': ਰੋਕੋ;  // %l - still %l - auto insert "library"
                    ਹਾਲਤ 't':  // %t - don't know $timeformat so can't constify
                        ਜੇ (argp) argp = argp->nextp();
                        ਰੋਕੋ;
                    ਮੂਲ:  // Most operators, just move to next argument
                        ਜੇ (argp) {
                            Ast_ਗੰਢ* ਅਚਲ nextp = argp->nextp();
                            AstSFormatArg* ਅਚਲ fargp = VN_CAST(argp, SFormatArg);
                            Ast_ਗੰਢ* ਅਚਲ subargp = fargp ? fargp->exprp() : argp;
                            ਅਚਲ VFormatAttr formatAttr
                                = argp
                                      ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                                      : VFormatAttr{};
                            ਜੇ (VN_IS(subargp, Const)) {  // Convert it
                                ਅਚਲ string ਬਾਹਰਲਾ
                                    = formatAttr.isEnum()
                                          ? const_ਅੰਕ_V(subargp).displayedEnum(fargp, fmt)
                                          : const_ਅੰਕ_V(subargp).displayed(nodep, fmt, formatAttr);
                                UINFO(9, "     DispConst: " << fmt << " -> " << ਬਾਹਰਲਾ << "  for "
                                                            << subargp);
                                // fmt = out w/ replace % with %% as it must later when
                                // used as a format output as literal.
                                fmt = VString::quotePercent(ਬਾਹਰਲਾ);
                                VL_DO_DANGLING(ਧੱਕੋ_Deletep(argp->unlink_Fr_ਪਿੱਠ()), argp);
                            }
                            argp = nextp;
                        }
                        ਰੋਕੋ;
                    }  // switch
                    newFormat += fmt;
                } ਨਹੀਂਤਾਂ {
                    newFormat += ch;
                }
            }
            ਜੇ (newFormat != nodep->ਪਾਠ()) {
                nodep->ਪਾਠ(newFormat);
                UINFO(9, "  Display out " << nodep);
            }
        }
        ਜੇ (!nodep->exprsp() && nodep->ਪਾਠ().ਲੱਭੋ('%') == string::npos && !nodep->hidden()) {
            // Just a simple constant string - the formatting is pointless
            UINFO(9, "replaceConstStr");
            VL_DO_DANGLING(replaceConstString(nodep, nodep->ਪਾਠ()), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (VN_IS(nodep->backp(), ਗੰਢ_ਸੌਂਪੋ)  // Can't remove under Display etc
            && nodep->ਪਾਠ() == "%s" && VN_IS(nodep->exprsp(), SFormatArg)
            && VN_AS(nodep->exprsp(), SFormatArg)->formatAttr().isString()
            && !nodep->exprsp()->nextp()) {
            UINFO(9, "  Display(%p, expr) -> constant expr " << nodep);
            nodep->replaceWith(VN_AS(nodep->exprsp(), SFormatArg)->exprp()->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormat* nodep) override {
        iterateChildren(nodep);
        ਜੇ (!m_doNConst) ਵਾਪਸ;
        ਜੇ (m_doNConst) {
            // If it's a constant string written to a string variable, inline it as an assignment
            AstSFormatF* ਅਚਲ fmtp = nodep->fmtp();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp();
            AstBasicDType* ਅਚਲ basicp = VN_CAST(lhsp->dtypep()->skipRefp(), BasicDType);
            ਜੇ (basicp && basicp->isString() && !fmtp->exprsp()
                && fmtp->ਪਾਠ().ਲੱਭੋ('%') == string::npos) {
                AstConst* ਅਚਲ strp
                    = ਨਵਾਂ AstConst{fmtp->fileline(), AstConst::String{}, fmtp->ਪਾਠ()};
                lhsp->unlink_Fr_ਪਿੱਠ();
                nodep->replaceWith(ਨਵਾਂ Ast_ਸੌਂਪੋ{nodep->fileline(), lhsp, strp});
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ;
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        VL_RESTORER(m_underRecFunc);
        ਜੇ (nodep->recursive()) m_underRecFunc = ਸੱਚ;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        iterateChildren(nodep);
        m_hasJumpDelay = ਸੱਚ;  // As don't analyze inside tasks for timing controls
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        // Note excludes AstFuncRef as other visitor below
        iterateChildren(nodep);
        m_hasJumpDelay = ਸੱਚ;  // As don't analyze inside tasks for timing controls
    }
    ਖਾਲੀ ਦੇਖੋ(AstFuncRef* nodep) override {
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ_FTask_Ref*>(nodep));
        ਜੇ (m_params) {  // Only parameters force us to do constant function call propagation
            replaceWithSimulation(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstArg* nodep) override {
        // replaceWithSimulation on the Arg's parent FuncRef replaces these
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override {
        VL_RESTORER(m_hasLoopTest);
        ਅਚਲ ਤਾਰਕਿਕ oldHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = ਝੂਠ;
        m_hasLoopTest = ਝੂਠ;
        iterateChildren(nodep);
        ਅਚਲ ਤਾਰਕਿਕ thisLoopHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = thisLoopHasJumpDelay || oldHasJumpDelay;
        // If the first statement always break, the loop is useless
        ਜੇ (ਅਚਲ AstLoopTest* ਅਚਲ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            ਜੇ (testp->condp()->isZero() && (!testp->nextp() || canDeleteStatements())) {
                nodep->v3warn(UNUSEDLOOP, "Loop condition is always false");
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                ਵਾਪਸ;
            }
        }
        // If last statement always breaks, repalce loop with body
        ਜੇ (Ast_ਗੰਢ* lastp = nodep->stmtsp()) {
            ਜਦੋਂ (Ast_ਗੰਢ* ਅਚਲ nextp = lastp->nextp()) lastp = nextp;
            ਜੇ (AstLoopTest* ਅਚਲ testp = VN_CAST(lastp, LoopTest)) {
                ਜੇ (testp->condp()->isZero()) {
                    VL_DO_DANGLING2(ਧੱਕੋ_Deletep(testp->unlink_Fr_ਪਿੱਠ()), testp, lastp);
                    nodep->replaceWith(nodep->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                }
            }
        }
        // Warn on infinite loop
        ਜੇ (!m_hasLoopTest && !thisLoopHasJumpDelay) {
            nodep->v3warn(INFINITELOOP, "Infinite loop (condition always true)");
            nodep->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::INFINITELOOP, ਸੱਚ);  // Complain once
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoopTest* nodep) override {
        iterateChildren(nodep);

        // If never breaks, remove
        ਜੇ (nodep->condp()->isNeqZero()) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }

        m_hasLoopTest = ਸੱਚ;

        // If always breaks, subsequent statements are dead code, delete them
        ਜੇ (nodep->condp()->isZero()) {
            Ast_ਗੰਢ* ਅਚਲ nextp = nodep->nextp();
            ਜੇ (nextp && canDeleteStatements()) {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nextp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()), nodep);
            }
            ਵਾਪਸ;
        }

        ਜੇ (ਸੰਕਾਰਜ_Bool_ਖਿਸਕਾਓ(nodep->condp())) replace_Bool_ਖਿਸਕਾਓ(nodep->condp());
    }

    ਖਾਲੀ ਦੇਖੋ(AstConsArray* nodep) override { iterateChildren(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstConsStruct* nodep) override { iterateChildren(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cons_Struct_ਵਸਤੂ* nodep) override { iterateChildren(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cons_Array_ਵਸਤੂ* nodep) override { iterateChildren(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstUnbounded* nodep) override { iterateChildren(nodep); }
    // These are converted by V3Param.  Don't constify as we don't want the
    // from() VARREF to disappear, if any.
    // If output of a presel didn't get consted, chances are V3Param didn't visit properly
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Pre_Sel*) override {}

    // Ignored, can eliminate early
    ਖਾਲੀ ਦੇਖੋ(AstSysIgnore* nodep) override {
        iterateChildren(nodep);
        ਜੇ (m_doNConst) VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Stmt_ਵਿਅੰਜਕ* nodep) override {
        iterateChildren(nodep);
        // Malformed due to child being deleted, remove here
        ਜੇ (!nodep->exprp()) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }
        // Remove if expression is trivially pure
        ਜੇ (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), VarRef)) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }
        // Remove calls to empty functions
        ਜੇ (ਅਚਲ AstCCall* ਅਚਲ callp = VN_CAST(nodep->exprp(), CCall)) {
            ਅਚਲ AstCFunc* ਅਚਲ funcp = callp->funcp();
            ਜੇ (!callp->argsp() && funcp->ਖਾਲੀ_ਥਾਵਾਂ_Body()) {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                ਵਾਪਸ;
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
    ਖਾਲੀ ਦੇਖੋ(AstBasicDType* nodep) override {
        iterateChildren(nodep);
        nodep->cvtRangeConst();
    }

    //-----
    // Jump elimination

    ਖਾਲੀ ਦੇਖੋ(AstJumpGo* nodep) override {
        // Any statements following the JumpGo (at this statement level) never execute, delete
        ਜੇ (nodep->nextp() && canDeleteStatements()) {
            ਧੱਕੋ_Deletep(nodep->nextp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        }

        // JumpGo as last statement in target JumpBlock (including last in a last sub-list),
        // is a no-op, remove it.
        ਜੇ (!nodep->nextp()) {
            ਲਈ (Ast_ਗੰਢ* abovep = nodep->abovep(); abovep; abovep = abovep->abovep()) {
                ਜੇ (abovep == nodep->blockp()) {
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                    ਵਾਪਸ;
                }
                // Stop if not doing expensive, or if the above node is not the last in its list,
                // ... or if it's not an 'if' TODO: it would be enough if it was not a branch.
                ਜੇ (!m_doExpensive || abovep->nextp() || !VN_IS(abovep, If)) ਰੋਕੋ;
            }
        }
        // Mark JumpBlock as used
        m_ਵਰਤਿਆ_Jump_ਖੰਡਾਂ.emplace(nodep->blockp());
        m_hasJumpDelay = ਸੱਚ;
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Jump_ਖੰਡ* nodep) override {
        iterateChildren(nodep);

        // If first statement is an AstLoopTest, pull it before the jump block
        ਜੇ (AstLoopTest* ਅਚਲ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(testp->unlink_Fr_ਪਿੱਠ());
        }

        // Remove if empty
        ਜੇ (!nodep->stmtsp()) {
            UINFO(4, "JUMPLABEL => empty " << nodep);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }

        // If no JumpGo points to this node, replace it with its body
        ਜੇ (!m_ਵਰਤਿਆ_Jump_ਖੰਡਾਂ.ਗਿਣਤੀ_2(nodep)) {
            UINFO(4, "JUMPLABEL => unused " << nodep);
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }

    //-----
    // Below lines are magic expressions processed by astgen
    //  TREE_SKIP_VISIT("AstNODETYPE")    # Rename normal visit to visitGen and don't iterate
    //-----
    // clang-format off
    ਰੁੱਖ_SKIP_ਦੇਖੋ("ArraySel");
    ਰੁੱਖ_SKIP_ਦੇਖੋ("CAwait");

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
    //ਲੜੀਆਂ
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
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        // Default: Just iterate
        ਜੇ (m_required) {
            ਜੇ (VN_IS(nodep, ਗੰਢ_DType) || VN_IS(nodep, Range) || VN_IS(nodep, ਟੁਕੜਾ_Sel)
                || VN_IS(nodep, Dot) || VN_IS(nodep, ਪਾਠ_2)) {
                // ignore
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, ਨਮੂਨਾ) || VN_IS(nodep, PatMember)) {
                // A parameter override pattern is typed later, so only fold its members
                iterateChildren(nodep);
            } ਨਹੀਂਤਾਂ ਜੇ (AstCellRef* ਅਚਲ crp = VN_CAST(nodep, CellRef)) {
                iterate(crp->exprp());
                ਜੇ (Ast_ਗੰਢ* ਅਚਲ newp = crp->exprp()) {
                    crp->replaceWithKeepDType(newp->unlink_Fr_ਪਿੱਠ());
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(crp), crp);
                }
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ {
                nodep->v3error("Expecting expression to be constant, but can't convert a "
                               << nodep->prettyTypeName() << " to constant.");
            }
        } ਨਹੀਂਤਾਂ {
            ਜੇ (nodep->isTimingControl()) m_hasJumpDelay = ਸੱਚ;
            // Calculate the width of this operation
            ਜੇ (m_params && !nodep->ਚੌੜਾਈ()) nodep = V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Params_Edit(nodep);
            iterateChildren(nodep);
        }
    }

ਜਨਤਕ:
    // Processing Mode Enum
    ਗਿਣਤੀ Proc_ਬਹੁਲਕ : uint8_t {
        PROC_PARAMS_NOWARN,
        PROC_PARAMS,
        PROC_GENERATE,
        PROC_ਜੀਵਤ,
        PROC_V_WARN,
        PROC_V_NOWARN,
        PROC_V_EXPENSIVE,
        PROC_CPP
    };

    // CONSTRUCTORS
    ConstVisitor(Proc_ਬਹੁਲਕ pmode, ਤਾਰਕਿਕ globalPass)
        : m_globalPass{globalPass}
        , m_concswap_ਨਾਂ{globalPass ? ("__Vconcswap_" + cvt_ਮੰਜ਼ਿਲ_Str(s_global_Pass_ਅੰਕ++)) : ""} {
        // clang-format off
        ਚੋਣ (pmode) {
        ਹਾਲਤ PROC_PARAMS_NOWARN:  m_doV = ਸੱਚ;  m_doNConst = ਸੱਚ; m_params = ਸੱਚ;
                                  m_required = ਝੂਠ; ਰੋਕੋ;
        ਹਾਲਤ PROC_PARAMS:         m_doV = ਸੱਚ;  m_doNConst = ਸੱਚ; m_params = ਸੱਚ;
                                  m_required = ਸੱਚ; ਰੋਕੋ;
        ਹਾਲਤ PROC_GENERATE:       m_doV = ਸੱਚ;  m_doNConst = ਸੱਚ; m_params = ਸੱਚ;
                                  m_required = ਸੱਚ; m_doGenerate = ਸੱਚ; ਰੋਕੋ;
        ਹਾਲਤ PROC_ਜੀਵਤ:           ਰੋਕੋ;
        ਹਾਲਤ PROC_V_WARN:         m_doV = ਸੱਚ;  m_doNConst = ਸੱਚ; m_warn = ਸੱਚ; m_convert_Logic_ਮੰਜ਼ਿਲ_ਦੋਆਧਾਰੀ_ਅੰਕ = ਸੱਚ; ਰੋਕੋ;
        ਹਾਲਤ PROC_V_NOWARN:       m_doV = ਸੱਚ;  m_doNConst = ਸੱਚ; ਰੋਕੋ;
        ਹਾਲਤ PROC_V_EXPENSIVE:    m_doV = ਸੱਚ;  m_doNConst = ਸੱਚ; m_doExpensive = ਸੱਚ; ਰੋਕੋ;
        ਹਾਲਤ PROC_CPP:            m_doV = ਝੂਠ; m_doNConst = ਸੱਚ; m_doCpp = ਸੱਚ; ਰੋਕੋ;
        ਮੂਲ:                  v3fatalSrc("Bad case"); ਰੋਕੋ;
        }
        // clang-format on
    }
    ~ConstVisitor() override {
        ਜੇ (m_doCpp) {
            ਜੇ (m_globalPass) {
                V3Stats::ਜੋੜੋ_Stat("Optimizations, Const bit op reduction", m_stat_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_Reduction);
            } ਨਹੀਂਤਾਂ {
                V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Optimizations, Const bit op reduction", m_stat_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_Reduction);
            }
        }
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Optimizations, Cond redundant expressions", m_stat_Cond_ਵਿਅੰਜਕ_Redundant);
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Optimizations, If cond redundant expressions",
                            m_stat_If_Cond_ਵਿਅੰਜਕ_Redundant);
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Optimizations, Concat merges", m_stat_Concat_ਮਿਲਾਓ);
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Optimizations, Member access predicate node visits",
                            m_statMemberAccessVisits);
    }

    Ast_ਗੰਢ* mainAcceptEdit(Ast_ਗੰਢ* nodep) {
        VIsCached::clear_Cache_ਰੁੱਖ();  // Avoid using any stale isPure
        // Operate starting at a random place
        ਵਾਪਸ iterateSubtreeReturnEdits(nodep);
    }
};

uint32_t ConstVisitor::s_global_Pass_ਅੰਕ = 0;

//######################################################################
// Const class functions

//! Force this cell node's parameter list to become a constant
ਖਾਲੀ V3Const::constifyParamsEdit(Ast_ਗੰਢ* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Params_Edit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS, /* globalPass: */ ਝੂਠ};
    // cppcheck-suppress constVariablePointer // edited below
    ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ਜੇ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } ਨਹੀਂਤਾਂ {
        (ਖਾਲੀ)visitor.mainAcceptEdit(nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump if have next line
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

//! Constify this cell node's parameter list if possible
ਖਾਲੀ V3Const::constifyParamsNoWarnEdit(Ast_ਗੰਢ* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Params_Edit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS_NOWARN, /* globalPass: */ ਝੂਠ};
    // cppcheck-suppress constVariablePointer // edited below
    ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ਜੇ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } ਨਹੀਂਤਾਂ {
        (ਖਾਲੀ)visitor.mainAcceptEdit(nodep);
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
ਖਾਲੀ V3Const::constifyGenerateParamsEdit(Ast_ਗੰਢ* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE:");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized
    // yet.

    // Make sure we've sized everything first
    nodep = V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Generate_Params_Edit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_GENERATE, /* globalPass: */ ਝੂਠ};
    // cppcheck-suppress constVariablePointer // edited below
    ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ਜੇ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } ਨਹੀਂਤਾਂ {
        VL_DO_DANGLING(visitor.mainAcceptEdit(nodep), nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump this.
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

ਖਾਲੀ V3Const::constifyAllLint(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_WARN, /* globalPass: */ ਸੱਚ};
        (ਖਾਲੀ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("const", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

ਖਾਲੀ V3Const::constifyCpp(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ ਸੱਚ};
        (ਖਾਲੀ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("const_cpp", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

Ast_ਗੰਢ* V3Const::constifyEdit(Ast_ਗੰਢ* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_NOWARN, /* globalPass: */ ਝੂਠ};
    nodep = visitor.mainAcceptEdit(nodep);
    ਵਾਪਸ nodep;
}

Ast_ਗੰਢ* V3Const::constifyEditCpp(Ast_ਗੰਢ* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ ਝੂਠ};
    nodep = visitor.mainAcceptEdit(nodep);
    ਵਾਪਸ nodep;
}

ਖਾਲੀ V3Const::constify_All_ਜੀਵਤ(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    // This only pushes constants up, doesn't make any other edits
    // IE doesn't prune dead statements, as we need to do some usability checks after this
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_ਜੀਵਤ, /* globalPass: */ ਸੱਚ};
        (ਖਾਲੀ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("const", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

ਖਾਲੀ V3Const::constifyAll(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ ਸੱਚ};
        (ਖਾਲੀ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("const", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

Ast_ਗੰਢ* V3Const::constifyExpensiveEdit(Ast_ਗੰਢ* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ ਝੂਠ};
    nodep = visitor.mainAcceptEdit(nodep);
    ਵਾਪਸ nodep;
}
