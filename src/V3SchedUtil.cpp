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
    AstScope* ثابت scopeTopp = netlistp->سر_Scopep()->scopep();
    AstCFunc* ثابت funcp = جديد AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(ریښتیا);
    funcp->isStatic(خطأ);
    funcp->isLoose(ریښتیا);
    funcp->slow(slow);
    funcp->isConst(خطأ);
    funcp->decl_شخصي(ریښتیا);
    scopeTopp->جمع_کړه_Blocksp(funcp);
    أرجع funcp;
}

AstVarScope* new_دلیل(AstCFunc* funcp, Ast_غوټه_DType* dtypep, ثابت string& name,
                         VDirection direction) {
    File_کرښه* ثابت flp = funcp->fileline();
    AstScope* ثابت scopep = funcp->scopep();
    AstVar* ثابت varp = جديد AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(ریښتیا);
    varp->direction(direction);
    funcp->جمع_کړه_Argsp(varp);
    AstVarScope* ثابت vscp = جديد AstVarScope{flp, scopep, varp};
    scopep->جمع_کړه_Varsp(vscp);
    أرجع vscp;
}

Ast_غوټه_Stmt* سېټ_Var(AstVarScope* vscp, uint32_t val) {
    File_کرښه* ثابت flp = vscp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* ثابت valp = جديد AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->عدد().سېټ_Long(val);
    أرجع جديد Ast_ټاکل{flp, refp, valp};
}

Ast_غوټه_Stmt* incrementVar(AstVarScope* vscp) {
    File_کرښه* ثابت flp = vscp->fileline();
    AstVarRef* ثابت wrefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* ثابت rrefp = جديد AstVarRef{flp, vscp, VAccess::READ};
    AstConst* ثابت onep = جديد AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->عدد().سېټ_Long(1);
    أرجع جديد Ast_ټاکل{flp, wrefp, جديد Ast_جمع_کړه{flp, rrefp, onep}};
}

Ast_غوټه_Stmt* callVoidFunc(AstCFunc* funcp) {
    إذا (!funcp) أرجع تش_اشاره;
    AstCCall* ثابت callp = جديد AstCCall{funcp->fileline(), funcp};
    callp->dtype_سېټ_Void();
    أرجع callp->makeStmt();
}

ساكن AstCFunc* وېشل_Check_Create_New_Sub_Func(AstCFunc* ofuncp) {
    ساكن std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    ثابت uint32_t func_عدد = s_funcNums[ofuncp]++;
    ثابت std::string name = ofuncp->name() + "__" + cvt_منزل_Str(func_عدد);
    AstScope* ثابت scopep = ofuncp->scopep();
    AstCFunc* ثابت subFuncp = جديد AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->جمع_کړه_Blocksp(subFuncp);
    subFuncp->dontCombine(ریښتیا);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(ریښتیا);
    subFuncp->slow(ofuncp->slow());
    subFuncp->decl_شخصي(ofuncp->decl_شخصي());
    إذا (ofuncp->needProcess()) subFuncp->سېټ_Need_Process();
    لكل (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* ثابت clonep = argp->clone_ونه(خطأ);
        subFuncp->جمع_کړه_Argsp(clonep);
        AstVarScope* ثابت vscp = جديد AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->جمع_کړه_Varsp(vscp);
        argp->user3p(vscp);
    }
    أرجع subFuncp;
};

