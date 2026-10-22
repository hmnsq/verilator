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
    AstScope* ثابت scopeTopp = netlistp->چوٹی_Scopep()->scopep();
    AstCFunc* ثابت funcp = جديد AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(سچ);
    funcp->isStatic(خطأ);
    funcp->isLoose(سچ);
    funcp->slow(slow);
    funcp->isConst(خطأ);
    funcp->decl_نجی(سچ);
    scopeTopp->جوڑو_Blocksp(funcp);
    أرجع funcp;
}

AstVarScope* new_دلیل(AstCFunc* funcp, Ast_گرہ_DType* dtypep, ثابت string& name,
                         VDirection direction) {
    File_سطر* ثابت flp = funcp->fileline();
    AstScope* ثابت scopep = funcp->scopep();
    AstVar* ثابت varp = جديد AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(سچ);
    varp->direction(direction);
    funcp->جوڑو_Argsp(varp);
    AstVarScope* ثابت vscp = جديد AstVarScope{flp, scopep, varp};
    scopep->جوڑو_Varsp(vscp);
    أرجع vscp;
}

Ast_گرہ_Stmt* سیٹ_Var(AstVarScope* vscp, uint32_t val) {
    File_سطر* ثابت flp = vscp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* ثابت valp = جديد AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->عدد().سیٹ_Long(val);
    أرجع جديد Ast_سونپو{flp, refp, valp};
}

Ast_گرہ_Stmt* incrementVar(AstVarScope* vscp) {
    File_سطر* ثابت flp = vscp->fileline();
    AstVarRef* ثابت wrefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* ثابت rrefp = جديد AstVarRef{flp, vscp, VAccess::READ};
    AstConst* ثابت onep = جديد AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->عدد().سیٹ_Long(1);
    أرجع جديد Ast_سونپو{flp, wrefp, جديد Ast_جوڑو{flp, rrefp, onep}};
}

Ast_گرہ_Stmt* callVoidFunc(AstCFunc* funcp) {
    إذا (!funcp) أرجع خالی_سوچک;
    AstCCall* ثابت callp = جديد AstCCall{funcp->fileline(), funcp};
    callp->dtype_سیٹ_Void();
    أرجع callp->makeStmt();
}

ساكن AstCFunc* ونڈو_Check_Create_New_Sub_Func(AstCFunc* ofuncp) {
    ساكن std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    ثابت uint32_t func_عدد = s_funcNums[ofuncp]++;
    ثابت std::string name = ofuncp->name() + "__" + cvt_منزل_Str(func_عدد);
    AstScope* ثابت scopep = ofuncp->scopep();
    AstCFunc* ثابت subFuncp = جديد AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->جوڑو_Blocksp(subFuncp);
    subFuncp->dontCombine(سچ);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(سچ);
    subFuncp->slow(ofuncp->slow());
    subFuncp->decl_نجی(ofuncp->decl_نجی());
    إذا (ofuncp->needProcess()) subFuncp->سیٹ_Need_Process();
    لكل (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* ثابت clonep = argp->clone_درخت(خطأ);
        subFuncp->جوڑو_Argsp(clonep);
        AstVarScope* ثابت vscp = جديد AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->جوڑو_Varsp(vscp);
        argp->user3p(vscp);
    }
    أرجع subFuncp;
};

