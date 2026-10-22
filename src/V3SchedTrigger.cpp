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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3EmitCBase.h"
#அந்தர்பாவ "V3EmitV.h"
#அந்தர்பாவ "V3Order.h"
#அந்தர்பாவ "V3Sched.h"
#அந்தர்பாவ "V3SenExprBuilder.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ V3Sched {

நாமாகாஶ {

using util::newArgument;

AstVarScope* newLocal(AstCFunc* funcp, Ast_முனை_DType* dtypep, மாறிலி std::string& name) {
    File_வரி* மாறிலி flp = funcp->fileline();
    AstScope* மாறிலி scopep = funcp->scopep();
    AstVar* மாறிலி varp = புதிய AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(உண்மை);
    varp->noReset(உண்மை);
    funcp->addVarsp(varp);
    AstVarScope* மாறிலி vscp = புதிய AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    திரும்பு vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() மாறிலி {
    UASSERT(m_n_Pre_சொற்கள், "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* மாறிலி netlistp = v3Global.rootp();
    File_வரி* மாறிலி flp = netlistp->உச்சி_Scopep()->fileline();
    Ast_முனை_DType* மாறிலி u32DTypep = netlistp->findUInt32DType();
    Ast_முனை_DType* மாறிலி strDtypep = netlistp->findStringDType();

    // Dumping function always slow
    மாறிலி std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* மாறிலி funcp = util::makeSubFunction(netlistp, name, உண்மை);
    funcp->isStatic(உண்மை);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* மாறிலி eVscp = newArgument(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* மாறிலி tVscp = newArgument(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    மாறிலி auto rd = [flp](AstVarScope* vp) { திரும்பு புதிய AstVarRef{flp, vp, VAccess::READ}; };
    மாறிலி auto wr = [flp](AstVarScope* vp) { திரும்பு புதிய AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* மாறிலி vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* மாறிலி iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->addStmtsp(util::setVar(iVscp, 0));
        // Add loop
        AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
        funcp->addStmtsp(loopp);
        // Loop body
        Ast_முனை_கோவை* மாறிலி lhsp = புதிய AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_முனை_கோவை* மாறிலி rhsp = புதிய AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_முனை_கோவை* மாறிலி limp = புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{}, 32, m_n_Vec_சொற்கள்};
        loopp->addStmtsp(புதிய AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(iVscp));
        loopp->addStmtsp(புதிய AstLoopTest{flp, loopp, புதிய AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* மாறிலி callp = புதிய AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(vVscp));
        callp->addArgsp(rd(tVscp));
        funcp->addStmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* மாறிலி pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* மாறிலி jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->addStmtsp(util::setVar(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_முனை_கோவை* மாறிலி lhsp = புதிய AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_முனை_கோவை* மாறிலி rhsp = புதிய AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_முனை_கோவை* மாறிலி limp = புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{}, 32, m_n_Pre_சொற்கள்};
            loopp->addStmtsp(புதிய AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(புதிய AstLoopTest{flp, loopp, புதிய AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_முனை_கோவை* மாறிலி lhsp = புதிய AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_முனை_கோவை* மாறிலி rhsp = புதிய AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
            Ast_முனை_கோவை* மாறிலி limp = புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{}, 32, m_n_Vec_சொற்கள்};
            loopp->addStmtsp(புதிய AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(புதிய AstLoopTest{flp, loopp, புதிய AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* மாறிலி callp = புதிய AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(pVscp));
        callp->addArgsp(
            புதிய AstConcatN{flp, rd(tVscp), புதிய AstConst{flp, AstConst::String{}, " pre"}});
        funcp->addStmtsp(callp->makeStmt());
    }

    திரும்பு funcp;
}

AstCFunc* TriggerKit::createAnySetFunc(AstUnpackArrayDType* மாறிலி dtypep) மாறிலி {
    AstNetlist* மாறிலி netlistp = v3Global.rootp();
    File_வரி* மாறிலி flp = netlistp->உச்சி_Scopep()->fileline();
    Ast_முனை_DType* மாறிலி u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* மாறிலி funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(உண்மை);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* மாறிலி iVscp = newArgument(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* மாறிலி nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    மாறிலி auto rd = [flp](AstVarScope* vp) { திரும்பு புதிய AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);
    funcp->addStmtsp(புதிய AstCReturn{flp, புதிய AstConst{flp, AstConst::BitFalse{}}});

    // Loop body
    மாறிலி uint32_t n_சொற்கள் = dtypep->elementsConst();
    Ast_முனை_கோவை* மாறிலி condp = புதிய AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_முனை_Stmt* மாறிலி thenp = புதிய AstCReturn{flp, புதிய AstConst{flp, AstConst::BitTrue{}}};
    Ast_முனை_கோவை* மாறிலி limp = புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{}, 32, n_சொற்கள்};
    loopp->addStmtsp(புதிய AstIf{flp, condp, thenp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(புதிய AstLoopTest{flp, loopp, புதிய AstLt{flp, rd(nVscp), limp}});

    // Done
    திரும்பு funcp;
}
AstCFunc* TriggerKit::createClearFunc() மாறிலி {
    AstNetlist* மாறிலி netlistp = v3Global.rootp();
    File_வரி* மாறிலி flp = netlistp->உச்சி_Scopep()->fileline();
    Ast_முனை_DType* மாறிலி u32DTypep = netlistp->findUInt32DType();

    // Create function
    AstCFunc* மாறிலி funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(உண்மை);

    // Add arguments
    AstVarScope* மாறிலி oVscp = newArgument(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* மாறிலி nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    மாறிலி auto rd = [flp](AstVarScope* vp) { திரும்பு புதிய AstVarRef{flp, vp, VAccess::READ}; };
    மாறிலி auto wr = [flp](AstVarScope* vp) { திரும்பு புதிய AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_முனை_கோவை* மாறிலி lhsp = புதிய AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_முனை_கோவை* மாறிலி rhsp = புதிய AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
    Ast_முனை_கோவை* மாறிலி limp = புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{}, 32, m_n_Vec_சொற்கள்};
    loopp->addStmtsp(புதிய AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(புதிய AstLoopTest{flp, loopp, புதிய AstLt{flp, rd(nVscp), limp}});

    // Done
    திரும்பு funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* மாறிலி oDtypep,
                                       AstUnpackArrayDType* மாறிலி iDtypep) மாறிலி {
    AstNetlist* மாறிலி netlistp = v3Global.rootp();
    File_வரி* மாறிலி flp = netlistp->உச்சி_Scopep()->fileline();
    Ast_முனை_DType* மாறிலி u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* மாறிலி funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(உண்மை);

    // Add arguments
    AstVarScope* மாறிலி oVscp = newArgument(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* மாறிலி iVscp = newArgument(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* மாறிலி nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    மாறிலி auto rd = [flp](AstVarScope* vp) { திரும்பு புதிய AstVarRef{flp, vp, VAccess::READ}; };
    மாறிலி auto wr = [flp](AstVarScope* vp) { திரும்பு புதிய AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_முனை_கோவை* மாறிலி lhsp = புதிய AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_முனை_கோவை* மாறிலி oWordp = புதிய AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_முனை_கோவை* மாறிலி iWordp = புதிய AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_முனை_கோவை* மாறிலி rhsp = புதிய AstOr{flp, oWordp, iWordp};
    AstConst* மாறிலி outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* மாறிலி உள்ளீடு_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_முனை_கோவை* மாறிலி limp = outputRangeLeftp->இலக்கம்().toSInt() < உள்ளீடு_Range_Leftp->இலக்கம்().toSInt()
                                  ? outputRangeLeftp->clone_மரம்_Pure(பொய்)
                                  : உள்ளீடு_Range_Leftp->clone_மரம்_Pure(பொய்);
    loopp->addStmtsp(புதிய AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(புதிய AstLoopTest{flp, loopp, புதிய AstLte{flp, rd(nVscp), limp}});

    // Done
    திரும்பு funcp;
}

Ast_முனை_கோவை* TriggerKit::newAnySetCall(AstVarScope* மாறிலி vscp) மாறிலி {
    File_வரி* மாறிலி flp = v3Global.rootp()->உச்சி_Scopep()->fileline();
    எனில் (!m_n_Vec_சொற்கள்) திரும்பு புதிய AstConst{flp, AstConst::BitFalse{}};

    AstCFunc* funcp = nullptr;
    எனில் (vscp->dtypep() == m_trigVecDTypep) {
        எனில் (!m_anySetVecp) m_anySetVecp = createAnySetFunc(m_trigVecDTypep);
        funcp = m_anySetVecp;
    } இல்லையெனில் எனில் (vscp->dtypep() == m_trigExtDTypep) {
        எனில் (!m_anySetExtp) m_anySetExtp = createAnySetFunc(m_trigExtDTypep);
        funcp = m_anySetExtp;
    } இல்லையெனில் {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* மாறிலி callp = புதிய AstCCall{flp, funcp};
    callp->addArgsp(புதிய AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetBit();
    திரும்பு callp;
}
Ast_முனை_Stmt* TriggerKit::newClearCall(AstVarScope* மாறிலி vscp) மாறிலி {
    எனில் (!m_n_Vec_சொற்கள்) திரும்பு nullptr;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    எனில் (!m_clearp) m_clearp = createClearFunc();
    File_வரி* மாறிலி flp = v3Global.rootp()->உச்சி_Scopep()->fileline();
    AstCCall* மாறிலி callp = புதிய AstCCall{flp, m_clearp};
    callp->addArgsp(புதிய AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetVoid();
    திரும்பு callp->makeStmt();
}
Ast_முனை_Stmt* TriggerKit::newOrIntoCall(AstVarScope* மாறிலி oVscp, AstVarScope* மாறிலி iVscp) மாறிலி {
    எனில் (!m_n_Vec_சொற்கள்) திரும்பு nullptr;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    மாறிலி size_t mask
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[mask];
    எனில் (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_வரி* மாறிலி flp = v3Global.rootp()->உச்சி_Scopep()->fileline();
    AstCCall* மாறிலி callp = புதிய AstCCall{flp, funcp};
    callp->addArgsp(புதிய AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->addArgsp(புதிய AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtypeSetVoid();
    திரும்பு callp->makeStmt();
}

Ast_முனை_Stmt* TriggerKit::newCompBaseCall() மாறிலி {
    எனில் (!m_n_Vec_சொற்கள்) திரும்பு nullptr;
    File_வரி* மாறிலி flp = v3Global.rootp()->உச்சி_Scopep()->fileline();
    AstCCall* மாறிலி callp = புதிய AstCCall{flp, m_compVecp};
    callp->dtypeSetVoid();
    திரும்பு callp->makeStmt();
}

Ast_முனை_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) மாறிலி {
    எனில் (!m_n_Pre_சொற்கள்) திரும்பு nullptr;
    File_வரி* மாறிலி flp = v3Global.rootp()->உச்சி_Scopep()->fileline();
    AstCCall* மாறிலி callp = புதிய AstCCall{flp, m_compExtp};
    callp->addArgsp(புதிய AstVarRef{flp, vscp, VAccess::READ});
    callp->dtypeSetVoid();
    திரும்பு callp->makeStmt();
}

Ast_முனை_Stmt* TriggerKit::newDumpCall(AstVarScope* மாறிலி vscp, மாறிலி std::string& tag,
                                     தருக்க debugOnly) மாறிலி {
    எனில் (!m_n_Vec_சொற்கள்) திரும்பு nullptr;
    AstCFunc* funcp = nullptr;
    எனில் (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } இல்லையெனில் எனில் (vscp->dtypep() == m_trigExtDTypep) {
        எனில் (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } இல்லையெனில் {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_வரி* மாறிலி flp = v3Global.rootp()->உச்சி_Scopep()->fileline();
    AstCCall* மாறிலி callp = புதிய AstCCall{flp, funcp};
    callp->addArgsp(புதிய AstVarRef{flp, vscp, VAccess::READ});
    callp->addArgsp(புதிய AstConst{flp, AstConst::String{}, tag});
    callp->dtypeSetVoid();
    AstCStmt* மாறிலி cstmtp = புதிய AstCStmt{flp};
    cstmtp->add("#ifdef VL_DEBUG\n");
    எனில் (debugOnly) {
        cstmtp->add("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->add(callp->makeStmt());
        cstmtp->add("}\n");
    } இல்லையெனில் {
        cstmtp->add(callp->makeStmt());
    }
    cstmtp->add("#endif");
    திரும்பு cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(மாறிலி std::string& name) மாறிலி {
    எனில் (!m_n_Vec_சொற்கள்) திரும்பு nullptr;
    திரும்பு v3Global.rootp()->உச்சி_Scopep()->createTemp("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extraTriggerIndex(uint32_t index) மாறிலி {
    UASSERT(index <= m_n_Extra_சொற்கள் * WORD_SIZE, "Invalid external trigger index");
    திரும்பு index + m_n_Sense_சொற்கள் * WORD_SIZE;
}

std::set<uint32_t> TriggerKit::triggerIndices(மாறிலி Ast_Sen_மரம்* senTreep, தருக்க pre) மாறிலி {
    std::set<uint32_t> indices;
    க்கு (மாறிலி AstSenItem* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), SenItem)) {
        uint32_t index = m_senItem2TrigIdx.at(*sip);
        எனில் (pre) {
            UASSERT_OBJ(index < m_n_Pre_சொற்கள் * WORD_SIZE, sip, "Not a 'pre' trigger");
            index += m_n_Vec_சொற்கள் * WORD_SIZE;
        }
        indices.செருகு(index);
    }
    திரும்பு indices;
}

Ast_முனை_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) மாறிலி {
    index = extraTriggerIndex(index);
    மாறிலி uint32_t wordIndex = index / WORD_SIZE;
    மாறிலி uint32_t bitIndex = index % WORD_SIZE;
    File_வரி* மாறிலி flp = vscp->fileline();
    AstVarRef* மாறிலி refp = புதிய AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_முனை_கோவை* மாறிலி wordp = புதிய AstArraySel{flp, refp, static_cast<முழு>(wordIndex)};
    Ast_முனை_கோவை* மாறிலி trigLhsp = புதிய AstSel{flp, wordp, static_cast<முழு>(bitIndex), 1};
    Ast_முனை_கோவை* மாறிலி trigRhsp = புதிய AstVarRef{flp, vscp, VAccess::READ};
    திரும்பு புதிய AstAssign{flp, trigLhsp, trigRhsp};
}

வெறுமை TriggerKit::addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) மாறிலி {
    File_வரி* மாறிலி flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_முனை* மாறிலி setp = newExtraTriggerAssignment(vscp, index);
    setp->addNext(புதிய AstAssign{flp, புதிய AstVarRef{flp, vscp, VAccess::WRITE},
                                புதிய AstConst{flp, AstConst::BitFalse{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
வெறுமை TriggerKit::add_மதிப்பு_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t index) மாறிலி {
    index += m_n_Sense_சொற்கள் * WORD_SIZE;
    மாறிலி uint32_t wordIndex = index / WORD_SIZE;
    மாறிலி uint32_t bitIndex = index % WORD_SIZE;

    Ast_உச்சி_Scope* மாறிலி உச்சி_Scopep = netlistp->உச்சி_Scopep();
    File_வரி* மாறிலி flp = netlistp->fileline();
    Ast_முனை_DType* மாறிலி dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    எனில் (மாறிலி AstBasicDType* மாறிலி bdtypep = VN_CAST(dtypep, BasicDType)) {
        எனில் (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            திரும்பு;
        }
    } இல்லையெனில் எனில் (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, முனை_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        திரும்பு;
    }

    மாறிலி auto rdInst = [flp](AstVarScope* vp) { திரும்பு புதிய AstVarRef{flp, vp, VAccess::READ}; };
    மாறிலி auto wrPrev = [flp](AstVarScope* vp) { திரும்பு புதிய AstVarRef{flp, vp, VAccess::WRITE}; };
    மாறிலி auto rdPrev = [flp](AstVarScope* vp) { திரும்பு புதிய AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    மாறிலி std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* மாறிலி prevVscp = உச்சி_Scopep->createTemp(prevName, instVscp->dtypep());

    // Initialize prev = inst
    எனில் (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* மாறிலி cmhp = புதிய AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        initFuncp->addStmtsp(cmhp->makeStmt());
    } இல்லையெனில் {
        initFuncp->addStmtsp(புதிய AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_முனை_கோவை* neqp;
    எனில் (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* மாறிலி cmhp
            = புதிய AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtypeSetBit();
        neqp = cmhp;
    } இல்லையெனில் {
        neqp = புதிய AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_முனை* updp;
    எனில் (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* மாறிலி cmhp = புதிய AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        updp = cmhp->makeStmt();
    } இல்லையெனில் {
        updp = புதிய AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* மாறிலி refp = புதிய AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_முனை_கோவை* மாறிலி wordp = புதிய AstArraySel{flp, refp, static_cast<முழு>(wordIndex)};
    Ast_முனை_கோவை* மாறிலி trigLhsp = புதிய AstSel{flp, wordp, static_cast<முழு>(bitIndex), 1};
    Ast_முனை* மாறிலி setp = புதிய AstAssign{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->addNext(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(மாறிலி std::string& name, தருக்க slow, uint32_t n_Sense_சொற்கள்,
                       uint32_t n_Extra_சொற்கள், uint32_t n_Pre_சொற்கள்,
                       std::unordered_map<VNRef<மாறிலி AstSenItem>, size_t> senItem2TrigIdx,
                       மாறிலி std::vector<மாறிலி AstSenItem*>& senItemps, தருக்க useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_சொற்கள்{n_Sense_சொற்கள்}
    , m_n_Extra_சொற்கள்{n_Extra_சொற்கள்}
    , m_n_Pre_சொற்கள்{n_Pre_சொற்கள்}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trigIdx2SenItem{senItemps} {
    // If no triggers, we don't need to generate anything
    எனில் (!m_n_Vec_சொற்கள்) திரும்பு;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trigIdx2SenItem.resize((m_n_Vec_சொற்கள் + m_n_Pre_சொற்கள்) * WORD_SIZE);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_சொற்கள் * WORD_SIZE,
              m_trigIdx2SenItem.begin() + m_n_Vec_சொற்கள் * WORD_SIZE);
    AstNetlist* மாறிலி netlistp = v3Global.rootp();
    AstScope* மாறிலி scopep = netlistp->உச்சி_Scopep()->scopep();
    File_வரி* மாறிலி flp = scopep->fileline();
    // Data type of a single trigger word
    m_wordDTypep = netlistp->findBitDType(WORD_SIZE, WORD_SIZE, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* மாறிலி rp = புதிய AstRange{flp, static_cast<முழு>(m_n_Vec_சொற்கள் - 1), 0};
    m_trigVecDTypep = புதிய AstUnpackArrayDType{flp, m_wordDTypep, rp};
    netlistp->typeTablep()->addTypesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    எனில் (m_n_Pre_சொற்கள்) {
        AstRange* மாறிலி ep = புதிய AstRange{flp, static_cast<முழு>(m_n_Vec_சொற்கள் + m_n_Pre_சொற்கள் - 1), 0};
        m_trigExtDTypep = புதிய AstUnpackArrayDType{flp, m_wordDTypep, ep};
        netlistp->typeTablep()->addTypesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } இல்லையெனில் {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->உச்சி_Scopep()->createTemp("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->isInternal(உண்மை);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, உண்மை);
    m_dumpp->isStatic(உண்மை);
    m_dumpp->ifdef("VL_DEBUG");
    எனில் (useAcc) {
        m_vscAccp
            = netlistp->உச்சி_Scopep()->createTemp("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->isInternal(உண்மை);
    }
}

AstAssign* TriggerKit::createSenTrigVecAssignment(AstVarScope* மாறிலி target,
                                                  std::vector<Ast_முனை_கோவை*>& trigps) {
    File_வரி* மாறிலி flp = target->fileline();
    AstAssign* trigStmtsp = nullptr;
    // Assign sense triggers vector one word at a time
    க்கு (size_t i = 0; i < trigps.size(); i += WORD_SIZE) {
        // Concatenate all bits in this trigger word using a balanced
        க்கு (uint32_t level = 0; level < WORD_SIZE_LOG2; ++level) {
            மாறிலி uint32_t stride = 1 << level;
            க்கு (uint32_t j = 0; j < WORD_SIZE; j += 2 * stride) {
                trigps[i + j] = புதிய AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = nullptr;
            }
        }

        // Set the whole word in the trigger vector
        மாறிலி முழு wordIndex = static_cast<முழு>(i / WORD_SIZE);
        AstArraySel* மாறிலி aselp
            = புதிய AstArraySel{flp, புதிய AstVarRef{flp, target, VAccess::WRITE}, wordIndex};
        trigStmtsp = Ast_முனை::addNext(trigStmtsp, புதிய AstAssign{flp, aselp, trigps[i]});
    }
    திரும்பு trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* மாறிலி initFuncp,  //
                              Sen_கோவை_Builder& sen_கோவை_Builder,  //
                              மாறிலி std::vector<மாறிலி Ast_Sen_மரம்*>& preTreeps,  //
                              மாறிலி std::vector<மாறிலி Ast_Sen_மரம்*>& senTreeps,  //
                              மாறிலி string& name,  //
                              மாறிலி ExtraTriggers& extraTriggers,  //
                              தருக்க slow,  //
                              தருக்க useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::vector<மாறிலி AstSenItem*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<மாறிலி AstSenItem>, size_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    க்கு (மாறிலி Ast_Sen_மரம்* மாறிலி senTreep : preTreeps) {
        க்கு (மாறிலி AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            மாறிலி auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            எனில் (pair.second) senItemps.தள்ளு_back(itemp);
        }
    }
    மாறிலி uint32_t n_Pre_Sen_பொருட்கள் = senItemps.size();
    V3Stats::addStat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_பொருட்கள்);
    // Number of pre triggers, rounded up to a full word.
    மாறிலி uint32_t nPreTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    மாறிலி uint32_t n_Pre_சொற்கள் = nPreTriggers / WORD_SIZE;

    // Process the rest of the trees
    க்கு (மாறிலி Ast_Sen_மரம்* மாறிலி senTreep : senTreeps) {
        க்கு (மாறிலி AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            மாறிலி auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            எனில் (pair.second) senItemps.தள்ளு_back(itemp);
        }
    }
    மாறிலி uint32_t n_Sen_பொருட்கள் = senItemps.size() - nPreTriggers;
    V3Stats::addStat("Scheduling, '" + name + "' sense triggers", n_Sen_பொருட்கள் + n_Pre_Sen_பொருட்கள்);
    // Number of sense triggers, rounded up to a full word
    மாறிலி uint32_t nSenseTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    மாறிலி uint32_t n_Sense_சொற்கள் = nSenseTriggers / WORD_SIZE;

    // Allocate space for the extra triggers
    V3Stats::addStat("Scheduling, '" + name + "' extra triggers", extraTriggers.size());
    // Number of extra triggers, rounded up to a full word.
    மாறிலி uint32_t nExtraTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(extraTriggers.size());
    மாறிலி uint32_t n_Extra_சொற்கள் = nExtraTriggers / WORD_SIZE;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_சொற்கள், n_Extra_சொற்கள், n_Pre_சொற்கள், senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    எனில் (!kit.m_n_Vec_சொற்கள்) திரும்பு kit;

    File_வரி* மாறிலி flp = netlistp->உச்சி_Scopep()->fileline();

    // Creates read/write reference
    மாறிலி auto rd = [flp](AstVarScope* vp) { திரும்பு புதிய AstVarRef{flp, vp, VAccess::READ}; };
    மாறிலி auto wr = [flp](AstVarScope* vp) { திரும்பு புதிய AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* மாறிலி dumpTrgp
        = newArgument(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* மாறிலி dumpTagp
        = newArgument(kit.m_dumpp, netlistp->findStringDType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* மாறிலி ifp = புதிய AstIf{flp, புதிய AstLogNot{flp, kit.newAnySetCall(dumpTrgp)}};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* மாறிலி cstmtp = புதிய AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    மாறிலி auto addDebug = [&](uint32_t index, மாறிலி string& text) {
        மாறிலி முழு wrdIndex = static_cast<முழு>(index / WORD_SIZE);
        மாறிலி முழு bitIndex = static_cast<முழு>(index % WORD_SIZE);
        Ast_முனை_கோவை* மாறிலி aselp = புதிய AstArraySel{flp, rd(dumpTrgp), wrdIndex};
        Ast_முனை_கோவை* மாறிலி condp = புதிய AstSel{flp, aselp, bitIndex, 1};
        AstIf* மாறிலி ifp = புதிய AstIf{flp, condp};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* மாறிலி cstmtp = புதிய AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"' region trigger index " + std::to_string(index) + " is active: " + text
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::vector<Ast_முனை_கோவை*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_முனை_Stmt* initialTrigsp = nullptr;
    க்கு (size_t i = 0; i < senItemps.size(); ++i) {
        மாறிலி AstSenItem* மாறிலி senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        எனில் (!senItemp) {
            trigps.emplace_back(புதிய AstConst{flp, AstConst::BitFalse{}});
            தொடர்;
        }

        // Create the trigger computation expression
        மாறிலி auto& pair = sen_கோவை_Builder.build(senItemp);
        trigps.emplace_back(pair.first);

        // Add initialization time trigger
        எனில் (pair.second || v3Global.opt.xInitialEdge()) {
            மாறிலி முழு wrdIndex = static_cast<முழு>(i / WORD_SIZE);
            மாறிலி முழு bitIndex = static_cast<முழு>(i % WORD_SIZE);
            Ast_முனை_கோவை* மாறிலி wordp = புதிய AstArraySel{flp, wr(kit.m_vscp), wrdIndex};
            Ast_முனை_கோவை* மாறிலி lhsp = புதிய AstSel{flp, wordp, bitIndex, 1};
            Ast_முனை_கோவை* மாறிலி rhsp = புதிய AstConst{flp, AstConst::BitTrue{}};
            எனில் (useAcc) {
                initFuncp->addStmtsp(புதிய AstAssign{flp, lhsp, rhsp});
            } இல்லையெனில் {
                initialTrigsp = Ast_முனை::addNext(initialTrigsp, புதிய AstAssign{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_மரம்(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        addDebug(i, desc);
    }
    UASSERT(trigps.size() == nSenseTriggers, "Inconsistent number of trigger expressions");

    AstAssign* மாறிலி trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    க்கு (unsigned i = 0; i < extraTriggers.size(); ++i) {
        addDebug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    மாறிலி Sen_கோவை_Builder::Results senResults = sen_கோவை_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    க்கு (Ast_முனை_Stmt* மாறிலி nodep : senResults.m_inits) initFuncp->addStmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_உச்சி_Scope* மாறிலி உச்சி_Scopep = netlistp->உச்சி_Scopep();
    {
        AstCFunc* மாறிலி fp = kit.m_compVecp;
        // Trigger computation
        க்கு (Ast_முனை_Stmt* மாறிலி nodep : senResults.m_preUpdates) fp->addStmtsp(nodep);
        fp->addStmtsp(trigStmtsp);
        க்கு (Ast_முனை_Stmt* மாறிலி nodep : senResults.m_postUpdates) fp->addStmtsp(nodep);
        // Add the initialization time triggers
        எனில் (initialTrigsp) {
            AstVarScope* மாறிலி initVscp = உச்சி_Scopep->createTemp("__V" + name + "DidInit", 1);
            AstIf* மாறிலி ifp = புதிய AstIf{flp, புதிய AstNot{flp, rd(initVscp)}};
            fp->addStmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->addThensp(util::setVar(initVscp, 1));
            ifp->addThensp(initialTrigsp);
        }
        util::splitCheck(fp);
    };
    // If there are 'pre' triggers, compute them
    எனில் (kit.m_n_Pre_சொற்கள்) {
        AstCFunc* மாறிலி fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* மாறிலி latchedp
            = newArgument(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* மாறிலி nVscp = உச்சி_Scopep->createTemp("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(உண்மை);
        // Add a loop to compute the pre words
        AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
        fp->addStmtsp(util::setVar(nVscp, 0));
        fp->addStmtsp(loopp);
        // Loop body
        Ast_முனை_கோவை* மாறிலி offsetp = புதிய AstConst{flp, kit.m_n_Vec_சொற்கள்};
        Ast_முனை_கோவை* மாறிலி lIdxp = புதிய AstAdd{flp, rd(nVscp), offsetp};
        Ast_முனை_கோவை* மாறிலி lhsp = புதிய AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_முனை_கோவை* மாறிலி aWordp = புதிய AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_முனை_கோவை* மாறிலி bWordp = புதிய AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_முனை_கோவை* மாறிலி rhsp = புதிய AstAnd{flp, aWordp, புதிய AstNot{flp, bWordp}};
        Ast_முனை_கோவை* மாறிலி limp = புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{}, 32, n_Pre_சொற்கள்};
        loopp->addStmtsp(புதிய AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(nVscp));
        loopp->addStmtsp(புதிய AstLoopTest{flp, loopp, புதிய AstLt{flp, rd(nVscp), limp}});
        util::splitCheck(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    எனில் (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlinkFrBackWithNext()->delete_மரம்();
    // Done with the trigger dump function, split as might be large
    util::splitCheck(kit.m_dumpp);

    திரும்பு kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
வகுப்பு AwaitBeforeTrigVisitor final : பொது VNVisitor {
    மாறிலி VNUser1InUse m_user1InUse;

    /**
     * AstCAwait::user1()       ->  bool.           True if node has been visited
     * AstSenTree::user1p()     ->  AstCFunc*.      Function that has to be called before awaiting
     *                                              for CAwait pointing to this SenTree
     * AstCFunc::user1p()       ->  AstVarScope*    Function's local temporary extended trigger
     *                                              vector variable scope
     */

    // Netlist - needed for using util::makeSubFunction()
    AstNetlist* மாறிலி m_netlistp;
    // Trigger kit - for accessing trigger vectors and mapping senItems to thier indexes
    மாறிலி TriggerKit& m_trigKit;
    // Expression builder - for building expressions from SenItems
    Sen_கோவை_Builder& m_sen_கோவை_Builder;
    // Generator of unique names for before-trigger function
    V3UniqueNames m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::vector<std::pair<AstCFunc*, Ast_Sen_மரம்*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::vector<std::pair<Ast_Sen_மரம்*, Ast_முனை_கோவை*>> m_sen_மரம்_To_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_முனை>, std::vector<AstSenItem*>> m_sen_கோவை_To_Sen_Item;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    நிலையான Ast_முனை* get_Sen_Hash_முனை(மாறிலி AstSenItem* மாறிலி nodep) {
        எனில் (AstVarRef* மாறிலி varRefp = VN_CAST(nodep->sensp(), VarRef)) திரும்பு varRefp;
        திரும்பு nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    வெறுமை fill_Sen_கோவை_To_Sen_Item() {
        க்கு (auto sen_மரம்_Sched : m_sen_மரம்_To_Sched) {
            Ast_Sen_மரம்* மாறிலி senTreep = sen_மரம்_Sched.first;

            க்கு (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                மாறிலி VEdgeType edge = senItemp->edgeType();
                எனில் (edge.anEdge() || edge == VEdgeType::ET_CHANGED
                    || edge == VEdgeType::ET_HYBRID) {
                    m_sen_கோவை_To_Sen_Item[*get_Sen_Hash_முனை(senItemp)].தள்ளு_back(senItemp);
                }
            }
        }

        std::vector<VNRef<Ast_முனை>> toRemove;
        க்கு (மாறிலி auto& sen_கோவை_To_Sen_மரம் : m_sen_கோவை_To_Sen_Item) {
            std::vector<AstSenItem*> senItemps = sen_கோவை_To_Sen_மரம்.second;
            toRemove.தள்ளு_back(sen_கோவை_To_Sen_மரம்.first);
            க்கு (size_t i = 1; i < senItemps.size(); ++i) {
                எனில் (senItemps[i]->edgeType() != senItemps[i - 1]->edgeType()) {
                    toRemove.எடு_back();
                    நிறுத்து;
                }
            }
        }
        க்கு (மாறிலி VNRef<Ast_முனை> it : toRemove) m_sen_கோவை_To_Sen_Item.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<size_t, std::map<size_t, std::vector<Ast_முனை_கோவை*>>>
    getUsedTriggersToTrees(மாறிலி std::set<size_t>& usedTriggers) {
        std::map<size_t, std::map<size_t, std::vector<Ast_முனை_கோவை*>>> usedTrigsToUsingTrees;
        க்கு (auto sen_மரம்_Sched : m_sen_மரம்_To_Sched) {
            மாறிலி Ast_Sen_மரம்* மாறிலி senTreep = sen_மரம்_Sched.first;
            Ast_முனை_கோவை* மாறிலி shedp = sen_மரம்_Sched.second;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            க்கு (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                மாறிலி size_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                எனில் (usedTriggers.find(idx) != usedTriggers.end()) {
                    usedTrigsToUsingTrees[idx / TriggerKit::WORD_SIZE]
                                         [size_t{1} << (idx % TriggerKit::WORD_SIZE)]
                                             .தள்ளு_back(shedp);
                }
            }
        }
        எனில் (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            க்கு (மாறிலி auto& triggersToTrees : usedTrigsToUsingTrees) {
                க்கு (மாறிலி auto& bitsToTrees : triggersToTrees.second) {
                    மாறிலி std::set<மாறிலி Ast_முனை_கோவை*> exprps{bitsToTrees.second.begin(),
                                                              bitsToTrees.second.end()};
                    UASSERT(bitsToTrees.second.size() == exprps.size(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        திரும்பு usedTrigsToUsingTrees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_மரம்* மாறிலி senTreep) {
        File_வரி* மாறிலி flp = senTreep->fileline();
        எனில் (!senTreep->user1p()) {
            AstCFunc* மாறிலி funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), பொய்);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* மாறிலி tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(உண்மை);

            AstVar* மாறிலி argp = புதிய AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->findBasicDType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(உண்மை);
            argp->direction(VDirection::உள்ளீடு);
            funcp->addArgsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_back(funcp, senTreep);
        }
        AstCCall* மாறிலி callp = புதிய AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtypeSetVoid();
        திரும்பு callp;
    }

    வெறுமை visit(AstCAwait* மாறிலி nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        எனில் (மாறிலி AstCMethodHard* மாறிலி cMethodHardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            எனில் (cMethodHardp->method() == VCMethod::SCHED_TRIGGER) {
                AstCCall* மாறிலி beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_முனை* மாறிலி pinp = cMethodHardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, cMethodHardp, "No event description");
                beforeTrigp->addArgsp(VN_AS(pinp, முனை_கோவை)->clone_மரம்(பொய்));

                // Call the before-trigger function before the CAwait
                nodep->addHereThisAsNext(beforeTrigp->makeStmt());
                m_sen_மரம்_To_Sched.emplace_back(nodep->sentreep(), cMethodHardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    வெறுமை visit(Ast_முனை* மாறிலி nodep) override { iterateChildren(nodep); }

பொது:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_கோவை_Builder& sen_கோவை_Builder,
                           மாறிலி TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_கோவை_Builder{sen_கோவை_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_கோவை_To_Sen_Item();

        std::vector<Ast_முனை_கோவை*> trigps;
        std::set<size_t> usedTriggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        க்கு (மாறிலி auto& funcToUsedTriggers : m_generatedFuncs) {
            AstCFunc* மாறிலி funcp = funcToUsedTriggers.first;
            AstVarScope* மாறிலி vscp = VN_AS(funcp->user1p(), VarScope);
            File_வரி* மாறிலி flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_மரம்* மாறிலி senTreep = funcToUsedTriggers.second;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                மாறிலி auto emplaceAt
                    = [flp, &trigps, &usedTriggers](Ast_முனை_கோவை* மாறிலி exprp, மாறிலி size_t pos) {
                          மாறிலி size_t targetSize
                              = vlstd::roundUpToMultipleOf<TriggerKit::WORD_SIZE>(pos + 1);
                          எனில் (trigps.capacity() < targetSize) trigps.reserve(targetSize * 2);
                          போது (trigps.size() < targetSize) {
                              trigps.தள்ளு_back(புதிய AstConst{flp, AstConst::BitFalse{}});
                          }
                          trigps[pos]->delete_மரம்();
                          trigps[pos] = exprp;
                          usedTriggers.செருகு(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                க்கு (மாறிலி AstSenItem* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), SenItem)) {
                    மாறிலி size_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_கோவை_Builder.build(itemp).first, idx);
                    auto iter = m_sen_கோவை_To_Sen_Item.find(*get_Sen_Hash_முனை(itemp));
                    எனில் (iter != m_sen_கோவை_To_Sen_Item.end()) {
                        க்கு (AstSenItem* மாறிலி additionalItemp : iter->second) {
                            மாறிலி size_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_கோவை_Builder.build(additionalItemp).first, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                மாறிலி Sen_கோவை_Builder::Results results
                    = m_sen_கோவை_Builder.getResultsAndClearUpdates();
                க்கு (Ast_முனை_Stmt* மாறிலி stmtsp : results.m_inits) funcp->addStmtsp(stmtsp);
                க்கு (Ast_முனை_Stmt* மாறிலி stmtsp : results.m_preUpdates) funcp->addStmtsp(stmtsp);
                funcp->addStmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                க்கு (Ast_முனை_Stmt* மாறிலி stmtsp : results.m_postUpdates) funcp->addStmtsp(stmtsp);
            }

            மாறிலி std::map<size_t, std::map<size_t, std::vector<Ast_முனை_கோவை*>>>
                usedTrigsToUsingTrees = getUsedTriggersToTrees(usedTriggers);
            usedTriggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            மாறிலி auto getIdx = [flp](AstVarScope* மாறிலி scocep, VAccess access, size_t idx) {
                திரும்பு புதிய AstArraySel{flp, புதிய AstVarRef{flp, scocep, access},
                                       புதிய AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* மாறிலி argpVscp = புதிய AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->addVarsp(argpVscp);

            // Mark as ready triggered schedulers
            க்கு (மாறிலி auto& triggersToTrees : usedTrigsToUsingTrees) {
                மாறிலி size_t word = triggersToTrees.first;

                க்கு (மாறிலி auto& bitsToTrees : triggersToTrees.second) {
                    மாறிலி size_t bit = bitsToTrees.first;
                    மாறிலி auto& schedulers = bitsToTrees.second;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* மாறிலி maskConstp = புதிய AstConst{flp, AstConst::Unsized64{}, bit};
                    AstAnd* மாறிலி condp
                        = புதிய AstAnd{flp, getIdx(vscp, VAccess::READ, word), maskConstp};
                    AstIf* மாறிலி ifp = புதிய AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    க்கு (Ast_முனை_கோவை* மாறிலி schedp : schedulers) {
                        AstCMethodHard* மாறிலி callp = புதிய AstCMethodHard{
                            flp, schedp->clone_மரம்(பொய்), VCMethod::SCHED_READY};
                        callp->dtypeSetVoid();
                        callp->addPinsp(புதிய AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->addThensp(callp->makeStmt());
                    }
                    funcp->addStmtsp(ifp);
                }
            }

            AstVarScope* மாறிலி vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            க்கு (மாறிலி auto& triggersToTrees : usedTrigsToUsingTrees) {
                மாறிலி size_t word = triggersToTrees.first;
                funcp->addStmtsp(புதிய AstAssign{flp, getIdx(vscAccp, VAccess::WRITE, word),
                                               புதிய AstOr{flp, getIdx(vscAccp, VAccess::READ, word),
                                                         getIdx(vscp, VAccess::READ, word)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = இயல்பு;
};

வெறுமை beforeTrigVisitor(AstNetlist* netlistp, Sen_கோவை_Builder& sen_கோவை_Builder,
                       மாறிலி TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_கோவை_Builder, trigKit};
}

}  // namespace V3Sched
