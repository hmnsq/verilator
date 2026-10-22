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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Dfg.h"
#அந்தர்பாவ "V3DfgPasses.h"
#அந்தர்பாவ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ {

// Create an AstNodeExpr out of a DfgVertex. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
template <typename T_முனை, typename T_Vertex, typename... Ops>
T_முனை* make_முனை(மாறிலி T_Vertex* vtxp, Ops... ops) {
    T_முனை* மாறிலி nodep = புதிய T_முனை{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->width() == static_cast<முழு>(vtxp->width()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->width() << " vs " << vtxp->width());
    திரும்பு nodep;
}

//======================================================================
// Vertices needing special conversion

template <>
Ast_மொத்தம்_Ones* make_முனை<Ast_மொத்தம்_Ones, Dfg_மொத்தம்_Ones, Ast_முனை_கோவை*>(  //
    மாறிலி Dfg_மொத்தம்_Ones* vtxp, Ast_முனை_கோவை* op1) {
    Ast_மொத்தம்_Ones* மாறிலி nodep = புதிய Ast_மொத்தம்_Ones{vtxp->fileline(), op1};
    திரும்பு nodep;
}

template <>
AstExtend* make_முனை<AstExtend, DfgExtend, Ast_முனை_கோவை*>(  //
    மாறிலி DfgExtend* vtxp, Ast_முனை_கோவை* op1) {
    திரும்பு புதிய AstExtend{vtxp->fileline(), op1, static_cast<முழு>(vtxp->width())};
}

template <>
AstExtendS* make_முனை<AstExtendS, DfgExtendS, Ast_முனை_கோவை*>(  //
    மாறிலி DfgExtendS* vtxp, Ast_முனை_கோவை* op1) {
    திரும்பு புதிய AstExtendS{vtxp->fileline(), op1, static_cast<முழு>(vtxp->width())};
}

template <>
AstShiftL* make_முனை<AstShiftL, DfgShiftL, Ast_முனை_கோவை*, Ast_முனை_கோவை*>(  //
    மாறிலி DfgShiftL* vtxp, Ast_முனை_கோவை* op1, Ast_முனை_கோவை* op2) {
    திரும்பு புதிய AstShiftL{vtxp->fileline(), op1, op2, static_cast<முழு>(vtxp->width())};
}

template <>
AstShiftR* make_முனை<AstShiftR, DfgShiftR, Ast_முனை_கோவை*, Ast_முனை_கோவை*>(  //
    மாறிலி DfgShiftR* vtxp, Ast_முனை_கோவை* op1, Ast_முனை_கோவை* op2) {
    திரும்பு புதிய AstShiftR{vtxp->fileline(), op1, op2, static_cast<முழு>(vtxp->width())};
}

template <>
AstShiftRS* make_முனை<AstShiftRS, DfgShiftRS, Ast_முனை_கோவை*, Ast_முனை_கோவை*>(  //
    மாறிலி DfgShiftRS* vtxp, Ast_முனை_கோவை* op1, Ast_முனை_கோவை* op2) {
    திரும்பு புதிய AstShiftRS{vtxp->fileline(), op1, op2, static_cast<முழு>(vtxp->width())};
}

}  // namespace

