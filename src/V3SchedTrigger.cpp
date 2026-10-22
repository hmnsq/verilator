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

ورتوں util::new_دلیل;

AstVarScope* newLocal(AstCFunc* funcp, Ast_گرہ_DType* dtypep, ثابت std::string& name) {
    File_سطر* ثابت flp = funcp->fileline();
    AstScope* ثابت scopep = funcp->scopep();
    AstVar* ثابت varp = جديد AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(سچ);
    varp->noReset(سچ);
    funcp->جوڑو_Varsp(varp);
    AstVarScope* ثابت vscp = جديد AstVarScope{flp, scopep, varp};
    scopep->جوڑو_Varsp(vscp);
    أرجع vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() ثابت {
    UASSERT(m_n_Pre_الفاظ, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->چوٹی_Scopep()->fileline();
    Ast_گرہ_DType* ثابت u32DTypep = netlistp->لبھو_UInt32_DType();
    Ast_گرہ_DType* ثابت strDtypep = netlistp->لبھو_String_DType();

    // Dumping function always slow
    ثابت std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, سچ);
    funcp->isStatic(سچ);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* ثابت eVscp = new_دلیل(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* ثابت tVscp = new_دلیل(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    ثابت خودکار rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودکار wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* ثابت vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* ثابت iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->جوڑو_Stmtsp(util::سیٹ_Var(iVscp, 0));
        // Add loop
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        funcp->جوڑو_Stmtsp(loopp);
        // Loop body
        Ast_گرہ_اظہاریہ* ثابت lhsp = جديد AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_گرہ_اظہاریہ* ثابت rhsp = جديد AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_گرہ_اظہاریہ* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, m_n_Vec_الفاظ};
        loopp->جوڑو_Stmtsp(جديد Ast_سونپو{flp, lhsp, rhsp});
        loopp->جوڑو_Stmtsp(util::incrementVar(iVscp));
        loopp->جوڑو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* ثابت callp = جديد AstCCall{flp, m_dumpp};
        callp->dtype_سیٹ_Void();
        callp->جوڑو_Argsp(rd(vVscp));
        callp->جوڑو_Argsp(rd(tVscp));
        funcp->جوڑو_Stmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* ثابت pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* ثابت jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->جوڑو_Stmtsp(util::سیٹ_Var(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            funcp->جوڑو_Stmtsp(loopp);
            // Loop body
            Ast_گرہ_اظہاریہ* ثابت lhsp = جديد AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_گرہ_اظہاریہ* ثابت rhsp = جديد AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_گرہ_اظہاریہ* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, m_n_Pre_الفاظ};
            loopp->جوڑو_Stmtsp(جديد Ast_سونپو{flp, lhsp, rhsp});
            loopp->جوڑو_Stmtsp(util::incrementVar(jVscp));
            loopp->جوڑو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            funcp->جوڑو_Stmtsp(loopp);
            // Loop body
            Ast_گرہ_اظہاریہ* ثابت lhsp = جديد AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_گرہ_اظہاریہ* ثابت rhsp = جديد AstConst{flp, AstConst::DTyped{}, m_لفظ_DTypep};
            Ast_گرہ_اظہاریہ* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, m_n_Vec_الفاظ};
            loopp->جوڑو_Stmtsp(جديد Ast_سونپو{flp, lhsp, rhsp});
            loopp->جوڑو_Stmtsp(util::incrementVar(jVscp));
            loopp->جوڑو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* ثابت callp = جديد AstCCall{flp, m_dumpp};
        callp->dtype_سیٹ_Void();
        callp->جوڑو_Argsp(rd(pVscp));
        callp->جوڑو_Argsp(
            جديد AstConcatN{flp, rd(tVscp), جديد AstConst{flp, AstConst::String{}, " pre"}});
        funcp->جوڑو_Stmtsp(callp->makeStmt());
    }

    أرجع funcp;
}

AstCFunc* TriggerKit::create_Any_سیٹ_Func(AstUnpackArrayDType* ثابت dtypep) ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->چوٹی_Scopep()->fileline();
    Ast_گرہ_DType* ثابت u32DTypep = netlistp->لبھو_UInt32_DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(سچ);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* ثابت iVscp = new_دلیل(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    ثابت خودکار rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* ثابت loopp = جديد AstLoop{flp};
    funcp->جوڑو_Stmtsp(util::سیٹ_Var(nVscp, 0));
    funcp->جوڑو_Stmtsp(loopp);
    funcp->جوڑو_Stmtsp(جديد AstCReturn{flp, جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}});

    // Loop body
    ثابت uint32_t n_الفاظ = dtypep->elementsConst();
    Ast_گرہ_اظہاریہ* ثابت condp = جديد AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_گرہ_Stmt* ثابت thenp = جديد AstCReturn{flp, جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}};
    Ast_گرہ_اظہاریہ* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, n_الفاظ};
    loopp->جوڑو_Stmtsp(جديد AstIf{flp, condp, thenp});
    loopp->جوڑو_Stmtsp(util::incrementVar(nVscp));
    loopp->جوڑو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});

    //پورا
    أرجع funcp;
}
AstCFunc* TriggerKit::createClearFunc() ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->چوٹی_Scopep()->fileline();
    Ast_گرہ_DType* ثابت u32DTypep = netlistp->لبھو_UInt32_DType();

    // Create function
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(سچ);

    // Add arguments
    AstVarScope* ثابت oVscp = new_دلیل(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ثابت خودکار rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودکار wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ثابت loopp = جديد AstLoop{flp};
    funcp->جوڑو_Stmtsp(util::سیٹ_Var(nVscp, 0));
    funcp->جوڑو_Stmtsp(loopp);

    // Loop body
    Ast_گرہ_اظہاریہ* ثابت lhsp = جديد AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_گرہ_اظہاریہ* ثابت rhsp = جديد AstConst{flp, AstConst::DTyped{}, m_لفظ_DTypep};
    Ast_گرہ_اظہاریہ* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, m_n_Vec_الفاظ};
    loopp->جوڑو_Stmtsp(جديد Ast_سونپو{flp, lhsp, rhsp});
    loopp->جوڑو_Stmtsp(util::incrementVar(nVscp));
    loopp->جوڑو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});

    //پورا
    أرجع funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ثابت oDtypep,
                                       AstUnpackArrayDType* ثابت iDtypep) ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->چوٹی_Scopep()->fileline();
    Ast_گرہ_DType* ثابت u32DTypep = netlistp->لبھو_UInt32_DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(سچ);

    // Add arguments
    AstVarScope* ثابت oVscp = new_دلیل(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* ثابت iVscp = new_دلیل(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ثابت خودکار rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودکار wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ثابت loopp = جديد AstLoop{flp};
    funcp->جوڑو_Stmtsp(util::سیٹ_Var(nVscp, 0));
    funcp->جوڑو_Stmtsp(loopp);

    // Loop body
    Ast_گرہ_اظہاریہ* ثابت lhsp = جديد AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_گرہ_اظہاریہ* ثابت oWordp = جديد AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_گرہ_اظہاریہ* ثابت iWordp = جديد AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_گرہ_اظہاریہ* ثابت rhsp = جديد AstOr{flp, oWordp, iWordp};
    AstConst* ثابت outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ثابت داخلہ_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_گرہ_اظہاریہ* ثابت limp = outputRangeLeftp->عدد().منزل_SInt() < داخلہ_Range_Leftp->عدد().منزل_SInt()
                                  ? outputRangeLeftp->clone_درخت_خالص(خطأ)
                                  : داخلہ_Range_Leftp->clone_درخت_خالص(خطأ);
    loopp->جوڑو_Stmtsp(جديد Ast_سونپو{flp, lhsp, rhsp});
    loopp->جوڑو_Stmtsp(util::incrementVar(nVscp));
    loopp->جوڑو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLte{flp, rd(nVscp), limp}});

    //پورا
    أرجع funcp;
}