فراغ وېشل_Check_پای_Sub_Func(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             ثابت std::unordered_map<ثابت AstVar*, AstVarScope*>& argVscps) {
    File_کرښه* ثابت flp = subFuncp->fileline();
    AstCCall* ثابت callp = جديد AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtype_سېټ_Void();
    // Pass arguments through to subfunction
    لكل (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->جمع_کړه_Argsp(جديد AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    منطقي لري_Await = خطأ;
    subFuncp->foreach([&](Ast_غوټه* nodep) {
        // Record if it has a CAwait
        إذا (VN_IS(nodep, CAwait)) لري_Await = ریښتیا;
        // Redirect references to arguments to the clone in the sub-function
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            إذا (AstVarScope* ثابت vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    إذا (ofuncp->isCoroutine() && لري_Await) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->جمع_کړه_Stmtsp(جديد AstCAwait{flp, callp});
    } وإلا {
        ofuncp->جمع_کړه_Stmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
ساكن std::ویکټور<منطقي> وېشل_Check_Breakable(ثابت AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::ویکټور<ثابت Ast_غوټه*> stmtps;
    std::unordered_map<ثابت AstVar*, اندازه_t> declIdx;  // Local var -> index it is declared at
    لكل (ثابت Ast_غوټه* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.اندازه());
        stmtps.ټېل_شا(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::ویکټور<اندازه_t> وروستی_Use(stmtps.اندازه());
    لكل (اندازه_t i = 0; i < stmtps.اندازه(); ++i) {
        وروستی_Use[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](ثابت Ast_غوټه_Var_Ref* refp) {
            ثابت خپلکاری it = declIdx.پیدا_کړه(refp->varp());  // 'end()' if not one of our locals
            إذا (it != declIdx.پای_ټکی()) وروستی_Use[it->دوهم] = std::max(وروستی_Use[it->دوهم], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::ویکټور<منطقي> breakable(stmtps.اندازه(), ریښتیا);
    اندازه_t ژوندی_پای_ټکی = 0;  // Last index any so far declared local is referenced at
    لكل (اندازه_t i = 0; i < stmtps.اندازه(); ++i) {
        breakable[i] = ژوندی_پای_ټکی < i;
        إذا (VN_IS(stmtps[i], Var)) ژوندی_پای_ټکی = std::max(ژوندی_پای_ټکی, وروستی_Use[i]);
    }
    أرجع breakable;
}

// Split large function according to --output-split-cfuncs
فراغ وېشل_Check(AstCFunc* ثابت ofuncp) {
    إذا (!ofuncp) أرجع;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    إذا (!v3Global.opt.output_وېشل_CFuncs() || !ofuncp->stmtsp()) أرجع;
    إذا (ofuncp->غوټه_شمېر() < v3Global.opt.output_وېشل_CFuncs()) أرجع;

    // Statement boundaries that would separate a local declaration from a reference to it
    ثابت std::ویکټور<منطقي> breakable = وېشل_Check_Breakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<ثابت AstVar*, AstVarScope*> argVscps;
    لكل (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.اندازه() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        منطقي وموندل = خطأ;
        لكل (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            إذا (vscp->varp() != argp) استمر;
            argVscps[argp] = vscp;
            وموندل = ریښتیا;
            اكسر;
        }
        UASSERT_OBJ(وموندل, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    ثابت VNUser3_دننه_Use user3_دننه_Use;

    اندازه_t اندازه = 0;
    AstCFunc* subFuncp = تش_اشاره;

    // Move statements one by one to the new sub-functions
    Ast_غوټه* stmtsp = ofuncp->stmtsp()->unlink_Fr_شا_With_بل();
    لكل (اندازه_t شاخص = 0; Ast_غوټه* ثابت itemp = stmtsp; ++شاخص) {
        stmtsp = stmtsp->nextp();
        إذا (stmtsp) stmtsp->unlink_Fr_شا_With_بل();
        ثابت اندازه_t توکی_اندازه = static_cast<اندازه_t>(itemp->غوټه_شمېر());
        اندازه += توکی_اندازه;

        إذا (اندازه > static_cast<اندازه_t>(v3Global.opt.output_وېشل_CFuncs()) && breakable[شاخص]) {
            إذا (subFuncp) وېشل_Check_پای_Sub_Func(ofuncp, subFuncp, argVscps);
            subFuncp = تش_اشاره;
            اندازه = توکی_اندازه;
        }

        إذا (!subFuncp) subFuncp = وېشل_Check_Create_New_Sub_Func(ofuncp);
        subFuncp->جمع_کړه_Stmtsp(itemp);
    }
    إذا (subFuncp) وېشل_Check_پای_Sub_Func(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
