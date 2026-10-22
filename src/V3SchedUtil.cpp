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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3EmitCBase.h"
#شامل "V3EmitV.h"
#شامل "V3Order.h"
#شامل "V3Sched.h"
#شامل "V3SenExprBuilder.h"
#شامل "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

فضاینام V3Sched {
فضاینام util {

AstCFunc* makeSubFunction(AstNetlist* netlistp, ثابت string& name, منطقی slow) {
    AstScope* ثابت scopeTopp = netlistp->بالاترین_Scopep()->scopep();
    AstCFunc* ثابت funcp = جدید AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(درست);
    funcp->isStatic(نادرست);
    funcp->isLoose(درست);
    funcp->slow(slow);
    funcp->isConst(نادرست);
    funcp->declPrivate(درست);
    scopeTopp->addBlocksp(funcp);
    بازگشت funcp;
}

AstVarScope* newArgument(AstCFunc* funcp, Ast_گره_DType* dtypep, ثابت string& name,
                         VDirection direction) {
    File_سطر* ثابت flp = funcp->fileline();
    AstScope* ثابت scopep = funcp->scopep();
    AstVar* ثابت varp = جدید AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(درست);
    varp->direction(direction);
    funcp->addArgsp(varp);
    AstVarScope* ثابت vscp = جدید AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    بازگشت vscp;
}

Ast_گره_Stmt* setVar(AstVarScope* vscp, uint32_t val) {
    File_سطر* ثابت flp = vscp->fileline();
    AstVarRef* ثابت refp = جدید AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* ثابت valp = جدید AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->رقم().setLong(val);
    بازگشت جدید AstAssign{flp, refp, valp};
}

Ast_گره_Stmt* incrementVar(AstVarScope* vscp) {
    File_سطر* ثابت flp = vscp->fileline();
    AstVarRef* ثابت wrefp = جدید AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* ثابت rrefp = جدید AstVarRef{flp, vscp, VAccess::READ};
    AstConst* ثابت onep = جدید AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->رقم().setLong(1);
    بازگشت جدید AstAssign{flp, wrefp, جدید AstAdd{flp, rrefp, onep}};
}

Ast_گره_Stmt* callVoidFunc(AstCFunc* funcp) {
    اگر (!funcp) بازگشت nullptr;
    AstCCall* ثابت callp = جدید AstCCall{funcp->fileline(), funcp};
    callp->dtypeSetVoid();
    بازگشت callp->makeStmt();
}

ایستا AstCFunc* splitCheckCreateNewSubFunc(AstCFunc* ofuncp) {
    ایستا std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    ثابت uint32_t func_رقم = s_funcNums[ofuncp]++;
    ثابت std::string name = ofuncp->name() + "__" + cvtToStr(func_رقم);
    AstScope* ثابت scopep = ofuncp->scopep();
    AstCFunc* ثابت subFuncp = جدید AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->addBlocksp(subFuncp);
    subFuncp->dontCombine(درست);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(درست);
    subFuncp->slow(ofuncp->slow());
    subFuncp->declPrivate(ofuncp->declPrivate());
    اگر (ofuncp->needProcess()) subFuncp->setNeedProcess();
    برای (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* ثابت clonep = argp->clone_درخت(نادرست);
        subFuncp->addArgsp(clonep);
        AstVarScope* ثابت vscp = جدید AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->addVarsp(vscp);
        argp->user3p(vscp);
    }
    بازگشت subFuncp;
};

تهی splitCheckFinishSubFunc(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             ثابت std::unordered_map<ثابت AstVar*, AstVarScope*>& argVscps) {
    File_سطر* ثابت flp = subFuncp->fileline();
    AstCCall* ثابت callp = جدید AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtypeSetVoid();
    // Pass arguments through to subfunction
    برای (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->addArgsp(جدید AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    منطقی containsAwait = نادرست;
    subFuncp->foreach([&](Ast_گره* nodep) {
        // Record if it has a CAwait
        اگر (VN_IS(nodep, CAwait)) containsAwait = درست;
        // Redirect references to arguments to the clone in the sub-function
        اگر (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            اگر (AstVarScope* ثابت vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    اگر (ofuncp->isCoroutine() && containsAwait) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->addStmtsp(جدید AstCAwait{flp, callp});
    } وگرنه {
        ofuncp->addStmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
ایستا std::vector<منطقی> splitCheckBreakable(ثابت AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::vector<ثابت Ast_گره*> stmtps;
    std::unordered_map<ثابت AstVar*, size_t> declIdx;  // Local var -> index it is declared at
    برای (ثابت Ast_گره* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        اگر (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.size());
        stmtps.افزودن_back(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::vector<size_t> lastUse(stmtps.size());
    برای (size_t i = 0; i < stmtps.size(); ++i) {
        lastUse[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](ثابت Ast_گره_Var_Ref* refp) {
            ثابت auto it = declIdx.find(refp->varp());  // 'end()' if not one of our locals
            اگر (it != declIdx.end()) lastUse[it->second] = std::max(lastUse[it->second], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::vector<منطقی> breakable(stmtps.size(), درست);
    size_t liveEnd = 0;  // Last index any so far declared local is referenced at
    برای (size_t i = 0; i < stmtps.size(); ++i) {
        breakable[i] = liveEnd < i;
        اگر (VN_IS(stmtps[i], Var)) liveEnd = std::max(liveEnd, lastUse[i]);
    }
    بازگشت breakable;
}

// Split large function according to --output-split-cfuncs
تهی splitCheck(AstCFunc* ثابت ofuncp) {
    اگر (!ofuncp) بازگشت;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    اگر (!v3Global.opt.outputSplitCFuncs() || !ofuncp->stmtsp()) بازگشت;
    اگر (ofuncp->گره_شمار() < v3Global.opt.outputSplitCFuncs()) بازگشت;

    // Statement boundaries that would separate a local declaration from a reference to it
    ثابت std::vector<منطقی> breakable = splitCheckBreakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<ثابت AstVar*, AstVarScope*> argVscps;
    برای (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.size() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        منطقی found = نادرست;
        برای (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            اگر (vscp->varp() != argp) ادامه;
            argVscps[argp] = vscp;
            found = درست;
            بشکن;
        }
        UASSERT_OBJ(found, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    ثابت VNUser3InUse user3InUse;

    size_t size = 0;
    AstCFunc* subFuncp = nullptr;

    // Move statements one by one to the new sub-functions
    Ast_گره* stmtsp = ofuncp->stmtsp()->unlinkFrBackWithNext();
    برای (size_t index = 0; Ast_گره* ثابت itemp = stmtsp; ++index) {
        stmtsp = stmtsp->nextp();
        اگر (stmtsp) stmtsp->unlinkFrBackWithNext();
        ثابت size_t itemSize = static_cast<size_t>(itemp->گره_شمار());
        size += itemSize;

        اگر (size > static_cast<size_t>(v3Global.opt.outputSplitCFuncs()) && breakable[index]) {
            اگر (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
            subFuncp = nullptr;
            size = itemSize;
        }

        اگر (!subFuncp) subFuncp = splitCheckCreateNewSubFunc(ofuncp);
        subFuncp->addStmtsp(itemp);
    }
    اگر (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
