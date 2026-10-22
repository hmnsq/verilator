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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"
#ଅନ୍ତର୍ଭାଵ "V3Order.h"
#ଅନ୍ତର୍ଭାଵ "V3Sched.h"
#ଅନ୍ତର୍ଭାଵ "V3SenExprBuilder.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ V3Sched {
ନାମାକାଶ util {

AstCFunc* makeSubFunction(AstNetlist* netlistp, ଅଚଳ string& name, ତାର୍କିକ slow) {
    AstScope* ଅଚଳ scopeTopp = netlistp->ଶୀର୍ଷ_Scopep()->scopep();
    AstCFunc* ଅଚଳ funcp = ନୂତନ AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(ସତ୍ୟ);
    funcp->isStatic(ମିଥ୍ୟା);
    funcp->isLoose(ସତ୍ୟ);
    funcp->slow(slow);
    funcp->isConst(ମିଥ୍ୟା);
    funcp->declPrivate(ସତ୍ୟ);
    scopeTopp->addBlocksp(funcp);
    ଫେରନ୍ତୁ funcp;
}

AstVarScope* newArgument(AstCFunc* funcp, Ast_ଗଣ୍ଠି_DType* dtypep, ଅଚଳ string& name,
                         VDirection direction) {
    File_ଧାଡ଼ି* ଅଚଳ flp = funcp->fileline();
    AstScope* ଅଚଳ scopep = funcp->scopep();
    AstVar* ଅଚଳ varp = ନୂତନ AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(ସତ୍ୟ);
    varp->direction(direction);
    funcp->addArgsp(varp);
    AstVarScope* ଅଚଳ vscp = ନୂତନ AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    ଫେରନ୍ତୁ vscp;
}

Ast_ଗଣ୍ଠି_Stmt* setVar(AstVarScope* vscp, uint32_t val) {
    File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
    AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* ଅଚଳ valp = ନୂତନ AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->ଅଙ୍କ().setLong(val);
    ଫେରନ୍ତୁ ନୂତନ AstAssign{flp, refp, valp};
}

Ast_ଗଣ୍ଠି_Stmt* incrementVar(AstVarScope* vscp) {
    File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
    AstVarRef* ଅଚଳ wrefp = ନୂତନ AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* ଅଚଳ rrefp = ନୂତନ AstVarRef{flp, vscp, VAccess::READ};
    AstConst* ଅଚଳ onep = ନୂତନ AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->ଅଙ୍କ().setLong(1);
    ଫେରନ୍ତୁ ନୂତନ AstAssign{flp, wrefp, ନୂତନ AstAdd{flp, rrefp, onep}};
}

Ast_ଗଣ୍ଠି_Stmt* callVoidFunc(AstCFunc* funcp) {
    ଯଦି (!funcp) ଫେରନ୍ତୁ nullptr;
    AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{funcp->fileline(), funcp};
    callp->dtypeSetVoid();
    ଫେରନ୍ତୁ callp->makeStmt();
}

ସ୍ଥିର AstCFunc* splitCheckCreateNewSubFunc(AstCFunc* ofuncp) {
    ସ୍ଥିର std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    ଅଚଳ uint32_t func_ଅଙ୍କ = s_funcNums[ofuncp]++;
    ଅଚଳ std::string name = ofuncp->name() + "__" + cvtToStr(func_ଅଙ୍କ);
    AstScope* ଅଚଳ scopep = ofuncp->scopep();
    AstCFunc* ଅଚଳ subFuncp = ନୂତନ AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->addBlocksp(subFuncp);
    subFuncp->dontCombine(ସତ୍ୟ);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(ସତ୍ୟ);
    subFuncp->slow(ofuncp->slow());
    subFuncp->declPrivate(ofuncp->declPrivate());
    ଯଦି (ofuncp->needProcess()) subFuncp->setNeedProcess();
    ପାଇଁ (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* ଅଚଳ clonep = argp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        subFuncp->addArgsp(clonep);
        AstVarScope* ଅଚଳ vscp = ନୂତନ AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->addVarsp(vscp);
        argp->user3p(vscp);
    }
    ଫେରନ୍ତୁ subFuncp;
};

ଶୂନ୍ୟ splitCheckFinishSubFunc(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             ଅଚଳ std::unordered_map<ଅଚଳ AstVar*, AstVarScope*>& argVscps) {
    File_ଧାଡ଼ି* ଅଚଳ flp = subFuncp->fileline();
    AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtypeSetVoid();
    // Pass arguments through to subfunction
    ପାଇଁ (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->addArgsp(ନୂତନ AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    ତାର୍କିକ containsAwait = ମିଥ୍ୟା;
    subFuncp->foreach([&](Ast_ଗଣ୍ଠି* nodep) {
        // Record if it has a CAwait
        ଯଦି (VN_IS(nodep, CAwait)) containsAwait = ସତ୍ୟ;
        // Redirect references to arguments to the clone in the sub-function
        ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(nodep, VarRef)) {
            ଯଦି (AstVarScope* ଅଚଳ vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    ଯଦି (ofuncp->isCoroutine() && containsAwait) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->addStmtsp(ନୂତନ AstCAwait{flp, callp});
    } ଅନ୍ୟଥା {
        ofuncp->addStmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
ସ୍ଥିର std::vector<ତାର୍କିକ> splitCheckBreakable(ଅଚଳ AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::vector<ଅଚଳ Ast_ଗଣ୍ଠି*> stmtps;
    std::unordered_map<ଅଚଳ AstVar*, size_t> declIdx;  // Local var -> index it is declared at
    ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.size());
        stmtps.ଠେଲ_back(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::vector<size_t> lastUse(stmtps.size());
    ପାଇଁ (size_t i = 0; i < stmtps.size(); ++i) {
        lastUse[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* refp) {
            ଅଚଳ auto it = declIdx.find(refp->varp());  // 'end()' if not one of our locals
            ଯଦି (it != declIdx.end()) lastUse[it->second] = std::max(lastUse[it->second], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::vector<ତାର୍କିକ> breakable(stmtps.size(), ସତ୍ୟ);
    size_t liveEnd = 0;  // Last index any so far declared local is referenced at
    ପାଇଁ (size_t i = 0; i < stmtps.size(); ++i) {
        breakable[i] = liveEnd < i;
        ଯଦି (VN_IS(stmtps[i], Var)) liveEnd = std::max(liveEnd, lastUse[i]);
    }
    ଫେରନ୍ତୁ breakable;
}

// Split large function according to --output-split-cfuncs
ଶୂନ୍ୟ splitCheck(AstCFunc* ଅଚଳ ofuncp) {
    ଯଦି (!ofuncp) ଫେରନ୍ତୁ;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    ଯଦି (!v3Global.opt.outputSplitCFuncs() || !ofuncp->stmtsp()) ଫେରନ୍ତୁ;
    ଯଦି (ofuncp->ଗଣ୍ଠି_ଗଣନା() < v3Global.opt.outputSplitCFuncs()) ଫେରନ୍ତୁ;

    // Statement boundaries that would separate a local declaration from a reference to it
    ଅଚଳ std::vector<ତାର୍କିକ> breakable = splitCheckBreakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<ଅଚଳ AstVar*, AstVarScope*> argVscps;
    ପାଇଁ (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.size() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        ତାର୍କିକ found = ମିଥ୍ୟା;
        ପାଇଁ (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            ଯଦି (vscp->varp() != argp) ଜାରି;
            argVscps[argp] = vscp;
            found = ସତ୍ୟ;
            ଭାଙ୍ଗ;
        }
        UASSERT_OBJ(found, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    ଅଚଳ VNUser3InUse user3InUse;

    size_t size = 0;
    AstCFunc* subFuncp = nullptr;

    // Move statements one by one to the new sub-functions
    Ast_ଗଣ୍ଠି* stmtsp = ofuncp->stmtsp()->unlinkFrBackWithNext();
    ପାଇଁ (size_t index = 0; Ast_ଗଣ୍ଠି* ଅଚଳ itemp = stmtsp; ++index) {
        stmtsp = stmtsp->nextp();
        ଯଦି (stmtsp) stmtsp->unlinkFrBackWithNext();
        ଅଚଳ size_t itemSize = static_cast<size_t>(itemp->ଗଣ୍ଠି_ଗଣନା());
        size += itemSize;

        ଯଦି (size > static_cast<size_t>(v3Global.opt.outputSplitCFuncs()) && breakable[index]) {
            ଯଦି (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
            subFuncp = nullptr;
            size = itemSize;
        }

        ଯଦି (!subFuncp) subFuncp = splitCheckCreateNewSubFunc(ofuncp);
        subFuncp->addStmtsp(itemp);
    }
    ଯଦି (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
