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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Sampled.h"

#अन्तर्भाव "V3Sched.h"
#अन्तर्भाव "V3SharedTmps.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clock state, as a visitor of each AstNode

वर्ग SampledVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()  -> AstVarScope*. The VarScope that stores sampled value
    //  AstVarRef::user1()    -> bool. Whether already converted
    ध्रुव VNUser1_अन्तर्_Use m_user1_अन्तर्_Use;

    // STATE - across all visitors
    AstCFunc* ध्रुव m_sampleFuncp;  // The 'sample' eval function assigning the sampled values
    V3SharedTmps m_sampledTmps{"__Vsampled", VVarType::MODULETEMP};  // Sampled variables
    V3SharedTmps m_forceTmps{"__VsampledForce", VVarType::MODULETEMP};  // Sampled force values
    // Keys remain in the tree as the sampled variables' value expressions.
    std::unordered_map<VNRef<Ast_ग्रन्थिः>, AstVarScope*> m_forceSamples;

    // STATE - for current visit position (use VL_RESTORER)
    AstScope* m_scopep = शून्यसूचकः;  // Current scope
    तार्किक m_अन्तर्_Sampled = असत्य;  // True inside a sampled expression

    // METHODS

    // Create a variable in the current scope, assigned 'valuep' at the start of each evaluation
    AstVarScope* createSampledVar(V3SharedTmps& tmps, Ast_ग्रन्थिः_व्यञ्जकम्* valuep,
                                  ध्रुव std::string& name) {
        File_पङ्क्तिः* ध्रुव flp = valuep->fileline();
        AstVarScope* ध्रुव vscp = tmps.make(flp, m_scopep, valuep->dtypep(), name);
        vscp->varp()->sampled(सत्य);
        AstVarRef* ध्रुव lhsp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
        m_sampleFuncp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, lhsp, valuep});
        UINFO(4, "New Sampled: " << vscp);
        प्रत्यागम vscp;
    }

    // VISITORS
    शून्य दर्शय(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstSampled* nodep) override {
        VL_RESTORER(m_अन्तर्_Sampled);
        m_अन्तर्_Sampled = सत्य;
        iterateChildren(nodep);
        nodep->replaceWith(nodep->exprp()->unlink_Fr_पृष्ठभागः());
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstCMethodHard* nodep) override {
        यदि (!m_अन्तर्_Sampled || nodep->विधानम्() != VCMethod::FORCE_READ) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        // Force vectors hold pointers to live RHS shadows. Copying the vector and base
        // separately would let later writes change the sampled value (IEEE 1800-2023 16.5.1).
        // Whole-signal reads have no user expressions to evaluate at the point of use.
        ध्रुव स्वचालित pair = m_forceSamples.emplace(*nodep, शून्यसूचकः);
        AstVarScope*& vscp = pair.first->second;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        VNRelinker relinker;
        nodep->unlink_Fr_पृष्ठभागः(&relinker);
        यदि (pair.second) {
            vscp = createSampledVar(m_forceTmps, nodep, "");
        } अन्यथा {
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
        AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, vscp, VAccess::READ};
        refp->user1_समुच्चयः_Once();
        relinker.relink(refp);
    }
    शून्य दर्शय(AstVarRef* nodep) override {
        iterateChildren(nodep);
        यदि (m_अन्तर्_Sampled && !nodep->user1_समुच्चयः_Once()) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Should have failed in V3Access");
            AstVarScope* ध्रुव vscp = nodep->varScopep();
            यदि (!vscp->user1p()) {
                AstVarRef* ध्रुव valuep = नूतन AstVarRef{vscp->fileline(), vscp, VAccess::READ};
                vscp->user1p(createSampledVar(m_sampledTmps, valuep, vscp->varp()->name()));
            }
            AstVarScope* ध्रुव sampledp = VN_AS(vscp->user1p(), VarScope);
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstVarRef{nodep->fileline(), sampledp, VAccess::READ};
            newp->user1_समुच्चयः_Once();  // Don't sample this one
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }

    //--------------------
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट SampledVisitor(AstNetlist* netlistp)
        : m_sampleFuncp{netlistp->evalFuncp(VEval::SAMPLE)} {
        iterate(netlistp);
        V3Sched::util::विभज_Check(m_sampleFuncp);
    }
    ~SampledVisitor() override = स्वभाव;
};

//######################################################################
// Sampled class functions

शून्य V3Sampled::sampledAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SampledVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("sampled", 0, dump_वृक्षः_Either_Level() >= 3);
}
