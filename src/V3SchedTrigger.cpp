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

AstVarScope* newLocal(AstCFunc* funcp, AstNodeDType* dtypep, ثابت std::string& name) {
    FileLine* ثابت flp = funcp->fileline();
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
    UASSERT(m_nPreWords, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ثابت netlistp = v3Global.rootp();
    FileLine* ثابت flp = netlistp->topScopep()->fileline();
    AstNodeDType* ثابت u32DTypep = netlistp->findUInt32DType();
    AstNodeDType* ثابت strDtypep = netlistp->findStringDType();

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
        AstNodeExpr* ثابت lhsp = جديد AstArraySel{flp, wr(vVscp), rd(iVscp)};
        AstNodeExpr* ثابت rhsp = جديد AstArraySel{flp, rd(eVscp), rd(iVscp)};
        AstNodeExpr* ثابت limp = جديد AstConst{flp, AstConst::WidthedValue{}, 32, m_nVecWords};
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
            AstNodeExpr* ثابت lhsp = جديد AstArraySel{flp, wr(pVscp), rd(jVscp)};
            AstNodeExpr* ثابت rhsp = جديد AstArraySel{flp, rd(eVscp), rd(jVscp)};
            AstNodeExpr* ثابت limp = جديد AstConst{flp, AstConst::WidthedValue{}, 32, m_nPreWords};
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
            AstNodeExpr* ثابت lhsp = جديد AstArraySel{flp, wr(pVscp), rd(jVscp)};
            AstNodeExpr* ثابت rhsp = جديد AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
            AstNodeExpr* ثابت limp = جديد AstConst{flp, AstConst::WidthedValue{}, 32, m_nVecWords};
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
    FileLine* ثابت flp = netlistp->topScopep()->fileline();
    AstNodeDType* ثابت u32DTypep = netlistp->findUInt32DType();

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
    ثابت uint32_t nWords = dtypep->elementsConst();
    AstNodeExpr* ثابت condp = جديد AstArraySel{flp, rd(iVscp), rd(nVscp)};
    AstNodeStmt* ثابت thenp = جديد AstCReturn{flp, جديد AstConst{flp, AstConst::BitTrue{}}};
    AstNodeExpr* ثابت limp = جديد AstConst{flp, AstConst::WidthedValue{}, 32, nWords};
    loopp->addStmtsp(جديد AstIf{flp, condp, thenp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});

    // Done
    أرجع funcp;
}
AstCFunc* TriggerKit::createClearFunc() ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    FileLine* ثابت flp = netlistp->topScopep()->fileline();
    AstNodeDType* ثابت u32DTypep = netlistp->findUInt32DType();

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
    AstNodeExpr* ثابت lhsp = جديد AstArraySel{flp, wr(oVscp), rd(nVscp)};
    AstNodeExpr* ثابت rhsp = جديد AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
    AstNodeExpr* ثابت limp = جديد AstConst{flp, AstConst::WidthedValue{}, 32, m_nVecWords};
    loopp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});

    // Done
    أرجع funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ثابت oDtypep,
                                       AstUnpackArrayDType* ثابت iDtypep) ثابت {
    AstNetlist* ثابت netlistp = v3Global.rootp();
    FileLine* ثابت flp = netlistp->topScopep()->fileline();
    AstNodeDType* ثابت u32DTypep = netlistp->findUInt32DType();

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
    AstNodeExpr* ثابت lhsp = جديد AstArraySel{flp, wr(oVscp), rd(nVscp)};
    AstNodeExpr* ثابت oWordp = جديد AstArraySel{flp, rd(oVscp), rd(nVscp)};
    AstNodeExpr* ثابت iWordp = جديد AstArraySel{flp, rd(iVscp), rd(nVscp)};
    AstNodeExpr* ثابت rhsp = جديد AstOr{flp, oWordp, iWordp};
    AstConst* ثابت outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ثابت inputRangeLeftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    AstNodeExpr* ثابت limp = outputRangeLeftp->num().toSInt() < inputRangeLeftp->num().toSInt()
                                  ? outputRangeLeftp->cloneTreePure(خطأ)
                                  : inputRangeLeftp->cloneTreePure(خطأ);
    loopp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(جديد AstLoopTest{flp, loopp, جديد AstLte{flp, rd(nVscp), limp}});

    // Done
    أرجع funcp;
}

