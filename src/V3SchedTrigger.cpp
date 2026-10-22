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

AstVarScope* newLocal(AstCFunc* funcp, Ast_गाँठ_DType* dtypep, ध्रुव std::string& name) {
    File_पंक्ति* ध्रुव flp = funcp->fileline();
    AstScope* ध्रुव scopep = funcp->scopep();
    AstVar* ध्रुव varp = नूतन AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(सत्य);
    varp->noReset(सत्य);
    funcp->addVarsp(varp);
    AstVarScope* ध्रुव vscp = नूतन AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    प्रत्यागम vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() ध्रुव {
    UASSERT(m_n_Pre_शब्दगण, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    File_पंक्ति* ध्रुव flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाँठ_DType* ध्रुव u32DTypep = netlistp->findUInt32DType();
    Ast_गाँठ_DType* ध्रुव strDtypep = netlistp->findStringDType();

    // Dumping function always slow
    ध्रुव std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* ध्रुव funcp = util::makeSubFunction(netlistp, name, सत्य);
    funcp->isStatic(सत्य);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* ध्रुव eVscp = newArgument(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* ध्रुव tVscp = newArgument(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    ध्रुव स्वतः rd = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::READ}; };
    ध्रुव स्वतः wr = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

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
        Ast_गाँठ_व्यंजक* ध्रुव lhsp = नूतन AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_गाँठ_व्यंजक* ध्रुव rhsp = नूतन AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_गाँठ_व्यंजक* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मूल्य{}, 32, m_n_Vec_शब्दगण};
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
            Ast_गाँठ_व्यंजक* ध्रुव lhsp = नूतन AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_गाँठ_व्यंजक* ध्रुव rhsp = नूतन AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_गाँठ_व्यंजक* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मूल्य{}, 32, m_n_Pre_शब्दगण};
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
            Ast_गाँठ_व्यंजक* ध्रुव lhsp = नूतन AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_गाँठ_व्यंजक* ध्रुव rhsp = नूतन AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
            Ast_गाँठ_व्यंजक* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मूल्य{}, 32, m_n_Vec_शब्दगण};
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

    प्रत्यागम funcp;
}

AstCFunc* TriggerKit::createAnySetFunc(AstUnpackArrayDType* ध्रुव dtypep) ध्रुव {
    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    File_पंक्ति* ध्रुव flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाँठ_DType* ध्रुव u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* ध्रुव funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(सत्य);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* ध्रुव iVscp = newArgument(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ध्रुव nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    ध्रुव स्वतः rd = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);
    funcp->addStmtsp(नूतन AstCReturn{flp, नूतन AstConst{flp, AstConst::BitFalse{}}});

    // Loop body
    ध्रुव uint32_t n_शब्दगण = dtypep->elementsConst();
    Ast_गाँठ_व्यंजक* ध्रुव condp = नूतन AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_गाँठ_Stmt* ध्रुव thenp = नूतन AstCReturn{flp, नूतन AstConst{flp, AstConst::BitTrue{}}};
    Ast_गाँठ_व्यंजक* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मूल्य{}, 32, n_शब्दगण};
    loopp->addStmtsp(नूतन AstIf{flp, condp, thenp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(nVscp), limp}});

    // Done
    प्रत्यागम funcp;
}
AstCFunc* TriggerKit::createClearFunc() ध्रुव {
    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    File_पंक्ति* ध्रुव flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाँठ_DType* ध्रुव u32DTypep = netlistp->findUInt32DType();

    // Create function
    AstCFunc* ध्रुव funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(सत्य);

    // Add arguments
    AstVarScope* ध्रुव oVscp = newArgument(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* ध्रुव nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ध्रुव स्वतः rd = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::READ}; };
    ध्रुव स्वतः wr = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_गाँठ_व्यंजक* ध्रुव lhsp = नूतन AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_गाँठ_व्यंजक* ध्रुव rhsp = नूतन AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
    Ast_गाँठ_व्यंजक* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मूल्य{}, 32, m_n_Vec_शब्दगण};
    loopp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(nVscp), limp}});

    // Done
    प्रत्यागम funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ध्रुव oDtypep,
                                       AstUnpackArrayDType* ध्रुव iDtypep) ध्रुव {
    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    File_पंक्ति* ध्रुव flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाँठ_DType* ध्रुव u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* ध्रुव funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(सत्य);

    // Add arguments
    AstVarScope* ध्रुव oVscp = newArgument(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* ध्रुव iVscp = newArgument(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ध्रुव nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ध्रुव स्वतः rd = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::READ}; };
    ध्रुव स्वतः wr = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_गाँठ_व्यंजक* ध्रुव lhsp = नूतन AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_गाँठ_व्यंजक* ध्रुव oWordp = नूतन AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_गाँठ_व्यंजक* ध्रुव iWordp = नूतन AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_गाँठ_व्यंजक* ध्रुव rhsp = नूतन AstOr{flp, oWordp, iWordp};
    AstConst* ध्रुव outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ध्रुव निवेश_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_गाँठ_व्यंजक* ध्रुव limp = outputRangeLeftp->अंक().toSInt() < निवेश_Range_Leftp->अंक().toSInt()
                                  ? outputRangeLeftp->clone_वृक्ष_Pure(असत्य)
                                  : निवेश_Range_Leftp->clone_वृक्ष_Pure(असत्य);
    loopp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLte{flp, rd(nVscp), limp}});

    // Done
    प्रत्यागम funcp;
}

