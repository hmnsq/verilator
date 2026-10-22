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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3EmitCBase.h"
#અન્તર્ભાવ "V3EmitV.h"
#અન્તર્ભાવ "V3Order.h"
#અન્તર્ભાવ "V3Sched.h"
#અન્તર્ભાવ "V3SenExprBuilder.h"
#અન્તર્ભાવ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ V3Sched {
નામાકાશ util {

AstCFunc* makeSubFunction(AstNetlist* netlistp, અચલ string& name, તાર્કિક slow) {
    AstScope* અચલ scopeTopp = netlistp->ટોચ_Scopep()->scopep();
    AstCFunc* અચલ funcp = નવું AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(સાચું);
    funcp->isStatic(ખોટું);
    funcp->isLoose(સાચું);
    funcp->slow(slow);
    funcp->isConst(ખોટું);
    funcp->declPrivate(સાચું);
    scopeTopp->addBlocksp(funcp);
    પાછા funcp;
}

AstVarScope* newArgument(AstCFunc* funcp, Ast_ગાંઠ_DType* dtypep, અચલ string& name,
                         VDirection direction) {
    File_લીટી* અચલ flp = funcp->fileline();
    AstScope* અચલ scopep = funcp->scopep();
    AstVar* અચલ varp = નવું AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(સાચું);
    varp->direction(direction);
    funcp->addArgsp(varp);
    AstVarScope* અચલ vscp = નવું AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    પાછા vscp;
}

Ast_ગાંઠ_Stmt* setVar(AstVarScope* vscp, uint32_t val) {
    File_લીટી* અચલ flp = vscp->fileline();
    AstVarRef* અચલ refp = નવું AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* અચલ valp = નવું AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->અંક().setLong(val);
    પાછા નવું AstAssign{flp, refp, valp};
}

Ast_ગાંઠ_Stmt* incrementVar(AstVarScope* vscp) {
    File_લીટી* અચલ flp = vscp->fileline();
    AstVarRef* અચલ wrefp = નવું AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* અચલ rrefp = નવું AstVarRef{flp, vscp, VAccess::READ};
    AstConst* અચલ onep = નવું AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->અંક().setLong(1);
    પાછા નવું AstAssign{flp, wrefp, નવું AstAdd{flp, rrefp, onep}};
}

Ast_ગાંઠ_Stmt* callVoidFunc(AstCFunc* funcp) {
    જો (!funcp) પાછા nullptr;
    AstCCall* અચલ callp = નવું AstCCall{funcp->fileline(), funcp};
    callp->dtypeSetVoid();
    પાછા callp->makeStmt();
}

સ્થિર AstCFunc* splitCheckCreateNewSubFunc(AstCFunc* ofuncp) {
    સ્થિર std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    અચલ uint32_t func_અંક = s_funcNums[ofuncp]++;
    અચલ std::string name = ofuncp->name() + "__" + cvtToStr(func_અંક);
    AstScope* અચલ scopep = ofuncp->scopep();
    AstCFunc* અચલ subFuncp = નવું AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->addBlocksp(subFuncp);
    subFuncp->dontCombine(સાચું);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(સાચું);
    subFuncp->slow(ofuncp->slow());
    subFuncp->declPrivate(ofuncp->declPrivate());
    જો (ofuncp->needProcess()) subFuncp->setNeedProcess();
    માટે (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* અચલ clonep = argp->clone_વૃક્ષ(ખોટું);
        subFuncp->addArgsp(clonep);
        AstVarScope* અચલ vscp = નવું AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->addVarsp(vscp);
        argp->user3p(vscp);
    }
    પાછા subFuncp;
};

શૂન્ય splitCheckFinishSubFunc(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             અચલ std::unordered_map<અચલ AstVar*, AstVarScope*>& argVscps) {
    File_લીટી* અચલ flp = subFuncp->fileline();
    AstCCall* અચલ callp = નવું AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtypeSetVoid();
    // Pass arguments through to subfunction
    માટે (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->addArgsp(નવું AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    તાર્કિક containsAwait = ખોટું;
    subFuncp->foreach([&](Ast_ગાંઠ* nodep) {
        // Record if it has a CAwait
        જો (VN_IS(nodep, CAwait)) containsAwait = સાચું;
        // Redirect references to arguments to the clone in the sub-function
        જો (AstVarRef* અચલ refp = VN_CAST(nodep, VarRef)) {
            જો (AstVarScope* અચલ vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    જો (ofuncp->isCoroutine() && containsAwait) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->addStmtsp(નવું AstCAwait{flp, callp});
    } નહીંતર {
        ofuncp->addStmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
સ્થિર std::vector<તાર્કિક> splitCheckBreakable(અચલ AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::vector<અચલ Ast_ગાંઠ*> stmtps;
    std::unordered_map<અચલ AstVar*, size_t> declIdx;  // Local var -> index it is declared at
    માટે (અચલ Ast_ગાંઠ* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        જો (અચલ AstVar* અચલ varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.size());
        stmtps.ધકેલો_back(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::vector<size_t> lastUse(stmtps.size());
    માટે (size_t i = 0; i < stmtps.size(); ++i) {
        lastUse[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](અચલ Ast_ગાંઠ_Var_Ref* refp) {
            અચલ auto it = declIdx.find(refp->varp());  // 'end()' if not one of our locals
            જો (it != declIdx.end()) lastUse[it->second] = std::max(lastUse[it->second], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::vector<તાર્કિક> breakable(stmtps.size(), સાચું);
    size_t liveEnd = 0;  // Last index any so far declared local is referenced at
    માટે (size_t i = 0; i < stmtps.size(); ++i) {
        breakable[i] = liveEnd < i;
        જો (VN_IS(stmtps[i], Var)) liveEnd = std::max(liveEnd, lastUse[i]);
    }
    પાછા breakable;
}

// Split large function according to --output-split-cfuncs
શૂન્ય splitCheck(AstCFunc* અચલ ofuncp) {
    જો (!ofuncp) પાછા;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    જો (!v3Global.opt.outputSplitCFuncs() || !ofuncp->stmtsp()) પાછા;
    જો (ofuncp->ગાંઠ_ગણતરી() < v3Global.opt.outputSplitCFuncs()) પાછા;

    // Statement boundaries that would separate a local declaration from a reference to it
    અચલ std::vector<તાર્કિક> breakable = splitCheckBreakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<અચલ AstVar*, AstVarScope*> argVscps;
    માટે (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.size() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        તાર્કિક found = ખોટું;
        માટે (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            જો (vscp->varp() != argp) ચાલુ;
            argVscps[argp] = vscp;
            found = સાચું;
            તોડો;
        }
        UASSERT_OBJ(found, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    અચલ VNUser3InUse user3InUse;

    size_t size = 0;
    AstCFunc* subFuncp = nullptr;

    // Move statements one by one to the new sub-functions
    Ast_ગાંઠ* stmtsp = ofuncp->stmtsp()->unlinkFrBackWithNext();
    માટે (size_t index = 0; Ast_ગાંઠ* અચલ itemp = stmtsp; ++index) {
        stmtsp = stmtsp->nextp();
        જો (stmtsp) stmtsp->unlinkFrBackWithNext();
        અચલ size_t itemSize = static_cast<size_t>(itemp->ગાંઠ_ગણતરી());
        size += itemSize;

        જો (size > static_cast<size_t>(v3Global.opt.outputSplitCFuncs()) && breakable[index]) {
            જો (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
            subFuncp = nullptr;
            size = itemSize;
        }

        જો (!subFuncp) subFuncp = splitCheckCreateNewSubFunc(ofuncp);
        subFuncp->addStmtsp(itemp);
    }
    જો (subFuncp) splitCheckFinishSubFunc(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
