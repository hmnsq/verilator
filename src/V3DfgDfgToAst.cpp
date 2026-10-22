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
ढाँचा <typename T_गाँठो, typename T_Vertex, typename... Ops>
T_गाँठो* make_गाँठो(अचर T_Vertex* vtxp, Ops... ops) {
    T_गाँठो* अचर nodep = नूतन T_गाँठो{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->width() == static_cast<पूर्णाङ्क>(vtxp->width()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->width() << " vs " << vtxp->width());
    फर्काऊ nodep;
}

//======================================================================
// Vertices needing special conversion

ढाँचा <>
Ast_गन्ती_Ones* make_गाँठो<Ast_गन्ती_Ones, Dfg_गन्ती_Ones, Ast_गाँठो_अभिव्यक्ति*>(  //
    अचर Dfg_गन्ती_Ones* vtxp, Ast_गाँठो_अभिव्यक्ति* op1) {
    Ast_गन्ती_Ones* अचर nodep = नूतन Ast_गन्ती_Ones{vtxp->fileline(), op1};
    फर्काऊ nodep;
}

ढाँचा <>
AstExtend* make_गाँठो<AstExtend, DfgExtend, Ast_गाँठो_अभिव्यक्ति*>(  //
    अचर DfgExtend* vtxp, Ast_गाँठो_अभिव्यक्ति* op1) {
    फर्काऊ नूतन AstExtend{vtxp->fileline(), op1, static_cast<पूर्णाङ्क>(vtxp->width())};
}

ढाँचा <>
AstExtendS* make_गाँठो<AstExtendS, DfgExtendS, Ast_गाँठो_अभिव्यक्ति*>(  //
    अचर DfgExtendS* vtxp, Ast_गाँठो_अभिव्यक्ति* op1) {
    फर्काऊ नूतन AstExtendS{vtxp->fileline(), op1, static_cast<पूर्णाङ्क>(vtxp->width())};
}

ढाँचा <>
AstShiftL* make_गाँठो<AstShiftL, DfgShiftL, Ast_गाँठो_अभिव्यक्ति*, Ast_गाँठो_अभिव्यक्ति*>(  //
    अचर DfgShiftL* vtxp, Ast_गाँठो_अभिव्यक्ति* op1, Ast_गाँठो_अभिव्यक्ति* op2) {
    फर्काऊ नूतन AstShiftL{vtxp->fileline(), op1, op2, static_cast<पूर्णाङ्क>(vtxp->width())};
}

ढाँचा <>
AstShiftR* make_गाँठो<AstShiftR, DfgShiftR, Ast_गाँठो_अभिव्यक्ति*, Ast_गाँठो_अभिव्यक्ति*>(  //
    अचर DfgShiftR* vtxp, Ast_गाँठो_अभिव्यक्ति* op1, Ast_गाँठो_अभिव्यक्ति* op2) {
    फर्काऊ नूतन AstShiftR{vtxp->fileline(), op1, op2, static_cast<पूर्णाङ्क>(vtxp->width())};
}

ढाँचा <>
AstShiftRS* make_गाँठो<AstShiftRS, DfgShiftRS, Ast_गाँठो_अभिव्यक्ति*, Ast_गाँठो_अभिव्यक्ति*>(  //
    अचर DfgShiftRS* vtxp, Ast_गाँठो_अभिव्यक्ति* op1, Ast_गाँठो_अभिव्यक्ति* op2) {
    फर्काऊ नूतन AstShiftRS{vtxp->fileline(), op1, op2, static_cast<पूर्णाङ्क>(vtxp->width())};
}

}  // namespace

