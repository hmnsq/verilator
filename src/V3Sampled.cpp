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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Sampled.h"

#অন্তর্ভাব "V3Sched.h"
#অন্তর্ভাব "V3SharedTmps.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clock state, as a visitor of each AstNode

বর্গ SampledVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()  -> AstVarScope*. The VarScope that stores sampled value
    //  AstVarRef::user1()    -> bool. Whether already converted
    ধ্রুব VNUser1InUse m_user1InUse;

    // STATE - across all visitors
    AstCFunc* ধ্রুব m_sampleFuncp;  // The 'sample' eval function assigning the sampled values
    V3SharedTmps m_sampledTmps{"__Vsampled", VVarType::MODULETEMP};  // Sampled variables
    V3SharedTmps m_forceTmps{"__VsampledForce", VVarType::MODULETEMP};  // Sampled force values
    // Keys remain in the tree as the sampled variables' value expressions.
    std::unordered_map<VNRef<Ast_গ্রন্থি>, AstVarScope*> m_forceSamples;

    // STATE - for current visit position (use VL_RESTORER)
    AstScope* m_scopep = nullptr;  // Current scope
    তার্কিক m_inSampled = অসত্য;  // True inside a sampled expression

    // METHODS

    // Create a variable in the current scope, assigned 'valuep' at the start of each evaluation
    AstVarScope* createSampledVar(V3SharedTmps& tmps, Ast_গ্রন্থি_রাশিমালা* valuep,
                                  ধ্রুব std::string& name) {
        File_সারি* ধ্রুব flp = valuep->fileline();
        AstVarScope* ধ্রুব vscp = tmps.make(flp, m_scopep, valuep->dtypep(), name);
        vscp->varp()->sampled(সত্য);
        AstVarRef* ধ্রুব lhsp = নূতন AstVarRef{flp, vscp, VAccess::WRITE};
        m_sampleFuncp->addStmtsp(নূতন AstAssign{flp, lhsp, valuep});
        UINFO(4, "New Sampled: " << vscp);
        প্রত্যাগম vscp;
    }

    // VISITORS
    শূন্য visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstSampled* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = সত্য;
        iterateChildren(nodep);
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstCMethodHard* nodep) override {
        যদি (!m_inSampled || nodep->method() != VCMethod::FORCE_READ) {
            iterateChildren(nodep);
            প্রত্যাগম;
        }
        // Force vectors hold pointers to live RHS shadows. Copying the vector and base
        // separately would let later writes change the sampled value (IEEE 1800-2023 16.5.1).
        // Whole-signal reads have no user expressions to evaluate at the point of use.
        ধ্রুব auto pair = m_forceSamples.emplace(*nodep, nullptr);
        AstVarScope*& vscp = pair.first->second;
        File_সারি* ধ্রুব flp = nodep->fileline();
        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);
        যদি (pair.second) {
            vscp = createSampledVar(m_forceTmps, nodep, "");
        } অন্যথা {
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
        AstVarRef* ধ্রুব refp = নূতন AstVarRef{flp, vscp, VAccess::READ};
        refp->user1SetOnce();
        relinker.relink(refp);
    }
    শূন্য visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        যদি (m_inSampled && !nodep->user1SetOnce()) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Should have failed in V3Access");
            AstVarScope* ধ্রুব vscp = nodep->varScopep();
            যদি (!vscp->user1p()) {
                AstVarRef* ধ্রুব valuep = নূতন AstVarRef{vscp->fileline(), vscp, VAccess::READ};
                vscp->user1p(createSampledVar(m_sampledTmps, valuep, vscp->varp()->name()));
            }
            AstVarScope* ধ্রুব sampledp = VN_AS(vscp->user1p(), VarScope);
            Ast_গ্রন্থি* ধ্রুব newp = নূতন AstVarRef{nodep->fileline(), sampledp, VAccess::READ};
            newp->user1SetOnce();  // Don't sample this one
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }

    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit SampledVisitor(AstNetlist* netlistp)
        : m_sampleFuncp{netlistp->evalFuncp(VEval::SAMPLE)} {
        iterate(netlistp);
        V3Sched::util::splitCheck(m_sampleFuncp);
    }
    ~SampledVisitor() override = স্বভাব;
};

//######################################################################
// Sampled class functions

শূন্য V3Sampled::sampledAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SampledVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("sampled", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
