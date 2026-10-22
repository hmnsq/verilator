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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Cfg.h"
#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3DfgPasses.h"
#అన్తర్భావ "V3EmitV.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <iterator>

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ {

// Create a DfgVertex out of a AstNodeExpr. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
మూస <రకం_పేరు T_శీర్షం, రకం_పేరు T_కణుపు>
T_శీర్షం* make_శీర్షం(స్థిరరాశి T_కణుపు* nodep, Dfg_లేఖాచిత్రం& dfg, స్థిరరాశి Dfg_దత్తాంశం_Type& dtype) {
    తిరిగి కొత్త T_శీర్షం{dfg, nodep->fileline(), dtype};
}

మూస <>
DfgArraySel* make_శీర్షం<DfgArraySel, AstArraySel>(స్థిరరాశి AstArraySel* nodep, Dfg_లేఖాచిత్రం& dfg,
                                                  స్థిరరాశి Dfg_దత్తాంశం_Type& dtype) {
    // Some earlier passes create malformed ArraySels, just bail on those...
    // See t_bitsel_wire_array_bad
    ఒకవేళ (VN_IS(nodep->fromp(), Const)) తిరిగి శూన్యసూచిక;
    ఒకవేళ (!VN_IS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) తిరిగి శూన్యసూచిక;
    తిరిగి కొత్త DfgArraySel{dfg, nodep->fileline(), dtype};
}

}  // namespace

// Visitor that can convert Ast statements and expressions in Dfg vertices
తరగతి Ast_గమ్యం_Dfg_Converter final : బహిరంగ VNVisitor {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // STATE
    Dfg_లేఖాచిత్రం& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats

    // Current logic vertex we are synthesizing
    DfgLogic* m_logicp = శూన్యసూచిక;
    // Variable updates produced by currently converted statement. This almost
    // always have a single element, so a vector is ok
    std::సదిశ<std::pair<AstVarScope*, Dfg_శీర్షం_Var*>>* m_updatesp = శూన్యసూచిక;

    తార్కిక m_దొరికింది_Unhandled = అబద్ధం;  // Found node not implemented as DFG or not implemented 'visit'
    తార్కిక m_converting = అబద్ధం;  // We are trying to convert some logic at the moment

    // METHODS

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    మూస <రకం_పేరు శీర్షం_2, రకం_పేరు... వాదనలు_2>
    శీర్షం_2* make(వాదనలు_2&&... వాదనలు) {
        static_assert(!std::is_పునాది_యొక్క<Dfg_శీర్షం_Var, శీర్షం_2>::విలువ, "Do not use for variables");
        static_assert(std::is_పునాది_యొక్క<Dfg_శీర్షం, శీర్షం_2>::విలువ, "'Vertex' must be a 'DfgVertex'");
        శీర్షం_2* స్థిరరాశి vtxp = కొత్త శీర్షం_2{m_dfg, std::forward<వాదనలు_2>(వాదనలు)...};
        m_logicp->synth().emplace_వెనుక(vtxp);
        తిరిగి vtxp;
    }

    // Returns true if the expression cannot (or should not) be represented by DFG
    తార్కిక unhandled(Ast_కణుపు_సమాసం* nodep) {
        // Short-circuiting if something was already unhandled
        ఒకవేళ (m_దొరికింది_Unhandled) {
            // Impure nodes cannot be represented
            ఒకవేళ (!nodep->is_శుద్ధ()) {
                m_దొరికింది_Unhandled = నిజం;
                ++m_ctx.m_conv.nonRepImpure;
            }
        }
        తిరిగి m_దొరికింది_Unhandled;
    }

    తార్కిక isSupported(స్థిరరాశి AstVarRef* nodep) {
        // Cannot represent cross module references
        ఒకవేళ (nodep->classOrPackagep()) తిరిగి అబద్ధం;
        // Check target
        తిరిగి V3Dfg::isSupported(nodep->varScopep());
    }

    // Given an RValue expression, return the equivalent Vertex, or nullptr if not representable.
    Dfg_శీర్షం* convertRValue(Ast_కణుపు_సమాసం* nodep) {
        UASSERT_OBJ(!m_converting, nodep, "'convertingRValue' should not be called recursively");
        VL_RESTORER(m_converting);
        VL_RESTORER(m_దొరికింది_Unhandled);
        m_converting = నిజం;
        m_దొరికింది_Unhandled = అబద్ధం;

        // Convert the expression
        iterate(nodep);

        // If falied to convert, return nullptr
        ఒకవేళ (m_దొరికింది_Unhandled) తిరిగి శూన్యసూచిక;

        // Traversal set user2p to the equivalent vertex
        Dfg_శీర్షం* స్థిరరాశి vtxp = nodep->user2u().గమ్యం<Dfg_శీర్షం*>();
        UASSERT_OBJ(vtxp, nodep, "Missing Dfg vertex after conversion");
        తిరిగి vtxp;
    }

    // Given an LValue expression, return the splice node that writes the
    // destination, together with the index to use for splicing in the value.
    // Returns {nullptr, 0}, if the given LValue expression is not supported.
    std::pair<Dfg_శీర్షం_Splice*, uint32_t> convertLValue(Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి vrefp = VN_CAST(nodep, VarRef)) {
            UASSERT_OBJ(vrefp->access().isWriteOnly(), vrefp, "Non-WriteOnly reference");
            ఒకవేళ (!isSupported(vrefp)) {
                ++m_ctx.m_conv.nonRepLValue;
                తిరిగి {శూన్యసూచిక, 0};
            }

            // Get (or create a new) temporary for this variable
            స్థిరరాశి Dfg_శీర్షం_Var* స్థిరరాశి vtxp = [&]() -> Dfg_శీర్షం_Var* {
                // The variable being assigned
                AstVarScope* స్థిరరాశి vscp = vrefp->varScopep();

                // Find existing one, if any
                కొరకు (స్థిరరాశి స్వయంచాలక& pair : *m_updatesp) {
                    ఒకవేళ (pair.మొదటి == vscp) తిరిగి pair.రెండవ;
                }

                // Create new one
                Dfg_శీర్షం_Var* స్థిరరాశి newp = createTmp(*m_logicp, vscp, "SynthAssign");
                m_updatesp->emplace_వెనుక(vscp, newp);

                // Create the Splice driver for the new temporary
                ఒకవేళ (newp->is<DfgVarPacked>()) {
                    newp->srcp(make<DfgSplicePacked>(newp->fileline(), newp->dtype()));
                } లేకపోతే ఒకవేళ (newp->is<DfgVarArray>()) {
                    newp->srcp(make<DfgSpliceArray>(newp->fileline(), newp->dtype()));
                } లేకపోతే {
                    nodep->v3fatalSrc("Unhandled DfgVertexVar sub-type");
                }

                // Use new temporary
                తిరిగి newp;
            }();

            // Return the Splice driver
            తిరిగి {vtxp->srcp()->as<Dfg_శీర్షం_Splice>(), 0};
        }

        ఒకవేళ (స్థిరరాశి AstSel* selp = VN_CAST(nodep, Sel)) {
            // Only handle constant selects
            స్థిరరాశి AstConst* స్థిరరాశి lsbp = VN_CAST(selp->lsbp(), Const);
            ఒకవేళ (!lsbp) {
                ++m_ctx.m_conv.nonRepLValue;
                తిరిగి {శూన్యసూచిక, 0};
            }
            uint32_t lsb = lsbp->గమ్యం_UInt();

            // Convert the 'fromp' sub-expression
            స్థిరరాశి స్వయంచాలక pair = convertLValue(selp->fromp());
            ఒకవేళ (!pair.మొదటి) తిరిగి {శూన్యసూచిక, 0};
            DfgSplicePacked* స్థిరరాశి splicep = pair.మొదటి->మూస as<DfgSplicePacked>();
            // Adjust index.
            lsb += pair.రెండవ;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            ఒకవేళ (lsb + static_cast<uint32_t>(selp->వెడల్పు_Const()) > splicep->కొలత()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                తిరిగి {శూన్యసూచిక, 0};
            }

            // AstSel doesn't change type kind (array vs packed), so we can use
            // the existing splice driver with adjusted lsb
            తిరిగి {splicep, lsb};
        }

        ఒకవేళ (స్థిరరాశి AstArraySel* స్థిరరాశి aselp = VN_CAST(nodep, ArraySel)) {
            // Only handle constant selects
            స్థిరరాశి AstConst* స్థిరరాశి indexp = VN_CAST(aselp->bitp(), Const);
            ఒకవేళ (!indexp) {
                ++m_ctx.m_conv.nonRepLValue;
                తిరిగి {శూన్యసూచిక, 0};
            }
            uint32_t సూచీ = indexp->గమ్యం_UInt();

            // Convert the 'fromp' sub-expression
            స్థిరరాశి స్వయంచాలక pair = convertLValue(aselp->fromp());
            ఒకవేళ (!pair.మొదటి) తిరిగి {శూన్యసూచిక, 0};
            DfgSpliceArray* స్థిరరాశి splicep = pair.మొదటి->మూస as<DfgSpliceArray>();
            // Adjust index. Note pair.second is always 0, but we might handle array slices later..
            సూచీ += pair.రెండవ;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            ఒకవేళ (సూచీ + 1U > splicep->కొలత()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                తిరిగి {శూన్యసూచిక, 0};
            }

            // Ensure the Splice driver exists for this element
            ఒకవేళ (!splicep->driverAt(సూచీ)) {
                File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
                // This should never fail
                స్థిరరాశి Dfg_దత్తాంశం_Type& dtype = *Dfg_దత్తాంశం_Type::fromAst(nodep->dtypep());
                ఒకవేళ (dtype.isPacked()) {
                    DfgSplicePacked* స్థిరరాశి newp = make<DfgSplicePacked>(flp, dtype);
                    స్థిరరాశి Dfg_దత్తాంశం_Type& uaDtype = Dfg_దత్తాంశం_Type::array(dtype, 1);
                    DfgUnitArray* స్థిరరాశి uap = make<DfgUnitArray>(flp, uaDtype);
                    uap->srcp(newp);
                    splicep->కూడు_Driver(uap, సూచీ, flp);
                } లేకపోతే ఒకవేళ (dtype.isArray()) {
                    DfgSpliceArray* స్థిరరాశి newp = make<DfgSpliceArray>(flp, dtype);
                    splicep->కూడు_Driver(newp, సూచీ, flp);
                } లేకపోతే {
                    nodep->v3fatalSrc("Unhandled data type kind");
                }
            }

            // Return the splice driver
            Dfg_శీర్షం* driverp = splicep->driverAt(సూచీ);
            ఒకవేళ (స్థిరరాశి DfgUnitArray* స్థిరరాశి uap = driverp->cast<DfgUnitArray>()) {
                driverp = uap->srcp();
            }
            తిరిగి {driverp->as<Dfg_శీర్షం_Splice>(), 0};
        }

        ++m_ctx.m_conv.nonRepLValue;
        తిరిగి {శూన్యసూచిక, 0};
    }

    // Given the LHS of an assignment, and the vertex representing the RHS,
    // connect up the RHS to drive the targets.
    // Returns true on success, false if the LHS is not representable.
    తార్కిక convertAssignment(File_పంక్తి* flp, Ast_కణుపు_సమాసం* lhsp, Dfg_శీర్షం* vtxp) {
        // Represents a DFG assignment contributed by the AST assignment with the above 'lhsp'.
        // There might be multiple of these if 'lhsp' is a concatenation.
        నిర్మాణం Assignment final {
            Dfg_శీర్షం_Splice* m_lhsp;  // Splice vertex being driven
            uint32_t m_idx;  // Position (bit or array index) within m_lhsp being driven
            Dfg_శీర్షం* m_rhsp;  // Vertex driving that position
            Assignment() = తొలగించు;
            Assignment(Dfg_శీర్షం_Splice* lhsp, uint32_t idx, Dfg_శీర్షం* rhsp)
                : m_lhsp{lhsp}
                , m_idx{idx}
                , m_rhsp{rhsp} {}
        };

        // Simplify the LHS, to get rid of things like SEL(CONCAT(_, _), _)
        ఒకవేళ (!VN_IS(lhsp, VarRef)) lhsp = VN_AS(V3Const::constifyExpensiveEdit(lhsp), కణుపు_సమాసం);

        // Assigning compound expressions to a concatenated LHS requires a temporary
        // to avoid multiple use of the expression
        ఒకవేళ (VN_IS(lhsp, Concat) && !vtxp->is<Dfg_శీర్షం_Var>() && !vtxp->is<DfgConst>()) {
            Dfg_శీర్షం_Var* స్థిరరాశి tmpp = createTmp(*m_logicp, flp, vtxp->dtype(), "Unpack");
            tmpp->srcp(vtxp);
            vtxp = tmpp;
        }

        // Convert each concatenation LHS separately, gather all assignments
        // we need to do into 'assignments', return true if all LValues
        // converted successfully.
        std::సదిశ<Assignment> assignments;
        స్థిరరాశి std::function<తార్కిక(Ast_కణుపు_సమాసం*, uint32_t)> convertAllLValues
            = [&](Ast_కణుపు_సమాసం* subp, uint32_t lsb) -> తార్కిక {
            // Concatenation on the LHS, convert each part
            ఒకవేళ (AstConcat* స్థిరరాశి concatp = VN_CAST(subp, Concat)) {
                Ast_కణుపు_సమాసం* స్థిరరాశి cRhsp = concatp->rhsp();
                Ast_కణుపు_సమాసం* స్థిరరాశి cLhsp = concatp->lhsp();
                // Convert Rigth of concat
                ఒకవేళ (!convertAllLValues(cRhsp, lsb)) తిరిగి అబద్ధం;
                // Convert Left of concat
                తిరిగి convertAllLValues(cLhsp, lsb + cRhsp->వెడల్పు());
            }

            // Non-concatenation, convert the LValue
            స్థిరరాశి స్వయంచాలక pair = convertLValue(subp);
            ఒకవేళ (!pair.మొదటి) తిరిగి అబద్ధం;

            // If whole lhs, just use it
            ఒకవేళ (subp == lhsp) {
                assignments.emplace_వెనుక(pair.మొదటి, pair.రెండవ, vtxp);
                తిరిగి నిజం;
            }

            // Otherwise select the relevant bits
            స్థిరరాశి Dfg_దత్తాంశం_Type& dtype = *Dfg_దత్తాంశం_Type::fromAst(subp->dtypep());
            DfgSel* స్థిరరాశి selp = make<DfgSel>(subp->fileline(), dtype);
            selp->fromp(vtxp);
            selp->lsb(lsb);
            assignments.emplace_వెనుక(pair.మొదటి, pair.రెండవ, selp);
            తిరిగి నిజం;
        };

        // Convert the given LHS assignment, give up if any LValues failed to convert
        ఒకవేళ (!convertAllLValues(lhsp, 0)) తిరిగి అబద్ధం;

        // All successful, connect the drivers
        కొరకు (స్థిరరాశి Assignment& అంశం_2 : assignments) {
            ఒకవేళ (DfgSplicePacked* స్థిరరాశి spp = అంశం_2.m_lhsp->మూస cast<DfgSplicePacked>()) {
                spp->కూడు_Driver(అంశం_2.m_rhsp, అంశం_2.m_idx, flp);
            } లేకపోతే ఒకవేళ (DfgSpliceArray* స్థిరరాశి sap = అంశం_2.m_lhsp->మూస cast<DfgSpliceArray>()) {
                // TODO: multi-dimensional arrays will need changes here
                స్థిరరాశి Dfg_దత్తాంశం_Type& rDt = అంశం_2.m_rhsp->dtype();
                ఒకవేళ (rDt.isPacked()) {
                    // RHS is assigning an element of this array. Need a DfgUnitArray adapter.
                    DfgUnitArray* స్థిరరాశి uap = make<DfgUnitArray>(flp, Dfg_దత్తాంశం_Type::array(rDt, 1));
                    uap->srcp(అంశం_2.m_rhsp);
                    sap->కూడు_Driver(uap, అంశం_2.m_idx, flp);
                } లేకపోతే {
                    // RHS is assigning an array (or array slice). Should be the same element type.
                    sap->కూడు_Driver(అంశం_2.m_rhsp, అంశం_2.m_idx, flp);
                }
            } లేకపోతే {
                అంశం_2.m_lhsp->v3fatalSrc("Unhandled DfgVertexSplice sub-type");
            }
        }

        తిరిగి నిజం;
    }

    // VISITORS

    // Unhandled node
    శూన్య సందర్శించు(Ast_కణుపు* /*nodep*/) override {
        ఒకవేళ (!m_దొరికింది_Unhandled && m_converting) ++m_ctx.m_conv.nonRepUnknown;
        m_దొరికింది_Unhandled = నిజం;
    }

    // Expressions - mostly auto generated, but a few special ones
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ఒకవేళ (unhandled(nodep)) తిరిగి;
        // This visit method is only called on RValues, where only read refs are supported
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Non-ReadOnly reference");
        ఒకవేళ (!isSupported(nodep)) {
            m_దొరికింది_Unhandled = నిజం;
            ++m_ctx.m_conv.nonRepVarRef;
            తిరిగి;
        }

        // Variable should have been bound before starting conversion
        Dfg_శీర్షం* స్థిరరాశి vtxp = nodep->varScopep()->user2u().మూస గమ్యం<Dfg_శీర్షం_Var*>();
        UASSERT_OBJ(vtxp, nodep, "Referenced variable has no associated DfgVertexVar");
        nodep->user2p(vtxp);
    }
    శూన్య సందర్శించు(AstConst* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ఒకవేళ (unhandled(nodep)) తిరిగి;

        ఒకవేళ (nodep->వెడల్పు() != nodep->అంకె().వెడల్పు()) {
            // Sometimes the width of the AstConst is not the same as the
            // V3Number it holds. Truncate it here. TODO: should this be allowed?
            V3_సంఖ్య అంకె{nodep, nodep->వెడల్పు()};
            అంకె.opSel(nodep->అంకె(), nodep->వెడల్పు() - 1, 0);
            Dfg_శీర్షం* స్థిరరాశి vtxp = make<DfgConst>(nodep->fileline(), అంకె);
            nodep->user2p(vtxp);
        } లేకపోతే {
            Dfg_శీర్షం* స్థిరరాశి vtxp = make<DfgConst>(nodep->fileline(), nodep->అంకె());
            nodep->user2p(vtxp);
        }
    }
    శూన్య సందర్శించు(AstCReset* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ఒకవేళ (unhandled(nodep)) తిరిగి;

        స్థిరరాశి Dfg_దత్తాంశం_Type* స్థిరరాశి dtypep = Dfg_దత్తాంశం_Type::fromAst(nodep->dtypep());
        ఒకవేళ (!dtypep) {
            m_దొరికింది_Unhandled = నిజం;
            ++m_ctx.m_conv.nonRepDType;
            తిరిగి;
        }

        UASSERT_OBJ(!nodep->constructing(), nodep,
                    "CReset should be non-constructing at this stage");

        Dfg_శీర్షం* స్థిరరాశి vtxp = make<DfgCReset>(nodep->fileline(), *dtypep);
        nodep->user2p(vtxp);
    }
    శూన్య సందర్శించు(Ast_సరిపోలిక_Masked* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ఒకవేళ (unhandled(nodep)) తిరిగి;

        స్థిరరాశి Dfg_దత్తాంశం_Type* స్థిరరాశి dtypep = Dfg_దత్తాంశం_Type::fromAst(nodep->dtypep());
        ఒకవేళ (!dtypep) {
            m_దొరికింది_Unhandled = నిజం;
            ++m_ctx.m_conv.nonRepDType;
            తిరిగి;
        }

        iterate(nodep->lhsp());
        ఒకవేళ (m_దొరికింది_Unhandled) తిరిగి;
        iterate(nodep->matchp());
        ఒకవేళ (m_దొరికింది_Unhandled) తిరిగి;

        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        Dfg_సరిపోలిక_Masked* స్థిరరాశి vtxp = make<Dfg_సరిపోలిక_Masked>(flp, *dtypep);
        vtxp->lhsp(nodep->lhsp()->user2u().గమ్యం<Dfg_శీర్షం*>());
        vtxp->matchp(nodep->matchp()->user2u().గమ్యం<Dfg_శీర్షం*>());
        nodep->user2p(vtxp);
    }
    శూన్య సందర్శించు(AstReplicate* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ఒకవేళ (unhandled(nodep)) తిరిగి;

        స్థిరరాశి Dfg_దత్తాంశం_Type* స్థిరరాశి dtypep = Dfg_దత్తాంశం_Type::fromAst(nodep->dtypep());
        ఒకవేళ (!dtypep) {
            m_దొరికింది_Unhandled = నిజం;
            ++m_ctx.m_conv.nonRepDType;
            తిరిగి;
        }

        iterate(nodep->srcp());
        ఒకవేళ (m_దొరికింది_Unhandled) తిరిగి;

        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        DfgRep* స్థిరరాశి vtxp = make<DfgRep>(flp, *dtypep);
        vtxp->srcp(nodep->srcp()->user2u().గమ్యం<Dfg_శీర్షం*>());
        nodep->user2p(vtxp);
    }
    శూన్య సందర్శించు(AstSel* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        ఒకవేళ (unhandled(nodep)) తిరిగి;

        స్థిరరాశి Dfg_దత్తాంశం_Type* స్థిరరాశి dtypep = Dfg_దత్తాంశం_Type::fromAst(nodep->dtypep());
        ఒకవేళ (!dtypep) {
            m_దొరికింది_Unhandled = నిజం;
            ++m_ctx.m_conv.nonRepDType;
            తిరిగి;
        }

        iterate(nodep->fromp());
        ఒకవేళ (m_దొరికింది_Unhandled) తిరిగి;

        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        Dfg_శీర్షం* vtxp = శూన్యసూచిక;
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->lsbp(), Const)) {
            స్థిరరాశి uint32_t lsb = constp->గమ్యం_UInt();
            స్థిరరాశి uint32_t msb = lsb + nodep->వెడల్పు_Const() - 1;
            Dfg_శీర్షం* స్థిరరాశి fromp = nodep->fromp()->user2u().గమ్యం<Dfg_శీర్షం*>();
            // Unfortunately we can still have out of bounds selects due to how
            // indices are truncated for speed reasons in V3Width/V3Unknown.
            ఒకవేళ (msb >= fromp->కొలత()) {
                m_దొరికింది_Unhandled = నిజం;
                ++m_ctx.m_conv.nonRepOOBSel;
                తిరిగి;
            }
            DfgSel* స్థిరరాశి selp = make<DfgSel>(flp, *dtypep);
            selp->fromp(fromp);
            selp->lsb(lsb);
            vtxp = selp;
        } లేకపోతే {
            iterate(nodep->lsbp());
            ఒకవేళ (m_దొరికింది_Unhandled) తిరిగి;
            DfgMux* స్థిరరాశి muxp = make<DfgMux>(flp, *dtypep);
            muxp->fromp(nodep->fromp()->user2u().గమ్యం<Dfg_శీర్షం*>());
            muxp->lsbp(nodep->lsbp()->user2u().గమ్యం<Dfg_శీర్షం*>());
            vtxp = muxp;
        }
        nodep->user2p(vtxp);
    }

