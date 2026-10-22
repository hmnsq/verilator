// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Removal of SAMPLED
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
// V3Sampled's Transformations:
//
// Top Scope:
//   Replace each variable reference under SAMPLED with a new variable.
//   Capture whole-signal force reads by value, before sampling their children.
//   Assign the new variables in the 'sample' eval function.
//   Remove SAMPLED.
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Sampled.h"

#ಅನ್ತರ್ಭಾವ "V3Sched.h"
#ಅನ್ತರ್ಭಾವ "V3SharedTmps.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clock state, as a visitor of each AstNode

ವರ್ಗ SampledVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()  -> AstVarScope*. The VarScope that stores sampled value
    //  AstVarRef::user1()    -> bool. Whether already converted
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;

    // STATE - across all visitors
    AstCFunc* ಸ್ಥಿರಾಂಕ m_sampleFuncp;  // The 'sample' eval function assigning the sampled values
    V3SharedTmps m_sampledTmps{"__Vsampled", VVarType::MODULETEMP};  // Sampled variables
    V3SharedTmps m_forceTmps{"__VsampledForce", VVarType::MODULETEMP};  // Sampled force values
    // Keys remain in the tree as the sampled variables' value expressions.
    std::unordered_map<VNRef<Ast_ಗಂಟು>, AstVarScope*> m_forceSamples;

    // STATE - for current visit position (use VL_RESTORER)
    AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // Current scope
    ತಾರ್ಕಿಕ m_ಒಳ_Sampled = ಸುಳ್ಳು;  // True inside a sampled expression

    // METHODS

    // Create a variable in the current scope, assigned 'valuep' at the start of each evaluation
    AstVarScope* createSampledVar(V3SharedTmps& tmps, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep,
                                  ಸ್ಥಿರಾಂಕ std::string& name) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = valuep->fileline();
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = tmps.make(flp, m_scopep, valuep->dtypep(), name);
        vscp->varp()->sampled(ನಿಜ);
        AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstVarRef{flp, vscp, VAccess::WRITE};
        m_sampleFuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsp, valuep});
        UINFO(4, "New Sampled: " << vscp);
        ಹಿಂತಿರುಗಿ vscp;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSampled* nodep) override {
        VL_RESTORER(m_ಒಳ_Sampled);
        m_ಒಳ_Sampled = ನಿಜ;
        iterateChildren(nodep);
        nodep->replaceWith(nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodHard* nodep) override {
        ಆದರೆ (!m_ಒಳ_Sampled || nodep->ವಿಧಾನ() != VCMethod::FORCE_READ) {
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // Force vectors hold pointers to live RHS shadows. Copying the vector and base
        // separately would let later writes change the sampled value (IEEE 1800-2023 16.5.1).
        // Whole-signal reads have no user expressions to evaluate at the point of use.
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_forceSamples.emplace(*nodep, ಶೂನ್ಯಸೂಚಕ);
        AstVarScope*& vscp = pair.ಮೊದಲ->ಎರಡನೇ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        VNRelinker relinker;
        nodep->unlink_Fr_ಹಿಂಭಾಗ(&relinker);
        ಆದರೆ (pair.ಎರಡನೇ) {
            vscp = createSampledVar(m_forceTmps, nodep, "");
        } ಇಲ್ಲದಿದ್ದರೆ {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
        AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{flp, vscp, VAccess::READ};
        refp->user1_ಗಣ_Once();
        relinker.relink(refp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (m_ಒಳ_Sampled && !nodep->user1_ಗಣ_Once()) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Should have failed in V3Access");
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = nodep->varScopep();
            ಆದರೆ (!vscp->user1p()) {
                AstVarRef* ಸ್ಥಿರಾಂಕ valuep = ಹೊಸ AstVarRef{vscp->fileline(), vscp, VAccess::READ};
                vscp->user1p(createSampledVar(m_sampledTmps, valuep, vscp->varp()->name()));
            }
            AstVarScope* ಸ್ಥಿರಾಂಕ sampledp = VN_AS(vscp->user1p(), VarScope);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstVarRef{nodep->fileline(), sampledp, VAccess::READ};
            newp->user1_ಗಣ_Once();  // Don't sample this one
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ SampledVisitor(AstNetlist* netlistp)
        : m_sampleFuncp{netlistp->evalFuncp(VEval::SAMPLE)} {
        iterate(netlistp);
        V3Sched::util::ವಿಭಜಿಸು_Check(m_sampleFuncp);
    }
    ~SampledVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Sampled class functions

ಶೂನ್ಯ V3Sampled::sampledAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SampledVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("sampled", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
