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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"
#ضمن "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق {

// Create an AstNodeExpr out of a DfgVertex. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
template <typename T_Node, typename T_Vertex, typename... Ops>
T_Node* makeNode(ثابت T_Vertex* vtxp, Ops... ops) {
    T_Node* ثابت nodep = جديد T_Node{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->width() == static_cast<صحيح>(vtxp->width()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->width() << " vs " << vtxp->width());
    החזר nodep;
}

//======================================================================
// Vertices needing special conversion

template <>
AstCountOnes* makeNode<AstCountOnes, DfgCountOnes, AstNodeExpr*>(  //
    ثابت DfgCountOnes* vtxp, AstNodeExpr* op1) {
    AstCountOnes* ثابت nodep = جديد AstCountOnes{vtxp->fileline(), op1};
    החזר nodep;
}

template <>
AstExtend* makeNode<AstExtend, DfgExtend, AstNodeExpr*>(  //
    ثابت DfgExtend* vtxp, AstNodeExpr* op1) {
    החזר جديد AstExtend{vtxp->fileline(), op1, static_cast<صحيح>(vtxp->width())};
}

template <>
AstExtendS* makeNode<AstExtendS, DfgExtendS, AstNodeExpr*>(  //
    ثابت DfgExtendS* vtxp, AstNodeExpr* op1) {
    החזר جديد AstExtendS{vtxp->fileline(), op1, static_cast<صحيح>(vtxp->width())};
}

template <>
AstShiftL* makeNode<AstShiftL, DfgShiftL, AstNodeExpr*, AstNodeExpr*>(  //
    ثابت DfgShiftL* vtxp, AstNodeExpr* op1, AstNodeExpr* op2) {
    החזר جديد AstShiftL{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->width())};
}

template <>
AstShiftR* makeNode<AstShiftR, DfgShiftR, AstNodeExpr*, AstNodeExpr*>(  //
    ثابت DfgShiftR* vtxp, AstNodeExpr* op1, AstNodeExpr* op2) {
    החזר جديد AstShiftR{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->width())};
}

template <>
AstShiftRS* makeNode<AstShiftRS, DfgShiftRS, AstNodeExpr*, AstNodeExpr*>(  //
    ثابت DfgShiftRS* vtxp, AstNodeExpr* op1, AstNodeExpr* op2) {
    החזר جديد AstShiftRS{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->width())};
}

}  // namespace

