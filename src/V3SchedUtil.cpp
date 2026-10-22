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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3Order.h"
#ಅನ್ತರ್ಭಾವ "V3Sched.h"
#ಅನ್ತರ್ಭಾವ "V3SenExprBuilder.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ V3Sched {
ನಾಮಾಕಾಶ util {

AstCFunc* makeSubFunction(AstNetlist* netlistp, ಧ್ರುವ string& name, ತಾರ್ಕಿಕ slow) {
    AstScope* ಧ್ರುವ scopeTopp = netlistp->ತುದಿ_Scopep()->scopep();
    AstCFunc* ಧ್ರುವ funcp = ನೂತನ AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(ಸತ್ಯ);
    funcp->isStatic(ಅಸತ್ಯ);
    funcp->isLoose(ಸತ್ಯ);
    funcp->slow(slow);
    funcp->isConst(ಅಸತ್ಯ);
    funcp->declPrivate(ಸತ್ಯ);
    scopeTopp->addBlocksp(funcp);
    ಪ್ರತ್ಯಾಗಮ funcp;
}

AstVarScope* newArgument(AstCFunc* funcp, Ast_ಗಂಟು_DType* dtypep, ಧ್ರುವ string& name,
                         VDirection direction) {
    File_ಸಾಲು* ಧ್ರುವ flp = funcp->fileline();
    AstScope* ಧ್ರುವ scopep = funcp->scopep();
    AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(ಸತ್ಯ);
    varp->direction(direction);
    funcp->addArgsp(varp);
    AstVarScope* ಧ್ರುವ vscp = ನೂತನ AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    ಪ್ರತ್ಯಾಗಮ vscp;
}

Ast_ಗಂಟು_Stmt* setVar(AstVarScope* vscp, uint32_t val) {
    File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
    AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* ಧ್ರುವ valp = ನೂತನ AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->ಅಂಕೆ().setLong(val);
    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{flp, refp, valp};
}

Ast_ಗಂಟು_Stmt* incrementVar(AstVarScope* vscp) {
    File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
    AstVarRef* ಧ್ರುವ wrefp = ನೂತನ AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* ಧ್ರುವ rrefp = ನೂತನ AstVarRef{flp, vscp, VAccess::READ};
    AstConst* ಧ್ರುವ onep = ನೂತನ AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->ಅಂಕೆ().setLong(1);
    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{flp, wrefp, ನೂತನ AstAdd{flp, rrefp, onep}};
}

Ast_ಗಂಟು_Stmt* callVoidFunc(AstCFunc* funcp) {
    ಯದಿ (!funcp) ಪ್ರತ್ಯಾಗಮ nullptr;
    AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{funcp->fileline(), funcp};
    callp->dtypeSetVoid();
    ಪ್ರತ್ಯಾಗಮ callp->makeStmt();
}

ಸ್ಥಿರ AstCFunc* splitCheckCreateNewSubFunc(AstCFunc* ofuncp) {
    ಸ್ಥಿರ std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    ಧ್ರುವ uint32_t func_ಅಂಕೆ = s_funcNums[ofuncp]++;
    ಧ್ರುವ std::string name = ofuncp->name() + "__" + cvtToStr(func_ಅಂಕೆ);
    AstScope* ಧ್ರುವ scopep = ofuncp->scopep();
    AstCFunc* ಧ್ರುವ subFuncp = ನೂತನ AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->addBlocksp(subFuncp);
    subFuncp->dontCombine(ಸತ್ಯ);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(ಸತ್ಯ);
    subFuncp->slow(ofuncp->slow());
    subFuncp->declPrivate(ofuncp->declPrivate());
    ಯದಿ (ofuncp->needProcess()) subFuncp->setNeedProcess();
    ಪ್ರತಿ (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* ಧ್ರುವ clonep = argp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        subFuncp->addArgsp(clonep);
        AstVarScope* ಧ್ರುವ vscp = ನೂತನ AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->addVarsp(vscp);
        argp->user3p(vscp);
    }
    ಪ್ರತ್ಯಾಗಮ subFuncp;
};

ಶೂನ್ಯ splitCheckFinishSubFunc(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             ಧ್ರುವ std::unordered_map<ಧ್ರುವ AstVar*, AstVarScope*>& argVscps) {
    File_ಸಾಲು* ಧ್ರುವ flp = subFuncp->fileline();
    AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtypeSetVoid();
    // Pass arguments through to subfunction
    ಪ್ರತಿ (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->addArgsp(ನೂತನ AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    ತಾರ್ಕಿಕ containsAwait = ಅಸತ್ಯ;
    subFuncp->foreach([&](Ast_ಗಂಟು* nodep) {
        // Record if it has a CAwait
        ಯದಿ (VN_IS(nodep, CAwait)) containsAwait = ಸತ್ಯ;
        // Redirect references to arguments to the clone in the sub-function
        ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep, VarRef)) {
            ಯದಿ (AstVarScope* ಧ್ರುವ vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    ಯದಿ (ofuncp->isCoroutine() && containsAwait) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->addStmtsp(ನೂತನ AstCAwait{flp, callp});
    } ಅನ್ಯಥಾ {
        ofuncp->addStmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
ಸ್ಥಿರ std::vector<ತಾರ್ಕಿಕ> splitCheckBreakable(ಧ್ರುವ AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::vector<ಧ್ರುವ Ast_ಗಂಟು*> stmtps;
    std::unordered_map<ಧ್ರುವ AstVar*, size_t> declIdx;  // Local var -> index it is declared at
    ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.size());
        stmtps.ತಳ್ಳು_back(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::vector<size_t> lastUse(stmtps.size());
    ಪ್ರತಿ (size_t i = 0; i < stmtps.size(); ++i) {
        lastUse[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* refp) {
            ಧ್ರುವ auto it = declIdx.find(refp->varp());  // 'end()' if not one of our locals
            ಯದಿ (it != declIdx.end()) lastUse[it->second] = std::max(lastUse[it->second], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::vector<ತಾರ್ಕಿಕ> breakable(stmtps.size(), ಸತ್ಯ);
    size_t liveEnd = 0;  // Last index any so far declared local is referenced at
    ಪ್ರತಿ (size_t i = 0; i < stmtps.size(); ++i) {
        breakable[i] = liveEnd < i;
        ಯದಿ (VN_IS(stmtps[i], Var)) liveEnd = std::max(liveEnd, lastUse[i]);
    }
    ಪ್ರತ್ಯಾಗಮ breakable;
}

// Split large function according to --output-split-cfuncs
ಶೂನ್ಯ splitCheck(AstCFunc* ಧ್ರುವ ofuncp) {
    ಯದಿ (!ofuncp) ಪ್ರತ್ಯಾಗಮ;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    ಯದಿ (!v3Global.opt.outputSplitCFuncs() || !ofuncp->stmtsp()) ಪ್ರತ್ಯಾಗಮ;
    ಯದಿ (ofuncp->ಗಂಟು_ಎಣಿಕೆ() < v3Global.opt.outputSplitCFuncs()) ಪ್ರತ್ಯಾಗಮ;

    // Statement boundaries that would separate a local declaration from a reference to it
    ಧ್ರುವ std::vector<ತಾರ್ಕಿಕ> breakable = splitCheckBreakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<ಧ್ರುವ AstVar*, AstVarScope*> argVscps;
    ಪ್ರತಿ (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.size() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        ತಾರ್ಕಿಕ found = ಅಸತ್ಯ;
        ಪ್ರತಿ (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            ಯದಿ (vscp->varp() != argp) ಅನುವರ್ತ;
            argVscps[argp] = vscp;
            found = ಸತ್ಯ;
            ಭಙ್ಗ;
        }
        UASSERT_OBJ(found, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    ಧ್ರುವ VNUser3InUse user3InUse;

    size_t size = 0;
    AstCFunc* subFuncp = nullptr;

    // Move statements one by one to the new sub-functions
    Ast_ಗಂಟು* stmtsp = ofuncp->stmtsp()->unlinkFrBackWithNext();
    ಪ್ರತಿ (size_t index = 0; Ast_ಗಂಟು* ಧ್ರುವ itemp = stmtsp; ++index) {
        stmtsp = stmtsp->nextp();
        ಯದಿ (stmtsp) stmtsp->unlinkFrBackWithNext();
        ಧ್ರುವ size_t itemSize = static_cast<size_t>(itemp->ಗಂಟು_ಎಣಿಕೆ());
        size += itemSize;

        ಯದಿ (size > static_cast<size_t>(v3Global.opt.outputSplitCFuncs()) && breakable[index]) {
            ಯದಿ (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
            subFuncp = nullptr;
            size = itemSize;
        }

        ಯದಿ (!subFuncp) subFuncp = splitCheckCreateNewSubFunc(ofuncp);
        subFuncp->addStmtsp(itemp);
    }
    ಯದಿ (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
