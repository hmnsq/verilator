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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Cfg.h"
#অন্তর্ভাৱ "V3Const.h"
#অন্তর্ভাৱ "V3Dfg.h"
#অন্তর্ভাৱ "V3DfgPasses.h"
#অন্তর্ভাৱ "V3EmitV.h"

#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <iterator>

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ {

// Create a DfgVertex out of a AstNodeExpr. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
template <typename T_Vertex, typename T_গ্ৰন্থি>
T_Vertex* makeVertex(ধ্রুৱ T_গ্ৰন্থি* nodep, DfgGraph& dfg, ধ্রুৱ DfgDataType& dtype) {
    ঘূৰাই নূতন T_Vertex{dfg, nodep->fileline(), dtype};
}

template <>
DfgArraySel* makeVertex<DfgArraySel, AstArraySel>(ধ্রুৱ AstArraySel* nodep, DfgGraph& dfg,
                                                  ধ্রুৱ DfgDataType& dtype) {
    // Some earlier passes create malformed ArraySels, just bail on those...
    // See t_bitsel_wire_array_bad
    যদি (VN_IS(nodep->fromp(), Const)) ঘূৰাই nullptr;
    যদি (!VN_IS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) ঘূৰাই nullptr;
    ঘূৰাই নূতন DfgArraySel{dfg, nodep->fileline(), dtype};
}

}  // namespace

