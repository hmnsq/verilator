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
    ध्रुव VNUser1InUse m_user1InUse;

    // STATE - across all visitors
    AstCFunc* ध्रुव m_sampleFuncp;  // The 'sample' eval function assigning the sampled values
    V3SharedTmps m_sampledTmps{"__Vsampled", VVarType::MODULETEMP};  // Sampled variables
    V3SharedTmps m_forceTmps{"__VsampledForce", VVarType::MODULETEMP};  // Sampled force values
    // Keys remain in the tree as the sampled variables' value expressions.
    std::unordered_map<VNRef<Ast_गाठ>, AstVarScope*> m_forceSamples;

    // STATE - for current visit position (use VL_RESTORER)
    AstScope* m_scopep = nullptr;  // Current scope
    तार्किक m_inSampled = असत्य;  // True inside a sampled expression

    // METHODS

    // Create a variable in the current scope, assigned 'valuep' at the start of each evaluation
    AstVarScope* createSampledVar(V3SharedTmps& tmps, Ast_गाठ_अभिव्यक्ती* valuep,
                                  ध्रुव std::string& name) {
        File_ओळ* ध्रुव flp = valuep->fileline();
        AstVarScope* ध्रुव vscp = tmps.make(flp, m_scopep, valuep->dtypep(), name);
        vscp->varp()->sampled(सत्य);
        AstVarRef* ध्रुव lhsp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
        m_sampleFuncp->addStmtsp(नूतन AstAssign{flp, lhsp, valuep});
        UINFO(4, "New Sampled: " << vscp);
        प्रत्यागम vscp;
    }

    // VISITORS
    शून्य visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstSampled* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = सत्य;
        iterateChildren(nodep);
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstCMethodHard* nodep) override {
        यदि (!m_inSampled || nodep->method() != VCMethod::FORCE_READ) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        // Force vectors hold pointers to live RHS shadows. Copying the vector and base
        // separately would let later writes change the sampled value (IEEE 1800-2023 16.5.1).
        // Whole-signal reads have no user expressions to evaluate at the point of use.
        ध्रुव auto pair = m_forceSamples.emplace(*nodep, nullptr);
        AstVarScope*& vscp = pair.first->second;
        File_ओळ* ध्रुव flp = nodep->fileline();
        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);
        यदि (pair.second) {
            vscp = createSampledVar(m_forceTmps, nodep, "");
        } अन्यथा {
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
        AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, vscp, VAccess::READ};
        refp->user1SetOnce();
        relinker.relink(refp);
    }
    शून्य visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        यदि (m_inSampled && !nodep->user1SetOnce()) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Should have failed in V3Access");
            AstVarScope* ध्रुव vscp = nodep->varScopep();
            यदि (!vscp->user1p()) {
                AstVarRef* ध्रुव valuep = नूतन AstVarRef{vscp->fileline(), vscp, VAccess::READ};
                vscp->user1p(createSampledVar(m_sampledTmps, valuep, vscp->varp()->name()));
            }
            AstVarScope* ध्रुव sampledp = VN_AS(vscp->user1p(), VarScope);
            Ast_गाठ* ध्रुव newp = नूतन AstVarRef{nodep->fileline(), sampledp, VAccess::READ};
            newp->user1SetOnce();  // Don't sample this one
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }

    //--------------------
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit SampledVisitor(AstNetlist* netlistp)
        : m_sampleFuncp{netlistp->evalFuncp(VEval::SAMPLE)} {
        iterate(netlistp);
        V3Sched::util::splitCheck(m_sampleFuncp);
    }
    ~SampledVisitor() override = स्वभाव;
};

//######################################################################
// Sampled class functions

शून्य V3Sampled::sampledAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SampledVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("sampled", 0, dump_वृक्ष_Either_Level() >= 3);
}
