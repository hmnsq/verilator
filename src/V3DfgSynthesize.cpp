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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Ast.h"
#شامل "V3Cfg.h"
#شامل "V3Const.h"
#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"
#شامل "V3EmitV.h"

#شامل <algorithm>
#شامل <iterator>

VL_DEFINE_DEBUG_FUNCTIONS;

فضاینام {

// Create a DfgVertex out of a AstNodeExpr. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
template <typename T_Vertex, typename T_گره>
T_Vertex* makeVertex(ثابت T_گره* nodep, DfgGraph& dfg, ثابت DfgDataType& dtype) {
    بازگشت جدید T_Vertex{dfg, nodep->fileline(), dtype};
}

template <>
DfgArraySel* makeVertex<DfgArraySel, AstArraySel>(ثابت AstArraySel* nodep, DfgGraph& dfg,
                                                  ثابت DfgDataType& dtype) {
    // Some earlier passes create malformed ArraySels, just bail on those...
    // See t_bitsel_wire_array_bad
    اگر (VN_IS(nodep->fromp(), Const)) بازگشت nullptr;
    اگر (!VN_IS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) بازگشت nullptr;
    بازگشت جدید DfgArraySel{dfg, nodep->fileline(), dtype};
}

}  // namespace

// Visitor that can convert Ast statements and expressions in Dfg vertices
رده AstToDfgConverter final : عمومی VNVisitor {
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

    منطقی m_foundUnhandled = نادرست;  // Found node not implemented as DFG or not implemented 'visit'
    منطقی m_converting = نادرست;  // We are trying to convert some logic at the moment

    // METHODS

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    template <typename Vertex, typename... Args>
    Vertex* make(Args&&... args) {
        static_assert(!std::is_base_of<DfgVertexVar, Vertex>::مقدار, "Do not use for variables");
        static_assert(std::is_base_of<DfgVertex, Vertex>::مقدار, "'Vertex' must be a 'DfgVertex'");
        Vertex* ثابت vtxp = جدید Vertex{m_dfg, std::forward<Args>(args)...};
        m_logicp->synth().emplace_back(vtxp);
        بازگشت vtxp;
    }

    // Returns true if the expression cannot (or should not) be represented by DFG
    منطقی unhandled(Ast_گره_عبارت* nodep) {
        // Short-circuiting if something was already unhandled
        اگر (m_foundUnhandled) {
            // Impure nodes cannot be represented
            اگر (!nodep->isPure()) {
                m_foundUnhandled = درست;
                ++m_ctx.m_conv.nonRepImpure;
            }
        }
        بازگشت m_foundUnhandled;
    }

    منطقی isSupported(ثابت AstVarRef* nodep) {
        // Cannot represent cross module references
        اگر (nodep->classOrPackagep()) بازگشت نادرست;
        // Check target
        بازگشت V3Dfg::isSupported(nodep->varScopep());
    }

    // Given an RValue expression, return the equivalent Vertex, or nullptr if not representable.
    DfgVertex* convertRValue(Ast_گره_عبارت* nodep) {
        UASSERT_OBJ(!m_converting, nodep, "'convertingRValue' should not be called recursively");
        VL_RESTORER(m_converting);
        VL_RESTORER(m_foundUnhandled);
        m_converting = درست;
        m_foundUnhandled = نادرست;

        // Convert the expression
        iterate(nodep);

        // If falied to convert, return nullptr
        اگر (m_foundUnhandled) بازگشت nullptr;

        // Traversal set user2p to the equivalent vertex
        DfgVertex* ثابت vtxp = nodep->user2u().to<DfgVertex*>();
        UASSERT_OBJ(vtxp, nodep, "Missing Dfg vertex after conversion");
        بازگشت vtxp;
    }

    // Given an LValue expression, return the splice node that writes the
    // destination, together with the index to use for splicing in the value.
    // Returns {nullptr, 0}, if the given LValue expression is not supported.
    std::pair<DfgVertexSplice*, uint32_t> convertLValue(Ast_گره_عبارت* nodep) {
        اگر (ثابت AstVarRef* ثابت vrefp = VN_CAST(nodep, VarRef)) {
            UASSERT_OBJ(vrefp->access().isWriteOnly(), vrefp, "Non-WriteOnly reference");
            اگر (!isSupported(vrefp)) {
                ++m_ctx.m_conv.nonRepLValue;
                بازگشت {nullptr, 0};
            }

            // Get (or create a new) temporary for this variable
            ثابت DfgVertexVar* ثابت vtxp = [&]() -> DfgVertexVar* {
                // The variable being assigned
                AstVarScope* ثابت vscp = vrefp->varScopep();

                // Find existing one, if any
                برای (ثابت auto& pair : *m_updatesp) {
                    اگر (pair.first == vscp) بازگشت pair.second;
                }

                // Create new one
                DfgVertexVar* ثابت newp = createTmp(*m_logicp, vscp, "SynthAssign");
                m_updatesp->emplace_back(vscp, newp);

                // Create the Splice driver for the new temporary
                اگر (newp->is<DfgVarPacked>()) {
                    newp->srcp(make<DfgSplicePacked>(newp->fileline(), newp->dtype()));
                } وگرنه اگر (newp->is<DfgVarArray>()) {
                    newp->srcp(make<DfgSpliceArray>(newp->fileline(), newp->dtype()));
                } وگرنه {
                    nodep->v3fatalSrc("Unhandled DfgVertexVar sub-type");
                }

                // Use new temporary
                بازگشت newp;
            }();

            // Return the Splice driver
            بازگشت {vtxp->srcp()->as<DfgVertexSplice>(), 0};
        }

        اگر (ثابت AstSel* selp = VN_CAST(nodep, Sel)) {
            // Only handle constant selects
            ثابت AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const);
            اگر (!lsbp) {
                ++m_ctx.m_conv.nonRepLValue;
                بازگشت {nullptr, 0};
            }
            uint32_t lsb = lsbp->toUInt();

            // Convert the 'fromp' sub-expression
            ثابت auto pair = convertLValue(selp->fromp());
            اگر (!pair.first) بازگشت {nullptr, 0};
            DfgSplicePacked* ثابت splicep = pair.first->template as<DfgSplicePacked>();
            // Adjust index.
            lsb += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            اگر (lsb + static_cast<uint32_t>(selp->widthConst()) > splicep->size()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                بازگشت {nullptr, 0};
            }

            // AstSel doesn't change type kind (array vs packed), so we can use
            // the existing splice driver with adjusted lsb
            بازگشت {splicep, lsb};
        }

        اگر (ثابت AstArraySel* ثابت aselp = VN_CAST(nodep, ArraySel)) {
            // Only handle constant selects
            ثابت AstConst* ثابت indexp = VN_CAST(aselp->bitp(), Const);
            اگر (!indexp) {
                ++m_ctx.m_conv.nonRepLValue;
                بازگشت {nullptr, 0};
            }
            uint32_t index = indexp->toUInt();

            // Convert the 'fromp' sub-expression
            ثابت auto pair = convertLValue(aselp->fromp());
            اگر (!pair.first) بازگشت {nullptr, 0};
            DfgSpliceArray* ثابت splicep = pair.first->template as<DfgSpliceArray>();
            // Adjust index. Note pair.second is always 0, but we might handle array slices later..
            index += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            اگر (index + 1U > splicep->size()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                بازگشت {nullptr, 0};
            }

            // Ensure the Splice driver exists for this element
            اگر (!splicep->driverAt(index)) {
                File_سطر* ثابت flp = nodep->fileline();
                // This should never fail
                ثابت DfgDataType& dtype = *DfgDataType::fromAst(nodep->dtypep());
                اگر (dtype.isPacked()) {
                    DfgSplicePacked* ثابت newp = make<DfgSplicePacked>(flp, dtype);
                    ثابت DfgDataType& uaDtype = DfgDataType::array(dtype, 1);
                    DfgUnitArray* ثابت uap = make<DfgUnitArray>(flp, uaDtype);
                    uap->srcp(newp);
                    splicep->addDriver(uap, index, flp);
                } وگرنه اگر (dtype.isArray()) {
                    DfgSpliceArray* ثابت newp = make<DfgSpliceArray>(flp, dtype);
                    splicep->addDriver(newp, index, flp);
                } وگرنه {
                    nodep->v3fatalSrc("Unhandled data type kind");
                }
            }

            // Return the splice driver
            DfgVertex* driverp = splicep->driverAt(index);
            اگر (ثابت DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>()) {
                driverp = uap->srcp();
            }
            بازگشت {driverp->as<DfgVertexSplice>(), 0};
        }

        ++m_ctx.m_conv.nonRepLValue;
        بازگشت {nullptr, 0};
    }

    // Given the LHS of an assignment, and the vertex representing the RHS,
    // connect up the RHS to drive the targets.
    // Returns true on success, false if the LHS is not representable.
    منطقی convertAssignment(File_سطر* flp, Ast_گره_عبارت* lhsp, DfgVertex* vtxp) {
        // Represents a DFG assignment contributed by the AST assignment with the above 'lhsp'.
        // There might be multiple of these if 'lhsp' is a concatenation.
        ساختار Assignment final {
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
        اگر (!VN_IS(lhsp, VarRef)) lhsp = VN_AS(V3Const::constifyExpensiveEdit(lhsp), گره_عبارت);

        // Assigning compound expressions to a concatenated LHS requires a temporary
        // to avoid multiple use of the expression
        اگر (VN_IS(lhsp, Concat) && !vtxp->is<DfgVertexVar>() && !vtxp->is<DfgConst>()) {
            DfgVertexVar* ثابت tmpp = createTmp(*m_logicp, flp, vtxp->dtype(), "Unpack");
            tmpp->srcp(vtxp);
            vtxp = tmpp;
        }

        // Convert each concatenation LHS separately, gather all assignments
        // we need to do into 'assignments', return true if all LValues
        // converted successfully.
        std::vector<Assignment> assignments;
        ثابت std::function<منطقی(Ast_گره_عبارت*, uint32_t)> convertAllLValues
            = [&](Ast_گره_عبارت* subp, uint32_t lsb) -> منطقی {
            // Concatenation on the LHS, convert each part
            اگر (AstConcat* ثابت concatp = VN_CAST(subp, Concat)) {
                Ast_گره_عبارت* ثابت cRhsp = concatp->rhsp();
                Ast_گره_عبارت* ثابت cLhsp = concatp->lhsp();
                // Convert Rigth of concat
                اگر (!convertAllLValues(cRhsp, lsb)) بازگشت نادرست;
                // Convert Left of concat
                بازگشت convertAllLValues(cLhsp, lsb + cRhsp->width());
            }

            // Non-concatenation, convert the LValue
            ثابت auto pair = convertLValue(subp);
            اگر (!pair.first) بازگشت نادرست;

            // If whole lhs, just use it
            اگر (subp == lhsp) {
                assignments.emplace_back(pair.first, pair.second, vtxp);
                بازگشت درست;
            }

            // Otherwise select the relevant bits
            ثابت DfgDataType& dtype = *DfgDataType::fromAst(subp->dtypep());
            DfgSel* ثابت selp = make<DfgSel>(subp->fileline(), dtype);
            selp->fromp(vtxp);
            selp->lsb(lsb);
            assignments.emplace_back(pair.first, pair.second, selp);
            بازگشت درست;
        };

        // Convert the given LHS assignment, give up if any LValues failed to convert
        اگر (!convertAllLValues(lhsp, 0)) بازگشت نادرست;

        // All successful, connect the drivers
        برای (ثابت Assignment& item : assignments) {
            اگر (DfgSplicePacked* ثابت spp = item.m_lhsp->template cast<DfgSplicePacked>()) {
                spp->addDriver(item.m_rhsp, item.m_idx, flp);
            } وگرنه اگر (DfgSpliceArray* ثابت sap = item.m_lhsp->template cast<DfgSpliceArray>()) {
                // TODO: multi-dimensional arrays will need changes here
                ثابت DfgDataType& rDt = item.m_rhsp->dtype();
                اگر (rDt.isPacked()) {
                    // RHS is assigning an element of this array. Need a DfgUnitArray adapter.
                    DfgUnitArray* ثابت uap = make<DfgUnitArray>(flp, DfgDataType::array(rDt, 1));
                    uap->srcp(item.m_rhsp);
                    sap->addDriver(uap, item.m_idx, flp);
                } وگرنه {
                    // RHS is assigning an array (or array slice). Should be the same element type.
                    sap->addDriver(item.m_rhsp, item.m_idx, flp);
                }
            } وگرنه {
                item.m_lhsp->v3fatalSrc("Unhandled DfgVertexSplice sub-type");
            }
        }

        بازگشت درست;
    }

    // VISITORS

    // Unhandled node
    تهی visit(Ast_گره* /*nodep*/) override {
        اگر (!m_foundUnhandled && m_converting) ++m_ctx.m_conv.nonRepUnknown;
        m_foundUnhandled = درست;
    }

    // Expressions - mostly auto generated, but a few special ones
    تهی visit(AstVarRef* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        اگر (unhandled(nodep)) بازگشت;
        // This visit method is only called on RValues, where only read refs are supported
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Non-ReadOnly reference");
        اگر (!isSupported(nodep)) {
            m_foundUnhandled = درست;
            ++m_ctx.m_conv.nonRepVarRef;
            بازگشت;
        }

        // Variable should have been bound before starting conversion
        DfgVertex* ثابت vtxp = nodep->varScopep()->user2u().template to<DfgVertexVar*>();
        UASSERT_OBJ(vtxp, nodep, "Referenced variable has no associated DfgVertexVar");
        nodep->user2p(vtxp);
    }
    تهی visit(AstConst* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        اگر (unhandled(nodep)) بازگشت;

        اگر (nodep->width() != nodep->رقم().width()) {
            // Sometimes the width of the AstConst is not the same as the
            // V3Number it holds. Truncate it here. TODO: should this be allowed?
            V3_عدد رقم{nodep, nodep->width()};
            رقم.opSel(nodep->رقم(), nodep->width() - 1, 0);
            DfgVertex* ثابت vtxp = make<DfgConst>(nodep->fileline(), رقم);
            nodep->user2p(vtxp);
        } وگرنه {
            DfgVertex* ثابت vtxp = make<DfgConst>(nodep->fileline(), nodep->رقم());
            nodep->user2p(vtxp);
        }
    }
    تهی visit(AstCReset* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        اگر (unhandled(nodep)) بازگشت;

        ثابت DfgDataType* ثابت dtypep = DfgDataType::fromAst(nodep->dtypep());
        اگر (!dtypep) {
            m_foundUnhandled = درست;
            ++m_ctx.m_conv.nonRepDType;
            بازگشت;
        }

        UASSERT_OBJ(!nodep->constructing(), nodep,
                    "CReset should be non-constructing at this stage");

        DfgVertex* ثابت vtxp = make<DfgCReset>(nodep->fileline(), *dtypep);
        nodep->user2p(vtxp);
    }
    تهی visit(AstMatchMasked* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        اگر (unhandled(nodep)) بازگشت;

        ثابت DfgDataType* ثابت dtypep = DfgDataType::fromAst(nodep->dtypep());
        اگر (!dtypep) {
            m_foundUnhandled = درست;
            ++m_ctx.m_conv.nonRepDType;
            بازگشت;
        }

        iterate(nodep->lhsp());
        اگر (m_foundUnhandled) بازگشت;
        iterate(nodep->matchp());
        اگر (m_foundUnhandled) بازگشت;

        File_سطر* ثابت flp = nodep->fileline();
        DfgMatchMasked* ثابت vtxp = make<DfgMatchMasked>(flp, *dtypep);
        vtxp->lhsp(nodep->lhsp()->user2u().to<DfgVertex*>());
        vtxp->matchp(nodep->matchp()->user2u().to<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    تهی visit(AstReplicate* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        اگر (unhandled(nodep)) بازگشت;

        ثابت DfgDataType* ثابت dtypep = DfgDataType::fromAst(nodep->dtypep());
        اگر (!dtypep) {
            m_foundUnhandled = درست;
            ++m_ctx.m_conv.nonRepDType;
            بازگشت;
        }

        iterate(nodep->srcp());
        اگر (m_foundUnhandled) بازگشت;

        File_سطر* ثابت flp = nodep->fileline();
        DfgRep* ثابت vtxp = make<DfgRep>(flp, *dtypep);
        vtxp->srcp(nodep->srcp()->user2u().to<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    تهی visit(AstSel* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        اگر (unhandled(nodep)) بازگشت;

        ثابت DfgDataType* ثابت dtypep = DfgDataType::fromAst(nodep->dtypep());
        اگر (!dtypep) {
            m_foundUnhandled = درست;
            ++m_ctx.m_conv.nonRepDType;
            بازگشت;
        }

        iterate(nodep->fromp());
        اگر (m_foundUnhandled) بازگشت;

        File_سطر* ثابت flp = nodep->fileline();
        DfgVertex* vtxp = nullptr;
        اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep->lsbp(), Const)) {
            ثابت uint32_t lsb = constp->toUInt();
            ثابت uint32_t msb = lsb + nodep->widthConst() - 1;
            DfgVertex* ثابت fromp = nodep->fromp()->user2u().to<DfgVertex*>();
            // Unfortunately we can still have out of bounds selects due to how
            // indices are truncated for speed reasons in V3Width/V3Unknown.
            اگر (msb >= fromp->size()) {
                m_foundUnhandled = درست;
                ++m_ctx.m_conv.nonRepOOBSel;
                بازگشت;
            }
            DfgSel* ثابت selp = make<DfgSel>(flp, *dtypep);
            selp->fromp(fromp);
            selp->lsb(lsb);
            vtxp = selp;
        } وگرنه {
            iterate(nodep->lsbp());
            اگر (m_foundUnhandled) بازگشت;
            DfgMux* ثابت muxp = make<DfgMux>(flp, *dtypep);
            muxp->fromp(nodep->fromp()->user2u().to<DfgVertex*>());
            muxp->lsbp(nodep->lsbp()->user2u().to<DfgVertex*>());
            vtxp = muxp;
        }
        nodep->user2p(vtxp);
    }

// The rest of the visit methods for expressions are generated by 'astgen'
#شامل "V3Dfg__gen_ast_to_dfg.h"

عمومی:
    // PUBLIC METHODS

    // Create temporay variable capable of holding the given type
    DfgVertexVar* createTmp(DfgLogic& logic, File_سطر* flp, ثابت DfgDataType& dtype,
                            ثابت std::string& prefix) {
        DfgVertexVar* ثابت vtxp = m_dfg.makeNewVar(flp, prefix, dtype, logic.scopep());
        logic.synth().emplace_back(vtxp);
        vtxp->vscp()->varp()->isInternal(درست);
        vtxp->tmpForp(vtxp->vscp());
        بازگشت vtxp;
    }

    // Create a new temporary variable capable of holding 'varp'
    DfgVertexVar* createTmp(DfgLogic& logic, AstVarScope* vscp, ثابت std::string& prefix) {
        AstVar* ثابت astVarp = vscp->varp();
        File_سطر* ثابت flp = astVarp->fileline();
        ثابت DfgDataType& dtype = *DfgDataType::fromAst(astVarp->dtypep());
        ثابت std::string prfx = prefix + "_" + astVarp->name();
        DfgVertexVar* ثابت vtxp = createTmp(logic, flp, dtype, prfx);
        vtxp->tmpForp(vscp);
        بازگشت vtxp;
    }

    // Convert AstAssign to Dfg, return true if successful.
    // Fills 'updates' with bindings for assigned variables.
    منطقی convert(std::vector<std::pair<AstVarScope*, DfgVertexVar*>>& updates, DfgLogic& vtx,
                 Ast_گره_Assign* nodep) {
        UASSERT_OBJ(VN_IS(nodep, Assign) || VN_IS(nodep, AssignW), nodep, "Bad NodeAssign");
        UASSERT_OBJ(updates.empty(), nodep, "'updates' should be empty");
        VL_RESTORER(m_updatesp);
        VL_RESTORER(m_logicp);
        m_updatesp = &updates;
        m_logicp = &vtx;
        // Assignment with timing control shouldn't make it this far
        UASSERT_OBJ(!nodep->timingControlp(), nodep, "Shouldn't make it this far");
        // Convert it
        ++m_ctx.m_conv.درون_داد_Assignments;
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp();
        // Check data types are compatible.
        ثابت DfgDataType* ثابت lDtypep = DfgDataType::fromAst(lhsp->dtypep());
        ثابت DfgDataType* ثابت rDtypep = DfgDataType::fromAst(rhsp->dtypep());
        اگر (!lDtypep || !rDtypep) {
            ++m_ctx.m_conv.nonRepDType;
            بازگشت نادرست;
        }
        // For now, only direct array assignment is supported (e.g. a = b, but not a = _ ? b : c)
        اگر (rDtypep->isArray()) {
            اگر (!VN_IS(rhsp, VarRef)
                || !lhsp->dtypep()->skipRefp()->same_درخت(rhsp->dtypep()->skipRefp())) {
                ++m_ctx.m_conv.nonRepDType;
                بازگشت نادرست;
            }
        }
        // Widths should match at this point
        UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Mismatched width reached DFG");
        // Convert the RHS expression
        DfgVertex* ثابت rVtxp = convertRValue(rhsp);
        اگر (!rVtxp) بازگشت نادرست;
        // Connect the RHS vertex to the LHS targets
        ثابت منطقی success = convertAssignment(nodep->fileline(), lhsp, rVtxp);
        اگر (success) ++m_ctx.m_conv.representable;
        بازگشت success;
    }

    // Convert RValue expression to Dfg. Returns nullptr if failed.
    DfgVertex* convert(DfgLogic& vtx, Ast_گره_عبارت* nodep) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;
        // Convert it
        ++m_ctx.m_conv.درون_داد_Expressions;
        DfgVertex* ثابت vtxp = convertRValue(nodep);
        اگر (vtxp) ++m_ctx.m_conv.representable;
        بازگشت vtxp;
    }

    // CONSTRUCTOR
    AstToDfgConverter(DfgGraph& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {}
};

// Debug aid - outisde 'AstToDfgSynthesize' as it is a template, but want one instance
V3DebugBisect s_dfgSynthDebugBisect{"DfgSynthesize"};

رده AstToDfgSynthesize final {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // TYPES

    // SymTab must be ordered in order to yield stable results
    ساختار AstVarScopeComparator final {
        ایستا صحیح s_vscpIdCounter;  // Counter for lazily allocating the unique AstVarScope IDs
        منطقی operator()(AstVarScope* lhs, AstVarScope* rhs) ثابت {
            اگر (!lhs->user4()) lhs->user4(++s_vscpIdCounter);
            اگر (!rhs->user4()) rhs->user4(++s_vscpIdCounter);
            بازگشت lhs->user4() < rhs->user4();
        }
    };
    using SymTab = std::map<AstVarScope*, DfgVertexVar*, AstVarScopeComparator>;

    // Represents a [potentially partial] driver of a variable
    ساختار Driver final {
        DfgVertex* m_vtxp = nullptr;  // Driving vertex
        uint32_t m_lo = 0;  // Low index of driven range (internal, not Verilog)
        uint32_t m_hi = 0;  // High index of driven range (internal, not Verilog)
        File_سطر* m_flp = nullptr;  // Location of driver in source

        Driver() = پیشفرض;
        Driver(DfgVertex* vtxp, uint32_t lo, File_سطر* flp)
            : m_vtxp{vtxp}
            , m_lo{lo}
            , m_hi{lo + vtxp->size() - 1U}
            , m_flp{flp} {}
        operator منطقی() ثابت { بازگشت m_vtxp != nullptr; }

        منطقی operator<(ثابت Driver& other) ثابت {
            اگر (m_lo != other.m_lo) بازگشت m_lo < other.m_lo;
            اگر (m_hi != other.m_hi) بازگشت m_hi < other.m_hi;
            بازگشت m_flp->operatorCompare(*other.m_flp) < 0;
        }

        منطقی operator<=(ثابت Driver& other) ثابت { بازگشت !(other < *این); }
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
    Cfg_Dominator_درخت m_dom_درخت;  // The dominator tree of the current CFG

    // STATE - Some debug aid
    // We stop after synthesizing s_dfgSynthDebugLimit vertices (if non-zero).
    // This is the problematic logic (last one we synthesize), assuming a
    // bisection search over s_dfgSynthDebugLimit.
    DfgLogic* m_debugLogicp = nullptr;
    // Source (upstream) cone of outputs of m_debugLogicp
    std::unique_ptr<std::unordered_set<ثابت DfgVertex*>> m_debugOSrcConep{nullptr};

    // METHODS

    // Dump current graph for debugging ...
    تهی debugDump(ثابت نویسه* name) {
        // If we have the debugged logic, compute the vertices feeding its outputs
        اگر (VL_UNLIKELY(m_debugLogicp)) {
            std::vector<ثابت DfgVertex*> outputs;
            m_debugLogicp->foreachSink([&outputs](ثابت DfgVertex& v) {
                outputs.emplace_back(v.singleSink()->as<DfgVertexVar>());
                بازگشت نادرست;
            });
            m_debugOSrcConep = m_dfg.sourceCone(outputs);
        }

        اگر (VL_UNLIKELY(dumpDfgLevel() >= 9 || m_debugOSrcConep)) {
            m_dfg.dumpDotFilePrefixed(name);
            اگر (m_debugOSrcConep) {
                // Dump only the subgraph involving the inputs and outputs of the bad vertex
                m_dfg.dumpDotFilePrefixed(name + "-min"s, [&](ثابت DfgVertex& v) -> منطقی {
                    بازگشت m_debugOSrcConep->شمار(&v);
                });
            }
        }
    }

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    template <typename Vertex, typename... Args>
    Vertex* make(Args&&... args) {
        static_assert(!std::is_base_of<DfgVertexVar, Vertex>::مقدار, "Do not use for variables");
        static_assert(std::is_base_of<DfgVertex, Vertex>::مقدار, "'Vertex' must be a 'DfgVertex'");
        Vertex* ثابت vtxp = جدید Vertex{m_dfg, std::forward<Args>(args)...};
        اگر (m_logicp) m_logicp->synth().emplace_back(vtxp);
        بازگشت vtxp;
    }

    // Gather all drivers of a resolved variable
    ایستا std::vector<Driver> gatherDrivers(DfgVertexSplice* vtxp) {
        // Collect them all, check if they are sorted
        std::vector<Driver> drivers;
        drivers.reserve(vtxp->nInputs());
        منطقی sorted = درست;
        vtxp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_سطر* flp) {
            // Collect the driver
            drivers.emplace_back(&src, lo, flp);
            // Check if drivers are sorted - most often they are
            ثابت size_t n = drivers.size();
            اگر (n >= 2 && drivers[n - 1] < drivers[n - 2]) sorted = نادرست;
            بازگشت نادرست;
        });

        // Sort if unsorted
        اگر (!sorted) std::stable_sort(drivers.begin(), drivers.end());

        // Done
        بازگشت drivers;
    }

    // Returns true if the driver supplies a complete array element.
    ایستا منطقی driverCoversWholeElement(ثابت Driver& driver) {
        ثابت DfgUnitArray* ثابت unitp = driver.m_vtxp->cast<DfgUnitArray>();
        اگر (!unitp) بازگشت نادرست;
        اگر (DfgVertexSplice* ثابت splicep = unitp->srcp()->cast<DfgVertexSplice>()) {
            بازگشت splicep->wholep();
        }
        بازگشت درست;
    }

    // Unlike 'wholep', several element drivers can collectively cover an array.
    ایستا منطقی spliceCoversWhole(DfgVertexSplice* ثابت splicep) {
        اگر (splicep->wholep()) بازگشت درست;
        اگر (splicep->isPacked()) بازگشت نادرست;
        uint32_t next = 0;
        برای (ثابت Driver& driver : gatherDrivers(splicep)) {
            اگر (driver.m_lo != next || !driverCoversWholeElement(driver)) بازگشت نادرست;
            next = driver.m_hi + 1;
        }
        بازگشت next == splicep->size();
    }

    // Returns true if the driver cone contains any variable introduced by
    // tristate lowering. Used to distinguish intentional tristate contributor
    // overlap from accidental multidrive.
    ایستا منطقی containsTriLoweredVar(DfgVertex* rootp) {
        std::vector<ثابت DfgVertex*> پشته;
        std::vector<ثابت DfgVertex*> visited;
        پشته.emplace_back(rootp);
        وقتی (!پشته.empty()) {
            ثابت DfgVertex* ثابت vtxp = پشته.back();
            پشته.برداشت_back();
            اگر (std::find(visited.begin(), visited.end(), vtxp) != visited.end()) ادامه;
            visited.emplace_back(vtxp);
            اگر (ثابت DfgVertexVar* ثابت varp = vtxp->cast<DfgVertexVar>()) {
                اگر (varp->vscp()->varp()->dfgTriLowered()) بازگشت درست;
            }
            vtxp->foreachSource([&](ثابت DfgVertex& src) {
                پشته.emplace_back(&src);
                بازگشت نادرست;
            });
        }
        بازگشت نادرست;
    }

    // Returns true if the driver is a direct variable forward (no logic).
    ایستا منطقی isDirectVarDriver(ثابت DfgVertex* vtxp) { بازگشت vtxp->is<DfgVertexVar>(); }

    // Gather all synthesized drivers of an unresolved variable
    ایستا std::vector<Driver> gatherDriversUnresolved(DfgUnresolved* vtxp) {
        std::vector<Driver> drivers;
        drivers.reserve(vtxp->nInputs());

        // For better locations in error reporting, we unpick concatenations
        // which are sometimes introduced by combinint assignments in V3Const.
        ثابت std::function<تهی(DfgVertex*, uint32_t, File_سطر*)> gather
            = [&](DfgVertex* vtxp, uint32_t lo, File_سطر* flp) -> تهی {
            اگر (DfgConcat* ثابت concatp = vtxp->cast<DfgConcat>()) {
                DfgVertex* ثابت rhsp = concatp->rhsp();
                gather(rhsp, lo, rhsp->fileline());
                DfgVertex* ثابت lhsp = concatp->lhsp();
                gather(lhsp, lo + rhsp->width(), lhsp->fileline());
                بازگشت;
            }
            drivers.emplace_back(vtxp, lo, flp);
        };

        // Gather all synthesized drivers
        vtxp->foreachSource([&](DfgVertex& src) {
            // Can ignore the original DfgLogic
            اگر (src.is<DfgLogic>()) بازگشت نادرست;
            // Synthesized drivers must be a splice at this point
            DfgVertexSplice* ثابت splicep = src.as<DfgVertexSplice>();
            // Collect the driver
            splicep->foreachDriver([&](DfgVertex& src, uint32_t lo, File_سطر* flp) {
                gather(&src, lo, flp);
                بازگشت نادرست;
            });
            بازگشت نادرست;
        });

        // Sort the drivers
        std::stable_sort(drivers.begin(), drivers.end());

        // Done
        بازگشت drivers;
    }

    // Given two drivers, combine the driven sub-ranges into the first one if
    // possible. First bool returned indicates successfully combined and there
    // are no multi-driven bits. Second bool returned indicates we warned
    // already about multi-driven bits.
    std::pair<منطقی, منطقی> combineDrivers(DfgVertexVar& var, ثابت std::string& sub,  //
                                         Driver& a, ثابت Driver& b) {
        // We can only combine array drivers ...
        اگر (a.m_vtxp->isPacked()) بازگشت {نادرست, نادرست};
        // ... that both drive a single element ...
        اگر (a.m_lo != b.m_lo) بازگشت {نادرست, نادرست};
        ثابت DfgUnitArray* ثابت aUap = a.m_vtxp->template cast<DfgUnitArray>();
        اگر (!aUap) بازگشت {نادرست, نادرست};
        ثابت DfgUnitArray* ثابت bUap = b.m_vtxp->template cast<DfgUnitArray>();
        اگر (!bUap) بازگشت {نادرست, نادرست};
        // ... and are themeselves partial
        DfgSplicePacked* ثابت aSp = aUap->srcp()->template cast<DfgSplicePacked>();
        اگر (!aSp) بازگشت {نادرست, نادرست};
        DfgSplicePacked* ثابت bSp = bUap->srcp()->template cast<DfgSplicePacked>();
        اگر (!bSp) بازگشت {نادرست, نادرست};
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
        اگر (!normalizeDrivers(var, abDrivers, sub + "[" + std::to_string(a.m_lo) + "]")) {
            بازگشت {نادرست, درست};
        }

        // Successfully resolved. Needs a new splice and unit.
        File_سطر* ثابت flp = var.fileline();
        DfgSplicePacked* ثابت splicep = make<DfgSplicePacked>(flp, aSp->dtype());
        برای (ثابت Driver& d : abDrivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        DfgUnitArray* ثابت uap = make<DfgUnitArray>(flp, aUap->dtype());
        uap->srcp(splicep);
        a.m_vtxp = uap;
        بازگشت {درست, نادرست};
    }

    // Combine and coalesce the given drivers.
    // Returns true iff no multi-driven bits are present.
    منطقی normalizeDrivers(DfgVertexVar& var, std::vector<Driver>& drivers,
                          ثابت std::string& sub = "") {
        اگر (drivers.empty()) بازگشت درست;

        // What type of values are we combining
        ثابت منطقی isPacked = drivers[0].m_vtxp->isPacked();

        // Found a multidriven part ?
        منطقی multiDriven = نادرست;

        // Iterate through the sorted drivers. Index 'i' is the driver we are
        // resolving driver 'j' agains, and if required, we merge 'j' into 'i'.
        size_t i = 0;
        برای (size_t j = 1; j < drivers.size();) {
            UASSERT_OBJ(i < j, &var, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            اگر (!jD) {
                ++j;
                ادامه;
            }
            // If 'i' was moved, move 'j' in it's place
            اگر (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                ادامه;
            }

            // We have 2 valid drivers now
            UASSERT_OBJ(iD <= jD, &var, "Should always be sorted");
            UASSERT_OBJ(jD.m_vtxp->isPacked() == isPacked, &var, "Mixed type drivers");

            // If no overlap, consider next pair
            اگر (iD.m_hi < jD.m_lo) {
                ++i;
                اگر (i == j) ++j;
                ادامه;
            }

            // There is an overlap. Attempt to combine them.
            منطقی combined = نادرست;
            منطقی warned = نادرست;
            std::tie(combined, warned) = combineDrivers(var, sub, iD, jD);

            // If sucessfully combined, 'j' is no longer needed, it was combined into 'i'
            اگر (combined) {
                jD = Driver{};
                ++j;
                ادامه;
            }

            // Found overlap that cannot be resolved
            multiDriven = درست;
            // Compare next driver
            ++j;

            // Do not warn again if we warned during 'combineDrivers'
            اگر (warned) ادامه;

            // The variable to warn on
            AstVarScope* ثابت vscp = var.tmpForp() ? var.tmpForp() : var.vscp();

            // Loop index often abused, so suppress
            اگر (vscp->varp()->isUsedLoopIdx()) ادامه;
            // Tristate lowering can intentionally create overlapping contributors.
            // Keep the signal marked multidriven for DFG fallback, but suppress
            // warning only when both overlapping driver cones look tri-lowered.
            اگر (vscp->varp()->dfgAllowMultidriveTri()) {
                ثابت منطقی iTri = containsTriLoweredVar(iD.m_vtxp);
                ثابت منطقی jTri = containsTriLoweredVar(jD.m_vtxp);
                ثابت منطقی triPair = iTri && jTri;
                ثابت منطقی triAndBridge = (iTri && isDirectVarDriver(jD.m_vtxp))
                                          || (jTri && isDirectVarDriver(iD.m_vtxp));
                اگر (triPair || triAndBridge) ادامه;
            }

            // Warn the user now
            ثابت std::string lo = std::to_string(jD.m_lo);
            ثابت std::string hi = std::to_string(std::min(iD.m_hi, jD.m_hi));
            ثابت std::string kind = isPacked ? "Bit" : "Element";
            ثابت std::string part = hi == lo ? (" [" + lo + "]") : ("s [" + hi + ":" + lo + "]");

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
        اگر (!multiDriven && isPacked) coalesceDrivers(drivers);

        بازگشت !multiDriven;
    }

    // Coalesce adjacent drivers into single ones
    تهی coalesceDrivers(std::vector<Driver>& drivers) {
        UASSERT(!drivers.empty(), "Can't coalesce 0 drivers");
        UASSERT_OBJ(drivers[0].m_vtxp->isPacked(), drivers[0].m_vtxp, "Can only coalesce packed");

        size_t i = 0;
        برای (size_t j = 1; j < drivers.size();) {
            UASSERT(i < j, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            اگر (!jD) {
                ++j;
                ادامه;
            }
            // If 'i' was moved, move 'j' in it's place
            اگر (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                ادامه;
            }

            // We have 2 valid drivers now
            UASSERT(iD <= jD, "Should always be sorted");

            // If not adjacent, move on
            اگر (iD.m_hi + 1 != jD.m_lo) {
                ++i;
                اگر (i == j) ++j;
                ادامه;
            }

            // Coalesce Adjacent ranges,
            ثابت DfgDataType& dt = DfgDataType::packed(iD.m_vtxp->width() + jD.m_vtxp->width());
            DfgConcat* ثابت concatp = make<DfgConcat>(iD.m_flp, dt);
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
    DfgVertexSplice* makeSplice(DfgVertexVar& var, ثابت std::vector<Driver>& newDrivers) {
        UASSERT_OBJ(!newDrivers.empty(), &var, "'makeSplice' called with no new drivers");
        // Create new driver
        DfgVertexSplice* splicep = nullptr;
        اگر (var.is<DfgVarPacked>()) {
            splicep = make<DfgSplicePacked>(var.fileline(), var.dtype());
        } وگرنه اگر (var.is<DfgVarArray>()) {
            splicep = make<DfgSpliceArray>(var.fileline(), var.dtype());
        } وگرنه {
            var.v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        برای (ثابت Driver& d : newDrivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        بازگشت splicep;
    }

    // Initialzie input symbol table of entry CfgBlock
    تهی initialize_Entry_نماد_Table(SymTab& iSymTab) {
        m_logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* ثابت vvp = src.as<DfgVertexVar>();
            iSymTab[vvp->vscp()] = vvp;
            بازگشت نادرست;
        });
    }

    // Join variable drivers across a control flow confluence (insert muxes ...)
    DfgVertexVar* joinDrivers(AstVarScope* vscp, DfgVertexVar* predicatep,  //
                              DfgVertexVar* thenp, DfgVertexVar* elsep) {
        AstVarScope* ثابت thenVscp = thenp->tmpForp() ? thenp->tmpForp() : thenp->vscp();
        AstVarScope* ثابت elseVscp = elsep->tmpForp() ? elsep->tmpForp() : elsep->vscp();
        UASSERT_OBJ(thenVscp == elseVscp, vscp, "Attempting to join unrelated variables");

        // If both bindings are the the same (variable not updated through either path),
        // then there is nothing to do, can use the same binding
        اگر (thenp == elsep) بازگشت thenp;

        // We can't join the input variable just yet, so bail
        اگر (thenp->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_درون_داد;
            بازگشت nullptr;
        }
        اگر (elsep->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_درون_داد;
            بازگشت nullptr;
        }

        // Can't do arrays yet
        اگر (!thenp->isPacked()) {
            ++m_ctx.m_synt.nonSynArray;
            بازگشت nullptr;
        }

        // Gather drivers of 'thenp' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* ثابت tDefaultp = thenp->defaultp();
        std::vector<Driver> tDrivers = gatherDrivers(thenp->srcp()->as<DfgVertexSplice>());

        // Gather drivers of 'elsep' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* ثابت eDefaultp = elsep->defaultp();
        std::vector<Driver> eDrivers = gatherDrivers(elsep->srcp()->as<DfgVertexSplice>());

        // Default drivers should be the same or not present on either
        UASSERT_OBJ(tDefaultp == eDefaultp, vscp, "Different default drivers");

        // Location to use for the join vertices
        File_سطر* ثابت flp = predicatep->fileline();

        // Create a fresh temporary for the joined value
        DfgVertexVar* ثابت joinp = m_converter.createTmp(*m_logicp, vscp, "SynthJoin");
        DfgVertexSplice* ثابت joinSplicep = make<DfgSplicePacked>(flp, joinp->dtype());
        joinp->srcp(joinSplicep);

        // If both paths are fully driven, just create a simple conditional
        اگر (tDrivers.size() == 1  //
            && tDrivers[0].m_lo == 0  //
            && tDrivers[0].m_hi == thenp->width() - 1  //
            && eDrivers.size() == 1  //
            && eDrivers[0].m_lo == 0  //
            && eDrivers[0].m_hi == elsep->width() - 1) {
            UASSERT_OBJ(!tDefaultp, vscp, "Fully driven variable have default driver");

            DfgCond* ثابت condp = make<DfgCond>(flp, joinp->dtype());
            condp->condp(predicatep);
            condp->thenp(thenp);
            condp->elsep(elsep);
            joinSplicep->addDriver(condp, 0, tDrivers[0].m_flp);

            // Done
            بازگشت joinp;
        }

        // Otherwise we need to merge them part by part

        // If different bits are driven, then some might not have been assigned.. Latch?
        اگر (tDrivers.size() != eDrivers.size()) {
            ++m_ctx.m_synt.nonSynLatch;
            بازگشت nullptr;
        }

        برای (size_t i = 0; i < tDrivers.size(); ++i) {
            ثابت Driver& tDriver = tDrivers[i];
            ثابت Driver& eDriver = eDrivers[i];
            // If different bits are driven, then some might not have been assigned.. Latch?
            اگر (tDriver.m_lo != eDriver.m_lo || tDriver.m_hi != eDriver.m_hi) {
                ++m_ctx.m_synt.nonSynLatch;
                بازگشت nullptr;
            }

            ثابت DfgDataType& dtype = DfgDataType::packed(tDriver.m_hi - tDriver.m_lo + 1);
            DfgCond* ثابت condp = make<DfgCond>(flp, dtype);
            condp->condp(predicatep);

            // We actally need to select the bits from the joined variables, not use the drivers
            DfgSel* ثابت thenSelp = make<DfgSel>(flp, tDriver.m_vtxp->dtype());
            thenSelp->lsb(tDriver.m_lo);
            thenSelp->fromp(thenp);
            condp->thenp(thenSelp);

            // Same for the 'else' part
            DfgSel* ثابت elseSelp = make<DfgSel>(flp, eDriver.m_vtxp->dtype());
            elseSelp->lsb(eDriver.m_lo);
            elseSelp->fromp(elsep);
            condp->elsep(elseSelp);

            // Add it as a driver to the join
            joinSplicep->addDriver(condp, tDriver.m_lo, tDriver.m_flp);
        }

        // If there was a default driver, add it to te join
        اگر (tDefaultp) joinp->defaultp(tDefaultp);

        // Done
        بازگشت joinp;
    }

    // Merge 'thenSymTab' into 'elseSymTab' using the given predicate to join values
    منطقی join_نماد_Tables(SymTab& elseSymTab, DfgVertexVar* predicatep, ثابت SymTab& thenSymTab) {
        // Any variable that does not have a binding on both paths will be removed. These might be
        // temporaries, loop vars, etc used only in one branch. Conversion will fail if the
        // variable is actually referenced later.
        std::vector<AstVarScope*> toRemove;

        // Join each symbol
        برای (std::pair<AstVarScope* ثابت, DfgVertexVar*>& pair : elseSymTab) {
            AstVarScope* ثابت varp = pair.first;
            // Find same variable on the else path
            ثابت auto it = thenSymTab.find(varp);
            // Record for removal if not assigned on both paths
            اگر (it == thenSymTab.end()) {
                toRemove.emplace_back(varp);
                ادامه;
            }
            // Join paths with the predicate
            DfgVertexVar* ثابت thenp = it->second;
            DfgVertexVar* ثابت elsep = pair.second;
            DfgVertexVar* ثابت newp = joinDrivers(varp, predicatep, thenp, elsep);
            اگر (!newp) بازگشت نادرست;
            pair.second = newp;
        }

        // Remove variables not assigned on both paths
        برای (AstVarScope* ثابت varp : toRemove) elseSymTab.erase(varp);

        // Done
        بازگشت درست;
    }

    // Given two joining control flow edges, compute how to join their symbols.
    // Returns the predicaete to join over, and the 'then' and 'else' blocks.
    std::tuple<DfgVertexVar*, ثابت CfgBlock*, ثابت CfgBlock*>  //
    howToJoin(ثابت CfgEdge* ثابت ap, ثابت CfgEdge* ثابت bp) {
        // Find the closest common dominator of the two paths
        ثابت CfgBlock* ثابت domp = m_dom_درخت.closestCommonDominator(ap->srcp(), bp->srcp());
        // These paths join here, so 'domp' must be a branch, otherwise it's not the closest
        UASSERT_OBJ(domp->isBranch(), domp, "closestCommonDominator is not a branch");

        // The branches of the common dominator
        ثابت CfgEdge* ثابت takenEdgep = domp->takenEdgep();
        ثابت CfgEdge* ثابت untknEdgep = domp->untknEdgep();

        // We check if the taken branch dominates the path to either blocks,
        // and if the untaken branch dominates the path to the other block.
        // If so, we can use the branch condition as predicate, otherwise
        // we must use the path predicate as there are ways to get from one
        // branch of the dominator to the other. We need to be careful if
        // either branches are directly to the join block. This is fine,
        // it's as if there was an empty block on that critical edge which
        // is dominated by that path.

        اگر (takenEdgep == ap || m_dom_درخت.dominates(takenEdgep->dstp(), ap->srcp())) {
            اگر (untknEdgep == bp || m_dom_درخت.dominates(untknEdgep->dstp(), bp->srcp())) {
                // Taken path dominates 'ap' and untaken dominates 'bp', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                بازگشت std::make_tuple(m_bbToCondp[domp], ap->srcp(), bp->srcp());
            }
        } وگرنه اگر (takenEdgep == bp || m_dom_درخت.dominates(takenEdgep->dstp(), bp->srcp())) {
            اگر (untknEdgep == ap || m_dom_درخت.dominates(untknEdgep->dstp(), ap->srcp())) {
                // Taken path dominates 'bp' and untaken dominates 'ap', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                بازگشت std::make_tuple(m_bbToCondp[domp], bp->srcp(), ap->srcp());
            }
        }

        // The branches don't dominate the joined blocks, must use the path predicate
        ++m_ctx.m_synt.joinUsingPathPredicate;

        // TODO: We could do better here: use the path predicate of the closest
        // cominating blocks, pick the one from the lower rank, etc, but this
        // generic case is very rare, most synthesizable logic has
        // series-parallel CFGs which are covered by the earlier cases.
        بازگشت std::make_tuple(m_edgeToPredicatep[ap], ap->srcp(), bp->srcp());
    }

    // Combine the output symbol tables of the predecessors of the given
    // block to compute the input symtol table for the given block.
    منطقی create_درون_داد_نماد_Table(ثابت CfgBlock& bb) {
        // The input symbol table of the given block, we are computing it now
        SymTab& joined = m_bbToISymTab[bb];

        // Input symbol table of entry block is special
        اگر (bb.isEnter()) {
            initialize_Entry_نماد_Table(joined);
            بازگشت درست;
        }

        // Current input symbol table should be empty, we will fill it in here
        UASSERT(joined.empty(), "Unprocessed input symbol table should be empty");

        // Fast path if there is only one predecessor - TODO: use less copying
        اگر (!bb.isJoin()) {
            joined = m_bbToOSymTab[bb.firstPredecessorp()];
            بازگشت درست;
        }

        // We also have a simpler job if there are 2 predecessors
        اگر (bb.isTwoWayJoin()) {
            DfgVertexVar* predicatep = nullptr;
            ثابت CfgBlock* thenp = nullptr;
            ثابت CfgBlock* elsep = nullptr;
            std::tie(predicatep, thenp, elsep)
                = howToJoin(bb.firstPredecessorEdgep(), bb.lastPredecessorEdgep());
            // Copy from else
            joined = m_bbToOSymTab[elsep];
            // Join with then
            بازگشت join_نماد_Tables(joined, predicatep, m_bbToOSymTab[*thenp]);
        }

        // General hard way

        // Gather predecessors
        ساختار Predecessor final {
            ثابت CfgBlock* m_bbp;  // Predeccessor block
            DfgVertexVar* m_predicatep;  // Predicate predecessor reached this block with
            ثابت SymTab* m_oSymTabp;  // Output symbol table or predecessor
            Predecessor() = delete;
            Predecessor(ثابت CfgBlock* bbp, DfgVertexVar* predicatep, ثابت SymTab* oSymTabp)
                : m_bbp{bbp}
                , m_predicatep{predicatep}
                , m_oSymTabp{oSymTabp} {}
        };

        ثابت std::vector<Predecessor> predecessors = [&]() {
            std::vector<Predecessor> res;
            برای (ثابت V3GraphEdge& edge : bb.inEdges()) {
                ثابت CfgEdge& cfgEdge = static_cast<ثابت CfgEdge&>(edge);
                ثابت CfgBlock* ثابت predecessorp = cfgEdge.srcp();
                DfgVertexVar* ثابت predicatep = m_edgeToPredicatep[cfgEdge];
                ثابت SymTab* ثابت oSymTabp = &m_bbToOSymTab[predecessorp];
                res.emplace_back(predecessorp, predicatep, oSymTabp);
            }
            // Sort predecessors reverse topologically. This way earlier blocks
            // will come after later blocks, and the entry block is last if present.
            std::sort(res.begin(), res.end(), [](ثابت Predecessor& a, ثابت Predecessor& b) {  //
                بازگشت *a.m_bbp > *b.m_bbp;
            });
            بازگشت res;
        }();

        // Start by copying the bindings from the frist predecessor
        joined = *predecessors[0].m_oSymTabp;
        // Join over all other predecessors
        برای (size_t i = 1; i < predecessors.size(); ++i) {
            DfgVertexVar* ثابت predicatep = predecessors[i].m_predicatep;
            ثابت SymTab& oSymTab = *predecessors[i].m_oSymTabp;
            اگر (!join_نماد_Tables(joined, predicatep, oSymTab)) بازگشت نادرست;
        }

        بازگشت درست;
    }

    std::vector<Driver> computePropagatedDrivers(ثابت std::vector<Driver>& newDrivers,
                                                 DfgVertexVar* oldp) {
        // Gather drivers of 'oldp' - they are in incresing range order with no overlaps
        ثابت std::vector<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.empty(), oldp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::vector<Driver> propagatedDrivers;

        // Add bits between 'msb' and 'lsb' from 'oldp' to 'pDrivers'
        ثابت auto addOldDriver = [&](File_سطر* ثابت flp, uint32_t msb, uint32_t lsb) {
            UASSERT_OBJ(propagatedDrivers.empty() || lsb > propagatedDrivers.back().m_hi, flp,
                        "Drivers should be in ascending order");
            DfgSel* ثابت selp = make<DfgSel>(flp, DfgDataType::packed(msb - lsb + 1));
            selp->lsb(lsb);
            selp->fromp(oldp);
            propagatedDrivers.emplace_back(selp, lsb, flp);
        };

        // Incorporate old drivers
        برای (ثابت Driver& oDriver : oldDrivers) {
            File_سطر* ثابت flp = oDriver.m_flp;
            // Range to consider inserting, we will adjust oldLo as we process drivers
            uint32_t oldLo = oDriver.m_lo;
            ثابت uint32_t oldHi = oDriver.m_hi;

            // Loop for now, can move to bisection search if this is a problem, shouldn't be ...
            برای (ثابت Driver& nDriver : newDrivers) {
                UASSERT_OBJ(oldHi >= oldLo, flp, "Should have stopped iteration");
                // If new driver is entirely below old driver, move on to
                اگر (nDriver.m_hi < oldLo) ادامه;
                // If new driver is entirely above old driver, we can stop
                اگر (oldHi < nDriver.m_lo) بشکن;

                // There is an overlap between 'oDriver' and 'nDriver'.
                // Insert the low bits and adjust the insertion range.
                // The rest will take care of itself on subsequent iterations.
                اگر (oldLo < nDriver.m_lo) addOldDriver(flp, nDriver.m_lo - 1, oldLo);
                oldLo = nDriver.m_hi + 1;

                // Stop if no more bits remaining in the old driver
                اگر (oldLo > oldHi) بشکن;
            }

            // Insert remaining bits if any
            اگر (oldHi >= oldLo) addOldDriver(flp, oldHi, oldLo);
        }

        بازگشت propagatedDrivers;
    }

    // Propagate whole elements with a linear walk over the sorted driver lists.
    // Reads use the previous value temporary, preserving assignment-version bindings.
    // Partial elements and non-unit drivers retain the nonsynthesized-process fallback.
    منطقی computePropagatedArrayDrivers(ثابت std::vector<Driver>& newDrivers,
                                       DfgVertexVar* ثابت oldp,
                                       std::vector<Driver>& propagatedDrivers) {
        // Bound quadratic vertex growth: array drivers cannot be coalesced.
        ایستا constexpr uint32_t MAX_ARRAY_ELEMENTS = 32;
        اگر (oldp->size() > MAX_ARRAY_ELEMENTS) بازگشت نادرست;
        برای (ثابت Driver& driver : newDrivers) {
            اگر (!driverCoversWholeElement(driver)) بازگشت نادرست;
        }
        ثابت std::vector<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.empty(), oldp, "Should have a proper driver");
        برای (ثابت Driver& driver : oldDrivers) {
            اگر (!driverCoversWholeElement(driver)) بازگشت نادرست;
        }

        propagatedDrivers.reserve(oldDrivers.size());
        auto nIt = newDrivers.begin();
        برای (ثابت Driver& oDriver : oldDrivers) {
            وقتی (nIt != newDrivers.end() && nIt->m_lo < oDriver.m_lo) ++nIt;
            اگر (nIt != newDrivers.end() && nIt->m_lo == oDriver.m_lo) ادامه;

            File_سطر* ثابت flp = oDriver.m_flp;
            ثابت DfgUnitArray* ثابت oldUnitp = oDriver.m_vtxp->as<DfgUnitArray>();
            DfgArraySel* ثابت selp = make<DfgArraySel>(flp, oldUnitp->srcp()->dtype());
            selp->fromp(oldp);
            selp->bitp(make<DfgConst>(flp, static_cast<size_t>(VL_IDATASIZE), oDriver.m_lo));
            DfgUnitArray* ثابت newUnitp = make<DfgUnitArray>(flp, oldUnitp->dtype());
            newUnitp->srcp(selp);
            propagatedDrivers.emplace_back(newUnitp, oDriver.m_lo, flp);
        }
        بازگشت درست;
    }

    // Given the drivers of a variable after converting a single statement
    // 'newp', add drivers from 'oldp' that were not reassigned be drivers
    // in newp. This computes the total result of all previous assignments.
    منطقی incorporate_Previous_مقدار(AstVarScope* vscp, DfgVertexVar* newp, DfgVertexVar* oldp) {
        UASSERT_OBJ(newp->srcp(), vscp, "Assigned variable has no driver");

        // Easy if there is no old value...
        اگر (!oldp) بازگشت درست;

        // New driver was not yet coalesced, so should always be a splice
        DfgVertexSplice* ثابت nSplicep = newp->srcp()->as<DfgVertexSplice>();

        // If the old value is the real variable we just computed the new value for,
        // then it is the circular feedback into the synthesized block, add it as default driver.
        اگر (oldp->vscp() == vscp) {
            اگر (!spliceCoversWhole(nSplicep)) newp->defaultp(oldp);
            بازگشت درست;
        }

        UASSERT_OBJ(oldp->srcp(), vscp, "Previously assigned variable has no driver");

        // Gather drivers of 'newp' - they are in incresing range order with no overlaps
        UASSERT_OBJ(!newp->defaultp(), newp, "Converted value should not have default");
        std::vector<Driver> nDrivers = gatherDrivers(newp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!nDrivers.empty(), newp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::vector<Driver> pDrivers;
        اگر (newp->isPacked()) {
            pDrivers = computePropagatedDrivers(nDrivers, oldp);
        } وگرنه اگر (!computePropagatedArrayDrivers(nDrivers, oldp, pDrivers)) {
            ++m_ctx.m_synt.nonSynArray;
            بازگشت نادرست;
        }

        اگر (!pDrivers.empty()) {
            // Need to merge propagated sources, so reset the splice
            nSplicep->resetDrivers();
            // Merge drivers - they are both sorted and non-overlapping
            std::vector<Driver> drivers;
            drivers.reserve(nDrivers.size() + pDrivers.size());
            std::merge(nDrivers.begin(), nDrivers.end(), pDrivers.begin(), pDrivers.end(),
                       std::back_inserter(drivers));
            // Coalesce adjacent ranges
            اگر (newp->isPacked()) coalesceDrivers(drivers);
            // Reinsert drivers in order
            برای (ثابت Driver& d : drivers) nSplicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        }

        // If the old had a default, add to the new one too, unless redundant
        اگر (oldp->defaultp() && !spliceCoversWhole(nSplicep)) newp->defaultp(oldp->defaultp());

        // Done
        بازگشت درست;
    }

    // Synthesize the given statements with the given input symbol table.
    // Returns true if successfolly synthesized.
    // Populates the given output symbol table.
    // Populates the given reference with the condition of the terminator branch, if any.
    منطقی synthesizeBasicBlock(SymTab& oSymTab, DfgVertex*& condpr,
                              ثابت std::vector<Ast_گره_Stmt*>& stmtps, ثابت SymTab& iSymTab) {
        // Use fresh set of vertices in m_converter
        ثابت VNUser2InUse user2InUse;

        // Initialize AstVarScope -> Vertex bindings available in this block
        برای (ثابت auto& pair : iSymTab) {
            AstVarScope* ثابت varp = pair.first;
            DfgVertexVar* ثابت vtxp = pair.second;
            varp->user2p(vtxp);
            oSymTab[varp] = vtxp;
        }

        // Synthesize each statement one after the other
        std::vector<std::pair<AstVarScope*, DfgVertexVar*>> updates;
        برای (Ast_گره_Stmt* ثابت stmtp : stmtps) {
            // Regular statements
            Ast_گره_Assign* ثابت ap = VN_CAST(stmtp, گره_Assign);
            اگر (ap && (VN_IS(ap, Assign) || VN_IS(ap, AssignW))) {
                // Convert this assignment
                اگر (!m_converter.convert(updates, *m_logicp, ap)) {
                    ++m_ctx.m_synt.nonSynConv;
                    بازگشت نادرست;
                }
                // Apply variable updates from this statement
                برای (ثابت auto& pair : updates) {
                    // The target variable that was assigned to
                    AstVarScope* ثابت vscp = pair.first;
                    // The new, potentially partially assigned value
                    DfgVertexVar* ثابت newp = pair.second;
                    // Normalize drivers within this statement, bail if multidriven
                    DfgVertexSplice* ثابت srcp = newp->srcp()->as<DfgVertexSplice>();
                    std::vector<Driver> drivers = gatherDrivers(srcp);
                    ثابت منطقی single = drivers.size() == 1;
                    اگر (!normalizeDrivers(*newp, drivers)) {
                        ++m_ctx.m_synt.nonSynMultidrive;
                        بازگشت نادرست;
                    }
                    // If there were more than one driver (often not), re-add in case coalesced
                    اگر (!single) {
                        srcp->resetDrivers();
                        برای (ثابت Driver& d : drivers) srcp->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
                    }
                    // The old value, if any
                    DfgVertexVar* ثابت oldp = vscp->user2u().template to<DfgVertexVar*>();
                    // Inncorporate old value into the new value
                    اگر (!incorporate_Previous_مقدار(vscp, newp, oldp)) بازگشت نادرست;
                    // Update binding of target variable
                    vscp->user2p(newp);
                    // Update output symbol table of this block
                    oSymTab[vscp] = newp;
                }
                updates.clear();
                ادامه;
            }

            // Terminator branches
            اگر (AstIf* ثابت ifp = VN_CAST(stmtp, If)) {
                UASSERT_OBJ(ifp == stmtps.back(), ifp, "Branch should be last statement");
                // Convert condition, give up if failed
                DfgVertex* condp = m_converter.convert(*m_logicp, ifp->condp());
                اگر (!condp) {
                    ++m_ctx.m_synt.nonSynConv;
                    بازگشت نادرست;
                }
                // Single bit condition can be use directly, otherwise: use 'condp != 0'
                اگر (condp->width() != 1) {
                    File_سطر* ثابت flp = condp->fileline();
                    DfgNeq* ثابت neqp = make<DfgNeq>(flp, DfgDataType::packed(1));
                    neqp->lhsp(make<DfgConst>(flp, condp->width(), 0U));
                    neqp->rhsp(condp);
                    condp = neqp;
                }
                condpr = condp;
                ادامه;
            }

            // Unhandled
            ++m_ctx.m_synt.nonSynStmt;
            بازگشت نادرست;
        }

        بازگشت درست;
    }

    // Assign path predicates to the outgoing control flow edges of the given block
    تهی assignPathPredicates(ثابت CfgBlock& bb) {
        // Nothing to do for the exit block
        اگر (bb.isExit()) بازگشت;

        // Get the predicate of this block
        DfgVertex* ثابت predp = [&]() -> DfgVertex* {
            // Entry block has no predecessors, use constant true
            اگر (bb.isEnter()) بازگشت make<DfgConst>(m_logicp->fileline(), 1U, 1U);

            // For any other block, 'or' together all the incoming predicates
            ثابت auto& inEdges = bb.inEdges();
            auto it = inEdges.begin();
            DfgVertex* resp = m_edgeToPredicatep[static_cast<ثابت CfgEdge&>(*it)];
            وقتی (++it != inEdges.end()) {
                DfgOr* ثابت orp = make<DfgOr>(resp->fileline(), resp->dtype());
                orp->rhsp(resp);
                orp->lhsp(m_edgeToPredicatep[static_cast<ثابت CfgEdge&>(*it)]);
                resp = orp;
            }
            بازگشت resp;
        }();

        ثابت DfgDataType& dtype = predp->dtype();

        ثابت auto mkTmp = [&](File_سطر* flp, ثابت نویسه* name, DfgVertex* srcp) {
            ثابت std::string prefix = "_BB" + std::to_string(bb.id()) + "_" + name;
            DfgVertexVar* ثابت tmpp = m_converter.createTmp(*m_logicp, flp, dtype, prefix);
            tmpp->srcp(srcp);
            بازگشت tmpp;
        };

        // Assign it to a variable in case it's used multiple times
        DfgVertexVar* ثابت pInp = mkTmp(predp->fileline(), "PathIn", predp);

        // For uncondional branches, the successor predicate edge is the same
        اگر (!bb.isBranch()) {
            m_edgeToPredicatep[bb.takenEdgep()] = mkTmp(pInp->fileline(), "Goto", pInp);
            بازگشت;
        }

        // For branches, we need to factor in the branch condition
        DfgVertex* ثابت condp = m_bbToCondp[bb];
        File_سطر* ثابت flp = condp->fileline();

        // Predicate for taken branch: 'predp & condp'
        {
            DfgAnd* ثابت takenPredp = make<DfgAnd>(flp, dtype);
            takenPredp->lhsp(pInp);
            takenPredp->rhsp(condp);
            m_edgeToPredicatep[bb.takenEdgep()] = mkTmp(flp, "Taken", takenPredp);
        }

        // Predicate for untaken branch: 'predp & ~condp'
        {
            DfgAnd* ثابت untknPredp = make<DfgAnd>(flp, dtype);
            untknPredp->lhsp(pInp);
            DfgNot* ثابت notp = make<DfgNot>(flp, dtype);
            notp->srcp(condp);
            untknPredp->rhsp(notp);
            m_edgeToPredicatep[bb.untknEdgep()] = mkTmp(flp, "Untkn", untknPredp);
        }
    }

    // Returns true if all external updates to volatile variables are observed correctly
    منطقی checkExtWrites() {
        برای (ثابت DfgVertex* ثابت vtxp : m_logicp->synth()) {
            ثابت DfgVertexVar* ثابت varp = vtxp->cast<DfgVertexVar>();
            اگر (!varp) ادامه;
            // If the variable we synthesized this vertex for is volatile, and
            // the value of the synthesized temporary is observed, we might be
            // missing an external update, so we mut give up.
            اگر (!varp->hasSinks()) ادامه;
            اگر (!DfgVertexVar::isVolatile(varp->tmpForp())) ادامه;
            ++m_ctx.m_synt.nonSynExtWrite;
            بازگشت نادرست;
        }
        بازگشت درست;
    }

    // Add the synthesized values as drivers to the output variables of the current DfgLogic
    منطقی addSynthesizedOutput(SymTab& oSymTab) {
        // It's possible we think a variable is written by the DfgLogic when
        // it actauly isn't, e.g.: '{a[0], b[0]}[1] = ...' does not write 'b'.
        // These LHS forms can happen after some earlier tranforms. We
        // should just run V3Const on them earlier, but we will do belt and
        // braces and check here too. We can't touch any output variables if so.
        ثابت منطقی missing = m_logicp->foreachSink([&](ثابت DfgVertex& sink) {
            ثابت DfgUnresolved* ثابت unresolvedp = sink.as<DfgUnresolved>();
            AstVarScope* ثابت vscp = unresolvedp->singleSink()->as<DfgVertexVar>()->vscp();
            بازگشت !oSymTab.شمار(vscp);
        });
        اگر (missing) {
            ++m_ctx.m_synt.nonSynFalseWrite;
            بازگشت نادرست;
        }

        // Add sinks to read the computed values for the target variables
        بازگشت !m_logicp->foreachSink([&](DfgVertex& sink) {
            DfgUnresolved* ثابت unresolvedp = sink.as<DfgUnresolved>();
            ثابت DfgVertexVar* ثابت varp = unresolvedp->singleSink()->as<DfgVertexVar>();
            DfgVertexVar* ثابت resp = oSymTab.at(varp->vscp());
            UASSERT_OBJ(resp->srcp(), resp, "Undriven result");

            // If the output is not used further in the synthesized logic itself,
            // then resp will be deleted before we return, so we can just use
            // its splice directly without ending up with a multi-use operation.
            اگر (!resp->hasSinks()) {
                unresolvedp->addDriver(resp->srcp()->as<DfgVertexSplice>());
                بازگشت نادرست;  // OK, continue.
            }

            // TODO: computePropagatedDrivers cannot handle arrays, should
            // never happen with simple continous assignments
            اگر (!resp->isPacked()) {
                ++m_ctx.m_synt.nonSynArray;
                بازگشت درست;  // Not OK, give up
            }

            // We need to add a new splice to avoid multi-use of the original splice
            DfgSplicePacked* ثابت splicep
                = جدید DfgSplicePacked{m_dfg, resp->fileline(), resp->dtype()};
            // Drivers are the same
            ثابت std::vector<Driver> drivers = computePropagatedDrivers({}, resp);
            برای (ثابت Driver& d : drivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
            unresolvedp->addDriver(splicep);
            بازگشت نادرست;  // OK, continue
        });
    }

    // Synthesize the given AstAssignW. Returns true on success.
    منطقی synthesizeAssignW(AstAssignW* nodep) {
        ++m_ctx.m_synt.درون_داد_Assign;

        // The input and output symbol tables
        SymTab iSymTab;
        SymTab oSymTab;

        // Initialzie input symbol table
        initialize_Entry_نماد_Table(iSymTab);

        // Synthesize as if it was in a single CfgBlock CFG
        DfgVertex* condp = nullptr;
        ثابت منطقی success = synthesizeBasicBlock(oSymTab, condp, {nodep}, iSymTab);
        UASSERT_OBJ(!condp, nodep, "Conditional AstAssignW ???");
        اگر (!success) بازگشت نادرست;

        // Check exernal writes are observed correctly
        اگر (!checkExtWrites()) بازگشت نادرست;

        // Add resolved output variable drivers
        بازگشت addSynthesizedOutput(oSymTab);
    }

    // Synthesize the given AstAlways. Returns true on success.
    منطقی synthesizeCfg(CfgGraph& cfg) {
        ++m_ctx.m_synt.درون_داد_Always;

        // If there is a backward edge (loop), we can't synthesize it
        اگر (cfg.containsLoop()) {
            ++m_ctx.m_synt.nonSynLoop;
            ++m_ctx.m_synt.cfgCyclic;
            بازگشت نادرست;
        }

        // If it's a trivial CFG we can save on some work
        اگر (cfg.nBlocks() == 1) {
            ++m_ctx.m_synt.cfgTrivial;
        } وگرنه {
            // Insert two-way join blocks to aid multiplexer ordering
            اگر (cfg.درج_Two_Way_Joins()) {
                ++m_ctx.m_synt.cfgSp;
            } وگرنه {
                ++m_ctx.m_synt.cfgDag;
            }
            // Initialize maps needed for non-trivial CFGs
            m_dom_درخت = Cfg_Dominator_درخت{cfg};
            m_edgeToPredicatep = cfg.makeEdgeMap<DfgVertexVar*>();
        }

        // Initialize CfgMaps
        m_bbToISymTab = cfg.makeBlockMap<SymTab>();
        m_bbToOSymTab = cfg.makeBlockMap<SymTab>();
        m_bbToCondp = cfg.makeBlockMap<DfgVertexVar*>();

        // Synthesize all blocks
        برای (ثابت V3GraphVertex& vtx : cfg.vertices()) {
            ثابت CfgBlock& bb = static_cast<ثابت CfgBlock&>(vtx);
            // Prepare the input symbol table of this block (enter, or join predecessor blocks)
            اگر (!create_درون_داد_نماد_Table(bb)) بازگشت نادرست;
            // Synthesize this block
            DfgVertex* condp = nullptr;
            اگر (!synthesizeBasicBlock(m_bbToOSymTab[bb], condp, bb.stmtps(), m_bbToISymTab[bb])) {
                بازگشت نادرست;
            }
            // Create a temporary for the branch condition as it might be used multiple times
            اگر (condp) {
                File_سطر* ثابت flp = condp->fileline();
                ثابت DfgDataType& dtype = condp->dtype();
                ثابت std::string prefix = "_BB" + std::to_string(bb.id()) + "_Cond";
                DfgVertexVar* ثابت vp = m_converter.createTmp(*m_logicp, flp, dtype, prefix);
                vp->srcp(condp);
                m_bbToCondp[bb] = vp;
            }
            // Set the path predicates on the successor edges
            assignPathPredicates(bb);
        }

        // Check exernal writes are observed correctly
        اگر (!checkExtWrites()) بازگشت نادرست;

        // Add resolved output variable drivers
        بازگشت addSynthesizedOutput(m_bbToOSymTab[cfg.exit()]);
    }

    // Synthesize a DfgLogic into regular vertices. Returns ture on success.
    منطقی synthesize(DfgLogic& vtx) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;

        اگر (AstAssignW* ثابت ap = VN_CAST(vtx.nodep()->stmtsp(), AssignW)) {
            اگر (ap->nextp()) بازگشت نادرست;
            اگر (!synthesizeAssignW(ap)) بازگشت نادرست;
            ++m_ctx.m_synt.synthAssign;
            بازگشت درست;
        }

        اگر (!synthesizeCfg(vtx.cfg())) بازگشت نادرست;
        ++m_ctx.m_synt.synthAlways;
        بازگشت درست;
    }

    // Revert all DfgLogic in m_toRevert, or DfgLogic driving the DfgUnresolved
    // vertices in m_toRevert, and transitively the same for any DfgUnresolved
    // driven by the reverted DfgLogic. Delete all DfgUnresolved involed.
    تهی revert(VDouble0& statCountr) {
        m_toRevert.foreach([&](DfgVertex& vtx) {
            // Process DfgLogic
            اگر (DfgLogic* ثابت vtxp = vtx.cast<DfgLogic>()) {
                UASSERT_OBJ(vtxp->selectedForSynthesis(), vtxp, "Shouldn't reach here unselected");
                // Revert this logic
                UASSERT_OBJ(!vtxp->reverted(), vtxp, "Should be reverting now");
                ++statCountr;
                برای (DfgVertex* ثابت p : vtxp->synth()) VL_DO_DANGLING(p->unlinkDelete(m_dfg), p);
                vtxp->synth().clear();
                vtxp->setReverted();
                // Add all DfgUnresolved it drives to the work list
                vtxp->foreachSink([&](DfgVertex& snk) {
                    m_toRevert.افزودن_front(*snk.as<DfgUnresolved>());
                    بازگشت نادرست;
                });
                بازگشت;
            }

            // Process DfgUnresolved
            اگر (DfgUnresolved* ثابت vtxp = vtx.cast<DfgUnresolved>()) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->setHasModWrRefs();
                // Add all driving DfgLogic to the work list
                vtxp->foreachSource([&](DfgVertex& src) {
                    DfgLogic* ثابت lp = src.cast<DfgLogic>();
                    اگر (lp && !lp->reverted()) m_toRevert.افزودن_front(*lp);
                    بازگشت نادرست;
                });
                // Delete the DfgUnresolved driver
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
                بازگشت;
            }

            // There should be nothing else on the worklist
            vtx.v3fatalSrc("Unexpected vertex type");
        });
    }

    // Synthesize all of the given vertices
    تهی اصلی() {
        //-------------------------------------------------------------------
        UINFO(5, "Step 0: Remove all DfgLogic not selected for synthesis");
        برای (DfgVertex* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgUnresolved
            اگر (!vtxp->is<DfgUnresolved>()) ادامه;
            منطقی anySelected = نادرست;
            منطقی anyUnselected = نادرست;
            vtxp->foreachSource([&](DfgVertex& src) {
                ثابت DfgLogic& logic = *src.as<DfgLogic>();
                اگر (logic.selectedForSynthesis()) {
                    anySelected = درست;
                } وگرنه {
                    anyUnselected = درست;
                }
                بازگشت نادرست;
            });
            // There should be a driver
            UASSERT_OBJ(anySelected || anyUnselected, vtxp, "'DfgUnresolved' with no driver");
            // All drivers should be selected or all should be unselected
            UASSERT_OBJ(!(anySelected && anyUnselected), vtxp, "Invalid 'DfgLogic' selection");
            // If all drivers are unselected, delete this DfgUnresolved here
            اگر (anyUnselected) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->setHasModWrRefs();
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
        برای (DfgVertex* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgLogic
            DfgLogic* ثابت logicp = vtxp->cast<DfgLogic>();
            اگر (!logicp) ادامه;
            اگر (logicp->selectedForSynthesis()) ادامه;
            // There should be no sinks left for unselected DfgLogic, delete them here
            UASSERT_OBJ(!logicp->hasSinks(), vtxp, "Unselected 'DfgLogic' with sinks remaining");
            // Input variables will be read in Ast code, add Ast reference vertices
            // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
            ثابت VNUser4InUse m_user4InUse;
            logicp->foreachSource([](DfgVertex& src) {
                src.as<DfgVertexVar>()->vscp()->user4p(&src);
                بازگشت نادرست;
            });
            V3DfgPasses::addAstRefs(m_dfg, logicp->nodep(), [](Ast_گره* varp) {  //
                بازگشت varp->user4u().to<DfgVertexVar*>();
            });
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        debugDump("synth-selected");

        //-------------------------------------------------------------------
        UINFO(5, "Step 1: Attempting to synthesize each of the selected DfgLogic");
        {
            // AstVarScope::user4() -> int: unique ID for 'AstVarScopeComparator'
            ثابت VNUser4InUse user4InUse;
            برای (DfgVertex& vtx : m_dfg.opVertices()) {
                DfgLogic* ثابت logicp = vtx.cast<DfgLogic>();
                اگر (!logicp) ادامه;

                // We should only have DfgLogic remaining that was selected for synthesis
                UASSERT_OBJ(logicp->selectedForSynthesis(), logicp, "Unselected DfgLogic remains");

                // Debug aid
                ثابت auto debugCallback = [&]() -> تهی {
                    // This is the breaking logic
                    m_debugLogicp = logicp;
                    // Dump it
                    UINFOTREE(0, logicp->nodep(), "Problematic DfgLogic: " << logicp, "  ");
                    V3EmitV::debug_Verilog_For_درخت(logicp->nodep(), std::cout);
                    debugDump("synth-lastok");
                };
                اگر (VL_UNLIKELY(s_dfgSynthDebugBisect.stop(debugCallback))) بشکن;

                // Synthesize it, if failed, enqueue for reversion
                اگر (!synthesize(*logicp)) {
                    logicp->setNonSynthesizable();
                    m_toRevert.افزودن_front(*logicp);
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
        std::unordered_map<ثابت DfgVertexVar*, DfgVertexSplice*> resolvedDrivers;
        // Compute resolved drivers of all variablees
        برای (DfgVertexVar& var : m_dfg.varVertices()) {
            اگر (!var.srcp()) ادامه;
            DfgUnresolved* ثابت unresolvedp = var.srcp()->cast<DfgUnresolved>();
            اگر (!unresolvedp) بشکن;  // Stop when reached the synthesized temporaries

            // Resolve the synthesized drivers
            DfgVertexSplice* ثابت resolvedp = [&]() -> DfgVertexSplice* {
                // All synthesized drivers were normalized already,
                // so if there is only one, it can be used directly
                اگر (unresolvedp->nInputs() == 1) {
                    بازگشت unresolvedp->inputp(0)->as<DfgVertexSplice>();
                }
                // Otherwise gather the synthesized drivers
                std::vector<Driver> drivers = gatherDriversUnresolved(unresolvedp);
                // Normalize them, make resolved driver if all good
                اگر (normalizeDrivers(var, drivers)) بازگشت makeSplice(var, drivers);
                // If mutlidriven, record and ignore
                multidrivenps.emplace_back(&var);
                m_toRevert.افزودن_front(*unresolvedp);
                بازگشت nullptr;
            }();
            // Bail if multidriven
            اگر (!resolvedp) ادامه;
            // Add to map for next loop
            ثابت منطقی newEntry = resolvedDrivers.emplace(&var, resolvedp).second;
            UASSERT_OBJ(newEntry, &var, "Duplicate driver");
        }
        // Revert and remove drivers of multi-driven variables
        revert(m_ctx.m_synt.revertMultidrive);
        // Replace all DfgUnresolved with the resolved drivers
        برای (ثابت DfgVertexVar& var : m_dfg.varVertices()) {
            اگر (!var.srcp()) ادامه;
            DfgUnresolved* ثابت srcp = var.srcp()->cast<DfgUnresolved>();
            اگر (!srcp) بشکن;  // Stop when reached the synthesized temporaries

            // Replace it
            srcp->replaceWith(resolvedDrivers.at(&var));
            VL_DO_DANGLING(srcp->unlinkDelete(m_dfg), srcp);
        }
        debugDump("synth-resolved");

        //-------------------------------------------------------------------
        UINFO(5, "Step 4: Remove all remaining DfgLogic and DfgUnresolved");
        برای (DfgVertex* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            // Previous step should have removed all DfgUnresolved
            UASSERT_OBJ(!vtxp->is<DfgUnresolved>(), vtxp, "DfgUnresolved remains");

            // Process only DfgLogic
            DfgLogic* ثابت logicp = vtxp->cast<DfgLogic>();
            اگر (!logicp) ادامه;

            // Earlier pass should have removed all sinks
            UASSERT_OBJ(!logicp->hasSinks(), logicp, "DfgLogic sink remains");

            اگر (!logicp->synth().empty()) {
                // If synthesized, delete the corresponding AstNode. It is now in Dfg.
                logicp->nodep()->unlinkFrBack()->delete_درخت();
            } وگرنه {
                // Not synthesized. Logic stays in Ast. Add Ast reference vertices.
                // Outputs already marked by revertTransivelyAndRemove.
                // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
                ثابت VNUser4InUse m_user4InUse;
                logicp->foreachSource([](DfgVertex& src) {
                    src.as<DfgVertexVar>()->vscp()->user4p(&src);
                    بازگشت نادرست;
                });
                V3DfgPasses::addAstRefs(m_dfg, logicp->nodep(), [](Ast_گره* varp) {  //
                    بازگشت varp->user4u().to<DfgVertexVar*>();
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
        برای (DfgVertex* ثابت vtxp : m_dfg.opVertices().unlinkable()) {
            DfgVertexSplice* ثابت splicep = vtxp->cast<DfgVertexSplice>();
            اگر (!splicep) ادامه;

            // Might not have a sink if the driving logic was revered, remove
            اگر (!splicep->hasSinks()) {
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
                ادامه;
            }

            // It should alway have drivers
            UASSERT_OBJ(splicep->nInputs(), splicep, "Splice with no drivers");

            // If redundant, remove it
            اگر (DfgVertex* ثابت wholep = splicep->wholep()) {
                اگر (DfgVertexVar* ثابت varp = splicep->singleSink()->cast<DfgVertexVar>()) {
                    varp->driver_File_سطر(splicep->driver_File_سطر(0));
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
        اصلی();
    }

عمومی:
    ایستا تهی apply(DfgGraph& dfg, V3DfgSynthesisContext& ctx) {
        AstToDfgSynthesize{dfg, ctx};

        // Final step outside, as both AstToDfgSynthesize and removeUnused used DfgUserMap
        UINFO(5, "Step 6: Remove all unused vertices");
        V3DfgPasses::removeUnused(dfg);
        اگر (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("synth-rmunused");

        // No operation vertex should have multiple sinks. Cyclic decomoposition
        // depends on this and it can easily be ensured by using temporaries.
        // Also, all sources should be connected at this point
        اگر (v3Global.opt.debugCheck()) {
            برای (DfgVertex& vtx : dfg.opVertices()) {
                UASSERT_OBJ(!vtx.hasMultipleSinks(), &vtx, "Operation has multiple sinks");
                برای (size_t i = 0; i < vtx.nInputs(); ++i) {
                    UASSERT_OBJ(vtx.inputp(i), &vtx, "Unconnected source operand");
                }
            }
            V3DfgPasses::typeCheck(dfg);
        }
    }
};

صحیح AstToDfgSynthesize::AstVarScopeComparator::s_vscpIdCounter = 0;

// Decide which DfgLogic to attempt to synthesize
ایستا تهی dfgSelectLogicForSynthesis(DfgGraph& dfg) {
    // If we are told to synthesize everything, we will do so ...
    اگر (v3Global.opt.fDfgSynthesizeAll()) {
        برای (DfgVertex& vtx : dfg.opVertices()) {
            اگر (DfgLogic* ثابت logicp = vtx.cast<DfgLogic>()) logicp->setSelectedForSynthesis();
        }
        بازگشت;
    }

    // Otherwise figure out which vertices are likely worth synthesizing.

    std::vector<DfgVertexVar*> circularVarps;
    // Synthesize all drivers of circular variables
    {
        DfgUserMap<uint64_t> scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, scc);
        برای (DfgVertexVar& var : dfg.varVertices()) {
            اگر (!scc.at(var)) ادامه;
            // This is a circular variable
            circularVarps.emplace_back(&var);
        }
    }

    // To cover all drivers, use a work list
    DfgWorklist worklist{dfg};

    // Synthesize all drivers of circular variables
    برای (ثابت DfgVertexVar* ثابت varp : circularVarps) {
        varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
            worklist.افزودن_front(*driver.as<DfgLogic>());
            بازگشت نادرست;
        });
    }

    // Choose some simple special cases to always synthesize
    برای (DfgVertex& vtx : dfg.opVertices()) {
        DfgLogic* ثابت logicp = vtx.cast<DfgLogic>();
        اگر (!logicp || worklist.contains(*logicp)) ادامه;
        // If drives an unused variable, synthesize it so the partial logic can be removed
        اگر (logicp->drivesUnusedVars()) {
            worklist.افزودن_front(*logicp);
            ادامه;
        }
        // Blocks corresponding to continuous assignments
        اگر (logicp->nodep()->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            worklist.افزودن_front(*logicp);
            ادامه;
        }
        ثابت CfgGraph& cfg = logicp->cfg();
        // Straight line code with no branches
        اگر (cfg.nBlocks() == 1) {
            worklist.افزودن_front(*logicp);
            ادامه;
        }
        // Blocks driving exactly 1 variable
        اگر (!logicp->hasMultipleSinks()) worklist.افزودن_front(*logicp);
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
            اگر (!isVisited.درج(unresolved).second) بازگشت نادرست;
            unresolved->foreachSource([&](DfgVertex& sibling) {
                DfgLogic& siblingLogic = *sibling.as<DfgLogic>();
                اگر (!siblingLogic.selectedForSynthesis()) worklist.افزودن_front(siblingLogic);
                بازگشت نادرست;
            });
            بازگشت نادرست;
        });
    });
}

تهی V3DfgPasses::synthesize(DfgGraph& dfg, V3DfgContext& ctx) {
    // Select which DfgLogic to attempt to synthesize
    dfgSelectLogicForSynthesis(dfg);
    // Synthesize them - also removes un-synthesized DfgLogic, so must run even if nothing selected
    AstToDfgSynthesize::apply(dfg, ctx.m_synthContext);
}
