// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Convert DfgGraph to AstModule
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
// Convert DfgGraph back to AstModule. We recursively construct AstNodeExpr expressions for each
// DfgVertex which represents a storage location (e.g.: DfgVarPacked), or has multiple sinks
// without driving a storage location (and hence needs a temporary variable to duplication). The
// recursion stops when we reach a DfgVertex representing a storage location (e.g.: DfgVarPacked),
// or a vertex that that has multiple sinks (as these nodes will have a [potentially new temporary]
// corresponding// storage location). Redundant variables (those whose source vertex drives
// multiple variables) are eliminated when possible. Vertices driving multiple variables are
// rendered once, driving an arbitrarily (but deterministically) chosen canonical variable, and the
// corresponding redundant variables are assigned from the canonical variable.
//
//*************************************************************************

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgPasses.h"
#अन्तर्भाव "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश {

// Create an AstNodeExpr out of a DfgVertex. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
template <typename T_ग्रन्थिः, typename T_Vertex, typename... Ops>
T_ग्रन्थिः* make_ग्रन्थिः(ध्रुव T_Vertex* vtxp, Ops... ops) {
    T_ग्रन्थिः* ध्रुव nodep = नूतन T_ग्रन्थिः{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->width() == static_cast<पूर्णाङ्क>(vtxp->width()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->width() << " vs " << vtxp->width());
    पटिनिवत्त nodep;
}

//======================================================================
// Vertices needing special conversion

template <>
Ast_गणनम्_Ones* make_ग्रन्थिः<Ast_गणनम्_Ones, Dfg_गणनम्_Ones, Ast_ग्रन्थिः_व्यञ्जकम्*>(  //
    ध्रुव Dfg_गणनम्_Ones* vtxp, Ast_ग्रन्थिः_व्यञ्जकम्* op1) {
    Ast_गणनम्_Ones* ध्रुव nodep = नूतन Ast_गणनम्_Ones{vtxp->fileline(), op1};
    पटिनिवत्त nodep;
}

template <>
AstExtend* make_ग्रन्थिः<AstExtend, DfgExtend, Ast_ग्रन्थिः_व्यञ्जकम्*>(  //
    ध्रुव DfgExtend* vtxp, Ast_ग्रन्थिः_व्यञ्जकम्* op1) {
    पटिनिवत्त नूतन AstExtend{vtxp->fileline(), op1, static_cast<पूर्णाङ्क>(vtxp->width())};
}

template <>
AstExtendS* make_ग्रन्थिः<AstExtendS, DfgExtendS, Ast_ग्रन्थिः_व्यञ्जकम्*>(  //
    ध्रुव DfgExtendS* vtxp, Ast_ग्रन्थिः_व्यञ्जकम्* op1) {
    पटिनिवत्त नूतन AstExtendS{vtxp->fileline(), op1, static_cast<पूर्णाङ्क>(vtxp->width())};
}

template <>
AstShiftL* make_ग्रन्थिः<AstShiftL, DfgShiftL, Ast_ग्रन्थिः_व्यञ्जकम्*, Ast_ग्रन्थिः_व्यञ्जकम्*>(  //
    ध्रुव DfgShiftL* vtxp, Ast_ग्रन्थिः_व्यञ्जकम्* op1, Ast_ग्रन्थिः_व्यञ्जकम्* op2) {
    पटिनिवत्त नूतन AstShiftL{vtxp->fileline(), op1, op2, static_cast<पूर्णाङ्क>(vtxp->width())};
}

template <>
AstShiftR* make_ग्रन्थिः<AstShiftR, DfgShiftR, Ast_ग्रन्थिः_व्यञ्जकम्*, Ast_ग्रन्थिः_व्यञ्जकम्*>(  //
    ध्रुव DfgShiftR* vtxp, Ast_ग्रन्थिः_व्यञ्जकम्* op1, Ast_ग्रन्थिः_व्यञ्जकम्* op2) {
    पटिनिवत्त नूतन AstShiftR{vtxp->fileline(), op1, op2, static_cast<पूर्णाङ्क>(vtxp->width())};
}

template <>
AstShiftRS* make_ग्रन्थिः<AstShiftRS, DfgShiftRS, Ast_ग्रन्थिः_व्यञ्जकम्*, Ast_ग्रन्थिः_व्यञ्जकम्*>(  //
    ध्रुव DfgShiftRS* vtxp, Ast_ग्रन्थिः_व्यञ्जकम्* op1, Ast_ग्रन्थिः_व्यञ्जकम्* op2) {
    पटिनिवत्त नूतन AstShiftRS{vtxp->fileline(), op1, op2, static_cast<पूर्णाङ्क>(vtxp->width())};
}

}  // namespace

