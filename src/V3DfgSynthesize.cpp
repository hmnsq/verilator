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
प्रतिरूपम् <प्रकारनाम T_Vertex, प्रकारनाम T_ग्रन्थिः>
T_Vertex* makeVertex(ध्रुव T_ग्रन्थिः* nodep, DfgGraph& dfg, ध्रुव Dfg_दत्त_Type& dtype) {
    प्रत्यागम नूतन T_Vertex{dfg, nodep->fileline(), dtype};
}

प्रतिरूपम् <>
DfgArraySel* makeVertex<DfgArraySel, AstArraySel>(ध्रुव AstArraySel* nodep, DfgGraph& dfg,
                                                  ध्रुव Dfg_दत्त_Type& dtype) {
    // Some earlier passes create malformed ArraySels, just bail on those...
    // See t_bitsel_wire_array_bad
    यदि (VN_IS(nodep->fromp(), Const)) प्रत्यागम शून्यसूचकः;
    यदि (!VN_IS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) प्रत्यागम शून्यसूचकः;
    प्रत्यागम नूतन DfgArraySel{dfg, nodep->fileline(), dtype};
}

}  // namespace

// Visitor that can convert Ast statements and expressions in Dfg vertices
वर्ग Ast_गन्तव्यम्_Dfg_Converter final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats

    // Current logic vertex we are synthesizing
    DfgLogic* m_logicp = शून्यसूचकः;
    // Variable updates produced by currently converted statement. This almost
    // always have a single element, so a vector is ok
    std::सदिशम्<std::pair<AstVarScope*, DfgVertexVar*>>* m_updatesp = शून्यसूचकः;

    तार्किक m_प्राप्तम्_Unhandled = असत्य;  // Found node not implemented as DFG or not implemented 'visit'
    तार्किक m_converting = असत्य;  // We are trying to convert some logic at the moment

    // METHODS

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    प्रतिरूपम् <प्रकारनाम Vertex, प्रकारनाम... तर्काः_2>
    Vertex* make(तर्काः_2&&... तर्काः) {
        static_assert(!std::is_आधारः_सम्बन्धि<DfgVertexVar, Vertex>::मानम्, "Do not use for variables");
        static_assert(std::is_आधारः_सम्बन्धि<DfgVertex, Vertex>::मानम्, "'Vertex' must be a 'DfgVertex'");
        Vertex* ध्रुव vtxp = नूतन Vertex{m_dfg, std::forward<तर्काः_2>(तर्काः)...};
        m_logicp->synth().emplace_पृष्ठभागः(vtxp);
        प्रत्यागम vtxp;
    }

    // Returns true if the expression cannot (or should not) be represented by DFG
    तार्किक unhandled(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        // Short-circuiting if something was already unhandled
        यदि (m_प्राप्तम्_Unhandled) {
            // Impure nodes cannot be represented
            यदि (!nodep->is_विशुद्धम्()) {
                m_प्राप्तम्_Unhandled = सत्य;
                ++m_ctx.m_conv.nonRepImpure;
            }
        }
        प्रत्यागम m_प्राप्तम्_Unhandled;
    }

    तार्किक isSupported(ध्रुव AstVarRef* nodep) {
        // Cannot represent cross module references
        यदि (nodep->classOrPackagep()) प्रत्यागम असत्य;
        // Check target
        प्रत्यागम V3Dfg::isSupported(nodep->varScopep());
    }

    // Given an RValue expression, return the equivalent Vertex, or nullptr if not representable.
    DfgVertex* convertRValue(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        UASSERT_OBJ(!m_converting, nodep, "'convertingRValue' should not be called recursively");
        VL_RESTORER(m_converting);
        VL_RESTORER(m_प्राप्तम्_Unhandled);
        m_converting = सत्य;
        m_प्राप्तम्_Unhandled = असत्य;

        // Convert the expression
        iterate(nodep);

        // If falied to convert, return nullptr
        यदि (m_प्राप्तम्_Unhandled) प्रत्यागम शून्यसूचकः;

        // Traversal set user2p to the equivalent vertex
        DfgVertex* ध्रुव vtxp = nodep->user2u().गन्तव्यम्<DfgVertex*>();
        UASSERT_OBJ(vtxp, nodep, "Missing Dfg vertex after conversion");
        प्रत्यागम vtxp;
    }

    // Given an LValue expression, return the splice node that writes the
    // destination, together with the index to use for splicing in the value.
    // Returns {nullptr, 0}, if the given LValue expression is not supported.
    std::pair<DfgVertexSplice*, uint32_t> convertLValue(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (ध्रुव AstVarRef* ध्रुव vrefp = VN_CAST(nodep, VarRef)) {
            UASSERT_OBJ(vrefp->access().isWriteOnly(), vrefp, "Non-WriteOnly reference");
            यदि (!isSupported(vrefp)) {
                ++m_ctx.m_conv.nonRepLValue;
                प्रत्यागम {शून्यसूचकः, 0};
            }

            // Get (or create a new) temporary for this variable
            ध्रुव DfgVertexVar* ध्रुव vtxp = [&]() -> DfgVertexVar* {
                // The variable being assigned
                AstVarScope* ध्रुव vscp = vrefp->varScopep();

                // Find existing one, if any
                प्रति (ध्रुव स्वचालित& pair : *m_updatesp) {
                    यदि (pair.first == vscp) प्रत्यागम pair.second;
                }

                // Create new one
                DfgVertexVar* ध्रुव newp = createTmp(*m_logicp, vscp, "SynthAssign");
                m_updatesp->emplace_पृष्ठभागः(vscp, newp);

                // Create the Splice driver for the new temporary
                यदि (newp->is<DfgVarPacked>()) {
                    newp->srcp(make<DfgSplicePacked>(newp->fileline(), newp->dtype()));
                } अन्यथा यदि (newp->is<DfgVarArray>()) {
                    newp->srcp(make<DfgSpliceArray>(newp->fileline(), newp->dtype()));
                } अन्यथा {
                    nodep->v3fatalSrc("Unhandled DfgVertexVar sub-type");
                }

                // Use new temporary
                प्रत्यागम newp;
            }();

            // Return the Splice driver
            प्रत्यागम {vtxp->srcp()->as<DfgVertexSplice>(), 0};
        }

        यदि (ध्रुव AstSel* selp = VN_CAST(nodep, Sel)) {
            // Only handle constant selects
            ध्रुव AstConst* ध्रुव lsbp = VN_CAST(selp->lsbp(), Const);
            यदि (!lsbp) {
                ++m_ctx.m_conv.nonRepLValue;
                प्रत्यागम {शून्यसूचकः, 0};
            }
            uint32_t lsb = lsbp->गन्तव्यम्_UInt();

            // Convert the 'fromp' sub-expression
            ध्रुव स्वचालित pair = convertLValue(selp->fromp());
            यदि (!pair.first) प्रत्यागम {शून्यसूचकः, 0};
            DfgSplicePacked* ध्रुव splicep = pair.first->प्रतिरूपम् as<DfgSplicePacked>();
            // Adjust index.
            lsb += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            यदि (lsb + static_cast<uint32_t>(selp->विस्तारः_Const()) > splicep->आकार()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                प्रत्यागम {शून्यसूचकः, 0};
            }

            // AstSel doesn't change type kind (array vs packed), so we can use
            // the existing splice driver with adjusted lsb
            प्रत्यागम {splicep, lsb};
        }

        यदि (ध्रुव AstArraySel* ध्रुव aselp = VN_CAST(nodep, ArraySel)) {
            // Only handle constant selects
            ध्रुव AstConst* ध्रुव indexp = VN_CAST(aselp->bitp(), Const);
            यदि (!indexp) {
                ++m_ctx.m_conv.nonRepLValue;
                प्रत्यागम {शून्यसूचकः, 0};
            }
            uint32_t सूचकांक = indexp->गन्तव्यम्_UInt();

            // Convert the 'fromp' sub-expression
            ध्रुव स्वचालित pair = convertLValue(aselp->fromp());
            यदि (!pair.first) प्रत्यागम {शून्यसूचकः, 0};
            DfgSpliceArray* ध्रुव splicep = pair.first->प्रतिरूपम् as<DfgSpliceArray>();
            // Adjust index. Note pair.second is always 0, but we might handle array slices later..
            सूचकांक += pair.second;

            // Don't optimize if statically out of bounds. TODO: Maybe later ...
            यदि (सूचकांक + 1U > splicep->आकार()) {
                ++m_ctx.m_conv.nonRepOOBSel;
                प्रत्यागम {शून्यसूचकः, 0};
            }

            // Ensure the Splice driver exists for this element
            यदि (!splicep->driverAt(सूचकांक)) {
                File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
                // This should never fail
                ध्रुव Dfg_दत्त_Type& dtype = *Dfg_दत्त_Type::fromAst(nodep->dtypep());
                यदि (dtype.isPacked()) {
                    DfgSplicePacked* ध्रुव newp = make<DfgSplicePacked>(flp, dtype);
                    ध्रुव Dfg_दत्त_Type& uaDtype = Dfg_दत्त_Type::array(dtype, 1);
                    DfgUnitArray* ध्रुव uap = make<DfgUnitArray>(flp, uaDtype);
                    uap->srcp(newp);
                    splicep->सङ्कलय_Driver(uap, सूचकांक, flp);
                } अन्यथा यदि (dtype.isArray()) {
                    DfgSpliceArray* ध्रुव newp = make<DfgSpliceArray>(flp, dtype);
                    splicep->सङ्कलय_Driver(newp, सूचकांक, flp);
                } अन्यथा {
                    nodep->v3fatalSrc("Unhandled data type kind");
                }
            }

            // Return the splice driver
            DfgVertex* driverp = splicep->driverAt(सूचकांक);
            यदि (ध्रुव DfgUnitArray* ध्रुव uap = driverp->cast<DfgUnitArray>()) {
                driverp = uap->srcp();
            }
            प्रत्यागम {driverp->as<DfgVertexSplice>(), 0};
        }

        ++m_ctx.m_conv.nonRepLValue;
        प्रत्यागम {शून्यसूचकः, 0};
    }

    // Given the LHS of an assignment, and the vertex representing the RHS,
    // connect up the RHS to drive the targets.
    // Returns true on success, false if the LHS is not representable.
    तार्किक convertAssignment(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, DfgVertex* vtxp) {
        // Represents a DFG assignment contributed by the AST assignment with the above 'lhsp'.
        // There might be multiple of these if 'lhsp' is a concatenation.
        संरचना Assignment final {
            DfgVertexSplice* m_lhsp;  // Splice vertex being driven
            uint32_t m_idx;  // Position (bit or array index) within m_lhsp being driven
            DfgVertex* m_rhsp;  // Vertex driving that position
            Assignment() = विलोपय;
            Assignment(DfgVertexSplice* lhsp, uint32_t idx, DfgVertex* rhsp)
                : m_lhsp{lhsp}
                , m_idx{idx}
                , m_rhsp{rhsp} {}
        };

        // Simplify the LHS, to get rid of things like SEL(CONCAT(_, _), _)
        यदि (!VN_IS(lhsp, VarRef)) lhsp = VN_AS(V3Const::constifyExpensiveEdit(lhsp), ग्रन्थिः_व्यञ्जकम्);

        // Assigning compound expressions to a concatenated LHS requires a temporary
        // to avoid multiple use of the expression
        यदि (VN_IS(lhsp, Concat) && !vtxp->is<DfgVertexVar>() && !vtxp->is<DfgConst>()) {
            DfgVertexVar* ध्रुव tmpp = createTmp(*m_logicp, flp, vtxp->dtype(), "Unpack");
            tmpp->srcp(vtxp);
            vtxp = tmpp;
        }

        // Convert each concatenation LHS separately, gather all assignments
        // we need to do into 'assignments', return true if all LValues
        // converted successfully.
        std::सदिशम्<Assignment> assignments;
        ध्रुव std::function<तार्किक(Ast_ग्रन्थिः_व्यञ्जकम्*, uint32_t)> convertAllLValues
            = [&](Ast_ग्रन्थिः_व्यञ्जकम्* subp, uint32_t lsb) -> तार्किक {
            // Concatenation on the LHS, convert each part
            यदि (AstConcat* ध्रुव concatp = VN_CAST(subp, Concat)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cRhsp = concatp->rhsp();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cLhsp = concatp->lhsp();
                // Convert Rigth of concat
                यदि (!convertAllLValues(cRhsp, lsb)) प्रत्यागम असत्य;
                // Convert Left of concat
                प्रत्यागम convertAllLValues(cLhsp, lsb + cRhsp->विस्तारः());
            }

            // Non-concatenation, convert the LValue
            ध्रुव स्वचालित pair = convertLValue(subp);
            यदि (!pair.first) प्रत्यागम असत्य;

            // If whole lhs, just use it
            यदि (subp == lhsp) {
                assignments.emplace_पृष्ठभागः(pair.first, pair.second, vtxp);
                प्रत्यागम सत्य;
            }

            // Otherwise select the relevant bits
            ध्रुव Dfg_दत्त_Type& dtype = *Dfg_दत्त_Type::fromAst(subp->dtypep());
            DfgSel* ध्रुव selp = make<DfgSel>(subp->fileline(), dtype);
            selp->fromp(vtxp);
            selp->lsb(lsb);
            assignments.emplace_पृष्ठभागः(pair.first, pair.second, selp);
            प्रत्यागम सत्य;
        };

        // Convert the given LHS assignment, give up if any LValues failed to convert
        यदि (!convertAllLValues(lhsp, 0)) प्रत्यागम असत्य;

        // All successful, connect the drivers
        प्रति (ध्रुव Assignment& item : assignments) {
            यदि (DfgSplicePacked* ध्रुव spp = item.m_lhsp->प्रतिरूपम् cast<DfgSplicePacked>()) {
                spp->सङ्कलय_Driver(item.m_rhsp, item.m_idx, flp);
            } अन्यथा यदि (DfgSpliceArray* ध्रुव sap = item.m_lhsp->प्रतिरूपम् cast<DfgSpliceArray>()) {
                // TODO: multi-dimensional arrays will need changes here
                ध्रुव Dfg_दत्त_Type& rDt = item.m_rhsp->dtype();
                यदि (rDt.isPacked()) {
                    // RHS is assigning an element of this array. Need a DfgUnitArray adapter.
                    DfgUnitArray* ध्रुव uap = make<DfgUnitArray>(flp, Dfg_दत्त_Type::array(rDt, 1));
                    uap->srcp(item.m_rhsp);
                    sap->सङ्कलय_Driver(uap, item.m_idx, flp);
                } अन्यथा {
                    // RHS is assigning an array (or array slice). Should be the same element type.
                    sap->सङ्कलय_Driver(item.m_rhsp, item.m_idx, flp);
                }
            } अन्यथा {
                item.m_lhsp->v3fatalSrc("Unhandled DfgVertexSplice sub-type");
            }
        }

        प्रत्यागम सत्य;
    }

    // VISITORS

    // Unhandled node
    शून्य दर्शय(Ast_ग्रन्थिः* /*nodep*/) override {
        यदि (!m_प्राप्तम्_Unhandled && m_converting) ++m_ctx.m_conv.nonRepUnknown;
        m_प्राप्तम्_Unhandled = सत्य;
    }

    // Expressions - mostly auto generated, but a few special ones
    शून्य दर्शय(AstVarRef* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        यदि (unhandled(nodep)) प्रत्यागम;
        // This visit method is only called on RValues, where only read refs are supported
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Non-ReadOnly reference");
        यदि (!isSupported(nodep)) {
            m_प्राप्तम्_Unhandled = सत्य;
            ++m_ctx.m_conv.nonRepVarRef;
            प्रत्यागम;
        }

        // Variable should have been bound before starting conversion
        DfgVertex* ध्रुव vtxp = nodep->varScopep()->user2u().प्रतिरूपम् गन्तव्यम्<DfgVertexVar*>();
        UASSERT_OBJ(vtxp, nodep, "Referenced variable has no associated DfgVertexVar");
        nodep->user2p(vtxp);
    }
    शून्य दर्शय(AstConst* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        यदि (unhandled(nodep)) प्रत्यागम;

        यदि (nodep->विस्तारः() != nodep->अङ्कः().विस्तारः()) {
            // Sometimes the width of the AstConst is not the same as the
            // V3Number it holds. Truncate it here. TODO: should this be allowed?
            V3_सङ्ख्या अङ्कः{nodep, nodep->विस्तारः()};
            अङ्कः.opSel(nodep->अङ्कः(), nodep->विस्तारः() - 1, 0);
            DfgVertex* ध्रुव vtxp = make<DfgConst>(nodep->fileline(), अङ्कः);
            nodep->user2p(vtxp);
        } अन्यथा {
            DfgVertex* ध्रुव vtxp = make<DfgConst>(nodep->fileline(), nodep->अङ्कः());
            nodep->user2p(vtxp);
        }
    }
    शून्य दर्शय(AstCReset* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        यदि (unhandled(nodep)) प्रत्यागम;

        ध्रुव Dfg_दत्त_Type* ध्रुव dtypep = Dfg_दत्त_Type::fromAst(nodep->dtypep());
        यदि (!dtypep) {
            m_प्राप्तम्_Unhandled = सत्य;
            ++m_ctx.m_conv.nonRepDType;
            प्रत्यागम;
        }

        UASSERT_OBJ(!nodep->constructing(), nodep,
                    "CReset should be non-constructing at this stage");

        DfgVertex* ध्रुव vtxp = make<DfgCReset>(nodep->fileline(), *dtypep);
        nodep->user2p(vtxp);
    }
    शून्य दर्शय(Ast_मेलनम्_Masked* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        यदि (unhandled(nodep)) प्रत्यागम;

        ध्रुव Dfg_दत्त_Type* ध्रुव dtypep = Dfg_दत्त_Type::fromAst(nodep->dtypep());
        यदि (!dtypep) {
            m_प्राप्तम्_Unhandled = सत्य;
            ++m_ctx.m_conv.nonRepDType;
            प्रत्यागम;
        }

        iterate(nodep->lhsp());
        यदि (m_प्राप्तम्_Unhandled) प्रत्यागम;
        iterate(nodep->matchp());
        यदि (m_प्राप्तम्_Unhandled) प्रत्यागम;

        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Dfg_मेलनम्_Masked* ध्रुव vtxp = make<Dfg_मेलनम्_Masked>(flp, *dtypep);
        vtxp->lhsp(nodep->lhsp()->user2u().गन्तव्यम्<DfgVertex*>());
        vtxp->matchp(nodep->matchp()->user2u().गन्तव्यम्<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    शून्य दर्शय(AstReplicate* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        यदि (unhandled(nodep)) प्रत्यागम;

        ध्रुव Dfg_दत्त_Type* ध्रुव dtypep = Dfg_दत्त_Type::fromAst(nodep->dtypep());
        यदि (!dtypep) {
            m_प्राप्तम्_Unhandled = सत्य;
            ++m_ctx.m_conv.nonRepDType;
            प्रत्यागम;
        }

        iterate(nodep->srcp());
        यदि (m_प्राप्तम्_Unhandled) प्रत्यागम;

        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        DfgRep* ध्रुव vtxp = make<DfgRep>(flp, *dtypep);
        vtxp->srcp(nodep->srcp()->user2u().गन्तव्यम्<DfgVertex*>());
        nodep->user2p(vtxp);
    }
    शून्य दर्शय(AstSel* nodep) override {
        UASSERT_OBJ(m_converting, nodep, "AstToDfg visit called without m_converting");
        UASSERT_OBJ(!nodep->user2p(), nodep, "Already has Dfg vertex");
        यदि (unhandled(nodep)) प्रत्यागम;

        ध्रुव Dfg_दत्त_Type* ध्रुव dtypep = Dfg_दत्त_Type::fromAst(nodep->dtypep());
        यदि (!dtypep) {
            m_प्राप्तम्_Unhandled = सत्य;
            ++m_ctx.m_conv.nonRepDType;
            प्रत्यागम;
        }

        iterate(nodep->fromp());
        यदि (m_प्राप्तम्_Unhandled) प्रत्यागम;

        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        DfgVertex* vtxp = शून्यसूचकः;
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->lsbp(), Const)) {
            ध्रुव uint32_t lsb = constp->गन्तव्यम्_UInt();
            ध्रुव uint32_t msb = lsb + nodep->विस्तारः_Const() - 1;
            DfgVertex* ध्रुव fromp = nodep->fromp()->user2u().गन्तव्यम्<DfgVertex*>();
            // Unfortunately we can still have out of bounds selects due to how
            // indices are truncated for speed reasons in V3Width/V3Unknown.
            यदि (msb >= fromp->आकार()) {
                m_प्राप्तम्_Unhandled = सत्य;
                ++m_ctx.m_conv.nonRepOOBSel;
                प्रत्यागम;
            }
            DfgSel* ध्रुव selp = make<DfgSel>(flp, *dtypep);
            selp->fromp(fromp);
            selp->lsb(lsb);
            vtxp = selp;
        } अन्यथा {
            iterate(nodep->lsbp());
            यदि (m_प्राप्तम्_Unhandled) प्रत्यागम;
            DfgMux* ध्रुव muxp = make<DfgMux>(flp, *dtypep);
            muxp->fromp(nodep->fromp()->user2u().गन्तव्यम्<DfgVertex*>());
            muxp->lsbp(nodep->lsbp()->user2u().गन्तव्यम्<DfgVertex*>());
            vtxp = muxp;
        }
        nodep->user2p(vtxp);
    }

