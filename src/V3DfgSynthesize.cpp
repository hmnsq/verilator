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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Cfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPasses.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <iterator>

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ {

// Create a DfgVertex out of a AstNodeExpr. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_ਸ਼ੀਰਸ਼, ਕਿਸਮ_ਨਾਂ T_ਗੰਢ>
T_ਸ਼ੀਰਸ਼* make_ਸ਼ੀਰਸ਼(ਅਚਲ T_ਗੰਢ* nodep, Dfg_ਗ੍ਰਾਫ& dfg, ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype) {
    ਵਾਪਸ ਨਵਾਂ T_ਸ਼ੀਰਸ਼{dfg, nodep->fileline(), dtype};
}

ਸਾਂਚਾ <>
DfgArraySel* make_ਸ਼ੀਰਸ਼<DfgArraySel, AstArraySel>(ਅਚਲ AstArraySel* nodep, Dfg_ਗ੍ਰਾਫ& dfg,
                                                  ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype) {
    // Some earlier passes create malformed ArraySels, just bail on those...
    // See t_bitsel_wire_array_bad
    ਜੇ (VN_IS(nodep->fromp(), Const)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    ਜੇ (!VN_IS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    ਵਾਪਸ ਨਵਾਂ DfgArraySel{dfg, nodep->fileline(), dtype};
}

}  // namespace

// Visitor that can convert Ast statements and expressions in Dfg vertices
ਵਰਗ Ast_ਮੰਜ਼ਿਲ_Dfg_Converter final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // STATE
    Dfg_ਗ੍ਰਾਫ& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats

    // Current logic vertex we are synthesizing
    DfgLogic* m_logicp = ਖਾਲੀ_ਸੂਚਕ;
    // Variable updates produced by currently converted statement. This almost
    // always have a single element, so a vector is ok
    std::ਸਦਿਸ਼<std::pair<AstVarScope*, Dfg_ਸ਼ੀਰਸ਼_Var*>>* m_updatesp = ਖਾਲੀ_ਸੂਚਕ;

    ਤਾਰਕਿਕ m_ਮਿਲਿਆ_Unhandled = ਝੂਠ;  // Found node not implemented as DFG or not implemented 'visit'
    ਤਾਰਕਿਕ m_converting = ਝੂਠ;  // We are trying to convert some logic at the moment

    // METHODS

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2, ਕਿਸਮ_ਨਾਂ... ਦਲੀਲਾਂ_2>
    ਸ਼ੀਰਸ਼_2* make(ਦਲੀਲਾਂ_2&&... ਦਲੀਲਾਂ) {
        static_assert(!std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼_Var, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ, "Do not use for variables");
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ, "'Vertex' must be a 'DfgVertex'");
        ਸ਼ੀਰਸ਼_2* ਅਚਲ vtxp = ਨਵਾਂ ਸ਼ੀਰਸ਼_2{m_dfg, std::forward<ਦਲੀਲਾਂ_2>(ਦਲੀਲਾਂ)...};
        m_logicp->synth().emplace_ਪਿੱਠ(vtxp);
        ਵਾਪਸ vtxp;
    }

    // Returns true if the expression cannot (or should not) be represented by DFG
    ਤਾਰਕਿਕ unhandled(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        // Short-circuiting if something was already unhandled
        ਜੇ (m_ਮਿਲਿਆ_Unhandled) {
            // Impure nodes cannot be represented
            ਜੇ (!nodep->is_ਸ਼ੁੱਧ()) {
                m_ਮਿਲਿਆ_Unhandled = ਸੱਚ;
                ++m_ctx.m_conv.nonRepImpure;
            }
        }
        ਵਾਪਸ m_ਮਿਲਿਆ_Unhandled;
    }

    ਤਾਰਕਿਕ isSupported(ਅਚਲ AstVarRef* nodep) {
        // Cannot represent cross module references
        ਜੇ (nodep->classOrPackagep()) ਵਾਪਸ ਝੂਠ;
        // Check target
        ਵਾਪਸ V3Dfg::isSupported(nodep->varScopep());
    }

    // Given an RValue expression, return the equivalent Vertex, or nullptr if not representable.
    Dfg_ਸ਼ੀਰਸ਼* convertRValue(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        UASSERT_OBJ(!m_converting, nodep, "'convertingRValue' should not be called recursively");
        VL_RESTORER(m_converting);
        VL_RESTORER(m_ਮਿਲਿਆ_Unhandled);
        m_converting = ਸੱਚ;
        m_ਮਿਲਿਆ_Unhandled = ਝੂਠ;

        // Convert the expression
        iterate(nodep);

        // If falied to convert, return nullptr
        ਜੇ (m_ਮਿਲਿਆ_Unhandled) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        // Traversal set user2p to the equivalent vertex
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = nodep->user2u().ਮੰਜ਼ਿਲ<Dfg_ਸ਼ੀਰਸ਼*>();
        UASSERT_OBJ(vtxp, nodep, "Missing Dfg vertex after conversion");
        ਵਾਪਸ vtxp;
    }

    // Given an LValue expression, return the splice node that writes the
    // destination, together with the index to use for splicing in the value.
    // Returns {nullptr, 0}, if the given LValue expression is not supported.
    std::pair<Dfg_ਸ਼ੀਰਸ਼_Splice*, uint32_t> convertLValue(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ vrefp = VN_CAST(nodep, VarRef)) {
            UASSERT_OBJ(vrefp->access().isWriteOnly(), vrefp, "Non-WriteOnly reference");
            ਜੇ (!isSupported(vrefp)) {
                ++m_ctx.m_conv.nonRepLValue;
                ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0};
            }

            // Get (or create a new) temporary for this variable
            ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ vtxp = [&]() -> Dfg_ਸ਼ੀਰਸ਼_Var* {
                // The variable being assigned
                AstVarScope* ਅਚਲ vscp = vrefp->varScopep();

                // Find existing one, if any
                ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : *m_updatesp) {
                    ਜੇ (pair.ਪਹਿਲਾ == vscp) ਵਾਪਸ pair.ਦੂਜਾ;
                }

                // Create new one
                Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ newp = createTmp(*m_logicp, vscp, "SynthAssign");
                m_updatesp->emplace_ਪਿੱਠ(vscp, newp);

                // Create the Splice driver for the new temporary
                ਜੇ (newp->is<DfgVarPacked>()) {
                    newp->srcp(make<DfgSplicePacked>(newp->fileline(), newp->dtype()));
                } ਨਹੀਂਤਾਂ ਜੇ (newp->is<DfgVarArray>()) {
                    newp->srcp(make<DfgSpliceArray>(newp->fileline(), newp->dtype()));
                } ਨਹੀਂਤਾਂ {
                    nodep->v3fatalSrc("Unhandled DfgVertexVar sub-type");
                }

                // Use new temporary
                ਵਾਪਸ newp;
            }();

            // Return the Splice driver
            ਵਾਪਸ {vtxp->srcp()->as<Dfg_ਸ਼ੀਰਸ਼_Splice>(), 0};
        }

        ਜੇ (ਅਚਲ AstSel* selp = VN_CAST(nodep, Sel)) {
            // Only handle constant selects
            ਅਚਲ AstConst* ਅਚਲ lsbp = VN_CAST(selp->lsbp(), Const);
            ਜੇ (!lsbp) {
                ++m_ctx.m_conv.nonRepLValue;
                ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0};
            }
            uint32_t lsb = lsbp->ਮੰਜ਼ਿਲ_UInt();

            // Convert the 'fromp' sub-expression
            ਅਚਲ ਸਵੈਚਾਲਿਤ pair = convertLValue(selp->fromp());
            ਜੇ (!pair.ਪਹਿਲਾ) ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0};
            DfgSplicePacked* ਅਚਲ splicep = pair.ਪਹਿਲਾ->ਸਾਂਚਾ as<DfgSplicePacked>();
            // Adjust index.
            lsb += pair.ਦੂਜਾ;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            ਜੇ (lsb + static_cast<uint32_t>(selp->ਚੌੜਾਈ_Const()) > splicep->ਮਾਪ()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0};
            }

            // AstSel doesn't change type kind (array vs packed), so we can use
            // the existing splice driver with adjusted lsb
            ਵਾਪਸ {splicep, lsb};
        }

        ਜੇ (ਅਚਲ AstArraySel* ਅਚਲ aselp = VN_CAST(nodep, ArraySel)) {
            // Only handle constant selects
            ਅਚਲ AstConst* ਅਚਲ indexp = VN_CAST(aselp->bitp(), Const);
            ਜੇ (!indexp) {
                ++m_ctx.m_conv.nonRepLValue;
                ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0};
            }
            uint32_t ਸੂਚਕਾਂਕ = indexp->ਮੰਜ਼ਿਲ_UInt();

            // Convert the 'fromp' sub-expression
            ਅਚਲ ਸਵੈਚਾਲਿਤ pair = convertLValue(aselp->fromp());
            ਜੇ (!pair.ਪਹਿਲਾ) ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0};
            DfgSpliceArray* ਅਚਲ splicep = pair.ਪਹਿਲਾ->ਸਾਂਚਾ as<DfgSpliceArray>();
            // Adjust index. Note pair.second is always 0, but we might handle array slices later..
            ਸੂਚਕਾਂਕ += pair.ਦੂਜਾ;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            ਜੇ (ਸੂਚਕਾਂਕ + 1U > splicep->ਮਾਪ()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0};
            }

            // Ensure the Splice driver exists for this element
            ਜੇ (!splicep->driverAt(ਸੂਚਕਾਂਕ)) {
                File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
                // This should never fail
                ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype = *Dfg_ਅੰਕੜੇ_Type::fromAst(nodep->dtypep());
                ਜੇ (dtype.isPacked()) {
                    DfgSplicePacked* ਅਚਲ newp = make<DfgSplicePacked>(flp, dtype);
                    ਅਚਲ Dfg_ਅੰਕੜੇ_Type& uaDtype = Dfg_ਅੰਕੜੇ_Type::array(dtype, 1);
                    DfgUnitArray* ਅਚਲ uap = make<DfgUnitArray>(flp, uaDtype);
                    uap->srcp(newp);
                    splicep->ਜੋੜੋ_Driver(uap, ਸੂਚਕਾਂਕ, flp);
                } ਨਹੀਂਤਾਂ ਜੇ (dtype.isArray()) {
                    DfgSpliceArray* ਅਚਲ newp = make<DfgSpliceArray>(flp, dtype);
                    splicep->ਜੋੜੋ_Driver(newp, ਸੂਚਕਾਂਕ, flp);
                } ਨਹੀਂਤਾਂ {
                    nodep->v3fatalSrc("Unhandled data type kind");
                }
            }

            // Return the splice driver
            Dfg_ਸ਼ੀਰਸ਼* driverp = splicep->driverAt(ਸੂਚਕਾਂਕ);
            ਜੇ (ਅਚਲ DfgUnitArray* ਅਚਲ uap = driverp->cast<DfgUnitArray>()) {
                driverp = uap->srcp();
            }
            ਵਾਪਸ {driverp->as<Dfg_ਸ਼ੀਰਸ਼_Splice>(), 0};
        }

        ++m_ctx.m_conv.nonRepLValue;
        ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0};
    }

    // Given the LHS of an assignment, and the vertex representing the RHS,
    // connect up the RHS to drive the targets.
    // Returns true on success, false if the LHS is not representable.
    ਤਾਰਕਿਕ convertAssignment(File_ਸਤਰ* flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        // Represents a DFG assignment contributed by the AST assignment with the above 'lhsp'.
        // There might be multiple of these if 'lhsp' is a concatenation.
        ਬਣਤਰ Assignment final {
            Dfg_ਸ਼ੀਰਸ਼_Splice* m_lhsp;  // Splice vertex being driven
            uint32_t m_idx;  // Position (bit or array index) within m_lhsp being driven
            Dfg_ਸ਼ੀਰਸ਼* m_rhsp;  // Vertex driving that position
            Assignment() = ਮਿਟਾਓ;
            Assignment(Dfg_ਸ਼ੀਰਸ਼_Splice* lhsp, uint32_t idx, Dfg_ਸ਼ੀਰਸ਼* rhsp)
                : m_lhsp{lhsp}
                , m_idx{idx}
                , m_rhsp{rhsp} {}
        };

        // Simplify the LHS, to get rid of things like SEL(CONCAT(_, _), _)
        ਜੇ (!VN_IS(lhsp, VarRef)) lhsp = VN_AS(V3Const::constifyExpensiveEdit(lhsp), ਗੰਢ_ਵਿਅੰਜਕ);

        // Assigning compound expressions to a concatenated LHS requires a temporary
        // to avoid multiple use of the expression
        ਜੇ (VN_IS(lhsp, Concat) && !vtxp->is<Dfg_ਸ਼ੀਰਸ਼_Var>() && !vtxp->is<DfgConst>()) {
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ tmpp = createTmp(*m_logicp, flp, vtxp->dtype(), "Unpack");
            tmpp->srcp(vtxp);
            vtxp = tmpp;
        }

        // Convert each concatenation LHS separately, gather all assignments
        // we need to do into 'assignments', return true if all LValues
        // converted successfully.
        std::ਸਦਿਸ਼<Assignment> assignments;
        ਅਚਲ std::function<ਤਾਰਕਿਕ(Ast_ਗੰਢ_ਵਿਅੰਜਕ*, uint32_t)> convertAllLValues
            = [&](Ast_ਗੰਢ_ਵਿਅੰਜਕ* subp, uint32_t lsb) -> ਤਾਰਕਿਕ {
            // Concatenation on the LHS, convert each part
            ਜੇ (AstConcat* ਅਚਲ concatp = VN_CAST(subp, Concat)) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cRhsp = concatp->rhsp();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cLhsp = concatp->lhsp();
                // Convert Rigth of concat
                ਜੇ (!convertAllLValues(cRhsp, lsb)) ਵਾਪਸ ਝੂਠ;
                // Convert Left of concat
                ਵਾਪਸ convertAllLValues(cLhsp, lsb + cRhsp->ਚੌੜਾਈ());
            }

            // Non-concatenation, convert the LValue
            ਅਚਲ ਸਵੈਚਾਲਿਤ pair = convertLValue(subp);
            ਜੇ (!pair.ਪਹਿਲਾ) ਵਾਪਸ ਝੂਠ;

            // If whole lhs, just use it
            ਜੇ (subp == lhsp) {
                assignments.emplace_ਪਿੱਠ(pair.ਪਹਿਲਾ, pair.ਦੂਜਾ, vtxp);
                ਵਾਪਸ ਸੱਚ;
            }

            // Otherwise select the relevant bits
            ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype = *Dfg_ਅੰਕੜੇ_Type::fromAst(subp->dtypep());
            DfgSel* ਅਚਲ selp = make<DfgSel>(subp->fileline(), dtype);
            selp->fromp(vtxp);
            selp->lsb(lsb);
            assignments.emplace_ਪਿੱਠ(pair.ਪਹਿਲਾ, pair.ਦੂਜਾ, selp);
            ਵਾਪਸ ਸੱਚ;
        };

        // Convert the given LHS assignment, give up if any LValues failed to convert
        ਜੇ (!convertAllLValues(lhsp, 0)) ਵਾਪਸ ਝੂਠ;

        // All successful, connect the drivers
        ਲਈ (ਅਚਲ Assignment& ਵਸਤੂ_3 : assignments) {
            ਜੇ (DfgSplicePacked* ਅਚਲ spp = ਵਸਤੂ_3.m_lhsp->ਸਾਂਚਾ cast<DfgSplicePacked>()) {
                spp->ਜੋੜੋ_Driver(ਵਸਤੂ_3.m_rhsp, ਵਸਤੂ_3.m_idx, flp);
            } ਨਹੀਂਤਾਂ ਜੇ (DfgSpliceArray* ਅਚਲ sap = ਵਸਤੂ_3.m_lhsp->ਸਾਂਚਾ cast<DfgSpliceArray>()) {
                // TODO: multi-dimensional arrays will need changes here
                ਅਚਲ Dfg_ਅੰਕੜੇ_Type& rDt = ਵਸਤੂ_3.m_rhsp->dtype();
                ਜੇ (rDt.isPacked()) {
                    // RHS is assigning an element of this array. Need a DfgUnitArray adapter.
                    DfgUnitArray* ਅਚਲ uap = make<DfgUnitArray>(flp, Dfg_ਅੰਕੜੇ_Type::array(rDt, 1));
                    uap->srcp(ਵਸਤੂ_3.m_rhsp);
                    sap->ਜੋੜੋ_Driver(uap, ਵਸਤੂ_3.m_idx, flp);
                } ਨਹੀਂਤਾਂ {
                    // RHS is assigning an array (or array slice). Should be the same element type.
                    sap->ਜੋੜੋ_Driver(ਵਸਤੂ_3.m_rhsp, ਵਸਤੂ_3.m_idx, flp);
                }
            } ਨਹੀਂਤਾਂ {
                ਵਸਤੂ_3.m_lhsp->v3fatalSrc("Unhandled DfgVertexSplice sub-type");
            }
        }

        ਵਾਪਸ ਸੱਚ;
    }

    // VISITORS

    // Unhandled node
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* /*nodep*/) override {
        ਜੇ (!m_ਮਿਲਿਆ_Unhandled && m_converting) ++m_ctx.m_conv.nonRepUnknown;
        m_ਮਿਲਿਆ_Unhandled = ਸੱਚ;
    }

    // Expressions - mostly auto generated, but a few special ones
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ਜੇ (unhandled(nodep)) ਵਾਪਸ;
        // This visit method is only called on RValues, where only read refs are supported
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Non-ReadOnly reference");
        ਜੇ (!isSupported(nodep)) {
            m_ਮਿਲਿਆ_Unhandled = ਸੱਚ;
            ++m_ctx.m_conv.nonRepVarRef;
            ਵਾਪਸ;
        }

        // Variable should have been bound before starting conversion
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = nodep->varScopep()->user2u().ਸਾਂਚਾ ਮੰਜ਼ਿਲ<Dfg_ਸ਼ੀਰਸ਼_Var*>();
        UASSERT_OBJ(vtxp, nodep, "Referenced variable has no associated DfgVertexVar");
        nodep->user2p(vtxp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConst* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ਜੇ (unhandled(nodep)) ਵਾਪਸ;

        ਜੇ (nodep->ਚੌੜਾਈ() != nodep->ਅੰਕ().ਚੌੜਾਈ()) {
            // Sometimes the width of the AstConst is not the same as the
            // V3Number it holds. Truncate it here. TODO: should this be allowed?
            V3_ਸੰਖਿਆ ਅੰਕ{nodep, nodep->ਚੌੜਾਈ()};
            ਅੰਕ.opSel(nodep->ਅੰਕ(), nodep->ਚੌੜਾਈ() - 1, 0);
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = make<DfgConst>(nodep->fileline(), ਅੰਕ);
            nodep->user2p(vtxp);
        } ਨਹੀਂਤਾਂ {
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = make<DfgConst>(nodep->fileline(), nodep->ਅੰਕ());
            nodep->user2p(vtxp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCReset* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ਜੇ (unhandled(nodep)) ਵਾਪਸ;

        ਅਚਲ Dfg_ਅੰਕੜੇ_Type* ਅਚਲ dtypep = Dfg_ਅੰਕੜੇ_Type::fromAst(nodep->dtypep());
        ਜੇ (!dtypep) {
            m_ਮਿਲਿਆ_Unhandled = ਸੱਚ;
            ++m_ctx.m_conv.nonRepDType;
            ਵਾਪਸ;
        }

        UASSERT_OBJ(!nodep->constructing(), nodep,
                    "CReset should be non-constructing at this stage");

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = make<DfgCReset>(nodep->fileline(), *dtypep);
        nodep->user2p(vtxp);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਮੇਲ_Masked* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ਜੇ (unhandled(nodep)) ਵਾਪਸ;

        ਅਚਲ Dfg_ਅੰਕੜੇ_Type* ਅਚਲ dtypep = Dfg_ਅੰਕੜੇ_Type::fromAst(nodep->dtypep());
        ਜੇ (!dtypep) {
            m_ਮਿਲਿਆ_Unhandled = ਸੱਚ;
            ++m_ctx.m_conv.nonRepDType;
            ਵਾਪਸ;
        }

        iterate(nodep->lhsp());
        ਜੇ (m_ਮਿਲਿਆ_Unhandled) ਵਾਪਸ;
        iterate(nodep->matchp());
        ਜੇ (m_ਮਿਲਿਆ_Unhandled) ਵਾਪਸ;

        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        Dfg_ਮੇਲ_Masked* ਅਚਲ vtxp = make<Dfg_ਮੇਲ_Masked>(flp, *dtypep);
        vtxp->lhsp(nodep->lhsp()->user2u().ਮੰਜ਼ਿਲ<Dfg_ਸ਼ੀਰਸ਼*>());
        vtxp->matchp(nodep->matchp()->user2u().ਮੰਜ਼ਿਲ<Dfg_ਸ਼ੀਰਸ਼*>());
        nodep->user2p(vtxp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstReplicate* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ਜੇ (unhandled(nodep)) ਵਾਪਸ;

        ਅਚਲ Dfg_ਅੰਕੜੇ_Type* ਅਚਲ dtypep = Dfg_ਅੰਕੜੇ_Type::fromAst(nodep->dtypep());
        ਜੇ (!dtypep) {
            m_ਮਿਲਿਆ_Unhandled = ਸੱਚ;
            ++m_ctx.m_conv.nonRepDType;
            ਵਾਪਸ;
        }

        iterate(nodep->srcp());
        ਜੇ (m_ਮਿਲਿਆ_Unhandled) ਵਾਪਸ;

        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        DfgRep* ਅਚਲ vtxp = make<DfgRep>(flp, *dtypep);
        vtxp->srcp(nodep->srcp()->user2u().ਮੰਜ਼ਿਲ<Dfg_ਸ਼ੀਰਸ਼*>());
        nodep->user2p(vtxp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ਜੇ (unhandled(nodep)) ਵਾਪਸ;

        ਅਚਲ Dfg_ਅੰਕੜੇ_Type* ਅਚਲ dtypep = Dfg_ਅੰਕੜੇ_Type::fromAst(nodep->dtypep());
        ਜੇ (!dtypep) {
            m_ਮਿਲਿਆ_Unhandled = ਸੱਚ;
            ++m_ctx.m_conv.nonRepDType;
            ਵਾਪਸ;
        }

        iterate(nodep->fromp());
        ਜੇ (m_ਮਿਲਿਆ_Unhandled) ਵਾਪਸ;

        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        Dfg_ਸ਼ੀਰਸ਼* vtxp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->lsbp(), Const)) {
            ਅਚਲ uint32_t lsb = constp->ਮੰਜ਼ਿਲ_UInt();
            ਅਚਲ uint32_t msb = lsb + nodep->ਚੌੜਾਈ_Const() - 1;
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ fromp = nodep->fromp()->user2u().ਮੰਜ਼ਿਲ<Dfg_ਸ਼ੀਰਸ਼*>();
            // Unfortunately we can still have out of bounds selects due to how
            // indices are truncated for speed reasons in V3Width/V3Unknown.
            ਜੇ (msb >= fromp->ਮਾਪ()) {
                m_ਮਿਲਿਆ_Unhandled = ਸੱਚ;
                ++m_ctx.m_conv.nonRepOOBSel;
                ਵਾਪਸ;
            }
            DfgSel* ਅਚਲ selp = make<DfgSel>(flp, *dtypep);
            selp->fromp(fromp);
            selp->lsb(lsb);
            vtxp = selp;
        } ਨਹੀਂਤਾਂ {
            iterate(nodep->lsbp());
            ਜੇ (m_ਮਿਲਿਆ_Unhandled) ਵਾਪਸ;
            DfgMux* ਅਚਲ muxp = make<DfgMux>(flp, *dtypep);
            muxp->fromp(nodep->fromp()->user2u().ਮੰਜ਼ਿਲ<Dfg_ਸ਼ੀਰਸ਼*>());
            muxp->lsbp(nodep->lsbp()->user2u().ਮੰਜ਼ਿਲ<Dfg_ਸ਼ੀਰਸ਼*>());
            vtxp = muxp;
        }
        nodep->user2p(vtxp);
    }

// The rest of the visit methods for expressions are generated by 'astgen'
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg__gen_ast_to_dfg.h"

ਜਨਤਕ:
    // PUBLIC METHODS

    // Create temporay variable capable of holding the given type
    Dfg_ਸ਼ੀਰਸ਼_Var* createTmp(DfgLogic& logic, File_ਸਤਰ* flp, ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype,
                            ਅਚਲ std::string& ਅਗੇਤਰ) {
        Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ vtxp = m_dfg.makeNewVar(flp, ਅਗੇਤਰ, dtype, logic.scopep());
        logic.synth().emplace_ਪਿੱਠ(vtxp);
        vtxp->vscp()->varp()->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
        vtxp->tmpForp(vtxp->vscp());
        ਵਾਪਸ vtxp;
    }

    // Create a new temporary variable capable of holding 'varp'
    Dfg_ਸ਼ੀਰਸ਼_Var* createTmp(DfgLogic& logic, AstVarScope* vscp, ਅਚਲ std::string& ਅਗੇਤਰ) {
        AstVar* ਅਚਲ astVarp = vscp->varp();
        File_ਸਤਰ* ਅਚਲ flp = astVarp->fileline();
        ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype = *Dfg_ਅੰਕੜੇ_Type::fromAst(astVarp->dtypep());
        ਅਚਲ std::string prfx = ਅਗੇਤਰ + "_" + astVarp->name();
        Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ vtxp = createTmp(logic, flp, dtype, prfx);
        vtxp->tmpForp(vscp);
        ਵਾਪਸ vtxp;
    }

    // Convert AstAssign to Dfg, return true if successful.
    // Fills 'updates' with bindings for assigned variables.
    ਤਾਰਕਿਕ convert(std::ਸਦਿਸ਼<std::pair<AstVarScope*, Dfg_ਸ਼ੀਰਸ਼_Var*>>& updates, DfgLogic& vtx,
                 Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) {
        UASSERT_OBJ(VN_IS(nodep, ਸੌਂਪੋ) || VN_IS(nodep, ਸੌਂਪੋ_W), nodep, "Bad NodeAssign");
        UASSERT_OBJ(updates.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "'updates' should be empty");
        VL_RESTORER(m_updatesp);
        VL_RESTORER(m_logicp);
        m_updatesp = &updates;
        m_logicp = &vtx;
        // Assignment with timing control shouldn't make it this far
        UASSERT_OBJ(!nodep->timingControlp(), nodep, "Shouldn't make it this far");
        // Convert it
        ++m_ctx.m_conv.ਨਿਵੇਸ਼_Assignments;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp();
        // Check data types are compatible.
        ਅਚਲ Dfg_ਅੰਕੜੇ_Type* ਅਚਲ lDtypep = Dfg_ਅੰਕੜੇ_Type::fromAst(lhsp->dtypep());
        ਅਚਲ Dfg_ਅੰਕੜੇ_Type* ਅਚਲ rDtypep = Dfg_ਅੰਕੜੇ_Type::fromAst(rhsp->dtypep());
        ਜੇ (!lDtypep || !rDtypep) {
            ++m_ctx.m_conv.nonRepDType;
            ਵਾਪਸ ਝੂਠ;
        }
        // For now, only direct array assignment is supported (e.g. a = b, but not a = _ ? b : c)
        ਜੇ (rDtypep->isArray()) {
            ਜੇ (!VN_IS(rhsp, VarRef)
                || !lhsp->dtypep()->skipRefp()->same_ਰੁੱਖ(rhsp->dtypep()->skipRefp())) {
                ++m_ctx.m_conv.nonRepDType;
                ਵਾਪਸ ਝੂਠ;
            }
        }
        // Widths should match at this point
        UASSERT_OBJ(lhsp->ਚੌੜਾਈ() == rhsp->ਚੌੜਾਈ(), nodep, "Mismatched width reached DFG");
        // Convert the RHS expression
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rVtxp = convertRValue(rhsp);
        ਜੇ (!rVtxp) ਵਾਪਸ ਝੂਠ;
        // Connect the RHS vertex to the LHS targets
        ਅਚਲ ਤਾਰਕਿਕ success = convertAssignment(nodep->fileline(), lhsp, rVtxp);
        ਜੇ (success) ++m_ctx.m_conv.representable;
        ਵਾਪਸ success;
    }

    // Convert RValue expression to Dfg. Returns nullptr if failed.
    Dfg_ਸ਼ੀਰਸ਼* convert(DfgLogic& vtx, Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;
        // Convert it
        ++m_ctx.m_conv.ਨਿਵੇਸ਼_Expressions;
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = convertRValue(nodep);
        ਜੇ (vtxp) ++m_ctx.m_conv.representable;
        ਵਾਪਸ vtxp;
    }

    // CONSTRUCTOR
    Ast_ਮੰਜ਼ਿਲ_Dfg_Converter(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {}
};

// Debug aid - outisde 'AstToDfgSynthesize' as it is a template, but want one instance
V3DebugBisect s_dfgSynthDebugBisect{"DfgSynthesize"};

ਵਰਗ Ast_ਮੰਜ਼ਿਲ_Dfg_Synthesize final {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // TYPES

    // SymTab must be ordered in order to yield stable results
    ਬਣਤਰ AstVarScopeComparator final {
        ਸਥਿਰ ਪੂਰਨ s_vscpIdCounter;  // Counter for lazily allocating the unique AstVarScope IDs
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(AstVarScope* lhs, AstVarScope* rhs) ਅਚਲ {
            ਜੇ (!lhs->user4()) lhs->user4(++s_vscpIdCounter);
            ਜੇ (!rhs->user4()) rhs->user4(++s_vscpIdCounter);
            ਵਾਪਸ lhs->user4() < rhs->user4();
        }
    };
    ਵਰਤੋਂ SymTab = std::map<AstVarScope*, Dfg_ਸ਼ੀਰਸ਼_Var*, AstVarScopeComparator>;

    // Represents a [potentially partial] driver of a variable
    ਬਣਤਰ Driver final {
        Dfg_ਸ਼ੀਰਸ਼* m_vtxp = ਖਾਲੀ_ਸੂਚਕ;  // Driving vertex
        uint32_t m_lo = 0;  // Low index of driven range (internal, not Verilog)
        uint32_t m_hi = 0;  // High index of driven range (internal, not Verilog)
        File_ਸਤਰ* m_flp = ਖਾਲੀ_ਸੂਚਕ;  // Location of driver in source

        Driver() = ਮੂਲ;
        Driver(Dfg_ਸ਼ੀਰਸ਼* vtxp, uint32_t lo, File_ਸਤਰ* flp)
            : m_vtxp{vtxp}
            , m_lo{lo}
            , m_hi{lo + vtxp->ਮਾਪ() - 1U}
            , m_flp{flp} {}
        ਸੰਚਾਲਕ ਤਾਰਕਿਕ() ਅਚਲ { ਵਾਪਸ m_vtxp != ਖਾਲੀ_ਸੂਚਕ; }

        ਤਾਰਕਿਕ ਸੰਚਾਲਕ<(ਅਚਲ Driver& other) ਅਚਲ {
            ਜੇ (m_lo != other.m_lo) ਵਾਪਸ m_lo < other.m_lo;
            ਜੇ (m_hi != other.m_hi) ਵਾਪਸ m_hi < other.m_hi;
            ਵਾਪਸ m_flp->operatorCompare(*other.m_flp) < 0;
        }

        ਤਾਰਕਿਕ ਸੰਚਾਲਕ<=(ਅਚਲ Driver& other) ਅਚਲ { ਵਾਪਸ !(other < *ਇਹ); }
    };

    // STATE - Persistent
    Dfg_ਗ੍ਰਾਫ& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats
    Ast_ਮੰਜ਼ਿਲ_Dfg_Converter m_converter;  // The convert instance to use for each construct
    DfgWorklist m_ਮੰਜ਼ਿਲ_Revert{m_dfg};  // We need a worklist for reverting synthesis

    // STATE - for current DfgLogic being synthesized
    DfgLogic* m_logicp = ਖਾਲੀ_ਸੂਚਕ;  // Current logic vertex we are synthesizing
    Cfg_ਖੰਡ_Map<SymTab> m_bb_ਮੰਜ਼ਿਲ_ISym_Tab;  // Map from CfgBlock -> input symbol table
    Cfg_ਖੰਡ_Map<SymTab> m_bb_ਮੰਜ਼ਿਲ_OSym_Tab;  // Map from CfgBlock -> output symbol table
    Cfg_ਖੰਡ_Map<Dfg_ਸ਼ੀਰਸ਼_Var*> m_bb_ਮੰਜ਼ਿਲ_Condp;  // Map from CfgBlock ->  terminating branch condition
    Cfg_ਕਿਨਾਰਾ_Map<Dfg_ਸ਼ੀਰਸ਼_Var*> m_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ_Predicatep;  // Map CfgGraphEdge -> path predicate to there
    Cfg_Dominator_ਰੁੱਖ m_dom_ਰੁੱਖ;  // The dominator tree of the current CFG

    // STATE - Some debug aid
    // We stop after synthesizing s_dfgSynthDebugLimit vertices (if non-zero).
    // This is the problematic logic (last one we synthesize), assuming a
    // bisection search over s_dfgSynthDebugLimit.
    DfgLogic* m_debugLogicp = ਖਾਲੀ_ਸੂਚਕ;
    // Source (upstream) cone of outputs of m_debugLogicp
    std::unique_ptr<std::unordered_ਸੈੱਟ<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*>> m_debugOSrcConep{ਖਾਲੀ_ਸੂਚਕ};

    // METHODS

    // Dump current graph for debugging ...
    ਖਾਲੀ debugDump(ਅਚਲ ਅੱਖਰ* name) {
        // If we have the debugged logic, compute the vertices feeding its outputs
        ਜੇ (VL_UNLIKELY(m_debugLogicp)) {
            std::ਸਦਿਸ਼<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*> outputs;
            m_debugLogicp->foreachSink([&outputs](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& v) {
                outputs.emplace_ਪਿੱਠ(v.singleSink()->as<Dfg_ਸ਼ੀਰਸ਼_Var>());
                ਵਾਪਸ ਝੂਠ;
            });
            m_debugOSrcConep = m_dfg.ਸਰੋਤ_Cone(outputs);
        }

        ਜੇ (VL_UNLIKELY(dumpDfgLevel() >= 9 || m_debugOSrcConep)) {
            m_dfg.dumpDotFilePrefixed(name);
            ਜੇ (m_debugOSrcConep) {
                // Dump only the subgraph involving the inputs and outputs of the bad vertex
                m_dfg.dumpDotFilePrefixed(name + "-min"s, [&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& v) -> ਤਾਰਕਿਕ {
                    ਵਾਪਸ m_debugOSrcConep->ਗਿਣਤੀ_2(&v);
                });
            }
        }
    }

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2, ਕਿਸਮ_ਨਾਂ... ਦਲੀਲਾਂ_2>
    ਸ਼ੀਰਸ਼_2* make(ਦਲੀਲਾਂ_2&&... ਦਲੀਲਾਂ) {
        static_assert(!std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼_Var, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ, "Do not use for variables");
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ, "'Vertex' must be a 'DfgVertex'");
        ਸ਼ੀਰਸ਼_2* ਅਚਲ vtxp = ਨਵਾਂ ਸ਼ੀਰਸ਼_2{m_dfg, std::forward<ਦਲੀਲਾਂ_2>(ਦਲੀਲਾਂ)...};
        ਜੇ (m_logicp) m_logicp->synth().emplace_ਪਿੱਠ(vtxp);
        ਵਾਪਸ vtxp;
    }

    // Gather all drivers of a resolved variable
    ਸਥਿਰ std::ਸਦਿਸ਼<Driver> gatherDrivers(Dfg_ਸ਼ੀਰਸ਼_Splice* vtxp) {
        // Collect them all, check if they are sorted
        std::ਸਦਿਸ਼<Driver> drivers;
        drivers.reserve(vtxp->nInputs());
        ਤਾਰਕਿਕ sorted = ਸੱਚ;
        vtxp->foreachDriver([&](Dfg_ਸ਼ੀਰਸ਼& src, uint32_t lo, File_ਸਤਰ* flp) {
            // Collect the driver
            drivers.emplace_ਪਿੱਠ(&src, lo, flp);
            // Check if drivers are sorted - most often they are
            ਅਚਲ ਮਾਪ_t n = drivers.ਮਾਪ();
            ਜੇ (n >= 2 && drivers[n - 1] < drivers[n - 2]) sorted = ਝੂਠ;
            ਵਾਪਸ ਝੂਠ;
        });

        // Sort if unsorted
        ਜੇ (!sorted) std::stable_ਛਾਂਟੋ(drivers.begin(), drivers.ਸਿਰਾ());

        //ਪੂਰਾ
        ਵਾਪਸ drivers;
    }

    // Returns true if the driver supplies a complete array element.
    ਸਥਿਰ ਤਾਰਕਿਕ driver_Covers_ਸਾਰਾ_Element(ਅਚਲ Driver& driver) {
        ਅਚਲ DfgUnitArray* ਅਚਲ unitp = driver.m_vtxp->cast<DfgUnitArray>();
        ਜੇ (!unitp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (Dfg_ਸ਼ੀਰਸ਼_Splice* ਅਚਲ splicep = unitp->srcp()->cast<Dfg_ਸ਼ੀਰਸ਼_Splice>()) {
            ਵਾਪਸ splicep->wholep();
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // Unlike 'wholep', several element drivers can collectively cover an array.
    ਸਥਿਰ ਤਾਰਕਿਕ splice_Covers_ਸਾਰਾ(Dfg_ਸ਼ੀਰਸ਼_Splice* ਅਚਲ splicep) {
        ਜੇ (splicep->wholep()) ਵਾਪਸ ਸੱਚ;
        ਜੇ (splicep->isPacked()) ਵਾਪਸ ਝੂਠ;
        uint32_t ਅਗਲਾ = 0;
        ਲਈ (ਅਚਲ Driver& driver : gatherDrivers(splicep)) {
            ਜੇ (driver.m_lo != ਅਗਲਾ || !driver_Covers_ਸਾਰਾ_Element(driver)) ਵਾਪਸ ਝੂਠ;
            ਅਗਲਾ = driver.m_hi + 1;
        }
        ਵਾਪਸ ਅਗਲਾ == splicep->ਮਾਪ();
    }

    // Returns true if the driver cone contains any variable introduced by
    // tristate lowering. Used to distinguish intentional tristate contributor
    // overlap from accidental multidrive.
    ਸਥਿਰ ਤਾਰਕਿਕ ਸ਼ਾਮਲ_ਹੈ_Tri_Lowered_Var(Dfg_ਸ਼ੀਰਸ਼* rootp) {
        std::ਸਦਿਸ਼<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*> ਢੇਰ;
        std::ਸਦਿਸ਼<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*> ਦੇਖਿਆ;
        ਢੇਰ.emplace_ਪਿੱਠ(rootp);
        ਜਦੋਂ (!ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = ਢੇਰ.ਪਿੱਠ();
            ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
            ਜੇ (std::ਲੱਭੋ(ਦੇਖਿਆ.begin(), ਦੇਖਿਆ.ਸਿਰਾ(), vtxp) != ਦੇਖਿਆ.ਸਿਰਾ()) ਜਾਰੀ;
            ਦੇਖਿਆ.emplace_ਪਿੱਠ(vtxp);
            ਜੇ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = vtxp->cast<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
                ਜੇ (varp->vscp()->varp()->dfgTriLowered()) ਵਾਪਸ ਸੱਚ;
            }
            vtxp->foreach_ਸਰੋਤ([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& src) {
                ਢੇਰ.emplace_ਪਿੱਠ(&src);
                ਵਾਪਸ ਝੂਠ;
            });
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // Returns true if the driver is a direct variable forward (no logic).
    ਸਥਿਰ ਤਾਰਕਿਕ is_ਸਿੱਧਾ_Var_Driver(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* vtxp) { ਵਾਪਸ vtxp->is<Dfg_ਸ਼ੀਰਸ਼_Var>(); }

    // Gather all synthesized drivers of an unresolved variable
    ਸਥਿਰ std::ਸਦਿਸ਼<Driver> gatherDriversUnresolved(DfgUnresolved* vtxp) {
        std::ਸਦਿਸ਼<Driver> drivers;
        drivers.reserve(vtxp->nInputs());

        // For better locations in error reporting, we unpick concatenations
        // which are sometimes introduced by combinint assignments in V3Const.
        ਅਚਲ std::function<ਖਾਲੀ(Dfg_ਸ਼ੀਰਸ਼*, uint32_t, File_ਸਤਰ*)> gather
            = [&](Dfg_ਸ਼ੀਰਸ਼* vtxp, uint32_t lo, File_ਸਤਰ* flp) -> ਖਾਲੀ {
            ਜੇ (DfgConcat* ਅਚਲ concatp = vtxp->cast<DfgConcat>()) {
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = concatp->rhsp();
                gather(rhsp, lo, rhsp->fileline());
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = concatp->lhsp();
                gather(lhsp, lo + rhsp->ਚੌੜਾਈ(), lhsp->fileline());
                ਵਾਪਸ;
            }
            drivers.emplace_ਪਿੱਠ(vtxp, lo, flp);
        };

        // Gather all synthesized drivers
        vtxp->foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& src) {
            // Can ignore the original DfgLogic
            ਜੇ (src.is<DfgLogic>()) ਵਾਪਸ ਝੂਠ;
            // Synthesized drivers must be a splice at this point
            Dfg_ਸ਼ੀਰਸ਼_Splice* ਅਚਲ splicep = src.as<Dfg_ਸ਼ੀਰਸ਼_Splice>();
            // Collect the driver
            splicep->foreachDriver([&](Dfg_ਸ਼ੀਰਸ਼& src, uint32_t lo, File_ਸਤਰ* flp) {
                gather(&src, lo, flp);
                ਵਾਪਸ ਝੂਠ;
            });
            ਵਾਪਸ ਝੂਠ;
        });

        // Sort the drivers
        std::stable_ਛਾਂਟੋ(drivers.begin(), drivers.ਸਿਰਾ());

        //ਪੂਰਾ
        ਵਾਪਸ drivers;
    }

    // Given two drivers, combine the driven sub-ranges into the first one if
    // possible. First bool returned indicates successfully combined and there
    // are no multi-driven bits. Second bool returned indicates we warned
    // already about multi-driven bits.
    std::pair<ਤਾਰਕਿਕ, ਤਾਰਕਿਕ> combineDrivers(Dfg_ਸ਼ੀਰਸ਼_Var& var, ਅਚਲ std::string& sub,  //
                                         Driver& a, ਅਚਲ Driver& b) {
        // We can only combine array drivers ...
        ਜੇ (a.m_vtxp->isPacked()) ਵਾਪਸ {ਝੂਠ, ਝੂਠ};
        // ... that both drive a single element ...
        ਜੇ (a.m_lo != b.m_lo) ਵਾਪਸ {ਝੂਠ, ਝੂਠ};
        ਅਚਲ DfgUnitArray* ਅਚਲ aUap = a.m_vtxp->ਸਾਂਚਾ cast<DfgUnitArray>();
        ਜੇ (!aUap) ਵਾਪਸ {ਝੂਠ, ਝੂਠ};
        ਅਚਲ DfgUnitArray* ਅਚਲ bUap = b.m_vtxp->ਸਾਂਚਾ cast<DfgUnitArray>();
        ਜੇ (!bUap) ਵਾਪਸ {ਝੂਠ, ਝੂਠ};
        // ... and are themeselves partial
        DfgSplicePacked* ਅਚਲ aSp = aUap->srcp()->ਸਾਂਚਾ cast<DfgSplicePacked>();
        ਜੇ (!aSp) ਵਾਪਸ {ਝੂਠ, ਝੂਠ};
        DfgSplicePacked* ਅਚਲ bSp = bUap->srcp()->ਸਾਂਚਾ cast<DfgSplicePacked>();
        ਜੇ (!bSp) ਵਾਪਸ {ਝੂਠ, ਝੂਠ};
        UASSERT_OBJ(aSp->dtype() == bSp->dtype(), &var, "DTypes should match");

        // Gather drivers of a
        std::ਸਦਿਸ਼<Driver> aDrivers = gatherDrivers(aSp);
        // Gather drivers of b
        std::ਸਦਿਸ਼<Driver> bDrivers = gatherDrivers(bSp);

        // Merge them
        std::ਸਦਿਸ਼<Driver> abDrivers;
        abDrivers.reserve(aDrivers.ਮਾਪ() + bDrivers.ਮਾਪ());
        std::ਮਿਲਾਓ(  //
            aDrivers.begin(), aDrivers.ਸਿਰਾ(),  //
            bDrivers.begin(), bDrivers.ਸਿਰਾ(),  //
            std::ਪਿੱਠ_inserter(abDrivers)  //
        );

        // Attempt to resolve them
        ਜੇ (!normalizeDrivers(var, abDrivers, sub + "[" + std::ਮੰਜ਼ਿਲ_string(a.m_lo) + "]")) {
            ਵਾਪਸ {ਝੂਠ, ਸੱਚ};
        }

        // Successfully resolved. Needs a new splice and unit.
        File_ਸਤਰ* ਅਚਲ flp = var.fileline();
        DfgSplicePacked* ਅਚਲ splicep = make<DfgSplicePacked>(flp, aSp->dtype());
        ਲਈ (ਅਚਲ Driver& d : abDrivers) splicep->ਜੋੜੋ_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        DfgUnitArray* ਅਚਲ uap = make<DfgUnitArray>(flp, aUap->dtype());
        uap->srcp(splicep);
        a.m_vtxp = uap;
        ਵਾਪਸ {ਸੱਚ, ਝੂਠ};
    }

    // Combine and coalesce the given drivers.
    // Returns true iff no multi-driven bits are present.
    ਤਾਰਕਿਕ normalizeDrivers(Dfg_ਸ਼ੀਰਸ਼_Var& var, std::ਸਦਿਸ਼<Driver>& drivers,
                          ਅਚਲ std::string& sub = "") {
        ਜੇ (drivers.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਸੱਚ;

        // What type of values are we combining
        ਅਚਲ ਤਾਰਕਿਕ isPacked = drivers[0].m_vtxp->isPacked();

        // Found a multidriven part ?
        ਤਾਰਕਿਕ multiDriven = ਝੂਠ;

        // Iterate through the sorted drivers. Index 'i' is the driver we are
        // resolving driver 'j' agains, and if required, we merge 'j' into 'i'.
        ਮਾਪ_t i = 0;
        ਲਈ (ਮਾਪ_t j = 1; j < drivers.ਮਾਪ();) {
            UASSERT_OBJ(i < j, &var, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            ਜੇ (!jD) {
                ++j;
                ਜਾਰੀ;
            }
            // If 'i' was moved, move 'j' in it's place
            ਜੇ (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                ਜਾਰੀ;
            }

            // We have 2 valid drivers now
            UASSERT_OBJ(iD <= jD, &var, "Should always be sorted");
            UASSERT_OBJ(jD.m_vtxp->isPacked() == isPacked, &var, "Mixed type drivers");

            // If no overlap, consider next pair
            ਜੇ (iD.m_hi < jD.m_lo) {
                ++i;
                ਜੇ (i == j) ++j;
                ਜਾਰੀ;
            }

            // There is an overlap. Attempt to combine them.
            ਤਾਰਕਿਕ combined = ਝੂਠ;
            ਤਾਰਕਿਕ warned = ਝੂਠ;
            std::tie(combined, warned) = combineDrivers(var, sub, iD, jD);

            // If sucessfully combined, 'j' is no longer needed, it was combined into 'i'
            ਜੇ (combined) {
                jD = Driver{};
                ++j;
                ਜਾਰੀ;
            }

            // Found overlap that cannot be resolved
            multiDriven = ਸੱਚ;
            // Compare next driver
            ++j;

            // Do not warn again if we warned during 'combineDrivers'
            ਜੇ (warned) ਜਾਰੀ;

            // The variable to warn on
            AstVarScope* ਅਚਲ vscp = var.tmpForp() ? var.tmpForp() : var.vscp();

            // Loop index often abused, so suppress
            ਜੇ (vscp->varp()->is_ਵਰਤਿਆ_Loop_Idx()) ਜਾਰੀ;
            // Tristate lowering can intentionally create overlapping contributors.
            // Keep the signal marked multidriven for DFG fallback, but suppress
            // warning only when both overlapping driver cones look tri-lowered.
            ਜੇ (vscp->varp()->dfgAllowMultidriveTri()) {
                ਅਚਲ ਤਾਰਕਿਕ iTri = ਸ਼ਾਮਲ_ਹੈ_Tri_Lowered_Var(iD.m_vtxp);
                ਅਚਲ ਤਾਰਕਿਕ jTri = ਸ਼ਾਮਲ_ਹੈ_Tri_Lowered_Var(jD.m_vtxp);
                ਅਚਲ ਤਾਰਕਿਕ triPair = iTri && jTri;
                ਅਚਲ ਤਾਰਕਿਕ triAndBridge = (iTri && is_ਸਿੱਧਾ_Var_Driver(jD.m_vtxp))
                                          || (jTri && is_ਸਿੱਧਾ_Var_Driver(iD.m_vtxp));
                ਜੇ (triPair || triAndBridge) ਜਾਰੀ;
            }

            // Warn the user now
            ਅਚਲ std::string lo = std::ਮੰਜ਼ਿਲ_string(jD.m_lo);
            ਅਚਲ std::string hi = std::ਮੰਜ਼ਿਲ_string(std::min(iD.m_hi, jD.m_hi));
            ਅਚਲ std::string ਕਿਸਮ = isPacked ? "Bit" : "Element";
            ਅਚਲ std::string part = hi == lo ? (" [" + lo + "]") : ("s [" + hi + ":" + lo + "]");

            vscp->v3warn(  //
                MULTIDRIVEN,  //
                ਕਿਸਮ << part << " of signal '" << vscp->prettyName() << sub << "'"
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
        ਜੇ (!multiDriven && isPacked) coalesceDrivers(drivers);

        ਵਾਪਸ !multiDriven;
    }

    // Coalesce adjacent drivers into single ones
    ਖਾਲੀ coalesceDrivers(std::ਸਦਿਸ਼<Driver>& drivers) {
        UASSERT(!drivers.ਖਾਲੀ_ਥਾਵਾਂ(), "Can't coalesce 0 drivers");
        UASSERT_OBJ(drivers[0].m_vtxp->isPacked(), drivers[0].m_vtxp, "Can only coalesce packed");

        ਮਾਪ_t i = 0;
        ਲਈ (ਮਾਪ_t j = 1; j < drivers.ਮਾਪ();) {
            UASSERT(i < j, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            ਜੇ (!jD) {
                ++j;
                ਜਾਰੀ;
            }
            // If 'i' was moved, move 'j' in it's place
            ਜੇ (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                ਜਾਰੀ;
            }

            // We have 2 valid drivers now
            UASSERT(iD <= jD, "Should always be sorted");

            // If not adjacent, move on
            ਜੇ (iD.m_hi + 1 != jD.m_lo) {
                ++i;
                ਜੇ (i == j) ++j;
                ਜਾਰੀ;
            }

            // Coalesce Adjacent ranges,
            ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dt = Dfg_ਅੰਕੜੇ_Type::packed(iD.m_vtxp->ਚੌੜਾਈ() + jD.m_vtxp->ਚੌੜਾਈ());
            DfgConcat* ਅਚਲ concatp = make<DfgConcat>(iD.m_flp, dt);
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
    Dfg_ਸ਼ੀਰਸ਼_Splice* makeSplice(Dfg_ਸ਼ੀਰਸ਼_Var& var, ਅਚਲ std::ਸਦਿਸ਼<Driver>& newDrivers) {
        UASSERT_OBJ(!newDrivers.ਖਾਲੀ_ਥਾਵਾਂ(), &var, "'makeSplice' called with no new drivers");
        // Create new driver
        Dfg_ਸ਼ੀਰਸ਼_Splice* splicep = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (var.is<DfgVarPacked>()) {
            splicep = make<DfgSplicePacked>(var.fileline(), var.dtype());
        } ਨਹੀਂਤਾਂ ਜੇ (var.is<DfgVarArray>()) {
            splicep = make<DfgSpliceArray>(var.fileline(), var.dtype());
        } ਨਹੀਂਤਾਂ {
            var.v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        ਲਈ (ਅਚਲ Driver& d : newDrivers) splicep->ਜੋੜੋ_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        ਵਾਪਸ splicep;
    }

    // Initialzie input symbol table of entry CfgBlock
    ਖਾਲੀ initialize_ਇੰਦਰਾਜ_ਚਿੰਨ੍ਹ_ਸਾਰਣੀ(SymTab& iSymTab) {
        m_logicp->foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& src) {
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ vvp = src.as<Dfg_ਸ਼ੀਰਸ਼_Var>();
            iSymTab[vvp->vscp()] = vvp;
            ਵਾਪਸ ਝੂਠ;
        });
    }

    // Join variable drivers across a control flow confluence (insert muxes ...)
    Dfg_ਸ਼ੀਰਸ਼_Var* joinDrivers(AstVarScope* vscp, Dfg_ਸ਼ੀਰਸ਼_Var* predicatep,  //
                              Dfg_ਸ਼ੀਰਸ਼_Var* thenp, Dfg_ਸ਼ੀਰਸ਼_Var* elsep) {
        AstVarScope* ਅਚਲ thenVscp = thenp->tmpForp() ? thenp->tmpForp() : thenp->vscp();
        AstVarScope* ਅਚਲ elseVscp = elsep->tmpForp() ? elsep->tmpForp() : elsep->vscp();
        UASSERT_OBJ(thenVscp == elseVscp, vscp, "Attempting to join unrelated variables");

        // If both bindings are the the same (variable not updated through either path),
        // then there is nothing to do, can use the same binding
        ਜੇ (thenp == elsep) ਵਾਪਸ thenp;

        // We can't join the input variable just yet, so bail
        ਜੇ (thenp->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_ਨਿਵੇਸ਼;
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਜੇ (elsep->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_ਨਿਵੇਸ਼;
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }

        // Can't do arrays yet
        ਜੇ (!thenp->isPacked()) {
            ++m_ctx.m_synt.nonSynArray;
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }

        // Gather drivers of 'thenp' - only if 'thenp' is not an input to the synthesized block
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ tDefaultp = thenp->defaultp();
        std::ਸਦਿਸ਼<Driver> tDrivers = gatherDrivers(thenp->srcp()->as<Dfg_ਸ਼ੀਰਸ਼_Splice>());

        // Gather drivers of 'elsep' - only if 'thenp' is not an input to the synthesized block
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ eDefaultp = elsep->defaultp();
        std::ਸਦਿਸ਼<Driver> eDrivers = gatherDrivers(elsep->srcp()->as<Dfg_ਸ਼ੀਰਸ਼_Splice>());

        // Default drivers should be the same or not present on either
        UASSERT_OBJ(tDefaultp == eDefaultp, vscp, "Different default drivers");

        // Location to use for the join vertices
        File_ਸਤਰ* ਅਚਲ flp = predicatep->fileline();

        // Create a fresh temporary for the joined value
        Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ joinp = m_converter.createTmp(*m_logicp, vscp, "SynthJoin");
        Dfg_ਸ਼ੀਰਸ਼_Splice* ਅਚਲ joinSplicep = make<DfgSplicePacked>(flp, joinp->dtype());
        joinp->srcp(joinSplicep);

        // If both paths are fully driven, just create a simple conditional
        ਜੇ (tDrivers.ਮਾਪ() == 1  //
            && tDrivers[0].m_lo == 0  //
            && tDrivers[0].m_hi == thenp->ਚੌੜਾਈ() - 1  //
            && eDrivers.ਮਾਪ() == 1  //
            && eDrivers[0].m_lo == 0  //
            && eDrivers[0].m_hi == elsep->ਚੌੜਾਈ() - 1) {
            UASSERT_OBJ(!tDefaultp, vscp, "Fully driven variable have default driver");

            DfgCond* ਅਚਲ condp = make<DfgCond>(flp, joinp->dtype());
            condp->condp(predicatep);
            condp->thenp(thenp);
            condp->elsep(elsep);
            joinSplicep->ਜੋੜੋ_Driver(condp, 0, tDrivers[0].m_flp);

            //ਪੂਰਾ
            ਵਾਪਸ joinp;
        }

        // Otherwise we need to merge them part by part

        // If different bits are driven, then some might not have been assigned.. Latch?
        ਜੇ (tDrivers.ਮਾਪ() != eDrivers.ਮਾਪ()) {
            ++m_ctx.m_synt.nonSynLatch;
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }

        ਲਈ (ਮਾਪ_t i = 0; i < tDrivers.ਮਾਪ(); ++i) {
            ਅਚਲ Driver& tDriver = tDrivers[i];
            ਅਚਲ Driver& eDriver = eDrivers[i];
            // If different bits are driven, then some might not have been assigned.. Latch?
            ਜੇ (tDriver.m_lo != eDriver.m_lo || tDriver.m_hi != eDriver.m_hi) {
                ++m_ctx.m_synt.nonSynLatch;
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            }

            ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype = Dfg_ਅੰਕੜੇ_Type::packed(tDriver.m_hi - tDriver.m_lo + 1);
            DfgCond* ਅਚਲ condp = make<DfgCond>(flp, dtype);
            condp->condp(predicatep);

            // We actally need to select the bits from the joined variables, not use the drivers
            DfgSel* ਅਚਲ thenSelp = make<DfgSel>(flp, tDriver.m_vtxp->dtype());
            thenSelp->lsb(tDriver.m_lo);
            thenSelp->fromp(thenp);
            condp->thenp(thenSelp);

            // Same for the 'else' part
            DfgSel* ਅਚਲ elseSelp = make<DfgSel>(flp, eDriver.m_vtxp->dtype());
            elseSelp->lsb(eDriver.m_lo);
            elseSelp->fromp(elsep);
            condp->elsep(elseSelp);

            // Add it as a driver to the join
            joinSplicep->ਜੋੜੋ_Driver(condp, tDriver.m_lo, tDriver.m_flp);
        }

        // If there was a default driver, add it to te join
        ਜੇ (tDefaultp) joinp->defaultp(tDefaultp);

        //ਪੂਰਾ
        ਵਾਪਸ joinp;
    }

    // Merge 'thenSymTab' into 'elseSymTab' using the given predicate to join values
    ਤਾਰਕਿਕ join_ਚਿੰਨ੍ਹ_Tables(SymTab& elseSymTab, Dfg_ਸ਼ੀਰਸ਼_Var* predicatep, ਅਚਲ SymTab& thenSymTab) {
        // Any variable that does not have a binding on both paths will be removed. These might be
        // temporaries, loop vars, etc used only in one branch. Conversion will fail if the
        // variable is actually referenced later.
        std::ਸਦਿਸ਼<AstVarScope*> ਮੰਜ਼ਿਲ_Remove;

        // Join each symbol
        ਲਈ (std::pair<AstVarScope* ਅਚਲ, Dfg_ਸ਼ੀਰਸ਼_Var*>& pair : elseSymTab) {
            AstVarScope* ਅਚਲ varp = pair.ਪਹਿਲਾ;
            // Find same variable on the else path
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = thenSymTab.ਲੱਭੋ(varp);
            // Record for removal if not assigned on both paths
            ਜੇ (it == thenSymTab.ਸਿਰਾ()) {
                ਮੰਜ਼ਿਲ_Remove.emplace_ਪਿੱਠ(varp);
                ਜਾਰੀ;
            }
            // Join paths with the predicate
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ thenp = it->ਦੂਜਾ;
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ elsep = pair.ਦੂਜਾ;
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ newp = joinDrivers(varp, predicatep, thenp, elsep);
            ਜੇ (!newp) ਵਾਪਸ ਝੂਠ;
            pair.ਦੂਜਾ = newp;
        }

        // Remove variables not assigned on both paths
        ਲਈ (AstVarScope* ਅਚਲ varp : ਮੰਜ਼ਿਲ_Remove) elseSymTab.erase(varp);

        //ਪੂਰਾ
        ਵਾਪਸ ਸੱਚ;
    }

    // Given two joining control flow edges, compute how to join their symbols.
    // Returns the predicaete to join over, and the 'then' and 'else' blocks.
    std::tuple<Dfg_ਸ਼ੀਰਸ਼_Var*, ਅਚਲ Cfg_ਖੰਡ*, ਅਚਲ Cfg_ਖੰਡ*>  //
    how_ਮੰਜ਼ਿਲ_Join(ਅਚਲ Cfg_ਕਿਨਾਰਾ* ਅਚਲ ap, ਅਚਲ Cfg_ਕਿਨਾਰਾ* ਅਚਲ bp) {
        // Find the closest common dominator of the two paths
        ਅਚਲ Cfg_ਖੰਡ* ਅਚਲ domp = m_dom_ਰੁੱਖ.closestCommonDominator(ap->srcp(), bp->srcp());
        // These paths join here, so 'domp' must be a branch, otherwise it's not the closest
        UASSERT_OBJ(domp->isBranch(), domp, "closestCommonDominator is not a branch");

        // The branches of the common dominator
        ਅਚਲ Cfg_ਕਿਨਾਰਾ* ਅਚਲ ਲਿਆ_Edgep = domp->ਲਿਆ_Edgep();
        ਅਚਲ Cfg_ਕਿਨਾਰਾ* ਅਚਲ untknEdgep = domp->untknEdgep();

        // We check if the taken branch dominates the path to either blocks,
        // and if the untaken branch dominates the path to the other block.
        // If so, we can use the branch condition as predicate, otherwise
        // we must use the path predicate as there are ways to get from one
        // branch of the dominator to the other. We need to be careful if
        // either branches are directly to the join block. This is fine,
        // it's as if there was an empty block on that critical edge which
        // is dominated by that path.

        ਜੇ (ਲਿਆ_Edgep == ap || m_dom_ਰੁੱਖ.dominates(ਲਿਆ_Edgep->dstp(), ap->srcp())) {
            ਜੇ (untknEdgep == bp || m_dom_ਰੁੱਖ.dominates(untknEdgep->dstp(), bp->srcp())) {
                // Taken path dominates 'ap' and untaken dominates 'bp', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                ਵਾਪਸ std::make_tuple(m_bb_ਮੰਜ਼ਿਲ_Condp[domp], ap->srcp(), bp->srcp());
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਲਿਆ_Edgep == bp || m_dom_ਰੁੱਖ.dominates(ਲਿਆ_Edgep->dstp(), bp->srcp())) {
            ਜੇ (untknEdgep == ap || m_dom_ਰੁੱਖ.dominates(untknEdgep->dstp(), ap->srcp())) {
                // Taken path dominates 'bp' and untaken dominates 'ap', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                ਵਾਪਸ std::make_tuple(m_bb_ਮੰਜ਼ਿਲ_Condp[domp], bp->srcp(), ap->srcp());
            }
        }

        // The branches don't dominate the joined blocks, must use the path predicate
        ++m_ctx.m_synt.join_Using_ਰਸਤਾ_Predicate;

        // TODO: We could do better here: use the path predicate of the closest
        // cominating blocks, pick the one from the lower rank, etc, but this
        // generic case is very rare, most synthesizable logic has
        // series-parallel CFGs which are covered by the earlier cases.
        ਵਾਪਸ std::make_tuple(m_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ_Predicatep[ap], ap->srcp(), bp->srcp());
    }

    // Combine the output symbol tables of the predecessors of the given
    // block to compute the input symtol table for the given block.
    ਤਾਰਕਿਕ create_ਨਿਵੇਸ਼_ਚਿੰਨ੍ਹ_ਸਾਰਣੀ(ਅਚਲ Cfg_ਖੰਡ& bb) {
        // The input symbol table of the given block, we are computing it now
        SymTab& joined = m_bb_ਮੰਜ਼ਿਲ_ISym_Tab[bb];

        // Input symbol table of entry block is special
        ਜੇ (bb.isEnter()) {
            initialize_ਇੰਦਰਾਜ_ਚਿੰਨ੍ਹ_ਸਾਰਣੀ(joined);
            ਵਾਪਸ ਸੱਚ;
        }

        // Current input symbol table should be empty, we will fill it in here
        UASSERT(joined.ਖਾਲੀ_ਥਾਵਾਂ(), "Unprocessed input symbol table should be empty");

        // Fast path if there is only one predecessor - TODO: use less copying
        ਜੇ (!bb.isJoin()) {
            joined = m_bb_ਮੰਜ਼ਿਲ_OSym_Tab[bb.ਪਹਿਲਾ_Predecessorp()];
            ਵਾਪਸ ਸੱਚ;
        }

        // We also have a simpler job if there are 2 predecessors
        ਜੇ (bb.isTwoWayJoin()) {
            Dfg_ਸ਼ੀਰਸ਼_Var* predicatep = ਖਾਲੀ_ਸੂਚਕ;
            ਅਚਲ Cfg_ਖੰਡ* thenp = ਖਾਲੀ_ਸੂਚਕ;
            ਅਚਲ Cfg_ਖੰਡ* elsep = ਖਾਲੀ_ਸੂਚਕ;
            std::tie(predicatep, thenp, elsep)
                = how_ਮੰਜ਼ਿਲ_Join(bb.ਪਹਿਲਾ_Predecessor_Edgep(), bb.ਆਖਰੀ_Predecessor_Edgep());
            // Copy from else
            joined = m_bb_ਮੰਜ਼ਿਲ_OSym_Tab[elsep];
            // Join with then
            ਵਾਪਸ join_ਚਿੰਨ੍ਹ_Tables(joined, predicatep, m_bb_ਮੰਜ਼ਿਲ_OSym_Tab[*thenp]);
        }

        // General hard way

        // Gather predecessors
        ਬਣਤਰ Predecessor final {
            ਅਚਲ Cfg_ਖੰਡ* m_bbp;  // Predeccessor block
            Dfg_ਸ਼ੀਰਸ਼_Var* m_predicatep;  // Predicate predecessor reached this block with
            ਅਚਲ SymTab* m_oSymTabp;  // Output symbol table or predecessor
            Predecessor() = ਮਿਟਾਓ;
            Predecessor(ਅਚਲ Cfg_ਖੰਡ* bbp, Dfg_ਸ਼ੀਰਸ਼_Var* predicatep, ਅਚਲ SymTab* oSymTabp)
                : m_bbp{bbp}
                , m_predicatep{predicatep}
                , m_oSymTabp{oSymTabp} {}
        };

        ਅਚਲ std::ਸਦਿਸ਼<Predecessor> predecessors = [&]() {
            std::ਸਦਿਸ਼<Predecessor> res;
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : bb.ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                ਅਚਲ Cfg_ਕਿਨਾਰਾ& cfg_ਕਿਨਾਰਾ = static_cast<ਅਚਲ Cfg_ਕਿਨਾਰਾ&>(ਕਿਨਾਰਾ);
                ਅਚਲ Cfg_ਖੰਡ* ਅਚਲ predecessorp = cfg_ਕਿਨਾਰਾ.srcp();
                Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ predicatep = m_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ_Predicatep[cfg_ਕਿਨਾਰਾ];
                ਅਚਲ SymTab* ਅਚਲ oSymTabp = &m_bb_ਮੰਜ਼ਿਲ_OSym_Tab[predecessorp];
                res.emplace_ਪਿੱਠ(predecessorp, predicatep, oSymTabp);
            }
            // Sort predecessors reverse topologically. This way earlier blocks
            // will come after later blocks, and the entry block is last if present.
            std::ਛਾਂਟੋ(res.begin(), res.ਸਿਰਾ(), [](ਅਚਲ Predecessor& a, ਅਚਲ Predecessor& b) {  //
                ਵਾਪਸ *a.m_bbp > *b.m_bbp;
            });
            ਵਾਪਸ res;
        }();

        // Start by copying the bindings from the frist predecessor
        joined = *predecessors[0].m_oSymTabp;
        // Join over all other predecessors
        ਲਈ (ਮਾਪ_t i = 1; i < predecessors.ਮਾਪ(); ++i) {
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ predicatep = predecessors[i].m_predicatep;
            ਅਚਲ SymTab& oSymTab = *predecessors[i].m_oSymTabp;
            ਜੇ (!join_ਚਿੰਨ੍ਹ_Tables(joined, predicatep, oSymTab)) ਵਾਪਸ ਝੂਠ;
        }

        ਵਾਪਸ ਸੱਚ;
    }

    std::ਸਦਿਸ਼<Driver> computePropagatedDrivers(ਅਚਲ std::ਸਦਿਸ਼<Driver>& newDrivers,
                                                 Dfg_ਸ਼ੀਰਸ਼_Var* oldp) {
        // Gather drivers of 'oldp' - they are in incresing range order with no overlaps
        ਅਚਲ std::ਸਦਿਸ਼<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<Dfg_ਸ਼ੀਰਸ਼_Splice>());
        UASSERT_OBJ(!oldDrivers.ਖਾਲੀ_ਥਾਵਾਂ(), oldp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::ਸਦਿਸ਼<Driver> propagatedDrivers;

        // Add bits between 'msb' and 'lsb' from 'oldp' to 'pDrivers'
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਜੋੜੋ_Old_Driver = [&](File_ਸਤਰ* ਅਚਲ flp, uint32_t msb, uint32_t lsb) {
            UASSERT_OBJ(propagatedDrivers.ਖਾਲੀ_ਥਾਵਾਂ() || lsb > propagatedDrivers.ਪਿੱਠ().m_hi, flp,
                        "Drivers should be in ascending order");
            DfgSel* ਅਚਲ selp = make<DfgSel>(flp, Dfg_ਅੰਕੜੇ_Type::packed(msb - lsb + 1));
            selp->lsb(lsb);
            selp->fromp(oldp);
            propagatedDrivers.emplace_ਪਿੱਠ(selp, lsb, flp);
        };

        // Incorporate old drivers
        ਲਈ (ਅਚਲ Driver& oDriver : oldDrivers) {
            File_ਸਤਰ* ਅਚਲ flp = oDriver.m_flp;
            // Range to consider inserting, we will adjust oldLo as we process drivers
            uint32_t oldLo = oDriver.m_lo;
            ਅਚਲ uint32_t oldHi = oDriver.m_hi;

            // Loop for now, can move to bisection search if this is a problem, shouldn't be ...
            ਲਈ (ਅਚਲ Driver& nDriver : newDrivers) {
                UASSERT_OBJ(oldHi >= oldLo, flp, "Should have stopped iteration");
                // If new driver is entirely below old driver, move on to
                ਜੇ (nDriver.m_hi < oldLo) ਜਾਰੀ;
                // If new driver is entirely above old driver, we can stop
                ਜੇ (oldHi < nDriver.m_lo) ਰੋਕੋ;

                // There is an overlap between 'oDriver' and 'nDriver'.
                // Insert the low bits and adjust the insertion range.
                // The rest will take care of itself on subsequent iterations.
                ਜੇ (oldLo < nDriver.m_lo) ਜੋੜੋ_Old_Driver(flp, nDriver.m_lo - 1, oldLo);
                oldLo = nDriver.m_hi + 1;

                // Stop if no more bits remaining in the old driver
                ਜੇ (oldLo > oldHi) ਰੋਕੋ;
            }

            // Insert remaining bits if any
            ਜੇ (oldHi >= oldLo) ਜੋੜੋ_Old_Driver(flp, oldHi, oldLo);
        }

        ਵਾਪਸ propagatedDrivers;
    }

    // Propagate whole elements with a linear walk over the sorted driver lists.
    // Reads use the previous value temporary, preserving assignment-version bindings.
    // Partial elements and non-unit drivers retain the nonsynthesized-process fallback.
    ਤਾਰਕਿਕ computePropagatedArrayDrivers(ਅਚਲ std::ਸਦਿਸ਼<Driver>& newDrivers,
                                       Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ oldp,
                                       std::ਸਦਿਸ਼<Driver>& propagatedDrivers) {
        // Bound quadratic vertex growth: array drivers cannot be coalesced.
        ਸਥਿਰ constexpr uint32_t MAX_ARRAY_ELEMENTS = 32;
        ਜੇ (oldp->ਮਾਪ() > MAX_ARRAY_ELEMENTS) ਵਾਪਸ ਝੂਠ;
        ਲਈ (ਅਚਲ Driver& driver : newDrivers) {
            ਜੇ (!driver_Covers_ਸਾਰਾ_Element(driver)) ਵਾਪਸ ਝੂਠ;
        }
        ਅਚਲ std::ਸਦਿਸ਼<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<Dfg_ਸ਼ੀਰਸ਼_Splice>());
        UASSERT_OBJ(!oldDrivers.ਖਾਲੀ_ਥਾਵਾਂ(), oldp, "Should have a proper driver");
        ਲਈ (ਅਚਲ Driver& driver : oldDrivers) {
            ਜੇ (!driver_Covers_ਸਾਰਾ_Element(driver)) ਵਾਪਸ ਝੂਠ;
        }

        propagatedDrivers.reserve(oldDrivers.ਮਾਪ());
        ਸਵੈਚਾਲਿਤ nIt = newDrivers.begin();
        ਲਈ (ਅਚਲ Driver& oDriver : oldDrivers) {
            ਜਦੋਂ (nIt != newDrivers.ਸਿਰਾ() && nIt->m_lo < oDriver.m_lo) ++nIt;
            ਜੇ (nIt != newDrivers.ਸਿਰਾ() && nIt->m_lo == oDriver.m_lo) ਜਾਰੀ;

            File_ਸਤਰ* ਅਚਲ flp = oDriver.m_flp;
            ਅਚਲ DfgUnitArray* ਅਚਲ oldUnitp = oDriver.m_vtxp->as<DfgUnitArray>();
            DfgArraySel* ਅਚਲ selp = make<DfgArraySel>(flp, oldUnitp->srcp()->dtype());
            selp->fromp(oldp);
            selp->bitp(make<DfgConst>(flp, static_cast<ਮਾਪ_t>(VL_IDATASIZE), oDriver.m_lo));
            DfgUnitArray* ਅਚਲ newUnitp = make<DfgUnitArray>(flp, oldUnitp->dtype());
            newUnitp->srcp(selp);
            propagatedDrivers.emplace_ਪਿੱਠ(newUnitp, oDriver.m_lo, flp);
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // Given the drivers of a variable after converting a single statement
    // 'newp', add drivers from 'oldp' that were not reassigned be drivers
    // in newp. This computes the total result of all previous assignments.
    ਤਾਰਕਿਕ incorporate_ਪਿਛਲਾ_ਮੁੱਲ(AstVarScope* vscp, Dfg_ਸ਼ੀਰਸ਼_Var* newp, Dfg_ਸ਼ੀਰਸ਼_Var* oldp) {
        UASSERT_OBJ(newp->srcp(), vscp, "Assigned variable has no driver");

        // Easy if there is no old value...
        ਜੇ (!oldp) ਵਾਪਸ ਸੱਚ;

        // New driver was not yet coalesced, so should always be a splice
        Dfg_ਸ਼ੀਰਸ਼_Splice* ਅਚਲ nSplicep = newp->srcp()->as<Dfg_ਸ਼ੀਰਸ਼_Splice>();

        // If the old value is the real variable we just computed the new value for,
        // then it is the circular feedback into the synthesized block, add it as default driver.
        ਜੇ (oldp->vscp() == vscp) {
            ਜੇ (!splice_Covers_ਸਾਰਾ(nSplicep)) newp->defaultp(oldp);
            ਵਾਪਸ ਸੱਚ;
        }

        UASSERT_OBJ(oldp->srcp(), vscp, "Previously assigned variable has no driver");

        // Gather drivers of 'newp' - they are in incresing range order with no overlaps
        UASSERT_OBJ(!newp->defaultp(), newp, "Converted value should not have default");
        std::ਸਦਿਸ਼<Driver> nDrivers = gatherDrivers(newp->srcp()->as<Dfg_ਸ਼ੀਰਸ਼_Splice>());
        UASSERT_OBJ(!nDrivers.ਖਾਲੀ_ਥਾਵਾਂ(), newp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::ਸਦਿਸ਼<Driver> pDrivers;
        ਜੇ (newp->isPacked()) {
            pDrivers = computePropagatedDrivers(nDrivers, oldp);
        } ਨਹੀਂਤਾਂ ਜੇ (!computePropagatedArrayDrivers(nDrivers, oldp, pDrivers)) {
            ++m_ctx.m_synt.nonSynArray;
            ਵਾਪਸ ਝੂਠ;
        }

        ਜੇ (!pDrivers.ਖਾਲੀ_ਥਾਵਾਂ()) {
            // Need to merge propagated sources, so reset the splice
            nSplicep->resetDrivers();
            // Merge drivers - they are both sorted and non-overlapping
            std::ਸਦਿਸ਼<Driver> drivers;
            drivers.reserve(nDrivers.ਮਾਪ() + pDrivers.ਮਾਪ());
            std::ਮਿਲਾਓ(nDrivers.begin(), nDrivers.ਸਿਰਾ(), pDrivers.begin(), pDrivers.ਸਿਰਾ(),
                       std::ਪਿੱਠ_inserter(drivers));
            // Coalesce adjacent ranges
            ਜੇ (newp->isPacked()) coalesceDrivers(drivers);
            // Reinsert drivers in order
            ਲਈ (ਅਚਲ Driver& d : drivers) nSplicep->ਜੋੜੋ_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        }

        // If the old had a default, add to the new one too, unless redundant
        ਜੇ (oldp->defaultp() && !splice_Covers_ਸਾਰਾ(nSplicep)) newp->defaultp(oldp->defaultp());

        //ਪੂਰਾ
        ਵਾਪਸ ਸੱਚ;
    }

    // Synthesize the given statements with the given input symbol table.
    // Returns true if successfolly synthesized.
    // Populates the given output symbol table.
    // Populates the given reference with the condition of the terminator branch, if any.
    ਤਾਰਕਿਕ synthesize_Basic_ਖੰਡ(SymTab& oSymTab, Dfg_ਸ਼ੀਰਸ਼*& condpr,
                              ਅਚਲ std::ਸਦਿਸ਼<Ast_ਗੰਢ_Stmt*>& stmtps, ਅਚਲ SymTab& iSymTab) {
        // Use fresh set of vertices in m_converter
        ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;

        // Initialize AstVarScope -> Vertex bindings available in this block
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : iSymTab) {
            AstVarScope* ਅਚਲ varp = pair.ਪਹਿਲਾ;
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ vtxp = pair.ਦੂਜਾ;
            varp->user2p(vtxp);
            oSymTab[varp] = vtxp;
        }

        // Synthesize each statement one after the other
        std::ਸਦਿਸ਼<std::pair<AstVarScope*, Dfg_ਸ਼ੀਰਸ਼_Var*>> updates;
        ਲਈ (Ast_ਗੰਢ_Stmt* ਅਚਲ stmtp : stmtps) {
            // Regular statements
            Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ ap = VN_CAST(stmtp, ਗੰਢ_ਸੌਂਪੋ);
            ਜੇ (ap && (VN_IS(ap, ਸੌਂਪੋ) || VN_IS(ap, ਸੌਂਪੋ_W))) {
                // Convert this assignment
                ਜੇ (!m_converter.convert(updates, *m_logicp, ap)) {
                    ++m_ctx.m_synt.nonSynConv;
                    ਵਾਪਸ ਝੂਠ;
                }
                // Apply variable updates from this statement
                ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : updates) {
                    // The target variable that was assigned to
                    AstVarScope* ਅਚਲ vscp = pair.ਪਹਿਲਾ;
                    // The new, potentially partially assigned value
                    Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ newp = pair.ਦੂਜਾ;
                    // Normalize drivers within this statement, bail if multidriven
                    Dfg_ਸ਼ੀਰਸ਼_Splice* ਅਚਲ srcp = newp->srcp()->as<Dfg_ਸ਼ੀਰਸ਼_Splice>();
                    std::ਸਦਿਸ਼<Driver> drivers = gatherDrivers(srcp);
                    ਅਚਲ ਤਾਰਕਿਕ single = drivers.ਮਾਪ() == 1;
                    ਜੇ (!normalizeDrivers(*newp, drivers)) {
                        ++m_ctx.m_synt.nonSynMultidrive;
                        ਵਾਪਸ ਝੂਠ;
                    }
                    // If there were more than one driver (often not), re-add in case coalesced
                    ਜੇ (!single) {
                        srcp->resetDrivers();
                        ਲਈ (ਅਚਲ Driver& d : drivers) srcp->ਜੋੜੋ_Driver(d.m_vtxp, d.m_lo, d.m_flp);
                    }
                    // The old value, if any
                    Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ oldp = vscp->user2u().ਸਾਂਚਾ ਮੰਜ਼ਿਲ<Dfg_ਸ਼ੀਰਸ਼_Var*>();
                    // Inncorporate old value into the new value
                    ਜੇ (!incorporate_ਪਿਛਲਾ_ਮੁੱਲ(vscp, newp, oldp)) ਵਾਪਸ ਝੂਠ;
                    // Update binding of target variable
                    vscp->user2p(newp);
                    // Update output symbol table of this block
                    oSymTab[vscp] = newp;
                }
                updates.clear();
                ਜਾਰੀ;
            }

            // Terminator branches
            ਜੇ (AstIf* ਅਚਲ ifp = VN_CAST(stmtp, If)) {
                UASSERT_OBJ(ifp == stmtps.ਪਿੱਠ(), ifp, "Branch should be last statement");
                // Convert condition, give up if failed
                Dfg_ਸ਼ੀਰਸ਼* condp = m_converter.convert(*m_logicp, ifp->condp());
                ਜੇ (!condp) {
                    ++m_ctx.m_synt.nonSynConv;
                    ਵਾਪਸ ਝੂਠ;
                }
                // Single bit condition can be use directly, otherwise: use 'condp != 0'
                ਜੇ (condp->ਚੌੜਾਈ() != 1) {
                    File_ਸਤਰ* ਅਚਲ flp = condp->fileline();
                    DfgNeq* ਅਚਲ neqp = make<DfgNeq>(flp, Dfg_ਅੰਕੜੇ_Type::packed(1));
                    neqp->lhsp(make<DfgConst>(flp, condp->ਚੌੜਾਈ(), 0U));
                    neqp->rhsp(condp);
                    condp = neqp;
                }
                condpr = condp;
                ਜਾਰੀ;
            }

            // Unhandled
            ++m_ctx.m_synt.nonSynStmt;
            ਵਾਪਸ ਝੂਠ;
        }

        ਵਾਪਸ ਸੱਚ;
    }

    // Assign path predicates to the outgoing control flow edges of the given block
    ਖਾਲੀ ਸੌਂਪੋ_ਰਸਤਾ_Predicates(ਅਚਲ Cfg_ਖੰਡ& bb) {
        // Nothing to do for the exit block
        ਜੇ (bb.isExit()) ਵਾਪਸ;

        // Get the predicate of this block
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ predp = [&]() -> Dfg_ਸ਼ੀਰਸ਼* {
            // Entry block has no predecessors, use constant true
            ਜੇ (bb.isEnter()) ਵਾਪਸ make<DfgConst>(m_logicp->fileline(), 1U, 1U);

            // For any other block, 'or' together all the incoming predicates
            ਅਚਲ ਸਵੈਚਾਲਿਤ& ਵਿੱਚ_ਕਿਨਾਰੇ = bb.ਵਿੱਚ_ਕਿਨਾਰੇ();
            ਸਵੈਚਾਲਿਤ it = ਵਿੱਚ_ਕਿਨਾਰੇ.begin();
            Dfg_ਸ਼ੀਰਸ਼* resp = m_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ_Predicatep[static_cast<ਅਚਲ Cfg_ਕਿਨਾਰਾ&>(*it)];
            ਜਦੋਂ (++it != ਵਿੱਚ_ਕਿਨਾਰੇ.ਸਿਰਾ()) {
                DfgOr* ਅਚਲ orp = make<DfgOr>(resp->fileline(), resp->dtype());
                orp->rhsp(resp);
                orp->lhsp(m_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ_Predicatep[static_cast<ਅਚਲ Cfg_ਕਿਨਾਰਾ&>(*it)]);
                resp = orp;
            }
            ਵਾਪਸ resp;
        }();

        ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype = predp->dtype();

        ਅਚਲ ਸਵੈਚਾਲਿਤ mkTmp = [&](File_ਸਤਰ* flp, ਅਚਲ ਅੱਖਰ* name, Dfg_ਸ਼ੀਰਸ਼* srcp) {
            ਅਚਲ std::string ਅਗੇਤਰ = "_BB" + std::ਮੰਜ਼ਿਲ_string(bb.id()) + "_" + name;
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ tmpp = m_converter.createTmp(*m_logicp, flp, dtype, ਅਗੇਤਰ);
            tmpp->srcp(srcp);
            ਵਾਪਸ tmpp;
        };

        // Assign it to a variable in case it's used multiple times
        Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ pInp = mkTmp(predp->fileline(), "PathIn", predp);

        // For uncondional branches, the successor predicate edge is the same
        ਜੇ (!bb.isBranch()) {
            m_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ_Predicatep[bb.ਲਿਆ_Edgep()] = mkTmp(pInp->fileline(), "Goto", pInp);
            ਵਾਪਸ;
        }

        // For branches, we need to factor in the branch condition
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ condp = m_bb_ਮੰਜ਼ਿਲ_Condp[bb];
        File_ਸਤਰ* ਅਚਲ flp = condp->fileline();

        // Predicate for taken branch: 'predp & condp'
        {
            DfgAnd* ਅਚਲ ਲਿਆ_Predp = make<DfgAnd>(flp, dtype);
            ਲਿਆ_Predp->lhsp(pInp);
            ਲਿਆ_Predp->rhsp(condp);
            m_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ_Predicatep[bb.ਲਿਆ_Edgep()] = mkTmp(flp, "Taken", ਲਿਆ_Predp);
        }

        // Predicate for untaken branch: 'predp & ~condp'
        {
            DfgAnd* ਅਚਲ untknPredp = make<DfgAnd>(flp, dtype);
            untknPredp->lhsp(pInp);
            DfgNot* ਅਚਲ notp = make<DfgNot>(flp, dtype);
            notp->srcp(condp);
            untknPredp->rhsp(notp);
            m_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ_Predicatep[bb.untknEdgep()] = mkTmp(flp, "Untkn", untknPredp);
        }
    }

    // Returns true if all external updates to volatile variables are observed correctly
    ਤਾਰਕਿਕ checkExtWrites() {
        ਲਈ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_logicp->synth()) {
            ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = vtxp->cast<Dfg_ਸ਼ੀਰਸ਼_Var>();
            ਜੇ (!varp) ਜਾਰੀ;
            // If the variable we synthesized this vertex for is volatile, and
            // the value of the synthesized temporary is observed, we might be
            // missing an external update, so we mut give up.
            ਜੇ (!varp->hasSinks()) ਜਾਰੀ;
            ਜੇ (!Dfg_ਸ਼ੀਰਸ਼_Var::isVolatile(varp->tmpForp())) ਜਾਰੀ;
            ++m_ctx.m_synt.nonSynExtWrite;
            ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // Add the synthesized values as drivers to the output variables of the current DfgLogic
    ਤਾਰਕਿਕ ਜੋੜੋ_Synthesized_Output(SymTab& oSymTab) {
        // It's possible we think a variable is written by the DfgLogic when
        // it actauly isn't, e.g.: '{a[0], b[0]}[1] = ...' does not write 'b'.
        // These LHS forms can happen after some earlier tranforms. We
        // should just run V3Const on them earlier, but we will do belt and
        // braces and check here too. We can't touch any output variables if so.
        ਅਚਲ ਤਾਰਕਿਕ missing = m_logicp->foreachSink([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& sink) {
            ਅਚਲ DfgUnresolved* ਅਚਲ unresolvedp = sink.as<DfgUnresolved>();
            AstVarScope* ਅਚਲ vscp = unresolvedp->singleSink()->as<Dfg_ਸ਼ੀਰਸ਼_Var>()->vscp();
            ਵਾਪਸ !oSymTab.ਗਿਣਤੀ_2(vscp);
        });
        ਜੇ (missing) {
            ++m_ctx.m_synt.nonSynFalseWrite;
            ਵਾਪਸ ਝੂਠ;
        }

        // Add sinks to read the computed values for the target variables
        ਵਾਪਸ !m_logicp->foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& sink) {
            DfgUnresolved* ਅਚਲ unresolvedp = sink.as<DfgUnresolved>();
            ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = unresolvedp->singleSink()->as<Dfg_ਸ਼ੀਰਸ਼_Var>();
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ resp = oSymTab.at(varp->vscp());
            UASSERT_OBJ(resp->srcp(), resp, "Undriven result");

            // If the output is not used further in the synthesized logic itself,
            // then resp will be deleted before we return, so we can just use
            // its splice directly without ending up with a multi-use operation.
            ਜੇ (!resp->hasSinks()) {
                unresolvedp->ਜੋੜੋ_Driver(resp->srcp()->as<Dfg_ਸ਼ੀਰਸ਼_Splice>());
                ਵਾਪਸ ਝੂਠ;  // OK, continue.
            }

            // TODO: computePropagatedDrivers cannot handle arrays, should
            // never happen with simple continous assignments
            ਜੇ (!resp->isPacked()) {
                ++m_ctx.m_synt.nonSynArray;
                ਵਾਪਸ ਸੱਚ;  // Not OK, give up
            }

            // We need to add a new splice to avoid multi-use of the original splice
            DfgSplicePacked* ਅਚਲ splicep
                = ਨਵਾਂ DfgSplicePacked{m_dfg, resp->fileline(), resp->dtype()};
            // Drivers are the same
            ਅਚਲ std::ਸਦਿਸ਼<Driver> drivers = computePropagatedDrivers({}, resp);
            ਲਈ (ਅਚਲ Driver& d : drivers) splicep->ਜੋੜੋ_Driver(d.m_vtxp, d.m_lo, d.m_flp);
            unresolvedp->ਜੋੜੋ_Driver(splicep);
            ਵਾਪਸ ਝੂਠ;  // OK, continue
        });
    }

    // Synthesize the given AstAssignW. Returns true on success.
    ਤਾਰਕਿਕ synthesize_ਸੌਂਪੋ_W(Ast_ਸੌਂਪੋ_W* nodep) {
        ++m_ctx.m_synt.ਨਿਵੇਸ਼_ਸੌਂਪੋ;

        // The input and output symbol tables
        SymTab iSymTab;
        SymTab oSymTab;

        // Initialzie input symbol table
        initialize_ਇੰਦਰਾਜ_ਚਿੰਨ੍ਹ_ਸਾਰਣੀ(iSymTab);

        // Synthesize as if it was in a single CfgBlock CFG
        Dfg_ਸ਼ੀਰਸ਼* condp = ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ ਤਾਰਕਿਕ success = synthesize_Basic_ਖੰਡ(oSymTab, condp, {nodep}, iSymTab);
        UASSERT_OBJ(!condp, nodep, "Conditional AstAssignW ???");
        ਜੇ (!success) ਵਾਪਸ ਝੂਠ;

        // Check exernal writes are observed correctly
        ਜੇ (!checkExtWrites()) ਵਾਪਸ ਝੂਠ;

        // Add resolved output variable drivers
        ਵਾਪਸ ਜੋੜੋ_Synthesized_Output(oSymTab);
    }

    // Synthesize the given AstAlways. Returns true on success.
    ਤਾਰਕਿਕ synthesizeCfg(Cfg_ਗ੍ਰਾਫ& cfg) {
        ++m_ctx.m_synt.ਨਿਵੇਸ਼_Always;

        // If there is a backward edge (loop), we can't synthesize it
        ਜੇ (cfg.ਸ਼ਾਮਲ_ਹੈ_Loop()) {
            ++m_ctx.m_synt.nonSynLoop;
            ++m_ctx.m_synt.cfgCyclic;
            ਵਾਪਸ ਝੂਠ;
        }

        // If it's a trivial CFG we can save on some work
        ਜੇ (cfg.n_ਖੰਡਾਂ() == 1) {
            ++m_ctx.m_synt.cfgTrivial;
        } ਨਹੀਂਤਾਂ {
            // Insert two-way join blocks to aid multiplexer ordering
            ਜੇ (cfg.ਪਾਓ_Two_Way_Joins()) {
                ++m_ctx.m_synt.cfgSp;
            } ਨਹੀਂਤਾਂ {
                ++m_ctx.m_synt.cfgDag;
            }
            // Initialize maps needed for non-trivial CFGs
            m_dom_ਰੁੱਖ = Cfg_Dominator_ਰੁੱਖ{cfg};
            m_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ_Predicatep = cfg.make_ਕਿਨਾਰਾ_Map<Dfg_ਸ਼ੀਰਸ਼_Var*>();
        }

        // Initialize CfgMaps
        m_bb_ਮੰਜ਼ਿਲ_ISym_Tab = cfg.make_ਖੰਡ_Map<SymTab>();
        m_bb_ਮੰਜ਼ਿਲ_OSym_Tab = cfg.make_ਖੰਡ_Map<SymTab>();
        m_bb_ਮੰਜ਼ਿਲ_Condp = cfg.make_ਖੰਡ_Map<Dfg_ਸ਼ੀਰਸ਼_Var*>();

        // Synthesize all blocks
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : cfg.vertices()) {
            ਅਚਲ Cfg_ਖੰਡ& bb = static_cast<ਅਚਲ Cfg_ਖੰਡ&>(vtx);
            // Prepare the input symbol table of this block (enter, or join predecessor blocks)
            ਜੇ (!create_ਨਿਵੇਸ਼_ਚਿੰਨ੍ਹ_ਸਾਰਣੀ(bb)) ਵਾਪਸ ਝੂਠ;
            // Synthesize this block
            Dfg_ਸ਼ੀਰਸ਼* condp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (!synthesize_Basic_ਖੰਡ(m_bb_ਮੰਜ਼ਿਲ_OSym_Tab[bb], condp, bb.stmtps(), m_bb_ਮੰਜ਼ਿਲ_ISym_Tab[bb])) {
                ਵਾਪਸ ਝੂਠ;
            }
            // Create a temporary for the branch condition as it might be used multiple times
            ਜੇ (condp) {
                File_ਸਤਰ* ਅਚਲ flp = condp->fileline();
                ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype = condp->dtype();
                ਅਚਲ std::string ਅਗੇਤਰ = "_BB" + std::ਮੰਜ਼ਿਲ_string(bb.id()) + "_Cond";
                Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ vp = m_converter.createTmp(*m_logicp, flp, dtype, ਅਗੇਤਰ);
                vp->srcp(condp);
                m_bb_ਮੰਜ਼ਿਲ_Condp[bb] = vp;
            }
            // Set the path predicates on the successor edges
            ਸੌਂਪੋ_ਰਸਤਾ_Predicates(bb);
        }

        // Check exernal writes are observed correctly
        ਜੇ (!checkExtWrites()) ਵਾਪਸ ਝੂਠ;

        // Add resolved output variable drivers
        ਵਾਪਸ ਜੋੜੋ_Synthesized_Output(m_bb_ਮੰਜ਼ਿਲ_OSym_Tab[cfg.ਬਾਹਰ_ਨਿਕਲੋ()]);
    }

    // Synthesize a DfgLogic into regular vertices. Returns ture on success.
    ਤਾਰਕਿਕ synthesize(DfgLogic& vtx) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;

        ਜੇ (Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = VN_CAST(vtx.nodep()->stmtsp(), ਸੌਂਪੋ_W)) {
            ਜੇ (ap->nextp()) ਵਾਪਸ ਝੂਠ;
            ਜੇ (!synthesize_ਸੌਂਪੋ_W(ap)) ਵਾਪਸ ਝੂਠ;
            ++m_ctx.m_synt.synth_ਸੌਂਪੋ;
            ਵਾਪਸ ਸੱਚ;
        }

        ਜੇ (!synthesizeCfg(vtx.cfg())) ਵਾਪਸ ਝੂਠ;
        ++m_ctx.m_synt.synthAlways;
        ਵਾਪਸ ਸੱਚ;
    }

    // Revert all DfgLogic in m_toRevert, or DfgLogic driving the DfgUnresolved
    // vertices in m_toRevert, and transitively the same for any DfgUnresolved
    // driven by the reverted DfgLogic. Delete all DfgUnresolved involed.
    ਖਾਲੀ revert(VDouble0& statCountr) {
        m_ਮੰਜ਼ਿਲ_Revert.foreach([&](Dfg_ਸ਼ੀਰਸ਼& vtx) {
            // Process DfgLogic
            ਜੇ (DfgLogic* ਅਚਲ vtxp = vtx.cast<DfgLogic>()) {
                UASSERT_OBJ(vtxp->selectedForSynthesis(), vtxp, "Shouldn't reach here unselected");
                // Revert this logic
                UASSERT_OBJ(!vtxp->reverted(), vtxp, "Should be reverting now");
                ++statCountr;
                ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ p : vtxp->synth()) VL_DO_DANGLING(p->unlinkDelete(m_dfg), p);
                vtxp->synth().clear();
                vtxp->ਸੈੱਟ_Reverted();
                // Add all DfgUnresolved it drives to the work list
                vtxp->foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& snk) {
                    m_ਮੰਜ਼ਿਲ_Revert.ਧੱਕੋ_front(*snk.as<DfgUnresolved>());
                    ਵਾਪਸ ਝੂਠ;
                });
                ਵਾਪਸ;
            }

            // Process DfgUnresolved
            ਜੇ (DfgUnresolved* ਅਚਲ vtxp = vtx.cast<DfgUnresolved>()) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<Dfg_ਸ਼ੀਰਸ਼_Var>()->ਸੈੱਟ_Has_Mod_Wr_Refs();
                // Add all driving DfgLogic to the work list
                vtxp->foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& src) {
                    DfgLogic* ਅਚਲ lp = src.cast<DfgLogic>();
                    ਜੇ (lp && !lp->reverted()) m_ਮੰਜ਼ਿਲ_Revert.ਧੱਕੋ_front(*lp);
                    ਵਾਪਸ ਝੂਠ;
                });
                // Delete the DfgUnresolved driver
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
                ਵਾਪਸ;
            }

            // There should be nothing else on the worklist
            vtx.v3fatalSrc("Unexpected vertex type");
        });
    }

    // Synthesize all of the given vertices
    ਖਾਲੀ ਮੁੱਖ() {
        //-------------------------------------------------------------------
        UINFO(5, "Step 0: Remove all DfgLogic not selected for synthesis");
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgUnresolved
            ਜੇ (!vtxp->is<DfgUnresolved>()) ਜਾਰੀ;
            ਤਾਰਕਿਕ anySelected = ਝੂਠ;
            ਤਾਰਕਿਕ anyUnselected = ਝੂਠ;
            vtxp->foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& src) {
                ਅਚਲ DfgLogic& logic = *src.as<DfgLogic>();
                ਜੇ (logic.selectedForSynthesis()) {
                    anySelected = ਸੱਚ;
                } ਨਹੀਂਤਾਂ {
                    anyUnselected = ਸੱਚ;
                }
                ਵਾਪਸ ਝੂਠ;
            });
            // There should be a driver
            UASSERT_OBJ(anySelected || anyUnselected, vtxp, "'DfgUnresolved' with no driver");
            // All drivers should be selected or all should be unselected
            UASSERT_OBJ(!(anySelected && anyUnselected), vtxp, "Invalid 'DfgLogic' selection");
            // If all drivers are unselected, delete this DfgUnresolved here
            ਜੇ (anyUnselected) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<Dfg_ਸ਼ੀਰਸ਼_Var>()->ਸੈੱਟ_Has_Mod_Wr_Refs();
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgLogic
            DfgLogic* ਅਚਲ logicp = vtxp->cast<DfgLogic>();
            ਜੇ (!logicp) ਜਾਰੀ;
            ਜੇ (logicp->selectedForSynthesis()) ਜਾਰੀ;
            // There should be no sinks left for unselected DfgLogic, delete them here
            UASSERT_OBJ(!logicp->hasSinks(), vtxp, "Unselected 'DfgLogic' with sinks remaining");
            // Input variables will be read in Ast code, add Ast reference vertices
            // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
            ਅਚਲ VNUser4_ਵਿੱਚ_Use m_user4_ਵਿੱਚ_Use;
            logicp->foreach_ਸਰੋਤ([](Dfg_ਸ਼ੀਰਸ਼& src) {
                src.as<Dfg_ਸ਼ੀਰਸ਼_Var>()->vscp()->user4p(&src);
                ਵਾਪਸ ਝੂਠ;
            });
            V3DfgPasses::ਜੋੜੋ_Ast_Refs(m_dfg, logicp->nodep(), [](Ast_ਗੰਢ* varp) {  //
                ਵਾਪਸ varp->user4u().ਮੰਜ਼ਿਲ<Dfg_ਸ਼ੀਰਸ਼_Var*>();
            });
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        debugDump("synth-selected");

        //-------------------------------------------------------------------
        UINFO(5, "Step 1: Attempting to synthesize each of the selected DfgLogic");
        {
            // AstVarScope::user4() -> int: unique ID for 'AstVarScopeComparator'
            ਅਚਲ VNUser4_ਵਿੱਚ_Use user4_ਵਿੱਚ_Use;
            ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : m_dfg.opVertices()) {
                DfgLogic* ਅਚਲ logicp = vtx.cast<DfgLogic>();
                ਜੇ (!logicp) ਜਾਰੀ;

                // We should only have DfgLogic remaining that was selected for synthesis
                UASSERT_OBJ(logicp->selectedForSynthesis(), logicp, "Unselected DfgLogic remains");

                // Debug aid
                ਅਚਲ ਸਵੈਚਾਲਿਤ debugCallback = [&]() -> ਖਾਲੀ {
                    // This is the breaking logic
                    m_debugLogicp = logicp;
                    // Dump it
                    UINFOTREE(0, logicp->nodep(), "Problematic DfgLogic: " << logicp, "  ");
                    V3EmitV::debug_Verilog_For_ਰੁੱਖ(logicp->nodep(), std::ਬਾਹਰ);
                    debugDump("synth-lastok");
                };
                ਜੇ (VL_UNLIKELY(s_dfgSynthDebugBisect.stop(debugCallback))) ਰੋਕੋ;

                // Synthesize it, if failed, enqueue for reversion
                ਜੇ (!synthesize(*logicp)) {
                    logicp->ਸੈੱਟ_Non_Synthesizable();
                    m_ਮੰਜ਼ਿਲ_Revert.ਧੱਕੋ_front(*logicp);
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
        std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*> multidrivenps;
        // Map from variable to its resolved driver
        std::unordered_map<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var*, Dfg_ਸ਼ੀਰਸ਼_Splice*> resolvedDrivers;
        // Compute resolved drivers of all variablees
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var& var : m_dfg.varVertices()) {
            ਜੇ (!var.srcp()) ਜਾਰੀ;
            DfgUnresolved* ਅਚਲ unresolvedp = var.srcp()->cast<DfgUnresolved>();
            ਜੇ (!unresolvedp) ਰੋਕੋ;  // Stop when reached the synthesized temporaries

            // Resolve the synthesized drivers
            Dfg_ਸ਼ੀਰਸ਼_Splice* ਅਚਲ resolvedp = [&]() -> Dfg_ਸ਼ੀਰਸ਼_Splice* {
                // All synthesized drivers were normalized already,
                // so if there is only one, it can be used directly
                ਜੇ (unresolvedp->nInputs() == 1) {
                    ਵਾਪਸ unresolvedp->inputp(0)->as<Dfg_ਸ਼ੀਰਸ਼_Splice>();
                }
                // Otherwise gather the synthesized drivers
                std::ਸਦਿਸ਼<Driver> drivers = gatherDriversUnresolved(unresolvedp);
                // Normalize them, make resolved driver if all good
                ਜੇ (normalizeDrivers(var, drivers)) ਵਾਪਸ makeSplice(var, drivers);
                // If mutlidriven, record and ignore
                multidrivenps.emplace_ਪਿੱਠ(&var);
                m_ਮੰਜ਼ਿਲ_Revert.ਧੱਕੋ_front(*unresolvedp);
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            }();
            // Bail if multidriven
            ਜੇ (!resolvedp) ਜਾਰੀ;
            // Add to map for next loop
            ਅਚਲ ਤਾਰਕਿਕ new_ਇੰਦਰਾਜ = resolvedDrivers.emplace(&var, resolvedp).ਦੂਜਾ;
            UASSERT_OBJ(new_ਇੰਦਰਾਜ, &var, "Duplicate driver");
        }
        // Revert and remove drivers of multi-driven variables
        revert(m_ctx.m_synt.revertMultidrive);
        // Replace all DfgUnresolved with the resolved drivers
        ਲਈ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var& var : m_dfg.varVertices()) {
            ਜੇ (!var.srcp()) ਜਾਰੀ;
            DfgUnresolved* ਅਚਲ srcp = var.srcp()->cast<DfgUnresolved>();
            ਜੇ (!srcp) ਰੋਕੋ;  // Stop when reached the synthesized temporaries

            // Replace it
            srcp->replaceWith(resolvedDrivers.at(&var));
            VL_DO_DANGLING(srcp->unlinkDelete(m_dfg), srcp);
        }
        debugDump("synth-resolved");

        //-------------------------------------------------------------------
        UINFO(5, "Step 4: Remove all remaining DfgLogic and DfgUnresolved");
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_dfg.opVertices().unlinkable()) {
            // Previous step should have removed all DfgUnresolved
            UASSERT_OBJ(!vtxp->is<DfgUnresolved>(), vtxp, "DfgUnresolved remains");

            // Process only DfgLogic
            DfgLogic* ਅਚਲ logicp = vtxp->cast<DfgLogic>();
            ਜੇ (!logicp) ਜਾਰੀ;

            // Earlier pass should have removed all sinks
            UASSERT_OBJ(!logicp->hasSinks(), logicp, "DfgLogic sink remains");

            ਜੇ (!logicp->synth().ਖਾਲੀ_ਥਾਵਾਂ()) {
                // If synthesized, delete the corresponding AstNode. It is now in Dfg.
                logicp->nodep()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
            } ਨਹੀਂਤਾਂ {
                // Not synthesized. Logic stays in Ast. Add Ast reference vertices.
                // Outputs already marked by revertTransivelyAndRemove.
                // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
                ਅਚਲ VNUser4_ਵਿੱਚ_Use m_user4_ਵਿੱਚ_Use;
                logicp->foreach_ਸਰੋਤ([](Dfg_ਸ਼ੀਰਸ਼& src) {
                    src.as<Dfg_ਸ਼ੀਰਸ਼_Var>()->vscp()->user4p(&src);
                    ਵਾਪਸ ਝੂਠ;
                });
                V3DfgPasses::ਜੋੜੋ_Ast_Refs(m_dfg, logicp->nodep(), [](Ast_ਗੰਢ* varp) {  //
                    ਵਾਪਸ varp->user4u().ਮੰਜ਼ਿਲ<Dfg_ਸ਼ੀਰਸ਼_Var*>();
                });
            }

            // Delete this DfgLogic
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        // Reset the debug pointer, we have deleted it in the loop above ...
        m_debugLogicp = ਖਾਲੀ_ਸੂਚਕ;
        debugDump("synth-rmlogics");

        //-------------------------------------------------------------------
        UINFO(5, "Step 5: Remove unnecessary splices");
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_dfg.opVertices().unlinkable()) {
            Dfg_ਸ਼ੀਰਸ਼_Splice* ਅਚਲ splicep = vtxp->cast<Dfg_ਸ਼ੀਰਸ਼_Splice>();
            ਜੇ (!splicep) ਜਾਰੀ;

            // Might not have a sink if the driving logic was revered, remove
            ਜੇ (!splicep->hasSinks()) {
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
                ਜਾਰੀ;
            }

            // It should alway have drivers
            UASSERT_OBJ(splicep->nInputs(), splicep, "Splice with no drivers");

            // If redundant, remove it
            ਜੇ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ wholep = splicep->wholep()) {
                ਜੇ (Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = splicep->singleSink()->cast<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
                    varp->driver_File_ਸਤਰ(splicep->driver_File_ਸਤਰ(0));
                }
                splicep->replaceWith(wholep);
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
            }
        }
        debugDump("synth-rmsplice");
    }

    // CONSTRUCTOR
    Ast_ਮੰਜ਼ਿਲ_Dfg_Synthesize(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx}
        , m_converter{dfg, ctx} {
        ਮੁੱਖ();
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgSynthesisContext& ctx) {
        Ast_ਮੰਜ਼ਿਲ_Dfg_Synthesize{dfg, ctx};

        // Final step outside, as both AstToDfgSynthesize and removeUnused used DfgUserMap
        UINFO(5, "Step 6: Remove all unused vertices");
        V3DfgPasses::remove_ਅਣਵਰਤਿਆ(dfg);
        ਜੇ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("synth-rmunused");

        // No operation vertex should have multiple sinks. Cyclic decomoposition
        // depends on this and it can easily be ensured by using temporaries.
        // Also, all sources should be connected at this point
        ਜੇ (v3Global.opt.debugCheck()) {
            ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : dfg.opVertices()) {
                UASSERT_OBJ(!vtx.has_ਗੁਣਜ_Sinks(), &vtx, "Operation has multiple sinks");
                ਲਈ (ਮਾਪ_t i = 0; i < vtx.nInputs(); ++i) {
                    UASSERT_OBJ(vtx.inputp(i), &vtx, "Unconnected source operand");
                }
            }
            V3DfgPasses::typeCheck(dfg);
        }
    }
};

