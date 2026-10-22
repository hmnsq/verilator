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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3EmitCBase.h"
#অন্তর্ভাব "V3EmitV.h"
#অন্তর্ভাব "V3Order.h"
#অন্তর্ভাব "V3Sched.h"
#অন্তর্ভাব "V3SenExprBuilder.h"
#অন্তর্ভাব "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ V3Sched {
নামাকাশ util {

AstCFunc* makeSubFunction(AstNetlist* netlistp, ধ্রুবক string& name, যৌক্তিক slow) {
    AstScope* ধ্রুবক scopeTopp = netlistp->শীর্ষ_Scopep()->scopep();
    AstCFunc* ধ্রুবক funcp = নতুন AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(সত্য);
    funcp->isStatic(মিথ্যা);
    funcp->isLoose(সত্য);
    funcp->slow(slow);
    funcp->isConst(মিথ্যা);
    funcp->declPrivate(সত্য);
    scopeTopp->addBlocksp(funcp);
    ফেরত funcp;
}

AstVarScope* newArgument(AstCFunc* funcp, Ast_গ্রন্থি_DType* dtypep, ধ্রুবক string& name,
                         VDirection direction) {
    File_সারি* ধ্রুবক flp = funcp->fileline();
    AstScope* ধ্রুবক scopep = funcp->scopep();
    AstVar* ধ্রুবক varp = নতুন AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(সত্য);
    varp->direction(direction);
    funcp->addArgsp(varp);
    AstVarScope* ধ্রুবক vscp = নতুন AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    ফেরত vscp;
}

Ast_গ্রন্থি_Stmt* setVar(AstVarScope* vscp, uint32_t val) {
    File_সারি* ধ্রুবক flp = vscp->fileline();
    AstVarRef* ধ্রুবক refp = নতুন AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* ধ্রুবক valp = নতুন AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->অঙ্ক().setLong(val);
    ফেরত নতুন AstAssign{flp, refp, valp};
}

Ast_গ্রন্থি_Stmt* incrementVar(AstVarScope* vscp) {
    File_সারি* ধ্রুবক flp = vscp->fileline();
    AstVarRef* ধ্রুবক wrefp = নতুন AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* ধ্রুবক rrefp = নতুন AstVarRef{flp, vscp, VAccess::READ};
    AstConst* ধ্রুবক onep = নতুন AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->অঙ্ক().setLong(1);
    ফেরত নতুন AstAssign{flp, wrefp, নতুন AstAdd{flp, rrefp, onep}};
}

Ast_গ্রন্থি_Stmt* callVoidFunc(AstCFunc* funcp) {
    যদি (!funcp) ফেরত nullptr;
    AstCCall* ধ্রুবক callp = নতুন AstCCall{funcp->fileline(), funcp};
    callp->dtypeSetVoid();
    ফেরত callp->makeStmt();
}

স্থির AstCFunc* splitCheckCreateNewSubFunc(AstCFunc* ofuncp) {
    স্থির std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    ধ্রুবক uint32_t func_অঙ্ক = s_funcNums[ofuncp]++;
    ধ্রুবক std::string name = ofuncp->name() + "__" + cvtToStr(func_অঙ্ক);
    AstScope* ধ্রুবক scopep = ofuncp->scopep();
    AstCFunc* ধ্রুবক subFuncp = নতুন AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->addBlocksp(subFuncp);
    subFuncp->dontCombine(সত্য);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(সত্য);
    subFuncp->slow(ofuncp->slow());
    subFuncp->declPrivate(ofuncp->declPrivate());
    যদি (ofuncp->needProcess()) subFuncp->setNeedProcess();
    জন্য (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* ধ্রুবক clonep = argp->clone_বৃক্ষ(মিথ্যা);
        subFuncp->addArgsp(clonep);
        AstVarScope* ধ্রুবক vscp = নতুন AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->addVarsp(vscp);
        argp->user3p(vscp);
    }
    ফেরত subFuncp;
};

শূন্য splitCheckFinishSubFunc(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             ধ্রুবক std::unordered_map<ধ্রুবক AstVar*, AstVarScope*>& argVscps) {
    File_সারি* ধ্রুবক flp = subFuncp->fileline();
    AstCCall* ধ্রুবক callp = নতুন AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtypeSetVoid();
    // Pass arguments through to subfunction
    জন্য (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->addArgsp(নতুন AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    যৌক্তিক containsAwait = মিথ্যা;
    subFuncp->foreach([&](Ast_গ্রন্থি* nodep) {
        // Record if it has a CAwait
        যদি (VN_IS(nodep, CAwait)) containsAwait = সত্য;
        // Redirect references to arguments to the clone in the sub-function
        যদি (AstVarRef* ধ্রুবক refp = VN_CAST(nodep, VarRef)) {
            যদি (AstVarScope* ধ্রুবক vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    যদি (ofuncp->isCoroutine() && containsAwait) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->addStmtsp(নতুন AstCAwait{flp, callp});
    } অন্যথা {
        ofuncp->addStmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
স্থির std::vector<যৌক্তিক> splitCheckBreakable(ধ্রুবক AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::vector<ধ্রুবক Ast_গ্রন্থি*> stmtps;
    std::unordered_map<ধ্রুবক AstVar*, আকার_t> declIdx;  // Local var -> index it is declared at
    জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        যদি (ধ্রুবক AstVar* ধ্রুবক varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.আকার_2());
        stmtps.ঠেলো_back(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::vector<আকার_t> lastUse(stmtps.আকার_2());
    জন্য (আকার_t i = 0; i < stmtps.আকার_2(); ++i) {
        lastUse[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](ধ্রুবক Ast_গ্রন্থি_Var_Ref* refp) {
            ধ্রুবক auto it = declIdx.find(refp->varp());  // 'end()' if not one of our locals
            যদি (it != declIdx.end()) lastUse[it->second] = std::max(lastUse[it->second], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::vector<যৌক্তিক> breakable(stmtps.আকার_2(), সত্য);
    আকার_t liveEnd = 0;  // Last index any so far declared local is referenced at
    জন্য (আকার_t i = 0; i < stmtps.আকার_2(); ++i) {
        breakable[i] = liveEnd < i;
        যদি (VN_IS(stmtps[i], Var)) liveEnd = std::max(liveEnd, lastUse[i]);
    }
    ফেরত breakable;
}

// Split large function according to --output-split-cfuncs
শূন্য splitCheck(AstCFunc* ধ্রুবক ofuncp) {
    যদি (!ofuncp) ফেরত;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    যদি (!v3Global.opt.outputSplitCFuncs() || !ofuncp->stmtsp()) ফেরত;
    যদি (ofuncp->গ্রন্থি_গুনতি() < v3Global.opt.outputSplitCFuncs()) ফেরত;

    // Statement boundaries that would separate a local declaration from a reference to it
    ধ্রুবক std::vector<যৌক্তিক> breakable = splitCheckBreakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<ধ্রুবক AstVar*, AstVarScope*> argVscps;
    জন্য (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.আকার_2() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        যৌক্তিক found = মিথ্যা;
        জন্য (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            যদি (vscp->varp() != argp) চালু;
            argVscps[argp] = vscp;
            found = সত্য;
            বিরতি;
        }
        UASSERT_OBJ(found, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    ধ্রুবক VNUser3InUse user3InUse;

    আকার_t আকার_2 = 0;
    AstCFunc* subFuncp = nullptr;

    // Move statements one by one to the new sub-functions
    Ast_গ্রন্থি* stmtsp = ofuncp->stmtsp()->unlink_Fr_Back_With_পরবর্তী();
    জন্য (আকার_t সূচক = 0; Ast_গ্রন্থি* ধ্রুবক itemp = stmtsp; ++সূচক) {
        stmtsp = stmtsp->nextp();
        যদি (stmtsp) stmtsp->unlink_Fr_Back_With_পরবর্তী();
        ধ্রুবক আকার_t item_আকার = static_cast<আকার_t>(itemp->গ্রন্থি_গুনতি());
        আকার_2 += item_আকার;

        যদি (আকার_2 > static_cast<আকার_t>(v3Global.opt.outputSplitCFuncs()) && breakable[সূচক]) {
            যদি (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
            subFuncp = nullptr;
            আকার_2 = item_আকার;
        }

        যদি (!subFuncp) subFuncp = splitCheckCreateNewSubFunc(ofuncp);
        subFuncp->addStmtsp(itemp);
    }
    যদি (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
