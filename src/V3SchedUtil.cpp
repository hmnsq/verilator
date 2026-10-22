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

AstCFunc* makeSubFunction(AstNetlist* netlistp, ध्रुव string& name, तार्किक slow) {
    AstScope* ध्रुव scopeTopp = netlistp->शीर्ष_Scopep()->scopep();
    AstCFunc* ध्रुव funcp = नूतन AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(सत्य);
    funcp->isStatic(असत्य);
    funcp->isLoose(सत्य);
    funcp->slow(slow);
    funcp->isConst(असत्य);
    funcp->declPrivate(सत्य);
    scopeTopp->addBlocksp(funcp);
    प्रत्यागम funcp;
}

AstVarScope* newArgument(AstCFunc* funcp, Ast_गाठ_DType* dtypep, ध्रुव string& name,
                         VDirection direction) {
    File_ओळ* ध्रुव flp = funcp->fileline();
    AstScope* ध्रुव scopep = funcp->scopep();
    AstVar* ध्रुव varp = नूतन AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(सत्य);
    varp->direction(direction);
    funcp->addArgsp(varp);
    AstVarScope* ध्रुव vscp = नूतन AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    प्रत्यागम vscp;
}

Ast_गाठ_Stmt* setVar(AstVarScope* vscp, uint32_t val) {
    File_ओळ* ध्रुव flp = vscp->fileline();
    AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* ध्रुव valp = नूतन AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->अंक().setLong(val);
    प्रत्यागम नूतन AstAssign{flp, refp, valp};
}

Ast_गाठ_Stmt* incrementVar(AstVarScope* vscp) {
    File_ओळ* ध्रुव flp = vscp->fileline();
    AstVarRef* ध्रुव wrefp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* ध्रुव rrefp = नूतन AstVarRef{flp, vscp, VAccess::READ};
    AstConst* ध्रुव onep = नूतन AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->अंक().setLong(1);
    प्रत्यागम नूतन AstAssign{flp, wrefp, नूतन AstAdd{flp, rrefp, onep}};
}

Ast_गाठ_Stmt* callVoidFunc(AstCFunc* funcp) {
    यदि (!funcp) प्रत्यागम nullptr;
    AstCCall* ध्रुव callp = नूतन AstCCall{funcp->fileline(), funcp};
    callp->dtypeSetVoid();
    प्रत्यागम callp->makeStmt();
}

स्थिर AstCFunc* splitCheckCreateNewSubFunc(AstCFunc* ofuncp) {
    स्थिर std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    ध्रुव uint32_t func_अंक = s_funcNums[ofuncp]++;
    ध्रुव std::string name = ofuncp->name() + "__" + cvtToStr(func_अंक);
    AstScope* ध्रुव scopep = ofuncp->scopep();
    AstCFunc* ध्रुव subFuncp = नूतन AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->addBlocksp(subFuncp);
    subFuncp->dontCombine(सत्य);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(सत्य);
    subFuncp->slow(ofuncp->slow());
    subFuncp->declPrivate(ofuncp->declPrivate());
    यदि (ofuncp->needProcess()) subFuncp->setNeedProcess();
    प्रति (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* ध्रुव clonep = argp->clone_वृक्ष(असत्य);
        subFuncp->addArgsp(clonep);
        AstVarScope* ध्रुव vscp = नूतन AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->addVarsp(vscp);
        argp->user3p(vscp);
    }
    प्रत्यागम subFuncp;
};

शून्य splitCheckFinishSubFunc(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             ध्रुव std::unordered_map<ध्रुव AstVar*, AstVarScope*>& argVscps) {
    File_ओळ* ध्रुव flp = subFuncp->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtypeSetVoid();
    // Pass arguments through to subfunction
    प्रति (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->addArgsp(नूतन AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    तार्किक containsAwait = असत्य;
    subFuncp->foreach([&](Ast_गाठ* nodep) {
        // Record if it has a CAwait
        यदि (VN_IS(nodep, CAwait)) containsAwait = सत्य;
        // Redirect references to arguments to the clone in the sub-function
        यदि (AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) {
            यदि (AstVarScope* ध्रुव vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
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
स्थिर std::vector<तार्किक> splitCheckBreakable(ध्रुव AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::vector<ध्रुव Ast_गाठ*> stmtps;
    std::unordered_map<ध्रुव AstVar*, size_t> declIdx;  // Local var -> index it is declared at
    प्रति (ध्रुव Ast_गाठ* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.size());
        stmtps.ढकला_back(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::vector<size_t> lastUse(stmtps.size());
    प्रति (size_t i = 0; i < stmtps.size(); ++i) {
        lastUse[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](ध्रुव Ast_गाठ_Var_Ref* refp) {
            ध्रुव auto it = declIdx.find(refp->varp());  // 'end()' if not one of our locals
            यदि (it != declIdx.end()) lastUse[it->second] = std::max(lastUse[it->second], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::vector<तार्किक> breakable(stmtps.size(), सत्य);
    size_t liveEnd = 0;  // Last index any so far declared local is referenced at
    प्रति (size_t i = 0; i < stmtps.size(); ++i) {
        breakable[i] = liveEnd < i;
        यदि (VN_IS(stmtps[i], Var)) liveEnd = std::max(liveEnd, lastUse[i]);
    }
    प्रत्यागम breakable;
}

// Split large function according to --output-split-cfuncs
शून्य splitCheck(AstCFunc* ध्रुव ofuncp) {
    यदि (!ofuncp) प्रत्यागम;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    यदि (!v3Global.opt.outputSplitCFuncs() || !ofuncp->stmtsp()) प्रत्यागम;
    यदि (ofuncp->गाठ_मोजणी() < v3Global.opt.outputSplitCFuncs()) प्रत्यागम;

    // Statement boundaries that would separate a local declaration from a reference to it
    ध्रुव std::vector<तार्किक> breakable = splitCheckBreakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<ध्रुव AstVar*, AstVarScope*> argVscps;
    प्रति (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.size() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        तार्किक found = असत्य;
        प्रति (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            यदि (vscp->varp() != argp) अनुवर्त;
            argVscps[argp] = vscp;
            found = सत्य;
            भङ्ग;
        }
        UASSERT_OBJ(found, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    ध्रुव VNUser3InUse user3InUse;

    size_t size = 0;
    AstCFunc* subFuncp = nullptr;

    // Move statements one by one to the new sub-functions
    Ast_गाठ* stmtsp = ofuncp->stmtsp()->unlinkFrBackWithNext();
    प्रति (size_t index = 0; Ast_गाठ* ध्रुव itemp = stmtsp; ++index) {
        stmtsp = stmtsp->nextp();
        यदि (stmtsp) stmtsp->unlinkFrBackWithNext();
        ध्रुव size_t itemSize = static_cast<size_t>(itemp->गाठ_मोजणी());
        size += itemSize;

        यदि (size > static_cast<size_t>(v3Global.opt.outputSplitCFuncs()) && breakable[index]) {
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
