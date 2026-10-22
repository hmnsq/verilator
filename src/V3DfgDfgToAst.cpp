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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPasses.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ {

// Create an AstNodeExpr out of a DfgVertex. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_ਗੰਢ, ਕਿਸਮ_ਨਾਂ T_ਸ਼ੀਰਸ਼, ਕਿਸਮ_ਨਾਂ... Ops>
T_ਗੰਢ* make_ਗੰਢ(ਅਚਲ T_ਸ਼ੀਰਸ਼* vtxp, Ops... ops) {
    T_ਗੰਢ* ਅਚਲ nodep = ਨਵਾਂ T_ਗੰਢ{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->ਚੌੜਾਈ() == static_cast<ਪੂਰਨ>(vtxp->ਚੌੜਾਈ()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->ਚੌੜਾਈ() << " vs " << vtxp->ਚੌੜਾਈ());
    ਵਾਪਸ nodep;
}

//======================================================================
// Vertices needing special conversion

ਸਾਂਚਾ <>
Ast_ਗਿਣਤੀ_Ones* make_ਗੰਢ<Ast_ਗਿਣਤੀ_Ones, Dfg_ਗਿਣਤੀ_Ones, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(  //
    ਅਚਲ Dfg_ਗਿਣਤੀ_Ones* vtxp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* op1) {
    Ast_ਗਿਣਤੀ_Ones* ਅਚਲ nodep = ਨਵਾਂ Ast_ਗਿਣਤੀ_Ones{vtxp->fileline(), op1};
    ਵਾਪਸ nodep;
}

ਸਾਂਚਾ <>
AstExtend* make_ਗੰਢ<AstExtend, DfgExtend, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(  //
    ਅਚਲ DfgExtend* vtxp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* op1) {
    ਵਾਪਸ ਨਵਾਂ AstExtend{vtxp->fileline(), op1, static_cast<ਪੂਰਨ>(vtxp->ਚੌੜਾਈ())};
}

ਸਾਂਚਾ <>
AstExtendS* make_ਗੰਢ<AstExtendS, DfgExtendS, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(  //
    ਅਚਲ DfgExtendS* vtxp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* op1) {
    ਵਾਪਸ ਨਵਾਂ AstExtendS{vtxp->fileline(), op1, static_cast<ਪੂਰਨ>(vtxp->ਚੌੜਾਈ())};
}

ਸਾਂਚਾ <>
Ast_ਖਿਸਕਾਓ_L* make_ਗੰਢ<Ast_ਖਿਸਕਾਓ_L, Dfg_ਖਿਸਕਾਓ_L, Ast_ਗੰਢ_ਵਿਅੰਜਕ*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(  //
    ਅਚਲ Dfg_ਖਿਸਕਾਓ_L* vtxp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* op1, Ast_ਗੰਢ_ਵਿਅੰਜਕ* op2) {
    ਵਾਪਸ ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{vtxp->fileline(), op1, op2, static_cast<ਪੂਰਨ>(vtxp->ਚੌੜਾਈ())};
}

ਸਾਂਚਾ <>
Ast_ਖਿਸਕਾਓ_R* make_ਗੰਢ<Ast_ਖਿਸਕਾਓ_R, Dfg_ਖਿਸਕਾਓ_R, Ast_ਗੰਢ_ਵਿਅੰਜਕ*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(  //
    ਅਚਲ Dfg_ਖਿਸਕਾਓ_R* vtxp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* op1, Ast_ਗੰਢ_ਵਿਅੰਜਕ* op2) {
    ਵਾਪਸ ਨਵਾਂ Ast_ਖਿਸਕਾਓ_R{vtxp->fileline(), op1, op2, static_cast<ਪੂਰਨ>(vtxp->ਚੌੜਾਈ())};
}

ਸਾਂਚਾ <>
Ast_ਖਿਸਕਾਓ_RS* make_ਗੰਢ<Ast_ਖਿਸਕਾਓ_RS, Dfg_ਖਿਸਕਾਓ_RS, Ast_ਗੰਢ_ਵਿਅੰਜਕ*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(  //
    ਅਚਲ Dfg_ਖਿਸਕਾਓ_RS* vtxp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* op1, Ast_ਗੰਢ_ਵਿਅੰਜਕ* op2) {
    ਵਾਪਸ ਨਵਾਂ Ast_ਖਿਸਕਾਓ_RS{vtxp->fileline(), op1, op2, static_cast<ਪੂਰਨ>(vtxp->ਚੌੜਾਈ())};
}

}  // namespace

ਵਰਗ Dfg_ਮੰਜ਼ਿਲ_Ast_Visitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_user2_ਵਿੱਚ_Use;

    // STATE
    V3_Dfg_Dfg_ਮੰਜ਼ਿਲ_Ast_Context& m_ctx;  // The context for stats
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_resultp = ਖਾਲੀ_ਸੂਚਕ;  // The result node of the current traversal
    AstAlways* m_alwaysp = ਖਾਲੀ_ਸੂਚਕ;  // Process to add assignments to, if have a default driver
    Ast_ਸਰਗਰਮ* m_activep = ਖਾਲੀ_ਸੂਚਕ;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    ਸਥਿਰ Ast_ਸਰਗਰਮ* get_Comb_ਸਰਗਰਮ(AstScope* scopep) {
        ਜੇ (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            ਲਈ (Ast_ਗੰਢ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_ਸਰਗਰਮ* ਅਚਲ activep = VN_CAST(nodep, ਸਰਗਰਮ);
                ਜੇ (!activep) ਜਾਰੀ;
                ਜੇ (activep->hasCombo()) {
                    scopep->user2p(activep);
                    ਰੋਕੋ;
                }
            }
            // If there isn't one, create a new one
            ਜੇ (!scopep->user2p()) {
                File_ਸਤਰ* ਅਚਲ flp = scopep->fileline();
                Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep
                    = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, Ast_Sen_ਵਸਤੂ::Combo{}}};
                Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "", senTreep};
                activep->sen_ਰੁੱਖ_Storep(senTreep);
                scopep->ਜੋੜੋ_Blocksp(activep);
                scopep->user2p(activep);
            }
        }
        ਵਾਪਸ VN_AS(scopep->user2p(), ਸਰਗਰਮ);
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* convert_Dfg_ਸ਼ੀਰਸ਼_ਮੰਜ਼ਿਲ_Ast_ਗੰਢ_ਵਿਅੰਜਕ(Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<Dfg_ਸ਼ੀਰਸ਼_Var>() || vtxp->is<DfgConst>()  //
                        || !vtxp->has_ਗੁਣਜ_Sinks() || vtxp->is_Cheaper_Than_ਲੱਦੋ(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ resultp = m_resultp;
        m_resultp = ਖਾਲੀ_ਸੂਚਕ;
        ਵਾਪਸ resultp;
    }

    ਖਾਲੀ createAssignment(File_ਸਤਰ* flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Dfg_ਸ਼ੀਰਸ਼* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_ਨਤੀਜਾ_Equations;
        // Render the driver
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = convert_Dfg_ਸ਼ੀਰਸ਼_ਮੰਜ਼ਿਲ_Ast_ਗੰਢ_ਵਿਅੰਜਕ(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_ਗੰਢ* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        ਜੇ (m_alwaysp) {
            m_alwaysp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsp, rhsp});
            ਵਾਪਸ;
        }

        // Otherwise create an AssignW
        Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{flp, lhsp, rhsp};
        m_activep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{ap});
    }

    ਖਾਲੀ convertDriver(File_ਸਤਰ* flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Dfg_ਸ਼ੀਰਸ਼* driverp) {
        ਜੇ (DfgSplicePacked* ਅਚਲ sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](Dfg_ਸ਼ੀਰਸ਼& src, uint32_t lo, File_ਸਤਰ* dflp) {
                // Create Sel
                AstConst* ਅਚਲ lsbp = ਨਵਾਂ AstConst{dflp, lo};
                ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = static_cast<ਪੂਰਨ>(src.ਚੌੜਾਈ());
                AstSel* ਅਚਲ nLhsp = ਨਵਾਂ AstSel{dflp, lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), lsbp, ਚੌੜਾਈ};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_ਰੁੱਖ(), nLhsp);
                ਵਾਪਸ ਝੂਠ;
            });
            ਵਾਪਸ;
        }

        ਜੇ (DfgSpliceArray* ਅਚਲ sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](Dfg_ਸ਼ੀਰਸ਼& src, uint32_t lo, File_ਸਤਰ* dflp) {
                UASSERT_OBJ(src.ਮਾਪ() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ਅਚਲ idxp = ਨਵਾਂ AstConst{dflp, lo};
                AstArraySel* ਅਚਲ nLhsp = ਨਵਾਂ AstArraySel{dflp, lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), idxp};
                // Convert source
                ਜੇ (ਅਚਲ DfgUnitArray* ਅਚਲ uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } ਨਹੀਂਤਾਂ {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_ਰੁੱਖ(), nLhsp);
                ਵਾਪਸ ਝੂਠ;
            });
            ਵਾਪਸ;
        }

        ਜੇ (ਅਚਲ DfgUnitArray* ਅਚਲ uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ਅਚਲ idxp = ਨਵਾਂ AstConst{flp, 0};
            AstArraySel* ਅਚਲ nLhsp = ਨਵਾਂ AstArraySel{flp, lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_ਰੁੱਖ(), nLhsp);
            ਵਾਪਸ;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), driverp);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Dfg_ਸ਼ੀਰਸ਼* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    ਖਾਲੀ ਦੇਖੋ(DfgVarPacked* vtxp) override {
        m_resultp = ਨਵਾਂ AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    ਖਾਲੀ ਦੇਖੋ(DfgVarArray* vtxp) override {
        m_resultp = ਨਵਾਂ AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    ਖਾਲੀ ਦੇਖੋ(DfgPrev* vtxp) override {
        m_resultp = ਨਵਾਂ AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    ਖਾਲੀ ਦੇਖੋ(DfgConst* vtxp) override {  //
        m_resultp = ਨਵਾਂ AstConst{vtxp->fileline(), vtxp->ਅੰਕ()};
    }
    ਖਾਲੀ ਦੇਖੋ(DfgCReset* vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<Dfg_ਸ਼ੀਰਸ਼_Var>(), sinkp, "CReset should drive a variable");
        AstVar* ਅਚਲ varp = sinkp->as<Dfg_ਸ਼ੀਰਸ਼_Var>()->vscp()->varp();
        m_resultp = ਨਵਾਂ AstCReset{vtxp->fileline(), varp, ਝੂਠ};
    }
    ਖਾਲੀ ਦੇਖੋ(Dfg_ਮੇਲ_Masked* vtxp) override {
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = convert_Dfg_ਸ਼ੀਰਸ਼_ਮੰਜ਼ਿਲ_Ast_ਗੰਢ_ਵਿਅੰਜਕ(vtxp->lhsp());
        AstVarScope* ਅਚਲ matchp = vtxp->matchp()->as<Dfg_ਸ਼ੀਰਸ਼_Var>()->vscp();
        m_resultp = ਨਵਾਂ Ast_ਮੇਲ_Masked{flp, lhsp, ਨਵਾਂ AstVarRef{flp, matchp, VAccess::READ}};
    }

    ਖਾਲੀ ਦੇਖੋ(DfgRep* vtxp) override {
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ srcp = convert_Dfg_ਸ਼ੀਰਸ਼_ਮੰਜ਼ਿਲ_Ast_ਗੰਢ_ਵਿਅੰਜਕ(vtxp->srcp());
        m_resultp = ਨਵਾਂ AstReplicate{flp, srcp, vtxp->ਗਿਣਤੀ_2()};
    }

    ਖਾਲੀ ਦੇਖੋ(DfgSel* vtxp) override {
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = convert_Dfg_ਸ਼ੀਰਸ਼_ਮੰਜ਼ਿਲ_Ast_ਗੰਢ_ਵਿਅੰਜਕ(vtxp->fromp());
        AstConst* ਅਚਲ lsbp = ਨਵਾਂ AstConst{flp, vtxp->lsb()};
        m_resultp = ਨਵਾਂ AstSel{flp, fromp, lsbp, static_cast<ਪੂਰਨ>(vtxp->ਚੌੜਾਈ())};
    }

    ਖਾਲੀ ਦੇਖੋ(DfgMux* vtxp) override {
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = convert_Dfg_ਸ਼ੀਰਸ਼_ਮੰਜ਼ਿਲ_Ast_ਗੰਢ_ਵਿਅੰਜਕ(vtxp->fromp());
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lsbp = convert_Dfg_ਸ਼ੀਰਸ਼_ਮੰਜ਼ਿਲ_Ast_ਗੰਢ_ਵਿਅੰਜਕ(vtxp->lsbp());
        m_resultp = ਨਵਾਂ AstSel{flp, fromp, lsbp, static_cast<ਪੂਰਨ>(vtxp->ਚੌੜਾਈ())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    Dfg_ਮੰਜ਼ਿਲ_Ast_Visitor(Dfg_ਗ੍ਰਾਫ& dfg, V3_Dfg_Dfg_ਮੰਜ਼ਿਲ_Ast_Context& ctx)
        : m_ctx{ctx} {
        ਜੇ (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            ਜੇ (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                ਜਾਰੀ;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_ਸਤਰ* ਅਚਲ flp = vtx.driver_File_ਸਤਰ() ? vtx.driver_File_ਸਤਰ() : vtx.fileline();
            AstVarRef* ਅਚਲ lhsp = ਨਵਾਂ AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = get_Comb_ਸਰਗਰਮ(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            ਜੇ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = ਨਵਾਂ AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, ਖਾਲੀ_ਸੂਚਕ};
                m_activep->ਜੋੜੋ_Stmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_ਰੁੱਖ(), lhsp);
        }

        // Update Ast References
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Ast& vtx : dfg.astVertices()) {
            ਜੇ (DfgAstRd* ਅਚਲ rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = convert_Dfg_ਸ਼ੀਰਸ਼_ਮੰਜ਼ਿਲ_Ast_ਗੰਢ_ਵਿਅੰਜਕ(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                ਜੇ (exprp->same_ਰੁੱਖ(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_ਰੁੱਖ(), exprp);
                    ਜਾਰੀ;
                }
                // Replace the reference with the expression
                ਜੇ (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } ਨਹੀਂਤਾਂ {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_ਰੁੱਖ();
                rVtxp->exprp(exprp);
            }
        }
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(Dfg_ਗ੍ਰਾਫ& dfg, V3_Dfg_Dfg_ਮੰਜ਼ਿਲ_Ast_Context& ctx) { Dfg_ਮੰਜ਼ਿਲ_Ast_Visitor{dfg, ctx}; }
};

ਖਾਲੀ V3DfgPasses::dfg_ਮੰਜ਼ਿਲ_Ast(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgContext& ctx) {
    Dfg_ਮੰਜ਼ਿਲ_Ast_Visitor::apply(dfg, ctx.m_dfg2AstContext);
}
