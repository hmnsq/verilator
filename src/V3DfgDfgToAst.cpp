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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Dfg.h"
#അന്തര്ഭാവ "V3DfgPasses.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

നാമാകാശ {

// Create an AstNodeExpr out of a DfgVertex. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
template <typename T_കണ്ണി, typename T_Vertex, typename... Ops>
T_കണ്ണി* make_കണ്ണി(സ്ഥിരം T_Vertex* vtxp, Ops... ops) {
    T_കണ്ണി* സ്ഥിരം nodep = പുതിയ T_കണ്ണി{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->width() == static_cast<പൂർണ്ണസംഖ്യ>(vtxp->width()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->width() << " vs " << vtxp->width());
    തിരികെ nodep;
}

//======================================================================
// Vertices needing special conversion

template <>
Ast_എണ്ണം_Ones* make_കണ്ണി<Ast_എണ്ണം_Ones, Dfg_എണ്ണം_Ones, Ast_കണ്ണി_വ്യഞ്ജകം*>(  //
    സ്ഥിരം Dfg_എണ്ണം_Ones* vtxp, Ast_കണ്ണി_വ്യഞ്ജകം* op1) {
    Ast_എണ്ണം_Ones* സ്ഥിരം nodep = പുതിയ Ast_എണ്ണം_Ones{vtxp->fileline(), op1};
    തിരികെ nodep;
}

template <>
AstExtend* make_കണ്ണി<AstExtend, DfgExtend, Ast_കണ്ണി_വ്യഞ്ജകം*>(  //
    സ്ഥിരം DfgExtend* vtxp, Ast_കണ്ണി_വ്യഞ്ജകം* op1) {
    തിരികെ പുതിയ AstExtend{vtxp->fileline(), op1, static_cast<പൂർണ്ണസംഖ്യ>(vtxp->width())};
}

template <>
AstExtendS* make_കണ്ണി<AstExtendS, DfgExtendS, Ast_കണ്ണി_വ്യഞ്ജകം*>(  //
    സ്ഥിരം DfgExtendS* vtxp, Ast_കണ്ണി_വ്യഞ്ജകം* op1) {
    തിരികെ പുതിയ AstExtendS{vtxp->fileline(), op1, static_cast<പൂർണ്ണസംഖ്യ>(vtxp->width())};
}

template <>
AstShiftL* make_കണ്ണി<AstShiftL, DfgShiftL, Ast_കണ്ണി_വ്യഞ്ജകം*, Ast_കണ്ണി_വ്യഞ്ജകം*>(  //
    സ്ഥിരം DfgShiftL* vtxp, Ast_കണ്ണി_വ്യഞ്ജകം* op1, Ast_കണ്ണി_വ്യഞ്ജകം* op2) {
    തിരികെ പുതിയ AstShiftL{vtxp->fileline(), op1, op2, static_cast<പൂർണ്ണസംഖ്യ>(vtxp->width())};
}

template <>
AstShiftR* make_കണ്ണി<AstShiftR, DfgShiftR, Ast_കണ്ണി_വ്യഞ്ജകം*, Ast_കണ്ണി_വ്യഞ്ജകം*>(  //
    സ്ഥിരം DfgShiftR* vtxp, Ast_കണ്ണി_വ്യഞ്ജകം* op1, Ast_കണ്ണി_വ്യഞ്ജകം* op2) {
    തിരികെ പുതിയ AstShiftR{vtxp->fileline(), op1, op2, static_cast<പൂർണ്ണസംഖ്യ>(vtxp->width())};
}

template <>
AstShiftRS* make_കണ്ണി<AstShiftRS, DfgShiftRS, Ast_കണ്ണി_വ്യഞ്ജകം*, Ast_കണ്ണി_വ്യഞ്ജകം*>(  //
    സ്ഥിരം DfgShiftRS* vtxp, Ast_കണ്ണി_വ്യഞ്ജകം* op1, Ast_കണ്ണി_വ്യഞ്ജകം* op2) {
    തിരികെ പുതിയ AstShiftRS{vtxp->fileline(), op1, op2, static_cast<പൂർണ്ണസംഖ്യ>(vtxp->width())};
}

}  // namespace

വർഗ്ഗം DfgToAstVisitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    സ്ഥിരം VNUser2InUse m_user2InUse;

    // STATE
    V3DfgDfgToAstContext& m_ctx;  // The context for stats
    Ast_കണ്ണി_വ്യഞ്ജകം* m_resultp = nullptr;  // The result node of the current traversal
    AstAlways* m_alwaysp = nullptr;  // Process to add assignments to, if have a default driver
    AstActive* m_activep = nullptr;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    സ്ഥിതം AstActive* getCombActive(AstScope* scopep) {
        എങ്കിൽ (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            വേണ്ടി (Ast_കണ്ണി* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* സ്ഥിരം activep = VN_CAST(nodep, Active);
                എങ്കിൽ (!activep) തുടരുക;
                എങ്കിൽ (activep->hasCombo()) {
                    scopep->user2p(activep);
                    നിർത്തുക;
                }
            }
            // If there isn't one, create a new one
            എങ്കിൽ (!scopep->user2p()) {
                File_വരി* സ്ഥിരം flp = scopep->fileline();
                Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep
                    = പുതിയ Ast_Sen_വൃക്ഷം{flp, പുതിയ AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* സ്ഥിരം activep = പുതിയ AstActive{flp, "", senTreep};
                activep->sen_വൃക്ഷം_Storep(senTreep);
                scopep->addBlocksp(activep);
                scopep->user2p(activep);
            }
        }
        തിരികെ VN_AS(scopep->user2p(), Active);
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* convert_Dfg_Vertex_To_Ast_കണ്ണി_വ്യഞ്ജകം(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->hasMultipleSinks() || vtxp->isCheaperThanLoad(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം resultp = m_resultp;
        m_resultp = nullptr;
        തിരികെ resultp;
    }

    ശൂന്യം createAssignment(File_വരി* flp, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_resultEquations;
        // Render the driver
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = convert_Dfg_Vertex_To_Ast_കണ്ണി_വ്യഞ്ജകം(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_കണ്ണി* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        എങ്കിൽ (m_alwaysp) {
            m_alwaysp->addStmtsp(പുതിയ AstAssign{flp, lhsp, rhsp});
            തിരികെ;
        }

        // Otherwise create an AssignW
        AstAssignW* സ്ഥിരം ap = പുതിയ AstAssignW{flp, lhsp, rhsp};
        m_activep->addStmtsp(പുതിയ AstAlways{ap});
    }

    ശൂന്യം convertDriver(File_വരി* flp, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, DfgVertex* driverp) {
        എങ്കിൽ (DfgSplicePacked* സ്ഥിരം sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_വരി* dflp) {
                // Create Sel
                AstConst* സ്ഥിരം lsbp = പുതിയ AstConst{dflp, lo};
                സ്ഥിരം പൂർണ്ണസംഖ്യ width = static_cast<പൂർണ്ണസംഖ്യ>(src.width());
                AstSel* സ്ഥിരം nLhsp = പുതിയ AstSel{dflp, lhsp->clone_വൃക്ഷം_Pure(അസത്യം), lsbp, width};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_വൃക്ഷം(), nLhsp);
                തിരികെ അസത്യം;
            });
            തിരികെ;
        }

        എങ്കിൽ (DfgSpliceArray* സ്ഥിരം sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_വരി* dflp) {
                UASSERT_OBJ(src.size() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* സ്ഥിരം idxp = പുതിയ AstConst{dflp, lo};
                AstArraySel* സ്ഥിരം nLhsp = പുതിയ AstArraySel{dflp, lhsp->clone_വൃക്ഷം_Pure(അസത്യം), idxp};
                // Convert source
                എങ്കിൽ (സ്ഥിരം DfgUnitArray* സ്ഥിരം uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } അല്ലെങ്കിൽ {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_വൃക്ഷം(), nLhsp);
                തിരികെ അസത്യം;
            });
            തിരികെ;
        }

        എങ്കിൽ (സ്ഥിരം DfgUnitArray* സ്ഥിരം uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* സ്ഥിരം idxp = പുതിയ AstConst{flp, 0};
            AstArraySel* സ്ഥിരം nLhsp = പുതിയ AstArraySel{flp, lhsp->clone_വൃക്ഷം_Pure(അസത്യം), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_വൃക്ഷം(), nLhsp);
            തിരികെ;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_വൃക്ഷം_Pure(അസത്യം), driverp);
    }

    // VISITORS
    ശൂന്യം visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    ശൂന്യം visit(DfgVarPacked* vtxp) override {
        m_resultp = പുതിയ AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    ശൂന്യം visit(DfgVarArray* vtxp) override {
        m_resultp = പുതിയ AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    ശൂന്യം visit(DfgPrev* vtxp) override {
        m_resultp = പുതിയ AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    ശൂന്യം visit(DfgConst* vtxp) override {  //
        m_resultp = പുതിയ AstConst{vtxp->fileline(), vtxp->അക്കം()};
    }
    ശൂന്യം visit(DfgCReset* vtxp) override {
        DfgVertex* സ്ഥിരം sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* സ്ഥിരം varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = പുതിയ AstCReset{vtxp->fileline(), varp, അസത്യം};
    }
    ശൂന്യം visit(DfgMatchMasked* vtxp) override {
        File_വരി* സ്ഥിരം flp = vtxp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = convert_Dfg_Vertex_To_Ast_കണ്ണി_വ്യഞ്ജകം(vtxp->lhsp());
        AstVarScope* സ്ഥിരം matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = പുതിയ AstMatchMasked{flp, lhsp, പുതിയ AstVarRef{flp, matchp, VAccess::READ}};
    }

    ശൂന്യം visit(DfgRep* vtxp) override {
        File_വരി* സ്ഥിരം flp = vtxp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം srcp = convert_Dfg_Vertex_To_Ast_കണ്ണി_വ്യഞ്ജകം(vtxp->srcp());
        m_resultp = പുതിയ AstReplicate{flp, srcp, vtxp->എണ്ണം()};
    }

    ശൂന്യം visit(DfgSel* vtxp) override {
        File_വരി* സ്ഥിരം flp = vtxp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = convert_Dfg_Vertex_To_Ast_കണ്ണി_വ്യഞ്ജകം(vtxp->fromp());
        AstConst* സ്ഥിരം lsbp = പുതിയ AstConst{flp, vtxp->lsb()};
        m_resultp = പുതിയ AstSel{flp, fromp, lsbp, static_cast<പൂർണ്ണസംഖ്യ>(vtxp->width())};
    }

    ശൂന്യം visit(DfgMux* vtxp) override {
        File_വരി* സ്ഥിരം flp = vtxp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = convert_Dfg_Vertex_To_Ast_കണ്ണി_വ്യഞ്ജകം(vtxp->fromp());
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lsbp = convert_Dfg_Vertex_To_Ast_കണ്ണി_വ്യഞ്ജകം(vtxp->lsbp());
        m_resultp = പുതിയ AstSel{flp, fromp, lsbp, static_cast<പൂർണ്ണസംഖ്യ>(vtxp->width())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#അന്തര്ഭാവ "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    DfgToAstVisitor(DfgGraph& dfg, V3DfgDfgToAstContext& ctx)
        : m_ctx{ctx} {
        എങ്കിൽ (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        വേണ്ടി (DfgVertexVar& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            എങ്കിൽ (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                തുടരുക;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_വരി* സ്ഥിരം flp = vtx.driver_File_വരി() ? vtx.driver_File_വരി() : vtx.fileline();
            AstVarRef* സ്ഥിരം lhsp = പുതിയ AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = getCombActive(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            എങ്കിൽ (DfgVertex* സ്ഥിരം defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = പുതിയ AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr};
                m_activep->addStmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_വൃക്ഷം_Pure(അസത്യം), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_വൃക്ഷം(), lhsp);
        }

        // Update Ast References
        വേണ്ടി (DfgVertexAst& vtx : dfg.astVertices()) {
            എങ്കിൽ (DfgAstRd* സ്ഥിരം rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = convert_Dfg_Vertex_To_Ast_കണ്ണി_വ്യഞ്ജകം(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                എങ്കിൽ (exprp->same_വൃക്ഷം(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_വൃക്ഷം(), exprp);
                    തുടരുക;
                }
                // Replace the reference with the expression
                എങ്കിൽ (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } അല്ലെങ്കിൽ {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_വൃക്ഷം();
                rVtxp->exprp(exprp);
            }
        }
    }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(DfgGraph& dfg, V3DfgDfgToAstContext& ctx) { DfgToAstVisitor{dfg, ctx}; }
};

ശൂന്യം V3DfgPasses::dfgToAst(DfgGraph& dfg, V3DfgContext& ctx) {
    DfgToAstVisitor::apply(dfg, ctx.m_dfg2AstContext);
}
