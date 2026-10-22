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
ढाँचा <typename T_गाठ, typename T_Vertex, typename... Ops>
T_गाठ* make_गाठ(स्थिरांक T_Vertex* vtxp, Ops... ops) {
    T_गाठ* स्थिरांक nodep = नवीन T_गाठ{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->width() == static_cast<पूर्णांक>(vtxp->width()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->width() << " vs " << vtxp->width());
    परत nodep;
}

//======================================================================
// Vertices needing special conversion

ढाँचा <>
Ast_मोजणी_Ones* make_गाठ<Ast_मोजणी_Ones, Dfg_मोजणी_Ones, Ast_गाठ_अभिव्यक्ती*>(  //
    स्थिरांक Dfg_मोजणी_Ones* vtxp, Ast_गाठ_अभिव्यक्ती* op1) {
    Ast_मोजणी_Ones* स्थिरांक nodep = नवीन Ast_मोजणी_Ones{vtxp->fileline(), op1};
    परत nodep;
}

ढाँचा <>
AstExtend* make_गाठ<AstExtend, DfgExtend, Ast_गाठ_अभिव्यक्ती*>(  //
    स्थिरांक DfgExtend* vtxp, Ast_गाठ_अभिव्यक्ती* op1) {
    परत नवीन AstExtend{vtxp->fileline(), op1, static_cast<पूर्णांक>(vtxp->width())};
}

ढाँचा <>
AstExtendS* make_गाठ<AstExtendS, DfgExtendS, Ast_गाठ_अभिव्यक्ती*>(  //
    स्थिरांक DfgExtendS* vtxp, Ast_गाठ_अभिव्यक्ती* op1) {
    परत नवीन AstExtendS{vtxp->fileline(), op1, static_cast<पूर्णांक>(vtxp->width())};
}

ढाँचा <>
AstShiftL* make_गाठ<AstShiftL, DfgShiftL, Ast_गाठ_अभिव्यक्ती*, Ast_गाठ_अभिव्यक्ती*>(  //
    स्थिरांक DfgShiftL* vtxp, Ast_गाठ_अभिव्यक्ती* op1, Ast_गाठ_अभिव्यक्ती* op2) {
    परत नवीन AstShiftL{vtxp->fileline(), op1, op2, static_cast<पूर्णांक>(vtxp->width())};
}

ढाँचा <>
AstShiftR* make_गाठ<AstShiftR, DfgShiftR, Ast_गाठ_अभिव्यक्ती*, Ast_गाठ_अभिव्यक्ती*>(  //
    स्थिरांक DfgShiftR* vtxp, Ast_गाठ_अभिव्यक्ती* op1, Ast_गाठ_अभिव्यक्ती* op2) {
    परत नवीन AstShiftR{vtxp->fileline(), op1, op2, static_cast<पूर्णांक>(vtxp->width())};
}

ढाँचा <>
AstShiftRS* make_गाठ<AstShiftRS, DfgShiftRS, Ast_गाठ_अभिव्यक्ती*, Ast_गाठ_अभिव्यक्ती*>(  //
    स्थिरांक DfgShiftRS* vtxp, Ast_गाठ_अभिव्यक्ती* op1, Ast_गाठ_अभिव्यक्ती* op2) {
    परत नवीन AstShiftRS{vtxp->fileline(), op1, op2, static_cast<पूर्णांक>(vtxp->width())};
}

}  // namespace