فراغ ونڈو_Check_انت_Sub_Func(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             ثابت std::unordered_map<ثابت AstVar*, AstVarScope*>& argVscps) {
    File_سطر* ثابت flp = subFuncp->fileline();
    AstCCall* ثابت callp = جديد AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtype_سیٹ_Void();
    // Pass arguments through to subfunction
    لكل (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->جوڑو_Argsp(جديد AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    منطقي شامل_اے_Await = خطأ;
    subFuncp->foreach([&](Ast_گرہ* nodep) {
        // Record if it has a CAwait
        إذا (VN_IS(nodep, CAwait)) شامل_اے_Await = سچ;
        // Redirect references to arguments to the clone in the sub-function
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            إذا (AstVarScope* ثابت vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    إذا (ofuncp->isCoroutine() && شامل_اے_Await) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->جوڑو_Stmtsp(جديد AstCAwait{flp, callp});
    } وإلا {
        ofuncp->جوڑو_Stmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
ساكن std::سمتیہ<منطقي> ونڈو_Check_Breakable(ثابت AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::سمتیہ<ثابت Ast_گرہ*> stmtps;
    std::unordered_map<ثابت AstVar*, ناپ_t> declIdx;  // Local var -> index it is declared at
    لكل (ثابت Ast_گرہ* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.ناپ());
        stmtps.ڈالو_پٹھ(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::سمتیہ<ناپ_t> آخری_Use(stmtps.ناپ());
    لكل (ناپ_t i = 0; i < stmtps.ناپ(); ++i) {
        آخری_Use[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](ثابت Ast_گرہ_Var_Ref* refp) {
            ثابت خودکار it = declIdx.لبھو(refp->varp());  // 'end()' if not one of our locals
            إذا (it != declIdx.سرا()) آخری_Use[it->دوجا] = std::max(آخری_Use[it->دوجا], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::سمتیہ<منطقي> breakable(stmtps.ناپ(), سچ);
    ناپ_t جیوندا_سرا = 0;  // Last index any so far declared local is referenced at
    لكل (ناپ_t i = 0; i < stmtps.ناپ(); ++i) {
        breakable[i] = جیوندا_سرا < i;
        إذا (VN_IS(stmtps[i], Var)) جیوندا_سرا = std::max(جیوندا_سرا, آخری_Use[i]);
    }
    أرجع breakable;
}

// Split large function according to --output-split-cfuncs
فراغ ونڈو_Check(AstCFunc* ثابت ofuncp) {
    إذا (!ofuncp) أرجع;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    إذا (!v3Global.opt.output_ونڈو_CFuncs() || !ofuncp->stmtsp()) أرجع;
    إذا (ofuncp->گرہ_گنتی() < v3Global.opt.output_ونڈو_CFuncs()) أرجع;

    // Statement boundaries that would separate a local declaration from a reference to it
    ثابت std::سمتیہ<منطقي> breakable = ونڈو_Check_Breakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<ثابت AstVar*, AstVarScope*> argVscps;
    لكل (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.ناپ() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        منطقي لبھیا = خطأ;
        لكل (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            إذا (vscp->varp() != argp) استمر;
            argVscps[argp] = vscp;
            لبھیا = سچ;
            اكسر;
        }
        UASSERT_OBJ(لبھیا, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    ثابت VNUser3_وچ_Use user3_وچ_Use;

    ناپ_t ناپ = 0;
    AstCFunc* subFuncp = خالی_سوچک;

    // Move statements one by one to the new sub-functions
    Ast_گرہ* stmtsp = ofuncp->stmtsp()->unlink_Fr_پٹھ_With_اگلا();
    لكل (ناپ_t اشاریہ = 0; Ast_گرہ* ثابت itemp = stmtsp; ++اشاریہ) {
        stmtsp = stmtsp->nextp();
        إذا (stmtsp) stmtsp->unlink_Fr_پٹھ_With_اگلا();
        ثابت ناپ_t شے_ناپ = static_cast<ناپ_t>(itemp->گرہ_گنتی());
        ناپ += شے_ناپ;

        إذا (ناپ > static_cast<ناپ_t>(v3Global.opt.output_ونڈو_CFuncs()) && breakable[اشاریہ]) {
            إذا (subFuncp) ونڈو_Check_انت_Sub_Func(ofuncp, subFuncp, argVscps);
            subFuncp = خالی_سوچک;
            ناپ = شے_ناپ;
        }

        إذا (!subFuncp) subFuncp = ونڈو_Check_Create_New_Sub_Func(ofuncp);
        subFuncp->جوڑو_Stmtsp(itemp);
    }
    إذا (subFuncp) ونڈو_Check_انت_Sub_Func(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
