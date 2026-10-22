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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Const.h"
#ضمن "V3EmitCBase.h"
#ضمن "V3EmitV.h"
#ضمن "V3Order.h"
#ضمن "V3Sched.h"
#ضمن "V3SenExprBuilder.h"
#ضمن "V3Stats.h"
#ضمن "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق V3Sched {

نطاق {

کارول util::new_دلیل;

AstVarScope* newLocal(AstCFunc* funcp, Ast_غوټه_DType* dtypep, ثابت std::string& name) {
    File_کرښه* ثابت flp = funcp->fileline();
    AstScope* ثابت scopep = funcp->scopep();
    AstVar* ثابت varp = جديد AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(ریښتیا);
    varp->noReset(ریښتیا);
    funcp->جمع_کړه_Varsp(varp);
    AstVarScope* ثابت vscp = جديد AstVarScope{flp, scopep, varp};
    scopep->جمع_کړه_Varsp(vscp);
    أرجع vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() ثابت {
    UASSERT(m_n_Pre_ټکي, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_کرښه* ثابت flp = netlistp->سر_Scopep()->fileline();
    Ast_غوټه_DType* ثابت u32DTypep = netlistp->پیدا_کړه_UInt32_DType();
    Ast_غوټه_DType* ثابت strDtypep = netlistp->پیدا_کړه_String_DType();

    // Dumping function always slow
    ثابت std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, ریښتیا);
    funcp->isStatic(ریښتیا);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* ثابت eVscp = new_دلیل(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* ثابت tVscp = new_دلیل(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    ثابت خپلکاری rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خپلکاری wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* ثابت vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* ثابت iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->جمع_کړه_Stmtsp(util::سېټ_Var(iVscp, 0));
        // Add loop
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        funcp->جمع_کړه_Stmtsp(loopp);
        // Loop body
        Ast_غوټه_بیان* ثابت lhsp = جديد AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_غوټه_بیان* ثابت rhsp = جديد AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_غوټه_بیان* ثابت limp = جديد AstConst{flp, AstConst::Widthed_ارزښت{}, 32, m_n_Vec_ټکي};
        loopp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, lhsp, rhsp});
        loopp->جمع_کړه_Stmtsp(util::incrementVar(iVscp));
        loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* ثابت callp = جديد AstCCall{flp, m_dumpp};
        callp->dtype_سېټ_Void();
        callp->جمع_کړه_Argsp(rd(vVscp));
        callp->جمع_کړه_Argsp(rd(tVscp));
        funcp->جمع_کړه_Stmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* ثابت pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* ثابت jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->جمع_کړه_Stmtsp(util::سېټ_Var(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            funcp->جمع_کړه_Stmtsp(loopp);
            // Loop body
            Ast_غوټه_بیان* ثابت lhsp = جديد AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_غوټه_بیان* ثابت rhsp = جديد AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_غوټه_بیان* ثابت limp = جديد AstConst{flp, AstConst::Widthed_ارزښت{}, 32, m_n_Pre_ټکي};
            loopp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, lhsp, rhsp});
            loopp->جمع_کړه_Stmtsp(util::incrementVar(jVscp));
            loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            funcp->جمع_کړه_Stmtsp(loopp);
            // Loop body
            Ast_غوټه_بیان* ثابت lhsp = جديد AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_غوټه_بیان* ثابت rhsp = جديد AstConst{flp, AstConst::DTyped{}, m_کلمه_DTypep};
            Ast_غوټه_بیان* ثابت limp = جديد AstConst{flp, AstConst::Widthed_ارزښت{}, 32, m_n_Vec_ټکي};
            loopp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, lhsp, rhsp});
            loopp->جمع_کړه_Stmtsp(util::incrementVar(jVscp));
            loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* ثابت callp = جديد AstCCall{flp, m_dumpp};
        callp->dtype_سېټ_Void();
        callp->جمع_کړه_Argsp(rd(pVscp));
        callp->جمع_کړه_Argsp(
            جديد AstConcatN{flp, rd(tVscp), جديد AstConst{flp, AstConst::String{}, " pre"}});
        funcp->جمع_کړه_Stmtsp(callp->makeStmt());
    }

    أرجع funcp;
}

