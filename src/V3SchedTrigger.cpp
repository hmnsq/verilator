// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Create triggers for code scheduling
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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3EmitCBase.h"
#അന്തര്ഭാവ "V3EmitV.h"
#അന്തര്ഭാവ "V3Order.h"
#അന്തര്ഭാവ "V3Sched.h"
#അന്തര്ഭാവ "V3SenExprBuilder.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

നാമാകാശ V3Sched {

നാമാകാശ {

using util::newArgument;

AstVarScope* newLocal(AstCFunc* funcp, Ast_കണ്ണി_DType* dtypep, സ്ഥിരം std::string& name) {
    File_വരി* സ്ഥിരം flp = funcp->fileline();
    AstScope* സ്ഥിരം scopep = funcp->scopep();
    AstVar* സ്ഥിരം varp = പുതിയ AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(സത്യം);
    varp->noReset(സത്യം);
    funcp->addVarsp(varp);
    AstVarScope* സ്ഥിരം vscp = പുതിയ AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    തിരികെ vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() സ്ഥിരം {
    UASSERT(m_n_Pre_വാക്കുകൾ, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* സ്ഥിരം netlistp = v3Global.rootp();
    File_വരി* സ്ഥിരം flp = netlistp->മുകൾ_Scopep()->fileline();
    Ast_കണ്ണി_DType* സ്ഥിരം u32DTypep = netlistp->findUInt32DType();
    Ast_കണ്ണി_DType* സ്ഥിരം strDtypep = netlistp->findStringDType();

    // Dumping function always slow
    സ്ഥിരം std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* സ്ഥിരം funcp = util::makeSubFunction(netlistp, name, സത്യം);
    funcp->isStatic(സത്യം);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* സ്ഥിരം eVscp = newArgument(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* സ്ഥിരം tVscp = newArgument(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    സ്ഥിരം auto rd = [flp](AstVarScope* vp) { തിരികെ പുതിയ AstVarRef{flp, vp, VAccess::READ}; };
    സ്ഥിരം auto wr = [flp](AstVarScope* vp) { തിരികെ പുതിയ AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* സ്ഥിരം vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* സ്ഥിരം iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->addStmtsp(util::setVar(iVscp, 0));
        // Add loop
        AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
        funcp->addStmtsp(loopp);
        // Loop body
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = പുതിയ AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = പുതിയ AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം limp = പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, 32, m_n_Vec_വാക്കുകൾ};
        loopp->addStmtsp(പുതിയ AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(iVscp));
        loopp->addStmtsp(പുതിയ AstLoopTest{flp, loopp, പുതിയ AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(vVscp));
        callp->addArgsp(rd(tVscp));
        funcp->addStmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* സ്ഥിരം pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* സ്ഥിരം jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->addStmtsp(util::setVar(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = പുതിയ AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = പുതിയ AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം limp = പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, 32, m_n_Pre_വാക്കുകൾ};
            loopp->addStmtsp(പുതിയ AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(പുതിയ AstLoopTest{flp, loopp, പുതിയ AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = പുതിയ AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = പുതിയ AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം limp = പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, 32, m_n_Vec_വാക്കുകൾ};
            loopp->addStmtsp(പുതിയ AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(പുതിയ AstLoopTest{flp, loopp, പുതിയ AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(pVscp));
        callp->addArgsp(
            പുതിയ AstConcatN{flp, rd(tVscp), പുതിയ AstConst{flp, AstConst::String{}, " pre"}});
        funcp->addStmtsp(callp->makeStmt());
    }

    തിരികെ funcp;
}

AstCFunc* TriggerKit::createAnySetFunc(AstUnpackArrayDType* സ്ഥിരം dtypep) സ്ഥിരം {
    AstNetlist* സ്ഥിരം netlistp = v3Global.rootp();
    File_വരി* സ്ഥിരം flp = netlistp->മുകൾ_Scopep()->fileline();
    Ast_കണ്ണി_DType* സ്ഥിരം u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* സ്ഥിരം funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(സത്യം);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* സ്ഥിരം iVscp = newArgument(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* സ്ഥിരം nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    സ്ഥിരം auto rd = [flp](AstVarScope* vp) { തിരികെ പുതിയ AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);
    funcp->addStmtsp(പുതിയ AstCReturn{flp, പുതിയ AstConst{flp, AstConst::BitFalse{}}});

    // Loop body
    സ്ഥിരം uint32_t n_വാക്കുകൾ = dtypep->elementsConst();
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = പുതിയ AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_കണ്ണി_Stmt* സ്ഥിരം thenp = പുതിയ AstCReturn{flp, പുതിയ AstConst{flp, AstConst::BitTrue{}}};
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം limp = പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, 32, n_വാക്കുകൾ};
    loopp->addStmtsp(പുതിയ AstIf{flp, condp, thenp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(പുതിയ AstLoopTest{flp, loopp, പുതിയ AstLt{flp, rd(nVscp), limp}});

    // Done
    തിരികെ funcp;
}
AstCFunc* TriggerKit::createClearFunc() സ്ഥിരം {
    AstNetlist* സ്ഥിരം netlistp = v3Global.rootp();
    File_വരി* സ്ഥിരം flp = netlistp->മുകൾ_Scopep()->fileline();
    Ast_കണ്ണി_DType* സ്ഥിരം u32DTypep = netlistp->findUInt32DType();

    // Create function
    AstCFunc* സ്ഥിരം funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(സത്യം);

    // Add arguments
    AstVarScope* സ്ഥിരം oVscp = newArgument(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* സ്ഥിരം nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    സ്ഥിരം auto rd = [flp](AstVarScope* vp) { തിരികെ പുതിയ AstVarRef{flp, vp, VAccess::READ}; };
    സ്ഥിരം auto wr = [flp](AstVarScope* vp) { തിരികെ പുതിയ AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = പുതിയ AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = പുതിയ AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം limp = പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, 32, m_n_Vec_വാക്കുകൾ};
    loopp->addStmtsp(പുതിയ AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(പുതിയ AstLoopTest{flp, loopp, പുതിയ AstLt{flp, rd(nVscp), limp}});

    // Done
    തിരികെ funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* സ്ഥിരം oDtypep,
                                       AstUnpackArrayDType* സ്ഥിരം iDtypep) സ്ഥിരം {
    AstNetlist* സ്ഥിരം netlistp = v3Global.rootp();
    File_വരി* സ്ഥിരം flp = netlistp->മുകൾ_Scopep()->fileline();
    Ast_കണ്ണി_DType* സ്ഥിരം u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* സ്ഥിരം funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(സത്യം);

    // Add arguments
    AstVarScope* സ്ഥിരം oVscp = newArgument(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* സ്ഥിരം iVscp = newArgument(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* സ്ഥിരം nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    സ്ഥിരം auto rd = [flp](AstVarScope* vp) { തിരികെ പുതിയ AstVarRef{flp, vp, VAccess::READ}; };
    സ്ഥിരം auto wr = [flp](AstVarScope* vp) { തിരികെ പുതിയ AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = പുതിയ AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം oWordp = പുതിയ AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം iWordp = പുതിയ AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = പുതിയ AstOr{flp, oWordp, iWordp};
    AstConst* സ്ഥിരം outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* സ്ഥിരം നിവേശം_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം limp = outputRangeLeftp->അക്കം().toSInt() < നിവേശം_Range_Leftp->അക്കം().toSInt()
                                  ? outputRangeLeftp->clone_വൃക്ഷം_Pure(അസത്യം)
                                  : നിവേശം_Range_Leftp->clone_വൃക്ഷം_Pure(അസത്യം);
    loopp->addStmtsp(പുതിയ AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(പുതിയ AstLoopTest{flp, loopp, പുതിയ AstLte{flp, rd(nVscp), limp}});

    // Done
    തിരികെ funcp;
}

Ast_കണ്ണി_വ്യഞ്ജകം* TriggerKit::newAnySetCall(AstVarScope* സ്ഥിരം vscp) സ്ഥിരം {
    File_വരി* സ്ഥിരം flp = v3Global.rootp()->മുകൾ_Scopep()->fileline();
    എങ്കിൽ (!m_n_Vec_വാക്കുകൾ) തിരികെ പുതിയ AstConst{flp, AstConst::BitFalse{}};

    AstCFunc* funcp = nullptr;
    എങ്കിൽ (vscp->dtypep() == m_trigVecDTypep) {
        എങ്കിൽ (!m_anySetVecp) m_anySetVecp = createAnySetFunc(m_trigVecDTypep);
        funcp = m_anySetVecp;
    } അല്ലെങ്കിൽ എങ്കിൽ (vscp->dtypep() == m_trigExtDTypep) {
        എങ്കിൽ (!m_anySetExtp) m_anySetExtp = createAnySetFunc(m_trigExtDTypep);
        funcp = m_anySetExtp;
    } അല്ലെങ്കിൽ {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, funcp};
    callp->addArgsp(പുതിയ AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetBit();
    തിരികെ callp;
}
Ast_കണ്ണി_Stmt* TriggerKit::newClearCall(AstVarScope* സ്ഥിരം vscp) സ്ഥിരം {
    എങ്കിൽ (!m_n_Vec_വാക്കുകൾ) തിരികെ nullptr;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    എങ്കിൽ (!m_clearp) m_clearp = createClearFunc();
    File_വരി* സ്ഥിരം flp = v3Global.rootp()->മുകൾ_Scopep()->fileline();
    AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, m_clearp};
    callp->addArgsp(പുതിയ AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetVoid();
    തിരികെ callp->makeStmt();
}
Ast_കണ്ണി_Stmt* TriggerKit::newOrIntoCall(AstVarScope* സ്ഥിരം oVscp, AstVarScope* സ്ഥിരം iVscp) സ്ഥിരം {
    എങ്കിൽ (!m_n_Vec_വാക്കുകൾ) തിരികെ nullptr;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    സ്ഥിരം size_t mask
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[mask];
    എങ്കിൽ (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_വരി* സ്ഥിരം flp = v3Global.rootp()->മുകൾ_Scopep()->fileline();
    AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, funcp};
    callp->addArgsp(പുതിയ AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->addArgsp(പുതിയ AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtypeSetVoid();
    തിരികെ callp->makeStmt();
}

Ast_കണ്ണി_Stmt* TriggerKit::newCompBaseCall() സ്ഥിരം {
    എങ്കിൽ (!m_n_Vec_വാക്കുകൾ) തിരികെ nullptr;
    File_വരി* സ്ഥിരം flp = v3Global.rootp()->മുകൾ_Scopep()->fileline();
    AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, m_compVecp};
    callp->dtypeSetVoid();
    തിരികെ callp->makeStmt();
}

Ast_കണ്ണി_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) സ്ഥിരം {
    എങ്കിൽ (!m_n_Pre_വാക്കുകൾ) തിരികെ nullptr;
    File_വരി* സ്ഥിരം flp = v3Global.rootp()->മുകൾ_Scopep()->fileline();
    AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, m_compExtp};
    callp->addArgsp(പുതിയ AstVarRef{flp, vscp, VAccess::READ});
    callp->dtypeSetVoid();
    തിരികെ callp->makeStmt();
}

Ast_കണ്ണി_Stmt* TriggerKit::newDumpCall(AstVarScope* സ്ഥിരം vscp, സ്ഥിരം std::string& tag,
                                     തർക്കം debugOnly) സ്ഥിരം {
    എങ്കിൽ (!m_n_Vec_വാക്കുകൾ) തിരികെ nullptr;
    AstCFunc* funcp = nullptr;
    എങ്കിൽ (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } അല്ലെങ്കിൽ എങ്കിൽ (vscp->dtypep() == m_trigExtDTypep) {
        എങ്കിൽ (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } അല്ലെങ്കിൽ {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_വരി* സ്ഥിരം flp = v3Global.rootp()->മുകൾ_Scopep()->fileline();
    AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, funcp};
    callp->addArgsp(പുതിയ AstVarRef{flp, vscp, VAccess::READ});
    callp->addArgsp(പുതിയ AstConst{flp, AstConst::String{}, tag});
    callp->dtypeSetVoid();
    AstCStmt* സ്ഥിരം cstmtp = പുതിയ AstCStmt{flp};
    cstmtp->add("#ifdef VL_DEBUG\n");
    എങ്കിൽ (debugOnly) {
        cstmtp->add("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->add(callp->makeStmt());
        cstmtp->add("}\n");
    } അല്ലെങ്കിൽ {
        cstmtp->add(callp->makeStmt());
    }
    cstmtp->add("#endif");
    തിരികെ cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(സ്ഥിരം std::string& name) സ്ഥിരം {
    എങ്കിൽ (!m_n_Vec_വാക്കുകൾ) തിരികെ nullptr;
    തിരികെ v3Global.rootp()->മുകൾ_Scopep()->createTemp("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extraTriggerIndex(uint32_t index) സ്ഥിരം {
    UASSERT(index <= m_n_Extra_വാക്കുകൾ * WORD_SIZE, "Invalid external trigger index");
    തിരികെ index + m_n_Sense_വാക്കുകൾ * WORD_SIZE;
}

std::set<uint32_t> TriggerKit::triggerIndices(സ്ഥിരം Ast_Sen_വൃക്ഷം* senTreep, തർക്കം pre) സ്ഥിരം {
    std::set<uint32_t> indices;
    വേണ്ടി (സ്ഥിരം AstSenItem* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), SenItem)) {
        uint32_t index = m_senItem2TrigIdx.at(*sip);
        എങ്കിൽ (pre) {
            UASSERT_OBJ(index < m_n_Pre_വാക്കുകൾ * WORD_SIZE, sip, "Not a 'pre' trigger");
            index += m_n_Vec_വാക്കുകൾ * WORD_SIZE;
        }
        indices.തിരുകുക(index);
    }
    തിരികെ indices;
}

Ast_കണ്ണി_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) സ്ഥിരം {
    index = extraTriggerIndex(index);
    സ്ഥിരം uint32_t wordIndex = index / WORD_SIZE;
    സ്ഥിരം uint32_t bitIndex = index % WORD_SIZE;
    File_വരി* സ്ഥിരം flp = vscp->fileline();
    AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം wordp = പുതിയ AstArraySel{flp, refp, static_cast<പൂർണ്ണസംഖ്യ>(wordIndex)};
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം trigLhsp = പുതിയ AstSel{flp, wordp, static_cast<പൂർണ്ണസംഖ്യ>(bitIndex), 1};
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം trigRhsp = പുതിയ AstVarRef{flp, vscp, VAccess::READ};
    തിരികെ പുതിയ AstAssign{flp, trigLhsp, trigRhsp};
}

ശൂന്യം TriggerKit::addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) സ്ഥിരം {
    File_വരി* സ്ഥിരം flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_കണ്ണി* സ്ഥിരം setp = newExtraTriggerAssignment(vscp, index);
    setp->addNext(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, vscp, VAccess::WRITE},
                                പുതിയ AstConst{flp, AstConst::BitFalse{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
ശൂന്യം TriggerKit::add_മൂല്യം_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t index) സ്ഥിരം {
    index += m_n_Sense_വാക്കുകൾ * WORD_SIZE;
    സ്ഥിരം uint32_t wordIndex = index / WORD_SIZE;
    സ്ഥിരം uint32_t bitIndex = index % WORD_SIZE;

    Ast_മുകൾ_Scope* സ്ഥിരം മുകൾ_Scopep = netlistp->മുകൾ_Scopep();
    File_വരി* സ്ഥിരം flp = netlistp->fileline();
    Ast_കണ്ണി_DType* സ്ഥിരം dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം bdtypep = VN_CAST(dtypep, BasicDType)) {
        എങ്കിൽ (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            തിരികെ;
        }
    } അല്ലെങ്കിൽ എങ്കിൽ (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, കണ്ണി_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        തിരികെ;
    }

    സ്ഥിരം auto rdInst = [flp](AstVarScope* vp) { തിരികെ പുതിയ AstVarRef{flp, vp, VAccess::READ}; };
    സ്ഥിരം auto wrPrev = [flp](AstVarScope* vp) { തിരികെ പുതിയ AstVarRef{flp, vp, VAccess::WRITE}; };
    സ്ഥിരം auto rdPrev = [flp](AstVarScope* vp) { തിരികെ പുതിയ AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    സ്ഥിരം std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* സ്ഥിരം prevVscp = മുകൾ_Scopep->createTemp(prevName, instVscp->dtypep());

    // Initialize prev = inst
    എങ്കിൽ (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* സ്ഥിരം cmhp = പുതിയ AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        initFuncp->addStmtsp(cmhp->makeStmt());
    } അല്ലെങ്കിൽ {
        initFuncp->addStmtsp(പുതിയ AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_കണ്ണി_വ്യഞ്ജകം* neqp;
    എങ്കിൽ (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* സ്ഥിരം cmhp
            = പുതിയ AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtypeSetBit();
        neqp = cmhp;
    } അല്ലെങ്കിൽ {
        neqp = പുതിയ AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_കണ്ണി* updp;
    എങ്കിൽ (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* സ്ഥിരം cmhp = പുതിയ AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        updp = cmhp->makeStmt();
    } അല്ലെങ്കിൽ {
        updp = പുതിയ AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം wordp = പുതിയ AstArraySel{flp, refp, static_cast<പൂർണ്ണസംഖ്യ>(wordIndex)};
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം trigLhsp = പുതിയ AstSel{flp, wordp, static_cast<പൂർണ്ണസംഖ്യ>(bitIndex), 1};
    Ast_കണ്ണി* സ്ഥിരം setp = പുതിയ AstAssign{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->addNext(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(സ്ഥിരം std::string& name, തർക്കം slow, uint32_t n_Sense_വാക്കുകൾ,
                       uint32_t n_Extra_വാക്കുകൾ, uint32_t n_Pre_വാക്കുകൾ,
                       std::unordered_map<VNRef<സ്ഥിരം AstSenItem>, size_t> senItem2TrigIdx,
                       സ്ഥിരം std::vector<സ്ഥിരം AstSenItem*>& senItemps, തർക്കം useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_വാക്കുകൾ{n_Sense_വാക്കുകൾ}
    , m_n_Extra_വാക്കുകൾ{n_Extra_വാക്കുകൾ}
    , m_n_Pre_വാക്കുകൾ{n_Pre_വാക്കുകൾ}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trigIdx2SenItem{senItemps} {
    // If no triggers, we don't need to generate anything
    എങ്കിൽ (!m_n_Vec_വാക്കുകൾ) തിരികെ;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trigIdx2SenItem.resize((m_n_Vec_വാക്കുകൾ + m_n_Pre_വാക്കുകൾ) * WORD_SIZE);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_വാക്കുകൾ * WORD_SIZE,
              m_trigIdx2SenItem.begin() + m_n_Vec_വാക്കുകൾ * WORD_SIZE);
    AstNetlist* സ്ഥിരം netlistp = v3Global.rootp();
    AstScope* സ്ഥിരം scopep = netlistp->മുകൾ_Scopep()->scopep();
    File_വരി* സ്ഥിരം flp = scopep->fileline();
    // Data type of a single trigger word
    m_wordDTypep = netlistp->findBitDType(WORD_SIZE, WORD_SIZE, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* സ്ഥിരം rp = പുതിയ AstRange{flp, static_cast<പൂർണ്ണസംഖ്യ>(m_n_Vec_വാക്കുകൾ - 1), 0};
    m_trigVecDTypep = പുതിയ AstUnpackArrayDType{flp, m_wordDTypep, rp};
    netlistp->typeTablep()->addTypesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    എങ്കിൽ (m_n_Pre_വാക്കുകൾ) {
        AstRange* സ്ഥിരം ep = പുതിയ AstRange{flp, static_cast<പൂർണ്ണസംഖ്യ>(m_n_Vec_വാക്കുകൾ + m_n_Pre_വാക്കുകൾ - 1), 0};
        m_trigExtDTypep = പുതിയ AstUnpackArrayDType{flp, m_wordDTypep, ep};
        netlistp->typeTablep()->addTypesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } അല്ലെങ്കിൽ {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->മുകൾ_Scopep()->createTemp("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->isInternal(സത്യം);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, സത്യം);
    m_dumpp->isStatic(സത്യം);
    m_dumpp->ifdef("VL_DEBUG");
    എങ്കിൽ (useAcc) {
        m_vscAccp
            = netlistp->മുകൾ_Scopep()->createTemp("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->isInternal(സത്യം);
    }
}

AstAssign* TriggerKit::createSenTrigVecAssignment(AstVarScope* സ്ഥിരം target,
                                                  std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>& trigps) {
    File_വരി* സ്ഥിരം flp = target->fileline();
    AstAssign* trigStmtsp = nullptr;
    // Assign sense triggers vector one word at a time
    വേണ്ടി (size_t i = 0; i < trigps.size(); i += WORD_SIZE) {
        // Concatenate all bits in this trigger word using a balanced
        വേണ്ടി (uint32_t level = 0; level < WORD_SIZE_LOG2; ++level) {
            സ്ഥിരം uint32_t stride = 1 << level;
            വേണ്ടി (uint32_t j = 0; j < WORD_SIZE; j += 2 * stride) {
                trigps[i + j] = പുതിയ AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = nullptr;
            }
        }

        // Set the whole word in the trigger vector
        സ്ഥിരം പൂർണ്ണസംഖ്യ wordIndex = static_cast<പൂർണ്ണസംഖ്യ>(i / WORD_SIZE);
        AstArraySel* സ്ഥിരം aselp
            = പുതിയ AstArraySel{flp, പുതിയ AstVarRef{flp, target, VAccess::WRITE}, wordIndex};
        trigStmtsp = Ast_കണ്ണി::addNext(trigStmtsp, പുതിയ AstAssign{flp, aselp, trigps[i]});
    }
    തിരികെ trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* സ്ഥിരം initFuncp,  //
                              Sen_വ്യഞ്ജകം_Builder& sen_വ്യഞ്ജകം_Builder,  //
                              സ്ഥിരം std::vector<സ്ഥിരം Ast_Sen_വൃക്ഷം*>& preTreeps,  //
                              സ്ഥിരം std::vector<സ്ഥിരം Ast_Sen_വൃക്ഷം*>& senTreeps,  //
                              സ്ഥിരം string& name,  //
                              സ്ഥിരം ExtraTriggers& extraTriggers,  //
                              തർക്കം slow,  //
                              തർക്കം useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::vector<സ്ഥിരം AstSenItem*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<സ്ഥിരം AstSenItem>, size_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    വേണ്ടി (സ്ഥിരം Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep : preTreeps) {
        വേണ്ടി (സ്ഥിരം AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            സ്ഥിരം auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            എങ്കിൽ (pair.second) senItemps.തള്ളുക_back(itemp);
        }
    }
    സ്ഥിരം uint32_t n_Pre_Sen_ഇനങ്ങൾ = senItemps.size();
    V3Stats::addStat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_ഇനങ്ങൾ);
    // Number of pre triggers, rounded up to a full word.
    സ്ഥിരം uint32_t nPreTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    സ്ഥിരം uint32_t n_Pre_വാക്കുകൾ = nPreTriggers / WORD_SIZE;

    // Process the rest of the trees
    വേണ്ടി (സ്ഥിരം Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep : senTreeps) {
        വേണ്ടി (സ്ഥിരം AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            സ്ഥിരം auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            എങ്കിൽ (pair.second) senItemps.തള്ളുക_back(itemp);
        }
    }
    സ്ഥിരം uint32_t n_Sen_ഇനങ്ങൾ = senItemps.size() - nPreTriggers;
    V3Stats::addStat("Scheduling, '" + name + "' sense triggers", n_Sen_ഇനങ്ങൾ + n_Pre_Sen_ഇനങ്ങൾ);
    // Number of sense triggers, rounded up to a full word
    സ്ഥിരം uint32_t nSenseTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    സ്ഥിരം uint32_t n_Sense_വാക്കുകൾ = nSenseTriggers / WORD_SIZE;

    // Allocate space for the extra triggers
    V3Stats::addStat("Scheduling, '" + name + "' extra triggers", extraTriggers.size());
    // Number of extra triggers, rounded up to a full word.
    സ്ഥിരം uint32_t nExtraTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(extraTriggers.size());
    സ്ഥിരം uint32_t n_Extra_വാക്കുകൾ = nExtraTriggers / WORD_SIZE;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_വാക്കുകൾ, n_Extra_വാക്കുകൾ, n_Pre_വാക്കുകൾ, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    എങ്കിൽ (!kit.m_n_Vec_വാക്കുകൾ) തിരികെ kit;

    File_വരി* സ്ഥിരം flp = netlistp->മുകൾ_Scopep()->fileline();

    // Creates read/write reference
    സ്ഥിരം auto rd = [flp](AstVarScope* vp) { തിരികെ പുതിയ AstVarRef{flp, vp, VAccess::READ}; };
    സ്ഥിരം auto wr = [flp](AstVarScope* vp) { തിരികെ പുതിയ AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* സ്ഥിരം dumpTrgp
        = newArgument(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* സ്ഥിരം dumpTagp
        = newArgument(kit.m_dumpp, netlistp->findStringDType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, പുതിയ AstLogNot{flp, kit.newAnySetCall(dumpTrgp)}};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* സ്ഥിരം cstmtp = പുതിയ AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    സ്ഥിരം auto addDebug = [&](uint32_t index, സ്ഥിരം string& text) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ wrdIndex = static_cast<പൂർണ്ണസംഖ്യ>(index / WORD_SIZE);
        സ്ഥിരം പൂർണ്ണസംഖ്യ bitIndex = static_cast<പൂർണ്ണസംഖ്യ>(index % WORD_SIZE);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം aselp = പുതിയ AstArraySel{flp, rd(dumpTrgp), wrdIndex};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = പുതിയ AstSel{flp, aselp, bitIndex, 1};
        AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, condp};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* സ്ഥിരം cstmtp = പുതിയ AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"' region trigger index " + std::to_string(index) + " is active: " + text
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_കണ്ണി_Stmt* initialTrigsp = nullptr;
    വേണ്ടി (size_t i = 0; i < senItemps.size(); ++i) {
        സ്ഥിരം AstSenItem* സ്ഥിരം senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        എങ്കിൽ (!senItemp) {
            trigps.emplace_back(പുതിയ AstConst{flp, AstConst::BitFalse{}});
            തുടരുക;
        }

        // Create the trigger computation expression
        സ്ഥിരം auto& pair = sen_വ്യഞ്ജകം_Builder.build(senItemp);
        trigps.emplace_back(pair.first);

        // Add initialization time trigger
        എങ്കിൽ (pair.second || v3Global.opt.xInitialEdge()) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ wrdIndex = static_cast<പൂർണ്ണസംഖ്യ>(i / WORD_SIZE);
            സ്ഥിരം പൂർണ്ണസംഖ്യ bitIndex = static_cast<പൂർണ്ണസംഖ്യ>(i % WORD_SIZE);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം wordp = പുതിയ AstArraySel{flp, wr(kit.m_vscp), wrdIndex};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = പുതിയ AstSel{flp, wordp, bitIndex, 1};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = പുതിയ AstConst{flp, AstConst::BitTrue{}};
            എങ്കിൽ (useAcc) {
                initFuncp->addStmtsp(പുതിയ AstAssign{flp, lhsp, rhsp});
            } അല്ലെങ്കിൽ {
                initialTrigsp = Ast_കണ്ണി::addNext(initialTrigsp, പുതിയ AstAssign{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_വൃക്ഷം(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        addDebug(i, desc);
    }
    UASSERT(trigps.size() == nSenseTriggers, "Inconsistent number of trigger expressions");

    AstAssign* സ്ഥിരം trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    വേണ്ടി (unsigned i = 0; i < extraTriggers.size(); ++i) {
        addDebug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    സ്ഥിരം Sen_വ്യഞ്ജകം_Builder::Results senResults = sen_വ്യഞ്ജകം_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    വേണ്ടി (Ast_കണ്ണി_Stmt* സ്ഥിരം nodep : senResults.m_inits) initFuncp->addStmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_മുകൾ_Scope* സ്ഥിരം മുകൾ_Scopep = netlistp->മുകൾ_Scopep();
    {
        AstCFunc* സ്ഥിരം fp = kit.m_compVecp;
        // Trigger computation
        വേണ്ടി (Ast_കണ്ണി_Stmt* സ്ഥിരം nodep : senResults.m_preUpdates) fp->addStmtsp(nodep);
        fp->addStmtsp(trigStmtsp);
        വേണ്ടി (Ast_കണ്ണി_Stmt* സ്ഥിരം nodep : senResults.m_postUpdates) fp->addStmtsp(nodep);
        // Add the initialization time triggers
        എങ്കിൽ (initialTrigsp) {
            AstVarScope* സ്ഥിരം initVscp = മുകൾ_Scopep->createTemp("__V" + name + "DidInit", 1);
            AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, പുതിയ AstNot{flp, rd(initVscp)}};
            fp->addStmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->addThensp(util::setVar(initVscp, 1));
            ifp->addThensp(initialTrigsp);
        }
        util::splitCheck(fp);
    };
    // If there are 'pre' triggers, compute them
    എങ്കിൽ (kit.m_n_Pre_വാക്കുകൾ) {
        AstCFunc* സ്ഥിരം fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* സ്ഥിരം latchedp
            = newArgument(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* സ്ഥിരം nVscp = മുകൾ_Scopep->createTemp("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(സത്യം);
        // Add a loop to compute the pre words
        AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
        fp->addStmtsp(util::setVar(nVscp, 0));
        fp->addStmtsp(loopp);
        // Loop body
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം offsetp = പുതിയ AstConst{flp, kit.m_n_Vec_വാക്കുകൾ};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lIdxp = പുതിയ AstAdd{flp, rd(nVscp), offsetp};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = പുതിയ AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം aWordp = പുതിയ AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bWordp = പുതിയ AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = പുതിയ AstAnd{flp, aWordp, പുതിയ AstNot{flp, bWordp}};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം limp = പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, 32, n_Pre_വാക്കുകൾ};
        loopp->addStmtsp(പുതിയ AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(nVscp));
        loopp->addStmtsp(പുതിയ AstLoopTest{flp, loopp, പുതിയ AstLt{flp, rd(nVscp), limp}});
        util::splitCheck(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    എങ്കിൽ (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlinkFrBackWithNext()->delete_വൃക്ഷം();
    // Done with the trigger dump function, split as might be large
    util::splitCheck(kit.m_dumpp);

    തിരികെ kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
വർഗ്ഗം AwaitBeforeTrigVisitor final : പൊതു VNVisitor {
    സ്ഥിരം VNUser1InUse m_user1InUse;

    /**
     * AstCAwait::user1()       ->  bool.           True if node has been visited
     * AstSenTree::user1p()     ->  AstCFunc*.      Function that has to be called before awaiting
     *                                              for CAwait pointing to this SenTree
     * AstCFunc::user1p()       ->  AstVarScope*    Function's local temporary extended trigger
     *                                              vector variable scope
     */

    // Netlist - needed for using util::makeSubFunction()
    AstNetlist* സ്ഥിരം m_netlistp;
    // Trigger kit - for accessing trigger vectors and mapping senItems to thier indexes
    സ്ഥിരം TriggerKit& m_trigKit;
    // Expression builder - for building expressions from SenItems
    Sen_വ്യഞ്ജകം_Builder& m_sen_വ്യഞ്ജകം_Builder;
    // Generator of unique names for before-trigger function
    V3UniqueNames m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::vector<std::pair<AstCFunc*, Ast_Sen_വൃക്ഷം*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::vector<std::pair<Ast_Sen_വൃക്ഷം*, Ast_കണ്ണി_വ്യഞ്ജകം*>> m_sen_വൃക്ഷം_To_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_കണ്ണി>, std::vector<AstSenItem*>> m_sen_വ്യഞ്ജകം_To_Sen_Item;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    സ്ഥിതം Ast_കണ്ണി* get_Sen_Hash_കണ്ണി(സ്ഥിരം AstSenItem* സ്ഥിരം nodep) {
        എങ്കിൽ (AstVarRef* സ്ഥിരം varRefp = VN_CAST(nodep->sensp(), VarRef)) തിരികെ varRefp;
        തിരികെ nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    ശൂന്യം fill_Sen_വ്യഞ്ജകം_To_Sen_Item() {
        വേണ്ടി (auto sen_വൃക്ഷം_Sched : m_sen_വൃക്ഷം_To_Sched) {
            Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep = sen_വൃക്ഷം_Sched.first;

            വേണ്ടി (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                സ്ഥിരം VEdgeType edge = senItemp->edgeType();
                എങ്കിൽ (edge.anEdge() || edge == VEdgeType::ET_CHANGED
                    || edge == VEdgeType::ET_HYBRID) {
                    m_sen_വ്യഞ്ജകം_To_Sen_Item[*get_Sen_Hash_കണ്ണി(senItemp)].തള്ളുക_back(senItemp);
                }
            }
        }

        std::vector<VNRef<Ast_കണ്ണി>> toRemove;
        വേണ്ടി (സ്ഥിരം auto& sen_വ്യഞ്ജകം_To_Sen_വൃക്ഷം : m_sen_വ്യഞ്ജകം_To_Sen_Item) {
            std::vector<AstSenItem*> senItemps = sen_വ്യഞ്ജകം_To_Sen_വൃക്ഷം.second;
            toRemove.തള്ളുക_back(sen_വ്യഞ്ജകം_To_Sen_വൃക്ഷം.first);
            വേണ്ടി (size_t i = 1; i < senItemps.size(); ++i) {
                എങ്കിൽ (senItemps[i]->edgeType() != senItemps[i - 1]->edgeType()) {
                    toRemove.എടുക്കുക_back();
                    നിർത്തുക;
                }
            }
        }
        വേണ്ടി (സ്ഥിരം VNRef<Ast_കണ്ണി> it : toRemove) m_sen_വ്യഞ്ജകം_To_Sen_Item.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<size_t, std::map<size_t, std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>>>
    getUsedTriggersToTrees(സ്ഥിരം std::set<size_t>& usedTriggers) {
        std::map<size_t, std::map<size_t, std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>>> usedTrigsToUsingTrees;
        വേണ്ടി (auto sen_വൃക്ഷം_Sched : m_sen_വൃക്ഷം_To_Sched) {
            സ്ഥിരം Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep = sen_വൃക്ഷം_Sched.first;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം shedp = sen_വൃക്ഷം_Sched.second;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            വേണ്ടി (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                സ്ഥിരം size_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                എങ്കിൽ (usedTriggers.find(idx) != usedTriggers.end()) {
                    usedTrigsToUsingTrees[idx / TriggerKit::WORD_SIZE]
                                         [size_t{1} << (idx % TriggerKit::WORD_SIZE)]
                                             .തള്ളുക_back(shedp);
                }
            }
        }
        എങ്കിൽ (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            വേണ്ടി (സ്ഥിരം auto& triggersToTrees : usedTrigsToUsingTrees) {
                വേണ്ടി (സ്ഥിരം auto& bitsToTrees : triggersToTrees.second) {
                    സ്ഥിരം std::set<സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം*> exprps{bitsToTrees.second.begin(),
                                                              bitsToTrees.second.end()};
                    UASSERT(bitsToTrees.second.size() == exprps.size(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        തിരികെ usedTrigsToUsingTrees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep) {
        File_വരി* സ്ഥിരം flp = senTreep->fileline();
        എങ്കിൽ (!senTreep->user1p()) {
            AstCFunc* സ്ഥിരം funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), അസത്യം);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* സ്ഥിരം tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(സത്യം);

            AstVar* സ്ഥിരം argp = പുതിയ AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->findBasicDType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(സത്യം);
            argp->direction(VDirection::നിവേശം);
            funcp->addArgsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_back(funcp, senTreep);
        }
        AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtypeSetVoid();
        തിരികെ callp;
    }

    ശൂന്യം visit(AstCAwait* സ്ഥിരം nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        എങ്കിൽ (സ്ഥിരം AstCMethodHard* സ്ഥിരം cMethodHardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            എങ്കിൽ (cMethodHardp->method() == VCMethod::SCHED_TRIGGER) {
                AstCCall* സ്ഥിരം beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_കണ്ണി* സ്ഥിരം pinp = cMethodHardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, cMethodHardp, "No event description");
                beforeTrigp->addArgsp(VN_AS(pinp, കണ്ണി_വ്യഞ്ജകം)->clone_വൃക്ഷം(അസത്യം));

                // Call the before-trigger function before the CAwait
                nodep->addHereThisAsNext(beforeTrigp->makeStmt());
                m_sen_വൃക്ഷം_To_Sched.emplace_back(nodep->sentreep(), cMethodHardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* സ്ഥിരം nodep) override { iterateChildren(nodep); }

പൊതു:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_വ്യഞ്ജകം_Builder& sen_വ്യഞ്ജകം_Builder,
                           സ്ഥിരം TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_വ്യഞ്ജകം_Builder{sen_വ്യഞ്ജകം_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_വ്യഞ്ജകം_To_Sen_Item();

        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> trigps;
        std::set<size_t> usedTriggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        വേണ്ടി (സ്ഥിരം auto& funcToUsedTriggers : m_generatedFuncs) {
            AstCFunc* സ്ഥിരം funcp = funcToUsedTriggers.first;
            AstVarScope* സ്ഥിരം vscp = VN_AS(funcp->user1p(), VarScope);
            File_വരി* സ്ഥിരം flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep = funcToUsedTriggers.second;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                സ്ഥിരം auto emplaceAt
                    = [flp, &trigps, &usedTriggers](Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp, സ്ഥിരം size_t pos) {
                          സ്ഥിരം size_t targetSize
                              = vlstd::roundUpToMultipleOf<TriggerKit::WORD_SIZE>(pos + 1);
                          എങ്കിൽ (trigps.capacity() < targetSize) trigps.reserve(targetSize * 2);
                          ആകുമ്പോൾ (trigps.size() < targetSize) {
                              trigps.തള്ളുക_back(പുതിയ AstConst{flp, AstConst::BitFalse{}});
                          }
                          trigps[pos]->delete_വൃക്ഷം();
                          trigps[pos] = exprp;
                          usedTriggers.തിരുകുക(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                വേണ്ടി (സ്ഥിരം AstSenItem* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), SenItem)) {
                    സ്ഥിരം size_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_വ്യഞ്ജകം_Builder.build(itemp).first, idx);
                    auto iter = m_sen_വ്യഞ്ജകം_To_Sen_Item.find(*get_Sen_Hash_കണ്ണി(itemp));
                    എങ്കിൽ (iter != m_sen_വ്യഞ്ജകം_To_Sen_Item.end()) {
                        വേണ്ടി (AstSenItem* സ്ഥിരം additionalItemp : iter->second) {
                            സ്ഥിരം size_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_വ്യഞ്ജകം_Builder.build(additionalItemp).first, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                സ്ഥിരം Sen_വ്യഞ്ജകം_Builder::Results results
                    = m_sen_വ്യഞ്ജകം_Builder.getResultsAndClearUpdates();
                വേണ്ടി (Ast_കണ്ണി_Stmt* സ്ഥിരം stmtsp : results.m_inits) funcp->addStmtsp(stmtsp);
                വേണ്ടി (Ast_കണ്ണി_Stmt* സ്ഥിരം stmtsp : results.m_preUpdates) funcp->addStmtsp(stmtsp);
                funcp->addStmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                വേണ്ടി (Ast_കണ്ണി_Stmt* സ്ഥിരം stmtsp : results.m_postUpdates) funcp->addStmtsp(stmtsp);
            }

            സ്ഥിരം std::map<size_t, std::map<size_t, std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>>>
                usedTrigsToUsingTrees = getUsedTriggersToTrees(usedTriggers);
            usedTriggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            സ്ഥിരം auto getIdx = [flp](AstVarScope* സ്ഥിരം scocep, VAccess access, size_t idx) {
                തിരികെ പുതിയ AstArraySel{flp, പുതിയ AstVarRef{flp, scocep, access},
                                       പുതിയ AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* സ്ഥിരം argpVscp = പുതിയ AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->addVarsp(argpVscp);

            // Mark as ready triggered schedulers
            വേണ്ടി (സ്ഥിരം auto& triggersToTrees : usedTrigsToUsingTrees) {
                സ്ഥിരം size_t word = triggersToTrees.first;

                വേണ്ടി (സ്ഥിരം auto& bitsToTrees : triggersToTrees.second) {
                    സ്ഥിരം size_t bit = bitsToTrees.first;
                    സ്ഥിരം auto& schedulers = bitsToTrees.second;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* സ്ഥിരം maskConstp = പുതിയ AstConst{flp, AstConst::Unsized64{}, bit};
                    AstAnd* സ്ഥിരം condp
                        = പുതിയ AstAnd{flp, getIdx(vscp, VAccess::READ, word), maskConstp};
                    AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം schedp : schedulers) {
                        AstCMethodHard* സ്ഥിരം callp = പുതിയ AstCMethodHard{
                            flp, schedp->clone_വൃക്ഷം(അസത്യം), VCMethod::SCHED_READY};
                        callp->dtypeSetVoid();
                        callp->addPinsp(പുതിയ AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->addThensp(callp->makeStmt());
                    }
                    funcp->addStmtsp(ifp);
                }
            }

            AstVarScope* സ്ഥിരം vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            വേണ്ടി (സ്ഥിരം auto& triggersToTrees : usedTrigsToUsingTrees) {
                സ്ഥിരം size_t word = triggersToTrees.first;
                funcp->addStmtsp(പുതിയ AstAssign{flp, getIdx(vscAccp, VAccess::WRITE, word),
                                               പുതിയ AstOr{flp, getIdx(vscAccp, VAccess::READ, word),
                                                         getIdx(vscp, VAccess::READ, word)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = സ്ഥിരസ്ഥിതി;
};

ശൂന്യം beforeTrigVisitor(AstNetlist* netlistp, Sen_വ്യഞ്ജകം_Builder& sen_വ്യഞ്ജകം_Builder,
                       സ്ഥിരം TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_വ്യഞ്ജകം_Builder, trigKit};
}

}  // namespace V3Sched
