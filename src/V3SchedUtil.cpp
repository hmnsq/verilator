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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3Order.h"
#अन्तर्भाव "V3Sched.h"
#अन्तर्भाव "V3SenExprBuilder.h"
#अन्तर्भाव "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश V3Sched {
नामाकाश util {

AstCFunc* makeSubFunction(AstNetlist* netlistp, स्थिरांक string& name, तार्किक slow) {
    AstScope* स्थिरांक scopeTopp = netlistp->शीर्ष_Scopep()->scopep();
    AstCFunc* स्थिरांक funcp = नवीन AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(सत्य);
    funcp->isStatic(असत्य);
    funcp->isLoose(सत्य);
    funcp->slow(slow);
    funcp->isConst(असत्य);
    funcp->declPrivate(सत्य);
    scopeTopp->addBlocksp(funcp);
    परत funcp;
}

AstVarScope* newArgument(AstCFunc* funcp, Ast_गाठ_DType* dtypep, स्थिरांक string& name,
                         VDirection direction) {
    File_ओळ* स्थिरांक flp = funcp->fileline();
    AstScope* स्थिरांक scopep = funcp->scopep();
    AstVar* स्थिरांक varp = नवीन AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(सत्य);
    varp->direction(direction);
    funcp->addArgsp(varp);
    AstVarScope* स्थिरांक vscp = नवीन AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    परत vscp;
}

Ast_गाठ_Stmt* setVar(AstVarScope* vscp, uint32_t val) {
    File_ओळ* स्थिरांक flp = vscp->fileline();
    AstVarRef* स्थिरांक refp = नवीन AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* स्थिरांक valp = नवीन AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->अंक().setLong(val);
    परत नवीन AstAssign{flp, refp, valp};
}

Ast_गाठ_Stmt* incrementVar(AstVarScope* vscp) {
    File_ओळ* स्थिरांक flp = vscp->fileline();
    AstVarRef* स्थिरांक wrefp = नवीन AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* स्थिरांक rrefp = नवीन AstVarRef{flp, vscp, VAccess::READ};
    AstConst* स्थिरांक onep = नवीन AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->अंक().setLong(1);
    परत नवीन AstAssign{flp, wrefp, नवीन AstAdd{flp, rrefp, onep}};
}

Ast_गाठ_Stmt* callVoidFunc(AstCFunc* funcp) {
    जर (!funcp) परत nullptr;
    AstCCall* स्थिरांक callp = नवीन AstCCall{funcp->fileline(), funcp};
    callp->dtypeSetVoid();
    परत callp->makeStmt();
}

स्थिर AstCFunc* splitCheckCreateNewSubFunc(AstCFunc* ofuncp) {
    स्थिर std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    स्थिरांक uint32_t func_अंक = s_funcNums[ofuncp]++;
    स्थिरांक std::string name = ofuncp->name() + "__" + cvtToStr(func_अंक);
    AstScope* स्थिरांक scopep = ofuncp->scopep();
    AstCFunc* स्थिरांक subFuncp = नवीन AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->addBlocksp(subFuncp);
    subFuncp->dontCombine(सत्य);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(सत्य);
    subFuncp->slow(ofuncp->slow());
    subFuncp->declPrivate(ofuncp->declPrivate());
    जर (ofuncp->needProcess()) subFuncp->setNeedProcess();
    साठी (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* स्थिरांक clonep = argp->clone_वृक्ष(असत्य);
        subFuncp->addArgsp(clonep);
        AstVarScope* स्थिरांक vscp = नवीन AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->addVarsp(vscp);
        argp->user3p(vscp);
    }
    परत subFuncp;
};

शून्य splitCheckFinishSubFunc(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             स्थिरांक std::unordered_map<स्थिरांक AstVar*, AstVarScope*>& argVscps) {
    File_ओळ* स्थिरांक flp = subFuncp->fileline();
    AstCCall* स्थिरांक callp = नवीन AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtypeSetVoid();
    // Pass arguments through to subfunction
    साठी (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->addArgsp(नवीन AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    तार्किक containsAwait = असत्य;
    subFuncp->foreach([&](Ast_गाठ* nodep) {
        // Record if it has a CAwait
        जर (VN_IS(nodep, CAwait)) containsAwait = सत्य;
        // Redirect references to arguments to the clone in the sub-function
        जर (AstVarRef* स्थिरांक refp = VN_CAST(nodep, VarRef)) {
            जर (AstVarScope* स्थिरांक vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    जर (ofuncp->isCoroutine() && containsAwait) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->addStmtsp(नवीन AstCAwait{flp, callp});
    } अन्यथा {
        ofuncp->addStmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
स्थिर std::vector<तार्किक> splitCheckBreakable(स्थिरांक AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::vector<स्थिरांक Ast_गाठ*> stmtps;
    std::unordered_map<स्थिरांक AstVar*, माप_प्रकार> declIdx;  // Local var -> index it is declared at
    साठी (स्थिरांक Ast_गाठ* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        जर (स्थिरांक AstVar* स्थिरांक varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.size());
        stmtps.ढकला_back(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::vector<माप_प्रकार> lastUse(stmtps.size());
    साठी (माप_प्रकार i = 0; i < stmtps.size(); ++i) {
        lastUse[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](स्थिरांक Ast_गाठ_Var_Ref* refp) {
            स्थिरांक स्वतः it = declIdx.find(refp->varp());  // 'end()' if not one of our locals
            जर (it != declIdx.end()) lastUse[it->second] = std::max(lastUse[it->second], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::vector<तार्किक> breakable(stmtps.size(), सत्य);
    माप_प्रकार liveEnd = 0;  // Last index any so far declared local is referenced at
    साठी (माप_प्रकार i = 0; i < stmtps.size(); ++i) {
        breakable[i] = liveEnd < i;
        जर (VN_IS(stmtps[i], Var)) liveEnd = std::max(liveEnd, lastUse[i]);
    }
    परत breakable;
}

// Split large function according to --output-split-cfuncs
शून्य splitCheck(AstCFunc* स्थिरांक ofuncp) {
    जर (!ofuncp) परत;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    जर (!v3Global.opt.outputSplitCFuncs() || !ofuncp->stmtsp()) परत;
    जर (ofuncp->गाठ_मोजणी() < v3Global.opt.outputSplitCFuncs()) परत;

    // Statement boundaries that would separate a local declaration from a reference to it
    स्थिरांक std::vector<तार्किक> breakable = splitCheckBreakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<स्थिरांक AstVar*, AstVarScope*> argVscps;
    साठी (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.size() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        तार्किक found = असत्य;
        साठी (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            जर (vscp->varp() != argp) सुरूठेवा;
            argVscps[argp] = vscp;
            found = सत्य;
            थांबा;
        }
        UASSERT_OBJ(found, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    स्थिरांक VNUser3InUse user3InUse;

    माप_प्रकार size = 0;
    AstCFunc* subFuncp = nullptr;

    // Move statements one by one to the new sub-functions
    Ast_गाठ* stmtsp = ofuncp->stmtsp()->unlinkFrBackWithNext();
    साठी (माप_प्रकार index = 0; Ast_गाठ* स्थिरांक itemp = stmtsp; ++index) {
        stmtsp = stmtsp->nextp();
        जर (stmtsp) stmtsp->unlinkFrBackWithNext();
        स्थिरांक माप_प्रकार itemSize = static_cast<माप_प्रकार>(itemp->गाठ_मोजणी());
        size += itemSize;

        जर (size > static_cast<माप_प्रकार>(v3Global.opt.outputSplitCFuncs()) && breakable[index]) {
            जर (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
            subFuncp = nullptr;
            size = itemSize;
        }

        जर (!subFuncp) subFuncp = splitCheckCreateNewSubFunc(ofuncp);
        subFuncp->addStmtsp(itemp);
    }
    जर (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
