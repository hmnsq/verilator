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

استعمال util::new_دليل;

AstVarScope* newLocal(AstCFunc* funcp, Ast_ڳنڍ_DType* dtypep, ثابت std::string& name) {
    File_سٽ* ثابت flp = funcp->fileline();
    AstScope* ثابت scopep = funcp->scopep();
    AstVar* ثابت varp = جديد AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(سچ);
    varp->noReset(سچ);
    funcp->جوڙيو_Varsp(varp);
    AstVarScope* ثابت vscp = جديد AstVarScope{flp, scopep, varp};
    scopep->جوڙيو_Varsp(vscp);
    أرجع vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() ثابت {
    UASSERT(m_n_Pre_لفظ, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سٽ* ثابت flp = netlistp->چوٽي_Scopep()->fileline();
    Ast_ڳنڍ_DType* ثابت u32DTypep = netlistp->ڳوليو_UInt32_DType();
    Ast_ڳنڍ_DType* ثابت strDtypep = netlistp->ڳوليو_String_DType();

    // Dumping function always slow
    ثابت std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, سچ);
    funcp->isStatic(سچ);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* ثابت eVscp = new_دليل(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* ثابت tVscp = new_دليل(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    ثابت خودڪار rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودڪار wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* ثابت vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* ثابت iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->جوڙيو_Stmtsp(util::سيٽ_Var(iVscp, 0));
        // Add loop
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        funcp->جوڙيو_Stmtsp(loopp);
        // Loop body
        Ast_ڳنڍ_اظهار* ثابت lhsp = جديد AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_ڳنڍ_اظهار* ثابت rhsp = جديد AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_ڳنڍ_اظهار* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, m_n_Vec_لفظ};
        loopp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, lhsp, rhsp});
        loopp->جوڙيو_Stmtsp(util::incrementVar(iVscp));
        loopp->جوڙيو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* ثابت callp = جديد AstCCall{flp, m_dumpp};
        callp->dtype_سيٽ_Void();
        callp->جوڙيو_Argsp(rd(vVscp));
        callp->جوڙيو_Argsp(rd(tVscp));
        funcp->جوڙيو_Stmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* ثابت pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* ثابت jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->جوڙيو_Stmtsp(util::سيٽ_Var(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            funcp->جوڙيو_Stmtsp(loopp);
            // Loop body
            Ast_ڳنڍ_اظهار* ثابت lhsp = جديد AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ڳنڍ_اظهار* ثابت rhsp = جديد AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_ڳنڍ_اظهار* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, m_n_Pre_لفظ};
            loopp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, lhsp, rhsp});
            loopp->جوڙيو_Stmtsp(util::incrementVar(jVscp));
            loopp->جوڙيو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            funcp->جوڙيو_Stmtsp(loopp);
            // Loop body
            Ast_ڳنڍ_اظهار* ثابت lhsp = جديد AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ڳنڍ_اظهار* ثابت rhsp = جديد AstConst{flp, AstConst::DTyped{}, m_لفظ_DTypep};
            Ast_ڳنڍ_اظهار* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, m_n_Vec_لفظ};
            loopp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, lhsp, rhsp});
            loopp->جوڙيو_Stmtsp(util::incrementVar(jVscp));
            loopp->جوڙيو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* ثابت callp = جديد AstCCall{flp, m_dumpp};
        callp->dtype_سيٽ_Void();
        callp->جوڙيو_Argsp(rd(pVscp));
        callp->جوڙيو_Argsp(
            جديد AstConcatN{flp, rd(tVscp), جديد AstConst{flp, AstConst::String{}, " pre"}});
        funcp->جوڙيو_Stmtsp(callp->makeStmt());
    }

    أرجع funcp;
}

