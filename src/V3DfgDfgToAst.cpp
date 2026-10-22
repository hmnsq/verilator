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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3DfgPasses.h"
#అన్తర్భావ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ {

// Create an AstNodeExpr out of a DfgVertex. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
మూస <రకం_పేరు T_కణుపు, రకం_పేరు T_శీర్షం, రకం_పేరు... Ops>
T_కణుపు* make_కణుపు(స్థిరరాశి T_శీర్షం* vtxp, Ops... ops) {
    T_కణుపు* స్థిరరాశి nodep = కొత్త T_కణుపు{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->వెడల్పు() == static_cast<పూర్ణాంకం>(vtxp->వెడల్పు()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->వెడల్పు() << " vs " << vtxp->వెడల్పు());
    తిరిగి nodep;
}

//======================================================================
// Vertices needing special conversion

మూస <>
Ast_లెక్క_Ones* make_కణుపు<Ast_లెక్క_Ones, Dfg_లెక్క_Ones, Ast_కణుపు_సమాసం*>(  //
    స్థిరరాశి Dfg_లెక్క_Ones* vtxp, Ast_కణుపు_సమాసం* op1) {
    Ast_లెక్క_Ones* స్థిరరాశి nodep = కొత్త Ast_లెక్క_Ones{vtxp->fileline(), op1};
    తిరిగి nodep;
}

మూస <>
AstExtend* make_కణుపు<AstExtend, DfgExtend, Ast_కణుపు_సమాసం*>(  //
    స్థిరరాశి DfgExtend* vtxp, Ast_కణుపు_సమాసం* op1) {
    తిరిగి కొత్త AstExtend{vtxp->fileline(), op1, static_cast<పూర్ణాంకం>(vtxp->వెడల్పు())};
}

మూస <>
AstExtendS* make_కణుపు<AstExtendS, DfgExtendS, Ast_కణుపు_సమాసం*>(  //
    స్థిరరాశి DfgExtendS* vtxp, Ast_కణుపు_సమాసం* op1) {
    తిరిగి కొత్త AstExtendS{vtxp->fileline(), op1, static_cast<పూర్ణాంకం>(vtxp->వెడల్పు())};
}

మూస <>
Ast_జరుపు_L* make_కణుపు<Ast_జరుపు_L, Dfg_జరుపు_L, Ast_కణుపు_సమాసం*, Ast_కణుపు_సమాసం*>(  //
    స్థిరరాశి Dfg_జరుపు_L* vtxp, Ast_కణుపు_సమాసం* op1, Ast_కణుపు_సమాసం* op2) {
    తిరిగి కొత్త Ast_జరుపు_L{vtxp->fileline(), op1, op2, static_cast<పూర్ణాంకం>(vtxp->వెడల్పు())};
}

మూస <>
Ast_జరుపు_R* make_కణుపు<Ast_జరుపు_R, Dfg_జరుపు_R, Ast_కణుపు_సమాసం*, Ast_కణుపు_సమాసం*>(  //
    స్థిరరాశి Dfg_జరుపు_R* vtxp, Ast_కణుపు_సమాసం* op1, Ast_కణుపు_సమాసం* op2) {
    తిరిగి కొత్త Ast_జరుపు_R{vtxp->fileline(), op1, op2, static_cast<పూర్ణాంకం>(vtxp->వెడల్పు())};
}

మూస <>
Ast_జరుపు_RS* make_కణుపు<Ast_జరుపు_RS, Dfg_జరుపు_RS, Ast_కణుపు_సమాసం*, Ast_కణుపు_సమాసం*>(  //
    స్థిరరాశి Dfg_జరుపు_RS* vtxp, Ast_కణుపు_సమాసం* op1, Ast_కణుపు_సమాసం* op2) {
    తిరిగి కొత్త Ast_జరుపు_RS{vtxp->fileline(), op1, op2, static_cast<పూర్ణాంకం>(vtxp->వెడల్పు())};
}

}  // namespace

