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
    AstScope* ثابت scopeTopp = netlistp->لووتکە_Scopep()->scopep();
    AstCFunc* ثابت funcp = جديد AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(true);
    funcp->isStatic(خطأ);
    funcp->isLoose(true);
    funcp->slow(slow);
    funcp->isConst(خطأ);
    funcp->declPrivate(true);
    scopeTopp->addBlocksp(funcp);
    أرجع funcp;
}

AstVarScope* newArgument(AstCFunc* funcp, Ast_گرێ_DType* dtypep, ثابت string& name,
                         VDirection direction) {
    File_هێڵ* ثابت flp = funcp->fileline();
    AstScope* ثابت scopep = funcp->scopep();
    AstVar* ثابت varp = جديد AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(true);
    varp->direction(direction);
    funcp->addArgsp(varp);
    AstVarScope* ثابت vscp = جديد AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    أرجع vscp;
}

Ast_گرێ_Stmt* setVar(AstVarScope* vscp, uint32_t val) {
    File_هێڵ* ثابت flp = vscp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* ثابت valp = جديد AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->ژمارە().setLong(val);
    أرجع جديد AstAssign{flp, refp, valp};
}

Ast_گرێ_Stmt* incrementVar(AstVarScope* vscp) {
    File_هێڵ* ثابت flp = vscp->fileline();
    AstVarRef* ثابت wrefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* ثابت rrefp = جديد AstVarRef{flp, vscp, VAccess::READ};
    AstConst* ثابت onep = جديد AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->ژمارە().setLong(1);
    أرجع جديد AstAssign{flp, wrefp, جديد AstAdd{flp, rrefp, onep}};
}

Ast_گرێ_Stmt* callVoidFunc(AstCFunc* funcp) {
    إذا (!funcp) أرجع nullptr;
    AstCCall* ثابت callp = جديد AstCCall{funcp->fileline(), funcp};
    callp->dtypeSetVoid();
    أرجع callp->makeStmt();
}

ساكن AstCFunc* splitCheckCreateNewSubFunc(AstCFunc* ofuncp) {
    ساكن std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    ثابت uint32_t func_ژمارە = s_funcNums[ofuncp]++;
    ثابت std::string name = ofuncp->name() + "__" + cvtToStr(func_ژمارە);
    AstScope* ثابت scopep = ofuncp->scopep();
    AstCFunc* ثابت subFuncp = جديد AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->addBlocksp(subFuncp);
    subFuncp->dontCombine(true);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(true);
    subFuncp->slow(ofuncp->slow());
    subFuncp->declPrivate(ofuncp->declPrivate());
    إذا (ofuncp->needProcess()) subFuncp->setNeedProcess();
    لكل (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* ثابت clonep = argp->clone_درەخت(خطأ);
        subFuncp->addArgsp(clonep);
        AstVarScope* ثابت vscp = جديد AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->addVarsp(vscp);
        argp->user3p(vscp);
    }
    أرجع subFuncp;
};

فراغ splitCheckFinishSubFunc(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             ثابت std::unordered_map<ثابت AstVar*, AstVarScope*>& argVscps) {
    File_هێڵ* ثابت flp = subFuncp->fileline();
    AstCCall* ثابت callp = جديد AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtypeSetVoid();
    // Pass arguments through to subfunction
    لكل (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->addArgsp(جديد AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    منطقي containsAwait = خطأ;
    subFuncp->foreach([&](Ast_گرێ* nodep) {
        // Record if it has a CAwait
        إذا (VN_IS(nodep, CAwait)) containsAwait = true;
        // Redirect references to arguments to the clone in the sub-function
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            إذا (AstVarScope* ثابت vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    إذا (ofuncp->isCoroutine() && containsAwait) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->addStmtsp(جديد AstCAwait{flp, callp});
    } وإلا {
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
    std::vector<ثابت Ast_گرێ*> stmtps;
    std::unordered_map<ثابت AstVar*, size_t> declIdx;  // Local var -> index it is declared at
    لكل (ثابت Ast_گرێ* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.size());
        stmtps.پاڵنان_back(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::vector<size_t> lastUse(stmtps.size());
    لكل (size_t i = 0; i < stmtps.size(); ++i) {
        lastUse[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](ثابت Ast_گرێ_Var_Ref* refp) {
            ثابت auto it = declIdx.find(refp->varp());  // 'end()' if not one of our locals
            إذا (it != declIdx.end()) lastUse[it->second] = std::max(lastUse[it->second], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::vector<منطقي> breakable(stmtps.size(), true);
    size_t liveEnd = 0;  // Last index any so far declared local is referenced at
    لكل (size_t i = 0; i < stmtps.size(); ++i) {
        breakable[i] = liveEnd < i;
        إذا (VN_IS(stmtps[i], Var)) liveEnd = std::max(liveEnd, lastUse[i]);
    }
    أرجع breakable;
}

// Split large function according to --output-split-cfuncs
فراغ splitCheck(AstCFunc* ثابت ofuncp) {
    إذا (!ofuncp) أرجع;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    إذا (!v3Global.opt.outputSplitCFuncs() || !ofuncp->stmtsp()) أرجع;
    إذا (ofuncp->گرێ_ژماردن() < v3Global.opt.outputSplitCFuncs()) أرجع;

    // Statement boundaries that would separate a local declaration from a reference to it
    ثابت std::vector<منطقي> breakable = splitCheckBreakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<ثابت AstVar*, AstVarScope*> argVscps;
    لكل (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.size() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        منطقي found = خطأ;
        لكل (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            إذا (vscp->varp() != argp) استمر;
            argVscps[argp] = vscp;
            found = true;
            اكسر;
        }
        UASSERT_OBJ(found, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    ثابت VNUser3InUse user3InUse;

    size_t size = 0;
    AstCFunc* subFuncp = nullptr;

    // Move statements one by one to the new sub-functions
    Ast_گرێ* stmtsp = ofuncp->stmtsp()->unlinkFrBackWithNext();
    لكل (size_t index = 0; Ast_گرێ* ثابت itemp = stmtsp; ++index) {
        stmtsp = stmtsp->nextp();
        إذا (stmtsp) stmtsp->unlinkFrBackWithNext();
        ثابت size_t itemSize = static_cast<size_t>(itemp->گرێ_ژماردن());
        size += itemSize;

        إذا (size > static_cast<size_t>(v3Global.opt.outputSplitCFuncs()) && breakable[index]) {
            إذا (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
            subFuncp = nullptr;
            size = itemSize;
        }

        إذا (!subFuncp) subFuncp = splitCheckCreateNewSubFunc(ofuncp);
        subFuncp->addStmtsp(itemp);
    }
    إذا (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
