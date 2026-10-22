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

AstVarScope* newLocal(AstCFunc* funcp, Ast_गाठ_DType* dtypep, स्थिरांक std::string& name) {
    File_ओळ* स्थिरांक flp = funcp->fileline();
    AstScope* स्थिरांक scopep = funcp->scopep();
    AstVar* स्थिरांक varp = नवीन AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(सत्य);
    varp->noReset(सत्य);
    funcp->addVarsp(varp);
    AstVarScope* स्थिरांक vscp = नवीन AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    परत vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() स्थिरांक {
    UASSERT(m_n_Pre_शब्द, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* स्थिरांक netlistp = v3Global.rootp();
    File_ओळ* स्थिरांक flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाठ_DType* स्थिरांक u32DTypep = netlistp->findUInt32DType();
    Ast_गाठ_DType* स्थिरांक strDtypep = netlistp->findStringDType();

    // Dumping function always slow
    स्थिरांक std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* स्थिरांक funcp = util::makeSubFunction(netlistp, name, सत्य);
    funcp->isStatic(सत्य);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* स्थिरांक eVscp = newArgument(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* स्थिरांक tVscp = newArgument(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    स्थिरांक स्वतः rd = [flp](AstVarScope* vp) { परत नवीन AstVarRef{flp, vp, VAccess::READ}; };
    स्थिरांक स्वतः wr = [flp](AstVarScope* vp) { परत नवीन AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* स्थिरांक vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* स्थिरांक iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->addStmtsp(util::setVar(iVscp, 0));
        // Add loop
        AstLoop* स्थिरांक loopp = नवीन AstLoop{flp};
        funcp->addStmtsp(loopp);
        // Loop body
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = नवीन AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = नवीन AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक limp = नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, 32, m_n_Vec_शब्द};
        loopp->addStmtsp(नवीन AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(iVscp));
        loopp->addStmtsp(नवीन AstLoopTest{flp, loopp, नवीन AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* स्थिरांक callp = नवीन AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(vVscp));
        callp->addArgsp(rd(tVscp));
        funcp->addStmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* स्थिरांक pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* स्थिरांक jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->addStmtsp(util::setVar(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* स्थिरांक loopp = नवीन AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = नवीन AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = नवीन AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक limp = नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, 32, m_n_Pre_शब्द};
            loopp->addStmtsp(नवीन AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(नवीन AstLoopTest{flp, loopp, नवीन AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* स्थिरांक loopp = नवीन AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = नवीन AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = नवीन AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक limp = नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, 32, m_n_Vec_शब्द};
            loopp->addStmtsp(नवीन AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(नवीन AstLoopTest{flp, loopp, नवीन AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* स्थिरांक callp = नवीन AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(pVscp));
        callp->addArgsp(
            नवीन AstConcatN{flp, rd(tVscp), नवीन AstConst{flp, AstConst::String{}, " pre"}});
        funcp->addStmtsp(callp->makeStmt());
    }

    परत funcp;
}

AstCFunc* TriggerKit::createAnySetFunc(AstUnpackArrayDType* स्थिरांक dtypep) स्थिरांक {
    AstNetlist* स्थिरांक netlistp = v3Global.rootp();
    File_ओळ* स्थिरांक flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाठ_DType* स्थिरांक u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* स्थिरांक funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(सत्य);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* स्थिरांक iVscp = newArgument(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* स्थिरांक nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    स्थिरांक स्वतः rd = [flp](AstVarScope* vp) { परत नवीन AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* स्थिरांक loopp = नवीन AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);
    funcp->addStmtsp(नवीन AstCReturn{flp, नवीन AstConst{flp, AstConst::BitFalse{}}});

    // Loop body
    स्थिरांक uint32_t n_शब्द = dtypep->elementsConst();
    Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = नवीन AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_गाठ_Stmt* स्थिरांक thenp = नवीन AstCReturn{flp, नवीन AstConst{flp, AstConst::BitTrue{}}};
    Ast_गाठ_अभिव्यक्ती* स्थिरांक limp = नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, 32, n_शब्द};
    loopp->addStmtsp(नवीन AstIf{flp, condp, thenp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(नवीन AstLoopTest{flp, loopp, नवीन AstLt{flp, rd(nVscp), limp}});

    // Done
    परत funcp;
}
AstCFunc* TriggerKit::createClearFunc() स्थिरांक {
    AstNetlist* स्थिरांक netlistp = v3Global.rootp();
    File_ओळ* स्थिरांक flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाठ_DType* स्थिरांक u32DTypep = netlistp->findUInt32DType();

    // Create function
    AstCFunc* स्थिरांक funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(सत्य);

    // Add arguments
    AstVarScope* स्थिरांक oVscp = newArgument(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* स्थिरांक nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    स्थिरांक स्वतः rd = [flp](AstVarScope* vp) { परत नवीन AstVarRef{flp, vp, VAccess::READ}; };
    स्थिरांक स्वतः wr = [flp](AstVarScope* vp) { परत नवीन AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* स्थिरांक loopp = नवीन AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = नवीन AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = नवीन AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
    Ast_गाठ_अभिव्यक्ती* स्थिरांक limp = नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, 32, m_n_Vec_शब्द};
    loopp->addStmtsp(नवीन AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(नवीन AstLoopTest{flp, loopp, नवीन AstLt{flp, rd(nVscp), limp}});

    // Done
    परत funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* स्थिरांक oDtypep,
                                       AstUnpackArrayDType* स्थिरांक iDtypep) स्थिरांक {
    AstNetlist* स्थिरांक netlistp = v3Global.rootp();
    File_ओळ* स्थिरांक flp = netlistp->शीर्ष_Scopep()->fileline();
    Ast_गाठ_DType* स्थिरांक u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* स्थिरांक funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(सत्य);

    // Add arguments
    AstVarScope* स्थिरांक oVscp = newArgument(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* स्थिरांक iVscp = newArgument(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* स्थिरांक nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    स्थिरांक स्वतः rd = [flp](AstVarScope* vp) { परत नवीन AstVarRef{flp, vp, VAccess::READ}; };
    स्थिरांक स्वतः wr = [flp](AstVarScope* vp) { परत नवीन AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* स्थिरांक loopp = नवीन AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = नवीन AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_गाठ_अभिव्यक्ती* स्थिरांक oWordp = नवीन AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_गाठ_अभिव्यक्ती* स्थिरांक iWordp = नवीन AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = नवीन AstOr{flp, oWordp, iWordp};
    AstConst* स्थिरांक outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* स्थिरांक आदान_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_गाठ_अभिव्यक्ती* स्थिरांक limp = outputRangeLeftp->अंक().toSInt() < आदान_Range_Leftp->अंक().toSInt()
                                  ? outputRangeLeftp->clone_वृक्ष_Pure(असत्य)
                                  : आदान_Range_Leftp->clone_वृक्ष_Pure(असत्य);
    loopp->addStmtsp(नवीन AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(नवीन AstLoopTest{flp, loopp, नवीन AstLte{flp, rd(nVscp), limp}});

    // Done
    परत funcp;
}

Ast_गाठ_अभिव्यक्ती* TriggerKit::newAnySetCall(AstVarScope* स्थिरांक vscp) स्थिरांक {
    File_ओळ* स्थिरांक flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    जर (!m_n_Vec_शब्द) परत नवीन AstConst{flp, AstConst::BitFalse{}};

    AstCFunc* funcp = nullptr;
    जर (vscp->dtypep() == m_trigVecDTypep) {
        जर (!m_anySetVecp) m_anySetVecp = createAnySetFunc(m_trigVecDTypep);
        funcp = m_anySetVecp;
    } अन्यथा जर (vscp->dtypep() == m_trigExtDTypep) {
        जर (!m_anySetExtp) m_anySetExtp = createAnySetFunc(m_trigExtDTypep);
        funcp = m_anySetExtp;
    } अन्यथा {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* स्थिरांक callp = नवीन AstCCall{flp, funcp};
    callp->addArgsp(नवीन AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetBit();
    परत callp;
}
Ast_गाठ_Stmt* TriggerKit::newClearCall(AstVarScope* स्थिरांक vscp) स्थिरांक {
    जर (!m_n_Vec_शब्द) परत nullptr;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    जर (!m_clearp) m_clearp = createClearFunc();
    File_ओळ* स्थिरांक flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* स्थिरांक callp = नवीन AstCCall{flp, m_clearp};
    callp->addArgsp(नवीन AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetVoid();
    परत callp->makeStmt();
}
Ast_गाठ_Stmt* TriggerKit::newOrIntoCall(AstVarScope* स्थिरांक oVscp, AstVarScope* स्थिरांक iVscp) स्थिरांक {
    जर (!m_n_Vec_शब्द) परत nullptr;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    स्थिरांक माप_प्रकार mask
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[mask];
    जर (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_ओळ* स्थिरांक flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* स्थिरांक callp = नवीन AstCCall{flp, funcp};
    callp->addArgsp(नवीन AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->addArgsp(नवीन AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtypeSetVoid();
    परत callp->makeStmt();
}

Ast_गाठ_Stmt* TriggerKit::newCompBaseCall() स्थिरांक {
    जर (!m_n_Vec_शब्द) परत nullptr;
    File_ओळ* स्थिरांक flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* स्थिरांक callp = नवीन AstCCall{flp, m_compVecp};
    callp->dtypeSetVoid();
    परत callp->makeStmt();
}

Ast_गाठ_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) स्थिरांक {
    जर (!m_n_Pre_शब्द) परत nullptr;
    File_ओळ* स्थिरांक flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* स्थिरांक callp = नवीन AstCCall{flp, m_compExtp};
    callp->addArgsp(नवीन AstVarRef{flp, vscp, VAccess::READ});
    callp->dtypeSetVoid();
    परत callp->makeStmt();
}

Ast_गाठ_Stmt* TriggerKit::newDumpCall(AstVarScope* स्थिरांक vscp, स्थिरांक std::string& tag,
                                     तार्किक debugOnly) स्थिरांक {
    जर (!m_n_Vec_शब्द) परत nullptr;
    AstCFunc* funcp = nullptr;
    जर (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } अन्यथा जर (vscp->dtypep() == m_trigExtDTypep) {
        जर (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } अन्यथा {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_ओळ* स्थिरांक flp = v3Global.rootp()->शीर्ष_Scopep()->fileline();
    AstCCall* स्थिरांक callp = नवीन AstCCall{flp, funcp};
    callp->addArgsp(नवीन AstVarRef{flp, vscp, VAccess::READ});
    callp->addArgsp(नवीन AstConst{flp, AstConst::String{}, tag});
    callp->dtypeSetVoid();
    AstCStmt* स्थिरांक cstmtp = नवीन AstCStmt{flp};
    cstmtp->add("#ifdef VL_DEBUG\n");
    जर (debugOnly) {
        cstmtp->add("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->add(callp->makeStmt());
        cstmtp->add("}\n");
    } अन्यथा {
        cstmtp->add(callp->makeStmt());
    }
    cstmtp->add("#endif");
    परत cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(स्थिरांक std::string& name) स्थिरांक {
    जर (!m_n_Vec_शब्द) परत nullptr;
    परत v3Global.rootp()->शीर्ष_Scopep()->createTemp("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extraTriggerIndex(uint32_t index) स्थिरांक {
    UASSERT(index <= m_n_Extra_शब्द * WORD_SIZE, "Invalid external trigger index");
    परत index + m_n_Sense_शब्द * WORD_SIZE;
}

std::set<uint32_t> TriggerKit::triggerIndices(स्थिरांक Ast_Sen_वृक्ष* senTreep, तार्किक pre) स्थिरांक {
    std::set<uint32_t> indices;
    साठी (स्थिरांक AstSenItem* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), SenItem)) {
        uint32_t index = m_senItem2TrigIdx.at(*sip);
        जर (pre) {
            UASSERT_OBJ(index < m_n_Pre_शब्द * WORD_SIZE, sip, "Not a 'pre' trigger");
            index += m_n_Vec_शब्द * WORD_SIZE;
        }
        indices.घाला(index);
    }
    परत indices;
}

Ast_गाठ_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) स्थिरांक {
    index = extraTriggerIndex(index);
    स्थिरांक uint32_t wordIndex = index / WORD_SIZE;
    स्थिरांक uint32_t bitIndex = index % WORD_SIZE;
    File_ओळ* स्थिरांक flp = vscp->fileline();
    AstVarRef* स्थिरांक refp = नवीन AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_गाठ_अभिव्यक्ती* स्थिरांक wordp = नवीन AstArraySel{flp, refp, static_cast<पूर्णांक>(wordIndex)};
    Ast_गाठ_अभिव्यक्ती* स्थिरांक trigLhsp = नवीन AstSel{flp, wordp, static_cast<पूर्णांक>(bitIndex), 1};
    Ast_गाठ_अभिव्यक्ती* स्थिरांक trigRhsp = नवीन AstVarRef{flp, vscp, VAccess::READ};
    परत नवीन AstAssign{flp, trigLhsp, trigRhsp};
}

शून्य TriggerKit::addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) स्थिरांक {
    File_ओळ* स्थिरांक flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_गाठ* स्थिरांक setp = newExtraTriggerAssignment(vscp, index);
    setp->addNext(नवीन AstAssign{flp, नवीन AstVarRef{flp, vscp, VAccess::WRITE},
                                नवीन AstConst{flp, AstConst::BitFalse{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
शून्य TriggerKit::add_मूल्य_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t index) स्थिरांक {
    index += m_n_Sense_शब्द * WORD_SIZE;
    स्थिरांक uint32_t wordIndex = index / WORD_SIZE;
    स्थिरांक uint32_t bitIndex = index % WORD_SIZE;

    Ast_शीर्ष_Scope* स्थिरांक शीर्ष_Scopep = netlistp->शीर्ष_Scopep();
    File_ओळ* स्थिरांक flp = netlistp->fileline();
    Ast_गाठ_DType* स्थिरांक dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    जर (स्थिरांक AstBasicDType* स्थिरांक bdtypep = VN_CAST(dtypep, BasicDType)) {
        जर (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            परत;
        }
    } अन्यथा जर (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, गाठ_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        परत;
    }

    स्थिरांक स्वतः rdInst = [flp](AstVarScope* vp) { परत नवीन AstVarRef{flp, vp, VAccess::READ}; };
    स्थिरांक स्वतः wrPrev = [flp](AstVarScope* vp) { परत नवीन AstVarRef{flp, vp, VAccess::WRITE}; };
    स्थिरांक स्वतः rdPrev = [flp](AstVarScope* vp) { परत नवीन AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    स्थिरांक std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* स्थिरांक prevVscp = शीर्ष_Scopep->createTemp(prevName, instVscp->dtypep());

    // Initialize prev = inst
    जर (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* स्थिरांक cmhp = नवीन AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        initFuncp->addStmtsp(cmhp->makeStmt());
    } अन्यथा {
        initFuncp->addStmtsp(नवीन AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_गाठ_अभिव्यक्ती* neqp;
    जर (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* स्थिरांक cmhp
            = नवीन AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtypeSetBit();
        neqp = cmhp;
    } अन्यथा {
        neqp = नवीन AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_गाठ* updp;
    जर (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* स्थिरांक cmhp = नवीन AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        updp = cmhp->makeStmt();
    } अन्यथा {
        updp = नवीन AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* स्थिरांक refp = नवीन AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_गाठ_अभिव्यक्ती* स्थिरांक wordp = नवीन AstArraySel{flp, refp, static_cast<पूर्णांक>(wordIndex)};
    Ast_गाठ_अभिव्यक्ती* स्थिरांक trigLhsp = नवीन AstSel{flp, wordp, static_cast<पूर्णांक>(bitIndex), 1};
    Ast_गाठ* स्थिरांक setp = नवीन AstAssign{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->addNext(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(स्थिरांक std::string& name, तार्किक slow, uint32_t n_Sense_शब्द,
                       uint32_t n_Extra_शब्द, uint32_t n_Pre_शब्द,
                       std::unordered_map<VNRef<स्थिरांक AstSenItem>, माप_प्रकार> senItem2TrigIdx,
                       स्थिरांक std::vector<स्थिरांक AstSenItem*>& senItemps, तार्किक useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_शब्द{n_Sense_शब्द}
    , m_n_Extra_शब्द{n_Extra_शब्द}
    , m_n_Pre_शब्द{n_Pre_शब्द}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trigIdx2SenItem{senItemps} {
    // If no triggers, we don't need to generate anything
    जर (!m_n_Vec_शब्द) परत;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trigIdx2SenItem.resize((m_n_Vec_शब्द + m_n_Pre_शब्द) * WORD_SIZE);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_शब्द * WORD_SIZE,
              m_trigIdx2SenItem.begin() + m_n_Vec_शब्द * WORD_SIZE);
    AstNetlist* स्थिरांक netlistp = v3Global.rootp();
    AstScope* स्थिरांक scopep = netlistp->शीर्ष_Scopep()->scopep();
    File_ओळ* स्थिरांक flp = scopep->fileline();
    // Data type of a single trigger word
    m_wordDTypep = netlistp->findBitDType(WORD_SIZE, WORD_SIZE, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* स्थिरांक rp = नवीन AstRange{flp, static_cast<पूर्णांक>(m_n_Vec_शब्द - 1), 0};
    m_trigVecDTypep = नवीन AstUnpackArrayDType{flp, m_wordDTypep, rp};
    netlistp->typeTablep()->addTypesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    जर (m_n_Pre_शब्द) {
        AstRange* स्थिरांक ep = नवीन AstRange{flp, static_cast<पूर्णांक>(m_n_Vec_शब्द + m_n_Pre_शब्द - 1), 0};
        m_trigExtDTypep = नवीन AstUnpackArrayDType{flp, m_wordDTypep, ep};
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
    जर (useAcc) {
        m_vscAccp
            = netlistp->शीर्ष_Scopep()->createTemp("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->isInternal(सत्य);
    }
}

AstAssign* TriggerKit::createSenTrigVecAssignment(AstVarScope* स्थिरांक target,
                                                  std::vector<Ast_गाठ_अभिव्यक्ती*>& trigps) {
    File_ओळ* स्थिरांक flp = target->fileline();
    AstAssign* trigStmtsp = nullptr;
    // Assign sense triggers vector one word at a time
    साठी (माप_प्रकार i = 0; i < trigps.size(); i += WORD_SIZE) {
        // Concatenate all bits in this trigger word using a balanced
        साठी (uint32_t level = 0; level < WORD_SIZE_LOG2; ++level) {
            स्थिरांक uint32_t stride = 1 << level;
            साठी (uint32_t j = 0; j < WORD_SIZE; j += 2 * stride) {
                trigps[i + j] = नवीन AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = nullptr;
            }
        }

        // Set the whole word in the trigger vector
        स्थिरांक पूर्णांक wordIndex = static_cast<पूर्णांक>(i / WORD_SIZE);
        AstArraySel* स्थिरांक aselp
            = नवीन AstArraySel{flp, नवीन AstVarRef{flp, target, VAccess::WRITE}, wordIndex};
        trigStmtsp = Ast_गाठ::addNext(trigStmtsp, नवीन AstAssign{flp, aselp, trigps[i]});
    }
    परत trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* स्थिरांक initFuncp,  //
                              Sen_अभिव्यक्ती_Builder& sen_अभिव्यक्ती_Builder,  //
                              स्थिरांक std::vector<स्थिरांक Ast_Sen_वृक्ष*>& preTreeps,  //
                              स्थिरांक std::vector<स्थिरांक Ast_Sen_वृक्ष*>& senTreeps,  //
                              स्थिरांक string& name,  //
                              स्थिरांक ExtraTriggers& extraTriggers,  //
                              तार्किक slow,  //
                              तार्किक useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::vector<स्थिरांक AstSenItem*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<स्थिरांक AstSenItem>, माप_प्रकार> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    साठी (स्थिरांक Ast_Sen_वृक्ष* स्थिरांक senTreep : preTreeps) {
        साठी (स्थिरांक AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            स्थिरांक स्वतः pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            जर (pair.second) senItemps.ढकला_back(itemp);
        }
    }
    स्थिरांक uint32_t n_Pre_Sen_वस्तुसंच = senItemps.size();
    V3Stats::addStat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_वस्तुसंच);
    // Number of pre triggers, rounded up to a full word.
    स्थिरांक uint32_t nPreTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    स्थिरांक uint32_t n_Pre_शब्द = nPreTriggers / WORD_SIZE;

    // Process the rest of the trees
    साठी (स्थिरांक Ast_Sen_वृक्ष* स्थिरांक senTreep : senTreeps) {
        साठी (स्थिरांक AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            स्थिरांक स्वतः pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            जर (pair.second) senItemps.ढकला_back(itemp);
        }
    }
    स्थिरांक uint32_t n_Sen_वस्तुसंच = senItemps.size() - nPreTriggers;
    V3Stats::addStat("Scheduling, '" + name + "' sense triggers", n_Sen_वस्तुसंच + n_Pre_Sen_वस्तुसंच);
    // Number of sense triggers, rounded up to a full word
    स्थिरांक uint32_t nSenseTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    स्थिरांक uint32_t n_Sense_शब्द = nSenseTriggers / WORD_SIZE;

    // Allocate space for the extra triggers
    V3Stats::addStat("Scheduling, '" + name + "' extra triggers", extraTriggers.size());
    // Number of extra triggers, rounded up to a full word.
    स्थिरांक uint32_t nExtraTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(extraTriggers.size());
    स्थिरांक uint32_t n_Extra_शब्द = nExtraTriggers / WORD_SIZE;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_शब्द, n_Extra_शब्द, n_Pre_शब्द, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    जर (!kit.m_n_Vec_शब्द) परत kit;

    File_ओळ* स्थिरांक flp = netlistp->शीर्ष_Scopep()->fileline();

    // Creates read/write reference
    स्थिरांक स्वतः rd = [flp](AstVarScope* vp) { परत नवीन AstVarRef{flp, vp, VAccess::READ}; };
    स्थिरांक स्वतः wr = [flp](AstVarScope* vp) { परत नवीन AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* स्थिरांक dumpTrgp
        = newArgument(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* स्थिरांक dumpTagp
        = newArgument(kit.m_dumpp, netlistp->findStringDType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* स्थिरांक ifp = नवीन AstIf{flp, नवीन AstLogNot{flp, kit.newAnySetCall(dumpTrgp)}};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* स्थिरांक cstmtp = नवीन AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    स्थिरांक स्वतः addDebug = [&](uint32_t index, स्थिरांक string& text) {
        स्थिरांक पूर्णांक wrdIndex = static_cast<पूर्णांक>(index / WORD_SIZE);
        स्थिरांक पूर्णांक bitIndex = static_cast<पूर्णांक>(index % WORD_SIZE);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक aselp = नवीन AstArraySel{flp, rd(dumpTrgp), wrdIndex};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = नवीन AstSel{flp, aselp, bitIndex, 1};
        AstIf* स्थिरांक ifp = नवीन AstIf{flp, condp};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* स्थिरांक cstmtp = नवीन AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"' region trigger index " + std::to_string(index) + " is active: " + text
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::vector<Ast_गाठ_अभिव्यक्ती*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_गाठ_Stmt* initialTrigsp = nullptr;
    साठी (माप_प्रकार i = 0; i < senItemps.size(); ++i) {
        स्थिरांक AstSenItem* स्थिरांक senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        जर (!senItemp) {
            trigps.emplace_back(नवीन AstConst{flp, AstConst::BitFalse{}});
            सुरूठेवा;
        }

        // Create the trigger computation expression
        स्थिरांक स्वतः& pair = sen_अभिव्यक्ती_Builder.build(senItemp);
        trigps.emplace_back(pair.first);

        // Add initialization time trigger
        जर (pair.second || v3Global.opt.xInitialEdge()) {
            स्थिरांक पूर्णांक wrdIndex = static_cast<पूर्णांक>(i / WORD_SIZE);
            स्थिरांक पूर्णांक bitIndex = static_cast<पूर्णांक>(i % WORD_SIZE);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक wordp = नवीन AstArraySel{flp, wr(kit.m_vscp), wrdIndex};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = नवीन AstSel{flp, wordp, bitIndex, 1};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = नवीन AstConst{flp, AstConst::BitTrue{}};
            जर (useAcc) {
                initFuncp->addStmtsp(नवीन AstAssign{flp, lhsp, rhsp});
            } अन्यथा {
                initialTrigsp = Ast_गाठ::addNext(initialTrigsp, नवीन AstAssign{flp, lhsp, rhsp});
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

    AstAssign* स्थिरांक trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    साठी (अचिन्हित i = 0; i < extraTriggers.size(); ++i) {
        addDebug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    स्थिरांक Sen_अभिव्यक्ती_Builder::Results senResults = sen_अभिव्यक्ती_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    साठी (Ast_गाठ_Stmt* स्थिरांक nodep : senResults.m_inits) initFuncp->addStmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_शीर्ष_Scope* स्थिरांक शीर्ष_Scopep = netlistp->शीर्ष_Scopep();
    {
        AstCFunc* स्थिरांक fp = kit.m_compVecp;
        // Trigger computation
        साठी (Ast_गाठ_Stmt* स्थिरांक nodep : senResults.m_preUpdates) fp->addStmtsp(nodep);
        fp->addStmtsp(trigStmtsp);
        साठी (Ast_गाठ_Stmt* स्थिरांक nodep : senResults.m_postUpdates) fp->addStmtsp(nodep);
        // Add the initialization time triggers
        जर (initialTrigsp) {
            AstVarScope* स्थिरांक initVscp = शीर्ष_Scopep->createTemp("__V" + name + "DidInit", 1);
            AstIf* स्थिरांक ifp = नवीन AstIf{flp, नवीन AstNot{flp, rd(initVscp)}};
            fp->addStmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->addThensp(util::setVar(initVscp, 1));
            ifp->addThensp(initialTrigsp);
        }
        util::splitCheck(fp);
    };
    // If there are 'pre' triggers, compute them
    जर (kit.m_n_Pre_शब्द) {
        AstCFunc* स्थिरांक fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* स्थिरांक latchedp
            = newArgument(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* स्थिरांक nVscp = शीर्ष_Scopep->createTemp("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(सत्य);
        // Add a loop to compute the pre words
        AstLoop* स्थिरांक loopp = नवीन AstLoop{flp};
        fp->addStmtsp(util::setVar(nVscp, 0));
        fp->addStmtsp(loopp);
        // Loop body
        Ast_गाठ_अभिव्यक्ती* स्थिरांक offsetp = नवीन AstConst{flp, kit.m_n_Vec_शब्द};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lIdxp = नवीन AstAdd{flp, rd(nVscp), offsetp};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = नवीन AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक aWordp = नवीन AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक bWordp = नवीन AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = नवीन AstAnd{flp, aWordp, नवीन AstNot{flp, bWordp}};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक limp = नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, 32, n_Pre_शब्द};
        loopp->addStmtsp(नवीन AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(nVscp));
        loopp->addStmtsp(नवीन AstLoopTest{flp, loopp, नवीन AstLt{flp, rd(nVscp), limp}});
        util::splitCheck(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    जर (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlinkFrBackWithNext()->delete_वृक्ष();
    // Done with the trigger dump function, split as might be large
    util::splitCheck(kit.m_dumpp);

    परत kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
वर्ग AwaitBeforeTrigVisitor final : सार्वजनिक VNVisitor {
    स्थिरांक VNUser1InUse m_user1InUse;

    /**
     * AstCAwait::user1()       ->  bool.           True if node has been visited
     * AstSenTree::user1p()     ->  AstCFunc*.      Function that has to be called before awaiting
     *                                              for CAwait pointing to this SenTree
     * AstCFunc::user1p()       ->  AstVarScope*    Function's local temporary extended trigger
     *                                              vector variable scope
     */

    // Netlist - needed for using util::makeSubFunction()
    AstNetlist* स्थिरांक m_netlistp;
    // Trigger kit - for accessing trigger vectors and mapping senItems to thier indexes
    स्थिरांक TriggerKit& m_trigKit;
    // Expression builder - for building expressions from SenItems
    Sen_अभिव्यक्ती_Builder& m_sen_अभिव्यक्ती_Builder;
    // Generator of unique names for before-trigger function
    V3UniqueNames m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::vector<std::pair<AstCFunc*, Ast_Sen_वृक्ष*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::vector<std::pair<Ast_Sen_वृक्ष*, Ast_गाठ_अभिव्यक्ती*>> m_sen_वृक्ष_To_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_गाठ>, std::vector<AstSenItem*>> m_sen_अभिव्यक्ती_To_Sen_Item;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    स्थिर Ast_गाठ* get_Sen_Hash_गाठ(स्थिरांक AstSenItem* स्थिरांक nodep) {
        जर (AstVarRef* स्थिरांक varRefp = VN_CAST(nodep->sensp(), VarRef)) परत varRefp;
        परत nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    शून्य fill_Sen_अभिव्यक्ती_To_Sen_Item() {
        साठी (स्वतः sen_वृक्ष_Sched : m_sen_वृक्ष_To_Sched) {
            Ast_Sen_वृक्ष* स्थिरांक senTreep = sen_वृक्ष_Sched.first;

            साठी (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                स्थिरांक VEdgeType edge = senItemp->edgeType();
                जर (edge.anEdge() || edge == VEdgeType::ET_CHANGED
                    || edge == VEdgeType::ET_HYBRID) {
                    m_sen_अभिव्यक्ती_To_Sen_Item[*get_Sen_Hash_गाठ(senItemp)].ढकला_back(senItemp);
                }
            }
        }

        std::vector<VNRef<Ast_गाठ>> toRemove;
        साठी (स्थिरांक स्वतः& sen_अभिव्यक्ती_To_Sen_वृक्ष : m_sen_अभिव्यक्ती_To_Sen_Item) {
            std::vector<AstSenItem*> senItemps = sen_अभिव्यक्ती_To_Sen_वृक्ष.second;
            toRemove.ढकला_back(sen_अभिव्यक्ती_To_Sen_वृक्ष.first);
            साठी (माप_प्रकार i = 1; i < senItemps.size(); ++i) {
                जर (senItemps[i]->edgeType() != senItemps[i - 1]->edgeType()) {
                    toRemove.काढा_back();
                    थांबा;
                }
            }
        }
        साठी (स्थिरांक VNRef<Ast_गाठ> it : toRemove) m_sen_अभिव्यक्ती_To_Sen_Item.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<माप_प्रकार, std::map<माप_प्रकार, std::vector<Ast_गाठ_अभिव्यक्ती*>>>
    getUsedTriggersToTrees(स्थिरांक std::set<माप_प्रकार>& usedTriggers) {
        std::map<माप_प्रकार, std::map<माप_प्रकार, std::vector<Ast_गाठ_अभिव्यक्ती*>>> usedTrigsToUsingTrees;
        साठी (स्वतः sen_वृक्ष_Sched : m_sen_वृक्ष_To_Sched) {
            स्थिरांक Ast_Sen_वृक्ष* स्थिरांक senTreep = sen_वृक्ष_Sched.first;
            Ast_गाठ_अभिव्यक्ती* स्थिरांक shedp = sen_वृक्ष_Sched.second;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            साठी (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                स्थिरांक माप_प्रकार idx = m_trigKit.senItem2TrigIdx(senItemp);
                जर (usedTriggers.find(idx) != usedTriggers.end()) {
                    usedTrigsToUsingTrees[idx / TriggerKit::WORD_SIZE]
                                         [माप_प्रकार{1} << (idx % TriggerKit::WORD_SIZE)]
                                             .ढकला_back(shedp);
                }
            }
        }
        जर (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            साठी (स्थिरांक स्वतः& triggersToTrees : usedTrigsToUsingTrees) {
                साठी (स्थिरांक स्वतः& bitsToTrees : triggersToTrees.second) {
                    स्थिरांक std::set<स्थिरांक Ast_गाठ_अभिव्यक्ती*> exprps{bitsToTrees.second.begin(),
                                                              bitsToTrees.second.end()};
                    UASSERT(bitsToTrees.second.size() == exprps.size(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        परत usedTrigsToUsingTrees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_वृक्ष* स्थिरांक senTreep) {
        File_ओळ* स्थिरांक flp = senTreep->fileline();
        जर (!senTreep->user1p()) {
            AstCFunc* स्थिरांक funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), असत्य);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* स्थिरांक tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(सत्य);

            AstVar* स्थिरांक argp = नवीन AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->findBasicDType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(सत्य);
            argp->direction(VDirection::आदान);
            funcp->addArgsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_back(funcp, senTreep);
        }
        AstCCall* स्थिरांक callp = नवीन AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtypeSetVoid();
        परत callp;
    }

    शून्य visit(AstCAwait* स्थिरांक nodep) override {
        जर (nodep->user1SetOnce()) परत;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        जर (स्थिरांक AstCMethodHard* स्थिरांक cMethodHardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            जर (cMethodHardp->method() == VCMethod::SCHED_TRIGGER) {
                AstCCall* स्थिरांक beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_गाठ* स्थिरांक pinp = cMethodHardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, cMethodHardp, "No event description");
                beforeTrigp->addArgsp(VN_AS(pinp, गाठ_अभिव्यक्ती)->clone_वृक्ष(असत्य));

                // Call the before-trigger function before the CAwait
                nodep->addHereThisAsNext(beforeTrigp->makeStmt());
                m_sen_वृक्ष_To_Sched.emplace_back(nodep->sentreep(), cMethodHardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    शून्य visit(Ast_गाठ* स्थिरांक nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_अभिव्यक्ती_Builder& sen_अभिव्यक्ती_Builder,
                           स्थिरांक TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_अभिव्यक्ती_Builder{sen_अभिव्यक्ती_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_अभिव्यक्ती_To_Sen_Item();

        std::vector<Ast_गाठ_अभिव्यक्ती*> trigps;
        std::set<माप_प्रकार> usedTriggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        साठी (स्थिरांक स्वतः& funcToUsedTriggers : m_generatedFuncs) {
            AstCFunc* स्थिरांक funcp = funcToUsedTriggers.first;
            AstVarScope* स्थिरांक vscp = VN_AS(funcp->user1p(), VarScope);
            File_ओळ* स्थिरांक flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_वृक्ष* स्थिरांक senTreep = funcToUsedTriggers.second;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                स्थिरांक स्वतः emplaceAt
                    = [flp, &trigps, &usedTriggers](Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp, स्थिरांक माप_प्रकार pos) {
                          स्थिरांक माप_प्रकार targetSize
                              = vlstd::roundUpToMultipleOf<TriggerKit::WORD_SIZE>(pos + 1);
                          जर (trigps.capacity() < targetSize) trigps.reserve(targetSize * 2);
                          पर्यंत (trigps.size() < targetSize) {
                              trigps.ढकला_back(नवीन AstConst{flp, AstConst::BitFalse{}});
                          }
                          trigps[pos]->delete_वृक्ष();
                          trigps[pos] = exprp;
                          usedTriggers.घाला(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                साठी (स्थिरांक AstSenItem* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), SenItem)) {
                    स्थिरांक माप_प्रकार idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_अभिव्यक्ती_Builder.build(itemp).first, idx);
                    स्वतः iter = m_sen_अभिव्यक्ती_To_Sen_Item.find(*get_Sen_Hash_गाठ(itemp));
                    जर (iter != m_sen_अभिव्यक्ती_To_Sen_Item.end()) {
                        साठी (AstSenItem* स्थिरांक additionalItemp : iter->second) {
                            स्थिरांक माप_प्रकार idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_अभिव्यक्ती_Builder.build(additionalItemp).first, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                स्थिरांक Sen_अभिव्यक्ती_Builder::Results results
                    = m_sen_अभिव्यक्ती_Builder.getResultsAndClearUpdates();
                साठी (Ast_गाठ_Stmt* स्थिरांक stmtsp : results.m_inits) funcp->addStmtsp(stmtsp);
                साठी (Ast_गाठ_Stmt* स्थिरांक stmtsp : results.m_preUpdates) funcp->addStmtsp(stmtsp);
                funcp->addStmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                साठी (Ast_गाठ_Stmt* स्थिरांक stmtsp : results.m_postUpdates) funcp->addStmtsp(stmtsp);
            }

            स्थिरांक std::map<माप_प्रकार, std::map<माप_प्रकार, std::vector<Ast_गाठ_अभिव्यक्ती*>>>
                usedTrigsToUsingTrees = getUsedTriggersToTrees(usedTriggers);
            usedTriggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            स्थिरांक स्वतः getIdx = [flp](AstVarScope* स्थिरांक scocep, VAccess access, माप_प्रकार idx) {
                परत नवीन AstArraySel{flp, नवीन AstVarRef{flp, scocep, access},
                                       नवीन AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* स्थिरांक argpVscp = नवीन AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->addVarsp(argpVscp);

            // Mark as ready triggered schedulers
            साठी (स्थिरांक स्वतः& triggersToTrees : usedTrigsToUsingTrees) {
                स्थिरांक माप_प्रकार word = triggersToTrees.first;

                साठी (स्थिरांक स्वतः& bitsToTrees : triggersToTrees.second) {
                    स्थिरांक माप_प्रकार bit = bitsToTrees.first;
                    स्थिरांक स्वतः& schedulers = bitsToTrees.second;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* स्थिरांक maskConstp = नवीन AstConst{flp, AstConst::Unsized64{}, bit};
                    AstAnd* स्थिरांक condp
                        = नवीन AstAnd{flp, getIdx(vscp, VAccess::READ, word), maskConstp};
                    AstIf* स्थिरांक ifp = नवीन AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक schedp : schedulers) {
                        AstCMethodHard* स्थिरांक callp = नवीन AstCMethodHard{
                            flp, schedp->clone_वृक्ष(असत्य), VCMethod::SCHED_READY};
                        callp->dtypeSetVoid();
                        callp->addPinsp(नवीन AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->addThensp(callp->makeStmt());
                    }
                    funcp->addStmtsp(ifp);
                }
            }

            AstVarScope* स्थिरांक vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            साठी (स्थिरांक स्वतः& triggersToTrees : usedTrigsToUsingTrees) {
                स्थिरांक माप_प्रकार word = triggersToTrees.first;
                funcp->addStmtsp(नवीन AstAssign{flp, getIdx(vscAccp, VAccess::WRITE, word),
                                               नवीन AstOr{flp, getIdx(vscAccp, VAccess::READ, word),
                                                         getIdx(vscp, VAccess::READ, word)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = पूर्वनिर्धारित;
};

शून्य beforeTrigVisitor(AstNetlist* netlistp, Sen_अभिव्यक्ती_Builder& sen_अभिव्यक्ती_Builder,
                       स्थिरांक TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_अभिव्यक्ती_Builder, trigKit};
}

}  // namespace V3Sched