తరగతి Dfg_గమ్యం_Ast_Visitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    స్థిరరాశి VNUser2_లోపల_Use m_user2_లోపల_Use;

    // STATE
    V3_Dfg_Dfg_గమ్యం_Ast_Context& m_ctx;  // The context for stats
    Ast_కణుపు_సమాసం* m_resultp = శూన్యసూచిక;  // The result node of the current traversal
    AstAlways* m_alwaysp = శూన్యసూచిక;  // Process to add assignments to, if have a default driver
    Ast_చురుకైన* m_activep = శూన్యసూచిక;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    స్థిర Ast_చురుకైన* get_Comb_చురుకైన(AstScope* scopep) {
        ఒకవేళ (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            కొరకు (Ast_కణుపు* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_చురుకైన* స్థిరరాశి activep = VN_CAST(nodep, చురుకైన);
                ఒకవేళ (!activep) కొనసాగు;
                ఒకవేళ (activep->hasCombo()) {
                    scopep->user2p(activep);
                    ఆపు;
                }
            }
            // If there isn't one, create a new one
            ఒకవేళ (!scopep->user2p()) {
                File_పంక్తి* స్థిరరాశి flp = scopep->fileline();
                Ast_Sen_వృక్షం* స్థిరరాశి senTreep
                    = కొత్త Ast_Sen_వృక్షం{flp, కొత్త Ast_Sen_అంశం{flp, Ast_Sen_అంశం::Combo{}}};
                Ast_చురుకైన* స్థిరరాశి activep = కొత్త Ast_చురుకైన{flp, "", senTreep};
                activep->sen_వృక్షం_Storep(senTreep);
                scopep->కూడు_Blocksp(activep);
                scopep->user2p(activep);
            }
        }
        తిరిగి VN_AS(scopep->user2p(), చురుకైన);
    }

    Ast_కణుపు_సమాసం* convert_Dfg_శీర్షం_గమ్యం_Ast_కణుపు_సమాసం(Dfg_శీర్షం* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<Dfg_శీర్షం_Var>() || vtxp->is<DfgConst>()  //
                        || !vtxp->has_గుణిజం_Sinks() || vtxp->is_Cheaper_Than_ఎక్కించు(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_కణుపు_సమాసం* స్థిరరాశి resultp = m_resultp;
        m_resultp = శూన్యసూచిక;
        తిరిగి resultp;
    }

    శూన్య createAssignment(File_పంక్తి* flp, Ast_కణుపు_సమాసం* lhsp, Dfg_శీర్షం* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_ఫలితం_Equations;
        // Render the driver
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = convert_Dfg_శీర్షం_గమ్యం_Ast_కణుపు_సమాసం(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_కణుపు* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        ఒకవేళ (m_alwaysp) {
            m_alwaysp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, lhsp, rhsp});
            తిరిగి;
        }

        // Otherwise create an AssignW
        Ast_కేటాయించు_W* స్థిరరాశి ap = కొత్త Ast_కేటాయించు_W{flp, lhsp, rhsp};
        m_activep->కూడు_Stmtsp(కొత్త AstAlways{ap});
    }

    శూన్య convertDriver(File_పంక్తి* flp, Ast_కణుపు_సమాసం* lhsp, Dfg_శీర్షం* driverp) {
        ఒకవేళ (DfgSplicePacked* స్థిరరాశి sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](Dfg_శీర్షం& src, uint32_t lo, File_పంక్తి* dflp) {
                // Create Sel
                AstConst* స్థిరరాశి lsbp = కొత్త AstConst{dflp, lo};
                స్థిరరాశి పూర్ణాంకం వెడల్పు = static_cast<పూర్ణాంకం>(src.వెడల్పు());
                AstSel* స్థిరరాశి nLhsp = కొత్త AstSel{dflp, lhsp->clone_వృక్షం_శుద్ధ(అబద్ధం), lsbp, వెడల్పు};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_వృక్షం(), nLhsp);
                తిరిగి అబద్ధం;
            });
            తిరిగి;
        }

        ఒకవేళ (DfgSpliceArray* స్థిరరాశి sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](Dfg_శీర్షం& src, uint32_t lo, File_పంక్తి* dflp) {
                UASSERT_OBJ(src.కొలత() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* స్థిరరాశి idxp = కొత్త AstConst{dflp, lo};
                AstArraySel* స్థిరరాశి nLhsp = కొత్త AstArraySel{dflp, lhsp->clone_వృక్షం_శుద్ధ(అబద్ధం), idxp};
                // Convert source
                ఒకవేళ (స్థిరరాశి DfgUnitArray* స్థిరరాశి uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } లేకపోతే {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_వృక్షం(), nLhsp);
                తిరిగి అబద్ధం;
            });
            తిరిగి;
        }

        ఒకవేళ (స్థిరరాశి DfgUnitArray* స్థిరరాశి uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* స్థిరరాశి idxp = కొత్త AstConst{flp, 0};
            AstArraySel* స్థిరరాశి nLhsp = కొత్త AstArraySel{flp, lhsp->clone_వృక్షం_శుద్ధ(అబద్ధం), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_వృక్షం(), nLhsp);
            తిరిగి;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_వృక్షం_శుద్ధ(అబద్ధం), driverp);
    }

    // VISITORS
    శూన్య సందర్శించు(Dfg_శీర్షం* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    శూన్య సందర్శించు(DfgVarPacked* vtxp) override {
        m_resultp = కొత్త AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    శూన్య సందర్శించు(DfgVarArray* vtxp) override {
        m_resultp = కొత్త AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    శూన్య సందర్శించు(DfgPrev* vtxp) override {
        m_resultp = కొత్త AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    శూన్య సందర్శించు(DfgConst* vtxp) override {  //
        m_resultp = కొత్త AstConst{vtxp->fileline(), vtxp->అంకె()};
    }
    శూన్య సందర్శించు(DfgCReset* vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<Dfg_శీర్షం_Var>(), sinkp, "CReset should drive a variable");
        AstVar* స్థిరరాశి varp = sinkp->as<Dfg_శీర్షం_Var>()->vscp()->varp();
        m_resultp = కొత్త AstCReset{vtxp->fileline(), varp, అబద్ధం};
    }
    శూన్య సందర్శించు(Dfg_సరిపోలిక_Masked* vtxp) override {
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = convert_Dfg_శీర్షం_గమ్యం_Ast_కణుపు_సమాసం(vtxp->lhsp());
        AstVarScope* స్థిరరాశి matchp = vtxp->matchp()->as<Dfg_శీర్షం_Var>()->vscp();
        m_resultp = కొత్త Ast_సరిపోలిక_Masked{flp, lhsp, కొత్త AstVarRef{flp, matchp, VAccess::READ}};
    }

    శూన్య సందర్శించు(DfgRep* vtxp) override {
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి srcp = convert_Dfg_శీర్షం_గమ్యం_Ast_కణుపు_సమాసం(vtxp->srcp());
        m_resultp = కొత్త AstReplicate{flp, srcp, vtxp->లెక్క()};
    }

    శూన్య సందర్శించు(DfgSel* vtxp) override {
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి fromp = convert_Dfg_శీర్షం_గమ్యం_Ast_కణుపు_సమాసం(vtxp->fromp());
        AstConst* స్థిరరాశి lsbp = కొత్త AstConst{flp, vtxp->lsb()};
        m_resultp = కొత్త AstSel{flp, fromp, lsbp, static_cast<పూర్ణాంకం>(vtxp->వెడల్పు())};
    }

    శూన్య సందర్శించు(DfgMux* vtxp) override {
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి fromp = convert_Dfg_శీర్షం_గమ్యం_Ast_కణుపు_సమాసం(vtxp->fromp());
        Ast_కణుపు_సమాసం* స్థిరరాశి lsbp = convert_Dfg_శీర్షం_గమ్యం_Ast_కణుపు_సమాసం(vtxp->lsbp());
        m_resultp = కొత్త AstSel{flp, fromp, lsbp, static_cast<పూర్ణాంకం>(vtxp->వెడల్పు())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#అన్తర్భావ "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    Dfg_గమ్యం_Ast_Visitor(Dfg_లేఖాచిత్రం& dfg, V3_Dfg_Dfg_గమ్యం_Ast_Context& ctx)
        : m_ctx{ctx} {
        ఒకవేళ (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        కొరకు (Dfg_శీర్షం_Var& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            ఒకవేళ (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                కొనసాగు;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_పంక్తి* స్థిరరాశి flp = vtx.driver_File_పంక్తి() ? vtx.driver_File_పంక్తి() : vtx.fileline();
            AstVarRef* స్థిరరాశి lhsp = కొత్త AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = get_Comb_చురుకైన(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            ఒకవేళ (Dfg_శీర్షం* స్థిరరాశి defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = కొత్త AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, శూన్యసూచిక};
                m_activep->కూడు_Stmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_వృక్షం_శుద్ధ(అబద్ధం), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_వృక్షం(), lhsp);
        }

        // Update Ast References
        కొరకు (Dfg_శీర్షం_Ast& vtx : dfg.astVertices()) {
            ఒకవేళ (DfgAstRd* స్థిరరాశి rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_కణుపు_సమాసం* స్థిరరాశి exprp = convert_Dfg_శీర్షం_గమ్యం_Ast_కణుపు_సమాసం(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                ఒకవేళ (exprp->same_వృక్షం(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_వృక్షం(), exprp);
                    కొనసాగు;
                }
                // Replace the reference with the expression
                ఒకవేళ (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } లేకపోతే {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_వృక్షం();
                rVtxp->exprp(exprp);
            }
        }
    }

బహిరంగ:
    స్థిర శూన్య apply(Dfg_లేఖాచిత్రం& dfg, V3_Dfg_Dfg_గమ్యం_Ast_Context& ctx) { Dfg_గమ్యం_Ast_Visitor{dfg, ctx}; }
};

శూన్య V3DfgPasses::dfg_గమ్యం_Ast(Dfg_లేఖాచిత్రం& dfg, V3DfgContext& ctx) {
    Dfg_గమ్యం_Ast_Visitor::apply(dfg, ctx.m_dfg2AstContext);
}
