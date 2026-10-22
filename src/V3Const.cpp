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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Const.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Simulate.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3String.h"
#अन्तर्भाव "V3UniqueNames.h"
#अन्तर्भाव "V3Width.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <memory>
#अन्तर्भाव <type_traits>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

#परिभाषय वृक्षः_SKIP_दर्शय(...)
#परिभाषय TREEOP1(...)
#परिभाषय TREEOPA(...)
#परिभाषय TREEOP(...)
#परिभाषय TREEOPS(...)
#परिभाषय TREEOPC(...)
#परिभाषय TREEOPV(...)

//######################################################################
// Utilities

स्थिर तार्किक isConst(ध्रुव Ast_ग्रन्थिः* nodep, uint64_t v) {
    ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const);
    प्रत्यागम constp && constp->गन्तव्यम्_UQuad() == v;
}

प्रतिरूपम् <प्रकारनाम T>
स्थिर प्रकारनाम std::enable_if<std::is_integral<T>::मानम्, तार्किक>::type isPow2(T val) {
    प्रत्यागम (val & (val - 1)) == 0;
}

स्थिर पूर्णाङ्क गणनम्_Trailing_Zeroes(uint64_t val) {
    UASSERT(val, "countTrailingZeroes argument must be non-zero");
#यदि defined(__GNUC__) && !defined(VL_NO_BUILTINS)
    प्रत्यागम __builtin_ctzll(val);
#अन्यथा
    पूर्णाङ्क bit = 0;
    val = ~val;
    यावत् (val & 1) {
        ++bit;
        val >>= 1;
    }
    प्रत्यागम bit;
#यदि_अन्तः
}

// Allocates a unique id for each AstNode it is applied to, held in user4.
वर्ग VNIdAllocator final {
    // NODE STATE
    // AstNode::user4p -> size_t. Unique id of the node (0/nullptr if not allocated yet)

    // MEMBERS
    // Claimed lazily on first use TODO: fix conflict with V3Param user4 slot
    std::unique_ptr<VNUser4_अन्तर्_Use> m_inuser4p;
    आकार_t m_अग्र_Id = 0;  // Id allocated most recently

सार्वजनिक:
    // Return the unique id of the given node, allocating a new one if it has none yet
    आकार_t संक्रियकः()(Ast_ग्रन्थिः* nodep) {
        यदि (!m_inuser4p) m_inuser4p = std::make_unique<VNUser4_अन्तर्_Use>();  // Claim on first use
        यदि (!nodep->user4p()) nodep->user4p(reinterpret_cast<शून्य*>(++m_अग्र_Id));
        प्रत्यागम reinterpret_cast<आकार_t>(nodep->user4p());
    }
};

// This visitor can be used in the post-expanded Ast from V3Expand, where the Ast satisfies:
// - Constants are 64 bit at most (because words are accessed via AstWordSel)
// - Variables are scoped.
वर्ग Const_Bit_Op_वृक्षः_Visitor final : सार्वजनिक VNVisitorConst {
    // TYPES

    // Holds a node to be added as a term in the reduction tree, it's equivalent op count, and a
    // bool indicating if the term is clean (0/1 value, or if the top bits might be dirty)
    उपयुज्य परिणाम_पदांशः = std::tuple<Ast_ग्रन्थिः_व्यञ्जकम्*, अचिह्नित, तार्किक>;

    वर्ग पर्णम्_Info final {  // Leaf node (either AstConst or AstVarRef)
        // MEMBERS
        तार्किक m_polarity = सत्य;  // Invert result due to NOT
        पूर्णाङ्क m_lsb = 0;  // LSB of actually used bit of m_refp->varp()
        पूर्णाङ्क m_msb = 0;  // MSB of actually used bit of m_refp->varp()
        पूर्णाङ्क m_wordIdx = -1;  // -1 means AstWordSel is not used.
        AstVarRef* m_refp = शून्यसूचकः;  // Leaf's variable reference
        ध्रुव AstConst* m_constp = शून्यसूचकः;  // Leaf's constant

    सार्वजनिक:
        // CONSTRUCTORS
        पर्णम्_Info() = स्वभाव;
        पर्णम्_Info(ध्रुव पर्णम्_Info& other) = स्वभाव;
        पर्णम्_Info& संक्रियकः=(ध्रुव पर्णम्_Info& other) = स्वभाव;
        पर्णम्_Info(पर्णम्_Info&& other) = स्वभाव;
        पर्णम्_Info& संक्रियकः=(पर्णम्_Info&& other) = स्वभाव;
        स्पष्ट पर्णम्_Info(पूर्णाङ्क lsb)
            : m_lsb{lsb} {}

        // METHODS
        शून्य समुच्चयः_पर्णम्(AstVarRef* refp) {
            UASSERT_OBJ(!m_refp && !m_constp, refp, "Must be called just once");
            m_refp = refp;
            m_msb = refp->varp()->विस्तारः_Min() - 1;
        }
        शून्य समुच्चयः_पर्णम्(ध्रुव AstConst* constp) {
            UASSERT_OBJ(!m_refp && !m_constp, constp, "Must be called just once");
            m_constp = constp;
            m_msb = constp->विस्तारः_Min() - 1;
        }
        // updateBitRange(), limitBitRangeToLsb(), and polarity() must be called during ascending
        // back to the root.
        शून्य updateBitRange(पूर्णाङ्क newLsb, पूर्णाङ्क newMsb) {
            यदि ((m_lsb <= m_msb && newLsb > newMsb) || (m_lsb > m_msb && m_lsb < newLsb)) {
                // When the new bit range is out of m_refp, clear polarity because nodes below is
                // shifted out to zero.
                // This kind of clear may happen several times. e.g. (!(1'b1 >> 1)) >> 1
                polarity(सत्य);
            }
            m_lsb = newLsb;
            m_msb = newMsb;
        }
        शून्य updateBitRange(ध्रुव AstCCast* castp) {
            updateBitRange(m_lsb, std::min(m_msb, m_lsb + castp->विस्तारः() - 1));
        }
        शून्य updateBitRange(ध्रुव Ast_सरणम्_R* shiftp) {
            updateBitRange(m_lsb + VN_AS(shiftp->rhsp(), Const)->गन्तव्यम्_UInt(), m_msb);
        }
        शून्य सीमा_Bit_Range_गन्तव्यम्_Lsb() { updateBitRange(m_lsb, std::min(m_msb, m_lsb)); }
        पूर्णाङ्क wordIdx() ध्रुव { प्रत्यागम m_wordIdx; }
        शून्य wordIdx(पूर्णाङ्क i) { m_wordIdx = i; }
        तार्किक polarity() ध्रुव { प्रत्यागम m_polarity; }
        शून्य polarity(तार्किक p) { m_polarity = p; }

        AstVarRef* refp() ध्रुव { प्रत्यागम m_refp; }
        ध्रुव AstConst* constp() ध्रुव { प्रत्यागम m_constp; }
        तार्किक missingWordSel() ध्रुव {
            // When V3Expand is skipped, WordSel is not inserted.
            प्रत्यागम m_refp->isWide() && m_wordIdx == -1;
        }
        पूर्णाङ्क lsb() ध्रुव { प्रत्यागम m_lsb; }

        पूर्णाङ्क msb() ध्रुव { प्रत्यागम std::min(m_msb, var_विस्तारः() - 1); }
        पूर्णाङ्क var_विस्तारः() ध्रुव {
            UASSERT(m_refp, "m_refp should be set");
            ध्रुव पूर्णाङ्क विस्तारः = m_refp->varp()->विस्तारः_Min();
            यदि (!m_refp->isWide()) {
                UASSERT_OBJ(m_wordIdx == -1, m_refp, "Bad word index into non-wide");
                प्रत्यागम विस्तारः;
            } अन्यथा {
                यदि (missingWordSel()) प्रत्यागम विस्तारः;
                UASSERT_OBJ(m_wordIdx >= 0, m_refp, "Bad word index into wide");
                ध्रुव पूर्णाङ्क द्वयङ्काः_अन्तर्_MSW = VL_BITBIT_E(विस्तारः) ? VL_BITBIT_E(विस्तारः) : VL_EDATASIZE;
                प्रत्यागम m_wordIdx == m_refp->विस्तारः_शब्दाः() - 1 ? द्वयङ्काः_अन्तर्_MSW : VL_EDATASIZE;
            }
        }
    };

    संरचना Bit_Polarity_प्रविष्टिः final {  // Found bit polarity during iterate()
        पर्णम्_Info m_info;  // Leaf (variable or constant) bit polarity was found on
        तार्किक m_polarity = असत्य;  // Polarity the bit must have to match
        पूर्णाङ्क m_bit = 0;  // Bit index within the leaf that was tested
        Bit_Polarity_प्रविष्टिः(ध्रुव पर्णम्_Info& info, तार्किक pol, पूर्णाङ्क bit)
            : m_info{info}
            , m_polarity{pol}
            , m_bit{bit} {}
        Bit_Polarity_प्रविष्टिः() = स्वभाव;
    };

    संरचना Frozen_ग्रन्थिः_Info final {  // Context when a frozen node is found
        तार्किक m_polarity;  // Polarity the frozen node must match
        पूर्णाङ्क m_lsb;  // LSB position of the frozen node
        तार्किक संक्रियकः<(ध्रुव Frozen_ग्रन्थिः_Info& other) ध्रुव {
            यदि (m_lsb != other.m_lsb) प्रत्यागम m_lsb < other.m_lsb;
            प्रत्यागम m_polarity < other.m_polarity;
        }
    };

    वर्ग Restorer final {  // Restore the original state unless disableRestore() is called
        Const_Bit_Op_वृक्षः_Visitor& m_visitor;  // Visitor whose state is saved and restored
        ध्रुव आकार_t m_polarities_आकार;  // Saved m_visitor.m_bitPolarities size to truncate back to
        ध्रुव आकार_t m_frozen_आकार;  // Saved m_visitor.m_frozenNodes size to truncate back to
        ध्रुव अचिह्नित m_ops;  // Saved m_visitor.m_ops to restore
        ध्रुव तार्किक m_polarity;  // Saved m_visitor.m_polarity to restore
        तार्किक m_restore = सत्य;  // Whether the destructor still needs to restore state

    सार्वजनिक:
        स्पष्ट Restorer(Const_Bit_Op_वृक्षः_Visitor& visitor)
            : m_visitor{visitor}
            , m_polarities_आकार{visitor.m_bitPolarities.आकार()}
            , m_frozen_आकार{visitor.m_frozenNodes.आकार()}
            , m_ops{visitor.m_ops}
            , m_polarity{visitor.m_polarity} {}
        ~Restorer() {
            UASSERT(m_visitor.m_bitPolarities.आकार() >= m_polarities_आकार,
                    "m_bitPolarities must grow monotonically");
            UASSERT(m_visitor.m_frozenNodes.आकार() >= m_frozen_आकार,
                    "m_frozenNodes must grow monotonically");
            यदि (m_restore) restoreNow();
        }
        शून्य disableRestore() { m_restore = असत्य; }
        शून्य restoreNow() {
            UASSERT(m_restore, "Can be called just once");
            m_visitor.m_bitPolarities.resize(m_polarities_आकार);
            m_visitor.m_frozenNodes.resize(m_frozen_आकार);
            m_visitor.m_ops = m_ops;
            m_visitor.m_polarity = m_polarity;
            m_restore = असत्य;
        }
    };
    // Collect information for each Variable to transform as below
    वर्ग VarInfo final {
        // MEMBERS
        पूर्णाङ्क m_ज्ञातम्_परिणाम = -1;  // -1: result is not known, 0 or 1: result of this tree
        ध्रुव Const_Bit_Op_वृक्षः_Visitor* ध्रुव
            m_parentp;  // ConstBitOpTreeVisitor holding this VarInfo
        AstVarRef* ध्रुव m_refp;  // Points the variable that this VarInfo covers
        ध्रुव पूर्णाङ्क m_विस्तारः;  // Width of term this VarInfo refers to
        V3_सङ्ख्या m_bitPolarity;  // Coefficient of each bit

    सार्वजनिक:
        // METHODS
        तार्किक has_Const_परिणाम() ध्रुव { प्रत्यागम m_ज्ञातम्_परिणाम >= 0 || m_bitPolarity.isAllX(); }
        // The constant result. Only valid if hasConstResult() returned true.
        तार्किक get_Const_परिणाम() ध्रुव {
            // Note that this condition covers m_knownResult == -1 but m_bitPolarity.isAllX(),
            // in which case the result is 0
            प्रत्यागम m_ज्ञातम्_परिणाम == 1;
        }
        ध्रुव AstVarRef* refp() ध्रुव { प्रत्यागम m_refp; }
        तार्किक sameVarAs(ध्रुव Ast_ग्रन्थिः_Var_Ref* otherp) ध्रुव { प्रत्यागम m_refp->same_ग्रन्थिः(otherp); }
        शून्य समुच्चयः_Polarity(तार्किक compBit, पूर्णाङ्क bit) {
            // Ignore if already determined a known reduction
            यदि (m_ज्ञातम्_परिणाम >= 0) प्रत्यागम;
            UASSERT_OBJ(bit < m_विस्तारः, m_refp,
                        "Bit index out of range: " << bit << " width: " << m_विस्तारः);
            यदि (m_bitPolarity.bitIsX(bit)) {  // The bit is not yet marked with either polarity
                m_bitPolarity.समुच्चयः_Bit(bit, compBit);
            } अन्यथा {  // The bit has already been marked with some polarity
                ध्रुव तार्किक sameFlag = m_bitPolarity.bitIs1(bit) == compBit;
                यदि (m_parentp->is_Xor_वृक्षः()) {
                    UASSERT_OBJ(compBit && sameFlag, m_refp, "Only true is set in Xor tree");
                    // a ^ a ^ b == b so we can ignore a
                    m_bitPolarity.समुच्चयः_Bit(bit, 'x');
                } अन्यथा {  // And, Or
                    // Can ignore this nodep as the bit is already marked with the same polarity
                    यदि (sameFlag) प्रत्यागम;  // a & a == a, b | b == b
                    // Otherwise result is constant (a & ~a == 0) or (a | ~a == 1)
                    m_ज्ञातम्_परिणाम = m_parentp->is_And_वृक्षः() ? 0 : 1;
                    m_bitPolarity.समुच्चयः_All_द्वयङ्काः_X();  // The variable is not referred anymore
                }
            }
        }

        // Return reduction term for this VarInfo, together with the number of ops in the term,
        // and a boolean indicating if the term is clean (1-bit vs multi-bit value)
        परिणाम_पदांशः get_परिणाम_पदांशः() ध्रुव {
            UASSERT_OBJ(!has_Const_परिणाम(), m_refp, "getTerm on reduction that yields constant");
            File_पङ्क्तिः* ध्रुव fl = m_refp->fileline();

            // Get the term we are referencing (the WordSel, if wide, otherwise just the VarRef)
            Ast_ग्रन्थिः_व्यञ्जकम्* srcp = VN_CAST(m_refp->backp(), WordSel);
            यदि (!srcp) srcp = m_refp;
            srcp = srcp->clone_वृक्षः(असत्य);

            // Signed variables might have redundant sign bits that need masking.
            ध्रुव तार्किक has_Redundant_Sign_द्वयङ्काः
                = m_refp->varp()->dtypep()->isSigned()
                  && (m_refp->isWide() ? (m_विस्तारः != VL_EDATASIZE)
                                       : (m_विस्तारः < 8 || !isPow2(m_विस्तारः)));

            // Get the mask that selects the bits that are relevant in this term
            V3_सङ्ख्या आवरणम्_अङ्कः{srcp, m_विस्तारः, 0};
            आवरणम्_अङ्कः.op_द्वयङ्काः_Non_XZ(m_bitPolarity);  // 'x' -> 0, 0->1, 1->1
            ध्रुव uint64_t आवरणम्_Val = आवरणम्_अङ्कः.गन्तव्यम्_UQuad();
            UASSERT_OBJ(आवरणम्_Val != 0, m_refp,
                        "Should have been recognized as having const 0 result");

            // Parts of the return value
            Ast_ग्रन्थिः_व्यञ्जकम्* resultp = srcp;  // The tree for this term
            अचिह्नित ops = 0;  // Number of ops in this term
            तार्किक clean = असत्य;  // Whether the term is clean (has value 0 or 1)

            यदि (isPow2(आवरणम्_Val)) {
                // If we only want a single bit, shift it out instead of a masked compare. Shifts
                // don't go through the flags register on x86 and are hence faster. This is also
                // always fewer or same ops as mask and compare, but with shorter instructions on
                // x86.

                // Find the index of the bit we want.
                ध्रुव पूर्णाङ्क bit = गणनम्_Trailing_Zeroes(आवरणम्_Val);
                // If we want something other than the bottom bit, shift it out
                यदि (bit != 0) {
                    resultp = नूतन Ast_सरणम्_R{fl, resultp,
                                            नूतन AstConst{fl, static_cast<uint32_t>(bit)}, m_विस्तारः};
                    ++ops;
                }
                // Negate it if necessary
                ध्रुव तार्किक negate = m_bitPolarity.bitIs0(bit);
                यदि (negate) {
                    resultp = नूतन AstNot{fl, resultp};
                    ++ops;
                }
                // Clean if MSB of unsigned value, and not negated
                clean = (bit == m_विस्तारः - 1) && !has_Redundant_Sign_द्वयङ्काः && !negate;
            } अन्यथा {
                // We want multiple bits. Go ahead and extract them.

                // Check if masking is required, and if so apply it
                ध्रुव तार्किक needsMasking = आवरणम्_Val != VL_आवरणम्_Q(m_विस्तारः) || has_Redundant_Sign_द्वयङ्काः;
                यदि (needsMasking) {
                    resultp = नूतन AstAnd{fl, नूतन AstConst{fl, आवरणम्_अङ्कः}, resultp};
                    ++ops;
                }

                // Create the sub-expression for this term
                यदि (m_parentp->is_Xor_वृक्षः()) {
                    यदि (needsMasking) {
                        // Reduce the masked term to the minimum known width,
                        // to use the smallest RedXor formula
                        ध्रुव पूर्णाङ्क विस्तारः_Min = आवरणम्_अङ्कः.विस्तारः_गन्तव्यम्_Fit();
                        resultp->dtype_Chg_विस्तारः(विस्तारः_Min, विस्तारः_Min);
                    }
                    resultp = नूतन AstRedXor{fl, resultp};
                    ++ops;
                    clean = असत्य;
                    // VL_REDXOR_* returns IData, set width accordingly to avoid unnecessary casts
                    resultp->dtype_Chg_विस्तारः(VL_IDATASIZE, 1);
                } अन्यथा यदि (m_parentp->is_And_वृक्षः()) {
                    V3_सङ्ख्या comp_अङ्कः{srcp, m_विस्तारः, 0};
                    comp_अङ्कः.op_द्वयङ्काः_One(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    resultp = नूतन AstEq{fl, नूतन AstConst{fl, comp_अङ्कः}, resultp};
                    ++ops;
                    clean = सत्य;
                } अन्यथा {  // Or
                    V3_सङ्ख्या comp_अङ्कः{srcp, m_विस्तारः, 0};
                    comp_अङ्कः.op_द्वयङ्काः_One(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    comp_अङ्कः.opXor(V3_सङ्ख्या{comp_अङ्कः}, आवरणम्_अङ्कः);
                    resultp = नूतन AstNeq{fl, नूतन AstConst{fl, comp_अङ्कः}, resultp};
                    ++ops;
                    clean = सत्य;
                }
            }

            प्रत्यागम परिणाम_पदांशः{resultp, ops, clean};
        }

        // CONSTRUCTORS
        VarInfo(Const_Bit_Op_वृक्षः_Visitor* parent, AstVarRef* refp, पूर्णाङ्क विस्तारः)
            : m_parentp{parent}
            , m_refp{refp}
            , m_विस्तारः{विस्तारः}
            , m_bitPolarity{refp, m_विस्तारः} {
            m_bitPolarity.समुच्चयः_All_द्वयङ्काः_X();
        }
    };

    // MEMBERS
    तार्किक m_failed = असत्य;
    तार्किक m_polarity = सत्य;  // Flip when AstNot comes
    अचिह्नित m_ops;  // Number of operations such as And, Or, Xor, Sel...
    पूर्णाङ्क m_lsb = 0;  // Current LSB
    पर्णम्_Info* m_leafp = शून्यसूचकः;  // AstConst or AstVarRef that currently looking for
    ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव m_rootp;  // Root of this AST subtree

    std::सदिशम्<std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, Frozen_ग्रन्थिः_Info>>
        m_frozenNodes;  // Nodes that cannot be optimized
    std::सदिशम्<Bit_Polarity_प्रविष्टिः> m_bitPolarities;  // Polarity of bits found during iterate()
    std::सदिशम्<std::unique_ptr<VarInfo>> m_varInfos;  // VarInfo for each variable, [0] is nullptr
    VNIdAllocator& m_ids;  // Node id allocator, owned by ConstVisitor
    // Base index of m_varInfos that points VarInfo, keyed by AstVarScope/AstVarRef id,
    // zero means not set yet.
    std::unordered_map<आकार_t, पूर्णाङ्क> m_आधारः_Idxs;

    // METHODS

    तार्किक is_And_वृक्षः() ध्रुव { प्रत्यागम VN_IS(m_rootp, And); }
    तार्किक is_Or_वृक्षः() ध्रुव { प्रत्यागम VN_IS(m_rootp, Or); }
    तार्किक is_Xor_वृक्षः() ध्रुव { प्रत्यागम VN_IS(m_rootp, Xor) || VN_IS(m_rootp, RedXor); }

#परिभाषय CONST_BITOP_RETURN_IF(cond, nodep) \
    यदि (समुच्चयः_Failed(cond, #cond, nodep, __LINE__)) प्रत्यागम

#परिभाषय CONST_BITOP_समुच्चयः_FAILED(reason, nodep) समुच्चयः_Failed(सत्य, reason, nodep, __LINE__)

    तार्किक समुच्चयः_Failed(तार्किक fail, ध्रुव वर्ण* reason, ध्रुव Ast_ग्रन्थिः* nodep, पूर्णाङ्क पङ्क्तिः) {
        यदि (fail && !m_failed) {
            UINFO(9, "cannot optimize " << m_rootp << " reason:" << reason << " called from line:"
                                        << पङ्क्तिः << " when checking:" << nodep);
            // UINFOTREE(9, m_rootp, "", "root");
            m_failed = सत्य;
        }
        प्रत्यागम m_failed;
    }
    शून्य incrOps(ध्रुव Ast_ग्रन्थिः* nodep, पूर्णाङ्क पङ्क्तिः) {
        ++m_ops;
        UINFO(9, "Increment to " << m_ops << " " << nodep << " called from line " << पङ्क्तिः);
    }
    VarInfo& getVarInfo(ध्रुव पर्णम्_Info& ref) {
        UASSERT_OBJ(ref.refp(), m_rootp, "null varref in And/Or/Xor optimization");
        Ast_ग्रन्थिः* nodep = ref.refp()->varScopep();
        यदि (!nodep) nodep = ref.refp()->varp();  // Not scoped
        पूर्णाङ्क& आधारः_Idxr = m_आधारः_Idxs[m_ids(nodep)];
        यदि (आधारः_Idxr == 0) {  // Not set yet
            आधारः_Idxr = m_varInfos.आकार();
            ध्रुव पूर्णाङ्क अङ्कः_शब्दाः
                = ref.refp()->dtypep()->isWide() ? ref.refp()->dtypep()->विस्तारः_शब्दाः() : 1;
            m_varInfos.resize(m_varInfos.आकार() + अङ्कः_शब्दाः);
        }
        ध्रुव आकार_t idx = आधारः_Idxr + std::max(0, ref.wordIdx());
        VarInfo* varInfop = m_varInfos[idx].get();
        यदि (!varInfop) {
            varInfop = नूतन VarInfo{एतत्, ref.refp(), ref.var_विस्तारः()};
            m_varInfos[idx].reset(varInfop);
            यदि (ref.missingWordSel()) {
                // ConstBitOpTreeVisitor makes some constants for masks and its type is uint64_t.
                // That's why V3Expand, that inserts WordSel, is needed.
                CONST_BITOP_समुच्चयः_FAILED("V3Expand is skipped", ref.refp());
            }
        } अन्यथा {
            यदि (!varInfop->sameVarAs(ref.refp()))
                CONST_BITOP_समुच्चयः_FAILED("different var (scope?)", ref.refp());
        }
        प्रत्यागम *varInfop;
    }

    // Traverse down to see AstConst or AstVarRef
    पर्णम्_Info find_पर्णम्(Ast_ग्रन्थिः* nodep, तार्किक expectConst) {
        यदि (!nodep->dtypep()->skipRefp()->isIntegralOrPacked()) प्रत्यागम पर्णम्_Info{};
        पर्णम्_Info info{m_lsb};
        {
            VL_RESTORER(m_leafp);
            m_leafp = &info;
            iterateConst(nodep);
        }

        तार्किक ok = !m_failed;
        यदि (expectConst) {
            ok &= !info.refp() && info.constp();
        } अन्यथा {
            ok &= info.refp() && !info.constp();
        }
        प्रत्यागम ok ? info : पर्णम्_Info{};
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { CONST_BITOP_समुच्चयः_FAILED("Hit unexpected op", nodep); }
    शून्य दर्शय(AstCCast* nodep) override {
        iterateChildrenConst(nodep);
        यदि (m_leafp) m_leafp->updateBitRange(nodep);
    }
    शून्य दर्शय(Ast_सरणम्_R* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->rhsp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->rhsp());
        m_lsb += constp->गन्तव्यम्_UInt();
        incrOps(nodep, __LINE__);
        iterateConst(nodep->lhsp());
        m_leafp->updateBitRange(nodep);
        m_lsb -= constp->गन्तव्यम्_UInt();
    }
    शून्य दर्शय(AstNot* nodep) override {
        CONST_BITOP_RETURN_IF(nodep->विस्तारः_Min() != 1, nodep);
        Ast_ग्रन्थिः* lhsp = nodep->lhsp();
        AstCCast* ध्रुव castp = VN_CAST(lhsp, CCast);
        यदि (castp) lhsp = castp->lhsp();
        CONST_BITOP_RETURN_IF(!is_Xor_वृक्षः() && !VN_IS(lhsp, VarRef) && !VN_IS(lhsp, सरणम्_R), lhsp);
        incrOps(nodep, __LINE__);
        m_polarity = !m_polarity;
        iterateChildrenConst(nodep);
        // Don't restore m_polarity for Xor as it counts parity of the entire tree
        यदि (!is_Xor_वृक्षः()) m_polarity = !m_polarity;
        यदि (m_leafp && castp) m_leafp->updateBitRange(castp);
        यदि (m_leafp) m_leafp->polarity(!m_leafp->polarity());
    }
    शून्य दर्शय(AstWordSel* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->bitp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->bitp());
        UASSERT_OBJ(m_leafp->wordIdx() == -1, nodep, "Unexpected nested WordSel");
        m_leafp->wordIdx(constp->गन्तव्यम्_SInt());
        iterateConst(nodep->fromp());
    }
    शून्य दर्शय(AstVarRef* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->समुच्चयः_पर्णम्(nodep);
    }
    शून्य दर्शय(AstConst* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->समुच्चयः_पर्णम्(nodep);
    }

    शून्य दर्शय(AstRedXor* nodep) override {
        Restorer restorer{*एतत्};
        CONST_BITOP_RETURN_IF(!VN_IS(m_rootp, Xor), nodep);
        Ast_ग्रन्थिः* lhsp = nodep->lhsp();
        ध्रुव AstCCast* ध्रुव castp = VN_CAST(lhsp, CCast);
        यदि (castp) lhsp = castp->lhsp();
        यदि (ध्रुव AstAnd* ध्रुव andp = VN_CAST(lhsp, And)) {  // '^(mask & leaf)'
            CONST_BITOP_RETURN_IF(!andp, lhsp);

            ध्रुव पर्णम्_Info& आवरणम् = find_पर्णम्(andp->lhsp(), सत्य);
            CONST_BITOP_RETURN_IF(!आवरणम्.constp() || आवरणम्.lsb() != 0, andp->lhsp());

            पर्णम्_Info ref = find_पर्णम्(andp->rhsp(), असत्य);
            CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());
            यदि (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all subtree succeeded

            ध्रुव V3_सङ्ख्या& आवरणम्_अङ्कः = आवरणम्.constp()->अङ्कः();

            incrOps(nodep, __LINE__);
            incrOps(andp, __LINE__);

            // Mark all bits checked in this reduction
            ध्रुव पूर्णाङ्क maxBitIdx = std::min(ref.lsb() + आवरणम्_अङ्कः.विस्तारः(), ref.msb() + 1);
            प्रति (पूर्णाङ्क bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                ध्रुव पूर्णाङ्क आवरणम्_Idx = bitIdx - ref.lsb();
                यदि (आवरणम्_अङ्कः.bitIs0(आवरणम्_Idx)) अनुवर्त;
                // Set true, m_polarity takes care of the entire parity
                m_bitPolarities.emplace_पृष्ठभागः(ref, सत्य, bitIdx);
            }
        } अन्यथा {  // '^leaf'
            पर्णम्_Info ref = find_पर्णम्(lhsp, असत्य);
            CONST_BITOP_RETURN_IF(!ref.refp(), lhsp);
            यदि (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all checks passed

            incrOps(nodep, __LINE__);

            // Mark all bits checked by this comparison
            प्रति (पूर्णाङ्क bitIdx = ref.lsb(); bitIdx <= ref.msb(); ++bitIdx) {
                m_bitPolarities.emplace_पृष्ठभागः(ref, सत्य, bitIdx);
            }
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः_Biop* nodep) override {
        यदि (VN_IS(nodep, And) && isConst(nodep->lhsp(), 1)) {  // 1 & _
            // Always reach past a plain making AND
            Restorer restorer{*एतत्};
            incrOps(nodep, __LINE__);
            iterateConst(nodep->rhsp());
            यदि (m_leafp) m_leafp->सीमा_Bit_Range_गन्तव्यम्_Lsb();
            CONST_BITOP_RETURN_IF(m_failed, nodep->rhsp());
            restorer.disableRestore();  // Now all checks passed
        } अन्यथा यदि (nodep->type() == m_rootp->type()) {  // And, Or, Xor
            // subtree under NOT can be optimized only in XOR tree.
            CONST_BITOP_RETURN_IF(!m_polarity && !is_Xor_वृक्षः(), nodep);
            incrOps(nodep, __LINE__);
            प्रति (ध्रुव तार्किक दक्षिणतः : {असत्य, सत्य}) {
                VL_RESTORER(m_leafp);
                Restorer restorer{*एतत्};
                पर्णम्_Info पर्णम्_Info_2{m_lsb};
                // cppcheck-suppress danglingLifetime
                m_leafp = &पर्णम्_Info_2;
                Ast_ग्रन्थिः_व्यञ्जकम्* opp = दक्षिणतः ? nodep->rhsp() : nodep->lhsp();
                ध्रुव तार्किक origFailed = m_failed;
                iterateConst(opp);
                यदि (पर्णम्_Info_2.constp() || m_failed) {
                    // Revert changes in leaf
                    restorer.restoreNow();
                    // Reach past a cast then add to frozen nodes to be added to final reduction
                    यदि (ध्रुव AstCCast* ध्रुव castp = VN_CAST(opp, CCast)) opp = castp->lhsp();
                    ध्रुव तार्किक pol = is_Xor_वृक्षः() || m_polarity;  // Only AND/OR tree needs polarity
                    UASSERT_OBJ(pol, nodep, "AND/OR tree expects m_polarity==true");
                    m_frozenNodes.emplace_पृष्ठभागः(opp, Frozen_ग्रन्थिः_Info{pol, m_lsb});
                    m_failed = origFailed;
                    अनुवर्त;
                }
                restorer.disableRestore();  // Now all checks passed
                यदि (पर्णम्_Info_2.refp()) {
                    // The conditional on the lsb being in range is necessary for some degenerate
                    // case, e.g.: (IData)((QData)wide[0] >> 32), or <1-bit-var> >> 1, which is
                    // just zero
                    यदि (पर्णम्_Info_2.lsb() <= पर्णम्_Info_2.msb()) {
                        m_bitPolarities.emplace_पृष्ठभागः(पर्णम्_Info_2, is_Xor_वृक्षः() || पर्णम्_Info_2.polarity(),
                                                     पर्णम्_Info_2.lsb());
                    } अन्यथा यदि ((is_And_वृक्षः() && पर्णम्_Info_2.polarity())
                               || (is_Or_वृक्षः() && !पर्णम्_Info_2.polarity())) {
                        // If there is a constant 0 term in an And tree or 1 term in an Or tree, we
                        // must include it. Fudge this by adding a bit with both polarities, which
                        // will simplify to zero or one respectively.
                        // Note that Xor tree does not need this kind of care, polarity of Xor tree
                        // is already cared when visitin AstNot. Taking xor with 1'b0 is nop.
                        m_bitPolarities.emplace_पृष्ठभागः(पर्णम्_Info_2, सत्य, 0);
                        m_bitPolarities.emplace_पृष्ठभागः(पर्णम्_Info_2, असत्य, 0);
                    }
                }
            }
        } अन्यथा यदि ((is_And_वृक्षः() && VN_IS(nodep, Eq)) || (is_Or_वृक्षः() && VN_IS(nodep, Neq))) {
            Restorer restorer{*एतत्};
            CONST_BITOP_RETURN_IF(!m_polarity, nodep);
            CONST_BITOP_RETURN_IF(m_lsb, nodep);  // the result of EQ/NE is 1 bit width
            ध्रुव Ast_ग्रन्थिः* lhsp = nodep->lhsp();
            यदि (ध्रुव AstCCast* ध्रुव castp = VN_CAST(lhsp, CCast)) lhsp = castp->lhsp();
            ध्रुव AstConst* ध्रुव constp = VN_CAST(lhsp, Const);
            CONST_BITOP_RETURN_IF(!constp, nodep->lhsp());

            ध्रुव V3_सङ्ख्या& comp_अङ्कः = constp->अङ्कः();

            स्वचालित समुच्चयः_Polarities = [एतत्, &comp_अङ्कः](ध्रुव पर्णम्_Info& ref, ध्रुव V3_सङ्ख्या* maskp) {
                ध्रुव तार्किक आवरणम्_Flip = is_And_वृक्षः() ^ ref.polarity();
                पूर्णाङ्क constant_विस्तारः = comp_अङ्कः.विस्तारः();
                यदि (maskp) constant_विस्तारः = std::max(constant_विस्तारः, maskp->विस्तारः());
                ध्रुव पूर्णाङ्क maxBitIdx = std::max(ref.lsb() + constant_विस्तारः, ref.msb() + 1);
                // Mark all bits checked by this comparison
                प्रति (पूर्णाङ्क bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                    ध्रुव पूर्णाङ्क आवरणम्_Idx = bitIdx - ref.lsb();
                    ध्रुव तार्किक mask0 = maskp && maskp->bitIs0(आवरणम्_Idx);
                    ध्रुव तार्किक बहिर्गमनम्_सम्बन्धि_Range = bitIdx > ref.msb();
                    यदि (mask0 || बहिर्गमनम्_सम्बन्धि_Range) {  // RHS is 0
                        यदि (comp_अङ्कः.bitIs1(आवरणम्_Idx)) {
                            // LHS is 1
                            // And tree: 1 == 0 => always false, set v && !v
                            // Or tree : 1 != 0 => always true, set v || !v
                            m_bitPolarities.emplace_पृष्ठभागः(ref, सत्य, 0);
                            m_bitPolarities.emplace_पृष्ठभागः(ref, असत्य, 0);
                            भङ्ग;
                        } अन्यथा {  // This bitIdx is irrelevant
                            अनुवर्त;
                        }
                    }
                    ध्रुव तार्किक polarity = comp_अङ्कः.bitIs1(आवरणम्_Idx) != आवरणम्_Flip;
                    m_bitPolarities.emplace_पृष्ठभागः(ref, polarity, bitIdx);
                }
            };

            यदि (ध्रुव AstAnd* ध्रुव andp = VN_CAST(nodep->rhsp(), And)) {  // comp == (mask & v)
                ध्रुव पर्णम्_Info& आवरणम् = find_पर्णम्(andp->lhsp(), सत्य);
                CONST_BITOP_RETURN_IF(!आवरणम्.constp() || आवरणम्.lsb() != 0, andp->lhsp());

                ध्रुव पर्णम्_Info& ref = find_पर्णम्(andp->rhsp(), असत्य);
                CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());

                restorer.disableRestore();  // Now all checks passed

                ध्रुव V3_सङ्ख्या& आवरणम्_अङ्कः = आवरणम्.constp()->अङ्कः();

                incrOps(nodep, __LINE__);
                incrOps(andp, __LINE__);

                समुच्चयः_Polarities(ref, &आवरणम्_अङ्कः);
            } अन्यथा {  // comp == v
                ध्रुव पर्णम्_Info& ref = find_पर्णम्(nodep->rhsp(), असत्य);
                CONST_BITOP_RETURN_IF(!ref.refp(), nodep->rhsp());

                restorer.disableRestore();  // Now all checks passed

                incrOps(nodep, __LINE__);

                समुच्चयः_Polarities(ref, शून्यसूचकः);
            }
        } अन्यथा {
            CONST_BITOP_समुच्चयः_FAILED("Mixture of different ops cannot be optimized", nodep);
        }
    }

    // CONSTRUCTORS
    Const_Bit_Op_वृक्षः_Visitor(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, अचिह्नित externalOps, VNIdAllocator& ids)
        : m_ops{externalOps}
        , m_rootp{nodep}
        , m_ids{ids} {
        // Fill nullptr at [0] because a base index of 0 means not set yet
        m_varInfos.प्रक्षिप_पृष्ठभागः(शून्यसूचकः);
        CONST_BITOP_RETURN_IF(!is_And_वृक्षः() && !is_Or_वृक्षः() && !is_Xor_वृक्षः(), nodep);
        यदि (Ast_ग्रन्थिः_Biop* ध्रुव biopp = VN_CAST(nodep, ग्रन्थिः_Biop)) {
            iterateConst(biopp);
        } अन्यथा {
            UASSERT_OBJ(VN_IS(nodep, RedXor), nodep, "Must be RedXor");
            incrOps(nodep, __LINE__);
            iterateChildrenConst(nodep);
        }
        प्रति (स्वचालित&& प्रविष्टिः : m_bitPolarities) {
            getVarInfo(प्रविष्टिः.m_info).समुच्चयः_Polarity(प्रविष्टिः.m_polarity, प्रविष्टिः.m_bit);
        }
        UASSERT_OBJ(is_Xor_वृक्षः() || m_polarity, nodep, "must be the original polarity");
    }
    आभास ~Const_Bit_Op_वृक्षः_Visitor() = स्वभाव;
#अपरिभाषय CONST_BITOP_RETURN_IF
#अपरिभाषय CONST_BITOP_समुच्चयः_FAILED

सार्वजनिक:
    // Transform as below.
    // v[0] & v[1] => 2'b11 == (2'b11 & v)
    // v[0] | v[1] => 2'b00 != (2'b11 & v)
    // v[0] ^ v[1] => ^{2'b11 & v}
    // (3'b011 == (3'b011 & v)) & v[2]  => 3'b111 == (3'b111 & v)
    // (3'b000 != (3'b011 & v)) | v[2]  => 3'b000 != (3'b111 & v)
    // Reduction ops are transformed in the same way.
    // &{v[0], v[1]} => 2'b11 == (2'b11 & v)
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* simplify(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, पूर्णाङ्क परिणाम_विस्तारः, अचिह्नित externalOps,
                                 VDouble0& reduction, VNIdAllocator& ids) {
        UASSERT_OBJ(1 <= परिणाम_विस्तारः && परिणाम_विस्तारः <= 64, nodep, "resultWidth out of range");

        // Walk tree, gathering all terms referenced in expression
        ध्रुव Const_Bit_Op_वृक्षः_Visitor visitor{nodep, externalOps, ids};

        // If failed on root node is not optimizable, or there are no variable terms, then done
        यदि (visitor.m_failed || visitor.m_varInfos.आकार() == 1) प्रत्यागम शून्यसूचकः;

        // FileLine used for constructing all new nodes in this function
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();

        // Get partial result each term referenced, count total number of ops and keep track of
        // whether we have clean/dirty terms. visitor.m_varInfos appears in deterministic order,
        // so the optimized tree is deterministic as well.

        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> termps;
        termps.reserve(visitor.m_varInfos.आकार() - 1);
        अचिह्नित परिणाम_Ops = 0;
        तार्किक has_Clean_पदांशः = असत्य;
        तार्किक has_Dirty_पदांशः = असत्य;

        प्रति (स्वचालित&& v : visitor.m_varInfos) {
            यदि (!v) अनुवर्त;  // Skip nullptr at m_varInfos[0]
            यदि (v->has_Const_परिणाम()) {
                // If a constant term is known, we can either drop it or the whole tree is constant
                Ast_ग्रन्थिः_व्यञ्जकम्* resultp = शून्यसूचकः;
                यदि (v->get_Const_परिणाम()) {
                    UASSERT_OBJ(visitor.is_Or_वृक्षः(), nodep,
                                "Only OR tree can yield known 1 result");
                    UINFO(9, "OR tree with const 1 term: " << v->refp());
                    // Known 1 bit in OR tree, whole result is 1
                    resultp = नूतन AstConst{fl, AstConst::BitTrue{}};
                } अन्यथा यदि (visitor.is_And_वृक्षः()) {
                    UINFO(9, "AND tree with const 0 term: " << v->refp());
                    // Known 0 bit in AND tree, whole result is 0
                    resultp = नूतन AstConst{fl, AstConst::BitFalse{}};
                } अन्यथा {
                    // Known 0 bit in OR or XOR tree. Ignore it.
                    अनुवर्त;
                }
                // Set width and widthMin precisely
                resultp->dtype_Chg_विस्तारः(परिणाम_विस्तारः, 1);
                प्रति (Ast_ग्रन्थिः* ध्रुव termp : termps) VL_DO_DANGLING(termp->delete_वृक्षः(), termp);
                प्रत्यागम resultp;
            }
            ध्रुव परिणाम_पदांशः परिणाम = v->get_परिणाम_पदांशः();
            termps.प्रक्षिप_पृष्ठभागः(std::get<0>(परिणाम));
            परिणाम_Ops += std::get<1>(परिणाम);
            यदि (std::get<2>(परिणाम)) {
                has_Clean_पदांशः = सत्य;
                UINFO(9, "Clean term: " << termps.पृष्ठभागः());
            } अन्यथा {
                has_Dirty_पदांशः = सत्य;
                UINFO(9, "Dirty term: " << termps.पृष्ठभागः());
            }
        }

        // Group by FrozenNodeInfo
        std::map<Frozen_ग्रन्थिः_Info, std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>> frozenNodes;
        // Check if frozen terms are clean or not
        प्रति (ध्रुव स्वचालित& frozenInfo : visitor.m_frozenNodes) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव termp = frozenInfo.first;
            // Comparison operators are clean
            यदि ((VN_IS(termp, Eq) || VN_IS(termp, Neq) || VN_IS(termp, Lt) || VN_IS(termp, Lte)
                 || VN_IS(termp, Gt) || VN_IS(termp, Gte))
                && frozenInfo.second.m_lsb == 0) {
                has_Clean_पदांशः = सत्य;
            } अन्यथा {
                // Otherwise, conservatively assume the frozen term is dirty
                has_Dirty_पदांशः = सत्य;
                UINFO(9, "Dirty frozen term: " << termp);
            }
            frozenNodes[frozenInfo.second].प्रक्षिप_पृष्ठभागः(termp);
        }

        // Figure out if a final negation is required
        ध्रुव तार्किक needsFlip = visitor.is_Xor_वृक्षः() && !visitor.m_polarity;

        // Figure out if the final tree needs cleaning
        ध्रुव तार्किक needsCleaning = visitor.is_And_वृक्षः() ? !has_Clean_पदांशः : has_Dirty_पदांशः;

        // Add size of reduction tree to op count
        परिणाम_Ops += termps.आकार() - 1;
        प्रति (ध्रुव स्वचालित& lsbAndNodes : frozenNodes) {
            यदि (lsbAndNodes.first.m_lsb > 0) ++परिणाम_Ops;  // Needs AstShiftR
            यदि (!lsbAndNodes.first.m_polarity) ++परिणाम_Ops;  // Needs AstNot
            परिणाम_Ops += lsbAndNodes.second.आकार();
        }
        // Add final polarity flip in Xor tree
        यदि (needsFlip) ++परिणाम_Ops;
        // Add final cleaning AND
        यदि (needsCleaning) ++परिणाम_Ops;

        यदि (debug() >= 9) {  // LCOV_EXCL_START
            निर्गमः << "-  Bitop tree considered:\n";
            प्रति (ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव termp : termps) termp->dump_वृक्षः("-  Reduced term: ");
            प्रति (ध्रुव std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, Frozen_ग्रन्थिः_Info>& termp : visitor.m_frozenNodes) {
                termp.first->dump_वृक्षः("-  Frozen term with lsb "
                                      + std::गन्तव्यम्_string(termp.second.m_lsb) + " polarity "
                                      + std::गन्तव्यम्_string(termp.second.m_polarity) + ": ");
            }
            निर्गमः << "-  Needs flipping: " << needsFlip << "\n";
            निर्गमः << "-  Needs cleaning: " << needsCleaning << "\n";
            निर्गमः << "-  Size: " << परिणाम_Ops << " input size: " << visitor.m_ops << "\n";
        }  // LCOV_EXCL_STOP

        // Sometimes we have no terms left after ignoring redundant terms
        // (all of which were zeroes)
        यदि (termps.रिक्तानि() && visitor.m_frozenNodes.रिक्तानि()) {
            reduction += visitor.m_ops;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव resultp = needsFlip ? नूतन AstConst{fl, AstConst::BitTrue{}}
                                                   : नूतन AstConst{fl, AstConst::BitFalse{}};
            resultp->dtype_Chg_विस्तारः(परिणाम_विस्तारः, 1);
            प्रत्यागम resultp;
        }

        // Only substitute the result if beneficial as determined by operation count
        यदि (visitor.m_ops <= परिणाम_Ops) {
            प्रति (Ast_ग्रन्थिः* ध्रुव termp : termps) VL_DO_DANGLING(termp->delete_वृक्षः(), termp);
            प्रत्यागम शून्यसूचकः;
        }

        // Update statistics
        reduction += visitor.m_ops - परिणाम_Ops;

        // Reduction op to combine terms
        ध्रुव स्वचालित reduce = [&visitor, fl](Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) -> Ast_ग्रन्थिः_व्यञ्जकम्* {
            यदि (!lhsp) प्रत्यागम rhsp;
            यदि (visitor.is_And_वृक्षः()) {
                प्रत्यागम नूतन AstAnd{fl, lhsp, rhsp};
            } अन्यथा यदि (visitor.is_Or_वृक्षः()) {
                प्रत्यागम नूतन AstOr{fl, lhsp, rhsp};
            } अन्यथा {
                प्रत्यागम नूतन AstXor{fl, lhsp, rhsp};
            }
        };

        // Compute result by reducing all terms
        Ast_ग्रन्थिः_व्यञ्जकम्* resultp = शून्यसूचकः;
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव termp : termps) {  //
            resultp = reduce(resultp, termp);
        }
        // Add any frozen terms to the reduction
        प्रति (स्वचालित&& nodes : frozenNodes) {
            // nodes.second has same lsb and polarity
            Ast_ग्रन्थिः_व्यञ्जकम्* termp = शून्यसूचकः;
            प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव itemp : nodes.second) {
                termp = reduce(termp, itemp->unlink_Fr_पृष्ठभागः());
            }
            यदि (nodes.first.m_lsb > 0) {  // LSB is not 0, so shiftR
                Ast_ग्रन्थिः_DType* ध्रुव dtypep = termp->dtypep();
                termp = नूतन Ast_सरणम्_R{termp->fileline(), termp,
                                      नूतन AstConst(termp->fileline(), AstConst::Widthed_मानम्{},
                                                   termp->विस्तारः(), nodes.first.m_lsb)};
                termp->dtypep(dtypep);
            }
            यदि (!nodes.first.m_polarity) {  // Polarity is inverted, so append Not
                Ast_ग्रन्थिः_DType* ध्रुव dtypep = termp->dtypep();
                termp = नूतन AstNot{termp->fileline(), termp};
                termp->dtypep(dtypep);
            }
            resultp = reduce(resultp, termp);
        }

        // Set width of masks to expected result width. This is required to prevent later removal
        // of the masking node e.g. by the "AND with all ones" rule. If the result width happens
        // to be 1, we still need to ensure the AstAnd is not dropped, so use a wider mask in this
        // special case.
        ध्रुव पूर्णाङ्क आवरणम्_विस्तारः
            = std::max(resultp->विस्तारः(), परिणाम_विस्तारः == 1 ? VL_IDATASIZE : परिणाम_विस्तारः);

        // Apply final polarity flip
        यदि (needsFlip) {
            यदि (needsCleaning) {
                // Cleaning will be added below. Use a NOT which is a byte shorter on x86
                resultp = नूतन AstNot{fl, resultp};
            } अन्यथा {
                // Keep result clean by using XOR(1, _)
                AstConst* ध्रुव maskp = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, आवरणम्_विस्तारः, 1};
                resultp = नूतन AstXor{fl, maskp, resultp};
            }
        }

        // Apply final cleaning
        यदि (needsCleaning) {
            AstConst* ध्रुव maskp = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, आवरणम्_विस्तारः, 1};
            resultp = नूतन AstAnd{fl, maskp, resultp};
        }

        // Cast back to original size if required
        यदि (resultp->विस्तारः() != परिणाम_विस्तारः) {
            resultp = नूतन AstCCast{fl, resultp, परिणाम_विस्तारः, 1};
        }

        // Set width and widthMin precisely
        resultp->dtype_Chg_विस्तारः(परिणाम_विस्तारः, 1);

        प्रत्यागम resultp;
    }
};

