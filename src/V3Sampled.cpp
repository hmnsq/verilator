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
    ಧ್ರುವ VNUser1InUse m_user1InUse;

    // STATE - across all visitors
    AstCFunc* ಧ್ರುವ m_sampleFuncp;  // The 'sample' eval function assigning the sampled values
    V3SharedTmps m_sampledTmps{"__Vsampled", VVarType::MODULETEMP};  // Sampled variables
    V3SharedTmps m_forceTmps{"__VsampledForce", VVarType::MODULETEMP};  // Sampled force values
    // Keys remain in the tree as the sampled variables' value expressions.
    std::unordered_map<VNRef<Ast_ಗಂಟು>, AstVarScope*> m_forceSamples;

    // STATE - for current visit position (use VL_RESTORER)
    AstScope* m_scopep = nullptr;  // Current scope
    ತಾರ್ಕಿಕ m_inSampled = ಅಸತ್ಯ;  // True inside a sampled expression

    // METHODS

    // Create a variable in the current scope, assigned 'valuep' at the start of each evaluation
    AstVarScope* createSampledVar(V3SharedTmps& tmps, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep,
                                  ಧ್ರುವ std::string& name) {
        File_ಸಾಲು* ಧ್ರುವ flp = valuep->fileline();
        AstVarScope* ಧ್ರುವ vscp = tmps.make(flp, m_scopep, valuep->dtypep(), name);
        vscp->varp()->sampled(ಸತ್ಯ);
        AstVarRef* ಧ್ರುವ lhsp = ನೂತನ AstVarRef{flp, vscp, VAccess::WRITE};
        m_sampleFuncp->addStmtsp(ನೂತನ AstAssign{flp, lhsp, valuep});
        UINFO(4, "New Sampled: " << vscp);
        ಪ್ರತ್ಯಾಗಮ vscp;
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstSampled* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = ಸತ್ಯ;
        iterateChildren(nodep);
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstCMethodHard* nodep) override {
        ಯದಿ (!m_inSampled || nodep->method() != VCMethod::FORCE_READ) {
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Force vectors hold pointers to live RHS shadows. Copying the vector and base
        // separately would let later writes change the sampled value (IEEE 1800-2023 16.5.1).
        // Whole-signal reads have no user expressions to evaluate at the point of use.
        ಧ್ರುವ auto pair = m_forceSamples.emplace(*nodep, nullptr);
        AstVarScope*& vscp = pair.first->second;
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);
        ಯದಿ (pair.second) {
            vscp = createSampledVar(m_forceTmps, nodep, "");
        } ಅನ್ಯಥಾ {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
        AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{flp, vscp, VAccess::READ};
        refp->user1SetOnce();
        relinker.relink(refp);
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (m_inSampled && !nodep->user1SetOnce()) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Should have failed in V3Access");
            AstVarScope* ಧ್ರುವ vscp = nodep->varScopep();
            ಯದಿ (!vscp->user1p()) {
                AstVarRef* ಧ್ರುವ valuep = ನೂತನ AstVarRef{vscp->fileline(), vscp, VAccess::READ};
                vscp->user1p(createSampledVar(m_sampledTmps, valuep, vscp->varp()->name()));
            }
            AstVarScope* ಧ್ರುವ sampledp = VN_AS(vscp->user1p(), VarScope);
            Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstVarRef{nodep->fileline(), sampledp, VAccess::READ};
            newp->user1SetOnce();  // Don't sample this one
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit SampledVisitor(AstNetlist* netlistp)
        : m_sampleFuncp{netlistp->evalFuncp(VEval::SAMPLE)} {
        iterate(netlistp);
        V3Sched::util::splitCheck(m_sampleFuncp);
    }
    ~SampledVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Sampled class functions

ಶೂನ್ಯ V3Sampled::sampledAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SampledVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("sampled", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
