// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Convert DfgLogic into primitive operations
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
//
// Synthesize DfgLogic vertices in as a graph, as created by V3DfgAstToDfg
// into primitive vertices.
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Cfg.h"
#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <iterator>

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ {

// Create a DfgVertex out of a AstNodeExpr. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_ಶೃಂಗ, ಬಗೆ_ಹೆಸರು T_ಗಂಟು>
T_ಶೃಂಗ* make_ಶೃಂಗ(ಸ್ಥಿರಾಂಕ T_ಗಂಟು* nodep, Dfg_ನಕ್ಷೆ& dfg, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype) {
    ಹಿಂತಿರುಗಿ ಹೊಸ T_ಶೃಂಗ{dfg, nodep->fileline(), dtype};
}

ಮಾದರಿ <>
DfgArraySel* make_ಶೃಂಗ<DfgArraySel, AstArraySel>(ಸ್ಥಿರಾಂಕ AstArraySel* nodep, Dfg_ನಕ್ಷೆ& dfg,
                                                  ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype) {
    // Some earlier passes create malformed ArraySels, just bail on those...
    // See t_bitsel_wire_array_bad
    ಆದರೆ (VN_IS(nodep->fromp(), Const)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    ಆದರೆ (!VN_IS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    ಹಿಂತಿರುಗಿ ಹೊಸ DfgArraySel{dfg, nodep->fileline(), dtype};
}

}  // namespace

// Visitor that can convert Ast statements and expressions in Dfg vertices
ವರ್ಗ Ast_ಗಮ್ಯ_Dfg_Converter final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // STATE
    Dfg_ನಕ್ಷೆ& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats

    // Current logic vertex we are synthesizing
    DfgLogic* m_logicp = ಶೂನ್ಯಸೂಚಕ;
    // Variable updates produced by currently converted statement. This almost
    // always have a single element, so a vector is ok
    std::ಸದಿಶ<std::pair<AstVarScope*, Dfg_ಶೃಂಗ_Var*>>* m_updatesp = ಶೂನ್ಯಸೂಚಕ;

    ತಾರ್ಕಿಕ m_ಸಿಕ್ಕಿತು_Unhandled = ಸುಳ್ಳು;  // Found node not implemented as DFG or not implemented 'visit'
    ತಾರ್ಕಿಕ m_converting = ಸುಳ್ಳು;  // We are trying to convert some logic at the moment

    // METHODS

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2, ಬಗೆ_ಹೆಸರು... ವಾದಗಳು_2>
    ಶೃಂಗ_2* make(ವಾದಗಳು_2&&... ವಾದಗಳು) {
        static_assert(!std::is_ತಳ_ದ<Dfg_ಶೃಂಗ_Var, ಶೃಂಗ_2>::ಮೌಲ್ಯ, "Do not use for variables");
        static_assert(std::is_ತಳ_ದ<Dfg_ಶೃಂಗ, ಶೃಂಗ_2>::ಮೌಲ್ಯ, "'Vertex' must be a 'DfgVertex'");
        ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ vtxp = ಹೊಸ ಶೃಂಗ_2{m_dfg, std::forward<ವಾದಗಳು_2>(ವಾದಗಳು)...};
        m_logicp->synth().emplace_ಹಿಂಭಾಗ(vtxp);
        ಹಿಂತಿರುಗಿ vtxp;
    }

    // Returns true if the expression cannot (or should not) be represented by DFG
    ತಾರ್ಕಿಕ unhandled(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // Short-circuiting if something was already unhandled
        ಆದರೆ (m_ಸಿಕ್ಕಿತು_Unhandled) {
            // Impure nodes cannot be represented
            ಆದರೆ (!nodep->is_ಶುದ್ಧ()) {
                m_ಸಿಕ್ಕಿತು_Unhandled = ನಿಜ;
                ++m_ctx.m_conv.nonRepImpure;
            }
        }
        ಹಿಂತಿರುಗಿ m_ಸಿಕ್ಕಿತು_Unhandled;
    }

    ತಾರ್ಕಿಕ isSupported(ಸ್ಥಿರಾಂಕ AstVarRef* nodep) {
        // Cannot represent cross module references
        ಆದರೆ (nodep->classOrPackagep()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Check target
        ಹಿಂತಿರುಗಿ V3Dfg::isSupported(nodep->varScopep());
    }

    // Given an RValue expression, return the equivalent Vertex, or nullptr if not representable.
    Dfg_ಶೃಂಗ* convertRValue(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        UASSERT_OBJ(!m_converting, nodep, "'convertingRValue' should not be called recursively");
        VL_RESTORER(m_converting);
        VL_RESTORER(m_ಸಿಕ್ಕಿತು_Unhandled);
        m_converting = ನಿಜ;
        m_ಸಿಕ್ಕಿತು_Unhandled = ಸುಳ್ಳು;

        // Convert the expression
        iterate(nodep);

        // If falied to convert, return nullptr
        ಆದರೆ (m_ಸಿಕ್ಕಿತು_Unhandled) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        // Traversal set user2p to the equivalent vertex
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = nodep->user2u().ಗಮ್ಯ<Dfg_ಶೃಂಗ*>();
        UASSERT_OBJ(vtxp, nodep, "Missing Dfg vertex after conversion");
        ಹಿಂತಿರುಗಿ vtxp;
    }

    // Given an LValue expression, return the splice node that writes the
    // destination, together with the index to use for splicing in the value.
    // Returns {nullptr, 0}, if the given LValue expression is not supported.
    std::pair<Dfg_ಶೃಂಗ_Splice*, uint32_t> convertLValue(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ vrefp = VN_CAST(nodep, VarRef)) {
            UASSERT_OBJ(vrefp->access().isWriteOnly(), vrefp, "Non-WriteOnly reference");
            ಆದರೆ (!isSupported(vrefp)) {
                ++m_ctx.m_conv.nonRepLValue;
                ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0};
            }

            // Get (or create a new) temporary for this variable
            ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ vtxp = [&]() -> Dfg_ಶೃಂಗ_Var* {
                // The variable being assigned
                AstVarScope* ಸ್ಥಿರಾಂಕ vscp = vrefp->varScopep();

                // Find existing one, if any
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : *m_updatesp) {
                    ಆದರೆ (pair.ಮೊದಲ == vscp) ಹಿಂತಿರುಗಿ pair.ಎರಡನೇ;
                }

                // Create new one
                Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ newp = createTmp(*m_logicp, vscp, "SynthAssign");
                m_updatesp->emplace_ಹಿಂಭಾಗ(vscp, newp);

                // Create the Splice driver for the new temporary
                ಆದರೆ (newp->is<DfgVarPacked>()) {
                    newp->srcp(make<DfgSplicePacked>(newp->fileline(), newp->dtype()));
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (newp->is<DfgVarArray>()) {
                    newp->srcp(make<DfgSpliceArray>(newp->fileline(), newp->dtype()));
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->v3fatalSrc("Unhandled DfgVertexVar sub-type");
                }

                // Use new temporary
                ಹಿಂತಿರುಗಿ newp;
            }();

            // Return the Splice driver
            ಹಿಂತಿರುಗಿ {vtxp->srcp()->as<Dfg_ಶೃಂಗ_Splice>(), 0};
        }

        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSel* selp = VN_CAST(nodep, Sel)) {
            // Only handle constant selects
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ lsbp = VN_CAST(selp->lsbp(), Const);
            ಆದರೆ (!lsbp) {
                ++m_ctx.m_conv.nonRepLValue;
                ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0};
            }
            uint32_t lsb = lsbp->ಗಮ್ಯ_UInt();

            // Convert the 'fromp' sub-expression
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = convertLValue(selp->fromp());
            ಆದರೆ (!pair.ಮೊದಲ) ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0};
            DfgSplicePacked* ಸ್ಥಿರಾಂಕ splicep = pair.ಮೊದಲ->ಮಾದರಿ as<DfgSplicePacked>();
            // Adjust index.
            lsb += pair.ಎರಡನೇ;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            ಆದರೆ (lsb + static_cast<uint32_t>(selp->ಅಗಲ_Const()) > splicep->ಗಾತ್ರ_2()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0};
            }

            // AstSel doesn't change type kind (array vs packed), so we can use
            // the existing splice driver with adjusted lsb
            ಹಿಂತಿರುಗಿ {splicep, lsb};
        }

        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstArraySel* ಸ್ಥಿರಾಂಕ aselp = VN_CAST(nodep, ArraySel)) {
            // Only handle constant selects
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ indexp = VN_CAST(aselp->bitp(), Const);
            ಆದರೆ (!indexp) {
                ++m_ctx.m_conv.nonRepLValue;
                ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0};
            }
            uint32_t ಸೂಚ್ಯಂಕ = indexp->ಗಮ್ಯ_UInt();

            // Convert the 'fromp' sub-expression
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = convertLValue(aselp->fromp());
            ಆದರೆ (!pair.ಮೊದಲ) ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0};
            DfgSpliceArray* ಸ್ಥಿರಾಂಕ splicep = pair.ಮೊದಲ->ಮಾದರಿ as<DfgSpliceArray>();
            // Adjust index. Note pair.second is always 0, but we might handle array slices later..
            ಸೂಚ್ಯಂಕ += pair.ಎರಡನೇ;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            ಆದರೆ (ಸೂಚ್ಯಂಕ + 1U > splicep->ಗಾತ್ರ_2()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0};
            }

            // Ensure the Splice driver exists for this element
            ಆದರೆ (!splicep->driverAt(ಸೂಚ್ಯಂಕ)) {
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
                // This should never fail
                ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype = *Dfg_ದತ್ತಾಂಶ_Type::fromAst(nodep->dtypep());
                ಆದರೆ (dtype.isPacked()) {
                    DfgSplicePacked* ಸ್ಥಿರಾಂಕ newp = make<DfgSplicePacked>(flp, dtype);
                    ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& uaDtype = Dfg_ದತ್ತಾಂಶ_Type::array(dtype, 1);
                    DfgUnitArray* ಸ್ಥಿರಾಂಕ uap = make<DfgUnitArray>(flp, uaDtype);
                    uap->srcp(newp);
                    splicep->ಕೂಡಿಸು_Driver(uap, ಸೂಚ್ಯಂಕ, flp);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (dtype.isArray()) {
                    DfgSpliceArray* ಸ್ಥಿರಾಂಕ newp = make<DfgSpliceArray>(flp, dtype);
                    splicep->ಕೂಡಿಸು_Driver(newp, ಸೂಚ್ಯಂಕ, flp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->v3fatalSrc("Unhandled data type kind");
                }
            }

            // Return the splice driver
            Dfg_ಶೃಂಗ* driverp = splicep->driverAt(ಸೂಚ್ಯಂಕ);
            ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgUnitArray* ಸ್ಥಿರಾಂಕ uap = driverp->cast<DfgUnitArray>()) {
                driverp = uap->srcp();
            }
            ಹಿಂತಿರುಗಿ {driverp->as<Dfg_ಶೃಂಗ_Splice>(), 0};
        }

        ++m_ctx.m_conv.nonRepLValue;
        ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0};
    }

    // Given the LHS of an assignment, and the vertex representing the RHS,
    // connect up the RHS to drive the targets.
    // Returns true on success, false if the LHS is not representable.
    ತಾರ್ಕಿಕ convertAssignment(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Dfg_ಶೃಂಗ* vtxp) {
        // Represents a DFG assignment contributed by the AST assignment with the above 'lhsp'.
        // There might be multiple of these if 'lhsp' is a concatenation.
        ರಚನೆ Assignment final {
            Dfg_ಶೃಂಗ_Splice* m_lhsp;  // Splice vertex being driven
            uint32_t m_idx;  // Position (bit or array index) within m_lhsp being driven
            Dfg_ಶೃಂಗ* m_rhsp;  // Vertex driving that position
            Assignment() = ಅಳಿಸು;
            Assignment(Dfg_ಶೃಂಗ_Splice* lhsp, uint32_t idx, Dfg_ಶೃಂಗ* rhsp)
                : m_lhsp{lhsp}
                , m_idx{idx}
                , m_rhsp{rhsp} {}
        };

        // Simplify the LHS, to get rid of things like SEL(CONCAT(_, _), _)
        ಆದರೆ (!VN_IS(lhsp, VarRef)) lhsp = VN_AS(V3Const::constifyExpensiveEdit(lhsp), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);

        // Assigning compound expressions to a concatenated LHS requires a temporary
        // to avoid multiple use of the expression
        ಆದರೆ (VN_IS(lhsp, Concat) && !vtxp->is<Dfg_ಶೃಂಗ_Var>() && !vtxp->is<DfgConst>()) {
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ tmpp = createTmp(*m_logicp, flp, vtxp->dtype(), "Unpack");
            tmpp->srcp(vtxp);
            vtxp = tmpp;
        }

        // Convert each concatenation LHS separately, gather all assignments
        // we need to do into 'assignments', return true if all LValues
        // converted successfully.
        std::ಸದಿಶ<Assignment> assignments;
        ಸ್ಥಿರಾಂಕ std::function<ತಾರ್ಕಿಕ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, uint32_t)> convertAllLValues
            = [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subp, uint32_t lsb) -> ತಾರ್ಕಿಕ {
            // Concatenation on the LHS, convert each part
            ಆದರೆ (AstConcat* ಸ್ಥಿರಾಂಕ concatp = VN_CAST(subp, Concat)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cRhsp = concatp->rhsp();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cLhsp = concatp->lhsp();
                // Convert Rigth of concat
                ಆದರೆ (!convertAllLValues(cRhsp, lsb)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                // Convert Left of concat
                ಹಿಂತಿರುಗಿ convertAllLValues(cLhsp, lsb + cRhsp->ಅಗಲ());
            }

            // Non-concatenation, convert the LValue
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = convertLValue(subp);
            ಆದರೆ (!pair.ಮೊದಲ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

            // If whole lhs, just use it
            ಆದರೆ (subp == lhsp) {
                assignments.emplace_ಹಿಂಭಾಗ(pair.ಮೊದಲ, pair.ಎರಡನೇ, vtxp);
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }

            // Otherwise select the relevant bits
            ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype = *Dfg_ದತ್ತಾಂಶ_Type::fromAst(subp->dtypep());
            DfgSel* ಸ್ಥಿರಾಂಕ selp = make<DfgSel>(subp->fileline(), dtype);
            selp->fromp(vtxp);
            selp->lsb(lsb);
            assignments.emplace_ಹಿಂಭಾಗ(pair.ಮೊದಲ, pair.ಎರಡನೇ, selp);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        };

        // Convert the given LHS assignment, give up if any LValues failed to convert
        ಆದರೆ (!convertAllLValues(lhsp, 0)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // All successful, connect the drivers
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Assignment& ಅಂಶ_2 : assignments) {
            ಆದರೆ (DfgSplicePacked* ಸ್ಥಿರಾಂಕ spp = ಅಂಶ_2.m_lhsp->ಮಾದರಿ cast<DfgSplicePacked>()) {
                spp->ಕೂಡಿಸು_Driver(ಅಂಶ_2.m_rhsp, ಅಂಶ_2.m_idx, flp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (DfgSpliceArray* ಸ್ಥಿರಾಂಕ sap = ಅಂಶ_2.m_lhsp->ಮಾದರಿ cast<DfgSpliceArray>()) {
                // TODO: multi-dimensional arrays will need changes here
                ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& rDt = ಅಂಶ_2.m_rhsp->dtype();
                ಆದರೆ (rDt.isPacked()) {
                    // RHS is assigning an element of this array. Need a DfgUnitArray adapter.
                    DfgUnitArray* ಸ್ಥಿರಾಂಕ uap = make<DfgUnitArray>(flp, Dfg_ದತ್ತಾಂಶ_Type::array(rDt, 1));
                    uap->srcp(ಅಂಶ_2.m_rhsp);
                    sap->ಕೂಡಿಸು_Driver(uap, ಅಂಶ_2.m_idx, flp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // RHS is assigning an array (or array slice). Should be the same element type.
                    sap->ಕೂಡಿಸು_Driver(ಅಂಶ_2.m_rhsp, ಅಂಶ_2.m_idx, flp);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಅಂಶ_2.m_lhsp->v3fatalSrc("Unhandled DfgVertexSplice sub-type");
            }
        }

        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // VISITORS

    // Unhandled node
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* /*nodep*/) override {
        ಆದರೆ (!m_ಸಿಕ್ಕಿತು_Unhandled && m_converting) ++m_ctx.m_conv.nonRepUnknown;
        m_ಸಿಕ್ಕಿತು_Unhandled = ನಿಜ;
    }

    // Expressions - mostly auto generated, but a few special ones
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ಆದರೆ (unhandled(nodep)) ಹಿಂತಿರುಗಿ;
        // This visit method is only called on RValues, where only read refs are supported
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Non-ReadOnly reference");
        ಆದರೆ (!isSupported(nodep)) {
            m_ಸಿಕ್ಕಿತು_Unhandled = ನಿಜ;
            ++m_ctx.m_conv.nonRepVarRef;
            ಹಿಂತಿರುಗಿ;
        }

        // Variable should have been bound before starting conversion
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = nodep->varScopep()->user2u().ಮಾದರಿ ಗಮ್ಯ<Dfg_ಶೃಂಗ_Var*>();
        UASSERT_OBJ(vtxp, nodep, "Referenced variable has no associated DfgVertexVar");
        nodep->user2p(vtxp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConst* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ಆದರೆ (unhandled(nodep)) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (nodep->ಅಗಲ() != nodep->ಅಂಕೆ().ಅಗಲ()) {
            // Sometimes the width of the AstConst is not the same as the
            // V3Number it holds. Truncate it here. TODO: should this be allowed?
            V3_ಸಂಖ್ಯೆ ಅಂಕೆ{nodep, nodep->ಅಗಲ()};
            ಅಂಕೆ.opSel(nodep->ಅಂಕೆ(), nodep->ಅಗಲ() - 1, 0);
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = make<DfgConst>(nodep->fileline(), ಅಂಕೆ);
            nodep->user2p(vtxp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = make<DfgConst>(nodep->fileline(), nodep->ಅಂಕೆ());
            nodep->user2p(vtxp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCReset* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ಆದರೆ (unhandled(nodep)) ಹಿಂತಿರುಗಿ;

        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type* ಸ್ಥಿರಾಂಕ dtypep = Dfg_ದತ್ತಾಂಶ_Type::fromAst(nodep->dtypep());
        ಆದರೆ (!dtypep) {
            m_ಸಿಕ್ಕಿತು_Unhandled = ನಿಜ;
            ++m_ctx.m_conv.nonRepDType;
            ಹಿಂತಿರುಗಿ;
        }

        UASSERT_OBJ(!nodep->constructing(), nodep,
                    "CReset should be non-constructing at this stage");

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = make<DfgCReset>(nodep->fileline(), *dtypep);
        nodep->user2p(vtxp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಹೊಂದಿಕೆ_Masked* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ಆದರೆ (unhandled(nodep)) ಹಿಂತಿರುಗಿ;

        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type* ಸ್ಥಿರಾಂಕ dtypep = Dfg_ದತ್ತಾಂಶ_Type::fromAst(nodep->dtypep());
        ಆದರೆ (!dtypep) {
            m_ಸಿಕ್ಕಿತು_Unhandled = ನಿಜ;
            ++m_ctx.m_conv.nonRepDType;
            ಹಿಂತಿರುಗಿ;
        }

        iterate(nodep->lhsp());
        ಆದರೆ (m_ಸಿಕ್ಕಿತು_Unhandled) ಹಿಂತಿರುಗಿ;
        iterate(nodep->matchp());
        ಆದರೆ (m_ಸಿಕ್ಕಿತು_Unhandled) ಹಿಂತಿರುಗಿ;

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        Dfg_ಹೊಂದಿಕೆ_Masked* ಸ್ಥಿರಾಂಕ vtxp = make<Dfg_ಹೊಂದಿಕೆ_Masked>(flp, *dtypep);
        vtxp->lhsp(nodep->lhsp()->user2u().ಗಮ್ಯ<Dfg_ಶೃಂಗ*>());
        vtxp->matchp(nodep->matchp()->user2u().ಗಮ್ಯ<Dfg_ಶೃಂಗ*>());
        nodep->user2p(vtxp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstReplicate* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ಆದರೆ (unhandled(nodep)) ಹಿಂತಿರುಗಿ;

        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type* ಸ್ಥಿರಾಂಕ dtypep = Dfg_ದತ್ತಾಂಶ_Type::fromAst(nodep->dtypep());
        ಆದರೆ (!dtypep) {
            m_ಸಿಕ್ಕಿತು_Unhandled = ನಿಜ;
            ++m_ctx.m_conv.nonRepDType;
            ಹಿಂತಿರುಗಿ;
        }

        iterate(nodep->srcp());
        ಆದರೆ (m_ಸಿಕ್ಕಿತು_Unhandled) ಹಿಂತಿರುಗಿ;

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        DfgRep* ಸ್ಥಿರಾಂಕ vtxp = make<DfgRep>(flp, *dtypep);
        vtxp->srcp(nodep->srcp()->user2u().ಗಮ್ಯ<Dfg_ಶೃಂಗ*>());
        nodep->user2p(vtxp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ಆದರೆ (unhandled(nodep)) ಹಿಂತಿರುಗಿ;

        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type* ಸ್ಥಿರಾಂಕ dtypep = Dfg_ದತ್ತಾಂಶ_Type::fromAst(nodep->dtypep());
        ಆದರೆ (!dtypep) {
            m_ಸಿಕ್ಕಿತು_Unhandled = ನಿಜ;
            ++m_ctx.m_conv.nonRepDType;
            ಹಿಂತಿರುಗಿ;
        }

        iterate(nodep->fromp());
        ಆದರೆ (m_ಸಿಕ್ಕಿತು_Unhandled) ಹಿಂತಿರುಗಿ;

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        Dfg_ಶೃಂಗ* vtxp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->lsbp(), Const)) {
            ಸ್ಥಿರಾಂಕ uint32_t lsb = constp->ಗಮ್ಯ_UInt();
            ಸ್ಥಿರಾಂಕ uint32_t msb = lsb + nodep->ಅಗಲ_Const() - 1;
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromp = nodep->fromp()->user2u().ಗಮ್ಯ<Dfg_ಶೃಂಗ*>();
            // Unfortunately we can still have out of bounds selects due to how
            // indices are truncated for speed reasons in V3Width/V3Unknown.
            ಆದರೆ (msb >= fromp->ಗಾತ್ರ_2()) {
                m_ಸಿಕ್ಕಿತು_Unhandled = ನಿಜ;
                ++m_ctx.m_conv.nonRepOOBSel;
                ಹಿಂತಿರುಗಿ;
            }
            DfgSel* ಸ್ಥಿರಾಂಕ selp = make<DfgSel>(flp, *dtypep);
            selp->fromp(fromp);
            selp->lsb(lsb);
            vtxp = selp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterate(nodep->lsbp());
            ಆದರೆ (m_ಸಿಕ್ಕಿತು_Unhandled) ಹಿಂತಿರುಗಿ;
            DfgMux* ಸ್ಥಿರಾಂಕ muxp = make<DfgMux>(flp, *dtypep);
            muxp->fromp(nodep->fromp()->user2u().ಗಮ್ಯ<Dfg_ಶೃಂಗ*>());
            muxp->lsbp(nodep->lsbp()->user2u().ಗಮ್ಯ<Dfg_ಶೃಂಗ*>());
            vtxp = muxp;
        }
        nodep->user2p(vtxp);
    }

// The rest of the visit methods for expressions are generated by 'astgen'
#ಅನ್ತರ್ಭಾವ "V3Dfg__gen_ast_to_dfg.h"

ಸಾರ್ವಜನಿಕ:
    // PUBLIC METHODS

    // Create temporay variable capable of holding the given type
    Dfg_ಶೃಂಗ_Var* createTmp(DfgLogic& logic, File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype,
                            ಸ್ಥಿರಾಂಕ std::string& ಪೂರ್ವಪ್ರತ್ಯಯ) {
        Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ vtxp = m_dfg.makeNewVar(flp, ಪೂರ್ವಪ್ರತ್ಯಯ, dtype, logic.scopep());
        logic.synth().emplace_ಹಿಂಭಾಗ(vtxp);
        vtxp->vscp()->varp()->is_ಆಂತರಿಕ(ನಿಜ);
        vtxp->tmpForp(vtxp->vscp());
        ಹಿಂತಿರುಗಿ vtxp;
    }

    // Create a new temporary variable capable of holding 'varp'
    Dfg_ಶೃಂಗ_Var* createTmp(DfgLogic& logic, AstVarScope* vscp, ಸ್ಥಿರಾಂಕ std::string& ಪೂರ್ವಪ್ರತ್ಯಯ) {
        AstVar* ಸ್ಥಿರಾಂಕ astVarp = vscp->varp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = astVarp->fileline();
        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype = *Dfg_ದತ್ತಾಂಶ_Type::fromAst(astVarp->dtypep());
        ಸ್ಥಿರಾಂಕ std::string prfx = ಪೂರ್ವಪ್ರತ್ಯಯ + "_" + astVarp->name();
        Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ vtxp = createTmp(logic, flp, dtype, prfx);
        vtxp->tmpForp(vscp);
        ಹಿಂತಿರುಗಿ vtxp;
    }

    // Convert AstAssign to Dfg, return true if successful.
    // Fills 'updates' with bindings for assigned variables.
    ತಾರ್ಕಿಕ convert(std::ಸದಿಶ<std::pair<AstVarScope*, Dfg_ಶೃಂಗ_Var*>>& updates, DfgLogic& vtx,
                 Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) {
        UASSERT_OBJ(VN_IS(nodep, ನಿಯೋಜಿಸು) || VN_IS(nodep, ನಿಯೋಜಿಸು_W), nodep, "Bad NodeAssign");
        UASSERT_OBJ(updates.ಖಾಲಿಗಳು(), nodep, "'updates' should be empty");
        VL_RESTORER(m_updatesp);
        VL_RESTORER(m_logicp);
        m_updatesp = &updates;
        m_logicp = &vtx;
        // Assignment with timing control shouldn't make it this far
        UASSERT_OBJ(!nodep->timingControlp(), nodep, "Shouldn't make it this far");
        // Convert it
        ++m_ctx.m_conv.ಒಳಹರಿವು_Assignments;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp();
        // Check data types are compatible.
        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type* ಸ್ಥಿರಾಂಕ lDtypep = Dfg_ದತ್ತಾಂಶ_Type::fromAst(lhsp->dtypep());
        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type* ಸ್ಥಿರಾಂಕ rDtypep = Dfg_ದತ್ತಾಂಶ_Type::fromAst(rhsp->dtypep());
        ಆದರೆ (!lDtypep || !rDtypep) {
            ++m_ctx.m_conv.nonRepDType;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        // For now, only direct array assignment is supported (e.g. a = b, but not a = _ ? b : c)
        ಆದರೆ (rDtypep->isArray()) {
            ಆದರೆ (!VN_IS(rhsp, VarRef)
                || !lhsp->dtypep()->skipRefp()->same_ವೃಕ್ಷ(rhsp->dtypep()->skipRefp())) {
                ++m_ctx.m_conv.nonRepDType;
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
        }
        // Widths should match at this point
        UASSERT_OBJ(lhsp->ಅಗಲ() == rhsp->ಅಗಲ(), nodep, "Mismatched width reached DFG");
        // Convert the RHS expression
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rVtxp = convertRValue(rhsp);
        ಆದರೆ (!rVtxp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Connect the RHS vertex to the LHS targets
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ success = convertAssignment(nodep->fileline(), lhsp, rVtxp);
        ಆದರೆ (success) ++m_ctx.m_conv.representable;
        ಹಿಂತಿರುಗಿ success;
    }

    // Convert RValue expression to Dfg. Returns nullptr if failed.
    Dfg_ಶೃಂಗ* convert(DfgLogic& vtx, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;
        // Convert it
        ++m_ctx.m_conv.ಒಳಹರಿವು_Expressions;
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = convertRValue(nodep);
        ಆದರೆ (vtxp) ++m_ctx.m_conv.representable;
        ಹಿಂತಿರುಗಿ vtxp;
    }

    // CONSTRUCTOR
    Ast_ಗಮ್ಯ_Dfg_Converter(Dfg_ನಕ್ಷೆ& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {}
};

// Debug aid - outisde 'AstToDfgSynthesize' as it is a template, but want one instance
V3DebugBisect s_dfgSynthDebugBisect{"DfgSynthesize"};

ವರ್ಗ Ast_ಗಮ್ಯ_Dfg_Synthesize final {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // TYPES

    // SymTab must be ordered in order to yield stable results
    ರಚನೆ AstVarScopeComparator final {
        ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ s_vscpIdCounter;  // Counter for lazily allocating the unique AstVarScope IDs
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(AstVarScope* lhs, AstVarScope* rhs) ಸ್ಥಿರಾಂಕ {
            ಆದರೆ (!lhs->user4()) lhs->user4(++s_vscpIdCounter);
            ಆದರೆ (!rhs->user4()) rhs->user4(++s_vscpIdCounter);
            ಹಿಂತಿರುಗಿ lhs->user4() < rhs->user4();
        }
    };
    ಬಳಸಿ SymTab = std::map<AstVarScope*, Dfg_ಶೃಂಗ_Var*, AstVarScopeComparator>;

    // Represents a [potentially partial] driver of a variable
    ರಚನೆ Driver final {
        Dfg_ಶೃಂಗ* m_vtxp = ಶೂನ್ಯಸೂಚಕ;  // Driving vertex
        uint32_t m_lo = 0;  // Low index of driven range (internal, not Verilog)
        uint32_t m_hi = 0;  // High index of driven range (internal, not Verilog)
        File_ಸಾಲು* m_flp = ಶೂನ್ಯಸೂಚಕ;  // Location of driver in source

        Driver() = ಪೂರ್ವನಿಯೋಜಿತ;
        Driver(Dfg_ಶೃಂಗ* vtxp, uint32_t lo, File_ಸಾಲು* flp)
            : m_vtxp{vtxp}
            , m_lo{lo}
            , m_hi{lo + vtxp->ಗಾತ್ರ_2() - 1U}
            , m_flp{flp} {}
        ಕ್ರಿಯಾಕಾರಕ ತಾರ್ಕಿಕ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vtxp != ಶೂನ್ಯಸೂಚಕ; }

        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ Driver& other) ಸ್ಥಿರಾಂಕ {
            ಆದರೆ (m_lo != other.m_lo) ಹಿಂತಿರುಗಿ m_lo < other.m_lo;
            ಆದರೆ (m_hi != other.m_hi) ಹಿಂತಿರುಗಿ m_hi < other.m_hi;
            ಹಿಂತಿರುಗಿ m_flp->operatorCompare(*other.m_flp) < 0;
        }

        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ<=(ಸ್ಥಿರಾಂಕ Driver& other) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !(other < *ಇದು); }
    };

    // STATE - Persistent
    Dfg_ನಕ್ಷೆ& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats
    Ast_ಗಮ್ಯ_Dfg_Converter m_converter;  // The convert instance to use for each construct
    DfgWorklist m_ಗಮ್ಯ_Revert{m_dfg};  // We need a worklist for reverting synthesis

    // STATE - for current DfgLogic being synthesized
    DfgLogic* m_logicp = ಶೂನ್ಯಸೂಚಕ;  // Current logic vertex we are synthesizing
    Cfg_ಖಂಡ_Map<SymTab> m_bb_ಗಮ್ಯ_ISym_Tab;  // Map from CfgBlock -> input symbol table
    Cfg_ಖಂಡ_Map<SymTab> m_bb_ಗಮ್ಯ_OSym_Tab;  // Map from CfgBlock -> output symbol table
    Cfg_ಖಂಡ_Map<Dfg_ಶೃಂಗ_Var*> m_bb_ಗಮ್ಯ_Condp;  // Map from CfgBlock ->  terminating branch condition
    Cfg_ಅಂಚು_Map<Dfg_ಶೃಂಗ_Var*> m_ಅಂಚು_ಗಮ್ಯ_Predicatep;  // Map CfgGraphEdge -> path predicate to there
    Cfg_Dominator_ವೃಕ್ಷ m_dom_ವೃಕ್ಷ;  // The dominator tree of the current CFG

    // STATE - Some debug aid
    // We stop after synthesizing s_dfgSynthDebugLimit vertices (if non-zero).
    // This is the problematic logic (last one we synthesize), assuming a
    // bisection search over s_dfgSynthDebugLimit.
    DfgLogic* m_debugLogicp = ಶೂನ್ಯಸೂಚಕ;
    // Source (upstream) cone of outputs of m_debugLogicp
    std::unique_ptr<std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*>> m_debugOSrcConep{ಶೂನ್ಯಸೂಚಕ};

    // METHODS

    // Dump current graph for debugging ...
    ಶೂನ್ಯ debugDump(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* name) {
        // If we have the debugged logic, compute the vertices feeding its outputs
        ಆದರೆ (VL_UNLIKELY(m_debugLogicp)) {
            std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*> outputs;
            m_debugLogicp->foreachSink([&outputs](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& v) {
                outputs.emplace_ಹಿಂಭಾಗ(v.singleSink()->as<Dfg_ಶೃಂಗ_Var>());
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            m_debugOSrcConep = m_dfg.ಮೂಲ_Cone(outputs);
        }

        ಆದರೆ (VL_UNLIKELY(dumpDfgLevel() >= 9 || m_debugOSrcConep)) {
            m_dfg.dumpDotFilePrefixed(name);
            ಆದರೆ (m_debugOSrcConep) {
                // Dump only the subgraph involving the inputs and outputs of the bad vertex
                m_dfg.dumpDotFilePrefixed(name + "-min"s, [&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& v) -> ತಾರ್ಕಿಕ {
                    ಹಿಂತಿರುಗಿ m_debugOSrcConep->ಎಣಿಕೆ_2(&v);
                });
            }
        }
    }

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2, ಬಗೆ_ಹೆಸರು... ವಾದಗಳು_2>
    ಶೃಂಗ_2* make(ವಾದಗಳು_2&&... ವಾದಗಳು) {
        static_assert(!std::is_ತಳ_ದ<Dfg_ಶೃಂಗ_Var, ಶೃಂಗ_2>::ಮೌಲ್ಯ, "Do not use for variables");
        static_assert(std::is_ತಳ_ದ<Dfg_ಶೃಂಗ, ಶೃಂಗ_2>::ಮೌಲ್ಯ, "'Vertex' must be a 'DfgVertex'");
        ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ vtxp = ಹೊಸ ಶೃಂಗ_2{m_dfg, std::forward<ವಾದಗಳು_2>(ವಾದಗಳು)...};
        ಆದರೆ (m_logicp) m_logicp->synth().emplace_ಹಿಂಭಾಗ(vtxp);
        ಹಿಂತಿರುಗಿ vtxp;
    }

    // Gather all drivers of a resolved variable
    ಸ್ಥಿರ std::ಸದಿಶ<Driver> gatherDrivers(Dfg_ಶೃಂಗ_Splice* vtxp) {
        // Collect them all, check if they are sorted
        std::ಸದಿಶ<Driver> drivers;
        drivers.reserve(vtxp->nInputs());
        ತಾರ್ಕಿಕ sorted = ನಿಜ;
        vtxp->foreachDriver([&](Dfg_ಶೃಂಗ& src, uint32_t lo, File_ಸಾಲು* flp) {
            // Collect the driver
            drivers.emplace_ಹಿಂಭಾಗ(&src, lo, flp);
            // Check if drivers are sorted - most often they are
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t n = drivers.ಗಾತ್ರ_2();
            ಆದರೆ (n >= 2 && drivers[n - 1] < drivers[n - 2]) sorted = ಸುಳ್ಳು;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });

        // Sort if unsorted
        ಆದರೆ (!sorted) std::stable_ವಿಂಗಡಿಸು(drivers.begin(), drivers.ಕೊನೆ());

        //ಮುಗಿದ
        ಹಿಂತಿರುಗಿ drivers;
    }

    // Returns true if the driver supplies a complete array element.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ driver_Covers_ಸಂಪೂರ್ಣ_Element(ಸ್ಥಿರಾಂಕ Driver& driver) {
        ಸ್ಥಿರಾಂಕ DfgUnitArray* ಸ್ಥಿರಾಂಕ unitp = driver.m_vtxp->cast<DfgUnitArray>();
        ಆದರೆ (!unitp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (Dfg_ಶೃಂಗ_Splice* ಸ್ಥಿರಾಂಕ splicep = unitp->srcp()->cast<Dfg_ಶೃಂಗ_Splice>()) {
            ಹಿಂತಿರುಗಿ splicep->wholep();
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Unlike 'wholep', several element drivers can collectively cover an array.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ splice_Covers_ಸಂಪೂರ್ಣ(Dfg_ಶೃಂಗ_Splice* ಸ್ಥಿರಾಂಕ splicep) {
        ಆದರೆ (splicep->wholep()) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (splicep->isPacked()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        uint32_t ಮುಂದಿನ = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Driver& driver : gatherDrivers(splicep)) {
            ಆದರೆ (driver.m_lo != ಮುಂದಿನ || !driver_Covers_ಸಂಪೂರ್ಣ_Element(driver)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಮುಂದಿನ = driver.m_hi + 1;
        }
        ಹಿಂತಿರುಗಿ ಮುಂದಿನ == splicep->ಗಾತ್ರ_2();
    }

    // Returns true if the driver cone contains any variable introduced by
    // tristate lowering. Used to distinguish intentional tristate contributor
    // overlap from accidental multidrive.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಒಳಗೊಂಡಿದೆ_Tri_Lowered_Var(Dfg_ಶೃಂಗ* rootp) {
        std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*> ರಾಶಿ;
        std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*> ಭೇಟಿಯಾದ;
        ರಾಶಿ.emplace_ಹಿಂಭಾಗ(rootp);
        ಆಗ (!ರಾಶಿ.ಖಾಲಿಗಳು()) {
            ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = ರಾಶಿ.ಹಿಂಭಾಗ();
            ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
            ಆದರೆ (std::ಕಂಡುಹಿಡಿ(ಭೇಟಿಯಾದ.begin(), ಭೇಟಿಯಾದ.ಕೊನೆ(), vtxp) != ಭೇಟಿಯಾದ.ಕೊನೆ()) ಮುಂದುವರಿ;
            ಭೇಟಿಯಾದ.emplace_ಹಿಂಭಾಗ(vtxp);
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = vtxp->cast<Dfg_ಶೃಂಗ_Var>()) {
                ಆದರೆ (varp->vscp()->varp()->dfgTriLowered()) ಹಿಂತಿರುಗಿ ನಿಜ;
            }
            vtxp->foreach_ಮೂಲ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& src) {
                ರಾಶಿ.emplace_ಹಿಂಭಾಗ(&src);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Returns true if the driver is a direct variable forward (no logic).
    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_ನೇರ_Var_Driver(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* vtxp) { ಹಿಂತಿರುಗಿ vtxp->is<Dfg_ಶೃಂಗ_Var>(); }

    // Gather all synthesized drivers of an unresolved variable
    ಸ್ಥಿರ std::ಸದಿಶ<Driver> gatherDriversUnresolved(DfgUnresolved* vtxp) {
        std::ಸದಿಶ<Driver> drivers;
        drivers.reserve(vtxp->nInputs());

        // For better locations in error reporting, we unpick concatenations
        // which are sometimes introduced by combinint assignments in V3Const.
        ಸ್ಥಿರಾಂಕ std::function<ಶೂನ್ಯ(Dfg_ಶೃಂಗ*, uint32_t, File_ಸಾಲು*)> gather
            = [&](Dfg_ಶೃಂಗ* vtxp, uint32_t lo, File_ಸಾಲು* flp) -> ಶೂನ್ಯ {
            ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ concatp = vtxp->cast<DfgConcat>()) {
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = concatp->rhsp();
                gather(rhsp, lo, rhsp->fileline());
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = concatp->lhsp();
                gather(lhsp, lo + rhsp->ಅಗಲ(), lhsp->fileline());
                ಹಿಂತಿರುಗಿ;
            }
            drivers.emplace_ಹಿಂಭಾಗ(vtxp, lo, flp);
        };

        // Gather all synthesized drivers
        vtxp->foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& src) {
            // Can ignore the original DfgLogic
            ಆದರೆ (src.is<DfgLogic>()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Synthesized drivers must be a splice at this point
            Dfg_ಶೃಂಗ_Splice* ಸ್ಥಿರಾಂಕ splicep = src.as<Dfg_ಶೃಂಗ_Splice>();
            // Collect the driver
            splicep->foreachDriver([&](Dfg_ಶೃಂಗ& src, uint32_t lo, File_ಸಾಲು* flp) {
                gather(&src, lo, flp);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });

        // Sort the drivers
        std::stable_ವಿಂಗಡಿಸು(drivers.begin(), drivers.ಕೊನೆ());

        //ಮುಗಿದ
        ಹಿಂತಿರುಗಿ drivers;
    }

    // Given two drivers, combine the driven sub-ranges into the first one if
    // possible. First bool returned indicates successfully combined and there
    // are no multi-driven bits. Second bool returned indicates we warned
    // already about multi-driven bits.
    std::pair<ತಾರ್ಕಿಕ, ತಾರ್ಕಿಕ> combineDrivers(Dfg_ಶೃಂಗ_Var& var, ಸ್ಥಿರಾಂಕ std::string& sub,  //
                                         Driver& a, ಸ್ಥಿರಾಂಕ Driver& b) {
        // We can only combine array drivers ...
        ಆದರೆ (a.m_vtxp->isPacked()) ಹಿಂತಿರುಗಿ {ಸುಳ್ಳು, ಸುಳ್ಳು};
        // ... that both drive a single element ...
        ಆದರೆ (a.m_lo != b.m_lo) ಹಿಂತಿರುಗಿ {ಸುಳ್ಳು, ಸುಳ್ಳು};
        ಸ್ಥಿರಾಂಕ DfgUnitArray* ಸ್ಥಿರಾಂಕ aUap = a.m_vtxp->ಮಾದರಿ cast<DfgUnitArray>();
        ಆದರೆ (!aUap) ಹಿಂತಿರುಗಿ {ಸುಳ್ಳು, ಸುಳ್ಳು};
        ಸ್ಥಿರಾಂಕ DfgUnitArray* ಸ್ಥಿರಾಂಕ bUap = b.m_vtxp->ಮಾದರಿ cast<DfgUnitArray>();
        ಆದರೆ (!bUap) ಹಿಂತಿರುಗಿ {ಸುಳ್ಳು, ಸುಳ್ಳು};
        // ... and are themeselves partial
        DfgSplicePacked* ಸ್ಥಿರಾಂಕ aSp = aUap->srcp()->ಮಾದರಿ cast<DfgSplicePacked>();
        ಆದರೆ (!aSp) ಹಿಂತಿರುಗಿ {ಸುಳ್ಳು, ಸುಳ್ಳು};
        DfgSplicePacked* ಸ್ಥಿರಾಂಕ bSp = bUap->srcp()->ಮಾದರಿ cast<DfgSplicePacked>();
        ಆದರೆ (!bSp) ಹಿಂತಿರುಗಿ {ಸುಳ್ಳು, ಸುಳ್ಳು};
        UASSERT_OBJ(aSp->dtype() == bSp->dtype(), &var, "DTypes should match");

        // Gather drivers of a
        std::ಸದಿಶ<Driver> aDrivers = gatherDrivers(aSp);
        // Gather drivers of b
        std::ಸದಿಶ<Driver> bDrivers = gatherDrivers(bSp);

        // Merge them
        std::ಸದಿಶ<Driver> abDrivers;
        abDrivers.reserve(aDrivers.ಗಾತ್ರ_2() + bDrivers.ಗಾತ್ರ_2());
        std::ವಿಲೀನ(  //
            aDrivers.begin(), aDrivers.ಕೊನೆ(),  //
            bDrivers.begin(), bDrivers.ಕೊನೆ(),  //
            std::ಹಿಂಭಾಗ_inserter(abDrivers)  //
        );

        // Attempt to resolve them
        ಆದರೆ (!normalizeDrivers(var, abDrivers, sub + "[" + std::ಗಮ್ಯ_string(a.m_lo) + "]")) {
            ಹಿಂತಿರುಗಿ {ಸುಳ್ಳು, ನಿಜ};
        }

        // Successfully resolved. Needs a new splice and unit.
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = var.fileline();
        DfgSplicePacked* ಸ್ಥಿರಾಂಕ splicep = make<DfgSplicePacked>(flp, aSp->dtype());
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Driver& d : abDrivers) splicep->ಕೂಡಿಸು_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        DfgUnitArray* ಸ್ಥಿರಾಂಕ uap = make<DfgUnitArray>(flp, aUap->dtype());
        uap->srcp(splicep);
        a.m_vtxp = uap;
        ಹಿಂತಿರುಗಿ {ನಿಜ, ಸುಳ್ಳು};
    }

    // Combine and coalesce the given drivers.
    // Returns true iff no multi-driven bits are present.
    ತಾರ್ಕಿಕ normalizeDrivers(Dfg_ಶೃಂಗ_Var& var, std::ಸದಿಶ<Driver>& drivers,
                          ಸ್ಥಿರಾಂಕ std::string& sub = "") {
        ಆದರೆ (drivers.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ನಿಜ;

        // What type of values are we combining
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isPacked = drivers[0].m_vtxp->isPacked();

        // Found a multidriven part ?
        ತಾರ್ಕಿಕ multiDriven = ಸುಳ್ಳು;

        // Iterate through the sorted drivers. Index 'i' is the driver we are
        // resolving driver 'j' agains, and if required, we merge 'j' into 'i'.
        ಗಾತ್ರ_t i = 0;
        ಗಾಗಿ (ಗಾತ್ರ_t j = 1; j < drivers.ಗಾತ್ರ_2();) {
            UASSERT_OBJ(i < j, &var, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            ಆದರೆ (!jD) {
                ++j;
                ಮುಂದುವರಿ;
            }
            // If 'i' was moved, move 'j' in it's place
            ಆದರೆ (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                ಮುಂದುವರಿ;
            }

            // We have 2 valid drivers now
            UASSERT_OBJ(iD <= jD, &var, "Should always be sorted");
            UASSERT_OBJ(jD.m_vtxp->isPacked() == isPacked, &var, "Mixed type drivers");

            // If no overlap, consider next pair
            ಆದರೆ (iD.m_hi < jD.m_lo) {
                ++i;
                ಆದರೆ (i == j) ++j;
                ಮುಂದುವರಿ;
            }

            // There is an overlap. Attempt to combine them.
            ತಾರ್ಕಿಕ combined = ಸುಳ್ಳು;
            ತಾರ್ಕಿಕ warned = ಸುಳ್ಳು;
            std::tie(combined, warned) = combineDrivers(var, sub, iD, jD);

            // If sucessfully combined, 'j' is no longer needed, it was combined into 'i'
            ಆದರೆ (combined) {
                jD = Driver{};
                ++j;
                ಮುಂದುವರಿ;
            }

            // Found overlap that cannot be resolved
            multiDriven = ನಿಜ;
            // Compare next driver
            ++j;

            // Do not warn again if we warned during 'combineDrivers'
            ಆದರೆ (warned) ಮುಂದುವರಿ;

            // The variable to warn on
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = var.tmpForp() ? var.tmpForp() : var.vscp();

            // Loop index often abused, so suppress
            ಆದರೆ (vscp->varp()->is_ಬಳಸಿದ_Loop_Idx()) ಮುಂದುವರಿ;
            // Tristate lowering can intentionally create overlapping contributors.
            // Keep the signal marked multidriven for DFG fallback, but suppress
            // warning only when both overlapping driver cones look tri-lowered.
            ಆದರೆ (vscp->varp()->dfgAllowMultidriveTri()) {
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ iTri = ಒಳಗೊಂಡಿದೆ_Tri_Lowered_Var(iD.m_vtxp);
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ jTri = ಒಳಗೊಂಡಿದೆ_Tri_Lowered_Var(jD.m_vtxp);
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ triPair = iTri && jTri;
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ triAndBridge = (iTri && is_ನೇರ_Var_Driver(jD.m_vtxp))
                                          || (jTri && is_ನೇರ_Var_Driver(iD.m_vtxp));
                ಆದರೆ (triPair || triAndBridge) ಮುಂದುವರಿ;
            }

            // Warn the user now
            ಸ್ಥಿರಾಂಕ std::string lo = std::ಗಮ್ಯ_string(jD.m_lo);
            ಸ್ಥಿರಾಂಕ std::string hi = std::ಗಮ್ಯ_string(std::min(iD.m_hi, jD.m_hi));
            ಸ್ಥಿರಾಂಕ std::string ಬಗೆ = isPacked ? "Bit" : "Element";
            ಸ್ಥಿರಾಂಕ std::string part = hi == lo ? (" [" + lo + "]") : ("s [" + hi + ":" + lo + "]");

            vscp->v3warn(  //
                MULTIDRIVEN,  //
                ಬಗೆ << part << " of signal '" << vscp->prettyName() << sub << "'"
                     << " have multiple combinational drivers."
                     << " This can cause performance degradation.\n"
                     << iD.m_flp->warnOther() << "... Location of offending driver\n"
                     << iD.m_flp->warnContextPrimary() << '\n'
                     << jD.m_flp->warnOther() << "... Location of offending driver\n"
                     << jD.m_flp->warnContextSecondary());
        }
        // Rightsize vector
        drivers.resize(i + 1);

        // Coalesce adjacent drivers
        ಆದರೆ (!multiDriven && isPacked) coalesceDrivers(drivers);

        ಹಿಂತಿರುಗಿ !multiDriven;
    }

    // Coalesce adjacent drivers into single ones
    ಶೂನ್ಯ coalesceDrivers(std::ಸದಿಶ<Driver>& drivers) {
        UASSERT(!drivers.ಖಾಲಿಗಳು(), "Can't coalesce 0 drivers");
        UASSERT_OBJ(drivers[0].m_vtxp->isPacked(), drivers[0].m_vtxp, "Can only coalesce packed");

        ಗಾತ್ರ_t i = 0;
        ಗಾಗಿ (ಗಾತ್ರ_t j = 1; j < drivers.ಗಾತ್ರ_2();) {
            UASSERT(i < j, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            ಆದರೆ (!jD) {
                ++j;
                ಮುಂದುವರಿ;
            }
            // If 'i' was moved, move 'j' in it's place
            ಆದರೆ (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                ಮುಂದುವರಿ;
            }

            // We have 2 valid drivers now
            UASSERT(iD <= jD, "Should always be sorted");

            // If not adjacent, move on
            ಆದರೆ (iD.m_hi + 1 != jD.m_lo) {
                ++i;
                ಆದರೆ (i == j) ++j;
                ಮುಂದುವರಿ;
            }

            // Coalesce Adjacent ranges,
            ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dt = Dfg_ದತ್ತಾಂಶ_Type::packed(iD.m_vtxp->ಅಗಲ() + jD.m_vtxp->ಅಗಲ());
            DfgConcat* ಸ್ಥಿರಾಂಕ concatp = make<DfgConcat>(iD.m_flp, dt);
            concatp->rhsp(iD.m_vtxp);
            concatp->lhsp(jD.m_vtxp);
            iD.m_vtxp = concatp;
            iD.m_hi = jD.m_hi;
            jD = Driver{};

            // Consider next driver
            ++j;
        }
        // Rightsize vector
        drivers.resize(i + 1);
    }

    // Make a new splice with the given drivers
    Dfg_ಶೃಂಗ_Splice* makeSplice(Dfg_ಶೃಂಗ_Var& var, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Driver>& newDrivers) {
        UASSERT_OBJ(!newDrivers.ಖಾಲಿಗಳು(), &var, "'makeSplice' called with no new drivers");
        // Create new driver
        Dfg_ಶೃಂಗ_Splice* splicep = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (var.is<DfgVarPacked>()) {
            splicep = make<DfgSplicePacked>(var.fileline(), var.dtype());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (var.is<DfgVarArray>()) {
            splicep = make<DfgSpliceArray>(var.fileline(), var.dtype());
        } ಇಲ್ಲದಿದ್ದರೆ {
            var.v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Driver& d : newDrivers) splicep->ಕೂಡಿಸು_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        ಹಿಂತಿರುಗಿ splicep;
    }

    // Initialzie input symbol table of entry CfgBlock
    ಶೂನ್ಯ initialize_ನಮೂದು_ಚಿಹ್ನೆ_ಕೋಷ್ಟಕ(SymTab& iSymTab) {
        m_logicp->foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& src) {
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ vvp = src.as<Dfg_ಶೃಂಗ_Var>();
            iSymTab[vvp->vscp()] = vvp;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
    }

    // Join variable drivers across a control flow confluence (insert muxes ...)
    Dfg_ಶೃಂಗ_Var* joinDrivers(AstVarScope* vscp, Dfg_ಶೃಂಗ_Var* predicatep,  //
                              Dfg_ಶೃಂಗ_Var* thenp, Dfg_ಶೃಂಗ_Var* elsep) {
        AstVarScope* ಸ್ಥಿರಾಂಕ thenVscp = thenp->tmpForp() ? thenp->tmpForp() : thenp->vscp();
        AstVarScope* ಸ್ಥಿರಾಂಕ elseVscp = elsep->tmpForp() ? elsep->tmpForp() : elsep->vscp();
        UASSERT_OBJ(thenVscp == elseVscp, vscp, "Attempting to join unrelated variables");

        // If both bindings are the the same (variable not updated through either path),
        // then there is nothing to do, can use the same binding
        ಆದರೆ (thenp == elsep) ಹಿಂತಿರುಗಿ thenp;

        // We can't join the input variable just yet, so bail
        ಆದರೆ (thenp->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_ಒಳಹರಿವು;
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಆದರೆ (elsep->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_ಒಳಹರಿವು;
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }

        // Can't do arrays yet
        ಆದರೆ (!thenp->isPacked()) {
            ++m_ctx.m_synt.nonSynArray;
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }

        // Gather drivers of 'thenp' - only if 'thenp' is not an input to the synthesized block
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ tDefaultp = thenp->defaultp();
        std::ಸದಿಶ<Driver> tDrivers = gatherDrivers(thenp->srcp()->as<Dfg_ಶೃಂಗ_Splice>());

        // Gather drivers of 'elsep' - only if 'thenp' is not an input to the synthesized block
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ eDefaultp = elsep->defaultp();
        std::ಸದಿಶ<Driver> eDrivers = gatherDrivers(elsep->srcp()->as<Dfg_ಶೃಂಗ_Splice>());

        // Default drivers should be the same or not present on either
        UASSERT_OBJ(tDefaultp == eDefaultp, vscp, "Different default drivers");

        // Location to use for the join vertices
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = predicatep->fileline();

        // Create a fresh temporary for the joined value
        Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ joinp = m_converter.createTmp(*m_logicp, vscp, "SynthJoin");
        Dfg_ಶೃಂಗ_Splice* ಸ್ಥಿರಾಂಕ joinSplicep = make<DfgSplicePacked>(flp, joinp->dtype());
        joinp->srcp(joinSplicep);

        // If both paths are fully driven, just create a simple conditional
        ಆದರೆ (tDrivers.ಗಾತ್ರ_2() == 1  //
            && tDrivers[0].m_lo == 0  //
            && tDrivers[0].m_hi == thenp->ಅಗಲ() - 1  //
            && eDrivers.ಗಾತ್ರ_2() == 1  //
            && eDrivers[0].m_lo == 0  //
            && eDrivers[0].m_hi == elsep->ಅಗಲ() - 1) {
            UASSERT_OBJ(!tDefaultp, vscp, "Fully driven variable have default driver");

            DfgCond* ಸ್ಥಿರಾಂಕ condp = make<DfgCond>(flp, joinp->dtype());
            condp->condp(predicatep);
            condp->thenp(thenp);
            condp->elsep(elsep);
            joinSplicep->ಕೂಡಿಸು_Driver(condp, 0, tDrivers[0].m_flp);

            //ಮುಗಿದ
            ಹಿಂತಿರುಗಿ joinp;
        }

        // Otherwise we need to merge them part by part

        // If different bits are driven, then some might not have been assigned.. Latch?
        ಆದರೆ (tDrivers.ಗಾತ್ರ_2() != eDrivers.ಗಾತ್ರ_2()) {
            ++m_ctx.m_synt.nonSynLatch;
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }

        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < tDrivers.ಗಾತ್ರ_2(); ++i) {
            ಸ್ಥಿರಾಂಕ Driver& tDriver = tDrivers[i];
            ಸ್ಥಿರಾಂಕ Driver& eDriver = eDrivers[i];
            // If different bits are driven, then some might not have been assigned.. Latch?
            ಆದರೆ (tDriver.m_lo != eDriver.m_lo || tDriver.m_hi != eDriver.m_hi) {
                ++m_ctx.m_synt.nonSynLatch;
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            }

            ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype = Dfg_ದತ್ತಾಂಶ_Type::packed(tDriver.m_hi - tDriver.m_lo + 1);
            DfgCond* ಸ್ಥಿರಾಂಕ condp = make<DfgCond>(flp, dtype);
            condp->condp(predicatep);

            // We actally need to select the bits from the joined variables, not use the drivers
            DfgSel* ಸ್ಥಿರಾಂಕ thenSelp = make<DfgSel>(flp, tDriver.m_vtxp->dtype());
            thenSelp->lsb(tDriver.m_lo);
            thenSelp->fromp(thenp);
            condp->thenp(thenSelp);

            // Same for the 'else' part
            DfgSel* ಸ್ಥಿರಾಂಕ elseSelp = make<DfgSel>(flp, eDriver.m_vtxp->dtype());
            elseSelp->lsb(eDriver.m_lo);
            elseSelp->fromp(elsep);
            condp->elsep(elseSelp);

            // Add it as a driver to the join
            joinSplicep->ಕೂಡಿಸು_Driver(condp, tDriver.m_lo, tDriver.m_flp);
        }

        // If there was a default driver, add it to te join
        ಆದರೆ (tDefaultp) joinp->defaultp(tDefaultp);

        //ಮುಗಿದ
        ಹಿಂತಿರುಗಿ joinp;
    }

    // Merge 'thenSymTab' into 'elseSymTab' using the given predicate to join values
    ತಾರ್ಕಿಕ join_ಚಿಹ್ನೆ_Tables(SymTab& elseSymTab, Dfg_ಶೃಂಗ_Var* predicatep, ಸ್ಥಿರಾಂಕ SymTab& thenSymTab) {
        // Any variable that does not have a binding on both paths will be removed. These might be
        // temporaries, loop vars, etc used only in one branch. Conversion will fail if the
        // variable is actually referenced later.
        std::ಸದಿಶ<AstVarScope*> ಗಮ್ಯ_Remove;

        // Join each symbol
        ಗಾಗಿ (std::pair<AstVarScope* ಸ್ಥಿರಾಂಕ, Dfg_ಶೃಂಗ_Var*>& pair : elseSymTab) {
            AstVarScope* ಸ್ಥಿರಾಂಕ varp = pair.ಮೊದಲ;
            // Find same variable on the else path
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = thenSymTab.ಕಂಡುಹಿಡಿ(varp);
            // Record for removal if not assigned on both paths
            ಆದರೆ (it == thenSymTab.ಕೊನೆ()) {
                ಗಮ್ಯ_Remove.emplace_ಹಿಂಭಾಗ(varp);
                ಮುಂದುವರಿ;
            }
            // Join paths with the predicate
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ thenp = it->ಎರಡನೇ;
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ elsep = pair.ಎರಡನೇ;
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ newp = joinDrivers(varp, predicatep, thenp, elsep);
            ಆದರೆ (!newp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            pair.ಎರಡನೇ = newp;
        }

        // Remove variables not assigned on both paths
        ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ varp : ಗಮ್ಯ_Remove) elseSymTab.erase(varp);

        //ಮುಗಿದ
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Given two joining control flow edges, compute how to join their symbols.
    // Returns the predicaete to join over, and the 'then' and 'else' blocks.
    std::tuple<Dfg_ಶೃಂಗ_Var*, ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ*, ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ*>  //
    how_ಗಮ್ಯ_Join(ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು* ಸ್ಥಿರಾಂಕ ap, ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು* ಸ್ಥಿರಾಂಕ bp) {
        // Find the closest common dominator of the two paths
        ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ domp = m_dom_ವೃಕ್ಷ.closestCommonDominator(ap->srcp(), bp->srcp());
        // These paths join here, so 'domp' must be a branch, otherwise it's not the closest
        UASSERT_OBJ(domp->isBranch(), domp, "closestCommonDominator is not a branch");

        // The branches of the common dominator
        ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು* ಸ್ಥಿರಾಂಕ ತೆಗೆದುಕೊಂಡ_Edgep = domp->ತೆಗೆದುಕೊಂಡ_Edgep();
        ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು* ಸ್ಥಿರಾಂಕ untknEdgep = domp->untknEdgep();

        // We check if the taken branch dominates the path to either blocks,
        // and if the untaken branch dominates the path to the other block.
        // If so, we can use the branch condition as predicate, otherwise
        // we must use the path predicate as there are ways to get from one
        // branch of the dominator to the other. We need to be careful if
        // either branches are directly to the join block. This is fine,
        // it's as if there was an empty block on that critical edge which
        // is dominated by that path.

        ಆದರೆ (ತೆಗೆದುಕೊಂಡ_Edgep == ap || m_dom_ವೃಕ್ಷ.dominates(ತೆಗೆದುಕೊಂಡ_Edgep->dstp(), ap->srcp())) {
            ಆದರೆ (untknEdgep == bp || m_dom_ವೃಕ್ಷ.dominates(untknEdgep->dstp(), bp->srcp())) {
                // Taken path dominates 'ap' and untaken dominates 'bp', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                ಹಿಂತಿರುಗಿ std::make_tuple(m_bb_ಗಮ್ಯ_Condp[domp], ap->srcp(), bp->srcp());
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ತೆಗೆದುಕೊಂಡ_Edgep == bp || m_dom_ವೃಕ್ಷ.dominates(ತೆಗೆದುಕೊಂಡ_Edgep->dstp(), bp->srcp())) {
            ಆದರೆ (untknEdgep == ap || m_dom_ವೃಕ್ಷ.dominates(untknEdgep->dstp(), ap->srcp())) {
                // Taken path dominates 'bp' and untaken dominates 'ap', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                ಹಿಂತಿರುಗಿ std::make_tuple(m_bb_ಗಮ್ಯ_Condp[domp], bp->srcp(), ap->srcp());
            }
        }

        // The branches don't dominate the joined blocks, must use the path predicate
        ++m_ctx.m_synt.join_Using_ಮಾರ್ಗ_Predicate;

        // TODO: We could do better here: use the path predicate of the closest
        // cominating blocks, pick the one from the lower rank, etc, but this
        // generic case is very rare, most synthesizable logic has
        // series-parallel CFGs which are covered by the earlier cases.
        ಹಿಂತಿರುಗಿ std::make_tuple(m_ಅಂಚು_ಗಮ್ಯ_Predicatep[ap], ap->srcp(), bp->srcp());
    }

    // Combine the output symbol tables of the predecessors of the given
    // block to compute the input symtol table for the given block.
    ತಾರ್ಕಿಕ create_ಒಳಹರಿವು_ಚಿಹ್ನೆ_ಕೋಷ್ಟಕ(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& bb) {
        // The input symbol table of the given block, we are computing it now
        SymTab& joined = m_bb_ಗಮ್ಯ_ISym_Tab[bb];

        // Input symbol table of entry block is special
        ಆದರೆ (bb.isEnter()) {
            initialize_ನಮೂದು_ಚಿಹ್ನೆ_ಕೋಷ್ಟಕ(joined);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }

        // Current input symbol table should be empty, we will fill it in here
        UASSERT(joined.ಖಾಲಿಗಳು(), "Unprocessed input symbol table should be empty");

        // Fast path if there is only one predecessor - TODO: use less copying
        ಆದರೆ (!bb.isJoin()) {
            joined = m_bb_ಗಮ್ಯ_OSym_Tab[bb.ಮೊದಲ_Predecessorp()];
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }

        // We also have a simpler job if there are 2 predecessors
        ಆದರೆ (bb.isTwoWayJoin()) {
            Dfg_ಶೃಂಗ_Var* predicatep = ಶೂನ್ಯಸೂಚಕ;
            ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* thenp = ಶೂನ್ಯಸೂಚಕ;
            ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* elsep = ಶೂನ್ಯಸೂಚಕ;
            std::tie(predicatep, thenp, elsep)
                = how_ಗಮ್ಯ_Join(bb.ಮೊದಲ_Predecessor_Edgep(), bb.ಕೊನೆಯ_Predecessor_Edgep());
            // Copy from else
            joined = m_bb_ಗಮ್ಯ_OSym_Tab[elsep];
            // Join with then
            ಹಿಂತಿರುಗಿ join_ಚಿಹ್ನೆ_Tables(joined, predicatep, m_bb_ಗಮ್ಯ_OSym_Tab[*thenp]);
        }

        // General hard way

        // Gather predecessors
        ರಚನೆ Predecessor final {
            ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* m_bbp;  // Predeccessor block
            Dfg_ಶೃಂಗ_Var* m_predicatep;  // Predicate predecessor reached this block with
            ಸ್ಥಿರಾಂಕ SymTab* m_oSymTabp;  // Output symbol table or predecessor
            Predecessor() = ಅಳಿಸು;
            Predecessor(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* bbp, Dfg_ಶೃಂಗ_Var* predicatep, ಸ್ಥಿರಾಂಕ SymTab* oSymTabp)
                : m_bbp{bbp}
                , m_predicatep{predicatep}
                , m_oSymTabp{oSymTabp} {}
        };

        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Predecessor> predecessors = [&]() {
            std::ಸದಿಶ<Predecessor> res;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : bb.ಒಳ_ಅಂಚುಗಳು()) {
                ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು& cfg_ಅಂಚು = static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು&>(ಅಂಚು);
                ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ predecessorp = cfg_ಅಂಚು.srcp();
                Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ predicatep = m_ಅಂಚು_ಗಮ್ಯ_Predicatep[cfg_ಅಂಚು];
                ಸ್ಥಿರಾಂಕ SymTab* ಸ್ಥಿರಾಂಕ oSymTabp = &m_bb_ಗಮ್ಯ_OSym_Tab[predecessorp];
                res.emplace_ಹಿಂಭಾಗ(predecessorp, predicatep, oSymTabp);
            }
            // Sort predecessors reverse topologically. This way earlier blocks
            // will come after later blocks, and the entry block is last if present.
            std::ವಿಂಗಡಿಸು(res.begin(), res.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ Predecessor& a, ಸ್ಥಿರಾಂಕ Predecessor& b) {  //
                ಹಿಂತಿರುಗಿ *a.m_bbp > *b.m_bbp;
            });
            ಹಿಂತಿರುಗಿ res;
        }();

        // Start by copying the bindings from the frist predecessor
        joined = *predecessors[0].m_oSymTabp;
        // Join over all other predecessors
        ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < predecessors.ಗಾತ್ರ_2(); ++i) {
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ predicatep = predecessors[i].m_predicatep;
            ಸ್ಥಿರಾಂಕ SymTab& oSymTab = *predecessors[i].m_oSymTabp;
            ಆದರೆ (!join_ಚಿಹ್ನೆ_Tables(joined, predicatep, oSymTab)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    std::ಸದಿಶ<Driver> computePropagatedDrivers(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Driver>& newDrivers,
                                                 Dfg_ಶೃಂಗ_Var* oldp) {
        // Gather drivers of 'oldp' - they are in incresing range order with no overlaps
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<Dfg_ಶೃಂಗ_Splice>());
        UASSERT_OBJ(!oldDrivers.ಖಾಲಿಗಳು(), oldp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::ಸದಿಶ<Driver> propagatedDrivers;

        // Add bits between 'msb' and 'lsb' from 'oldp' to 'pDrivers'
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_Old_Driver = [&](File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp, uint32_t msb, uint32_t lsb) {
            UASSERT_OBJ(propagatedDrivers.ಖಾಲಿಗಳು() || lsb > propagatedDrivers.ಹಿಂಭಾಗ().m_hi, flp,
                        "Drivers should be in ascending order");
            DfgSel* ಸ್ಥಿರಾಂಕ selp = make<DfgSel>(flp, Dfg_ದತ್ತಾಂಶ_Type::packed(msb - lsb + 1));
            selp->lsb(lsb);
            selp->fromp(oldp);
            propagatedDrivers.emplace_ಹಿಂಭಾಗ(selp, lsb, flp);
        };

        // Incorporate old drivers
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Driver& oDriver : oldDrivers) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = oDriver.m_flp;
            // Range to consider inserting, we will adjust oldLo as we process drivers
            uint32_t oldLo = oDriver.m_lo;
            ಸ್ಥಿರಾಂಕ uint32_t oldHi = oDriver.m_hi;

            // Loop for now, can move to bisection search if this is a problem, shouldn't be ...
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Driver& nDriver : newDrivers) {
                UASSERT_OBJ(oldHi >= oldLo, flp, "Should have stopped iteration");
                // If new driver is entirely below old driver, move on to
                ಆದರೆ (nDriver.m_hi < oldLo) ಮುಂದುವರಿ;
                // If new driver is entirely above old driver, we can stop
                ಆದರೆ (oldHi < nDriver.m_lo) ನಿಲ್ಲಿಸು;

                // There is an overlap between 'oDriver' and 'nDriver'.
                // Insert the low bits and adjust the insertion range.
                // The rest will take care of itself on subsequent iterations.
                ಆದರೆ (oldLo < nDriver.m_lo) ಕೂಡಿಸು_Old_Driver(flp, nDriver.m_lo - 1, oldLo);
                oldLo = nDriver.m_hi + 1;

                // Stop if no more bits remaining in the old driver
                ಆದರೆ (oldLo > oldHi) ನಿಲ್ಲಿಸು;
            }

            // Insert remaining bits if any
            ಆದರೆ (oldHi >= oldLo) ಕೂಡಿಸು_Old_Driver(flp, oldHi, oldLo);
        }

        ಹಿಂತಿರುಗಿ propagatedDrivers;
    }

    // Propagate whole elements with a linear walk over the sorted driver lists.
    // Reads use the previous value temporary, preserving assignment-version bindings.
    // Partial elements and non-unit drivers retain the nonsynthesized-process fallback.
    ತಾರ್ಕಿಕ computePropagatedArrayDrivers(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Driver>& newDrivers,
                                       Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ oldp,
                                       std::ಸದಿಶ<Driver>& propagatedDrivers) {
        // Bound quadratic vertex growth: array drivers cannot be coalesced.
        ಸ್ಥಿರ constexpr uint32_t MAX_ARRAY_ELEMENTS = 32;
        ಆದರೆ (oldp->ಗಾತ್ರ_2() > MAX_ARRAY_ELEMENTS) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Driver& driver : newDrivers) {
            ಆದರೆ (!driver_Covers_ಸಂಪೂರ್ಣ_Element(driver)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<Dfg_ಶೃಂಗ_Splice>());
        UASSERT_OBJ(!oldDrivers.ಖಾಲಿಗಳು(), oldp, "Should have a proper driver");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Driver& driver : oldDrivers) {
            ಆದರೆ (!driver_Covers_ಸಂಪೂರ್ಣ_Element(driver)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        propagatedDrivers.reserve(oldDrivers.ಗಾತ್ರ_2());
        ಸ್ವಯಂಚಾಲಿತ nIt = newDrivers.begin();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Driver& oDriver : oldDrivers) {
            ಆಗ (nIt != newDrivers.ಕೊನೆ() && nIt->m_lo < oDriver.m_lo) ++nIt;
            ಆದರೆ (nIt != newDrivers.ಕೊನೆ() && nIt->m_lo == oDriver.m_lo) ಮುಂದುವರಿ;

            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = oDriver.m_flp;
            ಸ್ಥಿರಾಂಕ DfgUnitArray* ಸ್ಥಿರಾಂಕ oldUnitp = oDriver.m_vtxp->as<DfgUnitArray>();
            DfgArraySel* ಸ್ಥಿರಾಂಕ selp = make<DfgArraySel>(flp, oldUnitp->srcp()->dtype());
            selp->fromp(oldp);
            selp->bitp(make<DfgConst>(flp, static_cast<ಗಾತ್ರ_t>(VL_IDATASIZE), oDriver.m_lo));
            DfgUnitArray* ಸ್ಥಿರಾಂಕ newUnitp = make<DfgUnitArray>(flp, oldUnitp->dtype());
            newUnitp->srcp(selp);
            propagatedDrivers.emplace_ಹಿಂಭಾಗ(newUnitp, oDriver.m_lo, flp);
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Given the drivers of a variable after converting a single statement
    // 'newp', add drivers from 'oldp' that were not reassigned be drivers
    // in newp. This computes the total result of all previous assignments.
    ತಾರ್ಕಿಕ incorporate_ಹಿಂದಿನ_ಮೌಲ್ಯ(AstVarScope* vscp, Dfg_ಶೃಂಗ_Var* newp, Dfg_ಶೃಂಗ_Var* oldp) {
        UASSERT_OBJ(newp->srcp(), vscp, "Assigned variable has no driver");

        // Easy if there is no old value...
        ಆದರೆ (!oldp) ಹಿಂತಿರುಗಿ ನಿಜ;

        // New driver was not yet coalesced, so should always be a splice
        Dfg_ಶೃಂಗ_Splice* ಸ್ಥಿರಾಂಕ nSplicep = newp->srcp()->as<Dfg_ಶೃಂಗ_Splice>();

        // If the old value is the real variable we just computed the new value for,
        // then it is the circular feedback into the synthesized block, add it as default driver.
        ಆದರೆ (oldp->vscp() == vscp) {
            ಆದರೆ (!splice_Covers_ಸಂಪೂರ್ಣ(nSplicep)) newp->defaultp(oldp);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }

        UASSERT_OBJ(oldp->srcp(), vscp, "Previously assigned variable has no driver");

        // Gather drivers of 'newp' - they are in incresing range order with no overlaps
        UASSERT_OBJ(!newp->defaultp(), newp, "Converted value should not have default");
        std::ಸದಿಶ<Driver> nDrivers = gatherDrivers(newp->srcp()->as<Dfg_ಶೃಂಗ_Splice>());
        UASSERT_OBJ(!nDrivers.ಖಾಲಿಗಳು(), newp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::ಸದಿಶ<Driver> pDrivers;
        ಆದರೆ (newp->isPacked()) {
            pDrivers = computePropagatedDrivers(nDrivers, oldp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!computePropagatedArrayDrivers(nDrivers, oldp, pDrivers)) {
            ++m_ctx.m_synt.nonSynArray;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        ಆದರೆ (!pDrivers.ಖಾಲಿಗಳು()) {
            // Need to merge propagated sources, so reset the splice
            nSplicep->resetDrivers();
            // Merge drivers - they are both sorted and non-overlapping
            std::ಸದಿಶ<Driver> drivers;
            drivers.reserve(nDrivers.ಗಾತ್ರ_2() + pDrivers.ಗಾತ್ರ_2());
            std::ವಿಲೀನ(nDrivers.begin(), nDrivers.ಕೊನೆ(), pDrivers.begin(), pDrivers.ಕೊನೆ(),
                       std::ಹಿಂಭಾಗ_inserter(drivers));
            // Coalesce adjacent ranges
            ಆದರೆ (newp->isPacked()) coalesceDrivers(drivers);
            // Reinsert drivers in order
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Driver& d : drivers) nSplicep->ಕೂಡಿಸು_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        }

        // If the old had a default, add to the new one too, unless redundant
        ಆದರೆ (oldp->defaultp() && !splice_Covers_ಸಂಪೂರ್ಣ(nSplicep)) newp->defaultp(oldp->defaultp());

        //ಮುಗಿದ
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Synthesize the given statements with the given input symbol table.
    // Returns true if successfolly synthesized.
    // Populates the given output symbol table.
    // Populates the given reference with the condition of the terminator branch, if any.
    ತಾರ್ಕಿಕ synthesize_Basic_ಖಂಡ(SymTab& oSymTab, Dfg_ಶೃಂಗ*& condpr,
                              ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_ಗಂಟು_Stmt*>& stmtps, ಸ್ಥಿರಾಂಕ SymTab& iSymTab) {
        // Use fresh set of vertices in m_converter
        ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;

        // Initialize AstVarScope -> Vertex bindings available in this block
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : iSymTab) {
            AstVarScope* ಸ್ಥಿರಾಂಕ varp = pair.ಮೊದಲ;
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ vtxp = pair.ಎರಡನೇ;
            varp->user2p(vtxp);
            oSymTab[varp] = vtxp;
        }

        // Synthesize each statement one after the other
        std::ಸದಿಶ<std::pair<AstVarScope*, Dfg_ಶೃಂಗ_Var*>> updates;
        ಗಾಗಿ (Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtp : stmtps) {
            // Regular statements
            Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ ap = VN_CAST(stmtp, ಗಂಟು_ನಿಯೋಜಿಸು);
            ಆದರೆ (ap && (VN_IS(ap, ನಿಯೋಜಿಸು) || VN_IS(ap, ನಿಯೋಜಿಸು_W))) {
                // Convert this assignment
                ಆದರೆ (!m_converter.convert(updates, *m_logicp, ap)) {
                    ++m_ctx.m_synt.nonSynConv;
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }
                // Apply variable updates from this statement
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : updates) {
                    // The target variable that was assigned to
                    AstVarScope* ಸ್ಥಿರಾಂಕ vscp = pair.ಮೊದಲ;
                    // The new, potentially partially assigned value
                    Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ newp = pair.ಎರಡನೇ;
                    // Normalize drivers within this statement, bail if multidriven
                    Dfg_ಶೃಂಗ_Splice* ಸ್ಥಿರಾಂಕ srcp = newp->srcp()->as<Dfg_ಶೃಂಗ_Splice>();
                    std::ಸದಿಶ<Driver> drivers = gatherDrivers(srcp);
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ single = drivers.ಗಾತ್ರ_2() == 1;
                    ಆದರೆ (!normalizeDrivers(*newp, drivers)) {
                        ++m_ctx.m_synt.nonSynMultidrive;
                        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                    }
                    // If there were more than one driver (often not), re-add in case coalesced
                    ಆದರೆ (!single) {
                        srcp->resetDrivers();
                        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Driver& d : drivers) srcp->ಕೂಡಿಸು_Driver(d.m_vtxp, d.m_lo, d.m_flp);
                    }
                    // The old value, if any
                    Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ oldp = vscp->user2u().ಮಾದರಿ ಗಮ್ಯ<Dfg_ಶೃಂಗ_Var*>();
                    // Inncorporate old value into the new value
                    ಆದರೆ (!incorporate_ಹಿಂದಿನ_ಮೌಲ್ಯ(vscp, newp, oldp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                    // Update binding of target variable
                    vscp->user2p(newp);
                    // Update output symbol table of this block
                    oSymTab[vscp] = newp;
                }
                updates.clear();
                ಮುಂದುವರಿ;
            }

            // Terminator branches
            ಆದರೆ (AstIf* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(stmtp, If)) {
                UASSERT_OBJ(ifp == stmtps.ಹಿಂಭಾಗ(), ifp, "Branch should be last statement");
                // Convert condition, give up if failed
                Dfg_ಶೃಂಗ* condp = m_converter.convert(*m_logicp, ifp->condp());
                ಆದರೆ (!condp) {
                    ++m_ctx.m_synt.nonSynConv;
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }
                // Single bit condition can be use directly, otherwise: use 'condp != 0'
                ಆದರೆ (condp->ಅಗಲ() != 1) {
                    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = condp->fileline();
                    DfgNeq* ಸ್ಥಿರಾಂಕ neqp = make<DfgNeq>(flp, Dfg_ದತ್ತಾಂಶ_Type::packed(1));
                    neqp->lhsp(make<DfgConst>(flp, condp->ಅಗಲ(), 0U));
                    neqp->rhsp(condp);
                    condp = neqp;
                }
                condpr = condp;
                ಮುಂದುವರಿ;
            }

            // Unhandled
            ++m_ctx.m_synt.nonSynStmt;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Assign path predicates to the outgoing control flow edges of the given block
    ಶೂನ್ಯ ನಿಯೋಜಿಸು_ಮಾರ್ಗ_Predicates(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& bb) {
        // Nothing to do for the exit block
        ಆದರೆ (bb.isExit()) ಹಿಂತಿರುಗಿ;

        // Get the predicate of this block
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ predp = [&]() -> Dfg_ಶೃಂಗ* {
            // Entry block has no predecessors, use constant true
            ಆದರೆ (bb.isEnter()) ಹಿಂತಿರುಗಿ make<DfgConst>(m_logicp->fileline(), 1U, 1U);

            // For any other block, 'or' together all the incoming predicates
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಒಳ_ಅಂಚುಗಳು = bb.ಒಳ_ಅಂಚುಗಳು();
            ಸ್ವಯಂಚಾಲಿತ it = ಒಳ_ಅಂಚುಗಳು.begin();
            Dfg_ಶೃಂಗ* resp = m_ಅಂಚು_ಗಮ್ಯ_Predicatep[static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು&>(*it)];
            ಆಗ (++it != ಒಳ_ಅಂಚುಗಳು.ಕೊನೆ()) {
                DfgOr* ಸ್ಥಿರಾಂಕ orp = make<DfgOr>(resp->fileline(), resp->dtype());
                orp->rhsp(resp);
                orp->lhsp(m_ಅಂಚು_ಗಮ್ಯ_Predicatep[static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು&>(*it)]);
                resp = orp;
            }
            ಹಿಂತಿರುಗಿ resp;
        }();

        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype = predp->dtype();

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ mkTmp = [&](File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* name, Dfg_ಶೃಂಗ* srcp) {
            ಸ್ಥಿರಾಂಕ std::string ಪೂರ್ವಪ್ರತ್ಯಯ = "_BB" + std::ಗಮ್ಯ_string(bb.id()) + "_" + name;
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ tmpp = m_converter.createTmp(*m_logicp, flp, dtype, ಪೂರ್ವಪ್ರತ್ಯಯ);
            tmpp->srcp(srcp);
            ಹಿಂತಿರುಗಿ tmpp;
        };

        // Assign it to a variable in case it's used multiple times
        Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ pInp = mkTmp(predp->fileline(), "PathIn", predp);

        // For uncondional branches, the successor predicate edge is the same
        ಆದರೆ (!bb.isBranch()) {
            m_ಅಂಚು_ಗಮ್ಯ_Predicatep[bb.ತೆಗೆದುಕೊಂಡ_Edgep()] = mkTmp(pInp->fileline(), "Goto", pInp);
            ಹಿಂತಿರುಗಿ;
        }

        // For branches, we need to factor in the branch condition
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ condp = m_bb_ಗಮ್ಯ_Condp[bb];
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = condp->fileline();

        // Predicate for taken branch: 'predp & condp'
        {
            DfgAnd* ಸ್ಥಿರಾಂಕ ತೆಗೆದುಕೊಂಡ_Predp = make<DfgAnd>(flp, dtype);
            ತೆಗೆದುಕೊಂಡ_Predp->lhsp(pInp);
            ತೆಗೆದುಕೊಂಡ_Predp->rhsp(condp);
            m_ಅಂಚು_ಗಮ್ಯ_Predicatep[bb.ತೆಗೆದುಕೊಂಡ_Edgep()] = mkTmp(flp, "Taken", ತೆಗೆದುಕೊಂಡ_Predp);
        }

        // Predicate for untaken branch: 'predp & ~condp'
        {
            DfgAnd* ಸ್ಥಿರಾಂಕ untknPredp = make<DfgAnd>(flp, dtype);
            untknPredp->lhsp(pInp);
            DfgNot* ಸ್ಥಿರಾಂಕ notp = make<DfgNot>(flp, dtype);
            notp->srcp(condp);
            untknPredp->rhsp(notp);
            m_ಅಂಚು_ಗಮ್ಯ_Predicatep[bb.untknEdgep()] = mkTmp(flp, "Untkn", untknPredp);
        }
    }

    // Returns true if all external updates to volatile variables are observed correctly
    ತಾರ್ಕಿಕ checkExtWrites() {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_logicp->synth()) {
            ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = vtxp->cast<Dfg_ಶೃಂಗ_Var>();
            ಆದರೆ (!varp) ಮುಂದುವರಿ;
            // If the variable we synthesized this vertex for is volatile, and
            // the value of the synthesized temporary is observed, we might be
            // missing an external update, so we mut give up.
            ಆದರೆ (!varp->hasSinks()) ಮುಂದುವರಿ;
            ಆದರೆ (!Dfg_ಶೃಂಗ_Var::isVolatile(varp->tmpForp())) ಮುಂದುವರಿ;
            ++m_ctx.m_synt.nonSynExtWrite;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Add the synthesized values as drivers to the output variables of the current DfgLogic
    ತಾರ್ಕಿಕ ಕೂಡಿಸು_Synthesized_Output(SymTab& oSymTab) {
        // It's possible we think a variable is written by the DfgLogic when
        // it actauly isn't, e.g.: '{a[0], b[0]}[1] = ...' does not write 'b'.
        // These LHS forms can happen after some earlier tranforms. We
        // should just run V3Const on them earlier, but we will do belt and
        // braces and check here too. We can't touch any output variables if so.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ missing = m_logicp->foreachSink([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& sink) {
            ಸ್ಥಿರಾಂಕ DfgUnresolved* ಸ್ಥಿರಾಂಕ unresolvedp = sink.as<DfgUnresolved>();
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = unresolvedp->singleSink()->as<Dfg_ಶೃಂಗ_Var>()->vscp();
            ಹಿಂತಿರುಗಿ !oSymTab.ಎಣಿಕೆ_2(vscp);
        });
        ಆದರೆ (missing) {
            ++m_ctx.m_synt.nonSynFalseWrite;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        // Add sinks to read the computed values for the target variables
        ಹಿಂತಿರುಗಿ !m_logicp->foreachSink([&](Dfg_ಶೃಂಗ& sink) {
            DfgUnresolved* ಸ್ಥಿರಾಂಕ unresolvedp = sink.as<DfgUnresolved>();
            ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = unresolvedp->singleSink()->as<Dfg_ಶೃಂಗ_Var>();
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ resp = oSymTab.at(varp->vscp());
            UASSERT_OBJ(resp->srcp(), resp, "Undriven result");

            // If the output is not used further in the synthesized logic itself,
            // then resp will be deleted before we return, so we can just use
            // its splice directly without ending up with a multi-use operation.
            ಆದರೆ (!resp->hasSinks()) {
                unresolvedp->ಕೂಡಿಸು_Driver(resp->srcp()->as<Dfg_ಶೃಂಗ_Splice>());
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // OK, continue.
            }

            // TODO: computePropagatedDrivers cannot handle arrays, should
            // never happen with simple continous assignments
            ಆದರೆ (!resp->isPacked()) {
                ++m_ctx.m_synt.nonSynArray;
                ಹಿಂತಿರುಗಿ ನಿಜ;  // Not OK, give up
            }

            // We need to add a new splice to avoid multi-use of the original splice
            DfgSplicePacked* ಸ್ಥಿರಾಂಕ splicep
                = ಹೊಸ DfgSplicePacked{m_dfg, resp->fileline(), resp->dtype()};
            // Drivers are the same
            ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Driver> drivers = computePropagatedDrivers({}, resp);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Driver& d : drivers) splicep->ಕೂಡಿಸು_Driver(d.m_vtxp, d.m_lo, d.m_flp);
            unresolvedp->ಕೂಡಿಸು_Driver(splicep);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // OK, continue
        });
    }

    // Synthesize the given AstAssignW. Returns true on success.
    ತಾರ್ಕಿಕ synthesize_ನಿಯೋಜಿಸು_W(Ast_ನಿಯೋಜಿಸು_W* nodep) {
        ++m_ctx.m_synt.ಒಳಹರಿವು_ನಿಯೋಜಿಸು;

        // The input and output symbol tables
        SymTab iSymTab;
        SymTab oSymTab;

        // Initialzie input symbol table
        initialize_ನಮೂದು_ಚಿಹ್ನೆ_ಕೋಷ್ಟಕ(iSymTab);

        // Synthesize as if it was in a single CfgBlock CFG
        Dfg_ಶೃಂಗ* condp = ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ success = synthesize_Basic_ಖಂಡ(oSymTab, condp, {nodep}, iSymTab);
        UASSERT_OBJ(!condp, nodep, "Conditional AstAssignW ???");
        ಆದರೆ (!success) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // Check exernal writes are observed correctly
        ಆದರೆ (!checkExtWrites()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // Add resolved output variable drivers
        ಹಿಂತಿರುಗಿ ಕೂಡಿಸು_Synthesized_Output(oSymTab);
    }

    // Synthesize the given AstAlways. Returns true on success.
    ತಾರ್ಕಿಕ synthesizeCfg(Cfg_ನಕ್ಷೆ& cfg) {
        ++m_ctx.m_synt.ಒಳಹರಿವು_Always;

        // If there is a backward edge (loop), we can't synthesize it
        ಆದರೆ (cfg.ಒಳಗೊಂಡಿದೆ_Loop()) {
            ++m_ctx.m_synt.nonSynLoop;
            ++m_ctx.m_synt.cfgCyclic;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        // If it's a trivial CFG we can save on some work
        ಆದರೆ (cfg.n_ಖಂಡಗಳು() == 1) {
            ++m_ctx.m_synt.cfgTrivial;
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Insert two-way join blocks to aid multiplexer ordering
            ಆದರೆ (cfg.ಸೇರಿಸು_Two_Way_Joins()) {
                ++m_ctx.m_synt.cfgSp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ++m_ctx.m_synt.cfgDag;
            }
            // Initialize maps needed for non-trivial CFGs
            m_dom_ವೃಕ್ಷ = Cfg_Dominator_ವೃಕ್ಷ{cfg};
            m_ಅಂಚು_ಗಮ್ಯ_Predicatep = cfg.make_ಅಂಚು_Map<Dfg_ಶೃಂಗ_Var*>();
        }

        // Initialize CfgMaps
        m_bb_ಗಮ್ಯ_ISym_Tab = cfg.make_ಖಂಡ_Map<SymTab>();
        m_bb_ಗಮ್ಯ_OSym_Tab = cfg.make_ಖಂಡ_Map<SymTab>();
        m_bb_ಗಮ್ಯ_Condp = cfg.make_ಖಂಡ_Map<Dfg_ಶೃಂಗ_Var*>();

        // Synthesize all blocks
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : cfg.vertices()) {
            ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& bb = static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ&>(vtx);
            // Prepare the input symbol table of this block (enter, or join predecessor blocks)
            ಆದರೆ (!create_ಒಳಹರಿವು_ಚಿಹ್ನೆ_ಕೋಷ್ಟಕ(bb)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Synthesize this block
            Dfg_ಶೃಂಗ* condp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (!synthesize_Basic_ಖಂಡ(m_bb_ಗಮ್ಯ_OSym_Tab[bb], condp, bb.stmtps(), m_bb_ಗಮ್ಯ_ISym_Tab[bb])) {
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            // Create a temporary for the branch condition as it might be used multiple times
            ಆದರೆ (condp) {
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = condp->fileline();
                ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype = condp->dtype();
                ಸ್ಥಿರಾಂಕ std::string ಪೂರ್ವಪ್ರತ್ಯಯ = "_BB" + std::ಗಮ್ಯ_string(bb.id()) + "_Cond";
                Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ vp = m_converter.createTmp(*m_logicp, flp, dtype, ಪೂರ್ವಪ್ರತ್ಯಯ);
                vp->srcp(condp);
                m_bb_ಗಮ್ಯ_Condp[bb] = vp;
            }
            // Set the path predicates on the successor edges
            ನಿಯೋಜಿಸು_ಮಾರ್ಗ_Predicates(bb);
        }

        // Check exernal writes are observed correctly
        ಆದರೆ (!checkExtWrites()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // Add resolved output variable drivers
        ಹಿಂತಿರುಗಿ ಕೂಡಿಸು_Synthesized_Output(m_bb_ಗಮ್ಯ_OSym_Tab[cfg.ನಿರ್ಗಮಿಸು()]);
    }

    // Synthesize a DfgLogic into regular vertices. Returns ture on success.
    ತಾರ್ಕಿಕ synthesize(DfgLogic& vtx) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;

        ಆದರೆ (Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ ap = VN_CAST(vtx.nodep()->stmtsp(), ನಿಯೋಜಿಸು_W)) {
            ಆದರೆ (ap->nextp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (!synthesize_ನಿಯೋಜಿಸು_W(ap)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ++m_ctx.m_synt.synth_ನಿಯೋಜಿಸು;
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }

        ಆದರೆ (!synthesizeCfg(vtx.cfg())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ++m_ctx.m_synt.synthAlways;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Revert all DfgLogic in m_toRevert, or DfgLogic driving the DfgUnresolved
    // vertices in m_toRevert, and transitively the same for any DfgUnresolved
    // driven by the reverted DfgLogic. Delete all DfgUnresolved involed.
    ಶೂನ್ಯ revert(VDouble0& statCountr) {
        m_ಗಮ್ಯ_Revert.foreach([&](Dfg_ಶೃಂಗ& vtx) {
            // Process DfgLogic
            ಆದರೆ (DfgLogic* ಸ್ಥಿರಾಂಕ vtxp = vtx.cast<DfgLogic>()) {
                UASSERT_OBJ(vtxp->selectedForSynthesis(), vtxp, "Shouldn't reach here unselected");
                // Revert this logic
                UASSERT_OBJ(!vtxp->reverted(), vtxp, "Should be reverting now");
                ++statCountr;
                ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ p : vtxp->synth()) VL_DO_DANGLING(p->unlinkDelete(m_dfg), p);
                vtxp->synth().clear();
                vtxp->ಗಣ_Reverted();
                // Add all DfgUnresolved it drives to the work list
                vtxp->foreachSink([&](Dfg_ಶೃಂಗ& snk) {
                    m_ಗಮ್ಯ_Revert.ತಳ್ಳು_front(*snk.as<DfgUnresolved>());
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                });
                ಹಿಂತಿರುಗಿ;
            }

            // Process DfgUnresolved
            ಆದರೆ (DfgUnresolved* ಸ್ಥಿರಾಂಕ vtxp = vtx.cast<DfgUnresolved>()) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<Dfg_ಶೃಂಗ_Var>()->ಗಣ_Has_Mod_Wr_Refs();
                // Add all driving DfgLogic to the work list
                vtxp->foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& src) {
                    DfgLogic* ಸ್ಥಿರಾಂಕ lp = src.cast<DfgLogic>();
                    ಆದರೆ (lp && !lp->reverted()) m_ಗಮ್ಯ_Revert.ತಳ್ಳು_front(*lp);
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                });
                // Delete the DfgUnresolved driver
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
                ಹಿಂತಿರುಗಿ;
            }

            // There should be nothing else on the worklist
            vtx.v3fatalSrc("Unexpected vertex type");
        });
    }

    // Synthesize all of the given vertices
    ಶೂನ್ಯ ಮುಖ್ಯ() {
        //-------------------------------------------------------------------
        UINFO(5, "Step 0: Remove all DfgLogic not selected for synthesis");
        ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgUnresolved
            ಆದರೆ (!vtxp->is<DfgUnresolved>()) ಮುಂದುವರಿ;
            ತಾರ್ಕಿಕ anySelected = ಸುಳ್ಳು;
            ತಾರ್ಕಿಕ anyUnselected = ಸುಳ್ಳು;
            vtxp->foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& src) {
                ಸ್ಥಿರಾಂಕ DfgLogic& logic = *src.as<DfgLogic>();
                ಆದರೆ (logic.selectedForSynthesis()) {
                    anySelected = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    anyUnselected = ನಿಜ;
                }
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            // There should be a driver
            UASSERT_OBJ(anySelected || anyUnselected, vtxp, "'DfgUnresolved' with no driver");
            // All drivers should be selected or all should be unselected
            UASSERT_OBJ(!(anySelected && anyUnselected), vtxp, "Invalid 'DfgLogic' selection");
            // If all drivers are unselected, delete this DfgUnresolved here
            ಆದರೆ (anyUnselected) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<Dfg_ಶೃಂಗ_Var>()->ಗಣ_Has_Mod_Wr_Refs();
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
        ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgLogic
            DfgLogic* ಸ್ಥಿರಾಂಕ logicp = vtxp->cast<DfgLogic>();
            ಆದರೆ (!logicp) ಮುಂದುವರಿ;
            ಆದರೆ (logicp->selectedForSynthesis()) ಮುಂದುವರಿ;
            // There should be no sinks left for unselected DfgLogic, delete them here
            UASSERT_OBJ(!logicp->hasSinks(), vtxp, "Unselected 'DfgLogic' with sinks remaining");
            // Input variables will be read in Ast code, add Ast reference vertices
            // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
            ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use m_user4_ಒಳ_Use;
            logicp->foreach_ಮೂಲ([](Dfg_ಶೃಂಗ& src) {
                src.as<Dfg_ಶೃಂಗ_Var>()->vscp()->user4p(&src);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            V3DfgPasses::ಕೂಡಿಸು_Ast_Refs(m_dfg, logicp->nodep(), [](Ast_ಗಂಟು* varp) {  //
                ಹಿಂತಿರುಗಿ varp->user4u().ಗಮ್ಯ<Dfg_ಶೃಂಗ_Var*>();
            });
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        debugDump("synth-selected");

        //-------------------------------------------------------------------
        UINFO(5, "Step 1: Attempting to synthesize each of the selected DfgLogic");
        {
            // AstVarScope::user4() -> int: unique ID for 'AstVarScopeComparator'
            ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use user4_ಒಳ_Use;
            ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : m_dfg.opVertices()) {
                DfgLogic* ಸ್ಥಿರಾಂಕ logicp = vtx.cast<DfgLogic>();
                ಆದರೆ (!logicp) ಮುಂದುವರಿ;

                // We should only have DfgLogic remaining that was selected for synthesis
                UASSERT_OBJ(logicp->selectedForSynthesis(), logicp, "Unselected DfgLogic remains");

                // Debug aid
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ debugCallback = [&]() -> ಶೂನ್ಯ {
                    // This is the breaking logic
                    m_debugLogicp = logicp;
                    // Dump it
                    UINFOTREE(0, logicp->nodep(), "Problematic DfgLogic: " << logicp, "  ");
                    V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(logicp->nodep(), std::ಹೊರಗೆ);
                    debugDump("synth-lastok");
                };
                ಆದರೆ (VL_UNLIKELY(s_dfgSynthDebugBisect.stop(debugCallback))) ನಿಲ್ಲಿಸು;

                // Synthesize it, if failed, enqueue for reversion
                ಆದರೆ (!synthesize(*logicp)) {
                    logicp->ಗಣ_Non_Synthesizable();
                    m_ಗಮ್ಯ_Revert.ತಳ್ಳು_front(*logicp);
                }
            }
        }
        debugDump("synth-converted");

        //-------------------------------------------------------------------
        UINFO(5, "Step 2: Revert drivers of variables with unsynthesizeable drivers");
        // We do this as the variables might be multi-driven, we can't know if synthesis failed
        revert(m_ctx.m_synt.revertNonSyn);
        debugDump("synth-reverted");

        //-------------------------------------------------------------------
        UINFO(5, "Step 3: Resolve synthesized drivers of original (non-temporary) variables");
        // List of multi-driven variables
        std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*> multidrivenps;
        // Map from variable to its resolved driver
        std::unordered_map<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var*, Dfg_ಶೃಂಗ_Splice*> resolvedDrivers;
        // Compute resolved drivers of all variablees
        ಗಾಗಿ (Dfg_ಶೃಂಗ_Var& var : m_dfg.varVertices()) {
            ಆದರೆ (!var.srcp()) ಮುಂದುವರಿ;
            DfgUnresolved* ಸ್ಥಿರಾಂಕ unresolvedp = var.srcp()->cast<DfgUnresolved>();
            ಆದರೆ (!unresolvedp) ನಿಲ್ಲಿಸು;  // Stop when reached the synthesized temporaries

            // Resolve the synthesized drivers
            Dfg_ಶೃಂಗ_Splice* ಸ್ಥಿರಾಂಕ resolvedp = [&]() -> Dfg_ಶೃಂಗ_Splice* {
                // All synthesized drivers were normalized already,
                // so if there is only one, it can be used directly
                ಆದರೆ (unresolvedp->nInputs() == 1) {
                    ಹಿಂತಿರುಗಿ unresolvedp->inputp(0)->as<Dfg_ಶೃಂಗ_Splice>();
                }
                // Otherwise gather the synthesized drivers
                std::ಸದಿಶ<Driver> drivers = gatherDriversUnresolved(unresolvedp);
                // Normalize them, make resolved driver if all good
                ಆದರೆ (normalizeDrivers(var, drivers)) ಹಿಂತಿರುಗಿ makeSplice(var, drivers);
                // If mutlidriven, record and ignore
                multidrivenps.emplace_ಹಿಂಭಾಗ(&var);
                m_ಗಮ್ಯ_Revert.ತಳ್ಳು_front(*unresolvedp);
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            }();
            // Bail if multidriven
            ಆದರೆ (!resolvedp) ಮುಂದುವರಿ;
            // Add to map for next loop
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ new_ನಮೂದು = resolvedDrivers.emplace(&var, resolvedp).ಎರಡನೇ;
            UASSERT_OBJ(new_ನಮೂದು, &var, "Duplicate driver");
        }
        // Revert and remove drivers of multi-driven variables
        revert(m_ctx.m_synt.revertMultidrive);
        // Replace all DfgUnresolved with the resolved drivers
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var& var : m_dfg.varVertices()) {
            ಆದರೆ (!var.srcp()) ಮುಂದುವರಿ;
            DfgUnresolved* ಸ್ಥಿರಾಂಕ srcp = var.srcp()->cast<DfgUnresolved>();
            ಆದರೆ (!srcp) ನಿಲ್ಲಿಸು;  // Stop when reached the synthesized temporaries

            // Replace it
            srcp->replaceWith(resolvedDrivers.at(&var));
            VL_DO_DANGLING(srcp->unlinkDelete(m_dfg), srcp);
        }
        debugDump("synth-resolved");

        //-------------------------------------------------------------------
        UINFO(5, "Step 4: Remove all remaining DfgLogic and DfgUnresolved");
        ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_dfg.opVertices().unlinkable()) {
            // Previous step should have removed all DfgUnresolved
            UASSERT_OBJ(!vtxp->is<DfgUnresolved>(), vtxp, "DfgUnresolved remains");

            // Process only DfgLogic
            DfgLogic* ಸ್ಥಿರಾಂಕ logicp = vtxp->cast<DfgLogic>();
            ಆದರೆ (!logicp) ಮುಂದುವರಿ;

            // Earlier pass should have removed all sinks
            UASSERT_OBJ(!logicp->hasSinks(), logicp, "DfgLogic sink remains");

            ಆದರೆ (!logicp->synth().ಖಾಲಿಗಳು()) {
                // If synthesized, delete the corresponding AstNode. It is now in Dfg.
                logicp->nodep()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Not synthesized. Logic stays in Ast. Add Ast reference vertices.
                // Outputs already marked by revertTransivelyAndRemove.
                // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
                ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use m_user4_ಒಳ_Use;
                logicp->foreach_ಮೂಲ([](Dfg_ಶೃಂಗ& src) {
                    src.as<Dfg_ಶೃಂಗ_Var>()->vscp()->user4p(&src);
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                });
                V3DfgPasses::ಕೂಡಿಸು_Ast_Refs(m_dfg, logicp->nodep(), [](Ast_ಗಂಟು* varp) {  //
                    ಹಿಂತಿರುಗಿ varp->user4u().ಗಮ್ಯ<Dfg_ಶೃಂಗ_Var*>();
                });
            }

            // Delete this DfgLogic
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        // Reset the debug pointer, we have deleted it in the loop above ...
        m_debugLogicp = ಶೂನ್ಯಸೂಚಕ;
        debugDump("synth-rmlogics");

        //-------------------------------------------------------------------
        UINFO(5, "Step 5: Remove unnecessary splices");
        ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_dfg.opVertices().unlinkable()) {
            Dfg_ಶೃಂಗ_Splice* ಸ್ಥಿರಾಂಕ splicep = vtxp->cast<Dfg_ಶೃಂಗ_Splice>();
            ಆದರೆ (!splicep) ಮುಂದುವರಿ;

            // Might not have a sink if the driving logic was revered, remove
            ಆದರೆ (!splicep->hasSinks()) {
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
                ಮುಂದುವರಿ;
            }

            // It should alway have drivers
            UASSERT_OBJ(splicep->nInputs(), splicep, "Splice with no drivers");

            // If redundant, remove it
            ಆದರೆ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ wholep = splicep->wholep()) {
                ಆದರೆ (Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = splicep->singleSink()->cast<Dfg_ಶೃಂಗ_Var>()) {
                    varp->driver_File_ಸಾಲು(splicep->driver_File_ಸಾಲು(0));
                }
                splicep->replaceWith(wholep);
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
            }
        }
        debugDump("synth-rmsplice");
    }

    // CONSTRUCTOR
    Ast_ಗಮ್ಯ_Dfg_Synthesize(Dfg_ನಕ್ಷೆ& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx}
        , m_converter{dfg, ctx} {
        ಮುಖ್ಯ();
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(Dfg_ನಕ್ಷೆ& dfg, V3DfgSynthesisContext& ctx) {
        Ast_ಗಮ್ಯ_Dfg_Synthesize{dfg, ctx};

        // Final step outside, as both AstToDfgSynthesize and removeUnused used DfgUserMap
        UINFO(5, "Step 6: Remove all unused vertices");
        V3DfgPasses::remove_ಬಳಸದ(dfg);
        ಆದರೆ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("synth-rmunused");

        // No operation vertex should have multiple sinks. Cyclic decomoposition
        // depends on this and it can easily be ensured by using temporaries.
        // Also, all sources should be connected at this point
        ಆದರೆ (v3Global.opt.debugCheck()) {
            ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : dfg.opVertices()) {
                UASSERT_OBJ(!vtx.has_ಅಪವರ್ತ್ಯ_Sinks(), &vtx, "Operation has multiple sinks");
                ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < vtx.nInputs(); ++i) {
                    UASSERT_OBJ(vtx.inputp(i), &vtx, "Unconnected source operand");
                }
            }
            V3DfgPasses::typeCheck(dfg);
        }
    }
};

ಪೂರ್ಣಾಂಕ Ast_ಗಮ್ಯ_Dfg_Synthesize::AstVarScopeComparator::s_vscpIdCounter = 0;

// Decide which DfgLogic to attempt to synthesize
ಸ್ಥಿರ ಶೂನ್ಯ dfgSelectLogicForSynthesis(Dfg_ನಕ್ಷೆ& dfg) {
    // If we are told to synthesize everything, we will do so ...
    ಆದರೆ (v3Global.opt.fDfgSynthesizeAll()) {
        ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : dfg.opVertices()) {
            ಆದರೆ (DfgLogic* ಸ್ಥಿರಾಂಕ logicp = vtx.cast<DfgLogic>()) logicp->ಗಣ_Selected_For_Synthesis();
        }
        ಹಿಂತಿರುಗಿ;
    }

    // Otherwise figure out which vertices are likely worth synthesizing.

    std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*> circularVarps;
    // Synthesize all drivers of circular variables
    {
        DfgUserMap<uint64_t> scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, scc);
        ಗಾಗಿ (Dfg_ಶೃಂಗ_Var& var : dfg.varVertices()) {
            ಆದರೆ (!scc.at(var)) ಮುಂದುವರಿ;
            // This is a circular variable
            circularVarps.emplace_ಹಿಂಭಾಗ(&var);
        }
    }

    // To cover all drivers, use a work list
    DfgWorklist worklist{dfg};

    // Synthesize all drivers of circular variables
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp : circularVarps) {
        varp->srcp()->as<DfgUnresolved>()->foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& driver) {
            worklist.ತಳ್ಳು_front(*driver.as<DfgLogic>());
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
    }

    // Choose some simple special cases to always synthesize
    ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : dfg.opVertices()) {
        DfgLogic* ಸ್ಥಿರಾಂಕ logicp = vtx.cast<DfgLogic>();
        ಆದರೆ (!logicp || worklist.ಒಳಗೊಂಡಿದೆ(*logicp)) ಮುಂದುವರಿ;
        // If drives an unused variable, synthesize it so the partial logic can be removed
        ಆದರೆ (logicp->drives_ಬಳಸದ_Vars()) {
            worklist.ತಳ್ಳು_front(*logicp);
            ಮುಂದುವರಿ;
        }
        // Blocks corresponding to continuous assignments
        ಆದರೆ (logicp->nodep()->keyword() == VAlwaysKwd::CONT_ನಿಯೋಜಿಸು) {
            worklist.ತಳ್ಳು_front(*logicp);
            ಮುಂದುವರಿ;
        }
        ಸ್ಥಿರಾಂಕ Cfg_ನಕ್ಷೆ& cfg = logicp->cfg();
        // Straight line code with no branches
        ಆದರೆ (cfg.n_ಖಂಡಗಳು() == 1) {
            worklist.ತಳ್ಳು_front(*logicp);
            ಮುಂದುವರಿ;
        }
        // Blocks driving exactly 1 variable
        ಆದರೆ (!logicp->has_ಅಪವರ್ತ್ಯ_Sinks()) worklist.ತಳ್ಳು_front(*logicp);
    }

    std::unordered_ಗಣ<DfgUnresolved*> is_ಭೇಟಿಯಾದ;

    // Now expand to cover all logic driving the same set of variables and mark
    worklist.foreach([&](Dfg_ಶೃಂಗ& vtx) {
        DfgLogic& logic = *vtx.as<DfgLogic>();
        UASSERT_OBJ(!logic.selectedForSynthesis(), &vtx, "Should not be visited twice");
        // Mark as selected for synthesis
        logic.ಗಣ_Selected_For_Synthesis();
        // Enqueue all other logic driving the same variables as this one
        logic.foreachSink([&](Dfg_ಶೃಂಗ& sink) {
            DfgUnresolved* unresolved = sink.as<DfgUnresolved>();
            ಆದರೆ (!is_ಭೇಟಿಯಾದ.ಸೇರಿಸು(unresolved).ಎರಡನೇ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            unresolved->foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& sibling) {
                DfgLogic& siblingLogic = *sibling.as<DfgLogic>();
                ಆದರೆ (!siblingLogic.selectedForSynthesis()) worklist.ತಳ್ಳು_front(siblingLogic);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
    });
}

ಶೂನ್ಯ V3DfgPasses::synthesize(Dfg_ನಕ್ಷೆ& dfg, V3DfgContext& ctx) {
    // Select which DfgLogic to attempt to synthesize
    dfgSelectLogicForSynthesis(dfg);
    // Synthesize them - also removes un-synthesized DfgLogic, so must run even if nothing selected
    Ast_ಗಮ್ಯ_Dfg_Synthesize::apply(dfg, ctx.m_synthContext);
}