//######################################################################
// Const state, as a visitor of each AstNode

वर्ग ConstVisitor final : सार्वजनिक VNVisitor {
    // CONSTANTS
    स्थिर constexpr अचिह्नित CONCAT_MERGABLE_MAX_गभीरता = 10;  // Limit alg recursion
    // Largest right-hand side that is copied to each part of a split concatenation assignment
    स्थिर constexpr पूर्णाङ्क CONCAT_नियोजय_MAX_RHS_NODES = 16;

    // NODE STATE
    // See VNIdAllocator. All per node state below is held in side tables keyed on the
    // node ids it hands out, as node pointers are not stable keys within V3Const.

    // STATE
    VNIdAllocator m_ids;  // Node id allocator
    तार्किक m_params = असत्य;  // If true, propagate parameterized and true numbers only
    तार्किक m_required = असत्य;  // If true, must become a constant
    तार्किक m_wremove = सत्य;  // Inside scope, no assignw removal
    तार्किक m_warn = असत्य;  // Output warnings
    तार्किक m_doExpensive = असत्य;  // Enable computationally expensive optimizations
    तार्किक m_doCpp = असत्य;  // Enable late-stage C++ optimizations
    तार्किक m_doNConst = असत्य;  // Enable non-constant-child simplifications
    तार्किक m_doV = असत्य;  // Verilog, not C++ conversion
    तार्किक m_doGenerate = असत्य;  // Postpone width checking inside generate
    तार्किक m_convert_Logic_गन्तव्यम्_Bit = असत्य;  // Convert logical operators to bitwise
    तार्किक m_hasJumpDelay = असत्य;  // JumpGo or Delay under this loop
    तार्किक m_hasLoopTest = असत्य;  // Contains AstLoopTest
    तार्किक m_underRecFunc = असत्य;  // Under a recursive function
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    ध्रुव AstArraySel* m_selp = शून्यसूचकः;  // Current select
    ध्रुव AstScope* m_scopep = शून्यसूचकः;  // Current scope
    ध्रुव Ast_Attr_सम्बन्धि* m_attrp = शून्यसूचकः;  // Current attribute
    VDouble0 m_statBitOpReduction;  // Ops reduced in ConstBitOpTreeVisitor
    VDouble0 m_statConcatMerge;  // Concat merges
    VDouble0 m_stat_Cond_व्यञ्जकम्_Redundant;  // Conditional repeated expressions
    VDouble0 m_stat_If_Cond_व्यञ्जकम्_Redundant;  // Conditional repeated expressions
    VDouble0 m_statMemberAccessVisits;  // Nodes visited by containsMemberAccessRecurse
    ध्रुव तार्किक m_globalPass;  // ConstVisitor invoked as a global pass
    स्थिर uint32_t s_global_Pass_अङ्कः;  // Counts number of times ConstVisitor invoked as global pass
    V3_Unique_नामानि m_concswap_नामानि;  // For generating unique temporary variable names
    // Caches results of matchBiopToBitwise, keyed on node id
    std::unordered_map<आकार_t, तार्किक> m_अन्तर्भवति_Member_Access;
    // Items of enum currently being recursed into. Cannot use VNIdAllocator, as this runs in
    // every mode, including those invoked under a pass holding user4 (V3Param). Node pointers
    // are safe as keys here, unlike elsewhere in V3Const: folding the item value below does
    // free nodes whose addresses are then reused, but AstEnumItem is only ever constructed
    // while parsing (verilog.y and V3LinkParse), so a reused address can never become one,
    // and every key looked up is an AstEnumItem.
    std::unordered_समुच्चयः<ध्रुव AstEnumItem*> m_recursing_Enum_वस्तूनि;
    std::unordered_समुच्चयः<Ast_Jump_खण्डम्*> m_प्रयुक्तम्_Jump_खण्डानि;  // JumpBlocks used by some JumpGo

    // METHODS

    // Return true if statements may be deleted. Until statics are lifted, a variable declared
    // in a statement might be referenced from elsewhere (e.g. hierarchically).
    स्थिर तार्किक canDeleteStatements() { प्रत्यागम v3Global.staticsLifted(); }

    शून्य deleteVarScopesUnder(Ast_ग्रन्थिः* subtreep) {
        यदि (!subtreep) प्रत्यागम;
        यदि (!m_scopep) प्रत्यागम;
        std::unordered_समुच्चयः<AstVar*> varps;
        subtreep->foreach_And_अग्र([&](AstVar* varp) { varps.निवेशय(varp); });
        यदि (varps.रिक्तानि()) प्रत्यागम;
        प्रति (AstVarScope *vscp = m_scopep->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            यदि (varps.find(vscp->varp()) != varps.अन्तः_2()) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(vscp->unlink_Fr_पृष्ठभागः()), vscp);
            }
        }
    }

    V3_सङ्ख्या const_अङ्कः_V(Ast_ग्रन्थिः* nodep) {
        // Contract C width to V width (if needed, else just direct copy)
        // The upper zeros in the C representation can otherwise cause
        // wrong results in some operations, e.g. MulS
        ध्रुव V3_सङ्ख्या& numc = VN_AS(nodep, Const)->अङ्कः();
        प्रत्यागम !numc.is_सङ्ख्या() ? numc : V3_सङ्ख्या{nodep, nodep->विस्तारः_Min_V(), numc};
    }
    V3_सङ्ख्या गन्तव्यम्_अङ्कः_C(Ast_ग्रन्थिः* nodep, ध्रुव V3_सङ्ख्या& numv) {
        // Extend V width back to C width for given node
        प्रत्यागम !numv.is_सङ्ख्या() ? numv : V3_सङ्ख्या{nodep, nodep->विस्तारः(), numv};
    }

    स्थिर तार्किक अधस्तनम्_As_Fixed_Aggregate(ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep) {
        प्रत्यागम dtypep->isStreamableFixedAggregate() && dtypep->अन्तर्भवति_Unpacked_Struct();
    }

    AstStructSel* newStructSel(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp, ध्रुव AstMemberDType* ध्रुव itemp) {
        AstStructSel* ध्रुव selp = नूतन AstStructSel{fromp->fileline(), fromp, itemp->name()};
        selp->dtypeFrom(itemp->dtypep());
        प्रत्यागम selp;
    }

    शून्य collectFixedAggregateTerms(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp, std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>& termps,
                                    ध्रुव तार्किक packReal) {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = fromp->dtypep()->skipRefp();
        यदि (ध्रुव AstUnpackArrayDType* ध्रुव unpackDtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            प्रति (ध्रुव पूर्णाङ्क idx : unpackDtypep->declRange().seq_वामः_गन्तव्यम्_दक्षिणः()) {
                AstArraySel* ध्रुव selp
                    = नूतन AstArraySel{fromp->fileline(), fromp->clone_वृक्षः_विशुद्धम्(असत्य), idx};
                collectFixedAggregateTerms(selp, termps, packReal);
            }
            VL_DO_DANGLING(प्रक्षिप_Deletep(fromp), fromp);
        } अन्यथा यदि (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव sdtypep
                   = VN_CAST(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
            यदि (sdtypep->packed()) {
                termps.प्रक्षिप_पृष्ठभागः(fromp);
                प्रत्यागम;
            }
            प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                collectFixedAggregateTerms(newStructSel(fromp->clone_वृक्षः_विशुद्धम्(असत्य), itemp),
                                           termps, packReal);
            }
            VL_DO_DANGLING(प्रक्षिप_Deletep(fromp), fromp);
        } अन्यथा यदि (packReal && dtypep->isDouble()) {
            termps.प्रक्षिप_पृष्ठभागः(नूतन Ast_Real_गन्तव्यम्_द्वयङ्काः{fromp->fileline(), fromp});
        } अन्यथा {
            termps.प्रक्षिप_पृष्ठभागः(fromp);
        }
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* packFixedAggregate(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp) {
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> termps;
        collectFixedAggregateTerms(fromp, termps, सत्य);
        UASSERT(!termps.रिक्तानि(), "No stream terms");
        Ast_ग्रन्थिः_व्यञ्जकम्* resultp = termps[0];
        प्रति (आकार_t i = 1; i < termps.आकार(); ++i) {
            resultp = नूतन AstConcat{resultp->fileline(), resultp, termps[i]};
        }
        प्रत्यागम resultp;
    }

    शून्य replace_नियोजय_गन्तव्यम्_Fixed_Aggregate(Ast_ग्रन्थिः_नियोजय* ध्रुव nodep, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव dstp,
                                       Ast_ग्रन्थिः_व्यञ्जकम्* srcp) {
        ध्रुव पूर्णाङ्क dst_विस्तारः = dstp->dtypep()->विस्तारः_प्रवाहः();
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> termps;
        collectFixedAggregateTerms(dstp, termps, असत्य);
        पूर्णाङ्क src_विस्तारः = srcp->विस्तारः();
        यदि (src_विस्तारः < dst_विस्तारः) {
            AstExtend* ध्रुव extendp = नूतन AstExtend{srcp->fileline(), srcp, dst_विस्तारः};
            srcp = नूतन Ast_सरणम्_L{
                extendp->fileline(), extendp,
                नूतन AstConst{extendp->fileline(), static_cast<uint32_t>(dst_विस्तारः - src_विस्तारः)},
                dst_विस्तारः};
            src_विस्तारः = dst_विस्तारः;
        }
        Ast_ग्रन्थिः_नियोजय* newp = शून्यसूचकः;
        पूर्णाङ्क विस्थापनम् = 0;
        प्रति (आकार_t i = 0; i < termps.आकार(); ++i) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव termp = termps[i];
            ध्रुव पूर्णाङ्क विस्तारः = termp->dtypep()->विस्तारः_प्रवाहः();
            ध्रुव पूर्णाङ्क lsb = src_विस्तारः - विस्थापनम् - विस्तारः;
            विस्थापनम् += विस्तारः;
            Ast_ग्रन्थिः_व्यञ्जकम्* rhsp
                = नूतन AstSel{srcp->fileline(), srcp->clone_वृक्षः_विशुद्धम्(असत्य), lsb, विस्तारः};
            यदि (termp->dtypep()->skipRefp()->isDouble()) {
                rhsp = नूतन Ast_द्वयङ्काः_गन्तव्यम्_Real_D{rhsp->fileline(), rhsp};
            }
            Ast_ग्रन्थिः_नियोजय* ध्रुव assignp = nodep->cloneType(termp, rhsp);
            assignp->dtypeFrom(termp);
            newp = Ast_ग्रन्थिः::सङ्कलय_अग्र(newp, assignp);
        }
        nodep->सङ्कलय_अग्र_अत्र(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(srcp), srcp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }

    तार्किक सङ्कार्यम्_Const(ध्रुव Ast_ग्रन्थिः* nodep) { प्रत्यागम VN_IS(nodep, Const); }
    तार्किक सङ्कार्यम्_Asv_Const(ध्रुव Ast_ग्रन्थिः* nodep) {
        // BIASV(CONST, BIASV(CONST,...)) -> BIASV( BIASV_CONSTED(a,b), ...)
        ध्रुव Ast_ग्रन्थिः_Bi_Com_Asv* ध्रुव bnodep = VN_CAST(nodep, ग्रन्थिः_Bi_Com_Asv);
        यदि (!bnodep) प्रत्यागम असत्य;
        यदि (!VN_IS(bnodep->lhsp(), Const)) प्रत्यागम असत्य;
        ध्रुव Ast_ग्रन्थिः_Bi_Com_Asv* ध्रुव rnodep = VN_CAST(bnodep->rhsp(), ग्रन्थिः_Bi_Com_Asv);
        यदि (!rnodep) प्रत्यागम असत्य;
        यदि (rnodep->type() != bnodep->type()) प्रत्यागम असत्य;
        यदि (rnodep->विस्तारः() != bnodep->विस्तारः()) प्रत्यागम असत्य;
        यदि (rnodep->lhsp()->विस्तारः() != bnodep->lhsp()->विस्तारः()) प्रत्यागम असत्य;
        यदि (!VN_IS(rnodep->lhsp(), Const)) प्रत्यागम असत्य;
        प्रत्यागम सत्य;
    }
    तार्किक सङ्कार्यम्_Asv_Same(ध्रुव Ast_ग्रन्थिः* nodep) {
        // BIASV(SAMEa, BIASV(SAMEb,...)) -> BIASV( BIASV(SAMEa,SAMEb), ...)
        ध्रुव Ast_ग्रन्थिः_Bi_Com_Asv* ध्रुव bnodep = VN_CAST(nodep, ग्रन्थिः_Bi_Com_Asv);
        यदि (!bnodep) प्रत्यागम असत्य;
        ध्रुव Ast_ग्रन्थिः_Bi_Com_Asv* ध्रुव rnodep = VN_CAST(bnodep->rhsp(), ग्रन्थिः_Bi_Com_Asv);
        यदि (!rnodep) प्रत्यागम असत्य;
        यदि (rnodep->type() != bnodep->type()) प्रत्यागम असत्य;
        यदि (rnodep->विस्तारः() != bnodep->विस्तारः()) प्रत्यागम असत्य;
        प्रत्यागम operandsSame(bnodep->lhsp(), rnodep->lhsp());
    }
    तार्किक सङ्कार्यम्_Asv_LUp(ध्रुव Ast_ग्रन्थिः* nodep) {
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
        ध्रुव Ast_ग्रन्थिः_Bi_Com_Asv* ध्रुव bnodep = VN_CAST(nodep, ग्रन्थिः_Bi_Com_Asv);
        यदि (!bnodep) प्रत्यागम असत्य;
        ध्रुव Ast_ग्रन्थिः_Bi_Com_Asv* ध्रुव lnodep = VN_CAST(bnodep->lhsp(), ग्रन्थिः_Bi_Com_Asv);
        यदि (!lnodep) प्रत्यागम असत्य;
        यदि (lnodep->type() != bnodep->type()) प्रत्यागम असत्य;
        यदि (lnodep->विस्तारः() != bnodep->विस्तारः()) प्रत्यागम असत्य;
        प्रत्यागम VN_IS(lnodep->lhsp(), Const);
    }
    तार्किक सङ्कार्यम्_Asv_RUp(ध्रुव Ast_ग्रन्थिः* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr)) ?
        ध्रुव Ast_ग्रन्थिः_Bi_Com_Asv* ध्रुव bnodep = VN_CAST(nodep, ग्रन्थिः_Bi_Com_Asv);
        यदि (!bnodep) प्रत्यागम असत्य;
        ध्रुव Ast_ग्रन्थिः_Bi_Com_Asv* ध्रुव rnodep = VN_CAST(bnodep->rhsp(), ग्रन्थिः_Bi_Com_Asv);
        यदि (!rnodep) प्रत्यागम असत्य;
        यदि (rnodep->type() != bnodep->type()) प्रत्यागम असत्य;
        यदि (rnodep->विस्तारः() != bnodep->विस्तारः()) प्रत्यागम असत्य;
        प्रत्यागम VN_IS(rnodep->lhsp(), Const);
    }
    स्थिर तार्किक सङ्कार्यम्_Sub_सङ्कलय(ध्रुव Ast_ग्रन्थिः* nodep) {
        // SUB( ADD(CONSTx,y), CONSTz) -> ADD(SUB(CONSTx,CONSTz), y)
        ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव np = VN_CAST(nodep, ग्रन्थिः_Biop);
        ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव lp = VN_CAST(np->lhsp(), ग्रन्थिः_Biop);
        प्रत्यागम (lp && VN_IS(lp->lhsp(), Const) && VN_IS(np->rhsp(), Const)
                && lp->विस्तारः() == np->विस्तारः());
    }
    तार्किक मेलनम्_Redundant_Clean(AstAnd* andp) {
        // Remove And with constant one inserted by V3Clean
        // 1 & (a == b)  -> (IData)(a == b)
        // When bool is casted to int, the value is either 0 or 1
        AstConst* ध्रुव constp = VN_AS(andp->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isOne(), andp->lhsp(), "TRREEOPC must meet this condition");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = andp->rhsp();
        AstCCast* ccastp = शून्यसूचकः;
        ध्रुव स्वचालित isEqOrNeq = [](ध्रुव Ast_ग्रन्थिः* nodep) -> तार्किक {  //
            प्रत्यागम VN_IS(nodep, Eq) || VN_IS(nodep, Neq);
        };
        यदि (isEqOrNeq(rhsp)) {
            ccastp = नूतन AstCCast{andp->fileline(), rhsp->unlink_Fr_पृष्ठभागः(), andp};
        } अन्यथा यदि (AstCCast* ध्रुव tmpp = VN_CAST(rhsp, CCast)) {
            यदि (isEqOrNeq(tmpp->lhsp())) {
                यदि (tmpp->विस्तारः() == andp->विस्तारः()) {
                    tmpp->unlink_Fr_पृष्ठभागः();
                    ccastp = tmpp;
                } अन्यथा {
                    ccastp = नूतन AstCCast{andp->fileline(), tmpp->lhsp()->unlink_Fr_पृष्ठभागः(), andp};
                }
            }
        }
        यदि (ccastp) {
            andp->replaceWithKeepDType(ccastp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(andp), andp);
            प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    स्थिर तार्किक सङ्कार्यम्_And_Or_Same(ध्रुव Ast_ग्रन्थिः* nodep) {
        // OR( AND(VAL,x), AND(VAL,y)) -> AND(VAL,OR(x,y))
        // OR( AND(x,VAL), AND(y,VAL)) -> AND(OR(x,y),VAL)
        ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव np = VN_CAST(nodep, ग्रन्थिः_Biop);
        ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव lp = VN_CAST(np->lhsp(), ग्रन्थिः_Biop);
        ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव rp = VN_CAST(np->rhsp(), ग्रन्थिः_Biop);
        प्रत्यागम (lp && rp && lp->विस्तारः() == rp->विस्तारः() && lp->type() == rp->type()
                && (operandsSame(lp->lhsp(), rp->lhsp()) || operandsSame(lp->rhsp(), rp->rhsp())));
    }
    तार्किक मेलनम्_Or_And_Not(Ast_ग्रन्थिः_Biop* nodep) {
        // AstOr{$a, AstAnd{AstNot{$b}, $c}} if $a.width1, $a==$b => AstOr{$a,$c}
        // Someday we'll sort the biops completely and this can be simplified
        // This often results from our simplified clock generation:
        // if (rst) ... else if (enable)... -> OR(rst,AND(!rst,enable))
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ap;
        Ast_ग्रन्थिः_Biop* andp;
        यदि (VN_IS(nodep->lhsp(), And)) {
            andp = VN_AS(nodep->lhsp(), And);
            ap = nodep->rhsp();
        } अन्यथा यदि (VN_IS(nodep->rhsp(), And)) {
            andp = VN_AS(nodep->rhsp(), And);
            ap = nodep->lhsp();
        } अन्यथा {
            प्रत्यागम असत्य;
        }
        ध्रुव Ast_ग्रन्थिः_Uniop* notp;
        Ast_ग्रन्थिः_व्यञ्जकम्* cp;
        यदि (VN_IS(andp->lhsp(), Not)) {
            notp = VN_AS(andp->lhsp(), Not);
            cp = andp->rhsp();
        } अन्यथा यदि (VN_IS(andp->rhsp(), Not)) {
            notp = VN_AS(andp->rhsp(), Not);
            cp = andp->lhsp();
        } अन्यथा {
            प्रत्यागम असत्य;
        }
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bp = notp->lhsp();
        यदि (!operandsSame(ap, bp)) प्रत्यागम असत्य;
        // Do it
        cp->unlink_Fr_पृष्ठभागः();
        VL_DO_DANGLING(प्रक्षिप_Deletep(andp->unlink_Fr_पृष्ठभागः()), andp);
        VL_DANGLING(notp);
        // Replace whichever branch is now dangling
        यदि (nodep->rhsp()) {
            nodep->lhsp(cp);
        } अन्यथा {
            nodep->rhsp(cp);
        }
        प्रत्यागम सत्य;
    }
    तार्किक मेलनम्_And_Cond(AstAnd* nodep) {
        // Push down a AND into conditional, when one side of conditional is constant
        // (otherwise we'd be trading one operation for two operations)
        // V3Clean often makes this pattern, as it postpones the AND until
        // as high as possible, which is usually the right choice, except for this.
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstCond* ध्रुव condp = VN_CAST(nodep->rhsp(), Cond);
        यदि (!condp) प्रत्यागम असत्य;
        यदि (!VN_IS(condp->thenp(), Const) && !VN_IS(condp->elsep(), Const)) प्रत्यागम असत्य;
        AstConst* ध्रुव maskp = VN_CAST(nodep->lhsp(), Const);
        यदि (!maskp) प्रत्यागम असत्य;
        UINFO(4, "AND(CONSTm, CONDcond(c, i, e))->CONDcond(c, AND(m,i), AND(m, e)) " << nodep);
        AstCond* ध्रुव newp = नूतन AstCond{
            condp->fileline(), condp->condp()->unlink_Fr_पृष्ठभागः(),
            नूतन AstAnd{nodep->fileline(), maskp->clone_वृक्षः(असत्य), condp->thenp()->unlink_Fr_पृष्ठभागः()},
            नूतन AstAnd{nodep->fileline(), maskp->clone_वृक्षः(असत्य),
                       condp->elsep()->unlink_Fr_पृष्ठभागः()}};
        newp->thenp()->dtypeFrom(nodep);  // As And might have been to change widths
        newp->elsep()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        प्रत्यागम सत्य;
    }

    तार्किक मेलनम्_Cond_Cond(ध्रुव AstCond* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        यदि (VN_IS(nodep->backp(), Cond)) प्रत्यागम असत्य;  // Was checked when visited parent
        यदि (!VN_IS(nodep->thenp(), Cond) && !VN_IS(nodep->elsep(), Cond))
            प्रत्यागम असत्य;  // Short circuit
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> truesp;
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> falsesp;
        मेलनम्_Cond_Cond_Recurse(nodep, truesp /*ref*/, falsesp /*ref*/);
        प्रत्यागम असत्य;  // Can optimize further
    }
    शून्य मेलनम्_Cond_Cond_Recurse(ध्रुव AstCond* nodep, std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>& truesp,
                              std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        यदि (truesp.आकार() > 4 || falsesp.आकार() > 4) प्रत्यागम;
        यदि (!nodep->condp()->is_विशुद्धम्()) प्रत्यागम;
        तार्किक replaced = असत्य;
        प्रति (ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* condp : truesp) {
            यदि (replaced) भङ्ग;
            यदि (!operandsSame(nodep->condp(), condp)) अनुवर्त;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_अङ्कः(nodep->condp(), 1);
            replaced = सत्य;
            ++m_stat_Cond_व्यञ्जकम्_Redundant;
        }
        प्रति (ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* condp : falsesp) {
            यदि (replaced) भङ्ग;
            यदि (!operandsSame(nodep->condp(), condp)) अनुवर्त;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = सत्य;
            ++m_stat_Cond_व्यञ्जकम्_Redundant;
        }
        यदि (ध्रुव AstCond* subCondp = VN_CAST(nodep->thenp(), Cond)) {
            यदि (!replaced) truesp.emplace_पृष्ठभागः(nodep->condp());
            मेलनम्_Cond_Cond_Recurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            यदि (!replaced) truesp.निष्कासय_पृष्ठभागः();
        }
        यदि (ध्रुव AstCond* subCondp = VN_CAST(nodep->elsep(), Cond)) {
            यदि (!replaced) falsesp.emplace_पृष्ठभागः(nodep->condp());
            मेलनम्_Cond_Cond_Recurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            यदि (!replaced) falsesp.निष्कासय_पृष्ठभागः();
        }
    }
    शून्य मेलनम्_If_Cond_Cond(ध्रुव Ast_ग्रन्थिः_If* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        यदि (VN_IS(nodep->backp(), If)) प्रत्यागम;  // Was checked when visited parent
        यदि (!VN_IS(nodep->thensp(), If) && !VN_IS(nodep->elsesp(), If)) प्रत्यागम;  // Short circuit
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> truesp;
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> falsesp;
        मेलनम्_If_Cond_Cond_Recurse(nodep, truesp /*ref*/, falsesp /*ref*/);
    }
    शून्य मेलनम्_If_Cond_Cond_Recurse(ध्रुव Ast_ग्रन्थिः_If* nodep, std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>& truesp,
                                std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        यदि (truesp.आकार() > 4 || falsesp.आकार() > 4) प्रत्यागम;
        यदि (!nodep->condp()->is_विशुद्धम्()) प्रत्यागम;
        // A constant condition is folded by visit(AstNodeIf), once statements can be deleted
        यदि (VN_IS(nodep->condp(), Const)) प्रत्यागम;
        तार्किक replaced = असत्य;
        प्रति (ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* condp : truesp) {
            यदि (replaced) भङ्ग;
            यदि (!operandsSame(nodep->condp(), condp)) अनुवर्त;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_अङ्कः(nodep->condp(), 1);
            replaced = सत्य;
            ++m_stat_If_Cond_व्यञ्जकम्_Redundant;
        }
        प्रति (ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* condp : falsesp) {
            यदि (replaced) भङ्ग;
            यदि (!operandsSame(nodep->condp(), condp)) अनुवर्त;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = सत्य;
            ++m_stat_If_Cond_व्यञ्जकम्_Redundant;
        }
        // We only check the first statement of parent IF is an If
        // So we don't need to check for effects in the executing thensp/elsesp
        // altering the child't condition.  e.g. 'if (x) begin x=1; if (x) end'
        यदि (ध्रुव Ast_ग्रन्थिः_If* subIfp = VN_CAST(nodep->thensp(), ग्रन्थिः_If)) {
            यदि (!replaced) truesp.emplace_पृष्ठभागः(nodep->condp());
            मेलनम्_If_Cond_Cond_Recurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            यदि (!replaced) truesp.निष्कासय_पृष्ठभागः();
        }
        यदि (ध्रुव Ast_ग्रन्थिः_If* subIfp = VN_CAST(nodep->elsesp(), ग्रन्थिः_If)) {
            यदि (!replaced) falsesp.emplace_पृष्ठभागः(nodep->condp());
            मेलनम्_If_Cond_Cond_Recurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            यदि (!replaced) falsesp.निष्कासय_पृष्ठभागः();
        }
    }

    शून्य मेलनम्_If_Same_चरमः(Ast_ग्रन्थिः_If* nodep) {
        यदि (!nodep->thensp()) प्रत्यागम;
        यदि (!nodep->elsesp()) प्रत्यागम;
        यदि (!canDeleteStatements()) प्रत्यागम;
        // Iterate bodies in reverse
        Ast_ग्रन्थिः* tp = nodep->thensp()->lastp();
        Ast_ग्रन्थिः* ep = nodep->elsesp()->lastp();
        यावत् (tp && ep && tp->same_वृक्षः(ep)) {
            UASSERT_OBJ(!tp->nextp(), tp, "Expected no nextp");
            UASSERT_OBJ(!ep->nextp(), ep, "Expected no nextp");
            Ast_ग्रन्थिः* ध्रुव tPrevp = tp->prevp();
            Ast_ग्रन्थिः* ध्रुव ePrevp = ep->prevp();
            nodep->सङ्कलय_अग्र_अत्र(tp->unlink_Fr_पृष्ठभागः());
            VL_DO_DANGLING(प्रक्षिप_Deletep(ep->unlink_Fr_पृष्ठभागः()), ep);
            tp = tPrevp;
            ep = ePrevp;
        }
    }

    तार्किक मेलनम्_Masked_Or(AstAnd* nodep) {
        // Masking an OR with terms that have no bits set under the mask is replaced with masking
        // only the remaining terms. Canonical example as generated by V3Expand is:
        // 0xff & (a << 8 | b >> 24) --> 0xff & (b >> 24)

        // Compute how many significant bits are in the mask
        ध्रुव AstConst* ध्रुव constp = VN_AS(nodep->lhsp(), Const);
        ध्रुव uint32_t significant_द्वयङ्काः = constp->अङ्कः().विस्तारः_गन्तव्यम्_Fit();

        AstOr* ध्रुव orp = VN_AS(nodep->rhsp(), Or);

        // Predicate for checking whether the bottom 'significantBits' bits of the given expression
        // are all zeroes.
        ध्रुव स्वचालित checkBottomClear = [=](ध्रुव Ast_ग्रन्थिः* nodep) -> तार्किक {
            यदि (ध्रुव Ast_सरणम्_L* ध्रुव shiftp = VN_CAST(nodep, सरणम्_L)) {
                यदि (ध्रुव AstConst* ध्रुव scp = VN_CAST(shiftp->rhsp(), Const)) {
                    प्रत्यागम scp->अङ्कः().गन्तव्यम्_UInt() >= significant_द्वयङ्काः;
                }
            }
            प्रत्यागम असत्य;
        };

        ध्रुव तार्किक orLIsRedundant = checkBottomClear(orp->lhsp());
        ध्रुव तार्किक orRIsRedundant = checkBottomClear(orp->rhsp());

        यदि (orLIsRedundant && orRIsRedundant) {
            nodep->replaceWithKeepDType(
                नूतन AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम सत्य;
        } अन्यथा यदि (orLIsRedundant) {
            orp->replaceWithKeepDType(orp->rhsp()->unlink_Fr_पृष्ठभागः());
            VL_DO_DANGLING(प्रक्षिप_Deletep(orp), orp);
            प्रत्यागम असत्य;  // input node is still valid, keep going
        } अन्यथा यदि (orRIsRedundant) {
            orp->replaceWithKeepDType(orp->lhsp()->unlink_Fr_पृष्ठभागः());
            VL_DO_DANGLING(प्रक्षिप_Deletep(orp), orp);
            प्रत्यागम असत्य;  // input node is still valid, keep going
        } अन्यथा {
            प्रत्यागम असत्य;
        }
    }
    तार्किक मेलनम्_Masked_सरणम्(AstAnd* nodep) {
        // Drop redundant masking of right shift result. E.g: 0xff & ((uint32_t)a >> 24). This
        // commonly appears after V3Expand and the simplification in matchMaskedOr. Similarly,
        // drop redundant masking of left shift result. E.g.: 0xff000000 & ((uint32_t)a << 24).

        ध्रुव स्वचालित check_आवरणम् = [nodep, एतत्](ध्रुव V3_सङ्ख्या& आवरणम्) -> तार्किक {
            ध्रुव AstConst* ध्रुव constp = VN_AS(nodep->lhsp(), Const);
            यदि (constp->अङ्कः().isCaseEq(आवरणम्)) {
                Ast_ग्रन्थिः* ध्रुव rhsp = nodep->rhsp();
                rhsp->unlink_Fr_पृष्ठभागः();
                nodep->replaceWithKeepDType(rhsp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम सत्य;
            }
            प्रत्यागम असत्य;
        };

        // Check if masking is redundant
        यदि (ध्रुव Ast_सरणम्_R* ध्रुव shiftp = VN_CAST(nodep->rhsp(), सरणम्_R)) {
            यदि (ध्रुव AstConst* ध्रुव scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_सङ्ख्या आवरणम्_Lo{nodep, nodep->विस्तारः()};
                आवरणम्_Lo.समुच्चयः_आवरणम्(nodep->विस्तारः() - scp->अङ्कः().गन्तव्यम्_UInt());
                प्रत्यागम check_आवरणम्(आवरणम्_Lo);
            }
        } अन्यथा यदि (ध्रुव Ast_सरणम्_L* ध्रुव shiftp = VN_CAST(nodep->rhsp(), सरणम्_L)) {
            यदि (ध्रुव AstConst* ध्रुव scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_सङ्ख्या आवरणम्{nodep, nodep->विस्तारः()};
                ध्रुव uint32_t सरणम्_Amount = scp->अङ्कः().गन्तव्यम्_UInt();
                आवरणम्.समुच्चयः_आवरणम्(nodep->विस्तारः() - सरणम्_Amount, सरणम्_Amount);
                प्रत्यागम check_आवरणम्(आवरणम्);
            }
        }
        प्रत्यागम असत्य;
    }

    तार्किक मेलनम्_Masked_Zero(ध्रुव AstAnd* nodep) {
        // Turn masking of known zero bits into constant zero. Commonly appears after V3Expand.
        ध्रुव AstConst* ध्रुव maskp = VN_AS(nodep->lhsp(), Const);
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp();
        ध्रुव uint32_t msbP1 = maskp->अङ्कः().most_समुच्चयः_Bit_P1();
        यदि (!msbP1) प्रत्यागम असत्य;  // Don't rewrite, separate rule matches for this
        ध्रुव uint32_t msb = msbP1 - 1;
        ध्रुव uint32_t lsb = maskp->अङ्कः().न्यूनतमम्_समुच्चयः_Bit_P1() - 1;

        यदि (ध्रुव Ast_सरणम्_L* ध्रुव shiftp = VN_CAST(rhsp, सरणम्_L)) {
            // 'a << S' forces the low S bits to zero
            यदि (AstConst* ध्रुव scp = VN_CAST(shiftp->rhsp(), Const)) {
                प्रत्यागम scp->अङ्कः().माति_अन्तर्_UInt()  //
                       && (scp->अङ्कः().गन्तव्यम्_UInt() > msb);
            }
        }
        यदि (ध्रुव Ast_सरणम्_R* ध्रुव shiftp = VN_CAST(rhsp, सरणम्_R)) {
            // 'a >> S' forces the high S bits to zero. Check against the width of the shifted
            // operand, V3Expand can create shifts wider than their inputs. Must use width(),
            // not widthMin(), as bits above widthMin() are not guaranteed to be zero.
            यदि (AstConst* ध्रुव scp = VN_CAST(shiftp->rhsp(), Const)) {
                प्रत्यागम scp->अङ्कः().माति_अन्तर्_UInt()
                       && (lsb + scp->अङ्कः().गन्तव्यम्_UInt()
                           >= static_cast<uint32_t>(shiftp->lhsp()->विस्तारः()));
            }
        }
        यदि (ध्रुव AstMul* ध्रुव mulp = VN_CAST(rhsp, Mul)) {
            // 'C * a' forces the low N bits to zero where 'C' has low zero bits
            यदि (AstConst* ध्रुव cp = VN_CAST(mulp->lhsp(), Const)) {
                प्रत्यागम cp->अङ्कः().न्यूनतमम्_समुच्चयः_Bit_P1() > msb + 1;
            }
        }
        यदि (ध्रुव AstExtend* ध्रुव extendp = VN_CAST(rhsp, Extend)) {
            // Zero-extension forces the bits above the source width to zero
            प्रत्यागम lsb >= static_cast<uint32_t>(extendp->lhsp()->विस्तारः());
        }

        प्रत्यागम असत्य;
    }

    तार्किक मेलनम्_Bit_Op_वृक्षः(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (nodep->विस्तारः_Min() != 1) प्रत्यागम असत्य;
        यदि (!v3Global.opt.f_Const_Bit_Op_वृक्षः()) प्रत्यागम असत्य;

        ध्रुव पूर्णाङ्क विस्तारः = nodep->विस्तारः();
        Ast_ग्रन्थिः_व्यञ्जकम्* rootp = nodep;
        अचिह्नित externalOps = 0;
        // Reach past a plain making AND
        यदि (ध्रुव AstAnd* ध्रुव andp = VN_CAST(nodep, And)) {
            यदि (isConst(andp->lhsp(), 1)) {
                rootp = andp->rhsp();
                externalOps = 1;
            }
        }

        // Only optimize if rootp is in fact the root of a tree of identical
        // operations, otherwise analysis on an eventually unoptimizable unablanced
        // tree can go O(N^2) as we would re-analyze every time as we move up the
        // tree, repeatedly re-discover the sub-tree is not optimizable.
        यदि (rootp->type() == nodep->backp()->type()) प्रत्यागम असत्य;

        std::string debug_उपसर्गः;
        यदि (debug() >= 9) {  // LCOV_EXCL_START
            स्थिर पूर्णाङ्क s_c = 0;
            debug_उपसर्गः = "-  matchBitOpTree[" + std::गन्तव्यम्_string(++s_c) + "] ";
            nodep->dump_वृक्षः(debug_उपसर्गः + "INPUT: ");
        }  // LCOV_EXCL_STOP

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = Const_Bit_Op_वृक्षः_Visitor::simplify(rootp, विस्तारः, externalOps,
                                                                  m_statBitOpReduction, m_ids);

        यदि (newp) {
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Transformed leaf of bit tree to " << newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }

        यदि (debug() >= 9) {  // LCOV_EXCL_START
            यदि (newp) {
                newp->dump_वृक्षः(debug_उपसर्गः + "RESULT: ");
            } अन्यथा {
                निर्गमः << debug_उपसर्गः << "not replaced\n";
            }
        }  // LCOV_EXCL_STOP

        प्रत्यागम newp;
    }
    स्थिर तार्किक सङ्कार्यम्_सरणम्_Same(ध्रुव Ast_ग्रन्थिः* nodep) {
        ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव np = VN_AS(nodep, ग्रन्थिः_Biop);
        {
            ध्रुव Ast_सरणम्_L* ध्रुव lp = VN_CAST(np->lhsp(), सरणम्_L);
            ध्रुव Ast_सरणम्_L* ध्रुव rp = VN_CAST(np->rhsp(), सरणम्_L);
            यदि (lp && rp) {
                प्रत्यागम (lp->विस्तारः() == rp->विस्तारः() && lp->lhsp()->विस्तारः() == rp->lhsp()->विस्तारः()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        {
            ध्रुव Ast_सरणम्_R* ध्रुव lp = VN_CAST(np->lhsp(), सरणम्_R);
            ध्रुव Ast_सरणम्_R* ध्रुव rp = VN_CAST(np->rhsp(), सरणम्_R);
            यदि (lp && rp) {
                प्रत्यागम (lp->विस्तारः() == rp->विस्तारः() && lp->lhsp()->विस्तारः() == rp->lhsp()->विस्तारः()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        प्रत्यागम असत्य;
    }
    तार्किक सङ्कार्यम्_Huge_सरणम्_L(ध्रुव Ast_ग्रन्थिः_Biop* nodep) {
        प्रत्यागम (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->अङ्कः().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->अङ्कः().माति_अन्तर्_UInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->गन्तव्यम्_UInt()
                        >= static_cast<uint32_t>(nodep->विस्तारः())))
                && nodep->lhsp()->is_विशुद्धम्());
    }
    तार्किक सङ्कार्यम्_Huge_सरणम्_R(ध्रुव Ast_ग्रन्थिः_Biop* nodep) {
        प्रत्यागम (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->अङ्कः().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->अङ्कः().माति_अन्तर्_UInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->गन्तव्यम्_UInt()
                        >= static_cast<uint32_t>(nodep->lhsp()->विस्तारः())))
                && nodep->lhsp()->is_विशुद्धम्()
                && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), प्रवाहः_DType)));
    }
    तार्किक सङ्कार्यम्_Is_Two(ध्रुव Ast_ग्रन्थिः* nodep) {
        ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const);
        यदि (!constp) प्रत्यागम असत्य;  // not constant
        यदि (constp->अङ्कः().isFourState()) प्रत्यागम असत्य;  // four-state
        यदि (nodep->विस्तारः() > VL_QUADSIZE) प्रत्यागम असत्य;  // too wide
        यदि (nodep->isSigned() && constp->अङ्कः().isNegative()) प्रत्यागम असत्य;  // signed and negative
        प्रत्यागम constp->गन्तव्यम्_UQuad() == 2;
    }
    तार्किक सङ्कार्यम्_Is_Twostate(ध्रुव Ast_ग्रन्थिः* nodep) {
        प्रत्यागम (VN_IS(nodep, Const) && !VN_AS(nodep, Const)->अङ्कः().isFourState());
    }
    तार्किक सङ्कार्यम्_Is_Pow_Two(ध्रुव Ast_ग्रन्थिः* nodep) {
        यदि (!सङ्कार्यम्_Is_Twostate(nodep)) प्रत्यागम असत्य;
        प्रत्यागम (1 == VN_AS(nodep, Const)->अङ्कः().गणनम्_Ones());
    }
    तार्किक सङ्कार्यम्_सरणम्_Op(ध्रुव Ast_ग्रन्थिः_Biop* nodep) {
        यदि (!VN_IS(nodep->rhsp(), Const)) प्रत्यागम असत्य;
        ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव lhsp = VN_CAST(nodep->lhsp(), ग्रन्थिः_Biop);
        यदि (!lhsp || !(VN_IS(lhsp, And) || VN_IS(lhsp, Or) || VN_IS(lhsp, Xor))) प्रत्यागम असत्य;
        यदि (nodep->विस्तारः() != lhsp->विस्तारः()) प्रत्यागम असत्य;
        यदि (nodep->विस्तारः() != lhsp->lhsp()->विस्तारः()) प्रत्यागम असत्य;
        यदि (nodep->विस्तारः() != lhsp->rhsp()->विस्तारः()) प्रत्यागम असत्य;

        // Only do it if we can fruther reduce one of the sides if pulling the shift through:
        // - Either operands are constants
        यदि (VN_IS(lhsp->lhsp(), Const)) प्रत्यागम सत्य;
        यदि (VN_IS(lhsp->rhsp(), Const)) प्रत्यागम सत्य;
        // - Const shift on either side
        यदि (Ast_ग्रन्थिः_Biop* ध्रुव llp = VN_CAST(lhsp->lhsp(), ग्रन्थिः_Biop)) {
            प्रत्यागम (VN_IS(llp, सरणम्_L) || VN_IS(llp, सरणम्_R)) && VN_IS(llp->rhsp(), Const);
        }
        यदि (Ast_ग्रन्थिः_Biop* ध्रुव lrp = VN_CAST(lhsp->rhsp(), ग्रन्थिः_Biop)) {
            प्रत्यागम (VN_IS(lrp, सरणम्_L) || VN_IS(lrp, सरणम्_R)) && VN_IS(lrp->rhsp(), Const);
        }
        // Otherwise we would increase logic size
        प्रत्यागम असत्य;
    }
    तार्किक सङ्कार्यम्_सरणम्_सरणम्(ध्रुव Ast_ग्रन्थिः_Biop* nodep) {
        // We could add a AND though.
        ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव lhsp = VN_CAST(nodep->lhsp(), ग्रन्थिः_Biop);
        यदि (!lhsp || !(VN_IS(lhsp, सरणम्_L) || VN_IS(lhsp, सरणम्_R))) प्रत्यागम असत्य;
        // We can only get rid of a<<b>>c or a<<b<<c, with constant b & c
        // because bits may be masked in that process, or (b+c) may exceed the word width.
        यदि (!(VN_IS(nodep->rhsp(), Const) && VN_IS(lhsp->rhsp(), Const))) प्रत्यागम असत्य;
        यदि (VN_AS(nodep->rhsp(), Const)->अङ्कः().isFourState()
            || VN_AS(lhsp->rhsp(), Const)->अङ्कः().isFourState())
            प्रत्यागम असत्य;
        यदि (nodep->विस्तारः() != lhsp->विस्तारः()) प्रत्यागम असत्य;
        यदि (nodep->विस्तारः() != lhsp->lhsp()->विस्तारः()) प्रत्यागम असत्य;
        प्रत्यागम सत्य;
    }
    तार्किक सङ्कार्यम्_Word_OOB(ध्रुव AstWordSel* nodep) {
        // V3Expand may make a arraysel that exceeds the bounds of the array
        // It was an expression, then got constified.  In reality, the WordSel
        // must be wrapped in a Cond, that will be false.
        प्रत्यागम (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ग्रन्थिः_Var_Ref)
                && VN_AS(nodep->fromp(), ग्रन्थिः_Var_Ref)->access().isReadOnly()
                && (static_cast<पूर्णाङ्क>(VN_AS(nodep->bitp(), Const)->गन्तव्यम्_UInt())
                    >= VN_AS(nodep->fromp(), ग्रन्थिः_Var_Ref)->varp()->विस्तारः_शब्दाः()));
    }
    तार्किक सङ्कार्यम्_Sel_पूर्णम्(ध्रुव AstSel* nodep) {
        प्रत्यागम (VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0
                && static_cast<पूर्णाङ्क>(nodep->विस्तारः_Const()) == nodep->fromp()->विस्तारः());
    }
    तार्किक सङ्कार्यम्_Sel_Extend(AstSel* nodep) {
        यदि (!m_doV) प्रत्यागम असत्य;
        AstExtend* ध्रुव extendp = VN_CAST(nodep->fromp(), Extend);
        यदि (!extendp) प्रत्यागम असत्य;
        AstConst* ध्रुव lsbp = VN_CAST(nodep->lsbp(), Const);
        यदि (!lsbp) प्रत्यागम असत्य;
        ध्रुव पूर्णाङ्क विस्तारः = nodep->विस्तारः_Const();
        ध्रुव पूर्णाङ्क lsb = lsbp->गन्तव्यम्_SInt();
        ध्रुव पूर्णाङ्क msb = lsb + विस्तारः - 1;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = extendp->lhsp();
        यदि (!lhsp->is_विशुद्धम्()) प्रत्यागम असत्य;

        // Selecting the entire extended expression, replace with that
        यदि (lsb == 0 && msb == lhsp->विस्तारः() - 1) {
            lhsp->unlink_Fr_पृष्ठभागः();
            nodep->replaceWithKeepDType(lhsp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम सत्य;
        }
        // Select entirely in the extended part - replace with zero
        यदि (lsb >= lhsp->विस्तारः()) {
            replaceZero(nodep);
            प्रत्यागम सत्य;
        }
        // Select entirely in the extended expression - replace with select from that
        यदि (msb < lhsp->विस्तारः()) {
            lhsp->unlink_Fr_पृष्ठभागः();
            lsbp->unlink_Fr_पृष्ठभागः();
            nodep->replaceWithKeepDType(नूतन AstSel{nodep->fileline(), lhsp, lsbp, विस्तारः});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम सत्य;
        }
        // Select straddles both sides, but is just a shorter extend
        यदि (lsb == 0) {
            lhsp->unlink_Fr_पृष्ठभागः();
            nodep->replaceWithKeepDType(नूतन AstExtend{nodep->fileline(), lhsp});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम सत्य;
        }

        प्रत्यागम असत्य;
    }
    तार्किक सङ्कार्यम्_Sel_Bi_अधस्तनम्(AstSel* nodep) {
        // SEL(ADD(a,b),(width-1),0) -> ADD(SEL(a),SEL(b))
        // Add or any operation which doesn't care if we discard top bits
        Ast_ग्रन्थिः_Biop* ध्रुव bip = VN_CAST(nodep->fromp(), ग्रन्थिः_Biop);
        यदि (!(m_doV && bip && VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0)) प्रत्यागम असत्य;
        UINFOTREE(9, nodep, "", "SEL(BI)-in");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bilhsp = bip->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव birhsp = bip->rhsp()->unlink_Fr_पृष्ठभागः();
        bip->lhsp(नूतन AstSel{nodep->fileline(), bilhsp, 0, nodep->विस्तारः_Const()});
        bip->rhsp(नूतन AstSel{nodep->fileline(), birhsp, 0, nodep->विस्तारः_Const()});
        UINFOTREE(9, bip, "", "SEL(BI)-ou");
        VL_DO_DANGLING(replaceWChild(nodep, bip), nodep);
        प्रत्यागम सत्य;
    }
    तार्किक सङ्कार्यम्_Sel_सरणम्_अधस्तनम्(AstSel* nodep) {
        // AND({a}, SHIFTR({b}, {c})) is often shorthand in C for Verilog {b}[{c} :+ {a}]
        // becomes thought other optimizations
        // SEL(SHIFTR({a},{b}),{lsb},{width}) -> SEL({a},{lsb+b},{width})
        ध्रुव Ast_सरणम्_R* ध्रुव shiftp = VN_CAST(nodep->fromp(), सरणम्_R);
        यदि (!(m_doV && shiftp && VN_IS(shiftp->rhsp(), Const) && VN_IS(nodep->lsbp(), Const))) {
            प्रत्यागम असत्य;
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ap = shiftp->lhsp();
        ध्रुव AstConst* ध्रुव bp = VN_AS(shiftp->rhsp(), Const);
        ध्रुव AstConst* ध्रुव lp = VN_AS(nodep->lsbp(), Const);
        यदि (bp->isWide() || bp->अङ्कः().isFourState() || bp->अङ्कः().isNegative() || lp->isWide()
            || lp->अङ्कः().isFourState() || lp->अङ्कः().isNegative()) {
            प्रत्यागम असत्य;
        }
        ध्रुव पूर्णाङ्क newLsb = lp->गन्तव्यम्_SInt() + bp->गन्तव्यम्_SInt();
        यदि (newLsb + nodep->विस्तारः_Const() > ap->विस्तारः()) प्रत्यागम असत्य;
        //
        UINFO(9, "SEL(SHIFTR(a,b),l,w) -> SEL(a,l+b,w)");
        UINFOTREE(9, nodep, "", "SEL(SH)-in");
        AstSel* ध्रुव newp
            = नूतन AstSel{nodep->fileline(), ap->unlink_Fr_पृष्ठभागः(), newLsb, nodep->विस्तारः_Const()};
        nodep->replaceWithKeepDType(newp);
        UINFOTREE(9, newp, "", "SEL(SH)-ou");
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        प्रत्यागम सत्य;
    }

    तार्किक सङ्कार्यम्_Bi_Extend_Const_Shrink(Ast_ग्रन्थिः_Biop* nodep) {
        // Loop unrolling favors standalone compares
        // EQ(const{width32}, EXTEND(xx{width3})) -> EQ(const{3}, xx{3})
        // The constant must have zero bits (+ 1 if signed) or compare
        // would be incorrect. See also operandBiExtendConst
        AstExtend* ध्रुव extendp = VN_CAST(nodep->rhsp(), Extend);
        यदि (!extendp) प्रत्यागम असत्य;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव smallerp = extendp->lhsp();
        ध्रुव पूर्णाङ्क subsize = smallerp->विस्तारः();
        AstConst* ध्रुव constp = VN_CAST(nodep->lhsp(), Const);
        यदि (!constp) प्रत्यागम असत्य;
        यदि (!constp->अङ्कः().is_द्वयङ्काः_Zero(constp->विस्तारः() - 1, subsize)) प्रत्यागम असत्य;
        //
        UINFOTREE(9, nodep, "", "BI(EXTEND)-in");
        smallerp->unlink_Fr_पृष्ठभागः();
        VL_DO_DANGLING(प्रक्षिप_Deletep(extendp->unlink_Fr_पृष्ठभागः()), extendp);  // aka nodep->lhsp.
        nodep->rhsp(smallerp);

        constp->unlink_Fr_पृष्ठभागः();
        ध्रुव V3_सङ्ख्या अङ्कः{constp, subsize, constp->अङ्कः()};
        nodep->lhsp(नूतन AstConst{constp->fileline(), अङ्कः});
        VL_DO_DANGLING(प्रक्षिप_Deletep(constp), constp);
        प्रत्यागम असत्य;  // input node is still valid, keep going
    }
    तार्किक सङ्कार्यम्_Bi_Extend_Const_Over(ध्रुव Ast_ग्रन्थिः_Biop* nodep) {
        // EQ(const{width32}, EXTEND(xx{width3})) -> constant
        // When the constant has non-zero bits above the extend it's a constant.
        // Avoids compiler warning
        ध्रुव AstExtend* ध्रुव extendp = VN_CAST(nodep->rhsp(), Extend);
        यदि (!extendp) प्रत्यागम असत्य;
        ध्रुव Ast_ग्रन्थिः* ध्रुव smallerp = extendp->lhsp();
        ध्रुव पूर्णाङ्क subsize = smallerp->विस्तारः();
        ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->lhsp(), Const);
        यदि (!constp) प्रत्यागम असत्य;
        यदि (constp->अङ्कः().is_द्वयङ्काः_Zero(constp->विस्तारः() - 1, subsize)) प्रत्यागम असत्य;
        प्रत्यागम सत्य;
    }

    // Extraction checks
    तार्किक warnSelect(AstSel* nodep) {
        यदि (m_doGenerate) {
            // Never checked yet
            V3_विस्तारः::विस्तारः_Params_Edit(nodep);
            iterateChildren(nodep);  // May need "constifying"
        }
        // Find range of dtype we are selecting from
        // Similar code in V3Unknown::AstSel
        ध्रुव तार्किक doit = सत्य;
        यदि (m_warn && VN_IS(nodep->lsbp(), Const) && doit) {
            ध्रुव पूर्णाङ्क maxDeclBit = nodep->declRange().hiMaxSelect() * nodep->decl_El_विस्तारः()
                                   + (nodep->decl_El_विस्तारः() - 1);
            यदि (VN_AS(nodep->lsbp(), Const)->अङ्कः().isFourState()) {
                nodep->v3error("Selection index is constantly unknown or tristated: "
                               "lsb="
                               << nodep->lsbp()->name() << " width=" << nodep->विस्तारः_Const());
                // Replacing nodep will make a mess above, so we replace the offender
                replaceZero(nodep->lsbp());
            } अन्यथा यदि (nodep->declRange().ranged()
                       && (nodep->msbConst() > maxDeclBit || nodep->lsbConst() > maxDeclBit)) {
                // See also warning in V3Width
                // Must adjust by element width as declRange() is in number of elements
                string msbLsbProtected;
                यदि (nodep->decl_El_विस्तारः() == 0) {
                    msbLsbProtected = "(nodep->declElWidth() == 0) "
                                      + std::गन्तव्यम्_string(nodep->msbConst()) + ":"
                                      + std::गन्तव्यम्_string(nodep->lsbConst());
                } अन्यथा {
                    msbLsbProtected = std::गन्तव्यम्_string(nodep->msbConst() / nodep->decl_El_विस्तारः())
                                      + ":"
                                      + std::गन्तव्यम्_string(nodep->lsbConst() / nodep->decl_El_विस्तारः());
                }
                nodep->v3warn(SELRANGE,
                              "Selection index out of range: "
                                  << msbLsbProtected << " outside "
                                  << nodep->declRange().hiMaxSelect() << ":0"
                                  << (nodep->declRange().lo() >= 0
                                          ? ""
                                          : (" (adjusted +" + cvt_गन्तव्यम्_Str(-nodep->declRange().lo())
                                             + " to account for negative lsb)")));
                UINFO(1, "    Related Raw index is " << nodep->msbConst() << ":"
                                                     << nodep->lsbConst());
                // Don't replace with zero, we'll do it later
            }
        }
        प्रत्यागम असत्य;  // Not a transform, so NOP
    }

    स्थिर तार्किक operandsSame(ध्रुव Ast_ग्रन्थिः* node1p, ध्रुव Ast_ग्रन्थिः* node2p) {
        // For now we just detect constants & simple vars, though it could be more generic
        यदि (ध्रुव AstConst* ध्रुव const1p = VN_CAST(node1p, Const)) {
            यदि (ध्रुव AstConst* ध्रुव const2p = VN_CAST(node2p, Const)) {
                प्रत्यागम V3_सङ्ख्या{node1p->fileline(), 1, 0}
                    .opEq(const1p->अङ्कः(), const2p->अङ्कः())
                    .isEqOne();
            }
        }
        यदि (VN_IS(node1p, VarRef) && VN_IS(node2p, VarRef)) {
            // Avoid comparing widthMin's, which results in lost optimization attempts
            // If cleanup sameGateTree to be smarter, this can be restored.
            // return node1p->sameGateTree(node2p);
            प्रत्यागम node1p->isSame(node2p);
        }
        // Pattern created by coverage-line/branch; avoid compiler tautological-compare warning
        यदि (ध्रुव AstAnd* ध्रुव and1p = VN_CAST(node1p, And)) {
            यदि (ध्रुव AstAnd* ध्रुव and2p = VN_CAST(node2p, And)) {
                यदि (VN_IS(and1p->lhsp(), Const) && VN_IS(and1p->rhsp(), ग्रन्थिः_Var_Ref)
                    && VN_IS(and2p->lhsp(), Const) && VN_IS(and2p->rhsp(), ग्रन्थिः_Var_Ref))
                    प्रत्यागम node1p->same_Gate_वृक्षः(node2p);
            }
        }
        प्रत्यागम असत्य;
    }
    तार्किक if_Same_नियोजय(ध्रुव Ast_ग्रन्थिः_If* nodep) {
        ध्रुव Ast_ग्रन्थिः_नियोजय* ध्रुव thensp = VN_CAST(nodep->thensp(), ग्रन्थिः_नियोजय);
        ध्रुव Ast_ग्रन्थिः_नियोजय* ध्रुव elsesp = VN_CAST(nodep->elsesp(), ग्रन्थिः_नियोजय);
        यदि (!thensp || thensp->nextp()) प्रत्यागम असत्य;  // Must be SINGLE statement
        यदि (!elsesp || elsesp->nextp()) प्रत्यागम असत्य;
        यदि (thensp->type() != elsesp->type()) प्रत्यागम असत्य;  // Can't mix an assigndly with assign
        यदि (!thensp->lhsp()->same_Gate_वृक्षः(elsesp->lhsp())) प्रत्यागम असत्य;
        यदि (!thensp->rhsp()->gate_वृक्षः()) प्रत्यागम असत्य;
        यदि (!elsesp->rhsp()->gate_वृक्षः()) प्रत्यागम असत्य;
        // Must not create an expression with unpacked array type
        यदि (VN_IS(thensp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) प्रत्यागम असत्य;
        यदि (VN_IS(elsesp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) प्रत्यागम असत्य;
        यदि (m_underRecFunc) प्रत्यागम असत्य;  // This optimization may lead to infinite recursion
        यदि (m_doCpp) {  // Do not combine ifs cretaed by V3Premit
            यदि (VN_IS(thensp->lhsp(), VarRef) && thensp->lhsp()->isWide()) प्रत्यागम असत्य;
        }
        // Only do it if not calls and both pure, otherwise undoes V3LiftExpr
        प्रत्यागम !VN_IS(thensp->rhsp(), ग्रन्थिः_FTask_Ref)  //
               && !VN_IS(elsesp->rhsp(), ग्रन्थिः_FTask_Ref)  //
               && thensp->rhsp()->is_विशुद्धम्()  //
               && elsesp->rhsp()->is_विशुद्धम्();
    }
    तार्किक सङ्कार्यम्_If_If(ध्रुव Ast_ग्रन्थिः_If* nodep) {
        यदि (nodep->elsesp()) प्रत्यागम असत्य;
        ध्रुव Ast_ग्रन्थिः_If* ध्रुव अधस्तनम्_Ifp = VN_CAST(nodep->thensp(), ग्रन्थिः_If);
        यदि (!अधस्तनम्_Ifp || अधस्तनम्_Ifp->nextp()) प्रत्यागम असत्य;
        यदि (nodep->type() != अधस्तनम्_Ifp->type()) प्रत्यागम असत्य;
        यदि (Ast_ग्रन्थिः::afterCommentp(अधस्तनम्_Ifp->elsesp())) प्रत्यागम असत्य;
        प्रत्यागम सत्य;
    }
    तार्किक ifConcatMergeableBiop(ध्रुव Ast_ग्रन्थिः* nodep) {
        प्रत्यागम (VN_IS(nodep, And) || VN_IS(nodep, Or) || VN_IS(nodep, Xor));
    }
    तार्किक ifAdjacentSel(ध्रुव AstSel* lhsp, ध्रुव AstSel* rhsp) {
        यदि (!v3Global.opt.fAssemble()) प्रत्यागम असत्य;  // opt disabled
        यदि (!lhsp || !rhsp) प्रत्यागम असत्य;
        ध्रुव Ast_ग्रन्थिः* ध्रुव lfromp = lhsp->fromp();
        ध्रुव Ast_ग्रन्थिः* ध्रुव rfromp = rhsp->fromp();
        यदि (!lfromp || !rfromp || !lfromp->same_Gate_वृक्षः(rfromp)) प्रत्यागम असत्य;
        ध्रुव AstConst* ध्रुव lstart = VN_CAST(lhsp->lsbp(), Const);
        ध्रुव AstConst* ध्रुव rstart = VN_CAST(rhsp->lsbp(), Const);
        यदि (!lstart || !rstart) प्रत्यागम असत्य;  // too complicated
        ध्रुव पूर्णाङ्क rend = (rstart->गन्तव्यम्_SInt() + rhsp->विस्तारः_Const());
        प्रत्यागम (rend == lstart->गन्तव्यम्_SInt());
    }
    तार्किक ifMergeAdjacent(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) {
        // Called by concatmergeable to determine if {lhsp, rhsp} make sense
        यदि (!v3Global.opt.fAssemble()) प्रत्यागम असत्य;  // opt disabled
        // Two same varref
        यदि (operandsSame(lhsp, rhsp)) प्रत्यागम सत्य;
        ध्रुव AstSel* ध्रुव lselp = VN_CAST(lhsp, Sel);
        ध्रुव AstSel* ध्रुव rselp = VN_CAST(rhsp, Sel);
        यदि (!lselp && !rselp) प्रत्यागम असत्य;
        यदि (lselp && !VN_IS(lselp->lsbp(), Const)) प्रत्यागम असत्य;
        यदि (rselp && !VN_IS(rselp->lsbp(), Const)) प्रत्यागम असत्य;
        // a[i:j] {a[j-1:k], b}
        यदि (lselp && !rselp && VN_IS(rhsp, Concat))
            प्रत्यागम ifMergeAdjacent(lhsp, VN_AS(rhsp, Concat)->lhsp());
        // {b, a[j:k]} a[k-1:i]
        यदि (rselp && !lselp && VN_IS(lhsp, Concat))
            प्रत्यागम ifMergeAdjacent(VN_AS(lhsp, Concat)->rhsp(), rhsp);
        // a a[msb:j]
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lfromp = lselp                                ? lselp->fromp()
                                          : lhsp->same_Gate_वृक्षः(rselp->fromp()) ? lhsp
                                                                               : शून्यसूचकः;
        // a[i:0] a
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rfromp = rselp                                ? rselp->fromp()
                                          : rhsp->same_Gate_वृक्षः(lselp->fromp()) ? rhsp
                                                                               : शून्यसूचकः;
        यदि (!lfromp || !rfromp || !lfromp->same_Gate_वृक्षः(rfromp)) प्रत्यागम असत्य;

        ध्रुव int32_t lstart = lselp ? lselp->lsbConst() : 0;
        ध्रुव int32_t rstart = rselp ? rselp->lsbConst() : 0;
        ध्रुव int32_t rend = rstart + (rselp ? rselp->विस्तारः_Const() : rhsp->विस्तारः());
        // a[i:j] a[j-1:k]
        यदि (rend == lstart) प्रत्यागम सत्य;
        // a[i:0] a[msb:j]
        यदि (rend == rfromp->विस्तारः() && lstart == 0) प्रत्यागम सत्य;
        प्रत्यागम असत्य;
    }
    तार्किक concatMergeable(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, अचिह्नित गभीरता) {
        // Determine if {a OP b, c OP d} => {a, c} OP {b, d} is advantageous
        यदि (!v3Global.opt.fAssemble()) प्रत्यागम असत्य;  // opt disabled
        यदि (lhsp->type() != rhsp->type()) प्रत्यागम असत्य;
        यदि (!ifConcatMergeableBiop(lhsp)) प्रत्यागम असत्य;
        यदि (गभीरता > CONCAT_MERGABLE_MAX_गभीरता) प्रत्यागम असत्य;  // As worse case O(n^2) algorithm

        ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव lp = VN_CAST(lhsp, ग्रन्थिः_Biop);
        ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव rp = VN_CAST(rhsp, ग्रन्थिः_Biop);
        यदि (!lp || !rp) प्रत्यागम असत्य;
        // {a[]&b[], a[]&b[]}
        ध्रुव तार्किक lad = ifMergeAdjacent(lp->lhsp(), rp->lhsp());
        ध्रुव तार्किक rad = ifMergeAdjacent(lp->rhsp(), rp->rhsp());
        यदि (lad && rad) प्रत्यागम सत्य;
        // {a[] & b[]&c[], a[] & b[]&c[]}
        यदि (lad && concatMergeable(lp->rhsp(), rp->rhsp(), गभीरता + 1)) प्रत्यागम सत्य;
        // {a[]&b[] & c[], a[]&b[] & c[]}
        यदि (rad && concatMergeable(lp->lhsp(), rp->lhsp(), गभीरता + 1)) प्रत्यागम सत्य;
        // {(a[]&b[])&(c[]&d[]), (a[]&b[])&(c[]&d[])}
        यदि (concatMergeable(lp->lhsp(), rp->lhsp(), गभीरता + 1)
            && concatMergeable(lp->rhsp(), rp->rhsp(), गभीरता + 1)) {
            प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }
    स्थिर तार्किक operands_Same_विस्तारः(ध्रुव Ast_ग्रन्थिः* lhsp, ध्रुव Ast_ग्रन्थिः* rhsp) {
        प्रत्यागम lhsp->विस्तारः() == rhsp->विस्तारः();
    }

    //----------------------------------------
    // Constant Replacement functions.
    // These all take a node, delete its tree, and replaces it with a constant

    शून्य replace_अङ्कः(Ast_ग्रन्थिः* oldp, ध्रुव V3_सङ्ख्या& अङ्कः, ध्रुव string& origParamName = "") {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        UASSERT_OBJ(!(VN_IS(oldp, Const) && !VN_AS(oldp, Const)->अङ्कः().isFourState()), oldp,
                    "Already constant??");
        AstConst* ध्रुव newp = नूतन AstConst{oldp->fileline(), अङ्कः};
        यदि (!origParamName.रिक्तानि()) newp->origParamName(origParamName);
        oldp->replaceWithKeepDType(newp);
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        VL_DO_DANGLING(प्रक्षिप_Deletep(oldp), oldp);
    }
    शून्य replace_अङ्कः(Ast_ग्रन्थिः* nodep, uint32_t val) {
        ध्रुव V3_सङ्ख्या अङ्कः{nodep, nodep->विस्तारः(), val};
        VL_DO_DANGLING(replace_अङ्कः(nodep, अङ्कः), nodep);
    }
    शून्य replace_अङ्कः_Signed(Ast_ग्रन्थिः_Biop* nodep, uint32_t val) {
        // We allow both sides to be constant, as one may have come from
        // parameter propagation, etc.
        यदि (m_warn && !(VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const))) {
            nodep->v3warn(UNSIGNED, "Comparison is constant due to unsigned arithmetic");
        }
        VL_DO_DANGLING(replace_अङ्कः(nodep, val), nodep);
    }
    शून्य replace_अङ्कः_Limited(Ast_ग्रन्थिः_Biop* nodep, uint32_t val) {
        // Avoids gcc warning about same
        यदि (m_warn) nodep->v3warn(CMPCONST, "Comparison is constant due to limited range");
        VL_DO_DANGLING(replace_अङ्कः(nodep, val), nodep);
    }
    शून्य replaceZero(Ast_ग्रन्थिः* nodep) { VL_DO_DANGLING(replace_अङ्कः(nodep, 0), nodep); }
    शून्य replace_Zero_Chk_विशुद्धम्(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* checkp) {
        // For example, "0 * n" -> 0 if n has no side effects
        // Else strength reduce it to 0 & n.
        // If ever change the operation note AstAnd rule specially ignores this created pattern
        यदि (checkp->is_विशुद्धम्()) {
            VL_DO_DANGLING(replace_अङ्कः(nodep, 0), nodep);
        } अन्यथा {
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstAnd{nodep->fileline(), नूतन AstConst{nodep->fileline(), 0},
                                             checkp->unlink_Fr_पृष्ठभागः()};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }
    शून्य replaceAllOnes(Ast_ग्रन्थिः* nodep) {
        V3_सङ्ख्या ones{nodep, nodep->विस्तारः(), 0};
        ones.समुच्चयः_आवरणम्(nodep->विस्तारः());
        VL_DO_DANGLING(replace_अङ्कः(nodep, ones), nodep);
    }
    शून्य replaceConst(Ast_ग्रन्थिः_Uniop* nodep) {
        V3_सङ्ख्या numv{nodep, nodep->विस्तारः_Min_V()};
        nodep->सङ्ख्या_सञ्चालय(numv, const_अङ्कः_V(nodep->lhsp()));
        ध्रुव V3_सङ्ख्या& अङ्कः = गन्तव्यम्_अङ्कः_C(nodep, numv);
        UINFO(4, "UNICONST -> " << अङ्कः);
        VL_DO_DANGLING(replace_अङ्कः(nodep, अङ्कः), nodep);
    }
    शून्य replaceConst(Ast_ग्रन्थिः_Biop* nodep) {
        V3_सङ्ख्या numv{nodep, nodep->विस्तारः_Min_V()};
        nodep->सङ्ख्या_सञ्चालय(numv, const_अङ्कः_V(nodep->lhsp()), const_अङ्कः_V(nodep->rhsp()));
        ध्रुव V3_सङ्ख्या& अङ्कः = गन्तव्यम्_अङ्कः_C(nodep, numv);
        UINFO(4, "BICONST -> " << अङ्कः);
        VL_DO_DANGLING(replace_अङ्कः(nodep, अङ्कः), nodep);
    }
    शून्य replaceConst(Ast_ग्रन्थिः_Triop* nodep) {
        V3_सङ्ख्या numv{nodep, nodep->विस्तारः_Min_V()};
        nodep->सङ्ख्या_सञ्चालय(numv, const_अङ्कः_V(nodep->lhsp()), const_अङ्कः_V(nodep->rhsp()),
                             const_अङ्कः_V(nodep->thsp()));
        ध्रुव V3_सङ्ख्या& अङ्कः = गन्तव्यम्_अङ्कः_C(nodep, numv);
        UINFO(4, "TRICONST -> " << अङ्कः);
        VL_DO_DANGLING(replace_अङ्कः(nodep, अङ्कः), nodep);
    }
    शून्य replaceConst(Ast_ग्रन्थिः_Quadop* nodep) {
        V3_सङ्ख्या numv{nodep, nodep->विस्तारः_Min_V()};
        nodep->सङ्ख्या_सञ्चालय(numv, const_अङ्कः_V(nodep->lhsp()), const_अङ्कः_V(nodep->rhsp()),
                             const_अङ्कः_V(nodep->thsp()), const_अङ्कः_V(nodep->fhsp()));
        ध्रुव V3_सङ्ख्या& अङ्कः = गन्तव्यम्_अङ्कः_C(nodep, numv);
        UINFO(4, "QUADCONST -> " << अङ्कः);
        VL_DO_DANGLING(replace_अङ्कः(nodep, अङ्कः), nodep);
    }

    शून्य replaceConstString(Ast_ग्रन्थिः* oldp, ध्रुव string& अङ्कः) {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        Ast_ग्रन्थिः* ध्रुव newp = नूतन AstConst{oldp->fileline(), AstConst::String{}, अङ्कः};
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        oldp->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(oldp), oldp);
    }
    //----------------------------------------
    // Replacement functions.
    // These all take a node and replace it with something else

    शून्य replaceWChild(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* childp) {
        // NODE(..., CHILD(...)) -> CHILD(...)
        childp->unlink_Fr_पृष्ठभागः_With_अग्र();
        // If replacing a SEL for example, the data type comes from the parent (is less wide).
        // This may adversely affect the operation of the node being replaced.
        nodep->replaceWithKeepDType(childp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य replaceWChildBool(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* childp) {
        // NODE(..., CHILD(...)) -> REDOR(CHILD(...))
        childp->unlink_Fr_पृष्ठभागः();
        यदि (childp->width1()) {
            nodep->replaceWithKeepDType(childp);
        } अन्यथा {
            nodep->replaceWithKeepDType(नूतन AstRedOr{childp->fileline(), childp});
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    //! Replace a ternary node with its RHS after iterating
    //! Used with short-circuiting, where the RHS has not yet been iterated.
    शून्य replaceWIteratedRhs(Ast_ग्रन्थिः_Triop* nodep) {
        यदि (Ast_ग्रन्थिः* ध्रुव rhsp = nodep->rhsp()) iterate_And_अग्र_Null(rhsp);
        replaceWChild(nodep, nodep->rhsp());  // May have changed
    }

    //! Replace a ternary node with its THS after iterating
    //! Used with short-circuiting, where the THS has not yet been iterated.
    शून्य replaceWIteratedThs(Ast_ग्रन्थिः_Triop* nodep) {
        यदि (Ast_ग्रन्थिः* ध्रुव thsp = nodep->thsp()) iterate_And_अग्र_Null(thsp);
        replaceWChild(nodep, nodep->thsp());  // May have changed
    }
    शून्य replaceWLhs(Ast_ग्रन्थिः_Uniop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    शून्य replaceWLhs(Ast_ग्रन्थिः_Biop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    शून्य replaceWRhs(Ast_ग्रन्थिः_Biop* nodep) {
        // Keep RHS, remove LHS
        replaceWChild(nodep, nodep->rhsp());
    }
    शून्य replaceWLhsBool(Ast_ग्रन्थिः_Biop* nodep) { replaceWChildBool(nodep, nodep->lhsp()); }
    शून्य replaceWRhsBool(Ast_ग्रन्थिः_Biop* nodep) { replaceWChildBool(nodep, nodep->rhsp()); }
    शून्य replaceAsv(Ast_ग्रन्थिः_Biop* nodep) {
        // BIASV(CONSTa, BIASV(CONSTb, c)) -> BIASV( BIASV_CONSTED(a,b), c)
        // BIASV(SAMEa,  BIASV(SAMEb, c))  -> BIASV( BIASV(SAMEa,SAMEb), c)
        // UINFOTREE(1, nodep, "", "repAsvConst_old");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ap = nodep->lhsp();
        Ast_ग्रन्थिः_Biop* ध्रुव rp = VN_AS(nodep->rhsp(), ग्रन्थिः_Biop);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bp = rp->lhsp();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp = rp->rhsp();
        ap->unlink_Fr_पृष्ठभागः();
        bp->unlink_Fr_पृष्ठभागः();
        cp->unlink_Fr_पृष्ठभागः();
        rp->unlink_Fr_पृष्ठभागः();
        nodep->lhsp(rp);
        nodep->rhsp(cp);
        rp->lhsp(ap);
        rp->rhsp(bp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        यदि (VN_IS(rp->lhsp(), Const) && VN_IS(rp->rhsp(), Const)) replaceConst(rp);
        iterate(nodep);  // Proceed to fixed point
    }
    शून्य replaceAsvLUp(Ast_ग्रन्थिः_Biop* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r))
        Ast_ग्रन्थिः_Biop* ध्रुव lp = VN_AS(nodep->lhsp()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_Biop);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव llp = lp->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lrp = lp->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
        nodep->lhsp(llp);
        nodep->rhsp(lp);
        lp->lhsp(lrp);
        lp->rhsp(rp);
        lp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    शून्य replaceAsvRUp(Ast_ग्रन्थिः_Biop* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr))
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_Biop* ध्रुव rp = VN_AS(nodep->rhsp()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_Biop);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rlp = rp->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rrp = rp->rhsp()->unlink_Fr_पृष्ठभागः();
        nodep->lhsp(rlp);
        nodep->rhsp(rp);
        rp->lhsp(lp);
        rp->rhsp(rrp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    शून्य replaceAndOr(Ast_ग्रन्थिः_Biop* nodep) {
        //  OR  (AND (CONSTll,lr), AND(CONSTrl==ll,rr))    -> AND (CONSTll, OR(lr,rr))
        //  OR  (AND (CONSTll,lr), AND(CONSTrl,    rr=lr)) -> AND (OR(ll,rl), rr)
        // nodep ^lp  ^llp   ^lrp  ^rp  ^rlp       ^rrp
        // (Or/And may also be reversed)
        Ast_ग्रन्थिः_Biop* ध्रुव lp = VN_AS(nodep->lhsp()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_Biop);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव llp = lp->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lrp = lp->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_Biop* ध्रुव rp = VN_AS(nodep->rhsp()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_Biop);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rlp = rp->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rrp = rp->rhsp()->unlink_Fr_पृष्ठभागः();
        nodep->replaceWithKeepDType(lp);
        यदि (operandsSame(llp, rlp)) {
            lp->lhsp(llp);
            lp->rhsp(nodep);
            lp->dtypeFrom(nodep);
            nodep->lhsp(lrp);
            nodep->rhsp(rrp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(rp), rp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(rlp), rlp);
        } अन्यथा यदि (operandsSame(lrp, rrp)) {
            lp->lhsp(nodep);
            lp->rhsp(rrp);
            lp->dtypeFrom(nodep);
            nodep->lhsp(llp);
            nodep->rhsp(rlp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(rp), rp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(lrp), lrp);
        } अन्यथा {
            nodep->v3fatalSrc("replaceAndOr on something operandAndOrSame shouldn't have matched");
        }
        // UINFOTREE(1, nodep, "", "repAndOr_new");
    }
    शून्य replace_सरणम्_Same(Ast_ग्रन्थिः_Biop* nodep) {
        // Or(Shift(ll,CONSTlr),Shift(rl,CONSTrr==lr)) -> Shift(Or(ll,rl),CONSTlr)
        // (Or/And may also be reversed)
        Ast_ग्रन्थिः_Biop* ध्रुव lp = VN_AS(nodep->lhsp()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_Biop);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव llp = lp->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lrp = lp->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_Biop* ध्रुव rp = VN_AS(nodep->rhsp()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_Biop);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rlp = rp->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rrp = rp->rhsp()->unlink_Fr_पृष्ठभागः();
        nodep->replaceWithKeepDType(lp);
        lp->lhsp(nodep);
        lp->rhsp(lrp);
        nodep->lhsp(llp);
        nodep->rhsp(rlp);
        nodep->dtypep(llp->dtypep());  // dtype of Biop is before shift.
        VL_DO_DANGLING(प्रक्षिप_Deletep(rp), rp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(rrp), rrp);
        // UINFOTREE(1, nodep, "", "repShiftSame_new");
    }
    शून्य replaceConcatSel(AstConcat* nodep) {
        // {a[1], a[0]} -> a[1:0]
        AstSel* ध्रुव lselp = VN_AS(nodep->lhsp()->unlink_Fr_पृष्ठभागः(), Sel);
        AstSel* ध्रुव rselp = VN_AS(nodep->rhsp()->unlink_Fr_पृष्ठभागः(), Sel);
        ध्रुव पूर्णाङ्क lstart = lselp->lsbConst();
        ध्रुव पूर्णाङ्क lwidth = lselp->विस्तारः_Const();
        ध्रुव पूर्णाङ्क rstart = rselp->lsbConst();
        ध्रुव पूर्णाङ्क rwidth = rselp->विस्तारः_Const();

        UASSERT_OBJ((rstart + rwidth) == lstart, nodep,
                    "tried to merge two selects which are not adjacent");
        AstSel* ध्रुव newselp = नूतन AstSel{
            lselp->fromp()->fileline(), rselp->fromp()->unlink_Fr_पृष्ठभागः(), rstart, lwidth + rwidth};
        UINFO(5, "merged two adjacent sel " << lselp << " and " << rselp << " to one " << newselp);

        nodep->replaceWithKeepDType(newselp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(lselp), lselp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(rselp), rselp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य replaceConcatMerge(AstConcat* nodep) {
        // {llp OP lrp, rlp OP rrp} => {llp, rlp} OP {lrp, rrp}, where OP = AND/OR/XOR
        Ast_ग्रन्थिः_Biop* ध्रुव lp = VN_AS(nodep->lhsp(), ग्रन्थिः_Biop);
        Ast_ग्रन्थिः_Biop* ध्रुव rp = VN_AS(nodep->rhsp(), ग्रन्थिः_Biop);
        यदि (concatMergeable(lp, rp, 0)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव llp = lp->lhsp();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lrp = lp->rhsp();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rlp = rp->lhsp();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rrp = rp->rhsp();
            AstConcat* ध्रुव newlp = नूतन AstConcat{rlp->fileline(), llp->clone_वृक्षः_विशुद्धम्(असत्य),
                                                   rlp->clone_वृक्षः_विशुद्धम्(असत्य)};
            AstConcat* ध्रुव newrp = नूतन AstConcat{rrp->fileline(), lrp->clone_वृक्षः_विशुद्धम्(असत्य),
                                                   rrp->clone_वृक्षः_विशुद्धम्(असत्य)};
            // use the lhs to replace the parent concat
            llp->replaceWith(newlp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(llp), llp);
            lrp->replaceWith(newrp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(lrp), lrp);
            lp->dtype_Chg_विस्तारः_Signed(newlp->विस्तारः(), newlp->विस्तारः(), VSigning::UNSIGNED);
            UINFO(5, "merged " << nodep);
            ++m_statConcatMerge;
            VL_DO_DANGLING(प्रक्षिप_Deletep(rp->unlink_Fr_पृष्ठभागः()), rp);
            nodep->replaceWithKeepDType(lp->unlink_Fr_पृष्ठभागः());
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            iterate(lp->lhsp());
            iterate(lp->rhsp());
        } अन्यथा {
            nodep->v3fatalSrc("tried to merge two Concat which are not adjacent");
        }
    }
    शून्य replaceExtend(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* arg0p) {
        // -> EXTEND(nodep)
        // like a AstExtend{$rhsp}, but we need to set the width correctly from base node
        arg0p->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp
            = (VN_IS(nodep, ExtendS)
                   ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtendS{nodep->fileline(), arg0p})
                   : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtend{nodep->fileline(), arg0p}));
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य replace_Pow_सरणम्(Ast_ग्रन्थिः_Biop* nodep) {  // Pow or PowS
        UINFO(5, "POW(2,b)->SHIFTL(1,b) " << nodep);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_सरणम्_L* ध्रुव newp
            = नूतन Ast_सरणम्_L{nodep->fileline(), नूतन AstConst{nodep->fileline(), 1}, rhsp};
        newp->lhsp()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य replace_Mul_सरणम्(AstMul* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "MUL(2^n,b)->SHIFTL(b,n) " << nodep);
        ध्रुव पूर्णाङ्क amount = VN_AS(nodep->lhsp(), Const)->अङ्कः().most_समुच्चयः_Bit_P1() - 1;  // 2^n->n+1
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव opp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_सरणम्_L* ध्रुव newp
            = नूतन Ast_सरणम्_L{nodep->fileline(), opp, नूतन AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य replace_Div_सरणम्(AstDiv* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "DIV(b,2^n)->SHIFTR(b,n) " << nodep);
        ध्रुव पूर्णाङ्क amount = VN_AS(nodep->rhsp(), Const)->अङ्कः().most_समुच्चयः_Bit_P1() - 1;  // 2^n->n+1
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव opp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_सरणम्_R* ध्रुव newp
            = नूतन Ast_सरणम्_R{nodep->fileline(), opp, नूतन AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य replaceModAnd(AstModDiv* nodep) {  // Mod, but not ModS as not simple shift
        UINFO(5, "MOD(b,2^n)->AND(b,2^n-1) " << nodep);
        ध्रुव पूर्णाङ्क amount = VN_AS(nodep->rhsp(), Const)->अङ्कः().most_समुच्चयः_Bit_P1() - 1;  // 2^n->n+1
        V3_सङ्ख्या आवरणम्{nodep, nodep->विस्तारः()};
        आवरणम्.समुच्चयः_आवरणम्(amount);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव opp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        AstAnd* ध्रुव newp
            = नूतन AstAnd{nodep->fileline(), opp, नूतन AstConst{nodep->fileline(), आवरणम्}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य replace_सरणम्_Op(Ast_ग्रन्थिः_Biop* nodep) {
        UINFO(5, "SHIFT(AND(a,b),CONST) with a/b special ->AND(SHIFT(a,CONST),SHIFT(b,CONST)) "
                     << nodep);
        ध्रुव पूर्णाङ्क विस्तारः = nodep->विस्तारः();
        ध्रुव पूर्णाङ्क विस्तारः_Min = nodep->विस्तारः_Min();
        VNRelinker handle;
        nodep->unlink_Fr_पृष्ठभागः(&handle);
        Ast_ग्रन्थिः_Biop* ध्रुव lhsp = VN_AS(nodep->lhsp(), ग्रन्थिः_Biop);
        lhsp->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव shiftp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ap = lhsp->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bp = lhsp->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_Biop* ध्रुव shift1p = nodep;
        Ast_ग्रन्थिः_Biop* ध्रुव shift2p = nodep->clone_वृक्षः(सत्य);
        shift1p->lhsp(ap);
        shift1p->rhsp(shiftp->clone_वृक्षः_विशुद्धम्(सत्य));
        shift2p->lhsp(bp);
        shift2p->rhsp(shiftp);
        Ast_ग्रन्थिः_Biop* ध्रुव newp = lhsp;
        newp->lhsp(shift1p);
        newp->rhsp(shift2p);
        newp->dtype_Chg_विस्तारः(विस्तारः, विस्तारः_Min);  // The new AND must have width of the original SHIFT
        handle.relink(newp);
        iterate(newp);  // Further reduce, either node may have more reductions.
    }
    शून्य replace_सरणम्_सरणम्(Ast_ग्रन्थिः_Biop* nodep) {
        UINFO(4, "SHIFT(SHIFT(a,s1),s2)->SHIFT(a,ADD(s1,s2)) " << nodep);
        UINFOTREE(9, nodep, "", "repShiftShift_old");
        Ast_ग्रन्थिः_Biop* ध्रुव lhsp = VN_AS(nodep->lhsp(), ग्रन्थिः_Biop);
        lhsp->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ap = lhsp->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव shift1p = lhsp->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव shift2p = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
        // Shift1p and shift2p may have different sizes, both are
        // self-determined so sum with infinite width
        यदि (nodep->type() == lhsp->type()) {
            ध्रुव पूर्णाङ्क shift1 = VN_AS(shift1p, Const)->गन्तव्यम्_UInt();
            ध्रुव पूर्णाङ्क shift2 = VN_AS(shift2p, Const)->गन्तव्यम्_UInt();
            ध्रुव पूर्णाङ्क newshift = shift1 + shift2;
            VL_DO_DANGLING(प्रक्षिप_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(प्रक्षिप_Deletep(shift2p), shift2p);
            nodep->lhsp(ap);
            nodep->rhsp(नूतन AstConst(nodep->fileline(), newshift));
            iterate(nodep);  // Further reduce, either node may have more reductions.
        } अन्यथा {
            // We know shift amounts are constant, but might be a mixed left/right shift
            पूर्णाङ्क shift1 = VN_AS(shift1p, Const)->गन्तव्यम्_UInt();
            यदि (VN_IS(lhsp, सरणम्_R)) shift1 = -shift1;
            पूर्णाङ्क shift2 = VN_AS(shift2p, Const)->गन्तव्यम्_UInt();
            यदि (VN_IS(nodep, सरणम्_R)) shift2 = -shift2;
            ध्रुव पूर्णाङ्क newshift = shift1 + shift2;
            VL_DO_DANGLING(प्रक्षिप_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(प्रक्षिप_Deletep(shift2p), shift2p);
            Ast_ग्रन्थिः_व्यञ्जकम्* newp;
            V3_सङ्ख्या mask1{nodep, nodep->विस्तारः()};
            V3_सङ्ख्या ones{nodep, nodep->विस्तारः()};
            ones.समुच्चयः_आवरणम्(nodep->विस्तारः());
            यदि (shift1 < 0) {
                mask1.op_सरणम्_R(ones, V3_सङ्ख्या(nodep, VL_IDATASIZE, -shift1));
            } अन्यथा {
                mask1.op_सरणम्_L(ones, V3_सङ्ख्या(nodep, VL_IDATASIZE, shift1));
            }
            V3_सङ्ख्या आवरणम्{nodep, nodep->विस्तारः()};
            यदि (shift2 < 0) {
                आवरणम्.op_सरणम्_R(mask1, V3_सङ्ख्या(nodep, VL_IDATASIZE, -shift2));
            } अन्यथा {
                आवरणम्.op_सरणम्_L(mask1, V3_सङ्ख्या(nodep, VL_IDATASIZE, shift2));
            }
            यदि (newshift < 0) {
                newp = नूतन Ast_सरणम्_R{nodep->fileline(), ap,
                                     नूतन AstConst(nodep->fileline(), -newshift)};
            } अन्यथा {
                newp = नूतन Ast_सरणम्_L{nodep->fileline(), ap,
                                     नूतन AstConst(nodep->fileline(), newshift)};
            }
            newp->dtypeFrom(nodep);
            newp = नूतन AstAnd{nodep->fileline(), newp, नूतन AstConst{nodep->fileline(), आवरणम्}};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            // UINFOTREE(1, newp, "", "repShiftShift_new");
            iterate(newp);  // Further reduce, either node may have more reductions.
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(lhsp), lhsp);
    }

    तार्किक replace_नियोजय_Multi_Sel(Ast_ग्रन्थिः_नियोजय* nodep) {
        // Multiple assignments to sequential bits can be concated
        // ASSIGN(SEL(a),aq), ASSIGN(SEL(a+1),bq) -> ASSIGN(SEL(a:b),CONCAT(aq,bq)
        // ie. assign var[2]=a, assign var[3]=b -> assign var[3:2]={b,a}

        // Skip if we're not const'ing an entire module (IE doing only one assign, etc)
        यदि (!m_modp) प्रत्यागम असत्य;
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ध्रुव sel1p = VN_CAST(nodep->lhsp(), Sel);
        यदि (!sel1p) प्रत्यागम असत्य;
        Ast_ग्रन्थिः_नियोजय* ध्रुव nextp = VN_CAST(nodep->nextp(), ग्रन्थिः_नियोजय);
        यदि (!nextp) प्रत्यागम असत्य;
        यदि (nodep->type() != nextp->type()) प्रत्यागम असत्य;
        ध्रुव AstSel* ध्रुव sel2p = VN_CAST(nextp->lhsp(), Sel);
        यदि (!sel2p) प्रत्यागम असत्य;
        AstVarRef* ध्रुव varref1p = VN_CAST(sel1p->fromp(), VarRef);
        यदि (!varref1p) प्रत्यागम असत्य;
        ध्रुव AstVarRef* ध्रुव varref2p = VN_CAST(sel2p->fromp(), VarRef);
        यदि (!varref2p) प्रत्यागम असत्य;
        यदि (!varref1p->same_Gate_वृक्षः(varref2p)) प्रत्यागम असत्य;
        ध्रुव AstConst* ध्रुव con1p = VN_CAST(sel1p->lsbp(), Const);
        यदि (!con1p) प्रत्यागम असत्य;
        ध्रुव AstConst* ध्रुव con2p = VN_CAST(sel2p->lsbp(), Const);
        यदि (!con2p) प्रत्यागम असत्य;
        // We need to make sure there's no self-references involved in either
        // assignment.  For speed, we only look 3 deep, then give up.
        यदि (!varNotReferenced(nodep->rhsp(), varref1p->varp())) प्रत्यागम असत्य;
        यदि (!varNotReferenced(nextp->rhsp(), varref2p->varp())) प्रत्यागम असत्य;
        // If a variable is marked split_var, access to the variable should not be merged.
        यदि (varref1p->varp()->attr_विभज_Var() || varref2p->varp()->attr_विभज_Var()) प्रत्यागम असत्य;
        // Swap?
        यदि ((con1p->गन्तव्यम्_SInt() != con2p->गन्तव्यम्_SInt() + sel2p->विस्तारः())
            && (con2p->गन्तव्यम्_SInt() != con1p->गन्तव्यम्_SInt() + sel1p->विस्तारः())) {
            प्रत्यागम असत्य;
        }
        ध्रुव तार्किक lsb_First_नियोजय = (con1p->गन्तव्यम्_UInt() < con2p->गन्तव्यम्_UInt());
        UINFO(4, "replaceAssignMultiSel " << nodep);
        UINFO(4, "                   && " << nextp);
        // UINFOTREE(1, nodep, "", "comb1");
        // UINFOTREE(1, nextp, "", "comb2");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhs1p = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhs2p = nextp->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_नियोजय* newp;
        यदि (lsb_First_नियोजय) {
            newp = nodep->cloneType(नूतन AstSel{sel1p->fileline(), varref1p->unlink_Fr_पृष्ठभागः(),
                                               sel1p->lsbConst(), sel1p->विस्तारः() + sel2p->विस्तारः()},
                                    नूतन AstConcat{rhs1p->fileline(), rhs2p, rhs1p});
        } अन्यथा {
            newp = nodep->cloneType(नूतन AstSel{sel1p->fileline(), varref1p->unlink_Fr_पृष्ठभागः(),
                                               sel2p->lsbConst(), sel1p->विस्तारः() + sel2p->विस्तारः()},
                                    नूतन AstConcat{rhs1p->fileline(), rhs1p, rhs2p});
        }
        // UINFOTREE(1, pnewp, "", "conew");
        nodep->replaceWith(newp);  // dypep intentionally changing
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nextp->unlink_Fr_पृष्ठभागः()), nextp);
        प्रत्यागम सत्य;
    }

    तार्किक replace_नियोजय_SFormat_F(Ast_ग्रन्थिः_नियोजय* nodep) {
        // Rewrite 'x = sformatf(...)' into 'sformat(x, ...)', which is more efficient
        // at the call site as it does not need to check if 'x' needs to be freed first.
        // This is a somewhat common pattern after lowering assertions.
        यदि (!VN_IS(nodep, नियोजय)) प्रत्यागम असत्य;
        AstVarRef* ध्रुव lhsp = VN_CAST(nodep->lhsp(), VarRef);
        यदि (!lhsp) प्रत्यागम असत्य;
        AstSFormatF* ध्रुव fmtp = VN_CAST(nodep->rhsp(), SFormatF);
        यदि (!fmtp) प्रत्यागम असत्य;
        lhsp->unlink_Fr_पृष्ठभागः();
        fmtp->unlink_Fr_पृष्ठभागः();
        nodep->replaceWith(नूतन AstSFormat{nodep->fileline(), fmtp, lhsp});
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        प्रत्यागम सत्य;
    }

    तार्किक varNotReferenced(Ast_ग्रन्थिः* nodep, AstVar* varp, पूर्णाङ्क level = 0) {
        // Return true if varp never referenced under node.
        // Return false if referenced, or tree too deep to be worth it, or side effects
        यदि (!nodep) प्रत्यागम सत्य;
        यदि (level > 2) प्रत्यागम असत्य;
        यदि (!nodep->is_विशुद्धम्()) प्रत्यागम असत्य;  // For example a $fgetc can't be reordered
        यदि (VN_IS(nodep, ग्रन्थिः_Var_Ref) && VN_AS(nodep, ग्रन्थिः_Var_Ref)->varp() == varp) प्रत्यागम असत्य;
        प्रत्यागम (varNotReferenced(nodep->nextp(), varp, level + 1)
                && varNotReferenced(nodep->op1p(), varp, level + 1)
                && varNotReferenced(nodep->op2p(), varp, level + 1)
                && varNotReferenced(nodep->op3p(), varp, level + 1)
                && varNotReferenced(nodep->op4p(), varp, level + 1));
    }

    तार्किक replace_ग्रन्थिः_नियोजय(Ast_ग्रन्थिः_नियोजय* nodep) {
        यदि (VN_IS(nodep->lhsp(), VarRef) && VN_IS(nodep->rhsp(), VarRef)
            && VN_AS(nodep->lhsp(), VarRef)->sameNoLvalue(VN_AS(nodep->rhsp(), VarRef))
            && !VN_IS(nodep, नियोजय_Dly)) {
            // X = X.  Quite pointless, though X <= X may override another earlier assignment
            यदि (VN_IS(nodep, नियोजय_W)) {
                यदि (!v3Global.opt.pedantic())
                    nodep->v3error(
                        "Wire inputs its own output, creating circular logic (wire x=x)");
                प्रत्यागम असत्य;  // Don't delete the assign, or V3Gate will freak out
            } अन्यथा {
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                प्रत्यागम सत्य;
            }
        } अन्यथा यदि (m_doV && VN_IS(nodep->lhsp(), Concat)) {
            तार्किक need_अस्थायी = असत्य;
            // A blocking assignment with an intra-assignment timing control evaluates its value,
            // then waits once (IEEE 1800-2023 9.4.5), here to assign a temporary for the parts
            ध्रुव तार्किक waits = VN_IS(nodep, नियोजय) && nodep->timingControlp();
            // Splitting copies the RHS once per part, so a large RHS would make the tree
            // grow with the product of the number of parts and the RHS size
            ध्रुव तार्किक need_अस्थायी_विशुद्धम् = !nodep->rhsp()->is_विशुद्धम्() || waits
                                      || nodep->rhsp()->isLargerThan(CONCAT_नियोजय_MAX_RHS_NODES);
            यदि (m_warn && !VN_IS(nodep, नियोजय_Dly)
                && !need_अस्थायी_विशुद्धम्) {  // Is same var on LHS and RHS?
                // If the rhs is not pure, we need a temporary variable anyway
                std::unordered_समुच्चयः<आकार_t> lhsVarIds;
                nodep->lhsp()->foreach([&](ध्रुव AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    lhsVarIds.emplace(m_ids(refp->varp()));
                });
                nodep->rhsp()->foreach([&](ध्रुव AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    यदि (lhsVarIds.गणनम्(m_ids(refp->varp()))) need_अस्थायी = सत्य;
                });
            }
            यदि (need_अस्थायी_विशुद्धम्) {
                // if the RHS is impure or large we need to create a temporary variable for it,
                // because further handling involves copying of the RHS.
                UINFO(4, "  ASSITEMPPURE " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp,rhs),
                //                                ASSIGN(lc1,SEL(temp,{size1})),
                //                                ASSIGN(lc2,SEL(temp,{size2}))

                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
                AstVar* ध्रुव अस्थायी_Purep = नूतन AstVar{rhsp->fileline(), VVarType::BLOCKTEMP,
                                                     m_concswap_नामानि.get(rhsp), rhsp->dtypep()};
                // Static, as read right after written, also in a fork
                अस्थायी_Purep->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->सङ्कलय_Stmtsp(अस्थायी_Purep);
                AstVarRef* ध्रुव अस्थायी_विशुद्धम्_Refp
                    = नूतन AstVarRef{rhsp->fileline(), अस्थायी_Purep, VAccess::WRITE};
                Ast_ग्रन्थिः_नियोजय* ध्रुव asnp
                    = VN_IS(nodep, नियोजय_Dly)
                          ? नूतन Ast_नियोजय{nodep->fileline(), अस्थायी_विशुद्धम्_Refp, rhsp}
                          : nodep->cloneType(अस्थायी_विशुद्धम्_Refp, rhsp);
                nodep->सङ्कलय_अत्र_This_As_अग्र(asnp);
                nodep->rhsp(नूतन AstVarRef{rhsp->fileline(), अस्थायी_Purep, VAccess::READ});
                // Only the assignment of the temporary waits
                यदि (waits) प्रक्षिप_Deletep(nodep->timingControlp()->unlink_Fr_पृष्ठभागः());
            } अन्यथा यदि (need_अस्थायी) {
                // The first time we constify, there may be the same variable on the LHS
                // and RHS.  In that case, we must use temporaries, or {a,b}={b,a} will break.
                UINFO(4, "  ASSITEMP " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp1,SEL(rhs,{size})),
                //                                ASSIGN(temp2,SEL(newrhs,{size}))
                //                                ASSIGN(lc1,temp1),
                //                                ASSIGN(lc2,temp2)
            } अन्यथा {
                UINFO(4, "  ASSI " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(lc1,SEL(rhs,{size})),
                //                                ASSIGN(lc2,SEL(newrhs,{size}))
            }
            // The NBA evaluates its intra-assignment delay as it executes (IEEE 1800-2023 4.9.4),
            // so once for the NBAs to all parts
            AstDelay* ध्रुव delayp
                = VN_IS(nodep, नियोजय_Dly) ? VN_CAST(nodep->timingControlp(), Delay) : शून्यसूचकः;
            यदि (delayp && !delayp->lhsp()->is_विशुद्धम्()) {
                File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = delayp->lhsp()->unlink_Fr_पृष्ठभागः();
                AstVar* ध्रुव tempp = नूतन AstVar{flp, VVarType::BLOCKTEMP,
                                                 m_concswap_नामानि.get(valuep), valuep->dtypep()};
                // Static, as read right after written, when the NBAs to the parts start waiting
                tempp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->सङ्कलय_Stmtsp(tempp);
                nodep->सङ्कलय_अत्र_This_As_अग्र(
                    नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, tempp, VAccess::WRITE}, valuep});
                delayp->lhsp(नूतन AstVarRef{flp, tempp, VAccess::READ});
            }
            UINFOTREE(9, nodep, "", "Ass_old");
            // Unlink the stuff
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lc1p = VN_AS(nodep->lhsp(), Concat)->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lc2p = VN_AS(nodep->lhsp(), Concat)->rhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव conp = VN_AS(nodep->lhsp(), Concat)->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhs2p = rhsp->clone_वृक्षः_विशुद्धम्(असत्य);
            // Calc widths
            ध्रुव पूर्णाङ्क lsb2 = 0;
            ध्रुव पूर्णाङ्क msb2 = lsb2 + lc2p->विस्तारः() - 1;
            ध्रुव पूर्णाङ्क lsb1 = msb2 + 1;
            ध्रुव पूर्णाङ्क msb1 = lsb1 + lc1p->विस्तारः() - 1;
            UASSERT_OBJ(msb1 == (conp->विस्तारः() - 1), nodep, "Width calc mismatch");
            // Form ranges
            AstSel* ध्रुव sel1p = नूतन AstSel{conp->fileline(), rhsp, lsb1, msb1 - lsb1 + 1};
            AstSel* ध्रुव sel2p = नूतन AstSel{conp->fileline(), rhs2p, lsb2, msb2 - lsb2 + 1};
            // Make new assigns of same flavor as old one
            //*** Not cloneTree; just one node.
            Ast_ग्रन्थिः_नियोजय* newp = शून्यसूचकः;
            यदि (!need_अस्थायी) {
                Ast_ग्रन्थिः_नियोजय* ध्रुव asn1ap = nodep->cloneType(lc1p, sel1p);
                Ast_ग्रन्थिः_नियोजय* ध्रुव asn2ap = nodep->cloneType(lc2p, sel2p);
                asn1ap->dtypeFrom(sel1p);
                asn2ap->dtypeFrom(sel2p);
                newp = Ast_ग्रन्थिः::सङ्कलय_अग्र(newp, asn1ap);
                newp = Ast_ग्रन्थिः::सङ्कलय_अग्र(newp, asn2ap);
            } अन्यथा {
                UASSERT_OBJ(m_modp, nodep, "Not under module");
                UASSERT_OBJ(m_globalPass, nodep,
                            "Should not reach here when not invoked on whole AstNetlist");
                // We could create just one temp variable, but we'll get better optimization
                // if we make one per term.
                AstVar* ध्रुव temp1p
                    = नूतन AstVar{sel1p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswap_नामानि.get(sel1p), VFlagLogicPacked{}, msb1 - lsb1 + 1};
                AstVar* ध्रुव temp2p
                    = नूतन AstVar{sel2p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswap_नामानि.get(sel2p), VFlagLogicPacked{}, msb2 - lsb2 + 1};
                m_modp->सङ्कलय_Stmtsp(temp1p);
                m_modp->सङ्कलय_Stmtsp(temp2p);
                Ast_ग्रन्थिः_नियोजय* ध्रुव asn1ap = nodep->cloneType(
                    नूतन AstVarRef{sel1p->fileline(), temp1p, VAccess::WRITE}, sel1p);
                Ast_ग्रन्थिः_नियोजय* ध्रुव asn2ap = nodep->cloneType(
                    नूतन AstVarRef{sel2p->fileline(), temp2p, VAccess::WRITE}, sel2p);
                Ast_ग्रन्थिः_नियोजय* ध्रुव asn1bp = nodep->cloneType(
                    lc1p, नूतन AstVarRef{sel1p->fileline(), temp1p, VAccess::READ});
                Ast_ग्रन्थिः_नियोजय* ध्रुव asn2bp = nodep->cloneType(
                    lc2p, नूतन AstVarRef{sel2p->fileline(), temp2p, VAccess::READ});
                asn1ap->dtypeFrom(temp1p);
                asn1bp->dtypeFrom(temp1p);
                asn2ap->dtypeFrom(temp2p);
                asn2bp->dtypeFrom(temp2p);
                // This order matters
                newp = Ast_ग्रन्थिः::सङ्कलय_अग्र(newp, asn1ap);
                newp = Ast_ग्रन्थिः::सङ्कलय_अग्र(newp, asn2ap);
                newp = Ast_ग्रन्थिः::सङ्कलय_अग्र(newp, asn1bp);
                newp = Ast_ग्रन्थिः::सङ्कलय_अग्र(newp, asn2bp);
            }
            यदि (debug() >= 9 && newp) newp->dump_वृक्षः_And_अग्र(निर्गमः, "-     _new: ");
            nodep->सङ्कलय_अग्र_अत्र(newp);
            // Cleanup
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            VL_DO_DANGLING(प्रक्षिप_Deletep(conp), conp);
            // Further reduce, either node may have more reductions.
            प्रत्यागम सत्य;
        } अन्यथा यदि (m_doV && VN_IS(nodep->rhsp(), Cvt_Packed_गन्तव्यम्_Array)
                   && अधस्तनम्_As_Fixed_Aggregate(nodep->lhsp()->dtypep())) {
            Ast_Cvt_Packed_गन्तव्यम्_Array* ध्रुव cvtp
                = VN_AS(nodep->rhsp(), Cvt_Packed_गन्तव्यम्_Array)->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* srcp = cvtp->fromp()->unlink_Fr_पृष्ठभागः();
            यदि (अधस्तनम्_As_Fixed_Aggregate(srcp->dtypep())) {
                srcp = packFixedAggregate(srcp);
            } अन्यथा यदि (Ast_ग्रन्थिः_प्रवाहः* ध्रुव streamp = VN_CAST(srcp, ग्रन्थिः_प्रवाहः)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव प्रवाहः_Srcp = streamp->lhsp();
                यदि (अधस्तनम्_As_Fixed_Aggregate(प्रवाहः_Srcp->dtypep())) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव packedp = packFixedAggregate(प्रवाहः_Srcp->unlink_Fr_पृष्ठभागः());
                    streamp->lhsp(packedp);
                    streamp->dtype_समुच्चयः_Bit_Or_Logic_Unsized(packedp->विस्तारः(), packedp->विस्तारः_Min(),
                                                       VSigning::UNSIGNED,
                                                       packedp->isFourstateOrNoDtype());
                }
            }
            VL_DO_DANGLING(प्रक्षिप_Deletep(cvtp), cvtp);
            replace_नियोजय_गन्तव्यम्_Fixed_Aggregate(nodep, nodep->lhsp()->unlink_Fr_पृष्ठभागः(), srcp);
            प्रत्यागम सत्य;
        } अन्यथा यदि (m_doV && VN_IS(nodep->rhsp(), प्रवाहः_R)
                   && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            // The right-streaming operator on rhs of assignment does not
            // change the order of bits. Eliminate stream but keep its lhsp.
            // Add a cast if needed.
            Ast_प्रवाहः_R* ध्रुव streamp = VN_AS(nodep->rhsp(), प्रवाहः_R)->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* srcp = streamp->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_DType* ध्रुव srcDTypep = srcp->dtypep()->skipRefp();
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            यदि (अधस्तनम्_As_Fixed_Aggregate(srcDTypep)) {
                srcp = packFixedAggregate(srcp);
            } अन्यथा यदि (VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)) {
                यदि (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    पूर्णाङ्क src_Element_द्वयङ्काः = 0;
                    यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elemDtp = srcDTypep->subDTypep()) {
                        src_Element_द्वयङ्काः = elemDtp->विस्तारः();
                    }
                    पूर्णाङ्क dst_Element_द्वयङ्काः = 0;
                    यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_द्वयङ्काः = elemDtp->विस्तारः();
                    }
                    srcp = नूतन Ast_Cvt_Array_गन्तव्यम्_Array{
                        srcp->fileline(), srcp,          nodep->dtypep(), असत्य, 1,
                        dst_Element_द्वयङ्काः,   src_Element_द्वयङ्काः};
                } अन्यथा {
                    srcp = नूतन Ast_Cvt_Array_गन्तव्यम्_Packed{srcp->fileline(), srcp, nodep->dtypep()};
                }
            } अन्यथा यदि (VN_IS(srcDTypep, UnpackArrayDType)) {
                srcp = नूतन Ast_Cvt_Array_गन्तव्यम्_Packed{srcp->fileline(), srcp, srcDTypep};
                // Handling the case where lhs is wider than rhs by inserting zeros. StreamL does
                // not require this, since the left streaming operator implicitly handles this.
                ध्रुव पूर्णाङ्क packed_द्वयङ्काः = nodep->lhsp()->विस्तारः_Min();
                ध्रुव पूर्णाङ्क unpack_द्वयङ्काः
                    = srcDTypep->arrayUnpackedElements() * srcDTypep->subDTypep()->विस्तारः_Min();
                ध्रुव uint32_t विस्थापनम् = packed_द्वयङ्काः > unpack_द्वयङ्काः ? packed_द्वयङ्काः - unpack_द्वयङ्काः : 0;
                srcp = नूतन Ast_सरणम्_L{srcp->fileline(), srcp,
                                     नूतन AstConst{srcp->fileline(), विस्थापनम्}, packed_द्वयङ्काः};
            }
            यदि (!VN_IS(dstDTypep, UnpackArrayDType) && !VN_IS(dstDTypep, QueueDType)
                && !VN_IS(dstDTypep, DynArrayDType)) {
                ध्रुव पूर्णाङ्क s_विस्तारः = srcp->विस्तारः();
                ध्रुव पूर्णाङ्क d_विस्तारः = nodep->lhsp()->विस्तारः();
                यदि (s_विस्तारः < d_विस्तारः) {
                    AstExtend* ध्रुव extendp = नूतन AstExtend{srcp->fileline(), srcp, d_विस्तारः};
                    srcp = नूतन Ast_सरणम्_L{
                        srcp->fileline(), extendp,
                        नूतन AstConst{srcp->fileline(), static_cast<uint32_t>(d_विस्तारः - s_विस्तारः)},
                        d_विस्तारः};
                }
            }
            nodep->rhsp(srcp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            प्रत्यागम सत्य;
        } अन्यथा यदि (m_doV && VN_IS(nodep->lhsp(), प्रवाहः_L)) {
            // Push the stream operator to the rhs of the assignment statement
            Ast_ग्रन्थिः_व्यञ्जकम्* streamp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव dstp = VN_AS(streamp, प्रवाहः_L)->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_DType* ध्रुव dstDTypep = dstp->dtypep()->skipRefp();
            Ast_ग्रन्थिः_व्यञ्जकम्* srcp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव srcDTypep = srcp->dtypep()->skipRefp();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamL reverses, so reverse=true)
            यदि ((VN_IS(srcDTypep, UnpackArrayDType) || VN_IS(srcDTypep, QueueDType)
                 || VN_IS(srcDTypep, DynArrayDType))
                && (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType))) {
                पूर्णाङ्क खण्डम्_आकार = 1;
                यदि (ध्रुव AstConst* ध्रुव constp
                    = VN_CAST(VN_AS(streamp, प्रवाहः_L)->rhsp(), Const)) {
                    खण्डम्_आकार = constp->गन्तव्यम्_SInt();
                    यदि (VL_UNLIKELY(खण्डम्_आकार <= 0)) खण्डम्_आकार = 1;
                }
                पूर्णाङ्क src_Element_द्वयङ्काः = 0;
                यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elemDtp = srcDTypep->subDTypep()) {
                    src_Element_द्वयङ्काः = elemDtp->विस्तारः();
                }
                पूर्णाङ्क dst_Element_द्वयङ्काः = 0;
                यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elemDtp = dstDTypep->subDTypep()) {
                    dst_Element_द्वयङ्काः = elemDtp->विस्तारः();
                }
                nodep->lhsp(dstp);
                nodep->rhsp(नूतन Ast_Cvt_Array_गन्तव्यम्_Array{srcp->fileline(), srcp, dstDTypep, सत्य,
                                                   खण्डम्_आकार, dst_Element_द्वयङ्काः, src_Element_द्वयङ्काः});
                nodep->dtypep(dstDTypep);
                VL_DO_DANGLING(प्रक्षिप_Deletep(streamp), streamp);
                प्रत्यागम सत्य;
            }
            यदि (अधस्तनम्_As_Fixed_Aggregate(srcDTypep)) srcp = packFixedAggregate(srcp);
            ध्रुव पूर्णाङ्क s_विस्तारः = srcp->विस्तारः();
            ध्रुव पूर्णाङ्क d_विस्तारः = dstp->विस्तारः();
            // Connect the rhs to the stream operator and update its width
            VN_AS(streamp, प्रवाहः_L)->lhsp(srcp);
            यदि (VN_IS(srcDTypep, DynArrayDType) || VN_IS(srcDTypep, QueueDType)
                || VN_IS(srcDTypep, UnpackArrayDType)) {
                streamp->dtype_समुच्चयः_प्रवाहः();
            } अन्यथा {
                streamp->dtype_समुच्चयः_Bit_Or_Logic_Unsized(srcp->विस्तारः(), srcp->विस्तारः_Min(),
                                                   VSigning::UNSIGNED,
                                                   srcp->isFourstateOrNoDtype());
            }
            यदि (VN_IS(dstDTypep, UnpackArrayDType)) {
                streamp = नूतन Ast_Cvt_Packed_गन्तव्यम्_Array{nodep->fileline(), streamp, dstDTypep};
            } अन्यथा {
                यदि (d_विस्तारः == 0) {
                    streamp = नूतन Ast_Cvt_Packed_गन्तव्यम्_Array{nodep->fileline(), streamp, dstDTypep};
                } अन्यथा यदि (s_विस्तारः >= d_विस्तारः) {
                    streamp = नूतन AstSel{streamp->fileline(), streamp, s_विस्तारः - d_विस्तारः, d_विस्तारः};
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(streamp);
            nodep->dtypep(dstDTypep);
            प्रत्यागम सत्य;
        } अन्यथा यदि (m_doV && VN_IS(nodep->lhsp(), प्रवाहः_R)) {
            // The right stream operator on lhs of assignment statement does
            // not reorder bits. However, if the rhs is wider than the lhs,
            // then we select bits from the left-most, not the right-most.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव streamp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव dstp = VN_AS(streamp, प्रवाहः_R)->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_DType* ध्रुव dstDTypep = dstp->dtypep()->skipRefp();
            Ast_ग्रन्थिः_व्यञ्जकम्* srcp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamR does not reverse, so reverse=false).
            // V3Width may have wrapped the source in CvtArrayToPacked; unwrap it.
            यदि (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* origSrcp = srcp;
                यदि (Ast_Cvt_Array_गन्तव्यम्_Packed* ध्रुव cvtp = VN_CAST(srcp, Cvt_Array_गन्तव्यम्_Packed)) {
                    origSrcp = cvtp->fromp();
                }
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव origSrcDTypep = origSrcp->dtypep()->skipRefp();
                यदि (VN_IS(origSrcDTypep, UnpackArrayDType) || VN_IS(origSrcDTypep, QueueDType)
                    || VN_IS(origSrcDTypep, DynArrayDType)) {
                    पूर्णाङ्क src_Element_द्वयङ्काः = 0;
                    यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elemDtp = origSrcDTypep->subDTypep()) {
                        src_Element_द्वयङ्काः = elemDtp->विस्तारः();
                    }
                    पूर्णाङ्क dst_Element_द्वयङ्काः = 0;
                    यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_द्वयङ्काः = elemDtp->विस्तारः();
                    }
                    यदि (VN_IS(srcp, Cvt_Array_गन्तव्यम्_Packed)) {
                        origSrcp = VN_AS(srcp, Cvt_Array_गन्तव्यम्_Packed)->fromp()->unlink_Fr_पृष्ठभागः();
                        VL_DO_DANGLING(प्रक्षिप_Deletep(srcp), srcp);
                        srcp = origSrcp;
                    }
                    // Descending unpacked arrays need element reversal
                    तार्किक reverse = असत्य;
                    यदि (ध्रुव AstUnpackArrayDType* ध्रुव unpackDtp
                        = VN_CAST(origSrcDTypep, UnpackArrayDType)) {
                        reverse = !unpackDtp->declRange().ascending();
                    }
                    nodep->lhsp(dstp);
                    nodep->rhsp(नूतन Ast_Cvt_Array_गन्तव्यम्_Array{srcp->fileline(), srcp, dstDTypep, reverse,
                                                       1, dst_Element_द्वयङ्काः, src_Element_द्वयङ्काः});
                    nodep->dtypep(dstDTypep);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(streamp), streamp);
                    प्रत्यागम सत्य;
                }
            }
            ध्रुव पूर्णाङ्क s_विस्तारः = srcp->विस्तारः();
            ध्रुव पूर्णाङ्क d_विस्तारः = dstp->विस्तारः();
            यदि (VN_IS(dstDTypep, UnpackArrayDType)) {
                ध्रुव पूर्णाङ्क dst_Bit_विस्तारः
                    = d_विस्तारः * VN_AS(dstDTypep, UnpackArrayDType)->arrayUnpackedElements();
                // Handling the case where rhs is wider than lhs. StreamL does not require this
                // since the combination of the left streaming operation and the implicit
                // truncation in VL_ASSIGN_UNPACK automatically selects the left-most bits.
                यदि (s_विस्तारः > dst_Bit_विस्तारः) {
                    srcp
                        = नूतन AstSel{streamp->fileline(), srcp, s_विस्तारः - dst_Bit_विस्तारः, dst_Bit_विस्तारः};
                }
                srcp = नूतन Ast_Cvt_Packed_गन्तव्यम्_Array{nodep->fileline(), srcp, dstDTypep};
            } अन्यथा {
                यदि (d_विस्तारः == 0) {
                    srcp = नूतन Ast_Cvt_Packed_गन्तव्यम्_Array{nodep->fileline(), srcp, dstDTypep};
                } अन्यथा यदि (s_विस्तारः >= d_विस्तारः) {
                    srcp = नूतन AstSel{streamp->fileline(), srcp, s_विस्तारः - d_विस्तारः, d_विस्तारः};
                } अन्यथा {
                    // Source narrower than destination: left-justify by shifting left.
                    // The right stream operator packs left-to-right, so remaining
                    // LSBs are zero-filled (IEEE 1800-2023 11.4.14.3).
                    यदि (!VN_IS(srcp->dtypep()->skipRefp(), QueueDType)) {
                        AstExtend* ध्रुव extendp = नूतन AstExtend{srcp->fileline(), srcp, d_विस्तारः};
                        srcp = नूतन Ast_सरणम्_L{
                            srcp->fileline(), extendp,
                            नूतन AstConst{srcp->fileline(), static_cast<uint32_t>(d_विस्तारः - s_विस्तारः)},
                            d_विस्तारः};
                    } अन्यथा {  // if it is a queue we dont need do do a shiftL
                        srcp = नूतन AstExtend{srcp->fileline(), srcp};
                    }
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(srcp);
            nodep->dtypep(dstDTypep);
            VL_DO_DANGLING(प्रक्षिप_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            प्रत्यागम सत्य;
        } अन्यथा यदि (m_doV && VN_IS(nodep->rhsp(), प्रवाहः_L)) {
            Ast_प्रवाहः_L* streamp = VN_AS(nodep->rhsp(), प्रवाहः_L);
            Ast_ग्रन्थिः_व्यञ्जकम्* srcp = streamp->lhsp();
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव srcDTypep = srcp->dtypep()->skipRefp();
            Ast_ग्रन्थिः_DType* ध्रुव dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            यदि (अधस्तनम्_As_Fixed_Aggregate(srcDTypep)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव packedp = packFixedAggregate(srcp->unlink_Fr_पृष्ठभागः());
                streamp->lhsp(packedp);
                streamp->dtype_समुच्चयः_Bit_Or_Logic_Unsized(packedp->विस्तारः(), packedp->विस्तारः_Min(),
                                                   VSigning::UNSIGNED,
                                                   packedp->isFourstateOrNoDtype());
                srcp = packedp;
            } अन्यथा यदि ((VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)
                        || VN_IS(srcDTypep, UnpackArrayDType))) {
                यदि (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    पूर्णाङ्क खण्डम्_आकार = 1;
                    यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(streamp->rhsp(), Const)) {
                        खण्डम्_आकार = constp->गन्तव्यम्_SInt();
                        यदि (VL_UNLIKELY(खण्डम्_आकार <= 0)) {
                            // Not reachable due to higher level checks when parsing stream
                            // operators commented out to not fail v3error-coverage-checks.
                            // nodep->v3error("Stream block size must be positive, got " <<
                            // blockSize); nodep->v3error("Stream block size must be positive, got
                            // " << blockSize);
                            खण्डम्_आकार = 1;
                        }
                    }
                    // Not reachable due to higher level checks when parsing stream operators
                    // commented out to not fail v3error-coverage-checks.
                    // else {
                    // nodep->v3error("Stream block size must be constant (got " <<
                    // streamp->rhsp()->prettyTypeName() << ")");
                    // }
                    पूर्णाङ्क src_Element_द्वयङ्काः = 0;
                    यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elemDtp = srcDTypep->subDTypep()) {
                        src_Element_द्वयङ्काः = elemDtp->विस्तारः();
                    }
                    पूर्णाङ्क dst_Element_द्वयङ्काः = 0;
                    यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_द्वयङ्काः = elemDtp->विस्तारः();
                    }
                    streamp->unlink_Fr_पृष्ठभागः();
                    srcp = नूतन Ast_Cvt_Array_गन्तव्यम्_Array{
                        srcp->fileline(), srcp->unlink_Fr_पृष्ठभागः(), dstDTypep,     सत्य,
                        खण्डम्_आकार,        dst_Element_द्वयङ्काः,       src_Element_द्वयङ्काः};
                    nodep->rhsp(srcp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(streamp), streamp);
                } अन्यथा {
                    streamp->lhsp(नूतन Ast_Cvt_Array_गन्तव्यम्_Packed{srcp->fileline(), srcp->unlink_Fr_पृष्ठभागः(),
                                                          dstDTypep});
                    streamp->dtypeFrom(dstDTypep);
                }
            }
        } अन्यथा यदि (m_doV && replace_नियोजय_Multi_Sel(nodep)) {
            प्रत्यागम सत्य;
        } अन्यथा यदि (replace_नियोजय_SFormat_F(nodep)) {
            प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    // Boolean replacements
    तार्किक सङ्कार्यम्_Bool_सरणम्(ध्रुव Ast_ग्रन्थिः* nodep) {
        // boolean test of AND(const,SHIFTR(x,const)) -> test of AND(SHIFTL(x,const), x)
        यदि (!VN_IS(nodep, And)) प्रत्यागम असत्य;
        यदि (!VN_IS(VN_AS(nodep, And)->lhsp(), Const)) प्रत्यागम असत्य;
        यदि (!VN_IS(VN_AS(nodep, And)->rhsp(), सरणम्_R)) प्रत्यागम असत्य;
        ध्रुव Ast_सरणम्_R* ध्रुव shiftp = VN_AS(VN_AS(nodep, And)->rhsp(), सरणम्_R);
        यदि (!VN_IS(shiftp->rhsp(), Const)) प्रत्यागम असत्य;
        यदि (static_cast<uint32_t>(nodep->विस्तारः()) <= VN_AS(shiftp->rhsp(), Const)->गन्तव्यम्_UInt()) {
            प्रत्यागम असत्य;
        }
        प्रत्यागम सत्य;
    }
    शून्य replace_Bool_सरणम्(Ast_ग्रन्थिः* nodep) {
        UINFOTREE(9, nodep, "", "bshft_old");
        AstConst* ध्रुव andConstp = VN_AS(VN_AS(nodep, And)->lhsp(), Const);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp
            = VN_AS(VN_AS(nodep, And)->rhsp(), सरणम्_R)->lhsp()->unlink_Fr_पृष्ठभागः();
        AstConst* ध्रुव सरणम्_Constp
            = VN_AS(VN_AS(VN_AS(nodep, And)->rhsp(), सरणम्_R)->rhsp(), Const);
        V3_सङ्ख्या val{andConstp, andConstp->विस्तारः()};
        val.op_सरणम्_L(andConstp->अङ्कः(), सरणम्_Constp->अङ्कः());
        AstAnd* ध्रुव newp
            = नूतन AstAnd{nodep->fileline(), नूतन AstConst{nodep->fileline(), val}, fromp};
        // widthMin no longer applicable if different C-expanded width
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        UINFOTREE(9, newp, "", "_new");
    }

    शून्य replaceWithSimulation(Ast_ग्रन्थिः* nodep) {
        SimulateVisitor simvis;
        // Run it - may be unoptimizable due to large for loop, etc
        simvis.mainParamEmulate(nodep);
        यदि (!simvis.optimizable()) {
            ध्रुव Ast_ग्रन्थिः* errorp = simvis.whyNotNodep();
            यदि (!errorp) errorp = nodep;
            nodep->v3error("Expecting expression to be constant, but can't determine constant for "
                           << nodep->prettyTypeName() << '\n'
                           << errorp->warnOther() << "... Location of non-constant "
                           << errorp->prettyTypeName() << ": " << simvis.why_Not_सन्देशः());
            VL_DO_DANGLING(replaceZero(nodep), nodep);
        } अन्यथा {
            // Fetch the result
            Ast_ग्रन्थिः* ध्रुव valuep = simvis.fetch_मानम्_Null(nodep);  // valuep is owned by Simulate
            UASSERT_OBJ(valuep, nodep, "No value returned from simulation");
            // Replace it
            Ast_ग्रन्थिः* ध्रुव newp = valuep->clone_वृक्षः(असत्य);
            newp->fileline(nodep->fileline());
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Simulate->" << newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }

    //----------------------------------------

    // VISITORS
    शून्य दर्शय(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        iterateChildrenBackwardsConst(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_concswap_नामानि.reset();
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstCFunc* nodep) override {
        // No ASSIGNW removals under funcs, we've long eliminated INITIALs
        // (We should perhaps rename the assignw's to just assigns)
        VL_RESTORER(m_wremove);
        m_wremove = असत्य;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstCLocalScope* nodep) override {
        iterateChildren(nodep);
        यदि (!nodep->stmtsp()) {
            nodep->unlink_Fr_पृष्ठभागः();
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }
    शून्य दर्शय(AstScope* nodep) override {
        // No ASSIGNW removals under scope, we've long eliminated INITIALs
        VL_RESTORER(m_wremove);
        VL_RESTORER(m_scopep);
        m_wremove = असत्य;
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    शून्य swapSides(Ast_ग्रन्थिः_Bi_Com* nodep) {
        // COMMUTATIVE({a},CONST) -> COMMUTATIVE(CONST,{a})
        // This simplifies later optimizations
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः_With_अग्र();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः_With_अग्र();
        nodep->lhsp(rhsp);
        nodep->rhsp(lhsp);
        iterate(nodep);  // Again?
    }

    तार्किक अन्तर्भवति_Member_Access_Recurse(Ast_ग्रन्थिः* ध्रुव nodep) {
        यदि (!nodep) प्रत्यागम असत्य;
        ध्रुव आकार_t id = m_ids(nodep);
        ध्रुव स्वचालित it = m_अन्तर्भवति_Member_Access.find(id);
        यदि (it != m_अन्तर्भवति_Member_Access.अन्तः_2()) प्रत्यागम it->second;
        ++m_statMemberAccessVisits;
        तार्किक परिणाम = असत्य;
        यदि (VN_IS(nodep, MemberSel) || VN_IS(nodep, विधानम्_Call) || VN_IS(nodep, CMethodCall)) {
            परिणाम = सत्य;
        } अन्यथा यदि (ध्रुव Ast_ग्रन्थिः_FTask_Ref* ध्रुव funcRefp = VN_CAST(nodep, ग्रन्थिः_FTask_Ref)) {
            यदि (अन्तर्भवति_Member_Access_Recurse(funcRefp->taskp())) परिणाम = सत्य;
        } अन्यथा यदि (ध्रुव Ast_ग्रन्थिः_CCall* ध्रुव funcRefp = VN_CAST(nodep, ग्रन्थिः_CCall)) {
            यदि (अन्तर्भवति_Member_Access_Recurse(funcRefp->funcp())) परिणाम = सत्य;
        } अन्यथा यदि (ध्रुव Ast_ग्रन्थिः_FTask* ध्रुव funcp = VN_CAST(nodep, ग्रन्थिः_FTask)) {
            // Assume that it has a member access
            यदि (funcp->recursive()) परिणाम = सत्य;
        } अन्यथा यदि (ध्रुव AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc)) {
            यदि (funcp->recursive()) परिणाम = सत्य;
        }
        यदि (!परिणाम) {
            परिणाम = अन्तर्भवति_Member_Access_Recurse(nodep->op1p())
                     || अन्तर्भवति_Member_Access_Recurse(nodep->op2p())
                     || अन्तर्भवति_Member_Access_Recurse(nodep->op3p())
                     || अन्तर्भवति_Member_Access_Recurse(nodep->op4p());
        }
        यदि (!परिणाम && !VN_IS(nodep, ग्रन्थिः_FTask)
            && !VN_IS(nodep, CFunc)  // don't enter into next function
            && अन्तर्भवति_Member_Access_Recurse(nodep->nextp())) {
            परिणाम = सत्य;
        }
        m_अन्तर्भवति_Member_Access.emplace(id, परिणाम);
        प्रत्यागम परिणाम;
    }

    तार्किक मेलनम्_Biop_गन्तव्यम्_Bitwise(Ast_ग्रन्थिः_Biop* ध्रुव nodep) {
        यदि (!m_convert_Logic_गन्तव्यम्_Bit) प्रत्यागम असत्य;
        यदि (!nodep->lhsp()->width1()) प्रत्यागम असत्य;
        यदि (!nodep->rhsp()->width1()) प्रत्यागम असत्य;
        यदि (!nodep->is_विशुद्धम्()) प्रत्यागम असत्य;
        यदि (अन्तर्भवति_Member_Access_Recurse(nodep)) प्रत्यागम असत्य;
        प्रत्यागम सत्य;
    }
    तार्किक मेलनम्_Concat_Rand(AstConcat* nodep) {
        //    CONCAT(RAND, RAND) - created by Chisel code
        AstRand* ध्रुव aRandp = VN_CAST(nodep->lhsp(), Rand);
        ध्रुव AstRand* ध्रुव bRandp = VN_CAST(nodep->rhsp(), Rand);
        यदि (!aRandp || !bRandp) प्रत्यागम असत्य;
        यदि (!aRandp->combinable(bRandp)) प्रत्यागम असत्य;
        UINFO(4, "Concat(Rand,Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlink_Fr_पृष्ठभागः());
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        प्रत्यागम सत्य;
    }
    तार्किक मेलनम्_Sel_Rand(AstSel* nodep) {
        //    SEL(RAND) - created by Chisel code
        AstRand* ध्रुव aRandp = VN_CAST(nodep->fromp(), Rand);
        यदि (!aRandp) प्रत्यागम असत्य;
        यदि (aRandp->seedp()) प्रत्यागम असत्य;
        UINFO(4, "Sel(Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlink_Fr_पृष्ठभागः());
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        प्रत्यागम सत्य;
    }
    तार्किक मेलनम्_गन्तव्यम्_String_NConst(Ast_गन्तव्यम्_String_N* nodep) {
        iterateChildren(nodep);
        यदि (ध्रुव AstConsArray* ध्रुव consp = VN_CAST(nodep->lhsp(), ConsArray)) {
            यदि (!(m_doExpensive || m_params)) प्रत्यागम असत्य;
            यदि (!consp->is_Constant_वृक्षः()) प्रत्यागम असत्य;
        } अन्यथा यदि (!VN_IS(nodep->lhsp(), Const)) {
            प्रत्यागम असत्य;
        }
        replaceWithSimulation(nodep);
        प्रत्यागम सत्य;
    }
    पूर्णाङ्क सङ्कार्यम्_Concat_Move(ध्रुव AstConcat* nodep) {
        //    CONCAT under concat  (See moveConcat)
        // Return value: true indicates to do it; 2 means move to LHS
        ध्रुव AstConcat* ध्रुव abConcp = VN_CAST(nodep->lhsp(), Concat);
        ध्रुव AstConcat* ध्रुव bcConcp = VN_CAST(nodep->rhsp(), Concat);
        यदि (!abConcp && !bcConcp) प्रत्यागम 0;
        यदि (bcConcp) {
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ap = nodep->lhsp();
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bp = bcConcp->lhsp();
            // If a+b == 32,64,96 etc, then we want to have a+b together on LHS
            यदि (VL_BITBIT_I(ap->विस्तारः() + bp->विस्तारः()) == 0) प्रत्यागम 2;  // Transform 2: to abConc
        } अन्यथा {  // abConcp
            // Unless lhs is already 32 bits due to above, reorder it
            यदि (VL_BITBIT_I(nodep->lhsp()->विस्तारः()) != 0) प्रत्यागम 1;  // Transform 1: to bcConc
        }
        प्रत्यागम 0;  // ok
    }
    शून्य moveConcat(AstConcat* nodep) {
        //    1: CONCAT(CONCAT({a},{b}),{c})  -> CONCAT({a},CONCAT({b}, {c}))
        // or 2: CONCAT({a}, CONCAT({b},{c})) -> CONCAT(CONCAT({a},{b}),{c})
        // Because the lhs of a concat needs a shift but the rhs doesn't,
        // putting additional CONCATs on the RHS leads to fewer assembler operations.
        // However, we'll end up with lots of wide moves if we make huge trees
        // like that, so on 32 bit boundaries, we'll do the opposite form.
        UINFO(4, "Move concat: " << nodep);
        यदि (सङ्कार्यम्_Concat_Move(nodep) > 1) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ap = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
            AstConcat* ध्रुव bcConcp = VN_AS(nodep->rhsp(), Concat);
            bcConcp->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bp = bcConcp->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp = bcConcp->rhsp()->unlink_Fr_पृष्ठभागः();
            AstConcat* ध्रुव abConcp = नूतन AstConcat{bcConcp->fileline(), ap, bp};
            nodep->lhsp(abConcp);
            nodep->rhsp(cp);
            // If bp was a concat, then we have this exact same form again!
            // Recurse rather then calling node->iterate to prevent 2^n recursion!
            यदि (सङ्कार्यम्_Concat_Move(abConcp)) moveConcat(abConcp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(bcConcp), bcConcp);
        } अन्यथा {
            AstConcat* ध्रुव abConcp = VN_AS(nodep->lhsp(), Concat);
            abConcp->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ap = abConcp->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bp = abConcp->rhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
            AstConcat* ध्रुव bcConcp = नूतन AstConcat{abConcp->fileline(), bp, cp};
            nodep->lhsp(ap);
            nodep->rhsp(bcConcp);
            यदि (सङ्कार्यम्_Concat_Move(bcConcp)) moveConcat(bcConcp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(abConcp), abConcp);
        }
    }

    // Special cases
    शून्य दर्शय(AstConst*) override {}  // Already constant

    शून्य दर्शय(AstCell* nodep) override {
        यदि (m_params) {
            iterate_And_अग्र_Null(nodep->paramsp());
        } अन्यथा {
            iterateChildren(nodep);
        }
    }
    शून्य दर्शय(AstClassOrPackageRef* nodep) override { iterateChildren(nodep); }

    शून्य दर्शय(Ast_मेलनम्_Masked* nodep) override {
        // Do not iterate the tables, they must be constant pool entries
        iterate(nodep->lhsp());
        यदि (AstConst* ध्रुव constp = VN_CAST(nodep->lhsp(), Const)) {
            replace_अङ्कः(nodep, Ast_मेलनम्_Masked::fold(constp->अङ्कः(), nodep->matchp()->varp()));
        }
    }

    शून्य दर्शय(AstPin* nodep) override { iterateChildren(nodep); }

    शून्य replaceLogEq(AstLogEq* nodep) {
        // LOGEQ(a,b) => AstLogAnd{AstLogOr{AstLogNot{a},b},AstLogOr{AstLogNot{b},a}}
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
        // Do exactly as IEEE says, might result in extra terms, so in future may do differently
        AstLogAnd* ध्रुव newp = नूतन AstLogAnd{
            nodep->fileline(),
            नूतन AstLogOr{nodep->fileline(), नूतन AstLogNot{nodep->fileline(), lhsp}, rhsp},
            नूतन AstLogOr{nodep->fileline(),
                         नूतन AstLogNot{nodep->fileline(), rhsp->clone_वृक्षः_विशुद्धम्(असत्य)},
                         lhsp->clone_वृक्षः_विशुद्धम्(असत्य)}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य replaceSelSel(AstSel* nodep) {
        // SEL(SEL({x},a,b),c,d) => SEL({x},a+c,d)
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ध्रुव belowp = VN_AS(nodep->fromp(), Sel);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = belowp->fromp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lsb1p = nodep->lsbp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lsb2p = belowp->lsbp()->unlink_Fr_पृष्ठभागः();
        // Eliminate lower range
        UINFO(4, "Elim Lower range: " << nodep);
        Ast_ग्रन्थिः_व्यञ्जकम्* newlsbp;
        यदि (VN_IS(lsb1p, Const) && VN_IS(lsb2p, Const)) {
            newlsbp = नूतन AstConst{lsb1p->fileline(),
                                   VN_AS(lsb1p, Const)->गन्तव्यम्_UInt() + VN_AS(lsb2p, Const)->गन्तव्यम्_UInt()};
            VL_DO_DANGLING(प्रक्षिप_Deletep(lsb1p), lsb1p);
            VL_DO_DANGLING(प्रक्षिप_Deletep(lsb2p), lsb2p);
        } अन्यथा {
            // Width is important, we need the width of the fromp's expression, not the
            // potentially smaller lsb1p's width, but don't insert a redundant AstExtend.
            // Note that due to some sloppiness in earlier passes, lsb1p might actually be wider,
            // so extend to the wider type.
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव widep = lsb1p->विस्तारः() > lsb2p->विस्तारः() ? lsb1p : lsb2p;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = widep->विस्तारः() > lsb2p->विस्तारः()
                                          ? नूतन AstExtend{lsb2p->fileline(), lsb2p}
                                          : lsb2p;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = widep->विस्तारः() > lsb1p->विस्तारः()
                                          ? नूतन AstExtend{lsb1p->fileline(), lsb1p}
                                          : lsb1p;
            lhsp->dtypeFrom(widep);
            rhsp->dtypeFrom(widep);
            newlsbp = नूतन Ast_सङ्कलय{lsb1p->fileline(), lhsp, rhsp};
            newlsbp->dtypeFrom(widep);
        }
        AstSel* ध्रुव newp = नूतन AstSel{nodep->fileline(), fromp, newlsbp, nodep->विस्तारः_Const()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य replaceSelConcat(AstSel* nodep) {
        // SEL(CONCAT(a,b),c,d) => SEL(a or b, . .)
        AstConcat* ध्रुव conp = VN_AS(nodep->fromp(), Concat);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव conLhsp = conp->lhsp();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव conRhsp = conp->rhsp();
        यदि (static_cast<पूर्णाङ्क>(nodep->lsbConst()) >= conRhsp->विस्तारः()) {
            conLhsp->unlink_Fr_पृष्ठभागः();
            AstSel* ध्रुव newp
                = नूतन AstSel{nodep->fileline(), conLhsp, nodep->lsbConst() - conRhsp->विस्तारः(),
                             nodep->विस्तारः_Const()};
            nodep->replaceWithKeepDType(newp);
        } अन्यथा यदि (static_cast<पूर्णाङ्क>(nodep->msbConst()) < conRhsp->विस्तारः()) {
            conRhsp->unlink_Fr_पृष्ठभागः();
            AstSel* ध्रुव newp
                = नूतन AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(), nodep->विस्तारः_Const()};
            nodep->replaceWithKeepDType(newp);
        } अन्यथा {
            // Yuk, split between the two
            conRhsp->unlink_Fr_पृष्ठभागः();
            conLhsp->unlink_Fr_पृष्ठभागः();
            AstConcat* ध्रुव newp
                = नूतन AstConcat{nodep->fileline(),
                                नूतन AstSel{nodep->fileline(), conLhsp, 0,
                                           nodep->msbConst() - conRhsp->विस्तारः() + 1},
                                नूतन AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(),
                                           conRhsp->विस्तारः() - nodep->lsbConst()}};
            nodep->replaceWithKeepDType(newp);
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    तार्किक सङ्कार्यम्_Sel_Replicate(AstSel* nodep) {
        // SEL(REPLICATE(from,rep),lsb,width) => SEL(from,0,width) as long
        // as SEL's width <= b's width
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ध्रुव repp = VN_AS(nodep->fromp(), Replicate);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = repp->srcp();
        ध्रुव AstConst* ध्रुव lsbp = VN_CAST(nodep->lsbp(), Const);
        यदि (!lsbp) प्रत्यागम असत्य;
        UASSERT_OBJ(fromp->विस्तारः(), nodep, "Not widthed");
        यदि ((lsbp->गन्तव्यम्_UInt() / fromp->विस्तारः())
            != ((lsbp->गन्तव्यम्_UInt() + nodep->विस्तारः() - 1) / fromp->विस्तारः())) {
            प्रत्यागम असत्य;
        }
        //
        fromp->unlink_Fr_पृष्ठभागः();
        AstSel* ध्रुव newp = नूतन AstSel{
            nodep->fileline(), fromp,
            नूतन AstConst{lsbp->fileline(), lsbp->गन्तव्यम्_UInt() % fromp->विस्तारः()}, nodep->विस्तारः_Const()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        प्रत्यागम सत्य;
    }
    तार्किक सङ्कार्यम्_Rep_Rep(AstReplicate* nodep) {
        // REPLICATE(REPLICATE2(from2,cnt2),cnt1) => REPLICATE(from2,(cnt1+cnt2))
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ध्रुव rep2p = VN_AS(nodep->srcp(), Replicate);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव from2p = rep2p->srcp();
        ध्रुव AstConst* ध्रुव cnt1p = VN_CAST(nodep->countp(), Const);
        यदि (!cnt1p) प्रत्यागम असत्य;
        ध्रुव AstConst* ध्रुव cnt2p = VN_CAST(rep2p->countp(), Const);
        यदि (!cnt2p) प्रत्यागम असत्य;
        //
        from2p->unlink_Fr_पृष्ठभागः();
        AstReplicate* ध्रुव newp
            = नूतन AstReplicate{nodep->fileline(), from2p, cnt1p->गन्तव्यम्_UInt() * cnt2p->गन्तव्यम्_UInt()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        प्रत्यागम सत्य;
    }
    तार्किक सङ्कार्यम्_Concat_Same(AstConcat* nodep) {
        // CONCAT(fromp,fromp) -> REPLICATE(fromp,1+1)
        // CONCAT(REP(fromp,cnt1),fromp) -> REPLICATE(fromp,cnt1+1)
        // CONCAT(fromp,REP(fromp,cnt1)) -> REPLICATE(fromp,1+cnt1)
        // CONCAT(REP(fromp,cnt1),REP(fromp,cnt2)) -> REPLICATE(fromp,cnt1+cnt2)
        Ast_ग्रन्थिः_व्यञ्जकम्* from1p = nodep->lhsp();
        uint32_t cnt1 = 1;
        Ast_ग्रन्थिः_व्यञ्जकम्* from2p = nodep->rhsp();
        uint32_t cnt2 = 1;
        यदि (VN_IS(from1p, Replicate)) {
            ध्रुव AstConst* ध्रुव cnt1p = VN_CAST(VN_CAST(from1p, Replicate)->countp(), Const);
            यदि (!cnt1p) प्रत्यागम असत्य;
            from1p = VN_AS(from1p, Replicate)->srcp();
            cnt1 = cnt1p->गन्तव्यम्_UInt();
        }
        यदि (VN_IS(from2p, Replicate)) {
            ध्रुव AstConst* ध्रुव cnt2p = VN_CAST(VN_CAST(from2p, Replicate)->countp(), Const);
            यदि (!cnt2p) प्रत्यागम असत्य;
            from2p = VN_AS(from2p, Replicate)->srcp();
            cnt2 = cnt2p->गन्तव्यम्_UInt();
        }
        यदि (!operandsSame(from1p, from2p)) प्रत्यागम असत्य;
        //
        from1p->unlink_Fr_पृष्ठभागः();
        AstReplicate* ध्रुव newp = नूतन AstReplicate{nodep->fileline(), from1p, cnt1 + cnt2};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        प्रत्यागम सत्य;
    }
    शून्य replaceSelIntoBiop(AstSel* nodep) {
        // SEL(BUFIF1(a,b),1,bit) => BUFIF1(SEL(a,1,bit),SEL(b,1,bit))
        Ast_ग्रन्थिः_Biop* ध्रुव fromp = VN_AS(nodep->fromp()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_Biop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lsbp = nodep->lsbp()->unlink_Fr_पृष्ठभागः();
        //
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bilhsp = fromp->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव birhsp = fromp->rhsp()->unlink_Fr_पृष्ठभागः();
        //
        fromp->lhsp(
            नूतन AstSel{nodep->fileline(), bilhsp, lsbp->clone_वृक्षः_विशुद्धम्(सत्य), nodep->विस्तारः_Const()});
        fromp->rhsp(नूतन AstSel{nodep->fileline(), birhsp, lsbp, nodep->विस्तारः_Const()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य replaceSelIntoUniop(AstSel* nodep) {
        // SEL(NOT(a),1,bit) => NOT(SEL(a,bit))
        Ast_ग्रन्थिः_Uniop* ध्रुव fromp = VN_AS(nodep->fromp()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_Uniop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lsbp = nodep->lsbp()->unlink_Fr_पृष्ठभागः();
        //
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bilhsp = fromp->lhsp()->unlink_Fr_पृष्ठभागः();
        //
        fromp->lhsp(नूतन AstSel{nodep->fileline(), bilhsp, lsbp, nodep->विस्तारः_Const()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य दर्शय(Ast_Attr_सम्बन्धि* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstArraySel* nodep) override {
        iterate_And_अग्र_Null(nodep->bitp());
        यदि (VN_IS(nodep->bitp(), Const)
            && VN_IS(nodep->fromp(), VarRef)
            // Need to make sure it's an array object so don't mis-allow a constant (bug509.)
            && VN_AS(nodep->fromp(), VarRef)->varp()
            && VN_IS(VN_AS(nodep->fromp(), VarRef)->varp()->valuep(), ConsArray)) {
            m_selp = nodep;  // Ask visit(AstVarRef) to replace varref with const
        }
        iterate_And_अग्र_Null(nodep->fromp());
        यदि (VN_IS(nodep->fromp(), Const)) {  // It did.
            यदि (!m_selp) {
                nodep->v3error("Illegal assignment of constant to unpacked array");
            } अन्यथा {
                Ast_ग्रन्थिः* ध्रुव fromp = nodep->fromp()->unlink_Fr_पृष्ठभागः();
                nodep->replaceWithKeepDType(fromp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            }
        }
        // Handle ARRAYSEL directly on ConsArray (not through VarRef)
        अन्यथा यदि (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ConsArray)) {
            ध्रुव AstConsArray* ध्रुव consp = VN_AS(nodep->fromp(), ConsArray);
            ध्रुव uint32_t bit = VN_AS(nodep->bitp(), Const)->गन्तव्यम्_UInt();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव itemp = consp->valuep(bit);
            यदि (VN_IS(itemp, Const)) {
                ध्रुव V3_सङ्ख्या& अङ्कः = VN_AS(itemp, Const)->अङ्कः();
                VL_DO_DANGLING(replace_अङ्कः(nodep, अङ्कः), nodep);
            } अन्यथा यदि (m_params && itemp) {
                // An aggregate element, e.g. a structure in an array parameter
                nodep->replaceWithKeepDType(itemp->clone_वृक्षः(असत्य));
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            }
        }
        m_selp = शून्यसूचकः;
    }

    // Evaluate a slice of an unpacked array.  If constantification is
    // required (m_required=true), call replaceWithSimulation() to compute
    // the slice via simulation.  Otherwise just iterate the children.
    शून्य दर्शय(Ast_खण्डिका_Sel* nodep) override {
        // First constify or width any child nodes
        iterateChildren(nodep);
        यदि (!m_required) प्रत्यागम;  // Do nothing unless we are in parameter mode
        // Fallback to simulation: this will invoke SimulateVisitor::visit(AstSliceSel*)
        replaceWithSimulation(nodep);
    }

    शून्य दर्शय(AstStructSel* nodep) override {
        iterateChildren(nodep);

        यदि (ध्रुव AstConsStruct* ध्रुव consp = VN_CAST(nodep->fromp(), ConsStruct)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = consp->memberValuep(nodep->name());
            यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const)) {
                VL_DO_DANGLING(replace_अङ्कः(nodep, constp->अङ्कः()), nodep);
            } अन्यथा {
                nodep->replaceWith(valuep->clone_वृक्षः(असत्य));
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            }
        }
    }

    शून्य दर्शय(AstCAwait* nodep) override {
        m_hasJumpDelay = सत्य;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Not linked");
        तार्किक did = असत्य;
        यदि (m_doV && (!nodep->varp()->const_Pool_प्रविष्टिः() || m_selp) && nodep->varp()->valuep()
            && !m_attrp) {
            // UINFOTREE(1, valuep, "", "visitvaref");
            iterate_And_अग्र_Null(nodep->varp()->valuep());  // May change nodep->varp()->valuep()
            Ast_ग्रन्थिः* ध्रुव valuep = nodep->varp()->valuep();
            // The selected element, when under a constant select of an array
            ध्रुव AstConsArray* ध्रुव consp = VN_CAST(valuep, ConsArray);
            ध्रुव Ast_ग्रन्थिः* ध्रुव selItemp
                = (m_selp && consp) ? consp->valuep(m_selp->bitConst()) : शून्यसूचकः;
            यदि (nodep->access().isReadOnly() && valuep
                && ((!m_params  // Can reduce constant wires into equations
                     && m_doNConst && v3Global.opt.fConst() && nodep->varp()->isConst())
                    || nodep->varp()->isParam())) {
                यदि (सङ्कार्यम्_Const(valuep)) {
                    ध्रुव V3_सङ्ख्या& अङ्कः = VN_AS(valuep, Const)->अङ्कः();
                    // UINFO(2, "constVisit " << cvtToHex(valuep) << " " << num);
                    ध्रुव string origParamName
                        = nodep->varp()->isParam() ? nodep->varp()->name() : "";
                    VL_DO_DANGLING(replace_अङ्कः(nodep, अङ्कः, origParamName), nodep);
                    did = सत्य;
                } अन्यथा यदि (ध्रुव AstConst* ध्रुव itemp = VN_CAST(selItemp, Const)) {
                    VL_DO_DANGLING(replace_अङ्कः(nodep, itemp->अङ्कः()), nodep);
                    did = सत्य;
                } अन्यथा यदि (m_params && consp) {
                    // Allow parameters to pass arrays
                    // Earlier recursion of ConsArray made sure each array value is constant
                    // This exception is fairly fragile, i.e. doesn't
                    // support arrays of arrays or other stuff
                    Ast_ग्रन्थिः* ध्रुव newp = valuep->clone_वृक्षः(असत्य);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    did = सत्य;
                } अन्यथा यदि (m_params && VN_IS(valuep, ConsStruct)) {
                    Ast_ग्रन्थिः* ध्रुव newp = valuep->clone_वृक्षः(असत्य);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    did = सत्य;
                } अन्यथा यदि (nodep->varp()->isParam() && VN_IS(valuep, Unbounded)) {
                    Ast_ग्रन्थिः* ध्रुव newp = valuep->clone_वृक्षः(असत्य);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    did = सत्य;
                }
            }
        }
        यदि (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but variable isn't const: "
                           << nodep->varp()->prettyNameQ());
        }
    }
    शून्य दर्शय(Ast_व्यञ्जकम्_Stmt* nodep) override {
        iterateChildren(nodep);
        यदि (!Ast_ग्रन्थिः::afterCommentp(nodep->stmtsp())) {
            deleteVarScopesUnder(nodep->stmtsp());
            UINFO(8, "ExprStmt(...) " << nodep << " " << nodep->resultp());
            nodep->replaceWith(nodep->resultp()->unlink_Fr_पृष्ठभागः());
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            // Removing the ExprStmt might have made something impure above now pure
        }
    }
    शून्य दर्शय(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        तार्किक did = असत्य;
        यदि (nodep->itemp()->valuep()) {
            // UINFOTREE(1, nodep->itemp()->valuep(), "", "visitvaref");
            ध्रुव AstEnumItem* ध्रुव itemp = nodep->itemp();
            यदि (m_recursing_Enum_वस्तूनि.गणनम्(itemp)) {
                nodep->v3error("Recursive enum value: " << nodep->itemp()->prettyNameQ());
            } अन्यथा {
                m_recursing_Enum_वस्तूनि.emplace(itemp);
                iterate_And_अग्र_Null(nodep->itemp()->valuep());
                m_recursing_Enum_वस्तूनि.erase(itemp);
            }
            यदि (AstConst* ध्रुव valuep = VN_CAST(nodep->itemp()->valuep(), Const)) {
                ध्रुव V3_सङ्ख्या& अङ्कः = valuep->अङ्कः();
                VL_DO_DANGLING(replace_अङ्कः(nodep, अङ्कः), nodep);
                did = सत्य;
            }
        }
        यदि (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but enum value isn't const: "
                           << nodep->itemp()->prettyNameQ());
        }
    }

    तार्किक only_Sen_Item_अन्तर्_Sen_वृक्षः(ध्रुव AstSenItem* nodep) {
        // Only one if it's not in a list
        प्रत्यागम (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }
    शून्य दर्शय(AstSenItem* nodep) override {
        iterateChildren(nodep);
        यदि (m_doNConst
            && !v3Global.opt.timing().is_समुच्चयः_True()  // If --timing, V3Sched would turn this into an
                                                   // infinite loop. See #5080
            && (VN_IS(nodep->sensp(), Const) || VN_IS(nodep->sensp(), EnumItemRef)
                || (nodep->varrefp() && nodep->varrefp()->varp()->isParam()))) {
            // Constants in sensitivity lists may be removed (we'll simplify later)
            यदि (nodep->isClocked()) {  // A constant can never get a pos/negedge
                यदि (only_Sen_Item_अन्तर्_Sen_वृक्षः(nodep)) {
                    यदि (nodep->edgeType() == VEdgeType::ET_परिवर्तितम्) {
                        // TODO: This really is dodgy, as strictly compliant simulators will not
                        //       execute this block, but but t_func_check relies on it
                        nodep->replaceWith(
                            नूतन AstSenItem{nodep->fileline(), AstSenItem::Initial{}});
                    } अन्यथा {
                        nodep->replaceWith(नूतन AstSenItem{nodep->fileline(), AstSenItem::Never{}});
                    }
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                } अन्यथा {
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                }
            } अन्यथा {  // Otherwise it may compute a result that needs to settle out
                nodep->replaceWith(नूतन AstSenItem{nodep->fileline(), AstSenItem::Combo{}});
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            }
        } अन्यथा यदि (m_doNConst && VN_IS(nodep->sensp(), Not)) {
            // V3Gate may propagate NOTs into clocks... Just deal with it
            Ast_ग्रन्थिः* ध्रुव sensp = nodep->sensp();
            Ast_ग्रन्थिः* चरमः_Sensp = sensp;
            तार्किक invert = असत्य;
            यावत् (VN_IS(चरमः_Sensp, Not)) {
                चरमः_Sensp = VN_AS(चरमः_Sensp, Not)->lhsp();
                invert = !invert;
            }
            UINFO(8, "senItem(NOT...) " << nodep << " " << invert);
            यदि (invert) nodep->edgeType(nodep->edgeType().invert());
            sensp->replaceWith(चरमः_Sensp->unlink_Fr_पृष्ठभागः());
            VL_DO_DANGLING(प्रक्षिप_Deletep(sensp), sensp);
        }
    }

    वर्ग SenItemCmp final {
        स्थिर पूर्णाङ्क cmp(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ap, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* bp) {
            ध्रुव VNType aType = ap->type();
            ध्रुव VNType bType = bp->type();
            यदि (aType != bType) प्रत्यागम static_cast<पूर्णाङ्क>(bType) - static_cast<पूर्णाङ्क>(aType);

            यदि (ध्रुव AstVarRef* ध्रुव aRefp = VN_CAST(ap, VarRef)) {
                ध्रुव AstVarRef* ध्रुव bRefp = VN_AS(bp, VarRef);
                // Looks visually better if we keep sorted by name
                यदि (aRefp->name() < bRefp->name()) प्रत्यागम -1;
                यदि (aRefp->name() > bRefp->name()) प्रत्यागम 1;
                // But might be same name with different scopes
                यदि (aRefp->varScopep() < bRefp->varScopep()) प्रत्यागम -1;
                यदि (aRefp->varScopep() > bRefp->varScopep()) प्रत्यागम 1;
                // Or rarely, different data types
                यदि (aRefp->dtypep() < bRefp->dtypep()) प्रत्यागम -1;
                यदि (aRefp->dtypep() > bRefp->dtypep()) प्रत्यागम 1;
                प्रत्यागम 0;
            }

            यदि (ध्रुव AstConst* ध्रुव aConstp = VN_CAST(ap, Const)) {
                ध्रुव AstConst* ध्रुव bConstp = VN_AS(bp, Const);
                यदि (aConstp->अङ्कः().isLtXZ(bConstp->अङ्कः())) प्रत्यागम -1;
                यदि (bConstp->अङ्कः().isLtXZ(aConstp->अङ्कः())) प्रत्यागम 1;
                प्रत्यागम 0;
            }

            यदि (ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव aBiOpp = VN_CAST(ap, ग्रन्थिः_Biop)) {
                ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव bBiOpp = VN_AS(bp, ग्रन्थिः_Biop);
                // Compare RHSs first, as LHS might be const
                यदि (ध्रुव पूर्णाङ्क c = cmp(aBiOpp->rhsp(), bBiOpp->rhsp())) प्रत्यागम c;
                प्रत्यागम cmp(aBiOpp->lhsp(), bBiOpp->lhsp());
            }

            यदि (ध्रुव AstCMethodHard* ध्रुव aCallp = VN_CAST(ap, CMethodHard)) {
                ध्रुव AstCMethodHard* ध्रुव bCallp = VN_AS(bp, CMethodHard);
                यदि (aCallp->विधानम्() < bCallp->विधानम्()) प्रत्यागम -1;
                यदि (aCallp->विधानम्() > bCallp->विधानम्()) प्रत्यागम 1;
                यदि (ध्रुव पूर्णाङ्क c = cmp(aCallp->fromp(), bCallp->fromp())) प्रत्यागम c;
                ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* aPinsp = aCallp->pinsp();
                ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* bPinsp = bCallp->pinsp();
                यावत् (aPinsp && bPinsp) {
                    यदि (ध्रुव पूर्णाङ्क c = cmp(aPinsp, bPinsp)) प्रत्यागम c;
                    aPinsp = VN_AS(aPinsp->nextp(), ग्रन्थिः_व्यञ्जकम्);
                    bPinsp = VN_AS(bPinsp->nextp(), ग्रन्थिः_व्यञ्जकम्);
                }
                प्रत्यागम aPinsp ? -1 : bPinsp ? 1 : 0;
            }

            प्रत्यागम 0;
        }

    सार्वजनिक:
        तार्किक संक्रियकः()(ध्रुव AstSenItem* lhsp, ध्रुव AstSenItem* rhsp) ध्रुव {
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lSensp = lhsp->sensp();
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rSensp = rhsp->sensp();
            यदि (lSensp && rSensp) {
                // If both terms have sensitivity expressions, recursively compare them
                यदि (ध्रुव पूर्णाङ्क c = cmp(lSensp, rSensp)) प्रत्यागम c < 0;
            } अन्यथा यदि (lSensp || rSensp) {
                // Terms with sensitivity expressions come after those without
                प्रत्यागम rSensp;
            }
            // Finally sort by edge, AFTER variable, as we want multiple edges for same var
            // adjacent. note the SenTree optimizer requires this order (more general first,
            // less general last)
            प्रत्यागम lhsp->edgeType() < rhsp->edgeType();
        }
    };

    शून्य दर्शय(Ast_Sen_वृक्षः* nodep) override {
        iterateChildren(nodep);
        यदि (m_doExpensive) {
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
            प्रति (AstSenItem *nextp, *senp = nodep->sensesp(); senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                // cppcheck-suppress unassignedVariable  // cppcheck bug
                ध्रुव SenItemCmp cmp;
                यदि (nextp && !cmp(senp, nextp)) {
                    // Something's out of order, sort it
                    senp = शून्यसूचकः;
                    std::सदिशम्<AstSenItem*> vec;
                    प्रति (AstSenItem* senp = nodep->sensesp(); senp;
                         senp = VN_AS(senp->nextp(), SenItem)) {
                        vec.प्रक्षिप_पृष्ठभागः(senp);
                    }
                    stable_क्रमय(vec.begin(), vec.अन्तः_2(), SenItemCmp());
                    प्रति (ध्रुव स्वचालित& ip : vec) ip->unlink_Fr_पृष्ठभागः();
                    प्रति (ध्रुव स्वचालित& ip : vec) nodep->सङ्कलय_Sensesp(ip);
                    भङ्ग;
                }
            }

            // Pass 2, remove duplicates and simplify adjacent terms if possible
            प्रति (AstSenItem *senp = nodep->sensesp(), *nextp; senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                यदि (!nextp) भङ्ग;
                AstSenItem* ध्रुव lItemp = senp;
                AstSenItem* ध्रुव rItemp = nextp;
                ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lSenp = lItemp->sensp();
                ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rSenp = rItemp->sensp();
                यदि (!lSenp || !rSenp) अनुवर्त;

                यदि (lSenp->same_Gate_वृक्षः(rSenp)) {
                    // POSEDGE or NEGEDGE -> BOTHEDGE. (We've sorted POSEDGE, before NEGEDGE, so we
                    // do not need to test for the opposite orders.)
                    यदि (lItemp->edgeType() == VEdgeType::ET_POSEDGE
                        && rItemp->edgeType() == VEdgeType::ET_NEGEDGE) {
                        // Make both terms BOTHEDGE, the second will be removed below
                        lItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                        rItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                    }

                    // Remove identical expressions
                    यदि (lItemp->edgeType() == rItemp->edgeType()) {
                        VL_DO_DANGLING(प्रक्षिप_Deletep(rItemp->unlink_Fr_पृष्ठभागः()), rItemp);
                        nextp = lItemp;
                    }
                }
            }
        }
    }

    //-----
    // Zero elimination
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        iterateChildren(nodep);
        यदि (nodep->timingControlp()) m_hasJumpDelay = सत्य;
        यदि (m_doNConst && replace_ग्रन्थिः_नियोजय(nodep)) प्रत्यागम;
    }
    शून्य दर्शय(AstAlias* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    शून्य दर्शय(AstAliasScope* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    शून्य दर्शय(Ast_नियोजय_W* nodep) override {
        iterateChildren(nodep);
        यदि (m_doNConst && replace_ग्रन्थिः_नियोजय(nodep)) प्रत्यागम;
        // Process containing this AssignW as single body statement
        AstAlways* ध्रुव procp = VN_CAST(nodep->backp(), Always);
        यदि (!procp || procp->stmtsp() != nodep || nodep->nextp()) प्रत्यागम;
        // Not VarXRef, as different refs may set different values to each hierarchy
        Ast_ग्रन्थिः_Var_Ref* ध्रुव varrefp = VN_CAST(nodep->lhsp(), VarRef);
        यदि (m_wremove && !m_params && m_doNConst && m_modp && सङ्कार्यम्_Const(nodep->rhsp())
            && !VN_AS(nodep->rhsp(), Const)->अङ्कः().isFourState()
            && varrefp  // Don't do messes with BITREFs/ARRAYREFs
            && !varrefp->varp()->hasStrengthAssignment()  // Strengths are resolved in V3Tristate
            && !varrefp->varp()->valuep()  // Not already constified
            && !varrefp->varScopep()  // Not scoped (or each scope may have different initial val.)
            && !varrefp->varp()->isForced()  // Not forced (not really a constant)
        ) {
            // ASSIGNW (VARREF, const) -> INITIAL ( ASSIGN (VARREF, const) )
            UINFO(4, "constAssignW " << nodep);
            // Make a initial assignment
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
            varrefp->unlink_Fr_पृष्ठभागः();
            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            AstInitial* ध्रुव newinitp = नूतन AstInitial{flp, नूतन Ast_नियोजय{flp, varrefp, exprp}};
            procp->replaceWith(newinitp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(procp), procp);
            // Set the initial value right in the variable so we can constant propagate
            Ast_ग्रन्थिः* ध्रुव initvaluep = exprp->clone_वृक्षः(असत्य);
            varrefp->varp()->isConst(सत्य);
            varrefp->varp()->valuep(initvaluep);
        }
    }
    शून्य दर्शय(AstCReset* nodep) override {
        iterateChildren(nodep);
        यदि (!m_doNConst) प्रत्यागम;
        ध्रुव AstBasicDType* ध्रुव bdtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType);
        यदि (!bdtypep) प्रत्यागम;
        यदि (!bdtypep->isZeroInit()) प्रत्यागम;
        AstConst* ध्रुव newp = नूतन AstConst{nodep->fileline(), V3_सङ्ख्या{nodep, bdtypep}};
        UINFO(9, "CRESET(0) => CONST(0) " << nodep);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(Ast_Cvt_Unpack_गन्तव्यम्_Unpack* nodep) override {
        iterateChildren(nodep);
        AstConsArray* ध्रुव fromp = VN_CAST(nodep->fromp(), ConsArray);
        यदि (!fromp) प्रत्यागम;
        // Fold the outermost dimension, converting the elements for any dimension under it
        ध्रुव AstUnpackArrayDType* ध्रुव गन्तव्यम्_Arrayp
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ध्रुव AstUnpackArrayDType* ध्रुव fromArrayp
            = VN_AS(fromp->dtypep()->skipRefp(), UnpackArrayDType);
        // This dimension may not be reversed, only one under it
        ध्रुव तार्किक reverse
            = गन्तव्यम्_Arrayp->declRange().ascending() != fromArrayp->declRange().ascending();
        ध्रुव uint64_t चरमः_सूचकांक = गन्तव्यम्_Arrayp->elementsConst() - 1;
        Ast_ग्रन्थिः_DType* ध्रुव subDTypep = गन्तव्यम्_Arrayp->subDTypep();
        ध्रुव AstUnpackArrayDType* ध्रुव subArrayp
            = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
        ध्रुव स्वचालित convert = [&](Ast_ग्रन्थिः_व्यञ्जकम्* valuep) -> Ast_ग्रन्थिः_व्यञ्जकम्* {
            valuep->unlink_Fr_पृष्ठभागः();
            यदि (!subArrayp || !subArrayp->is_Reverse_सम्बन्धि(valuep->dtypep())) प्रत्यागम valuep;
            प्रत्यागम नूतन Ast_Cvt_Unpack_गन्तव्यम्_Unpack{valuep->fileline(), valuep, subDTypep};
        };
        AstConsArray* ध्रुव newp
            = नूतन AstConsArray{fromp->fileline(), nodep->dtypep(),
                               fromp->defaultp() ? convert(fromp->defaultp()) : शून्यसूचकः};
        प्रति (ध्रुव AstConsArrayItem* itemp = fromp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            newp->valuep(reverse ? चरमः_सूचकांक - itemp->सूचकांक() : itemp->सूचकांक(),
                         convert(itemp->valuep()));
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        iterate(newp);  // Fold the conversions of the elements
    }
    शून्य दर्शय(Ast_Cvt_Array_गन्तव्यम्_Array* nodep) override {
        iterateChildren(nodep);
        // Handle the case where we have a stream operation inside a cast conversion
        // Check for both StreamL and StreamR operations
        Ast_ग्रन्थिः_प्रवाहः* streamp = शून्यसूचकः;
        तार्किक isReverse = असत्य;
        यदि (Ast_प्रवाहः_L* ध्रुव प्रवाहः_Lp = VN_CAST(nodep->fromp(), प्रवाहः_L)) {
            streamp = प्रवाहः_Lp;
            isReverse = सत्य;  // StreamL reverses the operation
        } अन्यथा यदि (Ast_प्रवाहः_R* ध्रुव प्रवाहः_Rp = VN_CAST(nodep->fromp(), प्रवाहः_R)) {
            streamp = प्रवाहः_Rp;
            isReverse = असत्य;  // StreamR doesn't reverse the operation
        }
        यदि (streamp) {
            Ast_ग्रन्थिः_व्यञ्जकम्* srcp = streamp->lhsp();
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव srcDTypep = srcp->dtypep()->skipRefp();
            Ast_ग्रन्थिः_DType* ध्रुव dstDTypep = nodep->dtypep()->skipRefp();
            यदि (VN_IS(srcDTypep, QueueDType) && VN_IS(dstDTypep, QueueDType)) {
                पूर्णाङ्क खण्डम्_आकार = 1;
                यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(streamp->rhsp(), Const)) {
                    खण्डम्_आकार = constp->गन्तव्यम्_SInt();
                    यदि (VL_UNLIKELY(खण्डम्_आकार <= 0)) {
                        // Not reachable due to higher level checks when parsing stream
                        // operators commented out to not fail v3error-coverage-checks.
                        // nodep->v3error("Stream block size must be positive, got " <<
                        // blockSize);
                        खण्डम्_आकार = 1;
                    }
                }
                // Not reachable due to higher level checks when parsing stream operators
                // commented out to not fail v3error-coverage-checks.
                // else {
                //    nodep->v3error("Stream block size must be constant (got " <<
                //    streamp->rhsp()->prettyTypeName() << ")");
                // }
                पूर्णाङ्क src_Element_द्वयङ्काः = 0;
                यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elemDtp = srcDTypep->subDTypep()) {
                    src_Element_द्वयङ्काः = elemDtp->विस्तारः();
                }
                पूर्णाङ्क dst_Element_द्वयङ्काः = 0;
                यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elemDtp = dstDTypep->subDTypep()) {
                    dst_Element_द्वयङ्काः = elemDtp->विस्तारः();
                }
                streamp->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः_व्यञ्जकम्* newp = नूतन Ast_Cvt_Array_गन्तव्यम्_Array{
                    srcp->fileline(), srcp->unlink_Fr_पृष्ठभागः(), dstDTypep,     isReverse,
                    खण्डम्_आकार,        dst_Element_द्वयङ्काः,       src_Element_द्वयङ्काः};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(streamp), streamp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम;
            }
        }
    }
    शून्य दर्शय(AstRelease* nodep) override {
        // cppcheck-suppress constVariablePointer // children unlinked below
        यदि (AstConcat* ध्रुव concatp = VN_CAST(nodep->lhsp(), Concat)) {
            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            AstRelease* ध्रुव newLp = नूतन AstRelease{flp, concatp->lhsp()->unlink_Fr_पृष्ठभागः()};
            AstRelease* ध्रुव newRp = नूतन AstRelease{flp, concatp->rhsp()->unlink_Fr_पृष्ठभागः()};
            nodep->replaceWith(newLp);
            newLp->सङ्कलय_अग्र_अत्र(newRp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            दर्शय(newLp);
            दर्शय(newRp);
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः_If* nodep) override {
        iterateChildren(nodep);
        यदि (m_doNConst) {
            यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->condp(), Const)) {
                Ast_ग्रन्थिः* keepp = शून्यसूचकः;
                Ast_ग्रन्थिः* delp = शून्यसूचकः;
                यदि (constp->isZero()) {
                    UINFO(4, "IF(0,{any},{x}) => {x}: " << nodep);
                    keepp = nodep->elsesp();
                    delp = nodep->thensp();
                } अन्यथा यदि (!m_doV || constp->isNeqZero()) {  // Might be X in Verilog
                    UINFO(4, "IF(!0,{x},{any}) => {x}: " << nodep);
                    keepp = nodep->thensp();
                    delp = nodep->elsesp();
                } अन्यथा {
                    UINFO(4, "IF condition is X, retaining: " << nodep);
                    प्रत्यागम;
                }
                यदि (!canDeleteStatements()) {
                    UINFO(4, "IF retained until statics lifted: " << nodep);
                    प्रत्यागम;
                }

                // If we delete a branch that contains variable declarations, also delete the
                // corresponding varscopes so we don't leave dangling AstVarScope::m_varp pointers.
                deleteVarScopesUnder(delp);

                यदि (keepp) {
                    keepp->unlink_Fr_पृष्ठभागः_With_अग्र();
                    nodep->replaceWith(keepp);
                } अन्यथा {
                    nodep->unlink_Fr_पृष्ठभागः();
                    deleteVarScopesUnder(nodep->thensp());
                    deleteVarScopesUnder(nodep->elsesp());
                }
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);

                // Removed branch could contain only impurity within a function and the
                // function could be purified
                VIsCached::clear_Cache_वृक्षः();
            } अन्यथा यदि (!Ast_ग्रन्थिः::afterCommentp(nodep->thensp())
                       && !Ast_ग्रन्थिः::afterCommentp(nodep->elsesp())) {
                यदि (!nodep->condp()->is_विशुद्धम्()) {
                    // Condition has side effect - leave, but don't need return value
                    UINFO(4, "IF({x}) ; => STMTEXPR({x}): " << nodep);
                    nodep->fileline()->warnOff(V3_Error_संहिता::IGNOREDRETURN, सत्य);
                    Ast_ग्रन्थिः_Stmt* ध्रुव newp
                        = नूतन Ast_Stmt_व्यञ्जकम्{nodep->fileline(), nodep->condp()->unlink_Fr_पृष्ठभागः()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                } अन्यथा {
                    // Empty block, remove it
                    UINFO(4, "IF({x}) ; => ; : " << nodep);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                }
            } अन्यथा यदि (!Ast_ग्रन्थिः::afterCommentp(nodep->thensp())) {
                UINFO(4, "IF({x}) nullptr {...} => IF(NOT{x}}: " << nodep);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = nodep->condp();
                Ast_ग्रन्थिः* ध्रुव elsesp = nodep->elsesp();
                condp->unlink_Fr_पृष्ठभागः_With_अग्र();
                elsesp->unlink_Fr_पृष्ठभागः_With_अग्र();
                यदि (nodep->thensp()) {  // Must have been comment
                    प्रक्षिप_Deletep(nodep->thensp()->unlink_Fr_पृष्ठभागः_With_अग्र());
                }
                nodep->condp(नूतन AstLogNot{condp->fileline(),
                                           condp});  // LogNot, as C++ optimization also possible
                nodep->सङ्कलय_Thensp(elsesp);
                nodep->branchPred(nodep->branchPred().invert());
            } अन्यथा यदि (((VN_IS(nodep->condp(), Not) && nodep->condp()->विस्तारः() == 1)
                        || VN_IS(nodep->condp(), LogNot))
                       && nodep->thensp() && nodep->elsesp()) {
                UINFO(4, "IF(NOT {x})  => IF(x) swapped if/else" << nodep);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp
                    = VN_AS(nodep->condp(), ग्रन्थिः_Uniop)->lhsp()->unlink_Fr_पृष्ठभागः_With_अग्र();
                Ast_ग्रन्थिः* ध्रुव thensp = nodep->thensp()->unlink_Fr_पृष्ठभागः_With_अग्र();
                Ast_ग्रन्थिः* ध्रुव elsesp = nodep->elsesp()->unlink_Fr_पृष्ठभागः_With_अग्र();
                AstIf* ध्रुव ifp = नूतन AstIf{nodep->fileline(), condp, elsesp, thensp};
                ifp->isBoundsCheck(nodep->isBoundsCheck());  // Copy bounds check info
                ifp->branchPred(nodep->branchPred().invert());
                nodep->replaceWith(ifp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            } अन्यथा यदि (if_Same_नियोजय(nodep)) {
                UINFO(4,
                      "IF({a}) ASSIGN({b},{c}) else ASSIGN({b},{d}) => ASSIGN({b}, {a}?{c}:{d})");
                Ast_ग्रन्थिः_नियोजय* ध्रुव thensp = VN_AS(nodep->thensp(), ग्रन्थिः_नियोजय);
                // cppcheck-suppress constVariablePointer // children unlinked below
                Ast_ग्रन्थिः_नियोजय* ध्रुव elsesp = VN_AS(nodep->elsesp(), ग्रन्थिः_नियोजय);
                thensp->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = nodep->condp()->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव truep = thensp->rhsp()->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव falsep = elsesp->rhsp()->unlink_Fr_पृष्ठभागः();
                thensp->rhsp(नूतन AstCond{truep->fileline(), condp, truep, falsep});
                nodep->replaceWith(thensp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            } अन्यथा यदि (असत्य  // Disabled, as vpm assertions are faster
                              // without due to short-circuiting
                       && सङ्कार्यम्_If_If(nodep)) {
                UINFO(9, "IF({a}) IF({b}) => IF({a} && {b})");
                Ast_ग्रन्थिः_If* ध्रुव अधस्तनम्_Ifp = VN_AS(nodep->thensp(), ग्रन्थिः_If);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = nodep->condp()->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः* ध्रुव अधस्तनम्_Thensp = अधस्तनम्_Ifp->thensp()->unlink_Fr_पृष्ठभागः_With_अग्र();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव अधस्तनम्_Condp = अधस्तनम्_Ifp->condp()->unlink_Fr_पृष्ठभागः_With_अग्र();
                nodep->condp(नूतन AstLogAnd{अधस्तनम्_Ifp->fileline(), condp, अधस्तनम्_Condp});
                अधस्तनम्_Ifp->replaceWith(अधस्तनम्_Thensp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(अधस्तनम्_Ifp), अधस्तनम्_Ifp);
            } अन्यथा {
                // Optimizations that don't reform the IF itself
                यदि (सङ्कार्यम्_Bool_सरणम्(nodep->condp())) replace_Bool_सरणम्(nodep->condp());
                मेलनम्_If_Same_चरमः(nodep);
                मेलनम्_If_Cond_Cond(nodep);
            }
        }
    }

    शून्य दर्शय(AstDisplay* nodep) override {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        iterateChildren(nodep);
        यदि (stmtDisplayDisplay(nodep)) प्रत्यागम;
    }
    तार्किक stmtDisplayDisplay(AstDisplay* nodep) {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        यदि (!m_modp) प्रत्यागम असत्य;  // Don't optimize under single statement
        AstDisplay* ध्रुव prevp = VN_CAST(nodep->backp(), Display);
        यदि (!prevp) प्रत्यागम असत्य;
        यदि (!((prevp->displayType() == nodep->displayType())
              || (prevp->displayType() == VDisplayType::DT_WRITE
                  && nodep->displayType() == VDisplayType::DT_DISPLAY)
              || (prevp->displayType() == VDisplayType::DT_DISPLAY
                  && nodep->displayType() == VDisplayType::DT_WRITE)))
            प्रत्यागम असत्य;
        यदि ((prevp->filep() && !nodep->filep()) || (!prevp->filep() && nodep->filep())
            || (prevp->filep() && nodep->filep() && !prevp->filep()->same_वृक्षः(nodep->filep())))
            प्रत्यागम असत्य;
        यदि (!prevp->fmtp() || prevp->fmtp()->nextp() || !nodep->fmtp() || nodep->fmtp()->nextp())
            प्रत्यागम असत्य;
        AstSFormatF* ध्रुव pformatp = prevp->fmtp();
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSFormatF* ध्रुव nformatp = nodep->fmtp();
        // We don't merge scopeNames as can have only one and might be different scopes (late in
        // process) Also rare for real code to print %m multiple times in same message
        यदि (nformatp->scopeNamep() && pformatp->scopeNamep()) प्रत्यागम असत्य;
        // We don't early merge arguments as might need to later print warnings with
        // right line numbers, nor scopeNames as might be different scopes (late in process)
        यदि (!m_doCpp && pformatp->exprsp()) प्रत्यागम असत्य;
        यदि (!m_doCpp && nformatp->exprsp()) प्रत्यागम असत्य;
        यदि (pformatp->exprsp() && !pformatp->exprsp()->is_विशुद्धम्_And_अग्र()) प्रत्यागम असत्य;
        यदि (nformatp->exprsp() && !nformatp->exprsp()->is_विशुद्धम्_And_अग्र()) प्रत्यागम असत्य;
        // Avoid huge merges
        स्थिर constexpr पूर्णाङ्क DISPLAY_MAX_MERGE_दीर्घता = 500;
        यदि (pformatp->text().दीर्घता_2() + nformatp->text().दीर्घता_2() > DISPLAY_MAX_MERGE_दीर्घता)
            प्रत्यागम असत्य;
        //
        UINFO(9, "DISPLAY(SF({a})) DISPLAY(SF({b})) -> DISPLAY(SF({a}+{b}))");
        // Convert DT_DISPLAY to DT_WRITE as may allow later optimizations
        यदि (prevp->displayType() == VDisplayType::DT_DISPLAY) {
            prevp->displayType(VDisplayType::DT_WRITE);
            pformatp->text(pformatp->text() + "\n");
        }
        // We can't replace prev() as the edit tracking iterators will get confused.
        // So instead we edit the prev note itself.
        यदि (prevp->सङ्कलय_Newline()) pformatp->text(pformatp->text() + "\n");
        pformatp->text(pformatp->text() + nformatp->text());
        यदि (!prevp->सङ्कलय_Newline() && nodep->सङ्कलय_Newline()) pformatp->text(pformatp->text() + "\n");
        यदि (nformatp->exprsp()) pformatp->सङ्कलय_Exprsp(nformatp->exprsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
        यदि (AstScopeName* ध्रुव scopeNamep = nformatp->scopeNamep()) {
            scopeNamep->unlink_Fr_पृष्ठभागः_With_अग्र();
            pformatp->scopeNamep(scopeNamep);
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
        प्रत्यागम सत्य;
    }
    शून्य दर्शय(AstSFormatArg* nodep) override {
        // Skip namep(): requiring its runtime lookup to be constant can reject valid
        // enum-valued constant-function calls in parameters. displayedEnum() resolves
        // the name from the folded exprp() value and enum dtype instead.
        iterate_And_अग्र_Null(nodep->exprp());
    }
    शून्य दर्शय(AstSFormatF* nodep) override {
        // Substitute constants into displays.  The main point of this is to
        // simplify assertion methodologies which call functions with display's.
        // This eliminates a pile of wide temps, and makes the C a whole lot more readable.
        iterateChildren(nodep);
        यदि (nodep->व्यञ्जकम्_Format_2()) {
            // Upconvert to text-based format?
            // Similar code in V3LinkResolve::visit(AstSFormatF)
            UASSERT_OBJ(nodep->text() == "", nodep,
                        "Non-format $sformatf should have text format == \"\"");
            यदि (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->अङ्कः().isFromString()) {
                AstConst* ध्रुव fmtp = VN_AS(nodep->exprsp()->unlink_Fr_पृष्ठभागः(), Const);
                nodep->text(fmtp->अङ्कः().गन्तव्यम्_String());
                nodep->व्यञ्जकम्_Format_2(असत्य);
                VL_DO_DANGLING(प्रक्षिप_Deletep(fmtp), fmtp);
            }
        }
        यदि (nodep->व्यञ्जकम्_Format_2()) प्रत्यागम;  // Runtime, unfortunately  (unless constify format later)
        तार्किक anyconst = असत्य;
        प्रति (Ast_ग्रन्थिः* argp = nodep->exprsp(); argp; argp = argp->nextp()) {
            AstSFormatArg* ध्रुव fargp = VN_CAST(argp, SFormatArg);
            Ast_ग्रन्थिः* ध्रुव subargp = fargp ? fargp->exprp() : argp;
            यदि (VN_IS(subargp, Const)) {
                anyconst = सत्य;
                भङ्ग;
            }
        }
        यदि (m_doNConst && anyconst) {
            UINFO(9, "  Display in  " << nodep->text());
            string newFormat;
            string fmt;
            तार्किक अन्तर्_Pct = असत्य;
            Ast_ग्रन्थिः* argp = nodep->exprsp();
            ध्रुव string text = nodep->text();
            प्रति (वर्ण ch : text) {
                यदि (!अन्तर्_Pct && ch == '%') {
                    अन्तर्_Pct = सत्य;
                    fmt = ch;
                } अन्यथा यदि (अन्तर्_Pct && (std::अङ्कः_वा(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } अन्यथा यदि (अन्तर्_Pct) {
                    अन्तर्_Pct = असत्य;
                    fmt += ch;
                    ch = std::tolower(ch);
                    विकल्प (ch) {
                    पक्ष '%': भङ्ग;  // %% - still %%
                    पक्ष 'm': भङ्ग;  // %m - still %m - auto insert "name"
                    पक्ष 'l': भङ्ग;  // %l - still %l - auto insert "library"
                    पक्ष 't':  // %t - don't know $timeformat so can't constify
                        यदि (argp) argp = argp->nextp();
                        भङ्ग;
                    स्वभाव:  // Most operators, just move to next argument
                        यदि (argp) {
                            Ast_ग्रन्थिः* ध्रुव nextp = argp->nextp();
                            AstSFormatArg* ध्रुव fargp = VN_CAST(argp, SFormatArg);
                            Ast_ग्रन्थिः* ध्रुव subargp = fargp ? fargp->exprp() : argp;
                            ध्रुव VFormatAttr formatAttr
                                = argp
                                      ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                                      : VFormatAttr{};
                            यदि (VN_IS(subargp, Const)) {  // Convert it
                                ध्रुव string बहिर्गमनम्
                                    = formatAttr.isEnum()
                                          ? const_अङ्कः_V(subargp).displayedEnum(fargp, fmt)
                                          : const_अङ्कः_V(subargp).displayed(nodep, fmt, formatAttr);
                                UINFO(9, "     DispConst: " << fmt << " -> " << बहिर्गमनम् << "  for "
                                                            << subargp);
                                // fmt = out w/ replace % with %% as it must later when
                                // used as a format output as literal.
                                fmt = VString::quotePercent(बहिर्गमनम्);
                                VL_DO_DANGLING(प्रक्षिप_Deletep(argp->unlink_Fr_पृष्ठभागः()), argp);
                            }
                            argp = nextp;
                        }
                        भङ्ग;
                    }  // switch
                    newFormat += fmt;
                } अन्यथा {
                    newFormat += ch;
                }
            }
            यदि (newFormat != nodep->text()) {
                nodep->text(newFormat);
                UINFO(9, "  Display out " << nodep);
            }
        }
        यदि (!nodep->exprsp() && nodep->text().find('%') == string::npos && !nodep->hidden()) {
            // Just a simple constant string - the formatting is pointless
            UINFO(9, "replaceConstStr");
            VL_DO_DANGLING(replaceConstString(nodep, nodep->text()), nodep);
            प्रत्यागम;
        }
        यदि (VN_IS(nodep->backp(), ग्रन्थिः_नियोजय)  // Can't remove under Display etc
            && nodep->text() == "%s" && VN_IS(nodep->exprsp(), SFormatArg)
            && VN_AS(nodep->exprsp(), SFormatArg)->formatAttr().isString()
            && !nodep->exprsp()->nextp()) {
            UINFO(9, "  Display(%p, expr) -> constant expr " << nodep);
            nodep->replaceWith(VN_AS(nodep->exprsp(), SFormatArg)->exprp()->unlink_Fr_पृष्ठभागः());
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }
    }
    शून्य दर्शय(AstSFormat* nodep) override {
        iterateChildren(nodep);
        यदि (!m_doNConst) प्रत्यागम;
        यदि (m_doNConst) {
            // If it's a constant string written to a string variable, inline it as an assignment
            AstSFormatF* ध्रुव fmtp = nodep->fmtp();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp();
            AstBasicDType* ध्रुव basicp = VN_CAST(lhsp->dtypep()->skipRefp(), BasicDType);
            यदि (basicp && basicp->isString() && !fmtp->exprsp()
                && fmtp->text().find('%') == string::npos) {
                AstConst* ध्रुव strp
                    = नूतन AstConst{fmtp->fileline(), AstConst::String{}, fmtp->text()};
                lhsp->unlink_Fr_पृष्ठभागः();
                nodep->replaceWith(नूतन Ast_नियोजय{nodep->fileline(), lhsp, strp});
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम;
            }
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_underRecFunc);
        यदि (nodep->recursive()) m_underRecFunc = सत्य;
        iterateChildren(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_CCall* nodep) override {
        iterateChildren(nodep);
        m_hasJumpDelay = सत्य;  // As don't analyze inside tasks for timing controls
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        // Note excludes AstFuncRef as other visitor below
        iterateChildren(nodep);
        m_hasJumpDelay = सत्य;  // As don't analyze inside tasks for timing controls
    }
    शून्य दर्शय(AstFuncRef* nodep) override {
        दर्शय(static_cast<Ast_ग्रन्थिः_FTask_Ref*>(nodep));
        यदि (m_params) {  // Only parameters force us to do constant function call propagation
            replaceWithSimulation(nodep);
        }
    }
    शून्य दर्शय(AstArg* nodep) override {
        // replaceWithSimulation on the Arg's parent FuncRef replaces these
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstLoop* nodep) override {
        VL_RESTORER(m_hasLoopTest);
        ध्रुव तार्किक oldHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = असत्य;
        m_hasLoopTest = असत्य;
        iterateChildren(nodep);
        ध्रुव तार्किक thisLoopHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = thisLoopHasJumpDelay || oldHasJumpDelay;
        // If the first statement always break, the loop is useless
        यदि (ध्रुव AstLoopTest* ध्रुव testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            यदि (testp->condp()->isZero() && (!testp->nextp() || canDeleteStatements())) {
                nodep->v3warn(UNUSEDLOOP, "Loop condition is always false");
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                प्रत्यागम;
            }
        }
        // If last statement always breaks, repalce loop with body
        यदि (Ast_ग्रन्थिः* lastp = nodep->stmtsp()) {
            यावत् (Ast_ग्रन्थिः* ध्रुव nextp = lastp->nextp()) lastp = nextp;
            यदि (AstLoopTest* ध्रुव testp = VN_CAST(lastp, LoopTest)) {
                यदि (testp->condp()->isZero()) {
                    VL_DO_DANGLING2(प्रक्षिप_Deletep(testp->unlink_Fr_पृष्ठभागः()), testp, lastp);
                    nodep->replaceWith(nodep->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम;
                }
            }
        }
        // Warn on infinite loop
        यदि (!m_hasLoopTest && !thisLoopHasJumpDelay) {
            nodep->v3warn(INFINITELOOP, "Infinite loop (condition always true)");
            nodep->fileline()->modifyWarnOff(V3_Error_संहिता::INFINITELOOP, सत्य);  // Complain once
        }
    }
    शून्य दर्शय(AstLoopTest* nodep) override {
        iterateChildren(nodep);

        // If never breaks, remove
        यदि (nodep->condp()->isNeqZero()) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }

        m_hasLoopTest = सत्य;

        // If always breaks, subsequent statements are dead code, delete them
        यदि (nodep->condp()->isZero()) {
            Ast_ग्रन्थिः* ध्रुव nextp = nodep->nextp();
            यदि (nextp && canDeleteStatements()) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(nextp->unlink_Fr_पृष्ठभागः_With_अग्र()), nodep);
            }
            प्रत्यागम;
        }

        यदि (सङ्कार्यम्_Bool_सरणम्(nodep->condp())) replace_Bool_सरणम्(nodep->condp());
    }

    शून्य दर्शय(AstConsArray* nodep) override { iterateChildren(nodep); }
    शून्य दर्शय(AstConsStruct* nodep) override { iterateChildren(nodep); }
    शून्य दर्शय(AstConsStructItem* nodep) override { iterateChildren(nodep); }
    शून्य दर्शय(AstConsArrayItem* nodep) override { iterateChildren(nodep); }
    शून्य दर्शय(AstUnbounded* nodep) override { iterateChildren(nodep); }
    // These are converted by V3Param.  Don't constify as we don't want the
    // from() VARREF to disappear, if any.
    // If output of a presel didn't get consted, chances are V3Param didn't visit properly
    शून्य दर्शय(Ast_ग्रन्थिः_Pre_Sel*) override {}

    // Ignored, can eliminate early
    शून्य दर्शय(AstSysIgnore* nodep) override {
        iterateChildren(nodep);
        यदि (m_doNConst) VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }

    शून्य दर्शय(Ast_Stmt_व्यञ्जकम्* nodep) override {
        iterateChildren(nodep);
        // Malformed due to child being deleted, remove here
        यदि (!nodep->exprp()) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }
        // Remove if expression is trivially pure
        यदि (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), VarRef)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }
        // Remove calls to empty functions
        यदि (ध्रुव AstCCall* ध्रुव callp = VN_CAST(nodep->exprp(), CCall)) {
            ध्रुव AstCFunc* ध्रुव funcp = callp->funcp();
            यदि (!callp->argsp() && funcp->रिक्तानि_Body()) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                प्रत्यागम;
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
    शून्य दर्शय(AstBasicDType* nodep) override {
        iterateChildren(nodep);
        nodep->cvtRangeConst();
    }

    //-----
    // Jump elimination

    शून्य दर्शय(AstJumpGo* nodep) override {
        // Any statements following the JumpGo (at this statement level) never execute, delete
        यदि (nodep->nextp() && canDeleteStatements()) {
            प्रक्षिप_Deletep(nodep->nextp()->unlink_Fr_पृष्ठभागः_With_अग्र());
        }

        // JumpGo as last statement in target JumpBlock (including last in a last sub-list),
        // is a no-op, remove it.
        यदि (!nodep->nextp()) {
            प्रति (Ast_ग्रन्थिः* abovep = nodep->abovep(); abovep; abovep = abovep->abovep()) {
                यदि (abovep == nodep->blockp()) {
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                    प्रत्यागम;
                }
                // Stop if not doing expensive, or if the above node is not the last in its list,
                // ... or if it's not an 'if' TODO: it would be enough if it was not a branch.
                यदि (!m_doExpensive || abovep->nextp() || !VN_IS(abovep, If)) भङ्ग;
            }
        }
        // Mark JumpBlock as used
        m_प्रयुक्तम्_Jump_खण्डानि.emplace(nodep->blockp());
        m_hasJumpDelay = सत्य;
    }

    शून्य दर्शय(Ast_Jump_खण्डम्* nodep) override {
        iterateChildren(nodep);

        // If first statement is an AstLoopTest, pull it before the jump block
        यदि (AstLoopTest* ध्रुव testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            nodep->सङ्कलय_अत्र_This_As_अग्र(testp->unlink_Fr_पृष्ठभागः());
        }

        // Remove if empty
        यदि (!nodep->stmtsp()) {
            UINFO(4, "JUMPLABEL => empty " << nodep);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }

        // If no JumpGo points to this node, replace it with its body
        यदि (!m_प्रयुक्तम्_Jump_खण्डानि.गणनम्(nodep)) {
            UINFO(4, "JUMPLABEL => unused " << nodep);
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }

    //-----
    // Below lines are magic expressions processed by astgen
    //  TREE_SKIP_VISIT("AstNODETYPE")    # Rename normal visit to visitGen and don't iterate
    //-----
    // clang-format off
    वृक्षः_SKIP_दर्शय("ArraySel");
    वृक्षः_SKIP_दर्शय("CAwait");

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
    //वाक्यानि
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
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        // Default: Just iterate
        यदि (m_required) {
            यदि (VN_IS(nodep, ग्रन्थिः_DType) || VN_IS(nodep, Range) || VN_IS(nodep, खण्डिका_Sel)
                || VN_IS(nodep, Dot) || VN_IS(nodep, Text)) {
                // ignore
            } अन्यथा यदि (VN_IS(nodep, आदर्शः) || VN_IS(nodep, PatMember)) {
                // A parameter override pattern is typed later, so only fold its members
                iterateChildren(nodep);
            } अन्यथा यदि (AstCellRef* ध्रुव crp = VN_CAST(nodep, CellRef)) {
                iterate(crp->exprp());
                यदि (Ast_ग्रन्थिः* ध्रुव newp = crp->exprp()) {
                    crp->replaceWithKeepDType(newp->unlink_Fr_पृष्ठभागः());
                    VL_DO_DANGLING(प्रक्षिप_Deletep(crp), crp);
                }
                प्रत्यागम;
            } अन्यथा {
                nodep->v3error("Expecting expression to be constant, but can't convert a "
                               << nodep->prettyTypeName() << " to constant.");
            }
        } अन्यथा {
            यदि (nodep->isTimingControl()) m_hasJumpDelay = सत्य;
            // Calculate the width of this operation
            यदि (m_params && !nodep->विस्तारः()) nodep = V3_विस्तारः::विस्तारः_Params_Edit(nodep);
            iterateChildren(nodep);
        }
    }

सार्वजनिक:
    // Processing Mode Enum
    गणना Proc_बहुलकम् : uint8_t {
        PROC_PARAMS_NOWARN,
        PROC_PARAMS,
        PROC_GENERATE,
        PROC_जीवितम्,
        PROC_V_WARN,
        PROC_V_NOWARN,
        PROC_V_EXPENSIVE,
        PROC_CPP
    };

    // CONSTRUCTORS
    ConstVisitor(Proc_बहुलकम् pmode, तार्किक globalPass)
        : m_globalPass{globalPass}
        , m_concswap_नामानि{globalPass ? ("__Vconcswap_" + cvt_गन्तव्यम्_Str(s_global_Pass_अङ्कः++)) : ""} {
        // clang-format off
        विकल्प (pmode) {
        पक्ष PROC_PARAMS_NOWARN:  m_doV = सत्य;  m_doNConst = सत्य; m_params = सत्य;
                                  m_required = असत्य; भङ्ग;
        पक्ष PROC_PARAMS:         m_doV = सत्य;  m_doNConst = सत्य; m_params = सत्य;
                                  m_required = सत्य; भङ्ग;
        पक्ष PROC_GENERATE:       m_doV = सत्य;  m_doNConst = सत्य; m_params = सत्य;
                                  m_required = सत्य; m_doGenerate = सत्य; भङ्ग;
        पक्ष PROC_जीवितम्:           भङ्ग;
        पक्ष PROC_V_WARN:         m_doV = सत्य;  m_doNConst = सत्य; m_warn = सत्य; m_convert_Logic_गन्तव्यम्_Bit = सत्य; भङ्ग;
        पक्ष PROC_V_NOWARN:       m_doV = सत्य;  m_doNConst = सत्य; भङ्ग;
        पक्ष PROC_V_EXPENSIVE:    m_doV = सत्य;  m_doNConst = सत्य; m_doExpensive = सत्य; भङ्ग;
        पक्ष PROC_CPP:            m_doV = असत्य; m_doNConst = सत्य; m_doCpp = सत्य; भङ्ग;
        स्वभाव:                  v3fatalSrc("Bad case"); भङ्ग;
        }
        // clang-format on
    }
    ~ConstVisitor() override {
        यदि (m_doCpp) {
            यदि (m_globalPass) {
                V3Stats::सङ्कलय_Stat("Optimizations, Const bit op reduction", m_statBitOpReduction);
            } अन्यथा {
                V3Stats::सङ्कलय_Stat_योग("Optimizations, Const bit op reduction", m_statBitOpReduction);
            }
        }
        V3Stats::सङ्कलय_Stat_योग("Optimizations, Cond redundant expressions", m_stat_Cond_व्यञ्जकम्_Redundant);
        V3Stats::सङ्कलय_Stat_योग("Optimizations, If cond redundant expressions",
                            m_stat_If_Cond_व्यञ्जकम्_Redundant);
        V3Stats::सङ्कलय_Stat_योग("Optimizations, Concat merges", m_statConcatMerge);
        V3Stats::सङ्कलय_Stat_योग("Optimizations, Member access predicate node visits",
                            m_statMemberAccessVisits);
    }

    Ast_ग्रन्थिः* mainAcceptEdit(Ast_ग्रन्थिः* nodep) {
        VIsCached::clear_Cache_वृक्षः();  // Avoid using any stale isPure
        // Operate starting at a random place
        प्रत्यागम iterateSubtreeReturnEdits(nodep);
    }
};

uint32_t ConstVisitor::s_global_Pass_अङ्कः = 0;

//######################################################################
// Const class functions

//! Force this cell node's parameter list to become a constant
शून्य V3Const::constifyParamsEdit(Ast_ग्रन्थिः* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3_विस्तारः::विस्तारः_Params_Edit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS, /* globalPass: */ असत्य};
    // cppcheck-suppress constVariablePointer // edited below
    यदि (AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        यदि (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } अन्यथा {
        (शून्य)visitor.mainAcceptEdit(nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump if have next line
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

//! Constify this cell node's parameter list if possible
शून्य V3Const::constifyParamsNoWarnEdit(Ast_ग्रन्थिः* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3_विस्तारः::विस्तारः_Params_Edit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS_NOWARN, /* globalPass: */ असत्य};
    // cppcheck-suppress constVariablePointer // edited below
    यदि (AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        यदि (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } अन्यथा {
        (शून्य)visitor.mainAcceptEdit(nodep);
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
शून्य V3Const::constifyGenerateParamsEdit(Ast_ग्रन्थिः* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE:");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized
    // yet.

    // Make sure we've sized everything first
    nodep = V3_विस्तारः::विस्तारः_Generate_Params_Edit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_GENERATE, /* globalPass: */ असत्य};
    // cppcheck-suppress constVariablePointer // edited below
    यदि (AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        यदि (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } अन्यथा {
        VL_DO_DANGLING(visitor.mainAcceptEdit(nodep), nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump this.
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

शून्य V3Const::constifyAllLint(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_WARN, /* globalPass: */ सत्य};
        (शून्य)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("const", 0, dump_वृक्षः_Either_Level() >= 3);
}

शून्य V3Const::constifyCpp(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ सत्य};
        (शून्य)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("const_cpp", 0, dump_वृक्षः_Either_Level() >= 3);
}

Ast_ग्रन्थिः* V3Const::constifyEdit(Ast_ग्रन्थिः* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_NOWARN, /* globalPass: */ असत्य};
    nodep = visitor.mainAcceptEdit(nodep);
    प्रत्यागम nodep;
}

Ast_ग्रन्थिः* V3Const::constifyEditCpp(Ast_ग्रन्थिः* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ असत्य};
    nodep = visitor.mainAcceptEdit(nodep);
    प्रत्यागम nodep;
}

शून्य V3Const::constify_All_जीवितम्(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    // This only pushes constants up, doesn't make any other edits
    // IE doesn't prune dead statements, as we need to do some usability checks after this
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_जीवितम्, /* globalPass: */ सत्य};
        (शून्य)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("const", 0, dump_वृक्षः_Either_Level() >= 3);
}

शून्य V3Const::constifyAll(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ सत्य};
        (शून्य)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("const", 0, dump_वृक्षः_Either_Level() >= 3);
}

Ast_ग्रन्थिः* V3Const::constifyExpensiveEdit(Ast_ग्रन्थिः* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ असत्य};
    nodep = visitor.mainAcceptEdit(nodep);
    प्रत्यागम nodep;
}
