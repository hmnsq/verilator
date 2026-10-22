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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Sampled.h"

#ضمن "V3Sched.h"
#ضمن "V3SharedTmps.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clock state, as a visitor of each AstNode

صنف SampledVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()  -> AstVarScope*. The VarScope that stores sampled value
    //  AstVarRef::user1()    -> bool. Whether already converted
    ثابت VNUser1InUse m_user1InUse;

    // STATE - across all visitors
    AstCFunc* ثابت m_sampleFuncp;  // The 'sample' eval function assigning the sampled values
    V3SharedTmps m_sampledTmps{"__Vsampled", VVarType::MODULETEMP};  // Sampled variables
    V3SharedTmps m_forceTmps{"__VsampledForce", VVarType::MODULETEMP};  // Sampled force values
    // Keys remain in the tree as the sampled variables' value expressions.
    std::unordered_map<VNRef<Ast_نود>, AstVarScope*> m_forceSamples;

    // STATE - for current visit position (use VL_RESTORER)
    AstScope* m_scopep = nullptr;  // Current scope
    منطقي m_inSampled = خطأ;  // True inside a sampled expression

    // METHODS

    // Create a variable in the current scope, assigned 'valuep' at the start of each evaluation
    AstVarScope* createSampledVar(V3SharedTmps& tmps, Ast_نود_اوڠکاڤن* valuep,
                                  ثابت std::string& name) {
        File_باريس* ثابت flp = valuep->fileline();
        AstVarScope* ثابت vscp = tmps.make(flp, m_scopep, valuep->dtypep(), name);
        vscp->varp()->sampled(true);
        AstVarRef* ثابت lhsp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
        m_sampleFuncp->addStmtsp(جديد AstAssign{flp, lhsp, valuep});
        UINFO(4, "New Sampled: " << vscp);
        أرجع vscp;
    }

    // VISITORS
    فراغ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstSampled* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = true;
        iterateChildren(nodep);
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(تولق_Deletep(nodep), nodep);
    }
    فراغ visit(AstCMethodHard* nodep) override {
        إذا (!m_inSampled || nodep->method() != VCMethod::FORCE_READ) {
            iterateChildren(nodep);
            أرجع;
        }
        // Force vectors hold pointers to live RHS shadows. Copying the vector and base
        // separately would let later writes change the sampled value (IEEE 1800-2023 16.5.1).
        // Whole-signal reads have no user expressions to evaluate at the point of use.
        ثابت auto pair = m_forceSamples.emplace(*nodep, nullptr);
        AstVarScope*& vscp = pair.first->second;
        File_باريس* ثابت flp = nodep->fileline();
        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);
        إذا (pair.second) {
            vscp = createSampledVar(m_forceTmps, nodep, "");
        } وإلا {
            VL_DO_DANGLING(تولق_Deletep(nodep), nodep);
        }
        AstVarRef* ثابت refp = جديد AstVarRef{flp, vscp, VAccess::READ};
        refp->user1SetOnce();
        relinker.relink(refp);
    }
    فراغ visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        إذا (m_inSampled && !nodep->user1SetOnce()) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Should have failed in V3Access");
            AstVarScope* ثابت vscp = nodep->varScopep();
            إذا (!vscp->user1p()) {
                AstVarRef* ثابت valuep = جديد AstVarRef{vscp->fileline(), vscp, VAccess::READ};
                vscp->user1p(createSampledVar(m_sampledTmps, valuep, vscp->varp()->name()));
            }
            AstVarScope* ثابت sampledp = VN_AS(vscp->user1p(), VarScope);
            Ast_نود* ثابت newp = جديد AstVarRef{nodep->fileline(), sampledp, VAccess::READ};
            newp->user1SetOnce();  // Don't sample this one
            nodep->replaceWith(newp);
            VL_DO_DANGLING(تولق_Deletep(nodep), nodep);
        }
    }

    //--------------------
    فراغ visit(Ast_نود* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit SampledVisitor(AstNetlist* netlistp)
        : m_sampleFuncp{netlistp->evalFuncp(VEval::SAMPLE)} {
        iterate(netlistp);
        V3Sched::util::splitCheck(m_sampleFuncp);
    }
    ~SampledVisitor() override = افتراضي;
};

//######################################################################
// Sampled class functions

فراغ V3Sampled::sampledAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SampledVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ڤوکوق("sampled", 0, dump_ڤوکوق_Either_Level() >= 3);
}
