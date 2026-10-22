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

using util::newArgument;

AstVarScope* newLocal(AstCFunc* funcp, Ast_عقدة_DType* dtypep, ثابت std::string& name) {
    File_سطر* ثابت flp = funcp->fileline();
    AstScope* ثابت scopep = funcp->scopep();
    AstVar* ثابت varp = جديد AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(true);
    varp->noReset(true);
    funcp->addVarsp(varp);
    AstVarScope* ثابت vscp = جديد AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    أرجع vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() ثابت {
    UASSERT(m_n_Pre_كلمات, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->قمة_Scopep()->fileline();
    Ast_عقدة_DType* ثابت u32DTypep = netlistp->findUInt32DType();
    Ast_عقدة_DType* ثابت strDtypep = netlistp->findStringDType();

    // Dumping function always slow
    ثابت std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, true);
    funcp->isStatic(true);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* ثابت eVscp = newArgument(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* ثابت tVscp = newArgument(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    ثابت auto rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت auto wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* ثابت vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* ثابت iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->addStmtsp(util::setVar(iVscp, 0));
        // Add loop
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        funcp->addStmtsp(loopp);
        // Loop body
        Ast_عقدة_تعبير* ثابت lhsp = جديد AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_عقدة_تعبير* ثابت rhsp = جديد AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_عقدة_تعبير* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قيمة{}, 32, m_n_Vec_كلمات};
        loopp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(iVscp));
        loopp->addStmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* ثابت callp = جديد AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(vVscp));
        callp->addArgsp(rd(tVscp));
        funcp->addStmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* ثابت pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* ثابت jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->addStmtsp(util::setVar(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_عقدة_تعبير* ثابت lhsp = جديد AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_عقدة_تعبير* ثابت rhsp = جديد AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_عقدة_تعبير* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قيمة{}, 32, m_n_Pre_كلمات};
            loopp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_عقدة_تعبير* ثابت lhsp = جديد AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_عقدة_تعبير* ثابت rhsp = جديد AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
            Ast_عقدة_تعبير* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قيمة{}, 32, m_n_Vec_كلمات};
            loopp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* ثابت callp = جديد AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(pVscp));
        callp->addArgsp(
            جديد AstConcatN{flp, rd(tVscp), جديد AstConst{flp, AstConst::String{}, " pre"}});
        funcp->addStmtsp(callp->makeStmt());
    }

    أرجع funcp;
}

