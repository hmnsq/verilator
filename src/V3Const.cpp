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

#define वृक्ष_SKIP_VISIT(...)
#define TREEOP1(...)
#define TREEOPA(...)
#define TREEOP(...)
#define TREEOPS(...)
#define TREEOPC(...)
#define TREEOPV(...)

//######################################################################
// Utilities

स्थिर तार्किक isConst(स्थिरांक Ast_गाठ* nodep, uint64_t v) {
    स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep, Const);
    परत constp && constp->toUQuad() == v;
}

ढाँचा <typename T>
स्थिर typename std::enable_if<std::is_integral<T>::मूल्य, तार्किक>::type isPow2(T val) {
    परत (val & (val - 1)) == 0;
}

स्थिर पूर्णांक मोजणी_Trailing_Zeroes(uint64_t val) {
    UASSERT(val, "countTrailingZeroes argument must be non-zero");
#जर defined(__GNUC__) && !defined(VL_NO_BUILTINS)
    परत __builtin_ctzll(val);
#अन्यथा
    पूर्णांक bit = 0;
    val = ~val;
    पर्यंत (val & 1) {
        ++bit;
        val >>= 1;
    }
    परत bit;
#endif
}

// Allocates a unique id for each AstNode it is applied to, held in user4.
वर्ग VNIdAllocator final {
    // NODE STATE
    // AstNode::user4p -> size_t. Unique id of the node (0/nullptr if not allocated yet)

    // MEMBERS
    // Claimed lazily on first use TODO: fix conflict with V3Param user4 slot
    std::unique_ptr<VNUser4InUse> m_inuser4p;
    माप_प्रकार m_nextId = 0;  // Id allocated most recently

सार्वजनिक:
    // Return the unique id of the given node, allocating a new one if it has none yet
    माप_प्रकार चालक()(Ast_गाठ* nodep) {
        जर (!m_inuser4p) m_inuser4p = std::make_unique<VNUser4InUse>();  // Claim on first use
        जर (!nodep->user4p()) nodep->user4p(reinterpret_cast<शून्य*>(++m_nextId));
        परत reinterpret_cast<माप_प्रकार>(nodep->user4p());
    }
};

// This visitor can be used in the post-expanded Ast from V3Expand, where the Ast satisfies:
// - Constants are 64 bit at most (because words are accessed via AstWordSel)
// - Variables are scoped.
वर्ग Const_Bit_Op_वृक्ष_Visitor final : सार्वजनिक VNVisitorConst {
    // TYPES

    // Holds a node to be added as a term in the reduction tree, it's equivalent op count, and a
    // bool indicating if the term is clean (0/1 value, or if the top bits might be dirty)
    using ResultTerm = std::tuple<Ast_गाठ_अभिव्यक्ती*, अचिन्हित, तार्किक>;

    वर्ग LeafInfo final {  // Leaf node (either AstConst or AstVarRef)
        // MEMBERS
        तार्किक m_polarity = सत्य;  // Invert result due to NOT
        पूर्णांक m_lsb = 0;  // LSB of actually used bit of m_refp->varp()
        पूर्णांक m_msb = 0;  // MSB of actually used bit of m_refp->varp()
        पूर्णांक m_wordIdx = -1;  // -1 means AstWordSel is not used.
        AstVarRef* m_refp = nullptr;  // Leaf's variable reference
        स्थिरांक AstConst* m_constp = nullptr;  // Leaf's constant

    सार्वजनिक:
        // CONSTRUCTORS
        LeafInfo() = पूर्वनिर्धारित;
        LeafInfo(स्थिरांक LeafInfo& other) = पूर्वनिर्धारित;
        LeafInfo& चालक=(स्थिरांक LeafInfo& other) = पूर्वनिर्धारित;
        LeafInfo(LeafInfo&& other) = पूर्वनिर्धारित;
        LeafInfo& चालक=(LeafInfo&& other) = पूर्वनिर्धारित;
        explicit LeafInfo(पूर्णांक lsb)
            : m_lsb{lsb} {}

        // METHODS
        शून्य setLeaf(AstVarRef* refp) {
            UASSERT_OBJ(!m_refp && !m_constp, refp, "Must be called just once");
            m_refp = refp;
            m_msb = refp->varp()->widthMin() - 1;
        }
        शून्य setLeaf(स्थिरांक AstConst* constp) {
            UASSERT_OBJ(!m_refp && !m_constp, constp, "Must be called just once");
            m_constp = constp;
            m_msb = constp->widthMin() - 1;
        }
        // updateBitRange(), limitBitRangeToLsb(), and polarity() must be called during ascending
        // back to the root.
        शून्य updateBitRange(पूर्णांक newLsb, पूर्णांक newMsb) {
            जर ((m_lsb <= m_msb && newLsb > newMsb) || (m_lsb > m_msb && m_lsb < newLsb)) {
                // When the new bit range is out of m_refp, clear polarity because nodes below is
                // shifted out to zero.
                // This kind of clear may happen several times. e.g. (!(1'b1 >> 1)) >> 1
                polarity(सत्य);
            }
            m_lsb = newLsb;
            m_msb = newMsb;
        }
        शून्य updateBitRange(स्थिरांक AstCCast* castp) {
            updateBitRange(m_lsb, std::min(m_msb, m_lsb + castp->width() - 1));
        }
        शून्य updateBitRange(स्थिरांक AstShiftR* shiftp) {
            updateBitRange(m_lsb + VN_AS(shiftp->rhsp(), Const)->toUInt(), m_msb);
        }
        शून्य limitBitRangeToLsb() { updateBitRange(m_lsb, std::min(m_msb, m_lsb)); }
        पूर्णांक wordIdx() स्थिरांक { परत m_wordIdx; }
        शून्य wordIdx(पूर्णांक i) { m_wordIdx = i; }
        तार्किक polarity() स्थिरांक { परत m_polarity; }
        शून्य polarity(तार्किक p) { m_polarity = p; }

        AstVarRef* refp() स्थिरांक { परत m_refp; }
        स्थिरांक AstConst* constp() स्थिरांक { परत m_constp; }
        तार्किक missingWordSel() स्थिरांक {
            // When V3Expand is skipped, WordSel is not inserted.
            परत m_refp->isWide() && m_wordIdx == -1;
        }
        पूर्णांक lsb() स्थिरांक { परत m_lsb; }

        पूर्णांक msb() स्थिरांक { परत std::min(m_msb, varWidth() - 1); }
        पूर्णांक varWidth() स्थिरांक {
            UASSERT(m_refp, "m_refp should be set");
            स्थिरांक पूर्णांक width = m_refp->varp()->widthMin();
            जर (!m_refp->isWide()) {
                UASSERT_OBJ(m_wordIdx == -1, m_refp, "Bad word index into non-wide");
                परत width;
            } अन्यथा {
                जर (missingWordSel()) परत width;
                UASSERT_OBJ(m_wordIdx >= 0, m_refp, "Bad word index into wide");
                स्थिरांक पूर्णांक bitsInMSW = VL_BITBIT_E(width) ? VL_BITBIT_E(width) : VL_EDATASIZE;
                परत m_wordIdx == m_refp->width_शब्द() - 1 ? bitsInMSW : VL_EDATASIZE;
            }
        }
    };

    संरचना BitPolarityEntry final {  // Found bit polarity during iterate()
        LeafInfo m_info;  // Leaf (variable or constant) bit polarity was found on
        तार्किक m_polarity = असत्य;  // Polarity the bit must have to match
        पूर्णांक m_bit = 0;  // Bit index within the leaf that was tested
        BitPolarityEntry(स्थिरांक LeafInfo& info, तार्किक pol, पूर्णांक bit)
            : m_info{info}
            , m_polarity{pol}
            , m_bit{bit} {}
        BitPolarityEntry() = पूर्वनिर्धारित;
    };

    संरचना Frozen_गाठ_Info final {  // Context when a frozen node is found
        तार्किक m_polarity;  // Polarity the frozen node must match
        पूर्णांक m_lsb;  // LSB position of the frozen node
        तार्किक चालक<(स्थिरांक Frozen_गाठ_Info& other) स्थिरांक {
            जर (m_lsb != other.m_lsb) परत m_lsb < other.m_lsb;
            परत m_polarity < other.m_polarity;
        }
    };

    वर्ग Restorer final {  // Restore the original state unless disableRestore() is called
        Const_Bit_Op_वृक्ष_Visitor& m_visitor;  // Visitor whose state is saved and restored
        स्थिरांक माप_प्रकार m_polaritiesSize;  // Saved m_visitor.m_bitPolarities size to truncate back to
        स्थिरांक माप_प्रकार m_frozenSize;  // Saved m_visitor.m_frozenNodes size to truncate back to
        स्थिरांक अचिन्हित m_ops;  // Saved m_visitor.m_ops to restore
        स्थिरांक तार्किक m_polarity;  // Saved m_visitor.m_polarity to restore
        तार्किक m_restore = सत्य;  // Whether the destructor still needs to restore state

    सार्वजनिक:
        explicit Restorer(Const_Bit_Op_वृक्ष_Visitor& visitor)
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
            जर (m_restore) restoreNow();
        }
        शून्य disableRestore() { m_restore = असत्य; }
        शून्य restoreNow() {
            UASSERT(m_restore, "Can be called just once");
            m_visitor.m_bitPolarities.resize(m_polaritiesSize);
            m_visitor.m_frozenNodes.resize(m_frozenSize);
            m_visitor.m_ops = m_ops;
            m_visitor.m_polarity = m_polarity;
            m_restore = असत्य;
        }
    };
    // Collect information for each Variable to transform as below
    वर्ग VarInfo final {
        // MEMBERS
        पूर्णांक m_knownResult = -1;  // -1: result is not known, 0 or 1: result of this tree
        स्थिरांक Const_Bit_Op_वृक्ष_Visitor* स्थिरांक
            m_parentp;  // ConstBitOpTreeVisitor holding this VarInfo
        AstVarRef* स्थिरांक m_refp;  // Points the variable that this VarInfo covers
        स्थिरांक पूर्णांक m_width;  // Width of term this VarInfo refers to
        V3_संख्या m_bitPolarity;  // Coefficient of each bit

    सार्वजनिक:
        // METHODS
        तार्किक hasConstResult() स्थिरांक { परत m_knownResult >= 0 || m_bitPolarity.isAllX(); }
        // The constant result. Only valid if hasConstResult() returned true.
        तार्किक getConstResult() स्थिरांक {
            // Note that this condition covers m_knownResult == -1 but m_bitPolarity.isAllX(),
            // in which case the result is 0
            परत m_knownResult == 1;
        }
        स्थिरांक AstVarRef* refp() स्थिरांक { परत m_refp; }
        तार्किक sameVarAs(स्थिरांक Ast_गाठ_Var_Ref* otherp) स्थिरांक { परत m_refp->same_गाठ(otherp); }
        शून्य setPolarity(तार्किक compBit, पूर्णांक bit) {
            // Ignore if already determined a known reduction
            जर (m_knownResult >= 0) परत;
            UASSERT_OBJ(bit < m_width, m_refp,
                        "Bit index out of range: " << bit << " width: " << m_width);
            जर (m_bitPolarity.bitIsX(bit)) {  // The bit is not yet marked with either polarity
                m_bitPolarity.setBit(bit, compBit);
            } अन्यथा {  // The bit has already been marked with some polarity
                स्थिरांक तार्किक sameFlag = m_bitPolarity.bitIs1(bit) == compBit;
                जर (m_parentp->is_Xor_वृक्ष()) {
                    UASSERT_OBJ(compBit && sameFlag, m_refp, "Only true is set in Xor tree");
                    // a ^ a ^ b == b so we can ignore a
                    m_bitPolarity.setBit(bit, 'x');
                } अन्यथा {  // And, Or
                    // Can ignore this nodep as the bit is already marked with the same polarity
                    जर (sameFlag) परत;  // a & a == a, b | b == b
                    // Otherwise result is constant (a & ~a == 0) or (a | ~a == 1)
                    m_knownResult = m_parentp->is_And_वृक्ष() ? 0 : 1;
                    m_bitPolarity.setAllBitsX();  // The variable is not referred anymore
                }
            }
        }

        // Return reduction term for this VarInfo, together with the number of ops in the term,
        // and a boolean indicating if the term is clean (1-bit vs multi-bit value)
        ResultTerm getResultTerm() स्थिरांक {
            UASSERT_OBJ(!hasConstResult(), m_refp, "getTerm on reduction that yields constant");
            File_ओळ* स्थिरांक fl = m_refp->fileline();

            // Get the term we are referencing (the WordSel, if wide, otherwise just the VarRef)
            Ast_गाठ_अभिव्यक्ती* srcp = VN_CAST(m_refp->backp(), WordSel);
            जर (!srcp) srcp = m_refp;
            srcp = srcp->clone_वृक्ष(असत्य);

            // Signed variables might have redundant sign bits that need masking.
            स्थिरांक तार्किक hasRedundantSignBits
                = m_refp->varp()->dtypep()->isSigned()
                  && (m_refp->isWide() ? (m_width != VL_EDATASIZE)
                                       : (m_width < 8 || !isPow2(m_width)));

            // Get the mask that selects the bits that are relevant in this term
            V3_संख्या mask_अंक{srcp, m_width, 0};
            mask_अंक.opBitsNonXZ(m_bitPolarity);  // 'x' -> 0, 0->1, 1->1
            स्थिरांक uint64_t maskVal = mask_अंक.toUQuad();
            UASSERT_OBJ(maskVal != 0, m_refp,
                        "Should have been recognized as having const 0 result");

            // Parts of the return value
            Ast_गाठ_अभिव्यक्ती* resultp = srcp;  // The tree for this term
            अचिन्हित ops = 0;  // Number of ops in this term
            तार्किक clean = असत्य;  // Whether the term is clean (has value 0 or 1)

            जर (isPow2(maskVal)) {
                // If we only want a single bit, shift it out instead of a masked compare. Shifts
                // don't go through the flags register on x86 and are hence faster. This is also
                // always fewer or same ops as mask and compare, but with shorter instructions on
                // x86.

                // Find the index of the bit we want.
                स्थिरांक पूर्णांक bit = मोजणी_Trailing_Zeroes(maskVal);
                // If we want something other than the bottom bit, shift it out
                जर (bit != 0) {
                    resultp = नवीन AstShiftR{fl, resultp,
                                            नवीन AstConst{fl, static_cast<uint32_t>(bit)}, m_width};
                    ++ops;
                }
                // Negate it if necessary
                स्थिरांक तार्किक negate = m_bitPolarity.bitIs0(bit);
                जर (negate) {
                    resultp = नवीन AstNot{fl, resultp};
                    ++ops;
                }
                // Clean if MSB of unsigned value, and not negated
                clean = (bit == m_width - 1) && !hasRedundantSignBits && !negate;
            } अन्यथा {
                // We want multiple bits. Go ahead and extract them.

                // Check if masking is required, and if so apply it
                स्थिरांक तार्किक needsMasking = maskVal != VL_MASK_Q(m_width) || hasRedundantSignBits;
                जर (needsMasking) {
                    resultp = नवीन AstAnd{fl, नवीन AstConst{fl, mask_अंक}, resultp};
                    ++ops;
                }

                // Create the sub-expression for this term
                जर (m_parentp->is_Xor_वृक्ष()) {
                    जर (needsMasking) {
                        // Reduce the masked term to the minimum known width,
                        // to use the smallest RedXor formula
                        स्थिरांक पूर्णांक widthMin = mask_अंक.widthToFit();
                        resultp->dtypeChgWidth(widthMin, widthMin);
                    }
                    resultp = नवीन AstRedXor{fl, resultp};
                    ++ops;
                    clean = असत्य;
                    // VL_REDXOR_* returns IData, set width accordingly to avoid unnecessary casts
                    resultp->dtypeChgWidth(VL_IDATASIZE, 1);
                } अन्यथा जर (m_parentp->is_And_वृक्ष()) {
                    V3_संख्या comp_अंक{srcp, m_width, 0};
                    comp_अंक.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    resultp = नवीन AstEq{fl, नवीन AstConst{fl, comp_अंक}, resultp};
                    ++ops;
                    clean = सत्य;
                } अन्यथा {  // Or
                    V3_संख्या comp_अंक{srcp, m_width, 0};
                    comp_अंक.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    comp_अंक.opXor(V3_संख्या{comp_अंक}, mask_अंक);
                    resultp = नवीन AstNeq{fl, नवीन AstConst{fl, comp_अंक}, resultp};
                    ++ops;
                    clean = सत्य;
                }
            }

            परत ResultTerm{resultp, ops, clean};
        }

        // CONSTRUCTORS
        VarInfo(Const_Bit_Op_वृक्ष_Visitor* parent, AstVarRef* refp, पूर्णांक width)
            : m_parentp{parent}
            , m_refp{refp}
            , m_width{width}
            , m_bitPolarity{refp, m_width} {
            m_bitPolarity.setAllBitsX();
        }
    };

    // MEMBERS
    तार्किक m_failed = असत्य;
    तार्किक m_polarity = सत्य;  // Flip when AstNot comes
    अचिन्हित m_ops;  // Number of operations such as And, Or, Xor, Sel...
    पूर्णांक m_lsb = 0;  // Current LSB
    LeafInfo* m_leafp = nullptr;  // AstConst or AstVarRef that currently looking for
    स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक m_rootp;  // Root of this AST subtree

    std::vector<std::pair<Ast_गाठ_अभिव्यक्ती*, Frozen_गाठ_Info>>
        m_frozenNodes;  // Nodes that cannot be optimized
    std::vector<BitPolarityEntry> m_bitPolarities;  // Polarity of bits found during iterate()
    std::vector<std::unique_ptr<VarInfo>> m_varInfos;  // VarInfo for each variable, [0] is nullptr
    VNIdAllocator& m_ids;  // Node id allocator, owned by ConstVisitor
    // Base index of m_varInfos that points VarInfo, keyed by AstVarScope/AstVarRef id,
    // zero means not set yet.
    std::unordered_map<माप_प्रकार, पूर्णांक> m_baseIdxs;

    // METHODS

    तार्किक is_And_वृक्ष() स्थिरांक { परत VN_IS(m_rootp, And); }
    तार्किक is_Or_वृक्ष() स्थिरांक { परत VN_IS(m_rootp, Or); }
    तार्किक is_Xor_वृक्ष() स्थिरांक { परत VN_IS(m_rootp, Xor) || VN_IS(m_rootp, RedXor); }

#define CONST_BITOP_RETURN_IF(cond, nodep) \
    जर (setFailed(cond, #cond, nodep, __LINE__)) परत

#define CONST_BITOP_SET_FAILED(reason, nodep) setFailed(सत्य, reason, nodep, __LINE__)

    तार्किक setFailed(तार्किक fail, स्थिरांक अक्षर* reason, स्थिरांक Ast_गाठ* nodep, पूर्णांक ओळ) {
        जर (fail && !m_failed) {
            UINFO(9, "cannot optimize " << m_rootp << " reason:" << reason << " called from line:"
                                        << ओळ << " when checking:" << nodep);
            // UINFOTREE(9, m_rootp, "", "root");
            m_failed = सत्य;
        }
        परत m_failed;
    }
    शून्य incrOps(स्थिरांक Ast_गाठ* nodep, पूर्णांक ओळ) {
        ++m_ops;
        UINFO(9, "Increment to " << m_ops << " " << nodep << " called from line " << ओळ);
    }
    VarInfo& getVarInfo(स्थिरांक LeafInfo& ref) {
        UASSERT_OBJ(ref.refp(), m_rootp, "null varref in And/Or/Xor optimization");
        Ast_गाठ* nodep = ref.refp()->varScopep();
        जर (!nodep) nodep = ref.refp()->varp();  // Not scoped
        पूर्णांक& baseIdxr = m_baseIdxs[m_ids(nodep)];
        जर (baseIdxr == 0) {  // Not set yet
            baseIdxr = m_varInfos.size();
            स्थिरांक पूर्णांक अंक_शब्द
                = ref.refp()->dtypep()->isWide() ? ref.refp()->dtypep()->width_शब्द() : 1;
            m_varInfos.resize(m_varInfos.size() + अंक_शब्द);
        }
        स्थिरांक माप_प्रकार idx = baseIdxr + std::max(0, ref.wordIdx());
        VarInfo* varInfop = m_varInfos[idx].get();
        जर (!varInfop) {
            varInfop = नवीन VarInfo{हे, ref.refp(), ref.varWidth()};
            m_varInfos[idx].reset(varInfop);
            जर (ref.missingWordSel()) {
                // ConstBitOpTreeVisitor makes some constants for masks and its type is uint64_t.
                // That's why V3Expand, that inserts WordSel, is needed.
                CONST_BITOP_SET_FAILED("V3Expand is skipped", ref.refp());
            }
        } अन्यथा {
            जर (!varInfop->sameVarAs(ref.refp()))
                CONST_BITOP_SET_FAILED("different var (scope?)", ref.refp());
        }
        परत *varInfop;
    }

    // Traverse down to see AstConst or AstVarRef
    LeafInfo findLeaf(Ast_गाठ* nodep, तार्किक expectConst) {
        जर (!nodep->dtypep()->skipRefp()->isIntegralOrPacked()) परत LeafInfo{};
        LeafInfo info{m_lsb};
        {
            VL_RESTORER(m_leafp);
            m_leafp = &info;
            iterateConst(nodep);
        }

        तार्किक ok = !m_failed;
        जर (expectConst) {
            ok &= !info.refp() && info.constp();
        } अन्यथा {
            ok &= info.refp() && !info.constp();
        }
        परत ok ? info : LeafInfo{};
    }

    // VISITORS
    शून्य visit(Ast_गाठ* nodep) override { CONST_BITOP_SET_FAILED("Hit unexpected op", nodep); }
    शून्य visit(AstCCast* nodep) override {
        iterateChildrenConst(nodep);
        जर (m_leafp) m_leafp->updateBitRange(nodep);
    }
    शून्य visit(AstShiftR* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->rhsp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->rhsp());
        m_lsb += constp->toUInt();
        incrOps(nodep, __LINE__);
        iterateConst(nodep->lhsp());
        m_leafp->updateBitRange(nodep);
        m_lsb -= constp->toUInt();
    }
    शून्य visit(AstNot* nodep) override {
        CONST_BITOP_RETURN_IF(nodep->widthMin() != 1, nodep);
        Ast_गाठ* lhsp = nodep->lhsp();
        AstCCast* स्थिरांक castp = VN_CAST(lhsp, CCast);
        जर (castp) lhsp = castp->lhsp();
        CONST_BITOP_RETURN_IF(!is_Xor_वृक्ष() && !VN_IS(lhsp, VarRef) && !VN_IS(lhsp, ShiftR), lhsp);
        incrOps(nodep, __LINE__);
        m_polarity = !m_polarity;
        iterateChildrenConst(nodep);
        // Don't restore m_polarity for Xor as it counts parity of the entire tree
        जर (!is_Xor_वृक्ष()) m_polarity = !m_polarity;
        जर (m_leafp && castp) m_leafp->updateBitRange(castp);
        जर (m_leafp) m_leafp->polarity(!m_leafp->polarity());
    }
    शून्य visit(AstWordSel* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->bitp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->bitp());
        UASSERT_OBJ(m_leafp->wordIdx() == -1, nodep, "Unexpected nested WordSel");
        m_leafp->wordIdx(constp->toSInt());
        iterateConst(nodep->fromp());
    }
    शून्य visit(AstVarRef* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }
    शून्य visit(AstConst* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }

    शून्य visit(AstRedXor* nodep) override {
        Restorer restorer{*हे};
        CONST_BITOP_RETURN_IF(!VN_IS(m_rootp, Xor), nodep);
        Ast_गाठ* lhsp = nodep->lhsp();
        स्थिरांक AstCCast* स्थिरांक castp = VN_CAST(lhsp, CCast);
        जर (castp) lhsp = castp->lhsp();
        जर (स्थिरांक AstAnd* स्थिरांक andp = VN_CAST(lhsp, And)) {  // '^(mask & leaf)'
            CONST_BITOP_RETURN_IF(!andp, lhsp);

            स्थिरांक LeafInfo& mask = findLeaf(andp->lhsp(), सत्य);
            CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

            LeafInfo ref = findLeaf(andp->rhsp(), असत्य);
            CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());
            जर (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all subtree succeeded

            स्थिरांक V3_संख्या& mask_अंक = mask.constp()->अंक();

            incrOps(nodep, __LINE__);
            incrOps(andp, __LINE__);

            // Mark all bits checked in this reduction
            स्थिरांक पूर्णांक maxBitIdx = std::min(ref.lsb() + mask_अंक.width(), ref.msb() + 1);
            साठी (पूर्णांक bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                स्थिरांक पूर्णांक maskIdx = bitIdx - ref.lsb();
                जर (mask_अंक.bitIs0(maskIdx)) सुरूठेवा;
                // Set true, m_polarity takes care of the entire parity
                m_bitPolarities.emplace_back(ref, सत्य, bitIdx);
            }
        } अन्यथा {  // '^leaf'
            LeafInfo ref = findLeaf(lhsp, असत्य);
            CONST_BITOP_RETURN_IF(!ref.refp(), lhsp);
            जर (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all checks passed

            incrOps(nodep, __LINE__);

            // Mark all bits checked by this comparison
            साठी (पूर्णांक bitIdx = ref.lsb(); bitIdx <= ref.msb(); ++bitIdx) {
                m_bitPolarities.emplace_back(ref, सत्य, bitIdx);
            }
        }
    }

    शून्य visit(Ast_गाठ_Biop* nodep) override {
        जर (VN_IS(nodep, And) && isConst(nodep->lhsp(), 1)) {  // 1 & _
            // Always reach past a plain making AND
            Restorer restorer{*हे};
            incrOps(nodep, __LINE__);
            iterateConst(nodep->rhsp());
            जर (m_leafp) m_leafp->limitBitRangeToLsb();
            CONST_BITOP_RETURN_IF(m_failed, nodep->rhsp());
            restorer.disableRestore();  // Now all checks passed
        } अन्यथा जर (nodep->type() == m_rootp->type()) {  // And, Or, Xor
            // subtree under NOT can be optimized only in XOR tree.
            CONST_BITOP_RETURN_IF(!m_polarity && !is_Xor_वृक्ष(), nodep);
            incrOps(nodep, __LINE__);
            साठी (स्थिरांक तार्किक उजवा : {असत्य, सत्य}) {
                VL_RESTORER(m_leafp);
                Restorer restorer{*हे};
                LeafInfo leafInfo{m_lsb};
                // cppcheck-suppress danglingLifetime
                m_leafp = &leafInfo;
                Ast_गाठ_अभिव्यक्ती* opp = उजवा ? nodep->rhsp() : nodep->lhsp();
                स्थिरांक तार्किक origFailed = m_failed;
                iterateConst(opp);
                जर (leafInfo.constp() || m_failed) {
                    // Revert changes in leaf
                    restorer.restoreNow();
                    // Reach past a cast then add to frozen nodes to be added to final reduction
                    जर (स्थिरांक AstCCast* स्थिरांक castp = VN_CAST(opp, CCast)) opp = castp->lhsp();
                    स्थिरांक तार्किक pol = is_Xor_वृक्ष() || m_polarity;  // Only AND/OR tree needs polarity
                    UASSERT_OBJ(pol, nodep, "AND/OR tree expects m_polarity==true");
                    m_frozenNodes.emplace_back(opp, Frozen_गाठ_Info{pol, m_lsb});
                    m_failed = origFailed;
                    सुरूठेवा;
                }
                restorer.disableRestore();  // Now all checks passed
                जर (leafInfo.refp()) {
                    // The conditional on the lsb being in range is necessary for some degenerate
                    // case, e.g.: (IData)((QData)wide[0] >> 32), or <1-bit-var> >> 1, which is
                    // just zero
                    जर (leafInfo.lsb() <= leafInfo.msb()) {
                        m_bitPolarities.emplace_back(leafInfo, is_Xor_वृक्ष() || leafInfo.polarity(),
                                                     leafInfo.lsb());
                    } अन्यथा जर ((is_And_वृक्ष() && leafInfo.polarity())
                               || (is_Or_वृक्ष() && !leafInfo.polarity())) {
                        // If there is a constant 0 term in an And tree or 1 term in an Or tree, we
                        // must include it. Fudge this by adding a bit with both polarities, which
                        // will simplify to zero or one respectively.
                        // Note that Xor tree does not need this kind of care, polarity of Xor tree
                        // is already cared when visitin AstNot. Taking xor with 1'b0 is nop.
                        m_bitPolarities.emplace_back(leafInfo, सत्य, 0);
                        m_bitPolarities.emplace_back(leafInfo, असत्य, 0);
                    }
                }
            }
        } अन्यथा जर ((is_And_वृक्ष() && VN_IS(nodep, Eq)) || (is_Or_वृक्ष() && VN_IS(nodep, Neq))) {
            Restorer restorer{*हे};
            CONST_BITOP_RETURN_IF(!m_polarity, nodep);
            CONST_BITOP_RETURN_IF(m_lsb, nodep);  // the result of EQ/NE is 1 bit width
            स्थिरांक Ast_गाठ* lhsp = nodep->lhsp();
            जर (स्थिरांक AstCCast* स्थिरांक castp = VN_CAST(lhsp, CCast)) lhsp = castp->lhsp();
            स्थिरांक AstConst* स्थिरांक constp = VN_CAST(lhsp, Const);
            CONST_BITOP_RETURN_IF(!constp, nodep->lhsp());

            स्थिरांक V3_संख्या& comp_अंक = constp->अंक();

            स्वतः setPolarities = [हे, &comp_अंक](स्थिरांक LeafInfo& ref, स्थिरांक V3_संख्या* maskp) {
                स्थिरांक तार्किक maskFlip = is_And_वृक्ष() ^ ref.polarity();
                पूर्णांक constantWidth = comp_अंक.width();
                जर (maskp) constantWidth = std::max(constantWidth, maskp->width());
                स्थिरांक पूर्णांक maxBitIdx = std::max(ref.lsb() + constantWidth, ref.msb() + 1);
                // Mark all bits checked by this comparison
                साठी (पूर्णांक bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                    स्थिरांक पूर्णांक maskIdx = bitIdx - ref.lsb();
                    स्थिरांक तार्किक mask0 = maskp && maskp->bitIs0(maskIdx);
                    स्थिरांक तार्किक outOfRange = bitIdx > ref.msb();
                    जर (mask0 || outOfRange) {  // RHS is 0
                        जर (comp_अंक.bitIs1(maskIdx)) {
                            // LHS is 1
                            // And tree: 1 == 0 => always false, set v && !v
                            // Or tree : 1 != 0 => always true, set v || !v
                            m_bitPolarities.emplace_back(ref, सत्य, 0);
                            m_bitPolarities.emplace_back(ref, असत्य, 0);
                            थांबा;
                        } अन्यथा {  // This bitIdx is irrelevant
                            सुरूठेवा;
                        }
                    }
                    स्थिरांक तार्किक polarity = comp_अंक.bitIs1(maskIdx) != maskFlip;
                    m_bitPolarities.emplace_back(ref, polarity, bitIdx);
                }
            };

            जर (स्थिरांक AstAnd* स्थिरांक andp = VN_CAST(nodep->rhsp(), And)) {  // comp == (mask & v)
                स्थिरांक LeafInfo& mask = findLeaf(andp->lhsp(), सत्य);
                CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

                स्थिरांक LeafInfo& ref = findLeaf(andp->rhsp(), असत्य);
                CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());

                restorer.disableRestore();  // Now all checks passed

                स्थिरांक V3_संख्या& mask_अंक = mask.constp()->अंक();

                incrOps(nodep, __LINE__);
                incrOps(andp, __LINE__);

                setPolarities(ref, &mask_अंक);
            } अन्यथा {  // comp == v
                स्थिरांक LeafInfo& ref = findLeaf(nodep->rhsp(), असत्य);
                CONST_BITOP_RETURN_IF(!ref.refp(), nodep->rhsp());

                restorer.disableRestore();  // Now all checks passed

                incrOps(nodep, __LINE__);

                setPolarities(ref, nullptr);
            }
        } अन्यथा {
            CONST_BITOP_SET_FAILED("Mixture of different ops cannot be optimized", nodep);
        }
    }

    // CONSTRUCTORS
    Const_Bit_Op_वृक्ष_Visitor(Ast_गाठ_अभिव्यक्ती* nodep, अचिन्हित externalOps, VNIdAllocator& ids)
        : m_ops{externalOps}
        , m_rootp{nodep}
        , m_ids{ids} {
        // Fill nullptr at [0] because a base index of 0 means not set yet
        m_varInfos.ढकला_back(nullptr);
        CONST_BITOP_RETURN_IF(!is_And_वृक्ष() && !is_Or_वृक्ष() && !is_Xor_वृक्ष(), nodep);
        जर (Ast_गाठ_Biop* स्थिरांक biopp = VN_CAST(nodep, गाठ_Biop)) {
            iterateConst(biopp);
        } अन्यथा {
            UASSERT_OBJ(VN_IS(nodep, RedXor), nodep, "Must be RedXor");
            incrOps(nodep, __LINE__);
            iterateChildrenConst(nodep);
        }
        साठी (स्वतः&& entry : m_bitPolarities) {
            getVarInfo(entry.m_info).setPolarity(entry.m_polarity, entry.m_bit);
        }
        UASSERT_OBJ(is_Xor_वृक्ष() || m_polarity, nodep, "must be the original polarity");
    }
    आभासी ~Const_Bit_Op_वृक्ष_Visitor() = पूर्वनिर्धारित;
