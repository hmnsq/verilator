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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Dfg.h"
#समावेश "V3DfgPasses.h"
#समावेश "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

namespace {

// Create an AstNodeExpr out of a DfgVertex. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
ढाँचा <typename T_गाँठ, typename T_Vertex, typename... Ops>
T_गाँठ* make_गाँठ(स्थिर T_Vertex* vtxp, Ops... ops) {
    T_गाँठ* स्थिर nodep = नया T_गाँठ{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->width() == static_cast<पूर्णांक>(vtxp->width()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->width() << " vs " << vtxp->width());
    वापस nodep;
}

//======================================================================
// Vertices needing special conversion

ढाँचा <>
Ast_गिनती_Ones* make_गाँठ<Ast_गिनती_Ones, Dfg_गिनती_Ones, Ast_गाँठ_व्यंजक*>(  //
    स्थिर Dfg_गिनती_Ones* vtxp, Ast_गाँठ_व्यंजक* op1) {
    Ast_गिनती_Ones* स्थिर nodep = नया Ast_गिनती_Ones{vtxp->fileline(), op1};
    वापस nodep;
}

ढाँचा <>
AstExtend* make_गाँठ<AstExtend, DfgExtend, Ast_गाँठ_व्यंजक*>(  //
    स्थिर DfgExtend* vtxp, Ast_गाँठ_व्यंजक* op1) {
    वापस नया AstExtend{vtxp->fileline(), op1, static_cast<पूर्णांक>(vtxp->width())};
}

ढाँचा <>
AstExtendS* make_गाँठ<AstExtendS, DfgExtendS, Ast_गाँठ_व्यंजक*>(  //
    स्थिर DfgExtendS* vtxp, Ast_गाँठ_व्यंजक* op1) {
    वापस नया AstExtendS{vtxp->fileline(), op1, static_cast<पूर्णांक>(vtxp->width())};
}

ढाँचा <>
AstShiftL* make_गाँठ<AstShiftL, DfgShiftL, Ast_गाँठ_व्यंजक*, Ast_गाँठ_व्यंजक*>(  //
    स्थिर DfgShiftL* vtxp, Ast_गाँठ_व्यंजक* op1, Ast_गाँठ_व्यंजक* op2) {
    वापस नया AstShiftL{vtxp->fileline(), op1, op2, static_cast<पूर्णांक>(vtxp->width())};
}

ढाँचा <>
AstShiftR* make_गाँठ<AstShiftR, DfgShiftR, Ast_गाँठ_व्यंजक*, Ast_गाँठ_व्यंजक*>(  //
    स्थिर DfgShiftR* vtxp, Ast_गाँठ_व्यंजक* op1, Ast_गाँठ_व्यंजक* op2) {
    वापस नया AstShiftR{vtxp->fileline(), op1, op2, static_cast<पूर्णांक>(vtxp->width())};
}

ढाँचा <>
AstShiftRS* make_गाँठ<AstShiftRS, DfgShiftRS, Ast_गाँठ_व्यंजक*, Ast_गाँठ_व्यंजक*>(  //
    स्थिर DfgShiftRS* vtxp, Ast_गाँठ_व्यंजक* op1, Ast_गाँठ_व्यंजक* op2) {
    वापस नया AstShiftRS{vtxp->fileline(), op1, op2, static_cast<पूर्णांक>(vtxp->width())};
}

}  // namespace

श्रेणी DfgToAstVisitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    स्थिर VNUser2InUse m_user2InUse;

    // STATE
    V3DfgDfgToAstContext& m_ctx;  // The context for stats
    Ast_गाँठ_व्यंजक* m_resultp = nullptr;  // The result node of the current traversal
    AstAlways* m_alwaysp = nullptr;  // Process to add assignments to, if have a default driver
    AstActive* m_activep = nullptr;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    अटल AstActive* getCombActive(AstScope* scopep) {
        अगर (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            क्रम (Ast_गाँठ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* स्थिर activep = VN_CAST(nodep, Active);
                अगर (!activep) जारी;
                अगर (activep->hasCombo()) {
                    scopep->user2p(activep);
                    अवरोध;
                }
            }
            // If there isn't one, create a new one
            अगर (!scopep->user2p()) {
                File_पंक्ति* स्थिर flp = scopep->fileline();
                Ast_Sen_वृक्ष* स्थिर senTreep
                    = नया Ast_Sen_वृक्ष{flp, नया AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* स्थिर activep = नया AstActive{flp, "", senTreep};
                activep->sen_वृक्ष_Storep(senTreep);
                scopep->addBlocksp(activep);
                scopep->user2p(activep);
            }
        }
        वापस VN_AS(scopep->user2p(), Active);
    }

    Ast_गाँठ_व्यंजक* convert_Dfg_Vertex_To_Ast_गाँठ_व्यंजक(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->hasMultipleSinks() || vtxp->isCheaperThanLoad(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_गाँठ_व्यंजक* स्थिर resultp = m_resultp;
        m_resultp = nullptr;
        वापस resultp;
    }

    व्योम createAssignment(File_पंक्ति* flp, Ast_गाँठ_व्यंजक* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_परिणाम_Equations;
        // Render the driver
        Ast_गाँठ_व्यंजक* स्थिर rhsp = convert_Dfg_Vertex_To_Ast_गाँठ_व्यंजक(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_गाँठ* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        अगर (m_alwaysp) {
            m_alwaysp->addStmtsp(नया AstAssign{flp, lhsp, rhsp});
            वापस;
        }

        // Otherwise create an AssignW
        AstAssignW* स्थिर ap = नया AstAssignW{flp, lhsp, rhsp};
        m_activep->addStmtsp(नया AstAlways{ap});
    }

    व्योम convertDriver(File_पंक्ति* flp, Ast_गाँठ_व्यंजक* lhsp, DfgVertex* driverp) {
        अगर (DfgSplicePacked* स्थिर sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_पंक्ति* dflp) {
                // Create Sel
                AstConst* स्थिर lsbp = नया AstConst{dflp, lo};
                स्थिर पूर्णांक width = static_cast<पूर्णांक>(src.width());
                AstSel* स्थिर nLhsp = नया AstSel{dflp, lhsp->clone_वृक्ष_Pure(false), lsbp, width};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_वृक्ष(), nLhsp);
                वापस false;
            });
            वापस;
        }

        अगर (DfgSpliceArray* स्थिर sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_पंक्ति* dflp) {
                UASSERT_OBJ(src.आकार() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* स्थिर idxp = नया AstConst{dflp, lo};
                AstArraySel* स्थिर nLhsp = नया AstArraySel{dflp, lhsp->clone_वृक्ष_Pure(false), idxp};
                // Convert source
                अगर (स्थिर DfgUnitArray* स्थिर uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } अन्यथा {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_वृक्ष(), nLhsp);
                वापस false;
            });
            वापस;
        }

