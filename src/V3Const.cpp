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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Const.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Simulate.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3String.h"
#అన్తర్భావ "V3UniqueNames.h"
#అన్తర్భావ "V3Width.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <memory>
#అన్తర్భావ <type_traits>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

#నిర్వచించు వృక్షం_SKIP_సందర్శించు(...)
#నిర్వచించు TREEOP1(...)
#నిర్వచించు TREEOPA(...)
#నిర్వచించు TREEOP(...)
#నిర్వచించు TREEOPS(...)
#నిర్వచించు TREEOPC(...)
#నిర్వచించు TREEOPV(...)

//######################################################################
// Utilities

స్థిర తార్కిక isConst(స్థిరరాశి Ast_కణుపు* nodep, uint64_t v) {
    స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const);
    తిరిగి constp && constp->గమ్యం_UQuad() == v;
}

మూస <రకం_పేరు T>
స్థిర రకం_పేరు std::enable_if<std::is_integral<T>::విలువ, తార్కిక>::type isPow2(T val) {
    తిరిగి (val & (val - 1)) == 0;
}

స్థిర పూర్ణాంకం లెక్క_Trailing_Zeroes(uint64_t val) {
    UASSERT(val, "countTrailingZeroes argument must be non-zero");
#ఒకవేళ defined(__GNUC__) && !defined(VL_NO_BUILTINS)
    తిరిగి __builtin_ctzll(val);
#లేకపోతే
    పూర్ణాంకం బిట్టు_2 = 0;
    val = ~val;
    అయితే (val & 1) {
        ++బిట్టు_2;
        val >>= 1;
    }
    తిరిగి బిట్టు_2;
#అయితే_ముగింపు
}

// Allocates a unique id for each AstNode it is applied to, held in user4.
తరగతి VNIdAllocator final {
    // NODE STATE
    // AstNode::user4p -> size_t. Unique id of the node (0/nullptr if not allocated yet)

    // MEMBERS
    // Claimed lazily on first use TODO: fix conflict with V3Param user4 slot
    std::unique_ptr<VNUser4_లోపల_Use> m_inuser4p;
    కొలత_t m_తదుపరి_Id = 0;  // Id allocated most recently

బహిరంగ:
    // Return the unique id of the given node, allocating a new one if it has none yet
    కొలత_t క్రియాకారకం()(Ast_కణుపు* nodep) {
        ఒకవేళ (!m_inuser4p) m_inuser4p = std::make_unique<VNUser4_లోపల_Use>();  // Claim on first use
        ఒకవేళ (!nodep->user4p()) nodep->user4p(reinterpret_cast<శూన్య*>(++m_తదుపరి_Id));
        తిరిగి reinterpret_cast<కొలత_t>(nodep->user4p());
    }
};

// This visitor can be used in the post-expanded Ast from V3Expand, where the Ast satisfies:
// - Constants are 64 bit at most (because words are accessed via AstWordSel)
// - Variables are scoped.
తరగతి Const_బిట్టు_Op_వృక్షం_Visitor final : బహిరంగ VNVisitorConst {
    // TYPES

    // Holds a node to be added as a term in the reduction tree, it's equivalent op count, and a
    // bool indicating if the term is clean (0/1 value, or if the top bits might be dirty)
    ఉపయోగించి ఫలితం_పదం = std::tuple<Ast_కణుపు_సమాసం*, గుర్తులేని, తార్కిక>;

    తరగతి ఆకు_Info final {  // Leaf node (either AstConst or AstVarRef)
        // MEMBERS
        తార్కిక m_polarity = నిజం;  // Invert result due to NOT
        పూర్ణాంకం m_lsb = 0;  // LSB of actually used bit of m_refp->varp()
        పూర్ణాంకం m_msb = 0;  // MSB of actually used bit of m_refp->varp()
        పూర్ణాంకం m_మాట_Idx = -1;  // -1 means AstWordSel is not used.
        AstVarRef* m_refp = శూన్యసూచిక;  // Leaf's variable reference
        స్థిరరాశి AstConst* m_constp = శూన్యసూచిక;  // Leaf's constant

    బహిరంగ:
        // CONSTRUCTORS
        ఆకు_Info() = అప్రమేయ;
        ఆకు_Info(స్థిరరాశి ఆకు_Info& other) = అప్రమేయ;
        ఆకు_Info& క్రియాకారకం=(స్థిరరాశి ఆకు_Info& other) = అప్రమేయ;
        ఆకు_Info(ఆకు_Info&& other) = అప్రమేయ;
        ఆకు_Info& క్రియాకారకం=(ఆకు_Info&& other) = అప్రమేయ;
        స్పష్టమైన ఆకు_Info(పూర్ణాంకం lsb)
            : m_lsb{lsb} {}

        // METHODS
        శూన్య సమితి_ఆకు(AstVarRef* refp) {
            UASSERT_OBJ(!m_refp && !m_constp, refp, "Must be called just once");
            m_refp = refp;
            m_msb = refp->varp()->వెడల్పు_Min() - 1;
        }
        శూన్య సమితి_ఆకు(స్థిరరాశి AstConst* constp) {
            UASSERT_OBJ(!m_refp && !m_constp, constp, "Must be called just once");
            m_constp = constp;
            m_msb = constp->వెడల్పు_Min() - 1;
        }
        // updateBitRange(), limitBitRangeToLsb(), and polarity() must be called during ascending
        // back to the root.
        శూన్య update_బిట్టు_Range(పూర్ణాంకం newLsb, పూర్ణాంకం newMsb) {
            ఒకవేళ ((m_lsb <= m_msb && newLsb > newMsb) || (m_lsb > m_msb && m_lsb < newLsb)) {
                // When the new bit range is out of m_refp, clear polarity because nodes below is
                // shifted out to zero.
                // This kind of clear may happen several times. e.g. (!(1'b1 >> 1)) >> 1
                polarity(నిజం);
            }
            m_lsb = newLsb;
            m_msb = newMsb;
        }
        శూన్య update_బిట్టు_Range(స్థిరరాశి AstCCast* castp) {
            update_బిట్టు_Range(m_lsb, std::min(m_msb, m_lsb + castp->వెడల్పు() - 1));
        }
        శూన్య update_బిట్టు_Range(స్థిరరాశి Ast_జరుపు_R* shiftp) {
            update_బిట్టు_Range(m_lsb + VN_AS(shiftp->rhsp(), Const)->గమ్యం_UInt(), m_msb);
        }
        శూన్య హద్దు_బిట్టు_Range_గమ్యం_Lsb() { update_బిట్టు_Range(m_lsb, std::min(m_msb, m_lsb)); }
        పూర్ణాంకం మాట_Idx() స్థిరరాశి { తిరిగి m_మాట_Idx; }
        శూన్య మాట_Idx(పూర్ణాంకం i) { m_మాట_Idx = i; }
        తార్కిక polarity() స్థిరరాశి { తిరిగి m_polarity; }
        శూన్య polarity(తార్కిక p) { m_polarity = p; }

        AstVarRef* refp() స్థిరరాశి { తిరిగి m_refp; }
        స్థిరరాశి AstConst* constp() స్థిరరాశి { తిరిగి m_constp; }
        తార్కిక missing_మాట_Sel() స్థిరరాశి {
            // When V3Expand is skipped, WordSel is not inserted.
            తిరిగి m_refp->isWide() && m_మాట_Idx == -1;
        }
        పూర్ణాంకం lsb() స్థిరరాశి { తిరిగి m_lsb; }

        పూర్ణాంకం msb() స్థిరరాశి { తిరిగి std::min(m_msb, var_వెడల్పు() - 1); }
        పూర్ణాంకం var_వెడల్పు() స్థిరరాశి {
            UASSERT(m_refp, "m_refp should be set");
            స్థిరరాశి పూర్ణాంకం వెడల్పు = m_refp->varp()->వెడల్పు_Min();
            ఒకవేళ (!m_refp->isWide()) {
                UASSERT_OBJ(m_మాట_Idx == -1, m_refp, "Bad word index into non-wide");
                తిరిగి వెడల్పు;
            } లేకపోతే {
                ఒకవేళ (missing_మాట_Sel()) తిరిగి వెడల్పు;
                UASSERT_OBJ(m_మాట_Idx >= 0, m_refp, "Bad word index into wide");
                స్థిరరాశి పూర్ణాంకం బిట్లు_లోపల_MSW = VL_BITBIT_E(వెడల్పు) ? VL_BITBIT_E(వెడల్పు) : VL_EDATASIZE;
                తిరిగి m_మాట_Idx == m_refp->వెడల్పు_పదాలు() - 1 ? బిట్లు_లోపల_MSW : VL_EDATASIZE;
            }
        }
    };

    నిర్మాణం బిట్టు_Polarity_నమోదు final {  // Found bit polarity during iterate()
        ఆకు_Info m_info;  // Leaf (variable or constant) bit polarity was found on
        తార్కిక m_polarity = అబద్ధం;  // Polarity the bit must have to match
        పూర్ణాంకం m_బిట్టు = 0;  // Bit index within the leaf that was tested
        బిట్టు_Polarity_నమోదు(స్థిరరాశి ఆకు_Info& info, తార్కిక pol, పూర్ణాంకం బిట్టు_2)
            : m_info{info}
            , m_polarity{pol}
            , m_బిట్టు{బిట్టు_2} {}
        బిట్టు_Polarity_నమోదు() = అప్రమేయ;
    };

    నిర్మాణం Frozen_కణుపు_Info final {  // Context when a frozen node is found
        తార్కిక m_polarity;  // Polarity the frozen node must match
        పూర్ణాంకం m_lsb;  // LSB position of the frozen node
        తార్కిక క్రియాకారకం<(స్థిరరాశి Frozen_కణుపు_Info& other) స్థిరరాశి {
            ఒకవేళ (m_lsb != other.m_lsb) తిరిగి m_lsb < other.m_lsb;
            తిరిగి m_polarity < other.m_polarity;
        }
    };

    తరగతి Restorer final {  // Restore the original state unless disableRestore() is called
        Const_బిట్టు_Op_వృక్షం_Visitor& m_visitor;  // Visitor whose state is saved and restored
        స్థిరరాశి కొలత_t m_polarities_కొలత;  // Saved m_visitor.m_bitPolarities size to truncate back to
        స్థిరరాశి కొలత_t m_frozen_కొలత;  // Saved m_visitor.m_frozenNodes size to truncate back to
        స్థిరరాశి గుర్తులేని m_ops;  // Saved m_visitor.m_ops to restore
        స్థిరరాశి తార్కిక m_polarity;  // Saved m_visitor.m_polarity to restore
        తార్కిక m_restore = నిజం;  // Whether the destructor still needs to restore state

    బహిరంగ:
        స్పష్టమైన Restorer(Const_బిట్టు_Op_వృక్షం_Visitor& visitor)
            : m_visitor{visitor}
            , m_polarities_కొలత{visitor.m_బిట్టు_Polarities.కొలత()}
            , m_frozen_కొలత{visitor.m_frozenNodes.కొలత()}
            , m_ops{visitor.m_ops}
            , m_polarity{visitor.m_polarity} {}
        ~Restorer() {
            UASSERT(m_visitor.m_బిట్టు_Polarities.కొలత() >= m_polarities_కొలత,
                    "m_bitPolarities must grow monotonically");
            UASSERT(m_visitor.m_frozenNodes.కొలత() >= m_frozen_కొలత,
                    "m_frozenNodes must grow monotonically");
            ఒకవేళ (m_restore) restoreNow();
        }
        శూన్య disableRestore() { m_restore = అబద్ధం; }
        శూన్య restoreNow() {
            UASSERT(m_restore, "Can be called just once");
            m_visitor.m_బిట్టు_Polarities.resize(m_polarities_కొలత);
            m_visitor.m_frozenNodes.resize(m_frozen_కొలత);
            m_visitor.m_ops = m_ops;
            m_visitor.m_polarity = m_polarity;
            m_restore = అబద్ధం;
        }
    };
    // Collect information for each Variable to transform as below
    తరగతి VarInfo final {
        // MEMBERS
        పూర్ణాంకం m_తెలిసిన_ఫలితం = -1;  // -1: result is not known, 0 or 1: result of this tree
        స్థిరరాశి Const_బిట్టు_Op_వృక్షం_Visitor* స్థిరరాశి
            m_parentp;  // ConstBitOpTreeVisitor holding this VarInfo
        AstVarRef* స్థిరరాశి m_refp;  // Points the variable that this VarInfo covers
        స్థిరరాశి పూర్ణాంకం m_వెడల్పు;  // Width of term this VarInfo refers to
        V3_సంఖ్య m_బిట్టు_Polarity;  // Coefficient of each bit

    బహిరంగ:
        // METHODS
        తార్కిక has_Const_ఫలితం() స్థిరరాశి { తిరిగి m_తెలిసిన_ఫలితం >= 0 || m_బిట్టు_Polarity.isAllX(); }
        // The constant result. Only valid if hasConstResult() returned true.
        తార్కిక get_Const_ఫలితం() స్థిరరాశి {
            // Note that this condition covers m_knownResult == -1 but m_bitPolarity.isAllX(),
            // in which case the result is 0
            తిరిగి m_తెలిసిన_ఫలితం == 1;
        }
        స్థిరరాశి AstVarRef* refp() స్థిరరాశి { తిరిగి m_refp; }
        తార్కిక sameVarAs(స్థిరరాశి Ast_కణుపు_Var_Ref* otherp) స్థిరరాశి { తిరిగి m_refp->same_కణుపు(otherp); }
        శూన్య సమితి_Polarity(తార్కిక comp_బిట్టు, పూర్ణాంకం బిట్టు_2) {
            // Ignore if already determined a known reduction
            ఒకవేళ (m_తెలిసిన_ఫలితం >= 0) తిరిగి;
            UASSERT_OBJ(బిట్టు_2 < m_వెడల్పు, m_refp,
                        "Bit index out of range: " << బిట్టు_2 << " width: " << m_వెడల్పు);
            ఒకవేళ (m_బిట్టు_Polarity.బిట్టు_Is_X(బిట్టు_2)) {  // The bit is not yet marked with either polarity
                m_బిట్టు_Polarity.సమితి_బిట్టు(బిట్టు_2, comp_బిట్టు);
            } లేకపోతే {  // The bit has already been marked with some polarity
                స్థిరరాశి తార్కిక sameFlag = m_బిట్టు_Polarity.బిట్టు_Is1(బిట్టు_2) == comp_బిట్టు;
                ఒకవేళ (m_parentp->is_Xor_వృక్షం()) {
                    UASSERT_OBJ(comp_బిట్టు && sameFlag, m_refp, "Only true is set in Xor tree");
                    // a ^ a ^ b == b so we can ignore a
                    m_బిట్టు_Polarity.సమితి_బిట్టు(బిట్టు_2, 'x');
                } లేకపోతే {  // And, Or
                    // Can ignore this nodep as the bit is already marked with the same polarity
                    ఒకవేళ (sameFlag) తిరిగి;  // a & a == a, b | b == b
                    // Otherwise result is constant (a & ~a == 0) or (a | ~a == 1)
                    m_తెలిసిన_ఫలితం = m_parentp->is_And_వృక్షం() ? 0 : 1;
                    m_బిట్టు_Polarity.సమితి_All_బిట్లు_X();  // The variable is not referred anymore
                }
            }
        }

        // Return reduction term for this VarInfo, together with the number of ops in the term,
        // and a boolean indicating if the term is clean (1-bit vs multi-bit value)
        ఫలితం_పదం get_ఫలితం_పదం() స్థిరరాశి {
            UASSERT_OBJ(!has_Const_ఫలితం(), m_refp, "getTerm on reduction that yields constant");
            File_పంక్తి* స్థిరరాశి fl = m_refp->fileline();

            // Get the term we are referencing (the WordSel, if wide, otherwise just the VarRef)
            Ast_కణుపు_సమాసం* srcp = VN_CAST(m_refp->backp(), మాట_Sel);
            ఒకవేళ (!srcp) srcp = m_refp;
            srcp = srcp->clone_వృక్షం(అబద్ధం);

            // Signed variables might have redundant sign bits that need masking.
            స్థిరరాశి తార్కిక has_Redundant_Sign_బిట్లు
                = m_refp->varp()->dtypep()->isSigned()
                  && (m_refp->isWide() ? (m_వెడల్పు != VL_EDATASIZE)
                                       : (m_వెడల్పు < 8 || !isPow2(m_వెడల్పు)));

            // Get the mask that selects the bits that are relevant in this term
            V3_సంఖ్య ముసుగు_అంకె{srcp, m_వెడల్పు, 0};
            ముసుగు_అంకె.op_బిట్లు_Non_XZ(m_బిట్టు_Polarity);  // 'x' -> 0, 0->1, 1->1
            స్థిరరాశి uint64_t ముసుగు_Val = ముసుగు_అంకె.గమ్యం_UQuad();
            UASSERT_OBJ(ముసుగు_Val != 0, m_refp,
                        "Should have been recognized as having const 0 result");

            // Parts of the return value
            Ast_కణుపు_సమాసం* resultp = srcp;  // The tree for this term
            గుర్తులేని ops = 0;  // Number of ops in this term
            తార్కిక clean = అబద్ధం;  // Whether the term is clean (has value 0 or 1)

            ఒకవేళ (isPow2(ముసుగు_Val)) {
                // If we only want a single bit, shift it out instead of a masked compare. Shifts
                // don't go through the flags register on x86 and are hence faster. This is also
                // always fewer or same ops as mask and compare, but with shorter instructions on
                // x86.

                // Find the index of the bit we want.
                స్థిరరాశి పూర్ణాంకం బిట్టు_2 = లెక్క_Trailing_Zeroes(ముసుగు_Val);
                // If we want something other than the bottom bit, shift it out
                ఒకవేళ (బిట్టు_2 != 0) {
                    resultp = కొత్త Ast_జరుపు_R{fl, resultp,
                                            కొత్త AstConst{fl, static_cast<uint32_t>(బిట్టు_2)}, m_వెడల్పు};
                    ++ops;
                }
                // Negate it if necessary
                స్థిరరాశి తార్కిక negate = m_బిట్టు_Polarity.బిట్టు_Is0(బిట్టు_2);
                ఒకవేళ (negate) {
                    resultp = కొత్త AstNot{fl, resultp};
                    ++ops;
                }
                // Clean if MSB of unsigned value, and not negated
                clean = (బిట్టు_2 == m_వెడల్పు - 1) && !has_Redundant_Sign_బిట్లు && !negate;
            } లేకపోతే {
                // We want multiple bits. Go ahead and extract them.

                // Check if masking is required, and if so apply it
                స్థిరరాశి తార్కిక needsMasking = ముసుగు_Val != VL_ముసుగు_Q(m_వెడల్పు) || has_Redundant_Sign_బిట్లు;
                ఒకవేళ (needsMasking) {
                    resultp = కొత్త AstAnd{fl, కొత్త AstConst{fl, ముసుగు_అంకె}, resultp};
                    ++ops;
                }

                // Create the sub-expression for this term
                ఒకవేళ (m_parentp->is_Xor_వృక్షం()) {
                    ఒకవేళ (needsMasking) {
                        // Reduce the masked term to the minimum known width,
                        // to use the smallest RedXor formula
                        స్థిరరాశి పూర్ణాంకం వెడల్పు_Min = ముసుగు_అంకె.వెడల్పు_గమ్యం_Fit();
                        resultp->dtype_Chg_వెడల్పు(వెడల్పు_Min, వెడల్పు_Min);
                    }
                    resultp = కొత్త AstRedXor{fl, resultp};
                    ++ops;
                    clean = అబద్ధం;
                    // VL_REDXOR_* returns IData, set width accordingly to avoid unnecessary casts
                    resultp->dtype_Chg_వెడల్పు(VL_IDATASIZE, 1);
                } లేకపోతే ఒకవేళ (m_parentp->is_And_వృక్షం()) {
                    V3_సంఖ్య comp_అంకె{srcp, m_వెడల్పు, 0};
                    comp_అంకె.op_బిట్లు_One(m_బిట్టు_Polarity);  // 'x'->0, 0->0, 1->1
                    resultp = కొత్త AstEq{fl, కొత్త AstConst{fl, comp_అంకె}, resultp};
                    ++ops;
                    clean = నిజం;
                } లేకపోతే {  // Or
                    V3_సంఖ్య comp_అంకె{srcp, m_వెడల్పు, 0};
                    comp_అంకె.op_బిట్లు_One(m_బిట్టు_Polarity);  // 'x'->0, 0->0, 1->1
                    comp_అంకె.opXor(V3_సంఖ్య{comp_అంకె}, ముసుగు_అంకె);
                    resultp = కొత్త AstNeq{fl, కొత్త AstConst{fl, comp_అంకె}, resultp};
                    ++ops;
                    clean = నిజం;
                }
            }

            తిరిగి ఫలితం_పదం{resultp, ops, clean};
        }

        // CONSTRUCTORS
        VarInfo(Const_బిట్టు_Op_వృక్షం_Visitor* మాతృ, AstVarRef* refp, పూర్ణాంకం వెడల్పు)
            : m_parentp{మాతృ}
            , m_refp{refp}
            , m_వెడల్పు{వెడల్పు}
            , m_బిట్టు_Polarity{refp, m_వెడల్పు} {
            m_బిట్టు_Polarity.సమితి_All_బిట్లు_X();
        }
    };

    // MEMBERS
    తార్కిక m_failed = అబద్ధం;
    తార్కిక m_polarity = నిజం;  // Flip when AstNot comes
    గుర్తులేని m_ops;  // Number of operations such as And, Or, Xor, Sel...
    పూర్ణాంకం m_lsb = 0;  // Current LSB
    ఆకు_Info* m_leafp = శూన్యసూచిక;  // AstConst or AstVarRef that currently looking for
    స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి m_rootp;  // Root of this AST subtree

    std::సదిశ<std::pair<Ast_కణుపు_సమాసం*, Frozen_కణుపు_Info>>
        m_frozenNodes;  // Nodes that cannot be optimized
    std::సదిశ<బిట్టు_Polarity_నమోదు> m_బిట్టు_Polarities;  // Polarity of bits found during iterate()
    std::సదిశ<std::unique_ptr<VarInfo>> m_varInfos;  // VarInfo for each variable, [0] is nullptr
    VNIdAllocator& m_ids;  // Node id allocator, owned by ConstVisitor
    // Base index of m_varInfos that points VarInfo, keyed by AstVarScope/AstVarRef id,
    // zero means not set yet.
    std::unordered_map<కొలత_t, పూర్ణాంకం> m_పునాది_Idxs;

    // METHODS

    తార్కిక is_And_వృక్షం() స్థిరరాశి { తిరిగి VN_IS(m_rootp, And); }
    తార్కిక is_Or_వృక్షం() స్థిరరాశి { తిరిగి VN_IS(m_rootp, Or); }
    తార్కిక is_Xor_వృక్షం() స్థిరరాశి { తిరిగి VN_IS(m_rootp, Xor) || VN_IS(m_rootp, RedXor); }

#నిర్వచించు CONST_BITOP_RETURN_IF(cond, nodep) \
    ఒకవేళ (సమితి_Failed(cond, #cond, nodep, __LINE__)) తిరిగి

#నిర్వచించు CONST_BITOP_సమితి_FAILED(reason, nodep) సమితి_Failed(నిజం, reason, nodep, __LINE__)

    తార్కిక సమితి_Failed(తార్కిక fail, స్థిరరాశి అక్షరం* reason, స్థిరరాశి Ast_కణుపు* nodep, పూర్ణాంకం పంక్తి) {
        ఒకవేళ (fail && !m_failed) {
            UINFO(9, "cannot optimize " << m_rootp << " reason:" << reason << " called from line:"
                                        << పంక్తి << " when checking:" << nodep);
            // UINFOTREE(9, m_rootp, "", "root");
            m_failed = నిజం;
        }
        తిరిగి m_failed;
    }
    శూన్య incrOps(స్థిరరాశి Ast_కణుపు* nodep, పూర్ణాంకం పంక్తి) {
        ++m_ops;
        UINFO(9, "Increment to " << m_ops << " " << nodep << " called from line " << పంక్తి);
    }
    VarInfo& getVarInfo(స్థిరరాశి ఆకు_Info& ref) {
        UASSERT_OBJ(ref.refp(), m_rootp, "null varref in And/Or/Xor optimization");
        Ast_కణుపు* nodep = ref.refp()->varScopep();
        ఒకవేళ (!nodep) nodep = ref.refp()->varp();  // Not scoped
        పూర్ణాంకం& పునాది_Idxr = m_పునాది_Idxs[m_ids(nodep)];
        ఒకవేళ (పునాది_Idxr == 0) {  // Not set yet
            పునాది_Idxr = m_varInfos.కొలత();
            స్థిరరాశి పూర్ణాంకం అంకె_పదాలు
                = ref.refp()->dtypep()->isWide() ? ref.refp()->dtypep()->వెడల్పు_పదాలు() : 1;
            m_varInfos.resize(m_varInfos.కొలత() + అంకె_పదాలు);
        }
        స్థిరరాశి కొలత_t idx = పునాది_Idxr + std::max(0, ref.మాట_Idx());
        VarInfo* varInfop = m_varInfos[idx].get();
        ఒకవేళ (!varInfop) {
            varInfop = కొత్త VarInfo{ఇది, ref.refp(), ref.var_వెడల్పు()};
            m_varInfos[idx].reset(varInfop);
            ఒకవేళ (ref.missing_మాట_Sel()) {
                // ConstBitOpTreeVisitor makes some constants for masks and its type is uint64_t.
                // That's why V3Expand, that inserts WordSel, is needed.
                CONST_BITOP_సమితి_FAILED("V3Expand is skipped", ref.refp());
            }
        } లేకపోతే {
            ఒకవేళ (!varInfop->sameVarAs(ref.refp()))
                CONST_BITOP_సమితి_FAILED("different var (scope?)", ref.refp());
        }
        తిరిగి *varInfop;
    }

    // Traverse down to see AstConst or AstVarRef
    ఆకు_Info కనుగొను_ఆకు(Ast_కణుపు* nodep, తార్కిక expectConst) {
        ఒకవేళ (!nodep->dtypep()->skipRefp()->isIntegralOrPacked()) తిరిగి ఆకు_Info{};
        ఆకు_Info info{m_lsb};
        {
            VL_RESTORER(m_leafp);
            m_leafp = &info;
            iterateConst(nodep);
        }

        తార్కిక ok = !m_failed;
        ఒకవేళ (expectConst) {
            ok &= !info.refp() && info.constp();
        } లేకపోతే {
            ok &= info.refp() && !info.constp();
        }
        తిరిగి ok ? info : ఆకు_Info{};
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { CONST_BITOP_సమితి_FAILED("Hit unexpected op", nodep); }
    శూన్య సందర్శించు(AstCCast* nodep) override {
        iterateChildrenConst(nodep);
        ఒకవేళ (m_leafp) m_leafp->update_బిట్టు_Range(nodep);
    }
    శూన్య సందర్శించు(Ast_జరుపు_R* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->rhsp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->rhsp());
        m_lsb += constp->గమ్యం_UInt();
        incrOps(nodep, __LINE__);
        iterateConst(nodep->lhsp());
        m_leafp->update_బిట్టు_Range(nodep);
        m_lsb -= constp->గమ్యం_UInt();
    }
    శూన్య సందర్శించు(AstNot* nodep) override {
        CONST_BITOP_RETURN_IF(nodep->వెడల్పు_Min() != 1, nodep);
        Ast_కణుపు* lhsp = nodep->lhsp();
        AstCCast* స్థిరరాశి castp = VN_CAST(lhsp, CCast);
        ఒకవేళ (castp) lhsp = castp->lhsp();
        CONST_BITOP_RETURN_IF(!is_Xor_వృక్షం() && !VN_IS(lhsp, VarRef) && !VN_IS(lhsp, జరుపు_R), lhsp);
        incrOps(nodep, __LINE__);
        m_polarity = !m_polarity;
        iterateChildrenConst(nodep);
        // Don't restore m_polarity for Xor as it counts parity of the entire tree
        ఒకవేళ (!is_Xor_వృక్షం()) m_polarity = !m_polarity;
        ఒకవేళ (m_leafp && castp) m_leafp->update_బిట్టు_Range(castp);
        ఒకవేళ (m_leafp) m_leafp->polarity(!m_leafp->polarity());
    }
    శూన్య సందర్శించు(Ast_మాట_Sel* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->bitp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->bitp());
        UASSERT_OBJ(m_leafp->మాట_Idx() == -1, nodep, "Unexpected nested WordSel");
        m_leafp->మాట_Idx(constp->గమ్యం_SInt());
        iterateConst(nodep->fromp());
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->సమితి_ఆకు(nodep);
    }
    శూన్య సందర్శించు(AstConst* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->సమితి_ఆకు(nodep);
    }

    శూన్య సందర్శించు(AstRedXor* nodep) override {
        Restorer restorer{*ఇది};
        CONST_BITOP_RETURN_IF(!VN_IS(m_rootp, Xor), nodep);
        Ast_కణుపు* lhsp = nodep->lhsp();
        స్థిరరాశి AstCCast* స్థిరరాశి castp = VN_CAST(lhsp, CCast);
        ఒకవేళ (castp) lhsp = castp->lhsp();
        ఒకవేళ (స్థిరరాశి AstAnd* స్థిరరాశి andp = VN_CAST(lhsp, And)) {  // '^(mask & leaf)'
            CONST_BITOP_RETURN_IF(!andp, lhsp);

            స్థిరరాశి ఆకు_Info& ముసుగు = కనుగొను_ఆకు(andp->lhsp(), నిజం);
            CONST_BITOP_RETURN_IF(!ముసుగు.constp() || ముసుగు.lsb() != 0, andp->lhsp());

            ఆకు_Info ref = కనుగొను_ఆకు(andp->rhsp(), అబద్ధం);
            CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());
            ఒకవేళ (castp) ref.update_బిట్టు_Range(castp);

            restorer.disableRestore();  // Now all subtree succeeded

            స్థిరరాశి V3_సంఖ్య& ముసుగు_అంకె = ముసుగు.constp()->అంకె();

            incrOps(nodep, __LINE__);
            incrOps(andp, __LINE__);

            // Mark all bits checked in this reduction
            స్థిరరాశి పూర్ణాంకం max_బిట్టు_Idx = std::min(ref.lsb() + ముసుగు_అంకె.వెడల్పు(), ref.msb() + 1);
            కొరకు (పూర్ణాంకం బిట్టు_Idx = ref.lsb(); బిట్టు_Idx < max_బిట్టు_Idx; ++బిట్టు_Idx) {
                స్థిరరాశి పూర్ణాంకం ముసుగు_Idx = బిట్టు_Idx - ref.lsb();
                ఒకవేళ (ముసుగు_అంకె.బిట్టు_Is0(ముసుగు_Idx)) కొనసాగు;
                // Set true, m_polarity takes care of the entire parity
                m_బిట్టు_Polarities.emplace_వెనుక(ref, నిజం, బిట్టు_Idx);
            }
        } లేకపోతే {  // '^leaf'
            ఆకు_Info ref = కనుగొను_ఆకు(lhsp, అబద్ధం);
            CONST_BITOP_RETURN_IF(!ref.refp(), lhsp);
            ఒకవేళ (castp) ref.update_బిట్టు_Range(castp);

            restorer.disableRestore();  // Now all checks passed

            incrOps(nodep, __LINE__);

            // Mark all bits checked by this comparison
            కొరకు (పూర్ణాంకం బిట్టు_Idx = ref.lsb(); బిట్టు_Idx <= ref.msb(); ++బిట్టు_Idx) {
                m_బిట్టు_Polarities.emplace_వెనుక(ref, నిజం, బిట్టు_Idx);
            }
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు_Biop* nodep) override {
        ఒకవేళ (VN_IS(nodep, And) && isConst(nodep->lhsp(), 1)) {  // 1 & _
            // Always reach past a plain making AND
            Restorer restorer{*ఇది};
            incrOps(nodep, __LINE__);
            iterateConst(nodep->rhsp());
            ఒకవేళ (m_leafp) m_leafp->హద్దు_బిట్టు_Range_గమ్యం_Lsb();
            CONST_BITOP_RETURN_IF(m_failed, nodep->rhsp());
            restorer.disableRestore();  // Now all checks passed
        } లేకపోతే ఒకవేళ (nodep->type() == m_rootp->type()) {  // And, Or, Xor
            // subtree under NOT can be optimized only in XOR tree.
            CONST_BITOP_RETURN_IF(!m_polarity && !is_Xor_వృక్షం(), nodep);
            incrOps(nodep, __LINE__);
            కొరకు (స్థిరరాశి తార్కిక కుడి : {అబద్ధం, నిజం}) {
                VL_RESTORER(m_leafp);
                Restorer restorer{*ఇది};
                ఆకు_Info ఆకు_Info_2{m_lsb};
                // cppcheck-suppress danglingLifetime
                m_leafp = &ఆకు_Info_2;
                Ast_కణుపు_సమాసం* opp = కుడి ? nodep->rhsp() : nodep->lhsp();
                స్థిరరాశి తార్కిక origFailed = m_failed;
                iterateConst(opp);
                ఒకవేళ (ఆకు_Info_2.constp() || m_failed) {
                    // Revert changes in leaf
                    restorer.restoreNow();
                    // Reach past a cast then add to frozen nodes to be added to final reduction
                    ఒకవేళ (స్థిరరాశి AstCCast* స్థిరరాశి castp = VN_CAST(opp, CCast)) opp = castp->lhsp();
                    స్థిరరాశి తార్కిక pol = is_Xor_వృక్షం() || m_polarity;  // Only AND/OR tree needs polarity
                    UASSERT_OBJ(pol, nodep, "AND/OR tree expects m_polarity==true");
                    m_frozenNodes.emplace_వెనుక(opp, Frozen_కణుపు_Info{pol, m_lsb});
                    m_failed = origFailed;
                    కొనసాగు;
                }
                restorer.disableRestore();  // Now all checks passed
                ఒకవేళ (ఆకు_Info_2.refp()) {
                    // The conditional on the lsb being in range is necessary for some degenerate
                    // case, e.g.: (IData)((QData)wide[0] >> 32), or <1-bit-var> >> 1, which is
                    // just zero
                    ఒకవేళ (ఆకు_Info_2.lsb() <= ఆకు_Info_2.msb()) {
                        m_బిట్టు_Polarities.emplace_వెనుక(ఆకు_Info_2, is_Xor_వృక్షం() || ఆకు_Info_2.polarity(),
                                                     ఆకు_Info_2.lsb());
                    } లేకపోతే ఒకవేళ ((is_And_వృక్షం() && ఆకు_Info_2.polarity())
                               || (is_Or_వృక్షం() && !ఆకు_Info_2.polarity())) {
                        // If there is a constant 0 term in an And tree or 1 term in an Or tree, we
                        // must include it. Fudge this by adding a bit with both polarities, which
                        // will simplify to zero or one respectively.
                        // Note that Xor tree does not need this kind of care, polarity of Xor tree
                        // is already cared when visitin AstNot. Taking xor with 1'b0 is nop.
                        m_బిట్టు_Polarities.emplace_వెనుక(ఆకు_Info_2, నిజం, 0);
                        m_బిట్టు_Polarities.emplace_వెనుక(ఆకు_Info_2, అబద్ధం, 0);
                    }
                }
            }
        } లేకపోతే ఒకవేళ ((is_And_వృక్షం() && VN_IS(nodep, Eq)) || (is_Or_వృక్షం() && VN_IS(nodep, Neq))) {
            Restorer restorer{*ఇది};
            CONST_BITOP_RETURN_IF(!m_polarity, nodep);
            CONST_BITOP_RETURN_IF(m_lsb, nodep);  // the result of EQ/NE is 1 bit width
            స్థిరరాశి Ast_కణుపు* lhsp = nodep->lhsp();
            ఒకవేళ (స్థిరరాశి AstCCast* స్థిరరాశి castp = VN_CAST(lhsp, CCast)) lhsp = castp->lhsp();
            స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(lhsp, Const);
            CONST_BITOP_RETURN_IF(!constp, nodep->lhsp());

            స్థిరరాశి V3_సంఖ్య& comp_అంకె = constp->అంకె();

            స్వయంచాలక సమితి_Polarities = [ఇది, &comp_అంకె](స్థిరరాశి ఆకు_Info& ref, స్థిరరాశి V3_సంఖ్య* maskp) {
                స్థిరరాశి తార్కిక ముసుగు_Flip = is_And_వృక్షం() ^ ref.polarity();
                పూర్ణాంకం constant_వెడల్పు = comp_అంకె.వెడల్పు();
                ఒకవేళ (maskp) constant_వెడల్పు = std::max(constant_వెడల్పు, maskp->వెడల్పు());
                స్థిరరాశి పూర్ణాంకం max_బిట్టు_Idx = std::max(ref.lsb() + constant_వెడల్పు, ref.msb() + 1);
                // Mark all bits checked by this comparison
                కొరకు (పూర్ణాంకం బిట్టు_Idx = ref.lsb(); బిట్టు_Idx < max_బిట్టు_Idx; ++బిట్టు_Idx) {
                    స్థిరరాశి పూర్ణాంకం ముసుగు_Idx = బిట్టు_Idx - ref.lsb();
                    స్థిరరాశి తార్కిక mask0 = maskp && maskp->బిట్టు_Is0(ముసుగు_Idx);
                    స్థిరరాశి తార్కిక బహిర్గతం_యొక్క_Range = బిట్టు_Idx > ref.msb();
                    ఒకవేళ (mask0 || బహిర్గతం_యొక్క_Range) {  // RHS is 0
                        ఒకవేళ (comp_అంకె.బిట్టు_Is1(ముసుగు_Idx)) {
                            // LHS is 1
                            // And tree: 1 == 0 => always false, set v && !v
                            // Or tree : 1 != 0 => always true, set v || !v
                            m_బిట్టు_Polarities.emplace_వెనుక(ref, నిజం, 0);
                            m_బిట్టు_Polarities.emplace_వెనుక(ref, అబద్ధం, 0);
                            ఆపు;
                        } లేకపోతే {  // This bitIdx is irrelevant
                            కొనసాగు;
                        }
                    }
                    స్థిరరాశి తార్కిక polarity = comp_అంకె.బిట్టు_Is1(ముసుగు_Idx) != ముసుగు_Flip;
                    m_బిట్టు_Polarities.emplace_వెనుక(ref, polarity, బిట్టు_Idx);
                }
            };

            ఒకవేళ (స్థిరరాశి AstAnd* స్థిరరాశి andp = VN_CAST(nodep->rhsp(), And)) {  // comp == (mask & v)
                స్థిరరాశి ఆకు_Info& ముసుగు = కనుగొను_ఆకు(andp->lhsp(), నిజం);
                CONST_BITOP_RETURN_IF(!ముసుగు.constp() || ముసుగు.lsb() != 0, andp->lhsp());

                స్థిరరాశి ఆకు_Info& ref = కనుగొను_ఆకు(andp->rhsp(), అబద్ధం);
                CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());

                restorer.disableRestore();  // Now all checks passed

                స్థిరరాశి V3_సంఖ్య& ముసుగు_అంకె = ముసుగు.constp()->అంకె();

                incrOps(nodep, __LINE__);
                incrOps(andp, __LINE__);

                సమితి_Polarities(ref, &ముసుగు_అంకె);
            } లేకపోతే {  // comp == v
                స్థిరరాశి ఆకు_Info& ref = కనుగొను_ఆకు(nodep->rhsp(), అబద్ధం);
                CONST_BITOP_RETURN_IF(!ref.refp(), nodep->rhsp());

                restorer.disableRestore();  // Now all checks passed

                incrOps(nodep, __LINE__);

                సమితి_Polarities(ref, శూన్యసూచిక);
            }
        } లేకపోతే {
            CONST_BITOP_సమితి_FAILED("Mixture of different ops cannot be optimized", nodep);
        }
    }

    // CONSTRUCTORS
    Const_బిట్టు_Op_వృక్షం_Visitor(Ast_కణుపు_సమాసం* nodep, గుర్తులేని externalOps, VNIdAllocator& ids)
        : m_ops{externalOps}
        , m_rootp{nodep}
        , m_ids{ids} {
        // Fill nullptr at [0] because a base index of 0 means not set yet
        m_varInfos.నెట్టు_వెనుక(శూన్యసూచిక);
        CONST_BITOP_RETURN_IF(!is_And_వృక్షం() && !is_Or_వృక్షం() && !is_Xor_వృక్షం(), nodep);
        ఒకవేళ (Ast_కణుపు_Biop* స్థిరరాశి biopp = VN_CAST(nodep, కణుపు_Biop)) {
            iterateConst(biopp);
        } లేకపోతే {
            UASSERT_OBJ(VN_IS(nodep, RedXor), nodep, "Must be RedXor");
            incrOps(nodep, __LINE__);
            iterateChildrenConst(nodep);
        }
        కొరకు (స్వయంచాలక&& నమోదు_2 : m_బిట్టు_Polarities) {
            getVarInfo(నమోదు_2.m_info).సమితి_Polarity(నమోదు_2.m_polarity, నమోదు_2.m_బిట్టు);
        }
        UASSERT_OBJ(is_Xor_వృక్షం() || m_polarity, nodep, "must be the original polarity");
    }
    మాయా ~Const_బిట్టు_Op_వృక్షం_Visitor() = అప్రమేయ;
#నిర్వచనం_రద్దు CONST_BITOP_RETURN_IF
#నిర్వచనం_రద్దు CONST_BITOP_సమితి_FAILED

బహిరంగ:
    // Transform as below.
    // v[0] & v[1] => 2'b11 == (2'b11 & v)
    // v[0] | v[1] => 2'b00 != (2'b11 & v)
    // v[0] ^ v[1] => ^{2'b11 & v}
    // (3'b011 == (3'b011 & v)) & v[2]  => 3'b111 == (3'b111 & v)
    // (3'b000 != (3'b011 & v)) | v[2]  => 3'b000 != (3'b111 & v)
    // Reduction ops are transformed in the same way.
    // &{v[0], v[1]} => 2'b11 == (2'b11 & v)
    స్థిర Ast_కణుపు_సమాసం* simplify(Ast_కణుపు_సమాసం* nodep, పూర్ణాంకం ఫలితం_వెడల్పు, గుర్తులేని externalOps,
                                 VDouble0& reduction, VNIdAllocator& ids) {
        UASSERT_OBJ(1 <= ఫలితం_వెడల్పు && ఫలితం_వెడల్పు <= 64, nodep, "resultWidth out of range");

        // Walk tree, gathering all terms referenced in expression
        స్థిరరాశి Const_బిట్టు_Op_వృక్షం_Visitor visitor{nodep, externalOps, ids};

        // If failed on root node is not optimizable, or there are no variable terms, then done
        ఒకవేళ (visitor.m_failed || visitor.m_varInfos.కొలత() == 1) తిరిగి శూన్యసూచిక;

        // FileLine used for constructing all new nodes in this function
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();

        // Get partial result each term referenced, count total number of ops and keep track of
        // whether we have clean/dirty terms. visitor.m_varInfos appears in deterministic order,
        // so the optimized tree is deterministic as well.

        std::సదిశ<Ast_కణుపు_సమాసం*> termps;
        termps.reserve(visitor.m_varInfos.కొలత() - 1);
        గుర్తులేని ఫలితం_Ops = 0;
        తార్కిక has_Clean_పదం = అబద్ధం;
        తార్కిక has_Dirty_పదం = అబద్ధం;

        కొరకు (స్వయంచాలక&& v : visitor.m_varInfos) {
            ఒకవేళ (!v) కొనసాగు;  // Skip nullptr at m_varInfos[0]
            ఒకవేళ (v->has_Const_ఫలితం()) {
                // If a constant term is known, we can either drop it or the whole tree is constant
                Ast_కణుపు_సమాసం* resultp = శూన్యసూచిక;
                ఒకవేళ (v->get_Const_ఫలితం()) {
                    UASSERT_OBJ(visitor.is_Or_వృక్షం(), nodep,
                                "Only OR tree can yield known 1 result");
                    UINFO(9, "OR tree with const 1 term: " << v->refp());
                    // Known 1 bit in OR tree, whole result is 1
                    resultp = కొత్త AstConst{fl, AstConst::బిట్టు_True{}};
                } లేకపోతే ఒకవేళ (visitor.is_And_వృక్షం()) {
                    UINFO(9, "AND tree with const 0 term: " << v->refp());
                    // Known 0 bit in AND tree, whole result is 0
                    resultp = కొత్త AstConst{fl, AstConst::బిట్టు_False{}};
                } లేకపోతే {
                    // Known 0 bit in OR or XOR tree. Ignore it.
                    కొనసాగు;
                }
                // Set width and widthMin precisely
                resultp->dtype_Chg_వెడల్పు(ఫలితం_వెడల్పు, 1);
                కొరకు (Ast_కణుపు* స్థిరరాశి termp : termps) VL_DO_DANGLING(termp->delete_వృక్షం(), termp);
                తిరిగి resultp;
            }
            స్థిరరాశి ఫలితం_పదం ఫలితం = v->get_ఫలితం_పదం();
            termps.నెట్టు_వెనుక(std::get<0>(ఫలితం));
            ఫలితం_Ops += std::get<1>(ఫలితం);
            ఒకవేళ (std::get<2>(ఫలితం)) {
                has_Clean_పదం = నిజం;
                UINFO(9, "Clean term: " << termps.వెనుక());
            } లేకపోతే {
                has_Dirty_పదం = నిజం;
                UINFO(9, "Dirty term: " << termps.వెనుక());
            }
        }

        // Group by FrozenNodeInfo
        std::map<Frozen_కణుపు_Info, std::సదిశ<Ast_కణుపు_సమాసం*>> frozenNodes;
        // Check if frozen terms are clean or not
        కొరకు (స్థిరరాశి స్వయంచాలక& frozenInfo : visitor.m_frozenNodes) {
            Ast_కణుపు_సమాసం* స్థిరరాశి termp = frozenInfo.మొదటి;
            // Comparison operators are clean
            ఒకవేళ ((VN_IS(termp, Eq) || VN_IS(termp, Neq) || VN_IS(termp, Lt) || VN_IS(termp, Lte)
                 || VN_IS(termp, Gt) || VN_IS(termp, Gte))
                && frozenInfo.రెండవ.m_lsb == 0) {
                has_Clean_పదం = నిజం;
            } లేకపోతే {
                // Otherwise, conservatively assume the frozen term is dirty
                has_Dirty_పదం = నిజం;
                UINFO(9, "Dirty frozen term: " << termp);
            }
            frozenNodes[frozenInfo.రెండవ].నెట్టు_వెనుక(termp);
        }

        // Figure out if a final negation is required
        స్థిరరాశి తార్కిక needsFlip = visitor.is_Xor_వృక్షం() && !visitor.m_polarity;

        // Figure out if the final tree needs cleaning
        స్థిరరాశి తార్కిక needsCleaning = visitor.is_And_వృక్షం() ? !has_Clean_పదం : has_Dirty_పదం;

        // Add size of reduction tree to op count
        ఫలితం_Ops += termps.కొలత() - 1;
        కొరకు (స్థిరరాశి స్వయంచాలక& lsbAndNodes : frozenNodes) {
            ఒకవేళ (lsbAndNodes.మొదటి.m_lsb > 0) ++ఫలితం_Ops;  // Needs AstShiftR
            ఒకవేళ (!lsbAndNodes.మొదటి.m_polarity) ++ఫలితం_Ops;  // Needs AstNot
            ఫలితం_Ops += lsbAndNodes.రెండవ.కొలత();
        }
        // Add final polarity flip in Xor tree
        ఒకవేళ (needsFlip) ++ఫలితం_Ops;
        // Add final cleaning AND
        ఒకవేళ (needsCleaning) ++ఫలితం_Ops;

        ఒకవేళ (debug() >= 9) {  // LCOV_EXCL_START
            బయటకు << "-  Bitop tree considered:\n";
            కొరకు (స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి termp : termps) termp->dump_వృక్షం("-  Reduced term: ");
            కొరకు (స్థిరరాశి std::pair<Ast_కణుపు_సమాసం*, Frozen_కణుపు_Info>& termp : visitor.m_frozenNodes) {
                termp.మొదటి->dump_వృక్షం("-  Frozen term with lsb "
                                      + std::గమ్యం_string(termp.రెండవ.m_lsb) + " polarity "
                                      + std::గమ్యం_string(termp.రెండవ.m_polarity) + ": ");
            }
            బయటకు << "-  Needs flipping: " << needsFlip << "\n";
            బయటకు << "-  Needs cleaning: " << needsCleaning << "\n";
            బయటకు << "-  Size: " << ఫలితం_Ops << " input size: " << visitor.m_ops << "\n";
        }  // LCOV_EXCL_STOP

        // Sometimes we have no terms left after ignoring redundant terms
        // (all of which were zeroes)
        ఒకవేళ (termps.ఖాళీలు() && visitor.m_frozenNodes.ఖాళీలు()) {
            reduction += visitor.m_ops;
            Ast_కణుపు_సమాసం* స్థిరరాశి resultp = needsFlip ? కొత్త AstConst{fl, AstConst::బిట్టు_True{}}
                                                   : కొత్త AstConst{fl, AstConst::బిట్టు_False{}};
            resultp->dtype_Chg_వెడల్పు(ఫలితం_వెడల్పు, 1);
            తిరిగి resultp;
        }

        // Only substitute the result if beneficial as determined by operation count
        ఒకవేళ (visitor.m_ops <= ఫలితం_Ops) {
            కొరకు (Ast_కణుపు* స్థిరరాశి termp : termps) VL_DO_DANGLING(termp->delete_వృక్షం(), termp);
            తిరిగి శూన్యసూచిక;
        }

        // Update statistics
        reduction += visitor.m_ops - ఫలితం_Ops;

        // Reduction op to combine terms
        స్థిరరాశి స్వయంచాలక reduce = [&visitor, fl](Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp) -> Ast_కణుపు_సమాసం* {
            ఒకవేళ (!lhsp) తిరిగి rhsp;
            ఒకవేళ (visitor.is_And_వృక్షం()) {
                తిరిగి కొత్త AstAnd{fl, lhsp, rhsp};
            } లేకపోతే ఒకవేళ (visitor.is_Or_వృక్షం()) {
                తిరిగి కొత్త AstOr{fl, lhsp, rhsp};
            } లేకపోతే {
                తిరిగి కొత్త AstXor{fl, lhsp, rhsp};
            }
        };

        // Compute result by reducing all terms
        Ast_కణుపు_సమాసం* resultp = శూన్యసూచిక;
        కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి termp : termps) {  //
            resultp = reduce(resultp, termp);
        }
        // Add any frozen terms to the reduction
        కొరకు (స్వయంచాలక&& nodes : frozenNodes) {
            // nodes.second has same lsb and polarity
            Ast_కణుపు_సమాసం* termp = శూన్యసూచిక;
            కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి itemp : nodes.రెండవ) {
                termp = reduce(termp, itemp->unlink_Fr_వెనుక());
            }
            ఒకవేళ (nodes.మొదటి.m_lsb > 0) {  // LSB is not 0, so shiftR
                Ast_కణుపు_DType* స్థిరరాశి dtypep = termp->dtypep();
                termp = కొత్త Ast_జరుపు_R{termp->fileline(), termp,
                                      కొత్త AstConst(termp->fileline(), AstConst::Widthed_విలువ{},
                                                   termp->వెడల్పు(), nodes.మొదటి.m_lsb)};
                termp->dtypep(dtypep);
            }
            ఒకవేళ (!nodes.మొదటి.m_polarity) {  // Polarity is inverted, so append Not
                Ast_కణుపు_DType* స్థిరరాశి dtypep = termp->dtypep();
                termp = కొత్త AstNot{termp->fileline(), termp};
                termp->dtypep(dtypep);
            }
            resultp = reduce(resultp, termp);
        }

        // Set width of masks to expected result width. This is required to prevent later removal
        // of the masking node e.g. by the "AND with all ones" rule. If the result width happens
        // to be 1, we still need to ensure the AstAnd is not dropped, so use a wider mask in this
        // special case.
        స్థిరరాశి పూర్ణాంకం ముసుగు_వెడల్పు
            = std::max(resultp->వెడల్పు(), ఫలితం_వెడల్పు == 1 ? VL_IDATASIZE : ఫలితం_వెడల్పు);

        // Apply final polarity flip
        ఒకవేళ (needsFlip) {
            ఒకవేళ (needsCleaning) {
                // Cleaning will be added below. Use a NOT which is a byte shorter on x86
                resultp = కొత్త AstNot{fl, resultp};
            } లేకపోతే {
                // Keep result clean by using XOR(1, _)
                AstConst* స్థిరరాశి maskp = కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, ముసుగు_వెడల్పు, 1};
                resultp = కొత్త AstXor{fl, maskp, resultp};
            }
        }

        // Apply final cleaning
        ఒకవేళ (needsCleaning) {
            AstConst* స్థిరరాశి maskp = కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, ముసుగు_వెడల్పు, 1};
            resultp = కొత్త AstAnd{fl, maskp, resultp};
        }

        // Cast back to original size if required
        ఒకవేళ (resultp->వెడల్పు() != ఫలితం_వెడల్పు) {
            resultp = కొత్త AstCCast{fl, resultp, ఫలితం_వెడల్పు, 1};
        }

        // Set width and widthMin precisely
        resultp->dtype_Chg_వెడల్పు(ఫలితం_వెడల్పు, 1);

        తిరిగి resultp;
    }
};

