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
ढाँचा <typename T_Vertex, typename T_गाँठो>
T_Vertex* makeVertex(अचर T_गाँठो* nodep, DfgGraph& dfg, अचर DfgDataType& dtype) {
    फर्काऊ नूतन T_Vertex{dfg, nodep->fileline(), dtype};
}

ढाँचा <>
DfgArraySel* makeVertex<DfgArraySel, AstArraySel>(अचर AstArraySel* nodep, DfgGraph& dfg,
                                                  अचर DfgDataType& dtype) {
    // Some earlier passes create malformed ArraySels, just bail on those...
    // See t_bitsel_wire_array_bad
    यदि (VN_IS(nodep->fromp(), Const)) फर्काऊ nullptr;
    यदि (!VN_IS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) फर्काऊ nullptr;
    फर्काऊ नूतन DfgArraySel{dfg, nodep->fileline(), dtype};
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
        static_assert(!std::is_base_of<DfgVertexVar, Vertex>::मान, "Do not use for variables");
        static_assert(std::is_base_of<DfgVertex, Vertex>::मान, "'Vertex' must be a 'DfgVertex'");
        Vertex* अचर vtxp = नूतन Vertex{m_dfg, std::forward<Args>(args)...};
        m_logicp->synth().emplace_back(vtxp);
        फर्काऊ vtxp;
    }

    // Returns true if the expression cannot (or should not) be represented by DFG
    तार्किक unhandled(Ast_गाँठो_अभिव्यक्ति* nodep) {
        // Short-circuiting if something was already unhandled
        यदि (m_foundUnhandled) {
            // Impure nodes cannot be represented
            यदि (!nodep->isPure()) {
                m_foundUnhandled = सत्य;
                ++m_ctx.m_conv.nonRepImpure;
            }
        }
        फर्काऊ m_foundUnhandled;
    }

    तार्किक isSupported(अचर AstVarRef* nodep) {
        // Cannot represent cross module references
        यदि (nodep->classOrPackagep()) फर्काऊ असत्य;
        // Check target
        फर्काऊ V3Dfg::isSupported(nodep->varScopep());
    }

    // Given an RValue expression, return the equivalent Vertex, or nullptr if not representable.
    DfgVertex* convertRValue(Ast_गाँठो_अभिव्यक्ति* nodep) {
        UASSERT_OBJ(!m_converting, nodep, "'convertingRValue' should not be called recursively");
        VL_RESTORER(m_converting);
        VL_RESTORER(m_foundUnhandled);
        m_converting = सत्य;
        m_foundUnhandled = असत्य;

        // Convert the expression
        iterate(nodep);

        // If falied to convert, return nullptr
        यदि (m_foundUnhandled) फर्काऊ nullptr;

        // Traversal set user2p to the equivalent vertex
        DfgVertex* अचर vtxp = nodep->user2u().to<DfgVertex*>();
        UASSERT_OBJ(vtxp, nodep, "Missing Dfg vertex after conversion");
        फर्काऊ vtxp;
    }

    // Given an LValue expression, return the splice node that writes the
    // destination, together with the index to use for splicing in the value.
    // Returns {nullptr, 0}, if the given LValue expression is not supported.
    std::pair<DfgVertexSplice*, uint32_t> convertLValue(Ast_गाँठो_अभिव्यक्ति* nodep) {
        यदि (अचर AstVarRef* अचर vrefp = VN_CAST(nodep, VarRef)) {
            UASSERT_OBJ(vrefp->access().isWriteOnly(), vrefp, "Non-WriteOnly reference");
            यदि (!isSupported(vrefp)) {
                ++m_ctx.m_conv.nonRepLValue;
                फर्काऊ {nullptr, 0};
            }

            // Get (or create a new) temporary for this variable
            अचर DfgVertexVar* अचर vtxp = [&]() -> DfgVertexVar* {
                // The variable being assigned
                AstVarScope* अचर vscp = vrefp->varScopep();

                // Find existing one, if any
                लागि (अचर स्वतः& pair : *m_updatesp) {
                    यदि (pair.first == vscp) फर्काऊ pair.second;
                }

                // Create new one
                DfgVertexVar* अचर newp = createTmp(*m_logicp, vscp, "SynthAssign");
                m_updatesp->emplace_back(vscp, newp);

                // Create the Splice driver for the new temporary
                यदि (newp->is<DfgVarPacked>()) {
                    newp->srcp(make<DfgSplicePacked>(newp->fileline(), newp->dtype()));
                } अन्यथा यदि (newp->is<DfgVarArray>()) {
                    newp->srcp(make<DfgSpliceArray>(newp->fileline(), newp->dtype()));
                } अन्यथा {
                    nodep->v3fatalSrc("Unhandled DfgVertexVar sub-type");
                }

                // Use new temporary
                फर्काऊ newp;
            }();

            // Return the Splice driver
            फर्काऊ {vtxp->srcp()->as<DfgVertexSplice>(), 0};
        }

        यदि (अचर AstSel* selp = VN_CAST(nodep, Sel)) {
            // Only handle constant selects
            अचर AstConst* अचर lsbp = VN_CAST(selp->lsbp(), Const);
            यदि (!lsbp) {
                ++m_ctx.m_conv.nonRepLValue;
                फर्काऊ {nullptr, 0};
            }
            uint32_t lsb = lsbp->toUInt();

            // Convert the 'fromp' sub-expression
            अचर स्वतः pair = convertLValue(selp->fromp());
            यदि (!pair.first) फर्काऊ {nullptr, 0};
            DfgSplicePacked* अचर splicep = pair.first->ढाँचा as<DfgSplicePacked>();
            // Adjust index.
            lsb += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            यदि (lsb + static_cast<uint32_t>(selp->widthConst()) > splicep->size()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                फर्काऊ {nullptr, 0};
            }

            // AstSel doesn't change type kind (array vs packed), so we can use
            // the existing splice driver with adjusted lsb
            फर्काऊ {splicep, lsb};
        }

        यदि (अचर AstArraySel* अचर aselp = VN_CAST(nodep, ArraySel)) {
            // Only handle constant selects
            अचर AstConst* अचर indexp = VN_CAST(aselp->bitp(), Const);
            यदि (!indexp) {
                ++m_ctx.m_conv.nonRepLValue;
                फर्काऊ {nullptr, 0};
            }
            uint32_t index = indexp->toUInt();

            // Convert the 'fromp' sub-expression
            अचर स्वतः pair = convertLValue(aselp->fromp());
            यदि (!pair.first) फर्काऊ {nullptr, 0};
            DfgSpliceArray* अचर splicep = pair.first->ढाँचा as<DfgSpliceArray>();
            // Adjust index. Note pair.second is always 0, but we might handle array slices later..
            index += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            यदि (index + 1U > splicep->size()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                फर्काऊ {nullptr, 0};
            }

            // Ensure the Splice driver exists for this element
            यदि (!splicep->driverAt(index)) {
                File_पङ्क्ति* अचर flp = nodep->fileline();
                // This should never fail
                अचर DfgDataType& dtype = *DfgDataType::fromAst(nodep->dtypep());
                यदि (dtype.isPacked()) {
                    DfgSplicePacked* अचर newp = make<DfgSplicePacked>(flp, dtype);
                    अचर DfgDataType& uaDtype = DfgDataType::array(dtype, 1);
                    DfgUnitArray* अचर uap = make<DfgUnitArray>(flp, uaDtype);
                    uap->srcp(newp);
                    splicep->addDriver(uap, index, flp);
                } अन्यथा यदि (dtype.isArray()) {
                    DfgSpliceArray* अचर newp = make<DfgSpliceArray>(flp, dtype);
                    splicep->addDriver(newp, index, flp);
                } अन्यथा {
                    nodep->v3fatalSrc("Unhandled data type kind");
                }
            }

            // Return the splice driver
            DfgVertex* driverp = splicep->driverAt(index);
            यदि (अचर DfgUnitArray* अचर uap = driverp->cast<DfgUnitArray>()) {
                driverp = uap->srcp();
            }
            फर्काऊ {driverp->as<DfgVertexSplice>(), 0};
        }

        ++m_ctx.m_conv.nonRepLValue;
        फर्काऊ {nullptr, 0};
    }

    // Given the LHS of an assignment, and the vertex representing the RHS,
    // connect up the RHS to drive the targets.
    // Returns true on success, false if the LHS is not representable.
    तार्किक convertAssignment(File_पङ्क्ति* flp, Ast_गाँठो_अभिव्यक्ति* lhsp, DfgVertex* vtxp) {
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
        यदि (!VN_IS(lhsp, VarRef)) lhsp = VN_AS(V3Const::constifyExpensiveEdit(lhsp), गाँठो_अभिव्यक्ति);

        // Assigning compound expressions to a concatenated LHS requires a temporary
        // to avoid multiple use of the expression
        यदि (VN_IS(lhsp, Concat) && !vtxp->is<DfgVertexVar>() && !vtxp->is<DfgConst>()) {
            DfgVertexVar* अचर tmpp = createTmp(*m_logicp, flp, vtxp->dtype(), "Unpack");
            tmpp->srcp(vtxp);
            vtxp = tmpp;
        }

        // Convert each concatenation LHS separately, gather all assignments
        // we need to do into 'assignments', return true if all LValues
        // converted successfully.
        std::vector<Assignment> assignments;
        अचर std::function<तार्किक(Ast_गाँठो_अभिव्यक्ति*, uint32_t)> convertAllLValues
            = [&](Ast_गाँठो_अभिव्यक्ति* subp, uint32_t lsb) -> तार्किक {
            // Concatenation on the LHS, convert each part
            यदि (AstConcat* अचर concatp = VN_CAST(subp, Concat)) {
                Ast_गाँठो_अभिव्यक्ति* अचर cRhsp = concatp->rhsp();
                Ast_गाँठो_अभिव्यक्ति* अचर cLhsp = concatp->lhsp();
                // Convert Rigth of concat
                यदि (!convertAllLValues(cRhsp, lsb)) फर्काऊ असत्य;
                // Convert Left of concat
                फर्काऊ convertAllLValues(cLhsp, lsb + cRhsp->width());
            }

            // Non-concatenation, convert the LValue
            अचर स्वतः pair = convertLValue(subp);
            यदि (!pair.first) फर्काऊ असत्य;

            // If whole lhs, just use it
            यदि (subp == lhsp) {
                assignments.emplace_back(pair.first, pair.second, vtxp);
                फर्काऊ सत्य;
            }

            // Otherwise select the relevant bits
            अचर DfgDataType& dtype = *DfgDataType::fromAst(subp->dtypep());
            DfgSel* अचर selp = make<DfgSel>(subp->fileline(), dtype);
            selp->fromp(vtxp);
            selp->lsb(lsb);
            assignments.emplace_back(pair.first, pair.second, selp);
            फर्काऊ सत्य;
        };

        // Convert the given LHS assignment, give up if any LValues failed to convert
        यदि (!convertAllLValues(lhsp, 0)) फर्काऊ असत्य;

        // All successful, connect the drivers
        लागि (अचर Assignment& item : assignments) {
            यदि (DfgSplicePacked* अचर spp = item.m_lhsp->ढाँचा cast<DfgSplicePacked>()) {
                spp->addDriver(item.m_rhsp, item.m_idx, flp);
            } अन्यथा यदि (DfgSpliceArray* अचर sap = item.m_lhsp->ढाँचा cast<DfgSpliceArray>()) {
                // TODO: multi-dimensional arrays will need changes here
                अचर DfgDataType& rDt = item.m_rhsp->dtype();
                यदि (rDt.isPacked()) {
                    // RHS is assigning an element of this array. Need a DfgUnitArray adapter.
                    DfgUnitArray* अचर uap = make<DfgUnitArray>(flp, DfgDataType::array(rDt, 1));
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

        फर्काऊ सत्य;
    }

    // VISITORS

    // Unhandled node
    शून्य visit(Ast_गाँठो* /*nodep*/) override {
        यदि (!m_foundUnhandled && m_converting) ++m_ctx.m_conv.nonRepUnknown;
        m_foundUnhandled = सत्य;
    }

    // Expressions - mostly auto generated, but a few special ones
    शून्य visit(AstVarRef* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        यदि (unhandled(nodep)) फर्काऊ;
        // This visit method is only called on RValues, where only read refs are supported
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Non-ReadOnly reference");
        यदि (!isSupported(nodep)) {
            m_foundUnhandled = सत्य;
            ++m_ctx.m_conv.nonRepVarRef;
            फर्काऊ;
        }

        // Variable should have been bound before starting conversion
        DfgVertex* अचर vtxp = nodep->varScopep()->user2u().ढाँचा to<DfgVertexVar*>();
        UASSERT_OBJ(vtxp, nodep, "Referenced variable has no associated DfgVertexVar");
        nodep->user2p(vtxp);
    }
    शून्य visit(AstConst* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        यदि (unhandled(nodep)) फर्काऊ;

        यदि (nodep->width() != nodep->अङ्क().width()) {
            // Sometimes the width of the AstConst is not the same as the
            // V3Number it holds. Truncate it here. TODO: should this be allowed?
            V3_सङ्ख्या अङ्क{nodep, nodep->width()};
            अङ्क.opSel(nodep->अङ्क(), nodep->width() - 1, 0);
            DfgVertex* अचर vtxp = make<DfgConst>(nodep->fileline(), अङ्क);
            nodep->user2p(vtxp);
        } अन्यथा {
            DfgVertex* अचर vtxp = make<DfgConst>(nodep->fileline(), nodep->अङ्क());
            nodep->user2p(vtxp);
        }
    }
    शून्य visit(AstCReset* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        यदि (unhandled(nodep)) फर्काऊ;

        अचर DfgDataType* अचर dtypep = DfgDataType::fromAst(nodep->dtypep());
        यदि (!dtypep) {
            m_foundUnhandled = सत्य;
            ++m_ctx.m_conv.nonRepDType;
            फर्काऊ;
        }

        UASSERT_OBJ(!nodep->constructing(), nodep,
                    "CReset should be non-constructing at this stage");

        DfgVertex* अचर vtxp = make<DfgCReset>(nodep->fileline(), *dtypep);
        nodep->user2p(vtxp);
    }
    शून्य visit(AstMatchMasked* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        यदि (unhandled(nodep)) फर्काऊ;

        अचर DfgDataType* अचर dtypep = DfgDataType::fromAst(nodep->dtypep());
        यदि (!dtypep) {
            m_foundUnhandled = सत्य;
            ++m_ctx.m_conv.nonRepDType;
            फर्काऊ;
        }

        iterate(nodep->lhsp());
        यदि (m_foundUnhandled) फर्काऊ;
        iterate(nodep->matchp());
        यदि (m_foundUnhandled) फर्काऊ;

        File_पङ्क्ति* अचर flp = nodep->fileline();
        DfgMatchMasked* अचर vtxp = make<DfgMatchMasked>(flp, *dtypep);
        vtxp->lhsp(nodep->lhsp()->user2u().to<DfgVertex*>());
        vtxp->matchp(nodep->matchp()->user2u().to<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    शून्य visit(AstReplicate* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        यदि (unhandled(nodep)) फर्काऊ;

        अचर DfgDataType* अचर dtypep = DfgDataType::fromAst(nodep->dtypep());
        यदि (!dtypep) {
            m_foundUnhandled = सत्य;
            ++m_ctx.m_conv.nonRepDType;
            फर्काऊ;
        }

        iterate(nodep->srcp());
        यदि (m_foundUnhandled) फर्काऊ;

        File_पङ्क्ति* अचर flp = nodep->fileline();
        DfgRep* अचर vtxp = make<DfgRep>(flp, *dtypep);
        vtxp->srcp(nodep->srcp()->user2u().to<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    शून्य visit(AstSel* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        यदि (unhandled(nodep)) फर्काऊ;

        अचर DfgDataType* अचर dtypep = DfgDataType::fromAst(nodep->dtypep());
        यदि (!dtypep) {
            m_foundUnhandled = सत्य;
            ++m_ctx.m_conv.nonRepDType;
            फर्काऊ;
        }

        iterate(nodep->fromp());
        यदि (m_foundUnhandled) फर्काऊ;

        File_पङ्क्ति* अचर flp = nodep->fileline();
        DfgVertex* vtxp = nullptr;
        यदि (अचर AstConst* अचर constp = VN_CAST(nodep->lsbp(), Const)) {
            अचर uint32_t lsb = constp->toUInt();
            अचर uint32_t msb = lsb + nodep->widthConst() - 1;
            DfgVertex* अचर fromp = nodep->fromp()->user2u().to<DfgVertex*>();
            // Unfortunately we can still have out of bounds selects due to how
            // indices are truncated for speed reasons in V3Width/V3Unknown.
            यदि (msb >= fromp->size()) {
                m_foundUnhandled = सत्य;
                ++m_ctx.m_conv.nonRepOOBSel;
                फर्काऊ;
            }
            DfgSel* अचर selp = make<DfgSel>(flp, *dtypep);
            selp->fromp(fromp);
            selp->lsb(lsb);
            vtxp = selp;
        } अन्यथा {
            iterate(nodep->lsbp());
            यदि (m_foundUnhandled) फर्काऊ;
            DfgMux* अचर muxp = make<DfgMux>(flp, *dtypep);
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
    DfgVertexVar* createTmp(DfgLogic& logic, File_पङ्क्ति* flp, अचर DfgDataType& dtype,
                            अचर std::string& prefix) {
        DfgVertexVar* अचर vtxp = m_dfg.makeNewVar(flp, prefix, dtype, logic.scopep());
        logic.synth().emplace_back(vtxp);
        vtxp->vscp()->varp()->isInternal(सत्य);
        vtxp->tmpForp(vtxp->vscp());
        फर्काऊ vtxp;
    }

    // Create a new temporary variable capable of holding 'varp'
    DfgVertexVar* createTmp(DfgLogic& logic, AstVarScope* vscp, अचर std::string& prefix) {
        AstVar* अचर astVarp = vscp->varp();
        File_पङ्क्ति* अचर flp = astVarp->fileline();
        अचर DfgDataType& dtype = *DfgDataType::fromAst(astVarp->dtypep());
        अचर std::string prfx = prefix + "_" + astVarp->name();
        DfgVertexVar* अचर vtxp = createTmp(logic, flp, dtype, prfx);
        vtxp->tmpForp(vscp);
        फर्काऊ vtxp;
    }

    // Convert AstAssign to Dfg, return true if successful.
    // Fills 'updates' with bindings for assigned variables.
    तार्किक convert(std::vector<std::pair<AstVarScope*, DfgVertexVar*>>& updates, DfgLogic& vtx,
                 Ast_गाँठो_Assign* nodep) {
        UASSERT_OBJ(VN_IS(nodep, Assign) || VN_IS(nodep, AssignW), nodep, "Bad NodeAssign");
        UASSERT_OBJ(updates.empty(), nodep, "'updates' should be empty");
        VL_RESTORER(m_updatesp);
        VL_RESTORER(m_logicp);
        m_updatesp = &updates;
        m_logicp = &vtx;
        // Assignment with timing control shouldn't make it this far
        UASSERT_OBJ(!nodep->timingControlp(), nodep, "Shouldn't make it this far");
        // Convert it
        ++m_ctx.m_conv.आगत_Assignments;
        Ast_गाँठो_अभिव्यक्ति* अचर lhsp = nodep->lhsp();
        Ast_गाँठो_अभिव्यक्ति* अचर rhsp = nodep->rhsp();
        // Check data types are compatible.
        अचर DfgDataType* अचर lDtypep = DfgDataType::fromAst(lhsp->dtypep());
        अचर DfgDataType* अचर rDtypep = DfgDataType::fromAst(rhsp->dtypep());
        यदि (!lDtypep || !rDtypep) {
            ++m_ctx.m_conv.nonRepDType;
            फर्काऊ असत्य;
        }
        // For now, only direct array assignment is supported (e.g. a = b, but not a = _ ? b : c)
        यदि (rDtypep->isArray()) {
            यदि (!VN_IS(rhsp, VarRef)
                || !lhsp->dtypep()->skipRefp()->same_रूख(rhsp->dtypep()->skipRefp())) {
                ++m_ctx.m_conv.nonRepDType;
                फर्काऊ असत्य;
            }
        }
        // Widths should match at this point
        UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Mismatched width reached DFG");
        // Convert the RHS expression
        DfgVertex* अचर rVtxp = convertRValue(rhsp);
        यदि (!rVtxp) फर्काऊ असत्य;
        // Connect the RHS vertex to the LHS targets
        अचर तार्किक success = convertAssignment(nodep->fileline(), lhsp, rVtxp);
        यदि (success) ++m_ctx.m_conv.representable;
        फर्काऊ success;
    }

    // Convert RValue expression to Dfg. Returns nullptr if failed.
    DfgVertex* convert(DfgLogic& vtx, Ast_गाँठो_अभिव्यक्ति* nodep) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;
        // Convert it
        ++m_ctx.m_conv.आगत_Expressions;
        DfgVertex* अचर vtxp = convertRValue(nodep);
        यदि (vtxp) ++m_ctx.m_conv.representable;
        फर्काऊ vtxp;
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
        स्थिर पूर्णाङ्क s_vscpIdCounter;  // Counter for lazily allocating the unique AstVarScope IDs
        तार्किक चालक()(AstVarScope* lhs, AstVarScope* rhs) अचर {
            यदि (!lhs->user4()) lhs->user4(++s_vscpIdCounter);
            यदि (!rhs->user4()) rhs->user4(++s_vscpIdCounter);
            फर्काऊ lhs->user4() < rhs->user4();
        }
    };
    using SymTab = std::map<AstVarScope*, DfgVertexVar*, AstVarScopeComparator>;

    // Represents a [potentially partial] driver of a variable
    संरचना Driver final {
        DfgVertex* m_vtxp = nullptr;  // Driving vertex
        uint32_t m_lo = 0;  // Low index of driven range (internal, not Verilog)
        uint32_t m_hi = 0;  // High index of driven range (internal, not Verilog)
        File_पङ्क्ति* m_flp = nullptr;  // Location of driver in source

        Driver() = स्वभाव;
        Driver(DfgVertex* vtxp, uint32_t lo, File_पङ्क्ति* flp)
            : m_vtxp{vtxp}
            , m_lo{lo}
            , m_hi{lo + vtxp->size() - 1U}
            , m_flp{flp} {}
        चालक तार्किक() अचर { फर्काऊ m_vtxp != nullptr; }

        तार्किक चालक<(अचर Driver& other) अचर {
            यदि (m_lo != other.m_lo) फर्काऊ m_lo < other.m_lo;
            यदि (m_hi != other.m_hi) फर्काऊ m_hi < other.m_hi;
            फर्काऊ m_flp->operatorCompare(*other.m_flp) < 0;
        }

        तार्किक चालक<=(अचर Driver& other) अचर { फर्काऊ !(other < *एतत्); }
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
    Cfg_Dominator_रूख m_dom_रूख;  // The dominator tree of the current CFG

    // STATE - Some debug aid
    // We stop after synthesizing s_dfgSynthDebugLimit vertices (if non-zero).
    // This is the problematic logic (last one we synthesize), assuming a
    // bisection search over s_dfgSynthDebugLimit.
    DfgLogic* m_debugLogicp = nullptr;
    // Source (upstream) cone of outputs of m_debugLogicp
    std::unique_ptr<std::unordered_set<अचर DfgVertex*>> m_debugOSrcConep{nullptr};

    // METHODS

    // Dump current graph for debugging ...
    शून्य debugDump(अचर वर्ण* name) {
        // If we have the debugged logic, compute the vertices feeding its outputs
        यदि (VL_UNLIKELY(m_debugLogicp)) {
            std::vector<अचर DfgVertex*> outputs;
            m_debugLogicp->foreachSink([&outputs](अचर DfgVertex& v) {
                outputs.emplace_back(v.singleSink()->as<DfgVertexVar>());
                फर्काऊ असत्य;
            });
            m_debugOSrcConep = m_dfg.sourceCone(outputs);
        }

        यदि (VL_UNLIKELY(dumpDfgLevel() >= 9 || m_debugOSrcConep)) {
            m_dfg.dumpDotFilePrefixed(name);
            यदि (m_debugOSrcConep) {
                // Dump only the subgraph involving the inputs and outputs of the bad vertex
                m_dfg.dumpDotFilePrefixed(name + "-min"s, [&](अचर DfgVertex& v) -> तार्किक {
                    फर्काऊ m_debugOSrcConep->गन्ती(&v);
                });
            }
        }
    }

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    ढाँचा <typename Vertex, typename... Args>
    Vertex* make(Args&&... args) {
        static_assert(!std::is_base_of<DfgVertexVar, Vertex>::मान, "Do not use for variables");
        static_assert(std::is_base_of<DfgVertex, Vertex>::मान, "'Vertex' must be a 'DfgVertex'");
        Vertex* अचर vtxp = नूतन Vertex{m_dfg, std::forward<Args>(args)...};
        यदि (m_logicp) m_logicp->synth().emplace_back(vtxp);
        फर्काऊ vtxp;
    }

    // Gather all drivers of a resolved variable
    स्थिर std::vector<Driver> gatherDrivers(DfgVertexSplice* vtxp) {
        // Collect them all, check if they are sorted
        std::vector<Driver> drivers;
        drivers.reserve(vtxp->nInputs());
        तार्किक sorted = सत्य;
        vtxp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_पङ्क्ति* flp) {
            // Collect the driver
            drivers.emplace_back(&src, lo, flp);
            // Check if drivers are sorted - most often they are
            अचर माप_प्रकार n = drivers.size();
            यदि (n >= 2 && drivers[n - 1] < drivers[n - 2]) sorted = असत्य;
            फर्काऊ असत्य;
        });

        // Sort if unsorted
        यदि (!sorted) std::stable_sort(drivers.begin(), drivers.end());

        // Done
        फर्काऊ drivers;
    }

    // Returns true if the driver supplies a complete array element.
    स्थिर तार्किक driverCoversWholeElement(अचर Driver& driver) {
        अचर DfgUnitArray* अचर unitp = driver.m_vtxp->cast<DfgUnitArray>();
        यदि (!unitp) फर्काऊ असत्य;
        यदि (DfgVertexSplice* अचर splicep = unitp->srcp()->cast<DfgVertexSplice>()) {
            फर्काऊ splicep->wholep();
        }
        फर्काऊ सत्य;
    }

    // Unlike 'wholep', several element drivers can collectively cover an array.
    स्थिर तार्किक spliceCoversWhole(DfgVertexSplice* अचर splicep) {
        यदि (splicep->wholep()) फर्काऊ सत्य;
        यदि (splicep->isPacked()) फर्काऊ असत्य;
        uint32_t next = 0;
        लागि (अचर Driver& driver : gatherDrivers(splicep)) {
            यदि (driver.m_lo != next || !driverCoversWholeElement(driver)) फर्काऊ असत्य;
            next = driver.m_hi + 1;
        }
        फर्काऊ next == splicep->size();
    }

    // Returns true if the driver cone contains any variable introduced by
    // tristate lowering. Used to distinguish intentional tristate contributor
    // overlap from accidental multidrive.
    स्थिर तार्किक containsTriLoweredVar(DfgVertex* rootp) {
        std::vector<अचर DfgVertex*> थाक;
        std::vector<अचर DfgVertex*> visited;
        थाक.emplace_back(rootp);
        जबसम्म (!थाक.empty()) {
            अचर DfgVertex* अचर vtxp = थाक.back();
            थाक.झिक_back();
            यदि (std::find(visited.begin(), visited.end(), vtxp) != visited.end()) जारी;
            visited.emplace_back(vtxp);
            यदि (अचर DfgVertexVar* अचर varp = vtxp->cast<DfgVertexVar>()) {
                यदि (varp->vscp()->varp()->dfgTriLowered()) फर्काऊ सत्य;
            }
            vtxp->foreachSource([&](अचर DfgVertex& src) {
                थाक.emplace_back(&src);
                फर्काऊ असत्य;
            });
        }
        फर्काऊ असत्य;
    }

    // Returns true if the driver is a direct variable forward (no logic).
    स्थिर तार्किक isDirectVarDriver(अचर DfgVertex* vtxp) { फर्काऊ vtxp->is<DfgVertexVar>(); }

    // Gather all synthesized drivers of an unresolved variable
    स्थिर std::vector<Driver> gatherDriversUnresolved(DfgUnresolved* vtxp) {
        std::vector<Driver> drivers;
        drivers.reserve(vtxp->nInputs());

        // For better locations in error reporting, we unpick concatenations
        // which are sometimes introduced by combinint assignments in V3Const.
        अचर std::function<शून्य(DfgVertex*, uint32_t, File_पङ्क्ति*)> gather
            = [&](DfgVertex* vtxp, uint32_t lo, File_पङ्क्ति* flp) -> शून्य {
            यदि (DfgConcat* अचर concatp = vtxp->cast<DfgConcat>()) {
                DfgVertex* अचर rhsp = concatp->rhsp();
                gather(rhsp, lo, rhsp->fileline());
                DfgVertex* अचर lhsp = concatp->lhsp();
                gather(lhsp, lo + rhsp->width(), lhsp->fileline());
                फर्काऊ;
            }
            drivers.emplace_back(vtxp, lo, flp);
        };

        // Gather all synthesized drivers
        vtxp->foreachSource([&](DfgVertex& src) {
            // Can ignore the original DfgLogic
            यदि (src.is<DfgLogic>()) फर्काऊ असत्य;
            // Synthesized drivers must be a splice at this point
            DfgVertexSplice* अचर splicep = src.as<DfgVertexSplice>();
            // Collect the driver
            splicep->foreachDriver([&](DfgVertex& src, uint32_t lo, File_पङ्क्ति* flp) {
                gather(&src, lo, flp);
                फर्काऊ असत्य;
            });
            फर्काऊ असत्य;
        });

        // Sort the drivers
        std::stable_sort(drivers.begin(), drivers.end());

        // Done
        फर्काऊ drivers;
    }

    // Given two drivers, combine the driven sub-ranges into the first one if
    // possible. First bool returned indicates successfully combined and there
    // are no multi-driven bits. Second bool returned indicates we warned
    // already about multi-driven bits.
    std::pair<तार्किक, तार्किक> combineDrivers(DfgVertexVar& var, अचर std::string& sub,  //
                                         Driver& a, अचर Driver& b) {
        // We can only combine array drivers ...
        यदि (a.m_vtxp->isPacked()) फर्काऊ {असत्य, असत्य};
        // ... that both drive a single element ...
        यदि (a.m_lo != b.m_lo) फर्काऊ {असत्य, असत्य};
        अचर DfgUnitArray* अचर aUap = a.m_vtxp->ढाँचा cast<DfgUnitArray>();
        यदि (!aUap) फर्काऊ {असत्य, असत्य};
        अचर DfgUnitArray* अचर bUap = b.m_vtxp->ढाँचा cast<DfgUnitArray>();
        यदि (!bUap) फर्काऊ {असत्य, असत्य};
        // ... and are themeselves partial
        DfgSplicePacked* अचर aSp = aUap->srcp()->ढाँचा cast<DfgSplicePacked>();
        यदि (!aSp) फर्काऊ {असत्य, असत्य};
        DfgSplicePacked* अचर bSp = bUap->srcp()->ढाँचा cast<DfgSplicePacked>();
        यदि (!bSp) फर्काऊ {असत्य, असत्य};
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
        यदि (!normalizeDrivers(var, abDrivers, sub + "[" + std::to_string(a.m_lo) + "]")) {
            फर्काऊ {असत्य, सत्य};
        }

        // Successfully resolved. Needs a new splice and unit.
        File_पङ्क्ति* अचर flp = var.fileline();
        DfgSplicePacked* अचर splicep = make<DfgSplicePacked>(flp, aSp->dtype());
        लागि (अचर Driver& d : abDrivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        DfgUnitArray* अचर uap = make<DfgUnitArray>(flp, aUap->dtype());
        uap->srcp(splicep);
        a.m_vtxp = uap;
        फर्काऊ {सत्य, असत्य};
    }

    // Combine and coalesce the given drivers.
    // Returns true iff no multi-driven bits are present.
    तार्किक normalizeDrivers(DfgVertexVar& var, std::vector<Driver>& drivers,
                          अचर std::string& sub = "") {
        यदि (drivers.empty()) फर्काऊ सत्य;

        // What type of values are we combining
        अचर तार्किक isPacked = drivers[0].m_vtxp->isPacked();

        // Found a multidriven part ?
        तार्किक multiDriven = असत्य;

        // Iterate through the sorted drivers. Index 'i' is the driver we are
        // resolving driver 'j' agains, and if required, we merge 'j' into 'i'.
        माप_प्रकार i = 0;
        लागि (माप_प्रकार j = 1; j < drivers.size();) {
            UASSERT_OBJ(i < j, &var, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            यदि (!jD) {
                ++j;
                जारी;
            }
            // If 'i' was moved, move 'j' in it's place
            यदि (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                जारी;
            }

            // We have 2 valid drivers now
            UASSERT_OBJ(iD <= jD, &var, "Should always be sorted");
            UASSERT_OBJ(jD.m_vtxp->isPacked() == isPacked, &var, "Mixed type drivers");

            // If no overlap, consider next pair
            यदि (iD.m_hi < jD.m_lo) {
                ++i;
                यदि (i == j) ++j;
                जारी;
            }

            // There is an overlap. Attempt to combine them.
            तार्किक combined = असत्य;
            तार्किक warned = असत्य;
            std::tie(combined, warned) = combineDrivers(var, sub, iD, jD);

            // If sucessfully combined, 'j' is no longer needed, it was combined into 'i'
            यदि (combined) {
                jD = Driver{};
                ++j;
                जारी;
            }

            // Found overlap that cannot be resolved
            multiDriven = सत्य;
            // Compare next driver
            ++j;

            // Do not warn again if we warned during 'combineDrivers'
            यदि (warned) जारी;

            // The variable to warn on
            AstVarScope* अचर vscp = var.tmpForp() ? var.tmpForp() : var.vscp();

            // Loop index often abused, so suppress
            यदि (vscp->varp()->isUsedLoopIdx()) जारी;
            // Tristate lowering can intentionally create overlapping contributors.
            // Keep the signal marked multidriven for DFG fallback, but suppress
            // warning only when both overlapping driver cones look tri-lowered.
            यदि (vscp->varp()->dfgAllowMultidriveTri()) {
                अचर तार्किक iTri = containsTriLoweredVar(iD.m_vtxp);
                अचर तार्किक jTri = containsTriLoweredVar(jD.m_vtxp);
                अचर तार्किक triPair = iTri && jTri;
                अचर तार्किक triAndBridge = (iTri && isDirectVarDriver(jD.m_vtxp))
                                          || (jTri && isDirectVarDriver(iD.m_vtxp));
                यदि (triPair || triAndBridge) जारी;
            }

            // Warn the user now
            अचर std::string lo = std::to_string(jD.m_lo);
            अचर std::string hi = std::to_string(std::min(iD.m_hi, jD.m_hi));
            अचर std::string kind = isPacked ? "Bit" : "Element";
            अचर std::string part = hi == lo ? (" [" + lo + "]") : ("s [" + hi + ":" + lo + "]");

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
        यदि (!multiDriven && isPacked) coalesceDrivers(drivers);

        फर्काऊ !multiDriven;
    }

    // Coalesce adjacent drivers into single ones
    शून्य coalesceDrivers(std::vector<Driver>& drivers) {
        UASSERT(!drivers.empty(), "Can't coalesce 0 drivers");
        UASSERT_OBJ(drivers[0].m_vtxp->isPacked(), drivers[0].m_vtxp, "Can only coalesce packed");

        माप_प्रकार i = 0;
        लागि (माप_प्रकार j = 1; j < drivers.size();) {
            UASSERT(i < j, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            यदि (!jD) {
                ++j;
                जारी;
            }
            // If 'i' was moved, move 'j' in it's place
            यदि (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                जारी;
            }

            // We have 2 valid drivers now
            UASSERT(iD <= jD, "Should always be sorted");

            // If not adjacent, move on
            यदि (iD.m_hi + 1 != jD.m_lo) {
                ++i;
                यदि (i == j) ++j;
                जारी;
            }

            // Coalesce Adjacent ranges,
            अचर DfgDataType& dt = DfgDataType::packed(iD.m_vtxp->width() + jD.m_vtxp->width());
            DfgConcat* अचर concatp = make<DfgConcat>(iD.m_flp, dt);
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
    DfgVertexSplice* makeSplice(DfgVertexVar& var, अचर std::vector<Driver>& newDrivers) {
        UASSERT_OBJ(!newDrivers.empty(), &var, "'makeSplice' called with no new drivers");
        // Create new driver
        DfgVertexSplice* splicep = nullptr;
        यदि (var.is<DfgVarPacked>()) {
            splicep = make<DfgSplicePacked>(var.fileline(), var.dtype());
        } अन्यथा यदि (var.is<DfgVarArray>()) {
            splicep = make<DfgSpliceArray>(var.fileline(), var.dtype());
        } अन्यथा {
            var.v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        लागि (अचर Driver& d : newDrivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        फर्काऊ splicep;
    }

    // Initialzie input symbol table of entry CfgBlock
    शून्य initialize_Entry_चिह्न_Table(SymTab& iSymTab) {
        m_logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* अचर vvp = src.as<DfgVertexVar>();
            iSymTab[vvp->vscp()] = vvp;
            फर्काऊ असत्य;
        });
    }

    // Join variable drivers across a control flow confluence (insert muxes ...)
    DfgVertexVar* joinDrivers(AstVarScope* vscp, DfgVertexVar* predicatep,  //
                              DfgVertexVar* thenp, DfgVertexVar* elsep) {
        AstVarScope* अचर thenVscp = thenp->tmpForp() ? thenp->tmpForp() : thenp->vscp();
        AstVarScope* अचर elseVscp = elsep->tmpForp() ? elsep->tmpForp() : elsep->vscp();
        UASSERT_OBJ(thenVscp == elseVscp, vscp, "Attempting to join unrelated variables");

        // If both bindings are the the same (variable not updated through either path),
        // then there is nothing to do, can use the same binding
        यदि (thenp == elsep) फर्काऊ thenp;

        // We can't join the input variable just yet, so bail
        यदि (thenp->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_आगत;
            फर्काऊ nullptr;
        }
        यदि (elsep->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_आगत;
            फर्काऊ nullptr;
        }

        // Can't do arrays yet
        यदि (!thenp->isPacked()) {
            ++m_ctx.m_synt.nonSynArray;
            फर्काऊ nullptr;
        }

        // Gather drivers of 'thenp' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* अचर tDefaultp = thenp->defaultp();
        std::vector<Driver> tDrivers = gatherDrivers(thenp->srcp()->as<DfgVertexSplice>());

        // Gather drivers of 'elsep' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* अचर eDefaultp = elsep->defaultp();
        std::vector<Driver> eDrivers = gatherDrivers(elsep->srcp()->as<DfgVertexSplice>());

        // Default drivers should be the same or not present on either
        UASSERT_OBJ(tDefaultp == eDefaultp, vscp, "Different default drivers");

        // Location to use for the join vertices
        File_पङ्क्ति* अचर flp = predicatep->fileline();

        // Create a fresh temporary for the joined value
        DfgVertexVar* अचर joinp = m_converter.createTmp(*m_logicp, vscp, "SynthJoin");
        DfgVertexSplice* अचर joinSplicep = make<DfgSplicePacked>(flp, joinp->dtype());
        joinp->srcp(joinSplicep);

        // If both paths are fully driven, just create a simple conditional
        यदि (tDrivers.size() == 1  //
            && tDrivers[0].m_lo == 0  //
            && tDrivers[0].m_hi == thenp->width() - 1  //
            && eDrivers.size() == 1  //
            && eDrivers[0].m_lo == 0  //
            && eDrivers[0].m_hi == elsep->width() - 1) {
            UASSERT_OBJ(!tDefaultp, vscp, "Fully driven variable have default driver");

            DfgCond* अचर condp = make<DfgCond>(flp, joinp->dtype());
            condp->condp(predicatep);
            condp->thenp(thenp);
            condp->elsep(elsep);
            joinSplicep->addDriver(condp, 0, tDrivers[0].m_flp);

            // Done
            फर्काऊ joinp;
        }

        // Otherwise we need to merge them part by part

        // If different bits are driven, then some might not have been assigned.. Latch?
        यदि (tDrivers.size() != eDrivers.size()) {
            ++m_ctx.m_synt.nonSynLatch;
            फर्काऊ nullptr;
        }

        लागि (माप_प्रकार i = 0; i < tDrivers.size(); ++i) {
            अचर Driver& tDriver = tDrivers[i];
            अचर Driver& eDriver = eDrivers[i];
            // If different bits are driven, then some might not have been assigned.. Latch?
            यदि (tDriver.m_lo != eDriver.m_lo || tDriver.m_hi != eDriver.m_hi) {
                ++m_ctx.m_synt.nonSynLatch;
                फर्काऊ nullptr;
            }

            अचर DfgDataType& dtype = DfgDataType::packed(tDriver.m_hi - tDriver.m_lo + 1);
            DfgCond* अचर condp = make<DfgCond>(flp, dtype);
            condp->condp(predicatep);

            // We actally need to select the bits from the joined variables, not use the drivers
            DfgSel* अचर thenSelp = make<DfgSel>(flp, tDriver.m_vtxp->dtype());
            thenSelp->lsb(tDriver.m_lo);
            thenSelp->fromp(thenp);
            condp->thenp(thenSelp);

            // Same for the 'else' part
            DfgSel* अचर elseSelp = make<DfgSel>(flp, eDriver.m_vtxp->dtype());
            elseSelp->lsb(eDriver.m_lo);
            elseSelp->fromp(elsep);
            condp->elsep(elseSelp);

            // Add it as a driver to the join
            joinSplicep->addDriver(condp, tDriver.m_lo, tDriver.m_flp);
        }

        // If there was a default driver, add it to te join
        यदि (tDefaultp) joinp->defaultp(tDefaultp);

        // Done
        फर्काऊ joinp;
    }

    // Merge 'thenSymTab' into 'elseSymTab' using the given predicate to join values
    तार्किक join_चिह्न_Tables(SymTab& elseSymTab, DfgVertexVar* predicatep, अचर SymTab& thenSymTab) {
        // Any variable that does not have a binding on both paths will be removed. These might be
        // temporaries, loop vars, etc used only in one branch. Conversion will fail if the
        // variable is actually referenced later.
        std::vector<AstVarScope*> toRemove;

        // Join each symbol
        लागि (std::pair<AstVarScope* अचर, DfgVertexVar*>& pair : elseSymTab) {
            AstVarScope* अचर varp = pair.first;
            // Find same variable on the else path
            अचर स्वतः it = thenSymTab.find(varp);
            // Record for removal if not assigned on both paths
            यदि (it == thenSymTab.end()) {
                toRemove.emplace_back(varp);
                जारी;
            }
            // Join paths with the predicate
            DfgVertexVar* अचर thenp = it->second;
            DfgVertexVar* अचर elsep = pair.second;
            DfgVertexVar* अचर newp = joinDrivers(varp, predicatep, thenp, elsep);
            यदि (!newp) फर्काऊ असत्य;
            pair.second = newp;
        }

        // Remove variables not assigned on both paths
        लागि (AstVarScope* अचर varp : toRemove) elseSymTab.erase(varp);

        // Done
        फर्काऊ सत्य;
    }

    // Given two joining control flow edges, compute how to join their symbols.
    // Returns the predicaete to join over, and the 'then' and 'else' blocks.
    std::tuple<DfgVertexVar*, अचर CfgBlock*, अचर CfgBlock*>  //
    howToJoin(अचर CfgEdge* अचर ap, अचर CfgEdge* अचर bp) {
        // Find the closest common dominator of the two paths
        अचर CfgBlock* अचर domp = m_dom_रूख.closestCommonDominator(ap->srcp(), bp->srcp());
        // These paths join here, so 'domp' must be a branch, otherwise it's not the closest
        UASSERT_OBJ(domp->isBranch(), domp, "closestCommonDominator is not a branch");

        // The branches of the common dominator
        अचर CfgEdge* अचर takenEdgep = domp->takenEdgep();
        अचर CfgEdge* अचर untknEdgep = domp->untknEdgep();

        // We check if the taken branch dominates the path to either blocks,
        // and if the untaken branch dominates the path to the other block.
        // If so, we can use the branch condition as predicate, otherwise
        // we must use the path predicate as there are ways to get from one
        // branch of the dominator to the other. We need to be careful if
        // either branches are directly to the join block. This is fine,
        // it's as if there was an empty block on that critical edge which
        // is dominated by that path.

        यदि (takenEdgep == ap || m_dom_रूख.dominates(takenEdgep->dstp(), ap->srcp())) {
            यदि (untknEdgep == bp || m_dom_रूख.dominates(untknEdgep->dstp(), bp->srcp())) {
                // Taken path dominates 'ap' and untaken dominates 'bp', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                फर्काऊ std::make_tuple(m_bbToCondp[domp], ap->srcp(), bp->srcp());
            }
        } अन्यथा यदि (takenEdgep == bp || m_dom_रूख.dominates(takenEdgep->dstp(), bp->srcp())) {
            यदि (untknEdgep == ap || m_dom_रूख.dominates(untknEdgep->dstp(), ap->srcp())) {
                // Taken path dominates 'bp' and untaken dominates 'ap', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                फर्काऊ std::make_tuple(m_bbToCondp[domp], bp->srcp(), ap->srcp());
            }
        }

        // The branches don't dominate the joined blocks, must use the path predicate
        ++m_ctx.m_synt.joinUsingPathPredicate;

        // TODO: We could do better here: use the path predicate of the closest
        // cominating blocks, pick the one from the lower rank, etc, but this
        // generic case is very rare, most synthesizable logic has
        // series-parallel CFGs which are covered by the earlier cases.
        फर्काऊ std::make_tuple(m_edgeToPredicatep[ap], ap->srcp(), bp->srcp());
    }

    // Combine the output symbol tables of the predecessors of the given
    // block to compute the input symtol table for the given block.
    तार्किक create_आगत_चिह्न_Table(अचर CfgBlock& bb) {
        // The input symbol table of the given block, we are computing it now
        SymTab& joined = m_bbToISymTab[bb];

        // Input symbol table of entry block is special
        यदि (bb.isEnter()) {
            initialize_Entry_चिह्न_Table(joined);
            फर्काऊ सत्य;
        }

        // Current input symbol table should be empty, we will fill it in here
        UASSERT(joined.empty(), "Unprocessed input symbol table should be empty");

        // Fast path if there is only one predecessor - TODO: use less copying
        यदि (!bb.isJoin()) {
            joined = m_bbToOSymTab[bb.firstPredecessorp()];
            फर्काऊ सत्य;
        }

        // We also have a simpler job if there are 2 predecessors
        यदि (bb.isTwoWayJoin()) {
            DfgVertexVar* predicatep = nullptr;
            अचर CfgBlock* thenp = nullptr;
            अचर CfgBlock* elsep = nullptr;
            std::tie(predicatep, thenp, elsep)
                = howToJoin(bb.firstPredecessorEdgep(), bb.lastPredecessorEdgep());
            // Copy from else
            joined = m_bbToOSymTab[elsep];
            // Join with then
            फर्काऊ join_चिह्न_Tables(joined, predicatep, m_bbToOSymTab[*thenp]);
        }

        // General hard way

        // Gather predecessors
        संरचना Predecessor final {
            अचर CfgBlock* m_bbp;  // Predeccessor block
            DfgVertexVar* m_predicatep;  // Predicate predecessor reached this block with
            अचर SymTab* m_oSymTabp;  // Output symbol table or predecessor
            Predecessor() = delete;
            Predecessor(अचर CfgBlock* bbp, DfgVertexVar* predicatep, अचर SymTab* oSymTabp)
                : m_bbp{bbp}
                , m_predicatep{predicatep}
                , m_oSymTabp{oSymTabp} {}
        };

        अचर std::vector<Predecessor> predecessors = [&]() {
            std::vector<Predecessor> res;
            लागि (अचर V3GraphEdge& edge : bb.inEdges()) {
                अचर CfgEdge& cfgEdge = static_cast<अचर CfgEdge&>(edge);
                अचर CfgBlock* अचर predecessorp = cfgEdge.srcp();
                DfgVertexVar* अचर predicatep = m_edgeToPredicatep[cfgEdge];
                अचर SymTab* अचर oSymTabp = &m_bbToOSymTab[predecessorp];
                res.emplace_back(predecessorp, predicatep, oSymTabp);
            }
            // Sort predecessors reverse topologically. This way earlier blocks
            // will come after later blocks, and the entry block is last if present.
            std::sort(res.begin(), res.end(), [](अचर Predecessor& a, अचर Predecessor& b) {  //
                फर्काऊ *a.m_bbp > *b.m_bbp;
            });
            फर्काऊ res;
        }();

        // Start by copying the bindings from the frist predecessor
        joined = *predecessors[0].m_oSymTabp;
        // Join over all other predecessors
        लागि (माप_प्रकार i = 1; i < predecessors.size(); ++i) {
            DfgVertexVar* अचर predicatep = predecessors[i].m_predicatep;
            अचर SymTab& oSymTab = *predecessors[i].m_oSymTabp;
            यदि (!join_चिह्न_Tables(joined, predicatep, oSymTab)) फर्काऊ असत्य;
        }

        फर्काऊ सत्य;
    }

    std::vector<Driver> computePropagatedDrivers(अचर std::vector<Driver>& newDrivers,
                                                 DfgVertexVar* oldp) {
        // Gather drivers of 'oldp' - they are in incresing range order with no overlaps
        अचर std::vector<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.empty(), oldp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::vector<Driver> propagatedDrivers;

        // Add bits between 'msb' and 'lsb' from 'oldp' to 'pDrivers'
        अचर स्वतः addOldDriver = [&](File_पङ्क्ति* अचर flp, uint32_t msb, uint32_t lsb) {
            UASSERT_OBJ(propagatedDrivers.empty() || lsb > propagatedDrivers.back().m_hi, flp,
                        "Drivers should be in ascending order");
            DfgSel* अचर selp = make<DfgSel>(flp, DfgDataType::packed(msb - lsb + 1));
            selp->lsb(lsb);
            selp->fromp(oldp);
            propagatedDrivers.emplace_back(selp, lsb, flp);
        };

        // Incorporate old drivers
        लागि (अचर Driver& oDriver : oldDrivers) {
            File_पङ्क्ति* अचर flp = oDriver.m_flp;
            // Range to consider inserting, we will adjust oldLo as we process drivers
            uint32_t oldLo = oDriver.m_lo;
            अचर uint32_t oldHi = oDriver.m_hi;

            // Loop for now, can move to bisection search if this is a problem, shouldn't be ...
            लागि (अचर Driver& nDriver : newDrivers) {
                UASSERT_OBJ(oldHi >= oldLo, flp, "Should have stopped iteration");
                // If new driver is entirely below old driver, move on to
                यदि (nDriver.m_hi < oldLo) जारी;
                // If new driver is entirely above old driver, we can stop
                यदि (oldHi < nDriver.m_lo) रोक;

                // There is an overlap between 'oDriver' and 'nDriver'.
                // Insert the low bits and adjust the insertion range.
                // The rest will take care of itself on subsequent iterations.
                यदि (oldLo < nDriver.m_lo) addOldDriver(flp, nDriver.m_lo - 1, oldLo);
                oldLo = nDriver.m_hi + 1;

                // Stop if no more bits remaining in the old driver
                यदि (oldLo > oldHi) रोक;
            }

            // Insert remaining bits if any
            यदि (oldHi >= oldLo) addOldDriver(flp, oldHi, oldLo);
        }

        फर्काऊ propagatedDrivers;
    }

    // Propagate whole elements with a linear walk over the sorted driver lists.
    // Reads use the previous value temporary, preserving assignment-version bindings.
    // Partial elements and non-unit drivers retain the nonsynthesized-process fallback.
    तार्किक computePropagatedArrayDrivers(अचर std::vector<Driver>& newDrivers,
                                       DfgVertexVar* अचर oldp,
                                       std::vector<Driver>& propagatedDrivers) {
        // Bound quadratic vertex growth: array drivers cannot be coalesced.
        स्थिर constexpr uint32_t MAX_ARRAY_ELEMENTS = 32;
        यदि (oldp->size() > MAX_ARRAY_ELEMENTS) फर्काऊ असत्य;
        लागि (अचर Driver& driver : newDrivers) {
            यदि (!driverCoversWholeElement(driver)) फर्काऊ असत्य;
        }
        अचर std::vector<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.empty(), oldp, "Should have a proper driver");
        लागि (अचर Driver& driver : oldDrivers) {
            यदि (!driverCoversWholeElement(driver)) फर्काऊ असत्य;
        }

        propagatedDrivers.reserve(oldDrivers.size());
        स्वतः nIt = newDrivers.begin();
        लागि (अचर Driver& oDriver : oldDrivers) {
            जबसम्म (nIt != newDrivers.end() && nIt->m_lo < oDriver.m_lo) ++nIt;
            यदि (nIt != newDrivers.end() && nIt->m_lo == oDriver.m_lo) जारी;

            File_पङ्क्ति* अचर flp = oDriver.m_flp;
            अचर DfgUnitArray* अचर oldUnitp = oDriver.m_vtxp->as<DfgUnitArray>();
            DfgArraySel* अचर selp = make<DfgArraySel>(flp, oldUnitp->srcp()->dtype());
            selp->fromp(oldp);
            selp->bitp(make<DfgConst>(flp, static_cast<माप_प्रकार>(VL_IDATASIZE), oDriver.m_lo));
            DfgUnitArray* अचर newUnitp = make<DfgUnitArray>(flp, oldUnitp->dtype());
            newUnitp->srcp(selp);
            propagatedDrivers.emplace_back(newUnitp, oDriver.m_lo, flp);
        }
        फर्काऊ सत्य;
    }

    // Given the drivers of a variable after converting a single statement
    // 'newp', add drivers from 'oldp' that were not reassigned be drivers
    // in newp. This computes the total result of all previous assignments.
    तार्किक incorporate_Previous_मान(AstVarScope* vscp, DfgVertexVar* newp, DfgVertexVar* oldp) {
        UASSERT_OBJ(newp->srcp(), vscp, "Assigned variable has no driver");

        // Easy if there is no old value...
        यदि (!oldp) फर्काऊ सत्य;

        // New driver was not yet coalesced, so should always be a splice
        DfgVertexSplice* अचर nSplicep = newp->srcp()->as<DfgVertexSplice>();

        // If the old value is the real variable we just computed the new value for,
        // then it is the circular feedback into the synthesized block, add it as default driver.
        यदि (oldp->vscp() == vscp) {
            यदि (!spliceCoversWhole(nSplicep)) newp->defaultp(oldp);
            फर्काऊ सत्य;
        }

        UASSERT_OBJ(oldp->srcp(), vscp, "Previously assigned variable has no driver");

        // Gather drivers of 'newp' - they are in incresing range order with no overlaps
        UASSERT_OBJ(!newp->defaultp(), newp, "Converted value should not have default");
        std::vector<Driver> nDrivers = gatherDrivers(newp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!nDrivers.empty(), newp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::vector<Driver> pDrivers;
        यदि (newp->isPacked()) {
            pDrivers = computePropagatedDrivers(nDrivers, oldp);
        } अन्यथा यदि (!computePropagatedArrayDrivers(nDrivers, oldp, pDrivers)) {
            ++m_ctx.m_synt.nonSynArray;
            फर्काऊ असत्य;
        }

        यदि (!pDrivers.empty()) {
            // Need to merge propagated sources, so reset the splice
            nSplicep->resetDrivers();
            // Merge drivers - they are both sorted and non-overlapping
            std::vector<Driver> drivers;
            drivers.reserve(nDrivers.size() + pDrivers.size());
            std::merge(nDrivers.begin(), nDrivers.end(), pDrivers.begin(), pDrivers.end(),
                       std::back_inserter(drivers));
            // Coalesce adjacent ranges
            यदि (newp->isPacked()) coalesceDrivers(drivers);
            // Reinsert drivers in order
            लागि (अचर Driver& d : drivers) nSplicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
        }

        // If the old had a default, add to the new one too, unless redundant
        यदि (oldp->defaultp() && !spliceCoversWhole(nSplicep)) newp->defaultp(oldp->defaultp());

        // Done
        फर्काऊ सत्य;
    }

    // Synthesize the given statements with the given input symbol table.
    // Returns true if successfolly synthesized.
    // Populates the given output symbol table.
    // Populates the given reference with the condition of the terminator branch, if any.
    तार्किक synthesizeBasicBlock(SymTab& oSymTab, DfgVertex*& condpr,
                              अचर std::vector<Ast_गाँठो_Stmt*>& stmtps, अचर SymTab& iSymTab) {
        // Use fresh set of vertices in m_converter
        अचर VNUser2InUse user2InUse;

        // Initialize AstVarScope -> Vertex bindings available in this block
        लागि (अचर स्वतः& pair : iSymTab) {
            AstVarScope* अचर varp = pair.first;
            DfgVertexVar* अचर vtxp = pair.second;
            varp->user2p(vtxp);
            oSymTab[varp] = vtxp;
        }

        // Synthesize each statement one after the other
        std::vector<std::pair<AstVarScope*, DfgVertexVar*>> updates;
        लागि (Ast_गाँठो_Stmt* अचर stmtp : stmtps) {
            // Regular statements
            Ast_गाँठो_Assign* अचर ap = VN_CAST(stmtp, गाँठो_Assign);
            यदि (ap && (VN_IS(ap, Assign) || VN_IS(ap, AssignW))) {
                // Convert this assignment
                यदि (!m_converter.convert(updates, *m_logicp, ap)) {
                    ++m_ctx.m_synt.nonSynConv;
                    फर्काऊ असत्य;
                }
                // Apply variable updates from this statement
                लागि (अचर स्वतः& pair : updates) {
                    // The target variable that was assigned to
                    AstVarScope* अचर vscp = pair.first;
                    // The new, potentially partially assigned value
                    DfgVertexVar* अचर newp = pair.second;
                    // Normalize drivers within this statement, bail if multidriven
                    DfgVertexSplice* अचर srcp = newp->srcp()->as<DfgVertexSplice>();
                    std::vector<Driver> drivers = gatherDrivers(srcp);
                    अचर तार्किक single = drivers.size() == 1;
                    यदि (!normalizeDrivers(*newp, drivers)) {
                        ++m_ctx.m_synt.nonSynMultidrive;
                        फर्काऊ असत्य;
                    }
                    // If there were more than one driver (often not), re-add in case coalesced
                    यदि (!single) {
                        srcp->resetDrivers();
                        लागि (अचर Driver& d : drivers) srcp->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
                    }
                    // The old value, if any
                    DfgVertexVar* अचर oldp = vscp->user2u().ढाँचा to<DfgVertexVar*>();
                    // Inncorporate old value into the new value
                    यदि (!incorporate_Previous_मान(vscp, newp, oldp)) फर्काऊ असत्य;
                    // Update binding of target variable
                    vscp->user2p(newp);
                    // Update output symbol table of this block
                    oSymTab[vscp] = newp;
                }
                updates.clear();
                जारी;
            }

            // Terminator branches
            यदि (AstIf* अचर ifp = VN_CAST(stmtp, If)) {
                UASSERT_OBJ(ifp == stmtps.back(), ifp, "Branch should be last statement");
                // Convert condition, give up if failed
                DfgVertex* condp = m_converter.convert(*m_logicp, ifp->condp());
                यदि (!condp) {
                    ++m_ctx.m_synt.nonSynConv;
                    फर्काऊ असत्य;
                }
                // Single bit condition can be use directly, otherwise: use 'condp != 0'
                यदि (condp->width() != 1) {
                    File_पङ्क्ति* अचर flp = condp->fileline();
                    DfgNeq* अचर neqp = make<DfgNeq>(flp, DfgDataType::packed(1));
                    neqp->lhsp(make<DfgConst>(flp, condp->width(), 0U));
                    neqp->rhsp(condp);
                    condp = neqp;
                }
                condpr = condp;
                जारी;
            }

            // Unhandled
            ++m_ctx.m_synt.nonSynStmt;
            फर्काऊ असत्य;
        }

        फर्काऊ सत्य;
    }

    // Assign path predicates to the outgoing control flow edges of the given block
    शून्य assignPathPredicates(अचर CfgBlock& bb) {
        // Nothing to do for the exit block
        यदि (bb.isExit()) फर्काऊ;

        // Get the predicate of this block
        DfgVertex* अचर predp = [&]() -> DfgVertex* {
            // Entry block has no predecessors, use constant true
            यदि (bb.isEnter()) फर्काऊ make<DfgConst>(m_logicp->fileline(), 1U, 1U);

            // For any other block, 'or' together all the incoming predicates
            अचर स्वतः& inEdges = bb.inEdges();
            स्वतः it = inEdges.begin();
            DfgVertex* resp = m_edgeToPredicatep[static_cast<अचर CfgEdge&>(*it)];
            जबसम्म (++it != inEdges.end()) {
                DfgOr* अचर orp = make<DfgOr>(resp->fileline(), resp->dtype());
                orp->rhsp(resp);
                orp->lhsp(m_edgeToPredicatep[static_cast<अचर CfgEdge&>(*it)]);
                resp = orp;
            }
            फर्काऊ resp;
        }();

        अचर DfgDataType& dtype = predp->dtype();

        अचर स्वतः mkTmp = [&](File_पङ्क्ति* flp, अचर वर्ण* name, DfgVertex* srcp) {
            अचर std::string prefix = "_BB" + std::to_string(bb.id()) + "_" + name;
            DfgVertexVar* अचर tmpp = m_converter.createTmp(*m_logicp, flp, dtype, prefix);
            tmpp->srcp(srcp);
            फर्काऊ tmpp;
        };

        // Assign it to a variable in case it's used multiple times
        DfgVertexVar* अचर pInp = mkTmp(predp->fileline(), "PathIn", predp);

        // For uncondional branches, the successor predicate edge is the same
        यदि (!bb.isBranch()) {
            m_edgeToPredicatep[bb.takenEdgep()] = mkTmp(pInp->fileline(), "Goto", pInp);
            फर्काऊ;
        }

        // For branches, we need to factor in the branch condition
        DfgVertex* अचर condp = m_bbToCondp[bb];
        File_पङ्क्ति* अचर flp = condp->fileline();

        // Predicate for taken branch: 'predp & condp'
        {
            DfgAnd* अचर takenPredp = make<DfgAnd>(flp, dtype);
            takenPredp->lhsp(pInp);
            takenPredp->rhsp(condp);
            m_edgeToPredicatep[bb.takenEdgep()] = mkTmp(flp, "Taken", takenPredp);
        }

        // Predicate for untaken branch: 'predp & ~condp'
        {
            DfgAnd* अचर untknPredp = make<DfgAnd>(flp, dtype);
            untknPredp->lhsp(pInp);
            DfgNot* अचर notp = make<DfgNot>(flp, dtype);
            notp->srcp(condp);
            untknPredp->rhsp(notp);
            m_edgeToPredicatep[bb.untknEdgep()] = mkTmp(flp, "Untkn", untknPredp);
        }
    }

    // Returns true if all external updates to volatile variables are observed correctly
    तार्किक checkExtWrites() {
        लागि (अचर DfgVertex* अचर vtxp : m_logicp->synth()) {
            अचर DfgVertexVar* अचर varp = vtxp->cast<DfgVertexVar>();
            यदि (!varp) जारी;
            // If the variable we synthesized this vertex for is volatile, and
            // the value of the synthesized temporary is observed, we might be
            // missing an external update, so we mut give up.
            यदि (!varp->hasSinks()) जारी;
            यदि (!DfgVertexVar::isVolatile(varp->tmpForp())) जारी;
            ++m_ctx.m_synt.nonSynExtWrite;
            फर्काऊ असत्य;
        }
        फर्काऊ सत्य;
    }

    // Add the synthesized values as drivers to the output variables of the current DfgLogic
    तार्किक addSynthesizedOutput(SymTab& oSymTab) {
        // It's possible we think a variable is written by the DfgLogic when
        // it actauly isn't, e.g.: '{a[0], b[0]}[1] = ...' does not write 'b'.
        // These LHS forms can happen after some earlier tranforms. We
        // should just run V3Const on them earlier, but we will do belt and
        // braces and check here too. We can't touch any output variables if so.
        अचर तार्किक missing = m_logicp->foreachSink([&](अचर DfgVertex& sink) {
            अचर DfgUnresolved* अचर unresolvedp = sink.as<DfgUnresolved>();
            AstVarScope* अचर vscp = unresolvedp->singleSink()->as<DfgVertexVar>()->vscp();
            फर्काऊ !oSymTab.गन्ती(vscp);
        });
        यदि (missing) {
            ++m_ctx.m_synt.nonSynFalseWrite;
            फर्काऊ असत्य;
        }

        // Add sinks to read the computed values for the target variables
        फर्काऊ !m_logicp->foreachSink([&](DfgVertex& sink) {
            DfgUnresolved* अचर unresolvedp = sink.as<DfgUnresolved>();
            अचर DfgVertexVar* अचर varp = unresolvedp->singleSink()->as<DfgVertexVar>();
            DfgVertexVar* अचर resp = oSymTab.at(varp->vscp());
            UASSERT_OBJ(resp->srcp(), resp, "Undriven result");

            // If the output is not used further in the synthesized logic itself,
            // then resp will be deleted before we return, so we can just use
            // its splice directly without ending up with a multi-use operation.
            यदि (!resp->hasSinks()) {
                unresolvedp->addDriver(resp->srcp()->as<DfgVertexSplice>());
                फर्काऊ असत्य;  // OK, continue.
            }

            // TODO: computePropagatedDrivers cannot handle arrays, should
            // never happen with simple continous assignments
            यदि (!resp->isPacked()) {
                ++m_ctx.m_synt.nonSynArray;
                फर्काऊ सत्य;  // Not OK, give up
            }

            // We need to add a new splice to avoid multi-use of the original splice
            DfgSplicePacked* अचर splicep
                = नूतन DfgSplicePacked{m_dfg, resp->fileline(), resp->dtype()};
            // Drivers are the same
            अचर std::vector<Driver> drivers = computePropagatedDrivers({}, resp);
            लागि (अचर Driver& d : drivers) splicep->addDriver(d.m_vtxp, d.m_lo, d.m_flp);
            unresolvedp->addDriver(splicep);
            फर्काऊ असत्य;  // OK, continue
        });
    }

    // Synthesize the given AstAssignW. Returns true on success.
    तार्किक synthesizeAssignW(AstAssignW* nodep) {
        ++m_ctx.m_synt.आगत_Assign;

        // The input and output symbol tables
        SymTab iSymTab;
        SymTab oSymTab;

        // Initialzie input symbol table
        initialize_Entry_चिह्न_Table(iSymTab);

        // Synthesize as if it was in a single CfgBlock CFG
        DfgVertex* condp = nullptr;
        अचर तार्किक success = synthesizeBasicBlock(oSymTab, condp, {nodep}, iSymTab);
        UASSERT_OBJ(!condp, nodep, "Conditional AstAssignW ???");
        यदि (!success) फर्काऊ असत्य;

        // Check exernal writes are observed correctly
        यदि (!checkExtWrites()) फर्काऊ असत्य;

        // Add resolved output variable drivers
        फर्काऊ addSynthesizedOutput(oSymTab);
    }

    // Synthesize the given AstAlways. Returns true on success.
    तार्किक synthesizeCfg(CfgGraph& cfg) {
        ++m_ctx.m_synt.आगत_Always;

        // If there is a backward edge (loop), we can't synthesize it
        यदि (cfg.containsLoop()) {
            ++m_ctx.m_synt.nonSynLoop;
            ++m_ctx.m_synt.cfgCyclic;
            फर्काऊ असत्य;
        }

        // If it's a trivial CFG we can save on some work
        यदि (cfg.nBlocks() == 1) {
            ++m_ctx.m_synt.cfgTrivial;
        } अन्यथा {
            // Insert two-way join blocks to aid multiplexer ordering
            यदि (cfg.घुसाउ_Two_Way_Joins()) {
                ++m_ctx.m_synt.cfgSp;
            } अन्यथा {
                ++m_ctx.m_synt.cfgDag;
            }
            // Initialize maps needed for non-trivial CFGs
            m_dom_रूख = Cfg_Dominator_रूख{cfg};
            m_edgeToPredicatep = cfg.makeEdgeMap<DfgVertexVar*>();
        }

        // Initialize CfgMaps
        m_bbToISymTab = cfg.makeBlockMap<SymTab>();
        m_bbToOSymTab = cfg.makeBlockMap<SymTab>();
        m_bbToCondp = cfg.makeBlockMap<DfgVertexVar*>();

        // Synthesize all blocks
        लागि (अचर V3GraphVertex& vtx : cfg.vertices()) {
            अचर CfgBlock& bb = static_cast<अचर CfgBlock&>(vtx);
            // Prepare the input symbol table of this block (enter, or join predecessor blocks)
            यदि (!create_आगत_चिह्न_Table(bb)) फर्काऊ असत्य;
            // Synthesize this block
            DfgVertex* condp = nullptr;
            यदि (!synthesizeBasicBlock(m_bbToOSymTab[bb], condp, bb.stmtps(), m_bbToISymTab[bb])) {
                फर्काऊ असत्य;
            }
            // Create a temporary for the branch condition as it might be used multiple times
            यदि (condp) {
                File_पङ्क्ति* अचर flp = condp->fileline();
                अचर DfgDataType& dtype = condp->dtype();
                अचर std::string prefix = "_BB" + std::to_string(bb.id()) + "_Cond";
                DfgVertexVar* अचर vp = m_converter.createTmp(*m_logicp, flp, dtype, prefix);
                vp->srcp(condp);
                m_bbToCondp[bb] = vp;
            }
            // Set the path predicates on the successor edges
            assignPathPredicates(bb);
        }

        // Check exernal writes are observed correctly
        यदि (!checkExtWrites()) फर्काऊ असत्य;

        // Add resolved output variable drivers
        फर्काऊ addSynthesizedOutput(m_bbToOSymTab[cfg.निकास()]);
    }

    // Synthesize a DfgLogic into regular vertices. Returns ture on success.
    तार्किक synthesize(DfgLogic& vtx) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;

        यदि (AstAssignW* अचर ap = VN_CAST(vtx.nodep()->stmtsp(), AssignW)) {
            यदि (ap->nextp()) फर्काऊ असत्य;
            यदि (!synthesizeAssignW(ap)) फर्काऊ असत्य;
            ++m_ctx.m_synt.synthAssign;
            फर्काऊ सत्य;
        }

        यदि (!synthesizeCfg(vtx.cfg())) फर्काऊ असत्य;
        ++m_ctx.m_synt.synthAlways;
        फर्काऊ सत्य;
    }

    // Revert all DfgLogic in m_toRevert, or DfgLogic driving the DfgUnresolved
    // vertices in m_toRevert, and transitively the same for any DfgUnresolved
    // driven by the reverted DfgLogic. Delete all DfgUnresolved involed.
    शून्य revert(VDouble0& statCountr) {
        m_toRevert.foreach([&](DfgVertex& vtx) {
            // Process DfgLogic
            यदि (DfgLogic* अचर vtxp = vtx.cast<DfgLogic>()) {
                UASSERT_OBJ(vtxp->selectedForSynthesis(), vtxp, "Shouldn't reach here unselected");
                // Revert this logic
                UASSERT_OBJ(!vtxp->reverted(), vtxp, "Should be reverting now");
                ++statCountr;
                लागि (DfgVertex* अचर p : vtxp->synth()) VL_DO_DANGLING(p->unlinkDelete(m_dfg), p);
                vtxp->synth().clear();
                vtxp->setReverted();
                // Add all DfgUnresolved it drives to the work list
                vtxp->foreachSink([&](DfgVertex& snk) {
                    m_toRevert.थप_front(*snk.as<DfgUnresolved>());
                    फर्काऊ असत्य;
                });
                फर्काऊ;
            }

            // Process DfgUnresolved
            यदि (DfgUnresolved* अचर vtxp = vtx.cast<DfgUnresolved>()) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->setHasModWrRefs();
                // Add all driving DfgLogic to the work list
                vtxp->foreachSource([&](DfgVertex& src) {
                    DfgLogic* अचर lp = src.cast<DfgLogic>();
                    यदि (lp && !lp->reverted()) m_toRevert.थप_front(*lp);
                    फर्काऊ असत्य;
                });
                // Delete the DfgUnresolved driver
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
                फर्काऊ;
            }

            // There should be nothing else on the worklist
            vtx.v3fatalSrc("Unexpected vertex type");
        });
    }

    // Synthesize all of the given vertices
    शून्य main() {
        //-------------------------------------------------------------------
        UINFO(5, "Step 0: Remove all DfgLogic not selected for synthesis");
        लागि (DfgVertex* अचर vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgUnresolved
            यदि (!vtxp->is<DfgUnresolved>()) जारी;
            तार्किक anySelected = असत्य;
            तार्किक anyUnselected = असत्य;
            vtxp->foreachSource([&](DfgVertex& src) {
                अचर DfgLogic& logic = *src.as<DfgLogic>();
                यदि (logic.selectedForSynthesis()) {
                    anySelected = सत्य;
                } अन्यथा {
                    anyUnselected = सत्य;
                }
                फर्काऊ असत्य;
            });
            // There should be a driver
            UASSERT_OBJ(anySelected || anyUnselected, vtxp, "'DfgUnresolved' with no driver");
            // All drivers should be selected or all should be unselected
            UASSERT_OBJ(!(anySelected && anyUnselected), vtxp, "Invalid 'DfgLogic' selection");
            // If all drivers are unselected, delete this DfgUnresolved here
            यदि (anyUnselected) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->setHasModWrRefs();
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
        लागि (DfgVertex* अचर vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgLogic
            DfgLogic* अचर logicp = vtxp->cast<DfgLogic>();
            यदि (!logicp) जारी;
            यदि (logicp->selectedForSynthesis()) जारी;
            // There should be no sinks left for unselected DfgLogic, delete them here
            UASSERT_OBJ(!logicp->hasSinks(), vtxp, "Unselected 'DfgLogic' with sinks remaining");
            // Input variables will be read in Ast code, add Ast reference vertices
            // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
            अचर VNUser4InUse m_user4InUse;
            logicp->foreachSource([](DfgVertex& src) {
                src.as<DfgVertexVar>()->vscp()->user4p(&src);
                फर्काऊ असत्य;
            });
            V3DfgPasses::addAstRefs(m_dfg, logicp->nodep(), [](Ast_गाँठो* varp) {  //
                फर्काऊ varp->user4u().to<DfgVertexVar*>();
            });
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        debugDump("synth-selected");

        //-------------------------------------------------------------------
        UINFO(5, "Step 1: Attempting to synthesize each of the selected DfgLogic");
        {
            // AstVarScope::user4() -> int: unique ID for 'AstVarScopeComparator'
            अचर VNUser4InUse user4InUse;
            लागि (DfgVertex& vtx : m_dfg.opVertices()) {
                DfgLogic* अचर logicp = vtx.cast<DfgLogic>();
                यदि (!logicp) जारी;

                // We should only have DfgLogic remaining that was selected for synthesis
                UASSERT_OBJ(logicp->selectedForSynthesis(), logicp, "Unselected DfgLogic remains");

                // Debug aid
                अचर स्वतः debugCallback = [&]() -> शून्य {
                    // This is the breaking logic
                    m_debugLogicp = logicp;
                    // Dump it
                    UINFOTREE(0, logicp->nodep(), "Problematic DfgLogic: " << logicp, "  ");
                    V3EmitV::debug_Verilog_For_रूख(logicp->nodep(), std::cout);
                    debugDump("synth-lastok");
                };
                यदि (VL_UNLIKELY(s_dfgSynthDebugBisect.stop(debugCallback))) रोक;

                // Synthesize it, if failed, enqueue for reversion
                यदि (!synthesize(*logicp)) {
                    logicp->setNonSynthesizable();
                    m_toRevert.थप_front(*logicp);
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
        std::unordered_map<अचर DfgVertexVar*, DfgVertexSplice*> resolvedDrivers;
        // Compute resolved drivers of all variablees
        लागि (DfgVertexVar& var : m_dfg.varVertices()) {
            यदि (!var.srcp()) जारी;
            DfgUnresolved* अचर unresolvedp = var.srcp()->cast<DfgUnresolved>();
            यदि (!unresolvedp) रोक;  // Stop when reached the synthesized temporaries

            // Resolve the synthesized drivers
            DfgVertexSplice* अचर resolvedp = [&]() -> DfgVertexSplice* {
                // All synthesized drivers were normalized already,
                // so if there is only one, it can be used directly
                यदि (unresolvedp->nInputs() == 1) {
                    फर्काऊ unresolvedp->inputp(0)->as<DfgVertexSplice>();
                }
                // Otherwise gather the synthesized drivers
                std::vector<Driver> drivers = gatherDriversUnresolved(unresolvedp);
                // Normalize them, make resolved driver if all good
                यदि (normalizeDrivers(var, drivers)) फर्काऊ makeSplice(var, drivers);
                // If mutlidriven, record and ignore
                multidrivenps.emplace_back(&var);
                m_toRevert.थप_front(*unresolvedp);
                फर्काऊ nullptr;
            }();
            // Bail if multidriven
            यदि (!resolvedp) जारी;
            // Add to map for next loop
            अचर तार्किक newEntry = resolvedDrivers.emplace(&var, resolvedp).second;
            UASSERT_OBJ(newEntry, &var, "Duplicate driver");
        }
        // Revert and remove drivers of multi-driven variables
        revert(m_ctx.m_synt.revertMultidrive);
        // Replace all DfgUnresolved with the resolved drivers
        लागि (अचर DfgVertexVar& var : m_dfg.varVertices()) {
            यदि (!var.srcp()) जारी;
            DfgUnresolved* अचर srcp = var.srcp()->cast<DfgUnresolved>();
            यदि (!srcp) रोक;  // Stop when reached the synthesized temporaries

            // Replace it
            srcp->replaceWith(resolvedDrivers.at(&var));
            VL_DO_DANGLING(srcp->unlinkDelete(m_dfg), srcp);
        }
        debugDump("synth-resolved");

        //-------------------------------------------------------------------
        UINFO(5, "Step 4: Remove all remaining DfgLogic and DfgUnresolved");
        लागि (DfgVertex* अचर vtxp : m_dfg.opVertices().unlinkable()) {
            // Previous step should have removed all DfgUnresolved
            UASSERT_OBJ(!vtxp->is<DfgUnresolved>(), vtxp, "DfgUnresolved remains");

            // Process only DfgLogic
            DfgLogic* अचर logicp = vtxp->cast<DfgLogic>();
            यदि (!logicp) जारी;

            // Earlier pass should have removed all sinks
            UASSERT_OBJ(!logicp->hasSinks(), logicp, "DfgLogic sink remains");

            यदि (!logicp->synth().empty()) {
                // If synthesized, delete the corresponding AstNode. It is now in Dfg.
                logicp->nodep()->unlinkFrBack()->delete_रूख();
            } अन्यथा {
                // Not synthesized. Logic stays in Ast. Add Ast reference vertices.
                // Outputs already marked by revertTransivelyAndRemove.
                // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
                अचर VNUser4InUse m_user4InUse;
                logicp->foreachSource([](DfgVertex& src) {
                    src.as<DfgVertexVar>()->vscp()->user4p(&src);
                    फर्काऊ असत्य;
                });
                V3DfgPasses::addAstRefs(m_dfg, logicp->nodep(), [](Ast_गाँठो* varp) {  //
                    फर्काऊ varp->user4u().to<DfgVertexVar*>();
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
        लागि (DfgVertex* अचर vtxp : m_dfg.opVertices().unlinkable()) {
            DfgVertexSplice* अचर splicep = vtxp->cast<DfgVertexSplice>();
            यदि (!splicep) जारी;

            // Might not have a sink if the driving logic was revered, remove
            यदि (!splicep->hasSinks()) {
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
                जारी;
            }

            // It should alway have drivers
            UASSERT_OBJ(splicep->nInputs(), splicep, "Splice with no drivers");

            // If redundant, remove it
            यदि (DfgVertex* अचर wholep = splicep->wholep()) {
                यदि (DfgVertexVar* अचर varp = splicep->singleSink()->cast<DfgVertexVar>()) {
                    varp->driver_File_पङ्क्ति(splicep->driver_File_पङ्क्ति(0));
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
        यदि (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("synth-rmunused");

        // No operation vertex should have multiple sinks. Cyclic decomoposition
        // depends on this and it can easily be ensured by using temporaries.
        // Also, all sources should be connected at this point
        यदि (v3Global.opt.debugCheck()) {
            लागि (DfgVertex& vtx : dfg.opVertices()) {
                UASSERT_OBJ(!vtx.hasMultipleSinks(), &vtx, "Operation has multiple sinks");
                लागि (माप_प्रकार i = 0; i < vtx.nInputs(); ++i) {
                    UASSERT_OBJ(vtx.inputp(i), &vtx, "Unconnected source operand");
                }
            }
            V3DfgPasses::typeCheck(dfg);
        }
    }
};

पूर्णाङ्क AstToDfgSynthesize::AstVarScopeComparator::s_vscpIdCounter = 0;

// Decide which DfgLogic to attempt to synthesize
स्थिर शून्य dfgSelectLogicForSynthesis(DfgGraph& dfg) {
    // If we are told to synthesize everything, we will do so ...
    यदि (v3Global.opt.fDfgSynthesizeAll()) {
        लागि (DfgVertex& vtx : dfg.opVertices()) {
            यदि (DfgLogic* अचर logicp = vtx.cast<DfgLogic>()) logicp->setSelectedForSynthesis();
        }
        फर्काऊ;
    }

    // Otherwise figure out which vertices are likely worth synthesizing.

    std::vector<DfgVertexVar*> circularVarps;
    // Synthesize all drivers of circular variables
    {
        DfgUserMap<uint64_t> scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, scc);
        लागि (DfgVertexVar& var : dfg.varVertices()) {
            यदि (!scc.at(var)) जारी;
            // This is a circular variable
            circularVarps.emplace_back(&var);
        }
    }

    // To cover all drivers, use a work list
    DfgWorklist worklist{dfg};

    // Synthesize all drivers of circular variables
    लागि (अचर DfgVertexVar* अचर varp : circularVarps) {
        varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
            worklist.थप_front(*driver.as<DfgLogic>());
            फर्काऊ असत्य;
        });
    }

    // Choose some simple special cases to always synthesize
    लागि (DfgVertex& vtx : dfg.opVertices()) {
        DfgLogic* अचर logicp = vtx.cast<DfgLogic>();
        यदि (!logicp || worklist.contains(*logicp)) जारी;
        // If drives an unused variable, synthesize it so the partial logic can be removed
        यदि (logicp->drivesUnusedVars()) {
            worklist.थप_front(*logicp);
            जारी;
        }
        // Blocks corresponding to continuous assignments
        यदि (logicp->nodep()->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            worklist.थप_front(*logicp);
            जारी;
        }
        अचर CfgGraph& cfg = logicp->cfg();
        // Straight line code with no branches
        यदि (cfg.nBlocks() == 1) {
            worklist.थप_front(*logicp);
            जारी;
        }
        // Blocks driving exactly 1 variable
        यदि (!logicp->hasMultipleSinks()) worklist.थप_front(*logicp);
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
            यदि (!isVisited.घुसाउ(unresolved).second) फर्काऊ असत्य;
            unresolved->foreachSource([&](DfgVertex& sibling) {
                DfgLogic& siblingLogic = *sibling.as<DfgLogic>();
                यदि (!siblingLogic.selectedForSynthesis()) worklist.थप_front(siblingLogic);
                फर्काऊ असत्य;
            });
            फर्काऊ असत्य;
        });
    });
}

शून्य V3DfgPasses::synthesize(DfgGraph& dfg, V3DfgContext& ctx) {
    // Select which DfgLogic to attempt to synthesize
    dfgSelectLogicForSynthesis(dfg);
    // Synthesize them - also removes un-synthesized DfgLogic, so must run even if nothing selected
    AstToDfgSynthesize::apply(dfg, ctx.m_synthContext);
}
