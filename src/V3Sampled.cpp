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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Sampled.h"

#అన్తర్భావ "V3Sched.h"
#అన్తర్భావ "V3SharedTmps.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clock state, as a visitor of each AstNode

తరగతి SampledVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()  -> AstVarScope*. The VarScope that stores sampled value
    //  AstVarRef::user1()    -> bool. Whether already converted
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;

    // STATE - across all visitors
    AstCFunc* స్థిరరాశి m_sampleFuncp;  // The 'sample' eval function assigning the sampled values
    V3SharedTmps m_sampledTmps{"__Vsampled", VVarType::MODULETEMP};  // Sampled variables
    V3SharedTmps m_forceTmps{"__VsampledForce", VVarType::MODULETEMP};  // Sampled force values
    // Keys remain in the tree as the sampled variables' value expressions.
    std::unordered_map<VNRef<Ast_కణుపు>, AstVarScope*> m_forceSamples;

    // STATE - for current visit position (use VL_RESTORER)
    AstScope* m_scopep = శూన్యసూచిక;  // Current scope
    తార్కిక m_లోపల_Sampled = అబద్ధం;  // True inside a sampled expression

    // METHODS

    // Create a variable in the current scope, assigned 'valuep' at the start of each evaluation
    AstVarScope* createSampledVar(V3SharedTmps& tmps, Ast_కణుపు_సమాసం* valuep,
                                  స్థిరరాశి std::string& name) {
        File_పంక్తి* స్థిరరాశి flp = valuep->fileline();
        AstVarScope* స్థిరరాశి vscp = tmps.make(flp, m_scopep, valuep->dtypep(), name);
        vscp->varp()->sampled(నిజం);
        AstVarRef* స్థిరరాశి lhsp = కొత్త AstVarRef{flp, vscp, VAccess::WRITE};
        m_sampleFuncp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, lhsp, valuep});
        UINFO(4, "New Sampled: " << vscp);
        తిరిగి vscp;
    }

    // VISITORS
    శూన్య సందర్శించు(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstSampled* nodep) override {
        VL_RESTORER(m_లోపల_Sampled);
        m_లోపల_Sampled = నిజం;
        iterateChildren(nodep);
        nodep->replaceWith(nodep->exprp()->unlink_Fr_వెనుక());
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstCMethodHard* nodep) override {
        ఒకవేళ (!m_లోపల_Sampled || nodep->పద్ధతి() != VCMethod::FORCE_READ) {
            iterateChildren(nodep);
            తిరిగి;
        }
        // Force vectors hold pointers to live RHS shadows. Copying the vector and base
        // separately would let later writes change the sampled value (IEEE 1800-2023 16.5.1).
        // Whole-signal reads have no user expressions to evaluate at the point of use.
        స్థిరరాశి స్వయంచాలక pair = m_forceSamples.emplace(*nodep, శూన్యసూచిక);
        AstVarScope*& vscp = pair.మొదటి->రెండవ;
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        VNRelinker relinker;
        nodep->unlink_Fr_వెనుక(&relinker);
        ఒకవేళ (pair.రెండవ) {
            vscp = createSampledVar(m_forceTmps, nodep, "");
        } లేకపోతే {
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
        AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{flp, vscp, VAccess::READ};
        refp->user1_సమితి_Once();
        relinker.relink(refp);
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (m_లోపల_Sampled && !nodep->user1_సమితి_Once()) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Should have failed in V3Access");
            AstVarScope* స్థిరరాశి vscp = nodep->varScopep();
            ఒకవేళ (!vscp->user1p()) {
                AstVarRef* స్థిరరాశి valuep = కొత్త AstVarRef{vscp->fileline(), vscp, VAccess::READ};
                vscp->user1p(createSampledVar(m_sampledTmps, valuep, vscp->varp()->name()));
            }
            AstVarScope* స్థిరరాశి sampledp = VN_AS(vscp->user1p(), VarScope);
            Ast_కణుపు* స్థిరరాశి newp = కొత్త AstVarRef{nodep->fileline(), sampledp, VAccess::READ};
            newp->user1_సమితి_Once();  // Don't sample this one
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }

    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన SampledVisitor(AstNetlist* netlistp)
        : m_sampleFuncp{netlistp->evalFuncp(VEval::SAMPLE)} {
        iterate(netlistp);
        V3Sched::util::విభజించు_Check(m_sampleFuncp);
    }
    ~SampledVisitor() override = అప్రమేయ;
};

//######################################################################
// Sampled class functions

శూన్య V3Sampled::sampledAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SampledVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("sampled", 0, dump_వృక్షం_Either_Level() >= 3);
}
