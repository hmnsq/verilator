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
प्रतिरूपम् <प्रकारनाम T_ग्रन्थिः, प्रकारनाम T_Vertex, प्रकारनाम... Ops>
T_ग्रन्थिः* make_ग्रन्थिः(ध्रुव T_Vertex* vtxp, Ops... ops) {
    T_ग्रन्थिः* ध्रुव nodep = नूतन T_ग्रन्थिः{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->विस्तारः() == static_cast<पूर्णाङ्क>(vtxp->विस्तारः()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->विस्तारः() << " vs " << vtxp->विस्तारः());
    प्रत्यागम nodep;
}

//======================================================================
// Vertices needing special conversion

प्रतिरूपम् <>
Ast_गणनम्_Ones* make_ग्रन्थिः<Ast_गणनम्_Ones, Dfg_गणनम्_Ones, Ast_ग्रन्थिः_व्यञ्जकम्*>(  //
    ध्रुव Dfg_गणनम्_Ones* vtxp, Ast_ग्रन्थिः_व्यञ्जकम्* op1) {
    Ast_गणनम्_Ones* ध्रुव nodep = नूतन Ast_गणनम्_Ones{vtxp->fileline(), op1};
    प्रत्यागम nodep;
}

प्रतिरूपम् <>
AstExtend* make_ग्रन्थिः<AstExtend, DfgExtend, Ast_ग्रन्थिः_व्यञ्जकम्*>(  //
    ध्रुव DfgExtend* vtxp, Ast_ग्रन्थिः_व्यञ्जकम्* op1) {
    प्रत्यागम नूतन AstExtend{vtxp->fileline(), op1, static_cast<पूर्णाङ्क>(vtxp->विस्तारः())};
}

प्रतिरूपम् <>
AstExtendS* make_ग्रन्थिः<AstExtendS, DfgExtendS, Ast_ग्रन्थिः_व्यञ्जकम्*>(  //
    ध्रुव DfgExtendS* vtxp, Ast_ग्रन्थिः_व्यञ्जकम्* op1) {
    प्रत्यागम नूतन AstExtendS{vtxp->fileline(), op1, static_cast<पूर्णाङ्क>(vtxp->विस्तारः())};
}

प्रतिरूपम् <>
Ast_सरणम्_L* make_ग्रन्थिः<Ast_सरणम्_L, Dfg_सरणम्_L, Ast_ग्रन्थिः_व्यञ्जकम्*, Ast_ग्रन्थिः_व्यञ्जकम्*>(  //
    ध्रुव Dfg_सरणम्_L* vtxp, Ast_ग्रन्थिः_व्यञ्जकम्* op1, Ast_ग्रन्थिः_व्यञ्जकम्* op2) {
    प्रत्यागम नूतन Ast_सरणम्_L{vtxp->fileline(), op1, op2, static_cast<पूर्णाङ्क>(vtxp->विस्तारः())};
}

प्रतिरूपम् <>
Ast_सरणम्_R* make_ग्रन्थिः<Ast_सरणम्_R, Dfg_सरणम्_R, Ast_ग्रन्थिः_व्यञ्जकम्*, Ast_ग्रन्थिः_व्यञ्जकम्*>(  //
    ध्रुव Dfg_सरणम्_R* vtxp, Ast_ग्रन्थिः_व्यञ्जकम्* op1, Ast_ग्रन्थिः_व्यञ्जकम्* op2) {
    प्रत्यागम नूतन Ast_सरणम्_R{vtxp->fileline(), op1, op2, static_cast<पूर्णाङ्क>(vtxp->विस्तारः())};
}

प्रतिरूपम् <>
Ast_सरणम्_RS* make_ग्रन्थिः<Ast_सरणम्_RS, Dfg_सरणम्_RS, Ast_ग्रन्थिः_व्यञ्जकम्*, Ast_ग्रन्थिः_व्यञ्जकम्*>(  //
    ध्रुव Dfg_सरणम्_RS* vtxp, Ast_ग्रन्थिः_व्यञ्जकम्* op1, Ast_ग्रन्थिः_व्यञ्जकम्* op2) {
    प्रत्यागम नूतन Ast_सरणम्_RS{vtxp->fileline(), op1, op2, static_cast<पूर्णाङ्क>(vtxp->विस्तारः())};
}

}  // namespace