// The rest of the visit methods for expressions are generated by 'astgen'
#अन्तर्भाव "V3Dfg__gen_ast_to_dfg.h"

सार्वजनिक:
    // PUBLIC METHODS

    // Create temporay variable capable of holding the given type
    DfgVertexVar* createTmp(DfgLogic& logic, File_पङ्क्तिः* flp, ध्रुव Dfg_दत्त_Type& dtype,
                            ध्रुव std::string& उपसर्गः) {
        DfgVertexVar* ध्रुव vtxp = m_dfg.makeNewVar(flp, उपसर्गः, dtype, logic.scopep());
        logic.synth().emplace_पृष्ठभागः(vtxp);
        vtxp->vscp()->varp()->is_आन्तरिकम्(सत्य);
        vtxp->tmpForp(vtxp->vscp());
        प्रत्यागम vtxp;
    }

    // Create a new temporary variable capable of holding 'varp'
    DfgVertexVar* createTmp(DfgLogic& logic, AstVarScope* vscp, ध्रुव std::string& उपसर्गः) {
        AstVar* ध्रुव astVarp = vscp->varp();
        File_पङ्क्तिः* ध्रुव flp = astVarp->fileline();
        ध्रुव Dfg_दत्त_Type& dtype = *Dfg_दत्त_Type::fromAst(astVarp->dtypep());
        ध्रुव std::string prfx = उपसर्गः + "_" + astVarp->name();
        DfgVertexVar* ध्रुव vtxp = createTmp(logic, flp, dtype, prfx);
        vtxp->tmpForp(vscp);
        प्रत्यागम vtxp;
    }

    // Convert AstAssign to Dfg, return true if successful.
    // Fills 'updates' with bindings for assigned variables.
    तार्किक convert(std::सदिशम्<std::pair<AstVarScope*, DfgVertexVar*>>& updates, DfgLogic& vtx,
                 Ast_ग्रन्थिः_नियोजय* nodep) {
        UASSERT_OBJ(VN_IS(nodep, नियोजय) || VN_IS(nodep, नियोजय_W), nodep, "Bad NodeAssign");
        UASSERT_OBJ(updates.रिक्तानि(), nodep, "'updates' should be empty");
        VL_RESTORER(m_updatesp);
        VL_RESTORER(m_logicp);
        m_updatesp = &updates;
        m_logicp = &vtx;
        // Assignment with timing control shouldn't make it this far
        UASSERT_OBJ(!nodep->timingControlp(), nodep, "Shouldn't make it this far");
        // Convert it
        ++m_ctx.m_conv.निवेशः_Assignments;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp();
        // Check data types are compatible.
        ध्रुव Dfg_दत्त_Type* ध्रुव lDtypep = Dfg_दत्त_Type::fromAst(lhsp->dtypep());
        ध्रुव Dfg_दत्त_Type* ध्रुव rDtypep = Dfg_दत्त_Type::fromAst(rhsp->dtypep());
        यदि (!lDtypep || !rDtypep) {
            ++m_ctx.m_conv.nonRepDType;
            प्रत्यागम असत्य;
        }
        // For now, only direct array assignment is supported (e.g. a = b, but not a = _ ? b : c)
        यदि (rDtypep->isArray()) {
            यदि (!VN_IS(rhsp, VarRef)
                || !lhsp->dtypep()->skipRefp()->same_वृक्षः(rhsp->dtypep()->skipRefp())) {
                ++m_ctx.m_conv.nonRepDType;
                प्रत्यागम असत्य;
            }
        }
        // Widths should match at this point
        UASSERT_OBJ(lhsp->विस्तारः() == rhsp->विस्तारः(), nodep, "Mismatched width reached DFG");
        // Convert the RHS expression
        DfgVertex* ध्रुव rVtxp = convertRValue(rhsp);
        यदि (!rVtxp) प्रत्यागम असत्य;
        // Connect the RHS vertex to the LHS targets
        ध्रुव तार्किक success = convertAssignment(nodep->fileline(), lhsp, rVtxp);
        यदि (success) ++m_ctx.m_conv.representable;
        प्रत्यागम success;
    }

    // Convert RValue expression to Dfg. Returns nullptr if failed.
    DfgVertex* convert(DfgLogic& vtx, Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;
        // Convert it
        ++m_ctx.m_conv.निवेशः_Expressions;
        DfgVertex* ध्रुव vtxp = convertRValue(nodep);
        यदि (vtxp) ++m_ctx.m_conv.representable;
        प्रत्यागम vtxp;
    }

    // CONSTRUCTOR
    Ast_गन्तव्यम्_Dfg_Converter(DfgGraph& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {}
};

