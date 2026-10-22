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
    AstScope* ثابت scopeTopp = netlistp->چوٽي_Scopep()->scopep();
    AstCFunc* ثابت funcp = جديد AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(سچ);
    funcp->isStatic(خطأ);
    funcp->isLoose(سچ);
    funcp->slow(slow);
    funcp->isConst(خطأ);
    funcp->decl_ذاتي(سچ);
    scopeTopp->جوڙيو_Blocksp(funcp);
    أرجع funcp;
}

AstVarScope* new_دليل(AstCFunc* funcp, Ast_ڳنڍ_DType* dtypep, ثابت string& name,
                         VDirection direction) {
    File_سٽ* ثابت flp = funcp->fileline();
    AstScope* ثابت scopep = funcp->scopep();
    AstVar* ثابت varp = جديد AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(سچ);
    varp->direction(direction);
    funcp->جوڙيو_Argsp(varp);
    AstVarScope* ثابت vscp = جديد AstVarScope{flp, scopep, varp};
    scopep->جوڙيو_Varsp(vscp);
    أرجع vscp;
}

Ast_ڳنڍ_Stmt* سيٽ_Var(AstVarScope* vscp, uint32_t val) {
    File_سٽ* ثابت flp = vscp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* ثابت valp = جديد AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->انگ().سيٽ_Long(val);
    أرجع جديد Ast_سونپيو{flp, refp, valp};
}

Ast_ڳنڍ_Stmt* incrementVar(AstVarScope* vscp) {
    File_سٽ* ثابت flp = vscp->fileline();
    AstVarRef* ثابت wrefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* ثابت rrefp = جديد AstVarRef{flp, vscp, VAccess::READ};
    AstConst* ثابت onep = جديد AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->انگ().سيٽ_Long(1);
    أرجع جديد Ast_سونپيو{flp, wrefp, جديد Ast_جوڙيو{flp, rrefp, onep}};
}

Ast_ڳنڍ_Stmt* callVoidFunc(AstCFunc* funcp) {
    إذا (!funcp) أرجع خالي_اشارو;
    AstCCall* ثابت callp = جديد AstCCall{funcp->fileline(), funcp};
    callp->dtype_سيٽ_Void();
    أرجع callp->makeStmt();
}

ساكن AstCFunc* ورهايو_Check_Create_New_Sub_Func(AstCFunc* ofuncp) {
    ساكن std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    ثابت uint32_t func_انگ = s_funcNums[ofuncp]++;
    ثابت std::string name = ofuncp->name() + "__" + cvt_منزل_Str(func_انگ);
    AstScope* ثابت scopep = ofuncp->scopep();
    AstCFunc* ثابت subFuncp = جديد AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->جوڙيو_Blocksp(subFuncp);
    subFuncp->dontCombine(سچ);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(سچ);
    subFuncp->slow(ofuncp->slow());
    subFuncp->decl_ذاتي(ofuncp->decl_ذاتي());
    إذا (ofuncp->needProcess()) subFuncp->سيٽ_Need_Process();
    لكل (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* ثابت clonep = argp->clone_وڻ(خطأ);
        subFuncp->جوڙيو_Argsp(clonep);
        AstVarScope* ثابت vscp = جديد AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->جوڙيو_Varsp(vscp);
        argp->user3p(vscp);
    }
    أرجع subFuncp;
};