வகுப்பு DfgToAstVisitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    மாறிலி VNUser2InUse m_user2InUse;

    // STATE
    V3DfgDfgToAstContext& m_ctx;  // The context for stats
    Ast_முனை_கோவை* m_resultp = nullptr;  // The result node of the current traversal
    AstAlways* m_alwaysp = nullptr;  // Process to add assignments to, if have a default driver
    AstActive* m_activep = nullptr;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    நிலையான AstActive* getCombActive(AstScope* scopep) {
        எனில் (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            க்கு (Ast_முனை* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* மாறிலி activep = VN_CAST(nodep, Active);
                எனில் (!activep) தொடர்;
                எனில் (activep->hasCombo()) {
                    scopep->user2p(activep);
                    நிறுத்து;
                }
            }
            // If there isn't one, create a new one
            எனில் (!scopep->user2p()) {
                File_வரி* மாறிலி flp = scopep->fileline();
                Ast_Sen_மரம்* மாறிலி senTreep
                    = புதிய Ast_Sen_மரம்{flp, புதிய AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* மாறிலி activep = புதிய AstActive{flp, "", senTreep};
                activep->sen_மரம்_Storep(senTreep);
                scopep->addBlocksp(activep);
                scopep->user2p(activep);
            }
        }
        திரும்பு VN_AS(scopep->user2p(), Active);
    }

    Ast_முனை_கோவை* convert_Dfg_Vertex_To_Ast_முனை_கோவை(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->hasMultipleSinks() || vtxp->isCheaperThanLoad(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_முனை_கோவை* மாறிலி resultp = m_resultp;
        m_resultp = nullptr;
        திரும்பு resultp;
    }

    வெறுமை createAssignment(File_வரி* flp, Ast_முனை_கோவை* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_resultEquations;
        // Render the driver
        Ast_முனை_கோவை* மாறிலி rhsp = convert_Dfg_Vertex_To_Ast_முனை_கோவை(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_முனை* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        எனில் (m_alwaysp) {
            m_alwaysp->addStmtsp(புதிய AstAssign{flp, lhsp, rhsp});
            திரும்பு;
        }

        // Otherwise create an AssignW
        AstAssignW* மாறிலி ap = புதிய AstAssignW{flp, lhsp, rhsp};
        m_activep->addStmtsp(புதிய AstAlways{ap});
    }

    வெறுமை convertDriver(File_வரி* flp, Ast_முனை_கோவை* lhsp, DfgVertex* driverp) {
        எனில் (DfgSplicePacked* மாறிலி sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_வரி* dflp) {
                // Create Sel
                AstConst* மாறிலி lsbp = புதிய AstConst{dflp, lo};
                மாறிலி முழு width = static_cast<முழு>(src.width());
                AstSel* மாறிலி nLhsp = புதிய AstSel{dflp, lhsp->clone_மரம்_Pure(பொய்), lsbp, width};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_மரம்(), nLhsp);
                திரும்பு பொய்;
            });
            திரும்பு;
        }

        எனில் (DfgSpliceArray* மாறிலி sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_வரி* dflp) {
                UASSERT_OBJ(src.size() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* மாறிலி idxp = புதிய AstConst{dflp, lo};
                AstArraySel* மாறிலி nLhsp = புதிய AstArraySel{dflp, lhsp->clone_மரம்_Pure(பொய்), idxp};
                // Convert source
                எனில் (மாறிலி DfgUnitArray* மாறிலி uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } இல்லையெனில் {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_மரம்(), nLhsp);
                திரும்பு பொய்;
            });
            திரும்பு;
        }

        எனில் (மாறிலி DfgUnitArray* மாறிலி uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* மாறிலி idxp = புதிய AstConst{flp, 0};
            AstArraySel* மாறிலி nLhsp = புதிய AstArraySel{flp, lhsp->clone_மரம்_Pure(பொய்), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_மரம்(), nLhsp);
            திரும்பு;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_மரம்_Pure(பொய்), driverp);
    }

    // VISITORS
    வெறுமை visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    வெறுமை visit(DfgVarPacked* vtxp) override {
        m_resultp = புதிய AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    வெறுமை visit(DfgVarArray* vtxp) override {
        m_resultp = புதிய AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    வெறுமை visit(DfgPrev* vtxp) override {
        m_resultp = புதிய AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    வெறுமை visit(DfgConst* vtxp) override {  //
        m_resultp = புதிய AstConst{vtxp->fileline(), vtxp->இலக்கம்()};
    }
    வெறுமை visit(DfgCReset* vtxp) override {
        DfgVertex* மாறிலி sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* மாறிலி varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = புதிய AstCReset{vtxp->fileline(), varp, பொய்};
    }
    வெறுமை visit(DfgMatchMasked* vtxp) override {
        File_வரி* மாறிலி flp = vtxp->fileline();
        Ast_முனை_கோவை* மாறிலி lhsp = convert_Dfg_Vertex_To_Ast_முனை_கோவை(vtxp->lhsp());
        AstVarScope* மாறிலி matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = புதிய AstMatchMasked{flp, lhsp, புதிய AstVarRef{flp, matchp, VAccess::READ}};
    }

    வெறுமை visit(DfgRep* vtxp) override {
        File_வரி* மாறிலி flp = vtxp->fileline();
        Ast_முனை_கோவை* மாறிலி srcp = convert_Dfg_Vertex_To_Ast_முனை_கோவை(vtxp->srcp());
        m_resultp = புதிய AstReplicate{flp, srcp, vtxp->மொத்தம்()};
    }

    வெறுமை visit(DfgSel* vtxp) override {
        File_வரி* மாறிலி flp = vtxp->fileline();
        Ast_முனை_கோவை* மாறிலி fromp = convert_Dfg_Vertex_To_Ast_முனை_கோவை(vtxp->fromp());
        AstConst* மாறிலி lsbp = புதிய AstConst{flp, vtxp->lsb()};
        m_resultp = புதிய AstSel{flp, fromp, lsbp, static_cast<முழு>(vtxp->width())};
    }

    வெறுமை visit(DfgMux* vtxp) override {
        File_வரி* மாறிலி flp = vtxp->fileline();
        Ast_முனை_கோவை* மாறிலி fromp = convert_Dfg_Vertex_To_Ast_முனை_கோவை(vtxp->fromp());
        Ast_முனை_கோவை* மாறிலி lsbp = convert_Dfg_Vertex_To_Ast_முனை_கோவை(vtxp->lsbp());
        m_resultp = புதிய AstSel{flp, fromp, lsbp, static_cast<முழு>(vtxp->width())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#அந்தர்பாவ "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    DfgToAstVisitor(DfgGraph& dfg, V3DfgDfgToAstContext& ctx)
        : m_ctx{ctx} {
        எனில் (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        க்கு (DfgVertexVar& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            எனில் (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                தொடர்;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_வரி* மாறிலி flp = vtx.driver_File_வரி() ? vtx.driver_File_வரி() : vtx.fileline();
            AstVarRef* மாறிலி lhsp = புதிய AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = getCombActive(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            எனில் (DfgVertex* மாறிலி defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = புதிய AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr};
                m_activep->addStmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_மரம்_Pure(பொய்), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_மரம்(), lhsp);
        }

        // Update Ast References
        க்கு (DfgVertexAst& vtx : dfg.astVertices()) {
            எனில் (DfgAstRd* மாறிலி rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_முனை_கோவை* மாறிலி exprp = convert_Dfg_Vertex_To_Ast_முனை_கோவை(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                எனில் (exprp->same_மரம்(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_மரம்(), exprp);
                    தொடர்;
                }
                // Replace the reference with the expression
                எனில் (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } இல்லையெனில் {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_மரம்();
                rVtxp->exprp(exprp);
            }
        }
    }

பொது:
    நிலையான வெறுமை apply(DfgGraph& dfg, V3DfgDfgToAstContext& ctx) { DfgToAstVisitor{dfg, ctx}; }
};

வெறுமை V3DfgPasses::dfgToAst(DfgGraph& dfg, V3DfgContext& ctx) {
    DfgToAstVisitor::apply(dfg, ctx.m_dfg2AstContext);
}