#undef CONST_BITOP_RETURN_IF
#undef CONST_BITOP_SET_FAILED

सार्वजनिक:
    // Transform as below.
    // v[0] & v[1] => 2'b11 == (2'b11 & v)
    // v[0] | v[1] => 2'b00 != (2'b11 & v)
    // v[0] ^ v[1] => ^{2'b11 & v}
    // (3'b011 == (3'b011 & v)) & v[2]  => 3'b111 == (3'b111 & v)
    // (3'b000 != (3'b011 & v)) | v[2]  => 3'b000 != (3'b111 & v)
    // Reduction ops are transformed in the same way.
    // &{v[0], v[1]} => 2'b11 == (2'b11 & v)
    स्थिर Ast_गाठ_अभिव्यक्ती* simplify(Ast_गाठ_अभिव्यक्ती* nodep, पूर्णांक resultWidth, अचिन्हित externalOps,
                                 VDouble0& reduction, VNIdAllocator& ids) {
        UASSERT_OBJ(1 <= resultWidth && resultWidth <= 64, nodep, "resultWidth out of range");

        // Walk tree, gathering all terms referenced in expression
        स्थिरांक Const_Bit_Op_वृक्ष_Visitor visitor{nodep, externalOps, ids};

        // If failed on root node is not optimizable, or there are no variable terms, then done
        जर (visitor.m_failed || visitor.m_varInfos.size() == 1) परत nullptr;

        // FileLine used for constructing all new nodes in this function
        File_ओळ* स्थिरांक fl = nodep->fileline();

        // Get partial result each term referenced, count total number of ops and keep track of
        // whether we have clean/dirty terms. visitor.m_varInfos appears in deterministic order,
        // so the optimized tree is deterministic as well.

        std::vector<Ast_गाठ_अभिव्यक्ती*> termps;
        termps.reserve(visitor.m_varInfos.size() - 1);
        अचिन्हित resultOps = 0;
        तार्किक hasCleanTerm = असत्य;
        तार्किक hasDirtyTerm = असत्य;

        साठी (स्वतः&& v : visitor.m_varInfos) {
            जर (!v) सुरूठेवा;  // Skip nullptr at m_varInfos[0]
            जर (v->hasConstResult()) {
                // If a constant term is known, we can either drop it or the whole tree is constant
                Ast_गाठ_अभिव्यक्ती* resultp = nullptr;
                जर (v->getConstResult()) {
                    UASSERT_OBJ(visitor.is_Or_वृक्ष(), nodep,
                                "Only OR tree can yield known 1 result");
                    UINFO(9, "OR tree with const 1 term: " << v->refp());
                    // Known 1 bit in OR tree, whole result is 1
                    resultp = नवीन AstConst{fl, AstConst::BitTrue{}};
                } अन्यथा जर (visitor.is_And_वृक्ष()) {
                    UINFO(9, "AND tree with const 0 term: " << v->refp());
                    // Known 0 bit in AND tree, whole result is 0
                    resultp = नवीन AstConst{fl, AstConst::BitFalse{}};
                } अन्यथा {
                    // Known 0 bit in OR or XOR tree. Ignore it.
                    सुरूठेवा;
                }
                // Set width and widthMin precisely
                resultp->dtypeChgWidth(resultWidth, 1);
                साठी (Ast_गाठ* स्थिरांक termp : termps) VL_DO_DANGLING(termp->delete_वृक्ष(), termp);
                परत resultp;
            }
            स्थिरांक ResultTerm result = v->getResultTerm();
            termps.ढकला_back(std::get<0>(result));
            resultOps += std::get<1>(result);
            जर (std::get<2>(result)) {
                hasCleanTerm = सत्य;
                UINFO(9, "Clean term: " << termps.back());
            } अन्यथा {
                hasDirtyTerm = सत्य;
                UINFO(9, "Dirty term: " << termps.back());
            }
        }

        // Group by FrozenNodeInfo
        std::map<Frozen_गाठ_Info, std::vector<Ast_गाठ_अभिव्यक्ती*>> frozenNodes;
        // Check if frozen terms are clean or not
        साठी (स्थिरांक स्वतः& frozenInfo : visitor.m_frozenNodes) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक termp = frozenInfo.first;
            // Comparison operators are clean
            जर ((VN_IS(termp, Eq) || VN_IS(termp, Neq) || VN_IS(termp, Lt) || VN_IS(termp, Lte)
                 || VN_IS(termp, Gt) || VN_IS(termp, Gte))
                && frozenInfo.second.m_lsb == 0) {
                hasCleanTerm = सत्य;
            } अन्यथा {
                // Otherwise, conservatively assume the frozen term is dirty
                hasDirtyTerm = सत्य;
                UINFO(9, "Dirty frozen term: " << termp);
            }
            frozenNodes[frozenInfo.second].ढकला_back(termp);
        }

        // Figure out if a final negation is required
        स्थिरांक तार्किक needsFlip = visitor.is_Xor_वृक्ष() && !visitor.m_polarity;

        // Figure out if the final tree needs cleaning
        स्थिरांक तार्किक needsCleaning = visitor.is_And_वृक्ष() ? !hasCleanTerm : hasDirtyTerm;

        // Add size of reduction tree to op count
        resultOps += termps.size() - 1;
        साठी (स्थिरांक स्वतः& lsbAndNodes : frozenNodes) {
            जर (lsbAndNodes.first.m_lsb > 0) ++resultOps;  // Needs AstShiftR
            जर (!lsbAndNodes.first.m_polarity) ++resultOps;  // Needs AstNot
            resultOps += lsbAndNodes.second.size();
        }
        // Add final polarity flip in Xor tree
        जर (needsFlip) ++resultOps;
        // Add final cleaning AND
        जर (needsCleaning) ++resultOps;

        जर (debug() >= 9) {  // LCOV_EXCL_START
            cout << "-  Bitop tree considered:\n";
            साठी (स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक termp : termps) termp->dump_वृक्ष("-  Reduced term: ");
            साठी (स्थिरांक std::pair<Ast_गाठ_अभिव्यक्ती*, Frozen_गाठ_Info>& termp : visitor.m_frozenNodes) {
                termp.first->dump_वृक्ष("-  Frozen term with lsb "
                                      + std::to_string(termp.second.m_lsb) + " polarity "
                                      + std::to_string(termp.second.m_polarity) + ": ");
            }
            cout << "-  Needs flipping: " << needsFlip << "\n";
            cout << "-  Needs cleaning: " << needsCleaning << "\n";
            cout << "-  Size: " << resultOps << " input size: " << visitor.m_ops << "\n";
        }  // LCOV_EXCL_STOP

        // Sometimes we have no terms left after ignoring redundant terms
        // (all of which were zeroes)
        जर (termps.empty() && visitor.m_frozenNodes.empty()) {
            reduction += visitor.m_ops;
            Ast_गाठ_अभिव्यक्ती* स्थिरांक resultp = needsFlip ? नवीन AstConst{fl, AstConst::BitTrue{}}
                                                   : नवीन AstConst{fl, AstConst::BitFalse{}};
            resultp->dtypeChgWidth(resultWidth, 1);
            परत resultp;
        }

        // Only substitute the result if beneficial as determined by operation count
        जर (visitor.m_ops <= resultOps) {
            साठी (Ast_गाठ* स्थिरांक termp : termps) VL_DO_DANGLING(termp->delete_वृक्ष(), termp);
            परत nullptr;
        }

        // Update statistics
        reduction += visitor.m_ops - resultOps;

        // Reduction op to combine terms
        स्थिरांक स्वतः reduce = [&visitor, fl](Ast_गाठ_अभिव्यक्ती* lhsp, Ast_गाठ_अभिव्यक्ती* rhsp) -> Ast_गाठ_अभिव्यक्ती* {
            जर (!lhsp) परत rhsp;
            जर (visitor.is_And_वृक्ष()) {
                परत नवीन AstAnd{fl, lhsp, rhsp};
            } अन्यथा जर (visitor.is_Or_वृक्ष()) {
                परत नवीन AstOr{fl, lhsp, rhsp};
            } अन्यथा {
                परत नवीन AstXor{fl, lhsp, rhsp};
            }
        };

        // Compute result by reducing all terms
        Ast_गाठ_अभिव्यक्ती* resultp = nullptr;
        साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक termp : termps) {  //
            resultp = reduce(resultp, termp);
        }
        // Add any frozen terms to the reduction
        साठी (स्वतः&& nodes : frozenNodes) {
            // nodes.second has same lsb and polarity
            Ast_गाठ_अभिव्यक्ती* termp = nullptr;
            साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक itemp : nodes.second) {
                termp = reduce(termp, itemp->unlinkFrBack());
            }
            जर (nodes.first.m_lsb > 0) {  // LSB is not 0, so shiftR
                Ast_गाठ_DType* स्थिरांक dtypep = termp->dtypep();
                termp = नवीन AstShiftR{termp->fileline(), termp,
                                      नवीन AstConst(termp->fileline(), AstConst::Widthed_मूल्य{},
                                                   termp->width(), nodes.first.m_lsb)};
                termp->dtypep(dtypep);
            }
            जर (!nodes.first.m_polarity) {  // Polarity is inverted, so append Not
                Ast_गाठ_DType* स्थिरांक dtypep = termp->dtypep();
                termp = नवीन AstNot{termp->fileline(), termp};
                termp->dtypep(dtypep);
            }
            resultp = reduce(resultp, termp);
        }

        // Set width of masks to expected result width. This is required to prevent later removal
        // of the masking node e.g. by the "AND with all ones" rule. If the result width happens
        // to be 1, we still need to ensure the AstAnd is not dropped, so use a wider mask in this
        // special case.
        स्थिरांक पूर्णांक maskWidth
            = std::max(resultp->width(), resultWidth == 1 ? VL_IDATASIZE : resultWidth);

        // Apply final polarity flip
        जर (needsFlip) {
            जर (needsCleaning) {
                // Cleaning will be added below. Use a NOT which is a byte shorter on x86
                resultp = नवीन AstNot{fl, resultp};
            } अन्यथा {
                // Keep result clean by using XOR(1, _)
                AstConst* स्थिरांक maskp = नवीन AstConst{fl, AstConst::Widthed_मूल्य{}, maskWidth, 1};
                resultp = नवीन AstXor{fl, maskp, resultp};
            }
        }

        // Apply final cleaning
        जर (needsCleaning) {
            AstConst* स्थिरांक maskp = नवीन AstConst{fl, AstConst::Widthed_मूल्य{}, maskWidth, 1};
            resultp = नवीन AstAnd{fl, maskp, resultp};
        }

        // Cast back to original size if required
        जर (resultp->width() != resultWidth) {
            resultp = नवीन AstCCast{fl, resultp, resultWidth, 1};
        }

        // Set width and widthMin precisely
        resultp->dtypeChgWidth(resultWidth, 1);

        परत resultp;
    }
};

//######################################################################
// Const state, as a visitor of each AstNode

