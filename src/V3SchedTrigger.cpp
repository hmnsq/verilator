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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3Order.h"
#अन्तर्भाव "V3Sched.h"
#अन्तर्भाव "V3SenExprBuilder.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश V3Sched {

नामाकाश {

using util::newArgument;

AstVarScope* newLocal(AstCFunc* funcp, Ast_ग्रन्थिः_DType* dtypep, ध्रुव std::string& name) {
    File_पङ्क्तिः* ध्रुव flp = funcp->fileline();
    AstScope* ध्रुव scopep = funcp->scopep();
    AstVar* ध्रुव varp = नूतन AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(सच्च);
    varp->noReset(सच्च);
    funcp->addVarsp(varp);
    AstVarScope* ध्रुव vscp = नूतन AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    पटिनिवत्त vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() ध्रुव {
    UASSERT(m_n_Pre_शब्दाः, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    File_पङ्क्तिः* ध्रुव flp = netlistp->शिखरम्_Scopep()->fileline();
    Ast_ग्रन्थिः_DType* ध्रुव u32DTypep = netlistp->findUInt32DType();
    Ast_ग्रन्थिः_DType* ध्रुव strDtypep = netlistp->findStringDType();

    // Dumping function always slow
    ध्रुव std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* ध्रुव funcp = util::makeSubFunction(netlistp, name, सच्च);
    funcp->isStatic(सच्च);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* ध्रुव eVscp = newArgument(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* ध्रुव tVscp = newArgument(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    ध्रुव auto rd = [flp](AstVarScope* vp) { पटिनिवत्त नूतन AstVarRef{flp, vp, VAccess::READ}; };
    ध्रुव auto wr = [flp](AstVarScope* vp) { पटिनिवत्त नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* ध्रुव vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* ध्रुव iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->addStmtsp(util::setVar(iVscp, 0));
        // Add loop
        AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
        funcp->addStmtsp(loopp);
        // Loop body
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, m_n_Vec_शब्दाः};
        loopp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(iVscp));
        loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* ध्रुव callp = नूतन AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(vVscp));
        callp->addArgsp(rd(tVscp));
        funcp->addStmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* ध्रुव pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* ध्रुव jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->addStmtsp(util::setVar(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, m_n_Pre_शब्दाः};
            loopp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, m_n_Vec_शब्दाः};
            loopp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* ध्रुव callp = नूतन AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(pVscp));
        callp->addArgsp(
            नूतन AstConcatN{flp, rd(tVscp), नूतन AstConst{flp, AstConst::String{}, " pre"}});
        funcp->addStmtsp(callp->makeStmt());
    }

    पटिनिवत्त funcp;
}

AstCFunc* TriggerKit::createAnySetFunc(AstUnpackArrayDType* ध्रुव dtypep) ध्रुव {
    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    File_पङ्क्तिः* ध्रुव flp = netlistp->शिखरम्_Scopep()->fileline();
    Ast_ग्रन्थिः_DType* ध्रुव u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* ध्रुव funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(सच्च);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* ध्रुव iVscp = newArgument(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ध्रुव nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    ध्रुव auto rd = [flp](AstVarScope* vp) { पटिनिवत्त नूतन AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);
    funcp->addStmtsp(नूतन AstCReturn{flp, नूतन AstConst{flp, AstConst::BitFalse{}}});

    // Loop body
    ध्रुव uint32_t n_शब्दाः = dtypep->elementsConst();
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = नूतन AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ग्रन्थिः_Stmt* ध्रुव thenp = नूतन AstCReturn{flp, नूतन AstConst{flp, AstConst::BitTrue{}}};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, n_शब्दाः};
    loopp->addStmtsp(नूतन AstIf{flp, condp, thenp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(nVscp), limp}});

    // Done
    पटिनिवत्त funcp;
}
AstCFunc* TriggerKit::createClearFunc() ध्रुव {
    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    File_पङ्क्तिः* ध्रुव flp = netlistp->शिखरम्_Scopep()->fileline();
    Ast_ग्रन्थिः_DType* ध्रुव u32DTypep = netlistp->findUInt32DType();

    // Create function
    AstCFunc* ध्रुव funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(सच्च);

    // Add arguments
    AstVarScope* ध्रुव oVscp = newArgument(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* ध्रुव nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ध्रुव auto rd = [flp](AstVarScope* vp) { पटिनिवत्त नूतन AstVarRef{flp, vp, VAccess::READ}; };
    ध्रुव auto wr = [flp](AstVarScope* vp) { पटिनिवत्त नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, m_n_Vec_शब्दाः};
    loopp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(nVscp), limp}});

    // Done
    पटिनिवत्त funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ध्रुव oDtypep,
                                       AstUnpackArrayDType* ध्रुव iDtypep) ध्रुव {
    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    File_पङ्क्तिः* ध्रुव flp = netlistp->शिखरम्_Scopep()->fileline();
    Ast_ग्रन्थिः_DType* ध्रुव u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* ध्रुव funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(सच्च);

    // Add arguments
    AstVarScope* ध्रुव oVscp = newArgument(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* ध्रुव iVscp = newArgument(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ध्रुव nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ध्रुव auto rd = [flp](AstVarScope* vp) { पटिनिवत्त नूतन AstVarRef{flp, vp, VAccess::READ}; };
    ध्रुव auto wr = [flp](AstVarScope* vp) { पटिनिवत्त नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव oWordp = नूतन AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव iWordp = नूतन AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstOr{flp, oWordp, iWordp};
    AstConst* ध्रुव outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ध्रुव निवेशः_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव limp = outputRangeLeftp->अङ्कः().toSInt() < निवेशः_Range_Leftp->अङ्कः().toSInt()
                                  ? outputRangeLeftp->clone_वृक्षः_Pure(मिच्छा)
                                  : निवेशः_Range_Leftp->clone_वृक्षः_Pure(मिच्छा);
    loopp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLte{flp, rd(nVscp), limp}});

    // Done
    पटिनिवत्त funcp;
}

Ast_ग्रन्थिः_व्यञ्जकम्* TriggerKit::newAnySetCall(AstVarScope* ध्रुव vscp) ध्रुव {
    File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->शिखरम्_Scopep()->fileline();
    सचे (!m_n_Vec_शब्दाः) पटिनिवत्त नूतन AstConst{flp, AstConst::BitFalse{}};

    AstCFunc* funcp = nullptr;
    सचे (vscp->dtypep() == m_trigVecDTypep) {
        सचे (!m_anySetVecp) m_anySetVecp = createAnySetFunc(m_trigVecDTypep);
        funcp = m_anySetVecp;
    } अञ्ञथा सचे (vscp->dtypep() == m_trigExtDTypep) {
        सचे (!m_anySetExtp) m_anySetExtp = createAnySetFunc(m_trigExtDTypep);
        funcp = m_anySetExtp;
    } अञ्ञथा {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, funcp};
    callp->addArgsp(नूतन AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetBit();
    पटिनिवत्त callp;
}
Ast_ग्रन्थिः_Stmt* TriggerKit::newClearCall(AstVarScope* ध्रुव vscp) ध्रुव {
    सचे (!m_n_Vec_शब्दाः) पटिनिवत्त nullptr;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    सचे (!m_clearp) m_clearp = createClearFunc();
    File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->शिखरम्_Scopep()->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, m_clearp};
    callp->addArgsp(नूतन AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetVoid();
    पटिनिवत्त callp->makeStmt();
}
Ast_ग्रन्थिः_Stmt* TriggerKit::newOrIntoCall(AstVarScope* ध्रुव oVscp, AstVarScope* ध्रुव iVscp) ध्रुव {
    सचे (!m_n_Vec_शब्दाः) पटिनिवत्त nullptr;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    ध्रुव size_t mask
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[mask];
    सचे (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->शिखरम्_Scopep()->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, funcp};
    callp->addArgsp(नूतन AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->addArgsp(नूतन AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtypeSetVoid();
    पटिनिवत्त callp->makeStmt();
}

Ast_ग्रन्थिः_Stmt* TriggerKit::newCompBaseCall() ध्रुव {
    सचे (!m_n_Vec_शब्दाः) पटिनिवत्त nullptr;
    File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->शिखरम्_Scopep()->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, m_compVecp};
    callp->dtypeSetVoid();
    पटिनिवत्त callp->makeStmt();
}

Ast_ग्रन्थिः_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ध्रुव {
    सचे (!m_n_Pre_शब्दाः) पटिनिवत्त nullptr;
    File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->शिखरम्_Scopep()->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, m_compExtp};
    callp->addArgsp(नूतन AstVarRef{flp, vscp, VAccess::READ});
    callp->dtypeSetVoid();
    पटिनिवत्त callp->makeStmt();
}

Ast_ग्रन्थिः_Stmt* TriggerKit::newDumpCall(AstVarScope* ध्रुव vscp, ध्रुव std::string& tag,
                                     तार्किक debugOnly) ध्रुव {
    सचे (!m_n_Vec_शब्दाः) पटिनिवत्त nullptr;
    AstCFunc* funcp = nullptr;
    सचे (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } अञ्ञथा सचे (vscp->dtypep() == m_trigExtDTypep) {
        सचे (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } अञ्ञथा {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->शिखरम्_Scopep()->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, funcp};
    callp->addArgsp(नूतन AstVarRef{flp, vscp, VAccess::READ});
    callp->addArgsp(नूतन AstConst{flp, AstConst::String{}, tag});
    callp->dtypeSetVoid();
    AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{flp};
    cstmtp->add("#ifdef VL_DEBUG\n");
    सचे (debugOnly) {
        cstmtp->add("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->add(callp->makeStmt());
        cstmtp->add("}\n");
    } अञ्ञथा {
        cstmtp->add(callp->makeStmt());
    }
    cstmtp->add("#endif");
    पटिनिवत्त cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(ध्रुव std::string& name) ध्रुव {
    सचे (!m_n_Vec_शब्दाः) पटिनिवत्त nullptr;
    पटिनिवत्त v3Global.rootp()->शिखरम्_Scopep()->createTemp("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extraTriggerIndex(uint32_t index) ध्रुव {
    UASSERT(index <= m_n_Extra_शब्दाः * WORD_SIZE, "Invalid external trigger index");
    पटिनिवत्त index + m_n_Sense_शब्दाः * WORD_SIZE;
}

std::set<uint32_t> TriggerKit::triggerIndices(ध्रुव Ast_Sen_वृक्षः* senTreep, तार्किक pre) ध्रुव {
    std::set<uint32_t> indices;
    अत्थाय (ध्रुव AstSenItem* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), SenItem)) {
        uint32_t index = m_senItem2TrigIdx.at(*sip);
        सचे (pre) {
            UASSERT_OBJ(index < m_n_Pre_शब्दाः * WORD_SIZE, sip, "Not a 'pre' trigger");
            index += m_n_Vec_शब्दाः * WORD_SIZE;
        }
        indices.निवेशय(index);
    }
    पटिनिवत्त indices;
}

Ast_ग्रन्थिः_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ध्रुव {
    index = extraTriggerIndex(index);
    ध्रुव uint32_t wordIndex = index / WORD_SIZE;
    ध्रुव uint32_t bitIndex = index % WORD_SIZE;
    File_पङ्क्तिः* ध्रुव flp = vscp->fileline();
    AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव wordp = नूतन AstArraySel{flp, refp, static_cast<पूर्णाङ्क>(wordIndex)};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव trigLhsp = नूतन AstSel{flp, wordp, static_cast<पूर्णाङ्क>(bitIndex), 1};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव trigRhsp = नूतन AstVarRef{flp, vscp, VAccess::READ};
    पटिनिवत्त नूतन AstAssign{flp, trigLhsp, trigRhsp};
}

शून्य TriggerKit::addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ध्रुव {
    File_पङ्क्तिः* ध्रुव flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_ग्रन्थिः* ध्रुव setp = newExtraTriggerAssignment(vscp, index);
    setp->addNext(नूतन AstAssign{flp, नूतन AstVarRef{flp, vscp, VAccess::WRITE},
                                नूतन AstConst{flp, AstConst::BitFalse{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
शून्य TriggerKit::add_मानम्_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t index) ध्रुव {
    index += m_n_Sense_शब्दाः * WORD_SIZE;
    ध्रुव uint32_t wordIndex = index / WORD_SIZE;
    ध्रुव uint32_t bitIndex = index % WORD_SIZE;

    Ast_शिखरम्_Scope* ध्रुव शिखरम्_Scopep = netlistp->शिखरम्_Scopep();
    File_पङ्क्तिः* ध्रुव flp = netlistp->fileline();
    Ast_ग्रन्थिः_DType* ध्रुव dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    सचे (ध्रुव AstBasicDType* ध्रुव bdtypep = VN_CAST(dtypep, BasicDType)) {
        सचे (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            पटिनिवत्त;
        }
    } अञ्ञथा सचे (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, ग्रन्थिः_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        पटिनिवत्त;
    }

    ध्रुव auto rdInst = [flp](AstVarScope* vp) { पटिनिवत्त नूतन AstVarRef{flp, vp, VAccess::READ}; };
    ध्रुव auto wrPrev = [flp](AstVarScope* vp) { पटिनिवत्त नूतन AstVarRef{flp, vp, VAccess::WRITE}; };
    ध्रुव auto rdPrev = [flp](AstVarScope* vp) { पटिनिवत्त नूतन AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    ध्रुव std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* ध्रुव prevVscp = शिखरम्_Scopep->createTemp(prevName, instVscp->dtypep());

    // Initialize prev = inst
    सचे (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ध्रुव cmhp = नूतन AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        initFuncp->addStmtsp(cmhp->makeStmt());
    } अञ्ञथा {
        initFuncp->addStmtsp(नूतन AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_ग्रन्थिः_व्यञ्जकम्* neqp;
    सचे (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ध्रुव cmhp
            = नूतन AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtypeSetBit();
        neqp = cmhp;
    } अञ्ञथा {
        neqp = नूतन AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_ग्रन्थिः* updp;
    सचे (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ध्रुव cmhp = नूतन AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        updp = cmhp->makeStmt();
    } अञ्ञथा {
        updp = नूतन AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव wordp = नूतन AstArraySel{flp, refp, static_cast<पूर्णाङ्क>(wordIndex)};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव trigLhsp = नूतन AstSel{flp, wordp, static_cast<पूर्णाङ्क>(bitIndex), 1};
    Ast_ग्रन्थिः* ध्रुव setp = नूतन AstAssign{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->addNext(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ध्रुव std::string& name, तार्किक slow, uint32_t n_Sense_शब्दाः,
                       uint32_t n_Extra_शब्दाः, uint32_t n_Pre_शब्दाः,
                       std::unordered_map<VNRef<ध्रुव AstSenItem>, size_t> senItem2TrigIdx,
                       ध्रुव std::vector<ध्रुव AstSenItem*>& senItemps, तार्किक useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_शब्दाः{n_Sense_शब्दाः}
    , m_n_Extra_शब्दाः{n_Extra_शब्दाः}
    , m_n_Pre_शब्दाः{n_Pre_शब्दाः}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trigIdx2SenItem{senItemps} {
    // If no triggers, we don't need to generate anything
    सचे (!m_n_Vec_शब्दाः) पटिनिवत्त;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trigIdx2SenItem.resize((m_n_Vec_शब्दाः + m_n_Pre_शब्दाः) * WORD_SIZE);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_शब्दाः * WORD_SIZE,
              m_trigIdx2SenItem.begin() + m_n_Vec_शब्दाः * WORD_SIZE);
    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    AstScope* ध्रुव scopep = netlistp->शिखरम्_Scopep()->scopep();
    File_पङ्क्तिः* ध्रुव flp = scopep->fileline();
    // Data type of a single trigger word
    m_wordDTypep = netlistp->findBitDType(WORD_SIZE, WORD_SIZE, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ध्रुव rp = नूतन AstRange{flp, static_cast<पूर्णाङ्क>(m_n_Vec_शब्दाः - 1), 0};
    m_trigVecDTypep = नूतन AstUnpackArrayDType{flp, m_wordDTypep, rp};
    netlistp->typeTablep()->addTypesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    सचे (m_n_Pre_शब्दाः) {
        AstRange* ध्रुव ep = नूतन AstRange{flp, static_cast<पूर्णाङ्क>(m_n_Vec_शब्दाः + m_n_Pre_शब्दाः - 1), 0};
        m_trigExtDTypep = नूतन AstUnpackArrayDType{flp, m_wordDTypep, ep};
        netlistp->typeTablep()->addTypesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } अञ्ञथा {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->शिखरम्_Scopep()->createTemp("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->isInternal(सच्च);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, सच्च);
    m_dumpp->isStatic(सच्च);
    m_dumpp->ifdef("VL_DEBUG");
    सचे (useAcc) {
        m_vscAccp
            = netlistp->शिखरम्_Scopep()->createTemp("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->isInternal(सच्च);
    }
}

AstAssign* TriggerKit::createSenTrigVecAssignment(AstVarScope* ध्रुव target,
                                                  std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*>& trigps) {
    File_पङ्क्तिः* ध्रुव flp = target->fileline();
    AstAssign* trigStmtsp = nullptr;
    // Assign sense triggers vector one word at a time
    अत्थाय (size_t i = 0; i < trigps.size(); i += WORD_SIZE) {
        // Concatenate all bits in this trigger word using a balanced
        अत्थाय (uint32_t level = 0; level < WORD_SIZE_LOG2; ++level) {
            ध्रुव uint32_t stride = 1 << level;
            अत्थाय (uint32_t j = 0; j < WORD_SIZE; j += 2 * stride) {
                trigps[i + j] = नूतन AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = nullptr;
            }
        }

        // Set the whole word in the trigger vector
        ध्रुव पूर्णाङ्क wordIndex = static_cast<पूर्णाङ्क>(i / WORD_SIZE);
        AstArraySel* ध्रुव aselp
            = नूतन AstArraySel{flp, नूतन AstVarRef{flp, target, VAccess::WRITE}, wordIndex};
        trigStmtsp = Ast_ग्रन्थिः::addNext(trigStmtsp, नूतन AstAssign{flp, aselp, trigps[i]});
    }
    पटिनिवत्त trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ध्रुव initFuncp,  //
                              Sen_व्यञ्जकम्_Builder& sen_व्यञ्जकम्_Builder,  //
                              ध्रुव std::vector<ध्रुव Ast_Sen_वृक्षः*>& preTreeps,  //
                              ध्रुव std::vector<ध्रुव Ast_Sen_वृक्षः*>& senTreeps,  //
                              ध्रुव string& name,  //
                              ध्रुव ExtraTriggers& extraTriggers,  //
                              तार्किक slow,  //
                              तार्किक useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::vector<ध्रुव AstSenItem*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<ध्रुव AstSenItem>, size_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    अत्थाय (ध्रुव Ast_Sen_वृक्षः* ध्रुव senTreep : preTreeps) {
        अत्थाय (ध्रुव AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ध्रुव auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            सचे (pair.second) senItemps.प्रक्षिप_back(itemp);
        }
    }
    ध्रुव uint32_t n_Pre_Sen_वस्तूनि = senItemps.size();
    V3Stats::addStat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_वस्तूनि);
    // Number of pre triggers, rounded up to a full word.
    ध्रुव uint32_t nPreTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ध्रुव uint32_t n_Pre_शब्दाः = nPreTriggers / WORD_SIZE;

    // Process the rest of the trees
    अत्थाय (ध्रुव Ast_Sen_वृक्षः* ध्रुव senTreep : senTreeps) {
        अत्थाय (ध्रुव AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ध्रुव auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            सचे (pair.second) senItemps.प्रक्षिप_back(itemp);
        }
    }
    ध्रुव uint32_t n_Sen_वस्तूनि = senItemps.size() - nPreTriggers;
    V3Stats::addStat("Scheduling, '" + name + "' sense triggers", n_Sen_वस्तूनि + n_Pre_Sen_वस्तूनि);
    // Number of sense triggers, rounded up to a full word
    ध्रुव uint32_t nSenseTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ध्रुव uint32_t n_Sense_शब्दाः = nSenseTriggers / WORD_SIZE;

    // Allocate space for the extra triggers
    V3Stats::addStat("Scheduling, '" + name + "' extra triggers", extraTriggers.size());
    // Number of extra triggers, rounded up to a full word.
    ध्रुव uint32_t nExtraTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(extraTriggers.size());
    ध्रुव uint32_t n_Extra_शब्दाः = nExtraTriggers / WORD_SIZE;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_शब्दाः, n_Extra_शब्दाः, n_Pre_शब्दाः, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    सचे (!kit.m_n_Vec_शब्दाः) पटिनिवत्त kit;

    File_पङ्क्तिः* ध्रुव flp = netlistp->शिखरम्_Scopep()->fileline();

    // Creates read/write reference
    ध्रुव auto rd = [flp](AstVarScope* vp) { पटिनिवत्त नूतन AstVarRef{flp, vp, VAccess::READ}; };
    ध्रुव auto wr = [flp](AstVarScope* vp) { पटिनिवत्त नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* ध्रुव dumpTrgp
        = newArgument(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* ध्रुव dumpTagp
        = newArgument(kit.m_dumpp, netlistp->findStringDType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* ध्रुव ifp = नूतन AstIf{flp, नूतन AstLogNot{flp, kit.newAnySetCall(dumpTrgp)}};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    ध्रुव auto addDebug = [&](uint32_t index, ध्रुव string& text) {
        ध्रुव पूर्णाङ्क wrdIndex = static_cast<पूर्णाङ्क>(index / WORD_SIZE);
        ध्रुव पूर्णाङ्क bitIndex = static_cast<पूर्णाङ्क>(index % WORD_SIZE);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव aselp = नूतन AstArraySel{flp, rd(dumpTrgp), wrdIndex};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = नूतन AstSel{flp, aselp, bitIndex, 1};
        AstIf* ध्रुव ifp = नूतन AstIf{flp, condp};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"' region trigger index " + std::to_string(index) + " is active: " + text
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_ग्रन्थिः_Stmt* initialTrigsp = nullptr;
    अत्थाय (size_t i = 0; i < senItemps.size(); ++i) {
        ध्रुव AstSenItem* ध्रुव senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        सचे (!senItemp) {
            trigps.emplace_back(नूतन AstConst{flp, AstConst::BitFalse{}});
            अनुवर्त;
        }

        // Create the trigger computation expression
        ध्रुव auto& pair = sen_व्यञ्जकम्_Builder.build(senItemp);
        trigps.emplace_back(pair.first);

        // Add initialization time trigger
        सचे (pair.second || v3Global.opt.xInitialEdge()) {
            ध्रुव पूर्णाङ्क wrdIndex = static_cast<पूर्णाङ्क>(i / WORD_SIZE);
            ध्रुव पूर्णाङ्क bitIndex = static_cast<पूर्णाङ्क>(i % WORD_SIZE);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव wordp = नूतन AstArraySel{flp, wr(kit.m_vscp), wrdIndex};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstSel{flp, wordp, bitIndex, 1};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstConst{flp, AstConst::BitTrue{}};
            सचे (useAcc) {
                initFuncp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
            } अञ्ञथा {
                initialTrigsp = Ast_ग्रन्थिः::addNext(initialTrigsp, नूतन AstAssign{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_वृक्षः(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        addDebug(i, desc);
    }
    UASSERT(trigps.size() == nSenseTriggers, "Inconsistent number of trigger expressions");

    AstAssign* ध्रुव trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    अत्थाय (unsigned i = 0; i < extraTriggers.size(); ++i) {
        addDebug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    ध्रुव Sen_व्यञ्जकम्_Builder::Results senResults = sen_व्यञ्जकम्_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    अत्थाय (Ast_ग्रन्थिः_Stmt* ध्रुव nodep : senResults.m_inits) initFuncp->addStmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_शिखरम्_Scope* ध्रुव शिखरम्_Scopep = netlistp->शिखरम्_Scopep();
    {
        AstCFunc* ध्रुव fp = kit.m_compVecp;
        // Trigger computation
        अत्थाय (Ast_ग्रन्थिः_Stmt* ध्रुव nodep : senResults.m_preUpdates) fp->addStmtsp(nodep);
        fp->addStmtsp(trigStmtsp);
        अत्थाय (Ast_ग्रन्थिः_Stmt* ध्रुव nodep : senResults.m_postUpdates) fp->addStmtsp(nodep);
        // Add the initialization time triggers
        सचे (initialTrigsp) {
            AstVarScope* ध्रुव initVscp = शिखरम्_Scopep->createTemp("__V" + name + "DidInit", 1);
            AstIf* ध्रुव ifp = नूतन AstIf{flp, नूतन AstNot{flp, rd(initVscp)}};
            fp->addStmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->addThensp(util::setVar(initVscp, 1));
            ifp->addThensp(initialTrigsp);
        }
        util::splitCheck(fp);
    };
    // If there are 'pre' triggers, compute them
    सचे (kit.m_n_Pre_शब्दाः) {
        AstCFunc* ध्रुव fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ध्रुव latchedp
            = newArgument(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ध्रुव nVscp = शिखरम्_Scopep->createTemp("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(सच्च);
        // Add a loop to compute the pre words
        AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
        fp->addStmtsp(util::setVar(nVscp, 0));
        fp->addStmtsp(loopp);
        // Loop body
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव offsetp = नूतन AstConst{flp, kit.m_n_Vec_शब्दाः};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lIdxp = नूतन AstAdd{flp, rd(nVscp), offsetp};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव aWordp = नूतन AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bWordp = नूतन AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstAnd{flp, aWordp, नूतन AstNot{flp, bWordp}};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, n_Pre_शब्दाः};
        loopp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(nVscp));
        loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(nVscp), limp}});
        util::splitCheck(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    सचे (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlinkFrBackWithNext()->delete_वृक्षः();
    // Done with the trigger dump function, split as might be large
    util::splitCheck(kit.m_dumpp);

    पटिनिवत्त kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
वग्ग AwaitBeforeTrigVisitor final : सार्वजनिक VNVisitor {
    ध्रुव VNUser1InUse m_user1InUse;

    /**
     * AstCAwait::user1()       ->  bool.           True if node has been visited
     * AstSenTree::user1p()     ->  AstCFunc*.      Function that has to be called before awaiting
     *                                              for CAwait pointing to this SenTree
     * AstCFunc::user1p()       ->  AstVarScope*    Function's local temporary extended trigger
     *                                              vector variable scope
     */

    // Netlist - needed for using util::makeSubFunction()
    AstNetlist* ध्रुव m_netlistp;
    // Trigger kit - for accessing trigger vectors and mapping senItems to thier indexes
    ध्रुव TriggerKit& m_trigKit;
    // Expression builder - for building expressions from SenItems
    Sen_व्यञ्जकम्_Builder& m_sen_व्यञ्जकम्_Builder;
    // Generator of unique names for before-trigger function
    V3UniqueNames m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::vector<std::pair<AstCFunc*, Ast_Sen_वृक्षः*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::vector<std::pair<Ast_Sen_वृक्षः*, Ast_ग्रन्थिः_व्यञ्जकम्*>> m_sen_वृक्षः_To_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_ग्रन्थिः>, std::vector<AstSenItem*>> m_sen_व्यञ्जकम्_To_Sen_Item;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    स्थिर Ast_ग्रन्थिः* get_Sen_Hash_ग्रन्थिः(ध्रुव AstSenItem* ध्रुव nodep) {
        सचे (AstVarRef* ध्रुव varRefp = VN_CAST(nodep->sensp(), VarRef)) पटिनिवत्त varRefp;
        पटिनिवत्त nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    शून्य fill_Sen_व्यञ्जकम्_To_Sen_Item() {
        अत्थाय (auto sen_वृक्षः_Sched : m_sen_वृक्षः_To_Sched) {
            Ast_Sen_वृक्षः* ध्रुव senTreep = sen_वृक्षः_Sched.first;

            अत्थाय (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ध्रुव VEdgeType edge = senItemp->edgeType();
                सचे (edge.anEdge() || edge == VEdgeType::ET_CHANGED
                    || edge == VEdgeType::ET_HYBRID) {
                    m_sen_व्यञ्जकम्_To_Sen_Item[*get_Sen_Hash_ग्रन्थिः(senItemp)].प्रक्षिप_back(senItemp);
                }
            }
        }

        std::vector<VNRef<Ast_ग्रन्थिः>> toRemove;
        अत्थाय (ध्रुव auto& sen_व्यञ्जकम्_To_Sen_वृक्षः : m_sen_व्यञ्जकम्_To_Sen_Item) {
            std::vector<AstSenItem*> senItemps = sen_व्यञ्जकम्_To_Sen_वृक्षः.second;
            toRemove.प्रक्षिप_back(sen_व्यञ्जकम्_To_Sen_वृक्षः.first);
            अत्थाय (size_t i = 1; i < senItemps.size(); ++i) {
                सचे (senItemps[i]->edgeType() != senItemps[i - 1]->edgeType()) {
                    toRemove.निष्कासय_back();
                    भङ्ग;
                }
            }
        }
        अत्थाय (ध्रुव VNRef<Ast_ग्रन्थिः> it : toRemove) m_sen_व्यञ्जकम्_To_Sen_Item.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<size_t, std::map<size_t, std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*>>>
    getUsedTriggersToTrees(ध्रुव std::set<size_t>& usedTriggers) {
        std::map<size_t, std::map<size_t, std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*>>> usedTrigsToUsingTrees;
        अत्थाय (auto sen_वृक्षः_Sched : m_sen_वृक्षः_To_Sched) {
            ध्रुव Ast_Sen_वृक्षः* ध्रुव senTreep = sen_वृक्षः_Sched.first;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव shedp = sen_वृक्षः_Sched.second;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            अत्थाय (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ध्रुव size_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                सचे (usedTriggers.find(idx) != usedTriggers.end()) {
                    usedTrigsToUsingTrees[idx / TriggerKit::WORD_SIZE]
                                         [size_t{1} << (idx % TriggerKit::WORD_SIZE)]
                                             .प्रक्षिप_back(shedp);
                }
            }
        }
        सचे (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            अत्थाय (ध्रुव auto& triggersToTrees : usedTrigsToUsingTrees) {
                अत्थाय (ध्रुव auto& bitsToTrees : triggersToTrees.second) {
                    ध्रुव std::set<ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्*> exprps{bitsToTrees.second.begin(),
                                                              bitsToTrees.second.end()};
                    UASSERT(bitsToTrees.second.size() == exprps.size(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        पटिनिवत्त usedTrigsToUsingTrees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_वृक्षः* ध्रुव senTreep) {
        File_पङ्क्तिः* ध्रुव flp = senTreep->fileline();
        सचे (!senTreep->user1p()) {
            AstCFunc* ध्रुव funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), मिच्छा);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* ध्रुव tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(सच्च);

            AstVar* ध्रुव argp = नूतन AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->findBasicDType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(सच्च);
            argp->direction(VDirection::निवेशः);
            funcp->addArgsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_back(funcp, senTreep);
        }
        AstCCall* ध्रुव callp = नूतन AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtypeSetVoid();
        पटिनिवत्त callp;
    }

    शून्य visit(AstCAwait* ध्रुव nodep) override {
        सचे (nodep->user1SetOnce()) पटिनिवत्त;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        सचे (ध्रुव AstCMethodHard* ध्रुव cMethodHardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            सचे (cMethodHardp->method() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ध्रुव beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_ग्रन्थिः* ध्रुव pinp = cMethodHardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, cMethodHardp, "No event description");
                beforeTrigp->addArgsp(VN_AS(pinp, ग्रन्थिः_व्यञ्जकम्)->clone_वृक्षः(मिच्छा));

                // Call the before-trigger function before the CAwait
                nodep->addHereThisAsNext(beforeTrigp->makeStmt());
                m_sen_वृक्षः_To_Sched.emplace_back(nodep->sentreep(), cMethodHardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    शून्य visit(Ast_ग्रन्थिः* ध्रुव nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_व्यञ्जकम्_Builder& sen_व्यञ्जकम्_Builder,
                           ध्रुव TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_व्यञ्जकम्_Builder{sen_व्यञ्जकम्_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_व्यञ्जकम्_To_Sen_Item();

        std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> trigps;
        std::set<size_t> usedTriggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        अत्थाय (ध्रुव auto& funcToUsedTriggers : m_generatedFuncs) {
            AstCFunc* ध्रुव funcp = funcToUsedTriggers.first;
            AstVarScope* ध्रुव vscp = VN_AS(funcp->user1p(), VarScope);
            File_पङ्क्तिः* ध्रुव flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_वृक्षः* ध्रुव senTreep = funcToUsedTriggers.second;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ध्रुव auto emplaceAt
                    = [flp, &trigps, &usedTriggers](Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp, ध्रुव size_t pos) {
                          ध्रुव size_t targetSize
                              = vlstd::roundUpToMultipleOf<TriggerKit::WORD_SIZE>(pos + 1);
                          सचे (trigps.capacity() < targetSize) trigps.reserve(targetSize * 2);
                          याव (trigps.size() < targetSize) {
                              trigps.प्रक्षिप_back(नूतन AstConst{flp, AstConst::BitFalse{}});
                          }
                          trigps[pos]->delete_वृक्षः();
                          trigps[pos] = exprp;
                          usedTriggers.निवेशय(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                अत्थाय (ध्रुव AstSenItem* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), SenItem)) {
                    ध्रुव size_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_व्यञ्जकम्_Builder.build(itemp).first, idx);
                    auto iter = m_sen_व्यञ्जकम्_To_Sen_Item.find(*get_Sen_Hash_ग्रन्थिः(itemp));
                    सचे (iter != m_sen_व्यञ्जकम्_To_Sen_Item.end()) {
                        अत्थाय (AstSenItem* ध्रुव additionalItemp : iter->second) {
                            ध्रुव size_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_व्यञ्जकम्_Builder.build(additionalItemp).first, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ध्रुव Sen_व्यञ्जकम्_Builder::Results results
                    = m_sen_व्यञ्जकम्_Builder.getResultsAndClearUpdates();
                अत्थाय (Ast_ग्रन्थिः_Stmt* ध्रुव stmtsp : results.m_inits) funcp->addStmtsp(stmtsp);
                अत्थाय (Ast_ग्रन्थिः_Stmt* ध्रुव stmtsp : results.m_preUpdates) funcp->addStmtsp(stmtsp);
                funcp->addStmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                अत्थाय (Ast_ग्रन्थिः_Stmt* ध्रुव stmtsp : results.m_postUpdates) funcp->addStmtsp(stmtsp);
            }

            ध्रुव std::map<size_t, std::map<size_t, std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*>>>
                usedTrigsToUsingTrees = getUsedTriggersToTrees(usedTriggers);
            usedTriggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            ध्रुव auto getIdx = [flp](AstVarScope* ध्रुव scocep, VAccess access, size_t idx) {
                पटिनिवत्त नूतन AstArraySel{flp, नूतन AstVarRef{flp, scocep, access},
                                       नूतन AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* ध्रुव argpVscp = नूतन AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->addVarsp(argpVscp);

            // Mark as ready triggered schedulers
            अत्थाय (ध्रुव auto& triggersToTrees : usedTrigsToUsingTrees) {
                ध्रुव size_t word = triggersToTrees.first;

                अत्थाय (ध्रुव auto& bitsToTrees : triggersToTrees.second) {
                    ध्रुव size_t bit = bitsToTrees.first;
                    ध्रुव auto& schedulers = bitsToTrees.second;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* ध्रुव maskConstp = नूतन AstConst{flp, AstConst::Unsized64{}, bit};
                    AstAnd* ध्रुव condp
                        = नूतन AstAnd{flp, getIdx(vscp, VAccess::READ, word), maskConstp};
                    AstIf* ध्रुव ifp = नूतन AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव schedp : schedulers) {
                        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
                            flp, schedp->clone_वृक्षः(मिच्छा), VCMethod::SCHED_READY};
                        callp->dtypeSetVoid();
                        callp->addPinsp(नूतन AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->addThensp(callp->makeStmt());
                    }
                    funcp->addStmtsp(ifp);
                }
            }

            AstVarScope* ध्रुव vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            अत्थाय (ध्रुव auto& triggersToTrees : usedTrigsToUsingTrees) {
                ध्रुव size_t word = triggersToTrees.first;
                funcp->addStmtsp(नूतन AstAssign{flp, getIdx(vscAccp, VAccess::WRITE, word),
                                               नूतन AstOr{flp, getIdx(vscAccp, VAccess::READ, word),
                                                         getIdx(vscp, VAccess::READ, word)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = स्वभाव;
};

शून्य beforeTrigVisitor(AstNetlist* netlistp, Sen_व्यञ्जकम्_Builder& sen_व्यञ्जकम्_Builder,
                       ध्रुव TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_व्यञ्जकम्_Builder, trigKit};
}

}  // namespace V3Sched