वग्ग DfgToAstVisitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ध्रुव VNUser2InUse m_user2InUse;

    // STATE
    V3DfgDfgToAstContext& m_ctx;  // The context for stats
    Ast_ग्रन्थिः_व्यञ्जकम्* m_resultp = nullptr;  // The result node of the current traversal
    AstAlways* m_alwaysp = nullptr;  // Process to add assignments to, if have a default driver
    AstActive* m_activep = nullptr;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    स्थिर AstActive* getCombActive(AstScope* scopep) {
        सचे (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            अत्थाय (Ast_ग्रन्थिः* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ध्रुव activep = VN_CAST(nodep, Active);
                सचे (!activep) अनुवर्त;
                सचे (activep->hasCombo()) {
                    scopep->user2p(activep);
                    भङ्ग;
                }
            }
            // If there isn't one, create a new one
            सचे (!scopep->user2p()) {
                File_पङ्क्तिः* ध्रुव flp = scopep->fileline();
                Ast_Sen_वृक्षः* ध्रुव senTreep
                    = नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ध्रुव activep = नूतन AstActive{flp, "", senTreep};
                activep->sen_वृक्षः_Storep(senTreep);
                scopep->addBlocksp(activep);
                scopep->user2p(activep);
            }
        }
        पटिनिवत्त VN_AS(scopep->user2p(), Active);
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* convert_Dfg_Vertex_To_Ast_ग्रन्थिः_व्यञ्जकम्(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->hasMultipleSinks() || vtxp->isCheaperThanLoad(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव resultp = m_resultp;
        m_resultp = nullptr;
        पटिनिवत्त resultp;
    }

    शून्य createAssignment(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_resultEquations;
        // Render the driver
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = convert_Dfg_Vertex_To_Ast_ग्रन्थिः_व्यञ्जकम्(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_ग्रन्थिः* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        सचे (m_alwaysp) {
            m_alwaysp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
            पटिनिवत्त;
        }

        // Otherwise create an AssignW
        AstAssignW* ध्रुव ap = नूतन AstAssignW{flp, lhsp, rhsp};
        m_activep->addStmtsp(नूतन AstAlways{ap});
    }

    शून्य convertDriver(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, DfgVertex* driverp) {
        सचे (DfgSplicePacked* ध्रुव sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_पङ्क्तिः* dflp) {
                // Create Sel
                AstConst* ध्रुव lsbp = नूतन AstConst{dflp, lo};
                ध्रुव पूर्णाङ्क width = static_cast<पूर्णाङ्क>(src.width());
                AstSel* ध्रुव nLhsp = नूतन AstSel{dflp, lhsp->clone_वृक्षः_Pure(मिच्छा), lsbp, width};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_वृक्षः(), nLhsp);
                पटिनिवत्त मिच्छा;
            });
            पटिनिवत्त;
        }

        सचे (DfgSpliceArray* ध्रुव sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_पङ्क्तिः* dflp) {
                UASSERT_OBJ(src.size() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ध्रुव idxp = नूतन AstConst{dflp, lo};
                AstArraySel* ध्रुव nLhsp = नूतन AstArraySel{dflp, lhsp->clone_वृक्षः_Pure(मिच्छा), idxp};
                // Convert source
                सचे (ध्रुव DfgUnitArray* ध्रुव uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } अञ्ञथा {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_वृक्षः(), nLhsp);
                पटिनिवत्त मिच्छा;
            });
            पटिनिवत्त;
        }

        सचे (ध्रुव DfgUnitArray* ध्रुव uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ध्रुव idxp = नूतन AstConst{flp, 0};
            AstArraySel* ध्रुव nLhsp = नूतन AstArraySel{flp, lhsp->clone_वृक्षः_Pure(मिच्छा), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_वृक्षः(), nLhsp);
            पटिनिवत्त;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_वृक्षः_Pure(मिच्छा), driverp);
    }

    // VISITORS
    शून्य visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    शून्य visit(DfgVarPacked* vtxp) override {
        m_resultp = नूतन AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    शून्य visit(DfgVarArray* vtxp) override {
        m_resultp = नूतन AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    शून्य visit(DfgPrev* vtxp) override {
        m_resultp = नूतन AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    शून्य visit(DfgConst* vtxp) override {  //
        m_resultp = नूतन AstConst{vtxp->fileline(), vtxp->अङ्कः()};
    }
    शून्य visit(DfgCReset* vtxp) override {
        DfgVertex* ध्रुव sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* ध्रुव varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = नूतन AstCReset{vtxp->fileline(), varp, मिच्छा};
    }
    शून्य visit(DfgMatchMasked* vtxp) override {
        File_पङ्क्तिः* ध्रुव flp = vtxp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = convert_Dfg_Vertex_To_Ast_ग्रन्थिः_व्यञ्जकम्(vtxp->lhsp());
        AstVarScope* ध्रुव matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = नूतन AstMatchMasked{flp, lhsp, नूतन AstVarRef{flp, matchp, VAccess::READ}};
    }

    शून्य visit(DfgRep* vtxp) override {
        File_पङ्क्तिः* ध्रुव flp = vtxp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव srcp = convert_Dfg_Vertex_To_Ast_ग्रन्थिः_व्यञ्जकम्(vtxp->srcp());
        m_resultp = नूतन AstReplicate{flp, srcp, vtxp->गणनम्()};
    }

    शून्य visit(DfgSel* vtxp) override {
        File_पङ्क्तिः* ध्रुव flp = vtxp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = convert_Dfg_Vertex_To_Ast_ग्रन्थिः_व्यञ्जकम्(vtxp->fromp());
        AstConst* ध्रुव lsbp = नूतन AstConst{flp, vtxp->lsb()};
        m_resultp = नूतन AstSel{flp, fromp, lsbp, static_cast<पूर्णाङ्क>(vtxp->width())};
    }

    शून्य visit(DfgMux* vtxp) override {
        File_पङ्क्तिः* ध्रुव flp = vtxp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = convert_Dfg_Vertex_To_Ast_ग्रन्थिः_व्यञ्जकम्(vtxp->fromp());
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lsbp = convert_Dfg_Vertex_To_Ast_ग्रन्थिः_व्यञ्जकम्(vtxp->lsbp());
        m_resultp = नूतन AstSel{flp, fromp, lsbp, static_cast<पूर्णाङ्क>(vtxp->width())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#अन्तर्भाव "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    DfgToAstVisitor(DfgGraph& dfg, V3DfgDfgToAstContext& ctx)
        : m_ctx{ctx} {
        सचे (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        अत्थाय (DfgVertexVar& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            सचे (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                अनुवर्त;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_पङ्क्तिः* ध्रुव flp = vtx.driver_File_पङ्क्तिः() ? vtx.driver_File_पङ्क्तिः() : vtx.fileline();
            AstVarRef* ध्रुव lhsp = नूतन AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = getCombActive(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            सचे (DfgVertex* ध्रुव defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = नूतन AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr};
                m_activep->addStmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_वृक्षः_Pure(मिच्छा), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_वृक्षः(), lhsp);
        }

        // Update Ast References
        अत्थाय (DfgVertexAst& vtx : dfg.astVertices()) {
            सचे (DfgAstRd* ध्रुव rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = convert_Dfg_Vertex_To_Ast_ग्रन्थिः_व्यञ्जकम्(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                सचे (exprp->same_वृक्षः(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_वृक्षः(), exprp);
                    अनुवर्त;
                }
                // Replace the reference with the expression
                सचे (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } अञ्ञथा {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_वृक्षः();
                rVtxp->exprp(exprp);
            }
        }
    }

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, V3DfgDfgToAstContext& ctx) { DfgToAstVisitor{dfg, ctx}; }
};

शून्य V3DfgPasses::dfgToAst(DfgGraph& dfg, V3DfgContext& ctx) {
    DfgToAstVisitor::apply(dfg, ctx.m_dfg2AstContext);
}