वर्ग ConstVisitor final : सार्वजनिक VNVisitor {
    // CONSTANTS
    स्थिर constexpr अचिन्हित CONCAT_MERGABLE_MAX_DEPTH = 10;  // Limit alg recursion
    // Largest right-hand side that is copied to each part of a split concatenation assignment
    स्थिर constexpr पूर्णांक CONCAT_ASSIGN_MAX_RHS_NODES = 16;

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
    तार्किक m_convertLogicToBit = असत्य;  // Convert logical operators to bitwise
    तार्किक m_hasJumpDelay = असत्य;  // JumpGo or Delay under this loop
    तार्किक m_hasLoopTest = असत्य;  // Contains AstLoopTest
    तार्किक m_underRecFunc = असत्य;  // Under a recursive function
    Ast_गाठ_Module* m_modp = nullptr;  // Current module
    स्थिरांक AstArraySel* m_selp = nullptr;  // Current select
    स्थिरांक AstScope* m_scopep = nullptr;  // Current scope
    स्थिरांक AstAttrOf* m_attrp = nullptr;  // Current attribute
    VDouble0 m_statBitOpReduction;  // Ops reduced in ConstBitOpTreeVisitor
    VDouble0 m_statConcatMerge;  // Concat merges
    VDouble0 m_stat_Cond_अभिव्यक्ती_Redundant;  // Conditional repeated expressions
    VDouble0 m_stat_If_Cond_अभिव्यक्ती_Redundant;  // Conditional repeated expressions
    VDouble0 m_statMemberAccessVisits;  // Nodes visited by containsMemberAccessRecurse
    स्थिरांक तार्किक m_globalPass;  // ConstVisitor invoked as a global pass
    स्थिर uint32_t s_global_Pass_अंक;  // Counts number of times ConstVisitor invoked as global pass
    V3UniqueNames m_concswapNames;  // For generating unique temporary variable names
    // Caches results of matchBiopToBitwise, keyed on node id
    std::unordered_map<माप_प्रकार, तार्किक> m_containsMemberAccess;
    // Items of enum currently being recursed into. Cannot use VNIdAllocator, as this runs in
    // every mode, including those invoked under a pass holding user4 (V3Param). Node pointers
    // are safe as keys here, unlike elsewhere in V3Const: folding the item value below does
    // free nodes whose addresses are then reused, but AstEnumItem is only ever constructed
    // while parsing (verilog.y and V3LinkParse), so a reused address can never become one,
    // and every key looked up is an AstEnumItem.
    std::unordered_set<स्थिरांक AstEnumItem*> m_recursing_Enum_वस्तुसंच;
    std::unordered_set<AstJumpBlock*> m_usedJumpBlocks;  // JumpBlocks used by some JumpGo

    // METHODS

    // Return true if statements may be deleted. Until statics are lifted, a variable declared
    // in a statement might be referenced from elsewhere (e.g. hierarchically).
    स्थिर तार्किक canDeleteStatements() { परत v3Global.staticsLifted(); }

    शून्य deleteVarScopesUnder(Ast_गाठ* subtreep) {
        जर (!subtreep) परत;
        जर (!m_scopep) परत;
        std::unordered_set<AstVar*> varps;
        subtreep->foreachAndNext([&](AstVar* varp) { varps.घाला(varp); });
        जर (varps.empty()) परत;
        साठी (AstVarScope *vscp = m_scopep->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            जर (varps.find(vscp->varp()) != varps.end()) {
                VL_DO_DANGLING(ढकला_Deletep(vscp->unlinkFrBack()), vscp);
            }
        }
    }

    V3_संख्या const_अंक_V(Ast_गाठ* nodep) {
        // Contract C width to V width (if needed, else just direct copy)
        // The upper zeros in the C representation can otherwise cause
        // wrong results in some operations, e.g. MulS
        स्थिरांक V3_संख्या& numc = VN_AS(nodep, Const)->अंक();
        परत !numc.is_संख्या() ? numc : V3_संख्या{nodep, nodep->widthMinV(), numc};
    }
    V3_संख्या to_अंक_C(Ast_गाठ* nodep, स्थिरांक V3_संख्या& numv) {
        // Extend V width back to C width for given node
        परत !numv.is_संख्या() ? numv : V3_संख्या{nodep, nodep->width(), numv};
    }

    स्थिर तार्किक lowerAsFixedAggregate(स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep) {
        परत dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }

    AstStructSel* newStructSel(Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp, स्थिरांक AstMemberDType* स्थिरांक itemp) {
        AstStructSel* स्थिरांक selp = नवीन AstStructSel{fromp->fileline(), fromp, itemp->name()};
        selp->dtypeFrom(itemp->dtypep());
        परत selp;
    }

    शून्य collectFixedAggregateTerms(Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp, std::vector<Ast_गाठ_अभिव्यक्ती*>& termps,
                                    स्थिरांक तार्किक packReal) {
        स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep = fromp->dtypep()->skipRefp();
        जर (स्थिरांक AstUnpackArrayDType* स्थिरांक unpackDtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            साठी (स्थिरांक पूर्णांक idx : unpackDtypep->declRange().seq_डावा_To_उजवा()) {
                AstArraySel* स्थिरांक selp
                    = नवीन AstArraySel{fromp->fileline(), fromp->clone_वृक्ष_Pure(असत्य), idx};
                collectFixedAggregateTerms(selp, termps, packReal);
            }
            VL_DO_DANGLING(ढकला_Deletep(fromp), fromp);
        } अन्यथा जर (स्थिरांक Ast_गाठ_UOr_Struct_DType* स्थिरांक sdtypep
                   = VN_CAST(dtypep, गाठ_UOr_Struct_DType)) {
            जर (sdtypep->packed()) {
                termps.ढकला_back(fromp);
                परत;
            }
            साठी (स्थिरांक AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                collectFixedAggregateTerms(newStructSel(fromp->clone_वृक्ष_Pure(असत्य), itemp),
                                           termps, packReal);
            }
            VL_DO_DANGLING(ढकला_Deletep(fromp), fromp);
        } अन्यथा जर (packReal && dtypep->isDouble()) {
            termps.ढकला_back(नवीन AstRealToBits{fromp->fileline(), fromp});
        } अन्यथा {
            termps.ढकला_back(fromp);
        }
    }

    Ast_गाठ_अभिव्यक्ती* packFixedAggregate(Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp) {
        std::vector<Ast_गाठ_अभिव्यक्ती*> termps;
        collectFixedAggregateTerms(fromp, termps, सत्य);
        UASSERT(!termps.empty(), "No stream terms");
        Ast_गाठ_अभिव्यक्ती* resultp = termps[0];
        साठी (माप_प्रकार i = 1; i < termps.size(); ++i) {
            resultp = नवीन AstConcat{resultp->fileline(), resultp, termps[i]};
        }
        परत resultp;
    }

    शून्य replaceAssignToFixedAggregate(Ast_गाठ_Assign* स्थिरांक nodep, Ast_गाठ_अभिव्यक्ती* स्थिरांक dstp,
                                       Ast_गाठ_अभिव्यक्ती* srcp) {
        स्थिरांक पूर्णांक dstWidth = dstp->dtypep()->widthStream();
        std::vector<Ast_गाठ_अभिव्यक्ती*> termps;
        collectFixedAggregateTerms(dstp, termps, असत्य);
        पूर्णांक srcWidth = srcp->width();
        जर (srcWidth < dstWidth) {
            AstExtend* स्थिरांक extendp = नवीन AstExtend{srcp->fileline(), srcp, dstWidth};
            srcp = नवीन AstShiftL{
                extendp->fileline(), extendp,
                नवीन AstConst{extendp->fileline(), static_cast<uint32_t>(dstWidth - srcWidth)},
                dstWidth};
            srcWidth = dstWidth;
        }
        Ast_गाठ_Assign* newp = nullptr;
        पूर्णांक offset = 0;
        साठी (माप_प्रकार i = 0; i < termps.size(); ++i) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक termp = termps[i];
            स्थिरांक पूर्णांक width = termp->dtypep()->widthStream();
            स्थिरांक पूर्णांक lsb = srcWidth - offset - width;
            offset += width;
            Ast_गाठ_अभिव्यक्ती* rhsp
                = नवीन AstSel{srcp->fileline(), srcp->clone_वृक्ष_Pure(असत्य), lsb, width};
            जर (termp->dtypep()->skipRefp()->isDouble()) {
                rhsp = नवीन AstBitsToRealD{rhsp->fileline(), rhsp};
            }
            Ast_गाठ_Assign* स्थिरांक assignp = nodep->cloneType(termp, rhsp);
            assignp->dtypeFrom(termp);
            newp = Ast_गाठ::addNext(newp, assignp);
        }
        nodep->addNextHere(newp);
        VL_DO_DANGLING(ढकला_Deletep(srcp), srcp);
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }

    तार्किक operandConst(स्थिरांक Ast_गाठ* nodep) { परत VN_IS(nodep, Const); }
    तार्किक operandAsvConst(स्थिरांक Ast_गाठ* nodep) {
        // BIASV(CONST, BIASV(CONST,...)) -> BIASV( BIASV_CONSTED(a,b), ...)
        स्थिरांक Ast_गाठ_Bi_Com_Asv* स्थिरांक bnodep = VN_CAST(nodep, गाठ_Bi_Com_Asv);
        जर (!bnodep) परत असत्य;
        जर (!VN_IS(bnodep->lhsp(), Const)) परत असत्य;
        स्थिरांक Ast_गाठ_Bi_Com_Asv* स्थिरांक rnodep = VN_CAST(bnodep->rhsp(), गाठ_Bi_Com_Asv);
        जर (!rnodep) परत असत्य;
        जर (rnodep->type() != bnodep->type()) परत असत्य;
        जर (rnodep->width() != bnodep->width()) परत असत्य;
        जर (rnodep->lhsp()->width() != bnodep->lhsp()->width()) परत असत्य;
        जर (!VN_IS(rnodep->lhsp(), Const)) परत असत्य;
        परत सत्य;
    }
    तार्किक operandAsvSame(स्थिरांक Ast_गाठ* nodep) {
        // BIASV(SAMEa, BIASV(SAMEb,...)) -> BIASV( BIASV(SAMEa,SAMEb), ...)
        स्थिरांक Ast_गाठ_Bi_Com_Asv* स्थिरांक bnodep = VN_CAST(nodep, गाठ_Bi_Com_Asv);
        जर (!bnodep) परत असत्य;
        स्थिरांक Ast_गाठ_Bi_Com_Asv* स्थिरांक rnodep = VN_CAST(bnodep->rhsp(), गाठ_Bi_Com_Asv);
        जर (!rnodep) परत असत्य;
        जर (rnodep->type() != bnodep->type()) परत असत्य;
        जर (rnodep->width() != bnodep->width()) परत असत्य;
        परत operandsSame(bnodep->lhsp(), rnodep->lhsp());
    }
    तार्किक operandAsvLUp(स्थिरांक Ast_गाठ* nodep) {
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
        स्थिरांक Ast_गाठ_Bi_Com_Asv* स्थिरांक bnodep = VN_CAST(nodep, गाठ_Bi_Com_Asv);
        जर (!bnodep) परत असत्य;
        स्थिरांक Ast_गाठ_Bi_Com_Asv* स्थिरांक lnodep = VN_CAST(bnodep->lhsp(), गाठ_Bi_Com_Asv);
        जर (!lnodep) परत असत्य;
        जर (lnodep->type() != bnodep->type()) परत असत्य;
        जर (lnodep->width() != bnodep->width()) परत असत्य;
        परत VN_IS(lnodep->lhsp(), Const);
    }
    तार्किक operandAsvRUp(स्थिरांक Ast_गाठ* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr)) ?
        स्थिरांक Ast_गाठ_Bi_Com_Asv* स्थिरांक bnodep = VN_CAST(nodep, गाठ_Bi_Com_Asv);
        जर (!bnodep) परत असत्य;
        स्थिरांक Ast_गाठ_Bi_Com_Asv* स्थिरांक rnodep = VN_CAST(bnodep->rhsp(), गाठ_Bi_Com_Asv);
        जर (!rnodep) परत असत्य;
        जर (rnodep->type() != bnodep->type()) परत असत्य;
        जर (rnodep->width() != bnodep->width()) परत असत्य;
        परत VN_IS(rnodep->lhsp(), Const);
    }
    स्थिर तार्किक operandSubAdd(स्थिरांक Ast_गाठ* nodep) {
        // SUB( ADD(CONSTx,y), CONSTz) -> ADD(SUB(CONSTx,CONSTz), y)
        स्थिरांक Ast_गाठ_Biop* स्थिरांक np = VN_CAST(nodep, गाठ_Biop);
        स्थिरांक Ast_गाठ_Biop* स्थिरांक lp = VN_CAST(np->lhsp(), गाठ_Biop);
        परत (lp && VN_IS(lp->lhsp(), Const) && VN_IS(np->rhsp(), Const)
                && lp->width() == np->width());
    }
    तार्किक matchRedundantClean(AstAnd* andp) {
        // Remove And with constant one inserted by V3Clean
        // 1 & (a == b)  -> (IData)(a == b)
        // When bool is casted to int, the value is either 0 or 1
        AstConst* स्थिरांक constp = VN_AS(andp->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isOne(), andp->lhsp(), "TRREEOPC must meet this condition");
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = andp->rhsp();
        AstCCast* ccastp = nullptr;
        स्थिरांक स्वतः isEqOrNeq = [](स्थिरांक Ast_गाठ* nodep) -> तार्किक {  //
            परत VN_IS(nodep, Eq) || VN_IS(nodep, Neq);
        };
        जर (isEqOrNeq(rhsp)) {
            ccastp = नवीन AstCCast{andp->fileline(), rhsp->unlinkFrBack(), andp};
        } अन्यथा जर (AstCCast* स्थिरांक tmpp = VN_CAST(rhsp, CCast)) {
            जर (isEqOrNeq(tmpp->lhsp())) {
                जर (tmpp->width() == andp->width()) {
                    tmpp->unlinkFrBack();
                    ccastp = tmpp;
                } अन्यथा {
                    ccastp = नवीन AstCCast{andp->fileline(), tmpp->lhsp()->unlinkFrBack(), andp};
                }
            }
        }
        जर (ccastp) {
            andp->replaceWithKeepDType(ccastp);
            VL_DO_DANGLING(ढकला_Deletep(andp), andp);
            परत सत्य;
        }
        परत असत्य;
    }

    स्थिर तार्किक operandAndOrSame(स्थिरांक Ast_गाठ* nodep) {
        // OR( AND(VAL,x), AND(VAL,y)) -> AND(VAL,OR(x,y))
        // OR( AND(x,VAL), AND(y,VAL)) -> AND(OR(x,y),VAL)
        स्थिरांक Ast_गाठ_Biop* स्थिरांक np = VN_CAST(nodep, गाठ_Biop);
        स्थिरांक Ast_गाठ_Biop* स्थिरांक lp = VN_CAST(np->lhsp(), गाठ_Biop);
        स्थिरांक Ast_गाठ_Biop* स्थिरांक rp = VN_CAST(np->rhsp(), गाठ_Biop);
        परत (lp && rp && lp->width() == rp->width() && lp->type() == rp->type()
                && (operandsSame(lp->lhsp(), rp->lhsp()) || operandsSame(lp->rhsp(), rp->rhsp())));
    }
    तार्किक matchOrAndNot(Ast_गाठ_Biop* nodep) {
        // AstOr{$a, AstAnd{AstNot{$b}, $c}} if $a.width1, $a==$b => AstOr{$a,$c}
        // Someday we'll sort the biops completely and this can be simplified
        // This often results from our simplified clock generation:
        // if (rst) ... else if (enable)... -> OR(rst,AND(!rst,enable))
        स्थिरांक Ast_गाठ_अभिव्यक्ती* ap;
        Ast_गाठ_Biop* andp;
        जर (VN_IS(nodep->lhsp(), And)) {
            andp = VN_AS(nodep->lhsp(), And);
            ap = nodep->rhsp();
        } अन्यथा जर (VN_IS(nodep->rhsp(), And)) {
            andp = VN_AS(nodep->rhsp(), And);
            ap = nodep->lhsp();
        } अन्यथा {
            परत असत्य;
        }
        स्थिरांक Ast_गाठ_Uniop* notp;
        Ast_गाठ_अभिव्यक्ती* cp;
        जर (VN_IS(andp->lhsp(), Not)) {
            notp = VN_AS(andp->lhsp(), Not);
            cp = andp->rhsp();
        } अन्यथा जर (VN_IS(andp->rhsp(), Not)) {
            notp = VN_AS(andp->rhsp(), Not);
            cp = andp->lhsp();
        } अन्यथा {
            परत असत्य;
        }
        स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक bp = notp->lhsp();
        जर (!operandsSame(ap, bp)) परत असत्य;
        // Do it
        cp->unlinkFrBack();
        VL_DO_DANGLING(ढकला_Deletep(andp->unlinkFrBack()), andp);
        VL_DANGLING(notp);
        // Replace whichever branch is now dangling
        जर (nodep->rhsp()) {
            nodep->lhsp(cp);
        } अन्यथा {
            nodep->rhsp(cp);
        }
        परत सत्य;
    }
    तार्किक matchAndCond(AstAnd* nodep) {
        // Push down a AND into conditional, when one side of conditional is constant
        // (otherwise we'd be trading one operation for two operations)
        // V3Clean often makes this pattern, as it postpones the AND until
        // as high as possible, which is usually the right choice, except for this.
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstCond* स्थिरांक condp = VN_CAST(nodep->rhsp(), Cond);
        जर (!condp) परत असत्य;
        जर (!VN_IS(condp->thenp(), Const) && !VN_IS(condp->elsep(), Const)) परत असत्य;
        AstConst* स्थिरांक maskp = VN_CAST(nodep->lhsp(), Const);
        जर (!maskp) परत असत्य;
        UINFO(4, "AND(CONSTm, CONDcond(c, i, e))->CONDcond(c, AND(m,i), AND(m, e)) " << nodep);
        AstCond* स्थिरांक newp = नवीन AstCond{
            condp->fileline(), condp->condp()->unlinkFrBack(),
            नवीन AstAnd{nodep->fileline(), maskp->clone_वृक्ष(असत्य), condp->thenp()->unlinkFrBack()},
            नवीन AstAnd{nodep->fileline(), maskp->clone_वृक्ष(असत्य),
                       condp->elsep()->unlinkFrBack()}};
        newp->thenp()->dtypeFrom(nodep);  // As And might have been to change widths
        newp->elsep()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        परत सत्य;
    }

    तार्किक matchCondCond(स्थिरांक AstCond* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        जर (VN_IS(nodep->backp(), Cond)) परत असत्य;  // Was checked when visited parent
        जर (!VN_IS(nodep->thenp(), Cond) && !VN_IS(nodep->elsep(), Cond))
            परत असत्य;  // Short circuit
        std::vector<Ast_गाठ_अभिव्यक्ती*> truesp;
        std::vector<Ast_गाठ_अभिव्यक्ती*> falsesp;
        matchCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
        परत असत्य;  // Can optimize further
    }
    शून्य matchCondCondRecurse(स्थिरांक AstCond* nodep, std::vector<Ast_गाठ_अभिव्यक्ती*>& truesp,
                              std::vector<Ast_गाठ_अभिव्यक्ती*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        जर (truesp.size() > 4 || falsesp.size() > 4) परत;
        जर (!nodep->condp()->isPure()) परत;
        तार्किक replaced = असत्य;
        साठी (स्थिरांक Ast_गाठ_अभिव्यक्ती* condp : truesp) {
            जर (replaced) थांबा;
            जर (!operandsSame(nodep->condp(), condp)) सुरूठेवा;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_अंक(nodep->condp(), 1);
            replaced = सत्य;
            ++m_stat_Cond_अभिव्यक्ती_Redundant;
        }
        साठी (स्थिरांक Ast_गाठ_अभिव्यक्ती* condp : falsesp) {
            जर (replaced) थांबा;
            जर (!operandsSame(nodep->condp(), condp)) सुरूठेवा;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = सत्य;
            ++m_stat_Cond_अभिव्यक्ती_Redundant;
        }
        जर (स्थिरांक AstCond* subCondp = VN_CAST(nodep->thenp(), Cond)) {
            जर (!replaced) truesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            जर (!replaced) truesp.काढा_back();
        }
        जर (स्थिरांक AstCond* subCondp = VN_CAST(nodep->elsep(), Cond)) {
            जर (!replaced) falsesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            जर (!replaced) falsesp.काढा_back();
        }
    }
    शून्य matchIfCondCond(स्थिरांक Ast_गाठ_If* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        जर (VN_IS(nodep->backp(), If)) परत;  // Was checked when visited parent
        जर (!VN_IS(nodep->thensp(), If) && !VN_IS(nodep->elsesp(), If)) परत;  // Short circuit
        std::vector<Ast_गाठ_अभिव्यक्ती*> truesp;
        std::vector<Ast_गाठ_अभिव्यक्ती*> falsesp;
        matchIfCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
    }
    शून्य matchIfCondCondRecurse(स्थिरांक Ast_गाठ_If* nodep, std::vector<Ast_गाठ_अभिव्यक्ती*>& truesp,
                                std::vector<Ast_गाठ_अभिव्यक्ती*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        जर (truesp.size() > 4 || falsesp.size() > 4) परत;
        जर (!nodep->condp()->isPure()) परत;
        // A constant condition is folded by visit(AstNodeIf), once statements can be deleted
        जर (VN_IS(nodep->condp(), Const)) परत;
        तार्किक replaced = असत्य;
        साठी (स्थिरांक Ast_गाठ_अभिव्यक्ती* condp : truesp) {
            जर (replaced) थांबा;
            जर (!operandsSame(nodep->condp(), condp)) सुरूठेवा;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_अंक(nodep->condp(), 1);
            replaced = सत्य;
            ++m_stat_If_Cond_अभिव्यक्ती_Redundant;
        }
        साठी (स्थिरांक Ast_गाठ_अभिव्यक्ती* condp : falsesp) {
            जर (replaced) थांबा;
            जर (!operandsSame(nodep->condp(), condp)) सुरूठेवा;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = सत्य;
            ++m_stat_If_Cond_अभिव्यक्ती_Redundant;
        }
        // We only check the first statement of parent IF is an If
        // So we don't need to check for effects in the executing thensp/elsesp
        // altering the child't condition.  e.g. 'if (x) begin x=1; if (x) end'
        जर (स्थिरांक Ast_गाठ_If* subIfp = VN_CAST(nodep->thensp(), गाठ_If)) {
            जर (!replaced) truesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            जर (!replaced) truesp.काढा_back();
        }
        जर (स्थिरांक Ast_गाठ_If* subIfp = VN_CAST(nodep->elsesp(), गाठ_If)) {
            जर (!replaced) falsesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            जर (!replaced) falsesp.काढा_back();
        }
    }

    शून्य matchIfSameLast(Ast_गाठ_If* nodep) {
        जर (!nodep->thensp()) परत;
        जर (!nodep->elsesp()) परत;
        जर (!canDeleteStatements()) परत;
        // Iterate bodies in reverse
        Ast_गाठ* tp = nodep->thensp()->lastp();
        Ast_गाठ* ep = nodep->elsesp()->lastp();
        पर्यंत (tp && ep && tp->same_वृक्ष(ep)) {
            UASSERT_OBJ(!tp->nextp(), tp, "Expected no nextp");
            UASSERT_OBJ(!ep->nextp(), ep, "Expected no nextp");
            Ast_गाठ* स्थिरांक tPrevp = tp->prevp();
            Ast_गाठ* स्थिरांक ePrevp = ep->prevp();
            nodep->addNextHere(tp->unlinkFrBack());
            VL_DO_DANGLING(ढकला_Deletep(ep->unlinkFrBack()), ep);
            tp = tPrevp;
            ep = ePrevp;
        }
    }

    तार्किक matchMaskedOr(AstAnd* nodep) {
        // Masking an OR with terms that have no bits set under the mask is replaced with masking
        // only the remaining terms. Canonical example as generated by V3Expand is:
        // 0xff & (a << 8 | b >> 24) --> 0xff & (b >> 24)

        // Compute how many significant bits are in the mask
        स्थिरांक AstConst* स्थिरांक constp = VN_AS(nodep->lhsp(), Const);
        स्थिरांक uint32_t significantBits = constp->अंक().widthToFit();

        AstOr* स्थिरांक orp = VN_AS(nodep->rhsp(), Or);

        // Predicate for checking whether the bottom 'significantBits' bits of the given expression
        // are all zeroes.
        स्थिरांक स्वतः checkBottomClear = [=](स्थिरांक Ast_गाठ* nodep) -> तार्किक {
            जर (स्थिरांक AstShiftL* स्थिरांक shiftp = VN_CAST(nodep, ShiftL)) {
                जर (स्थिरांक AstConst* स्थिरांक scp = VN_CAST(shiftp->rhsp(), Const)) {
                    परत scp->अंक().toUInt() >= significantBits;
                }
            }
            परत असत्य;
        };

        स्थिरांक तार्किक orLIsRedundant = checkBottomClear(orp->lhsp());
        स्थिरांक तार्किक orRIsRedundant = checkBottomClear(orp->rhsp());

        जर (orLIsRedundant && orRIsRedundant) {
            nodep->replaceWithKeepDType(
                नवीन AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत सत्य;
        } अन्यथा जर (orLIsRedundant) {
            orp->replaceWithKeepDType(orp->rhsp()->unlinkFrBack());
            VL_DO_DANGLING(ढकला_Deletep(orp), orp);
            परत असत्य;  // input node is still valid, keep going
        } अन्यथा जर (orRIsRedundant) {
            orp->replaceWithKeepDType(orp->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(ढकला_Deletep(orp), orp);
            परत असत्य;  // input node is still valid, keep going
        } अन्यथा {
            परत असत्य;
        }
    }
    तार्किक matchMaskedShift(AstAnd* nodep) {
        // Drop redundant masking of right shift result. E.g: 0xff & ((uint32_t)a >> 24). This
        // commonly appears after V3Expand and the simplification in matchMaskedOr. Similarly,
        // drop redundant masking of left shift result. E.g.: 0xff000000 & ((uint32_t)a << 24).

        स्थिरांक स्वतः checkMask = [nodep, हे](स्थिरांक V3_संख्या& mask) -> तार्किक {
            स्थिरांक AstConst* स्थिरांक constp = VN_AS(nodep->lhsp(), Const);
            जर (constp->अंक().isCaseEq(mask)) {
                Ast_गाठ* स्थिरांक rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                nodep->replaceWithKeepDType(rhsp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                परत सत्य;
            }
            परत असत्य;
        };

        // Check if masking is redundant
        जर (स्थिरांक AstShiftR* स्थिरांक shiftp = VN_CAST(nodep->rhsp(), ShiftR)) {
            जर (स्थिरांक AstConst* स्थिरांक scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_संख्या maskLo{nodep, nodep->width()};
                maskLo.setMask(nodep->width() - scp->अंक().toUInt());
                परत checkMask(maskLo);
            }
        } अन्यथा जर (स्थिरांक AstShiftL* स्थिरांक shiftp = VN_CAST(nodep->rhsp(), ShiftL)) {
            जर (स्थिरांक AstConst* स्थिरांक scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_संख्या mask{nodep, nodep->width()};
                स्थिरांक uint32_t shiftAmount = scp->अंक().toUInt();
                mask.setMask(nodep->width() - shiftAmount, shiftAmount);
                परत checkMask(mask);
            }
        }
        परत असत्य;
    }

    तार्किक matchMaskedZero(स्थिरांक AstAnd* nodep) {
        // Turn masking of known zero bits into constant zero. Commonly appears after V3Expand.
        स्थिरांक AstConst* स्थिरांक maskp = VN_AS(nodep->lhsp(), Const);
        स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp();
        स्थिरांक uint32_t msbP1 = maskp->अंक().mostSetBitP1();
        जर (!msbP1) परत असत्य;  // Don't rewrite, separate rule matches for this
        स्थिरांक uint32_t msb = msbP1 - 1;
        स्थिरांक uint32_t lsb = maskp->अंक().leastSetBitP1() - 1;

        जर (स्थिरांक AstShiftL* स्थिरांक shiftp = VN_CAST(rhsp, ShiftL)) {
            // 'a << S' forces the low S bits to zero
            जर (AstConst* स्थिरांक scp = VN_CAST(shiftp->rhsp(), Const)) {
                परत scp->अंक().fitsInUInt()  //
                       && (scp->अंक().toUInt() > msb);
            }
        }
        जर (स्थिरांक AstShiftR* स्थिरांक shiftp = VN_CAST(rhsp, ShiftR)) {
            // 'a >> S' forces the high S bits to zero. Check against the width of the shifted
            // operand, V3Expand can create shifts wider than their inputs. Must use width(),
            // not widthMin(), as bits above widthMin() are not guaranteed to be zero.
            जर (AstConst* स्थिरांक scp = VN_CAST(shiftp->rhsp(), Const)) {
                परत scp->अंक().fitsInUInt()
                       && (lsb + scp->अंक().toUInt()
                           >= static_cast<uint32_t>(shiftp->lhsp()->width()));
            }
        }
        जर (स्थिरांक AstMul* स्थिरांक mulp = VN_CAST(rhsp, Mul)) {
            // 'C * a' forces the low N bits to zero where 'C' has low zero bits
            जर (AstConst* स्थिरांक cp = VN_CAST(mulp->lhsp(), Const)) {
                परत cp->अंक().leastSetBitP1() > msb + 1;
            }
        }
        जर (स्थिरांक AstExtend* स्थिरांक extendp = VN_CAST(rhsp, Extend)) {
            // Zero-extension forces the bits above the source width to zero
            परत lsb >= static_cast<uint32_t>(extendp->lhsp()->width());
        }

        परत असत्य;
    }

    तार्किक match_Bit_Op_वृक्ष(Ast_गाठ_अभिव्यक्ती* nodep) {
        जर (nodep->widthMin() != 1) परत असत्य;
        जर (!v3Global.opt.f_Const_Bit_Op_वृक्ष()) परत असत्य;

        स्थिरांक पूर्णांक width = nodep->width();
        Ast_गाठ_अभिव्यक्ती* rootp = nodep;
        अचिन्हित externalOps = 0;
        // Reach past a plain making AND
        जर (स्थिरांक AstAnd* स्थिरांक andp = VN_CAST(nodep, And)) {
            जर (isConst(andp->lhsp(), 1)) {
                rootp = andp->rhsp();
                externalOps = 1;
            }
        }

        // Only optimize if rootp is in fact the root of a tree of identical
        // operations, otherwise analysis on an eventually unoptimizable unablanced
        // tree can go O(N^2) as we would re-analyze every time as we move up the
        // tree, repeatedly re-discover the sub-tree is not optimizable.
        जर (rootp->type() == nodep->backp()->type()) परत असत्य;

        std::string debugPrefix;
        जर (debug() >= 9) {  // LCOV_EXCL_START
            स्थिर पूर्णांक s_c = 0;
            debugPrefix = "-  matchBitOpTree[" + std::to_string(++s_c) + "] ";
            nodep->dump_वृक्ष(debugPrefix + "INPUT: ");
        }  // LCOV_EXCL_STOP

        Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = Const_Bit_Op_वृक्ष_Visitor::simplify(rootp, width, externalOps,
                                                                  m_statBitOpReduction, m_ids);

        जर (newp) {
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Transformed leaf of bit tree to " << newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }

        जर (debug() >= 9) {  // LCOV_EXCL_START
            जर (newp) {
                newp->dump_वृक्ष(debugPrefix + "RESULT: ");
            } अन्यथा {
                cout << debugPrefix << "not replaced\n";
            }
        }  // LCOV_EXCL_STOP

        परत newp;
    }
    स्थिर तार्किक operandShiftSame(स्थिरांक Ast_गाठ* nodep) {
        स्थिरांक Ast_गाठ_Biop* स्थिरांक np = VN_AS(nodep, गाठ_Biop);
        {
            स्थिरांक AstShiftL* स्थिरांक lp = VN_CAST(np->lhsp(), ShiftL);
            स्थिरांक AstShiftL* स्थिरांक rp = VN_CAST(np->rhsp(), ShiftL);
            जर (lp && rp) {
                परत (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        {
            स्थिरांक AstShiftR* स्थिरांक lp = VN_CAST(np->lhsp(), ShiftR);
            स्थिरांक AstShiftR* स्थिरांक rp = VN_CAST(np->rhsp(), ShiftR);
            जर (lp && rp) {
                परत (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        परत असत्य;
    }
    तार्किक operandHugeShiftL(स्थिरांक Ast_गाठ_Biop* nodep) {
        परत (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->अंक().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->अंक().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->width())))
                && nodep->lhsp()->isPure());
    }
    तार्किक operandHugeShiftR(स्थिरांक Ast_गाठ_Biop* nodep) {
        परत (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->अंक().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->अंक().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->lhsp()->width())))
                && nodep->lhsp()->isPure()
                && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)));
    }
    तार्किक operandIsTwo(स्थिरांक Ast_गाठ* nodep) {
        स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep, Const);
        जर (!constp) परत असत्य;  // not constant
        जर (constp->अंक().isFourState()) परत असत्य;  // four-state
        जर (nodep->width() > VL_QUADSIZE) परत असत्य;  // too wide
        जर (nodep->isSigned() && constp->अंक().isNegative()) परत असत्य;  // signed and negative
        परत constp->toUQuad() == 2;
    }
    तार्किक operandIsTwostate(स्थिरांक Ast_गाठ* nodep) {
        परत (VN_IS(nodep, Const) && !VN_AS(nodep, Const)->अंक().isFourState());
    }
    तार्किक operandIsPowTwo(स्थिरांक Ast_गाठ* nodep) {
        जर (!operandIsTwostate(nodep)) परत असत्य;
        परत (1 == VN_AS(nodep, Const)->अंक().मोजणी_Ones());
    }
    तार्किक operandShiftOp(स्थिरांक Ast_गाठ_Biop* nodep) {
        जर (!VN_IS(nodep->rhsp(), Const)) परत असत्य;
        स्थिरांक Ast_गाठ_Biop* स्थिरांक lhsp = VN_CAST(nodep->lhsp(), गाठ_Biop);
        जर (!lhsp || !(VN_IS(lhsp, And) || VN_IS(lhsp, Or) || VN_IS(lhsp, Xor))) परत असत्य;
        जर (nodep->width() != lhsp->width()) परत असत्य;
        जर (nodep->width() != lhsp->lhsp()->width()) परत असत्य;
        जर (nodep->width() != lhsp->rhsp()->width()) परत असत्य;

        // Only do it if we can fruther reduce one of the sides if pulling the shift through:
        // - Either operands are constants
        जर (VN_IS(lhsp->lhsp(), Const)) परत सत्य;
        जर (VN_IS(lhsp->rhsp(), Const)) परत सत्य;
        // - Const shift on either side
        जर (Ast_गाठ_Biop* स्थिरांक llp = VN_CAST(lhsp->lhsp(), गाठ_Biop)) {
            परत (VN_IS(llp, ShiftL) || VN_IS(llp, ShiftR)) && VN_IS(llp->rhsp(), Const);
        }
        जर (Ast_गाठ_Biop* स्थिरांक lrp = VN_CAST(lhsp->rhsp(), गाठ_Biop)) {
            परत (VN_IS(lrp, ShiftL) || VN_IS(lrp, ShiftR)) && VN_IS(lrp->rhsp(), Const);
        }
        // Otherwise we would increase logic size
        परत असत्य;
    }
    तार्किक operandShiftShift(स्थिरांक Ast_गाठ_Biop* nodep) {
        // We could add a AND though.
        स्थिरांक Ast_गाठ_Biop* स्थिरांक lhsp = VN_CAST(nodep->lhsp(), गाठ_Biop);
        जर (!lhsp || !(VN_IS(lhsp, ShiftL) || VN_IS(lhsp, ShiftR))) परत असत्य;
        // We can only get rid of a<<b>>c or a<<b<<c, with constant b & c
        // because bits may be masked in that process, or (b+c) may exceed the word width.
        जर (!(VN_IS(nodep->rhsp(), Const) && VN_IS(lhsp->rhsp(), Const))) परत असत्य;
        जर (VN_AS(nodep->rhsp(), Const)->अंक().isFourState()
            || VN_AS(lhsp->rhsp(), Const)->अंक().isFourState())
            परत असत्य;
        जर (nodep->width() != lhsp->width()) परत असत्य;
        जर (nodep->width() != lhsp->lhsp()->width()) परत असत्य;
        परत सत्य;
    }
    तार्किक operandWordOOB(स्थिरांक AstWordSel* nodep) {
        // V3Expand may make a arraysel that exceeds the bounds of the array
        // It was an expression, then got constified.  In reality, the WordSel
        // must be wrapped in a Cond, that will be false.
        परत (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), गाठ_Var_Ref)
                && VN_AS(nodep->fromp(), गाठ_Var_Ref)->access().isReadOnly()
                && (static_cast<पूर्णांक>(VN_AS(nodep->bitp(), Const)->toUInt())
                    >= VN_AS(nodep->fromp(), गाठ_Var_Ref)->varp()->width_शब्द()));
    }
    तार्किक operandSelFull(स्थिरांक AstSel* nodep) {
        परत (VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0
                && static_cast<पूर्णांक>(nodep->widthConst()) == nodep->fromp()->width());
    }
    तार्किक operandSelExtend(AstSel* nodep) {
        जर (!m_doV) परत असत्य;
        AstExtend* स्थिरांक extendp = VN_CAST(nodep->fromp(), Extend);
        जर (!extendp) परत असत्य;
        AstConst* स्थिरांक lsbp = VN_CAST(nodep->lsbp(), Const);
        जर (!lsbp) परत असत्य;
        स्थिरांक पूर्णांक width = nodep->widthConst();
        स्थिरांक पूर्णांक lsb = lsbp->toSInt();
        स्थिरांक पूर्णांक msb = lsb + width - 1;
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = extendp->lhsp();
        जर (!lhsp->isPure()) परत असत्य;

        // Selecting the entire extended expression, replace with that
        जर (lsb == 0 && msb == lhsp->width() - 1) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(lhsp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत सत्य;
        }
        // Select entirely in the extended part - replace with zero
        जर (lsb >= lhsp->width()) {
            replaceZero(nodep);
            परत सत्य;
        }
        // Select entirely in the extended expression - replace with select from that
        जर (msb < lhsp->width()) {
            lhsp->unlinkFrBack();
            lsbp->unlinkFrBack();
            nodep->replaceWithKeepDType(नवीन AstSel{nodep->fileline(), lhsp, lsbp, width});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत सत्य;
        }
        // Select straddles both sides, but is just a shorter extend
        जर (lsb == 0) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(नवीन AstExtend{nodep->fileline(), lhsp});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत सत्य;
        }

        परत असत्य;
    }
    तार्किक operandSelBiLower(AstSel* nodep) {
        // SEL(ADD(a,b),(width-1),0) -> ADD(SEL(a),SEL(b))
        // Add or any operation which doesn't care if we discard top bits
        Ast_गाठ_Biop* स्थिरांक bip = VN_CAST(nodep->fromp(), गाठ_Biop);
        जर (!(m_doV && bip && VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0)) परत असत्य;
        UINFOTREE(9, nodep, "", "SEL(BI)-in");
        Ast_गाठ_अभिव्यक्ती* स्थिरांक bilhsp = bip->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक birhsp = bip->rhsp()->unlinkFrBack();
        bip->lhsp(नवीन AstSel{nodep->fileline(), bilhsp, 0, nodep->widthConst()});
        bip->rhsp(नवीन AstSel{nodep->fileline(), birhsp, 0, nodep->widthConst()});
        UINFOTREE(9, bip, "", "SEL(BI)-ou");
        VL_DO_DANGLING(replaceWChild(nodep, bip), nodep);
        परत सत्य;
    }
    तार्किक operandSelShiftLower(AstSel* nodep) {
        // AND({a}, SHIFTR({b}, {c})) is often shorthand in C for Verilog {b}[{c} :+ {a}]
        // becomes thought other optimizations
        // SEL(SHIFTR({a},{b}),{lsb},{width}) -> SEL({a},{lsb+b},{width})
        स्थिरांक AstShiftR* स्थिरांक shiftp = VN_CAST(nodep->fromp(), ShiftR);
        जर (!(m_doV && shiftp && VN_IS(shiftp->rhsp(), Const) && VN_IS(nodep->lsbp(), Const))) {
            परत असत्य;
        }
        Ast_गाठ_अभिव्यक्ती* स्थिरांक ap = shiftp->lhsp();
        स्थिरांक AstConst* स्थिरांक bp = VN_AS(shiftp->rhsp(), Const);
        स्थिरांक AstConst* स्थिरांक lp = VN_AS(nodep->lsbp(), Const);
        जर (bp->isWide() || bp->अंक().isFourState() || bp->अंक().isNegative() || lp->isWide()
            || lp->अंक().isFourState() || lp->अंक().isNegative()) {
            परत असत्य;
        }
        स्थिरांक पूर्णांक newLsb = lp->toSInt() + bp->toSInt();
        जर (newLsb + nodep->widthConst() > ap->width()) परत असत्य;
        //
        UINFO(9, "SEL(SHIFTR(a,b),l,w) -> SEL(a,l+b,w)");
        UINFOTREE(9, nodep, "", "SEL(SH)-in");
        AstSel* स्थिरांक newp
            = नवीन AstSel{nodep->fileline(), ap->unlinkFrBack(), newLsb, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        UINFOTREE(9, newp, "", "SEL(SH)-ou");
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        परत सत्य;
    }

    तार्किक operandBiExtendConstShrink(Ast_गाठ_Biop* nodep) {
        // Loop unrolling favors standalone compares
        // EQ(const{width32}, EXTEND(xx{width3})) -> EQ(const{3}, xx{3})
        // The constant must have zero bits (+ 1 if signed) or compare
        // would be incorrect. See also operandBiExtendConst
        AstExtend* स्थिरांक extendp = VN_CAST(nodep->rhsp(), Extend);
        जर (!extendp) परत असत्य;
        Ast_गाठ_अभिव्यक्ती* स्थिरांक smallerp = extendp->lhsp();
        स्थिरांक पूर्णांक subsize = smallerp->width();
        AstConst* स्थिरांक constp = VN_CAST(nodep->lhsp(), Const);
        जर (!constp) परत असत्य;
        जर (!constp->अंक().isBitsZero(constp->width() - 1, subsize)) परत असत्य;
        //
        UINFOTREE(9, nodep, "", "BI(EXTEND)-in");
        smallerp->unlinkFrBack();
        VL_DO_DANGLING(ढकला_Deletep(extendp->unlinkFrBack()), extendp);  // aka nodep->lhsp.
        nodep->rhsp(smallerp);

        constp->unlinkFrBack();
        स्थिरांक V3_संख्या अंक{constp, subsize, constp->अंक()};
        nodep->lhsp(नवीन AstConst{constp->fileline(), अंक});
        VL_DO_DANGLING(ढकला_Deletep(constp), constp);
        परत असत्य;  // input node is still valid, keep going
    }
    तार्किक operandBiExtendConstOver(स्थिरांक Ast_गाठ_Biop* nodep) {
        // EQ(const{width32}, EXTEND(xx{width3})) -> constant
        // When the constant has non-zero bits above the extend it's a constant.
        // Avoids compiler warning
        स्थिरांक AstExtend* स्थिरांक extendp = VN_CAST(nodep->rhsp(), Extend);
        जर (!extendp) परत असत्य;
        स्थिरांक Ast_गाठ* स्थिरांक smallerp = extendp->lhsp();
        स्थिरांक पूर्णांक subsize = smallerp->width();
        स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->lhsp(), Const);
        जर (!constp) परत असत्य;
        जर (constp->अंक().isBitsZero(constp->width() - 1, subsize)) परत असत्य;
        परत सत्य;
    }

    // Extraction checks
    तार्किक warnSelect(AstSel* nodep) {
        जर (m_doGenerate) {
            // Never checked yet
            V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);  // May need "constifying"
        }
        // Find range of dtype we are selecting from
        // Similar code in V3Unknown::AstSel
        स्थिरांक तार्किक doit = सत्य;
        जर (m_warn && VN_IS(nodep->lsbp(), Const) && doit) {
            स्थिरांक पूर्णांक maxDeclBit = nodep->declRange().hiMaxSelect() * nodep->declElWidth()
                                   + (nodep->declElWidth() - 1);
            जर (VN_AS(nodep->lsbp(), Const)->अंक().isFourState()) {
                nodep->v3error("Selection index is constantly unknown or tristated: "
                               "lsb="
                               << nodep->lsbp()->name() << " width=" << nodep->widthConst());
                // Replacing nodep will make a mess above, so we replace the offender
                replaceZero(nodep->lsbp());
            } अन्यथा जर (nodep->declRange().ranged()
                       && (nodep->msbConst() > maxDeclBit || nodep->lsbConst() > maxDeclBit)) {
                // See also warning in V3Width
                // Must adjust by element width as declRange() is in number of elements
                string msbLsbProtected;
                जर (nodep->declElWidth() == 0) {
                    msbLsbProtected = "(nodep->declElWidth() == 0) "
                                      + std::to_string(nodep->msbConst()) + ":"
                                      + std::to_string(nodep->lsbConst());
                } अन्यथा {
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
        परत असत्य;  // Not a transform, so NOP
    }

    स्थिर तार्किक operandsSame(स्थिरांक Ast_गाठ* node1p, स्थिरांक Ast_गाठ* node2p) {
        // For now we just detect constants & simple vars, though it could be more generic
        जर (स्थिरांक AstConst* स्थिरांक const1p = VN_CAST(node1p, Const)) {
            जर (स्थिरांक AstConst* स्थिरांक const2p = VN_CAST(node2p, Const)) {
                परत V3_संख्या{node1p->fileline(), 1, 0}
                    .opEq(const1p->अंक(), const2p->अंक())
                    .isEqOne();
            }
        }
        जर (VN_IS(node1p, VarRef) && VN_IS(node2p, VarRef)) {
            // Avoid comparing widthMin's, which results in lost optimization attempts
            // If cleanup sameGateTree to be smarter, this can be restored.
            // return node1p->sameGateTree(node2p);
            परत node1p->isSame(node2p);
        }
        // Pattern created by coverage-line/branch; avoid compiler tautological-compare warning
        जर (स्थिरांक AstAnd* स्थिरांक and1p = VN_CAST(node1p, And)) {
            जर (स्थिरांक AstAnd* स्थिरांक and2p = VN_CAST(node2p, And)) {
                जर (VN_IS(and1p->lhsp(), Const) && VN_IS(and1p->rhsp(), गाठ_Var_Ref)
                    && VN_IS(and2p->lhsp(), Const) && VN_IS(and2p->rhsp(), गाठ_Var_Ref))
                    परत node1p->same_Gate_वृक्ष(node2p);
            }
        }
        परत असत्य;
    }
    तार्किक ifSameAssign(स्थिरांक Ast_गाठ_If* nodep) {
        स्थिरांक Ast_गाठ_Assign* स्थिरांक thensp = VN_CAST(nodep->thensp(), गाठ_Assign);
        स्थिरांक Ast_गाठ_Assign* स्थिरांक elsesp = VN_CAST(nodep->elsesp(), गाठ_Assign);
        जर (!thensp || thensp->nextp()) परत असत्य;  // Must be SINGLE statement
        जर (!elsesp || elsesp->nextp()) परत असत्य;
        जर (thensp->type() != elsesp->type()) परत असत्य;  // Can't mix an assigndly with assign
        जर (!thensp->lhsp()->same_Gate_वृक्ष(elsesp->lhsp())) परत असत्य;
        जर (!thensp->rhsp()->gate_वृक्ष()) परत असत्य;
        जर (!elsesp->rhsp()->gate_वृक्ष()) परत असत्य;
        // Must not create an expression with unpacked array type
        जर (VN_IS(thensp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) परत असत्य;
        जर (VN_IS(elsesp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) परत असत्य;
        जर (m_underRecFunc) परत असत्य;  // This optimization may lead to infinite recursion
        जर (m_doCpp) {  // Do not combine ifs cretaed by V3Premit
            जर (VN_IS(thensp->lhsp(), VarRef) && thensp->lhsp()->isWide()) परत असत्य;
        }
        // Only do it if not calls and both pure, otherwise undoes V3LiftExpr
        परत !VN_IS(thensp->rhsp(), गाठ_FTask_Ref)  //
               && !VN_IS(elsesp->rhsp(), गाठ_FTask_Ref)  //
               && thensp->rhsp()->isPure()  //
               && elsesp->rhsp()->isPure();
    }
    तार्किक operandIfIf(स्थिरांक Ast_गाठ_If* nodep) {
        जर (nodep->elsesp()) परत असत्य;
        स्थिरांक Ast_गाठ_If* स्थिरांक lowerIfp = VN_CAST(nodep->thensp(), गाठ_If);
        जर (!lowerIfp || lowerIfp->nextp()) परत असत्य;
        जर (nodep->type() != lowerIfp->type()) परत असत्य;
        जर (Ast_गाठ::afterCommentp(lowerIfp->elsesp())) परत असत्य;
        परत सत्य;
    }
    तार्किक ifConcatMergeableBiop(स्थिरांक Ast_गाठ* nodep) {
        परत (VN_IS(nodep, And) || VN_IS(nodep, Or) || VN_IS(nodep, Xor));
    }
    तार्किक ifAdjacentSel(स्थिरांक AstSel* lhsp, स्थिरांक AstSel* rhsp) {
        जर (!v3Global.opt.fAssemble()) परत असत्य;  // opt disabled
        जर (!lhsp || !rhsp) परत असत्य;
        स्थिरांक Ast_गाठ* स्थिरांक lfromp = lhsp->fromp();
        स्थिरांक Ast_गाठ* स्थिरांक rfromp = rhsp->fromp();
        जर (!lfromp || !rfromp || !lfromp->same_Gate_वृक्ष(rfromp)) परत असत्य;
        स्थिरांक AstConst* स्थिरांक lstart = VN_CAST(lhsp->lsbp(), Const);
        स्थिरांक AstConst* स्थिरांक rstart = VN_CAST(rhsp->lsbp(), Const);
        जर (!lstart || !rstart) परत असत्य;  // too complicated
        स्थिरांक पूर्णांक rend = (rstart->toSInt() + rhsp->widthConst());
        परत (rend == lstart->toSInt());
    }
    तार्किक ifMergeAdjacent(स्थिरांक Ast_गाठ_अभिव्यक्ती* lhsp, स्थिरांक Ast_गाठ_अभिव्यक्ती* rhsp) {
        // Called by concatmergeable to determine if {lhsp, rhsp} make sense
        जर (!v3Global.opt.fAssemble()) परत असत्य;  // opt disabled
        // Two same varref
        जर (operandsSame(lhsp, rhsp)) परत सत्य;
        स्थिरांक AstSel* स्थिरांक lselp = VN_CAST(lhsp, Sel);
        स्थिरांक AstSel* स्थिरांक rselp = VN_CAST(rhsp, Sel);
        जर (!lselp && !rselp) परत असत्य;
        जर (lselp && !VN_IS(lselp->lsbp(), Const)) परत असत्य;
        जर (rselp && !VN_IS(rselp->lsbp(), Const)) परत असत्य;
        // a[i:j] {a[j-1:k], b}
        जर (lselp && !rselp && VN_IS(rhsp, Concat))
            परत ifMergeAdjacent(lhsp, VN_AS(rhsp, Concat)->lhsp());
        // {b, a[j:k]} a[k-1:i]
        जर (rselp && !lselp && VN_IS(lhsp, Concat))
            परत ifMergeAdjacent(VN_AS(lhsp, Concat)->rhsp(), rhsp);
        // a a[msb:j]
        स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक lfromp = lselp                                ? lselp->fromp()
                                          : lhsp->same_Gate_वृक्ष(rselp->fromp()) ? lhsp
                                                                               : nullptr;
        // a[i:0] a
        स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक rfromp = rselp                                ? rselp->fromp()
                                          : rhsp->same_Gate_वृक्ष(lselp->fromp()) ? rhsp
                                                                               : nullptr;
        जर (!lfromp || !rfromp || !lfromp->same_Gate_वृक्ष(rfromp)) परत असत्य;

        स्थिरांक int32_t lstart = lselp ? lselp->lsbConst() : 0;
        स्थिरांक int32_t rstart = rselp ? rselp->lsbConst() : 0;
        स्थिरांक int32_t rend = rstart + (rselp ? rselp->widthConst() : rhsp->width());
        // a[i:j] a[j-1:k]
        जर (rend == lstart) परत सत्य;
        // a[i:0] a[msb:j]
        जर (rend == rfromp->width() && lstart == 0) परत सत्य;
        परत असत्य;
    }
    तार्किक concatMergeable(स्थिरांक Ast_गाठ_अभिव्यक्ती* lhsp, स्थिरांक Ast_गाठ_अभिव्यक्ती* rhsp, अचिन्हित depth) {
        // Determine if {a OP b, c OP d} => {a, c} OP {b, d} is advantageous
        जर (!v3Global.opt.fAssemble()) परत असत्य;  // opt disabled
        जर (lhsp->type() != rhsp->type()) परत असत्य;
        जर (!ifConcatMergeableBiop(lhsp)) परत असत्य;
        जर (depth > CONCAT_MERGABLE_MAX_DEPTH) परत असत्य;  // As worse case O(n^2) algorithm

        स्थिरांक Ast_गाठ_Biop* स्थिरांक lp = VN_CAST(lhsp, गाठ_Biop);
        स्थिरांक Ast_गाठ_Biop* स्थिरांक rp = VN_CAST(rhsp, गाठ_Biop);
        जर (!lp || !rp) परत असत्य;
        // {a[]&b[], a[]&b[]}
        स्थिरांक तार्किक lad = ifMergeAdjacent(lp->lhsp(), rp->lhsp());
        स्थिरांक तार्किक rad = ifMergeAdjacent(lp->rhsp(), rp->rhsp());
        जर (lad && rad) परत सत्य;
        // {a[] & b[]&c[], a[] & b[]&c[]}
        जर (lad && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) परत सत्य;
        // {a[]&b[] & c[], a[]&b[] & c[]}
        जर (rad && concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)) परत सत्य;
        // {(a[]&b[])&(c[]&d[]), (a[]&b[])&(c[]&d[])}
        जर (concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)
            && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) {
            परत सत्य;
        }
        परत असत्य;
    }
    स्थिर तार्किक operandsSameWidth(स्थिरांक Ast_गाठ* lhsp, स्थिरांक Ast_गाठ* rhsp) {
        परत lhsp->width() == rhsp->width();
    }

    //----------------------------------------
    // Constant Replacement functions.
    // These all take a node, delete its tree, and replaces it with a constant

    शून्य replace_अंक(Ast_गाठ* oldp, स्थिरांक V3_संख्या& अंक, स्थिरांक string& origParamName = "") {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        UASSERT_OBJ(!(VN_IS(oldp, Const) && !VN_AS(oldp, Const)->अंक().isFourState()), oldp,
                    "Already constant??");
        AstConst* स्थिरांक newp = नवीन AstConst{oldp->fileline(), अंक};
        जर (!origParamName.empty()) newp->origParamName(origParamName);
        oldp->replaceWithKeepDType(newp);
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        VL_DO_DANGLING(ढकला_Deletep(oldp), oldp);
    }
    शून्य replace_अंक(Ast_गाठ* nodep, uint32_t val) {
        स्थिरांक V3_संख्या अंक{nodep, nodep->width(), val};
        VL_DO_DANGLING(replace_अंक(nodep, अंक), nodep);
    }
    शून्य replace_अंक_Signed(Ast_गाठ_Biop* nodep, uint32_t val) {
        // We allow both sides to be constant, as one may have come from
        // parameter propagation, etc.
        जर (m_warn && !(VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const))) {
            nodep->v3warn(UNSIGNED, "Comparison is constant due to unsigned arithmetic");
        }
        VL_DO_DANGLING(replace_अंक(nodep, val), nodep);
    }
    शून्य replace_अंक_Limited(Ast_गाठ_Biop* nodep, uint32_t val) {
        // Avoids gcc warning about same
        जर (m_warn) nodep->v3warn(CMPCONST, "Comparison is constant due to limited range");
        VL_DO_DANGLING(replace_अंक(nodep, val), nodep);
    }
    शून्य replaceZero(Ast_गाठ* nodep) { VL_DO_DANGLING(replace_अंक(nodep, 0), nodep); }
    शून्य replaceZeroChkPure(Ast_गाठ* nodep, Ast_गाठ_अभिव्यक्ती* checkp) {
        // For example, "0 * n" -> 0 if n has no side effects
        // Else strength reduce it to 0 & n.
        // If ever change the operation note AstAnd rule specially ignores this created pattern
        जर (checkp->isPure()) {
            VL_DO_DANGLING(replace_अंक(nodep, 0), nodep);
        } अन्यथा {
            Ast_गाठ* स्थिरांक newp = नवीन AstAnd{nodep->fileline(), नवीन AstConst{nodep->fileline(), 0},
                                             checkp->unlinkFrBack()};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }
    शून्य replaceAllOnes(Ast_गाठ* nodep) {
        V3_संख्या ones{nodep, nodep->width(), 0};
        ones.setMask(nodep->width());
        VL_DO_DANGLING(replace_अंक(nodep, ones), nodep);
    }
    शून्य replaceConst(Ast_गाठ_Uniop* nodep) {
        V3_संख्या numv{nodep, nodep->widthMinV()};
        nodep->संख्या_Operate(numv, const_अंक_V(nodep->lhsp()));
        स्थिरांक V3_संख्या& अंक = to_अंक_C(nodep, numv);
        UINFO(4, "UNICONST -> " << अंक);
        VL_DO_DANGLING(replace_अंक(nodep, अंक), nodep);
    }
    शून्य replaceConst(Ast_गाठ_Biop* nodep) {
        V3_संख्या numv{nodep, nodep->widthMinV()};
        nodep->संख्या_Operate(numv, const_अंक_V(nodep->lhsp()), const_अंक_V(nodep->rhsp()));
        स्थिरांक V3_संख्या& अंक = to_अंक_C(nodep, numv);
        UINFO(4, "BICONST -> " << अंक);
        VL_DO_DANGLING(replace_अंक(nodep, अंक), nodep);
    }
    शून्य replaceConst(Ast_गाठ_Triop* nodep) {
        V3_संख्या numv{nodep, nodep->widthMinV()};
        nodep->संख्या_Operate(numv, const_अंक_V(nodep->lhsp()), const_अंक_V(nodep->rhsp()),
                             const_अंक_V(nodep->thsp()));
        स्थिरांक V3_संख्या& अंक = to_अंक_C(nodep, numv);
        UINFO(4, "TRICONST -> " << अंक);
        VL_DO_DANGLING(replace_अंक(nodep, अंक), nodep);
    }
    शून्य replaceConst(Ast_गाठ_Quadop* nodep) {
        V3_संख्या numv{nodep, nodep->widthMinV()};
        nodep->संख्या_Operate(numv, const_अंक_V(nodep->lhsp()), const_अंक_V(nodep->rhsp()),
                             const_अंक_V(nodep->thsp()), const_अंक_V(nodep->fhsp()));
        स्थिरांक V3_संख्या& अंक = to_अंक_C(nodep, numv);
        UINFO(4, "QUADCONST -> " << अंक);
        VL_DO_DANGLING(replace_अंक(nodep, अंक), nodep);
    }

    शून्य replaceConstString(Ast_गाठ* oldp, स्थिरांक string& अंक) {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        Ast_गाठ* स्थिरांक newp = नवीन AstConst{oldp->fileline(), AstConst::String{}, अंक};
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        oldp->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(oldp), oldp);
    }
    //----------------------------------------
    // Replacement functions.
    // These all take a node and replace it with something else

    शून्य replaceWChild(Ast_गाठ* nodep, Ast_गाठ_अभिव्यक्ती* childp) {
        // NODE(..., CHILD(...)) -> CHILD(...)
        childp->unlinkFrBackWithNext();
        // If replacing a SEL for example, the data type comes from the parent (is less wide).
        // This may adversely affect the operation of the node being replaced.
        nodep->replaceWithKeepDType(childp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    शून्य replaceWChildBool(Ast_गाठ* nodep, Ast_गाठ_अभिव्यक्ती* childp) {
        // NODE(..., CHILD(...)) -> REDOR(CHILD(...))
        childp->unlinkFrBack();
        जर (childp->width1()) {
            nodep->replaceWithKeepDType(childp);
        } अन्यथा {
            nodep->replaceWithKeepDType(नवीन AstRedOr{childp->fileline(), childp});
        }
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    //! Replace a ternary node with its RHS after iterating
    //! Used with short-circuiting, where the RHS has not yet been iterated.
    शून्य replaceWIteratedRhs(Ast_गाठ_Triop* nodep) {
        जर (Ast_गाठ* स्थिरांक rhsp = nodep->rhsp()) iterateAndNextNull(rhsp);
        replaceWChild(nodep, nodep->rhsp());  // May have changed
    }

    //! Replace a ternary node with its THS after iterating
    //! Used with short-circuiting, where the THS has not yet been iterated.
    शून्य replaceWIteratedThs(Ast_गाठ_Triop* nodep) {
        जर (Ast_गाठ* स्थिरांक thsp = nodep->thsp()) iterateAndNextNull(thsp);
        replaceWChild(nodep, nodep->thsp());  // May have changed
    }
    शून्य replaceWLhs(Ast_गाठ_Uniop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    शून्य replaceWLhs(Ast_गाठ_Biop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    शून्य replaceWRhs(Ast_गाठ_Biop* nodep) {
        // Keep RHS, remove LHS
        replaceWChild(nodep, nodep->rhsp());
    }
    शून्य replaceWLhsBool(Ast_गाठ_Biop* nodep) { replaceWChildBool(nodep, nodep->lhsp()); }
    शून्य replaceWRhsBool(Ast_गाठ_Biop* nodep) { replaceWChildBool(nodep, nodep->rhsp()); }
    शून्य replaceAsv(Ast_गाठ_Biop* nodep) {
        // BIASV(CONSTa, BIASV(CONSTb, c)) -> BIASV( BIASV_CONSTED(a,b), c)
        // BIASV(SAMEa,  BIASV(SAMEb, c))  -> BIASV( BIASV(SAMEa,SAMEb), c)
        // UINFOTREE(1, nodep, "", "repAsvConst_old");
        Ast_गाठ_अभिव्यक्ती* स्थिरांक ap = nodep->lhsp();
        Ast_गाठ_Biop* स्थिरांक rp = VN_AS(nodep->rhsp(), गाठ_Biop);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक bp = rp->lhsp();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक cp = rp->rhsp();
        ap->unlinkFrBack();
        bp->unlinkFrBack();
        cp->unlinkFrBack();
        rp->unlinkFrBack();
        nodep->lhsp(rp);
        nodep->rhsp(cp);
        rp->lhsp(ap);
        rp->rhsp(bp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        जर (VN_IS(rp->lhsp(), Const) && VN_IS(rp->rhsp(), Const)) replaceConst(rp);
        iterate(nodep);  // Proceed to fixed point
    }
    शून्य replaceAsvLUp(Ast_गाठ_Biop* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r))
        Ast_गाठ_Biop* स्थिरांक lp = VN_AS(nodep->lhsp()->unlinkFrBack(), गाठ_Biop);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक llp = lp->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lrp = lp->rhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rp = nodep->rhsp()->unlinkFrBack();
        nodep->lhsp(llp);
        nodep->rhsp(lp);
        lp->lhsp(lrp);
        lp->rhsp(rp);
        lp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    शून्य replaceAsvRUp(Ast_गाठ_Biop* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr))
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lp = nodep->lhsp()->unlinkFrBack();
        Ast_गाठ_Biop* स्थिरांक rp = VN_AS(nodep->rhsp()->unlinkFrBack(), गाठ_Biop);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rlp = rp->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rrp = rp->rhsp()->unlinkFrBack();
        nodep->lhsp(rlp);
        nodep->rhsp(rp);
        rp->lhsp(lp);
        rp->rhsp(rrp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    शून्य replaceAndOr(Ast_गाठ_Biop* nodep) {
        //  OR  (AND (CONSTll,lr), AND(CONSTrl==ll,rr))    -> AND (CONSTll, OR(lr,rr))
        //  OR  (AND (CONSTll,lr), AND(CONSTrl,    rr=lr)) -> AND (OR(ll,rl), rr)
        // nodep ^lp  ^llp   ^lrp  ^rp  ^rlp       ^rrp
        // (Or/And may also be reversed)
        Ast_गाठ_Biop* स्थिरांक lp = VN_AS(nodep->lhsp()->unlinkFrBack(), गाठ_Biop);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक llp = lp->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lrp = lp->rhsp()->unlinkFrBack();
        Ast_गाठ_Biop* स्थिरांक rp = VN_AS(nodep->rhsp()->unlinkFrBack(), गाठ_Biop);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rlp = rp->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        जर (operandsSame(llp, rlp)) {
            lp->lhsp(llp);
            lp->rhsp(nodep);
            lp->dtypeFrom(nodep);
            nodep->lhsp(lrp);
            nodep->rhsp(rrp);
            VL_DO_DANGLING(ढकला_Deletep(rp), rp);
            VL_DO_DANGLING(ढकला_Deletep(rlp), rlp);
        } अन्यथा जर (operandsSame(lrp, rrp)) {
            lp->lhsp(nodep);
            lp->rhsp(rrp);
            lp->dtypeFrom(nodep);
            nodep->lhsp(llp);
            nodep->rhsp(rlp);
            VL_DO_DANGLING(ढकला_Deletep(rp), rp);
            VL_DO_DANGLING(ढकला_Deletep(lrp), lrp);
        } अन्यथा {
            nodep->v3fatalSrc("replaceAndOr on something operandAndOrSame shouldn't have matched");
        }
        // UINFOTREE(1, nodep, "", "repAndOr_new");
    }
    शून्य replaceShiftSame(Ast_गाठ_Biop* nodep) {
        // Or(Shift(ll,CONSTlr),Shift(rl,CONSTrr==lr)) -> Shift(Or(ll,rl),CONSTlr)
        // (Or/And may also be reversed)
        Ast_गाठ_Biop* स्थिरांक lp = VN_AS(nodep->lhsp()->unlinkFrBack(), गाठ_Biop);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक llp = lp->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lrp = lp->rhsp()->unlinkFrBack();
        Ast_गाठ_Biop* स्थिरांक rp = VN_AS(nodep->rhsp()->unlinkFrBack(), गाठ_Biop);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rlp = rp->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        lp->lhsp(nodep);
        lp->rhsp(lrp);
        nodep->lhsp(llp);
        nodep->rhsp(rlp);
        nodep->dtypep(llp->dtypep());  // dtype of Biop is before shift.
        VL_DO_DANGLING(ढकला_Deletep(rp), rp);
        VL_DO_DANGLING(ढकला_Deletep(rrp), rrp);
        // UINFOTREE(1, nodep, "", "repShiftSame_new");
    }
    शून्य replaceConcatSel(AstConcat* nodep) {
        // {a[1], a[0]} -> a[1:0]
        AstSel* स्थिरांक lselp = VN_AS(nodep->lhsp()->unlinkFrBack(), Sel);
        AstSel* स्थिरांक rselp = VN_AS(nodep->rhsp()->unlinkFrBack(), Sel);
        स्थिरांक पूर्णांक lstart = lselp->lsbConst();
        स्थिरांक पूर्णांक lwidth = lselp->widthConst();
        स्थिरांक पूर्णांक rstart = rselp->lsbConst();
        स्थिरांक पूर्णांक rwidth = rselp->widthConst();

        UASSERT_OBJ((rstart + rwidth) == lstart, nodep,
                    "tried to merge two selects which are not adjacent");
        AstSel* स्थिरांक newselp = नवीन AstSel{
            lselp->fromp()->fileline(), rselp->fromp()->unlinkFrBack(), rstart, lwidth + rwidth};
        UINFO(5, "merged two adjacent sel " << lselp << " and " << rselp << " to one " << newselp);

        nodep->replaceWithKeepDType(newselp);
        VL_DO_DANGLING(ढकला_Deletep(lselp), lselp);
        VL_DO_DANGLING(ढकला_Deletep(rselp), rselp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य replaceConcatMerge(AstConcat* nodep) {
        // {llp OP lrp, rlp OP rrp} => {llp, rlp} OP {lrp, rrp}, where OP = AND/OR/XOR
        Ast_गाठ_Biop* स्थिरांक lp = VN_AS(nodep->lhsp(), गाठ_Biop);
        Ast_गाठ_Biop* स्थिरांक rp = VN_AS(nodep->rhsp(), गाठ_Biop);
        जर (concatMergeable(lp, rp, 0)) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक llp = lp->lhsp();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lrp = lp->rhsp();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rlp = rp->lhsp();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rrp = rp->rhsp();
            AstConcat* स्थिरांक newlp = नवीन AstConcat{rlp->fileline(), llp->clone_वृक्ष_Pure(असत्य),
                                                   rlp->clone_वृक्ष_Pure(असत्य)};
            AstConcat* स्थिरांक newrp = नवीन AstConcat{rrp->fileline(), lrp->clone_वृक्ष_Pure(असत्य),
                                                   rrp->clone_वृक्ष_Pure(असत्य)};
            // use the lhs to replace the parent concat
            llp->replaceWith(newlp);
            VL_DO_DANGLING(ढकला_Deletep(llp), llp);
            lrp->replaceWith(newrp);
            VL_DO_DANGLING(ढकला_Deletep(lrp), lrp);
            lp->dtypeChgWidthSigned(newlp->width(), newlp->width(), VSigning::UNSIGNED);
            UINFO(5, "merged " << nodep);
            ++m_statConcatMerge;
            VL_DO_DANGLING(ढकला_Deletep(rp->unlinkFrBack()), rp);
            nodep->replaceWithKeepDType(lp->unlinkFrBack());
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            iterate(lp->lhsp());
            iterate(lp->rhsp());
        } अन्यथा {
            nodep->v3fatalSrc("tried to merge two Concat which are not adjacent");
        }
    }
    शून्य replaceExtend(Ast_गाठ* nodep, Ast_गाठ_अभिव्यक्ती* arg0p) {
        // -> EXTEND(nodep)
        // like a AstExtend{$rhsp}, but we need to set the width correctly from base node
        arg0p->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक newp
            = (VN_IS(nodep, ExtendS)
                   ? static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstExtendS{nodep->fileline(), arg0p})
                   : static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstExtend{nodep->fileline(), arg0p}));
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य replacePowShift(Ast_गाठ_Biop* nodep) {  // Pow or PowS
        UINFO(5, "POW(2,b)->SHIFTL(1,b) " << nodep);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* स्थिरांक newp
            = नवीन AstShiftL{nodep->fileline(), नवीन AstConst{nodep->fileline(), 1}, rhsp};
        newp->lhsp()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य replaceMulShift(AstMul* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "MUL(2^n,b)->SHIFTL(b,n) " << nodep);
        स्थिरांक पूर्णांक amount = VN_AS(nodep->lhsp(), Const)->अंक().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_गाठ_अभिव्यक्ती* स्थिरांक opp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* स्थिरांक newp
            = नवीन AstShiftL{nodep->fileline(), opp, नवीन AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य replaceDivShift(AstDiv* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "DIV(b,2^n)->SHIFTR(b,n) " << nodep);
        स्थिरांक पूर्णांक amount = VN_AS(nodep->rhsp(), Const)->अंक().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_गाठ_अभिव्यक्ती* स्थिरांक opp = nodep->lhsp()->unlinkFrBack();
        AstShiftR* स्थिरांक newp
            = नवीन AstShiftR{nodep->fileline(), opp, नवीन AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य replaceModAnd(AstModDiv* nodep) {  // Mod, but not ModS as not simple shift
        UINFO(5, "MOD(b,2^n)->AND(b,2^n-1) " << nodep);
        स्थिरांक पूर्णांक amount = VN_AS(nodep->rhsp(), Const)->अंक().mostSetBitP1() - 1;  // 2^n->n+1
        V3_संख्या mask{nodep, nodep->width()};
        mask.setMask(amount);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक opp = nodep->lhsp()->unlinkFrBack();
        AstAnd* स्थिरांक newp
            = नवीन AstAnd{nodep->fileline(), opp, नवीन AstConst{nodep->fileline(), mask}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य replaceShiftOp(Ast_गाठ_Biop* nodep) {
        UINFO(5, "SHIFT(AND(a,b),CONST) with a/b special ->AND(SHIFT(a,CONST),SHIFT(b,CONST)) "
                     << nodep);
        स्थिरांक पूर्णांक width = nodep->width();
        स्थिरांक पूर्णांक widthMin = nodep->widthMin();
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        Ast_गाठ_Biop* स्थिरांक lhsp = VN_AS(nodep->lhsp(), गाठ_Biop);
        lhsp->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक shiftp = nodep->rhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक ap = lhsp->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक bp = lhsp->rhsp()->unlinkFrBack();
        Ast_गाठ_Biop* स्थिरांक shift1p = nodep;
        Ast_गाठ_Biop* स्थिरांक shift2p = nodep->clone_वृक्ष(सत्य);
        shift1p->lhsp(ap);
        shift1p->rhsp(shiftp->clone_वृक्ष_Pure(सत्य));
        shift2p->lhsp(bp);
        shift2p->rhsp(shiftp);
        Ast_गाठ_Biop* स्थिरांक newp = lhsp;
        newp->lhsp(shift1p);
        newp->rhsp(shift2p);
        newp->dtypeChgWidth(width, widthMin);  // The new AND must have width of the original SHIFT
        handle.relink(newp);
        iterate(newp);  // Further reduce, either node may have more reductions.
    }
    शून्य replaceShiftShift(Ast_गाठ_Biop* nodep) {
        UINFO(4, "SHIFT(SHIFT(a,s1),s2)->SHIFT(a,ADD(s1,s2)) " << nodep);
        UINFOTREE(9, nodep, "", "repShiftShift_old");
        Ast_गाठ_Biop* स्थिरांक lhsp = VN_AS(nodep->lhsp(), गाठ_Biop);
        lhsp->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक ap = lhsp->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक shift1p = lhsp->rhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक shift2p = nodep->rhsp()->unlinkFrBack();
        // Shift1p and shift2p may have different sizes, both are
        // self-determined so sum with infinite width
        जर (nodep->type() == lhsp->type()) {
            स्थिरांक पूर्णांक shift1 = VN_AS(shift1p, Const)->toUInt();
            स्थिरांक पूर्णांक shift2 = VN_AS(shift2p, Const)->toUInt();
            स्थिरांक पूर्णांक newshift = shift1 + shift2;
            VL_DO_DANGLING(ढकला_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ढकला_Deletep(shift2p), shift2p);
            nodep->lhsp(ap);
            nodep->rhsp(नवीन AstConst(nodep->fileline(), newshift));
            iterate(nodep);  // Further reduce, either node may have more reductions.
        } अन्यथा {
            // We know shift amounts are constant, but might be a mixed left/right shift
            पूर्णांक shift1 = VN_AS(shift1p, Const)->toUInt();
            जर (VN_IS(lhsp, ShiftR)) shift1 = -shift1;
            पूर्णांक shift2 = VN_AS(shift2p, Const)->toUInt();
            जर (VN_IS(nodep, ShiftR)) shift2 = -shift2;
            स्थिरांक पूर्णांक newshift = shift1 + shift2;
            VL_DO_DANGLING(ढकला_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ढकला_Deletep(shift2p), shift2p);
            Ast_गाठ_अभिव्यक्ती* newp;
            V3_संख्या mask1{nodep, nodep->width()};
            V3_संख्या ones{nodep, nodep->width()};
            ones.setMask(nodep->width());
            जर (shift1 < 0) {
                mask1.opShiftR(ones, V3_संख्या(nodep, VL_IDATASIZE, -shift1));
            } अन्यथा {
                mask1.opShiftL(ones, V3_संख्या(nodep, VL_IDATASIZE, shift1));
            }
            V3_संख्या mask{nodep, nodep->width()};
            जर (shift2 < 0) {
                mask.opShiftR(mask1, V3_संख्या(nodep, VL_IDATASIZE, -shift2));
            } अन्यथा {
                mask.opShiftL(mask1, V3_संख्या(nodep, VL_IDATASIZE, shift2));
            }
            जर (newshift < 0) {
                newp = नवीन AstShiftR{nodep->fileline(), ap,
                                     नवीन AstConst(nodep->fileline(), -newshift)};
            } अन्यथा {
                newp = नवीन AstShiftL{nodep->fileline(), ap,
                                     नवीन AstConst(nodep->fileline(), newshift)};
            }
            newp->dtypeFrom(nodep);
            newp = नवीन AstAnd{nodep->fileline(), newp, नवीन AstConst{nodep->fileline(), mask}};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            // UINFOTREE(1, newp, "", "repShiftShift_new");
            iterate(newp);  // Further reduce, either node may have more reductions.
        }
        VL_DO_DANGLING(ढकला_Deletep(lhsp), lhsp);
    }

    तार्किक replaceAssignMultiSel(Ast_गाठ_Assign* nodep) {
        // Multiple assignments to sequential bits can be concated
        // ASSIGN(SEL(a),aq), ASSIGN(SEL(a+1),bq) -> ASSIGN(SEL(a:b),CONCAT(aq,bq)
        // ie. assign var[2]=a, assign var[3]=b -> assign var[3:2]={b,a}

        // Skip if we're not const'ing an entire module (IE doing only one assign, etc)
        जर (!m_modp) परत असत्य;
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* स्थिरांक sel1p = VN_CAST(nodep->lhsp(), Sel);
        जर (!sel1p) परत असत्य;
        Ast_गाठ_Assign* स्थिरांक nextp = VN_CAST(nodep->nextp(), गाठ_Assign);
        जर (!nextp) परत असत्य;
        जर (nodep->type() != nextp->type()) परत असत्य;
        स्थिरांक AstSel* स्थिरांक sel2p = VN_CAST(nextp->lhsp(), Sel);
        जर (!sel2p) परत असत्य;
        AstVarRef* स्थिरांक varref1p = VN_CAST(sel1p->fromp(), VarRef);
        जर (!varref1p) परत असत्य;
        स्थिरांक AstVarRef* स्थिरांक varref2p = VN_CAST(sel2p->fromp(), VarRef);
        जर (!varref2p) परत असत्य;
        जर (!varref1p->same_Gate_वृक्ष(varref2p)) परत असत्य;
        स्थिरांक AstConst* स्थिरांक con1p = VN_CAST(sel1p->lsbp(), Const);
        जर (!con1p) परत असत्य;
        स्थिरांक AstConst* स्थिरांक con2p = VN_CAST(sel2p->lsbp(), Const);
        जर (!con2p) परत असत्य;
        // We need to make sure there's no self-references involved in either
        // assignment.  For speed, we only look 3 deep, then give up.
        जर (!varNotReferenced(nodep->rhsp(), varref1p->varp())) परत असत्य;
        जर (!varNotReferenced(nextp->rhsp(), varref2p->varp())) परत असत्य;
        // If a variable is marked split_var, access to the variable should not be merged.
        जर (varref1p->varp()->attrSplitVar() || varref2p->varp()->attrSplitVar()) परत असत्य;
        // Swap?
        जर ((con1p->toSInt() != con2p->toSInt() + sel2p->width())
            && (con2p->toSInt() != con1p->toSInt() + sel1p->width())) {
            परत असत्य;
        }
        स्थिरांक तार्किक lsbFirstAssign = (con1p->toUInt() < con2p->toUInt());
        UINFO(4, "replaceAssignMultiSel " << nodep);
        UINFO(4, "                   && " << nextp);
        // UINFOTREE(1, nodep, "", "comb1");
        // UINFOTREE(1, nextp, "", "comb2");
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhs2p = nextp->rhsp()->unlinkFrBack();
        Ast_गाठ_Assign* newp;
        जर (lsbFirstAssign) {
            newp = nodep->cloneType(नवीन AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel1p->lsbConst(), sel1p->width() + sel2p->width()},
                                    नवीन AstConcat{rhs1p->fileline(), rhs2p, rhs1p});
        } अन्यथा {
            newp = nodep->cloneType(नवीन AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel2p->lsbConst(), sel1p->width() + sel2p->width()},
                                    नवीन AstConcat{rhs1p->fileline(), rhs1p, rhs2p});
        }
        // UINFOTREE(1, pnewp, "", "conew");
        nodep->replaceWith(newp);  // dypep intentionally changing
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        VL_DO_DANGLING(ढकला_Deletep(nextp->unlinkFrBack()), nextp);
        परत सत्य;
    }

    तार्किक replaceAssignSFormatF(Ast_गाठ_Assign* nodep) {
        // Rewrite 'x = sformatf(...)' into 'sformat(x, ...)', which is more efficient
        // at the call site as it does not need to check if 'x' needs to be freed first.
        // This is a somewhat common pattern after lowering assertions.
        जर (!VN_IS(nodep, Assign)) परत असत्य;
        AstVarRef* स्थिरांक lhsp = VN_CAST(nodep->lhsp(), VarRef);
        जर (!lhsp) परत असत्य;
        AstSFormatF* स्थिरांक fmtp = VN_CAST(nodep->rhsp(), SFormatF);
        जर (!fmtp) परत असत्य;
        lhsp->unlinkFrBack();
        fmtp->unlinkFrBack();
        nodep->replaceWith(नवीन AstSFormat{nodep->fileline(), fmtp, lhsp});
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        परत सत्य;
    }

    तार्किक varNotReferenced(Ast_गाठ* nodep, AstVar* varp, पूर्णांक level = 0) {
        // Return true if varp never referenced under node.
        // Return false if referenced, or tree too deep to be worth it, or side effects
        जर (!nodep) परत सत्य;
        जर (level > 2) परत असत्य;
        जर (!nodep->isPure()) परत असत्य;  // For example a $fgetc can't be reordered
        जर (VN_IS(nodep, गाठ_Var_Ref) && VN_AS(nodep, गाठ_Var_Ref)->varp() == varp) परत असत्य;
        परत (varNotReferenced(nodep->nextp(), varp, level + 1)
                && varNotReferenced(nodep->op1p(), varp, level + 1)
                && varNotReferenced(nodep->op2p(), varp, level + 1)
                && varNotReferenced(nodep->op3p(), varp, level + 1)
                && varNotReferenced(nodep->op4p(), varp, level + 1));
    }

    तार्किक replace_गाठ_Assign(Ast_गाठ_Assign* nodep) {
        जर (VN_IS(nodep->lhsp(), VarRef) && VN_IS(nodep->rhsp(), VarRef)
            && VN_AS(nodep->lhsp(), VarRef)->sameNoLvalue(VN_AS(nodep->rhsp(), VarRef))
            && !VN_IS(nodep, AssignDly)) {
            // X = X.  Quite pointless, though X <= X may override another earlier assignment
            जर (VN_IS(nodep, AssignW)) {
                जर (!v3Global.opt.pedantic())
                    nodep->v3error(
                        "Wire inputs its own output, creating circular logic (wire x=x)");
                परत असत्य;  // Don't delete the assign, or V3Gate will freak out
            } अन्यथा {
                VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                परत सत्य;
            }
        } अन्यथा जर (m_doV && VN_IS(nodep->lhsp(), Concat)) {
            तार्किक need_temp = असत्य;
            // A blocking assignment with an intra-assignment timing control evaluates its value,
            // then waits once (IEEE 1800-2023 9.4.5), here to assign a temporary for the parts
            स्थिरांक तार्किक waits = VN_IS(nodep, Assign) && nodep->timingControlp();
            // Splitting copies the RHS once per part, so a large RHS would make the tree
            // grow with the product of the number of parts and the RHS size
            स्थिरांक तार्किक needTempPure = !nodep->rhsp()->isPure() || waits
                                      || nodep->rhsp()->isLargerThan(CONCAT_ASSIGN_MAX_RHS_NODES);
            जर (m_warn && !VN_IS(nodep, AssignDly)
                && !needTempPure) {  // Is same var on LHS and RHS?
                // If the rhs is not pure, we need a temporary variable anyway
                std::unordered_set<माप_प्रकार> lhsVarIds;
                nodep->lhsp()->foreach([&](स्थिरांक AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    lhsVarIds.emplace(m_ids(refp->varp()));
                });
                nodep->rhsp()->foreach([&](स्थिरांक AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    जर (lhsVarIds.मोजणी(m_ids(refp->varp()))) need_temp = सत्य;
                });
            }
            जर (needTempPure) {
                // if the RHS is impure or large we need to create a temporary variable for it,
                // because further handling involves copying of the RHS.
                UINFO(4, "  ASSITEMPPURE " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp,rhs),
                //                                ASSIGN(lc1,SEL(temp,{size1})),
                //                                ASSIGN(lc2,SEL(temp,{size2}))

                Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
                AstVar* स्थिरांक tempPurep = नवीन AstVar{rhsp->fileline(), VVarType::BLOCKTEMP,
                                                     m_concswapNames.get(rhsp), rhsp->dtypep()};
                // Static, as read right after written, also in a fork
                tempPurep->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempPurep);
                AstVarRef* स्थिरांक tempPureRefp
                    = नवीन AstVarRef{rhsp->fileline(), tempPurep, VAccess::WRITE};
                Ast_गाठ_Assign* स्थिरांक asnp
                    = VN_IS(nodep, AssignDly)
                          ? नवीन AstAssign{nodep->fileline(), tempPureRefp, rhsp}
                          : nodep->cloneType(tempPureRefp, rhsp);
                nodep->addHereThisAsNext(asnp);
                nodep->rhsp(नवीन AstVarRef{rhsp->fileline(), tempPurep, VAccess::READ});
                // Only the assignment of the temporary waits
                जर (waits) ढकला_Deletep(nodep->timingControlp()->unlinkFrBack());
            } अन्यथा जर (need_temp) {
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
            AstDelay* स्थिरांक delayp
                = VN_IS(nodep, AssignDly) ? VN_CAST(nodep->timingControlp(), Delay) : nullptr;
            जर (delayp && !delayp->lhsp()->isPure()) {
                File_ओळ* स्थिरांक flp = nodep->fileline();
                Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = delayp->lhsp()->unlinkFrBack();
                AstVar* स्थिरांक tempp = नवीन AstVar{flp, VVarType::BLOCKTEMP,
                                                 m_concswapNames.get(valuep), valuep->dtypep()};
                // Static, as read right after written, when the NBAs to the parts start waiting
                tempp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempp);
                nodep->addHereThisAsNext(
                    नवीन AstAssign{flp, नवीन AstVarRef{flp, tempp, VAccess::WRITE}, valuep});
                delayp->lhsp(नवीन AstVarRef{flp, tempp, VAccess::READ});
            }
            UINFOTREE(9, nodep, "", "Ass_old");
            // Unlink the stuff
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lc1p = VN_AS(nodep->lhsp(), Concat)->lhsp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lc2p = VN_AS(nodep->lhsp(), Concat)->rhsp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक conp = VN_AS(nodep->lhsp(), Concat)->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhs2p = rhsp->clone_वृक्ष_Pure(असत्य);
            // Calc widths
            स्थिरांक पूर्णांक lsb2 = 0;
            स्थिरांक पूर्णांक msb2 = lsb2 + lc2p->width() - 1;
            स्थिरांक पूर्णांक lsb1 = msb2 + 1;
            स्थिरांक पूर्णांक msb1 = lsb1 + lc1p->width() - 1;
            UASSERT_OBJ(msb1 == (conp->width() - 1), nodep, "Width calc mismatch");
            // Form ranges
            AstSel* स्थिरांक sel1p = नवीन AstSel{conp->fileline(), rhsp, lsb1, msb1 - lsb1 + 1};
            AstSel* स्थिरांक sel2p = नवीन AstSel{conp->fileline(), rhs2p, lsb2, msb2 - lsb2 + 1};
            // Make new assigns of same flavor as old one
            //*** Not cloneTree; just one node.
            Ast_गाठ_Assign* newp = nullptr;
            जर (!need_temp) {
                Ast_गाठ_Assign* स्थिरांक asn1ap = nodep->cloneType(lc1p, sel1p);
                Ast_गाठ_Assign* स्थिरांक asn2ap = nodep->cloneType(lc2p, sel2p);
                asn1ap->dtypeFrom(sel1p);
                asn2ap->dtypeFrom(sel2p);
                newp = Ast_गाठ::addNext(newp, asn1ap);
                newp = Ast_गाठ::addNext(newp, asn2ap);
            } अन्यथा {
                UASSERT_OBJ(m_modp, nodep, "Not under module");
                UASSERT_OBJ(m_globalPass, nodep,
                            "Should not reach here when not invoked on whole AstNetlist");
                // We could create just one temp variable, but we'll get better optimization
                // if we make one per term.
                AstVar* स्थिरांक temp1p
                    = नवीन AstVar{sel1p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel1p), VFlagLogicPacked{}, msb1 - lsb1 + 1};
                AstVar* स्थिरांक temp2p
                    = नवीन AstVar{sel2p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel2p), VFlagLogicPacked{}, msb2 - lsb2 + 1};
                m_modp->addStmtsp(temp1p);
                m_modp->addStmtsp(temp2p);
                Ast_गाठ_Assign* स्थिरांक asn1ap = nodep->cloneType(
                    नवीन AstVarRef{sel1p->fileline(), temp1p, VAccess::WRITE}, sel1p);
                Ast_गाठ_Assign* स्थिरांक asn2ap = nodep->cloneType(
                    नवीन AstVarRef{sel2p->fileline(), temp2p, VAccess::WRITE}, sel2p);
                Ast_गाठ_Assign* स्थिरांक asn1bp = nodep->cloneType(
                    lc1p, नवीन AstVarRef{sel1p->fileline(), temp1p, VAccess::READ});
                Ast_गाठ_Assign* स्थिरांक asn2bp = nodep->cloneType(
                    lc2p, नवीन AstVarRef{sel2p->fileline(), temp2p, VAccess::READ});
                asn1ap->dtypeFrom(temp1p);
                asn1bp->dtypeFrom(temp1p);
                asn2ap->dtypeFrom(temp2p);
                asn2bp->dtypeFrom(temp2p);
                // This order matters
                newp = Ast_गाठ::addNext(newp, asn1ap);
                newp = Ast_गाठ::addNext(newp, asn2ap);
                newp = Ast_गाठ::addNext(newp, asn1bp);
                newp = Ast_गाठ::addNext(newp, asn2bp);
            }
            जर (debug() >= 9 && newp) newp->dump_वृक्ष_And_Next(cout, "-     _new: ");
            nodep->addNextHere(newp);
            // Cleanup
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
            VL_DO_DANGLING(ढकला_Deletep(conp), conp);
            // Further reduce, either node may have more reductions.
            परत सत्य;
        } अन्यथा जर (m_doV && VN_IS(nodep->rhsp(), CvtPackedToArray)
                   && lowerAsFixedAggregate(nodep->lhsp()->dtypep())) {
            AstCvtPackedToArray* स्थिरांक cvtp
                = VN_AS(nodep->rhsp(), CvtPackedToArray)->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* srcp = cvtp->fromp()->unlinkFrBack();
            जर (lowerAsFixedAggregate(srcp->dtypep())) {
                srcp = packFixedAggregate(srcp);
            } अन्यथा जर (Ast_गाठ_Stream* स्थिरांक streamp = VN_CAST(srcp, गाठ_Stream)) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक streamSrcp = streamp->lhsp();
                जर (lowerAsFixedAggregate(streamSrcp->dtypep())) {
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक packedp = packFixedAggregate(streamSrcp->unlinkFrBack());
                    streamp->lhsp(packedp);
                    streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                       VSigning::UNSIGNED,
                                                       packedp->isFourstateOrNoDtype());
                }
            }
            VL_DO_DANGLING(ढकला_Deletep(cvtp), cvtp);
            replaceAssignToFixedAggregate(nodep, nodep->lhsp()->unlinkFrBack(), srcp);
            परत सत्य;
        } अन्यथा जर (m_doV && VN_IS(nodep->rhsp(), StreamR)
                   && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            // The right-streaming operator on rhs of assignment does not
            // change the order of bits. Eliminate stream but keep its lhsp.
            // Add a cast if needed.
            AstStreamR* स्थिरांक streamp = VN_AS(nodep->rhsp(), StreamR)->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* srcp = streamp->lhsp()->unlinkFrBack();
            Ast_गाठ_DType* स्थिरांक srcDTypep = srcp->dtypep()->skipRefp();
            स्थिरांक Ast_गाठ_DType* स्थिरांक dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            जर (lowerAsFixedAggregate(srcDTypep)) {
                srcp = packFixedAggregate(srcp);
            } अन्यथा जर (VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)) {
                जर (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    पूर्णांक srcElementBits = 0;
                    जर (स्थिरांक Ast_गाठ_DType* स्थिरांक elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    पूर्णांक dstElementBits = 0;
                    जर (स्थिरांक Ast_गाठ_DType* स्थिरांक elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    srcp = नवीन AstCvtArrayToArray{
                        srcp->fileline(), srcp,          nodep->dtypep(), असत्य, 1,
                        dstElementBits,   srcElementBits};
                } अन्यथा {
                    srcp = नवीन AstCvtArrayToPacked{srcp->fileline(), srcp, nodep->dtypep()};
                }
            } अन्यथा जर (VN_IS(srcDTypep, UnpackArrayDType)) {
                srcp = नवीन AstCvtArrayToPacked{srcp->fileline(), srcp, srcDTypep};
                // Handling the case where lhs is wider than rhs by inserting zeros. StreamL does
                // not require this, since the left streaming operator implicitly handles this.
                स्थिरांक पूर्णांक packedBits = nodep->lhsp()->widthMin();
                स्थिरांक पूर्णांक unpackBits
                    = srcDTypep->arrayUnpackedElements() * srcDTypep->subDTypep()->widthMin();
                स्थिरांक uint32_t offset = packedBits > unpackBits ? packedBits - unpackBits : 0;
                srcp = नवीन AstShiftL{srcp->fileline(), srcp,
                                     नवीन AstConst{srcp->fileline(), offset}, packedBits};
            }
            जर (!VN_IS(dstDTypep, UnpackArrayDType) && !VN_IS(dstDTypep, QueueDType)
                && !VN_IS(dstDTypep, DynArrayDType)) {
                स्थिरांक पूर्णांक sWidth = srcp->width();
                स्थिरांक पूर्णांक dWidth = nodep->lhsp()->width();
                जर (sWidth < dWidth) {
                    AstExtend* स्थिरांक extendp = नवीन AstExtend{srcp->fileline(), srcp, dWidth};
                    srcp = नवीन AstShiftL{
                        srcp->fileline(), extendp,
                        नवीन AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                        dWidth};
                }
            }
            nodep->rhsp(srcp);
            VL_DO_DANGLING(ढकला_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            परत सत्य;
        } अन्यथा जर (m_doV && VN_IS(nodep->lhsp(), StreamL)) {
            // Push the stream operator to the rhs of the assignment statement
            Ast_गाठ_अभिव्यक्ती* streamp = nodep->lhsp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक dstp = VN_AS(streamp, StreamL)->lhsp()->unlinkFrBack();
            Ast_गाठ_DType* स्थिरांक dstDTypep = dstp->dtypep()->skipRefp();
            Ast_गाठ_अभिव्यक्ती* srcp = nodep->rhsp()->unlinkFrBack();
            स्थिरांक Ast_गाठ_DType* स्थिरांक srcDTypep = srcp->dtypep()->skipRefp();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamL reverses, so reverse=true)
            जर ((VN_IS(srcDTypep, UnpackArrayDType) || VN_IS(srcDTypep, QueueDType)
                 || VN_IS(srcDTypep, DynArrayDType))
                && (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType))) {
                पूर्णांक blockSize = 1;
                जर (स्थिरांक AstConst* स्थिरांक constp
                    = VN_CAST(VN_AS(streamp, StreamL)->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    जर (VL_UNLIKELY(blockSize <= 0)) blockSize = 1;
                }
                पूर्णांक srcElementBits = 0;
                जर (स्थिरांक Ast_गाठ_DType* स्थिरांक elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                पूर्णांक dstElementBits = 0;
                जर (स्थिरांक Ast_गाठ_DType* स्थिरांक elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                nodep->lhsp(dstp);
                nodep->rhsp(नवीन AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, सत्य,
                                                   blockSize, dstElementBits, srcElementBits});
                nodep->dtypep(dstDTypep);
                VL_DO_DANGLING(ढकला_Deletep(streamp), streamp);
                परत सत्य;
            }
            जर (lowerAsFixedAggregate(srcDTypep)) srcp = packFixedAggregate(srcp);
            स्थिरांक पूर्णांक sWidth = srcp->width();
            स्थिरांक पूर्णांक dWidth = dstp->width();
            // Connect the rhs to the stream operator and update its width
            VN_AS(streamp, StreamL)->lhsp(srcp);
            जर (VN_IS(srcDTypep, DynArrayDType) || VN_IS(srcDTypep, QueueDType)
                || VN_IS(srcDTypep, UnpackArrayDType)) {
                streamp->dtypeSetStream();
            } अन्यथा {
                streamp->dtypeSetBitOrLogicUnsized(srcp->width(), srcp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   srcp->isFourstateOrNoDtype());
            }
            जर (VN_IS(dstDTypep, UnpackArrayDType)) {
                streamp = नवीन AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
            } अन्यथा {
                जर (dWidth == 0) {
                    streamp = नवीन AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
                } अन्यथा जर (sWidth >= dWidth) {
                    streamp = नवीन AstSel{streamp->fileline(), streamp, sWidth - dWidth, dWidth};
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(streamp);
            nodep->dtypep(dstDTypep);
            परत सत्य;
        } अन्यथा जर (m_doV && VN_IS(nodep->lhsp(), StreamR)) {
            // The right stream operator on lhs of assignment statement does
            // not reorder bits. However, if the rhs is wider than the lhs,
            // then we select bits from the left-most, not the right-most.
            Ast_गाठ_अभिव्यक्ती* स्थिरांक streamp = nodep->lhsp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक dstp = VN_AS(streamp, StreamR)->lhsp()->unlinkFrBack();
            Ast_गाठ_DType* स्थिरांक dstDTypep = dstp->dtypep()->skipRefp();
            Ast_गाठ_अभिव्यक्ती* srcp = nodep->rhsp()->unlinkFrBack();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamR does not reverse, so reverse=false).
            // V3Width may have wrapped the source in CvtArrayToPacked; unwrap it.
            जर (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                Ast_गाठ_अभिव्यक्ती* origSrcp = srcp;
                जर (AstCvtArrayToPacked* स्थिरांक cvtp = VN_CAST(srcp, CvtArrayToPacked)) {
                    origSrcp = cvtp->fromp();
                }
                स्थिरांक Ast_गाठ_DType* स्थिरांक origSrcDTypep = origSrcp->dtypep()->skipRefp();
                जर (VN_IS(origSrcDTypep, UnpackArrayDType) || VN_IS(origSrcDTypep, QueueDType)
                    || VN_IS(origSrcDTypep, DynArrayDType)) {
                    पूर्णांक srcElementBits = 0;
                    जर (स्थिरांक Ast_गाठ_DType* स्थिरांक elemDtp = origSrcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    पूर्णांक dstElementBits = 0;
                    जर (स्थिरांक Ast_गाठ_DType* स्थिरांक elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    जर (VN_IS(srcp, CvtArrayToPacked)) {
                        origSrcp = VN_AS(srcp, CvtArrayToPacked)->fromp()->unlinkFrBack();
                        VL_DO_DANGLING(ढकला_Deletep(srcp), srcp);
                        srcp = origSrcp;
                    }
                    // Descending unpacked arrays need element reversal
                    तार्किक reverse = असत्य;
                    जर (स्थिरांक AstUnpackArrayDType* स्थिरांक unpackDtp
                        = VN_CAST(origSrcDTypep, UnpackArrayDType)) {
                        reverse = !unpackDtp->declRange().ascending();
                    }
                    nodep->lhsp(dstp);
                    nodep->rhsp(नवीन AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, reverse,
                                                       1, dstElementBits, srcElementBits});
                    nodep->dtypep(dstDTypep);
                    VL_DO_DANGLING(ढकला_Deletep(streamp), streamp);
                    परत सत्य;
                }
            }
            स्थिरांक पूर्णांक sWidth = srcp->width();
            स्थिरांक पूर्णांक dWidth = dstp->width();
            जर (VN_IS(dstDTypep, UnpackArrayDType)) {
                स्थिरांक पूर्णांक dstBitWidth
                    = dWidth * VN_AS(dstDTypep, UnpackArrayDType)->arrayUnpackedElements();
                // Handling the case where rhs is wider than lhs. StreamL does not require this
                // since the combination of the left streaming operation and the implicit
                // truncation in VL_ASSIGN_UNPACK automatically selects the left-most bits.
                जर (sWidth > dstBitWidth) {
                    srcp
                        = नवीन AstSel{streamp->fileline(), srcp, sWidth - dstBitWidth, dstBitWidth};
                }
                srcp = नवीन AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
            } अन्यथा {
                जर (dWidth == 0) {
                    srcp = नवीन AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
                } अन्यथा जर (sWidth >= dWidth) {
                    srcp = नवीन AstSel{streamp->fileline(), srcp, sWidth - dWidth, dWidth};
                } अन्यथा {
                    // Source narrower than destination: left-justify by shifting left.
                    // The right stream operator packs left-to-right, so remaining
                    // LSBs are zero-filled (IEEE 1800-2023 11.4.14.3).
                    जर (!VN_IS(srcp->dtypep()->skipRefp(), QueueDType)) {
                        AstExtend* स्थिरांक extendp = नवीन AstExtend{srcp->fileline(), srcp, dWidth};
                        srcp = नवीन AstShiftL{
                            srcp->fileline(), extendp,
                            नवीन AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                            dWidth};
                    } अन्यथा {  // if it is a queue we dont need do do a shiftL
                        srcp = नवीन AstExtend{srcp->fileline(), srcp};
                    }
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(srcp);
            nodep->dtypep(dstDTypep);
            VL_DO_DANGLING(ढकला_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            परत सत्य;
        } अन्यथा जर (m_doV && VN_IS(nodep->rhsp(), StreamL)) {
            AstStreamL* streamp = VN_AS(nodep->rhsp(), StreamL);
            Ast_गाठ_अभिव्यक्ती* srcp = streamp->lhsp();
            स्थिरांक Ast_गाठ_DType* स्थिरांक srcDTypep = srcp->dtypep()->skipRefp();
            Ast_गाठ_DType* स्थिरांक dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            जर (lowerAsFixedAggregate(srcDTypep)) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक packedp = packFixedAggregate(srcp->unlinkFrBack());
                streamp->lhsp(packedp);
                streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   packedp->isFourstateOrNoDtype());
                srcp = packedp;
            } अन्यथा जर ((VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)
                        || VN_IS(srcDTypep, UnpackArrayDType))) {
                जर (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    पूर्णांक blockSize = 1;
                    जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(streamp->rhsp(), Const)) {
                        blockSize = constp->toSInt();
                        जर (VL_UNLIKELY(blockSize <= 0)) {
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
                    पूर्णांक srcElementBits = 0;
                    जर (स्थिरांक Ast_गाठ_DType* स्थिरांक elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    पूर्णांक dstElementBits = 0;
                    जर (स्थिरांक Ast_गाठ_DType* स्थिरांक elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    streamp->unlinkFrBack();
                    srcp = नवीन AstCvtArrayToArray{
                        srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     सत्य,
                        blockSize,        dstElementBits,       srcElementBits};
                    nodep->rhsp(srcp);
                    VL_DO_DANGLING(ढकला_Deletep(streamp), streamp);
                } अन्यथा {
                    streamp->lhsp(नवीन AstCvtArrayToPacked{srcp->fileline(), srcp->unlinkFrBack(),
                                                          dstDTypep});
                    streamp->dtypeFrom(dstDTypep);
                }
            }
        } अन्यथा जर (m_doV && replaceAssignMultiSel(nodep)) {
            परत सत्य;
        } अन्यथा जर (replaceAssignSFormatF(nodep)) {
            परत सत्य;
        }
        परत असत्य;
    }

    // Boolean replacements
    तार्किक operandBoolShift(स्थिरांक Ast_गाठ* nodep) {
        // boolean test of AND(const,SHIFTR(x,const)) -> test of AND(SHIFTL(x,const), x)
        जर (!VN_IS(nodep, And)) परत असत्य;
        जर (!VN_IS(VN_AS(nodep, And)->lhsp(), Const)) परत असत्य;
        जर (!VN_IS(VN_AS(nodep, And)->rhsp(), ShiftR)) परत असत्य;
        स्थिरांक AstShiftR* स्थिरांक shiftp = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR);
        जर (!VN_IS(shiftp->rhsp(), Const)) परत असत्य;
        जर (static_cast<uint32_t>(nodep->width()) <= VN_AS(shiftp->rhsp(), Const)->toUInt()) {
            परत असत्य;
        }
        परत सत्य;
    }
    शून्य replaceBoolShift(Ast_गाठ* nodep) {
        UINFOTREE(9, nodep, "", "bshft_old");
        AstConst* स्थिरांक andConstp = VN_AS(VN_AS(nodep, And)->lhsp(), Const);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp
            = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->lhsp()->unlinkFrBack();
        AstConst* स्थिरांक shiftConstp
            = VN_AS(VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->rhsp(), Const);
        V3_संख्या val{andConstp, andConstp->width()};
        val.opShiftL(andConstp->अंक(), shiftConstp->अंक());
        AstAnd* स्थिरांक newp
            = नवीन AstAnd{nodep->fileline(), नवीन AstConst{nodep->fileline(), val}, fromp};
        // widthMin no longer applicable if different C-expanded width
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        UINFOTREE(9, newp, "", "_new");
    }

    शून्य replaceWithSimulation(Ast_गाठ* nodep) {
        SimulateVisitor simvis;
        // Run it - may be unoptimizable due to large for loop, etc
        simvis.mainParamEmulate(nodep);
        जर (!simvis.optimizable()) {
            स्थिरांक Ast_गाठ* errorp = simvis.whyNotNodep();
            जर (!errorp) errorp = nodep;
            nodep->v3error("Expecting expression to be constant, but can't determine constant for "
                           << nodep->prettyTypeName() << '\n'
                           << errorp->warnOther() << "... Location of non-constant "
                           << errorp->prettyTypeName() << ": " << simvis.whyNotMessage());
            VL_DO_DANGLING(replaceZero(nodep), nodep);
        } अन्यथा {
            // Fetch the result
            Ast_गाठ* स्थिरांक valuep = simvis.fetch_मूल्य_Null(nodep);  // valuep is owned by Simulate
            UASSERT_OBJ(valuep, nodep, "No value returned from simulation");
            // Replace it
            Ast_गाठ* स्थिरांक newp = valuep->clone_वृक्ष(असत्य);
            newp->fileline(nodep->fileline());
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Simulate->" << newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }

    //----------------------------------------

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        iterateChildrenBackwardsConst(nodep);
    }
    शून्य visit(Ast_गाठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_concswapNames.reset();
        iterateChildren(nodep);
    }
    शून्य visit(AstCFunc* nodep) override {
        // No ASSIGNW removals under funcs, we've long eliminated INITIALs
        // (We should perhaps rename the assignw's to just assigns)
        VL_RESTORER(m_wremove);
        m_wremove = असत्य;
        iterateChildren(nodep);
    }
    शून्य visit(AstCLocalScope* nodep) override {
        iterateChildren(nodep);
        जर (!nodep->stmtsp()) {
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }
    शून्य visit(AstScope* nodep) override {
        // No ASSIGNW removals under scope, we've long eliminated INITIALs
        VL_RESTORER(m_wremove);
        VL_RESTORER(m_scopep);
        m_wremove = असत्य;
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    शून्य swapSides(Ast_गाठ_Bi_Com* nodep) {
        // COMMUTATIVE({a},CONST) -> COMMUTATIVE(CONST,{a})
        // This simplifies later optimizations
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp()->unlinkFrBackWithNext();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBackWithNext();
        nodep->lhsp(rhsp);
        nodep->rhsp(lhsp);
        iterate(nodep);  // Again?
    }

    तार्किक containsMemberAccessRecurse(Ast_गाठ* स्थिरांक nodep) {
        जर (!nodep) परत असत्य;
        स्थिरांक माप_प्रकार id = m_ids(nodep);
        स्थिरांक स्वतः it = m_containsMemberAccess.find(id);
        जर (it != m_containsMemberAccess.end()) परत it->second;
        ++m_statMemberAccessVisits;
        तार्किक result = असत्य;
        जर (VN_IS(nodep, MemberSel) || VN_IS(nodep, MethodCall) || VN_IS(nodep, CMethodCall)) {
            result = सत्य;
        } अन्यथा जर (स्थिरांक Ast_गाठ_FTask_Ref* स्थिरांक funcRefp = VN_CAST(nodep, गाठ_FTask_Ref)) {
            जर (containsMemberAccessRecurse(funcRefp->taskp())) result = सत्य;
        } अन्यथा जर (स्थिरांक Ast_गाठ_CCall* स्थिरांक funcRefp = VN_CAST(nodep, गाठ_CCall)) {
            जर (containsMemberAccessRecurse(funcRefp->funcp())) result = सत्य;
        } अन्यथा जर (स्थिरांक Ast_गाठ_FTask* स्थिरांक funcp = VN_CAST(nodep, गाठ_FTask)) {
            // Assume that it has a member access
            जर (funcp->recursive()) result = सत्य;
        } अन्यथा जर (स्थिरांक AstCFunc* स्थिरांक funcp = VN_CAST(nodep, CFunc)) {
            जर (funcp->recursive()) result = सत्य;
        }
        जर (!result) {
            result = containsMemberAccessRecurse(nodep->op1p())
                     || containsMemberAccessRecurse(nodep->op2p())
                     || containsMemberAccessRecurse(nodep->op3p())
                     || containsMemberAccessRecurse(nodep->op4p());
        }
        जर (!result && !VN_IS(nodep, गाठ_FTask)
            && !VN_IS(nodep, CFunc)  // don't enter into next function
            && containsMemberAccessRecurse(nodep->nextp())) {
            result = सत्य;
        }
        m_containsMemberAccess.emplace(id, result);
        परत result;
    }

    तार्किक matchBiopToBitwise(Ast_गाठ_Biop* स्थिरांक nodep) {
        जर (!m_convertLogicToBit) परत असत्य;
        जर (!nodep->lhsp()->width1()) परत असत्य;
        जर (!nodep->rhsp()->width1()) परत असत्य;
        जर (!nodep->isPure()) परत असत्य;
        जर (containsMemberAccessRecurse(nodep)) परत असत्य;
        परत सत्य;
    }
    तार्किक matchConcatRand(AstConcat* nodep) {
        //    CONCAT(RAND, RAND) - created by Chisel code
        AstRand* स्थिरांक aRandp = VN_CAST(nodep->lhsp(), Rand);
        स्थिरांक AstRand* स्थिरांक bRandp = VN_CAST(nodep->rhsp(), Rand);
        जर (!aRandp || !bRandp) परत असत्य;
        जर (!aRandp->combinable(bRandp)) परत असत्य;
        UINFO(4, "Concat(Rand,Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        परत सत्य;
    }
    तार्किक matchSelRand(AstSel* nodep) {
        //    SEL(RAND) - created by Chisel code
        AstRand* स्थिरांक aRandp = VN_CAST(nodep->fromp(), Rand);
        जर (!aRandp) परत असत्य;
        जर (aRandp->seedp()) परत असत्य;
        UINFO(4, "Sel(Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        परत सत्य;
    }
    तार्किक matchToStringNConst(AstToStringN* nodep) {
        iterateChildren(nodep);
        जर (स्थिरांक AstConsArray* स्थिरांक consp = VN_CAST(nodep->lhsp(), ConsArray)) {
            जर (!(m_doExpensive || m_params)) परत असत्य;
            जर (!consp->is_Constant_वृक्ष()) परत असत्य;
        } अन्यथा जर (!VN_IS(nodep->lhsp(), Const)) {
            परत असत्य;
        }
        replaceWithSimulation(nodep);
        परत सत्य;
    }
    पूर्णांक operandConcatMove(स्थिरांक AstConcat* nodep) {
        //    CONCAT under concat  (See moveConcat)
        // Return value: true indicates to do it; 2 means move to LHS
        स्थिरांक AstConcat* स्थिरांक abConcp = VN_CAST(nodep->lhsp(), Concat);
        स्थिरांक AstConcat* स्थिरांक bcConcp = VN_CAST(nodep->rhsp(), Concat);
        जर (!abConcp && !bcConcp) परत 0;
        जर (bcConcp) {
            स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक ap = nodep->lhsp();
            स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक bp = bcConcp->lhsp();
            // If a+b == 32,64,96 etc, then we want to have a+b together on LHS
            जर (VL_BITBIT_I(ap->width() + bp->width()) == 0) परत 2;  // Transform 2: to abConc
        } अन्यथा {  // abConcp
            // Unless lhs is already 32 bits due to above, reorder it
            जर (VL_BITBIT_I(nodep->lhsp()->width()) != 0) परत 1;  // Transform 1: to bcConc
        }
        परत 0;  // ok
    }
    शून्य moveConcat(AstConcat* nodep) {
        //    1: CONCAT(CONCAT({a},{b}),{c})  -> CONCAT({a},CONCAT({b}, {c}))
        // or 2: CONCAT({a}, CONCAT({b},{c})) -> CONCAT(CONCAT({a},{b}),{c})
        // Because the lhs of a concat needs a shift but the rhs doesn't,
        // putting additional CONCATs on the RHS leads to fewer assembler operations.
        // However, we'll end up with lots of wide moves if we make huge trees
        // like that, so on 32 bit boundaries, we'll do the opposite form.
        UINFO(4, "Move concat: " << nodep);
        जर (operandConcatMove(nodep) > 1) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक ap = nodep->lhsp()->unlinkFrBack();
            AstConcat* स्थिरांक bcConcp = VN_AS(nodep->rhsp(), Concat);
            bcConcp->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक bp = bcConcp->lhsp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक cp = bcConcp->rhsp()->unlinkFrBack();
            AstConcat* स्थिरांक abConcp = नवीन AstConcat{bcConcp->fileline(), ap, bp};
            nodep->lhsp(abConcp);
            nodep->rhsp(cp);
            // If bp was a concat, then we have this exact same form again!
            // Recurse rather then calling node->iterate to prevent 2^n recursion!
            जर (operandConcatMove(abConcp)) moveConcat(abConcp);
            VL_DO_DANGLING(ढकला_Deletep(bcConcp), bcConcp);
        } अन्यथा {
            AstConcat* स्थिरांक abConcp = VN_AS(nodep->lhsp(), Concat);
            abConcp->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक ap = abConcp->lhsp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक bp = abConcp->rhsp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक cp = nodep->rhsp()->unlinkFrBack();
            AstConcat* स्थिरांक bcConcp = नवीन AstConcat{abConcp->fileline(), bp, cp};
            nodep->lhsp(ap);
            nodep->rhsp(bcConcp);
            जर (operandConcatMove(bcConcp)) moveConcat(bcConcp);
            VL_DO_DANGLING(ढकला_Deletep(abConcp), abConcp);
        }
    }

    // Special cases
    शून्य visit(AstConst*) override {}  // Already constant

    शून्य visit(AstCell* nodep) override {
        जर (m_params) {
            iterateAndNextNull(nodep->paramsp());
        } अन्यथा {
            iterateChildren(nodep);
        }
    }
    शून्य visit(AstClassOrPackageRef* nodep) override { iterateChildren(nodep); }

    शून्य visit(AstMatchMasked* nodep) override {
        // Do not iterate the tables, they must be constant pool entries
        iterate(nodep->lhsp());
        जर (AstConst* स्थिरांक constp = VN_CAST(nodep->lhsp(), Const)) {
            replace_अंक(nodep, AstMatchMasked::fold(constp->अंक(), nodep->matchp()->varp()));
        }
    }

    शून्य visit(AstPin* nodep) override { iterateChildren(nodep); }

    शून्य replaceLogEq(AstLogEq* nodep) {
        // LOGEQ(a,b) => AstLogAnd{AstLogOr{AstLogNot{a},b},AstLogOr{AstLogNot{b},a}}
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
        // Do exactly as IEEE says, might result in extra terms, so in future may do differently
        AstLogAnd* स्थिरांक newp = नवीन AstLogAnd{
            nodep->fileline(),
            नवीन AstLogOr{nodep->fileline(), नवीन AstLogNot{nodep->fileline(), lhsp}, rhsp},
            नवीन AstLogOr{nodep->fileline(),
                         नवीन AstLogNot{nodep->fileline(), rhsp->clone_वृक्ष_Pure(असत्य)},
                         lhsp->clone_वृक्ष_Pure(असत्य)}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    शून्य replaceSelSel(AstSel* nodep) {
        // SEL(SEL({x},a,b),c,d) => SEL({x},a+c,d)
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* स्थिरांक belowp = VN_AS(nodep->fromp(), Sel);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = belowp->fromp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lsb1p = nodep->lsbp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lsb2p = belowp->lsbp()->unlinkFrBack();
        // Eliminate lower range
        UINFO(4, "Elim Lower range: " << nodep);
        Ast_गाठ_अभिव्यक्ती* newlsbp;
        जर (VN_IS(lsb1p, Const) && VN_IS(lsb2p, Const)) {
            newlsbp = नवीन AstConst{lsb1p->fileline(),
                                   VN_AS(lsb1p, Const)->toUInt() + VN_AS(lsb2p, Const)->toUInt()};
            VL_DO_DANGLING(ढकला_Deletep(lsb1p), lsb1p);
            VL_DO_DANGLING(ढकला_Deletep(lsb2p), lsb2p);
        } अन्यथा {
            // Width is important, we need the width of the fromp's expression, not the
            // potentially smaller lsb1p's width, but don't insert a redundant AstExtend.
            // Note that due to some sloppiness in earlier passes, lsb1p might actually be wider,
            // so extend to the wider type.
            स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक widep = lsb1p->width() > lsb2p->width() ? lsb1p : lsb2p;
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = widep->width() > lsb2p->width()
                                          ? नवीन AstExtend{lsb2p->fileline(), lsb2p}
                                          : lsb2p;
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = widep->width() > lsb1p->width()
                                          ? नवीन AstExtend{lsb1p->fileline(), lsb1p}
                                          : lsb1p;
            lhsp->dtypeFrom(widep);
            rhsp->dtypeFrom(widep);
            newlsbp = नवीन AstAdd{lsb1p->fileline(), lhsp, rhsp};
            newlsbp->dtypeFrom(widep);
        }
        AstSel* स्थिरांक newp = नवीन AstSel{nodep->fileline(), fromp, newlsbp, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    शून्य replaceSelConcat(AstSel* nodep) {
        // SEL(CONCAT(a,b),c,d) => SEL(a or b, . .)
        AstConcat* स्थिरांक conp = VN_AS(nodep->fromp(), Concat);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक conLhsp = conp->lhsp();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक conRhsp = conp->rhsp();
        जर (static_cast<पूर्णांक>(nodep->lsbConst()) >= conRhsp->width()) {
            conLhsp->unlinkFrBack();
            AstSel* स्थिरांक newp
                = नवीन AstSel{nodep->fileline(), conLhsp, nodep->lsbConst() - conRhsp->width(),
                             nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } अन्यथा जर (static_cast<पूर्णांक>(nodep->msbConst()) < conRhsp->width()) {
            conRhsp->unlinkFrBack();
            AstSel* स्थिरांक newp
                = नवीन AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(), nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } अन्यथा {
            // Yuk, split between the two
            conRhsp->unlinkFrBack();
            conLhsp->unlinkFrBack();
            AstConcat* स्थिरांक newp
                = नवीन AstConcat{nodep->fileline(),
                                नवीन AstSel{nodep->fileline(), conLhsp, 0,
                                           nodep->msbConst() - conRhsp->width() + 1},
                                नवीन AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(),
                                           conRhsp->width() - nodep->lsbConst()}};
            nodep->replaceWithKeepDType(newp);
        }
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    तार्किक operandSelReplicate(AstSel* nodep) {
        // SEL(REPLICATE(from,rep),lsb,width) => SEL(from,0,width) as long
        // as SEL's width <= b's width
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* स्थिरांक repp = VN_AS(nodep->fromp(), Replicate);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = repp->srcp();
        स्थिरांक AstConst* स्थिरांक lsbp = VN_CAST(nodep->lsbp(), Const);
        जर (!lsbp) परत असत्य;
        UASSERT_OBJ(fromp->width(), nodep, "Not widthed");
        जर ((lsbp->toUInt() / fromp->width())
            != ((lsbp->toUInt() + nodep->width() - 1) / fromp->width())) {
            परत असत्य;
        }
        //
        fromp->unlinkFrBack();
        AstSel* स्थिरांक newp = नवीन AstSel{
            nodep->fileline(), fromp,
            नवीन AstConst{lsbp->fileline(), lsbp->toUInt() % fromp->width()}, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        परत सत्य;
    }
    तार्किक operandRepRep(AstReplicate* nodep) {
        // REPLICATE(REPLICATE2(from2,cnt2),cnt1) => REPLICATE(from2,(cnt1+cnt2))
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* स्थिरांक rep2p = VN_AS(nodep->srcp(), Replicate);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक from2p = rep2p->srcp();
        स्थिरांक AstConst* स्थिरांक cnt1p = VN_CAST(nodep->countp(), Const);
        जर (!cnt1p) परत असत्य;
        स्थिरांक AstConst* स्थिरांक cnt2p = VN_CAST(rep2p->countp(), Const);
        जर (!cnt2p) परत असत्य;
        //
        from2p->unlinkFrBack();
        AstReplicate* स्थिरांक newp
            = नवीन AstReplicate{nodep->fileline(), from2p, cnt1p->toUInt() * cnt2p->toUInt()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        परत सत्य;
    }
    तार्किक operandConcatSame(AstConcat* nodep) {
        // CONCAT(fromp,fromp) -> REPLICATE(fromp,1+1)
        // CONCAT(REP(fromp,cnt1),fromp) -> REPLICATE(fromp,cnt1+1)
        // CONCAT(fromp,REP(fromp,cnt1)) -> REPLICATE(fromp,1+cnt1)
        // CONCAT(REP(fromp,cnt1),REP(fromp,cnt2)) -> REPLICATE(fromp,cnt1+cnt2)
        Ast_गाठ_अभिव्यक्ती* from1p = nodep->lhsp();
        uint32_t cnt1 = 1;
        Ast_गाठ_अभिव्यक्ती* from2p = nodep->rhsp();
        uint32_t cnt2 = 1;
        जर (VN_IS(from1p, Replicate)) {
            स्थिरांक AstConst* स्थिरांक cnt1p = VN_CAST(VN_CAST(from1p, Replicate)->countp(), Const);
            जर (!cnt1p) परत असत्य;
            from1p = VN_AS(from1p, Replicate)->srcp();
            cnt1 = cnt1p->toUInt();
        }
        जर (VN_IS(from2p, Replicate)) {
            स्थिरांक AstConst* स्थिरांक cnt2p = VN_CAST(VN_CAST(from2p, Replicate)->countp(), Const);
            जर (!cnt2p) परत असत्य;
            from2p = VN_AS(from2p, Replicate)->srcp();
            cnt2 = cnt2p->toUInt();
        }
        जर (!operandsSame(from1p, from2p)) परत असत्य;
        //
        from1p->unlinkFrBack();
        AstReplicate* स्थिरांक newp = नवीन AstReplicate{nodep->fileline(), from1p, cnt1 + cnt2};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        परत सत्य;
    }
    शून्य replaceSelIntoBiop(AstSel* nodep) {
        // SEL(BUFIF1(a,b),1,bit) => BUFIF1(SEL(a,1,bit),SEL(b,1,bit))
        Ast_गाठ_Biop* स्थिरांक fromp = VN_AS(nodep->fromp()->unlinkFrBack(), गाठ_Biop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_गाठ_अभिव्यक्ती* स्थिरांक bilhsp = fromp->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक birhsp = fromp->rhsp()->unlinkFrBack();
        //
        fromp->lhsp(
            नवीन AstSel{nodep->fileline(), bilhsp, lsbp->clone_वृक्ष_Pure(सत्य), nodep->widthConst()});
        fromp->rhsp(नवीन AstSel{nodep->fileline(), birhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य replaceSelIntoUniop(AstSel* nodep) {
        // SEL(NOT(a),1,bit) => NOT(SEL(a,bit))
        Ast_गाठ_Uniop* स्थिरांक fromp = VN_AS(nodep->fromp()->unlinkFrBack(), गाठ_Uniop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_गाठ_अभिव्यक्ती* स्थिरांक bilhsp = fromp->lhsp()->unlinkFrBack();
        //
        fromp->lhsp(नवीन AstSel{nodep->fileline(), bilhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    शून्य visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        iterateChildren(nodep);
    }

    शून्य visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->bitp());
        जर (VN_IS(nodep->bitp(), Const)
            && VN_IS(nodep->fromp(), VarRef)
            // Need to make sure it's an array object so don't mis-allow a constant (bug509.)
            && VN_AS(nodep->fromp(), VarRef)->varp()
            && VN_IS(VN_AS(nodep->fromp(), VarRef)->varp()->valuep(), ConsArray)) {
            m_selp = nodep;  // Ask visit(AstVarRef) to replace varref with const
        }
        iterateAndNextNull(nodep->fromp());
        जर (VN_IS(nodep->fromp(), Const)) {  // It did.
            जर (!m_selp) {
                nodep->v3error("Illegal assignment of constant to unpacked array");
            } अन्यथा {
                Ast_गाठ* स्थिरांक fromp = nodep->fromp()->unlinkFrBack();
                nodep->replaceWithKeepDType(fromp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            }
        }
        // Handle ARRAYSEL directly on ConsArray (not through VarRef)
        अन्यथा जर (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ConsArray)) {
            स्थिरांक AstConsArray* स्थिरांक consp = VN_AS(nodep->fromp(), ConsArray);
            स्थिरांक uint32_t bit = VN_AS(nodep->bitp(), Const)->toUInt();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक itemp = consp->valuep(bit);
            जर (VN_IS(itemp, Const)) {
                स्थिरांक V3_संख्या& अंक = VN_AS(itemp, Const)->अंक();
                VL_DO_DANGLING(replace_अंक(nodep, अंक), nodep);
            } अन्यथा जर (m_params && itemp) {
                // An aggregate element, e.g. a structure in an array parameter
                nodep->replaceWithKeepDType(itemp->clone_वृक्ष(असत्य));
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            }
        }
        m_selp = nullptr;
    }

    // Evaluate a slice of an unpacked array.  If constantification is
    // required (m_required=true), call replaceWithSimulation() to compute
    // the slice via simulation.  Otherwise just iterate the children.
    शून्य visit(AstSliceSel* nodep) override {
        // First constify or width any child nodes
        iterateChildren(nodep);
        जर (!m_required) परत;  // Do nothing unless we are in parameter mode
        // Fallback to simulation: this will invoke SimulateVisitor::visit(AstSliceSel*)
        replaceWithSimulation(nodep);
    }

    शून्य visit(AstStructSel* nodep) override {
        iterateChildren(nodep);

        जर (स्थिरांक AstConsStruct* स्थिरांक consp = VN_CAST(nodep->fromp(), ConsStruct)) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = consp->memberValuep(nodep->name());
            जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(valuep, Const)) {
                VL_DO_DANGLING(replace_अंक(nodep, constp->अंक()), nodep);
            } अन्यथा {
                nodep->replaceWith(valuep->clone_वृक्ष(असत्य));
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            }
        }
    }

    शून्य visit(AstCAwait* nodep) override {
        m_hasJumpDelay = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_Var_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Not linked");
        तार्किक did = असत्य;
        जर (m_doV && (!nodep->varp()->constPoolEntry() || m_selp) && nodep->varp()->valuep()
            && !m_attrp) {
            // UINFOTREE(1, valuep, "", "visitvaref");
            iterateAndNextNull(nodep->varp()->valuep());  // May change nodep->varp()->valuep()
            Ast_गाठ* स्थिरांक valuep = nodep->varp()->valuep();
            // The selected element, when under a constant select of an array
            स्थिरांक AstConsArray* स्थिरांक consp = VN_CAST(valuep, ConsArray);
            स्थिरांक Ast_गाठ* स्थिरांक selItemp
                = (m_selp && consp) ? consp->valuep(m_selp->bitConst()) : nullptr;
            जर (nodep->access().isReadOnly() && valuep
                && ((!m_params  // Can reduce constant wires into equations
                     && m_doNConst && v3Global.opt.fConst() && nodep->varp()->isConst())
                    || nodep->varp()->isParam())) {
                जर (operandConst(valuep)) {
                    स्थिरांक V3_संख्या& अंक = VN_AS(valuep, Const)->अंक();
                    // UINFO(2, "constVisit " << cvtToHex(valuep) << " " << num);
                    स्थिरांक string origParamName
                        = nodep->varp()->isParam() ? nodep->varp()->name() : "";
                    VL_DO_DANGLING(replace_अंक(nodep, अंक, origParamName), nodep);
                    did = सत्य;
                } अन्यथा जर (स्थिरांक AstConst* स्थिरांक itemp = VN_CAST(selItemp, Const)) {
                    VL_DO_DANGLING(replace_अंक(nodep, itemp->अंक()), nodep);
                    did = सत्य;
                } अन्यथा जर (m_params && consp) {
                    // Allow parameters to pass arrays
                    // Earlier recursion of ConsArray made sure each array value is constant
                    // This exception is fairly fragile, i.e. doesn't
                    // support arrays of arrays or other stuff
                    Ast_गाठ* स्थिरांक newp = valuep->clone_वृक्ष(असत्य);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    did = सत्य;
                } अन्यथा जर (m_params && VN_IS(valuep, ConsStruct)) {
                    Ast_गाठ* स्थिरांक newp = valuep->clone_वृक्ष(असत्य);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    did = सत्य;
                } अन्यथा जर (nodep->varp()->isParam() && VN_IS(valuep, Unbounded)) {
                    Ast_गाठ* स्थिरांक newp = valuep->clone_वृक्ष(असत्य);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    did = सत्य;
                }
            }
        }
        जर (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but variable isn't const: "
                           << nodep->varp()->prettyNameQ());
        }
    }
    शून्य visit(Ast_अभिव्यक्ती_Stmt* nodep) override {
        iterateChildren(nodep);
        जर (!Ast_गाठ::afterCommentp(nodep->stmtsp())) {
            deleteVarScopesUnder(nodep->stmtsp());
            UINFO(8, "ExprStmt(...) " << nodep << " " << nodep->resultp());
            nodep->replaceWith(nodep->resultp()->unlinkFrBack());
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            // Removing the ExprStmt might have made something impure above now pure
        }
    }
    शून्य visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        तार्किक did = असत्य;
        जर (nodep->itemp()->valuep()) {
            // UINFOTREE(1, nodep->itemp()->valuep(), "", "visitvaref");
            स्थिरांक AstEnumItem* स्थिरांक itemp = nodep->itemp();
            जर (m_recursing_Enum_वस्तुसंच.मोजणी(itemp)) {
                nodep->v3error("Recursive enum value: " << nodep->itemp()->prettyNameQ());
            } अन्यथा {
                m_recursing_Enum_वस्तुसंच.emplace(itemp);
                iterateAndNextNull(nodep->itemp()->valuep());
                m_recursing_Enum_वस्तुसंच.erase(itemp);
            }
            जर (AstConst* स्थिरांक valuep = VN_CAST(nodep->itemp()->valuep(), Const)) {
                स्थिरांक V3_संख्या& अंक = valuep->अंक();
                VL_DO_DANGLING(replace_अंक(nodep, अंक), nodep);
                did = सत्य;
            }
        }
        जर (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but enum value isn't const: "
                           << nodep->itemp()->prettyNameQ());
        }
    }

    तार्किक only_Sen_Item_In_Sen_वृक्ष(स्थिरांक AstSenItem* nodep) {
        // Only one if it's not in a list
        परत (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }
    शून्य visit(AstSenItem* nodep) override {
        iterateChildren(nodep);
        जर (m_doNConst
            && !v3Global.opt.timing().isSetTrue()  // If --timing, V3Sched would turn this into an
                                                   // infinite loop. See #5080
            && (VN_IS(nodep->sensp(), Const) || VN_IS(nodep->sensp(), EnumItemRef)
                || (nodep->varrefp() && nodep->varrefp()->varp()->isParam()))) {
            // Constants in sensitivity lists may be removed (we'll simplify later)
            जर (nodep->isClocked()) {  // A constant can never get a pos/negedge
                जर (only_Sen_Item_In_Sen_वृक्ष(nodep)) {
                    जर (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                        // TODO: This really is dodgy, as strictly compliant simulators will not
                        //       execute this block, but but t_func_check relies on it
                        nodep->replaceWith(
                            नवीन AstSenItem{nodep->fileline(), AstSenItem::Initial{}});
                    } अन्यथा {
                        nodep->replaceWith(नवीन AstSenItem{nodep->fileline(), AstSenItem::Never{}});
                    }
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                } अन्यथा {
                    VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } अन्यथा {  // Otherwise it may compute a result that needs to settle out
                nodep->replaceWith(नवीन AstSenItem{nodep->fileline(), AstSenItem::Combo{}});
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            }
        } अन्यथा जर (m_doNConst && VN_IS(nodep->sensp(), Not)) {
            // V3Gate may propagate NOTs into clocks... Just deal with it
            Ast_गाठ* स्थिरांक sensp = nodep->sensp();
            Ast_गाठ* lastSensp = sensp;
            तार्किक invert = असत्य;
            पर्यंत (VN_IS(lastSensp, Not)) {
                lastSensp = VN_AS(lastSensp, Not)->lhsp();
                invert = !invert;
            }
            UINFO(8, "senItem(NOT...) " << nodep << " " << invert);
            जर (invert) nodep->edgeType(nodep->edgeType().invert());
            sensp->replaceWith(lastSensp->unlinkFrBack());
            VL_DO_DANGLING(ढकला_Deletep(sensp), sensp);
        }
    }

    वर्ग SenItemCmp final {
        स्थिर पूर्णांक cmp(स्थिरांक Ast_गाठ_अभिव्यक्ती* ap, स्थिरांक Ast_गाठ_अभिव्यक्ती* bp) {
            स्थिरांक VNType aType = ap->type();
            स्थिरांक VNType bType = bp->type();
            जर (aType != bType) परत static_cast<पूर्णांक>(bType) - static_cast<पूर्णांक>(aType);

            जर (स्थिरांक AstVarRef* स्थिरांक aRefp = VN_CAST(ap, VarRef)) {
                स्थिरांक AstVarRef* स्थिरांक bRefp = VN_AS(bp, VarRef);
                // Looks visually better if we keep sorted by name
                जर (aRefp->name() < bRefp->name()) परत -1;
                जर (aRefp->name() > bRefp->name()) परत 1;
                // But might be same name with different scopes
                जर (aRefp->varScopep() < bRefp->varScopep()) परत -1;
                जर (aRefp->varScopep() > bRefp->varScopep()) परत 1;
                // Or rarely, different data types
                जर (aRefp->dtypep() < bRefp->dtypep()) परत -1;
                जर (aRefp->dtypep() > bRefp->dtypep()) परत 1;
                परत 0;
            }

            जर (स्थिरांक AstConst* स्थिरांक aConstp = VN_CAST(ap, Const)) {
                स्थिरांक AstConst* स्थिरांक bConstp = VN_AS(bp, Const);
                जर (aConstp->अंक().isLtXZ(bConstp->अंक())) परत -1;
                जर (bConstp->अंक().isLtXZ(aConstp->अंक())) परत 1;
                परत 0;
            }

            जर (स्थिरांक Ast_गाठ_Biop* स्थिरांक aBiOpp = VN_CAST(ap, गाठ_Biop)) {
                स्थिरांक Ast_गाठ_Biop* स्थिरांक bBiOpp = VN_AS(bp, गाठ_Biop);
                // Compare RHSs first, as LHS might be const
                जर (स्थिरांक पूर्णांक c = cmp(aBiOpp->rhsp(), bBiOpp->rhsp())) परत c;
                परत cmp(aBiOpp->lhsp(), bBiOpp->lhsp());
            }

            जर (स्थिरांक AstCMethodHard* स्थिरांक aCallp = VN_CAST(ap, CMethodHard)) {
                स्थिरांक AstCMethodHard* स्थिरांक bCallp = VN_AS(bp, CMethodHard);
                जर (aCallp->method() < bCallp->method()) परत -1;
                जर (aCallp->method() > bCallp->method()) परत 1;
                जर (स्थिरांक पूर्णांक c = cmp(aCallp->fromp(), bCallp->fromp())) परत c;
                स्थिरांक Ast_गाठ_अभिव्यक्ती* aPinsp = aCallp->pinsp();
                स्थिरांक Ast_गाठ_अभिव्यक्ती* bPinsp = bCallp->pinsp();
                पर्यंत (aPinsp && bPinsp) {
                    जर (स्थिरांक पूर्णांक c = cmp(aPinsp, bPinsp)) परत c;
                    aPinsp = VN_AS(aPinsp->nextp(), गाठ_अभिव्यक्ती);
                    bPinsp = VN_AS(bPinsp->nextp(), गाठ_अभिव्यक्ती);
                }
                परत aPinsp ? -1 : bPinsp ? 1 : 0;
            }

            परत 0;
        }

    सार्वजनिक:
        तार्किक चालक()(स्थिरांक AstSenItem* lhsp, स्थिरांक AstSenItem* rhsp) स्थिरांक {
            स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक lSensp = lhsp->sensp();
            स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक rSensp = rhsp->sensp();
            जर (lSensp && rSensp) {
                // If both terms have sensitivity expressions, recursively compare them
                जर (स्थिरांक पूर्णांक c = cmp(lSensp, rSensp)) परत c < 0;
            } अन्यथा जर (lSensp || rSensp) {
                // Terms with sensitivity expressions come after those without
                परत rSensp;
            }
            // Finally sort by edge, AFTER variable, as we want multiple edges for same var
            // adjacent. note the SenTree optimizer requires this order (more general first,
            // less general last)
            परत lhsp->edgeType() < rhsp->edgeType();
        }
    };

    शून्य visit(Ast_Sen_वृक्ष* nodep) override {
        iterateChildren(nodep);
        जर (m_doExpensive) {
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
            साठी (AstSenItem *nextp, *senp = nodep->sensesp(); senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                // cppcheck-suppress unassignedVariable  // cppcheck bug
                स्थिरांक SenItemCmp cmp;
                जर (nextp && !cmp(senp, nextp)) {
                    // Something's out of order, sort it
                    senp = nullptr;
                    std::vector<AstSenItem*> vec;
                    साठी (AstSenItem* senp = nodep->sensesp(); senp;
                         senp = VN_AS(senp->nextp(), SenItem)) {
                        vec.ढकला_back(senp);
                    }
                    stable_sort(vec.begin(), vec.end(), SenItemCmp());
                    साठी (स्थिरांक स्वतः& ip : vec) ip->unlinkFrBack();
                    साठी (स्थिरांक स्वतः& ip : vec) nodep->addSensesp(ip);
                    थांबा;
                }
            }

            // Pass 2, remove duplicates and simplify adjacent terms if possible
            साठी (AstSenItem *senp = nodep->sensesp(), *nextp; senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                जर (!nextp) थांबा;
                AstSenItem* स्थिरांक lItemp = senp;
                AstSenItem* स्थिरांक rItemp = nextp;
                स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक lSenp = lItemp->sensp();
                स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक rSenp = rItemp->sensp();
                जर (!lSenp || !rSenp) सुरूठेवा;

                जर (lSenp->same_Gate_वृक्ष(rSenp)) {
                    // POSEDGE or NEGEDGE -> BOTHEDGE. (We've sorted POSEDGE, before NEGEDGE, so we
                    // do not need to test for the opposite orders.)
                    जर (lItemp->edgeType() == VEdgeType::ET_POSEDGE
                        && rItemp->edgeType() == VEdgeType::ET_NEGEDGE) {
                        // Make both terms BOTHEDGE, the second will be removed below
                        lItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                        rItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                    }

                    // Remove identical expressions
                    जर (lItemp->edgeType() == rItemp->edgeType()) {
                        VL_DO_DANGLING(ढकला_Deletep(rItemp->unlinkFrBack()), rItemp);
                        nextp = lItemp;
                    }
                }
            }
        }
    }

    //-----
    // Zero elimination
    शून्य visit(Ast_गाठ_Assign* nodep) override {
        iterateChildren(nodep);
        जर (nodep->timingControlp()) m_hasJumpDelay = सत्य;
        जर (m_doNConst && replace_गाठ_Assign(nodep)) परत;
    }
    शून्य visit(AstAlias* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    शून्य visit(AstAliasScope* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    शून्य visit(AstAssignW* nodep) override {
        iterateChildren(nodep);
        जर (m_doNConst && replace_गाठ_Assign(nodep)) परत;
        // Process containing this AssignW as single body statement
        AstAlways* स्थिरांक procp = VN_CAST(nodep->backp(), Always);
        जर (!procp || procp->stmtsp() != nodep || nodep->nextp()) परत;
        // Not VarXRef, as different refs may set different values to each hierarchy
        Ast_गाठ_Var_Ref* स्थिरांक varrefp = VN_CAST(nodep->lhsp(), VarRef);
        जर (m_wremove && !m_params && m_doNConst && m_modp && operandConst(nodep->rhsp())
            && !VN_AS(nodep->rhsp(), Const)->अंक().isFourState()
            && varrefp  // Don't do messes with BITREFs/ARRAYREFs
            && !varrefp->varp()->hasStrengthAssignment()  // Strengths are resolved in V3Tristate
            && !varrefp->varp()->valuep()  // Not already constified
            && !varrefp->varScopep()  // Not scoped (or each scope may have different initial val.)
            && !varrefp->varp()->isForced()  // Not forced (not really a constant)
        ) {
            // ASSIGNW (VARREF, const) -> INITIAL ( ASSIGN (VARREF, const) )
            UINFO(4, "constAssignW " << nodep);
            // Make a initial assignment
            Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = nodep->rhsp()->unlinkFrBack();
            varrefp->unlinkFrBack();
            File_ओळ* स्थिरांक flp = nodep->fileline();
            AstInitial* स्थिरांक newinitp = नवीन AstInitial{flp, नवीन AstAssign{flp, varrefp, exprp}};
            procp->replaceWith(newinitp);
            VL_DO_DANGLING(ढकला_Deletep(procp), procp);
            // Set the initial value right in the variable so we can constant propagate
            Ast_गाठ* स्थिरांक initvaluep = exprp->clone_वृक्ष(असत्य);
            varrefp->varp()->isConst(सत्य);
            varrefp->varp()->valuep(initvaluep);
        }
    }
    शून्य visit(AstCReset* nodep) override {
        iterateChildren(nodep);
        जर (!m_doNConst) परत;
        स्थिरांक AstBasicDType* स्थिरांक bdtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType);
        जर (!bdtypep) परत;
        जर (!bdtypep->isZeroInit()) परत;
        AstConst* स्थिरांक newp = नवीन AstConst{nodep->fileline(), V3_संख्या{nodep, bdtypep}};
        UINFO(9, "CRESET(0) => CONST(0) " << nodep);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstCvtUnpackToUnpack* nodep) override {
        iterateChildren(nodep);
        AstConsArray* स्थिरांक fromp = VN_CAST(nodep->fromp(), ConsArray);
        जर (!fromp) परत;
        // Fold the outermost dimension, converting the elements for any dimension under it
        स्थिरांक AstUnpackArrayDType* स्थिरांक toArrayp
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        स्थिरांक AstUnpackArrayDType* स्थिरांक fromArrayp
            = VN_AS(fromp->dtypep()->skipRefp(), UnpackArrayDType);
        // This dimension may not be reversed, only one under it
        स्थिरांक तार्किक reverse
            = toArrayp->declRange().ascending() != fromArrayp->declRange().ascending();
        स्थिरांक uint64_t lastIndex = toArrayp->elementsConst() - 1;
        Ast_गाठ_DType* स्थिरांक subDTypep = toArrayp->subDTypep();
        स्थिरांक AstUnpackArrayDType* स्थिरांक subArrayp
            = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
        स्थिरांक स्वतः convert = [&](Ast_गाठ_अभिव्यक्ती* valuep) -> Ast_गाठ_अभिव्यक्ती* {
            valuep->unlinkFrBack();
            जर (!subArrayp || !subArrayp->isReverseOf(valuep->dtypep())) परत valuep;
            परत नवीन AstCvtUnpackToUnpack{valuep->fileline(), valuep, subDTypep};
        };
        AstConsArray* स्थिरांक newp
            = नवीन AstConsArray{fromp->fileline(), nodep->dtypep(),
                               fromp->defaultp() ? convert(fromp->defaultp()) : nullptr};
        साठी (स्थिरांक AstConsArrayItem* itemp = fromp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            newp->valuep(reverse ? lastIndex - itemp->index() : itemp->index(),
                         convert(itemp->valuep()));
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        iterate(newp);  // Fold the conversions of the elements
    }
    शून्य visit(AstCvtArrayToArray* nodep) override {
        iterateChildren(nodep);
        // Handle the case where we have a stream operation inside a cast conversion
        // Check for both StreamL and StreamR operations
        Ast_गाठ_Stream* streamp = nullptr;
        तार्किक isReverse = असत्य;
        जर (AstStreamL* स्थिरांक streamLp = VN_CAST(nodep->fromp(), StreamL)) {
            streamp = streamLp;
            isReverse = सत्य;  // StreamL reverses the operation
        } अन्यथा जर (AstStreamR* स्थिरांक streamRp = VN_CAST(nodep->fromp(), StreamR)) {
            streamp = streamRp;
            isReverse = असत्य;  // StreamR doesn't reverse the operation
        }
        जर (streamp) {
            Ast_गाठ_अभिव्यक्ती* srcp = streamp->lhsp();
            स्थिरांक Ast_गाठ_DType* स्थिरांक srcDTypep = srcp->dtypep()->skipRefp();
            Ast_गाठ_DType* स्थिरांक dstDTypep = nodep->dtypep()->skipRefp();
            जर (VN_IS(srcDTypep, QueueDType) && VN_IS(dstDTypep, QueueDType)) {
                पूर्णांक blockSize = 1;
                जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(streamp->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    जर (VL_UNLIKELY(blockSize <= 0)) {
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
                पूर्णांक srcElementBits = 0;
                जर (स्थिरांक Ast_गाठ_DType* स्थिरांक elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                पूर्णांक dstElementBits = 0;
                जर (स्थिरांक Ast_गाठ_DType* स्थिरांक elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                streamp->unlinkFrBack();
                Ast_गाठ_अभिव्यक्ती* newp = नवीन AstCvtArrayToArray{
                    srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     isReverse,
                    blockSize,        dstElementBits,       srcElementBits};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(streamp), streamp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                परत;
            }
        }
    }
    शून्य visit(AstRelease* nodep) override {
        // cppcheck-suppress constVariablePointer // children unlinked below
        जर (AstConcat* स्थिरांक concatp = VN_CAST(nodep->lhsp(), Concat)) {
            File_ओळ* स्थिरांक flp = nodep->fileline();
            AstRelease* स्थिरांक newLp = नवीन AstRelease{flp, concatp->lhsp()->unlinkFrBack()};
            AstRelease* स्थिरांक newRp = नवीन AstRelease{flp, concatp->rhsp()->unlinkFrBack()};
            nodep->replaceWith(newLp);
            newLp->addNextHere(newRp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            visit(newLp);
            visit(newRp);
        }
    }

    शून्य visit(Ast_गाठ_If* nodep) override {
        iterateChildren(nodep);
        जर (m_doNConst) {
            जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->condp(), Const)) {
                Ast_गाठ* keepp = nullptr;
                Ast_गाठ* delp = nullptr;
                जर (constp->isZero()) {
                    UINFO(4, "IF(0,{any},{x}) => {x}: " << nodep);
                    keepp = nodep->elsesp();
                    delp = nodep->thensp();
                } अन्यथा जर (!m_doV || constp->isNeqZero()) {  // Might be X in Verilog
                    UINFO(4, "IF(!0,{x},{any}) => {x}: " << nodep);
                    keepp = nodep->thensp();
                    delp = nodep->elsesp();
                } अन्यथा {
                    UINFO(4, "IF condition is X, retaining: " << nodep);
                    परत;
                }
                जर (!canDeleteStatements()) {
                    UINFO(4, "IF retained until statics lifted: " << nodep);
                    परत;
                }

                // If we delete a branch that contains variable declarations, also delete the
                // corresponding varscopes so we don't leave dangling AstVarScope::m_varp pointers.
                deleteVarScopesUnder(delp);

                जर (keepp) {
                    keepp->unlinkFrBackWithNext();
                    nodep->replaceWith(keepp);
                } अन्यथा {
                    nodep->unlinkFrBack();
                    deleteVarScopesUnder(nodep->thensp());
                    deleteVarScopesUnder(nodep->elsesp());
                }
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);

                // Removed branch could contain only impurity within a function and the
                // function could be purified
                VIsCached::clear_Cache_वृक्ष();
            } अन्यथा जर (!Ast_गाठ::afterCommentp(nodep->thensp())
                       && !Ast_गाठ::afterCommentp(nodep->elsesp())) {
                जर (!nodep->condp()->isPure()) {
                    // Condition has side effect - leave, but don't need return value
                    UINFO(4, "IF({x}) ; => STMTEXPR({x}): " << nodep);
                    nodep->fileline()->warnOff(V3ErrorCode::IGNOREDRETURN, सत्य);
                    Ast_गाठ_Stmt* स्थिरांक newp
                        = नवीन Ast_Stmt_अभिव्यक्ती{nodep->fileline(), nodep->condp()->unlinkFrBack()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                } अन्यथा {
                    // Empty block, remove it
                    UINFO(4, "IF({x}) ; => ; : " << nodep);
                    VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } अन्यथा जर (!Ast_गाठ::afterCommentp(nodep->thensp())) {
                UINFO(4, "IF({x}) nullptr {...} => IF(NOT{x}}: " << nodep);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = nodep->condp();
                Ast_गाठ* स्थिरांक elsesp = nodep->elsesp();
                condp->unlinkFrBackWithNext();
                elsesp->unlinkFrBackWithNext();
                जर (nodep->thensp()) {  // Must have been comment
                    ढकला_Deletep(nodep->thensp()->unlinkFrBackWithNext());
                }
                nodep->condp(नवीन AstLogNot{condp->fileline(),
                                           condp});  // LogNot, as C++ optimization also possible
                nodep->addThensp(elsesp);
                nodep->branchPred(nodep->branchPred().invert());
            } अन्यथा जर (((VN_IS(nodep->condp(), Not) && nodep->condp()->width() == 1)
                        || VN_IS(nodep->condp(), LogNot))
                       && nodep->thensp() && nodep->elsesp()) {
                UINFO(4, "IF(NOT {x})  => IF(x) swapped if/else" << nodep);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक condp
                    = VN_AS(nodep->condp(), गाठ_Uniop)->lhsp()->unlinkFrBackWithNext();
                Ast_गाठ* स्थिरांक thensp = nodep->thensp()->unlinkFrBackWithNext();
                Ast_गाठ* स्थिरांक elsesp = nodep->elsesp()->unlinkFrBackWithNext();
                AstIf* स्थिरांक ifp = नवीन AstIf{nodep->fileline(), condp, elsesp, thensp};
                ifp->isBoundsCheck(nodep->isBoundsCheck());  // Copy bounds check info
                ifp->branchPred(nodep->branchPred().invert());
                nodep->replaceWith(ifp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            } अन्यथा जर (ifSameAssign(nodep)) {
                UINFO(4,
                      "IF({a}) ASSIGN({b},{c}) else ASSIGN({b},{d}) => ASSIGN({b}, {a}?{c}:{d})");
                Ast_गाठ_Assign* स्थिरांक thensp = VN_AS(nodep->thensp(), गाठ_Assign);
                // cppcheck-suppress constVariablePointer // children unlinked below
                Ast_गाठ_Assign* स्थिरांक elsesp = VN_AS(nodep->elsesp(), गाठ_Assign);
                thensp->unlinkFrBack();
                Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = nodep->condp()->unlinkFrBack();
                Ast_गाठ_अभिव्यक्ती* स्थिरांक truep = thensp->rhsp()->unlinkFrBack();
                Ast_गाठ_अभिव्यक्ती* स्थिरांक falsep = elsesp->rhsp()->unlinkFrBack();
                thensp->rhsp(नवीन AstCond{truep->fileline(), condp, truep, falsep});
                nodep->replaceWith(thensp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            } अन्यथा जर (असत्य  // Disabled, as vpm assertions are faster
                              // without due to short-circuiting
                       && operandIfIf(nodep)) {
                UINFO(9, "IF({a}) IF({b}) => IF({a} && {b})");
                Ast_गाठ_If* स्थिरांक lowerIfp = VN_AS(nodep->thensp(), गाठ_If);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = nodep->condp()->unlinkFrBack();
                Ast_गाठ* स्थिरांक lowerThensp = lowerIfp->thensp()->unlinkFrBackWithNext();
                Ast_गाठ_अभिव्यक्ती* स्थिरांक lowerCondp = lowerIfp->condp()->unlinkFrBackWithNext();
                nodep->condp(नवीन AstLogAnd{lowerIfp->fileline(), condp, lowerCondp});
                lowerIfp->replaceWith(lowerThensp);
                VL_DO_DANGLING(ढकला_Deletep(lowerIfp), lowerIfp);
            } अन्यथा {
                // Optimizations that don't reform the IF itself
                जर (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
                matchIfSameLast(nodep);
                matchIfCondCond(nodep);
            }
        }
    }

    शून्य visit(AstDisplay* nodep) override {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        iterateChildren(nodep);
        जर (stmtDisplayDisplay(nodep)) परत;
    }
    तार्किक stmtDisplayDisplay(AstDisplay* nodep) {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        जर (!m_modp) परत असत्य;  // Don't optimize under single statement
        AstDisplay* स्थिरांक prevp = VN_CAST(nodep->backp(), Display);
        जर (!prevp) परत असत्य;
        जर (!((prevp->displayType() == nodep->displayType())
              || (prevp->displayType() == VDisplayType::DT_WRITE
                  && nodep->displayType() == VDisplayType::DT_DISPLAY)
              || (prevp->displayType() == VDisplayType::DT_DISPLAY
                  && nodep->displayType() == VDisplayType::DT_WRITE)))
            परत असत्य;
        जर ((prevp->filep() && !nodep->filep()) || (!prevp->filep() && nodep->filep())
            || (prevp->filep() && nodep->filep() && !prevp->filep()->same_वृक्ष(nodep->filep())))
            परत असत्य;
        जर (!prevp->fmtp() || prevp->fmtp()->nextp() || !nodep->fmtp() || nodep->fmtp()->nextp())
            परत असत्य;
        AstSFormatF* स्थिरांक pformatp = prevp->fmtp();
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSFormatF* स्थिरांक nformatp = nodep->fmtp();
        // We don't merge scopeNames as can have only one and might be different scopes (late in
        // process) Also rare for real code to print %m multiple times in same message
        जर (nformatp->scopeNamep() && pformatp->scopeNamep()) परत असत्य;
        // We don't early merge arguments as might need to later print warnings with
        // right line numbers, nor scopeNames as might be different scopes (late in process)
        जर (!m_doCpp && pformatp->exprsp()) परत असत्य;
        जर (!m_doCpp && nformatp->exprsp()) परत असत्य;
        जर (pformatp->exprsp() && !pformatp->exprsp()->isPureAndNext()) परत असत्य;
        जर (nformatp->exprsp() && !nformatp->exprsp()->isPureAndNext()) परत असत्य;
        // Avoid huge merges
        स्थिर constexpr पूर्णांक DISPLAY_MAX_MERGE_LENGTH = 500;
        जर (pformatp->text().length() + nformatp->text().length() > DISPLAY_MAX_MERGE_LENGTH)
            परत असत्य;
        //
        UINFO(9, "DISPLAY(SF({a})) DISPLAY(SF({b})) -> DISPLAY(SF({a}+{b}))");
        // Convert DT_DISPLAY to DT_WRITE as may allow later optimizations
        जर (prevp->displayType() == VDisplayType::DT_DISPLAY) {
            prevp->displayType(VDisplayType::DT_WRITE);
            pformatp->text(pformatp->text() + "\n");
        }
        // We can't replace prev() as the edit tracking iterators will get confused.
        // So instead we edit the prev note itself.
        जर (prevp->addNewline()) pformatp->text(pformatp->text() + "\n");
        pformatp->text(pformatp->text() + nformatp->text());
        जर (!prevp->addNewline() && nodep->addNewline()) pformatp->text(pformatp->text() + "\n");
        जर (nformatp->exprsp()) pformatp->addExprsp(nformatp->exprsp()->unlinkFrBackWithNext());
        जर (AstScopeName* स्थिरांक scopeNamep = nformatp->scopeNamep()) {
            scopeNamep->unlinkFrBackWithNext();
            pformatp->scopeNamep(scopeNamep);
        }
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
        परत सत्य;
    }
    शून्य visit(AstSFormatArg* nodep) override {
        // Skip namep(): requiring its runtime lookup to be constant can reject valid
        // enum-valued constant-function calls in parameters. displayedEnum() resolves
        // the name from the folded exprp() value and enum dtype instead.
        iterateAndNextNull(nodep->exprp());
    }
    शून्य visit(AstSFormatF* nodep) override {
        // Substitute constants into displays.  The main point of this is to
        // simplify assertion methodologies which call functions with display's.
        // This eliminates a pile of wide temps, and makes the C a whole lot more readable.
        iterateChildren(nodep);
        जर (nodep->अभिव्यक्ती_Format_2()) {
            // Upconvert to text-based format?
            // Similar code in V3LinkResolve::visit(AstSFormatF)
            UASSERT_OBJ(nodep->text() == "", nodep,
                        "Non-format $sformatf should have text format == \"\"");
            जर (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->अंक().isFromString()) {
                AstConst* स्थिरांक fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->अंक().toString());
                nodep->अभिव्यक्ती_Format_2(असत्य);
                VL_DO_DANGLING(ढकला_Deletep(fmtp), fmtp);
            }
        }
        जर (nodep->अभिव्यक्ती_Format_2()) परत;  // Runtime, unfortunately  (unless constify format later)
        तार्किक anyconst = असत्य;
        साठी (Ast_गाठ* argp = nodep->exprsp(); argp; argp = argp->nextp()) {
            AstSFormatArg* स्थिरांक fargp = VN_CAST(argp, SFormatArg);
            Ast_गाठ* स्थिरांक subargp = fargp ? fargp->exprp() : argp;
            जर (VN_IS(subargp, Const)) {
                anyconst = सत्य;
                थांबा;
            }
        }
        जर (m_doNConst && anyconst) {
            UINFO(9, "  Display in  " << nodep->text());
            string newFormat;
            string fmt;
            तार्किक inPct = असत्य;
            Ast_गाठ* argp = nodep->exprsp();
            स्थिरांक string text = nodep->text();
            साठी (अक्षर ch : text) {
                जर (!inPct && ch == '%') {
                    inPct = सत्य;
                    fmt = ch;
                } अन्यथा जर (inPct && (std::है_अंक(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } अन्यथा जर (inPct) {
                    inPct = असत्य;
                    fmt += ch;
                    ch = std::छोटे(ch);
                    निवड (ch) {
                    स्थिती '%': थांबा;  // %% - still %%
                    स्थिती 'm': थांबा;  // %m - still %m - auto insert "name"
                    स्थिती 'l': थांबा;  // %l - still %l - auto insert "library"
                    स्थिती 't':  // %t - don't know $timeformat so can't constify
                        जर (argp) argp = argp->nextp();
                        थांबा;
                    पूर्वनिर्धारित:  // Most operators, just move to next argument
                        जर (argp) {
                            Ast_गाठ* स्थिरांक nextp = argp->nextp();
                            AstSFormatArg* स्थिरांक fargp = VN_CAST(argp, SFormatArg);
                            Ast_गाठ* स्थिरांक subargp = fargp ? fargp->exprp() : argp;
                            स्थिरांक VFormatAttr formatAttr
                                = argp
                                      ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                                      : VFormatAttr{};
                            जर (VN_IS(subargp, Const)) {  // Convert it
                                स्थिरांक string out
                                    = formatAttr.isEnum()
                                          ? const_अंक_V(subargp).displayedEnum(fargp, fmt)
                                          : const_अंक_V(subargp).displayed(nodep, fmt, formatAttr);
                                UINFO(9, "     DispConst: " << fmt << " -> " << out << "  for "
                                                            << subargp);
                                // fmt = out w/ replace % with %% as it must later when
                                // used as a format output as literal.
                                fmt = VString::quotePercent(out);
                                VL_DO_DANGLING(ढकला_Deletep(argp->unlinkFrBack()), argp);
                            }
                            argp = nextp;
                        }
                        थांबा;
                    }  // switch
                    newFormat += fmt;
                } अन्यथा {
                    newFormat += ch;
                }
            }
            जर (newFormat != nodep->text()) {
                nodep->text(newFormat);
                UINFO(9, "  Display out " << nodep);
            }
        }
        जर (!nodep->exprsp() && nodep->text().find('%') == string::npos && !nodep->hidden()) {
            // Just a simple constant string - the formatting is pointless
            UINFO(9, "replaceConstStr");
            VL_DO_DANGLING(replaceConstString(nodep, nodep->text()), nodep);
            परत;
        }
        जर (VN_IS(nodep->backp(), गाठ_Assign)  // Can't remove under Display etc
            && nodep->text() == "%s" && VN_IS(nodep->exprsp(), SFormatArg)
            && VN_AS(nodep->exprsp(), SFormatArg)->formatAttr().isString()
            && !nodep->exprsp()->nextp()) {
            UINFO(9, "  Display(%p, expr) -> constant expr " << nodep);
            nodep->replaceWith(VN_AS(nodep->exprsp(), SFormatArg)->exprp()->unlinkFrBack());
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }
    }
    शून्य visit(AstSFormat* nodep) override {
        iterateChildren(nodep);
        जर (!m_doNConst) परत;
        जर (m_doNConst) {
            // If it's a constant string written to a string variable, inline it as an assignment
            AstSFormatF* स्थिरांक fmtp = nodep->fmtp();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp();
            AstBasicDType* स्थिरांक basicp = VN_CAST(lhsp->dtypep()->skipRefp(), BasicDType);
            जर (basicp && basicp->isString() && !fmtp->exprsp()
                && fmtp->text().find('%') == string::npos) {
                AstConst* स्थिरांक strp
                    = नवीन AstConst{fmtp->fileline(), AstConst::String{}, fmtp->text()};
                lhsp->unlinkFrBack();
                nodep->replaceWith(नवीन AstAssign{nodep->fileline(), lhsp, strp});
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                परत;
            }
        }
    }
    शून्य visit(Ast_गाठ_FTask* nodep) override {
        VL_RESTORER(m_underRecFunc);
        जर (nodep->recursive()) m_underRecFunc = सत्य;
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाठ_CCall* nodep) override {
        iterateChildren(nodep);
        m_hasJumpDelay = सत्य;  // As don't analyze inside tasks for timing controls
    }
    शून्य visit(Ast_गाठ_FTask_Ref* nodep) override {
        // Note excludes AstFuncRef as other visitor below
        iterateChildren(nodep);
        m_hasJumpDelay = सत्य;  // As don't analyze inside tasks for timing controls
    }
    शून्य visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_गाठ_FTask_Ref*>(nodep));
        जर (m_params) {  // Only parameters force us to do constant function call propagation
            replaceWithSimulation(nodep);
        }
    }
    शून्य visit(AstArg* nodep) override {
        // replaceWithSimulation on the Arg's parent FuncRef replaces these
        iterateChildren(nodep);
    }
    शून्य visit(AstLoop* nodep) override {
        VL_RESTORER(m_hasLoopTest);
        स्थिरांक तार्किक oldHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = असत्य;
        m_hasLoopTest = असत्य;
        iterateChildren(nodep);
        स्थिरांक तार्किक thisLoopHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = thisLoopHasJumpDelay || oldHasJumpDelay;
        // If the first statement always break, the loop is useless
        जर (स्थिरांक AstLoopTest* स्थिरांक testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            जर (testp->condp()->isZero() && (!testp->nextp() || canDeleteStatements())) {
                nodep->v3warn(UNUSEDLOOP, "Loop condition is always false");
                VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                परत;
            }
        }
        // If last statement always breaks, repalce loop with body
        जर (Ast_गाठ* lastp = nodep->stmtsp()) {
            पर्यंत (Ast_गाठ* स्थिरांक nextp = lastp->nextp()) lastp = nextp;
            जर (AstLoopTest* स्थिरांक testp = VN_CAST(lastp, LoopTest)) {
                जर (testp->condp()->isZero()) {
                    VL_DO_DANGLING2(ढकला_Deletep(testp->unlinkFrBack()), testp, lastp);
                    nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत;
                }
            }
        }
        // Warn on infinite loop
        जर (!m_hasLoopTest && !thisLoopHasJumpDelay) {
            nodep->v3warn(INFINITELOOP, "Infinite loop (condition always true)");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::INFINITELOOP, सत्य);  // Complain once
        }
    }
    शून्य visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);

        // If never breaks, remove
        जर (nodep->condp()->isNeqZero()) {
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
            परत;
        }

        m_hasLoopTest = सत्य;

        // If always breaks, subsequent statements are dead code, delete them
        जर (nodep->condp()->isZero()) {
            Ast_गाठ* स्थिरांक nextp = nodep->nextp();
            जर (nextp && canDeleteStatements()) {
                VL_DO_DANGLING(ढकला_Deletep(nextp->unlinkFrBackWithNext()), nodep);
            }
            परत;
        }

        जर (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
    }

    शून्य visit(AstConsArray* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstConsStruct* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstConsStructItem* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstConsArrayItem* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstUnbounded* nodep) override { iterateChildren(nodep); }
    // These are converted by V3Param.  Don't constify as we don't want the
    // from() VARREF to disappear, if any.
    // If output of a presel didn't get consted, chances are V3Param didn't visit properly
    शून्य visit(Ast_गाठ_Pre_Sel*) override {}

    // Ignored, can eliminate early
    शून्य visit(AstSysIgnore* nodep) override {
        iterateChildren(nodep);
        जर (m_doNConst) VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }

    शून्य visit(Ast_Stmt_अभिव्यक्ती* nodep) override {
        iterateChildren(nodep);
        // Malformed due to child being deleted, remove here
        जर (!nodep->exprp()) {
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
            परत;
        }
        // Remove if expression is trivially pure
        जर (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), VarRef)) {
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
            परत;
        }
        // Remove calls to empty functions
        जर (स्थिरांक AstCCall* स्थिरांक callp = VN_CAST(nodep->exprp(), CCall)) {
            स्थिरांक AstCFunc* स्थिरांक funcp = callp->funcp();
            जर (!callp->argsp() && funcp->emptyBody()) {
                VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                परत;
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
    शून्य visit(AstBasicDType* nodep) override {
        iterateChildren(nodep);
        nodep->cvtRangeConst();
    }

    //-----
    // Jump elimination

    शून्य visit(AstJumpGo* nodep) override {
        // Any statements following the JumpGo (at this statement level) never execute, delete
        जर (nodep->nextp() && canDeleteStatements()) {
            ढकला_Deletep(nodep->nextp()->unlinkFrBackWithNext());
        }

        // JumpGo as last statement in target JumpBlock (including last in a last sub-list),
        // is a no-op, remove it.
        जर (!nodep->nextp()) {
            साठी (Ast_गाठ* abovep = nodep->abovep(); abovep; abovep = abovep->abovep()) {
                जर (abovep == nodep->blockp()) {
                    VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                    परत;
                }
                // Stop if not doing expensive, or if the above node is not the last in its list,
                // ... or if it's not an 'if' TODO: it would be enough if it was not a branch.
                जर (!m_doExpensive || abovep->nextp() || !VN_IS(abovep, If)) थांबा;
            }
        }
        // Mark JumpBlock as used
        m_usedJumpBlocks.emplace(nodep->blockp());
        m_hasJumpDelay = सत्य;
    }

    शून्य visit(AstJumpBlock* nodep) override {
        iterateChildren(nodep);

        // If first statement is an AstLoopTest, pull it before the jump block
        जर (AstLoopTest* स्थिरांक testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            nodep->addHereThisAsNext(testp->unlinkFrBack());
        }

        // Remove if empty
        जर (!nodep->stmtsp()) {
            UINFO(4, "JUMPLABEL => empty " << nodep);
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
            परत;
        }

        // If no JumpGo points to this node, replace it with its body
        जर (!m_usedJumpBlocks.मोजणी(nodep)) {
            UINFO(4, "JUMPLABEL => unused " << nodep);
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }

    //-----
    // Below lines are magic expressions processed by astgen
    //  TREE_SKIP_VISIT("AstNODETYPE")    # Rename normal visit to visitGen and don't iterate
    //-----
    // clang-format off
    वृक्ष_SKIP_VISIT("ArraySel");
    वृक्ष_SKIP_VISIT("CAwait");

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
    शून्य visit(Ast_गाठ* nodep) override {
        // Default: Just iterate
        जर (m_required) {
            जर (VN_IS(nodep, गाठ_DType) || VN_IS(nodep, Range) || VN_IS(nodep, SliceSel)
                || VN_IS(nodep, Dot) || VN_IS(nodep, Text)) {
                // ignore
            } अन्यथा जर (VN_IS(nodep, Pattern) || VN_IS(nodep, PatMember)) {
                // A parameter override pattern is typed later, so only fold its members
                iterateChildren(nodep);
            } अन्यथा जर (AstCellRef* स्थिरांक crp = VN_CAST(nodep, CellRef)) {
                iterate(crp->exprp());
                जर (Ast_गाठ* स्थिरांक newp = crp->exprp()) {
                    crp->replaceWithKeepDType(newp->unlinkFrBack());
                    VL_DO_DANGLING(ढकला_Deletep(crp), crp);
                }
                परत;
            } अन्यथा {
                nodep->v3error("Expecting expression to be constant, but can't convert a "
                               << nodep->prettyTypeName() << " to constant.");
            }
        } अन्यथा {
            जर (nodep->isTimingControl()) m_hasJumpDelay = सत्य;
            // Calculate the width of this operation
            जर (m_params && !nodep->width()) nodep = V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);
        }
    }

सार्वजनिक:
    // Processing Mode Enum
    गणना ProcMode : uint8_t {
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
    ConstVisitor(ProcMode pmode, तार्किक globalPass)
        : m_globalPass{globalPass}
        , m_concswapNames{globalPass ? ("__Vconcswap_" + cvtToStr(s_global_Pass_अंक++)) : ""} {
        // clang-format off
        निवड (pmode) {
        स्थिती PROC_PARAMS_NOWARN:  m_doV = सत्य;  m_doNConst = सत्य; m_params = सत्य;
                                  m_required = असत्य; थांबा;
        स्थिती PROC_PARAMS:         m_doV = सत्य;  m_doNConst = सत्य; m_params = सत्य;
                                  m_required = सत्य; थांबा;
        स्थिती PROC_GENERATE:       m_doV = सत्य;  m_doNConst = सत्य; m_params = सत्य;
                                  m_required = सत्य; m_doGenerate = सत्य; थांबा;
        स्थिती PROC_LIVE:           थांबा;
        स्थिती PROC_V_WARN:         m_doV = सत्य;  m_doNConst = सत्य; m_warn = सत्य; m_convertLogicToBit = सत्य; थांबा;
        स्थिती PROC_V_NOWARN:       m_doV = सत्य;  m_doNConst = सत्य; थांबा;
        स्थिती PROC_V_EXPENSIVE:    m_doV = सत्य;  m_doNConst = सत्य; m_doExpensive = सत्य; थांबा;
        स्थिती PROC_CPP:            m_doV = असत्य; m_doNConst = सत्य; m_doCpp = सत्य; थांबा;
        पूर्वनिर्धारित:                  v3fatalSrc("Bad case"); थांबा;
        }
        // clang-format on
    }
    ~ConstVisitor() override {
        जर (m_doCpp) {
            जर (m_globalPass) {
                V3Stats::addStat("Optimizations, Const bit op reduction", m_statBitOpReduction);
            } अन्यथा {
                V3Stats::addStatSum("Optimizations, Const bit op reduction", m_statBitOpReduction);
            }
        }
        V3Stats::addStatSum("Optimizations, Cond redundant expressions", m_stat_Cond_अभिव्यक्ती_Redundant);
        V3Stats::addStatSum("Optimizations, If cond redundant expressions",
                            m_stat_If_Cond_अभिव्यक्ती_Redundant);
        V3Stats::addStatSum("Optimizations, Concat merges", m_statConcatMerge);
        V3Stats::addStatSum("Optimizations, Member access predicate node visits",
                            m_statMemberAccessVisits);
    }

    Ast_गाठ* mainAcceptEdit(Ast_गाठ* nodep) {
        VIsCached::clear_Cache_वृक्ष();  // Avoid using any stale isPure
        // Operate starting at a random place
        परत iterateSubtreeReturnEdits(nodep);
    }
};

uint32_t ConstVisitor::s_global_Pass_अंक = 0;

//######################################################################
// Const class functions

//! Force this cell node's parameter list to become a constant
शून्य V3Const::constifyParamsEdit(Ast_गाठ* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS, /* globalPass: */ असत्य};
    // cppcheck-suppress constVariablePointer // edited below
    जर (AstVar* स्थिरांक varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        जर (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } अन्यथा {
        (शून्य)visitor.mainAcceptEdit(nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump if have next line
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

//! Constify this cell node's parameter list if possible
शून्य V3Const::constifyParamsNoWarnEdit(Ast_गाठ* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS_NOWARN, /* globalPass: */ असत्य};
    // cppcheck-suppress constVariablePointer // edited below
    जर (AstVar* स्थिरांक varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        जर (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
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
शून्य V3Const::constifyGenerateParamsEdit(Ast_गाठ* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE:");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized
    // yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthGenerateParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_GENERATE, /* globalPass: */ असत्य};
    // cppcheck-suppress constVariablePointer // edited below
    जर (AstVar* स्थिरांक varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        जर (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
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
    V3Global::dump_Check_Global_वृक्ष("const", 0, dump_वृक्ष_Either_Level() >= 3);
}

शून्य V3Const::constifyCpp(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ सत्य};
        (शून्य)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("const_cpp", 0, dump_वृक्ष_Either_Level() >= 3);
}

Ast_गाठ* V3Const::constifyEdit(Ast_गाठ* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_NOWARN, /* globalPass: */ असत्य};
    nodep = visitor.mainAcceptEdit(nodep);
    परत nodep;
}

Ast_गाठ* V3Const::constifyEditCpp(Ast_गाठ* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ असत्य};
    nodep = visitor.mainAcceptEdit(nodep);
    परत nodep;
}

शून्य V3Const::constifyAllLive(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    // This only pushes constants up, doesn't make any other edits
    // IE doesn't prune dead statements, as we need to do some usability checks after this
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_LIVE, /* globalPass: */ सत्य};
        (शून्य)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("const", 0, dump_वृक्ष_Either_Level() >= 3);
}

शून्य V3Const::constifyAll(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ सत्य};
        (शून्य)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("const", 0, dump_वृक्ष_Either_Level() >= 3);
}

Ast_गाठ* V3Const::constifyExpensiveEdit(Ast_गाठ* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ असत्य};
    nodep = visitor.mainAcceptEdit(nodep);
    परत nodep;
}
