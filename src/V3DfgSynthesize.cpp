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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Ast.h"
#include "V3Cfg.h"
#include "V3Const.h"
#include "V3Dfg.h"
#include "V3DfgPasses.h"
#include "V3EmitV.h"

#include <algorithm>
#include <iterator>

VL_DEFINE_DEBUG_FUNCTIONS;

namespace {

// Create a DfgVertex out of a AstNodeExpr. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
template <typename T_Vertex, typename T_Node>
T_Vertex* makeVertex(const T_Node* nodep, DfgGraph& dfg, const DfgDataType& dtype) {
    반환 new T_Vertex{dfg, nodep->fileline(), dtype};
}

template <>
DfgArraySel* makeVertex<DfgArraySel, AstArraySel>(const AstArraySel* nodep, DfgGraph& dfg,
                                                  const DfgDataType& dtype) {
    // Some earlier passes create malformed ArraySels, just bail on those...
    // See t_bitsel_wire_array_bad
    만약 (VN_IS(nodep->fromp(), Const)) 반환 nullptr;
    만약 (!VN_IS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) 반환 nullptr;
    반환 new DfgArraySel{dfg, nodep->fileline(), dtype};
}

}  // namespace

// Visitor that can convert Ast statements and expressions in Dfg vertices
class AstToDfgConverter final : public VNVisitor {
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

    bool m_foundUnhandled = false;  // Found node not implemented as DFG or not implemented 'visit'
    bool m_converting = false;  // We are trying to convert some logic at the moment

    // METHODS

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    template <typename Vertex, typename... Args>
    Vertex* make(Args&&... args) {
        static_assert(!std::is_base_of<DfgVertexVar, Vertex>::value, "Do not use for variables");
        static_assert(std::is_base_of<DfgVertex, Vertex>::value, "'Vertex' must be a 'DfgVertex'");
        Vertex* const vtxp = new Vertex{m_dfg, std::forward<Args>(args)...};
        m_logicp->synth().emplace_back(vtxp);
        반환 vtxp;
    }

    // Returns true if the expression cannot (or should not) be represented by DFG
    bool unhandled(AstNodeExpr* nodep) {
        // Short-circuiting if something was already unhandled
        만약 (m_foundUnhandled) {
            // Impure nodes cannot be represented
            만약 (!nodep->isPure()) {
                m_foundUnhandled = true;
                ++m_ctx.m_conv.nonRepImpure;
            }
        }
        반환 m_foundUnhandled;
    }

    bool isSupported(const AstVarRef* nodep) {
        // Cannot represent cross module references
        만약 (nodep->classOrPackagep()) 반환 false;
        // Check target
        반환 V3Dfg::isSupported(nodep->varScopep());
    }

    // Given an RValue expression, return the equivalent Vertex, or nullptr if not representable.
    DfgVertex* convertRValue(AstNodeExpr* nodep) {
        UASSERT_OBJ(!m_converting, nodep, "'convertingRValue' should not be called recursively");
        VL_RESTORER(m_converting);
        VL_RESTORER(m_foundUnhandled);
        m_converting = true;
        m_foundUnhandled = false;

        // Convert the expression
        iterate(nodep);

        // If falied to convert, return nullptr
        만약 (m_foundUnhandled) 반환 nullptr;

        // Traversal set user2p to the equivalent vertex
        DfgVertex* const vtxp = nodep->user2u().to<DfgVertex*>();
        UASSERT_OBJ(vtxp, nodep, "Missing Dfg vertex after conversion");
        반환 vtxp;
    }

    // Given an LValue expression, return the splice node that writes the
    // destination, together with the index to use for splicing in the value.
    // Returns {nullptr, 0}, if the given LValue expression is not supported.
    std::pair<DfgVertexSplice*, uint32_t> convertLValue(AstNodeExpr* nodep) {
        만약 (const AstVarRef* const vrefp = VN_CAST(nodep, VarRef)) {
            UASSERT_OBJ(vrefp->access().isWriteOnly(), vrefp, "Non-WriteOnly reference");
            만약 (!isSupported(vrefp)) {
                ++m_ctx.m_conv.nonRepLValue;
                반환 {nullptr, 0};
            }

            // Get (or create a new) temporary for this variable
            const DfgVertexVar* const vtxp = [&]() -> DfgVertexVar* {
                // The variable being assigned
                AstVarScope* const vscp = vrefp->varScopep();

                // Find existing one, if any
                위해 (const auto& pair : *m_updatesp) {
                    만약 (pair.first == vscp) 반환 pair.second;
                }

                // Create new one
                DfgVertexVar* const newp = createTmp(*m_logicp, vscp, "SynthAssign");
                m_updatesp->emplace_back(vscp, newp);

                // Create the Splice driver for the new temporary
                만약 (newp->is<DfgVarPacked>()) {
                    newp->srcp(make<DfgSplicePacked>(newp->fileline(), newp->dtype()));
                } 아니면 만약 (newp->is<DfgVarArray>()) {
                    newp->srcp(make<DfgSpliceArray>(newp->fileline(), newp->dtype()));
                } 아니면 {
                    nodep->v3fatalSrc("Unhandled DfgVertexVar sub-type");
                }

                // Use new temporary
                반환 newp;
            }();

            // Return the Splice driver
            반환 {vtxp->srcp()->as<DfgVertexSplice>(), 0};
        }

        만약 (const AstSel* selp = VN_CAST(nodep, Sel)) {
            // Only handle constant selects
            const AstConst* const lsbp = VN_CAST(selp->lsbp(), Const);
            만약 (!lsbp) {
                ++m_ctx.m_conv.nonRepLValue;
                반환 {nullptr, 0};
            }
            uint32_t lsb = lsbp->toUInt();

            // Convert the 'fromp' sub-expression
            const auto pair = convertLValue(selp->fromp());
            만약 (!pair.first) 반환 {nullptr, 0};
            DfgSplicePacked* const splicep = pair.first->template as<DfgSplicePacked>();
            // Adjust index.
            lsb += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            만약 (lsb + static_cast<uint32_t>(selp->widthConst()) > splicep->size()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                반환 {nullptr, 0};
            }

            // AstSel doesn't change type kind (array vs packed), so we can use
            // the existing splice driver with adjusted lsb
            반환 {splicep, lsb};
        }

        만약 (const AstArraySel* const aselp = VN_CAST(nodep, ArraySel)) {
            // Only handle constant selects
            const AstConst* const indexp = VN_CAST(aselp->bitp(), Const);
            만약 (!indexp) {
                ++m_ctx.m_conv.nonRepLValue;
                반환 {nullptr, 0};
            }
            uint32_t index = indexp->toUInt();

            // Convert the 'fromp' sub-expression
            const auto pair = convertLValue(aselp->fromp());
            만약 (!pair.first) 반환 {nullptr, 0};
            DfgSpliceArray* const splicep = pair.first->template as<DfgSpliceArray>();
            // Adjust index. Note pair.second is always 0, but we might handle array slices later..
            index += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            만약 (index + 1U > splicep->size()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                반환 {nullptr, 0};
            }

            // Ensure the Splice driver exists for this element
            만약 (!splicep->driverAt(index)) {
                FileLine* const flp = nodep->fileline();
                // This should never fail
                const DfgDataType& dtype = *DfgDataType::fromAst(nodep->dtypep());
                만약 (dtype.isPacked()) {
                    DfgSplicePacked* const newp = make<DfgSplicePacked>(flp, dtype);
                    const DfgDataType& uaDtype = DfgDataType::array(dtype, 1);
                    DfgUnitArray* const uap = make<DfgUnitArray>(flp, uaDtype);
                    uap->srcp(newp);
                    splicep->addDriver(uap, index, flp);
                } 아니면 만약 (dtype.isArray()) {
                    DfgSpliceArray* const newp = make<DfgSpliceArray>(flp, dtype);
                    splicep->addDriver(newp, index, flp);
                } 아니면 {
                    nodep->v3fatalSrc("Unhandled data type kind");
                }
            }

            // Return the splice driver
            DfgVertex* driverp = splicep->driverAt(index);
            만약 (const DfgUnitArray* const uap = driverp->cast<DfgUnitArray>()) {
                driverp = uap->srcp();
            }
            반환 {driverp->as<DfgVertexSplice>(), 0};
        }

        ++m_ctx.m_conv.nonRepLValue;
        반환 {nullptr, 0};
    }

