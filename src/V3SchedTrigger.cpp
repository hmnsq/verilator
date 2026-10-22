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

استفاده util::new_آرگومان;

AstVarScope* newLocal(AstCFunc* funcp, Ast_گره_DType* dtypep, ثابت std::string& name) {
    File_سطر* ثابت flp = funcp->fileline();
    AstScope* ثابت scopep = funcp->scopep();
    AstVar* ثابت varp = جديد AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(درست);
    varp->noReset(درست);
    funcp->جمع_کن_Varsp(varp);
    AstVarScope* ثابت vscp = جديد AstVarScope{flp, scopep, varp};
    scopep->جمع_کن_Varsp(vscp);
    أرجع vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() ثابت {
    UASSERT(m_n_Pre_واژگان, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->بالاترین_Scopep()->fileline();
    Ast_گره_DType* ثابت u32DTypep = netlistp->پیدا_کن_UInt32_DType();
    Ast_گره_DType* ثابت strDtypep = netlistp->پیدا_کن_String_DType();

    // Dumping function always slow
    ثابت std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, درست);
    funcp->isStatic(درست);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* ثابت eVscp = new_آرگومان(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* ثابت tVscp = new_آرگومان(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    ثابت خودکار rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودکار wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* ثابت vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* ثابت iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->جمع_کن_Stmtsp(util::مجموعه_Var(iVscp, 0));
        // Add loop
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        funcp->جمع_کن_Stmtsp(loopp);
        // Loop body
        Ast_گره_عبارت* ثابت lhsp = جديد AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_گره_عبارت* ثابت rhsp = جديد AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_گره_عبارت* ثابت limp = جديد AstConst{flp, AstConst::Widthed_مقدار{}, 32, m_n_Vec_واژگان};
        loopp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{flp, lhsp, rhsp});
        loopp->جمع_کن_Stmtsp(util::incrementVar(iVscp));
        loopp->جمع_کن_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* ثابت callp = جديد AstCCall{flp, m_dumpp};
        callp->dtype_مجموعه_Void();
        callp->جمع_کن_Argsp(rd(vVscp));
        callp->جمع_کن_Argsp(rd(tVscp));
        funcp->جمع_کن_Stmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* ثابت pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* ثابت jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->جمع_کن_Stmtsp(util::مجموعه_Var(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            funcp->جمع_کن_Stmtsp(loopp);
            // Loop body
            Ast_گره_عبارت* ثابت lhsp = جديد AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_گره_عبارت* ثابت rhsp = جديد AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_گره_عبارت* ثابت limp = جديد AstConst{flp, AstConst::Widthed_مقدار{}, 32, m_n_Pre_واژگان};
            loopp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{flp, lhsp, rhsp});
            loopp->جمع_کن_Stmtsp(util::incrementVar(jVscp));
            loopp->جمع_کن_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            funcp->جمع_کن_Stmtsp(loopp);
            // Loop body
            Ast_گره_عبارت* ثابت lhsp = جديد AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_گره_عبارت* ثابت rhsp = جديد AstConst{flp, AstConst::DTyped{}, m_کلمه_DTypep};
            Ast_گره_عبارت* ثابت limp = جديد AstConst{flp, AstConst::Widthed_مقدار{}, 32, m_n_Vec_واژگان};
            loopp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{flp, lhsp, rhsp});
            loopp->جمع_کن_Stmtsp(util::incrementVar(jVscp));
            loopp->جمع_کن_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* ثابت callp = جديد AstCCall{flp, m_dumpp};
        callp->dtype_مجموعه_Void();
        callp->جمع_کن_Argsp(rd(pVscp));
        callp->جمع_کن_Argsp(
            جديد AstConcatN{flp, rd(tVscp), جديد AstConst{flp, AstConst::String{}, " pre"}});
        funcp->جمع_کن_Stmtsp(callp->makeStmt());
    }

    أرجع funcp;
}

AstCFunc* TriggerKit::create_Any_مجموعه_Func(AstUnpackArrayDType* ثابت dtypep) ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->بالاترین_Scopep()->fileline();
    Ast_گره_DType* ثابت u32DTypep = netlistp->پیدا_کن_UInt32_DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(درست);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* ثابت iVscp = new_آرگومان(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    ثابت خودکار rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* ثابت loopp = جديد AstLoop{flp};
    funcp->جمع_کن_Stmtsp(util::مجموعه_Var(nVscp, 0));
    funcp->جمع_کن_Stmtsp(loopp);
    funcp->جمع_کن_Stmtsp(جديد AstCReturn{flp, جديد AstConst{flp, AstConst::بیت_False{}}});

    // Loop body
    ثابت uint32_t n_واژگان = dtypep->elementsConst();
    Ast_گره_عبارت* ثابت condp = جديد AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_گره_Stmt* ثابت thenp = جديد AstCReturn{flp, جديد AstConst{flp, AstConst::بیت_True{}}};
    Ast_گره_عبارت* ثابت limp = جديد AstConst{flp, AstConst::Widthed_مقدار{}, 32, n_واژگان};
    loopp->جمع_کن_Stmtsp(جديد AstIf{flp, condp, thenp});
    loopp->جمع_کن_Stmtsp(util::incrementVar(nVscp));
    loopp->جمع_کن_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});

    //انجامشده
    أرجع funcp;
}
AstCFunc* TriggerKit::createClearFunc() ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->بالاترین_Scopep()->fileline();
    Ast_گره_DType* ثابت u32DTypep = netlistp->پیدا_کن_UInt32_DType();

    // Create function
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(درست);

    // Add arguments
    AstVarScope* ثابت oVscp = new_آرگومان(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ثابت خودکار rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودکار wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ثابت loopp = جديد AstLoop{flp};
    funcp->جمع_کن_Stmtsp(util::مجموعه_Var(nVscp, 0));
    funcp->جمع_کن_Stmtsp(loopp);

    // Loop body
    Ast_گره_عبارت* ثابت lhsp = جديد AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_گره_عبارت* ثابت rhsp = جديد AstConst{flp, AstConst::DTyped{}, m_کلمه_DTypep};
    Ast_گره_عبارت* ثابت limp = جديد AstConst{flp, AstConst::Widthed_مقدار{}, 32, m_n_Vec_واژگان};
    loopp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{flp, lhsp, rhsp});
    loopp->جمع_کن_Stmtsp(util::incrementVar(nVscp));
    loopp->جمع_کن_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});

    //انجامشده
    أرجع funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ثابت oDtypep,
                                       AstUnpackArrayDType* ثابت iDtypep) ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->بالاترین_Scopep()->fileline();
    Ast_گره_DType* ثابت u32DTypep = netlistp->پیدا_کن_UInt32_DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(درست);

    // Add arguments
    AstVarScope* ثابت oVscp = new_آرگومان(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* ثابت iVscp = new_آرگومان(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ثابت خودکار rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودکار wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ثابت loopp = جديد AstLoop{flp};
    funcp->جمع_کن_Stmtsp(util::مجموعه_Var(nVscp, 0));
    funcp->جمع_کن_Stmtsp(loopp);

    // Loop body
    Ast_گره_عبارت* ثابت lhsp = جديد AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_گره_عبارت* ثابت oWordp = جديد AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_گره_عبارت* ثابت iWordp = جديد AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_گره_عبارت* ثابت rhsp = جديد AstOr{flp, oWordp, iWordp};
    AstConst* ثابت outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ثابت درون_داد_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_گره_عبارت* ثابت limp = outputRangeLeftp->رقم().مقصد_SInt() < درون_داد_Range_Leftp->رقم().مقصد_SInt()
                                  ? outputRangeLeftp->clone_درخت_خالص(خطأ)
                                  : درون_داد_Range_Leftp->clone_درخت_خالص(خطأ);
    loopp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{flp, lhsp, rhsp});
    loopp->جمع_کن_Stmtsp(util::incrementVar(nVscp));
    loopp->جمع_کن_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLte{flp, rd(nVscp), limp}});

    //انجامشده
    أرجع funcp;
}

