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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Order.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sched.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SenExprBuilder.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ V3Sched {

ਨਾਮਾਕਾਸ਼ {

ਵਰਤੋਂ util::new_ਦਲੀਲ;

AstVarScope* newLocal(AstCFunc* funcp, Ast_ਗੰਢ_DType* dtypep, ਅਚਲ std::string& name) {
    File_ਸਤਰ* ਅਚਲ flp = funcp->fileline();
    AstScope* ਅਚਲ scopep = funcp->scopep();
    AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(ਸੱਚ);
    varp->noReset(ਸੱਚ);
    funcp->ਜੋੜੋ_Varsp(varp);
    AstVarScope* ਅਚਲ vscp = ਨਵਾਂ AstVarScope{flp, scopep, varp};
    scopep->ਜੋੜੋ_Varsp(vscp);
    ਵਾਪਸ vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() ਅਚਲ {
    UASSERT(m_n_Pre_ਸ਼ਬਦ, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ਅਚਲ netlistp = v3Global.rootp();
    File_ਸਤਰ* ਅਚਲ flp = netlistp->ਸਿਖਰ_Scopep()->fileline();
    Ast_ਗੰਢ_DType* ਅਚਲ u32DTypep = netlistp->ਲੱਭੋ_UInt32_DType();
    Ast_ਗੰਢ_DType* ਅਚਲ strDtypep = netlistp->ਲੱਭੋ_String_DType();

    // Dumping function always slow
    ਅਚਲ std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* ਅਚਲ funcp = util::makeSubFunction(netlistp, name, ਸੱਚ);
    funcp->isStatic(ਸੱਚ);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* ਅਚਲ eVscp = new_ਦਲੀਲ(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* ਅਚਲ tVscp = new_ਦਲੀਲ(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    ਅਚਲ ਸਵੈਚਾਲਿਤ rd = [flp](AstVarScope* vp) { ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, vp, VAccess::READ}; };
    ਅਚਲ ਸਵੈਚਾਲਿਤ wr = [flp](AstVarScope* vp) { ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* ਅਚਲ vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* ਅਚਲ iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->ਜੋੜੋ_Stmtsp(util::ਸੈੱਟ_Var(iVscp, 0));
        // Add loop
        AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
        funcp->ਜੋੜੋ_Stmtsp(loopp);
        // Loop body
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = ਨਵਾਂ AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = ਨਵਾਂ AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ limp = ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, 32, m_n_Vec_ਸ਼ਬਦ};
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsp, rhsp});
        loopp->ਜੋੜੋ_Stmtsp(util::incrementVar(iVscp));
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{flp, loopp, ਨਵਾਂ AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, m_dumpp};
        callp->dtype_ਸੈੱਟ_Void();
        callp->ਜੋੜੋ_Argsp(rd(vVscp));
        callp->ਜੋੜੋ_Argsp(rd(tVscp));
        funcp->ਜੋੜੋ_Stmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* ਅਚਲ pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* ਅਚਲ jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->ਜੋੜੋ_Stmtsp(util::ਸੈੱਟ_Var(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
            funcp->ਜੋੜੋ_Stmtsp(loopp);
            // Loop body
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = ਨਵਾਂ AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = ਨਵਾਂ AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ limp = ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, 32, m_n_Pre_ਸ਼ਬਦ};
            loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsp, rhsp});
            loopp->ਜੋੜੋ_Stmtsp(util::incrementVar(jVscp));
            loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{flp, loopp, ਨਵਾਂ AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
            funcp->ਜੋੜੋ_Stmtsp(loopp);
            // Loop body
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = ਨਵਾਂ AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = ਨਵਾਂ AstConst{flp, AstConst::DTyped{}, m_ਸ਼ਬਦ_DTypep};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ limp = ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, 32, m_n_Vec_ਸ਼ਬਦ};
            loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsp, rhsp});
            loopp->ਜੋੜੋ_Stmtsp(util::incrementVar(jVscp));
            loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{flp, loopp, ਨਵਾਂ AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, m_dumpp};
        callp->dtype_ਸੈੱਟ_Void();
        callp->ਜੋੜੋ_Argsp(rd(pVscp));
        callp->ਜੋੜੋ_Argsp(
            ਨਵਾਂ AstConcatN{flp, rd(tVscp), ਨਵਾਂ AstConst{flp, AstConst::String{}, " pre"}});
        funcp->ਜੋੜੋ_Stmtsp(callp->makeStmt());
    }

    ਵਾਪਸ funcp;
}

