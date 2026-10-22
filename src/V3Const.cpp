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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Simulate.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"
#ಅನ್ತರ್ಭಾವ "V3Width.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <type_traits>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

#ವ್ಯಾಖ್ಯಾನಿಸು ವೃಕ್ಷ_SKIP_ಭೇಟಿ(...)
#ವ್ಯಾಖ್ಯಾನಿಸು TREEOP1(...)
#ವ್ಯಾಖ್ಯಾನಿಸು TREEOPA(...)
#ವ್ಯಾಖ್ಯಾನಿಸು TREEOP(...)
#ವ್ಯಾಖ್ಯಾನಿಸು TREEOPS(...)
#ವ್ಯಾಖ್ಯಾನಿಸು TREEOPC(...)
#ವ್ಯಾಖ್ಯಾನಿಸು TREEOPV(...)

//######################################################################
// Utilities

ಸ್ಥಿರ ತಾರ್ಕಿಕ isConst(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, uint64_t v) {
    ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const);
    ಹಿಂತಿರುಗಿ constp && constp->ಗಮ್ಯ_UQuad() == v;
}

ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T>
ಸ್ಥಿರ ಬಗೆ_ಹೆಸರು std::enable_if<std::is_integral<T>::ಮೌಲ್ಯ, ತಾರ್ಕಿಕ>::type isPow2(T val) {
    ಹಿಂತಿರುಗಿ (val & (val - 1)) == 0;
}

ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ ಎಣಿಕೆ_Trailing_Zeroes(uint64_t val) {
    UASSERT(val, "countTrailingZeroes argument must be non-zero");
#ಆದರೆ defined(__GNUC__) && !defined(VL_NO_BUILTINS)
    ಹಿಂತಿರುಗಿ __builtin_ctzll(val);
#ಇಲ್ಲದಿದ್ದರೆ
    ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0;
    val = ~val;
    ಆಗ (val & 1) {
        ++ಬಿಟ್_2;
        val >>= 1;
    }
    ಹಿಂತಿರುಗಿ ಬಿಟ್_2;
#ಆದರೆ_ಅಂತ್ಯ
}

// Allocates a unique id for each AstNode it is applied to, held in user4.
ವರ್ಗ VNIdAllocator final {
    // NODE STATE
    // AstNode::user4p -> size_t. Unique id of the node (0/nullptr if not allocated yet)

    // MEMBERS
    // Claimed lazily on first use TODO: fix conflict with V3Param user4 slot
    std::unique_ptr<VNUser4_ಒಳ_Use> m_inuser4p;
    ಗಾತ್ರ_t m_ಮುಂದಿನ_Id = 0;  // Id allocated most recently

ಸಾರ್ವಜನಿಕ:
    // Return the unique id of the given node, allocating a new one if it has none yet
    ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (!m_inuser4p) m_inuser4p = std::make_unique<VNUser4_ಒಳ_Use>();  // Claim on first use
        ಆದರೆ (!nodep->user4p()) nodep->user4p(reinterpret_cast<ಶೂನ್ಯ*>(++m_ಮುಂದಿನ_Id));
        ಹಿಂತಿರುಗಿ reinterpret_cast<ಗಾತ್ರ_t>(nodep->user4p());
    }
};

// This visitor can be used in the post-expanded Ast from V3Expand, where the Ast satisfies:
// - Constants are 64 bit at most (because words are accessed via AstWordSel)
// - Variables are scoped.
ವರ್ಗ Const_ಬಿಟ್_Op_ವೃಕ್ಷ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // TYPES

    // Holds a node to be added as a term in the reduction tree, it's equivalent op count, and a
    // bool indicating if the term is clean (0/1 value, or if the top bits might be dirty)
    ಬಳಸಿ ಫಲಿತಾಂಶ_ಪದ = std::tuple<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, ಚಿಹ್ನೆರಹಿತ, ತಾರ್ಕಿಕ>;

    ವರ್ಗ ಎಲೆ_Info final {  // Leaf node (either AstConst or AstVarRef)
        // MEMBERS
        ತಾರ್ಕಿಕ m_polarity = ನಿಜ;  // Invert result due to NOT
        ಪೂರ್ಣಾಂಕ m_lsb = 0;  // LSB of actually used bit of m_refp->varp()
        ಪೂರ್ಣಾಂಕ m_msb = 0;  // MSB of actually used bit of m_refp->varp()
        ಪೂರ್ಣಾಂಕ m_ಶಬ್ದ_Idx = -1;  // -1 means AstWordSel is not used.
        AstVarRef* m_refp = ಶೂನ್ಯಸೂಚಕ;  // Leaf's variable reference
        ಸ್ಥಿರಾಂಕ AstConst* m_constp = ಶೂನ್ಯಸೂಚಕ;  // Leaf's constant

    ಸಾರ್ವಜನಿಕ:
        // CONSTRUCTORS
        ಎಲೆ_Info() = ಪೂರ್ವನಿಯೋಜಿತ;
        ಎಲೆ_Info(ಸ್ಥಿರಾಂಕ ಎಲೆ_Info& other) = ಪೂರ್ವನಿಯೋಜಿತ;
        ಎಲೆ_Info& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ ಎಲೆ_Info& other) = ಪೂರ್ವನಿಯೋಜಿತ;
        ಎಲೆ_Info(ಎಲೆ_Info&& other) = ಪೂರ್ವನಿಯೋಜಿತ;
        ಎಲೆ_Info& ಕ್ರಿಯಾಕಾರಕ=(ಎಲೆ_Info&& other) = ಪೂರ್ವನಿಯೋಜಿತ;
        ಸ್ಪಷ್ಟ ಎಲೆ_Info(ಪೂರ್ಣಾಂಕ lsb)
            : m_lsb{lsb} {}

        // METHODS
        ಶೂನ್ಯ ಗಣ_ಎಲೆ(AstVarRef* refp) {
            UASSERT_OBJ(!m_refp && !m_constp, refp, "Must be called just once");
            m_refp = refp;
            m_msb = refp->varp()->ಅಗಲ_Min() - 1;
        }
        ಶೂನ್ಯ ಗಣ_ಎಲೆ(ಸ್ಥಿರಾಂಕ AstConst* constp) {
            UASSERT_OBJ(!m_refp && !m_constp, constp, "Must be called just once");
            m_constp = constp;
            m_msb = constp->ಅಗಲ_Min() - 1;
        }
        // updateBitRange(), limitBitRangeToLsb(), and polarity() must be called during ascending
        // back to the root.
        ಶೂನ್ಯ update_ಬಿಟ್_Range(ಪೂರ್ಣಾಂಕ newLsb, ಪೂರ್ಣಾಂಕ newMsb) {
            ಆದರೆ ((m_lsb <= m_msb && newLsb > newMsb) || (m_lsb > m_msb && m_lsb < newLsb)) {
                // When the new bit range is out of m_refp, clear polarity because nodes below is
                // shifted out to zero.
                // This kind of clear may happen several times. e.g. (!(1'b1 >> 1)) >> 1
                polarity(ನಿಜ);
            }
            m_lsb = newLsb;
            m_msb = newMsb;
        }
        ಶೂನ್ಯ update_ಬಿಟ್_Range(ಸ್ಥಿರಾಂಕ AstCCast* castp) {
            update_ಬಿಟ್_Range(m_lsb, std::min(m_msb, m_lsb + castp->ಅಗಲ() - 1));
        }
        ಶೂನ್ಯ update_ಬಿಟ್_Range(ಸ್ಥಿರಾಂಕ Ast_ಸರಿಸು_R* shiftp) {
            update_ಬಿಟ್_Range(m_lsb + VN_AS(shiftp->rhsp(), Const)->ಗಮ್ಯ_UInt(), m_msb);
        }
        ಶೂನ್ಯ ಎಲ್ಲೆ_ಬಿಟ್_Range_ಗಮ್ಯ_Lsb() { update_ಬಿಟ್_Range(m_lsb, std::min(m_msb, m_lsb)); }
        ಪೂರ್ಣಾಂಕ ಶಬ್ದ_Idx() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಶಬ್ದ_Idx; }
        ಶೂನ್ಯ ಶಬ್ದ_Idx(ಪೂರ್ಣಾಂಕ i) { m_ಶಬ್ದ_Idx = i; }
        ತಾರ್ಕಿಕ polarity() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_polarity; }
        ಶೂನ್ಯ polarity(ತಾರ್ಕಿಕ p) { m_polarity = p; }

        AstVarRef* refp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_refp; }
        ಸ್ಥಿರಾಂಕ AstConst* constp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_constp; }
        ತಾರ್ಕಿಕ missing_ಶಬ್ದ_Sel() ಸ್ಥಿರಾಂಕ {
            // When V3Expand is skipped, WordSel is not inserted.
            ಹಿಂತಿರುಗಿ m_refp->isWide() && m_ಶಬ್ದ_Idx == -1;
        }
        ಪೂರ್ಣಾಂಕ lsb() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_lsb; }

        ಪೂರ್ಣಾಂಕ msb() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ std::min(m_msb, var_ಅಗಲ() - 1); }
        ಪೂರ್ಣಾಂಕ var_ಅಗಲ() ಸ್ಥಿರಾಂಕ {
            UASSERT(m_refp, "m_refp should be set");
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = m_refp->varp()->ಅಗಲ_Min();
            ಆದರೆ (!m_refp->isWide()) {
                UASSERT_OBJ(m_ಶಬ್ದ_Idx == -1, m_refp, "Bad word index into non-wide");
                ಹಿಂತಿರುಗಿ ಅಗಲ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (missing_ಶಬ್ದ_Sel()) ಹಿಂತಿರುಗಿ ಅಗಲ;
                UASSERT_OBJ(m_ಶಬ್ದ_Idx >= 0, m_refp, "Bad word index into wide");
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಬಿಟ್ಟುಗಳು_ಒಳ_MSW = VL_BITBIT_E(ಅಗಲ) ? VL_BITBIT_E(ಅಗಲ) : VL_EDATASIZE;
                ಹಿಂತಿರುಗಿ m_ಶಬ್ದ_Idx == m_refp->ಅಗಲ_ಪದಗಳು() - 1 ? ಬಿಟ್ಟುಗಳು_ಒಳ_MSW : VL_EDATASIZE;
            }
        }
    };

    ರಚನೆ ಬಿಟ್_Polarity_ನಮೂದು final {  // Found bit polarity during iterate()
        ಎಲೆ_Info m_info;  // Leaf (variable or constant) bit polarity was found on
        ತಾರ್ಕಿಕ m_polarity = ಸುಳ್ಳು;  // Polarity the bit must have to match
        ಪೂರ್ಣಾಂಕ m_ಬಿಟ್ = 0;  // Bit index within the leaf that was tested
        ಬಿಟ್_Polarity_ನಮೂದು(ಸ್ಥಿರಾಂಕ ಎಲೆ_Info& info, ತಾರ್ಕಿಕ pol, ಪೂರ್ಣಾಂಕ ಬಿಟ್_2)
            : m_info{info}
            , m_polarity{pol}
            , m_ಬಿಟ್{ಬಿಟ್_2} {}
        ಬಿಟ್_Polarity_ನಮೂದು() = ಪೂರ್ವನಿಯೋಜಿತ;
    };

    ರಚನೆ Frozen_ಗಂಟು_Info final {  // Context when a frozen node is found
        ತಾರ್ಕಿಕ m_polarity;  // Polarity the frozen node must match
        ಪೂರ್ಣಾಂಕ m_lsb;  // LSB position of the frozen node
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ Frozen_ಗಂಟು_Info& other) ಸ್ಥಿರಾಂಕ {
            ಆದರೆ (m_lsb != other.m_lsb) ಹಿಂತಿರುಗಿ m_lsb < other.m_lsb;
            ಹಿಂತಿರುಗಿ m_polarity < other.m_polarity;
        }
    };

    ವರ್ಗ Restorer final {  // Restore the original state unless disableRestore() is called
        Const_ಬಿಟ್_Op_ವೃಕ್ಷ_Visitor& m_visitor;  // Visitor whose state is saved and restored
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t m_polarities_ಗಾತ್ರ;  // Saved m_visitor.m_bitPolarities size to truncate back to
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t m_frozen_ಗಾತ್ರ;  // Saved m_visitor.m_frozenNodes size to truncate back to
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ m_ops;  // Saved m_visitor.m_ops to restore
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_polarity;  // Saved m_visitor.m_polarity to restore
        ತಾರ್ಕಿಕ m_restore = ನಿಜ;  // Whether the destructor still needs to restore state

    ಸಾರ್ವಜನಿಕ:
        ಸ್ಪಷ್ಟ Restorer(Const_ಬಿಟ್_Op_ವೃಕ್ಷ_Visitor& visitor)
            : m_visitor{visitor}
            , m_polarities_ಗಾತ್ರ{visitor.m_ಬಿಟ್_Polarities.ಗಾತ್ರ_2()}
            , m_frozen_ಗಾತ್ರ{visitor.m_frozenNodes.ಗಾತ್ರ_2()}
            , m_ops{visitor.m_ops}
            , m_polarity{visitor.m_polarity} {}
        ~Restorer() {
            UASSERT(m_visitor.m_ಬಿಟ್_Polarities.ಗಾತ್ರ_2() >= m_polarities_ಗಾತ್ರ,
                    "m_bitPolarities must grow monotonically");
            UASSERT(m_visitor.m_frozenNodes.ಗಾತ್ರ_2() >= m_frozen_ಗಾತ್ರ,
                    "m_frozenNodes must grow monotonically");
            ಆದರೆ (m_restore) restoreNow();
        }
        ಶೂನ್ಯ disableRestore() { m_restore = ಸುಳ್ಳು; }
        ಶೂನ್ಯ restoreNow() {
            UASSERT(m_restore, "Can be called just once");
            m_visitor.m_ಬಿಟ್_Polarities.resize(m_polarities_ಗಾತ್ರ);
            m_visitor.m_frozenNodes.resize(m_frozen_ಗಾತ್ರ);
            m_visitor.m_ops = m_ops;
            m_visitor.m_polarity = m_polarity;
            m_restore = ಸುಳ್ಳು;
        }
    };
    // Collect information for each Variable to transform as below
    ವರ್ಗ VarInfo final {
        // MEMBERS
        ಪೂರ್ಣಾಂಕ m_ತಿಳಿದ_ಫಲಿತಾಂಶ = -1;  // -1: result is not known, 0 or 1: result of this tree
        ಸ್ಥಿರಾಂಕ Const_ಬಿಟ್_Op_ವೃಕ್ಷ_Visitor* ಸ್ಥಿರಾಂಕ
            m_parentp;  // ConstBitOpTreeVisitor holding this VarInfo
        AstVarRef* ಸ್ಥಿರಾಂಕ m_refp;  // Points the variable that this VarInfo covers
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_ಅಗಲ;  // Width of term this VarInfo refers to
        V3_ಸಂಖ್ಯೆ m_ಬಿಟ್_Polarity;  // Coefficient of each bit

    ಸಾರ್ವಜನಿಕ:
        // METHODS
        ತಾರ್ಕಿಕ has_Const_ಫಲಿತಾಂಶ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತಿಳಿದ_ಫಲಿತಾಂಶ >= 0 || m_ಬಿಟ್_Polarity.isAllX(); }
        // The constant result. Only valid if hasConstResult() returned true.
        ತಾರ್ಕಿಕ get_Const_ಫಲಿತಾಂಶ() ಸ್ಥಿರಾಂಕ {
            // Note that this condition covers m_knownResult == -1 but m_bitPolarity.isAllX(),
            // in which case the result is 0
            ಹಿಂತಿರುಗಿ m_ತಿಳಿದ_ಫಲಿತಾಂಶ == 1;
        }
        ಸ್ಥಿರಾಂಕ AstVarRef* refp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_refp; }
        ತಾರ್ಕಿಕ sameVarAs(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* otherp) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_refp->same_ಗಂಟು(otherp); }
        ಶೂನ್ಯ ಗಣ_Polarity(ತಾರ್ಕಿಕ comp_ಬಿಟ್, ಪೂರ್ಣಾಂಕ ಬಿಟ್_2) {
            // Ignore if already determined a known reduction
            ಆದರೆ (m_ತಿಳಿದ_ಫಲಿತಾಂಶ >= 0) ಹಿಂತಿರುಗಿ;
            UASSERT_OBJ(ಬಿಟ್_2 < m_ಅಗಲ, m_refp,
                        "Bit index out of range: " << ಬಿಟ್_2 << " width: " << m_ಅಗಲ);
            ಆದರೆ (m_ಬಿಟ್_Polarity.ಬಿಟ್_Is_X(ಬಿಟ್_2)) {  // The bit is not yet marked with either polarity
                m_ಬಿಟ್_Polarity.ಗಣ_ಬಿಟ್(ಬಿಟ್_2, comp_ಬಿಟ್);
            } ಇಲ್ಲದಿದ್ದರೆ {  // The bit has already been marked with some polarity
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ sameFlag = m_ಬಿಟ್_Polarity.ಬಿಟ್_Is1(ಬಿಟ್_2) == comp_ಬಿಟ್;
                ಆದರೆ (m_parentp->is_Xor_ವೃಕ್ಷ()) {
                    UASSERT_OBJ(comp_ಬಿಟ್ && sameFlag, m_refp, "Only true is set in Xor tree");
                    // a ^ a ^ b == b so we can ignore a
                    m_ಬಿಟ್_Polarity.ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 'x');
                } ಇಲ್ಲದಿದ್ದರೆ {  // And, Or
                    // Can ignore this nodep as the bit is already marked with the same polarity
                    ಆದರೆ (sameFlag) ಹಿಂತಿರುಗಿ;  // a & a == a, b | b == b
                    // Otherwise result is constant (a & ~a == 0) or (a | ~a == 1)
                    m_ತಿಳಿದ_ಫಲಿತಾಂಶ = m_parentp->is_And_ವೃಕ್ಷ() ? 0 : 1;
                    m_ಬಿಟ್_Polarity.ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();  // The variable is not referred anymore
                }
            }
        }

        // Return reduction term for this VarInfo, together with the number of ops in the term,
        // and a boolean indicating if the term is clean (1-bit vs multi-bit value)
        ಫಲಿತಾಂಶ_ಪದ get_ಫಲಿತಾಂಶ_ಪದ() ಸ್ಥಿರಾಂಕ {
            UASSERT_OBJ(!has_Const_ಫಲಿತಾಂಶ(), m_refp, "getTerm on reduction that yields constant");
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = m_refp->fileline();

            // Get the term we are referencing (the WordSel, if wide, otherwise just the VarRef)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = VN_CAST(m_refp->backp(), ಶಬ್ದ_Sel);
            ಆದರೆ (!srcp) srcp = m_refp;
            srcp = srcp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);

            // Signed variables might have redundant sign bits that need masking.
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ has_Redundant_Sign_ಬಿಟ್ಟುಗಳು
                = m_refp->varp()->dtypep()->isSigned()
                  && (m_refp->isWide() ? (m_ಅಗಲ != VL_EDATASIZE)
                                       : (m_ಅಗಲ < 8 || !isPow2(m_ಅಗಲ)));

            // Get the mask that selects the bits that are relevant in this term
            V3_ಸಂಖ್ಯೆ ಮುಖವಾಡ_ಅಂಕೆ{srcp, m_ಅಗಲ, 0};
            ಮುಖವಾಡ_ಅಂಕೆ.op_ಬಿಟ್ಟುಗಳು_Non_XZ(m_ಬಿಟ್_Polarity);  // 'x' -> 0, 0->1, 1->1
            ಸ್ಥಿರಾಂಕ uint64_t ಮುಖವಾಡ_Val = ಮುಖವಾಡ_ಅಂಕೆ.ಗಮ್ಯ_UQuad();
            UASSERT_OBJ(ಮುಖವಾಡ_Val != 0, m_refp,
                        "Should have been recognized as having const 0 result");

            // Parts of the return value
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = srcp;  // The tree for this term
            ಚಿಹ್ನೆರಹಿತ ops = 0;  // Number of ops in this term
            ತಾರ್ಕಿಕ clean = ಸುಳ್ಳು;  // Whether the term is clean (has value 0 or 1)

            ಆದರೆ (isPow2(ಮುಖವಾಡ_Val)) {
                // If we only want a single bit, shift it out instead of a masked compare. Shifts
                // don't go through the flags register on x86 and are hence faster. This is also
                // always fewer or same ops as mask and compare, but with shorter instructions on
                // x86.

                // Find the index of the bit we want.
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = ಎಣಿಕೆ_Trailing_Zeroes(ಮುಖವಾಡ_Val);
                // If we want something other than the bottom bit, shift it out
                ಆದರೆ (ಬಿಟ್_2 != 0) {
                    resultp = ಹೊಸ Ast_ಸರಿಸು_R{fl, resultp,
                                            ಹೊಸ AstConst{fl, static_cast<uint32_t>(ಬಿಟ್_2)}, m_ಅಗಲ};
                    ++ops;
                }
                // Negate it if necessary
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ negate = m_ಬಿಟ್_Polarity.ಬಿಟ್_Is0(ಬಿಟ್_2);
                ಆದರೆ (negate) {
                    resultp = ಹೊಸ AstNot{fl, resultp};
                    ++ops;
                }
                // Clean if MSB of unsigned value, and not negated
                clean = (ಬಿಟ್_2 == m_ಅಗಲ - 1) && !has_Redundant_Sign_ಬಿಟ್ಟುಗಳು && !negate;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // We want multiple bits. Go ahead and extract them.

                // Check if masking is required, and if so apply it
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ needsMasking = ಮುಖವಾಡ_Val != VL_ಮುಖವಾಡ_Q(m_ಅಗಲ) || has_Redundant_Sign_ಬಿಟ್ಟುಗಳು;
                ಆದರೆ (needsMasking) {
                    resultp = ಹೊಸ AstAnd{fl, ಹೊಸ AstConst{fl, ಮುಖವಾಡ_ಅಂಕೆ}, resultp};
                    ++ops;
                }

                // Create the sub-expression for this term
                ಆದರೆ (m_parentp->is_Xor_ವೃಕ್ಷ()) {
                    ಆದರೆ (needsMasking) {
                        // Reduce the masked term to the minimum known width,
                        // to use the smallest RedXor formula
                        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ_Min = ಮುಖವಾಡ_ಅಂಕೆ.ಅಗಲ_ಗಮ್ಯ_Fit();
                        resultp->dtype_Chg_ಅಗಲ(ಅಗಲ_Min, ಅಗಲ_Min);
                    }
                    resultp = ಹೊಸ AstRedXor{fl, resultp};
                    ++ops;
                    clean = ಸುಳ್ಳು;
                    // VL_REDXOR_* returns IData, set width accordingly to avoid unnecessary casts
                    resultp->dtype_Chg_ಅಗಲ(VL_IDATASIZE, 1);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_parentp->is_And_ವೃಕ್ಷ()) {
                    V3_ಸಂಖ್ಯೆ comp_ಅಂಕೆ{srcp, m_ಅಗಲ, 0};
                    comp_ಅಂಕೆ.op_ಬಿಟ್ಟುಗಳು_One(m_ಬಿಟ್_Polarity);  // 'x'->0, 0->0, 1->1
                    resultp = ಹೊಸ AstEq{fl, ಹೊಸ AstConst{fl, comp_ಅಂಕೆ}, resultp};
                    ++ops;
                    clean = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ {  // Or
                    V3_ಸಂಖ್ಯೆ comp_ಅಂಕೆ{srcp, m_ಅಗಲ, 0};
                    comp_ಅಂಕೆ.op_ಬಿಟ್ಟುಗಳು_One(m_ಬಿಟ್_Polarity);  // 'x'->0, 0->0, 1->1
                    comp_ಅಂಕೆ.opXor(V3_ಸಂಖ್ಯೆ{comp_ಅಂಕೆ}, ಮುಖವಾಡ_ಅಂಕೆ);
                    resultp = ಹೊಸ AstNeq{fl, ಹೊಸ AstConst{fl, comp_ಅಂಕೆ}, resultp};
                    ++ops;
                    clean = ನಿಜ;
                }
            }

            ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ_ಪದ{resultp, ops, clean};
        }

        // CONSTRUCTORS
        VarInfo(Const_ಬಿಟ್_Op_ವೃಕ್ಷ_Visitor* ಪೋಷಕ, AstVarRef* refp, ಪೂರ್ಣಾಂಕ ಅಗಲ)
            : m_parentp{ಪೋಷಕ}
            , m_refp{refp}
            , m_ಅಗಲ{ಅಗಲ}
            , m_ಬಿಟ್_Polarity{refp, m_ಅಗಲ} {
            m_ಬಿಟ್_Polarity.ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
        }
    };

    // MEMBERS
    ತಾರ್ಕಿಕ m_failed = ಸುಳ್ಳು;
    ತಾರ್ಕಿಕ m_polarity = ನಿಜ;  // Flip when AstNot comes
    ಚಿಹ್ನೆರಹಿತ m_ops;  // Number of operations such as And, Or, Xor, Sel...
    ಪೂರ್ಣಾಂಕ m_lsb = 0;  // Current LSB
    ಎಲೆ_Info* m_leafp = ಶೂನ್ಯಸೂಚಕ;  // AstConst or AstVarRef that currently looking for
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ m_rootp;  // Root of this AST subtree

    std::ಸದಿಶ<std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Frozen_ಗಂಟು_Info>>
        m_frozenNodes;  // Nodes that cannot be optimized
    std::ಸದಿಶ<ಬಿಟ್_Polarity_ನಮೂದು> m_ಬಿಟ್_Polarities;  // Polarity of bits found during iterate()
    std::ಸದಿಶ<std::unique_ptr<VarInfo>> m_varInfos;  // VarInfo for each variable, [0] is nullptr
    VNIdAllocator& m_ids;  // Node id allocator, owned by ConstVisitor
    // Base index of m_varInfos that points VarInfo, keyed by AstVarScope/AstVarRef id,
    // zero means not set yet.
    std::unordered_map<ಗಾತ್ರ_t, ಪೂರ್ಣಾಂಕ> m_ತಳ_Idxs;

    // METHODS

    ತಾರ್ಕಿಕ is_And_ವೃಕ್ಷ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VN_IS(m_rootp, And); }
    ತಾರ್ಕಿಕ is_Or_ವೃಕ್ಷ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VN_IS(m_rootp, Or); }
    ತಾರ್ಕಿಕ is_Xor_ವೃಕ್ಷ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VN_IS(m_rootp, Xor) || VN_IS(m_rootp, RedXor); }

