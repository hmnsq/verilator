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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sampled.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sched.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SharedTmps.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clock state, as a visitor of each AstNode

ਵਰਗ SampledVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()  -> AstVarScope*. The VarScope that stores sampled value
    //  AstVarRef::user1()    -> bool. Whether already converted
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;

    // STATE - across all visitors
    AstCFunc* ਅਚਲ m_sampleFuncp;  // The 'sample' eval function assigning the sampled values
    V3SharedTmps m_sampledTmps{"__Vsampled", VVarType::MODULETEMP};  // Sampled variables
    V3SharedTmps m_forceTmps{"__VsampledForce", VVarType::MODULETEMP};  // Sampled force values
    // Keys remain in the tree as the sampled variables' value expressions.
    std::unordered_map<VNRef<Ast_ਗੰਢ>, AstVarScope*> m_forceSamples;

    // STATE - for current visit position (use VL_RESTORER)
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Current scope
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Sampled = ਝੂਠ;  // True inside a sampled expression

    // METHODS

    // Create a variable in the current scope, assigned 'valuep' at the start of each evaluation
    AstVarScope* createSampledVar(V3SharedTmps& tmps, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep,
                                  ਅਚਲ std::string& name) {
        File_ਸਤਰ* ਅਚਲ flp = valuep->fileline();
        AstVarScope* ਅਚਲ vscp = tmps.make(flp, m_scopep, valuep->dtypep(), name);
        vscp->varp()->sampled(ਸੱਚ);
        AstVarRef* ਅਚਲ lhsp = ਨਵਾਂ AstVarRef{flp, vscp, VAccess::WRITE};
        m_sampleFuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsp, valuep});
        UINFO(4, "New Sampled: " << vscp);
        ਵਾਪਸ vscp;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSampled* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Sampled);
        m_ਵਿੱਚ_Sampled = ਸੱਚ;
        iterateChildren(nodep);
        nodep->replaceWith(nodep->exprp()->unlink_Fr_ਪਿੱਠ());
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodHard* nodep) override {
        ਜੇ (!m_ਵਿੱਚ_Sampled || nodep->ਵਿਧੀ() != VCMethod::FORCE_READ) {
            iterateChildren(nodep);
            ਵਾਪਸ;
        }
        // Force vectors hold pointers to live RHS shadows. Copying the vector and base
        // separately would let later writes change the sampled value (IEEE 1800-2023 16.5.1).
        // Whole-signal reads have no user expressions to evaluate at the point of use.
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_forceSamples.emplace(*nodep, ਖਾਲੀ_ਸੂਚਕ);
        AstVarScope*& vscp = pair.ਪਹਿਲਾ->ਦੂਜਾ;
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        VNRelinker relinker;
        nodep->unlink_Fr_ਪਿੱਠ(&relinker);
        ਜੇ (pair.ਦੂਜਾ) {
            vscp = createSampledVar(m_forceTmps, nodep, "");
        } ਨਹੀਂਤਾਂ {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
        AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ};
        refp->user1_ਸੈੱਟ_Once();
        relinker.relink(refp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        iterateChildren(nodep);
        ਜੇ (m_ਵਿੱਚ_Sampled && !nodep->user1_ਸੈੱਟ_Once()) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Should have failed in V3Access");
            AstVarScope* ਅਚਲ vscp = nodep->varScopep();
            ਜੇ (!vscp->user1p()) {
                AstVarRef* ਅਚਲ valuep = ਨਵਾਂ AstVarRef{vscp->fileline(), vscp, VAccess::READ};
                vscp->user1p(createSampledVar(m_sampledTmps, valuep, vscp->varp()->name()));
            }
            AstVarScope* ਅਚਲ sampledp = VN_AS(vscp->user1p(), VarScope);
            Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstVarRef{nodep->fileline(), sampledp, VAccess::READ};
            newp->user1_ਸੈੱਟ_Once();  // Don't sample this one
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ SampledVisitor(AstNetlist* netlistp)
        : m_sampleFuncp{netlistp->evalFuncp(VEval::SAMPLE)} {
        iterate(netlistp);
        V3Sched::util::ਵੰਡੋ_Check(m_sampleFuncp);
    }
    ~SampledVisitor() override = ਮੂਲ;
};

//######################################################################
// Sampled class functions

ਖਾਲੀ V3Sampled::sampledAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SampledVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("sampled", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
