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
    AstScope* ध्रुव scopeTopp = netlistp->शिखरम्_Scopep()->scopep();
    AstCFunc* ध्रुव funcp = नूतन AstCFunc{netlistp->fileline(), name, scopeTopp, ""};
    funcp->dontCombine(सत्य);
    funcp->isStatic(असत्य);
    funcp->isLoose(सत्य);
    funcp->slow(slow);
    funcp->isConst(असत्य);
    funcp->decl_वैयक्तिकम्(सत्य);
    scopeTopp->सङ्कलय_Blocksp(funcp);
    प्रत्यागम funcp;
}

AstVarScope* new_तर्कः(AstCFunc* funcp, Ast_ग्रन्थिः_DType* dtypep, ध्रुव string& name,
                         VDirection direction) {
    File_पङ्क्तिः* ध्रुव flp = funcp->fileline();
    AstScope* ध्रुव scopep = funcp->scopep();
    AstVar* ध्रुव varp = नूतन AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(सत्य);
    varp->direction(direction);
    funcp->सङ्कलय_Argsp(varp);
    AstVarScope* ध्रुव vscp = नूतन AstVarScope{flp, scopep, varp};
    scopep->सङ्कलय_Varsp(vscp);
    प्रत्यागम vscp;
}

Ast_ग्रन्थिः_Stmt* समुच्चयः_Var(AstVarScope* vscp, uint32_t val) {
    File_पङ्क्तिः* ध्रुव flp = vscp->fileline();
    AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
    AstConst* ध्रुव valp = नूतन AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    valp->अङ्कः().समुच्चयः_Long(val);
    प्रत्यागम नूतन Ast_नियोजय{flp, refp, valp};
}

Ast_ग्रन्थिः_Stmt* incrementVar(AstVarScope* vscp) {
    File_पङ्क्तिः* ध्रुव flp = vscp->fileline();
    AstVarRef* ध्रुव wrefp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
    AstVarRef* ध्रुव rrefp = नूतन AstVarRef{flp, vscp, VAccess::READ};
    AstConst* ध्रुव onep = नूतन AstConst{flp, AstConst::DTyped{}, vscp->dtypep()};
    onep->अङ्कः().समुच्चयः_Long(1);
    प्रत्यागम नूतन Ast_नियोजय{flp, wrefp, नूतन Ast_सङ्कलय{flp, rrefp, onep}};
}

Ast_ग्रन्थिः_Stmt* callVoidFunc(AstCFunc* funcp) {
    यदि (!funcp) प्रत्यागम शून्यसूचकः;
    AstCCall* ध्रुव callp = नूतन AstCCall{funcp->fileline(), funcp};
    callp->dtype_समुच्चयः_Void();
    प्रत्यागम callp->makeStmt();
}

स्थिर AstCFunc* विभज_Check_Create_New_Sub_Func(AstCFunc* ofuncp) {
    स्थिर std::map<AstCFunc*, uint32_t> s_funcNums;  // What split number to attach to a function
    ध्रुव uint32_t func_अङ्कः = s_funcNums[ofuncp]++;
    ध्रुव std::string name = ofuncp->name() + "__" + cvt_गन्तव्यम्_Str(func_अङ्कः);
    AstScope* ध्रुव scopep = ofuncp->scopep();
    AstCFunc* ध्रुव subFuncp = नूतन AstCFunc{ofuncp->fileline(), name, scopep};
    scopep->सङ्कलय_Blocksp(subFuncp);
    subFuncp->dontCombine(सत्य);
    subFuncp->isStatic(ofuncp->isStatic());
    subFuncp->isLoose(सत्य);
    subFuncp->slow(ofuncp->slow());
    subFuncp->decl_वैयक्तिकम्(ofuncp->decl_वैयक्तिकम्());
    यदि (ofuncp->needProcess()) subFuncp->समुच्चयः_Need_Process();
    प्रति (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        AstVar* ध्रुव clonep = argp->clone_वृक्षः(असत्य);
        subFuncp->सङ्कलय_Argsp(clonep);
        AstVarScope* ध्रुव vscp = नूतन AstVarScope{clonep->fileline(), scopep, clonep};
        scopep->सङ्कलय_Varsp(vscp);
        argp->user3p(vscp);
    }
    प्रत्यागम subFuncp;
};

शून्य विभज_Check_Finish_Sub_Func(AstCFunc* ofuncp, AstCFunc* subFuncp,
                             ध्रुव std::unordered_map<ध्रुव AstVar*, AstVarScope*>& argVscps) {
    File_पङ्क्तिः* ध्रुव flp = subFuncp->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{subFuncp->fileline(), subFuncp};
    callp->dtype_समुच्चयः_Void();
    // Pass arguments through to subfunction
    प्रति (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argp->direction() == VDirection::CONSTREF, argp, "Unexpected direction");
        callp->सङ्कलय_Argsp(नूतन AstVarRef{flp, argVscps.at(argp), VAccess::READ});
    }

    तार्किक अन्तर्भवति_Await = असत्य;
    subFuncp->foreach([&](Ast_ग्रन्थिः* nodep) {
        // Record if it has a CAwait
        यदि (VN_IS(nodep, CAwait)) अन्तर्भवति_Await = सत्य;
        // Redirect references to arguments to the clone in the sub-function
        यदि (AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) {
            यदि (AstVarScope* ध्रुव vscp = VN_AS(refp->varp()->user3p(), VarScope)) {
                refp->varp(vscp->varp());
                refp->varScopep(vscp);
            }
        }
    });

    यदि (ofuncp->isCoroutine() && अन्तर्भवति_Await) {  // Wrap call with co_await
        subFuncp->rtnType("VlCoroutine");
        ofuncp->सङ्कलय_Stmtsp(नूतन AstCAwait{flp, callp});
    } अन्यथा {
        ofuncp->सङ्कलय_Stmtsp(callp->makeStmt());
    }
}