#ವ್ಯಾಖ್ಯಾನಿಸು CONST_BITOP_RETURN_IF(cond, nodep) \
    ಆದರೆ (ಗಣ_Failed(cond, #cond, nodep, __LINE__)) ಹಿಂತಿರುಗಿ

#ವ್ಯಾಖ್ಯಾನಿಸು CONST_BITOP_ಗಣ_FAILED(reason, nodep) ಗಣ_Failed(ನಿಜ, reason, nodep, __LINE__)

    ತಾರ್ಕಿಕ ಗಣ_Failed(ತಾರ್ಕಿಕ fail, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* reason, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಂಕ ಸಾಲು) {
        ಆದರೆ (fail && !m_failed) {
            UINFO(9, "cannot optimize " << m_rootp << " reason:" << reason << " called from line:"
                                        << ಸಾಲು << " when checking:" << nodep);
            // UINFOTREE(9, m_rootp, "", "root");
            m_failed = ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ m_failed;
    }
    ಶೂನ್ಯ incrOps(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಂಕ ಸಾಲು) {
        ++m_ops;
        UINFO(9, "Increment to " << m_ops << " " << nodep << " called from line " << ಸಾಲು);
    }
    VarInfo& getVarInfo(ಸ್ಥಿರಾಂಕ ಎಲೆ_Info& ref) {
        UASSERT_OBJ(ref.refp(), m_rootp, "null varref in And/Or/Xor optimization");
        Ast_ಗಂಟು* nodep = ref.refp()->varScopep();
        ಆದರೆ (!nodep) nodep = ref.refp()->varp();  // Not scoped
        ಪೂರ್ಣಾಂಕ& ತಳ_Idxr = m_ತಳ_Idxs[m_ids(nodep)];
        ಆದರೆ (ತಳ_Idxr == 0) {  // Not set yet
            ತಳ_Idxr = m_varInfos.ಗಾತ್ರ_2();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಂಕೆ_ಪದಗಳು
                = ref.refp()->dtypep()->isWide() ? ref.refp()->dtypep()->ಅಗಲ_ಪದಗಳು() : 1;
            m_varInfos.resize(m_varInfos.ಗಾತ್ರ_2() + ಅಂಕೆ_ಪದಗಳು);
        }
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t idx = ತಳ_Idxr + std::max(0, ref.ಶಬ್ದ_Idx());
        VarInfo* varInfop = m_varInfos[idx].get();
        ಆದರೆ (!varInfop) {
            varInfop = ಹೊಸ VarInfo{ಇದು, ref.refp(), ref.var_ಅಗಲ()};
            m_varInfos[idx].reset(varInfop);
            ಆದರೆ (ref.missing_ಶಬ್ದ_Sel()) {
                // ConstBitOpTreeVisitor makes some constants for masks and its type is uint64_t.
                // That's why V3Expand, that inserts WordSel, is needed.
                CONST_BITOP_ಗಣ_FAILED("V3Expand is skipped", ref.refp());
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (!varInfop->sameVarAs(ref.refp()))
                CONST_BITOP_ಗಣ_FAILED("different var (scope?)", ref.refp());
        }
        ಹಿಂತಿರುಗಿ *varInfop;
    }

    // Traverse down to see AstConst or AstVarRef
    ಎಲೆ_Info ಕಂಡುಹಿಡಿ_ಎಲೆ(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ expectConst) {
        ಆದರೆ (!nodep->dtypep()->skipRefp()->isIntegralOrPacked()) ಹಿಂತಿರುಗಿ ಎಲೆ_Info{};
        ಎಲೆ_Info info{m_lsb};
        {
            VL_RESTORER(m_leafp);
            m_leafp = &info;
            iterateConst(nodep);
        }

        ತಾರ್ಕಿಕ ok = !m_failed;
        ಆದರೆ (expectConst) {
            ok &= !info.refp() && info.constp();
        } ಇಲ್ಲದಿದ್ದರೆ {
            ok &= info.refp() && !info.constp();
        }
        ಹಿಂತಿರುಗಿ ok ? info : ಎಲೆ_Info{};
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { CONST_BITOP_ಗಣ_FAILED("Hit unexpected op", nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCCast* nodep) override {
        iterateChildrenConst(nodep);
        ಆದರೆ (m_leafp) m_leafp->update_ಬಿಟ್_Range(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸರಿಸು_R* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->rhsp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->rhsp());
        m_lsb += constp->ಗಮ್ಯ_UInt();
        incrOps(nodep, __LINE__);
        iterateConst(nodep->lhsp());
        m_leafp->update_ಬಿಟ್_Range(nodep);
        m_lsb -= constp->ಗಮ್ಯ_UInt();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstNot* nodep) override {
        CONST_BITOP_RETURN_IF(nodep->ಅಗಲ_Min() != 1, nodep);
        Ast_ಗಂಟು* lhsp = nodep->lhsp();
        AstCCast* ಸ್ಥಿರಾಂಕ castp = VN_CAST(lhsp, CCast);
        ಆದರೆ (castp) lhsp = castp->lhsp();
        CONST_BITOP_RETURN_IF(!is_Xor_ವೃಕ್ಷ() && !VN_IS(lhsp, VarRef) && !VN_IS(lhsp, ಸರಿಸು_R), lhsp);
        incrOps(nodep, __LINE__);
        m_polarity = !m_polarity;
        iterateChildrenConst(nodep);
        // Don't restore m_polarity for Xor as it counts parity of the entire tree
        ಆದರೆ (!is_Xor_ವೃಕ್ಷ()) m_polarity = !m_polarity;
        ಆದರೆ (m_leafp && castp) m_leafp->update_ಬಿಟ್_Range(castp);
        ಆದರೆ (m_leafp) m_leafp->polarity(!m_leafp->polarity());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಶಬ್ದ_Sel* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->bitp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->bitp());
        UASSERT_OBJ(m_leafp->ಶಬ್ದ_Idx() == -1, nodep, "Unexpected nested WordSel");
        m_leafp->ಶಬ್ದ_Idx(constp->ಗಮ್ಯ_SInt());
        iterateConst(nodep->fromp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->ಗಣ_ಎಲೆ(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConst* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->ಗಣ_ಎಲೆ(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstRedXor* nodep) override {
        Restorer restorer{*ಇದು};
        CONST_BITOP_RETURN_IF(!VN_IS(m_rootp, Xor), nodep);
        Ast_ಗಂಟು* lhsp = nodep->lhsp();
        ಸ್ಥಿರಾಂಕ AstCCast* ಸ್ಥಿರಾಂಕ castp = VN_CAST(lhsp, CCast);
        ಆದರೆ (castp) lhsp = castp->lhsp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAnd* ಸ್ಥಿರಾಂಕ andp = VN_CAST(lhsp, And)) {  // '^(mask & leaf)'
            CONST_BITOP_RETURN_IF(!andp, lhsp);

            ಸ್ಥಿರಾಂಕ ಎಲೆ_Info& ಮುಖವಾಡ = ಕಂಡುಹಿಡಿ_ಎಲೆ(andp->lhsp(), ನಿಜ);
            CONST_BITOP_RETURN_IF(!ಮುಖವಾಡ.constp() || ಮುಖವಾಡ.lsb() != 0, andp->lhsp());

            ಎಲೆ_Info ref = ಕಂಡುಹಿಡಿ_ಎಲೆ(andp->rhsp(), ಸುಳ್ಳು);
            CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());
            ಆದರೆ (castp) ref.update_ಬಿಟ್_Range(castp);

            restorer.disableRestore();  // Now all subtree succeeded

            ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಮುಖವಾಡ_ಅಂಕೆ = ಮುಖವಾಡ.constp()->ಅಂಕೆ();

            incrOps(nodep, __LINE__);
            incrOps(andp, __LINE__);

            // Mark all bits checked in this reduction
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ max_ಬಿಟ್_Idx = std::min(ref.lsb() + ಮುಖವಾಡ_ಅಂಕೆ.ಅಗಲ(), ref.msb() + 1);
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_Idx = ref.lsb(); ಬಿಟ್_Idx < max_ಬಿಟ್_Idx; ++ಬಿಟ್_Idx) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಮುಖವಾಡ_Idx = ಬಿಟ್_Idx - ref.lsb();
                ಆದರೆ (ಮುಖವಾಡ_ಅಂಕೆ.ಬಿಟ್_Is0(ಮುಖವಾಡ_Idx)) ಮುಂದುವರಿ;
                // Set true, m_polarity takes care of the entire parity
                m_ಬಿಟ್_Polarities.emplace_ಹಿಂಭಾಗ(ref, ನಿಜ, ಬಿಟ್_Idx);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {  // '^leaf'
            ಎಲೆ_Info ref = ಕಂಡುಹಿಡಿ_ಎಲೆ(lhsp, ಸುಳ್ಳು);
            CONST_BITOP_RETURN_IF(!ref.refp(), lhsp);
            ಆದರೆ (castp) ref.update_ಬಿಟ್_Range(castp);

            restorer.disableRestore();  // Now all checks passed

            incrOps(nodep, __LINE__);

            // Mark all bits checked by this comparison
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_Idx = ref.lsb(); ಬಿಟ್_Idx <= ref.msb(); ++ಬಿಟ್_Idx) {
                m_ಬಿಟ್_Polarities.emplace_ಹಿಂಭಾಗ(ref, ನಿಜ, ಬಿಟ್_Idx);
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Biop* nodep) override {
        ಆದರೆ (VN_IS(nodep, And) && isConst(nodep->lhsp(), 1)) {  // 1 & _
            // Always reach past a plain making AND
            Restorer restorer{*ಇದು};
            incrOps(nodep, __LINE__);
            iterateConst(nodep->rhsp());
            ಆದರೆ (m_leafp) m_leafp->ಎಲ್ಲೆ_ಬಿಟ್_Range_ಗಮ್ಯ_Lsb();
            CONST_BITOP_RETURN_IF(m_failed, nodep->rhsp());
            restorer.disableRestore();  // Now all checks passed
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->type() == m_rootp->type()) {  // And, Or, Xor
            // subtree under NOT can be optimized only in XOR tree.
            CONST_BITOP_RETURN_IF(!m_polarity && !is_Xor_ವೃಕ್ಷ(), nodep);
            incrOps(nodep, __LINE__);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಬಲ : {ಸುಳ್ಳು, ನಿಜ}) {
                VL_RESTORER(m_leafp);
                Restorer restorer{*ಇದು};
                ಎಲೆ_Info ಎಲೆ_Info_2{m_lsb};
                // cppcheck-suppress danglingLifetime
                m_leafp = &ಎಲೆ_Info_2;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* opp = ಬಲ ? nodep->rhsp() : nodep->lhsp();
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ origFailed = m_failed;
                iterateConst(opp);
                ಆದರೆ (ಎಲೆ_Info_2.constp() || m_failed) {
                    // Revert changes in leaf
                    restorer.restoreNow();
                    // Reach past a cast then add to frozen nodes to be added to final reduction
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCCast* ಸ್ಥಿರಾಂಕ castp = VN_CAST(opp, CCast)) opp = castp->lhsp();
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ pol = is_Xor_ವೃಕ್ಷ() || m_polarity;  // Only AND/OR tree needs polarity
                    UASSERT_OBJ(pol, nodep, "AND/OR tree expects m_polarity==true");
                    m_frozenNodes.emplace_ಹಿಂಭಾಗ(opp, Frozen_ಗಂಟು_Info{pol, m_lsb});
                    m_failed = origFailed;
                    ಮುಂದುವರಿ;
                }
                restorer.disableRestore();  // Now all checks passed
                ಆದರೆ (ಎಲೆ_Info_2.refp()) {
                    // The conditional on the lsb being in range is necessary for some degenerate
                    // case, e.g.: (IData)((QData)wide[0] >> 32), or <1-bit-var> >> 1, which is
                    // just zero
                    ಆದರೆ (ಎಲೆ_Info_2.lsb() <= ಎಲೆ_Info_2.msb()) {
                        m_ಬಿಟ್_Polarities.emplace_ಹಿಂಭಾಗ(ಎಲೆ_Info_2, is_Xor_ವೃಕ್ಷ() || ಎಲೆ_Info_2.polarity(),
                                                     ಎಲೆ_Info_2.lsb());
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((is_And_ವೃಕ್ಷ() && ಎಲೆ_Info_2.polarity())
                               || (is_Or_ವೃಕ್ಷ() && !ಎಲೆ_Info_2.polarity())) {
                        // If there is a constant 0 term in an And tree or 1 term in an Or tree, we
                        // must include it. Fudge this by adding a bit with both polarities, which
                        // will simplify to zero or one respectively.
                        // Note that Xor tree does not need this kind of care, polarity of Xor tree
                        // is already cared when visitin AstNot. Taking xor with 1'b0 is nop.
                        m_ಬಿಟ್_Polarities.emplace_ಹಿಂಭಾಗ(ಎಲೆ_Info_2, ನಿಜ, 0);
                        m_ಬಿಟ್_Polarities.emplace_ಹಿಂಭಾಗ(ಎಲೆ_Info_2, ಸುಳ್ಳು, 0);
                    }
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((is_And_ವೃಕ್ಷ() && VN_IS(nodep, Eq)) || (is_Or_ವೃಕ್ಷ() && VN_IS(nodep, Neq))) {
            Restorer restorer{*ಇದು};
            CONST_BITOP_RETURN_IF(!m_polarity, nodep);
            CONST_BITOP_RETURN_IF(m_lsb, nodep);  // the result of EQ/NE is 1 bit width
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* lhsp = nodep->lhsp();
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCCast* ಸ್ಥಿರಾಂಕ castp = VN_CAST(lhsp, CCast)) lhsp = castp->lhsp();
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(lhsp, Const);
            CONST_BITOP_RETURN_IF(!constp, nodep->lhsp());

            ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& comp_ಅಂಕೆ = constp->ಅಂಕೆ();

            ಸ್ವಯಂಚಾಲಿತ ಗಣ_Polarities = [ಇದು, &comp_ಅಂಕೆ](ಸ್ಥಿರಾಂಕ ಎಲೆ_Info& ref, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ* maskp) {
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಮುಖವಾಡ_Flip = is_And_ವೃಕ್ಷ() ^ ref.polarity();
                ಪೂರ್ಣಾಂಕ constant_ಅಗಲ = comp_ಅಂಕೆ.ಅಗಲ();
                ಆದರೆ (maskp) constant_ಅಗಲ = std::max(constant_ಅಗಲ, maskp->ಅಗಲ());
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ max_ಬಿಟ್_Idx = std::max(ref.lsb() + constant_ಅಗಲ, ref.msb() + 1);
                // Mark all bits checked by this comparison
                ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_Idx = ref.lsb(); ಬಿಟ್_Idx < max_ಬಿಟ್_Idx; ++ಬಿಟ್_Idx) {
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಮುಖವಾಡ_Idx = ಬಿಟ್_Idx - ref.lsb();
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ mask0 = maskp && maskp->ಬಿಟ್_Is0(ಮುಖವಾಡ_Idx);
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಹೊರಹೋಗು_ದ_Range = ಬಿಟ್_Idx > ref.msb();
                    ಆದರೆ (mask0 || ಹೊರಹೋಗು_ದ_Range) {  // RHS is 0
                        ಆದರೆ (comp_ಅಂಕೆ.ಬಿಟ್_Is1(ಮುಖವಾಡ_Idx)) {
                            // LHS is 1
                            // And tree: 1 == 0 => always false, set v && !v
                            // Or tree : 1 != 0 => always true, set v || !v
                            m_ಬಿಟ್_Polarities.emplace_ಹಿಂಭಾಗ(ref, ನಿಜ, 0);
                            m_ಬಿಟ್_Polarities.emplace_ಹಿಂಭಾಗ(ref, ಸುಳ್ಳು, 0);
                            ನಿಲ್ಲಿಸು;
                        } ಇಲ್ಲದಿದ್ದರೆ {  // This bitIdx is irrelevant
                            ಮುಂದುವರಿ;
                        }
                    }
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ polarity = comp_ಅಂಕೆ.ಬಿಟ್_Is1(ಮುಖವಾಡ_Idx) != ಮುಖವಾಡ_Flip;
                    m_ಬಿಟ್_Polarities.emplace_ಹಿಂಭಾಗ(ref, polarity, ಬಿಟ್_Idx);
                }
            };

            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAnd* ಸ್ಥಿರಾಂಕ andp = VN_CAST(nodep->rhsp(), And)) {  // comp == (mask & v)
                ಸ್ಥಿರಾಂಕ ಎಲೆ_Info& ಮುಖವಾಡ = ಕಂಡುಹಿಡಿ_ಎಲೆ(andp->lhsp(), ನಿಜ);
                CONST_BITOP_RETURN_IF(!ಮುಖವಾಡ.constp() || ಮುಖವಾಡ.lsb() != 0, andp->lhsp());

                ಸ್ಥಿರಾಂಕ ಎಲೆ_Info& ref = ಕಂಡುಹಿಡಿ_ಎಲೆ(andp->rhsp(), ಸುಳ್ಳು);
                CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());

                restorer.disableRestore();  // Now all checks passed

                ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಮುಖವಾಡ_ಅಂಕೆ = ಮುಖವಾಡ.constp()->ಅಂಕೆ();

                incrOps(nodep, __LINE__);
                incrOps(andp, __LINE__);

                ಗಣ_Polarities(ref, &ಮುಖವಾಡ_ಅಂಕೆ);
            } ಇಲ್ಲದಿದ್ದರೆ {  // comp == v
                ಸ್ಥಿರಾಂಕ ಎಲೆ_Info& ref = ಕಂಡುಹಿಡಿ_ಎಲೆ(nodep->rhsp(), ಸುಳ್ಳು);
                CONST_BITOP_RETURN_IF(!ref.refp(), nodep->rhsp());

                restorer.disableRestore();  // Now all checks passed

                incrOps(nodep, __LINE__);

                ಗಣ_Polarities(ref, ಶೂನ್ಯಸೂಚಕ);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            CONST_BITOP_ಗಣ_FAILED("Mixture of different ops cannot be optimized", nodep);
        }
    }

    // CONSTRUCTORS
    Const_ಬಿಟ್_Op_ವೃಕ್ಷ_Visitor(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಚಿಹ್ನೆರಹಿತ externalOps, VNIdAllocator& ids)
        : m_ops{externalOps}
        , m_rootp{nodep}
        , m_ids{ids} {
        // Fill nullptr at [0] because a base index of 0 means not set yet
        m_varInfos.ತಳ್ಳು_ಹಿಂಭಾಗ(ಶೂನ್ಯಸೂಚಕ);
        CONST_BITOP_RETURN_IF(!is_And_ವೃಕ್ಷ() && !is_Or_ವೃಕ್ಷ() && !is_Xor_ವೃಕ್ಷ(), nodep);
        ಆದರೆ (Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ biopp = VN_CAST(nodep, ಗಂಟು_Biop)) {
            iterateConst(biopp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            UASSERT_OBJ(VN_IS(nodep, RedXor), nodep, "Must be RedXor");
            incrOps(nodep, __LINE__);
            iterateChildrenConst(nodep);
        }
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ&& ನಮೂದು : m_ಬಿಟ್_Polarities) {
            getVarInfo(ನಮೂದು.m_info).ಗಣ_Polarity(ನಮೂದು.m_polarity, ನಮೂದು.m_ಬಿಟ್);
        }
        UASSERT_OBJ(is_Xor_ವೃಕ್ಷ() || m_polarity, nodep, "must be the original polarity");
    }
    ವಾಸ್ತವಿಕ ~Const_ಬಿಟ್_Op_ವೃಕ್ಷ_Visitor() = ಪೂರ್ವನಿಯೋಜಿತ;
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು CONST_BITOP_RETURN_IF
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು CONST_BITOP_ಗಣ_FAILED

ಸಾರ್ವಜನಿಕ:
    // Transform as below.
    // v[0] & v[1] => 2'b11 == (2'b11 & v)
    // v[0] | v[1] => 2'b00 != (2'b11 & v)
    // v[0] ^ v[1] => ^{2'b11 & v}
    // (3'b011 == (3'b011 & v)) & v[2]  => 3'b111 == (3'b111 & v)
    // (3'b000 != (3'b011 & v)) | v[2]  => 3'b000 != (3'b111 & v)
    // Reduction ops are transformed in the same way.
    // &{v[0], v[1]} => 2'b11 == (2'b11 & v)
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* simplify(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಪೂರ್ಣಾಂಕ ಫಲಿತಾಂಶ_ಅಗಲ, ಚಿಹ್ನೆರಹಿತ externalOps,
                                 VDouble0& reduction, VNIdAllocator& ids) {
        UASSERT_OBJ(1 <= ಫಲಿತಾಂಶ_ಅಗಲ && ಫಲಿತಾಂಶ_ಅಗಲ <= 64, nodep, "resultWidth out of range");

        // Walk tree, gathering all terms referenced in expression
        ಸ್ಥಿರಾಂಕ Const_ಬಿಟ್_Op_ವೃಕ್ಷ_Visitor visitor{nodep, externalOps, ids};

        // If failed on root node is not optimizable, or there are no variable terms, then done
        ಆದರೆ (visitor.m_failed || visitor.m_varInfos.ಗಾತ್ರ_2() == 1) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        // FileLine used for constructing all new nodes in this function
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();

        // Get partial result each term referenced, count total number of ops and keep track of
        // whether we have clean/dirty terms. visitor.m_varInfos appears in deterministic order,
        // so the optimized tree is deterministic as well.

        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> termps;
        termps.reserve(visitor.m_varInfos.ಗಾತ್ರ_2() - 1);
        ಚಿಹ್ನೆರಹಿತ ಫಲಿತಾಂಶ_Ops = 0;
        ತಾರ್ಕಿಕ has_Clean_ಪದ = ಸುಳ್ಳು;
        ತಾರ್ಕಿಕ has_Dirty_ಪದ = ಸುಳ್ಳು;

        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ&& v : visitor.m_varInfos) {
            ಆದರೆ (!v) ಮುಂದುವರಿ;  // Skip nullptr at m_varInfos[0]
            ಆದರೆ (v->has_Const_ಫಲಿತಾಂಶ()) {
                // If a constant term is known, we can either drop it or the whole tree is constant
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (v->get_Const_ಫಲಿತಾಂಶ()) {
                    UASSERT_OBJ(visitor.is_Or_ವೃಕ್ಷ(), nodep,
                                "Only OR tree can yield known 1 result");
                    UINFO(9, "OR tree with const 1 term: " << v->refp());
                    // Known 1 bit in OR tree, whole result is 1
                    resultp = ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}};
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (visitor.is_And_ವೃಕ್ಷ()) {
                    UINFO(9, "AND tree with const 0 term: " << v->refp());
                    // Known 0 bit in AND tree, whole result is 0
                    resultp = ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Known 0 bit in OR or XOR tree. Ignore it.
                    ಮುಂದುವರಿ;
                }
                // Set width and widthMin precisely
                resultp->dtype_Chg_ಅಗಲ(ಫಲಿತಾಂಶ_ಅಗಲ, 1);
                ಗಾಗಿ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ termp : termps) VL_DO_DANGLING(termp->delete_ವೃಕ್ಷ(), termp);
                ಹಿಂತಿರುಗಿ resultp;
            }
            ಸ್ಥಿರಾಂಕ ಫಲಿತಾಂಶ_ಪದ ಫಲಿತಾಂಶ = v->get_ಫಲಿತಾಂಶ_ಪದ();
            termps.ತಳ್ಳು_ಹಿಂಭಾಗ(std::get<0>(ಫಲಿತಾಂಶ));
            ಫಲಿತಾಂಶ_Ops += std::get<1>(ಫಲಿತಾಂಶ);
            ಆದರೆ (std::get<2>(ಫಲಿತಾಂಶ)) {
                has_Clean_ಪದ = ನಿಜ;
                UINFO(9, "Clean term: " << termps.ಹಿಂಭಾಗ());
            } ಇಲ್ಲದಿದ್ದರೆ {
                has_Dirty_ಪದ = ನಿಜ;
                UINFO(9, "Dirty term: " << termps.ಹಿಂಭಾಗ());
            }
        }

        // Group by FrozenNodeInfo
        std::map<Frozen_ಗಂಟು_Info, std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> frozenNodes;
        // Check if frozen terms are clean or not
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& frozenInfo : visitor.m_frozenNodes) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ termp = frozenInfo.ಮೊದಲ;
            // Comparison operators are clean
            ಆದರೆ ((VN_IS(termp, Eq) || VN_IS(termp, Neq) || VN_IS(termp, Lt) || VN_IS(termp, Lte)
                 || VN_IS(termp, Gt) || VN_IS(termp, Gte))
                && frozenInfo.ಎರಡನೇ.m_lsb == 0) {
                has_Clean_ಪದ = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Otherwise, conservatively assume the frozen term is dirty
                has_Dirty_ಪದ = ನಿಜ;
                UINFO(9, "Dirty frozen term: " << termp);
            }
            frozenNodes[frozenInfo.ಎರಡನೇ].ತಳ್ಳು_ಹಿಂಭಾಗ(termp);
        }

        // Figure out if a final negation is required
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ needsFlip = visitor.is_Xor_ವೃಕ್ಷ() && !visitor.m_polarity;

        // Figure out if the final tree needs cleaning
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ needsCleaning = visitor.is_And_ವೃಕ್ಷ() ? !has_Clean_ಪದ : has_Dirty_ಪದ;

        // Add size of reduction tree to op count
        ಫಲಿತಾಂಶ_Ops += termps.ಗಾತ್ರ_2() - 1;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& lsbAndNodes : frozenNodes) {
            ಆದರೆ (lsbAndNodes.ಮೊದಲ.m_lsb > 0) ++ಫಲಿತಾಂಶ_Ops;  // Needs AstShiftR
            ಆದರೆ (!lsbAndNodes.ಮೊದಲ.m_polarity) ++ಫಲಿತಾಂಶ_Ops;  // Needs AstNot
            ಫಲಿತಾಂಶ_Ops += lsbAndNodes.ಎರಡನೇ.ಗಾತ್ರ_2();
        }
        // Add final polarity flip in Xor tree
        ಆದರೆ (needsFlip) ++ಫಲಿತಾಂಶ_Ops;
        // Add final cleaning AND
        ಆದರೆ (needsCleaning) ++ಫಲಿತಾಂಶ_Ops;

        ಆದರೆ (debug() >= 9) {  // LCOV_EXCL_START
            ಹೊರಗೆ << "-  Bitop tree considered:\n";
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ termp : termps) termp->dump_ವೃಕ್ಷ("-  Reduced term: ");
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Frozen_ಗಂಟು_Info>& termp : visitor.m_frozenNodes) {
                termp.ಮೊದಲ->dump_ವೃಕ್ಷ("-  Frozen term with lsb "
                                      + std::ಗಮ್ಯ_string(termp.ಎರಡನೇ.m_lsb) + " polarity "
                                      + std::ಗಮ್ಯ_string(termp.ಎರಡನೇ.m_polarity) + ": ");
            }
            ಹೊರಗೆ << "-  Needs flipping: " << needsFlip << "\n";
            ಹೊರಗೆ << "-  Needs cleaning: " << needsCleaning << "\n";
            ಹೊರಗೆ << "-  Size: " << ಫಲಿತಾಂಶ_Ops << " input size: " << visitor.m_ops << "\n";
        }  // LCOV_EXCL_STOP

        // Sometimes we have no terms left after ignoring redundant terms
        // (all of which were zeroes)
        ಆದರೆ (termps.ಖಾಲಿಗಳು() && visitor.m_frozenNodes.ಖಾಲಿಗಳು()) {
            reduction += visitor.m_ops;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ resultp = needsFlip ? ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}}
                                                   : ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}};
            resultp->dtype_Chg_ಅಗಲ(ಫಲಿತಾಂಶ_ಅಗಲ, 1);
            ಹಿಂತಿರುಗಿ resultp;
        }

        // Only substitute the result if beneficial as determined by operation count
        ಆದರೆ (visitor.m_ops <= ಫಲಿತಾಂಶ_Ops) {
            ಗಾಗಿ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ termp : termps) VL_DO_DANGLING(termp->delete_ವೃಕ್ಷ(), termp);
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }

        // Update statistics
        reduction += visitor.m_ops - ಫಲಿತಾಂಶ_Ops;

        // Reduction op to combine terms
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ reduce = [&visitor, fl](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            ಆದರೆ (!lhsp) ಹಿಂತಿರುಗಿ rhsp;
            ಆದರೆ (visitor.is_And_ವೃಕ್ಷ()) {
                ಹಿಂತಿರುಗಿ ಹೊಸ AstAnd{fl, lhsp, rhsp};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (visitor.is_Or_ವೃಕ್ಷ()) {
                ಹಿಂತಿರುಗಿ ಹೊಸ AstOr{fl, lhsp, rhsp};
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹಿಂತಿರುಗಿ ಹೊಸ AstXor{fl, lhsp, rhsp};
            }
        };

        // Compute result by reducing all terms
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ termp : termps) {  //
            resultp = reduce(resultp, termp);
        }
        // Add any frozen terms to the reduction
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ&& nodes : frozenNodes) {
            // nodes.second has same lsb and polarity
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* termp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ itemp : nodes.ಎರಡನೇ) {
                termp = reduce(termp, itemp->unlink_Fr_ಹಿಂಭಾಗ());
            }
            ಆದರೆ (nodes.ಮೊದಲ.m_lsb > 0) {  // LSB is not 0, so shiftR
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = termp->dtypep();
                termp = ಹೊಸ Ast_ಸರಿಸು_R{termp->fileline(), termp,
                                      ಹೊಸ AstConst(termp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{},
                                                   termp->ಅಗಲ(), nodes.ಮೊದಲ.m_lsb)};
                termp->dtypep(dtypep);
            }
            ಆದರೆ (!nodes.ಮೊದಲ.m_polarity) {  // Polarity is inverted, so append Not
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = termp->dtypep();
                termp = ಹೊಸ AstNot{termp->fileline(), termp};
                termp->dtypep(dtypep);
            }
            resultp = reduce(resultp, termp);
        }

        // Set width of masks to expected result width. This is required to prevent later removal
        // of the masking node e.g. by the "AND with all ones" rule. If the result width happens
        // to be 1, we still need to ensure the AstAnd is not dropped, so use a wider mask in this
        // special case.
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಮುಖವಾಡ_ಅಗಲ
            = std::max(resultp->ಅಗಲ(), ಫಲಿತಾಂಶ_ಅಗಲ == 1 ? VL_IDATASIZE : ಫಲಿತಾಂಶ_ಅಗಲ);

        // Apply final polarity flip
        ಆದರೆ (needsFlip) {
            ಆದರೆ (needsCleaning) {
                // Cleaning will be added below. Use a NOT which is a byte shorter on x86
                resultp = ಹೊಸ AstNot{fl, resultp};
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Keep result clean by using XOR(1, _)
                AstConst* ಸ್ಥಿರಾಂಕ maskp = ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, ಮುಖವಾಡ_ಅಗಲ, 1};
                resultp = ಹೊಸ AstXor{fl, maskp, resultp};
            }
        }

        // Apply final cleaning
        ಆದರೆ (needsCleaning) {
            AstConst* ಸ್ಥಿರಾಂಕ maskp = ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, ಮುಖವಾಡ_ಅಗಲ, 1};
            resultp = ಹೊಸ AstAnd{fl, maskp, resultp};
        }

        // Cast back to original size if required
        ಆದರೆ (resultp->ಅಗಲ() != ಫಲಿತಾಂಶ_ಅಗಲ) {
            resultp = ಹೊಸ AstCCast{fl, resultp, ಫಲಿತಾಂಶ_ಅಗಲ, 1};
        }

        // Set width and widthMin precisely
        resultp->dtype_Chg_ಅಗಲ(ಫಲಿತಾಂಶ_ಅಗಲ, 1);

        ಹಿಂತಿರುಗಿ resultp;
    }
};

//######################################################################
// Const state, as a visitor of each AstNode