// Visitor that can convert Ast statements and expressions in Dfg vertices
শ্ৰেণী AstToDfgConverter final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats

    // Current logic vertex we are synthesizing
    DfgLogic* m_logicp = nullptr;
    // Variable updates produced by currently converted statement. This almost
    // always have a single element, so a vector is ok
    std::vector<std::pair<AstVarScope*, DfgVertexVar*>>* m_updatesp = nullptr;

    তার্কিক m_foundUnhandled = মিছা;  // Found node not implemented as DFG or not implemented 'visit'
    তার্কিক m_converting = মিছা;  // We are trying to convert some logic at the moment

    // METHODS

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    template <typename Vertex, typename... Args>
    Vertex* make(Args&&... args) {
        static_assert(!std::is_base_of<DfgVertexVar, Vertex>::মান, "Do not use for variables");
        static_assert(std::is_base_of<DfgVertex, Vertex>::মান, "'Vertex' must be a 'DfgVertex'");
        Vertex* ধ্রুৱ vtxp = নূতন Vertex{m_dfg, std::forward<Args>(args)...};
        m_logicp->synth().emplace_back(vtxp);
        ঘূৰাই vtxp;
    }

    // Returns true if the expression cannot (or should not) be represented by DFG
    তার্কিক unhandled(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        // Short-circuiting if something was already unhandled
        যদি (m_foundUnhandled) {
            // Impure nodes cannot be represented
            যদি (!nodep->isPure()) {
                m_foundUnhandled = সঁচা;
                ++m_ctx.m_conv.nonRepImpure;
            }
        }
        ঘূৰাই m_foundUnhandled;
    }

    তার্কিক isSupported(ধ্রুৱ AstVarRef* nodep) {
        // Cannot represent cross module references
        যদি (nodep->classOrPackagep()) ঘূৰাই মিছা;
        // Check target
        ঘূৰাই V3Dfg::isSupported(nodep->varScopep());
    }

    // Given an RValue expression, return the equivalent Vertex, or nullptr if not representable.
    DfgVertex* convertRValue(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        UASSERT_OBJ(!m_converting, nodep, "'convertingRValue' should not be called recursively");
        VL_RESTORER(m_converting);
        VL_RESTORER(m_foundUnhandled);
        m_converting = সঁচা;
        m_foundUnhandled = মিছা;

        // Convert the expression
        iterate(nodep);

        // If falied to convert, return nullptr
        যদি (m_foundUnhandled) ঘূৰাই nullptr;

        // Traversal set user2p to the equivalent vertex
        DfgVertex* ধ্রুৱ vtxp = nodep->user2u().to<DfgVertex*>();
        UASSERT_OBJ(vtxp, nodep, "Missing Dfg vertex after conversion");
        ঘূৰাই vtxp;
    }

    // Given an LValue expression, return the splice node that writes the
    // destination, together with the index to use for splicing in the value.
    // Returns {nullptr, 0}, if the given LValue expression is not supported.
    std::pair<DfgVertexSplice*, uint32_t> convertLValue(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        যদি (ধ্রুৱ AstVarRef* ধ্রুৱ vrefp = VN_CAST(nodep, VarRef)) {
            UASSERT_OBJ(vrefp->access().isWriteOnly(), vrefp, "Non-WriteOnly reference");
            যদি (!isSupported(vrefp)) {
                ++m_ctx.m_conv.nonRepLValue;
                ঘূৰাই {nullptr, 0};
            }

            // Get (or create a new) temporary for this variable
            ধ্রুৱ DfgVertexVar* ধ্রুৱ vtxp = [&]() -> DfgVertexVar* {
                // The variable being assigned
                AstVarScope* ধ্রুৱ vscp = vrefp->varScopep();

                // Find existing one, if any
                বাবে (ধ্রুৱ auto& pair : *m_updatesp) {
                    যদি (pair.first == vscp) ঘূৰাই pair.second;
                }

                // Create new one
                DfgVertexVar* ধ্রুৱ newp = createTmp(*m_logicp, vscp, "SynthAssign");
                m_updatesp->emplace_back(vscp, newp);

                // Create the Splice driver for the new temporary
                যদি (newp->is<DfgVarPacked>()) {
                    newp->srcp(make<DfgSplicePacked>(newp->fileline(), newp->dtype()));
                } অন্যথা যদি (newp->is<DfgVarArray>()) {
                    newp->srcp(make<DfgSpliceArray>(newp->fileline(), newp->dtype()));
                } অন্যথা {
                    nodep->v3fatalSrc("Unhandled DfgVertexVar sub-type");
                }

                // Use new temporary
                ঘূৰাই newp;
            }();

            // Return the Splice driver
            ঘূৰাই {vtxp->srcp()->as<DfgVertexSplice>(), 0};
        }

        যদি (ধ্রুৱ AstSel* selp = VN_CAST(nodep, Sel)) {
            // Only handle constant selects
            ধ্রুৱ AstConst* ধ্রুৱ lsbp = VN_CAST(selp->lsbp(), Const);
            যদি (!lsbp) {
                ++m_ctx.m_conv.nonRepLValue;
                ঘূৰাই {nullptr, 0};
            }
            uint32_t lsb = lsbp->toUInt();

            // Convert the 'fromp' sub-expression
            ধ্রুৱ auto pair = convertLValue(selp->fromp());
            যদি (!pair.first) ঘূৰাই {nullptr, 0};
            DfgSplicePacked* ধ্রুৱ splicep = pair.first->template as<DfgSplicePacked>();
            // Adjust index.
            lsb += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            যদি (lsb + static_cast<uint32_t>(selp->widthConst()) > splicep->size()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                ঘূৰাই {nullptr, 0};
            }

            // AstSel doesn't change type kind (array vs packed), so we can use
            // the existing splice driver with adjusted lsb
            ঘূৰাই {splicep, lsb};
        }

        যদি (ধ্রুৱ AstArraySel* ধ্রুৱ aselp = VN_CAST(nodep, ArraySel)) {
            // Only handle constant selects
            ধ্রুৱ AstConst* ধ্রুৱ indexp = VN_CAST(aselp->bitp(), Const);
            যদি (!indexp) {
                ++m_ctx.m_conv.nonRepLValue;
                ঘূৰাই {nullptr, 0};
            }
            uint32_t index = indexp->toUInt();

            // Convert the 'fromp' sub-expression
            ধ্রুৱ auto pair = convertLValue(aselp->fromp());
            যদি (!pair.first) ঘূৰাই {nullptr, 0};
            DfgSpliceArray* ধ্রুৱ splicep = pair.first->template as<DfgSpliceArray>();
            // Adjust index. Note pair.second is always 0, but we might handle array slices later..
            index += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            যদি (index + 1U > splicep->size()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                ঘূৰাই {nullptr, 0};
            }

            // Ensure the Splice driver exists for this element
            যদি (!splicep->driverAt(index)) {
                File_সাৰি* ধ্রুৱ flp = nodep->fileline();
                // This should never fail
                ধ্রুৱ DfgDataType& dtype = *DfgDataType::fromAst(nodep->dtypep());
                যদি (dtype.isPacked()) {
                    DfgSplicePacked* ধ্রুৱ newp = make<DfgSplicePacked>(flp, dtype);
                    ধ্রুৱ DfgDataType& uaDtype = DfgDataType::array(dtype, 1);
                    DfgUnitArray* ধ্রুৱ uap = make<DfgUnitArray>(flp, uaDtype);
                    uap->srcp(newp);
                    splicep->addDriver(uap, index, flp);
                } অন্যথা যদি (dtype.isArray()) {
                    DfgSpliceArray* ধ্রুৱ newp = make<DfgSpliceArray>(flp, dtype);
                    splicep->addDriver(newp, index, flp);
                } অন্যথা {
                    nodep->v3fatalSrc("Unhandled data type kind");
                }
            }

            // Return the splice driver
            DfgVertex* driverp = splicep->driverAt(index);
            যদি (ধ্রুৱ DfgUnitArray* ধ্রুৱ uap = driverp->cast<DfgUnitArray>()) {
                driverp = uap->srcp();
            }
            ঘূৰাই {driverp->as<DfgVertexSplice>(), 0};
        }

        ++m_ctx.m_conv.nonRepLValue;
        ঘূৰাই {nullptr, 0};
    }

    // Given the LHS of an assignment, and the vertex representing the RHS,
    // connect up the RHS to drive the targets.
    // Returns true on success, false if the LHS is not representable.
    তার্কিক convertAssignment(File_সাৰি* flp, Ast_গ্ৰন্থি_ৰাশিমালা* lhsp, DfgVertex* vtxp) {
        // Represents a DFG assignment contributed by the AST assignment with the above 'lhsp'.
        // There might be multiple of these if 'lhsp' is a concatenation.
        সংরচনা Assignment final {
            DfgVertexSplice* m_lhsp;  // Splice vertex being driven
            uint32_t m_idx;  // Position (bit or array index) within m_lhsp being driven
            DfgVertex* m_rhsp;  // Vertex driving that position
            Assignment() = delete;
            Assignment(DfgVertexSplice* lhsp, uint32_t idx, DfgVertex* rhsp)
                : m_lhsp{lhsp}
                , m_idx{idx}
                , m_rhsp{rhsp} {}
        };

        // Simplify the LHS, to get rid of things like SEL(CONCAT(_, _), _)
        যদি (!VN_IS(lhsp, VarRef)) lhsp = VN_AS(V3Const::constifyExpensiveEdit(lhsp), গ্ৰন্থি_ৰাশিমালা);

        // Assigning compound expressions to a concatenated LHS requires a temporary
        // to avoid multiple use of the expression
        যদি (VN_IS(lhsp, Concat) && !vtxp->is<DfgVertexVar>() && !vtxp->is<DfgConst>()) {
            DfgVertexVar* ধ্রুৱ tmpp = createTmp(*m_logicp, flp, vtxp->dtype(), "Unpack");
            tmpp->srcp(vtxp);
            vtxp = tmpp;
        }

        // Convert each concatenation LHS separately, gather all assignments
        // we need to do into 'assignments', return true if all LValues
        // converted successfully.
        std::vector<Assignment> assignments;
        ধ্রুৱ std::function<তার্কিক(Ast_গ্ৰন্থি_ৰাশিমালা*, uint32_t)> convertAllLValues
            = [&](Ast_গ্ৰন্থি_ৰাশিমালা* subp, uint32_t lsb) -> তার্কিক {
            // Concatenation on the LHS, convert each part
            যদি (AstConcat* ধ্রুৱ concatp = VN_CAST(subp, Concat)) {
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ cRhsp = concatp->rhsp();
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ cLhsp = concatp->lhsp();
                // Convert Rigth of concat
                যদি (!convertAllLValues(cRhsp, lsb)) ঘূৰাই মিছা;
                // Convert Left of concat
                ঘূৰাই convertAllLValues(cLhsp, lsb + cRhsp->width());
            }

            // Non-concatenation, convert the LValue
            ধ্রুৱ auto pair = convertLValue(subp);
            যদি (!pair.first) ঘূৰাই মিছা;

            // If whole lhs, just use it
            যদি (subp == lhsp) {
                assignments.emplace_back(pair.first, pair.second, vtxp);
                ঘূৰাই সঁচা;
            }

            // Otherwise select the relevant bits
            ধ্রুৱ DfgDataType& dtype = *DfgDataType::fromAst(subp->dtypep());
            DfgSel* ধ্রুৱ selp = make<DfgSel>(subp->fileline(), dtype);
            selp->fromp(vtxp);
            selp->lsb(lsb);
            assignments.emplace_back(pair.first, pair.second, selp);
            ঘূৰাই সঁচা;
        };

        // Convert the given LHS assignment, give up if any LValues failed to convert
        যদি (!convertAllLValues(lhsp, 0)) ঘূৰাই মিছা;

        // All successful, connect the drivers
        বাবে (ধ্রুৱ Assignment& item : assignments) {
            যদি (DfgSplicePacked* ধ্রুৱ spp = item.m_lhsp->template cast<DfgSplicePacked>()) {
                spp->addDriver(item.m_rhsp, item.m_idx, flp);
            } অন্যথা যদি (DfgSpliceArray* ধ্রুৱ sap = item.m_lhsp->template cast<DfgSpliceArray>()) {
                // TODO: multi-dimensional arrays will need changes here
                ধ্রুৱ DfgDataType& rDt = item.m_rhsp->dtype();
                যদি (rDt.isPacked()) {
                    // RHS is assigning an element of this array. Need a DfgUnitArray adapter.
                    DfgUnitArray* ধ্রুৱ uap = make<DfgUnitArray>(flp, DfgDataType::array(rDt, 1));
                    uap->srcp(item.m_rhsp);
                    sap->addDriver(uap, item.m_idx, flp);
                } অন্যথা {
                    // RHS is assigning an array (or array slice). Should be the same element type.
                    sap->addDriver(item.m_rhsp, item.m_idx, flp);
                }
            } অন্যথা {
                item.m_lhsp->v3fatalSrc("Unhandled DfgVertexSplice sub-type");
            }
        }

        ঘূৰাই সঁচা;
    }

    // VISITORS

    // Unhandled node
    শূন্য visit(Ast_গ্ৰন্থি* /*nodep*/) override {
        যদি (!m_foundUnhandled && m_converting) ++m_ctx.m_conv.nonRepUnknown;
        m_foundUnhandled = সঁচা;
    }

    // Expressions - mostly auto generated, but a few special ones
    শূন্য visit(AstVarRef* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        যদি (unhandled(nodep)) ঘূৰাই;
        // This visit method is only called on RValues, where only read refs are supported
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Non-ReadOnly reference");
        যদি (!isSupported(nodep)) {
            m_foundUnhandled = সঁচা;
            ++m_ctx.m_conv.nonRepVarRef;
            ঘূৰাই;
        }

        // Variable should have been bound before starting conversion
        DfgVertex* ধ্রুৱ vtxp = nodep->varScopep()->user2u().template to<DfgVertexVar*>();
        UASSERT_OBJ(vtxp, nodep, "Referenced variable has no associated DfgVertexVar");
        nodep->user2p(vtxp);
    }
    শূন্য visit(AstConst* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        যদি (unhandled(nodep)) ঘূৰাই;

        যদি (nodep->width() != nodep->অঙ্ক().width()) {
            // Sometimes the width of the AstConst is not the same as the
            // V3Number it holds. Truncate it here. TODO: should this be allowed?
            V3_সংখ্যা অঙ্ক{nodep, nodep->width()};
            অঙ্ক.opSel(nodep->অঙ্ক(), nodep->width() - 1, 0);
            DfgVertex* ধ্রুৱ vtxp = make<DfgConst>(nodep->fileline(), অঙ্ক);
            nodep->user2p(vtxp);
        } অন্যথা {
            DfgVertex* ধ্রুৱ vtxp = make<DfgConst>(nodep->fileline(), nodep->অঙ্ক());
            nodep->user2p(vtxp);
        }
    }
    শূন্য visit(AstCReset* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        যদি (unhandled(nodep)) ঘূৰাই;

        ধ্রুৱ DfgDataType* ধ্রুৱ dtypep = DfgDataType::fromAst(nodep->dtypep());
        যদি (!dtypep) {
            m_foundUnhandled = সঁচা;
            ++m_ctx.m_conv.nonRepDType;
            ঘূৰাই;
        }

        UASSERT_OBJ(!nodep->constructing(), nodep,
                    "CReset should be non-constructing at this stage");

        DfgVertex* ধ্রুৱ vtxp = make<DfgCReset>(nodep->fileline(), *dtypep);
        nodep->user2p(vtxp);
    }
    শূন্য visit(AstMatchMasked* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        যদি (unhandled(nodep)) ঘূৰাই;

        ধ্রুৱ DfgDataType* ধ্রুৱ dtypep = DfgDataType::fromAst(nodep->dtypep());
        যদি (!dtypep) {
            m_foundUnhandled = সঁচা;
            ++m_ctx.m_conv.nonRepDType;
            ঘূৰাই;
        }

        iterate(nodep->lhsp());
        যদি (m_foundUnhandled) ঘূৰাই;
        iterate(nodep->matchp());
        যদি (m_foundUnhandled) ঘূৰাই;

        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        DfgMatchMasked* ধ্রুৱ vtxp = make<DfgMatchMasked>(flp, *dtypep);
        vtxp->lhsp(nodep->lhsp()->user2u().to<DfgVertex*>());
        vtxp->matchp(nodep->matchp()->user2u().to<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    শূন্য visit(AstReplicate* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        যদি (unhandled(nodep)) ঘূৰাই;

        ধ্রুৱ DfgDataType* ধ্রুৱ dtypep = DfgDataType::fromAst(nodep->dtypep());
        যদি (!dtypep) {
            m_foundUnhandled = সঁচা;
            ++m_ctx.m_conv.nonRepDType;
            ঘূৰাই;
        }

        iterate(nodep->srcp());
        যদি (m_foundUnhandled) ঘূৰাই;

        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        DfgRep* ধ্রুৱ vtxp = make<DfgRep>(flp, *dtypep);
        vtxp->srcp(nodep->srcp()->user2u().to<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    শূন্য visit(AstSel* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        যদি (unhandled(nodep)) ঘূৰাই;

        ধ্রুৱ DfgDataType* ধ্রুৱ dtypep = DfgDataType::fromAst(nodep->dtypep());
        যদি (!dtypep) {
            m_foundUnhandled = সঁচা;
            ++m_ctx.m_conv.nonRepDType;
            ঘূৰাই;
        }

        iterate(nodep->fromp());
        যদি (m_foundUnhandled) ঘূৰাই;

        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        DfgVertex* vtxp = nullptr;
        যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep->lsbp(), Const)) {
            ধ্রুৱ uint32_t lsb = constp->toUInt();
            ধ্রুৱ uint32_t msb = lsb + nodep->widthConst() - 1;
            DfgVertex* ধ্রুৱ fromp = nodep->fromp()->user2u().to<DfgVertex*>();
            // Unfortunately we can still have out of bounds selects due to how
            // indices are truncated for speed reasons in V3Width/V3Unknown.
            যদি (msb >= fromp->size()) {
                m_foundUnhandled = সঁচা;
                ++m_ctx.m_conv.nonRepOOBSel;
                ঘূৰাই;
            }
            DfgSel* ধ্রুৱ selp = make<DfgSel>(flp, *dtypep);
            selp->fromp(fromp);
            selp->lsb(lsb);
            vtxp = selp;
        } অন্যথা {
            iterate(nodep->lsbp());
            যদি (m_foundUnhandled) ঘূৰাই;
            DfgMux* ধ্রুৱ muxp = make<DfgMux>(flp, *dtypep);
            muxp->fromp(nodep->fromp()->user2u().to<DfgVertex*>());
            muxp->lsbp(nodep->lsbp()->user2u().to<DfgVertex*>());
            vtxp = muxp;
        }
        nodep->user2p(vtxp);
    }

// The rest of the visit methods for expressions are generated by 'astgen'
#অন্তর্ভাৱ "V3Dfg__gen_ast_to_dfg.h"

সার্ৱজনিক:
    // PUBLIC METHODS

    // Create temporay variable capable of holding the given type
    DfgVertexVar* createTmp(DfgLogic& logic, File_সাৰি* flp, ধ্রুৱ DfgDataType& dtype,
                            ধ্রুৱ std::string& prefix) {
        DfgVertexVar* ধ্রুৱ vtxp = m_dfg.makeNewVar(flp, prefix, dtype, logic.scopep());
        logic.synth().emplace_back(vtxp);
        vtxp->vscp()->varp()->isInternal(সঁচা);
        vtxp->tmpForp(vtxp->vscp());
        ঘূৰাই vtxp;
    }

    // Create a new temporary variable capable of holding 'varp'
    DfgVertexVar* createTmp(DfgLogic& logic, AstVarScope* vscp, ধ্রুৱ std::string& prefix) {
        AstVar* ধ্রুৱ astVarp = vscp->varp();
        File_সাৰি* ধ্রুৱ flp = astVarp->fileline();
        ধ্রুৱ DfgDataType& dtype = *DfgDataType::fromAst(astVarp->dtypep());
        ধ্রুৱ std::string prfx = prefix + "_" + astVarp->name();
        DfgVertexVar* ধ্রুৱ vtxp = createTmp(logic, flp, dtype, prfx);
        vtxp->tmpForp(vscp);
        ঘূৰাই vtxp;
    }

    // Convert AstAssign to Dfg, return true if successful.
    // Fills 'updates' with bindings for assigned variables.
    তার্কিক convert(std::vector<std::pair<AstVarScope*, DfgVertexVar*>>& updates, DfgLogic& vtx,
                 Ast_গ্ৰন্থি_Assign* nodep) {
        UASSERT_OBJ(VN_IS(nodep, Assign) || VN_IS(nodep, AssignW), nodep, "Bad NodeAssign");
        UASSERT_OBJ(updates.empty(), nodep, "'updates' should be empty");
        VL_RESTORER(m_updatesp);
        VL_RESTORER(m_logicp);
        m_updatesp = &updates;
        m_logicp = &vtx;
        // Assignment with timing control shouldn't make it this far
        UASSERT_OBJ(!nodep->timingControlp(), nodep, "Shouldn't make it this far");
        // Convert it
        ++m_ctx.m_conv.প্ৰবেশ_Assignments;
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp();
        // Check data types are compatible.
        ধ্রুৱ DfgDataType* ধ্রুৱ lDtypep = DfgDataType::fromAst(lhsp->dtypep());
        ধ্রুৱ DfgDataType* ধ্রুৱ rDtypep = DfgDataType::fromAst(rhsp->dtypep());
        যদি (!lDtypep || !rDtypep) {
            ++m_ctx.m_conv.nonRepDType;
            ঘূৰাই মিছা;
        }
        // For now, only direct array assignment is supported (e.g. a = b, but not a = _ ? b : c)
        যদি (rDtypep->isArray()) {
            যদি (!VN_IS(rhsp, VarRef)
                || !lhsp->dtypep()->skipRefp()->same_বৃক্ষ(rhsp->dtypep()->skipRefp())) {
                ++m_ctx.m_conv.nonRepDType;
                ঘূৰাই মিছা;
            }
        }
        // Widths should match at this point
        UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Mismatched width reached DFG");
        // Convert the RHS expression
        DfgVertex* ধ্রুৱ rVtxp = convertRValue(rhsp);
        যদি (!rVtxp) ঘূৰাই মিছা;
        // Connect the RHS vertex to the LHS targets
        ধ্রুৱ তার্কিক success = convertAssignment(nodep->fileline(), lhsp, rVtxp);
        যদি (success) ++m_ctx.m_conv.representable;
        ঘূৰাই success;
    }

    // Convert RValue expression to Dfg. Returns nullptr if failed.
    DfgVertex* convert(DfgLogic& vtx, Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;
        // Convert it
        ++m_ctx.m_conv.প্ৰবেশ_Expressions;
        DfgVertex* ধ্রুৱ vtxp = convertRValue(nodep);
        যদি (vtxp) ++m_ctx.m_conv.representable;
        ঘূৰাই vtxp;
    }

    // CONSTRUCTOR
    AstToDfgConverter(DfgGraph& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {}
};

// Debug aid - outisde 'AstToDfgSynthesize' as it is a template, but want one instance
V3DebugBisect s_dfgSynthDebugBisect{"DfgSynthesize"};

শ্ৰেণী AstToDfgSynthesize final {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // TYPES

    // SymTab must be ordered in order to yield stable results
    সংরচনা AstVarScopeComparator final {
        স্থির পূর্ণাঙ্ক s_vscpIdCounter;  // Counter for lazily allocating the unique AstVarScope IDs
        তার্কিক operator()(AstVarScope* lhs, AstVarScope* rhs) ধ্রুৱ {
            যদি (!lhs->user4()) lhs->user4(++s_vscpIdCounter);
            যদি (!rhs->user4()) rhs->user4(++s_vscpIdCounter);
            ঘূৰাই lhs->user4() < rhs->user4();
        }
    };
    using SymTab = std::map<AstVarScope*, DfgVertexVar*, AstVarScopeComparator>;

    // Represents a [potentially partial] driver of a variable
    সংরচনা Driver final {
        DfgVertex* m_vtxp = nullptr;  // Driving vertex
        uint32_t m_lo = 0;  // Low index of driven range (internal, not Verilog)
        uint32_t m_hi = 0;  // High index of driven range (internal, not Verilog)
        File_সাৰি* m_flp = nullptr;  // Location of driver in source

        Driver() = স্ৱভাৱ;
        Driver(DfgVertex* vtxp, uint32_t lo, File_সাৰি* flp)
            : m_vtxp{vtxp}
            , m_lo{lo}
            , m_hi{lo + vtxp->size() - 1U}
            , m_flp{flp} {}
        operator তার্কিক() ধ্রুৱ { ঘূৰাই m_vtxp != nullptr; }

        তার্কিক operator<(ধ্রুৱ Driver& other) ধ্রুৱ {
            যদি (m_lo != other.m_lo) ঘূৰাই m_lo < other.m_lo;
            যদি (m_hi != other.m_hi) ঘূৰাই m_hi < other.m_hi;
            ঘূৰাই m_flp->operatorCompare(*other.m_flp) < 0;
        }

        তার্কিক operator<=(ধ্রুৱ Driver& other) ধ্রুৱ { ঘূৰাই !(other < *এতত্); }
    };

    // STATE - Persistent
    DfgGraph& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats
    AstToDfgConverter m_converter;  // The convert instance to use for each construct
    DfgWorklist m_toRevert{m_dfg};  // We need a worklist for reverting synthesis

    // STATE - for current DfgLogic being synthesized
    DfgLogic* m_logicp = nullptr;  // Current logic vertex we are synthesizing
    CfgBlockMap<SymTab> m_bbToISymTab;  // Map from CfgBlock -> input symbol table
    CfgBlockMap<SymTab> m_bbToOSymTab;  // Map from CfgBlock -> output symbol table
    CfgBlockMap<DfgVertexVar*> m_bbToCondp;  // Map from CfgBlock ->  terminating branch condition
    CfgEdgeMap<DfgVertexVar*> m_edgeToPredicatep;  // Map CfgGraphEdge -> path predicate to there
    Cfg_Dominator_বৃক্ষ m_dom_বৃক্ষ;  // The dominator tree of the current CFG

    // STATE - Some debug aid
    // We stop after synthesizing s_dfgSynthDebugLimit vertices (if non-zero).
    // This is the problematic logic (last one we synthesize), assuming a
    // bisection search over s_dfgSynthDebugLimit.
    DfgLogic* m_debugLogicp = nullptr;
    // Source (upstream) cone of outputs of m_debugLogicp
    std::unique_ptr<std::unordered_set<ধ্রুৱ DfgVertex*>> m_debugOSrcConep{nullptr};

    // METHODS

    // Dump current graph for debugging ...
    শূন্য debugDump(ধ্রুৱ ৱর্ণ* name) {
        // If we have the debugged logic, compute the vertices feeding its outputs
        যদি (VL_UNLIKELY(m_debugLogicp)) {
            std::vector<ধ্রুৱ DfgVertex*> outputs;
            m_debugLogicp->foreachSink([&outputs](ধ্রুৱ DfgVertex& v) {
                outputs.emplace_back(v.singleSink()->as<DfgVertexVar>());
                ঘূৰাই মিছা;
            });
            m_debugOSrcConep = m_dfg.sourceCone(outputs);
        }

        যদি (VL_UNLIKELY(dumpDfgLevel() >= 9 || m_debugOSrcConep)) {
            m_dfg.dumpDotFilePrefixed(name);
            যদি (m_debugOSrcConep) {
                // Dump only the subgraph involving the inputs and outputs of the bad vertex
                m_dfg.dumpDotFilePrefixed(name + "-min"s, [&](ধ্রুৱ DfgVertex& v) -> তার্কিক {
                    ঘূৰাই m_debugOSrcConep->গুনতি(&v);
                });
            }
        }
    }

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    template <typename Vertex, typename... Args>
    Vertex* make(Args&&... args) {
        static_assert(!std::is_base_of<DfgVertexVar, Vertex>::মান, "Do not use for variables");
        static_assert(std::is_base_of<DfgVertex, Vertex>::মান, "'Vertex' must be a 'DfgVertex'");
        Vertex* ধ্রুৱ vtxp = নূতন Vertex{m_dfg, std::forward<Args>(args)...};
        যদি (m_logicp) m_logicp->synth().emplace_back(vtxp);
        ঘূৰাই vtxp;
    }

    // Gather all drivers of a resolved variable
    স্থির std::vector<Driver> gatherDrivers(DfgVertexSplice* vtxp) {
        // Collect them all, check if they are sorted
        std::vector<Driver> drivers;
        drivers.reserve(vtxp->nInputs());
        তার্কিক sorted = সঁচা;
        vtxp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_সাৰি* flp) {
            // Collect the driver
            drivers.emplace_back(&src, lo, flp);
            // Check if drivers are sorted - most often they are
            ধ্রুৱ size_t n = drivers.size();
            যদি (n >= 2 && drivers[n - 1] < drivers[n - 2]) sorted = মিছা;
            ঘূৰাই মিছা;
        });

        // Sort if unsorted
        যদি (!sorted) std::stable_sort(drivers.begin(), drivers.end());

        // Done
        ঘূৰাই drivers;
    }

    // Returns true if the driver supplies a complete array element.
    স্থির তার্কিক driverCoversWholeElement(ধ্রুৱ Driver& driver) {
        ধ্রুৱ DfgUnitArray* ধ্রুৱ unitp = driver.m_vtxp->cast<DfgUnitArray>();
        যদি (!unitp) ঘূৰাই মিছা;
        যদি (DfgVertexSplice* ধ্রুৱ splicep = unitp->srcp()->cast<DfgVertexSplice>()) {
            ঘূৰাই splicep->wholep();
        }
        ঘূৰাই সঁচা;
    }

    // Unlike 'wholep', several element drivers can collectively cover an array.
    স্থির তার্কিক spliceCoversWhole(DfgVertexSplice* ধ্রুৱ splicep) {
        যদি (splicep->wholep()) ঘূৰাই সঁচা;
        যদি (splicep->isPacked()) ঘূৰাই মিছা;
        uint32_t next = 0;
        বাবে (ধ্রুৱ Driver& driver : gatherDrivers(splicep)) {
            যদি (driver.m_lo != next || !driverCoversWholeElement(driver)) ঘূৰাই মিছা;
            next = driver.m_hi + 1;
        }
        ঘূৰাই next == splicep->size();
    }

    // Returns true if the driver cone contains any variable introduced by
    // tristate lowering. Used to distinguish intentional tristate contributor
    // overlap from accidental multidrive.
    স্থির তার্কিক containsTriLoweredVar(DfgVertex* rootp) {
        std::vector<ধ্রুৱ DfgVertex*> স্তূপ;
        std::vector<ধ্রুৱ DfgVertex*> visited;
        স্তূপ.emplace_back(rootp);
        যেতিয়ালৈ (!স্তূপ.empty()) {
            ধ্রুৱ DfgVertex* ধ্রুৱ vtxp = স্তূপ.back();
            স্তূপ.বেৰ_কৰো_back();
            যদি (std::find(visited.begin(), visited.end(), vtxp) != visited.end()) অনুৱর্ত;
            visited.emplace_back(vtxp);
            যদি (ধ্রুৱ DfgVertexVar* ধ্রুৱ varp = vtxp->cast<DfgVertexVar>()) {
                যদি (varp->vscp()->varp()->dfgTriLowered()) ঘূৰাই সঁচা;
            }
            vtxp->foreachSource([&](ধ্রুৱ DfgVertex& src) {
                স্তূপ.emplace_back(&src);
                ঘূৰাই মিছা;
            });
        }
        ঘূৰাই মিছা;
    }

    // Returns true if the driver is a direct variable forward (no logic).
    স্থির তার্কিক isDirectVarDriver(ধ্রুৱ DfgVertex* vtxp) { ঘূৰাই vtxp->is<DfgVertexVar>(); }

    // Gather all synthesized drivers of an unresolved variable
    স্থির std::vector<Driver> gatherDriversUnresolved(DfgUnresolved* vtxp) {
        std::vector<Driver> drivers;
        drivers.reserve(vtxp->nInputs());

        // For better locations in error reporting, we unpick concatenations
        // which are sometimes introduced by combinint assignments in V3Const.
        ধ্রুৱ std::function<শূন্য(DfgVertex*, uint32_t, File_সাৰি*)> gather
            = [&](DfgVertex* vtxp, uint32_t lo, File_সাৰি* flp) -> শূন্য {
            যদি (DfgConcat* ধ্রুৱ concatp = vtxp->cast<DfgConcat>()) {
                DfgVertex* ধ্রুৱ rhsp = concatp->rhsp();
                gather(rhsp, lo, rhsp->fileline());
                DfgVertex* ধ্রুৱ lhsp = concatp->lhsp();
                gather(lhsp, lo + rhsp->width(), lhsp->fileline());
                ঘূৰাই;
            }
            drivers.emplace_back(vtxp, lo, flp);
        };

        // Gather all synthesized drivers
        vtxp->foreachSource([&](DfgVertex& src) {
            // Can ignore the original DfgLogic
            যদি (src.is<DfgLogic>()) ঘূৰাই মিছা;
            // Synthesized drivers must be a splice at this point
            DfgVertexSplice* ধ্রুৱ splicep = src.as<DfgVertexSplice>();
            // Collect the driver
            splicep->foreachDriver([&](DfgVertex& src, uint32_t lo, File_সাৰি* flp) {
                gather(&src, lo, flp);
                ঘূৰাই মিছা;
            });
            ঘূৰাই মিছা;
        });

        // Sort the drivers
        std::stable_sort(drivers.begin(), drivers.end());

        // Done
        ঘূৰাই drivers;
    }

    // Given two drivers, combine the driven sub-ranges into the first one if
    // possible. First bool returned indicates successfully combined and there
    // are no multi-driven bits. Second bool returned indicates we warned
    // already about multi-driven bits.
    std::pair<তার্কিক, তার্কিক> combineDrivers(DfgVertexVar& var, ধ্রুৱ std::string& sub,  //
                                         Driver& a, ধ্রুৱ Driver& b) {
        // We can only combine array drivers ...
        যদি (a.m_vtxp->isPacked()) ঘূৰাই {মিছা, মিছা};
        // ... that both drive a single element ...
        যদি (a.m_lo != b.m_lo) ঘূৰাই {মিছা, মিছা};
        ধ্রুৱ DfgUnitArray* ধ্রুৱ aUap = a.m_vtxp->template cast<DfgUnitArray>();
        যদি (!aUap) ঘূৰাই {মিছা, মিছা};
        ধ্রুৱ DfgUnitArray* ধ্রুৱ bUap = b.m_vtxp->template cast<DfgUnitArray>();
        যদি (!bUap) ঘূৰাই {মিছা, মিছা};
        // ... and are themeselves partial
        DfgSplicePacked* ধ্রুৱ aSp = aUap->srcp()->template cast<DfgSplicePacked>();
        যদি (!aSp) ঘূৰাই {মিছা, মিছা};
        DfgSplicePacked* ধ্রুৱ bSp = bUap->srcp()->template cast<DfgSplicePacked>();
        যদি (!bSp) ঘূৰাই {মিছা, মিছা};
        UASSERT_OBJ(aSp->dtype() == bSp->dtype(), &var, "DTypes should match");

        // Gather drivers of a
        std::vector<Driver> aDrivers = gatherDrivers(aSp);
        // Gather drivers of b
        std::vector<Driver> bDrivers = gatherDrivers(bSp);

        // Merge them
        std::vector<Driver> abDrivers;
        abDrivers.reserve(aDrivers.size() + bDrivers.size());
        std::merge(  //
            aDrivers.begin(), aDrivers.end(),  //
            bDrivers.begin(), bDrivers.end(),  //
            std::back_inserter(abDrivers)  //
        );

        // Attempt to resolve them
        যদি (!normalizeDrivers(var, abDrivers, sub + "[" + std::to_string(a.m_lo) + "]")) {
            ঘূৰাই {মিছা, সঁচা};
        }

        // Successfully resolved. Needs a new splice and unit.
        File_সাৰি* ধ্রুৱ flp = var.fileline();
        DfgSplicePacked* ধ্রুৱ splicep = make<DfgSplicePacked>(flp, aSp->dtype());
        বাবে (ধ্রুৱ Driver& d : abDrivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        DfgUnitArray* ধ্রুৱ uap = make<DfgUnitArray>(flp, aUap->dtype());
        uap->srcp(splicep);
        a.m_vtxp = uap;
        ঘূৰাই {সঁচা, মিছা};
    }

    // Combine and coalesce the given drivers.
    // Returns true iff no multi-driven bits are present.
    তার্কিক normalizeDrivers(DfgVertexVar& var, std::vector<Driver>& drivers,
                          ধ্রুৱ std::string& sub = "") {
        যদি (drivers.empty()) ঘূৰাই সঁচা;

        // What type of values are we combining
        ধ্রুৱ তার্কিক isPacked = drivers[0].m_vtxp->isPacked();

        // Found a multidriven part ?
        তার্কিক multiDriven = মিছা;

        // Iterate through the sorted drivers. Index 'i' is the driver we are
        // resolving driver 'j' agains, and if required, we merge 'j' into 'i'.
        size_t i = 0;
        বাবে (size_t j = 1; j < drivers.size();) {
            UASSERT_OBJ(i < j, &var, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            যদি (!jD) {
                ++j;
                অনুৱর্ত;
            }
            // If 'i' was moved, move 'j' in it's place
            যদি (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                অনুৱর্ত;
            }

            // We have 2 valid drivers now
            UASSERT_OBJ(iD <= jD, &var, "Should always be sorted");
            UASSERT_OBJ(jD.m_vtxp->isPacked() == isPacked, &var, "Mixed type drivers");

            // If no overlap, consider next pair
            যদি (iD.m_hi < jD.m_lo) {
                ++i;
                যদি (i == j) ++j;
                অনুৱর্ত;
            }

            // There is an overlap. Attempt to combine them.
            তার্কিক combined = মিছা;
            তার্কিক warned = মিছা;
            std::tie(combined, warned) = combineDrivers(var, sub, iD, jD);

            // If sucessfully combined, 'j' is no longer needed, it was combined into 'i'
            যদি (combined) {
                jD = Driver{};
                ++j;
                অনুৱর্ত;
            }

            // Found overlap that cannot be resolved
            multiDriven = সঁচা;
            // Compare next driver
            ++j;

            // Do not warn again if we warned during 'combineDrivers'
            যদি (warned) অনুৱর্ত;

            // The variable to warn on
            AstVarScope* ধ্রুৱ vscp = var.tmpForp() ? var.tmpForp() : var.vscp();

            // Loop index often abused, so suppress
            যদি (vscp->varp()->isUsedLoopIdx()) অনুৱর্ত;
            // Tristate lowering can intentionally create overlapping contributors.
            // Keep the signal marked multidriven for DFG fallback, but suppress
            // warning only when both overlapping driver cones look tri-lowered.
            যদি (vscp->varp()->dfgAllowMultidriveTri()) {
                ধ্রুৱ তার্কিক iTri = containsTriLoweredVar(iD.m_vtxp);
                ধ্রুৱ তার্কিক jTri = containsTriLoweredVar(jD.m_vtxp);
                ধ্রুৱ তার্কিক triPair = iTri && jTri;
                ধ্রুৱ তার্কিক triAndBridge = (iTri && isDirectVarDriver(jD.m_vtxp))
                                          || (jTri && isDirectVarDriver(iD.m_vtxp));
                যদি (triPair || triAndBridge) অনুৱর্ত;
            }

            // Warn the user now
            ধ্রুৱ std::string lo = std::to_string(jD.m_lo);
            ধ্রুৱ std::string hi = std::to_string(std::min(iD.m_hi, jD.m_hi));
            ধ্রুৱ std::string kind = isPacked ? "Bit" : "Element";
            ধ্রুৱ std::string part = hi == lo ? (" [" + lo + "]") : ("s [" + hi + ":" + lo + "]");

            vscp->v3warn(  //
                MULTIDRIVEN,  //
                kind << part << " of signal '" << vscp->prettyName() << sub << "'"
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
        যদি (!multiDriven && isPacked) coalesceDrivers(drivers);

        ঘূৰাই !multiDriven;
    }

    // Coalesce adjacent drivers into single ones
    শূন্য coalesceDrivers(std::vector<Driver>& drivers) {
        UASSERT(!drivers.empty(), "Can't coalesce 0 drivers");
        UASSERT_OBJ(drivers[0].m_vtxp->isPacked(), drivers[0].m_vtxp, "Can only coalesce packed");

        size_t i = 0;
        বাবে (size_t j = 1; j < drivers.size();) {
            UASSERT(i < j, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            যদি (!jD) {
                ++j;
                অনুৱর্ত;
            }
            // If 'i' was moved, move 'j' in it's place
            যদি (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                অনুৱর্ত;
            }

            // We have 2 valid drivers now
            UASSERT(iD <= jD, "Should always be sorted");

            // If not adjacent, move on
            যদি (iD.m_hi + 1 != jD.m_lo) {
                ++i;
                যদি (i == j) ++j;
                অনুৱর্ত;
            }

            // Coalesce Adjacent ranges,
            ধ্রুৱ DfgDataType& dt = DfgDataType::packed(iD.m_vtxp->width() + jD.m_vtxp->width());
            DfgConcat* ধ্রুৱ concatp = make<DfgConcat>(iD.m_flp, dt);
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
    DfgVertexSplice* makeSplice(DfgVertexVar& var, ধ্রুৱ std::vector<Driver>& newDrivers) {
        UASSERT_OBJ(!newDrivers.empty(), &var, "'makeSplice' called with no new drivers");
        // Create new driver
        DfgVertexSplice* splicep = nullptr;
        যদি (var.is<DfgVarPacked>()) {
            splicep = make<DfgSplicePacked>(var.fileline(), var.dtype());
        } অন্যথা যদি (var.is<DfgVarArray>()) {
            splicep = make<DfgSpliceArray>(var.fileline(), var.dtype());
        } অন্যথা {
            var.v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        বাবে (ধ্রুৱ Driver& d : newDrivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        ঘূৰাই splicep;
    }

    // Initialzie input symbol table of entry CfgBlock
    শূন্য initialize_Entry_প্ৰতীক_Table(SymTab& iSymTab) {
        m_logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* ধ্রুৱ vvp = src.as<DfgVertexVar>();
            iSymTab[vvp->vscp()] = vvp;
            ঘূৰাই মিছা;
        });
    }

    // Join variable drivers across a control flow confluence (insert muxes ...)
    DfgVertexVar* joinDrivers(AstVarScope* vscp, DfgVertexVar* predicatep,  //
                              DfgVertexVar* thenp, DfgVertexVar* elsep) {
        AstVarScope* ধ্রুৱ thenVscp = thenp->tmpForp() ? thenp->tmpForp() : thenp->vscp();
        AstVarScope* ধ্রুৱ elseVscp = elsep->tmpForp() ? elsep->tmpForp() : elsep->vscp();
        UASSERT_OBJ(thenVscp == elseVscp, vscp, "Attempting to join unrelated variables");

        // If both bindings are the the same (variable not updated through either path),
        // then there is nothing to do, can use the same binding
        যদি (thenp == elsep) ঘূৰাই thenp;

        // We can't join the input variable just yet, so bail
        যদি (thenp->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_প্ৰবেশ;
            ঘূৰাই nullptr;
        }
        যদি (elsep->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_প্ৰবেশ;
            ঘূৰাই nullptr;
        }

        // Can't do arrays yet
        যদি (!thenp->isPacked()) {
            ++m_ctx.m_synt.nonSynArray;
            ঘূৰাই nullptr;
        }

        // Gather drivers of 'thenp' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* ধ্রুৱ tDefaultp = thenp->defaultp();
        std::vector<Driver> tDrivers = gatherDrivers(thenp->srcp()->as<DfgVertexSplice>());

        // Gather drivers of 'elsep' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* ধ্রুৱ eDefaultp = elsep->defaultp();
        std::vector<Driver> eDrivers = gatherDrivers(elsep->srcp()->as<DfgVertexSplice>());

        // Default drivers should be the same or not present on either
        UASSERT_OBJ(tDefaultp == eDefaultp, vscp, "Different default drivers");

        // Location to use for the join vertices
        File_সাৰি* ধ্রুৱ flp = predicatep->fileline();

        // Create a fresh temporary for the joined value
        DfgVertexVar* ধ্রুৱ joinp = m_converter.createTmp(*m_logicp, vscp, "SynthJoin");
        DfgVertexSplice* ধ্রুৱ joinSplicep = make<DfgSplicePacked>(flp, joinp->dtype());
        joinp->srcp(joinSplicep);

        // If both paths are fully driven, just create a simple conditional
        যদি (tDrivers.size() == 1  //
            && tDrivers[0].m_lo == 0  //
            && tDrivers[0].m_hi == thenp->width() - 1  //
            && eDrivers.size() == 1  //
            && eDrivers[0].m_lo == 0  //
            && eDrivers[0].m_hi == elsep->width() - 1) {
            UASSERT_OBJ(!tDefaultp, vscp, "Fully driven variable have default driver");

            DfgCond* ধ্রুৱ condp = make<DfgCond>(flp, joinp->dtype());
            condp->condp(predicatep);
            condp->thenp(thenp);
            condp->elsep(elsep);
            joinSplicep->addDriver(condp, 0, tDrivers[0].m_flp);

            // Done
            ঘূৰাই joinp;
        }

        // Otherwise we need to merge them part by part

        // If different bits are driven, then some might not have been assigned.. Latch?
        যদি (tDrivers.size() != eDrivers.size()) {
            ++m_ctx.m_synt.nonSynLatch;
            ঘূৰাই nullptr;
        }

        বাবে (size_t i = 0; i < tDrivers.size(); ++i) {
            ধ্রুৱ Driver& tDriver = tDrivers[i];
            ধ্রুৱ Driver& eDriver = eDrivers[i];
            // If different bits are driven, then some might not have been assigned.. Latch?
            যদি (tDriver.m_lo != eDriver.m_lo || tDriver.m_hi != eDriver.m_hi) {
                ++m_ctx.m_synt.nonSynLatch;
                ঘূৰাই nullptr;
            }

            ধ্রুৱ DfgDataType& dtype = DfgDataType::packed(tDriver.m_hi - tDriver.m_lo + 1);
            DfgCond* ধ্রুৱ condp = make<DfgCond>(flp, dtype);
            condp->condp(predicatep);

            // We actally need to select the bits from the joined variables, not use the drivers
            DfgSel* ধ্রুৱ thenSelp = make<DfgSel>(flp, tDriver.m_vtxp->dtype());
            thenSelp->lsb(tDriver.m_lo);
            thenSelp->fromp(thenp);
            condp->thenp(thenSelp);

            // Same for the 'else' part
            DfgSel* ধ্রুৱ elseSelp = make<DfgSel>(flp, eDriver.m_vtxp->dtype());
            elseSelp->lsb(eDriver.m_lo);
            elseSelp->fromp(elsep);
            condp->elsep(elseSelp);

            // Add it as a driver to the join
            joinSplicep->addDriver(condp, tDriver.m_lo, tDriver.m_flp);
        }

        // If there was a default driver, add it to te join
        যদি (tDefaultp) joinp->defaultp(tDefaultp);

        // Done
        ঘূৰাই joinp;
    }

    // Merge 'thenSymTab' into 'elseSymTab' using the given predicate to join values
    তার্কিক join_প্ৰতীক_Tables(SymTab& elseSymTab, DfgVertexVar* predicatep, ধ্রুৱ SymTab& thenSymTab) {
        // Any variable that does not have a binding on both paths will be removed. These might be
        // temporaries, loop vars, etc used only in one branch. Conversion will fail if the
        // variable is actually referenced later.
        std::vector<AstVarScope*> toRemove;

        // Join each symbol
        বাবে (std::pair<AstVarScope* ধ্রুৱ, DfgVertexVar*>& pair : elseSymTab) {
            AstVarScope* ধ্রুৱ varp = pair.first;
            // Find same variable on the else path
            ধ্রুৱ auto it = thenSymTab.find(varp);
            // Record for removal if not assigned on both paths
            যদি (it == thenSymTab.end()) {
                toRemove.emplace_back(varp);
                অনুৱর্ত;
            }
            // Join paths with the predicate
            DfgVertexVar* ধ্রুৱ thenp = it->second;
            DfgVertexVar* ধ্রুৱ elsep = pair.second;
            DfgVertexVar* ধ্রুৱ newp = joinDrivers(varp, predicatep, thenp, elsep);
            যদি (!newp) ঘূৰাই মিছা;
            pair.second = newp;
        }

        // Remove variables not assigned on both paths
        বাবে (AstVarScope* ধ্রুৱ varp : toRemove) elseSymTab.erase(varp);

        // Done
        ঘূৰাই সঁচা;
    }

    // Given two joining control flow edges, compute how to join their symbols.
    // Returns the predicaete to join over, and the 'then' and 'else' blocks.
    std::tuple<DfgVertexVar*, ধ্রুৱ CfgBlock*, ধ্রুৱ CfgBlock*>  //
    howToJoin(ধ্রুৱ CfgEdge* ধ্রুৱ ap, ধ্রুৱ CfgEdge* ধ্রুৱ bp) {
        // Find the closest common dominator of the two paths
        ধ্রুৱ CfgBlock* ধ্রুৱ domp = m_dom_বৃক্ষ.closestCommonDominator(ap->srcp(), bp->srcp());
        // These paths join here, so 'domp' must be a branch, otherwise it's not the closest
        UASSERT_OBJ(domp->isBranch(), domp, "closestCommonDominator is not a branch");

        // The branches of the common dominator
        ধ্রুৱ CfgEdge* ধ্রুৱ takenEdgep = domp->takenEdgep();
        ধ্রুৱ CfgEdge* ধ্রুৱ untknEdgep = domp->untknEdgep();

        // We check if the taken branch dominates the path to either blocks,
        // and if the untaken branch dominates the path to the other block.
        // If so, we can use the branch condition as predicate, otherwise
        // we must use the path predicate as there are ways to get from one
        // branch of the dominator to the other. We need to be careful if
        // either branches are directly to the join block. This is fine,
        // it's as if there was an empty block on that critical edge which
        // is dominated by that path.

        যদি (takenEdgep == ap || m_dom_বৃক্ষ.dominates(takenEdgep->dstp(), ap->srcp())) {
            যদি (untknEdgep == bp || m_dom_বৃক্ষ.dominates(untknEdgep->dstp(), bp->srcp())) {
                // Taken path dominates 'ap' and untaken dominates 'bp', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                ঘূৰাই std::make_tuple(m_bbToCondp[domp], ap->srcp(), bp->srcp());
            }
        } অন্যথা যদি (takenEdgep == bp || m_dom_বৃক্ষ.dominates(takenEdgep->dstp(), bp->srcp())) {
            যদি (untknEdgep == ap || m_dom_বৃক্ষ.dominates(untknEdgep->dstp(), ap->srcp())) {
                // Taken path dominates 'bp' and untaken dominates 'ap', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                ঘূৰাই std::make_tuple(m_bbToCondp[domp], bp->srcp(), ap->srcp());
            }
        }

        // The branches don't dominate the joined blocks, must use the path predicate
        ++m_ctx.m_synt.joinUsingPathPredicate;

        // TODO: We could do better here: use the path predicate of the closest
        // cominating blocks, pick the one from the lower rank, etc, but this
        // generic case is very rare, most synthesizable logic has
        // series-parallel CFGs which are covered by the earlier cases.
        ঘূৰাই std::make_tuple(m_edgeToPredicatep[ap], ap->srcp(), bp->srcp());
    }

    // Combine the output symbol tables of the predecessors of the given
    // block to compute the input symtol table for the given block.
    তার্কিক create_প্ৰবেশ_প্ৰতীক_Table(ধ্রুৱ CfgBlock& bb) {
        // The input symbol table of the given block, we are computing it now
        SymTab& joined = m_bbToISymTab[bb];

        // Input symbol table of entry block is special
        যদি (bb.isEnter()) {
            initialize_Entry_প্ৰতীক_Table(joined);
            ঘূৰাই সঁচা;
        }

        // Current input symbol table should be empty, we will fill it in here
        UASSERT(joined.empty(), "Unprocessed input symbol table should be empty");

        // Fast path if there is only one predecessor - TODO: use less copying
        যদি (!bb.isJoin()) {
            joined = m_bbToOSymTab[bb.firstPredecessorp()];
            ঘূৰাই সঁচা;
        }

        // We also have a simpler job if there are 2 predecessors
        যদি (bb.isTwoWayJoin()) {
            DfgVertexVar* predicatep = nullptr;
            ধ্রুৱ CfgBlock* thenp = nullptr;
            ধ্রুৱ CfgBlock* elsep = nullptr;
            std::tie(predicatep, thenp, elsep)
                = howToJoin(bb.firstPredecessorEdgep(), bb.lastPredecessorEdgep());
            // Copy from else
            joined = m_bbToOSymTab[elsep];
            // Join with then
            ঘূৰাই join_প্ৰতীক_Tables(joined, predicatep, m_bbToOSymTab[*thenp]);
        }

        // General hard way

        // Gather predecessors
        সংরচনা Predecessor final {
            ধ্রুৱ CfgBlock* m_bbp;  // Predeccessor block
            DfgVertexVar* m_predicatep;  // Predicate predecessor reached this block with
            ধ্রুৱ SymTab* m_oSymTabp;  // Output symbol table or predecessor
            Predecessor() = delete;
            Predecessor(ধ্রুৱ CfgBlock* bbp, DfgVertexVar* predicatep, ধ্রুৱ SymTab* oSymTabp)
                : m_bbp{bbp}
                , m_predicatep{predicatep}
                , m_oSymTabp{oSymTabp} {}
        };

        ধ্রুৱ std::vector<Predecessor> predecessors = [&]() {
            std::vector<Predecessor> res;
            বাবে (ধ্রুৱ V3GraphEdge& edge : bb.inEdges()) {
                ধ্রুৱ CfgEdge& cfgEdge = static_cast<ধ্রুৱ CfgEdge&>(edge);
                ধ্রুৱ CfgBlock* ধ্রুৱ predecessorp = cfgEdge.srcp();
                DfgVertexVar* ধ্রুৱ predicatep = m_edgeToPredicatep[cfgEdge];
                ধ্রুৱ SymTab* ধ্রুৱ oSymTabp = &m_bbToOSymTab[predecessorp];
                res.emplace_back(predecessorp, predicatep, oSymTabp);
            }
            // Sort predecessors reverse topologically. This way earlier blocks
            // will come after later blocks, and the entry block is last if present.
            std::sort(res.begin(), res.end(), [](ধ্রুৱ Predecessor& a, ধ্রুৱ Predecessor& b) {  //
                ঘূৰাই *a.m_bbp > *b.m_bbp;
            });
            ঘূৰাই res;
        }();

        // Start by copying the bindings from the frist predecessor
        joined = *predecessors[0].m_oSymTabp;
        // Join over all other predecessors
        বাবে (size_t i = 1; i < predecessors.size(); ++i) {
            DfgVertexVar* ধ্রুৱ predicatep = predecessors[i].m_predicatep;
            ধ্রুৱ SymTab& oSymTab = *predecessors[i].m_oSymTabp;
            যদি (!join_প্ৰতীক_Tables(joined, predicatep, oSymTab)) ঘূৰাই মিছা;
        }

        ঘূৰাই সঁচা;
    }

    std::vector<Driver> computePropagatedDrivers(ধ্রুৱ std::vector<Driver>& newDrivers,
                                                 DfgVertexVar* oldp) {
        // Gather drivers of 'oldp' - they are in incresing range order with no overlaps
        ধ্রুৱ std::vector<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.empty(), oldp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::vector<Driver> propagatedDrivers;

        // Add bits between 'msb' and 'lsb' from 'oldp' to 'pDrivers'
        ধ্রুৱ auto addOldDriver = [&](File_সাৰি* ধ্রুৱ flp, uint32_t msb, uint32_t lsb) {
            UASSERT_OBJ(propagatedDrivers.empty() || lsb > propagatedDrivers.back().m_hi, flp,
                        "Drivers should be in ascending order");
            DfgSel* ধ্রুৱ selp = make<DfgSel>(flp, DfgDataType::packed(msb - lsb + 1));
            selp->lsb(lsb);
            selp->fromp(oldp);
            propagatedDrivers.emplace_back(selp, lsb, flp);
        };

        // Incorporate old drivers
        বাবে (ধ্রুৱ Driver& oDriver : oldDrivers) {
            File_সাৰি* ধ্রুৱ flp = oDriver.m_flp;
            // Range to consider inserting, we will adjust oldLo as we process drivers
            uint32_t oldLo = oDriver.m_lo;
            ধ্রুৱ uint32_t oldHi = oDriver.m_hi;

            // Loop for now, can move to bisection search if this is a problem, shouldn't be ...
            বাবে (ধ্রুৱ Driver& nDriver : newDrivers) {
                UASSERT_OBJ(oldHi >= oldLo, flp, "Should have stopped iteration");
                // If new driver is entirely below old driver, move on to
                যদি (nDriver.m_hi < oldLo) অনুৱর্ত;
                // If new driver is entirely above old driver, we can stop
                যদি (oldHi < nDriver.m_lo) ভঙ্গ;

                // There is an overlap between 'oDriver' and 'nDriver'.
                // Insert the low bits and adjust the insertion range.
                // The rest will take care of itself on subsequent iterations.
                যদি (oldLo < nDriver.m_lo) addOldDriver(flp, nDriver.m_lo - 1, oldLo);
                oldLo = nDriver.m_hi + 1;

                // Stop if no more bits remaining in the old driver
                যদি (oldLo > oldHi) ভঙ্গ;
            }

            // Insert remaining bits if any
            যদি (oldHi >= oldLo) addOldDriver(flp, oldHi, oldLo);
        }

        ঘূৰাই propagatedDrivers;
    }

    // Propagate whole elements with a linear walk over the sorted driver lists.
    // Reads use the previous value temporary, preserving assignment-version bindings.
    // Partial elements and non-unit drivers retain the nonsynthesized-process fallback.
    তার্কিক computePropagatedArrayDrivers(ধ্রুৱ std::vector<Driver>& newDrivers,
                                       DfgVertexVar* ধ্রুৱ oldp,
                                       std::vector<Driver>& propagatedDrivers) {
        // Bound quadratic vertex growth: array drivers cannot be coalesced.
        স্থির constexpr uint32_t MAX_ARRAY_ELEMENTS = 32;
        যদি (oldp->size() > MAX_ARRAY_ELEMENTS) ঘূৰাই মিছা;
        বাবে (ধ্রুৱ Driver& driver : newDrivers) {
            যদি (!driverCoversWholeElement(driver)) ঘূৰাই মিছা;
        }
        ধ্রুৱ std::vector<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.empty(), oldp, "Should have a proper driver");
        বাবে (ধ্রুৱ Driver& driver : oldDrivers) {
            যদি (!driverCoversWholeElement(driver)) ঘূৰাই মিছা;
        }

        propagatedDrivers.reserve(oldDrivers.size());
        auto nIt = newDrivers.begin();
        বাবে (ধ্রুৱ Driver& oDriver : oldDrivers) {
            যেতিয়ালৈ (nIt != newDrivers.end() && nIt->m_lo < oDriver.m_lo) ++nIt;
            যদি (nIt != newDrivers.end() && nIt->m_lo == oDriver.m_lo) অনুৱর্ত;

            File_সাৰি* ধ্রুৱ flp = oDriver.m_flp;
            ধ্রুৱ DfgUnitArray* ধ্রুৱ oldUnitp = oDriver.m_vtxp->as<DfgUnitArray>();
            DfgArraySel* ধ্রুৱ selp = make<DfgArraySel>(flp, oldUnitp->srcp()->dtype());
            selp->fromp(oldp);
            selp->bitp(make<DfgConst>(flp, static_cast<size_t>(VL_IDATASIZE), oDriver.m_lo));
            DfgUnitArray* ধ্রুৱ newUnitp = make<DfgUnitArray>(flp, oldUnitp->dtype());
            newUnitp->srcp(selp);
            propagatedDrivers.emplace_back(newUnitp, oDriver.m_lo, flp);
        }
        ঘূৰাই সঁচা;
    }

    // Given the drivers of a variable after converting a single statement
    // 'newp', add drivers from 'oldp' that were not reassigned be drivers
    // in newp. This computes the total result of all previous assignments.
    তার্কিক incorporate_Previous_মান(AstVarScope* vscp, DfgVertexVar* newp, DfgVertexVar* oldp) {
        UASSERT_OBJ(newp->srcp(), vscp, "Assigned variable has no driver");

        // Easy if there is no old value...
        যদি (!oldp) ঘূৰাই সঁচা;

        // New driver was not yet coalesced, so should always be a splice
        DfgVertexSplice* ধ্রুৱ nSplicep = newp->srcp()->as<DfgVertexSplice>();

        // If the old value is the real variable we just computed the new value for,
        // then it is the circular feedback into the synthesized block, add it as default driver.
        যদি (oldp->vscp() == vscp) {
            যদি (!spliceCoversWhole(nSplicep)) newp->defaultp(oldp);
            ঘূৰাই সঁচা;
        }

        UASSERT_OBJ(oldp->srcp(), vscp, "Previously assigned variable has no driver");

        // Gather drivers of 'newp' - they are in incresing range order with no overlaps
        UASSERT_OBJ(!newp->defaultp(), newp, "Converted value should not have default");
        std::vector<Driver> nDrivers = gatherDrivers(newp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!nDrivers.empty(), newp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::vector<Driver> pDrivers;
        যদি (newp->isPacked()) {
            pDrivers = computePropagatedDrivers(nDrivers, oldp);
        } অন্যথা যদি (!computePropagatedArrayDrivers(nDrivers, oldp, pDrivers)) {
            ++m_ctx.m_synt.nonSynArray;
            ঘূৰাই মিছা;
        }

        যদি (!pDrivers.empty()) {
            // Need to merge propagated sources, so reset the splice
            nSplicep->resetDrivers();
            // Merge drivers - they are both sorted and non-overlapping
            std::vector<Driver> drivers;
            drivers.reserve(nDrivers.size() + pDrivers.size());
            std::merge(nDrivers.begin(), nDrivers.end(), pDrivers.begin(), pDrivers.end(),
                       std::back_inserter(drivers));
            // Coalesce adjacent ranges
            যদি (newp->isPacked()) coalesceDrivers(drivers);
            // Reinsert drivers in order
            বাবে (ধ্রুৱ Driver& d : drivers) nSplicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        }

        // If the old had a default, add to the new one too, unless redundant
        যদি (oldp->defaultp() && !spliceCoversWhole(nSplicep)) newp->defaultp(oldp->defaultp());

        // Done
        ঘূৰাই সঁচা;
    }

    // Synthesize the given statements with the given input symbol table.
    // Returns true if successfolly synthesized.
    // Populates the given output symbol table.
    // Populates the given reference with the condition of the terminator branch, if any.
    তার্কিক synthesizeBasicBlock(SymTab& oSymTab, DfgVertex*& condpr,
                              ধ্রুৱ std::vector<Ast_গ্ৰন্থি_Stmt*>& stmtps, ধ্রুৱ SymTab& iSymTab) {
        // Use fresh set of vertices in m_converter
        ধ্রুৱ VNUser2InUse user2InUse;

        // Initialize AstVarScope -> Vertex bindings available in this block
        বাবে (ধ্রুৱ auto& pair : iSymTab) {
            AstVarScope* ধ্রুৱ varp = pair.first;
            DfgVertexVar* ধ্রুৱ vtxp = pair.second;
            varp->user2p(vtxp);
            oSymTab[varp] = vtxp;
        }

        // Synthesize each statement one after the other
        std::vector<std::pair<AstVarScope*, DfgVertexVar*>> updates;
        বাবে (Ast_গ্ৰন্থি_Stmt* ধ্রুৱ stmtp : stmtps) {
            // Regular statements
            Ast_গ্ৰন্থি_Assign* ধ্রুৱ ap = VN_CAST(stmtp, গ্ৰন্থি_Assign);
            যদি (ap && (VN_IS(ap, Assign) || VN_IS(ap, AssignW))) {
                // Convert this assignment
                যদি (!m_converter.convert(updates, *m_logicp, ap)) {
                    ++m_ctx.m_synt.nonSynConv;
                    ঘূৰাই মিছা;
                }
                // Apply variable updates from this statement
                বাবে (ধ্রুৱ auto& pair : updates) {
                    // The target variable that was assigned to
                    AstVarScope* ধ্রুৱ vscp = pair.first;
                    // The new, potentially partially assigned value
                    DfgVertexVar* ধ্রুৱ newp = pair.second;
                    // Normalize drivers within this statement, bail if multidriven
                    DfgVertexSplice* ধ্রুৱ srcp = newp->srcp()->as<DfgVertexSplice>();
                    std::vector<Driver> drivers = gatherDrivers(srcp);
                    ধ্রুৱ তার্কিক single = drivers.size() == 1;
                    যদি (!normalizeDrivers(*newp, drivers)) {
                        ++m_ctx.m_synt.nonSynMultidrive;
                        ঘূৰাই মিছা;
                    }
                    // If there were more than one driver (often not), re-add in case coalesced
                    যদি (!single) {
                        srcp->resetDrivers();
                        বাবে (ধ্রুৱ Driver& d : drivers) srcp->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
                    }
                    // The old value, if any
                    DfgVertexVar* ধ্রুৱ oldp = vscp->user2u().template to<DfgVertexVar*>();
                    // Inncorporate old value into the new value
                    যদি (!incorporate_Previous_মান(vscp, newp, oldp)) ঘূৰাই মিছা;
                    // Update binding of target variable
                    vscp->user2p(newp);
                    // Update output symbol table of this block
                    oSymTab[vscp] = newp;
                }
                updates.clear();
                অনুৱর্ত;
            }

            // Terminator branches
            যদি (AstIf* ধ্রুৱ ifp = VN_CAST(stmtp, If)) {
                UASSERT_OBJ(ifp == stmtps.back(), ifp, "Branch should be last statement");
                // Convert condition, give up if failed
                DfgVertex* condp = m_converter.convert(*m_logicp, ifp->condp());
                যদি (!condp) {
                    ++m_ctx.m_synt.nonSynConv;
                    ঘূৰাই মিছা;
                }
                // Single bit condition can be use directly, otherwise: use 'condp != 0'
                যদি (condp->width() != 1) {
                    File_সাৰি* ধ্রুৱ flp = condp->fileline();
                    DfgNeq* ধ্রুৱ neqp = make<DfgNeq>(flp, DfgDataType::packed(1));
                    neqp->lhsp(make<DfgConst>(flp, condp->width(), 0U));
                    neqp->rhsp(condp);
                    condp = neqp;
                }
                condpr = condp;
                অনুৱর্ত;
            }

            // Unhandled
            ++m_ctx.m_synt.nonSynStmt;
            ঘূৰাই মিছা;
        }

        ঘূৰাই সঁচা;
    }

    // Assign path predicates to the outgoing control flow edges of the given block
    শূন্য assignPathPredicates(ধ্রুৱ CfgBlock& bb) {
        // Nothing to do for the exit block
        যদি (bb.isExit()) ঘূৰাই;

        // Get the predicate of this block
        DfgVertex* ধ্রুৱ predp = [&]() -> DfgVertex* {
            // Entry block has no predecessors, use constant true
            যদি (bb.isEnter()) ঘূৰাই make<DfgConst>(m_logicp->fileline(), 1U, 1U);

            // For any other block, 'or' together all the incoming predicates
            ধ্রুৱ auto& inEdges = bb.inEdges();
            auto it = inEdges.begin();
            DfgVertex* resp = m_edgeToPredicatep[static_cast<ধ্রুৱ CfgEdge&>(*it)];
            যেতিয়ালৈ (++it != inEdges.end()) {
                DfgOr* ধ্রুৱ orp = make<DfgOr>(resp->fileline(), resp->dtype());
                orp->rhsp(resp);
                orp->lhsp(m_edgeToPredicatep[static_cast<ধ্রুৱ CfgEdge&>(*it)]);
                resp = orp;
            }
            ঘূৰাই resp;
        }();

        ধ্রুৱ DfgDataType& dtype = predp->dtype();

        ধ্রুৱ auto mkTmp = [&](File_সাৰি* flp, ধ্রুৱ ৱর্ণ* name, DfgVertex* srcp) {
            ধ্রুৱ std::string prefix = "_BB" + std::to_string(bb.id()) + "_" + name;
            DfgVertexVar* ধ্রুৱ tmpp = m_converter.createTmp(*m_logicp, flp, dtype, prefix);
            tmpp->srcp(srcp);
            ঘূৰাই tmpp;
        };

        // Assign it to a variable in case it's used multiple times
        DfgVertexVar* ধ্রুৱ pInp = mkTmp(predp->fileline(), "PathIn", predp);

        // For uncondional branches, the successor predicate edge is the same
        যদি (!bb.isBranch()) {
            m_edgeToPredicatep[bb.takenEdgep()] = mkTmp(pInp->fileline(), "Goto", pInp);
            ঘূৰাই;
        }

        // For branches, we need to factor in the branch condition
        DfgVertex* ধ্রুৱ condp = m_bbToCondp[bb];
        File_সাৰি* ধ্রুৱ flp = condp->fileline();

        // Predicate for taken branch: 'predp & condp'
        {
            DfgAnd* ধ্রুৱ takenPredp = make<DfgAnd>(flp, dtype);
            takenPredp->lhsp(pInp);
            takenPredp->rhsp(condp);
            m_edgeToPredicatep[bb.takenEdgep()] = mkTmp(flp, "Taken", takenPredp);
        }

        // Predicate for untaken branch: 'predp & ~condp'
        {
            DfgAnd* ধ্রুৱ untknPredp = make<DfgAnd>(flp, dtype);
            untknPredp->lhsp(pInp);
            DfgNot* ধ্রুৱ notp = make<DfgNot>(flp, dtype);
            notp->srcp(condp);
            untknPredp->rhsp(notp);
            m_edgeToPredicatep[bb.untknEdgep()] = mkTmp(flp, "Untkn", untknPredp);
        }
    }

    // Returns true if all external updates to volatile variables are observed correctly
    তার্কিক checkExtWrites() {
        বাবে (ধ্রুৱ DfgVertex* ধ্রুৱ vtxp : m_logicp->synth()) {
            ধ্রুৱ DfgVertexVar* ধ্রুৱ varp = vtxp->cast<DfgVertexVar>();
            যদি (!varp) অনুৱর্ত;
            // If the variable we synthesized this vertex for is volatile, and
            // the value of the synthesized temporary is observed, we might be
            // missing an external update, so we mut give up.
            যদি (!varp->hasSinks()) অনুৱর্ত;
            যদি (!DfgVertexVar::isVolatile(varp->tmpForp())) অনুৱর্ত;
            ++m_ctx.m_synt.nonSynExtWrite;
            ঘূৰাই মিছা;
        }
        ঘূৰাই সঁচা;
    }

    // Add the synthesized values as drivers to the output variables of the current DfgLogic
    তার্কিক addSynthesizedOutput(SymTab& oSymTab) {
        // It's possible we think a variable is written by the DfgLogic when
        // it actauly isn't, e.g.: '{a[0], b[0]}[1] = ...' does not write 'b'.
        // These LHS forms can happen after some earlier tranforms. We
        // should just run V3Const on them earlier, but we will do belt and
        // braces and check here too. We can't touch any output variables if so.
        ধ্রুৱ তার্কিক missing = m_logicp->foreachSink([&](ধ্রুৱ DfgVertex& sink) {
            ধ্রুৱ DfgUnresolved* ধ্রুৱ unresolvedp = sink.as<DfgUnresolved>();
            AstVarScope* ধ্রুৱ vscp = unresolvedp->singleSink()->as<DfgVertexVar>()->vscp();
            ঘূৰাই !oSymTab.গুনতি(vscp);
        });
        যদি (missing) {
            ++m_ctx.m_synt.nonSynFalseWrite;
            ঘূৰাই মিছা;
        }

        // Add sinks to read the computed values for the target variables
        ঘূৰাই !m_logicp->foreachSink([&](DfgVertex& sink) {
            DfgUnresolved* ধ্রুৱ unresolvedp = sink.as<DfgUnresolved>();
            ধ্রুৱ DfgVertexVar* ধ্রুৱ varp = unresolvedp->singleSink()->as<DfgVertexVar>();
            DfgVertexVar* ধ্রুৱ resp = oSymTab.at(varp->vscp());
            UASSERT_OBJ(resp->srcp(), resp, "Undriven result");

            // If the output is not used further in the synthesized logic itself,
            // then resp will be deleted before we return, so we can just use
            // its splice directly without ending up with a multi-use operation.
            যদি (!resp->hasSinks()) {
                unresolvedp->addDriver(resp->srcp()->as<DfgVertexSplice>());
                ঘূৰাই মিছা;  // OK, continue.
            }

            // TODO: computePropagatedDrivers cannot handle arrays, should
            // never happen with simple continous assignments
            যদি (!resp->isPacked()) {
                ++m_ctx.m_synt.nonSynArray;
                ঘূৰাই সঁচা;  // Not OK, give up
            }

            // We need to add a new splice to avoid multi-use of the original splice
            DfgSplicePacked* ধ্রুৱ splicep
                = নূতন DfgSplicePacked{m_dfg, resp->fileline(), resp->dtype()};
            // Drivers are the same
            ধ্রুৱ std::vector<Driver> drivers = computePropagatedDrivers({}, resp);
            বাবে (ধ্রুৱ Driver& d : drivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
            unresolvedp->addDriver(splicep);
            ঘূৰাই মিছা;  // OK, continue
        });
    }

    // Synthesize the given AstAssignW. Returns true on success.
    তার্কিক synthesizeAssignW(AstAssignW* nodep) {
        ++m_ctx.m_synt.প্ৰবেশ_Assign;

        // The input and output symbol tables
        SymTab iSymTab;
        SymTab oSymTab;

        // Initialzie input symbol table
        initialize_Entry_প্ৰতীক_Table(iSymTab);

        // Synthesize as if it was in a single CfgBlock CFG
        DfgVertex* condp = nullptr;
        ধ্রুৱ তার্কিক success = synthesizeBasicBlock(oSymTab, condp, {nodep}, iSymTab);
        UASSERT_OBJ(!condp, nodep, "Conditional AstAssignW ???");
        যদি (!success) ঘূৰাই মিছা;

        // Check exernal writes are observed correctly
        যদি (!checkExtWrites()) ঘূৰাই মিছা;

        // Add resolved output variable drivers
        ঘূৰাই addSynthesizedOutput(oSymTab);
    }

    // Synthesize the given AstAlways. Returns true on success.
    তার্কিক synthesizeCfg(CfgGraph& cfg) {
        ++m_ctx.m_synt.প্ৰবেশ_Always;

        // If there is a backward edge (loop), we can't synthesize it
        যদি (cfg.containsLoop()) {
            ++m_ctx.m_synt.nonSynLoop;
            ++m_ctx.m_synt.cfgCyclic;
            ঘূৰাই মিছা;
        }

        // If it's a trivial CFG we can save on some work
        যদি (cfg.nBlocks() == 1) {
            ++m_ctx.m_synt.cfgTrivial;
        } অন্যথা {
            // Insert two-way join blocks to aid multiplexer ordering
            যদি (cfg.ঢোকাও_Two_Way_Joins()) {
                ++m_ctx.m_synt.cfgSp;
            } অন্যথা {
                ++m_ctx.m_synt.cfgDag;
            }
            // Initialize maps needed for non-trivial CFGs
            m_dom_বৃক্ষ = Cfg_Dominator_বৃক্ষ{cfg};
            m_edgeToPredicatep = cfg.makeEdgeMap<DfgVertexVar*>();
        }

        // Initialize CfgMaps
        m_bbToISymTab = cfg.makeBlockMap<SymTab>();
        m_bbToOSymTab = cfg.makeBlockMap<SymTab>();
        m_bbToCondp = cfg.makeBlockMap<DfgVertexVar*>();

        // Synthesize all blocks
        বাবে (ধ্রুৱ V3GraphVertex& vtx : cfg.vertices()) {
            ধ্রুৱ CfgBlock& bb = static_cast<ধ্রুৱ CfgBlock&>(vtx);
            // Prepare the input symbol table of this block (enter, or join predecessor blocks)
            যদি (!create_প্ৰবেশ_প্ৰতীক_Table(bb)) ঘূৰাই মিছা;
            // Synthesize this block
            DfgVertex* condp = nullptr;
            যদি (!synthesizeBasicBlock(m_bbToOSymTab[bb], condp, bb.stmtps(), m_bbToISymTab[bb])) {
                ঘূৰাই মিছা;
            }
            // Create a temporary for the branch condition as it might be used multiple times
            যদি (condp) {
                File_সাৰি* ধ্রুৱ flp = condp->fileline();
                ধ্রুৱ DfgDataType& dtype = condp->dtype();
                ধ্রুৱ std::string prefix = "_BB" + std::to_string(bb.id()) + "_Cond";
                DfgVertexVar* ধ্রুৱ vp = m_converter.createTmp(*m_logicp, flp, dtype, prefix);
                vp->srcp(condp);
                m_bbToCondp[bb] = vp;
            }
            // Set the path predicates on the successor edges
            assignPathPredicates(bb);
        }

        // Check exernal writes are observed correctly
        যদি (!checkExtWrites()) ঘূৰাই মিছা;

        // Add resolved output variable drivers
        ঘূৰাই addSynthesizedOutput(m_bbToOSymTab[cfg.exit()]);
    }

    // Synthesize a DfgLogic into regular vertices. Returns ture on success.
    তার্কিক synthesize(DfgLogic& vtx) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;

        যদি (AstAssignW* ধ্রুৱ ap = VN_CAST(vtx.nodep()->stmtsp(), AssignW)) {
            যদি (ap->nextp()) ঘূৰাই মিছা;
            যদি (!synthesizeAssignW(ap)) ঘূৰাই মিছা;
            ++m_ctx.m_synt.synthAssign;
            ঘূৰাই সঁচা;
        }

        যদি (!synthesizeCfg(vtx.cfg())) ঘূৰাই মিছা;
        ++m_ctx.m_synt.synthAlways;
        ঘূৰাই সঁচা;
    }

    // Revert all DfgLogic in m_toRevert, or DfgLogic driving the DfgUnresolved
    // vertices in m_toRevert, and transitively the same for any DfgUnresolved
    // driven by the reverted DfgLogic. Delete all DfgUnresolved involed.
    শূন্য revert(VDouble0& statCountr) {
        m_toRevert.foreach([&](DfgVertex& vtx) {
            // Process DfgLogic
            যদি (DfgLogic* ধ্রুৱ vtxp = vtx.cast<DfgLogic>()) {
                UASSERT_OBJ(vtxp->selectedForSynthesis(), vtxp, "Shouldn't reach here unselected");
                // Revert this logic
                UASSERT_OBJ(!vtxp->reverted(), vtxp, "Should be reverting now");
                ++statCountr;
                বাবে (DfgVertex* ধ্রুৱ p : vtxp->synth()) VL_DO_DANGLING(p->unlinkDelete(m_dfg), p);
                vtxp->synth().clear();
                vtxp->setReverted();
                // Add all DfgUnresolved it drives to the work list
                vtxp->foreachSink([&](DfgVertex& snk) {
                    m_toRevert.ঠেলো_front(*snk.as<DfgUnresolved>());
                    ঘূৰাই মিছা;
                });
                ঘূৰাই;
            }

            // Process DfgUnresolved
            যদি (DfgUnresolved* ধ্রুৱ vtxp = vtx.cast<DfgUnresolved>()) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->setHasModWrRefs();
                // Add all driving DfgLogic to the work list
                vtxp->foreachSource([&](DfgVertex& src) {
                    DfgLogic* ধ্রুৱ lp = src.cast<DfgLogic>();
                    যদি (lp && !lp->reverted()) m_toRevert.ঠেলো_front(*lp);
                    ঘূৰাই মিছা;
                });
                // Delete the DfgUnresolved driver
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
                ঘূৰাই;
            }

            // There should be nothing else on the worklist
            vtx.v3fatalSrc("Unexpected vertex type");
        });
    }

    // Synthesize all of the given vertices
    শূন্য main() {
        //-------------------------------------------------------------------
        UINFO(5, "Step 0: Remove all DfgLogic not selected for synthesis");
        বাবে (DfgVertex* ধ্রুৱ vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgUnresolved
            যদি (!vtxp->is<DfgUnresolved>()) অনুৱর্ত;
            তার্কিক anySelected = মিছা;
            তার্কিক anyUnselected = মিছা;
            vtxp->foreachSource([&](DfgVertex& src) {
                ধ্রুৱ DfgLogic& logic = *src.as<DfgLogic>();
                যদি (logic.selectedForSynthesis()) {
                    anySelected = সঁচা;
                } অন্যথা {
                    anyUnselected = সঁচা;
                }
                ঘূৰাই মিছা;
            });
            // There should be a driver
            UASSERT_OBJ(anySelected || anyUnselected, vtxp, "'DfgUnresolved' with no driver");
            // All drivers should be selected or all should be unselected
            UASSERT_OBJ(!(anySelected && anyUnselected), vtxp, "Invalid 'DfgLogic' selection");
            // If all drivers are unselected, delete this DfgUnresolved here
            যদি (anyUnselected) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->setHasModWrRefs();
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
        বাবে (DfgVertex* ধ্রুৱ vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgLogic
            DfgLogic* ধ্রুৱ logicp = vtxp->cast<DfgLogic>();
            যদি (!logicp) অনুৱর্ত;
            যদি (logicp->selectedForSynthesis()) অনুৱর্ত;
            // There should be no sinks left for unselected DfgLogic, delete them here
            UASSERT_OBJ(!logicp->hasSinks(), vtxp, "Unselected 'DfgLogic' with sinks remaining");
            // Input variables will be read in Ast code, add Ast reference vertices
            // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
            ধ্রুৱ VNUser4InUse m_user4InUse;
            logicp->foreachSource([](DfgVertex& src) {
                src.as<DfgVertexVar>()->vscp()->user4p(&src);
                ঘূৰাই মিছা;
            });
            V3DfgPasses::addAstRefs(m_dfg, logicp->nodep(), [](Ast_গ্ৰন্থি* varp) {  //
                ঘূৰাই varp->user4u().to<DfgVertexVar*>();
            });
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        debugDump("synth-selected");

        //-------------------------------------------------------------------
        UINFO(5, "Step 1: Attempting to synthesize each of the selected DfgLogic");
        {
            // AstVarScope::user4() -> int: unique ID for 'AstVarScopeComparator'
            ধ্রুৱ VNUser4InUse user4InUse;
            বাবে (DfgVertex& vtx : m_dfg.opVertices()) {
                DfgLogic* ধ্রুৱ logicp = vtx.cast<DfgLogic>();
                যদি (!logicp) অনুৱর্ত;

                // We should only have DfgLogic remaining that was selected for synthesis
                UASSERT_OBJ(logicp->selectedForSynthesis(), logicp, "Unselected DfgLogic remains");

                // Debug aid
                ধ্রুৱ auto debugCallback = [&]() -> শূন্য {
                    // This is the breaking logic
                    m_debugLogicp = logicp;
                    // Dump it
                    UINFOTREE(0, logicp->nodep(), "Problematic DfgLogic: " << logicp, "  ");
                    V3EmitV::debug_Verilog_For_বৃক্ষ(logicp->nodep(), std::cout);
                    debugDump("synth-lastok");
                };
                যদি (VL_UNLIKELY(s_dfgSynthDebugBisect.stop(debugCallback))) ভঙ্গ;

                // Synthesize it, if failed, enqueue for reversion
                যদি (!synthesize(*logicp)) {
                    logicp->setNonSynthesizable();
                    m_toRevert.ঠেলো_front(*logicp);
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
        std::vector<DfgVertexVar*> multidrivenps;
        // Map from variable to its resolved driver
        std::unordered_map<ধ্রুৱ DfgVertexVar*, DfgVertexSplice*> resolvedDrivers;
        // Compute resolved drivers of all variablees
        বাবে (DfgVertexVar& var : m_dfg.varVertices()) {
            যদি (!var.srcp()) অনুৱর্ত;
            DfgUnresolved* ধ্রুৱ unresolvedp = var.srcp()->cast<DfgUnresolved>();
            যদি (!unresolvedp) ভঙ্গ;  // Stop when reached the synthesized temporaries

            // Resolve the synthesized drivers
            DfgVertexSplice* ধ্রুৱ resolvedp = [&]() -> DfgVertexSplice* {
                // All synthesized drivers were normalized already,
                // so if there is only one, it can be used directly
                যদি (unresolvedp->nInputs() == 1) {
                    ঘূৰাই unresolvedp->inputp(0)->as<DfgVertexSplice>();
                }
                // Otherwise gather the synthesized drivers
                std::vector<Driver> drivers = gatherDriversUnresolved(unresolvedp);
                // Normalize them, make resolved driver if all good
                যদি (normalizeDrivers(var, drivers)) ঘূৰাই makeSplice(var, drivers);
                // If mutlidriven, record and ignore
                multidrivenps.emplace_back(&var);
                m_toRevert.ঠেলো_front(*unresolvedp);
                ঘূৰাই nullptr;
            }();
            // Bail if multidriven
            যদি (!resolvedp) অনুৱর্ত;
            // Add to map for next loop
            ধ্রুৱ তার্কিক newEntry = resolvedDrivers.emplace(&var, resolvedp).second;
            UASSERT_OBJ(newEntry, &var, "Duplicate driver");
        }
        // Revert and remove drivers of multi-driven variables
        revert(m_ctx.m_synt.revertMultidrive);
        // Replace all DfgUnresolved with the resolved drivers
        বাবে (ধ্রুৱ DfgVertexVar& var : m_dfg.varVertices()) {
            যদি (!var.srcp()) অনুৱর্ত;
            DfgUnresolved* ধ্রুৱ srcp = var.srcp()->cast<DfgUnresolved>();
            যদি (!srcp) ভঙ্গ;  // Stop when reached the synthesized temporaries

            // Replace it
            srcp->replaceWith(resolvedDrivers.at(&var));
            VL_DO_DANGLING(srcp->unlinkDelete(m_dfg), srcp);
        }
        debugDump("synth-resolved");

        //-------------------------------------------------------------------
        UINFO(5, "Step 4: Remove all remaining DfgLogic and DfgUnresolved");
        বাবে (DfgVertex* ধ্রুৱ vtxp : m_dfg.opVertices().unlinkable()) {
            // Previous step should have removed all DfgUnresolved
            UASSERT_OBJ(!vtxp->is<DfgUnresolved>(), vtxp, "DfgUnresolved remains");

            // Process only DfgLogic
            DfgLogic* ধ্রুৱ logicp = vtxp->cast<DfgLogic>();
            যদি (!logicp) অনুৱর্ত;

            // Earlier pass should have removed all sinks
            UASSERT_OBJ(!logicp->hasSinks(), logicp, "DfgLogic sink remains");

            যদি (!logicp->synth().empty()) {
                // If synthesized, delete the corresponding AstNode. It is now in Dfg.
                logicp->nodep()->unlinkFrBack()->delete_বৃক্ষ();
            } অন্যথা {
                // Not synthesized. Logic stays in Ast. Add Ast reference vertices.
                // Outputs already marked by revertTransivelyAndRemove.
                // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
                ধ্রুৱ VNUser4InUse m_user4InUse;
                logicp->foreachSource([](DfgVertex& src) {
                    src.as<DfgVertexVar>()->vscp()->user4p(&src);
                    ঘূৰাই মিছা;
                });
                V3DfgPasses::addAstRefs(m_dfg, logicp->nodep(), [](Ast_গ্ৰন্থি* varp) {  //
                    ঘূৰাই varp->user4u().to<DfgVertexVar*>();
                });
            }

            // Delete this DfgLogic
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        // Reset the debug pointer, we have deleted it in the loop above ...
        m_debugLogicp = nullptr;
        debugDump("synth-rmlogics");

        //-------------------------------------------------------------------
        UINFO(5, "Step 5: Remove unnecessary splices");
        বাবে (DfgVertex* ধ্রুৱ vtxp : m_dfg.opVertices().unlinkable()) {
            DfgVertexSplice* ধ্রুৱ splicep = vtxp->cast<DfgVertexSplice>();
            যদি (!splicep) অনুৱর্ত;

            // Might not have a sink if the driving logic was revered, remove
            যদি (!splicep->hasSinks()) {
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
                অনুৱর্ত;
            }

            // It should alway have drivers
            UASSERT_OBJ(splicep->nInputs(), splicep, "Splice with no drivers");

            // If redundant, remove it
            যদি (DfgVertex* ধ্রুৱ wholep = splicep->wholep()) {
                যদি (DfgVertexVar* ধ্রুৱ varp = splicep->singleSink()->cast<DfgVertexVar>()) {
                    varp->driver_File_সাৰি(splicep->driver_File_সাৰি(0));
                }
                splicep->replaceWith(wholep);
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
            }
        }
        debugDump("synth-rmsplice");
    }

    // CONSTRUCTOR
    AstToDfgSynthesize(DfgGraph& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx}
        , m_converter{dfg, ctx} {
        main();
    }

সার্ৱজনিক:
    স্থির শূন্য apply(DfgGraph& dfg, V3DfgSynthesisContext& ctx) {
        AstToDfgSynthesize{dfg, ctx};

        // Final step outside, as both AstToDfgSynthesize and removeUnused used DfgUserMap
        UINFO(5, "Step 6: Remove all unused vertices");
        V3DfgPasses::removeUnused(dfg);
        যদি (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("synth-rmunused");

        // No operation vertex should have multiple sinks. Cyclic decomoposition
        // depends on this and it can easily be ensured by using temporaries.
        // Also, all sources should be connected at this point
        যদি (v3Global.opt.debugCheck()) {
            বাবে (DfgVertex& vtx : dfg.opVertices()) {
                UASSERT_OBJ(!vtx.hasMultipleSinks(), &vtx, "Operation has multiple sinks");
                বাবে (size_t i = 0; i < vtx.nInputs(); ++i) {
                    UASSERT_OBJ(vtx.inputp(i), &vtx, "Unconnected source operand");
                }
            }
            V3DfgPasses::typeCheck(dfg);
        }
    }
};

পূর্ণাঙ্ক AstToDfgSynthesize::AstVarScopeComparator::s_vscpIdCounter = 0;

// Decide which DfgLogic to attempt to synthesize
স্থির শূন্য dfgSelectLogicForSynthesis(DfgGraph& dfg) {
    // If we are told to synthesize everything, we will do so ...
    যদি (v3Global.opt.fDfgSynthesizeAll()) {
        বাবে (DfgVertex& vtx : dfg.opVertices()) {
            যদি (DfgLogic* ধ্রুৱ logicp = vtx.cast<DfgLogic>()) logicp->setSelectedForSynthesis();
        }
        ঘূৰাই;
    }

    // Otherwise figure out which vertices are likely worth synthesizing.

    std::vector<DfgVertexVar*> circularVarps;
    // Synthesize all drivers of circular variables
    {
        DfgUserMap<uint64_t> scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, scc);
        বাবে (DfgVertexVar& var : dfg.varVertices()) {
            যদি (!scc.at(var)) অনুৱর্ত;
            // This is a circular variable
            circularVarps.emplace_back(&var);
        }
    }

    // To cover all drivers, use a work list
    DfgWorklist worklist{dfg};

    // Synthesize all drivers of circular variables
    বাবে (ধ্রুৱ DfgVertexVar* ধ্রুৱ varp : circularVarps) {
        varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
            worklist.ঠেলো_front(*driver.as<DfgLogic>());
            ঘূৰাই মিছা;
        });
    }

    // Choose some simple special cases to always synthesize
    বাবে (DfgVertex& vtx : dfg.opVertices()) {
        DfgLogic* ধ্রুৱ logicp = vtx.cast<DfgLogic>();
        যদি (!logicp || worklist.contains(*logicp)) অনুৱর্ত;
        // If drives an unused variable, synthesize it so the partial logic can be removed
        যদি (logicp->drivesUnusedVars()) {
            worklist.ঠেলো_front(*logicp);
            অনুৱর্ত;
        }
        // Blocks corresponding to continuous assignments
        যদি (logicp->nodep()->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            worklist.ঠেলো_front(*logicp);
            অনুৱর্ত;
        }
        ধ্রুৱ CfgGraph& cfg = logicp->cfg();
        // Straight line code with no branches
        যদি (cfg.nBlocks() == 1) {
            worklist.ঠেলো_front(*logicp);
            অনুৱর্ত;
        }
        // Blocks driving exactly 1 variable
        যদি (!logicp->hasMultipleSinks()) worklist.ঠেলো_front(*logicp);
    }

    std::unordered_set<DfgUnresolved*> isVisited;

    // Now expand to cover all logic driving the same set of variables and mark
    worklist.foreach([&](DfgVertex& vtx) {
        DfgLogic& logic = *vtx.as<DfgLogic>();
        UASSERT_OBJ(!logic.selectedForSynthesis(), &vtx, "Should not be visited twice");
        // Mark as selected for synthesis
        logic.setSelectedForSynthesis();
        // Enqueue all other logic driving the same variables as this one
        logic.foreachSink([&](DfgVertex& sink) {
            DfgUnresolved* unresolved = sink.as<DfgUnresolved>();
            যদি (!isVisited.ঢোকাও(unresolved).second) ঘূৰাই মিছা;
            unresolved->foreachSource([&](DfgVertex& sibling) {
                DfgLogic& siblingLogic = *sibling.as<DfgLogic>();
                যদি (!siblingLogic.selectedForSynthesis()) worklist.ঠেলো_front(siblingLogic);
                ঘূৰাই মিছা;
            });
            ঘূৰাই মিছা;
        });
    });
}

শূন্য V3DfgPasses::synthesize(DfgGraph& dfg, V3DfgContext& ctx) {
    // Select which DfgLogic to attempt to synthesize
    dfgSelectLogicForSynthesis(dfg);
    // Synthesize them - also removes un-synthesized DfgLogic, so must run even if nothing selected
    AstToDfgSynthesize::apply(dfg, ctx.m_synthContext);
}