वर्ग DfgToAstVisitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    अचर VNUser2InUse m_user2InUse;

    // STATE
    V3DfgDfgToAstContext& m_ctx;  // The context for stats
    Ast_गाँठो_अभिव्यक्ति* m_resultp = nullptr;  // The result node of the current traversal
    AstAlways* m_alwaysp = nullptr;  // Process to add assignments to, if have a default driver
    AstActive* m_activep = nullptr;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    स्थिर AstActive* getCombActive(AstScope* scopep) {
        यदि (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            लागि (Ast_गाँठो* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* अचर activep = VN_CAST(nodep, Active);
                यदि (!activep) जारी;
                यदि (activep->hasCombo()) {
                    scopep->user2p(activep);
                    रोक;
                }
            }
            // If there isn't one, create a new one
            यदि (!scopep->user2p()) {
                File_पङ्क्ति* अचर flp = scopep->fileline();
                Ast_Sen_रूख* अचर senTreep
                    = नूतन Ast_Sen_रूख{flp, नूतन AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* अचर activep = नूतन AstActive{flp, "", senTreep};
                activep->sen_रूख_Storep(senTreep);
                scopep->addBlocksp(activep);
                scopep->user2p(activep);
            }
        }
        फर्काऊ VN_AS(scopep->user2p(), Active);
    }

    Ast_गाँठो_अभिव्यक्ति* convert_Dfg_Vertex_To_Ast_गाँठो_अभिव्यक्ति(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->hasMultipleSinks() || vtxp->isCheaperThanLoad(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_गाँठो_अभिव्यक्ति* अचर resultp = m_resultp;
        m_resultp = nullptr;
        फर्काऊ resultp;
    }

    शून्य createAssignment(File_पङ्क्ति* flp, Ast_गाँठो_अभिव्यक्ति* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_resultEquations;
        // Render the driver
        Ast_गाँठो_अभिव्यक्ति* अचर rhsp = convert_Dfg_Vertex_To_Ast_गाँठो_अभिव्यक्ति(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_गाँठो* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        यदि (m_alwaysp) {
            m_alwaysp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
            फर्काऊ;
        }

        // Otherwise create an AssignW
        AstAssignW* अचर ap = नूतन AstAssignW{flp, lhsp, rhsp};
        m_activep->addStmtsp(नूतन AstAlways{ap});
    }

    शून्य convertDriver(File_पङ्क्ति* flp, Ast_गाँठो_अभिव्यक्ति* lhsp, DfgVertex* driverp) {
        यदि (DfgSplicePacked* अचर sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_पङ्क्ति* dflp) {
                // Create Sel
                AstConst* अचर lsbp = नूतन AstConst{dflp, lo};
                अचर पूर्णाङ्क width = static_cast<पूर्णाङ्क>(src.width());
                AstSel* अचर nLhsp = नूतन AstSel{dflp, lhsp->clone_रूख_Pure(असत्य), lsbp, width};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_रूख(), nLhsp);
                फर्काऊ असत्य;
            });
            फर्काऊ;
        }

        यदि (DfgSpliceArray* अचर sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_पङ्क्ति* dflp) {
                UASSERT_OBJ(src.size() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* अचर idxp = नूतन AstConst{dflp, lo};
                AstArraySel* अचर nLhsp = नूतन AstArraySel{dflp, lhsp->clone_रूख_Pure(असत्य), idxp};
                // Convert source
                यदि (अचर DfgUnitArray* अचर uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } अन्यथा {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_रूख(), nLhsp);
                फर्काऊ असत्य;
            });
            फर्काऊ;
        }

        यदि (अचर DfgUnitArray* अचर uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* अचर idxp = नूतन AstConst{flp, 0};
            AstArraySel* अचर nLhsp = नूतन AstArraySel{flp, lhsp->clone_रूख_Pure(असत्य), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_रूख(), nLhsp);
            फर्काऊ;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_रूख_Pure(असत्य), driverp);
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
        m_resultp = नूतन AstConst{vtxp->fileline(), vtxp->अङ्क()};
    }
    शून्य visit(DfgCReset* vtxp) override {
        DfgVertex* अचर sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* अचर varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = नूतन AstCReset{vtxp->fileline(), varp, असत्य};
    }
    शून्य visit(DfgMatchMasked* vtxp) override {
        File_पङ्क्ति* अचर flp = vtxp->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर lhsp = convert_Dfg_Vertex_To_Ast_गाँठो_अभिव्यक्ति(vtxp->lhsp());
        AstVarScope* अचर matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = नूतन AstMatchMasked{flp, lhsp, नूतन AstVarRef{flp, matchp, VAccess::READ}};
    }

    शून्य visit(DfgRep* vtxp) override {
        File_पङ्क्ति* अचर flp = vtxp->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर srcp = convert_Dfg_Vertex_To_Ast_गाँठो_अभिव्यक्ति(vtxp->srcp());
        m_resultp = नूतन AstReplicate{flp, srcp, vtxp->गन्ती()};
    }

    शून्य visit(DfgSel* vtxp) override {
        File_पङ्क्ति* अचर flp = vtxp->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर fromp = convert_Dfg_Vertex_To_Ast_गाँठो_अभिव्यक्ति(vtxp->fromp());
        AstConst* अचर lsbp = नूतन AstConst{flp, vtxp->lsb()};
        m_resultp = नूतन AstSel{flp, fromp, lsbp, static_cast<पूर्णाङ्क>(vtxp->width())};
    }

    शून्य visit(DfgMux* vtxp) override {
        File_पङ्क्ति* अचर flp = vtxp->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर fromp = convert_Dfg_Vertex_To_Ast_गाँठो_अभिव्यक्ति(vtxp->fromp());
        Ast_गाँठो_अभिव्यक्ति* अचर lsbp = convert_Dfg_Vertex_To_Ast_गाँठो_अभिव्यक्ति(vtxp->lsbp());
        m_resultp = नूतन AstSel{flp, fromp, lsbp, static_cast<पूर्णाङ्क>(vtxp->width())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#अन्तर्भाव "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    DfgToAstVisitor(DfgGraph& dfg, V3DfgDfgToAstContext& ctx)
        : m_ctx{ctx} {
        यदि (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        लागि (DfgVertexVar& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            यदि (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                जारी;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_पङ्क्ति* अचर flp = vtx.driver_File_पङ्क्ति() ? vtx.driver_File_पङ्क्ति() : vtx.fileline();
            AstVarRef* अचर lhsp = नूतन AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = getCombActive(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            यदि (DfgVertex* अचर defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = नूतन AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr};
                m_activep->addStmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_रूख_Pure(असत्य), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_रूख(), lhsp);
        }

        // Update Ast References
        लागि (DfgVertexAst& vtx : dfg.astVertices()) {
            यदि (DfgAstRd* अचर rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_गाँठो_अभिव्यक्ति* अचर exprp = convert_Dfg_Vertex_To_Ast_गाँठो_अभिव्यक्ति(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                यदि (exprp->same_रूख(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_रूख(), exprp);
                    जारी;
                }
                // Replace the reference with the expression
                यदि (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } अन्यथा {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_रूख();
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