// The rest of the visit methods for expressions are generated by 'astgen'
#అన్తర్భావ "V3Dfg__gen_ast_to_dfg.h"

బహిరంగ:
    // PUBLIC METHODS

    // Create temporay variable capable of holding the given type
    Dfg_శీర్షం_Var* createTmp(DfgLogic& logic, File_పంక్తి* flp, స్థిరరాశి Dfg_దత్తాంశం_Type& dtype,
                            స్థిరరాశి std::string& ఉపసర్గ) {
        Dfg_శీర్షం_Var* స్థిరరాశి vtxp = m_dfg.makeNewVar(flp, ఉపసర్గ, dtype, logic.scopep());
        logic.synth().emplace_వెనుక(vtxp);
        vtxp->vscp()->varp()->is_అంతర్గత(నిజం);
        vtxp->tmpForp(vtxp->vscp());
        తిరిగి vtxp;
    }

    // Create a new temporary variable capable of holding 'varp'
    Dfg_శీర్షం_Var* createTmp(DfgLogic& logic, AstVarScope* vscp, స్థిరరాశి std::string& ఉపసర్గ) {
        AstVar* స్థిరరాశి astVarp = vscp->varp();
        File_పంక్తి* స్థిరరాశి flp = astVarp->fileline();
        స్థిరరాశి Dfg_దత్తాంశం_Type& dtype = *Dfg_దత్తాంశం_Type::fromAst(astVarp->dtypep());
        స్థిరరాశి std::string prfx = ఉపసర్గ + "_" + astVarp->name();
        Dfg_శీర్షం_Var* స్థిరరాశి vtxp = createTmp(logic, flp, dtype, prfx);
        vtxp->tmpForp(vscp);
        తిరిగి vtxp;
    }

    // Convert AstAssign to Dfg, return true if successful.
    // Fills 'updates' with bindings for assigned variables.
    తార్కిక convert(std::సదిశ<std::pair<AstVarScope*, Dfg_శీర్షం_Var*>>& updates, DfgLogic& vtx,
                 Ast_కణుపు_కేటాయించు* nodep) {
        UASSERT_OBJ(VN_IS(nodep, కేటాయించు) || VN_IS(nodep, కేటాయించు_W), nodep, "Bad NodeAssign");
        UASSERT_OBJ(updates.ఖాళీలు(), nodep, "'updates' should be empty");
        VL_RESTORER(m_updatesp);
        VL_RESTORER(m_logicp);
        m_updatesp = &updates;
        m_logicp = &vtx;
        // Assignment with timing control shouldn't make it this far
        UASSERT_OBJ(!nodep->timingControlp(), nodep, "Shouldn't make it this far");
        // Convert it
        ++m_ctx.m_conv.ఆదానం_Assignments;
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp();
        // Check data types are compatible.
        స్థిరరాశి Dfg_దత్తాంశం_Type* స్థిరరాశి lDtypep = Dfg_దత్తాంశం_Type::fromAst(lhsp->dtypep());
        స్థిరరాశి Dfg_దత్తాంశం_Type* స్థిరరాశి rDtypep = Dfg_దత్తాంశం_Type::fromAst(rhsp->dtypep());
        ఒకవేళ (!lDtypep || !rDtypep) {
            ++m_ctx.m_conv.nonRepDType;
            తిరిగి అబద్ధం;
        }
        // For now, only direct array assignment is supported (e.g. a = b, but not a = _ ? b : c)
        ఒకవేళ (rDtypep->isArray()) {
            ఒకవేళ (!VN_IS(rhsp, VarRef)
                || !lhsp->dtypep()->skipRefp()->same_వృక్షం(rhsp->dtypep()->skipRefp())) {
                ++m_ctx.m_conv.nonRepDType;
                తిరిగి అబద్ధం;
            }
        }
        // Widths should match at this point
        UASSERT_OBJ(lhsp->వెడల్పు() == rhsp->వెడల్పు(), nodep, "Mismatched width reached DFG");
        // Convert the RHS expression
        Dfg_శీర్షం* స్థిరరాశి rVtxp = convertRValue(rhsp);
        ఒకవేళ (!rVtxp) తిరిగి అబద్ధం;
        // Connect the RHS vertex to the LHS targets
        స్థిరరాశి తార్కిక success = convertAssignment(nodep->fileline(), lhsp, rVtxp);
        ఒకవేళ (success) ++m_ctx.m_conv.representable;
        తిరిగి success;
    }

    // Convert RValue expression to Dfg. Returns nullptr if failed.
    Dfg_శీర్షం* convert(DfgLogic& vtx, Ast_కణుపు_సమాసం* nodep) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;
        // Convert it
        ++m_ctx.m_conv.ఆదానం_Expressions;
        Dfg_శీర్షం* స్థిరరాశి vtxp = convertRValue(nodep);
        ఒకవేళ (vtxp) ++m_ctx.m_conv.representable;
        తిరిగి vtxp;
    }

    // CONSTRUCTOR
    Ast_గమ్యం_Dfg_Converter(Dfg_లేఖాచిత్రం& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {}
};

