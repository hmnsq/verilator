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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Cfg.h"
#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Dfg.h"
#അന്തര്ഭാവ "V3DfgPasses.h"
#അന്തര്ഭാവ "V3EmitV.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <iterator>

VL_DEFINE_DEBUG_FUNCTIONS;

നാമാകാശ {

// Create a DfgVertex out of a AstNodeExpr. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
template <typename T_Vertex, typename T_കണ്ണി>
T_Vertex* makeVertex(സ്ഥിരം T_കണ്ണി* nodep, DfgGraph& dfg, സ്ഥിരം DfgDataType& dtype) {
    തിരികെ പുതിയ T_Vertex{dfg, nodep->fileline(), dtype};
}

template <>
DfgArraySel* makeVertex<DfgArraySel, AstArraySel>(സ്ഥിരം AstArraySel* nodep, DfgGraph& dfg,
                                                  സ്ഥിരം DfgDataType& dtype) {
    // Some earlier passes create malformed ArraySels, just bail on those...
    // See t_bitsel_wire_array_bad
    എങ്കിൽ (VN_IS(nodep->fromp(), Const)) തിരികെ nullptr;
    എങ്കിൽ (!VN_IS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) തിരികെ nullptr;
    തിരികെ പുതിയ DfgArraySel{dfg, nodep->fileline(), dtype};
}

}  // namespace