AstCFunc* TriggerKit::createAnySetFunc(AstUnpackArrayDType* ثابت dtypep) ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->قمة_Scopep()->fileline();
    Ast_عقدة_DType* ثابت u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(true);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* ثابت iVscp = newArgument(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    ثابت auto rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* ثابت loopp = جديد AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);
    funcp->addStmtsp(جديد AstCReturn{flp, جديد AstConst{flp, AstConst::BitFalse{}}});

    // Loop body
    ثابت uint32_t n_كلمات = dtypep->elementsConst();
    Ast_عقدة_تعبير* ثابت condp = جديد AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_عقدة_Stmt* ثابت thenp = جديد AstCReturn{flp, جديد AstConst{flp, AstConst::BitTrue{}}};
    Ast_عقدة_تعبير* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قيمة{}, 32, n_كلمات};
    loopp->addStmtsp(جديد AstIf{flp, condp, thenp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});

    // Done
    أرجع funcp;
}
AstCFunc* TriggerKit::createClearFunc() ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->قمة_Scopep()->fileline();
    Ast_عقدة_DType* ثابت u32DTypep = netlistp->findUInt32DType();

    // Create function
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(true);

    // Add arguments
    AstVarScope* ثابت oVscp = newArgument(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ثابت auto rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت auto wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ثابت loopp = جديد AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_عقدة_تعبير* ثابت lhsp = جديد AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_عقدة_تعبير* ثابت rhsp = جديد AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
    Ast_عقدة_تعبير* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قيمة{}, 32, m_n_Vec_كلمات};
    loopp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});

    // Done
    أرجع funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ثابت oDtypep,
                                       AstUnpackArrayDType* ثابت iDtypep) ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->قمة_Scopep()->fileline();
    Ast_عقدة_DType* ثابت u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(true);

    // Add arguments
    AstVarScope* ثابت oVscp = newArgument(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* ثابت iVscp = newArgument(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ثابت auto rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت auto wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ثابت loopp = جديد AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_عقدة_تعبير* ثابت lhsp = جديد AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_عقدة_تعبير* ثابت oWordp = جديد AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_عقدة_تعبير* ثابت iWordp = جديد AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_عقدة_تعبير* ثابت rhsp = جديد AstOr{flp, oWordp, iWordp};
    AstConst* ثابت outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ثابت مدخل_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_عقدة_تعبير* ثابت limp = outputRangeLeftp->رقم().toSInt() < مدخل_Range_Leftp->رقم().toSInt()
                                  ? outputRangeLeftp->clone_شجرة_Pure(خطأ)
                                  : مدخل_Range_Leftp->clone_شجرة_Pure(خطأ);
    loopp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(جديد AstLoopTest{flp, loopp, جديد AstLte{flp, rd(nVscp), limp}});

    // Done
    أرجع funcp;
}

Ast_عقدة_تعبير* TriggerKit::newAnySetCall(AstVarScope* ثابت vscp) ثابت {
    File_سطر* ثابت flp = v3Global.rootp()->قمة_Scopep()->fileline();
    إذا (!m_n_Vec_كلمات) أرجع جديد AstConst{flp, AstConst::BitFalse{}};

    AstCFunc* funcp = nullptr;
    إذا (vscp->dtypep() == m_trigVecDTypep) {
        إذا (!m_anySetVecp) m_anySetVecp = createAnySetFunc(m_trigVecDTypep);
        funcp = m_anySetVecp;
    } وإلا إذا (vscp->dtypep() == m_trigExtDTypep) {
        إذا (!m_anySetExtp) m_anySetExtp = createAnySetFunc(m_trigExtDTypep);
        funcp = m_anySetExtp;
    } وإلا {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->addArgsp(جديد AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetBit();
    أرجع callp;
}
Ast_عقدة_Stmt* TriggerKit::newClearCall(AstVarScope* ثابت vscp) ثابت {
    إذا (!m_n_Vec_كلمات) أرجع nullptr;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    إذا (!m_clearp) m_clearp = createClearFunc();
    File_سطر* ثابت flp = v3Global.rootp()->قمة_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_clearp};
    callp->addArgsp(جديد AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetVoid();
    أرجع callp->makeStmt();
}
Ast_عقدة_Stmt* TriggerKit::newOrIntoCall(AstVarScope* ثابت oVscp, AstVarScope* ثابت iVscp) ثابت {
    إذا (!m_n_Vec_كلمات) أرجع nullptr;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    ثابت size_t mask
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[mask];
    إذا (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_سطر* ثابت flp = v3Global.rootp()->قمة_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->addArgsp(جديد AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->addArgsp(جديد AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtypeSetVoid();
    أرجع callp->makeStmt();
}

Ast_عقدة_Stmt* TriggerKit::newCompBaseCall() ثابت {
    إذا (!m_n_Vec_كلمات) أرجع nullptr;
    File_سطر* ثابت flp = v3Global.rootp()->قمة_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_compVecp};
    callp->dtypeSetVoid();
    أرجع callp->makeStmt();
}

Ast_عقدة_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ثابت {
    إذا (!m_n_Pre_كلمات) أرجع nullptr;
    File_سطر* ثابت flp = v3Global.rootp()->قمة_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_compExtp};
    callp->addArgsp(جديد AstVarRef{flp, vscp, VAccess::READ});
    callp->dtypeSetVoid();
    أرجع callp->makeStmt();
}

Ast_عقدة_Stmt* TriggerKit::newDumpCall(AstVarScope* ثابت vscp, ثابت std::string& tag,
                                     منطقي debugOnly) ثابت {
    إذا (!m_n_Vec_كلمات) أرجع nullptr;
    AstCFunc* funcp = nullptr;
    إذا (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } وإلا إذا (vscp->dtypep() == m_trigExtDTypep) {
        إذا (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } وإلا {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_سطر* ثابت flp = v3Global.rootp()->قمة_Scopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->addArgsp(جديد AstVarRef{flp, vscp, VAccess::READ});
    callp->addArgsp(جديد AstConst{flp, AstConst::String{}, tag});
    callp->dtypeSetVoid();
    AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
    cstmtp->add("#ifdef VL_DEBUG\n");
    إذا (debugOnly) {
        cstmtp->add("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->add(callp->makeStmt());
        cstmtp->add("}\n");
    } وإلا {
        cstmtp->add(callp->makeStmt());
    }
    cstmtp->add("#endif");
    أرجع cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(ثابت std::string& name) ثابت {
    إذا (!m_n_Vec_كلمات) أرجع nullptr;
    أرجع v3Global.rootp()->قمة_Scopep()->createTemp("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extraTriggerIndex(uint32_t index) ثابت {
    UASSERT(index <= m_n_Extra_كلمات * WORD_SIZE, "Invalid external trigger index");
    أرجع index + m_n_Sense_كلمات * WORD_SIZE;
}

std::set<uint32_t> TriggerKit::triggerIndices(ثابت Ast_Sen_شجرة* senTreep, منطقي pre) ثابت {
    std::set<uint32_t> indices;
    لكل (ثابت AstSenItem* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), SenItem)) {
        uint32_t index = m_senItem2TrigIdx.at(*sip);
        إذا (pre) {
            UASSERT_OBJ(index < m_n_Pre_كلمات * WORD_SIZE, sip, "Not a 'pre' trigger");
            index += m_n_Vec_كلمات * WORD_SIZE;
        }
        indices.أدرج(index);
    }
    أرجع indices;
}

Ast_عقدة_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ثابت {
    index = extraTriggerIndex(index);
    ثابت uint32_t wordIndex = index / WORD_SIZE;
    ثابت uint32_t bitIndex = index % WORD_SIZE;
    File_سطر* ثابت flp = vscp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_عقدة_تعبير* ثابت wordp = جديد AstArraySel{flp, refp, static_cast<صحيح>(wordIndex)};
    Ast_عقدة_تعبير* ثابت trigLhsp = جديد AstSel{flp, wordp, static_cast<صحيح>(bitIndex), 1};
    Ast_عقدة_تعبير* ثابت trigRhsp = جديد AstVarRef{flp, vscp, VAccess::READ};
    أرجع جديد AstAssign{flp, trigLhsp, trigRhsp};
}

فراغ TriggerKit::addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ثابت {
    File_سطر* ثابت flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_عقدة* ثابت setp = newExtraTriggerAssignment(vscp, index);
    setp->addNext(جديد AstAssign{flp, جديد AstVarRef{flp, vscp, VAccess::WRITE},
                                جديد AstConst{flp, AstConst::BitFalse{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
فراغ TriggerKit::add_قيمة_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t index) ثابت {
    index += m_n_Sense_كلمات * WORD_SIZE;
    ثابت uint32_t wordIndex = index / WORD_SIZE;
    ثابت uint32_t bitIndex = index % WORD_SIZE;

    Ast_قمة_Scope* ثابت قمة_Scopep = netlistp->قمة_Scopep();
    File_سطر* ثابت flp = netlistp->fileline();
    Ast_عقدة_DType* ثابت dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    إذا (ثابت AstBasicDType* ثابت bdtypep = VN_CAST(dtypep, BasicDType)) {
        إذا (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            أرجع;
        }
    } وإلا إذا (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, عقدة_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        أرجع;
    }

    ثابت auto rdInst = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت auto wrPrev = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };
    ثابت auto rdPrev = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    ثابت std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* ثابت prevVscp = قمة_Scopep->createTemp(prevName, instVscp->dtypep());

    // Initialize prev = inst
    إذا (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = جديد AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        initFuncp->addStmtsp(cmhp->makeStmt());
    } وإلا {
        initFuncp->addStmtsp(جديد AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_عقدة_تعبير* neqp;
    إذا (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp
            = جديد AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtypeSetBit();
        neqp = cmhp;
    } وإلا {
        neqp = جديد AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_عقدة* updp;
    إذا (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = جديد AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        updp = cmhp->makeStmt();
    } وإلا {
        updp = جديد AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ثابت refp = جديد AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_عقدة_تعبير* ثابت wordp = جديد AstArraySel{flp, refp, static_cast<صحيح>(wordIndex)};
    Ast_عقدة_تعبير* ثابت trigLhsp = جديد AstSel{flp, wordp, static_cast<صحيح>(bitIndex), 1};
    Ast_عقدة* ثابت setp = جديد AstAssign{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->addNext(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ثابت std::string& name, منطقي slow, uint32_t n_Sense_كلمات,
                       uint32_t n_Extra_كلمات, uint32_t n_Pre_كلمات,
                       std::unordered_map<VNRef<ثابت AstSenItem>, size_t> senItem2TrigIdx,
                       ثابت std::vector<ثابت AstSenItem*>& senItemps, منطقي useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_كلمات{n_Sense_كلمات}
    , m_n_Extra_كلمات{n_Extra_كلمات}
    , m_n_Pre_كلمات{n_Pre_كلمات}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trigIdx2SenItem{senItemps} {
    // If no triggers, we don't need to generate anything
    إذا (!m_n_Vec_كلمات) أرجع;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trigIdx2SenItem.resize((m_n_Vec_كلمات + m_n_Pre_كلمات) * WORD_SIZE);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_كلمات * WORD_SIZE,
              m_trigIdx2SenItem.begin() + m_n_Vec_كلمات * WORD_SIZE);
    AstNetlist* ثابت netlistp = v3Global.rootp();
    AstScope* ثابت scopep = netlistp->قمة_Scopep()->scopep();
    File_سطر* ثابت flp = scopep->fileline();
    // Data type of a single trigger word
    m_wordDTypep = netlistp->findBitDType(WORD_SIZE, WORD_SIZE, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ثابت rp = جديد AstRange{flp, static_cast<صحيح>(m_n_Vec_كلمات - 1), 0};
    m_trigVecDTypep = جديد AstUnpackArrayDType{flp, m_wordDTypep, rp};
    netlistp->typeTablep()->addTypesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    إذا (m_n_Pre_كلمات) {
        AstRange* ثابت ep = جديد AstRange{flp, static_cast<صحيح>(m_n_Vec_كلمات + m_n_Pre_كلمات - 1), 0};
        m_trigExtDTypep = جديد AstUnpackArrayDType{flp, m_wordDTypep, ep};
        netlistp->typeTablep()->addTypesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } وإلا {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->قمة_Scopep()->createTemp("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->isInternal(true);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, true);
    m_dumpp->isStatic(true);
    m_dumpp->ifdef("VL_DEBUG");
    إذا (useAcc) {
        m_vscAccp
            = netlistp->قمة_Scopep()->createTemp("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->isInternal(true);
    }
}

AstAssign* TriggerKit::createSenTrigVecAssignment(AstVarScope* ثابت target,
                                                  std::vector<Ast_عقدة_تعبير*>& trigps) {
    File_سطر* ثابت flp = target->fileline();
    AstAssign* trigStmtsp = nullptr;
    // Assign sense triggers vector one word at a time
    لكل (size_t i = 0; i < trigps.size(); i += WORD_SIZE) {
        // Concatenate all bits in this trigger word using a balanced
        لكل (uint32_t level = 0; level < WORD_SIZE_LOG2; ++level) {
            ثابت uint32_t stride = 1 << level;
            لكل (uint32_t j = 0; j < WORD_SIZE; j += 2 * stride) {
                trigps[i + j] = جديد AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = nullptr;
            }
        }

        // Set the whole word in the trigger vector
        ثابت صحيح wordIndex = static_cast<صحيح>(i / WORD_SIZE);
        AstArraySel* ثابت aselp
            = جديد AstArraySel{flp, جديد AstVarRef{flp, target, VAccess::WRITE}, wordIndex};
        trigStmtsp = Ast_عقدة::addNext(trigStmtsp, جديد AstAssign{flp, aselp, trigps[i]});
    }
    أرجع trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ثابت initFuncp,  //
                              Sen_تعبير_Builder& sen_تعبير_Builder,  //
                              ثابت std::vector<ثابت Ast_Sen_شجرة*>& preTreeps,  //
                              ثابت std::vector<ثابت Ast_Sen_شجرة*>& senTreeps,  //
                              ثابت string& name,  //
                              ثابت ExtraTriggers& extraTriggers,  //
                              منطقي slow,  //
                              منطقي useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::vector<ثابت AstSenItem*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<ثابت AstSenItem>, size_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    لكل (ثابت Ast_Sen_شجرة* ثابت senTreep : preTreeps) {
        لكل (ثابت AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            إذا (pair.second) senItemps.ادفع_back(itemp);
        }
    }
    ثابت uint32_t n_Pre_Sen_عناصر = senItemps.size();
    V3Stats::addStat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_عناصر);
    // Number of pre triggers, rounded up to a full word.
    ثابت uint32_t nPreTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ثابت uint32_t n_Pre_كلمات = nPreTriggers / WORD_SIZE;

    // Process the rest of the trees
    لكل (ثابت Ast_Sen_شجرة* ثابت senTreep : senTreeps) {
        لكل (ثابت AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            إذا (pair.second) senItemps.ادفع_back(itemp);
        }
    }
    ثابت uint32_t n_Sen_عناصر = senItemps.size() - nPreTriggers;
    V3Stats::addStat("Scheduling, '" + name + "' sense triggers", n_Sen_عناصر + n_Pre_Sen_عناصر);
    // Number of sense triggers, rounded up to a full word
    ثابت uint32_t nSenseTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ثابت uint32_t n_Sense_كلمات = nSenseTriggers / WORD_SIZE;

    // Allocate space for the extra triggers
    V3Stats::addStat("Scheduling, '" + name + "' extra triggers", extraTriggers.size());
    // Number of extra triggers, rounded up to a full word.
    ثابت uint32_t nExtraTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(extraTriggers.size());
    ثابت uint32_t n_Extra_كلمات = nExtraTriggers / WORD_SIZE;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_كلمات, n_Extra_كلمات, n_Pre_كلمات, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    إذا (!kit.m_n_Vec_كلمات) أرجع kit;

    File_سطر* ثابت flp = netlistp->قمة_Scopep()->fileline();

    // Creates read/write reference
    ثابت auto rd = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::READ}; };
    ثابت auto wr = [flp](AstVarScope* vp) { أرجع جديد AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* ثابت dumpTrgp
        = newArgument(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* ثابت dumpTagp
        = newArgument(kit.m_dumpp, netlistp->findStringDType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* ثابت ifp = جديد AstIf{flp, جديد AstLogNot{flp, kit.newAnySetCall(dumpTrgp)}};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    ثابت auto addDebug = [&](uint32_t index, ثابت string& text) {
        ثابت صحيح wrdIndex = static_cast<صحيح>(index / WORD_SIZE);
        ثابت صحيح bitIndex = static_cast<صحيح>(index % WORD_SIZE);
        Ast_عقدة_تعبير* ثابت aselp = جديد AstArraySel{flp, rd(dumpTrgp), wrdIndex};
        Ast_عقدة_تعبير* ثابت condp = جديد AstSel{flp, aselp, bitIndex, 1};
        AstIf* ثابت ifp = جديد AstIf{flp, condp};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"' region trigger index " + std::to_string(index) + " is active: " + text
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::vector<Ast_عقدة_تعبير*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_عقدة_Stmt* initialTrigsp = nullptr;
    لكل (size_t i = 0; i < senItemps.size(); ++i) {
        ثابت AstSenItem* ثابت senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        إذا (!senItemp) {
            trigps.emplace_back(جديد AstConst{flp, AstConst::BitFalse{}});
            استمر;
        }

        // Create the trigger computation expression
        ثابت auto& pair = sen_تعبير_Builder.build(senItemp);
        trigps.emplace_back(pair.first);

        // Add initialization time trigger
        إذا (pair.second || v3Global.opt.xInitialEdge()) {
            ثابت صحيح wrdIndex = static_cast<صحيح>(i / WORD_SIZE);
            ثابت صحيح bitIndex = static_cast<صحيح>(i % WORD_SIZE);
            Ast_عقدة_تعبير* ثابت wordp = جديد AstArraySel{flp, wr(kit.m_vscp), wrdIndex};
            Ast_عقدة_تعبير* ثابت lhsp = جديد AstSel{flp, wordp, bitIndex, 1};
            Ast_عقدة_تعبير* ثابت rhsp = جديد AstConst{flp, AstConst::BitTrue{}};
            إذا (useAcc) {
                initFuncp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});
            } وإلا {
                initialTrigsp = Ast_عقدة::addNext(initialTrigsp, جديد AstAssign{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_شجرة(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        addDebug(i, desc);
    }
    UASSERT(trigps.size() == nSenseTriggers, "Inconsistent number of trigger expressions");

    AstAssign* ثابت trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    لكل (unsigned i = 0; i < extraTriggers.size(); ++i) {
        addDebug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    ثابت Sen_تعبير_Builder::Results senResults = sen_تعبير_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    لكل (Ast_عقدة_Stmt* ثابت nodep : senResults.m_inits) initFuncp->addStmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_قمة_Scope* ثابت قمة_Scopep = netlistp->قمة_Scopep();
    {
        AstCFunc* ثابت fp = kit.m_compVecp;
        // Trigger computation
        لكل (Ast_عقدة_Stmt* ثابت nodep : senResults.m_preUpdates) fp->addStmtsp(nodep);
        fp->addStmtsp(trigStmtsp);
        لكل (Ast_عقدة_Stmt* ثابت nodep : senResults.m_postUpdates) fp->addStmtsp(nodep);
        // Add the initialization time triggers
        إذا (initialTrigsp) {
            AstVarScope* ثابت initVscp = قمة_Scopep->createTemp("__V" + name + "DidInit", 1);
            AstIf* ثابت ifp = جديد AstIf{flp, جديد AstNot{flp, rd(initVscp)}};
            fp->addStmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->addThensp(util::setVar(initVscp, 1));
            ifp->addThensp(initialTrigsp);
        }
        util::splitCheck(fp);
    };
    // If there are 'pre' triggers, compute them
    إذا (kit.m_n_Pre_كلمات) {
        AstCFunc* ثابت fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ثابت latchedp
            = newArgument(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ثابت nVscp = قمة_Scopep->createTemp("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(true);
        // Add a loop to compute the pre words
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        fp->addStmtsp(util::setVar(nVscp, 0));
        fp->addStmtsp(loopp);
        // Loop body
        Ast_عقدة_تعبير* ثابت offsetp = جديد AstConst{flp, kit.m_n_Vec_كلمات};
        Ast_عقدة_تعبير* ثابت lIdxp = جديد AstAdd{flp, rd(nVscp), offsetp};
        Ast_عقدة_تعبير* ثابت lhsp = جديد AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_عقدة_تعبير* ثابت aWordp = جديد AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_عقدة_تعبير* ثابت bWordp = جديد AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_عقدة_تعبير* ثابت rhsp = جديد AstAnd{flp, aWordp, جديد AstNot{flp, bWordp}};
        Ast_عقدة_تعبير* ثابت limp = جديد AstConst{flp, AstConst::Widthed_قيمة{}, 32, n_Pre_كلمات};
        loopp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(nVscp));
        loopp->addStmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});
        util::splitCheck(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    إذا (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlinkFrBackWithNext()->delete_شجرة();
    // Done with the trigger dump function, split as might be large
    util::splitCheck(kit.m_dumpp);

    أرجع kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
صنف AwaitBeforeTrigVisitor final : عام VNVisitor {
    ثابت VNUser1InUse m_user1InUse;

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
    Sen_تعبير_Builder& m_sen_تعبير_Builder;
    // Generator of unique names for before-trigger function
    V3UniqueNames m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::vector<std::pair<AstCFunc*, Ast_Sen_شجرة*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::vector<std::pair<Ast_Sen_شجرة*, Ast_عقدة_تعبير*>> m_sen_شجرة_To_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_عقدة>, std::vector<AstSenItem*>> m_sen_تعبير_To_Sen_Item;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    ساكن Ast_عقدة* get_Sen_Hash_عقدة(ثابت AstSenItem* ثابت nodep) {
        إذا (AstVarRef* ثابت varRefp = VN_CAST(nodep->sensp(), VarRef)) أرجع varRefp;
        أرجع nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    فراغ fill_Sen_تعبير_To_Sen_Item() {
        لكل (auto sen_شجرة_Sched : m_sen_شجرة_To_Sched) {
            Ast_Sen_شجرة* ثابت senTreep = sen_شجرة_Sched.first;

            لكل (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ثابت VEdgeType edge = senItemp->edgeType();
                إذا (edge.anEdge() || edge == VEdgeType::ET_CHANGED
                    || edge == VEdgeType::ET_HYBRID) {
                    m_sen_تعبير_To_Sen_Item[*get_Sen_Hash_عقدة(senItemp)].ادفع_back(senItemp);
                }
            }
        }

        std::vector<VNRef<Ast_عقدة>> toRemove;
        لكل (ثابت auto& sen_تعبير_To_Sen_شجرة : m_sen_تعبير_To_Sen_Item) {
            std::vector<AstSenItem*> senItemps = sen_تعبير_To_Sen_شجرة.second;
            toRemove.ادفع_back(sen_تعبير_To_Sen_شجرة.first);
            لكل (size_t i = 1; i < senItemps.size(); ++i) {
                إذا (senItemps[i]->edgeType() != senItemps[i - 1]->edgeType()) {
                    toRemove.اسحب_back();
                    اكسر;
                }
            }
        }
        لكل (ثابت VNRef<Ast_عقدة> it : toRemove) m_sen_تعبير_To_Sen_Item.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<size_t, std::map<size_t, std::vector<Ast_عقدة_تعبير*>>>
    getUsedTriggersToTrees(ثابت std::set<size_t>& usedTriggers) {
        std::map<size_t, std::map<size_t, std::vector<Ast_عقدة_تعبير*>>> usedTrigsToUsingTrees;
        لكل (auto sen_شجرة_Sched : m_sen_شجرة_To_Sched) {
            ثابت Ast_Sen_شجرة* ثابت senTreep = sen_شجرة_Sched.first;
            Ast_عقدة_تعبير* ثابت shedp = sen_شجرة_Sched.second;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            لكل (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ثابت size_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                إذا (usedTriggers.find(idx) != usedTriggers.end()) {
                    usedTrigsToUsingTrees[idx / TriggerKit::WORD_SIZE]
                                         [size_t{1} << (idx % TriggerKit::WORD_SIZE)]
                                             .ادفع_back(shedp);
                }
            }
        }
        إذا (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            لكل (ثابت auto& triggersToTrees : usedTrigsToUsingTrees) {
                لكل (ثابت auto& bitsToTrees : triggersToTrees.second) {
                    ثابت std::set<ثابت Ast_عقدة_تعبير*> exprps{bitsToTrees.second.begin(),
                                                              bitsToTrees.second.end()};
                    UASSERT(bitsToTrees.second.size() == exprps.size(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        أرجع usedTrigsToUsingTrees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_شجرة* ثابت senTreep) {
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
            funcp->noLife(true);

            AstVar* ثابت argp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->findBasicDType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(true);
            argp->direction(VDirection::مدخل);
            funcp->addArgsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_back(funcp, senTreep);
        }
        AstCCall* ثابت callp = جديد AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtypeSetVoid();
        أرجع callp;
    }

    فراغ visit(AstCAwait* ثابت nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        إذا (ثابت AstCMethodHard* ثابت cMethodHardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            إذا (cMethodHardp->method() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ثابت beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_عقدة* ثابت pinp = cMethodHardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, cMethodHardp, "No event description");
                beforeTrigp->addArgsp(VN_AS(pinp, عقدة_تعبير)->clone_شجرة(خطأ));

                // Call the before-trigger function before the CAwait
                nodep->addHereThisAsNext(beforeTrigp->makeStmt());
                m_sen_شجرة_To_Sched.emplace_back(nodep->sentreep(), cMethodHardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    فراغ visit(Ast_عقدة* ثابت nodep) override { iterateChildren(nodep); }

عام:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_تعبير_Builder& sen_تعبير_Builder,
                           ثابت TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_تعبير_Builder{sen_تعبير_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_تعبير_To_Sen_Item();

        std::vector<Ast_عقدة_تعبير*> trigps;
        std::set<size_t> usedTriggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        لكل (ثابت auto& funcToUsedTriggers : m_generatedFuncs) {
            AstCFunc* ثابت funcp = funcToUsedTriggers.first;
            AstVarScope* ثابت vscp = VN_AS(funcp->user1p(), VarScope);
            File_سطر* ثابت flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_شجرة* ثابت senTreep = funcToUsedTriggers.second;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ثابت auto emplaceAt
                    = [flp, &trigps, &usedTriggers](Ast_عقدة_تعبير* ثابت exprp, ثابت size_t pos) {
                          ثابت size_t targetSize
                              = vlstd::roundUpToMultipleOf<TriggerKit::WORD_SIZE>(pos + 1);
                          إذا (trigps.capacity() < targetSize) trigps.reserve(targetSize * 2);
                          طالما (trigps.size() < targetSize) {
                              trigps.ادفع_back(جديد AstConst{flp, AstConst::BitFalse{}});
                          }
                          trigps[pos]->delete_شجرة();
                          trigps[pos] = exprp;
                          usedTriggers.أدرج(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                لكل (ثابت AstSenItem* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), SenItem)) {
                    ثابت size_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_تعبير_Builder.build(itemp).first, idx);
                    auto iter = m_sen_تعبير_To_Sen_Item.find(*get_Sen_Hash_عقدة(itemp));
                    إذا (iter != m_sen_تعبير_To_Sen_Item.end()) {
                        لكل (AstSenItem* ثابت additionalItemp : iter->second) {
                            ثابت size_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_تعبير_Builder.build(additionalItemp).first, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ثابت Sen_تعبير_Builder::Results results
                    = m_sen_تعبير_Builder.getResultsAndClearUpdates();
                لكل (Ast_عقدة_Stmt* ثابت stmtsp : results.m_inits) funcp->addStmtsp(stmtsp);
                لكل (Ast_عقدة_Stmt* ثابت stmtsp : results.m_preUpdates) funcp->addStmtsp(stmtsp);
                funcp->addStmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                لكل (Ast_عقدة_Stmt* ثابت stmtsp : results.m_postUpdates) funcp->addStmtsp(stmtsp);
            }

            ثابت std::map<size_t, std::map<size_t, std::vector<Ast_عقدة_تعبير*>>>
                usedTrigsToUsingTrees = getUsedTriggersToTrees(usedTriggers);
            usedTriggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            ثابت auto getIdx = [flp](AstVarScope* ثابت scocep, VAccess access, size_t idx) {
                أرجع جديد AstArraySel{flp, جديد AstVarRef{flp, scocep, access},
                                       جديد AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* ثابت argpVscp = جديد AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->addVarsp(argpVscp);

            // Mark as ready triggered schedulers
            لكل (ثابت auto& triggersToTrees : usedTrigsToUsingTrees) {
                ثابت size_t word = triggersToTrees.first;

                لكل (ثابت auto& bitsToTrees : triggersToTrees.second) {
                    ثابت size_t bit = bitsToTrees.first;
                    ثابت auto& schedulers = bitsToTrees.second;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* ثابت maskConstp = جديد AstConst{flp, AstConst::Unsized64{}, bit};
                    AstAnd* ثابت condp
                        = جديد AstAnd{flp, getIdx(vscp, VAccess::READ, word), maskConstp};
                    AstIf* ثابت ifp = جديد AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    لكل (Ast_عقدة_تعبير* ثابت schedp : schedulers) {
                        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                            flp, schedp->clone_شجرة(خطأ), VCMethod::SCHED_READY};
                        callp->dtypeSetVoid();
                        callp->addPinsp(جديد AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->addThensp(callp->makeStmt());
                    }
                    funcp->addStmtsp(ifp);
                }
            }

            AstVarScope* ثابت vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            لكل (ثابت auto& triggersToTrees : usedTrigsToUsingTrees) {
                ثابت size_t word = triggersToTrees.first;
                funcp->addStmtsp(جديد AstAssign{flp, getIdx(vscAccp, VAccess::WRITE, word),
                                               جديد AstOr{flp, getIdx(vscAccp, VAccess::READ, word),
                                                         getIdx(vscp, VAccess::READ, word)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = افتراضي;
};

فراغ beforeTrigVisitor(AstNetlist* netlistp, Sen_تعبير_Builder& sen_تعبير_Builder,
                       ثابت TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_تعبير_Builder, trigKit};
}

}  // namespace V3Sched
