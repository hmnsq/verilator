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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Const.h"
#समावेश "V3EmitCBase.h"
#समावेश "V3EmitV.h"
#समावेश "V3Order.h"
#समावेश "V3Sched.h"
#समावेश "V3SenExprBuilder.h"
#समावेश "V3Stats.h"
#समावेश "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

namespace V3Sched {

namespace {

using util::newArgument;

AstVarScope* newLocal(AstCFunc* funcp, Ast_गाँठ_DType* dtypep, स्थिर std::string& name) {
    File_पंक्ति* स्थिर flp = funcp->fileline();
    AstScope* स्थिर scopep = funcp->scopep();
    AstVar* स्थिर varp = नया AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(true);
    varp->noReset(true);
    funcp->addVarsp(varp);
    AstVarScope* स्थिर vscp = नया AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    वापस vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() स्थिर {
    UASSERT(m_n_Pre_शब्दगण, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* स्थिर netlistp = v3Global.rootp();
    File_पंक्ति* स्थिर flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाँठ_DType* स्थिर u32DTypep = netlistp->findUInt32DType();
    Ast_गाँठ_DType* स्थिर strDtypep = netlistp->findStringDType();

    // Dumping function always slow
    स्थिर std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* स्थिर funcp = util::makeSubFunction(netlistp, name, true);
    funcp->isStatic(true);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* स्थिर eVscp = newArgument(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* स्थिर tVscp = newArgument(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    स्थिर स्वतः rd = [flp](AstVarScope* vp) { वापस नया AstVarRef{flp, vp, VAccess::READ}; };
    स्थिर स्वतः wr = [flp](AstVarScope* vp) { वापस नया AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* स्थिर vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* स्थिर iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->addStmtsp(util::setVar(iVscp, 0));
        // Add loop
        AstLoop* स्थिर loopp = नया AstLoop{flp};
        funcp->addStmtsp(loopp);
        // Loop body
        Ast_गाँठ_व्यंजक* स्थिर lhsp = नया AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_गाँठ_व्यंजक* स्थिर rhsp = नया AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_गाँठ_व्यंजक* स्थिर limp = नया AstConst{flp, AstConst::Widthed_मूल्य{}, 32, m_n_Vec_शब्दगण};
        loopp->addStmtsp(नया AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(iVscp));
        loopp->addStmtsp(नया AstLoopTest{flp, loopp, नया AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* स्थिर callp = नया AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(vVscp));
        callp->addArgsp(rd(tVscp));
        funcp->addStmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* स्थिर pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* स्थिर jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->addStmtsp(util::setVar(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* स्थिर loopp = नया AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_गाँठ_व्यंजक* स्थिर lhsp = नया AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_गाँठ_व्यंजक* स्थिर rhsp = नया AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_गाँठ_व्यंजक* स्थिर limp = नया AstConst{flp, AstConst::Widthed_मूल्य{}, 32, m_n_Pre_शब्दगण};
            loopp->addStmtsp(नया AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(नया AstLoopTest{flp, loopp, नया AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* स्थिर loopp = नया AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_गाँठ_व्यंजक* स्थिर lhsp = नया AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_गाँठ_व्यंजक* स्थिर rhsp = नया AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
            Ast_गाँठ_व्यंजक* स्थिर limp = नया AstConst{flp, AstConst::Widthed_मूल्य{}, 32, m_n_Vec_शब्दगण};
            loopp->addStmtsp(नया AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(नया AstLoopTest{flp, loopp, नया AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* स्थिर callp = नया AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(pVscp));
        callp->addArgsp(
            नया AstConcatN{flp, rd(tVscp), नया AstConst{flp, AstConst::String{}, " pre"}});
        funcp->addStmtsp(callp->makeStmt());
    }

    वापस funcp;
}

AstCFunc* TriggerKit::createAnySetFunc(AstUnpackArrayDType* स्थिर dtypep) स्थिर {
    AstNetlist* स्थिर netlistp = v3Global.rootp();
    File_पंक्ति* स्थिर flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाँठ_DType* स्थिर u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* स्थिर funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(true);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* स्थिर iVscp = newArgument(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* स्थिर nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    स्थिर स्वतः rd = [flp](AstVarScope* vp) { वापस नया AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* स्थिर loopp = नया AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);
    funcp->addStmtsp(नया AstCReturn{flp, नया AstConst{flp, AstConst::BitFalse{}}});

    // Loop body
    स्थिर uint32_t n_शब्दगण = dtypep->elementsConst();
    Ast_गाँठ_व्यंजक* स्थिर condp = नया AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_गाँठ_Stmt* स्थिर thenp = नया AstCReturn{flp, नया AstConst{flp, AstConst::BitTrue{}}};
    Ast_गाँठ_व्यंजक* स्थिर limp = नया AstConst{flp, AstConst::Widthed_मूल्य{}, 32, n_शब्दगण};
    loopp->addStmtsp(नया AstIf{flp, condp, thenp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(नया AstLoopTest{flp, loopp, नया AstLt{flp, rd(nVscp), limp}});

    // Done
    वापस funcp;
}
AstCFunc* TriggerKit::createClearFunc() स्थिर {
    AstNetlist* स्थिर netlistp = v3Global.rootp();
    File_पंक्ति* स्थिर flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाँठ_DType* स्थिर u32DTypep = netlistp->findUInt32DType();

    // Create function
    AstCFunc* स्थिर funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(true);

    // Add arguments
    AstVarScope* स्थिर oVscp = newArgument(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* स्थिर nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    स्थिर स्वतः rd = [flp](AstVarScope* vp) { वापस नया AstVarRef{flp, vp, VAccess::READ}; };
    स्थिर स्वतः wr = [flp](AstVarScope* vp) { वापस नया AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* स्थिर loopp = नया AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_गाँठ_व्यंजक* स्थिर lhsp = नया AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_गाँठ_व्यंजक* स्थिर rhsp = नया AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
    Ast_गाँठ_व्यंजक* स्थिर limp = नया AstConst{flp, AstConst::Widthed_मूल्य{}, 32, m_n_Vec_शब्दगण};
    loopp->addStmtsp(नया AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(नया AstLoopTest{flp, loopp, नया AstLt{flp, rd(nVscp), limp}});

    // Done
    वापस funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* स्थिर oDtypep,
                                       AstUnpackArrayDType* स्थिर iDtypep) स्थिर {
    AstNetlist* स्थिर netlistp = v3Global.rootp();
    File_पंक्ति* स्थिर flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाँठ_DType* स्थिर u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* स्थिर funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(true);

    // Add arguments
    AstVarScope* स्थिर oVscp = newArgument(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* स्थिर iVscp = newArgument(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* स्थिर nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    स्थिर स्वतः rd = [flp](AstVarScope* vp) { वापस नया AstVarRef{flp, vp, VAccess::READ}; };
    स्थिर स्वतः wr = [flp](AstVarScope* vp) { वापस नया AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* स्थिर loopp = नया AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_गाँठ_व्यंजक* स्थिर lhsp = नया AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_गाँठ_व्यंजक* स्थिर oWordp = नया AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_गाँठ_व्यंजक* स्थिर iWordp = नया AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_गाँठ_व्यंजक* स्थिर rhsp = नया AstOr{flp, oWordp, iWordp};
    AstConst* स्थिर outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* स्थिर निवेश_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_गाँठ_व्यंजक* स्थिर limp = outputRangeLeftp->अंक().toSInt() < निवेश_Range_Leftp->अंक().toSInt()
                                  ? outputRangeLeftp->clone_वृक्ष_Pure(false)
                                  : निवेश_Range_Leftp->clone_वृक्ष_Pure(false);
    loopp->addStmtsp(नया AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(नया AstLoopTest{flp, loopp, नया AstLte{flp, rd(nVscp), limp}});

    // Done
    वापस funcp;
}

Ast_गाँठ_व्यंजक* TriggerKit::newAnySetCall(AstVarScope* स्थिर vscp) स्थिर {
    File_पंक्ति* स्थिर flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    अगर (!m_n_Vec_शब्दगण) वापस नया AstConst{flp, AstConst::BitFalse{}};

    AstCFunc* funcp = nullptr;
    अगर (vscp->dtypep() == m_trigVecDTypep) {
        अगर (!m_anySetVecp) m_anySetVecp = createAnySetFunc(m_trigVecDTypep);
        funcp = m_anySetVecp;
    } अन्यथा अगर (vscp->dtypep() == m_trigExtDTypep) {
        अगर (!m_anySetExtp) m_anySetExtp = createAnySetFunc(m_trigExtDTypep);
        funcp = m_anySetExtp;
    } अन्यथा {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* स्थिर callp = नया AstCCall{flp, funcp};
    callp->addArgsp(नया AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetBit();
    वापस callp;
}
Ast_गाँठ_Stmt* TriggerKit::newClearCall(AstVarScope* स्थिर vscp) स्थिर {
    अगर (!m_n_Vec_शब्दगण) वापस nullptr;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    अगर (!m_clearp) m_clearp = createClearFunc();
    File_पंक्ति* स्थिर flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* स्थिर callp = नया AstCCall{flp, m_clearp};
    callp->addArgsp(नया AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetVoid();
    वापस callp->makeStmt();
}
Ast_गाँठ_Stmt* TriggerKit::newOrIntoCall(AstVarScope* स्थिर oVscp, AstVarScope* स्थिर iVscp) स्थिर {
    अगर (!m_n_Vec_शब्दगण) वापस nullptr;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    स्थिर माप_प्रकार mask
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[mask];
    अगर (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_पंक्ति* स्थिर flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* स्थिर callp = नया AstCCall{flp, funcp};
    callp->addArgsp(नया AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->addArgsp(नया AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtypeSetVoid();
    वापस callp->makeStmt();
}

Ast_गाँठ_Stmt* TriggerKit::newCompBaseCall() स्थिर {
    अगर (!m_n_Vec_शब्दगण) वापस nullptr;
    File_पंक्ति* स्थिर flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* स्थिर callp = नया AstCCall{flp, m_compVecp};
    callp->dtypeSetVoid();
    वापस callp->makeStmt();
}

Ast_गाँठ_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) स्थिर {
    अगर (!m_n_Pre_शब्दगण) वापस nullptr;
    File_पंक्ति* स्थिर flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* स्थिर callp = नया AstCCall{flp, m_compExtp};
    callp->addArgsp(नया AstVarRef{flp, vscp, VAccess::READ});
    callp->dtypeSetVoid();
    वापस callp->makeStmt();
}

Ast_गाँठ_Stmt* TriggerKit::newDumpCall(AstVarScope* स्थिर vscp, स्थिर std::string& tag,
                                     bool debugOnly) स्थिर {
    अगर (!m_n_Vec_शब्दगण) वापस nullptr;
    AstCFunc* funcp = nullptr;
    अगर (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } अन्यथा अगर (vscp->dtypep() == m_trigExtDTypep) {
        अगर (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } अन्यथा {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_पंक्ति* स्थिर flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* स्थिर callp = नया AstCCall{flp, funcp};
    callp->addArgsp(नया AstVarRef{flp, vscp, VAccess::READ});
    callp->addArgsp(नया AstConst{flp, AstConst::String{}, tag});
    callp->dtypeSetVoid();
    AstCStmt* स्थिर cstmtp = नया AstCStmt{flp};
    cstmtp->add("#ifdef VL_DEBUG\n");
    अगर (debugOnly) {
        cstmtp->add("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->add(callp->makeStmt());
        cstmtp->add("}\n");
    } अन्यथा {
        cstmtp->add(callp->makeStmt());
    }
    cstmtp->add("#endif");
    वापस cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(स्थिर std::string& name) स्थिर {
    अगर (!m_n_Vec_शब्दगण) वापस nullptr;
    वापस v3Global.rootp()->शीर्ष_Scopep()->create_अस्थायी("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extra_Trigger_सूचकांक(uint32_t सूचकांक_2) स्थिर {
    UASSERT(सूचकांक_2 <= m_n_Extra_शब्दगण * WORD_आकार, "Invalid external trigger index");
    वापस सूचकांक_2 + m_n_Sense_शब्दगण * WORD_आकार;
}

std::set<uint32_t> TriggerKit::triggerIndices(स्थिर Ast_Sen_वृक्ष* senTreep, bool pre) स्थिर {
    std::set<uint32_t> indices;
    क्रम (स्थिर AstSenItem* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), SenItem)) {
        uint32_t सूचकांक_2 = m_senItem2TrigIdx.at(*sip);
        अगर (pre) {
            UASSERT_OBJ(सूचकांक_2 < m_n_Pre_शब्दगण * WORD_आकार, sip, "Not a 'pre' trigger");
            सूचकांक_2 += m_n_Vec_शब्दगण * WORD_आकार;
        }
        indices.जोड़ो(सूचकांक_2);
    }
    वापस indices;
}

Ast_गाँठ_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t सूचकांक_2) स्थिर {
    सूचकांक_2 = extra_Trigger_सूचकांक(सूचकांक_2);
    स्थिर uint32_t word_सूचकांक = सूचकांक_2 / WORD_आकार;
    स्थिर uint32_t bit_सूचकांक = सूचकांक_2 % WORD_आकार;
    File_पंक्ति* स्थिर flp = vscp->fileline();
    AstVarRef* स्थिर refp = नया AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_गाँठ_व्यंजक* स्थिर wordp = नया AstArraySel{flp, refp, static_cast<पूर्णांक>(word_सूचकांक)};
    Ast_गाँठ_व्यंजक* स्थिर trigLhsp = नया AstSel{flp, wordp, static_cast<पूर्णांक>(bit_सूचकांक), 1};
    Ast_गाँठ_व्यंजक* स्थिर trigRhsp = नया AstVarRef{flp, vscp, VAccess::READ};
    वापस नया AstAssign{flp, trigLhsp, trigRhsp};
}

व्योम TriggerKit::addExtraTriggerAssignment(AstVarScope* vscp, uint32_t सूचकांक_2) स्थिर {
    File_पंक्ति* स्थिर flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_गाँठ* स्थिर setp = newExtraTriggerAssignment(vscp, सूचकांक_2);
    setp->add_अग्र(नया AstAssign{flp, नया AstVarRef{flp, vscp, VAccess::WRITE},
                                नया AstConst{flp, AstConst::BitFalse{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
व्योम TriggerKit::add_मूल्य_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t सूचकांक_2) स्थिर {
    सूचकांक_2 += m_n_Sense_शब्दगण * WORD_आकार;
    स्थिर uint32_t word_सूचकांक = सूचकांक_2 / WORD_आकार;
    स्थिर uint32_t bit_सूचकांक = सूचकांक_2 % WORD_आकार;

    Ast_शीर्ष_Scope* स्थिर शीर्ष_Scopep = netlistp->शीर्ष_Scopep();
    File_पंक्ति* स्थिर flp = netlistp->fileline();
    Ast_गाँठ_DType* स्थिर dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    अगर (स्थिर AstBasicDType* स्थिर bdtypep = VN_CAST(dtypep, BasicDType)) {
        अगर (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            वापस;
        }
    } अन्यथा अगर (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, गाँठ_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        वापस;
    }

    स्थिर स्वतः rdInst = [flp](AstVarScope* vp) { वापस नया AstVarRef{flp, vp, VAccess::READ}; };
    स्थिर स्वतः wr_पूर्व = [flp](AstVarScope* vp) { वापस नया AstVarRef{flp, vp, VAccess::WRITE}; };
    स्थिर स्वतः rd_पूर्व = [flp](AstVarScope* vp) { वापस नया AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    स्थिर std::string पूर्व_Name = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* स्थिर पूर्व_Vscp = शीर्ष_Scopep->create_अस्थायी(पूर्व_Name, instVscp->dtypep());

    // Initialize prev = inst
    अगर (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* स्थिर cmhp = नया AstCMethodHard{
            flp, wr_पूर्व(पूर्व_Vscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        initFuncp->addStmtsp(cmhp->makeStmt());
    } अन्यथा {
        initFuncp->addStmtsp(नया AstAssign{flp, wr_पूर्व(पूर्व_Vscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_गाँठ_व्यंजक* neqp;
    अगर (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* स्थिर cmhp
            = नया AstCMethodHard{flp, rd_पूर्व(पूर्व_Vscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtypeSetBit();
        neqp = cmhp;
    } अन्यथा {
        neqp = नया AstNeq{flp, rdInst(instVscp), rd_पूर्व(पूर्व_Vscp)};
    }

    // Build post-update: prev = inst
    Ast_गाँठ* updp;
    अगर (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* स्थिर cmhp = नया AstCMethodHard{
            flp, wr_पूर्व(पूर्व_Vscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        updp = cmhp->makeStmt();
    } अन्यथा {
        updp = नया AstAssign{flp, wr_पूर्व(पूर्व_Vscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* स्थिर refp = नया AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_गाँठ_व्यंजक* स्थिर wordp = नया AstArraySel{flp, refp, static_cast<पूर्णांक>(word_सूचकांक)};
    Ast_गाँठ_व्यंजक* स्थिर trigLhsp = नया AstSel{flp, wordp, static_cast<पूर्णांक>(bit_सूचकांक), 1};
    Ast_गाँठ* स्थिर setp = नया AstAssign{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->add_अग्र(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(स्थिर std::string& name, bool slow, uint32_t n_Sense_शब्दगण,
                       uint32_t n_Extra_शब्दगण, uint32_t n_Pre_शब्दगण,
                       std::unordered_map<VNRef<स्थिर AstSenItem>, माप_प्रकार> senItem2TrigIdx,
                       स्थिर std::vector<स्थिर AstSenItem*>& senItemps, bool useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_शब्दगण{n_Sense_शब्दगण}
    , m_n_Extra_शब्दगण{n_Extra_शब्दगण}
    , m_n_Pre_शब्दगण{n_Pre_शब्दगण}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trigIdx2SenItem{senItemps} {
    // If no triggers, we don't need to generate anything
    अगर (!m_n_Vec_शब्दगण) वापस;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trigIdx2SenItem.resize((m_n_Vec_शब्दगण + m_n_Pre_शब्दगण) * WORD_आकार);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_शब्दगण * WORD_आकार,
              m_trigIdx2SenItem.begin() + m_n_Vec_शब्दगण * WORD_आकार);
    AstNetlist* स्थिर netlistp = v3Global.rootp();
    AstScope* स्थिर scopep = netlistp->शीर्ष_Scopep()->scopep();
    File_पंक्ति* स्थिर flp = scopep->fileline();
    // Data type of a single trigger word
    m_wordDTypep = netlistp->findBitDType(WORD_आकार, WORD_आकार, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* स्थिर rp = नया AstRange{flp, static_cast<पूर्णांक>(m_n_Vec_शब्दगण - 1), 0};
    m_trigVecDTypep = नया AstUnpackArrayDType{flp, m_wordDTypep, rp};
    netlistp->typeTablep()->addTypesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    अगर (m_n_Pre_शब्दगण) {
        AstRange* स्थिर ep = नया AstRange{flp, static_cast<पूर्णांक>(m_n_Vec_शब्दगण + m_n_Pre_शब्दगण - 1), 0};
        m_trigExtDTypep = नया AstUnpackArrayDType{flp, m_wordDTypep, ep};
        netlistp->typeTablep()->addTypesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } अन्यथा {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->शीर्ष_Scopep()->create_अस्थायी("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->isInternal(true);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, true);
    m_dumpp->isStatic(true);
    m_dumpp->ifdef("VL_DEBUG");
    अगर (useAcc) {
        m_vscAccp
            = netlistp->शीर्ष_Scopep()->create_अस्थायी("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->isInternal(true);
    }
}

AstAssign* TriggerKit::createSenTrigVecAssignment(AstVarScope* स्थिर target,
                                                  std::vector<Ast_गाँठ_व्यंजक*>& trigps) {
    File_पंक्ति* स्थिर flp = target->fileline();
    AstAssign* trigStmtsp = nullptr;
    // Assign sense triggers vector one word at a time
    क्रम (माप_प्रकार i = 0; i < trigps.आकार(); i += WORD_आकार) {
        // Concatenate all bits in this trigger word using a balanced
        क्रम (uint32_t level = 0; level < WORD_आकार_LOG2; ++level) {
            स्थिर uint32_t stride = 1 << level;
            क्रम (uint32_t j = 0; j < WORD_आकार; j += 2 * stride) {
                trigps[i + j] = नया AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = nullptr;
            }
        }

        // Set the whole word in the trigger vector
        स्थिर पूर्णांक word_सूचकांक = static_cast<पूर्णांक>(i / WORD_आकार);
        AstArraySel* स्थिर aselp
            = नया AstArraySel{flp, नया AstVarRef{flp, target, VAccess::WRITE}, word_सूचकांक};
        trigStmtsp = Ast_गाँठ::add_अग्र(trigStmtsp, नया AstAssign{flp, aselp, trigps[i]});
    }
    वापस trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* स्थिर initFuncp,  //
                              Sen_व्यंजक_Builder& sen_व्यंजक_Builder,  //
                              स्थिर std::vector<स्थिर Ast_Sen_वृक्ष*>& preTreeps,  //
                              स्थिर std::vector<स्थिर Ast_Sen_वृक्ष*>& senTreeps,  //
                              स्थिर string& name,  //
                              स्थिर ExtraTriggers& extraTriggers,  //
                              bool slow,  //
                              bool useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::vector<स्थिर AstSenItem*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<स्थिर AstSenItem>, माप_प्रकार> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    क्रम (स्थिर Ast_Sen_वृक्ष* स्थिर senTreep : preTreeps) {
        क्रम (स्थिर AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            स्थिर स्वतः pair = senItem2TrigIdx.emplace(*itemp, senItemps.आकार());
            अगर (pair.second) senItemps.धकेलो_back(itemp);
        }
    }
    स्थिर uint32_t n_Pre_Sen_वस्तुएँ = senItemps.आकार();
    V3Stats::addStat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_वस्तुएँ);
    // Number of pre triggers, rounded up to a full word.
    स्थिर uint32_t nPreTriggers = vlstd::roundUpToMultipleOf<WORD_आकार>(senItemps.आकार());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    स्थिर uint32_t n_Pre_शब्दगण = nPreTriggers / WORD_आकार;

    // Process the rest of the trees
    क्रम (स्थिर Ast_Sen_वृक्ष* स्थिर senTreep : senTreeps) {
        क्रम (स्थिर AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            स्थिर स्वतः pair = senItem2TrigIdx.emplace(*itemp, senItemps.आकार());
            अगर (pair.second) senItemps.धकेलो_back(itemp);
        }
    }
    स्थिर uint32_t n_Sen_वस्तुएँ = senItemps.आकार() - nPreTriggers;
    V3Stats::addStat("Scheduling, '" + name + "' sense triggers", n_Sen_वस्तुएँ + n_Pre_Sen_वस्तुएँ);
    // Number of sense triggers, rounded up to a full word
    स्थिर uint32_t nSenseTriggers = vlstd::roundUpToMultipleOf<WORD_आकार>(senItemps.आकार());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    स्थिर uint32_t n_Sense_शब्दगण = nSenseTriggers / WORD_आकार;

    // Allocate space for the extra triggers
    V3Stats::addStat("Scheduling, '" + name + "' extra triggers", extraTriggers.आकार());
    // Number of extra triggers, rounded up to a full word.
    स्थिर uint32_t nExtraTriggers = vlstd::roundUpToMultipleOf<WORD_आकार>(extraTriggers.आकार());
    स्थिर uint32_t n_Extra_शब्दगण = nExtraTriggers / WORD_आकार;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_शब्दगण, n_Extra_शब्दगण, n_Pre_शब्दगण, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    अगर (!kit.m_n_Vec_शब्दगण) वापस kit;

    File_पंक्ति* स्थिर flp = netlistp->शीर्ष_Scopep()->fileline();

    // Creates read/write reference
    स्थिर स्वतः rd = [flp](AstVarScope* vp) { वापस नया AstVarRef{flp, vp, VAccess::READ}; };
    स्थिर स्वतः wr = [flp](AstVarScope* vp) { वापस नया AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* स्थिर dumpTrgp
        = newArgument(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* स्थिर dumpTagp
        = newArgument(kit.m_dumpp, netlistp->findStringDType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* स्थिर ifp = नया AstIf{flp, नया AstLogNot{flp, kit.newAnySetCall(dumpTrgp)}};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* स्थिर cstmtp = नया AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    स्थिर स्वतः addDebug = [&](uint32_t सूचकांक_2, स्थिर string& text) {
        स्थिर पूर्णांक wrd_सूचकांक = static_cast<पूर्णांक>(सूचकांक_2 / WORD_आकार);
        स्थिर पूर्णांक bit_सूचकांक = static_cast<पूर्णांक>(सूचकांक_2 % WORD_आकार);
        Ast_गाँठ_व्यंजक* स्थिर aselp = नया AstArraySel{flp, rd(dumpTrgp), wrd_सूचकांक};
        Ast_गाँठ_व्यंजक* स्थिर condp = नया AstSel{flp, aselp, bit_सूचकांक, 1};
        AstIf* स्थिर ifp = नया AstIf{flp, condp};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* स्थिर cstmtp = नया AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"' region trigger index " + std::to_string(सूचकांक_2) + " is active: " + text
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::vector<Ast_गाँठ_व्यंजक*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_गाँठ_Stmt* initialTrigsp = nullptr;
    क्रम (माप_प्रकार i = 0; i < senItemps.आकार(); ++i) {
        स्थिर AstSenItem* स्थिर senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        अगर (!senItemp) {
            trigps.emplace_back(नया AstConst{flp, AstConst::BitFalse{}});
            जारी;
        }

        // Create the trigger computation expression
        स्थिर स्वतः& pair = sen_व्यंजक_Builder.build(senItemp);
        trigps.emplace_back(pair.first);

        // Add initialization time trigger
        अगर (pair.second || v3Global.opt.xInitialEdge()) {
            स्थिर पूर्णांक wrd_सूचकांक = static_cast<पूर्णांक>(i / WORD_आकार);
            स्थिर पूर्णांक bit_सूचकांक = static_cast<पूर्णांक>(i % WORD_आकार);
            Ast_गाँठ_व्यंजक* स्थिर wordp = नया AstArraySel{flp, wr(kit.m_vscp), wrd_सूचकांक};
            Ast_गाँठ_व्यंजक* स्थिर lhsp = नया AstSel{flp, wordp, bit_सूचकांक, 1};
            Ast_गाँठ_व्यंजक* स्थिर rhsp = नया AstConst{flp, AstConst::BitTrue{}};
            अगर (useAcc) {
                initFuncp->addStmtsp(नया AstAssign{flp, lhsp, rhsp});
            } अन्यथा {
                initialTrigsp = Ast_गाँठ::add_अग्र(initialTrigsp, नया AstAssign{flp, lhsp, rhsp});
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
    UASSERT(trigps.आकार() == nSenseTriggers, "Inconsistent number of trigger expressions");

    AstAssign* स्थिर trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    क्रम (अचिन्हित i = 0; i < extraTriggers.आकार(); ++i) {
        addDebug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    स्थिर Sen_व्यंजक_Builder::Results senResults = sen_व्यंजक_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    क्रम (Ast_गाँठ_Stmt* स्थिर nodep : senResults.m_inits) initFuncp->addStmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_शीर्ष_Scope* स्थिर शीर्ष_Scopep = netlistp->शीर्ष_Scopep();
    {
        AstCFunc* स्थिर fp = kit.m_compVecp;
        // Trigger computation
        क्रम (Ast_गाँठ_Stmt* स्थिर nodep : senResults.m_preUpdates) fp->addStmtsp(nodep);
        fp->addStmtsp(trigStmtsp);
        क्रम (Ast_गाँठ_Stmt* स्थिर nodep : senResults.m_postUpdates) fp->addStmtsp(nodep);
        // Add the initialization time triggers
        अगर (initialTrigsp) {
            AstVarScope* स्थिर initVscp = शीर्ष_Scopep->create_अस्थायी("__V" + name + "DidInit", 1);
            AstIf* स्थिर ifp = नया AstIf{flp, नया AstNot{flp, rd(initVscp)}};
            fp->addStmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->addThensp(util::setVar(initVscp, 1));
            ifp->addThensp(initialTrigsp);
        }
        util::splitCheck(fp);
    };
    // If there are 'pre' triggers, compute them
    अगर (kit.m_n_Pre_शब्दगण) {
        AstCFunc* स्थिर fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* स्थिर latchedp
            = newArgument(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* स्थिर nVscp = शीर्ष_Scopep->create_अस्थायी("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(true);
        // Add a loop to compute the pre words
        AstLoop* स्थिर loopp = नया AstLoop{flp};
        fp->addStmtsp(util::setVar(nVscp, 0));
        fp->addStmtsp(loopp);
        // Loop body
        Ast_गाँठ_व्यंजक* स्थिर offsetp = नया AstConst{flp, kit.m_n_Vec_शब्दगण};
        Ast_गाँठ_व्यंजक* स्थिर lIdxp = नया AstAdd{flp, rd(nVscp), offsetp};
        Ast_गाँठ_व्यंजक* स्थिर lhsp = नया AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_गाँठ_व्यंजक* स्थिर aWordp = नया AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_गाँठ_व्यंजक* स्थिर bWordp = नया AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_गाँठ_व्यंजक* स्थिर rhsp = नया AstAnd{flp, aWordp, नया AstNot{flp, bWordp}};
        Ast_गाँठ_व्यंजक* स्थिर limp = नया AstConst{flp, AstConst::Widthed_मूल्य{}, 32, n_Pre_शब्दगण};
        loopp->addStmtsp(नया AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(nVscp));
        loopp->addStmtsp(नया AstLoopTest{flp, loopp, नया AstLt{flp, rd(nVscp), limp}});
        util::splitCheck(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    अगर (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlink_Fr_Back_With_अग्र()->delete_वृक्ष();
    // Done with the trigger dump function, split as might be large
    util::splitCheck(kit.m_dumpp);

    वापस kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
श्रेणी AwaitBeforeTrigVisitor final : खुला VNVisitor {
    स्थिर VNUser1InUse m_user1InUse;

    /**
     * AstCAwait::user1()       ->  bool.           True if node has been visited
     * AstSenTree::user1p()     ->  AstCFunc*.      Function that has to be called before awaiting
     *                                              for CAwait pointing to this SenTree
     * AstCFunc::user1p()       ->  AstVarScope*    Function's local temporary extended trigger
     *                                              vector variable scope
     */

    // Netlist - needed for using util::makeSubFunction()
    AstNetlist* स्थिर m_netlistp;
    // Trigger kit - for accessing trigger vectors and mapping senItems to thier indexes
    स्थिर TriggerKit& m_trigKit;
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
    अटल Ast_गाँठ* get_Sen_Hash_गाँठ(स्थिर AstSenItem* स्थिर nodep) {
        अगर (AstVarRef* स्थिर varRefp = VN_CAST(nodep->sensp(), VarRef)) वापस varRefp;
        वापस nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    व्योम fill_Sen_व्यंजक_To_Sen_Item() {
        क्रम (स्वतः sen_वृक्ष_Sched : m_sen_वृक्ष_To_Sched) {
            Ast_Sen_वृक्ष* स्थिर senTreep = sen_वृक्ष_Sched.first;

            क्रम (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                स्थिर VEdgeType edge = senItemp->edgeType();
                अगर (edge.anEdge() || edge == VEdgeType::ET_CHANGED
                    || edge == VEdgeType::ET_HYBRID) {
                    m_sen_व्यंजक_To_Sen_Item[*get_Sen_Hash_गाँठ(senItemp)].धकेलो_back(senItemp);
                }
            }
        }

        std::vector<VNRef<Ast_गाँठ>> toRemove;
        क्रम (स्थिर स्वतः& sen_व्यंजक_To_Sen_वृक्ष : m_sen_व्यंजक_To_Sen_Item) {
            std::vector<AstSenItem*> senItemps = sen_व्यंजक_To_Sen_वृक्ष.second;
            toRemove.धकेलो_back(sen_व्यंजक_To_Sen_वृक्ष.first);
            क्रम (माप_प्रकार i = 1; i < senItemps.आकार(); ++i) {
                अगर (senItemps[i]->edgeType() != senItemps[i - 1]->edgeType()) {
                    toRemove.निकालो_back();
                    अवरोध;
                }
            }
        }
        क्रम (स्थिर VNRef<Ast_गाँठ> it : toRemove) m_sen_व्यंजक_To_Sen_Item.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<माप_प्रकार, std::map<माप_प्रकार, std::vector<Ast_गाँठ_व्यंजक*>>>
    getUsedTriggersToTrees(स्थिर std::set<माप_प्रकार>& usedTriggers) {
        std::map<माप_प्रकार, std::map<माप_प्रकार, std::vector<Ast_गाँठ_व्यंजक*>>> usedTrigsToUsingTrees;
        क्रम (स्वतः sen_वृक्ष_Sched : m_sen_वृक्ष_To_Sched) {
            स्थिर Ast_Sen_वृक्ष* स्थिर senTreep = sen_वृक्ष_Sched.first;
            Ast_गाँठ_व्यंजक* स्थिर shedp = sen_वृक्ष_Sched.second;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            क्रम (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                स्थिर माप_प्रकार idx = m_trigKit.senItem2TrigIdx(senItemp);
                अगर (usedTriggers.find(idx) != usedTriggers.end()) {
                    usedTrigsToUsingTrees[idx / TriggerKit::WORD_आकार]
                                         [माप_प्रकार{1} << (idx % TriggerKit::WORD_आकार)]
                                             .धकेलो_back(shedp);
                }
            }
        }
        अगर (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            क्रम (स्थिर स्वतः& triggersToTrees : usedTrigsToUsingTrees) {
                क्रम (स्थिर स्वतः& bitsToTrees : triggersToTrees.second) {
                    स्थिर std::set<स्थिर Ast_गाँठ_व्यंजक*> exprps{bitsToTrees.second.begin(),
                                                              bitsToTrees.second.end()};
                    UASSERT(bitsToTrees.second.आकार() == exprps.आकार(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        वापस usedTrigsToUsingTrees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_वृक्ष* स्थिर senTreep) {
        File_पंक्ति* स्थिर flp = senTreep->fileline();
        अगर (!senTreep->user1p()) {
            AstCFunc* स्थिर funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.लो(senTreep), false);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* स्थिर tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(true);

            AstVar* स्थिर argp = नया AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->findBasicDType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(true);
            argp->direction(VDirection::निवेश);
            funcp->addArgsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_back(funcp, senTreep);
        }
        AstCCall* स्थिर callp = नया AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtypeSetVoid();
        वापस callp;
    }

    व्योम visit(AstCAwait* स्थिर nodep) override {
        अगर (nodep->user1SetOnce()) वापस;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        अगर (स्थिर AstCMethodHard* स्थिर cMethodHardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            अगर (cMethodHardp->method() == VCMethod::SCHED_TRIGGER) {
                AstCCall* स्थिर beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_गाँठ* स्थिर pinp = cMethodHardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, cMethodHardp, "No event description");
                beforeTrigp->addArgsp(VN_AS(pinp, गाँठ_व्यंजक)->clone_वृक्ष(false));

                // Call the before-trigger function before the CAwait
                nodep->add_Here_This_As_अग्र(beforeTrigp->makeStmt());
                m_sen_वृक्ष_To_Sched.emplace_back(nodep->sentreep(), cMethodHardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    व्योम visit(Ast_गाँठ* स्थिर nodep) override { iterateChildren(nodep); }

खुला:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_व्यंजक_Builder& sen_व्यंजक_Builder,
                           स्थिर TriggerKit& trigKit)
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
        क्रम (स्थिर स्वतः& funcToUsedTriggers : m_generatedFuncs) {
            AstCFunc* स्थिर funcp = funcToUsedTriggers.first;
            AstVarScope* स्थिर vscp = VN_AS(funcp->user1p(), VarScope);
            File_पंक्ति* स्थिर flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_वृक्ष* स्थिर senTreep = funcToUsedTriggers.second;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                स्थिर स्वतः emplaceAt
                    = [flp, &trigps, &usedTriggers](Ast_गाँठ_व्यंजक* स्थिर exprp, स्थिर माप_प्रकार pos) {
                          स्थिर माप_प्रकार target_आकार
                              = vlstd::roundUpToMultipleOf<TriggerKit::WORD_आकार>(pos + 1);
                          अगर (trigps.capacity() < target_आकार) trigps.reserve(target_आकार * 2);
                          जबतक (trigps.आकार() < target_आकार) {
                              trigps.धकेलो_back(नया AstConst{flp, AstConst::BitFalse{}});
                          }
                          trigps[pos]->delete_वृक्ष();
                          trigps[pos] = exprp;
                          usedTriggers.जोड़ो(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                क्रम (स्थिर AstSenItem* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), SenItem)) {
                    स्थिर माप_प्रकार idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_व्यंजक_Builder.build(itemp).first, idx);
                    स्वतः iter = m_sen_व्यंजक_To_Sen_Item.find(*get_Sen_Hash_गाँठ(itemp));
                    अगर (iter != m_sen_व्यंजक_To_Sen_Item.end()) {
                        क्रम (AstSenItem* स्थिर additionalItemp : iter->second) {
                            स्थिर माप_प्रकार idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_व्यंजक_Builder.build(additionalItemp).first, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                स्थिर Sen_व्यंजक_Builder::Results results
                    = m_sen_व्यंजक_Builder.getResultsAndClearUpdates();
                क्रम (Ast_गाँठ_Stmt* स्थिर stmtsp : results.m_inits) funcp->addStmtsp(stmtsp);
                क्रम (Ast_गाँठ_Stmt* स्थिर stmtsp : results.m_preUpdates) funcp->addStmtsp(stmtsp);
                funcp->addStmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.सफा();
                क्रम (Ast_गाँठ_Stmt* स्थिर stmtsp : results.m_postUpdates) funcp->addStmtsp(stmtsp);
            }

            स्थिर std::map<माप_प्रकार, std::map<माप_प्रकार, std::vector<Ast_गाँठ_व्यंजक*>>>
                usedTrigsToUsingTrees = getUsedTriggersToTrees(usedTriggers);
            usedTriggers.सफा();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            स्थिर स्वतः getIdx = [flp](AstVarScope* स्थिर scocep, VAccess access, माप_प्रकार idx) {
                वापस नया AstArraySel{flp, नया AstVarRef{flp, scocep, access},
                                       नया AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* स्थिर argpVscp = नया AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->addVarsp(argpVscp);

            // Mark as ready triggered schedulers
            क्रम (स्थिर स्वतः& triggersToTrees : usedTrigsToUsingTrees) {
                स्थिर माप_प्रकार word = triggersToTrees.first;

                क्रम (स्थिर स्वतः& bitsToTrees : triggersToTrees.second) {
                    स्थिर माप_प्रकार bit = bitsToTrees.first;
                    स्थिर स्वतः& schedulers = bitsToTrees.second;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* स्थिर maskConstp = नया AstConst{flp, AstConst::Unsized64{}, bit};
                    AstAnd* स्थिर condp
                        = नया AstAnd{flp, getIdx(vscp, VAccess::READ, word), maskConstp};
                    AstIf* स्थिर ifp = नया AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    क्रम (Ast_गाँठ_व्यंजक* स्थिर schedp : schedulers) {
                        AstCMethodHard* स्थिर callp = नया AstCMethodHard{
                            flp, schedp->clone_वृक्ष(false), VCMethod::SCHED_READY};
                        callp->dtypeSetVoid();
                        callp->addPinsp(नया AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->addThensp(callp->makeStmt());
                    }
                    funcp->addStmtsp(ifp);
                }
            }

            AstVarScope* स्थिर vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            क्रम (स्थिर स्वतः& triggersToTrees : usedTrigsToUsingTrees) {
                स्थिर माप_प्रकार word = triggersToTrees.first;
                funcp->addStmtsp(नया AstAssign{flp, getIdx(vscAccp, VAccess::WRITE, word),
                                               नया AstOr{flp, getIdx(vscAccp, VAccess::READ, word),
                                                         getIdx(vscp, VAccess::READ, word)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = शेष;
};

व्योम beforeTrigVisitor(AstNetlist* netlistp, Sen_व्यंजक_Builder& sen_व्यंजक_Builder,
                       स्थिर TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_व्यंजक_Builder, trigKit};
}

}  // namespace V3Sched
