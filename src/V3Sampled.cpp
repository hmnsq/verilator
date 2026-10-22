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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Sampled.h"

#അന്തര്ഭാവ "V3Sched.h"
#അന്തര്ഭാവ "V3SharedTmps.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clock state, as a visitor of each AstNode

വർഗ്ഗം SampledVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()  -> AstVarScope*. The VarScope that stores sampled value
    //  AstVarRef::user1()    -> bool. Whether already converted
    സ്ഥിരം VNUser1InUse m_user1InUse;

    // STATE - across all visitors
    AstCFunc* സ്ഥിരം m_sampleFuncp;  // The 'sample' eval function assigning the sampled values
    V3SharedTmps m_sampledTmps{"__Vsampled", VVarType::MODULETEMP};  // Sampled variables
    V3SharedTmps m_forceTmps{"__VsampledForce", VVarType::MODULETEMP};  // Sampled force values
    // Keys remain in the tree as the sampled variables' value expressions.
    std::unordered_map<VNRef<Ast_കണ്ണി>, AstVarScope*> m_forceSamples;

    // STATE - for current visit position (use VL_RESTORER)
    AstScope* m_scopep = nullptr;  // Current scope
    തർക്കം m_inSampled = അസത്യം;  // True inside a sampled expression

    // METHODS

    // Create a variable in the current scope, assigned 'valuep' at the start of each evaluation
    AstVarScope* createSampledVar(V3SharedTmps& tmps, Ast_കണ്ണി_വ്യഞ്ജകം* valuep,
                                  സ്ഥിരം std::string& name) {
        File_വരി* സ്ഥിരം flp = valuep->fileline();
        AstVarScope* സ്ഥിരം vscp = tmps.make(flp, m_scopep, valuep->dtypep(), name);
        vscp->varp()->sampled(സത്യം);
        AstVarRef* സ്ഥിരം lhsp = പുതിയ AstVarRef{flp, vscp, VAccess::WRITE};
        m_sampleFuncp->addStmtsp(പുതിയ AstAssign{flp, lhsp, valuep});
        UINFO(4, "New Sampled: " << vscp);
        തിരികെ vscp;
    }

    // VISITORS
    ശൂന്യം visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstSampled* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = സത്യം;
        iterateChildren(nodep);
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstCMethodHard* nodep) override {
        എങ്കിൽ (!m_inSampled || nodep->method() != VCMethod::FORCE_READ) {
            iterateChildren(nodep);
            തിരികെ;
        }
        // Force vectors hold pointers to live RHS shadows. Copying the vector and base
        // separately would let later writes change the sampled value (IEEE 1800-2023 16.5.1).
        // Whole-signal reads have no user expressions to evaluate at the point of use.
        സ്ഥിരം auto pair = m_forceSamples.emplace(*nodep, nullptr);
        AstVarScope*& vscp = pair.first->second;
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);
        എങ്കിൽ (pair.second) {
            vscp = createSampledVar(m_forceTmps, nodep, "");
        } അല്ലെങ്കിൽ {
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
        AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{flp, vscp, VAccess::READ};
        refp->user1SetOnce();
        relinker.relink(refp);
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (m_inSampled && !nodep->user1SetOnce()) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Should have failed in V3Access");
            AstVarScope* സ്ഥിരം vscp = nodep->varScopep();
            എങ്കിൽ (!vscp->user1p()) {
                AstVarRef* സ്ഥിരം valuep = പുതിയ AstVarRef{vscp->fileline(), vscp, VAccess::READ};
                vscp->user1p(createSampledVar(m_sampledTmps, valuep, vscp->varp()->name()));
            }
            AstVarScope* സ്ഥിരം sampledp = VN_AS(vscp->user1p(), VarScope);
            Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstVarRef{nodep->fileline(), sampledp, VAccess::READ};
            newp->user1SetOnce();  // Don't sample this one
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }

    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit SampledVisitor(AstNetlist* netlistp)
        : m_sampleFuncp{netlistp->evalFuncp(VEval::SAMPLE)} {
        iterate(netlistp);
        V3Sched::util::splitCheck(m_sampleFuncp);
    }
    ~SampledVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Sampled class functions

ശൂന്യം V3Sampled::sampledAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SampledVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("sampled", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
