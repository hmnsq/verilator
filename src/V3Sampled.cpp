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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Sampled.h"

#ଅନ୍ତର୍ଭାଵ "V3Sched.h"
#ଅନ୍ତର୍ଭାଵ "V3SharedTmps.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clock state, as a visitor of each AstNode

ଶ୍ରେଣୀ SampledVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()  -> AstVarScope*. The VarScope that stores sampled value
    //  AstVarRef::user1()    -> bool. Whether already converted
    ଅଚଳ VNUser1InUse m_user1InUse;

    // STATE - across all visitors
    AstCFunc* ଅଚଳ m_sampleFuncp;  // The 'sample' eval function assigning the sampled values
    V3SharedTmps m_sampledTmps{"__Vsampled", VVarType::MODULETEMP};  // Sampled variables
    V3SharedTmps m_forceTmps{"__VsampledForce", VVarType::MODULETEMP};  // Sampled force values
    // Keys remain in the tree as the sampled variables' value expressions.
    std::unordered_map<VNRef<Ast_ଗଣ୍ଠି>, AstVarScope*> m_forceSamples;

    // STATE - for current visit position (use VL_RESTORER)
    AstScope* m_scopep = nullptr;  // Current scope
    ତାର୍କିକ m_inSampled = ମିଥ୍ୟା;  // True inside a sampled expression

    // METHODS

    // Create a variable in the current scope, assigned 'valuep' at the start of each evaluation
    AstVarScope* createSampledVar(V3SharedTmps& tmps, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep,
                                  ଅଚଳ std::string& name) {
        File_ଧାଡ଼ି* ଅଚଳ flp = valuep->fileline();
        AstVarScope* ଅଚଳ vscp = tmps.make(flp, m_scopep, valuep->dtypep(), name);
        vscp->varp()->sampled(ସତ୍ୟ);
        AstVarRef* ଅଚଳ lhsp = ନୂତନ AstVarRef{flp, vscp, VAccess::WRITE};
        m_sampleFuncp->addStmtsp(ନୂତନ AstAssign{flp, lhsp, valuep});
        UINFO(4, "New Sampled: " << vscp);
        ଫେରନ୍ତୁ vscp;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstSampled* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = ସତ୍ୟ;
        iterateChildren(nodep);
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstCMethodHard* nodep) override {
        ଯଦି (!m_inSampled || nodep->method() != VCMethod::FORCE_READ) {
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }
        // Force vectors hold pointers to live RHS shadows. Copying the vector and base
        // separately would let later writes change the sampled value (IEEE 1800-2023 16.5.1).
        // Whole-signal reads have no user expressions to evaluate at the point of use.
        ଅଚଳ auto pair = m_forceSamples.emplace(*nodep, nullptr);
        AstVarScope*& vscp = pair.first->second;
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);
        ଯଦି (pair.second) {
            vscp = createSampledVar(m_forceTmps, nodep, "");
        } ଅନ୍ୟଥା {
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
        AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{flp, vscp, VAccess::READ};
        refp->user1SetOnce();
        relinker.relink(refp);
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        ଯଦି (m_inSampled && !nodep->user1SetOnce()) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Should have failed in V3Access");
            AstVarScope* ଅଚଳ vscp = nodep->varScopep();
            ଯଦି (!vscp->user1p()) {
                AstVarRef* ଅଚଳ valuep = ନୂତନ AstVarRef{vscp->fileline(), vscp, VAccess::READ};
                vscp->user1p(createSampledVar(m_sampledTmps, valuep, vscp->varp()->name()));
            }
            AstVarScope* ଅଚଳ sampledp = VN_AS(vscp->user1p(), VarScope);
            Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstVarRef{nodep->fileline(), sampledp, VAccess::READ};
            newp->user1SetOnce();  // Don't sample this one
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }

    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit SampledVisitor(AstNetlist* netlistp)
        : m_sampleFuncp{netlistp->evalFuncp(VEval::SAMPLE)} {
        iterate(netlistp);
        V3Sched::util::splitCheck(m_sampleFuncp);
    }
    ~SampledVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Sampled class functions

ଶୂନ୍ୟ V3Sampled::sampledAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SampledVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("sampled", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
