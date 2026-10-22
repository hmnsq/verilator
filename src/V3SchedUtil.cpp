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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Order.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sched.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SenExprBuilder.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ V3Sched {
ਨਾਮਾਕਾਸ਼ util {

AstCFunc* makeSubFunction(AstNetlist* netlistp, ਅਚਲ string& name, ਤਾਰਕਿਕ slow) {
    AstScope* ਅਚਲ scopeTopp = netlistp->ਸਿਖਰ_Scopep()->scopep();
    AstCFunc* ਅਚਲ funcp = ਨਵਾਂ AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(ਸੱਚ);
    funcp->isStatic(ਝੂਠ);
    funcp->isLoose(ਸੱਚ);
    funcp->slow(slow);
    funcp->isConst(ਝੂਠ);
    funcp->decl_ਨਿੱਜੀ(ਸੱਚ);
    scopeTopp->ਜੋੜੋ_Blocksp(funcp);
    ਵਾਪਸ funcp;
}

AstVarScope* new_ਦਲੀਲ(AstCFunc* funcp, Ast_ਗੰਢ_DType* dtypep, ਅਚਲ string& name,
                         VDirection direction) {
    File_ਸਤਰ* ਅਚਲ flp = funcp->fileline();
    AstScope* ਅਚਲ scopep = funcp->scopep();
    AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(ਸੱਚ);
    varp->direction(direction);
    funcp->ਜੋੜੋ_Argsp(varp);
    AstVarScope* ਅਚਲ vscp = ਨਵਾਂ AstVarScope{flp, scopep, varp};
    scopep->ਜੋੜੋ_Varsp(vscp);
    ਵਾਪਸ vscp;
}

Ast_ਗੰਢ_Stmt* ਸੈੱਟ_Var(AstVarScope* vscp, uint32_t val) {
    File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
    AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* ਅਚਲ valp = ਨਵਾਂ AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->ਅੰਕ().ਸੈੱਟ_Long(val);
    ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, refp, valp};
}

Ast_ਗੰਢ_Stmt* incrementVar(AstVarScope* vscp) {
    File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
    AstVarRef* ਅਚਲ wrefp = ਨਵਾਂ AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* ਅਚਲ rrefp = ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ};
    AstConst* ਅਚਲ onep = ਨਵਾਂ AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->ਅੰਕ().ਸੈੱਟ_Long(1);
    ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, wrefp, ਨਵਾਂ Ast_ਜੋੜੋ{flp, rrefp, onep}};
}