ਪੂਰਨ Ast_ਮੰਜ਼ਿਲ_Dfg_Synthesize::AstVarScopeComparator::s_vscpIdCounter = 0;

// Decide which DfgLogic to attempt to synthesize
ਸਥਿਰ ਖਾਲੀ dfgSelectLogicForSynthesis(Dfg_ਗ੍ਰਾਫ& dfg) {
    // If we are told to synthesize everything, we will do so ...
    ਜੇ (v3Global.opt.fDfgSynthesizeAll()) {
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : dfg.opVertices()) {
            ਜੇ (DfgLogic* ਅਚਲ logicp = vtx.cast<DfgLogic>()) logicp->ਸੈੱਟ_Selected_For_Synthesis();
        }
        ਵਾਪਸ;
    }

    // Otherwise figure out which vertices are likely worth synthesizing.

    std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*> circularVarps;
    // Synthesize all drivers of circular variables
    {
        DfgUserMap<uint64_t> scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, scc);
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var& var : dfg.varVertices()) {
            ਜੇ (!scc.at(var)) ਜਾਰੀ;
            // This is a circular variable
            circularVarps.emplace_ਪਿੱਠ(&var);
        }
    }

    // To cover all drivers, use a work list
    DfgWorklist worklist{dfg};

    // Synthesize all drivers of circular variables
    ਲਈ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp : circularVarps) {
        varp->srcp()->as<DfgUnresolved>()->foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& driver) {
            worklist.ਧੱਕੋ_front(*driver.as<DfgLogic>());
            ਵਾਪਸ ਝੂਠ;
        });
    }

    // Choose some simple special cases to always synthesize
    ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : dfg.opVertices()) {
        DfgLogic* ਅਚਲ logicp = vtx.cast<DfgLogic>();
        ਜੇ (!logicp || worklist.ਸ਼ਾਮਲ_ਹੈ(*logicp)) ਜਾਰੀ;
        // If drives an unused variable, synthesize it so the partial logic can be removed
        ਜੇ (logicp->drives_ਅਣਵਰਤਿਆ_Vars()) {
            worklist.ਧੱਕੋ_front(*logicp);
            ਜਾਰੀ;
        }
        // Blocks corresponding to continuous assignments
        ਜੇ (logicp->nodep()->keyword() == VAlwaysKwd::CONT_ਸੌਂਪੋ) {
            worklist.ਧੱਕੋ_front(*logicp);
            ਜਾਰੀ;
        }
        ਅਚਲ Cfg_ਗ੍ਰਾਫ& cfg = logicp->cfg();
        // Straight line code with no branches
        ਜੇ (cfg.n_ਖੰਡਾਂ() == 1) {
            worklist.ਧੱਕੋ_front(*logicp);
            ਜਾਰੀ;
        }
        // Blocks driving exactly 1 variable
        ਜੇ (!logicp->has_ਗੁਣਜ_Sinks()) worklist.ਧੱਕੋ_front(*logicp);
    }

    std::unordered_ਸੈੱਟ<DfgUnresolved*> is_ਦੇਖਿਆ;

    // Now expand to cover all logic driving the same set of variables and mark
    worklist.foreach([&](Dfg_ਸ਼ੀਰਸ਼& vtx) {
        DfgLogic& logic = *vtx.as<DfgLogic>();
        UASSERT_OBJ(!logic.selectedForSynthesis(), &vtx, "Should not be visited twice");
        // Mark as selected for synthesis
        logic.ਸੈੱਟ_Selected_For_Synthesis();
        // Enqueue all other logic driving the same variables as this one
        logic.foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& sink) {
            DfgUnresolved* unresolved = sink.as<DfgUnresolved>();
            ਜੇ (!is_ਦੇਖਿਆ.ਪਾਓ(unresolved).ਦੂਜਾ) ਵਾਪਸ ਝੂਠ;
            unresolved->foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& sibling) {
                DfgLogic& siblingLogic = *sibling.as<DfgLogic>();
                ਜੇ (!siblingLogic.selectedForSynthesis()) worklist.ਧੱਕੋ_front(siblingLogic);
                ਵਾਪਸ ਝੂਠ;
            });
            ਵਾਪਸ ਝੂਠ;
        });
    });
}

ਖਾਲੀ V3DfgPasses::synthesize(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgContext& ctx) {
    // Select which DfgLogic to attempt to synthesize
    dfgSelectLogicForSynthesis(dfg);
    // Synthesize them - also removes un-synthesized DfgLogic, so must run even if nothing selected
    Ast_ਮੰਜ਼ਿਲ_Dfg_Synthesize::apply(dfg, ctx.m_synthContext);
}