Ast_گره_عبارت* TriggerKit::new_Any_مجموعه_Call(AstVarScope* ثابت vscp) ثابت {
    File_سطر* ثابت flp = v3Global.rootp()->بالاترین_Scopep()->fileline();
    إذا (!m_n_Vec_واژگان) أرجع جديد AstConst{flp, AstConst::بیت_False{}};

    AstCFunc* funcp = اشاره_تهی;
    إذا (vscp->dtypep() == m_trigVecDTypep) {
        إذا (!m_any_مجموعه_Vecp) m_any_مجموعه_Vecp = create_Any_مجموعه_Func(m_trigVecDTypep);
        funcp = m_any_مجموعه_Vecp;
    } وإلا إذا (vscp->dtypep() == m_trigExtDTypep) {
        إذا (!m_any_مجموعه_Extp) m_any_مجموعه_Extp = create_Any_مجموعه_Func(m_trigExtDTypep);
        funcp = m_any_مجموعه_Extp;
    } وإلا {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->جمع_کن_Argsp(جديد AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_مجموعه_بیت();
    أرجع callp;
}
Ast_گره_Stmt* TriggerKit::newClearCall(AstVarScope* ثابت vscp) ثابت {
    إذا (!m_n_Vec_واژگان) أرجع اشاره_تهی;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    إذا (!m_clearp) m_clearp = createClearFunc();
    File_سطر* ثابت flp = v3Global.rootp()->بالاترین_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_clearp};
    callp->جمع_کن_Argsp(جديد AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_مجموعه_Void();
    أرجع callp->makeStmt();
}
Ast_گره_Stmt* TriggerKit::newOrIntoCall(AstVarScope* ثابت oVscp, AstVarScope* ثابت iVscp) ثابت {
    إذا (!m_n_Vec_واژگان) أرجع اشاره_تهی;
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
    File_سطر* ثابت flp = v3Global.rootp()->بالاترین_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->جمع_کن_Argsp(جديد AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->جمع_کن_Argsp(جديد AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtype_مجموعه_Void();
    أرجع callp->makeStmt();
}

Ast_گره_Stmt* TriggerKit::new_Comp_پایه_Call() ثابت {
    إذا (!m_n_Vec_واژگان) أرجع اشاره_تهی;
    File_سطر* ثابت flp = v3Global.rootp()->بالاترین_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_compVecp};
    callp->dtype_مجموعه_Void();
    أرجع callp->makeStmt();
}

Ast_گره_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ثابت {
    إذا (!m_n_Pre_واژگان) أرجع اشاره_تهی;
    File_سطر* ثابت flp = v3Global.rootp()->بالاترین_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_compExtp};
    callp->جمع_کن_Argsp(جديد AstVarRef{flp, vscp, VAccess::READ});
    callp->dtype_مجموعه_Void();
    أرجع callp->makeStmt();
}

Ast_گره_Stmt* TriggerKit::newDumpCall(AstVarScope* ثابت vscp, ثابت std::string& برچسب,
                                     منطقي debugOnly) ثابت {
    إذا (!m_n_Vec_واژگان) أرجع اشاره_تهی;
    AstCFunc* funcp = اشاره_تهی;
    إذا (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } وإلا إذا (vscp->dtypep() == m_trigExtDTypep) {
        إذا (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } وإلا {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_سطر* ثابت flp = v3Global.rootp()->بالاترین_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->جمع_کن_Argsp(جديد AstVarRef{flp, vscp, VAccess::READ});
    callp->جمع_کن_Argsp(جديد AstConst{flp, AstConst::String{}, برچسب});
    callp->dtype_مجموعه_Void();
    AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
    cstmtp->جمع_کن("#ifdef VL_DEBUG\n");
    إذا (debugOnly) {
        cstmtp->جمع_کن("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->جمع_کن(callp->makeStmt());
        cstmtp->جمع_کن("}\n");
    } وإلا {
        cstmtp->جمع_کن(callp->makeStmt());
    }
    cstmtp->جمع_کن("#endif");
    أرجع cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(ثابت std::string& name) ثابت {
    إذا (!m_n_Vec_واژگان) أرجع اشاره_تهی;
    أرجع v3Global.rootp()->بالاترین_Scopep()->create_موقت("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extra_Trigger_شاخص(uint32_t شاخص) ثابت {
    UASSERT(شاخص <= m_n_Extra_واژگان * کلمه_اندازه, "Invalid external trigger index");
    أرجع شاخص + m_n_Sense_واژگان * کلمه_اندازه;
}

std::مجموعه<uint32_t> TriggerKit::triggerIndices(ثابت Ast_Sen_درخت* senTreep, منطقي pre) ثابت {
    std::مجموعه<uint32_t> indices;
    لكل (ثابت Ast_Sen_قلم* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), Sen_قلم)) {
        uint32_t شاخص = m_senItem2TrigIdx.at(*sip);
        إذا (pre) {
            UASSERT_OBJ(شاخص < m_n_Pre_واژگان * کلمه_اندازه, sip, "Not a 'pre' trigger");
            شاخص += m_n_Vec_واژگان * کلمه_اندازه;
        }
        indices.درج(شاخص);
    }
    أرجع indices;
}

Ast_گره_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t شاخص) ثابت {
    شاخص = extra_Trigger_شاخص(شاخص);
    ثابت uint32_t کلمه_شاخص = شاخص / کلمه_اندازه;
    ثابت uint32_t بیت_شاخص = شاخص % کلمه_اندازه;
    File_سطر* ثابت flp = vscp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_گره_عبارت* ثابت wordp = جديد AstArraySel{flp, refp, static_cast<صحيح>(کلمه_شاخص)};
    Ast_گره_عبارت* ثابت trigLhsp = جديد AstSel{flp, wordp, static_cast<صحيح>(بیت_شاخص), 1};
    Ast_گره_عبارت* ثابت trigRhsp = جديد AstVarRef{flp, vscp, VAccess::READ};
    أرجع جديد Ast_تخصیص_بده{flp, trigLhsp, trigRhsp};
}

فراغ TriggerKit::جمع_کن_Extra_Trigger_Assignment(AstVarScope* vscp, uint32_t شاخص) ثابت {
    File_سطر* ثابت flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_گره* ثابت setp = newExtraTriggerAssignment(vscp, شاخص);
    setp->جمع_کن_بعدی(جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, vscp, VAccess::WRITE},
                                جديد AstConst{flp, AstConst::بیت_False{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
فراغ TriggerKit::جمع_کن_مقدار_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t شاخص) ثابت {
    شاخص += m_n_Sense_واژگان * کلمه_اندازه;
    ثابت uint32_t کلمه_شاخص = شاخص / کلمه_اندازه;
    ثابت uint32_t بیت_شاخص = شاخص % کلمه_اندازه;

    Ast_بالاترین_Scope* ثابت بالاترین_Scopep = netlistp->بالاترین_Scopep();
    File_سطر* ثابت flp = netlistp->fileline();
    Ast_گره_DType* ثابت dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    إذا (ثابت AstBasicDType* ثابت bdtypep = VN_CAST(dtypep, BasicDType)) {
        إذا (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            أرجع;
        }
    } وإلا إذا (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, گره_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
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
    AstVarScope* ثابت prevVscp = بالاترین_Scopep->create_موقت(prevName, instVscp->dtypep());

    // Initialize prev = inst
    إذا (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = جديد AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_تخصیص_بده, rdInst(instVscp)};
        cmhp->dtype_مجموعه_Void();
        initFuncp->جمع_کن_Stmtsp(cmhp->makeStmt());
    } وإلا {
        initFuncp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_گره_عبارت* neqp;
    إذا (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp
            = جديد AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtype_مجموعه_بیت();
        neqp = cmhp;
    } وإلا {
        neqp = جديد AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_گره* updp;
    إذا (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = جديد AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_تخصیص_بده, rdInst(instVscp)};
        cmhp->dtype_مجموعه_Void();
        updp = cmhp->makeStmt();
    } وإلا {
        updp = جديد Ast_تخصیص_بده{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ثابت refp = جديد AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_گره_عبارت* ثابت wordp = جديد AstArraySel{flp, refp, static_cast<صحيح>(کلمه_شاخص)};
    Ast_گره_عبارت* ثابت trigLhsp = جديد AstSel{flp, wordp, static_cast<صحيح>(بیت_شاخص), 1};
    Ast_گره* ثابت setp = جديد Ast_تخصیص_بده{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->جمع_کن_بعدی(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ثابت std::string& name, منطقي slow, uint32_t n_Sense_واژگان,
                       uint32_t n_Extra_واژگان, uint32_t n_Pre_واژگان,
                       std::unordered_map<VNRef<ثابت Ast_Sen_قلم>, اندازه_t> senItem2TrigIdx,
                       ثابت std::وکتور<ثابت Ast_Sen_قلم*>& senItemps, منطقي useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_واژگان{n_Sense_واژگان}
    , m_n_Extra_واژگان{n_Extra_واژگان}
    , m_n_Pre_واژگان{n_Pre_واژگان}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trig_Idx2_Sen_قلم{senItemps} {
    // If no triggers, we don't need to generate anything
    إذا (!m_n_Vec_واژگان) أرجع;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trig_Idx2_Sen_قلم.resize((m_n_Vec_واژگان + m_n_Pre_واژگان) * کلمه_اندازه);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_واژگان * کلمه_اندازه,
              m_trig_Idx2_Sen_قلم.begin() + m_n_Vec_واژگان * کلمه_اندازه);
    AstNetlist* ثابت netlistp = v3Global.rootp();
    AstScope* ثابت scopep = netlistp->بالاترین_Scopep()->scopep();
    File_سطر* ثابت flp = scopep->fileline();
    // Data type of a single trigger word
    m_کلمه_DTypep = netlistp->پیدا_کن_بیت_DType(کلمه_اندازه, کلمه_اندازه, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ثابت rp = جديد AstRange{flp, static_cast<صحيح>(m_n_Vec_واژگان - 1), 0};
    m_trigVecDTypep = جديد AstUnpackArrayDType{flp, m_کلمه_DTypep, rp};
    netlistp->typeTablep()->جمع_کن_Typesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    إذا (m_n_Pre_واژگان) {
        AstRange* ثابت ep = جديد AstRange{flp, static_cast<صحيح>(m_n_Vec_واژگان + m_n_Pre_واژگان - 1), 0};
        m_trigExtDTypep = جديد AstUnpackArrayDType{flp, m_کلمه_DTypep, ep};
        netlistp->typeTablep()->جمع_کن_Typesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } وإلا {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->بالاترین_Scopep()->create_موقت("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->is_داخلی(درست);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, درست);
    m_dumpp->isStatic(درست);
    m_dumpp->ifdef("VL_DEBUG");
    إذا (useAcc) {
        m_vscAccp
            = netlistp->بالاترین_Scopep()->create_موقت("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->is_داخلی(درست);
    }
}

Ast_تخصیص_بده* TriggerKit::createSenTrigVecAssignment(AstVarScope* ثابت هدف,
                                                  std::وکتور<Ast_گره_عبارت*>& trigps) {
    File_سطر* ثابت flp = هدف->fileline();
    Ast_تخصیص_بده* trigStmtsp = اشاره_تهی;
    // Assign sense triggers vector one word at a time
    لكل (اندازه_t i = 0; i < trigps.اندازه(); i += کلمه_اندازه) {
        // Concatenate all bits in this trigger word using a balanced
        لكل (uint32_t level = 0; level < کلمه_اندازه_LOG2; ++level) {
            ثابت uint32_t stride = 1 << level;
            لكل (uint32_t j = 0; j < کلمه_اندازه; j += 2 * stride) {
                trigps[i + j] = جديد AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = اشاره_تهی;
            }
        }

        // Set the whole word in the trigger vector
        ثابت صحيح کلمه_شاخص = static_cast<صحيح>(i / کلمه_اندازه);
        AstArraySel* ثابت aselp
            = جديد AstArraySel{flp, جديد AstVarRef{flp, هدف, VAccess::WRITE}, کلمه_شاخص};
        trigStmtsp = Ast_گره::جمع_کن_بعدی(trigStmtsp, جديد Ast_تخصیص_بده{flp, aselp, trigps[i]});
    }
    أرجع trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ثابت initFuncp,  //
                              Sen_عبارت_Builder& sen_عبارت_Builder,  //
                              ثابت std::وکتور<ثابت Ast_Sen_درخت*>& preTreeps,  //
                              ثابت std::وکتور<ثابت Ast_Sen_درخت*>& senTreeps,  //
                              ثابت string& name,  //
                              ثابت ExtraTriggers& extraTriggers,  //
                              منطقي slow,  //
                              منطقي useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::وکتور<ثابت Ast_Sen_قلم*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<ثابت Ast_Sen_قلم>, اندازه_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    لكل (ثابت Ast_Sen_درخت* ثابت senTreep : preTreeps) {
        لكل (ثابت Ast_Sen_قلم *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_قلم);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت خودکار pair = senItem2TrigIdx.emplace(*itemp, senItemps.اندازه());
            إذا (pair.دومی) senItemps.افزودن_پشت(itemp);
        }
    }
    ثابت uint32_t n_Pre_Sen_اقلام = senItemps.اندازه();
    V3Stats::جمع_کن_Stat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_اقلام);
    // Number of pre triggers, rounded up to a full word.
    ثابت uint32_t nPreTriggers = vlstd::دور_به_بالا_مقصد_مضرب_از<کلمه_اندازه>(senItemps.اندازه());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ثابت uint32_t n_Pre_واژگان = nPreTriggers / کلمه_اندازه;

    // Process the rest of the trees
    لكل (ثابت Ast_Sen_درخت* ثابت senTreep : senTreeps) {
        لكل (ثابت Ast_Sen_قلم *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_قلم);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت خودکار pair = senItem2TrigIdx.emplace(*itemp, senItemps.اندازه());
            إذا (pair.دومی) senItemps.افزودن_پشت(itemp);
        }
    }
    ثابت uint32_t n_Sen_اقلام = senItemps.اندازه() - nPreTriggers;
    V3Stats::جمع_کن_Stat("Scheduling, '" + name + "' sense triggers", n_Sen_اقلام + n_Pre_Sen_اقلام);
    // Number of sense triggers, rounded up to a full word
    ثابت uint32_t nSenseTriggers = vlstd::دور_به_بالا_مقصد_مضرب_از<کلمه_اندازه>(senItemps.اندازه());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ثابت uint32_t n_Sense_واژگان = nSenseTriggers / کلمه_اندازه;

    // Allocate space for the extra triggers
    V3Stats::جمع_کن_Stat("Scheduling, '" + name + "' extra triggers", extraTriggers.اندازه());
    // Number of extra triggers, rounded up to a full word.
    ثابت uint32_t nExtraTriggers = vlstd::دور_به_بالا_مقصد_مضرب_از<کلمه_اندازه>(extraTriggers.اندازه());
    ثابت uint32_t n_Extra_واژگان = nExtraTriggers / کلمه_اندازه;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_واژگان, n_Extra_واژگان, n_Pre_واژگان, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    إذا (!kit.m_n_Vec_واژگان) أرجع kit;

    File_سطر* ثابت flp = netlistp->بالاترین_Scopep()->fileline();

    // Creates read/write reference
    ثابت خودکار rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت خودکار wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* ثابت dumpTrgp
        = new_آرگومان(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* ثابت dumpTagp
        = new_آرگومان(kit.m_dumpp, netlistp->پیدا_کن_String_DType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* ثابت ifp = جديد AstIf{flp, جديد AstLogNot{flp, kit.new_Any_مجموعه_Call(dumpTrgp)}};
        kit.m_dumpp->جمع_کن_Stmtsp(ifp);
        AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
        ifp->جمع_کن_Thensp(cstmtp);
        cstmtp->جمع_کن("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->جمع_کن(rd(dumpTagp));
        cstmtp->جمع_کن(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    ثابت خودکار جمع_کن_Debug = [&](uint32_t شاخص, ثابت string& متن) {
        ثابت صحيح wrd_شاخص = static_cast<صحيح>(شاخص / کلمه_اندازه);
        ثابت صحيح بیت_شاخص = static_cast<صحيح>(شاخص % کلمه_اندازه);
        Ast_گره_عبارت* ثابت aselp = جديد AstArraySel{flp, rd(dumpTrgp), wrd_شاخص};
        Ast_گره_عبارت* ثابت condp = جديد AstSel{flp, aselp, بیت_شاخص, 1};
        AstIf* ثابت ifp = جديد AstIf{flp, condp};
        kit.m_dumpp->جمع_کن_Stmtsp(ifp);
        AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
        ifp->جمع_کن_Thensp(cstmtp);
        cstmtp->جمع_کن("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->جمع_کن(rd(dumpTagp));
        cstmtp->جمع_کن(" + \"' region trigger index " + std::مقصد_string(شاخص) + " is active: " + متن
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::وکتور<Ast_گره_عبارت*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_گره_Stmt* initialTrigsp = اشاره_تهی;
    لكل (اندازه_t i = 0; i < senItemps.اندازه(); ++i) {
        ثابت Ast_Sen_قلم* ثابت senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        إذا (!senItemp) {
            trigps.emplace_پشت(جديد AstConst{flp, AstConst::بیت_False{}});
            استمر;
        }

        // Create the trigger computation expression
        ثابت خودکار& pair = sen_عبارت_Builder.build(senItemp);
        trigps.emplace_پشت(pair.اولی);

        // Add initialization time trigger
        إذا (pair.دومی || v3Global.opt.x_Initial_یال()) {
            ثابت صحيح wrd_شاخص = static_cast<صحيح>(i / کلمه_اندازه);
            ثابت صحيح بیت_شاخص = static_cast<صحيح>(i % کلمه_اندازه);
            Ast_گره_عبارت* ثابت wordp = جديد AstArraySel{flp, wr(kit.m_vscp), wrd_شاخص};
            Ast_گره_عبارت* ثابت lhsp = جديد AstSel{flp, wordp, بیت_شاخص, 1};
            Ast_گره_عبارت* ثابت rhsp = جديد AstConst{flp, AstConst::بیت_True{}};
            إذا (useAcc) {
                initFuncp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{flp, lhsp, rhsp});
            } وإلا {
                initialTrigsp = Ast_گره::جمع_کن_بعدی(initialTrigsp, جديد Ast_تخصیص_بده{flp, lhsp, rhsp});
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
        جمع_کن_Debug(i, desc);
    }
    UASSERT(trigps.اندازه() == nSenseTriggers, "Inconsistent number of trigger expressions");

    Ast_تخصیص_بده* ثابت trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    لكل (بیعلامت i = 0; i < extraTriggers.اندازه(); ++i) {
        جمع_کن_Debug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    ثابت Sen_عبارت_Builder::Results senResults = sen_عبارت_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    لكل (Ast_گره_Stmt* ثابت nodep : senResults.m_inits) initFuncp->جمع_کن_Stmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_بالاترین_Scope* ثابت بالاترین_Scopep = netlistp->بالاترین_Scopep();
    {
        AstCFunc* ثابت fp = kit.m_compVecp;
        // Trigger computation
        لكل (Ast_گره_Stmt* ثابت nodep : senResults.m_preUpdates) fp->جمع_کن_Stmtsp(nodep);
        fp->جمع_کن_Stmtsp(trigStmtsp);
        لكل (Ast_گره_Stmt* ثابت nodep : senResults.m_postUpdates) fp->جمع_کن_Stmtsp(nodep);
        // Add the initialization time triggers
        إذا (initialTrigsp) {
            AstVarScope* ثابت initVscp = بالاترین_Scopep->create_موقت("__V" + name + "DidInit", 1);
            AstIf* ثابت ifp = جديد AstIf{flp, جديد AstNot{flp, rd(initVscp)}};
            fp->جمع_کن_Stmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->جمع_کن_Thensp(util::مجموعه_Var(initVscp, 1));
            ifp->جمع_کن_Thensp(initialTrigsp);
        }
        util::تقسیم_Check(fp);
    };
    // If there are 'pre' triggers, compute them
    إذا (kit.m_n_Pre_واژگان) {
        AstCFunc* ثابت fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ثابت latchedp
            = new_آرگومان(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ثابت nVscp = بالاترین_Scopep->create_موقت("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(درست);
        // Add a loop to compute the pre words
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        fp->جمع_کن_Stmtsp(util::مجموعه_Var(nVscp, 0));
        fp->جمع_کن_Stmtsp(loopp);
        // Loop body
        Ast_گره_عبارت* ثابت offsetp = جديد AstConst{flp, kit.m_n_Vec_واژگان};
        Ast_گره_عبارت* ثابت lIdxp = جديد Ast_جمع_کن{flp, rd(nVscp), offsetp};
        Ast_گره_عبارت* ثابت lhsp = جديد AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_گره_عبارت* ثابت aWordp = جديد AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_گره_عبارت* ثابت bWordp = جديد AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_گره_عبارت* ثابت rhsp = جديد AstAnd{flp, aWordp, جديد AstNot{flp, bWordp}};
        Ast_گره_عبارت* ثابت limp = جديد AstConst{flp, AstConst::Widthed_مقدار{}, 32, n_Pre_واژگان};
        loopp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{flp, lhsp, rhsp});
        loopp->جمع_کن_Stmtsp(util::incrementVar(nVscp));
        loopp->جمع_کن_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});
        util::تقسیم_Check(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    إذا (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlink_Fr_پشت_With_بعدی()->delete_درخت();
    // Done with the trigger dump function, split as might be large
    util::تقسیم_Check(kit.m_dumpp);

    أرجع kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
صنف AwaitBeforeTrigVisitor final : عام VNVisitor {
    ثابت VNUser1_درون_Use m_user1_درون_Use;

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
    Sen_عبارت_Builder& m_sen_عبارت_Builder;
    // Generator of unique names for before-trigger function
    V3_Unique_نامها m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::وکتور<std::pair<AstCFunc*, Ast_Sen_درخت*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::وکتور<std::pair<Ast_Sen_درخت*, Ast_گره_عبارت*>> m_sen_درخت_مقصد_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_گره>, std::وکتور<Ast_Sen_قلم*>> m_sen_عبارت_مقصد_Sen_قلم;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    ساكن Ast_گره* get_Sen_Hash_گره(ثابت Ast_Sen_قلم* ثابت nodep) {
        إذا (AstVarRef* ثابت varRefp = VN_CAST(nodep->sensp(), VarRef)) أرجع varRefp;
        أرجع nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    فراغ fill_Sen_عبارت_مقصد_Sen_قلم() {
        لكل (خودکار sen_درخت_Sched : m_sen_درخت_مقصد_Sched) {
            Ast_Sen_درخت* ثابت senTreep = sen_درخت_Sched.اولی;

            لكل (Ast_Sen_قلم* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_قلم)) {
                ثابت VEdgeType یال = senItemp->یال_Type();
                إذا (یال.an_یال() || یال == VEdgeType::ET_تغییریافته
                    || یال == VEdgeType::ET_HYBRID) {
                    m_sen_عبارت_مقصد_Sen_قلم[*get_Sen_Hash_گره(senItemp)].افزودن_پشت(senItemp);
                }
            }
        }

        std::وکتور<VNRef<Ast_گره>> مقصد_Remove;
        لكل (ثابت خودکار& sen_عبارت_مقصد_Sen_درخت : m_sen_عبارت_مقصد_Sen_قلم) {
            std::وکتور<Ast_Sen_قلم*> senItemps = sen_عبارت_مقصد_Sen_درخت.دومی;
            مقصد_Remove.افزودن_پشت(sen_عبارت_مقصد_Sen_درخت.اولی);
            لكل (اندازه_t i = 1; i < senItemps.اندازه(); ++i) {
                إذا (senItemps[i]->یال_Type() != senItemps[i - 1]->یال_Type()) {
                    مقصد_Remove.برداشت_پشت();
                    اكسر;
                }
            }
        }
        لكل (ثابت VNRef<Ast_گره> it : مقصد_Remove) m_sen_عبارت_مقصد_Sen_قلم.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<اندازه_t, std::map<اندازه_t, std::وکتور<Ast_گره_عبارت*>>>
    get_استفاده_شده_Triggers_مقصد_Trees(ثابت std::مجموعه<اندازه_t>& استفاده_شده_Triggers) {
        std::map<اندازه_t, std::map<اندازه_t, std::وکتور<Ast_گره_عبارت*>>> استفاده_شده_Trigs_مقصد_Using_Trees;
        لكل (خودکار sen_درخت_Sched : m_sen_درخت_مقصد_Sched) {
            ثابت Ast_Sen_درخت* ثابت senTreep = sen_درخت_Sched.اولی;
            Ast_گره_عبارت* ثابت shedp = sen_درخت_Sched.دومی;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            لكل (Ast_Sen_قلم* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_قلم)) {
                ثابت اندازه_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                إذا (استفاده_شده_Triggers.پیدا_کن(idx) != استفاده_شده_Triggers.انتها()) {
                    استفاده_شده_Trigs_مقصد_Using_Trees[idx / TriggerKit::کلمه_اندازه]
                                         [اندازه_t{1} << (idx % TriggerKit::کلمه_اندازه)]
                                             .افزودن_پشت(shedp);
                }
            }
        }
        إذا (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            لكل (ثابت خودکار& triggers_مقصد_Trees : استفاده_شده_Trigs_مقصد_Using_Trees) {
                لكل (ثابت خودکار& بیتها_مقصد_Trees : triggers_مقصد_Trees.دومی) {
                    ثابت std::مجموعه<ثابت Ast_گره_عبارت*> exprps{بیتها_مقصد_Trees.دومی.begin(),
                                                              بیتها_مقصد_Trees.دومی.انتها()};
                    UASSERT(بیتها_مقصد_Trees.دومی.اندازه() == exprps.اندازه(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        أرجع استفاده_شده_Trigs_مقصد_Using_Trees;
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
            funcp->noLife(درست);

            AstVar* ثابت argp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->پیدا_کن_Basic_DType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(درست);
            argp->direction(VDirection::درون_داد);
            funcp->جمع_کن_Argsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_پشت(funcp, senTreep);
        }
        AstCCall* ثابت callp = جديد AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtype_مجموعه_Void();
        أرجع callp;
    }

    فراغ بازدید(AstCAwait* ثابت nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        إذا (ثابت AstCMethodHard* ثابت c_روش_Hardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            إذا (c_روش_Hardp->روش() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ثابت beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_گره* ثابت pinp = c_روش_Hardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, c_روش_Hardp, "No event description");
                beforeTrigp->جمع_کن_Argsp(VN_AS(pinp, گره_عبارت)->clone_درخت(خطأ));

                // Call the before-trigger function before the CAwait
                nodep->جمع_کن_اینجا_This_As_بعدی(beforeTrigp->makeStmt());
                m_sen_درخت_مقصد_Sched.emplace_پشت(nodep->sentreep(), c_روش_Hardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    فراغ بازدید(Ast_گره* ثابت nodep) override { iterateChildren(nodep); }

عام:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_عبارت_Builder& sen_عبارت_Builder,
                           ثابت TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_عبارت_Builder{sen_عبارت_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_عبارت_مقصد_Sen_قلم();

        std::وکتور<Ast_گره_عبارت*> trigps;
        std::مجموعه<اندازه_t> استفاده_شده_Triggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        لكل (ثابت خودکار& func_مقصد_استفاده_شده_Triggers : m_generatedFuncs) {
            AstCFunc* ثابت funcp = func_مقصد_استفاده_شده_Triggers.اولی;
            AstVarScope* ثابت vscp = VN_AS(funcp->user1p(), VarScope);
            File_سطر* ثابت flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_درخت* ثابت senTreep = func_مقصد_استفاده_شده_Triggers.دومی;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ثابت خودکار emplaceAt
                    = [flp, &trigps, &استفاده_شده_Triggers](Ast_گره_عبارت* ثابت exprp, ثابت اندازه_t pos) {
                          ثابت اندازه_t هدف_اندازه
                              = vlstd::دور_به_بالا_مقصد_مضرب_از<TriggerKit::کلمه_اندازه>(pos + 1);
                          إذا (trigps.ظرفیت() < هدف_اندازه) trigps.reserve(هدف_اندازه * 2);
                          طالما (trigps.اندازه() < هدف_اندازه) {
                              trigps.افزودن_پشت(جديد AstConst{flp, AstConst::بیت_False{}});
                          }
                          trigps[pos]->delete_درخت();
                          trigps[pos] = exprp;
                          استفاده_شده_Triggers.درج(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                لكل (ثابت Ast_Sen_قلم* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Sen_قلم)) {
                    ثابت اندازه_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_عبارت_Builder.build(itemp).اولی, idx);
                    خودکار iter = m_sen_عبارت_مقصد_Sen_قلم.پیدا_کن(*get_Sen_Hash_گره(itemp));
                    إذا (iter != m_sen_عبارت_مقصد_Sen_قلم.انتها()) {
                        لكل (Ast_Sen_قلم* ثابت additionalItemp : iter->دومی) {
                            ثابت اندازه_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_عبارت_Builder.build(additionalItemp).اولی, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ثابت Sen_عبارت_Builder::Results results
                    = m_sen_عبارت_Builder.getResultsAndClearUpdates();
                لكل (Ast_گره_Stmt* ثابت stmtsp : results.m_inits) funcp->جمع_کن_Stmtsp(stmtsp);
                لكل (Ast_گره_Stmt* ثابت stmtsp : results.m_preUpdates) funcp->جمع_کن_Stmtsp(stmtsp);
                funcp->جمع_کن_Stmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                لكل (Ast_گره_Stmt* ثابت stmtsp : results.m_postUpdates) funcp->جمع_کن_Stmtsp(stmtsp);
            }

            ثابت std::map<اندازه_t, std::map<اندازه_t, std::وکتور<Ast_گره_عبارت*>>>
                استفاده_شده_Trigs_مقصد_Using_Trees = get_استفاده_شده_Triggers_مقصد_Trees(استفاده_شده_Triggers);
            استفاده_شده_Triggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            ثابت خودکار getIdx = [flp](AstVarScope* ثابت scocep, VAccess access, اندازه_t idx) {
                أرجع جديد AstArraySel{flp, جديد AstVarRef{flp, scocep, access},
                                       جديد AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* ثابت argpVscp = جديد AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->جمع_کن_Varsp(argpVscp);

            // Mark as ready triggered schedulers
            لكل (ثابت خودکار& triggers_مقصد_Trees : استفاده_شده_Trigs_مقصد_Using_Trees) {
                ثابت اندازه_t کلمه = triggers_مقصد_Trees.اولی;

                لكل (ثابت خودکار& بیتها_مقصد_Trees : triggers_مقصد_Trees.دومی) {
                    ثابت اندازه_t بیت_2 = بیتها_مقصد_Trees.اولی;
                    ثابت خودکار& schedulers = بیتها_مقصد_Trees.دومی;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* ثابت ماسک_Constp = جديد AstConst{flp, AstConst::Unsized64{}, بیت_2};
                    AstAnd* ثابت condp
                        = جديد AstAnd{flp, getIdx(vscp, VAccess::READ, کلمه), ماسک_Constp};
                    AstIf* ثابت ifp = جديد AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    لكل (Ast_گره_عبارت* ثابت schedp : schedulers) {
                        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                            flp, schedp->clone_درخت(خطأ), VCMethod::SCHED_READY};
                        callp->dtype_مجموعه_Void();
                        callp->جمع_کن_Pinsp(جديد AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->جمع_کن_Thensp(callp->makeStmt());
                    }
                    funcp->جمع_کن_Stmtsp(ifp);
                }
            }

            AstVarScope* ثابت vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            لكل (ثابت خودکار& triggers_مقصد_Trees : استفاده_شده_Trigs_مقصد_Using_Trees) {
                ثابت اندازه_t کلمه = triggers_مقصد_Trees.اولی;
                funcp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{flp, getIdx(vscAccp, VAccess::WRITE, کلمه),
                                               جديد AstOr{flp, getIdx(vscAccp, VAccess::READ, کلمه),
                                                         getIdx(vscp, VAccess::READ, کلمه)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = افتراضي;
};

فراغ beforeTrigVisitor(AstNetlist* netlistp, Sen_عبارت_Builder& sen_عبارت_Builder,
                       ثابت TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_عبارت_Builder, trigKit};
}

}  // namespace V3Sched