AstCFunc* TriggerKit::create_Any_سيٽ_Func(AstUnpackArrayDType* ثابت dtypep) ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سٽ* ثابت flp = netlistp->چوٽي_Scopep()->fileline();
    Ast_ڳنڍ_DType* ثابت u32DTypep = netlistp->ڳوليو_UInt32_DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(سچ);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* ثابت iVscp = new_دليل(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    ثابت خودڪار rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* ثابت loopp = جديد AstLoop{flp};
    funcp->جوڙيو_Stmtsp(util::سيٽ_Var(nVscp, 0));
    funcp->جوڙيو_Stmtsp(loopp);
    funcp->جوڙيو_Stmtsp(جديد AstCReturn{flp, جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}}});

    // Loop body
    ثابت uint32_t n_لفظ = dtypep->elementsConst();
    Ast_ڳنڍ_اظهار* ثابت condp = جديد AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ڳنڍ_Stmt* ثابت thenp = جديد AstCReturn{flp, جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}}};
    Ast_ڳنڍ_اظهار* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, n_لفظ};
    loopp->جوڙيو_Stmtsp(جديد AstIf{flp, condp, thenp});
    loopp->جوڙيو_Stmtsp(util::incrementVar(nVscp));
    loopp->جوڙيو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});

    //پورو
    أرجع funcp;
}
AstCFunc* TriggerKit::createClearFunc() ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سٽ* ثابت flp = netlistp->چوٽي_Scopep()->fileline();
    Ast_ڳنڍ_DType* ثابت u32DTypep = netlistp->ڳوليو_UInt32_DType();

    // Create function
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(سچ);

    // Add arguments
    AstVarScope* ثابت oVscp = new_دليل(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ثابت خودڪار rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودڪار wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ثابت loopp = جديد AstLoop{flp};
    funcp->جوڙيو_Stmtsp(util::سيٽ_Var(nVscp, 0));
    funcp->جوڙيو_Stmtsp(loopp);

    // Loop body
    Ast_ڳنڍ_اظهار* ثابت lhsp = جديد AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ڳنڍ_اظهار* ثابت rhsp = جديد AstConst{flp, AstConst::DTyped{}, m_لفظ_DTypep};
    Ast_ڳنڍ_اظهار* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, m_n_Vec_لفظ};
    loopp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, lhsp, rhsp});
    loopp->جوڙيو_Stmtsp(util::incrementVar(nVscp));
    loopp->جوڙيو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});

    //پورو
    أرجع funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ثابت oDtypep,
                                       AstUnpackArrayDType* ثابت iDtypep) ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سٽ* ثابت flp = netlistp->چوٽي_Scopep()->fileline();
    Ast_ڳنڍ_DType* ثابت u32DTypep = netlistp->ڳوليو_UInt32_DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(سچ);

    // Add arguments
    AstVarScope* ثابت oVscp = new_دليل(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* ثابت iVscp = new_دليل(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ثابت خودڪار rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودڪار wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ثابت loopp = جديد AstLoop{flp};
    funcp->جوڙيو_Stmtsp(util::سيٽ_Var(nVscp, 0));
    funcp->جوڙيو_Stmtsp(loopp);

    // Loop body
    Ast_ڳنڍ_اظهار* ثابت lhsp = جديد AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ڳنڍ_اظهار* ثابت oWordp = جديد AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_ڳنڍ_اظهار* ثابت iWordp = جديد AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ڳنڍ_اظهار* ثابت rhsp = جديد AstOr{flp, oWordp, iWordp};
    AstConst* ثابت outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ثابت داخلا_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_ڳنڍ_اظهار* ثابت limp = outputRangeLeftp->انگ().منزل_SInt() < داخلا_Range_Leftp->انگ().منزل_SInt()
                                  ? outputRangeLeftp->clone_وڻ_خالص(خطأ)
                                  : داخلا_Range_Leftp->clone_وڻ_خالص(خطأ);
    loopp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, lhsp, rhsp});
    loopp->جوڙيو_Stmtsp(util::incrementVar(nVscp));
    loopp->جوڙيو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLte{flp, rd(nVscp), limp}});

    //پورو
    أرجع funcp;
}