Ast_ਗੰਢ_Stmt* callVoidFunc(AstCFunc* funcp) {
    ਜੇ (!funcp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{funcp->fileline(), funcp};
    callp->dtype_ਸੈੱਟ_Void();
    ਵਾਪਸ callp->makeStmt();
}

ਸਥਿਰ AstCFunc* ਵੰਡੋ_Check_Create_New_Sub_Func(AstCFunc* ofuncp) {
    ਸਥਿਰ std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    ਅਚਲ uint32_t func_ਅੰਕ = s_funcNums[ofuncp]++;
    ਅਚਲ std::string name = ofuncp->name() + "__" + cvt_ਮੰਜ਼ਿਲ_Str(func_ਅੰਕ);
    AstScope* ਅਚਲ scopep = ofuncp->scopep();
    AstCFunc* ਅਚਲ subFuncp = ਨਵਾਂ AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->ਜੋੜੋ_Blocksp(subFuncp);
    subFuncp->dontCombine(ਸੱਚ);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(ਸੱਚ);
    subFuncp->slow(ofuncp->slow());
    subFuncp->decl_ਨਿੱਜੀ(ofuncp->decl_ਨਿੱਜੀ());
    ਜੇ (ofuncp->needProcess()) subFuncp->ਸੈੱਟ_Need_Process();
    ਲਈ (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* ਅਚਲ clonep = argp->clone_ਰੁੱਖ(ਝੂਠ);
        subFuncp->ਜੋੜੋ_Argsp(clonep);
        AstVarScope* ਅਚਲ vscp = ਨਵਾਂ AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->ਜੋੜੋ_Varsp(vscp);
        argp->user3p(vscp);
    }
    ਵਾਪਸ subFuncp;
};

ਖਾਲੀ ਵੰਡੋ_Check_ਅੰਤ_Sub_Func(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             ਅਚਲ std::unordered_map<ਅਚਲ AstVar*, AstVarScope*>& argVscps) {
    File_ਸਤਰ* ਅਚਲ flp = subFuncp->fileline();
    AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtype_ਸੈੱਟ_Void();
    // Pass arguments through to subfunction
    ਲਈ (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->ਜੋੜੋ_Argsp(ਨਵਾਂ AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    ਤਾਰਕਿਕ ਸ਼ਾਮਲ_ਹੈ_Await = ਝੂਠ;
    subFuncp->foreach([&](Ast_ਗੰਢ* nodep) {
        // Record if it has a CAwait
        ਜੇ (VN_IS(nodep, CAwait)) ਸ਼ਾਮਲ_ਹੈ_Await = ਸੱਚ;
        // Redirect references to arguments to the clone in the sub-function
        ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(nodep, VarRef)) {
            ਜੇ (AstVarScope* ਅਚਲ vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    ਜੇ (ofuncp->isCoroutine() && ਸ਼ਾਮਲ_ਹੈ_Await) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCAwait{flp, callp});
    } ਨਹੀਂਤਾਂ {
        ofuncp->ਜੋੜੋ_Stmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
ਸਥਿਰ std::ਸਦਿਸ਼<ਤਾਰਕਿਕ> ਵੰਡੋ_Check_Breakable(ਅਚਲ AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::ਸਦਿਸ਼<ਅਚਲ Ast_ਗੰਢ*> stmtps;
    std::unordered_map<ਅਚਲ AstVar*, ਮਾਪ_t> declIdx;  // Local var -> index it is declared at
    ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.ਮਾਪ());
        stmtps.ਧੱਕੋ_ਪਿੱਠ(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::ਸਦਿਸ਼<ਮਾਪ_t> ਆਖਰੀ_Use(stmtps.ਮਾਪ());
    ਲਈ (ਮਾਪ_t i = 0; i < stmtps.ਮਾਪ(); ++i) {
        ਆਖਰੀ_Use[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](ਅਚਲ Ast_ਗੰਢ_Var_Ref* refp) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = declIdx.ਲੱਭੋ(refp->varp());  // 'end()' if not one of our locals
            ਜੇ (it != declIdx.ਸਿਰਾ()) ਆਖਰੀ_Use[it->ਦੂਜਾ] = std::max(ਆਖਰੀ_Use[it->ਦੂਜਾ], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::ਸਦਿਸ਼<ਤਾਰਕਿਕ> breakable(stmtps.ਮਾਪ(), ਸੱਚ);
    ਮਾਪ_t ਜੀਵਤ_ਸਿਰਾ = 0;  // Last index any so far declared local is referenced at
    ਲਈ (ਮਾਪ_t i = 0; i < stmtps.ਮਾਪ(); ++i) {
        breakable[i] = ਜੀਵਤ_ਸਿਰਾ < i;
        ਜੇ (VN_IS(stmtps[i], Var)) ਜੀਵਤ_ਸਿਰਾ = std::max(ਜੀਵਤ_ਸਿਰਾ, ਆਖਰੀ_Use[i]);
    }
    ਵਾਪਸ breakable;
}

// Split large function according to --output-split-cfuncs
ਖਾਲੀ ਵੰਡੋ_Check(AstCFunc* ਅਚਲ ofuncp) {
    ਜੇ (!ofuncp) ਵਾਪਸ;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    ਜੇ (!v3Global.opt.output_ਵੰਡੋ_CFuncs() || !ofuncp->stmtsp()) ਵਾਪਸ;
    ਜੇ (ofuncp->ਗੰਢ_ਗਿਣਤੀ() < v3Global.opt.output_ਵੰਡੋ_CFuncs()) ਵਾਪਸ;

    // Statement boundaries that would separate a local declaration from a reference to it
    ਅਚਲ std::ਸਦਿਸ਼<ਤਾਰਕਿਕ> breakable = ਵੰਡੋ_Check_Breakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<ਅਚਲ AstVar*, AstVarScope*> argVscps;
    ਲਈ (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.ਮਾਪ() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        ਤਾਰਕਿਕ ਮਿਲਿਆ = ਝੂਠ;
        ਲਈ (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            ਜੇ (vscp->varp() != argp) ਜਾਰੀ;
            argVscps[argp] = vscp;
            ਮਿਲਿਆ = ਸੱਚ;
            ਰੋਕੋ;
        }
        UASSERT_OBJ(ਮਿਲਿਆ, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    ਅਚਲ VNUser3_ਵਿੱਚ_Use user3_ਵਿੱਚ_Use;

    ਮਾਪ_t ਮਾਪ = 0;
    AstCFunc* subFuncp = ਖਾਲੀ_ਸੂਚਕ;

    // Move statements one by one to the new sub-functions
    Ast_ਗੰਢ* stmtsp = ofuncp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
    ਲਈ (ਮਾਪ_t ਸੂਚਕਾਂਕ = 0; Ast_ਗੰਢ* ਅਚਲ itemp = stmtsp; ++ਸੂਚਕਾਂਕ) {
        stmtsp = stmtsp->nextp();
        ਜੇ (stmtsp) stmtsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
        ਅਚਲ ਮਾਪ_t ਵਸਤੂ_ਮਾਪ = static_cast<ਮਾਪ_t>(itemp->ਗੰਢ_ਗਿਣਤੀ());
        ਮਾਪ += ਵਸਤੂ_ਮਾਪ;

        ਜੇ (ਮਾਪ > static_cast<ਮਾਪ_t>(v3Global.opt.output_ਵੰਡੋ_CFuncs()) && breakable[ਸੂਚਕਾਂਕ]) {
            ਜੇ (subFuncp) ਵੰਡੋ_Check_ਅੰਤ_Sub_Func(ofuncp, subFuncp, argVscps);
            subFuncp = ਖਾਲੀ_ਸੂਚਕ;
            ਮਾਪ = ਵਸਤੂ_ਮਾਪ;
        }

        ਜੇ (!subFuncp) subFuncp = ਵੰਡੋ_Check_Create_New_Sub_Func(ofuncp);
        subFuncp->ਜੋੜੋ_Stmtsp(itemp);
    }
    ਜੇ (subFuncp) ਵੰਡੋ_Check_ਅੰਤ_Sub_Func(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
