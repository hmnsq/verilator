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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Dfg.h"
#અન્તર્ભાવ "V3DfgPasses.h"
#અન્તર્ભાવ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ {

// Create an AstNodeExpr out of a DfgVertex. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
template <typename T_ગાંઠ, typename T_Vertex, typename... Ops>
T_ગાંઠ* make_ગાંઠ(અચલ T_Vertex* vtxp, Ops... ops) {
    T_ગાંઠ* અચલ nodep = નવું T_ગાંઠ{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->width() == static_cast<પૂર્ણાંક>(vtxp->width()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->width() << " vs " << vtxp->width());
    પાછા nodep;
}

//======================================================================
// Vertices needing special conversion

template <>
Ast_ગણતરી_Ones* make_ગાંઠ<Ast_ગણતરી_Ones, Dfg_ગણતરી_Ones, Ast_ગાંઠ_પદાવલિ*>(  //
    અચલ Dfg_ગણતરી_Ones* vtxp, Ast_ગાંઠ_પદાવલિ* op1) {
    Ast_ગણતરી_Ones* અચલ nodep = નવું Ast_ગણતરી_Ones{vtxp->fileline(), op1};
    પાછા nodep;
}

template <>
AstExtend* make_ગાંઠ<AstExtend, DfgExtend, Ast_ગાંઠ_પદાવલિ*>(  //
    અચલ DfgExtend* vtxp, Ast_ગાંઠ_પદાવલિ* op1) {
    પાછા નવું AstExtend{vtxp->fileline(), op1, static_cast<પૂર્ણાંક>(vtxp->width())};
}

template <>
AstExtendS* make_ગાંઠ<AstExtendS, DfgExtendS, Ast_ગાંઠ_પદાવલિ*>(  //
    અચલ DfgExtendS* vtxp, Ast_ગાંઠ_પદાવલિ* op1) {
    પાછા નવું AstExtendS{vtxp->fileline(), op1, static_cast<પૂર્ણાંક>(vtxp->width())};
}

template <>
AstShiftL* make_ગાંઠ<AstShiftL, DfgShiftL, Ast_ગાંઠ_પદાવલિ*, Ast_ગાંઠ_પદાવલિ*>(  //
    અચલ DfgShiftL* vtxp, Ast_ગાંઠ_પદાવલિ* op1, Ast_ગાંઠ_પદાવલિ* op2) {
    પાછા નવું AstShiftL{vtxp->fileline(), op1, op2, static_cast<પૂર્ણાંક>(vtxp->width())};
}

template <>
AstShiftR* make_ગાંઠ<AstShiftR, DfgShiftR, Ast_ગાંઠ_પદાવલિ*, Ast_ગાંઠ_પદાવલિ*>(  //
    અચલ DfgShiftR* vtxp, Ast_ગાંઠ_પદાવલિ* op1, Ast_ગાંઠ_પદાવલિ* op2) {
    પાછા નવું AstShiftR{vtxp->fileline(), op1, op2, static_cast<પૂર્ણાંક>(vtxp->width())};
}

template <>
AstShiftRS* make_ગાંઠ<AstShiftRS, DfgShiftRS, Ast_ગાંઠ_પદાવલિ*, Ast_ગાંઠ_પદાવલિ*>(  //
    અચલ DfgShiftRS* vtxp, Ast_ગાંઠ_પદાવલિ* op1, Ast_ગાંઠ_પદાવલિ* op2) {
    પાછા નવું AstShiftRS{vtxp->fileline(), op1, op2, static_cast<પૂર્ણાંક>(vtxp->width())};
}

}  // namespace