        अगर (स्थिर DfgUnitArray* स्थिर uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* स्थिर idxp = नया AstConst{flp, 0};
            AstArraySel* स्थिर nLhsp = नया AstArraySel{flp, lhsp->clone_वृक्ष_Pure(false), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_वृक्ष(), nLhsp);
            वापस;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_वृक्ष_Pure(false), driverp);
    }

    // VISITORS
    व्योम visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    व्योम visit(DfgVarPacked* vtxp) override {
        m_resultp = नया AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    व्योम visit(DfgVarArray* vtxp) override {
        m_resultp = नया AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    व्योम visit(Dfg_पूर्व* vtxp) override {
        m_resultp = नया AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    व्योम visit(DfgConst* vtxp) override {  //
        m_resultp = नया AstConst{vtxp->fileline(), vtxp->अंक()};
    }
    व्योम visit(DfgCReset* vtxp) override {
        DfgVertex* स्थिर sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* स्थिर varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = नया AstCReset{vtxp->fileline(), varp, false};
    }
    व्योम visit(DfgMatchMasked* vtxp) override {
        File_पंक्ति* स्थिर flp = vtxp->fileline();
        Ast_गाँठ_व्यंजक* स्थिर lhsp = convert_Dfg_Vertex_To_Ast_गाँठ_व्यंजक(vtxp->lhsp());
        AstVarScope* स्थिर matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = नया AstMatchMasked{flp, lhsp, नया AstVarRef{flp, matchp, VAccess::READ}};
    }

    व्योम visit(DfgRep* vtxp) override {
        File_पंक्ति* स्थिर flp = vtxp->fileline();
        Ast_गाँठ_व्यंजक* स्थिर srcp = convert_Dfg_Vertex_To_Ast_गाँठ_व्यंजक(vtxp->srcp());
        m_resultp = नया AstReplicate{flp, srcp, vtxp->गिनती()};
    }

    व्योम visit(DfgSel* vtxp) override {
        File_पंक्ति* स्थिर flp = vtxp->fileline();
        Ast_गाँठ_व्यंजक* स्थिर fromp = convert_Dfg_Vertex_To_Ast_गाँठ_व्यंजक(vtxp->fromp());
        AstConst* स्थिर lsbp = नया AstConst{flp, vtxp->lsb()};
        m_resultp = नया AstSel{flp, fromp, lsbp, static_cast<पूर्णांक>(vtxp->width())};
    }

    व्योम visit(DfgMux* vtxp) override {
        File_पंक्ति* स्थिर flp = vtxp->fileline();
        Ast_गाँठ_व्यंजक* स्थिर fromp = convert_Dfg_Vertex_To_Ast_गाँठ_व्यंजक(vtxp->fromp());
        Ast_गाँठ_व्यंजक* स्थिर lsbp = convert_Dfg_Vertex_To_Ast_गाँठ_व्यंजक(vtxp->lsbp());
        m_resultp = नया AstSel{flp, fromp, lsbp, static_cast<पूर्णांक>(vtxp->width())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#समावेश "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    DfgToAstVisitor(DfgGraph& dfg, V3DfgDfgToAstContext& ctx)
        : m_ctx{ctx} {
        अगर (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        क्रम (DfgVertexVar& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            अगर (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                जारी;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_पंक्ति* स्थिर flp = vtx.driver_File_पंक्ति() ? vtx.driver_File_पंक्ति() : vtx.fileline();
            AstVarRef* स्थिर lhsp = नया AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = getCombActive(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            अगर (DfgVertex* स्थिर defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = नया AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr};
                m_activep->addStmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_वृक्ष_Pure(false), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_वृक्ष(), lhsp);
        }

        // Update Ast References
        क्रम (DfgVertexAst& vtx : dfg.astVertices()) {
            अगर (DfgAstRd* स्थिर rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_गाँठ_व्यंजक* स्थिर exprp = convert_Dfg_Vertex_To_Ast_गाँठ_व्यंजक(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                अगर (exprp->same_वृक्ष(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_वृक्ष(), exprp);
                    जारी;
                }
                // Replace the reference with the expression
                अगर (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } अन्यथा {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_वृक्ष();
                rVtxp->exprp(exprp);
            }
        }
    }

खुला:
    अटल व्योम apply(DfgGraph& dfg, V3DfgDfgToAstContext& ctx) { DfgToAstVisitor{dfg, ctx}; }
};

व्योम V3DfgPasses::dfgToAst(DfgGraph& dfg, V3DfgContext& ctx) {
    DfgToAstVisitor::apply(dfg, ctx.m_dfg2AstContext);
}
