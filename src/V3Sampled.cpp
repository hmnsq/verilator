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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Sampled.h"

#شامل "V3Sched.h"
#شامل "V3SharedTmps.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clock state, as a visitor of each AstNode

رده SampledVisitor final : عمومی VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()  -> AstVarScope*. The VarScope that stores sampled value
    //  AstVarRef::user1()    -> bool. Whether already converted
    ثابت VNUser1InUse m_user1InUse;

    // STATE - across all visitors
    AstCFunc* ثابت m_sampleFuncp;  // The 'sample' eval function assigning the sampled values
    V3SharedTmps m_sampledTmps{"__Vsampled", VVarType::MODULETEMP};  // Sampled variables
    V3SharedTmps m_forceTmps{"__VsampledForce", VVarType::MODULETEMP};  // Sampled force values
    // Keys remain in the tree as the sampled variables' value expressions.
    std::unordered_map<VNRef<Ast_گره>, AstVarScope*> m_forceSamples;

    // STATE - for current visit position (use VL_RESTORER)
    AstScope* m_scopep = nullptr;  // Current scope
    منطقی m_inSampled = نادرست;  // True inside a sampled expression

    // METHODS

    // Create a variable in the current scope, assigned 'valuep' at the start of each evaluation
    AstVarScope* createSampledVar(V3SharedTmps& tmps, Ast_گره_عبارت* valuep,
                                  ثابت std::string& name) {
        File_سطر* ثابت flp = valuep->fileline();
        AstVarScope* ثابت vscp = tmps.make(flp, m_scopep, valuep->dtypep(), name);
        vscp->varp()->sampled(درست);
        AstVarRef* ثابت lhsp = جدید AstVarRef{flp, vscp, VAccess::WRITE};
        m_sampleFuncp->addStmtsp(جدید AstAssign{flp, lhsp, valuep});
        UINFO(4, "New Sampled: " << vscp);
        بازگشت vscp;
    }

    // VISITORS
    تهی visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstSampled* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = درست;
        iterateChildren(nodep);
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstCMethodHard* nodep) override {
        اگر (!m_inSampled || nodep->method() != VCMethod::FORCE_READ) {
            iterateChildren(nodep);
            بازگشت;
        }
        // Force vectors hold pointers to live RHS shadows. Copying the vector and base
        // separately would let later writes change the sampled value (IEEE 1800-2023 16.5.1).
        // Whole-signal reads have no user expressions to evaluate at the point of use.
        ثابت auto pair = m_forceSamples.emplace(*nodep, nullptr);
        AstVarScope*& vscp = pair.first->second;
        File_سطر* ثابت flp = nodep->fileline();
        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);
        اگر (pair.second) {
            vscp = createSampledVar(m_forceTmps, nodep, "");
        } وگرنه {
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
        AstVarRef* ثابت refp = جدید AstVarRef{flp, vscp, VAccess::READ};
        refp->user1SetOnce();
        relinker.relink(refp);
    }
    تهی visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        اگر (m_inSampled && !nodep->user1SetOnce()) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Should have failed in V3Access");
            AstVarScope* ثابت vscp = nodep->varScopep();
            اگر (!vscp->user1p()) {
                AstVarRef* ثابت valuep = جدید AstVarRef{vscp->fileline(), vscp, VAccess::READ};
                vscp->user1p(createSampledVar(m_sampledTmps, valuep, vscp->varp()->name()));
            }
            AstVarScope* ثابت sampledp = VN_AS(vscp->user1p(), VarScope);
            Ast_گره* ثابت newp = جدید AstVarRef{nodep->fileline(), sampledp, VAccess::READ};
            newp->user1SetOnce();  // Don't sample this one
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }

    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit SampledVisitor(AstNetlist* netlistp)
        : m_sampleFuncp{netlistp->evalFuncp(VEval::SAMPLE)} {
        iterate(netlistp);
        V3Sched::util::splitCheck(m_sampleFuncp);
    }
    ~SampledVisitor() override = پیشفرض;
};

//######################################################################
// Sampled class functions

تهی V3Sampled::sampledAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SampledVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("sampled", 0, dump_درخت_Either_Level() >= 3);
}