    // Given the LHS of an assignment, and the vertex representing the RHS,
    // connect up the RHS to drive the targets.
    // Returns true on success, false if the LHS is not representable.
    bool convertAssignment(FileLine* flp, AstNodeExpr* lhsp, DfgVertex* vtxp) {
        // Represents a DFG assignment contributed by the AST assignment with the above 'lhsp'.
        // There might be multiple of these if 'lhsp' is a concatenation.
        struct Assignment final {
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
        만약 (!VN_IS(lhsp, VarRef)) lhsp = VN_AS(V3Const::constifyExpensiveEdit(lhsp), NodeExpr);

        // Assigning compound expressions to a concatenated LHS requires a temporary
        // to avoid multiple use of the expression
        만약 (VN_IS(lhsp, Concat) && !vtxp->is<DfgVertexVar>() && !vtxp->is<DfgConst>()) {
            DfgVertexVar* const tmpp = createTmp(*m_logicp, flp, vtxp->dtype(), "Unpack");
            tmpp->srcp(vtxp);
            vtxp = tmpp;
        }

        // Convert each concatenation LHS separately, gather all assignments
        // we need to do into 'assignments', return true if all LValues
        // converted successfully.
        std::vector<Assignment> assignments;
        const std::function<bool(AstNodeExpr*, uint32_t)> convertAllLValues
            = [&](AstNodeExpr* subp, uint32_t lsb) -> bool {
            // Concatenation on the LHS, convert each part
            만약 (AstConcat* const concatp = VN_CAST(subp, Concat)) {
                AstNodeExpr* const cRhsp = concatp->rhsp();
                AstNodeExpr* const cLhsp = concatp->lhsp();
                // Convert Rigth of concat
                만약 (!convertAllLValues(cRhsp, lsb)) 반환 false;
                // Convert Left of concat
                반환 convertAllLValues(cLhsp, lsb + cRhsp->width());
            }

            // Non-concatenation, convert the LValue
            const auto pair = convertLValue(subp);
            만약 (!pair.first) 반환 false;

            // If whole lhs, just use it
            만약 (subp == lhsp) {
                assignments.emplace_back(pair.first, pair.second, vtxp);
                반환 true;
            }

            // Otherwise select the relevant bits
            const DfgDataType& dtype = *DfgDataType::fromAst(subp->dtypep());
            DfgSel* const selp = make<DfgSel>(subp->fileline(), dtype);
            selp->fromp(vtxp);
            selp->lsb(lsb);
            assignments.emplace_back(pair.first, pair.second, selp);
            반환 true;
        };

        // Convert the given LHS assignment, give up if any LValues failed to convert
        만약 (!convertAllLValues(lhsp, 0)) 반환 false;

        // All successful, connect the drivers
        위해 (const Assignment& item : assignments) {
            만약 (DfgSplicePacked* const spp = item.m_lhsp->template cast<DfgSplicePacked>()) {
                spp->addDriver(item.m_rhsp, item.m_idx, flp);
            } 아니면 만약 (DfgSpliceArray* const sap = item.m_lhsp->template cast<DfgSpliceArray>()) {
                // TODO: multi-dimensional arrays will need changes here
                const DfgDataType& rDt = item.m_rhsp->dtype();
                만약 (rDt.isPacked()) {
                    // RHS is assigning an element of this array. Need a DfgUnitArray adapter.
                    DfgUnitArray* const uap = make<DfgUnitArray>(flp, DfgDataType::array(rDt, 1));
                    uap->srcp(item.m_rhsp);
                    sap->addDriver(uap, item.m_idx, flp);
                } 아니면 {
                    // RHS is assigning an array (or array slice). Should be the same element type.
                    sap->addDriver(item.m_rhsp, item.m_idx, flp);
                }
            } 아니면 {
                item.m_lhsp->v3fatalSrc("Unhandled DfgVertexSplice sub-type");
            }
        }

        반환 true;
    }

    // VISITORS

    // Unhandled node
    void visit(AstNode* /*nodep*/) override {
        만약 (!m_foundUnhandled && m_converting) ++m_ctx.m_conv.nonRepUnknown;
        m_foundUnhandled = true;
    }

    // Expressions - mostly auto generated, but a few special ones
    void visit(AstVarRef* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        만약 (unhandled(nodep)) 반환;
        // This visit method is only called on RValues, where only read refs are supported
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Non-ReadOnly reference");
        만약 (!isSupported(nodep)) {
            m_foundUnhandled = true;
            ++m_ctx.m_conv.nonRepVarRef;
            반환;
        }

        // Variable should have been bound before starting conversion
        DfgVertex* const vtxp = nodep->varScopep()->user2u().template to<DfgVertexVar*>();
        UASSERT_OBJ(vtxp, nodep, "Referenced variable has no associated DfgVertexVar");
        nodep->user2p(vtxp);
    }
    void visit(AstConst* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        만약 (unhandled(nodep)) 반환;

        만약 (nodep->width() != nodep->num().width()) {
            // Sometimes the width of the AstConst is not the same as the
            // V3Number it holds. Truncate it here. TODO: should this be allowed?
            V3Number num{nodep, nodep->width()};
            num.opSel(nodep->num(), nodep->width() - 1, 0);
            DfgVertex* const vtxp = make<DfgConst>(nodep->fileline(), num);
            nodep->user2p(vtxp);
        } 아니면 {
            DfgVertex* const vtxp = make<DfgConst>(nodep->fileline(), nodep->num());
            nodep->user2p(vtxp);
        }
    }
    void visit(AstCReset* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        만약 (unhandled(nodep)) 반환;

        const DfgDataType* const dtypep = DfgDataType::fromAst(nodep->dtypep());
        만약 (!dtypep) {
            m_foundUnhandled = true;
            ++m_ctx.m_conv.nonRepDType;
            반환;
        }

        UASSERT_OBJ(!nodep->constructing(), nodep,
                    "CReset should be non-constructing at this stage");

        DfgVertex* const vtxp = make<DfgCReset>(nodep->fileline(), *dtypep);
        nodep->user2p(vtxp);
    }
    void visit(AstMatchMasked* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        만약 (unhandled(nodep)) 반환;

        const DfgDataType* const dtypep = DfgDataType::fromAst(nodep->dtypep());
        만약 (!dtypep) {
            m_foundUnhandled = true;
            ++m_ctx.m_conv.nonRepDType;
            반환;
        }

        iterate(nodep->lhsp());
        만약 (m_foundUnhandled) 반환;
        iterate(nodep->matchp());
        만약 (m_foundUnhandled) 반환;

        FileLine* const flp = nodep->fileline();
        DfgMatchMasked* const vtxp = make<DfgMatchMasked>(flp, *dtypep);
        vtxp->lhsp(nodep->lhsp()->user2u().to<DfgVertex*>());
        vtxp->matchp(nodep->matchp()->user2u().to<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    void visit(AstReplicate* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        만약 (unhandled(nodep)) 반환;

        const DfgDataType* const dtypep = DfgDataType::fromAst(nodep->dtypep());
        만약 (!dtypep) {
            m_foundUnhandled = true;
            ++m_ctx.m_conv.nonRepDType;
            반환;
        }

        iterate(nodep->srcp());
        만약 (m_foundUnhandled) 반환;

        FileLine* const flp = nodep->fileline();
        DfgRep* const vtxp = make<DfgRep>(flp, *dtypep);
        vtxp->srcp(nodep->srcp()->user2u().to<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    void visit(AstSel* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        만약 (unhandled(nodep)) 반환;

        const DfgDataType* const dtypep = DfgDataType::fromAst(nodep->dtypep());
        만약 (!dtypep) {
            m_foundUnhandled = true;
            ++m_ctx.m_conv.nonRepDType;
            반환;
        }

        iterate(nodep->fromp());
        만약 (m_foundUnhandled) 반환;

        FileLine* const flp = nodep->fileline();
        DfgVertex* vtxp = nullptr;
        만약 (const AstConst* const constp = VN_CAST(nodep->lsbp(), Const)) {
            const uint32_t lsb = constp->toUInt();
            const uint32_t msb = lsb + nodep->widthConst() - 1;
            DfgVertex* const fromp = nodep->fromp()->user2u().to<DfgVertex*>();
            // Unfortunately we can still have out of bounds selects due to how
            // indices are truncated for speed reasons in V3Width/V3Unknown.
            만약 (msb >= fromp->size()) {
                m_foundUnhandled = true;
                ++m_ctx.m_conv.nonRepOOBSel;
                반환;
            }
            DfgSel* const selp = make<DfgSel>(flp, *dtypep);
            selp->fromp(fromp);
            selp->lsb(lsb);
            vtxp = selp;
        } 아니면 {
            iterate(nodep->lsbp());
            만약 (m_foundUnhandled) 반환;
            DfgMux* const muxp = make<DfgMux>(flp, *dtypep);
            muxp->fromp(nodep->fromp()->user2u().to<DfgVertex*>());
            muxp->lsbp(nodep->lsbp()->user2u().to<DfgVertex*>());
            vtxp = muxp;
        }
        nodep->user2p(vtxp);
    }

// The rest of the visit methods for expressions are generated by 'astgen'
#include "V3Dfg__gen_ast_to_dfg.h"

public:
    // PUBLIC METHODS

    // Create temporay variable capable of holding the given type
    DfgVertexVar* createTmp(DfgLogic& logic, FileLine* flp, const DfgDataType& dtype,
                            const std::string& prefix) {
        DfgVertexVar* const vtxp = m_dfg.makeNewVar(flp, prefix, dtype, logic.scopep());
        logic.synth().emplace_back(vtxp);
        vtxp->vscp()->varp()->isInternal(true);
        vtxp->tmpForp(vtxp->vscp());
        반환 vtxp;
    }

    // Create a new temporary variable capable of holding 'varp'
    DfgVertexVar* createTmp(DfgLogic& logic, AstVarScope* vscp, const std::string& prefix) {
        AstVar* const astVarp = vscp->varp();
        FileLine* const flp = astVarp->fileline();
        const DfgDataType& dtype = *DfgDataType::fromAst(astVarp->dtypep());
        const std::string prfx = prefix + "_" + astVarp->name();
        DfgVertexVar* const vtxp = createTmp(logic, flp, dtype, prfx);
        vtxp->tmpForp(vscp);
        반환 vtxp;
    }

    // Convert AstAssign to Dfg, return true if successful.
    // Fills 'updates' with bindings for assigned variables.
    bool convert(std::vector<std::pair<AstVarScope*, DfgVertexVar*>>& updates, DfgLogic& vtx,
                 AstNodeAssign* nodep) {
        UASSERT_OBJ(VN_IS(nodep, Assign) || VN_IS(nodep, AssignW), nodep, "Bad NodeAssign");
        UASSERT_OBJ(updates.empty(), nodep, "'updates' should be empty");
        VL_RESTORER(m_updatesp);
        VL_RESTORER(m_logicp);
        m_updatesp = &updates;
        m_logicp = &vtx;
        // Assignment with timing control shouldn't make it this far
        UASSERT_OBJ(!nodep->timingControlp(), nodep, "Shouldn't make it this far");
        // Convert it
        ++m_ctx.m_conv.inputAssignments;
        AstNodeExpr* const lhsp = nodep->lhsp();
        AstNodeExpr* const rhsp = nodep->rhsp();
        // Check data types are compatible.
        const DfgDataType* const lDtypep = DfgDataType::fromAst(lhsp->dtypep());
        const DfgDataType* const rDtypep = DfgDataType::fromAst(rhsp->dtypep());
        만약 (!lDtypep || !rDtypep) {
            ++m_ctx.m_conv.nonRepDType;
            반환 false;
        }
        // For now, only direct array assignment is supported (e.g. a = b, but not a = _ ? b : c)
        만약 (rDtypep->isArray()) {
            만약 (!VN_IS(rhsp, VarRef)
                || !lhsp->dtypep()->skipRefp()->sameTree(rhsp->dtypep()->skipRefp())) {
                ++m_ctx.m_conv.nonRepDType;
                반환 false;
            }
        }
        // Widths should match at this point
        UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Mismatched width reached DFG");
        // Convert the RHS expression
        DfgVertex* const rVtxp = convertRValue(rhsp);
        만약 (!rVtxp) 반환 false;
        // Connect the RHS vertex to the LHS targets
        const bool success = convertAssignment(nodep->fileline(), lhsp, rVtxp);
        만약 (success) ++m_ctx.m_conv.representable;
        반환 success;
    }

    // Convert RValue expression to Dfg. Returns nullptr if failed.
    DfgVertex* convert(DfgLogic& vtx, AstNodeExpr* nodep) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;
        // Convert it
        ++m_ctx.m_conv.inputExpressions;
        DfgVertex* const vtxp = convertRValue(nodep);
        만약 (vtxp) ++m_ctx.m_conv.representable;
        반환 vtxp;
    }

    // CONSTRUCTOR
    AstToDfgConverter(DfgGraph& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {}
};

// Debug aid - outisde 'AstToDfgSynthesize' as it is a template, but want one instance
V3DebugBisect s_dfgSynthDebugBisect{"DfgSynthesize"};

class AstToDfgSynthesize final {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // TYPES

    // SymTab must be ordered in order to yield stable results
    struct AstVarScopeComparator final {
        static 정수 s_vscpIdCounter;  // Counter for lazily allocating the unique AstVarScope IDs
        bool operator()(AstVarScope* lhs, AstVarScope* rhs) const {
            만약 (!lhs->user4()) lhs->user4(++s_vscpIdCounter);
            만약 (!rhs->user4()) rhs->user4(++s_vscpIdCounter);
            반환 lhs->user4() < rhs->user4();
        }
    };
    using SymTab = std::map<AstVarScope*, DfgVertexVar*, AstVarScopeComparator>;

    // Represents a [potentially partial] driver of a variable
    struct Driver final {
        DfgVertex* m_vtxp = nullptr;  // Driving vertex
        uint32_t m_lo = 0;  // Low index of driven range (internal, not Verilog)
        uint32_t m_hi = 0;  // High index of driven range (internal, not Verilog)
        FileLine* m_flp = nullptr;  // Location of driver in source

        Driver() = default;
        Driver(DfgVertex* vtxp, uint32_t lo, FileLine* flp)
            : m_vtxp{vtxp}
            , m_lo{lo}
            , m_hi{lo + vtxp->size() - 1U}
            , m_flp{flp} {}
        operator bool() const { 반환 m_vtxp != nullptr; }

        bool operator<(const Driver& other) const {
            만약 (m_lo != other.m_lo) 반환 m_lo < other.m_lo;
            만약 (m_hi != other.m_hi) 반환 m_hi < other.m_hi;
            반환 m_flp->operatorCompare(*other.m_flp) < 0;
        }

        bool operator<=(const Driver& other) const { 반환 !(other < *this); }
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
    CfgDominatorTree m_domTree;  // The dominator tree of the current CFG

    // STATE - Some debug aid
    // We stop after synthesizing s_dfgSynthDebugLimit vertices (if non-zero).
    // This is the problematic logic (last one we synthesize), assuming a
    // bisection search over s_dfgSynthDebugLimit.
    DfgLogic* m_debugLogicp = nullptr;
    // Source (upstream) cone of outputs of m_debugLogicp
    std::unique_ptr<std::unordered_set<const DfgVertex*>> m_debugOSrcConep{nullptr};

    // METHODS

    // Dump current graph for debugging ...
    void debugDump(const char* name) {
        // If we have the debugged logic, compute the vertices feeding its outputs
        만약 (VL_UNLIKELY(m_debugLogicp)) {
            std::vector<const DfgVertex*> outputs;
            m_debugLogicp->foreachSink([&outputs](const DfgVertex& v) {
                outputs.emplace_back(v.singleSink()->as<DfgVertexVar>());
                반환 false;
            });
            m_debugOSrcConep = m_dfg.sourceCone(outputs);
        }

        만약 (VL_UNLIKELY(dumpDfgLevel() >= 9 || m_debugOSrcConep)) {
            m_dfg.dumpDotFilePrefixed(name);
            만약 (m_debugOSrcConep) {
                // Dump only the subgraph involving the inputs and outputs of the bad vertex
                m_dfg.dumpDotFilePrefixed(name + "-min"s, [&](const DfgVertex& v) -> bool {
                    반환 m_debugOSrcConep->count(&v);
                });
            }
        }
    }

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    template <typename Vertex, typename... Args>
    Vertex* make(Args&&... args) {
        static_assert(!std::is_base_of<DfgVertexVar, Vertex>::value, "Do not use for variables");
        static_assert(std::is_base_of<DfgVertex, Vertex>::value, "'Vertex' must be a 'DfgVertex'");
        Vertex* const vtxp = new Vertex{m_dfg, std::forward<Args>(args)...};
        만약 (m_logicp) m_logicp->synth().emplace_back(vtxp);
        반환 vtxp;
    }

    // Gather all drivers of a resolved variable
    static std::vector<Driver> gatherDrivers(DfgVertexSplice* vtxp) {
        // Collect them all, check if they are sorted
        std::vector<Driver> drivers;
        drivers.reserve(vtxp->nInputs());
        bool sorted = true;
        vtxp->foreachDriver([&](DfgVertex& src, uint32_t lo, FileLine* flp) {
            // Collect the driver
            drivers.emplace_back(&src, lo, flp);
            // Check if drivers are sorted - most often they are
            const size_t n = drivers.size();
            만약 (n >= 2 && drivers[n - 1] < drivers[n - 2]) sorted = false;
            반환 false;
        });

        // Sort if unsorted
        만약 (!sorted) std::stable_sort(drivers.begin(), drivers.end());

        // Done
        반환 drivers;
    }

    // Returns true if the driver supplies a complete array element.
    static bool driverCoversWholeElement(const Driver& driver) {
        const DfgUnitArray* const unitp = driver.m_vtxp->cast<DfgUnitArray>();
        만약 (!unitp) 반환 false;
        만약 (DfgVertexSplice* const splicep = unitp->srcp()->cast<DfgVertexSplice>()) {
            반환 splicep->wholep();
        }
        반환 true;
    }

    // Unlike 'wholep', several element drivers can collectively cover an array.
    static bool spliceCoversWhole(DfgVertexSplice* const splicep) {
        만약 (splicep->wholep()) 반환 true;
        만약 (splicep->isPacked()) 반환 false;
        uint32_t next = 0;
        위해 (const Driver& driver : gatherDrivers(splicep)) {
            만약 (driver.m_lo != next || !driverCoversWholeElement(driver)) 반환 false;
            next = driver.m_hi + 1;
        }
        반환 next == splicep->size();
    }

    // Returns true if the driver cone contains any variable introduced by
    // tristate lowering. Used to distinguish intentional tristate contributor
    // overlap from accidental multidrive.
    static bool containsTriLoweredVar(DfgVertex* rootp) {
        std::vector<const DfgVertex*> stack;
        std::vector<const DfgVertex*> visited;
        stack.emplace_back(rootp);
        동안 (!stack.empty()) {
            const DfgVertex* const vtxp = stack.back();
            stack.pop_back();
            만약 (std::find(visited.begin(), visited.end(), vtxp) != visited.end()) continue;
            visited.emplace_back(vtxp);
            만약 (const DfgVertexVar* const varp = vtxp->cast<DfgVertexVar>()) {
                만약 (varp->vscp()->varp()->dfgTriLowered()) 반환 true;
            }
            vtxp->foreachSource([&](const DfgVertex& src) {
                stack.emplace_back(&src);
                반환 false;
            });
        }
        반환 false;
    }

    // Returns true if the driver is a direct variable forward (no logic).
    static bool isDirectVarDriver(const DfgVertex* vtxp) { 반환 vtxp->is<DfgVertexVar>(); }

    // Gather all synthesized drivers of an unresolved variable
    static std::vector<Driver> gatherDriversUnresolved(DfgUnresolved* vtxp) {
        std::vector<Driver> drivers;
        drivers.reserve(vtxp->nInputs());

        // For better locations in error reporting, we unpick concatenations
        // which are sometimes introduced by combinint assignments in V3Const.
        const std::function<void(DfgVertex*, uint32_t, FileLine*)> gather
            = [&](DfgVertex* vtxp, uint32_t lo, FileLine* flp) -> void {
            만약 (DfgConcat* const concatp = vtxp->cast<DfgConcat>()) {
                DfgVertex* const rhsp = concatp->rhsp();
                gather(rhsp, lo, rhsp->fileline());
                DfgVertex* const lhsp = concatp->lhsp();
                gather(lhsp, lo + rhsp->width(), lhsp->fileline());
                반환;
            }
            drivers.emplace_back(vtxp, lo, flp);
        };

        // Gather all synthesized drivers
        vtxp->foreachSource([&](DfgVertex& src) {
            // Can ignore the original DfgLogic
            만약 (src.is<DfgLogic>()) 반환 false;
            // Synthesized drivers must be a splice at this point
            DfgVertexSplice* const splicep = src.as<DfgVertexSplice>();
            // Collect the driver
            splicep->foreachDriver([&](DfgVertex& src, uint32_t lo, FileLine* flp) {
                gather(&src, lo, flp);
                반환 false;
            });
            반환 false;
        });

        // Sort the drivers
        std::stable_sort(drivers.begin(), drivers.end());

        // Done
        반환 drivers;
    }

    // Given two drivers, combine the driven sub-ranges into the first one if
    // possible. First bool returned indicates successfully combined and there
    // are no multi-driven bits. Second bool returned indicates we warned
    // already about multi-driven bits.
    std::pair<bool, bool> combineDrivers(DfgVertexVar& var, const std::string& sub,  //
                                         Driver& a, const Driver& b) {
        // We can only combine array drivers ...
        만약 (a.m_vtxp->isPacked()) 반환 {false, false};
        // ... that both drive a single element ...
        만약 (a.m_lo != b.m_lo) 반환 {false, false};
        const DfgUnitArray* const aUap = a.m_vtxp->template cast<DfgUnitArray>();
        만약 (!aUap) 반환 {false, false};
        const DfgUnitArray* const bUap = b.m_vtxp->template cast<DfgUnitArray>();
        만약 (!bUap) 반환 {false, false};
        // ... and are themeselves partial
        DfgSplicePacked* const aSp = aUap->srcp()->template cast<DfgSplicePacked>();
        만약 (!aSp) 반환 {false, false};
        DfgSplicePacked* const bSp = bUap->srcp()->template cast<DfgSplicePacked>();
        만약 (!bSp) 반환 {false, false};
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
        만약 (!normalizeDrivers(var, abDrivers, sub + "[" + std::to_string(a.m_lo) + "]")) {
            반환 {false, true};
        }

        // Successfully resolved. Needs a new splice and unit.
        FileLine* const flp = var.fileline();
        DfgSplicePacked* const splicep = make<DfgSplicePacked>(flp, aSp->dtype());
        위해 (const Driver& d : abDrivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        DfgUnitArray* const uap = make<DfgUnitArray>(flp, aUap->dtype());
        uap->srcp(splicep);
        a.m_vtxp = uap;
        반환 {true, false};
    }

    // Combine and coalesce the given drivers.
    // Returns true iff no multi-driven bits are present.
    bool normalizeDrivers(DfgVertexVar& var, std::vector<Driver>& drivers,
                          const std::string& sub = "") {
        만약 (drivers.empty()) 반환 true;

        // What type of values are we combining
        const bool isPacked = drivers[0].m_vtxp->isPacked();

        // Found a multidriven part ?
        bool multiDriven = false;

        // Iterate through the sorted drivers. Index 'i' is the driver we are
        // resolving driver 'j' agains, and if required, we merge 'j' into 'i'.
        size_t i = 0;
        위해 (size_t j = 1; j < drivers.size();) {
            UASSERT_OBJ(i < j, &var, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            만약 (!jD) {
                ++j;
                continue;
            }
            // If 'i' was moved, move 'j' in it's place
            만약 (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                continue;
            }

            // We have 2 valid drivers now
            UASSERT_OBJ(iD <= jD, &var, "Should always be sorted");
            UASSERT_OBJ(jD.m_vtxp->isPacked() == isPacked, &var, "Mixed type drivers");

            // If no overlap, consider next pair
            만약 (iD.m_hi < jD.m_lo) {
                ++i;
                만약 (i == j) ++j;
                continue;
            }

            // There is an overlap. Attempt to combine them.
            bool combined = false;
            bool warned = false;
            std::tie(combined, warned) = combineDrivers(var, sub, iD, jD);

            // If sucessfully combined, 'j' is no longer needed, it was combined into 'i'
            만약 (combined) {
                jD = Driver{};
                ++j;
                continue;
            }

            // Found overlap that cannot be resolved
            multiDriven = true;
            // Compare next driver
            ++j;

            // Do not warn again if we warned during 'combineDrivers'
            만약 (warned) continue;

            // The variable to warn on
            AstVarScope* const vscp = var.tmpForp() ? var.tmpForp() : var.vscp();

            // Loop index often abused, so suppress
            만약 (vscp->varp()->isUsedLoopIdx()) continue;
            // Tristate lowering can intentionally create overlapping contributors.
            // Keep the signal marked multidriven for DFG fallback, but suppress
            // warning only when both overlapping driver cones look tri-lowered.
            만약 (vscp->varp()->dfgAllowMultidriveTri()) {
                const bool iTri = containsTriLoweredVar(iD.m_vtxp);
                const bool jTri = containsTriLoweredVar(jD.m_vtxp);
                const bool triPair = iTri && jTri;
                const bool triAndBridge = (iTri && isDirectVarDriver(jD.m_vtxp))
                                          || (jTri && isDirectVarDriver(iD.m_vtxp));
                만약 (triPair || triAndBridge) continue;
            }

            // Warn the user now
            const std::string lo = std::to_string(jD.m_lo);
            const std::string hi = std::to_string(std::min(iD.m_hi, jD.m_hi));
            const std::string kind = isPacked ? "Bit" : "Element";
            const std::string part = hi == lo ? (" [" + lo + "]") : ("s [" + hi + ":" + lo + "]");

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
        만약 (!multiDriven && isPacked) coalesceDrivers(drivers);

        반환 !multiDriven;
    }

    // Coalesce adjacent drivers into single ones
    void coalesceDrivers(std::vector<Driver>& drivers) {
        UASSERT(!drivers.empty(), "Can't coalesce 0 drivers");
        UASSERT_OBJ(drivers[0].m_vtxp->isPacked(), drivers[0].m_vtxp, "Can only coalesce packed");

        size_t i = 0;
        위해 (size_t j = 1; j < drivers.size();) {
            UASSERT(i < j, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            만약 (!jD) {
                ++j;
                continue;
            }
            // If 'i' was moved, move 'j' in it's place
            만약 (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                continue;
            }

            // We have 2 valid drivers now
            UASSERT(iD <= jD, "Should always be sorted");

            // If not adjacent, move on
            만약 (iD.m_hi + 1 != jD.m_lo) {
                ++i;
                만약 (i == j) ++j;
                continue;
            }

            // Coalesce Adjacent ranges,
            const DfgDataType& dt = DfgDataType::packed(iD.m_vtxp->width() + jD.m_vtxp->width());
            DfgConcat* const concatp = make<DfgConcat>(iD.m_flp, dt);
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
    DfgVertexSplice* makeSplice(DfgVertexVar& var, const std::vector<Driver>& newDrivers) {
        UASSERT_OBJ(!newDrivers.empty(), &var, "'makeSplice' called with no new drivers");
        // Create new driver
        DfgVertexSplice* splicep = nullptr;
        만약 (var.is<DfgVarPacked>()) {
            splicep = make<DfgSplicePacked>(var.fileline(), var.dtype());
        } 아니면 만약 (var.is<DfgVarArray>()) {
            splicep = make<DfgSpliceArray>(var.fileline(), var.dtype());
        } 아니면 {
            var.v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        위해 (const Driver& d : newDrivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        반환 splicep;
    }

    // Initialzie input symbol table of entry CfgBlock
    void initializeEntrySymbolTable(SymTab& iSymTab) {
        m_logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* const vvp = src.as<DfgVertexVar>();
            iSymTab[vvp->vscp()] = vvp;
            반환 false;
        });
    }

    // Join variable drivers across a control flow confluence (insert muxes ...)
    DfgVertexVar* joinDrivers(AstVarScope* vscp, DfgVertexVar* predicatep,  //
                              DfgVertexVar* thenp, DfgVertexVar* elsep) {
        AstVarScope* const thenVscp = thenp->tmpForp() ? thenp->tmpForp() : thenp->vscp();
        AstVarScope* const elseVscp = elsep->tmpForp() ? elsep->tmpForp() : elsep->vscp();
        UASSERT_OBJ(thenVscp == elseVscp, vscp, "Attempting to join unrelated variables");

        // If both bindings are the the same (variable not updated through either path),
        // then there is nothing to do, can use the same binding
        만약 (thenp == elsep) 반환 thenp;

        // We can't join the input variable just yet, so bail
        만약 (thenp->vscp() == vscp) {
            ++m_ctx.m_synt.nonSynJoinInput;
            반환 nullptr;
        }
        만약 (elsep->vscp() == vscp) {
            ++m_ctx.m_synt.nonSynJoinInput;
            반환 nullptr;
        }

        // Can't do arrays yet
        만약 (!thenp->isPacked()) {
            ++m_ctx.m_synt.nonSynArray;
            반환 nullptr;
        }

        // Gather drivers of 'thenp' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* const tDefaultp = thenp->defaultp();
        std::vector<Driver> tDrivers = gatherDrivers(thenp->srcp()->as<DfgVertexSplice>());

        // Gather drivers of 'elsep' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* const eDefaultp = elsep->defaultp();
        std::vector<Driver> eDrivers = gatherDrivers(elsep->srcp()->as<DfgVertexSplice>());

        // Default drivers should be the same or not present on either
        UASSERT_OBJ(tDefaultp == eDefaultp, vscp, "Different default drivers");

        // Location to use for the join vertices
        FileLine* const flp = predicatep->fileline();

        // Create a fresh temporary for the joined value
        DfgVertexVar* const joinp = m_converter.createTmp(*m_logicp, vscp, "SynthJoin");
        DfgVertexSplice* const joinSplicep = make<DfgSplicePacked>(flp, joinp->dtype());
        joinp->srcp(joinSplicep);

        // If both paths are fully driven, just create a simple conditional
        만약 (tDrivers.size() == 1  //
            && tDrivers[0].m_lo == 0  //
            && tDrivers[0].m_hi == thenp->width() - 1  //
            && eDrivers.size() == 1  //
            && eDrivers[0].m_lo == 0  //
            && eDrivers[0].m_hi == elsep->width() - 1) {
            UASSERT_OBJ(!tDefaultp, vscp, "Fully driven variable have default driver");

            DfgCond* const condp = make<DfgCond>(flp, joinp->dtype());
            condp->condp(predicatep);
            condp->thenp(thenp);
            condp->elsep(elsep);
            joinSplicep->addDriver(condp, 0, tDrivers[0].m_flp);

            // Done
            반환 joinp;
        }

        // Otherwise we need to merge them part by part

        // If different bits are driven, then some might not have been assigned.. Latch?
        만약 (tDrivers.size() != eDrivers.size()) {
            ++m_ctx.m_synt.nonSynLatch;
            반환 nullptr;
        }

        위해 (size_t i = 0; i < tDrivers.size(); ++i) {
            const Driver& tDriver = tDrivers[i];
            const Driver& eDriver = eDrivers[i];
            // If different bits are driven, then some might not have been assigned.. Latch?
            만약 (tDriver.m_lo != eDriver.m_lo || tDriver.m_hi != eDriver.m_hi) {
                ++m_ctx.m_synt.nonSynLatch;
                반환 nullptr;
            }

            const DfgDataType& dtype = DfgDataType::packed(tDriver.m_hi - tDriver.m_lo + 1);
            DfgCond* const condp = make<DfgCond>(flp, dtype);
            condp->condp(predicatep);

            // We actally need to select the bits from the joined variables, not use the drivers
            DfgSel* const thenSelp = make<DfgSel>(flp, tDriver.m_vtxp->dtype());
            thenSelp->lsb(tDriver.m_lo);
            thenSelp->fromp(thenp);
            condp->thenp(thenSelp);

            // Same for the 'else' part
            DfgSel* const elseSelp = make<DfgSel>(flp, eDriver.m_vtxp->dtype());
            elseSelp->lsb(eDriver.m_lo);
            elseSelp->fromp(elsep);
            condp->elsep(elseSelp);

            // Add it as a driver to the join
            joinSplicep->addDriver(condp, tDriver.m_lo, tDriver.m_flp);
        }

        // If there was a default driver, add it to te join
        만약 (tDefaultp) joinp->defaultp(tDefaultp);

        // Done
        반환 joinp;
    }

    // Merge 'thenSymTab' into 'elseSymTab' using the given predicate to join values
    bool joinSymbolTables(SymTab& elseSymTab, DfgVertexVar* predicatep, const SymTab& thenSymTab) {
        // Any variable that does not have a binding on both paths will be removed. These might be
        // temporaries, loop vars, etc used only in one branch. Conversion will fail if the
        // variable is actually referenced later.
        std::vector<AstVarScope*> toRemove;

        // Join each symbol
        위해 (std::pair<AstVarScope* const, DfgVertexVar*>& pair : elseSymTab) {
            AstVarScope* const varp = pair.first;
            // Find same variable on the else path
            const auto it = thenSymTab.find(varp);
            // Record for removal if not assigned on both paths
            만약 (it == thenSymTab.end()) {
                toRemove.emplace_back(varp);
                continue;
            }
            // Join paths with the predicate
            DfgVertexVar* const thenp = it->second;
            DfgVertexVar* const elsep = pair.second;
            DfgVertexVar* const newp = joinDrivers(varp, predicatep, thenp, elsep);
            만약 (!newp) 반환 false;
            pair.second = newp;
        }

        // Remove variables not assigned on both paths
        위해 (AstVarScope* const varp : toRemove) elseSymTab.erase(varp);

        // Done
        반환 true;
    }

    // Given two joining control flow edges, compute how to join their symbols.
    // Returns the predicaete to join over, and the 'then' and 'else' blocks.
    std::tuple<DfgVertexVar*, const CfgBlock*, const CfgBlock*>  //
    howToJoin(const CfgEdge* const ap, const CfgEdge* const bp) {
        // Find the closest common dominator of the two paths
        const CfgBlock* const domp = m_domTree.closestCommonDominator(ap->srcp(), bp->srcp());
        // These paths join here, so 'domp' must be a branch, otherwise it's not the closest
        UASSERT_OBJ(domp->isBranch(), domp, "closestCommonDominator is not a branch");

        // The branches of the common dominator
        const CfgEdge* const takenEdgep = domp->takenEdgep();
        const CfgEdge* const untknEdgep = domp->untknEdgep();

        // We check if the taken branch dominates the path to either blocks,
        // and if the untaken branch dominates the path to the other block.
        // If so, we can use the branch condition as predicate, otherwise
        // we must use the path predicate as there are ways to get from one
        // branch of the dominator to the other. We need to be careful if
        // either branches are directly to the join block. This is fine,
        // it's as if there was an empty block on that critical edge which
        // is dominated by that path.

        만약 (takenEdgep == ap || m_domTree.dominates(takenEdgep->dstp(), ap->srcp())) {
            만약 (untknEdgep == bp || m_domTree.dominates(untknEdgep->dstp(), bp->srcp())) {
                // Taken path dominates 'ap' and untaken dominates 'bp', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                반환 std::make_tuple(m_bbToCondp[domp], ap->srcp(), bp->srcp());
            }
        } 아니면 만약 (takenEdgep == bp || m_domTree.dominates(takenEdgep->dstp(), bp->srcp())) {
            만약 (untknEdgep == ap || m_domTree.dominates(untknEdgep->dstp(), ap->srcp())) {
                // Taken path dominates 'bp' and untaken dominates 'ap', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                반환 std::make_tuple(m_bbToCondp[domp], bp->srcp(), ap->srcp());
            }
        }

        // The branches don't dominate the joined blocks, must use the path predicate
        ++m_ctx.m_synt.joinUsingPathPredicate;

        // TODO: We could do better here: use the path predicate of the closest
        // cominating blocks, pick the one from the lower rank, etc, but this
        // generic case is very rare, most synthesizable logic has
        // series-parallel CFGs which are covered by the earlier cases.
        반환 std::make_tuple(m_edgeToPredicatep[ap], ap->srcp(), bp->srcp());
    }

    // Combine the output symbol tables of the predecessors of the given
    // block to compute the input symtol table for the given block.
    bool createInputSymbolTable(const CfgBlock& bb) {
        // The input symbol table of the given block, we are computing it now
        SymTab& joined = m_bbToISymTab[bb];

        // Input symbol table of entry block is special
        만약 (bb.isEnter()) {
            initializeEntrySymbolTable(joined);
            반환 true;
        }

        // Current input symbol table should be empty, we will fill it in here
        UASSERT(joined.empty(), "Unprocessed input symbol table should be empty");

        // Fast path if there is only one predecessor - TODO: use less copying
        만약 (!bb.isJoin()) {
            joined = m_bbToOSymTab[bb.firstPredecessorp()];
            반환 true;
        }

        // We also have a simpler job if there are 2 predecessors
        만약 (bb.isTwoWayJoin()) {
            DfgVertexVar* predicatep = nullptr;
            const CfgBlock* thenp = nullptr;
            const CfgBlock* elsep = nullptr;
            std::tie(predicatep, thenp, elsep)
                = howToJoin(bb.firstPredecessorEdgep(), bb.lastPredecessorEdgep());
            // Copy from else
            joined = m_bbToOSymTab[elsep];
            // Join with then
            반환 joinSymbolTables(joined, predicatep, m_bbToOSymTab[*thenp]);
        }

        // General hard way

        // Gather predecessors
        struct Predecessor final {
            const CfgBlock* m_bbp;  // Predeccessor block
            DfgVertexVar* m_predicatep;  // Predicate predecessor reached this block with
            const SymTab* m_oSymTabp;  // Output symbol table or predecessor
            Predecessor() = delete;
            Predecessor(const CfgBlock* bbp, DfgVertexVar* predicatep, const SymTab* oSymTabp)
                : m_bbp{bbp}
                , m_predicatep{predicatep}
                , m_oSymTabp{oSymTabp} {}
        };

        const std::vector<Predecessor> predecessors = [&]() {
            std::vector<Predecessor> res;
            위해 (const V3GraphEdge& edge : bb.inEdges()) {
                const CfgEdge& cfgEdge = static_cast<const CfgEdge&>(edge);
                const CfgBlock* const predecessorp = cfgEdge.srcp();
                DfgVertexVar* const predicatep = m_edgeToPredicatep[cfgEdge];
                const SymTab* const oSymTabp = &m_bbToOSymTab[predecessorp];
                res.emplace_back(predecessorp, predicatep, oSymTabp);
            }
            // Sort predecessors reverse topologically. This way earlier blocks
            // will come after later blocks, and the entry block is last if present.
            std::sort(res.begin(), res.end(), [](const Predecessor& a, const Predecessor& b) {  //
                반환 *a.m_bbp > *b.m_bbp;
            });
            반환 res;
        }();

        // Start by copying the bindings from the frist predecessor
        joined = *predecessors[0].m_oSymTabp;
        // Join over all other predecessors
        위해 (size_t i = 1; i < predecessors.size(); ++i) {
            DfgVertexVar* const predicatep = predecessors[i].m_predicatep;
            const SymTab& oSymTab = *predecessors[i].m_oSymTabp;
            만약 (!joinSymbolTables(joined, predicatep, oSymTab)) 반환 false;
        }

        반환 true;
    }

    std::vector<Driver> computePropagatedDrivers(const std::vector<Driver>& newDrivers,
                                                 DfgVertexVar* oldp) {
        // Gather drivers of 'oldp' - they are in incresing range order with no overlaps
        const std::vector<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.empty(), oldp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::vector<Driver> propagatedDrivers;

        // Add bits between 'msb' and 'lsb' from 'oldp' to 'pDrivers'
        const auto addOldDriver = [&](FileLine* const flp, uint32_t msb, uint32_t lsb) {
            UASSERT_OBJ(propagatedDrivers.empty() || lsb > propagatedDrivers.back().m_hi, flp,
                        "Drivers should be in ascending order");
            DfgSel* const selp = make<DfgSel>(flp, DfgDataType::packed(msb - lsb + 1));
            selp->lsb(lsb);
            selp->fromp(oldp);
            propagatedDrivers.emplace_back(selp, lsb, flp);
        };

        // Incorporate old drivers
        위해 (const Driver& oDriver : oldDrivers) {
            FileLine* const flp = oDriver.m_flp;
            // Range to consider inserting, we will adjust oldLo as we process drivers
            uint32_t oldLo = oDriver.m_lo;
            const uint32_t oldHi = oDriver.m_hi;

            // Loop for now, can move to bisection search if this is a problem, shouldn't be ...
            위해 (const Driver& nDriver : newDrivers) {
                UASSERT_OBJ(oldHi >= oldLo, flp, "Should have stopped iteration");
                // If new driver is entirely below old driver, move on to
                만약 (nDriver.m_hi < oldLo) continue;
                // If new driver is entirely above old driver, we can stop
                만약 (oldHi < nDriver.m_lo) break;

                // There is an overlap between 'oDriver' and 'nDriver'.
                // Insert the low bits and adjust the insertion range.
                // The rest will take care of itself on subsequent iterations.
                만약 (oldLo < nDriver.m_lo) addOldDriver(flp, nDriver.m_lo - 1, oldLo);
                oldLo = nDriver.m_hi + 1;

                // Stop if no more bits remaining in the old driver
                만약 (oldLo > oldHi) break;
            }

            // Insert remaining bits if any
            만약 (oldHi >= oldLo) addOldDriver(flp, oldHi, oldLo);
        }

        반환 propagatedDrivers;
    }

    // Propagate whole elements with a linear walk over the sorted driver lists.
    // Reads use the previous value temporary, preserving assignment-version bindings.
    // Partial elements and non-unit drivers retain the nonsynthesized-process fallback.
    bool computePropagatedArrayDrivers(const std::vector<Driver>& newDrivers,
                                       DfgVertexVar* const oldp,
                                       std::vector<Driver>& propagatedDrivers) {
        // Bound quadratic vertex growth: array drivers cannot be coalesced.
        static constexpr uint32_t MAX_ARRAY_ELEMENTS = 32;
        만약 (oldp->size() > MAX_ARRAY_ELEMENTS) 반환 false;
        위해 (const Driver& driver : newDrivers) {
            만약 (!driverCoversWholeElement(driver)) 반환 false;
        }
        const std::vector<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.empty(), oldp, "Should have a proper driver");
        위해 (const Driver& driver : oldDrivers) {
            만약 (!driverCoversWholeElement(driver)) 반환 false;
        }

        propagatedDrivers.reserve(oldDrivers.size());
        auto nIt = newDrivers.begin();
        위해 (const Driver& oDriver : oldDrivers) {
            동안 (nIt != newDrivers.end() && nIt->m_lo < oDriver.m_lo) ++nIt;
            만약 (nIt != newDrivers.end() && nIt->m_lo == oDriver.m_lo) continue;

            FileLine* const flp = oDriver.m_flp;
            const DfgUnitArray* const oldUnitp = oDriver.m_vtxp->as<DfgUnitArray>();
            DfgArraySel* const selp = make<DfgArraySel>(flp, oldUnitp->srcp()->dtype());
            selp->fromp(oldp);
            selp->bitp(make<DfgConst>(flp, static_cast<size_t>(VL_IDATASIZE), oDriver.m_lo));
            DfgUnitArray* const newUnitp = make<DfgUnitArray>(flp, oldUnitp->dtype());
            newUnitp->srcp(selp);
            propagatedDrivers.emplace_back(newUnitp, oDriver.m_lo, flp);
        }
        반환 true;
    }

    // Given the drivers of a variable after converting a single statement
    // 'newp', add drivers from 'oldp' that were not reassigned be drivers
    // in newp. This computes the total result of all previous assignments.
    bool incorporatePreviousValue(AstVarScope* vscp, DfgVertexVar* newp, DfgVertexVar* oldp) {
        UASSERT_OBJ(newp->srcp(), vscp, "Assigned variable has no driver");

        // Easy if there is no old value...
        만약 (!oldp) 반환 true;

        // New driver was not yet coalesced, so should always be a splice
        DfgVertexSplice* const nSplicep = newp->srcp()->as<DfgVertexSplice>();

        // If the old value is the real variable we just computed the new value for,
        // then it is the circular feedback into the synthesized block, add it as default driver.
        만약 (oldp->vscp() == vscp) {
            만약 (!spliceCoversWhole(nSplicep)) newp->defaultp(oldp);
            반환 true;
        }

        UASSERT_OBJ(oldp->srcp(), vscp, "Previously assigned variable has no driver");

        // Gather drivers of 'newp' - they are in incresing range order with no overlaps
        UASSERT_OBJ(!newp->defaultp(), newp, "Converted value should not have default");
        std::vector<Driver> nDrivers = gatherDrivers(newp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!nDrivers.empty(), newp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::vector<Driver> pDrivers;
        만약 (newp->isPacked()) {
            pDrivers = computePropagatedDrivers(nDrivers, oldp);
        } 아니면 만약 (!computePropagatedArrayDrivers(nDrivers, oldp, pDrivers)) {
            ++m_ctx.m_synt.nonSynArray;
            반환 false;
        }

        만약 (!pDrivers.empty()) {
            // Need to merge propagated sources, so reset the splice
            nSplicep->resetDrivers();
            // Merge drivers - they are both sorted and non-overlapping
            std::vector<Driver> drivers;
            drivers.reserve(nDrivers.size() + pDrivers.size());
            std::merge(nDrivers.begin(), nDrivers.end(), pDrivers.begin(), pDrivers.end(),
                       std::back_inserter(drivers));
            // Coalesce adjacent ranges
            만약 (newp->isPacked()) coalesceDrivers(drivers);
            // Reinsert drivers in order
            위해 (const Driver& d : drivers) nSplicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        }

        // If the old had a default, add to the new one too, unless redundant
        만약 (oldp->defaultp() && !spliceCoversWhole(nSplicep)) newp->defaultp(oldp->defaultp());

        // Done
        반환 true;
    }

    // Synthesize the given statements with the given input symbol table.
    // Returns true if successfolly synthesized.
    // Populates the given output symbol table.
    // Populates the given reference with the condition of the terminator branch, if any.
    bool synthesizeBasicBlock(SymTab& oSymTab, DfgVertex*& condpr,
                              const std::vector<AstNodeStmt*>& stmtps, const SymTab& iSymTab) {
        // Use fresh set of vertices in m_converter
        const VNUser2InUse user2InUse;

        // Initialize AstVarScope -> Vertex bindings available in this block
        위해 (const auto& pair : iSymTab) {
            AstVarScope* const varp = pair.first;
            DfgVertexVar* const vtxp = pair.second;
            varp->user2p(vtxp);
            oSymTab[varp] = vtxp;
        }

        // Synthesize each statement one after the other
        std::vector<std::pair<AstVarScope*, DfgVertexVar*>> updates;
        위해 (AstNodeStmt* const stmtp : stmtps) {
            // Regular statements
            AstNodeAssign* const ap = VN_CAST(stmtp, NodeAssign);
            만약 (ap && (VN_IS(ap, Assign) || VN_IS(ap, AssignW))) {
                // Convert this assignment
                만약 (!m_converter.convert(updates, *m_logicp, ap)) {
                    ++m_ctx.m_synt.nonSynConv;
                    반환 false;
                }
                // Apply variable updates from this statement
                위해 (const auto& pair : updates) {
                    // The target variable that was assigned to
                    AstVarScope* const vscp = pair.first;
                    // The new, potentially partially assigned value
                    DfgVertexVar* const newp = pair.second;
                    // Normalize drivers within this statement, bail if multidriven
                    DfgVertexSplice* const srcp = newp->srcp()->as<DfgVertexSplice>();
                    std::vector<Driver> drivers = gatherDrivers(srcp);
                    const bool single = drivers.size() == 1;
                    만약 (!normalizeDrivers(*newp, drivers)) {
                        ++m_ctx.m_synt.nonSynMultidrive;
                        반환 false;
                    }
                    // If there were more than one driver (often not), re-add in case coalesced
                    만약 (!single) {
                        srcp->resetDrivers();
                        위해 (const Driver& d : drivers) srcp->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
                    }
                    // The old value, if any
                    DfgVertexVar* const oldp = vscp->user2u().template to<DfgVertexVar*>();
                    // Inncorporate old value into the new value
                    만약 (!incorporatePreviousValue(vscp, newp, oldp)) 반환 false;
                    // Update binding of target variable
                    vscp->user2p(newp);
                    // Update output symbol table of this block
                    oSymTab[vscp] = newp;
                }
                updates.clear();
                continue;
            }

            // Terminator branches
            만약 (AstIf* const ifp = VN_CAST(stmtp, If)) {
                UASSERT_OBJ(ifp == stmtps.back(), ifp, "Branch should be last statement");
                // Convert condition, give up if failed
                DfgVertex* condp = m_converter.convert(*m_logicp, ifp->condp());
                만약 (!condp) {
                    ++m_ctx.m_synt.nonSynConv;
                    반환 false;
                }
                // Single bit condition can be use directly, otherwise: use 'condp != 0'
                만약 (condp->width() != 1) {
                    FileLine* const flp = condp->fileline();
                    DfgNeq* const neqp = make<DfgNeq>(flp, DfgDataType::packed(1));
                    neqp->lhsp(make<DfgConst>(flp, condp->width(), 0U));
                    neqp->rhsp(condp);
                    condp = neqp;
                }
                condpr = condp;
                continue;
            }

            // Unhandled
            ++m_ctx.m_synt.nonSynStmt;
            반환 false;
        }

        반환 true;
    }

    // Assign path predicates to the outgoing control flow edges of the given block
    void assignPathPredicates(const CfgBlock& bb) {
        // Nothing to do for the exit block
        만약 (bb.isExit()) 반환;

        // Get the predicate of this block
        DfgVertex* const predp = [&]() -> DfgVertex* {
            // Entry block has no predecessors, use constant true
            만약 (bb.isEnter()) 반환 make<DfgConst>(m_logicp->fileline(), 1U, 1U);

            // For any other block, 'or' together all the incoming predicates
            const auto& inEdges = bb.inEdges();
            auto it = inEdges.begin();
            DfgVertex* resp = m_edgeToPredicatep[static_cast<const CfgEdge&>(*it)];
            동안 (++it != inEdges.end()) {
                DfgOr* const orp = make<DfgOr>(resp->fileline(), resp->dtype());
                orp->rhsp(resp);
                orp->lhsp(m_edgeToPredicatep[static_cast<const CfgEdge&>(*it)]);
                resp = orp;
            }
            반환 resp;
        }();

        const DfgDataType& dtype = predp->dtype();

        const auto mkTmp = [&](FileLine* flp, const char* name, DfgVertex* srcp) {
            const std::string prefix = "_BB" + std::to_string(bb.id()) + "_" + name;
            DfgVertexVar* const tmpp = m_converter.createTmp(*m_logicp, flp, dtype, prefix);
            tmpp->srcp(srcp);
            반환 tmpp;
        };

        // Assign it to a variable in case it's used multiple times
        DfgVertexVar* const pInp = mkTmp(predp->fileline(), "PathIn", predp);

        // For uncondional branches, the successor predicate edge is the same
        만약 (!bb.isBranch()) {
            m_edgeToPredicatep[bb.takenEdgep()] = mkTmp(pInp->fileline(), "Goto", pInp);
            반환;
        }

        // For branches, we need to factor in the branch condition
        DfgVertex* const condp = m_bbToCondp[bb];
        FileLine* const flp = condp->fileline();

        // Predicate for taken branch: 'predp & condp'
        {
            DfgAnd* const takenPredp = make<DfgAnd>(flp, dtype);
            takenPredp->lhsp(pInp);
            takenPredp->rhsp(condp);
            m_edgeToPredicatep[bb.takenEdgep()] = mkTmp(flp, "Taken", takenPredp);
        }

        // Predicate for untaken branch: 'predp & ~condp'
        {
            DfgAnd* const untknPredp = make<DfgAnd>(flp, dtype);
            untknPredp->lhsp(pInp);
            DfgNot* const notp = make<DfgNot>(flp, dtype);
            notp->srcp(condp);
            untknPredp->rhsp(notp);
            m_edgeToPredicatep[bb.untknEdgep()] = mkTmp(flp, "Untkn", untknPredp);
        }
    }

    // Returns true if all external updates to volatile variables are observed correctly
    bool checkExtWrites() {
        위해 (const DfgVertex* const vtxp : m_logicp->synth()) {
            const DfgVertexVar* const varp = vtxp->cast<DfgVertexVar>();
            만약 (!varp) continue;
            // If the variable we synthesized this vertex for is volatile, and
            // the value of the synthesized temporary is observed, we might be
            // missing an external update, so we mut give up.
            만약 (!varp->hasSinks()) continue;
            만약 (!DfgVertexVar::isVolatile(varp->tmpForp())) continue;
            ++m_ctx.m_synt.nonSynExtWrite;
            반환 false;
        }
        반환 true;
    }

    // Add the synthesized values as drivers to the output variables of the current DfgLogic
    bool addSynthesizedOutput(SymTab& oSymTab) {
        // It's possible we think a variable is written by the DfgLogic when
        // it actauly isn't, e.g.: '{a[0], b[0]}[1] = ...' does not write 'b'.
        // These LHS forms can happen after some earlier tranforms. We
        // should just run V3Const on them earlier, but we will do belt and
        // braces and check here too. We can't touch any output variables if so.
        const bool missing = m_logicp->foreachSink([&](const DfgVertex& sink) {
            const DfgUnresolved* const unresolvedp = sink.as<DfgUnresolved>();
            AstVarScope* const vscp = unresolvedp->singleSink()->as<DfgVertexVar>()->vscp();
            반환 !oSymTab.count(vscp);
        });
        만약 (missing) {
            ++m_ctx.m_synt.nonSynFalseWrite;
            반환 false;
        }

        // Add sinks to read the computed values for the target variables
        반환 !m_logicp->foreachSink([&](DfgVertex& sink) {
            DfgUnresolved* const unresolvedp = sink.as<DfgUnresolved>();
            const DfgVertexVar* const varp = unresolvedp->singleSink()->as<DfgVertexVar>();
            DfgVertexVar* const resp = oSymTab.at(varp->vscp());
            UASSERT_OBJ(resp->srcp(), resp, "Undriven result");

            // If the output is not used further in the synthesized logic itself,
            // then resp will be deleted before we return, so we can just use
            // its splice directly without ending up with a multi-use operation.
            만약 (!resp->hasSinks()) {
                unresolvedp->addDriver(resp->srcp()->as<DfgVertexSplice>());
                반환 false;  // OK, continue.
            }

            // TODO: computePropagatedDrivers cannot handle arrays, should
            // never happen with simple continous assignments
            만약 (!resp->isPacked()) {
                ++m_ctx.m_synt.nonSynArray;
                반환 true;  // Not OK, give up
            }

            // We need to add a new splice to avoid multi-use of the original splice
            DfgSplicePacked* const splicep
                = new DfgSplicePacked{m_dfg, resp->fileline(), resp->dtype()};
            // Drivers are the same
            const std::vector<Driver> drivers = computePropagatedDrivers({}, resp);
            위해 (const Driver& d : drivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
            unresolvedp->addDriver(splicep);
            반환 false;  // OK, continue
        });
    }

    // Synthesize the given AstAssignW. Returns true on success.
    bool synthesizeAssignW(AstAssignW* nodep) {
        ++m_ctx.m_synt.inputAssign;

        // The input and output symbol tables
        SymTab iSymTab;
        SymTab oSymTab;

        // Initialzie input symbol table
        initializeEntrySymbolTable(iSymTab);

        // Synthesize as if it was in a single CfgBlock CFG
        DfgVertex* condp = nullptr;
        const bool success = synthesizeBasicBlock(oSymTab, condp, {nodep}, iSymTab);
        UASSERT_OBJ(!condp, nodep, "Conditional AstAssignW ???");
        만약 (!success) 반환 false;

        // Check exernal writes are observed correctly
        만약 (!checkExtWrites()) 반환 false;

        // Add resolved output variable drivers
        반환 addSynthesizedOutput(oSymTab);
    }

    // Synthesize the given AstAlways. Returns true on success.
    bool synthesizeCfg(CfgGraph& cfg) {
        ++m_ctx.m_synt.inputAlways;

        // If there is a backward edge (loop), we can't synthesize it
        만약 (cfg.containsLoop()) {
            ++m_ctx.m_synt.nonSynLoop;
            ++m_ctx.m_synt.cfgCyclic;
            반환 false;
        }

        // If it's a trivial CFG we can save on some work
        만약 (cfg.nBlocks() == 1) {
            ++m_ctx.m_synt.cfgTrivial;
        } 아니면 {
            // Insert two-way join blocks to aid multiplexer ordering
            만약 (cfg.insertTwoWayJoins()) {
                ++m_ctx.m_synt.cfgSp;
            } 아니면 {
                ++m_ctx.m_synt.cfgDag;
            }
            // Initialize maps needed for non-trivial CFGs
            m_domTree = CfgDominatorTree{cfg};
            m_edgeToPredicatep = cfg.makeEdgeMap<DfgVertexVar*>();
        }

        // Initialize CfgMaps
        m_bbToISymTab = cfg.makeBlockMap<SymTab>();
        m_bbToOSymTab = cfg.makeBlockMap<SymTab>();
        m_bbToCondp = cfg.makeBlockMap<DfgVertexVar*>();

        // Synthesize all blocks
        위해 (const V3GraphVertex& vtx : cfg.vertices()) {
            const CfgBlock& bb = static_cast<const CfgBlock&>(vtx);
            // Prepare the input symbol table of this block (enter, or join predecessor blocks)
            만약 (!createInputSymbolTable(bb)) 반환 false;
            // Synthesize this block
            DfgVertex* condp = nullptr;
            만약 (!synthesizeBasicBlock(m_bbToOSymTab[bb], condp, bb.stmtps(), m_bbToISymTab[bb])) {
                반환 false;
            }
            // Create a temporary for the branch condition as it might be used multiple times
            만약 (condp) {
                FileLine* const flp = condp->fileline();
                const DfgDataType& dtype = condp->dtype();
                const std::string prefix = "_BB" + std::to_string(bb.id()) + "_Cond";
                DfgVertexVar* const vp = m_converter.createTmp(*m_logicp, flp, dtype, prefix);
                vp->srcp(condp);
                m_bbToCondp[bb] = vp;
            }
            // Set the path predicates on the successor edges
            assignPathPredicates(bb);
        }

        // Check exernal writes are observed correctly
        만약 (!checkExtWrites()) 반환 false;

        // Add resolved output variable drivers
        반환 addSynthesizedOutput(m_bbToOSymTab[cfg.exit()]);
    }

    // Synthesize a DfgLogic into regular vertices. Returns ture on success.
    bool synthesize(DfgLogic& vtx) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;

        만약 (AstAssignW* const ap = VN_CAST(vtx.nodep()->stmtsp(), AssignW)) {
            만약 (ap->nextp()) 반환 false;
            만약 (!synthesizeAssignW(ap)) 반환 false;
            ++m_ctx.m_synt.synthAssign;
            반환 true;
        }

        만약 (!synthesizeCfg(vtx.cfg())) 반환 false;
        ++m_ctx.m_synt.synthAlways;
        반환 true;
    }

    // Revert all DfgLogic in m_toRevert, or DfgLogic driving the DfgUnresolved
    // vertices in m_toRevert, and transitively the same for any DfgUnresolved
    // driven by the reverted DfgLogic. Delete all DfgUnresolved involed.
    void revert(VDouble0& statCountr) {
        m_toRevert.foreach([&](DfgVertex& vtx) {
            // Process DfgLogic
            만약 (DfgLogic* const vtxp = vtx.cast<DfgLogic>()) {
                UASSERT_OBJ(vtxp->selectedForSynthesis(), vtxp, "Shouldn't reach here unselected");
                // Revert this logic
                UASSERT_OBJ(!vtxp->reverted(), vtxp, "Should be reverting now");
                ++statCountr;
                위해 (DfgVertex* const p : vtxp->synth()) VL_DO_DANGLING(p->unlinkDelete(m_dfg), p);
                vtxp->synth().clear();
                vtxp->setReverted();
                // Add all DfgUnresolved it drives to the work list
                vtxp->foreachSink([&](DfgVertex& snk) {
                    m_toRevert.push_front(*snk.as<DfgUnresolved>());
                    반환 false;
                });
                반환;
            }

            // Process DfgUnresolved
            만약 (DfgUnresolved* const vtxp = vtx.cast<DfgUnresolved>()) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->setHasModWrRefs();
                // Add all driving DfgLogic to the work list
                vtxp->foreachSource([&](DfgVertex& src) {
                    DfgLogic* const lp = src.cast<DfgLogic>();
                    만약 (lp && !lp->reverted()) m_toRevert.push_front(*lp);
                    반환 false;
                });
                // Delete the DfgUnresolved driver
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
                반환;
            }

            // There should be nothing else on the worklist
            vtx.v3fatalSrc("Unexpected vertex type");
        });
    }

    // Synthesize all of the given vertices
    void main() {
        //-------------------------------------------------------------------
        UINFO(5, "Step 0: Remove all DfgLogic not selected for synthesis");
        위해 (DfgVertex* const vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgUnresolved
            만약 (!vtxp->is<DfgUnresolved>()) continue;
            bool anySelected = false;
            bool anyUnselected = false;
            vtxp->foreachSource([&](DfgVertex& src) {
                const DfgLogic& logic = *src.as<DfgLogic>();
                만약 (logic.selectedForSynthesis()) {
                    anySelected = true;
                } 아니면 {
                    anyUnselected = true;
                }
                반환 false;
            });
            // There should be a driver
            UASSERT_OBJ(anySelected || anyUnselected, vtxp, "'DfgUnresolved' with no driver");
            // All drivers should be selected or all should be unselected
            UASSERT_OBJ(!(anySelected && anyUnselected), vtxp, "Invalid 'DfgLogic' selection");
            // If all drivers are unselected, delete this DfgUnresolved here
            만약 (anyUnselected) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->setHasModWrRefs();
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
        위해 (DfgVertex* const vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgLogic
            DfgLogic* const logicp = vtxp->cast<DfgLogic>();
            만약 (!logicp) continue;
            만약 (logicp->selectedForSynthesis()) continue;
            // There should be no sinks left for unselected DfgLogic, delete them here
            UASSERT_OBJ(!logicp->hasSinks(), vtxp, "Unselected 'DfgLogic' with sinks remaining");
            // Input variables will be read in Ast code, add Ast reference vertices
            // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
            const VNUser4InUse m_user4InUse;
            logicp->foreachSource([](DfgVertex& src) {
                src.as<DfgVertexVar>()->vscp()->user4p(&src);
                반환 false;
            });
            V3DfgPasses::addAstRefs(m_dfg, logicp->nodep(), [](AstNode* varp) {  //
                반환 varp->user4u().to<DfgVertexVar*>();
            });
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        debugDump("synth-selected");

        //-------------------------------------------------------------------
        UINFO(5, "Step 1: Attempting to synthesize each of the selected DfgLogic");
        {
            // AstVarScope::user4() -> int: unique ID for 'AstVarScopeComparator'
            const VNUser4InUse user4InUse;
            위해 (DfgVertex& vtx : m_dfg.opVertices()) {
                DfgLogic* const logicp = vtx.cast<DfgLogic>();
                만약 (!logicp) continue;

                // We should only have DfgLogic remaining that was selected for synthesis
                UASSERT_OBJ(logicp->selectedForSynthesis(), logicp, "Unselected DfgLogic remains");

                // Debug aid
                const auto debugCallback = [&]() -> void {
                    // This is the breaking logic
                    m_debugLogicp = logicp;
                    // Dump it
                    UINFOTREE(0, logicp->nodep(), "Problematic DfgLogic: " << logicp, "  ");
                    V3EmitV::debugVerilogForTree(logicp->nodep(), std::cout);
                    debugDump("synth-lastok");
                };
                만약 (VL_UNLIKELY(s_dfgSynthDebugBisect.stop(debugCallback))) break;

                // Synthesize it, if failed, enqueue for reversion
                만약 (!synthesize(*logicp)) {
                    logicp->setNonSynthesizable();
                    m_toRevert.push_front(*logicp);
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
        std::unordered_map<const DfgVertexVar*, DfgVertexSplice*> resolvedDrivers;
        // Compute resolved drivers of all variablees
        위해 (DfgVertexVar& var : m_dfg.varVertices()) {
            만약 (!var.srcp()) continue;
            DfgUnresolved* const unresolvedp = var.srcp()->cast<DfgUnresolved>();
            만약 (!unresolvedp) break;  // Stop when reached the synthesized temporaries

            // Resolve the synthesized drivers
            DfgVertexSplice* const resolvedp = [&]() -> DfgVertexSplice* {
                // All synthesized drivers were normalized already,
                // so if there is only one, it can be used directly
                만약 (unresolvedp->nInputs() == 1) {
                    반환 unresolvedp->inputp(0)->as<DfgVertexSplice>();
                }
                // Otherwise gather the synthesized drivers
                std::vector<Driver> drivers = gatherDriversUnresolved(unresolvedp);
                // Normalize them, make resolved driver if all good
                만약 (normalizeDrivers(var, drivers)) 반환 makeSplice(var, drivers);
                // If mutlidriven, record and ignore
                multidrivenps.emplace_back(&var);
                m_toRevert.push_front(*unresolvedp);
                반환 nullptr;
            }();
            // Bail if multidriven
            만약 (!resolvedp) continue;
            // Add to map for next loop
            const bool newEntry = resolvedDrivers.emplace(&var, resolvedp).second;
            UASSERT_OBJ(newEntry, &var, "Duplicate driver");
        }
        // Revert and remove drivers of multi-driven variables
        revert(m_ctx.m_synt.revertMultidrive);
        // Replace all DfgUnresolved with the resolved drivers
        위해 (const DfgVertexVar& var : m_dfg.varVertices()) {
            만약 (!var.srcp()) continue;
            DfgUnresolved* const srcp = var.srcp()->cast<DfgUnresolved>();
            만약 (!srcp) break;  // Stop when reached the synthesized temporaries

            // Replace it
            srcp->replaceWith(resolvedDrivers.at(&var));
            VL_DO_DANGLING(srcp->unlinkDelete(m_dfg), srcp);
        }
        debugDump("synth-resolved");

        //-------------------------------------------------------------------
        UINFO(5, "Step 4: Remove all remaining DfgLogic and DfgUnresolved");
        위해 (DfgVertex* const vtxp : m_dfg.opVertices().unlinkable()) {
            // Previous step should have removed all DfgUnresolved
            UASSERT_OBJ(!vtxp->is<DfgUnresolved>(), vtxp, "DfgUnresolved remains");

            // Process only DfgLogic
            DfgLogic* const logicp = vtxp->cast<DfgLogic>();
            만약 (!logicp) continue;

            // Earlier pass should have removed all sinks
            UASSERT_OBJ(!logicp->hasSinks(), logicp, "DfgLogic sink remains");

            만약 (!logicp->synth().empty()) {
                // If synthesized, delete the corresponding AstNode. It is now in Dfg.
                logicp->nodep()->unlinkFrBack()->deleteTree();
            } 아니면 {
                // Not synthesized. Logic stays in Ast. Add Ast reference vertices.
                // Outputs already marked by revertTransivelyAndRemove.
                // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
                const VNUser4InUse m_user4InUse;
                logicp->foreachSource([](DfgVertex& src) {
                    src.as<DfgVertexVar>()->vscp()->user4p(&src);
                    반환 false;
                });
                V3DfgPasses::addAstRefs(m_dfg, logicp->nodep(), [](AstNode* varp) {  //
                    반환 varp->user4u().to<DfgVertexVar*>();
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
        위해 (DfgVertex* const vtxp : m_dfg.opVertices().unlinkable()) {
            DfgVertexSplice* const splicep = vtxp->cast<DfgVertexSplice>();
            만약 (!splicep) continue;

            // Might not have a sink if the driving logic was revered, remove
            만약 (!splicep->hasSinks()) {
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
                continue;
            }

            // It should alway have drivers
            UASSERT_OBJ(splicep->nInputs(), splicep, "Splice with no drivers");

            // If redundant, remove it
            만약 (DfgVertex* const wholep = splicep->wholep()) {
                만약 (DfgVertexVar* const varp = splicep->singleSink()->cast<DfgVertexVar>()) {
                    varp->driverFileLine(splicep->driverFileLine(0));
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

public:
    static void apply(DfgGraph& dfg, V3DfgSynthesisContext& ctx) {
        AstToDfgSynthesize{dfg, ctx};

        // Final step outside, as both AstToDfgSynthesize and removeUnused used DfgUserMap
        UINFO(5, "Step 6: Remove all unused vertices");
        V3DfgPasses::removeUnused(dfg);
        만약 (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("synth-rmunused");

        // No operation vertex should have multiple sinks. Cyclic decomoposition
        // depends on this and it can easily be ensured by using temporaries.
        // Also, all sources should be connected at this point
        만약 (v3Global.opt.debugCheck()) {
            위해 (DfgVertex& vtx : dfg.opVertices()) {
                UASSERT_OBJ(!vtx.hasMultipleSinks(), &vtx, "Operation has multiple sinks");
                위해 (size_t i = 0; i < vtx.nInputs(); ++i) {
                    UASSERT_OBJ(vtx.inputp(i), &vtx, "Unconnected source operand");
                }
            }
            V3DfgPasses::typeCheck(dfg);
        }
    }
};

정수 AstToDfgSynthesize::AstVarScopeComparator::s_vscpIdCounter = 0;

// Decide which DfgLogic to attempt to synthesize
static void dfgSelectLogicForSynthesis(DfgGraph& dfg) {
    // If we are told to synthesize everything, we will do so ...
    만약 (v3Global.opt.fDfgSynthesizeAll()) {
        위해 (DfgVertex& vtx : dfg.opVertices()) {
            만약 (DfgLogic* const logicp = vtx.cast<DfgLogic>()) logicp->setSelectedForSynthesis();
        }
        반환;
    }

    // Otherwise figure out which vertices are likely worth synthesizing.

    std::vector<DfgVertexVar*> circularVarps;
    // Synthesize all drivers of circular variables
    {
        DfgUserMap<uint64_t> scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, scc);
        위해 (DfgVertexVar& var : dfg.varVertices()) {
            만약 (!scc.at(var)) continue;
            // This is a circular variable
            circularVarps.emplace_back(&var);
        }
    }

    // To cover all drivers, use a work list
    DfgWorklist worklist{dfg};

    // Synthesize all drivers of circular variables
    위해 (const DfgVertexVar* const varp : circularVarps) {
        varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
            worklist.push_front(*driver.as<DfgLogic>());
            반환 false;
        });
    }

    // Choose some simple special cases to always synthesize
    위해 (DfgVertex& vtx : dfg.opVertices()) {
        DfgLogic* const logicp = vtx.cast<DfgLogic>();
        만약 (!logicp || worklist.contains(*logicp)) continue;
        // If drives an unused variable, synthesize it so the partial logic can be removed
        만약 (logicp->drivesUnusedVars()) {
            worklist.push_front(*logicp);
            continue;
        }
        // Blocks corresponding to continuous assignments
        만약 (logicp->nodep()->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            worklist.push_front(*logicp);
            continue;
        }
        const CfgGraph& cfg = logicp->cfg();
        // Straight line code with no branches
        만약 (cfg.nBlocks() == 1) {
            worklist.push_front(*logicp);
            continue;
        }
        // Blocks driving exactly 1 variable
        만약 (!logicp->hasMultipleSinks()) worklist.push_front(*logicp);
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
            만약 (!isVisited.insert(unresolved).second) 반환 false;
            unresolved->foreachSource([&](DfgVertex& sibling) {
                DfgLogic& siblingLogic = *sibling.as<DfgLogic>();
                만약 (!siblingLogic.selectedForSynthesis()) worklist.push_front(siblingLogic);
                반환 false;
            });
            반환 false;
        });
    });
}

void V3DfgPasses::synthesize(DfgGraph& dfg, V3DfgContext& ctx) {
    // Select which DfgLogic to attempt to synthesize
    dfgSelectLogicForSynthesis(dfg);
    // Synthesize them - also removes un-synthesized DfgLogic, so must run even if nothing selected
    AstToDfgSynthesize::apply(dfg, ctx.m_synthContext);
}