AstNodeExpr* TriggerKit::newAnySetCall(AstVarScope* ثابت vscp) ثابت {
    FileLine* ثابت flp = v3Global.rootp()->topScopep()->fileline();
    𐤀𐤌 (!m_nVecWords) أرجع جديد AstConst{flp, AstConst::BitFalse{}};

    AstCFunc* funcp = nullptr;
    𐤀𐤌 (vscp->dtypep() == m_trigVecDTypep) {
        𐤀𐤌 (!m_anySetVecp) m_anySetVecp = createAnySetFunc(m_trigVecDTypep);
        funcp = m_anySetVecp;
    } وإلا 𐤀𐤌 (vscp->dtypep() == m_trigExtDTypep) {
        𐤀𐤌 (!m_anySetExtp) m_anySetExtp = createAnySetFunc(m_trigExtDTypep);
        funcp = m_anySetExtp;
    } وإلا {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->addArgsp(جديد AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetBit();
    أرجع callp;
}
AstNodeStmt* TriggerKit::newClearCall(AstVarScope* ثابت vscp) ثابت {
    𐤀𐤌 (!m_nVecWords) أرجع nullptr;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    𐤀𐤌 (!m_clearp) m_clearp = createClearFunc();
    FileLine* ثابت flp = v3Global.rootp()->topScopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_clearp};
    callp->addArgsp(جديد AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetVoid();
    أرجع callp->makeStmt();
}
AstNodeStmt* TriggerKit::newOrIntoCall(AstVarScope* ثابت oVscp, AstVarScope* ثابت iVscp) ثابت {
    𐤀𐤌 (!m_nVecWords) أرجع nullptr;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    ثابت size_t mask
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[mask];
    𐤀𐤌 (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    FileLine* ثابت flp = v3Global.rootp()->topScopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->addArgsp(جديد AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->addArgsp(جديد AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtypeSetVoid();
    أرجع callp->makeStmt();
}

AstNodeStmt* TriggerKit::newCompBaseCall() ثابت {
    𐤀𐤌 (!m_nVecWords) أرجع nullptr;
    FileLine* ثابت flp = v3Global.rootp()->topScopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_compVecp};
    callp->dtypeSetVoid();
    أرجع callp->makeStmt();
}

AstNodeStmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ثابت {
    𐤀𐤌 (!m_nPreWords) أرجع nullptr;
    FileLine* ثابت flp = v3Global.rootp()->topScopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, m_compExtp};
    callp->addArgsp(جديد AstVarRef{flp, vscp, VAccess::READ});
    callp->dtypeSetVoid();
    أرجع callp->makeStmt();
}

AstNodeStmt* TriggerKit::newDumpCall(AstVarScope* ثابت vscp, ثابت std::string& tag,
                                     منطقي debugOnly) ثابت {
    𐤀𐤌 (!m_nVecWords) أرجع nullptr;
    AstCFunc* funcp = nullptr;
    𐤀𐤌 (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } وإلا 𐤀𐤌 (vscp->dtypep() == m_trigExtDTypep) {
        𐤀𐤌 (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } وإلا {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    FileLine* ثابت flp = v3Global.rootp()->topScopep()->fileline();
    AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
    callp->addArgsp(جديد AstVarRef{flp, vscp, VAccess::READ});
    callp->addArgsp(جديد AstConst{flp, AstConst::String{}, tag});
    callp->dtypeSetVoid();
    AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
    cstmtp->add("#ifdef VL_DEBUG\n");
    𐤀𐤌 (debugOnly) {
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
    𐤀𐤌 (!m_nVecWords) أرجع nullptr;
    أرجع v3Global.rootp()->topScopep()->createTemp("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extraTriggerIndex(uint32_t index) ثابت {
    UASSERT(index <= m_nExtraWords * WORD_SIZE, "Invalid external trigger index");
    أرجع index + m_nSenseWords * WORD_SIZE;
}

std::set<uint32_t> TriggerKit::triggerIndices(ثابت AstSenTree* senTreep, منطقي pre) ثابت {
    std::set<uint32_t> indices;
    لكل (ثابت AstSenItem* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), SenItem)) {
        uint32_t index = m_senItem2TrigIdx.at(*sip);
        𐤀𐤌 (pre) {
            UASSERT_OBJ(index < m_nPreWords * WORD_SIZE, sip, "Not a 'pre' trigger");
            index += m_nVecWords * WORD_SIZE;
        }
        indices.insert(index);
    }
    أرجع indices;
}

AstNodeStmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ثابت {
    index = extraTriggerIndex(index);
    ثابت uint32_t wordIndex = index / WORD_SIZE;
    ثابت uint32_t bitIndex = index % WORD_SIZE;
    FileLine* ثابت flp = vscp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{flp, m_vscp, VAccess::WRITE};
    AstNodeExpr* ثابت wordp = جديد AstArraySel{flp, refp, static_cast<صحيح>(wordIndex)};
    AstNodeExpr* ثابت trigLhsp = جديد AstSel{flp, wordp, static_cast<صحيح>(bitIndex), 1};
    AstNodeExpr* ثابت trigRhsp = جديد AstVarRef{flp, vscp, VAccess::READ};
    أرجع جديد AstAssign{flp, trigLhsp, trigRhsp};
}

فراغ TriggerKit::addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ثابت {
    FileLine* ثابت flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    AstNode* ثابت setp = newExtraTriggerAssignment(vscp, index);
    setp->addNext(جديد AstAssign{flp, جديد AstVarRef{flp, vscp, VAccess::WRITE},
                                جديد AstConst{flp, AstConst::BitFalse{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
فراغ TriggerKit::addValueChangeTriggerAssignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t index) ثابت {
    index += m_nSenseWords * WORD_SIZE;
    ثابت uint32_t wordIndex = index / WORD_SIZE;
    ثابت uint32_t bitIndex = index % WORD_SIZE;

    AstTopScope* ثابت topScopep = netlistp->topScopep();
    FileLine* ثابت flp = netlistp->fileline();
    AstNodeDType* ثابت dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    𐤀𐤌 (ثابت AstBasicDType* ثابت bdtypep = VN_CAST(dtypep, BasicDType)) {
        𐤀𐤌 (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            أرجع;
        }
    } وإلا 𐤀𐤌 (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, NodeUOrStructDType) && !VN_IS(dtypep, ClassRefDType)) {
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
    AstVarScope* ثابت prevVscp = topScopep->createTemp(prevName, instVscp->dtypep());

    // Initialize prev = inst
    𐤀𐤌 (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = جديد AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        initFuncp->addStmtsp(cmhp->makeStmt());
    } وإلا {
        initFuncp->addStmtsp(جديد AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    AstNodeExpr* neqp;
    𐤀𐤌 (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp
            = جديد AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtypeSetBit();
        neqp = cmhp;
    } وإلا {
        neqp = جديد AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    AstNode* updp;
    𐤀𐤌 (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ثابت cmhp = جديد AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        updp = cmhp->makeStmt();
    } وإلا {
        updp = جديد AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ثابت refp = جديد AstVarRef{flp, m_vscp, VAccess::WRITE};
    AstNodeExpr* ثابت wordp = جديد AstArraySel{flp, refp, static_cast<صحيح>(wordIndex)};
    AstNodeExpr* ثابت trigLhsp = جديد AstSel{flp, wordp, static_cast<صحيح>(bitIndex), 1};
    AstNode* ثابت setp = جديد AstAssign{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->addNext(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ثابت std::string& name, منطقي slow, uint32_t nSenseWords,
                       uint32_t nExtraWords, uint32_t nPreWords,
                       std::unordered_map<VNRef<ثابت AstSenItem>, size_t> senItem2TrigIdx,
                       ثابت std::vector<ثابت AstSenItem*>& senItemps, منطقي useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_nSenseWords{nSenseWords}
    , m_nExtraWords{nExtraWords}
    , m_nPreWords{nPreWords}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trigIdx2SenItem{senItemps} {
    // If no triggers, we don't need to generate anything
    𐤀𐤌 (!m_nVecWords) أرجع;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trigIdx2SenItem.resize((m_nVecWords + m_nPreWords) * WORD_SIZE);
    std::copy(senItemps.begin(), senItemps.begin() + m_nPreWords * WORD_SIZE,
              m_trigIdx2SenItem.begin() + m_nVecWords * WORD_SIZE);
    AstNetlist* ثابت netlistp = v3Global.rootp();
    AstScope* ثابت scopep = netlistp->topScopep()->scopep();
    FileLine* ثابت flp = scopep->fileline();
    // Data type of a single trigger word
    m_wordDTypep = netlistp->findBitDType(WORD_SIZE, WORD_SIZE, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ثابت rp = جديد AstRange{flp, static_cast<صحيح>(m_nVecWords - 1), 0};
    m_trigVecDTypep = جديد AstUnpackArrayDType{flp, m_wordDTypep, rp};
    netlistp->typeTablep()->addTypesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    𐤀𐤌 (m_nPreWords) {
        AstRange* ثابت ep = جديد AstRange{flp, static_cast<صحيح>(m_nVecWords + m_nPreWords - 1), 0};
        m_trigExtDTypep = جديد AstUnpackArrayDType{flp, m_wordDTypep, ep};
        netlistp->typeTablep()->addTypesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } وإلا {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->topScopep()->createTemp("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->isInternal(true);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, true);
    m_dumpp->isStatic(true);
    m_dumpp->ifdef("VL_DEBUG");
    𐤀𐤌 (useAcc) {
        m_vscAccp
            = netlistp->topScopep()->createTemp("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->isInternal(true);
    }
}

AstAssign* TriggerKit::createSenTrigVecAssignment(AstVarScope* ثابت target,
                                                  std::vector<AstNodeExpr*>& trigps) {
    FileLine* ثابت flp = target->fileline();
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
        trigStmtsp = AstNode::addNext(trigStmtsp, جديد AstAssign{flp, aselp, trigps[i]});
    }
    أرجع trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ثابت initFuncp,  //
                              SenExprBuilder& senExprBuilder,  //
                              ثابت std::vector<ثابت AstSenTree*>& preTreeps,  //
                              ثابت std::vector<ثابت AstSenTree*>& senTreeps,  //
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
    لكل (ثابت AstSenTree* ثابت senTreep : preTreeps) {
        لكل (ثابت AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            𐤀𐤌 (pair.second) senItemps.push_back(itemp);
        }
    }
    ثابت uint32_t nPreSenItems = senItemps.size();
    V3Stats::addStat("Scheduling, '" + name + "' pre triggers", nPreSenItems);
    // Number of pre triggers, rounded up to a full word.
    ثابت uint32_t nPreTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ثابت uint32_t nPreWords = nPreTriggers / WORD_SIZE;

    // Process the rest of the trees
    لكل (ثابت AstSenTree* ثابت senTreep : senTreeps) {
        لكل (ثابت AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ثابت auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            𐤀𐤌 (pair.second) senItemps.push_back(itemp);
        }
    }
    ثابت uint32_t nSenItems = senItemps.size() - nPreTriggers;
    V3Stats::addStat("Scheduling, '" + name + "' sense triggers", nSenItems + nPreSenItems);
    // Number of sense triggers, rounded up to a full word
    ثابت uint32_t nSenseTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ثابت uint32_t nSenseWords = nSenseTriggers / WORD_SIZE;

    // Allocate space for the extra triggers
    V3Stats::addStat("Scheduling, '" + name + "' extra triggers", extraTriggers.size());
    // Number of extra triggers, rounded up to a full word.
    ثابت uint32_t nExtraTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(extraTriggers.size());
    ثابت uint32_t nExtraWords = nExtraTriggers / WORD_SIZE;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  nSenseWords, nExtraWords, nPreWords, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    𐤀𐤌 (!kit.m_nVecWords) أرجع kit;

    FileLine* ثابت flp = netlistp->topScopep()->fileline();

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
        AstNodeExpr* ثابت aselp = جديد AstArraySel{flp, rd(dumpTrgp), wrdIndex};
        AstNodeExpr* ثابت condp = جديد AstSel{flp, aselp, bitIndex, 1};
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
    std::vector<AstNodeExpr*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    AstNodeStmt* initialTrigsp = nullptr;
    لكل (size_t i = 0; i < senItemps.size(); ++i) {
        ثابت AstSenItem* ثابت senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        𐤀𐤌 (!senItemp) {
            trigps.emplace_back(جديد AstConst{flp, AstConst::BitFalse{}});
            استمر;
        }

        // Create the trigger computation expression
        ثابت auto& pair = senExprBuilder.build(senItemp);
        trigps.emplace_back(pair.first);

        // Add initialization time trigger
        𐤀𐤌 (pair.second || v3Global.opt.xInitialEdge()) {
            ثابت صحيح wrdIndex = static_cast<صحيح>(i / WORD_SIZE);
            ثابت صحيح bitIndex = static_cast<صحيح>(i % WORD_SIZE);
            AstNodeExpr* ثابت wordp = جديد AstArraySel{flp, wr(kit.m_vscp), wrdIndex};
            AstNodeExpr* ثابت lhsp = جديد AstSel{flp, wordp, bitIndex, 1};
            AstNodeExpr* ثابت rhsp = جديد AstConst{flp, AstConst::BitTrue{}};
            𐤀𐤌 (useAcc) {
                initFuncp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});
            } وإلا {
                initialTrigsp = AstNode::addNext(initialTrigsp, جديد AstAssign{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilogForTree(senItemp, ss);
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
    ثابت SenExprBuilder::Results senResults = senExprBuilder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    لكل (AstNodeStmt* ثابت nodep : senResults.m_inits) initFuncp->addStmtsp(nodep);

    // Assemble the base trigger computation function
    AstTopScope* ثابت topScopep = netlistp->topScopep();
    {
        AstCFunc* ثابت fp = kit.m_compVecp;
        // Trigger computation
        لكل (AstNodeStmt* ثابت nodep : senResults.m_preUpdates) fp->addStmtsp(nodep);
        fp->addStmtsp(trigStmtsp);
        لكل (AstNodeStmt* ثابت nodep : senResults.m_postUpdates) fp->addStmtsp(nodep);
        // Add the initialization time triggers
        𐤀𐤌 (initialTrigsp) {
            AstVarScope* ثابت initVscp = topScopep->createTemp("__V" + name + "DidInit", 1);
            AstIf* ثابت ifp = جديد AstIf{flp, جديد AstNot{flp, rd(initVscp)}};
            fp->addStmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->addThensp(util::setVar(initVscp, 1));
            ifp->addThensp(initialTrigsp);
        }
        util::splitCheck(fp);
    };
    // If there are 'pre' triggers, compute them
    𐤀𐤌 (kit.m_nPreWords) {
        AstCFunc* ثابت fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ثابت latchedp
            = newArgument(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ثابت nVscp = topScopep->createTemp("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(true);
        // Add a loop to compute the pre words
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        fp->addStmtsp(util::setVar(nVscp, 0));
        fp->addStmtsp(loopp);
        // Loop body
        AstNodeExpr* ثابت offsetp = جديد AstConst{flp, kit.m_nVecWords};
        AstNodeExpr* ثابت lIdxp = جديد AstAdd{flp, rd(nVscp), offsetp};
        AstNodeExpr* ثابت lhsp = جديد AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        AstNodeExpr* ثابت aWordp = جديد AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        AstNodeExpr* ثابت bWordp = جديد AstArraySel{flp, rd(latchedp), rd(nVscp)};
        AstNodeExpr* ثابت rhsp = جديد AstAnd{flp, aWordp, جديد AstNot{flp, bWordp}};
        AstNodeExpr* ثابت limp = جديد AstConst{flp, AstConst::WidthedValue{}, 32, nPreWords};
        loopp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(nVscp));
        loopp->addStmtsp(جديد AstLoopTest{flp, loopp, جديد AstLt{flp, rd(nVscp), limp}});
        util::splitCheck(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    𐤀𐤌 (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlinkFrBackWithNext()->deleteTree();
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
    SenExprBuilder& m_senExprBuilder;
    // Generator of unique names for before-trigger function
    V3UniqueNames m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::vector<std::pair<AstCFunc*, AstSenTree*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::vector<std::pair<AstSenTree*, AstNodeExpr*>> m_senTreeToSched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<AstNode>, std::vector<AstSenItem*>> m_senExprToSenItem;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    ساكن AstNode* getSenHashNode(ثابت AstSenItem* ثابت nodep) {
        𐤀𐤌 (AstVarRef* ثابت varRefp = VN_CAST(nodep->sensp(), VarRef)) أرجع varRefp;
        أرجع nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    فراغ fillSenExprToSenItem() {
        لكل (auto senTreeSched : m_senTreeToSched) {
            AstSenTree* ثابت senTreep = senTreeSched.first;

            لكل (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ثابت VEdgeType edge = senItemp->edgeType();
                𐤀𐤌 (edge.anEdge() || edge == VEdgeType::ET_CHANGED
                    || edge == VEdgeType::ET_HYBRID) {
                    m_senExprToSenItem[*getSenHashNode(senItemp)].push_back(senItemp);
                }
            }
        }

        std::vector<VNRef<AstNode>> toRemove;
        لكل (ثابت auto& senExprToSenTree : m_senExprToSenItem) {
            std::vector<AstSenItem*> senItemps = senExprToSenTree.second;
            toRemove.push_back(senExprToSenTree.first);
            لكل (size_t i = 1; i < senItemps.size(); ++i) {
                𐤀𐤌 (senItemps[i]->edgeType() != senItemps[i - 1]->edgeType()) {
                    toRemove.pop_back();
                    اكسر;
                }
            }
        }
        لكل (ثابت VNRef<AstNode> it : toRemove) m_senExprToSenItem.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<size_t, std::map<size_t, std::vector<AstNodeExpr*>>>
    getUsedTriggersToTrees(ثابت std::set<size_t>& usedTriggers) {
        std::map<size_t, std::map<size_t, std::vector<AstNodeExpr*>>> usedTrigsToUsingTrees;
        لكل (auto senTreeSched : m_senTreeToSched) {
            ثابت AstSenTree* ثابت senTreep = senTreeSched.first;
            AstNodeExpr* ثابت shedp = senTreeSched.second;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            لكل (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ثابت size_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                𐤀𐤌 (usedTriggers.find(idx) != usedTriggers.end()) {
                    usedTrigsToUsingTrees[idx / TriggerKit::WORD_SIZE]
                                         [size_t{1} << (idx % TriggerKit::WORD_SIZE)]
                                             .push_back(shedp);
                }
            }
        }
        𐤀𐤌 (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            لكل (ثابت auto& triggersToTrees : usedTrigsToUsingTrees) {
                لكل (ثابت auto& bitsToTrees : triggersToTrees.second) {
                    ثابت std::set<ثابت AstNodeExpr*> exprps{bitsToTrees.second.begin(),
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
    AstCCall* getBeforeTriggerStmt(AstSenTree* ثابت senTreep) {
        FileLine* ثابت flp = senTreep->fileline();
        𐤀𐤌 (!senTreep->user1p()) {
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
            argp->direction(VDirection::INPUT);
            funcp->addArgsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_back(funcp, senTreep);
        }
        AstCCall* ثابت callp = جديد AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtypeSetVoid();
        أرجع callp;
    }

    فراغ visit(AstCAwait* ثابت nodep) override {
        𐤀𐤌 (nodep->user1SetOnce()) أرجع;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        𐤀𐤌 (ثابت AstCMethodHard* ثابت cMethodHardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            𐤀𐤌 (cMethodHardp->method() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ثابت beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                AstNode* ثابت pinp = cMethodHardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, cMethodHardp, "No event description");
                beforeTrigp->addArgsp(VN_AS(pinp, NodeExpr)->cloneTree(خطأ));

                // Call the before-trigger function before the CAwait
                nodep->addHereThisAsNext(beforeTrigp->makeStmt());
                m_senTreeToSched.emplace_back(nodep->sentreep(), cMethodHardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    فراغ visit(AstNode* ثابت nodep) override { iterateChildren(nodep); }

عام:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, SenExprBuilder& senExprBuilder,
                           ثابت TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_senExprBuilder{senExprBuilder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fillSenExprToSenItem();

        std::vector<AstNodeExpr*> trigps;
        std::set<size_t> usedTriggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        لكل (ثابت auto& funcToUsedTriggers : m_generatedFuncs) {
            AstCFunc* ثابت funcp = funcToUsedTriggers.first;
            AstVarScope* ثابت vscp = VN_AS(funcp->user1p(), VarScope);
            FileLine* ثابت flp = funcp->fileline();

            // Generate trigger evaluation
            {
                AstSenTree* ثابت senTreep = funcToUsedTriggers.second;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ثابت auto emplaceAt
                    = [flp, &trigps, &usedTriggers](AstNodeExpr* ثابت exprp, ثابت size_t pos) {
                          ثابت size_t targetSize
                              = vlstd::roundUpToMultipleOf<TriggerKit::WORD_SIZE>(pos + 1);
                          𐤀𐤌 (trigps.capacity() < targetSize) trigps.reserve(targetSize * 2);
                          طالما (trigps.size() < targetSize) {
                              trigps.push_back(جديد AstConst{flp, AstConst::BitFalse{}});
                          }
                          trigps[pos]->deleteTree();
                          trigps[pos] = exprp;
                          usedTriggers.insert(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                لكل (ثابت AstSenItem* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), SenItem)) {
                    ثابت size_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_senExprBuilder.build(itemp).first, idx);
                    auto iter = m_senExprToSenItem.find(*getSenHashNode(itemp));
                    𐤀𐤌 (iter != m_senExprToSenItem.end()) {
                        لكل (AstSenItem* ثابت additionalItemp : iter->second) {
                            ثابت size_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_senExprBuilder.build(additionalItemp).first, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ثابت SenExprBuilder::Results results
                    = m_senExprBuilder.getResultsAndClearUpdates();
                لكل (AstNodeStmt* ثابت stmtsp : results.m_inits) funcp->addStmtsp(stmtsp);
                لكل (AstNodeStmt* ثابت stmtsp : results.m_preUpdates) funcp->addStmtsp(stmtsp);
                funcp->addStmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                لكل (AstNodeStmt* ثابت stmtsp : results.m_postUpdates) funcp->addStmtsp(stmtsp);
            }

            ثابت std::map<size_t, std::map<size_t, std::vector<AstNodeExpr*>>>
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
                    لكل (AstNodeExpr* ثابت schedp : schedulers) {
                        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                            flp, schedp->cloneTree(خطأ), VCMethod::SCHED_READY};
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

فراغ beforeTrigVisitor(AstNetlist* netlistp, SenExprBuilder& senExprBuilder,
                       ثابت TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, senExprBuilder, trigKit};
}

}  // namespace V3Sched
