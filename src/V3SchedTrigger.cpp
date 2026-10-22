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

AstVarScope* newLocal(AstCFunc* funcp, Ast_गाँठो_DType* dtypep, अचर std::string& name) {
    File_पङ्क्ति* अचर flp = funcp->fileline();
    AstScope* अचर scopep = funcp->scopep();
    AstVar* अचर varp = नूतन AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(सत्य);
    varp->noReset(सत्य);
    funcp->addVarsp(varp);
    AstVarScope* अचर vscp = नूतन AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    फर्काऊ vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() अचर {
    UASSERT(m_n_Pre_शब्दहरू, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* अचर netlistp = v3Global.rootp();
    File_पङ्क्ति* अचर flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाँठो_DType* अचर u32DTypep = netlistp->findUInt32DType();
    Ast_गाँठो_DType* अचर strDtypep = netlistp->findStringDType();

    // Dumping function always slow
    अचर std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* अचर funcp = util::makeSubFunction(netlistp, name, सत्य);
    funcp->isStatic(सत्य);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* अचर eVscp = newArgument(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* अचर tVscp = newArgument(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    अचर स्वतः rd = [flp](AstVarScope* vp) { फर्काऊ नूतन AstVarRef{flp, vp, VAccess::READ}; };
    अचर स्वतः wr = [flp](AstVarScope* vp) { फर्काऊ नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* अचर vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* अचर iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->addStmtsp(util::setVar(iVscp, 0));
        // Add loop
        AstLoop* अचर loopp = नूतन AstLoop{flp};
        funcp->addStmtsp(loopp);
        // Loop body
        Ast_गाँठो_अभिव्यक्ति* अचर lhsp = नूतन AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_गाँठो_अभिव्यक्ति* अचर rhsp = नूतन AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_गाँठो_अभिव्यक्ति* अचर limp = नूतन AstConst{flp, AstConst::Widthed_मान{}, 32, m_n_Vec_शब्दहरू};
        loopp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(iVscp));
        loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* अचर callp = नूतन AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(vVscp));
        callp->addArgsp(rd(tVscp));
        funcp->addStmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* अचर pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* अचर jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->addStmtsp(util::setVar(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* अचर loopp = नूतन AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_गाँठो_अभिव्यक्ति* अचर lhsp = नूतन AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_गाँठो_अभिव्यक्ति* अचर rhsp = नूतन AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_गाँठो_अभिव्यक्ति* अचर limp = नूतन AstConst{flp, AstConst::Widthed_मान{}, 32, m_n_Pre_शब्दहरू};
            loopp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* अचर loopp = नूतन AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_गाँठो_अभिव्यक्ति* अचर lhsp = नूतन AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_गाँठो_अभिव्यक्ति* अचर rhsp = नूतन AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
            Ast_गाँठो_अभिव्यक्ति* अचर limp = नूतन AstConst{flp, AstConst::Widthed_मान{}, 32, m_n_Vec_शब्दहरू};
            loopp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* अचर callp = नूतन AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(pVscp));
        callp->addArgsp(
            नूतन AstConcatN{flp, rd(tVscp), नूतन AstConst{flp, AstConst::String{}, " pre"}});
        funcp->addStmtsp(callp->makeStmt());
    }

    फर्काऊ funcp;
}

AstCFunc* TriggerKit::createAnySetFunc(AstUnpackArrayDType* अचर dtypep) अचर {
    AstNetlist* अचर netlistp = v3Global.rootp();
    File_पङ्क्ति* अचर flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाँठो_DType* अचर u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* अचर funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(सत्य);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* अचर iVscp = newArgument(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* अचर nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    अचर स्वतः rd = [flp](AstVarScope* vp) { फर्काऊ नूतन AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* अचर loopp = नूतन AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);
    funcp->addStmtsp(नूतन AstCReturn{flp, नूतन AstConst{flp, AstConst::BitFalse{}}});

    // Loop body
    अचर uint32_t n_शब्दहरू = dtypep->elementsConst();
    Ast_गाँठो_अभिव्यक्ति* अचर condp = नूतन AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_गाँठो_Stmt* अचर thenp = नूतन AstCReturn{flp, नूतन AstConst{flp, AstConst::BitTrue{}}};
    Ast_गाँठो_अभिव्यक्ति* अचर limp = नूतन AstConst{flp, AstConst::Widthed_मान{}, 32, n_शब्दहरू};
    loopp->addStmtsp(नूतन AstIf{flp, condp, thenp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(nVscp), limp}});

    // Done
    फर्काऊ funcp;
}
AstCFunc* TriggerKit::createClearFunc() अचर {
    AstNetlist* अचर netlistp = v3Global.rootp();
    File_पङ्क्ति* अचर flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाँठो_DType* अचर u32DTypep = netlistp->findUInt32DType();

    // Create function
    AstCFunc* अचर funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(सत्य);

    // Add arguments
    AstVarScope* अचर oVscp = newArgument(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* अचर nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    अचर स्वतः rd = [flp](AstVarScope* vp) { फर्काऊ नूतन AstVarRef{flp, vp, VAccess::READ}; };
    अचर स्वतः wr = [flp](AstVarScope* vp) { फर्काऊ नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* अचर loopp = नूतन AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_गाँठो_अभिव्यक्ति* अचर lhsp = नूतन AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_गाँठो_अभिव्यक्ति* अचर rhsp = नूतन AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
    Ast_गाँठो_अभिव्यक्ति* अचर limp = नूतन AstConst{flp, AstConst::Widthed_मान{}, 32, m_n_Vec_शब्दहरू};
    loopp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(nVscp), limp}});

    // Done
    फर्काऊ funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* अचर oDtypep,
                                       AstUnpackArrayDType* अचर iDtypep) अचर {
    AstNetlist* अचर netlistp = v3Global.rootp();
    File_पङ्क्ति* अचर flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाँठो_DType* अचर u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* अचर funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(सत्य);

    // Add arguments
    AstVarScope* अचर oVscp = newArgument(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* अचर iVscp = newArgument(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* अचर nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    अचर स्वतः rd = [flp](AstVarScope* vp) { फर्काऊ नूतन AstVarRef{flp, vp, VAccess::READ}; };
    अचर स्वतः wr = [flp](AstVarScope* vp) { फर्काऊ नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* अचर loopp = नूतन AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_गाँठो_अभिव्यक्ति* अचर lhsp = नूतन AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_गाँठो_अभिव्यक्ति* अचर oWordp = नूतन AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_गाँठो_अभिव्यक्ति* अचर iWordp = नूतन AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_गाँठो_अभिव्यक्ति* अचर rhsp = नूतन AstOr{flp, oWordp, iWordp};
    AstConst* अचर outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* अचर आगत_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_गाँठो_अभिव्यक्ति* अचर limp = outputRangeLeftp->अङ्क().toSInt() < आगत_Range_Leftp->अङ्क().toSInt()
                                  ? outputRangeLeftp->clone_रूख_Pure(असत्य)
                                  : आगत_Range_Leftp->clone_रूख_Pure(असत्य);
    loopp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLte{flp, rd(nVscp), limp}});

    // Done
    फर्काऊ funcp;
}

Ast_गाँठो_अभिव्यक्ति* TriggerKit::newAnySetCall(AstVarScope* अचर vscp) अचर {
    File_पङ्क्ति* अचर flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    यदि (!m_n_Vec_शब्दहरू) फर्काऊ नूतन AstConst{flp, AstConst::BitFalse{}};

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
    AstCCall* अचर callp = नूतन AstCCall{flp, funcp};
    callp->addArgsp(नूतन AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetBit();
    फर्काऊ callp;
}
Ast_गाँठो_Stmt* TriggerKit::newClearCall(AstVarScope* अचर vscp) अचर {
    यदि (!m_n_Vec_शब्दहरू) फर्काऊ nullptr;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    यदि (!m_clearp) m_clearp = createClearFunc();
    File_पङ्क्ति* अचर flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* अचर callp = नूतन AstCCall{flp, m_clearp};
    callp->addArgsp(नूतन AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetVoid();
    फर्काऊ callp->makeStmt();
}
Ast_गाँठो_Stmt* TriggerKit::newOrIntoCall(AstVarScope* अचर oVscp, AstVarScope* अचर iVscp) अचर {
    यदि (!m_n_Vec_शब्दहरू) फर्काऊ nullptr;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    अचर माप_प्रकार mask
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[mask];
    यदि (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_पङ्क्ति* अचर flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* अचर callp = नूतन AstCCall{flp, funcp};
    callp->addArgsp(नूतन AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->addArgsp(नूतन AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtypeSetVoid();
    फर्काऊ callp->makeStmt();
}

Ast_गाँठो_Stmt* TriggerKit::newCompBaseCall() अचर {
    यदि (!m_n_Vec_शब्दहरू) फर्काऊ nullptr;
    File_पङ्क्ति* अचर flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* अचर callp = नूतन AstCCall{flp, m_compVecp};
    callp->dtypeSetVoid();
    फर्काऊ callp->makeStmt();
}

Ast_गाँठो_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) अचर {
    यदि (!m_n_Pre_शब्दहरू) फर्काऊ nullptr;
    File_पङ्क्ति* अचर flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* अचर callp = नूतन AstCCall{flp, m_compExtp};
    callp->addArgsp(नूतन AstVarRef{flp, vscp, VAccess::READ});
    callp->dtypeSetVoid();
    फर्काऊ callp->makeStmt();
}

Ast_गाँठो_Stmt* TriggerKit::newDumpCall(AstVarScope* अचर vscp, अचर std::string& tag,
                                     तार्किक debugOnly) अचर {
    यदि (!m_n_Vec_शब्दहरू) फर्काऊ nullptr;
    AstCFunc* funcp = nullptr;
    यदि (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } अन्यथा यदि (vscp->dtypep() == m_trigExtDTypep) {
        यदि (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } अन्यथा {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_पङ्क्ति* अचर flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* अचर callp = नूतन AstCCall{flp, funcp};
    callp->addArgsp(नूतन AstVarRef{flp, vscp, VAccess::READ});
    callp->addArgsp(नूतन AstConst{flp, AstConst::String{}, tag});
    callp->dtypeSetVoid();
    AstCStmt* अचर cstmtp = नूतन AstCStmt{flp};
    cstmtp->add("#ifdef VL_DEBUG\n");
    यदि (debugOnly) {
        cstmtp->add("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->add(callp->makeStmt());
        cstmtp->add("}\n");
    } अन्यथा {
        cstmtp->add(callp->makeStmt());
    }
    cstmtp->add("#endif");
    फर्काऊ cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(अचर std::string& name) अचर {
    यदि (!m_n_Vec_शब्दहरू) फर्काऊ nullptr;
    फर्काऊ v3Global.rootp()->शीर्ष_Scopep()->createTemp("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extraTriggerIndex(uint32_t index) अचर {
    UASSERT(index <= m_n_Extra_शब्दहरू * WORD_SIZE, "Invalid external trigger index");
    फर्काऊ index + m_n_Sense_शब्दहरू * WORD_SIZE;
}

std::set<uint32_t> TriggerKit::triggerIndices(अचर Ast_Sen_रूख* senTreep, तार्किक pre) अचर {
    std::set<uint32_t> indices;
    लागि (अचर AstSenItem* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), SenItem)) {
        uint32_t index = m_senItem2TrigIdx.at(*sip);
        यदि (pre) {
            UASSERT_OBJ(index < m_n_Pre_शब्दहरू * WORD_SIZE, sip, "Not a 'pre' trigger");
            index += m_n_Vec_शब्दहरू * WORD_SIZE;
        }
        indices.घुसाउ(index);
    }
    फर्काऊ indices;
}

Ast_गाँठो_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) अचर {
    index = extraTriggerIndex(index);
    अचर uint32_t wordIndex = index / WORD_SIZE;
    अचर uint32_t bitIndex = index % WORD_SIZE;
    File_पङ्क्ति* अचर flp = vscp->fileline();
    AstVarRef* अचर refp = नूतन AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_गाँठो_अभिव्यक्ति* अचर wordp = नूतन AstArraySel{flp, refp, static_cast<पूर्णाङ्क>(wordIndex)};
    Ast_गाँठो_अभिव्यक्ति* अचर trigLhsp = नूतन AstSel{flp, wordp, static_cast<पूर्णाङ्क>(bitIndex), 1};
    Ast_गाँठो_अभिव्यक्ति* अचर trigRhsp = नूतन AstVarRef{flp, vscp, VAccess::READ};
    फर्काऊ नूतन AstAssign{flp, trigLhsp, trigRhsp};
}

शून्य TriggerKit::addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) अचर {
    File_पङ्क्ति* अचर flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_गाँठो* अचर setp = newExtraTriggerAssignment(vscp, index);
    setp->addNext(नूतन AstAssign{flp, नूतन AstVarRef{flp, vscp, VAccess::WRITE},
                                नूतन AstConst{flp, AstConst::BitFalse{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
शून्य TriggerKit::add_मान_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t index) अचर {
    index += m_n_Sense_शब्दहरू * WORD_SIZE;
    अचर uint32_t wordIndex = index / WORD_SIZE;
    अचर uint32_t bitIndex = index % WORD_SIZE;

    Ast_शीर्ष_Scope* अचर शीर्ष_Scopep = netlistp->शीर्ष_Scopep();
    File_पङ्क्ति* अचर flp = netlistp->fileline();
    Ast_गाँठो_DType* अचर dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    यदि (अचर AstBasicDType* अचर bdtypep = VN_CAST(dtypep, BasicDType)) {
        यदि (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            फर्काऊ;
        }
    } अन्यथा यदि (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, गाँठो_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        फर्काऊ;
    }

    अचर स्वतः rdInst = [flp](AstVarScope* vp) { फर्काऊ नूतन AstVarRef{flp, vp, VAccess::READ}; };
    अचर स्वतः wrPrev = [flp](AstVarScope* vp) { फर्काऊ नूतन AstVarRef{flp, vp, VAccess::WRITE}; };
    अचर स्वतः rdPrev = [flp](AstVarScope* vp) { फर्काऊ नूतन AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    अचर std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* अचर prevVscp = शीर्ष_Scopep->createTemp(prevName, instVscp->dtypep());

    // Initialize prev = inst
    यदि (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* अचर cmhp = नूतन AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        initFuncp->addStmtsp(cmhp->makeStmt());
    } अन्यथा {
        initFuncp->addStmtsp(नूतन AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_गाँठो_अभिव्यक्ति* neqp;
    यदि (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* अचर cmhp
            = नूतन AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtypeSetBit();
        neqp = cmhp;
    } अन्यथा {
        neqp = नूतन AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_गाँठो* updp;
    यदि (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* अचर cmhp = नूतन AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        updp = cmhp->makeStmt();
    } अन्यथा {
        updp = नूतन AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* अचर refp = नूतन AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_गाँठो_अभिव्यक्ति* अचर wordp = नूतन AstArraySel{flp, refp, static_cast<पूर्णाङ्क>(wordIndex)};
    Ast_गाँठो_अभिव्यक्ति* अचर trigLhsp = नूतन AstSel{flp, wordp, static_cast<पूर्णाङ्क>(bitIndex), 1};
    Ast_गाँठो* अचर setp = नूतन AstAssign{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->addNext(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(अचर std::string& name, तार्किक slow, uint32_t n_Sense_शब्दहरू,
                       uint32_t n_Extra_शब्दहरू, uint32_t n_Pre_शब्दहरू,
                       std::unordered_map<VNRef<अचर AstSenItem>, माप_प्रकार> senItem2TrigIdx,
                       अचर std::vector<अचर AstSenItem*>& senItemps, तार्किक useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_शब्दहरू{n_Sense_शब्दहरू}
    , m_n_Extra_शब्दहरू{n_Extra_शब्दहरू}
    , m_n_Pre_शब्दहरू{n_Pre_शब्दहरू}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trigIdx2SenItem{senItemps} {
    // If no triggers, we don't need to generate anything
    यदि (!m_n_Vec_शब्दहरू) फर्काऊ;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trigIdx2SenItem.resize((m_n_Vec_शब्दहरू + m_n_Pre_शब्दहरू) * WORD_SIZE);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_शब्दहरू * WORD_SIZE,
              m_trigIdx2SenItem.begin() + m_n_Vec_शब्दहरू * WORD_SIZE);
    AstNetlist* अचर netlistp = v3Global.rootp();
    AstScope* अचर scopep = netlistp->शीर्ष_Scopep()->scopep();
    File_पङ्क्ति* अचर flp = scopep->fileline();
    // Data type of a single trigger word
    m_wordDTypep = netlistp->findBitDType(WORD_SIZE, WORD_SIZE, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* अचर rp = नूतन AstRange{flp, static_cast<पूर्णाङ्क>(m_n_Vec_शब्दहरू - 1), 0};
    m_trigVecDTypep = नूतन AstUnpackArrayDType{flp, m_wordDTypep, rp};
    netlistp->typeTablep()->addTypesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    यदि (m_n_Pre_शब्दहरू) {
        AstRange* अचर ep = नूतन AstRange{flp, static_cast<पूर्णाङ्क>(m_n_Vec_शब्दहरू + m_n_Pre_शब्दहरू - 1), 0};
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

AstAssign* TriggerKit::createSenTrigVecAssignment(AstVarScope* अचर target,
                                                  std::vector<Ast_गाँठो_अभिव्यक्ति*>& trigps) {
    File_पङ्क्ति* अचर flp = target->fileline();
    AstAssign* trigStmtsp = nullptr;
    // Assign sense triggers vector one word at a time
    लागि (माप_प्रकार i = 0; i < trigps.size(); i += WORD_SIZE) {
        // Concatenate all bits in this trigger word using a balanced
        लागि (uint32_t level = 0; level < WORD_SIZE_LOG2; ++level) {
            अचर uint32_t stride = 1 << level;
            लागि (uint32_t j = 0; j < WORD_SIZE; j += 2 * stride) {
                trigps[i + j] = नूतन AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = nullptr;
            }
        }

        // Set the whole word in the trigger vector
        अचर पूर्णाङ्क wordIndex = static_cast<पूर्णाङ्क>(i / WORD_SIZE);
        AstArraySel* अचर aselp
            = नूतन AstArraySel{flp, नूतन AstVarRef{flp, target, VAccess::WRITE}, wordIndex};
        trigStmtsp = Ast_गाँठो::addNext(trigStmtsp, नूतन AstAssign{flp, aselp, trigps[i]});
    }
    फर्काऊ trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* अचर initFuncp,  //
                              Sen_अभिव्यक्ति_Builder& sen_अभिव्यक्ति_Builder,  //
                              अचर std::vector<अचर Ast_Sen_रूख*>& preTreeps,  //
                              अचर std::vector<अचर Ast_Sen_रूख*>& senTreeps,  //
                              अचर string& name,  //
                              अचर ExtraTriggers& extraTriggers,  //
                              तार्किक slow,  //
                              तार्किक useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::vector<अचर AstSenItem*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<अचर AstSenItem>, माप_प्रकार> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    लागि (अचर Ast_Sen_रूख* अचर senTreep : preTreeps) {
        लागि (अचर AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            अचर स्वतः pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            यदि (pair.second) senItemps.थप_back(itemp);
        }
    }
    अचर uint32_t n_Pre_Sen_वस्तुहरू = senItemps.size();
    V3Stats::addStat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_वस्तुहरू);
    // Number of pre triggers, rounded up to a full word.
    अचर uint32_t nPreTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    अचर uint32_t n_Pre_शब्दहरू = nPreTriggers / WORD_SIZE;

    // Process the rest of the trees
    लागि (अचर Ast_Sen_रूख* अचर senTreep : senTreeps) {
        लागि (अचर AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            अचर स्वतः pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            यदि (pair.second) senItemps.थप_back(itemp);
        }
    }
    अचर uint32_t n_Sen_वस्तुहरू = senItemps.size() - nPreTriggers;
    V3Stats::addStat("Scheduling, '" + name + "' sense triggers", n_Sen_वस्तुहरू + n_Pre_Sen_वस्तुहरू);
    // Number of sense triggers, rounded up to a full word
    अचर uint32_t nSenseTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    अचर uint32_t n_Sense_शब्दहरू = nSenseTriggers / WORD_SIZE;

    // Allocate space for the extra triggers
    V3Stats::addStat("Scheduling, '" + name + "' extra triggers", extraTriggers.size());
    // Number of extra triggers, rounded up to a full word.
    अचर uint32_t nExtraTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(extraTriggers.size());
    अचर uint32_t n_Extra_शब्दहरू = nExtraTriggers / WORD_SIZE;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_शब्दहरू, n_Extra_शब्दहरू, n_Pre_शब्दहरू, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    यदि (!kit.m_n_Vec_शब्दहरू) फर्काऊ kit;

    File_पङ्क्ति* अचर flp = netlistp->शीर्ष_Scopep()->fileline();

    // Creates read/write reference
    अचर स्वतः rd = [flp](AstVarScope* vp) { फर्काऊ नूतन AstVarRef{flp, vp, VAccess::READ}; };
    अचर स्वतः wr = [flp](AstVarScope* vp) { फर्काऊ नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* अचर dumpTrgp
        = newArgument(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* अचर dumpTagp
        = newArgument(kit.m_dumpp, netlistp->findStringDType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* अचर ifp = नूतन AstIf{flp, नूतन AstLogNot{flp, kit.newAnySetCall(dumpTrgp)}};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* अचर cstmtp = नूतन AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    अचर स्वतः addDebug = [&](uint32_t index, अचर string& text) {
        अचर पूर्णाङ्क wrdIndex = static_cast<पूर्णाङ्क>(index / WORD_SIZE);
        अचर पूर्णाङ्क bitIndex = static_cast<पूर्णाङ्क>(index % WORD_SIZE);
        Ast_गाँठो_अभिव्यक्ति* अचर aselp = नूतन AstArraySel{flp, rd(dumpTrgp), wrdIndex};
        Ast_गाँठो_अभिव्यक्ति* अचर condp = नूतन AstSel{flp, aselp, bitIndex, 1};
        AstIf* अचर ifp = नूतन AstIf{flp, condp};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* अचर cstmtp = नूतन AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"' region trigger index " + std::to_string(index) + " is active: " + text
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::vector<Ast_गाँठो_अभिव्यक्ति*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_गाँठो_Stmt* initialTrigsp = nullptr;
    लागि (माप_प्रकार i = 0; i < senItemps.size(); ++i) {
        अचर AstSenItem* अचर senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        यदि (!senItemp) {
            trigps.emplace_back(नूतन AstConst{flp, AstConst::BitFalse{}});
            जारी;
        }

        // Create the trigger computation expression
        अचर स्वतः& pair = sen_अभिव्यक्ति_Builder.build(senItemp);
        trigps.emplace_back(pair.first);

        // Add initialization time trigger
        यदि (pair.second || v3Global.opt.xInitialEdge()) {
            अचर पूर्णाङ्क wrdIndex = static_cast<पूर्णाङ्क>(i / WORD_SIZE);
            अचर पूर्णाङ्क bitIndex = static_cast<पूर्णाङ्क>(i % WORD_SIZE);
            Ast_गाँठो_अभिव्यक्ति* अचर wordp = नूतन AstArraySel{flp, wr(kit.m_vscp), wrdIndex};
            Ast_गाँठो_अभिव्यक्ति* अचर lhsp = नूतन AstSel{flp, wordp, bitIndex, 1};
            Ast_गाँठो_अभिव्यक्ति* अचर rhsp = नूतन AstConst{flp, AstConst::BitTrue{}};
            यदि (useAcc) {
                initFuncp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
            } अन्यथा {
                initialTrigsp = Ast_गाँठो::addNext(initialTrigsp, नूतन AstAssign{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_रूख(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        addDebug(i, desc);
    }
    UASSERT(trigps.size() == nSenseTriggers, "Inconsistent number of trigger expressions");

    AstAssign* अचर trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    लागि (अचिन्हित i = 0; i < extraTriggers.size(); ++i) {
        addDebug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    अचर Sen_अभिव्यक्ति_Builder::Results senResults = sen_अभिव्यक्ति_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    लागि (Ast_गाँठो_Stmt* अचर nodep : senResults.m_inits) initFuncp->addStmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_शीर्ष_Scope* अचर शीर्ष_Scopep = netlistp->शीर्ष_Scopep();
    {
        AstCFunc* अचर fp = kit.m_compVecp;
        // Trigger computation
        लागि (Ast_गाँठो_Stmt* अचर nodep : senResults.m_preUpdates) fp->addStmtsp(nodep);
        fp->addStmtsp(trigStmtsp);
        लागि (Ast_गाँठो_Stmt* अचर nodep : senResults.m_postUpdates) fp->addStmtsp(nodep);
        // Add the initialization time triggers
        यदि (initialTrigsp) {
            AstVarScope* अचर initVscp = शीर्ष_Scopep->createTemp("__V" + name + "DidInit", 1);
            AstIf* अचर ifp = नूतन AstIf{flp, नूतन AstNot{flp, rd(initVscp)}};
            fp->addStmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->addThensp(util::setVar(initVscp, 1));
            ifp->addThensp(initialTrigsp);
        }
        util::splitCheck(fp);
    };
    // If there are 'pre' triggers, compute them
    यदि (kit.m_n_Pre_शब्दहरू) {
        AstCFunc* अचर fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* अचर latchedp
            = newArgument(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* अचर nVscp = शीर्ष_Scopep->createTemp("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(सत्य);
        // Add a loop to compute the pre words
        AstLoop* अचर loopp = नूतन AstLoop{flp};
        fp->addStmtsp(util::setVar(nVscp, 0));
        fp->addStmtsp(loopp);
        // Loop body
        Ast_गाँठो_अभिव्यक्ति* अचर offsetp = नूतन AstConst{flp, kit.m_n_Vec_शब्दहरू};
        Ast_गाँठो_अभिव्यक्ति* अचर lIdxp = नूतन AstAdd{flp, rd(nVscp), offsetp};
        Ast_गाँठो_अभिव्यक्ति* अचर lhsp = नूतन AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_गाँठो_अभिव्यक्ति* अचर aWordp = नूतन AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_गाँठो_अभिव्यक्ति* अचर bWordp = नूतन AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_गाँठो_अभिव्यक्ति* अचर rhsp = नूतन AstAnd{flp, aWordp, नूतन AstNot{flp, bWordp}};
        Ast_गाँठो_अभिव्यक्ति* अचर limp = नूतन AstConst{flp, AstConst::Widthed_मान{}, 32, n_Pre_शब्दहरू};
        loopp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(nVscp));
        loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(nVscp), limp}});
        util::splitCheck(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    यदि (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlinkFrBackWithNext()->delete_रूख();
    // Done with the trigger dump function, split as might be large
    util::splitCheck(kit.m_dumpp);

    फर्काऊ kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
वर्ग AwaitBeforeTrigVisitor final : सार्वजनिक VNVisitor {
    अचर VNUser1InUse m_user1InUse;

    /**
     * AstCAwait::user1()       ->  bool.           True if node has been visited
     * AstSenTree::user1p()     ->  AstCFunc*.      Function that has to be called before awaiting
     *                                              for CAwait pointing to this SenTree
     * AstCFunc::user1p()       ->  AstVarScope*    Function's local temporary extended trigger
     *                                              vector variable scope
     */

    // Netlist - needed for using util::makeSubFunction()
    AstNetlist* अचर m_netlistp;
    // Trigger kit - for accessing trigger vectors and mapping senItems to thier indexes
    अचर TriggerKit& m_trigKit;
    // Expression builder - for building expressions from SenItems
    Sen_अभिव्यक्ति_Builder& m_sen_अभिव्यक्ति_Builder;
    // Generator of unique names for before-trigger function
    V3UniqueNames m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::vector<std::pair<AstCFunc*, Ast_Sen_रूख*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::vector<std::pair<Ast_Sen_रूख*, Ast_गाँठो_अभिव्यक्ति*>> m_sen_रूख_To_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_गाँठो>, std::vector<AstSenItem*>> m_sen_अभिव्यक्ति_To_Sen_Item;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    स्थिर Ast_गाँठो* get_Sen_Hash_गाँठो(अचर AstSenItem* अचर nodep) {
        यदि (AstVarRef* अचर varRefp = VN_CAST(nodep->sensp(), VarRef)) फर्काऊ varRefp;
        फर्काऊ nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    शून्य fill_Sen_अभिव्यक्ति_To_Sen_Item() {
        लागि (स्वतः sen_रूख_Sched : m_sen_रूख_To_Sched) {
            Ast_Sen_रूख* अचर senTreep = sen_रूख_Sched.first;

            लागि (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                अचर VEdgeType edge = senItemp->edgeType();
                यदि (edge.anEdge() || edge == VEdgeType::ET_CHANGED
                    || edge == VEdgeType::ET_HYBRID) {
                    m_sen_अभिव्यक्ति_To_Sen_Item[*get_Sen_Hash_गाँठो(senItemp)].थप_back(senItemp);
                }
            }
        }

        std::vector<VNRef<Ast_गाँठो>> toRemove;
        लागि (अचर स्वतः& sen_अभिव्यक्ति_To_Sen_रूख : m_sen_अभिव्यक्ति_To_Sen_Item) {
            std::vector<AstSenItem*> senItemps = sen_अभिव्यक्ति_To_Sen_रूख.second;
            toRemove.थप_back(sen_अभिव्यक्ति_To_Sen_रूख.first);
            लागि (माप_प्रकार i = 1; i < senItemps.size(); ++i) {
                यदि (senItemps[i]->edgeType() != senItemps[i - 1]->edgeType()) {
                    toRemove.झिक_back();
                    रोक;
                }
            }
        }
        लागि (अचर VNRef<Ast_गाँठो> it : toRemove) m_sen_अभिव्यक्ति_To_Sen_Item.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<माप_प्रकार, std::map<माप_प्रकार, std::vector<Ast_गाँठो_अभिव्यक्ति*>>>
    getUsedTriggersToTrees(अचर std::set<माप_प्रकार>& usedTriggers) {
        std::map<माप_प्रकार, std::map<माप_प्रकार, std::vector<Ast_गाँठो_अभिव्यक्ति*>>> usedTrigsToUsingTrees;
        लागि (स्वतः sen_रूख_Sched : m_sen_रूख_To_Sched) {
            अचर Ast_Sen_रूख* अचर senTreep = sen_रूख_Sched.first;
            Ast_गाँठो_अभिव्यक्ति* अचर shedp = sen_रूख_Sched.second;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            लागि (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                अचर माप_प्रकार idx = m_trigKit.senItem2TrigIdx(senItemp);
                यदि (usedTriggers.find(idx) != usedTriggers.end()) {
                    usedTrigsToUsingTrees[idx / TriggerKit::WORD_SIZE]
                                         [माप_प्रकार{1} << (idx % TriggerKit::WORD_SIZE)]
                                             .थप_back(shedp);
                }
            }
        }
        यदि (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            लागि (अचर स्वतः& triggersToTrees : usedTrigsToUsingTrees) {
                लागि (अचर स्वतः& bitsToTrees : triggersToTrees.second) {
                    अचर std::set<अचर Ast_गाँठो_अभिव्यक्ति*> exprps{bitsToTrees.second.begin(),
                                                              bitsToTrees.second.end()};
                    UASSERT(bitsToTrees.second.size() == exprps.size(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        फर्काऊ usedTrigsToUsingTrees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_रूख* अचर senTreep) {
        File_पङ्क्ति* अचर flp = senTreep->fileline();
        यदि (!senTreep->user1p()) {
            AstCFunc* अचर funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), असत्य);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* अचर tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(सत्य);

            AstVar* अचर argp = नूतन AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->findBasicDType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(सत्य);
            argp->direction(VDirection::आगत);
            funcp->addArgsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_back(funcp, senTreep);
        }
        AstCCall* अचर callp = नूतन AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtypeSetVoid();
        फर्काऊ callp;
    }

    शून्य visit(AstCAwait* अचर nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        यदि (अचर AstCMethodHard* अचर cMethodHardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            यदि (cMethodHardp->method() == VCMethod::SCHED_TRIGGER) {
                AstCCall* अचर beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_गाँठो* अचर pinp = cMethodHardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, cMethodHardp, "No event description");
                beforeTrigp->addArgsp(VN_AS(pinp, गाँठो_अभिव्यक्ति)->clone_रूख(असत्य));

                // Call the before-trigger function before the CAwait
                nodep->addHereThisAsNext(beforeTrigp->makeStmt());
                m_sen_रूख_To_Sched.emplace_back(nodep->sentreep(), cMethodHardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    शून्य visit(Ast_गाँठो* अचर nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_अभिव्यक्ति_Builder& sen_अभिव्यक्ति_Builder,
                           अचर TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_अभिव्यक्ति_Builder{sen_अभिव्यक्ति_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_अभिव्यक्ति_To_Sen_Item();

        std::vector<Ast_गाँठो_अभिव्यक्ति*> trigps;
        std::set<माप_प्रकार> usedTriggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        लागि (अचर स्वतः& funcToUsedTriggers : m_generatedFuncs) {
            AstCFunc* अचर funcp = funcToUsedTriggers.first;
            AstVarScope* अचर vscp = VN_AS(funcp->user1p(), VarScope);
            File_पङ्क्ति* अचर flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_रूख* अचर senTreep = funcToUsedTriggers.second;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                अचर स्वतः emplaceAt
                    = [flp, &trigps, &usedTriggers](Ast_गाँठो_अभिव्यक्ति* अचर exprp, अचर माप_प्रकार pos) {
                          अचर माप_प्रकार targetSize
                              = vlstd::roundUpToMultipleOf<TriggerKit::WORD_SIZE>(pos + 1);
                          यदि (trigps.capacity() < targetSize) trigps.reserve(targetSize * 2);
                          जबसम्म (trigps.size() < targetSize) {
                              trigps.थप_back(नूतन AstConst{flp, AstConst::BitFalse{}});
                          }
                          trigps[pos]->delete_रूख();
                          trigps[pos] = exprp;
                          usedTriggers.घुसाउ(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                लागि (अचर AstSenItem* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), SenItem)) {
                    अचर माप_प्रकार idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_अभिव्यक्ति_Builder.build(itemp).first, idx);
                    स्वतः iter = m_sen_अभिव्यक्ति_To_Sen_Item.find(*get_Sen_Hash_गाँठो(itemp));
                    यदि (iter != m_sen_अभिव्यक्ति_To_Sen_Item.end()) {
                        लागि (AstSenItem* अचर additionalItemp : iter->second) {
                            अचर माप_प्रकार idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_अभिव्यक्ति_Builder.build(additionalItemp).first, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                अचर Sen_अभिव्यक्ति_Builder::Results results
                    = m_sen_अभिव्यक्ति_Builder.getResultsAndClearUpdates();
                लागि (Ast_गाँठो_Stmt* अचर stmtsp : results.m_inits) funcp->addStmtsp(stmtsp);
                लागि (Ast_गाँठो_Stmt* अचर stmtsp : results.m_preUpdates) funcp->addStmtsp(stmtsp);
                funcp->addStmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                लागि (Ast_गाँठो_Stmt* अचर stmtsp : results.m_postUpdates) funcp->addStmtsp(stmtsp);
            }

            अचर std::map<माप_प्रकार, std::map<माप_प्रकार, std::vector<Ast_गाँठो_अभिव्यक्ति*>>>
                usedTrigsToUsingTrees = getUsedTriggersToTrees(usedTriggers);
            usedTriggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            अचर स्वतः getIdx = [flp](AstVarScope* अचर scocep, VAccess access, माप_प्रकार idx) {
                फर्काऊ नूतन AstArraySel{flp, नूतन AstVarRef{flp, scocep, access},
                                       नूतन AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* अचर argpVscp = नूतन AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->addVarsp(argpVscp);

            // Mark as ready triggered schedulers
            लागि (अचर स्वतः& triggersToTrees : usedTrigsToUsingTrees) {
                अचर माप_प्रकार word = triggersToTrees.first;

                लागि (अचर स्वतः& bitsToTrees : triggersToTrees.second) {
                    अचर माप_प्रकार bit = bitsToTrees.first;
                    अचर स्वतः& schedulers = bitsToTrees.second;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* अचर maskConstp = नूतन AstConst{flp, AstConst::Unsized64{}, bit};
                    AstAnd* अचर condp
                        = नूतन AstAnd{flp, getIdx(vscp, VAccess::READ, word), maskConstp};
                    AstIf* अचर ifp = नूतन AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    लागि (Ast_गाँठो_अभिव्यक्ति* अचर schedp : schedulers) {
                        AstCMethodHard* अचर callp = नूतन AstCMethodHard{
                            flp, schedp->clone_रूख(असत्य), VCMethod::SCHED_READY};
                        callp->dtypeSetVoid();
                        callp->addPinsp(नूतन AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->addThensp(callp->makeStmt());
                    }
                    funcp->addStmtsp(ifp);
                }
            }

            AstVarScope* अचर vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            लागि (अचर स्वतः& triggersToTrees : usedTrigsToUsingTrees) {
                अचर माप_प्रकार word = triggersToTrees.first;
                funcp->addStmtsp(नूतन AstAssign{flp, getIdx(vscAccp, VAccess::WRITE, word),
                                               नूतन AstOr{flp, getIdx(vscAccp, VAccess::READ, word),
                                                         getIdx(vscp, VAccess::READ, word)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = स्वभाव;
};

शून्य beforeTrigVisitor(AstNetlist* netlistp, Sen_अभिव्यक्ति_Builder& sen_अभिव्यक्ति_Builder,
                       अचर TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_अभिव्यक्ति_Builder, trigKit};
}

}  // namespace V3Sched
