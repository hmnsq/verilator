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

AstCFunc* makeSubFunction(AstNetlist* netlistp, ಸ್ಥಿರಾಂಕ string& name, ತಾರ್ಕಿಕ slow) {
    AstScope* ಸ್ಥಿರಾಂಕ scopeTopp = netlistp->ತುದಿ_Scopep()->scopep();
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp = ಹೊಸ AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(ನಿಜ);
    funcp->isStatic(ಸುಳ್ಳು);
    funcp->isLoose(ನಿಜ);
    funcp->slow(slow);
    funcp->isConst(ಸುಳ್ಳು);
    funcp->decl_ಖಾಸಗಿ(ನಿಜ);
    scopeTopp->ಕೂಡಿಸು_Blocksp(funcp);
    ಹಿಂತಿರುಗಿ funcp;
}

AstVarScope* new_ವಾದ(AstCFunc* funcp, Ast_ಗಂಟು_DType* dtypep, ಸ್ಥಿರಾಂಕ string& name,
                         VDirection direction) {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = funcp->fileline();
    AstScope* ಸ್ಥಿರಾಂಕ scopep = funcp->scopep();
    AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(ನಿಜ);
    varp->direction(direction);
    funcp->ಕೂಡಿಸು_Argsp(varp);
    AstVarScope* ಸ್ಥಿರಾಂಕ vscp = ಹೊಸ AstVarScope{flp, scopep, varp};
    scopep->ಕೂಡಿಸು_Varsp(vscp);
    ಹಿಂತಿರುಗಿ vscp;
}

Ast_ಗಂಟು_Stmt* ಗಣ_Var(AstVarScope* vscp, uint32_t val) {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
    AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* ಸ್ಥಿರಾಂಕ valp = ಹೊಸ AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->ಅಂಕೆ().ಗಣ_Long(val);
    ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, refp, valp};
}

Ast_ಗಂಟು_Stmt* incrementVar(AstVarScope* vscp) {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
    AstVarRef* ಸ್ಥಿರಾಂಕ wrefp = ಹೊಸ AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* ಸ್ಥಿರಾಂಕ rrefp = ಹೊಸ AstVarRef{flp, vscp, VAccess::READ};
    AstConst* ಸ್ಥಿರಾಂಕ onep = ಹೊಸ AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->ಅಂಕೆ().ಗಣ_Long(1);
    ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, wrefp, ಹೊಸ Ast_ಕೂಡಿಸು{flp, rrefp, onep}};
}

