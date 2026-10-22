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

نامخلا V3Sched {
نامخلا util {

AstCFunc* makeSubFunction(AstNetlist* netlistp, ثابت string& name, منطقی slow) {
    AstScope* ثابت scopeTopp = netlistp->چوٹی_Scopep()->scopep();
    AstCFunc* ثابت funcp = نیا AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(سچ);
    funcp->isStatic(جھوٹ);
    funcp->isLoose(سچ);
    funcp->slow(slow);
    funcp->isConst(جھوٹ);
    funcp->decl_نجی(سچ);
    scopeTopp->جوڑو_Blocksp(funcp);
    واپس funcp;
}

AstVarScope* new_دلیل(AstCFunc* funcp, Ast_گرہ_DType* dtypep, ثابت string& name,
                         VDirection direction) {
    File_سطر* ثابت flp = funcp->fileline();
    AstScope* ثابت scopep = funcp->scopep();
    AstVar* ثابت varp = نیا AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(سچ);
    varp->direction(direction);
    funcp->جوڑو_Argsp(varp);
    AstVarScope* ثابت vscp = نیا AstVarScope{flp, scopep, varp};
    scopep->جوڑو_Varsp(vscp);
    واپس vscp;
}

Ast_گرہ_Stmt* سیٹ_Var(AstVarScope* vscp, uint32_t val) {
    File_سطر* ثابت flp = vscp->fileline();
    AstVarRef* ثابت refp = نیا AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* ثابت valp = نیا AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->عدد().سیٹ_Long(val);
    واپس نیا Ast_تفویض{flp, refp, valp};
}

Ast_گرہ_Stmt* incrementVar(AstVarScope* vscp) {
    File_سطر* ثابت flp = vscp->fileline();
    AstVarRef* ثابت wrefp = نیا AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* ثابت rrefp = نیا AstVarRef{flp, vscp, VAccess::READ};
    AstConst* ثابت onep = نیا AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->عدد().سیٹ_Long(1);
    واپس نیا Ast_تفویض{flp, wrefp, نیا Ast_جوڑو{flp, rrefp, onep}};
}

Ast_گرہ_Stmt* callVoidFunc(AstCFunc* funcp) {
    اگر (!funcp) واپس خالی_اشاریہ;
    AstCCall* ثابت callp = نیا AstCCall{funcp->fileline(), funcp};
    callp->dtype_سیٹ_Void();
    واپس callp->makeStmt();
}

ساکن AstCFunc* بانٹو_Check_Create_New_Sub_Func(AstCFunc* ofuncp) {
    ساکن std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    ثابت uint32_t func_عدد = s_funcNums[ofuncp]++;
    ثابت std::string name = ofuncp->name() + "__" + cvt_مقصد_Str(func_عدد);
    AstScope* ثابت scopep = ofuncp->scopep();
    AstCFunc* ثابت subFuncp = نیا AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->جوڑو_Blocksp(subFuncp);
    subFuncp->dontCombine(سچ);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(سچ);
    subFuncp->slow(ofuncp->slow());
    subFuncp->decl_نجی(ofuncp->decl_نجی());
    اگر (ofuncp->needProcess()) subFuncp->سیٹ_Need_Process();
    برائے (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* ثابت clonep = argp->clone_درخت(جھوٹ);
        subFuncp->جوڑو_Argsp(clonep);
        AstVarScope* ثابت vscp = نیا AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->جوڑو_Varsp(vscp);
        argp->user3p(vscp);
    }
    واپس subFuncp;
};

خالی بانٹو_Check_اختتام_Sub_Func(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             ثابت std::unordered_map<ثابت AstVar*, AstVarScope*>& argVscps) {
    File_سطر* ثابت flp = subFuncp->fileline();
    AstCCall* ثابت callp = نیا AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtype_سیٹ_Void();
    // Pass arguments through to subfunction
    برائے (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->جوڑو_Argsp(نیا AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    منطقی شامل_ہے_Await = جھوٹ;
    subFuncp->foreach([&](Ast_گرہ* nodep) {
        // Record if it has a CAwait
        اگر (VN_IS(nodep, CAwait)) شامل_ہے_Await = سچ;
        // Redirect references to arguments to the clone in the sub-function
        اگر (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            اگر (AstVarScope* ثابت vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    اگر (ofuncp->isCoroutine() && شامل_ہے_Await) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->جوڑو_Stmtsp(نیا AstCAwait{flp, callp});
    } ورنہ {
        ofuncp->جوڑو_Stmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
ساکن std::سمتیہ<منطقی> بانٹو_Check_Breakable(ثابت AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::سمتیہ<ثابت Ast_گرہ*> stmtps;
    std::unordered_map<ثابت AstVar*, سائز_t> declIdx;  // Local var -> index it is declared at
    برائے (ثابت Ast_گرہ* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        اگر (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.سائز());
        stmtps.ڈالو_عقب(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::سمتیہ<سائز_t> آخری_Use(stmtps.سائز());
    برائے (سائز_t i = 0; i < stmtps.سائز(); ++i) {
        آخری_Use[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](ثابت Ast_گرہ_Var_Ref* refp) {
            ثابت خودکار it = declIdx.ڈھونڈو(refp->varp());  // 'end()' if not one of our locals
            اگر (it != declIdx.انتہا()) آخری_Use[it->دوسرا] = std::max(آخری_Use[it->دوسرا], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::سمتیہ<منطقی> breakable(stmtps.سائز(), سچ);
    سائز_t زندہ_انتہا = 0;  // Last index any so far declared local is referenced at
    برائے (سائز_t i = 0; i < stmtps.سائز(); ++i) {
        breakable[i] = زندہ_انتہا < i;
        اگر (VN_IS(stmtps[i], Var)) زندہ_انتہا = std::max(زندہ_انتہا, آخری_Use[i]);
    }
    واپس breakable;
}

// Split large function according to --output-split-cfuncs
خالی بانٹو_Check(AstCFunc* ثابت ofuncp) {
    اگر (!ofuncp) واپس;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    اگر (!v3Global.opt.output_بانٹو_CFuncs() || !ofuncp->stmtsp()) واپس;
    اگر (ofuncp->گرہ_گنتی() < v3Global.opt.output_بانٹو_CFuncs()) واپس;

    // Statement boundaries that would separate a local declaration from a reference to it
    ثابت std::سمتیہ<منطقی> breakable = بانٹو_Check_Breakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<ثابت AstVar*, AstVarScope*> argVscps;
    برائے (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.سائز() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        منطقی ملا = جھوٹ;
        برائے (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            اگر (vscp->varp() != argp) جاری;
            argVscps[argp] = vscp;
            ملا = سچ;
            توڑو;
        }
        UASSERT_OBJ(ملا, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    ثابت VNUser3_اندر_Use user3_اندر_Use;

    سائز_t سائز = 0;
    AstCFunc* subFuncp = خالی_اشاریہ;

    // Move statements one by one to the new sub-functions
    Ast_گرہ* stmtsp = ofuncp->stmtsp()->unlink_Fr_عقب_With_اگلا();
    برائے (سائز_t اشاریہ = 0; Ast_گرہ* ثابت itemp = stmtsp; ++اشاریہ) {
        stmtsp = stmtsp->nextp();
        اگر (stmtsp) stmtsp->unlink_Fr_عقب_With_اگلا();
        ثابت سائز_t شے_سائز = static_cast<سائز_t>(itemp->گرہ_گنتی());
        سائز += شے_سائز;

        اگر (سائز > static_cast<سائز_t>(v3Global.opt.output_بانٹو_CFuncs()) && breakable[اشاریہ]) {
            اگر (subFuncp) بانٹو_Check_اختتام_Sub_Func(ofuncp, subFuncp, argVscps);
            subFuncp = خالی_اشاریہ;
            سائز = شے_سائز;
        }

        اگر (!subFuncp) subFuncp = بانٹو_Check_Create_New_Sub_Func(ofuncp);
        subFuncp->جوڑو_Stmtsp(itemp);
    }
    اگر (subFuncp) بانٹو_Check_اختتام_Sub_Func(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