// Visitor that can convert Ast statements and expressions in Dfg vertices
വർഗ്ഗം AstToDfgConverter final : പൊതു VNVisitor {
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

    തർക്കം m_foundUnhandled = അസത്യം;  // Found node not implemented as DFG or not implemented 'visit'
    തർക്കം m_converting = അസത്യം;  // We are trying to convert some logic at the moment

    // METHODS

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    template <typename Vertex, typename... Args>
    Vertex* make(Args&&... args) {
        static_assert(!std::is_base_of<DfgVertexVar, Vertex>::മൂല്യം, "Do not use for variables");
        static_assert(std::is_base_of<DfgVertex, Vertex>::മൂല്യം, "'Vertex' must be a 'DfgVertex'");
        Vertex* സ്ഥിരം vtxp = പുതിയ Vertex{m_dfg, std::forward<Args>(args)...};
        m_logicp->synth().emplace_back(vtxp);
        തിരികെ vtxp;
    }

    // Returns true if the expression cannot (or should not) be represented by DFG
    തർക്കം unhandled(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        // Short-circuiting if something was already unhandled
        എങ്കിൽ (m_foundUnhandled) {
            // Impure nodes cannot be represented
            എങ്കിൽ (!nodep->isPure()) {
                m_foundUnhandled = സത്യം;
                ++m_ctx.m_conv.nonRepImpure;
            }
        }
        തിരികെ m_foundUnhandled;
    }

    തർക്കം isSupported(സ്ഥിരം AstVarRef* nodep) {
        // Cannot represent cross module references
        എങ്കിൽ (nodep->classOrPackagep()) തിരികെ അസത്യം;
        // Check target
        തിരികെ V3Dfg::isSupported(nodep->varScopep());
    }

    // Given an RValue expression, return the equivalent Vertex, or nullptr if not representable.
    DfgVertex* convertRValue(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        UASSERT_OBJ(!m_converting, nodep, "'convertingRValue' should not be called recursively");
        VL_RESTORER(m_converting);
        VL_RESTORER(m_foundUnhandled);
        m_converting = സത്യം;
        m_foundUnhandled = അസത്യം;

        // Convert the expression
        iterate(nodep);

        // If falied to convert, return nullptr
        എങ്കിൽ (m_foundUnhandled) തിരികെ nullptr;

        // Traversal set user2p to the equivalent vertex
        DfgVertex* സ്ഥിരം vtxp = nodep->user2u().to<DfgVertex*>();
        UASSERT_OBJ(vtxp, nodep, "Missing Dfg vertex after conversion");
        തിരികെ vtxp;
    }

    // Given an LValue expression, return the splice node that writes the
    // destination, together with the index to use for splicing in the value.
    // Returns {nullptr, 0}, if the given LValue expression is not supported.
    std::pair<DfgVertexSplice*, uint32_t> convertLValue(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം vrefp = VN_CAST(nodep, VarRef)) {
            UASSERT_OBJ(vrefp->access().isWriteOnly(), vrefp, "Non-WriteOnly reference");
            എങ്കിൽ (!isSupported(vrefp)) {
                ++m_ctx.m_conv.nonRepLValue;
                തിരികെ {nullptr, 0};
            }

            // Get (or create a new) temporary for this variable
            സ്ഥിരം DfgVertexVar* സ്ഥിരം vtxp = [&]() -> DfgVertexVar* {
                // The variable being assigned
                AstVarScope* സ്ഥിരം vscp = vrefp->varScopep();

                // Find existing one, if any
                വേണ്ടി (സ്ഥിരം auto& pair : *m_updatesp) {
                    എങ്കിൽ (pair.first == vscp) തിരികെ pair.second;
                }

                // Create new one
                DfgVertexVar* സ്ഥിരം newp = createTmp(*m_logicp, vscp, "SynthAssign");
                m_updatesp->emplace_back(vscp, newp);

                // Create the Splice driver for the new temporary
                എങ്കിൽ (newp->is<DfgVarPacked>()) {
                    newp->srcp(make<DfgSplicePacked>(newp->fileline(), newp->dtype()));
                } അല്ലെങ്കിൽ എങ്കിൽ (newp->is<DfgVarArray>()) {
                    newp->srcp(make<DfgSpliceArray>(newp->fileline(), newp->dtype()));
                } അല്ലെങ്കിൽ {
                    nodep->v3fatalSrc("Unhandled DfgVertexVar sub-type");
                }

                // Use new temporary
                തിരികെ newp;
            }();

            // Return the Splice driver
            തിരികെ {vtxp->srcp()->as<DfgVertexSplice>(), 0};
        }

        എങ്കിൽ (സ്ഥിരം AstSel* selp = VN_CAST(nodep, Sel)) {
            // Only handle constant selects
            സ്ഥിരം AstConst* സ്ഥിരം lsbp = VN_CAST(selp->lsbp(), Const);
            എങ്കിൽ (!lsbp) {
                ++m_ctx.m_conv.nonRepLValue;
                തിരികെ {nullptr, 0};
            }
            uint32_t lsb = lsbp->toUInt();

            // Convert the 'fromp' sub-expression
            സ്ഥിരം auto pair = convertLValue(selp->fromp());
            എങ്കിൽ (!pair.first) തിരികെ {nullptr, 0};
            DfgSplicePacked* സ്ഥിരം splicep = pair.first->template as<DfgSplicePacked>();
            // Adjust index.
            lsb += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            എങ്കിൽ (lsb + static_cast<uint32_t>(selp->widthConst()) > splicep->size()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                തിരികെ {nullptr, 0};
            }

            // AstSel doesn't change type kind (array vs packed), so we can use
            // the existing splice driver with adjusted lsb
            തിരികെ {splicep, lsb};
        }

        എങ്കിൽ (സ്ഥിരം AstArraySel* സ്ഥിരം aselp = VN_CAST(nodep, ArraySel)) {
            // Only handle constant selects
            സ്ഥിരം AstConst* സ്ഥിരം indexp = VN_CAST(aselp->bitp(), Const);
            എങ്കിൽ (!indexp) {
                ++m_ctx.m_conv.nonRepLValue;
                തിരികെ {nullptr, 0};
            }
            uint32_t index = indexp->toUInt();

            // Convert the 'fromp' sub-expression
            സ്ഥിരം auto pair = convertLValue(aselp->fromp());
            എങ്കിൽ (!pair.first) തിരികെ {nullptr, 0};
            DfgSpliceArray* സ്ഥിരം splicep = pair.first->template as<DfgSpliceArray>();
            // Adjust index. Note pair.second is always 0, but we might handle array slices later..
            index += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            എങ്കിൽ (index + 1U > splicep->size()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                തിരികെ {nullptr, 0};
            }

            // Ensure the Splice driver exists for this element
            എങ്കിൽ (!splicep->driverAt(index)) {
                File_വരി* സ്ഥിരം flp = nodep->fileline();
                // This should never fail
                സ്ഥിരം DfgDataType& dtype = *DfgDataType::fromAst(nodep->dtypep());
                എങ്കിൽ (dtype.isPacked()) {
                    DfgSplicePacked* സ്ഥിരം newp = make<DfgSplicePacked>(flp, dtype);
                    സ്ഥിരം DfgDataType& uaDtype = DfgDataType::array(dtype, 1);
                    DfgUnitArray* സ്ഥിരം uap = make<DfgUnitArray>(flp, uaDtype);
                    uap->srcp(newp);
                    splicep->addDriver(uap, index, flp);
                } അല്ലെങ്കിൽ എങ്കിൽ (dtype.isArray()) {
                    DfgSpliceArray* സ്ഥിരം newp = make<DfgSpliceArray>(flp, dtype);
                    splicep->addDriver(newp, index, flp);
                } അല്ലെങ്കിൽ {
                    nodep->v3fatalSrc("Unhandled data type kind");
                }
            }

            // Return the splice driver
            DfgVertex* driverp = splicep->driverAt(index);
            എങ്കിൽ (സ്ഥിരം DfgUnitArray* സ്ഥിരം uap = driverp->cast<DfgUnitArray>()) {
                driverp = uap->srcp();
            }
            തിരികെ {driverp->as<DfgVertexSplice>(), 0};
        }

        ++m_ctx.m_conv.nonRepLValue;
        തിരികെ {nullptr, 0};
    }

    // Given the LHS of an assignment, and the vertex representing the RHS,
    // connect up the RHS to drive the targets.
    // Returns true on success, false if the LHS is not representable.
    തർക്കം convertAssignment(File_വരി* flp, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, DfgVertex* vtxp) {
        // Represents a DFG assignment contributed by the AST assignment with the above 'lhsp'.
        // There might be multiple of these if 'lhsp' is a concatenation.
        ഘടന Assignment final {
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
        എങ്കിൽ (!VN_IS(lhsp, VarRef)) lhsp = VN_AS(V3Const::constifyExpensiveEdit(lhsp), കണ്ണി_വ്യഞ്ജകം);

        // Assigning compound expressions to a concatenated LHS requires a temporary
        // to avoid multiple use of the expression
        എങ്കിൽ (VN_IS(lhsp, Concat) && !vtxp->is<DfgVertexVar>() && !vtxp->is<DfgConst>()) {
            DfgVertexVar* സ്ഥിരം tmpp = createTmp(*m_logicp, flp, vtxp->dtype(), "Unpack");
            tmpp->srcp(vtxp);
            vtxp = tmpp;
        }

        // Convert each concatenation LHS separately, gather all assignments
        // we need to do into 'assignments', return true if all LValues
        // converted successfully.
        std::vector<Assignment> assignments;
        സ്ഥിരം std::function<തർക്കം(Ast_കണ്ണി_വ്യഞ്ജകം*, uint32_t)> convertAllLValues
            = [&](Ast_കണ്ണി_വ്യഞ്ജകം* subp, uint32_t lsb) -> തർക്കം {
            // Concatenation on the LHS, convert each part
            എങ്കിൽ (AstConcat* സ്ഥിരം concatp = VN_CAST(subp, Concat)) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cRhsp = concatp->rhsp();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cLhsp = concatp->lhsp();
                // Convert Rigth of concat
                എങ്കിൽ (!convertAllLValues(cRhsp, lsb)) തിരികെ അസത്യം;
                // Convert Left of concat
                തിരികെ convertAllLValues(cLhsp, lsb + cRhsp->width());
            }

            // Non-concatenation, convert the LValue
            സ്ഥിരം auto pair = convertLValue(subp);
            എങ്കിൽ (!pair.first) തിരികെ അസത്യം;

            // If whole lhs, just use it
            എങ്കിൽ (subp == lhsp) {
                assignments.emplace_back(pair.first, pair.second, vtxp);
                തിരികെ സത്യം;
            }

            // Otherwise select the relevant bits
            സ്ഥിരം DfgDataType& dtype = *DfgDataType::fromAst(subp->dtypep());
            DfgSel* സ്ഥിരം selp = make<DfgSel>(subp->fileline(), dtype);
            selp->fromp(vtxp);
            selp->lsb(lsb);
            assignments.emplace_back(pair.first, pair.second, selp);
            തിരികെ സത്യം;
        };

        // Convert the given LHS assignment, give up if any LValues failed to convert
        എങ്കിൽ (!convertAllLValues(lhsp, 0)) തിരികെ അസത്യം;

        // All successful, connect the drivers
        വേണ്ടി (സ്ഥിരം Assignment& item : assignments) {
            എങ്കിൽ (DfgSplicePacked* സ്ഥിരം spp = item.m_lhsp->template cast<DfgSplicePacked>()) {
                spp->addDriver(item.m_rhsp, item.m_idx, flp);
            } അല്ലെങ്കിൽ എങ്കിൽ (DfgSpliceArray* സ്ഥിരം sap = item.m_lhsp->template cast<DfgSpliceArray>()) {
                // TODO: multi-dimensional arrays will need changes here
                സ്ഥിരം DfgDataType& rDt = item.m_rhsp->dtype();
                എങ്കിൽ (rDt.isPacked()) {
                    // RHS is assigning an element of this array. Need a DfgUnitArray adapter.
                    DfgUnitArray* സ്ഥിരം uap = make<DfgUnitArray>(flp, DfgDataType::array(rDt, 1));
                    uap->srcp(item.m_rhsp);
                    sap->addDriver(uap, item.m_idx, flp);
                } അല്ലെങ്കിൽ {
                    // RHS is assigning an array (or array slice). Should be the same element type.
                    sap->addDriver(item.m_rhsp, item.m_idx, flp);
                }
            } അല്ലെങ്കിൽ {
                item.m_lhsp->v3fatalSrc("Unhandled DfgVertexSplice sub-type");
            }
        }

        തിരികെ സത്യം;
    }

    // VISITORS

    // Unhandled node
    ശൂന്യം visit(Ast_കണ്ണി* /*nodep*/) override {
        എങ്കിൽ (!m_foundUnhandled && m_converting) ++m_ctx.m_conv.nonRepUnknown;
        m_foundUnhandled = സത്യം;
    }

    // Expressions - mostly auto generated, but a few special ones
    ശൂന്യം visit(AstVarRef* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        എങ്കിൽ (unhandled(nodep)) തിരികെ;
        // This visit method is only called on RValues, where only read refs are supported
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Non-ReadOnly reference");
        എങ്കിൽ (!isSupported(nodep)) {
            m_foundUnhandled = സത്യം;
            ++m_ctx.m_conv.nonRepVarRef;
            തിരികെ;
        }

        // Variable should have been bound before starting conversion
        DfgVertex* സ്ഥിരം vtxp = nodep->varScopep()->user2u().template to<DfgVertexVar*>();
        UASSERT_OBJ(vtxp, nodep, "Referenced variable has no associated DfgVertexVar");
        nodep->user2p(vtxp);
    }
    ശൂന്യം visit(AstConst* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        എങ്കിൽ (unhandled(nodep)) തിരികെ;

        എങ്കിൽ (nodep->width() != nodep->അക്കം().width()) {
            // Sometimes the width of the AstConst is not the same as the
            // V3Number it holds. Truncate it here. TODO: should this be allowed?
            V3_സംഖ്യ അക്കം{nodep, nodep->width()};
            അക്കം.opSel(nodep->അക്കം(), nodep->width() - 1, 0);
            DfgVertex* സ്ഥിരം vtxp = make<DfgConst>(nodep->fileline(), അക്കം);
            nodep->user2p(vtxp);
        } അല്ലെങ്കിൽ {
            DfgVertex* സ്ഥിരം vtxp = make<DfgConst>(nodep->fileline(), nodep->അക്കം());
            nodep->user2p(vtxp);
        }
    }
    ശൂന്യം visit(AstCReset* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        എങ്കിൽ (unhandled(nodep)) തിരികെ;

        സ്ഥിരം DfgDataType* സ്ഥിരം dtypep = DfgDataType::fromAst(nodep->dtypep());
        എങ്കിൽ (!dtypep) {
            m_foundUnhandled = സത്യം;
            ++m_ctx.m_conv.nonRepDType;
            തിരികെ;
        }

        UASSERT_OBJ(!nodep->constructing(), nodep,
                    "CReset should be non-constructing at this stage");

        DfgVertex* സ്ഥിരം vtxp = make<DfgCReset>(nodep->fileline(), *dtypep);
        nodep->user2p(vtxp);
    }
    ശൂന്യം visit(AstMatchMasked* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        എങ്കിൽ (unhandled(nodep)) തിരികെ;

        സ്ഥിരം DfgDataType* സ്ഥിരം dtypep = DfgDataType::fromAst(nodep->dtypep());
        എങ്കിൽ (!dtypep) {
            m_foundUnhandled = സത്യം;
            ++m_ctx.m_conv.nonRepDType;
            തിരികെ;
        }

        iterate(nodep->lhsp());
        എങ്കിൽ (m_foundUnhandled) തിരികെ;
        iterate(nodep->matchp());
        എങ്കിൽ (m_foundUnhandled) തിരികെ;

        File_വരി* സ്ഥിരം flp = nodep->fileline();
        DfgMatchMasked* സ്ഥിരം vtxp = make<DfgMatchMasked>(flp, *dtypep);
        vtxp->lhsp(nodep->lhsp()->user2u().to<DfgVertex*>());
        vtxp->matchp(nodep->matchp()->user2u().to<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    ശൂന്യം visit(AstReplicate* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        എങ്കിൽ (unhandled(nodep)) തിരികെ;

        സ്ഥിരം DfgDataType* സ്ഥിരം dtypep = DfgDataType::fromAst(nodep->dtypep());
        എങ്കിൽ (!dtypep) {
            m_foundUnhandled = സത്യം;
            ++m_ctx.m_conv.nonRepDType;
            തിരികെ;
        }

        iterate(nodep->srcp());
        എങ്കിൽ (m_foundUnhandled) തിരികെ;

        File_വരി* സ്ഥിരം flp = nodep->fileline();
        DfgRep* സ്ഥിരം vtxp = make<DfgRep>(flp, *dtypep);
        vtxp->srcp(nodep->srcp()->user2u().to<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    ശൂന്യം visit(AstSel* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        എങ്കിൽ (unhandled(nodep)) തിരികെ;

        സ്ഥിരം DfgDataType* സ്ഥിരം dtypep = DfgDataType::fromAst(nodep->dtypep());
        എങ്കിൽ (!dtypep) {
            m_foundUnhandled = സത്യം;
            ++m_ctx.m_conv.nonRepDType;
            തിരികെ;
        }

        iterate(nodep->fromp());
        എങ്കിൽ (m_foundUnhandled) തിരികെ;

        File_വരി* സ്ഥിരം flp = nodep->fileline();
        DfgVertex* vtxp = nullptr;
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->lsbp(), Const)) {
            സ്ഥിരം uint32_t lsb = constp->toUInt();
            സ്ഥിരം uint32_t msb = lsb + nodep->widthConst() - 1;
            DfgVertex* സ്ഥിരം fromp = nodep->fromp()->user2u().to<DfgVertex*>();
            // Unfortunately we can still have out of bounds selects due to how
            // indices are truncated for speed reasons in V3Width/V3Unknown.
            എങ്കിൽ (msb >= fromp->size()) {
                m_foundUnhandled = സത്യം;
                ++m_ctx.m_conv.nonRepOOBSel;
                തിരികെ;
            }
            DfgSel* സ്ഥിരം selp = make<DfgSel>(flp, *dtypep);
            selp->fromp(fromp);
            selp->lsb(lsb);
            vtxp = selp;
        } അല്ലെങ്കിൽ {
            iterate(nodep->lsbp());
            എങ്കിൽ (m_foundUnhandled) തിരികെ;
            DfgMux* സ്ഥിരം muxp = make<DfgMux>(flp, *dtypep);
            muxp->fromp(nodep->fromp()->user2u().to<DfgVertex*>());
            muxp->lsbp(nodep->lsbp()->user2u().to<DfgVertex*>());
            vtxp = muxp;
        }
        nodep->user2p(vtxp);
    }

// The rest of the visit methods for expressions are generated by 'astgen'
#അന്തര്ഭാവ "V3Dfg__gen_ast_to_dfg.h"

പൊതു:
    // PUBLIC METHODS

    // Create temporay variable capable of holding the given type
    DfgVertexVar* createTmp(DfgLogic& logic, File_വരി* flp, സ്ഥിരം DfgDataType& dtype,
                            സ്ഥിരം std::string& prefix) {
        DfgVertexVar* സ്ഥിരം vtxp = m_dfg.makeNewVar(flp, prefix, dtype, logic.scopep());
        logic.synth().emplace_back(vtxp);
        vtxp->vscp()->varp()->isInternal(സത്യം);
        vtxp->tmpForp(vtxp->vscp());
        തിരികെ vtxp;
    }

    // Create a new temporary variable capable of holding 'varp'
    DfgVertexVar* createTmp(DfgLogic& logic, AstVarScope* vscp, സ്ഥിരം std::string& prefix) {
        AstVar* സ്ഥിരം astVarp = vscp->varp();
        File_വരി* സ്ഥിരം flp = astVarp->fileline();
        സ്ഥിരം DfgDataType& dtype = *DfgDataType::fromAst(astVarp->dtypep());
        സ്ഥിരം std::string prfx = prefix + "_" + astVarp->name();
        DfgVertexVar* സ്ഥിരം vtxp = createTmp(logic, flp, dtype, prfx);
        vtxp->tmpForp(vscp);
        തിരികെ vtxp;
    }

    // Convert AstAssign to Dfg, return true if successful.
    // Fills 'updates' with bindings for assigned variables.
    തർക്കം convert(std::vector<std::pair<AstVarScope*, DfgVertexVar*>>& updates, DfgLogic& vtx,
                 Ast_കണ്ണി_Assign* nodep) {
        UASSERT_OBJ(VN_IS(nodep, Assign) || VN_IS(nodep, AssignW), nodep, "Bad NodeAssign");
        UASSERT_OBJ(updates.empty(), nodep, "'updates' should be empty");
        VL_RESTORER(m_updatesp);
        VL_RESTORER(m_logicp);
        m_updatesp = &updates;
        m_logicp = &vtx;
        // Assignment with timing control shouldn't make it this far
        UASSERT_OBJ(!nodep->timingControlp(), nodep, "Shouldn't make it this far");
        // Convert it
        ++m_ctx.m_conv.നിവേശം_Assignments;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp();
        // Check data types are compatible.
        സ്ഥിരം DfgDataType* സ്ഥിരം lDtypep = DfgDataType::fromAst(lhsp->dtypep());
        സ്ഥിരം DfgDataType* സ്ഥിരം rDtypep = DfgDataType::fromAst(rhsp->dtypep());
        എങ്കിൽ (!lDtypep || !rDtypep) {
            ++m_ctx.m_conv.nonRepDType;
            തിരികെ അസത്യം;
        }
        // For now, only direct array assignment is supported (e.g. a = b, but not a = _ ? b : c)
        എങ്കിൽ (rDtypep->isArray()) {
            എങ്കിൽ (!VN_IS(rhsp, VarRef)
                || !lhsp->dtypep()->skipRefp()->same_വൃക്ഷം(rhsp->dtypep()->skipRefp())) {
                ++m_ctx.m_conv.nonRepDType;
                തിരികെ അസത്യം;
            }
        }
        // Widths should match at this point
        UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Mismatched width reached DFG");
        // Convert the RHS expression
        DfgVertex* സ്ഥിരം rVtxp = convertRValue(rhsp);
        എങ്കിൽ (!rVtxp) തിരികെ അസത്യം;
        // Connect the RHS vertex to the LHS targets
        സ്ഥിരം തർക്കം success = convertAssignment(nodep->fileline(), lhsp, rVtxp);
        എങ്കിൽ (success) ++m_ctx.m_conv.representable;
        തിരികെ success;
    }

    // Convert RValue expression to Dfg. Returns nullptr if failed.
    DfgVertex* convert(DfgLogic& vtx, Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;
        // Convert it
        ++m_ctx.m_conv.നിവേശം_Expressions;
        DfgVertex* സ്ഥിരം vtxp = convertRValue(nodep);
        എങ്കിൽ (vtxp) ++m_ctx.m_conv.representable;
        തിരികെ vtxp;
    }

    // CONSTRUCTOR
    AstToDfgConverter(DfgGraph& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {}
};

// Debug aid - outisde 'AstToDfgSynthesize' as it is a template, but want one instance
V3DebugBisect s_dfgSynthDebugBisect{"DfgSynthesize"};

വർഗ്ഗം AstToDfgSynthesize final {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // TYPES

    // SymTab must be ordered in order to yield stable results
    ഘടന AstVarScopeComparator final {
        സ്ഥിതം പൂർണ്ണസംഖ്യ s_vscpIdCounter;  // Counter for lazily allocating the unique AstVarScope IDs
        തർക്കം operator()(AstVarScope* lhs, AstVarScope* rhs) സ്ഥിരം {
            എങ്കിൽ (!lhs->user4()) lhs->user4(++s_vscpIdCounter);
            എങ്കിൽ (!rhs->user4()) rhs->user4(++s_vscpIdCounter);
            തിരികെ lhs->user4() < rhs->user4();
        }
    };
    using SymTab = std::map<AstVarScope*, DfgVertexVar*, AstVarScopeComparator>;

    // Represents a [potentially partial] driver of a variable
    ഘടന Driver final {
        DfgVertex* m_vtxp = nullptr;  // Driving vertex
        uint32_t m_lo = 0;  // Low index of driven range (internal, not Verilog)
        uint32_t m_hi = 0;  // High index of driven range (internal, not Verilog)
        File_വരി* m_flp = nullptr;  // Location of driver in source

        Driver() = സ്ഥിരസ്ഥിതി;
        Driver(DfgVertex* vtxp, uint32_t lo, File_വരി* flp)
            : m_vtxp{vtxp}
            , m_lo{lo}
            , m_hi{lo + vtxp->size() - 1U}
            , m_flp{flp} {}
        operator തർക്കം() സ്ഥിരം { തിരികെ m_vtxp != nullptr; }

        തർക്കം operator<(സ്ഥിരം Driver& other) സ്ഥിരം {
            എങ്കിൽ (m_lo != other.m_lo) തിരികെ m_lo < other.m_lo;
            എങ്കിൽ (m_hi != other.m_hi) തിരികെ m_hi < other.m_hi;
            തിരികെ m_flp->operatorCompare(*other.m_flp) < 0;
        }

        തർക്കം operator<=(സ്ഥിരം Driver& other) സ്ഥിരം { തിരികെ !(other < *ഇത്); }
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
    Cfg_Dominator_വൃക്ഷം m_dom_വൃക്ഷം;  // The dominator tree of the current CFG

    // STATE - Some debug aid
    // We stop after synthesizing s_dfgSynthDebugLimit vertices (if non-zero).
    // This is the problematic logic (last one we synthesize), assuming a
    // bisection search over s_dfgSynthDebugLimit.
    DfgLogic* m_debugLogicp = nullptr;
    // Source (upstream) cone of outputs of m_debugLogicp
    std::unique_ptr<std::unordered_set<സ്ഥിരം DfgVertex*>> m_debugOSrcConep{nullptr};

    // METHODS

    // Dump current graph for debugging ...
    ശൂന്യം debugDump(സ്ഥിരം അക്ഷരം* name) {
        // If we have the debugged logic, compute the vertices feeding its outputs
        എങ്കിൽ (VL_UNLIKELY(m_debugLogicp)) {
            std::vector<സ്ഥിരം DfgVertex*> outputs;
            m_debugLogicp->foreachSink([&outputs](സ്ഥിരം DfgVertex& v) {
                outputs.emplace_back(v.singleSink()->as<DfgVertexVar>());
                തിരികെ അസത്യം;
            });
            m_debugOSrcConep = m_dfg.sourceCone(outputs);
        }

        എങ്കിൽ (VL_UNLIKELY(dumpDfgLevel() >= 9 || m_debugOSrcConep)) {
            m_dfg.dumpDotFilePrefixed(name);
            എങ്കിൽ (m_debugOSrcConep) {
                // Dump only the subgraph involving the inputs and outputs of the bad vertex
                m_dfg.dumpDotFilePrefixed(name + "-min"s, [&](സ്ഥിരം DfgVertex& v) -> തർക്കം {
                    തിരികെ m_debugOSrcConep->എണ്ണം(&v);
                });
            }
        }
    }

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    template <typename Vertex, typename... Args>
    Vertex* make(Args&&... args) {
        static_assert(!std::is_base_of<DfgVertexVar, Vertex>::മൂല്യം, "Do not use for variables");
        static_assert(std::is_base_of<DfgVertex, Vertex>::മൂല്യം, "'Vertex' must be a 'DfgVertex'");
        Vertex* സ്ഥിരം vtxp = പുതിയ Vertex{m_dfg, std::forward<Args>(args)...};
        എങ്കിൽ (m_logicp) m_logicp->synth().emplace_back(vtxp);
        തിരികെ vtxp;
    }

    // Gather all drivers of a resolved variable
    സ്ഥിതം std::vector<Driver> gatherDrivers(DfgVertexSplice* vtxp) {
        // Collect them all, check if they are sorted
        std::vector<Driver> drivers;
        drivers.reserve(vtxp->nInputs());
        തർക്കം sorted = സത്യം;
        vtxp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_വരി* flp) {
            // Collect the driver
            drivers.emplace_back(&src, lo, flp);
            // Check if drivers are sorted - most often they are
            സ്ഥിരം size_t n = drivers.size();
            എങ്കിൽ (n >= 2 && drivers[n - 1] < drivers[n - 2]) sorted = അസത്യം;
            തിരികെ അസത്യം;
        });

        // Sort if unsorted
        എങ്കിൽ (!sorted) std::stable_sort(drivers.begin(), drivers.end());

        // Done
        തിരികെ drivers;
    }

    // Returns true if the driver supplies a complete array element.
    സ്ഥിതം തർക്കം driverCoversWholeElement(സ്ഥിരം Driver& driver) {
        സ്ഥിരം DfgUnitArray* സ്ഥിരം unitp = driver.m_vtxp->cast<DfgUnitArray>();
        എങ്കിൽ (!unitp) തിരികെ അസത്യം;
        എങ്കിൽ (DfgVertexSplice* സ്ഥിരം splicep = unitp->srcp()->cast<DfgVertexSplice>()) {
            തിരികെ splicep->wholep();
        }
        തിരികെ സത്യം;
    }

    // Unlike 'wholep', several element drivers can collectively cover an array.
    സ്ഥിതം തർക്കം spliceCoversWhole(DfgVertexSplice* സ്ഥിരം splicep) {
        എങ്കിൽ (splicep->wholep()) തിരികെ സത്യം;
        എങ്കിൽ (splicep->isPacked()) തിരികെ അസത്യം;
        uint32_t next = 0;
        വേണ്ടി (സ്ഥിരം Driver& driver : gatherDrivers(splicep)) {
            എങ്കിൽ (driver.m_lo != next || !driverCoversWholeElement(driver)) തിരികെ അസത്യം;
            next = driver.m_hi + 1;
        }
        തിരികെ next == splicep->size();
    }

    // Returns true if the driver cone contains any variable introduced by
    // tristate lowering. Used to distinguish intentional tristate contributor
    // overlap from accidental multidrive.
    സ്ഥിതം തർക്കം containsTriLoweredVar(DfgVertex* rootp) {
        std::vector<സ്ഥിരം DfgVertex*> അട്ടി;
        std::vector<സ്ഥിരം DfgVertex*> visited;
        അട്ടി.emplace_back(rootp);
        ആകുമ്പോൾ (!അട്ടി.empty()) {
            സ്ഥിരം DfgVertex* സ്ഥിരം vtxp = അട്ടി.back();
            അട്ടി.എടുക്കുക_back();
            എങ്കിൽ (std::find(visited.begin(), visited.end(), vtxp) != visited.end()) തുടരുക;
            visited.emplace_back(vtxp);
            എങ്കിൽ (സ്ഥിരം DfgVertexVar* സ്ഥിരം varp = vtxp->cast<DfgVertexVar>()) {
                എങ്കിൽ (varp->vscp()->varp()->dfgTriLowered()) തിരികെ സത്യം;
            }
            vtxp->foreachSource([&](സ്ഥിരം DfgVertex& src) {
                അട്ടി.emplace_back(&src);
                തിരികെ അസത്യം;
            });
        }
        തിരികെ അസത്യം;
    }

    // Returns true if the driver is a direct variable forward (no logic).
    സ്ഥിതം തർക്കം isDirectVarDriver(സ്ഥിരം DfgVertex* vtxp) { തിരികെ vtxp->is<DfgVertexVar>(); }

    // Gather all synthesized drivers of an unresolved variable
    സ്ഥിതം std::vector<Driver> gatherDriversUnresolved(DfgUnresolved* vtxp) {
        std::vector<Driver> drivers;
        drivers.reserve(vtxp->nInputs());

        // For better locations in error reporting, we unpick concatenations
        // which are sometimes introduced by combinint assignments in V3Const.
        സ്ഥിരം std::function<ശൂന്യം(DfgVertex*, uint32_t, File_വരി*)> gather
            = [&](DfgVertex* vtxp, uint32_t lo, File_വരി* flp) -> ശൂന്യം {
            എങ്കിൽ (DfgConcat* സ്ഥിരം concatp = vtxp->cast<DfgConcat>()) {
                DfgVertex* സ്ഥിരം rhsp = concatp->rhsp();
                gather(rhsp, lo, rhsp->fileline());
                DfgVertex* സ്ഥിരം lhsp = concatp->lhsp();
                gather(lhsp, lo + rhsp->width(), lhsp->fileline());
                തിരികെ;
            }
            drivers.emplace_back(vtxp, lo, flp);
        };

        // Gather all synthesized drivers
        vtxp->foreachSource([&](DfgVertex& src) {
            // Can ignore the original DfgLogic
            എങ്കിൽ (src.is<DfgLogic>()) തിരികെ അസത്യം;
            // Synthesized drivers must be a splice at this point
            DfgVertexSplice* സ്ഥിരം splicep = src.as<DfgVertexSplice>();
            // Collect the driver
            splicep->foreachDriver([&](DfgVertex& src, uint32_t lo, File_വരി* flp) {
                gather(&src, lo, flp);
                തിരികെ അസത്യം;
            });
            തിരികെ അസത്യം;
        });

        // Sort the drivers
        std::stable_sort(drivers.begin(), drivers.end());

        // Done
        തിരികെ drivers;
    }

    // Given two drivers, combine the driven sub-ranges into the first one if
    // possible. First bool returned indicates successfully combined and there
    // are no multi-driven bits. Second bool returned indicates we warned
    // already about multi-driven bits.
    std::pair<തർക്കം, തർക്കം> combineDrivers(DfgVertexVar& var, സ്ഥിരം std::string& sub,  //
                                         Driver& a, സ്ഥിരം Driver& b) {
        // We can only combine array drivers ...
        എങ്കിൽ (a.m_vtxp->isPacked()) തിരികെ {അസത്യം, അസത്യം};
        // ... that both drive a single element ...
        എങ്കിൽ (a.m_lo != b.m_lo) തിരികെ {അസത്യം, അസത്യം};
        സ്ഥിരം DfgUnitArray* സ്ഥിരം aUap = a.m_vtxp->template cast<DfgUnitArray>();
        എങ്കിൽ (!aUap) തിരികെ {അസത്യം, അസത്യം};
        സ്ഥിരം DfgUnitArray* സ്ഥിരം bUap = b.m_vtxp->template cast<DfgUnitArray>();
        എങ്കിൽ (!bUap) തിരികെ {അസത്യം, അസത്യം};
        // ... and are themeselves partial
        DfgSplicePacked* സ്ഥിരം aSp = aUap->srcp()->template cast<DfgSplicePacked>();
        എങ്കിൽ (!aSp) തിരികെ {അസത്യം, അസത്യം};
        DfgSplicePacked* സ്ഥിരം bSp = bUap->srcp()->template cast<DfgSplicePacked>();
        എങ്കിൽ (!bSp) തിരികെ {അസത്യം, അസത്യം};
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
        എങ്കിൽ (!normalizeDrivers(var, abDrivers, sub + "[" + std::to_string(a.m_lo) + "]")) {
            തിരികെ {അസത്യം, സത്യം};
        }

        // Successfully resolved. Needs a new splice and unit.
        File_വരി* സ്ഥിരം flp = var.fileline();
        DfgSplicePacked* സ്ഥിരം splicep = make<DfgSplicePacked>(flp, aSp->dtype());
        വേണ്ടി (സ്ഥിരം Driver& d : abDrivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        DfgUnitArray* സ്ഥിരം uap = make<DfgUnitArray>(flp, aUap->dtype());
        uap->srcp(splicep);
        a.m_vtxp = uap;
        തിരികെ {സത്യം, അസത്യം};
    }

    // Combine and coalesce the given drivers.
    // Returns true iff no multi-driven bits are present.
    തർക്കം normalizeDrivers(DfgVertexVar& var, std::vector<Driver>& drivers,
                          സ്ഥിരം std::string& sub = "") {
        എങ്കിൽ (drivers.empty()) തിരികെ സത്യം;

        // What type of values are we combining
        സ്ഥിരം തർക്കം isPacked = drivers[0].m_vtxp->isPacked();

        // Found a multidriven part ?
        തർക്കം multiDriven = അസത്യം;

        // Iterate through the sorted drivers. Index 'i' is the driver we are
        // resolving driver 'j' agains, and if required, we merge 'j' into 'i'.
        size_t i = 0;
        വേണ്ടി (size_t j = 1; j < drivers.size();) {
            UASSERT_OBJ(i < j, &var, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            എങ്കിൽ (!jD) {
                ++j;
                തുടരുക;
            }
            // If 'i' was moved, move 'j' in it's place
            എങ്കിൽ (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                തുടരുക;
            }

            // We have 2 valid drivers now
            UASSERT_OBJ(iD <= jD, &var, "Should always be sorted");
            UASSERT_OBJ(jD.m_vtxp->isPacked() == isPacked, &var, "Mixed type drivers");

            // If no overlap, consider next pair
            എങ്കിൽ (iD.m_hi < jD.m_lo) {
                ++i;
                എങ്കിൽ (i == j) ++j;
                തുടരുക;
            }

            // There is an overlap. Attempt to combine them.
            തർക്കം combined = അസത്യം;
            തർക്കം warned = അസത്യം;
            std::tie(combined, warned) = combineDrivers(var, sub, iD, jD);

            // If sucessfully combined, 'j' is no longer needed, it was combined into 'i'
            എങ്കിൽ (combined) {
                jD = Driver{};
                ++j;
                തുടരുക;
            }

            // Found overlap that cannot be resolved
            multiDriven = സത്യം;
            // Compare next driver
            ++j;

            // Do not warn again if we warned during 'combineDrivers'
            എങ്കിൽ (warned) തുടരുക;

            // The variable to warn on
            AstVarScope* സ്ഥിരം vscp = var.tmpForp() ? var.tmpForp() : var.vscp();

            // Loop index often abused, so suppress
            എങ്കിൽ (vscp->varp()->isUsedLoopIdx()) തുടരുക;
            // Tristate lowering can intentionally create overlapping contributors.
            // Keep the signal marked multidriven for DFG fallback, but suppress
            // warning only when both overlapping driver cones look tri-lowered.
            എങ്കിൽ (vscp->varp()->dfgAllowMultidriveTri()) {
                സ്ഥിരം തർക്കം iTri = containsTriLoweredVar(iD.m_vtxp);
                സ്ഥിരം തർക്കം jTri = containsTriLoweredVar(jD.m_vtxp);
                സ്ഥിരം തർക്കം triPair = iTri && jTri;
                സ്ഥിരം തർക്കം triAndBridge = (iTri && isDirectVarDriver(jD.m_vtxp))
                                          || (jTri && isDirectVarDriver(iD.m_vtxp));
                എങ്കിൽ (triPair || triAndBridge) തുടരുക;
            }

            // Warn the user now
            സ്ഥിരം std::string lo = std::to_string(jD.m_lo);
            സ്ഥിരം std::string hi = std::to_string(std::min(iD.m_hi, jD.m_hi));
            സ്ഥിരം std::string kind = isPacked ? "Bit" : "Element";
            സ്ഥിരം std::string part = hi == lo ? (" [" + lo + "]") : ("s [" + hi + ":" + lo + "]");

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
        എങ്കിൽ (!multiDriven && isPacked) coalesceDrivers(drivers);

        തിരികെ !multiDriven;
    }

    // Coalesce adjacent drivers into single ones
    ശൂന്യം coalesceDrivers(std::vector<Driver>& drivers) {
        UASSERT(!drivers.empty(), "Can't coalesce 0 drivers");
        UASSERT_OBJ(drivers[0].m_vtxp->isPacked(), drivers[0].m_vtxp, "Can only coalesce packed");

        size_t i = 0;
        വേണ്ടി (size_t j = 1; j < drivers.size();) {
            UASSERT(i < j, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            എങ്കിൽ (!jD) {
                ++j;
                തുടരുക;
            }
            // If 'i' was moved, move 'j' in it's place
            എങ്കിൽ (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                തുടരുക;
            }

            // We have 2 valid drivers now
            UASSERT(iD <= jD, "Should always be sorted");

            // If not adjacent, move on
            എങ്കിൽ (iD.m_hi + 1 != jD.m_lo) {
                ++i;
                എങ്കിൽ (i == j) ++j;
                തുടരുക;
            }

            // Coalesce Adjacent ranges,
            സ്ഥിരം DfgDataType& dt = DfgDataType::packed(iD.m_vtxp->width() + jD.m_vtxp->width());
            DfgConcat* സ്ഥിരം concatp = make<DfgConcat>(iD.m_flp, dt);
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
    DfgVertexSplice* makeSplice(DfgVertexVar& var, സ്ഥിരം std::vector<Driver>& newDrivers) {
        UASSERT_OBJ(!newDrivers.empty(), &var, "'makeSplice' called with no new drivers");
        // Create new driver
        DfgVertexSplice* splicep = nullptr;
        എങ്കിൽ (var.is<DfgVarPacked>()) {
            splicep = make<DfgSplicePacked>(var.fileline(), var.dtype());
        } അല്ലെങ്കിൽ എങ്കിൽ (var.is<DfgVarArray>()) {
            splicep = make<DfgSpliceArray>(var.fileline(), var.dtype());
        } അല്ലെങ്കിൽ {
            var.v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        വേണ്ടി (സ്ഥിരം Driver& d : newDrivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        തിരികെ splicep;
    }

    // Initialzie input symbol table of entry CfgBlock
    ശൂന്യം initialize_Entry_ചിഹ്നം_Table(SymTab& iSymTab) {
        m_logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* സ്ഥിരം vvp = src.as<DfgVertexVar>();
            iSymTab[vvp->vscp()] = vvp;
            തിരികെ അസത്യം;
        });
    }

    // Join variable drivers across a control flow confluence (insert muxes ...)
    DfgVertexVar* joinDrivers(AstVarScope* vscp, DfgVertexVar* predicatep,  //
                              DfgVertexVar* thenp, DfgVertexVar* elsep) {
        AstVarScope* സ്ഥിരം thenVscp = thenp->tmpForp() ? thenp->tmpForp() : thenp->vscp();
        AstVarScope* സ്ഥിരം elseVscp = elsep->tmpForp() ? elsep->tmpForp() : elsep->vscp();
        UASSERT_OBJ(thenVscp == elseVscp, vscp, "Attempting to join unrelated variables");

        // If both bindings are the the same (variable not updated through either path),
        // then there is nothing to do, can use the same binding
        എങ്കിൽ (thenp == elsep) തിരികെ thenp;

        // We can't join the input variable just yet, so bail
        എങ്കിൽ (thenp->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_നിവേശം;
            തിരികെ nullptr;
        }
        എങ്കിൽ (elsep->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_നിവേശം;
            തിരികെ nullptr;
        }

        // Can't do arrays yet
        എങ്കിൽ (!thenp->isPacked()) {
            ++m_ctx.m_synt.nonSynArray;
            തിരികെ nullptr;
        }

        // Gather drivers of 'thenp' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* സ്ഥിരം tDefaultp = thenp->defaultp();
        std::vector<Driver> tDrivers = gatherDrivers(thenp->srcp()->as<DfgVertexSplice>());

        // Gather drivers of 'elsep' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* സ്ഥിരം eDefaultp = elsep->defaultp();
        std::vector<Driver> eDrivers = gatherDrivers(elsep->srcp()->as<DfgVertexSplice>());

        // Default drivers should be the same or not present on either
        UASSERT_OBJ(tDefaultp == eDefaultp, vscp, "Different default drivers");

        // Location to use for the join vertices
        File_വരി* സ്ഥിരം flp = predicatep->fileline();

        // Create a fresh temporary for the joined value
        DfgVertexVar* സ്ഥിരം joinp = m_converter.createTmp(*m_logicp, vscp, "SynthJoin");
        DfgVertexSplice* സ്ഥിരം joinSplicep = make<DfgSplicePacked>(flp, joinp->dtype());
        joinp->srcp(joinSplicep);

        // If both paths are fully driven, just create a simple conditional
        എങ്കിൽ (tDrivers.size() == 1  //
            && tDrivers[0].m_lo == 0  //
            && tDrivers[0].m_hi == thenp->width() - 1  //
            && eDrivers.size() == 1  //
            && eDrivers[0].m_lo == 0  //
            && eDrivers[0].m_hi == elsep->width() - 1) {
            UASSERT_OBJ(!tDefaultp, vscp, "Fully driven variable have default driver");

            DfgCond* സ്ഥിരം condp = make<DfgCond>(flp, joinp->dtype());
            condp->condp(predicatep);
            condp->thenp(thenp);
            condp->elsep(elsep);
            joinSplicep->addDriver(condp, 0, tDrivers[0].m_flp);

            // Done
            തിരികെ joinp;
        }

        // Otherwise we need to merge them part by part

        // If different bits are driven, then some might not have been assigned.. Latch?
        എങ്കിൽ (tDrivers.size() != eDrivers.size()) {
            ++m_ctx.m_synt.nonSynLatch;
            തിരികെ nullptr;
        }

        വേണ്ടി (size_t i = 0; i < tDrivers.size(); ++i) {
            സ്ഥിരം Driver& tDriver = tDrivers[i];
            സ്ഥിരം Driver& eDriver = eDrivers[i];
            // If different bits are driven, then some might not have been assigned.. Latch?
            എങ്കിൽ (tDriver.m_lo != eDriver.m_lo || tDriver.m_hi != eDriver.m_hi) {
                ++m_ctx.m_synt.nonSynLatch;
                തിരികെ nullptr;
            }

            സ്ഥിരം DfgDataType& dtype = DfgDataType::packed(tDriver.m_hi - tDriver.m_lo + 1);
            DfgCond* സ്ഥിരം condp = make<DfgCond>(flp, dtype);
            condp->condp(predicatep);

            // We actally need to select the bits from the joined variables, not use the drivers
            DfgSel* സ്ഥിരം thenSelp = make<DfgSel>(flp, tDriver.m_vtxp->dtype());
            thenSelp->lsb(tDriver.m_lo);
            thenSelp->fromp(thenp);
            condp->thenp(thenSelp);

            // Same for the 'else' part
            DfgSel* സ്ഥിരം elseSelp = make<DfgSel>(flp, eDriver.m_vtxp->dtype());
            elseSelp->lsb(eDriver.m_lo);
            elseSelp->fromp(elsep);
            condp->elsep(elseSelp);

            // Add it as a driver to the join
            joinSplicep->addDriver(condp, tDriver.m_lo, tDriver.m_flp);
        }

        // If there was a default driver, add it to te join
        എങ്കിൽ (tDefaultp) joinp->defaultp(tDefaultp);

        // Done
        തിരികെ joinp;
    }

    // Merge 'thenSymTab' into 'elseSymTab' using the given predicate to join values
    തർക്കം join_ചിഹ്നം_Tables(SymTab& elseSymTab, DfgVertexVar* predicatep, സ്ഥിരം SymTab& thenSymTab) {
        // Any variable that does not have a binding on both paths will be removed. These might be
        // temporaries, loop vars, etc used only in one branch. Conversion will fail if the
        // variable is actually referenced later.
        std::vector<AstVarScope*> toRemove;

        // Join each symbol
        വേണ്ടി (std::pair<AstVarScope* സ്ഥിരം, DfgVertexVar*>& pair : elseSymTab) {
            AstVarScope* സ്ഥിരം varp = pair.first;
            // Find same variable on the else path
            സ്ഥിരം auto it = thenSymTab.find(varp);
            // Record for removal if not assigned on both paths
            എങ്കിൽ (it == thenSymTab.end()) {
                toRemove.emplace_back(varp);
                തുടരുക;
            }
            // Join paths with the predicate
            DfgVertexVar* സ്ഥിരം thenp = it->second;
            DfgVertexVar* സ്ഥിരം elsep = pair.second;
            DfgVertexVar* സ്ഥിരം newp = joinDrivers(varp, predicatep, thenp, elsep);
            എങ്കിൽ (!newp) തിരികെ അസത്യം;
            pair.second = newp;
        }

        // Remove variables not assigned on both paths
        വേണ്ടി (AstVarScope* സ്ഥിരം varp : toRemove) elseSymTab.erase(varp);

        // Done
        തിരികെ സത്യം;
    }

    // Given two joining control flow edges, compute how to join their symbols.
    // Returns the predicaete to join over, and the 'then' and 'else' blocks.
    std::tuple<DfgVertexVar*, സ്ഥിരം CfgBlock*, സ്ഥിരം CfgBlock*>  //
    howToJoin(സ്ഥിരം CfgEdge* സ്ഥിരം ap, സ്ഥിരം CfgEdge* സ്ഥിരം bp) {
        // Find the closest common dominator of the two paths
        സ്ഥിരം CfgBlock* സ്ഥിരം domp = m_dom_വൃക്ഷം.closestCommonDominator(ap->srcp(), bp->srcp());
        // These paths join here, so 'domp' must be a branch, otherwise it's not the closest
        UASSERT_OBJ(domp->isBranch(), domp, "closestCommonDominator is not a branch");

        // The branches of the common dominator
        സ്ഥിരം CfgEdge* സ്ഥിരം takenEdgep = domp->takenEdgep();
        സ്ഥിരം CfgEdge* സ്ഥിരം untknEdgep = domp->untknEdgep();

        // We check if the taken branch dominates the path to either blocks,
        // and if the untaken branch dominates the path to the other block.
        // If so, we can use the branch condition as predicate, otherwise
        // we must use the path predicate as there are ways to get from one
        // branch of the dominator to the other. We need to be careful if
        // either branches are directly to the join block. This is fine,
        // it's as if there was an empty block on that critical edge which
        // is dominated by that path.

        എങ്കിൽ (takenEdgep == ap || m_dom_വൃക്ഷം.dominates(takenEdgep->dstp(), ap->srcp())) {
            എങ്കിൽ (untknEdgep == bp || m_dom_വൃക്ഷം.dominates(untknEdgep->dstp(), bp->srcp())) {
                // Taken path dominates 'ap' and untaken dominates 'bp', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                തിരികെ std::make_tuple(m_bbToCondp[domp], ap->srcp(), bp->srcp());
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (takenEdgep == bp || m_dom_വൃക്ഷം.dominates(takenEdgep->dstp(), bp->srcp())) {
            എങ്കിൽ (untknEdgep == ap || m_dom_വൃക്ഷം.dominates(untknEdgep->dstp(), ap->srcp())) {
                // Taken path dominates 'bp' and untaken dominates 'ap', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                തിരികെ std::make_tuple(m_bbToCondp[domp], bp->srcp(), ap->srcp());
            }
        }

        // The branches don't dominate the joined blocks, must use the path predicate
        ++m_ctx.m_synt.joinUsingPathPredicate;

        // TODO: We could do better here: use the path predicate of the closest
        // cominating blocks, pick the one from the lower rank, etc, but this
        // generic case is very rare, most synthesizable logic has
        // series-parallel CFGs which are covered by the earlier cases.
        തിരികെ std::make_tuple(m_edgeToPredicatep[ap], ap->srcp(), bp->srcp());
    }

    // Combine the output symbol tables of the predecessors of the given
    // block to compute the input symtol table for the given block.
    തർക്കം create_നിവേശം_ചിഹ്നം_Table(സ്ഥിരം CfgBlock& bb) {
        // The input symbol table of the given block, we are computing it now
        SymTab& joined = m_bbToISymTab[bb];

        // Input symbol table of entry block is special
        എങ്കിൽ (bb.isEnter()) {
            initialize_Entry_ചിഹ്നം_Table(joined);
            തിരികെ സത്യം;
        }

        // Current input symbol table should be empty, we will fill it in here
        UASSERT(joined.empty(), "Unprocessed input symbol table should be empty");

        // Fast path if there is only one predecessor - TODO: use less copying
        എങ്കിൽ (!bb.isJoin()) {
            joined = m_bbToOSymTab[bb.firstPredecessorp()];
            തിരികെ സത്യം;
        }

        // We also have a simpler job if there are 2 predecessors
        എങ്കിൽ (bb.isTwoWayJoin()) {
            DfgVertexVar* predicatep = nullptr;
            സ്ഥിരം CfgBlock* thenp = nullptr;
            സ്ഥിരം CfgBlock* elsep = nullptr;
            std::tie(predicatep, thenp, elsep)
                = howToJoin(bb.firstPredecessorEdgep(), bb.lastPredecessorEdgep());
            // Copy from else
            joined = m_bbToOSymTab[elsep];
            // Join with then
            തിരികെ join_ചിഹ്നം_Tables(joined, predicatep, m_bbToOSymTab[*thenp]);
        }

        // General hard way

        // Gather predecessors
        ഘടന Predecessor final {
            സ്ഥിരം CfgBlock* m_bbp;  // Predeccessor block
            DfgVertexVar* m_predicatep;  // Predicate predecessor reached this block with
            സ്ഥിരം SymTab* m_oSymTabp;  // Output symbol table or predecessor
            Predecessor() = delete;
            Predecessor(സ്ഥിരം CfgBlock* bbp, DfgVertexVar* predicatep, സ്ഥിരം SymTab* oSymTabp)
                : m_bbp{bbp}
                , m_predicatep{predicatep}
                , m_oSymTabp{oSymTabp} {}
        };

        സ്ഥിരം std::vector<Predecessor> predecessors = [&]() {
            std::vector<Predecessor> res;
            വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : bb.inEdges()) {
                സ്ഥിരം CfgEdge& cfgEdge = static_cast<സ്ഥിരം CfgEdge&>(edge);
                സ്ഥിരം CfgBlock* സ്ഥിരം predecessorp = cfgEdge.srcp();
                DfgVertexVar* സ്ഥിരം predicatep = m_edgeToPredicatep[cfgEdge];
                സ്ഥിരം SymTab* സ്ഥിരം oSymTabp = &m_bbToOSymTab[predecessorp];
                res.emplace_back(predecessorp, predicatep, oSymTabp);
            }
            // Sort predecessors reverse topologically. This way earlier blocks
            // will come after later blocks, and the entry block is last if present.
            std::sort(res.begin(), res.end(), [](സ്ഥിരം Predecessor& a, സ്ഥിരം Predecessor& b) {  //
                തിരികെ *a.m_bbp > *b.m_bbp;
            });
            തിരികെ res;
        }();

        // Start by copying the bindings from the frist predecessor
        joined = *predecessors[0].m_oSymTabp;
        // Join over all other predecessors
        വേണ്ടി (size_t i = 1; i < predecessors.size(); ++i) {
            DfgVertexVar* സ്ഥിരം predicatep = predecessors[i].m_predicatep;
            സ്ഥിരം SymTab& oSymTab = *predecessors[i].m_oSymTabp;
            എങ്കിൽ (!join_ചിഹ്നം_Tables(joined, predicatep, oSymTab)) തിരികെ അസത്യം;
        }

        തിരികെ സത്യം;
    }

    std::vector<Driver> computePropagatedDrivers(സ്ഥിരം std::vector<Driver>& newDrivers,
                                                 DfgVertexVar* oldp) {
        // Gather drivers of 'oldp' - they are in incresing range order with no overlaps
        സ്ഥിരം std::vector<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.empty(), oldp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::vector<Driver> propagatedDrivers;

        // Add bits between 'msb' and 'lsb' from 'oldp' to 'pDrivers'
        സ്ഥിരം auto addOldDriver = [&](File_വരി* സ്ഥിരം flp, uint32_t msb, uint32_t lsb) {
            UASSERT_OBJ(propagatedDrivers.empty() || lsb > propagatedDrivers.back().m_hi, flp,
                        "Drivers should be in ascending order");
            DfgSel* സ്ഥിരം selp = make<DfgSel>(flp, DfgDataType::packed(msb - lsb + 1));
            selp->lsb(lsb);
            selp->fromp(oldp);
            propagatedDrivers.emplace_back(selp, lsb, flp);
        };

        // Incorporate old drivers
        വേണ്ടി (സ്ഥിരം Driver& oDriver : oldDrivers) {
            File_വരി* സ്ഥിരം flp = oDriver.m_flp;
            // Range to consider inserting, we will adjust oldLo as we process drivers
            uint32_t oldLo = oDriver.m_lo;
            സ്ഥിരം uint32_t oldHi = oDriver.m_hi;

            // Loop for now, can move to bisection search if this is a problem, shouldn't be ...
            വേണ്ടി (സ്ഥിരം Driver& nDriver : newDrivers) {
                UASSERT_OBJ(oldHi >= oldLo, flp, "Should have stopped iteration");
                // If new driver is entirely below old driver, move on to
                എങ്കിൽ (nDriver.m_hi < oldLo) തുടരുക;
                // If new driver is entirely above old driver, we can stop
                എങ്കിൽ (oldHi < nDriver.m_lo) നിർത്തുക;

                // There is an overlap between 'oDriver' and 'nDriver'.
                // Insert the low bits and adjust the insertion range.
                // The rest will take care of itself on subsequent iterations.
                എങ്കിൽ (oldLo < nDriver.m_lo) addOldDriver(flp, nDriver.m_lo - 1, oldLo);
                oldLo = nDriver.m_hi + 1;

                // Stop if no more bits remaining in the old driver
                എങ്കിൽ (oldLo > oldHi) നിർത്തുക;
            }

            // Insert remaining bits if any
            എങ്കിൽ (oldHi >= oldLo) addOldDriver(flp, oldHi, oldLo);
        }

        തിരികെ propagatedDrivers;
    }

    // Propagate whole elements with a linear walk over the sorted driver lists.
    // Reads use the previous value temporary, preserving assignment-version bindings.
    // Partial elements and non-unit drivers retain the nonsynthesized-process fallback.
    തർക്കം computePropagatedArrayDrivers(സ്ഥിരം std::vector<Driver>& newDrivers,
                                       DfgVertexVar* സ്ഥിരം oldp,
                                       std::vector<Driver>& propagatedDrivers) {
        // Bound quadratic vertex growth: array drivers cannot be coalesced.
        സ്ഥിതം constexpr uint32_t MAX_ARRAY_ELEMENTS = 32;
        എങ്കിൽ (oldp->size() > MAX_ARRAY_ELEMENTS) തിരികെ അസത്യം;
        വേണ്ടി (സ്ഥിരം Driver& driver : newDrivers) {
            എങ്കിൽ (!driverCoversWholeElement(driver)) തിരികെ അസത്യം;
        }
        സ്ഥിരം std::vector<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.empty(), oldp, "Should have a proper driver");
        വേണ്ടി (സ്ഥിരം Driver& driver : oldDrivers) {
            എങ്കിൽ (!driverCoversWholeElement(driver)) തിരികെ അസത്യം;
        }

        propagatedDrivers.reserve(oldDrivers.size());
        auto nIt = newDrivers.begin();
        വേണ്ടി (സ്ഥിരം Driver& oDriver : oldDrivers) {
            ആകുമ്പോൾ (nIt != newDrivers.end() && nIt->m_lo < oDriver.m_lo) ++nIt;
            എങ്കിൽ (nIt != newDrivers.end() && nIt->m_lo == oDriver.m_lo) തുടരുക;

            File_വരി* സ്ഥിരം flp = oDriver.m_flp;
            സ്ഥിരം DfgUnitArray* സ്ഥിരം oldUnitp = oDriver.m_vtxp->as<DfgUnitArray>();
            DfgArraySel* സ്ഥിരം selp = make<DfgArraySel>(flp, oldUnitp->srcp()->dtype());
            selp->fromp(oldp);
            selp->bitp(make<DfgConst>(flp, static_cast<size_t>(VL_IDATASIZE), oDriver.m_lo));
            DfgUnitArray* സ്ഥിരം newUnitp = make<DfgUnitArray>(flp, oldUnitp->dtype());
            newUnitp->srcp(selp);
            propagatedDrivers.emplace_back(newUnitp, oDriver.m_lo, flp);
        }
        തിരികെ സത്യം;
    }

    // Given the drivers of a variable after converting a single statement
    // 'newp', add drivers from 'oldp' that were not reassigned be drivers
    // in newp. This computes the total result of all previous assignments.
    തർക്കം incorporate_Previous_മൂല്യം(AstVarScope* vscp, DfgVertexVar* newp, DfgVertexVar* oldp) {
        UASSERT_OBJ(newp->srcp(), vscp, "Assigned variable has no driver");

        // Easy if there is no old value...
        എങ്കിൽ (!oldp) തിരികെ സത്യം;

        // New driver was not yet coalesced, so should always be a splice
        DfgVertexSplice* സ്ഥിരം nSplicep = newp->srcp()->as<DfgVertexSplice>();

        // If the old value is the real variable we just computed the new value for,
        // then it is the circular feedback into the synthesized block, add it as default driver.
        എങ്കിൽ (oldp->vscp() == vscp) {
            എങ്കിൽ (!spliceCoversWhole(nSplicep)) newp->defaultp(oldp);
            തിരികെ സത്യം;
        }

        UASSERT_OBJ(oldp->srcp(), vscp, "Previously assigned variable has no driver");

        // Gather drivers of 'newp' - they are in incresing range order with no overlaps
        UASSERT_OBJ(!newp->defaultp(), newp, "Converted value should not have default");
        std::vector<Driver> nDrivers = gatherDrivers(newp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!nDrivers.empty(), newp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::vector<Driver> pDrivers;
        എങ്കിൽ (newp->isPacked()) {
            pDrivers = computePropagatedDrivers(nDrivers, oldp);
        } അല്ലെങ്കിൽ എങ്കിൽ (!computePropagatedArrayDrivers(nDrivers, oldp, pDrivers)) {
            ++m_ctx.m_synt.nonSynArray;
            തിരികെ അസത്യം;
        }

        എങ്കിൽ (!pDrivers.empty()) {
            // Need to merge propagated sources, so reset the splice
            nSplicep->resetDrivers();
            // Merge drivers - they are both sorted and non-overlapping
            std::vector<Driver> drivers;
            drivers.reserve(nDrivers.size() + pDrivers.size());
            std::merge(nDrivers.begin(), nDrivers.end(), pDrivers.begin(), pDrivers.end(),
                       std::back_inserter(drivers));
            // Coalesce adjacent ranges
            എങ്കിൽ (newp->isPacked()) coalesceDrivers(drivers);
            // Reinsert drivers in order
            വേണ്ടി (സ്ഥിരം Driver& d : drivers) nSplicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        }

        // If the old had a default, add to the new one too, unless redundant
        എങ്കിൽ (oldp->defaultp() && !spliceCoversWhole(nSplicep)) newp->defaultp(oldp->defaultp());

        // Done
        തിരികെ സത്യം;
    }

    // Synthesize the given statements with the given input symbol table.
    // Returns true if successfolly synthesized.
    // Populates the given output symbol table.
    // Populates the given reference with the condition of the terminator branch, if any.
    തർക്കം synthesizeBasicBlock(SymTab& oSymTab, DfgVertex*& condpr,
                              സ്ഥിരം std::vector<Ast_കണ്ണി_Stmt*>& stmtps, സ്ഥിരം SymTab& iSymTab) {
        // Use fresh set of vertices in m_converter
        സ്ഥിരം VNUser2InUse user2InUse;

        // Initialize AstVarScope -> Vertex bindings available in this block
        വേണ്ടി (സ്ഥിരം auto& pair : iSymTab) {
            AstVarScope* സ്ഥിരം varp = pair.first;
            DfgVertexVar* സ്ഥിരം vtxp = pair.second;
            varp->user2p(vtxp);
            oSymTab[varp] = vtxp;
        }

        // Synthesize each statement one after the other
        std::vector<std::pair<AstVarScope*, DfgVertexVar*>> updates;
        വേണ്ടി (Ast_കണ്ണി_Stmt* സ്ഥിരം stmtp : stmtps) {
            // Regular statements
            Ast_കണ്ണി_Assign* സ്ഥിരം ap = VN_CAST(stmtp, കണ്ണി_Assign);
            എങ്കിൽ (ap && (VN_IS(ap, Assign) || VN_IS(ap, AssignW))) {
                // Convert this assignment
                എങ്കിൽ (!m_converter.convert(updates, *m_logicp, ap)) {
                    ++m_ctx.m_synt.nonSynConv;
                    തിരികെ അസത്യം;
                }
                // Apply variable updates from this statement
                വേണ്ടി (സ്ഥിരം auto& pair : updates) {
                    // The target variable that was assigned to
                    AstVarScope* സ്ഥിരം vscp = pair.first;
                    // The new, potentially partially assigned value
                    DfgVertexVar* സ്ഥിരം newp = pair.second;
                    // Normalize drivers within this statement, bail if multidriven
                    DfgVertexSplice* സ്ഥിരം srcp = newp->srcp()->as<DfgVertexSplice>();
                    std::vector<Driver> drivers = gatherDrivers(srcp);
                    സ്ഥിരം തർക്കം single = drivers.size() == 1;
                    എങ്കിൽ (!normalizeDrivers(*newp, drivers)) {
                        ++m_ctx.m_synt.nonSynMultidrive;
                        തിരികെ അസത്യം;
                    }
                    // If there were more than one driver (often not), re-add in case coalesced
                    എങ്കിൽ (!single) {
                        srcp->resetDrivers();
                        വേണ്ടി (സ്ഥിരം Driver& d : drivers) srcp->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
                    }
                    // The old value, if any
                    DfgVertexVar* സ്ഥിരം oldp = vscp->user2u().template to<DfgVertexVar*>();
                    // Inncorporate old value into the new value
                    എങ്കിൽ (!incorporate_Previous_മൂല്യം(vscp, newp, oldp)) തിരികെ അസത്യം;
                    // Update binding of target variable
                    vscp->user2p(newp);
                    // Update output symbol table of this block
                    oSymTab[vscp] = newp;
                }
                updates.clear();
                തുടരുക;
            }

            // Terminator branches
            എങ്കിൽ (AstIf* സ്ഥിരം ifp = VN_CAST(stmtp, If)) {
                UASSERT_OBJ(ifp == stmtps.back(), ifp, "Branch should be last statement");
                // Convert condition, give up if failed
                DfgVertex* condp = m_converter.convert(*m_logicp, ifp->condp());
                എങ്കിൽ (!condp) {
                    ++m_ctx.m_synt.nonSynConv;
                    തിരികെ അസത്യം;
                }
                // Single bit condition can be use directly, otherwise: use 'condp != 0'
                എങ്കിൽ (condp->width() != 1) {
                    File_വരി* സ്ഥിരം flp = condp->fileline();
                    DfgNeq* സ്ഥിരം neqp = make<DfgNeq>(flp, DfgDataType::packed(1));
                    neqp->lhsp(make<DfgConst>(flp, condp->width(), 0U));
                    neqp->rhsp(condp);
                    condp = neqp;
                }
                condpr = condp;
                തുടരുക;
            }

            // Unhandled
            ++m_ctx.m_synt.nonSynStmt;
            തിരികെ അസത്യം;
        }

        തിരികെ സത്യം;
    }

    // Assign path predicates to the outgoing control flow edges of the given block
    ശൂന്യം assignPathPredicates(സ്ഥിരം CfgBlock& bb) {
        // Nothing to do for the exit block
        എങ്കിൽ (bb.isExit()) തിരികെ;

        // Get the predicate of this block
        DfgVertex* സ്ഥിരം predp = [&]() -> DfgVertex* {
            // Entry block has no predecessors, use constant true
            എങ്കിൽ (bb.isEnter()) തിരികെ make<DfgConst>(m_logicp->fileline(), 1U, 1U);

            // For any other block, 'or' together all the incoming predicates
            സ്ഥിരം auto& inEdges = bb.inEdges();
            auto it = inEdges.begin();
            DfgVertex* resp = m_edgeToPredicatep[static_cast<സ്ഥിരം CfgEdge&>(*it)];
            ആകുമ്പോൾ (++it != inEdges.end()) {
                DfgOr* സ്ഥിരം orp = make<DfgOr>(resp->fileline(), resp->dtype());
                orp->rhsp(resp);
                orp->lhsp(m_edgeToPredicatep[static_cast<സ്ഥിരം CfgEdge&>(*it)]);
                resp = orp;
            }
            തിരികെ resp;
        }();

        സ്ഥിരം DfgDataType& dtype = predp->dtype();

        സ്ഥിരം auto mkTmp = [&](File_വരി* flp, സ്ഥിരം അക്ഷരം* name, DfgVertex* srcp) {
            സ്ഥിരം std::string prefix = "_BB" + std::to_string(bb.id()) + "_" + name;
            DfgVertexVar* സ്ഥിരം tmpp = m_converter.createTmp(*m_logicp, flp, dtype, prefix);
            tmpp->srcp(srcp);
            തിരികെ tmpp;
        };

        // Assign it to a variable in case it's used multiple times
        DfgVertexVar* സ്ഥിരം pInp = mkTmp(predp->fileline(), "PathIn", predp);

        // For uncondional branches, the successor predicate edge is the same
        എങ്കിൽ (!bb.isBranch()) {
            m_edgeToPredicatep[bb.takenEdgep()] = mkTmp(pInp->fileline(), "Goto", pInp);
            തിരികെ;
        }

        // For branches, we need to factor in the branch condition
        DfgVertex* സ്ഥിരം condp = m_bbToCondp[bb];
        File_വരി* സ്ഥിരം flp = condp->fileline();

        // Predicate for taken branch: 'predp & condp'
        {
            DfgAnd* സ്ഥിരം takenPredp = make<DfgAnd>(flp, dtype);
            takenPredp->lhsp(pInp);
            takenPredp->rhsp(condp);
            m_edgeToPredicatep[bb.takenEdgep()] = mkTmp(flp, "Taken", takenPredp);
        }

        // Predicate for untaken branch: 'predp & ~condp'
        {
            DfgAnd* സ്ഥിരം untknPredp = make<DfgAnd>(flp, dtype);
            untknPredp->lhsp(pInp);
            DfgNot* സ്ഥിരം notp = make<DfgNot>(flp, dtype);
            notp->srcp(condp);
            untknPredp->rhsp(notp);
            m_edgeToPredicatep[bb.untknEdgep()] = mkTmp(flp, "Untkn", untknPredp);
        }
    }

    // Returns true if all external updates to volatile variables are observed correctly
    തർക്കം checkExtWrites() {
        വേണ്ടി (സ്ഥിരം DfgVertex* സ്ഥിരം vtxp : m_logicp->synth()) {
            സ്ഥിരം DfgVertexVar* സ്ഥിരം varp = vtxp->cast<DfgVertexVar>();
            എങ്കിൽ (!varp) തുടരുക;
            // If the variable we synthesized this vertex for is volatile, and
            // the value of the synthesized temporary is observed, we might be
            // missing an external update, so we mut give up.
            എങ്കിൽ (!varp->hasSinks()) തുടരുക;
            എങ്കിൽ (!DfgVertexVar::isVolatile(varp->tmpForp())) തുടരുക;
            ++m_ctx.m_synt.nonSynExtWrite;
            തിരികെ അസത്യം;
        }
        തിരികെ സത്യം;
    }

    // Add the synthesized values as drivers to the output variables of the current DfgLogic
    തർക്കം addSynthesizedOutput(SymTab& oSymTab) {
        // It's possible we think a variable is written by the DfgLogic when
        // it actauly isn't, e.g.: '{a[0], b[0]}[1] = ...' does not write 'b'.
        // These LHS forms can happen after some earlier tranforms. We
        // should just run V3Const on them earlier, but we will do belt and
        // braces and check here too. We can't touch any output variables if so.
        സ്ഥിരം തർക്കം missing = m_logicp->foreachSink([&](സ്ഥിരം DfgVertex& sink) {
            സ്ഥിരം DfgUnresolved* സ്ഥിരം unresolvedp = sink.as<DfgUnresolved>();
            AstVarScope* സ്ഥിരം vscp = unresolvedp->singleSink()->as<DfgVertexVar>()->vscp();
            തിരികെ !oSymTab.എണ്ണം(vscp);
        });
        എങ്കിൽ (missing) {
            ++m_ctx.m_synt.nonSynFalseWrite;
            തിരികെ അസത്യം;
        }

        // Add sinks to read the computed values for the target variables
        തിരികെ !m_logicp->foreachSink([&](DfgVertex& sink) {
            DfgUnresolved* സ്ഥിരം unresolvedp = sink.as<DfgUnresolved>();
            സ്ഥിരം DfgVertexVar* സ്ഥിരം varp = unresolvedp->singleSink()->as<DfgVertexVar>();
            DfgVertexVar* സ്ഥിരം resp = oSymTab.at(varp->vscp());
            UASSERT_OBJ(resp->srcp(), resp, "Undriven result");

            // If the output is not used further in the synthesized logic itself,
            // then resp will be deleted before we return, so we can just use
            // its splice directly without ending up with a multi-use operation.
            എങ്കിൽ (!resp->hasSinks()) {
                unresolvedp->addDriver(resp->srcp()->as<DfgVertexSplice>());
                തിരികെ അസത്യം;  // OK, continue.
            }

            // TODO: computePropagatedDrivers cannot handle arrays, should
            // never happen with simple continous assignments
            എങ്കിൽ (!resp->isPacked()) {
                ++m_ctx.m_synt.nonSynArray;
                തിരികെ സത്യം;  // Not OK, give up
            }

            // We need to add a new splice to avoid multi-use of the original splice
            DfgSplicePacked* സ്ഥിരം splicep
                = പുതിയ DfgSplicePacked{m_dfg, resp->fileline(), resp->dtype()};
            // Drivers are the same
            സ്ഥിരം std::vector<Driver> drivers = computePropagatedDrivers({}, resp);
            വേണ്ടി (സ്ഥിരം Driver& d : drivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
            unresolvedp->addDriver(splicep);
            തിരികെ അസത്യം;  // OK, continue
        });
    }

    // Synthesize the given AstAssignW. Returns true on success.
    തർക്കം synthesizeAssignW(AstAssignW* nodep) {
        ++m_ctx.m_synt.നിവേശം_Assign;

        // The input and output symbol tables
        SymTab iSymTab;
        SymTab oSymTab;

        // Initialzie input symbol table
        initialize_Entry_ചിഹ്നം_Table(iSymTab);

        // Synthesize as if it was in a single CfgBlock CFG
        DfgVertex* condp = nullptr;
        സ്ഥിരം തർക്കം success = synthesizeBasicBlock(oSymTab, condp, {nodep}, iSymTab);
        UASSERT_OBJ(!condp, nodep, "Conditional AstAssignW ???");
        എങ്കിൽ (!success) തിരികെ അസത്യം;

        // Check exernal writes are observed correctly
        എങ്കിൽ (!checkExtWrites()) തിരികെ അസത്യം;

        // Add resolved output variable drivers
        തിരികെ addSynthesizedOutput(oSymTab);
    }

    // Synthesize the given AstAlways. Returns true on success.
    തർക്കം synthesizeCfg(CfgGraph& cfg) {
        ++m_ctx.m_synt.നിവേശം_Always;

        // If there is a backward edge (loop), we can't synthesize it
        എങ്കിൽ (cfg.containsLoop()) {
            ++m_ctx.m_synt.nonSynLoop;
            ++m_ctx.m_synt.cfgCyclic;
            തിരികെ അസത്യം;
        }

        // If it's a trivial CFG we can save on some work
        എങ്കിൽ (cfg.nBlocks() == 1) {
            ++m_ctx.m_synt.cfgTrivial;
        } അല്ലെങ്കിൽ {
            // Insert two-way join blocks to aid multiplexer ordering
            എങ്കിൽ (cfg.തിരുകുക_Two_Way_Joins()) {
                ++m_ctx.m_synt.cfgSp;
            } അല്ലെങ്കിൽ {
                ++m_ctx.m_synt.cfgDag;
            }
            // Initialize maps needed for non-trivial CFGs
            m_dom_വൃക്ഷം = Cfg_Dominator_വൃക്ഷം{cfg};
            m_edgeToPredicatep = cfg.makeEdgeMap<DfgVertexVar*>();
        }

        // Initialize CfgMaps
        m_bbToISymTab = cfg.makeBlockMap<SymTab>();
        m_bbToOSymTab = cfg.makeBlockMap<SymTab>();
        m_bbToCondp = cfg.makeBlockMap<DfgVertexVar*>();

        // Synthesize all blocks
        വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : cfg.vertices()) {
            സ്ഥിരം CfgBlock& bb = static_cast<സ്ഥിരം CfgBlock&>(vtx);
            // Prepare the input symbol table of this block (enter, or join predecessor blocks)
            എങ്കിൽ (!create_നിവേശം_ചിഹ്നം_Table(bb)) തിരികെ അസത്യം;
            // Synthesize this block
            DfgVertex* condp = nullptr;
            എങ്കിൽ (!synthesizeBasicBlock(m_bbToOSymTab[bb], condp, bb.stmtps(), m_bbToISymTab[bb])) {
                തിരികെ അസത്യം;
            }
            // Create a temporary for the branch condition as it might be used multiple times
            എങ്കിൽ (condp) {
                File_വരി* സ്ഥിരം flp = condp->fileline();
                സ്ഥിരം DfgDataType& dtype = condp->dtype();
                സ്ഥിരം std::string prefix = "_BB" + std::to_string(bb.id()) + "_Cond";
                DfgVertexVar* സ്ഥിരം vp = m_converter.createTmp(*m_logicp, flp, dtype, prefix);
                vp->srcp(condp);
                m_bbToCondp[bb] = vp;
            }
            // Set the path predicates on the successor edges
            assignPathPredicates(bb);
        }

        // Check exernal writes are observed correctly
        എങ്കിൽ (!checkExtWrites()) തിരികെ അസത്യം;

        // Add resolved output variable drivers
        തിരികെ addSynthesizedOutput(m_bbToOSymTab[cfg.exit()]);
    }

    // Synthesize a DfgLogic into regular vertices. Returns ture on success.
    തർക്കം synthesize(DfgLogic& vtx) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;

        എങ്കിൽ (AstAssignW* സ്ഥിരം ap = VN_CAST(vtx.nodep()->stmtsp(), AssignW)) {
            എങ്കിൽ (ap->nextp()) തിരികെ അസത്യം;
            എങ്കിൽ (!synthesizeAssignW(ap)) തിരികെ അസത്യം;
            ++m_ctx.m_synt.synthAssign;
            തിരികെ സത്യം;
        }

        എങ്കിൽ (!synthesizeCfg(vtx.cfg())) തിരികെ അസത്യം;
        ++m_ctx.m_synt.synthAlways;
        തിരികെ സത്യം;
    }

    // Revert all DfgLogic in m_toRevert, or DfgLogic driving the DfgUnresolved
    // vertices in m_toRevert, and transitively the same for any DfgUnresolved
    // driven by the reverted DfgLogic. Delete all DfgUnresolved involed.
    ശൂന്യം revert(VDouble0& statCountr) {
        m_toRevert.foreach([&](DfgVertex& vtx) {
            // Process DfgLogic
            എങ്കിൽ (DfgLogic* സ്ഥിരം vtxp = vtx.cast<DfgLogic>()) {
                UASSERT_OBJ(vtxp->selectedForSynthesis(), vtxp, "Shouldn't reach here unselected");
                // Revert this logic
                UASSERT_OBJ(!vtxp->reverted(), vtxp, "Should be reverting now");
                ++statCountr;
                വേണ്ടി (DfgVertex* സ്ഥിരം p : vtxp->synth()) VL_DO_DANGLING(p->unlinkDelete(m_dfg), p);
                vtxp->synth().clear();
                vtxp->setReverted();
                // Add all DfgUnresolved it drives to the work list
                vtxp->foreachSink([&](DfgVertex& snk) {
                    m_toRevert.തള്ളുക_front(*snk.as<DfgUnresolved>());
                    തിരികെ അസത്യം;
                });
                തിരികെ;
            }

            // Process DfgUnresolved
            എങ്കിൽ (DfgUnresolved* സ്ഥിരം vtxp = vtx.cast<DfgUnresolved>()) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->setHasModWrRefs();
                // Add all driving DfgLogic to the work list
                vtxp->foreachSource([&](DfgVertex& src) {
                    DfgLogic* സ്ഥിരം lp = src.cast<DfgLogic>();
                    എങ്കിൽ (lp && !lp->reverted()) m_toRevert.തള്ളുക_front(*lp);
                    തിരികെ അസത്യം;
                });
                // Delete the DfgUnresolved driver
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
                തിരികെ;
            }

            // There should be nothing else on the worklist
            vtx.v3fatalSrc("Unexpected vertex type");
        });
    }

    // Synthesize all of the given vertices
    ശൂന്യം പ്രധാന() {
        //-------------------------------------------------------------------
        UINFO(5, "Step 0: Remove all DfgLogic not selected for synthesis");
        വേണ്ടി (DfgVertex* സ്ഥിരം vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgUnresolved
            എങ്കിൽ (!vtxp->is<DfgUnresolved>()) തുടരുക;
            തർക്കം anySelected = അസത്യം;
            തർക്കം anyUnselected = അസത്യം;
            vtxp->foreachSource([&](DfgVertex& src) {
                സ്ഥിരം DfgLogic& logic = *src.as<DfgLogic>();
                എങ്കിൽ (logic.selectedForSynthesis()) {
                    anySelected = സത്യം;
                } അല്ലെങ്കിൽ {
                    anyUnselected = സത്യം;
                }
                തിരികെ അസത്യം;
            });
            // There should be a driver
            UASSERT_OBJ(anySelected || anyUnselected, vtxp, "'DfgUnresolved' with no driver");
            // All drivers should be selected or all should be unselected
            UASSERT_OBJ(!(anySelected && anyUnselected), vtxp, "Invalid 'DfgLogic' selection");
            // If all drivers are unselected, delete this DfgUnresolved here
            എങ്കിൽ (anyUnselected) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->setHasModWrRefs();
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
        വേണ്ടി (DfgVertex* സ്ഥിരം vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgLogic
            DfgLogic* സ്ഥിരം logicp = vtxp->cast<DfgLogic>();
            എങ്കിൽ (!logicp) തുടരുക;
            എങ്കിൽ (logicp->selectedForSynthesis()) തുടരുക;
            // There should be no sinks left for unselected DfgLogic, delete them here
            UASSERT_OBJ(!logicp->hasSinks(), vtxp, "Unselected 'DfgLogic' with sinks remaining");
            // Input variables will be read in Ast code, add Ast reference vertices
            // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
            സ്ഥിരം VNUser4InUse m_user4InUse;
            logicp->foreachSource([](DfgVertex& src) {
                src.as<DfgVertexVar>()->vscp()->user4p(&src);
                തിരികെ അസത്യം;
            });
            V3DfgPasses::addAstRefs(m_dfg, logicp->nodep(), [](Ast_കണ്ണി* varp) {  //
                തിരികെ varp->user4u().to<DfgVertexVar*>();
            });
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        debugDump("synth-selected");

        //-------------------------------------------------------------------
        UINFO(5, "Step 1: Attempting to synthesize each of the selected DfgLogic");
        {
            // AstVarScope::user4() -> int: unique ID for 'AstVarScopeComparator'
            സ്ഥിരം VNUser4InUse user4InUse;
            വേണ്ടി (DfgVertex& vtx : m_dfg.opVertices()) {
                DfgLogic* സ്ഥിരം logicp = vtx.cast<DfgLogic>();
                എങ്കിൽ (!logicp) തുടരുക;

                // We should only have DfgLogic remaining that was selected for synthesis
                UASSERT_OBJ(logicp->selectedForSynthesis(), logicp, "Unselected DfgLogic remains");

                // Debug aid
                സ്ഥിരം auto debugCallback = [&]() -> ശൂന്യം {
                    // This is the breaking logic
                    m_debugLogicp = logicp;
                    // Dump it
                    UINFOTREE(0, logicp->nodep(), "Problematic DfgLogic: " << logicp, "  ");
                    V3EmitV::debug_Verilog_For_വൃക്ഷം(logicp->nodep(), std::cout);
                    debugDump("synth-lastok");
                };
                എങ്കിൽ (VL_UNLIKELY(s_dfgSynthDebugBisect.stop(debugCallback))) നിർത്തുക;

                // Synthesize it, if failed, enqueue for reversion
                എങ്കിൽ (!synthesize(*logicp)) {
                    logicp->setNonSynthesizable();
                    m_toRevert.തള്ളുക_front(*logicp);
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
        std::unordered_map<സ്ഥിരം DfgVertexVar*, DfgVertexSplice*> resolvedDrivers;
        // Compute resolved drivers of all variablees
        വേണ്ടി (DfgVertexVar& var : m_dfg.varVertices()) {
            എങ്കിൽ (!var.srcp()) തുടരുക;
            DfgUnresolved* സ്ഥിരം unresolvedp = var.srcp()->cast<DfgUnresolved>();
            എങ്കിൽ (!unresolvedp) നിർത്തുക;  // Stop when reached the synthesized temporaries

            // Resolve the synthesized drivers
            DfgVertexSplice* സ്ഥിരം resolvedp = [&]() -> DfgVertexSplice* {
                // All synthesized drivers were normalized already,
                // so if there is only one, it can be used directly
                എങ്കിൽ (unresolvedp->nInputs() == 1) {
                    തിരികെ unresolvedp->inputp(0)->as<DfgVertexSplice>();
                }
                // Otherwise gather the synthesized drivers
                std::vector<Driver> drivers = gatherDriversUnresolved(unresolvedp);
                // Normalize them, make resolved driver if all good
                എങ്കിൽ (normalizeDrivers(var, drivers)) തിരികെ makeSplice(var, drivers);
                // If mutlidriven, record and ignore
                multidrivenps.emplace_back(&var);
                m_toRevert.തള്ളുക_front(*unresolvedp);
                തിരികെ nullptr;
            }();
            // Bail if multidriven
            എങ്കിൽ (!resolvedp) തുടരുക;
            // Add to map for next loop
            സ്ഥിരം തർക്കം newEntry = resolvedDrivers.emplace(&var, resolvedp).second;
            UASSERT_OBJ(newEntry, &var, "Duplicate driver");
        }
        // Revert and remove drivers of multi-driven variables
        revert(m_ctx.m_synt.revertMultidrive);
        // Replace all DfgUnresolved with the resolved drivers
        വേണ്ടി (സ്ഥിരം DfgVertexVar& var : m_dfg.varVertices()) {
            എങ്കിൽ (!var.srcp()) തുടരുക;
            DfgUnresolved* സ്ഥിരം srcp = var.srcp()->cast<DfgUnresolved>();
            എങ്കിൽ (!srcp) നിർത്തുക;  // Stop when reached the synthesized temporaries

            // Replace it
            srcp->replaceWith(resolvedDrivers.at(&var));
            VL_DO_DANGLING(srcp->unlinkDelete(m_dfg), srcp);
        }
        debugDump("synth-resolved");

        //-------------------------------------------------------------------
        UINFO(5, "Step 4: Remove all remaining DfgLogic and DfgUnresolved");
        വേണ്ടി (DfgVertex* സ്ഥിരം vtxp : m_dfg.opVertices().unlinkable()) {
            // Previous step should have removed all DfgUnresolved
            UASSERT_OBJ(!vtxp->is<DfgUnresolved>(), vtxp, "DfgUnresolved remains");

            // Process only DfgLogic
            DfgLogic* സ്ഥിരം logicp = vtxp->cast<DfgLogic>();
            എങ്കിൽ (!logicp) തുടരുക;

            // Earlier pass should have removed all sinks
            UASSERT_OBJ(!logicp->hasSinks(), logicp, "DfgLogic sink remains");

            എങ്കിൽ (!logicp->synth().empty()) {
                // If synthesized, delete the corresponding AstNode. It is now in Dfg.
                logicp->nodep()->unlinkFrBack()->delete_വൃക്ഷം();
            } അല്ലെങ്കിൽ {
                // Not synthesized. Logic stays in Ast. Add Ast reference vertices.
                // Outputs already marked by revertTransivelyAndRemove.
                // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
                സ്ഥിരം VNUser4InUse m_user4InUse;
                logicp->foreachSource([](DfgVertex& src) {
                    src.as<DfgVertexVar>()->vscp()->user4p(&src);
                    തിരികെ അസത്യം;
                });
                V3DfgPasses::addAstRefs(m_dfg, logicp->nodep(), [](Ast_കണ്ണി* varp) {  //
                    തിരികെ varp->user4u().to<DfgVertexVar*>();
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
        വേണ്ടി (DfgVertex* സ്ഥിരം vtxp : m_dfg.opVertices().unlinkable()) {
            DfgVertexSplice* സ്ഥിരം splicep = vtxp->cast<DfgVertexSplice>();
            എങ്കിൽ (!splicep) തുടരുക;

            // Might not have a sink if the driving logic was revered, remove
            എങ്കിൽ (!splicep->hasSinks()) {
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
                തുടരുക;
            }

            // It should alway have drivers
            UASSERT_OBJ(splicep->nInputs(), splicep, "Splice with no drivers");

            // If redundant, remove it
            എങ്കിൽ (DfgVertex* സ്ഥിരം wholep = splicep->wholep()) {
                എങ്കിൽ (DfgVertexVar* സ്ഥിരം varp = splicep->singleSink()->cast<DfgVertexVar>()) {
                    varp->driver_File_വരി(splicep->driver_File_വരി(0));
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
        പ്രധാന();
    }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(DfgGraph& dfg, V3DfgSynthesisContext& ctx) {
        AstToDfgSynthesize{dfg, ctx};

        // Final step outside, as both AstToDfgSynthesize and removeUnused used DfgUserMap
        UINFO(5, "Step 6: Remove all unused vertices");
        V3DfgPasses::removeUnused(dfg);
        എങ്കിൽ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("synth-rmunused");

        // No operation vertex should have multiple sinks. Cyclic decomoposition
        // depends on this and it can easily be ensured by using temporaries.
        // Also, all sources should be connected at this point
        എങ്കിൽ (v3Global.opt.debugCheck()) {
            വേണ്ടി (DfgVertex& vtx : dfg.opVertices()) {
                UASSERT_OBJ(!vtx.hasMultipleSinks(), &vtx, "Operation has multiple sinks");
                വേണ്ടി (size_t i = 0; i < vtx.nInputs(); ++i) {
                    UASSERT_OBJ(vtx.inputp(i), &vtx, "Unconnected source operand");
                }
            }
            V3DfgPasses::typeCheck(dfg);
        }
    }
};

പൂർണ്ണസംഖ്യ AstToDfgSynthesize::AstVarScopeComparator::s_vscpIdCounter = 0;

// Decide which DfgLogic to attempt to synthesize
സ്ഥിതം ശൂന്യം dfgSelectLogicForSynthesis(DfgGraph& dfg) {
    // If we are told to synthesize everything, we will do so ...
    എങ്കിൽ (v3Global.opt.fDfgSynthesizeAll()) {
        വേണ്ടി (DfgVertex& vtx : dfg.opVertices()) {
            എങ്കിൽ (DfgLogic* സ്ഥിരം logicp = vtx.cast<DfgLogic>()) logicp->setSelectedForSynthesis();
        }
        തിരികെ;
    }

    // Otherwise figure out which vertices are likely worth synthesizing.

    std::vector<DfgVertexVar*> circularVarps;
    // Synthesize all drivers of circular variables
    {
        DfgUserMap<uint64_t> scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, scc);
        വേണ്ടി (DfgVertexVar& var : dfg.varVertices()) {
            എങ്കിൽ (!scc.at(var)) തുടരുക;
            // This is a circular variable
            circularVarps.emplace_back(&var);
        }
    }

    // To cover all drivers, use a work list
    DfgWorklist worklist{dfg};

    // Synthesize all drivers of circular variables
    വേണ്ടി (സ്ഥിരം DfgVertexVar* സ്ഥിരം varp : circularVarps) {
        varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
            worklist.തള്ളുക_front(*driver.as<DfgLogic>());
            തിരികെ അസത്യം;
        });
    }

    // Choose some simple special cases to always synthesize
    വേണ്ടി (DfgVertex& vtx : dfg.opVertices()) {
        DfgLogic* സ്ഥിരം logicp = vtx.cast<DfgLogic>();
        എങ്കിൽ (!logicp || worklist.contains(*logicp)) തുടരുക;
        // If drives an unused variable, synthesize it so the partial logic can be removed
        എങ്കിൽ (logicp->drivesUnusedVars()) {
            worklist.തള്ളുക_front(*logicp);
            തുടരുക;
        }
        // Blocks corresponding to continuous assignments
        എങ്കിൽ (logicp->nodep()->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            worklist.തള്ളുക_front(*logicp);
            തുടരുക;
        }
        സ്ഥിരം CfgGraph& cfg = logicp->cfg();
        // Straight line code with no branches
        എങ്കിൽ (cfg.nBlocks() == 1) {
            worklist.തള്ളുക_front(*logicp);
            തുടരുക;
        }
        // Blocks driving exactly 1 variable
        എങ്കിൽ (!logicp->hasMultipleSinks()) worklist.തള്ളുക_front(*logicp);
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
            എങ്കിൽ (!isVisited.തിരുകുക(unresolved).second) തിരികെ അസത്യം;
            unresolved->foreachSource([&](DfgVertex& sibling) {
                DfgLogic& siblingLogic = *sibling.as<DfgLogic>();
                എങ്കിൽ (!siblingLogic.selectedForSynthesis()) worklist.തള്ളുക_front(siblingLogic);
                തിരികെ അസത്യം;
            });
            തിരികെ അസത്യം;
        });
    });
}

ശൂന്യം V3DfgPasses::synthesize(DfgGraph& dfg, V3DfgContext& ctx) {
    // Select which DfgLogic to attempt to synthesize
    dfgSelectLogicForSynthesis(dfg);
    // Synthesize them - also removes un-synthesized DfgLogic, so must run even if nothing selected
    AstToDfgSynthesize::apply(dfg, ctx.m_synthContext);
}