AstCFunc* TriggerKit::create_Any_ਸੈੱਟ_Func(AstUnpackArrayDType* ਅਚਲ dtypep) ਅਚਲ {
    AstNetlist* ਅਚਲ netlistp = v3Global.rootp();
    File_ਸਤਰ* ਅਚਲ flp = netlistp->ਸਿਖਰ_Scopep()->fileline();
    Ast_ਗੰਢ_DType* ਅਚਲ u32DTypep = netlistp->ਲੱਭੋ_UInt32_DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* ਅਚਲ funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(ਸੱਚ);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* ਅਚਲ iVscp = new_ਦਲੀਲ(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ਅਚਲ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    ਅਚਲ ਸਵੈਚਾਲਿਤ rd = [flp](AstVarScope* vp) { ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
    funcp->ਜੋੜੋ_Stmtsp(util::ਸੈੱਟ_Var(nVscp, 0));
    funcp->ਜੋੜੋ_Stmtsp(loopp);
    funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCReturn{flp, ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}});

    // Loop body
    ਅਚਲ uint32_t n_ਸ਼ਬਦ = dtypep->elementsConst();
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = ਨਵਾਂ AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ਗੰਢ_Stmt* ਅਚਲ thenp = ਨਵਾਂ AstCReturn{flp, ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ limp = ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, 32, n_ਸ਼ਬਦ};
    loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{flp, condp, thenp});
    loopp->ਜੋੜੋ_Stmtsp(util::incrementVar(nVscp));
    loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{flp, loopp, ਨਵਾਂ AstLt{flp, rd(nVscp), limp}});

    //ਪੂਰਾ
    ਵਾਪਸ funcp;
}
AstCFunc* TriggerKit::createClearFunc() ਅਚਲ {
    AstNetlist* ਅਚਲ netlistp = v3Global.rootp();
    File_ਸਤਰ* ਅਚਲ flp = netlistp->ਸਿਖਰ_Scopep()->fileline();
    Ast_ਗੰਢ_DType* ਅਚਲ u32DTypep = netlistp->ਲੱਭੋ_UInt32_DType();

    // Create function
    AstCFunc* ਅਚਲ funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(ਸੱਚ);

    // Add arguments
    AstVarScope* ਅਚਲ oVscp = new_ਦਲੀਲ(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* ਅਚਲ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ਅਚਲ ਸਵੈਚਾਲਿਤ rd = [flp](AstVarScope* vp) { ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, vp, VAccess::READ}; };
    ਅਚਲ ਸਵੈਚਾਲਿਤ wr = [flp](AstVarScope* vp) { ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
    funcp->ਜੋੜੋ_Stmtsp(util::ਸੈੱਟ_Var(nVscp, 0));
    funcp->ਜੋੜੋ_Stmtsp(loopp);

    // Loop body
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = ਨਵਾਂ AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = ਨਵਾਂ AstConst{flp, AstConst::DTyped{}, m_ਸ਼ਬਦ_DTypep};
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ limp = ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, 32, m_n_Vec_ਸ਼ਬਦ};
    loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsp, rhsp});
    loopp->ਜੋੜੋ_Stmtsp(util::incrementVar(nVscp));
    loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{flp, loopp, ਨਵਾਂ AstLt{flp, rd(nVscp), limp}});

    //ਪੂਰਾ
    ਵਾਪਸ funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ਅਚਲ oDtypep,
                                       AstUnpackArrayDType* ਅਚਲ iDtypep) ਅਚਲ {
    AstNetlist* ਅਚਲ netlistp = v3Global.rootp();
    File_ਸਤਰ* ਅਚਲ flp = netlistp->ਸਿਖਰ_Scopep()->fileline();
    Ast_ਗੰਢ_DType* ਅਚਲ u32DTypep = netlistp->ਲੱਭੋ_UInt32_DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* ਅਚਲ funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(ਸੱਚ);

    // Add arguments
    AstVarScope* ਅਚਲ oVscp = new_ਦਲੀਲ(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* ਅਚਲ iVscp = new_ਦਲੀਲ(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ਅਚਲ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ਅਚਲ ਸਵੈਚਾਲਿਤ rd = [flp](AstVarScope* vp) { ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, vp, VAccess::READ}; };
    ਅਚਲ ਸਵੈਚਾਲਿਤ wr = [flp](AstVarScope* vp) { ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
    funcp->ਜੋੜੋ_Stmtsp(util::ਸੈੱਟ_Var(nVscp, 0));
    funcp->ਜੋੜੋ_Stmtsp(loopp);

    // Loop body
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = ਨਵਾਂ AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ oWordp = ਨਵਾਂ AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ iWordp = ਨਵਾਂ AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = ਨਵਾਂ AstOr{flp, oWordp, iWordp};
    AstConst* ਅਚਲ outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ਅਚਲ ਨਿਵੇਸ਼_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ limp = outputRangeLeftp->ਅੰਕ().ਮੰਜ਼ਿਲ_SInt() < ਨਿਵੇਸ਼_Range_Leftp->ਅੰਕ().ਮੰਜ਼ਿਲ_SInt()
                                  ? outputRangeLeftp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)
                                  : ਨਿਵੇਸ਼_Range_Leftp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
    loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsp, rhsp});
    loopp->ਜੋੜੋ_Stmtsp(util::incrementVar(nVscp));
    loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{flp, loopp, ਨਵਾਂ AstLte{flp, rd(nVscp), limp}});

    //ਪੂਰਾ
    ਵਾਪਸ funcp;
}

Ast_ਗੰਢ_ਵਿਅੰਜਕ* TriggerKit::new_Any_ਸੈੱਟ_Call(AstVarScope* ਅਚਲ vscp) ਅਚਲ {
    File_ਸਤਰ* ਅਚਲ flp = v3Global.rootp()->ਸਿਖਰ_Scopep()->fileline();
    ਜੇ (!m_n_Vec_ਸ਼ਬਦ) ਵਾਪਸ ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};

    AstCFunc* funcp = ਖਾਲੀ_ਸੂਚਕ;
    ਜੇ (vscp->dtypep() == m_trigVecDTypep) {
        ਜੇ (!m_any_ਸੈੱਟ_Vecp) m_any_ਸੈੱਟ_Vecp = create_Any_ਸੈੱਟ_Func(m_trigVecDTypep);
        funcp = m_any_ਸੈੱਟ_Vecp;
    } ਨਹੀਂਤਾਂ ਜੇ (vscp->dtypep() == m_trigExtDTypep) {
        ਜੇ (!m_any_ਸੈੱਟ_Extp) m_any_ਸੈੱਟ_Extp = create_Any_ਸੈੱਟ_Func(m_trigExtDTypep);
        funcp = m_any_ਸੈੱਟ_Extp;
    } ਨਹੀਂਤਾਂ {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, funcp};
    callp->ਜੋੜੋ_Argsp(ਨਵਾਂ AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    ਵਾਪਸ callp;
}
Ast_ਗੰਢ_Stmt* TriggerKit::newClearCall(AstVarScope* ਅਚਲ vscp) ਅਚਲ {
    ਜੇ (!m_n_Vec_ਸ਼ਬਦ) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    ਜੇ (!m_clearp) m_clearp = createClearFunc();
    File_ਸਤਰ* ਅਚਲ flp = v3Global.rootp()->ਸਿਖਰ_Scopep()->fileline();
    AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, m_clearp};
    callp->ਜੋੜੋ_Argsp(ਨਵਾਂ AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_ਸੈੱਟ_Void();
    ਵਾਪਸ callp->makeStmt();
}
Ast_ਗੰਢ_Stmt* TriggerKit::newOrIntoCall(AstVarScope* ਅਚਲ oVscp, AstVarScope* ਅਚਲ iVscp) ਅਚਲ {
    ਜੇ (!m_n_Vec_ਸ਼ਬਦ) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    ਅਚਲ ਮਾਪ_t ਨਕਾਬ
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[ਨਕਾਬ];
    ਜੇ (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_ਸਤਰ* ਅਚਲ flp = v3Global.rootp()->ਸਿਖਰ_Scopep()->fileline();
    AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, funcp};
    callp->ਜੋੜੋ_Argsp(ਨਵਾਂ AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->ਜੋੜੋ_Argsp(ਨਵਾਂ AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtype_ਸੈੱਟ_Void();
    ਵਾਪਸ callp->makeStmt();
}

Ast_ਗੰਢ_Stmt* TriggerKit::new_Comp_ਅਧਾਰ_Call() ਅਚਲ {
    ਜੇ (!m_n_Vec_ਸ਼ਬਦ) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    File_ਸਤਰ* ਅਚਲ flp = v3Global.rootp()->ਸਿਖਰ_Scopep()->fileline();
    AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, m_compVecp};
    callp->dtype_ਸੈੱਟ_Void();
    ਵਾਪਸ callp->makeStmt();
}

Ast_ਗੰਢ_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ਅਚਲ {
    ਜੇ (!m_n_Pre_ਸ਼ਬਦ) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    File_ਸਤਰ* ਅਚਲ flp = v3Global.rootp()->ਸਿਖਰ_Scopep()->fileline();
    AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, m_compExtp};
    callp->ਜੋੜੋ_Argsp(ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ});
    callp->dtype_ਸੈੱਟ_Void();
    ਵਾਪਸ callp->makeStmt();
}