//######################################################################
// Const state, as a visitor of each AstNode

తరగతి ConstVisitor final : బహిరంగ VNVisitor {
    // CONSTANTS
    స్థిర constexpr గుర్తులేని CONCAT_MERGABLE_MAX_లోతు = 10;  // Limit alg recursion
    // Largest right-hand side that is copied to each part of a split concatenation assignment
    స్థిర constexpr పూర్ణాంకం CONCAT_కేటాయించు_MAX_RHS_NODES = 16;

    // NODE STATE
    // See VNIdAllocator. All per node state below is held in side tables keyed on the
    // node ids it hands out, as node pointers are not stable keys within V3Const.

    // STATE
    VNIdAllocator m_ids;  // Node id allocator
    తార్కిక m_params = అబద్ధం;  // If true, propagate parameterized and true numbers only
    తార్కిక m_required = అబద్ధం;  // If true, must become a constant
    తార్కిక m_wremove = నిజం;  // Inside scope, no assignw removal
    తార్కిక m_warn = అబద్ధం;  // Output warnings
    తార్కిక m_doExpensive = అబద్ధం;  // Enable computationally expensive optimizations
    తార్కిక m_doCpp = అబద్ధం;  // Enable late-stage C++ optimizations
    తార్కిక m_doNConst = అబద్ధం;  // Enable non-constant-child simplifications
    తార్కిక m_doV = అబద్ధం;  // Verilog, not C++ conversion
    తార్కిక m_doGenerate = అబద్ధం;  // Postpone width checking inside generate
    తార్కిక m_convert_Logic_గమ్యం_బిట్టు = అబద్ధం;  // Convert logical operators to bitwise
    తార్కిక m_hasJumpDelay = అబద్ధం;  // JumpGo or Delay under this loop
    తార్కిక m_hasLoopTest = అబద్ధం;  // Contains AstLoopTest
    తార్కిక m_underRecFunc = అబద్ధం;  // Under a recursive function
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    స్థిరరాశి AstArraySel* m_selp = శూన్యసూచిక;  // Current select
    స్థిరరాశి AstScope* m_scopep = శూన్యసూచిక;  // Current scope
    స్థిరరాశి Ast_Attr_యొక్క* m_attrp = శూన్యసూచిక;  // Current attribute
    VDouble0 m_stat_బిట్టు_Op_Reduction;  // Ops reduced in ConstBitOpTreeVisitor
    VDouble0 m_stat_Concat_విలీనం;  // Concat merges
    VDouble0 m_stat_Cond_సమాసం_Redundant;  // Conditional repeated expressions
    VDouble0 m_stat_If_Cond_సమాసం_Redundant;  // Conditional repeated expressions
    VDouble0 m_statMemberAccessVisits;  // Nodes visited by containsMemberAccessRecurse
    స్థిరరాశి తార్కిక m_globalPass;  // ConstVisitor invoked as a global pass
    స్థిర uint32_t s_global_Pass_అంకె;  // Counts number of times ConstVisitor invoked as global pass
    V3_Unique_పేర్లు m_concswap_పేర్లు;  // For generating unique temporary variable names
    // Caches results of matchBiopToBitwise, keyed on node id
    std::unordered_map<కొలత_t, తార్కిక> m_కలిగియుంది_Member_Access;
    // Items of enum currently being recursed into. Cannot use VNIdAllocator, as this runs in
    // every mode, including those invoked under a pass holding user4 (V3Param). Node pointers
    // are safe as keys here, unlike elsewhere in V3Const: folding the item value below does
    // free nodes whose addresses are then reused, but AstEnumItem is only ever constructed
    // while parsing (verilog.y and V3LinkParse), so a reused address can never become one,
    // and every key looked up is an AstEnumItem.
    std::unordered_సమితి<స్థిరరాశి Ast_Enum_అంశం*> m_recursing_Enum_వస్తువులు;
    std::unordered_సమితి<Ast_Jump_ఖండం*> m_వాడిన_Jump_ఖండాలు;  // JumpBlocks used by some JumpGo

    // METHODS

    // Return true if statements may be deleted. Until statics are lifted, a variable declared
    // in a statement might be referenced from elsewhere (e.g. hierarchically).
    స్థిర తార్కిక canDeleteStatements() { తిరిగి v3Global.staticsLifted(); }

    శూన్య deleteVarScopesUnder(Ast_కణుపు* subtreep) {
        ఒకవేళ (!subtreep) తిరిగి;
        ఒకవేళ (!m_scopep) తిరిగి;
        std::unordered_సమితి<AstVar*> varps;
        subtreep->foreach_And_తదుపరి([&](AstVar* varp) { varps.చొప్పించు(varp); });
        ఒకవేళ (varps.ఖాళీలు()) తిరిగి;
        కొరకు (AstVarScope *vscp = m_scopep->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            ఒకవేళ (varps.కనుగొను(vscp->varp()) != varps.చివర()) {
                VL_DO_DANGLING(నెట్టు_Deletep(vscp->unlink_Fr_వెనుక()), vscp);
            }
        }
    }

    V3_సంఖ్య const_అంకె_V(Ast_కణుపు* nodep) {
        // Contract C width to V width (if needed, else just direct copy)
        // The upper zeros in the C representation can otherwise cause
        // wrong results in some operations, e.g. MulS
        స్థిరరాశి V3_సంఖ్య& numc = VN_AS(nodep, Const)->అంకె();
        తిరిగి !numc.is_సంఖ్య() ? numc : V3_సంఖ్య{nodep, nodep->వెడల్పు_Min_V(), numc};
    }
    V3_సంఖ్య గమ్యం_అంకె_C(Ast_కణుపు* nodep, స్థిరరాశి V3_సంఖ్య& numv) {
        // Extend V width back to C width for given node
        తిరిగి !numv.is_సంఖ్య() ? numv : V3_సంఖ్య{nodep, nodep->వెడల్పు(), numv};
    }

    స్థిర తార్కిక క్రింది_As_Fixed_Aggregate(స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep) {
        తిరిగి dtypep->isStreamableFixedAggregate() && dtypep->కలిగియుంది_Unpacked_Struct();
    }

    AstStructSel* newStructSel(Ast_కణుపు_సమాసం* స్థిరరాశి fromp, స్థిరరాశి AstMemberDType* స్థిరరాశి itemp) {
        AstStructSel* స్థిరరాశి selp = కొత్త AstStructSel{fromp->fileline(), fromp, itemp->name()};
        selp->dtypeFrom(itemp->dtypep());
        తిరిగి selp;
    }

    శూన్య collectFixedAggregateTerms(Ast_కణుపు_సమాసం* స్థిరరాశి fromp, std::సదిశ<Ast_కణుపు_సమాసం*>& termps,
                                    స్థిరరాశి తార్కిక packReal) {
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = fromp->dtypep()->skipRefp();
        ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి unpackDtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            కొరకు (స్థిరరాశి పూర్ణాంకం idx : unpackDtypep->declRange().seq_ఎడమ_గమ్యం_కుడి()) {
                AstArraySel* స్థిరరాశి selp
                    = కొత్త AstArraySel{fromp->fileline(), fromp->clone_వృక్షం_శుద్ధ(అబద్ధం), idx};
                collectFixedAggregateTerms(selp, termps, packReal);
            }
            VL_DO_DANGLING(నెట్టు_Deletep(fromp), fromp);
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి sdtypep
                   = VN_CAST(dtypep, కణుపు_UOr_Struct_DType)) {
            ఒకవేళ (sdtypep->packed()) {
                termps.నెట్టు_వెనుక(fromp);
                తిరిగి;
            }
            కొరకు (స్థిరరాశి AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                collectFixedAggregateTerms(newStructSel(fromp->clone_వృక్షం_శుద్ధ(అబద్ధం), itemp),
                                           termps, packReal);
            }
            VL_DO_DANGLING(నెట్టు_Deletep(fromp), fromp);
        } లేకపోతే ఒకవేళ (packReal && dtypep->isDouble()) {
            termps.నెట్టు_వెనుక(కొత్త Ast_Real_గమ్యం_బిట్లు{fromp->fileline(), fromp});
        } లేకపోతే {
            termps.నెట్టు_వెనుక(fromp);
        }
    }

    Ast_కణుపు_సమాసం* packFixedAggregate(Ast_కణుపు_సమాసం* స్థిరరాశి fromp) {
        std::సదిశ<Ast_కణుపు_సమాసం*> termps;
        collectFixedAggregateTerms(fromp, termps, నిజం);
        UASSERT(!termps.ఖాళీలు(), "No stream terms");
        Ast_కణుపు_సమాసం* resultp = termps[0];
        కొరకు (కొలత_t i = 1; i < termps.కొలత(); ++i) {
            resultp = కొత్త AstConcat{resultp->fileline(), resultp, termps[i]};
        }
        తిరిగి resultp;
    }

    శూన్య replace_కేటాయించు_గమ్యం_Fixed_Aggregate(Ast_కణుపు_కేటాయించు* స్థిరరాశి nodep, Ast_కణుపు_సమాసం* స్థిరరాశి dstp,
                                       Ast_కణుపు_సమాసం* srcp) {
        స్థిరరాశి పూర్ణాంకం dst_వెడల్పు = dstp->dtypep()->వెడల్పు_ప్రవాహం();
        std::సదిశ<Ast_కణుపు_సమాసం*> termps;
        collectFixedAggregateTerms(dstp, termps, అబద్ధం);
        పూర్ణాంకం src_వెడల్పు = srcp->వెడల్పు();
        ఒకవేళ (src_వెడల్పు < dst_వెడల్పు) {
            AstExtend* స్థిరరాశి extendp = కొత్త AstExtend{srcp->fileline(), srcp, dst_వెడల్పు};
            srcp = కొత్త Ast_జరుపు_L{
                extendp->fileline(), extendp,
                కొత్త AstConst{extendp->fileline(), static_cast<uint32_t>(dst_వెడల్పు - src_వెడల్పు)},
                dst_వెడల్పు};
            src_వెడల్పు = dst_వెడల్పు;
        }
        Ast_కణుపు_కేటాయించు* newp = శూన్యసూచిక;
        పూర్ణాంకం స్థానభ్రంశం = 0;
        కొరకు (కొలత_t i = 0; i < termps.కొలత(); ++i) {
            Ast_కణుపు_సమాసం* స్థిరరాశి termp = termps[i];
            స్థిరరాశి పూర్ణాంకం వెడల్పు = termp->dtypep()->వెడల్పు_ప్రవాహం();
            స్థిరరాశి పూర్ణాంకం lsb = src_వెడల్పు - స్థానభ్రంశం - వెడల్పు;
            స్థానభ్రంశం += వెడల్పు;
            Ast_కణుపు_సమాసం* rhsp
                = కొత్త AstSel{srcp->fileline(), srcp->clone_వృక్షం_శుద్ధ(అబద్ధం), lsb, వెడల్పు};
            ఒకవేళ (termp->dtypep()->skipRefp()->isDouble()) {
                rhsp = కొత్త Ast_బిట్లు_గమ్యం_Real_D{rhsp->fileline(), rhsp};
            }
            Ast_కణుపు_కేటాయించు* స్థిరరాశి assignp = nodep->cloneType(termp, rhsp);
            assignp->dtypeFrom(termp);
            newp = Ast_కణుపు::కూడు_తదుపరి(newp, assignp);
        }
        nodep->కూడు_తదుపరి_ఇక్కడ(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(srcp), srcp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }

    తార్కిక నిర్వాహ్యం_Const(స్థిరరాశి Ast_కణుపు* nodep) { తిరిగి VN_IS(nodep, Const); }
    తార్కిక నిర్వాహ్యం_Asv_Const(స్థిరరాశి Ast_కణుపు* nodep) {
        // BIASV(CONST, BIASV(CONST,...)) -> BIASV( BIASV_CONSTED(a,b), ...)
        స్థిరరాశి Ast_కణుపు_Bi_Com_Asv* స్థిరరాశి bnodep = VN_CAST(nodep, కణుపు_Bi_Com_Asv);
        ఒకవేళ (!bnodep) తిరిగి అబద్ధం;
        ఒకవేళ (!VN_IS(bnodep->lhsp(), Const)) తిరిగి అబద్ధం;
        స్థిరరాశి Ast_కణుపు_Bi_Com_Asv* స్థిరరాశి rnodep = VN_CAST(bnodep->rhsp(), కణుపు_Bi_Com_Asv);
        ఒకవేళ (!rnodep) తిరిగి అబద్ధం;
        ఒకవేళ (rnodep->type() != bnodep->type()) తిరిగి అబద్ధం;
        ఒకవేళ (rnodep->వెడల్పు() != bnodep->వెడల్పు()) తిరిగి అబద్ధం;
        ఒకవేళ (rnodep->lhsp()->వెడల్పు() != bnodep->lhsp()->వెడల్పు()) తిరిగి అబద్ధం;
        ఒకవేళ (!VN_IS(rnodep->lhsp(), Const)) తిరిగి అబద్ధం;
        తిరిగి నిజం;
    }
    తార్కిక నిర్వాహ్యం_Asv_Same(స్థిరరాశి Ast_కణుపు* nodep) {
        // BIASV(SAMEa, BIASV(SAMEb,...)) -> BIASV( BIASV(SAMEa,SAMEb), ...)
        స్థిరరాశి Ast_కణుపు_Bi_Com_Asv* స్థిరరాశి bnodep = VN_CAST(nodep, కణుపు_Bi_Com_Asv);
        ఒకవేళ (!bnodep) తిరిగి అబద్ధం;
        స్థిరరాశి Ast_కణుపు_Bi_Com_Asv* స్థిరరాశి rnodep = VN_CAST(bnodep->rhsp(), కణుపు_Bi_Com_Asv);
        ఒకవేళ (!rnodep) తిరిగి అబద్ధం;
        ఒకవేళ (rnodep->type() != bnodep->type()) తిరిగి అబద్ధం;
        ఒకవేళ (rnodep->వెడల్పు() != bnodep->వెడల్పు()) తిరిగి అబద్ధం;
        తిరిగి operandsSame(bnodep->lhsp(), rnodep->lhsp());
    }
    తార్కిక నిర్వాహ్యం_Asv_LUp(స్థిరరాశి Ast_కణుపు* nodep) {
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
        స్థిరరాశి Ast_కణుపు_Bi_Com_Asv* స్థిరరాశి bnodep = VN_CAST(nodep, కణుపు_Bi_Com_Asv);
        ఒకవేళ (!bnodep) తిరిగి అబద్ధం;
        స్థిరరాశి Ast_కణుపు_Bi_Com_Asv* స్థిరరాశి lnodep = VN_CAST(bnodep->lhsp(), కణుపు_Bi_Com_Asv);
        ఒకవేళ (!lnodep) తిరిగి అబద్ధం;
        ఒకవేళ (lnodep->type() != bnodep->type()) తిరిగి అబద్ధం;
        ఒకవేళ (lnodep->వెడల్పు() != bnodep->వెడల్పు()) తిరిగి అబద్ధం;
        తిరిగి VN_IS(lnodep->lhsp(), Const);
    }
    తార్కిక నిర్వాహ్యం_Asv_RUp(స్థిరరాశి Ast_కణుపు* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr)) ?
        స్థిరరాశి Ast_కణుపు_Bi_Com_Asv* స్థిరరాశి bnodep = VN_CAST(nodep, కణుపు_Bi_Com_Asv);
        ఒకవేళ (!bnodep) తిరిగి అబద్ధం;
        స్థిరరాశి Ast_కణుపు_Bi_Com_Asv* స్థిరరాశి rnodep = VN_CAST(bnodep->rhsp(), కణుపు_Bi_Com_Asv);
        ఒకవేళ (!rnodep) తిరిగి అబద్ధం;
        ఒకవేళ (rnodep->type() != bnodep->type()) తిరిగి అబద్ధం;
        ఒకవేళ (rnodep->వెడల్పు() != bnodep->వెడల్పు()) తిరిగి అబద్ధం;
        తిరిగి VN_IS(rnodep->lhsp(), Const);
    }
    స్థిర తార్కిక నిర్వాహ్యం_Sub_కూడు(స్థిరరాశి Ast_కణుపు* nodep) {
        // SUB( ADD(CONSTx,y), CONSTz) -> ADD(SUB(CONSTx,CONSTz), y)
        స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి np = VN_CAST(nodep, కణుపు_Biop);
        స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి lp = VN_CAST(np->lhsp(), కణుపు_Biop);
        తిరిగి (lp && VN_IS(lp->lhsp(), Const) && VN_IS(np->rhsp(), Const)
                && lp->వెడల్పు() == np->వెడల్పు());
    }
    తార్కిక సరిపోలిక_Redundant_Clean(AstAnd* andp) {
        // Remove And with constant one inserted by V3Clean
        // 1 & (a == b)  -> (IData)(a == b)
        // When bool is casted to int, the value is either 0 or 1
        AstConst* స్థిరరాశి constp = VN_AS(andp->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isOne(), andp->lhsp(), "TRREEOPC must meet this condition");
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = andp->rhsp();
        AstCCast* ccastp = శూన్యసూచిక;
        స్థిరరాశి స్వయంచాలక isEqOrNeq = [](స్థిరరాశి Ast_కణుపు* nodep) -> తార్కిక {  //
            తిరిగి VN_IS(nodep, Eq) || VN_IS(nodep, Neq);
        };
        ఒకవేళ (isEqOrNeq(rhsp)) {
            ccastp = కొత్త AstCCast{andp->fileline(), rhsp->unlink_Fr_వెనుక(), andp};
        } లేకపోతే ఒకవేళ (AstCCast* స్థిరరాశి tmpp = VN_CAST(rhsp, CCast)) {
            ఒకవేళ (isEqOrNeq(tmpp->lhsp())) {
                ఒకవేళ (tmpp->వెడల్పు() == andp->వెడల్పు()) {
                    tmpp->unlink_Fr_వెనుక();
                    ccastp = tmpp;
                } లేకపోతే {
                    ccastp = కొత్త AstCCast{andp->fileline(), tmpp->lhsp()->unlink_Fr_వెనుక(), andp};
                }
            }
        }
        ఒకవేళ (ccastp) {
            andp->replaceWithKeepDType(ccastp);
            VL_DO_DANGLING(నెట్టు_Deletep(andp), andp);
            తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    స్థిర తార్కిక నిర్వాహ్యం_And_Or_Same(స్థిరరాశి Ast_కణుపు* nodep) {
        // OR( AND(VAL,x), AND(VAL,y)) -> AND(VAL,OR(x,y))
        // OR( AND(x,VAL), AND(y,VAL)) -> AND(OR(x,y),VAL)
        స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి np = VN_CAST(nodep, కణుపు_Biop);
        స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి lp = VN_CAST(np->lhsp(), కణుపు_Biop);
        స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి rp = VN_CAST(np->rhsp(), కణుపు_Biop);
        తిరిగి (lp && rp && lp->వెడల్పు() == rp->వెడల్పు() && lp->type() == rp->type()
                && (operandsSame(lp->lhsp(), rp->lhsp()) || operandsSame(lp->rhsp(), rp->rhsp())));
    }
    తార్కిక సరిపోలిక_Or_And_Not(Ast_కణుపు_Biop* nodep) {
        // AstOr{$a, AstAnd{AstNot{$b}, $c}} if $a.width1, $a==$b => AstOr{$a,$c}
        // Someday we'll sort the biops completely and this can be simplified
        // This often results from our simplified clock generation:
        // if (rst) ... else if (enable)... -> OR(rst,AND(!rst,enable))
        స్థిరరాశి Ast_కణుపు_సమాసం* ap;
        Ast_కణుపు_Biop* andp;
        ఒకవేళ (VN_IS(nodep->lhsp(), And)) {
            andp = VN_AS(nodep->lhsp(), And);
            ap = nodep->rhsp();
        } లేకపోతే ఒకవేళ (VN_IS(nodep->rhsp(), And)) {
            andp = VN_AS(nodep->rhsp(), And);
            ap = nodep->lhsp();
        } లేకపోతే {
            తిరిగి అబద్ధం;
        }
        స్థిరరాశి Ast_కణుపు_Uniop* notp;
        Ast_కణుపు_సమాసం* cp;
        ఒకవేళ (VN_IS(andp->lhsp(), Not)) {
            notp = VN_AS(andp->lhsp(), Not);
            cp = andp->rhsp();
        } లేకపోతే ఒకవేళ (VN_IS(andp->rhsp(), Not)) {
            notp = VN_AS(andp->rhsp(), Not);
            cp = andp->lhsp();
        } లేకపోతే {
            తిరిగి అబద్ధం;
        }
        స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి bp = notp->lhsp();
        ఒకవేళ (!operandsSame(ap, bp)) తిరిగి అబద్ధం;
        // Do it
        cp->unlink_Fr_వెనుక();
        VL_DO_DANGLING(నెట్టు_Deletep(andp->unlink_Fr_వెనుక()), andp);
        VL_DANGLING(notp);
        // Replace whichever branch is now dangling
        ఒకవేళ (nodep->rhsp()) {
            nodep->lhsp(cp);
        } లేకపోతే {
            nodep->rhsp(cp);
        }
        తిరిగి నిజం;
    }
    తార్కిక సరిపోలిక_And_Cond(AstAnd* nodep) {
        // Push down a AND into conditional, when one side of conditional is constant
        // (otherwise we'd be trading one operation for two operations)
        // V3Clean often makes this pattern, as it postpones the AND until
        // as high as possible, which is usually the right choice, except for this.
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstCond* స్థిరరాశి condp = VN_CAST(nodep->rhsp(), Cond);
        ఒకవేళ (!condp) తిరిగి అబద్ధం;
        ఒకవేళ (!VN_IS(condp->thenp(), Const) && !VN_IS(condp->elsep(), Const)) తిరిగి అబద్ధం;
        AstConst* స్థిరరాశి maskp = VN_CAST(nodep->lhsp(), Const);
        ఒకవేళ (!maskp) తిరిగి అబద్ధం;
        UINFO(4, "AND(CONSTm, CONDcond(c, i, e))->CONDcond(c, AND(m,i), AND(m, e)) " << nodep);
        AstCond* స్థిరరాశి newp = కొత్త AstCond{
            condp->fileline(), condp->condp()->unlink_Fr_వెనుక(),
            కొత్త AstAnd{nodep->fileline(), maskp->clone_వృక్షం(అబద్ధం), condp->thenp()->unlink_Fr_వెనుక()},
            కొత్త AstAnd{nodep->fileline(), maskp->clone_వృక్షం(అబద్ధం),
                       condp->elsep()->unlink_Fr_వెనుక()}};
        newp->thenp()->dtypeFrom(nodep);  // As And might have been to change widths
        newp->elsep()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        తిరిగి నిజం;
    }

    తార్కిక సరిపోలిక_Cond_Cond(స్థిరరాశి AstCond* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        ఒకవేళ (VN_IS(nodep->backp(), Cond)) తిరిగి అబద్ధం;  // Was checked when visited parent
        ఒకవేళ (!VN_IS(nodep->thenp(), Cond) && !VN_IS(nodep->elsep(), Cond))
            తిరిగి అబద్ధం;  // Short circuit
        std::సదిశ<Ast_కణుపు_సమాసం*> truesp;
        std::సదిశ<Ast_కణుపు_సమాసం*> falsesp;
        సరిపోలిక_Cond_Cond_Recurse(nodep, truesp /*ref*/, falsesp /*ref*/);
        తిరిగి అబద్ధం;  // Can optimize further
    }
    శూన్య సరిపోలిక_Cond_Cond_Recurse(స్థిరరాశి AstCond* nodep, std::సదిశ<Ast_కణుపు_సమాసం*>& truesp,
                              std::సదిశ<Ast_కణుపు_సమాసం*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        ఒకవేళ (truesp.కొలత() > 4 || falsesp.కొలత() > 4) తిరిగి;
        ఒకవేళ (!nodep->condp()->is_శుద్ధ()) తిరిగి;
        తార్కిక replaced = అబద్ధం;
        కొరకు (స్థిరరాశి Ast_కణుపు_సమాసం* condp : truesp) {
            ఒకవేళ (replaced) ఆపు;
            ఒకవేళ (!operandsSame(nodep->condp(), condp)) కొనసాగు;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_అంకె(nodep->condp(), 1);
            replaced = నిజం;
            ++m_stat_Cond_సమాసం_Redundant;
        }
        కొరకు (స్థిరరాశి Ast_కణుపు_సమాసం* condp : falsesp) {
            ఒకవేళ (replaced) ఆపు;
            ఒకవేళ (!operandsSame(nodep->condp(), condp)) కొనసాగు;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = నిజం;
            ++m_stat_Cond_సమాసం_Redundant;
        }
        ఒకవేళ (స్థిరరాశి AstCond* subCondp = VN_CAST(nodep->thenp(), Cond)) {
            ఒకవేళ (!replaced) truesp.emplace_వెనుక(nodep->condp());
            సరిపోలిక_Cond_Cond_Recurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            ఒకవేళ (!replaced) truesp.తీయి_వెనుక();
        }
        ఒకవేళ (స్థిరరాశి AstCond* subCondp = VN_CAST(nodep->elsep(), Cond)) {
            ఒకవేళ (!replaced) falsesp.emplace_వెనుక(nodep->condp());
            సరిపోలిక_Cond_Cond_Recurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            ఒకవేళ (!replaced) falsesp.తీయి_వెనుక();
        }
    }
    శూన్య సరిపోలిక_If_Cond_Cond(స్థిరరాశి Ast_కణుపు_If* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        ఒకవేళ (VN_IS(nodep->backp(), If)) తిరిగి;  // Was checked when visited parent
        ఒకవేళ (!VN_IS(nodep->thensp(), If) && !VN_IS(nodep->elsesp(), If)) తిరిగి;  // Short circuit
        std::సదిశ<Ast_కణుపు_సమాసం*> truesp;
        std::సదిశ<Ast_కణుపు_సమాసం*> falsesp;
        సరిపోలిక_If_Cond_Cond_Recurse(nodep, truesp /*ref*/, falsesp /*ref*/);
    }
    శూన్య సరిపోలిక_If_Cond_Cond_Recurse(స్థిరరాశి Ast_కణుపు_If* nodep, std::సదిశ<Ast_కణుపు_సమాసం*>& truesp,
                                std::సదిశ<Ast_కణుపు_సమాసం*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        ఒకవేళ (truesp.కొలత() > 4 || falsesp.కొలత() > 4) తిరిగి;
        ఒకవేళ (!nodep->condp()->is_శుద్ధ()) తిరిగి;
        // A constant condition is folded by visit(AstNodeIf), once statements can be deleted
        ఒకవేళ (VN_IS(nodep->condp(), Const)) తిరిగి;
        తార్కిక replaced = అబద్ధం;
        కొరకు (స్థిరరాశి Ast_కణుపు_సమాసం* condp : truesp) {
            ఒకవేళ (replaced) ఆపు;
            ఒకవేళ (!operandsSame(nodep->condp(), condp)) కొనసాగు;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_అంకె(nodep->condp(), 1);
            replaced = నిజం;
            ++m_stat_If_Cond_సమాసం_Redundant;
        }
        కొరకు (స్థిరరాశి Ast_కణుపు_సమాసం* condp : falsesp) {
            ఒకవేళ (replaced) ఆపు;
            ఒకవేళ (!operandsSame(nodep->condp(), condp)) కొనసాగు;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = నిజం;
            ++m_stat_If_Cond_సమాసం_Redundant;
        }
        // We only check the first statement of parent IF is an If
        // So we don't need to check for effects in the executing thensp/elsesp
        // altering the child't condition.  e.g. 'if (x) begin x=1; if (x) end'
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_If* subIfp = VN_CAST(nodep->thensp(), కణుపు_If)) {
            ఒకవేళ (!replaced) truesp.emplace_వెనుక(nodep->condp());
            సరిపోలిక_If_Cond_Cond_Recurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            ఒకవేళ (!replaced) truesp.తీయి_వెనుక();
        }
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_If* subIfp = VN_CAST(nodep->elsesp(), కణుపు_If)) {
            ఒకవేళ (!replaced) falsesp.emplace_వెనుక(nodep->condp());
            సరిపోలిక_If_Cond_Cond_Recurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            ఒకవేళ (!replaced) falsesp.తీయి_వెనుక();
        }
    }

    శూన్య సరిపోలిక_If_Same_చివరి(Ast_కణుపు_If* nodep) {
        ఒకవేళ (!nodep->thensp()) తిరిగి;
        ఒకవేళ (!nodep->elsesp()) తిరిగి;
        ఒకవేళ (!canDeleteStatements()) తిరిగి;
        // Iterate bodies in reverse
        Ast_కణుపు* tp = nodep->thensp()->lastp();
        Ast_కణుపు* ep = nodep->elsesp()->lastp();
        అయితే (tp && ep && tp->same_వృక్షం(ep)) {
            UASSERT_OBJ(!tp->nextp(), tp, "Expected no nextp");
            UASSERT_OBJ(!ep->nextp(), ep, "Expected no nextp");
            Ast_కణుపు* స్థిరరాశి tPrevp = tp->prevp();
            Ast_కణుపు* స్థిరరాశి ePrevp = ep->prevp();
            nodep->కూడు_తదుపరి_ఇక్కడ(tp->unlink_Fr_వెనుక());
            VL_DO_DANGLING(నెట్టు_Deletep(ep->unlink_Fr_వెనుక()), ep);
            tp = tPrevp;
            ep = ePrevp;
        }
    }

    తార్కిక సరిపోలిక_Masked_Or(AstAnd* nodep) {
        // Masking an OR with terms that have no bits set under the mask is replaced with masking
        // only the remaining terms. Canonical example as generated by V3Expand is:
        // 0xff & (a << 8 | b >> 24) --> 0xff & (b >> 24)

        // Compute how many significant bits are in the mask
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_AS(nodep->lhsp(), Const);
        స్థిరరాశి uint32_t significant_బిట్లు = constp->అంకె().వెడల్పు_గమ్యం_Fit();

        AstOr* స్థిరరాశి orp = VN_AS(nodep->rhsp(), Or);

        // Predicate for checking whether the bottom 'significantBits' bits of the given expression
        // are all zeroes.
        స్థిరరాశి స్వయంచాలక checkBottomClear = [=](స్థిరరాశి Ast_కణుపు* nodep) -> తార్కిక {
            ఒకవేళ (స్థిరరాశి Ast_జరుపు_L* స్థిరరాశి shiftp = VN_CAST(nodep, జరుపు_L)) {
                ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి scp = VN_CAST(shiftp->rhsp(), Const)) {
                    తిరిగి scp->అంకె().గమ్యం_UInt() >= significant_బిట్లు;
                }
            }
            తిరిగి అబద్ధం;
        };

        స్థిరరాశి తార్కిక orLIsRedundant = checkBottomClear(orp->lhsp());
        స్థిరరాశి తార్కిక orRIsRedundant = checkBottomClear(orp->rhsp());

        ఒకవేళ (orLIsRedundant && orRIsRedundant) {
            nodep->replaceWithKeepDType(
                కొత్త AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి నిజం;
        } లేకపోతే ఒకవేళ (orLIsRedundant) {
            orp->replaceWithKeepDType(orp->rhsp()->unlink_Fr_వెనుక());
            VL_DO_DANGLING(నెట్టు_Deletep(orp), orp);
            తిరిగి అబద్ధం;  // input node is still valid, keep going
        } లేకపోతే ఒకవేళ (orRIsRedundant) {
            orp->replaceWithKeepDType(orp->lhsp()->unlink_Fr_వెనుక());
            VL_DO_DANGLING(నెట్టు_Deletep(orp), orp);
            తిరిగి అబద్ధం;  // input node is still valid, keep going
        } లేకపోతే {
            తిరిగి అబద్ధం;
        }
    }
    తార్కిక సరిపోలిక_Masked_జరుపు(AstAnd* nodep) {
        // Drop redundant masking of right shift result. E.g: 0xff & ((uint32_t)a >> 24). This
        // commonly appears after V3Expand and the simplification in matchMaskedOr. Similarly,
        // drop redundant masking of left shift result. E.g.: 0xff000000 & ((uint32_t)a << 24).

        స్థిరరాశి స్వయంచాలక check_ముసుగు = [nodep, ఇది](స్థిరరాశి V3_సంఖ్య& ముసుగు) -> తార్కిక {
            స్థిరరాశి AstConst* స్థిరరాశి constp = VN_AS(nodep->lhsp(), Const);
            ఒకవేళ (constp->అంకె().isCaseEq(ముసుగు)) {
                Ast_కణుపు* స్థిరరాశి rhsp = nodep->rhsp();
                rhsp->unlink_Fr_వెనుక();
                nodep->replaceWithKeepDType(rhsp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి నిజం;
            }
            తిరిగి అబద్ధం;
        };

        // Check if masking is redundant
        ఒకవేళ (స్థిరరాశి Ast_జరుపు_R* స్థిరరాశి shiftp = VN_CAST(nodep->rhsp(), జరుపు_R)) {
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_సంఖ్య ముసుగు_Lo{nodep, nodep->వెడల్పు()};
                ముసుగు_Lo.సమితి_ముసుగు(nodep->వెడల్పు() - scp->అంకె().గమ్యం_UInt());
                తిరిగి check_ముసుగు(ముసుగు_Lo);
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_జరుపు_L* స్థిరరాశి shiftp = VN_CAST(nodep->rhsp(), జరుపు_L)) {
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_సంఖ్య ముసుగు{nodep, nodep->వెడల్పు()};
                స్థిరరాశి uint32_t జరుపు_Amount = scp->అంకె().గమ్యం_UInt();
                ముసుగు.సమితి_ముసుగు(nodep->వెడల్పు() - జరుపు_Amount, జరుపు_Amount);
                తిరిగి check_ముసుగు(ముసుగు);
            }
        }
        తిరిగి అబద్ధం;
    }

    తార్కిక సరిపోలిక_Masked_Zero(స్థిరరాశి AstAnd* nodep) {
        // Turn masking of known zero bits into constant zero. Commonly appears after V3Expand.
        స్థిరరాశి AstConst* స్థిరరాశి maskp = VN_AS(nodep->lhsp(), Const);
        స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp();
        స్థిరరాశి uint32_t msbP1 = maskp->అంకె().most_సమితి_బిట్టు_P1();
        ఒకవేళ (!msbP1) తిరిగి అబద్ధం;  // Don't rewrite, separate rule matches for this
        స్థిరరాశి uint32_t msb = msbP1 - 1;
        స్థిరరాశి uint32_t lsb = maskp->అంకె().కనిష్ఠ_సమితి_బిట్టు_P1() - 1;

        ఒకవేళ (స్థిరరాశి Ast_జరుపు_L* స్థిరరాశి shiftp = VN_CAST(rhsp, జరుపు_L)) {
            // 'a << S' forces the low S bits to zero
            ఒకవేళ (AstConst* స్థిరరాశి scp = VN_CAST(shiftp->rhsp(), Const)) {
                తిరిగి scp->అంకె().సరిపోతుంది_లోపల_UInt()  //
                       && (scp->అంకె().గమ్యం_UInt() > msb);
            }
        }
        ఒకవేళ (స్థిరరాశి Ast_జరుపు_R* స్థిరరాశి shiftp = VN_CAST(rhsp, జరుపు_R)) {
            // 'a >> S' forces the high S bits to zero. Check against the width of the shifted
            // operand, V3Expand can create shifts wider than their inputs. Must use width(),
            // not widthMin(), as bits above widthMin() are not guaranteed to be zero.
            ఒకవేళ (AstConst* స్థిరరాశి scp = VN_CAST(shiftp->rhsp(), Const)) {
                తిరిగి scp->అంకె().సరిపోతుంది_లోపల_UInt()
                       && (lsb + scp->అంకె().గమ్యం_UInt()
                           >= static_cast<uint32_t>(shiftp->lhsp()->వెడల్పు()));
            }
        }
        ఒకవేళ (స్థిరరాశి AstMul* స్థిరరాశి mulp = VN_CAST(rhsp, Mul)) {
            // 'C * a' forces the low N bits to zero where 'C' has low zero bits
            ఒకవేళ (AstConst* స్థిరరాశి cp = VN_CAST(mulp->lhsp(), Const)) {
                తిరిగి cp->అంకె().కనిష్ఠ_సమితి_బిట్టు_P1() > msb + 1;
            }
        }
        ఒకవేళ (స్థిరరాశి AstExtend* స్థిరరాశి extendp = VN_CAST(rhsp, Extend)) {
            // Zero-extension forces the bits above the source width to zero
            తిరిగి lsb >= static_cast<uint32_t>(extendp->lhsp()->వెడల్పు());
        }

        తిరిగి అబద్ధం;
    }

    తార్కిక సరిపోలిక_బిట్టు_Op_వృక్షం(Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (nodep->వెడల్పు_Min() != 1) తిరిగి అబద్ధం;
        ఒకవేళ (!v3Global.opt.f_Const_బిట్టు_Op_వృక్షం()) తిరిగి అబద్ధం;

        స్థిరరాశి పూర్ణాంకం వెడల్పు = nodep->వెడల్పు();
        Ast_కణుపు_సమాసం* rootp = nodep;
        గుర్తులేని externalOps = 0;
        // Reach past a plain making AND
        ఒకవేళ (స్థిరరాశి AstAnd* స్థిరరాశి andp = VN_CAST(nodep, And)) {
            ఒకవేళ (isConst(andp->lhsp(), 1)) {
                rootp = andp->rhsp();
                externalOps = 1;
            }
        }

        // Only optimize if rootp is in fact the root of a tree of identical
        // operations, otherwise analysis on an eventually unoptimizable unablanced
        // tree can go O(N^2) as we would re-analyze every time as we move up the
        // tree, repeatedly re-discover the sub-tree is not optimizable.
        ఒకవేళ (rootp->type() == nodep->backp()->type()) తిరిగి అబద్ధం;

        std::string debug_ఉపసర్గ;
        ఒకవేళ (debug() >= 9) {  // LCOV_EXCL_START
            స్థిర పూర్ణాంకం s_c = 0;
            debug_ఉపసర్గ = "-  matchBitOpTree[" + std::గమ్యం_string(++s_c) + "] ";
            nodep->dump_వృక్షం(debug_ఉపసర్గ + "INPUT: ");
        }  // LCOV_EXCL_STOP

        Ast_కణుపు_సమాసం* స్థిరరాశి newp = Const_బిట్టు_Op_వృక్షం_Visitor::simplify(rootp, వెడల్పు, externalOps,
                                                                  m_stat_బిట్టు_Op_Reduction, m_ids);

        ఒకవేళ (newp) {
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Transformed leaf of bit tree to " << newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }

        ఒకవేళ (debug() >= 9) {  // LCOV_EXCL_START
            ఒకవేళ (newp) {
                newp->dump_వృక్షం(debug_ఉపసర్గ + "RESULT: ");
            } లేకపోతే {
                బయటకు << debug_ఉపసర్గ << "not replaced\n";
            }
        }  // LCOV_EXCL_STOP

        తిరిగి newp;
    }
    స్థిర తార్కిక నిర్వాహ్యం_జరుపు_Same(స్థిరరాశి Ast_కణుపు* nodep) {
        స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి np = VN_AS(nodep, కణుపు_Biop);
        {
            స్థిరరాశి Ast_జరుపు_L* స్థిరరాశి lp = VN_CAST(np->lhsp(), జరుపు_L);
            స్థిరరాశి Ast_జరుపు_L* స్థిరరాశి rp = VN_CAST(np->rhsp(), జరుపు_L);
            ఒకవేళ (lp && rp) {
                తిరిగి (lp->వెడల్పు() == rp->వెడల్పు() && lp->lhsp()->వెడల్పు() == rp->lhsp()->వెడల్పు()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        {
            స్థిరరాశి Ast_జరుపు_R* స్థిరరాశి lp = VN_CAST(np->lhsp(), జరుపు_R);
            స్థిరరాశి Ast_జరుపు_R* స్థిరరాశి rp = VN_CAST(np->rhsp(), జరుపు_R);
            ఒకవేళ (lp && rp) {
                తిరిగి (lp->వెడల్పు() == rp->వెడల్పు() && lp->lhsp()->వెడల్పు() == rp->lhsp()->వెడల్పు()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        తిరిగి అబద్ధం;
    }
    తార్కిక నిర్వాహ్యం_Huge_జరుపు_L(స్థిరరాశి Ast_కణుపు_Biop* nodep) {
        తిరిగి (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->అంకె().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->అంకె().సరిపోతుంది_లోపల_UInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->గమ్యం_UInt()
                        >= static_cast<uint32_t>(nodep->వెడల్పు())))
                && nodep->lhsp()->is_శుద్ధ());
    }
    తార్కిక నిర్వాహ్యం_Huge_జరుపు_R(స్థిరరాశి Ast_కణుపు_Biop* nodep) {
        తిరిగి (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->అంకె().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->అంకె().సరిపోతుంది_లోపల_UInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->గమ్యం_UInt()
                        >= static_cast<uint32_t>(nodep->lhsp()->వెడల్పు())))
                && nodep->lhsp()->is_శుద్ధ()
                && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), ప్రవాహం_DType)));
    }
    తార్కిక నిర్వాహ్యం_Is_Two(స్థిరరాశి Ast_కణుపు* nodep) {
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const);
        ఒకవేళ (!constp) తిరిగి అబద్ధం;  // not constant
        ఒకవేళ (constp->అంకె().isFourState()) తిరిగి అబద్ధం;  // four-state
        ఒకవేళ (nodep->వెడల్పు() > VL_QUADSIZE) తిరిగి అబద్ధం;  // too wide
        ఒకవేళ (nodep->isSigned() && constp->అంకె().isNegative()) తిరిగి అబద్ధం;  // signed and negative
        తిరిగి constp->గమ్యం_UQuad() == 2;
    }
    తార్కిక నిర్వాహ్యం_Is_Twostate(స్థిరరాశి Ast_కణుపు* nodep) {
        తిరిగి (VN_IS(nodep, Const) && !VN_AS(nodep, Const)->అంకె().isFourState());
    }
    తార్కిక నిర్వాహ్యం_Is_Pow_Two(స్థిరరాశి Ast_కణుపు* nodep) {
        ఒకవేళ (!నిర్వాహ్యం_Is_Twostate(nodep)) తిరిగి అబద్ధం;
        తిరిగి (1 == VN_AS(nodep, Const)->అంకె().లెక్క_Ones());
    }
    తార్కిక నిర్వాహ్యం_జరుపు_Op(స్థిరరాశి Ast_కణుపు_Biop* nodep) {
        ఒకవేళ (!VN_IS(nodep->rhsp(), Const)) తిరిగి అబద్ధం;
        స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి lhsp = VN_CAST(nodep->lhsp(), కణుపు_Biop);
        ఒకవేళ (!lhsp || !(VN_IS(lhsp, And) || VN_IS(lhsp, Or) || VN_IS(lhsp, Xor))) తిరిగి అబద్ధం;
        ఒకవేళ (nodep->వెడల్పు() != lhsp->వెడల్పు()) తిరిగి అబద్ధం;
        ఒకవేళ (nodep->వెడల్పు() != lhsp->lhsp()->వెడల్పు()) తిరిగి అబద్ధం;
        ఒకవేళ (nodep->వెడల్పు() != lhsp->rhsp()->వెడల్పు()) తిరిగి అబద్ధం;

        // Only do it if we can fruther reduce one of the sides if pulling the shift through:
        // - Either operands are constants
        ఒకవేళ (VN_IS(lhsp->lhsp(), Const)) తిరిగి నిజం;
        ఒకవేళ (VN_IS(lhsp->rhsp(), Const)) తిరిగి నిజం;
        // - Const shift on either side
        ఒకవేళ (Ast_కణుపు_Biop* స్థిరరాశి llp = VN_CAST(lhsp->lhsp(), కణుపు_Biop)) {
            తిరిగి (VN_IS(llp, జరుపు_L) || VN_IS(llp, జరుపు_R)) && VN_IS(llp->rhsp(), Const);
        }
        ఒకవేళ (Ast_కణుపు_Biop* స్థిరరాశి lrp = VN_CAST(lhsp->rhsp(), కణుపు_Biop)) {
            తిరిగి (VN_IS(lrp, జరుపు_L) || VN_IS(lrp, జరుపు_R)) && VN_IS(lrp->rhsp(), Const);
        }
        // Otherwise we would increase logic size
        తిరిగి అబద్ధం;
    }
    తార్కిక నిర్వాహ్యం_జరుపు_జరుపు(స్థిరరాశి Ast_కణుపు_Biop* nodep) {
        // We could add a AND though.
        స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి lhsp = VN_CAST(nodep->lhsp(), కణుపు_Biop);
        ఒకవేళ (!lhsp || !(VN_IS(lhsp, జరుపు_L) || VN_IS(lhsp, జరుపు_R))) తిరిగి అబద్ధం;
        // We can only get rid of a<<b>>c or a<<b<<c, with constant b & c
        // because bits may be masked in that process, or (b+c) may exceed the word width.
        ఒకవేళ (!(VN_IS(nodep->rhsp(), Const) && VN_IS(lhsp->rhsp(), Const))) తిరిగి అబద్ధం;
        ఒకవేళ (VN_AS(nodep->rhsp(), Const)->అంకె().isFourState()
            || VN_AS(lhsp->rhsp(), Const)->అంకె().isFourState())
            తిరిగి అబద్ధం;
        ఒకవేళ (nodep->వెడల్పు() != lhsp->వెడల్పు()) తిరిగి అబద్ధం;
        ఒకవేళ (nodep->వెడల్పు() != lhsp->lhsp()->వెడల్పు()) తిరిగి అబద్ధం;
        తిరిగి నిజం;
    }
    తార్కిక నిర్వాహ్యం_మాట_OOB(స్థిరరాశి Ast_మాట_Sel* nodep) {
        // V3Expand may make a arraysel that exceeds the bounds of the array
        // It was an expression, then got constified.  In reality, the WordSel
        // must be wrapped in a Cond, that will be false.
        తిరిగి (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), కణుపు_Var_Ref)
                && VN_AS(nodep->fromp(), కణుపు_Var_Ref)->access().isReadOnly()
                && (static_cast<పూర్ణాంకం>(VN_AS(nodep->bitp(), Const)->గమ్యం_UInt())
                    >= VN_AS(nodep->fromp(), కణుపు_Var_Ref)->varp()->వెడల్పు_పదాలు()));
    }
    తార్కిక నిర్వాహ్యం_Sel_నిండిన(స్థిరరాశి AstSel* nodep) {
        తిరిగి (VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0
                && static_cast<పూర్ణాంకం>(nodep->వెడల్పు_Const()) == nodep->fromp()->వెడల్పు());
    }
    తార్కిక నిర్వాహ్యం_Sel_Extend(AstSel* nodep) {
        ఒకవేళ (!m_doV) తిరిగి అబద్ధం;
        AstExtend* స్థిరరాశి extendp = VN_CAST(nodep->fromp(), Extend);
        ఒకవేళ (!extendp) తిరిగి అబద్ధం;
        AstConst* స్థిరరాశి lsbp = VN_CAST(nodep->lsbp(), Const);
        ఒకవేళ (!lsbp) తిరిగి అబద్ధం;
        స్థిరరాశి పూర్ణాంకం వెడల్పు = nodep->వెడల్పు_Const();
        స్థిరరాశి పూర్ణాంకం lsb = lsbp->గమ్యం_SInt();
        స్థిరరాశి పూర్ణాంకం msb = lsb + వెడల్పు - 1;
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = extendp->lhsp();
        ఒకవేళ (!lhsp->is_శుద్ధ()) తిరిగి అబద్ధం;

        // Selecting the entire extended expression, replace with that
        ఒకవేళ (lsb == 0 && msb == lhsp->వెడల్పు() - 1) {
            lhsp->unlink_Fr_వెనుక();
            nodep->replaceWithKeepDType(lhsp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి నిజం;
        }
        // Select entirely in the extended part - replace with zero
        ఒకవేళ (lsb >= lhsp->వెడల్పు()) {
            replaceZero(nodep);
            తిరిగి నిజం;
        }
        // Select entirely in the extended expression - replace with select from that
        ఒకవేళ (msb < lhsp->వెడల్పు()) {
            lhsp->unlink_Fr_వెనుక();
            lsbp->unlink_Fr_వెనుక();
            nodep->replaceWithKeepDType(కొత్త AstSel{nodep->fileline(), lhsp, lsbp, వెడల్పు});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి నిజం;
        }
        // Select straddles both sides, but is just a shorter extend
        ఒకవేళ (lsb == 0) {
            lhsp->unlink_Fr_వెనుక();
            nodep->replaceWithKeepDType(కొత్త AstExtend{nodep->fileline(), lhsp});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి నిజం;
        }

        తిరిగి అబద్ధం;
    }
    తార్కిక నిర్వాహ్యం_Sel_Bi_క్రింది(AstSel* nodep) {
        // SEL(ADD(a,b),(width-1),0) -> ADD(SEL(a),SEL(b))
        // Add or any operation which doesn't care if we discard top bits
        Ast_కణుపు_Biop* స్థిరరాశి bip = VN_CAST(nodep->fromp(), కణుపు_Biop);
        ఒకవేళ (!(m_doV && bip && VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0)) తిరిగి అబద్ధం;
        UINFOTREE(9, nodep, "", "SEL(BI)-in");
        Ast_కణుపు_సమాసం* స్థిరరాశి bilhsp = bip->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి birhsp = bip->rhsp()->unlink_Fr_వెనుక();
        bip->lhsp(కొత్త AstSel{nodep->fileline(), bilhsp, 0, nodep->వెడల్పు_Const()});
        bip->rhsp(కొత్త AstSel{nodep->fileline(), birhsp, 0, nodep->వెడల్పు_Const()});
        UINFOTREE(9, bip, "", "SEL(BI)-ou");
        VL_DO_DANGLING(replaceWChild(nodep, bip), nodep);
        తిరిగి నిజం;
    }
    తార్కిక నిర్వాహ్యం_Sel_జరుపు_క్రింది(AstSel* nodep) {
        // AND({a}, SHIFTR({b}, {c})) is often shorthand in C for Verilog {b}[{c} :+ {a}]
        // becomes thought other optimizations
        // SEL(SHIFTR({a},{b}),{lsb},{width}) -> SEL({a},{lsb+b},{width})
        స్థిరరాశి Ast_జరుపు_R* స్థిరరాశి shiftp = VN_CAST(nodep->fromp(), జరుపు_R);
        ఒకవేళ (!(m_doV && shiftp && VN_IS(shiftp->rhsp(), Const) && VN_IS(nodep->lsbp(), Const))) {
            తిరిగి అబద్ధం;
        }
        Ast_కణుపు_సమాసం* స్థిరరాశి ap = shiftp->lhsp();
        స్థిరరాశి AstConst* స్థిరరాశి bp = VN_AS(shiftp->rhsp(), Const);
        స్థిరరాశి AstConst* స్థిరరాశి lp = VN_AS(nodep->lsbp(), Const);
        ఒకవేళ (bp->isWide() || bp->అంకె().isFourState() || bp->అంకె().isNegative() || lp->isWide()
            || lp->అంకె().isFourState() || lp->అంకె().isNegative()) {
            తిరిగి అబద్ధం;
        }
        స్థిరరాశి పూర్ణాంకం newLsb = lp->గమ్యం_SInt() + bp->గమ్యం_SInt();
        ఒకవేళ (newLsb + nodep->వెడల్పు_Const() > ap->వెడల్పు()) తిరిగి అబద్ధం;
        //
        UINFO(9, "SEL(SHIFTR(a,b),l,w) -> SEL(a,l+b,w)");
        UINFOTREE(9, nodep, "", "SEL(SH)-in");
        AstSel* స్థిరరాశి newp
            = కొత్త AstSel{nodep->fileline(), ap->unlink_Fr_వెనుక(), newLsb, nodep->వెడల్పు_Const()};
        nodep->replaceWithKeepDType(newp);
        UINFOTREE(9, newp, "", "SEL(SH)-ou");
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        తిరిగి నిజం;
    }

    తార్కిక నిర్వాహ్యం_Bi_Extend_Const_Shrink(Ast_కణుపు_Biop* nodep) {
        // Loop unrolling favors standalone compares
        // EQ(const{width32}, EXTEND(xx{width3})) -> EQ(const{3}, xx{3})
        // The constant must have zero bits (+ 1 if signed) or compare
        // would be incorrect. See also operandBiExtendConst
        AstExtend* స్థిరరాశి extendp = VN_CAST(nodep->rhsp(), Extend);
        ఒకవేళ (!extendp) తిరిగి అబద్ధం;
        Ast_కణుపు_సమాసం* స్థిరరాశి smallerp = extendp->lhsp();
        స్థిరరాశి పూర్ణాంకం subsize = smallerp->వెడల్పు();
        AstConst* స్థిరరాశి constp = VN_CAST(nodep->lhsp(), Const);
        ఒకవేళ (!constp) తిరిగి అబద్ధం;
        ఒకవేళ (!constp->అంకె().is_బిట్లు_Zero(constp->వెడల్పు() - 1, subsize)) తిరిగి అబద్ధం;
        //
        UINFOTREE(9, nodep, "", "BI(EXTEND)-in");
        smallerp->unlink_Fr_వెనుక();
        VL_DO_DANGLING(నెట్టు_Deletep(extendp->unlink_Fr_వెనుక()), extendp);  // aka nodep->lhsp.
        nodep->rhsp(smallerp);

        constp->unlink_Fr_వెనుక();
        స్థిరరాశి V3_సంఖ్య అంకె{constp, subsize, constp->అంకె()};
        nodep->lhsp(కొత్త AstConst{constp->fileline(), అంకె});
        VL_DO_DANGLING(నెట్టు_Deletep(constp), constp);
        తిరిగి అబద్ధం;  // input node is still valid, keep going
    }
    తార్కిక నిర్వాహ్యం_Bi_Extend_Const_Over(స్థిరరాశి Ast_కణుపు_Biop* nodep) {
        // EQ(const{width32}, EXTEND(xx{width3})) -> constant
        // When the constant has non-zero bits above the extend it's a constant.
        // Avoids compiler warning
        స్థిరరాశి AstExtend* స్థిరరాశి extendp = VN_CAST(nodep->rhsp(), Extend);
        ఒకవేళ (!extendp) తిరిగి అబద్ధం;
        స్థిరరాశి Ast_కణుపు* స్థిరరాశి smallerp = extendp->lhsp();
        స్థిరరాశి పూర్ణాంకం subsize = smallerp->వెడల్పు();
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->lhsp(), Const);
        ఒకవేళ (!constp) తిరిగి అబద్ధం;
        ఒకవేళ (constp->అంకె().is_బిట్లు_Zero(constp->వెడల్పు() - 1, subsize)) తిరిగి అబద్ధం;
        తిరిగి నిజం;
    }

    // Extraction checks
    తార్కిక warnSelect(AstSel* nodep) {
        ఒకవేళ (m_doGenerate) {
            // Never checked yet
            V3_వెడల్పు::వెడల్పు_Params_Edit(nodep);
            iterateChildren(nodep);  // May need "constifying"
        }
        // Find range of dtype we are selecting from
        // Similar code in V3Unknown::AstSel
        స్థిరరాశి తార్కిక doit = నిజం;
        ఒకవేళ (m_warn && VN_IS(nodep->lsbp(), Const) && doit) {
            స్థిరరాశి పూర్ణాంకం max_Decl_బిట్టు = nodep->declRange().hiMaxSelect() * nodep->decl_El_వెడల్పు()
                                   + (nodep->decl_El_వెడల్పు() - 1);
            ఒకవేళ (VN_AS(nodep->lsbp(), Const)->అంకె().isFourState()) {
                nodep->v3error("Selection index is constantly unknown or tristated: "
                               "lsb="
                               << nodep->lsbp()->name() << " width=" << nodep->వెడల్పు_Const());
                // Replacing nodep will make a mess above, so we replace the offender
                replaceZero(nodep->lsbp());
            } లేకపోతే ఒకవేళ (nodep->declRange().ranged()
                       && (nodep->msbConst() > max_Decl_బిట్టు || nodep->lsbConst() > max_Decl_బిట్టు)) {
                // See also warning in V3Width
                // Must adjust by element width as declRange() is in number of elements
                string msbLsbProtected;
                ఒకవేళ (nodep->decl_El_వెడల్పు() == 0) {
                    msbLsbProtected = "(nodep->declElWidth() == 0) "
                                      + std::గమ్యం_string(nodep->msbConst()) + ":"
                                      + std::గమ్యం_string(nodep->lsbConst());
                } లేకపోతే {
                    msbLsbProtected = std::గమ్యం_string(nodep->msbConst() / nodep->decl_El_వెడల్పు())
                                      + ":"
                                      + std::గమ్యం_string(nodep->lsbConst() / nodep->decl_El_వెడల్పు());
                }
                nodep->v3warn(SELRANGE,
                              "Selection index out of range: "
                                  << msbLsbProtected << " outside "
                                  << nodep->declRange().hiMaxSelect() << ":0"
                                  << (nodep->declRange().lo() >= 0
                                          ? ""
                                          : (" (adjusted +" + cvt_గమ్యం_Str(-nodep->declRange().lo())
                                             + " to account for negative lsb)")));
                UINFO(1, "    Related Raw index is " << nodep->msbConst() << ":"
                                                     << nodep->lsbConst());
                // Don't replace with zero, we'll do it later
            }
        }
        తిరిగి అబద్ధం;  // Not a transform, so NOP
    }

    స్థిర తార్కిక operandsSame(స్థిరరాశి Ast_కణుపు* node1p, స్థిరరాశి Ast_కణుపు* node2p) {
        // For now we just detect constants & simple vars, though it could be more generic
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి const1p = VN_CAST(node1p, Const)) {
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి const2p = VN_CAST(node2p, Const)) {
                తిరిగి V3_సంఖ్య{node1p->fileline(), 1, 0}
                    .opEq(const1p->అంకె(), const2p->అంకె())
                    .isEqOne();
            }
        }
        ఒకవేళ (VN_IS(node1p, VarRef) && VN_IS(node2p, VarRef)) {
            // Avoid comparing widthMin's, which results in lost optimization attempts
            // If cleanup sameGateTree to be smarter, this can be restored.
            // return node1p->sameGateTree(node2p);
            తిరిగి node1p->isSame(node2p);
        }
        // Pattern created by coverage-line/branch; avoid compiler tautological-compare warning
        ఒకవేళ (స్థిరరాశి AstAnd* స్థిరరాశి and1p = VN_CAST(node1p, And)) {
            ఒకవేళ (స్థిరరాశి AstAnd* స్థిరరాశి and2p = VN_CAST(node2p, And)) {
                ఒకవేళ (VN_IS(and1p->lhsp(), Const) && VN_IS(and1p->rhsp(), కణుపు_Var_Ref)
                    && VN_IS(and2p->lhsp(), Const) && VN_IS(and2p->rhsp(), కణుపు_Var_Ref))
                    తిరిగి node1p->same_Gate_వృక్షం(node2p);
            }
        }
        తిరిగి అబద్ధం;
    }
    తార్కిక if_Same_కేటాయించు(స్థిరరాశి Ast_కణుపు_If* nodep) {
        స్థిరరాశి Ast_కణుపు_కేటాయించు* స్థిరరాశి thensp = VN_CAST(nodep->thensp(), కణుపు_కేటాయించు);
        స్థిరరాశి Ast_కణుపు_కేటాయించు* స్థిరరాశి elsesp = VN_CAST(nodep->elsesp(), కణుపు_కేటాయించు);
        ఒకవేళ (!thensp || thensp->nextp()) తిరిగి అబద్ధం;  // Must be SINGLE statement
        ఒకవేళ (!elsesp || elsesp->nextp()) తిరిగి అబద్ధం;
        ఒకవేళ (thensp->type() != elsesp->type()) తిరిగి అబద్ధం;  // Can't mix an assigndly with assign
        ఒకవేళ (!thensp->lhsp()->same_Gate_వృక్షం(elsesp->lhsp())) తిరిగి అబద్ధం;
        ఒకవేళ (!thensp->rhsp()->gate_వృక్షం()) తిరిగి అబద్ధం;
        ఒకవేళ (!elsesp->rhsp()->gate_వృక్షం()) తిరిగి అబద్ధం;
        // Must not create an expression with unpacked array type
        ఒకవేళ (VN_IS(thensp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) తిరిగి అబద్ధం;
        ఒకవేళ (VN_IS(elsesp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) తిరిగి అబద్ధం;
        ఒకవేళ (m_underRecFunc) తిరిగి అబద్ధం;  // This optimization may lead to infinite recursion
        ఒకవేళ (m_doCpp) {  // Do not combine ifs cretaed by V3Premit
            ఒకవేళ (VN_IS(thensp->lhsp(), VarRef) && thensp->lhsp()->isWide()) తిరిగి అబద్ధం;
        }
        // Only do it if not calls and both pure, otherwise undoes V3LiftExpr
        తిరిగి !VN_IS(thensp->rhsp(), కణుపు_FTask_Ref)  //
               && !VN_IS(elsesp->rhsp(), కణుపు_FTask_Ref)  //
               && thensp->rhsp()->is_శుద్ధ()  //
               && elsesp->rhsp()->is_శుద్ధ();
    }
    తార్కిక నిర్వాహ్యం_If_If(స్థిరరాశి Ast_కణుపు_If* nodep) {
        ఒకవేళ (nodep->elsesp()) తిరిగి అబద్ధం;
        స్థిరరాశి Ast_కణుపు_If* స్థిరరాశి క్రింది_Ifp = VN_CAST(nodep->thensp(), కణుపు_If);
        ఒకవేళ (!క్రింది_Ifp || క్రింది_Ifp->nextp()) తిరిగి అబద్ధం;
        ఒకవేళ (nodep->type() != క్రింది_Ifp->type()) తిరిగి అబద్ధం;
        ఒకవేళ (Ast_కణుపు::afterCommentp(క్రింది_Ifp->elsesp())) తిరిగి అబద్ధం;
        తిరిగి నిజం;
    }
    తార్కిక ifConcatMergeableBiop(స్థిరరాశి Ast_కణుపు* nodep) {
        తిరిగి (VN_IS(nodep, And) || VN_IS(nodep, Or) || VN_IS(nodep, Xor));
    }
    తార్కిక ifAdjacentSel(స్థిరరాశి AstSel* lhsp, స్థిరరాశి AstSel* rhsp) {
        ఒకవేళ (!v3Global.opt.fAssemble()) తిరిగి అబద్ధం;  // opt disabled
        ఒకవేళ (!lhsp || !rhsp) తిరిగి అబద్ధం;
        స్థిరరాశి Ast_కణుపు* స్థిరరాశి lfromp = lhsp->fromp();
        స్థిరరాశి Ast_కణుపు* స్థిరరాశి rfromp = rhsp->fromp();
        ఒకవేళ (!lfromp || !rfromp || !lfromp->same_Gate_వృక్షం(rfromp)) తిరిగి అబద్ధం;
        స్థిరరాశి AstConst* స్థిరరాశి lstart = VN_CAST(lhsp->lsbp(), Const);
        స్థిరరాశి AstConst* స్థిరరాశి rstart = VN_CAST(rhsp->lsbp(), Const);
        ఒకవేళ (!lstart || !rstart) తిరిగి అబద్ధం;  // too complicated
        స్థిరరాశి పూర్ణాంకం rend = (rstart->గమ్యం_SInt() + rhsp->వెడల్పు_Const());
        తిరిగి (rend == lstart->గమ్యం_SInt());
    }
    తార్కిక if_విలీనం_Adjacent(స్థిరరాశి Ast_కణుపు_సమాసం* lhsp, స్థిరరాశి Ast_కణుపు_సమాసం* rhsp) {
        // Called by concatmergeable to determine if {lhsp, rhsp} make sense
        ఒకవేళ (!v3Global.opt.fAssemble()) తిరిగి అబద్ధం;  // opt disabled
        // Two same varref
        ఒకవేళ (operandsSame(lhsp, rhsp)) తిరిగి నిజం;
        స్థిరరాశి AstSel* స్థిరరాశి lselp = VN_CAST(lhsp, Sel);
        స్థిరరాశి AstSel* స్థిరరాశి rselp = VN_CAST(rhsp, Sel);
        ఒకవేళ (!lselp && !rselp) తిరిగి అబద్ధం;
        ఒకవేళ (lselp && !VN_IS(lselp->lsbp(), Const)) తిరిగి అబద్ధం;
        ఒకవేళ (rselp && !VN_IS(rselp->lsbp(), Const)) తిరిగి అబద్ధం;
        // a[i:j] {a[j-1:k], b}
        ఒకవేళ (lselp && !rselp && VN_IS(rhsp, Concat))
            తిరిగి if_విలీనం_Adjacent(lhsp, VN_AS(rhsp, Concat)->lhsp());
        // {b, a[j:k]} a[k-1:i]
        ఒకవేళ (rselp && !lselp && VN_IS(lhsp, Concat))
            తిరిగి if_విలీనం_Adjacent(VN_AS(lhsp, Concat)->rhsp(), rhsp);
        // a a[msb:j]
        స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి lfromp = lselp                                ? lselp->fromp()
                                          : lhsp->same_Gate_వృక్షం(rselp->fromp()) ? lhsp
                                                                               : శూన్యసూచిక;
        // a[i:0] a
        స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి rfromp = rselp                                ? rselp->fromp()
                                          : rhsp->same_Gate_వృక్షం(lselp->fromp()) ? rhsp
                                                                               : శూన్యసూచిక;
        ఒకవేళ (!lfromp || !rfromp || !lfromp->same_Gate_వృక్షం(rfromp)) తిరిగి అబద్ధం;

        స్థిరరాశి int32_t lstart = lselp ? lselp->lsbConst() : 0;
        స్థిరరాశి int32_t rstart = rselp ? rselp->lsbConst() : 0;
        స్థిరరాశి int32_t rend = rstart + (rselp ? rselp->వెడల్పు_Const() : rhsp->వెడల్పు());
        // a[i:j] a[j-1:k]
        ఒకవేళ (rend == lstart) తిరిగి నిజం;
        // a[i:0] a[msb:j]
        ఒకవేళ (rend == rfromp->వెడల్పు() && lstart == 0) తిరిగి నిజం;
        తిరిగి అబద్ధం;
    }
    తార్కిక concatMergeable(స్థిరరాశి Ast_కణుపు_సమాసం* lhsp, స్థిరరాశి Ast_కణుపు_సమాసం* rhsp, గుర్తులేని లోతు) {
        // Determine if {a OP b, c OP d} => {a, c} OP {b, d} is advantageous
        ఒకవేళ (!v3Global.opt.fAssemble()) తిరిగి అబద్ధం;  // opt disabled
        ఒకవేళ (lhsp->type() != rhsp->type()) తిరిగి అబద్ధం;
        ఒకవేళ (!ifConcatMergeableBiop(lhsp)) తిరిగి అబద్ధం;
        ఒకవేళ (లోతు > CONCAT_MERGABLE_MAX_లోతు) తిరిగి అబద్ధం;  // As worse case O(n^2) algorithm

        స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి lp = VN_CAST(lhsp, కణుపు_Biop);
        స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి rp = VN_CAST(rhsp, కణుపు_Biop);
        ఒకవేళ (!lp || !rp) తిరిగి అబద్ధం;
        // {a[]&b[], a[]&b[]}
        స్థిరరాశి తార్కిక lad = if_విలీనం_Adjacent(lp->lhsp(), rp->lhsp());
        స్థిరరాశి తార్కిక rad = if_విలీనం_Adjacent(lp->rhsp(), rp->rhsp());
        ఒకవేళ (lad && rad) తిరిగి నిజం;
        // {a[] & b[]&c[], a[] & b[]&c[]}
        ఒకవేళ (lad && concatMergeable(lp->rhsp(), rp->rhsp(), లోతు + 1)) తిరిగి నిజం;
        // {a[]&b[] & c[], a[]&b[] & c[]}
        ఒకవేళ (rad && concatMergeable(lp->lhsp(), rp->lhsp(), లోతు + 1)) తిరిగి నిజం;
        // {(a[]&b[])&(c[]&d[]), (a[]&b[])&(c[]&d[])}
        ఒకవేళ (concatMergeable(lp->lhsp(), rp->lhsp(), లోతు + 1)
            && concatMergeable(lp->rhsp(), rp->rhsp(), లోతు + 1)) {
            తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }
    స్థిర తార్కిక operands_Same_వెడల్పు(స్థిరరాశి Ast_కణుపు* lhsp, స్థిరరాశి Ast_కణుపు* rhsp) {
        తిరిగి lhsp->వెడల్పు() == rhsp->వెడల్పు();
    }

    //----------------------------------------
    // Constant Replacement functions.
    // These all take a node, delete its tree, and replaces it with a constant

    శూన్య replace_అంకె(Ast_కణుపు* oldp, స్థిరరాశి V3_సంఖ్య& అంకె, స్థిరరాశి string& origParamName = "") {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        UASSERT_OBJ(!(VN_IS(oldp, Const) && !VN_AS(oldp, Const)->అంకె().isFourState()), oldp,
                    "Already constant??");
        AstConst* స్థిరరాశి newp = కొత్త AstConst{oldp->fileline(), అంకె};
        ఒకవేళ (!origParamName.ఖాళీలు()) newp->origParamName(origParamName);
        oldp->replaceWithKeepDType(newp);
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        VL_DO_DANGLING(నెట్టు_Deletep(oldp), oldp);
    }
    శూన్య replace_అంకె(Ast_కణుపు* nodep, uint32_t val) {
        స్థిరరాశి V3_సంఖ్య అంకె{nodep, nodep->వెడల్పు(), val};
        VL_DO_DANGLING(replace_అంకె(nodep, అంకె), nodep);
    }
    శూన్య replace_అంకె_Signed(Ast_కణుపు_Biop* nodep, uint32_t val) {
        // We allow both sides to be constant, as one may have come from
        // parameter propagation, etc.
        ఒకవేళ (m_warn && !(VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const))) {
            nodep->v3warn(UNSIGNED, "Comparison is constant due to unsigned arithmetic");
        }
        VL_DO_DANGLING(replace_అంకె(nodep, val), nodep);
    }
    శూన్య replace_అంకె_Limited(Ast_కణుపు_Biop* nodep, uint32_t val) {
        // Avoids gcc warning about same
        ఒకవేళ (m_warn) nodep->v3warn(CMPCONST, "Comparison is constant due to limited range");
        VL_DO_DANGLING(replace_అంకె(nodep, val), nodep);
    }
    శూన్య replaceZero(Ast_కణుపు* nodep) { VL_DO_DANGLING(replace_అంకె(nodep, 0), nodep); }
    శూన్య replace_Zero_Chk_శుద్ధ(Ast_కణుపు* nodep, Ast_కణుపు_సమాసం* checkp) {
        // For example, "0 * n" -> 0 if n has no side effects
        // Else strength reduce it to 0 & n.
        // If ever change the operation note AstAnd rule specially ignores this created pattern
        ఒకవేళ (checkp->is_శుద్ధ()) {
            VL_DO_DANGLING(replace_అంకె(nodep, 0), nodep);
        } లేకపోతే {
            Ast_కణుపు* స్థిరరాశి newp = కొత్త AstAnd{nodep->fileline(), కొత్త AstConst{nodep->fileline(), 0},
                                             checkp->unlink_Fr_వెనుక()};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }
    శూన్య replaceAllOnes(Ast_కణుపు* nodep) {
        V3_సంఖ్య ones{nodep, nodep->వెడల్పు(), 0};
        ones.సమితి_ముసుగు(nodep->వెడల్పు());
        VL_DO_DANGLING(replace_అంకె(nodep, ones), nodep);
    }
    శూన్య replaceConst(Ast_కణుపు_Uniop* nodep) {
        V3_సంఖ్య numv{nodep, nodep->వెడల్పు_Min_V()};
        nodep->సంఖ్య_నిర్వహించు(numv, const_అంకె_V(nodep->lhsp()));
        స్థిరరాశి V3_సంఖ్య& అంకె = గమ్యం_అంకె_C(nodep, numv);
        UINFO(4, "UNICONST -> " << అంకె);
        VL_DO_DANGLING(replace_అంకె(nodep, అంకె), nodep);
    }
    శూన్య replaceConst(Ast_కణుపు_Biop* nodep) {
        V3_సంఖ్య numv{nodep, nodep->వెడల్పు_Min_V()};
        nodep->సంఖ్య_నిర్వహించు(numv, const_అంకె_V(nodep->lhsp()), const_అంకె_V(nodep->rhsp()));
        స్థిరరాశి V3_సంఖ్య& అంకె = గమ్యం_అంకె_C(nodep, numv);
        UINFO(4, "BICONST -> " << అంకె);
        VL_DO_DANGLING(replace_అంకె(nodep, అంకె), nodep);
    }
    శూన్య replaceConst(Ast_కణుపు_Triop* nodep) {
        V3_సంఖ్య numv{nodep, nodep->వెడల్పు_Min_V()};
        nodep->సంఖ్య_నిర్వహించు(numv, const_అంకె_V(nodep->lhsp()), const_అంకె_V(nodep->rhsp()),
                             const_అంకె_V(nodep->thsp()));
        స్థిరరాశి V3_సంఖ్య& అంకె = గమ్యం_అంకె_C(nodep, numv);
        UINFO(4, "TRICONST -> " << అంకె);
        VL_DO_DANGLING(replace_అంకె(nodep, అంకె), nodep);
    }
    శూన్య replaceConst(Ast_కణుపు_Quadop* nodep) {
        V3_సంఖ్య numv{nodep, nodep->వెడల్పు_Min_V()};
        nodep->సంఖ్య_నిర్వహించు(numv, const_అంకె_V(nodep->lhsp()), const_అంకె_V(nodep->rhsp()),
                             const_అంకె_V(nodep->thsp()), const_అంకె_V(nodep->fhsp()));
        స్థిరరాశి V3_సంఖ్య& అంకె = గమ్యం_అంకె_C(nodep, numv);
        UINFO(4, "QUADCONST -> " << అంకె);
        VL_DO_DANGLING(replace_అంకె(nodep, అంకె), nodep);
    }

    శూన్య replaceConstString(Ast_కణుపు* oldp, స్థిరరాశి string& అంకె) {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        Ast_కణుపు* స్థిరరాశి newp = కొత్త AstConst{oldp->fileline(), AstConst::String{}, అంకె};
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        oldp->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(oldp), oldp);
    }
    //----------------------------------------
    // Replacement functions.
    // These all take a node and replace it with something else

    శూన్య replaceWChild(Ast_కణుపు* nodep, Ast_కణుపు_సమాసం* childp) {
        // NODE(..., CHILD(...)) -> CHILD(...)
        childp->unlink_Fr_వెనుక_With_తదుపరి();
        // If replacing a SEL for example, the data type comes from the parent (is less wide).
        // This may adversely affect the operation of the node being replaced.
        nodep->replaceWithKeepDType(childp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య replaceWChildBool(Ast_కణుపు* nodep, Ast_కణుపు_సమాసం* childp) {
        // NODE(..., CHILD(...)) -> REDOR(CHILD(...))
        childp->unlink_Fr_వెనుక();
        ఒకవేళ (childp->width1()) {
            nodep->replaceWithKeepDType(childp);
        } లేకపోతే {
            nodep->replaceWithKeepDType(కొత్త AstRedOr{childp->fileline(), childp});
        }
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    //! Replace a ternary node with its RHS after iterating
    //! Used with short-circuiting, where the RHS has not yet been iterated.
    శూన్య replaceWIteratedRhs(Ast_కణుపు_Triop* nodep) {
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి rhsp = nodep->rhsp()) iterate_And_తదుపరి_Null(rhsp);
        replaceWChild(nodep, nodep->rhsp());  // May have changed
    }

    //! Replace a ternary node with its THS after iterating
    //! Used with short-circuiting, where the THS has not yet been iterated.
    శూన్య replaceWIteratedThs(Ast_కణుపు_Triop* nodep) {
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి thsp = nodep->thsp()) iterate_And_తదుపరి_Null(thsp);
        replaceWChild(nodep, nodep->thsp());  // May have changed
    }
    శూన్య replaceWLhs(Ast_కణుపు_Uniop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    శూన్య replaceWLhs(Ast_కణుపు_Biop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    శూన్య replaceWRhs(Ast_కణుపు_Biop* nodep) {
        // Keep RHS, remove LHS
        replaceWChild(nodep, nodep->rhsp());
    }
    శూన్య replaceWLhsBool(Ast_కణుపు_Biop* nodep) { replaceWChildBool(nodep, nodep->lhsp()); }
    శూన్య replaceWRhsBool(Ast_కణుపు_Biop* nodep) { replaceWChildBool(nodep, nodep->rhsp()); }
    శూన్య replaceAsv(Ast_కణుపు_Biop* nodep) {
        // BIASV(CONSTa, BIASV(CONSTb, c)) -> BIASV( BIASV_CONSTED(a,b), c)
        // BIASV(SAMEa,  BIASV(SAMEb, c))  -> BIASV( BIASV(SAMEa,SAMEb), c)
        // UINFOTREE(1, nodep, "", "repAsvConst_old");
        Ast_కణుపు_సమాసం* స్థిరరాశి ap = nodep->lhsp();
        Ast_కణుపు_Biop* స్థిరరాశి rp = VN_AS(nodep->rhsp(), కణుపు_Biop);
        Ast_కణుపు_సమాసం* స్థిరరాశి bp = rp->lhsp();
        Ast_కణుపు_సమాసం* స్థిరరాశి cp = rp->rhsp();
        ap->unlink_Fr_వెనుక();
        bp->unlink_Fr_వెనుక();
        cp->unlink_Fr_వెనుక();
        rp->unlink_Fr_వెనుక();
        nodep->lhsp(rp);
        nodep->rhsp(cp);
        rp->lhsp(ap);
        rp->rhsp(bp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        ఒకవేళ (VN_IS(rp->lhsp(), Const) && VN_IS(rp->rhsp(), Const)) replaceConst(rp);
        iterate(nodep);  // Proceed to fixed point
    }
    శూన్య replaceAsvLUp(Ast_కణుపు_Biop* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r))
        Ast_కణుపు_Biop* స్థిరరాశి lp = VN_AS(nodep->lhsp()->unlink_Fr_వెనుక(), కణుపు_Biop);
        Ast_కణుపు_సమాసం* స్థిరరాశి llp = lp->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి lrp = lp->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి rp = nodep->rhsp()->unlink_Fr_వెనుక();
        nodep->lhsp(llp);
        nodep->rhsp(lp);
        lp->lhsp(lrp);
        lp->rhsp(rp);
        lp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    శూన్య replaceAsvRUp(Ast_కణుపు_Biop* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr))
        Ast_కణుపు_సమాసం* స్థిరరాశి lp = nodep->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_Biop* స్థిరరాశి rp = VN_AS(nodep->rhsp()->unlink_Fr_వెనుక(), కణుపు_Biop);
        Ast_కణుపు_సమాసం* స్థిరరాశి rlp = rp->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి rrp = rp->rhsp()->unlink_Fr_వెనుక();
        nodep->lhsp(rlp);
        nodep->rhsp(rp);
        rp->lhsp(lp);
        rp->rhsp(rrp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    శూన్య replaceAndOr(Ast_కణుపు_Biop* nodep) {
        //  OR  (AND (CONSTll,lr), AND(CONSTrl==ll,rr))    -> AND (CONSTll, OR(lr,rr))
        //  OR  (AND (CONSTll,lr), AND(CONSTrl,    rr=lr)) -> AND (OR(ll,rl), rr)
        // nodep ^lp  ^llp   ^lrp  ^rp  ^rlp       ^rrp
        // (Or/And may also be reversed)
        Ast_కణుపు_Biop* స్థిరరాశి lp = VN_AS(nodep->lhsp()->unlink_Fr_వెనుక(), కణుపు_Biop);
        Ast_కణుపు_సమాసం* స్థిరరాశి llp = lp->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి lrp = lp->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_Biop* స్థిరరాశి rp = VN_AS(nodep->rhsp()->unlink_Fr_వెనుక(), కణుపు_Biop);
        Ast_కణుపు_సమాసం* స్థిరరాశి rlp = rp->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి rrp = rp->rhsp()->unlink_Fr_వెనుక();
        nodep->replaceWithKeepDType(lp);
        ఒకవేళ (operandsSame(llp, rlp)) {
            lp->lhsp(llp);
            lp->rhsp(nodep);
            lp->dtypeFrom(nodep);
            nodep->lhsp(lrp);
            nodep->rhsp(rrp);
            VL_DO_DANGLING(నెట్టు_Deletep(rp), rp);
            VL_DO_DANGLING(నెట్టు_Deletep(rlp), rlp);
        } లేకపోతే ఒకవేళ (operandsSame(lrp, rrp)) {
            lp->lhsp(nodep);
            lp->rhsp(rrp);
            lp->dtypeFrom(nodep);
            nodep->lhsp(llp);
            nodep->rhsp(rlp);
            VL_DO_DANGLING(నెట్టు_Deletep(rp), rp);
            VL_DO_DANGLING(నెట్టు_Deletep(lrp), lrp);
        } లేకపోతే {
            nodep->v3fatalSrc("replaceAndOr on something operandAndOrSame shouldn't have matched");
        }
        // UINFOTREE(1, nodep, "", "repAndOr_new");
    }
    శూన్య replace_జరుపు_Same(Ast_కణుపు_Biop* nodep) {
        // Or(Shift(ll,CONSTlr),Shift(rl,CONSTrr==lr)) -> Shift(Or(ll,rl),CONSTlr)
        // (Or/And may also be reversed)
        Ast_కణుపు_Biop* స్థిరరాశి lp = VN_AS(nodep->lhsp()->unlink_Fr_వెనుక(), కణుపు_Biop);
        Ast_కణుపు_సమాసం* స్థిరరాశి llp = lp->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి lrp = lp->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_Biop* స్థిరరాశి rp = VN_AS(nodep->rhsp()->unlink_Fr_వెనుక(), కణుపు_Biop);
        Ast_కణుపు_సమాసం* స్థిరరాశి rlp = rp->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి rrp = rp->rhsp()->unlink_Fr_వెనుక();
        nodep->replaceWithKeepDType(lp);
        lp->lhsp(nodep);
        lp->rhsp(lrp);
        nodep->lhsp(llp);
        nodep->rhsp(rlp);
        nodep->dtypep(llp->dtypep());  // dtype of Biop is before shift.
        VL_DO_DANGLING(నెట్టు_Deletep(rp), rp);
        VL_DO_DANGLING(నెట్టు_Deletep(rrp), rrp);
        // UINFOTREE(1, nodep, "", "repShiftSame_new");
    }
    శూన్య replaceConcatSel(AstConcat* nodep) {
        // {a[1], a[0]} -> a[1:0]
        AstSel* స్థిరరాశి lselp = VN_AS(nodep->lhsp()->unlink_Fr_వెనుక(), Sel);
        AstSel* స్థిరరాశి rselp = VN_AS(nodep->rhsp()->unlink_Fr_వెనుక(), Sel);
        స్థిరరాశి పూర్ణాంకం lstart = lselp->lsbConst();
        స్థిరరాశి పూర్ణాంకం lwidth = lselp->వెడల్పు_Const();
        స్థిరరాశి పూర్ణాంకం rstart = rselp->lsbConst();
        స్థిరరాశి పూర్ణాంకం rwidth = rselp->వెడల్పు_Const();

        UASSERT_OBJ((rstart + rwidth) == lstart, nodep,
                    "tried to merge two selects which are not adjacent");
        AstSel* స్థిరరాశి newselp = కొత్త AstSel{
            lselp->fromp()->fileline(), rselp->fromp()->unlink_Fr_వెనుక(), rstart, lwidth + rwidth};
        UINFO(5, "merged two adjacent sel " << lselp << " and " << rselp << " to one " << newselp);

        nodep->replaceWithKeepDType(newselp);
        VL_DO_DANGLING(నెట్టు_Deletep(lselp), lselp);
        VL_DO_DANGLING(నెట్టు_Deletep(rselp), rselp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య replace_Concat_విలీనం(AstConcat* nodep) {
        // {llp OP lrp, rlp OP rrp} => {llp, rlp} OP {lrp, rrp}, where OP = AND/OR/XOR
        Ast_కణుపు_Biop* స్థిరరాశి lp = VN_AS(nodep->lhsp(), కణుపు_Biop);
        Ast_కణుపు_Biop* స్థిరరాశి rp = VN_AS(nodep->rhsp(), కణుపు_Biop);
        ఒకవేళ (concatMergeable(lp, rp, 0)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి llp = lp->lhsp();
            Ast_కణుపు_సమాసం* స్థిరరాశి lrp = lp->rhsp();
            Ast_కణుపు_సమాసం* స్థిరరాశి rlp = rp->lhsp();
            Ast_కణుపు_సమాసం* స్థిరరాశి rrp = rp->rhsp();
            AstConcat* స్థిరరాశి newlp = కొత్త AstConcat{rlp->fileline(), llp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                                   rlp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
            AstConcat* స్థిరరాశి newrp = కొత్త AstConcat{rrp->fileline(), lrp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                                   rrp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
            // use the lhs to replace the parent concat
            llp->replaceWith(newlp);
            VL_DO_DANGLING(నెట్టు_Deletep(llp), llp);
            lrp->replaceWith(newrp);
            VL_DO_DANGLING(నెట్టు_Deletep(lrp), lrp);
            lp->dtype_Chg_వెడల్పు_Signed(newlp->వెడల్పు(), newlp->వెడల్పు(), VSigning::UNSIGNED);
            UINFO(5, "merged " << nodep);
            ++m_stat_Concat_విలీనం;
            VL_DO_DANGLING(నెట్టు_Deletep(rp->unlink_Fr_వెనుక()), rp);
            nodep->replaceWithKeepDType(lp->unlink_Fr_వెనుక());
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            iterate(lp->lhsp());
            iterate(lp->rhsp());
        } లేకపోతే {
            nodep->v3fatalSrc("tried to merge two Concat which are not adjacent");
        }
    }
    శూన్య replaceExtend(Ast_కణుపు* nodep, Ast_కణుపు_సమాసం* arg0p) {
        // -> EXTEND(nodep)
        // like a AstExtend{$rhsp}, but we need to set the width correctly from base node
        arg0p->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి newp
            = (VN_IS(nodep, ExtendS)
                   ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstExtendS{nodep->fileline(), arg0p})
                   : static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstExtend{nodep->fileline(), arg0p}));
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య replace_Pow_జరుపు(Ast_కణుపు_Biop* nodep) {  // Pow or PowS
        UINFO(5, "POW(2,b)->SHIFTL(1,b) " << nodep);
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
        Ast_జరుపు_L* స్థిరరాశి newp
            = కొత్త Ast_జరుపు_L{nodep->fileline(), కొత్త AstConst{nodep->fileline(), 1}, rhsp};
        newp->lhsp()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య replace_Mul_జరుపు(AstMul* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "MUL(2^n,b)->SHIFTL(b,n) " << nodep);
        స్థిరరాశి పూర్ణాంకం amount = VN_AS(nodep->lhsp(), Const)->అంకె().most_సమితి_బిట్టు_P1() - 1;  // 2^n->n+1
        Ast_కణుపు_సమాసం* స్థిరరాశి opp = nodep->rhsp()->unlink_Fr_వెనుక();
        Ast_జరుపు_L* స్థిరరాశి newp
            = కొత్త Ast_జరుపు_L{nodep->fileline(), opp, కొత్త AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య replace_Div_జరుపు(AstDiv* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "DIV(b,2^n)->SHIFTR(b,n) " << nodep);
        స్థిరరాశి పూర్ణాంకం amount = VN_AS(nodep->rhsp(), Const)->అంకె().most_సమితి_బిట్టు_P1() - 1;  // 2^n->n+1
        Ast_కణుపు_సమాసం* స్థిరరాశి opp = nodep->lhsp()->unlink_Fr_వెనుక();
        Ast_జరుపు_R* స్థిరరాశి newp
            = కొత్త Ast_జరుపు_R{nodep->fileline(), opp, కొత్త AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య replaceModAnd(AstModDiv* nodep) {  // Mod, but not ModS as not simple shift
        UINFO(5, "MOD(b,2^n)->AND(b,2^n-1) " << nodep);
        స్థిరరాశి పూర్ణాంకం amount = VN_AS(nodep->rhsp(), Const)->అంకె().most_సమితి_బిట్టు_P1() - 1;  // 2^n->n+1
        V3_సంఖ్య ముసుగు{nodep, nodep->వెడల్పు()};
        ముసుగు.సమితి_ముసుగు(amount);
        Ast_కణుపు_సమాసం* స్థిరరాశి opp = nodep->lhsp()->unlink_Fr_వెనుక();
        AstAnd* స్థిరరాశి newp
            = కొత్త AstAnd{nodep->fileline(), opp, కొత్త AstConst{nodep->fileline(), ముసుగు}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య replace_జరుపు_Op(Ast_కణుపు_Biop* nodep) {
        UINFO(5, "SHIFT(AND(a,b),CONST) with a/b special ->AND(SHIFT(a,CONST),SHIFT(b,CONST)) "
                     << nodep);
        స్థిరరాశి పూర్ణాంకం వెడల్పు = nodep->వెడల్పు();
        స్థిరరాశి పూర్ణాంకం వెడల్పు_Min = nodep->వెడల్పు_Min();
        VNRelinker handle;
        nodep->unlink_Fr_వెనుక(&handle);
        Ast_కణుపు_Biop* స్థిరరాశి lhsp = VN_AS(nodep->lhsp(), కణుపు_Biop);
        lhsp->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి shiftp = nodep->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి ap = lhsp->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి bp = lhsp->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_Biop* స్థిరరాశి shift1p = nodep;
        Ast_కణుపు_Biop* స్థిరరాశి shift2p = nodep->clone_వృక్షం(నిజం);
        shift1p->lhsp(ap);
        shift1p->rhsp(shiftp->clone_వృక్షం_శుద్ధ(నిజం));
        shift2p->lhsp(bp);
        shift2p->rhsp(shiftp);
        Ast_కణుపు_Biop* స్థిరరాశి newp = lhsp;
        newp->lhsp(shift1p);
        newp->rhsp(shift2p);
        newp->dtype_Chg_వెడల్పు(వెడల్పు, వెడల్పు_Min);  // The new AND must have width of the original SHIFT
        handle.relink(newp);
        iterate(newp);  // Further reduce, either node may have more reductions.
    }
    శూన్య replace_జరుపు_జరుపు(Ast_కణుపు_Biop* nodep) {
        UINFO(4, "SHIFT(SHIFT(a,s1),s2)->SHIFT(a,ADD(s1,s2)) " << nodep);
        UINFOTREE(9, nodep, "", "repShiftShift_old");
        Ast_కణుపు_Biop* స్థిరరాశి lhsp = VN_AS(nodep->lhsp(), కణుపు_Biop);
        lhsp->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి ap = lhsp->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి shift1p = lhsp->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి shift2p = nodep->rhsp()->unlink_Fr_వెనుక();
        // Shift1p and shift2p may have different sizes, both are
        // self-determined so sum with infinite width
        ఒకవేళ (nodep->type() == lhsp->type()) {
            స్థిరరాశి పూర్ణాంకం shift1 = VN_AS(shift1p, Const)->గమ్యం_UInt();
            స్థిరరాశి పూర్ణాంకం shift2 = VN_AS(shift2p, Const)->గమ్యం_UInt();
            స్థిరరాశి పూర్ణాంకం newshift = shift1 + shift2;
            VL_DO_DANGLING(నెట్టు_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(నెట్టు_Deletep(shift2p), shift2p);
            nodep->lhsp(ap);
            nodep->rhsp(కొత్త AstConst(nodep->fileline(), newshift));
            iterate(nodep);  // Further reduce, either node may have more reductions.
        } లేకపోతే {
            // We know shift amounts are constant, but might be a mixed left/right shift
            పూర్ణాంకం shift1 = VN_AS(shift1p, Const)->గమ్యం_UInt();
            ఒకవేళ (VN_IS(lhsp, జరుపు_R)) shift1 = -shift1;
            పూర్ణాంకం shift2 = VN_AS(shift2p, Const)->గమ్యం_UInt();
            ఒకవేళ (VN_IS(nodep, జరుపు_R)) shift2 = -shift2;
            స్థిరరాశి పూర్ణాంకం newshift = shift1 + shift2;
            VL_DO_DANGLING(నెట్టు_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(నెట్టు_Deletep(shift2p), shift2p);
            Ast_కణుపు_సమాసం* newp;
            V3_సంఖ్య mask1{nodep, nodep->వెడల్పు()};
            V3_సంఖ్య ones{nodep, nodep->వెడల్పు()};
            ones.సమితి_ముసుగు(nodep->వెడల్పు());
            ఒకవేళ (shift1 < 0) {
                mask1.op_జరుపు_R(ones, V3_సంఖ్య(nodep, VL_IDATASIZE, -shift1));
            } లేకపోతే {
                mask1.op_జరుపు_L(ones, V3_సంఖ్య(nodep, VL_IDATASIZE, shift1));
            }
            V3_సంఖ్య ముసుగు{nodep, nodep->వెడల్పు()};
            ఒకవేళ (shift2 < 0) {
                ముసుగు.op_జరుపు_R(mask1, V3_సంఖ్య(nodep, VL_IDATASIZE, -shift2));
            } లేకపోతే {
                ముసుగు.op_జరుపు_L(mask1, V3_సంఖ్య(nodep, VL_IDATASIZE, shift2));
            }
            ఒకవేళ (newshift < 0) {
                newp = కొత్త Ast_జరుపు_R{nodep->fileline(), ap,
                                     కొత్త AstConst(nodep->fileline(), -newshift)};
            } లేకపోతే {
                newp = కొత్త Ast_జరుపు_L{nodep->fileline(), ap,
                                     కొత్త AstConst(nodep->fileline(), newshift)};
            }
            newp->dtypeFrom(nodep);
            newp = కొత్త AstAnd{nodep->fileline(), newp, కొత్త AstConst{nodep->fileline(), ముసుగు}};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            // UINFOTREE(1, newp, "", "repShiftShift_new");
            iterate(newp);  // Further reduce, either node may have more reductions.
        }
        VL_DO_DANGLING(నెట్టు_Deletep(lhsp), lhsp);
    }

    తార్కిక replace_కేటాయించు_Multi_Sel(Ast_కణుపు_కేటాయించు* nodep) {
        // Multiple assignments to sequential bits can be concated
        // ASSIGN(SEL(a),aq), ASSIGN(SEL(a+1),bq) -> ASSIGN(SEL(a:b),CONCAT(aq,bq)
        // ie. assign var[2]=a, assign var[3]=b -> assign var[3:2]={b,a}

        // Skip if we're not const'ing an entire module (IE doing only one assign, etc)
        ఒకవేళ (!m_modp) తిరిగి అబద్ధం;
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* స్థిరరాశి sel1p = VN_CAST(nodep->lhsp(), Sel);
        ఒకవేళ (!sel1p) తిరిగి అబద్ధం;
        Ast_కణుపు_కేటాయించు* స్థిరరాశి nextp = VN_CAST(nodep->nextp(), కణుపు_కేటాయించు);
        ఒకవేళ (!nextp) తిరిగి అబద్ధం;
        ఒకవేళ (nodep->type() != nextp->type()) తిరిగి అబద్ధం;
        స్థిరరాశి AstSel* స్థిరరాశి sel2p = VN_CAST(nextp->lhsp(), Sel);
        ఒకవేళ (!sel2p) తిరిగి అబద్ధం;
        AstVarRef* స్థిరరాశి varref1p = VN_CAST(sel1p->fromp(), VarRef);
        ఒకవేళ (!varref1p) తిరిగి అబద్ధం;
        స్థిరరాశి AstVarRef* స్థిరరాశి varref2p = VN_CAST(sel2p->fromp(), VarRef);
        ఒకవేళ (!varref2p) తిరిగి అబద్ధం;
        ఒకవేళ (!varref1p->same_Gate_వృక్షం(varref2p)) తిరిగి అబద్ధం;
        స్థిరరాశి AstConst* స్థిరరాశి con1p = VN_CAST(sel1p->lsbp(), Const);
        ఒకవేళ (!con1p) తిరిగి అబద్ధం;
        స్థిరరాశి AstConst* స్థిరరాశి con2p = VN_CAST(sel2p->lsbp(), Const);
        ఒకవేళ (!con2p) తిరిగి అబద్ధం;
        // We need to make sure there's no self-references involved in either
        // assignment.  For speed, we only look 3 deep, then give up.
        ఒకవేళ (!varNotReferenced(nodep->rhsp(), varref1p->varp())) తిరిగి అబద్ధం;
        ఒకవేళ (!varNotReferenced(nextp->rhsp(), varref2p->varp())) తిరిగి అబద్ధం;
        // If a variable is marked split_var, access to the variable should not be merged.
        ఒకవేళ (varref1p->varp()->attr_విభజించు_Var() || varref2p->varp()->attr_విభజించు_Var()) తిరిగి అబద్ధం;
        // Swap?
        ఒకవేళ ((con1p->గమ్యం_SInt() != con2p->గమ్యం_SInt() + sel2p->వెడల్పు())
            && (con2p->గమ్యం_SInt() != con1p->గమ్యం_SInt() + sel1p->వెడల్పు())) {
            తిరిగి అబద్ధం;
        }
        స్థిరరాశి తార్కిక lsb_మొదటి_కేటాయించు = (con1p->గమ్యం_UInt() < con2p->గమ్యం_UInt());
        UINFO(4, "replaceAssignMultiSel " << nodep);
        UINFO(4, "                   && " << nextp);
        // UINFOTREE(1, nodep, "", "comb1");
        // UINFOTREE(1, nextp, "", "comb2");
        Ast_కణుపు_సమాసం* స్థిరరాశి rhs1p = nodep->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhs2p = nextp->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_కేటాయించు* newp;
        ఒకవేళ (lsb_మొదటి_కేటాయించు) {
            newp = nodep->cloneType(కొత్త AstSel{sel1p->fileline(), varref1p->unlink_Fr_వెనుక(),
                                               sel1p->lsbConst(), sel1p->వెడల్పు() + sel2p->వెడల్పు()},
                                    కొత్త AstConcat{rhs1p->fileline(), rhs2p, rhs1p});
        } లేకపోతే {
            newp = nodep->cloneType(కొత్త AstSel{sel1p->fileline(), varref1p->unlink_Fr_వెనుక(),
                                               sel2p->lsbConst(), sel1p->వెడల్పు() + sel2p->వెడల్పు()},
                                    కొత్త AstConcat{rhs1p->fileline(), rhs1p, rhs2p});
        }
        // UINFOTREE(1, pnewp, "", "conew");
        nodep->replaceWith(newp);  // dypep intentionally changing
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        VL_DO_DANGLING(నెట్టు_Deletep(nextp->unlink_Fr_వెనుక()), nextp);
        తిరిగి నిజం;
    }

    తార్కిక replace_కేటాయించు_SFormat_F(Ast_కణుపు_కేటాయించు* nodep) {
        // Rewrite 'x = sformatf(...)' into 'sformat(x, ...)', which is more efficient
        // at the call site as it does not need to check if 'x' needs to be freed first.
        // This is a somewhat common pattern after lowering assertions.
        ఒకవేళ (!VN_IS(nodep, కేటాయించు)) తిరిగి అబద్ధం;
        AstVarRef* స్థిరరాశి lhsp = VN_CAST(nodep->lhsp(), VarRef);
        ఒకవేళ (!lhsp) తిరిగి అబద్ధం;
        AstSFormatF* స్థిరరాశి fmtp = VN_CAST(nodep->rhsp(), SFormatF);
        ఒకవేళ (!fmtp) తిరిగి అబద్ధం;
        lhsp->unlink_Fr_వెనుక();
        fmtp->unlink_Fr_వెనుక();
        nodep->replaceWith(కొత్త AstSFormat{nodep->fileline(), fmtp, lhsp});
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        తిరిగి నిజం;
    }

    తార్కిక varNotReferenced(Ast_కణుపు* nodep, AstVar* varp, పూర్ణాంకం level = 0) {
        // Return true if varp never referenced under node.
        // Return false if referenced, or tree too deep to be worth it, or side effects
        ఒకవేళ (!nodep) తిరిగి నిజం;
        ఒకవేళ (level > 2) తిరిగి అబద్ధం;
        ఒకవేళ (!nodep->is_శుద్ధ()) తిరిగి అబద్ధం;  // For example a $fgetc can't be reordered
        ఒకవేళ (VN_IS(nodep, కణుపు_Var_Ref) && VN_AS(nodep, కణుపు_Var_Ref)->varp() == varp) తిరిగి అబద్ధం;
        తిరిగి (varNotReferenced(nodep->nextp(), varp, level + 1)
                && varNotReferenced(nodep->op1p(), varp, level + 1)
                && varNotReferenced(nodep->op2p(), varp, level + 1)
                && varNotReferenced(nodep->op3p(), varp, level + 1)
                && varNotReferenced(nodep->op4p(), varp, level + 1));
    }

    తార్కిక replace_కణుపు_కేటాయించు(Ast_కణుపు_కేటాయించు* nodep) {
        ఒకవేళ (VN_IS(nodep->lhsp(), VarRef) && VN_IS(nodep->rhsp(), VarRef)
            && VN_AS(nodep->lhsp(), VarRef)->sameNoLvalue(VN_AS(nodep->rhsp(), VarRef))
            && !VN_IS(nodep, కేటాయించు_Dly)) {
            // X = X.  Quite pointless, though X <= X may override another earlier assignment
            ఒకవేళ (VN_IS(nodep, కేటాయించు_W)) {
                ఒకవేళ (!v3Global.opt.pedantic())
                    nodep->v3error(
                        "Wire inputs its own output, creating circular logic (wire x=x)");
                తిరిగి అబద్ధం;  // Don't delete the assign, or V3Gate will freak out
            } లేకపోతే {
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                తిరిగి నిజం;
            }
        } లేకపోతే ఒకవేళ (m_doV && VN_IS(nodep->lhsp(), Concat)) {
            తార్కిక need_తాత్కాలిక = అబద్ధం;
            // A blocking assignment with an intra-assignment timing control evaluates its value,
            // then waits once (IEEE 1800-2023 9.4.5), here to assign a temporary for the parts
            స్థిరరాశి తార్కిక waits = VN_IS(nodep, కేటాయించు) && nodep->timingControlp();
            // Splitting copies the RHS once per part, so a large RHS would make the tree
            // grow with the product of the number of parts and the RHS size
            స్థిరరాశి తార్కిక need_తాత్కాలిక_శుద్ధ = !nodep->rhsp()->is_శుద్ధ() || waits
                                      || nodep->rhsp()->isLargerThan(CONCAT_కేటాయించు_MAX_RHS_NODES);
            ఒకవేళ (m_warn && !VN_IS(nodep, కేటాయించు_Dly)
                && !need_తాత్కాలిక_శుద్ధ) {  // Is same var on LHS and RHS?
                // If the rhs is not pure, we need a temporary variable anyway
                std::unordered_సమితి<కొలత_t> lhsVarIds;
                nodep->lhsp()->foreach([&](స్థిరరాశి AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    lhsVarIds.emplace(m_ids(refp->varp()));
                });
                nodep->rhsp()->foreach([&](స్థిరరాశి AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    ఒకవేళ (lhsVarIds.లెక్క(m_ids(refp->varp()))) need_తాత్కాలిక = నిజం;
                });
            }
            ఒకవేళ (need_తాత్కాలిక_శుద్ధ) {
                // if the RHS is impure or large we need to create a temporary variable for it,
                // because further handling involves copying of the RHS.
                UINFO(4, "  ASSITEMPPURE " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp,rhs),
                //                                ASSIGN(lc1,SEL(temp,{size1})),
                //                                ASSIGN(lc2,SEL(temp,{size2}))

                Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
                AstVar* స్థిరరాశి తాత్కాలిక_Purep = కొత్త AstVar{rhsp->fileline(), VVarType::BLOCKTEMP,
                                                     m_concswap_పేర్లు.get(rhsp), rhsp->dtypep()};
                // Static, as read right after written, also in a fork
                తాత్కాలిక_Purep->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->కూడు_Stmtsp(తాత్కాలిక_Purep);
                AstVarRef* స్థిరరాశి తాత్కాలిక_శుద్ధ_Refp
                    = కొత్త AstVarRef{rhsp->fileline(), తాత్కాలిక_Purep, VAccess::WRITE};
                Ast_కణుపు_కేటాయించు* స్థిరరాశి asnp
                    = VN_IS(nodep, కేటాయించు_Dly)
                          ? కొత్త Ast_కేటాయించు{nodep->fileline(), తాత్కాలిక_శుద్ధ_Refp, rhsp}
                          : nodep->cloneType(తాత్కాలిక_శుద్ధ_Refp, rhsp);
                nodep->కూడు_ఇక్కడ_This_As_తదుపరి(asnp);
                nodep->rhsp(కొత్త AstVarRef{rhsp->fileline(), తాత్కాలిక_Purep, VAccess::READ});
                // Only the assignment of the temporary waits
                ఒకవేళ (waits) నెట్టు_Deletep(nodep->timingControlp()->unlink_Fr_వెనుక());
            } లేకపోతే ఒకవేళ (need_తాత్కాలిక) {
                // The first time we constify, there may be the same variable on the LHS
                // and RHS.  In that case, we must use temporaries, or {a,b}={b,a} will break.
                UINFO(4, "  ASSITEMP " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp1,SEL(rhs,{size})),
                //                                ASSIGN(temp2,SEL(newrhs,{size}))
                //                                ASSIGN(lc1,temp1),
                //                                ASSIGN(lc2,temp2)
            } లేకపోతే {
                UINFO(4, "  ASSI " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(lc1,SEL(rhs,{size})),
                //                                ASSIGN(lc2,SEL(newrhs,{size}))
            }
            // The NBA evaluates its intra-assignment delay as it executes (IEEE 1800-2023 4.9.4),
            // so once for the NBAs to all parts
            AstDelay* స్థిరరాశి delayp
                = VN_IS(nodep, కేటాయించు_Dly) ? VN_CAST(nodep->timingControlp(), Delay) : శూన్యసూచిక;
            ఒకవేళ (delayp && !delayp->lhsp()->is_శుద్ధ()) {
                File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
                Ast_కణుపు_సమాసం* స్థిరరాశి valuep = delayp->lhsp()->unlink_Fr_వెనుక();
                AstVar* స్థిరరాశి tempp = కొత్త AstVar{flp, VVarType::BLOCKTEMP,
                                                 m_concswap_పేర్లు.get(valuep), valuep->dtypep()};
                // Static, as read right after written, when the NBAs to the parts start waiting
                tempp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->కూడు_Stmtsp(tempp);
                nodep->కూడు_ఇక్కడ_This_As_తదుపరి(
                    కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, tempp, VAccess::WRITE}, valuep});
                delayp->lhsp(కొత్త AstVarRef{flp, tempp, VAccess::READ});
            }
            UINFOTREE(9, nodep, "", "Ass_old");
            // Unlink the stuff
            Ast_కణుపు_సమాసం* స్థిరరాశి lc1p = VN_AS(nodep->lhsp(), Concat)->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి lc2p = VN_AS(nodep->lhsp(), Concat)->rhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి conp = VN_AS(nodep->lhsp(), Concat)->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి rhs2p = rhsp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            // Calc widths
            స్థిరరాశి పూర్ణాంకం lsb2 = 0;
            స్థిరరాశి పూర్ణాంకం msb2 = lsb2 + lc2p->వెడల్పు() - 1;
            స్థిరరాశి పూర్ణాంకం lsb1 = msb2 + 1;
            స్థిరరాశి పూర్ణాంకం msb1 = lsb1 + lc1p->వెడల్పు() - 1;
            UASSERT_OBJ(msb1 == (conp->వెడల్పు() - 1), nodep, "Width calc mismatch");
            // Form ranges
            AstSel* స్థిరరాశి sel1p = కొత్త AstSel{conp->fileline(), rhsp, lsb1, msb1 - lsb1 + 1};
            AstSel* స్థిరరాశి sel2p = కొత్త AstSel{conp->fileline(), rhs2p, lsb2, msb2 - lsb2 + 1};
            // Make new assigns of same flavor as old one
            //*** Not cloneTree; just one node.
            Ast_కణుపు_కేటాయించు* newp = శూన్యసూచిక;
            ఒకవేళ (!need_తాత్కాలిక) {
                Ast_కణుపు_కేటాయించు* స్థిరరాశి asn1ap = nodep->cloneType(lc1p, sel1p);
                Ast_కణుపు_కేటాయించు* స్థిరరాశి asn2ap = nodep->cloneType(lc2p, sel2p);
                asn1ap->dtypeFrom(sel1p);
                asn2ap->dtypeFrom(sel2p);
                newp = Ast_కణుపు::కూడు_తదుపరి(newp, asn1ap);
                newp = Ast_కణుపు::కూడు_తదుపరి(newp, asn2ap);
            } లేకపోతే {
                UASSERT_OBJ(m_modp, nodep, "Not under module");
                UASSERT_OBJ(m_globalPass, nodep,
                            "Should not reach here when not invoked on whole AstNetlist");
                // We could create just one temp variable, but we'll get better optimization
                // if we make one per term.
                AstVar* స్థిరరాశి temp1p
                    = కొత్త AstVar{sel1p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswap_పేర్లు.get(sel1p), VFlagLogicPacked{}, msb1 - lsb1 + 1};
                AstVar* స్థిరరాశి temp2p
                    = కొత్త AstVar{sel2p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswap_పేర్లు.get(sel2p), VFlagLogicPacked{}, msb2 - lsb2 + 1};
                m_modp->కూడు_Stmtsp(temp1p);
                m_modp->కూడు_Stmtsp(temp2p);
                Ast_కణుపు_కేటాయించు* స్థిరరాశి asn1ap = nodep->cloneType(
                    కొత్త AstVarRef{sel1p->fileline(), temp1p, VAccess::WRITE}, sel1p);
                Ast_కణుపు_కేటాయించు* స్థిరరాశి asn2ap = nodep->cloneType(
                    కొత్త AstVarRef{sel2p->fileline(), temp2p, VAccess::WRITE}, sel2p);
                Ast_కణుపు_కేటాయించు* స్థిరరాశి asn1bp = nodep->cloneType(
                    lc1p, కొత్త AstVarRef{sel1p->fileline(), temp1p, VAccess::READ});
                Ast_కణుపు_కేటాయించు* స్థిరరాశి asn2bp = nodep->cloneType(
                    lc2p, కొత్త AstVarRef{sel2p->fileline(), temp2p, VAccess::READ});
                asn1ap->dtypeFrom(temp1p);
                asn1bp->dtypeFrom(temp1p);
                asn2ap->dtypeFrom(temp2p);
                asn2bp->dtypeFrom(temp2p);
                // This order matters
                newp = Ast_కణుపు::కూడు_తదుపరి(newp, asn1ap);
                newp = Ast_కణుపు::కూడు_తదుపరి(newp, asn2ap);
                newp = Ast_కణుపు::కూడు_తదుపరి(newp, asn1bp);
                newp = Ast_కణుపు::కూడు_తదుపరి(newp, asn2bp);
            }
            ఒకవేళ (debug() >= 9 && newp) newp->dump_వృక్షం_And_తదుపరి(బయటకు, "-     _new: ");
            nodep->కూడు_తదుపరి_ఇక్కడ(newp);
            // Cleanup
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            VL_DO_DANGLING(నెట్టు_Deletep(conp), conp);
            // Further reduce, either node may have more reductions.
            తిరిగి నిజం;
        } లేకపోతే ఒకవేళ (m_doV && VN_IS(nodep->rhsp(), Cvt_Packed_గమ్యం_Array)
                   && క్రింది_As_Fixed_Aggregate(nodep->lhsp()->dtypep())) {
            Ast_Cvt_Packed_గమ్యం_Array* స్థిరరాశి cvtp
                = VN_AS(nodep->rhsp(), Cvt_Packed_గమ్యం_Array)->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* srcp = cvtp->fromp()->unlink_Fr_వెనుక();
            ఒకవేళ (క్రింది_As_Fixed_Aggregate(srcp->dtypep())) {
                srcp = packFixedAggregate(srcp);
            } లేకపోతే ఒకవేళ (Ast_కణుపు_ప్రవాహం* స్థిరరాశి streamp = VN_CAST(srcp, కణుపు_ప్రవాహం)) {
                Ast_కణుపు_సమాసం* స్థిరరాశి ప్రవాహం_Srcp = streamp->lhsp();
                ఒకవేళ (క్రింది_As_Fixed_Aggregate(ప్రవాహం_Srcp->dtypep())) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి packedp = packFixedAggregate(ప్రవాహం_Srcp->unlink_Fr_వెనుక());
                    streamp->lhsp(packedp);
                    streamp->dtype_సమితి_బిట్టు_Or_Logic_Unsized(packedp->వెడల్పు(), packedp->వెడల్పు_Min(),
                                                       VSigning::UNSIGNED,
                                                       packedp->isFourstateOrNoDtype());
                }
            }
            VL_DO_DANGLING(నెట్టు_Deletep(cvtp), cvtp);
            replace_కేటాయించు_గమ్యం_Fixed_Aggregate(nodep, nodep->lhsp()->unlink_Fr_వెనుక(), srcp);
            తిరిగి నిజం;
        } లేకపోతే ఒకవేళ (m_doV && VN_IS(nodep->rhsp(), ప్రవాహం_R)
                   && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), వరుస_DType)) {
            // The right-streaming operator on rhs of assignment does not
            // change the order of bits. Eliminate stream but keep its lhsp.
            // Add a cast if needed.
            Ast_ప్రవాహం_R* స్థిరరాశి streamp = VN_AS(nodep->rhsp(), ప్రవాహం_R)->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* srcp = streamp->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_DType* స్థిరరాశి srcDTypep = srcp->dtypep()->skipRefp();
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            ఒకవేళ (క్రింది_As_Fixed_Aggregate(srcDTypep)) {
                srcp = packFixedAggregate(srcp);
            } లేకపోతే ఒకవేళ (VN_IS(srcDTypep, వరుస_DType) || VN_IS(srcDTypep, DynArrayDType)) {
                ఒకవేళ (VN_IS(dstDTypep, వరుస_DType) || VN_IS(dstDTypep, DynArrayDType)) {
                    పూర్ణాంకం src_Element_బిట్లు = 0;
                    ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి elemDtp = srcDTypep->subDTypep()) {
                        src_Element_బిట్లు = elemDtp->వెడల్పు();
                    }
                    పూర్ణాంకం dst_Element_బిట్లు = 0;
                    ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_బిట్లు = elemDtp->వెడల్పు();
                    }
                    srcp = కొత్త Ast_Cvt_Array_గమ్యం_Array{
                        srcp->fileline(), srcp,          nodep->dtypep(), అబద్ధం, 1,
                        dst_Element_బిట్లు,   src_Element_బిట్లు};
                } లేకపోతే {
                    srcp = కొత్త Ast_Cvt_Array_గమ్యం_Packed{srcp->fileline(), srcp, nodep->dtypep()};
                }
            } లేకపోతే ఒకవేళ (VN_IS(srcDTypep, UnpackArrayDType)) {
                srcp = కొత్త Ast_Cvt_Array_గమ్యం_Packed{srcp->fileline(), srcp, srcDTypep};
                // Handling the case where lhs is wider than rhs by inserting zeros. StreamL does
                // not require this, since the left streaming operator implicitly handles this.
                స్థిరరాశి పూర్ణాంకం packed_బిట్లు = nodep->lhsp()->వెడల్పు_Min();
                స్థిరరాశి పూర్ణాంకం unpack_బిట్లు
                    = srcDTypep->arrayUnpackedElements() * srcDTypep->subDTypep()->వెడల్పు_Min();
                స్థిరరాశి uint32_t స్థానభ్రంశం = packed_బిట్లు > unpack_బిట్లు ? packed_బిట్లు - unpack_బిట్లు : 0;
                srcp = కొత్త Ast_జరుపు_L{srcp->fileline(), srcp,
                                     కొత్త AstConst{srcp->fileline(), స్థానభ్రంశం}, packed_బిట్లు};
            }
            ఒకవేళ (!VN_IS(dstDTypep, UnpackArrayDType) && !VN_IS(dstDTypep, వరుస_DType)
                && !VN_IS(dstDTypep, DynArrayDType)) {
                స్థిరరాశి పూర్ణాంకం s_వెడల్పు = srcp->వెడల్పు();
                స్థిరరాశి పూర్ణాంకం d_వెడల్పు = nodep->lhsp()->వెడల్పు();
                ఒకవేళ (s_వెడల్పు < d_వెడల్పు) {
                    AstExtend* స్థిరరాశి extendp = కొత్త AstExtend{srcp->fileline(), srcp, d_వెడల్పు};
                    srcp = కొత్త Ast_జరుపు_L{
                        srcp->fileline(), extendp,
                        కొత్త AstConst{srcp->fileline(), static_cast<uint32_t>(d_వెడల్పు - s_వెడల్పు)},
                        d_వెడల్పు};
                }
            }
            nodep->rhsp(srcp);
            VL_DO_DANGLING(నెట్టు_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            తిరిగి నిజం;
        } లేకపోతే ఒకవేళ (m_doV && VN_IS(nodep->lhsp(), ప్రవాహం_L)) {
            // Push the stream operator to the rhs of the assignment statement
            Ast_కణుపు_సమాసం* streamp = nodep->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి dstp = VN_AS(streamp, ప్రవాహం_L)->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_DType* స్థిరరాశి dstDTypep = dstp->dtypep()->skipRefp();
            Ast_కణుపు_సమాసం* srcp = nodep->rhsp()->unlink_Fr_వెనుక();
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి srcDTypep = srcp->dtypep()->skipRefp();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamL reverses, so reverse=true)
            ఒకవేళ ((VN_IS(srcDTypep, UnpackArrayDType) || VN_IS(srcDTypep, వరుస_DType)
                 || VN_IS(srcDTypep, DynArrayDType))
                && (VN_IS(dstDTypep, వరుస_DType) || VN_IS(dstDTypep, DynArrayDType))) {
                పూర్ణాంకం ఖండం_కొలత = 1;
                ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp
                    = VN_CAST(VN_AS(streamp, ప్రవాహం_L)->rhsp(), Const)) {
                    ఖండం_కొలత = constp->గమ్యం_SInt();
                    ఒకవేళ (VL_UNLIKELY(ఖండం_కొలత <= 0)) ఖండం_కొలత = 1;
                }
                పూర్ణాంకం src_Element_బిట్లు = 0;
                ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి elemDtp = srcDTypep->subDTypep()) {
                    src_Element_బిట్లు = elemDtp->వెడల్పు();
                }
                పూర్ణాంకం dst_Element_బిట్లు = 0;
                ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి elemDtp = dstDTypep->subDTypep()) {
                    dst_Element_బిట్లు = elemDtp->వెడల్పు();
                }
                nodep->lhsp(dstp);
                nodep->rhsp(కొత్త Ast_Cvt_Array_గమ్యం_Array{srcp->fileline(), srcp, dstDTypep, నిజం,
                                                   ఖండం_కొలత, dst_Element_బిట్లు, src_Element_బిట్లు});
                nodep->dtypep(dstDTypep);
                VL_DO_DANGLING(నెట్టు_Deletep(streamp), streamp);
                తిరిగి నిజం;
            }
            ఒకవేళ (క్రింది_As_Fixed_Aggregate(srcDTypep)) srcp = packFixedAggregate(srcp);
            స్థిరరాశి పూర్ణాంకం s_వెడల్పు = srcp->వెడల్పు();
            స్థిరరాశి పూర్ణాంకం d_వెడల్పు = dstp->వెడల్పు();
            // Connect the rhs to the stream operator and update its width
            VN_AS(streamp, ప్రవాహం_L)->lhsp(srcp);
            ఒకవేళ (VN_IS(srcDTypep, DynArrayDType) || VN_IS(srcDTypep, వరుస_DType)
                || VN_IS(srcDTypep, UnpackArrayDType)) {
                streamp->dtype_సమితి_ప్రవాహం();
            } లేకపోతే {
                streamp->dtype_సమితి_బిట్టు_Or_Logic_Unsized(srcp->వెడల్పు(), srcp->వెడల్పు_Min(),
                                                   VSigning::UNSIGNED,
                                                   srcp->isFourstateOrNoDtype());
            }
            ఒకవేళ (VN_IS(dstDTypep, UnpackArrayDType)) {
                streamp = కొత్త Ast_Cvt_Packed_గమ్యం_Array{nodep->fileline(), streamp, dstDTypep};
            } లేకపోతే {
                ఒకవేళ (d_వెడల్పు == 0) {
                    streamp = కొత్త Ast_Cvt_Packed_గమ్యం_Array{nodep->fileline(), streamp, dstDTypep};
                } లేకపోతే ఒకవేళ (s_వెడల్పు >= d_వెడల్పు) {
                    streamp = కొత్త AstSel{streamp->fileline(), streamp, s_వెడల్పు - d_వెడల్పు, d_వెడల్పు};
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(streamp);
            nodep->dtypep(dstDTypep);
            తిరిగి నిజం;
        } లేకపోతే ఒకవేళ (m_doV && VN_IS(nodep->lhsp(), ప్రవాహం_R)) {
            // The right stream operator on lhs of assignment statement does
            // not reorder bits. However, if the rhs is wider than the lhs,
            // then we select bits from the left-most, not the right-most.
            Ast_కణుపు_సమాసం* స్థిరరాశి streamp = nodep->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి dstp = VN_AS(streamp, ప్రవాహం_R)->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_DType* స్థిరరాశి dstDTypep = dstp->dtypep()->skipRefp();
            Ast_కణుపు_సమాసం* srcp = nodep->rhsp()->unlink_Fr_వెనుక();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamR does not reverse, so reverse=false).
            // V3Width may have wrapped the source in CvtArrayToPacked; unwrap it.
            ఒకవేళ (VN_IS(dstDTypep, వరుస_DType) || VN_IS(dstDTypep, DynArrayDType)) {
                Ast_కణుపు_సమాసం* origSrcp = srcp;
                ఒకవేళ (Ast_Cvt_Array_గమ్యం_Packed* స్థిరరాశి cvtp = VN_CAST(srcp, Cvt_Array_గమ్యం_Packed)) {
                    origSrcp = cvtp->fromp();
                }
                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి origSrcDTypep = origSrcp->dtypep()->skipRefp();
                ఒకవేళ (VN_IS(origSrcDTypep, UnpackArrayDType) || VN_IS(origSrcDTypep, వరుస_DType)
                    || VN_IS(origSrcDTypep, DynArrayDType)) {
                    పూర్ణాంకం src_Element_బిట్లు = 0;
                    ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి elemDtp = origSrcDTypep->subDTypep()) {
                        src_Element_బిట్లు = elemDtp->వెడల్పు();
                    }
                    పూర్ణాంకం dst_Element_బిట్లు = 0;
                    ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_బిట్లు = elemDtp->వెడల్పు();
                    }
                    ఒకవేళ (VN_IS(srcp, Cvt_Array_గమ్యం_Packed)) {
                        origSrcp = VN_AS(srcp, Cvt_Array_గమ్యం_Packed)->fromp()->unlink_Fr_వెనుక();
                        VL_DO_DANGLING(నెట్టు_Deletep(srcp), srcp);
                        srcp = origSrcp;
                    }
                    // Descending unpacked arrays need element reversal
                    తార్కిక తిరగేయి = అబద్ధం;
                    ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి unpackDtp
                        = VN_CAST(origSrcDTypep, UnpackArrayDType)) {
                        తిరగేయి = !unpackDtp->declRange().ascending();
                    }
                    nodep->lhsp(dstp);
                    nodep->rhsp(కొత్త Ast_Cvt_Array_గమ్యం_Array{srcp->fileline(), srcp, dstDTypep, తిరగేయి,
                                                       1, dst_Element_బిట్లు, src_Element_బిట్లు});
                    nodep->dtypep(dstDTypep);
                    VL_DO_DANGLING(నెట్టు_Deletep(streamp), streamp);
                    తిరిగి నిజం;
                }
            }
            స్థిరరాశి పూర్ణాంకం s_వెడల్పు = srcp->వెడల్పు();
            స్థిరరాశి పూర్ణాంకం d_వెడల్పు = dstp->వెడల్పు();
            ఒకవేళ (VN_IS(dstDTypep, UnpackArrayDType)) {
                స్థిరరాశి పూర్ణాంకం dst_బిట్టు_వెడల్పు
                    = d_వెడల్పు * VN_AS(dstDTypep, UnpackArrayDType)->arrayUnpackedElements();
                // Handling the case where rhs is wider than lhs. StreamL does not require this
                // since the combination of the left streaming operation and the implicit
                // truncation in VL_ASSIGN_UNPACK automatically selects the left-most bits.
                ఒకవేళ (s_వెడల్పు > dst_బిట్టు_వెడల్పు) {
                    srcp
                        = కొత్త AstSel{streamp->fileline(), srcp, s_వెడల్పు - dst_బిట్టు_వెడల్పు, dst_బిట్టు_వెడల్పు};
                }
                srcp = కొత్త Ast_Cvt_Packed_గమ్యం_Array{nodep->fileline(), srcp, dstDTypep};
            } లేకపోతే {
                ఒకవేళ (d_వెడల్పు == 0) {
                    srcp = కొత్త Ast_Cvt_Packed_గమ్యం_Array{nodep->fileline(), srcp, dstDTypep};
                } లేకపోతే ఒకవేళ (s_వెడల్పు >= d_వెడల్పు) {
                    srcp = కొత్త AstSel{streamp->fileline(), srcp, s_వెడల్పు - d_వెడల్పు, d_వెడల్పు};
                } లేకపోతే {
                    // Source narrower than destination: left-justify by shifting left.
                    // The right stream operator packs left-to-right, so remaining
                    // LSBs are zero-filled (IEEE 1800-2023 11.4.14.3).
                    ఒకవేళ (!VN_IS(srcp->dtypep()->skipRefp(), వరుస_DType)) {
                        AstExtend* స్థిరరాశి extendp = కొత్త AstExtend{srcp->fileline(), srcp, d_వెడల్పు};
                        srcp = కొత్త Ast_జరుపు_L{
                            srcp->fileline(), extendp,
                            కొత్త AstConst{srcp->fileline(), static_cast<uint32_t>(d_వెడల్పు - s_వెడల్పు)},
                            d_వెడల్పు};
                    } లేకపోతే {  // if it is a queue we dont need do do a shiftL
                        srcp = కొత్త AstExtend{srcp->fileline(), srcp};
                    }
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(srcp);
            nodep->dtypep(dstDTypep);
            VL_DO_DANGLING(నెట్టు_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            తిరిగి నిజం;
        } లేకపోతే ఒకవేళ (m_doV && VN_IS(nodep->rhsp(), ప్రవాహం_L)) {
            Ast_ప్రవాహం_L* streamp = VN_AS(nodep->rhsp(), ప్రవాహం_L);
            Ast_కణుపు_సమాసం* srcp = streamp->lhsp();
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి srcDTypep = srcp->dtypep()->skipRefp();
            Ast_కణుపు_DType* స్థిరరాశి dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            ఒకవేళ (క్రింది_As_Fixed_Aggregate(srcDTypep)) {
                Ast_కణుపు_సమాసం* స్థిరరాశి packedp = packFixedAggregate(srcp->unlink_Fr_వెనుక());
                streamp->lhsp(packedp);
                streamp->dtype_సమితి_బిట్టు_Or_Logic_Unsized(packedp->వెడల్పు(), packedp->వెడల్పు_Min(),
                                                   VSigning::UNSIGNED,
                                                   packedp->isFourstateOrNoDtype());
                srcp = packedp;
            } లేకపోతే ఒకవేళ ((VN_IS(srcDTypep, వరుస_DType) || VN_IS(srcDTypep, DynArrayDType)
                        || VN_IS(srcDTypep, UnpackArrayDType))) {
                ఒకవేళ (VN_IS(dstDTypep, వరుస_DType) || VN_IS(dstDTypep, DynArrayDType)) {
                    పూర్ణాంకం ఖండం_కొలత = 1;
                    ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(streamp->rhsp(), Const)) {
                        ఖండం_కొలత = constp->గమ్యం_SInt();
                        ఒకవేళ (VL_UNLIKELY(ఖండం_కొలత <= 0)) {
                            // Not reachable due to higher level checks when parsing stream
                            // operators commented out to not fail v3error-coverage-checks.
                            // nodep->v3error("Stream block size must be positive, got " <<
                            // blockSize); nodep->v3error("Stream block size must be positive, got
                            // " << blockSize);
                            ఖండం_కొలత = 1;
                        }
                    }
                    // Not reachable due to higher level checks when parsing stream operators
                    // commented out to not fail v3error-coverage-checks.
                    // else {
                    // nodep->v3error("Stream block size must be constant (got " <<
                    // streamp->rhsp()->prettyTypeName() << ")");
                    // }
                    పూర్ణాంకం src_Element_బిట్లు = 0;
                    ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి elemDtp = srcDTypep->subDTypep()) {
                        src_Element_బిట్లు = elemDtp->వెడల్పు();
                    }
                    పూర్ణాంకం dst_Element_బిట్లు = 0;
                    ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి elemDtp = dstDTypep->subDTypep()) {
                        dst_Element_బిట్లు = elemDtp->వెడల్పు();
                    }
                    streamp->unlink_Fr_వెనుక();
                    srcp = కొత్త Ast_Cvt_Array_గమ్యం_Array{
                        srcp->fileline(), srcp->unlink_Fr_వెనుక(), dstDTypep,     నిజం,
                        ఖండం_కొలత,        dst_Element_బిట్లు,       src_Element_బిట్లు};
                    nodep->rhsp(srcp);
                    VL_DO_DANGLING(నెట్టు_Deletep(streamp), streamp);
                } లేకపోతే {
                    streamp->lhsp(కొత్త Ast_Cvt_Array_గమ్యం_Packed{srcp->fileline(), srcp->unlink_Fr_వెనుక(),
                                                          dstDTypep});
                    streamp->dtypeFrom(dstDTypep);
                }
            }
        } లేకపోతే ఒకవేళ (m_doV && replace_కేటాయించు_Multi_Sel(nodep)) {
            తిరిగి నిజం;
        } లేకపోతే ఒకవేళ (replace_కేటాయించు_SFormat_F(nodep)) {
            తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    // Boolean replacements
    తార్కిక నిర్వాహ్యం_Bool_జరుపు(స్థిరరాశి Ast_కణుపు* nodep) {
        // boolean test of AND(const,SHIFTR(x,const)) -> test of AND(SHIFTL(x,const), x)
        ఒకవేళ (!VN_IS(nodep, And)) తిరిగి అబద్ధం;
        ఒకవేళ (!VN_IS(VN_AS(nodep, And)->lhsp(), Const)) తిరిగి అబద్ధం;
        ఒకవేళ (!VN_IS(VN_AS(nodep, And)->rhsp(), జరుపు_R)) తిరిగి అబద్ధం;
        స్థిరరాశి Ast_జరుపు_R* స్థిరరాశి shiftp = VN_AS(VN_AS(nodep, And)->rhsp(), జరుపు_R);
        ఒకవేళ (!VN_IS(shiftp->rhsp(), Const)) తిరిగి అబద్ధం;
        ఒకవేళ (static_cast<uint32_t>(nodep->వెడల్పు()) <= VN_AS(shiftp->rhsp(), Const)->గమ్యం_UInt()) {
            తిరిగి అబద్ధం;
        }
        తిరిగి నిజం;
    }
    శూన్య replace_Bool_జరుపు(Ast_కణుపు* nodep) {
        UINFOTREE(9, nodep, "", "bshft_old");
        AstConst* స్థిరరాశి andConstp = VN_AS(VN_AS(nodep, And)->lhsp(), Const);
        Ast_కణుపు_సమాసం* స్థిరరాశి fromp
            = VN_AS(VN_AS(nodep, And)->rhsp(), జరుపు_R)->lhsp()->unlink_Fr_వెనుక();
        AstConst* స్థిరరాశి జరుపు_Constp
            = VN_AS(VN_AS(VN_AS(nodep, And)->rhsp(), జరుపు_R)->rhsp(), Const);
        V3_సంఖ్య val{andConstp, andConstp->వెడల్పు()};
        val.op_జరుపు_L(andConstp->అంకె(), జరుపు_Constp->అంకె());
        AstAnd* స్థిరరాశి newp
            = కొత్త AstAnd{nodep->fileline(), కొత్త AstConst{nodep->fileline(), val}, fromp};
        // widthMin no longer applicable if different C-expanded width
        nodep->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        UINFOTREE(9, newp, "", "_new");
    }

    శూన్య replaceWithSimulation(Ast_కణుపు* nodep) {
        SimulateVisitor simvis;
        // Run it - may be unoptimizable due to large for loop, etc
        simvis.mainParamEmulate(nodep);
        ఒకవేళ (!simvis.optimizable()) {
            స్థిరరాశి Ast_కణుపు* errorp = simvis.whyNotNodep();
            ఒకవేళ (!errorp) errorp = nodep;
            nodep->v3error("Expecting expression to be constant, but can't determine constant for "
                           << nodep->prettyTypeName() << '\n'
                           << errorp->warnOther() << "... Location of non-constant "
                           << errorp->prettyTypeName() << ": " << simvis.why_Not_సందేశం());
            VL_DO_DANGLING(replaceZero(nodep), nodep);
        } లేకపోతే {
            // Fetch the result
            Ast_కణుపు* స్థిరరాశి valuep = simvis.fetch_విలువ_Null(nodep);  // valuep is owned by Simulate
            UASSERT_OBJ(valuep, nodep, "No value returned from simulation");
            // Replace it
            Ast_కణుపు* స్థిరరాశి newp = valuep->clone_వృక్షం(అబద్ధం);
            newp->fileline(nodep->fileline());
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Simulate->" << newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }

    //----------------------------------------

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        iterateChildrenBackwardsConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_concswap_పేర్లు.reset();
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        // No ASSIGNW removals under funcs, we've long eliminated INITIALs
        // (We should perhaps rename the assignw's to just assigns)
        VL_RESTORER(m_wremove);
        m_wremove = అబద్ధం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCLocalScope* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (!nodep->stmtsp()) {
            nodep->unlink_Fr_వెనుక();
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }
    శూన్య సందర్శించు(AstScope* nodep) override {
        // No ASSIGNW removals under scope, we've long eliminated INITIALs
        VL_RESTORER(m_wremove);
        VL_RESTORER(m_scopep);
        m_wremove = అబద్ధం;
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    శూన్య మార్పిడి_Sides(Ast_కణుపు_Bi_Com* nodep) {
        // COMMUTATIVE({a},CONST) -> COMMUTATIVE(CONST,{a})
        // This simplifies later optimizations
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక_With_తదుపరి();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక_With_తదుపరి();
        nodep->lhsp(rhsp);
        nodep->rhsp(lhsp);
        iterate(nodep);  // Again?
    }

    తార్కిక కలిగియుంది_Member_Access_Recurse(Ast_కణుపు* స్థిరరాశి nodep) {
        ఒకవేళ (!nodep) తిరిగి అబద్ధం;
        స్థిరరాశి కొలత_t id = m_ids(nodep);
        స్థిరరాశి స్వయంచాలక it = m_కలిగియుంది_Member_Access.కనుగొను(id);
        ఒకవేళ (it != m_కలిగియుంది_Member_Access.చివర()) తిరిగి it->రెండవ;
        ++m_statMemberAccessVisits;
        తార్కిక ఫలితం = అబద్ధం;
        ఒకవేళ (VN_IS(nodep, MemberSel) || VN_IS(nodep, పద్ధతి_Call) || VN_IS(nodep, CMethodCall)) {
            ఫలితం = నిజం;
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_FTask_Ref* స్థిరరాశి funcRefp = VN_CAST(nodep, కణుపు_FTask_Ref)) {
            ఒకవేళ (కలిగియుంది_Member_Access_Recurse(funcRefp->taskp())) ఫలితం = నిజం;
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_CCall* స్థిరరాశి funcRefp = VN_CAST(nodep, కణుపు_CCall)) {
            ఒకవేళ (కలిగియుంది_Member_Access_Recurse(funcRefp->funcp())) ఫలితం = నిజం;
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_FTask* స్థిరరాశి funcp = VN_CAST(nodep, కణుపు_FTask)) {
            // Assume that it has a member access
            ఒకవేళ (funcp->recursive()) ఫలితం = నిజం;
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstCFunc* స్థిరరాశి funcp = VN_CAST(nodep, CFunc)) {
            ఒకవేళ (funcp->recursive()) ఫలితం = నిజం;
        }
        ఒకవేళ (!ఫలితం) {
            ఫలితం = కలిగియుంది_Member_Access_Recurse(nodep->op1p())
                     || కలిగియుంది_Member_Access_Recurse(nodep->op2p())
                     || కలిగియుంది_Member_Access_Recurse(nodep->op3p())
                     || కలిగియుంది_Member_Access_Recurse(nodep->op4p());
        }
        ఒకవేళ (!ఫలితం && !VN_IS(nodep, కణుపు_FTask)
            && !VN_IS(nodep, CFunc)  // don't enter into next function
            && కలిగియుంది_Member_Access_Recurse(nodep->nextp())) {
            ఫలితం = నిజం;
        }
        m_కలిగియుంది_Member_Access.emplace(id, ఫలితం);
        తిరిగి ఫలితం;
    }

    తార్కిక సరిపోలిక_Biop_గమ్యం_Bitwise(Ast_కణుపు_Biop* స్థిరరాశి nodep) {
        ఒకవేళ (!m_convert_Logic_గమ్యం_బిట్టు) తిరిగి అబద్ధం;
        ఒకవేళ (!nodep->lhsp()->width1()) తిరిగి అబద్ధం;
        ఒకవేళ (!nodep->rhsp()->width1()) తిరిగి అబద్ధం;
        ఒకవేళ (!nodep->is_శుద్ధ()) తిరిగి అబద్ధం;
        ఒకవేళ (కలిగియుంది_Member_Access_Recurse(nodep)) తిరిగి అబద్ధం;
        తిరిగి నిజం;
    }
    తార్కిక సరిపోలిక_Concat_Rand(AstConcat* nodep) {
        //    CONCAT(RAND, RAND) - created by Chisel code
        AstRand* స్థిరరాశి aRandp = VN_CAST(nodep->lhsp(), Rand);
        స్థిరరాశి AstRand* స్థిరరాశి bRandp = VN_CAST(nodep->rhsp(), Rand);
        ఒకవేళ (!aRandp || !bRandp) తిరిగి అబద్ధం;
        ఒకవేళ (!aRandp->combinable(bRandp)) తిరిగి అబద్ధం;
        UINFO(4, "Concat(Rand,Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlink_Fr_వెనుక());
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        తిరిగి నిజం;
    }
    తార్కిక సరిపోలిక_Sel_Rand(AstSel* nodep) {
        //    SEL(RAND) - created by Chisel code
        AstRand* స్థిరరాశి aRandp = VN_CAST(nodep->fromp(), Rand);
        ఒకవేళ (!aRandp) తిరిగి అబద్ధం;
        ఒకవేళ (aRandp->seedp()) తిరిగి అబద్ధం;
        UINFO(4, "Sel(Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlink_Fr_వెనుక());
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        తిరిగి నిజం;
    }
    తార్కిక సరిపోలిక_గమ్యం_String_NConst(Ast_గమ్యం_String_N* nodep) {
        iterateChildren(nodep);
        ఒకవేళ (స్థిరరాశి AstConsArray* స్థిరరాశి consp = VN_CAST(nodep->lhsp(), ConsArray)) {
            ఒకవేళ (!(m_doExpensive || m_params)) తిరిగి అబద్ధం;
            ఒకవేళ (!consp->is_Constant_వృక్షం()) తిరిగి అబద్ధం;
        } లేకపోతే ఒకవేళ (!VN_IS(nodep->lhsp(), Const)) {
            తిరిగి అబద్ధం;
        }
        replaceWithSimulation(nodep);
        తిరిగి నిజం;
    }
    పూర్ణాంకం నిర్వాహ్యం_Concat_Move(స్థిరరాశి AstConcat* nodep) {
        //    CONCAT under concat  (See moveConcat)
        // Return value: true indicates to do it; 2 means move to LHS
        స్థిరరాశి AstConcat* స్థిరరాశి abConcp = VN_CAST(nodep->lhsp(), Concat);
        స్థిరరాశి AstConcat* స్థిరరాశి bcConcp = VN_CAST(nodep->rhsp(), Concat);
        ఒకవేళ (!abConcp && !bcConcp) తిరిగి 0;
        ఒకవేళ (bcConcp) {
            స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి ap = nodep->lhsp();
            స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి bp = bcConcp->lhsp();
            // If a+b == 32,64,96 etc, then we want to have a+b together on LHS
            ఒకవేళ (VL_BITBIT_I(ap->వెడల్పు() + bp->వెడల్పు()) == 0) తిరిగి 2;  // Transform 2: to abConc
        } లేకపోతే {  // abConcp
            // Unless lhs is already 32 bits due to above, reorder it
            ఒకవేళ (VL_BITBIT_I(nodep->lhsp()->వెడల్పు()) != 0) తిరిగి 1;  // Transform 1: to bcConc
        }
        తిరిగి 0;  // ok
    }
    శూన్య moveConcat(AstConcat* nodep) {
        //    1: CONCAT(CONCAT({a},{b}),{c})  -> CONCAT({a},CONCAT({b}, {c}))
        // or 2: CONCAT({a}, CONCAT({b},{c})) -> CONCAT(CONCAT({a},{b}),{c})
        // Because the lhs of a concat needs a shift but the rhs doesn't,
        // putting additional CONCATs on the RHS leads to fewer assembler operations.
        // However, we'll end up with lots of wide moves if we make huge trees
        // like that, so on 32 bit boundaries, we'll do the opposite form.
        UINFO(4, "Move concat: " << nodep);
        ఒకవేళ (నిర్వాహ్యం_Concat_Move(nodep) > 1) {
            Ast_కణుపు_సమాసం* స్థిరరాశి ap = nodep->lhsp()->unlink_Fr_వెనుక();
            AstConcat* స్థిరరాశి bcConcp = VN_AS(nodep->rhsp(), Concat);
            bcConcp->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి bp = bcConcp->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి cp = bcConcp->rhsp()->unlink_Fr_వెనుక();
            AstConcat* స్థిరరాశి abConcp = కొత్త AstConcat{bcConcp->fileline(), ap, bp};
            nodep->lhsp(abConcp);
            nodep->rhsp(cp);
            // If bp was a concat, then we have this exact same form again!
            // Recurse rather then calling node->iterate to prevent 2^n recursion!
            ఒకవేళ (నిర్వాహ్యం_Concat_Move(abConcp)) moveConcat(abConcp);
            VL_DO_DANGLING(నెట్టు_Deletep(bcConcp), bcConcp);
        } లేకపోతే {
            AstConcat* స్థిరరాశి abConcp = VN_AS(nodep->lhsp(), Concat);
            abConcp->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి ap = abConcp->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి bp = abConcp->rhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి cp = nodep->rhsp()->unlink_Fr_వెనుక();
            AstConcat* స్థిరరాశి bcConcp = కొత్త AstConcat{abConcp->fileline(), bp, cp};
            nodep->lhsp(ap);
            nodep->rhsp(bcConcp);
            ఒకవేళ (నిర్వాహ్యం_Concat_Move(bcConcp)) moveConcat(bcConcp);
            VL_DO_DANGLING(నెట్టు_Deletep(abConcp), abConcp);
        }
    }

    // Special cases
    శూన్య సందర్శించు(AstConst*) override {}  // Already constant

    శూన్య సందర్శించు(AstCell* nodep) override {
        ఒకవేళ (m_params) {
            iterate_And_తదుపరి_Null(nodep->paramsp());
        } లేకపోతే {
            iterateChildren(nodep);
        }
    }
    శూన్య సందర్శించు(AstClassOrPackageRef* nodep) override { iterateChildren(nodep); }

    శూన్య సందర్శించు(Ast_సరిపోలిక_Masked* nodep) override {
        // Do not iterate the tables, they must be constant pool entries
        iterate(nodep->lhsp());
        ఒకవేళ (AstConst* స్థిరరాశి constp = VN_CAST(nodep->lhsp(), Const)) {
            replace_అంకె(nodep, Ast_సరిపోలిక_Masked::fold(constp->అంకె(), nodep->matchp()->varp()));
        }
    }

    శూన్య సందర్శించు(AstPin* nodep) override { iterateChildren(nodep); }

    శూన్య replaceLogEq(AstLogEq* nodep) {
        // LOGEQ(a,b) => AstLogAnd{AstLogOr{AstLogNot{a},b},AstLogOr{AstLogNot{b},a}}
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
        // Do exactly as IEEE says, might result in extra terms, so in future may do differently
        AstLogAnd* స్థిరరాశి newp = కొత్త AstLogAnd{
            nodep->fileline(),
            కొత్త AstLogOr{nodep->fileline(), కొత్త AstLogNot{nodep->fileline(), lhsp}, rhsp},
            కొత్త AstLogOr{nodep->fileline(),
                         కొత్త AstLogNot{nodep->fileline(), rhsp->clone_వృక్షం_శుద్ధ(అబద్ధం)},
                         lhsp->clone_వృక్షం_శుద్ధ(అబద్ధం)}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య replaceSelSel(AstSel* nodep) {
        // SEL(SEL({x},a,b),c,d) => SEL({x},a+c,d)
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* స్థిరరాశి belowp = VN_AS(nodep->fromp(), Sel);
        Ast_కణుపు_సమాసం* స్థిరరాశి fromp = belowp->fromp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి lsb1p = nodep->lsbp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి lsb2p = belowp->lsbp()->unlink_Fr_వెనుక();
        // Eliminate lower range
        UINFO(4, "Elim Lower range: " << nodep);
        Ast_కణుపు_సమాసం* newlsbp;
        ఒకవేళ (VN_IS(lsb1p, Const) && VN_IS(lsb2p, Const)) {
            newlsbp = కొత్త AstConst{lsb1p->fileline(),
                                   VN_AS(lsb1p, Const)->గమ్యం_UInt() + VN_AS(lsb2p, Const)->గమ్యం_UInt()};
            VL_DO_DANGLING(నెట్టు_Deletep(lsb1p), lsb1p);
            VL_DO_DANGLING(నెట్టు_Deletep(lsb2p), lsb2p);
        } లేకపోతే {
            // Width is important, we need the width of the fromp's expression, not the
            // potentially smaller lsb1p's width, but don't insert a redundant AstExtend.
            // Note that due to some sloppiness in earlier passes, lsb1p might actually be wider,
            // so extend to the wider type.
            స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి widep = lsb1p->వెడల్పు() > lsb2p->వెడల్పు() ? lsb1p : lsb2p;
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = widep->వెడల్పు() > lsb2p->వెడల్పు()
                                          ? కొత్త AstExtend{lsb2p->fileline(), lsb2p}
                                          : lsb2p;
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = widep->వెడల్పు() > lsb1p->వెడల్పు()
                                          ? కొత్త AstExtend{lsb1p->fileline(), lsb1p}
                                          : lsb1p;
            lhsp->dtypeFrom(widep);
            rhsp->dtypeFrom(widep);
            newlsbp = కొత్త Ast_కూడు{lsb1p->fileline(), lhsp, rhsp};
            newlsbp->dtypeFrom(widep);
        }
        AstSel* స్థిరరాశి newp = కొత్త AstSel{nodep->fileline(), fromp, newlsbp, nodep->వెడల్పు_Const()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య replaceSelConcat(AstSel* nodep) {
        // SEL(CONCAT(a,b),c,d) => SEL(a or b, . .)
        AstConcat* స్థిరరాశి conp = VN_AS(nodep->fromp(), Concat);
        Ast_కణుపు_సమాసం* స్థిరరాశి conLhsp = conp->lhsp();
        Ast_కణుపు_సమాసం* స్థిరరాశి conRhsp = conp->rhsp();
        ఒకవేళ (static_cast<పూర్ణాంకం>(nodep->lsbConst()) >= conRhsp->వెడల్పు()) {
            conLhsp->unlink_Fr_వెనుక();
            AstSel* స్థిరరాశి newp
                = కొత్త AstSel{nodep->fileline(), conLhsp, nodep->lsbConst() - conRhsp->వెడల్పు(),
                             nodep->వెడల్పు_Const()};
            nodep->replaceWithKeepDType(newp);
        } లేకపోతే ఒకవేళ (static_cast<పూర్ణాంకం>(nodep->msbConst()) < conRhsp->వెడల్పు()) {
            conRhsp->unlink_Fr_వెనుక();
            AstSel* స్థిరరాశి newp
                = కొత్త AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(), nodep->వెడల్పు_Const()};
            nodep->replaceWithKeepDType(newp);
        } లేకపోతే {
            // Yuk, split between the two
            conRhsp->unlink_Fr_వెనుక();
            conLhsp->unlink_Fr_వెనుక();
            AstConcat* స్థిరరాశి newp
                = కొత్త AstConcat{nodep->fileline(),
                                కొత్త AstSel{nodep->fileline(), conLhsp, 0,
                                           nodep->msbConst() - conRhsp->వెడల్పు() + 1},
                                కొత్త AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(),
                                           conRhsp->వెడల్పు() - nodep->lsbConst()}};
            nodep->replaceWithKeepDType(newp);
        }
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    తార్కిక నిర్వాహ్యం_Sel_Replicate(AstSel* nodep) {
        // SEL(REPLICATE(from,rep),lsb,width) => SEL(from,0,width) as long
        // as SEL's width <= b's width
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* స్థిరరాశి repp = VN_AS(nodep->fromp(), Replicate);
        Ast_కణుపు_సమాసం* స్థిరరాశి fromp = repp->srcp();
        స్థిరరాశి AstConst* స్థిరరాశి lsbp = VN_CAST(nodep->lsbp(), Const);
        ఒకవేళ (!lsbp) తిరిగి అబద్ధం;
        UASSERT_OBJ(fromp->వెడల్పు(), nodep, "Not widthed");
        ఒకవేళ ((lsbp->గమ్యం_UInt() / fromp->వెడల్పు())
            != ((lsbp->గమ్యం_UInt() + nodep->వెడల్పు() - 1) / fromp->వెడల్పు())) {
            తిరిగి అబద్ధం;
        }
        //
        fromp->unlink_Fr_వెనుక();
        AstSel* స్థిరరాశి newp = కొత్త AstSel{
            nodep->fileline(), fromp,
            కొత్త AstConst{lsbp->fileline(), lsbp->గమ్యం_UInt() % fromp->వెడల్పు()}, nodep->వెడల్పు_Const()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        తిరిగి నిజం;
    }
    తార్కిక నిర్వాహ్యం_Rep_Rep(AstReplicate* nodep) {
        // REPLICATE(REPLICATE2(from2,cnt2),cnt1) => REPLICATE(from2,(cnt1+cnt2))
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* స్థిరరాశి rep2p = VN_AS(nodep->srcp(), Replicate);
        Ast_కణుపు_సమాసం* స్థిరరాశి from2p = rep2p->srcp();
        స్థిరరాశి AstConst* స్థిరరాశి cnt1p = VN_CAST(nodep->countp(), Const);
        ఒకవేళ (!cnt1p) తిరిగి అబద్ధం;
        స్థిరరాశి AstConst* స్థిరరాశి cnt2p = VN_CAST(rep2p->countp(), Const);
        ఒకవేళ (!cnt2p) తిరిగి అబద్ధం;
        //
        from2p->unlink_Fr_వెనుక();
        AstReplicate* స్థిరరాశి newp
            = కొత్త AstReplicate{nodep->fileline(), from2p, cnt1p->గమ్యం_UInt() * cnt2p->గమ్యం_UInt()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        తిరిగి నిజం;
    }
    తార్కిక నిర్వాహ్యం_Concat_Same(AstConcat* nodep) {
        // CONCAT(fromp,fromp) -> REPLICATE(fromp,1+1)
        // CONCAT(REP(fromp,cnt1),fromp) -> REPLICATE(fromp,cnt1+1)
        // CONCAT(fromp,REP(fromp,cnt1)) -> REPLICATE(fromp,1+cnt1)
        // CONCAT(REP(fromp,cnt1),REP(fromp,cnt2)) -> REPLICATE(fromp,cnt1+cnt2)
        Ast_కణుపు_సమాసం* from1p = nodep->lhsp();
        uint32_t cnt1 = 1;
        Ast_కణుపు_సమాసం* from2p = nodep->rhsp();
        uint32_t cnt2 = 1;
        ఒకవేళ (VN_IS(from1p, Replicate)) {
            స్థిరరాశి AstConst* స్థిరరాశి cnt1p = VN_CAST(VN_CAST(from1p, Replicate)->countp(), Const);
            ఒకవేళ (!cnt1p) తిరిగి అబద్ధం;
            from1p = VN_AS(from1p, Replicate)->srcp();
            cnt1 = cnt1p->గమ్యం_UInt();
        }
        ఒకవేళ (VN_IS(from2p, Replicate)) {
            స్థిరరాశి AstConst* స్థిరరాశి cnt2p = VN_CAST(VN_CAST(from2p, Replicate)->countp(), Const);
            ఒకవేళ (!cnt2p) తిరిగి అబద్ధం;
            from2p = VN_AS(from2p, Replicate)->srcp();
            cnt2 = cnt2p->గమ్యం_UInt();
        }
        ఒకవేళ (!operandsSame(from1p, from2p)) తిరిగి అబద్ధం;
        //
        from1p->unlink_Fr_వెనుక();
        AstReplicate* స్థిరరాశి newp = కొత్త AstReplicate{nodep->fileline(), from1p, cnt1 + cnt2};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        తిరిగి నిజం;
    }
    శూన్య replaceSelIntoBiop(AstSel* nodep) {
        // SEL(BUFIF1(a,b),1,bit) => BUFIF1(SEL(a,1,bit),SEL(b,1,bit))
        Ast_కణుపు_Biop* స్థిరరాశి fromp = VN_AS(nodep->fromp()->unlink_Fr_వెనుక(), కణుపు_Biop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_కణుపు_సమాసం* స్థిరరాశి lsbp = nodep->lsbp()->unlink_Fr_వెనుక();
        //
        Ast_కణుపు_సమాసం* స్థిరరాశి bilhsp = fromp->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి birhsp = fromp->rhsp()->unlink_Fr_వెనుక();
        //
        fromp->lhsp(
            కొత్త AstSel{nodep->fileline(), bilhsp, lsbp->clone_వృక్షం_శుద్ధ(నిజం), nodep->వెడల్పు_Const()});
        fromp->rhsp(కొత్త AstSel{nodep->fileline(), birhsp, lsbp, nodep->వెడల్పు_Const()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య replaceSelIntoUniop(AstSel* nodep) {
        // SEL(NOT(a),1,bit) => NOT(SEL(a,bit))
        Ast_కణుపు_Uniop* స్థిరరాశి fromp = VN_AS(nodep->fromp()->unlink_Fr_వెనుక(), కణుపు_Uniop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_కణుపు_సమాసం* స్థిరరాశి lsbp = nodep->lsbp()->unlink_Fr_వెనుక();
        //
        Ast_కణుపు_సమాసం* స్థిరరాశి bilhsp = fromp->lhsp()->unlink_Fr_వెనుక();
        //
        fromp->lhsp(కొత్త AstSel{nodep->fileline(), bilhsp, lsbp, nodep->వెడల్పు_Const()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య సందర్శించు(Ast_Attr_యొక్క* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstArraySel* nodep) override {
        iterate_And_తదుపరి_Null(nodep->bitp());
        ఒకవేళ (VN_IS(nodep->bitp(), Const)
            && VN_IS(nodep->fromp(), VarRef)
            // Need to make sure it's an array object so don't mis-allow a constant (bug509.)
            && VN_AS(nodep->fromp(), VarRef)->varp()
            && VN_IS(VN_AS(nodep->fromp(), VarRef)->varp()->valuep(), ConsArray)) {
            m_selp = nodep;  // Ask visit(AstVarRef) to replace varref with const
        }
        iterate_And_తదుపరి_Null(nodep->fromp());
        ఒకవేళ (VN_IS(nodep->fromp(), Const)) {  // It did.
            ఒకవేళ (!m_selp) {
                nodep->v3error("Illegal assignment of constant to unpacked array");
            } లేకపోతే {
                Ast_కణుపు* స్థిరరాశి fromp = nodep->fromp()->unlink_Fr_వెనుక();
                nodep->replaceWithKeepDType(fromp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            }
        }
        // Handle ARRAYSEL directly on ConsArray (not through VarRef)
        లేకపోతే ఒకవేళ (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ConsArray)) {
            స్థిరరాశి AstConsArray* స్థిరరాశి consp = VN_AS(nodep->fromp(), ConsArray);
            స్థిరరాశి uint32_t బిట్టు_2 = VN_AS(nodep->bitp(), Const)->గమ్యం_UInt();
            Ast_కణుపు_సమాసం* స్థిరరాశి itemp = consp->valuep(బిట్టు_2);
            ఒకవేళ (VN_IS(itemp, Const)) {
                స్థిరరాశి V3_సంఖ్య& అంకె = VN_AS(itemp, Const)->అంకె();
                VL_DO_DANGLING(replace_అంకె(nodep, అంకె), nodep);
            } లేకపోతే ఒకవేళ (m_params && itemp) {
                // An aggregate element, e.g. a structure in an array parameter
                nodep->replaceWithKeepDType(itemp->clone_వృక్షం(అబద్ధం));
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            }
        }
        m_selp = శూన్యసూచిక;
    }

    // Evaluate a slice of an unpacked array.  If constantification is
    // required (m_required=true), call replaceWithSimulation() to compute
    // the slice via simulation.  Otherwise just iterate the children.
    శూన్య సందర్శించు(Ast_ముక్క_Sel* nodep) override {
        // First constify or width any child nodes
        iterateChildren(nodep);
        ఒకవేళ (!m_required) తిరిగి;  // Do nothing unless we are in parameter mode
        // Fallback to simulation: this will invoke SimulateVisitor::visit(AstSliceSel*)
        replaceWithSimulation(nodep);
    }

    శూన్య సందర్శించు(AstStructSel* nodep) override {
        iterateChildren(nodep);

        ఒకవేళ (స్థిరరాశి AstConsStruct* స్థిరరాశి consp = VN_CAST(nodep->fromp(), ConsStruct)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి valuep = consp->memberValuep(nodep->name());
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(valuep, Const)) {
                VL_DO_DANGLING(replace_అంకె(nodep, constp->అంకె()), nodep);
            } లేకపోతే {
                nodep->replaceWith(valuep->clone_వృక్షం(అబద్ధం));
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            }
        }
    }

    శూన్య సందర్శించు(AstCAwait* nodep) override {
        m_hasJumpDelay = నిజం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Not linked");
        తార్కిక did = అబద్ధం;
        ఒకవేళ (m_doV && (!nodep->varp()->const_Pool_నమోదు() || m_selp) && nodep->varp()->valuep()
            && !m_attrp) {
            // UINFOTREE(1, valuep, "", "visitvaref");
            iterate_And_తదుపరి_Null(nodep->varp()->valuep());  // May change nodep->varp()->valuep()
            Ast_కణుపు* స్థిరరాశి valuep = nodep->varp()->valuep();
            // The selected element, when under a constant select of an array
            స్థిరరాశి AstConsArray* స్థిరరాశి consp = VN_CAST(valuep, ConsArray);
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి selItemp
                = (m_selp && consp) ? consp->valuep(m_selp->బిట్టు_Const()) : శూన్యసూచిక;
            ఒకవేళ (nodep->access().isReadOnly() && valuep
                && ((!m_params  // Can reduce constant wires into equations
                     && m_doNConst && v3Global.opt.fConst() && nodep->varp()->isConst())
                    || nodep->varp()->isParam())) {
                ఒకవేళ (నిర్వాహ్యం_Const(valuep)) {
                    స్థిరరాశి V3_సంఖ్య& అంకె = VN_AS(valuep, Const)->అంకె();
                    // UINFO(2, "constVisit " << cvtToHex(valuep) << " " << num);
                    స్థిరరాశి string origParamName
                        = nodep->varp()->isParam() ? nodep->varp()->name() : "";
                    VL_DO_DANGLING(replace_అంకె(nodep, అంకె, origParamName), nodep);
                    did = నిజం;
                } లేకపోతే ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి itemp = VN_CAST(selItemp, Const)) {
                    VL_DO_DANGLING(replace_అంకె(nodep, itemp->అంకె()), nodep);
                    did = నిజం;
                } లేకపోతే ఒకవేళ (m_params && consp) {
                    // Allow parameters to pass arrays
                    // Earlier recursion of ConsArray made sure each array value is constant
                    // This exception is fairly fragile, i.e. doesn't
                    // support arrays of arrays or other stuff
                    Ast_కణుపు* స్థిరరాశి newp = valuep->clone_వృక్షం(అబద్ధం);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    did = నిజం;
                } లేకపోతే ఒకవేళ (m_params && VN_IS(valuep, ConsStruct)) {
                    Ast_కణుపు* స్థిరరాశి newp = valuep->clone_వృక్షం(అబద్ధం);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    did = నిజం;
                } లేకపోతే ఒకవేళ (nodep->varp()->isParam() && VN_IS(valuep, Unbounded)) {
                    Ast_కణుపు* స్థిరరాశి newp = valuep->clone_వృక్షం(అబద్ధం);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    did = నిజం;
                }
            }
        }
        ఒకవేళ (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but variable isn't const: "
                           << nodep->varp()->prettyNameQ());
        }
    }
    శూన్య సందర్శించు(Ast_సమాసం_Stmt* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (!Ast_కణుపు::afterCommentp(nodep->stmtsp())) {
            deleteVarScopesUnder(nodep->stmtsp());
            UINFO(8, "ExprStmt(...) " << nodep << " " << nodep->resultp());
            nodep->replaceWith(nodep->resultp()->unlink_Fr_వెనుక());
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            // Removing the ExprStmt might have made something impure above now pure
        }
    }
    శూన్య సందర్శించు(Ast_Enum_అంశం_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        తార్కిక did = అబద్ధం;
        ఒకవేళ (nodep->itemp()->valuep()) {
            // UINFOTREE(1, nodep->itemp()->valuep(), "", "visitvaref");
            స్థిరరాశి Ast_Enum_అంశం* స్థిరరాశి itemp = nodep->itemp();
            ఒకవేళ (m_recursing_Enum_వస్తువులు.లెక్క(itemp)) {
                nodep->v3error("Recursive enum value: " << nodep->itemp()->prettyNameQ());
            } లేకపోతే {
                m_recursing_Enum_వస్తువులు.emplace(itemp);
                iterate_And_తదుపరి_Null(nodep->itemp()->valuep());
                m_recursing_Enum_వస్తువులు.erase(itemp);
            }
            ఒకవేళ (AstConst* స్థిరరాశి valuep = VN_CAST(nodep->itemp()->valuep(), Const)) {
                స్థిరరాశి V3_సంఖ్య& అంకె = valuep->అంకె();
                VL_DO_DANGLING(replace_అంకె(nodep, అంకె), nodep);
                did = నిజం;
            }
        }
        ఒకవేళ (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but enum value isn't const: "
                           << nodep->itemp()->prettyNameQ());
        }
    }

    తార్కిక only_Sen_అంశం_లోపల_Sen_వృక్షం(స్థిరరాశి Ast_Sen_అంశం* nodep) {
        // Only one if it's not in a list
        తిరిగి (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }
    శూన్య సందర్శించు(Ast_Sen_అంశం* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (m_doNConst
            && !v3Global.opt.timing().is_సమితి_True()  // If --timing, V3Sched would turn this into an
                                                   // infinite loop. See #5080
            && (VN_IS(nodep->sensp(), Const) || VN_IS(nodep->sensp(), Enum_అంశం_Ref)
                || (nodep->varrefp() && nodep->varrefp()->varp()->isParam()))) {
            // Constants in sensitivity lists may be removed (we'll simplify later)
            ఒకవేళ (nodep->isClocked()) {  // A constant can never get a pos/negedge
                ఒకవేళ (only_Sen_అంశం_లోపల_Sen_వృక్షం(nodep)) {
                    ఒకవేళ (nodep->అంచు_Type() == VEdgeType::ET_మారింది) {
                        // TODO: This really is dodgy, as strictly compliant simulators will not
                        //       execute this block, but but t_func_check relies on it
                        nodep->replaceWith(
                            కొత్త Ast_Sen_అంశం{nodep->fileline(), Ast_Sen_అంశం::Initial{}});
                    } లేకపోతే {
                        nodep->replaceWith(కొత్త Ast_Sen_అంశం{nodep->fileline(), Ast_Sen_అంశం::Never{}});
                    }
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                } లేకపోతే {
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                }
            } లేకపోతే {  // Otherwise it may compute a result that needs to settle out
                nodep->replaceWith(కొత్త Ast_Sen_అంశం{nodep->fileline(), Ast_Sen_అంశం::Combo{}});
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            }
        } లేకపోతే ఒకవేళ (m_doNConst && VN_IS(nodep->sensp(), Not)) {
            // V3Gate may propagate NOTs into clocks... Just deal with it
            Ast_కణుపు* స్థిరరాశి sensp = nodep->sensp();
            Ast_కణుపు* చివరి_Sensp = sensp;
            తార్కిక invert = అబద్ధం;
            అయితే (VN_IS(చివరి_Sensp, Not)) {
                చివరి_Sensp = VN_AS(చివరి_Sensp, Not)->lhsp();
                invert = !invert;
            }
            UINFO(8, "senItem(NOT...) " << nodep << " " << invert);
            ఒకవేళ (invert) nodep->అంచు_Type(nodep->అంచు_Type().invert());
            sensp->replaceWith(చివరి_Sensp->unlink_Fr_వెనుక());
            VL_DO_DANGLING(నెట్టు_Deletep(sensp), sensp);
        }
    }

    తరగతి Sen_అంశం_Cmp final {
        స్థిర పూర్ణాంకం cmp(స్థిరరాశి Ast_కణుపు_సమాసం* ap, స్థిరరాశి Ast_కణుపు_సమాసం* bp) {
            స్థిరరాశి VNType aType = ap->type();
            స్థిరరాశి VNType bType = bp->type();
            ఒకవేళ (aType != bType) తిరిగి static_cast<పూర్ణాంకం>(bType) - static_cast<పూర్ణాంకం>(aType);

            ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి aRefp = VN_CAST(ap, VarRef)) {
                స్థిరరాశి AstVarRef* స్థిరరాశి bRefp = VN_AS(bp, VarRef);
                // Looks visually better if we keep sorted by name
                ఒకవేళ (aRefp->name() < bRefp->name()) తిరిగి -1;
                ఒకవేళ (aRefp->name() > bRefp->name()) తిరిగి 1;
                // But might be same name with different scopes
                ఒకవేళ (aRefp->varScopep() < bRefp->varScopep()) తిరిగి -1;
                ఒకవేళ (aRefp->varScopep() > bRefp->varScopep()) తిరిగి 1;
                // Or rarely, different data types
                ఒకవేళ (aRefp->dtypep() < bRefp->dtypep()) తిరిగి -1;
                ఒకవేళ (aRefp->dtypep() > bRefp->dtypep()) తిరిగి 1;
                తిరిగి 0;
            }

            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి aConstp = VN_CAST(ap, Const)) {
                స్థిరరాశి AstConst* స్థిరరాశి bConstp = VN_AS(bp, Const);
                ఒకవేళ (aConstp->అంకె().isLtXZ(bConstp->అంకె())) తిరిగి -1;
                ఒకవేళ (bConstp->అంకె().isLtXZ(aConstp->అంకె())) తిరిగి 1;
                తిరిగి 0;
            }

            ఒకవేళ (స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి aBiOpp = VN_CAST(ap, కణుపు_Biop)) {
                స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి bBiOpp = VN_AS(bp, కణుపు_Biop);
                // Compare RHSs first, as LHS might be const
                ఒకవేళ (స్థిరరాశి పూర్ణాంకం c = cmp(aBiOpp->rhsp(), bBiOpp->rhsp())) తిరిగి c;
                తిరిగి cmp(aBiOpp->lhsp(), bBiOpp->lhsp());
            }

            ఒకవేళ (స్థిరరాశి AstCMethodHard* స్థిరరాశి aCallp = VN_CAST(ap, CMethodHard)) {
                స్థిరరాశి AstCMethodHard* స్థిరరాశి bCallp = VN_AS(bp, CMethodHard);
                ఒకవేళ (aCallp->పద్ధతి() < bCallp->పద్ధతి()) తిరిగి -1;
                ఒకవేళ (aCallp->పద్ధతి() > bCallp->పద్ధతి()) తిరిగి 1;
                ఒకవేళ (స్థిరరాశి పూర్ణాంకం c = cmp(aCallp->fromp(), bCallp->fromp())) తిరిగి c;
                స్థిరరాశి Ast_కణుపు_సమాసం* aPinsp = aCallp->pinsp();
                స్థిరరాశి Ast_కణుపు_సమాసం* bPinsp = bCallp->pinsp();
                అయితే (aPinsp && bPinsp) {
                    ఒకవేళ (స్థిరరాశి పూర్ణాంకం c = cmp(aPinsp, bPinsp)) తిరిగి c;
                    aPinsp = VN_AS(aPinsp->nextp(), కణుపు_సమాసం);
                    bPinsp = VN_AS(bPinsp->nextp(), కణుపు_సమాసం);
                }
                తిరిగి aPinsp ? -1 : bPinsp ? 1 : 0;
            }

            తిరిగి 0;
        }

    బహిరంగ:
        తార్కిక క్రియాకారకం()(స్థిరరాశి Ast_Sen_అంశం* lhsp, స్థిరరాశి Ast_Sen_అంశం* rhsp) స్థిరరాశి {
            స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి lSensp = lhsp->sensp();
            స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి rSensp = rhsp->sensp();
            ఒకవేళ (lSensp && rSensp) {
                // If both terms have sensitivity expressions, recursively compare them
                ఒకవేళ (స్థిరరాశి పూర్ణాంకం c = cmp(lSensp, rSensp)) తిరిగి c < 0;
            } లేకపోతే ఒకవేళ (lSensp || rSensp) {
                // Terms with sensitivity expressions come after those without
                తిరిగి rSensp;
            }
            // Finally sort by edge, AFTER variable, as we want multiple edges for same var
            // adjacent. note the SenTree optimizer requires this order (more general first,
            // less general last)
            తిరిగి lhsp->అంచు_Type() < rhsp->అంచు_Type();
        }
    };

    శూన్య సందర్శించు(Ast_Sen_వృక్షం* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (m_doExpensive) {
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
            కొరకు (Ast_Sen_అంశం *nextp, *senp = nodep->sensesp(); senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), Sen_అంశం);
                // cppcheck-suppress unassignedVariable  // cppcheck bug
                స్థిరరాశి Sen_అంశం_Cmp cmp;
                ఒకవేళ (nextp && !cmp(senp, nextp)) {
                    // Something's out of order, sort it
                    senp = శూన్యసూచిక;
                    std::సదిశ<Ast_Sen_అంశం*> vec;
                    కొరకు (Ast_Sen_అంశం* senp = nodep->sensesp(); senp;
                         senp = VN_AS(senp->nextp(), Sen_అంశం)) {
                        vec.నెట్టు_వెనుక(senp);
                    }
                    stable_క్రమీకరించు(vec.begin(), vec.చివర(), Sen_అంశం_Cmp());
                    కొరకు (స్థిరరాశి స్వయంచాలక& ip : vec) ip->unlink_Fr_వెనుక();
                    కొరకు (స్థిరరాశి స్వయంచాలక& ip : vec) nodep->కూడు_Sensesp(ip);
                    ఆపు;
                }
            }

            // Pass 2, remove duplicates and simplify adjacent terms if possible
            కొరకు (Ast_Sen_అంశం *senp = nodep->sensesp(), *nextp; senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), Sen_అంశం);
                ఒకవేళ (!nextp) ఆపు;
                Ast_Sen_అంశం* స్థిరరాశి lItemp = senp;
                Ast_Sen_అంశం* స్థిరరాశి rItemp = nextp;
                స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి lSenp = lItemp->sensp();
                స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి rSenp = rItemp->sensp();
                ఒకవేళ (!lSenp || !rSenp) కొనసాగు;

                ఒకవేళ (lSenp->same_Gate_వృక్షం(rSenp)) {
                    // POSEDGE or NEGEDGE -> BOTHEDGE. (We've sorted POSEDGE, before NEGEDGE, so we
                    // do not need to test for the opposite orders.)
                    ఒకవేళ (lItemp->అంచు_Type() == VEdgeType::ET_POSEDGE
                        && rItemp->అంచు_Type() == VEdgeType::ET_NEGEDGE) {
                        // Make both terms BOTHEDGE, the second will be removed below
                        lItemp->అంచు_Type(VEdgeType::ET_BOTHEDGE);
                        rItemp->అంచు_Type(VEdgeType::ET_BOTHEDGE);
                    }

                    // Remove identical expressions
                    ఒకవేళ (lItemp->అంచు_Type() == rItemp->అంచు_Type()) {
                        VL_DO_DANGLING(నెట్టు_Deletep(rItemp->unlink_Fr_వెనుక()), rItemp);
                        nextp = lItemp;
                    }
                }
            }
        }
    }

    //-----
    // Zero elimination
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (nodep->timingControlp()) m_hasJumpDelay = నిజం;
        ఒకవేళ (m_doNConst && replace_కణుపు_కేటాయించు(nodep)) తిరిగి;
    }
    శూన్య సందర్శించు(AstAlias* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    శూన్య సందర్శించు(AstAliasScope* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_W* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (m_doNConst && replace_కణుపు_కేటాయించు(nodep)) తిరిగి;
        // Process containing this AssignW as single body statement
        AstAlways* స్థిరరాశి procp = VN_CAST(nodep->backp(), Always);
        ఒకవేళ (!procp || procp->stmtsp() != nodep || nodep->nextp()) తిరిగి;
        // Not VarXRef, as different refs may set different values to each hierarchy
        Ast_కణుపు_Var_Ref* స్థిరరాశి varrefp = VN_CAST(nodep->lhsp(), VarRef);
        ఒకవేళ (m_wremove && !m_params && m_doNConst && m_modp && నిర్వాహ్యం_Const(nodep->rhsp())
            && !VN_AS(nodep->rhsp(), Const)->అంకె().isFourState()
            && varrefp  // Don't do messes with BITREFs/ARRAYREFs
            && !varrefp->varp()->hasStrengthAssignment()  // Strengths are resolved in V3Tristate
            && !varrefp->varp()->valuep()  // Not already constified
            && !varrefp->varScopep()  // Not scoped (or each scope may have different initial val.)
            && !varrefp->varp()->isForced()  // Not forced (not really a constant)
        ) {
            // ASSIGNW (VARREF, const) -> INITIAL ( ASSIGN (VARREF, const) )
            UINFO(4, "constAssignW " << nodep);
            // Make a initial assignment
            Ast_కణుపు_సమాసం* స్థిరరాశి exprp = nodep->rhsp()->unlink_Fr_వెనుక();
            varrefp->unlink_Fr_వెనుక();
            File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
            AstInitial* స్థిరరాశి newinitp = కొత్త AstInitial{flp, కొత్త Ast_కేటాయించు{flp, varrefp, exprp}};
            procp->replaceWith(newinitp);
            VL_DO_DANGLING(నెట్టు_Deletep(procp), procp);
            // Set the initial value right in the variable so we can constant propagate
            Ast_కణుపు* స్థిరరాశి initvaluep = exprp->clone_వృక్షం(అబద్ధం);
            varrefp->varp()->isConst(నిజం);
            varrefp->varp()->valuep(initvaluep);
        }
    }
    శూన్య సందర్శించు(AstCReset* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (!m_doNConst) తిరిగి;
        స్థిరరాశి AstBasicDType* స్థిరరాశి bdtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType);
        ఒకవేళ (!bdtypep) తిరిగి;
        ఒకవేళ (!bdtypep->isZeroInit()) తిరిగి;
        AstConst* స్థిరరాశి newp = కొత్త AstConst{nodep->fileline(), V3_సంఖ్య{nodep, bdtypep}};
        UINFO(9, "CRESET(0) => CONST(0) " << nodep);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(Ast_Cvt_Unpack_గమ్యం_Unpack* nodep) override {
        iterateChildren(nodep);
        AstConsArray* స్థిరరాశి fromp = VN_CAST(nodep->fromp(), ConsArray);
        ఒకవేళ (!fromp) తిరిగి;
        // Fold the outermost dimension, converting the elements for any dimension under it
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి గమ్యం_Arrayp
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి fromArrayp
            = VN_AS(fromp->dtypep()->skipRefp(), UnpackArrayDType);
        // This dimension may not be reversed, only one under it
        స్థిరరాశి తార్కిక తిరగేయి
            = గమ్యం_Arrayp->declRange().ascending() != fromArrayp->declRange().ascending();
        స్థిరరాశి uint64_t చివరి_సూచీ = గమ్యం_Arrayp->elementsConst() - 1;
        Ast_కణుపు_DType* స్థిరరాశి subDTypep = గమ్యం_Arrayp->subDTypep();
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి subArrayp
            = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
        స్థిరరాశి స్వయంచాలక convert = [&](Ast_కణుపు_సమాసం* valuep) -> Ast_కణుపు_సమాసం* {
            valuep->unlink_Fr_వెనుక();
            ఒకవేళ (!subArrayp || !subArrayp->is_తిరగేయి_యొక్క(valuep->dtypep())) తిరిగి valuep;
            తిరిగి కొత్త Ast_Cvt_Unpack_గమ్యం_Unpack{valuep->fileline(), valuep, subDTypep};
        };
        AstConsArray* స్థిరరాశి newp
            = కొత్త AstConsArray{fromp->fileline(), nodep->dtypep(),
                               fromp->defaultp() ? convert(fromp->defaultp()) : శూన్యసూచిక};
        కొరకు (స్థిరరాశి Ast_Cons_Array_అంశం* itemp = fromp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_అంశం)) {
            newp->valuep(తిరగేయి ? చివరి_సూచీ - itemp->సూచీ() : itemp->సూచీ(),
                         convert(itemp->valuep()));
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        iterate(newp);  // Fold the conversions of the elements
    }
    శూన్య సందర్శించు(Ast_Cvt_Array_గమ్యం_Array* nodep) override {
        iterateChildren(nodep);
        // Handle the case where we have a stream operation inside a cast conversion
        // Check for both StreamL and StreamR operations
        Ast_కణుపు_ప్రవాహం* streamp = శూన్యసూచిక;
        తార్కిక is_తిరగేయి = అబద్ధం;
        ఒకవేళ (Ast_ప్రవాహం_L* స్థిరరాశి ప్రవాహం_Lp = VN_CAST(nodep->fromp(), ప్రవాహం_L)) {
            streamp = ప్రవాహం_Lp;
            is_తిరగేయి = నిజం;  // StreamL reverses the operation
        } లేకపోతే ఒకవేళ (Ast_ప్రవాహం_R* స్థిరరాశి ప్రవాహం_Rp = VN_CAST(nodep->fromp(), ప్రవాహం_R)) {
            streamp = ప్రవాహం_Rp;
            is_తిరగేయి = అబద్ధం;  // StreamR doesn't reverse the operation
        }
        ఒకవేళ (streamp) {
            Ast_కణుపు_సమాసం* srcp = streamp->lhsp();
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి srcDTypep = srcp->dtypep()->skipRefp();
            Ast_కణుపు_DType* స్థిరరాశి dstDTypep = nodep->dtypep()->skipRefp();
            ఒకవేళ (VN_IS(srcDTypep, వరుస_DType) && VN_IS(dstDTypep, వరుస_DType)) {
                పూర్ణాంకం ఖండం_కొలత = 1;
                ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(streamp->rhsp(), Const)) {
                    ఖండం_కొలత = constp->గమ్యం_SInt();
                    ఒకవేళ (VL_UNLIKELY(ఖండం_కొలత <= 0)) {
                        // Not reachable due to higher level checks when parsing stream
                        // operators commented out to not fail v3error-coverage-checks.
                        // nodep->v3error("Stream block size must be positive, got " <<
                        // blockSize);
                        ఖండం_కొలత = 1;
                    }
                }
                // Not reachable due to higher level checks when parsing stream operators
                // commented out to not fail v3error-coverage-checks.
                // else {
                //    nodep->v3error("Stream block size must be constant (got " <<
                //    streamp->rhsp()->prettyTypeName() << ")");
                // }
                పూర్ణాంకం src_Element_బిట్లు = 0;
                ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి elemDtp = srcDTypep->subDTypep()) {
                    src_Element_బిట్లు = elemDtp->వెడల్పు();
                }
                పూర్ణాంకం dst_Element_బిట్లు = 0;
                ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి elemDtp = dstDTypep->subDTypep()) {
                    dst_Element_బిట్లు = elemDtp->వెడల్పు();
                }
                streamp->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* newp = కొత్త Ast_Cvt_Array_గమ్యం_Array{
                    srcp->fileline(), srcp->unlink_Fr_వెనుక(), dstDTypep,     is_తిరగేయి,
                    ఖండం_కొలత,        dst_Element_బిట్లు,       src_Element_బిట్లు};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(streamp), streamp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి;
            }
        }
    }
    శూన్య సందర్శించు(AstRelease* nodep) override {
        // cppcheck-suppress constVariablePointer // children unlinked below
        ఒకవేళ (AstConcat* స్థిరరాశి concatp = VN_CAST(nodep->lhsp(), Concat)) {
            File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
            AstRelease* స్థిరరాశి newLp = కొత్త AstRelease{flp, concatp->lhsp()->unlink_Fr_వెనుక()};
            AstRelease* స్థిరరాశి newRp = కొత్త AstRelease{flp, concatp->rhsp()->unlink_Fr_వెనుక()};
            nodep->replaceWith(newLp);
            newLp->కూడు_తదుపరి_ఇక్కడ(newRp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            సందర్శించు(newLp);
            సందర్శించు(newRp);
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (m_doNConst) {
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->condp(), Const)) {
                Ast_కణుపు* keepp = శూన్యసూచిక;
                Ast_కణుపు* delp = శూన్యసూచిక;
                ఒకవేళ (constp->isZero()) {
                    UINFO(4, "IF(0,{any},{x}) => {x}: " << nodep);
                    keepp = nodep->elsesp();
                    delp = nodep->thensp();
                } లేకపోతే ఒకవేళ (!m_doV || constp->isNeqZero()) {  // Might be X in Verilog
                    UINFO(4, "IF(!0,{x},{any}) => {x}: " << nodep);
                    keepp = nodep->thensp();
                    delp = nodep->elsesp();
                } లేకపోతే {
                    UINFO(4, "IF condition is X, retaining: " << nodep);
                    తిరిగి;
                }
                ఒకవేళ (!canDeleteStatements()) {
                    UINFO(4, "IF retained until statics lifted: " << nodep);
                    తిరిగి;
                }

                // If we delete a branch that contains variable declarations, also delete the
                // corresponding varscopes so we don't leave dangling AstVarScope::m_varp pointers.
                deleteVarScopesUnder(delp);

                ఒకవేళ (keepp) {
                    keepp->unlink_Fr_వెనుక_With_తదుపరి();
                    nodep->replaceWith(keepp);
                } లేకపోతే {
                    nodep->unlink_Fr_వెనుక();
                    deleteVarScopesUnder(nodep->thensp());
                    deleteVarScopesUnder(nodep->elsesp());
                }
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);

                // Removed branch could contain only impurity within a function and the
                // function could be purified
                VIsCached::clear_Cache_వృక్షం();
            } లేకపోతే ఒకవేళ (!Ast_కణుపు::afterCommentp(nodep->thensp())
                       && !Ast_కణుపు::afterCommentp(nodep->elsesp())) {
                ఒకవేళ (!nodep->condp()->is_శుద్ధ()) {
                    // Condition has side effect - leave, but don't need return value
                    UINFO(4, "IF({x}) ; => STMTEXPR({x}): " << nodep);
                    nodep->fileline()->warnOff(V3_Error_సంకేతం::IGNOREDRETURN, నిజం);
                    Ast_కణుపు_Stmt* స్థిరరాశి newp
                        = కొత్త Ast_Stmt_సమాసం{nodep->fileline(), nodep->condp()->unlink_Fr_వెనుక()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                } లేకపోతే {
                    // Empty block, remove it
                    UINFO(4, "IF({x}) ; => ; : " << nodep);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                }
            } లేకపోతే ఒకవేళ (!Ast_కణుపు::afterCommentp(nodep->thensp())) {
                UINFO(4, "IF({x}) nullptr {...} => IF(NOT{x}}: " << nodep);
                Ast_కణుపు_సమాసం* స్థిరరాశి condp = nodep->condp();
                Ast_కణుపు* స్థిరరాశి elsesp = nodep->elsesp();
                condp->unlink_Fr_వెనుక_With_తదుపరి();
                elsesp->unlink_Fr_వెనుక_With_తదుపరి();
                ఒకవేళ (nodep->thensp()) {  // Must have been comment
                    నెట్టు_Deletep(nodep->thensp()->unlink_Fr_వెనుక_With_తదుపరి());
                }
                nodep->condp(కొత్త AstLogNot{condp->fileline(),
                                           condp});  // LogNot, as C++ optimization also possible
                nodep->కూడు_Thensp(elsesp);
                nodep->branchPred(nodep->branchPred().invert());
            } లేకపోతే ఒకవేళ (((VN_IS(nodep->condp(), Not) && nodep->condp()->వెడల్పు() == 1)
                        || VN_IS(nodep->condp(), LogNot))
                       && nodep->thensp() && nodep->elsesp()) {
                UINFO(4, "IF(NOT {x})  => IF(x) swapped if/else" << nodep);
                Ast_కణుపు_సమాసం* స్థిరరాశి condp
                    = VN_AS(nodep->condp(), కణుపు_Uniop)->lhsp()->unlink_Fr_వెనుక_With_తదుపరి();
                Ast_కణుపు* స్థిరరాశి thensp = nodep->thensp()->unlink_Fr_వెనుక_With_తదుపరి();
                Ast_కణుపు* స్థిరరాశి elsesp = nodep->elsesp()->unlink_Fr_వెనుక_With_తదుపరి();
                AstIf* స్థిరరాశి ifp = కొత్త AstIf{nodep->fileline(), condp, elsesp, thensp};
                ifp->isBoundsCheck(nodep->isBoundsCheck());  // Copy bounds check info
                ifp->branchPred(nodep->branchPred().invert());
                nodep->replaceWith(ifp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            } లేకపోతే ఒకవేళ (if_Same_కేటాయించు(nodep)) {
                UINFO(4,
                      "IF({a}) ASSIGN({b},{c}) else ASSIGN({b},{d}) => ASSIGN({b}, {a}?{c}:{d})");
                Ast_కణుపు_కేటాయించు* స్థిరరాశి thensp = VN_AS(nodep->thensp(), కణుపు_కేటాయించు);
                // cppcheck-suppress constVariablePointer // children unlinked below
                Ast_కణుపు_కేటాయించు* స్థిరరాశి elsesp = VN_AS(nodep->elsesp(), కణుపు_కేటాయించు);
                thensp->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* స్థిరరాశి condp = nodep->condp()->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* స్థిరరాశి truep = thensp->rhsp()->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* స్థిరరాశి falsep = elsesp->rhsp()->unlink_Fr_వెనుక();
                thensp->rhsp(కొత్త AstCond{truep->fileline(), condp, truep, falsep});
                nodep->replaceWith(thensp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            } లేకపోతే ఒకవేళ (అబద్ధం  // Disabled, as vpm assertions are faster
                              // without due to short-circuiting
                       && నిర్వాహ్యం_If_If(nodep)) {
                UINFO(9, "IF({a}) IF({b}) => IF({a} && {b})");
                Ast_కణుపు_If* స్థిరరాశి క్రింది_Ifp = VN_AS(nodep->thensp(), కణుపు_If);
                Ast_కణుపు_సమాసం* స్థిరరాశి condp = nodep->condp()->unlink_Fr_వెనుక();
                Ast_కణుపు* స్థిరరాశి క్రింది_Thensp = క్రింది_Ifp->thensp()->unlink_Fr_వెనుక_With_తదుపరి();
                Ast_కణుపు_సమాసం* స్థిరరాశి క్రింది_Condp = క్రింది_Ifp->condp()->unlink_Fr_వెనుక_With_తదుపరి();
                nodep->condp(కొత్త AstLogAnd{క్రింది_Ifp->fileline(), condp, క్రింది_Condp});
                క్రింది_Ifp->replaceWith(క్రింది_Thensp);
                VL_DO_DANGLING(నెట్టు_Deletep(క్రింది_Ifp), క్రింది_Ifp);
            } లేకపోతే {
                // Optimizations that don't reform the IF itself
                ఒకవేళ (నిర్వాహ్యం_Bool_జరుపు(nodep->condp())) replace_Bool_జరుపు(nodep->condp());
                సరిపోలిక_If_Same_చివరి(nodep);
                సరిపోలిక_If_Cond_Cond(nodep);
            }
        }
    }

    శూన్య సందర్శించు(AstDisplay* nodep) override {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        iterateChildren(nodep);
        ఒకవేళ (stmtDisplayDisplay(nodep)) తిరిగి;
    }
    తార్కిక stmtDisplayDisplay(AstDisplay* nodep) {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        ఒకవేళ (!m_modp) తిరిగి అబద్ధం;  // Don't optimize under single statement
        AstDisplay* స్థిరరాశి prevp = VN_CAST(nodep->backp(), Display);
        ఒకవేళ (!prevp) తిరిగి అబద్ధం;
        ఒకవేళ (!((prevp->displayType() == nodep->displayType())
              || (prevp->displayType() == VDisplayType::DT_WRITE
                  && nodep->displayType() == VDisplayType::DT_DISPLAY)
              || (prevp->displayType() == VDisplayType::DT_DISPLAY
                  && nodep->displayType() == VDisplayType::DT_WRITE)))
            తిరిగి అబద్ధం;
        ఒకవేళ ((prevp->filep() && !nodep->filep()) || (!prevp->filep() && nodep->filep())
            || (prevp->filep() && nodep->filep() && !prevp->filep()->same_వృక్షం(nodep->filep())))
            తిరిగి అబద్ధం;
        ఒకవేళ (!prevp->fmtp() || prevp->fmtp()->nextp() || !nodep->fmtp() || nodep->fmtp()->nextp())
            తిరిగి అబద్ధం;
        AstSFormatF* స్థిరరాశి pformatp = prevp->fmtp();
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSFormatF* స్థిరరాశి nformatp = nodep->fmtp();
        // We don't merge scopeNames as can have only one and might be different scopes (late in
        // process) Also rare for real code to print %m multiple times in same message
        ఒకవేళ (nformatp->scopeNamep() && pformatp->scopeNamep()) తిరిగి అబద్ధం;
        // We don't early merge arguments as might need to later print warnings with
        // right line numbers, nor scopeNames as might be different scopes (late in process)
        ఒకవేళ (!m_doCpp && pformatp->exprsp()) తిరిగి అబద్ధం;
        ఒకవేళ (!m_doCpp && nformatp->exprsp()) తిరిగి అబద్ధం;
        ఒకవేళ (pformatp->exprsp() && !pformatp->exprsp()->is_శుద్ధ_And_తదుపరి()) తిరిగి అబద్ధం;
        ఒకవేళ (nformatp->exprsp() && !nformatp->exprsp()->is_శుద్ధ_And_తదుపరి()) తిరిగి అబద్ధం;
        // Avoid huge merges
        స్థిర constexpr పూర్ణాంకం DISPLAY_MAX_విలీనం_నిడివి = 500;
        ఒకవేళ (pformatp->పాఠ్యం().నిడివి() + nformatp->పాఠ్యం().నిడివి() > DISPLAY_MAX_విలీనం_నిడివి)
            తిరిగి అబద్ధం;
        //
        UINFO(9, "DISPLAY(SF({a})) DISPLAY(SF({b})) -> DISPLAY(SF({a}+{b}))");
        // Convert DT_DISPLAY to DT_WRITE as may allow later optimizations
        ఒకవేళ (prevp->displayType() == VDisplayType::DT_DISPLAY) {
            prevp->displayType(VDisplayType::DT_WRITE);
            pformatp->పాఠ్యం(pformatp->పాఠ్యం() + "\n");
        }
        // We can't replace prev() as the edit tracking iterators will get confused.
        // So instead we edit the prev note itself.
        ఒకవేళ (prevp->కూడు_Newline()) pformatp->పాఠ్యం(pformatp->పాఠ్యం() + "\n");
        pformatp->పాఠ్యం(pformatp->పాఠ్యం() + nformatp->పాఠ్యం());
        ఒకవేళ (!prevp->కూడు_Newline() && nodep->కూడు_Newline()) pformatp->పాఠ్యం(pformatp->పాఠ్యం() + "\n");
        ఒకవేళ (nformatp->exprsp()) pformatp->కూడు_Exprsp(nformatp->exprsp()->unlink_Fr_వెనుక_With_తదుపరి());
        ఒకవేళ (AstScopeName* స్థిరరాశి scopeNamep = nformatp->scopeNamep()) {
            scopeNamep->unlink_Fr_వెనుక_With_తదుపరి();
            pformatp->scopeNamep(scopeNamep);
        }
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
        తిరిగి నిజం;
    }
    శూన్య సందర్శించు(AstSFormatArg* nodep) override {
        // Skip namep(): requiring its runtime lookup to be constant can reject valid
        // enum-valued constant-function calls in parameters. displayedEnum() resolves
        // the name from the folded exprp() value and enum dtype instead.
        iterate_And_తదుపరి_Null(nodep->exprp());
    }
    శూన్య సందర్శించు(AstSFormatF* nodep) override {
        // Substitute constants into displays.  The main point of this is to
        // simplify assertion methodologies which call functions with display's.
        // This eliminates a pile of wide temps, and makes the C a whole lot more readable.
        iterateChildren(nodep);
        ఒకవేళ (nodep->సమాసం_Format_2()) {
            // Upconvert to text-based format?
            // Similar code in V3LinkResolve::visit(AstSFormatF)
            UASSERT_OBJ(nodep->పాఠ్యం() == "", nodep,
                        "Non-format $sformatf should have text format == \"\"");
            ఒకవేళ (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->అంకె().isFromString()) {
                AstConst* స్థిరరాశి fmtp = VN_AS(nodep->exprsp()->unlink_Fr_వెనుక(), Const);
                nodep->పాఠ్యం(fmtp->అంకె().గమ్యం_String());
                nodep->సమాసం_Format_2(అబద్ధం);
                VL_DO_DANGLING(నెట్టు_Deletep(fmtp), fmtp);
            }
        }
        ఒకవేళ (nodep->సమాసం_Format_2()) తిరిగి;  // Runtime, unfortunately  (unless constify format later)
        తార్కిక anyconst = అబద్ధం;
        కొరకు (Ast_కణుపు* argp = nodep->exprsp(); argp; argp = argp->nextp()) {
            AstSFormatArg* స్థిరరాశి fargp = VN_CAST(argp, SFormatArg);
            Ast_కణుపు* స్థిరరాశి subargp = fargp ? fargp->exprp() : argp;
            ఒకవేళ (VN_IS(subargp, Const)) {
                anyconst = నిజం;
                ఆపు;
            }
        }
        ఒకవేళ (m_doNConst && anyconst) {
            UINFO(9, "  Display in  " << nodep->పాఠ్యం());
            string newFormat;
            string fmt;
            తార్కిక లోపల_Pct = అబద్ధం;
            Ast_కణుపు* argp = nodep->exprsp();
            స్థిరరాశి string పాఠ్యం = nodep->పాఠ్యం();
            కొరకు (అక్షరం ch : పాఠ్యం) {
                ఒకవేళ (!లోపల_Pct && ch == '%') {
                    లోపల_Pct = నిజం;
                    fmt = ch;
                } లేకపోతే ఒకవేళ (లోపల_Pct && (std::అంకెనా(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } లేకపోతే ఒకవేళ (లోపల_Pct) {
                    లోపల_Pct = అబద్ధం;
                    fmt += ch;
                    ch = std::tolower(ch);
                    ఎంపిక (ch) {
                    స్థితి '%': ఆపు;  // %% - still %%
                    స్థితి 'm': ఆపు;  // %m - still %m - auto insert "name"
                    స్థితి 'l': ఆపు;  // %l - still %l - auto insert "library"
                    స్థితి 't':  // %t - don't know $timeformat so can't constify
                        ఒకవేళ (argp) argp = argp->nextp();
                        ఆపు;
                    అప్రమేయ:  // Most operators, just move to next argument
                        ఒకవేళ (argp) {
                            Ast_కణుపు* స్థిరరాశి nextp = argp->nextp();
                            AstSFormatArg* స్థిరరాశి fargp = VN_CAST(argp, SFormatArg);
                            Ast_కణుపు* స్థిరరాశి subargp = fargp ? fargp->exprp() : argp;
                            స్థిరరాశి VFormatAttr formatAttr
                                = argp
                                      ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                                      : VFormatAttr{};
                            ఒకవేళ (VN_IS(subargp, Const)) {  // Convert it
                                స్థిరరాశి string బహిర్గతం
                                    = formatAttr.isEnum()
                                          ? const_అంకె_V(subargp).displayedEnum(fargp, fmt)
                                          : const_అంకె_V(subargp).displayed(nodep, fmt, formatAttr);
                                UINFO(9, "     DispConst: " << fmt << " -> " << బహిర్గతం << "  for "
                                                            << subargp);
                                // fmt = out w/ replace % with %% as it must later when
                                // used as a format output as literal.
                                fmt = VString::quotePercent(బహిర్గతం);
                                VL_DO_DANGLING(నెట్టు_Deletep(argp->unlink_Fr_వెనుక()), argp);
                            }
                            argp = nextp;
                        }
                        ఆపు;
                    }  // switch
                    newFormat += fmt;
                } లేకపోతే {
                    newFormat += ch;
                }
            }
            ఒకవేళ (newFormat != nodep->పాఠ్యం()) {
                nodep->పాఠ్యం(newFormat);
                UINFO(9, "  Display out " << nodep);
            }
        }
        ఒకవేళ (!nodep->exprsp() && nodep->పాఠ్యం().కనుగొను('%') == string::npos && !nodep->hidden()) {
            // Just a simple constant string - the formatting is pointless
            UINFO(9, "replaceConstStr");
            VL_DO_DANGLING(replaceConstString(nodep, nodep->పాఠ్యం()), nodep);
            తిరిగి;
        }
        ఒకవేళ (VN_IS(nodep->backp(), కణుపు_కేటాయించు)  // Can't remove under Display etc
            && nodep->పాఠ్యం() == "%s" && VN_IS(nodep->exprsp(), SFormatArg)
            && VN_AS(nodep->exprsp(), SFormatArg)->formatAttr().isString()
            && !nodep->exprsp()->nextp()) {
            UINFO(9, "  Display(%p, expr) -> constant expr " << nodep);
            nodep->replaceWith(VN_AS(nodep->exprsp(), SFormatArg)->exprp()->unlink_Fr_వెనుక());
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }
    }
    శూన్య సందర్శించు(AstSFormat* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (!m_doNConst) తిరిగి;
        ఒకవేళ (m_doNConst) {
            // If it's a constant string written to a string variable, inline it as an assignment
            AstSFormatF* స్థిరరాశి fmtp = nodep->fmtp();
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp();
            AstBasicDType* స్థిరరాశి basicp = VN_CAST(lhsp->dtypep()->skipRefp(), BasicDType);
            ఒకవేళ (basicp && basicp->isString() && !fmtp->exprsp()
                && fmtp->పాఠ్యం().కనుగొను('%') == string::npos) {
                AstConst* స్థిరరాశి strp
                    = కొత్త AstConst{fmtp->fileline(), AstConst::String{}, fmtp->పాఠ్యం()};
                lhsp->unlink_Fr_వెనుక();
                nodep->replaceWith(కొత్త Ast_కేటాయించు{nodep->fileline(), lhsp, strp});
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి;
            }
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        VL_RESTORER(m_underRecFunc);
        ఒకవేళ (nodep->recursive()) m_underRecFunc = నిజం;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        iterateChildren(nodep);
        m_hasJumpDelay = నిజం;  // As don't analyze inside tasks for timing controls
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        // Note excludes AstFuncRef as other visitor below
        iterateChildren(nodep);
        m_hasJumpDelay = నిజం;  // As don't analyze inside tasks for timing controls
    }
    శూన్య సందర్శించు(AstFuncRef* nodep) override {
        సందర్శించు(static_cast<Ast_కణుపు_FTask_Ref*>(nodep));
        ఒకవేళ (m_params) {  // Only parameters force us to do constant function call propagation
            replaceWithSimulation(nodep);
        }
    }
    శూన్య సందర్శించు(AstArg* nodep) override {
        // replaceWithSimulation on the Arg's parent FuncRef replaces these
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstLoop* nodep) override {
        VL_RESTORER(m_hasLoopTest);
        స్థిరరాశి తార్కిక oldHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = అబద్ధం;
        m_hasLoopTest = అబద్ధం;
        iterateChildren(nodep);
        స్థిరరాశి తార్కిక thisLoopHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = thisLoopHasJumpDelay || oldHasJumpDelay;
        // If the first statement always break, the loop is useless
        ఒకవేళ (స్థిరరాశి AstLoopTest* స్థిరరాశి testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            ఒకవేళ (testp->condp()->isZero() && (!testp->nextp() || canDeleteStatements())) {
                nodep->v3warn(UNUSEDLOOP, "Loop condition is always false");
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                తిరిగి;
            }
        }
        // If last statement always breaks, repalce loop with body
        ఒకవేళ (Ast_కణుపు* lastp = nodep->stmtsp()) {
            అయితే (Ast_కణుపు* స్థిరరాశి nextp = lastp->nextp()) lastp = nextp;
            ఒకవేళ (AstLoopTest* స్థిరరాశి testp = VN_CAST(lastp, LoopTest)) {
                ఒకవేళ (testp->condp()->isZero()) {
                    VL_DO_DANGLING2(నెట్టు_Deletep(testp->unlink_Fr_వెనుక()), testp, lastp);
                    nodep->replaceWith(nodep->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి;
                }
            }
        }
        // Warn on infinite loop
        ఒకవేళ (!m_hasLoopTest && !thisLoopHasJumpDelay) {
            nodep->v3warn(INFINITELOOP, "Infinite loop (condition always true)");
            nodep->fileline()->modifyWarnOff(V3_Error_సంకేతం::INFINITELOOP, నిజం);  // Complain once
        }
    }
    శూన్య సందర్శించు(AstLoopTest* nodep) override {
        iterateChildren(nodep);

        // If never breaks, remove
        ఒకవేళ (nodep->condp()->isNeqZero()) {
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }

        m_hasLoopTest = నిజం;

        // If always breaks, subsequent statements are dead code, delete them
        ఒకవేళ (nodep->condp()->isZero()) {
            Ast_కణుపు* స్థిరరాశి nextp = nodep->nextp();
            ఒకవేళ (nextp && canDeleteStatements()) {
                VL_DO_DANGLING(నెట్టు_Deletep(nextp->unlink_Fr_వెనుక_With_తదుపరి()), nodep);
            }
            తిరిగి;
        }

        ఒకవేళ (నిర్వాహ్యం_Bool_జరుపు(nodep->condp())) replace_Bool_జరుపు(nodep->condp());
    }

    శూన్య సందర్శించు(AstConsArray* nodep) override { iterateChildren(nodep); }
    శూన్య సందర్శించు(AstConsStruct* nodep) override { iterateChildren(nodep); }
    శూన్య సందర్శించు(Ast_Cons_Struct_అంశం* nodep) override { iterateChildren(nodep); }
    శూన్య సందర్శించు(Ast_Cons_Array_అంశం* nodep) override { iterateChildren(nodep); }
    శూన్య సందర్శించు(AstUnbounded* nodep) override { iterateChildren(nodep); }
    // These are converted by V3Param.  Don't constify as we don't want the
    // from() VARREF to disappear, if any.
    // If output of a presel didn't get consted, chances are V3Param didn't visit properly
    శూన్య సందర్శించు(Ast_కణుపు_Pre_Sel*) override {}

    // Ignored, can eliminate early
    శూన్య సందర్శించు(AstSysIgnore* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (m_doNConst) VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }

    శూన్య సందర్శించు(Ast_Stmt_సమాసం* nodep) override {
        iterateChildren(nodep);
        // Malformed due to child being deleted, remove here
        ఒకవేళ (!nodep->exprp()) {
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }
        // Remove if expression is trivially pure
        ఒకవేళ (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), VarRef)) {
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }
        // Remove calls to empty functions
        ఒకవేళ (స్థిరరాశి AstCCall* స్థిరరాశి callp = VN_CAST(nodep->exprp(), CCall)) {
            స్థిరరాశి AstCFunc* స్థిరరాశి funcp = callp->funcp();
            ఒకవేళ (!callp->argsp() && funcp->ఖాళీలు_Body()) {
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                తిరిగి;
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
    శూన్య సందర్శించు(AstBasicDType* nodep) override {
        iterateChildren(nodep);
        nodep->cvtRangeConst();
    }

    //-----
    // Jump elimination

    శూన్య సందర్శించు(AstJumpGo* nodep) override {
        // Any statements following the JumpGo (at this statement level) never execute, delete
        ఒకవేళ (nodep->nextp() && canDeleteStatements()) {
            నెట్టు_Deletep(nodep->nextp()->unlink_Fr_వెనుక_With_తదుపరి());
        }

        // JumpGo as last statement in target JumpBlock (including last in a last sub-list),
        // is a no-op, remove it.
        ఒకవేళ (!nodep->nextp()) {
            కొరకు (Ast_కణుపు* abovep = nodep->abovep(); abovep; abovep = abovep->abovep()) {
                ఒకవేళ (abovep == nodep->blockp()) {
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                    తిరిగి;
                }
                // Stop if not doing expensive, or if the above node is not the last in its list,
                // ... or if it's not an 'if' TODO: it would be enough if it was not a branch.
                ఒకవేళ (!m_doExpensive || abovep->nextp() || !VN_IS(abovep, If)) ఆపు;
            }
        }
        // Mark JumpBlock as used
        m_వాడిన_Jump_ఖండాలు.emplace(nodep->blockp());
        m_hasJumpDelay = నిజం;
    }

    శూన్య సందర్శించు(Ast_Jump_ఖండం* nodep) override {
        iterateChildren(nodep);

        // If first statement is an AstLoopTest, pull it before the jump block
        ఒకవేళ (AstLoopTest* స్థిరరాశి testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            nodep->కూడు_ఇక్కడ_This_As_తదుపరి(testp->unlink_Fr_వెనుక());
        }

        // Remove if empty
        ఒకవేళ (!nodep->stmtsp()) {
            UINFO(4, "JUMPLABEL => empty " << nodep);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }

        // If no JumpGo points to this node, replace it with its body
        ఒకవేళ (!m_వాడిన_Jump_ఖండాలు.లెక్క(nodep)) {
            UINFO(4, "JUMPLABEL => unused " << nodep);
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }

    //-----
    // Below lines are magic expressions processed by astgen
    //  TREE_SKIP_VISIT("AstNODETYPE")    # Rename normal visit to visitGen and don't iterate
    //-----
    // clang-format off
    వృక్షం_SKIP_సందర్శించు("ArraySel");
    వృక్షం_SKIP_సందర్శించు("CAwait");

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
    //అక్షరశ్రేణులు
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
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        // Default: Just iterate
        ఒకవేళ (m_required) {
            ఒకవేళ (VN_IS(nodep, కణుపు_DType) || VN_IS(nodep, Range) || VN_IS(nodep, ముక్క_Sel)
                || VN_IS(nodep, Dot) || VN_IS(nodep, పాఠ్యం_2)) {
                // ignore
            } లేకపోతే ఒకవేళ (VN_IS(nodep, నమూనా) || VN_IS(nodep, PatMember)) {
                // A parameter override pattern is typed later, so only fold its members
                iterateChildren(nodep);
            } లేకపోతే ఒకవేళ (AstCellRef* స్థిరరాశి crp = VN_CAST(nodep, CellRef)) {
                iterate(crp->exprp());
                ఒకవేళ (Ast_కణుపు* స్థిరరాశి newp = crp->exprp()) {
                    crp->replaceWithKeepDType(newp->unlink_Fr_వెనుక());
                    VL_DO_DANGLING(నెట్టు_Deletep(crp), crp);
                }
                తిరిగి;
            } లేకపోతే {
                nodep->v3error("Expecting expression to be constant, but can't convert a "
                               << nodep->prettyTypeName() << " to constant.");
            }
        } లేకపోతే {
            ఒకవేళ (nodep->isTimingControl()) m_hasJumpDelay = నిజం;
            // Calculate the width of this operation
            ఒకవేళ (m_params && !nodep->వెడల్పు()) nodep = V3_వెడల్పు::వెడల్పు_Params_Edit(nodep);
            iterateChildren(nodep);
        }
    }

బహిరంగ:
    // Processing Mode Enum
    గణన Proc_బహుళకం : uint8_t {
        PROC_PARAMS_NOWARN,
        PROC_PARAMS,
        PROC_GENERATE,
        PROC_సజీవ,
        PROC_V_WARN,
        PROC_V_NOWARN,
        PROC_V_EXPENSIVE,
        PROC_CPP
    };

    // CONSTRUCTORS
    ConstVisitor(Proc_బహుళకం pmode, తార్కిక globalPass)
        : m_globalPass{globalPass}
        , m_concswap_పేర్లు{globalPass ? ("__Vconcswap_" + cvt_గమ్యం_Str(s_global_Pass_అంకె++)) : ""} {
        // clang-format off
        ఎంపిక (pmode) {
        స్థితి PROC_PARAMS_NOWARN:  m_doV = నిజం;  m_doNConst = నిజం; m_params = నిజం;
                                  m_required = అబద్ధం; ఆపు;
        స్థితి PROC_PARAMS:         m_doV = నిజం;  m_doNConst = నిజం; m_params = నిజం;
                                  m_required = నిజం; ఆపు;
        స్థితి PROC_GENERATE:       m_doV = నిజం;  m_doNConst = నిజం; m_params = నిజం;
                                  m_required = నిజం; m_doGenerate = నిజం; ఆపు;
        స్థితి PROC_సజీవ:           ఆపు;
        స్థితి PROC_V_WARN:         m_doV = నిజం;  m_doNConst = నిజం; m_warn = నిజం; m_convert_Logic_గమ్యం_బిట్టు = నిజం; ఆపు;
        స్థితి PROC_V_NOWARN:       m_doV = నిజం;  m_doNConst = నిజం; ఆపు;
        స్థితి PROC_V_EXPENSIVE:    m_doV = నిజం;  m_doNConst = నిజం; m_doExpensive = నిజం; ఆపు;
        స్థితి PROC_CPP:            m_doV = అబద్ధం; m_doNConst = నిజం; m_doCpp = నిజం; ఆపు;
        అప్రమేయ:                  v3fatalSrc("Bad case"); ఆపు;
        }
        // clang-format on
    }
    ~ConstVisitor() override {
        ఒకవేళ (m_doCpp) {
            ఒకవేళ (m_globalPass) {
                V3Stats::కూడు_Stat("Optimizations, Const bit op reduction", m_stat_బిట్టు_Op_Reduction);
            } లేకపోతే {
                V3Stats::కూడు_Stat_కూడిక("Optimizations, Const bit op reduction", m_stat_బిట్టు_Op_Reduction);
            }
        }
        V3Stats::కూడు_Stat_కూడిక("Optimizations, Cond redundant expressions", m_stat_Cond_సమాసం_Redundant);
        V3Stats::కూడు_Stat_కూడిక("Optimizations, If cond redundant expressions",
                            m_stat_If_Cond_సమాసం_Redundant);
        V3Stats::కూడు_Stat_కూడిక("Optimizations, Concat merges", m_stat_Concat_విలీనం);
        V3Stats::కూడు_Stat_కూడిక("Optimizations, Member access predicate node visits",
                            m_statMemberAccessVisits);
    }

    Ast_కణుపు* mainAcceptEdit(Ast_కణుపు* nodep) {
        VIsCached::clear_Cache_వృక్షం();  // Avoid using any stale isPure
        // Operate starting at a random place
        తిరిగి iterateSubtreeReturnEdits(nodep);
    }
};

uint32_t ConstVisitor::s_global_Pass_అంకె = 0;

//######################################################################
// Const class functions

//! Force this cell node's parameter list to become a constant
శూన్య V3Const::constifyParamsEdit(Ast_కణుపు* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3_వెడల్పు::వెడల్పు_Params_Edit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS, /* globalPass: */ అబద్ధం};
    // cppcheck-suppress constVariablePointer // edited below
    ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ఒకవేళ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } లేకపోతే {
        (శూన్య)visitor.mainAcceptEdit(nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump if have next line
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

//! Constify this cell node's parameter list if possible
శూన్య V3Const::constifyParamsNoWarnEdit(Ast_కణుపు* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3_వెడల్పు::వెడల్పు_Params_Edit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS_NOWARN, /* globalPass: */ అబద్ధం};
    // cppcheck-suppress constVariablePointer // edited below
    ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ఒకవేళ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } లేకపోతే {
        (శూన్య)visitor.mainAcceptEdit(nodep);
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
శూన్య V3Const::constifyGenerateParamsEdit(Ast_కణుపు* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE:");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized
    // yet.

    // Make sure we've sized everything first
    nodep = V3_వెడల్పు::వెడల్పు_Generate_Params_Edit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_GENERATE, /* globalPass: */ అబద్ధం};
    // cppcheck-suppress constVariablePointer // edited below
    ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ఒకవేళ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } లేకపోతే {
        VL_DO_DANGLING(visitor.mainAcceptEdit(nodep), nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump this.
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

శూన్య V3Const::constifyAllLint(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_WARN, /* globalPass: */ నిజం};
        (శూన్య)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("const", 0, dump_వృక్షం_Either_Level() >= 3);
}

శూన్య V3Const::constifyCpp(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ నిజం};
        (శూన్య)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("const_cpp", 0, dump_వృక్షం_Either_Level() >= 3);
}

Ast_కణుపు* V3Const::constifyEdit(Ast_కణుపు* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_NOWARN, /* globalPass: */ అబద్ధం};
    nodep = visitor.mainAcceptEdit(nodep);
    తిరిగి nodep;
}

Ast_కణుపు* V3Const::constifyEditCpp(Ast_కణుపు* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ అబద్ధం};
    nodep = visitor.mainAcceptEdit(nodep);
    తిరిగి nodep;
}

శూన్య V3Const::constify_All_సజీవ(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    // This only pushes constants up, doesn't make any other edits
    // IE doesn't prune dead statements, as we need to do some usability checks after this
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_సజీవ, /* globalPass: */ నిజం};
        (శూన్య)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("const", 0, dump_వృక్షం_Either_Level() >= 3);
}

శూన్య V3Const::constifyAll(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ నిజం};
        (శూన్య)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("const", 0, dump_వృక్షం_Either_Level() >= 3);
}

Ast_కణుపు* V3Const::constifyExpensiveEdit(Ast_కణుపు* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ అబద్ధం};
    nodep = visitor.mainAcceptEdit(nodep);
    తిరిగి nodep;
}
