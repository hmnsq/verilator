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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3EmitCBase.h"
#അന്തര്ഭാവ "V3EmitV.h"
#അന്തര്ഭാവ "V3Order.h"
#അന്തര്ഭാവ "V3Sched.h"
#അന്തര്ഭാവ "V3SenExprBuilder.h"
#അന്തര്ഭാവ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

നാമാകാശ V3Sched {
നാമാകാശ util {

AstCFunc* makeSubFunction(AstNetlist* netlistp, സ്ഥിരം string& name, തർക്കം slow) {
    AstScope* സ്ഥിരം scopeTopp = netlistp->മുകൾ_Scopep()->scopep();
    AstCFunc* സ്ഥിരം funcp = പുതിയ AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(സത്യം);
    funcp->isStatic(അസത്യം);
    funcp->isLoose(സത്യം);
    funcp->slow(slow);
    funcp->isConst(അസത്യം);
    funcp->declPrivate(സത്യം);
    scopeTopp->addBlocksp(funcp);
    തിരികെ funcp;
}

AstVarScope* newArgument(AstCFunc* funcp, Ast_കണ്ണി_DType* dtypep, സ്ഥിരം string& name,
                         VDirection direction) {
    File_വരി* സ്ഥിരം flp = funcp->fileline();
    AstScope* സ്ഥിരം scopep = funcp->scopep();
    AstVar* സ്ഥിരം varp = പുതിയ AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(സത്യം);
    varp->direction(direction);
    funcp->addArgsp(varp);
    AstVarScope* സ്ഥിരം vscp = പുതിയ AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    തിരികെ vscp;
}

Ast_കണ്ണി_Stmt* setVar(AstVarScope* vscp, uint32_t val) {
    File_വരി* സ്ഥിരം flp = vscp->fileline();
    AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* സ്ഥിരം valp = പുതിയ AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->അക്കം().setLong(val);
    തിരികെ പുതിയ AstAssign{flp, refp, valp};
}

Ast_കണ്ണി_Stmt* incrementVar(AstVarScope* vscp) {
    File_വരി* സ്ഥിരം flp = vscp->fileline();
    AstVarRef* സ്ഥിരം wrefp = പുതിയ AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* സ്ഥിരം rrefp = പുതിയ AstVarRef{flp, vscp, VAccess::READ};
    AstConst* സ്ഥിരം onep = പുതിയ AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->അക്കം().setLong(1);
    തിരികെ പുതിയ AstAssign{flp, wrefp, പുതിയ AstAdd{flp, rrefp, onep}};
}

Ast_കണ്ണി_Stmt* callVoidFunc(AstCFunc* funcp) {
    എങ്കിൽ (!funcp) തിരികെ nullptr;
    AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{funcp->fileline(), funcp};
    callp->dtypeSetVoid();
    തിരികെ callp->makeStmt();
}

സ്ഥിതം AstCFunc* splitCheckCreateNewSubFunc(AstCFunc* ofuncp) {
    സ്ഥിതം std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    സ്ഥിരം uint32_t func_അക്കം = s_funcNums[ofuncp]++;
    സ്ഥിരം std::string name = ofuncp->name() + "__" + cvtToStr(func_അക്കം);
    AstScope* സ്ഥിരം scopep = ofuncp->scopep();
    AstCFunc* സ്ഥിരം subFuncp = പുതിയ AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->addBlocksp(subFuncp);
    subFuncp->dontCombine(സത്യം);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(സത്യം);
    subFuncp->slow(ofuncp->slow());
    subFuncp->declPrivate(ofuncp->declPrivate());
    എങ്കിൽ (ofuncp->needProcess()) subFuncp->setNeedProcess();
    വേണ്ടി (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* സ്ഥിരം clonep = argp->clone_വൃക്ഷം(അസത്യം);
        subFuncp->addArgsp(clonep);
        AstVarScope* സ്ഥിരം vscp = പുതിയ AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->addVarsp(vscp);
        argp->user3p(vscp);
    }
    തിരികെ subFuncp;
};

ശൂന്യം splitCheckFinishSubFunc(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             സ്ഥിരം std::unordered_map<സ്ഥിരം AstVar*, AstVarScope*>& argVscps) {
    File_വരി* സ്ഥിരം flp = subFuncp->fileline();
    AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtypeSetVoid();
    // Pass arguments through to subfunction
    വേണ്ടി (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->addArgsp(പുതിയ AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    തർക്കം containsAwait = അസത്യം;
    subFuncp->foreach([&](Ast_കണ്ണി* nodep) {
        // Record if it has a CAwait
        എങ്കിൽ (VN_IS(nodep, CAwait)) containsAwait = സത്യം;
        // Redirect references to arguments to the clone in the sub-function
        എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(nodep, VarRef)) {
            എങ്കിൽ (AstVarScope* സ്ഥിരം vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    എങ്കിൽ (ofuncp->isCoroutine() && containsAwait) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->addStmtsp(പുതിയ AstCAwait{flp, callp});
    } അല്ലെങ്കിൽ {
        ofuncp->addStmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
സ്ഥിതം std::vector<തർക്കം> splitCheckBreakable(സ്ഥിരം AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::vector<സ്ഥിരം Ast_കണ്ണി*> stmtps;
    std::unordered_map<സ്ഥിരം AstVar*, size_t> declIdx;  // Local var -> index it is declared at
    വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.size());
        stmtps.തള്ളുക_back(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::vector<size_t> lastUse(stmtps.size());
    വേണ്ടി (size_t i = 0; i < stmtps.size(); ++i) {
        lastUse[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](സ്ഥിരം Ast_കണ്ണി_Var_Ref* refp) {
            സ്ഥിരം auto it = declIdx.find(refp->varp());  // 'end()' if not one of our locals
            എങ്കിൽ (it != declIdx.end()) lastUse[it->second] = std::max(lastUse[it->second], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::vector<തർക്കം> breakable(stmtps.size(), സത്യം);
    size_t liveEnd = 0;  // Last index any so far declared local is referenced at
    വേണ്ടി (size_t i = 0; i < stmtps.size(); ++i) {
        breakable[i] = liveEnd < i;
        എങ്കിൽ (VN_IS(stmtps[i], Var)) liveEnd = std::max(liveEnd, lastUse[i]);
    }
    തിരികെ breakable;
}

// Split large function according to --output-split-cfuncs
ശൂന്യം splitCheck(AstCFunc* സ്ഥിരം ofuncp) {
    എങ്കിൽ (!ofuncp) തിരികെ;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    എങ്കിൽ (!v3Global.opt.outputSplitCFuncs() || !ofuncp->stmtsp()) തിരികെ;
    എങ്കിൽ (ofuncp->കണ്ണി_എണ്ണം() < v3Global.opt.outputSplitCFuncs()) തിരികെ;

    // Statement boundaries that would separate a local declaration from a reference to it
    സ്ഥിരം std::vector<തർക്കം> breakable = splitCheckBreakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<സ്ഥിരം AstVar*, AstVarScope*> argVscps;
    വേണ്ടി (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.size() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        തർക്കം found = അസത്യം;
        വേണ്ടി (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            എങ്കിൽ (vscp->varp() != argp) തുടരുക;
            argVscps[argp] = vscp;
            found = സത്യം;
            നിർത്തുക;
        }
        UASSERT_OBJ(found, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    സ്ഥിരം VNUser3InUse user3InUse;

    size_t size = 0;
    AstCFunc* subFuncp = nullptr;

    // Move statements one by one to the new sub-functions
    Ast_കണ്ണി* stmtsp = ofuncp->stmtsp()->unlinkFrBackWithNext();
    വേണ്ടി (size_t index = 0; Ast_കണ്ണി* സ്ഥിരം itemp = stmtsp; ++index) {
        stmtsp = stmtsp->nextp();
        എങ്കിൽ (stmtsp) stmtsp->unlinkFrBackWithNext();
        സ്ഥിരം size_t itemSize = static_cast<size_t>(itemp->കണ്ണി_എണ്ണം());
        size += itemSize;

        എങ്കിൽ (size > static_cast<size_t>(v3Global.opt.outputSplitCFuncs()) && breakable[index]) {
            എങ്കിൽ (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
            subFuncp = nullptr;
            size = itemSize;
        }

        എങ്കിൽ (!subFuncp) subFuncp = splitCheckCreateNewSubFunc(ofuncp);
        subFuncp->addStmtsp(itemp);
    }
    എങ്കിൽ (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