Ast_گرہ_اظہاریہ* TriggerKit::new_Any_سیٹ_Call(AstVarScope* ثابت vscp) ثابت {
    File_سطر* ثابت flp = v3Global.rootp()->چوٹی_Scopep()->fileline();
    إذا (!m_n_Vec_الفاظ) أرجع جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}};

    AstCFunc* funcp = خالی_سوچک;
    إذا (vscp->dtypep() == m_trigVecDTypep) {
        إذا (!m_any_سیٹ_Vecp) m_any_سیٹ_Vecp = create_Any_سیٹ_Func(m_trigVecDTypep);
        funcp = m_any_سیٹ_Vecp;
    } وإلا إذا (vscp->dtypep() == m_trigExtDTypep) {
        إذا (!m_any_سیٹ_Extp) m_any_سیٹ_Extp = create_Any_سیٹ_Func(m_trigExtDTypep);
        funcp = m_any_سیٹ_Extp;
    } وإلا {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->جوڑو_Argsp(جديد AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_سیٹ_ثنائی_ہندسہ();
    أرجع callp;
}
Ast_گرہ_Stmt* TriggerKit::newClearCall(AstVarScope* ثابت vscp) ثابت {
    إذا (!m_n_Vec_الفاظ) أرجع خالی_سوچک;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    إذا (!m_clearp) m_clearp = createClearFunc();
    File_سطر* ثابت flp = v3Global.rootp()->چوٹی_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_clearp};
    callp->جوڑو_Argsp(جديد AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_سیٹ_Void();
    أرجع callp->makeStmt();
}
Ast_گرہ_Stmt* TriggerKit::newOrIntoCall(AstVarScope* ثابت oVscp, AstVarScope* ثابت iVscp) ثابت {
    إذا (!m_n_Vec_الفاظ) أرجع خالی_سوچک;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    ثابت ناپ_t نقاب
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[نقاب];
    إذا (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_سطر* ثابت flp = v3Global.rootp()->چوٹی_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->جوڑو_Argsp(جديد AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->جوڑو_Argsp(جديد AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtype_سیٹ_Void();
    أرجع callp->makeStmt();
}

Ast_گرہ_Stmt* TriggerKit::new_Comp_ادھار_Call() ثابت {
    إذا (!m_n_Vec_الفاظ) أرجع خالی_سوچک;
    File_سطر* ثابت flp = v3Global.rootp()->چوٹی_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_compVecp};
    callp->dtype_سیٹ_Void();
    أرجع callp->makeStmt();
}

Ast_گرہ_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ثابت {
    إذا (!m_n_Pre_الفاظ) أرجع خالی_سوچک;
    File_سطر* ثابت flp = v3Global.rootp()->چوٹی_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_compExtp};
    callp->جوڑو_Argsp(جديد AstVarRef{flp, vscp, VAccess::READ});
    callp->dtype_سیٹ_Void();
    أرجع callp->makeStmt();
}

Ast_گرہ_Stmt* TriggerKit::newDumpCall(AstVarScope* ثابت vscp, ثابت std::string& ٹیگ,
                                     منطقي debugOnly) ثابت {
    إذا (!m_n_Vec_الفاظ) أرجع خالی_سوچک;
    AstCFunc* funcp = خالی_سوچک;
    إذا (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } وإلا إذا (vscp->dtypep() == m_trigExtDTypep) {
        إذا (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } وإلا {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_سطر* ثابت flp = v3Global.rootp()->چوٹی_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->جوڑو_Argsp(جديد AstVarRef{flp, vscp, VAccess::READ});
    callp->جوڑو_Argsp(جديد AstConst{flp, AstConst::String{}, ٹیگ});
    callp->dtype_سیٹ_Void();
    AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
    cstmtp->جوڑو("#ifdef VL_DEBUG\n");
    إذا (debugOnly) {
        cstmtp->جوڑو("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->جوڑو(callp->makeStmt());
        cstmtp->جوڑو("}\n");
    } وإلا {
        cstmtp->جوڑو(callp->makeStmt());
    }
    cstmtp->جوڑو("#endif");
    أرجع cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(ثابت std::string& name) ثابت {
    إذا (!m_n_Vec_الفاظ) أرجع خالی_سوچک;
    أرجع v3Global.rootp()->چوٹی_Scopep()->create_عارضی("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extra_Trigger_اشاریہ(uint32_t اشاریہ) ثابت {
    UASSERT(اشاریہ <= m_n_Extra_الفاظ * لفظ_ناپ, "Invalid external trigger index");
    أرجع اشاریہ + m_n_Sense_الفاظ * لفظ_ناپ;
}

std::سیٹ<uint32_t> TriggerKit::triggerIndices(ثابت Ast_Sen_درخت* senTreep, منطقي pre) ثابت {
    std::سیٹ<uint32_t> indices;
    لكل (ثابت Ast_Sen_شے* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), Sen_شے)) {
        uint32_t اشاریہ = m_senItem2TrigIdx.at(*sip);
        إذا (pre) {
            UASSERT_OBJ(اشاریہ < m_n_Pre_الفاظ * لفظ_ناپ, sip, "Not a 'pre' trigger");
            اشاریہ += m_n_Vec_الفاظ * لفظ_ناپ;
        }
        indices.داخل_کرو(اشاریہ);
    }
    أرجع indices;
}

Ast_گرہ_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t اشاریہ) ثابت {
    اشاریہ = extra_Trigger_اشاریہ(اشاریہ);
    ثابت uint32_t لفظ_اشاریہ = اشاریہ / لفظ_ناپ;
    ثابت uint32_t ثنائی_ہندسہ_اشاریہ = اشاریہ % لفظ_ناپ;
    File_سطر* ثابت flp = vscp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_گرہ_اظہاریہ* ثابت wordp = جديد AstArraySel{flp, refp, static_cast<صحيح>(لفظ_اشاریہ)};
    Ast_گرہ_اظہاریہ* ثابت trigLhsp = جديد AstSel{flp, wordp, static_cast<صحيح>(ثنائی_ہندسہ_اشاریہ), 1};
    Ast_گرہ_اظہاریہ* ثابت trigRhsp = جديد AstVarRef{flp, vscp, VAccess::READ};
    أرجع جديد Ast_سونپو{flp, trigLhsp, trigRhsp};
}

فراغ TriggerKit::جوڑو_Extra_Trigger_Assignment(AstVarScope* vscp, uint32_t اشاریہ) ثابت {
    File_سطر* ثابت flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_گرہ* ثابت setp = newExtraTriggerAssignment(vscp, اشاریہ);
    setp->جوڑو_اگلا(جديد Ast_سونپو{flp, جديد AstVarRef{flp, vscp, VAccess::WRITE},
                                جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
فراغ TriggerKit::جوڑو_قدر_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t اشاریہ) ثابت {
    اشاریہ += m_n_Sense_الفاظ * لفظ_ناپ;
    ثابت uint32_t لفظ_اشاریہ = اشاریہ / لفظ_ناپ;
    ثابت uint32_t ثنائی_ہندسہ_اشاریہ = اشاریہ % لفظ_ناپ;

    Ast_چوٹی_Scope* ثابت چوٹی_Scopep = netlistp->چوٹی_Scopep();
    File_سطر* ثابت flp = netlistp->fileline();
    Ast_گرہ_DType* ثابت dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    إذا (ثابت AstBasicDType* ثابت bdtypep = VN_CAST(dtypep, BasicDType)) {
        إذا (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            أرجع;
        }
    } وإلا إذا (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, گرہ_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        أرجع;
    }

    ثابت خودکار rdInst = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودکار wrPrev = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };
    ثابت خودکار rdPrev = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    ثابت std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* ثابت prevVscp = چوٹی_Scopep->create_عارضی(prevName, instVscp->dtypep());

    // Initialize prev = inst
    إذا (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = جديد AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_سونپو, rdInst(instVscp)};
        cmhp->dtype_سیٹ_Void();
        initFuncp->جوڑو_Stmtsp(cmhp->makeStmt());
    } وإلا {
        initFuncp->جوڑو_Stmtsp(جديد Ast_سونپو{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_گرہ_اظہاریہ* neqp;
    إذا (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp
            = جديد AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtype_سیٹ_ثنائی_ہندسہ();
        neqp = cmhp;
    } وإلا {
        neqp = جديد AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_گرہ* updp;
    إذا (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = جديد AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_سونپو, rdInst(instVscp)};
        cmhp->dtype_سیٹ_Void();
        updp = cmhp->makeStmt();
    } وإلا {
        updp = جديد Ast_سونپو{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ثابت refp = جديد AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_گرہ_اظہاریہ* ثابت wordp = جديد AstArraySel{flp, refp, static_cast<صحيح>(لفظ_اشاریہ)};
    Ast_گرہ_اظہاریہ* ثابت trigLhsp = جديد AstSel{flp, wordp, static_cast<صحيح>(ثنائی_ہندسہ_اشاریہ), 1};
    Ast_گرہ* ثابت setp = جديد Ast_سونپو{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->جوڑو_اگلا(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ثابت std::string& name, منطقي slow, uint32_t n_Sense_الفاظ,
                       uint32_t n_Extra_الفاظ, uint32_t n_Pre_الفاظ,
                       std::unordered_map<VNRef<ثابت Ast_Sen_شے>, ناپ_t> senItem2TrigIdx,
                       ثابت std::سمتیہ<ثابت Ast_Sen_شے*>& senItemps, منطقي useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_الفاظ{n_Sense_الفاظ}
    , m_n_Extra_الفاظ{n_Extra_الفاظ}
    , m_n_Pre_الفاظ{n_Pre_الفاظ}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trig_Idx2_Sen_شے{senItemps} {
    // If no triggers, we don't need to generate anything
    إذا (!m_n_Vec_الفاظ) أرجع;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trig_Idx2_Sen_شے.resize((m_n_Vec_الفاظ + m_n_Pre_الفاظ) * لفظ_ناپ);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_الفاظ * لفظ_ناپ,
              m_trig_Idx2_Sen_شے.begin() + m_n_Vec_الفاظ * لفظ_ناپ);
    AstNetlist* ثابت netlistp = v3Global.rootp();
    AstScope* ثابت scopep = netlistp->چوٹی_Scopep()->scopep();
    File_سطر* ثابت flp = scopep->fileline();
    // Data type of a single trigger word
    m_لفظ_DTypep = netlistp->لبھو_ثنائی_ہندسہ_DType(لفظ_ناپ, لفظ_ناپ, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ثابت rp = جديد AstRange{flp, static_cast<صحيح>(m_n_Vec_الفاظ - 1), 0};
    m_trigVecDTypep = جديد AstUnpackArrayDType{flp, m_لفظ_DTypep, rp};
    netlistp->typeTablep()->جوڑو_Typesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    إذا (m_n_Pre_الفاظ) {
        AstRange* ثابت ep = جديد AstRange{flp, static_cast<صحيح>(m_n_Vec_الفاظ + m_n_Pre_الفاظ - 1), 0};
        m_trigExtDTypep = جديد AstUnpackArrayDType{flp, m_لفظ_DTypep, ep};
        netlistp->typeTablep()->جوڑو_Typesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } وإلا {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->چوٹی_Scopep()->create_عارضی("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->is_اندرونی(سچ);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, سچ);
    m_dumpp->isStatic(سچ);
    m_dumpp->ifdef("VL_DEBUG");
    إذا (useAcc) {
        m_vscAccp
            = netlistp->چوٹی_Scopep()->create_عارضی("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->is_اندرونی(سچ);
    }
}

Ast_سونپو* TriggerKit::createSenTrigVecAssignment(AstVarScope* ثابت نشانہ,
                                                  std::سمتیہ<Ast_گرہ_اظہاریہ*>& trigps) {
    File_سطر* ثابت flp = نشانہ->fileline();
    Ast_سونپو* trigStmtsp = خالی_سوچک;
    // Assign sense triggers vector one word at a time
    لكل (ناپ_t i = 0; i < trigps.ناپ(); i += لفظ_ناپ) {
        // Concatenate all bits in this trigger word using a balanced
        لكل (uint32_t level = 0; level < لفظ_ناپ_LOG2; ++level) {
            ثابت uint32_t stride = 1 << level;
            لكل (uint32_t j = 0; j < لفظ_ناپ; j += 2 * stride) {
                trigps[i + j] = جديد AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = خالی_سوچک;
            }
        }

        // Set the whole word in the trigger vector
        ثابت صحيح لفظ_اشاریہ = static_cast<صحيح>(i / لفظ_ناپ);
        AstArraySel* ثابت aselp
            = جديد AstArraySel{flp, جديد AstVarRef{flp, نشانہ, VAccess::WRITE}, لفظ_اشاریہ};
        trigStmtsp = Ast_گرہ::جوڑو_اگلا(trigStmtsp, جديد Ast_سونپو{flp, aselp, trigps[i]});
    }
    أرجع trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ثابت initFuncp,  //
                              Sen_اظہاریہ_Builder& sen_اظہاریہ_Builder,  //
                              ثابت std::سمتیہ<ثابت Ast_Sen_درخت*>& preTreeps,  //
                              ثابت std::سمتیہ<ثابت Ast_Sen_درخت*>& senTreeps,  //
                              ثابت string& name,  //
                              ثابت ExtraTriggers& extraTriggers,  //
                              منطقي slow,  //
                              منطقي useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::سمتیہ<ثابت Ast_Sen_شے*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<ثابت Ast_Sen_شے>, ناپ_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    لكل (ثابت Ast_Sen_درخت* ثابت senTreep : preTreeps) {
        لكل (ثابت Ast_Sen_شے *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_شے);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت خودکار pair = senItem2TrigIdx.emplace(*itemp, senItemps.ناپ());
            إذا (pair.دوجا) senItemps.ڈالو_پٹھ(itemp);
        }
    }
    ثابت uint32_t n_Pre_Sen_اشیاء = senItemps.ناپ();
    V3Stats::جوڑو_Stat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_اشیاء);
    // Number of pre triggers, rounded up to a full word.
    ثابت uint32_t nPreTriggers = vlstd::گیڑا_اتے_منزل_مضرب_دا<لفظ_ناپ>(senItemps.ناپ());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ثابت uint32_t n_Pre_الفاظ = nPreTriggers / لفظ_ناپ;

    // Process the rest of the trees
    لكل (ثابت Ast_Sen_درخت* ثابت senTreep : senTreeps) {
        لكل (ثابت Ast_Sen_شے *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_شے);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت خودکار pair = senItem2TrigIdx.emplace(*itemp, senItemps.ناپ());
            إذا (pair.دوجا) senItemps.ڈالو_پٹھ(itemp);
        }
    }
    ثابت uint32_t n_Sen_اشیاء = senItemps.ناپ() - nPreTriggers;
    V3Stats::جوڑو_Stat("Scheduling, '" + name + "' sense triggers", n_Sen_اشیاء + n_Pre_Sen_اشیاء);
    // Number of sense triggers, rounded up to a full word
    ثابت uint32_t nSenseTriggers = vlstd::گیڑا_اتے_منزل_مضرب_دا<لفظ_ناپ>(senItemps.ناپ());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ثابت uint32_t n_Sense_الفاظ = nSenseTriggers / لفظ_ناپ;

    // Allocate space for the extra triggers
    V3Stats::جوڑو_Stat("Scheduling, '" + name + "' extra triggers", extraTriggers.ناپ());
    // Number of extra triggers, rounded up to a full word.
    ثابت uint32_t nExtraTriggers = vlstd::گیڑا_اتے_منزل_مضرب_دا<لفظ_ناپ>(extraTriggers.ناپ());
    ثابت uint32_t n_Extra_الفاظ = nExtraTriggers / لفظ_ناپ;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_الفاظ, n_Extra_الفاظ, n_Pre_الفاظ, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    إذا (!kit.m_n_Vec_الفاظ) أرجع kit;

    File_سطر* ثابت flp = netlistp->چوٹی_Scopep()->fileline();

    // Creates read/write reference
    ثابت خودکار rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودکار wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* ثابت dumpTrgp
        = new_دلیل(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* ثابت dumpTagp
        = new_دلیل(kit.m_dumpp, netlistp->لبھو_String_DType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* ثابت ifp = جديد AstIf{flp, جديد AstLogNot{flp, kit.new_Any_سیٹ_Call(dumpTrgp)}};
        kit.m_dumpp->جوڑو_Stmtsp(ifp);
        AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
        ifp->جوڑو_Thensp(cstmtp);
        cstmtp->جوڑو("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->جوڑو(rd(dumpTagp));
        cstmtp->جوڑو(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    ثابت خودکار جوڑو_Debug = [&](uint32_t اشاریہ, ثابت string& متن) {
        ثابت صحيح wrd_اشاریہ = static_cast<صحيح>(اشاریہ / لفظ_ناپ);
        ثابت صحيح ثنائی_ہندسہ_اشاریہ = static_cast<صحيح>(اشاریہ % لفظ_ناپ);
        Ast_گرہ_اظہاریہ* ثابت aselp = جديد AstArraySel{flp, rd(dumpTrgp), wrd_اشاریہ};
        Ast_گرہ_اظہاریہ* ثابت condp = جديد AstSel{flp, aselp, ثنائی_ہندسہ_اشاریہ, 1};
        AstIf* ثابت ifp = جديد AstIf{flp, condp};
        kit.m_dumpp->جوڑو_Stmtsp(ifp);
        AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
        ifp->جوڑو_Thensp(cstmtp);
        cstmtp->جوڑو("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->جوڑو(rd(dumpTagp));
        cstmtp->جوڑو(" + \"' region trigger index " + std::منزل_string(اشاریہ) + " is active: " + متن
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::سمتیہ<Ast_گرہ_اظہاریہ*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_گرہ_Stmt* initialTrigsp = خالی_سوچک;
    لكل (ناپ_t i = 0; i < senItemps.ناپ(); ++i) {
        ثابت Ast_Sen_شے* ثابت senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        إذا (!senItemp) {
            trigps.emplace_پٹھ(جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}});
            استمر;
        }

        // Create the trigger computation expression
        ثابت خودکار& pair = sen_اظہاریہ_Builder.build(senItemp);
        trigps.emplace_پٹھ(pair.پہلا);

        // Add initialization time trigger
        إذا (pair.دوجا || v3Global.opt.x_Initial_کنارا()) {
            ثابت صحيح wrd_اشاریہ = static_cast<صحيح>(i / لفظ_ناپ);
            ثابت صحيح ثنائی_ہندسہ_اشاریہ = static_cast<صحيح>(i % لفظ_ناپ);
            Ast_گرہ_اظہاریہ* ثابت wordp = جديد AstArraySel{flp, wr(kit.m_vscp), wrd_اشاریہ};
            Ast_گرہ_اظہاریہ* ثابت lhsp = جديد AstSel{flp, wordp, ثنائی_ہندسہ_اشاریہ, 1};
            Ast_گرہ_اظہاریہ* ثابت rhsp = جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}};
            إذا (useAcc) {
                initFuncp->جوڑو_Stmtsp(جديد Ast_سونپو{flp, lhsp, rhsp});
            } وإلا {
                initialTrigsp = Ast_گرہ::جوڑو_اگلا(initialTrigsp, جديد Ast_سونپو{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_درخت(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        جوڑو_Debug(i, desc);
    }
    UASSERT(trigps.ناپ() == nSenseTriggers, "Inconsistent number of trigger expressions");

    Ast_سونپو* ثابت trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    لكل (بنانشانی i = 0; i < extraTriggers.ناپ(); ++i) {
        جوڑو_Debug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    ثابت Sen_اظہاریہ_Builder::Results senResults = sen_اظہاریہ_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    لكل (Ast_گرہ_Stmt* ثابت nodep : senResults.m_inits) initFuncp->جوڑو_Stmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_چوٹی_Scope* ثابت چوٹی_Scopep = netlistp->چوٹی_Scopep();
    {
        AstCFunc* ثابت fp = kit.m_compVecp;
        // Trigger computation
        لكل (Ast_گرہ_Stmt* ثابت nodep : senResults.m_preUpdates) fp->جوڑو_Stmtsp(nodep);
        fp->جوڑو_Stmtsp(trigStmtsp);
        لكل (Ast_گرہ_Stmt* ثابت nodep : senResults.m_postUpdates) fp->جوڑو_Stmtsp(nodep);
        // Add the initialization time triggers
        إذا (initialTrigsp) {
            AstVarScope* ثابت initVscp = چوٹی_Scopep->create_عارضی("__V" + name + "DidInit", 1);
            AstIf* ثابت ifp = جديد AstIf{flp, جديد AstNot{flp, rd(initVscp)}};
            fp->جوڑو_Stmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->جوڑو_Thensp(util::سیٹ_Var(initVscp, 1));
            ifp->جوڑو_Thensp(initialTrigsp);
        }
        util::ونڈو_Check(fp);
    };
    // If there are 'pre' triggers, compute them
    إذا (kit.m_n_Pre_الفاظ) {
        AstCFunc* ثابت fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ثابت latchedp
            = new_دلیل(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ثابت nVscp = چوٹی_Scopep->create_عارضی("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(سچ);
        // Add a loop to compute the pre words
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        fp->جوڑو_Stmtsp(util::سیٹ_Var(nVscp, 0));
        fp->جوڑو_Stmtsp(loopp);
        // Loop body
        Ast_گرہ_اظہاریہ* ثابت offsetp = جديد AstConst{flp, kit.m_n_Vec_الفاظ};
        Ast_گرہ_اظہاریہ* ثابت lIdxp = جديد Ast_جوڑو{flp, rd(nVscp), offsetp};
        Ast_گرہ_اظہاریہ* ثابت lhsp = جديد AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_گرہ_اظہاریہ* ثابت aWordp = جديد AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_گرہ_اظہاریہ* ثابت bWordp = جديد AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_گرہ_اظہاریہ* ثابت rhsp = جديد AstAnd{flp, aWordp, جديد AstNot{flp, bWordp}};
        Ast_گرہ_اظہاریہ* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, n_Pre_الفاظ};
        loopp->جوڑو_Stmtsp(جديد Ast_سونپو{flp, lhsp, rhsp});
        loopp->جوڑو_Stmtsp(util::incrementVar(nVscp));
        loopp->جوڑو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});
        util::ونڈو_Check(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    إذا (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlink_Fr_پٹھ_With_اگلا()->delete_درخت();
    // Done with the trigger dump function, split as might be large
    util::ونڈو_Check(kit.m_dumpp);

    أرجع kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
صنف AwaitBeforeTrigVisitor final : عام VNVisitor {
    ثابت VNUser1_وچ_Use m_user1_وچ_Use;

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
    Sen_اظہاریہ_Builder& m_sen_اظہاریہ_Builder;
    // Generator of unique names for before-trigger function
    V3_Unique_ناں m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::سمتیہ<std::pair<AstCFunc*, Ast_Sen_درخت*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::سمتیہ<std::pair<Ast_Sen_درخت*, Ast_گرہ_اظہاریہ*>> m_sen_درخت_منزل_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_گرہ>, std::سمتیہ<Ast_Sen_شے*>> m_sen_اظہاریہ_منزل_Sen_شے;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    ساكن Ast_گرہ* get_Sen_Hash_گرہ(ثابت Ast_Sen_شے* ثابت nodep) {
        إذا (AstVarRef* ثابت varRefp = VN_CAST(nodep->sensp(), VarRef)) أرجع varRefp;
        أرجع nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    فراغ fill_Sen_اظہاریہ_منزل_Sen_شے() {
        لكل (خودکار sen_درخت_Sched : m_sen_درخت_منزل_Sched) {
            Ast_Sen_درخت* ثابت senTreep = sen_درخت_Sched.پہلا;

            لكل (Ast_Sen_شے* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_شے)) {
                ثابت VEdgeType کنارا = senItemp->کنارا_Type();
                إذا (کنارا.an_کنارا() || کنارا == VEdgeType::ET_بدلیا
                    || کنارا == VEdgeType::ET_HYBRID) {
                    m_sen_اظہاریہ_منزل_Sen_شے[*get_Sen_Hash_گرہ(senItemp)].ڈالو_پٹھ(senItemp);
                }
            }
        }

        std::سمتیہ<VNRef<Ast_گرہ>> منزل_Remove;
        لكل (ثابت خودکار& sen_اظہاریہ_منزل_Sen_درخت : m_sen_اظہاریہ_منزل_Sen_شے) {
            std::سمتیہ<Ast_Sen_شے*> senItemps = sen_اظہاریہ_منزل_Sen_درخت.دوجا;
            منزل_Remove.ڈالو_پٹھ(sen_اظہاریہ_منزل_Sen_درخت.پہلا);
            لكل (ناپ_t i = 1; i < senItemps.ناپ(); ++i) {
                إذا (senItemps[i]->کنارا_Type() != senItemps[i - 1]->کنارا_Type()) {
                    منزل_Remove.نکالو_پٹھ();
                    اكسر;
                }
            }
        }
        لكل (ثابت VNRef<Ast_گرہ> it : منزل_Remove) m_sen_اظہاریہ_منزل_Sen_شے.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<ناپ_t, std::map<ناپ_t, std::سمتیہ<Ast_گرہ_اظہاریہ*>>>
    get_ورتیا_Triggers_منزل_Trees(ثابت std::سیٹ<ناپ_t>& ورتیا_Triggers) {
        std::map<ناپ_t, std::map<ناپ_t, std::سمتیہ<Ast_گرہ_اظہاریہ*>>> ورتیا_Trigs_منزل_Using_Trees;
        لكل (خودکار sen_درخت_Sched : m_sen_درخت_منزل_Sched) {
            ثابت Ast_Sen_درخت* ثابت senTreep = sen_درخت_Sched.پہلا;
            Ast_گرہ_اظہاریہ* ثابت shedp = sen_درخت_Sched.دوجا;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            لكل (Ast_Sen_شے* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_شے)) {
                ثابت ناپ_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                إذا (ورتیا_Triggers.لبھو(idx) != ورتیا_Triggers.سرا()) {
                    ورتیا_Trigs_منزل_Using_Trees[idx / TriggerKit::لفظ_ناپ]
                                         [ناپ_t{1} << (idx % TriggerKit::لفظ_ناپ)]
                                             .ڈالو_پٹھ(shedp);
                }
            }
        }
        إذا (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            لكل (ثابت خودکار& triggers_منزل_Trees : ورتیا_Trigs_منزل_Using_Trees) {
                لكل (ثابت خودکار& بٹ_منزل_Trees : triggers_منزل_Trees.دوجا) {
                    ثابت std::سیٹ<ثابت Ast_گرہ_اظہاریہ*> exprps{بٹ_منزل_Trees.دوجا.begin(),
                                                              بٹ_منزل_Trees.دوجا.سرا()};
                    UASSERT(بٹ_منزل_Trees.دوجا.ناپ() == exprps.ناپ(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        أرجع ورتیا_Trigs_منزل_Using_Trees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_درخت* ثابت senTreep) {
        File_سطر* ثابت flp = senTreep->fileline();
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
                                            senTreep->لبھو_Basic_DType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(سچ);
            argp->direction(VDirection::داخلہ);
            funcp->جوڑو_Argsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_پٹھ(funcp, senTreep);
        }
        AstCCall* ثابت callp = جديد AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtype_سیٹ_Void();
        أرجع callp;
    }

    فراغ ویکھو(AstCAwait* ثابت nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        إذا (ثابت AstCMethodHard* ثابت c_طریقہ_Hardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            إذا (c_طریقہ_Hardp->طریقہ() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ثابت beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_گرہ* ثابت pinp = c_طریقہ_Hardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, c_طریقہ_Hardp, "No event description");
                beforeTrigp->جوڑو_Argsp(VN_AS(pinp, گرہ_اظہاریہ)->clone_درخت(خطأ));

                // Call the before-trigger function before the CAwait
                nodep->جوڑو_ایتھے_This_As_اگلا(beforeTrigp->makeStmt());
                m_sen_درخت_منزل_Sched.emplace_پٹھ(nodep->sentreep(), c_طریقہ_Hardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    فراغ ویکھو(Ast_گرہ* ثابت nodep) override { iterateChildren(nodep); }

عام:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_اظہاریہ_Builder& sen_اظہاریہ_Builder,
                           ثابت TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_اظہاریہ_Builder{sen_اظہاریہ_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_اظہاریہ_منزل_Sen_شے();

        std::سمتیہ<Ast_گرہ_اظہاریہ*> trigps;
        std::سیٹ<ناپ_t> ورتیا_Triggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        لكل (ثابت خودکار& func_منزل_ورتیا_Triggers : m_generatedFuncs) {
            AstCFunc* ثابت funcp = func_منزل_ورتیا_Triggers.پہلا;
            AstVarScope* ثابت vscp = VN_AS(funcp->user1p(), VarScope);
            File_سطر* ثابت flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_درخت* ثابت senTreep = func_منزل_ورتیا_Triggers.دوجا;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ثابت خودکار emplaceAt
                    = [flp, &trigps, &ورتیا_Triggers](Ast_گرہ_اظہاریہ* ثابت exprp, ثابت ناپ_t pos) {
                          ثابت ناپ_t نشانہ_ناپ
                              = vlstd::گیڑا_اتے_منزل_مضرب_دا<TriggerKit::لفظ_ناپ>(pos + 1);
                          إذا (trigps.سمرتھا() < نشانہ_ناپ) trigps.reserve(نشانہ_ناپ * 2);
                          طالما (trigps.ناپ() < نشانہ_ناپ) {
                              trigps.ڈالو_پٹھ(جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}});
                          }
                          trigps[pos]->delete_درخت();
                          trigps[pos] = exprp;
                          ورتیا_Triggers.داخل_کرو(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                لكل (ثابت Ast_Sen_شے* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Sen_شے)) {
                    ثابت ناپ_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_اظہاریہ_Builder.build(itemp).پہلا, idx);
                    خودکار iter = m_sen_اظہاریہ_منزل_Sen_شے.لبھو(*get_Sen_Hash_گرہ(itemp));
                    إذا (iter != m_sen_اظہاریہ_منزل_Sen_شے.سرا()) {
                        لكل (Ast_Sen_شے* ثابت additionalItemp : iter->دوجا) {
                            ثابت ناپ_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_اظہاریہ_Builder.build(additionalItemp).پہلا, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ثابت Sen_اظہاریہ_Builder::Results results
                    = m_sen_اظہاریہ_Builder.getResultsAndClearUpdates();
                لكل (Ast_گرہ_Stmt* ثابت stmtsp : results.m_inits) funcp->جوڑو_Stmtsp(stmtsp);
                لكل (Ast_گرہ_Stmt* ثابت stmtsp : results.m_preUpdates) funcp->جوڑو_Stmtsp(stmtsp);
                funcp->جوڑو_Stmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                لكل (Ast_گرہ_Stmt* ثابت stmtsp : results.m_postUpdates) funcp->جوڑو_Stmtsp(stmtsp);
            }

            ثابت std::map<ناپ_t, std::map<ناپ_t, std::سمتیہ<Ast_گرہ_اظہاریہ*>>>
                ورتیا_Trigs_منزل_Using_Trees = get_ورتیا_Triggers_منزل_Trees(ورتیا_Triggers);
            ورتیا_Triggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            ثابت خودکار getIdx = [flp](AstVarScope* ثابت scocep, VAccess access, ناپ_t idx) {
                أرجع جديد AstArraySel{flp, جديد AstVarRef{flp, scocep, access},
                                       جديد AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* ثابت argpVscp = جديد AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->جوڑو_Varsp(argpVscp);

            // Mark as ready triggered schedulers
            لكل (ثابت خودکار& triggers_منزل_Trees : ورتیا_Trigs_منزل_Using_Trees) {
                ثابت ناپ_t لفظ = triggers_منزل_Trees.پہلا;

                لكل (ثابت خودکار& بٹ_منزل_Trees : triggers_منزل_Trees.دوجا) {
                    ثابت ناپ_t ثنائی_ہندسہ_2 = بٹ_منزل_Trees.پہلا;
                    ثابت خودکار& schedulers = بٹ_منزل_Trees.دوجا;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* ثابت نقاب_Constp = جديد AstConst{flp, AstConst::Unsized64{}, ثنائی_ہندسہ_2};
                    AstAnd* ثابت condp
                        = جديد AstAnd{flp, getIdx(vscp, VAccess::READ, لفظ), نقاب_Constp};
                    AstIf* ثابت ifp = جديد AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    لكل (Ast_گرہ_اظہاریہ* ثابت schedp : schedulers) {
                        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                            flp, schedp->clone_درخت(خطأ), VCMethod::SCHED_READY};
                        callp->dtype_سیٹ_Void();
                        callp->جوڑو_Pinsp(جديد AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->جوڑو_Thensp(callp->makeStmt());
                    }
                    funcp->جوڑو_Stmtsp(ifp);
                }
            }

            AstVarScope* ثابت vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            لكل (ثابت خودکار& triggers_منزل_Trees : ورتیا_Trigs_منزل_Using_Trees) {
                ثابت ناپ_t لفظ = triggers_منزل_Trees.پہلا;
                funcp->جوڑو_Stmtsp(جديد Ast_سونپو{flp, getIdx(vscAccp, VAccess::WRITE, لفظ),
                                               جديد AstOr{flp, getIdx(vscAccp, VAccess::READ, لفظ),
                                                         getIdx(vscp, VAccess::READ, لفظ)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = افتراضي;
};

فراغ beforeTrigVisitor(AstNetlist* netlistp, Sen_اظہاریہ_Builder& sen_اظہاریہ_Builder,
                       ثابت TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_اظہاریہ_Builder, trigKit};
}

}  // namespace V3Sched
