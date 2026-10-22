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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3EmitCBase.h"
#అన్తర్భావ "V3EmitV.h"
#అన్తర్భావ "V3Order.h"
#అన్తర్భావ "V3Sched.h"
#అన్తర్భావ "V3SenExprBuilder.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ V3Sched {

నామాకాశ {

ఉపయోగించి util::new_వాదం;

AstVarScope* newLocal(AstCFunc* funcp, Ast_కణుపు_DType* dtypep, స్థిరరాశి std::string& name) {
    File_పంక్తి* స్థిరరాశి flp = funcp->fileline();
    AstScope* స్థిరరాశి scopep = funcp->scopep();
    AstVar* స్థిరరాశి varp = కొత్త AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(నిజం);
    varp->noReset(నిజం);
    funcp->కూడు_Varsp(varp);
    AstVarScope* స్థిరరాశి vscp = కొత్త AstVarScope{flp, scopep, varp};
    scopep->కూడు_Varsp(vscp);
    తిరిగి vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() స్థిరరాశి {
    UASSERT(m_n_Pre_పదాలు, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* స్థిరరాశి netlistp = v3Global.rootp();
    File_పంక్తి* స్థిరరాశి flp = netlistp->శిఖరం_Scopep()->fileline();
    Ast_కణుపు_DType* స్థిరరాశి u32DTypep = netlistp->కనుగొను_UInt32_DType();
    Ast_కణుపు_DType* స్థిరరాశి strDtypep = netlistp->కనుగొను_String_DType();

    // Dumping function always slow
    స్థిరరాశి std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* స్థిరరాశి funcp = util::makeSubFunction(netlistp, name, నిజం);
    funcp->isStatic(నిజం);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* స్థిరరాశి eVscp = new_వాదం(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* స్థిరరాశి tVscp = new_వాదం(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    స్థిరరాశి స్వయంచాలక rd = [flp](AstVarScope* vp) { తిరిగి కొత్త AstVarRef{flp, vp, VAccess::READ}; };
    స్థిరరాశి స్వయంచాలక wr = [flp](AstVarScope* vp) { తిరిగి కొత్త AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* స్థిరరాశి vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* స్థిరరాశి iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->కూడు_Stmtsp(util::సమితి_Var(iVscp, 0));
        // Add loop
        AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
        funcp->కూడు_Stmtsp(loopp);
        // Loop body
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = కొత్త AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = కొత్త AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_కణుపు_సమాసం* స్థిరరాశి limp = కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, 32, m_n_Vec_పదాలు};
        loopp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, lhsp, rhsp});
        loopp->కూడు_Stmtsp(util::incrementVar(iVscp));
        loopp->కూడు_Stmtsp(కొత్త AstLoopTest{flp, loopp, కొత్త AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, m_dumpp};
        callp->dtype_సమితి_Void();
        callp->కూడు_Argsp(rd(vVscp));
        callp->కూడు_Argsp(rd(tVscp));
        funcp->కూడు_Stmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* స్థిరరాశి pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* స్థిరరాశి jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->కూడు_Stmtsp(util::సమితి_Var(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
            funcp->కూడు_Stmtsp(loopp);
            // Loop body
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = కొత్త AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = కొత్త AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_కణుపు_సమాసం* స్థిరరాశి limp = కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, 32, m_n_Pre_పదాలు};
            loopp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, lhsp, rhsp});
            loopp->కూడు_Stmtsp(util::incrementVar(jVscp));
            loopp->కూడు_Stmtsp(కొత్త AstLoopTest{flp, loopp, కొత్త AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
            funcp->కూడు_Stmtsp(loopp);
            // Loop body
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = కొత్త AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = కొత్త AstConst{flp, AstConst::DTyped{}, m_మాట_DTypep};
            Ast_కణుపు_సమాసం* స్థిరరాశి limp = కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, 32, m_n_Vec_పదాలు};
            loopp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, lhsp, rhsp});
            loopp->కూడు_Stmtsp(util::incrementVar(jVscp));
            loopp->కూడు_Stmtsp(కొత్త AstLoopTest{flp, loopp, కొత్త AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, m_dumpp};
        callp->dtype_సమితి_Void();
        callp->కూడు_Argsp(rd(pVscp));
        callp->కూడు_Argsp(
            కొత్త AstConcatN{flp, rd(tVscp), కొత్త AstConst{flp, AstConst::String{}, " pre"}});
        funcp->కూడు_Stmtsp(callp->makeStmt());
    }

    తిరిగి funcp;
}

AstCFunc* TriggerKit::create_Any_సమితి_Func(AstUnpackArrayDType* స్థిరరాశి dtypep) స్థిరరాశి {
    AstNetlist* స్థిరరాశి netlistp = v3Global.rootp();
    File_పంక్తి* స్థిరరాశి flp = netlistp->శిఖరం_Scopep()->fileline();
    Ast_కణుపు_DType* స్థిరరాశి u32DTypep = netlistp->కనుగొను_UInt32_DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* స్థిరరాశి funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(నిజం);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* స్థిరరాశి iVscp = new_వాదం(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* స్థిరరాశి nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    స్థిరరాశి స్వయంచాలక rd = [flp](AstVarScope* vp) { తిరిగి కొత్త AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
    funcp->కూడు_Stmtsp(util::సమితి_Var(nVscp, 0));
    funcp->కూడు_Stmtsp(loopp);
    funcp->కూడు_Stmtsp(కొత్త AstCReturn{flp, కొత్త AstConst{flp, AstConst::బిట్టు_False{}}});

    // Loop body
    స్థిరరాశి uint32_t n_పదాలు = dtypep->elementsConst();
    Ast_కణుపు_సమాసం* స్థిరరాశి condp = కొత్త AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_కణుపు_Stmt* స్థిరరాశి thenp = కొత్త AstCReturn{flp, కొత్త AstConst{flp, AstConst::బిట్టు_True{}}};
    Ast_కణుపు_సమాసం* స్థిరరాశి limp = కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, 32, n_పదాలు};
    loopp->కూడు_Stmtsp(కొత్త AstIf{flp, condp, thenp});
    loopp->కూడు_Stmtsp(util::incrementVar(nVscp));
    loopp->కూడు_Stmtsp(కొత్త AstLoopTest{flp, loopp, కొత్త AstLt{flp, rd(nVscp), limp}});

    //పూర్తి
    తిరిగి funcp;
}
AstCFunc* TriggerKit::createClearFunc() స్థిరరాశి {
    AstNetlist* స్థిరరాశి netlistp = v3Global.rootp();
    File_పంక్తి* స్థిరరాశి flp = netlistp->శిఖరం_Scopep()->fileline();
    Ast_కణుపు_DType* స్థిరరాశి u32DTypep = netlistp->కనుగొను_UInt32_DType();

    // Create function
    AstCFunc* స్థిరరాశి funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(నిజం);

    // Add arguments
    AstVarScope* స్థిరరాశి oVscp = new_వాదం(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* స్థిరరాశి nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    స్థిరరాశి స్వయంచాలక rd = [flp](AstVarScope* vp) { తిరిగి కొత్త AstVarRef{flp, vp, VAccess::READ}; };
    స్థిరరాశి స్వయంచాలక wr = [flp](AstVarScope* vp) { తిరిగి కొత్త AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
    funcp->కూడు_Stmtsp(util::సమితి_Var(nVscp, 0));
    funcp->కూడు_Stmtsp(loopp);

    // Loop body
    Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = కొత్త AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = కొత్త AstConst{flp, AstConst::DTyped{}, m_మాట_DTypep};
    Ast_కణుపు_సమాసం* స్థిరరాశి limp = కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, 32, m_n_Vec_పదాలు};
    loopp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, lhsp, rhsp});
    loopp->కూడు_Stmtsp(util::incrementVar(nVscp));
    loopp->కూడు_Stmtsp(కొత్త AstLoopTest{flp, loopp, కొత్త AstLt{flp, rd(nVscp), limp}});

    //పూర్తి
    తిరిగి funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* స్థిరరాశి oDtypep,
                                       AstUnpackArrayDType* స్థిరరాశి iDtypep) స్థిరరాశి {
    AstNetlist* స్థిరరాశి netlistp = v3Global.rootp();
    File_పంక్తి* స్థిరరాశి flp = netlistp->శిఖరం_Scopep()->fileline();
    Ast_కణుపు_DType* స్థిరరాశి u32DTypep = netlistp->కనుగొను_UInt32_DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* స్థిరరాశి funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(నిజం);

    // Add arguments
    AstVarScope* స్థిరరాశి oVscp = new_వాదం(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* స్థిరరాశి iVscp = new_వాదం(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* స్థిరరాశి nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    స్థిరరాశి స్వయంచాలక rd = [flp](AstVarScope* vp) { తిరిగి కొత్త AstVarRef{flp, vp, VAccess::READ}; };
    స్థిరరాశి స్వయంచాలక wr = [flp](AstVarScope* vp) { తిరిగి కొత్త AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
    funcp->కూడు_Stmtsp(util::సమితి_Var(nVscp, 0));
    funcp->కూడు_Stmtsp(loopp);

    // Loop body
    Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = కొత్త AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_కణుపు_సమాసం* స్థిరరాశి oWordp = కొత్త AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_కణుపు_సమాసం* స్థిరరాశి iWordp = కొత్త AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = కొత్త AstOr{flp, oWordp, iWordp};
    AstConst* స్థిరరాశి outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* స్థిరరాశి ఆదానం_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_కణుపు_సమాసం* స్థిరరాశి limp = outputRangeLeftp->అంకె().గమ్యం_SInt() < ఆదానం_Range_Leftp->అంకె().గమ్యం_SInt()
                                  ? outputRangeLeftp->clone_వృక్షం_శుద్ధ(అబద్ధం)
                                  : ఆదానం_Range_Leftp->clone_వృక్షం_శుద్ధ(అబద్ధం);
    loopp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, lhsp, rhsp});
    loopp->కూడు_Stmtsp(util::incrementVar(nVscp));
    loopp->కూడు_Stmtsp(కొత్త AstLoopTest{flp, loopp, కొత్త AstLte{flp, rd(nVscp), limp}});

    //పూర్తి
    తిరిగి funcp;
}

Ast_కణుపు_సమాసం* TriggerKit::new_Any_సమితి_Call(AstVarScope* స్థిరరాశి vscp) స్థిరరాశి {
    File_పంక్తి* స్థిరరాశి flp = v3Global.rootp()->శిఖరం_Scopep()->fileline();
    ఒకవేళ (!m_n_Vec_పదాలు) తిరిగి కొత్త AstConst{flp, AstConst::బిట్టు_False{}};

    AstCFunc* funcp = శూన్యసూచిక;
    ఒకవేళ (vscp->dtypep() == m_trigVecDTypep) {
        ఒకవేళ (!m_any_సమితి_Vecp) m_any_సమితి_Vecp = create_Any_సమితి_Func(m_trigVecDTypep);
        funcp = m_any_సమితి_Vecp;
    } లేకపోతే ఒకవేళ (vscp->dtypep() == m_trigExtDTypep) {
        ఒకవేళ (!m_any_సమితి_Extp) m_any_సమితి_Extp = create_Any_సమితి_Func(m_trigExtDTypep);
        funcp = m_any_సమితి_Extp;
    } లేకపోతే {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, funcp};
    callp->కూడు_Argsp(కొత్త AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_సమితి_బిట్టు();
    తిరిగి callp;
}
Ast_కణుపు_Stmt* TriggerKit::newClearCall(AstVarScope* స్థిరరాశి vscp) స్థిరరాశి {
    ఒకవేళ (!m_n_Vec_పదాలు) తిరిగి శూన్యసూచిక;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    ఒకవేళ (!m_clearp) m_clearp = createClearFunc();
    File_పంక్తి* స్థిరరాశి flp = v3Global.rootp()->శిఖరం_Scopep()->fileline();
    AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, m_clearp};
    callp->కూడు_Argsp(కొత్త AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_సమితి_Void();
    తిరిగి callp->makeStmt();
}
Ast_కణుపు_Stmt* TriggerKit::newOrIntoCall(AstVarScope* స్థిరరాశి oVscp, AstVarScope* స్థిరరాశి iVscp) స్థిరరాశి {
    ఒకవేళ (!m_n_Vec_పదాలు) తిరిగి శూన్యసూచిక;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    స్థిరరాశి కొలత_t ముసుగు
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[ముసుగు];
    ఒకవేళ (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_పంక్తి* స్థిరరాశి flp = v3Global.rootp()->శిఖరం_Scopep()->fileline();
    AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, funcp};
    callp->కూడు_Argsp(కొత్త AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->కూడు_Argsp(కొత్త AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtype_సమితి_Void();
    తిరిగి callp->makeStmt();
}

Ast_కణుపు_Stmt* TriggerKit::new_Comp_పునాది_Call() స్థిరరాశి {
    ఒకవేళ (!m_n_Vec_పదాలు) తిరిగి శూన్యసూచిక;
    File_పంక్తి* స్థిరరాశి flp = v3Global.rootp()->శిఖరం_Scopep()->fileline();
    AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, m_compVecp};
    callp->dtype_సమితి_Void();
    తిరిగి callp->makeStmt();
}

Ast_కణుపు_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) స్థిరరాశి {
    ఒకవేళ (!m_n_Pre_పదాలు) తిరిగి శూన్యసూచిక;
    File_పంక్తి* స్థిరరాశి flp = v3Global.rootp()->శిఖరం_Scopep()->fileline();
    AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, m_compExtp};
    callp->కూడు_Argsp(కొత్త AstVarRef{flp, vscp, VAccess::READ});
    callp->dtype_సమితి_Void();
    తిరిగి callp->makeStmt();
}

Ast_కణుపు_Stmt* TriggerKit::newDumpCall(AstVarScope* స్థిరరాశి vscp, స్థిరరాశి std::string& ట్యాగ్,
                                     తార్కిక debugOnly) స్థిరరాశి {
    ఒకవేళ (!m_n_Vec_పదాలు) తిరిగి శూన్యసూచిక;
    AstCFunc* funcp = శూన్యసూచిక;
    ఒకవేళ (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } లేకపోతే ఒకవేళ (vscp->dtypep() == m_trigExtDTypep) {
        ఒకవేళ (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } లేకపోతే {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_పంక్తి* స్థిరరాశి flp = v3Global.rootp()->శిఖరం_Scopep()->fileline();
    AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, funcp};
    callp->కూడు_Argsp(కొత్త AstVarRef{flp, vscp, VAccess::READ});
    callp->కూడు_Argsp(కొత్త AstConst{flp, AstConst::String{}, ట్యాగ్});
    callp->dtype_సమితి_Void();
    AstCStmt* స్థిరరాశి cstmtp = కొత్త AstCStmt{flp};
    cstmtp->కూడు("#ifdef VL_DEBUG\n");
    ఒకవేళ (debugOnly) {
        cstmtp->కూడు("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->కూడు(callp->makeStmt());
        cstmtp->కూడు("}\n");
    } లేకపోతే {
        cstmtp->కూడు(callp->makeStmt());
    }
    cstmtp->కూడు("#endif");
    తిరిగి cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(స్థిరరాశి std::string& name) స్థిరరాశి {
    ఒకవేళ (!m_n_Vec_పదాలు) తిరిగి శూన్యసూచిక;
    తిరిగి v3Global.rootp()->శిఖరం_Scopep()->create_తాత్కాలిక("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extra_Trigger_సూచీ(uint32_t సూచీ) స్థిరరాశి {
    UASSERT(సూచీ <= m_n_Extra_పదాలు * మాట_కొలత, "Invalid external trigger index");
    తిరిగి సూచీ + m_n_Sense_పదాలు * మాట_కొలత;
}

std::సమితి<uint32_t> TriggerKit::triggerIndices(స్థిరరాశి Ast_Sen_వృక్షం* senTreep, తార్కిక pre) స్థిరరాశి {
    std::సమితి<uint32_t> indices;
    కొరకు (స్థిరరాశి Ast_Sen_అంశం* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), Sen_అంశం)) {
        uint32_t సూచీ = m_senItem2TrigIdx.at(*sip);
        ఒకవేళ (pre) {
            UASSERT_OBJ(సూచీ < m_n_Pre_పదాలు * మాట_కొలత, sip, "Not a 'pre' trigger");
            సూచీ += m_n_Vec_పదాలు * మాట_కొలత;
        }
        indices.చొప్పించు(సూచీ);
    }
    తిరిగి indices;
}

Ast_కణుపు_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t సూచీ) స్థిరరాశి {
    సూచీ = extra_Trigger_సూచీ(సూచీ);
    స్థిరరాశి uint32_t మాట_సూచీ = సూచీ / మాట_కొలత;
    స్థిరరాశి uint32_t బిట్టు_సూచీ = సూచీ % మాట_కొలత;
    File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
    AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_కణుపు_సమాసం* స్థిరరాశి wordp = కొత్త AstArraySel{flp, refp, static_cast<పూర్ణాంకం>(మాట_సూచీ)};
    Ast_కణుపు_సమాసం* స్థిరరాశి trigLhsp = కొత్త AstSel{flp, wordp, static_cast<పూర్ణాంకం>(బిట్టు_సూచీ), 1};
    Ast_కణుపు_సమాసం* స్థిరరాశి trigRhsp = కొత్త AstVarRef{flp, vscp, VAccess::READ};
    తిరిగి కొత్త Ast_కేటాయించు{flp, trigLhsp, trigRhsp};
}

శూన్య TriggerKit::కూడు_Extra_Trigger_Assignment(AstVarScope* vscp, uint32_t సూచీ) స్థిరరాశి {
    File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_కణుపు* స్థిరరాశి setp = newExtraTriggerAssignment(vscp, సూచీ);
    setp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, vscp, VAccess::WRITE},
                                కొత్త AstConst{flp, AstConst::బిట్టు_False{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
శూన్య TriggerKit::కూడు_విలువ_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t సూచీ) స్థిరరాశి {
    సూచీ += m_n_Sense_పదాలు * మాట_కొలత;
    స్థిరరాశి uint32_t మాట_సూచీ = సూచీ / మాట_కొలత;
    స్థిరరాశి uint32_t బిట్టు_సూచీ = సూచీ % మాట_కొలత;

    Ast_శిఖరం_Scope* స్థిరరాశి శిఖరం_Scopep = netlistp->శిఖరం_Scopep();
    File_పంక్తి* స్థిరరాశి flp = netlistp->fileline();
    Ast_కణుపు_DType* స్థిరరాశి dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి bdtypep = VN_CAST(dtypep, BasicDType)) {
        ఒకవేళ (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            తిరిగి;
        }
    } లేకపోతే ఒకవేళ (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, కణుపు_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        తిరిగి;
    }

    స్థిరరాశి స్వయంచాలక rdInst = [flp](AstVarScope* vp) { తిరిగి కొత్త AstVarRef{flp, vp, VAccess::READ}; };
    స్థిరరాశి స్వయంచాలక wrPrev = [flp](AstVarScope* vp) { తిరిగి కొత్త AstVarRef{flp, vp, VAccess::WRITE}; };
    స్థిరరాశి స్వయంచాలక rdPrev = [flp](AstVarScope* vp) { తిరిగి కొత్త AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    స్థిరరాశి std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* స్థిరరాశి prevVscp = శిఖరం_Scopep->create_తాత్కాలిక(prevName, instVscp->dtypep());

    // Initialize prev = inst
    ఒకవేళ (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* స్థిరరాశి cmhp = కొత్త AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_కేటాయించు, rdInst(instVscp)};
        cmhp->dtype_సమితి_Void();
        initFuncp->కూడు_Stmtsp(cmhp->makeStmt());
    } లేకపోతే {
        initFuncp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_కణుపు_సమాసం* neqp;
    ఒకవేళ (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* స్థిరరాశి cmhp
            = కొత్త AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtype_సమితి_బిట్టు();
        neqp = cmhp;
    } లేకపోతే {
        neqp = కొత్త AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_కణుపు* updp;
    ఒకవేళ (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* స్థిరరాశి cmhp = కొత్త AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_కేటాయించు, rdInst(instVscp)};
        cmhp->dtype_సమితి_Void();
        updp = cmhp->makeStmt();
    } లేకపోతే {
        updp = కొత్త Ast_కేటాయించు{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_కణుపు_సమాసం* స్థిరరాశి wordp = కొత్త AstArraySel{flp, refp, static_cast<పూర్ణాంకం>(మాట_సూచీ)};
    Ast_కణుపు_సమాసం* స్థిరరాశి trigLhsp = కొత్త AstSel{flp, wordp, static_cast<పూర్ణాంకం>(బిట్టు_సూచీ), 1};
    Ast_కణుపు* స్థిరరాశి setp = కొత్త Ast_కేటాయించు{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->కూడు_తదుపరి(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(స్థిరరాశి std::string& name, తార్కిక slow, uint32_t n_Sense_పదాలు,
                       uint32_t n_Extra_పదాలు, uint32_t n_Pre_పదాలు,
                       std::unordered_map<VNRef<స్థిరరాశి Ast_Sen_అంశం>, కొలత_t> senItem2TrigIdx,
                       స్థిరరాశి std::సదిశ<స్థిరరాశి Ast_Sen_అంశం*>& senItemps, తార్కిక useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_పదాలు{n_Sense_పదాలు}
    , m_n_Extra_పదాలు{n_Extra_పదాలు}
    , m_n_Pre_పదాలు{n_Pre_పదాలు}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trig_Idx2_Sen_అంశం{senItemps} {
    // If no triggers, we don't need to generate anything
    ఒకవేళ (!m_n_Vec_పదాలు) తిరిగి;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trig_Idx2_Sen_అంశం.resize((m_n_Vec_పదాలు + m_n_Pre_పదాలు) * మాట_కొలత);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_పదాలు * మాట_కొలత,
              m_trig_Idx2_Sen_అంశం.begin() + m_n_Vec_పదాలు * మాట_కొలత);
    AstNetlist* స్థిరరాశి netlistp = v3Global.rootp();
    AstScope* స్థిరరాశి scopep = netlistp->శిఖరం_Scopep()->scopep();
    File_పంక్తి* స్థిరరాశి flp = scopep->fileline();
    // Data type of a single trigger word
    m_మాట_DTypep = netlistp->కనుగొను_బిట్టు_DType(మాట_కొలత, మాట_కొలత, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* స్థిరరాశి rp = కొత్త AstRange{flp, static_cast<పూర్ణాంకం>(m_n_Vec_పదాలు - 1), 0};
    m_trigVecDTypep = కొత్త AstUnpackArrayDType{flp, m_మాట_DTypep, rp};
    netlistp->typeTablep()->కూడు_Typesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    ఒకవేళ (m_n_Pre_పదాలు) {
        AstRange* స్థిరరాశి ep = కొత్త AstRange{flp, static_cast<పూర్ణాంకం>(m_n_Vec_పదాలు + m_n_Pre_పదాలు - 1), 0};
        m_trigExtDTypep = కొత్త AstUnpackArrayDType{flp, m_మాట_DTypep, ep};
        netlistp->typeTablep()->కూడు_Typesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } లేకపోతే {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->శిఖరం_Scopep()->create_తాత్కాలిక("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->is_అంతర్గత(నిజం);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, నిజం);
    m_dumpp->isStatic(నిజం);
    m_dumpp->ifdef("VL_DEBUG");
    ఒకవేళ (useAcc) {
        m_vscAccp
            = netlistp->శిఖరం_Scopep()->create_తాత్కాలిక("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->is_అంతర్గత(నిజం);
    }
}

Ast_కేటాయించు* TriggerKit::createSenTrigVecAssignment(AstVarScope* స్థిరరాశి లక్ష్యం,
                                                  std::సదిశ<Ast_కణుపు_సమాసం*>& trigps) {
    File_పంక్తి* స్థిరరాశి flp = లక్ష్యం->fileline();
    Ast_కేటాయించు* trigStmtsp = శూన్యసూచిక;
    // Assign sense triggers vector one word at a time
    కొరకు (కొలత_t i = 0; i < trigps.కొలత(); i += మాట_కొలత) {
        // Concatenate all bits in this trigger word using a balanced
        కొరకు (uint32_t level = 0; level < మాట_కొలత_LOG2; ++level) {
            స్థిరరాశి uint32_t stride = 1 << level;
            కొరకు (uint32_t j = 0; j < మాట_కొలత; j += 2 * stride) {
                trigps[i + j] = కొత్త AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = శూన్యసూచిక;
            }
        }

        // Set the whole word in the trigger vector
        స్థిరరాశి పూర్ణాంకం మాట_సూచీ = static_cast<పూర్ణాంకం>(i / మాట_కొలత);
        AstArraySel* స్థిరరాశి aselp
            = కొత్త AstArraySel{flp, కొత్త AstVarRef{flp, లక్ష్యం, VAccess::WRITE}, మాట_సూచీ};
        trigStmtsp = Ast_కణుపు::కూడు_తదుపరి(trigStmtsp, కొత్త Ast_కేటాయించు{flp, aselp, trigps[i]});
    }
    తిరిగి trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* స్థిరరాశి initFuncp,  //
                              Sen_సమాసం_Builder& sen_సమాసం_Builder,  //
                              స్థిరరాశి std::సదిశ<స్థిరరాశి Ast_Sen_వృక్షం*>& preTreeps,  //
                              స్థిరరాశి std::సదిశ<స్థిరరాశి Ast_Sen_వృక్షం*>& senTreeps,  //
                              స్థిరరాశి string& name,  //
                              స్థిరరాశి ExtraTriggers& extraTriggers,  //
                              తార్కిక slow,  //
                              తార్కిక useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::సదిశ<స్థిరరాశి Ast_Sen_అంశం*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<స్థిరరాశి Ast_Sen_అంశం>, కొలత_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    కొరకు (స్థిరరాశి Ast_Sen_వృక్షం* స్థిరరాశి senTreep : preTreeps) {
        కొరకు (స్థిరరాశి Ast_Sen_అంశం *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_అంశం);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            స్థిరరాశి స్వయంచాలక pair = senItem2TrigIdx.emplace(*itemp, senItemps.కొలత());
            ఒకవేళ (pair.రెండవ) senItemps.నెట్టు_వెనుక(itemp);
        }
    }
    స్థిరరాశి uint32_t n_Pre_Sen_వస్తువులు = senItemps.కొలత();
    V3Stats::కూడు_Stat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_వస్తువులు);
    // Number of pre triggers, rounded up to a full word.
    స్థిరరాశి uint32_t nPreTriggers = vlstd::విడత_పైకి_గమ్యం_గుణిజం_యొక్క<మాట_కొలత>(senItemps.కొలత());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    స్థిరరాశి uint32_t n_Pre_పదాలు = nPreTriggers / మాట_కొలత;

    // Process the rest of the trees
    కొరకు (స్థిరరాశి Ast_Sen_వృక్షం* స్థిరరాశి senTreep : senTreeps) {
        కొరకు (స్థిరరాశి Ast_Sen_అంశం *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_అంశం);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            స్థిరరాశి స్వయంచాలక pair = senItem2TrigIdx.emplace(*itemp, senItemps.కొలత());
            ఒకవేళ (pair.రెండవ) senItemps.నెట్టు_వెనుక(itemp);
        }
    }
    స్థిరరాశి uint32_t n_Sen_వస్తువులు = senItemps.కొలత() - nPreTriggers;
    V3Stats::కూడు_Stat("Scheduling, '" + name + "' sense triggers", n_Sen_వస్తువులు + n_Pre_Sen_వస్తువులు);
    // Number of sense triggers, rounded up to a full word
    స్థిరరాశి uint32_t nSenseTriggers = vlstd::విడత_పైకి_గమ్యం_గుణిజం_యొక్క<మాట_కొలత>(senItemps.కొలత());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    స్థిరరాశి uint32_t n_Sense_పదాలు = nSenseTriggers / మాట_కొలత;

    // Allocate space for the extra triggers
    V3Stats::కూడు_Stat("Scheduling, '" + name + "' extra triggers", extraTriggers.కొలత());
    // Number of extra triggers, rounded up to a full word.
    స్థిరరాశి uint32_t nExtraTriggers = vlstd::విడత_పైకి_గమ్యం_గుణిజం_యొక్క<మాట_కొలత>(extraTriggers.కొలత());
    స్థిరరాశి uint32_t n_Extra_పదాలు = nExtraTriggers / మాట_కొలత;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_పదాలు, n_Extra_పదాలు, n_Pre_పదాలు, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    ఒకవేళ (!kit.m_n_Vec_పదాలు) తిరిగి kit;

    File_పంక్తి* స్థిరరాశి flp = netlistp->శిఖరం_Scopep()->fileline();

    // Creates read/write reference
    స్థిరరాశి స్వయంచాలక rd = [flp](AstVarScope* vp) { తిరిగి కొత్త AstVarRef{flp, vp, VAccess::READ}; };
    స్థిరరాశి స్వయంచాలక wr = [flp](AstVarScope* vp) { తిరిగి కొత్త AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* స్థిరరాశి dumpTrgp
        = new_వాదం(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* స్థిరరాశి dumpTagp
        = new_వాదం(kit.m_dumpp, netlistp->కనుగొను_String_DType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, కొత్త AstLogNot{flp, kit.new_Any_సమితి_Call(dumpTrgp)}};
        kit.m_dumpp->కూడు_Stmtsp(ifp);
        AstCStmt* స్థిరరాశి cstmtp = కొత్త AstCStmt{flp};
        ifp->కూడు_Thensp(cstmtp);
        cstmtp->కూడు("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->కూడు(rd(dumpTagp));
        cstmtp->కూడు(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    స్థిరరాశి స్వయంచాలక కూడు_Debug = [&](uint32_t సూచీ, స్థిరరాశి string& పాఠ్యం) {
        స్థిరరాశి పూర్ణాంకం wrd_సూచీ = static_cast<పూర్ణాంకం>(సూచీ / మాట_కొలత);
        స్థిరరాశి పూర్ణాంకం బిట్టు_సూచీ = static_cast<పూర్ణాంకం>(సూచీ % మాట_కొలత);
        Ast_కణుపు_సమాసం* స్థిరరాశి aselp = కొత్త AstArraySel{flp, rd(dumpTrgp), wrd_సూచీ};
        Ast_కణుపు_సమాసం* స్థిరరాశి condp = కొత్త AstSel{flp, aselp, బిట్టు_సూచీ, 1};
        AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, condp};
        kit.m_dumpp->కూడు_Stmtsp(ifp);
        AstCStmt* స్థిరరాశి cstmtp = కొత్త AstCStmt{flp};
        ifp->కూడు_Thensp(cstmtp);
        cstmtp->కూడు("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->కూడు(rd(dumpTagp));
        cstmtp->కూడు(" + \"' region trigger index " + std::గమ్యం_string(సూచీ) + " is active: " + పాఠ్యం
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::సదిశ<Ast_కణుపు_సమాసం*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_కణుపు_Stmt* initialTrigsp = శూన్యసూచిక;
    కొరకు (కొలత_t i = 0; i < senItemps.కొలత(); ++i) {
        స్థిరరాశి Ast_Sen_అంశం* స్థిరరాశి senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        ఒకవేళ (!senItemp) {
            trigps.emplace_వెనుక(కొత్త AstConst{flp, AstConst::బిట్టు_False{}});
            కొనసాగు;
        }

        // Create the trigger computation expression
        స్థిరరాశి స్వయంచాలక& pair = sen_సమాసం_Builder.build(senItemp);
        trigps.emplace_వెనుక(pair.మొదటి);

        // Add initialization time trigger
        ఒకవేళ (pair.రెండవ || v3Global.opt.x_Initial_అంచు()) {
            స్థిరరాశి పూర్ణాంకం wrd_సూచీ = static_cast<పూర్ణాంకం>(i / మాట_కొలత);
            స్థిరరాశి పూర్ణాంకం బిట్టు_సూచీ = static_cast<పూర్ణాంకం>(i % మాట_కొలత);
            Ast_కణుపు_సమాసం* స్థిరరాశి wordp = కొత్త AstArraySel{flp, wr(kit.m_vscp), wrd_సూచీ};
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = కొత్త AstSel{flp, wordp, బిట్టు_సూచీ, 1};
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = కొత్త AstConst{flp, AstConst::బిట్టు_True{}};
            ఒకవేళ (useAcc) {
                initFuncp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, lhsp, rhsp});
            } లేకపోతే {
                initialTrigsp = Ast_కణుపు::కూడు_తదుపరి(initialTrigsp, కొత్త Ast_కేటాయించు{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_వృక్షం(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        కూడు_Debug(i, desc);
    }
    UASSERT(trigps.కొలత() == nSenseTriggers, "Inconsistent number of trigger expressions");

    Ast_కేటాయించు* స్థిరరాశి trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    కొరకు (గుర్తులేని i = 0; i < extraTriggers.కొలత(); ++i) {
        కూడు_Debug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    స్థిరరాశి Sen_సమాసం_Builder::Results senResults = sen_సమాసం_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    కొరకు (Ast_కణుపు_Stmt* స్థిరరాశి nodep : senResults.m_inits) initFuncp->కూడు_Stmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_శిఖరం_Scope* స్థిరరాశి శిఖరం_Scopep = netlistp->శిఖరం_Scopep();
    {
        AstCFunc* స్థిరరాశి fp = kit.m_compVecp;
        // Trigger computation
        కొరకు (Ast_కణుపు_Stmt* స్థిరరాశి nodep : senResults.m_preUpdates) fp->కూడు_Stmtsp(nodep);
        fp->కూడు_Stmtsp(trigStmtsp);
        కొరకు (Ast_కణుపు_Stmt* స్థిరరాశి nodep : senResults.m_postUpdates) fp->కూడు_Stmtsp(nodep);
        // Add the initialization time triggers
        ఒకవేళ (initialTrigsp) {
            AstVarScope* స్థిరరాశి initVscp = శిఖరం_Scopep->create_తాత్కాలిక("__V" + name + "DidInit", 1);
            AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, కొత్త AstNot{flp, rd(initVscp)}};
            fp->కూడు_Stmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->కూడు_Thensp(util::సమితి_Var(initVscp, 1));
            ifp->కూడు_Thensp(initialTrigsp);
        }
        util::విభజించు_Check(fp);
    };
    // If there are 'pre' triggers, compute them
    ఒకవేళ (kit.m_n_Pre_పదాలు) {
        AstCFunc* స్థిరరాశి fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* స్థిరరాశి latchedp
            = new_వాదం(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* స్థిరరాశి nVscp = శిఖరం_Scopep->create_తాత్కాలిక("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(నిజం);
        // Add a loop to compute the pre words
        AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
        fp->కూడు_Stmtsp(util::సమితి_Var(nVscp, 0));
        fp->కూడు_Stmtsp(loopp);
        // Loop body
        Ast_కణుపు_సమాసం* స్థిరరాశి offsetp = కొత్త AstConst{flp, kit.m_n_Vec_పదాలు};
        Ast_కణుపు_సమాసం* స్థిరరాశి lIdxp = కొత్త Ast_కూడు{flp, rd(nVscp), offsetp};
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = కొత్త AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_కణుపు_సమాసం* స్థిరరాశి aWordp = కొత్త AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_కణుపు_సమాసం* స్థిరరాశి bWordp = కొత్త AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = కొత్త AstAnd{flp, aWordp, కొత్త AstNot{flp, bWordp}};
        Ast_కణుపు_సమాసం* స్థిరరాశి limp = కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, 32, n_Pre_పదాలు};
        loopp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, lhsp, rhsp});
        loopp->కూడు_Stmtsp(util::incrementVar(nVscp));
        loopp->కూడు_Stmtsp(కొత్త AstLoopTest{flp, loopp, కొత్త AstLt{flp, rd(nVscp), limp}});
        util::విభజించు_Check(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    ఒకవేళ (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి()->delete_వృక్షం();
    // Done with the trigger dump function, split as might be large
    util::విభజించు_Check(kit.m_dumpp);

    తిరిగి kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
తరగతి AwaitBeforeTrigVisitor final : బహిరంగ VNVisitor {
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;

    /**
     * AstCAwait::user1()       ->  bool.           True if node has been visited
     * AstSenTree::user1p()     ->  AstCFunc*.      Function that has to be called before awaiting
     *                                              for CAwait pointing to this SenTree
     * AstCFunc::user1p()       ->  AstVarScope*    Function's local temporary extended trigger
     *                                              vector variable scope
     */

    // Netlist - needed for using util::makeSubFunction()
    AstNetlist* స్థిరరాశి m_netlistp;
    // Trigger kit - for accessing trigger vectors and mapping senItems to thier indexes
    స్థిరరాశి TriggerKit& m_trigKit;
    // Expression builder - for building expressions from SenItems
    Sen_సమాసం_Builder& m_sen_సమాసం_Builder;
    // Generator of unique names for before-trigger function
    V3_Unique_పేర్లు m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::సదిశ<std::pair<AstCFunc*, Ast_Sen_వృక్షం*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::సదిశ<std::pair<Ast_Sen_వృక్షం*, Ast_కణుపు_సమాసం*>> m_sen_వృక్షం_గమ్యం_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_కణుపు>, std::సదిశ<Ast_Sen_అంశం*>> m_sen_సమాసం_గమ్యం_Sen_అంశం;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    స్థిర Ast_కణుపు* get_Sen_Hash_కణుపు(స్థిరరాశి Ast_Sen_అంశం* స్థిరరాశి nodep) {
        ఒకవేళ (AstVarRef* స్థిరరాశి varRefp = VN_CAST(nodep->sensp(), VarRef)) తిరిగి varRefp;
        తిరిగి nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    శూన్య fill_Sen_సమాసం_గమ్యం_Sen_అంశం() {
        కొరకు (స్వయంచాలక sen_వృక్షం_Sched : m_sen_వృక్షం_గమ్యం_Sched) {
            Ast_Sen_వృక్షం* స్థిరరాశి senTreep = sen_వృక్షం_Sched.మొదటి;

            కొరకు (Ast_Sen_అంశం* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_అంశం)) {
                స్థిరరాశి VEdgeType అంచు = senItemp->అంచు_Type();
                ఒకవేళ (అంచు.an_అంచు() || అంచు == VEdgeType::ET_మారింది
                    || అంచు == VEdgeType::ET_HYBRID) {
                    m_sen_సమాసం_గమ్యం_Sen_అంశం[*get_Sen_Hash_కణుపు(senItemp)].నెట్టు_వెనుక(senItemp);
                }
            }
        }

        std::సదిశ<VNRef<Ast_కణుపు>> గమ్యం_Remove;
        కొరకు (స్థిరరాశి స్వయంచాలక& sen_సమాసం_గమ్యం_Sen_వృక్షం : m_sen_సమాసం_గమ్యం_Sen_అంశం) {
            std::సదిశ<Ast_Sen_అంశం*> senItemps = sen_సమాసం_గమ్యం_Sen_వృక్షం.రెండవ;
            గమ్యం_Remove.నెట్టు_వెనుక(sen_సమాసం_గమ్యం_Sen_వృక్షం.మొదటి);
            కొరకు (కొలత_t i = 1; i < senItemps.కొలత(); ++i) {
                ఒకవేళ (senItemps[i]->అంచు_Type() != senItemps[i - 1]->అంచు_Type()) {
                    గమ్యం_Remove.తీయి_వెనుక();
                    ఆపు;
                }
            }
        }
        కొరకు (స్థిరరాశి VNRef<Ast_కణుపు> it : గమ్యం_Remove) m_sen_సమాసం_గమ్యం_Sen_అంశం.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<కొలత_t, std::map<కొలత_t, std::సదిశ<Ast_కణుపు_సమాసం*>>>
    get_వాడిన_Triggers_గమ్యం_Trees(స్థిరరాశి std::సమితి<కొలత_t>& వాడిన_Triggers) {
        std::map<కొలత_t, std::map<కొలత_t, std::సదిశ<Ast_కణుపు_సమాసం*>>> వాడిన_Trigs_గమ్యం_Using_Trees;
        కొరకు (స్వయంచాలక sen_వృక్షం_Sched : m_sen_వృక్షం_గమ్యం_Sched) {
            స్థిరరాశి Ast_Sen_వృక్షం* స్థిరరాశి senTreep = sen_వృక్షం_Sched.మొదటి;
            Ast_కణుపు_సమాసం* స్థిరరాశి shedp = sen_వృక్షం_Sched.రెండవ;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            కొరకు (Ast_Sen_అంశం* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_అంశం)) {
                స్థిరరాశి కొలత_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                ఒకవేళ (వాడిన_Triggers.కనుగొను(idx) != వాడిన_Triggers.చివర()) {
                    వాడిన_Trigs_గమ్యం_Using_Trees[idx / TriggerKit::మాట_కొలత]
                                         [కొలత_t{1} << (idx % TriggerKit::మాట_కొలత)]
                                             .నెట్టు_వెనుక(shedp);
                }
            }
        }
        ఒకవేళ (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            కొరకు (స్థిరరాశి స్వయంచాలక& triggers_గమ్యం_Trees : వాడిన_Trigs_గమ్యం_Using_Trees) {
                కొరకు (స్థిరరాశి స్వయంచాలక& బిట్లు_గమ్యం_Trees : triggers_గమ్యం_Trees.రెండవ) {
                    స్థిరరాశి std::సమితి<స్థిరరాశి Ast_కణుపు_సమాసం*> exprps{బిట్లు_గమ్యం_Trees.రెండవ.begin(),
                                                              బిట్లు_గమ్యం_Trees.రెండవ.చివర()};
                    UASSERT(బిట్లు_గమ్యం_Trees.రెండవ.కొలత() == exprps.కొలత(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        తిరిగి వాడిన_Trigs_గమ్యం_Using_Trees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_వృక్షం* స్థిరరాశి senTreep) {
        File_పంక్తి* స్థిరరాశి flp = senTreep->fileline();
        ఒకవేళ (!senTreep->user1p()) {
            AstCFunc* స్థిరరాశి funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), అబద్ధం);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* స్థిరరాశి tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(నిజం);

            AstVar* స్థిరరాశి argp = కొత్త AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->కనుగొను_Basic_DType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(నిజం);
            argp->direction(VDirection::ఆదానం);
            funcp->కూడు_Argsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_వెనుక(funcp, senTreep);
        }
        AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtype_సమితి_Void();
        తిరిగి callp;
    }

    శూన్య సందర్శించు(AstCAwait* స్థిరరాశి nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        ఒకవేళ (స్థిరరాశి AstCMethodHard* స్థిరరాశి c_పద్ధతి_Hardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            ఒకవేళ (c_పద్ధతి_Hardp->పద్ధతి() == VCMethod::SCHED_TRIGGER) {
                AstCCall* స్థిరరాశి beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_కణుపు* స్థిరరాశి pinp = c_పద్ధతి_Hardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, c_పద్ధతి_Hardp, "No event description");
                beforeTrigp->కూడు_Argsp(VN_AS(pinp, కణుపు_సమాసం)->clone_వృక్షం(అబద్ధం));

                // Call the before-trigger function before the CAwait
                nodep->కూడు_ఇక్కడ_This_As_తదుపరి(beforeTrigp->makeStmt());
                m_sen_వృక్షం_గమ్యం_Sched.emplace_వెనుక(nodep->sentreep(), c_పద్ధతి_Hardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* స్థిరరాశి nodep) override { iterateChildren(nodep); }

బహిరంగ:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_సమాసం_Builder& sen_సమాసం_Builder,
                           స్థిరరాశి TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_సమాసం_Builder{sen_సమాసం_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_సమాసం_గమ్యం_Sen_అంశం();

        std::సదిశ<Ast_కణుపు_సమాసం*> trigps;
        std::సమితి<కొలత_t> వాడిన_Triggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        కొరకు (స్థిరరాశి స్వయంచాలక& func_గమ్యం_వాడిన_Triggers : m_generatedFuncs) {
            AstCFunc* స్థిరరాశి funcp = func_గమ్యం_వాడిన_Triggers.మొదటి;
            AstVarScope* స్థిరరాశి vscp = VN_AS(funcp->user1p(), VarScope);
            File_పంక్తి* స్థిరరాశి flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_వృక్షం* స్థిరరాశి senTreep = func_గమ్యం_వాడిన_Triggers.రెండవ;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                స్థిరరాశి స్వయంచాలక emplaceAt
                    = [flp, &trigps, &వాడిన_Triggers](Ast_కణుపు_సమాసం* స్థిరరాశి exprp, స్థిరరాశి కొలత_t pos) {
                          స్థిరరాశి కొలత_t లక్ష్యం_కొలత
                              = vlstd::విడత_పైకి_గమ్యం_గుణిజం_యొక్క<TriggerKit::మాట_కొలత>(pos + 1);
                          ఒకవేళ (trigps.సామర్థ్యం() < లక్ష్యం_కొలత) trigps.reserve(లక్ష్యం_కొలత * 2);
                          అయితే (trigps.కొలత() < లక్ష్యం_కొలత) {
                              trigps.నెట్టు_వెనుక(కొత్త AstConst{flp, AstConst::బిట్టు_False{}});
                          }
                          trigps[pos]->delete_వృక్షం();
                          trigps[pos] = exprp;
                          వాడిన_Triggers.చొప్పించు(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                కొరకు (స్థిరరాశి Ast_Sen_అంశం* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Sen_అంశం)) {
                    స్థిరరాశి కొలత_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_సమాసం_Builder.build(itemp).మొదటి, idx);
                    స్వయంచాలక iter = m_sen_సమాసం_గమ్యం_Sen_అంశం.కనుగొను(*get_Sen_Hash_కణుపు(itemp));
                    ఒకవేళ (iter != m_sen_సమాసం_గమ్యం_Sen_అంశం.చివర()) {
                        కొరకు (Ast_Sen_అంశం* స్థిరరాశి additionalItemp : iter->రెండవ) {
                            స్థిరరాశి కొలత_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_సమాసం_Builder.build(additionalItemp).మొదటి, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                స్థిరరాశి Sen_సమాసం_Builder::Results results
                    = m_sen_సమాసం_Builder.getResultsAndClearUpdates();
                కొరకు (Ast_కణుపు_Stmt* స్థిరరాశి stmtsp : results.m_inits) funcp->కూడు_Stmtsp(stmtsp);
                కొరకు (Ast_కణుపు_Stmt* స్థిరరాశి stmtsp : results.m_preUpdates) funcp->కూడు_Stmtsp(stmtsp);
                funcp->కూడు_Stmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                కొరకు (Ast_కణుపు_Stmt* స్థిరరాశి stmtsp : results.m_postUpdates) funcp->కూడు_Stmtsp(stmtsp);
            }

            స్థిరరాశి std::map<కొలత_t, std::map<కొలత_t, std::సదిశ<Ast_కణుపు_సమాసం*>>>
                వాడిన_Trigs_గమ్యం_Using_Trees = get_వాడిన_Triggers_గమ్యం_Trees(వాడిన_Triggers);
            వాడిన_Triggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            స్థిరరాశి స్వయంచాలక getIdx = [flp](AstVarScope* స్థిరరాశి scocep, VAccess access, కొలత_t idx) {
                తిరిగి కొత్త AstArraySel{flp, కొత్త AstVarRef{flp, scocep, access},
                                       కొత్త AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* స్థిరరాశి argpVscp = కొత్త AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->కూడు_Varsp(argpVscp);

            // Mark as ready triggered schedulers
            కొరకు (స్థిరరాశి స్వయంచాలక& triggers_గమ్యం_Trees : వాడిన_Trigs_గమ్యం_Using_Trees) {
                స్థిరరాశి కొలత_t మాట = triggers_గమ్యం_Trees.మొదటి;

                కొరకు (స్థిరరాశి స్వయంచాలక& బిట్లు_గమ్యం_Trees : triggers_గమ్యం_Trees.రెండవ) {
                    స్థిరరాశి కొలత_t బిట్టు_2 = బిట్లు_గమ్యం_Trees.మొదటి;
                    స్థిరరాశి స్వయంచాలక& schedulers = బిట్లు_గమ్యం_Trees.రెండవ;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* స్థిరరాశి ముసుగు_Constp = కొత్త AstConst{flp, AstConst::Unsized64{}, బిట్టు_2};
                    AstAnd* స్థిరరాశి condp
                        = కొత్త AstAnd{flp, getIdx(vscp, VAccess::READ, మాట), ముసుగు_Constp};
                    AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి schedp : schedulers) {
                        AstCMethodHard* స్థిరరాశి callp = కొత్త AstCMethodHard{
                            flp, schedp->clone_వృక్షం(అబద్ధం), VCMethod::SCHED_READY};
                        callp->dtype_సమితి_Void();
                        callp->కూడు_Pinsp(కొత్త AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->కూడు_Thensp(callp->makeStmt());
                    }
                    funcp->కూడు_Stmtsp(ifp);
                }
            }

            AstVarScope* స్థిరరాశి vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            కొరకు (స్థిరరాశి స్వయంచాలక& triggers_గమ్యం_Trees : వాడిన_Trigs_గమ్యం_Using_Trees) {
                స్థిరరాశి కొలత_t మాట = triggers_గమ్యం_Trees.మొదటి;
                funcp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, getIdx(vscAccp, VAccess::WRITE, మాట),
                                               కొత్త AstOr{flp, getIdx(vscAccp, VAccess::READ, మాట),
                                                         getIdx(vscp, VAccess::READ, మాట)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = అప్రమేయ;
};

శూన్య beforeTrigVisitor(AstNetlist* netlistp, Sen_సమాసం_Builder& sen_సమాసం_Builder,
                       స్థిరరాశి TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_సమాసం_Builder, trigKit};
}

}  // namespace V3Sched