वर्ग DfgToAstVisitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    स्थिरांक VNUser2InUse m_user2InUse;

    // STATE
    V3DfgDfgToAstContext& m_ctx;  // The context for stats
    Ast_गाठ_अभिव्यक्ती* m_resultp = nullptr;  // The result node of the current traversal
    AstAlways* m_alwaysp = nullptr;  // Process to add assignments to, if have a default driver
    AstActive* m_activep = nullptr;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    स्थिर AstActive* getCombActive(AstScope* scopep) {
        जर (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            साठी (Ast_गाठ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* स्थिरांक activep = VN_CAST(nodep, Active);
                जर (!activep) सुरूठेवा;
                जर (activep->hasCombo()) {
                    scopep->user2p(activep);
                    थांबा;
                }
            }
            // If there isn't one, create a new one
            जर (!scopep->user2p()) {
                File_ओळ* स्थिरांक flp = scopep->fileline();
                Ast_Sen_वृक्ष* स्थिरांक senTreep
                    = नवीन Ast_Sen_वृक्ष{flp, नवीन AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* स्थिरांक activep = नवीन AstActive{flp, "", senTreep};
                activep->sen_वृक्ष_Storep(senTreep);
                scopep->addBlocksp(activep);
                scopep->user2p(activep);
            }
        }
        परत VN_AS(scopep->user2p(), Active);
    }

    Ast_गाठ_अभिव्यक्ती* convert_Dfg_Vertex_To_Ast_गाठ_अभिव्यक्ती(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->hasMultipleSinks() || vtxp->isCheaperThanLoad(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_गाठ_अभिव्यक्ती* स्थिरांक resultp = m_resultp;
        m_resultp = nullptr;
        परत resultp;
    }

    शून्य createAssignment(File_ओळ* flp, Ast_गाठ_अभिव्यक्ती* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_resultEquations;
        // Render the driver
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = convert_Dfg_Vertex_To_Ast_गाठ_अभिव्यक्ती(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_गाठ* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        जर (m_alwaysp) {
            m_alwaysp->addStmtsp(नवीन AstAssign{flp, lhsp, rhsp});
            परत;
        }

        // Otherwise create an AssignW
        AstAssignW* स्थिरांक ap = नवीन AstAssignW{flp, lhsp, rhsp};
        m_activep->addStmtsp(नवीन AstAlways{ap});
    }

    शून्य convertDriver(File_ओळ* flp, Ast_गाठ_अभिव्यक्ती* lhsp, DfgVertex* driverp) {
        जर (DfgSplicePacked* स्थिरांक sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_ओळ* dflp) {
                // Create Sel
                AstConst* स्थिरांक lsbp = नवीन AstConst{dflp, lo};
                स्थिरांक पूर्णांक width = static_cast<पूर्णांक>(src.width());
                AstSel* स्थिरांक nLhsp = नवीन AstSel{dflp, lhsp->clone_वृक्ष_Pure(असत्य), lsbp, width};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_वृक्ष(), nLhsp);
                परत असत्य;
            });
            परत;
        }

        जर (DfgSpliceArray* स्थिरांक sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_ओळ* dflp) {
                UASSERT_OBJ(src.size() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* स्थिरांक idxp = नवीन AstConst{dflp, lo};
                AstArraySel* स्थिरांक nLhsp = नवीन AstArraySel{dflp, lhsp->clone_वृक्ष_Pure(असत्य), idxp};
                // Convert source
                जर (स्थिरांक DfgUnitArray* स्थिरांक uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } अन्यथा {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_वृक्ष(), nLhsp);
                परत असत्य;
            });
            परत;
        }

        जर (स्थिरांक DfgUnitArray* स्थिरांक uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* स्थिरांक idxp = नवीन AstConst{flp, 0};
            AstArraySel* स्थिरांक nLhsp = नवीन AstArraySel{flp, lhsp->clone_वृक्ष_Pure(असत्य), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_वृक्ष(), nLhsp);
            परत;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_वृक्ष_Pure(असत्य), driverp);
    }

    // VISITORS
    शून्य visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    शून्य visit(DfgVarPacked* vtxp) override {
        m_resultp = नवीन AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    शून्य visit(DfgVarArray* vtxp) override {
        m_resultp = नवीन AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    शून्य visit(DfgPrev* vtxp) override {
        m_resultp = नवीन AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    शून्य visit(DfgConst* vtxp) override {  //
        m_resultp = नवीन AstConst{vtxp->fileline(), vtxp->अंक()};
    }
    शून्य visit(DfgCReset* vtxp) override {
        DfgVertex* स्थिरांक sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* स्थिरांक varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = नवीन AstCReset{vtxp->fileline(), varp, असत्य};
    }
    शून्य visit(DfgMatchMasked* vtxp) override {
        File_ओळ* स्थिरांक flp = vtxp->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = convert_Dfg_Vertex_To_Ast_गाठ_अभिव्यक्ती(vtxp->lhsp());
        AstVarScope* स्थिरांक matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = नवीन AstMatchMasked{flp, lhsp, नवीन AstVarRef{flp, matchp, VAccess::READ}};
    }

    शून्य visit(DfgRep* vtxp) override {
        File_ओळ* स्थिरांक flp = vtxp->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक srcp = convert_Dfg_Vertex_To_Ast_गाठ_अभिव्यक्ती(vtxp->srcp());
        m_resultp = नवीन AstReplicate{flp, srcp, vtxp->मोजणी()};
    }

    शून्य visit(DfgSel* vtxp) override {
        File_ओळ* स्थिरांक flp = vtxp->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = convert_Dfg_Vertex_To_Ast_गाठ_अभिव्यक्ती(vtxp->fromp());
        AstConst* स्थिरांक lsbp = नवीन AstConst{flp, vtxp->lsb()};
        m_resultp = नवीन AstSel{flp, fromp, lsbp, static_cast<पूर्णांक>(vtxp->width())};
    }

    शून्य visit(DfgMux* vtxp) override {
        File_ओळ* स्थिरांक flp = vtxp->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = convert_Dfg_Vertex_To_Ast_गाठ_अभिव्यक्ती(vtxp->fromp());
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lsbp = convert_Dfg_Vertex_To_Ast_गाठ_अभिव्यक्ती(vtxp->lsbp());
        m_resultp = नवीन AstSel{flp, fromp, lsbp, static_cast<पूर्णांक>(vtxp->width())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#अन्तर्भाव "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    DfgToAstVisitor(DfgGraph& dfg, V3DfgDfgToAstContext& ctx)
        : m_ctx{ctx} {
        जर (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        साठी (DfgVertexVar& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            जर (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                सुरूठेवा;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_ओळ* स्थिरांक flp = vtx.driver_File_ओळ() ? vtx.driver_File_ओळ() : vtx.fileline();
            AstVarRef* स्थिरांक lhsp = नवीन AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = getCombActive(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            जर (DfgVertex* स्थिरांक defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = नवीन AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr};
                m_activep->addStmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_वृक्ष_Pure(असत्य), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_वृक्ष(), lhsp);
        }

        // Update Ast References
        साठी (DfgVertexAst& vtx : dfg.astVertices()) {
            जर (DfgAstRd* स्थिरांक rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = convert_Dfg_Vertex_To_Ast_गाठ_अभिव्यक्ती(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                जर (exprp->same_वृक्ष(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_वृक्ष(), exprp);
                    सुरूठेवा;
                }
                // Replace the reference with the expression
                जर (VN_IS(exprp, VarRef)) {
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

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, V3DfgDfgToAstContext& ctx) { DfgToAstVisitor{dfg, ctx}; }
};

शून्य V3DfgPasses::dfgToAst(DfgGraph& dfg, V3DfgContext& ctx) {
    DfgToAstVisitor::apply(dfg, ctx.m_dfg2AstContext);
}