Ast_गाँठ_व्यंजक* TriggerKit::newAnySetCall(AstVarScope* ध्रुव vscp) ध्रुव {
    File_पंक्ति* ध्रुव flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    यदि (!m_n_Vec_शब्दगण) प्रत्यागम नूतन AstConst{flp, AstConst::BitFalse{}};

    AstCFunc* funcp = nullptr;
    यदि (vscp->dtypep() == m_trigVecDTypep) {
        यदि (!m_anySetVecp) m_anySetVecp = createAnySetFunc(m_trigVecDTypep);
        funcp = m_anySetVecp;
    } अन्यथा यदि (vscp->dtypep() == m_trigExtDTypep) {
        यदि (!m_anySetExtp) m_anySetExtp = createAnySetFunc(m_trigExtDTypep);
        funcp = m_anySetExtp;
    } अन्यथा {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, funcp};
    callp->addArgsp(नूतन AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetBit();
    प्रत्यागम callp;
}
Ast_गाँठ_Stmt* TriggerKit::newClearCall(AstVarScope* ध्रुव vscp) ध्रुव {
    यदि (!m_n_Vec_शब्दगण) प्रत्यागम nullptr;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    यदि (!m_clearp) m_clearp = createClearFunc();
    File_पंक्ति* ध्रुव flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, m_clearp};
    callp->addArgsp(नूतन AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetVoid();
    प्रत्यागम callp->makeStmt();
}
Ast_गाँठ_Stmt* TriggerKit::newOrIntoCall(AstVarScope* ध्रुव oVscp, AstVarScope* ध्रुव iVscp) ध्रुव {
    यदि (!m_n_Vec_शब्दगण) प्रत्यागम nullptr;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    ध्रुव माप_प्रकार mask
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[mask];
    यदि (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_पंक्ति* ध्रुव flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, funcp};
    callp->addArgsp(नूतन AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->addArgsp(नूतन AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtypeSetVoid();
    प्रत्यागम callp->makeStmt();
}

Ast_गाँठ_Stmt* TriggerKit::newCompBaseCall() ध्रुव {
    यदि (!m_n_Vec_शब्दगण) प्रत्यागम nullptr;
    File_पंक्ति* ध्रुव flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, m_compVecp};
    callp->dtypeSetVoid();
    प्रत्यागम callp->makeStmt();
}

Ast_गाँठ_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ध्रुव {
    यदि (!m_n_Pre_शब्दगण) प्रत्यागम nullptr;
    File_पंक्ति* ध्रुव flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, m_compExtp};
    callp->addArgsp(नूतन AstVarRef{flp, vscp, VAccess::READ});
    callp->dtypeSetVoid();
    प्रत्यागम callp->makeStmt();
}

Ast_गाँठ_Stmt* TriggerKit::newDumpCall(AstVarScope* ध्रुव vscp, ध्रुव std::string& tag,
                                     तार्किक debugOnly) ध्रुव {
    यदि (!m_n_Vec_शब्दगण) प्रत्यागम nullptr;
    AstCFunc* funcp = nullptr;
    यदि (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } अन्यथा यदि (vscp->dtypep() == m_trigExtDTypep) {
        यदि (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } अन्यथा {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_पंक्ति* ध्रुव flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, funcp};
    callp->addArgsp(नूतन AstVarRef{flp, vscp, VAccess::READ});
    callp->addArgsp(नूतन AstConst{flp, AstConst::String{}, tag});
    callp->dtypeSetVoid();
    AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{flp};
    cstmtp->add("#ifdef VL_DEBUG\n");
    यदि (debugOnly) {
        cstmtp->add("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->add(callp->makeStmt());
        cstmtp->add("}\n");
    } अन्यथा {
        cstmtp->add(callp->makeStmt());
    }
    cstmtp->add("#endif");
    प्रत्यागम cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(ध्रुव std::string& name) ध्रुव {
    यदि (!m_n_Vec_शब्दगण) प्रत्यागम nullptr;
    प्रत्यागम v3Global.rootp()->शीर्ष_Scopep()->createTemp("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extraTriggerIndex(uint32_t index) ध्रुव {
    UASSERT(index <= m_n_Extra_शब्दगण * WORD_SIZE, "Invalid external trigger index");
    प्रत्यागम index + m_n_Sense_शब्दगण * WORD_SIZE;
}

std::set<uint32_t> TriggerKit::triggerIndices(ध्रुव Ast_Sen_वृक्ष* senTreep, तार्किक pre) ध्रुव {
    std::set<uint32_t> indices;
    प्रति (ध्रुव AstSenItem* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), SenItem)) {
        uint32_t index = m_senItem2TrigIdx.at(*sip);
        यदि (pre) {
            UASSERT_OBJ(index < m_n_Pre_शब्दगण * WORD_SIZE, sip, "Not a 'pre' trigger");
            index += m_n_Vec_शब्दगण * WORD_SIZE;
        }
        indices.जोड़ो(index);
    }
    प्रत्यागम indices;
}

Ast_गाँठ_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ध्रुव {
    index = extraTriggerIndex(index);
    ध्रुव uint32_t wordIndex = index / WORD_SIZE;
    ध्रुव uint32_t bitIndex = index % WORD_SIZE;
    File_पंक्ति* ध्रुव flp = vscp->fileline();
    AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_गाँठ_व्यंजक* ध्रुव wordp = नूतन AstArraySel{flp, refp, static_cast<पूर्णाङ्क>(wordIndex)};
    Ast_गाँठ_व्यंजक* ध्रुव trigLhsp = नूतन AstSel{flp, wordp, static_cast<पूर्णाङ्क>(bitIndex), 1};
    Ast_गाँठ_व्यंजक* ध्रुव trigRhsp = नूतन AstVarRef{flp, vscp, VAccess::READ};
    प्रत्यागम नूतन AstAssign{flp, trigLhsp, trigRhsp};
}

शून्य TriggerKit::addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ध्रुव {
    File_पंक्ति* ध्रुव flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_गाँठ* ध्रुव setp = newExtraTriggerAssignment(vscp, index);
    setp->addNext(नूतन AstAssign{flp, नूतन AstVarRef{flp, vscp, VAccess::WRITE},
                                नूतन AstConst{flp, AstConst::BitFalse{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
शून्य TriggerKit::add_मूल्य_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t index) ध्रुव {
    index += m_n_Sense_शब्दगण * WORD_SIZE;
    ध्रुव uint32_t wordIndex = index / WORD_SIZE;
    ध्रुव uint32_t bitIndex = index % WORD_SIZE;

    Ast_शीर्ष_Scope* ध्रुव शीर्ष_Scopep = netlistp->शीर्ष_Scopep();
    File_पंक्ति* ध्रुव flp = netlistp->fileline();
    Ast_गाँठ_DType* ध्रुव dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    यदि (ध्रुव AstBasicDType* ध्रुव bdtypep = VN_CAST(dtypep, BasicDType)) {
        यदि (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            प्रत्यागम;
        }
    } अन्यथा यदि (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, गाँठ_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        प्रत्यागम;
    }

    ध्रुव स्वतः rdInst = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::READ}; };
    ध्रुव स्वतः wrPrev = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::WRITE}; };
    ध्रुव स्वतः rdPrev = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    ध्रुव std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* ध्रुव prevVscp = शीर्ष_Scopep->createTemp(prevName, instVscp->dtypep());

    // Initialize prev = inst
    यदि (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ध्रुव cmhp = नूतन AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        initFuncp->addStmtsp(cmhp->makeStmt());
    } अन्यथा {
        initFuncp->addStmtsp(नूतन AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_गाँठ_व्यंजक* neqp;
    यदि (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ध्रुव cmhp
            = नूतन AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtypeSetBit();
        neqp = cmhp;
    } अन्यथा {
        neqp = नूतन AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_गाँठ* updp;
    यदि (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ध्रुव cmhp = नूतन AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        updp = cmhp->makeStmt();
    } अन्यथा {
        updp = नूतन AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_गाँठ_व्यंजक* ध्रुव wordp = नूतन AstArraySel{flp, refp, static_cast<पूर्णाङ्क>(wordIndex)};
    Ast_गाँठ_व्यंजक* ध्रुव trigLhsp = नूतन AstSel{flp, wordp, static_cast<पूर्णाङ्क>(bitIndex), 1};
    Ast_गाँठ* ध्रुव setp = नूतन AstAssign{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->addNext(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ध्रुव std::string& name, तार्किक slow, uint32_t n_Sense_शब्दगण,
                       uint32_t n_Extra_शब्दगण, uint32_t n_Pre_शब्दगण,
                       std::unordered_map<VNRef<ध्रुव AstSenItem>, माप_प्रकार> senItem2TrigIdx,
                       ध्रुव std::vector<ध्रुव AstSenItem*>& senItemps, तार्किक useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_शब्दगण{n_Sense_शब्दगण}
    , m_n_Extra_शब्दगण{n_Extra_शब्दगण}
    , m_n_Pre_शब्दगण{n_Pre_शब्दगण}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trigIdx2SenItem{senItemps} {
    // If no triggers, we don't need to generate anything
    यदि (!m_n_Vec_शब्दगण) प्रत्यागम;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trigIdx2SenItem.resize((m_n_Vec_शब्दगण + m_n_Pre_शब्दगण) * WORD_SIZE);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_शब्दगण * WORD_SIZE,
              m_trigIdx2SenItem.begin() + m_n_Vec_शब्दगण * WORD_SIZE);
    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    AstScope* ध्रुव scopep = netlistp->शीर्ष_Scopep()->scopep();
    File_पंक्ति* ध्रुव flp = scopep->fileline();
    // Data type of a single trigger word
    m_wordDTypep = netlistp->findBitDType(WORD_SIZE, WORD_SIZE, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ध्रुव rp = नूतन AstRange{flp, static_cast<पूर्णाङ्क>(m_n_Vec_शब्दगण - 1), 0};
    m_trigVecDTypep = नूतन AstUnpackArrayDType{flp, m_wordDTypep, rp};
    netlistp->typeTablep()->addTypesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    यदि (m_n_Pre_शब्दगण) {
        AstRange* ध्रुव ep = नूतन AstRange{flp, static_cast<पूर्णाङ्क>(m_n_Vec_शब्दगण + m_n_Pre_शब्दगण - 1), 0};
        m_trigExtDTypep = नूतन AstUnpackArrayDType{flp, m_wordDTypep, ep};
        netlistp->typeTablep()->addTypesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } अन्यथा {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->शीर्ष_Scopep()->createTemp("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->isInternal(सत्य);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, सत्य);
    m_dumpp->isStatic(सत्य);
    m_dumpp->ifdef("VL_DEBUG");
    यदि (useAcc) {
        m_vscAccp
            = netlistp->शीर्ष_Scopep()->createTemp("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->isInternal(सत्य);
    }
}

AstAssign* TriggerKit::createSenTrigVecAssignment(AstVarScope* ध्रुव target,
                                                  std::vector<Ast_गाँठ_व्यंजक*>& trigps) {
    File_पंक्ति* ध्रुव flp = target->fileline();
    AstAssign* trigStmtsp = nullptr;
    // Assign sense triggers vector one word at a time
    प्रति (माप_प्रकार i = 0; i < trigps.size(); i += WORD_SIZE) {
        // Concatenate all bits in this trigger word using a balanced
        प्रति (uint32_t level = 0; level < WORD_SIZE_LOG2; ++level) {
            ध्रुव uint32_t stride = 1 << level;
            प्रति (uint32_t j = 0; j < WORD_SIZE; j += 2 * stride) {
                trigps[i + j] = नूतन AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = nullptr;
            }
        }

        // Set the whole word in the trigger vector
        ध्रुव पूर्णाङ्क wordIndex = static_cast<पूर्णाङ्क>(i / WORD_SIZE);
        AstArraySel* ध्रुव aselp
            = नूतन AstArraySel{flp, नूतन AstVarRef{flp, target, VAccess::WRITE}, wordIndex};
        trigStmtsp = Ast_गाँठ::addNext(trigStmtsp, नूतन AstAssign{flp, aselp, trigps[i]});
    }
    प्रत्यागम trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ध्रुव initFuncp,  //
                              Sen_व्यंजक_Builder& sen_व्यंजक_Builder,  //
                              ध्रुव std::vector<ध्रुव Ast_Sen_वृक्ष*>& preTreeps,  //
                              ध्रुव std::vector<ध्रुव Ast_Sen_वृक्ष*>& senTreeps,  //
                              ध्रुव string& name,  //
                              ध्रुव ExtraTriggers& extraTriggers,  //
                              तार्किक slow,  //
                              तार्किक useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::vector<ध्रुव AstSenItem*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<ध्रुव AstSenItem>, माप_प्रकार> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    प्रति (ध्रुव Ast_Sen_वृक्ष* ध्रुव senTreep : preTreeps) {
        प्रति (ध्रुव AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ध्रुव स्वतः pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            यदि (pair.second) senItemps.धकेलो_back(itemp);
        }
    }
    ध्रुव uint32_t n_Pre_Sen_वस्तुएँ = senItemps.size();
    V3Stats::addStat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_वस्तुएँ);
    // Number of pre triggers, rounded up to a full word.
    ध्रुव uint32_t nPreTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ध्रुव uint32_t n_Pre_शब्दगण = nPreTriggers / WORD_SIZE;

    // Process the rest of the trees
    प्रति (ध्रुव Ast_Sen_वृक्ष* ध्रुव senTreep : senTreeps) {
        प्रति (ध्रुव AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ध्रुव स्वतः pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            यदि (pair.second) senItemps.धकेलो_back(itemp);
        }
    }
    ध्रुव uint32_t n_Sen_वस्तुएँ = senItemps.size() - nPreTriggers;
    V3Stats::addStat("Scheduling, '" + name + "' sense triggers", n_Sen_वस्तुएँ + n_Pre_Sen_वस्तुएँ);
    // Number of sense triggers, rounded up to a full word
    ध्रुव uint32_t nSenseTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ध्रुव uint32_t n_Sense_शब्दगण = nSenseTriggers / WORD_SIZE;

    // Allocate space for the extra triggers
    V3Stats::addStat("Scheduling, '" + name + "' extra triggers", extraTriggers.size());
    // Number of extra triggers, rounded up to a full word.
    ध्रुव uint32_t nExtraTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(extraTriggers.size());
    ध्रुव uint32_t n_Extra_शब्दगण = nExtraTriggers / WORD_SIZE;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_शब्दगण, n_Extra_शब्दगण, n_Pre_शब्दगण, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    यदि (!kit.m_n_Vec_शब्दगण) प्रत्यागम kit;

    File_पंक्ति* ध्रुव flp = netlistp->शीर्ष_Scopep()->fileline();

    // Creates read/write reference
    ध्रुव स्वतः rd = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::READ}; };
    ध्रुव स्वतः wr = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

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
    ध्रुव स्वतः addDebug = [&](uint32_t index, ध्रुव string& text) {
        ध्रुव पूर्णाङ्क wrdIndex = static_cast<पूर्णाङ्क>(index / WORD_SIZE);
        ध्रुव पूर्णाङ्क bitIndex = static_cast<पूर्णाङ्क>(index % WORD_SIZE);
        Ast_गाँठ_व्यंजक* ध्रुव aselp = नूतन AstArraySel{flp, rd(dumpTrgp), wrdIndex};
        Ast_गाँठ_व्यंजक* ध्रुव condp = नूतन AstSel{flp, aselp, bitIndex, 1};
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
    std::vector<Ast_गाँठ_व्यंजक*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_गाँठ_Stmt* initialTrigsp = nullptr;
    प्रति (माप_प्रकार i = 0; i < senItemps.size(); ++i) {
        ध्रुव AstSenItem* ध्रुव senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        यदि (!senItemp) {
            trigps.emplace_back(नूतन AstConst{flp, AstConst::BitFalse{}});
            अनुवर्त;
        }

        // Create the trigger computation expression
        ध्रुव स्वतः& pair = sen_व्यंजक_Builder.build(senItemp);
        trigps.emplace_back(pair.first);

        // Add initialization time trigger
        यदि (pair.second || v3Global.opt.xInitialEdge()) {
            ध्रुव पूर्णाङ्क wrdIndex = static_cast<पूर्णाङ्क>(i / WORD_SIZE);
            ध्रुव पूर्णाङ्क bitIndex = static_cast<पूर्णाङ्क>(i % WORD_SIZE);
            Ast_गाँठ_व्यंजक* ध्रुव wordp = नूतन AstArraySel{flp, wr(kit.m_vscp), wrdIndex};
            Ast_गाँठ_व्यंजक* ध्रुव lhsp = नूतन AstSel{flp, wordp, bitIndex, 1};
            Ast_गाँठ_व्यंजक* ध्रुव rhsp = नूतन AstConst{flp, AstConst::BitTrue{}};
            यदि (useAcc) {
                initFuncp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
            } अन्यथा {
                initialTrigsp = Ast_गाँठ::addNext(initialTrigsp, नूतन AstAssign{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_वृक्ष(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        addDebug(i, desc);
    }
    UASSERT(trigps.size() == nSenseTriggers, "Inconsistent number of trigger expressions");

    AstAssign* ध्रुव trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    प्रति (अचिन्हित i = 0; i < extraTriggers.size(); ++i) {
        addDebug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    ध्रुव Sen_व्यंजक_Builder::Results senResults = sen_व्यंजक_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    प्रति (Ast_गाँठ_Stmt* ध्रुव nodep : senResults.m_inits) initFuncp->addStmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_शीर्ष_Scope* ध्रुव शीर्ष_Scopep = netlistp->शीर्ष_Scopep();
    {
        AstCFunc* ध्रुव fp = kit.m_compVecp;
        // Trigger computation
        प्रति (Ast_गाँठ_Stmt* ध्रुव nodep : senResults.m_preUpdates) fp->addStmtsp(nodep);
        fp->addStmtsp(trigStmtsp);
        प्रति (Ast_गाँठ_Stmt* ध्रुव nodep : senResults.m_postUpdates) fp->addStmtsp(nodep);
        // Add the initialization time triggers
        यदि (initialTrigsp) {
            AstVarScope* ध्रुव initVscp = शीर्ष_Scopep->createTemp("__V" + name + "DidInit", 1);
            AstIf* ध्रुव ifp = नूतन AstIf{flp, नूतन AstNot{flp, rd(initVscp)}};
            fp->addStmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->addThensp(util::setVar(initVscp, 1));
            ifp->addThensp(initialTrigsp);
        }
        util::splitCheck(fp);
    };
    // If there are 'pre' triggers, compute them
    यदि (kit.m_n_Pre_शब्दगण) {
        AstCFunc* ध्रुव fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ध्रुव latchedp
            = newArgument(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ध्रुव nVscp = शीर्ष_Scopep->createTemp("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(सत्य);
        // Add a loop to compute the pre words
        AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
        fp->addStmtsp(util::setVar(nVscp, 0));
        fp->addStmtsp(loopp);
        // Loop body
        Ast_गाँठ_व्यंजक* ध्रुव offsetp = नूतन AstConst{flp, kit.m_n_Vec_शब्दगण};
        Ast_गाँठ_व्यंजक* ध्रुव lIdxp = नूतन AstAdd{flp, rd(nVscp), offsetp};
        Ast_गाँठ_व्यंजक* ध्रुव lhsp = नूतन AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_गाँठ_व्यंजक* ध्रुव aWordp = नूतन AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_गाँठ_व्यंजक* ध्रुव bWordp = नूतन AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_गाँठ_व्यंजक* ध्रुव rhsp = नूतन AstAnd{flp, aWordp, नूतन AstNot{flp, bWordp}};
        Ast_गाँठ_व्यंजक* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मूल्य{}, 32, n_Pre_शब्दगण};
        loopp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(nVscp));
        loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(nVscp), limp}});
        util::splitCheck(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    यदि (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlinkFrBackWithNext()->delete_वृक्ष();
    // Done with the trigger dump function, split as might be large
    util::splitCheck(kit.m_dumpp);

    प्रत्यागम kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
वर्ग AwaitBeforeTrigVisitor final : सार्वजनिक VNVisitor {
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
    Sen_व्यंजक_Builder& m_sen_व्यंजक_Builder;
    // Generator of unique names for before-trigger function
    V3UniqueNames m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::vector<std::pair<AstCFunc*, Ast_Sen_वृक्ष*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::vector<std::pair<Ast_Sen_वृक्ष*, Ast_गाँठ_व्यंजक*>> m_sen_वृक्ष_To_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_गाँठ>, std::vector<AstSenItem*>> m_sen_व्यंजक_To_Sen_Item;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    स्थिर Ast_गाँठ* get_Sen_Hash_गाँठ(ध्रुव AstSenItem* ध्रुव nodep) {
        यदि (AstVarRef* ध्रुव varRefp = VN_CAST(nodep->sensp(), VarRef)) प्रत्यागम varRefp;
        प्रत्यागम nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    शून्य fill_Sen_व्यंजक_To_Sen_Item() {
        प्रति (स्वतः sen_वृक्ष_Sched : m_sen_वृक्ष_To_Sched) {
            Ast_Sen_वृक्ष* ध्रुव senTreep = sen_वृक्ष_Sched.first;

            प्रति (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ध्रुव VEdgeType edge = senItemp->edgeType();
                यदि (edge.anEdge() || edge == VEdgeType::ET_CHANGED
                    || edge == VEdgeType::ET_HYBRID) {
                    m_sen_व्यंजक_To_Sen_Item[*get_Sen_Hash_गाँठ(senItemp)].धकेलो_back(senItemp);
                }
            }
        }

        std::vector<VNRef<Ast_गाँठ>> toRemove;
        प्रति (ध्रुव स्वतः& sen_व्यंजक_To_Sen_वृक्ष : m_sen_व्यंजक_To_Sen_Item) {
            std::vector<AstSenItem*> senItemps = sen_व्यंजक_To_Sen_वृक्ष.second;
            toRemove.धकेलो_back(sen_व्यंजक_To_Sen_वृक्ष.first);
            प्रति (माप_प्रकार i = 1; i < senItemps.size(); ++i) {
                यदि (senItemps[i]->edgeType() != senItemps[i - 1]->edgeType()) {
                    toRemove.निकालो_back();
                    भङ्ग;
                }
            }
        }
        प्रति (ध्रुव VNRef<Ast_गाँठ> it : toRemove) m_sen_व्यंजक_To_Sen_Item.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<माप_प्रकार, std::map<माप_प्रकार, std::vector<Ast_गाँठ_व्यंजक*>>>
    getUsedTriggersToTrees(ध्रुव std::set<माप_प्रकार>& usedTriggers) {
        std::map<माप_प्रकार, std::map<माप_प्रकार, std::vector<Ast_गाँठ_व्यंजक*>>> usedTrigsToUsingTrees;
        प्रति (स्वतः sen_वृक्ष_Sched : m_sen_वृक्ष_To_Sched) {
            ध्रुव Ast_Sen_वृक्ष* ध्रुव senTreep = sen_वृक्ष_Sched.first;
            Ast_गाँठ_व्यंजक* ध्रुव shedp = sen_वृक्ष_Sched.second;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            प्रति (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ध्रुव माप_प्रकार idx = m_trigKit.senItem2TrigIdx(senItemp);
                यदि (usedTriggers.find(idx) != usedTriggers.end()) {
                    usedTrigsToUsingTrees[idx / TriggerKit::WORD_SIZE]
                                         [माप_प्रकार{1} << (idx % TriggerKit::WORD_SIZE)]
                                             .धकेलो_back(shedp);
                }
            }
        }
        यदि (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            प्रति (ध्रुव स्वतः& triggersToTrees : usedTrigsToUsingTrees) {
                प्रति (ध्रुव स्वतः& bitsToTrees : triggersToTrees.second) {
                    ध्रुव std::set<ध्रुव Ast_गाँठ_व्यंजक*> exprps{bitsToTrees.second.begin(),
                                                              bitsToTrees.second.end()};
                    UASSERT(bitsToTrees.second.size() == exprps.size(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        प्रत्यागम usedTrigsToUsingTrees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_वृक्ष* ध्रुव senTreep) {
        File_पंक्ति* ध्रुव flp = senTreep->fileline();
        यदि (!senTreep->user1p()) {
            AstCFunc* ध्रुव funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), असत्य);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* ध्रुव tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(सत्य);

            AstVar* ध्रुव argp = नूतन AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->findBasicDType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(सत्य);
            argp->direction(VDirection::निवेश);
            funcp->addArgsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_back(funcp, senTreep);
        }
        AstCCall* ध्रुव callp = नूतन AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtypeSetVoid();
        प्रत्यागम callp;
    }

    शून्य visit(AstCAwait* ध्रुव nodep) override {
        यदि (nodep->user1SetOnce()) प्रत्यागम;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        यदि (ध्रुव AstCMethodHard* ध्रुव cMethodHardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            यदि (cMethodHardp->method() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ध्रुव beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_गाँठ* ध्रुव pinp = cMethodHardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, cMethodHardp, "No event description");
                beforeTrigp->addArgsp(VN_AS(pinp, गाँठ_व्यंजक)->clone_वृक्ष(असत्य));

                // Call the before-trigger function before the CAwait
                nodep->addHereThisAsNext(beforeTrigp->makeStmt());
                m_sen_वृक्ष_To_Sched.emplace_back(nodep->sentreep(), cMethodHardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    शून्य visit(Ast_गाँठ* ध्रुव nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_व्यंजक_Builder& sen_व्यंजक_Builder,
                           ध्रुव TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_व्यंजक_Builder{sen_व्यंजक_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_व्यंजक_To_Sen_Item();

        std::vector<Ast_गाँठ_व्यंजक*> trigps;
        std::set<माप_प्रकार> usedTriggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        प्रति (ध्रुव स्वतः& funcToUsedTriggers : m_generatedFuncs) {
            AstCFunc* ध्रुव funcp = funcToUsedTriggers.first;
            AstVarScope* ध्रुव vscp = VN_AS(funcp->user1p(), VarScope);
            File_पंक्ति* ध्रुव flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_वृक्ष* ध्रुव senTreep = funcToUsedTriggers.second;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ध्रुव स्वतः emplaceAt
                    = [flp, &trigps, &usedTriggers](Ast_गाँठ_व्यंजक* ध्रुव exprp, ध्रुव माप_प्रकार pos) {
                          ध्रुव माप_प्रकार targetSize
                              = vlstd::roundUpToMultipleOf<TriggerKit::WORD_SIZE>(pos + 1);
                          यदि (trigps.capacity() < targetSize) trigps.reserve(targetSize * 2);
                          यावत् (trigps.size() < targetSize) {
                              trigps.धकेलो_back(नूतन AstConst{flp, AstConst::BitFalse{}});
                          }
                          trigps[pos]->delete_वृक्ष();
                          trigps[pos] = exprp;
                          usedTriggers.जोड़ो(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                प्रति (ध्रुव AstSenItem* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), SenItem)) {
                    ध्रुव माप_प्रकार idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_व्यंजक_Builder.build(itemp).first, idx);
                    स्वतः iter = m_sen_व्यंजक_To_Sen_Item.find(*get_Sen_Hash_गाँठ(itemp));
                    यदि (iter != m_sen_व्यंजक_To_Sen_Item.end()) {
                        प्रति (AstSenItem* ध्रुव additionalItemp : iter->second) {
                            ध्रुव माप_प्रकार idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_व्यंजक_Builder.build(additionalItemp).first, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ध्रुव Sen_व्यंजक_Builder::Results results
                    = m_sen_व्यंजक_Builder.getResultsAndClearUpdates();
                प्रति (Ast_गाँठ_Stmt* ध्रुव stmtsp : results.m_inits) funcp->addStmtsp(stmtsp);
                प्रति (Ast_गाँठ_Stmt* ध्रुव stmtsp : results.m_preUpdates) funcp->addStmtsp(stmtsp);
                funcp->addStmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                प्रति (Ast_गाँठ_Stmt* ध्रुव stmtsp : results.m_postUpdates) funcp->addStmtsp(stmtsp);
            }

            ध्रुव std::map<माप_प्रकार, std::map<माप_प्रकार, std::vector<Ast_गाँठ_व्यंजक*>>>
                usedTrigsToUsingTrees = getUsedTriggersToTrees(usedTriggers);
            usedTriggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            ध्रुव स्वतः getIdx = [flp](AstVarScope* ध्रुव scocep, VAccess access, माप_प्रकार idx) {
                प्रत्यागम नूतन AstArraySel{flp, नूतन AstVarRef{flp, scocep, access},
                                       नूतन AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* ध्रुव argpVscp = नूतन AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->addVarsp(argpVscp);

            // Mark as ready triggered schedulers
            प्रति (ध्रुव स्वतः& triggersToTrees : usedTrigsToUsingTrees) {
                ध्रुव माप_प्रकार word = triggersToTrees.first;

                प्रति (ध्रुव स्वतः& bitsToTrees : triggersToTrees.second) {
                    ध्रुव माप_प्रकार bit = bitsToTrees.first;
                    ध्रुव स्वतः& schedulers = bitsToTrees.second;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* ध्रुव maskConstp = नूतन AstConst{flp, AstConst::Unsized64{}, bit};
                    AstAnd* ध्रुव condp
                        = नूतन AstAnd{flp, getIdx(vscp, VAccess::READ, word), maskConstp};
                    AstIf* ध्रुव ifp = नूतन AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    प्रति (Ast_गाँठ_व्यंजक* ध्रुव schedp : schedulers) {
                        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
                            flp, schedp->clone_वृक्ष(असत्य), VCMethod::SCHED_READY};
                        callp->dtypeSetVoid();
                        callp->addPinsp(नूतन AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->addThensp(callp->makeStmt());
                    }
                    funcp->addStmtsp(ifp);
                }
            }

            AstVarScope* ध्रुव vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            प्रति (ध्रुव स्वतः& triggersToTrees : usedTrigsToUsingTrees) {
                ध्रुव माप_प्रकार word = triggersToTrees.first;
                funcp->addStmtsp(नूतन AstAssign{flp, getIdx(vscAccp, VAccess::WRITE, word),
                                               नूतन AstOr{flp, getIdx(vscAccp, VAccess::READ, word),
                                                         getIdx(vscp, VAccess::READ, word)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = स्वभाव;
};

शून्य beforeTrigVisitor(AstNetlist* netlistp, Sen_व्यंजक_Builder& sen_व्यंजक_Builder,
                       ध्रुव TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_व्यंजक_Builder, trigKit};
}

}  // namespace V3Sched
