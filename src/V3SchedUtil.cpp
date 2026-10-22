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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3EmitCBase.h"
#ضمن "V3EmitV.h"
#ضمن "V3Order.h"
#ضمن "V3Sched.h"
#ضمن "V3SenExprBuilder.h"
#ضمن "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق V3Sched {
نطاق util {

AstCFunc* makeSubFunction(AstNetlist* netlistp, ثابت string& name, منطقي slow) {
    AstScope* ثابت scopeTopp = netlistp->topScopep()->scopep();
    AstCFunc* ثابت funcp = جديد AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(אמת);
    funcp->isStatic(שקר);
    funcp->isLoose(אמת);
    funcp->slow(slow);
    funcp->isConst(שקר);
    funcp->declPrivate(אמת);
    scopeTopp->addBlocksp(funcp);
    החזר funcp;
}

AstVarScope* newArgument(AstCFunc* funcp, AstNodeDType* dtypep, ثابت string& name,
                         VDirection direction) {
    FileLine* ثابت flp = funcp->fileline();
    AstScope* ثابت scopep = funcp->scopep();
    AstVar* ثابت varp = جديد AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(אמת);
    varp->direction(direction);
    funcp->addArgsp(varp);
    AstVarScope* ثابت vscp = جديد AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    החזר vscp;
}

AstNodeStmt* setVar(AstVarScope* vscp, uint32_t val) {
    FileLine* ثابت flp = vscp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* ثابت valp = جديد AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->num().setLong(val);
    החזר جديد AstAssign{flp, refp, valp};
}

AstNodeStmt* incrementVar(AstVarScope* vscp) {
    FileLine* ثابت flp = vscp->fileline();
    AstVarRef* ثابت wrefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* ثابت rrefp = جديد AstVarRef{flp, vscp, VAccess::READ};
    AstConst* ثابت onep = جديد AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->num().setLong(1);
    החזר جديد AstAssign{flp, wrefp, جديد AstAdd{flp, rrefp, onep}};
}

AstNodeStmt* callVoidFunc(AstCFunc* funcp) {
    אם (!funcp) החזר nullptr;
    AstCCall* ثابت callp = جديد AstCCall{funcp->fileline(), funcp};
    callp->dtypeSetVoid();
    החזר callp->makeStmt();
}

ساكن AstCFunc* splitCheckCreateNewSubFunc(AstCFunc* ofuncp) {
    ساكن std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    ثابت uint32_t funcNum = s_funcNums[ofuncp]++;
    ثابت std::string name = ofuncp->name() + "__" + cvtToStr(funcNum);
    AstScope* ثابت scopep = ofuncp->scopep();
    AstCFunc* ثابت subFuncp = جديد AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->addBlocksp(subFuncp);
    subFuncp->dontCombine(אמת);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(אמת);
    subFuncp->slow(ofuncp->slow());
    subFuncp->declPrivate(ofuncp->declPrivate());
    אם (ofuncp->needProcess()) subFuncp->setNeedProcess();
    עבור (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* ثابت clonep = argp->cloneTree(שקר);
        subFuncp->addArgsp(clonep);
        AstVarScope* ثابت vscp = جديد AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->addVarsp(vscp);
        argp->user3p(vscp);
    }
    החזר subFuncp;
};

فراغ splitCheckFinishSubFunc(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             ثابت std::unordered_map<ثابت AstVar*, AstVarScope*>& argVscps) {
    FileLine* ثابت flp = subFuncp->fileline();
    AstCCall* ثابت callp = جديد AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtypeSetVoid();
    // Pass arguments through to subfunction
    עבור (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->addArgsp(جديد AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    منطقي containsAwait = שקר;
    subFuncp->foreach([&](AstNode* nodep) {
        // Record if it has a CAwait
        אם (VN_IS(nodep, CAwait)) containsAwait = אמת;
        // Redirect references to arguments to the clone in the sub-function
        אם (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            אם (AstVarScope* ثابت vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    אם (ofuncp->isCoroutine() && containsAwait) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->addStmtsp(جديد AstCAwait{flp, callp});
    } אחרת {
        ofuncp->addStmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
ساكن std::vector<منطقي> splitCheckBreakable(ثابت AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::vector<ثابت AstNode*> stmtps;
    std::unordered_map<ثابت AstVar*, size_t> declIdx;  // Local var -> index it is declared at
    עבור (ثابت AstNode* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        אם (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.size());
        stmtps.push_back(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::vector<size_t> lastUse(stmtps.size());
    עבור (size_t i = 0; i < stmtps.size(); ++i) {
        lastUse[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](ثابت AstNodeVarRef* refp) {
            ثابت auto it = declIdx.find(refp->varp());  // 'end()' if not one of our locals
            אם (it != declIdx.end()) lastUse[it->second] = std::max(lastUse[it->second], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::vector<منطقي> breakable(stmtps.size(), אמת);
    size_t liveEnd = 0;  // Last index any so far declared local is referenced at
    עבור (size_t i = 0; i < stmtps.size(); ++i) {
        breakable[i] = liveEnd < i;
        אם (VN_IS(stmtps[i], Var)) liveEnd = std::max(liveEnd, lastUse[i]);
    }
    החזר breakable;
}

// Split large function according to --output-split-cfuncs
فراغ splitCheck(AstCFunc* ثابت ofuncp) {
    אם (!ofuncp) החזר;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    אם (!v3Global.opt.outputSplitCFuncs() || !ofuncp->stmtsp()) החזר;
    אם (ofuncp->nodeCount() < v3Global.opt.outputSplitCFuncs()) החזר;

    // Statement boundaries that would separate a local declaration from a reference to it
    ثابت std::vector<منطقي> breakable = splitCheckBreakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<ثابت AstVar*, AstVarScope*> argVscps;
    עבור (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.size() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        منطقي found = שקר;
        עבור (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            אם (vscp->varp() != argp) استمر;
            argVscps[argp] = vscp;
            found = אמת;
            اكسر;
        }
        UASSERT_OBJ(found, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    ثابت VNUser3InUse user3InUse;

    size_t size = 0;
    AstCFunc* subFuncp = nullptr;

    // Move statements one by one to the new sub-functions
    AstNode* stmtsp = ofuncp->stmtsp()->unlinkFrBackWithNext();
    עבור (size_t index = 0; AstNode* ثابت itemp = stmtsp; ++index) {
        stmtsp = stmtsp->nextp();
        אם (stmtsp) stmtsp->unlinkFrBackWithNext();
        ثابت size_t itemSize = static_cast<size_t>(itemp->nodeCount());
        size += itemSize;

        אם (size > static_cast<size_t>(v3Global.opt.outputSplitCFuncs()) && breakable[index]) {
            אם (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
            subFuncp = nullptr;
            size = itemSize;
        }

        אם (!subFuncp) subFuncp = splitCheckCreateNewSubFunc(ofuncp);
        subFuncp->addStmtsp(itemp);
    }
    אם (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
