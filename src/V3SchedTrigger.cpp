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

فضاینام V3Sched {

فضاینام {

using util::newArgument;

AstVarScope* newLocal(AstCFunc* funcp, Ast_گره_DType* dtypep, ثابت std::string& name) {
    File_سطر* ثابت flp = funcp->fileline();
    AstScope* ثابت scopep = funcp->scopep();
    AstVar* ثابت varp = جدید AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(درست);
    varp->noReset(درست);
    funcp->addVarsp(varp);
    AstVarScope* ثابت vscp = جدید AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    بازگشت vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() ثابت {
    UASSERT(m_n_Pre_واژگان, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->بالاترین_Scopep()->fileline();
    Ast_گره_DType* ثابت u32DTypep = netlistp->findUInt32DType();
    Ast_گره_DType* ثابت strDtypep = netlistp->findStringDType();

    // Dumping function always slow
    ثابت std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, درست);
    funcp->isStatic(درست);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* ثابت eVscp = newArgument(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* ثابت tVscp = newArgument(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    ثابت auto rd = [flp](AstVarScope* vp) { بازگشت جدید AstVarRef{flp, vp, VAccess::READ}; };
    ثابت auto wr = [flp](AstVarScope* vp) { بازگشت جدید AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* ثابت vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* ثابت iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->addStmtsp(util::setVar(iVscp, 0));
        // Add loop
        AstLoop* ثابت loopp = جدید AstLoop{flp};
        funcp->addStmtsp(loopp);
        // Loop body
        Ast_گره_عبارت* ثابت lhsp = جدید AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_گره_عبارت* ثابت rhsp = جدید AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_گره_عبارت* ثابت limp = جدید AstConst{flp, AstConst::Widthed_مقدار{}, 32, m_n_Vec_واژگان};
        loopp->addStmtsp(جدید AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(iVscp));
        loopp->addStmtsp(جدید AstLoopTest{flp, loopp, جدید AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* ثابت callp = جدید AstCCall{flp, m_dumpp};
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
            AstLoop* ثابت loopp = جدید AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_گره_عبارت* ثابت lhsp = جدید AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_گره_عبارت* ثابت rhsp = جدید AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_گره_عبارت* ثابت limp = جدید AstConst{flp, AstConst::Widthed_مقدار{}, 32, m_n_Pre_واژگان};
            loopp->addStmtsp(جدید AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(جدید AstLoopTest{flp, loopp, جدید AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* ثابت loopp = جدید AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_گره_عبارت* ثابت lhsp = جدید AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_گره_عبارت* ثابت rhsp = جدید AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
            Ast_گره_عبارت* ثابت limp = جدید AstConst{flp, AstConst::Widthed_مقدار{}, 32, m_n_Vec_واژگان};
            loopp->addStmtsp(جدید AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(جدید AstLoopTest{flp, loopp, جدید AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* ثابت callp = جدید AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(pVscp));
        callp->addArgsp(
            جدید AstConcatN{flp, rd(tVscp), جدید AstConst{flp, AstConst::String{}, " pre"}});
        funcp->addStmtsp(callp->makeStmt());
    }

    بازگشت funcp;
}

AstCFunc* TriggerKit::createAnySetFunc(AstUnpackArrayDType* ثابت dtypep) ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->بالاترین_Scopep()->fileline();
    Ast_گره_DType* ثابت u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(درست);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* ثابت iVscp = newArgument(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    ثابت auto rd = [flp](AstVarScope* vp) { بازگشت جدید AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* ثابت loopp = جدید AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);
    funcp->addStmtsp(جدید AstCReturn{flp, جدید AstConst{flp, AstConst::BitFalse{}}});

    // Loop body
    ثابت uint32_t n_واژگان = dtypep->elementsConst();
    Ast_گره_عبارت* ثابت condp = جدید AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_گره_Stmt* ثابت thenp = جدید AstCReturn{flp, جدید AstConst{flp, AstConst::BitTrue{}}};
    Ast_گره_عبارت* ثابت limp = جدید AstConst{flp, AstConst::Widthed_مقدار{}, 32, n_واژگان};
    loopp->addStmtsp(جدید AstIf{flp, condp, thenp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(جدید AstLoopTest{flp, loopp, جدید AstLt{flp, rd(nVscp), limp}});

    // Done
    بازگشت funcp;
}
AstCFunc* TriggerKit::createClearFunc() ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->بالاترین_Scopep()->fileline();
    Ast_گره_DType* ثابت u32DTypep = netlistp->findUInt32DType();

    // Create function
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(درست);

    // Add arguments
    AstVarScope* ثابت oVscp = newArgument(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ثابت auto rd = [flp](AstVarScope* vp) { بازگشت جدید AstVarRef{flp, vp, VAccess::READ}; };
    ثابت auto wr = [flp](AstVarScope* vp) { بازگشت جدید AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ثابت loopp = جدید AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_گره_عبارت* ثابت lhsp = جدید AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_گره_عبارت* ثابت rhsp = جدید AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
    Ast_گره_عبارت* ثابت limp = جدید AstConst{flp, AstConst::Widthed_مقدار{}, 32, m_n_Vec_واژگان};
    loopp->addStmtsp(جدید AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(جدید AstLoopTest{flp, loopp, جدید AstLt{flp, rd(nVscp), limp}});

    // Done
    بازگشت funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ثابت oDtypep,
                                       AstUnpackArrayDType* ثابت iDtypep) ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سطر* ثابت flp = netlistp->بالاترین_Scopep()->fileline();
    Ast_گره_DType* ثابت u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* ثابت funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(درست);

    // Add arguments
    AstVarScope* ثابت oVscp = newArgument(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* ثابت iVscp = newArgument(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ثابت nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ثابت auto rd = [flp](AstVarScope* vp) { بازگشت جدید AstVarRef{flp, vp, VAccess::READ}; };
    ثابت auto wr = [flp](AstVarScope* vp) { بازگشت جدید AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ثابت loopp = جدید AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_گره_عبارت* ثابت lhsp = جدید AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_گره_عبارت* ثابت oWordp = جدید AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_گره_عبارت* ثابت iWordp = جدید AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_گره_عبارت* ثابت rhsp = جدید AstOr{flp, oWordp, iWordp};
    AstConst* ثابت outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ثابت درون_داد_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_گره_عبارت* ثابت limp = outputRangeLeftp->رقم().toSInt() < درون_داد_Range_Leftp->رقم().toSInt()
                                  ? outputRangeLeftp->clone_درخت_Pure(نادرست)
                                  : درون_داد_Range_Leftp->clone_درخت_Pure(نادرست);
    loopp->addStmtsp(جدید AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(جدید AstLoopTest{flp, loopp, جدید AstLte{flp, rd(nVscp), limp}});

    // Done
    بازگشت funcp;
}

Ast_گره_عبارت* TriggerKit::newAnySetCall(AstVarScope* ثابت vscp) ثابت {
    File_سطر* ثابت flp = v3Global.rootp()->بالاترین_Scopep()->fileline();
    اگر (!m_n_Vec_واژگان) بازگشت جدید AstConst{flp, AstConst::BitFalse{}};

    AstCFunc* funcp = nullptr;
    اگر (vscp->dtypep() == m_trigVecDTypep) {
        اگر (!m_anySetVecp) m_anySetVecp = createAnySetFunc(m_trigVecDTypep);
        funcp = m_anySetVecp;
    } وگرنه اگر (vscp->dtypep() == m_trigExtDTypep) {
        اگر (!m_anySetExtp) m_anySetExtp = createAnySetFunc(m_trigExtDTypep);
        funcp = m_anySetExtp;
    } وگرنه {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ثابت callp = جدید AstCCall{flp, funcp};
    callp->addArgsp(جدید AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetBit();
    بازگشت callp;
}
Ast_گره_Stmt* TriggerKit::newClearCall(AstVarScope* ثابت vscp) ثابت {
    اگر (!m_n_Vec_واژگان) بازگشت nullptr;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    اگر (!m_clearp) m_clearp = createClearFunc();
    File_سطر* ثابت flp = v3Global.rootp()->بالاترین_Scopep()->fileline();
    AstCCall* ثابت callp = جدید AstCCall{flp, m_clearp};
    callp->addArgsp(جدید AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetVoid();
    بازگشت callp->makeStmt();
}
Ast_گره_Stmt* TriggerKit::newOrIntoCall(AstVarScope* ثابت oVscp, AstVarScope* ثابت iVscp) ثابت {
    اگر (!m_n_Vec_واژگان) بازگشت nullptr;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    ثابت size_t mask
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[mask];
    اگر (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_سطر* ثابت flp = v3Global.rootp()->بالاترین_Scopep()->fileline();
    AstCCall* ثابت callp = جدید AstCCall{flp, funcp};
    callp->addArgsp(جدید AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->addArgsp(جدید AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtypeSetVoid();
    بازگشت callp->makeStmt();
}

Ast_گره_Stmt* TriggerKit::newCompBaseCall() ثابت {
    اگر (!m_n_Vec_واژگان) بازگشت nullptr;
    File_سطر* ثابت flp = v3Global.rootp()->بالاترین_Scopep()->fileline();
    AstCCall* ثابت callp = جدید AstCCall{flp, m_compVecp};
    callp->dtypeSetVoid();
    بازگشت callp->makeStmt();
}

Ast_گره_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ثابت {
    اگر (!m_n_Pre_واژگان) بازگشت nullptr;
    File_سطر* ثابت flp = v3Global.rootp()->بالاترین_Scopep()->fileline();
    AstCCall* ثابت callp = جدید AstCCall{flp, m_compExtp};
    callp->addArgsp(جدید AstVarRef{flp, vscp, VAccess::READ});
    callp->dtypeSetVoid();
    بازگشت callp->makeStmt();
}

Ast_گره_Stmt* TriggerKit::newDumpCall(AstVarScope* ثابت vscp, ثابت std::string& tag,
                                     منطقی debugOnly) ثابت {
    اگر (!m_n_Vec_واژگان) بازگشت nullptr;
    AstCFunc* funcp = nullptr;
    اگر (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } وگرنه اگر (vscp->dtypep() == m_trigExtDTypep) {
        اگر (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } وگرنه {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_سطر* ثابت flp = v3Global.rootp()->بالاترین_Scopep()->fileline();
    AstCCall* ثابت callp = جدید AstCCall{flp, funcp};
    callp->addArgsp(جدید AstVarRef{flp, vscp, VAccess::READ});
    callp->addArgsp(جدید AstConst{flp, AstConst::String{}, tag});
    callp->dtypeSetVoid();
    AstCStmt* ثابت cstmtp = جدید AstCStmt{flp};
    cstmtp->add("#ifdef VL_DEBUG\n");
    اگر (debugOnly) {
        cstmtp->add("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->add(callp->makeStmt());
        cstmtp->add("}\n");
    } وگرنه {
        cstmtp->add(callp->makeStmt());
    }
    cstmtp->add("#endif");
    بازگشت cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(ثابت std::string& name) ثابت {
    اگر (!m_n_Vec_واژگان) بازگشت nullptr;
    بازگشت v3Global.rootp()->بالاترین_Scopep()->createTemp("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extraTriggerIndex(uint32_t index) ثابت {
    UASSERT(index <= m_n_Extra_واژگان * WORD_SIZE, "Invalid external trigger index");
    بازگشت index + m_n_Sense_واژگان * WORD_SIZE;
}

std::set<uint32_t> TriggerKit::triggerIndices(ثابت Ast_Sen_درخت* senTreep, منطقی pre) ثابت {
    std::set<uint32_t> indices;
    برای (ثابت AstSenItem* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), SenItem)) {
        uint32_t index = m_senItem2TrigIdx.at(*sip);
        اگر (pre) {
            UASSERT_OBJ(index < m_n_Pre_واژگان * WORD_SIZE, sip, "Not a 'pre' trigger");
            index += m_n_Vec_واژگان * WORD_SIZE;
        }
        indices.درج(index);
    }
    بازگشت indices;
}

Ast_گره_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ثابت {
    index = extraTriggerIndex(index);
    ثابت uint32_t wordIndex = index / WORD_SIZE;
    ثابت uint32_t bitIndex = index % WORD_SIZE;
    File_سطر* ثابت flp = vscp->fileline();
    AstVarRef* ثابت refp = جدید AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_گره_عبارت* ثابت wordp = جدید AstArraySel{flp, refp, static_cast<صحیح>(wordIndex)};
    Ast_گره_عبارت* ثابت trigLhsp = جدید AstSel{flp, wordp, static_cast<صحیح>(bitIndex), 1};
    Ast_گره_عبارت* ثابت trigRhsp = جدید AstVarRef{flp, vscp, VAccess::READ};
    بازگشت جدید AstAssign{flp, trigLhsp, trigRhsp};
}

تهی TriggerKit::addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ثابت {
    File_سطر* ثابت flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_گره* ثابت setp = newExtraTriggerAssignment(vscp, index);
    setp->addNext(جدید AstAssign{flp, جدید AstVarRef{flp, vscp, VAccess::WRITE},
                                جدید AstConst{flp, AstConst::BitFalse{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
تهی TriggerKit::add_مقدار_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t index) ثابت {
    index += m_n_Sense_واژگان * WORD_SIZE;
    ثابت uint32_t wordIndex = index / WORD_SIZE;
    ثابت uint32_t bitIndex = index % WORD_SIZE;

    Ast_بالاترین_Scope* ثابت بالاترین_Scopep = netlistp->بالاترین_Scopep();
    File_سطر* ثابت flp = netlistp->fileline();
    Ast_گره_DType* ثابت dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    اگر (ثابت AstBasicDType* ثابت bdtypep = VN_CAST(dtypep, BasicDType)) {
        اگر (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            بازگشت;
        }
    } وگرنه اگر (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, گره_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        بازگشت;
    }

    ثابت auto rdInst = [flp](AstVarScope* vp) { بازگشت جدید AstVarRef{flp, vp, VAccess::READ}; };
    ثابت auto wrPrev = [flp](AstVarScope* vp) { بازگشت جدید AstVarRef{flp, vp, VAccess::WRITE}; };
    ثابت auto rdPrev = [flp](AstVarScope* vp) { بازگشت جدید AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    ثابت std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* ثابت prevVscp = بالاترین_Scopep->createTemp(prevName, instVscp->dtypep());

    // Initialize prev = inst
    اگر (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = جدید AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        initFuncp->addStmtsp(cmhp->makeStmt());
    } وگرنه {
        initFuncp->addStmtsp(جدید AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_گره_عبارت* neqp;
    اگر (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp
            = جدید AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtypeSetBit();
        neqp = cmhp;
    } وگرنه {
        neqp = جدید AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_گره* updp;
    اگر (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = جدید AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        updp = cmhp->makeStmt();
    } وگرنه {
        updp = جدید AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ثابت refp = جدید AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_گره_عبارت* ثابت wordp = جدید AstArraySel{flp, refp, static_cast<صحیح>(wordIndex)};
    Ast_گره_عبارت* ثابت trigLhsp = جدید AstSel{flp, wordp, static_cast<صحیح>(bitIndex), 1};
    Ast_گره* ثابت setp = جدید AstAssign{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->addNext(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ثابت std::string& name, منطقی slow, uint32_t n_Sense_واژگان,
                       uint32_t n_Extra_واژگان, uint32_t n_Pre_واژگان,
                       std::unordered_map<VNRef<ثابت AstSenItem>, size_t> senItem2TrigIdx,
                       ثابت std::vector<ثابت AstSenItem*>& senItemps, منطقی useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_واژگان{n_Sense_واژگان}
    , m_n_Extra_واژگان{n_Extra_واژگان}
    , m_n_Pre_واژگان{n_Pre_واژگان}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trigIdx2SenItem{senItemps} {
    // If no triggers, we don't need to generate anything
    اگر (!m_n_Vec_واژگان) بازگشت;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trigIdx2SenItem.resize((m_n_Vec_واژگان + m_n_Pre_واژگان) * WORD_SIZE);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_واژگان * WORD_SIZE,
              m_trigIdx2SenItem.begin() + m_n_Vec_واژگان * WORD_SIZE);
    AstNetlist* ثابت netlistp = v3Global.rootp();
    AstScope* ثابت scopep = netlistp->بالاترین_Scopep()->scopep();
    File_سطر* ثابت flp = scopep->fileline();
    // Data type of a single trigger word
    m_wordDTypep = netlistp->findBitDType(WORD_SIZE, WORD_SIZE, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ثابت rp = جدید AstRange{flp, static_cast<صحیح>(m_n_Vec_واژگان - 1), 0};
    m_trigVecDTypep = جدید AstUnpackArrayDType{flp, m_wordDTypep, rp};
    netlistp->typeTablep()->addTypesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    اگر (m_n_Pre_واژگان) {
        AstRange* ثابت ep = جدید AstRange{flp, static_cast<صحیح>(m_n_Vec_واژگان + m_n_Pre_واژگان - 1), 0};
        m_trigExtDTypep = جدید AstUnpackArrayDType{flp, m_wordDTypep, ep};
        netlistp->typeTablep()->addTypesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } وگرنه {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->بالاترین_Scopep()->createTemp("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->isInternal(درست);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, درست);
    m_dumpp->isStatic(درست);
    m_dumpp->ifdef("VL_DEBUG");
    اگر (useAcc) {
        m_vscAccp
            = netlistp->بالاترین_Scopep()->createTemp("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->isInternal(درست);
    }
}

AstAssign* TriggerKit::createSenTrigVecAssignment(AstVarScope* ثابت target,
                                                  std::vector<Ast_گره_عبارت*>& trigps) {
    File_سطر* ثابت flp = target->fileline();
    AstAssign* trigStmtsp = nullptr;
    // Assign sense triggers vector one word at a time
    برای (size_t i = 0; i < trigps.size(); i += WORD_SIZE) {
        // Concatenate all bits in this trigger word using a balanced
        برای (uint32_t level = 0; level < WORD_SIZE_LOG2; ++level) {
            ثابت uint32_t stride = 1 << level;
            برای (uint32_t j = 0; j < WORD_SIZE; j += 2 * stride) {
                trigps[i + j] = جدید AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = nullptr;
            }
        }

        // Set the whole word in the trigger vector
        ثابت صحیح wordIndex = static_cast<صحیح>(i / WORD_SIZE);
        AstArraySel* ثابت aselp
            = جدید AstArraySel{flp, جدید AstVarRef{flp, target, VAccess::WRITE}, wordIndex};
        trigStmtsp = Ast_گره::addNext(trigStmtsp, جدید AstAssign{flp, aselp, trigps[i]});
    }
    بازگشت trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ثابت initFuncp,  //
                              Sen_عبارت_Builder& sen_عبارت_Builder,  //
                              ثابت std::vector<ثابت Ast_Sen_درخت*>& preTreeps,  //
                              ثابت std::vector<ثابت Ast_Sen_درخت*>& senTreeps,  //
                              ثابت string& name,  //
                              ثابت ExtraTriggers& extraTriggers,  //
                              منطقی slow,  //
                              منطقی useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::vector<ثابت AstSenItem*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<ثابت AstSenItem>, size_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    برای (ثابت Ast_Sen_درخت* ثابت senTreep : preTreeps) {
        برای (ثابت AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            اگر (pair.second) senItemps.افزودن_back(itemp);
        }
    }
    ثابت uint32_t n_Pre_Sen_اقلام = senItemps.size();
    V3Stats::addStat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_اقلام);
    // Number of pre triggers, rounded up to a full word.
    ثابت uint32_t nPreTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ثابت uint32_t n_Pre_واژگان = nPreTriggers / WORD_SIZE;

    // Process the rest of the trees
    برای (ثابت Ast_Sen_درخت* ثابت senTreep : senTreeps) {
        برای (ثابت AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            اگر (pair.second) senItemps.افزودن_back(itemp);
        }
    }
    ثابت uint32_t n_Sen_اقلام = senItemps.size() - nPreTriggers;
    V3Stats::addStat("Scheduling, '" + name + "' sense triggers", n_Sen_اقلام + n_Pre_Sen_اقلام);
    // Number of sense triggers, rounded up to a full word
    ثابت uint32_t nSenseTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ثابت uint32_t n_Sense_واژگان = nSenseTriggers / WORD_SIZE;

    // Allocate space for the extra triggers
    V3Stats::addStat("Scheduling, '" + name + "' extra triggers", extraTriggers.size());
    // Number of extra triggers, rounded up to a full word.
    ثابت uint32_t nExtraTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(extraTriggers.size());
    ثابت uint32_t n_Extra_واژگان = nExtraTriggers / WORD_SIZE;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_واژگان, n_Extra_واژگان, n_Pre_واژگان, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    اگر (!kit.m_n_Vec_واژگان) بازگشت kit;

    File_سطر* ثابت flp = netlistp->بالاترین_Scopep()->fileline();

    // Creates read/write reference
    ثابت auto rd = [flp](AstVarScope* vp) { بازگشت جدید AstVarRef{flp, vp, VAccess::READ}; };
    ثابت auto wr = [flp](AstVarScope* vp) { بازگشت جدید AstVarRef{flp, vp, VAccess::WRITE}; };

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
        AstIf* ثابت ifp = جدید AstIf{flp, جدید AstLogNot{flp, kit.newAnySetCall(dumpTrgp)}};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* ثابت cstmtp = جدید AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    ثابت auto addDebug = [&](uint32_t index, ثابت string& text) {
        ثابت صحیح wrdIndex = static_cast<صحیح>(index / WORD_SIZE);
        ثابت صحیح bitIndex = static_cast<صحیح>(index % WORD_SIZE);
        Ast_گره_عبارت* ثابت aselp = جدید AstArraySel{flp, rd(dumpTrgp), wrdIndex};
        Ast_گره_عبارت* ثابت condp = جدید AstSel{flp, aselp, bitIndex, 1};
        AstIf* ثابت ifp = جدید AstIf{flp, condp};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* ثابت cstmtp = جدید AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"' region trigger index " + std::to_string(index) + " is active: " + text
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::vector<Ast_گره_عبارت*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_گره_Stmt* initialTrigsp = nullptr;
    برای (size_t i = 0; i < senItemps.size(); ++i) {
        ثابت AstSenItem* ثابت senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        اگر (!senItemp) {
            trigps.emplace_back(جدید AstConst{flp, AstConst::BitFalse{}});
            ادامه;
        }

        // Create the trigger computation expression
        ثابت auto& pair = sen_عبارت_Builder.build(senItemp);
        trigps.emplace_back(pair.first);

        // Add initialization time trigger
        اگر (pair.second || v3Global.opt.xInitialEdge()) {
            ثابت صحیح wrdIndex = static_cast<صحیح>(i / WORD_SIZE);
            ثابت صحیح bitIndex = static_cast<صحیح>(i % WORD_SIZE);
            Ast_گره_عبارت* ثابت wordp = جدید AstArraySel{flp, wr(kit.m_vscp), wrdIndex};
            Ast_گره_عبارت* ثابت lhsp = جدید AstSel{flp, wordp, bitIndex, 1};
            Ast_گره_عبارت* ثابت rhsp = جدید AstConst{flp, AstConst::BitTrue{}};
            اگر (useAcc) {
                initFuncp->addStmtsp(جدید AstAssign{flp, lhsp, rhsp});
            } وگرنه {
                initialTrigsp = Ast_گره::addNext(initialTrigsp, جدید AstAssign{flp, lhsp, rhsp});
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
        addDebug(i, desc);
    }
    UASSERT(trigps.size() == nSenseTriggers, "Inconsistent number of trigger expressions");

    AstAssign* ثابت trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    برای (unsigned i = 0; i < extraTriggers.size(); ++i) {
        addDebug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    ثابت Sen_عبارت_Builder::Results senResults = sen_عبارت_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    برای (Ast_گره_Stmt* ثابت nodep : senResults.m_inits) initFuncp->addStmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_بالاترین_Scope* ثابت بالاترین_Scopep = netlistp->بالاترین_Scopep();
    {
        AstCFunc* ثابت fp = kit.m_compVecp;
        // Trigger computation
        برای (Ast_گره_Stmt* ثابت nodep : senResults.m_preUpdates) fp->addStmtsp(nodep);
        fp->addStmtsp(trigStmtsp);
        برای (Ast_گره_Stmt* ثابت nodep : senResults.m_postUpdates) fp->addStmtsp(nodep);
        // Add the initialization time triggers
        اگر (initialTrigsp) {
            AstVarScope* ثابت initVscp = بالاترین_Scopep->createTemp("__V" + name + "DidInit", 1);
            AstIf* ثابت ifp = جدید AstIf{flp, جدید AstNot{flp, rd(initVscp)}};
            fp->addStmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->addThensp(util::setVar(initVscp, 1));
            ifp->addThensp(initialTrigsp);
        }
        util::splitCheck(fp);
    };
    // If there are 'pre' triggers, compute them
    اگر (kit.m_n_Pre_واژگان) {
        AstCFunc* ثابت fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ثابت latchedp
            = newArgument(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ثابت nVscp = بالاترین_Scopep->createTemp("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(درست);
        // Add a loop to compute the pre words
        AstLoop* ثابت loopp = جدید AstLoop{flp};
        fp->addStmtsp(util::setVar(nVscp, 0));
        fp->addStmtsp(loopp);
        // Loop body
        Ast_گره_عبارت* ثابت offsetp = جدید AstConst{flp, kit.m_n_Vec_واژگان};
        Ast_گره_عبارت* ثابت lIdxp = جدید AstAdd{flp, rd(nVscp), offsetp};
        Ast_گره_عبارت* ثابت lhsp = جدید AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_گره_عبارت* ثابت aWordp = جدید AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_گره_عبارت* ثابت bWordp = جدید AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_گره_عبارت* ثابت rhsp = جدید AstAnd{flp, aWordp, جدید AstNot{flp, bWordp}};
        Ast_گره_عبارت* ثابت limp = جدید AstConst{flp, AstConst::Widthed_مقدار{}, 32, n_Pre_واژگان};
        loopp->addStmtsp(جدید AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(nVscp));
        loopp->addStmtsp(جدید AstLoopTest{flp, loopp, جدید AstLt{flp, rd(nVscp), limp}});
        util::splitCheck(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    اگر (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlinkFrBackWithNext()->delete_درخت();
    // Done with the trigger dump function, split as might be large
    util::splitCheck(kit.m_dumpp);

    بازگشت kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
رده AwaitBeforeTrigVisitor final : عمومی VNVisitor {
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
    Sen_عبارت_Builder& m_sen_عبارت_Builder;
    // Generator of unique names for before-trigger function
    V3UniqueNames m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::vector<std::pair<AstCFunc*, Ast_Sen_درخت*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::vector<std::pair<Ast_Sen_درخت*, Ast_گره_عبارت*>> m_sen_درخت_To_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_گره>, std::vector<AstSenItem*>> m_sen_عبارت_To_Sen_Item;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    ایستا Ast_گره* get_Sen_Hash_گره(ثابت AstSenItem* ثابت nodep) {
        اگر (AstVarRef* ثابت varRefp = VN_CAST(nodep->sensp(), VarRef)) بازگشت varRefp;
        بازگشت nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    تهی fill_Sen_عبارت_To_Sen_Item() {
        برای (auto sen_درخت_Sched : m_sen_درخت_To_Sched) {
            Ast_Sen_درخت* ثابت senTreep = sen_درخت_Sched.first;

            برای (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ثابت VEdgeType edge = senItemp->edgeType();
                اگر (edge.anEdge() || edge == VEdgeType::ET_CHANGED
                    || edge == VEdgeType::ET_HYBRID) {
                    m_sen_عبارت_To_Sen_Item[*get_Sen_Hash_گره(senItemp)].افزودن_back(senItemp);
                }
            }
        }

        std::vector<VNRef<Ast_گره>> toRemove;
        برای (ثابت auto& sen_عبارت_To_Sen_درخت : m_sen_عبارت_To_Sen_Item) {
            std::vector<AstSenItem*> senItemps = sen_عبارت_To_Sen_درخت.second;
            toRemove.افزودن_back(sen_عبارت_To_Sen_درخت.first);
            برای (size_t i = 1; i < senItemps.size(); ++i) {
                اگر (senItemps[i]->edgeType() != senItemps[i - 1]->edgeType()) {
                    toRemove.برداشت_back();
                    بشکن;
                }
            }
        }
        برای (ثابت VNRef<Ast_گره> it : toRemove) m_sen_عبارت_To_Sen_Item.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<size_t, std::map<size_t, std::vector<Ast_گره_عبارت*>>>
    getUsedTriggersToTrees(ثابت std::set<size_t>& usedTriggers) {
        std::map<size_t, std::map<size_t, std::vector<Ast_گره_عبارت*>>> usedTrigsToUsingTrees;
        برای (auto sen_درخت_Sched : m_sen_درخت_To_Sched) {
            ثابت Ast_Sen_درخت* ثابت senTreep = sen_درخت_Sched.first;
            Ast_گره_عبارت* ثابت shedp = sen_درخت_Sched.second;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            برای (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ثابت size_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                اگر (usedTriggers.find(idx) != usedTriggers.end()) {
                    usedTrigsToUsingTrees[idx / TriggerKit::WORD_SIZE]
                                         [size_t{1} << (idx % TriggerKit::WORD_SIZE)]
                                             .افزودن_back(shedp);
                }
            }
        }
        اگر (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            برای (ثابت auto& triggersToTrees : usedTrigsToUsingTrees) {
                برای (ثابت auto& bitsToTrees : triggersToTrees.second) {
                    ثابت std::set<ثابت Ast_گره_عبارت*> exprps{bitsToTrees.second.begin(),
                                                              bitsToTrees.second.end()};
                    UASSERT(bitsToTrees.second.size() == exprps.size(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        بازگشت usedTrigsToUsingTrees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_درخت* ثابت senTreep) {
        File_سطر* ثابت flp = senTreep->fileline();
        اگر (!senTreep->user1p()) {
            AstCFunc* ثابت funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), نادرست);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* ثابت tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(درست);

            AstVar* ثابت argp = جدید AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->findBasicDType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(درست);
            argp->direction(VDirection::درون_داد);
            funcp->addArgsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_back(funcp, senTreep);
        }
        AstCCall* ثابت callp = جدید AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtypeSetVoid();
        بازگشت callp;
    }

    تهی visit(AstCAwait* ثابت nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        اگر (ثابت AstCMethodHard* ثابت cMethodHardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            اگر (cMethodHardp->method() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ثابت beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_گره* ثابت pinp = cMethodHardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, cMethodHardp, "No event description");
                beforeTrigp->addArgsp(VN_AS(pinp, گره_عبارت)->clone_درخت(نادرست));

                // Call the before-trigger function before the CAwait
                nodep->addHereThisAsNext(beforeTrigp->makeStmt());
                m_sen_درخت_To_Sched.emplace_back(nodep->sentreep(), cMethodHardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    تهی visit(Ast_گره* ثابت nodep) override { iterateChildren(nodep); }

عمومی:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_عبارت_Builder& sen_عبارت_Builder,
                           ثابت TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_عبارت_Builder{sen_عبارت_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_عبارت_To_Sen_Item();

        std::vector<Ast_گره_عبارت*> trigps;
        std::set<size_t> usedTriggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        برای (ثابت auto& funcToUsedTriggers : m_generatedFuncs) {
            AstCFunc* ثابت funcp = funcToUsedTriggers.first;
            AstVarScope* ثابت vscp = VN_AS(funcp->user1p(), VarScope);
            File_سطر* ثابت flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_درخت* ثابت senTreep = funcToUsedTriggers.second;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ثابت auto emplaceAt
                    = [flp, &trigps, &usedTriggers](Ast_گره_عبارت* ثابت exprp, ثابت size_t pos) {
                          ثابت size_t targetSize
                              = vlstd::roundUpToMultipleOf<TriggerKit::WORD_SIZE>(pos + 1);
                          اگر (trigps.capacity() < targetSize) trigps.reserve(targetSize * 2);
                          وقتی (trigps.size() < targetSize) {
                              trigps.افزودن_back(جدید AstConst{flp, AstConst::BitFalse{}});
                          }
                          trigps[pos]->delete_درخت();
                          trigps[pos] = exprp;
                          usedTriggers.درج(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                برای (ثابت AstSenItem* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), SenItem)) {
                    ثابت size_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_عبارت_Builder.build(itemp).first, idx);
                    auto iter = m_sen_عبارت_To_Sen_Item.find(*get_Sen_Hash_گره(itemp));
                    اگر (iter != m_sen_عبارت_To_Sen_Item.end()) {
                        برای (AstSenItem* ثابت additionalItemp : iter->second) {
                            ثابت size_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_عبارت_Builder.build(additionalItemp).first, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ثابت Sen_عبارت_Builder::Results results
                    = m_sen_عبارت_Builder.getResultsAndClearUpdates();
                برای (Ast_گره_Stmt* ثابت stmtsp : results.m_inits) funcp->addStmtsp(stmtsp);
                برای (Ast_گره_Stmt* ثابت stmtsp : results.m_preUpdates) funcp->addStmtsp(stmtsp);
                funcp->addStmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                برای (Ast_گره_Stmt* ثابت stmtsp : results.m_postUpdates) funcp->addStmtsp(stmtsp);
            }

            ثابت std::map<size_t, std::map<size_t, std::vector<Ast_گره_عبارت*>>>
                usedTrigsToUsingTrees = getUsedTriggersToTrees(usedTriggers);
            usedTriggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            ثابت auto getIdx = [flp](AstVarScope* ثابت scocep, VAccess access, size_t idx) {
                بازگشت جدید AstArraySel{flp, جدید AstVarRef{flp, scocep, access},
                                       جدید AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* ثابت argpVscp = جدید AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->addVarsp(argpVscp);

            // Mark as ready triggered schedulers
            برای (ثابت auto& triggersToTrees : usedTrigsToUsingTrees) {
                ثابت size_t word = triggersToTrees.first;

                برای (ثابت auto& bitsToTrees : triggersToTrees.second) {
                    ثابت size_t bit = bitsToTrees.first;
                    ثابت auto& schedulers = bitsToTrees.second;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* ثابت maskConstp = جدید AstConst{flp, AstConst::Unsized64{}, bit};
                    AstAnd* ثابت condp
                        = جدید AstAnd{flp, getIdx(vscp, VAccess::READ, word), maskConstp};
                    AstIf* ثابت ifp = جدید AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    برای (Ast_گره_عبارت* ثابت schedp : schedulers) {
                        AstCMethodHard* ثابت callp = جدید AstCMethodHard{
                            flp, schedp->clone_درخت(نادرست), VCMethod::SCHED_READY};
                        callp->dtypeSetVoid();
                        callp->addPinsp(جدید AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->addThensp(callp->makeStmt());
                    }
                    funcp->addStmtsp(ifp);
                }
            }

            AstVarScope* ثابت vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            برای (ثابت auto& triggersToTrees : usedTrigsToUsingTrees) {
                ثابت size_t word = triggersToTrees.first;
                funcp->addStmtsp(جدید AstAssign{flp, getIdx(vscAccp, VAccess::WRITE, word),
                                               جدید AstOr{flp, getIdx(vscAccp, VAccess::READ, word),
                                                         getIdx(vscp, VAccess::READ, word)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = پیشفرض;
};

تهی beforeTrigVisitor(AstNetlist* netlistp, Sen_عبارت_Builder& sen_عبارت_Builder,
                       ثابت TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_عبارت_Builder, trigKit};
}

}  // namespace V3Sched