// Debug aid - outisde 'AstToDfgSynthesize' as it is a template, but want one instance
V3DebugBisect s_dfgSynthDebugBisect{"DfgSynthesize"};

वर्ग Ast_गन्तव्यम्_Dfg_Synthesize final {
    // NODE STATE
    // AstNodeExpr/AstVar/AstVarScope::user2p -> DfgVertex* for this Node

    // TYPES

    // SymTab must be ordered in order to yield stable results
    संरचना AstVarScopeComparator final {
        स्थिर पूर्णाङ्क s_vscp_Id_गणक;  // Counter for lazily allocating the unique AstVarScope IDs
        तार्किक संक्रियकः()(AstVarScope* lhs, AstVarScope* rhs) ध्रुव {
            यदि (!lhs->user4()) lhs->user4(++s_vscp_Id_गणक);
            यदि (!rhs->user4()) rhs->user4(++s_vscp_Id_गणक);
            प्रत्यागम lhs->user4() < rhs->user4();
        }
    };
    उपयुज्य SymTab = std::map<AstVarScope*, DfgVertexVar*, AstVarScopeComparator>;

    // Represents a [potentially partial] driver of a variable
    संरचना Driver final {
        DfgVertex* m_vtxp = शून्यसूचकः;  // Driving vertex
        uint32_t m_lo = 0;  // Low index of driven range (internal, not Verilog)
        uint32_t m_hi = 0;  // High index of driven range (internal, not Verilog)
        File_पङ्क्तिः* m_flp = शून्यसूचकः;  // Location of driver in source

        Driver() = स्वभाव;
        Driver(DfgVertex* vtxp, uint32_t lo, File_पङ्क्तिः* flp)
            : m_vtxp{vtxp}
            , m_lo{lo}
            , m_hi{lo + vtxp->आकार() - 1U}
            , m_flp{flp} {}
        संक्रियकः तार्किक() ध्रुव { प्रत्यागम m_vtxp != शून्यसूचकः; }

        तार्किक संक्रियकः<(ध्रुव Driver& other) ध्रुव {
            यदि (m_lo != other.m_lo) प्रत्यागम m_lo < other.m_lo;
            यदि (m_hi != other.m_hi) प्रत्यागम m_hi < other.m_hi;
            प्रत्यागम m_flp->operatorCompare(*other.m_flp) < 0;
        }

        तार्किक संक्रियकः<=(ध्रुव Driver& other) ध्रुव { प्रत्यागम !(other < *एतत्); }
    };

    // STATE - Persistent
    DfgGraph& m_dfg;  // The graph being built
    V3DfgSynthesisContext& m_ctx;  // The context for stats
    Ast_गन्तव्यम्_Dfg_Converter m_converter;  // The convert instance to use for each construct
    DfgWorklist m_गन्तव्यम्_Revert{m_dfg};  // We need a worklist for reverting synthesis

    // STATE - for current DfgLogic being synthesized
    DfgLogic* m_logicp = शून्यसूचकः;  // Current logic vertex we are synthesizing
    Cfg_खण्डम्_Map<SymTab> m_bb_गन्तव्यम्_ISym_Tab;  // Map from CfgBlock -> input symbol table
    Cfg_खण्डम्_Map<SymTab> m_bb_गन्तव्यम्_OSym_Tab;  // Map from CfgBlock -> output symbol table
    Cfg_खण्डम्_Map<DfgVertexVar*> m_bb_गन्तव्यम्_Condp;  // Map from CfgBlock ->  terminating branch condition
    CfgEdgeMap<DfgVertexVar*> m_edge_गन्तव्यम्_Predicatep;  // Map CfgGraphEdge -> path predicate to there
    Cfg_Dominator_वृक्षः m_dom_वृक्षः;  // The dominator tree of the current CFG

    // STATE - Some debug aid
    // We stop after synthesizing s_dfgSynthDebugLimit vertices (if non-zero).
    // This is the problematic logic (last one we synthesize), assuming a
    // bisection search over s_dfgSynthDebugLimit.
    DfgLogic* m_debugLogicp = शून्यसूचकः;
    // Source (upstream) cone of outputs of m_debugLogicp
    std::unique_ptr<std::unordered_समुच्चयः<ध्रुव DfgVertex*>> m_debugOSrcConep{शून्यसूचकः};

    // METHODS

    // Dump current graph for debugging ...
    शून्य debugDump(ध्रुव वर्ण* name) {
        // If we have the debugged logic, compute the vertices feeding its outputs
        यदि (VL_UNLIKELY(m_debugLogicp)) {
            std::सदिशम्<ध्रुव DfgVertex*> outputs;
            m_debugLogicp->foreachSink([&outputs](ध्रुव DfgVertex& v) {
                outputs.emplace_पृष्ठभागः(v.singleSink()->as<DfgVertexVar>());
                प्रत्यागम असत्य;
            });
            m_debugOSrcConep = m_dfg.sourceCone(outputs);
        }

        यदि (VL_UNLIKELY(dumpDfgLevel() >= 9 || m_debugOSrcConep)) {
            m_dfg.dumpDotFilePrefixed(name);
            यदि (m_debugOSrcConep) {
                // Dump only the subgraph involving the inputs and outputs of the bad vertex
                m_dfg.dumpDotFilePrefixed(name + "-min"s, [&](ध्रुव DfgVertex& v) -> तार्किक {
                    प्रत्यागम m_debugOSrcConep->गणनम्(&v);
                });
            }
        }
    }

    // Allocate a new non-variable vertex, add it to the currently synthesized logic
    प्रतिरूपम् <प्रकारनाम Vertex, प्रकारनाम... तर्काः_2>
    Vertex* make(तर्काः_2&&... तर्काः) {
        static_assert(!std::is_आधारः_सम्बन्धि<DfgVertexVar, Vertex>::मानम्, "Do not use for variables");
        static_assert(std::is_आधारः_सम्बन्धि<DfgVertex, Vertex>::मानम्, "'Vertex' must be a 'DfgVertex'");
        Vertex* ध्रुव vtxp = नूतन Vertex{m_dfg, std::forward<तर्काः_2>(तर्काः)...};
        यदि (m_logicp) m_logicp->synth().emplace_पृष्ठभागः(vtxp);
        प्रत्यागम vtxp;
    }

    // Gather all drivers of a resolved variable
    स्थिर std::सदिशम्<Driver> gatherDrivers(DfgVertexSplice* vtxp) {
        // Collect them all, check if they are sorted
        std::सदिशम्<Driver> drivers;
        drivers.reserve(vtxp->nInputs());
        तार्किक sorted = सत्य;
        vtxp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_पङ्क्तिः* flp) {
            // Collect the driver
            drivers.emplace_पृष्ठभागः(&src, lo, flp);
            // Check if drivers are sorted - most often they are
            ध्रुव आकार_t n = drivers.आकार();
            यदि (n >= 2 && drivers[n - 1] < drivers[n - 2]) sorted = असत्य;
            प्रत्यागम असत्य;
        });

        // Sort if unsorted
        यदि (!sorted) std::stable_क्रमय(drivers.begin(), drivers.अन्तः_2());

        //समाप्तम्
        प्रत्यागम drivers;
    }

    // Returns true if the driver supplies a complete array element.
    स्थिर तार्किक driver_Covers_सम्पूर्णम्_Element(ध्रुव Driver& driver) {
        ध्रुव DfgUnitArray* ध्रुव unitp = driver.m_vtxp->cast<DfgUnitArray>();
        यदि (!unitp) प्रत्यागम असत्य;
        यदि (DfgVertexSplice* ध्रुव splicep = unitp->srcp()->cast<DfgVertexSplice>()) {
            प्रत्यागम splicep->wholep();
        }
        प्रत्यागम सत्य;
    }

    // Unlike 'wholep', several element drivers can collectively cover an array.
    स्थिर तार्किक splice_Covers_सम्पूर्णम्(DfgVertexSplice* ध्रुव splicep) {
        यदि (splicep->wholep()) प्रत्यागम सत्य;
        यदि (splicep->isPacked()) प्रत्यागम असत्य;
        uint32_t अग्र = 0;
        प्रति (ध्रुव Driver& driver : gatherDrivers(splicep)) {
            यदि (driver.m_lo != अग्र || !driver_Covers_सम्पूर्णम्_Element(driver)) प्रत्यागम असत्य;
            अग्र = driver.m_hi + 1;
        }
        प्रत्यागम अग्र == splicep->आकार();
    }

    // Returns true if the driver cone contains any variable introduced by
    // tristate lowering. Used to distinguish intentional tristate contributor
    // overlap from accidental multidrive.
    स्थिर तार्किक अन्तर्भवति_Tri_Lowered_Var(DfgVertex* rootp) {
        std::सदिशम्<ध्रुव DfgVertex*> राशिः;
        std::सदिशम्<ध्रुव DfgVertex*> visited;
        राशिः.emplace_पृष्ठभागः(rootp);
        यावत् (!राशिः.रिक्तानि()) {
            ध्रुव DfgVertex* ध्रुव vtxp = राशिः.पृष्ठभागः();
            राशिः.निष्कासय_पृष्ठभागः();
            यदि (std::find(visited.begin(), visited.अन्तः_2(), vtxp) != visited.अन्तः_2()) अनुवर्त;
            visited.emplace_पृष्ठभागः(vtxp);
            यदि (ध्रुव DfgVertexVar* ध्रुव varp = vtxp->cast<DfgVertexVar>()) {
                यदि (varp->vscp()->varp()->dfgTriLowered()) प्रत्यागम सत्य;
            }
            vtxp->foreachSource([&](ध्रुव DfgVertex& src) {
                राशिः.emplace_पृष्ठभागः(&src);
                प्रत्यागम असत्य;
            });
        }
        प्रत्यागम असत्य;
    }

    // Returns true if the driver is a direct variable forward (no logic).
    स्थिर तार्किक is_प्रत्यक्षम्_Var_Driver(ध्रुव DfgVertex* vtxp) { प्रत्यागम vtxp->is<DfgVertexVar>(); }

    // Gather all synthesized drivers of an unresolved variable
    स्थिर std::सदिशम्<Driver> gatherDriversUnresolved(DfgUnresolved* vtxp) {
        std::सदिशम्<Driver> drivers;
        drivers.reserve(vtxp->nInputs());

        // For better locations in error reporting, we unpick concatenations
        // which are sometimes introduced by combinint assignments in V3Const.
        ध्रुव std::function<शून्य(DfgVertex*, uint32_t, File_पङ्क्तिः*)> gather
            = [&](DfgVertex* vtxp, uint32_t lo, File_पङ्क्तिः* flp) -> शून्य {
            यदि (DfgConcat* ध्रुव concatp = vtxp->cast<DfgConcat>()) {
                DfgVertex* ध्रुव rhsp = concatp->rhsp();
                gather(rhsp, lo, rhsp->fileline());
                DfgVertex* ध्रुव lhsp = concatp->lhsp();
                gather(lhsp, lo + rhsp->विस्तारः(), lhsp->fileline());
                प्रत्यागम;
            }
            drivers.emplace_पृष्ठभागः(vtxp, lo, flp);
        };

        // Gather all synthesized drivers
        vtxp->foreachSource([&](DfgVertex& src) {
            // Can ignore the original DfgLogic
            यदि (src.is<DfgLogic>()) प्रत्यागम असत्य;
            // Synthesized drivers must be a splice at this point
            DfgVertexSplice* ध्रुव splicep = src.as<DfgVertexSplice>();
            // Collect the driver
            splicep->foreachDriver([&](DfgVertex& src, uint32_t lo, File_पङ्क्तिः* flp) {
                gather(&src, lo, flp);
                प्रत्यागम असत्य;
            });
            प्रत्यागम असत्य;
        });

        // Sort the drivers
        std::stable_क्रमय(drivers.begin(), drivers.अन्तः_2());

        //समाप्तम्
        प्रत्यागम drivers;
    }

    // Given two drivers, combine the driven sub-ranges into the first one if
    // possible. First bool returned indicates successfully combined and there
    // are no multi-driven bits. Second bool returned indicates we warned
    // already about multi-driven bits.
    std::pair<तार्किक, तार्किक> combineDrivers(DfgVertexVar& var, ध्रुव std::string& sub,  //
                                         Driver& a, ध्रुव Driver& b) {
        // We can only combine array drivers ...
        यदि (a.m_vtxp->isPacked()) प्रत्यागम {असत्य, असत्य};
        // ... that both drive a single element ...
        यदि (a.m_lo != b.m_lo) प्रत्यागम {असत्य, असत्य};
        ध्रुव DfgUnitArray* ध्रुव aUap = a.m_vtxp->प्रतिरूपम् cast<DfgUnitArray>();
        यदि (!aUap) प्रत्यागम {असत्य, असत्य};
        ध्रुव DfgUnitArray* ध्रुव bUap = b.m_vtxp->प्रतिरूपम् cast<DfgUnitArray>();
        यदि (!bUap) प्रत्यागम {असत्य, असत्य};
        // ... and are themeselves partial
        DfgSplicePacked* ध्रुव aSp = aUap->srcp()->प्रतिरूपम् cast<DfgSplicePacked>();
        यदि (!aSp) प्रत्यागम {असत्य, असत्य};
        DfgSplicePacked* ध्रुव bSp = bUap->srcp()->प्रतिरूपम् cast<DfgSplicePacked>();
        यदि (!bSp) प्रत्यागम {असत्य, असत्य};
        UASSERT_OBJ(aSp->dtype() == bSp->dtype(), &var, "DTypes should match");

        // Gather drivers of a
        std::सदिशम्<Driver> aDrivers = gatherDrivers(aSp);
        // Gather drivers of b
        std::सदिशम्<Driver> bDrivers = gatherDrivers(bSp);

        // Merge them
        std::सदिशम्<Driver> abDrivers;
        abDrivers.reserve(aDrivers.आकार() + bDrivers.आकार());
        std::merge(  //
            aDrivers.begin(), aDrivers.अन्तः_2(),  //
            bDrivers.begin(), bDrivers.अन्तः_2(),  //
            std::पृष्ठभागः_inserter(abDrivers)  //
        );

        // Attempt to resolve them
        यदि (!normalizeDrivers(var, abDrivers, sub + "[" + std::गन्तव्यम्_string(a.m_lo) + "]")) {
            प्रत्यागम {असत्य, सत्य};
        }

        // Successfully resolved. Needs a new splice and unit.
        File_पङ्क्तिः* ध्रुव flp = var.fileline();
        DfgSplicePacked* ध्रुव splicep = make<DfgSplicePacked>(flp, aSp->dtype());
        प्रति (ध्रुव Driver& d : abDrivers) splicep->सङ्कलय_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        DfgUnitArray* ध्रुव uap = make<DfgUnitArray>(flp, aUap->dtype());
        uap->srcp(splicep);
        a.m_vtxp = uap;
        प्रत्यागम {सत्य, असत्य};
    }

    // Combine and coalesce the given drivers.
    // Returns true iff no multi-driven bits are present.
    तार्किक normalizeDrivers(DfgVertexVar& var, std::सदिशम्<Driver>& drivers,
                          ध्रुव std::string& sub = "") {
        यदि (drivers.रिक्तानि()) प्रत्यागम सत्य;

        // What type of values are we combining
        ध्रुव तार्किक isPacked = drivers[0].m_vtxp->isPacked();

        // Found a multidriven part ?
        तार्किक multiDriven = असत्य;

        // Iterate through the sorted drivers. Index 'i' is the driver we are
        // resolving driver 'j' agains, and if required, we merge 'j' into 'i'.
        आकार_t i = 0;
        प्रति (आकार_t j = 1; j < drivers.आकार();) {
            UASSERT_OBJ(i < j, &var, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            यदि (!jD) {
                ++j;
                अनुवर्त;
            }
            // If 'i' was moved, move 'j' in it's place
            यदि (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                अनुवर्त;
            }

            // We have 2 valid drivers now
            UASSERT_OBJ(iD <= jD, &var, "Should always be sorted");
            UASSERT_OBJ(jD.m_vtxp->isPacked() == isPacked, &var, "Mixed type drivers");

            // If no overlap, consider next pair
            यदि (iD.m_hi < jD.m_lo) {
                ++i;
                यदि (i == j) ++j;
                अनुवर्त;
            }

            // There is an overlap. Attempt to combine them.
            तार्किक combined = असत्य;
            तार्किक warned = असत्य;
            std::tie(combined, warned) = combineDrivers(var, sub, iD, jD);

            // If sucessfully combined, 'j' is no longer needed, it was combined into 'i'
            यदि (combined) {
                jD = Driver{};
                ++j;
                अनुवर्त;
            }

            // Found overlap that cannot be resolved
            multiDriven = सत्य;
            // Compare next driver
            ++j;

            // Do not warn again if we warned during 'combineDrivers'
            यदि (warned) अनुवर्त;

            // The variable to warn on
            AstVarScope* ध्रुव vscp = var.tmpForp() ? var.tmpForp() : var.vscp();

            // Loop index often abused, so suppress
            यदि (vscp->varp()->is_प्रयुक्तम्_Loop_Idx()) अनुवर्त;
            // Tristate lowering can intentionally create overlapping contributors.
            // Keep the signal marked multidriven for DFG fallback, but suppress
            // warning only when both overlapping driver cones look tri-lowered.
            यदि (vscp->varp()->dfgAllowMultidriveTri()) {
                ध्रुव तार्किक iTri = अन्तर्भवति_Tri_Lowered_Var(iD.m_vtxp);
                ध्रुव तार्किक jTri = अन्तर्भवति_Tri_Lowered_Var(jD.m_vtxp);
                ध्रुव तार्किक triPair = iTri && jTri;
                ध्रुव तार्किक triAndBridge = (iTri && is_प्रत्यक्षम्_Var_Driver(jD.m_vtxp))
                                          || (jTri && is_प्रत्यक्षम्_Var_Driver(iD.m_vtxp));
                यदि (triPair || triAndBridge) अनुवर्त;
            }

            // Warn the user now
            ध्रुव std::string lo = std::गन्तव्यम्_string(jD.m_lo);
            ध्रुव std::string hi = std::गन्तव्यम्_string(std::min(iD.m_hi, jD.m_hi));
            ध्रुव std::string विधा = isPacked ? "Bit" : "Element";
            ध्रुव std::string part = hi == lo ? (" [" + lo + "]") : ("s [" + hi + ":" + lo + "]");

            vscp->v3warn(  //
                MULTIDRIVEN,  //
                विधा << part << " of signal '" << vscp->prettyName() << sub << "'"
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

        प्रत्यागम !multiDriven;
    }

    // Coalesce adjacent drivers into single ones
    शून्य coalesceDrivers(std::सदिशम्<Driver>& drivers) {
        UASSERT(!drivers.रिक्तानि(), "Can't coalesce 0 drivers");
        UASSERT_OBJ(drivers[0].m_vtxp->isPacked(), drivers[0].m_vtxp, "Can only coalesce packed");

        आकार_t i = 0;
        प्रति (आकार_t j = 1; j < drivers.आकार();) {
            UASSERT(i < j, "Invalid iteration");
            Driver& iD = drivers[i];
            Driver& jD = drivers[j];

            // If 'j' was moved, step forward
            यदि (!jD) {
                ++j;
                अनुवर्त;
            }
            // If 'i' was moved, move 'j' in it's place
            यदि (!iD) {
                iD = jD;
                jD = Driver{};
                ++j;
                अनुवर्त;
            }

            // We have 2 valid drivers now
            UASSERT(iD <= jD, "Should always be sorted");

            // If not adjacent, move on
            यदि (iD.m_hi + 1 != jD.m_lo) {
                ++i;
                यदि (i == j) ++j;
                अनुवर्त;
            }

            // Coalesce Adjacent ranges,
            ध्रुव Dfg_दत्त_Type& dt = Dfg_दत्त_Type::packed(iD.m_vtxp->विस्तारः() + jD.m_vtxp->विस्तारः());
            DfgConcat* ध्रुव concatp = make<DfgConcat>(iD.m_flp, dt);
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
    DfgVertexSplice* makeSplice(DfgVertexVar& var, ध्रुव std::सदिशम्<Driver>& newDrivers) {
        UASSERT_OBJ(!newDrivers.रिक्तानि(), &var, "'makeSplice' called with no new drivers");
        // Create new driver
        DfgVertexSplice* splicep = शून्यसूचकः;
        यदि (var.is<DfgVarPacked>()) {
            splicep = make<DfgSplicePacked>(var.fileline(), var.dtype());
        } अन्यथा यदि (var.is<DfgVarArray>()) {
            splicep = make<DfgSpliceArray>(var.fileline(), var.dtype());
        } अन्यथा {
            var.v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        प्रति (ध्रुव Driver& d : newDrivers) splicep->सङ्कलय_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        प्रत्यागम splicep;
    }

    // Initialzie input symbol table of entry CfgBlock
    शून्य initialize_प्रविष्टिः_चिह्नम्_सारणी(SymTab& iSymTab) {
        m_logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* ध्रुव vvp = src.as<DfgVertexVar>();
            iSymTab[vvp->vscp()] = vvp;
            प्रत्यागम असत्य;
        });
    }

    // Join variable drivers across a control flow confluence (insert muxes ...)
    DfgVertexVar* joinDrivers(AstVarScope* vscp, DfgVertexVar* predicatep,  //
                              DfgVertexVar* thenp, DfgVertexVar* elsep) {
        AstVarScope* ध्रुव thenVscp = thenp->tmpForp() ? thenp->tmpForp() : thenp->vscp();
        AstVarScope* ध्रुव elseVscp = elsep->tmpForp() ? elsep->tmpForp() : elsep->vscp();
        UASSERT_OBJ(thenVscp == elseVscp, vscp, "Attempting to join unrelated variables");

        // If both bindings are the the same (variable not updated through either path),
        // then there is nothing to do, can use the same binding
        यदि (thenp == elsep) प्रत्यागम thenp;

        // We can't join the input variable just yet, so bail
        यदि (thenp->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_निवेशः;
            प्रत्यागम शून्यसूचकः;
        }
        यदि (elsep->vscp() == vscp) {
            ++m_ctx.m_synt.non_Syn_Join_निवेशः;
            प्रत्यागम शून्यसूचकः;
        }

        // Can't do arrays yet
        यदि (!thenp->isPacked()) {
            ++m_ctx.m_synt.nonSynArray;
            प्रत्यागम शून्यसूचकः;
        }

        // Gather drivers of 'thenp' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* ध्रुव tDefaultp = thenp->defaultp();
        std::सदिशम्<Driver> tDrivers = gatherDrivers(thenp->srcp()->as<DfgVertexSplice>());

        // Gather drivers of 'elsep' - only if 'thenp' is not an input to the synthesized block
        DfgVertex* ध्रुव eDefaultp = elsep->defaultp();
        std::सदिशम्<Driver> eDrivers = gatherDrivers(elsep->srcp()->as<DfgVertexSplice>());

        // Default drivers should be the same or not present on either
        UASSERT_OBJ(tDefaultp == eDefaultp, vscp, "Different default drivers");

        // Location to use for the join vertices
        File_पङ्क्तिः* ध्रुव flp = predicatep->fileline();

        // Create a fresh temporary for the joined value
        DfgVertexVar* ध्रुव joinp = m_converter.createTmp(*m_logicp, vscp, "SynthJoin");
        DfgVertexSplice* ध्रुव joinSplicep = make<DfgSplicePacked>(flp, joinp->dtype());
        joinp->srcp(joinSplicep);

        // If both paths are fully driven, just create a simple conditional
        यदि (tDrivers.आकार() == 1  //
            && tDrivers[0].m_lo == 0  //
            && tDrivers[0].m_hi == thenp->विस्तारः() - 1  //
            && eDrivers.आकार() == 1  //
            && eDrivers[0].m_lo == 0  //
            && eDrivers[0].m_hi == elsep->विस्तारः() - 1) {
            UASSERT_OBJ(!tDefaultp, vscp, "Fully driven variable have default driver");

            DfgCond* ध्रुव condp = make<DfgCond>(flp, joinp->dtype());
            condp->condp(predicatep);
            condp->thenp(thenp);
            condp->elsep(elsep);
            joinSplicep->सङ्कलय_Driver(condp, 0, tDrivers[0].m_flp);

            //समाप्तम्
            प्रत्यागम joinp;
        }

        // Otherwise we need to merge them part by part

        // If different bits are driven, then some might not have been assigned.. Latch?
        यदि (tDrivers.आकार() != eDrivers.आकार()) {
            ++m_ctx.m_synt.nonSynLatch;
            प्रत्यागम शून्यसूचकः;
        }

        प्रति (आकार_t i = 0; i < tDrivers.आकार(); ++i) {
            ध्रुव Driver& tDriver = tDrivers[i];
            ध्रुव Driver& eDriver = eDrivers[i];
            // If different bits are driven, then some might not have been assigned.. Latch?
            यदि (tDriver.m_lo != eDriver.m_lo || tDriver.m_hi != eDriver.m_hi) {
                ++m_ctx.m_synt.nonSynLatch;
                प्रत्यागम शून्यसूचकः;
            }

            ध्रुव Dfg_दत्त_Type& dtype = Dfg_दत्त_Type::packed(tDriver.m_hi - tDriver.m_lo + 1);
            DfgCond* ध्रुव condp = make<DfgCond>(flp, dtype);
            condp->condp(predicatep);

            // We actally need to select the bits from the joined variables, not use the drivers
            DfgSel* ध्रुव thenSelp = make<DfgSel>(flp, tDriver.m_vtxp->dtype());
            thenSelp->lsb(tDriver.m_lo);
            thenSelp->fromp(thenp);
            condp->thenp(thenSelp);

            // Same for the 'else' part
            DfgSel* ध्रुव elseSelp = make<DfgSel>(flp, eDriver.m_vtxp->dtype());
            elseSelp->lsb(eDriver.m_lo);
            elseSelp->fromp(elsep);
            condp->elsep(elseSelp);

            // Add it as a driver to the join
            joinSplicep->सङ्कलय_Driver(condp, tDriver.m_lo, tDriver.m_flp);
        }

        // If there was a default driver, add it to te join
        यदि (tDefaultp) joinp->defaultp(tDefaultp);

        //समाप्तम्
        प्रत्यागम joinp;
    }

    // Merge 'thenSymTab' into 'elseSymTab' using the given predicate to join values
    तार्किक join_चिह्नम्_Tables(SymTab& elseSymTab, DfgVertexVar* predicatep, ध्रुव SymTab& thenSymTab) {
        // Any variable that does not have a binding on both paths will be removed. These might be
        // temporaries, loop vars, etc used only in one branch. Conversion will fail if the
        // variable is actually referenced later.
        std::सदिशम्<AstVarScope*> गन्तव्यम्_Remove;

        // Join each symbol
        प्रति (std::pair<AstVarScope* ध्रुव, DfgVertexVar*>& pair : elseSymTab) {
            AstVarScope* ध्रुव varp = pair.first;
            // Find same variable on the else path
            ध्रुव स्वचालित it = thenSymTab.find(varp);
            // Record for removal if not assigned on both paths
            यदि (it == thenSymTab.अन्तः_2()) {
                गन्तव्यम्_Remove.emplace_पृष्ठभागः(varp);
                अनुवर्त;
            }
            // Join paths with the predicate
            DfgVertexVar* ध्रुव thenp = it->second;
            DfgVertexVar* ध्रुव elsep = pair.second;
            DfgVertexVar* ध्रुव newp = joinDrivers(varp, predicatep, thenp, elsep);
            यदि (!newp) प्रत्यागम असत्य;
            pair.second = newp;
        }

        // Remove variables not assigned on both paths
        प्रति (AstVarScope* ध्रुव varp : गन्तव्यम्_Remove) elseSymTab.erase(varp);

        //समाप्तम्
        प्रत्यागम सत्य;
    }

    // Given two joining control flow edges, compute how to join their symbols.
    // Returns the predicaete to join over, and the 'then' and 'else' blocks.
    std::tuple<DfgVertexVar*, ध्रुव Cfg_खण्डम्*, ध्रुव Cfg_खण्डम्*>  //
    how_गन्तव्यम्_Join(ध्रुव CfgEdge* ध्रुव ap, ध्रुव CfgEdge* ध्रुव bp) {
        // Find the closest common dominator of the two paths
        ध्रुव Cfg_खण्डम्* ध्रुव domp = m_dom_वृक्षः.closestCommonDominator(ap->srcp(), bp->srcp());
        // These paths join here, so 'domp' must be a branch, otherwise it's not the closest
        UASSERT_OBJ(domp->isBranch(), domp, "closestCommonDominator is not a branch");

        // The branches of the common dominator
        ध्रुव CfgEdge* ध्रुव गृहीतम्_Edgep = domp->गृहीतम्_Edgep();
        ध्रुव CfgEdge* ध्रुव untknEdgep = domp->untknEdgep();

        // We check if the taken branch dominates the path to either blocks,
        // and if the untaken branch dominates the path to the other block.
        // If so, we can use the branch condition as predicate, otherwise
        // we must use the path predicate as there are ways to get from one
        // branch of the dominator to the other. We need to be careful if
        // either branches are directly to the join block. This is fine,
        // it's as if there was an empty block on that critical edge which
        // is dominated by that path.

        यदि (गृहीतम्_Edgep == ap || m_dom_वृक्षः.dominates(गृहीतम्_Edgep->dstp(), ap->srcp())) {
            यदि (untknEdgep == bp || m_dom_वृक्षः.dominates(untknEdgep->dstp(), bp->srcp())) {
                // Taken path dominates 'ap' and untaken dominates 'bp', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                प्रत्यागम std::make_tuple(m_bb_गन्तव्यम्_Condp[domp], ap->srcp(), bp->srcp());
            }
        } अन्यथा यदि (गृहीतम्_Edgep == bp || m_dom_वृक्षः.dominates(गृहीतम्_Edgep->dstp(), bp->srcp())) {
            यदि (untknEdgep == ap || m_dom_वृक्षः.dominates(untknEdgep->dstp(), ap->srcp())) {
                // Taken path dominates 'bp' and untaken dominates 'ap', use the branch condition
                ++m_ctx.m_synt.joinUsingBranchCondition;
                प्रत्यागम std::make_tuple(m_bb_गन्तव्यम्_Condp[domp], bp->srcp(), ap->srcp());
            }
        }

        // The branches don't dominate the joined blocks, must use the path predicate
        ++m_ctx.m_synt.join_Using_मार्गः_Predicate;

        // TODO: We could do better here: use the path predicate of the closest
        // cominating blocks, pick the one from the lower rank, etc, but this
        // generic case is very rare, most synthesizable logic has
        // series-parallel CFGs which are covered by the earlier cases.
        प्रत्यागम std::make_tuple(m_edge_गन्तव्यम्_Predicatep[ap], ap->srcp(), bp->srcp());
    }

    // Combine the output symbol tables of the predecessors of the given
    // block to compute the input symtol table for the given block.
    तार्किक create_निवेशः_चिह्नम्_सारणी(ध्रुव Cfg_खण्डम्& bb) {
        // The input symbol table of the given block, we are computing it now
        SymTab& joined = m_bb_गन्तव्यम्_ISym_Tab[bb];

        // Input symbol table of entry block is special
        यदि (bb.isEnter()) {
            initialize_प्रविष्टिः_चिह्नम्_सारणी(joined);
            प्रत्यागम सत्य;
        }

        // Current input symbol table should be empty, we will fill it in here
        UASSERT(joined.रिक्तानि(), "Unprocessed input symbol table should be empty");

        // Fast path if there is only one predecessor - TODO: use less copying
        यदि (!bb.isJoin()) {
            joined = m_bb_गन्तव्यम्_OSym_Tab[bb.firstPredecessorp()];
            प्रत्यागम सत्य;
        }

        // We also have a simpler job if there are 2 predecessors
        यदि (bb.isTwoWayJoin()) {
            DfgVertexVar* predicatep = शून्यसूचकः;
            ध्रुव Cfg_खण्डम्* thenp = शून्यसूचकः;
            ध्रुव Cfg_खण्डम्* elsep = शून्यसूचकः;
            std::tie(predicatep, thenp, elsep)
                = how_गन्तव्यम्_Join(bb.firstPredecessorEdgep(), bb.चरमः_Predecessor_Edgep());
            // Copy from else
            joined = m_bb_गन्तव्यम्_OSym_Tab[elsep];
            // Join with then
            प्रत्यागम join_चिह्नम्_Tables(joined, predicatep, m_bb_गन्तव्यम्_OSym_Tab[*thenp]);
        }

        // General hard way

        // Gather predecessors
        संरचना Predecessor final {
            ध्रुव Cfg_खण्डम्* m_bbp;  // Predeccessor block
            DfgVertexVar* m_predicatep;  // Predicate predecessor reached this block with
            ध्रुव SymTab* m_oSymTabp;  // Output symbol table or predecessor
            Predecessor() = विलोपय;
            Predecessor(ध्रुव Cfg_खण्डम्* bbp, DfgVertexVar* predicatep, ध्रुव SymTab* oSymTabp)
                : m_bbp{bbp}
                , m_predicatep{predicatep}
                , m_oSymTabp{oSymTabp} {}
        };

        ध्रुव std::सदिशम्<Predecessor> predecessors = [&]() {
            std::सदिशम्<Predecessor> res;
            प्रति (ध्रुव V3GraphEdge& edge : bb.अन्तर्_धाराः()) {
                ध्रुव CfgEdge& cfgEdge = static_cast<ध्रुव CfgEdge&>(edge);
                ध्रुव Cfg_खण्डम्* ध्रुव predecessorp = cfgEdge.srcp();
                DfgVertexVar* ध्रुव predicatep = m_edge_गन्तव्यम्_Predicatep[cfgEdge];
                ध्रुव SymTab* ध्रुव oSymTabp = &m_bb_गन्तव्यम्_OSym_Tab[predecessorp];
                res.emplace_पृष्ठभागः(predecessorp, predicatep, oSymTabp);
            }
            // Sort predecessors reverse topologically. This way earlier blocks
            // will come after later blocks, and the entry block is last if present.
            std::क्रमय(res.begin(), res.अन्तः_2(), [](ध्रुव Predecessor& a, ध्रुव Predecessor& b) {  //
                प्रत्यागम *a.m_bbp > *b.m_bbp;
            });
            प्रत्यागम res;
        }();

        // Start by copying the bindings from the frist predecessor
        joined = *predecessors[0].m_oSymTabp;
        // Join over all other predecessors
        प्रति (आकार_t i = 1; i < predecessors.आकार(); ++i) {
            DfgVertexVar* ध्रुव predicatep = predecessors[i].m_predicatep;
            ध्रुव SymTab& oSymTab = *predecessors[i].m_oSymTabp;
            यदि (!join_चिह्नम्_Tables(joined, predicatep, oSymTab)) प्रत्यागम असत्य;
        }

        प्रत्यागम सत्य;
    }

    std::सदिशम्<Driver> computePropagatedDrivers(ध्रुव std::सदिशम्<Driver>& newDrivers,
                                                 DfgVertexVar* oldp) {
        // Gather drivers of 'oldp' - they are in incresing range order with no overlaps
        ध्रुव std::सदिशम्<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.रिक्तानि(), oldp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::सदिशम्<Driver> propagatedDrivers;

        // Add bits between 'msb' and 'lsb' from 'oldp' to 'pDrivers'
        ध्रुव स्वचालित सङ्कलय_Old_Driver = [&](File_पङ्क्तिः* ध्रुव flp, uint32_t msb, uint32_t lsb) {
            UASSERT_OBJ(propagatedDrivers.रिक्तानि() || lsb > propagatedDrivers.पृष्ठभागः().m_hi, flp,
                        "Drivers should be in ascending order");
            DfgSel* ध्रुव selp = make<DfgSel>(flp, Dfg_दत्त_Type::packed(msb - lsb + 1));
            selp->lsb(lsb);
            selp->fromp(oldp);
            propagatedDrivers.emplace_पृष्ठभागः(selp, lsb, flp);
        };

        // Incorporate old drivers
        प्रति (ध्रुव Driver& oDriver : oldDrivers) {
            File_पङ्क्तिः* ध्रुव flp = oDriver.m_flp;
            // Range to consider inserting, we will adjust oldLo as we process drivers
            uint32_t oldLo = oDriver.m_lo;
            ध्रुव uint32_t oldHi = oDriver.m_hi;

            // Loop for now, can move to bisection search if this is a problem, shouldn't be ...
            प्रति (ध्रुव Driver& nDriver : newDrivers) {
                UASSERT_OBJ(oldHi >= oldLo, flp, "Should have stopped iteration");
                // If new driver is entirely below old driver, move on to
                यदि (nDriver.m_hi < oldLo) अनुवर्त;
                // If new driver is entirely above old driver, we can stop
                यदि (oldHi < nDriver.m_lo) भङ्ग;

                // There is an overlap between 'oDriver' and 'nDriver'.
                // Insert the low bits and adjust the insertion range.
                // The rest will take care of itself on subsequent iterations.
                यदि (oldLo < nDriver.m_lo) सङ्कलय_Old_Driver(flp, nDriver.m_lo - 1, oldLo);
                oldLo = nDriver.m_hi + 1;

                // Stop if no more bits remaining in the old driver
                यदि (oldLo > oldHi) भङ्ग;
            }

            // Insert remaining bits if any
            यदि (oldHi >= oldLo) सङ्कलय_Old_Driver(flp, oldHi, oldLo);
        }

        प्रत्यागम propagatedDrivers;
    }

    // Propagate whole elements with a linear walk over the sorted driver lists.
    // Reads use the previous value temporary, preserving assignment-version bindings.
    // Partial elements and non-unit drivers retain the nonsynthesized-process fallback.
    तार्किक computePropagatedArrayDrivers(ध्रुव std::सदिशम्<Driver>& newDrivers,
                                       DfgVertexVar* ध्रुव oldp,
                                       std::सदिशम्<Driver>& propagatedDrivers) {
        // Bound quadratic vertex growth: array drivers cannot be coalesced.
        स्थिर constexpr uint32_t MAX_ARRAY_ELEMENTS = 32;
        यदि (oldp->आकार() > MAX_ARRAY_ELEMENTS) प्रत्यागम असत्य;
        प्रति (ध्रुव Driver& driver : newDrivers) {
            यदि (!driver_Covers_सम्पूर्णम्_Element(driver)) प्रत्यागम असत्य;
        }
        ध्रुव std::सदिशम्<Driver> oldDrivers = gatherDrivers(oldp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!oldDrivers.रिक्तानि(), oldp, "Should have a proper driver");
        प्रति (ध्रुव Driver& driver : oldDrivers) {
            यदि (!driver_Covers_सम्पूर्णम्_Element(driver)) प्रत्यागम असत्य;
        }

        propagatedDrivers.reserve(oldDrivers.आकार());
        स्वचालित nIt = newDrivers.begin();
        प्रति (ध्रुव Driver& oDriver : oldDrivers) {
            यावत् (nIt != newDrivers.अन्तः_2() && nIt->m_lo < oDriver.m_lo) ++nIt;
            यदि (nIt != newDrivers.अन्तः_2() && nIt->m_lo == oDriver.m_lo) अनुवर्त;

            File_पङ्क्तिः* ध्रुव flp = oDriver.m_flp;
            ध्रुव DfgUnitArray* ध्रुव oldUnitp = oDriver.m_vtxp->as<DfgUnitArray>();
            DfgArraySel* ध्रुव selp = make<DfgArraySel>(flp, oldUnitp->srcp()->dtype());
            selp->fromp(oldp);
            selp->bitp(make<DfgConst>(flp, static_cast<आकार_t>(VL_IDATASIZE), oDriver.m_lo));
            DfgUnitArray* ध्रुव newUnitp = make<DfgUnitArray>(flp, oldUnitp->dtype());
            newUnitp->srcp(selp);
            propagatedDrivers.emplace_पृष्ठभागः(newUnitp, oDriver.m_lo, flp);
        }
        प्रत्यागम सत्य;
    }

    // Given the drivers of a variable after converting a single statement
    // 'newp', add drivers from 'oldp' that were not reassigned be drivers
    // in newp. This computes the total result of all previous assignments.
    तार्किक incorporate_Previous_मानम्(AstVarScope* vscp, DfgVertexVar* newp, DfgVertexVar* oldp) {
        UASSERT_OBJ(newp->srcp(), vscp, "Assigned variable has no driver");

        // Easy if there is no old value...
        यदि (!oldp) प्रत्यागम सत्य;

        // New driver was not yet coalesced, so should always be a splice
        DfgVertexSplice* ध्रुव nSplicep = newp->srcp()->as<DfgVertexSplice>();

        // If the old value is the real variable we just computed the new value for,
        // then it is the circular feedback into the synthesized block, add it as default driver.
        यदि (oldp->vscp() == vscp) {
            यदि (!splice_Covers_सम्पूर्णम्(nSplicep)) newp->defaultp(oldp);
            प्रत्यागम सत्य;
        }

        UASSERT_OBJ(oldp->srcp(), vscp, "Previously assigned variable has no driver");

        // Gather drivers of 'newp' - they are in incresing range order with no overlaps
        UASSERT_OBJ(!newp->defaultp(), newp, "Converted value should not have default");
        std::सदिशम्<Driver> nDrivers = gatherDrivers(newp->srcp()->as<DfgVertexSplice>());
        UASSERT_OBJ(!nDrivers.रिक्तानि(), newp, "Should have a proper driver");

        // Additional drivers of 'newp' propagated from 'oldp'
        std::सदिशम्<Driver> pDrivers;
        यदि (newp->isPacked()) {
            pDrivers = computePropagatedDrivers(nDrivers, oldp);
        } अन्यथा यदि (!computePropagatedArrayDrivers(nDrivers, oldp, pDrivers)) {
            ++m_ctx.m_synt.nonSynArray;
            प्रत्यागम असत्य;
        }

        यदि (!pDrivers.रिक्तानि()) {
            // Need to merge propagated sources, so reset the splice
            nSplicep->resetDrivers();
            // Merge drivers - they are both sorted and non-overlapping
            std::सदिशम्<Driver> drivers;
            drivers.reserve(nDrivers.आकार() + pDrivers.आकार());
            std::merge(nDrivers.begin(), nDrivers.अन्तः_2(), pDrivers.begin(), pDrivers.अन्तः_2(),
                       std::पृष्ठभागः_inserter(drivers));
            // Coalesce adjacent ranges
            यदि (newp->isPacked()) coalesceDrivers(drivers);
            // Reinsert drivers in order
            प्रति (ध्रुव Driver& d : drivers) nSplicep->सङ्कलय_Driver(d.m_vtxp, d.m_lo, d.m_flp);
        }

        // If the old had a default, add to the new one too, unless redundant
        यदि (oldp->defaultp() && !splice_Covers_सम्पूर्णम्(nSplicep)) newp->defaultp(oldp->defaultp());

        //समाप्तम्
        प्रत्यागम सत्य;
    }

    // Synthesize the given statements with the given input symbol table.
    // Returns true if successfolly synthesized.
    // Populates the given output symbol table.
    // Populates the given reference with the condition of the terminator branch, if any.
    तार्किक synthesize_Basic_खण्डम्(SymTab& oSymTab, DfgVertex*& condpr,
                              ध्रुव std::सदिशम्<Ast_ग्रन्थिः_Stmt*>& stmtps, ध्रुव SymTab& iSymTab) {
        // Use fresh set of vertices in m_converter
        ध्रुव VNUser2_अन्तर्_Use user2_अन्तर्_Use;

        // Initialize AstVarScope -> Vertex bindings available in this block
        प्रति (ध्रुव स्वचालित& pair : iSymTab) {
            AstVarScope* ध्रुव varp = pair.first;
            DfgVertexVar* ध्रुव vtxp = pair.second;
            varp->user2p(vtxp);
            oSymTab[varp] = vtxp;
        }

        // Synthesize each statement one after the other
        std::सदिशम्<std::pair<AstVarScope*, DfgVertexVar*>> updates;
        प्रति (Ast_ग्रन्थिः_Stmt* ध्रुव stmtp : stmtps) {
            // Regular statements
            Ast_ग्रन्थिः_नियोजय* ध्रुव ap = VN_CAST(stmtp, ग्रन्थिः_नियोजय);
            यदि (ap && (VN_IS(ap, नियोजय) || VN_IS(ap, नियोजय_W))) {
                // Convert this assignment
                यदि (!m_converter.convert(updates, *m_logicp, ap)) {
                    ++m_ctx.m_synt.nonSynConv;
                    प्रत्यागम असत्य;
                }
                // Apply variable updates from this statement
                प्रति (ध्रुव स्वचालित& pair : updates) {
                    // The target variable that was assigned to
                    AstVarScope* ध्रुव vscp = pair.first;
                    // The new, potentially partially assigned value
                    DfgVertexVar* ध्रुव newp = pair.second;
                    // Normalize drivers within this statement, bail if multidriven
                    DfgVertexSplice* ध्रुव srcp = newp->srcp()->as<DfgVertexSplice>();
                    std::सदिशम्<Driver> drivers = gatherDrivers(srcp);
                    ध्रुव तार्किक single = drivers.आकार() == 1;
                    यदि (!normalizeDrivers(*newp, drivers)) {
                        ++m_ctx.m_synt.nonSynMultidrive;
                        प्रत्यागम असत्य;
                    }
                    // If there were more than one driver (often not), re-add in case coalesced
                    यदि (!single) {
                        srcp->resetDrivers();
                        प्रति (ध्रुव Driver& d : drivers) srcp->सङ्कलय_Driver(d.m_vtxp, d.m_lo, d.m_flp);
                    }
                    // The old value, if any
                    DfgVertexVar* ध्रुव oldp = vscp->user2u().प्रतिरूपम् गन्तव्यम्<DfgVertexVar*>();
                    // Inncorporate old value into the new value
                    यदि (!incorporate_Previous_मानम्(vscp, newp, oldp)) प्रत्यागम असत्य;
                    // Update binding of target variable
                    vscp->user2p(newp);
                    // Update output symbol table of this block
                    oSymTab[vscp] = newp;
                }
                updates.clear();
                अनुवर्त;
            }

            // Terminator branches
            यदि (AstIf* ध्रुव ifp = VN_CAST(stmtp, If)) {
                UASSERT_OBJ(ifp == stmtps.पृष्ठभागः(), ifp, "Branch should be last statement");
                // Convert condition, give up if failed
                DfgVertex* condp = m_converter.convert(*m_logicp, ifp->condp());
                यदि (!condp) {
                    ++m_ctx.m_synt.nonSynConv;
                    प्रत्यागम असत्य;
                }
                // Single bit condition can be use directly, otherwise: use 'condp != 0'
                यदि (condp->विस्तारः() != 1) {
                    File_पङ्क्तिः* ध्रुव flp = condp->fileline();
                    DfgNeq* ध्रुव neqp = make<DfgNeq>(flp, Dfg_दत्त_Type::packed(1));
                    neqp->lhsp(make<DfgConst>(flp, condp->विस्तारः(), 0U));
                    neqp->rhsp(condp);
                    condp = neqp;
                }
                condpr = condp;
                अनुवर्त;
            }

            // Unhandled
            ++m_ctx.m_synt.nonSynStmt;
            प्रत्यागम असत्य;
        }

        प्रत्यागम सत्य;
    }

    // Assign path predicates to the outgoing control flow edges of the given block
    शून्य नियोजय_मार्गः_Predicates(ध्रुव Cfg_खण्डम्& bb) {
        // Nothing to do for the exit block
        यदि (bb.isExit()) प्रत्यागम;

        // Get the predicate of this block
        DfgVertex* ध्रुव predp = [&]() -> DfgVertex* {
            // Entry block has no predecessors, use constant true
            यदि (bb.isEnter()) प्रत्यागम make<DfgConst>(m_logicp->fileline(), 1U, 1U);

            // For any other block, 'or' together all the incoming predicates
            ध्रुव स्वचालित& अन्तर्_धाराः = bb.अन्तर्_धाराः();
            स्वचालित it = अन्तर्_धाराः.begin();
            DfgVertex* resp = m_edge_गन्तव्यम्_Predicatep[static_cast<ध्रुव CfgEdge&>(*it)];
            यावत् (++it != अन्तर्_धाराः.अन्तः_2()) {
                DfgOr* ध्रुव orp = make<DfgOr>(resp->fileline(), resp->dtype());
                orp->rhsp(resp);
                orp->lhsp(m_edge_गन्तव्यम्_Predicatep[static_cast<ध्रुव CfgEdge&>(*it)]);
                resp = orp;
            }
            प्रत्यागम resp;
        }();

        ध्रुव Dfg_दत्त_Type& dtype = predp->dtype();

        ध्रुव स्वचालित mkTmp = [&](File_पङ्क्तिः* flp, ध्रुव वर्ण* name, DfgVertex* srcp) {
            ध्रुव std::string उपसर्गः = "_BB" + std::गन्तव्यम्_string(bb.id()) + "_" + name;
            DfgVertexVar* ध्रुव tmpp = m_converter.createTmp(*m_logicp, flp, dtype, उपसर्गः);
            tmpp->srcp(srcp);
            प्रत्यागम tmpp;
        };

        // Assign it to a variable in case it's used multiple times
        DfgVertexVar* ध्रुव pInp = mkTmp(predp->fileline(), "PathIn", predp);

        // For uncondional branches, the successor predicate edge is the same
        यदि (!bb.isBranch()) {
            m_edge_गन्तव्यम्_Predicatep[bb.गृहीतम्_Edgep()] = mkTmp(pInp->fileline(), "Goto", pInp);
            प्रत्यागम;
        }

        // For branches, we need to factor in the branch condition
        DfgVertex* ध्रुव condp = m_bb_गन्तव्यम्_Condp[bb];
        File_पङ्क्तिः* ध्रुव flp = condp->fileline();

        // Predicate for taken branch: 'predp & condp'
        {
            DfgAnd* ध्रुव गृहीतम्_Predp = make<DfgAnd>(flp, dtype);
            गृहीतम्_Predp->lhsp(pInp);
            गृहीतम्_Predp->rhsp(condp);
            m_edge_गन्तव्यम्_Predicatep[bb.गृहीतम्_Edgep()] = mkTmp(flp, "Taken", गृहीतम्_Predp);
        }

        // Predicate for untaken branch: 'predp & ~condp'
        {
            DfgAnd* ध्रुव untknPredp = make<DfgAnd>(flp, dtype);
            untknPredp->lhsp(pInp);
            DfgNot* ध्रुव notp = make<DfgNot>(flp, dtype);
            notp->srcp(condp);
            untknPredp->rhsp(notp);
            m_edge_गन्तव्यम्_Predicatep[bb.untknEdgep()] = mkTmp(flp, "Untkn", untknPredp);
        }
    }

    // Returns true if all external updates to volatile variables are observed correctly
    तार्किक checkExtWrites() {
        प्रति (ध्रुव DfgVertex* ध्रुव vtxp : m_logicp->synth()) {
            ध्रुव DfgVertexVar* ध्रुव varp = vtxp->cast<DfgVertexVar>();
            यदि (!varp) अनुवर्त;
            // If the variable we synthesized this vertex for is volatile, and
            // the value of the synthesized temporary is observed, we might be
            // missing an external update, so we mut give up.
            यदि (!varp->hasSinks()) अनुवर्त;
            यदि (!DfgVertexVar::isVolatile(varp->tmpForp())) अनुवर्त;
            ++m_ctx.m_synt.nonSynExtWrite;
            प्रत्यागम असत्य;
        }
        प्रत्यागम सत्य;
    }

    // Add the synthesized values as drivers to the output variables of the current DfgLogic
    तार्किक सङ्कलय_Synthesized_Output(SymTab& oSymTab) {
        // It's possible we think a variable is written by the DfgLogic when
        // it actauly isn't, e.g.: '{a[0], b[0]}[1] = ...' does not write 'b'.
        // These LHS forms can happen after some earlier tranforms. We
        // should just run V3Const on them earlier, but we will do belt and
        // braces and check here too. We can't touch any output variables if so.
        ध्रुव तार्किक missing = m_logicp->foreachSink([&](ध्रुव DfgVertex& sink) {
            ध्रुव DfgUnresolved* ध्रुव unresolvedp = sink.as<DfgUnresolved>();
            AstVarScope* ध्रुव vscp = unresolvedp->singleSink()->as<DfgVertexVar>()->vscp();
            प्रत्यागम !oSymTab.गणनम्(vscp);
        });
        यदि (missing) {
            ++m_ctx.m_synt.nonSynFalseWrite;
            प्रत्यागम असत्य;
        }

        // Add sinks to read the computed values for the target variables
        प्रत्यागम !m_logicp->foreachSink([&](DfgVertex& sink) {
            DfgUnresolved* ध्रुव unresolvedp = sink.as<DfgUnresolved>();
            ध्रुव DfgVertexVar* ध्रुव varp = unresolvedp->singleSink()->as<DfgVertexVar>();
            DfgVertexVar* ध्रुव resp = oSymTab.at(varp->vscp());
            UASSERT_OBJ(resp->srcp(), resp, "Undriven result");

            // If the output is not used further in the synthesized logic itself,
            // then resp will be deleted before we return, so we can just use
            // its splice directly without ending up with a multi-use operation.
            यदि (!resp->hasSinks()) {
                unresolvedp->सङ्कलय_Driver(resp->srcp()->as<DfgVertexSplice>());
                प्रत्यागम असत्य;  // OK, continue.
            }

            // TODO: computePropagatedDrivers cannot handle arrays, should
            // never happen with simple continous assignments
            यदि (!resp->isPacked()) {
                ++m_ctx.m_synt.nonSynArray;
                प्रत्यागम सत्य;  // Not OK, give up
            }

            // We need to add a new splice to avoid multi-use of the original splice
            DfgSplicePacked* ध्रुव splicep
                = नूतन DfgSplicePacked{m_dfg, resp->fileline(), resp->dtype()};
            // Drivers are the same
            ध्रुव std::सदिशम्<Driver> drivers = computePropagatedDrivers({}, resp);
            प्रति (ध्रुव Driver& d : drivers) splicep->सङ्कलय_Driver(d.m_vtxp, d.m_lo, d.m_flp);
            unresolvedp->सङ्कलय_Driver(splicep);
            प्रत्यागम असत्य;  // OK, continue
        });
    }

    // Synthesize the given AstAssignW. Returns true on success.
    तार्किक synthesize_नियोजय_W(Ast_नियोजय_W* nodep) {
        ++m_ctx.m_synt.निवेशः_नियोजय;

        // The input and output symbol tables
        SymTab iSymTab;
        SymTab oSymTab;

        // Initialzie input symbol table
        initialize_प्रविष्टिः_चिह्नम्_सारणी(iSymTab);

        // Synthesize as if it was in a single CfgBlock CFG
        DfgVertex* condp = शून्यसूचकः;
        ध्रुव तार्किक success = synthesize_Basic_खण्डम्(oSymTab, condp, {nodep}, iSymTab);
        UASSERT_OBJ(!condp, nodep, "Conditional AstAssignW ???");
        यदि (!success) प्रत्यागम असत्य;

        // Check exernal writes are observed correctly
        यदि (!checkExtWrites()) प्रत्यागम असत्य;

        // Add resolved output variable drivers
        प्रत्यागम सङ्कलय_Synthesized_Output(oSymTab);
    }

    // Synthesize the given AstAlways. Returns true on success.
    तार्किक synthesizeCfg(CfgGraph& cfg) {
        ++m_ctx.m_synt.निवेशः_Always;

        // If there is a backward edge (loop), we can't synthesize it
        यदि (cfg.अन्तर्भवति_Loop()) {
            ++m_ctx.m_synt.nonSynLoop;
            ++m_ctx.m_synt.cfgCyclic;
            प्रत्यागम असत्य;
        }

        // If it's a trivial CFG we can save on some work
        यदि (cfg.n_खण्डानि() == 1) {
            ++m_ctx.m_synt.cfgTrivial;
        } अन्यथा {
            // Insert two-way join blocks to aid multiplexer ordering
            यदि (cfg.निवेशय_Two_Way_Joins()) {
                ++m_ctx.m_synt.cfgSp;
            } अन्यथा {
                ++m_ctx.m_synt.cfgDag;
            }
            // Initialize maps needed for non-trivial CFGs
            m_dom_वृक्षः = Cfg_Dominator_वृक्षः{cfg};
            m_edge_गन्तव्यम्_Predicatep = cfg.makeEdgeMap<DfgVertexVar*>();
        }

        // Initialize CfgMaps
        m_bb_गन्तव्यम्_ISym_Tab = cfg.make_खण्डम्_Map<SymTab>();
        m_bb_गन्तव्यम्_OSym_Tab = cfg.make_खण्डम्_Map<SymTab>();
        m_bb_गन्तव्यम्_Condp = cfg.make_खण्डम्_Map<DfgVertexVar*>();

        // Synthesize all blocks
        प्रति (ध्रुव V3GraphVertex& vtx : cfg.vertices()) {
            ध्रुव Cfg_खण्डम्& bb = static_cast<ध्रुव Cfg_खण्डम्&>(vtx);
            // Prepare the input symbol table of this block (enter, or join predecessor blocks)
            यदि (!create_निवेशः_चिह्नम्_सारणी(bb)) प्रत्यागम असत्य;
            // Synthesize this block
            DfgVertex* condp = शून्यसूचकः;
            यदि (!synthesize_Basic_खण्डम्(m_bb_गन्तव्यम्_OSym_Tab[bb], condp, bb.stmtps(), m_bb_गन्तव्यम्_ISym_Tab[bb])) {
                प्रत्यागम असत्य;
            }
            // Create a temporary for the branch condition as it might be used multiple times
            यदि (condp) {
                File_पङ्क्तिः* ध्रुव flp = condp->fileline();
                ध्रुव Dfg_दत्त_Type& dtype = condp->dtype();
                ध्रुव std::string उपसर्गः = "_BB" + std::गन्तव्यम्_string(bb.id()) + "_Cond";
                DfgVertexVar* ध्रुव vp = m_converter.createTmp(*m_logicp, flp, dtype, उपसर्गः);
                vp->srcp(condp);
                m_bb_गन्तव्यम्_Condp[bb] = vp;
            }
            // Set the path predicates on the successor edges
            नियोजय_मार्गः_Predicates(bb);
        }

        // Check exernal writes are observed correctly
        यदि (!checkExtWrites()) प्रत्यागम असत्य;

        // Add resolved output variable drivers
        प्रत्यागम सङ्कलय_Synthesized_Output(m_bb_गन्तव्यम्_OSym_Tab[cfg.निर्गच्छ()]);
    }

    // Synthesize a DfgLogic into regular vertices. Returns ture on success.
    तार्किक synthesize(DfgLogic& vtx) {
        VL_RESTORER(m_logicp);
        m_logicp = &vtx;

        यदि (Ast_नियोजय_W* ध्रुव ap = VN_CAST(vtx.nodep()->stmtsp(), नियोजय_W)) {
            यदि (ap->nextp()) प्रत्यागम असत्य;
            यदि (!synthesize_नियोजय_W(ap)) प्रत्यागम असत्य;
            ++m_ctx.m_synt.synth_नियोजय;
            प्रत्यागम सत्य;
        }

        यदि (!synthesizeCfg(vtx.cfg())) प्रत्यागम असत्य;
        ++m_ctx.m_synt.synthAlways;
        प्रत्यागम सत्य;
    }

    // Revert all DfgLogic in m_toRevert, or DfgLogic driving the DfgUnresolved
    // vertices in m_toRevert, and transitively the same for any DfgUnresolved
    // driven by the reverted DfgLogic. Delete all DfgUnresolved involed.
    शून्य revert(VDouble0& statCountr) {
        m_गन्तव्यम्_Revert.foreach([&](DfgVertex& vtx) {
            // Process DfgLogic
            यदि (DfgLogic* ध्रुव vtxp = vtx.cast<DfgLogic>()) {
                UASSERT_OBJ(vtxp->selectedForSynthesis(), vtxp, "Shouldn't reach here unselected");
                // Revert this logic
                UASSERT_OBJ(!vtxp->reverted(), vtxp, "Should be reverting now");
                ++statCountr;
                प्रति (DfgVertex* ध्रुव p : vtxp->synth()) VL_DO_DANGLING(p->unlinkDelete(m_dfg), p);
                vtxp->synth().clear();
                vtxp->समुच्चयः_Reverted();
                // Add all DfgUnresolved it drives to the work list
                vtxp->foreachSink([&](DfgVertex& snk) {
                    m_गन्तव्यम्_Revert.प्रक्षिप_front(*snk.as<DfgUnresolved>());
                    प्रत्यागम असत्य;
                });
                प्रत्यागम;
            }

            // Process DfgUnresolved
            यदि (DfgUnresolved* ध्रुव vtxp = vtx.cast<DfgUnresolved>()) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->समुच्चयः_Has_Mod_Wr_Refs();
                // Add all driving DfgLogic to the work list
                vtxp->foreachSource([&](DfgVertex& src) {
                    DfgLogic* ध्रुव lp = src.cast<DfgLogic>();
                    यदि (lp && !lp->reverted()) m_गन्तव्यम्_Revert.प्रक्षिप_front(*lp);
                    प्रत्यागम असत्य;
                });
                // Delete the DfgUnresolved driver
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
                प्रत्यागम;
            }

            // There should be nothing else on the worklist
            vtx.v3fatalSrc("Unexpected vertex type");
        });
    }

    // Synthesize all of the given vertices
    शून्य मुख्यम्() {
        //-------------------------------------------------------------------
        UINFO(5, "Step 0: Remove all DfgLogic not selected for synthesis");
        प्रति (DfgVertex* ध्रुव vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgUnresolved
            यदि (!vtxp->is<DfgUnresolved>()) अनुवर्त;
            तार्किक anySelected = असत्य;
            तार्किक anyUnselected = असत्य;
            vtxp->foreachSource([&](DfgVertex& src) {
                ध्रुव DfgLogic& logic = *src.as<DfgLogic>();
                यदि (logic.selectedForSynthesis()) {
                    anySelected = सत्य;
                } अन्यथा {
                    anyUnselected = सत्य;
                }
                प्रत्यागम असत्य;
            });
            // There should be a driver
            UASSERT_OBJ(anySelected || anyUnselected, vtxp, "'DfgUnresolved' with no driver");
            // All drivers should be selected or all should be unselected
            UASSERT_OBJ(!(anySelected && anyUnselected), vtxp, "Invalid 'DfgLogic' selection");
            // If all drivers are unselected, delete this DfgUnresolved here
            यदि (anyUnselected) {
                // The result variable will be driven from Ast code, mark as such
                vtxp->singleSink()->as<DfgVertexVar>()->समुच्चयः_Has_Mod_Wr_Refs();
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
        प्रति (DfgVertex* ध्रुव vtxp : m_dfg.opVertices().unlinkable()) {
            // Only processing DfgLogic
            DfgLogic* ध्रुव logicp = vtxp->cast<DfgLogic>();
            यदि (!logicp) अनुवर्त;
            यदि (logicp->selectedForSynthesis()) अनुवर्त;
            // There should be no sinks left for unselected DfgLogic, delete them here
            UASSERT_OBJ(!logicp->hasSinks(), vtxp, "Unselected 'DfgLogic' with sinks remaining");
            // Input variables will be read in Ast code, add Ast reference vertices
            // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
            ध्रुव VNUser4_अन्तर्_Use m_user4_अन्तर्_Use;
            logicp->foreachSource([](DfgVertex& src) {
                src.as<DfgVertexVar>()->vscp()->user4p(&src);
                प्रत्यागम असत्य;
            });
            V3DfgPasses::सङ्कलय_Ast_Refs(m_dfg, logicp->nodep(), [](Ast_ग्रन्थिः* varp) {  //
                प्रत्यागम varp->user4u().गन्तव्यम्<DfgVertexVar*>();
            });
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        debugDump("synth-selected");

        //-------------------------------------------------------------------
        UINFO(5, "Step 1: Attempting to synthesize each of the selected DfgLogic");
        {
            // AstVarScope::user4() -> int: unique ID for 'AstVarScopeComparator'
            ध्रुव VNUser4_अन्तर्_Use user4_अन्तर्_Use;
            प्रति (DfgVertex& vtx : m_dfg.opVertices()) {
                DfgLogic* ध्रुव logicp = vtx.cast<DfgLogic>();
                यदि (!logicp) अनुवर्त;

                // We should only have DfgLogic remaining that was selected for synthesis
                UASSERT_OBJ(logicp->selectedForSynthesis(), logicp, "Unselected DfgLogic remains");

                // Debug aid
                ध्रुव स्वचालित debugCallback = [&]() -> शून्य {
                    // This is the breaking logic
                    m_debugLogicp = logicp;
                    // Dump it
                    UINFOTREE(0, logicp->nodep(), "Problematic DfgLogic: " << logicp, "  ");
                    V3EmitV::debug_Verilog_For_वृक्षः(logicp->nodep(), std::निर्गमः);
                    debugDump("synth-lastok");
                };
                यदि (VL_UNLIKELY(s_dfgSynthDebugBisect.stop(debugCallback))) भङ्ग;

                // Synthesize it, if failed, enqueue for reversion
                यदि (!synthesize(*logicp)) {
                    logicp->समुच्चयः_Non_Synthesizable();
                    m_गन्तव्यम्_Revert.प्रक्षिप_front(*logicp);
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
        std::सदिशम्<DfgVertexVar*> multidrivenps;
        // Map from variable to its resolved driver
        std::unordered_map<ध्रुव DfgVertexVar*, DfgVertexSplice*> resolvedDrivers;
        // Compute resolved drivers of all variablees
        प्रति (DfgVertexVar& var : m_dfg.varVertices()) {
            यदि (!var.srcp()) अनुवर्त;
            DfgUnresolved* ध्रुव unresolvedp = var.srcp()->cast<DfgUnresolved>();
            यदि (!unresolvedp) भङ्ग;  // Stop when reached the synthesized temporaries

            // Resolve the synthesized drivers
            DfgVertexSplice* ध्रुव resolvedp = [&]() -> DfgVertexSplice* {
                // All synthesized drivers were normalized already,
                // so if there is only one, it can be used directly
                यदि (unresolvedp->nInputs() == 1) {
                    प्रत्यागम unresolvedp->inputp(0)->as<DfgVertexSplice>();
                }
                // Otherwise gather the synthesized drivers
                std::सदिशम्<Driver> drivers = gatherDriversUnresolved(unresolvedp);
                // Normalize them, make resolved driver if all good
                यदि (normalizeDrivers(var, drivers)) प्रत्यागम makeSplice(var, drivers);
                // If mutlidriven, record and ignore
                multidrivenps.emplace_पृष्ठभागः(&var);
                m_गन्तव्यम्_Revert.प्रक्षिप_front(*unresolvedp);
                प्रत्यागम शून्यसूचकः;
            }();
            // Bail if multidriven
            यदि (!resolvedp) अनुवर्त;
            // Add to map for next loop
            ध्रुव तार्किक new_प्रविष्टिः = resolvedDrivers.emplace(&var, resolvedp).second;
            UASSERT_OBJ(new_प्रविष्टिः, &var, "Duplicate driver");
        }
        // Revert and remove drivers of multi-driven variables
        revert(m_ctx.m_synt.revertMultidrive);
        // Replace all DfgUnresolved with the resolved drivers
        प्रति (ध्रुव DfgVertexVar& var : m_dfg.varVertices()) {
            यदि (!var.srcp()) अनुवर्त;
            DfgUnresolved* ध्रुव srcp = var.srcp()->cast<DfgUnresolved>();
            यदि (!srcp) भङ्ग;  // Stop when reached the synthesized temporaries

            // Replace it
            srcp->replaceWith(resolvedDrivers.at(&var));
            VL_DO_DANGLING(srcp->unlinkDelete(m_dfg), srcp);
        }
        debugDump("synth-resolved");

        //-------------------------------------------------------------------
        UINFO(5, "Step 4: Remove all remaining DfgLogic and DfgUnresolved");
        प्रति (DfgVertex* ध्रुव vtxp : m_dfg.opVertices().unlinkable()) {
            // Previous step should have removed all DfgUnresolved
            UASSERT_OBJ(!vtxp->is<DfgUnresolved>(), vtxp, "DfgUnresolved remains");

            // Process only DfgLogic
            DfgLogic* ध्रुव logicp = vtxp->cast<DfgLogic>();
            यदि (!logicp) अनुवर्त;

            // Earlier pass should have removed all sinks
            UASSERT_OBJ(!logicp->hasSinks(), logicp, "DfgLogic sink remains");

            यदि (!logicp->synth().रिक्तानि()) {
                // If synthesized, delete the corresponding AstNode. It is now in Dfg.
                logicp->nodep()->unlink_Fr_पृष्ठभागः()->delete_वृक्षः();
            } अन्यथा {
                // Not synthesized. Logic stays in Ast. Add Ast reference vertices.
                // Outputs already marked by revertTransivelyAndRemove.
                // AstVarScope::user4p() -> corresponding DfgVertexVar* in the graph
                ध्रुव VNUser4_अन्तर्_Use m_user4_अन्तर्_Use;
                logicp->foreachSource([](DfgVertex& src) {
                    src.as<DfgVertexVar>()->vscp()->user4p(&src);
                    प्रत्यागम असत्य;
                });
                V3DfgPasses::सङ्कलय_Ast_Refs(m_dfg, logicp->nodep(), [](Ast_ग्रन्थिः* varp) {  //
                    प्रत्यागम varp->user4u().गन्तव्यम्<DfgVertexVar*>();
                });
            }

            // Delete this DfgLogic
            VL_DO_DANGLING(logicp->unlinkDelete(m_dfg), logicp);
        }
        // Reset the debug pointer, we have deleted it in the loop above ...
        m_debugLogicp = शून्यसूचकः;
        debugDump("synth-rmlogics");

        //-------------------------------------------------------------------
        UINFO(5, "Step 5: Remove unnecessary splices");
        प्रति (DfgVertex* ध्रुव vtxp : m_dfg.opVertices().unlinkable()) {
            DfgVertexSplice* ध्रुव splicep = vtxp->cast<DfgVertexSplice>();
            यदि (!splicep) अनुवर्त;

            // Might not have a sink if the driving logic was revered, remove
            यदि (!splicep->hasSinks()) {
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
                अनुवर्त;
            }

            // It should alway have drivers
            UASSERT_OBJ(splicep->nInputs(), splicep, "Splice with no drivers");

            // If redundant, remove it
            यदि (DfgVertex* ध्रुव wholep = splicep->wholep()) {
                यदि (DfgVertexVar* ध्रुव varp = splicep->singleSink()->cast<DfgVertexVar>()) {
                    varp->driver_File_पङ्क्तिः(splicep->driver_File_पङ्क्तिः(0));
                }
                splicep->replaceWith(wholep);
                VL_DO_DANGLING(splicep->unlinkDelete(m_dfg), splicep);
            }
        }
        debugDump("synth-rmsplice");
    }

    // CONSTRUCTOR
    Ast_गन्तव्यम्_Dfg_Synthesize(DfgGraph& dfg, V3DfgSynthesisContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx}
        , m_converter{dfg, ctx} {
        मुख्यम्();
    }

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, V3DfgSynthesisContext& ctx) {
        Ast_गन्तव्यम्_Dfg_Synthesize{dfg, ctx};

        // Final step outside, as both AstToDfgSynthesize and removeUnused used DfgUserMap
        UINFO(5, "Step 6: Remove all unused vertices");
        V3DfgPasses::remove_अप्रयुक्तम्(dfg);
        यदि (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("synth-rmunused");

        // No operation vertex should have multiple sinks. Cyclic decomoposition
        // depends on this and it can easily be ensured by using temporaries.
        // Also, all sources should be connected at this point
        यदि (v3Global.opt.debugCheck()) {
            प्रति (DfgVertex& vtx : dfg.opVertices()) {
                UASSERT_OBJ(!vtx.has_गुणजः_Sinks(), &vtx, "Operation has multiple sinks");
                प्रति (आकार_t i = 0; i < vtx.nInputs(); ++i) {
                    UASSERT_OBJ(vtx.inputp(i), &vtx, "Unconnected source operand");
                }
            }
            V3DfgPasses::typeCheck(dfg);
        }
    }
};

पूर्णाङ्क Ast_गन्तव्यम्_Dfg_Synthesize::AstVarScopeComparator::s_vscp_Id_गणक = 0;

// Decide which DfgLogic to attempt to synthesize
स्थिर शून्य dfgSelectLogicForSynthesis(DfgGraph& dfg) {
    // If we are told to synthesize everything, we will do so ...
    यदि (v3Global.opt.fDfgSynthesizeAll()) {
        प्रति (DfgVertex& vtx : dfg.opVertices()) {
            यदि (DfgLogic* ध्रुव logicp = vtx.cast<DfgLogic>()) logicp->समुच्चयः_Selected_For_Synthesis();
        }
        प्रत्यागम;
    }

    // Otherwise figure out which vertices are likely worth synthesizing.

    std::सदिशम्<DfgVertexVar*> circularVarps;
    // Synthesize all drivers of circular variables
    {
        DfgUserMap<uint64_t> scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, scc);
        प्रति (DfgVertexVar& var : dfg.varVertices()) {
            यदि (!scc.at(var)) अनुवर्त;
            // This is a circular variable
            circularVarps.emplace_पृष्ठभागः(&var);
        }
    }

    // To cover all drivers, use a work list
    DfgWorklist worklist{dfg};

    // Synthesize all drivers of circular variables
    प्रति (ध्रुव DfgVertexVar* ध्रुव varp : circularVarps) {
        varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
            worklist.प्रक्षिप_front(*driver.as<DfgLogic>());
            प्रत्यागम असत्य;
        });
    }

    // Choose some simple special cases to always synthesize
    प्रति (DfgVertex& vtx : dfg.opVertices()) {
        DfgLogic* ध्रुव logicp = vtx.cast<DfgLogic>();
        यदि (!logicp || worklist.अन्तर्भवति(*logicp)) अनुवर्त;
        // If drives an unused variable, synthesize it so the partial logic can be removed
        यदि (logicp->drives_अप्रयुक्तम्_Vars()) {
            worklist.प्रक्षिप_front(*logicp);
            अनुवर्त;
        }
        // Blocks corresponding to continuous assignments
        यदि (logicp->nodep()->keyword() == VAlwaysKwd::CONT_नियोजय) {
            worklist.प्रक्षिप_front(*logicp);
            अनुवर्त;
        }
        ध्रुव CfgGraph& cfg = logicp->cfg();
        // Straight line code with no branches
        यदि (cfg.n_खण्डानि() == 1) {
            worklist.प्रक्षिप_front(*logicp);
            अनुवर्त;
        }
        // Blocks driving exactly 1 variable
        यदि (!logicp->has_गुणजः_Sinks()) worklist.प्रक्षिप_front(*logicp);
    }

    std::unordered_समुच्चयः<DfgUnresolved*> isVisited;

    // Now expand to cover all logic driving the same set of variables and mark
    worklist.foreach([&](DfgVertex& vtx) {
        DfgLogic& logic = *vtx.as<DfgLogic>();
        UASSERT_OBJ(!logic.selectedForSynthesis(), &vtx, "Should not be visited twice");
        // Mark as selected for synthesis
        logic.समुच्चयः_Selected_For_Synthesis();
        // Enqueue all other logic driving the same variables as this one
        logic.foreachSink([&](DfgVertex& sink) {
            DfgUnresolved* unresolved = sink.as<DfgUnresolved>();
            यदि (!isVisited.निवेशय(unresolved).second) प्रत्यागम असत्य;
            unresolved->foreachSource([&](DfgVertex& sibling) {
                DfgLogic& siblingLogic = *sibling.as<DfgLogic>();
                यदि (!siblingLogic.selectedForSynthesis()) worklist.प्रक्षिप_front(siblingLogic);
                प्रत्यागम असत्य;
            });
            प्रत्यागम असत्य;
        });
    });
}

शून्य V3DfgPasses::synthesize(DfgGraph& dfg, V3DfgContext& ctx) {
    // Select which DfgLogic to attempt to synthesize
    dfgSelectLogicForSynthesis(dfg);
    // Synthesize them - also removes un-synthesized DfgLogic, so must run even if nothing selected
    Ast_गन्तव्यम्_Dfg_Synthesize::apply(dfg, ctx.m_synthContext);
}