מחלקה DfgToAstVisitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ثابت VNUser2InUse m_user2InUse;

    // STATE
    V3DfgDfgToAstContext& m_ctx;  // The context for stats
    AstNodeExpr* m_resultp = nullptr;  // The result node of the current traversal
    AstAlways* m_alwaysp = nullptr;  // Process to add assignments to, if have a default driver
    AstActive* m_activep = nullptr;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    ساكن AstActive* getCombActive(AstScope* scopep) {
        אם (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            עבור (AstNode* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ثابت activep = VN_CAST(nodep, Active);
                אם (!activep) استمر;
                אם (activep->hasCombo()) {
                    scopep->user2p(activep);
                    اكسر;
                }
            }
            // If there isn't one, create a new one
            אם (!scopep->user2p()) {
                FileLine* ثابت flp = scopep->fileline();
                AstSenTree* ثابت senTreep
                    = جديد AstSenTree{flp, جديد AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ثابت activep = جديد AstActive{flp, "", senTreep};
                activep->senTreeStorep(senTreep);
                scopep->addBlocksp(activep);
                scopep->user2p(activep);
            }
        }
        החזר VN_AS(scopep->user2p(), Active);
    }

    AstNodeExpr* convertDfgVertexToAstNodeExpr(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->hasMultipleSinks() || vtxp->isCheaperThanLoad(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        AstNodeExpr* ثابت resultp = m_resultp;
        m_resultp = nullptr;
        החזר resultp;
    }

    فراغ createAssignment(FileLine* flp, AstNodeExpr* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_resultEquations;
        // Render the driver
        AstNodeExpr* ثابت rhsp = convertDfgVertexToAstNodeExpr(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](AstNode* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        אם (m_alwaysp) {
            m_alwaysp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});
            החזר;
        }

        // Otherwise create an AssignW
        AstAssignW* ثابت ap = جديد AstAssignW{flp, lhsp, rhsp};
        m_activep->addStmtsp(جديد AstAlways{ap});
    }

    فراغ convertDriver(FileLine* flp, AstNodeExpr* lhsp, DfgVertex* driverp) {
        אם (DfgSplicePacked* ثابت sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, FileLine* dflp) {
                // Create Sel
                AstConst* ثابت lsbp = جديد AstConst{dflp, lo};
                ثابت صحيح width = static_cast<صحيح>(src.width());
                AstSel* ثابت nLhsp = جديد AstSel{dflp, lhsp->cloneTreePure(שקר), lsbp, width};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->deleteTree(), nLhsp);
                החזר שקר;
            });
            החזר;
        }

        אם (DfgSpliceArray* ثابت sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, FileLine* dflp) {
                UASSERT_OBJ(src.size() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ثابت idxp = جديد AstConst{dflp, lo};
                AstArraySel* ثابت nLhsp = جديد AstArraySel{dflp, lhsp->cloneTreePure(שקר), idxp};
                // Convert source
                אם (ثابت DfgUnitArray* ثابت uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } אחרת {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->deleteTree(), nLhsp);
                החזר שקר;
            });
            החזר;
        }

        אם (ثابت DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ثابت idxp = جديد AstConst{flp, 0};
            AstArraySel* ثابت nLhsp = جديد AstArraySel{flp, lhsp->cloneTreePure(שקר), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->deleteTree(), nLhsp);
            החזר;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->cloneTreePure(שקר), driverp);
    }

    // VISITORS
    فراغ visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    فراغ visit(DfgVarPacked* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ visit(DfgVarArray* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ visit(DfgPrev* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ visit(DfgConst* vtxp) override {  //
        m_resultp = جديد AstConst{vtxp->fileline(), vtxp->num()};
    }
    فراغ visit(DfgCReset* vtxp) override {
        DfgVertex* ثابت sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* ثابت varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = جديد AstCReset{vtxp->fileline(), varp, שקר};
    }
    فراغ visit(DfgMatchMasked* vtxp) override {
        FileLine* ثابت flp = vtxp->fileline();
        AstNodeExpr* ثابت lhsp = convertDfgVertexToAstNodeExpr(vtxp->lhsp());
        AstVarScope* ثابت matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = جديد AstMatchMasked{flp, lhsp, جديد AstVarRef{flp, matchp, VAccess::READ}};
    }

    فراغ visit(DfgRep* vtxp) override {
        FileLine* ثابت flp = vtxp->fileline();
        AstNodeExpr* ثابت srcp = convertDfgVertexToAstNodeExpr(vtxp->srcp());
        m_resultp = جديد AstReplicate{flp, srcp, vtxp->count()};
    }

    فراغ visit(DfgSel* vtxp) override {
        FileLine* ثابت flp = vtxp->fileline();
        AstNodeExpr* ثابت fromp = convertDfgVertexToAstNodeExpr(vtxp->fromp());
        AstConst* ثابت lsbp = جديد AstConst{flp, vtxp->lsb()};
        m_resultp = جديد AstSel{flp, fromp, lsbp, static_cast<صحيح>(vtxp->width())};
    }

    فراغ visit(DfgMux* vtxp) override {
        FileLine* ثابت flp = vtxp->fileline();
        AstNodeExpr* ثابت fromp = convertDfgVertexToAstNodeExpr(vtxp->fromp());
        AstNodeExpr* ثابت lsbp = convertDfgVertexToAstNodeExpr(vtxp->lsbp());
        m_resultp = جديد AstSel{flp, fromp, lsbp, static_cast<صحيح>(vtxp->width())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#ضمن "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    DfgToAstVisitor(DfgGraph& dfg, V3DfgDfgToAstContext& ctx)
        : m_ctx{ctx} {
        אם (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        עבור (DfgVertexVar& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            אם (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                استمر;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            FileLine* ثابت flp = vtx.driverFileLine() ? vtx.driverFileLine() : vtx.fileline();
            AstVarRef* ثابت lhsp = جديد AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = getCombActive(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            אם (DfgVertex* ثابت defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = جديد AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr};
                m_activep->addStmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->cloneTreePure(שקר), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->deleteTree(), lhsp);
        }

        // Update Ast References
        עבור (DfgVertexAst& vtx : dfg.astVertices()) {
            אם (DfgAstRd* ثابت rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                AstNodeExpr* ثابت exprp = convertDfgVertexToAstNodeExpr(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                אם (exprp->sameTree(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->deleteTree(), exprp);
                    استمر;
                }
                // Replace the reference with the expression
                אם (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } אחרת {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->deleteTree();
                rVtxp->exprp(exprp);
            }
        }
    }

عام:
    ساكن فراغ apply(DfgGraph& dfg, V3DfgDfgToAstContext& ctx) { DfgToAstVisitor{dfg, ctx}; }
};

فراغ V3DfgPasses::dfgToAst(DfgGraph& dfg, V3DfgContext& ctx) {
    DfgToAstVisitor::apply(dfg, ctx.m_dfg2AstContext);
}
