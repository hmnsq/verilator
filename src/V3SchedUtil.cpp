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

AstCFunc* makeSubFunction(AstNetlist* netlistp, अचर string& name, तार्किक slow) {
    AstScope* अचर scopeTopp = netlistp->शीर्ष_Scopep()->scopep();
    AstCFunc* अचर funcp = नूतन AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(सत्य);
    funcp->isStatic(असत्य);
    funcp->isLoose(सत्य);
    funcp->slow(slow);
    funcp->isConst(असत्य);
    funcp->declPrivate(सत्य);
    scopeTopp->addBlocksp(funcp);
    फर्काऊ funcp;
}

AstVarScope* newArgument(AstCFunc* funcp, Ast_गाँठो_DType* dtypep, अचर string& name,
                         VDirection direction) {
    File_पङ्क्ति* अचर flp = funcp->fileline();
    AstScope* अचर scopep = funcp->scopep();
    AstVar* अचर varp = नूतन AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(सत्य);
    varp->direction(direction);
    funcp->addArgsp(varp);
    AstVarScope* अचर vscp = नूतन AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    फर्काऊ vscp;
}

Ast_गाँठो_Stmt* setVar(AstVarScope* vscp, uint32_t val) {
    File_पङ्क्ति* अचर flp = vscp->fileline();
    AstVarRef* अचर refp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* अचर valp = नूतन AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->अङ्क().setLong(val);
    फर्काऊ नूतन AstAssign{flp, refp, valp};
}

Ast_गाँठो_Stmt* incrementVar(AstVarScope* vscp) {
    File_पङ्क्ति* अचर flp = vscp->fileline();
    AstVarRef* अचर wrefp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* अचर rrefp = नूतन AstVarRef{flp, vscp, VAccess::READ};
    AstConst* अचर onep = नूतन AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->अङ्क().setLong(1);
    फर्काऊ नूतन AstAssign{flp, wrefp, नूतन AstAdd{flp, rrefp, onep}};
}

Ast_गाँठो_Stmt* callVoidFunc(AstCFunc* funcp) {
    यदि (!funcp) फर्काऊ nullptr;
    AstCCall* अचर callp = नूतन AstCCall{funcp->fileline(), funcp};
    callp->dtypeSetVoid();
    फर्काऊ callp->makeStmt();
}

स्थिर AstCFunc* splitCheckCreateNewSubFunc(AstCFunc* ofuncp) {
    स्थिर std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    अचर uint32_t func_अङ्क = s_funcNums[ofuncp]++;
    अचर std::string name = ofuncp->name() + "__" + cvtToStr(func_अङ्क);
    AstScope* अचर scopep = ofuncp->scopep();
    AstCFunc* अचर subFuncp = नूतन AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->addBlocksp(subFuncp);
    subFuncp->dontCombine(सत्य);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(सत्य);
    subFuncp->slow(ofuncp->slow());
    subFuncp->declPrivate(ofuncp->declPrivate());
    यदि (ofuncp->needProcess()) subFuncp->setNeedProcess();
    लागि (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* अचर clonep = argp->clone_रूख(असत्य);
        subFuncp->addArgsp(clonep);
        AstVarScope* अचर vscp = नूतन AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->addVarsp(vscp);
        argp->user3p(vscp);
    }
    फर्काऊ subFuncp;
};

शून्य splitCheckFinishSubFunc(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             अचर std::unordered_map<अचर AstVar*, AstVarScope*>& argVscps) {
    File_पङ्क्ति* अचर flp = subFuncp->fileline();
    AstCCall* अचर callp = नूतन AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtypeSetVoid();
    // Pass arguments through to subfunction
    लागि (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->addArgsp(नूतन AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    तार्किक containsAwait = असत्य;
    subFuncp->foreach([&](Ast_गाँठो* nodep) {
        // Record if it has a CAwait
        यदि (VN_IS(nodep, CAwait)) containsAwait = सत्य;
        // Redirect references to arguments to the clone in the sub-function
        यदि (AstVarRef* अचर refp = VN_CAST(nodep, VarRef)) {
            यदि (AstVarScope* अचर vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    यदि (ofuncp->isCoroutine() && containsAwait) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->addStmtsp(नूतन AstCAwait{flp, callp});
    } अन्यथा {
        ofuncp->addStmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
स्थिर std::vector<तार्किक> splitCheckBreakable(अचर AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::vector<अचर Ast_गाँठो*> stmtps;
    std::unordered_map<अचर AstVar*, माप_प्रकार> declIdx;  // Local var -> index it is declared at
    लागि (अचर Ast_गाँठो* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        यदि (अचर AstVar* अचर varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.size());
        stmtps.थप_back(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::vector<माप_प्रकार> lastUse(stmtps.size());
    लागि (माप_प्रकार i = 0; i < stmtps.size(); ++i) {
        lastUse[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](अचर Ast_गाँठो_Var_Ref* refp) {
            अचर स्वतः it = declIdx.find(refp->varp());  // 'end()' if not one of our locals
            यदि (it != declIdx.end()) lastUse[it->second] = std::max(lastUse[it->second], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::vector<तार्किक> breakable(stmtps.size(), सत्य);
    माप_प्रकार liveEnd = 0;  // Last index any so far declared local is referenced at
    लागि (माप_प्रकार i = 0; i < stmtps.size(); ++i) {
        breakable[i] = liveEnd < i;
        यदि (VN_IS(stmtps[i], Var)) liveEnd = std::max(liveEnd, lastUse[i]);
    }
    फर्काऊ breakable;
}

// Split large function according to --output-split-cfuncs
शून्य splitCheck(AstCFunc* अचर ofuncp) {
    यदि (!ofuncp) फर्काऊ;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    यदि (!v3Global.opt.outputSplitCFuncs() || !ofuncp->stmtsp()) फर्काऊ;
    यदि (ofuncp->गाँठो_गन्ती() < v3Global.opt.outputSplitCFuncs()) फर्काऊ;

    // Statement boundaries that would separate a local declaration from a reference to it
    अचर std::vector<तार्किक> breakable = splitCheckBreakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<अचर AstVar*, AstVarScope*> argVscps;
    लागि (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.size() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        तार्किक found = असत्य;
        लागि (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            यदि (vscp->varp() != argp) जारी;
            argVscps[argp] = vscp;
            found = सत्य;
            रोक;
        }
        UASSERT_OBJ(found, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    अचर VNUser3InUse user3InUse;

    माप_प्रकार size = 0;
    AstCFunc* subFuncp = nullptr;

    // Move statements one by one to the new sub-functions
    Ast_गाँठो* stmtsp = ofuncp->stmtsp()->unlinkFrBackWithNext();
    लागि (माप_प्रकार index = 0; Ast_गाँठो* अचर itemp = stmtsp; ++index) {
        stmtsp = stmtsp->nextp();
        यदि (stmtsp) stmtsp->unlinkFrBackWithNext();
        अचर माप_प्रकार itemSize = static_cast<माप_प्रकार>(itemp->गाँठो_गन्ती());
        size += itemSize;

        यदि (size > static_cast<माप_प्रकार>(v3Global.opt.outputSplitCFuncs()) && breakable[index]) {
            यदि (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
            subFuncp = nullptr;
            size = itemSize;
        }

        यदि (!subFuncp) subFuncp = splitCheckCreateNewSubFunc(ofuncp);
        subFuncp->addStmtsp(itemp);
    }
    यदि (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