// Debug aid - outisde 'AstToDfgSynthesize' as it is a template, but want one instance
V3DebugBisect s_dfgSynthDebugBisect{"DfgSynthesize"};

తరగతి Ast_గమ్యం_Dfg_Synthesize final {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // TYPES

    // SymTab must be ordered in order to yield stable results
    నిర్మాణం AstVarScopeComparator final {
        స్థిర పూర్ణాంకం s_vscpIdCounter;  // Counter for lazily allocating the unique AstVarScope IDs
        తార్కిక క్రియాకారకం()(AstVarScope* lhs, AstVarScope* rhs) స్థిరరాశి {
            ఒకవేళ (!lhs->user4()) lhs->user4(++s_vscpIdCounter);
            ఒకవేళ (!rhs->user4()) rhs->user4(++s_vscpIdCounter);
            తిరిగి lhs->user4() < rhs->user4();
        }
    };
    ఉపయోగించి SymTab = std::map<AstVarScope*, Dfg_శీర్షం_Var*, AstVarScopeComparator>;

    // Represents a [potentially partial] driver of a variable
    నిర్మాణం Driver final {
        Dfg_శీర్షం* m_vtxp = శూన్యసూచిక;  // Driving vertex
        uint32_t m_lo = 0;  // Low index of driven range (internal, not Verilog)
        uint32_t m_hi = 0;  // High index of driven range (internal, not Verilog)
        File_పంక్తి* m_flp = శూన్యసూచిక;  // Location of driver in source

        Driver() = అప్రమేయ;
        Driver(Dfg_శీర్షం* vtxp, uint32_t lo, File_పంక్తి* flp)
            : m_vtxp{vtxp}
            , m_lo{lo}
            , m_hi{lo + vtxp->కొలత() - 1U}
            , m_flp{flp} {}
        క్రియాకారకం తార్కిక() స్థిరరాశి { తిరిగి m_vtxp != శూన్యసూచిక; }

        తార్కిక క్రియాకారకం<(స్థిరరాశి Driver& other) స్థిరరాశి {
            ఒకవేళ (m_lo != other.m_lo) తిరిగి m_lo < other.m_lo;
            ఒకవేళ (m_hi != other.m_hi) తిరిగి m_hi < other.m_hi;
            తిరిగి m_flp->operatorCompare(*other.m_flp) < 0;
        }

        తార్కిక క్రియాకారకం<=(స్థిరరాశి Driver& other) స్థిరరాశి { తిరిగి !(other < *ఇది); }
    };

    // STATE - Persistent
    Dfg_లేఖాచిత్రం& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats
    Ast_గమ్యం_Dfg_Converter m_converter;  // The convert instance to use for each construct
    DfgWorklist m_గమ్యం_Revert{m_dfg};  // We need a worklist for reverting synthesis

    // STATE - for current DfgLogic being synthesized
    DfgLogic* m_logicp = శూన్యసూచిక;  // Current logic vertex we are synthesizing
    Cfg_ఖండం_Map<SymTab> m_bb_గమ్యం_ISym_Tab;  // Map from CfgBlock -> input symbol table
    Cfg_ఖండం_Map<SymTab> m_bb_గమ్యం_OSym_Tab;  // Map from CfgBlock -> output symbol table
    Cfg_ఖండం_Map<Dfg_శీర్షం_Var*> m_bb_గమ్యం_Condp;  // Map from CfgBlock ->  terminating branch condition
    Cfg_అంచు_Map<Dfg_శీర్షం_Var*> m_అంచు_గమ్యం_Predicatep;  // Map CfgGraphEdge -> path predicate to there
    Cfg_Dominator_వృక్షం m_dom_వృక్షం;  // The dominator tree of the current CFG

    // STATE - Some debug aid
    // We stop after synthesizing s_dfgSynthDebugLimit vertices (if non-zero).
    // This is the problematic logic (last one we synthesize), assuming a
    // bisection search over s_dfgSynthDebugLimit.
    DfgLogic* m_debugLogicp = శూన్యసూచిక;
    // Source (upstream) cone of outputs of m_debugLogicp
    std::unique_ptr<std::unordered_సమితి<స్థిరరాశి Dfg_శీర్షం*>> m_debugOSrcConep{శూన్యసూచిక};

    // METHODS

    // Dump current graph for debugging ...
    శూన్య debugDump(స్థిరరాశి అక్షరం* name) {
        // If we have the debugged logic, compute the vertices feeding its outputs
        ఒకవేళ (VL_UNLIKELY(m_debugLogicp)) {
            std::సదిశ<స్థిరరాశి Dfg_శీర్షం*> outputs;
            m_debugLogicp->foreachSink([&outputs](స్థిరరాశి Dfg_శీర్షం& v) {
                outputs.emplace_వెనుక(v.singleSink()->as<Dfg_శీర్షం_Var>());
                తిరిగి అబద్ధం;
            });
            m_debugOSrcConep = m_dfg.ఆధారం_Cone(outputs);
        }

        ఒకవేళ (VL_UNLIKELY(dumpDfgLevel() >= 9 || m_debugOSrcConep)) {
            m_dfg.dumpDotFilePrefixed(name);
            ఒకవేళ (m_debugOSrcConep) {
                // Dump only the subgraph involving the inputs and outputs of the bad vertex
                m_dfg.dumpDotFilePrefixed(name + "-min"s, [&](స్థిరరాశి Dfg_శీర్షం& v) -> తార్కిక {
                    తిరిగి m_debugOSrcConep->లెక్క(&v);
                });
            }
        }
    }

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    మూస <రకం_పేరు శీర్షం_2, రకం_పేరు... వాదనలు_2>
    శీర్షం_2* make(వాదనలు_2&&... వాదనలు) {
        static_assert(!std::is_పునాది_యొక్క<Dfg_శీర్షం_Var, శీర్షం_2>::విలువ, "Do not use for variables");
        static_assert(std::is_పునాది_యొక్క<Dfg_శీర్షం, శీర్షం_2>::విలువ, "'Vertex' must be a 'DfgVertex'");
        శీర్షం_2* స్థిరరాశి vtxp = కొత్త శీర్షం_2{m_dfg, std::forward<వాదనలు_2>(వాదనలు)...};
        ఒకవేళ (m_logicp) m_logicp->synth().emplace_వెనుక(vtxp);
        తిరిగి vtxp;
    }

    // Gather all drivers of a resolved variable
    స్థిర std::సదిశ<Driver> gatherDrivers(Dfg_శీర్షం_Splice* vtxp) {
        // Collect them all, check if they are sorted
        std::సదిశ<Driver> drivers;
        drivers.reserve(vtxp->nInputs());
        తార్కిక sorted = నిజం;
        vtxp->foreachDriver([&](Dfg_శీర్షం& src, uint32_t lo, File_పంక్తి* flp) {
            // Collect the driver
            drivers.emplace_వెనుక(&src, lo, flp);
            // Check if drivers are sorted - most often they are
            స్థిరరాశి కొలత_t n = drivers.కొలత();
            ఒకవేళ (n >= 2 && drivers[n - 1] < drivers[n - 2]) sorted = అబద్ధం;
            తిరిగి అబద్ధం;
        });

        // Sort if unsorted
        ఒకవేళ (!sorted) std::stable_క్రమీకరించు(drivers.begin(), drivers.చివర());

        //పూర్తి
        తిరిగి drivers;
    }

    // Returns true if the driver supplies a complete array element.
    స్థిర తార్కిక driver_Covers_సంపూర్ణ_Element(స్థిరరాశి Driver& driver) {
        స్థిరరాశి DfgUnitArray* స్థిరరాశి unitp = driver.m_vtxp->cast<DfgUnitArray>();
        ఒకవేళ (!unitp) తిరిగి అబద్ధం;
        ఒకవేళ (Dfg_శీర్షం_Splice* స్థిరరాశి splicep = unitp->srcp()->cast<Dfg_శీర్షం_Splice>()) {
            తిరిగి splicep->wholep();
        }
        తిరిగి నిజం;
    }

    // Unlike 'wholep', several element drivers can collectively cover an array.
    స్థిర తార్కిక splice_Covers_సంపూర్ణ(Dfg_శీర్షం_Splice* స్థిరరాశి splicep) {
        ఒకవేళ (splicep->wholep()) తిరిగి నిజం;
        ఒకవేళ (splicep->isPacked()) తిరిగి అబద్ధం;
        uint32_t తదుపరి = 0;
        కొరకు (స్థిరరాశి Driver& driver : gatherDrivers(splicep)) {
            ఒకవేళ (driver.m_lo != తదుపరి || !driver_Covers_సంపూర్ణ_Element(driver)) తిరిగి అబద్ధం;
            తదుపరి = driver.m_hi + 1;
        }
        తిరిగి తదుపరి == splicep->కొలత();
    }

    // Returns true if the driver cone contains any variable introduced by
    // tristate lowering. Used to distinguish intentional tristate contributor
    // overlap from accidental multidrive.
    స్థిర తార్కిక కలిగియుంది_Tri_Lowered_Var(Dfg_శీర్షం* rootp) {
        std::సదిశ<స్థిరరాశి Dfg_శీర్షం*> దొంతర;
        std::సదిశ<స్థిరరాశి Dfg_శీర్షం*> సందర్శించిన;
        దొంతర.emplace_వెనుక(rootp);
        అయితే (!దొంతర.ఖాళీలు()) {
            స్థిరరాశి Dfg_శీర్షం* స్థిరరాశి vtxp = దొంతర.వెనుక();
            దొంతర.తీయి_వెనుక();
            ఒకవేళ (std::కనుగొను(సందర్శించిన.begin(), సందర్శించిన.చివర(), vtxp) != సందర్శించిన.చివర()) కొనసాగు;
            సందర్శించిన.emplace_వెనుక(vtxp);
            ఒకవేళ (స్థిరరాశి Dfg_శీర్షం_Var* స్థిరరాశి varp = vtxp->cast<Dfg_శీర్షం_Var>()) {
                ఒకవేళ (varp->vscp()->varp()->dfgTriLowered()) తిరిగి నిజం;
            }
            vtxp->foreach_ఆధారం([&](స్థిరరాశి Dfg_శీర్షం& src) {
                దొంతర.emplace_వెనుక(&src);
                తిరిగి అబద్ధం;
            });
        }
        తిరిగి అబద్ధం;
    }

    // Returns true if the driver is a direct variable forward (no logic).
    స్థిర తార్కిక is_ప్రత్యక్ష_Var_Driver(స్థిరరాశి Dfg_శీర్షం* vtxp) { తిరిగి vtxp->is<Dfg_శీర్షం_Var>(); }

    // Gather all synthesized drivers of an unresolved variable
    స్థిర std::సదిశ<Driver> gatherDriversUnresolved(DfgUnresolved* vtxp) {
        std::సదిశ<Driver> drivers;
        drivers.reserve(vtxp->nInputs());

        // For better locations in error reporting, we unpick concatenations
        // which are sometimes introduced by combinint assignments in V3Const.
        స్థిరరాశి std::function<శూన్య(Dfg_శీర్షం*, uint32_t, File_పంక్తి*)> gather
            = [&](Dfg_శీర్షం* vtxp, uint32_t lo, File_పంక్తి* flp) -> శూన్య {
            ఒకవేళ (DfgConcat* స్థిరరాశి concatp = vtxp->cast<DfgConcat>()) {
                Dfg_శీర్షం* స్థిరరాశి rhsp = concatp->rhsp();
                gather(rhsp, lo, rhsp->fileline());
                Dfg_శీర్షం* స్థిరరాశి lhsp = concatp->lhsp();
                gather(lhsp, lo + rhsp->వెడల్పు(), lhsp->fileline());
                తిరిగి;
            }
            drivers.emplace_వెనుక(vtxp, lo, flp);
        };

        // Gather all synthesized drivers
        vtxp->foreach_ఆధారం([&](Dfg_శీర్షం& src) {
            // Can ignore the original DfgLogic
            ఒకవేళ (src.is<DfgLogic>()) తిరిగి అబద్ధం;
            // Synthesized drivers must be a splice at this point
            Dfg_శీర్షం_Splice* స్థిరరాశి splicep = src.as<Dfg_శీర్షం_Splice>();
            // Collect the driver
            splicep->foreachDriver([&](Dfg_శీర్షం& src, uint32_t lo, File_పంక్తి* flp) {
                gather(&src, lo, flp);
                తిరిగి అబద్ధం;
            });
            తిరిగి అబద్ధం;
        });

        // Sort the drivers
        std::stable_క్రమీకరించు(drivers.begin(), drivers.చివర());

        //పూర్తి
        తిరిగి drivers;
    }

    // Given two drivers, combine the driven sub-ranges into the first one if
    // possible. First bool returned indicates successfully combined and there
    // are no multi-driven bits. Second bool returned indicates we warned
    // already about multi-driven bits.
    std::pair<తార్కిక, తార్కిక> combineDrivers(Dfg_శీర్షం_Var& var, స్థిరరాశి std::string& sub,  //
                                         Driver& a, స్థిరరాశి Driver& b) {
        // We can only combine array drivers ...
        ఒకవేళ (a.m_vtxp->isPacked()) తిరిగి {అబద్ధం, అబద్ధం};
        // ... that both drive a single element ...
        ఒకవేళ (a.m_lo != b.m_lo) తిరిగి {అబద్ధం, అబద్ధం};
        స్థిరరాశి DfgUnitArray* స్థిరరాశి aUap = a.m_vtxp->మూస cast<DfgUnitArray>();
        ఒకవేళ (!aUap) తిరిగి {అబద్ధం, అబద్ధం};
        స్థిరరాశి DfgUnitArray* స్థిరరాశి bUap = b.m_vtxp->మూస cast<DfgUnitArray>();
        ఒకవేళ (!bUap) తిరిగి {అబద్ధం, అబద్ధం};
        // ... and are themeselves partial
        DfgSplicePacked* స్థిరరాశి aSp = aUap->srcp()->మూస cast<DfgSplicePacked>();
        ఒకవేళ (!aSp) తిరిగి {అబద్ధం, అబద్ధం};
        DfgSplicePacked* స్థిరరాశి bSp = bUap->srcp()->మూస cast<DfgSplicePacked>();
        ఒకవేళ (!bSp) తిరిగి {అబద్ధం, అబద్ధం};
        UASSERT_OBJ(aSp->dtype() == bSp->dtype(), &var, "DTypes should match");

        // Gather drivers of a
        std::సదిశ<Driver> aDrivers = gatherDrivers(aSp);
        // Gather drivers of b
        std::సదిశ<Driver> bDrivers = gatherDrivers(bSp);

        // Merge them
        std::సదిశ<Driver> abDrivers;
        abDrivers.reserve(aDrivers.కొలత() + bDrivers.కొలత());
        std::విలీనం(  //
            aDrivers.begin(), aDrivers.చివర(),  //
            bDrivers.begin(), bDrivers.చివర(),  //
            std::వెనుక_inserter(abDrivers)  //
        );

        // Attempt to resolve them
        ఒకవేళ (!normalizeDrivers(var, abDrivers, sub + "[" + std::గమ్యం_string(a.m_lo) + "]")) {
            తిరిగి {అబద్ధం, నిజం};
        }

        // Successfully resolved. Needs a new splice and unit.
        File_పంక్తి* స్థిరరాశి flp = var.fileline();
        DfgSplicePacked* స్థిరరాశి splicep = make<DfgSplicePacked>(flp, aSp->dtype());
        కొరకు (స్థిరరాశి Driver& d : abDrivers) splicep->కూడు_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        DfgUnitArray* స్థిరరాశి uap = make<DfgUnitArray>(flp, aUap->dtype());
        uap->srcp(splicep);
        a.m_vtxp = uap;
        తిరిగి {నిజం, అబద్ధం};
    }

    // Combine and coalesce the given drivers.
    // Returns true iff no multi-driven bits are present.
    తార్కిక normalizeDrivers(Dfg_శీర్షం_Var& var, std::సదిశ<Driver>& drivers,
                          స్థిరరాశి std::string& sub = "") {
        ఒకవేళ (drivers.ఖాళీలు()) తిరిగి నిజం;

        // What type of values are we combining
        స్థిరరాశి తార్కిక isPacked = drivers[0].m_vtxp->isPacked();

        // Found a multidriven part ?
        తార్కిక multiDriven = అబద్ధం;

        // Iterate through the sorted drivers. Index 'i' is the driver we are
        // resolving driver 'j' agains, and if required, we merge 'j' into 'i'.
        కొలత_t i = 0;
        కొరకు (కొలత_t j = 1; j < drivers.కొలత();) {
            UASSERT_OBJ(i < j, &var, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            ఒకవేళ (!jD) {
                ++j;
                కొనసాగు;
            }
            // If 'i' was moved, move 'j' in it's place
            ఒకవేళ (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                కొనసాగు;
            }

            // We have 2 valid drivers now
            UASSERT_OBJ(iD <= jD, &var, "Should always be sorted");
            UASSERT_OBJ(jD.m_vtxp->isPacked() == isPacked, &var, "Mixed type drivers");

            // If no overlap, consider next pair
            ఒకవేళ (iD.m_hi < jD.m_lo) {
                ++i;
                ఒకవేళ (i == j) ++j;
                కొనసాగు;
            }

            // There is an overlap. Attempt to combine them.
            తార్కిక combined = అబద్ధం;
            తార్కిక warned = అబద్ధం;
            std::tie(combined, warned) = combineDrivers(var, sub, iD, jD);

            // If sucessfully combined, 'j' is no longer needed, it was combined into 'i'
            ఒకవేళ (combined) {
                jD = Driver{};
                ++j;
                కొనసాగు;
            }

            // Found overlap that cannot be resolved
            multiDriven = నిజం;
            // Compare next driver
            ++j;

            // Do not warn again if we warned during 'combineDrivers'
            ఒకవేళ (warned) కొనసాగు;

            // The variable to warn on
            AstVarScope* స్థిరరాశి vscp = var.tmpForp() ? var.tmpForp() : var.vscp();

            // Loop index often abused, so suppress
            ఒకవేళ (vscp->varp()->is_వాడిన_Loop_Idx()) కొనసాగు;
            // Tristate lowering can intentionally create overlapping contributors.
            // Keep the signal marked multidriven for DFG fallback, but suppress
            // warning only when both overlapping driver cones look tri-lowered.
            ఒకవేళ (vscp->varp()->dfgAllowMultidriveTri()) {
                స్థిరరాశి తార్కిక iTri = కలిగియుంది_Tri_Lowered_Var(iD.m_vtxp);
                స్థిరరాశి తార్కిక jTri = కలిగియుంది_Tri_Lowered_Var(jD.m_vtxp);
                స్థిరరాశి తార్కిక triPair = iTri && jTri;
                స్థిరరాశి తార్కిక triAndBridge = (iTri && is_ప్రత్యక్ష_Var_Driver(jD.m_vtxp))
                                          || (jTri && is_ప్రత్యక్ష_Var_Driver(iD.m_vtxp));
                ఒకవేళ (triPair || triAndBridge) కొనసాగు;
            }

            // Warn the user now
            స్థిరరాశి std::string lo = std::గమ్యం_string(jD.m_lo);
            స్థిరరాశి std::string hi = std::గమ్యం_string(std::min(iD.m_hi, jD.m_hi));
            స్థిరరాశి std::string రకం = isPacked ? "Bit" : "Element";
            స్థిరరాశి std::string part = hi == lo ? (" [" + lo + "]") : ("s [" + hi + ":" + lo + "]");

            vscp->v3warn(  //
                MULTIDRIVEN,  //
                రకం << part << " of signal '" << vscp->prettyName() << sub << "'"
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
        ఒకవేళ (!multiDriven && isPacked) coalesceDrivers(drivers);

        తిరిగి !multiDriven;
    }

    // Coalesce adjacent drivers into single ones
    శూన్య coalesceDrivers(std::సదిశ<Driver>& drivers) {
        UASSERT(!drivers.ఖాళీలు(), "Can't coalesce 0 drivers");
        UASSERT_OBJ(drivers[0].m_vtxp->isPacked(), drivers[0].m_vtxp, "Can only coalesce packed");

        కొలత_t i = 0;
        కొరకు (కొలత_t j = 1; j < drivers.కొలత();) {
            UASSERT(i < j, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            ఒకవేళ (!jD) {
                ++j;
                కొనసాగు;
            }
            // If 'i' was moved, move 'j' in it's place
            ఒకవేళ (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                కొనసాగు;
            }

            // We have 2 valid drivers now
            UASSERT(iD <= jD, "Should always be sorted");

            // If not adjacent, move on
            ఒకవేళ (iD.m_hi + 1 != jD.m_lo) {
                ++i;
                ఒకవేళ (i == j) ++j;
                కొనసాగు;
            }

            // Coalesce Adjacent ranges,
            స్థిరరాశి Dfg_దత్తాంశం_Type& dt = Dfg_దత్తాంశం_Type::packed(iD.m_vtxp->వెడల్పు() + jD.m_vtxp->వెడల్పు());
            DfgConcat* స్థిరరాశి concatp = make<DfgConcat>(iD.m_flp, dt);
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
    Dfg_శీర్షం_Splice* makeSplice(Dfg_శీర్షం_Var& var, స్థిరరాశి std::సదిశ<Driver>& newDrivers) {
        UASSERT_OBJ(!newDrivers.ఖాళీలు(), &var, "'makeSplice' called with no new drivers");
        // Create new driver
        Dfg_శీర్షం_Splice* splicep = శూన్యసూచిక;
        ఒకవేళ (var.is<DfgVarPacked>()) {
            splicep = make<DfgSplicePacked>(var.fileline(), var.dtype());
        } లేకపోతే ఒకవేళ (var.is<DfgVarArray>()) {
            splicep = make<DfgSpliceArray>(var.fileline(), var.dtype());
        } లేకపోతే {
            var.v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        కొరకు (స్థిరరాశి Driver& d : newDrivers) splicep->కూడు_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        తిరిగి splicep;
    }

    // Initialzie input symbol table of entry CfgBlock
    శూన్య initialize_నమోదు_చిహ్నం_పట్టిక(SymTab& iSymTab) {
        m_logicp->foreach_ఆధారం([&](Dfg_శీర్షం& src) {
            Dfg_శీర్షం_Var* స్థిరరాశి vvp = src.as<Dfg_శీర్షం_Var>();
            iSymTab[vvp->vscp()] = vvp;
            తిరిగి అబద్ధం;
        });
    }

    // Join variable drivers across a control flow confluence (insert muxes ...)
    Dfg_శీర్షం_Var* joinDrivers(AstVarScope* vscp, Dfg_శీర్షం_Var* predicatep,  //
                              Dfg_శీర్షం_Var* thenp, Dfg_శీర్షం_Var* elsep) {
        AstVarScope* స్థిరరాశి thenVscp = thenp->tmpForp() ? thenp->tmpForp() : thenp->vscp();
        AstVarScope* స్థిరరాశి elseVscp = elsep->tmpForp() ? elsep->tmpForp() : elsep->vscp();
        UASSERT_OBJ(thenVscp == elseVscp, vscp, "Attempting to join unrelated variables");

        // If both bindings are the the same (variable not updated through either path),
        // then there is nothing to do, can use the same binding
        ఒకవేళ (thenp == elsep) తిరిగి thenp;

        // We can't join the input variable just yet, so bail
        ఒకవేళ (thenp->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_ఆదానం;
            తిరిగి శూన్యసూచిక;
        }
        ఒకవేళ (elsep->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_ఆదానం;
            తిరిగి శూన్యసూచిక;
        }

        // Can't do arrays yet
        ఒకవేళ (!thenp->isPacked()) {
            ++m_ctx.m_synt.nonSynArray;
            తిరిగి శూన్యసూచిక;
        }

        // Gather drivers of 'thenp' - only if 'thenp' is not an input to the synthesized block
        Dfg_శీర్షం* స్థిరరాశి tDefaultp = thenp->defaultp();
        std::సదిశ<Driver> tDrivers = gatherDrivers(thenp->srcp()->as<Dfg_శీర్షం_Splice>());

        // Gather drivers of 'elsep' - only if 'thenp' is not an input to the synthesized block
        Dfg_శీర్షం* స్థిరరాశి eDefaultp = elsep->defaultp();
        std::సదిశ<Driver> eDrivers = gatherDrivers(elsep->srcp()->as<Dfg_శీర్షం_Splice>());

        // Default drivers should be the same or not present on either
        UASSERT_OBJ(tDefaultp == eDefaultp, vscp, "Different default drivers");

        // Location to use for the join vertices
        File_పంక్తి* స్థిరరాశి flp = predicatep->fileline();

        // Create a fresh temporary for the joined value
        Dfg_శీర్షం_Var* స్థిరరాశి joinp = m_converter.createTmp(*m_logicp, vscp, "SynthJoin");
        Dfg_శీర్షం_Splice* స్థిరరాశి joinSplicep = make<DfgSplicePacked>(flp, joinp->dtype());
        joinp->srcp(joinSplicep);

        // If both paths are fully driven, just create a simple conditional
        ఒకవేళ (tDrivers.కొలత() == 1  //
            && tDrivers[0].m_lo == 0  //
            && tDrivers[0].m_hi == thenp->వెడల్పు() - 1  //
            && eDrivers.కొలత() == 1  //
            && eDrivers[0].m_lo == 0  //
            && eDrivers[0].m_hi == elsep->వెడల్పు() - 1) {
            UASSERT_OBJ(!tDefaultp, vscp, "Fully driven variable have default driver");

            DfgCond* స్థిరరాశి condp = make<DfgCond>(flp, joinp->dtype());
            condp->condp(predicatep);
            condp->thenp(thenp);
            condp->elsep(elsep);
            joinSplicep->కూడు_Driver(condp, 0, tDrivers[0].m_flp);

            //పూర్తి
            తిరిగి joinp;
        }

        // Otherwise we need to merge them part by part

        // If different bits are driven, then some might not have been assigned.. Latch?
        ఒకవేళ (tDrivers.కొలత() != eDrivers.కొలత()) {
            ++m_ctx.m_synt.nonSynLatch;
            తిరిగి శూన్యసూచిక;
        }

        కొరకు (కొలత_t i = 0; i < tDrivers.కొలత(); ++i) {
            స్థిరరాశి Driver& tDriver = tDrivers[i];
            స్థిరరాశి Driver& eDriver = eDrivers[i];
            // If different bits are driven, then some might not have been assigned.. Latch?
            ఒకవేళ (tDriver.m_lo != eDriver.m_lo || tDriver.m_hi != eDriver.m_hi) {
                ++m_ctx.m_synt.nonSynLatch;
                తిరిగి శూన్యసూచిక;
            }

            స్థిరరాశి Dfg_దత్తాంశం_Type& dtype = Dfg_దత్తాంశం_Type::packed(tDriver.m_hi - tDriver.m_lo + 1);
            DfgCond* స్థిరరాశి condp = make<DfgCond>(flp, dtype);
            condp->condp(predicatep);

            // We actally need to select the bits from the joined variables, not use the drivers
            DfgSel* స్థిరరాశి thenSelp = make<DfgSel>(flp, tDriver.m_vtxp->dtype());
            thenSelp->lsb(tDriver.m_lo);
            thenSelp->fromp(thenp);
            condp->thenp(thenSelp);

            // Same for the 'else' part
            DfgSel* స్థిరరాశి elseSelp = make<DfgSel>(flp, eDriver.m_vtxp->dtype());
            elseSelp->lsb(eDriver.m_lo);
            elseSelp->fromp(elsep);
            condp->elsep(elseSelp);

            // Add it as a driver to the join
            joinSplicep->కూడు_Driver(condp, tDriver.m_lo, tDriver.m_flp);
        }

        // If there was a default driver, add it to te join
        ఒకవేళ (tDefaultp) joinp->defaultp(tDefaultp);

        //పూర్తి
        తిరిగి joinp;
    }

    // Merge 'thenSymTab' into 'elseSymTab' using the given predicate to join values
    తార్కిక join_చిహ్నం_Tables(SymTab& elseSymTab, Dfg_శీర్షం_Var* predicatep, స్థిరరాశి SymTab& thenSymTab) {
        // Any variable that does not have a binding on both paths will be removed. These might be
        // temporaries, loop vars, etc used only in one branch. Conversion will fail if the
        // variable is actually referenced later.
        std::సదిశ<AstVarScope*> గమ్యం_Remove;

        // Join each symbol
        కొరకు (std::pair<AstVarScope* స్థిరరాశి, Dfg_శీర్షం_Var*>& pair : elseSymTab) {
            AstVarScope* స్థిరరాశి varp = pair.మొదటి;
            // Find same variable on the else path
            స్థిరరాశి స్వయంచాలక it = thenSymTab.కనుగొను(varp);
            // Record for removal if not assigned on both paths
            ఒకవేళ (it == thenSymTab.చివర()) {
                గమ్యం_Remove.emplace_వెనుక(varp);
                కొనసాగు;
            }
            // Join paths with the predicate
            Dfg_శీర్షం_Var* స్థిరరాశి thenp = it->రెండవ;
            Dfg_శీర్షం_Var* స్థిరరాశి elsep = pair.రెండవ;
            Dfg_శీర్షం_Var* స్థిరరాశి newp = joinDrivers(varp, predicatep, thenp, elsep);
            ఒకవేళ (!newp) తిరిగి అబద్ధం;
            pair.రెండవ = newp;
        }

        // Remove variables not assigned on both paths
        కొరకు (AstVarScope* స్థిరరాశి varp : గమ్యం_Remove) elseSymTab.erase(varp);

        //పూర్తి
        తిరిగి నిజం;
    }

    // Given two joining control flow edges, compute how to join their symbols.
    // Returns the predicaete to join over, and the 'then' and 'else' blocks.
    std::tuple<Dfg_శీర్షం_Var*, స్థిరరాశి Cfg_ఖండం*, స్థిరరాశి Cfg_ఖండం*>  //
    how_గమ్యం_Join(స్థిరరాశి Cfg_అంచు* స్థిరరాశి ap, స్థిరరాశి Cfg_అంచు* స్థిరరాశి bp) {
        // Find the closest common dominator of the two paths
        స్థిరరాశి Cfg_ఖండం* స్థిరరాశి domp = m_dom_వృక్షం.closestCommonDominator(ap->srcp(), bp->srcp());
        // These paths join here, so 'domp' must be a branch, otherwise it's not the closest
        UASSERT_OBJ(domp->isBranch(), domp, "closestCommonDominator is not a branch");

        // The branches of the common dominator
        స్థిరరాశి Cfg_అంచు* స్థిరరాశి తీసుకున్న_Edgep = domp->తీసుకున్న_Edgep();
        స్థిరరాశి Cfg_అంచు* స్థిరరాశి untknEdgep = domp->untknEdgep();

        // We check if the taken branch dominates the path to either blocks,
        // and if the untaken branch dominates the path to the other block.
        // If so, we can use the branch condition as predicate, otherwise
        // we must use the path predicate as there are ways to get from one
        // branch of the dominator to the other. We need to be careful if
        // either branches are directly to the join block. This is fine,
        // it's as if there was an empty block on that critical edge which
        // is dominated by that path.

        ఒకవేళ (తీసుకున్న_Edgep == ap || m_dom_వృక్షం.dominates(తీసుకున్న_Edgep->dstp(), ap->srcp())) {
            ఒకవేళ (untknEdgep == bp || m_dom_వృక్షం.dominates(untknEdgep->dstp(), bp->srcp())) {
                // Taken path dominates 'ap' and untaken dominates 'bp', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                తిరిగి std::make_tuple(m_bb_గమ్యం_Condp[domp], ap->srcp(), bp->srcp());
            }
        } లేకపోతే ఒకవేళ (తీసుకున్న_Edgep == bp || m_dom_వృక్షం.dominates(తీసుకున్న_Edgep->dstp(), bp->srcp())) {
            ఒకవేళ (untknEdgep == ap || m_dom_వృక్షం.dominates(untknEdgep->dstp(), ap->srcp())) {
                // Taken path dominates 'bp' and untaken dominates 'ap', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                తిరిగి std::make_tuple(m_bb_గమ్యం_Condp[domp], bp->srcp(), ap->srcp());
            }
        }

        // The branches don't dominate the joined blocks, must use the path predicate
        ++m_ctx.m_synt.join_Using_మార్గం_Predicate;

        // TODO: We could do better here: use the path predicate of the closest
        // cominating blocks, pick the one from the lower rank, etc, but this
        // generic case is very rare, most synthesizable logic has
        // series-parallel CFGs which are covered by the earlier cases.
        తిరిగి std::make_tuple(m_అంచు_గమ్యం_Predicatep[ap], ap->srcp(), bp->srcp());
    }

    // Combine the output symbol tables of the predecessors of the given
    // block to compute the input symtol table for the given block.
    తార్కిక create_ఆదానం_చిహ్నం_పట్టిక(స్థిరరాశి Cfg_ఖండం& bb) {
        // The input symbol table of the given block, we are computing it now
        SymTab& joined = m_bb_గమ్యం_ISym_Tab[bb];

        // Input symbol table of entry block is special
        ఒకవేళ (bb.isEnter()) {
            initialize_నమోదు_చిహ్నం_పట్టిక(joined);
            తిరిగి నిజం;
        }

        // Current input symbol table should be empty, we will fill it in here
        UASSERT(joined.ఖాళీలు(), "Unprocessed input symbol table should be empty");

        // Fast path if there is only one predecessor - TODO: use less copying
        ఒకవేళ (!bb.isJoin()) {
            joined = m_bb_గమ్యం_OSym_Tab[bb.మొదటి_Predecessorp()];
            తిరిగి నిజం;
        }

        // We also have a simpler job if there are 2 predecessors
        ఒకవేళ (bb.isTwoWayJoin()) {
            Dfg_శీర్షం_Var* predicatep = శూన్యసూచిక;
            స్థిరరాశి Cfg_ఖండం* thenp = శూన్యసూచిక;
            స్థిరరాశి Cfg_ఖండం* elsep = శూన్యసూచిక;
            std::tie(predicatep, thenp, elsep)
                = how_గమ్యం_Join(bb.మొదటి_Predecessor_Edgep(), bb.చివరి_Predecessor_Edgep());
            // Copy from else
            joined = m_bb_గమ్యం_OSym_Tab[elsep];
            // Join with then
            తిరిగి join_చిహ్నం_Tables(joined, predicatep, m_bb_గమ్యం_OSym_Tab[*thenp]);
        }

        // General hard way

        // Gather predecessors
        నిర్మాణం Predecessor final {
            స్థిరరాశి Cfg_ఖండం* m_bbp;  // Predeccessor block
            Dfg_శీర్షం_Var* m_predicatep;  // Predicate predecessor reached this block with
            స్థిరరాశి SymTab* m_oSymTabp;  // Output symbol table or predecessor
            Predecessor() = తొలగించు;
            Predecessor(స్థిరరాశి Cfg_ఖండం* bbp, Dfg_శీర్షం_Var* predicatep, స్థిరరాశి SymTab* oSymTabp)
                : m_bbp{bbp}
                , m_predicatep{predicatep}
                , m_oSymTabp{oSymTabp} {}
        };

        స్థిరరాశి std::సదిశ<Predecessor> predecessors = [&]() {
            std::సదిశ<Predecessor> res;
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : bb.లోపల_అంచులు()) {
                స్థిరరాశి Cfg_అంచు& cfg_అంచు = static_cast<స్థిరరాశి Cfg_అంచు&>(అంచు);
                స్థిరరాశి Cfg_ఖండం* స్థిరరాశి predecessorp = cfg_అంచు.srcp();
                Dfg_శీర్షం_Var* స్థిరరాశి predicatep = m_అంచు_గమ్యం_Predicatep[cfg_అంచు];
                స్థిరరాశి SymTab* స్థిరరాశి oSymTabp = &m_bb_గమ్యం_OSym_Tab[predecessorp];
                res.emplace_వెనుక(predecessorp, predicatep, oSymTabp);
            }
            // Sort predecessors reverse topologically. This way earlier blocks
            // will come after later blocks, and the entry block is last if present.
            std::క్రమీకరించు(res.begin(), res.చివర(), [](స్థిరరాశి Predecessor& a, స్థిరరాశి Predecessor& b) {  //
                తిరిగి *a.m_bbp > *b.m_bbp;
            });
            తిరిగి res;
        }();

        // Start by copying the bindings from the frist predecessor
        joined = *predecessors[0].m_oSymTabp;
        // Join over all other predecessors
        కొరకు (కొలత_t i = 1; i < predecessors.కొలత(); ++i) {
            Dfg_శీర్షం_Var* స్థిరరాశి predicatep = predecessors[i].m_predicatep;
            స్థిరరాశి SymTab& oSymTab = *predecessors[i].m_oSymTabp;
            ఒకవేళ (!join_చిహ్నం_Tables(joined, predicatep, oSymTab)) తిరిగి అబద్ధం;
        }

        తిరిగి నిజం;
    }

    std::సదిశ<Driver> computePropagatedDrivers(స్థిరరాశి std::సదిశ<Driver>& newDrivers,
                                                 Dfg_శీర్షం_Var* oldp) {
        // Gather drivers of 'oldp' - they are in incresing range order with no overlaps
        స్థిరరాశి std::సదిశ<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<Dfg_శీర్షం_Splice>());
        UASSERT_OBJ(!oldDrivers.ఖాళీలు(), oldp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::సదిశ<Driver> propagatedDrivers;

        // Add bits between 'msb' and 'lsb' from 'oldp' to 'pDrivers'
        స్థిరరాశి స్వయంచాలక కూడు_Old_Driver = [&](File_పంక్తి* స్థిరరాశి flp, uint32_t msb, uint32_t lsb) {
            UASSERT_OBJ(propagatedDrivers.ఖాళీలు() || lsb > propagatedDrivers.వెనుక().m_hi, flp,
                        "Drivers should be in ascending order");
            DfgSel* స్థిరరాశి selp = make<DfgSel>(flp, Dfg_దత్తాంశం_Type::packed(msb - lsb + 1));
            selp->lsb(lsb);
            selp->fromp(oldp);
            propagatedDrivers.emplace_వెనుక(selp, lsb, flp);
        };

        // Incorporate old drivers
        కొరకు (స్థిరరాశి Driver& oDriver : oldDrivers) {
            File_పంక్తి* స్థిరరాశి flp = oDriver.m_flp;
            // Range to consider inserting, we will adjust oldLo as we process drivers
            uint32_t oldLo = oDriver.m_lo;
            స్థిరరాశి uint32_t oldHi = oDriver.m_hi;

            // Loop for now, can move to bisection search if this is a problem, shouldn't be ...
            కొరకు (స్థిరరాశి Driver& nDriver : newDrivers) {
                UASSERT_OBJ(oldHi >= oldLo, flp, "Should have stopped iteration");
                // If new driver is entirely below old driver, move on to
                ఒకవేళ (nDriver.m_hi < oldLo) కొనసాగు;
                // If new driver is entirely above old driver, we can stop
                ఒకవేళ (oldHi < nDriver.m_lo) ఆపు;

                // There is an overlap between 'oDriver' and 'nDriver'.
                // Insert the low bits and adjust the insertion range.
                // The rest will take care of itself on subsequent iterations.
                ఒకవేళ (oldLo < nDriver.m_lo) కూడు_Old_Driver(flp, nDriver.m_lo - 1, oldLo);
                oldLo = nDriver.m_hi + 1;

                // Stop if no more bits remaining in the old driver
                ఒకవేళ (oldLo > oldHi) ఆపు;
            }

            // Insert remaining bits if any
            ఒకవేళ (oldHi >= oldLo) కూడు_Old_Driver(flp, oldHi, oldLo);
        }

        తిరిగి propagatedDrivers;
    }

    // Propagate whole elements with a linear walk over the sorted driver lists.
    // Reads use the previous value temporary, preserving assignment-version bindings.
    // Partial elements and non-unit drivers retain the nonsynthesized-process fallback.
    తార్కిక computePropagatedArrayDrivers(స్థిరరాశి std::సదిశ<Driver>& newDrivers,
                                       Dfg_శీర్షం_Var* స్థిరరాశి oldp,
                                       std::సదిశ<Driver>& propagatedDrivers) {
        // Bound quadratic vertex growth: array drivers cannot be coalesced.
        స్థిర constexpr uint32_t MAX_ARRAY_ELEMENTS = 32;
        ఒకవేళ (oldp->కొలత() > MAX_ARRAY_ELEMENTS) తిరిగి అబద్ధం;
        కొరకు (స్థిరరాశి Driver& driver : newDrivers) {
            ఒకవేళ (!driver_Covers_సంపూర్ణ_Element(driver)) తిరిగి అబద్ధం;
        }
        స్థిరరాశి std::సదిశ<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<Dfg_శీర్షం_Splice>());
        UASSERT_OBJ(!oldDrivers.ఖాళీలు(), oldp, "Should have a proper driver");
        కొరకు (స్థిరరాశి Driver& driver : oldDrivers) {
            ఒకవేళ (!driver_Covers_సంపూర్ణ_Element(driver)) తిరిగి అబద్ధం;
        }

        propagatedDrivers.reserve(oldDrivers.కొలత());
        స్వయంచాలక nIt = newDrivers.begin();
        కొరకు (స్థిరరాశి Driver& oDriver : oldDrivers) {
            అయితే (nIt != newDrivers.చివర() && nIt->m_lo < oDriver.m_lo) ++nIt;
            ఒకవేళ (nIt != newDrivers.చివర() && nIt->m_lo == oDriver.m_lo) కొనసాగు;

            File_పంక్తి* స్థిరరాశి flp = oDriver.m_flp;
            స్థిరరాశి DfgUnitArray* స్థిరరాశి oldUnitp = oDriver.m_vtxp->as<DfgUnitArray>();
            DfgArraySel* స్థిరరాశి selp = make<DfgArraySel>(flp, oldUnitp->srcp()->dtype());
            selp->fromp(oldp);
            selp->bitp(make<DfgConst>(flp, static_cast<కొలత_t>(VL_IDATASIZE), oDriver.m_lo));
            DfgUnitArray* స్థిరరాశి newUnitp = make<DfgUnitArray>(flp, oldUnitp->dtype());
            newUnitp->srcp(selp);
            propagatedDrivers.emplace_వెనుక(newUnitp, oDriver.m_lo, flp);
        }
        తిరిగి నిజం;
    }

    // Given the drivers of a variable after converting a single statement
    // 'newp', add drivers from 'oldp' that were not reassigned be drivers
    // in newp. This computes the total result of all previous assignments.
    తార్కిక incorporate_మునుపటి_విలువ(AstVarScope* vscp, Dfg_శీర్షం_Var* newp, Dfg_శీర్షం_Var* oldp) {
        UASSERT_OBJ(newp->srcp(), vscp, "Assigned variable has no driver");

        // Easy if there is no old value...
        ఒకవేళ (!oldp) తిరిగి నిజం;

        // New driver was not yet coalesced, so should always be a splice
        Dfg_శీర్షం_Splice* స్థిరరాశి nSplicep = newp->srcp()->as<Dfg_శీర్షం_Splice>();

        // If the old value is the real variable we just computed the new value for,
        // then it is the circular feedback into the synthesized block, add it as default driver.
        ఒకవేళ (oldp->vscp() == vscp) {
            ఒకవేళ (!splice_Covers_సంపూర్ణ(nSplicep)) newp->defaultp(oldp);
            తిరిగి నిజం;
        }

        UASSERT_OBJ(oldp->srcp(), vscp, "Previously assigned variable has no driver");

        // Gather drivers of 'newp' - they are in incresing range order with no overlaps
        UASSERT_OBJ(!newp->defaultp(), newp, "Converted value should not have default");
        std::సదిశ<Driver> nDrivers = gatherDrivers(newp->srcp()->as<Dfg_శీర్షం_Splice>());
        UASSERT_OBJ(!nDrivers.ఖాళీలు(), newp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::సదిశ<Driver> pDrivers;
        ఒకవేళ (newp->isPacked()) {
            pDrivers = computePropagatedDrivers(nDrivers, oldp);
        } లేకపోతే ఒకవేళ (!computePropagatedArrayDrivers(nDrivers, oldp, pDrivers)) {
            ++m_ctx.m_synt.nonSynArray;
            తిరిగి అబద్ధం;
        }

        ఒకవేళ (!pDrivers.ఖాళీలు()) {
            // Need to merge propagated sources, so reset the splice
            nSplicep->resetDrivers();
            // Merge drivers - they are both sorted and non-overlapping
            std::సదిశ<Driver> drivers;
            drivers.reserve(nDrivers.కొలత() + pDrivers.కొలత());
            std::విలీనం(nDrivers.begin(), nDrivers.చివర(), pDrivers.begin(), pDrivers.చివర(),
                       std::వెనుక_inserter(drivers));
            // Coalesce adjacent ranges
            ఒకవేళ (newp->isPacked()) coalesceDrivers(drivers);
            // Reinsert drivers in order
            కొరకు (స్థిరరాశి Driver& d : drivers) nSplicep->కూడు_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        }

        // If the old had a default, add to the new one too, unless redundant
        ఒకవేళ (oldp->defaultp() && !splice_Covers_సంపూర్ణ(nSplicep)) newp->defaultp(oldp->defaultp());

        //పూర్తి
        తిరిగి నిజం;
    }

    // Synthesize the given statements with the given input symbol table.
    // Returns true if successfolly synthesized.
    // Populates the given output symbol table.
    // Populates the given reference with the condition of the terminator branch, if any.
    తార్కిక synthesize_Basic_ఖండం(SymTab& oSymTab, Dfg_శీర్షం*& condpr,
                              స్థిరరాశి std::సదిశ<Ast_కణుపు_Stmt*>& stmtps, స్థిరరాశి SymTab& iSymTab) {
        // Use fresh set of vertices in m_converter
        స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;

        // Initialize AstVarScope -> Vertex bindings available in this block
        కొరకు (స్థిరరాశి స్వయంచాలక& pair : iSymTab) {
            AstVarScope* స్థిరరాశి varp = pair.మొదటి;
            Dfg_శీర్షం_Var* స్థిరరాశి vtxp = pair.రెండవ;
            varp->user2p(vtxp);
            oSymTab[varp] = vtxp;
        }

        // Synthesize each statement one after the other
        std::సదిశ<std::pair<AstVarScope*, Dfg_శీర్షం_Var*>> updates;
        కొరకు (Ast_కణుపు_Stmt* స్థిరరాశి stmtp : stmtps) {
            // Regular statements
            Ast_కణుపు_కేటాయించు* స్థిరరాశి ap = VN_CAST(stmtp, కణుపు_కేటాయించు);
            ఒకవేళ (ap && (VN_IS(ap, కేటాయించు) || VN_IS(ap, కేటాయించు_W))) {
                // Convert this assignment
                ఒకవేళ (!m_converter.convert(updates, *m_logicp, ap)) {
                    ++m_ctx.m_synt.nonSynConv;
                    తిరిగి అబద్ధం;
                }
                // Apply variable updates from this statement
                కొరకు (స్థిరరాశి స్వయంచాలక& pair : updates) {
                    // The target variable that was assigned to
                    AstVarScope* స్థిరరాశి vscp = pair.మొదటి;
                    // The new, potentially partially assigned value
                    Dfg_శీర్షం_Var* స్థిరరాశి newp = pair.రెండవ;
                    // Normalize drivers within this statement, bail if multidriven
                    Dfg_శీర్షం_Splice* స్థిరరాశి srcp = newp->srcp()->as<Dfg_శీర్షం_Splice>();
                    std::సదిశ<Driver> drivers = gatherDrivers(srcp);
                    స్థిరరాశి తార్కిక single = drivers.కొలత() == 1;
                    ఒకవేళ (!normalizeDrivers(*newp, drivers)) {
                        ++m_ctx.m_synt.nonSynMultidrive;
                        తిరిగి అబద్ధం;
                    }
                    // If there were more than one driver (often not), re-add in case coalesced
                    ఒకవేళ (!single) {
                        srcp->resetDrivers();
                        కొరకు (స్థిరరాశి Driver& d : drivers) srcp->కూడు_Driver(d.m_vtxp, d.m_lo, d.m_flp);
                    }
                    // The old value, if any
                    Dfg_శీర్షం_Var* స్థిరరాశి oldp = vscp->user2u().మూస గమ్యం<Dfg_శీర్షం_Var*>();
                    // Inncorporate old value into the new value
                    ఒకవేళ (!incorporate_మునుపటి_విలువ(vscp, newp, oldp)) తిరిగి అబద్ధం;
                    // Update binding of target variable
                    vscp->user2p(newp);
                    // Update output symbol table of this block
                    oSymTab[vscp] = newp;
                }
                updates.clear();
                కొనసాగు;
            }

            // Terminator branches
            ఒకవేళ (AstIf* స్థిరరాశి ifp = VN_CAST(stmtp, If)) {
                UASSERT_OBJ(ifp == stmtps.వెనుక(), ifp, "Branch should be last statement");
                // Convert condition, give up if failed
                Dfg_శీర్షం* condp = m_converter.convert(*m_logicp, ifp->condp());
                ఒకవేళ (!condp) {
                    ++m_ctx.m_synt.nonSynConv;
                    తిరిగి అబద్ధం;
                }
                // Single bit condition can be use directly, otherwise: use 'condp != 0'
                ఒకవేళ (condp->వెడల్పు() != 1) {
                    File_పంక్తి* స్థిరరాశి flp = condp->fileline();
                    DfgNeq* స్థిరరాశి neqp = make<DfgNeq>(flp, Dfg_దత్తాంశం_Type::packed(1));
                    neqp->lhsp(make<DfgConst>(flp, condp->వెడల్పు(), 0U));
                    neqp->rhsp(condp);
                    condp = neqp;
                }
                condpr = condp;
                కొనసాగు;
            }

            // Unhandled
            ++m_ctx.m_synt.nonSynStmt;
            తిరిగి అబద్ధం;
        }

        తిరిగి నిజం;
    }

    // Assign path predicates to the outgoing control flow edges of the given block
    శూన్య కేటాయించు_మార్గం_Predicates(స్థిరరాశి Cfg_ఖండం& bb) {
        // Nothing to do for the exit block
        ఒకవేళ (bb.isExit()) తిరిగి;

        // Get the predicate of this block
        Dfg_శీర్షం* స్థిరరాశి predp = [&]() -> Dfg_శీర్షం* {
            // Entry block has no predecessors, use constant true
            ఒకవేళ (bb.isEnter()) తిరిగి make<DfgConst>(m_logicp->fileline(), 1U, 1U);

            // For any other block, 'or' together all the incoming predicates
            స్థిరరాశి స్వయంచాలక& లోపల_అంచులు = bb.లోపల_అంచులు();
            స్వయంచాలక it = లోపల_అంచులు.begin();
            Dfg_శీర్షం* resp = m_అంచు_గమ్యం_Predicatep[static_cast<స్థిరరాశి Cfg_అంచు&>(*it)];
            అయితే (++it != లోపల_అంచులు.చివర()) {
                DfgOr* స్థిరరాశి orp = make<DfgOr>(resp->fileline(), resp->dtype());
                orp->rhsp(resp);
                orp->lhsp(m_అంచు_గమ్యం_Predicatep[static_cast<స్థిరరాశి Cfg_అంచు&>(*it)]);
                resp = orp;
            }
            తిరిగి resp;
        }();

        స్థిరరాశి Dfg_దత్తాంశం_Type& dtype = predp->dtype();

        స్థిరరాశి స్వయంచాలక mkTmp = [&](File_పంక్తి* flp, స్థిరరాశి అక్షరం* name, Dfg_శీర్షం* srcp) {
            స్థిరరాశి std::string ఉపసర్గ = "_BB" + std::గమ్యం_string(bb.id()) + "_" + name;
            Dfg_శీర్షం_Var* స్థిరరాశి tmpp = m_converter.createTmp(*m_logicp, flp, dtype, ఉపసర్గ);
            tmpp->srcp(srcp);
            తిరిగి tmpp;
        };

        // Assign it to a variable in case it's used multiple times
        Dfg_శీర్షం_Var* స్థిరరాశి pInp = mkTmp(predp->fileline(), "PathIn", predp);

        // For uncondional branches, the successor predicate edge is the same
        ఒకవేళ (!bb.isBranch()) {
            m_అంచు_గమ్యం_Predicatep[bb.తీసుకున్న_Edgep()] = mkTmp(pInp->fileline(), "Goto", pInp);
            తిరిగి;
        }

        // For branches, we need to factor in the branch condition
        Dfg_శీర్షం* స్థిరరాశి condp = m_bb_గమ్యం_Condp[bb];
        File_పంక్తి* స్థిరరాశి flp = condp->fileline();

        // Predicate for taken branch: 'predp & condp'
        {
            DfgAnd* స్థిరరాశి తీసుకున్న_Predp = make<DfgAnd>(flp, dtype);
            తీసుకున్న_Predp->lhsp(pInp);
            తీసుకున్న_Predp->rhsp(condp);
            m_అంచు_గమ్యం_Predicatep[bb.తీసుకున్న_Edgep()] = mkTmp(flp, "Taken", తీసుకున్న_Predp);
        }

        // Predicate for untaken branch: 'predp & ~condp'
        {
            DfgAnd* స్థిరరాశి untknPredp = make<DfgAnd>(flp, dtype);
            untknPredp->lhsp(pInp);
            DfgNot* స్థిరరాశి notp = make<DfgNot>(flp, dtype);
            notp->srcp(condp);
            untknPredp->rhsp(notp);
            m_అంచు_గమ్యం_Predicatep[bb.untknEdgep()] = mkTmp(flp, "Untkn", untknPredp);
        }
    }

    // Returns true if all external updates to volatile variables are observed correctly
    తార్కిక checkExtWrites() {
        కొరకు (స్థిరరాశి Dfg_శీర్షం* స్థిరరాశి vtxp : m_logicp->synth()) {
            స్థిరరాశి Dfg_శీర్షం_Var* స్థిరరాశి varp = vtxp->cast<Dfg_శీర్షం_Var>();
            ఒకవేళ (!varp) కొనసాగు;
            // If the variable we synthesized this vertex for is volatile, and
            // the value of the synthesized temporary is observed, we might be
            // missing an external update, so we mut give up.
            ఒకవేళ (!varp->hasSinks()) కొనసాగు;
            ఒకవేళ (!Dfg_శీర్షం_Var::isVolatile(varp->tmpForp())) కొనసాగు;
            ++m_ctx.m_synt.nonSynExtWrite;
            తిరిగి అబద్ధం;
        }
        తిరిగి నిజం;
    }

    // Add the synthesized values as drivers to the output variables of the current DfgLogic
    తార్కిక కూడు_Synthesized_Output(SymTab& oSymTab) {
        // It's possible we think a variable is written by the DfgLogic when
        // it actauly isn't, e.g.: '{a[0], b[0]}[1] = ...' does not write 'b'.
        // These LHS forms can happen after some earlier tranforms. We
        // should just run V3Const on them earlier, but we will do belt and
        // braces and check here too. We can't touch any output variables if so.
        స్థిరరాశి తార్కిక missing = m_logicp->foreachSink([&](స్థిరరాశి Dfg_శీర్షం& sink) {
            స్థిరరాశి DfgUnresolved* స్థిరరాశి unresolvedp = sink.as<DfgUnresolved>();
            AstVarScope* స్థిరరాశి vscp = unresolvedp->singleSink()->as<Dfg_శీర్షం_Var>()->vscp();
            తిరిగి !oSymTab.లెక్క(vscp);
        });
        ఒకవేళ (missing) {
            ++m_ctx.m_synt.nonSynFalseWrite;
            తిరిగి అబద్ధం;
        }

        // Add sinks to read the computed values for the target variables
        తిరిగి !m_logicp->foreachSink([&](Dfg_శీర్షం& sink) {
            DfgUnresolved* స్థిరరాశి unresolvedp = sink.as<DfgUnresolved>();
            స్థిరరాశి Dfg_శీర్షం_Var* స్థిరరాశి varp = unresolvedp->singleSink()->as<Dfg_శీర్షం_Var>();
            Dfg_శీర్షం_Var* స్థిరరాశి resp = oSymTab.at(varp->vscp());
            UASSERT_OBJ(resp->srcp(), resp, "Undriven result");

            // If the output is not used further in the synthesized logic itself,
            // then resp will be deleted before we return, so we can just use
            // its splice directly without ending up with a multi-use operation.
            ఒకవేళ (!resp->hasSinks()) {
                unresolvedp->కూడు_Driver(resp->srcp()->as<Dfg_శీర్షం_Splice>());
                తిరిగి అబద్ధం;  // OK, continue.
            }

            // TODO: computePropagatedDrivers cannot handle arrays, should
            // never happen with simple continous assignments
            ఒకవేళ (!resp->isPacked()) {
                ++m_ctx.m_synt.nonSynArray;
                తిరిగి నిజం;  // Not OK, give up
            }

            // We need to add a new splice to avoid multi-use of the original splice
            DfgSplicePacked* స్థిరరాశి splicep
                = కొత్త DfgSplicePacked{m_dfg, resp->fileline(), resp->dtype()};
            // Drivers are the same
            స్థిరరాశి std::సదిశ<Driver> drivers = computePropagatedDrivers({}, resp);
            కొరకు (స్థిరరాశి Driver& d : drivers) splicep->కూడు_Driver(d.m_vtxp, d.m_lo, d.m_flp);
            unresolvedp->కూడు_Driver(splicep);
            తిరిగి అబద్ధం;  // OK, continue
        });
    }

    // Synthesize the given AstAssignW. Returns true on success.
    తార్కిక synthesize_కేటాయించు_W(Ast_కేటాయించు_W* nodep) {
        ++m_ctx.m_synt.ఆదానం_కేటాయించు;

        // The input and output symbol tables
        SymTab iSymTab;
        SymTab oSymTab;

        // Initialzie input symbol table
        initialize_నమోదు_చిహ్నం_పట్టిక(iSymTab);

        // Synthesize as if it was in a single CfgBlock CFG
        Dfg_శీర్షం* condp = శూన్యసూచిక;
        స్థిరరాశి తార్కిక success = synthesize_Basic_ఖండం(oSymTab, condp, {nodep}, iSymTab);
        UASSERT_OBJ(!condp, nodep, "Conditional AstAssignW ???");
        ఒకవేళ (!success) తిరిగి అబద్ధం;

        // Check exernal writes are observed correctly
        ఒకవేళ (!checkExtWrites()) తిరిగి అబద్ధం;

        // Add resolved output variable drivers
        తిరిగి కూడు_Synthesized_Output(oSymTab);
    }

    // Synthesize the given AstAlways. Returns true on success.
    తార్కిక synthesizeCfg(Cfg_లేఖాచిత్రం& cfg) {
        ++m_ctx.m_synt.ఆదానం_Always;

        // If there is a backward edge (loop), we can't synthesize it
        ఒకవేళ (cfg.కలిగియుంది_Loop()) {
            ++m_ctx.m_synt.nonSynLoop;
            ++m_ctx.m_synt.cfgCyclic;
            తిరిగి అబద్ధం;
        }

        // If it's a trivial CFG we can save on some work
        ఒకవేళ (cfg.n_ఖండాలు() == 1) {
            ++m_ctx.m_synt.cfgTrivial;
        } లేకపోతే {
            // Insert two-way join blocks to aid multiplexer ordering
            ఒకవేళ (cfg.చొప్పించు_Two_Way_Joins()) {
                ++m_ctx.m_synt.cfgSp;
            } లేకపోతే {
                ++m_ctx.m_synt.cfgDag;
            }
            // Initialize maps needed for non-trivial CFGs
            m_dom_వృక్షం = Cfg_Dominator_వృక్షం{cfg};
            m_అంచు_గమ్యం_Predicatep = cfg.make_అంచు_Map<Dfg_శీర్షం_Var*>();
        }

        // Initialize CfgMaps
        m_bb_గమ్యం_ISym_Tab = cfg.make_ఖండం_Map<SymTab>();
        m_bb_గమ్యం_OSym_Tab = cfg.make_ఖండం_Map<SymTab>();
        m_bb_గమ్యం_Condp = cfg.make_ఖండం_Map<Dfg_శీర్షం_Var*>();

        // Synthesize all blocks
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : cfg.vertices()) {
            స్థిరరాశి Cfg_ఖండం& bb = static_cast<స్థిరరాశి Cfg_ఖండం&>(vtx);
            // Prepare the input symbol table of this block (enter, or join predecessor blocks)
            ఒకవేళ (!create_ఆదానం_చిహ్నం_పట్టిక(bb)) తిరిగి అబద్ధం;
            // Synthesize this block
            Dfg_శీర్షం* condp = శూన్యసూచిక;
            ఒకవేళ (!synthesize_Basic_ఖండం(m_bb_గమ్యం_OSym_Tab[bb], condp, bb.stmtps(), m_bb_గమ్యం_ISym_Tab[bb])) {
                తిరిగి అబద్ధం;
            }
            // Create a temporary for the branch condition as it might be used multiple times
            ఒకవేళ (condp) {
                File_పంక్తి* స్థిరరాశి flp = condp->fileline();
                స్థిరరాశి Dfg_దత్తాంశం_Type& dtype = condp->dtype();
                స్థిరరాశి std::string ఉపసర్గ = "_BB" + std::గమ్యం_string(bb.id()) + "_Cond";
                Dfg_శీర్షం_Var* స్థిరరాశి vp = m_converter.createTmp(*m_logicp, flp, dtype, ఉపసర్గ);
                vp->srcp(condp);
                m_bb_గమ్యం_Condp[bb] = vp;
            }
            // Set the path predicates on the successor edges
            కేటాయించు_మార్గం_Predicates(bb);
        }

        // Check exernal writes are observed correctly
        ఒకవేళ (!checkExtWrites()) తిరిగి అబద్ధం;

        // Add resolved output variable drivers
        తిరిగి కూడు_Synthesized_Output(m_bb_గమ్యం_OSym_Tab[cfg.నిష్క్రమించు()]);
    }

    // Synthesize a DfgLogic into regular vertices. Returns ture on success.
    తార్కిక synthesize(DfgLogic& vtx) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;

        ఒకవేళ (Ast_కేటాయించు_W* స్థిరరాశి ap = VN_CAST(vtx.nodep()->stmtsp(), కేటాయించు_W)) {
            ఒకవేళ (ap->nextp()) తిరిగి అబద్ధం;
            ఒకవేళ (!synthesize_కేటాయించు_W(ap)) తిరిగి అబద్ధం;
            ++m_ctx.m_synt.synth_కేటాయించు;
            తిరిగి నిజం;
        }

        ఒకవేళ (!synthesizeCfg(vtx.cfg())) తిరిగి అబద్ధం;
        ++m_ctx.m_synt.synthAlways;
        తిరిగి నిజం;
    }

    // Revert all DfgLogic in m_toRevert, or DfgLogic driving the DfgUnresolved
    // vertices in m_toRevert, and transitively the same for any DfgUnresolved
    // driven by the reverted DfgLogic. Delete all DfgUnresolved involed.
    శూన్య revert(VDouble0& statCountr) {
        m_గమ్యం_Revert.foreach([&](Dfg_శీర్షం& vtx) {
            // Process DfgLogic
            ఒకవేళ (DfgLogic* స్థిరరాశి vtxp = vtx.cast<DfgLogic>()) {
                UASSERT_OBJ(vtxp->selectedForSynthesis(), vtxp, "Shouldn't reach here unselected");
                // Revert this logic
                UASSERT_OBJ(!vtxp->reverted(), vtxp, "Should be reverting now");
                ++statCountr;
                కొరకు (Dfg_శీర్షం* స్థిరరాశి p : vtxp->synth()) VL_DO_DANGLING(p->unlinkDelete(m_dfg), p);
                vtxp->synth().clear();
                vtxp->సమితి_Reverted();
                // Add all DfgUnresolved it drives to the work list
                vtxp->foreachSink([&](Dfg_శీర్షం& snk) {
                    m_గమ్యం_Revert.నెట్టు_front(*snk.as<DfgUnresolved>());
                    తిరిగి అబద్ధం;
                });
                తిరిగి;
            }

            // Process DfgUnresolved
            ఒకవేళ (DfgUnresolved* స్థిరరాశి vtxp = vtx.cast<DfgUnresolved>()) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<Dfg_శీర్షం_Var>()->సమితి_Has_Mod_Wr_Refs();
                // Add all driving DfgLogic to the work list
                vtxp->foreach_ఆధారం([&](Dfg_శీర్షం& src) {
                    DfgLogic* స్థిరరాశి lp = src.cast<DfgLogic>();
                    ఒకవేళ (lp && !lp->reverted()) m_గమ్యం_Revert.నెట్టు_front(*lp);
                    తిరిగి అబద్ధం;
                });
                // Delete the DfgUnresolved driver
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
                తిరిగి;
            }

            // There should be nothing else on the worklist
            vtx.v3fatalSrc("Unexpected vertex type");
        });
    }

    // Synthesize all of the given vertices
    శూన్య ప్రధాన() {
        //-------------------------------------------------------------------
        UINFO(5, "Step 0: Remove all DfgLogic not selected for synthesis");
        కొరకు (Dfg_శీర్షం* స్థిరరాశి vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgUnresolved
            ఒకవేళ (!vtxp->is<DfgUnresolved>()) కొనసాగు;
            తార్కిక anySelected = అబద్ధం;
            తార్కిక anyUnselected = అబద్ధం;
            vtxp->foreach_ఆధారం([&](Dfg_శీర్షం& src) {
                స్థిరరాశి DfgLogic& logic = *src.as<DfgLogic>();
                ఒకవేళ (logic.selectedForSynthesis()) {
                    anySelected = నిజం;
                } లేకపోతే {
                    anyUnselected = నిజం;
                }
                తిరిగి అబద్ధం;
            });
            // There should be a driver
            UASSERT_OBJ(anySelected || anyUnselected, vtxp, "'DfgUnresolved' with no driver");
            // All drivers should be selected or all should be unselected
            UASSERT_OBJ(!(anySelected && anyUnselected), vtxp, "Invalid 'DfgLogic' selection");
            // If all drivers are unselected, delete this DfgUnresolved here
            ఒకవేళ (anyUnselected) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<Dfg_శీర్షం_Var>()->సమితి_Has_Mod_Wr_Refs();
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
        కొరకు (Dfg_శీర్షం* స్థిరరాశి vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgLogic
            DfgLogic* స్థిరరాశి logicp = vtxp->cast<DfgLogic>();
            ఒకవేళ (!logicp) కొనసాగు;
            ఒకవేళ (logicp->selectedForSynthesis()) కొనసాగు;
            // There should be no sinks left for unselected DfgLogic, delete them here
            UASSERT_OBJ(!logicp->hasSinks(), vtxp, "Unselected 'DfgLogic' with sinks remaining");
            // Input variables will be read in Ast code, add Ast reference vertices
            // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
            స్థిరరాశి VNUser4_లోపల_Use m_user4_లోపల_Use;
            logicp->foreach_ఆధారం([](Dfg_శీర్షం& src) {
                src.as<Dfg_శీర్షం_Var>()->vscp()->user4p(&src);
                తిరిగి అబద్ధం;
            });
            V3DfgPasses::కూడు_Ast_Refs(m_dfg, logicp->nodep(), [](Ast_కణుపు* varp) {  //
                తిరిగి varp->user4u().గమ్యం<Dfg_శీర్షం_Var*>();
            });
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        debugDump("synth-selected");

        //-------------------------------------------------------------------
        UINFO(5, "Step 1: Attempting to synthesize each of the selected DfgLogic");
        {
            // AstVarScope::user4() -> int: unique ID for 'AstVarScopeComparator'
            స్థిరరాశి VNUser4_లోపల_Use user4_లోపల_Use;
            కొరకు (Dfg_శీర్షం& vtx : m_dfg.opVertices()) {
                DfgLogic* స్థిరరాశి logicp = vtx.cast<DfgLogic>();
                ఒకవేళ (!logicp) కొనసాగు;

                // We should only have DfgLogic remaining that was selected for synthesis
                UASSERT_OBJ(logicp->selectedForSynthesis(), logicp, "Unselected DfgLogic remains");

                // Debug aid
                స్థిరరాశి స్వయంచాలక debugCallback = [&]() -> శూన్య {
                    // This is the breaking logic
                    m_debugLogicp = logicp;
                    // Dump it
                    UINFOTREE(0, logicp->nodep(), "Problematic DfgLogic: " << logicp, "  ");
                    V3EmitV::debug_Verilog_For_వృక్షం(logicp->nodep(), std::బయటకు);
                    debugDump("synth-lastok");
                };
                ఒకవేళ (VL_UNLIKELY(s_dfgSynthDebugBisect.stop(debugCallback))) ఆపు;

                // Synthesize it, if failed, enqueue for reversion
                ఒకవేళ (!synthesize(*logicp)) {
                    logicp->సమితి_Non_Synthesizable();
                    m_గమ్యం_Revert.నెట్టు_front(*logicp);
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
        std::సదిశ<Dfg_శీర్షం_Var*> multidrivenps;
        // Map from variable to its resolved driver
        std::unordered_map<స్థిరరాశి Dfg_శీర్షం_Var*, Dfg_శీర్షం_Splice*> resolvedDrivers;
        // Compute resolved drivers of all variablees
        కొరకు (Dfg_శీర్షం_Var& var : m_dfg.varVertices()) {
            ఒకవేళ (!var.srcp()) కొనసాగు;
            DfgUnresolved* స్థిరరాశి unresolvedp = var.srcp()->cast<DfgUnresolved>();
            ఒకవేళ (!unresolvedp) ఆపు;  // Stop when reached the synthesized temporaries

            // Resolve the synthesized drivers
            Dfg_శీర్షం_Splice* స్థిరరాశి resolvedp = [&]() -> Dfg_శీర్షం_Splice* {
                // All synthesized drivers were normalized already,
                // so if there is only one, it can be used directly
                ఒకవేళ (unresolvedp->nInputs() == 1) {
                    తిరిగి unresolvedp->inputp(0)->as<Dfg_శీర్షం_Splice>();
                }
                // Otherwise gather the synthesized drivers
                std::సదిశ<Driver> drivers = gatherDriversUnresolved(unresolvedp);
                // Normalize them, make resolved driver if all good
                ఒకవేళ (normalizeDrivers(var, drivers)) తిరిగి makeSplice(var, drivers);
                // If mutlidriven, record and ignore
                multidrivenps.emplace_వెనుక(&var);
                m_గమ్యం_Revert.నెట్టు_front(*unresolvedp);
                తిరిగి శూన్యసూచిక;
            }();
            // Bail if multidriven
            ఒకవేళ (!resolvedp) కొనసాగు;
            // Add to map for next loop
            స్థిరరాశి తార్కిక new_నమోదు = resolvedDrivers.emplace(&var, resolvedp).రెండవ;
            UASSERT_OBJ(new_నమోదు, &var, "Duplicate driver");
        }
        // Revert and remove drivers of multi-driven variables
        revert(m_ctx.m_synt.revertMultidrive);
        // Replace all DfgUnresolved with the resolved drivers
        కొరకు (స్థిరరాశి Dfg_శీర్షం_Var& var : m_dfg.varVertices()) {
            ఒకవేళ (!var.srcp()) కొనసాగు;
            DfgUnresolved* స్థిరరాశి srcp = var.srcp()->cast<DfgUnresolved>();
            ఒకవేళ (!srcp) ఆపు;  // Stop when reached the synthesized temporaries

            // Replace it
            srcp->replaceWith(resolvedDrivers.at(&var));
            VL_DO_DANGLING(srcp->unlinkDelete(m_dfg), srcp);
        }
        debugDump("synth-resolved");

        //-------------------------------------------------------------------
        UINFO(5, "Step 4: Remove all remaining DfgLogic and DfgUnresolved");
        కొరకు (Dfg_శీర్షం* స్థిరరాశి vtxp : m_dfg.opVertices().unlinkable()) {
            // Previous step should have removed all DfgUnresolved
            UASSERT_OBJ(!vtxp->is<DfgUnresolved>(), vtxp, "DfgUnresolved remains");

            // Process only DfgLogic
            DfgLogic* స్థిరరాశి logicp = vtxp->cast<DfgLogic>();
            ఒకవేళ (!logicp) కొనసాగు;

            // Earlier pass should have removed all sinks
            UASSERT_OBJ(!logicp->hasSinks(), logicp, "DfgLogic sink remains");

            ఒకవేళ (!logicp->synth().ఖాళీలు()) {
                // If synthesized, delete the corresponding AstNode. It is now in Dfg.
                logicp->nodep()->unlink_Fr_వెనుక()->delete_వృక్షం();
            } లేకపోతే {
                // Not synthesized. Logic stays in Ast. Add Ast reference vertices.
                // Outputs already marked by revertTransivelyAndRemove.
                // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
                స్థిరరాశి VNUser4_లోపల_Use m_user4_లోపల_Use;
                logicp->foreach_ఆధారం([](Dfg_శీర్షం& src) {
                    src.as<Dfg_శీర్షం_Var>()->vscp()->user4p(&src);
                    తిరిగి అబద్ధం;
                });
                V3DfgPasses::కూడు_Ast_Refs(m_dfg, logicp->nodep(), [](Ast_కణుపు* varp) {  //
                    తిరిగి varp->user4u().గమ్యం<Dfg_శీర్షం_Var*>();
                });
            }

            // Delete this DfgLogic
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        // Reset the debug pointer, we have deleted it in the loop above ...
        m_debugLogicp = శూన్యసూచిక;
        debugDump("synth-rmlogics");

        //-------------------------------------------------------------------
        UINFO(5, "Step 5: Remove unnecessary splices");
        కొరకు (Dfg_శీర్షం* స్థిరరాశి vtxp : m_dfg.opVertices().unlinkable()) {
            Dfg_శీర్షం_Splice* స్థిరరాశి splicep = vtxp->cast<Dfg_శీర్షం_Splice>();
            ఒకవేళ (!splicep) కొనసాగు;

            // Might not have a sink if the driving logic was revered, remove
            ఒకవేళ (!splicep->hasSinks()) {
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
                కొనసాగు;
            }

            // It should alway have drivers
            UASSERT_OBJ(splicep->nInputs(), splicep, "Splice with no drivers");

            // If redundant, remove it
            ఒకవేళ (Dfg_శీర్షం* స్థిరరాశి wholep = splicep->wholep()) {
                ఒకవేళ (Dfg_శీర్షం_Var* స్థిరరాశి varp = splicep->singleSink()->cast<Dfg_శీర్షం_Var>()) {
                    varp->driver_File_పంక్తి(splicep->driver_File_పంక్తి(0));
                }
                splicep->replaceWith(wholep);
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
            }
        }
        debugDump("synth-rmsplice");
    }

    // CONSTRUCTOR
    Ast_గమ్యం_Dfg_Synthesize(Dfg_లేఖాచిత్రం& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx}
        , m_converter{dfg, ctx} {
        ప్రధాన();
    }

బహిరంగ:
    స్థిర శూన్య apply(Dfg_లేఖాచిత్రం& dfg, V3DfgSynthesisContext& ctx) {
        Ast_గమ్యం_Dfg_Synthesize{dfg, ctx};

        // Final step outside, as both AstToDfgSynthesize and removeUnused used DfgUserMap
        UINFO(5, "Step 6: Remove all unused vertices");
        V3DfgPasses::remove_వాడని(dfg);
        ఒకవేళ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("synth-rmunused");

        // No operation vertex should have multiple sinks. Cyclic decomoposition
        // depends on this and it can easily be ensured by using temporaries.
        // Also, all sources should be connected at this point
        ఒకవేళ (v3Global.opt.debugCheck()) {
            కొరకు (Dfg_శీర్షం& vtx : dfg.opVertices()) {
                UASSERT_OBJ(!vtx.has_గుణిజం_Sinks(), &vtx, "Operation has multiple sinks");
                కొరకు (కొలత_t i = 0; i < vtx.nInputs(); ++i) {
                    UASSERT_OBJ(vtx.inputp(i), &vtx, "Unconnected source operand");
                }
            }
            V3DfgPasses::typeCheck(dfg);
        }
    }
};

పూర్ణాంకం Ast_గమ్యం_Dfg_Synthesize::AstVarScopeComparator::s_vscpIdCounter = 0;

// Decide which DfgLogic to attempt to synthesize
స్థిర శూన్య dfgSelectLogicForSynthesis(Dfg_లేఖాచిత్రం& dfg) {
    // If we are told to synthesize everything, we will do so ...
    ఒకవేళ (v3Global.opt.fDfgSynthesizeAll()) {
        కొరకు (Dfg_శీర్షం& vtx : dfg.opVertices()) {
            ఒకవేళ (DfgLogic* స్థిరరాశి logicp = vtx.cast<DfgLogic>()) logicp->సమితి_Selected_For_Synthesis();
        }
        తిరిగి;
    }

    // Otherwise figure out which vertices are likely worth synthesizing.

    std::సదిశ<Dfg_శీర్షం_Var*> circularVarps;
    // Synthesize all drivers of circular variables
    {
        DfgUserMap<uint64_t> scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, scc);
        కొరకు (Dfg_శీర్షం_Var& var : dfg.varVertices()) {
            ఒకవేళ (!scc.at(var)) కొనసాగు;
            // This is a circular variable
            circularVarps.emplace_వెనుక(&var);
        }
    }

    // To cover all drivers, use a work list
    DfgWorklist worklist{dfg};

    // Synthesize all drivers of circular variables
    కొరకు (స్థిరరాశి Dfg_శీర్షం_Var* స్థిరరాశి varp : circularVarps) {
        varp->srcp()->as<DfgUnresolved>()->foreach_ఆధారం([&](Dfg_శీర్షం& driver) {
            worklist.నెట్టు_front(*driver.as<DfgLogic>());
            తిరిగి అబద్ధం;
        });
    }

    // Choose some simple special cases to always synthesize
    కొరకు (Dfg_శీర్షం& vtx : dfg.opVertices()) {
        DfgLogic* స్థిరరాశి logicp = vtx.cast<DfgLogic>();
        ఒకవేళ (!logicp || worklist.కలిగియుంది(*logicp)) కొనసాగు;
        // If drives an unused variable, synthesize it so the partial logic can be removed
        ఒకవేళ (logicp->drives_వాడని_Vars()) {
            worklist.నెట్టు_front(*logicp);
            కొనసాగు;
        }
        // Blocks corresponding to continuous assignments
        ఒకవేళ (logicp->nodep()->keyword() == VAlwaysKwd::CONT_కేటాయించు) {
            worklist.నెట్టు_front(*logicp);
            కొనసాగు;
        }
        స్థిరరాశి Cfg_లేఖాచిత్రం& cfg = logicp->cfg();
        // Straight line code with no branches
        ఒకవేళ (cfg.n_ఖండాలు() == 1) {
            worklist.నెట్టు_front(*logicp);
            కొనసాగు;
        }
        // Blocks driving exactly 1 variable
        ఒకవేళ (!logicp->has_గుణిజం_Sinks()) worklist.నెట్టు_front(*logicp);
    }

    std::unordered_సమితి<DfgUnresolved*> is_సందర్శించిన;

    // Now expand to cover all logic driving the same set of variables and mark
    worklist.foreach([&](Dfg_శీర్షం& vtx) {
        DfgLogic& logic = *vtx.as<DfgLogic>();
        UASSERT_OBJ(!logic.selectedForSynthesis(), &vtx, "Should not be visited twice");
        // Mark as selected for synthesis
        logic.సమితి_Selected_For_Synthesis();
        // Enqueue all other logic driving the same variables as this one
        logic.foreachSink([&](Dfg_శీర్షం& sink) {
            DfgUnresolved* unresolved = sink.as<DfgUnresolved>();
            ఒకవేళ (!is_సందర్శించిన.చొప్పించు(unresolved).రెండవ) తిరిగి అబద్ధం;
            unresolved->foreach_ఆధారం([&](Dfg_శీర్షం& sibling) {
                DfgLogic& siblingLogic = *sibling.as<DfgLogic>();
                ఒకవేళ (!siblingLogic.selectedForSynthesis()) worklist.నెట్టు_front(siblingLogic);
                తిరిగి అబద్ధం;
            });
            తిరిగి అబద్ధం;
        });
    });
}

శూన్య V3DfgPasses::synthesize(Dfg_లేఖాచిత్రం& dfg, V3DfgContext& ctx) {
    // Select which DfgLogic to attempt to synthesize
    dfgSelectLogicForSynthesis(dfg);
    // Synthesize them - also removes un-synthesized DfgLogic, so must run even if nothing selected
    Ast_గమ్యం_Dfg_Synthesize::apply(dfg, ctx.m_synthContext);
}
