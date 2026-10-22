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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Cfg.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgPasses.h"
#अन्तर्भाव "V3EmitV.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <iterator>

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश {

// Create a DfgVertex out of a AstNodeExpr. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
ढाँचा <typename T_Vertex, typename T_गाठ>
T_Vertex* makeVertex(स्थिरांक T_गाठ* nodep, DfgGraph& dfg, स्थिरांक DfgDataType& dtype) {
    परत नवीन T_Vertex{dfg, nodep->fileline(), dtype};
}

ढाँचा <>
DfgArraySel* makeVertex<DfgArraySel, AstArraySel>(स्थिरांक AstArraySel* nodep, DfgGraph& dfg,
                                                  स्थिरांक DfgDataType& dtype) {
    // Some earlier passes create malformed ArraySels, just bail on those...
    // See t_bitsel_wire_array_bad
    जर (VN_IS(nodep->fromp(), Const)) परत nullptr;
    जर (!VN_IS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) परत nullptr;
    परत नवीन DfgArraySel{dfg, nodep->fileline(), dtype};
}

}  // namespace

// Visitor that can convert Ast statements and expressions in Dfg vertices
वर्ग AstToDfgConverter final : सार्वजनिक VNVisitor {
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

    तार्किक m_foundUnhandled = असत्य;  // Found node not implemented as DFG or not implemented 'visit'
    तार्किक m_converting = असत्य;  // We are trying to convert some logic at the moment

    // METHODS

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    ढाँचा <typename Vertex, typename... Args>
    Vertex* make(Args&&... args) {
        static_assert(!std::is_base_of<DfgVertexVar, Vertex>::मूल्य, "Do not use for variables");
        static_assert(std::is_base_of<DfgVertex, Vertex>::मूल्य, "'Vertex' must be a 'DfgVertex'");
        Vertex* स्थिरांक vtxp = नवीन Vertex{m_dfg, std::forward<Args>(args)...};
        m_logicp->synth().emplace_back(vtxp);
        परत vtxp;
    }

    // Returns true if the expression cannot (or should not) be represented by DFG
    तार्किक unhandled(Ast_गाठ_अभिव्यक्ती* nodep) {
        // Short-circuiting if something was already unhandled
        जर (m_foundUnhandled) {
            // Impure nodes cannot be represented
            जर (!nodep->isPure()) {
                m_foundUnhandled = सत्य;
                ++m_ctx.m_conv.nonRepImpure;
            }
        }
        परत m_foundUnhandled;
    }

    तार्किक isSupported(स्थिरांक AstVarRef* nodep) {
        // Cannot represent cross module references
        जर (nodep->classOrPackagep()) परत असत्य;
        // Check target
        परत V3Dfg::isSupported(nodep->varScopep());
    }

    // Given an RValue expression, return the equivalent Vertex, or nullptr if not representable.
    DfgVertex* convertRValue(Ast_गाठ_अभिव्यक्ती* nodep) {
        UASSERT_OBJ(!m_converting, nodep, "'convertingRValue' should not be called recursively");
        VL_RESTORER(m_converting);
        VL_RESTORER(m_foundUnhandled);
        m_converting = सत्य;
        m_foundUnhandled = असत्य;

        // Convert the expression
        iterate(nodep);

        // If falied to convert, return nullptr
        जर (m_foundUnhandled) परत nullptr;

        // Traversal set user2p to the equivalent vertex
        DfgVertex* स्थिरांक vtxp = nodep->user2u().to<DfgVertex*>();
        UASSERT_OBJ(vtxp, nodep, "Missing Dfg vertex after conversion");
        परत vtxp;
    }

    // Given an LValue expression, return the splice node that writes the
    // destination, together with the index to use for splicing in the value.
    // Returns {nullptr, 0}, if the given LValue expression is not supported.
    std::pair<DfgVertexSplice*, uint32_t> convertLValue(Ast_गाठ_अभिव्यक्ती* nodep) {
        जर (स्थिरांक AstVarRef* स्थिरांक vrefp = VN_CAST(nodep, VarRef)) {
            UASSERT_OBJ(vrefp->access().isWriteOnly(), vrefp, "Non-WriteOnly reference");
            जर (!isSupported(vrefp)) {
                ++m_ctx.m_conv.nonRepLValue;
                परत {nullptr, 0};
            }

            // Get (or create a new) temporary for this variable
            स्थिरांक DfgVertexVar* स्थिरांक vtxp = [&]() -> DfgVertexVar* {
                // The variable being assigned
                AstVarScope* स्थिरांक vscp = vrefp->varScopep();

                // Find existing one, if any
                साठी (स्थिरांक स्वतः& pair : *m_updatesp) {
                    जर (pair.first == vscp) परत pair.second;
                }

                // Create new one
                DfgVertexVar* स्थिरांक newp = createTmp(*m_logicp, vscp, "SynthAssign");
                m_updatesp->emplace_back(vscp, newp);

                // Create the Splice driver for the new temporary
                जर (newp->is<DfgVarPacked>()) {
                    newp->srcp(make<DfgSplicePacked>(newp->fileline(), newp->dtype()));
                } अन्यथा जर (newp->is<DfgVarArray>()) {
                    newp->srcp(make<DfgSpliceArray>(newp->fileline(), newp->dtype()));
                } अन्यथा {
                    nodep->v3fatalSrc("Unhandled DfgVertexVar sub-type");
                }

                // Use new temporary
                परत newp;
            }();

            // Return the Splice driver
            परत {vtxp->srcp()->as<DfgVertexSplice>(), 0};
        }

        जर (स्थिरांक AstSel* selp = VN_CAST(nodep, Sel)) {
            // Only handle constant selects
            स्थिरांक AstConst* स्थिरांक lsbp = VN_CAST(selp->lsbp(), Const);
            जर (!lsbp) {
                ++m_ctx.m_conv.nonRepLValue;
                परत {nullptr, 0};
            }
            uint32_t lsb = lsbp->toUInt();

            // Convert the 'fromp' sub-expression
            स्थिरांक स्वतः pair = convertLValue(selp->fromp());
            जर (!pair.first) परत {nullptr, 0};
            DfgSplicePacked* स्थिरांक splicep = pair.first->ढाँचा as<DfgSplicePacked>();
            // Adjust index.
            lsb += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            जर (lsb + static_cast<uint32_t>(selp->widthConst()) > splicep->size()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                परत {nullptr, 0};
            }

            // AstSel doesn't change type kind (array vs packed), so we can use
            // the existing splice driver with adjusted lsb
            परत {splicep, lsb};
        }

        जर (स्थिरांक AstArraySel* स्थिरांक aselp = VN_CAST(nodep, ArraySel)) {
            // Only handle constant selects
            स्थिरांक AstConst* स्थिरांक indexp = VN_CAST(aselp->bitp(), Const);
            जर (!indexp) {
                ++m_ctx.m_conv.nonRepLValue;
                परत {nullptr, 0};
            }
            uint32_t index = indexp->toUInt();

            // Convert the 'fromp' sub-expression
            स्थिरांक स्वतः pair = convertLValue(aselp->fromp());
            जर (!pair.first) परत {nullptr, 0};
            DfgSpliceArray* स्थिरांक splicep = pair.first->ढाँचा as<DfgSpliceArray>();
            // Adjust index. Note pair.second is always 0, but we might handle array slices later..
            index += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            जर (index + 1U > splicep->size()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                परत {nullptr, 0};
            }

            // Ensure the Splice driver exists for this element
            जर (!splicep->driverAt(index)) {
                File_ओळ* स्थिरांक flp = nodep->fileline();
                // This should never fail
                स्थिरांक DfgDataType& dtype = *DfgDataType::fromAst(nodep->dtypep());
                जर (dtype.isPacked()) {
                    DfgSplicePacked* स्थिरांक newp = make<DfgSplicePacked>(flp, dtype);
                    स्थिरांक DfgDataType& uaDtype = DfgDataType::array(dtype, 1);
                    DfgUnitArray* स्थिरांक uap = make<DfgUnitArray>(flp, uaDtype);
                    uap->srcp(newp);
                    splicep->addDriver(uap, index, flp);
                } अन्यथा जर (dtype.isArray()) {
                    DfgSpliceArray* स्थिरांक newp = make<DfgSpliceArray>(flp, dtype);
                    splicep->addDriver(newp, index, flp);
                } अन्यथा {
                    nodep->v3fatalSrc("Unhandled data type kind");
                }
            }

            // Return the splice driver
            DfgVertex* driverp = splicep->driverAt(index);
            जर (स्थिरांक DfgUnitArray* स्थिरांक uap = driverp->cast<DfgUnitArray>()) {
                driverp = uap->srcp();
            }
            परत {driverp->as<DfgVertexSplice>(), 0};
        }

        ++m_ctx.m_conv.nonRepLValue;
        परत {nullptr, 0};
    }

    // Given the LHS of an assignment, and the vertex representing the RHS,
    // connect up the RHS to drive the targets.
    // Returns true on success, false if the LHS is not representable.
    तार्किक convertAssignment(File_ओळ* flp, Ast_गाठ_अभिव्यक्ती* lhsp, DfgVertex* vtxp) {
        // Represents a DFG assignment contributed by the AST assignment with the above 'lhsp'.
        // There might be multiple of these if 'lhsp' is a concatenation.
        संरचना Assignment final {
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
        जर (!VN_IS(lhsp, VarRef)) lhsp = VN_AS(V3Const::constifyExpensiveEdit(lhsp), गाठ_अभिव्यक्ती);

        // Assigning compound expressions to a concatenated LHS requires a temporary
        // to avoid multiple use of the expression
        जर (VN_IS(lhsp, Concat) && !vtxp->is<DfgVertexVar>() && !vtxp->is<DfgConst>()) {
            DfgVertexVar* स्थिरांक tmpp = createTmp(*m_logicp, flp, vtxp->dtype(), "Unpack");
            tmpp->srcp(vtxp);
            vtxp = tmpp;
        }

        // Convert each concatenation LHS separately, gather all assignments
        // we need to do into 'assignments', return true if all LValues
        // converted successfully.
        std::vector<Assignment> assignments;
        स्थिरांक std::function<तार्किक(Ast_गाठ_अभिव्यक्ती*, uint32_t)> convertAllLValues
            = [&](Ast_गाठ_अभिव्यक्ती* subp, uint32_t lsb) -> तार्किक {
            // Concatenation on the LHS, convert each part
            जर (AstConcat* स्थिरांक concatp = VN_CAST(subp, Concat)) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक cRhsp = concatp->rhsp();
                Ast_गाठ_अभिव्यक्ती* स्थिरांक cLhsp = concatp->lhsp();
                // Convert Rigth of concat
                जर (!convertAllLValues(cRhsp, lsb)) परत असत्य;
                // Convert Left of concat
                परत convertAllLValues(cLhsp, lsb + cRhsp->width());
            }

            // Non-concatenation, convert the LValue
            स्थिरांक स्वतः pair = convertLValue(subp);
            जर (!pair.first) परत असत्य;

            // If whole lhs, just use it
            जर (subp == lhsp) {
                assignments.emplace_back(pair.first, pair.second, vtxp);
                परत सत्य;
            }

            // Otherwise select the relevant bits
            स्थिरांक DfgDataType& dtype = *DfgDataType::fromAst(subp->dtypep());
            DfgSel* स्थिरांक selp = make<DfgSel>(subp->fileline(), dtype);
            selp->fromp(vtxp);
            selp->lsb(lsb);
            assignments.emplace_back(pair.first, pair.second, selp);
            परत सत्य;
        };

        // Convert the given LHS assignment, give up if any LValues failed to convert
        जर (!convertAllLValues(lhsp, 0)) परत असत्य;

        // All successful, connect the drivers
        साठी (स्थिरांक Assignment& item : assignments) {
            जर (DfgSplicePacked* स्थिरांक spp = item.m_lhsp->ढाँचा cast<DfgSplicePacked>()) {
                spp->addDriver(item.m_rhsp, item.m_idx, flp);
            } अन्यथा जर (DfgSpliceArray* स्थिरांक sap = item.m_lhsp->ढाँचा cast<DfgSpliceArray>()) {
                // TODO: multi-dimensional arrays will need changes here
                स्थिरांक DfgDataType& rDt = item.m_rhsp->dtype();
                जर (rDt.isPacked()) {
                    // RHS is assigning an element of this array. Need a DfgUnitArray adapter.
                    DfgUnitArray* स्थिरांक uap = make<DfgUnitArray>(flp, DfgDataType::array(rDt, 1));
                    uap->srcp(item.m_rhsp);
                    sap->addDriver(uap, item.m_idx, flp);
                } अन्यथा {
                    // RHS is assigning an array (or array slice). Should be the same element type.
                    sap->addDriver(item.m_rhsp, item.m_idx, flp);
                }
            } अन्यथा {
                item.m_lhsp->v3fatalSrc("Unhandled DfgVertexSplice sub-type");
            }
        }

        परत सत्य;
    }

    // VISITORS

    // Unhandled node
    शून्य visit(Ast_गाठ* /*nodep*/) override {
        जर (!m_foundUnhandled && m_converting) ++m_ctx.m_conv.nonRepUnknown;
        m_foundUnhandled = सत्य;
    }

    // Expressions - mostly auto generated, but a few special ones
    शून्य visit(AstVarRef* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        जर (unhandled(nodep)) परत;
        // This visit method is only called on RValues, where only read refs are supported
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Non-ReadOnly reference");
        जर (!isSupported(nodep)) {
            m_foundUnhandled = सत्य;
            ++m_ctx.m_conv.nonRepVarRef;
            परत;
        }

        // Variable should have been bound before starting conversion
        DfgVertex* स्थिरांक vtxp = nodep->varScopep()->user2u().ढाँचा to<DfgVertexVar*>();
        UASSERT_OBJ(vtxp, nodep, "Referenced variable has no associated DfgVertexVar");
        nodep->user2p(vtxp);
    }
    शून्य visit(AstConst* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        जर (unhandled(nodep)) परत;

        जर (nodep->width() != nodep->अंक().width()) {
            // Sometimes the width of the AstConst is not the same as the
            // V3Number it holds. Truncate it here. TODO: should this be allowed?
            V3_संख्या अंक{nodep, nodep->width()};
            अंक.opSel(nodep->अंक(), nodep->width() - 1, 0);
            DfgVertex* स्थिरांक vtxp = make<DfgConst>(nodep->fileline(), अंक);
            nodep->user2p(vtxp);
        } अन्यथा {
            DfgVertex* स्थिरांक vtxp = make<DfgConst>(nodep->fileline(), nodep->अंक());
            nodep->user2p(vtxp);
        }
    }
    शून्य visit(AstCReset* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        जर (unhandled(nodep)) परत;

        स्थिरांक DfgDataType* स्थिरांक dtypep = DfgDataType::fromAst(nodep->dtypep());
        जर (!dtypep) {
            m_foundUnhandled = सत्य;
            ++m_ctx.m_conv.nonRepDType;
            परत;
        }

        UASSERT_OBJ(!nodep->constructing(), nodep,
                    "CReset should be non-constructing at this stage");

        DfgVertex* स्थिरांक vtxp = make<DfgCReset>(nodep->fileline(), *dtypep);
        nodep->user2p(vtxp);
    }
    शून्य visit(AstMatchMasked* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        जर (unhandled(nodep)) परत;

        स्थिरांक DfgDataType* स्थिरांक dtypep = DfgDataType::fromAst(nodep->dtypep());
        जर (!dtypep) {
            m_foundUnhandled = सत्य;
            ++m_ctx.m_conv.nonRepDType;
            परत;
        }

        iterate(nodep->lhsp());
        जर (m_foundUnhandled) परत;
        iterate(nodep->matchp());
        जर (m_foundUnhandled) परत;

        File_ओळ* स्थिरांक flp = nodep->fileline();
        DfgMatchMasked* स्थिरांक vtxp = make<DfgMatchMasked>(flp, *dtypep);
        vtxp->lhsp(nodep->lhsp()->user2u().to<DfgVertex*>());
        vtxp->matchp(nodep->matchp()->user2u().to<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    शून्य visit(AstReplicate* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        जर (unhandled(nodep)) परत;

        स्थिरांक DfgDataType* स्थिरांक dtypep = DfgDataType::fromAst(nodep->dtypep());
        जर (!dtypep) {
            m_foundUnhandled = सत्य;
            ++m_ctx.m_conv.nonRepDType;
            परत;
        }

        iterate(nodep->srcp());
        जर (m_foundUnhandled) परत;

        File_ओळ* स्थिरांक flp = nodep->fileline();
        DfgRep* स्थिरांक vtxp = make<DfgRep>(flp, *dtypep);
        vtxp->srcp(nodep->srcp()->user2u().to<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    शून्य visit(AstSel* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        जर (unhandled(nodep)) परत;

        स्थिरांक DfgDataType* स्थिरांक dtypep = DfgDataType::fromAst(nodep->dtypep());
        जर (!dtypep) {
            m_foundUnhandled = सत्य;
            ++m_ctx.m_conv.nonRepDType;
            परत;
        }

        iterate(nodep->fromp());
        जर (m_foundUnhandled) परत;

        File_ओळ* स्थिरांक flp = nodep->fileline();
        DfgVertex* vtxp = nullptr;
        जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->lsbp(), Const)) {
            स्थिरांक uint32_t lsb = constp->toUInt();
            स्थिरांक uint32_t msb = lsb + nodep->widthConst() - 1;
            DfgVertex* स्थिरांक fromp = nodep->fromp()->user2u().to<DfgVertex*>();
            // Unfortunately we can still have out of bounds selects due to how
            // indices are truncated for speed reasons in V3Width/V3Unknown.
            जर (msb >= fromp->size()) {
                m_foundUnhandled = सत्य;
                ++m_ctx.m_conv.nonRepOOBSel;
                परत;
            }
            DfgSel* स्थिरांक selp = make<DfgSel>(flp, *dtypep);
            selp->fromp(fromp);
            selp->lsb(lsb);
            vtxp = selp;
        } अन्यथा {
            iterate(nodep->lsbp());
            जर (m_foundUnhandled) परत;
            DfgMux* स्थिरांक muxp = make<DfgMux>(flp, *dtypep);
            muxp->fromp(nodep->fromp()->user2u().to<DfgVertex*>());
            muxp->lsbp(nodep->lsbp()->user2u().to<DfgVertex*>());
            vtxp = muxp;
        }
        nodep->user2p(vtxp);
    }

// The rest of the visit methods for expressions are generated by 'astgen'
#अन्तर्भाव "V3Dfg__gen_ast_to_dfg.h"

सार्वजनिक:
    // PUBLIC METHODS

    // Create temporay variable capable of holding the given type
    DfgVertexVar* createTmp(DfgLogic& logic, File_ओळ* flp, स्थिरांक DfgDataType& dtype,
                            स्थिरांक std::string& prefix) {
        DfgVertexVar* स्थिरांक vtxp = m_dfg.makeNewVar(flp, prefix, dtype, logic.scopep());
        logic.synth().emplace_back(vtxp);
        vtxp->vscp()->varp()->isInternal(सत्य);
        vtxp->tmpForp(vtxp->vscp());
        परत vtxp;
    }

    // Create a new temporary variable capable of holding 'varp'
    DfgVertexVar* createTmp(DfgLogic& logic, AstVarScope* vscp, स्थिरांक std::string& prefix) {
        AstVar* स्थिरांक astVarp = vscp->varp();
        File_ओळ* स्थिरांक flp = astVarp->fileline();
        स्थिरांक DfgDataType& dtype = *DfgDataType::fromAst(astVarp->dtypep());
        स्थिरांक std::string prfx = prefix + "_" + astVarp->name();
        DfgVertexVar* स्थिरांक vtxp = createTmp(logic, flp, dtype, prfx);
        vtxp->tmpForp(vscp);
        परत vtxp;
    }

    // Convert AstAssign to Dfg, return true if successful.
    // Fills 'updates' with bindings for assigned variables.
    तार्किक convert(std::vector<std::pair<AstVarScope*, DfgVertexVar*>>& updates, DfgLogic& vtx,
                 Ast_गाठ_Assign* nodep) {
        UASSERT_OBJ(VN_IS(nodep, Assign) || VN_IS(nodep, AssignW), nodep, "Bad NodeAssign");
        UASSERT_OBJ(updates.empty(), nodep, "'updates' should be empty");
        VL_RESTORER(m_updatesp);
        VL_RESTORER(m_logicp);
        m_updatesp = &updates;
        m_logicp = &vtx;
        // Assignment with timing control shouldn't make it this far
        UASSERT_OBJ(!nodep->timingControlp(), nodep, "Shouldn't make it this far");
        // Convert it
        ++m_ctx.m_conv.आदान_Assignments;
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp();
        // Check data types are compatible.
        स्थिरांक DfgDataType* स्थिरांक lDtypep = DfgDataType::fromAst(lhsp->dtypep());
        स्थिरांक DfgDataType* स्थिरांक rDtypep = DfgDataType::fromAst(rhsp->dtypep());
        जर (!lDtypep || !rDtypep) {
            ++m_ctx.m_conv.nonRepDType;
            परत असत्य;
        }
        // For now, only direct array assignment is supported (e.g. a = b, but not a = _ ? b : c)
        जर (rDtypep->isArray()) {
            जर (!VN_IS(rhsp, VarRef)
                || !lhsp->dtypep()->skipRefp()->same_वृक्ष(rhsp->dtypep()->skipRefp())) {
                ++m_ctx.m_conv.nonRepDType;
                परत असत्य;
            }
        }
        // Widths should match at this point
        UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Mismatched width reached DFG");
        // Convert the RHS expression
        DfgVertex* स्थिरांक rVtxp = convertRValue(rhsp);
        जर (!rVtxp) परत असत्य;
        // Connect the RHS vertex to the LHS targets
        स्थिरांक तार्किक success = convertAssignment(nodep->fileline(), lhsp, rVtxp);
        जर (success) ++m_ctx.m_conv.representable;
        परत success;
    }

    // Convert RValue expression to Dfg. Returns nullptr if failed.
    DfgVertex* convert(DfgLogic& vtx, Ast_गाठ_अभिव्यक्ती* nodep) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;
        // Convert it
        ++m_ctx.m_conv.आदान_Expressions;
        DfgVertex* स्थिरांक vtxp = convertRValue(nodep);
        जर (vtxp) ++m_ctx.m_conv.representable;
        परत vtxp;
    }

    // CONSTRUCTOR
    AstToDfgConverter(DfgGraph& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {}
};

// Debug aid - outisde 'AstToDfgSynthesize' as it is a template, but want one instance
V3DebugBisect s_dfgSynthDebugBisect{"DfgSynthesize"};

वर्ग AstToDfgSynthesize final {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // TYPES

    // SymTab must be ordered in order to yield stable results
    संरचना AstVarScopeComparator final {
        स्थिर पूर्णांक s_vscpIdCounter;  // Counter for lazily allocating the unique AstVarScope IDs
        तार्किक चालक()(AstVarScope* lhs, AstVarScope* rhs) स्थिरांक {
            जर (!lhs->user4()) lhs->user4(++s_vscpIdCounter);
            जर (!rhs->user4()) rhs->user4(++s_vscpIdCounter);
            परत lhs->user4() < rhs->user4();
        }
    };
    using SymTab = std::map<AstVarScope*, DfgVertexVar*, AstVarScopeComparator>;

    // Represents a [potentially partial] driver of a variable
    संरचना Driver final {
        DfgVertex* m_vtxp = nullptr;  // Driving vertex
        uint32_t m_lo = 0;  // Low index of driven range (internal, not Verilog)
        uint32_t m_hi = 0;  // High index of driven range (internal, not Verilog)
        File_ओळ* m_flp = nullptr;  // Location of driver in source

        Driver() = पूर्वनिर्धारित;
        Driver(DfgVertex* vtxp, uint32_t lo, File_ओळ* flp)
            : m_vtxp{vtxp}
            , m_lo{lo}
            , m_hi{lo + vtxp->size() - 1U}
            , m_flp{flp} {}
        चालक तार्किक() स्थिरांक { परत m_vtxp != nullptr; }

        तार्किक चालक<(स्थिरांक Driver& other) स्थिरांक {
            जर (m_lo != other.m_lo) परत m_lo < other.m_lo;
            जर (m_hi != other.m_hi) परत m_hi < other.m_hi;
            परत m_flp->operatorCompare(*other.m_flp) < 0;
        }

        तार्किक चालक<=(स्थिरांक Driver& other) स्थिरांक { परत !(other < *हे); }
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
    Cfg_Dominator_वृक्ष m_dom_वृक्ष;  // The dominator tree of the current CFG

    // STATE - Some debug aid
    // We stop after synthesizing s_dfgSynthDebugLimit vertices (if non-zero).
    // This is the problematic logic (last one we synthesize), assuming a
    // bisection search over s_dfgSynthDebugLimit.
    DfgLogic* m_debugLogicp = nullptr;
    // Source (upstream) cone of outputs of m_debugLogicp
    std::unique_ptr<std::unordered_set<स्थिरांक DfgVertex*>> m_debugOSrcConep{nullptr};

    // METHODS

    // Dump current graph for debugging ...
    शून्य debugDump(स्थिरांक अक्षर* name) {
        // If we have the debugged logic, compute the vertices feeding its outputs
        जर (VL_UNLIKELY(m_debugLogicp)) {
            std::vector<स्थिरांक DfgVertex*> outputs;
            m_debugLogicp->foreachSink([&outputs](स्थिरांक DfgVertex& v) {
                outputs.emplace_back(v.singleSink()->as<DfgVertexVar>());
                परत असत्य;
            });
            m_debugOSrcConep = m_dfg.sourceCone(outputs);
        }

        जर (VL_UNLIKELY(dumpDfgLevel() >= 9 || m_debugOSrcConep)) {
            m_dfg.dumpDotFilePrefixed(name);
            जर (m_debugOSrcConep) {
                // Dump only the subgraph involving the inputs and outputs of the bad vertex
                m_dfg.dumpDotFilePrefixed(name + "-min"s, [&](स्थिरांक DfgVertex& v) -> तार्किक {
                    परत m_debugOSrcConep->मोजणी(&v);
                });
            }
        }
    }

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    ढाँचा <typename Vertex, typename... Args>
    Vertex* make(Args&&... args) {
        static_assert(!std::is_base_of<DfgVertexVar, Vertex>::मूल्य, "Do not use for variables");
        static_assert(std::is_base_of<DfgVertex, Vertex>::मूल्य, "'Vertex' must be a 'DfgVertex'");
        Vertex* स्थिरांक vtxp = नवीन Vertex{m_dfg, std::forward<Args>(args)...};
        जर (m_logicp) m_logicp->synth().emplace_back(vtxp);
        परत vtxp;
    }

    // Gather all drivers of a resolved variable
    स्थिर std::vector<Driver> gatherDrivers(DfgVertexSplice* vtxp) {
        // Collect them all, check if they are sorted
        std::vector<Driver> drivers;
        drivers.reserve(vtxp->nInputs());
        तार्किक sorted = सत्य;
        vtxp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_ओळ* flp) {
            // Collect the driver
            drivers.emplace_back(&src, lo, flp);
            // Check if drivers are sorted - most often they are
            स्थिरांक माप_प्रकार n = drivers.size();
            जर (n >= 2 && drivers[n - 1] < drivers[n - 2]) sorted = असत्य;
            परत असत्य;
        });

        // Sort if unsorted
        जर (!sorted) std::stable_sort(drivers.begin(), drivers.end());

        // Done
        परत drivers;
    }

    // Returns true if the driver supplies a complete array element.
    स्थिर तार्किक driverCoversWholeElement(स्थिरांक Driver& driver) {
        स्थिरांक DfgUnitArray* स्थिरांक unitp = driver.m_vtxp->cast<DfgUnitArray>();
        जर (!unitp) परत असत्य;
        जर (DfgVertexSplice* स्थिरांक splicep = unitp->srcp()->cast<DfgVertexSplice>()) {
            परत splicep->wholep();
        }
        परत सत्य;
    }

    // Unlike 'wholep', several element drivers can collectively cover an array.
    स्थिर तार्किक spliceCoversWhole(DfgVertexSplice* स्थिरांक splicep) {
        जर (splicep->wholep()) परत सत्य;
        जर (splicep->isPacked()) परत असत्य;
        uint32_t next = 0;
        साठी (स्थिरांक Driver& driver : gatherDrivers(splicep)) {
            जर (driver.m_lo != next || !driverCoversWholeElement(driver)) परत असत्य;
            next = driver.m_hi + 1;
        }
        परत next == splicep->size();
    }

    // Returns true if the driver cone contains any variable introduced by
    // tristate lowering. Used to distinguish intentional tristate contributor
    // overlap from accidental multidrive.
    स्थिर तार्किक containsTriLoweredVar(DfgVertex* rootp) {
        std::vector<स्थिरांक DfgVertex*> चळत;
        std::vector<स्थिरांक DfgVertex*> visited;
        चळत.emplace_back(rootp);
        पर्यंत (!चळत.empty()) {
            स्थिरांक DfgVertex* स्थिरांक vtxp = चळत.back();
            चळत.काढा_back();
            जर (std::find(visited.begin(), visited.end(), vtxp) != visited.end()) सुरूठेवा;
            visited.emplace_back(vtxp);
            जर (स्थिरांक DfgVertexVar* स्थिरांक varp = vtxp->cast<DfgVertexVar>()) {
                जर (varp->vscp()->varp()->dfgTriLowered()) परत सत्य;
            }
            vtxp->foreachSource([&](स्थिरांक DfgVertex& src) {
                चळत.emplace_back(&src);
                परत असत्य;
            });
        }
        परत असत्य;
    }

    // Returns true if the driver is a direct variable forward (no logic).
    स्थिर तार्किक isDirectVarDriver(स्थिरांक DfgVertex* vtxp) { परत vtxp->is<DfgVertexVar>(); }

    // Gather all synthesized drivers of an unresolved variable
    स्थिर std::vector<Driver> gatherDriversUnresolved(DfgUnresolved* vtxp) {
        std::vector<Driver> drivers;
        drivers.reserve(vtxp->nInputs());

        // For better locations in error reporting, we unpick concatenations
        // which are sometimes introduced by combinint assignments in V3Const.
        स्थिरांक std::function<शून्य(DfgVertex*, uint32_t, File_ओळ*)> gather
            = [&](DfgVertex* vtxp, uint32_t lo, File_ओळ* flp) -> शून्य {
            जर (DfgConcat* स्थिरांक concatp = vtxp->cast<DfgConcat>()) {
                DfgVertex* स्थिरांक rhsp = concatp->rhsp();
                gather(rhsp, lo, rhsp->fileline());
                DfgVertex* स्थिरांक lhsp = concatp->lhsp();
                gather(lhsp, lo + rhsp->width(), lhsp->fileline());
                परत;
            }
            drivers.emplace_back(vtxp, lo, flp);
        };

        // Gather all synthesized drivers
        vtxp->foreachSource([&](DfgVertex& src) {
            // Can ignore the original DfgLogic
            जर (src.is<DfgLogic>()) परत असत्य;
            // Synthesized drivers must be a splice at this point
            DfgVertexSplice* स्थिरांक splicep = src.as<DfgVertexSplice>();
            // Collect the driver
            splicep->foreachDriver([&](DfgVertex& src, uint32_t lo, File_ओळ* flp) {
                gather(&src, lo, flp);
                परत असत्य;
            });
            परत असत्य;
        });

        // Sort the drivers
        std::stable_sort(drivers.begin(), drivers.end());

        // Done
        परत drivers;
    }

    // Given two drivers, combine the driven sub-ranges into the first one if
    // possible. First bool returned indicates successfully combined and there
    // are no multi-driven bits. Second bool returned indicates we warned
    // already about multi-driven bits.
    std::pair<तार्किक, तार्किक> combineDrivers(DfgVertexVar& var, स्थिरांक std::string& sub,  //
                                         Driver& a, स्थिरांक Driver& b) {
        // We can only combine array drivers ...
        जर (a.m_vtxp->isPacked()) परत {असत्य, असत्य};
        // ... that both drive a single element ...
        जर (a.m_lo != b.m_lo) परत {असत्य, असत्य};
        स्थिरांक DfgUnitArray* स्थिरांक aUap = a.m_vtxp->ढाँचा cast<DfgUnitArray>();
        जर (!aUap) परत {असत्य, असत्य};
        स्थिरांक DfgUnitArray* स्थिरांक bUap = b.m_vtxp->ढाँचा cast<DfgUnitArray>();
        जर (!bUap) परत {असत्य, असत्य};
        // ... and are themeselves partial
        DfgSplicePacked* स्थिरांक aSp = aUap->srcp()->ढाँचा cast<DfgSplicePacked>();
        जर (!aSp) परत {असत्य, असत्य};
        DfgSplicePacked* स्थिरांक bSp = bUap->srcp()->ढाँचा cast<DfgSplicePacked>();
        जर (!bSp) परत {असत्य, असत्य};
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
        जर (!normalizeDrivers(var, abDrivers, sub + "[" + std::to_string(a.m_lo) + "]")) {
            परत {असत्य, सत्य};
        }

        // Successfully resolved. Needs a new splice and unit.
        File_ओळ* स्थिरांक flp = var.fileline();
        DfgSplicePacked* स्थिरांक splicep = make<DfgSplicePacked>(flp, aSp->dtype());
        साठी (स्थिरांक Driver& d : abDrivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        DfgUnitArray* स्थिरांक uap = make<DfgUnitArray>(flp, aUap->dtype());
        uap->srcp(splicep);
        a.m_vtxp = uap;
        परत {सत्य, असत्य};
    }

    // Combine and coalesce the given drivers.
    // Returns true iff no multi-driven bits are present.
    तार्किक normalizeDrivers(DfgVertexVar& var, std::vector<Driver>& drivers,
                          स्थिरांक std::string& sub = "") {
        जर (drivers.empty()) परत सत्य;

        // What type of values are we combining
        स्थिरांक तार्किक isPacked = drivers[0].m_vtxp->isPacked();

        // Found a multidriven part ?
        तार्किक multiDriven = असत्य;

        // Iterate through the sorted drivers. Index 'i' is the driver we are
        // resolving driver 'j' agains, and if required, we merge 'j' into 'i'.
        माप_प्रकार i = 0;
        साठी (माप_प्रकार j = 1; j < drivers.size();) {
            UASSERT_OBJ(i < j, &var, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            जर (!jD) {
                ++j;
                सुरूठेवा;
            }
            // If 'i' was moved, move 'j' in it's place
            जर (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                सुरूठेवा;
            }

            // We have 2 valid drivers now
            UASSERT_OBJ(iD <= jD, &var, "Should always be sorted");
            UASSERT_OBJ(jD.m_vtxp->isPacked() == isPacked, &var, "Mixed type drivers");

            // If no overlap, consider next pair
            जर (iD.m_hi < jD.m_lo) {
                ++i;
                जर (i == j) ++j;
                सुरूठेवा;
            }

            // There is an overlap. Attempt to combine them.
            तार्किक combined = असत्य;
            तार्किक warned = असत्य;
            std::tie(combined, warned) = combineDrivers(var, sub, iD, jD);

            // If sucessfully combined, 'j' is no longer needed, it was combined into 'i'
            जर (combined) {
                jD = Driver{};
                ++j;
                सुरूठेवा;
            }

            // Found overlap that cannot be resolved
            multiDriven = सत्य;
            // Compare next driver
            ++j;

            // Do not warn again if we warned during 'combineDrivers'
            जर (warned) सुरूठेवा;

            // The variable to warn on
            AstVarScope* स्थिरांक vscp = var.tmpForp() ? var.tmpForp() : var.vscp();

            // Loop index often abused, so suppress
            जर (vscp->varp()->isUsedLoopIdx()) सुरूठेवा;
            // Tristate lowering can intentionally create overlapping contributors.
            // Keep the signal marked multidriven for DFG fallback, but suppress
            // warning only when both overlapping driver cones look tri-lowered.
            जर (vscp->varp()->dfgAllowMultidriveTri()) {
                स्थिरांक तार्किक iTri = containsTriLoweredVar(iD.m_vtxp);
                स्थिरांक तार्किक jTri = containsTriLoweredVar(jD.m_vtxp);
                स्थिरांक तार्किक triPair = iTri && jTri;
                स्थिरांक तार्किक triAndBridge = (iTri && isDirectVarDriver(jD.m_vtxp))
                                          || (jTri && isDirectVarDriver(iD.m_vtxp));
                जर (triPair || triAndBridge) सुरूठेवा;
            }

            // Warn the user now
            स्थिरांक std::string lo = std::to_string(jD.m_lo);
            स्थिरांक std::string hi = std::to_string(std::min(iD.m_hi, jD.m_hi));
            स्थिरांक std::string kind = isPacked ? "Bit" : "Element";
            स्थिरांक std::string part = hi == lo ? (" [" + lo + "]") : ("s [" + hi + ":" + lo + "]");

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
        जर (!multiDriven && isPacked) coalesceDrivers(drivers);

        परत !multiDriven;
    }

    // Coalesce adjacent drivers into single ones
    शून्य coalesceDrivers(std::vector<Driver>& drivers) {
        UASSERT(!drivers.empty(), "Can't coalesce 0 drivers");
        UASSERT_OBJ(drivers[0].m_vtxp->isPacked(), drivers[0].m_vtxp, "Can only coalesce packed");

        माप_प्रकार i = 0;
        साठी (माप_प्रकार j = 1; j < drivers.size();) {
            UASSERT(i < j, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            जर (!jD) {
                ++j;
                सुरूठेवा;
            }
            // If 'i' was moved, move 'j' in it's place
            जर (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                सुरूठेवा;
            }

            // We have 2 valid drivers now
            UASSERT(iD <= jD, "Should always be sorted");

            // If not adjacent, move on
            जर (iD.m_hi + 1 != jD.m_lo) {
                ++i;
                जर (i == j) ++j;
                सुरूठेवा;
            }

            // Coalesce Adjacent ranges,
            स्थिरांक DfgDataType& dt = DfgDataType::packed(iD.m_vtxp->width() + jD.m_vtxp->width());
            DfgConcat* स्थिरांक concatp = make<DfgConcat>(iD.m_flp, dt);
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
    DfgVertexSplice* makeSplice(DfgVertexVar& var, स्थिरांक std::vector<Driver>& newDrivers) {
        UASSERT_OBJ(!newDrivers.empty(), &var, "'makeSplice' called with no new drivers");
        // Create new driver
        DfgVertexSplice* splicep = nullptr;
        जर (var.is<DfgVarPacked>()) {
            splicep = make<DfgSplicePacked>(var.fileline(), var.dtype());
        } अन्यथा जर (var.is<DfgVarArray>()) {
            splicep = make<DfgSpliceArray>(var.fileline(), var.dtype());
        } अन्यथा {
            var.v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        साठी (स्थिरांक Driver& d : newDrivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        परत splicep;
    }

    // Initialzie input symbol table of entry CfgBlock
    शून्य initialize_Entry_चिन्ह_Table(SymTab& iSymTab) {
        m_logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* स्थिरांक vvp = src.as<DfgVertexVar>();
            iSymTab[vvp->vscp()] = vvp;
            परत असत्य;
        });
    }

    // Join variable drivers across a control flow confluence (insert muxes ...)
    DfgVertexVar* joinDrivers(AstVarScope* vscp, DfgVertexVar* predicatep,  //
                              DfgVertexVar* thenp, DfgVertexVar* elsep) {
        AstVarScope* स्थिरांक thenVscp = thenp->tmpForp() ? thenp->tmpForp() : thenp->vscp();
        AstVarScope* स्थिरांक elseVscp = elsep->tmpForp() ? elsep->tmpForp() : elsep->vscp();
        UASSERT_OBJ(thenVscp == elseVscp, vscp, "Attempting to join unrelated variables");

        // If both bindings are the the same (variable not updated through either path),
        // then there is nothing to do, can use the same binding
        जर (thenp == elsep) परत thenp;

        // We can't join the input variable just yet, so bail
        जर (thenp->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_आदान;
            परत nullptr;
        }
        जर (elsep->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_आदान;
            परत nullptr;
        }

        // Can't do arrays yet
        जर (!thenp->isPacked()) {
            ++m_ctx.m_synt.nonSynArray;
            परत nullptr;
        }

        // Gather drivers of 'thenp' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* स्थिरांक tDefaultp = thenp->defaultp();
        std::vector<Driver> tDrivers = gatherDrivers(thenp->srcp()->as<DfgVertexSplice>());

        // Gather drivers of 'elsep' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* स्थिरांक eDefaultp = elsep->defaultp();
        std::vector<Driver> eDrivers = gatherDrivers(elsep->srcp()->as<DfgVertexSplice>());

        // Default drivers should be the same or not present on either
        UASSERT_OBJ(tDefaultp == eDefaultp, vscp, "Different default drivers");

        // Location to use for the join vertices
        File_ओळ* स्थिरांक flp = predicatep->fileline();

        // Create a fresh temporary for the joined value
        DfgVertexVar* स्थिरांक joinp = m_converter.createTmp(*m_logicp, vscp, "SynthJoin");
        DfgVertexSplice* स्थिरांक joinSplicep = make<DfgSplicePacked>(flp, joinp->dtype());
        joinp->srcp(joinSplicep);

        // If both paths are fully driven, just create a simple conditional
        जर (tDrivers.size() == 1  //
            && tDrivers[0].m_lo == 0  //
            && tDrivers[0].m_hi == thenp->width() - 1  //
            && eDrivers.size() == 1  //
            && eDrivers[0].m_lo == 0  //
            && eDrivers[0].m_hi == elsep->width() - 1) {
            UASSERT_OBJ(!tDefaultp, vscp, "Fully driven variable have default driver");

            DfgCond* स्थिरांक condp = make<DfgCond>(flp, joinp->dtype());
            condp->condp(predicatep);
            condp->thenp(thenp);
            condp->elsep(elsep);
            joinSplicep->addDriver(condp, 0, tDrivers[0].m_flp);

            // Done
            परत joinp;
        }

        // Otherwise we need to merge them part by part

        // If different bits are driven, then some might not have been assigned.. Latch?
        जर (tDrivers.size() != eDrivers.size()) {
            ++m_ctx.m_synt.nonSynLatch;
            परत nullptr;
        }

        साठी (माप_प्रकार i = 0; i < tDrivers.size(); ++i) {
            स्थिरांक Driver& tDriver = tDrivers[i];
            स्थिरांक Driver& eDriver = eDrivers[i];
            // If different bits are driven, then some might not have been assigned.. Latch?
            जर (tDriver.m_lo != eDriver.m_lo || tDriver.m_hi != eDriver.m_hi) {
                ++m_ctx.m_synt.nonSynLatch;
                परत nullptr;
            }

            स्थिरांक DfgDataType& dtype = DfgDataType::packed(tDriver.m_hi - tDriver.m_lo + 1);
            DfgCond* स्थिरांक condp = make<DfgCond>(flp, dtype);
            condp->condp(predicatep);

            // We actally need to select the bits from the joined variables, not use the drivers
            DfgSel* स्थिरांक thenSelp = make<DfgSel>(flp, tDriver.m_vtxp->dtype());
            thenSelp->lsb(tDriver.m_lo);
            thenSelp->fromp(thenp);
            condp->thenp(thenSelp);

            // Same for the 'else' part
            DfgSel* स्थिरांक elseSelp = make<DfgSel>(flp, eDriver.m_vtxp->dtype());
            elseSelp->lsb(eDriver.m_lo);
            elseSelp->fromp(elsep);
            condp->elsep(elseSelp);

            // Add it as a driver to the join
            joinSplicep->addDriver(condp, tDriver.m_lo, tDriver.m_flp);
        }

        // If there was a default driver, add it to te join
        जर (tDefaultp) joinp->defaultp(tDefaultp);

        // Done
        परत joinp;
    }

    // Merge 'thenSymTab' into 'elseSymTab' using the given predicate to join values
    तार्किक join_चिन्ह_Tables(SymTab& elseSymTab, DfgVertexVar* predicatep, स्थिरांक SymTab& thenSymTab) {
        // Any variable that does not have a binding on both paths will be removed. These might be
        // temporaries, loop vars, etc used only in one branch. Conversion will fail if the
        // variable is actually referenced later.
        std::vector<AstVarScope*> toRemove;

        // Join each symbol
        साठी (std::pair<AstVarScope* स्थिरांक, DfgVertexVar*>& pair : elseSymTab) {
            AstVarScope* स्थिरांक varp = pair.first;
            // Find same variable on the else path
            स्थिरांक स्वतः it = thenSymTab.find(varp);
            // Record for removal if not assigned on both paths
            जर (it == thenSymTab.end()) {
                toRemove.emplace_back(varp);
                सुरूठेवा;
            }
            // Join paths with the predicate
            DfgVertexVar* स्थिरांक thenp = it->second;
            DfgVertexVar* स्थिरांक elsep = pair.second;
            DfgVertexVar* स्थिरांक newp = joinDrivers(varp, predicatep, thenp, elsep);
            जर (!newp) परत असत्य;
            pair.second = newp;
        }

        // Remove variables not assigned on both paths
        साठी (AstVarScope* स्थिरांक varp : toRemove) elseSymTab.erase(varp);

        // Done
        परत सत्य;
    }

    // Given two joining control flow edges, compute how to join their symbols.
    // Returns the predicaete to join over, and the 'then' and 'else' blocks.
    std::tuple<DfgVertexVar*, स्थिरांक CfgBlock*, स्थिरांक CfgBlock*>  //
    howToJoin(स्थिरांक CfgEdge* स्थिरांक ap, स्थिरांक CfgEdge* स्थिरांक bp) {
        // Find the closest common dominator of the two paths
        स्थिरांक CfgBlock* स्थिरांक domp = m_dom_वृक्ष.closestCommonDominator(ap->srcp(), bp->srcp());
        // These paths join here, so 'domp' must be a branch, otherwise it's not the closest
        UASSERT_OBJ(domp->isBranch(), domp, "closestCommonDominator is not a branch");

        // The branches of the common dominator
        स्थिरांक CfgEdge* स्थिरांक takenEdgep = domp->takenEdgep();
        स्थिरांक CfgEdge* स्थिरांक untknEdgep = domp->untknEdgep();

        // We check if the taken branch dominates the path to either blocks,
        // and if the untaken branch dominates the path to the other block.
        // If so, we can use the branch condition as predicate, otherwise
        // we must use the path predicate as there are ways to get from one
        // branch of the dominator to the other. We need to be careful if
        // either branches are directly to the join block. This is fine,
        // it's as if there was an empty block on that critical edge which
        // is dominated by that path.

        जर (takenEdgep == ap || m_dom_वृक्ष.dominates(takenEdgep->dstp(), ap->srcp())) {
            जर (untknEdgep == bp || m_dom_वृक्ष.dominates(untknEdgep->dstp(), bp->srcp())) {
                // Taken path dominates 'ap' and untaken dominates 'bp', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                परत std::make_tuple(m_bbToCondp[domp], ap->srcp(), bp->srcp());
            }
        } अन्यथा जर (takenEdgep == bp || m_dom_वृक्ष.dominates(takenEdgep->dstp(), bp->srcp())) {
            जर (untknEdgep == ap || m_dom_वृक्ष.dominates(untknEdgep->dstp(), ap->srcp())) {
                // Taken path dominates 'bp' and untaken dominates 'ap', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                परत std::make_tuple(m_bbToCondp[domp], bp->srcp(), ap->srcp());
            }
        }

        // The branches don't dominate the joined blocks, must use the path predicate
        ++m_ctx.m_synt.joinUsingPathPredicate;

        // TODO: We could do better here: use the path predicate of the closest
        // cominating blocks, pick the one from the lower rank, etc, but this
        // generic case is very rare, most synthesizable logic has
        // series-parallel CFGs which are covered by the earlier cases.
        परत std::make_tuple(m_edgeToPredicatep[ap], ap->srcp(), bp->srcp());
    }

    // Combine the output symbol tables of the predecessors of the given
    // block to compute the input symtol table for the given block.
    तार्किक create_आदान_चिन्ह_Table(स्थिरांक CfgBlock& bb) {
        // The input symbol table of the given block, we are computing it now
        SymTab& joined = m_bbToISymTab[bb];

        // Input symbol table of entry block is special
        जर (bb.isEnter()) {
            initialize_Entry_चिन्ह_Table(joined);
            परत सत्य;
        }

        // Current input symbol table should be empty, we will fill it in here
        UASSERT(joined.empty(), "Unprocessed input symbol table should be empty");

        // Fast path if there is only one predecessor - TODO: use less copying
        जर (!bb.isJoin()) {
            joined = m_bbToOSymTab[bb.firstPredecessorp()];
            परत सत्य;
        }

        // We also have a simpler job if there are 2 predecessors
        जर (bb.isTwoWayJoin()) {
            DfgVertexVar* predicatep = nullptr;
            स्थिरांक CfgBlock* thenp = nullptr;
            स्थिरांक CfgBlock* elsep = nullptr;
            std::tie(predicatep, thenp, elsep)
                = howToJoin(bb.firstPredecessorEdgep(), bb.lastPredecessorEdgep());
            // Copy from else
            joined = m_bbToOSymTab[elsep];
            // Join with then
            परत join_चिन्ह_Tables(joined, predicatep, m_bbToOSymTab[*thenp]);
        }

        // General hard way

        // Gather predecessors
        संरचना Predecessor final {
            स्थिरांक CfgBlock* m_bbp;  // Predeccessor block
            DfgVertexVar* m_predicatep;  // Predicate predecessor reached this block with
            स्थिरांक SymTab* m_oSymTabp;  // Output symbol table or predecessor
            Predecessor() = delete;
            Predecessor(स्थिरांक CfgBlock* bbp, DfgVertexVar* predicatep, स्थिरांक SymTab* oSymTabp)
                : m_bbp{bbp}
                , m_predicatep{predicatep}
                , m_oSymTabp{oSymTabp} {}
        };

        स्थिरांक std::vector<Predecessor> predecessors = [&]() {
            std::vector<Predecessor> res;
            साठी (स्थिरांक V3GraphEdge& edge : bb.inEdges()) {
                स्थिरांक CfgEdge& cfgEdge = static_cast<स्थिरांक CfgEdge&>(edge);
                स्थिरांक CfgBlock* स्थिरांक predecessorp = cfgEdge.srcp();
                DfgVertexVar* स्थिरांक predicatep = m_edgeToPredicatep[cfgEdge];
                स्थिरांक SymTab* स्थिरांक oSymTabp = &m_bbToOSymTab[predecessorp];
                res.emplace_back(predecessorp, predicatep, oSymTabp);
            }
            // Sort predecessors reverse topologically. This way earlier blocks
            // will come after later blocks, and the entry block is last if present.
            std::sort(res.begin(), res.end(), [](स्थिरांक Predecessor& a, स्थिरांक Predecessor& b) {  //
                परत *a.m_bbp > *b.m_bbp;
            });
            परत res;
        }();

        // Start by copying the bindings from the frist predecessor
        joined = *predecessors[0].m_oSymTabp;
        // Join over all other predecessors
        साठी (माप_प्रकार i = 1; i < predecessors.size(); ++i) {
            DfgVertexVar* स्थिरांक predicatep = predecessors[i].m_predicatep;
            स्थिरांक SymTab& oSymTab = *predecessors[i].m_oSymTabp;
            जर (!join_चिन्ह_Tables(joined, predicatep, oSymTab)) परत असत्य;
        }

        परत सत्य;
    }

    std::vector<Driver> computePropagatedDrivers(स्थिरांक std::vector<Driver>& newDrivers,
                                                 DfgVertexVar* oldp) {
        // Gather drivers of 'oldp' - they are in incresing range order with no overlaps
        स्थिरांक std::vector<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.empty(), oldp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::vector<Driver> propagatedDrivers;

        // Add bits between 'msb' and 'lsb' from 'oldp' to 'pDrivers'
        स्थिरांक स्वतः addOldDriver = [&](File_ओळ* स्थिरांक flp, uint32_t msb, uint32_t lsb) {
            UASSERT_OBJ(propagatedDrivers.empty() || lsb > propagatedDrivers.back().m_hi, flp,
                        "Drivers should be in ascending order");
            DfgSel* स्थिरांक selp = make<DfgSel>(flp, DfgDataType::packed(msb - lsb + 1));
            selp->lsb(lsb);
            selp->fromp(oldp);
            propagatedDrivers.emplace_back(selp, lsb, flp);
        };

        // Incorporate old drivers
        साठी (स्थिरांक Driver& oDriver : oldDrivers) {
            File_ओळ* स्थिरांक flp = oDriver.m_flp;
            // Range to consider inserting, we will adjust oldLo as we process drivers
            uint32_t oldLo = oDriver.m_lo;
            स्थिरांक uint32_t oldHi = oDriver.m_hi;

            // Loop for now, can move to bisection search if this is a problem, shouldn't be ...
            साठी (स्थिरांक Driver& nDriver : newDrivers) {
                UASSERT_OBJ(oldHi >= oldLo, flp, "Should have stopped iteration");
                // If new driver is entirely below old driver, move on to
                जर (nDriver.m_hi < oldLo) सुरूठेवा;
                // If new driver is entirely above old driver, we can stop
                जर (oldHi < nDriver.m_lo) थांबा;

                // There is an overlap between 'oDriver' and 'nDriver'.
                // Insert the low bits and adjust the insertion range.
                // The rest will take care of itself on subsequent iterations.
                जर (oldLo < nDriver.m_lo) addOldDriver(flp, nDriver.m_lo - 1, oldLo);
                oldLo = nDriver.m_hi + 1;

                // Stop if no more bits remaining in the old driver
                जर (oldLo > oldHi) थांबा;
            }

            // Insert remaining bits if any
            जर (oldHi >= oldLo) addOldDriver(flp, oldHi, oldLo);
        }

        परत propagatedDrivers;
    }

    // Propagate whole elements with a linear walk over the sorted driver lists.
    // Reads use the previous value temporary, preserving assignment-version bindings.
    // Partial elements and non-unit drivers retain the nonsynthesized-process fallback.
    तार्किक computePropagatedArrayDrivers(स्थिरांक std::vector<Driver>& newDrivers,
                                       DfgVertexVar* स्थिरांक oldp,
                                       std::vector<Driver>& propagatedDrivers) {
        // Bound quadratic vertex growth: array drivers cannot be coalesced.
        स्थिर constexpr uint32_t MAX_ARRAY_ELEMENTS = 32;
        जर (oldp->size() > MAX_ARRAY_ELEMENTS) परत असत्य;
        साठी (स्थिरांक Driver& driver : newDrivers) {
            जर (!driverCoversWholeElement(driver)) परत असत्य;
        }
        स्थिरांक std::vector<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.empty(), oldp, "Should have a proper driver");
        साठी (स्थिरांक Driver& driver : oldDrivers) {
            जर (!driverCoversWholeElement(driver)) परत असत्य;
        }

        propagatedDrivers.reserve(oldDrivers.size());
        स्वतः nIt = newDrivers.begin();
        साठी (स्थिरांक Driver& oDriver : oldDrivers) {
            पर्यंत (nIt != newDrivers.end() && nIt->m_lo < oDriver.m_lo) ++nIt;
            जर (nIt != newDrivers.end() && nIt->m_lo == oDriver.m_lo) सुरूठेवा;

            File_ओळ* स्थिरांक flp = oDriver.m_flp;
            स्थिरांक DfgUnitArray* स्थिरांक oldUnitp = oDriver.m_vtxp->as<DfgUnitArray>();
            DfgArraySel* स्थिरांक selp = make<DfgArraySel>(flp, oldUnitp->srcp()->dtype());
            selp->fromp(oldp);
            selp->bitp(make<DfgConst>(flp, static_cast<माप_प्रकार>(VL_IDATASIZE), oDriver.m_lo));
            DfgUnitArray* स्थिरांक newUnitp = make<DfgUnitArray>(flp, oldUnitp->dtype());
            newUnitp->srcp(selp);
            propagatedDrivers.emplace_back(newUnitp, oDriver.m_lo, flp);
        }
        परत सत्य;
    }

    // Given the drivers of a variable after converting a single statement
    // 'newp', add drivers from 'oldp' that were not reassigned be drivers
    // in newp. This computes the total result of all previous assignments.
    तार्किक incorporate_Previous_मूल्य(AstVarScope* vscp, DfgVertexVar* newp, DfgVertexVar* oldp) {
        UASSERT_OBJ(newp->srcp(), vscp, "Assigned variable has no driver");

        // Easy if there is no old value...
        जर (!oldp) परत सत्य;

        // New driver was not yet coalesced, so should always be a splice
        DfgVertexSplice* स्थिरांक nSplicep = newp->srcp()->as<DfgVertexSplice>();

        // If the old value is the real variable we just computed the new value for,
        // then it is the circular feedback into the synthesized block, add it as default driver.
        जर (oldp->vscp() == vscp) {
            जर (!spliceCoversWhole(nSplicep)) newp->defaultp(oldp);
            परत सत्य;
        }

        UASSERT_OBJ(oldp->srcp(), vscp, "Previously assigned variable has no driver");

        // Gather drivers of 'newp' - they are in incresing range order with no overlaps
        UASSERT_OBJ(!newp->defaultp(), newp, "Converted value should not have default");
        std::vector<Driver> nDrivers = gatherDrivers(newp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!nDrivers.empty(), newp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::vector<Driver> pDrivers;
        जर (newp->isPacked()) {
            pDrivers = computePropagatedDrivers(nDrivers, oldp);
        } अन्यथा जर (!computePropagatedArrayDrivers(nDrivers, oldp, pDrivers)) {
            ++m_ctx.m_synt.nonSynArray;
            परत असत्य;
        }

        जर (!pDrivers.empty()) {
            // Need to merge propagated sources, so reset the splice
            nSplicep->resetDrivers();
            // Merge drivers - they are both sorted and non-overlapping
            std::vector<Driver> drivers;
            drivers.reserve(nDrivers.size() + pDrivers.size());
            std::merge(nDrivers.begin(), nDrivers.end(), pDrivers.begin(), pDrivers.end(),
                       std::back_inserter(drivers));
            // Coalesce adjacent ranges
            जर (newp->isPacked()) coalesceDrivers(drivers);
            // Reinsert drivers in order
            साठी (स्थिरांक Driver& d : drivers) nSplicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        }

        // If the old had a default, add to the new one too, unless redundant
        जर (oldp->defaultp() && !spliceCoversWhole(nSplicep)) newp->defaultp(oldp->defaultp());

        // Done
        परत सत्य;
    }

    // Synthesize the given statements with the given input symbol table.
    // Returns true if successfolly synthesized.
    // Populates the given output symbol table.
    // Populates the given reference with the condition of the terminator branch, if any.
    तार्किक synthesizeBasicBlock(SymTab& oSymTab, DfgVertex*& condpr,
                              स्थिरांक std::vector<Ast_गाठ_Stmt*>& stmtps, स्थिरांक SymTab& iSymTab) {
        // Use fresh set of vertices in m_converter
        स्थिरांक VNUser2InUse user2InUse;

        // Initialize AstVarScope -> Vertex bindings available in this block
        साठी (स्थिरांक स्वतः& pair : iSymTab) {
            AstVarScope* स्थिरांक varp = pair.first;
            DfgVertexVar* स्थिरांक vtxp = pair.second;
            varp->user2p(vtxp);
            oSymTab[varp] = vtxp;
        }

        // Synthesize each statement one after the other
        std::vector<std::pair<AstVarScope*, DfgVertexVar*>> updates;
        साठी (Ast_गाठ_Stmt* स्थिरांक stmtp : stmtps) {
            // Regular statements
            Ast_गाठ_Assign* स्थिरांक ap = VN_CAST(stmtp, गाठ_Assign);
            जर (ap && (VN_IS(ap, Assign) || VN_IS(ap, AssignW))) {
                // Convert this assignment
                जर (!m_converter.convert(updates, *m_logicp, ap)) {
                    ++m_ctx.m_synt.nonSynConv;
                    परत असत्य;
                }
                // Apply variable updates from this statement
                साठी (स्थिरांक स्वतः& pair : updates) {
                    // The target variable that was assigned to
                    AstVarScope* स्थिरांक vscp = pair.first;
                    // The new, potentially partially assigned value
                    DfgVertexVar* स्थिरांक newp = pair.second;
                    // Normalize drivers within this statement, bail if multidriven
                    DfgVertexSplice* स्थिरांक srcp = newp->srcp()->as<DfgVertexSplice>();
                    std::vector<Driver> drivers = gatherDrivers(srcp);
                    स्थिरांक तार्किक single = drivers.size() == 1;
                    जर (!normalizeDrivers(*newp, drivers)) {
                        ++m_ctx.m_synt.nonSynMultidrive;
                        परत असत्य;
                    }
                    // If there were more than one driver (often not), re-add in case coalesced
                    जर (!single) {
                        srcp->resetDrivers();
                        साठी (स्थिरांक Driver& d : drivers) srcp->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
                    }
                    // The old value, if any
                    DfgVertexVar* स्थिरांक oldp = vscp->user2u().ढाँचा to<DfgVertexVar*>();
                    // Inncorporate old value into the new value
                    जर (!incorporate_Previous_मूल्य(vscp, newp, oldp)) परत असत्य;
                    // Update binding of target variable
                    vscp->user2p(newp);
                    // Update output symbol table of this block
                    oSymTab[vscp] = newp;
                }
                updates.clear();
                सुरूठेवा;
            }

            // Terminator branches
            जर (AstIf* स्थिरांक ifp = VN_CAST(stmtp, If)) {
                UASSERT_OBJ(ifp == stmtps.back(), ifp, "Branch should be last statement");
                // Convert condition, give up if failed
                DfgVertex* condp = m_converter.convert(*m_logicp, ifp->condp());
                जर (!condp) {
                    ++m_ctx.m_synt.nonSynConv;
                    परत असत्य;
                }
                // Single bit condition can be use directly, otherwise: use 'condp != 0'
                जर (condp->width() != 1) {
                    File_ओळ* स्थिरांक flp = condp->fileline();
                    DfgNeq* स्थिरांक neqp = make<DfgNeq>(flp, DfgDataType::packed(1));
                    neqp->lhsp(make<DfgConst>(flp, condp->width(), 0U));
                    neqp->rhsp(condp);
                    condp = neqp;
                }
                condpr = condp;
                सुरूठेवा;
            }

            // Unhandled
            ++m_ctx.m_synt.nonSynStmt;
            परत असत्य;
        }

        परत सत्य;
    }

    // Assign path predicates to the outgoing control flow edges of the given block
    शून्य assignPathPredicates(स्थिरांक CfgBlock& bb) {
        // Nothing to do for the exit block
        जर (bb.isExit()) परत;

        // Get the predicate of this block
        DfgVertex* स्थिरांक predp = [&]() -> DfgVertex* {
            // Entry block has no predecessors, use constant true
            जर (bb.isEnter()) परत make<DfgConst>(m_logicp->fileline(), 1U, 1U);

            // For any other block, 'or' together all the incoming predicates
            स्थिरांक स्वतः& inEdges = bb.inEdges();
            स्वतः it = inEdges.begin();
            DfgVertex* resp = m_edgeToPredicatep[static_cast<स्थिरांक CfgEdge&>(*it)];
            पर्यंत (++it != inEdges.end()) {
                DfgOr* स्थिरांक orp = make<DfgOr>(resp->fileline(), resp->dtype());
                orp->rhsp(resp);
                orp->lhsp(m_edgeToPredicatep[static_cast<स्थिरांक CfgEdge&>(*it)]);
                resp = orp;
            }
            परत resp;
        }();

        स्थिरांक DfgDataType& dtype = predp->dtype();

        स्थिरांक स्वतः mkTmp = [&](File_ओळ* flp, स्थिरांक अक्षर* name, DfgVertex* srcp) {
            स्थिरांक std::string prefix = "_BB" + std::to_string(bb.id()) + "_" + name;
            DfgVertexVar* स्थिरांक tmpp = m_converter.createTmp(*m_logicp, flp, dtype, prefix);
            tmpp->srcp(srcp);
            परत tmpp;
        };

        // Assign it to a variable in case it's used multiple times
        DfgVertexVar* स्थिरांक pInp = mkTmp(predp->fileline(), "PathIn", predp);

        // For uncondional branches, the successor predicate edge is the same
        जर (!bb.isBranch()) {
            m_edgeToPredicatep[bb.takenEdgep()] = mkTmp(pInp->fileline(), "Goto", pInp);
            परत;
        }

        // For branches, we need to factor in the branch condition
        DfgVertex* स्थिरांक condp = m_bbToCondp[bb];
        File_ओळ* स्थिरांक flp = condp->fileline();

        // Predicate for taken branch: 'predp & condp'
        {
            DfgAnd* स्थिरांक takenPredp = make<DfgAnd>(flp, dtype);
            takenPredp->lhsp(pInp);
            takenPredp->rhsp(condp);
            m_edgeToPredicatep[bb.takenEdgep()] = mkTmp(flp, "Taken", takenPredp);
        }

        // Predicate for untaken branch: 'predp & ~condp'
        {
            DfgAnd* स्थिरांक untknPredp = make<DfgAnd>(flp, dtype);
            untknPredp->lhsp(pInp);
            DfgNot* स्थिरांक notp = make<DfgNot>(flp, dtype);
            notp->srcp(condp);
            untknPredp->rhsp(notp);
            m_edgeToPredicatep[bb.untknEdgep()] = mkTmp(flp, "Untkn", untknPredp);
        }
    }

    // Returns true if all external updates to volatile variables are observed correctly
    तार्किक checkExtWrites() {
        साठी (स्थिरांक DfgVertex* स्थिरांक vtxp : m_logicp->synth()) {
            स्थिरांक DfgVertexVar* स्थिरांक varp = vtxp->cast<DfgVertexVar>();
            जर (!varp) सुरूठेवा;
            // If the variable we synthesized this vertex for is volatile, and
            // the value of the synthesized temporary is observed, we might be
            // missing an external update, so we mut give up.
            जर (!varp->hasSinks()) सुरूठेवा;
            जर (!DfgVertexVar::isVolatile(varp->tmpForp())) सुरूठेवा;
            ++m_ctx.m_synt.nonSynExtWrite;
            परत असत्य;
        }
        परत सत्य;
    }

    // Add the synthesized values as drivers to the output variables of the current DfgLogic
    तार्किक addSynthesizedOutput(SymTab& oSymTab) {
        // It's possible we think a variable is written by the DfgLogic when
        // it actauly isn't, e.g.: '{a[0], b[0]}[1] = ...' does not write 'b'.
        // These LHS forms can happen after some earlier tranforms. We
        // should just run V3Const on them earlier, but we will do belt and
        // braces and check here too. We can't touch any output variables if so.
        स्थिरांक तार्किक missing = m_logicp->foreachSink([&](स्थिरांक DfgVertex& sink) {
            स्थिरांक DfgUnresolved* स्थिरांक unresolvedp = sink.as<DfgUnresolved>();
            AstVarScope* स्थिरांक vscp = unresolvedp->singleSink()->as<DfgVertexVar>()->vscp();
            परत !oSymTab.मोजणी(vscp);
        });
        जर (missing) {
            ++m_ctx.m_synt.nonSynFalseWrite;
            परत असत्य;
        }

        // Add sinks to read the computed values for the target variables
        परत !m_logicp->foreachSink([&](DfgVertex& sink) {
            DfgUnresolved* स्थिरांक unresolvedp = sink.as<DfgUnresolved>();
            स्थिरांक DfgVertexVar* स्थिरांक varp = unresolvedp->singleSink()->as<DfgVertexVar>();
            DfgVertexVar* स्थिरांक resp = oSymTab.at(varp->vscp());
            UASSERT_OBJ(resp->srcp(), resp, "Undriven result");

            // If the output is not used further in the synthesized logic itself,
            // then resp will be deleted before we return, so we can just use
            // its splice directly without ending up with a multi-use operation.
            जर (!resp->hasSinks()) {
                unresolvedp->addDriver(resp->srcp()->as<DfgVertexSplice>());
                परत असत्य;  // OK, continue.
            }

            // TODO: computePropagatedDrivers cannot handle arrays, should
            // never happen with simple continous assignments
            जर (!resp->isPacked()) {
                ++m_ctx.m_synt.nonSynArray;
                परत सत्य;  // Not OK, give up
            }

            // We need to add a new splice to avoid multi-use of the original splice
            DfgSplicePacked* स्थिरांक splicep
                = नवीन DfgSplicePacked{m_dfg, resp->fileline(), resp->dtype()};
            // Drivers are the same
            स्थिरांक std::vector<Driver> drivers = computePropagatedDrivers({}, resp);
            साठी (स्थिरांक Driver& d : drivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
            unresolvedp->addDriver(splicep);
            परत असत्य;  // OK, continue
        });
    }

    // Synthesize the given AstAssignW. Returns true on success.
    तार्किक synthesizeAssignW(AstAssignW* nodep) {
        ++m_ctx.m_synt.आदान_Assign;

        // The input and output symbol tables
        SymTab iSymTab;
        SymTab oSymTab;

        // Initialzie input symbol table
        initialize_Entry_चिन्ह_Table(iSymTab);

        // Synthesize as if it was in a single CfgBlock CFG
        DfgVertex* condp = nullptr;
        स्थिरांक तार्किक success = synthesizeBasicBlock(oSymTab, condp, {nodep}, iSymTab);
        UASSERT_OBJ(!condp, nodep, "Conditional AstAssignW ???");
        जर (!success) परत असत्य;

        // Check exernal writes are observed correctly
        जर (!checkExtWrites()) परत असत्य;

        // Add resolved output variable drivers
        परत addSynthesizedOutput(oSymTab);
    }

    // Synthesize the given AstAlways. Returns true on success.
    तार्किक synthesizeCfg(CfgGraph& cfg) {
        ++m_ctx.m_synt.आदान_Always;

        // If there is a backward edge (loop), we can't synthesize it
        जर (cfg.containsLoop()) {
            ++m_ctx.m_synt.nonSynLoop;
            ++m_ctx.m_synt.cfgCyclic;
            परत असत्य;
        }

        // If it's a trivial CFG we can save on some work
        जर (cfg.nBlocks() == 1) {
            ++m_ctx.m_synt.cfgTrivial;
        } अन्यथा {
            // Insert two-way join blocks to aid multiplexer ordering
            जर (cfg.घाला_Two_Way_Joins()) {
                ++m_ctx.m_synt.cfgSp;
            } अन्यथा {
                ++m_ctx.m_synt.cfgDag;
            }
            // Initialize maps needed for non-trivial CFGs
            m_dom_वृक्ष = Cfg_Dominator_वृक्ष{cfg};
            m_edgeToPredicatep = cfg.makeEdgeMap<DfgVertexVar*>();
        }

        // Initialize CfgMaps
        m_bbToISymTab = cfg.makeBlockMap<SymTab>();
        m_bbToOSymTab = cfg.makeBlockMap<SymTab>();
        m_bbToCondp = cfg.makeBlockMap<DfgVertexVar*>();

        // Synthesize all blocks
        साठी (स्थिरांक V3GraphVertex& vtx : cfg.vertices()) {
            स्थिरांक CfgBlock& bb = static_cast<स्थिरांक CfgBlock&>(vtx);
            // Prepare the input symbol table of this block (enter, or join predecessor blocks)
            जर (!create_आदान_चिन्ह_Table(bb)) परत असत्य;
            // Synthesize this block
            DfgVertex* condp = nullptr;
            जर (!synthesizeBasicBlock(m_bbToOSymTab[bb], condp, bb.stmtps(), m_bbToISymTab[bb])) {
                परत असत्य;
            }
            // Create a temporary for the branch condition as it might be used multiple times
            जर (condp) {
                File_ओळ* स्थिरांक flp = condp->fileline();
                स्थिरांक DfgDataType& dtype = condp->dtype();
                स्थिरांक std::string prefix = "_BB" + std::to_string(bb.id()) + "_Cond";
                DfgVertexVar* स्थिरांक vp = m_converter.createTmp(*m_logicp, flp, dtype, prefix);
                vp->srcp(condp);
                m_bbToCondp[bb] = vp;
            }
            // Set the path predicates on the successor edges
            assignPathPredicates(bb);
        }

        // Check exernal writes are observed correctly
        जर (!checkExtWrites()) परत असत्य;

        // Add resolved output variable drivers
        परत addSynthesizedOutput(m_bbToOSymTab[cfg.निकास()]);
    }

    // Synthesize a DfgLogic into regular vertices. Returns ture on success.
    तार्किक synthesize(DfgLogic& vtx) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;

        जर (AstAssignW* स्थिरांक ap = VN_CAST(vtx.nodep()->stmtsp(), AssignW)) {
            जर (ap->nextp()) परत असत्य;
            जर (!synthesizeAssignW(ap)) परत असत्य;
            ++m_ctx.m_synt.synthAssign;
            परत सत्य;
        }

        जर (!synthesizeCfg(vtx.cfg())) परत असत्य;
        ++m_ctx.m_synt.synthAlways;
        परत सत्य;
    }

    // Revert all DfgLogic in m_toRevert, or DfgLogic driving the DfgUnresolved
    // vertices in m_toRevert, and transitively the same for any DfgUnresolved
    // driven by the reverted DfgLogic. Delete all DfgUnresolved involed.
    शून्य revert(VDouble0& statCountr) {
        m_toRevert.foreach([&](DfgVertex& vtx) {
            // Process DfgLogic
            जर (DfgLogic* स्थिरांक vtxp = vtx.cast<DfgLogic>()) {
                UASSERT_OBJ(vtxp->selectedForSynthesis(), vtxp, "Shouldn't reach here unselected");
                // Revert this logic
                UASSERT_OBJ(!vtxp->reverted(), vtxp, "Should be reverting now");
                ++statCountr;
                साठी (DfgVertex* स्थिरांक p : vtxp->synth()) VL_DO_DANGLING(p->unlinkDelete(m_dfg), p);
                vtxp->synth().clear();
                vtxp->setReverted();
                // Add all DfgUnresolved it drives to the work list
                vtxp->foreachSink([&](DfgVertex& snk) {
                    m_toRevert.ढकला_front(*snk.as<DfgUnresolved>());
                    परत असत्य;
                });
                परत;
            }

            // Process DfgUnresolved
            जर (DfgUnresolved* स्थिरांक vtxp = vtx.cast<DfgUnresolved>()) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->setHasModWrRefs();
                // Add all driving DfgLogic to the work list
                vtxp->foreachSource([&](DfgVertex& src) {
                    DfgLogic* स्थिरांक lp = src.cast<DfgLogic>();
                    जर (lp && !lp->reverted()) m_toRevert.ढकला_front(*lp);
                    परत असत्य;
                });
                // Delete the DfgUnresolved driver
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
                परत;
            }

            // There should be nothing else on the worklist
            vtx.v3fatalSrc("Unexpected vertex type");
        });
    }

    // Synthesize all of the given vertices
    शून्य main() {
        //-------------------------------------------------------------------
        UINFO(5, "Step 0: Remove all DfgLogic not selected for synthesis");
        साठी (DfgVertex* स्थिरांक vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgUnresolved
            जर (!vtxp->is<DfgUnresolved>()) सुरूठेवा;
            तार्किक anySelected = असत्य;
            तार्किक anyUnselected = असत्य;
            vtxp->foreachSource([&](DfgVertex& src) {
                स्थिरांक DfgLogic& logic = *src.as<DfgLogic>();
                जर (logic.selectedForSynthesis()) {
                    anySelected = सत्य;
                } अन्यथा {
                    anyUnselected = सत्य;
                }
                परत असत्य;
            });
            // There should be a driver
            UASSERT_OBJ(anySelected || anyUnselected, vtxp, "'DfgUnresolved' with no driver");
            // All drivers should be selected or all should be unselected
            UASSERT_OBJ(!(anySelected && anyUnselected), vtxp, "Invalid 'DfgLogic' selection");
            // If all drivers are unselected, delete this DfgUnresolved here
            जर (anyUnselected) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->setHasModWrRefs();
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
        साठी (DfgVertex* स्थिरांक vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgLogic
            DfgLogic* स्थिरांक logicp = vtxp->cast<DfgLogic>();
            जर (!logicp) सुरूठेवा;
            जर (logicp->selectedForSynthesis()) सुरूठेवा;
            // There should be no sinks left for unselected DfgLogic, delete them here
            UASSERT_OBJ(!logicp->hasSinks(), vtxp, "Unselected 'DfgLogic' with sinks remaining");
            // Input variables will be read in Ast code, add Ast reference vertices
            // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
            स्थिरांक VNUser4InUse m_user4InUse;
            logicp->foreachSource([](DfgVertex& src) {
                src.as<DfgVertexVar>()->vscp()->user4p(&src);
                परत असत्य;
            });
            V3DfgPasses::addAstRefs(m_dfg, logicp->nodep(), [](Ast_गाठ* varp) {  //
                परत varp->user4u().to<DfgVertexVar*>();
            });
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        debugDump("synth-selected");

        //-------------------------------------------------------------------
        UINFO(5, "Step 1: Attempting to synthesize each of the selected DfgLogic");
        {
            // AstVarScope::user4() -> int: unique ID for 'AstVarScopeComparator'
            स्थिरांक VNUser4InUse user4InUse;
            साठी (DfgVertex& vtx : m_dfg.opVertices()) {
                DfgLogic* स्थिरांक logicp = vtx.cast<DfgLogic>();
                जर (!logicp) सुरूठेवा;

                // We should only have DfgLogic remaining that was selected for synthesis
                UASSERT_OBJ(logicp->selectedForSynthesis(), logicp, "Unselected DfgLogic remains");

                // Debug aid
                स्थिरांक स्वतः debugCallback = [&]() -> शून्य {
                    // This is the breaking logic
                    m_debugLogicp = logicp;
                    // Dump it
                    UINFOTREE(0, logicp->nodep(), "Problematic DfgLogic: " << logicp, "  ");
                    V3EmitV::debug_Verilog_For_वृक्ष(logicp->nodep(), std::cout);
                    debugDump("synth-lastok");
                };
                जर (VL_UNLIKELY(s_dfgSynthDebugBisect.stop(debugCallback))) थांबा;

                // Synthesize it, if failed, enqueue for reversion
                जर (!synthesize(*logicp)) {
                    logicp->setNonSynthesizable();
                    m_toRevert.ढकला_front(*logicp);
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
        std::unordered_map<स्थिरांक DfgVertexVar*, DfgVertexSplice*> resolvedDrivers;
        // Compute resolved drivers of all variablees
        साठी (DfgVertexVar& var : m_dfg.varVertices()) {
            जर (!var.srcp()) सुरूठेवा;
            DfgUnresolved* स्थिरांक unresolvedp = var.srcp()->cast<DfgUnresolved>();
            जर (!unresolvedp) थांबा;  // Stop when reached the synthesized temporaries

            // Resolve the synthesized drivers
            DfgVertexSplice* स्थिरांक resolvedp = [&]() -> DfgVertexSplice* {
                // All synthesized drivers were normalized already,
                // so if there is only one, it can be used directly
                जर (unresolvedp->nInputs() == 1) {
                    परत unresolvedp->inputp(0)->as<DfgVertexSplice>();
                }
                // Otherwise gather the synthesized drivers
                std::vector<Driver> drivers = gatherDriversUnresolved(unresolvedp);
                // Normalize them, make resolved driver if all good
                जर (normalizeDrivers(var, drivers)) परत makeSplice(var, drivers);
                // If mutlidriven, record and ignore
                multidrivenps.emplace_back(&var);
                m_toRevert.ढकला_front(*unresolvedp);
                परत nullptr;
            }();
            // Bail if multidriven
            जर (!resolvedp) सुरूठेवा;
            // Add to map for next loop
            स्थिरांक तार्किक newEntry = resolvedDrivers.emplace(&var, resolvedp).second;
            UASSERT_OBJ(newEntry, &var, "Duplicate driver");
        }
        // Revert and remove drivers of multi-driven variables
        revert(m_ctx.m_synt.revertMultidrive);
        // Replace all DfgUnresolved with the resolved drivers
        साठी (स्थिरांक DfgVertexVar& var : m_dfg.varVertices()) {
            जर (!var.srcp()) सुरूठेवा;
            DfgUnresolved* स्थिरांक srcp = var.srcp()->cast<DfgUnresolved>();
            जर (!srcp) थांबा;  // Stop when reached the synthesized temporaries

            // Replace it
            srcp->replaceWith(resolvedDrivers.at(&var));
            VL_DO_DANGLING(srcp->unlinkDelete(m_dfg), srcp);
        }
        debugDump("synth-resolved");

        //-------------------------------------------------------------------
        UINFO(5, "Step 4: Remove all remaining DfgLogic and DfgUnresolved");
        साठी (DfgVertex* स्थिरांक vtxp : m_dfg.opVertices().unlinkable()) {
            // Previous step should have removed all DfgUnresolved
            UASSERT_OBJ(!vtxp->is<DfgUnresolved>(), vtxp, "DfgUnresolved remains");

            // Process only DfgLogic
            DfgLogic* स्थिरांक logicp = vtxp->cast<DfgLogic>();
            जर (!logicp) सुरूठेवा;

            // Earlier pass should have removed all sinks
            UASSERT_OBJ(!logicp->hasSinks(), logicp, "DfgLogic sink remains");

            जर (!logicp->synth().empty()) {
                // If synthesized, delete the corresponding AstNode. It is now in Dfg.
                logicp->nodep()->unlinkFrBack()->delete_वृक्ष();
            } अन्यथा {
                // Not synthesized. Logic stays in Ast. Add Ast reference vertices.
                // Outputs already marked by revertTransivelyAndRemove.
                // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
                स्थिरांक VNUser4InUse m_user4InUse;
                logicp->foreachSource([](DfgVertex& src) {
                    src.as<DfgVertexVar>()->vscp()->user4p(&src);
                    परत असत्य;
                });
                V3DfgPasses::addAstRefs(m_dfg, logicp->nodep(), [](Ast_गाठ* varp) {  //
                    परत varp->user4u().to<DfgVertexVar*>();
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
        साठी (DfgVertex* स्थिरांक vtxp : m_dfg.opVertices().unlinkable()) {
            DfgVertexSplice* स्थिरांक splicep = vtxp->cast<DfgVertexSplice>();
            जर (!splicep) सुरूठेवा;

            // Might not have a sink if the driving logic was revered, remove
            जर (!splicep->hasSinks()) {
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
                सुरूठेवा;
            }

            // It should alway have drivers
            UASSERT_OBJ(splicep->nInputs(), splicep, "Splice with no drivers");

            // If redundant, remove it
            जर (DfgVertex* स्थिरांक wholep = splicep->wholep()) {
                जर (DfgVertexVar* स्थिरांक varp = splicep->singleSink()->cast<DfgVertexVar>()) {
                    varp->driver_File_ओळ(splicep->driver_File_ओळ(0));
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

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, V3DfgSynthesisContext& ctx) {
        AstToDfgSynthesize{dfg, ctx};

        // Final step outside, as both AstToDfgSynthesize and removeUnused used DfgUserMap
        UINFO(5, "Step 6: Remove all unused vertices");
        V3DfgPasses::removeUnused(dfg);
        जर (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("synth-rmunused");

        // No operation vertex should have multiple sinks. Cyclic decomoposition
        // depends on this and it can easily be ensured by using temporaries.
        // Also, all sources should be connected at this point
        जर (v3Global.opt.debugCheck()) {
            साठी (DfgVertex& vtx : dfg.opVertices()) {
                UASSERT_OBJ(!vtx.hasMultipleSinks(), &vtx, "Operation has multiple sinks");
                साठी (माप_प्रकार i = 0; i < vtx.nInputs(); ++i) {
                    UASSERT_OBJ(vtx.inputp(i), &vtx, "Unconnected source operand");
                }
            }
            V3DfgPasses::typeCheck(dfg);
        }
    }
};

पूर्णांक AstToDfgSynthesize::AstVarScopeComparator::s_vscpIdCounter = 0;

// Decide which DfgLogic to attempt to synthesize
स्थिर शून्य dfgSelectLogicForSynthesis(DfgGraph& dfg) {
    // If we are told to synthesize everything, we will do so ...
    जर (v3Global.opt.fDfgSynthesizeAll()) {
        साठी (DfgVertex& vtx : dfg.opVertices()) {
            जर (DfgLogic* स्थिरांक logicp = vtx.cast<DfgLogic>()) logicp->setSelectedForSynthesis();
        }
        परत;
    }

    // Otherwise figure out which vertices are likely worth synthesizing.

    std::vector<DfgVertexVar*> circularVarps;
    // Synthesize all drivers of circular variables
    {
        DfgUserMap<uint64_t> scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, scc);
        साठी (DfgVertexVar& var : dfg.varVertices()) {
            जर (!scc.at(var)) सुरूठेवा;
            // This is a circular variable
            circularVarps.emplace_back(&var);
        }
    }

    // To cover all drivers, use a work list
    DfgWorklist worklist{dfg};

    // Synthesize all drivers of circular variables
    साठी (स्थिरांक DfgVertexVar* स्थिरांक varp : circularVarps) {
        varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
            worklist.ढकला_front(*driver.as<DfgLogic>());
            परत असत्य;
        });
    }

    // Choose some simple special cases to always synthesize
    साठी (DfgVertex& vtx : dfg.opVertices()) {
        DfgLogic* स्थिरांक logicp = vtx.cast<DfgLogic>();
        जर (!logicp || worklist.contains(*logicp)) सुरूठेवा;
        // If drives an unused variable, synthesize it so the partial logic can be removed
        जर (logicp->drivesUnusedVars()) {
            worklist.ढकला_front(*logicp);
            सुरूठेवा;
        }
        // Blocks corresponding to continuous assignments
        जर (logicp->nodep()->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            worklist.ढकला_front(*logicp);
            सुरूठेवा;
        }
        स्थिरांक CfgGraph& cfg = logicp->cfg();
        // Straight line code with no branches
        जर (cfg.nBlocks() == 1) {
            worklist.ढकला_front(*logicp);
            सुरूठेवा;
        }
        // Blocks driving exactly 1 variable
        जर (!logicp->hasMultipleSinks()) worklist.ढकला_front(*logicp);
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
            जर (!isVisited.घाला(unresolved).second) परत असत्य;
            unresolved->foreachSource([&](DfgVertex& sibling) {
                DfgLogic& siblingLogic = *sibling.as<DfgLogic>();
                जर (!siblingLogic.selectedForSynthesis()) worklist.ढकला_front(siblingLogic);
                परत असत्य;
            });
            परत असत्य;
        });
    });
}

शून्य V3DfgPasses::synthesize(DfgGraph& dfg, V3DfgContext& ctx) {
    // Select which DfgLogic to attempt to synthesize
    dfgSelectLogicForSynthesis(dfg);
    // Synthesize them - also removes un-synthesized DfgLogic, so must run even if nothing selected
    AstToDfgSynthesize::apply(dfg, ctx.m_synthContext);
}
