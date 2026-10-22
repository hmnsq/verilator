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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Cfg.h"
#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Dfg.h"
#அந்தர்பாவ "V3DfgPasses.h"
#அந்தர்பாவ "V3EmitV.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <iterator>

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ {

// Create a DfgVertex out of a AstNodeExpr. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
template <typename T_Vertex, typename T_முனை>
T_Vertex* makeVertex(மாறிலி T_முனை* nodep, DfgGraph& dfg, மாறிலி DfgDataType& dtype) {
    திரும்பு புதிய T_Vertex{dfg, nodep->fileline(), dtype};
}

template <>
DfgArraySel* makeVertex<DfgArraySel, AstArraySel>(மாறிலி AstArraySel* nodep, DfgGraph& dfg,
                                                  மாறிலி DfgDataType& dtype) {
    // Some earlier passes create malformed ArraySels, just bail on those...
    // See t_bitsel_wire_array_bad
    எனில் (VN_IS(nodep->fromp(), Const)) திரும்பு nullptr;
    எனில் (!VN_IS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) திரும்பு nullptr;
    திரும்பு புதிய DfgArraySel{dfg, nodep->fileline(), dtype};
}

}  // namespace

// Visitor that can convert Ast statements and expressions in Dfg vertices
வகுப்பு AstToDfgConverter final : பொது VNVisitor {
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

    தருக்க m_foundUnhandled = பொய்;  // Found node not implemented as DFG or not implemented 'visit'
    தருக்க m_converting = பொய்;  // We are trying to convert some logic at the moment

    // METHODS

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    template <typename Vertex, typename... Args>
    Vertex* make(Args&&... args) {
        static_assert(!std::is_base_of<DfgVertexVar, Vertex>::மதிப்பு, "Do not use for variables");
        static_assert(std::is_base_of<DfgVertex, Vertex>::மதிப்பு, "'Vertex' must be a 'DfgVertex'");
        Vertex* மாறிலி vtxp = புதிய Vertex{m_dfg, std::forward<Args>(args)...};
        m_logicp->synth().emplace_back(vtxp);
        திரும்பு vtxp;
    }

    // Returns true if the expression cannot (or should not) be represented by DFG
    தருக்க unhandled(Ast_முனை_கோவை* nodep) {
        // Short-circuiting if something was already unhandled
        எனில் (m_foundUnhandled) {
            // Impure nodes cannot be represented
            எனில் (!nodep->isPure()) {
                m_foundUnhandled = உண்மை;
                ++m_ctx.m_conv.nonRepImpure;
            }
        }
        திரும்பு m_foundUnhandled;
    }

    தருக்க isSupported(மாறிலி AstVarRef* nodep) {
        // Cannot represent cross module references
        எனில் (nodep->classOrPackagep()) திரும்பு பொய்;
        // Check target
        திரும்பு V3Dfg::isSupported(nodep->varScopep());
    }

    // Given an RValue expression, return the equivalent Vertex, or nullptr if not representable.
    DfgVertex* convertRValue(Ast_முனை_கோவை* nodep) {
        UASSERT_OBJ(!m_converting, nodep, "'convertingRValue' should not be called recursively");
        VL_RESTORER(m_converting);
        VL_RESTORER(m_foundUnhandled);
        m_converting = உண்மை;
        m_foundUnhandled = பொய்;

        // Convert the expression
        iterate(nodep);

        // If falied to convert, return nullptr
        எனில் (m_foundUnhandled) திரும்பு nullptr;

        // Traversal set user2p to the equivalent vertex
        DfgVertex* மாறிலி vtxp = nodep->user2u().to<DfgVertex*>();
        UASSERT_OBJ(vtxp, nodep, "Missing Dfg vertex after conversion");
        திரும்பு vtxp;
    }

    // Given an LValue expression, return the splice node that writes the
    // destination, together with the index to use for splicing in the value.
    // Returns {nullptr, 0}, if the given LValue expression is not supported.
    std::pair<DfgVertexSplice*, uint32_t> convertLValue(Ast_முனை_கோவை* nodep) {
        எனில் (மாறிலி AstVarRef* மாறிலி vrefp = VN_CAST(nodep, VarRef)) {
            UASSERT_OBJ(vrefp->access().isWriteOnly(), vrefp, "Non-WriteOnly reference");
            எனில் (!isSupported(vrefp)) {
                ++m_ctx.m_conv.nonRepLValue;
                திரும்பு {nullptr, 0};
            }

            // Get (or create a new) temporary for this variable
            மாறிலி DfgVertexVar* மாறிலி vtxp = [&]() -> DfgVertexVar* {
                // The variable being assigned
                AstVarScope* மாறிலி vscp = vrefp->varScopep();

                // Find existing one, if any
                க்கு (மாறிலி auto& pair : *m_updatesp) {
                    எனில் (pair.first == vscp) திரும்பு pair.second;
                }

                // Create new one
                DfgVertexVar* மாறிலி newp = createTmp(*m_logicp, vscp, "SynthAssign");
                m_updatesp->emplace_back(vscp, newp);

                // Create the Splice driver for the new temporary
                எனில் (newp->is<DfgVarPacked>()) {
                    newp->srcp(make<DfgSplicePacked>(newp->fileline(), newp->dtype()));
                } இல்லையெனில் எனில் (newp->is<DfgVarArray>()) {
                    newp->srcp(make<DfgSpliceArray>(newp->fileline(), newp->dtype()));
                } இல்லையெனில் {
                    nodep->v3fatalSrc("Unhandled DfgVertexVar sub-type");
                }

                // Use new temporary
                திரும்பு newp;
            }();

            // Return the Splice driver
            திரும்பு {vtxp->srcp()->as<DfgVertexSplice>(), 0};
        }

        எனில் (மாறிலி AstSel* selp = VN_CAST(nodep, Sel)) {
            // Only handle constant selects
            மாறிலி AstConst* மாறிலி lsbp = VN_CAST(selp->lsbp(), Const);
            எனில் (!lsbp) {
                ++m_ctx.m_conv.nonRepLValue;
                திரும்பு {nullptr, 0};
            }
            uint32_t lsb = lsbp->toUInt();

            // Convert the 'fromp' sub-expression
            மாறிலி auto pair = convertLValue(selp->fromp());
            எனில் (!pair.first) திரும்பு {nullptr, 0};
            DfgSplicePacked* மாறிலி splicep = pair.first->template as<DfgSplicePacked>();
            // Adjust index.
            lsb += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            எனில் (lsb + static_cast<uint32_t>(selp->widthConst()) > splicep->size()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                திரும்பு {nullptr, 0};
            }

            // AstSel doesn't change type kind (array vs packed), so we can use
            // the existing splice driver with adjusted lsb
            திரும்பு {splicep, lsb};
        }

        எனில் (மாறிலி AstArraySel* மாறிலி aselp = VN_CAST(nodep, ArraySel)) {
            // Only handle constant selects
            மாறிலி AstConst* மாறிலி indexp = VN_CAST(aselp->bitp(), Const);
            எனில் (!indexp) {
                ++m_ctx.m_conv.nonRepLValue;
                திரும்பு {nullptr, 0};
            }
            uint32_t index = indexp->toUInt();

            // Convert the 'fromp' sub-expression
            மாறிலி auto pair = convertLValue(aselp->fromp());
            எனில் (!pair.first) திரும்பு {nullptr, 0};
            DfgSpliceArray* மாறிலி splicep = pair.first->template as<DfgSpliceArray>();
            // Adjust index. Note pair.second is always 0, but we might handle array slices later..
            index += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            எனில் (index + 1U > splicep->size()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                திரும்பு {nullptr, 0};
            }

            // Ensure the Splice driver exists for this element
            எனில் (!splicep->driverAt(index)) {
                File_வரி* மாறிலி flp = nodep->fileline();
                // This should never fail
                மாறிலி DfgDataType& dtype = *DfgDataType::fromAst(nodep->dtypep());
                எனில் (dtype.isPacked()) {
                    DfgSplicePacked* மாறிலி newp = make<DfgSplicePacked>(flp, dtype);
                    மாறிலி DfgDataType& uaDtype = DfgDataType::array(dtype, 1);
                    DfgUnitArray* மாறிலி uap = make<DfgUnitArray>(flp, uaDtype);
                    uap->srcp(newp);
                    splicep->addDriver(uap, index, flp);
                } இல்லையெனில் எனில் (dtype.isArray()) {
                    DfgSpliceArray* மாறிலி newp = make<DfgSpliceArray>(flp, dtype);
                    splicep->addDriver(newp, index, flp);
                } இல்லையெனில் {
                    nodep->v3fatalSrc("Unhandled data type kind");
                }
            }

            // Return the splice driver
            DfgVertex* driverp = splicep->driverAt(index);
            எனில் (மாறிலி DfgUnitArray* மாறிலி uap = driverp->cast<DfgUnitArray>()) {
                driverp = uap->srcp();
            }
            திரும்பு {driverp->as<DfgVertexSplice>(), 0};
        }

        ++m_ctx.m_conv.nonRepLValue;
        திரும்பு {nullptr, 0};
    }

    // Given the LHS of an assignment, and the vertex representing the RHS,
    // connect up the RHS to drive the targets.
    // Returns true on success, false if the LHS is not representable.
    தருக்க convertAssignment(File_வரி* flp, Ast_முனை_கோவை* lhsp, DfgVertex* vtxp) {
        // Represents a DFG assignment contributed by the AST assignment with the above 'lhsp'.
        // There might be multiple of these if 'lhsp' is a concatenation.
        கட்டமைப்பு Assignment final {
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
        எனில் (!VN_IS(lhsp, VarRef)) lhsp = VN_AS(V3Const::constifyExpensiveEdit(lhsp), முனை_கோவை);

        // Assigning compound expressions to a concatenated LHS requires a temporary
        // to avoid multiple use of the expression
        எனில் (VN_IS(lhsp, Concat) && !vtxp->is<DfgVertexVar>() && !vtxp->is<DfgConst>()) {
            DfgVertexVar* மாறிலி tmpp = createTmp(*m_logicp, flp, vtxp->dtype(), "Unpack");
            tmpp->srcp(vtxp);
            vtxp = tmpp;
        }

        // Convert each concatenation LHS separately, gather all assignments
        // we need to do into 'assignments', return true if all LValues
        // converted successfully.
        std::vector<Assignment> assignments;
        மாறிலி std::function<தருக்க(Ast_முனை_கோவை*, uint32_t)> convertAllLValues
            = [&](Ast_முனை_கோவை* subp, uint32_t lsb) -> தருக்க {
            // Concatenation on the LHS, convert each part
            எனில் (AstConcat* மாறிலி concatp = VN_CAST(subp, Concat)) {
                Ast_முனை_கோவை* மாறிலி cRhsp = concatp->rhsp();
                Ast_முனை_கோவை* மாறிலி cLhsp = concatp->lhsp();
                // Convert Rigth of concat
                எனில் (!convertAllLValues(cRhsp, lsb)) திரும்பு பொய்;
                // Convert Left of concat
                திரும்பு convertAllLValues(cLhsp, lsb + cRhsp->width());
            }

            // Non-concatenation, convert the LValue
            மாறிலி auto pair = convertLValue(subp);
            எனில் (!pair.first) திரும்பு பொய்;

            // If whole lhs, just use it
            எனில் (subp == lhsp) {
                assignments.emplace_back(pair.first, pair.second, vtxp);
                திரும்பு உண்மை;
            }

            // Otherwise select the relevant bits
            மாறிலி DfgDataType& dtype = *DfgDataType::fromAst(subp->dtypep());
            DfgSel* மாறிலி selp = make<DfgSel>(subp->fileline(), dtype);
            selp->fromp(vtxp);
            selp->lsb(lsb);
            assignments.emplace_back(pair.first, pair.second, selp);
            திரும்பு உண்மை;
        };

        // Convert the given LHS assignment, give up if any LValues failed to convert
        எனில் (!convertAllLValues(lhsp, 0)) திரும்பு பொய்;

        // All successful, connect the drivers
        க்கு (மாறிலி Assignment& item : assignments) {
            எனில் (DfgSplicePacked* மாறிலி spp = item.m_lhsp->template cast<DfgSplicePacked>()) {
                spp->addDriver(item.m_rhsp, item.m_idx, flp);
            } இல்லையெனில் எனில் (DfgSpliceArray* மாறிலி sap = item.m_lhsp->template cast<DfgSpliceArray>()) {
                // TODO: multi-dimensional arrays will need changes here
                மாறிலி DfgDataType& rDt = item.m_rhsp->dtype();
                எனில் (rDt.isPacked()) {
                    // RHS is assigning an element of this array. Need a DfgUnitArray adapter.
                    DfgUnitArray* மாறிலி uap = make<DfgUnitArray>(flp, DfgDataType::array(rDt, 1));
                    uap->srcp(item.m_rhsp);
                    sap->addDriver(uap, item.m_idx, flp);
                } இல்லையெனில் {
                    // RHS is assigning an array (or array slice). Should be the same element type.
                    sap->addDriver(item.m_rhsp, item.m_idx, flp);
                }
            } இல்லையெனில் {
                item.m_lhsp->v3fatalSrc("Unhandled DfgVertexSplice sub-type");
            }
        }

        திரும்பு உண்மை;
    }

    // VISITORS

    // Unhandled node
    வெறுமை visit(Ast_முனை* /*nodep*/) override {
        எனில் (!m_foundUnhandled && m_converting) ++m_ctx.m_conv.nonRepUnknown;
        m_foundUnhandled = உண்மை;
    }

    // Expressions - mostly auto generated, but a few special ones
    வெறுமை visit(AstVarRef* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        எனில் (unhandled(nodep)) திரும்பு;
        // This visit method is only called on RValues, where only read refs are supported
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Non-ReadOnly reference");
        எனில் (!isSupported(nodep)) {
            m_foundUnhandled = உண்மை;
            ++m_ctx.m_conv.nonRepVarRef;
            திரும்பு;
        }

        // Variable should have been bound before starting conversion
        DfgVertex* மாறிலி vtxp = nodep->varScopep()->user2u().template to<DfgVertexVar*>();
        UASSERT_OBJ(vtxp, nodep, "Referenced variable has no associated DfgVertexVar");
        nodep->user2p(vtxp);
    }
    வெறுமை visit(AstConst* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        எனில் (unhandled(nodep)) திரும்பு;

        எனில் (nodep->width() != nodep->இலக்கம்().width()) {
            // Sometimes the width of the AstConst is not the same as the
            // V3Number it holds. Truncate it here. TODO: should this be allowed?
            V3_எண் இலக்கம்{nodep, nodep->width()};
            இலக்கம்.opSel(nodep->இலக்கம்(), nodep->width() - 1, 0);
            DfgVertex* மாறிலி vtxp = make<DfgConst>(nodep->fileline(), இலக்கம்);
            nodep->user2p(vtxp);
        } இல்லையெனில் {
            DfgVertex* மாறிலி vtxp = make<DfgConst>(nodep->fileline(), nodep->இலக்கம்());
            nodep->user2p(vtxp);
        }
    }
    வெறுமை visit(AstCReset* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        எனில் (unhandled(nodep)) திரும்பு;

        மாறிலி DfgDataType* மாறிலி dtypep = DfgDataType::fromAst(nodep->dtypep());
        எனில் (!dtypep) {
            m_foundUnhandled = உண்மை;
            ++m_ctx.m_conv.nonRepDType;
            திரும்பு;
        }

        UASSERT_OBJ(!nodep->constructing(), nodep,
                    "CReset should be non-constructing at this stage");

        DfgVertex* மாறிலி vtxp = make<DfgCReset>(nodep->fileline(), *dtypep);
        nodep->user2p(vtxp);
    }
    வெறுமை visit(AstMatchMasked* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        எனில் (unhandled(nodep)) திரும்பு;

        மாறிலி DfgDataType* மாறிலி dtypep = DfgDataType::fromAst(nodep->dtypep());
        எனில் (!dtypep) {
            m_foundUnhandled = உண்மை;
            ++m_ctx.m_conv.nonRepDType;
            திரும்பு;
        }

        iterate(nodep->lhsp());
        எனில் (m_foundUnhandled) திரும்பு;
        iterate(nodep->matchp());
        எனில் (m_foundUnhandled) திரும்பு;

        File_வரி* மாறிலி flp = nodep->fileline();
        DfgMatchMasked* மாறிலி vtxp = make<DfgMatchMasked>(flp, *dtypep);
        vtxp->lhsp(nodep->lhsp()->user2u().to<DfgVertex*>());
        vtxp->matchp(nodep->matchp()->user2u().to<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    வெறுமை visit(AstReplicate* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        எனில் (unhandled(nodep)) திரும்பு;

        மாறிலி DfgDataType* மாறிலி dtypep = DfgDataType::fromAst(nodep->dtypep());
        எனில் (!dtypep) {
            m_foundUnhandled = உண்மை;
            ++m_ctx.m_conv.nonRepDType;
            திரும்பு;
        }

        iterate(nodep->srcp());
        எனில் (m_foundUnhandled) திரும்பு;

        File_வரி* மாறிலி flp = nodep->fileline();
        DfgRep* மாறிலி vtxp = make<DfgRep>(flp, *dtypep);
        vtxp->srcp(nodep->srcp()->user2u().to<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    வெறுமை visit(AstSel* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        எனில் (unhandled(nodep)) திரும்பு;

        மாறிலி DfgDataType* மாறிலி dtypep = DfgDataType::fromAst(nodep->dtypep());
        எனில் (!dtypep) {
            m_foundUnhandled = உண்மை;
            ++m_ctx.m_conv.nonRepDType;
            திரும்பு;
        }

        iterate(nodep->fromp());
        எனில் (m_foundUnhandled) திரும்பு;

        File_வரி* மாறிலி flp = nodep->fileline();
        DfgVertex* vtxp = nullptr;
        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->lsbp(), Const)) {
            மாறிலி uint32_t lsb = constp->toUInt();
            மாறிலி uint32_t msb = lsb + nodep->widthConst() - 1;
            DfgVertex* மாறிலி fromp = nodep->fromp()->user2u().to<DfgVertex*>();
            // Unfortunately we can still have out of bounds selects due to how
            // indices are truncated for speed reasons in V3Width/V3Unknown.
            எனில் (msb >= fromp->size()) {
                m_foundUnhandled = உண்மை;
                ++m_ctx.m_conv.nonRepOOBSel;
                திரும்பு;
            }
            DfgSel* மாறிலி selp = make<DfgSel>(flp, *dtypep);
            selp->fromp(fromp);
            selp->lsb(lsb);
            vtxp = selp;
        } இல்லையெனில் {
            iterate(nodep->lsbp());
            எனில் (m_foundUnhandled) திரும்பு;
            DfgMux* மாறிலி muxp = make<DfgMux>(flp, *dtypep);
            muxp->fromp(nodep->fromp()->user2u().to<DfgVertex*>());
            muxp->lsbp(nodep->lsbp()->user2u().to<DfgVertex*>());
            vtxp = muxp;
        }
        nodep->user2p(vtxp);
    }

// The rest of the visit methods for expressions are generated by 'astgen'
#அந்தர்பாவ "V3Dfg__gen_ast_to_dfg.h"

பொது:
    // PUBLIC METHODS

    // Create temporay variable capable of holding the given type
    DfgVertexVar* createTmp(DfgLogic& logic, File_வரி* flp, மாறிலி DfgDataType& dtype,
                            மாறிலி std::string& prefix) {
        DfgVertexVar* மாறிலி vtxp = m_dfg.makeNewVar(flp, prefix, dtype, logic.scopep());
        logic.synth().emplace_back(vtxp);
        vtxp->vscp()->varp()->isInternal(உண்மை);
        vtxp->tmpForp(vtxp->vscp());
        திரும்பு vtxp;
    }

    // Create a new temporary variable capable of holding 'varp'
    DfgVertexVar* createTmp(DfgLogic& logic, AstVarScope* vscp, மாறிலி std::string& prefix) {
        AstVar* மாறிலி astVarp = vscp->varp();
        File_வரி* மாறிலி flp = astVarp->fileline();
        மாறிலி DfgDataType& dtype = *DfgDataType::fromAst(astVarp->dtypep());
        மாறிலி std::string prfx = prefix + "_" + astVarp->name();
        DfgVertexVar* மாறிலி vtxp = createTmp(logic, flp, dtype, prfx);
        vtxp->tmpForp(vscp);
        திரும்பு vtxp;
    }

    // Convert AstAssign to Dfg, return true if successful.
    // Fills 'updates' with bindings for assigned variables.
    தருக்க convert(std::vector<std::pair<AstVarScope*, DfgVertexVar*>>& updates, DfgLogic& vtx,
                 Ast_முனை_Assign* nodep) {
        UASSERT_OBJ(VN_IS(nodep, Assign) || VN_IS(nodep, AssignW), nodep, "Bad NodeAssign");
        UASSERT_OBJ(updates.empty(), nodep, "'updates' should be empty");
        VL_RESTORER(m_updatesp);
        VL_RESTORER(m_logicp);
        m_updatesp = &updates;
        m_logicp = &vtx;
        // Assignment with timing control shouldn't make it this far
        UASSERT_OBJ(!nodep->timingControlp(), nodep, "Shouldn't make it this far");
        // Convert it
        ++m_ctx.m_conv.உள்ளீடு_Assignments;
        Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp();
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp();
        // Check data types are compatible.
        மாறிலி DfgDataType* மாறிலி lDtypep = DfgDataType::fromAst(lhsp->dtypep());
        மாறிலி DfgDataType* மாறிலி rDtypep = DfgDataType::fromAst(rhsp->dtypep());
        எனில் (!lDtypep || !rDtypep) {
            ++m_ctx.m_conv.nonRepDType;
            திரும்பு பொய்;
        }
        // For now, only direct array assignment is supported (e.g. a = b, but not a = _ ? b : c)
        எனில் (rDtypep->isArray()) {
            எனில் (!VN_IS(rhsp, VarRef)
                || !lhsp->dtypep()->skipRefp()->same_மரம்(rhsp->dtypep()->skipRefp())) {
                ++m_ctx.m_conv.nonRepDType;
                திரும்பு பொய்;
            }
        }
        // Widths should match at this point
        UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Mismatched width reached DFG");
        // Convert the RHS expression
        DfgVertex* மாறிலி rVtxp = convertRValue(rhsp);
        எனில் (!rVtxp) திரும்பு பொய்;
        // Connect the RHS vertex to the LHS targets
        மாறிலி தருக்க success = convertAssignment(nodep->fileline(), lhsp, rVtxp);
        எனில் (success) ++m_ctx.m_conv.representable;
        திரும்பு success;
    }

    // Convert RValue expression to Dfg. Returns nullptr if failed.
    DfgVertex* convert(DfgLogic& vtx, Ast_முனை_கோவை* nodep) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;
        // Convert it
        ++m_ctx.m_conv.உள்ளீடு_Expressions;
        DfgVertex* மாறிலி vtxp = convertRValue(nodep);
        எனில் (vtxp) ++m_ctx.m_conv.representable;
        திரும்பு vtxp;
    }

    // CONSTRUCTOR
    AstToDfgConverter(DfgGraph& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {}
};

// Debug aid - outisde 'AstToDfgSynthesize' as it is a template, but want one instance
V3DebugBisect s_dfgSynthDebugBisect{"DfgSynthesize"};

வகுப்பு AstToDfgSynthesize final {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // TYPES

    // SymTab must be ordered in order to yield stable results
    கட்டமைப்பு AstVarScopeComparator final {
        நிலையான முழு s_vscpIdCounter;  // Counter for lazily allocating the unique AstVarScope IDs
        தருக்க operator()(AstVarScope* lhs, AstVarScope* rhs) மாறிலி {
            எனில் (!lhs->user4()) lhs->user4(++s_vscpIdCounter);
            எனில் (!rhs->user4()) rhs->user4(++s_vscpIdCounter);
            திரும்பு lhs->user4() < rhs->user4();
        }
    };
    using SymTab = std::map<AstVarScope*, DfgVertexVar*, AstVarScopeComparator>;

    // Represents a [potentially partial] driver of a variable
    கட்டமைப்பு Driver final {
        DfgVertex* m_vtxp = nullptr;  // Driving vertex
        uint32_t m_lo = 0;  // Low index of driven range (internal, not Verilog)
        uint32_t m_hi = 0;  // High index of driven range (internal, not Verilog)
        File_வரி* m_flp = nullptr;  // Location of driver in source

        Driver() = இயல்பு;
        Driver(DfgVertex* vtxp, uint32_t lo, File_வரி* flp)
            : m_vtxp{vtxp}
            , m_lo{lo}
            , m_hi{lo + vtxp->size() - 1U}
            , m_flp{flp} {}
        operator தருக்க() மாறிலி { திரும்பு m_vtxp != nullptr; }

        தருக்க operator<(மாறிலி Driver& other) மாறிலி {
            எனில் (m_lo != other.m_lo) திரும்பு m_lo < other.m_lo;
            எனில் (m_hi != other.m_hi) திரும்பு m_hi < other.m_hi;
            திரும்பு m_flp->operatorCompare(*other.m_flp) < 0;
        }

        தருக்க operator<=(மாறிலி Driver& other) மாறிலி { திரும்பு !(other < *இது); }
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
    Cfg_Dominator_மரம் m_dom_மரம்;  // The dominator tree of the current CFG

    // STATE - Some debug aid
    // We stop after synthesizing s_dfgSynthDebugLimit vertices (if non-zero).
    // This is the problematic logic (last one we synthesize), assuming a
    // bisection search over s_dfgSynthDebugLimit.
    DfgLogic* m_debugLogicp = nullptr;
    // Source (upstream) cone of outputs of m_debugLogicp
    std::unique_ptr<std::unordered_set<மாறிலி DfgVertex*>> m_debugOSrcConep{nullptr};

    // METHODS

    // Dump current graph for debugging ...
    வெறுமை debugDump(மாறிலி எழுத்து* name) {
        // If we have the debugged logic, compute the vertices feeding its outputs
        எனில் (VL_UNLIKELY(m_debugLogicp)) {
            std::vector<மாறிலி DfgVertex*> outputs;
            m_debugLogicp->foreachSink([&outputs](மாறிலி DfgVertex& v) {
                outputs.emplace_back(v.singleSink()->as<DfgVertexVar>());
                திரும்பு பொய்;
            });
            m_debugOSrcConep = m_dfg.sourceCone(outputs);
        }

        எனில் (VL_UNLIKELY(dumpDfgLevel() >= 9 || m_debugOSrcConep)) {
            m_dfg.dumpDotFilePrefixed(name);
            எனில் (m_debugOSrcConep) {
                // Dump only the subgraph involving the inputs and outputs of the bad vertex
                m_dfg.dumpDotFilePrefixed(name + "-min"s, [&](மாறிலி DfgVertex& v) -> தருக்க {
                    திரும்பு m_debugOSrcConep->மொத்தம்(&v);
                });
            }
        }
    }

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    template <typename Vertex, typename... Args>
    Vertex* make(Args&&... args) {
        static_assert(!std::is_base_of<DfgVertexVar, Vertex>::மதிப்பு, "Do not use for variables");
        static_assert(std::is_base_of<DfgVertex, Vertex>::மதிப்பு, "'Vertex' must be a 'DfgVertex'");
        Vertex* மாறிலி vtxp = புதிய Vertex{m_dfg, std::forward<Args>(args)...};
        எனில் (m_logicp) m_logicp->synth().emplace_back(vtxp);
        திரும்பு vtxp;
    }

    // Gather all drivers of a resolved variable
    நிலையான std::vector<Driver> gatherDrivers(DfgVertexSplice* vtxp) {
        // Collect them all, check if they are sorted
        std::vector<Driver> drivers;
        drivers.reserve(vtxp->nInputs());
        தருக்க sorted = உண்மை;
        vtxp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_வரி* flp) {
            // Collect the driver
            drivers.emplace_back(&src, lo, flp);
            // Check if drivers are sorted - most often they are
            மாறிலி size_t n = drivers.size();
            எனில் (n >= 2 && drivers[n - 1] < drivers[n - 2]) sorted = பொய்;
            திரும்பு பொய்;
        });

        // Sort if unsorted
        எனில் (!sorted) std::stable_sort(drivers.begin(), drivers.end());

        // Done
        திரும்பு drivers;
    }

    // Returns true if the driver supplies a complete array element.
    நிலையான தருக்க driverCoversWholeElement(மாறிலி Driver& driver) {
        மாறிலி DfgUnitArray* மாறிலி unitp = driver.m_vtxp->cast<DfgUnitArray>();
        எனில் (!unitp) திரும்பு பொய்;
        எனில் (DfgVertexSplice* மாறிலி splicep = unitp->srcp()->cast<DfgVertexSplice>()) {
            திரும்பு splicep->wholep();
        }
        திரும்பு உண்மை;
    }

    // Unlike 'wholep', several element drivers can collectively cover an array.
    நிலையான தருக்க spliceCoversWhole(DfgVertexSplice* மாறிலி splicep) {
        எனில் (splicep->wholep()) திரும்பு உண்மை;
        எனில் (splicep->isPacked()) திரும்பு பொய்;
        uint32_t next = 0;
        க்கு (மாறிலி Driver& driver : gatherDrivers(splicep)) {
            எனில் (driver.m_lo != next || !driverCoversWholeElement(driver)) திரும்பு பொய்;
            next = driver.m_hi + 1;
        }
        திரும்பு next == splicep->size();
    }

    // Returns true if the driver cone contains any variable introduced by
    // tristate lowering. Used to distinguish intentional tristate contributor
    // overlap from accidental multidrive.
    நிலையான தருக்க containsTriLoweredVar(DfgVertex* rootp) {
        std::vector<மாறிலி DfgVertex*> அடுக்கு;
        std::vector<மாறிலி DfgVertex*> visited;
        அடுக்கு.emplace_back(rootp);
        போது (!அடுக்கு.empty()) {
            மாறிலி DfgVertex* மாறிலி vtxp = அடுக்கு.back();
            அடுக்கு.எடு_back();
            எனில் (std::find(visited.begin(), visited.end(), vtxp) != visited.end()) தொடர்;
            visited.emplace_back(vtxp);
            எனில் (மாறிலி DfgVertexVar* மாறிலி varp = vtxp->cast<DfgVertexVar>()) {
                எனில் (varp->vscp()->varp()->dfgTriLowered()) திரும்பு உண்மை;
            }
            vtxp->foreachSource([&](மாறிலி DfgVertex& src) {
                அடுக்கு.emplace_back(&src);
                திரும்பு பொய்;
            });
        }
        திரும்பு பொய்;
    }

    // Returns true if the driver is a direct variable forward (no logic).
    நிலையான தருக்க isDirectVarDriver(மாறிலி DfgVertex* vtxp) { திரும்பு vtxp->is<DfgVertexVar>(); }

    // Gather all synthesized drivers of an unresolved variable
    நிலையான std::vector<Driver> gatherDriversUnresolved(DfgUnresolved* vtxp) {
        std::vector<Driver> drivers;
        drivers.reserve(vtxp->nInputs());

        // For better locations in error reporting, we unpick concatenations
        // which are sometimes introduced by combinint assignments in V3Const.
        மாறிலி std::function<வெறுமை(DfgVertex*, uint32_t, File_வரி*)> gather
            = [&](DfgVertex* vtxp, uint32_t lo, File_வரி* flp) -> வெறுமை {
            எனில் (DfgConcat* மாறிலி concatp = vtxp->cast<DfgConcat>()) {
                DfgVertex* மாறிலி rhsp = concatp->rhsp();
                gather(rhsp, lo, rhsp->fileline());
                DfgVertex* மாறிலி lhsp = concatp->lhsp();
                gather(lhsp, lo + rhsp->width(), lhsp->fileline());
                திரும்பு;
            }
            drivers.emplace_back(vtxp, lo, flp);
        };

        // Gather all synthesized drivers
        vtxp->foreachSource([&](DfgVertex& src) {
            // Can ignore the original DfgLogic
            எனில் (src.is<DfgLogic>()) திரும்பு பொய்;
            // Synthesized drivers must be a splice at this point
            DfgVertexSplice* மாறிலி splicep = src.as<DfgVertexSplice>();
            // Collect the driver
            splicep->foreachDriver([&](DfgVertex& src, uint32_t lo, File_வரி* flp) {
                gather(&src, lo, flp);
                திரும்பு பொய்;
            });
            திரும்பு பொய்;
        });

        // Sort the drivers
        std::stable_sort(drivers.begin(), drivers.end());

        // Done
        திரும்பு drivers;
    }

    // Given two drivers, combine the driven sub-ranges into the first one if
    // possible. First bool returned indicates successfully combined and there
    // are no multi-driven bits. Second bool returned indicates we warned
    // already about multi-driven bits.
    std::pair<தருக்க, தருக்க> combineDrivers(DfgVertexVar& var, மாறிலி std::string& sub,  //
                                         Driver& a, மாறிலி Driver& b) {
        // We can only combine array drivers ...
        எனில் (a.m_vtxp->isPacked()) திரும்பு {பொய், பொய்};
        // ... that both drive a single element ...
        எனில் (a.m_lo != b.m_lo) திரும்பு {பொய், பொய்};
        மாறிலி DfgUnitArray* மாறிலி aUap = a.m_vtxp->template cast<DfgUnitArray>();
        எனில் (!aUap) திரும்பு {பொய், பொய்};
        மாறிலி DfgUnitArray* மாறிலி bUap = b.m_vtxp->template cast<DfgUnitArray>();
        எனில் (!bUap) திரும்பு {பொய், பொய்};
        // ... and are themeselves partial
        DfgSplicePacked* மாறிலி aSp = aUap->srcp()->template cast<DfgSplicePacked>();
        எனில் (!aSp) திரும்பு {பொய், பொய்};
        DfgSplicePacked* மாறிலி bSp = bUap->srcp()->template cast<DfgSplicePacked>();
        எனில் (!bSp) திரும்பு {பொய், பொய்};
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
        எனில் (!normalizeDrivers(var, abDrivers, sub + "[" + std::to_string(a.m_lo) + "]")) {
            திரும்பு {பொய், உண்மை};
        }

        // Successfully resolved. Needs a new splice and unit.
        File_வரி* மாறிலி flp = var.fileline();
        DfgSplicePacked* மாறிலி splicep = make<DfgSplicePacked>(flp, aSp->dtype());
        க்கு (மாறிலி Driver& d : abDrivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        DfgUnitArray* மாறிலி uap = make<DfgUnitArray>(flp, aUap->dtype());
        uap->srcp(splicep);
        a.m_vtxp = uap;
        திரும்பு {உண்மை, பொய்};
    }

    // Combine and coalesce the given drivers.
    // Returns true iff no multi-driven bits are present.
    தருக்க normalizeDrivers(DfgVertexVar& var, std::vector<Driver>& drivers,
                          மாறிலி std::string& sub = "") {
        எனில் (drivers.empty()) திரும்பு உண்மை;

        // What type of values are we combining
        மாறிலி தருக்க isPacked = drivers[0].m_vtxp->isPacked();

        // Found a multidriven part ?
        தருக்க multiDriven = பொய்;

        // Iterate through the sorted drivers. Index 'i' is the driver we are
        // resolving driver 'j' agains, and if required, we merge 'j' into 'i'.
        size_t i = 0;
        க்கு (size_t j = 1; j < drivers.size();) {
            UASSERT_OBJ(i < j, &var, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            எனில் (!jD) {
                ++j;
                தொடர்;
            }
            // If 'i' was moved, move 'j' in it's place
            எனில் (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                தொடர்;
            }

            // We have 2 valid drivers now
            UASSERT_OBJ(iD <= jD, &var, "Should always be sorted");
            UASSERT_OBJ(jD.m_vtxp->isPacked() == isPacked, &var, "Mixed type drivers");

            // If no overlap, consider next pair
            எனில் (iD.m_hi < jD.m_lo) {
                ++i;
                எனில் (i == j) ++j;
                தொடர்;
            }

            // There is an overlap. Attempt to combine them.
            தருக்க combined = பொய்;
            தருக்க warned = பொய்;
            std::tie(combined, warned) = combineDrivers(var, sub, iD, jD);

            // If sucessfully combined, 'j' is no longer needed, it was combined into 'i'
            எனில் (combined) {
                jD = Driver{};
                ++j;
                தொடர்;
            }

            // Found overlap that cannot be resolved
            multiDriven = உண்மை;
            // Compare next driver
            ++j;

            // Do not warn again if we warned during 'combineDrivers'
            எனில் (warned) தொடர்;

            // The variable to warn on
            AstVarScope* மாறிலி vscp = var.tmpForp() ? var.tmpForp() : var.vscp();

            // Loop index often abused, so suppress
            எனில் (vscp->varp()->isUsedLoopIdx()) தொடர்;
            // Tristate lowering can intentionally create overlapping contributors.
            // Keep the signal marked multidriven for DFG fallback, but suppress
            // warning only when both overlapping driver cones look tri-lowered.
            எனில் (vscp->varp()->dfgAllowMultidriveTri()) {
                மாறிலி தருக்க iTri = containsTriLoweredVar(iD.m_vtxp);
                மாறிலி தருக்க jTri = containsTriLoweredVar(jD.m_vtxp);
                மாறிலி தருக்க triPair = iTri && jTri;
                மாறிலி தருக்க triAndBridge = (iTri && isDirectVarDriver(jD.m_vtxp))
                                          || (jTri && isDirectVarDriver(iD.m_vtxp));
                எனில் (triPair || triAndBridge) தொடர்;
            }

            // Warn the user now
            மாறிலி std::string lo = std::to_string(jD.m_lo);
            மாறிலி std::string hi = std::to_string(std::min(iD.m_hi, jD.m_hi));
            மாறிலி std::string kind = isPacked ? "Bit" : "Element";
            மாறிலி std::string part = hi == lo ? (" [" + lo + "]") : ("s [" + hi + ":" + lo + "]");

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
        எனில் (!multiDriven && isPacked) coalesceDrivers(drivers);

        திரும்பு !multiDriven;
    }

    // Coalesce adjacent drivers into single ones
    வெறுமை coalesceDrivers(std::vector<Driver>& drivers) {
        UASSERT(!drivers.empty(), "Can't coalesce 0 drivers");
        UASSERT_OBJ(drivers[0].m_vtxp->isPacked(), drivers[0].m_vtxp, "Can only coalesce packed");

        size_t i = 0;
        க்கு (size_t j = 1; j < drivers.size();) {
            UASSERT(i < j, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            எனில் (!jD) {
                ++j;
                தொடர்;
            }
            // If 'i' was moved, move 'j' in it's place
            எனில் (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                தொடர்;
            }

            // We have 2 valid drivers now
            UASSERT(iD <= jD, "Should always be sorted");

            // If not adjacent, move on
            எனில் (iD.m_hi + 1 != jD.m_lo) {
                ++i;
                எனில் (i == j) ++j;
                தொடர்;
            }

            // Coalesce Adjacent ranges,
            மாறிலி DfgDataType& dt = DfgDataType::packed(iD.m_vtxp->width() + jD.m_vtxp->width());
            DfgConcat* மாறிலி concatp = make<DfgConcat>(iD.m_flp, dt);
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
    DfgVertexSplice* makeSplice(DfgVertexVar& var, மாறிலி std::vector<Driver>& newDrivers) {
        UASSERT_OBJ(!newDrivers.empty(), &var, "'makeSplice' called with no new drivers");
        // Create new driver
        DfgVertexSplice* splicep = nullptr;
        எனில் (var.is<DfgVarPacked>()) {
            splicep = make<DfgSplicePacked>(var.fileline(), var.dtype());
        } இல்லையெனில் எனில் (var.is<DfgVarArray>()) {
            splicep = make<DfgSpliceArray>(var.fileline(), var.dtype());
        } இல்லையெனில் {
            var.v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        க்கு (மாறிலி Driver& d : newDrivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        திரும்பு splicep;
    }

    // Initialzie input symbol table of entry CfgBlock
    வெறுமை initialize_Entry_குறியீடு_Table(SymTab& iSymTab) {
        m_logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* மாறிலி vvp = src.as<DfgVertexVar>();
            iSymTab[vvp->vscp()] = vvp;
            திரும்பு பொய்;
        });
    }

    // Join variable drivers across a control flow confluence (insert muxes ...)
    DfgVertexVar* joinDrivers(AstVarScope* vscp, DfgVertexVar* predicatep,  //
                              DfgVertexVar* thenp, DfgVertexVar* elsep) {
        AstVarScope* மாறிலி thenVscp = thenp->tmpForp() ? thenp->tmpForp() : thenp->vscp();
        AstVarScope* மாறிலி elseVscp = elsep->tmpForp() ? elsep->tmpForp() : elsep->vscp();
        UASSERT_OBJ(thenVscp == elseVscp, vscp, "Attempting to join unrelated variables");

        // If both bindings are the the same (variable not updated through either path),
        // then there is nothing to do, can use the same binding
        எனில் (thenp == elsep) திரும்பு thenp;

        // We can't join the input variable just yet, so bail
        எனில் (thenp->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_உள்ளீடு;
            திரும்பு nullptr;
        }
        எனில் (elsep->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_உள்ளீடு;
            திரும்பு nullptr;
        }

        // Can't do arrays yet
        எனில் (!thenp->isPacked()) {
            ++m_ctx.m_synt.nonSynArray;
            திரும்பு nullptr;
        }

        // Gather drivers of 'thenp' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* மாறிலி tDefaultp = thenp->defaultp();
        std::vector<Driver> tDrivers = gatherDrivers(thenp->srcp()->as<DfgVertexSplice>());

        // Gather drivers of 'elsep' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* மாறிலி eDefaultp = elsep->defaultp();
        std::vector<Driver> eDrivers = gatherDrivers(elsep->srcp()->as<DfgVertexSplice>());

        // Default drivers should be the same or not present on either
        UASSERT_OBJ(tDefaultp == eDefaultp, vscp, "Different default drivers");

        // Location to use for the join vertices
        File_வரி* மாறிலி flp = predicatep->fileline();

        // Create a fresh temporary for the joined value
        DfgVertexVar* மாறிலி joinp = m_converter.createTmp(*m_logicp, vscp, "SynthJoin");
        DfgVertexSplice* மாறிலி joinSplicep = make<DfgSplicePacked>(flp, joinp->dtype());
        joinp->srcp(joinSplicep);

        // If both paths are fully driven, just create a simple conditional
        எனில் (tDrivers.size() == 1  //
            && tDrivers[0].m_lo == 0  //
            && tDrivers[0].m_hi == thenp->width() - 1  //
            && eDrivers.size() == 1  //
            && eDrivers[0].m_lo == 0  //
            && eDrivers[0].m_hi == elsep->width() - 1) {
            UASSERT_OBJ(!tDefaultp, vscp, "Fully driven variable have default driver");

            DfgCond* மாறிலி condp = make<DfgCond>(flp, joinp->dtype());
            condp->condp(predicatep);
            condp->thenp(thenp);
            condp->elsep(elsep);
            joinSplicep->addDriver(condp, 0, tDrivers[0].m_flp);

            // Done
            திரும்பு joinp;
        }

        // Otherwise we need to merge them part by part

        // If different bits are driven, then some might not have been assigned.. Latch?
        எனில் (tDrivers.size() != eDrivers.size()) {
            ++m_ctx.m_synt.nonSynLatch;
            திரும்பு nullptr;
        }

        க்கு (size_t i = 0; i < tDrivers.size(); ++i) {
            மாறிலி Driver& tDriver = tDrivers[i];
            மாறிலி Driver& eDriver = eDrivers[i];
            // If different bits are driven, then some might not have been assigned.. Latch?
            எனில் (tDriver.m_lo != eDriver.m_lo || tDriver.m_hi != eDriver.m_hi) {
                ++m_ctx.m_synt.nonSynLatch;
                திரும்பு nullptr;
            }

            மாறிலி DfgDataType& dtype = DfgDataType::packed(tDriver.m_hi - tDriver.m_lo + 1);
            DfgCond* மாறிலி condp = make<DfgCond>(flp, dtype);
            condp->condp(predicatep);

            // We actally need to select the bits from the joined variables, not use the drivers
            DfgSel* மாறிலி thenSelp = make<DfgSel>(flp, tDriver.m_vtxp->dtype());
            thenSelp->lsb(tDriver.m_lo);
            thenSelp->fromp(thenp);
            condp->thenp(thenSelp);

            // Same for the 'else' part
            DfgSel* மாறிலி elseSelp = make<DfgSel>(flp, eDriver.m_vtxp->dtype());
            elseSelp->lsb(eDriver.m_lo);
            elseSelp->fromp(elsep);
            condp->elsep(elseSelp);

            // Add it as a driver to the join
            joinSplicep->addDriver(condp, tDriver.m_lo, tDriver.m_flp);
        }

        // If there was a default driver, add it to te join
        எனில் (tDefaultp) joinp->defaultp(tDefaultp);

        // Done
        திரும்பு joinp;
    }

    // Merge 'thenSymTab' into 'elseSymTab' using the given predicate to join values
    தருக்க join_குறியீடு_Tables(SymTab& elseSymTab, DfgVertexVar* predicatep, மாறிலி SymTab& thenSymTab) {
        // Any variable that does not have a binding on both paths will be removed. These might be
        // temporaries, loop vars, etc used only in one branch. Conversion will fail if the
        // variable is actually referenced later.
        std::vector<AstVarScope*> toRemove;

        // Join each symbol
        க்கு (std::pair<AstVarScope* மாறிலி, DfgVertexVar*>& pair : elseSymTab) {
            AstVarScope* மாறிலி varp = pair.first;
            // Find same variable on the else path
            மாறிலி auto it = thenSymTab.find(varp);
            // Record for removal if not assigned on both paths
            எனில் (it == thenSymTab.end()) {
                toRemove.emplace_back(varp);
                தொடர்;
            }
            // Join paths with the predicate
            DfgVertexVar* மாறிலி thenp = it->second;
            DfgVertexVar* மாறிலி elsep = pair.second;
            DfgVertexVar* மாறிலி newp = joinDrivers(varp, predicatep, thenp, elsep);
            எனில் (!newp) திரும்பு பொய்;
            pair.second = newp;
        }

        // Remove variables not assigned on both paths
        க்கு (AstVarScope* மாறிலி varp : toRemove) elseSymTab.erase(varp);

        // Done
        திரும்பு உண்மை;
    }

    // Given two joining control flow edges, compute how to join their symbols.
    // Returns the predicaete to join over, and the 'then' and 'else' blocks.
    std::tuple<DfgVertexVar*, மாறிலி CfgBlock*, மாறிலி CfgBlock*>  //
    howToJoin(மாறிலி CfgEdge* மாறிலி ap, மாறிலி CfgEdge* மாறிலி bp) {
        // Find the closest common dominator of the two paths
        மாறிலி CfgBlock* மாறிலி domp = m_dom_மரம்.closestCommonDominator(ap->srcp(), bp->srcp());
        // These paths join here, so 'domp' must be a branch, otherwise it's not the closest
        UASSERT_OBJ(domp->isBranch(), domp, "closestCommonDominator is not a branch");

        // The branches of the common dominator
        மாறிலி CfgEdge* மாறிலி takenEdgep = domp->takenEdgep();
        மாறிலி CfgEdge* மாறிலி untknEdgep = domp->untknEdgep();

        // We check if the taken branch dominates the path to either blocks,
        // and if the untaken branch dominates the path to the other block.
        // If so, we can use the branch condition as predicate, otherwise
        // we must use the path predicate as there are ways to get from one
        // branch of the dominator to the other. We need to be careful if
        // either branches are directly to the join block. This is fine,
        // it's as if there was an empty block on that critical edge which
        // is dominated by that path.

        எனில் (takenEdgep == ap || m_dom_மரம்.dominates(takenEdgep->dstp(), ap->srcp())) {
            எனில் (untknEdgep == bp || m_dom_மரம்.dominates(untknEdgep->dstp(), bp->srcp())) {
                // Taken path dominates 'ap' and untaken dominates 'bp', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                திரும்பு std::make_tuple(m_bbToCondp[domp], ap->srcp(), bp->srcp());
            }
        } இல்லையெனில் எனில் (takenEdgep == bp || m_dom_மரம்.dominates(takenEdgep->dstp(), bp->srcp())) {
            எனில் (untknEdgep == ap || m_dom_மரம்.dominates(untknEdgep->dstp(), ap->srcp())) {
                // Taken path dominates 'bp' and untaken dominates 'ap', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                திரும்பு std::make_tuple(m_bbToCondp[domp], bp->srcp(), ap->srcp());
            }
        }

        // The branches don't dominate the joined blocks, must use the path predicate
        ++m_ctx.m_synt.joinUsingPathPredicate;

        // TODO: We could do better here: use the path predicate of the closest
        // cominating blocks, pick the one from the lower rank, etc, but this
        // generic case is very rare, most synthesizable logic has
        // series-parallel CFGs which are covered by the earlier cases.
        திரும்பு std::make_tuple(m_edgeToPredicatep[ap], ap->srcp(), bp->srcp());
    }

    // Combine the output symbol tables of the predecessors of the given
    // block to compute the input symtol table for the given block.
    தருக்க create_உள்ளீடு_குறியீடு_Table(மாறிலி CfgBlock& bb) {
        // The input symbol table of the given block, we are computing it now
        SymTab& joined = m_bbToISymTab[bb];

        // Input symbol table of entry block is special
        எனில் (bb.isEnter()) {
            initialize_Entry_குறியீடு_Table(joined);
            திரும்பு உண்மை;
        }

        // Current input symbol table should be empty, we will fill it in here
        UASSERT(joined.empty(), "Unprocessed input symbol table should be empty");

        // Fast path if there is only one predecessor - TODO: use less copying
        எனில் (!bb.isJoin()) {
            joined = m_bbToOSymTab[bb.firstPredecessorp()];
            திரும்பு உண்மை;
        }

        // We also have a simpler job if there are 2 predecessors
        எனில் (bb.isTwoWayJoin()) {
            DfgVertexVar* predicatep = nullptr;
            மாறிலி CfgBlock* thenp = nullptr;
            மாறிலி CfgBlock* elsep = nullptr;
            std::tie(predicatep, thenp, elsep)
                = howToJoin(bb.firstPredecessorEdgep(), bb.lastPredecessorEdgep());
            // Copy from else
            joined = m_bbToOSymTab[elsep];
            // Join with then
            திரும்பு join_குறியீடு_Tables(joined, predicatep, m_bbToOSymTab[*thenp]);
        }

        // General hard way

        // Gather predecessors
        கட்டமைப்பு Predecessor final {
            மாறிலி CfgBlock* m_bbp;  // Predeccessor block
            DfgVertexVar* m_predicatep;  // Predicate predecessor reached this block with
            மாறிலி SymTab* m_oSymTabp;  // Output symbol table or predecessor
            Predecessor() = delete;
            Predecessor(மாறிலி CfgBlock* bbp, DfgVertexVar* predicatep, மாறிலி SymTab* oSymTabp)
                : m_bbp{bbp}
                , m_predicatep{predicatep}
                , m_oSymTabp{oSymTabp} {}
        };

        மாறிலி std::vector<Predecessor> predecessors = [&]() {
            std::vector<Predecessor> res;
            க்கு (மாறிலி V3GraphEdge& edge : bb.inEdges()) {
                மாறிலி CfgEdge& cfgEdge = static_cast<மாறிலி CfgEdge&>(edge);
                மாறிலி CfgBlock* மாறிலி predecessorp = cfgEdge.srcp();
                DfgVertexVar* மாறிலி predicatep = m_edgeToPredicatep[cfgEdge];
                மாறிலி SymTab* மாறிலி oSymTabp = &m_bbToOSymTab[predecessorp];
                res.emplace_back(predecessorp, predicatep, oSymTabp);
            }
            // Sort predecessors reverse topologically. This way earlier blocks
            // will come after later blocks, and the entry block is last if present.
            std::sort(res.begin(), res.end(), [](மாறிலி Predecessor& a, மாறிலி Predecessor& b) {  //
                திரும்பு *a.m_bbp > *b.m_bbp;
            });
            திரும்பு res;
        }();

        // Start by copying the bindings from the frist predecessor
        joined = *predecessors[0].m_oSymTabp;
        // Join over all other predecessors
        க்கு (size_t i = 1; i < predecessors.size(); ++i) {
            DfgVertexVar* மாறிலி predicatep = predecessors[i].m_predicatep;
            மாறிலி SymTab& oSymTab = *predecessors[i].m_oSymTabp;
            எனில் (!join_குறியீடு_Tables(joined, predicatep, oSymTab)) திரும்பு பொய்;
        }

        திரும்பு உண்மை;
    }

    std::vector<Driver> computePropagatedDrivers(மாறிலி std::vector<Driver>& newDrivers,
                                                 DfgVertexVar* oldp) {
        // Gather drivers of 'oldp' - they are in incresing range order with no overlaps
        மாறிலி std::vector<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.empty(), oldp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::vector<Driver> propagatedDrivers;

        // Add bits between 'msb' and 'lsb' from 'oldp' to 'pDrivers'
        மாறிலி auto addOldDriver = [&](File_வரி* மாறிலி flp, uint32_t msb, uint32_t lsb) {
            UASSERT_OBJ(propagatedDrivers.empty() || lsb > propagatedDrivers.back().m_hi, flp,
                        "Drivers should be in ascending order");
            DfgSel* மாறிலி selp = make<DfgSel>(flp, DfgDataType::packed(msb - lsb + 1));
            selp->lsb(lsb);
            selp->fromp(oldp);
            propagatedDrivers.emplace_back(selp, lsb, flp);
        };

        // Incorporate old drivers
        க்கு (மாறிலி Driver& oDriver : oldDrivers) {
            File_வரி* மாறிலி flp = oDriver.m_flp;
            // Range to consider inserting, we will adjust oldLo as we process drivers
            uint32_t oldLo = oDriver.m_lo;
            மாறிலி uint32_t oldHi = oDriver.m_hi;

            // Loop for now, can move to bisection search if this is a problem, shouldn't be ...
            க்கு (மாறிலி Driver& nDriver : newDrivers) {
                UASSERT_OBJ(oldHi >= oldLo, flp, "Should have stopped iteration");
                // If new driver is entirely below old driver, move on to
                எனில் (nDriver.m_hi < oldLo) தொடர்;
                // If new driver is entirely above old driver, we can stop
                எனில் (oldHi < nDriver.m_lo) நிறுத்து;

                // There is an overlap between 'oDriver' and 'nDriver'.
                // Insert the low bits and adjust the insertion range.
                // The rest will take care of itself on subsequent iterations.
                எனில் (oldLo < nDriver.m_lo) addOldDriver(flp, nDriver.m_lo - 1, oldLo);
                oldLo = nDriver.m_hi + 1;

                // Stop if no more bits remaining in the old driver
                எனில் (oldLo > oldHi) நிறுத்து;
            }

            // Insert remaining bits if any
            எனில் (oldHi >= oldLo) addOldDriver(flp, oldHi, oldLo);
        }

        திரும்பு propagatedDrivers;
    }

    // Propagate whole elements with a linear walk over the sorted driver lists.
    // Reads use the previous value temporary, preserving assignment-version bindings.
    // Partial elements and non-unit drivers retain the nonsynthesized-process fallback.
    தருக்க computePropagatedArrayDrivers(மாறிலி std::vector<Driver>& newDrivers,
                                       DfgVertexVar* மாறிலி oldp,
                                       std::vector<Driver>& propagatedDrivers) {
        // Bound quadratic vertex growth: array drivers cannot be coalesced.
        நிலையான constexpr uint32_t MAX_ARRAY_ELEMENTS = 32;
        எனில் (oldp->size() > MAX_ARRAY_ELEMENTS) திரும்பு பொய்;
        க்கு (மாறிலி Driver& driver : newDrivers) {
            எனில் (!driverCoversWholeElement(driver)) திரும்பு பொய்;
        }
        மாறிலி std::vector<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.empty(), oldp, "Should have a proper driver");
        க்கு (மாறிலி Driver& driver : oldDrivers) {
            எனில் (!driverCoversWholeElement(driver)) திரும்பு பொய்;
        }

        propagatedDrivers.reserve(oldDrivers.size());
        auto nIt = newDrivers.begin();
        க்கு (மாறிலி Driver& oDriver : oldDrivers) {
            போது (nIt != newDrivers.end() && nIt->m_lo < oDriver.m_lo) ++nIt;
            எனில் (nIt != newDrivers.end() && nIt->m_lo == oDriver.m_lo) தொடர்;

            File_வரி* மாறிலி flp = oDriver.m_flp;
            மாறிலி DfgUnitArray* மாறிலி oldUnitp = oDriver.m_vtxp->as<DfgUnitArray>();
            DfgArraySel* மாறிலி selp = make<DfgArraySel>(flp, oldUnitp->srcp()->dtype());
            selp->fromp(oldp);
            selp->bitp(make<DfgConst>(flp, static_cast<size_t>(VL_IDATASIZE), oDriver.m_lo));
            DfgUnitArray* மாறிலி newUnitp = make<DfgUnitArray>(flp, oldUnitp->dtype());
            newUnitp->srcp(selp);
            propagatedDrivers.emplace_back(newUnitp, oDriver.m_lo, flp);
        }
        திரும்பு உண்மை;
    }

    // Given the drivers of a variable after converting a single statement
    // 'newp', add drivers from 'oldp' that were not reassigned be drivers
    // in newp. This computes the total result of all previous assignments.
    தருக்க incorporate_Previous_மதிப்பு(AstVarScope* vscp, DfgVertexVar* newp, DfgVertexVar* oldp) {
        UASSERT_OBJ(newp->srcp(), vscp, "Assigned variable has no driver");

        // Easy if there is no old value...
        எனில் (!oldp) திரும்பு உண்மை;

        // New driver was not yet coalesced, so should always be a splice
        DfgVertexSplice* மாறிலி nSplicep = newp->srcp()->as<DfgVertexSplice>();

        // If the old value is the real variable we just computed the new value for,
        // then it is the circular feedback into the synthesized block, add it as default driver.
        எனில் (oldp->vscp() == vscp) {
            எனில் (!spliceCoversWhole(nSplicep)) newp->defaultp(oldp);
            திரும்பு உண்மை;
        }

        UASSERT_OBJ(oldp->srcp(), vscp, "Previously assigned variable has no driver");

        // Gather drivers of 'newp' - they are in incresing range order with no overlaps
        UASSERT_OBJ(!newp->defaultp(), newp, "Converted value should not have default");
        std::vector<Driver> nDrivers = gatherDrivers(newp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!nDrivers.empty(), newp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::vector<Driver> pDrivers;
        எனில் (newp->isPacked()) {
            pDrivers = computePropagatedDrivers(nDrivers, oldp);
        } இல்லையெனில் எனில் (!computePropagatedArrayDrivers(nDrivers, oldp, pDrivers)) {
            ++m_ctx.m_synt.nonSynArray;
            திரும்பு பொய்;
        }

        எனில் (!pDrivers.empty()) {
            // Need to merge propagated sources, so reset the splice
            nSplicep->resetDrivers();
            // Merge drivers - they are both sorted and non-overlapping
            std::vector<Driver> drivers;
            drivers.reserve(nDrivers.size() + pDrivers.size());
            std::merge(nDrivers.begin(), nDrivers.end(), pDrivers.begin(), pDrivers.end(),
                       std::back_inserter(drivers));
            // Coalesce adjacent ranges
            எனில் (newp->isPacked()) coalesceDrivers(drivers);
            // Reinsert drivers in order
            க்கு (மாறிலி Driver& d : drivers) nSplicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        }

        // If the old had a default, add to the new one too, unless redundant
        எனில் (oldp->defaultp() && !spliceCoversWhole(nSplicep)) newp->defaultp(oldp->defaultp());

        // Done
        திரும்பு உண்மை;
    }

    // Synthesize the given statements with the given input symbol table.
    // Returns true if successfolly synthesized.
    // Populates the given output symbol table.
    // Populates the given reference with the condition of the terminator branch, if any.
    தருக்க synthesizeBasicBlock(SymTab& oSymTab, DfgVertex*& condpr,
                              மாறிலி std::vector<Ast_முனை_Stmt*>& stmtps, மாறிலி SymTab& iSymTab) {
        // Use fresh set of vertices in m_converter
        மாறிலி VNUser2InUse user2InUse;

        // Initialize AstVarScope -> Vertex bindings available in this block
        க்கு (மாறிலி auto& pair : iSymTab) {
            AstVarScope* மாறிலி varp = pair.first;
            DfgVertexVar* மாறிலி vtxp = pair.second;
            varp->user2p(vtxp);
            oSymTab[varp] = vtxp;
        }

        // Synthesize each statement one after the other
        std::vector<std::pair<AstVarScope*, DfgVertexVar*>> updates;
        க்கு (Ast_முனை_Stmt* மாறிலி stmtp : stmtps) {
            // Regular statements
            Ast_முனை_Assign* மாறிலி ap = VN_CAST(stmtp, முனை_Assign);
            எனில் (ap && (VN_IS(ap, Assign) || VN_IS(ap, AssignW))) {
                // Convert this assignment
                எனில் (!m_converter.convert(updates, *m_logicp, ap)) {
                    ++m_ctx.m_synt.nonSynConv;
                    திரும்பு பொய்;
                }
                // Apply variable updates from this statement
                க்கு (மாறிலி auto& pair : updates) {
                    // The target variable that was assigned to
                    AstVarScope* மாறிலி vscp = pair.first;
                    // The new, potentially partially assigned value
                    DfgVertexVar* மாறிலி newp = pair.second;
                    // Normalize drivers within this statement, bail if multidriven
                    DfgVertexSplice* மாறிலி srcp = newp->srcp()->as<DfgVertexSplice>();
                    std::vector<Driver> drivers = gatherDrivers(srcp);
                    மாறிலி தருக்க single = drivers.size() == 1;
                    எனில் (!normalizeDrivers(*newp, drivers)) {
                        ++m_ctx.m_synt.nonSynMultidrive;
                        திரும்பு பொய்;
                    }
                    // If there were more than one driver (often not), re-add in case coalesced
                    எனில் (!single) {
                        srcp->resetDrivers();
                        க்கு (மாறிலி Driver& d : drivers) srcp->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
                    }
                    // The old value, if any
                    DfgVertexVar* மாறிலி oldp = vscp->user2u().template to<DfgVertexVar*>();
                    // Inncorporate old value into the new value
                    எனில் (!incorporate_Previous_மதிப்பு(vscp, newp, oldp)) திரும்பு பொய்;
                    // Update binding of target variable
                    vscp->user2p(newp);
                    // Update output symbol table of this block
                    oSymTab[vscp] = newp;
                }
                updates.clear();
                தொடர்;
            }

            // Terminator branches
            எனில் (AstIf* மாறிலி ifp = VN_CAST(stmtp, If)) {
                UASSERT_OBJ(ifp == stmtps.back(), ifp, "Branch should be last statement");
                // Convert condition, give up if failed
                DfgVertex* condp = m_converter.convert(*m_logicp, ifp->condp());
                எனில் (!condp) {
                    ++m_ctx.m_synt.nonSynConv;
                    திரும்பு பொய்;
                }
                // Single bit condition can be use directly, otherwise: use 'condp != 0'
                எனில் (condp->width() != 1) {
                    File_வரி* மாறிலி flp = condp->fileline();
                    DfgNeq* மாறிலி neqp = make<DfgNeq>(flp, DfgDataType::packed(1));
                    neqp->lhsp(make<DfgConst>(flp, condp->width(), 0U));
                    neqp->rhsp(condp);
                    condp = neqp;
                }
                condpr = condp;
                தொடர்;
            }

            // Unhandled
            ++m_ctx.m_synt.nonSynStmt;
            திரும்பு பொய்;
        }

        திரும்பு உண்மை;
    }

    // Assign path predicates to the outgoing control flow edges of the given block
    வெறுமை assignPathPredicates(மாறிலி CfgBlock& bb) {
        // Nothing to do for the exit block
        எனில் (bb.isExit()) திரும்பு;

        // Get the predicate of this block
        DfgVertex* மாறிலி predp = [&]() -> DfgVertex* {
            // Entry block has no predecessors, use constant true
            எனில் (bb.isEnter()) திரும்பு make<DfgConst>(m_logicp->fileline(), 1U, 1U);

            // For any other block, 'or' together all the incoming predicates
            மாறிலி auto& inEdges = bb.inEdges();
            auto it = inEdges.begin();
            DfgVertex* resp = m_edgeToPredicatep[static_cast<மாறிலி CfgEdge&>(*it)];
            போது (++it != inEdges.end()) {
                DfgOr* மாறிலி orp = make<DfgOr>(resp->fileline(), resp->dtype());
                orp->rhsp(resp);
                orp->lhsp(m_edgeToPredicatep[static_cast<மாறிலி CfgEdge&>(*it)]);
                resp = orp;
            }
            திரும்பு resp;
        }();

        மாறிலி DfgDataType& dtype = predp->dtype();

        மாறிலி auto mkTmp = [&](File_வரி* flp, மாறிலி எழுத்து* name, DfgVertex* srcp) {
            மாறிலி std::string prefix = "_BB" + std::to_string(bb.id()) + "_" + name;
            DfgVertexVar* மாறிலி tmpp = m_converter.createTmp(*m_logicp, flp, dtype, prefix);
            tmpp->srcp(srcp);
            திரும்பு tmpp;
        };

        // Assign it to a variable in case it's used multiple times
        DfgVertexVar* மாறிலி pInp = mkTmp(predp->fileline(), "PathIn", predp);

        // For uncondional branches, the successor predicate edge is the same
        எனில் (!bb.isBranch()) {
            m_edgeToPredicatep[bb.takenEdgep()] = mkTmp(pInp->fileline(), "Goto", pInp);
            திரும்பு;
        }

        // For branches, we need to factor in the branch condition
        DfgVertex* மாறிலி condp = m_bbToCondp[bb];
        File_வரி* மாறிலி flp = condp->fileline();

        // Predicate for taken branch: 'predp & condp'
        {
            DfgAnd* மாறிலி takenPredp = make<DfgAnd>(flp, dtype);
            takenPredp->lhsp(pInp);
            takenPredp->rhsp(condp);
            m_edgeToPredicatep[bb.takenEdgep()] = mkTmp(flp, "Taken", takenPredp);
        }

        // Predicate for untaken branch: 'predp & ~condp'
        {
            DfgAnd* மாறிலி untknPredp = make<DfgAnd>(flp, dtype);
            untknPredp->lhsp(pInp);
            DfgNot* மாறிலி notp = make<DfgNot>(flp, dtype);
            notp->srcp(condp);
            untknPredp->rhsp(notp);
            m_edgeToPredicatep[bb.untknEdgep()] = mkTmp(flp, "Untkn", untknPredp);
        }
    }

    // Returns true if all external updates to volatile variables are observed correctly
    தருக்க checkExtWrites() {
        க்கு (மாறிலி DfgVertex* மாறிலி vtxp : m_logicp->synth()) {
            மாறிலி DfgVertexVar* மாறிலி varp = vtxp->cast<DfgVertexVar>();
            எனில் (!varp) தொடர்;
            // If the variable we synthesized this vertex for is volatile, and
            // the value of the synthesized temporary is observed, we might be
            // missing an external update, so we mut give up.
            எனில் (!varp->hasSinks()) தொடர்;
            எனில் (!DfgVertexVar::isVolatile(varp->tmpForp())) தொடர்;
            ++m_ctx.m_synt.nonSynExtWrite;
            திரும்பு பொய்;
        }
        திரும்பு உண்மை;
    }

    // Add the synthesized values as drivers to the output variables of the current DfgLogic
    தருக்க addSynthesizedOutput(SymTab& oSymTab) {
        // It's possible we think a variable is written by the DfgLogic when
        // it actauly isn't, e.g.: '{a[0], b[0]}[1] = ...' does not write 'b'.
        // These LHS forms can happen after some earlier tranforms. We
        // should just run V3Const on them earlier, but we will do belt and
        // braces and check here too. We can't touch any output variables if so.
        மாறிலி தருக்க missing = m_logicp->foreachSink([&](மாறிலி DfgVertex& sink) {
            மாறிலி DfgUnresolved* மாறிலி unresolvedp = sink.as<DfgUnresolved>();
            AstVarScope* மாறிலி vscp = unresolvedp->singleSink()->as<DfgVertexVar>()->vscp();
            திரும்பு !oSymTab.மொத்தம்(vscp);
        });
        எனில் (missing) {
            ++m_ctx.m_synt.nonSynFalseWrite;
            திரும்பு பொய்;
        }

        // Add sinks to read the computed values for the target variables
        திரும்பு !m_logicp->foreachSink([&](DfgVertex& sink) {
            DfgUnresolved* மாறிலி unresolvedp = sink.as<DfgUnresolved>();
            மாறிலி DfgVertexVar* மாறிலி varp = unresolvedp->singleSink()->as<DfgVertexVar>();
            DfgVertexVar* மாறிலி resp = oSymTab.at(varp->vscp());
            UASSERT_OBJ(resp->srcp(), resp, "Undriven result");

            // If the output is not used further in the synthesized logic itself,
            // then resp will be deleted before we return, so we can just use
            // its splice directly without ending up with a multi-use operation.
            எனில் (!resp->hasSinks()) {
                unresolvedp->addDriver(resp->srcp()->as<DfgVertexSplice>());
                திரும்பு பொய்;  // OK, continue.
            }

            // TODO: computePropagatedDrivers cannot handle arrays, should
            // never happen with simple continous assignments
            எனில் (!resp->isPacked()) {
                ++m_ctx.m_synt.nonSynArray;
                திரும்பு உண்மை;  // Not OK, give up
            }

            // We need to add a new splice to avoid multi-use of the original splice
            DfgSplicePacked* மாறிலி splicep
                = புதிய DfgSplicePacked{m_dfg, resp->fileline(), resp->dtype()};
            // Drivers are the same
            மாறிலி std::vector<Driver> drivers = computePropagatedDrivers({}, resp);
            க்கு (மாறிலி Driver& d : drivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
            unresolvedp->addDriver(splicep);
            திரும்பு பொய்;  // OK, continue
        });
    }

    // Synthesize the given AstAssignW. Returns true on success.
    தருக்க synthesizeAssignW(AstAssignW* nodep) {
        ++m_ctx.m_synt.உள்ளீடு_Assign;

        // The input and output symbol tables
        SymTab iSymTab;
        SymTab oSymTab;

        // Initialzie input symbol table
        initialize_Entry_குறியீடு_Table(iSymTab);

        // Synthesize as if it was in a single CfgBlock CFG
        DfgVertex* condp = nullptr;
        மாறிலி தருக்க success = synthesizeBasicBlock(oSymTab, condp, {nodep}, iSymTab);
        UASSERT_OBJ(!condp, nodep, "Conditional AstAssignW ???");
        எனில் (!success) திரும்பு பொய்;

        // Check exernal writes are observed correctly
        எனில் (!checkExtWrites()) திரும்பு பொய்;

        // Add resolved output variable drivers
        திரும்பு addSynthesizedOutput(oSymTab);
    }

    // Synthesize the given AstAlways. Returns true on success.
    தருக்க synthesizeCfg(CfgGraph& cfg) {
        ++m_ctx.m_synt.உள்ளீடு_Always;

        // If there is a backward edge (loop), we can't synthesize it
        எனில் (cfg.containsLoop()) {
            ++m_ctx.m_synt.nonSynLoop;
            ++m_ctx.m_synt.cfgCyclic;
            திரும்பு பொய்;
        }

        // If it's a trivial CFG we can save on some work
        எனில் (cfg.nBlocks() == 1) {
            ++m_ctx.m_synt.cfgTrivial;
        } இல்லையெனில் {
            // Insert two-way join blocks to aid multiplexer ordering
            எனில் (cfg.செருகு_Two_Way_Joins()) {
                ++m_ctx.m_synt.cfgSp;
            } இல்லையெனில் {
                ++m_ctx.m_synt.cfgDag;
            }
            // Initialize maps needed for non-trivial CFGs
            m_dom_மரம் = Cfg_Dominator_மரம்{cfg};
            m_edgeToPredicatep = cfg.makeEdgeMap<DfgVertexVar*>();
        }

        // Initialize CfgMaps
        m_bbToISymTab = cfg.makeBlockMap<SymTab>();
        m_bbToOSymTab = cfg.makeBlockMap<SymTab>();
        m_bbToCondp = cfg.makeBlockMap<DfgVertexVar*>();

        // Synthesize all blocks
        க்கு (மாறிலி V3GraphVertex& vtx : cfg.vertices()) {
            மாறிலி CfgBlock& bb = static_cast<மாறிலி CfgBlock&>(vtx);
            // Prepare the input symbol table of this block (enter, or join predecessor blocks)
            எனில் (!create_உள்ளீடு_குறியீடு_Table(bb)) திரும்பு பொய்;
            // Synthesize this block
            DfgVertex* condp = nullptr;
            எனில் (!synthesizeBasicBlock(m_bbToOSymTab[bb], condp, bb.stmtps(), m_bbToISymTab[bb])) {
                திரும்பு பொய்;
            }
            // Create a temporary for the branch condition as it might be used multiple times
            எனில் (condp) {
                File_வரி* மாறிலி flp = condp->fileline();
                மாறிலி DfgDataType& dtype = condp->dtype();
                மாறிலி std::string prefix = "_BB" + std::to_string(bb.id()) + "_Cond";
                DfgVertexVar* மாறிலி vp = m_converter.createTmp(*m_logicp, flp, dtype, prefix);
                vp->srcp(condp);
                m_bbToCondp[bb] = vp;
            }
            // Set the path predicates on the successor edges
            assignPathPredicates(bb);
        }

        // Check exernal writes are observed correctly
        எனில் (!checkExtWrites()) திரும்பு பொய்;

        // Add resolved output variable drivers
        திரும்பு addSynthesizedOutput(m_bbToOSymTab[cfg.exit()]);
    }

    // Synthesize a DfgLogic into regular vertices. Returns ture on success.
    தருக்க synthesize(DfgLogic& vtx) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;

        எனில் (AstAssignW* மாறிலி ap = VN_CAST(vtx.nodep()->stmtsp(), AssignW)) {
            எனில் (ap->nextp()) திரும்பு பொய்;
            எனில் (!synthesizeAssignW(ap)) திரும்பு பொய்;
            ++m_ctx.m_synt.synthAssign;
            திரும்பு உண்மை;
        }

        எனில் (!synthesizeCfg(vtx.cfg())) திரும்பு பொய்;
        ++m_ctx.m_synt.synthAlways;
        திரும்பு உண்மை;
    }

    // Revert all DfgLogic in m_toRevert, or DfgLogic driving the DfgUnresolved
    // vertices in m_toRevert, and transitively the same for any DfgUnresolved
    // driven by the reverted DfgLogic. Delete all DfgUnresolved involed.
    வெறுமை revert(VDouble0& statCountr) {
        m_toRevert.foreach([&](DfgVertex& vtx) {
            // Process DfgLogic
            எனில் (DfgLogic* மாறிலி vtxp = vtx.cast<DfgLogic>()) {
                UASSERT_OBJ(vtxp->selectedForSynthesis(), vtxp, "Shouldn't reach here unselected");
                // Revert this logic
                UASSERT_OBJ(!vtxp->reverted(), vtxp, "Should be reverting now");
                ++statCountr;
                க்கு (DfgVertex* மாறிலி p : vtxp->synth()) VL_DO_DANGLING(p->unlinkDelete(m_dfg), p);
                vtxp->synth().clear();
                vtxp->setReverted();
                // Add all DfgUnresolved it drives to the work list
                vtxp->foreachSink([&](DfgVertex& snk) {
                    m_toRevert.தள்ளு_front(*snk.as<DfgUnresolved>());
                    திரும்பு பொய்;
                });
                திரும்பு;
            }

            // Process DfgUnresolved
            எனில் (DfgUnresolved* மாறிலி vtxp = vtx.cast<DfgUnresolved>()) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->setHasModWrRefs();
                // Add all driving DfgLogic to the work list
                vtxp->foreachSource([&](DfgVertex& src) {
                    DfgLogic* மாறிலி lp = src.cast<DfgLogic>();
                    எனில் (lp && !lp->reverted()) m_toRevert.தள்ளு_front(*lp);
                    திரும்பு பொய்;
                });
                // Delete the DfgUnresolved driver
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
                திரும்பு;
            }

            // There should be nothing else on the worklist
            vtx.v3fatalSrc("Unexpected vertex type");
        });
    }

    // Synthesize all of the given vertices
    வெறுமை முதன்மை() {
        //-------------------------------------------------------------------
        UINFO(5, "Step 0: Remove all DfgLogic not selected for synthesis");
        க்கு (DfgVertex* மாறிலி vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgUnresolved
            எனில் (!vtxp->is<DfgUnresolved>()) தொடர்;
            தருக்க anySelected = பொய்;
            தருக்க anyUnselected = பொய்;
            vtxp->foreachSource([&](DfgVertex& src) {
                மாறிலி DfgLogic& logic = *src.as<DfgLogic>();
                எனில் (logic.selectedForSynthesis()) {
                    anySelected = உண்மை;
                } இல்லையெனில் {
                    anyUnselected = உண்மை;
                }
                திரும்பு பொய்;
            });
            // There should be a driver
            UASSERT_OBJ(anySelected || anyUnselected, vtxp, "'DfgUnresolved' with no driver");
            // All drivers should be selected or all should be unselected
            UASSERT_OBJ(!(anySelected && anyUnselected), vtxp, "Invalid 'DfgLogic' selection");
            // If all drivers are unselected, delete this DfgUnresolved here
            எனில் (anyUnselected) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->setHasModWrRefs();
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
        க்கு (DfgVertex* மாறிலி vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgLogic
            DfgLogic* மாறிலி logicp = vtxp->cast<DfgLogic>();
            எனில் (!logicp) தொடர்;
            எனில் (logicp->selectedForSynthesis()) தொடர்;
            // There should be no sinks left for unselected DfgLogic, delete them here
            UASSERT_OBJ(!logicp->hasSinks(), vtxp, "Unselected 'DfgLogic' with sinks remaining");
            // Input variables will be read in Ast code, add Ast reference vertices
            // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
            மாறிலி VNUser4InUse m_user4InUse;
            logicp->foreachSource([](DfgVertex& src) {
                src.as<DfgVertexVar>()->vscp()->user4p(&src);
                திரும்பு பொய்;
            });
            V3DfgPasses::addAstRefs(m_dfg, logicp->nodep(), [](Ast_முனை* varp) {  //
                திரும்பு varp->user4u().to<DfgVertexVar*>();
            });
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        debugDump("synth-selected");

        //-------------------------------------------------------------------
        UINFO(5, "Step 1: Attempting to synthesize each of the selected DfgLogic");
        {
            // AstVarScope::user4() -> int: unique ID for 'AstVarScopeComparator'
            மாறிலி VNUser4InUse user4InUse;
            க்கு (DfgVertex& vtx : m_dfg.opVertices()) {
                DfgLogic* மாறிலி logicp = vtx.cast<DfgLogic>();
                எனில் (!logicp) தொடர்;

                // We should only have DfgLogic remaining that was selected for synthesis
                UASSERT_OBJ(logicp->selectedForSynthesis(), logicp, "Unselected DfgLogic remains");

                // Debug aid
                மாறிலி auto debugCallback = [&]() -> வெறுமை {
                    // This is the breaking logic
                    m_debugLogicp = logicp;
                    // Dump it
                    UINFOTREE(0, logicp->nodep(), "Problematic DfgLogic: " << logicp, "  ");
                    V3EmitV::debug_Verilog_For_மரம்(logicp->nodep(), std::cout);
                    debugDump("synth-lastok");
                };
                எனில் (VL_UNLIKELY(s_dfgSynthDebugBisect.stop(debugCallback))) நிறுத்து;

                // Synthesize it, if failed, enqueue for reversion
                எனில் (!synthesize(*logicp)) {
                    logicp->setNonSynthesizable();
                    m_toRevert.தள்ளு_front(*logicp);
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
        std::unordered_map<மாறிலி DfgVertexVar*, DfgVertexSplice*> resolvedDrivers;
        // Compute resolved drivers of all variablees
        க்கு (DfgVertexVar& var : m_dfg.varVertices()) {
            எனில் (!var.srcp()) தொடர்;
            DfgUnresolved* மாறிலி unresolvedp = var.srcp()->cast<DfgUnresolved>();
            எனில் (!unresolvedp) நிறுத்து;  // Stop when reached the synthesized temporaries

            // Resolve the synthesized drivers
            DfgVertexSplice* மாறிலி resolvedp = [&]() -> DfgVertexSplice* {
                // All synthesized drivers were normalized already,
                // so if there is only one, it can be used directly
                எனில் (unresolvedp->nInputs() == 1) {
                    திரும்பு unresolvedp->inputp(0)->as<DfgVertexSplice>();
                }
                // Otherwise gather the synthesized drivers
                std::vector<Driver> drivers = gatherDriversUnresolved(unresolvedp);
                // Normalize them, make resolved driver if all good
                எனில் (normalizeDrivers(var, drivers)) திரும்பு makeSplice(var, drivers);
                // If mutlidriven, record and ignore
                multidrivenps.emplace_back(&var);
                m_toRevert.தள்ளு_front(*unresolvedp);
                திரும்பு nullptr;
            }();
            // Bail if multidriven
            எனில் (!resolvedp) தொடர்;
            // Add to map for next loop
            மாறிலி தருக்க newEntry = resolvedDrivers.emplace(&var, resolvedp).second;
            UASSERT_OBJ(newEntry, &var, "Duplicate driver");
        }
        // Revert and remove drivers of multi-driven variables
        revert(m_ctx.m_synt.revertMultidrive);
        // Replace all DfgUnresolved with the resolved drivers
        க்கு (மாறிலி DfgVertexVar& var : m_dfg.varVertices()) {
            எனில் (!var.srcp()) தொடர்;
            DfgUnresolved* மாறிலி srcp = var.srcp()->cast<DfgUnresolved>();
            எனில் (!srcp) நிறுத்து;  // Stop when reached the synthesized temporaries

            // Replace it
            srcp->replaceWith(resolvedDrivers.at(&var));
            VL_DO_DANGLING(srcp->unlinkDelete(m_dfg), srcp);
        }
        debugDump("synth-resolved");

        //-------------------------------------------------------------------
        UINFO(5, "Step 4: Remove all remaining DfgLogic and DfgUnresolved");
        க்கு (DfgVertex* மாறிலி vtxp : m_dfg.opVertices().unlinkable()) {
            // Previous step should have removed all DfgUnresolved
            UASSERT_OBJ(!vtxp->is<DfgUnresolved>(), vtxp, "DfgUnresolved remains");

            // Process only DfgLogic
            DfgLogic* மாறிலி logicp = vtxp->cast<DfgLogic>();
            எனில் (!logicp) தொடர்;

            // Earlier pass should have removed all sinks
            UASSERT_OBJ(!logicp->hasSinks(), logicp, "DfgLogic sink remains");

            எனில் (!logicp->synth().empty()) {
                // If synthesized, delete the corresponding AstNode. It is now in Dfg.
                logicp->nodep()->unlinkFrBack()->delete_மரம்();
            } இல்லையெனில் {
                // Not synthesized. Logic stays in Ast. Add Ast reference vertices.
                // Outputs already marked by revertTransivelyAndRemove.
                // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
                மாறிலி VNUser4InUse m_user4InUse;
                logicp->foreachSource([](DfgVertex& src) {
                    src.as<DfgVertexVar>()->vscp()->user4p(&src);
                    திரும்பு பொய்;
                });
                V3DfgPasses::addAstRefs(m_dfg, logicp->nodep(), [](Ast_முனை* varp) {  //
                    திரும்பு varp->user4u().to<DfgVertexVar*>();
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
        க்கு (DfgVertex* மாறிலி vtxp : m_dfg.opVertices().unlinkable()) {
            DfgVertexSplice* மாறிலி splicep = vtxp->cast<DfgVertexSplice>();
            எனில் (!splicep) தொடர்;

            // Might not have a sink if the driving logic was revered, remove
            எனில் (!splicep->hasSinks()) {
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
                தொடர்;
            }

            // It should alway have drivers
            UASSERT_OBJ(splicep->nInputs(), splicep, "Splice with no drivers");

            // If redundant, remove it
            எனில் (DfgVertex* மாறிலி wholep = splicep->wholep()) {
                எனில் (DfgVertexVar* மாறிலி varp = splicep->singleSink()->cast<DfgVertexVar>()) {
                    varp->driver_File_வரி(splicep->driver_File_வரி(0));
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
        முதன்மை();
    }

பொது:
    நிலையான வெறுமை apply(DfgGraph& dfg, V3DfgSynthesisContext& ctx) {
        AstToDfgSynthesize{dfg, ctx};

        // Final step outside, as both AstToDfgSynthesize and removeUnused used DfgUserMap
        UINFO(5, "Step 6: Remove all unused vertices");
        V3DfgPasses::removeUnused(dfg);
        எனில் (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("synth-rmunused");

        // No operation vertex should have multiple sinks. Cyclic decomoposition
        // depends on this and it can easily be ensured by using temporaries.
        // Also, all sources should be connected at this point
        எனில் (v3Global.opt.debugCheck()) {
            க்கு (DfgVertex& vtx : dfg.opVertices()) {
                UASSERT_OBJ(!vtx.hasMultipleSinks(), &vtx, "Operation has multiple sinks");
                க்கு (size_t i = 0; i < vtx.nInputs(); ++i) {
                    UASSERT_OBJ(vtx.inputp(i), &vtx, "Unconnected source operand");
                }
            }
            V3DfgPasses::typeCheck(dfg);
        }
    }
};

முழு AstToDfgSynthesize::AstVarScopeComparator::s_vscpIdCounter = 0;

// Decide which DfgLogic to attempt to synthesize
நிலையான வெறுமை dfgSelectLogicForSynthesis(DfgGraph& dfg) {
    // If we are told to synthesize everything, we will do so ...
    எனில் (v3Global.opt.fDfgSynthesizeAll()) {
        க்கு (DfgVertex& vtx : dfg.opVertices()) {
            எனில் (DfgLogic* மாறிலி logicp = vtx.cast<DfgLogic>()) logicp->setSelectedForSynthesis();
        }
        திரும்பு;
    }

    // Otherwise figure out which vertices are likely worth synthesizing.

    std::vector<DfgVertexVar*> circularVarps;
    // Synthesize all drivers of circular variables
    {
        DfgUserMap<uint64_t> scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, scc);
        க்கு (DfgVertexVar& var : dfg.varVertices()) {
            எனில் (!scc.at(var)) தொடர்;
            // This is a circular variable
            circularVarps.emplace_back(&var);
        }
    }

    // To cover all drivers, use a work list
    DfgWorklist worklist{dfg};

    // Synthesize all drivers of circular variables
    க்கு (மாறிலி DfgVertexVar* மாறிலி varp : circularVarps) {
        varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
            worklist.தள்ளு_front(*driver.as<DfgLogic>());
            திரும்பு பொய்;
        });
    }

    // Choose some simple special cases to always synthesize
    க்கு (DfgVertex& vtx : dfg.opVertices()) {
        DfgLogic* மாறிலி logicp = vtx.cast<DfgLogic>();
        எனில் (!logicp || worklist.contains(*logicp)) தொடர்;
        // If drives an unused variable, synthesize it so the partial logic can be removed
        எனில் (logicp->drivesUnusedVars()) {
            worklist.தள்ளு_front(*logicp);
            தொடர்;
        }
        // Blocks corresponding to continuous assignments
        எனில் (logicp->nodep()->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            worklist.தள்ளு_front(*logicp);
            தொடர்;
        }
        மாறிலி CfgGraph& cfg = logicp->cfg();
        // Straight line code with no branches
        எனில் (cfg.nBlocks() == 1) {
            worklist.தள்ளு_front(*logicp);
            தொடர்;
        }
        // Blocks driving exactly 1 variable
        எனில் (!logicp->hasMultipleSinks()) worklist.தள்ளு_front(*logicp);
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
            எனில் (!isVisited.செருகு(unresolved).second) திரும்பு பொய்;
            unresolved->foreachSource([&](DfgVertex& sibling) {
                DfgLogic& siblingLogic = *sibling.as<DfgLogic>();
                எனில் (!siblingLogic.selectedForSynthesis()) worklist.தள்ளு_front(siblingLogic);
                திரும்பு பொய்;
            });
            திரும்பு பொய்;
        });
    });
}

வெறுமை V3DfgPasses::synthesize(DfgGraph& dfg, V3DfgContext& ctx) {
    // Select which DfgLogic to attempt to synthesize
    dfgSelectLogicForSynthesis(dfg);
    // Synthesize them - also removes un-synthesized DfgLogic, so must run even if nothing selected
    AstToDfgSynthesize::apply(dfg, ctx.m_synthContext);
}