Ast_ಗಂಟು_Stmt* callVoidFunc(AstCFunc* funcp) {
    ಆದರೆ (!funcp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{funcp->fileline(), funcp};
    callp->dtype_ಗಣ_Void();
    ಹಿಂತಿರುಗಿ callp->makeStmt();
}

ಸ್ಥಿರ AstCFunc* ವಿಭಜಿಸು_Check_Create_New_Sub_Func(AstCFunc* ofuncp) {
    ಸ್ಥಿರ std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    ಸ್ಥಿರಾಂಕ uint32_t func_ಅಂಕೆ = s_funcNums[ofuncp]++;
    ಸ್ಥಿರಾಂಕ std::string name = ofuncp->name() + "__" + cvt_ಗಮ್ಯ_Str(func_ಅಂಕೆ);
    AstScope* ಸ್ಥಿರಾಂಕ scopep = ofuncp->scopep();
    AstCFunc* ಸ್ಥಿರಾಂಕ subFuncp = ಹೊಸ AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->ಕೂಡಿಸು_Blocksp(subFuncp);
    subFuncp->dontCombine(ನಿಜ);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(ನಿಜ);
    subFuncp->slow(ofuncp->slow());
    subFuncp->decl_ಖಾಸಗಿ(ofuncp->decl_ಖಾಸಗಿ());
    ಆದರೆ (ofuncp->needProcess()) subFuncp->ಗಣ_Need_Process();
    ಗಾಗಿ (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* ಸ್ಥಿರಾಂಕ clonep = argp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        subFuncp->ಕೂಡಿಸು_Argsp(clonep);
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = ಹೊಸ AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->ಕೂಡಿಸು_Varsp(vscp);
        argp->user3p(vscp);
    }
    ಹಿಂತಿರುಗಿ subFuncp;
};

ಶೂನ್ಯ ವಿಭಜಿಸು_Check_ಮುಕ್ತಾಯ_Sub_Func(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             ಸ್ಥಿರಾಂಕ std::unordered_map<ಸ್ಥಿರಾಂಕ AstVar*, AstVarScope*>& argVscps) {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = subFuncp->fileline();
    AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtype_ಗಣ_Void();
    // Pass arguments through to subfunction
    ಗಾಗಿ (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->ಕೂಡಿಸು_Argsp(ಹೊಸ AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    ತಾರ್ಕಿಕ ಒಳಗೊಂಡಿದೆ_Await = ಸುಳ್ಳು;
    subFuncp->foreach([&](Ast_ಗಂಟು* nodep) {
        // Record if it has a CAwait
        ಆದರೆ (VN_IS(nodep, CAwait)) ಒಳಗೊಂಡಿದೆ_Await = ನಿಜ;
        // Redirect references to arguments to the clone in the sub-function
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, VarRef)) {
            ಆದರೆ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    ಆದರೆ (ofuncp->isCoroutine() && ಒಳಗೊಂಡಿದೆ_Await) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCAwait{flp, callp});
    } ಇಲ್ಲದಿದ್ದರೆ {
        ofuncp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
ಸ್ಥಿರ std::ಸದಿಶ<ತಾರ್ಕಿಕ> ವಿಭಜಿಸು_Check_Breakable(ಸ್ಥಿರಾಂಕ AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*> stmtps;
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstVar*, ಗಾತ್ರ_t> declIdx;  // Local var -> index it is declared at
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.ಗಾತ್ರ_2());
        stmtps.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::ಸದಿಶ<ಗಾತ್ರ_t> ಕೊನೆಯ_Use(stmtps.ಗಾತ್ರ_2());
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < stmtps.ಗಾತ್ರ_2(); ++i) {
        ಕೊನೆಯ_Use[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* refp) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = declIdx.ಕಂಡುಹಿಡಿ(refp->varp());  // 'end()' if not one of our locals
            ಆದರೆ (it != declIdx.ಕೊನೆ()) ಕೊನೆಯ_Use[it->ಎರಡನೇ] = std::max(ಕೊನೆಯ_Use[it->ಎರಡನೇ], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::ಸದಿಶ<ತಾರ್ಕಿಕ> breakable(stmtps.ಗಾತ್ರ_2(), ನಿಜ);
    ಗಾತ್ರ_t ಜೀವಂತ_ಕೊನೆ = 0;  // Last index any so far declared local is referenced at
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < stmtps.ಗಾತ್ರ_2(); ++i) {
        breakable[i] = ಜೀವಂತ_ಕೊನೆ < i;
        ಆದರೆ (VN_IS(stmtps[i], Var)) ಜೀವಂತ_ಕೊನೆ = std::max(ಜೀವಂತ_ಕೊನೆ, ಕೊನೆಯ_Use[i]);
    }
    ಹಿಂತಿರುಗಿ breakable;
}

// Split large function according to --output-split-cfuncs
ಶೂನ್ಯ ವಿಭಜಿಸು_Check(AstCFunc* ಸ್ಥಿರಾಂಕ ofuncp) {
    ಆದರೆ (!ofuncp) ಹಿಂತಿರುಗಿ;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    ಆದರೆ (!v3Global.opt.output_ವಿಭಜಿಸು_CFuncs() || !ofuncp->stmtsp()) ಹಿಂತಿರುಗಿ;
    ಆದರೆ (ofuncp->ಗಂಟು_ಎಣಿಕೆ() < v3Global.opt.output_ವಿಭಜಿಸು_CFuncs()) ಹಿಂತಿರುಗಿ;

    // Statement boundaries that would separate a local declaration from a reference to it
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ತಾರ್ಕಿಕ> breakable = ವಿಭಜಿಸು_Check_Breakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstVar*, AstVarScope*> argVscps;
    ಗಾಗಿ (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.ಗಾತ್ರ_2() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        ತಾರ್ಕಿಕ ಸಿಕ್ಕಿತು = ಸುಳ್ಳು;
        ಗಾಗಿ (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            ಆದರೆ (vscp->varp() != argp) ಮುಂದುವರಿ;
            argVscps[argp] = vscp;
            ಸಿಕ್ಕಿತು = ನಿಜ;
            ನಿಲ್ಲಿಸು;
        }
        UASSERT_OBJ(ಸಿಕ್ಕಿತು, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use user3_ಒಳ_Use;

    ಗಾತ್ರ_t ಗಾತ್ರ_2 = 0;
    AstCFunc* subFuncp = ಶೂನ್ಯಸೂಚಕ;

    // Move statements one by one to the new sub-functions
    Ast_ಗಂಟು* stmtsp = ofuncp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
    ಗಾಗಿ (ಗಾತ್ರ_t ಸೂಚ್ಯಂಕ = 0; Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ itemp = stmtsp; ++ಸೂಚ್ಯಂಕ) {
        stmtsp = stmtsp->nextp();
        ಆದರೆ (stmtsp) stmtsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಅಂಶ_ಗಾತ್ರ = static_cast<ಗಾತ್ರ_t>(itemp->ಗಂಟು_ಎಣಿಕೆ());
        ಗಾತ್ರ_2 += ಅಂಶ_ಗಾತ್ರ;

        ಆದರೆ (ಗಾತ್ರ_2 > static_cast<ಗಾತ್ರ_t>(v3Global.opt.output_ವಿಭಜಿಸು_CFuncs()) && breakable[ಸೂಚ್ಯಂಕ]) {
            ಆದರೆ (subFuncp) ವಿಭಜಿಸು_Check_ಮುಕ್ತಾಯ_Sub_Func(ofuncp, subFuncp, argVscps);
            subFuncp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾತ್ರ_2 = ಅಂಶ_ಗಾತ್ರ;
        }

        ಆದರೆ (!subFuncp) subFuncp = ವಿಭಜಿಸು_Check_Create_New_Sub_Func(ofuncp);
        subFuncp->ಕೂಡಿಸು_Stmtsp(itemp);
    }
    ಆದರೆ (subFuncp) ವಿಭಜಿಸು_Check_ಮುಕ್ತಾಯ_Sub_Func(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
