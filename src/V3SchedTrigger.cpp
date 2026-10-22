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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3EmitCBase.h"
#અન્તર્ભાવ "V3EmitV.h"
#અન્તર્ભાવ "V3Order.h"
#અન્તર્ભાવ "V3Sched.h"
#અન્તર્ભાવ "V3SenExprBuilder.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ V3Sched {

નામાકાશ {

using util::newArgument;

AstVarScope* newLocal(AstCFunc* funcp, Ast_ગાંઠ_DType* dtypep, અચલ std::string& name) {
    File_લીટી* અચલ flp = funcp->fileline();
    AstScope* અચલ scopep = funcp->scopep();
    AstVar* અચલ varp = નવું AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(સાચું);
    varp->noReset(સાચું);
    funcp->addVarsp(varp);
    AstVarScope* અચલ vscp = નવું AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    પાછા vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() અચલ {
    UASSERT(m_n_Pre_શબ્દો, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* અચલ netlistp = v3Global.rootp();
    File_લીટી* અચલ flp = netlistp->ટોચ_Scopep()->fileline();
    Ast_ગાંઠ_DType* અચલ u32DTypep = netlistp->findUInt32DType();
    Ast_ગાંઠ_DType* અચલ strDtypep = netlistp->findStringDType();

    // Dumping function always slow
    અચલ std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* અચલ funcp = util::makeSubFunction(netlistp, name, સાચું);
    funcp->isStatic(સાચું);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* અચલ eVscp = newArgument(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* અચલ tVscp = newArgument(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    અચલ auto rd = [flp](AstVarScope* vp) { પાછા નવું AstVarRef{flp, vp, VAccess::READ}; };
    અચલ auto wr = [flp](AstVarScope* vp) { પાછા નવું AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* અચલ vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* અચલ iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->addStmtsp(util::setVar(iVscp, 0));
        // Add loop
        AstLoop* અચલ loopp = નવું AstLoop{flp};
        funcp->addStmtsp(loopp);
        // Loop body
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = નવું AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = નવું AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_ગાંઠ_પદાવલિ* અચલ limp = નવું AstConst{flp, AstConst::Widthed_કિંમત{}, 32, m_n_Vec_શબ્દો};
        loopp->addStmtsp(નવું AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(iVscp));
        loopp->addStmtsp(નવું AstLoopTest{flp, loopp, નવું AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* અચલ callp = નવું AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(vVscp));
        callp->addArgsp(rd(tVscp));
        funcp->addStmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* અચલ pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* અચલ jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->addStmtsp(util::setVar(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* અચલ loopp = નવું AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = નવું AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = નવું AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_ગાંઠ_પદાવલિ* અચલ limp = નવું AstConst{flp, AstConst::Widthed_કિંમત{}, 32, m_n_Pre_શબ્દો};
            loopp->addStmtsp(નવું AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(નવું AstLoopTest{flp, loopp, નવું AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* અચલ loopp = નવું AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = નવું AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = નવું AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
            Ast_ગાંઠ_પદાવલિ* અચલ limp = નવું AstConst{flp, AstConst::Widthed_કિંમત{}, 32, m_n_Vec_શબ્દો};
            loopp->addStmtsp(નવું AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(નવું AstLoopTest{flp, loopp, નવું AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* અચલ callp = નવું AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(pVscp));
        callp->addArgsp(
            નવું AstConcatN{flp, rd(tVscp), નવું AstConst{flp, AstConst::String{}, " pre"}});
        funcp->addStmtsp(callp->makeStmt());
    }

    પાછા funcp;
}

AstCFunc* TriggerKit::createAnySetFunc(AstUnpackArrayDType* અચલ dtypep) અચલ {
    AstNetlist* અચલ netlistp = v3Global.rootp();
    File_લીટી* અચલ flp = netlistp->ટોચ_Scopep()->fileline();
    Ast_ગાંઠ_DType* અચલ u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* અચલ funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(સાચું);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* અચલ iVscp = newArgument(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* અચલ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    અચલ auto rd = [flp](AstVarScope* vp) { પાછા નવું AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* અચલ loopp = નવું AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);
    funcp->addStmtsp(નવું AstCReturn{flp, નવું AstConst{flp, AstConst::BitFalse{}}});

    // Loop body
    અચલ uint32_t n_શબ્દો = dtypep->elementsConst();
    Ast_ગાંઠ_પદાવલિ* અચલ condp = નવું AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ગાંઠ_Stmt* અચલ thenp = નવું AstCReturn{flp, નવું AstConst{flp, AstConst::BitTrue{}}};
    Ast_ગાંઠ_પદાવલિ* અચલ limp = નવું AstConst{flp, AstConst::Widthed_કિંમત{}, 32, n_શબ્દો};
    loopp->addStmtsp(નવું AstIf{flp, condp, thenp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(નવું AstLoopTest{flp, loopp, નવું AstLt{flp, rd(nVscp), limp}});

    // Done
    પાછા funcp;
}
AstCFunc* TriggerKit::createClearFunc() અચલ {
    AstNetlist* અચલ netlistp = v3Global.rootp();
    File_લીટી* અચલ flp = netlistp->ટોચ_Scopep()->fileline();
    Ast_ગાંઠ_DType* અચલ u32DTypep = netlistp->findUInt32DType();

    // Create function
    AstCFunc* અચલ funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(સાચું);

    // Add arguments
    AstVarScope* અચલ oVscp = newArgument(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* અચલ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    અચલ auto rd = [flp](AstVarScope* vp) { પાછા નવું AstVarRef{flp, vp, VAccess::READ}; };
    અચલ auto wr = [flp](AstVarScope* vp) { પાછા નવું AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* અચલ loopp = નવું AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_ગાંઠ_પદાવલિ* અચલ lhsp = નવું AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ગાંઠ_પદાવલિ* અચલ rhsp = નવું AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
    Ast_ગાંઠ_પદાવલિ* અચલ limp = નવું AstConst{flp, AstConst::Widthed_કિંમત{}, 32, m_n_Vec_શબ્દો};
    loopp->addStmtsp(નવું AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(નવું AstLoopTest{flp, loopp, નવું AstLt{flp, rd(nVscp), limp}});

    // Done
    પાછા funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* અચલ oDtypep,
                                       AstUnpackArrayDType* અચલ iDtypep) અચલ {
    AstNetlist* અચલ netlistp = v3Global.rootp();
    File_લીટી* અચલ flp = netlistp->ટોચ_Scopep()->fileline();
    Ast_ગાંઠ_DType* અચલ u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* અચલ funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(સાચું);

    // Add arguments
    AstVarScope* અચલ oVscp = newArgument(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* અચલ iVscp = newArgument(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* અચલ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    અચલ auto rd = [flp](AstVarScope* vp) { પાછા નવું AstVarRef{flp, vp, VAccess::READ}; };
    અચલ auto wr = [flp](AstVarScope* vp) { પાછા નવું AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* અચલ loopp = નવું AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_ગાંઠ_પદાવલિ* અચલ lhsp = નવું AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ગાંઠ_પદાવલિ* અચલ oWordp = નવું AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_ગાંઠ_પદાવલિ* અચલ iWordp = નવું AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ગાંઠ_પદાવલિ* અચલ rhsp = નવું AstOr{flp, oWordp, iWordp};
    AstConst* અચલ outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* અચલ આગત_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_ગાંઠ_પદાવલિ* અચલ limp = outputRangeLeftp->અંક().toSInt() < આગત_Range_Leftp->અંક().toSInt()
                                  ? outputRangeLeftp->clone_વૃક્ષ_Pure(ખોટું)
                                  : આગત_Range_Leftp->clone_વૃક્ષ_Pure(ખોટું);
    loopp->addStmtsp(નવું AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(નવું AstLoopTest{flp, loopp, નવું AstLte{flp, rd(nVscp), limp}});

    // Done
    પાછા funcp;
}

Ast_ગાંઠ_પદાવલિ* TriggerKit::newAnySetCall(AstVarScope* અચલ vscp) અચલ {
    File_લીટી* અચલ flp = v3Global.rootp()->ટોચ_Scopep()->fileline();
    જો (!m_n_Vec_શબ્દો) પાછા નવું AstConst{flp, AstConst::BitFalse{}};

    AstCFunc* funcp = nullptr;
    જો (vscp->dtypep() == m_trigVecDTypep) {
        જો (!m_anySetVecp) m_anySetVecp = createAnySetFunc(m_trigVecDTypep);
        funcp = m_anySetVecp;
    } નહીંતર જો (vscp->dtypep() == m_trigExtDTypep) {
        જો (!m_anySetExtp) m_anySetExtp = createAnySetFunc(m_trigExtDTypep);
        funcp = m_anySetExtp;
    } નહીંતર {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* અચલ callp = નવું AstCCall{flp, funcp};
    callp->addArgsp(નવું AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetBit();
    પાછા callp;
}
Ast_ગાંઠ_Stmt* TriggerKit::newClearCall(AstVarScope* અચલ vscp) અચલ {
    જો (!m_n_Vec_શબ્દો) પાછા nullptr;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    જો (!m_clearp) m_clearp = createClearFunc();
    File_લીટી* અચલ flp = v3Global.rootp()->ટોચ_Scopep()->fileline();
    AstCCall* અચલ callp = નવું AstCCall{flp, m_clearp};
    callp->addArgsp(નવું AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetVoid();
    પાછા callp->makeStmt();
}
Ast_ગાંઠ_Stmt* TriggerKit::newOrIntoCall(AstVarScope* અચલ oVscp, AstVarScope* અચલ iVscp) અચલ {
    જો (!m_n_Vec_શબ્દો) પાછા nullptr;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    અચલ size_t mask
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[mask];
    જો (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_લીટી* અચલ flp = v3Global.rootp()->ટોચ_Scopep()->fileline();
    AstCCall* અચલ callp = નવું AstCCall{flp, funcp};
    callp->addArgsp(નવું AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->addArgsp(નવું AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtypeSetVoid();
    પાછા callp->makeStmt();
}

Ast_ગાંઠ_Stmt* TriggerKit::newCompBaseCall() અચલ {
    જો (!m_n_Vec_શબ્દો) પાછા nullptr;
    File_લીટી* અચલ flp = v3Global.rootp()->ટોચ_Scopep()->fileline();
    AstCCall* અચલ callp = નવું AstCCall{flp, m_compVecp};
    callp->dtypeSetVoid();
    પાછા callp->makeStmt();
}

Ast_ગાંઠ_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) અચલ {
    જો (!m_n_Pre_શબ્દો) પાછા nullptr;
    File_લીટી* અચલ flp = v3Global.rootp()->ટોચ_Scopep()->fileline();
    AstCCall* અચલ callp = નવું AstCCall{flp, m_compExtp};
    callp->addArgsp(નવું AstVarRef{flp, vscp, VAccess::READ});
    callp->dtypeSetVoid();
    પાછા callp->makeStmt();
}

Ast_ગાંઠ_Stmt* TriggerKit::newDumpCall(AstVarScope* અચલ vscp, અચલ std::string& tag,
                                     તાર્કિક debugOnly) અચલ {
    જો (!m_n_Vec_શબ્દો) પાછા nullptr;
    AstCFunc* funcp = nullptr;
    જો (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } નહીંતર જો (vscp->dtypep() == m_trigExtDTypep) {
        જો (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } નહીંતર {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_લીટી* અચલ flp = v3Global.rootp()->ટોચ_Scopep()->fileline();
    AstCCall* અચલ callp = નવું AstCCall{flp, funcp};
    callp->addArgsp(નવું AstVarRef{flp, vscp, VAccess::READ});
    callp->addArgsp(નવું AstConst{flp, AstConst::String{}, tag});
    callp->dtypeSetVoid();
    AstCStmt* અચલ cstmtp = નવું AstCStmt{flp};
    cstmtp->add("#ifdef VL_DEBUG\n");
    જો (debugOnly) {
        cstmtp->add("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->add(callp->makeStmt());
        cstmtp->add("}\n");
    } નહીંતર {
        cstmtp->add(callp->makeStmt());
    }
    cstmtp->add("#endif");
    પાછા cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(અચલ std::string& name) અચલ {
    જો (!m_n_Vec_શબ્દો) પાછા nullptr;
    પાછા v3Global.rootp()->ટોચ_Scopep()->createTemp("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extraTriggerIndex(uint32_t index) અચલ {
    UASSERT(index <= m_n_Extra_શબ્દો * WORD_SIZE, "Invalid external trigger index");
    પાછા index + m_n_Sense_શબ્દો * WORD_SIZE;
}

std::set<uint32_t> TriggerKit::triggerIndices(અચલ Ast_Sen_વૃક્ષ* senTreep, તાર્કિક pre) અચલ {
    std::set<uint32_t> indices;
    માટે (અચલ AstSenItem* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), SenItem)) {
        uint32_t index = m_senItem2TrigIdx.at(*sip);
        જો (pre) {
            UASSERT_OBJ(index < m_n_Pre_શબ્દો * WORD_SIZE, sip, "Not a 'pre' trigger");
            index += m_n_Vec_શબ્દો * WORD_SIZE;
        }
        indices.ઉમેરો(index);
    }
    પાછા indices;
}

Ast_ગાંઠ_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) અચલ {
    index = extraTriggerIndex(index);
    અચલ uint32_t wordIndex = index / WORD_SIZE;
    અચલ uint32_t bitIndex = index % WORD_SIZE;
    File_લીટી* અચલ flp = vscp->fileline();
    AstVarRef* અચલ refp = નવું AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ગાંઠ_પદાવલિ* અચલ wordp = નવું AstArraySel{flp, refp, static_cast<પૂર્ણાંક>(wordIndex)};
    Ast_ગાંઠ_પદાવલિ* અચલ trigLhsp = નવું AstSel{flp, wordp, static_cast<પૂર્ણાંક>(bitIndex), 1};
    Ast_ગાંઠ_પદાવલિ* અચલ trigRhsp = નવું AstVarRef{flp, vscp, VAccess::READ};
    પાછા નવું AstAssign{flp, trigLhsp, trigRhsp};
}

શૂન્ય TriggerKit::addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) અચલ {
    File_લીટી* અચલ flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_ગાંઠ* અચલ setp = newExtraTriggerAssignment(vscp, index);
    setp->addNext(નવું AstAssign{flp, નવું AstVarRef{flp, vscp, VAccess::WRITE},
                                નવું AstConst{flp, AstConst::BitFalse{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
શૂન્ય TriggerKit::add_કિંમત_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t index) અચલ {
    index += m_n_Sense_શબ્દો * WORD_SIZE;
    અચલ uint32_t wordIndex = index / WORD_SIZE;
    અચલ uint32_t bitIndex = index % WORD_SIZE;

    Ast_ટોચ_Scope* અચલ ટોચ_Scopep = netlistp->ટોચ_Scopep();
    File_લીટી* અચલ flp = netlistp->fileline();
    Ast_ગાંઠ_DType* અચલ dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    જો (અચલ AstBasicDType* અચલ bdtypep = VN_CAST(dtypep, BasicDType)) {
        જો (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            પાછા;
        }
    } નહીંતર જો (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, ગાંઠ_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        પાછા;
    }

    અચલ auto rdInst = [flp](AstVarScope* vp) { પાછા નવું AstVarRef{flp, vp, VAccess::READ}; };
    અચલ auto wrPrev = [flp](AstVarScope* vp) { પાછા નવું AstVarRef{flp, vp, VAccess::WRITE}; };
    અચલ auto rdPrev = [flp](AstVarScope* vp) { પાછા નવું AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    અચલ std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* અચલ prevVscp = ટોચ_Scopep->createTemp(prevName, instVscp->dtypep());

    // Initialize prev = inst
    જો (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* અચલ cmhp = નવું AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        initFuncp->addStmtsp(cmhp->makeStmt());
    } નહીંતર {
        initFuncp->addStmtsp(નવું AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_ગાંઠ_પદાવલિ* neqp;
    જો (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* અચલ cmhp
            = નવું AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtypeSetBit();
        neqp = cmhp;
    } નહીંતર {
        neqp = નવું AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_ગાંઠ* updp;
    જો (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* અચલ cmhp = નવું AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        updp = cmhp->makeStmt();
    } નહીંતર {
        updp = નવું AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* અચલ refp = નવું AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ગાંઠ_પદાવલિ* અચલ wordp = નવું AstArraySel{flp, refp, static_cast<પૂર્ણાંક>(wordIndex)};
    Ast_ગાંઠ_પદાવલિ* અચલ trigLhsp = નવું AstSel{flp, wordp, static_cast<પૂર્ણાંક>(bitIndex), 1};
    Ast_ગાંઠ* અચલ setp = નવું AstAssign{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->addNext(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(અચલ std::string& name, તાર્કિક slow, uint32_t n_Sense_શબ્દો,
                       uint32_t n_Extra_શબ્દો, uint32_t n_Pre_શબ્દો,
                       std::unordered_map<VNRef<અચલ AstSenItem>, size_t> senItem2TrigIdx,
                       અચલ std::vector<અચલ AstSenItem*>& senItemps, તાર્કિક useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_શબ્દો{n_Sense_શબ્દો}
    , m_n_Extra_શબ્દો{n_Extra_શબ્દો}
    , m_n_Pre_શબ્દો{n_Pre_શબ્દો}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trigIdx2SenItem{senItemps} {
    // If no triggers, we don't need to generate anything
    જો (!m_n_Vec_શબ્દો) પાછા;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trigIdx2SenItem.resize((m_n_Vec_શબ્દો + m_n_Pre_શબ્દો) * WORD_SIZE);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_શબ્દો * WORD_SIZE,
              m_trigIdx2SenItem.begin() + m_n_Vec_શબ્દો * WORD_SIZE);
    AstNetlist* અચલ netlistp = v3Global.rootp();
    AstScope* અચલ scopep = netlistp->ટોચ_Scopep()->scopep();
    File_લીટી* અચલ flp = scopep->fileline();
    // Data type of a single trigger word
    m_wordDTypep = netlistp->findBitDType(WORD_SIZE, WORD_SIZE, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* અચલ rp = નવું AstRange{flp, static_cast<પૂર્ણાંક>(m_n_Vec_શબ્દો - 1), 0};
    m_trigVecDTypep = નવું AstUnpackArrayDType{flp, m_wordDTypep, rp};
    netlistp->typeTablep()->addTypesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    જો (m_n_Pre_શબ્દો) {
        AstRange* અચલ ep = નવું AstRange{flp, static_cast<પૂર્ણાંક>(m_n_Vec_શબ્દો + m_n_Pre_શબ્દો - 1), 0};
        m_trigExtDTypep = નવું AstUnpackArrayDType{flp, m_wordDTypep, ep};
        netlistp->typeTablep()->addTypesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } નહીંતર {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->ટોચ_Scopep()->createTemp("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->isInternal(સાચું);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, સાચું);
    m_dumpp->isStatic(સાચું);
    m_dumpp->ifdef("VL_DEBUG");
    જો (useAcc) {
        m_vscAccp
            = netlistp->ટોચ_Scopep()->createTemp("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->isInternal(સાચું);
    }
}

AstAssign* TriggerKit::createSenTrigVecAssignment(AstVarScope* અચલ target,
                                                  std::vector<Ast_ગાંઠ_પદાવલિ*>& trigps) {
    File_લીટી* અચલ flp = target->fileline();
    AstAssign* trigStmtsp = nullptr;
    // Assign sense triggers vector one word at a time
    માટે (size_t i = 0; i < trigps.size(); i += WORD_SIZE) {
        // Concatenate all bits in this trigger word using a balanced
        માટે (uint32_t level = 0; level < WORD_SIZE_LOG2; ++level) {
            અચલ uint32_t stride = 1 << level;
            માટે (uint32_t j = 0; j < WORD_SIZE; j += 2 * stride) {
                trigps[i + j] = નવું AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = nullptr;
            }
        }

        // Set the whole word in the trigger vector
        અચલ પૂર્ણાંક wordIndex = static_cast<પૂર્ણાંક>(i / WORD_SIZE);
        AstArraySel* અચલ aselp
            = નવું AstArraySel{flp, નવું AstVarRef{flp, target, VAccess::WRITE}, wordIndex};
        trigStmtsp = Ast_ગાંઠ::addNext(trigStmtsp, નવું AstAssign{flp, aselp, trigps[i]});
    }
    પાછા trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* અચલ initFuncp,  //
                              Sen_પદાવલિ_Builder& sen_પદાવલિ_Builder,  //
                              અચલ std::vector<અચલ Ast_Sen_વૃક્ષ*>& preTreeps,  //
                              અચલ std::vector<અચલ Ast_Sen_વૃક્ષ*>& senTreeps,  //
                              અચલ string& name,  //
                              અચલ ExtraTriggers& extraTriggers,  //
                              તાર્કિક slow,  //
                              તાર્કિક useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::vector<અચલ AstSenItem*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<અચલ AstSenItem>, size_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    માટે (અચલ Ast_Sen_વૃક્ષ* અચલ senTreep : preTreeps) {
        માટે (અચલ AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            અચલ auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            જો (pair.second) senItemps.ધકેલો_back(itemp);
        }
    }
    અચલ uint32_t n_Pre_Sen_વસ્તુઓ = senItemps.size();
    V3Stats::addStat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_વસ્તુઓ);
    // Number of pre triggers, rounded up to a full word.
    અચલ uint32_t nPreTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    અચલ uint32_t n_Pre_શબ્દો = nPreTriggers / WORD_SIZE;

    // Process the rest of the trees
    માટે (અચલ Ast_Sen_વૃક્ષ* અચલ senTreep : senTreeps) {
        માટે (અચલ AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            અચલ auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            જો (pair.second) senItemps.ધકેલો_back(itemp);
        }
    }
    અચલ uint32_t n_Sen_વસ્તુઓ = senItemps.size() - nPreTriggers;
    V3Stats::addStat("Scheduling, '" + name + "' sense triggers", n_Sen_વસ્તુઓ + n_Pre_Sen_વસ્તુઓ);
    // Number of sense triggers, rounded up to a full word
    અચલ uint32_t nSenseTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    અચલ uint32_t n_Sense_શબ્દો = nSenseTriggers / WORD_SIZE;

    // Allocate space for the extra triggers
    V3Stats::addStat("Scheduling, '" + name + "' extra triggers", extraTriggers.size());
    // Number of extra triggers, rounded up to a full word.
    અચલ uint32_t nExtraTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(extraTriggers.size());
    અચલ uint32_t n_Extra_શબ્દો = nExtraTriggers / WORD_SIZE;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_શબ્દો, n_Extra_શબ્દો, n_Pre_શબ્દો, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    જો (!kit.m_n_Vec_શબ્દો) પાછા kit;

    File_લીટી* અચલ flp = netlistp->ટોચ_Scopep()->fileline();

    // Creates read/write reference
    અચલ auto rd = [flp](AstVarScope* vp) { પાછા નવું AstVarRef{flp, vp, VAccess::READ}; };
    અચલ auto wr = [flp](AstVarScope* vp) { પાછા નવું AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* અચલ dumpTrgp
        = newArgument(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* અચલ dumpTagp
        = newArgument(kit.m_dumpp, netlistp->findStringDType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* અચલ ifp = નવું AstIf{flp, નવું AstLogNot{flp, kit.newAnySetCall(dumpTrgp)}};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* અચલ cstmtp = નવું AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    અચલ auto addDebug = [&](uint32_t index, અચલ string& text) {
        અચલ પૂર્ણાંક wrdIndex = static_cast<પૂર્ણાંક>(index / WORD_SIZE);
        અચલ પૂર્ણાંક bitIndex = static_cast<પૂર્ણાંક>(index % WORD_SIZE);
        Ast_ગાંઠ_પદાવલિ* અચલ aselp = નવું AstArraySel{flp, rd(dumpTrgp), wrdIndex};
        Ast_ગાંઠ_પદાવલિ* અચલ condp = નવું AstSel{flp, aselp, bitIndex, 1};
        AstIf* અચલ ifp = નવું AstIf{flp, condp};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* અચલ cstmtp = નવું AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"' region trigger index " + std::to_string(index) + " is active: " + text
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::vector<Ast_ગાંઠ_પદાવલિ*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_ગાંઠ_Stmt* initialTrigsp = nullptr;
    માટે (size_t i = 0; i < senItemps.size(); ++i) {
        અચલ AstSenItem* અચલ senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        જો (!senItemp) {
            trigps.emplace_back(નવું AstConst{flp, AstConst::BitFalse{}});
            ચાલુ;
        }

        // Create the trigger computation expression
        અચલ auto& pair = sen_પદાવલિ_Builder.build(senItemp);
        trigps.emplace_back(pair.first);

        // Add initialization time trigger
        જો (pair.second || v3Global.opt.xInitialEdge()) {
            અચલ પૂર્ણાંક wrdIndex = static_cast<પૂર્ણાંક>(i / WORD_SIZE);
            અચલ પૂર્ણાંક bitIndex = static_cast<પૂર્ણાંક>(i % WORD_SIZE);
            Ast_ગાંઠ_પદાવલિ* અચલ wordp = નવું AstArraySel{flp, wr(kit.m_vscp), wrdIndex};
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = નવું AstSel{flp, wordp, bitIndex, 1};
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = નવું AstConst{flp, AstConst::BitTrue{}};
            જો (useAcc) {
                initFuncp->addStmtsp(નવું AstAssign{flp, lhsp, rhsp});
            } નહીંતર {
                initialTrigsp = Ast_ગાંઠ::addNext(initialTrigsp, નવું AstAssign{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_વૃક્ષ(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        addDebug(i, desc);
    }
    UASSERT(trigps.size() == nSenseTriggers, "Inconsistent number of trigger expressions");

    AstAssign* અચલ trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    માટે (unsigned i = 0; i < extraTriggers.size(); ++i) {
        addDebug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    અચલ Sen_પદાવલિ_Builder::Results senResults = sen_પદાવલિ_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    માટે (Ast_ગાંઠ_Stmt* અચલ nodep : senResults.m_inits) initFuncp->addStmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_ટોચ_Scope* અચલ ટોચ_Scopep = netlistp->ટોચ_Scopep();
    {
        AstCFunc* અચલ fp = kit.m_compVecp;
        // Trigger computation
        માટે (Ast_ગાંઠ_Stmt* અચલ nodep : senResults.m_preUpdates) fp->addStmtsp(nodep);
        fp->addStmtsp(trigStmtsp);
        માટે (Ast_ગાંઠ_Stmt* અચલ nodep : senResults.m_postUpdates) fp->addStmtsp(nodep);
        // Add the initialization time triggers
        જો (initialTrigsp) {
            AstVarScope* અચલ initVscp = ટોચ_Scopep->createTemp("__V" + name + "DidInit", 1);
            AstIf* અચલ ifp = નવું AstIf{flp, નવું AstNot{flp, rd(initVscp)}};
            fp->addStmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->addThensp(util::setVar(initVscp, 1));
            ifp->addThensp(initialTrigsp);
        }
        util::splitCheck(fp);
    };
    // If there are 'pre' triggers, compute them
    જો (kit.m_n_Pre_શબ્દો) {
        AstCFunc* અચલ fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* અચલ latchedp
            = newArgument(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* અચલ nVscp = ટોચ_Scopep->createTemp("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(સાચું);
        // Add a loop to compute the pre words
        AstLoop* અચલ loopp = નવું AstLoop{flp};
        fp->addStmtsp(util::setVar(nVscp, 0));
        fp->addStmtsp(loopp);
        // Loop body
        Ast_ગાંઠ_પદાવલિ* અચલ offsetp = નવું AstConst{flp, kit.m_n_Vec_શબ્દો};
        Ast_ગાંઠ_પદાવલિ* અચલ lIdxp = નવું AstAdd{flp, rd(nVscp), offsetp};
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = નવું AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_ગાંઠ_પદાવલિ* અચલ aWordp = નવું AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_ગાંઠ_પદાવલિ* અચલ bWordp = નવું AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = નવું AstAnd{flp, aWordp, નવું AstNot{flp, bWordp}};
        Ast_ગાંઠ_પદાવલિ* અચલ limp = નવું AstConst{flp, AstConst::Widthed_કિંમત{}, 32, n_Pre_શબ્દો};
        loopp->addStmtsp(નવું AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(nVscp));
        loopp->addStmtsp(નવું AstLoopTest{flp, loopp, નવું AstLt{flp, rd(nVscp), limp}});
        util::splitCheck(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    જો (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlinkFrBackWithNext()->delete_વૃક્ષ();
    // Done with the trigger dump function, split as might be large
    util::splitCheck(kit.m_dumpp);

    પાછા kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
વર્ગ AwaitBeforeTrigVisitor final : જાહેર VNVisitor {
    અચલ VNUser1InUse m_user1InUse;

    /**
     * AstCAwait::user1()       ->  bool.           True if node has been visited
     * AstSenTree::user1p()     ->  AstCFunc*.      Function that has to be called before awaiting
     *                                              for CAwait pointing to this SenTree
     * AstCFunc::user1p()       ->  AstVarScope*    Function's local temporary extended trigger
     *                                              vector variable scope
     */

    // Netlist - needed for using util::makeSubFunction()
    AstNetlist* અચલ m_netlistp;
    // Trigger kit - for accessing trigger vectors and mapping senItems to thier indexes
    અચલ TriggerKit& m_trigKit;
    // Expression builder - for building expressions from SenItems
    Sen_પદાવલિ_Builder& m_sen_પદાવલિ_Builder;
    // Generator of unique names for before-trigger function
    V3UniqueNames m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::vector<std::pair<AstCFunc*, Ast_Sen_વૃક્ષ*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::vector<std::pair<Ast_Sen_વૃક્ષ*, Ast_ગાંઠ_પદાવલિ*>> m_sen_વૃક્ષ_To_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_ગાંઠ>, std::vector<AstSenItem*>> m_sen_પદાવલિ_To_Sen_Item;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    સ્થિર Ast_ગાંઠ* get_Sen_Hash_ગાંઠ(અચલ AstSenItem* અચલ nodep) {
        જો (AstVarRef* અચલ varRefp = VN_CAST(nodep->sensp(), VarRef)) પાછા varRefp;
        પાછા nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    શૂન્ય fill_Sen_પદાવલિ_To_Sen_Item() {
        માટે (auto sen_વૃક્ષ_Sched : m_sen_વૃક્ષ_To_Sched) {
            Ast_Sen_વૃક્ષ* અચલ senTreep = sen_વૃક્ષ_Sched.first;

            માટે (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                અચલ VEdgeType edge = senItemp->edgeType();
                જો (edge.anEdge() || edge == VEdgeType::ET_CHANGED
                    || edge == VEdgeType::ET_HYBRID) {
                    m_sen_પદાવલિ_To_Sen_Item[*get_Sen_Hash_ગાંઠ(senItemp)].ધકેલો_back(senItemp);
                }
            }
        }

        std::vector<VNRef<Ast_ગાંઠ>> toRemove;
        માટે (અચલ auto& sen_પદાવલિ_To_Sen_વૃક્ષ : m_sen_પદાવલિ_To_Sen_Item) {
            std::vector<AstSenItem*> senItemps = sen_પદાવલિ_To_Sen_વૃક્ષ.second;
            toRemove.ધકેલો_back(sen_પદાવલિ_To_Sen_વૃક્ષ.first);
            માટે (size_t i = 1; i < senItemps.size(); ++i) {
                જો (senItemps[i]->edgeType() != senItemps[i - 1]->edgeType()) {
                    toRemove.કાઢો_back();
                    તોડો;
                }
            }
        }
        માટે (અચલ VNRef<Ast_ગાંઠ> it : toRemove) m_sen_પદાવલિ_To_Sen_Item.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<size_t, std::map<size_t, std::vector<Ast_ગાંઠ_પદાવલિ*>>>
    getUsedTriggersToTrees(અચલ std::set<size_t>& usedTriggers) {
        std::map<size_t, std::map<size_t, std::vector<Ast_ગાંઠ_પદાવલિ*>>> usedTrigsToUsingTrees;
        માટે (auto sen_વૃક્ષ_Sched : m_sen_વૃક્ષ_To_Sched) {
            અચલ Ast_Sen_વૃક્ષ* અચલ senTreep = sen_વૃક્ષ_Sched.first;
            Ast_ગાંઠ_પદાવલિ* અચલ shedp = sen_વૃક્ષ_Sched.second;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            માટે (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                અચલ size_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                જો (usedTriggers.find(idx) != usedTriggers.end()) {
                    usedTrigsToUsingTrees[idx / TriggerKit::WORD_SIZE]
                                         [size_t{1} << (idx % TriggerKit::WORD_SIZE)]
                                             .ધકેલો_back(shedp);
                }
            }
        }
        જો (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            માટે (અચલ auto& triggersToTrees : usedTrigsToUsingTrees) {
                માટે (અચલ auto& bitsToTrees : triggersToTrees.second) {
                    અચલ std::set<અચલ Ast_ગાંઠ_પદાવલિ*> exprps{bitsToTrees.second.begin(),
                                                              bitsToTrees.second.end()};
                    UASSERT(bitsToTrees.second.size() == exprps.size(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        પાછા usedTrigsToUsingTrees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_વૃક્ષ* અચલ senTreep) {
        File_લીટી* અચલ flp = senTreep->fileline();
        જો (!senTreep->user1p()) {
            AstCFunc* અચલ funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), ખોટું);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* અચલ tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(સાચું);

            AstVar* અચલ argp = નવું AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->findBasicDType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(સાચું);
            argp->direction(VDirection::આગત);
            funcp->addArgsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_back(funcp, senTreep);
        }
        AstCCall* અચલ callp = નવું AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtypeSetVoid();
        પાછા callp;
    }

    શૂન્ય visit(AstCAwait* અચલ nodep) override {
        જો (nodep->user1SetOnce()) પાછા;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        જો (અચલ AstCMethodHard* અચલ cMethodHardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            જો (cMethodHardp->method() == VCMethod::SCHED_TRIGGER) {
                AstCCall* અચલ beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_ગાંઠ* અચલ pinp = cMethodHardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, cMethodHardp, "No event description");
                beforeTrigp->addArgsp(VN_AS(pinp, ગાંઠ_પદાવલિ)->clone_વૃક્ષ(ખોટું));

                // Call the before-trigger function before the CAwait
                nodep->addHereThisAsNext(beforeTrigp->makeStmt());
                m_sen_વૃક્ષ_To_Sched.emplace_back(nodep->sentreep(), cMethodHardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* અચલ nodep) override { iterateChildren(nodep); }

જાહેર:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_પદાવલિ_Builder& sen_પદાવલિ_Builder,
                           અચલ TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_પદાવલિ_Builder{sen_પદાવલિ_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_પદાવલિ_To_Sen_Item();

        std::vector<Ast_ગાંઠ_પદાવલિ*> trigps;
        std::set<size_t> usedTriggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        માટે (અચલ auto& funcToUsedTriggers : m_generatedFuncs) {
            AstCFunc* અચલ funcp = funcToUsedTriggers.first;
            AstVarScope* અચલ vscp = VN_AS(funcp->user1p(), VarScope);
            File_લીટી* અચલ flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_વૃક્ષ* અચલ senTreep = funcToUsedTriggers.second;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                અચલ auto emplaceAt
                    = [flp, &trigps, &usedTriggers](Ast_ગાંઠ_પદાવલિ* અચલ exprp, અચલ size_t pos) {
                          અચલ size_t targetSize
                              = vlstd::roundUpToMultipleOf<TriggerKit::WORD_SIZE>(pos + 1);
                          જો (trigps.capacity() < targetSize) trigps.reserve(targetSize * 2);
                          જ્યારે (trigps.size() < targetSize) {
                              trigps.ધકેલો_back(નવું AstConst{flp, AstConst::BitFalse{}});
                          }
                          trigps[pos]->delete_વૃક્ષ();
                          trigps[pos] = exprp;
                          usedTriggers.ઉમેરો(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                માટે (અચલ AstSenItem* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), SenItem)) {
                    અચલ size_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_પદાવલિ_Builder.build(itemp).first, idx);
                    auto iter = m_sen_પદાવલિ_To_Sen_Item.find(*get_Sen_Hash_ગાંઠ(itemp));
                    જો (iter != m_sen_પદાવલિ_To_Sen_Item.end()) {
                        માટે (AstSenItem* અચલ additionalItemp : iter->second) {
                            અચલ size_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_પદાવલિ_Builder.build(additionalItemp).first, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                અચલ Sen_પદાવલિ_Builder::Results results
                    = m_sen_પદાવલિ_Builder.getResultsAndClearUpdates();
                માટે (Ast_ગાંઠ_Stmt* અચલ stmtsp : results.m_inits) funcp->addStmtsp(stmtsp);
                માટે (Ast_ગાંઠ_Stmt* અચલ stmtsp : results.m_preUpdates) funcp->addStmtsp(stmtsp);
                funcp->addStmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                માટે (Ast_ગાંઠ_Stmt* અચલ stmtsp : results.m_postUpdates) funcp->addStmtsp(stmtsp);
            }

            અચલ std::map<size_t, std::map<size_t, std::vector<Ast_ગાંઠ_પદાવલિ*>>>
                usedTrigsToUsingTrees = getUsedTriggersToTrees(usedTriggers);
            usedTriggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            અચલ auto getIdx = [flp](AstVarScope* અચલ scocep, VAccess access, size_t idx) {
                પાછા નવું AstArraySel{flp, નવું AstVarRef{flp, scocep, access},
                                       નવું AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* અચલ argpVscp = નવું AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->addVarsp(argpVscp);

            // Mark as ready triggered schedulers
            માટે (અચલ auto& triggersToTrees : usedTrigsToUsingTrees) {
                અચલ size_t word = triggersToTrees.first;

                માટે (અચલ auto& bitsToTrees : triggersToTrees.second) {
                    અચલ size_t bit = bitsToTrees.first;
                    અચલ auto& schedulers = bitsToTrees.second;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* અચલ maskConstp = નવું AstConst{flp, AstConst::Unsized64{}, bit};
                    AstAnd* અચલ condp
                        = નવું AstAnd{flp, getIdx(vscp, VAccess::READ, word), maskConstp};
                    AstIf* અચલ ifp = નવું AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    માટે (Ast_ગાંઠ_પદાવલિ* અચલ schedp : schedulers) {
                        AstCMethodHard* અચલ callp = નવું AstCMethodHard{
                            flp, schedp->clone_વૃક્ષ(ખોટું), VCMethod::SCHED_READY};
                        callp->dtypeSetVoid();
                        callp->addPinsp(નવું AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->addThensp(callp->makeStmt());
                    }
                    funcp->addStmtsp(ifp);
                }
            }

            AstVarScope* અચલ vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            માટે (અચલ auto& triggersToTrees : usedTrigsToUsingTrees) {
                અચલ size_t word = triggersToTrees.first;
                funcp->addStmtsp(નવું AstAssign{flp, getIdx(vscAccp, VAccess::WRITE, word),
                                               નવું AstOr{flp, getIdx(vscAccp, VAccess::READ, word),
                                                         getIdx(vscp, VAccess::READ, word)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = મૂળભૂત;
};

શૂન્ય beforeTrigVisitor(AstNetlist* netlistp, Sen_પદાવલિ_Builder& sen_પદાવલિ_Builder,
                       અચલ TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_પદાવલિ_Builder, trigKit};
}

}  // namespace V3Sched