वर्ग Dfg_गन्तव्यम्_Ast_Visitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ध्रुव VNUser2_अन्तर्_Use m_user2_अन्तर्_Use;

    // STATE
    V3_Dfg_Dfg_गन्तव्यम्_Ast_Context& m_ctx;  // The context for stats
    Ast_ग्रन्थिः_व्यञ्जकम्* m_resultp = शून्यसूचकः;  // The result node of the current traversal
    AstAlways* m_alwaysp = शून्यसूचकः;  // Process to add assignments to, if have a default driver
    Ast_सक्रिय* m_activep = शून्यसूचकः;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    स्थिर Ast_सक्रिय* get_Comb_सक्रिय(AstScope* scopep) {
        यदि (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            प्रति (Ast_ग्रन्थिः* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_सक्रिय* ध्रुव activep = VN_CAST(nodep, सक्रिय);
                यदि (!activep) अनुवर्त;
                यदि (activep->hasCombo()) {
                    scopep->user2p(activep);
                    भङ्ग;
                }
            }
            // If there isn't one, create a new one
            यदि (!scopep->user2p()) {
                File_पङ्क्तिः* ध्रुव flp = scopep->fileline();
                Ast_Sen_वृक्षः* ध्रुव senTreep
                    = नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, AstSenItem::Combo{}}};
                Ast_सक्रिय* ध्रुव activep = नूतन Ast_सक्रिय{flp, "", senTreep};
                activep->sen_वृक्षः_Storep(senTreep);
                scopep->सङ्कलय_Blocksp(activep);
                scopep->user2p(activep);
            }
        }
        प्रत्यागम VN_AS(scopep->user2p(), सक्रिय);
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* convert_Dfg_Vertex_गन्तव्यम्_Ast_ग्रन्थिः_व्यञ्जकम्(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->has_गुणजः_Sinks() || vtxp->is_Cheaper_Than_आरोपय(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव resultp = m_resultp;
        m_resultp = शून्यसूचकः;
        प्रत्यागम resultp;
    }

    शून्य createAssignment(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_परिणाम_Equations;
        // Render the driver
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = convert_Dfg_Vertex_गन्तव्यम्_Ast_ग्रन्थिः_व्यञ्जकम्(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_ग्रन्थिः* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        यदि (m_alwaysp) {
            m_alwaysp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, lhsp, rhsp});
            प्रत्यागम;
        }

        // Otherwise create an AssignW
        Ast_नियोजय_W* ध्रुव ap = नूतन Ast_नियोजय_W{flp, lhsp, rhsp};
        m_activep->सङ्कलय_Stmtsp(नूतन AstAlways{ap});
    }

    शून्य convertDriver(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, DfgVertex* driverp) {
        यदि (DfgSplicePacked* ध्रुव sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_पङ्क्तिः* dflp) {
                // Create Sel
                AstConst* ध्रुव lsbp = नूतन AstConst{dflp, lo};
                ध्रुव पूर्णाङ्क विस्तारः = static_cast<पूर्णाङ्क>(src.विस्तारः());
                AstSel* ध्रुव nLhsp = नूतन AstSel{dflp, lhsp->clone_वृक्षः_विशुद्धम्(असत्य), lsbp, विस्तारः};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_वृक्षः(), nLhsp);
                प्रत्यागम असत्य;
            });
            प्रत्यागम;
        }

        यदि (DfgSpliceArray* ध्रुव sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_पङ्क्तिः* dflp) {
                UASSERT_OBJ(src.आकार() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ध्रुव idxp = नूतन AstConst{dflp, lo};
                AstArraySel* ध्रुव nLhsp = नूतन AstArraySel{dflp, lhsp->clone_वृक्षः_विशुद्धम्(असत्य), idxp};
                // Convert source
                यदि (ध्रुव DfgUnitArray* ध्रुव uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } अन्यथा {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_वृक्षः(), nLhsp);
                प्रत्यागम असत्य;
            });
            प्रत्यागम;
        }

        यदि (ध्रुव DfgUnitArray* ध्रुव uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ध्रुव idxp = नूतन AstConst{flp, 0};
            AstArraySel* ध्रुव nLhsp = नूतन AstArraySel{flp, lhsp->clone_वृक्षः_विशुद्धम्(असत्य), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_वृक्षः(), nLhsp);
            प्रत्यागम;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_वृक्षः_विशुद्धम्(असत्य), driverp);
    }

    // VISITORS
    शून्य दर्शय(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    शून्य दर्शय(DfgVarPacked* vtxp) override {
        m_resultp = नूतन AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    शून्य दर्शय(DfgVarArray* vtxp) override {
        m_resultp = नूतन AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    शून्य दर्शय(Dfg_पूर्व* vtxp) override {
        m_resultp = नूतन AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    शून्य दर्शय(DfgConst* vtxp) override {  //
        m_resultp = नूतन AstConst{vtxp->fileline(), vtxp->अङ्कः()};
    }
    शून्य दर्शय(DfgCReset* vtxp) override {
        DfgVertex* ध्रुव sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* ध्रुव varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = नूतन AstCReset{vtxp->fileline(), varp, असत्य};
    }
    शून्य दर्शय(Dfg_मेलनम्_Masked* vtxp) override {
        File_पङ्क्तिः* ध्रुव flp = vtxp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = convert_Dfg_Vertex_गन्तव्यम्_Ast_ग्रन्थिः_व्यञ्जकम्(vtxp->lhsp());
        AstVarScope* ध्रुव matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = नूतन Ast_मेलनम्_Masked{flp, lhsp, नूतन AstVarRef{flp, matchp, VAccess::READ}};
    }

    शून्य दर्शय(DfgRep* vtxp) override {
        File_पङ्क्तिः* ध्रुव flp = vtxp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव srcp = convert_Dfg_Vertex_गन्तव्यम्_Ast_ग्रन्थिः_व्यञ्जकम्(vtxp->srcp());
        m_resultp = नूतन AstReplicate{flp, srcp, vtxp->गणनम्()};
    }

    शून्य दर्शय(DfgSel* vtxp) override {
        File_पङ्क्तिः* ध्रुव flp = vtxp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = convert_Dfg_Vertex_गन्तव्यम्_Ast_ग्रन्थिः_व्यञ्जकम्(vtxp->fromp());
        AstConst* ध्रुव lsbp = नूतन AstConst{flp, vtxp->lsb()};
        m_resultp = नूतन AstSel{flp, fromp, lsbp, static_cast<पूर्णाङ्क>(vtxp->विस्तारः())};
    }

    शून्य दर्शय(DfgMux* vtxp) override {
        File_पङ्क्तिः* ध्रुव flp = vtxp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = convert_Dfg_Vertex_गन्तव्यम्_Ast_ग्रन्थिः_व्यञ्जकम्(vtxp->fromp());
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lsbp = convert_Dfg_Vertex_गन्तव्यम्_Ast_ग्रन्थिः_व्यञ्जकम्(vtxp->lsbp());
        m_resultp = नूतन AstSel{flp, fromp, lsbp, static_cast<पूर्णाङ्क>(vtxp->विस्तारः())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#अन्तर्भाव "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    Dfg_गन्तव्यम्_Ast_Visitor(DfgGraph& dfg, V3_Dfg_Dfg_गन्तव्यम्_Ast_Context& ctx)
        : m_ctx{ctx} {
        यदि (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        प्रति (DfgVertexVar& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            यदि (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                अनुवर्त;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_पङ्क्तिः* ध्रुव flp = vtx.driver_File_पङ्क्तिः() ? vtx.driver_File_पङ्क्तिः() : vtx.fileline();
            AstVarRef* ध्रुव lhsp = नूतन AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = get_Comb_सक्रिय(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            यदि (DfgVertex* ध्रुव defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = नूतन AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, शून्यसूचकः};
                m_activep->सङ्कलय_Stmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_वृक्षः_विशुद्धम्(असत्य), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_वृक्षः(), lhsp);
        }

        // Update Ast References
        प्रति (DfgVertexAst& vtx : dfg.astVertices()) {
            यदि (DfgAstRd* ध्रुव rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = convert_Dfg_Vertex_गन्तव्यम्_Ast_ग्रन्थिः_व्यञ्जकम्(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                यदि (exprp->same_वृक्षः(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_वृक्षः(), exprp);
                    अनुवर्त;
                }
                // Replace the reference with the expression
                यदि (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } अन्यथा {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_वृक्षः();
                rVtxp->exprp(exprp);
            }
        }
    }

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, V3_Dfg_Dfg_गन्तव्यम्_Ast_Context& ctx) { Dfg_गन्तव्यम्_Ast_Visitor{dfg, ctx}; }
};

शून्य V3DfgPasses::dfg_गन्तव्यम्_Ast(DfgGraph& dfg, V3DfgContext& ctx) {
    Dfg_गन्तव्यम्_Ast_Visitor::apply(dfg, ctx.m_dfg2AstContext);
}
