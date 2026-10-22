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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Const.h"
#شامل "V3EmitCBase.h"
#شامل "V3EmitV.h"
#شامل "V3Order.h"
#شامل "V3Sched.h"
#شامل "V3SenExprBuilder.h"
#شامل "V3Stats.h"
#شامل "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

نامخلا V3Sched {

نامخلا {

استعمال util::new_دلیل;

AstVarScope* newLocal(AstCFunc* funcp, Ast_گرہ_DType* dtypep, ثابت std::string& name) {
    File_سطر* ثابت flp = funcp->fileline();
    AstScope* ثابت scopep = funcp->scopep();
    AstVar* ثابت varp = نیا AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(سچ);
    varp->noReset(سچ);
    funcp->جوڑو_Varsp(varp);
    AstVarScope* ثابت vscp = نیا AstVarScope{flp, scopep, varp};
    scopep->جوڑو_Varsp(vscp);
    واپس vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() ثابت {
    UASSERT(m_n_Pre_الفاظ, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->چوٹی_Scopep()->fileline();
    Ast_گرہ_DType* ثابت u32DTypep = netlistp->ڈھونڈو_UInt32_DType();
    Ast_گرہ_DType* ثابت strDtypep = netlistp->ڈھونڈو_String_DType();

    // Dumping function always slow
    ثابت std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, سچ);
    funcp->isStatic(سچ);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* ثابت eVscp = new_دلیل(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* ثابت tVscp = new_دلیل(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    ثابت خودکار rd = [flp](AstVarScope* vp) { واپس نیا AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودکار wr = [flp](AstVarScope* vp) { واپس نیا AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* ثابت vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* ثابت iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->جوڑو_Stmtsp(util::سیٹ_Var(iVscp, 0));
        // Add loop
        AstLoop* ثابت loopp = نیا AstLoop{flp};
        funcp->جوڑو_Stmtsp(loopp);
        // Loop body
        Ast_گرہ_اظہاریہ* ثابت lhsp = نیا AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_گرہ_اظہاریہ* ثابت rhsp = نیا AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_گرہ_اظہاریہ* ثابت limp = نیا AstConst{flp, AstConst::Widthed_قدر{}, 32, m_n_Vec_الفاظ};
        loopp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, lhsp, rhsp});
        loopp->جوڑو_Stmtsp(util::incrementVar(iVscp));
        loopp->جوڑو_Stmtsp(نیا AstLoopTest{flp, loopp, نیا AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* ثابت callp = نیا AstCCall{flp, m_dumpp};
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
            AstLoop* ثابت loopp = نیا AstLoop{flp};
            funcp->جوڑو_Stmtsp(loopp);
            // Loop body
            Ast_گرہ_اظہاریہ* ثابت lhsp = نیا AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_گرہ_اظہاریہ* ثابت rhsp = نیا AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_گرہ_اظہاریہ* ثابت limp = نیا AstConst{flp, AstConst::Widthed_قدر{}, 32, m_n_Pre_الفاظ};
            loopp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, lhsp, rhsp});
            loopp->جوڑو_Stmtsp(util::incrementVar(jVscp));
            loopp->جوڑو_Stmtsp(نیا AstLoopTest{flp, loopp, نیا AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* ثابت loopp = نیا AstLoop{flp};
            funcp->جوڑو_Stmtsp(loopp);
            // Loop body
            Ast_گرہ_اظہاریہ* ثابت lhsp = نیا AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_گرہ_اظہاریہ* ثابت rhsp = نیا AstConst{flp, AstConst::DTyped{}, m_لفظ_DTypep};
            Ast_گرہ_اظہاریہ* ثابت limp = نیا AstConst{flp, AstConst::Widthed_قدر{}, 32, m_n_Vec_الفاظ};
            loopp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, lhsp, rhsp});
            loopp->جوڑو_Stmtsp(util::incrementVar(jVscp));
            loopp->جوڑو_Stmtsp(نیا AstLoopTest{flp, loopp, نیا AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* ثابت callp = نیا AstCCall{flp, m_dumpp};
        callp->dtype_سیٹ_Void();
        callp->جوڑو_Argsp(rd(pVscp));
        callp->جوڑو_Argsp(
            نیا AstConcatN{flp, rd(tVscp), نیا AstConst{flp, AstConst::String{}, " pre"}});
        funcp->جوڑو_Stmtsp(callp->makeStmt());
    }

    واپس funcp;
}

AstCFunc* TriggerKit::create_Any_سیٹ_Func(AstUnpackArrayDType* ثابت dtypep) ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->چوٹی_Scopep()->fileline();
    Ast_گرہ_DType* ثابت u32DTypep = netlistp->ڈھونڈو_UInt32_DType();

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
    ثابت خودکار rd = [flp](AstVarScope* vp) { واپس نیا AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* ثابت loopp = نیا AstLoop{flp};
    funcp->جوڑو_Stmtsp(util::سیٹ_Var(nVscp, 0));
    funcp->جوڑو_Stmtsp(loopp);
    funcp->جوڑو_Stmtsp(نیا AstCReturn{flp, نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}});

    // Loop body
    ثابت uint32_t n_الفاظ = dtypep->elementsConst();
    Ast_گرہ_اظہاریہ* ثابت condp = نیا AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_گرہ_Stmt* ثابت thenp = نیا AstCReturn{flp, نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}};
    Ast_گرہ_اظہاریہ* ثابت limp = نیا AstConst{flp, AstConst::Widthed_قدر{}, 32, n_الفاظ};
    loopp->جوڑو_Stmtsp(نیا AstIf{flp, condp, thenp});
    loopp->جوڑو_Stmtsp(util::incrementVar(nVscp));
    loopp->جوڑو_Stmtsp(نیا AstLoopTest{flp, loopp, نیا AstLt{flp, rd(nVscp), limp}});

    //مکمل
    واپس funcp;
}
AstCFunc* TriggerKit::createClearFunc() ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->چوٹی_Scopep()->fileline();
    Ast_گرہ_DType* ثابت u32DTypep = netlistp->ڈھونڈو_UInt32_DType();

    // Create function
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(سچ);

    // Add arguments
    AstVarScope* ثابت oVscp = new_دلیل(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ثابت خودکار rd = [flp](AstVarScope* vp) { واپس نیا AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودکار wr = [flp](AstVarScope* vp) { واپس نیا AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ثابت loopp = نیا AstLoop{flp};
    funcp->جوڑو_Stmtsp(util::سیٹ_Var(nVscp, 0));
    funcp->جوڑو_Stmtsp(loopp);

    // Loop body
    Ast_گرہ_اظہاریہ* ثابت lhsp = نیا AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_گرہ_اظہاریہ* ثابت rhsp = نیا AstConst{flp, AstConst::DTyped{}, m_لفظ_DTypep};
    Ast_گرہ_اظہاریہ* ثابت limp = نیا AstConst{flp, AstConst::Widthed_قدر{}, 32, m_n_Vec_الفاظ};
    loopp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, lhsp, rhsp});
    loopp->جوڑو_Stmtsp(util::incrementVar(nVscp));
    loopp->جوڑو_Stmtsp(نیا AstLoopTest{flp, loopp, نیا AstLt{flp, rd(nVscp), limp}});

    //مکمل
    واپس funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ثابت oDtypep,
                                       AstUnpackArrayDType* ثابت iDtypep) ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->چوٹی_Scopep()->fileline();
    Ast_گرہ_DType* ثابت u32DTypep = netlistp->ڈھونڈو_UInt32_DType();

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
    ثابت خودکار rd = [flp](AstVarScope* vp) { واپس نیا AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودکار wr = [flp](AstVarScope* vp) { واپس نیا AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ثابت loopp = نیا AstLoop{flp};
    funcp->جوڑو_Stmtsp(util::سیٹ_Var(nVscp, 0));
    funcp->جوڑو_Stmtsp(loopp);

    // Loop body
    Ast_گرہ_اظہاریہ* ثابت lhsp = نیا AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_گرہ_اظہاریہ* ثابت oWordp = نیا AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_گرہ_اظہاریہ* ثابت iWordp = نیا AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_گرہ_اظہاریہ* ثابت rhsp = نیا AstOr{flp, oWordp, iWordp};
    AstConst* ثابت outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ثابت داخلہ_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_گرہ_اظہاریہ* ثابت limp = outputRangeLeftp->عدد().مقصد_SInt() < داخلہ_Range_Leftp->عدد().مقصد_SInt()
                                  ? outputRangeLeftp->clone_درخت_خالص(جھوٹ)
                                  : داخلہ_Range_Leftp->clone_درخت_خالص(جھوٹ);
    loopp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, lhsp, rhsp});
    loopp->جوڑو_Stmtsp(util::incrementVar(nVscp));
    loopp->جوڑو_Stmtsp(نیا AstLoopTest{flp, loopp, نیا AstLte{flp, rd(nVscp), limp}});

    //مکمل
    واپس funcp;
}

Ast_گرہ_اظہاریہ* TriggerKit::new_Any_سیٹ_Call(AstVarScope* ثابت vscp) ثابت {
    File_سطر* ثابت flp = v3Global.rootp()->چوٹی_Scopep()->fileline();
    اگر (!m_n_Vec_الفاظ) واپس نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}};

    AstCFunc* funcp = خالی_اشاریہ;
    اگر (vscp->dtypep() == m_trigVecDTypep) {
        اگر (!m_any_سیٹ_Vecp) m_any_سیٹ_Vecp = create_Any_سیٹ_Func(m_trigVecDTypep);
        funcp = m_any_سیٹ_Vecp;
    } ورنہ اگر (vscp->dtypep() == m_trigExtDTypep) {
        اگر (!m_any_سیٹ_Extp) m_any_سیٹ_Extp = create_Any_سیٹ_Func(m_trigExtDTypep);
        funcp = m_any_سیٹ_Extp;
    } ورنہ {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ثابت callp = نیا AstCCall{flp, funcp};
    callp->جوڑو_Argsp(نیا AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_سیٹ_ثنائی_ہندسہ();
    واپس callp;
}
Ast_گرہ_Stmt* TriggerKit::newClearCall(AstVarScope* ثابت vscp) ثابت {
    اگر (!m_n_Vec_الفاظ) واپس خالی_اشاریہ;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    اگر (!m_clearp) m_clearp = createClearFunc();
    File_سطر* ثابت flp = v3Global.rootp()->چوٹی_Scopep()->fileline();
    AstCCall* ثابت callp = نیا AstCCall{flp, m_clearp};
    callp->جوڑو_Argsp(نیا AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_سیٹ_Void();
    واپس callp->makeStmt();
}
Ast_گرہ_Stmt* TriggerKit::newOrIntoCall(AstVarScope* ثابت oVscp, AstVarScope* ثابت iVscp) ثابت {
    اگر (!m_n_Vec_الفاظ) واپس خالی_اشاریہ;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    ثابت سائز_t نقاب
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[نقاب];
    اگر (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_سطر* ثابت flp = v3Global.rootp()->چوٹی_Scopep()->fileline();
    AstCCall* ثابت callp = نیا AstCCall{flp, funcp};
    callp->جوڑو_Argsp(نیا AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->جوڑو_Argsp(نیا AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtype_سیٹ_Void();
    واپس callp->makeStmt();
}

Ast_گرہ_Stmt* TriggerKit::new_Comp_بنیاد_Call() ثابت {
    اگر (!m_n_Vec_الفاظ) واپس خالی_اشاریہ;
    File_سطر* ثابت flp = v3Global.rootp()->چوٹی_Scopep()->fileline();
    AstCCall* ثابت callp = نیا AstCCall{flp, m_compVecp};
    callp->dtype_سیٹ_Void();
    واپس callp->makeStmt();
}

Ast_گرہ_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ثابت {
    اگر (!m_n_Pre_الفاظ) واپس خالی_اشاریہ;
    File_سطر* ثابت flp = v3Global.rootp()->چوٹی_Scopep()->fileline();
    AstCCall* ثابت callp = نیا AstCCall{flp, m_compExtp};
    callp->جوڑو_Argsp(نیا AstVarRef{flp, vscp, VAccess::READ});
    callp->dtype_سیٹ_Void();
    واپس callp->makeStmt();
}

Ast_گرہ_Stmt* TriggerKit::newDumpCall(AstVarScope* ثابت vscp, ثابت std::string& لیبل,
                                     منطقی debugOnly) ثابت {
    اگر (!m_n_Vec_الفاظ) واپس خالی_اشاریہ;
    AstCFunc* funcp = خالی_اشاریہ;
    اگر (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } ورنہ اگر (vscp->dtypep() == m_trigExtDTypep) {
        اگر (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } ورنہ {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_سطر* ثابت flp = v3Global.rootp()->چوٹی_Scopep()->fileline();
    AstCCall* ثابت callp = نیا AstCCall{flp, funcp};
    callp->جوڑو_Argsp(نیا AstVarRef{flp, vscp, VAccess::READ});
    callp->جوڑو_Argsp(نیا AstConst{flp, AstConst::String{}, لیبل});
    callp->dtype_سیٹ_Void();
    AstCStmt* ثابت cstmtp = نیا AstCStmt{flp};
    cstmtp->جوڑو("#ifdef VL_DEBUG\n");
    اگر (debugOnly) {
        cstmtp->جوڑو("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->جوڑو(callp->makeStmt());
        cstmtp->جوڑو("}\n");
    } ورنہ {
        cstmtp->جوڑو(callp->makeStmt());
    }
    cstmtp->جوڑو("#endif");
    واپس cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(ثابت std::string& name) ثابت {
    اگر (!m_n_Vec_الفاظ) واپس خالی_اشاریہ;
    واپس v3Global.rootp()->چوٹی_Scopep()->create_عارضی("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extra_Trigger_اشاریہ(uint32_t اشاریہ) ثابت {
    UASSERT(اشاریہ <= m_n_Extra_الفاظ * لفظ_سائز, "Invalid external trigger index");
    واپس اشاریہ + m_n_Sense_الفاظ * لفظ_سائز;
}

std::سیٹ<uint32_t> TriggerKit::triggerIndices(ثابت Ast_Sen_درخت* senTreep, منطقی pre) ثابت {
    std::سیٹ<uint32_t> indices;
    برائے (ثابت Ast_Sen_شے* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), Sen_شے)) {
        uint32_t اشاریہ = m_senItem2TrigIdx.at(*sip);
        اگر (pre) {
            UASSERT_OBJ(اشاریہ < m_n_Pre_الفاظ * لفظ_سائز, sip, "Not a 'pre' trigger");
            اشاریہ += m_n_Vec_الفاظ * لفظ_سائز;
        }
        indices.داخل_کرو(اشاریہ);
    }
    واپس indices;
}

Ast_گرہ_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t اشاریہ) ثابت {
    اشاریہ = extra_Trigger_اشاریہ(اشاریہ);
    ثابت uint32_t لفظ_اشاریہ = اشاریہ / لفظ_سائز;
    ثابت uint32_t ثنائی_ہندسہ_اشاریہ = اشاریہ % لفظ_سائز;
    File_سطر* ثابت flp = vscp->fileline();
    AstVarRef* ثابت refp = نیا AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_گرہ_اظہاریہ* ثابت wordp = نیا AstArraySel{flp, refp, static_cast<صحیح>(لفظ_اشاریہ)};
    Ast_گرہ_اظہاریہ* ثابت trigLhsp = نیا AstSel{flp, wordp, static_cast<صحیح>(ثنائی_ہندسہ_اشاریہ), 1};
    Ast_گرہ_اظہاریہ* ثابت trigRhsp = نیا AstVarRef{flp, vscp, VAccess::READ};
    واپس نیا Ast_تفویض{flp, trigLhsp, trigRhsp};
}

خالی TriggerKit::جوڑو_Extra_Trigger_Assignment(AstVarScope* vscp, uint32_t اشاریہ) ثابت {
    File_سطر* ثابت flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_گرہ* ثابت setp = newExtraTriggerAssignment(vscp, اشاریہ);
    setp->جوڑو_اگلا(نیا Ast_تفویض{flp, نیا AstVarRef{flp, vscp, VAccess::WRITE},
                                نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
خالی TriggerKit::جوڑو_قدر_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t اشاریہ) ثابت {
    اشاریہ += m_n_Sense_الفاظ * لفظ_سائز;
    ثابت uint32_t لفظ_اشاریہ = اشاریہ / لفظ_سائز;
    ثابت uint32_t ثنائی_ہندسہ_اشاریہ = اشاریہ % لفظ_سائز;

    Ast_چوٹی_Scope* ثابت چوٹی_Scopep = netlistp->چوٹی_Scopep();
    File_سطر* ثابت flp = netlistp->fileline();
    Ast_گرہ_DType* ثابت dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    اگر (ثابت AstBasicDType* ثابت bdtypep = VN_CAST(dtypep, BasicDType)) {
        اگر (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            واپس;
        }
    } ورنہ اگر (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, گرہ_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        واپس;
    }

    ثابت خودکار rdInst = [flp](AstVarScope* vp) { واپس نیا AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودکار wr_پچھلا = [flp](AstVarScope* vp) { واپس نیا AstVarRef{flp, vp, VAccess::WRITE}; };
    ثابت خودکار rd_پچھلا = [flp](AstVarScope* vp) { واپس نیا AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    ثابت std::string پچھلا_Name = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* ثابت پچھلا_Vscp = چوٹی_Scopep->create_عارضی(پچھلا_Name, instVscp->dtypep());

    // Initialize prev = inst
    اگر (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = نیا AstCMethodHard{
            flp, wr_پچھلا(پچھلا_Vscp), VCMethod::UNPACKED_تفویض, rdInst(instVscp)};
        cmhp->dtype_سیٹ_Void();
        initFuncp->جوڑو_Stmtsp(cmhp->makeStmt());
    } ورنہ {
        initFuncp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, wr_پچھلا(پچھلا_Vscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_گرہ_اظہاریہ* neqp;
    اگر (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp
            = نیا AstCMethodHard{flp, rd_پچھلا(پچھلا_Vscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtype_سیٹ_ثنائی_ہندسہ();
        neqp = cmhp;
    } ورنہ {
        neqp = نیا AstNeq{flp, rdInst(instVscp), rd_پچھلا(پچھلا_Vscp)};
    }

    // Build post-update: prev = inst
    Ast_گرہ* updp;
    اگر (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = نیا AstCMethodHard{
            flp, wr_پچھلا(پچھلا_Vscp), VCMethod::UNPACKED_تفویض, rdInst(instVscp)};
        cmhp->dtype_سیٹ_Void();
        updp = cmhp->makeStmt();
    } ورنہ {
        updp = نیا Ast_تفویض{flp, wr_پچھلا(پچھلا_Vscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ثابت refp = نیا AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_گرہ_اظہاریہ* ثابت wordp = نیا AstArraySel{flp, refp, static_cast<صحیح>(لفظ_اشاریہ)};
    Ast_گرہ_اظہاریہ* ثابت trigLhsp = نیا AstSel{flp, wordp, static_cast<صحیح>(ثنائی_ہندسہ_اشاریہ), 1};
    Ast_گرہ* ثابت setp = نیا Ast_تفویض{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->جوڑو_اگلا(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ثابت std::string& name, منطقی slow, uint32_t n_Sense_الفاظ,
                       uint32_t n_Extra_الفاظ, uint32_t n_Pre_الفاظ,
                       std::unordered_map<VNRef<ثابت Ast_Sen_شے>, سائز_t> senItem2TrigIdx,
                       ثابت std::سمتیہ<ثابت Ast_Sen_شے*>& senItemps, منطقی useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_الفاظ{n_Sense_الفاظ}
    , m_n_Extra_الفاظ{n_Extra_الفاظ}
    , m_n_Pre_الفاظ{n_Pre_الفاظ}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trig_Idx2_Sen_شے{senItemps} {
    // If no triggers, we don't need to generate anything
    اگر (!m_n_Vec_الفاظ) واپس;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trig_Idx2_Sen_شے.resize((m_n_Vec_الفاظ + m_n_Pre_الفاظ) * لفظ_سائز);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_الفاظ * لفظ_سائز,
              m_trig_Idx2_Sen_شے.begin() + m_n_Vec_الفاظ * لفظ_سائز);
    AstNetlist* ثابت netlistp = v3Global.rootp();
    AstScope* ثابت scopep = netlistp->چوٹی_Scopep()->scopep();
    File_سطر* ثابت flp = scopep->fileline();
    // Data type of a single trigger word
    m_لفظ_DTypep = netlistp->ڈھونڈو_ثنائی_ہندسہ_DType(لفظ_سائز, لفظ_سائز, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ثابت rp = نیا AstRange{flp, static_cast<صحیح>(m_n_Vec_الفاظ - 1), 0};
    m_trigVecDTypep = نیا AstUnpackArrayDType{flp, m_لفظ_DTypep, rp};
    netlistp->typeTablep()->جوڑو_Typesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    اگر (m_n_Pre_الفاظ) {
        AstRange* ثابت ep = نیا AstRange{flp, static_cast<صحیح>(m_n_Vec_الفاظ + m_n_Pre_الفاظ - 1), 0};
        m_trigExtDTypep = نیا AstUnpackArrayDType{flp, m_لفظ_DTypep, ep};
        netlistp->typeTablep()->جوڑو_Typesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } ورنہ {
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
    اگر (useAcc) {
        m_vscAccp
            = netlistp->چوٹی_Scopep()->create_عارضی("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->is_اندرونی(سچ);
    }
}

Ast_تفویض* TriggerKit::createSenTrigVecAssignment(AstVarScope* ثابت ہدف,
                                                  std::سمتیہ<Ast_گرہ_اظہاریہ*>& trigps) {
    File_سطر* ثابت flp = ہدف->fileline();
    Ast_تفویض* trigStmtsp = خالی_اشاریہ;
    // Assign sense triggers vector one word at a time
    برائے (سائز_t i = 0; i < trigps.سائز(); i += لفظ_سائز) {
        // Concatenate all bits in this trigger word using a balanced
        برائے (uint32_t level = 0; level < لفظ_سائز_LOG2; ++level) {
            ثابت uint32_t stride = 1 << level;
            برائے (uint32_t j = 0; j < لفظ_سائز; j += 2 * stride) {
                trigps[i + j] = نیا AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = خالی_اشاریہ;
            }
        }

        // Set the whole word in the trigger vector
        ثابت صحیح لفظ_اشاریہ = static_cast<صحیح>(i / لفظ_سائز);
        AstArraySel* ثابت aselp
            = نیا AstArraySel{flp, نیا AstVarRef{flp, ہدف, VAccess::WRITE}, لفظ_اشاریہ};
        trigStmtsp = Ast_گرہ::جوڑو_اگلا(trigStmtsp, نیا Ast_تفویض{flp, aselp, trigps[i]});
    }
    واپس trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ثابت initFuncp,  //
                              Sen_اظہاریہ_Builder& sen_اظہاریہ_Builder,  //
                              ثابت std::سمتیہ<ثابت Ast_Sen_درخت*>& preTreeps,  //
                              ثابت std::سمتیہ<ثابت Ast_Sen_درخت*>& senTreeps,  //
                              ثابت string& name,  //
                              ثابت ExtraTriggers& extraTriggers,  //
                              منطقی slow,  //
                              منطقی useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::سمتیہ<ثابت Ast_Sen_شے*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<ثابت Ast_Sen_شے>, سائز_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    برائے (ثابت Ast_Sen_درخت* ثابت senTreep : preTreeps) {
        برائے (ثابت Ast_Sen_شے *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_شے);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت خودکار pair = senItem2TrigIdx.emplace(*itemp, senItemps.سائز());
            اگر (pair.دوسرا) senItemps.ڈالو_عقب(itemp);
        }
    }
    ثابت uint32_t n_Pre_Sen_اشیاء = senItemps.سائز();
    V3Stats::جوڑو_Stat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_اشیاء);
    // Number of pre triggers, rounded up to a full word.
    ثابت uint32_t nPreTriggers = vlstd::باری_اوپر_مقصد_مضرب_کا<لفظ_سائز>(senItemps.سائز());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ثابت uint32_t n_Pre_الفاظ = nPreTriggers / لفظ_سائز;

    // Process the rest of the trees
    برائے (ثابت Ast_Sen_درخت* ثابت senTreep : senTreeps) {
        برائے (ثابت Ast_Sen_شے *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_شے);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت خودکار pair = senItem2TrigIdx.emplace(*itemp, senItemps.سائز());
            اگر (pair.دوسرا) senItemps.ڈالو_عقب(itemp);
        }
    }
    ثابت uint32_t n_Sen_اشیاء = senItemps.سائز() - nPreTriggers;
    V3Stats::جوڑو_Stat("Scheduling, '" + name + "' sense triggers", n_Sen_اشیاء + n_Pre_Sen_اشیاء);
    // Number of sense triggers, rounded up to a full word
    ثابت uint32_t nSenseTriggers = vlstd::باری_اوپر_مقصد_مضرب_کا<لفظ_سائز>(senItemps.سائز());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ثابت uint32_t n_Sense_الفاظ = nSenseTriggers / لفظ_سائز;

    // Allocate space for the extra triggers
    V3Stats::جوڑو_Stat("Scheduling, '" + name + "' extra triggers", extraTriggers.سائز());
    // Number of extra triggers, rounded up to a full word.
    ثابت uint32_t nExtraTriggers = vlstd::باری_اوپر_مقصد_مضرب_کا<لفظ_سائز>(extraTriggers.سائز());
    ثابت uint32_t n_Extra_الفاظ = nExtraTriggers / لفظ_سائز;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_الفاظ, n_Extra_الفاظ, n_Pre_الفاظ, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    اگر (!kit.m_n_Vec_الفاظ) واپس kit;

    File_سطر* ثابت flp = netlistp->چوٹی_Scopep()->fileline();

    // Creates read/write reference
    ثابت خودکار rd = [flp](AstVarScope* vp) { واپس نیا AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودکار wr = [flp](AstVarScope* vp) { واپس نیا AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* ثابت dumpTrgp
        = new_دلیل(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* ثابت dumpTagp
        = new_دلیل(kit.m_dumpp, netlistp->ڈھونڈو_String_DType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* ثابت ifp = نیا AstIf{flp, نیا AstLogNot{flp, kit.new_Any_سیٹ_Call(dumpTrgp)}};
        kit.m_dumpp->جوڑو_Stmtsp(ifp);
        AstCStmt* ثابت cstmtp = نیا AstCStmt{flp};
        ifp->جوڑو_Thensp(cstmtp);
        cstmtp->جوڑو("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->جوڑو(rd(dumpTagp));
        cstmtp->جوڑو(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    ثابت خودکار جوڑو_Debug = [&](uint32_t اشاریہ, ثابت string& متن) {
        ثابت صحیح wrd_اشاریہ = static_cast<صحیح>(اشاریہ / لفظ_سائز);
        ثابت صحیح ثنائی_ہندسہ_اشاریہ = static_cast<صحیح>(اشاریہ % لفظ_سائز);
        Ast_گرہ_اظہاریہ* ثابت aselp = نیا AstArraySel{flp, rd(dumpTrgp), wrd_اشاریہ};
        Ast_گرہ_اظہاریہ* ثابت condp = نیا AstSel{flp, aselp, ثنائی_ہندسہ_اشاریہ, 1};
        AstIf* ثابت ifp = نیا AstIf{flp, condp};
        kit.m_dumpp->جوڑو_Stmtsp(ifp);
        AstCStmt* ثابت cstmtp = نیا AstCStmt{flp};
        ifp->جوڑو_Thensp(cstmtp);
        cstmtp->جوڑو("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->جوڑو(rd(dumpTagp));
        cstmtp->جوڑو(" + \"' region trigger index " + std::مقصد_string(اشاریہ) + " is active: " + متن
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::سمتیہ<Ast_گرہ_اظہاریہ*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_گرہ_Stmt* initialTrigsp = خالی_اشاریہ;
    برائے (سائز_t i = 0; i < senItemps.سائز(); ++i) {
        ثابت Ast_Sen_شے* ثابت senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        اگر (!senItemp) {
            trigps.emplace_عقب(نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}});
            جاری;
        }

        // Create the trigger computation expression
        ثابت خودکار& pair = sen_اظہاریہ_Builder.build(senItemp);
        trigps.emplace_عقب(pair.پہلا);

        // Add initialization time trigger
        اگر (pair.دوسرا || v3Global.opt.x_Initial_کنارہ()) {
            ثابت صحیح wrd_اشاریہ = static_cast<صحیح>(i / لفظ_سائز);
            ثابت صحیح ثنائی_ہندسہ_اشاریہ = static_cast<صحیح>(i % لفظ_سائز);
            Ast_گرہ_اظہاریہ* ثابت wordp = نیا AstArraySel{flp, wr(kit.m_vscp), wrd_اشاریہ};
            Ast_گرہ_اظہاریہ* ثابت lhsp = نیا AstSel{flp, wordp, ثنائی_ہندسہ_اشاریہ, 1};
            Ast_گرہ_اظہاریہ* ثابت rhsp = نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}};
            اگر (useAcc) {
                initFuncp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, lhsp, rhsp});
            } ورنہ {
                initialTrigsp = Ast_گرہ::جوڑو_اگلا(initialTrigsp, نیا Ast_تفویض{flp, lhsp, rhsp});
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
    UASSERT(trigps.سائز() == nSenseTriggers, "Inconsistent number of trigger expressions");

    Ast_تفویض* ثابت trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    برائے (غیرعلامتی i = 0; i < extraTriggers.سائز(); ++i) {
        جوڑو_Debug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    ثابت Sen_اظہاریہ_Builder::Results senResults = sen_اظہاریہ_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    برائے (Ast_گرہ_Stmt* ثابت nodep : senResults.m_inits) initFuncp->جوڑو_Stmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_چوٹی_Scope* ثابت چوٹی_Scopep = netlistp->چوٹی_Scopep();
    {
        AstCFunc* ثابت fp = kit.m_compVecp;
        // Trigger computation
        برائے (Ast_گرہ_Stmt* ثابت nodep : senResults.m_preUpdates) fp->جوڑو_Stmtsp(nodep);
        fp->جوڑو_Stmtsp(trigStmtsp);
        برائے (Ast_گرہ_Stmt* ثابت nodep : senResults.m_postUpdates) fp->جوڑو_Stmtsp(nodep);
        // Add the initialization time triggers
        اگر (initialTrigsp) {
            AstVarScope* ثابت initVscp = چوٹی_Scopep->create_عارضی("__V" + name + "DidInit", 1);
            AstIf* ثابت ifp = نیا AstIf{flp, نیا AstNot{flp, rd(initVscp)}};
            fp->جوڑو_Stmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->جوڑو_Thensp(util::سیٹ_Var(initVscp, 1));
            ifp->جوڑو_Thensp(initialTrigsp);
        }
        util::بانٹو_Check(fp);
    };
    // If there are 'pre' triggers, compute them
    اگر (kit.m_n_Pre_الفاظ) {
        AstCFunc* ثابت fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ثابت latchedp
            = new_دلیل(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ثابت nVscp = چوٹی_Scopep->create_عارضی("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(سچ);
        // Add a loop to compute the pre words
        AstLoop* ثابت loopp = نیا AstLoop{flp};
        fp->جوڑو_Stmtsp(util::سیٹ_Var(nVscp, 0));
        fp->جوڑو_Stmtsp(loopp);
        // Loop body
        Ast_گرہ_اظہاریہ* ثابت offsetp = نیا AstConst{flp, kit.m_n_Vec_الفاظ};
        Ast_گرہ_اظہاریہ* ثابت lIdxp = نیا Ast_جوڑو{flp, rd(nVscp), offsetp};
        Ast_گرہ_اظہاریہ* ثابت lhsp = نیا AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_گرہ_اظہاریہ* ثابت aWordp = نیا AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_گرہ_اظہاریہ* ثابت bWordp = نیا AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_گرہ_اظہاریہ* ثابت rhsp = نیا AstAnd{flp, aWordp, نیا AstNot{flp, bWordp}};
        Ast_گرہ_اظہاریہ* ثابت limp = نیا AstConst{flp, AstConst::Widthed_قدر{}, 32, n_Pre_الفاظ};
        loopp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, lhsp, rhsp});
        loopp->جوڑو_Stmtsp(util::incrementVar(nVscp));
        loopp->جوڑو_Stmtsp(نیا AstLoopTest{flp, loopp, نیا AstLt{flp, rd(nVscp), limp}});
        util::بانٹو_Check(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    اگر (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlink_Fr_عقب_With_اگلا()->delete_درخت();
    // Done with the trigger dump function, split as might be large
    util::بانٹو_Check(kit.m_dumpp);

    واپس kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
جماعت AwaitBeforeTrigVisitor final : عوامی VNVisitor {
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;

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
    V3_Unique_نام m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::سمتیہ<std::pair<AstCFunc*, Ast_Sen_درخت*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::سمتیہ<std::pair<Ast_Sen_درخت*, Ast_گرہ_اظہاریہ*>> m_sen_درخت_مقصد_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_گرہ>, std::سمتیہ<Ast_Sen_شے*>> m_sen_اظہاریہ_مقصد_Sen_شے;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    ساکن Ast_گرہ* get_Sen_Hash_گرہ(ثابت Ast_Sen_شے* ثابت nodep) {
        اگر (AstVarRef* ثابت varRefp = VN_CAST(nodep->sensp(), VarRef)) واپس varRefp;
        واپس nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    خالی fill_Sen_اظہاریہ_مقصد_Sen_شے() {
        برائے (خودکار sen_درخت_Sched : m_sen_درخت_مقصد_Sched) {
            Ast_Sen_درخت* ثابت senTreep = sen_درخت_Sched.پہلا;

            برائے (Ast_Sen_شے* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_شے)) {
                ثابت VEdgeType کنارہ = senItemp->کنارہ_Type();
                اگر (کنارہ.an_کنارہ() || کنارہ == VEdgeType::ET_بدلا
                    || کنارہ == VEdgeType::ET_HYBRID) {
                    m_sen_اظہاریہ_مقصد_Sen_شے[*get_Sen_Hash_گرہ(senItemp)].ڈالو_عقب(senItemp);
                }
            }
        }

        std::سمتیہ<VNRef<Ast_گرہ>> مقصد_Remove;
        برائے (ثابت خودکار& sen_اظہاریہ_مقصد_Sen_درخت : m_sen_اظہاریہ_مقصد_Sen_شے) {
            std::سمتیہ<Ast_Sen_شے*> senItemps = sen_اظہاریہ_مقصد_Sen_درخت.دوسرا;
            مقصد_Remove.ڈالو_عقب(sen_اظہاریہ_مقصد_Sen_درخت.پہلا);
            برائے (سائز_t i = 1; i < senItemps.سائز(); ++i) {
                اگر (senItemps[i]->کنارہ_Type() != senItemps[i - 1]->کنارہ_Type()) {
                    مقصد_Remove.نکالو_عقب();
                    توڑو;
                }
            }
        }
        برائے (ثابت VNRef<Ast_گرہ> it : مقصد_Remove) m_sen_اظہاریہ_مقصد_Sen_شے.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<سائز_t, std::map<سائز_t, std::سمتیہ<Ast_گرہ_اظہاریہ*>>>
    get_مستعمل_Triggers_مقصد_Trees(ثابت std::سیٹ<سائز_t>& مستعمل_Triggers) {
        std::map<سائز_t, std::map<سائز_t, std::سمتیہ<Ast_گرہ_اظہاریہ*>>> مستعمل_Trigs_مقصد_Using_Trees;
        برائے (خودکار sen_درخت_Sched : m_sen_درخت_مقصد_Sched) {
            ثابت Ast_Sen_درخت* ثابت senTreep = sen_درخت_Sched.پہلا;
            Ast_گرہ_اظہاریہ* ثابت shedp = sen_درخت_Sched.دوسرا;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            برائے (Ast_Sen_شے* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_شے)) {
                ثابت سائز_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                اگر (مستعمل_Triggers.ڈھونڈو(idx) != مستعمل_Triggers.انتہا()) {
                    مستعمل_Trigs_مقصد_Using_Trees[idx / TriggerKit::لفظ_سائز]
                                         [سائز_t{1} << (idx % TriggerKit::لفظ_سائز)]
                                             .ڈالو_عقب(shedp);
                }
            }
        }
        اگر (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            برائے (ثابت خودکار& triggers_مقصد_Trees : مستعمل_Trigs_مقصد_Using_Trees) {
                برائے (ثابت خودکار& بٹ_مقصد_Trees : triggers_مقصد_Trees.دوسرا) {
                    ثابت std::سیٹ<ثابت Ast_گرہ_اظہاریہ*> exprps{بٹ_مقصد_Trees.دوسرا.begin(),
                                                              بٹ_مقصد_Trees.دوسرا.انتہا()};
                    UASSERT(بٹ_مقصد_Trees.دوسرا.سائز() == exprps.سائز(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        واپس مستعمل_Trigs_مقصد_Using_Trees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_درخت* ثابت senTreep) {
        File_سطر* ثابت flp = senTreep->fileline();
        اگر (!senTreep->user1p()) {
            AstCFunc* ثابت funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), جھوٹ);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* ثابت tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(سچ);

            AstVar* ثابت argp = نیا AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->ڈھونڈو_Basic_DType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(سچ);
            argp->direction(VDirection::داخلہ);
            funcp->جوڑو_Argsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_عقب(funcp, senTreep);
        }
        AstCCall* ثابت callp = نیا AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtype_سیٹ_Void();
        واپس callp;
    }

    خالی دورہ(AstCAwait* ثابت nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        اگر (ثابت AstCMethodHard* ثابت c_طریقہ_Hardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            اگر (c_طریقہ_Hardp->طریقہ() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ثابت beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_گرہ* ثابت pinp = c_طریقہ_Hardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, c_طریقہ_Hardp, "No event description");
                beforeTrigp->جوڑو_Argsp(VN_AS(pinp, گرہ_اظہاریہ)->clone_درخت(جھوٹ));

                // Call the before-trigger function before the CAwait
                nodep->جوڑو_یہاں_This_As_اگلا(beforeTrigp->makeStmt());
                m_sen_درخت_مقصد_Sched.emplace_عقب(nodep->sentreep(), c_طریقہ_Hardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    خالی دورہ(Ast_گرہ* ثابت nodep) override { iterateChildren(nodep); }

عوامی:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_اظہاریہ_Builder& sen_اظہاریہ_Builder,
                           ثابت TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_اظہاریہ_Builder{sen_اظہاریہ_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_اظہاریہ_مقصد_Sen_شے();

        std::سمتیہ<Ast_گرہ_اظہاریہ*> trigps;
        std::سیٹ<سائز_t> مستعمل_Triggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        برائے (ثابت خودکار& func_مقصد_مستعمل_Triggers : m_generatedFuncs) {
            AstCFunc* ثابت funcp = func_مقصد_مستعمل_Triggers.پہلا;
            AstVarScope* ثابت vscp = VN_AS(funcp->user1p(), VarScope);
            File_سطر* ثابت flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_درخت* ثابت senTreep = func_مقصد_مستعمل_Triggers.دوسرا;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ثابت خودکار emplaceAt
                    = [flp, &trigps, &مستعمل_Triggers](Ast_گرہ_اظہاریہ* ثابت exprp, ثابت سائز_t pos) {
                          ثابت سائز_t ہدف_سائز
                              = vlstd::باری_اوپر_مقصد_مضرب_کا<TriggerKit::لفظ_سائز>(pos + 1);
                          اگر (trigps.استعداد() < ہدف_سائز) trigps.reserve(ہدف_سائز * 2);
                          جب (trigps.سائز() < ہدف_سائز) {
                              trigps.ڈالو_عقب(نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}});
                          }
                          trigps[pos]->delete_درخت();
                          trigps[pos] = exprp;
                          مستعمل_Triggers.داخل_کرو(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                برائے (ثابت Ast_Sen_شے* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Sen_شے)) {
                    ثابت سائز_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_اظہاریہ_Builder.build(itemp).پہلا, idx);
                    خودکار iter = m_sen_اظہاریہ_مقصد_Sen_شے.ڈھونڈو(*get_Sen_Hash_گرہ(itemp));
                    اگر (iter != m_sen_اظہاریہ_مقصد_Sen_شے.انتہا()) {
                        برائے (Ast_Sen_شے* ثابت additionalItemp : iter->دوسرا) {
                            ثابت سائز_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_اظہاریہ_Builder.build(additionalItemp).پہلا, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ثابت Sen_اظہاریہ_Builder::Results results
                    = m_sen_اظہاریہ_Builder.getResultsAndClearUpdates();
                برائے (Ast_گرہ_Stmt* ثابت stmtsp : results.m_inits) funcp->جوڑو_Stmtsp(stmtsp);
                برائے (Ast_گرہ_Stmt* ثابت stmtsp : results.m_preUpdates) funcp->جوڑو_Stmtsp(stmtsp);
                funcp->جوڑو_Stmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                برائے (Ast_گرہ_Stmt* ثابت stmtsp : results.m_postUpdates) funcp->جوڑو_Stmtsp(stmtsp);
            }

            ثابت std::map<سائز_t, std::map<سائز_t, std::سمتیہ<Ast_گرہ_اظہاریہ*>>>
                مستعمل_Trigs_مقصد_Using_Trees = get_مستعمل_Triggers_مقصد_Trees(مستعمل_Triggers);
            مستعمل_Triggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            ثابت خودکار getIdx = [flp](AstVarScope* ثابت scocep, VAccess access, سائز_t idx) {
                واپس نیا AstArraySel{flp, نیا AstVarRef{flp, scocep, access},
                                       نیا AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* ثابت argpVscp = نیا AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->جوڑو_Varsp(argpVscp);

            // Mark as ready triggered schedulers
            برائے (ثابت خودکار& triggers_مقصد_Trees : مستعمل_Trigs_مقصد_Using_Trees) {
                ثابت سائز_t لفظ = triggers_مقصد_Trees.پہلا;

                برائے (ثابت خودکار& بٹ_مقصد_Trees : triggers_مقصد_Trees.دوسرا) {
                    ثابت سائز_t ثنائی_ہندسہ_2 = بٹ_مقصد_Trees.پہلا;
                    ثابت خودکار& schedulers = بٹ_مقصد_Trees.دوسرا;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* ثابت نقاب_Constp = نیا AstConst{flp, AstConst::Unsized64{}, ثنائی_ہندسہ_2};
                    AstAnd* ثابت condp
                        = نیا AstAnd{flp, getIdx(vscp, VAccess::READ, لفظ), نقاب_Constp};
                    AstIf* ثابت ifp = نیا AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    برائے (Ast_گرہ_اظہاریہ* ثابت schedp : schedulers) {
                        AstCMethodHard* ثابت callp = نیا AstCMethodHard{
                            flp, schedp->clone_درخت(جھوٹ), VCMethod::SCHED_READY};
                        callp->dtype_سیٹ_Void();
                        callp->جوڑو_Pinsp(نیا AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->جوڑو_Thensp(callp->makeStmt());
                    }
                    funcp->جوڑو_Stmtsp(ifp);
                }
            }

            AstVarScope* ثابت vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            برائے (ثابت خودکار& triggers_مقصد_Trees : مستعمل_Trigs_مقصد_Using_Trees) {
                ثابت سائز_t لفظ = triggers_مقصد_Trees.پہلا;
                funcp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, getIdx(vscAccp, VAccess::WRITE, لفظ),
                                               نیا AstOr{flp, getIdx(vscAccp, VAccess::READ, لفظ),
                                                         getIdx(vscp, VAccess::READ, لفظ)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = طےشدہ;
};

خالی beforeTrigVisitor(AstNetlist* netlistp, Sen_اظہاریہ_Builder& sen_اظہاریہ_Builder,
                       ثابت TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_اظہاریہ_Builder, trigKit};
}

}  // namespace V3Sched
