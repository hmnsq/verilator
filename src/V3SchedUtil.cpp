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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3EmitCBase.h"
#অন্তর্ভাৱ "V3EmitV.h"
#অন্তর্ভাৱ "V3Order.h"
#অন্তর্ভাৱ "V3Sched.h"
#অন্তর্ভাৱ "V3SenExprBuilder.h"
#অন্তর্ভাৱ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ V3Sched {
নামাকাশ util {

AstCFunc* makeSubFunction(AstNetlist* netlistp, ধ্রুৱ string& name, তার্কিক slow) {
    AstScope* ধ্রুৱ scopeTopp = netlistp->শীৰ্ষ_Scopep()->scopep();
    AstCFunc* ধ্রুৱ funcp = নূতন AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(সঁচা);
    funcp->isStatic(মিছা);
    funcp->isLoose(সঁচা);
    funcp->slow(slow);
    funcp->isConst(মিছা);
    funcp->declPrivate(সঁচা);
    scopeTopp->addBlocksp(funcp);
    ঘূৰাই funcp;
}

AstVarScope* newArgument(AstCFunc* funcp, Ast_গ্ৰন্থি_DType* dtypep, ধ্রুৱ string& name,
                         VDirection direction) {
    File_সাৰি* ধ্রুৱ flp = funcp->fileline();
    AstScope* ধ্রুৱ scopep = funcp->scopep();
    AstVar* ধ্রুৱ varp = নূতন AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(সঁচা);
    varp->direction(direction);
    funcp->addArgsp(varp);
    AstVarScope* ধ্রুৱ vscp = নূতন AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    ঘূৰাই vscp;
}

Ast_গ্ৰন্থি_Stmt* setVar(AstVarScope* vscp, uint32_t val) {
    File_সাৰি* ধ্রুৱ flp = vscp->fileline();
    AstVarRef* ধ্রুৱ refp = নূতন AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* ধ্রুৱ valp = নূতন AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->অঙ্ক().setLong(val);
    ঘূৰাই নূতন AstAssign{flp, refp, valp};
}

Ast_গ্ৰন্থি_Stmt* incrementVar(AstVarScope* vscp) {
    File_সাৰি* ধ্রুৱ flp = vscp->fileline();
    AstVarRef* ধ্রুৱ wrefp = নূতন AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* ধ্রুৱ rrefp = নূতন AstVarRef{flp, vscp, VAccess::READ};
    AstConst* ধ্রুৱ onep = নূতন AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->অঙ্ক().setLong(1);
    ঘূৰাই নূতন AstAssign{flp, wrefp, নূতন AstAdd{flp, rrefp, onep}};
}

Ast_গ্ৰন্থি_Stmt* callVoidFunc(AstCFunc* funcp) {
    যদি (!funcp) ঘূৰাই nullptr;
    AstCCall* ধ্রুৱ callp = নূতন AstCCall{funcp->fileline(), funcp};
    callp->dtypeSetVoid();
    ঘূৰাই callp->makeStmt();
}

স্থির AstCFunc* splitCheckCreateNewSubFunc(AstCFunc* ofuncp) {
    স্থির std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    ধ্রুৱ uint32_t func_অঙ্ক = s_funcNums[ofuncp]++;
    ধ্রুৱ std::string name = ofuncp->name() + "__" + cvtToStr(func_অঙ্ক);
    AstScope* ধ্রুৱ scopep = ofuncp->scopep();
    AstCFunc* ধ্রুৱ subFuncp = নূতন AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->addBlocksp(subFuncp);
    subFuncp->dontCombine(সঁচা);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(সঁচা);
    subFuncp->slow(ofuncp->slow());
    subFuncp->declPrivate(ofuncp->declPrivate());
    যদি (ofuncp->needProcess()) subFuncp->setNeedProcess();
    বাবে (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* ধ্রুৱ clonep = argp->clone_বৃক্ষ(মিছা);
        subFuncp->addArgsp(clonep);
        AstVarScope* ধ্রুৱ vscp = নূতন AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->addVarsp(vscp);
        argp->user3p(vscp);
    }
    ঘূৰাই subFuncp;
};

শূন্য splitCheckFinishSubFunc(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             ধ্রুৱ std::unordered_map<ধ্রুৱ AstVar*, AstVarScope*>& argVscps) {
    File_সাৰি* ধ্রুৱ flp = subFuncp->fileline();
    AstCCall* ধ্রুৱ callp = নূতন AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtypeSetVoid();
    // Pass arguments through to subfunction
    বাবে (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->addArgsp(নূতন AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    তার্কিক containsAwait = মিছা;
    subFuncp->foreach([&](Ast_গ্ৰন্থি* nodep) {
        // Record if it has a CAwait
        যদি (VN_IS(nodep, CAwait)) containsAwait = সঁচা;
        // Redirect references to arguments to the clone in the sub-function
        যদি (AstVarRef* ধ্রুৱ refp = VN_CAST(nodep, VarRef)) {
            যদি (AstVarScope* ধ্রুৱ vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    যদি (ofuncp->isCoroutine() && containsAwait) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->addStmtsp(নূতন AstCAwait{flp, callp});
    } অন্যথা {
        ofuncp->addStmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
স্থির std::vector<তার্কিক> splitCheckBreakable(ধ্রুৱ AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::vector<ধ্রুৱ Ast_গ্ৰন্থি*> stmtps;
    std::unordered_map<ধ্রুৱ AstVar*, size_t> declIdx;  // Local var -> index it is declared at
    বাবে (ধ্রুৱ Ast_গ্ৰন্থি* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        যদি (ধ্রুৱ AstVar* ধ্রুৱ varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.size());
        stmtps.ঠেলো_back(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::vector<size_t> lastUse(stmtps.size());
    বাবে (size_t i = 0; i < stmtps.size(); ++i) {
        lastUse[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref* refp) {
            ধ্রুৱ auto it = declIdx.find(refp->varp());  // 'end()' if not one of our locals
            যদি (it != declIdx.end()) lastUse[it->second] = std::max(lastUse[it->second], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::vector<তার্কিক> breakable(stmtps.size(), সঁচা);
    size_t liveEnd = 0;  // Last index any so far declared local is referenced at
    বাবে (size_t i = 0; i < stmtps.size(); ++i) {
        breakable[i] = liveEnd < i;
        যদি (VN_IS(stmtps[i], Var)) liveEnd = std::max(liveEnd, lastUse[i]);
    }
    ঘূৰাই breakable;
}

// Split large function according to --output-split-cfuncs
শূন্য splitCheck(AstCFunc* ধ্রুৱ ofuncp) {
    যদি (!ofuncp) ঘূৰাই;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    যদি (!v3Global.opt.outputSplitCFuncs() || !ofuncp->stmtsp()) ঘূৰাই;
    যদি (ofuncp->গ্ৰন্থি_গুনতি() < v3Global.opt.outputSplitCFuncs()) ঘূৰাই;

    // Statement boundaries that would separate a local declaration from a reference to it
    ধ্রুৱ std::vector<তার্কিক> breakable = splitCheckBreakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<ধ্রুৱ AstVar*, AstVarScope*> argVscps;
    বাবে (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.size() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        তার্কিক found = মিছা;
        বাবে (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            যদি (vscp->varp() != argp) অনুৱর্ত;
            argVscps[argp] = vscp;
            found = সঁচা;
            ভঙ্গ;
        }
        UASSERT_OBJ(found, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    ধ্রুৱ VNUser3InUse user3InUse;

    size_t size = 0;
    AstCFunc* subFuncp = nullptr;

    // Move statements one by one to the new sub-functions
    Ast_গ্ৰন্থি* stmtsp = ofuncp->stmtsp()->unlinkFrBackWithNext();
    বাবে (size_t index = 0; Ast_গ্ৰন্থি* ধ্রুৱ itemp = stmtsp; ++index) {
        stmtsp = stmtsp->nextp();
        যদি (stmtsp) stmtsp->unlinkFrBackWithNext();
        ধ্রুৱ size_t itemSize = static_cast<size_t>(itemp->গ্ৰন্থি_গুনতি());
        size += itemSize;

        যদি (size > static_cast<size_t>(v3Global.opt.outputSplitCFuncs()) && breakable[index]) {
            যদি (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
            subFuncp = nullptr;
            size = itemSize;
        }

        যদি (!subFuncp) subFuncp = splitCheckCreateNewSubFunc(ofuncp);
        subFuncp->addStmtsp(itemp);
    }
    যদি (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