Ast_ڳنڍ_اظهار* TriggerKit::new_Any_سيٽ_Call(AstVarScope* ثابت vscp) ثابت {
    File_سٽ* ثابت flp = v3Global.rootp()->چوٽي_Scopep()->fileline();
    إذا (!m_n_Vec_لفظ) أرجع جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}};

    AstCFunc* funcp = خالي_اشارو;
    إذا (vscp->dtypep() == m_trigVecDTypep) {
        إذا (!m_any_سيٽ_Vecp) m_any_سيٽ_Vecp = create_Any_سيٽ_Func(m_trigVecDTypep);
        funcp = m_any_سيٽ_Vecp;
    } وإلا إذا (vscp->dtypep() == m_trigExtDTypep) {
        إذا (!m_any_سيٽ_Extp) m_any_سيٽ_Extp = create_Any_سيٽ_Func(m_trigExtDTypep);
        funcp = m_any_سيٽ_Extp;
    } وإلا {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->جوڙيو_Argsp(جديد AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_سيٽ_ٻٽو_انگ();
    أرجع callp;
}
Ast_ڳنڍ_Stmt* TriggerKit::newClearCall(AstVarScope* ثابت vscp) ثابت {
    إذا (!m_n_Vec_لفظ) أرجع خالي_اشارو;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    إذا (!m_clearp) m_clearp = createClearFunc();
    File_سٽ* ثابت flp = v3Global.rootp()->چوٽي_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_clearp};
    callp->جوڙيو_Argsp(جديد AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_سيٽ_Void();
    أرجع callp->makeStmt();
}
Ast_ڳنڍ_Stmt* TriggerKit::newOrIntoCall(AstVarScope* ثابت oVscp, AstVarScope* ثابت iVscp) ثابت {
    إذا (!m_n_Vec_لفظ) أرجع خالي_اشارو;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    ثابت سائيز_t نقاب
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[نقاب];
    إذا (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_سٽ* ثابت flp = v3Global.rootp()->چوٽي_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->جوڙيو_Argsp(جديد AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->جوڙيو_Argsp(جديد AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtype_سيٽ_Void();
    أرجع callp->makeStmt();
}

Ast_ڳنڍ_Stmt* TriggerKit::new_Comp_بنياد_Call() ثابت {
    إذا (!m_n_Vec_لفظ) أرجع خالي_اشارو;
    File_سٽ* ثابت flp = v3Global.rootp()->چوٽي_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_compVecp};
    callp->dtype_سيٽ_Void();
    أرجع callp->makeStmt();
}

Ast_ڳنڍ_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ثابت {
    إذا (!m_n_Pre_لفظ) أرجع خالي_اشارو;
    File_سٽ* ثابت flp = v3Global.rootp()->چوٽي_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_compExtp};
    callp->جوڙيو_Argsp(جديد AstVarRef{flp, vscp, VAccess::READ});
    callp->dtype_سيٽ_Void();
    أرجع callp->makeStmt();
}

Ast_ڳنڍ_Stmt* TriggerKit::newDumpCall(AstVarScope* ثابت vscp, ثابت std::string& ٽيگ,
                                     منطقي debugOnly) ثابت {
    إذا (!m_n_Vec_لفظ) أرجع خالي_اشارو;
    AstCFunc* funcp = خالي_اشارو;
    إذا (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } وإلا إذا (vscp->dtypep() == m_trigExtDTypep) {
        إذا (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } وإلا {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_سٽ* ثابت flp = v3Global.rootp()->چوٽي_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->جوڙيو_Argsp(جديد AstVarRef{flp, vscp, VAccess::READ});
    callp->جوڙيو_Argsp(جديد AstConst{flp, AstConst::String{}, ٽيگ});
    callp->dtype_سيٽ_Void();
    AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
    cstmtp->جوڙيو("#ifdef VL_DEBUG\n");
    إذا (debugOnly) {
        cstmtp->جوڙيو("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->جوڙيو(callp->makeStmt());
        cstmtp->جوڙيو("}\n");
    } وإلا {
        cstmtp->جوڙيو(callp->makeStmt());
    }
    cstmtp->جوڙيو("#endif");
    أرجع cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(ثابت std::string& name) ثابت {
    إذا (!m_n_Vec_لفظ) أرجع خالي_اشارو;
    أرجع v3Global.rootp()->چوٽي_Scopep()->create_عارضي("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extra_Trigger_اشاريو(uint32_t اشاريو) ثابت {
    UASSERT(اشاريو <= m_n_Extra_لفظ * لفظ_سائيز, "Invalid external trigger index");
    أرجع اشاريو + m_n_Sense_لفظ * لفظ_سائيز;
}

std::سيٽ<uint32_t> TriggerKit::triggerIndices(ثابت Ast_Sen_وڻ* senTreep, منطقي pre) ثابت {
    std::سيٽ<uint32_t> indices;
    لكل (ثابت Ast_Sen_شي* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), Sen_شي)) {
        uint32_t اشاريو = m_senItem2TrigIdx.at(*sip);
        إذا (pre) {
            UASSERT_OBJ(اشاريو < m_n_Pre_لفظ * لفظ_سائيز, sip, "Not a 'pre' trigger");
            اشاريو += m_n_Vec_لفظ * لفظ_سائيز;
        }
        indices.وجهو(اشاريو);
    }
    أرجع indices;
}

Ast_ڳنڍ_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t اشاريو) ثابت {
    اشاريو = extra_Trigger_اشاريو(اشاريو);
    ثابت uint32_t لفظ_اشاريو = اشاريو / لفظ_سائيز;
    ثابت uint32_t ٻٽو_انگ_اشاريو = اشاريو % لفظ_سائيز;
    File_سٽ* ثابت flp = vscp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ڳنڍ_اظهار* ثابت wordp = جديد AstArraySel{flp, refp, static_cast<صحيح>(لفظ_اشاريو)};
    Ast_ڳنڍ_اظهار* ثابت trigLhsp = جديد AstSel{flp, wordp, static_cast<صحيح>(ٻٽو_انگ_اشاريو), 1};
    Ast_ڳنڍ_اظهار* ثابت trigRhsp = جديد AstVarRef{flp, vscp, VAccess::READ};
    أرجع جديد Ast_سونپيو{flp, trigLhsp, trigRhsp};
}

فراغ TriggerKit::جوڙيو_Extra_Trigger_Assignment(AstVarScope* vscp, uint32_t اشاريو) ثابت {
    File_سٽ* ثابت flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_ڳنڍ* ثابت setp = newExtraTriggerAssignment(vscp, اشاريو);
    setp->جوڙيو_ايندڙ(جديد Ast_سونپيو{flp, جديد AstVarRef{flp, vscp, VAccess::WRITE},
                                جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
فراغ TriggerKit::جوڙيو_قدر_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t اشاريو) ثابت {
    اشاريو += m_n_Sense_لفظ * لفظ_سائيز;
    ثابت uint32_t لفظ_اشاريو = اشاريو / لفظ_سائيز;
    ثابت uint32_t ٻٽو_انگ_اشاريو = اشاريو % لفظ_سائيز;

    Ast_چوٽي_Scope* ثابت چوٽي_Scopep = netlistp->چوٽي_Scopep();
    File_سٽ* ثابت flp = netlistp->fileline();
    Ast_ڳنڍ_DType* ثابت dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    إذا (ثابت AstBasicDType* ثابت bdtypep = VN_CAST(dtypep, BasicDType)) {
        إذا (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            أرجع;
        }
    } وإلا إذا (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, ڳنڍ_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        أرجع;
    }

    ثابت خودڪار rdInst = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودڪار wrPrev = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };
    ثابت خودڪار rdPrev = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    ثابت std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* ثابت prevVscp = چوٽي_Scopep->create_عارضي(prevName, instVscp->dtypep());

    // Initialize prev = inst
    إذا (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = جديد AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_سونپيو, rdInst(instVscp)};
        cmhp->dtype_سيٽ_Void();
        initFuncp->جوڙيو_Stmtsp(cmhp->makeStmt());
    } وإلا {
        initFuncp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_ڳنڍ_اظهار* neqp;
    إذا (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp
            = جديد AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtype_سيٽ_ٻٽو_انگ();
        neqp = cmhp;
    } وإلا {
        neqp = جديد AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_ڳنڍ* updp;
    إذا (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = جديد AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_سونپيو, rdInst(instVscp)};
        cmhp->dtype_سيٽ_Void();
        updp = cmhp->makeStmt();
    } وإلا {
        updp = جديد Ast_سونپيو{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ثابت refp = جديد AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ڳنڍ_اظهار* ثابت wordp = جديد AstArraySel{flp, refp, static_cast<صحيح>(لفظ_اشاريو)};
    Ast_ڳنڍ_اظهار* ثابت trigLhsp = جديد AstSel{flp, wordp, static_cast<صحيح>(ٻٽو_انگ_اشاريو), 1};
    Ast_ڳنڍ* ثابت setp = جديد Ast_سونپيو{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->جوڙيو_ايندڙ(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ثابت std::string& name, منطقي slow, uint32_t n_Sense_لفظ,
                       uint32_t n_Extra_لفظ, uint32_t n_Pre_لفظ,
                       std::unordered_map<VNRef<ثابت Ast_Sen_شي>, سائيز_t> senItem2TrigIdx,
                       ثابت std::ويڪٽر<ثابت Ast_Sen_شي*>& senItemps, منطقي useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_لفظ{n_Sense_لفظ}
    , m_n_Extra_لفظ{n_Extra_لفظ}
    , m_n_Pre_لفظ{n_Pre_لفظ}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trig_Idx2_Sen_شي{senItemps} {
    // If no triggers, we don't need to generate anything
    إذا (!m_n_Vec_لفظ) أرجع;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trig_Idx2_Sen_شي.resize((m_n_Vec_لفظ + m_n_Pre_لفظ) * لفظ_سائيز);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_لفظ * لفظ_سائيز,
              m_trig_Idx2_Sen_شي.begin() + m_n_Vec_لفظ * لفظ_سائيز);
    AstNetlist* ثابت netlistp = v3Global.rootp();
    AstScope* ثابت scopep = netlistp->چوٽي_Scopep()->scopep();
    File_سٽ* ثابت flp = scopep->fileline();
    // Data type of a single trigger word
    m_لفظ_DTypep = netlistp->ڳوليو_ٻٽو_انگ_DType(لفظ_سائيز, لفظ_سائيز, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ثابت rp = جديد AstRange{flp, static_cast<صحيح>(m_n_Vec_لفظ - 1), 0};
    m_trigVecDTypep = جديد AstUnpackArrayDType{flp, m_لفظ_DTypep, rp};
    netlistp->typeTablep()->جوڙيو_Typesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    إذا (m_n_Pre_لفظ) {
        AstRange* ثابت ep = جديد AstRange{flp, static_cast<صحيح>(m_n_Vec_لفظ + m_n_Pre_لفظ - 1), 0};
        m_trigExtDTypep = جديد AstUnpackArrayDType{flp, m_لفظ_DTypep, ep};
        netlistp->typeTablep()->جوڙيو_Typesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } وإلا {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->چوٽي_Scopep()->create_عارضي("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->is_اندروني(سچ);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, سچ);
    m_dumpp->isStatic(سچ);
    m_dumpp->ifdef("VL_DEBUG");
    إذا (useAcc) {
        m_vscAccp
            = netlistp->چوٽي_Scopep()->create_عارضي("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->is_اندروني(سچ);
    }
}

Ast_سونپيو* TriggerKit::createSenTrigVecAssignment(AstVarScope* ثابت هدف,
                                                  std::ويڪٽر<Ast_ڳنڍ_اظهار*>& trigps) {
    File_سٽ* ثابت flp = هدف->fileline();
    Ast_سونپيو* trigStmtsp = خالي_اشارو;
    // Assign sense triggers vector one word at a time
    لكل (سائيز_t i = 0; i < trigps.سائيز(); i += لفظ_سائيز) {
        // Concatenate all bits in this trigger word using a balanced
        لكل (uint32_t level = 0; level < لفظ_سائيز_LOG2; ++level) {
            ثابت uint32_t stride = 1 << level;
            لكل (uint32_t j = 0; j < لفظ_سائيز; j += 2 * stride) {
                trigps[i + j] = جديد AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = خالي_اشارو;
            }
        }

        // Set the whole word in the trigger vector
        ثابت صحيح لفظ_اشاريو = static_cast<صحيح>(i / لفظ_سائيز);
        AstArraySel* ثابت aselp
            = جديد AstArraySel{flp, جديد AstVarRef{flp, هدف, VAccess::WRITE}, لفظ_اشاريو};
        trigStmtsp = Ast_ڳنڍ::جوڙيو_ايندڙ(trigStmtsp, جديد Ast_سونپيو{flp, aselp, trigps[i]});
    }
    أرجع trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ثابت initFuncp,  //
                              Sen_اظهار_Builder& sen_اظهار_Builder,  //
                              ثابت std::ويڪٽر<ثابت Ast_Sen_وڻ*>& preTreeps,  //
                              ثابت std::ويڪٽر<ثابت Ast_Sen_وڻ*>& senTreeps,  //
                              ثابت string& name,  //
                              ثابت ExtraTriggers& extraTriggers,  //
                              منطقي slow,  //
                              منطقي useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::ويڪٽر<ثابت Ast_Sen_شي*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<ثابت Ast_Sen_شي>, سائيز_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    لكل (ثابت Ast_Sen_وڻ* ثابت senTreep : preTreeps) {
        لكل (ثابت Ast_Sen_شي *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_شي);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت خودڪار pair = senItem2TrigIdx.emplace(*itemp, senItemps.سائيز());
            إذا (pair.ٻيو) senItemps.ڌڪو_پٺ(itemp);
        }
    }
    ثابت uint32_t n_Pre_Sen_شيون = senItemps.سائيز();
    V3Stats::جوڙيو_Stat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_شيون);
    // Number of pre triggers, rounded up to a full word.
    ثابت uint32_t nPreTriggers = vlstd::ڦيرو_مٿي_منزل_گڻو_جو<لفظ_سائيز>(senItemps.سائيز());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ثابت uint32_t n_Pre_لفظ = nPreTriggers / لفظ_سائيز;

    // Process the rest of the trees
    لكل (ثابت Ast_Sen_وڻ* ثابت senTreep : senTreeps) {
        لكل (ثابت Ast_Sen_شي *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_شي);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت خودڪار pair = senItem2TrigIdx.emplace(*itemp, senItemps.سائيز());
            إذا (pair.ٻيو) senItemps.ڌڪو_پٺ(itemp);
        }
    }
    ثابت uint32_t n_Sen_شيون = senItemps.سائيز() - nPreTriggers;
    V3Stats::جوڙيو_Stat("Scheduling, '" + name + "' sense triggers", n_Sen_شيون + n_Pre_Sen_شيون);
    // Number of sense triggers, rounded up to a full word
    ثابت uint32_t nSenseTriggers = vlstd::ڦيرو_مٿي_منزل_گڻو_جو<لفظ_سائيز>(senItemps.سائيز());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ثابت uint32_t n_Sense_لفظ = nSenseTriggers / لفظ_سائيز;

    // Allocate space for the extra triggers
    V3Stats::جوڙيو_Stat("Scheduling, '" + name + "' extra triggers", extraTriggers.سائيز());
    // Number of extra triggers, rounded up to a full word.
    ثابت uint32_t nExtraTriggers = vlstd::ڦيرو_مٿي_منزل_گڻو_جو<لفظ_سائيز>(extraTriggers.سائيز());
    ثابت uint32_t n_Extra_لفظ = nExtraTriggers / لفظ_سائيز;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_لفظ, n_Extra_لفظ, n_Pre_لفظ, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    إذا (!kit.m_n_Vec_لفظ) أرجع kit;

    File_سٽ* ثابت flp = netlistp->چوٽي_Scopep()->fileline();

    // Creates read/write reference
    ثابت خودڪار rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودڪار wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* ثابت dumpTrgp
        = new_دليل(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* ثابت dumpTagp
        = new_دليل(kit.m_dumpp, netlistp->ڳوليو_String_DType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* ثابت ifp = جديد AstIf{flp, جديد AstLogNot{flp, kit.new_Any_سيٽ_Call(dumpTrgp)}};
        kit.m_dumpp->جوڙيو_Stmtsp(ifp);
        AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
        ifp->جوڙيو_Thensp(cstmtp);
        cstmtp->جوڙيو("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->جوڙيو(rd(dumpTagp));
        cstmtp->جوڙيو(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    ثابت خودڪار جوڙيو_Debug = [&](uint32_t اشاريو, ثابت string& متن) {
        ثابت صحيح wrd_اشاريو = static_cast<صحيح>(اشاريو / لفظ_سائيز);
        ثابت صحيح ٻٽو_انگ_اشاريو = static_cast<صحيح>(اشاريو % لفظ_سائيز);
        Ast_ڳنڍ_اظهار* ثابت aselp = جديد AstArraySel{flp, rd(dumpTrgp), wrd_اشاريو};
        Ast_ڳنڍ_اظهار* ثابت condp = جديد AstSel{flp, aselp, ٻٽو_انگ_اشاريو, 1};
        AstIf* ثابت ifp = جديد AstIf{flp, condp};
        kit.m_dumpp->جوڙيو_Stmtsp(ifp);
        AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
        ifp->جوڙيو_Thensp(cstmtp);
        cstmtp->جوڙيو("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->جوڙيو(rd(dumpTagp));
        cstmtp->جوڙيو(" + \"' region trigger index " + std::منزل_string(اشاريو) + " is active: " + متن
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::ويڪٽر<Ast_ڳنڍ_اظهار*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_ڳنڍ_Stmt* initialTrigsp = خالي_اشارو;
    لكل (سائيز_t i = 0; i < senItemps.سائيز(); ++i) {
        ثابت Ast_Sen_شي* ثابت senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        إذا (!senItemp) {
            trigps.emplace_پٺ(جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}});
            استمر;
        }

        // Create the trigger computation expression
        ثابت خودڪار& pair = sen_اظهار_Builder.build(senItemp);
        trigps.emplace_پٺ(pair.پهريون);

        // Add initialization time trigger
        إذا (pair.ٻيو || v3Global.opt.x_Initial_ڪنارو()) {
            ثابت صحيح wrd_اشاريو = static_cast<صحيح>(i / لفظ_سائيز);
            ثابت صحيح ٻٽو_انگ_اشاريو = static_cast<صحيح>(i % لفظ_سائيز);
            Ast_ڳنڍ_اظهار* ثابت wordp = جديد AstArraySel{flp, wr(kit.m_vscp), wrd_اشاريو};
            Ast_ڳنڍ_اظهار* ثابت lhsp = جديد AstSel{flp, wordp, ٻٽو_انگ_اشاريو, 1};
            Ast_ڳنڍ_اظهار* ثابت rhsp = جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}};
            إذا (useAcc) {
                initFuncp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, lhsp, rhsp});
            } وإلا {
                initialTrigsp = Ast_ڳنڍ::جوڙيو_ايندڙ(initialTrigsp, جديد Ast_سونپيو{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_وڻ(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        جوڙيو_Debug(i, desc);
    }
    UASSERT(trigps.سائيز() == nSenseTriggers, "Inconsistent number of trigger expressions");

    Ast_سونپيو* ثابت trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    لكل (بنانشان i = 0; i < extraTriggers.سائيز(); ++i) {
        جوڙيو_Debug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    ثابت Sen_اظهار_Builder::Results senResults = sen_اظهار_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    لكل (Ast_ڳنڍ_Stmt* ثابت nodep : senResults.m_inits) initFuncp->جوڙيو_Stmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_چوٽي_Scope* ثابت چوٽي_Scopep = netlistp->چوٽي_Scopep();
    {
        AstCFunc* ثابت fp = kit.m_compVecp;
        // Trigger computation
        لكل (Ast_ڳنڍ_Stmt* ثابت nodep : senResults.m_preUpdates) fp->جوڙيو_Stmtsp(nodep);
        fp->جوڙيو_Stmtsp(trigStmtsp);
        لكل (Ast_ڳنڍ_Stmt* ثابت nodep : senResults.m_postUpdates) fp->جوڙيو_Stmtsp(nodep);
        // Add the initialization time triggers
        إذا (initialTrigsp) {
            AstVarScope* ثابت initVscp = چوٽي_Scopep->create_عارضي("__V" + name + "DidInit", 1);
            AstIf* ثابت ifp = جديد AstIf{flp, جديد AstNot{flp, rd(initVscp)}};
            fp->جوڙيو_Stmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->جوڙيو_Thensp(util::سيٽ_Var(initVscp, 1));
            ifp->جوڙيو_Thensp(initialTrigsp);
        }
        util::ورهايو_Check(fp);
    };
    // If there are 'pre' triggers, compute them
    إذا (kit.m_n_Pre_لفظ) {
        AstCFunc* ثابت fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ثابت latchedp
            = new_دليل(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ثابت nVscp = چوٽي_Scopep->create_عارضي("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(سچ);
        // Add a loop to compute the pre words
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        fp->جوڙيو_Stmtsp(util::سيٽ_Var(nVscp, 0));
        fp->جوڙيو_Stmtsp(loopp);
        // Loop body
        Ast_ڳنڍ_اظهار* ثابت offsetp = جديد AstConst{flp, kit.m_n_Vec_لفظ};
        Ast_ڳنڍ_اظهار* ثابت lIdxp = جديد Ast_جوڙيو{flp, rd(nVscp), offsetp};
        Ast_ڳنڍ_اظهار* ثابت lhsp = جديد AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_ڳنڍ_اظهار* ثابت aWordp = جديد AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_ڳنڍ_اظهار* ثابت bWordp = جديد AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_ڳنڍ_اظهار* ثابت rhsp = جديد AstAnd{flp, aWordp, جديد AstNot{flp, bWordp}};
        Ast_ڳنڍ_اظهار* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, n_Pre_لفظ};
        loopp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, lhsp, rhsp});
        loopp->جوڙيو_Stmtsp(util::incrementVar(nVscp));
        loopp->جوڙيو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});
        util::ورهايو_Check(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    إذا (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlink_Fr_پٺ_With_ايندڙ()->delete_وڻ();
    // Done with the trigger dump function, split as might be large
    util::ورهايو_Check(kit.m_dumpp);

    أرجع kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
صنف AwaitBeforeTrigVisitor final : عام VNVisitor {
    ثابت VNUser1_اندريون_Use m_user1_اندريون_Use;

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
    Sen_اظهار_Builder& m_sen_اظهار_Builder;
    // Generator of unique names for before-trigger function
    V3_Unique_نالا m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::ويڪٽر<std::pair<AstCFunc*, Ast_Sen_وڻ*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::ويڪٽر<std::pair<Ast_Sen_وڻ*, Ast_ڳنڍ_اظهار*>> m_sen_وڻ_منزل_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_ڳنڍ>, std::ويڪٽر<Ast_Sen_شي*>> m_sen_اظهار_منزل_Sen_شي;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    ساكن Ast_ڳنڍ* get_Sen_Hash_ڳنڍ(ثابت Ast_Sen_شي* ثابت nodep) {
        إذا (AstVarRef* ثابت varRefp = VN_CAST(nodep->sensp(), VarRef)) أرجع varRefp;
        أرجع nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    فراغ fill_Sen_اظهار_منزل_Sen_شي() {
        لكل (خودڪار sen_وڻ_Sched : m_sen_وڻ_منزل_Sched) {
            Ast_Sen_وڻ* ثابت senTreep = sen_وڻ_Sched.پهريون;

            لكل (Ast_Sen_شي* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_شي)) {
                ثابت VEdgeType ڪنارو = senItemp->ڪنارو_Type();
                إذا (ڪنارو.an_ڪنارو() || ڪنارو == VEdgeType::ET_بدليو
                    || ڪنارو == VEdgeType::ET_HYBRID) {
                    m_sen_اظهار_منزل_Sen_شي[*get_Sen_Hash_ڳنڍ(senItemp)].ڌڪو_پٺ(senItemp);
                }
            }
        }

        std::ويڪٽر<VNRef<Ast_ڳنڍ>> منزل_Remove;
        لكل (ثابت خودڪار& sen_اظهار_منزل_Sen_وڻ : m_sen_اظهار_منزل_Sen_شي) {
            std::ويڪٽر<Ast_Sen_شي*> senItemps = sen_اظهار_منزل_Sen_وڻ.ٻيو;
            منزل_Remove.ڌڪو_پٺ(sen_اظهار_منزل_Sen_وڻ.پهريون);
            لكل (سائيز_t i = 1; i < senItemps.سائيز(); ++i) {
                إذا (senItemps[i]->ڪنارو_Type() != senItemps[i - 1]->ڪنارو_Type()) {
                    منزل_Remove.ڪڍو_پٺ();
                    اكسر;
                }
            }
        }
        لكل (ثابت VNRef<Ast_ڳنڍ> it : منزل_Remove) m_sen_اظهار_منزل_Sen_شي.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<سائيز_t, std::map<سائيز_t, std::ويڪٽر<Ast_ڳنڍ_اظهار*>>>
    get_استعمال_ٿيل_Triggers_منزل_Trees(ثابت std::سيٽ<سائيز_t>& استعمال_ٿيل_Triggers) {
        std::map<سائيز_t, std::map<سائيز_t, std::ويڪٽر<Ast_ڳنڍ_اظهار*>>> استعمال_ٿيل_Trigs_منزل_Using_Trees;
        لكل (خودڪار sen_وڻ_Sched : m_sen_وڻ_منزل_Sched) {
            ثابت Ast_Sen_وڻ* ثابت senTreep = sen_وڻ_Sched.پهريون;
            Ast_ڳنڍ_اظهار* ثابت shedp = sen_وڻ_Sched.ٻيو;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            لكل (Ast_Sen_شي* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_شي)) {
                ثابت سائيز_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                إذا (استعمال_ٿيل_Triggers.ڳوليو(idx) != استعمال_ٿيل_Triggers.پڇاڙي()) {
                    استعمال_ٿيل_Trigs_منزل_Using_Trees[idx / TriggerKit::لفظ_سائيز]
                                         [سائيز_t{1} << (idx % TriggerKit::لفظ_سائيز)]
                                             .ڌڪو_پٺ(shedp);
                }
            }
        }
        إذا (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            لكل (ثابت خودڪار& triggers_منزل_Trees : استعمال_ٿيل_Trigs_منزل_Using_Trees) {
                لكل (ثابت خودڪار& بٽ_منزل_Trees : triggers_منزل_Trees.ٻيو) {
                    ثابت std::سيٽ<ثابت Ast_ڳنڍ_اظهار*> exprps{بٽ_منزل_Trees.ٻيو.begin(),
                                                              بٽ_منزل_Trees.ٻيو.پڇاڙي()};
                    UASSERT(بٽ_منزل_Trees.ٻيو.سائيز() == exprps.سائيز(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        أرجع استعمال_ٿيل_Trigs_منزل_Using_Trees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_وڻ* ثابت senTreep) {
        File_سٽ* ثابت flp = senTreep->fileline();
        إذا (!senTreep->user1p()) {
            AstCFunc* ثابت funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), خطأ);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* ثابت tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(سچ);

            AstVar* ثابت argp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->ڳوليو_Basic_DType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(سچ);
            argp->direction(VDirection::داخلا);
            funcp->جوڙيو_Argsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_پٺ(funcp, senTreep);
        }
        AstCCall* ثابت callp = جديد AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtype_سيٽ_Void();
        أرجع callp;
    }

    فراغ گهمو(AstCAwait* ثابت nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        إذا (ثابت AstCMethodHard* ثابت c_طريقو_Hardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            إذا (c_طريقو_Hardp->طريقو() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ثابت beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_ڳنڍ* ثابت pinp = c_طريقو_Hardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, c_طريقو_Hardp, "No event description");
                beforeTrigp->جوڙيو_Argsp(VN_AS(pinp, ڳنڍ_اظهار)->clone_وڻ(خطأ));

                // Call the before-trigger function before the CAwait
                nodep->جوڙيو_هتي_This_As_ايندڙ(beforeTrigp->makeStmt());
                m_sen_وڻ_منزل_Sched.emplace_پٺ(nodep->sentreep(), c_طريقو_Hardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ* ثابت nodep) override { iterateChildren(nodep); }

عام:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_اظهار_Builder& sen_اظهار_Builder,
                           ثابت TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_اظهار_Builder{sen_اظهار_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_اظهار_منزل_Sen_شي();

        std::ويڪٽر<Ast_ڳنڍ_اظهار*> trigps;
        std::سيٽ<سائيز_t> استعمال_ٿيل_Triggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        لكل (ثابت خودڪار& func_منزل_استعمال_ٿيل_Triggers : m_generatedFuncs) {
            AstCFunc* ثابت funcp = func_منزل_استعمال_ٿيل_Triggers.پهريون;
            AstVarScope* ثابت vscp = VN_AS(funcp->user1p(), VarScope);
            File_سٽ* ثابت flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_وڻ* ثابت senTreep = func_منزل_استعمال_ٿيل_Triggers.ٻيو;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ثابت خودڪار emplaceAt
                    = [flp, &trigps, &استعمال_ٿيل_Triggers](Ast_ڳنڍ_اظهار* ثابت exprp, ثابت سائيز_t pos) {
                          ثابت سائيز_t هدف_سائيز
                              = vlstd::ڦيرو_مٿي_منزل_گڻو_جو<TriggerKit::لفظ_سائيز>(pos + 1);
                          إذا (trigps.صلاحيت() < هدف_سائيز) trigps.reserve(هدف_سائيز * 2);
                          طالما (trigps.سائيز() < هدف_سائيز) {
                              trigps.ڌڪو_پٺ(جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}});
                          }
                          trigps[pos]->delete_وڻ();
                          trigps[pos] = exprp;
                          استعمال_ٿيل_Triggers.وجهو(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                لكل (ثابت Ast_Sen_شي* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Sen_شي)) {
                    ثابت سائيز_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_اظهار_Builder.build(itemp).پهريون, idx);
                    خودڪار iter = m_sen_اظهار_منزل_Sen_شي.ڳوليو(*get_Sen_Hash_ڳنڍ(itemp));
                    إذا (iter != m_sen_اظهار_منزل_Sen_شي.پڇاڙي()) {
                        لكل (Ast_Sen_شي* ثابت additionalItemp : iter->ٻيو) {
                            ثابت سائيز_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_اظهار_Builder.build(additionalItemp).پهريون, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ثابت Sen_اظهار_Builder::Results results
                    = m_sen_اظهار_Builder.getResultsAndClearUpdates();
                لكل (Ast_ڳنڍ_Stmt* ثابت stmtsp : results.m_inits) funcp->جوڙيو_Stmtsp(stmtsp);
                لكل (Ast_ڳنڍ_Stmt* ثابت stmtsp : results.m_preUpdates) funcp->جوڙيو_Stmtsp(stmtsp);
                funcp->جوڙيو_Stmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                لكل (Ast_ڳنڍ_Stmt* ثابت stmtsp : results.m_postUpdates) funcp->جوڙيو_Stmtsp(stmtsp);
            }

            ثابت std::map<سائيز_t, std::map<سائيز_t, std::ويڪٽر<Ast_ڳنڍ_اظهار*>>>
                استعمال_ٿيل_Trigs_منزل_Using_Trees = get_استعمال_ٿيل_Triggers_منزل_Trees(استعمال_ٿيل_Triggers);
            استعمال_ٿيل_Triggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            ثابت خودڪار getIdx = [flp](AstVarScope* ثابت scocep, VAccess access, سائيز_t idx) {
                أرجع جديد AstArraySel{flp, جديد AstVarRef{flp, scocep, access},
                                       جديد AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* ثابت argpVscp = جديد AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->جوڙيو_Varsp(argpVscp);

            // Mark as ready triggered schedulers
            لكل (ثابت خودڪار& triggers_منزل_Trees : استعمال_ٿيل_Trigs_منزل_Using_Trees) {
                ثابت سائيز_t لفظ = triggers_منزل_Trees.پهريون;

                لكل (ثابت خودڪار& بٽ_منزل_Trees : triggers_منزل_Trees.ٻيو) {
                    ثابت سائيز_t ٻٽو_انگ_2 = بٽ_منزل_Trees.پهريون;
                    ثابت خودڪار& schedulers = بٽ_منزل_Trees.ٻيو;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* ثابت نقاب_Constp = جديد AstConst{flp, AstConst::Unsized64{}, ٻٽو_انگ_2};
                    AstAnd* ثابت condp
                        = جديد AstAnd{flp, getIdx(vscp, VAccess::READ, لفظ), نقاب_Constp};
                    AstIf* ثابت ifp = جديد AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    لكل (Ast_ڳنڍ_اظهار* ثابت schedp : schedulers) {
                        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                            flp, schedp->clone_وڻ(خطأ), VCMethod::SCHED_READY};
                        callp->dtype_سيٽ_Void();
                        callp->جوڙيو_Pinsp(جديد AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->جوڙيو_Thensp(callp->makeStmt());
                    }
                    funcp->جوڙيو_Stmtsp(ifp);
                }
            }

            AstVarScope* ثابت vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            لكل (ثابت خودڪار& triggers_منزل_Trees : استعمال_ٿيل_Trigs_منزل_Using_Trees) {
                ثابت سائيز_t لفظ = triggers_منزل_Trees.پهريون;
                funcp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, getIdx(vscAccp, VAccess::WRITE, لفظ),
                                               جديد AstOr{flp, getIdx(vscAccp, VAccess::READ, لفظ),
                                                         getIdx(vscp, VAccess::READ, لفظ)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = افتراضي;
};

فراغ beforeTrigVisitor(AstNetlist* netlistp, Sen_اظهار_Builder& sen_اظهار_Builder,
                       ثابت TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_اظهار_Builder, trigKit};
}

}  // namespace V3Sched