ವರ್ಗ ConstVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // CONSTANTS
    ಸ್ಥಿರ constexpr ಚಿಹ್ನೆರಹಿತ CONCAT_MERGABLE_MAX_ಆಳ = 10;  // Limit alg recursion
    // Largest right-hand side that is copied to each part of a split concatenation assignment
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ CONCAT_ನಿಯೋಜಿಸು_MAX_RHS_NODES = 16;

    // NODE STATE
    // See VNIdAllocator. All per node state below is held in side tables keyed on the
    // node ids it hands out, as node pointers are not stable keys within V3Const.

    // STATE
    VNIdAllocator m_ids;  // Node id allocator
    ತಾರ್ಕಿಕ m_params = ಸುಳ್ಳು;  // If true, propagate parameterized and true numbers only
    ತಾರ್ಕಿಕ m_required = ಸುಳ್ಳು;  // If true, must become a constant
    ತಾರ್ಕಿಕ m_wremove = ನಿಜ;  // Inside scope, no assignw removal
    ತಾರ್ಕಿಕ m_warn = ಸುಳ್ಳು;  // Output warnings
    ತಾರ್ಕಿಕ m_doExpensive = ಸುಳ್ಳು;  // Enable computationally expensive optimizations
    ತಾರ್ಕಿಕ m_doCpp = ಸುಳ್ಳು;  // Enable late-stage C++ optimizations
    ತಾರ್ಕಿಕ m_doNConst = ಸುಳ್ಳು;  // Enable non-constant-child simplifications
    ತಾರ್ಕಿಕ m_doV = ಸುಳ್ಳು;  // Verilog, not C++ conversion
    ತಾರ್ಕಿಕ m_doGenerate = ಸುಳ್ಳು;  // Postpone width checking inside generate
    ತಾರ್ಕಿಕ m_convert_Logic_ಗಮ್ಯ_ಬಿಟ್ = ಸುಳ್ಳು;  // Convert logical operators to bitwise
    ತಾರ್ಕಿಕ m_hasJumpDelay = ಸುಳ್ಳು;  // JumpGo or Delay under this loop
    ತಾರ್ಕಿಕ m_hasLoopTest = ಸುಳ್ಳು;  // Contains AstLoopTest
    ತಾರ್ಕಿಕ m_underRecFunc = ಸುಳ್ಳು;  // Under a recursive function
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    ಸ್ಥಿರಾಂಕ AstArraySel* m_selp = ಶೂನ್ಯಸೂಚಕ;  // Current select
    ಸ್ಥಿರಾಂಕ AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // Current scope
    ಸ್ಥಿರಾಂಕ Ast_Attr_ದ* m_attrp = ಶೂನ್ಯಸೂಚಕ;  // Current attribute
    VDouble0 m_stat_ಬಿಟ್_Op_Reduction;  // Ops reduced in ConstBitOpTreeVisitor
    VDouble0 m_stat_Concat_ವಿಲೀನ;  // Concat merges
    VDouble0 m_stat_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant;  // Conditional repeated expressions
    VDouble0 m_stat_If_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant;  // Conditional repeated expressions
    VDouble0 m_statMemberAccessVisits;  // Nodes visited by containsMemberAccessRecurse
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_globalPass;  // ConstVisitor invoked as a global pass
    ಸ್ಥಿರ uint32_t s_global_Pass_ಅಂಕೆ;  // Counts number of times ConstVisitor invoked as global pass
    V3_Unique_ಹೆಸರುಗಳು m_concswap_ಹೆಸರುಗಳು;  // For generating unique temporary variable names
    // Caches results of matchBiopToBitwise, keyed on node id
    std::unordered_map<ಗಾತ್ರ_t, ತಾರ್ಕಿಕ> m_ಒಳಗೊಂಡಿದೆ_Member_Access;
    // Items of enum currently being recursed into. Cannot use VNIdAllocator, as this runs in
    // every mode, including those invoked under a pass holding user4 (V3Param). Node pointers
    // are safe as keys here, unlike elsewhere in V3Const: folding the item value below does
    // free nodes whose addresses are then reused, but AstEnumItem is only ever constructed
    // while parsing (verilog.y and V3LinkParse), so a reused address can never become one,
    // and every key looked up is an AstEnumItem.
    std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ*> m_recursing_Enum_ವಸ್ತುಗಳು;
    std::unordered_ಗಣ<Ast_Jump_ಖಂಡ*> m_ಬಳಸಿದ_Jump_ಖಂಡಗಳು;  // JumpBlocks used by some JumpGo

    // METHODS

    // Return true if statements may be deleted. Until statics are lifted, a variable declared
    // in a statement might be referenced from elsewhere (e.g. hierarchically).
    ಸ್ಥಿರ ತಾರ್ಕಿಕ canDeleteStatements() { ಹಿಂತಿರುಗಿ v3Global.staticsLifted(); }

    ಶೂನ್ಯ deleteVarScopesUnder(Ast_ಗಂಟು* subtreep) {
        ಆದರೆ (!subtreep) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!m_scopep) ಹಿಂತಿರುಗಿ;
        std::unordered_ಗಣ<AstVar*> varps;
        subtreep->foreach_And_ಮುಂದಿನ([&](AstVar* varp) { varps.ಸೇರಿಸು(varp); });
        ಆದರೆ (varps.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;
        ಗಾಗಿ (AstVarScope *vscp = m_scopep->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            ಆದರೆ (varps.ಕಂಡುಹಿಡಿ(vscp->varp()) != varps.ಕೊನೆ()) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(vscp->unlink_Fr_ಹಿಂಭಾಗ()), vscp);
            }
        }
    }

    V3_ಸಂಖ್ಯೆ const_ಅಂಕೆ_V(Ast_ಗಂಟು* nodep) {
        // Contract C width to V width (if needed, else just direct copy)
        // The upper zeros in the C representation can otherwise cause
        // wrong results in some operations, e.g. MulS
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& numc = VN_AS(nodep, Const)->ಅಂಕೆ();
        ಹಿಂತಿರುಗಿ !numc.is_ಸಂಖ್ಯೆ() ? numc : V3_ಸಂಖ್ಯೆ{nodep, nodep->ಅಗಲ_Min_V(), numc};
    }
    V3_ಸಂಖ್ಯೆ ಗಮ್ಯ_ಅಂಕೆ_C(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& numv) {
        // Extend V width back to C width for given node
        ಹಿಂತಿರುಗಿ !numv.is_ಸಂಖ್ಯೆ() ? numv : V3_ಸಂಖ್ಯೆ{nodep, nodep->ಅಗಲ(), numv};
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಅಧೋ_As_Fixed_Aggregate(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep) {
        ಹಿಂತಿರುಗಿ dtypep->isStreamableFixedAggregate() && dtypep->ಒಳಗೊಂಡಿದೆ_Unpacked_Struct();
    }

    AstStructSel* newStructSel(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp, ಸ್ಥಿರಾಂಕ AstMemberDType* ಸ್ಥಿರಾಂಕ itemp) {
        AstStructSel* ಸ್ಥಿರಾಂಕ selp = ಹೊಸ AstStructSel{fromp->fileline(), fromp, itemp->name()};
        selp->dtypeFrom(itemp->dtypep());
        ಹಿಂತಿರುಗಿ selp;
    }

    ಶೂನ್ಯ collectFixedAggregateTerms(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp, std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& termps,
                                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ packReal) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = fromp->dtypep()->skipRefp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ unpackDtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ idx : unpackDtypep->declRange().seq_ಎಡ_ಗಮ್ಯ_ಬಲ()) {
                AstArraySel* ಸ್ಥಿರಾಂಕ selp
                    = ಹೊಸ AstArraySel{fromp->fileline(), fromp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), idx};
                collectFixedAggregateTerms(selp, termps, packReal);
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(fromp), fromp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ sdtypep
                   = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
            ಆದರೆ (sdtypep->packed()) {
                termps.ತಳ್ಳು_ಹಿಂಭಾಗ(fromp);
                ಹಿಂತಿರುಗಿ;
            }
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                collectFixedAggregateTerms(newStructSel(fromp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), itemp),
                                           termps, packReal);
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(fromp), fromp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (packReal && dtypep->isDouble()) {
            termps.ತಳ್ಳು_ಹಿಂಭಾಗ(ಹೊಸ Ast_Real_ಗಮ್ಯ_ಬಿಟ್ಟುಗಳು{fromp->fileline(), fromp});
        } ಇಲ್ಲದಿದ್ದರೆ {
            termps.ತಳ್ಳು_ಹಿಂಭಾಗ(fromp);
        }
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* packFixedAggregate(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp) {
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> termps;
        collectFixedAggregateTerms(fromp, termps, ನಿಜ);
        UASSERT(!termps.ಖಾಲಿಗಳು(), "No stream terms");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = termps[0];
        ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < termps.ಗಾತ್ರ_2(); ++i) {
            resultp = ಹೊಸ AstConcat{resultp->fileline(), resultp, termps[i]};
        }
        ಹಿಂತಿರುಗಿ resultp;
    }

    ಶೂನ್ಯ replace_ನಿಯೋಜಿಸು_ಗಮ್ಯ_Fixed_Aggregate(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ dstp,
                                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ dst_ಅಗಲ = dstp->dtypep()->ಅಗಲ_ಹರಿವು();
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> termps;
        collectFixedAggregateTerms(dstp, termps, ಸುಳ್ಳು);
        ಪೂರ್ಣಾಂಕ src_ಅಗಲ = srcp->ಅಗಲ();
        ಆದರೆ (src_ಅಗಲ < dst_ಅಗಲ) {
            AstExtend* ಸ್ಥಿರಾಂಕ extendp = ಹೊಸ AstExtend{srcp->fileline(), srcp, dst_ಅಗಲ};
            srcp = ಹೊಸ Ast_ಸರಿಸು_L{
                extendp->fileline(), extendp,
                ಹೊಸ AstConst{extendp->fileline(), static_cast<uint32_t>(dst_ಅಗಲ - src_ಅಗಲ)},
                dst_ಅಗಲ};
            src_ಅಗಲ = dst_ಅಗಲ;
        }
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* newp = ಶೂನ್ಯಸೂಚಕ;
        ಪೂರ್ಣಾಂಕ ಸ್ಥಾನಾಂತರ = 0;
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < termps.ಗಾತ್ರ_2(); ++i) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ termp = termps[i];
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = termp->dtypep()->ಅಗಲ_ಹರಿವು();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb = src_ಅಗಲ - ಸ್ಥಾನಾಂತರ - ಅಗಲ;
            ಸ್ಥಾನಾಂತರ += ಅಗಲ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp
                = ಹೊಸ AstSel{srcp->fileline(), srcp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), lsb, ಅಗಲ};
            ಆದರೆ (termp->dtypep()->skipRefp()->isDouble()) {
                rhsp = ಹೊಸ Ast_ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Real_D{rhsp->fileline(), rhsp};
            }
            Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = nodep->cloneType(termp, rhsp);
            assignp->dtypeFrom(termp);
            newp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(newp, assignp);
        }
        nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(srcp), srcp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }

    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Const(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) { ಹಿಂತಿರುಗಿ VN_IS(nodep, Const); }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Asv_Const(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        // BIASV(CONST, BIASV(CONST,...)) -> BIASV( BIASV_CONSTED(a,b), ...)
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Bi_Com_Asv* ಸ್ಥಿರಾಂಕ bnodep = VN_CAST(nodep, ಗಂಟು_Bi_Com_Asv);
        ಆದರೆ (!bnodep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!VN_IS(bnodep->lhsp(), Const)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Bi_Com_Asv* ಸ್ಥಿರಾಂಕ rnodep = VN_CAST(bnodep->rhsp(), ಗಂಟು_Bi_Com_Asv);
        ಆದರೆ (!rnodep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (rnodep->type() != bnodep->type()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (rnodep->ಅಗಲ() != bnodep->ಅಗಲ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (rnodep->lhsp()->ಅಗಲ() != bnodep->lhsp()->ಅಗಲ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!VN_IS(rnodep->lhsp(), Const)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Asv_Same(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        // BIASV(SAMEa, BIASV(SAMEb,...)) -> BIASV( BIASV(SAMEa,SAMEb), ...)
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Bi_Com_Asv* ಸ್ಥಿರಾಂಕ bnodep = VN_CAST(nodep, ಗಂಟು_Bi_Com_Asv);
        ಆದರೆ (!bnodep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Bi_Com_Asv* ಸ್ಥಿರಾಂಕ rnodep = VN_CAST(bnodep->rhsp(), ಗಂಟು_Bi_Com_Asv);
        ಆದರೆ (!rnodep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (rnodep->type() != bnodep->type()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (rnodep->ಅಗಲ() != bnodep->ಅಗಲ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ operandsSame(bnodep->lhsp(), rnodep->lhsp());
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Asv_LUp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
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
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Bi_Com_Asv* ಸ್ಥಿರಾಂಕ bnodep = VN_CAST(nodep, ಗಂಟು_Bi_Com_Asv);
        ಆದರೆ (!bnodep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Bi_Com_Asv* ಸ್ಥಿರಾಂಕ lnodep = VN_CAST(bnodep->lhsp(), ಗಂಟು_Bi_Com_Asv);
        ಆದರೆ (!lnodep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (lnodep->type() != bnodep->type()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (lnodep->ಅಗಲ() != bnodep->ಅಗಲ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ VN_IS(lnodep->lhsp(), Const);
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Asv_RUp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr)) ?
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Bi_Com_Asv* ಸ್ಥಿರಾಂಕ bnodep = VN_CAST(nodep, ಗಂಟು_Bi_Com_Asv);
        ಆದರೆ (!bnodep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Bi_Com_Asv* ಸ್ಥಿರಾಂಕ rnodep = VN_CAST(bnodep->rhsp(), ಗಂಟು_Bi_Com_Asv);
        ಆದರೆ (!rnodep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (rnodep->type() != bnodep->type()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (rnodep->ಅಗಲ() != bnodep->ಅಗಲ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ VN_IS(rnodep->lhsp(), Const);
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Sub_ಕೂಡಿಸು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        // SUB( ADD(CONSTx,y), CONSTz) -> ADD(SUB(CONSTx,CONSTz), y)
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ np = VN_CAST(nodep, ಗಂಟು_Biop);
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ lp = VN_CAST(np->lhsp(), ಗಂಟು_Biop);
        ಹಿಂತಿರುಗಿ (lp && VN_IS(lp->lhsp(), Const) && VN_IS(np->rhsp(), Const)
                && lp->ಅಗಲ() == np->ಅಗಲ());
    }
    ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ_Redundant_Clean(AstAnd* andp) {
        // Remove And with constant one inserted by V3Clean
        // 1 & (a == b)  -> (IData)(a == b)
        // When bool is casted to int, the value is either 0 or 1
        AstConst* ಸ್ಥಿರಾಂಕ constp = VN_AS(andp->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isOne(), andp->lhsp(), "TRREEOPC must meet this condition");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = andp->rhsp();
        AstCCast* ccastp = ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ isEqOrNeq = [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) -> ತಾರ್ಕಿಕ {  //
            ಹಿಂತಿರುಗಿ VN_IS(nodep, Eq) || VN_IS(nodep, Neq);
        };
        ಆದರೆ (isEqOrNeq(rhsp)) {
            ccastp = ಹೊಸ AstCCast{andp->fileline(), rhsp->unlink_Fr_ಹಿಂಭಾಗ(), andp};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstCCast* ಸ್ಥಿರಾಂಕ tmpp = VN_CAST(rhsp, CCast)) {
            ಆದರೆ (isEqOrNeq(tmpp->lhsp())) {
                ಆದರೆ (tmpp->ಅಗಲ() == andp->ಅಗಲ()) {
                    tmpp->unlink_Fr_ಹಿಂಭಾಗ();
                    ccastp = tmpp;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ccastp = ಹೊಸ AstCCast{andp->fileline(), tmpp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), andp};
                }
            }
        }
        ಆದರೆ (ccastp) {
            andp->replaceWithKeepDType(ccastp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(andp), andp);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_And_Or_Same(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        // OR( AND(VAL,x), AND(VAL,y)) -> AND(VAL,OR(x,y))
        // OR( AND(x,VAL), AND(y,VAL)) -> AND(OR(x,y),VAL)
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ np = VN_CAST(nodep, ಗಂಟು_Biop);
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ lp = VN_CAST(np->lhsp(), ಗಂಟು_Biop);
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ rp = VN_CAST(np->rhsp(), ಗಂಟು_Biop);
        ಹಿಂತಿರುಗಿ (lp && rp && lp->ಅಗಲ() == rp->ಅಗಲ() && lp->type() == rp->type()
                && (operandsSame(lp->lhsp(), rp->lhsp()) || operandsSame(lp->rhsp(), rp->rhsp())));
    }
    ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ_Or_And_Not(Ast_ಗಂಟು_Biop* nodep) {
        // AstOr{$a, AstAnd{AstNot{$b}, $c}} if $a.width1, $a==$b => AstOr{$a,$c}
        // Someday we'll sort the biops completely and this can be simplified
        // This often results from our simplified clock generation:
        // if (rst) ... else if (enable)... -> OR(rst,AND(!rst,enable))
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ap;
        Ast_ಗಂಟು_Biop* andp;
        ಆದರೆ (VN_IS(nodep->lhsp(), And)) {
            andp = VN_AS(nodep->lhsp(), And);
            ap = nodep->rhsp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep->rhsp(), And)) {
            andp = VN_AS(nodep->rhsp(), And);
            ap = nodep->lhsp();
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Uniop* notp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* cp;
        ಆದರೆ (VN_IS(andp->lhsp(), Not)) {
            notp = VN_AS(andp->lhsp(), Not);
            cp = andp->rhsp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(andp->rhsp(), Not)) {
            notp = VN_AS(andp->rhsp(), Not);
            cp = andp->lhsp();
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bp = notp->lhsp();
        ಆದರೆ (!operandsSame(ap, bp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Do it
        cp->unlink_Fr_ಹಿಂಭಾಗ();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(andp->unlink_Fr_ಹಿಂಭಾಗ()), andp);
        VL_DANGLING(notp);
        // Replace whichever branch is now dangling
        ಆದರೆ (nodep->rhsp()) {
            nodep->lhsp(cp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->rhsp(cp);
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ_And_Cond(AstAnd* nodep) {
        // Push down a AND into conditional, when one side of conditional is constant
        // (otherwise we'd be trading one operation for two operations)
        // V3Clean often makes this pattern, as it postpones the AND until
        // as high as possible, which is usually the right choice, except for this.
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstCond* ಸ್ಥಿರಾಂಕ condp = VN_CAST(nodep->rhsp(), Cond);
        ಆದರೆ (!condp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!VN_IS(condp->thenp(), Const) && !VN_IS(condp->elsep(), Const)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstConst* ಸ್ಥಿರಾಂಕ maskp = VN_CAST(nodep->lhsp(), Const);
        ಆದರೆ (!maskp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        UINFO(4, "AND(CONSTm, CONDcond(c, i, e))->CONDcond(c, AND(m,i), AND(m, e)) " << nodep);
        AstCond* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstCond{
            condp->fileline(), condp->condp()->unlink_Fr_ಹಿಂಭಾಗ(),
            ಹೊಸ AstAnd{nodep->fileline(), maskp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), condp->thenp()->unlink_Fr_ಹಿಂಭಾಗ()},
            ಹೊಸ AstAnd{nodep->fileline(), maskp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                       condp->elsep()->unlink_Fr_ಹಿಂಭಾಗ()}};
        newp->thenp()->dtypeFrom(nodep);  // As And might have been to change widths
        newp->elsep()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ_Cond_Cond(ಸ್ಥಿರಾಂಕ AstCond* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        ಆದರೆ (VN_IS(nodep->backp(), Cond)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Was checked when visited parent
        ಆದರೆ (!VN_IS(nodep->thenp(), Cond) && !VN_IS(nodep->elsep(), Cond))
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Short circuit
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> truesp;
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> falsesp;
        ಹೊಂದಿಕೆ_Cond_Cond_Recurse(nodep, truesp /*ref*/, falsesp /*ref*/);
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Can optimize further
    }
    ಶೂನ್ಯ ಹೊಂದಿಕೆ_Cond_Cond_Recurse(ಸ್ಥಿರಾಂಕ AstCond* nodep, std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& truesp,
                              std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        ಆದರೆ (truesp.ಗಾತ್ರ_2() > 4 || falsesp.ಗಾತ್ರ_2() > 4) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!nodep->condp()->is_ಶುದ್ಧ()) ಹಿಂತಿರುಗಿ;
        ತಾರ್ಕಿಕ replaced = ಸುಳ್ಳು;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp : truesp) {
            ಆದರೆ (replaced) ನಿಲ್ಲಿಸು;
            ಆದರೆ (!operandsSame(nodep->condp(), condp)) ಮುಂದುವರಿ;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_ಅಂಕೆ(nodep->condp(), 1);
            replaced = ನಿಜ;
            ++m_stat_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant;
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp : falsesp) {
            ಆದರೆ (replaced) ನಿಲ್ಲಿಸು;
            ಆದರೆ (!operandsSame(nodep->condp(), condp)) ಮುಂದುವರಿ;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = ನಿಜ;
            ++m_stat_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCond* subCondp = VN_CAST(nodep->thenp(), Cond)) {
            ಆದರೆ (!replaced) truesp.emplace_ಹಿಂಭಾಗ(nodep->condp());
            ಹೊಂದಿಕೆ_Cond_Cond_Recurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            ಆದರೆ (!replaced) truesp.ತೆಗೆ_ಹಿಂಭಾಗ();
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCond* subCondp = VN_CAST(nodep->elsep(), Cond)) {
            ಆದರೆ (!replaced) falsesp.emplace_ಹಿಂಭಾಗ(nodep->condp());
            ಹೊಂದಿಕೆ_Cond_Cond_Recurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            ಆದರೆ (!replaced) falsesp.ತೆಗೆ_ಹಿಂಭಾಗ();
        }
    }
    ಶೂನ್ಯ ಹೊಂದಿಕೆ_If_Cond_Cond(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_If* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        ಆದರೆ (VN_IS(nodep->backp(), If)) ಹಿಂತಿರುಗಿ;  // Was checked when visited parent
        ಆದರೆ (!VN_IS(nodep->thensp(), If) && !VN_IS(nodep->elsesp(), If)) ಹಿಂತಿರುಗಿ;  // Short circuit
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> truesp;
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> falsesp;
        ಹೊಂದಿಕೆ_If_Cond_Cond_Recurse(nodep, truesp /*ref*/, falsesp /*ref*/);
    }
    ಶೂನ್ಯ ಹೊಂದಿಕೆ_If_Cond_Cond_Recurse(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_If* nodep, std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& truesp,
                                std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        ಆದರೆ (truesp.ಗಾತ್ರ_2() > 4 || falsesp.ಗಾತ್ರ_2() > 4) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!nodep->condp()->is_ಶುದ್ಧ()) ಹಿಂತಿರುಗಿ;
        // A constant condition is folded by visit(AstNodeIf), once statements can be deleted
        ಆದರೆ (VN_IS(nodep->condp(), Const)) ಹಿಂತಿರುಗಿ;
        ತಾರ್ಕಿಕ replaced = ಸುಳ್ಳು;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp : truesp) {
            ಆದರೆ (replaced) ನಿಲ್ಲಿಸು;
            ಆದರೆ (!operandsSame(nodep->condp(), condp)) ಮುಂದುವರಿ;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_ಅಂಕೆ(nodep->condp(), 1);
            replaced = ನಿಜ;
            ++m_stat_If_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant;
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp : falsesp) {
            ಆದರೆ (replaced) ನಿಲ್ಲಿಸು;
            ಆದರೆ (!operandsSame(nodep->condp(), condp)) ಮುಂದುವರಿ;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = ನಿಜ;
            ++m_stat_If_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant;
        }
        // We only check the first statement of parent IF is an If
        // So we don't need to check for effects in the executing thensp/elsesp
        // altering the child't condition.  e.g. 'if (x) begin x=1; if (x) end'
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_If* subIfp = VN_CAST(nodep->thensp(), ಗಂಟು_If)) {
            ಆದರೆ (!replaced) truesp.emplace_ಹಿಂಭಾಗ(nodep->condp());
            ಹೊಂದಿಕೆ_If_Cond_Cond_Recurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            ಆದರೆ (!replaced) truesp.ತೆಗೆ_ಹಿಂಭಾಗ();
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_If* subIfp = VN_CAST(nodep->elsesp(), ಗಂಟು_If)) {
            ಆದರೆ (!replaced) falsesp.emplace_ಹಿಂಭಾಗ(nodep->condp());
            ಹೊಂದಿಕೆ_If_Cond_Cond_Recurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            ಆದರೆ (!replaced) falsesp.ತೆಗೆ_ಹಿಂಭಾಗ();
        }
    }

    ಶೂನ್ಯ ಹೊಂದಿಕೆ_If_Same_ಕೊನೆಯ(Ast_ಗಂಟು_If* nodep) {
        ಆದರೆ (!nodep->thensp()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!nodep->elsesp()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!canDeleteStatements()) ಹಿಂತಿರುಗಿ;
        // Iterate bodies in reverse
        Ast_ಗಂಟು* tp = nodep->thensp()->lastp();
        Ast_ಗಂಟು* ep = nodep->elsesp()->lastp();
        ಆಗ (tp && ep && tp->same_ವೃಕ್ಷ(ep)) {
            UASSERT_OBJ(!tp->nextp(), tp, "Expected no nextp");
            UASSERT_OBJ(!ep->nextp(), ep, "Expected no nextp");
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ tPrevp = tp->prevp();
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ ePrevp = ep->prevp();
            nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(tp->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(ep->unlink_Fr_ಹಿಂಭಾಗ()), ep);
            tp = tPrevp;
            ep = ePrevp;
        }
    }

    ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ_Masked_Or(AstAnd* nodep) {
        // Masking an OR with terms that have no bits set under the mask is replaced with masking
        // only the remaining terms. Canonical example as generated by V3Expand is:
        // 0xff & (a << 8 | b >> 24) --> 0xff & (b >> 24)

        // Compute how many significant bits are in the mask
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_AS(nodep->lhsp(), Const);
        ಸ್ಥಿರಾಂಕ uint32_t significant_ಬಿಟ್ಟುಗಳು = constp->ಅಂಕೆ().ಅಗಲ_ಗಮ್ಯ_Fit();

        AstOr* ಸ್ಥಿರಾಂಕ orp = VN_AS(nodep->rhsp(), Or);

        // Predicate for checking whether the bottom 'significantBits' bits of the given expression
        // are all zeroes.
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ checkBottomClear = [=](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) -> ತಾರ್ಕಿಕ {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಸರಿಸು_L* ಸ್ಥಿರಾಂಕ shiftp = VN_CAST(nodep, ಸರಿಸು_L)) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ scp = VN_CAST(shiftp->rhsp(), Const)) {
                    ಹಿಂತಿರುಗಿ scp->ಅಂಕೆ().ಗಮ್ಯ_UInt() >= significant_ಬಿಟ್ಟುಗಳು;
                }
            }
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        };

        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ orLIsRedundant = checkBottomClear(orp->lhsp());
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ orRIsRedundant = checkBottomClear(orp->rhsp());

        ಆದರೆ (orLIsRedundant && orRIsRedundant) {
            nodep->replaceWithKeepDType(
                ಹೊಸ AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (orLIsRedundant) {
            orp->replaceWithKeepDType(orp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(orp), orp);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // input node is still valid, keep going
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (orRIsRedundant) {
            orp->replaceWithKeepDType(orp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(orp), orp);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // input node is still valid, keep going
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
    }
    ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ_Masked_ಸರಿಸು(AstAnd* nodep) {
        // Drop redundant masking of right shift result. E.g: 0xff & ((uint32_t)a >> 24). This
        // commonly appears after V3Expand and the simplification in matchMaskedOr. Similarly,
        // drop redundant masking of left shift result. E.g.: 0xff000000 & ((uint32_t)a << 24).

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ check_ಮುಖವಾಡ = [nodep, ಇದು](ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಮುಖವಾಡ) -> ತಾರ್ಕಿಕ {
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_AS(nodep->lhsp(), Const);
            ಆದರೆ (constp->ಅಂಕೆ().isCaseEq(ಮುಖವಾಡ)) {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp();
                rhsp->unlink_Fr_ಹಿಂಭಾಗ();
                nodep->replaceWithKeepDType(rhsp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        };

        // Check if masking is redundant
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಸರಿಸು_R* ಸ್ಥಿರಾಂಕ shiftp = VN_CAST(nodep->rhsp(), ಸರಿಸು_R)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_ಸಂಖ್ಯೆ ಮುಖವಾಡ_Lo{nodep, nodep->ಅಗಲ()};
                ಮುಖವಾಡ_Lo.ಗಣ_ಮುಖವಾಡ(nodep->ಅಗಲ() - scp->ಅಂಕೆ().ಗಮ್ಯ_UInt());
                ಹಿಂತಿರುಗಿ check_ಮುಖವಾಡ(ಮುಖವಾಡ_Lo);
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಸರಿಸು_L* ಸ್ಥಿರಾಂಕ shiftp = VN_CAST(nodep->rhsp(), ಸರಿಸು_L)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_ಸಂಖ್ಯೆ ಮುಖವಾಡ{nodep, nodep->ಅಗಲ()};
                ಸ್ಥಿರಾಂಕ uint32_t ಸರಿಸು_Amount = scp->ಅಂಕೆ().ಗಮ್ಯ_UInt();
                ಮುಖವಾಡ.ಗಣ_ಮುಖವಾಡ(nodep->ಅಗಲ() - ಸರಿಸು_Amount, ಸರಿಸು_Amount);
                ಹಿಂತಿರುಗಿ check_ಮುಖವಾಡ(ಮುಖವಾಡ);
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ_Masked_Zero(ಸ್ಥಿರಾಂಕ AstAnd* nodep) {
        // Turn masking of known zero bits into constant zero. Commonly appears after V3Expand.
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ maskp = VN_AS(nodep->lhsp(), Const);
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp();
        ಸ್ಥಿರಾಂಕ uint32_t msbP1 = maskp->ಅಂಕೆ().most_ಗಣ_ಬಿಟ್_P1();
        ಆದರೆ (!msbP1) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Don't rewrite, separate rule matches for this
        ಸ್ಥಿರಾಂಕ uint32_t msb = msbP1 - 1;
        ಸ್ಥಿರಾಂಕ uint32_t lsb = maskp->ಅಂಕೆ().ಕನಿಷ್ಠ_ಗಣ_ಬಿಟ್_P1() - 1;

        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಸರಿಸು_L* ಸ್ಥಿರಾಂಕ shiftp = VN_CAST(rhsp, ಸರಿಸು_L)) {
            // 'a << S' forces the low S bits to zero
            ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ scp = VN_CAST(shiftp->rhsp(), Const)) {
                ಹಿಂತಿರುಗಿ scp->ಅಂಕೆ().ಹೊಂದುತ್ತದೆ_ಒಳ_UInt()  //
                       && (scp->ಅಂಕೆ().ಗಮ್ಯ_UInt() > msb);
            }
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಸರಿಸು_R* ಸ್ಥಿರಾಂಕ shiftp = VN_CAST(rhsp, ಸರಿಸು_R)) {
            // 'a >> S' forces the high S bits to zero. Check against the width of the shifted
            // operand, V3Expand can create shifts wider than their inputs. Must use width(),
            // not widthMin(), as bits above widthMin() are not guaranteed to be zero.
            ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ scp = VN_CAST(shiftp->rhsp(), Const)) {
                ಹಿಂತಿರುಗಿ scp->ಅಂಕೆ().ಹೊಂದುತ್ತದೆ_ಒಳ_UInt()
                       && (lsb + scp->ಅಂಕೆ().ಗಮ್ಯ_UInt()
                           >= static_cast<uint32_t>(shiftp->lhsp()->ಅಗಲ()));
            }
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMul* ಸ್ಥಿರಾಂಕ mulp = VN_CAST(rhsp, Mul)) {
            // 'C * a' forces the low N bits to zero where 'C' has low zero bits
            ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ cp = VN_CAST(mulp->lhsp(), Const)) {
                ಹಿಂತಿರುಗಿ cp->ಅಂಕೆ().ಕನಿಷ್ಠ_ಗಣ_ಬಿಟ್_P1() > msb + 1;
            }
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstExtend* ಸ್ಥಿರಾಂಕ extendp = VN_CAST(rhsp, Extend)) {
            // Zero-extension forces the bits above the source width to zero
            ಹಿಂತಿರುಗಿ lsb >= static_cast<uint32_t>(extendp->lhsp()->ಅಗಲ());
        }

        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ_ಬಿಟ್_Op_ವೃಕ್ಷ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (nodep->ಅಗಲ_Min() != 1) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!v3Global.opt.f_Const_ಬಿಟ್_Op_ವೃಕ್ಷ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = nodep->ಅಗಲ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rootp = nodep;
        ಚಿಹ್ನೆರಹಿತ externalOps = 0;
        // Reach past a plain making AND
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAnd* ಸ್ಥಿರಾಂಕ andp = VN_CAST(nodep, And)) {
            ಆದರೆ (isConst(andp->lhsp(), 1)) {
                rootp = andp->rhsp();
                externalOps = 1;
            }
        }

        // Only optimize if rootp is in fact the root of a tree of identical
        // operations, otherwise analysis on an eventually unoptimizable unablanced
        // tree can go O(N^2) as we would re-analyze every time as we move up the
        // tree, repeatedly re-discover the sub-tree is not optimizable.
        ಆದರೆ (rootp->type() == nodep->backp()->type()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        std::string debug_ಪೂರ್ವಪ್ರತ್ಯಯ;
        ಆದರೆ (debug() >= 9) {  // LCOV_EXCL_START
            ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ s_c = 0;
            debug_ಪೂರ್ವಪ್ರತ್ಯಯ = "-  matchBitOpTree[" + std::ಗಮ್ಯ_string(++s_c) + "] ";
            nodep->dump_ವೃಕ್ಷ(debug_ಪೂರ್ವಪ್ರತ್ಯಯ + "INPUT: ");
        }  // LCOV_EXCL_STOP

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = Const_ಬಿಟ್_Op_ವೃಕ್ಷ_Visitor::simplify(rootp, ಅಗಲ, externalOps,
                                                                  m_stat_ಬಿಟ್_Op_Reduction, m_ids);

        ಆದರೆ (newp) {
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Transformed leaf of bit tree to " << newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }

        ಆದರೆ (debug() >= 9) {  // LCOV_EXCL_START
            ಆದರೆ (newp) {
                newp->dump_ವೃಕ್ಷ(debug_ಪೂರ್ವಪ್ರತ್ಯಯ + "RESULT: ");
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹೊರಗೆ << debug_ಪೂರ್ವಪ್ರತ್ಯಯ << "not replaced\n";
            }
        }  // LCOV_EXCL_STOP

        ಹಿಂತಿರುಗಿ newp;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_ಸರಿಸು_Same(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ np = VN_AS(nodep, ಗಂಟು_Biop);
        {
            ಸ್ಥಿರಾಂಕ Ast_ಸರಿಸು_L* ಸ್ಥಿರಾಂಕ lp = VN_CAST(np->lhsp(), ಸರಿಸು_L);
            ಸ್ಥಿರಾಂಕ Ast_ಸರಿಸು_L* ಸ್ಥಿರಾಂಕ rp = VN_CAST(np->rhsp(), ಸರಿಸು_L);
            ಆದರೆ (lp && rp) {
                ಹಿಂತಿರುಗಿ (lp->ಅಗಲ() == rp->ಅಗಲ() && lp->lhsp()->ಅಗಲ() == rp->lhsp()->ಅಗಲ()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        {
            ಸ್ಥಿರಾಂಕ Ast_ಸರಿಸು_R* ಸ್ಥಿರಾಂಕ lp = VN_CAST(np->lhsp(), ಸರಿಸು_R);
            ಸ್ಥಿರಾಂಕ Ast_ಸರಿಸು_R* ಸ್ಥಿರಾಂಕ rp = VN_CAST(np->rhsp(), ಸರಿಸು_R);
            ಆದರೆ (lp && rp) {
                ಹಿಂತಿರುಗಿ (lp->ಅಗಲ() == rp->ಅಗಲ() && lp->lhsp()->ಅಗಲ() == rp->lhsp()->ಅಗಲ()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Huge_ಸರಿಸು_L(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* nodep) {
        ಹಿಂತಿರುಗಿ (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().ಹೊಂದುತ್ತದೆ_ಒಳ_UInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->ಗಮ್ಯ_UInt()
                        >= static_cast<uint32_t>(nodep->ಅಗಲ())))
                && nodep->lhsp()->is_ಶುದ್ಧ());
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Huge_ಸರಿಸು_R(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* nodep) {
        ಹಿಂತಿರುಗಿ (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().ಹೊಂದುತ್ತದೆ_ಒಳ_UInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->ಗಮ್ಯ_UInt()
                        >= static_cast<uint32_t>(nodep->lhsp()->ಅಗಲ())))
                && nodep->lhsp()->is_ಶುದ್ಧ()
                && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), ಹರಿವು_DType)));
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Is_Two(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const);
        ಆದರೆ (!constp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // not constant
        ಆದರೆ (constp->ಅಂಕೆ().isFourState()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // four-state
        ಆದರೆ (nodep->ಅಗಲ() > VL_QUADSIZE) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // too wide
        ಆದರೆ (nodep->isSigned() && constp->ಅಂಕೆ().isNegative()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // signed and negative
        ಹಿಂತಿರುಗಿ constp->ಗಮ್ಯ_UQuad() == 2;
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Is_Twostate(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಹಿಂತಿರುಗಿ (VN_IS(nodep, Const) && !VN_AS(nodep, Const)->ಅಂಕೆ().isFourState());
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Is_Pow_Two(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಆದರೆ (!ಕಾರ್ಯಾಂಶ_Is_Twostate(nodep)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ (1 == VN_AS(nodep, Const)->ಅಂಕೆ().ಎಣಿಕೆ_Ones());
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_ಸರಿಸು_Op(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* nodep) {
        ಆದರೆ (!VN_IS(nodep->rhsp(), Const)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(nodep->lhsp(), ಗಂಟು_Biop);
        ಆದರೆ (!lhsp || !(VN_IS(lhsp, And) || VN_IS(lhsp, Or) || VN_IS(lhsp, Xor))) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (nodep->ಅಗಲ() != lhsp->ಅಗಲ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (nodep->ಅಗಲ() != lhsp->lhsp()->ಅಗಲ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (nodep->ಅಗಲ() != lhsp->rhsp()->ಅಗಲ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // Only do it if we can fruther reduce one of the sides if pulling the shift through:
        // - Either operands are constants
        ಆದರೆ (VN_IS(lhsp->lhsp(), Const)) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (VN_IS(lhsp->rhsp(), Const)) ಹಿಂತಿರುಗಿ ನಿಜ;
        // - Const shift on either side
        ಆದರೆ (Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ llp = VN_CAST(lhsp->lhsp(), ಗಂಟು_Biop)) {
            ಹಿಂತಿರುಗಿ (VN_IS(llp, ಸರಿಸು_L) || VN_IS(llp, ಸರಿಸು_R)) && VN_IS(llp->rhsp(), Const);
        }
        ಆದರೆ (Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ lrp = VN_CAST(lhsp->rhsp(), ಗಂಟು_Biop)) {
            ಹಿಂತಿರುಗಿ (VN_IS(lrp, ಸರಿಸು_L) || VN_IS(lrp, ಸರಿಸು_R)) && VN_IS(lrp->rhsp(), Const);
        }
        // Otherwise we would increase logic size
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_ಸರಿಸು_ಸರಿಸು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* nodep) {
        // We could add a AND though.
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(nodep->lhsp(), ಗಂಟು_Biop);
        ಆದರೆ (!lhsp || !(VN_IS(lhsp, ಸರಿಸು_L) || VN_IS(lhsp, ಸರಿಸು_R))) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // We can only get rid of a<<b>>c or a<<b<<c, with constant b & c
        // because bits may be masked in that process, or (b+c) may exceed the word width.
        ಆದರೆ (!(VN_IS(nodep->rhsp(), Const) && VN_IS(lhsp->rhsp(), Const))) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().isFourState()
            || VN_AS(lhsp->rhsp(), Const)->ಅಂಕೆ().isFourState())
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (nodep->ಅಗಲ() != lhsp->ಅಗಲ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (nodep->ಅಗಲ() != lhsp->lhsp()->ಅಗಲ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_ಶಬ್ದ_OOB(ಸ್ಥಿರಾಂಕ Ast_ಶಬ್ದ_Sel* nodep) {
        // V3Expand may make a arraysel that exceeds the bounds of the array
        // It was an expression, then got constified.  In reality, the WordSel
        // must be wrapped in a Cond, that will be false.
        ಹಿಂತಿರುಗಿ (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ಗಂಟು_Var_Ref)
                && VN_AS(nodep->fromp(), ಗಂಟು_Var_Ref)->access().isReadOnly()
                && (static_cast<ಪೂರ್ಣಾಂಕ>(VN_AS(nodep->bitp(), Const)->ಗಮ್ಯ_UInt())
                    >= VN_AS(nodep->fromp(), ಗಂಟು_Var_Ref)->varp()->ಅಗಲ_ಪದಗಳು()));
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Sel_ತುಂಬಿದ(ಸ್ಥಿರಾಂಕ AstSel* nodep) {
        ಹಿಂತಿರುಗಿ (VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0
                && static_cast<ಪೂರ್ಣಾಂಕ>(nodep->ಅಗಲ_Const()) == nodep->fromp()->ಅಗಲ());
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Sel_Extend(AstSel* nodep) {
        ಆದರೆ (!m_doV) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstExtend* ಸ್ಥಿರಾಂಕ extendp = VN_CAST(nodep->fromp(), Extend);
        ಆದರೆ (!extendp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstConst* ಸ್ಥಿರಾಂಕ lsbp = VN_CAST(nodep->lsbp(), Const);
        ಆದರೆ (!lsbp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = nodep->ಅಗಲ_Const();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb = lsbp->ಗಮ್ಯ_SInt();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ msb = lsb + ಅಗಲ - 1;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = extendp->lhsp();
        ಆದರೆ (!lhsp->is_ಶುದ್ಧ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // Selecting the entire extended expression, replace with that
        ಆದರೆ (lsb == 0 && msb == lhsp->ಅಗಲ() - 1) {
            lhsp->unlink_Fr_ಹಿಂಭಾಗ();
            nodep->replaceWithKeepDType(lhsp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        // Select entirely in the extended part - replace with zero
        ಆದರೆ (lsb >= lhsp->ಅಗಲ()) {
            replaceZero(nodep);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        // Select entirely in the extended expression - replace with select from that
        ಆದರೆ (msb < lhsp->ಅಗಲ()) {
            lhsp->unlink_Fr_ಹಿಂಭಾಗ();
            lsbp->unlink_Fr_ಹಿಂಭಾಗ();
            nodep->replaceWithKeepDType(ಹೊಸ AstSel{nodep->fileline(), lhsp, lsbp, ಅಗಲ});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        // Select straddles both sides, but is just a shorter extend
        ಆದರೆ (lsb == 0) {
            lhsp->unlink_Fr_ಹಿಂಭಾಗ();
            nodep->replaceWithKeepDType(ಹೊಸ AstExtend{nodep->fileline(), lhsp});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }

        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Sel_Bi_ಅಧೋ(AstSel* nodep) {
        // SEL(ADD(a,b),(width-1),0) -> ADD(SEL(a),SEL(b))
        // Add or any operation which doesn't care if we discard top bits
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ bip = VN_CAST(nodep->fromp(), ಗಂಟು_Biop);
        ಆದರೆ (!(m_doV && bip && VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        UINFOTREE(9, nodep, "", "SEL(BI)-in");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bilhsp = bip->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ birhsp = bip->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        bip->lhsp(ಹೊಸ AstSel{nodep->fileline(), bilhsp, 0, nodep->ಅಗಲ_Const()});
        bip->rhsp(ಹೊಸ AstSel{nodep->fileline(), birhsp, 0, nodep->ಅಗಲ_Const()});
        UINFOTREE(9, bip, "", "SEL(BI)-ou");
        VL_DO_DANGLING(replaceWChild(nodep, bip), nodep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Sel_ಸರಿಸು_ಅಧೋ(AstSel* nodep) {
        // AND({a}, SHIFTR({b}, {c})) is often shorthand in C for Verilog {b}[{c} :+ {a}]
        // becomes thought other optimizations
        // SEL(SHIFTR({a},{b}),{lsb},{width}) -> SEL({a},{lsb+b},{width})
        ಸ್ಥಿರಾಂಕ Ast_ಸರಿಸು_R* ಸ್ಥಿರಾಂಕ shiftp = VN_CAST(nodep->fromp(), ಸರಿಸು_R);
        ಆದರೆ (!(m_doV && shiftp && VN_IS(shiftp->rhsp(), Const) && VN_IS(nodep->lsbp(), Const))) {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ap = shiftp->lhsp();
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ bp = VN_AS(shiftp->rhsp(), Const);
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ lp = VN_AS(nodep->lsbp(), Const);
        ಆದರೆ (bp->isWide() || bp->ಅಂಕೆ().isFourState() || bp->ಅಂಕೆ().isNegative() || lp->isWide()
            || lp->ಅಂಕೆ().isFourState() || lp->ಅಂಕೆ().isNegative()) {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ newLsb = lp->ಗಮ್ಯ_SInt() + bp->ಗಮ್ಯ_SInt();
        ಆದರೆ (newLsb + nodep->ಅಗಲ_Const() > ap->ಅಗಲ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        //
        UINFO(9, "SEL(SHIFTR(a,b),l,w) -> SEL(a,l+b,w)");
        UINFOTREE(9, nodep, "", "SEL(SH)-in");
        AstSel* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ AstSel{nodep->fileline(), ap->unlink_Fr_ಹಿಂಭಾಗ(), newLsb, nodep->ಅಗಲ_Const()};
        nodep->replaceWithKeepDType(newp);
        UINFOTREE(9, newp, "", "SEL(SH)-ou");
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Bi_Extend_Const_Shrink(Ast_ಗಂಟು_Biop* nodep) {
        // Loop unrolling favors standalone compares
        // EQ(const{width32}, EXTEND(xx{width3})) -> EQ(const{3}, xx{3})
        // The constant must have zero bits (+ 1 if signed) or compare
        // would be incorrect. See also operandBiExtendConst
        AstExtend* ಸ್ಥಿರಾಂಕ extendp = VN_CAST(nodep->rhsp(), Extend);
        ಆದರೆ (!extendp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ smallerp = extendp->lhsp();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ subsize = smallerp->ಅಗಲ();
        AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->lhsp(), Const);
        ಆದರೆ (!constp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!constp->ಅಂಕೆ().is_ಬಿಟ್ಟುಗಳು_Zero(constp->ಅಗಲ() - 1, subsize)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        //
        UINFOTREE(9, nodep, "", "BI(EXTEND)-in");
        smallerp->unlink_Fr_ಹಿಂಭಾಗ();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(extendp->unlink_Fr_ಹಿಂಭಾಗ()), extendp);  // aka nodep->lhsp.
        nodep->rhsp(smallerp);

        constp->unlink_Fr_ಹಿಂಭಾಗ();
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ಅಂಕೆ{constp, subsize, constp->ಅಂಕೆ()};
        nodep->lhsp(ಹೊಸ AstConst{constp->fileline(), ಅಂಕೆ});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(constp), constp);
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // input node is still valid, keep going
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Bi_Extend_Const_Over(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* nodep) {
        // EQ(const{width32}, EXTEND(xx{width3})) -> constant
        // When the constant has non-zero bits above the extend it's a constant.
        // Avoids compiler warning
        ಸ್ಥಿರಾಂಕ AstExtend* ಸ್ಥಿರಾಂಕ extendp = VN_CAST(nodep->rhsp(), Extend);
        ಆದರೆ (!extendp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ smallerp = extendp->lhsp();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ subsize = smallerp->ಅಗಲ();
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->lhsp(), Const);
        ಆದರೆ (!constp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (constp->ಅಂಕೆ().is_ಬಿಟ್ಟುಗಳು_Zero(constp->ಅಗಲ() - 1, subsize)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Extraction checks
    ತಾರ್ಕಿಕ warnSelect(AstSel* nodep) {
        ಆದರೆ (m_doGenerate) {
            // Never checked yet
            V3_ಅಗಲ::ಅಗಲ_Params_Edit(nodep);
            iterateChildren(nodep);  // May need "constifying"
        }
        // Find range of dtype we are selecting from
        // Similar code in V3Unknown::AstSel
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ doit = ನಿಜ;
        ಆದರೆ (m_warn && VN_IS(nodep->lsbp(), Const) && doit) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ max_Decl_ಬಿಟ್ = nodep->declRange().hiMaxSelect() * nodep->decl_El_ಅಗಲ()
                                   + (nodep->decl_El_ಅಗಲ() - 1);
            ಆದರೆ (VN_AS(nodep->lsbp(), Const)->ಅಂಕೆ().isFourState()) {
                nodep->v3error("Selection index is constantly unknown or tristated: "
                               "lsb="
                               << nodep->lsbp()->name() << " width=" << nodep->ಅಗಲ_Const());
                // Replacing nodep will make a mess above, so we replace the offender
                replaceZero(nodep->lsbp());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->declRange().ranged()
                       && (nodep->msbConst() > max_Decl_ಬಿಟ್ || nodep->lsbConst() > max_Decl_ಬಿಟ್)) {
                // See also warning in V3Width
                // Must adjust by element width as declRange() is in number of elements
                string msbLsbProtected;
                ಆದರೆ (nodep->decl_El_ಅಗಲ() == 0) {
                    msbLsbProtected = "(nodep->declElWidth() == 0) "
                                      + std::ಗಮ್ಯ_string(nodep->msbConst()) + ":"
                                      + std::ಗಮ್ಯ_string(nodep->lsbConst());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    msbLsbProtected = std::ಗಮ್ಯ_string(nodep->msbConst() / nodep->decl_El_ಅಗಲ())
                                      + ":"
                                      + std::ಗಮ್ಯ_string(nodep->lsbConst() / nodep->decl_El_ಅಗಲ());
                }
                nodep->v3warn(SELRANGE,
                              "Selection index out of range: "
                                  << msbLsbProtected << " outside "
                                  << nodep->declRange().hiMaxSelect() << ":0"
                                  << (nodep->declRange().lo() >= 0
                                          ? ""
                                          : (" (adjusted +" + cvt_ಗಮ್ಯ_Str(-nodep->declRange().lo())
                                             + " to account for negative lsb)")));
                UINFO(1, "    Related Raw index is " << nodep->msbConst() << ":"
                                                     << nodep->lsbConst());
                // Don't replace with zero, we'll do it later
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Not a transform, so NOP
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ operandsSame(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* node1p, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* node2p) {
        // For now we just detect constants & simple vars, though it could be more generic
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ const1p = VN_CAST(node1p, Const)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ const2p = VN_CAST(node2p, Const)) {
                ಹಿಂತಿರುಗಿ V3_ಸಂಖ್ಯೆ{node1p->fileline(), 1, 0}
                    .opEq(const1p->ಅಂಕೆ(), const2p->ಅಂಕೆ())
                    .isEqOne();
            }
        }
        ಆದರೆ (VN_IS(node1p, VarRef) && VN_IS(node2p, VarRef)) {
            // Avoid comparing widthMin's, which results in lost optimization attempts
            // If cleanup sameGateTree to be smarter, this can be restored.
            // return node1p->sameGateTree(node2p);
            ಹಿಂತಿರುಗಿ node1p->isSame(node2p);
        }
        // Pattern created by coverage-line/branch; avoid compiler tautological-compare warning
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAnd* ಸ್ಥಿರಾಂಕ and1p = VN_CAST(node1p, And)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAnd* ಸ್ಥಿರಾಂಕ and2p = VN_CAST(node2p, And)) {
                ಆದರೆ (VN_IS(and1p->lhsp(), Const) && VN_IS(and1p->rhsp(), ಗಂಟು_Var_Ref)
                    && VN_IS(and2p->lhsp(), Const) && VN_IS(and2p->rhsp(), ಗಂಟು_Var_Ref))
                    ಹಿಂತಿರುಗಿ node1p->same_Gate_ವೃಕ್ಷ(node2p);
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ತಾರ್ಕಿಕ if_Same_ನಿಯೋಜಿಸು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_If* nodep) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ thensp = VN_CAST(nodep->thensp(), ಗಂಟು_ನಿಯೋಜಿಸು);
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ elsesp = VN_CAST(nodep->elsesp(), ಗಂಟು_ನಿಯೋಜಿಸು);
        ಆದರೆ (!thensp || thensp->nextp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Must be SINGLE statement
        ಆದರೆ (!elsesp || elsesp->nextp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (thensp->type() != elsesp->type()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Can't mix an assigndly with assign
        ಆದರೆ (!thensp->lhsp()->same_Gate_ವೃಕ್ಷ(elsesp->lhsp())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!thensp->rhsp()->gate_ವೃಕ್ಷ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!elsesp->rhsp()->gate_ವೃಕ್ಷ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Must not create an expression with unpacked array type
        ಆದರೆ (VN_IS(thensp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (VN_IS(elsesp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (m_underRecFunc) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // This optimization may lead to infinite recursion
        ಆದರೆ (m_doCpp) {  // Do not combine ifs cretaed by V3Premit
            ಆದರೆ (VN_IS(thensp->lhsp(), VarRef) && thensp->lhsp()->isWide()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        // Only do it if not calls and both pure, otherwise undoes V3LiftExpr
        ಹಿಂತಿರುಗಿ !VN_IS(thensp->rhsp(), ಗಂಟು_FTask_Ref)  //
               && !VN_IS(elsesp->rhsp(), ಗಂಟು_FTask_Ref)  //
               && thensp->rhsp()->is_ಶುದ್ಧ()  //
               && elsesp->rhsp()->is_ಶುದ್ಧ();
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_If_If(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_If* nodep) {
        ಆದರೆ (nodep->elsesp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_If* ಸ್ಥಿರಾಂಕ ಅಧೋ_Ifp = VN_CAST(nodep->thensp(), ಗಂಟು_If);
        ಆದರೆ (!ಅಧೋ_Ifp || ಅಧೋ_Ifp->nextp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (nodep->type() != ಅಧೋ_Ifp->type()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (Ast_ಗಂಟು::afterCommentp(ಅಧೋ_Ifp->elsesp())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ತಾರ್ಕಿಕ ifConcatMergeableBiop(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಹಿಂತಿರುಗಿ (VN_IS(nodep, And) || VN_IS(nodep, Or) || VN_IS(nodep, Xor));
    }
    ತಾರ್ಕಿಕ ifAdjacentSel(ಸ್ಥಿರಾಂಕ AstSel* lhsp, ಸ್ಥಿರಾಂಕ AstSel* rhsp) {
        ಆದರೆ (!v3Global.opt.fAssemble()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // opt disabled
        ಆದರೆ (!lhsp || !rhsp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ lfromp = lhsp->fromp();
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ rfromp = rhsp->fromp();
        ಆದರೆ (!lfromp || !rfromp || !lfromp->same_Gate_ವೃಕ್ಷ(rfromp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ lstart = VN_CAST(lhsp->lsbp(), Const);
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ rstart = VN_CAST(rhsp->lsbp(), Const);
        ಆದರೆ (!lstart || !rstart) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // too complicated
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ rend = (rstart->ಗಮ್ಯ_SInt() + rhsp->ಅಗಲ_Const());
        ಹಿಂತಿರುಗಿ (rend == lstart->ಗಮ್ಯ_SInt());
    }
    ತಾರ್ಕಿಕ if_ವಿಲೀನ_Adjacent(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        // Called by concatmergeable to determine if {lhsp, rhsp} make sense
        ಆದರೆ (!v3Global.opt.fAssemble()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // opt disabled
        // Two same varref
        ಆದರೆ (operandsSame(lhsp, rhsp)) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ lselp = VN_CAST(lhsp, Sel);
        ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ rselp = VN_CAST(rhsp, Sel);
        ಆದರೆ (!lselp && !rselp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (lselp && !VN_IS(lselp->lsbp(), Const)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (rselp && !VN_IS(rselp->lsbp(), Const)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // a[i:j] {a[j-1:k], b}
        ಆದರೆ (lselp && !rselp && VN_IS(rhsp, Concat))
            ಹಿಂತಿರುಗಿ if_ವಿಲೀನ_Adjacent(lhsp, VN_AS(rhsp, Concat)->lhsp());
        // {b, a[j:k]} a[k-1:i]
        ಆದರೆ (rselp && !lselp && VN_IS(lhsp, Concat))
            ಹಿಂತಿರುಗಿ if_ವಿಲೀನ_Adjacent(VN_AS(lhsp, Concat)->rhsp(), rhsp);
        // a a[msb:j]
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lfromp = lselp                                ? lselp->fromp()
                                          : lhsp->same_Gate_ವೃಕ್ಷ(rselp->fromp()) ? lhsp
                                                                               : ಶೂನ್ಯಸೂಚಕ;
        // a[i:0] a
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rfromp = rselp                                ? rselp->fromp()
                                          : rhsp->same_Gate_ವೃಕ್ಷ(lselp->fromp()) ? rhsp
                                                                               : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!lfromp || !rfromp || !lfromp->same_Gate_ವೃಕ್ಷ(rfromp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        ಸ್ಥಿರಾಂಕ int32_t lstart = lselp ? lselp->lsbConst() : 0;
        ಸ್ಥಿರಾಂಕ int32_t rstart = rselp ? rselp->lsbConst() : 0;
        ಸ್ಥಿರಾಂಕ int32_t rend = rstart + (rselp ? rselp->ಅಗಲ_Const() : rhsp->ಅಗಲ());
        // a[i:j] a[j-1:k]
        ಆದರೆ (rend == lstart) ಹಿಂತಿರುಗಿ ನಿಜ;
        // a[i:0] a[msb:j]
        ಆದರೆ (rend == rfromp->ಅಗಲ() && lstart == 0) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ತಾರ್ಕಿಕ concatMergeable(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ಚಿಹ್ನೆರಹಿತ ಆಳ) {
        // Determine if {a OP b, c OP d} => {a, c} OP {b, d} is advantageous
        ಆದರೆ (!v3Global.opt.fAssemble()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // opt disabled
        ಆದರೆ (lhsp->type() != rhsp->type()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!ifConcatMergeableBiop(lhsp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (ಆಳ > CONCAT_MERGABLE_MAX_ಆಳ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // As worse case O(n^2) algorithm

        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ lp = VN_CAST(lhsp, ಗಂಟು_Biop);
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ rp = VN_CAST(rhsp, ಗಂಟು_Biop);
        ಆದರೆ (!lp || !rp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // {a[]&b[], a[]&b[]}
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ lad = if_ವಿಲೀನ_Adjacent(lp->lhsp(), rp->lhsp());
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rad = if_ವಿಲೀನ_Adjacent(lp->rhsp(), rp->rhsp());
        ಆದರೆ (lad && rad) ಹಿಂತಿರುಗಿ ನಿಜ;
        // {a[] & b[]&c[], a[] & b[]&c[]}
        ಆದರೆ (lad && concatMergeable(lp->rhsp(), rp->rhsp(), ಆಳ + 1)) ಹಿಂತಿರುಗಿ ನಿಜ;
        // {a[]&b[] & c[], a[]&b[] & c[]}
        ಆದರೆ (rad && concatMergeable(lp->lhsp(), rp->lhsp(), ಆಳ + 1)) ಹಿಂತಿರುಗಿ ನಿಜ;
        // {(a[]&b[])&(c[]&d[]), (a[]&b[])&(c[]&d[])}
        ಆದರೆ (concatMergeable(lp->lhsp(), rp->lhsp(), ಆಳ + 1)
            && concatMergeable(lp->rhsp(), rp->rhsp(), ಆಳ + 1)) {
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ operands_Same_ಅಗಲ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* lhsp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* rhsp) {
        ಹಿಂತಿರುಗಿ lhsp->ಅಗಲ() == rhsp->ಅಗಲ();
    }

    //----------------------------------------
    // Constant Replacement functions.
    // These all take a node, delete its tree, and replaces it with a constant

    ಶೂನ್ಯ replace_ಅಂಕೆ(Ast_ಗಂಟು* oldp, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ, ಸ್ಥಿರಾಂಕ string& origParamName = "") {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        UASSERT_OBJ(!(VN_IS(oldp, Const) && !VN_AS(oldp, Const)->ಅಂಕೆ().isFourState()), oldp,
                    "Already constant??");
        AstConst* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{oldp->fileline(), ಅಂಕೆ};
        ಆದರೆ (!origParamName.ಖಾಲಿಗಳು()) newp->origParamName(origParamName);
        oldp->replaceWithKeepDType(newp);
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(oldp), oldp);
    }
    ಶೂನ್ಯ replace_ಅಂಕೆ(Ast_ಗಂಟು* nodep, uint32_t val) {
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ಅಂಕೆ{nodep, nodep->ಅಗಲ(), val};
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ), nodep);
    }
    ಶೂನ್ಯ replace_ಅಂಕೆ_Signed(Ast_ಗಂಟು_Biop* nodep, uint32_t val) {
        // We allow both sides to be constant, as one may have come from
        // parameter propagation, etc.
        ಆದರೆ (m_warn && !(VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const))) {
            nodep->v3warn(UNSIGNED, "Comparison is constant due to unsigned arithmetic");
        }
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, val), nodep);
    }
    ಶೂನ್ಯ replace_ಅಂಕೆ_Limited(Ast_ಗಂಟು_Biop* nodep, uint32_t val) {
        // Avoids gcc warning about same
        ಆದರೆ (m_warn) nodep->v3warn(CMPCONST, "Comparison is constant due to limited range");
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, val), nodep);
    }
    ಶೂನ್ಯ replaceZero(Ast_ಗಂಟು* nodep) { VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, 0), nodep); }
    ಶೂನ್ಯ replace_Zero_Chk_ಶುದ್ಧ(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* checkp) {
        // For example, "0 * n" -> 0 if n has no side effects
        // Else strength reduce it to 0 & n.
        // If ever change the operation note AstAnd rule specially ignores this created pattern
        ಆದರೆ (checkp->is_ಶುದ್ಧ()) {
            VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, 0), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstAnd{nodep->fileline(), ಹೊಸ AstConst{nodep->fileline(), 0},
                                             checkp->unlink_Fr_ಹಿಂಭಾಗ()};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ replaceAllOnes(Ast_ಗಂಟು* nodep) {
        V3_ಸಂಖ್ಯೆ ones{nodep, nodep->ಅಗಲ(), 0};
        ones.ಗಣ_ಮುಖವಾಡ(nodep->ಅಗಲ());
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ones), nodep);
    }
    ಶೂನ್ಯ replaceConst(Ast_ಗಂಟು_Uniop* nodep) {
        V3_ಸಂಖ್ಯೆ numv{nodep, nodep->ಅಗಲ_Min_V()};
        nodep->ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(numv, const_ಅಂಕೆ_V(nodep->lhsp()));
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = ಗಮ್ಯ_ಅಂಕೆ_C(nodep, numv);
        UINFO(4, "UNICONST -> " << ಅಂಕೆ);
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ), nodep);
    }
    ಶೂನ್ಯ replaceConst(Ast_ಗಂಟು_Biop* nodep) {
        V3_ಸಂಖ್ಯೆ numv{nodep, nodep->ಅಗಲ_Min_V()};
        nodep->ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(numv, const_ಅಂಕೆ_V(nodep->lhsp()), const_ಅಂಕೆ_V(nodep->rhsp()));
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = ಗಮ್ಯ_ಅಂಕೆ_C(nodep, numv);
        UINFO(4, "BICONST -> " << ಅಂಕೆ);
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ), nodep);
    }
    ಶೂನ್ಯ replaceConst(Ast_ಗಂಟು_Triop* nodep) {
        V3_ಸಂಖ್ಯೆ numv{nodep, nodep->ಅಗಲ_Min_V()};
        nodep->ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(numv, const_ಅಂಕೆ_V(nodep->lhsp()), const_ಅಂಕೆ_V(nodep->rhsp()),
                             const_ಅಂಕೆ_V(nodep->thsp()));
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = ಗಮ್ಯ_ಅಂಕೆ_C(nodep, numv);
        UINFO(4, "TRICONST -> " << ಅಂಕೆ);
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ), nodep);
    }
    ಶೂನ್ಯ replaceConst(Ast_ಗಂಟು_Quadop* nodep) {
        V3_ಸಂಖ್ಯೆ numv{nodep, nodep->ಅಗಲ_Min_V()};
        nodep->ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(numv, const_ಅಂಕೆ_V(nodep->lhsp()), const_ಅಂಕೆ_V(nodep->rhsp()),
                             const_ಅಂಕೆ_V(nodep->thsp()), const_ಅಂಕೆ_V(nodep->fhsp()));
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = ಗಮ್ಯ_ಅಂಕೆ_C(nodep, numv);
        UINFO(4, "QUADCONST -> " << ಅಂಕೆ);
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ), nodep);
    }

    ಶೂನ್ಯ replaceConstString(Ast_ಗಂಟು* oldp, ಸ್ಥಿರಾಂಕ string& ಅಂಕೆ) {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{oldp->fileline(), AstConst::String{}, ಅಂಕೆ};
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        oldp->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(oldp), oldp);
    }
    //----------------------------------------
    // Replacement functions.
    // These all take a node and replace it with something else

    ಶೂನ್ಯ replaceWChild(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* childp) {
        // NODE(..., CHILD(...)) -> CHILD(...)
        childp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
        // If replacing a SEL for example, the data type comes from the parent (is less wide).
        // This may adversely affect the operation of the node being replaced.
        nodep->replaceWithKeepDType(childp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ replaceWChildBool(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* childp) {
        // NODE(..., CHILD(...)) -> REDOR(CHILD(...))
        childp->unlink_Fr_ಹಿಂಭಾಗ();
        ಆದರೆ (childp->width1()) {
            nodep->replaceWithKeepDType(childp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->replaceWithKeepDType(ಹೊಸ AstRedOr{childp->fileline(), childp});
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    //! Replace a ternary node with its RHS after iterating
    //! Used with short-circuiting, where the RHS has not yet been iterated.
    ಶೂನ್ಯ replaceWIteratedRhs(Ast_ಗಂಟು_Triop* nodep) {
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()) iterate_And_ಮುಂದಿನ_Null(rhsp);
        replaceWChild(nodep, nodep->rhsp());  // May have changed
    }

    //! Replace a ternary node with its THS after iterating
    //! Used with short-circuiting, where the THS has not yet been iterated.
    ಶೂನ್ಯ replaceWIteratedThs(Ast_ಗಂಟು_Triop* nodep) {
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ thsp = nodep->thsp()) iterate_And_ಮುಂದಿನ_Null(thsp);
        replaceWChild(nodep, nodep->thsp());  // May have changed
    }
    ಶೂನ್ಯ replaceWLhs(Ast_ಗಂಟು_Uniop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    ಶೂನ್ಯ replaceWLhs(Ast_ಗಂಟು_Biop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    ಶೂನ್ಯ replaceWRhs(Ast_ಗಂಟು_Biop* nodep) {
        // Keep RHS, remove LHS
        replaceWChild(nodep, nodep->rhsp());
    }
    ಶೂನ್ಯ replaceWLhsBool(Ast_ಗಂಟು_Biop* nodep) { replaceWChildBool(nodep, nodep->lhsp()); }
    ಶೂನ್ಯ replaceWRhsBool(Ast_ಗಂಟು_Biop* nodep) { replaceWChildBool(nodep, nodep->rhsp()); }
    ಶೂನ್ಯ replaceAsv(Ast_ಗಂಟು_Biop* nodep) {
        // BIASV(CONSTa, BIASV(CONSTb, c)) -> BIASV( BIASV_CONSTED(a,b), c)
        // BIASV(SAMEa,  BIASV(SAMEb, c))  -> BIASV( BIASV(SAMEa,SAMEb), c)
        // UINFOTREE(1, nodep, "", "repAsvConst_old");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ap = nodep->lhsp();
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ rp = VN_AS(nodep->rhsp(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bp = rp->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cp = rp->rhsp();
        ap->unlink_Fr_ಹಿಂಭಾಗ();
        bp->unlink_Fr_ಹಿಂಭಾಗ();
        cp->unlink_Fr_ಹಿಂಭಾಗ();
        rp->unlink_Fr_ಹಿಂಭಾಗ();
        nodep->lhsp(rp);
        nodep->rhsp(cp);
        rp->lhsp(ap);
        rp->rhsp(bp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        ಆದರೆ (VN_IS(rp->lhsp(), Const) && VN_IS(rp->rhsp(), Const)) replaceConst(rp);
        iterate(nodep);  // Proceed to fixed point
    }
    ಶೂನ್ಯ replaceAsvLUp(Ast_ಗಂಟು_Biop* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r))
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ lp = VN_AS(nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ llp = lp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lrp = lp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        nodep->lhsp(llp);
        nodep->rhsp(lp);
        lp->lhsp(lrp);
        lp->rhsp(rp);
        lp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    ಶೂನ್ಯ replaceAsvRUp(Ast_ಗಂಟು_Biop* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr))
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ rp = VN_AS(nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rlp = rp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rrp = rp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        nodep->lhsp(rlp);
        nodep->rhsp(rp);
        rp->lhsp(lp);
        rp->rhsp(rrp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    ಶೂನ್ಯ replaceAndOr(Ast_ಗಂಟು_Biop* nodep) {
        //  OR  (AND (CONSTll,lr), AND(CONSTrl==ll,rr))    -> AND (CONSTll, OR(lr,rr))
        //  OR  (AND (CONSTll,lr), AND(CONSTrl,    rr=lr)) -> AND (OR(ll,rl), rr)
        // nodep ^lp  ^llp   ^lrp  ^rp  ^rlp       ^rrp
        // (Or/And may also be reversed)
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ lp = VN_AS(nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ llp = lp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lrp = lp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ rp = VN_AS(nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rlp = rp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rrp = rp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        nodep->replaceWithKeepDType(lp);
        ಆದರೆ (operandsSame(llp, rlp)) {
            lp->lhsp(llp);
            lp->rhsp(nodep);
            lp->dtypeFrom(nodep);
            nodep->lhsp(lrp);
            nodep->rhsp(rrp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(rp), rp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(rlp), rlp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (operandsSame(lrp, rrp)) {
            lp->lhsp(nodep);
            lp->rhsp(rrp);
            lp->dtypeFrom(nodep);
            nodep->lhsp(llp);
            nodep->rhsp(rlp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(rp), rp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(lrp), lrp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("replaceAndOr on something operandAndOrSame shouldn't have matched");
        }
        // UINFOTREE(1, nodep, "", "repAndOr_new");
    }
    ಶೂನ್ಯ replace_ಸರಿಸು_Same(Ast_ಗಂಟು_Biop* nodep) {
        // Or(Shift(ll,CONSTlr),Shift(rl,CONSTrr==lr)) -> Shift(Or(ll,rl),CONSTlr)
        // (Or/And may also be reversed)
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ lp = VN_AS(nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ llp = lp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lrp = lp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ rp = VN_AS(nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rlp = rp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rrp = rp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        nodep->replaceWithKeepDType(lp);
        lp->lhsp(nodep);
        lp->rhsp(lrp);
        nodep->lhsp(llp);
        nodep->rhsp(rlp);
        nodep->dtypep(llp->dtypep());  // dtype of Biop is before shift.
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(rp), rp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(rrp), rrp);
        // UINFOTREE(1, nodep, "", "repShiftSame_new");
    }
    ಶೂನ್ಯ replaceConcatSel(AstConcat* nodep) {
        // {a[1], a[0]} -> a[1:0]
        AstSel* ಸ್ಥಿರಾಂಕ lselp = VN_AS(nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), Sel);
        AstSel* ಸ್ಥಿರಾಂಕ rselp = VN_AS(nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ(), Sel);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lstart = lselp->lsbConst();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lwidth = lselp->ಅಗಲ_Const();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ rstart = rselp->lsbConst();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ rwidth = rselp->ಅಗಲ_Const();

        UASSERT_OBJ((rstart + rwidth) == lstart, nodep,
                    "tried to merge two selects which are not adjacent");
        AstSel* ಸ್ಥಿರಾಂಕ newselp = ಹೊಸ AstSel{
            lselp->fromp()->fileline(), rselp->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), rstart, lwidth + rwidth};
        UINFO(5, "merged two adjacent sel " << lselp << " and " << rselp << " to one " << newselp);

        nodep->replaceWithKeepDType(newselp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(lselp), lselp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(rselp), rselp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ replace_Concat_ವಿಲೀನ(AstConcat* nodep) {
        // {llp OP lrp, rlp OP rrp} => {llp, rlp} OP {lrp, rrp}, where OP = AND/OR/XOR
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ lp = VN_AS(nodep->lhsp(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ rp = VN_AS(nodep->rhsp(), ಗಂಟು_Biop);
        ಆದರೆ (concatMergeable(lp, rp, 0)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ llp = lp->lhsp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lrp = lp->rhsp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rlp = rp->lhsp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rrp = rp->rhsp();
            AstConcat* ಸ್ಥಿರಾಂಕ newlp = ಹೊಸ AstConcat{rlp->fileline(), llp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                                   rlp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
            AstConcat* ಸ್ಥಿರಾಂಕ newrp = ಹೊಸ AstConcat{rrp->fileline(), lrp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                                   rrp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
            // use the lhs to replace the parent concat
            llp->replaceWith(newlp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(llp), llp);
            lrp->replaceWith(newrp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(lrp), lrp);
            lp->dtype_Chg_ಅಗಲ_Signed(newlp->ಅಗಲ(), newlp->ಅಗಲ(), VSigning::UNSIGNED);
            UINFO(5, "merged " << nodep);
            ++m_stat_Concat_ವಿಲೀನ;
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(rp->unlink_Fr_ಹಿಂಭಾಗ()), rp);
            nodep->replaceWithKeepDType(lp->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            iterate(lp->lhsp());
            iterate(lp->rhsp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("tried to merge two Concat which are not adjacent");
        }
    }
    ಶೂನ್ಯ replaceExtend(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* arg0p) {
        // -> EXTEND(nodep)
        // like a AstExtend{$rhsp}, but we need to set the width correctly from base node
        arg0p->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp
            = (VN_IS(nodep, ExtendS)
                   ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstExtendS{nodep->fileline(), arg0p})
                   : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstExtend{nodep->fileline(), arg0p}));
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ replace_Pow_ಸರಿಸು(Ast_ಗಂಟು_Biop* nodep) {  // Pow or PowS
        UINFO(5, "POW(2,b)->SHIFTL(1,b) " << nodep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಸರಿಸು_L* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ Ast_ಸರಿಸು_L{nodep->fileline(), ಹೊಸ AstConst{nodep->fileline(), 1}, rhsp};
        newp->lhsp()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ replace_Mul_ಸರಿಸು(AstMul* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "MUL(2^n,b)->SHIFTL(b,n) " << nodep);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ amount = VN_AS(nodep->lhsp(), Const)->ಅಂಕೆ().most_ಗಣ_ಬಿಟ್_P1() - 1;  // 2^n->n+1
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ opp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಸರಿಸು_L* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ Ast_ಸರಿಸು_L{nodep->fileline(), opp, ಹೊಸ AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ replace_Div_ಸರಿಸು(AstDiv* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "DIV(b,2^n)->SHIFTR(b,n) " << nodep);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ amount = VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().most_ಗಣ_ಬಿಟ್_P1() - 1;  // 2^n->n+1
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ opp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಸರಿಸು_R* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ Ast_ಸರಿಸು_R{nodep->fileline(), opp, ಹೊಸ AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ replaceModAnd(AstModDiv* nodep) {  // Mod, but not ModS as not simple shift
        UINFO(5, "MOD(b,2^n)->AND(b,2^n-1) " << nodep);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ amount = VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().most_ಗಣ_ಬಿಟ್_P1() - 1;  // 2^n->n+1
        V3_ಸಂಖ್ಯೆ ಮುಖವಾಡ{nodep, nodep->ಅಗಲ()};
        ಮುಖವಾಡ.ಗಣ_ಮುಖವಾಡ(amount);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ opp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        AstAnd* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ AstAnd{nodep->fileline(), opp, ಹೊಸ AstConst{nodep->fileline(), ಮುಖವಾಡ}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ replace_ಸರಿಸು_Op(Ast_ಗಂಟು_Biop* nodep) {
        UINFO(5, "SHIFT(AND(a,b),CONST) with a/b special ->AND(SHIFT(a,CONST),SHIFT(b,CONST)) "
                     << nodep);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = nodep->ಅಗಲ();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ_Min = nodep->ಅಗಲ_Min();
        VNRelinker handle;
        nodep->unlink_Fr_ಹಿಂಭಾಗ(&handle);
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ lhsp = VN_AS(nodep->lhsp(), ಗಂಟು_Biop);
        lhsp->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ shiftp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ap = lhsp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bp = lhsp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ shift1p = nodep;
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ shift2p = nodep->clone_ವೃಕ್ಷ(ನಿಜ);
        shift1p->lhsp(ap);
        shift1p->rhsp(shiftp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ));
        shift2p->lhsp(bp);
        shift2p->rhsp(shiftp);
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ newp = lhsp;
        newp->lhsp(shift1p);
        newp->rhsp(shift2p);
        newp->dtype_Chg_ಅಗಲ(ಅಗಲ, ಅಗಲ_Min);  // The new AND must have width of the original SHIFT
        handle.relink(newp);
        iterate(newp);  // Further reduce, either node may have more reductions.
    }
    ಶೂನ್ಯ replace_ಸರಿಸು_ಸರಿಸು(Ast_ಗಂಟು_Biop* nodep) {
        UINFO(4, "SHIFT(SHIFT(a,s1),s2)->SHIFT(a,ADD(s1,s2)) " << nodep);
        UINFOTREE(9, nodep, "", "repShiftShift_old");
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ lhsp = VN_AS(nodep->lhsp(), ಗಂಟು_Biop);
        lhsp->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ap = lhsp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ shift1p = lhsp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ shift2p = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        // Shift1p and shift2p may have different sizes, both are
        // self-determined so sum with infinite width
        ಆದರೆ (nodep->type() == lhsp->type()) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ shift1 = VN_AS(shift1p, Const)->ಗಮ್ಯ_UInt();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ shift2 = VN_AS(shift2p, Const)->ಗಮ್ಯ_UInt();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ newshift = shift1 + shift2;
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(shift2p), shift2p);
            nodep->lhsp(ap);
            nodep->rhsp(ಹೊಸ AstConst(nodep->fileline(), newshift));
            iterate(nodep);  // Further reduce, either node may have more reductions.
        } ಇಲ್ಲದಿದ್ದರೆ {
            // We know shift amounts are constant, but might be a mixed left/right shift
            ಪೂರ್ಣಾಂಕ shift1 = VN_AS(shift1p, Const)->ಗಮ್ಯ_UInt();
            ಆದರೆ (VN_IS(lhsp, ಸರಿಸು_R)) shift1 = -shift1;
            ಪೂರ್ಣಾಂಕ shift2 = VN_AS(shift2p, Const)->ಗಮ್ಯ_UInt();
            ಆದರೆ (VN_IS(nodep, ಸರಿಸು_R)) shift2 = -shift2;
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ newshift = shift1 + shift2;
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(shift2p), shift2p);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
            V3_ಸಂಖ್ಯೆ mask1{nodep, nodep->ಅಗಲ()};
            V3_ಸಂಖ್ಯೆ ones{nodep, nodep->ಅಗಲ()};
            ones.ಗಣ_ಮುಖವಾಡ(nodep->ಅಗಲ());
            ಆದರೆ (shift1 < 0) {
                mask1.op_ಸರಿಸು_R(ones, V3_ಸಂಖ್ಯೆ(nodep, VL_IDATASIZE, -shift1));
            } ಇಲ್ಲದಿದ್ದರೆ {
                mask1.op_ಸರಿಸು_L(ones, V3_ಸಂಖ್ಯೆ(nodep, VL_IDATASIZE, shift1));
            }
            V3_ಸಂಖ್ಯೆ ಮುಖವಾಡ{nodep, nodep->ಅಗಲ()};
            ಆದರೆ (shift2 < 0) {
                ಮುಖವಾಡ.op_ಸರಿಸು_R(mask1, V3_ಸಂಖ್ಯೆ(nodep, VL_IDATASIZE, -shift2));
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಮುಖವಾಡ.op_ಸರಿಸು_L(mask1, V3_ಸಂಖ್ಯೆ(nodep, VL_IDATASIZE, shift2));
            }
            ಆದರೆ (newshift < 0) {
                newp = ಹೊಸ Ast_ಸರಿಸು_R{nodep->fileline(), ap,
                                     ಹೊಸ AstConst(nodep->fileline(), -newshift)};
            } ಇಲ್ಲದಿದ್ದರೆ {
                newp = ಹೊಸ Ast_ಸರಿಸು_L{nodep->fileline(), ap,
                                     ಹೊಸ AstConst(nodep->fileline(), newshift)};
            }
            newp->dtypeFrom(nodep);
            newp = ಹೊಸ AstAnd{nodep->fileline(), newp, ಹೊಸ AstConst{nodep->fileline(), ಮುಖವಾಡ}};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            // UINFOTREE(1, newp, "", "repShiftShift_new");
            iterate(newp);  // Further reduce, either node may have more reductions.
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(lhsp), lhsp);
    }

    ತಾರ್ಕಿಕ replace_ನಿಯೋಜಿಸು_Multi_Sel(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) {
        // Multiple assignments to sequential bits can be concated
        // ASSIGN(SEL(a),aq), ASSIGN(SEL(a+1),bq) -> ASSIGN(SEL(a:b),CONCAT(aq,bq)
        // ie. assign var[2]=a, assign var[3]=b -> assign var[3:2]={b,a}

        // Skip if we're not const'ing an entire module (IE doing only one assign, etc)
        ಆದರೆ (!m_modp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ಸ್ಥಿರಾಂಕ sel1p = VN_CAST(nodep->lhsp(), Sel);
        ಆದರೆ (!sel1p) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ nextp = VN_CAST(nodep->nextp(), ಗಂಟು_ನಿಯೋಜಿಸು);
        ಆದರೆ (!nextp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (nodep->type() != nextp->type()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ sel2p = VN_CAST(nextp->lhsp(), Sel);
        ಆದರೆ (!sel2p) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstVarRef* ಸ್ಥಿರಾಂಕ varref1p = VN_CAST(sel1p->fromp(), VarRef);
        ಆದರೆ (!varref1p) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varref2p = VN_CAST(sel2p->fromp(), VarRef);
        ಆದರೆ (!varref2p) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!varref1p->same_Gate_ವೃಕ್ಷ(varref2p)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ con1p = VN_CAST(sel1p->lsbp(), Const);
        ಆದರೆ (!con1p) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ con2p = VN_CAST(sel2p->lsbp(), Const);
        ಆದರೆ (!con2p) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // We need to make sure there's no self-references involved in either
        // assignment.  For speed, we only look 3 deep, then give up.
        ಆದರೆ (!varNotReferenced(nodep->rhsp(), varref1p->varp())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!varNotReferenced(nextp->rhsp(), varref2p->varp())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // If a variable is marked split_var, access to the variable should not be merged.
        ಆದರೆ (varref1p->varp()->attr_ವಿಭಜಿಸು_Var() || varref2p->varp()->attr_ವಿಭಜಿಸು_Var()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Swap?
        ಆದರೆ ((con1p->ಗಮ್ಯ_SInt() != con2p->ಗಮ್ಯ_SInt() + sel2p->ಅಗಲ())
            && (con2p->ಗಮ್ಯ_SInt() != con1p->ಗಮ್ಯ_SInt() + sel1p->ಅಗಲ())) {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ lsb_ಮೊದಲ_ನಿಯೋಜಿಸು = (con1p->ಗಮ್ಯ_UInt() < con2p->ಗಮ್ಯ_UInt());
        UINFO(4, "replaceAssignMultiSel " << nodep);
        UINFO(4, "                   && " << nextp);
        // UINFOTREE(1, nodep, "", "comb1");
        // UINFOTREE(1, nextp, "", "comb2");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhs1p = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhs2p = nextp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* newp;
        ಆದರೆ (lsb_ಮೊದಲ_ನಿಯೋಜಿಸು) {
            newp = nodep->cloneType(ಹೊಸ AstSel{sel1p->fileline(), varref1p->unlink_Fr_ಹಿಂಭಾಗ(),
                                               sel1p->lsbConst(), sel1p->ಅಗಲ() + sel2p->ಅಗಲ()},
                                    ಹೊಸ AstConcat{rhs1p->fileline(), rhs2p, rhs1p});
        } ಇಲ್ಲದಿದ್ದರೆ {
            newp = nodep->cloneType(ಹೊಸ AstSel{sel1p->fileline(), varref1p->unlink_Fr_ಹಿಂಭಾಗ(),
                                               sel2p->lsbConst(), sel1p->ಅಗಲ() + sel2p->ಅಗಲ()},
                                    ಹೊಸ AstConcat{rhs1p->fileline(), rhs1p, rhs2p});
        }
        // UINFOTREE(1, pnewp, "", "conew");
        nodep->replaceWith(newp);  // dypep intentionally changing
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nextp->unlink_Fr_ಹಿಂಭಾಗ()), nextp);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ತಾರ್ಕಿಕ replace_ನಿಯೋಜಿಸು_SFormat_F(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) {
        // Rewrite 'x = sformatf(...)' into 'sformat(x, ...)', which is more efficient
        // at the call site as it does not need to check if 'x' needs to be freed first.
        // This is a somewhat common pattern after lowering assertions.
        ಆದರೆ (!VN_IS(nodep, ನಿಯೋಜಿಸು)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(nodep->lhsp(), VarRef);
        ಆದರೆ (!lhsp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstSFormatF* ಸ್ಥಿರಾಂಕ fmtp = VN_CAST(nodep->rhsp(), SFormatF);
        ಆದರೆ (!fmtp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        lhsp->unlink_Fr_ಹಿಂಭಾಗ();
        fmtp->unlink_Fr_ಹಿಂಭಾಗ();
        nodep->replaceWith(ಹೊಸ AstSFormat{nodep->fileline(), fmtp, lhsp});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ತಾರ್ಕಿಕ varNotReferenced(Ast_ಗಂಟು* nodep, AstVar* varp, ಪೂರ್ಣಾಂಕ level = 0) {
        // Return true if varp never referenced under node.
        // Return false if referenced, or tree too deep to be worth it, or side effects
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (level > 2) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!nodep->is_ಶುದ್ಧ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // For example a $fgetc can't be reordered
        ಆದರೆ (VN_IS(nodep, ಗಂಟು_Var_Ref) && VN_AS(nodep, ಗಂಟು_Var_Ref)->varp() == varp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ (varNotReferenced(nodep->nextp(), varp, level + 1)
                && varNotReferenced(nodep->op1p(), varp, level + 1)
                && varNotReferenced(nodep->op2p(), varp, level + 1)
                && varNotReferenced(nodep->op3p(), varp, level + 1)
                && varNotReferenced(nodep->op4p(), varp, level + 1));
    }

    ತಾರ್ಕಿಕ replace_ಗಂಟು_ನಿಯೋಜಿಸು(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) {
        ಆದರೆ (VN_IS(nodep->lhsp(), VarRef) && VN_IS(nodep->rhsp(), VarRef)
            && VN_AS(nodep->lhsp(), VarRef)->sameNoLvalue(VN_AS(nodep->rhsp(), VarRef))
            && !VN_IS(nodep, ನಿಯೋಜಿಸು_Dly)) {
            // X = X.  Quite pointless, though X <= X may override another earlier assignment
            ಆದರೆ (VN_IS(nodep, ನಿಯೋಜಿಸು_W)) {
                ಆದರೆ (!v3Global.opt.pedantic())
                    nodep->v3error(
                        "Wire inputs its own output, creating circular logic (wire x=x)");
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Don't delete the assign, or V3Gate will freak out
            } ಇಲ್ಲದಿದ್ದರೆ {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_doV && VN_IS(nodep->lhsp(), Concat)) {
            ತಾರ್ಕಿಕ need_ತಾತ್ಕಾಲಿಕ = ಸುಳ್ಳು;
            // A blocking assignment with an intra-assignment timing control evaluates its value,
            // then waits once (IEEE 1800-2023 9.4.5), here to assign a temporary for the parts
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ waits = VN_IS(nodep, ನಿಯೋಜಿಸು) && nodep->timingControlp();
            // Splitting copies the RHS once per part, so a large RHS would make the tree
            // grow with the product of the number of parts and the RHS size
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ need_ತಾತ್ಕಾಲಿಕ_ಶುದ್ಧ = !nodep->rhsp()->is_ಶುದ್ಧ() || waits
                                      || nodep->rhsp()->isLargerThan(CONCAT_ನಿಯೋಜಿಸು_MAX_RHS_NODES);
            ಆದರೆ (m_warn && !VN_IS(nodep, ನಿಯೋಜಿಸು_Dly)
                && !need_ತಾತ್ಕಾಲಿಕ_ಶುದ್ಧ) {  // Is same var on LHS and RHS?
                // If the rhs is not pure, we need a temporary variable anyway
                std::unordered_ಗಣ<ಗಾತ್ರ_t> lhsVarIds;
                nodep->lhsp()->foreach([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    lhsVarIds.emplace(m_ids(refp->varp()));
                });
                nodep->rhsp()->foreach([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    ಆದರೆ (lhsVarIds.ಎಣಿಕೆ_2(m_ids(refp->varp()))) need_ತಾತ್ಕಾಲಿಕ = ನಿಜ;
                });
            }
            ಆದರೆ (need_ತಾತ್ಕಾಲಿಕ_ಶುದ್ಧ) {
                // if the RHS is impure or large we need to create a temporary variable for it,
                // because further handling involves copying of the RHS.
                UINFO(4, "  ASSITEMPPURE " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp,rhs),
                //                                ASSIGN(lc1,SEL(temp,{size1})),
                //                                ASSIGN(lc2,SEL(temp,{size2}))

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                AstVar* ಸ್ಥಿರಾಂಕ ತಾತ್ಕಾಲಿಕ_Purep = ಹೊಸ AstVar{rhsp->fileline(), VVarType::BLOCKTEMP,
                                                     m_concswap_ಹೆಸರುಗಳು.get(rhsp), rhsp->dtypep()};
                // Static, as read right after written, also in a fork
                ತಾತ್ಕಾಲಿಕ_Purep->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ಕೂಡಿಸು_Stmtsp(ತಾತ್ಕಾಲಿಕ_Purep);
                AstVarRef* ಸ್ಥಿರಾಂಕ ತಾತ್ಕಾಲಿಕ_ಶುದ್ಧ_Refp
                    = ಹೊಸ AstVarRef{rhsp->fileline(), ತಾತ್ಕಾಲಿಕ_Purep, VAccess::WRITE};
                Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ asnp
                    = VN_IS(nodep, ನಿಯೋಜಿಸು_Dly)
                          ? ಹೊಸ Ast_ನಿಯೋಜಿಸು{nodep->fileline(), ತಾತ್ಕಾಲಿಕ_ಶುದ್ಧ_Refp, rhsp}
                          : nodep->cloneType(ತಾತ್ಕಾಲಿಕ_ಶುದ್ಧ_Refp, rhsp);
                nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(asnp);
                nodep->rhsp(ಹೊಸ AstVarRef{rhsp->fileline(), ತಾತ್ಕಾಲಿಕ_Purep, VAccess::READ});
                // Only the assignment of the temporary waits
                ಆದರೆ (waits) ತಳ್ಳು_Deletep(nodep->timingControlp()->unlink_Fr_ಹಿಂಭಾಗ());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (need_ತಾತ್ಕಾಲಿಕ) {
                // The first time we constify, there may be the same variable on the LHS
                // and RHS.  In that case, we must use temporaries, or {a,b}={b,a} will break.
                UINFO(4, "  ASSITEMP " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp1,SEL(rhs,{size})),
                //                                ASSIGN(temp2,SEL(newrhs,{size}))
                //                                ASSIGN(lc1,temp1),
                //                                ASSIGN(lc2,temp2)
            } ಇಲ್ಲದಿದ್ದರೆ {
                UINFO(4, "  ASSI " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(lc1,SEL(rhs,{size})),
                //                                ASSIGN(lc2,SEL(newrhs,{size}))
            }
            // The NBA evaluates its intra-assignment delay as it executes (IEEE 1800-2023 4.9.4),
            // so once for the NBAs to all parts
            AstDelay* ಸ್ಥಿರಾಂಕ delayp
                = VN_IS(nodep, ನಿಯೋಜಿಸು_Dly) ? VN_CAST(nodep->timingControlp(), Delay) : ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (delayp && !delayp->lhsp()->is_ಶುದ್ಧ()) {
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = delayp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                AstVar* ಸ್ಥಿರಾಂಕ tempp = ಹೊಸ AstVar{flp, VVarType::BLOCKTEMP,
                                                 m_concswap_ಹೆಸರುಗಳು.get(valuep), valuep->dtypep()};
                // Static, as read right after written, when the NBAs to the parts start waiting
                tempp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ಕೂಡಿಸು_Stmtsp(tempp);
                nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(
                    ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, tempp, VAccess::WRITE}, valuep});
                delayp->lhsp(ಹೊಸ AstVarRef{flp, tempp, VAccess::READ});
            }
            UINFOTREE(9, nodep, "", "Ass_old");
            // Unlink the stuff
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lc1p = VN_AS(nodep->lhsp(), Concat)->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lc2p = VN_AS(nodep->lhsp(), Concat)->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ conp = VN_AS(nodep->lhsp(), Concat)->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhs2p = rhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            // Calc widths
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb2 = 0;
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ msb2 = lsb2 + lc2p->ಅಗಲ() - 1;
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb1 = msb2 + 1;
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ msb1 = lsb1 + lc1p->ಅಗಲ() - 1;
            UASSERT_OBJ(msb1 == (conp->ಅಗಲ() - 1), nodep, "Width calc mismatch");
            // Form ranges
            AstSel* ಸ್ಥಿರಾಂಕ sel1p = ಹೊಸ AstSel{conp->fileline(), rhsp, lsb1, msb1 - lsb1 + 1};
            AstSel* ಸ್ಥಿರಾಂಕ sel2p = ಹೊಸ AstSel{conp->fileline(), rhs2p, lsb2, msb2 - lsb2 + 1};
            // Make new assigns of same flavor as old one
            //*** Not cloneTree; just one node.
            Ast_ಗಂಟು_ನಿಯೋಜಿಸು* newp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (!need_ತಾತ್ಕಾಲಿಕ) {
                Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ asn1ap = nodep->cloneType(lc1p, sel1p);
                Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ asn2ap = nodep->cloneType(lc2p, sel2p);
                asn1ap->dtypeFrom(sel1p);
                asn2ap->dtypeFrom(sel2p);
                newp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(newp, asn1ap);
                newp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(newp, asn2ap);
            } ಇಲ್ಲದಿದ್ದರೆ {
                UASSERT_OBJ(m_modp, nodep, "Not under module");
                UASSERT_OBJ(m_globalPass, nodep,
                            "Should not reach here when not invoked on whole AstNetlist");
                // We could create just one temp variable, but we'll get better optimization
                // if we make one per term.
                AstVar* ಸ್ಥಿರಾಂಕ temp1p
                    = ಹೊಸ AstVar{sel1p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswap_ಹೆಸರುಗಳು.get(sel1p), VFlagLogicPacked{}, msb1 - lsb1 + 1};
                AstVar* ಸ್ಥಿರಾಂಕ temp2p
                    = ಹೊಸ AstVar{sel2p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswap_ಹೆಸರುಗಳು.get(sel2p), VFlagLogicPacked{}, msb2 - lsb2 + 1};
                m_modp->ಕೂಡಿಸು_Stmtsp(temp1p);
                m_modp->ಕೂಡಿಸು_Stmtsp(temp2p);
                Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ asn1ap = nodep->cloneType(
                    ಹೊಸ AstVarRef{sel1p->fileline(), temp1p, VAccess::WRITE}, sel1p);
                Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ asn2ap = nodep->cloneType(
                    ಹೊಸ AstVarRef{sel2p->fileline(), temp2p, VAccess::WRITE}, sel2p);
                Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ asn1bp = nodep->cloneType(
                    lc1p, ಹೊಸ AstVarRef{sel1p->fileline(), temp1p, VAccess::READ});
                Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ asn2bp = nodep->cloneType(
                    lc2p, ಹೊಸ AstVarRef{sel2p->fileline(), temp2p, VAccess::READ});
                asn1ap->dtypeFrom(temp1p);
                asn1bp->dtypeFrom(temp1p);
                asn2ap->dtypeFrom(temp2p);
                asn2bp->dtypeFrom(temp2p);
                // This order matters
                newp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(newp, asn1ap);
                newp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(newp, asn2ap);
                newp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(newp, asn1bp);
                newp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(newp, asn2bp);
            }
            ಆದರೆ (debug() >= 9 && newp) newp->dump_ವೃಕ್ಷ_And_ಮುಂದಿನ(ಹೊರಗೆ, "-     _new: ");
            nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newp);
            // Cleanup
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(conp), conp);
            // Further reduce, either node may have more reductions.
            ಹಿಂತಿರುಗಿ ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_doV && VN_IS(nodep->rhsp(), Cvt_Packed_ಗಮ್ಯ_Array)
                   && ಅಧೋ_As_Fixed_Aggregate(nodep->lhsp()->dtypep())) {
            Ast_Cvt_Packed_ಗಮ್ಯ_Array* ಸ್ಥಿರಾಂಕ cvtp
                = VN_AS(nodep->rhsp(), Cvt_Packed_ಗಮ್ಯ_Array)->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = cvtp->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
            ಆದರೆ (ಅಧೋ_As_Fixed_Aggregate(srcp->dtypep())) {
                srcp = packFixedAggregate(srcp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_ಹರಿವು* ಸ್ಥಿರಾಂಕ streamp = VN_CAST(srcp, ಗಂಟು_ಹರಿವು)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಹರಿವು_Srcp = streamp->lhsp();
                ಆದರೆ (ಅಧೋ_As_Fixed_Aggregate(ಹರಿವು_Srcp->dtypep())) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ packedp = packFixedAggregate(ಹರಿವು_Srcp->unlink_Fr_ಹಿಂಭಾಗ());
                    streamp->lhsp(packedp);
                    streamp->dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(packedp->ಅಗಲ(), packedp->ಅಗಲ_Min(),
                                                       VSigning::UNSIGNED,
                                                       packedp->isFourstateOrNoDtype());
                }
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(cvtp), cvtp);
            replace_ನಿಯೋಜಿಸು_ಗಮ್ಯ_Fixed_Aggregate(nodep, nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), srcp);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_doV && VN_IS(nodep->rhsp(), ಹರಿವು_R)
                   && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), ಸರತಿ_DType)) {
            // The right-streaming operator on rhs of assignment does not
            // change the order of bits. Eliminate stream but keep its lhsp.
            // Add a cast if needed.
            Ast_ಹರಿವು_R* ಸ್ಥಿರಾಂಕ streamp = VN_AS(nodep->rhsp(), ಹರಿವು_R)->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = streamp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ srcDTypep = srcp->dtypep()->skipRefp();
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            ಆದರೆ (ಅಧೋ_As_Fixed_Aggregate(srcDTypep)) {
                srcp = packFixedAggregate(srcp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(srcDTypep, ಸರತಿ_DType) || VN_IS(srcDTypep, DynArrayDType)) {
                ಆದರೆ (VN_IS(dstDTypep, ಸರತಿ_DType) || VN_IS(dstDTypep, DynArrayDType)) {
                    ಪೂರ್ಣಾಂಕ src_Element_ಬಿಟ್ಟುಗಳು = 0;
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = srcDTypep->subDTypep()) {
                        src_Element_ಬಿಟ್ಟುಗಳು = elemDtp->ಅಗಲ();
                    }
                    ಪೂರ್ಣಾಂಕ dst_Element_ಬಿಟ್ಟುಗಳು = 0;
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_ಬಿಟ್ಟುಗಳು = elemDtp->ಅಗಲ();
                    }
                    srcp = ಹೊಸ Ast_Cvt_Array_ಗಮ್ಯ_Array{
                        srcp->fileline(), srcp,          nodep->dtypep(), ಸುಳ್ಳು, 1,
                        dst_Element_ಬಿಟ್ಟುಗಳು,   src_Element_ಬಿಟ್ಟುಗಳು};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    srcp = ಹೊಸ Ast_Cvt_Array_ಗಮ್ಯ_Packed{srcp->fileline(), srcp, nodep->dtypep()};
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(srcDTypep, UnpackArrayDType)) {
                srcp = ಹೊಸ Ast_Cvt_Array_ಗಮ್ಯ_Packed{srcp->fileline(), srcp, srcDTypep};
                // Handling the case where lhs is wider than rhs by inserting zeros. StreamL does
                // not require this, since the left streaming operator implicitly handles this.
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ packed_ಬಿಟ್ಟುಗಳು = nodep->lhsp()->ಅಗಲ_Min();
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ unpack_ಬಿಟ್ಟುಗಳು
                    = srcDTypep->arrayUnpackedElements() * srcDTypep->subDTypep()->ಅಗಲ_Min();
                ಸ್ಥಿರಾಂಕ uint32_t ಸ್ಥಾನಾಂತರ = packed_ಬಿಟ್ಟುಗಳು > unpack_ಬಿಟ್ಟುಗಳು ? packed_ಬಿಟ್ಟುಗಳು - unpack_ಬಿಟ್ಟುಗಳು : 0;
                srcp = ಹೊಸ Ast_ಸರಿಸು_L{srcp->fileline(), srcp,
                                     ಹೊಸ AstConst{srcp->fileline(), ಸ್ಥಾನಾಂತರ}, packed_ಬಿಟ್ಟುಗಳು};
            }
            ಆದರೆ (!VN_IS(dstDTypep, UnpackArrayDType) && !VN_IS(dstDTypep, ಸರತಿ_DType)
                && !VN_IS(dstDTypep, DynArrayDType)) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ s_ಅಗಲ = srcp->ಅಗಲ();
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ d_ಅಗಲ = nodep->lhsp()->ಅಗಲ();
                ಆದರೆ (s_ಅಗಲ < d_ಅಗಲ) {
                    AstExtend* ಸ್ಥಿರಾಂಕ extendp = ಹೊಸ AstExtend{srcp->fileline(), srcp, d_ಅಗಲ};
                    srcp = ಹೊಸ Ast_ಸರಿಸು_L{
                        srcp->fileline(), extendp,
                        ಹೊಸ AstConst{srcp->fileline(), static_cast<uint32_t>(d_ಅಗಲ - s_ಅಗಲ)},
                        d_ಅಗಲ};
                }
            }
            nodep->rhsp(srcp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            ಹಿಂತಿರುಗಿ ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_doV && VN_IS(nodep->lhsp(), ಹರಿವು_L)) {
            // Push the stream operator to the rhs of the assignment statement
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* streamp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ dstp = VN_AS(streamp, ಹರಿವು_L)->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dstDTypep = dstp->dtypep()->skipRefp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ srcDTypep = srcp->dtypep()->skipRefp();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamL reverses, so reverse=true)
            ಆದರೆ ((VN_IS(srcDTypep, UnpackArrayDType) || VN_IS(srcDTypep, ಸರತಿ_DType)
                 || VN_IS(srcDTypep, DynArrayDType))
                && (VN_IS(dstDTypep, ಸರತಿ_DType) || VN_IS(dstDTypep, DynArrayDType))) {
                ಪೂರ್ಣಾಂಕ ಖಂಡ_ಗಾತ್ರ = 1;
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp
                    = VN_CAST(VN_AS(streamp, ಹರಿವು_L)->rhsp(), Const)) {
                    ಖಂಡ_ಗಾತ್ರ = constp->ಗಮ್ಯ_SInt();
                    ಆದರೆ (VL_UNLIKELY(ಖಂಡ_ಗಾತ್ರ <= 0)) ಖಂಡ_ಗಾತ್ರ = 1;
                }
                ಪೂರ್ಣಾಂಕ src_Element_ಬಿಟ್ಟುಗಳು = 0;
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = srcDTypep->subDTypep()) {
                    src_Element_ಬಿಟ್ಟುಗಳು = elemDtp->ಅಗಲ();
                }
                ಪೂರ್ಣಾಂಕ dst_Element_ಬಿಟ್ಟುಗಳು = 0;
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = dstDTypep->subDTypep()) {
                    dst_Element_ಬಿಟ್ಟುಗಳು = elemDtp->ಅಗಲ();
                }
                nodep->lhsp(dstp);
                nodep->rhsp(ಹೊಸ Ast_Cvt_Array_ಗಮ್ಯ_Array{srcp->fileline(), srcp, dstDTypep, ನಿಜ,
                                                   ಖಂಡ_ಗಾತ್ರ, dst_Element_ಬಿಟ್ಟುಗಳು, src_Element_ಬಿಟ್ಟುಗಳು});
                nodep->dtypep(dstDTypep);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(streamp), streamp);
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
            ಆದರೆ (ಅಧೋ_As_Fixed_Aggregate(srcDTypep)) srcp = packFixedAggregate(srcp);
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ s_ಅಗಲ = srcp->ಅಗಲ();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ d_ಅಗಲ = dstp->ಅಗಲ();
            // Connect the rhs to the stream operator and update its width
            VN_AS(streamp, ಹರಿವು_L)->lhsp(srcp);
            ಆದರೆ (VN_IS(srcDTypep, DynArrayDType) || VN_IS(srcDTypep, ಸರತಿ_DType)
                || VN_IS(srcDTypep, UnpackArrayDType)) {
                streamp->dtype_ಗಣ_ಹರಿವು();
            } ಇಲ್ಲದಿದ್ದರೆ {
                streamp->dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(srcp->ಅಗಲ(), srcp->ಅಗಲ_Min(),
                                                   VSigning::UNSIGNED,
                                                   srcp->isFourstateOrNoDtype());
            }
            ಆದರೆ (VN_IS(dstDTypep, UnpackArrayDType)) {
                streamp = ಹೊಸ Ast_Cvt_Packed_ಗಮ್ಯ_Array{nodep->fileline(), streamp, dstDTypep};
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (d_ಅಗಲ == 0) {
                    streamp = ಹೊಸ Ast_Cvt_Packed_ಗಮ್ಯ_Array{nodep->fileline(), streamp, dstDTypep};
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (s_ಅಗಲ >= d_ಅಗಲ) {
                    streamp = ಹೊಸ AstSel{streamp->fileline(), streamp, s_ಅಗಲ - d_ಅಗಲ, d_ಅಗಲ};
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(streamp);
            nodep->dtypep(dstDTypep);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_doV && VN_IS(nodep->lhsp(), ಹರಿವು_R)) {
            // The right stream operator on lhs of assignment statement does
            // not reorder bits. However, if the rhs is wider than the lhs,
            // then we select bits from the left-most, not the right-most.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ streamp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ dstp = VN_AS(streamp, ಹರಿವು_R)->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dstDTypep = dstp->dtypep()->skipRefp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamR does not reverse, so reverse=false).
            // V3Width may have wrapped the source in CvtArrayToPacked; unwrap it.
            ಆದರೆ (VN_IS(dstDTypep, ಸರತಿ_DType) || VN_IS(dstDTypep, DynArrayDType)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* origSrcp = srcp;
                ಆದರೆ (Ast_Cvt_Array_ಗಮ್ಯ_Packed* ಸ್ಥಿರಾಂಕ cvtp = VN_CAST(srcp, Cvt_Array_ಗಮ್ಯ_Packed)) {
                    origSrcp = cvtp->fromp();
                }
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ origSrcDTypep = origSrcp->dtypep()->skipRefp();
                ಆದರೆ (VN_IS(origSrcDTypep, UnpackArrayDType) || VN_IS(origSrcDTypep, ಸರತಿ_DType)
                    || VN_IS(origSrcDTypep, DynArrayDType)) {
                    ಪೂರ್ಣಾಂಕ src_Element_ಬಿಟ್ಟುಗಳು = 0;
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = origSrcDTypep->subDTypep()) {
                        src_Element_ಬಿಟ್ಟುಗಳು = elemDtp->ಅಗಲ();
                    }
                    ಪೂರ್ಣಾಂಕ dst_Element_ಬಿಟ್ಟುಗಳು = 0;
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_ಬಿಟ್ಟುಗಳು = elemDtp->ಅಗಲ();
                    }
                    ಆದರೆ (VN_IS(srcp, Cvt_Array_ಗಮ್ಯ_Packed)) {
                        origSrcp = VN_AS(srcp, Cvt_Array_ಗಮ್ಯ_Packed)->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(srcp), srcp);
                        srcp = origSrcp;
                    }
                    // Descending unpacked arrays need element reversal
                    ತಾರ್ಕಿಕ ಹಿಮ್ಮುಖ = ಸುಳ್ಳು;
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ unpackDtp
                        = VN_CAST(origSrcDTypep, UnpackArrayDType)) {
                        ಹಿಮ್ಮುಖ = !unpackDtp->declRange().ascending();
                    }
                    nodep->lhsp(dstp);
                    nodep->rhsp(ಹೊಸ Ast_Cvt_Array_ಗಮ್ಯ_Array{srcp->fileline(), srcp, dstDTypep, ಹಿಮ್ಮುಖ,
                                                       1, dst_Element_ಬಿಟ್ಟುಗಳು, src_Element_ಬಿಟ್ಟುಗಳು});
                    nodep->dtypep(dstDTypep);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(streamp), streamp);
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
            }
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ s_ಅಗಲ = srcp->ಅಗಲ();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ d_ಅಗಲ = dstp->ಅಗಲ();
            ಆದರೆ (VN_IS(dstDTypep, UnpackArrayDType)) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ dst_ಬಿಟ್_ಅಗಲ
                    = d_ಅಗಲ * VN_AS(dstDTypep, UnpackArrayDType)->arrayUnpackedElements();
                // Handling the case where rhs is wider than lhs. StreamL does not require this
                // since the combination of the left streaming operation and the implicit
                // truncation in VL_ASSIGN_UNPACK automatically selects the left-most bits.
                ಆದರೆ (s_ಅಗಲ > dst_ಬಿಟ್_ಅಗಲ) {
                    srcp
                        = ಹೊಸ AstSel{streamp->fileline(), srcp, s_ಅಗಲ - dst_ಬಿಟ್_ಅಗಲ, dst_ಬಿಟ್_ಅಗಲ};
                }
                srcp = ಹೊಸ Ast_Cvt_Packed_ಗಮ್ಯ_Array{nodep->fileline(), srcp, dstDTypep};
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (d_ಅಗಲ == 0) {
                    srcp = ಹೊಸ Ast_Cvt_Packed_ಗಮ್ಯ_Array{nodep->fileline(), srcp, dstDTypep};
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (s_ಅಗಲ >= d_ಅಗಲ) {
                    srcp = ಹೊಸ AstSel{streamp->fileline(), srcp, s_ಅಗಲ - d_ಅಗಲ, d_ಅಗಲ};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Source narrower than destination: left-justify by shifting left.
                    // The right stream operator packs left-to-right, so remaining
                    // LSBs are zero-filled (IEEE 1800-2023 11.4.14.3).
                    ಆದರೆ (!VN_IS(srcp->dtypep()->skipRefp(), ಸರತಿ_DType)) {
                        AstExtend* ಸ್ಥಿರಾಂಕ extendp = ಹೊಸ AstExtend{srcp->fileline(), srcp, d_ಅಗಲ};
                        srcp = ಹೊಸ Ast_ಸರಿಸು_L{
                            srcp->fileline(), extendp,
                            ಹೊಸ AstConst{srcp->fileline(), static_cast<uint32_t>(d_ಅಗಲ - s_ಅಗಲ)},
                            d_ಅಗಲ};
                    } ಇಲ್ಲದಿದ್ದರೆ {  // if it is a queue we dont need do do a shiftL
                        srcp = ಹೊಸ AstExtend{srcp->fileline(), srcp};
                    }
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(srcp);
            nodep->dtypep(dstDTypep);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            ಹಿಂತಿರುಗಿ ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_doV && VN_IS(nodep->rhsp(), ಹರಿವು_L)) {
            Ast_ಹರಿವು_L* streamp = VN_AS(nodep->rhsp(), ಹರಿವು_L);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = streamp->lhsp();
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ srcDTypep = srcp->dtypep()->skipRefp();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            ಆದರೆ (ಅಧೋ_As_Fixed_Aggregate(srcDTypep)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ packedp = packFixedAggregate(srcp->unlink_Fr_ಹಿಂಭಾಗ());
                streamp->lhsp(packedp);
                streamp->dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(packedp->ಅಗಲ(), packedp->ಅಗಲ_Min(),
                                                   VSigning::UNSIGNED,
                                                   packedp->isFourstateOrNoDtype());
                srcp = packedp;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((VN_IS(srcDTypep, ಸರತಿ_DType) || VN_IS(srcDTypep, DynArrayDType)
                        || VN_IS(srcDTypep, UnpackArrayDType))) {
                ಆದರೆ (VN_IS(dstDTypep, ಸರತಿ_DType) || VN_IS(dstDTypep, DynArrayDType)) {
                    ಪೂರ್ಣಾಂಕ ಖಂಡ_ಗಾತ್ರ = 1;
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(streamp->rhsp(), Const)) {
                        ಖಂಡ_ಗಾತ್ರ = constp->ಗಮ್ಯ_SInt();
                        ಆದರೆ (VL_UNLIKELY(ಖಂಡ_ಗಾತ್ರ <= 0)) {
                            // Not reachable due to higher level checks when parsing stream
                            // operators commented out to not fail v3error-coverage-checks.
                            // nodep->v3error("Stream block size must be positive, got " <<
                            // blockSize); nodep->v3error("Stream block size must be positive, got
                            // " << blockSize);
                            ಖಂಡ_ಗಾತ್ರ = 1;
                        }
                    }
                    // Not reachable due to higher level checks when parsing stream operators
                    // commented out to not fail v3error-coverage-checks.
                    // else {
                    // nodep->v3error("Stream block size must be constant (got " <<
                    // streamp->rhsp()->prettyTypeName() << ")");
                    // }
                    ಪೂರ್ಣಾಂಕ src_Element_ಬಿಟ್ಟುಗಳು = 0;
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = srcDTypep->subDTypep()) {
                        src_Element_ಬಿಟ್ಟುಗಳು = elemDtp->ಅಗಲ();
                    }
                    ಪೂರ್ಣಾಂಕ dst_Element_ಬಿಟ್ಟುಗಳು = 0;
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_ಬಿಟ್ಟುಗಳು = elemDtp->ಅಗಲ();
                    }
                    streamp->unlink_Fr_ಹಿಂಭಾಗ();
                    srcp = ಹೊಸ Ast_Cvt_Array_ಗಮ್ಯ_Array{
                        srcp->fileline(), srcp->unlink_Fr_ಹಿಂಭಾಗ(), dstDTypep,     ನಿಜ,
                        ಖಂಡ_ಗಾತ್ರ,        dst_Element_ಬಿಟ್ಟುಗಳು,       src_Element_ಬಿಟ್ಟುಗಳು};
                    nodep->rhsp(srcp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(streamp), streamp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    streamp->lhsp(ಹೊಸ Ast_Cvt_Array_ಗಮ್ಯ_Packed{srcp->fileline(), srcp->unlink_Fr_ಹಿಂಭಾಗ(),
                                                          dstDTypep});
                    streamp->dtypeFrom(dstDTypep);
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_doV && replace_ನಿಯೋಜಿಸು_Multi_Sel(nodep)) {
            ಹಿಂತಿರುಗಿ ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (replace_ನಿಯೋಜಿಸು_SFormat_F(nodep)) {
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Boolean replacements
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Bool_ಸರಿಸು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        // boolean test of AND(const,SHIFTR(x,const)) -> test of AND(SHIFTL(x,const), x)
        ಆದರೆ (!VN_IS(nodep, And)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!VN_IS(VN_AS(nodep, And)->lhsp(), Const)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!VN_IS(VN_AS(nodep, And)->rhsp(), ಸರಿಸು_R)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_ಸರಿಸು_R* ಸ್ಥಿರಾಂಕ shiftp = VN_AS(VN_AS(nodep, And)->rhsp(), ಸರಿಸು_R);
        ಆದರೆ (!VN_IS(shiftp->rhsp(), Const)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (static_cast<uint32_t>(nodep->ಅಗಲ()) <= VN_AS(shiftp->rhsp(), Const)->ಗಮ್ಯ_UInt()) {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಶೂನ್ಯ replace_Bool_ಸರಿಸು(Ast_ಗಂಟು* nodep) {
        UINFOTREE(9, nodep, "", "bshft_old");
        AstConst* ಸ್ಥಿರಾಂಕ andConstp = VN_AS(VN_AS(nodep, And)->lhsp(), Const);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp
            = VN_AS(VN_AS(nodep, And)->rhsp(), ಸರಿಸು_R)->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        AstConst* ಸ್ಥಿರಾಂಕ ಸರಿಸು_Constp
            = VN_AS(VN_AS(VN_AS(nodep, And)->rhsp(), ಸರಿಸು_R)->rhsp(), Const);
        V3_ಸಂಖ್ಯೆ val{andConstp, andConstp->ಅಗಲ()};
        val.op_ಸರಿಸು_L(andConstp->ಅಂಕೆ(), ಸರಿಸು_Constp->ಅಂಕೆ());
        AstAnd* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ AstAnd{nodep->fileline(), ಹೊಸ AstConst{nodep->fileline(), val}, fromp};
        // widthMin no longer applicable if different C-expanded width
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        UINFOTREE(9, newp, "", "_new");
    }

    ಶೂನ್ಯ replaceWithSimulation(Ast_ಗಂಟು* nodep) {
        SimulateVisitor simvis;
        // Run it - may be unoptimizable due to large for loop, etc
        simvis.mainParamEmulate(nodep);
        ಆದರೆ (!simvis.optimizable()) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* errorp = simvis.whyNotNodep();
            ಆದರೆ (!errorp) errorp = nodep;
            nodep->v3error("Expecting expression to be constant, but can't determine constant for "
                           << nodep->prettyTypeName() << '\n'
                           << errorp->warnOther() << "... Location of non-constant "
                           << errorp->prettyTypeName() << ": " << simvis.why_Not_ಸಂದೇಶ());
            VL_DO_DANGLING(replaceZero(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Fetch the result
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ valuep = simvis.fetch_ಮೌಲ್ಯ_Null(nodep);  // valuep is owned by Simulate
            UASSERT_OBJ(valuep, nodep, "No value returned from simulation");
            // Replace it
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = valuep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            newp->fileline(nodep->fileline());
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Simulate->" << newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    //----------------------------------------

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        iterateChildrenBackwardsConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_concswap_ಹೆಸರುಗಳು.reset();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        // No ASSIGNW removals under funcs, we've long eliminated INITIALs
        // (We should perhaps rename the assignw's to just assigns)
        VL_RESTORER(m_wremove);
        m_wremove = ಸುಳ್ಳು;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCLocalScope* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (!nodep->stmtsp()) {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        // No ASSIGNW removals under scope, we've long eliminated INITIALs
        VL_RESTORER(m_wremove);
        VL_RESTORER(m_scopep);
        m_wremove = ಸುಳ್ಳು;
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಅದಲುಬದಲು_Sides(Ast_ಗಂಟು_Bi_Com* nodep) {
        // COMMUTATIVE({a},CONST) -> COMMUTATIVE(CONST,{a})
        // This simplifies later optimizations
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
        nodep->lhsp(rhsp);
        nodep->rhsp(lhsp);
        iterate(nodep);  // Again?
    }

    ತಾರ್ಕಿಕ ಒಳಗೊಂಡಿದೆ_Member_Access_Recurse(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t id = m_ids(nodep);
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_ಒಳಗೊಂಡಿದೆ_Member_Access.ಕಂಡುಹಿಡಿ(id);
        ಆದರೆ (it != m_ಒಳಗೊಂಡಿದೆ_Member_Access.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
        ++m_statMemberAccessVisits;
        ತಾರ್ಕಿಕ ಫಲಿತಾಂಶ = ಸುಳ್ಳು;
        ಆದರೆ (VN_IS(nodep, MemberSel) || VN_IS(nodep, ವಿಧಾನ_Call) || VN_IS(nodep, CMethodCall)) {
            ಫಲಿತಾಂಶ = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ funcRefp = VN_CAST(nodep, ಗಂಟು_FTask_Ref)) {
            ಆದರೆ (ಒಳಗೊಂಡಿದೆ_Member_Access_Recurse(funcRefp->taskp())) ಫಲಿತಾಂಶ = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_CCall* ಸ್ಥಿರಾಂಕ funcRefp = VN_CAST(nodep, ಗಂಟು_CCall)) {
            ಆದರೆ (ಒಳಗೊಂಡಿದೆ_Member_Access_Recurse(funcRefp->funcp())) ಫಲಿತಾಂಶ = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ funcp = VN_CAST(nodep, ಗಂಟು_FTask)) {
            // Assume that it has a member access
            ಆದರೆ (funcp->recursive()) ಫಲಿತಾಂಶ = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ funcp = VN_CAST(nodep, CFunc)) {
            ಆದರೆ (funcp->recursive()) ಫಲಿತಾಂಶ = ನಿಜ;
        }
        ಆದರೆ (!ಫಲಿತಾಂಶ) {
            ಫಲಿತಾಂಶ = ಒಳಗೊಂಡಿದೆ_Member_Access_Recurse(nodep->op1p())
                     || ಒಳಗೊಂಡಿದೆ_Member_Access_Recurse(nodep->op2p())
                     || ಒಳಗೊಂಡಿದೆ_Member_Access_Recurse(nodep->op3p())
                     || ಒಳಗೊಂಡಿದೆ_Member_Access_Recurse(nodep->op4p());
        }
        ಆದರೆ (!ಫಲಿತಾಂಶ && !VN_IS(nodep, ಗಂಟು_FTask)
            && !VN_IS(nodep, CFunc)  // don't enter into next function
            && ಒಳಗೊಂಡಿದೆ_Member_Access_Recurse(nodep->nextp())) {
            ಫಲಿತಾಂಶ = ನಿಜ;
        }
        m_ಒಳಗೊಂಡಿದೆ_Member_Access.emplace(id, ಫಲಿತಾಂಶ);
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

    ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ_Biop_ಗಮ್ಯ_Bitwise(Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ nodep) {
        ಆದರೆ (!m_convert_Logic_ಗಮ್ಯ_ಬಿಟ್) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!nodep->lhsp()->width1()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!nodep->rhsp()->width1()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!nodep->is_ಶುದ್ಧ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (ಒಳಗೊಂಡಿದೆ_Member_Access_Recurse(nodep)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ_Concat_Rand(AstConcat* nodep) {
        //    CONCAT(RAND, RAND) - created by Chisel code
        AstRand* ಸ್ಥಿರಾಂಕ aRandp = VN_CAST(nodep->lhsp(), Rand);
        ಸ್ಥಿರಾಂಕ AstRand* ಸ್ಥಿರಾಂಕ bRandp = VN_CAST(nodep->rhsp(), Rand);
        ಆದರೆ (!aRandp || !bRandp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!aRandp->combinable(bRandp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        UINFO(4, "Concat(Rand,Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlink_Fr_ಹಿಂಭಾಗ());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ_Sel_Rand(AstSel* nodep) {
        //    SEL(RAND) - created by Chisel code
        AstRand* ಸ್ಥಿರಾಂಕ aRandp = VN_CAST(nodep->fromp(), Rand);
        ಆದರೆ (!aRandp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (aRandp->seedp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        UINFO(4, "Sel(Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlink_Fr_ಹಿಂಭಾಗ());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ_ಗಮ್ಯ_String_NConst(Ast_ಗಮ್ಯ_String_N* nodep) {
        iterateChildren(nodep);
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConsArray* ಸ್ಥಿರಾಂಕ consp = VN_CAST(nodep->lhsp(), ConsArray)) {
            ಆದರೆ (!(m_doExpensive || m_params)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (!consp->is_Constant_ವೃಕ್ಷ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!VN_IS(nodep->lhsp(), Const)) {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        replaceWithSimulation(nodep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಪೂರ್ಣಾಂಕ ಕಾರ್ಯಾಂಶ_Concat_Move(ಸ್ಥಿರಾಂಕ AstConcat* nodep) {
        //    CONCAT under concat  (See moveConcat)
        // Return value: true indicates to do it; 2 means move to LHS
        ಸ್ಥಿರಾಂಕ AstConcat* ಸ್ಥಿರಾಂಕ abConcp = VN_CAST(nodep->lhsp(), Concat);
        ಸ್ಥಿರಾಂಕ AstConcat* ಸ್ಥಿರಾಂಕ bcConcp = VN_CAST(nodep->rhsp(), Concat);
        ಆದರೆ (!abConcp && !bcConcp) ಹಿಂತಿರುಗಿ 0;
        ಆದರೆ (bcConcp) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ap = nodep->lhsp();
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bp = bcConcp->lhsp();
            // If a+b == 32,64,96 etc, then we want to have a+b together on LHS
            ಆದರೆ (VL_BITBIT_I(ap->ಅಗಲ() + bp->ಅಗಲ()) == 0) ಹಿಂತಿರುಗಿ 2;  // Transform 2: to abConc
        } ಇಲ್ಲದಿದ್ದರೆ {  // abConcp
            // Unless lhs is already 32 bits due to above, reorder it
            ಆದರೆ (VL_BITBIT_I(nodep->lhsp()->ಅಗಲ()) != 0) ಹಿಂತಿರುಗಿ 1;  // Transform 1: to bcConc
        }
        ಹಿಂತಿರುಗಿ 0;  // ok
    }
    ಶೂನ್ಯ moveConcat(AstConcat* nodep) {
        //    1: CONCAT(CONCAT({a},{b}),{c})  -> CONCAT({a},CONCAT({b}, {c}))
        // or 2: CONCAT({a}, CONCAT({b},{c})) -> CONCAT(CONCAT({a},{b}),{c})
        // Because the lhs of a concat needs a shift but the rhs doesn't,
        // putting additional CONCATs on the RHS leads to fewer assembler operations.
        // However, we'll end up with lots of wide moves if we make huge trees
        // like that, so on 32 bit boundaries, we'll do the opposite form.
        UINFO(4, "Move concat: " << nodep);
        ಆದರೆ (ಕಾರ್ಯಾಂಶ_Concat_Move(nodep) > 1) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ap = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            AstConcat* ಸ್ಥಿರಾಂಕ bcConcp = VN_AS(nodep->rhsp(), Concat);
            bcConcp->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bp = bcConcp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cp = bcConcp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            AstConcat* ಸ್ಥಿರಾಂಕ abConcp = ಹೊಸ AstConcat{bcConcp->fileline(), ap, bp};
            nodep->lhsp(abConcp);
            nodep->rhsp(cp);
            // If bp was a concat, then we have this exact same form again!
            // Recurse rather then calling node->iterate to prevent 2^n recursion!
            ಆದರೆ (ಕಾರ್ಯಾಂಶ_Concat_Move(abConcp)) moveConcat(abConcp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(bcConcp), bcConcp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            AstConcat* ಸ್ಥಿರಾಂಕ abConcp = VN_AS(nodep->lhsp(), Concat);
            abConcp->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ap = abConcp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bp = abConcp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            AstConcat* ಸ್ಥಿರಾಂಕ bcConcp = ಹೊಸ AstConcat{abConcp->fileline(), bp, cp};
            nodep->lhsp(ap);
            nodep->rhsp(bcConcp);
            ಆದರೆ (ಕಾರ್ಯಾಂಶ_Concat_Move(bcConcp)) moveConcat(bcConcp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(abConcp), abConcp);
        }
    }

    // Special cases
    ಶೂನ್ಯ ಭೇಟಿ(AstConst*) override {}  // Already constant

    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        ಆದರೆ (m_params) {
            iterate_And_ಮುಂದಿನ_Null(nodep->paramsp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassOrPackageRef* nodep) override { iterateChildren(nodep); }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಹೊಂದಿಕೆ_Masked* nodep) override {
        // Do not iterate the tables, they must be constant pool entries
        iterate(nodep->lhsp());
        ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->lhsp(), Const)) {
            replace_ಅಂಕೆ(nodep, Ast_ಹೊಂದಿಕೆ_Masked::fold(constp->ಅಂಕೆ(), nodep->matchp()->varp()));
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstPin* nodep) override { iterateChildren(nodep); }

    ಶೂನ್ಯ replaceLogEq(AstLogEq* nodep) {
        // LOGEQ(a,b) => AstLogAnd{AstLogOr{AstLogNot{a},b},AstLogOr{AstLogNot{b},a}}
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        // Do exactly as IEEE says, might result in extra terms, so in future may do differently
        AstLogAnd* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstLogAnd{
            nodep->fileline(),
            ಹೊಸ AstLogOr{nodep->fileline(), ಹೊಸ AstLogNot{nodep->fileline(), lhsp}, rhsp},
            ಹೊಸ AstLogOr{nodep->fileline(),
                         ಹೊಸ AstLogNot{nodep->fileline(), rhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)},
                         lhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ replaceSelSel(AstSel* nodep) {
        // SEL(SEL({x},a,b),c,d) => SEL({x},a+c,d)
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ಸ್ಥಿರಾಂಕ belowp = VN_AS(nodep->fromp(), Sel);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = belowp->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lsb1p = nodep->lsbp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lsb2p = belowp->lsbp()->unlink_Fr_ಹಿಂಭಾಗ();
        // Eliminate lower range
        UINFO(4, "Elim Lower range: " << nodep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newlsbp;
        ಆದರೆ (VN_IS(lsb1p, Const) && VN_IS(lsb2p, Const)) {
            newlsbp = ಹೊಸ AstConst{lsb1p->fileline(),
                                   VN_AS(lsb1p, Const)->ಗಮ್ಯ_UInt() + VN_AS(lsb2p, Const)->ಗಮ್ಯ_UInt()};
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(lsb1p), lsb1p);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(lsb2p), lsb2p);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Width is important, we need the width of the fromp's expression, not the
            // potentially smaller lsb1p's width, but don't insert a redundant AstExtend.
            // Note that due to some sloppiness in earlier passes, lsb1p might actually be wider,
            // so extend to the wider type.
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ widep = lsb1p->ಅಗಲ() > lsb2p->ಅಗಲ() ? lsb1p : lsb2p;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = widep->ಅಗಲ() > lsb2p->ಅಗಲ()
                                          ? ಹೊಸ AstExtend{lsb2p->fileline(), lsb2p}
                                          : lsb2p;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = widep->ಅಗಲ() > lsb1p->ಅಗಲ()
                                          ? ಹೊಸ AstExtend{lsb1p->fileline(), lsb1p}
                                          : lsb1p;
            lhsp->dtypeFrom(widep);
            rhsp->dtypeFrom(widep);
            newlsbp = ಹೊಸ Ast_ಕೂಡಿಸು{lsb1p->fileline(), lhsp, rhsp};
            newlsbp->dtypeFrom(widep);
        }
        AstSel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstSel{nodep->fileline(), fromp, newlsbp, nodep->ಅಗಲ_Const()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ replaceSelConcat(AstSel* nodep) {
        // SEL(CONCAT(a,b),c,d) => SEL(a or b, . .)
        AstConcat* ಸ್ಥಿರಾಂಕ conp = VN_AS(nodep->fromp(), Concat);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ conLhsp = conp->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ conRhsp = conp->rhsp();
        ಆದರೆ (static_cast<ಪೂರ್ಣಾಂಕ>(nodep->lsbConst()) >= conRhsp->ಅಗಲ()) {
            conLhsp->unlink_Fr_ಹಿಂಭಾಗ();
            AstSel* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstSel{nodep->fileline(), conLhsp, nodep->lsbConst() - conRhsp->ಅಗಲ(),
                             nodep->ಅಗಲ_Const()};
            nodep->replaceWithKeepDType(newp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (static_cast<ಪೂರ್ಣಾಂಕ>(nodep->msbConst()) < conRhsp->ಅಗಲ()) {
            conRhsp->unlink_Fr_ಹಿಂಭಾಗ();
            AstSel* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(), nodep->ಅಗಲ_Const()};
            nodep->replaceWithKeepDType(newp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Yuk, split between the two
            conRhsp->unlink_Fr_ಹಿಂಭಾಗ();
            conLhsp->unlink_Fr_ಹಿಂಭಾಗ();
            AstConcat* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstConcat{nodep->fileline(),
                                ಹೊಸ AstSel{nodep->fileline(), conLhsp, 0,
                                           nodep->msbConst() - conRhsp->ಅಗಲ() + 1},
                                ಹೊಸ AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(),
                                           conRhsp->ಅಗಲ() - nodep->lsbConst()}};
            nodep->replaceWithKeepDType(newp);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Sel_Replicate(AstSel* nodep) {
        // SEL(REPLICATE(from,rep),lsb,width) => SEL(from,0,width) as long
        // as SEL's width <= b's width
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ಸ್ಥಿರಾಂಕ repp = VN_AS(nodep->fromp(), Replicate);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = repp->srcp();
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ lsbp = VN_CAST(nodep->lsbp(), Const);
        ಆದರೆ (!lsbp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        UASSERT_OBJ(fromp->ಅಗಲ(), nodep, "Not widthed");
        ಆದರೆ ((lsbp->ಗಮ್ಯ_UInt() / fromp->ಅಗಲ())
            != ((lsbp->ಗಮ್ಯ_UInt() + nodep->ಅಗಲ() - 1) / fromp->ಅಗಲ())) {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        //
        fromp->unlink_Fr_ಹಿಂಭಾಗ();
        AstSel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstSel{
            nodep->fileline(), fromp,
            ಹೊಸ AstConst{lsbp->fileline(), lsbp->ಗಮ್ಯ_UInt() % fromp->ಅಗಲ()}, nodep->ಅಗಲ_Const()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Rep_Rep(AstReplicate* nodep) {
        // REPLICATE(REPLICATE2(from2,cnt2),cnt1) => REPLICATE(from2,(cnt1+cnt2))
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ಸ್ಥಿರಾಂಕ rep2p = VN_AS(nodep->srcp(), Replicate);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ from2p = rep2p->srcp();
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ cnt1p = VN_CAST(nodep->countp(), Const);
        ಆದರೆ (!cnt1p) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ cnt2p = VN_CAST(rep2p->countp(), Const);
        ಆದರೆ (!cnt2p) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        //
        from2p->unlink_Fr_ಹಿಂಭಾಗ();
        AstReplicate* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ AstReplicate{nodep->fileline(), from2p, cnt1p->ಗಮ್ಯ_UInt() * cnt2p->ಗಮ್ಯ_UInt()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ತಾರ್ಕಿಕ ಕಾರ್ಯಾಂಶ_Concat_Same(AstConcat* nodep) {
        // CONCAT(fromp,fromp) -> REPLICATE(fromp,1+1)
        // CONCAT(REP(fromp,cnt1),fromp) -> REPLICATE(fromp,cnt1+1)
        // CONCAT(fromp,REP(fromp,cnt1)) -> REPLICATE(fromp,1+cnt1)
        // CONCAT(REP(fromp,cnt1),REP(fromp,cnt2)) -> REPLICATE(fromp,cnt1+cnt2)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* from1p = nodep->lhsp();
        uint32_t cnt1 = 1;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* from2p = nodep->rhsp();
        uint32_t cnt2 = 1;
        ಆದರೆ (VN_IS(from1p, Replicate)) {
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ cnt1p = VN_CAST(VN_CAST(from1p, Replicate)->countp(), Const);
            ಆದರೆ (!cnt1p) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            from1p = VN_AS(from1p, Replicate)->srcp();
            cnt1 = cnt1p->ಗಮ್ಯ_UInt();
        }
        ಆದರೆ (VN_IS(from2p, Replicate)) {
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ cnt2p = VN_CAST(VN_CAST(from2p, Replicate)->countp(), Const);
            ಆದರೆ (!cnt2p) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            from2p = VN_AS(from2p, Replicate)->srcp();
            cnt2 = cnt2p->ಗಮ್ಯ_UInt();
        }
        ಆದರೆ (!operandsSame(from1p, from2p)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        //
        from1p->unlink_Fr_ಹಿಂಭಾಗ();
        AstReplicate* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstReplicate{nodep->fileline(), from1p, cnt1 + cnt2};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಶೂನ್ಯ replaceSelIntoBiop(AstSel* nodep) {
        // SEL(BUFIF1(a,b),1,bit) => BUFIF1(SEL(a,1,bit),SEL(b,1,bit))
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ fromp = VN_AS(nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_Biop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lsbp = nodep->lsbp()->unlink_Fr_ಹಿಂಭಾಗ();
        //
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bilhsp = fromp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ birhsp = fromp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        //
        fromp->lhsp(
            ಹೊಸ AstSel{nodep->fileline(), bilhsp, lsbp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ), nodep->ಅಗಲ_Const()});
        fromp->rhsp(ಹೊಸ AstSel{nodep->fileline(), birhsp, lsbp, nodep->ಅಗಲ_Const()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ replaceSelIntoUniop(AstSel* nodep) {
        // SEL(NOT(a),1,bit) => NOT(SEL(a,bit))
        Ast_ಗಂಟು_Uniop* ಸ್ಥಿರಾಂಕ fromp = VN_AS(nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_Uniop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lsbp = nodep->lsbp()->unlink_Fr_ಹಿಂಭಾಗ();
        //
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bilhsp = fromp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        //
        fromp->lhsp(ಹೊಸ AstSel{nodep->fileline(), bilhsp, lsbp, nodep->ಅಗಲ_Const()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Attr_ದ* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstArraySel* nodep) override {
        iterate_And_ಮುಂದಿನ_Null(nodep->bitp());
        ಆದರೆ (VN_IS(nodep->bitp(), Const)
            && VN_IS(nodep->fromp(), VarRef)
            // Need to make sure it's an array object so don't mis-allow a constant (bug509.)
            && VN_AS(nodep->fromp(), VarRef)->varp()
            && VN_IS(VN_AS(nodep->fromp(), VarRef)->varp()->valuep(), ConsArray)) {
            m_selp = nodep;  // Ask visit(AstVarRef) to replace varref with const
        }
        iterate_And_ಮುಂದಿನ_Null(nodep->fromp());
        ಆದರೆ (VN_IS(nodep->fromp(), Const)) {  // It did.
            ಆದರೆ (!m_selp) {
                nodep->v3error("Illegal assignment of constant to unpacked array");
            } ಇಲ್ಲದಿದ್ದರೆ {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ fromp = nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
                nodep->replaceWithKeepDType(fromp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
        // Handle ARRAYSEL directly on ConsArray (not through VarRef)
        ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ConsArray)) {
            ಸ್ಥಿರಾಂಕ AstConsArray* ಸ್ಥಿರಾಂಕ consp = VN_AS(nodep->fromp(), ConsArray);
            ಸ್ಥಿರಾಂಕ uint32_t ಬಿಟ್_2 = VN_AS(nodep->bitp(), Const)->ಗಮ್ಯ_UInt();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ itemp = consp->valuep(ಬಿಟ್_2);
            ಆದರೆ (VN_IS(itemp, Const)) {
                ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = VN_AS(itemp, Const)->ಅಂಕೆ();
                VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_params && itemp) {
                // An aggregate element, e.g. a structure in an array parameter
                nodep->replaceWithKeepDType(itemp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
        m_selp = ಶೂನ್ಯಸೂಚಕ;
    }

    // Evaluate a slice of an unpacked array.  If constantification is
    // required (m_required=true), call replaceWithSimulation() to compute
    // the slice via simulation.  Otherwise just iterate the children.
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ತುಂಡು_Sel* nodep) override {
        // First constify or width any child nodes
        iterateChildren(nodep);
        ಆದರೆ (!m_required) ಹಿಂತಿರುಗಿ;  // Do nothing unless we are in parameter mode
        // Fallback to simulation: this will invoke SimulateVisitor::visit(AstSliceSel*)
        replaceWithSimulation(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstStructSel* nodep) override {
        iterateChildren(nodep);

        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConsStruct* ಸ್ಥಿರಾಂಕ consp = VN_CAST(nodep->fromp(), ConsStruct)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = consp->memberValuep(nodep->name());
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(valuep, Const)) {
                VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, constp->ಅಂಕೆ()), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->replaceWith(valuep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCAwait* nodep) override {
        m_hasJumpDelay = ನಿಜ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Not linked");
        ತಾರ್ಕಿಕ did = ಸುಳ್ಳು;
        ಆದರೆ (m_doV && (!nodep->varp()->const_Pool_ನಮೂದು() || m_selp) && nodep->varp()->valuep()
            && !m_attrp) {
            // UINFOTREE(1, valuep, "", "visitvaref");
            iterate_And_ಮುಂದಿನ_Null(nodep->varp()->valuep());  // May change nodep->varp()->valuep()
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ valuep = nodep->varp()->valuep();
            // The selected element, when under a constant select of an array
            ಸ್ಥಿರಾಂಕ AstConsArray* ಸ್ಥಿರಾಂಕ consp = VN_CAST(valuep, ConsArray);
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ selItemp
                = (m_selp && consp) ? consp->valuep(m_selp->ಬಿಟ್_Const()) : ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (nodep->access().isReadOnly() && valuep
                && ((!m_params  // Can reduce constant wires into equations
                     && m_doNConst && v3Global.opt.fConst() && nodep->varp()->isConst())
                    || nodep->varp()->isParam())) {
                ಆದರೆ (ಕಾರ್ಯಾಂಶ_Const(valuep)) {
                    ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = VN_AS(valuep, Const)->ಅಂಕೆ();
                    // UINFO(2, "constVisit " << cvtToHex(valuep) << " " << num);
                    ಸ್ಥಿರಾಂಕ string origParamName
                        = nodep->varp()->isParam() ? nodep->varp()->name() : "";
                    VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ, origParamName), nodep);
                    did = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ itemp = VN_CAST(selItemp, Const)) {
                    VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, itemp->ಅಂಕೆ()), nodep);
                    did = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_params && consp) {
                    // Allow parameters to pass arrays
                    // Earlier recursion of ConsArray made sure each array value is constant
                    // This exception is fairly fragile, i.e. doesn't
                    // support arrays of arrays or other stuff
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = valuep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    did = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_params && VN_IS(valuep, ConsStruct)) {
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = valuep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    did = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->varp()->isParam() && VN_IS(valuep, Unbounded)) {
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = valuep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    did = ನಿಜ;
                }
            }
        }
        ಆದರೆ (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but variable isn't const: "
                           << nodep->varp()->prettyNameQ());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (!Ast_ಗಂಟು::afterCommentp(nodep->stmtsp())) {
            deleteVarScopesUnder(nodep->stmtsp());
            UINFO(8, "ExprStmt(...) " << nodep << " " << nodep->resultp());
            nodep->replaceWith(nodep->resultp()->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            // Removing the ExprStmt might have made something impure above now pure
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Enum_ಅಂಶ_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        ತಾರ್ಕಿಕ did = ಸುಳ್ಳು;
        ಆದರೆ (nodep->itemp()->valuep()) {
            // UINFOTREE(1, nodep->itemp()->valuep(), "", "visitvaref");
            ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ* ಸ್ಥಿರಾಂಕ itemp = nodep->itemp();
            ಆದರೆ (m_recursing_Enum_ವಸ್ತುಗಳು.ಎಣಿಕೆ_2(itemp)) {
                nodep->v3error("Recursive enum value: " << nodep->itemp()->prettyNameQ());
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_recursing_Enum_ವಸ್ತುಗಳು.emplace(itemp);
                iterate_And_ಮುಂದಿನ_Null(nodep->itemp()->valuep());
                m_recursing_Enum_ವಸ್ತುಗಳು.erase(itemp);
            }
            ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ valuep = VN_CAST(nodep->itemp()->valuep(), Const)) {
                ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = valuep->ಅಂಕೆ();
                VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ), nodep);
                did = ನಿಜ;
            }
        }
        ಆದರೆ (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but enum value isn't const: "
                           << nodep->itemp()->prettyNameQ());
        }
    }

    ತಾರ್ಕಿಕ only_Sen_ಅಂಶ_ಒಳ_Sen_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* nodep) {
        // Only one if it's not in a list
        ಹಿಂತಿರುಗಿ (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ಅಂಶ* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (m_doNConst
            && !v3Global.opt.timing().is_ಗಣ_True()  // If --timing, V3Sched would turn this into an
                                                   // infinite loop. See #5080
            && (VN_IS(nodep->sensp(), Const) || VN_IS(nodep->sensp(), Enum_ಅಂಶ_Ref)
                || (nodep->varrefp() && nodep->varrefp()->varp()->isParam()))) {
            // Constants in sensitivity lists may be removed (we'll simplify later)
            ಆದರೆ (nodep->isClocked()) {  // A constant can never get a pos/negedge
                ಆದರೆ (only_Sen_ಅಂಶ_ಒಳ_Sen_ವೃಕ್ಷ(nodep)) {
                    ಆದರೆ (nodep->ಅಂಚು_Type() == VEdgeType::ET_ಬದಲಾಯಿತು) {
                        // TODO: This really is dodgy, as strictly compliant simulators will not
                        //       execute this block, but but t_func_check relies on it
                        nodep->replaceWith(
                            ಹೊಸ Ast_Sen_ಅಂಶ{nodep->fileline(), Ast_Sen_ಅಂಶ::Initial{}});
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        nodep->replaceWith(ಹೊಸ Ast_Sen_ಅಂಶ{nodep->fileline(), Ast_Sen_ಅಂಶ::Never{}});
                    }
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {  // Otherwise it may compute a result that needs to settle out
                nodep->replaceWith(ಹೊಸ Ast_Sen_ಅಂಶ{nodep->fileline(), Ast_Sen_ಅಂಶ::Combo{}});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_doNConst && VN_IS(nodep->sensp(), Not)) {
            // V3Gate may propagate NOTs into clocks... Just deal with it
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ sensp = nodep->sensp();
            Ast_ಗಂಟು* ಕೊನೆಯ_Sensp = sensp;
            ತಾರ್ಕಿಕ invert = ಸುಳ್ಳು;
            ಆಗ (VN_IS(ಕೊನೆಯ_Sensp, Not)) {
                ಕೊನೆಯ_Sensp = VN_AS(ಕೊನೆಯ_Sensp, Not)->lhsp();
                invert = !invert;
            }
            UINFO(8, "senItem(NOT...) " << nodep << " " << invert);
            ಆದರೆ (invert) nodep->ಅಂಚು_Type(nodep->ಅಂಚು_Type().invert());
            sensp->replaceWith(ಕೊನೆಯ_Sensp->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(sensp), sensp);
        }
    }

    ವರ್ಗ Sen_ಅಂಶ_Cmp final {
        ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ cmp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ap, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bp) {
            ಸ್ಥಿರಾಂಕ VNType aType = ap->type();
            ಸ್ಥಿರಾಂಕ VNType bType = bp->type();
            ಆದರೆ (aType != bType) ಹಿಂತಿರುಗಿ static_cast<ಪೂರ್ಣಾಂಕ>(bType) - static_cast<ಪೂರ್ಣಾಂಕ>(aType);

            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ aRefp = VN_CAST(ap, VarRef)) {
                ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ bRefp = VN_AS(bp, VarRef);
                // Looks visually better if we keep sorted by name
                ಆದರೆ (aRefp->name() < bRefp->name()) ಹಿಂತಿರುಗಿ -1;
                ಆದರೆ (aRefp->name() > bRefp->name()) ಹಿಂತಿರುಗಿ 1;
                // But might be same name with different scopes
                ಆದರೆ (aRefp->varScopep() < bRefp->varScopep()) ಹಿಂತಿರುಗಿ -1;
                ಆದರೆ (aRefp->varScopep() > bRefp->varScopep()) ಹಿಂತಿರುಗಿ 1;
                // Or rarely, different data types
                ಆದರೆ (aRefp->dtypep() < bRefp->dtypep()) ಹಿಂತಿರುಗಿ -1;
                ಆದರೆ (aRefp->dtypep() > bRefp->dtypep()) ಹಿಂತಿರುಗಿ 1;
                ಹಿಂತಿರುಗಿ 0;
            }

            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ aConstp = VN_CAST(ap, Const)) {
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ bConstp = VN_AS(bp, Const);
                ಆದರೆ (aConstp->ಅಂಕೆ().isLtXZ(bConstp->ಅಂಕೆ())) ಹಿಂತಿರುಗಿ -1;
                ಆದರೆ (bConstp->ಅಂಕೆ().isLtXZ(aConstp->ಅಂಕೆ())) ಹಿಂತಿರುಗಿ 1;
                ಹಿಂತಿರುಗಿ 0;
            }

            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ aBiOpp = VN_CAST(ap, ಗಂಟು_Biop)) {
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ bBiOpp = VN_AS(bp, ಗಂಟು_Biop);
                // Compare RHSs first, as LHS might be const
                ಆದರೆ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ c = cmp(aBiOpp->rhsp(), bBiOpp->rhsp())) ಹಿಂತಿರುಗಿ c;
                ಹಿಂತಿರುಗಿ cmp(aBiOpp->lhsp(), bBiOpp->lhsp());
            }

            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCMethodHard* ಸ್ಥಿರಾಂಕ aCallp = VN_CAST(ap, CMethodHard)) {
                ಸ್ಥಿರಾಂಕ AstCMethodHard* ಸ್ಥಿರಾಂಕ bCallp = VN_AS(bp, CMethodHard);
                ಆದರೆ (aCallp->ವಿಧಾನ() < bCallp->ವಿಧಾನ()) ಹಿಂತಿರುಗಿ -1;
                ಆದರೆ (aCallp->ವಿಧಾನ() > bCallp->ವಿಧಾನ()) ಹಿಂತಿರುಗಿ 1;
                ಆದರೆ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ c = cmp(aCallp->fromp(), bCallp->fromp())) ಹಿಂತಿರುಗಿ c;
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* aPinsp = aCallp->pinsp();
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bPinsp = bCallp->pinsp();
                ಆಗ (aPinsp && bPinsp) {
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ c = cmp(aPinsp, bPinsp)) ಹಿಂತಿರುಗಿ c;
                    aPinsp = VN_AS(aPinsp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                    bPinsp = VN_AS(bPinsp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                }
                ಹಿಂತಿರುಗಿ aPinsp ? -1 : bPinsp ? 1 : 0;
            }

            ಹಿಂತಿರುಗಿ 0;
        }

    ಸಾರ್ವಜನಿಕ:
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* lhsp, ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* rhsp) ಸ್ಥಿರಾಂಕ {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lSensp = lhsp->sensp();
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rSensp = rhsp->sensp();
            ಆದರೆ (lSensp && rSensp) {
                // If both terms have sensitivity expressions, recursively compare them
                ಆದರೆ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ c = cmp(lSensp, rSensp)) ಹಿಂತಿರುಗಿ c < 0;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lSensp || rSensp) {
                // Terms with sensitivity expressions come after those without
                ಹಿಂತಿರುಗಿ rSensp;
            }
            // Finally sort by edge, AFTER variable, as we want multiple edges for same var
            // adjacent. note the SenTree optimizer requires this order (more general first,
            // less general last)
            ಹಿಂತಿರುಗಿ lhsp->ಅಂಚು_Type() < rhsp->ಅಂಚು_Type();
        }
    };

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (m_doExpensive) {
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
            ಗಾಗಿ (Ast_Sen_ಅಂಶ *nextp, *senp = nodep->sensesp(); senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), Sen_ಅಂಶ);
                // cppcheck-suppress unassignedVariable  // cppcheck bug
                ಸ್ಥಿರಾಂಕ Sen_ಅಂಶ_Cmp cmp;
                ಆದರೆ (nextp && !cmp(senp, nextp)) {
                    // Something's out of order, sort it
                    senp = ಶೂನ್ಯಸೂಚಕ;
                    std::ಸದಿಶ<Ast_Sen_ಅಂಶ*> vec;
                    ಗಾಗಿ (Ast_Sen_ಅಂಶ* senp = nodep->sensesp(); senp;
                         senp = VN_AS(senp->nextp(), Sen_ಅಂಶ)) {
                        vec.ತಳ್ಳು_ಹಿಂಭಾಗ(senp);
                    }
                    stable_ವಿಂಗಡಿಸು(vec.begin(), vec.ಕೊನೆ(), Sen_ಅಂಶ_Cmp());
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ip : vec) ip->unlink_Fr_ಹಿಂಭಾಗ();
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ip : vec) nodep->ಕೂಡಿಸು_Sensesp(ip);
                    ನಿಲ್ಲಿಸು;
                }
            }

            // Pass 2, remove duplicates and simplify adjacent terms if possible
            ಗಾಗಿ (Ast_Sen_ಅಂಶ *senp = nodep->sensesp(), *nextp; senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), Sen_ಅಂಶ);
                ಆದರೆ (!nextp) ನಿಲ್ಲಿಸು;
                Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ lItemp = senp;
                Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ rItemp = nextp;
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lSenp = lItemp->sensp();
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rSenp = rItemp->sensp();
                ಆದರೆ (!lSenp || !rSenp) ಮುಂದುವರಿ;

                ಆದರೆ (lSenp->same_Gate_ವೃಕ್ಷ(rSenp)) {
                    // POSEDGE or NEGEDGE -> BOTHEDGE. (We've sorted POSEDGE, before NEGEDGE, so we
                    // do not need to test for the opposite orders.)
                    ಆದರೆ (lItemp->ಅಂಚು_Type() == VEdgeType::ET_POSEDGE
                        && rItemp->ಅಂಚು_Type() == VEdgeType::ET_NEGEDGE) {
                        // Make both terms BOTHEDGE, the second will be removed below
                        lItemp->ಅಂಚು_Type(VEdgeType::ET_BOTHEDGE);
                        rItemp->ಅಂಚು_Type(VEdgeType::ET_BOTHEDGE);
                    }

                    // Remove identical expressions
                    ಆದರೆ (lItemp->ಅಂಚು_Type() == rItemp->ಅಂಚು_Type()) {
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(rItemp->unlink_Fr_ಹಿಂಭಾಗ()), rItemp);
                        nextp = lItemp;
                    }
                }
            }
        }
    }

    //-----
    // Zero elimination
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (nodep->timingControlp()) m_hasJumpDelay = ನಿಜ;
        ಆದರೆ (m_doNConst && replace_ಗಂಟು_ನಿಯೋಜಿಸು(nodep)) ಹಿಂತಿರುಗಿ;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlias* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAliasScope* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_W* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (m_doNConst && replace_ಗಂಟು_ನಿಯೋಜಿಸು(nodep)) ಹಿಂತಿರುಗಿ;
        // Process containing this AssignW as single body statement
        AstAlways* ಸ್ಥಿರಾಂಕ procp = VN_CAST(nodep->backp(), Always);
        ಆದರೆ (!procp || procp->stmtsp() != nodep || nodep->nextp()) ಹಿಂತಿರುಗಿ;
        // Not VarXRef, as different refs may set different values to each hierarchy
        Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(nodep->lhsp(), VarRef);
        ಆದರೆ (m_wremove && !m_params && m_doNConst && m_modp && ಕಾರ್ಯಾಂಶ_Const(nodep->rhsp())
            && !VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().isFourState()
            && varrefp  // Don't do messes with BITREFs/ARRAYREFs
            && !varrefp->varp()->hasStrengthAssignment()  // Strengths are resolved in V3Tristate
            && !varrefp->varp()->valuep()  // Not already constified
            && !varrefp->varScopep()  // Not scoped (or each scope may have different initial val.)
            && !varrefp->varp()->isForced()  // Not forced (not really a constant)
        ) {
            // ASSIGNW (VARREF, const) -> INITIAL ( ASSIGN (VARREF, const) )
            UINFO(4, "constAssignW " << nodep);
            // Make a initial assignment
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            varrefp->unlink_Fr_ಹಿಂಭಾಗ();
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
            AstInitial* ಸ್ಥಿರಾಂಕ newinitp = ಹೊಸ AstInitial{flp, ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, varrefp, exprp}};
            procp->replaceWith(newinitp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(procp), procp);
            // Set the initial value right in the variable so we can constant propagate
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ initvaluep = exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            varrefp->varp()->isConst(ನಿಜ);
            varrefp->varp()->valuep(initvaluep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCReset* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (!m_doNConst) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ bdtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType);
        ಆದರೆ (!bdtypep) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!bdtypep->isZeroInit()) ಹಿಂತಿರುಗಿ;
        AstConst* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{nodep->fileline(), V3_ಸಂಖ್ಯೆ{nodep, bdtypep}};
        UINFO(9, "CRESET(0) => CONST(0) " << nodep);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cvt_Unpack_ಗಮ್ಯ_Unpack* nodep) override {
        iterateChildren(nodep);
        AstConsArray* ಸ್ಥಿರಾಂಕ fromp = VN_CAST(nodep->fromp(), ConsArray);
        ಆದರೆ (!fromp) ಹಿಂತಿರುಗಿ;
        // Fold the outermost dimension, converting the elements for any dimension under it
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_Arrayp
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ fromArrayp
            = VN_AS(fromp->dtypep()->skipRefp(), UnpackArrayDType);
        // This dimension may not be reversed, only one under it
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಹಿಮ್ಮುಖ
            = ಗಮ್ಯ_Arrayp->declRange().ascending() != fromArrayp->declRange().ascending();
        ಸ್ಥಿರಾಂಕ uint64_t ಕೊನೆಯ_ಸೂಚ್ಯಂಕ = ಗಮ್ಯ_Arrayp->elementsConst() - 1;
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subDTypep = ಗಮ್ಯ_Arrayp->subDTypep();
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ subArrayp
            = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ convert = [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            valuep->unlink_Fr_ಹಿಂಭಾಗ();
            ಆದರೆ (!subArrayp || !subArrayp->is_ಹಿಮ್ಮುಖ_ದ(valuep->dtypep())) ಹಿಂತಿರುಗಿ valuep;
            ಹಿಂತಿರುಗಿ ಹೊಸ Ast_Cvt_Unpack_ಗಮ್ಯ_Unpack{valuep->fileline(), valuep, subDTypep};
        };
        AstConsArray* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ AstConsArray{fromp->fileline(), nodep->dtypep(),
                               fromp->defaultp() ? convert(fromp->defaultp()) : ಶೂನ್ಯಸೂಚಕ};
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Cons_Array_ಅಂಶ* itemp = fromp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_ಅಂಶ)) {
            newp->valuep(ಹಿಮ್ಮುಖ ? ಕೊನೆಯ_ಸೂಚ್ಯಂಕ - itemp->ಸೂಚ್ಯಂಕ() : itemp->ಸೂಚ್ಯಂಕ(),
                         convert(itemp->valuep()));
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        iterate(newp);  // Fold the conversions of the elements
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cvt_Array_ಗಮ್ಯ_Array* nodep) override {
        iterateChildren(nodep);
        // Handle the case where we have a stream operation inside a cast conversion
        // Check for both StreamL and StreamR operations
        Ast_ಗಂಟು_ಹರಿವು* streamp = ಶೂನ್ಯಸೂಚಕ;
        ತಾರ್ಕಿಕ is_ಹಿಮ್ಮುಖ = ಸುಳ್ಳು;
        ಆದರೆ (Ast_ಹರಿವು_L* ಸ್ಥಿರಾಂಕ ಹರಿವು_Lp = VN_CAST(nodep->fromp(), ಹರಿವು_L)) {
            streamp = ಹರಿವು_Lp;
            is_ಹಿಮ್ಮುಖ = ನಿಜ;  // StreamL reverses the operation
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಹರಿವು_R* ಸ್ಥಿರಾಂಕ ಹರಿವು_Rp = VN_CAST(nodep->fromp(), ಹರಿವು_R)) {
            streamp = ಹರಿವು_Rp;
            is_ಹಿಮ್ಮುಖ = ಸುಳ್ಳು;  // StreamR doesn't reverse the operation
        }
        ಆದರೆ (streamp) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = streamp->lhsp();
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ srcDTypep = srcp->dtypep()->skipRefp();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dstDTypep = nodep->dtypep()->skipRefp();
            ಆದರೆ (VN_IS(srcDTypep, ಸರತಿ_DType) && VN_IS(dstDTypep, ಸರತಿ_DType)) {
                ಪೂರ್ಣಾಂಕ ಖಂಡ_ಗಾತ್ರ = 1;
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(streamp->rhsp(), Const)) {
                    ಖಂಡ_ಗಾತ್ರ = constp->ಗಮ್ಯ_SInt();
                    ಆದರೆ (VL_UNLIKELY(ಖಂಡ_ಗಾತ್ರ <= 0)) {
                        // Not reachable due to higher level checks when parsing stream
                        // operators commented out to not fail v3error-coverage-checks.
                        // nodep->v3error("Stream block size must be positive, got " <<
                        // blockSize);
                        ಖಂಡ_ಗಾತ್ರ = 1;
                    }
                }
                // Not reachable due to higher level checks when parsing stream operators
                // commented out to not fail v3error-coverage-checks.
                // else {
                //    nodep->v3error("Stream block size must be constant (got " <<
                //    streamp->rhsp()->prettyTypeName() << ")");
                // }
                ಪೂರ್ಣಾಂಕ src_Element_ಬಿಟ್ಟುಗಳು = 0;
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = srcDTypep->subDTypep()) {
                    src_Element_ಬಿಟ್ಟುಗಳು = elemDtp->ಅಗಲ();
                }
                ಪೂರ್ಣಾಂಕ dst_Element_ಬಿಟ್ಟುಗಳು = 0;
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = dstDTypep->subDTypep()) {
                    dst_Element_ಬಿಟ್ಟುಗಳು = elemDtp->ಅಗಲ();
                }
                streamp->unlink_Fr_ಹಿಂಭಾಗ();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ಹೊಸ Ast_Cvt_Array_ಗಮ್ಯ_Array{
                    srcp->fileline(), srcp->unlink_Fr_ಹಿಂಭಾಗ(), dstDTypep,     is_ಹಿಮ್ಮುಖ,
                    ಖಂಡ_ಗಾತ್ರ,        dst_Element_ಬಿಟ್ಟುಗಳು,       src_Element_ಬಿಟ್ಟುಗಳು};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(streamp), streamp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRelease* nodep) override {
        // cppcheck-suppress constVariablePointer // children unlinked below
        ಆದರೆ (AstConcat* ಸ್ಥಿರಾಂಕ concatp = VN_CAST(nodep->lhsp(), Concat)) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
            AstRelease* ಸ್ಥಿರಾಂಕ newLp = ಹೊಸ AstRelease{flp, concatp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ()};
            AstRelease* ಸ್ಥಿರಾಂಕ newRp = ಹೊಸ AstRelease{flp, concatp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ()};
            nodep->replaceWith(newLp);
            newLp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newRp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಭೇಟಿ(newLp);
            ಭೇಟಿ(newRp);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (m_doNConst) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->condp(), Const)) {
                Ast_ಗಂಟು* keepp = ಶೂನ್ಯಸೂಚಕ;
                Ast_ಗಂಟು* delp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (constp->isZero()) {
                    UINFO(4, "IF(0,{any},{x}) => {x}: " << nodep);
                    keepp = nodep->elsesp();
                    delp = nodep->thensp();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_doV || constp->isNeqZero()) {  // Might be X in Verilog
                    UINFO(4, "IF(!0,{x},{any}) => {x}: " << nodep);
                    keepp = nodep->thensp();
                    delp = nodep->elsesp();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    UINFO(4, "IF condition is X, retaining: " << nodep);
                    ಹಿಂತಿರುಗಿ;
                }
                ಆದರೆ (!canDeleteStatements()) {
                    UINFO(4, "IF retained until statics lifted: " << nodep);
                    ಹಿಂತಿರುಗಿ;
                }

                // If we delete a branch that contains variable declarations, also delete the
                // corresponding varscopes so we don't leave dangling AstVarScope::m_varp pointers.
                deleteVarScopesUnder(delp);

                ಆದರೆ (keepp) {
                    keepp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                    nodep->replaceWith(keepp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->unlink_Fr_ಹಿಂಭಾಗ();
                    deleteVarScopesUnder(nodep->thensp());
                    deleteVarScopesUnder(nodep->elsesp());
                }
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);

                // Removed branch could contain only impurity within a function and the
                // function could be purified
                VIsCached::clear_Cache_ವೃಕ್ಷ();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!Ast_ಗಂಟು::afterCommentp(nodep->thensp())
                       && !Ast_ಗಂಟು::afterCommentp(nodep->elsesp())) {
                ಆದರೆ (!nodep->condp()->is_ಶುದ್ಧ()) {
                    // Condition has side effect - leave, but don't need return value
                    UINFO(4, "IF({x}) ; => STMTEXPR({x}): " << nodep);
                    nodep->fileline()->warnOff(V3_Error_ಸಂಕೇತ::IGNOREDRETURN, ನಿಜ);
                    Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ newp
                        = ಹೊಸ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{nodep->fileline(), nodep->condp()->unlink_Fr_ಹಿಂಭಾಗ()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Empty block, remove it
                    UINFO(4, "IF({x}) ; => ; : " << nodep);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!Ast_ಗಂಟು::afterCommentp(nodep->thensp())) {
                UINFO(4, "IF({x}) nullptr {...} => IF(NOT{x}}: " << nodep);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = nodep->condp();
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ elsesp = nodep->elsesp();
                condp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                elsesp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                ಆದರೆ (nodep->thensp()) {  // Must have been comment
                    ತಳ್ಳು_Deletep(nodep->thensp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                }
                nodep->condp(ಹೊಸ AstLogNot{condp->fileline(),
                                           condp});  // LogNot, as C++ optimization also possible
                nodep->ಕೂಡಿಸು_Thensp(elsesp);
                nodep->branchPred(nodep->branchPred().invert());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (((VN_IS(nodep->condp(), Not) && nodep->condp()->ಅಗಲ() == 1)
                        || VN_IS(nodep->condp(), LogNot))
                       && nodep->thensp() && nodep->elsesp()) {
                UINFO(4, "IF(NOT {x})  => IF(x) swapped if/else" << nodep);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp
                    = VN_AS(nodep->condp(), ಗಂಟು_Uniop)->lhsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ thensp = nodep->thensp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ elsesp = nodep->elsesp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{nodep->fileline(), condp, elsesp, thensp};
                ifp->isBoundsCheck(nodep->isBoundsCheck());  // Copy bounds check info
                ifp->branchPred(nodep->branchPred().invert());
                nodep->replaceWith(ifp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (if_Same_ನಿಯೋಜಿಸು(nodep)) {
                UINFO(4,
                      "IF({a}) ASSIGN({b},{c}) else ASSIGN({b},{d}) => ASSIGN({b}, {a}?{c}:{d})");
                Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ thensp = VN_AS(nodep->thensp(), ಗಂಟು_ನಿಯೋಜಿಸು);
                // cppcheck-suppress constVariablePointer // children unlinked below
                Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ elsesp = VN_AS(nodep->elsesp(), ಗಂಟು_ನಿಯೋಜಿಸು);
                thensp->unlink_Fr_ಹಿಂಭಾಗ();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = nodep->condp()->unlink_Fr_ಹಿಂಭಾಗ();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ truep = thensp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ falsep = elsesp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                thensp->rhsp(ಹೊಸ AstCond{truep->fileline(), condp, truep, falsep});
                nodep->replaceWith(thensp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸುಳ್ಳು  // Disabled, as vpm assertions are faster
                              // without due to short-circuiting
                       && ಕಾರ್ಯಾಂಶ_If_If(nodep)) {
                UINFO(9, "IF({a}) IF({b}) => IF({a} && {b})");
                Ast_ಗಂಟು_If* ಸ್ಥಿರಾಂಕ ಅಧೋ_Ifp = VN_AS(nodep->thensp(), ಗಂಟು_If);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = nodep->condp()->unlink_Fr_ಹಿಂಭಾಗ();
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ ಅಧೋ_Thensp = ಅಧೋ_Ifp->thensp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಅಧೋ_Condp = ಅಧೋ_Ifp->condp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                nodep->condp(ಹೊಸ AstLogAnd{ಅಧೋ_Ifp->fileline(), condp, ಅಧೋ_Condp});
                ಅಧೋ_Ifp->replaceWith(ಅಧೋ_Thensp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(ಅಧೋ_Ifp), ಅಧೋ_Ifp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Optimizations that don't reform the IF itself
                ಆದರೆ (ಕಾರ್ಯಾಂಶ_Bool_ಸರಿಸು(nodep->condp())) replace_Bool_ಸರಿಸು(nodep->condp());
                ಹೊಂದಿಕೆ_If_Same_ಕೊನೆಯ(nodep);
                ಹೊಂದಿಕೆ_If_Cond_Cond(nodep);
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstDisplay* nodep) override {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        iterateChildren(nodep);
        ಆದರೆ (stmtDisplayDisplay(nodep)) ಹಿಂತಿರುಗಿ;
    }
    ತಾರ್ಕಿಕ stmtDisplayDisplay(AstDisplay* nodep) {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        ಆದರೆ (!m_modp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Don't optimize under single statement
        AstDisplay* ಸ್ಥಿರಾಂಕ prevp = VN_CAST(nodep->backp(), Display);
        ಆದರೆ (!prevp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!((prevp->displayType() == nodep->displayType())
              || (prevp->displayType() == VDisplayType::DT_WRITE
                  && nodep->displayType() == VDisplayType::DT_DISPLAY)
              || (prevp->displayType() == VDisplayType::DT_DISPLAY
                  && nodep->displayType() == VDisplayType::DT_WRITE)))
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ ((prevp->filep() && !nodep->filep()) || (!prevp->filep() && nodep->filep())
            || (prevp->filep() && nodep->filep() && !prevp->filep()->same_ವೃಕ್ಷ(nodep->filep())))
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!prevp->fmtp() || prevp->fmtp()->nextp() || !nodep->fmtp() || nodep->fmtp()->nextp())
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstSFormatF* ಸ್ಥಿರಾಂಕ pformatp = prevp->fmtp();
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSFormatF* ಸ್ಥಿರಾಂಕ nformatp = nodep->fmtp();
        // We don't merge scopeNames as can have only one and might be different scopes (late in
        // process) Also rare for real code to print %m multiple times in same message
        ಆದರೆ (nformatp->scopeNamep() && pformatp->scopeNamep()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // We don't early merge arguments as might need to later print warnings with
        // right line numbers, nor scopeNames as might be different scopes (late in process)
        ಆದರೆ (!m_doCpp && pformatp->exprsp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!m_doCpp && nformatp->exprsp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (pformatp->exprsp() && !pformatp->exprsp()->is_ಶುದ್ಧ_And_ಮುಂದಿನ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (nformatp->exprsp() && !nformatp->exprsp()->is_ಶುದ್ಧ_And_ಮುಂದಿನ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Avoid huge merges
        ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ DISPLAY_MAX_ವಿಲೀನ_ಉದ್ದಳತೆ = 500;
        ಆದರೆ (pformatp->ಪಠ್ಯ().ಉದ್ದಳತೆ() + nformatp->ಪಠ್ಯ().ಉದ್ದಳತೆ() > DISPLAY_MAX_ವಿಲೀನ_ಉದ್ದಳತೆ)
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        //
        UINFO(9, "DISPLAY(SF({a})) DISPLAY(SF({b})) -> DISPLAY(SF({a}+{b}))");
        // Convert DT_DISPLAY to DT_WRITE as may allow later optimizations
        ಆದರೆ (prevp->displayType() == VDisplayType::DT_DISPLAY) {
            prevp->displayType(VDisplayType::DT_WRITE);
            pformatp->ಪಠ್ಯ(pformatp->ಪಠ್ಯ() + "\n");
        }
        // We can't replace prev() as the edit tracking iterators will get confused.
        // So instead we edit the prev note itself.
        ಆದರೆ (prevp->ಕೂಡಿಸು_Newline()) pformatp->ಪಠ್ಯ(pformatp->ಪಠ್ಯ() + "\n");
        pformatp->ಪಠ್ಯ(pformatp->ಪಠ್ಯ() + nformatp->ಪಠ್ಯ());
        ಆದರೆ (!prevp->ಕೂಡಿಸು_Newline() && nodep->ಕೂಡಿಸು_Newline()) pformatp->ಪಠ್ಯ(pformatp->ಪಠ್ಯ() + "\n");
        ಆದರೆ (nformatp->exprsp()) pformatp->ಕೂಡಿಸು_Exprsp(nformatp->exprsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        ಆದರೆ (AstScopeName* ಸ್ಥಿರಾಂಕ scopeNamep = nformatp->scopeNamep()) {
            scopeNamep->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            pformatp->scopeNamep(scopeNamep);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatArg* nodep) override {
        // Skip namep(): requiring its runtime lookup to be constant can reject valid
        // enum-valued constant-function calls in parameters. displayedEnum() resolves
        // the name from the folded exprp() value and enum dtype instead.
        iterate_And_ಮುಂದಿನ_Null(nodep->exprp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatF* nodep) override {
        // Substitute constants into displays.  The main point of this is to
        // simplify assertion methodologies which call functions with display's.
        // This eliminates a pile of wide temps, and makes the C a whole lot more readable.
        iterateChildren(nodep);
        ಆದರೆ (nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2()) {
            // Upconvert to text-based format?
            // Similar code in V3LinkResolve::visit(AstSFormatF)
            UASSERT_OBJ(nodep->ಪಠ್ಯ() == "", nodep,
                        "Non-format $sformatf should have text format == \"\"");
            ಆದರೆ (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->ಅಂಕೆ().isFromString()) {
                AstConst* ಸ್ಥಿರಾಂಕ fmtp = VN_AS(nodep->exprsp()->unlink_Fr_ಹಿಂಭಾಗ(), Const);
                nodep->ಪಠ್ಯ(fmtp->ಅಂಕೆ().ಗಮ್ಯ_String());
                nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2(ಸುಳ್ಳು);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(fmtp), fmtp);
            }
        }
        ಆದರೆ (nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2()) ಹಿಂತಿರುಗಿ;  // Runtime, unfortunately  (unless constify format later)
        ತಾರ್ಕಿಕ anyconst = ಸುಳ್ಳು;
        ಗಾಗಿ (Ast_ಗಂಟು* argp = nodep->exprsp(); argp; argp = argp->nextp()) {
            AstSFormatArg* ಸ್ಥಿರಾಂಕ fargp = VN_CAST(argp, SFormatArg);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ subargp = fargp ? fargp->exprp() : argp;
            ಆದರೆ (VN_IS(subargp, Const)) {
                anyconst = ನಿಜ;
                ನಿಲ್ಲಿಸು;
            }
        }
        ಆದರೆ (m_doNConst && anyconst) {
            UINFO(9, "  Display in  " << nodep->ಪಠ್ಯ());
            string newFormat;
            string fmt;
            ತಾರ್ಕಿಕ ಒಳ_Pct = ಸುಳ್ಳು;
            Ast_ಗಂಟು* argp = nodep->exprsp();
            ಸ್ಥಿರಾಂಕ string ಪಠ್ಯ = nodep->ಪಠ್ಯ();
            ಗಾಗಿ (ಅಕ್ಷರ ch : ಪಠ್ಯ) {
                ಆದರೆ (!ಒಳ_Pct && ch == '%') {
                    ಒಳ_Pct = ನಿಜ;
                    fmt = ch;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಒಳ_Pct && (std::ಅಂಕಿಯೇ(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಒಳ_Pct) {
                    ಒಳ_Pct = ಸುಳ್ಳು;
                    fmt += ch;
                    ch = std::tolower(ch);
                    ಆಯ್ಕೆ (ch) {
                    ಸ್ಥಿತಿ '%': ನಿಲ್ಲಿಸು;  // %% - still %%
                    ಸ್ಥಿತಿ 'm': ನಿಲ್ಲಿಸು;  // %m - still %m - auto insert "name"
                    ಸ್ಥಿತಿ 'l': ನಿಲ್ಲಿಸು;  // %l - still %l - auto insert "library"
                    ಸ್ಥಿತಿ 't':  // %t - don't know $timeformat so can't constify
                        ಆದರೆ (argp) argp = argp->nextp();
                        ನಿಲ್ಲಿಸು;
                    ಪೂರ್ವನಿಯೋಜಿತ:  // Most operators, just move to next argument
                        ಆದರೆ (argp) {
                            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = argp->nextp();
                            AstSFormatArg* ಸ್ಥಿರಾಂಕ fargp = VN_CAST(argp, SFormatArg);
                            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ subargp = fargp ? fargp->exprp() : argp;
                            ಸ್ಥಿರಾಂಕ VFormatAttr formatAttr
                                = argp
                                      ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                                      : VFormatAttr{};
                            ಆದರೆ (VN_IS(subargp, Const)) {  // Convert it
                                ಸ್ಥಿರಾಂಕ string ಹೊರಹೋಗು
                                    = formatAttr.isEnum()
                                          ? const_ಅಂಕೆ_V(subargp).displayedEnum(fargp, fmt)
                                          : const_ಅಂಕೆ_V(subargp).displayed(nodep, fmt, formatAttr);
                                UINFO(9, "     DispConst: " << fmt << " -> " << ಹೊರಹೋಗು << "  for "
                                                            << subargp);
                                // fmt = out w/ replace % with %% as it must later when
                                // used as a format output as literal.
                                fmt = VString::quotePercent(ಹೊರಹೋಗು);
                                VL_DO_DANGLING(ತಳ್ಳು_Deletep(argp->unlink_Fr_ಹಿಂಭಾಗ()), argp);
                            }
                            argp = nextp;
                        }
                        ನಿಲ್ಲಿಸು;
                    }  // switch
                    newFormat += fmt;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    newFormat += ch;
                }
            }
            ಆದರೆ (newFormat != nodep->ಪಠ್ಯ()) {
                nodep->ಪಠ್ಯ(newFormat);
                UINFO(9, "  Display out " << nodep);
            }
        }
        ಆದರೆ (!nodep->exprsp() && nodep->ಪಠ್ಯ().ಕಂಡುಹಿಡಿ('%') == string::npos && !nodep->hidden()) {
            // Just a simple constant string - the formatting is pointless
            UINFO(9, "replaceConstStr");
            VL_DO_DANGLING(replaceConstString(nodep, nodep->ಪಠ್ಯ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (VN_IS(nodep->backp(), ಗಂಟು_ನಿಯೋಜಿಸು)  // Can't remove under Display etc
            && nodep->ಪಠ್ಯ() == "%s" && VN_IS(nodep->exprsp(), SFormatArg)
            && VN_AS(nodep->exprsp(), SFormatArg)->formatAttr().isString()
            && !nodep->exprsp()->nextp()) {
            UINFO(9, "  Display(%p, expr) -> constant expr " << nodep);
            nodep->replaceWith(VN_AS(nodep->exprsp(), SFormatArg)->exprp()->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormat* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (!m_doNConst) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (m_doNConst) {
            // If it's a constant string written to a string variable, inline it as an assignment
            AstSFormatF* ಸ್ಥಿರಾಂಕ fmtp = nodep->fmtp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp();
            AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = VN_CAST(lhsp->dtypep()->skipRefp(), BasicDType);
            ಆದರೆ (basicp && basicp->isString() && !fmtp->exprsp()
                && fmtp->ಪಠ್ಯ().ಕಂಡುಹಿಡಿ('%') == string::npos) {
                AstConst* ಸ್ಥಿರಾಂಕ strp
                    = ಹೊಸ AstConst{fmtp->fileline(), AstConst::String{}, fmtp->ಪಠ್ಯ()};
                lhsp->unlink_Fr_ಹಿಂಭಾಗ();
                nodep->replaceWith(ಹೊಸ Ast_ನಿಯೋಜಿಸು{nodep->fileline(), lhsp, strp});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_underRecFunc);
        ಆದರೆ (nodep->recursive()) m_underRecFunc = ನಿಜ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        iterateChildren(nodep);
        m_hasJumpDelay = ನಿಜ;  // As don't analyze inside tasks for timing controls
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        // Note excludes AstFuncRef as other visitor below
        iterateChildren(nodep);
        m_hasJumpDelay = ನಿಜ;  // As don't analyze inside tasks for timing controls
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFuncRef* nodep) override {
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು_FTask_Ref*>(nodep));
        ಆದರೆ (m_params) {  // Only parameters force us to do constant function call propagation
            replaceWithSimulation(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstArg* nodep) override {
        // replaceWithSimulation on the Arg's parent FuncRef replaces these
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override {
        VL_RESTORER(m_hasLoopTest);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ oldHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = ಸುಳ್ಳು;
        m_hasLoopTest = ಸುಳ್ಳು;
        iterateChildren(nodep);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ thisLoopHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = thisLoopHasJumpDelay || oldHasJumpDelay;
        // If the first statement always break, the loop is useless
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstLoopTest* ಸ್ಥಿರಾಂಕ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            ಆದರೆ (testp->condp()->isZero() && (!testp->nextp() || canDeleteStatements())) {
                nodep->v3warn(UNUSEDLOOP, "Loop condition is always false");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                ಹಿಂತಿರುಗಿ;
            }
        }
        // If last statement always breaks, repalce loop with body
        ಆದರೆ (Ast_ಗಂಟು* lastp = nodep->stmtsp()) {
            ಆಗ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = lastp->nextp()) lastp = nextp;
            ಆದರೆ (AstLoopTest* ಸ್ಥಿರಾಂಕ testp = VN_CAST(lastp, LoopTest)) {
                ಆದರೆ (testp->condp()->isZero()) {
                    VL_DO_DANGLING2(ತಳ್ಳು_Deletep(testp->unlink_Fr_ಹಿಂಭಾಗ()), testp, lastp);
                    nodep->replaceWith(nodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
        // Warn on infinite loop
        ಆದರೆ (!m_hasLoopTest && !thisLoopHasJumpDelay) {
            nodep->v3warn(INFINITELOOP, "Infinite loop (condition always true)");
            nodep->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::INFINITELOOP, ನಿಜ);  // Complain once
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoopTest* nodep) override {
        iterateChildren(nodep);

        // If never breaks, remove
        ಆದರೆ (nodep->condp()->isNeqZero()) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        m_hasLoopTest = ನಿಜ;

        // If always breaks, subsequent statements are dead code, delete them
        ಆದರೆ (nodep->condp()->isZero()) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = nodep->nextp();
            ಆದರೆ (nextp && canDeleteStatements()) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nextp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()), nodep);
            }
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (ಕಾರ್ಯಾಂಶ_Bool_ಸರಿಸು(nodep->condp())) replace_Bool_ಸರಿಸು(nodep->condp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstConsArray* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstConsStruct* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cons_Struct_ಅಂಶ* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cons_Array_ಅಂಶ* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstUnbounded* nodep) override { iterateChildren(nodep); }
    // These are converted by V3Param.  Don't constify as we don't want the
    // from() VARREF to disappear, if any.
    // If output of a presel didn't get consted, chances are V3Param didn't visit properly
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Pre_Sel*) override {}

    // Ignored, can eliminate early
    ಶೂನ್ಯ ಭೇಟಿ(AstSysIgnore* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (m_doNConst) VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        iterateChildren(nodep);
        // Malformed due to child being deleted, remove here
        ಆದರೆ (!nodep->exprp()) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // Remove if expression is trivially pure
        ಆದರೆ (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), VarRef)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // Remove calls to empty functions
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCCall* ಸ್ಥಿರಾಂಕ callp = VN_CAST(nodep->exprp(), CCall)) {
            ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ funcp = callp->funcp();
            ಆದರೆ (!callp->argsp() && funcp->ಖಾಲಿಗಳು_Body()) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                ಹಿಂತಿರುಗಿ;
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
    ಶೂನ್ಯ ಭೇಟಿ(AstBasicDType* nodep) override {
        iterateChildren(nodep);
        nodep->cvtRangeConst();
    }

    //-----
    // Jump elimination

    ಶೂನ್ಯ ಭೇಟಿ(AstJumpGo* nodep) override {
        // Any statements following the JumpGo (at this statement level) never execute, delete
        ಆದರೆ (nodep->nextp() && canDeleteStatements()) {
            ತಳ್ಳು_Deletep(nodep->nextp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        }

        // JumpGo as last statement in target JumpBlock (including last in a last sub-list),
        // is a no-op, remove it.
        ಆದರೆ (!nodep->nextp()) {
            ಗಾಗಿ (Ast_ಗಂಟು* abovep = nodep->abovep(); abovep; abovep = abovep->abovep()) {
                ಆದರೆ (abovep == nodep->blockp()) {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
                // Stop if not doing expensive, or if the above node is not the last in its list,
                // ... or if it's not an 'if' TODO: it would be enough if it was not a branch.
                ಆದರೆ (!m_doExpensive || abovep->nextp() || !VN_IS(abovep, If)) ನಿಲ್ಲಿಸು;
            }
        }
        // Mark JumpBlock as used
        m_ಬಳಸಿದ_Jump_ಖಂಡಗಳು.emplace(nodep->blockp());
        m_hasJumpDelay = ನಿಜ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Jump_ಖಂಡ* nodep) override {
        iterateChildren(nodep);

        // If first statement is an AstLoopTest, pull it before the jump block
        ಆದರೆ (AstLoopTest* ಸ್ಥಿರಾಂಕ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(testp->unlink_Fr_ಹಿಂಭಾಗ());
        }

        // Remove if empty
        ಆದರೆ (!nodep->stmtsp()) {
            UINFO(4, "JUMPLABEL => empty " << nodep);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        // If no JumpGo points to this node, replace it with its body
        ಆದರೆ (!m_ಬಳಸಿದ_Jump_ಖಂಡಗಳು.ಎಣಿಕೆ_2(nodep)) {
            UINFO(4, "JUMPLABEL => unused " << nodep);
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    //-----
    // Below lines are magic expressions processed by astgen
    //  TREE_SKIP_VISIT("AstNODETYPE")    # Rename normal visit to visitGen and don't iterate
    //-----
    // clang-format off
    ವೃಕ್ಷ_SKIP_ಭೇಟಿ("ArraySel");
    ವೃಕ್ಷ_SKIP_ಭೇಟಿ("CAwait");

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
    //ಅಕ್ಷರಸರಣಿಗಳು
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
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        // Default: Just iterate
        ಆದರೆ (m_required) {
            ಆದರೆ (VN_IS(nodep, ಗಂಟು_DType) || VN_IS(nodep, Range) || VN_IS(nodep, ತುಂಡು_Sel)
                || VN_IS(nodep, Dot) || VN_IS(nodep, ಪಠ್ಯ_2)) {
                // ignore
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, ನಮೂನೆ) || VN_IS(nodep, PatMember)) {
                // A parameter override pattern is typed later, so only fold its members
                iterateChildren(nodep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstCellRef* ಸ್ಥಿರಾಂಕ crp = VN_CAST(nodep, CellRef)) {
                iterate(crp->exprp());
                ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = crp->exprp()) {
                    crp->replaceWithKeepDType(newp->unlink_Fr_ಹಿಂಭಾಗ());
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(crp), crp);
                }
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3error("Expecting expression to be constant, but can't convert a "
                               << nodep->prettyTypeName() << " to constant.");
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (nodep->isTimingControl()) m_hasJumpDelay = ನಿಜ;
            // Calculate the width of this operation
            ಆದರೆ (m_params && !nodep->ಅಗಲ()) nodep = V3_ಅಗಲ::ಅಗಲ_Params_Edit(nodep);
            iterateChildren(nodep);
        }
    }

ಸಾರ್ವಜನಿಕ:
    // Processing Mode Enum
    ಎಣಿಕೆ Proc_ಬಹುಲಕ : uint8_t {
        PROC_PARAMS_NOWARN,
        PROC_PARAMS,
        PROC_GENERATE,
        PROC_ಜೀವಂತ,
        PROC_V_WARN,
        PROC_V_NOWARN,
        PROC_V_EXPENSIVE,
        PROC_CPP
    };

    // CONSTRUCTORS
    ConstVisitor(Proc_ಬಹುಲಕ pmode, ತಾರ್ಕಿಕ globalPass)
        : m_globalPass{globalPass}
        , m_concswap_ಹೆಸರುಗಳು{globalPass ? ("__Vconcswap_" + cvt_ಗಮ್ಯ_Str(s_global_Pass_ಅಂಕೆ++)) : ""} {
        // clang-format off
        ಆಯ್ಕೆ (pmode) {
        ಸ್ಥಿತಿ PROC_PARAMS_NOWARN:  m_doV = ನಿಜ;  m_doNConst = ನಿಜ; m_params = ನಿಜ;
                                  m_required = ಸುಳ್ಳು; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ PROC_PARAMS:         m_doV = ನಿಜ;  m_doNConst = ನಿಜ; m_params = ನಿಜ;
                                  m_required = ನಿಜ; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ PROC_GENERATE:       m_doV = ನಿಜ;  m_doNConst = ನಿಜ; m_params = ನಿಜ;
                                  m_required = ನಿಜ; m_doGenerate = ನಿಜ; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ PROC_ಜೀವಂತ:           ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ PROC_V_WARN:         m_doV = ನಿಜ;  m_doNConst = ನಿಜ; m_warn = ನಿಜ; m_convert_Logic_ಗಮ್ಯ_ಬಿಟ್ = ನಿಜ; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ PROC_V_NOWARN:       m_doV = ನಿಜ;  m_doNConst = ನಿಜ; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ PROC_V_EXPENSIVE:    m_doV = ನಿಜ;  m_doNConst = ನಿಜ; m_doExpensive = ನಿಜ; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ PROC_CPP:            m_doV = ಸುಳ್ಳು; m_doNConst = ನಿಜ; m_doCpp = ನಿಜ; ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ:                  v3fatalSrc("Bad case"); ನಿಲ್ಲಿಸು;
        }
        // clang-format on
    }
    ~ConstVisitor() override {
        ಆದರೆ (m_doCpp) {
            ಆದರೆ (m_globalPass) {
                V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Const bit op reduction", m_stat_ಬಿಟ್_Op_Reduction);
            } ಇಲ್ಲದಿದ್ದರೆ {
                V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Optimizations, Const bit op reduction", m_stat_ಬಿಟ್_Op_Reduction);
            }
        }
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Optimizations, Cond redundant expressions", m_stat_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant);
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Optimizations, If cond redundant expressions",
                            m_stat_If_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant);
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Optimizations, Concat merges", m_stat_Concat_ವಿಲೀನ);
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Optimizations, Member access predicate node visits",
                            m_statMemberAccessVisits);
    }

    Ast_ಗಂಟು* mainAcceptEdit(Ast_ಗಂಟು* nodep) {
        VIsCached::clear_Cache_ವೃಕ್ಷ();  // Avoid using any stale isPure
        // Operate starting at a random place
        ಹಿಂತಿರುಗಿ iterateSubtreeReturnEdits(nodep);
    }
};

uint32_t ConstVisitor::s_global_Pass_ಅಂಕೆ = 0;

//######################################################################
// Const class functions

//! Force this cell node's parameter list to become a constant
ಶೂನ್ಯ V3Const::constifyParamsEdit(Ast_ಗಂಟು* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3_ಅಗಲ::ಅಗಲ_Params_Edit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS, /* globalPass: */ ಸುಳ್ಳು};
    // cppcheck-suppress constVariablePointer // edited below
    ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ಆದರೆ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } ಇಲ್ಲದಿದ್ದರೆ {
        (ಶೂನ್ಯ)visitor.mainAcceptEdit(nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump if have next line
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

//! Constify this cell node's parameter list if possible
ಶೂನ್ಯ V3Const::constifyParamsNoWarnEdit(Ast_ಗಂಟು* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3_ಅಗಲ::ಅಗಲ_Params_Edit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS_NOWARN, /* globalPass: */ ಸುಳ್ಳು};
    // cppcheck-suppress constVariablePointer // edited below
    ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ಆದರೆ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } ಇಲ್ಲದಿದ್ದರೆ {
        (ಶೂನ್ಯ)visitor.mainAcceptEdit(nodep);
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
ಶೂನ್ಯ V3Const::constifyGenerateParamsEdit(Ast_ಗಂಟು* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE:");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized
    // yet.

    // Make sure we've sized everything first
    nodep = V3_ಅಗಲ::ಅಗಲ_Generate_Params_Edit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_GENERATE, /* globalPass: */ ಸುಳ್ಳು};
    // cppcheck-suppress constVariablePointer // edited below
    ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ಆದರೆ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } ಇಲ್ಲದಿದ್ದರೆ {
        VL_DO_DANGLING(visitor.mainAcceptEdit(nodep), nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump this.
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

ಶೂನ್ಯ V3Const::constifyAllLint(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_WARN, /* globalPass: */ ನಿಜ};
        (ಶೂನ್ಯ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("const", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3Const::constifyCpp(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ ನಿಜ};
        (ಶೂನ್ಯ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("const_cpp", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

Ast_ಗಂಟು* V3Const::constifyEdit(Ast_ಗಂಟು* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_NOWARN, /* globalPass: */ ಸುಳ್ಳು};
    nodep = visitor.mainAcceptEdit(nodep);
    ಹಿಂತಿರುಗಿ nodep;
}

Ast_ಗಂಟು* V3Const::constifyEditCpp(Ast_ಗಂಟು* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ ಸುಳ್ಳು};
    nodep = visitor.mainAcceptEdit(nodep);
    ಹಿಂತಿರುಗಿ nodep;
}

ಶೂನ್ಯ V3Const::constify_All_ಜೀವಂತ(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    // This only pushes constants up, doesn't make any other edits
    // IE doesn't prune dead statements, as we need to do some usability checks after this
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_ಜೀವಂತ, /* globalPass: */ ನಿಜ};
        (ಶೂನ್ಯ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("const", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3Const::constifyAll(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ ನಿಜ};
        (ಶೂನ್ಯ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("const", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

Ast_ಗಂಟು* V3Const::constifyExpensiveEdit(Ast_ಗಂಟು* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ ಸುಳ್ಳು};
    nodep = visitor.mainAcceptEdit(nodep);
    ಹಿಂತಿರುಗಿ nodep;
}