Ast_ਗੰਢ_Stmt* TriggerKit::newDumpCall(AstVarScope* ਅਚਲ vscp, ਅਚਲ std::string& ਟੈਗ,
                                     ਤਾਰਕਿਕ debugOnly) ਅਚਲ {
    ਜੇ (!m_n_Vec_ਸ਼ਬਦ) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    AstCFunc* funcp = ਖਾਲੀ_ਸੂਚਕ;
    ਜੇ (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } ਨਹੀਂਤਾਂ ਜੇ (vscp->dtypep() == m_trigExtDTypep) {
        ਜੇ (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } ਨਹੀਂਤਾਂ {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_ਸਤਰ* ਅਚਲ flp = v3Global.rootp()->ਸਿਖਰ_Scopep()->fileline();
    AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, funcp};
    callp->ਜੋੜੋ_Argsp(ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ});
    callp->ਜੋੜੋ_Argsp(ਨਵਾਂ AstConst{flp, AstConst::String{}, ਟੈਗ});
    callp->dtype_ਸੈੱਟ_Void();
    AstCStmt* ਅਚਲ cstmtp = ਨਵਾਂ AstCStmt{flp};
    cstmtp->ਜੋੜੋ("#ifdef VL_DEBUG\n");
    ਜੇ (debugOnly) {
        cstmtp->ਜੋੜੋ("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->ਜੋੜੋ(callp->makeStmt());
        cstmtp->ਜੋੜੋ("}\n");
    } ਨਹੀਂਤਾਂ {
        cstmtp->ਜੋੜੋ(callp->makeStmt());
    }
    cstmtp->ਜੋੜੋ("#endif");
    ਵਾਪਸ cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(ਅਚਲ std::string& name) ਅਚਲ {
    ਜੇ (!m_n_Vec_ਸ਼ਬਦ) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    ਵਾਪਸ v3Global.rootp()->ਸਿਖਰ_Scopep()->create_ਆਰਜ਼ੀ("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extra_Trigger_ਸੂਚਕਾਂਕ(uint32_t ਸੂਚਕਾਂਕ) ਅਚਲ {
    UASSERT(ਸੂਚਕਾਂਕ <= m_n_Extra_ਸ਼ਬਦ * ਸ਼ਬਦ_ਮਾਪ, "Invalid external trigger index");
    ਵਾਪਸ ਸੂਚਕਾਂਕ + m_n_Sense_ਸ਼ਬਦ * ਸ਼ਬਦ_ਮਾਪ;
}

std::ਸੈੱਟ<uint32_t> TriggerKit::triggerIndices(ਅਚਲ Ast_Sen_ਰੁੱਖ* senTreep, ਤਾਰਕਿਕ pre) ਅਚਲ {
    std::ਸੈੱਟ<uint32_t> indices;
    ਲਈ (ਅਚਲ Ast_Sen_ਵਸਤੂ* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), Sen_ਵਸਤੂ)) {
        uint32_t ਸੂਚਕਾਂਕ = m_senItem2TrigIdx.at(*sip);
        ਜੇ (pre) {
            UASSERT_OBJ(ਸੂਚਕਾਂਕ < m_n_Pre_ਸ਼ਬਦ * ਸ਼ਬਦ_ਮਾਪ, sip, "Not a 'pre' trigger");
            ਸੂਚਕਾਂਕ += m_n_Vec_ਸ਼ਬਦ * ਸ਼ਬਦ_ਮਾਪ;
        }
        indices.ਪਾਓ(ਸੂਚਕਾਂਕ);
    }
    ਵਾਪਸ indices;
}

Ast_ਗੰਢ_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t ਸੂਚਕਾਂਕ) ਅਚਲ {
    ਸੂਚਕਾਂਕ = extra_Trigger_ਸੂਚਕਾਂਕ(ਸੂਚਕਾਂਕ);
    ਅਚਲ uint32_t ਸ਼ਬਦ_ਸੂਚਕਾਂਕ = ਸੂਚਕਾਂਕ / ਸ਼ਬਦ_ਮਾਪ;
    ਅਚਲ uint32_t ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੂਚਕਾਂਕ = ਸੂਚਕਾਂਕ % ਸ਼ਬਦ_ਮਾਪ;
    File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
    AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ wordp = ਨਵਾਂ AstArraySel{flp, refp, static_cast<ਪੂਰਨ>(ਸ਼ਬਦ_ਸੂਚਕਾਂਕ)};
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ trigLhsp = ਨਵਾਂ AstSel{flp, wordp, static_cast<ਪੂਰਨ>(ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੂਚਕਾਂਕ), 1};
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ trigRhsp = ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ};
    ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, trigLhsp, trigRhsp};
}

ਖਾਲੀ TriggerKit::ਜੋੜੋ_Extra_Trigger_Assignment(AstVarScope* vscp, uint32_t ਸੂਚਕਾਂਕ) ਅਚਲ {
    File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_ਗੰਢ* ਅਚਲ setp = newExtraTriggerAssignment(vscp, ਸੂਚਕਾਂਕ);
    setp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, vscp, VAccess::WRITE},
                                ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
