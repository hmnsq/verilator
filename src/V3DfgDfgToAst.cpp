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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Dfg.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgPasses.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ {

// Create an AstNodeExpr out of a DfgVertex. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
template <typename T_ଗଣ୍ଠି, typename T_Vertex, typename... Ops>
T_ଗଣ୍ଠି* make_ଗଣ୍ଠି(ଅଚଳ T_Vertex* vtxp, Ops... ops) {
    T_ଗଣ୍ଠି* ଅଚଳ nodep = ନୂତନ T_ଗଣ୍ଠି{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->width() == static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(vtxp->width()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->width() << " vs " << vtxp->width());
    ଫେରନ୍ତୁ nodep;
}

//======================================================================
// Vertices needing special conversion

template <>
Ast_ଗଣନା_Ones* make_ଗଣ୍ଠି<Ast_ଗଣନା_Ones, Dfg_ଗଣନା_Ones, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(  //
    ଅଚଳ Dfg_ଗଣନା_Ones* vtxp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* op1) {
    Ast_ଗଣନା_Ones* ଅଚଳ nodep = ନୂତନ Ast_ଗଣନା_Ones{vtxp->fileline(), op1};
    ଫେରନ୍ତୁ nodep;
}

template <>
AstExtend* make_ଗଣ୍ଠି<AstExtend, DfgExtend, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(  //
    ଅଚଳ DfgExtend* vtxp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* op1) {
    ଫେରନ୍ତୁ ନୂତନ AstExtend{vtxp->fileline(), op1, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(vtxp->width())};
}

template <>
AstExtendS* make_ଗଣ୍ଠି<AstExtendS, DfgExtendS, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(  //
    ଅଚଳ DfgExtendS* vtxp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* op1) {
    ଫେରନ୍ତୁ ନୂତନ AstExtendS{vtxp->fileline(), op1, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(vtxp->width())};
}

template <>
AstShiftL* make_ଗଣ୍ଠି<AstShiftL, DfgShiftL, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(  //
    ଅଚଳ DfgShiftL* vtxp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* op1, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* op2) {
    ଫେରନ୍ତୁ ନୂତନ AstShiftL{vtxp->fileline(), op1, op2, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(vtxp->width())};
}

template <>
AstShiftR* make_ଗଣ୍ଠି<AstShiftR, DfgShiftR, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(  //
    ଅଚଳ DfgShiftR* vtxp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* op1, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* op2) {
    ଫେରନ୍ତୁ ନୂତନ AstShiftR{vtxp->fileline(), op1, op2, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(vtxp->width())};
}

template <>
AstShiftRS* make_ଗଣ୍ଠି<AstShiftRS, DfgShiftRS, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(  //
    ଅଚଳ DfgShiftRS* vtxp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* op1, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* op2) {
    ଫେରନ୍ତୁ ନୂତନ AstShiftRS{vtxp->fileline(), op1, op2, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(vtxp->width())};
}

}  // namespace

ଶ୍ରେଣୀ DfgToAstVisitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ଅଚଳ VNUser2InUse m_user2InUse;

    // STATE
    V3DfgDfgToAstContext& m_ctx;  // The context for stats
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_resultp = nullptr;  // The result node of the current traversal
    AstAlways* m_alwaysp = nullptr;  // Process to add assignments to, if have a default driver
    AstActive* m_activep = nullptr;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    ସ୍ଥିର AstActive* getCombActive(AstScope* scopep) {
        ଯଦି (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ଅଚଳ activep = VN_CAST(nodep, Active);
                ଯଦି (!activep) ଜାରି;
                ଯଦି (activep->hasCombo()) {
                    scopep->user2p(activep);
                    ଭାଙ୍ଗ;
                }
            }
            // If there isn't one, create a new one
            ଯଦି (!scopep->user2p()) {
                File_ଧାଡ଼ି* ଅଚଳ flp = scopep->fileline();
                Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep
                    = ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, ନୂତନ AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ଅଚଳ activep = ନୂତନ AstActive{flp, "", senTreep};
                activep->sen_ବୃକ୍ଷ_Storep(senTreep);
                scopep->addBlocksp(activep);
                scopep->user2p(activep);
            }
        }
        ଫେରନ୍ତୁ VN_AS(scopep->user2p(), Active);
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* convert_Dfg_Vertex_To_Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->hasMultipleSinks() || vtxp->isCheaperThanLoad(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ resultp = m_resultp;
        m_resultp = nullptr;
        ଫେରନ୍ତୁ resultp;
    }

    ଶୂନ୍ୟ createAssignment(File_ଧାଡ଼ି* flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_resultEquations;
        // Render the driver
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = convert_Dfg_Vertex_To_Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_ଗଣ୍ଠି* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        ଯଦି (m_alwaysp) {
            m_alwaysp->addStmtsp(ନୂତନ AstAssign{flp, lhsp, rhsp});
            ଫେରନ୍ତୁ;
        }

        // Otherwise create an AssignW
        AstAssignW* ଅଚଳ ap = ନୂତନ AstAssignW{flp, lhsp, rhsp};
        m_activep->addStmtsp(ନୂତନ AstAlways{ap});
    }

    ଶୂନ୍ୟ convertDriver(File_ଧାଡ଼ି* flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, DfgVertex* driverp) {
        ଯଦି (DfgSplicePacked* ଅଚଳ sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_ଧାଡ଼ି* dflp) {
                // Create Sel
                AstConst* ଅଚଳ lsbp = ନୂତନ AstConst{dflp, lo};
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(src.width());
                AstSel* ଅଚଳ nLhsp = ନୂତନ AstSel{dflp, lhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), lsbp, width};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_ବୃକ୍ଷ(), nLhsp);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
            ଫେରନ୍ତୁ;
        }

        ଯଦି (DfgSpliceArray* ଅଚଳ sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_ଧାଡ଼ି* dflp) {
                UASSERT_OBJ(src.size() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ଅଚଳ idxp = ନୂତନ AstConst{dflp, lo};
                AstArraySel* ଅଚଳ nLhsp = ନୂତନ AstArraySel{dflp, lhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), idxp};
                // Convert source
                ଯଦି (ଅଚଳ DfgUnitArray* ଅଚଳ uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } ଅନ୍ୟଥା {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_ବୃକ୍ଷ(), nLhsp);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
            ଫେରନ୍ତୁ;
        }

        ଯଦି (ଅଚଳ DfgUnitArray* ଅଚଳ uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ଅଚଳ idxp = ନୂତନ AstConst{flp, 0};
            AstArraySel* ଅଚଳ nLhsp = ନୂତନ AstArraySel{flp, lhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_ବୃକ୍ଷ(), nLhsp);
            ଫେରନ୍ତୁ;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), driverp);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    ଶୂନ୍ୟ visit(DfgVarPacked* vtxp) override {
        m_resultp = ନୂତନ AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    ଶୂନ୍ୟ visit(DfgVarArray* vtxp) override {
        m_resultp = ନୂତନ AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    ଶୂନ୍ୟ visit(DfgPrev* vtxp) override {
        m_resultp = ନୂତନ AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    ଶୂନ୍ୟ visit(DfgConst* vtxp) override {  //
        m_resultp = ନୂତନ AstConst{vtxp->fileline(), vtxp->ଅଙ୍କ()};
    }
    ଶୂନ୍ୟ visit(DfgCReset* vtxp) override {
        DfgVertex* ଅଚଳ sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* ଅଚଳ varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = ନୂତନ AstCReset{vtxp->fileline(), varp, ମିଥ୍ୟା};
    }
    ଶୂନ୍ୟ visit(DfgMatchMasked* vtxp) override {
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = convert_Dfg_Vertex_To_Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି(vtxp->lhsp());
        AstVarScope* ଅଚଳ matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = ନୂତନ AstMatchMasked{flp, lhsp, ନୂତନ AstVarRef{flp, matchp, VAccess::READ}};
    }

    ଶୂନ୍ୟ visit(DfgRep* vtxp) override {
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ srcp = convert_Dfg_Vertex_To_Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି(vtxp->srcp());
        m_resultp = ନୂତନ AstReplicate{flp, srcp, vtxp->ଗଣନା_2()};
    }

    ଶୂନ୍ୟ visit(DfgSel* vtxp) override {
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = convert_Dfg_Vertex_To_Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି(vtxp->fromp());
        AstConst* ଅଚଳ lsbp = ନୂତନ AstConst{flp, vtxp->lsb()};
        m_resultp = ନୂତନ AstSel{flp, fromp, lsbp, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(vtxp->width())};
    }

    ଶୂନ୍ୟ visit(DfgMux* vtxp) override {
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = convert_Dfg_Vertex_To_Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି(vtxp->fromp());
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lsbp = convert_Dfg_Vertex_To_Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି(vtxp->lsbp());
        m_resultp = ନୂତନ AstSel{flp, fromp, lsbp, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(vtxp->width())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#ଅନ୍ତର୍ଭାଵ "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    DfgToAstVisitor(DfgGraph& dfg, V3DfgDfgToAstContext& ctx)
        : m_ctx{ctx} {
        ଯଦି (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        ପାଇଁ (DfgVertexVar& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            ଯଦି (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                ଜାରି;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_ଧାଡ଼ି* ଅଚଳ flp = vtx.driver_File_ଧାଡ଼ି() ? vtx.driver_File_ଧାଡ଼ି() : vtx.fileline();
            AstVarRef* ଅଚଳ lhsp = ନୂତନ AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = getCombActive(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            ଯଦି (DfgVertex* ଅଚଳ defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = ନୂତନ AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr};
                m_activep->addStmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_ବୃକ୍ଷ(), lhsp);
        }

        // Update Ast References
        ପାଇଁ (DfgVertexAst& vtx : dfg.astVertices()) {
            ଯଦି (DfgAstRd* ଅଚଳ rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = convert_Dfg_Vertex_To_Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                ଯଦି (exprp->same_ବୃକ୍ଷ(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_ବୃକ୍ଷ(), exprp);
                    ଜାରି;
                }
                // Replace the reference with the expression
                ଯଦି (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } ଅନ୍ୟଥା {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_ବୃକ୍ଷ();
                rVtxp->exprp(exprp);
            }
        }
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(DfgGraph& dfg, V3DfgDfgToAstContext& ctx) { DfgToAstVisitor{dfg, ctx}; }
};

ଶୂନ୍ୟ V3DfgPasses::dfgToAst(DfgGraph& dfg, V3DfgContext& ctx) {
    DfgToAstVisitor::apply(dfg, ctx.m_dfg2AstContext);
}
