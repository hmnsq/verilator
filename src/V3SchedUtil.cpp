// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Utility functions used by code scheduling
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
//
//
//
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3EmitCBase.h"
#అన్తర్భావ "V3EmitV.h"
#అన్తర్భావ "V3Order.h"
#అన్తర్భావ "V3Sched.h"
#అన్తర్భావ "V3SenExprBuilder.h"
#అన్తర్భావ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ V3Sched {
నామాకాశ util {

AstCFunc* makeSubFunction(AstNetlist* netlistp, స్థిరరాశి string& name, తార్కిక slow) {
    AstScope* స్థిరరాశి scopeTopp = netlistp->శిఖరం_Scopep()->scopep();
    AstCFunc* స్థిరరాశి funcp = కొత్త AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(నిజం);
    funcp->isStatic(అబద్ధం);
    funcp->isLoose(నిజం);
    funcp->slow(slow);
    funcp->isConst(అబద్ధం);
    funcp->decl_వ్యక్తిగత(నిజం);
    scopeTopp->కూడు_Blocksp(funcp);
    తిరిగి funcp;
}

AstVarScope* new_వాదం(AstCFunc* funcp, Ast_కణుపు_DType* dtypep, స్థిరరాశి string& name,
                         VDirection direction) {
    File_పంక్తి* స్థిరరాశి flp = funcp->fileline();
    AstScope* స్థిరరాశి scopep = funcp->scopep();
    AstVar* స్థిరరాశి varp = కొత్త AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(నిజం);
    varp->direction(direction);
    funcp->కూడు_Argsp(varp);
    AstVarScope* స్థిరరాశి vscp = కొత్త AstVarScope{flp, scopep, varp};
    scopep->కూడు_Varsp(vscp);
    తిరిగి vscp;
}

Ast_కణుపు_Stmt* సమితి_Var(AstVarScope* vscp, uint32_t val) {
    File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
    AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* స్థిరరాశి valp = కొత్త AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->అంకె().సమితి_Long(val);
    తిరిగి కొత్త Ast_కేటాయించు{flp, refp, valp};
}

Ast_కణుపు_Stmt* incrementVar(AstVarScope* vscp) {
    File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
    AstVarRef* స్థిరరాశి wrefp = కొత్త AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* స్థిరరాశి rrefp = కొత్త AstVarRef{flp, vscp, VAccess::READ};
    AstConst* స్థిరరాశి onep = కొత్త AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->అంకె().సమితి_Long(1);
    తిరిగి కొత్త Ast_కేటాయించు{flp, wrefp, కొత్త Ast_కూడు{flp, rrefp, onep}};
}

Ast_కణుపు_Stmt* callVoidFunc(AstCFunc* funcp) {
    ఒకవేళ (!funcp) తిరిగి శూన్యసూచిక;
    AstCCall* స్థిరరాశి callp = కొత్త AstCCall{funcp->fileline(), funcp};
    callp->dtype_సమితి_Void();
    తిరిగి callp->makeStmt();
}

స్థిర AstCFunc* విభజించు_Check_Create_New_Sub_Func(AstCFunc* ofuncp) {
    స్థిర std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    స్థిరరాశి uint32_t func_అంకె = s_funcNums[ofuncp]++;
    స్థిరరాశి std::string name = ofuncp->name() + "__" + cvt_గమ్యం_Str(func_అంకె);
    AstScope* స్థిరరాశి scopep = ofuncp->scopep();
    AstCFunc* స్థిరరాశి subFuncp = కొత్త AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->కూడు_Blocksp(subFuncp);
    subFuncp->dontCombine(నిజం);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(నిజం);
    subFuncp->slow(ofuncp->slow());
    subFuncp->decl_వ్యక్తిగత(ofuncp->decl_వ్యక్తిగత());
    ఒకవేళ (ofuncp->needProcess()) subFuncp->సమితి_Need_Process();
    కొరకు (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* స్థిరరాశి clonep = argp->clone_వృక్షం(అబద్ధం);
        subFuncp->కూడు_Argsp(clonep);
        AstVarScope* స్థిరరాశి vscp = కొత్త AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->కూడు_Varsp(vscp);
        argp->user3p(vscp);
    }
    తిరిగి subFuncp;
};

శూన్య విభజించు_Check_ముగింపు_Sub_Func(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             స్థిరరాశి std::unordered_map<స్థిరరాశి AstVar*, AstVarScope*>& argVscps) {
    File_పంక్తి* స్థిరరాశి flp = subFuncp->fileline();
    AstCCall* స్థిరరాశి callp = కొత్త AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtype_సమితి_Void();
    // Pass arguments through to subfunction
    కొరకు (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->కూడు_Argsp(కొత్త AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    తార్కిక కలిగియుంది_Await = అబద్ధం;
    subFuncp->foreach([&](Ast_కణుపు* nodep) {
        // Record if it has a CAwait
        ఒకవేళ (VN_IS(nodep, CAwait)) కలిగియుంది_Await = నిజం;
        // Redirect references to arguments to the clone in the sub-function
        ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(nodep, VarRef)) {
            ఒకవేళ (AstVarScope* స్థిరరాశి vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    ఒకవేళ (ofuncp->isCoroutine() && కలిగియుంది_Await) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->కూడు_Stmtsp(కొత్త AstCAwait{flp, callp});
    } లేకపోతే {
        ofuncp->కూడు_Stmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
స్థిర std::సదిశ<తార్కిక> విభజించు_Check_Breakable(స్థిరరాశి AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::సదిశ<స్థిరరాశి Ast_కణుపు*> stmtps;
    std::unordered_map<స్థిరరాశి AstVar*, కొలత_t> declIdx;  // Local var -> index it is declared at
    కొరకు (స్థిరరాశి Ast_కణుపు* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.కొలత());
        stmtps.నెట్టు_వెనుక(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::సదిశ<కొలత_t> చివరి_Use(stmtps.కొలత());
    కొరకు (కొలత_t i = 0; i < stmtps.కొలత(); ++i) {
        చివరి_Use[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](స్థిరరాశి Ast_కణుపు_Var_Ref* refp) {
            స్థిరరాశి స్వయంచాలక it = declIdx.కనుగొను(refp->varp());  // 'end()' if not one of our locals
            ఒకవేళ (it != declIdx.చివర()) చివరి_Use[it->రెండవ] = std::max(చివరి_Use[it->రెండవ], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::సదిశ<తార్కిక> breakable(stmtps.కొలత(), నిజం);
    కొలత_t సజీవ_చివర = 0;  // Last index any so far declared local is referenced at
    కొరకు (కొలత_t i = 0; i < stmtps.కొలత(); ++i) {
        breakable[i] = సజీవ_చివర < i;
        ఒకవేళ (VN_IS(stmtps[i], Var)) సజీవ_చివర = std::max(సజీవ_చివర, చివరి_Use[i]);
    }
    తిరిగి breakable;
}

// Split large function according to --output-split-cfuncs
శూన్య విభజించు_Check(AstCFunc* స్థిరరాశి ofuncp) {
    ఒకవేళ (!ofuncp) తిరిగి;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    ఒకవేళ (!v3Global.opt.output_విభజించు_CFuncs() || !ofuncp->stmtsp()) తిరిగి;
    ఒకవేళ (ofuncp->కణుపు_లెక్క() < v3Global.opt.output_విభజించు_CFuncs()) తిరిగి;

    // Statement boundaries that would separate a local declaration from a reference to it
    స్థిరరాశి std::సదిశ<తార్కిక> breakable = విభజించు_Check_Breakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<స్థిరరాశి AstVar*, AstVarScope*> argVscps;
    కొరకు (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.కొలత() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        తార్కిక దొరికింది = అబద్ధం;
        కొరకు (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            ఒకవేళ (vscp->varp() != argp) కొనసాగు;
            argVscps[argp] = vscp;
            దొరికింది = నిజం;
            ఆపు;
        }
        UASSERT_OBJ(దొరికింది, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    స్థిరరాశి VNUser3_లోపల_Use user3_లోపల_Use;

    కొలత_t కొలత = 0;
    AstCFunc* subFuncp = శూన్యసూచిక;

    // Move statements one by one to the new sub-functions
    Ast_కణుపు* stmtsp = ofuncp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి();
    కొరకు (కొలత_t సూచీ = 0; Ast_కణుపు* స్థిరరాశి itemp = stmtsp; ++సూచీ) {
        stmtsp = stmtsp->nextp();
        ఒకవేళ (stmtsp) stmtsp->unlink_Fr_వెనుక_With_తదుపరి();
        స్థిరరాశి కొలత_t అంశం_కొలత = static_cast<కొలత_t>(itemp->కణుపు_లెక్క());
        కొలత += అంశం_కొలత;

        ఒకవేళ (కొలత > static_cast<కొలత_t>(v3Global.opt.output_విభజించు_CFuncs()) && breakable[సూచీ]) {
            ఒకవేళ (subFuncp) విభజించు_Check_ముగింపు_Sub_Func(ofuncp, subFuncp, argVscps);
            subFuncp = శూన్యసూచిక;
            కొలత = అంశం_కొలత;
        }

        ఒకవేళ (!subFuncp) subFuncp = విభజించు_Check_Create_New_Sub_Func(ofuncp);
        subFuncp->కూడు_Stmtsp(itemp);
    }
    ఒకవేళ (subFuncp) విభజించు_Check_ముగింపు_Sub_Func(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