ਖਾਲੀ TriggerKit::ਜੋੜੋ_ਮੁੱਲ_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t ਸੂਚਕਾਂਕ) ਅਚਲ {
    ਸੂਚਕਾਂਕ += m_n_Sense_ਸ਼ਬਦ * ਸ਼ਬਦ_ਮਾਪ;
    ਅਚਲ uint32_t ਸ਼ਬਦ_ਸੂਚਕਾਂਕ = ਸੂਚਕਾਂਕ / ਸ਼ਬਦ_ਮਾਪ;
    ਅਚਲ uint32_t ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੂਚਕਾਂਕ = ਸੂਚਕਾਂਕ % ਸ਼ਬਦ_ਮਾਪ;

    Ast_ਸਿਖਰ_Scope* ਅਚਲ ਸਿਖਰ_Scopep = netlistp->ਸਿਖਰ_Scopep();
    File_ਸਤਰ* ਅਚਲ flp = netlistp->fileline();
    Ast_ਗੰਢ_DType* ਅਚਲ dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ bdtypep = VN_CAST(dtypep, BasicDType)) {
        ਜੇ (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            ਵਾਪਸ;
        }
    } ਨਹੀਂਤਾਂ ਜੇ (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, ਗੰਢ_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        ਵਾਪਸ;
    }

    ਅਚਲ ਸਵੈਚਾਲਿਤ rdInst = [flp](AstVarScope* vp) { ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, vp, VAccess::READ}; };
    ਅਚਲ ਸਵੈਚਾਲਿਤ wrPrev = [flp](AstVarScope* vp) { ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, vp, VAccess::WRITE}; };
    ਅਚਲ ਸਵੈਚਾਲਿਤ rdPrev = [flp](AstVarScope* vp) { ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    ਅਚਲ std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* ਅਚਲ prevVscp = ਸਿਖਰ_Scopep->create_ਆਰਜ਼ੀ(prevName, instVscp->dtypep());

    // Initialize prev = inst
    ਜੇ (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ਅਚਲ cmhp = ਨਵਾਂ AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ਸੌਂਪੋ, rdInst(instVscp)};
        cmhp->dtype_ਸੈੱਟ_Void();
        initFuncp->ਜੋੜੋ_Stmtsp(cmhp->makeStmt());
    } ਨਹੀਂਤਾਂ {
        initFuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* neqp;
    ਜੇ (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ਅਚਲ cmhp
            = ਨਵਾਂ AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        neqp = cmhp;
    } ਨਹੀਂਤਾਂ {
        neqp = ਨਵਾਂ AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_ਗੰਢ* updp;
    ਜੇ (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ਅਚਲ cmhp = ਨਵਾਂ AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ਸੌਂਪੋ, rdInst(instVscp)};
        cmhp->dtype_ਸੈੱਟ_Void();
        updp = cmhp->makeStmt();
    } ਨਹੀਂਤਾਂ {
        updp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ wordp = ਨਵਾਂ AstArraySel{flp, refp, static_cast<ਪੂਰਨ>(ਸ਼ਬਦ_ਸੂਚਕਾਂਕ)};
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ trigLhsp = ਨਵਾਂ AstSel{flp, wordp, static_cast<ਪੂਰਨ>(ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੂਚਕਾਂਕ), 1};
    Ast_ਗੰਢ* ਅਚਲ setp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->ਜੋੜੋ_ਅਗਲਾ(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ਅਚਲ std::string& name, ਤਾਰਕਿਕ slow, uint32_t n_Sense_ਸ਼ਬਦ,
                       uint32_t n_Extra_ਸ਼ਬਦ, uint32_t n_Pre_ਸ਼ਬਦ,
                       std::unordered_map<VNRef<ਅਚਲ Ast_Sen_ਵਸਤੂ>, ਮਾਪ_t> senItem2TrigIdx,
                       ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ Ast_Sen_ਵਸਤੂ*>& senItemps, ਤਾਰਕਿਕ useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_ਸ਼ਬਦ{n_Sense_ਸ਼ਬਦ}
    , m_n_Extra_ਸ਼ਬਦ{n_Extra_ਸ਼ਬਦ}
    , m_n_Pre_ਸ਼ਬਦ{n_Pre_ਸ਼ਬਦ}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trig_Idx2_Sen_ਵਸਤੂ{senItemps} {
    // If no triggers, we don't need to generate anything
    ਜੇ (!m_n_Vec_ਸ਼ਬਦ) ਵਾਪਸ;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trig_Idx2_Sen_ਵਸਤੂ.resize((m_n_Vec_ਸ਼ਬਦ + m_n_Pre_ਸ਼ਬਦ) * ਸ਼ਬਦ_ਮਾਪ);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_ਸ਼ਬਦ * ਸ਼ਬਦ_ਮਾਪ,
              m_trig_Idx2_Sen_ਵਸਤੂ.begin() + m_n_Vec_ਸ਼ਬਦ * ਸ਼ਬਦ_ਮਾਪ);
    AstNetlist* ਅਚਲ netlistp = v3Global.rootp();
    AstScope* ਅਚਲ scopep = netlistp->ਸਿਖਰ_Scopep()->scopep();
    File_ਸਤਰ* ਅਚਲ flp = scopep->fileline();
    // Data type of a single trigger word
    m_ਸ਼ਬਦ_DTypep = netlistp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(ਸ਼ਬਦ_ਮਾਪ, ਸ਼ਬਦ_ਮਾਪ, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ਅਚਲ rp = ਨਵਾਂ AstRange{flp, static_cast<ਪੂਰਨ>(m_n_Vec_ਸ਼ਬਦ - 1), 0};
    m_trigVecDTypep = ਨਵਾਂ AstUnpackArrayDType{flp, m_ਸ਼ਬਦ_DTypep, rp};
    netlistp->typeTablep()->ਜੋੜੋ_Typesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    ਜੇ (m_n_Pre_ਸ਼ਬਦ) {
        AstRange* ਅਚਲ ep = ਨਵਾਂ AstRange{flp, static_cast<ਪੂਰਨ>(m_n_Vec_ਸ਼ਬਦ + m_n_Pre_ਸ਼ਬਦ - 1), 0};
        m_trigExtDTypep = ਨਵਾਂ AstUnpackArrayDType{flp, m_ਸ਼ਬਦ_DTypep, ep};
        netlistp->typeTablep()->ਜੋੜੋ_Typesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } ਨਹੀਂਤਾਂ {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->ਸਿਖਰ_Scopep()->create_ਆਰਜ਼ੀ("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, ਸੱਚ);
    m_dumpp->isStatic(ਸੱਚ);
    m_dumpp->ifdef("VL_DEBUG");
    ਜੇ (useAcc) {
        m_vscAccp
            = netlistp->ਸਿਖਰ_Scopep()->create_ਆਰਜ਼ੀ("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
    }
}

Ast_ਸੌਂਪੋ* TriggerKit::createSenTrigVecAssignment(AstVarScope* ਅਚਲ ਨਿਸ਼ਾਨਾ,
                                                  std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>& trigps) {
    File_ਸਤਰ* ਅਚਲ flp = ਨਿਸ਼ਾਨਾ->fileline();
    Ast_ਸੌਂਪੋ* trigStmtsp = ਖਾਲੀ_ਸੂਚਕ;
    // Assign sense triggers vector one word at a time
    ਲਈ (ਮਾਪ_t i = 0; i < trigps.ਮਾਪ(); i += ਸ਼ਬਦ_ਮਾਪ) {
        // Concatenate all bits in this trigger word using a balanced
        ਲਈ (uint32_t level = 0; level < ਸ਼ਬਦ_ਮਾਪ_LOG2; ++level) {
            ਅਚਲ uint32_t stride = 1 << level;
            ਲਈ (uint32_t j = 0; j < ਸ਼ਬਦ_ਮਾਪ; j += 2 * stride) {
                trigps[i + j] = ਨਵਾਂ AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = ਖਾਲੀ_ਸੂਚਕ;
            }
        }

        // Set the whole word in the trigger vector
        ਅਚਲ ਪੂਰਨ ਸ਼ਬਦ_ਸੂਚਕਾਂਕ = static_cast<ਪੂਰਨ>(i / ਸ਼ਬਦ_ਮਾਪ);
        AstArraySel* ਅਚਲ aselp
            = ਨਵਾਂ AstArraySel{flp, ਨਵਾਂ AstVarRef{flp, ਨਿਸ਼ਾਨਾ, VAccess::WRITE}, ਸ਼ਬਦ_ਸੂਚਕਾਂਕ};
        trigStmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(trigStmtsp, ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, aselp, trigps[i]});
    }
    ਵਾਪਸ trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ਅਚਲ initFuncp,  //
                              Sen_ਵਿਅੰਜਕ_Builder& sen_ਵਿਅੰਜਕ_Builder,  //
                              ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ Ast_Sen_ਰੁੱਖ*>& preTreeps,  //
                              ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ Ast_Sen_ਰੁੱਖ*>& senTreeps,  //
                              ਅਚਲ string& name,  //
                              ਅਚਲ ExtraTriggers& extraTriggers,  //
                              ਤਾਰਕਿਕ slow,  //
                              ਤਾਰਕਿਕ useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::ਸਦਿਸ਼<ਅਚਲ Ast_Sen_ਵਸਤੂ*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<ਅਚਲ Ast_Sen_ਵਸਤੂ>, ਮਾਪ_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    ਲਈ (ਅਚਲ Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep : preTreeps) {
        ਲਈ (ਅਚਲ Ast_Sen_ਵਸਤੂ *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_ਵਸਤੂ);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ਅਚਲ ਸਵੈਚਾਲਿਤ pair = senItem2TrigIdx.emplace(*itemp, senItemps.ਮਾਪ());
            ਜੇ (pair.ਦੂਜਾ) senItemps.ਧੱਕੋ_ਪਿੱਠ(itemp);
        }
    }
    ਅਚਲ uint32_t n_Pre_Sen_ਚੀਜ਼ਾਂ = senItemps.ਮਾਪ();
    V3Stats::ਜੋੜੋ_Stat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_ਚੀਜ਼ਾਂ);
    // Number of pre triggers, rounded up to a full word.
    ਅਚਲ uint32_t nPreTriggers = vlstd::ਗੇੜਾ_ਉੱਪਰ_ਮੰਜ਼ਿਲ_ਗੁਣਜ_ਦਾ<ਸ਼ਬਦ_ਮਾਪ>(senItemps.ਮਾਪ());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ਅਚਲ uint32_t n_Pre_ਸ਼ਬਦ = nPreTriggers / ਸ਼ਬਦ_ਮਾਪ;

    // Process the rest of the trees
    ਲਈ (ਅਚਲ Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep : senTreeps) {
        ਲਈ (ਅਚਲ Ast_Sen_ਵਸਤੂ *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_ਵਸਤੂ);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ਅਚਲ ਸਵੈਚਾਲਿਤ pair = senItem2TrigIdx.emplace(*itemp, senItemps.ਮਾਪ());
            ਜੇ (pair.ਦੂਜਾ) senItemps.ਧੱਕੋ_ਪਿੱਠ(itemp);
        }
    }
    ਅਚਲ uint32_t n_Sen_ਚੀਜ਼ਾਂ = senItemps.ਮਾਪ() - nPreTriggers;
    V3Stats::ਜੋੜੋ_Stat("Scheduling, '" + name + "' sense triggers", n_Sen_ਚੀਜ਼ਾਂ + n_Pre_Sen_ਚੀਜ਼ਾਂ);
    // Number of sense triggers, rounded up to a full word
    ਅਚਲ uint32_t nSenseTriggers = vlstd::ਗੇੜਾ_ਉੱਪਰ_ਮੰਜ਼ਿਲ_ਗੁਣਜ_ਦਾ<ਸ਼ਬਦ_ਮਾਪ>(senItemps.ਮਾਪ());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ਅਚਲ uint32_t n_Sense_ਸ਼ਬਦ = nSenseTriggers / ਸ਼ਬਦ_ਮਾਪ;

    // Allocate space for the extra triggers
    V3Stats::ਜੋੜੋ_Stat("Scheduling, '" + name + "' extra triggers", extraTriggers.ਮਾਪ());
    // Number of extra triggers, rounded up to a full word.
    ਅਚਲ uint32_t nExtraTriggers = vlstd::ਗੇੜਾ_ਉੱਪਰ_ਮੰਜ਼ਿਲ_ਗੁਣਜ_ਦਾ<ਸ਼ਬਦ_ਮਾਪ>(extraTriggers.ਮਾਪ());
    ਅਚਲ uint32_t n_Extra_ਸ਼ਬਦ = nExtraTriggers / ਸ਼ਬਦ_ਮਾਪ;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_ਸ਼ਬਦ, n_Extra_ਸ਼ਬਦ, n_Pre_ਸ਼ਬਦ, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    ਜੇ (!kit.m_n_Vec_ਸ਼ਬਦ) ਵਾਪਸ kit;

    File_ਸਤਰ* ਅਚਲ flp = netlistp->ਸਿਖਰ_Scopep()->fileline();

    // Creates read/write reference
    ਅਚਲ ਸਵੈਚਾਲਿਤ rd = [flp](AstVarScope* vp) { ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, vp, VAccess::READ}; };
    ਅਚਲ ਸਵੈਚਾਲਿਤ wr = [flp](AstVarScope* vp) { ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* ਅਚਲ dumpTrgp
        = new_ਦਲੀਲ(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* ਅਚਲ dumpTagp
        = new_ਦਲੀਲ(kit.m_dumpp, netlistp->ਲੱਭੋ_String_DType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstLogNot{flp, kit.new_Any_ਸੈੱਟ_Call(dumpTrgp)}};
        kit.m_dumpp->ਜੋੜੋ_Stmtsp(ifp);
        AstCStmt* ਅਚਲ cstmtp = ਨਵਾਂ AstCStmt{flp};
        ifp->ਜੋੜੋ_Thensp(cstmtp);
        cstmtp->ਜੋੜੋ("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->ਜੋੜੋ(rd(dumpTagp));
        cstmtp->ਜੋੜੋ(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਜੋੜੋ_Debug = [&](uint32_t ਸੂਚਕਾਂਕ, ਅਚਲ string& ਪਾਠ) {
        ਅਚਲ ਪੂਰਨ wrd_ਸੂਚਕਾਂਕ = static_cast<ਪੂਰਨ>(ਸੂਚਕਾਂਕ / ਸ਼ਬਦ_ਮਾਪ);
        ਅਚਲ ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੂਚਕਾਂਕ = static_cast<ਪੂਰਨ>(ਸੂਚਕਾਂਕ % ਸ਼ਬਦ_ਮਾਪ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ aselp = ਨਵਾਂ AstArraySel{flp, rd(dumpTrgp), wrd_ਸੂਚਕਾਂਕ};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = ਨਵਾਂ AstSel{flp, aselp, ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੂਚਕਾਂਕ, 1};
        AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, condp};
        kit.m_dumpp->ਜੋੜੋ_Stmtsp(ifp);
        AstCStmt* ਅਚਲ cstmtp = ਨਵਾਂ AstCStmt{flp};
        ifp->ਜੋੜੋ_Thensp(cstmtp);
        cstmtp->ਜੋੜੋ("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->ਜੋੜੋ(rd(dumpTagp));
        cstmtp->ਜੋੜੋ(" + \"' region trigger index " + std::ਮੰਜ਼ਿਲ_string(ਸੂਚਕਾਂਕ) + " is active: " + ਪਾਠ
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_ਗੰਢ_Stmt* initialTrigsp = ਖਾਲੀ_ਸੂਚਕ;
    ਲਈ (ਮਾਪ_t i = 0; i < senItemps.ਮਾਪ(); ++i) {
        ਅਚਲ Ast_Sen_ਵਸਤੂ* ਅਚਲ senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        ਜੇ (!senItemp) {
            trigps.emplace_ਪਿੱਠ(ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
            ਜਾਰੀ;
        }

        // Create the trigger computation expression
        ਅਚਲ ਸਵੈਚਾਲਿਤ& pair = sen_ਵਿਅੰਜਕ_Builder.build(senItemp);
        trigps.emplace_ਪਿੱਠ(pair.ਪਹਿਲਾ);

        // Add initialization time trigger
        ਜੇ (pair.ਦੂਜਾ || v3Global.opt.x_Initial_ਕਿਨਾਰਾ()) {
            ਅਚਲ ਪੂਰਨ wrd_ਸੂਚਕਾਂਕ = static_cast<ਪੂਰਨ>(i / ਸ਼ਬਦ_ਮਾਪ);
            ਅਚਲ ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੂਚਕਾਂਕ = static_cast<ਪੂਰਨ>(i % ਸ਼ਬਦ_ਮਾਪ);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ wordp = ਨਵਾਂ AstArraySel{flp, wr(kit.m_vscp), wrd_ਸੂਚਕਾਂਕ};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = ਨਵਾਂ AstSel{flp, wordp, ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੂਚਕਾਂਕ, 1};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
            ਜੇ (useAcc) {
                initFuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsp, rhsp});
            } ਨਹੀਂਤਾਂ {
                initialTrigsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(initialTrigsp, ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_ਰੁੱਖ(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        ਜੋੜੋ_Debug(i, desc);
    }
    UASSERT(trigps.ਮਾਪ() == nSenseTriggers, "Inconsistent number of trigger expressions");

    Ast_ਸੌਂਪੋ* ਅਚਲ trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ i = 0; i < extraTriggers.ਮਾਪ(); ++i) {
        ਜੋੜੋ_Debug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    ਅਚਲ Sen_ਵਿਅੰਜਕ_Builder::Results senResults = sen_ਵਿਅੰਜਕ_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    ਲਈ (Ast_ਗੰਢ_Stmt* ਅਚਲ nodep : senResults.m_inits) initFuncp->ਜੋੜੋ_Stmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_ਸਿਖਰ_Scope* ਅਚਲ ਸਿਖਰ_Scopep = netlistp->ਸਿਖਰ_Scopep();
    {
        AstCFunc* ਅਚਲ fp = kit.m_compVecp;
        // Trigger computation
        ਲਈ (Ast_ਗੰਢ_Stmt* ਅਚਲ nodep : senResults.m_preUpdates) fp->ਜੋੜੋ_Stmtsp(nodep);
        fp->ਜੋੜੋ_Stmtsp(trigStmtsp);
        ਲਈ (Ast_ਗੰਢ_Stmt* ਅਚਲ nodep : senResults.m_postUpdates) fp->ਜੋੜੋ_Stmtsp(nodep);
        // Add the initialization time triggers
        ਜੇ (initialTrigsp) {
            AstVarScope* ਅਚਲ initVscp = ਸਿਖਰ_Scopep->create_ਆਰਜ਼ੀ("__V" + name + "DidInit", 1);
            AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstNot{flp, rd(initVscp)}};
            fp->ਜੋੜੋ_Stmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->ਜੋੜੋ_Thensp(util::ਸੈੱਟ_Var(initVscp, 1));
            ifp->ਜੋੜੋ_Thensp(initialTrigsp);
        }
        util::ਵੰਡੋ_Check(fp);
    };
    // If there are 'pre' triggers, compute them
    ਜੇ (kit.m_n_Pre_ਸ਼ਬਦ) {
        AstCFunc* ਅਚਲ fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ਅਚਲ latchedp
            = new_ਦਲੀਲ(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ਅਚਲ nVscp = ਸਿਖਰ_Scopep->create_ਆਰਜ਼ੀ("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(ਸੱਚ);
        // Add a loop to compute the pre words
        AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
        fp->ਜੋੜੋ_Stmtsp(util::ਸੈੱਟ_Var(nVscp, 0));
        fp->ਜੋੜੋ_Stmtsp(loopp);
        // Loop body
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ offsetp = ਨਵਾਂ AstConst{flp, kit.m_n_Vec_ਸ਼ਬਦ};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lIdxp = ਨਵਾਂ Ast_ਜੋੜੋ{flp, rd(nVscp), offsetp};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = ਨਵਾਂ AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ aWordp = ਨਵਾਂ AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bWordp = ਨਵਾਂ AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = ਨਵਾਂ AstAnd{flp, aWordp, ਨਵਾਂ AstNot{flp, bWordp}};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ limp = ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, 32, n_Pre_ਸ਼ਬਦ};
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsp, rhsp});
        loopp->ਜੋੜੋ_Stmtsp(util::incrementVar(nVscp));
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{flp, loopp, ਨਵਾਂ AstLt{flp, rd(nVscp), limp}});
        util::ਵੰਡੋ_Check(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    ਜੇ (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()->delete_ਰੁੱਖ();
    // Done with the trigger dump function, split as might be large
    util::ਵੰਡੋ_Check(kit.m_dumpp);

    ਵਾਪਸ kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
ਵਰਗ AwaitBeforeTrigVisitor final : ਜਨਤਕ VNVisitor {
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;

    /**
     * AstCAwait::user1()       ->  bool.           True if node has been visited
     * AstSenTree::user1p()     ->  AstCFunc*.      Function that has to be called before awaiting
     *                                              for CAwait pointing to this SenTree
     * AstCFunc::user1p()       ->  AstVarScope*    Function's local temporary extended trigger
     *                                              vector variable scope
     */

    // Netlist - needed for using util::makeSubFunction()
    AstNetlist* ਅਚਲ m_netlistp;
    // Trigger kit - for accessing trigger vectors and mapping senItems to thier indexes
    ਅਚਲ TriggerKit& m_trigKit;
    // Expression builder - for building expressions from SenItems
    Sen_ਵਿਅੰਜਕ_Builder& m_sen_ਵਿਅੰਜਕ_Builder;
    // Generator of unique names for before-trigger function
    V3_Unique_ਨਾਂ m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::ਸਦਿਸ਼<std::pair<AstCFunc*, Ast_Sen_ਰੁੱਖ*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::ਸਦਿਸ਼<std::pair<Ast_Sen_ਰੁੱਖ*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>> m_sen_ਰੁੱਖ_ਮੰਜ਼ਿਲ_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_ਗੰਢ>, std::ਸਦਿਸ਼<Ast_Sen_ਵਸਤੂ*>> m_sen_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_Sen_ਵਸਤੂ;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    ਸਥਿਰ Ast_ਗੰਢ* get_Sen_Hash_ਗੰਢ(ਅਚਲ Ast_Sen_ਵਸਤੂ* ਅਚਲ nodep) {
        ਜੇ (AstVarRef* ਅਚਲ varRefp = VN_CAST(nodep->sensp(), VarRef)) ਵਾਪਸ varRefp;
        ਵਾਪਸ nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    ਖਾਲੀ fill_Sen_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_Sen_ਵਸਤੂ() {
        ਲਈ (ਸਵੈਚਾਲਿਤ sen_ਰੁੱਖ_Sched : m_sen_ਰੁੱਖ_ਮੰਜ਼ਿਲ_Sched) {
            Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = sen_ਰੁੱਖ_Sched.ਪਹਿਲਾ;

            ਲਈ (Ast_Sen_ਵਸਤੂ* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_ਵਸਤੂ)) {
                ਅਚਲ VEdgeType ਕਿਨਾਰਾ = senItemp->ਕਿਨਾਰਾ_Type();
                ਜੇ (ਕਿਨਾਰਾ.an_ਕਿਨਾਰਾ() || ਕਿਨਾਰਾ == VEdgeType::ET_ਬਦਲਿਆ
                    || ਕਿਨਾਰਾ == VEdgeType::ET_HYBRID) {
                    m_sen_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_Sen_ਵਸਤੂ[*get_Sen_Hash_ਗੰਢ(senItemp)].ਧੱਕੋ_ਪਿੱਠ(senItemp);
                }
            }
        }

        std::ਸਦਿਸ਼<VNRef<Ast_ਗੰਢ>> ਮੰਜ਼ਿਲ_Remove;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& sen_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_Sen_ਰੁੱਖ : m_sen_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_Sen_ਵਸਤੂ) {
            std::ਸਦਿਸ਼<Ast_Sen_ਵਸਤੂ*> senItemps = sen_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_Sen_ਰੁੱਖ.ਦੂਜਾ;
            ਮੰਜ਼ਿਲ_Remove.ਧੱਕੋ_ਪਿੱਠ(sen_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_Sen_ਰੁੱਖ.ਪਹਿਲਾ);
            ਲਈ (ਮਾਪ_t i = 1; i < senItemps.ਮਾਪ(); ++i) {
                ਜੇ (senItemps[i]->ਕਿਨਾਰਾ_Type() != senItemps[i - 1]->ਕਿਨਾਰਾ_Type()) {
                    ਮੰਜ਼ਿਲ_Remove.ਕੱਢੋ_ਪਿੱਠ();
                    ਰੋਕੋ;
                }
            }
        }
        ਲਈ (ਅਚਲ VNRef<Ast_ਗੰਢ> it : ਮੰਜ਼ਿਲ_Remove) m_sen_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_Sen_ਵਸਤੂ.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<ਮਾਪ_t, std::map<ਮਾਪ_t, std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>>>
    get_ਵਰਤਿਆ_Triggers_ਮੰਜ਼ਿਲ_Trees(ਅਚਲ std::ਸੈੱਟ<ਮਾਪ_t>& ਵਰਤਿਆ_Triggers) {
        std::map<ਮਾਪ_t, std::map<ਮਾਪ_t, std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>>> ਵਰਤਿਆ_Trigs_ਮੰਜ਼ਿਲ_Using_Trees;
        ਲਈ (ਸਵੈਚਾਲਿਤ sen_ਰੁੱਖ_Sched : m_sen_ਰੁੱਖ_ਮੰਜ਼ਿਲ_Sched) {
            ਅਚਲ Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = sen_ਰੁੱਖ_Sched.ਪਹਿਲਾ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ shedp = sen_ਰੁੱਖ_Sched.ਦੂਜਾ;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            ਲਈ (Ast_Sen_ਵਸਤੂ* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_ਵਸਤੂ)) {
                ਅਚਲ ਮਾਪ_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                ਜੇ (ਵਰਤਿਆ_Triggers.ਲੱਭੋ(idx) != ਵਰਤਿਆ_Triggers.ਸਿਰਾ()) {
                    ਵਰਤਿਆ_Trigs_ਮੰਜ਼ਿਲ_Using_Trees[idx / TriggerKit::ਸ਼ਬਦ_ਮਾਪ]
                                         [ਮਾਪ_t{1} << (idx % TriggerKit::ਸ਼ਬਦ_ਮਾਪ)]
                                             .ਧੱਕੋ_ਪਿੱਠ(shedp);
                }
            }
        }
        ਜੇ (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& triggers_ਮੰਜ਼ਿਲ_Trees : ਵਰਤਿਆ_Trigs_ਮੰਜ਼ਿਲ_Using_Trees) {
                ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਬਿੱਟ_ਮੰਜ਼ਿਲ_Trees : triggers_ਮੰਜ਼ਿਲ_Trees.ਦੂਜਾ) {
                    ਅਚਲ std::ਸੈੱਟ<ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ*> exprps{ਬਿੱਟ_ਮੰਜ਼ਿਲ_Trees.ਦੂਜਾ.begin(),
                                                              ਬਿੱਟ_ਮੰਜ਼ਿਲ_Trees.ਦੂਜਾ.ਸਿਰਾ()};
                    UASSERT(ਬਿੱਟ_ਮੰਜ਼ਿਲ_Trees.ਦੂਜਾ.ਮਾਪ() == exprps.ਮਾਪ(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        ਵਾਪਸ ਵਰਤਿਆ_Trigs_ਮੰਜ਼ਿਲ_Using_Trees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep) {
        File_ਸਤਰ* ਅਚਲ flp = senTreep->fileline();
        ਜੇ (!senTreep->user1p()) {
            AstCFunc* ਅਚਲ funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), ਝੂਠ);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* ਅਚਲ tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(ਸੱਚ);

            AstVar* ਅਚਲ argp = ਨਵਾਂ AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(ਸੱਚ);
            argp->direction(VDirection::ਨਿਵੇਸ਼);
            funcp->ਜੋੜੋ_Argsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_ਪਿੱਠ(funcp, senTreep);
        }
        AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtype_ਸੈੱਟ_Void();
        ਵਾਪਸ callp;
    }

    ਖਾਲੀ ਦੇਖੋ(AstCAwait* ਅਚਲ nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        ਜੇ (ਅਚਲ AstCMethodHard* ਅਚਲ c_ਵਿਧੀ_Hardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            ਜੇ (c_ਵਿਧੀ_Hardp->ਵਿਧੀ() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ਅਚਲ beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_ਗੰਢ* ਅਚਲ pinp = c_ਵਿਧੀ_Hardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, c_ਵਿਧੀ_Hardp, "No event description");
                beforeTrigp->ਜੋੜੋ_Argsp(VN_AS(pinp, ਗੰਢ_ਵਿਅੰਜਕ)->clone_ਰੁੱਖ(ਝੂਠ));

                // Call the before-trigger function before the CAwait
                nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(beforeTrigp->makeStmt());
                m_sen_ਰੁੱਖ_ਮੰਜ਼ਿਲ_Sched.emplace_ਪਿੱਠ(nodep->sentreep(), c_ਵਿਧੀ_Hardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* ਅਚਲ nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_ਵਿਅੰਜਕ_Builder& sen_ਵਿਅੰਜਕ_Builder,
                           ਅਚਲ TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_ਵਿਅੰਜਕ_Builder{sen_ਵਿਅੰਜਕ_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_Sen_ਵਸਤੂ();

        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> trigps;
        std::ਸੈੱਟ<ਮਾਪ_t> ਵਰਤਿਆ_Triggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& func_ਮੰਜ਼ਿਲ_ਵਰਤਿਆ_Triggers : m_generatedFuncs) {
            AstCFunc* ਅਚਲ funcp = func_ਮੰਜ਼ਿਲ_ਵਰਤਿਆ_Triggers.ਪਹਿਲਾ;
            AstVarScope* ਅਚਲ vscp = VN_AS(funcp->user1p(), VarScope);
            File_ਸਤਰ* ਅਚਲ flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = func_ਮੰਜ਼ਿਲ_ਵਰਤਿਆ_Triggers.ਦੂਜਾ;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ਅਚਲ ਸਵੈਚਾਲਿਤ emplaceAt
                    = [flp, &trigps, &ਵਰਤਿਆ_Triggers](Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp, ਅਚਲ ਮਾਪ_t pos) {
                          ਅਚਲ ਮਾਪ_t ਨਿਸ਼ਾਨਾ_ਮਾਪ
                              = vlstd::ਗੇੜਾ_ਉੱਪਰ_ਮੰਜ਼ਿਲ_ਗੁਣਜ_ਦਾ<TriggerKit::ਸ਼ਬਦ_ਮਾਪ>(pos + 1);
                          ਜੇ (trigps.ਸਮਰੱਥਾ() < ਨਿਸ਼ਾਨਾ_ਮਾਪ) trigps.reserve(ਨਿਸ਼ਾਨਾ_ਮਾਪ * 2);
                          ਜਦੋਂ (trigps.ਮਾਪ() < ਨਿਸ਼ਾਨਾ_ਮਾਪ) {
                              trigps.ਧੱਕੋ_ਪਿੱਠ(ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
                          }
                          trigps[pos]->delete_ਰੁੱਖ();
                          trigps[pos] = exprp;
                          ਵਰਤਿਆ_Triggers.ਪਾਓ(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                ਲਈ (ਅਚਲ Ast_Sen_ਵਸਤੂ* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Sen_ਵਸਤੂ)) {
                    ਅਚਲ ਮਾਪ_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_ਵਿਅੰਜਕ_Builder.build(itemp).ਪਹਿਲਾ, idx);
                    ਸਵੈਚਾਲਿਤ iter = m_sen_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_Sen_ਵਸਤੂ.ਲੱਭੋ(*get_Sen_Hash_ਗੰਢ(itemp));
                    ਜੇ (iter != m_sen_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_Sen_ਵਸਤੂ.ਸਿਰਾ()) {
                        ਲਈ (Ast_Sen_ਵਸਤੂ* ਅਚਲ additionalItemp : iter->ਦੂਜਾ) {
                            ਅਚਲ ਮਾਪ_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_ਵਿਅੰਜਕ_Builder.build(additionalItemp).ਪਹਿਲਾ, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ਅਚਲ Sen_ਵਿਅੰਜਕ_Builder::Results results
                    = m_sen_ਵਿਅੰਜਕ_Builder.getResultsAndClearUpdates();
                ਲਈ (Ast_ਗੰਢ_Stmt* ਅਚਲ stmtsp : results.m_inits) funcp->ਜੋੜੋ_Stmtsp(stmtsp);
                ਲਈ (Ast_ਗੰਢ_Stmt* ਅਚਲ stmtsp : results.m_preUpdates) funcp->ਜੋੜੋ_Stmtsp(stmtsp);
                funcp->ਜੋੜੋ_Stmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                ਲਈ (Ast_ਗੰਢ_Stmt* ਅਚਲ stmtsp : results.m_postUpdates) funcp->ਜੋੜੋ_Stmtsp(stmtsp);
            }

            ਅਚਲ std::map<ਮਾਪ_t, std::map<ਮਾਪ_t, std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>>>
                ਵਰਤਿਆ_Trigs_ਮੰਜ਼ਿਲ_Using_Trees = get_ਵਰਤਿਆ_Triggers_ਮੰਜ਼ਿਲ_Trees(ਵਰਤਿਆ_Triggers);
            ਵਰਤਿਆ_Triggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            ਅਚਲ ਸਵੈਚਾਲਿਤ getIdx = [flp](AstVarScope* ਅਚਲ scocep, VAccess access, ਮਾਪ_t idx) {
                ਵਾਪਸ ਨਵਾਂ AstArraySel{flp, ਨਵਾਂ AstVarRef{flp, scocep, access},
                                       ਨਵਾਂ AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* ਅਚਲ argpVscp = ਨਵਾਂ AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->ਜੋੜੋ_Varsp(argpVscp);

            // Mark as ready triggered schedulers
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& triggers_ਮੰਜ਼ਿਲ_Trees : ਵਰਤਿਆ_Trigs_ਮੰਜ਼ਿਲ_Using_Trees) {
                ਅਚਲ ਮਾਪ_t ਸ਼ਬਦ = triggers_ਮੰਜ਼ਿਲ_Trees.ਪਹਿਲਾ;

                ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਬਿੱਟ_ਮੰਜ਼ਿਲ_Trees : triggers_ਮੰਜ਼ਿਲ_Trees.ਦੂਜਾ) {
                    ਅਚਲ ਮਾਪ_t ਦੋਆਧਾਰੀ_ਅੰਕ_2 = ਬਿੱਟ_ਮੰਜ਼ਿਲ_Trees.ਪਹਿਲਾ;
                    ਅਚਲ ਸਵੈਚਾਲਿਤ& schedulers = ਬਿੱਟ_ਮੰਜ਼ਿਲ_Trees.ਦੂਜਾ;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* ਅਚਲ ਨਕਾਬ_Constp = ਨਵਾਂ AstConst{flp, AstConst::Unsized64{}, ਦੋਆਧਾਰੀ_ਅੰਕ_2};
                    AstAnd* ਅਚਲ condp
                        = ਨਵਾਂ AstAnd{flp, getIdx(vscp, VAccess::READ, ਸ਼ਬਦ), ਨਕਾਬ_Constp};
                    AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ schedp : schedulers) {
                        AstCMethodHard* ਅਚਲ callp = ਨਵਾਂ AstCMethodHard{
                            flp, schedp->clone_ਰੁੱਖ(ਝੂਠ), VCMethod::SCHED_READY};
                        callp->dtype_ਸੈੱਟ_Void();
                        callp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->ਜੋੜੋ_Thensp(callp->makeStmt());
                    }
                    funcp->ਜੋੜੋ_Stmtsp(ifp);
                }
            }

            AstVarScope* ਅਚਲ vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& triggers_ਮੰਜ਼ਿਲ_Trees : ਵਰਤਿਆ_Trigs_ਮੰਜ਼ਿਲ_Using_Trees) {
                ਅਚਲ ਮਾਪ_t ਸ਼ਬਦ = triggers_ਮੰਜ਼ਿਲ_Trees.ਪਹਿਲਾ;
                funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, getIdx(vscAccp, VAccess::WRITE, ਸ਼ਬਦ),
                                               ਨਵਾਂ AstOr{flp, getIdx(vscAccp, VAccess::READ, ਸ਼ਬਦ),
                                                         getIdx(vscp, VAccess::READ, ਸ਼ਬਦ)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = ਮੂਲ;
};

ਖਾਲੀ beforeTrigVisitor(AstNetlist* netlistp, Sen_ਵਿਅੰਜਕ_Builder& sen_ਵਿਅੰਜਕ_Builder,
                       ਅਚਲ TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_ਵਿਅੰਜਕ_Builder, trigKit};
}

}  // namespace V3Sched