AstCFunc* TriggerKit::create_Any_سېټ_Func(AstUnpackArrayDType* ثابت dtypep) ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_کرښه* ثابت flp = netlistp->سر_Scopep()->fileline();
    Ast_غوټه_DType* ثابت u32DTypep = netlistp->پیدا_کړه_UInt32_DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(ریښتیا);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* ثابت iVscp = new_دلیل(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    ثابت خپلکاری rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* ثابت loopp = جديد AstLoop{flp};
    funcp->جمع_کړه_Stmtsp(util::سېټ_Var(nVscp, 0));
    funcp->جمع_کړه_Stmtsp(loopp);
    funcp->جمع_کړه_Stmtsp(جديد AstCReturn{flp, جديد AstConst{flp, AstConst::بټ_False{}}});

    // Loop body
    ثابت uint32_t n_ټکي = dtypep->elementsConst();
    Ast_غوټه_بیان* ثابت condp = جديد AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_غوټه_Stmt* ثابت thenp = جديد AstCReturn{flp, جديد AstConst{flp, AstConst::بټ_True{}}};
    Ast_غوټه_بیان* ثابت limp = جديد AstConst{flp, AstConst::Widthed_ارزښت{}, 32, n_ټکي};
    loopp->جمع_کړه_Stmtsp(جديد AstIf{flp, condp, thenp});
    loopp->جمع_کړه_Stmtsp(util::incrementVar(nVscp));
    loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});

    //بشپړ
    أرجع funcp;
}
AstCFunc* TriggerKit::createClearFunc() ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_کرښه* ثابت flp = netlistp->سر_Scopep()->fileline();
    Ast_غوټه_DType* ثابت u32DTypep = netlistp->پیدا_کړه_UInt32_DType();

    // Create function
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(ریښتیا);

    // Add arguments
    AstVarScope* ثابت oVscp = new_دلیل(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ثابت خپلکاری rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خپلکاری wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ثابت loopp = جديد AstLoop{flp};
    funcp->جمع_کړه_Stmtsp(util::سېټ_Var(nVscp, 0));
    funcp->جمع_کړه_Stmtsp(loopp);

    // Loop body
    Ast_غوټه_بیان* ثابت lhsp = جديد AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_غوټه_بیان* ثابت rhsp = جديد AstConst{flp, AstConst::DTyped{}, m_کلمه_DTypep};
    Ast_غوټه_بیان* ثابت limp = جديد AstConst{flp, AstConst::Widthed_ارزښت{}, 32, m_n_Vec_ټکي};
    loopp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, lhsp, rhsp});
    loopp->جمع_کړه_Stmtsp(util::incrementVar(nVscp));
    loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});

    //بشپړ
    أرجع funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ثابت oDtypep,
                                       AstUnpackArrayDType* ثابت iDtypep) ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_کرښه* ثابت flp = netlistp->سر_Scopep()->fileline();
    Ast_غوټه_DType* ثابت u32DTypep = netlistp->پیدا_کړه_UInt32_DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(ریښتیا);

    // Add arguments
    AstVarScope* ثابت oVscp = new_دلیل(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* ثابت iVscp = new_دلیل(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ثابت خپلکاری rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خپلکاری wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ثابت loopp = جديد AstLoop{flp};
    funcp->جمع_کړه_Stmtsp(util::سېټ_Var(nVscp, 0));
    funcp->جمع_کړه_Stmtsp(loopp);

    // Loop body
    Ast_غوټه_بیان* ثابت lhsp = جديد AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_غوټه_بیان* ثابت oWordp = جديد AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_غوټه_بیان* ثابت iWordp = جديد AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_غوټه_بیان* ثابت rhsp = جديد AstOr{flp, oWordp, iWordp};
    AstConst* ثابت outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ثابت ننوت_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_غوټه_بیان* ثابت limp = outputRangeLeftp->عدد().منزل_SInt() < ننوت_Range_Leftp->عدد().منزل_SInt()
                                  ? outputRangeLeftp->clone_ونه_خالص(خطأ)
                                  : ننوت_Range_Leftp->clone_ونه_خالص(خطأ);
    loopp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, lhsp, rhsp});
    loopp->جمع_کړه_Stmtsp(util::incrementVar(nVscp));
    loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLte{flp, rd(nVscp), limp}});

    //بشپړ
    أرجع funcp;
}

