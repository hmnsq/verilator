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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3EmitCBase.h"
#அந்தர்பாவ "V3EmitV.h"
#அந்தர்பாவ "V3Order.h"
#அந்தர்பாவ "V3Sched.h"
#அந்தர்பாவ "V3SenExprBuilder.h"
#அந்தர்பாவ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ V3Sched {
நாமாகாஶ util {

AstCFunc* makeSubFunction(AstNetlist* netlistp, மாறிலி string& name, தருக்க slow) {
    AstScope* மாறிலி scopeTopp = netlistp->உச்சி_Scopep()->scopep();
    AstCFunc* மாறிலி funcp = புதிய AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(உண்மை);
    funcp->isStatic(பொய்);
    funcp->isLoose(உண்மை);
    funcp->slow(slow);
    funcp->isConst(பொய்);
    funcp->declPrivate(உண்மை);
    scopeTopp->addBlocksp(funcp);
    திரும்பு funcp;
}

AstVarScope* newArgument(AstCFunc* funcp, Ast_முனை_DType* dtypep, மாறிலி string& name,
                         VDirection direction) {
    File_வரி* மாறிலி flp = funcp->fileline();
    AstScope* மாறிலி scopep = funcp->scopep();
    AstVar* மாறிலி varp = புதிய AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(உண்மை);
    varp->direction(direction);
    funcp->addArgsp(varp);
    AstVarScope* மாறிலி vscp = புதிய AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    திரும்பு vscp;
}

Ast_முனை_Stmt* setVar(AstVarScope* vscp, uint32_t val) {
    File_வரி* மாறிலி flp = vscp->fileline();
    AstVarRef* மாறிலி refp = புதிய AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* மாறிலி valp = புதிய AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->இலக்கம்().setLong(val);
    திரும்பு புதிய AstAssign{flp, refp, valp};
}

Ast_முனை_Stmt* incrementVar(AstVarScope* vscp) {
    File_வரி* மாறிலி flp = vscp->fileline();
    AstVarRef* மாறிலி wrefp = புதிய AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* மாறிலி rrefp = புதிய AstVarRef{flp, vscp, VAccess::READ};
    AstConst* மாறிலி onep = புதிய AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->இலக்கம்().setLong(1);
    திரும்பு புதிய AstAssign{flp, wrefp, புதிய AstAdd{flp, rrefp, onep}};
}

Ast_முனை_Stmt* callVoidFunc(AstCFunc* funcp) {
    எனில் (!funcp) திரும்பு nullptr;
    AstCCall* மாறிலி callp = புதிய AstCCall{funcp->fileline(), funcp};
    callp->dtypeSetVoid();
    திரும்பு callp->makeStmt();
}

நிலையான AstCFunc* splitCheckCreateNewSubFunc(AstCFunc* ofuncp) {
    நிலையான std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    மாறிலி uint32_t func_இலக்கம் = s_funcNums[ofuncp]++;
    மாறிலி std::string name = ofuncp->name() + "__" + cvtToStr(func_இலக்கம்);
    AstScope* மாறிலி scopep = ofuncp->scopep();
    AstCFunc* மாறிலி subFuncp = புதிய AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->addBlocksp(subFuncp);
    subFuncp->dontCombine(உண்மை);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(உண்மை);
    subFuncp->slow(ofuncp->slow());
    subFuncp->declPrivate(ofuncp->declPrivate());
    எனில் (ofuncp->needProcess()) subFuncp->setNeedProcess();
    க்கு (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* மாறிலி clonep = argp->clone_மரம்(பொய்);
        subFuncp->addArgsp(clonep);
        AstVarScope* மாறிலி vscp = புதிய AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->addVarsp(vscp);
        argp->user3p(vscp);
    }
    திரும்பு subFuncp;
};

வெறுமை splitCheckFinishSubFunc(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             மாறிலி std::unordered_map<மாறிலி AstVar*, AstVarScope*>& argVscps) {
    File_வரி* மாறிலி flp = subFuncp->fileline();
    AstCCall* மாறிலி callp = புதிய AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtypeSetVoid();
    // Pass arguments through to subfunction
    க்கு (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->addArgsp(புதிய AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    தருக்க containsAwait = பொய்;
    subFuncp->foreach([&](Ast_முனை* nodep) {
        // Record if it has a CAwait
        எனில் (VN_IS(nodep, CAwait)) containsAwait = உண்மை;
        // Redirect references to arguments to the clone in the sub-function
        எனில் (AstVarRef* மாறிலி refp = VN_CAST(nodep, VarRef)) {
            எனில் (AstVarScope* மாறிலி vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    எனில் (ofuncp->isCoroutine() && containsAwait) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->addStmtsp(புதிய AstCAwait{flp, callp});
    } இல்லையெனில் {
        ofuncp->addStmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
நிலையான std::vector<தருக்க> splitCheckBreakable(மாறிலி AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::vector<மாறிலி Ast_முனை*> stmtps;
    std::unordered_map<மாறிலி AstVar*, size_t> declIdx;  // Local var -> index it is declared at
    க்கு (மாறிலி Ast_முனை* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.size());
        stmtps.தள்ளு_back(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::vector<size_t> lastUse(stmtps.size());
    க்கு (size_t i = 0; i < stmtps.size(); ++i) {
        lastUse[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](மாறிலி Ast_முனை_Var_Ref* refp) {
            மாறிலி auto it = declIdx.find(refp->varp());  // 'end()' if not one of our locals
            எனில் (it != declIdx.end()) lastUse[it->second] = std::max(lastUse[it->second], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::vector<தருக்க> breakable(stmtps.size(), உண்மை);
    size_t liveEnd = 0;  // Last index any so far declared local is referenced at
    க்கு (size_t i = 0; i < stmtps.size(); ++i) {
        breakable[i] = liveEnd < i;
        எனில் (VN_IS(stmtps[i], Var)) liveEnd = std::max(liveEnd, lastUse[i]);
    }
    திரும்பு breakable;
}

// Split large function according to --output-split-cfuncs
வெறுமை splitCheck(AstCFunc* மாறிலி ofuncp) {
    எனில் (!ofuncp) திரும்பு;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    எனில் (!v3Global.opt.outputSplitCFuncs() || !ofuncp->stmtsp()) திரும்பு;
    எனில் (ofuncp->முனை_மொத்தம்() < v3Global.opt.outputSplitCFuncs()) திரும்பு;

    // Statement boundaries that would separate a local declaration from a reference to it
    மாறிலி std::vector<தருக்க> breakable = splitCheckBreakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<மாறிலி AstVar*, AstVarScope*> argVscps;
    க்கு (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.size() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        தருக்க found = பொய்;
        க்கு (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            எனில் (vscp->varp() != argp) தொடர்;
            argVscps[argp] = vscp;
            found = உண்மை;
            நிறுத்து;
        }
        UASSERT_OBJ(found, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    மாறிலி VNUser3InUse user3InUse;

    size_t size = 0;
    AstCFunc* subFuncp = nullptr;

    // Move statements one by one to the new sub-functions
    Ast_முனை* stmtsp = ofuncp->stmtsp()->unlinkFrBackWithNext();
    க்கு (size_t index = 0; Ast_முனை* மாறிலி itemp = stmtsp; ++index) {
        stmtsp = stmtsp->nextp();
        எனில் (stmtsp) stmtsp->unlinkFrBackWithNext();
        மாறிலி size_t itemSize = static_cast<size_t>(itemp->முனை_மொத்தம்());
        size += itemSize;

        எனில் (size > static_cast<size_t>(v3Global.opt.outputSplitCFuncs()) && breakable[index]) {
            எனில் (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
            subFuncp = nullptr;
            size = itemSize;
        }

        எனில் (!subFuncp) subFuncp = splitCheckCreateNewSubFunc(ofuncp);
        subFuncp->addStmtsp(itemp);
    }
    எனில் (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