વર્ગ DfgToAstVisitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    અચલ VNUser2InUse m_user2InUse;

    // STATE
    V3DfgDfgToAstContext& m_ctx;  // The context for stats
    Ast_ગાંઠ_પદાવલિ* m_resultp = nullptr;  // The result node of the current traversal
    AstAlways* m_alwaysp = nullptr;  // Process to add assignments to, if have a default driver
    AstActive* m_activep = nullptr;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    સ્થિર AstActive* getCombActive(AstScope* scopep) {
        જો (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            માટે (Ast_ગાંઠ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* અચલ activep = VN_CAST(nodep, Active);
                જો (!activep) ચાલુ;
                જો (activep->hasCombo()) {
                    scopep->user2p(activep);
                    તોડો;
                }
            }
            // If there isn't one, create a new one
            જો (!scopep->user2p()) {
                File_લીટી* અચલ flp = scopep->fileline();
                Ast_Sen_વૃક્ષ* અચલ senTreep
                    = નવું Ast_Sen_વૃક્ષ{flp, નવું AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* અચલ activep = નવું AstActive{flp, "", senTreep};
                activep->sen_વૃક્ષ_Storep(senTreep);
                scopep->addBlocksp(activep);
                scopep->user2p(activep);
            }
        }
        પાછા VN_AS(scopep->user2p(), Active);
    }

    Ast_ગાંઠ_પદાવલિ* convert_Dfg_Vertex_To_Ast_ગાંઠ_પદાવલિ(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->hasMultipleSinks() || vtxp->isCheaperThanLoad(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_ગાંઠ_પદાવલિ* અચલ resultp = m_resultp;
        m_resultp = nullptr;
        પાછા resultp;
    }

    શૂન્ય createAssignment(File_લીટી* flp, Ast_ગાંઠ_પદાવલિ* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_resultEquations;
        // Render the driver
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = convert_Dfg_Vertex_To_Ast_ગાંઠ_પદાવલિ(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_ગાંઠ* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        જો (m_alwaysp) {
            m_alwaysp->addStmtsp(નવું AstAssign{flp, lhsp, rhsp});
            પાછા;
        }

        // Otherwise create an AssignW
        AstAssignW* અચલ ap = નવું AstAssignW{flp, lhsp, rhsp};
        m_activep->addStmtsp(નવું AstAlways{ap});
    }

    શૂન્ય convertDriver(File_લીટી* flp, Ast_ગાંઠ_પદાવલિ* lhsp, DfgVertex* driverp) {
        જો (DfgSplicePacked* અચલ sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_લીટી* dflp) {
                // Create Sel
                AstConst* અચલ lsbp = નવું AstConst{dflp, lo};
                અચલ પૂર્ણાંક width = static_cast<પૂર્ણાંક>(src.width());
                AstSel* અચલ nLhsp = નવું AstSel{dflp, lhsp->clone_વૃક્ષ_Pure(ખોટું), lsbp, width};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_વૃક્ષ(), nLhsp);
                પાછા ખોટું;
            });
            પાછા;
        }

        જો (DfgSpliceArray* અચલ sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_લીટી* dflp) {
                UASSERT_OBJ(src.size() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* અચલ idxp = નવું AstConst{dflp, lo};
                AstArraySel* અચલ nLhsp = નવું AstArraySel{dflp, lhsp->clone_વૃક્ષ_Pure(ખોટું), idxp};
                // Convert source
                જો (અચલ DfgUnitArray* અચલ uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } નહીંતર {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_વૃક્ષ(), nLhsp);
                પાછા ખોટું;
            });
            પાછા;
        }

        જો (અચલ DfgUnitArray* અચલ uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* અચલ idxp = નવું AstConst{flp, 0};
            AstArraySel* અચલ nLhsp = નવું AstArraySel{flp, lhsp->clone_વૃક્ષ_Pure(ખોટું), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_વૃક્ષ(), nLhsp);
            પાછા;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_વૃક્ષ_Pure(ખોટું), driverp);
    }

    // VISITORS
    શૂન્ય visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    શૂન્ય visit(DfgVarPacked* vtxp) override {
        m_resultp = નવું AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    શૂન્ય visit(DfgVarArray* vtxp) override {
        m_resultp = નવું AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    શૂન્ય visit(DfgPrev* vtxp) override {
        m_resultp = નવું AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    શૂન્ય visit(DfgConst* vtxp) override {  //
        m_resultp = નવું AstConst{vtxp->fileline(), vtxp->અંક()};
    }
    શૂન્ય visit(DfgCReset* vtxp) override {
        DfgVertex* અચલ sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* અચલ varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = નવું AstCReset{vtxp->fileline(), varp, ખોટું};
    }
    શૂન્ય visit(DfgMatchMasked* vtxp) override {
        File_લીટી* અચલ flp = vtxp->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = convert_Dfg_Vertex_To_Ast_ગાંઠ_પદાવલિ(vtxp->lhsp());
        AstVarScope* અચલ matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = નવું AstMatchMasked{flp, lhsp, નવું AstVarRef{flp, matchp, VAccess::READ}};
    }

    શૂન્ય visit(DfgRep* vtxp) override {
        File_લીટી* અચલ flp = vtxp->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ srcp = convert_Dfg_Vertex_To_Ast_ગાંઠ_પદાવલિ(vtxp->srcp());
        m_resultp = નવું AstReplicate{flp, srcp, vtxp->ગણતરી()};
    }

    શૂન્ય visit(DfgSel* vtxp) override {
        File_લીટી* અચલ flp = vtxp->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ fromp = convert_Dfg_Vertex_To_Ast_ગાંઠ_પદાવલિ(vtxp->fromp());
        AstConst* અચલ lsbp = નવું AstConst{flp, vtxp->lsb()};
        m_resultp = નવું AstSel{flp, fromp, lsbp, static_cast<પૂર્ણાંક>(vtxp->width())};
    }

    શૂન્ય visit(DfgMux* vtxp) override {
        File_લીટી* અચલ flp = vtxp->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ fromp = convert_Dfg_Vertex_To_Ast_ગાંઠ_પદાવલિ(vtxp->fromp());
        Ast_ગાંઠ_પદાવલિ* અચલ lsbp = convert_Dfg_Vertex_To_Ast_ગાંઠ_પદાવલિ(vtxp->lsbp());
        m_resultp = નવું AstSel{flp, fromp, lsbp, static_cast<પૂર્ણાંક>(vtxp->width())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#અન્તર્ભાવ "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    DfgToAstVisitor(DfgGraph& dfg, V3DfgDfgToAstContext& ctx)
        : m_ctx{ctx} {
        જો (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        માટે (DfgVertexVar& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            જો (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                ચાલુ;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_લીટી* અચલ flp = vtx.driver_File_લીટી() ? vtx.driver_File_લીટી() : vtx.fileline();
            AstVarRef* અચલ lhsp = નવું AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = getCombActive(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            જો (DfgVertex* અચલ defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = નવું AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr};
                m_activep->addStmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_વૃક્ષ_Pure(ખોટું), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_વૃક્ષ(), lhsp);
        }

        // Update Ast References
        માટે (DfgVertexAst& vtx : dfg.astVertices()) {
            જો (DfgAstRd* અચલ rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_ગાંઠ_પદાવલિ* અચલ exprp = convert_Dfg_Vertex_To_Ast_ગાંઠ_પદાવલિ(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                જો (exprp->same_વૃક્ષ(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_વૃક્ષ(), exprp);
                    ચાલુ;
                }
                // Replace the reference with the expression
                જો (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } નહીંતર {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_વૃક્ષ();
                rVtxp->exprp(exprp);
            }
        }
    }

જાહેર:
    સ્થિર શૂન્ય apply(DfgGraph& dfg, V3DfgDfgToAstContext& ctx) { DfgToAstVisitor{dfg, ctx}; }
};

શૂન્ય V3DfgPasses::dfgToAst(DfgGraph& dfg, V3DfgContext& ctx) {
    DfgToAstVisitor::apply(dfg, ctx.m_dfg2AstContext);
}