Ast_غوټه_بیان* TriggerKit::new_Any_سېټ_Call(AstVarScope* ثابت vscp) ثابت {
    File_کرښه* ثابت flp = v3Global.rootp()->سر_Scopep()->fileline();
    إذا (!m_n_Vec_ټکي) أرجع جديد AstConst{flp, AstConst::بټ_False{}};

    AstCFunc* funcp = تش_اشاره;
    إذا (vscp->dtypep() == m_trigVecDTypep) {
        إذا (!m_any_سېټ_Vecp) m_any_سېټ_Vecp = create_Any_سېټ_Func(m_trigVecDTypep);
        funcp = m_any_سېټ_Vecp;
    } وإلا إذا (vscp->dtypep() == m_trigExtDTypep) {
        إذا (!m_any_سېټ_Extp) m_any_سېټ_Extp = create_Any_سېټ_Func(m_trigExtDTypep);
        funcp = m_any_سېټ_Extp;
    } وإلا {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->جمع_کړه_Argsp(جديد AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_سېټ_بټ();
    أرجع callp;
}
Ast_غوټه_Stmt* TriggerKit::newClearCall(AstVarScope* ثابت vscp) ثابت {
    إذا (!m_n_Vec_ټکي) أرجع تش_اشاره;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    إذا (!m_clearp) m_clearp = createClearFunc();
    File_کرښه* ثابت flp = v3Global.rootp()->سر_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_clearp};
    callp->جمع_کړه_Argsp(جديد AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_سېټ_Void();
    أرجع callp->makeStmt();
}
Ast_غوټه_Stmt* TriggerKit::newOrIntoCall(AstVarScope* ثابت oVscp, AstVarScope* ثابت iVscp) ثابت {
    إذا (!m_n_Vec_ټکي) أرجع تش_اشاره;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    ثابت اندازه_t ماسک
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[ماسک];
    إذا (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_کرښه* ثابت flp = v3Global.rootp()->سر_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->جمع_کړه_Argsp(جديد AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->جمع_کړه_Argsp(جديد AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtype_سېټ_Void();
    أرجع callp->makeStmt();
}

Ast_غوټه_Stmt* TriggerKit::new_Comp_بنسټ_Call() ثابت {
    إذا (!m_n_Vec_ټکي) أرجع تش_اشاره;
    File_کرښه* ثابت flp = v3Global.rootp()->سر_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_compVecp};
    callp->dtype_سېټ_Void();
    أرجع callp->makeStmt();
}

Ast_غوټه_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ثابت {
    إذا (!m_n_Pre_ټکي) أرجع تش_اشاره;
    File_کرښه* ثابت flp = v3Global.rootp()->سر_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_compExtp};
    callp->جمع_کړه_Argsp(جديد AstVarRef{flp, vscp, VAccess::READ});
    callp->dtype_سېټ_Void();
    أرجع callp->makeStmt();
}

Ast_غوټه_Stmt* TriggerKit::newDumpCall(AstVarScope* ثابت vscp, ثابت std::string& نښان,
                                     منطقي debugOnly) ثابت {
    إذا (!m_n_Vec_ټکي) أرجع تش_اشاره;
    AstCFunc* funcp = تش_اشاره;
    إذا (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } وإلا إذا (vscp->dtypep() == m_trigExtDTypep) {
        إذا (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } وإلا {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_کرښه* ثابت flp = v3Global.rootp()->سر_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->جمع_کړه_Argsp(جديد AstVarRef{flp, vscp, VAccess::READ});
    callp->جمع_کړه_Argsp(جديد AstConst{flp, AstConst::String{}, نښان});
    callp->dtype_سېټ_Void();
    AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
    cstmtp->جمع_کړه("#ifdef VL_DEBUG\n");
    إذا (debugOnly) {
        cstmtp->جمع_کړه("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->جمع_کړه(callp->makeStmt());
        cstmtp->جمع_کړه("}\n");
    } وإلا {
        cstmtp->جمع_کړه(callp->makeStmt());
    }
    cstmtp->جمع_کړه("#endif");
    أرجع cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(ثابت std::string& name) ثابت {
    إذا (!m_n_Vec_ټکي) أرجع تش_اشاره;
    أرجع v3Global.rootp()->سر_Scopep()->create_لنډمهاله("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extra_Trigger_شاخص(uint32_t شاخص) ثابت {
    UASSERT(شاخص <= m_n_Extra_ټکي * کلمه_اندازه, "Invalid external trigger index");
    أرجع شاخص + m_n_Sense_ټکي * کلمه_اندازه;
}

std::سېټ<uint32_t> TriggerKit::triggerIndices(ثابت Ast_Sen_ونه* senTreep, منطقي pre) ثابت {
    std::سېټ<uint32_t> indices;
    لكل (ثابت Ast_Sen_توکی* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), Sen_توکی)) {
        uint32_t شاخص = m_senItem2TrigIdx.at(*sip);
        إذا (pre) {
            UASSERT_OBJ(شاخص < m_n_Pre_ټکي * کلمه_اندازه, sip, "Not a 'pre' trigger");
            شاخص += m_n_Vec_ټکي * کلمه_اندازه;
        }
        indices.ورننباسه(شاخص);
    }
    أرجع indices;
}

Ast_غوټه_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t شاخص) ثابت {
    شاخص = extra_Trigger_شاخص(شاخص);
    ثابت uint32_t کلمه_شاخص = شاخص / کلمه_اندازه;
    ثابت uint32_t بټ_شاخص = شاخص % کلمه_اندازه;
    File_کرښه* ثابت flp = vscp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_غوټه_بیان* ثابت wordp = جديد AstArraySel{flp, refp, static_cast<صحيح>(کلمه_شاخص)};
    Ast_غوټه_بیان* ثابت trigLhsp = جديد AstSel{flp, wordp, static_cast<صحيح>(بټ_شاخص), 1};
    Ast_غوټه_بیان* ثابت trigRhsp = جديد AstVarRef{flp, vscp, VAccess::READ};
    أرجع جديد Ast_ټاکل{flp, trigLhsp, trigRhsp};
}

فراغ TriggerKit::جمع_کړه_Extra_Trigger_Assignment(AstVarScope* vscp, uint32_t شاخص) ثابت {
    File_کرښه* ثابت flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_غوټه* ثابت setp = newExtraTriggerAssignment(vscp, شاخص);
    setp->جمع_کړه_بل(جديد Ast_ټاکل{flp, جديد AstVarRef{flp, vscp, VAccess::WRITE},
                                جديد AstConst{flp, AstConst::بټ_False{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
فراغ TriggerKit::جمع_کړه_ارزښت_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t شاخص) ثابت {
    شاخص += m_n_Sense_ټکي * کلمه_اندازه;
    ثابت uint32_t کلمه_شاخص = شاخص / کلمه_اندازه;
    ثابت uint32_t بټ_شاخص = شاخص % کلمه_اندازه;

    Ast_سر_Scope* ثابت سر_Scopep = netlistp->سر_Scopep();
    File_کرښه* ثابت flp = netlistp->fileline();
    Ast_غوټه_DType* ثابت dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    إذا (ثابت AstBasicDType* ثابت bdtypep = VN_CAST(dtypep, BasicDType)) {
        إذا (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            أرجع;
        }
    } وإلا إذا (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, غوټه_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        أرجع;
    }

    ثابت خپلکاری rdInst = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خپلکاری wrPrev = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };
    ثابت خپلکاری rdPrev = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    ثابت std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* ثابت prevVscp = سر_Scopep->create_لنډمهاله(prevName, instVscp->dtypep());

    // Initialize prev = inst
    إذا (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = جديد AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ټاکل, rdInst(instVscp)};
        cmhp->dtype_سېټ_Void();
        initFuncp->جمع_کړه_Stmtsp(cmhp->makeStmt());
    } وإلا {
        initFuncp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_غوټه_بیان* neqp;
    إذا (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp
            = جديد AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtype_سېټ_بټ();
        neqp = cmhp;
    } وإلا {
        neqp = جديد AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_غوټه* updp;
    إذا (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = جديد AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ټاکل, rdInst(instVscp)};
        cmhp->dtype_سېټ_Void();
        updp = cmhp->makeStmt();
    } وإلا {
        updp = جديد Ast_ټاکل{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ثابت refp = جديد AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_غوټه_بیان* ثابت wordp = جديد AstArraySel{flp, refp, static_cast<صحيح>(کلمه_شاخص)};
    Ast_غوټه_بیان* ثابت trigLhsp = جديد AstSel{flp, wordp, static_cast<صحيح>(بټ_شاخص), 1};
    Ast_غوټه* ثابت setp = جديد Ast_ټاکل{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->جمع_کړه_بل(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ثابت std::string& name, منطقي slow, uint32_t n_Sense_ټکي,
                       uint32_t n_Extra_ټکي, uint32_t n_Pre_ټکي,
                       std::unordered_map<VNRef<ثابت Ast_Sen_توکی>, اندازه_t> senItem2TrigIdx,
                       ثابت std::ویکټور<ثابت Ast_Sen_توکی*>& senItemps, منطقي useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_ټکي{n_Sense_ټکي}
    , m_n_Extra_ټکي{n_Extra_ټکي}
    , m_n_Pre_ټکي{n_Pre_ټکي}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trig_Idx2_Sen_توکی{senItemps} {
    // If no triggers, we don't need to generate anything
    إذا (!m_n_Vec_ټکي) أرجع;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trig_Idx2_Sen_توکی.resize((m_n_Vec_ټکي + m_n_Pre_ټکي) * کلمه_اندازه);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_ټکي * کلمه_اندازه,
              m_trig_Idx2_Sen_توکی.begin() + m_n_Vec_ټکي * کلمه_اندازه);
    AstNetlist* ثابت netlistp = v3Global.rootp();
    AstScope* ثابت scopep = netlistp->سر_Scopep()->scopep();
    File_کرښه* ثابت flp = scopep->fileline();
    // Data type of a single trigger word
    m_کلمه_DTypep = netlistp->پیدا_کړه_بټ_DType(کلمه_اندازه, کلمه_اندازه, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ثابت rp = جديد AstRange{flp, static_cast<صحيح>(m_n_Vec_ټکي - 1), 0};
    m_trigVecDTypep = جديد AstUnpackArrayDType{flp, m_کلمه_DTypep, rp};
    netlistp->typeTablep()->جمع_کړه_Typesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    إذا (m_n_Pre_ټکي) {
        AstRange* ثابت ep = جديد AstRange{flp, static_cast<صحيح>(m_n_Vec_ټکي + m_n_Pre_ټکي - 1), 0};
        m_trigExtDTypep = جديد AstUnpackArrayDType{flp, m_کلمه_DTypep, ep};
        netlistp->typeTablep()->جمع_کړه_Typesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } وإلا {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->سر_Scopep()->create_لنډمهاله("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->is_دننی(ریښتیا);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, ریښتیا);
    m_dumpp->isStatic(ریښتیا);
    m_dumpp->ifdef("VL_DEBUG");
    إذا (useAcc) {
        m_vscAccp
            = netlistp->سر_Scopep()->create_لنډمهاله("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->is_دننی(ریښتیا);
    }
}

Ast_ټاکل* TriggerKit::createSenTrigVecAssignment(AstVarScope* ثابت موخه,
                                                  std::ویکټور<Ast_غوټه_بیان*>& trigps) {
    File_کرښه* ثابت flp = موخه->fileline();
    Ast_ټاکل* trigStmtsp = تش_اشاره;
    // Assign sense triggers vector one word at a time
    لكل (اندازه_t i = 0; i < trigps.اندازه(); i += کلمه_اندازه) {
        // Concatenate all bits in this trigger word using a balanced
        لكل (uint32_t level = 0; level < کلمه_اندازه_LOG2; ++level) {
            ثابت uint32_t stride = 1 << level;
            لكل (uint32_t j = 0; j < کلمه_اندازه; j += 2 * stride) {
                trigps[i + j] = جديد AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = تش_اشاره;
            }
        }

        // Set the whole word in the trigger vector
        ثابت صحيح کلمه_شاخص = static_cast<صحيح>(i / کلمه_اندازه);
        AstArraySel* ثابت aselp
            = جديد AstArraySel{flp, جديد AstVarRef{flp, موخه, VAccess::WRITE}, کلمه_شاخص};
        trigStmtsp = Ast_غوټه::جمع_کړه_بل(trigStmtsp, جديد Ast_ټاکل{flp, aselp, trigps[i]});
    }
    أرجع trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ثابت initFuncp,  //
                              Sen_بیان_Builder& sen_بیان_Builder,  //
                              ثابت std::ویکټور<ثابت Ast_Sen_ونه*>& preTreeps,  //
                              ثابت std::ویکټور<ثابت Ast_Sen_ونه*>& senTreeps,  //
                              ثابت string& name,  //
                              ثابت ExtraTriggers& extraTriggers,  //
                              منطقي slow,  //
                              منطقي useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::ویکټور<ثابت Ast_Sen_توکی*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<ثابت Ast_Sen_توکی>, اندازه_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    لكل (ثابت Ast_Sen_ونه* ثابت senTreep : preTreeps) {
        لكل (ثابت Ast_Sen_توکی *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_توکی);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت خپلکاری pair = senItem2TrigIdx.emplace(*itemp, senItemps.اندازه());
            إذا (pair.دوهم) senItemps.ټېل_شا(itemp);
        }
    }
    ثابت uint32_t n_Pre_Sen_توکي = senItemps.اندازه();
    V3Stats::جمع_کړه_Stat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_توکي);
    // Number of pre triggers, rounded up to a full word.
    ثابت uint32_t nPreTriggers = vlstd::پړاو_پورته_منزل_مضرب_د<کلمه_اندازه>(senItemps.اندازه());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ثابت uint32_t n_Pre_ټکي = nPreTriggers / کلمه_اندازه;

    // Process the rest of the trees
    لكل (ثابت Ast_Sen_ونه* ثابت senTreep : senTreeps) {
        لكل (ثابت Ast_Sen_توکی *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_توکی);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت خپلکاری pair = senItem2TrigIdx.emplace(*itemp, senItemps.اندازه());
            إذا (pair.دوهم) senItemps.ټېل_شا(itemp);
        }
    }
    ثابت uint32_t n_Sen_توکي = senItemps.اندازه() - nPreTriggers;
    V3Stats::جمع_کړه_Stat("Scheduling, '" + name + "' sense triggers", n_Sen_توکي + n_Pre_Sen_توکي);
    // Number of sense triggers, rounded up to a full word
    ثابت uint32_t nSenseTriggers = vlstd::پړاو_پورته_منزل_مضرب_د<کلمه_اندازه>(senItemps.اندازه());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ثابت uint32_t n_Sense_ټکي = nSenseTriggers / کلمه_اندازه;

    // Allocate space for the extra triggers
    V3Stats::جمع_کړه_Stat("Scheduling, '" + name + "' extra triggers", extraTriggers.اندازه());
    // Number of extra triggers, rounded up to a full word.
    ثابت uint32_t nExtraTriggers = vlstd::پړاو_پورته_منزل_مضرب_د<کلمه_اندازه>(extraTriggers.اندازه());
    ثابت uint32_t n_Extra_ټکي = nExtraTriggers / کلمه_اندازه;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_ټکي, n_Extra_ټکي, n_Pre_ټکي, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    إذا (!kit.m_n_Vec_ټکي) أرجع kit;

    File_کرښه* ثابت flp = netlistp->سر_Scopep()->fileline();

    // Creates read/write reference
    ثابت خپلکاری rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خپلکاری wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* ثابت dumpTrgp
        = new_دلیل(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* ثابت dumpTagp
        = new_دلیل(kit.m_dumpp, netlistp->پیدا_کړه_String_DType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* ثابت ifp = جديد AstIf{flp, جديد AstLogNot{flp, kit.new_Any_سېټ_Call(dumpTrgp)}};
        kit.m_dumpp->جمع_کړه_Stmtsp(ifp);
        AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
        ifp->جمع_کړه_Thensp(cstmtp);
        cstmtp->جمع_کړه("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->جمع_کړه(rd(dumpTagp));
        cstmtp->جمع_کړه(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    ثابت خپلکاری جمع_کړه_Debug = [&](uint32_t شاخص, ثابت string& متن) {
        ثابت صحيح wrd_شاخص = static_cast<صحيح>(شاخص / کلمه_اندازه);
        ثابت صحيح بټ_شاخص = static_cast<صحيح>(شاخص % کلمه_اندازه);
        Ast_غوټه_بیان* ثابت aselp = جديد AstArraySel{flp, rd(dumpTrgp), wrd_شاخص};
        Ast_غوټه_بیان* ثابت condp = جديد AstSel{flp, aselp, بټ_شاخص, 1};
        AstIf* ثابت ifp = جديد AstIf{flp, condp};
        kit.m_dumpp->جمع_کړه_Stmtsp(ifp);
        AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
        ifp->جمع_کړه_Thensp(cstmtp);
        cstmtp->جمع_کړه("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->جمع_کړه(rd(dumpTagp));
        cstmtp->جمع_کړه(" + \"' region trigger index " + std::منزل_string(شاخص) + " is active: " + متن
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::ویکټور<Ast_غوټه_بیان*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_غوټه_Stmt* initialTrigsp = تش_اشاره;
    لكل (اندازه_t i = 0; i < senItemps.اندازه(); ++i) {
        ثابت Ast_Sen_توکی* ثابت senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        إذا (!senItemp) {
            trigps.emplace_شا(جديد AstConst{flp, AstConst::بټ_False{}});
            استمر;
        }

        // Create the trigger computation expression
        ثابت خپلکاری& pair = sen_بیان_Builder.build(senItemp);
        trigps.emplace_شا(pair.لومړی);

        // Add initialization time trigger
        إذا (pair.دوهم || v3Global.opt.x_Initial_څنډه()) {
            ثابت صحيح wrd_شاخص = static_cast<صحيح>(i / کلمه_اندازه);
            ثابت صحيح بټ_شاخص = static_cast<صحيح>(i % کلمه_اندازه);
            Ast_غوټه_بیان* ثابت wordp = جديد AstArraySel{flp, wr(kit.m_vscp), wrd_شاخص};
            Ast_غوټه_بیان* ثابت lhsp = جديد AstSel{flp, wordp, بټ_شاخص, 1};
            Ast_غوټه_بیان* ثابت rhsp = جديد AstConst{flp, AstConst::بټ_True{}};
            إذا (useAcc) {
                initFuncp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, lhsp, rhsp});
            } وإلا {
                initialTrigsp = Ast_غوټه::جمع_کړه_بل(initialTrigsp, جديد Ast_ټاکل{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_ونه(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        جمع_کړه_Debug(i, desc);
    }
    UASSERT(trigps.اندازه() == nSenseTriggers, "Inconsistent number of trigger expressions");

    Ast_ټاکل* ثابت trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    لكل (بینښه i = 0; i < extraTriggers.اندازه(); ++i) {
        جمع_کړه_Debug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    ثابت Sen_بیان_Builder::Results senResults = sen_بیان_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    لكل (Ast_غوټه_Stmt* ثابت nodep : senResults.m_inits) initFuncp->جمع_کړه_Stmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_سر_Scope* ثابت سر_Scopep = netlistp->سر_Scopep();
    {
        AstCFunc* ثابت fp = kit.m_compVecp;
        // Trigger computation
        لكل (Ast_غوټه_Stmt* ثابت nodep : senResults.m_preUpdates) fp->جمع_کړه_Stmtsp(nodep);
        fp->جمع_کړه_Stmtsp(trigStmtsp);
        لكل (Ast_غوټه_Stmt* ثابت nodep : senResults.m_postUpdates) fp->جمع_کړه_Stmtsp(nodep);
        // Add the initialization time triggers
        إذا (initialTrigsp) {
            AstVarScope* ثابت initVscp = سر_Scopep->create_لنډمهاله("__V" + name + "DidInit", 1);
            AstIf* ثابت ifp = جديد AstIf{flp, جديد AstNot{flp, rd(initVscp)}};
            fp->جمع_کړه_Stmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->جمع_کړه_Thensp(util::سېټ_Var(initVscp, 1));
            ifp->جمع_کړه_Thensp(initialTrigsp);
        }
        util::وېشل_Check(fp);
    };
    // If there are 'pre' triggers, compute them
    إذا (kit.m_n_Pre_ټکي) {
        AstCFunc* ثابت fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ثابت latchedp
            = new_دلیل(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ثابت nVscp = سر_Scopep->create_لنډمهاله("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(ریښتیا);
        // Add a loop to compute the pre words
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        fp->جمع_کړه_Stmtsp(util::سېټ_Var(nVscp, 0));
        fp->جمع_کړه_Stmtsp(loopp);
        // Loop body
        Ast_غوټه_بیان* ثابت offsetp = جديد AstConst{flp, kit.m_n_Vec_ټکي};
        Ast_غوټه_بیان* ثابت lIdxp = جديد Ast_جمع_کړه{flp, rd(nVscp), offsetp};
        Ast_غوټه_بیان* ثابت lhsp = جديد AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_غوټه_بیان* ثابت aWordp = جديد AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_غوټه_بیان* ثابت bWordp = جديد AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_غوټه_بیان* ثابت rhsp = جديد AstAnd{flp, aWordp, جديد AstNot{flp, bWordp}};
        Ast_غوټه_بیان* ثابت limp = جديد AstConst{flp, AstConst::Widthed_ارزښت{}, 32, n_Pre_ټکي};
        loopp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, lhsp, rhsp});
        loopp->جمع_کړه_Stmtsp(util::incrementVar(nVscp));
        loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});
        util::وېشل_Check(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    إذا (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlink_Fr_شا_With_بل()->delete_ونه();
    // Done with the trigger dump function, split as might be large
    util::وېشل_Check(kit.m_dumpp);

    أرجع kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
صنف AwaitBeforeTrigVisitor final : عام VNVisitor {
    ثابت VNUser1_دننه_Use m_user1_دننه_Use;

    /**
     * AstCAwait::user1()       ->  bool.           True if node has been visited
     * AstSenTree::user1p()     ->  AstCFunc*.      Function that has to be called before awaiting
     *                                              for CAwait pointing to this SenTree
     * AstCFunc::user1p()       ->  AstVarScope*    Function's local temporary extended trigger
     *                                              vector variable scope
     */

    // Netlist - needed for using util::makeSubFunction()
    AstNetlist* ثابت m_netlistp;
    // Trigger kit - for accessing trigger vectors and mapping senItems to thier indexes
    ثابت TriggerKit& m_trigKit;
    // Expression builder - for building expressions from SenItems
    Sen_بیان_Builder& m_sen_بیان_Builder;
    // Generator of unique names for before-trigger function
    V3_Unique_نومونه m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::ویکټور<std::pair<AstCFunc*, Ast_Sen_ونه*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::ویکټور<std::pair<Ast_Sen_ونه*, Ast_غوټه_بیان*>> m_sen_ونه_منزل_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_غوټه>, std::ویکټور<Ast_Sen_توکی*>> m_sen_بیان_منزل_Sen_توکی;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    ساكن Ast_غوټه* get_Sen_Hash_غوټه(ثابت Ast_Sen_توکی* ثابت nodep) {
        إذا (AstVarRef* ثابت varRefp = VN_CAST(nodep->sensp(), VarRef)) أرجع varRefp;
        أرجع nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    فراغ fill_Sen_بیان_منزل_Sen_توکی() {
        لكل (خپلکاری sen_ونه_Sched : m_sen_ونه_منزل_Sched) {
            Ast_Sen_ونه* ثابت senTreep = sen_ونه_Sched.لومړی;

            لكل (Ast_Sen_توکی* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_توکی)) {
                ثابت VEdgeType څنډه = senItemp->څنډه_Type();
                إذا (څنډه.an_څنډه() || څنډه == VEdgeType::ET_بدل_شوی
                    || څنډه == VEdgeType::ET_HYBRID) {
                    m_sen_بیان_منزل_Sen_توکی[*get_Sen_Hash_غوټه(senItemp)].ټېل_شا(senItemp);
                }
            }
        }

        std::ویکټور<VNRef<Ast_غوټه>> منزل_Remove;
        لكل (ثابت خپلکاری& sen_بیان_منزل_Sen_ونه : m_sen_بیان_منزل_Sen_توکی) {
            std::ویکټور<Ast_Sen_توکی*> senItemps = sen_بیان_منزل_Sen_ونه.دوهم;
            منزل_Remove.ټېل_شا(sen_بیان_منزل_Sen_ونه.لومړی);
            لكل (اندازه_t i = 1; i < senItemps.اندازه(); ++i) {
                إذا (senItemps[i]->څنډه_Type() != senItemps[i - 1]->څنډه_Type()) {
                    منزل_Remove.راوباسه_شا();
                    اكسر;
                }
            }
        }
        لكل (ثابت VNRef<Ast_غوټه> it : منزل_Remove) m_sen_بیان_منزل_Sen_توکی.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<اندازه_t, std::map<اندازه_t, std::ویکټور<Ast_غوټه_بیان*>>>
    get_کارول_شوی_Triggers_منزل_Trees(ثابت std::سېټ<اندازه_t>& کارول_شوی_Triggers) {
        std::map<اندازه_t, std::map<اندازه_t, std::ویکټور<Ast_غوټه_بیان*>>> کارول_شوی_Trigs_منزل_Using_Trees;
        لكل (خپلکاری sen_ونه_Sched : m_sen_ونه_منزل_Sched) {
            ثابت Ast_Sen_ونه* ثابت senTreep = sen_ونه_Sched.لومړی;
            Ast_غوټه_بیان* ثابت shedp = sen_ونه_Sched.دوهم;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            لكل (Ast_Sen_توکی* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_توکی)) {
                ثابت اندازه_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                إذا (کارول_شوی_Triggers.پیدا_کړه(idx) != کارول_شوی_Triggers.پای_ټکی()) {
                    کارول_شوی_Trigs_منزل_Using_Trees[idx / TriggerKit::کلمه_اندازه]
                                         [اندازه_t{1} << (idx % TriggerKit::کلمه_اندازه)]
                                             .ټېل_شا(shedp);
                }
            }
        }
        إذا (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            لكل (ثابت خپلکاری& triggers_منزل_Trees : کارول_شوی_Trigs_منزل_Using_Trees) {
                لكل (ثابت خپلکاری& بټونه_منزل_Trees : triggers_منزل_Trees.دوهم) {
                    ثابت std::سېټ<ثابت Ast_غوټه_بیان*> exprps{بټونه_منزل_Trees.دوهم.begin(),
                                                              بټونه_منزل_Trees.دوهم.پای_ټکی()};
                    UASSERT(بټونه_منزل_Trees.دوهم.اندازه() == exprps.اندازه(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        أرجع کارول_شوی_Trigs_منزل_Using_Trees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_ونه* ثابت senTreep) {
        File_کرښه* ثابت flp = senTreep->fileline();
        إذا (!senTreep->user1p()) {
            AstCFunc* ثابت funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), خطأ);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* ثابت tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(ریښتیا);

            AstVar* ثابت argp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(ریښتیا);
            argp->direction(VDirection::ننوت);
            funcp->جمع_کړه_Argsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_شا(funcp, senTreep);
        }
        AstCCall* ثابت callp = جديد AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtype_سېټ_Void();
        أرجع callp;
    }

    فراغ لیدنه(AstCAwait* ثابت nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        إذا (ثابت AstCMethodHard* ثابت c_لاره_Hardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            إذا (c_لاره_Hardp->لاره() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ثابت beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_غوټه* ثابت pinp = c_لاره_Hardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, c_لاره_Hardp, "No event description");
                beforeTrigp->جمع_کړه_Argsp(VN_AS(pinp, غوټه_بیان)->clone_ونه(خطأ));

                // Call the before-trigger function before the CAwait
                nodep->جمع_کړه_دلته_This_As_بل(beforeTrigp->makeStmt());
                m_sen_ونه_منزل_Sched.emplace_شا(nodep->sentreep(), c_لاره_Hardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    فراغ لیدنه(Ast_غوټه* ثابت nodep) override { iterateChildren(nodep); }

عام:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_بیان_Builder& sen_بیان_Builder,
                           ثابت TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_بیان_Builder{sen_بیان_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_بیان_منزل_Sen_توکی();

        std::ویکټور<Ast_غوټه_بیان*> trigps;
        std::سېټ<اندازه_t> کارول_شوی_Triggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        لكل (ثابت خپلکاری& func_منزل_کارول_شوی_Triggers : m_generatedFuncs) {
            AstCFunc* ثابت funcp = func_منزل_کارول_شوی_Triggers.لومړی;
            AstVarScope* ثابت vscp = VN_AS(funcp->user1p(), VarScope);
            File_کرښه* ثابت flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_ونه* ثابت senTreep = func_منزل_کارول_شوی_Triggers.دوهم;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ثابت خپلکاری emplaceAt
                    = [flp, &trigps, &کارول_شوی_Triggers](Ast_غوټه_بیان* ثابت exprp, ثابت اندازه_t pos) {
                          ثابت اندازه_t موخه_اندازه
                              = vlstd::پړاو_پورته_منزل_مضرب_د<TriggerKit::کلمه_اندازه>(pos + 1);
                          إذا (trigps.ظرفیت() < موخه_اندازه) trigps.reserve(موخه_اندازه * 2);
                          طالما (trigps.اندازه() < موخه_اندازه) {
                              trigps.ټېل_شا(جديد AstConst{flp, AstConst::بټ_False{}});
                          }
                          trigps[pos]->delete_ونه();
                          trigps[pos] = exprp;
                          کارول_شوی_Triggers.ورننباسه(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                لكل (ثابت Ast_Sen_توکی* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Sen_توکی)) {
                    ثابت اندازه_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_بیان_Builder.build(itemp).لومړی, idx);
                    خپلکاری iter = m_sen_بیان_منزل_Sen_توکی.پیدا_کړه(*get_Sen_Hash_غوټه(itemp));
                    إذا (iter != m_sen_بیان_منزل_Sen_توکی.پای_ټکی()) {
                        لكل (Ast_Sen_توکی* ثابت additionalItemp : iter->دوهم) {
                            ثابت اندازه_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_بیان_Builder.build(additionalItemp).لومړی, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ثابت Sen_بیان_Builder::Results results
                    = m_sen_بیان_Builder.getResultsAndClearUpdates();
                لكل (Ast_غوټه_Stmt* ثابت stmtsp : results.m_inits) funcp->جمع_کړه_Stmtsp(stmtsp);
                لكل (Ast_غوټه_Stmt* ثابت stmtsp : results.m_preUpdates) funcp->جمع_کړه_Stmtsp(stmtsp);
                funcp->جمع_کړه_Stmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                لكل (Ast_غوټه_Stmt* ثابت stmtsp : results.m_postUpdates) funcp->جمع_کړه_Stmtsp(stmtsp);
            }

            ثابت std::map<اندازه_t, std::map<اندازه_t, std::ویکټور<Ast_غوټه_بیان*>>>
                کارول_شوی_Trigs_منزل_Using_Trees = get_کارول_شوی_Triggers_منزل_Trees(کارول_شوی_Triggers);
            کارول_شوی_Triggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            ثابت خپلکاری getIdx = [flp](AstVarScope* ثابت scocep, VAccess access, اندازه_t idx) {
                أرجع جديد AstArraySel{flp, جديد AstVarRef{flp, scocep, access},
                                       جديد AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* ثابت argpVscp = جديد AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->جمع_کړه_Varsp(argpVscp);

            // Mark as ready triggered schedulers
            لكل (ثابت خپلکاری& triggers_منزل_Trees : کارول_شوی_Trigs_منزل_Using_Trees) {
                ثابت اندازه_t کلمه = triggers_منزل_Trees.لومړی;

                لكل (ثابت خپلکاری& بټونه_منزل_Trees : triggers_منزل_Trees.دوهم) {
                    ثابت اندازه_t بټ_2 = بټونه_منزل_Trees.لومړی;
                    ثابت خپلکاری& schedulers = بټونه_منزل_Trees.دوهم;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* ثابت ماسک_Constp = جديد AstConst{flp, AstConst::Unsized64{}, بټ_2};
                    AstAnd* ثابت condp
                        = جديد AstAnd{flp, getIdx(vscp, VAccess::READ, کلمه), ماسک_Constp};
                    AstIf* ثابت ifp = جديد AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    لكل (Ast_غوټه_بیان* ثابت schedp : schedulers) {
                        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                            flp, schedp->clone_ونه(خطأ), VCMethod::SCHED_READY};
                        callp->dtype_سېټ_Void();
                        callp->جمع_کړه_Pinsp(جديد AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->جمع_کړه_Thensp(callp->makeStmt());
                    }
                    funcp->جمع_کړه_Stmtsp(ifp);
                }
            }

            AstVarScope* ثابت vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            لكل (ثابت خپلکاری& triggers_منزل_Trees : کارول_شوی_Trigs_منزل_Using_Trees) {
                ثابت اندازه_t کلمه = triggers_منزل_Trees.لومړی;
                funcp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, getIdx(vscAccp, VAccess::WRITE, کلمه),
                                               جديد AstOr{flp, getIdx(vscAccp, VAccess::READ, کلمه),
                                                         getIdx(vscp, VAccess::READ, کلمه)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = افتراضي;
};

فراغ beforeTrigVisitor(AstNetlist* netlistp, Sen_بیان_Builder& sen_بیان_Builder,
                       ثابت TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_بیان_Builder, trigKit};
}

}  // namespace V3Sched