// Compute, for each top level statement of 'ofuncp', whether a new sub-function may begin there.
// Sub-functions are emitted as separate C++ functions, so they cannot see each other's automatic
// storage. A function-local AstVar declared among the top level statements must therefore end up
// in the same sub-function as every reference to it, otherwise the emitted C++ refers to an
// undeclared identifier (and the AstVar can be deleted from under the still-live references).
स्थिर std::सदिशम्<तार्किक> विभज_Check_Breakable(ध्रुव AstCFunc* ofuncp) {
    // Gather the top level statements, and where each locally declared variable is declared
    std::सदिशम्<ध्रुव Ast_ग्रन्थिः*> stmtps;
    std::unordered_map<ध्रुव AstVar*, आकार_t> declIdx;  // Local var -> index it is declared at
    प्रति (ध्रुव Ast_ग्रन्थिः* nodep = ofuncp->stmtsp(); nodep; nodep = nodep->nextp()) {
        यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) declIdx.emplace(varp, stmtps.आकार());
        stmtps.प्रक्षिप_पृष्ठभागः(nodep);
    }

    // Find the last statement referencing each locally declared variable
    std::सदिशम्<आकार_t> चरमः_Use(stmtps.आकार());
    प्रति (आकार_t i = 0; i < stmtps.आकार(); ++i) {
        चरमः_Use[i] = i;  // A declaration is live at least where it is declared
        stmtps[i]->foreach([&](ध्रुव Ast_ग्रन्थिः_Var_Ref* refp) {
            ध्रुव स्वचालित it = declIdx.find(refp->varp());  // 'end()' if not one of our locals
            यदि (it != declIdx.अन्तः_2()) चरमः_Use[it->second] = std::max(चरमः_Use[it->second], i);
        });
    }

    // A break before statement 'i' is allowed only if no local declared before 'i' is still live
    std::सदिशम्<तार्किक> breakable(stmtps.आकार(), सत्य);
    आकार_t जीवितम्_अन्तः = 0;  // Last index any so far declared local is referenced at
    प्रति (आकार_t i = 0; i < stmtps.आकार(); ++i) {
        breakable[i] = जीवितम्_अन्तः < i;
        यदि (VN_IS(stmtps[i], Var)) जीवितम्_अन्तः = std::max(जीवितम्_अन्तः, चरमः_Use[i]);
    }
    प्रत्यागम breakable;
}

// Split large function according to --output-split-cfuncs
शून्य विभज_Check(AstCFunc* ध्रुव ofuncp) {
    यदि (!ofuncp) प्रत्यागम;
    UASSERT_OBJ(!ofuncp->varsp(), ofuncp, "Can't split function with local variables");
    यदि (!v3Global.opt.output_विभज_CFuncs() || !ofuncp->stmtsp()) प्रत्यागम;
    यदि (ofuncp->ग्रन्थिः_गणनम्() < v3Global.opt.output_विभज_CFuncs()) प्रत्यागम;

    // Statement boundaries that would separate a local declaration from a reference to it
    ध्रुव std::सदिशम्<तार्किक> breakable = विभज_Check_Breakable(ofuncp);

    // Need to find the AstVarScopes for the function arguments. They should be in the same Scope.
    std::unordered_map<ध्रुव AstVar*, AstVarScope*> argVscps;
    प्रति (AstVar* argp = ofuncp->argsp(); argp; argp = VN_AS(argp->nextp(), Var)) {
        UASSERT_OBJ(argVscps.आकार() < 2, argp, "There should be at most 2 arguments, or O(n^2)");
        तार्किक प्राप्तम् = असत्य;
        प्रति (AstVarScope *vscp = ofuncp->scopep()->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            यदि (vscp->varp() != argp) अनुवर्त;
            argVscps[argp] = vscp;
            प्राप्तम् = सत्य;
            भङ्ग;
        }
        UASSERT_OBJ(प्राप्तम्, argp, "Can't find VarScope for function argument");
    }

    // AstVar::user3p(): AstVarScope for function argument in clone
    ध्रुव VNUser3_अन्तर्_Use user3_अन्तर्_Use;

    आकार_t आकार = 0;
    AstCFunc* subFuncp = शून्यसूचकः;

    // Move statements one by one to the new sub-functions
    Ast_ग्रन्थिः* stmtsp = ofuncp->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र();
    प्रति (आकार_t सूचकांक = 0; Ast_ग्रन्थिः* ध्रुव itemp = stmtsp; ++सूचकांक) {
        stmtsp = stmtsp->nextp();
        यदि (stmtsp) stmtsp->unlink_Fr_पृष्ठभागः_With_अग्र();
        ध्रुव आकार_t item_आकार = static_cast<आकार_t>(itemp->ग्रन्थिः_गणनम्());
        आकार += item_आकार;

        यदि (आकार > static_cast<आकार_t>(v3Global.opt.output_विभज_CFuncs()) && breakable[सूचकांक]) {
            यदि (subFuncp) विभज_Check_Finish_Sub_Func(ofuncp, subFuncp, argVscps);
            subFuncp = शून्यसूचकः;
            आकार = item_आकार;
        }

        यदि (!subFuncp) subFuncp = विभज_Check_Create_New_Sub_Func(ofuncp);
        subFuncp->सङ्कलय_Stmtsp(itemp);
    }
    यदि (subFuncp) विभज_Check_Finish_Sub_Func(ofuncp, subFuncp, argVscps);
}

}  // namespace util

}  // namespace V3Sched