فراغ ورهايو_Check_پڄاڻي_Sub_Func(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             ثابت std::unordered_map<ثابت AstVar*, AstVarScope*>& argVscps) {
    File_سٽ* ثابت flp = subFuncp->fileline();
    AstCCall* ثابت callp = جديد AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtype_سيٽ_Void();
    // Pass arguments through to subfunction
    لكل (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->جوڙيو_Argsp(جديد AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    منطقي شامل_آهي_Await = خطأ;
    subFuncp->foreach([&](Ast_ڳنڍ* nodep) {
        // Record if it has a CAwait
        إذا (VN_IS(nodep, CAwait)) شامل_آهي_Await = سچ;
        // Redirect references to arguments to the clone in the sub-function
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            إذا (AstVarScope* ثابت vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    إذا (ofuncp->isCoroutine() && شامل_آهي_Await) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->جوڙيو_Stmtsp(جديد AstCAwait{flp, callp});
    } وإلا {
        ofuncp->جوڙيو_Stmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
ساكن std::ويڪٽر<منطقي> ورهايو_Check_Breakable(ثابت AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::ويڪٽر<ثابت Ast_ڳنڍ*> stmtps;
    std::unordered_map<ثابت AstVar*, سائيز_t> declIdx;  // Local var -> index it is declared at
    لكل (ثابت Ast_ڳنڍ* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.سائيز());
        stmtps.ڌڪو_پٺ(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::ويڪٽر<سائيز_t> آخري_Use(stmtps.سائيز());
    لكل (سائيز_t i = 0; i < stmtps.سائيز(); ++i) {
        آخري_Use[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](ثابت Ast_ڳنڍ_Var_Ref* refp) {
            ثابت خودڪار it = declIdx.ڳوليو(refp->varp());  // 'end()' if not one of our locals
            إذا (it != declIdx.پڇاڙي()) آخري_Use[it->ٻيو] = std::max(آخري_Use[it->ٻيو], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::ويڪٽر<منطقي> breakable(stmtps.سائيز(), سچ);
    سائيز_t جيئرو_پڇاڙي = 0;  // Last index any so far declared local is referenced at
    لكل (سائيز_t i = 0; i < stmtps.سائيز(); ++i) {
        breakable[i] = جيئرو_پڇاڙي < i;
        إذا (VN_IS(stmtps[i], Var)) جيئرو_پڇاڙي = std::max(جيئرو_پڇاڙي, آخري_Use[i]);
    }
    أرجع breakable;
}

// Split large function according to --output-split-cfuncs
فراغ ورهايو_Check(AstCFunc* ثابت ofuncp) {
    إذا (!ofuncp) أرجع;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    إذا (!v3Global.opt.output_ورهايو_CFuncs() || !ofuncp->stmtsp()) أرجع;
    إذا (ofuncp->ڳنڍ_ڳڻپ() < v3Global.opt.output_ورهايو_CFuncs()) أرجع;

    // Statement boundaries that would separate a local declaration from a reference to it
    ثابت std::ويڪٽر<منطقي> breakable = ورهايو_Check_Breakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<ثابت AstVar*, AstVarScope*> argVscps;
    لكل (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.سائيز() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        منطقي مليو = خطأ;
        لكل (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            إذا (vscp->varp() != argp) استمر;
            argVscps[argp] = vscp;
            مليو = سچ;
            اكسر;
        }
        UASSERT_OBJ(مليو, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    ثابت VNUser3_اندريون_Use user3_اندريون_Use;

    سائيز_t سائيز = 0;
    AstCFunc* subFuncp = خالي_اشارو;

    // Move statements one by one to the new sub-functions
    Ast_ڳنڍ* stmtsp = ofuncp->stmtsp()->unlink_Fr_پٺ_With_ايندڙ();
    لكل (سائيز_t اشاريو = 0; Ast_ڳنڍ* ثابت itemp = stmtsp; ++اشاريو) {
        stmtsp = stmtsp->nextp();
        إذا (stmtsp) stmtsp->unlink_Fr_پٺ_With_ايندڙ();
        ثابت سائيز_t شي_سائيز = static_cast<سائيز_t>(itemp->ڳنڍ_ڳڻپ());
        سائيز += شي_سائيز;

        إذا (سائيز > static_cast<سائيز_t>(v3Global.opt.output_ورهايو_CFuncs()) && breakable[اشاريو]) {
            إذا (subFuncp) ورهايو_Check_پڄاڻي_Sub_Func(ofuncp, subFuncp, argVscps);
            subFuncp = خالي_اشارو;
            سائيز = شي_سائيز;
        }

        إذا (!subFuncp) subFuncp = ورهايو_Check_Create_New_Sub_Func(ofuncp);
        subFuncp->جوڙيو_Stmtsp(itemp);
    }
    إذا (subFuncp) ورهايو_Check_پڄاڻي_Sub_Func(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
