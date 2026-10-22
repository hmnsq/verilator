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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Const.h"
#অন্তর্ভাৱ "V3EmitCBase.h"
#অন্তর্ভাৱ "V3EmitV.h"
#অন্তর্ভাৱ "V3Order.h"
#অন্তর্ভাৱ "V3Sched.h"
#অন্তর্ভাৱ "V3SenExprBuilder.h"
#অন্তর্ভাৱ "V3Stats.h"
#অন্তর্ভাৱ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ V3Sched {

নামাকাশ {

using util::newArgument;

AstVarScope* newLocal(AstCFunc* funcp, Ast_গ্ৰন্থি_DType* dtypep, ধ্রুৱ std::string& name) {
    File_সাৰি* ধ্রুৱ flp = funcp->fileline();
    AstScope* ধ্রুৱ scopep = funcp->scopep();
    AstVar* ধ্রুৱ varp = নূতন AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(সঁচা);
    varp->noReset(সঁচা);
    funcp->addVarsp(varp);
    AstVarScope* ধ্রুৱ vscp = নূতন AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    ঘূৰাই vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() ধ্রুৱ {
    UASSERT(m_n_Pre_শব্দগুলি, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ধ্রুৱ netlistp = v3Global.rootp();
    File_সাৰি* ধ্রুৱ flp = netlistp->শীৰ্ষ_Scopep()->fileline();
    Ast_গ্ৰন্থি_DType* ধ্রুৱ u32DTypep = netlistp->findUInt32DType();
    Ast_গ্ৰন্থি_DType* ধ্রুৱ strDtypep = netlistp->findStringDType();

    // Dumping function always slow
    ধ্রুৱ std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* ধ্রুৱ funcp = util::makeSubFunction(netlistp, name, সঁচা);
    funcp->isStatic(সঁচা);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* ধ্রুৱ eVscp = newArgument(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* ধ্রুৱ tVscp = newArgument(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    ধ্রুৱ auto rd = [flp](AstVarScope* vp) { ঘূৰাই নূতন AstVarRef{flp, vp, VAccess::READ}; };
    ধ্রুৱ auto wr = [flp](AstVarScope* vp) { ঘূৰাই নূতন AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* ধ্রুৱ vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* ধ্রুৱ iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->addStmtsp(util::setVar(iVscp, 0));
        // Add loop
        AstLoop* ধ্রুৱ loopp = নূতন AstLoop{flp};
        funcp->addStmtsp(loopp);
        // Loop body
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = নূতন AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = নূতন AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ limp = নূতন AstConst{flp, AstConst::Widthed_মান{}, 32, m_n_Vec_শব্দগুলি};
        loopp->addStmtsp(নূতন AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(iVscp));
        loopp->addStmtsp(নূতন AstLoopTest{flp, loopp, নূতন AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(vVscp));
        callp->addArgsp(rd(tVscp));
        funcp->addStmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* ধ্রুৱ pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* ধ্রুৱ jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->addStmtsp(util::setVar(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* ধ্রুৱ loopp = নূতন AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = নূতন AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = নূতন AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ limp = নূতন AstConst{flp, AstConst::Widthed_মান{}, 32, m_n_Pre_শব্দগুলি};
            loopp->addStmtsp(নূতন AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(নূতন AstLoopTest{flp, loopp, নূতন AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* ধ্রুৱ loopp = নূতন AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = নূতন AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = নূতন AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ limp = নূতন AstConst{flp, AstConst::Widthed_মান{}, 32, m_n_Vec_শব্দগুলি};
            loopp->addStmtsp(নূতন AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(নূতন AstLoopTest{flp, loopp, নূতন AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(pVscp));
        callp->addArgsp(
            নূতন AstConcatN{flp, rd(tVscp), নূতন AstConst{flp, AstConst::String{}, " pre"}});
        funcp->addStmtsp(callp->makeStmt());
    }

    ঘূৰাই funcp;
}

AstCFunc* TriggerKit::createAnySetFunc(AstUnpackArrayDType* ধ্রুৱ dtypep) ধ্রুৱ {
    AstNetlist* ধ্রুৱ netlistp = v3Global.rootp();
    File_সাৰি* ধ্রুৱ flp = netlistp->শীৰ্ষ_Scopep()->fileline();
    Ast_গ্ৰন্থি_DType* ধ্রুৱ u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* ধ্রুৱ funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(সঁচা);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* ধ্রুৱ iVscp = newArgument(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ধ্রুৱ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    ধ্রুৱ auto rd = [flp](AstVarScope* vp) { ঘূৰাই নূতন AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* ধ্রুৱ loopp = নূতন AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);
    funcp->addStmtsp(নূতন AstCReturn{flp, নূতন AstConst{flp, AstConst::BitFalse{}}});

    // Loop body
    ধ্রুৱ uint32_t n_শব্দগুলি = dtypep->elementsConst();
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ condp = নূতন AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_গ্ৰন্থি_Stmt* ধ্রুৱ thenp = নূতন AstCReturn{flp, নূতন AstConst{flp, AstConst::BitTrue{}}};
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ limp = নূতন AstConst{flp, AstConst::Widthed_মান{}, 32, n_শব্দগুলি};
    loopp->addStmtsp(নূতন AstIf{flp, condp, thenp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(নূতন AstLoopTest{flp, loopp, নূতন AstLt{flp, rd(nVscp), limp}});

    // Done
    ঘূৰাই funcp;
}
AstCFunc* TriggerKit::createClearFunc() ধ্রুৱ {
    AstNetlist* ধ্রুৱ netlistp = v3Global.rootp();
    File_সাৰি* ধ্রুৱ flp = netlistp->শীৰ্ষ_Scopep()->fileline();
    Ast_গ্ৰন্থি_DType* ধ্রুৱ u32DTypep = netlistp->findUInt32DType();

    // Create function
    AstCFunc* ধ্রুৱ funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(সঁচা);

    // Add arguments
    AstVarScope* ধ্রুৱ oVscp = newArgument(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* ধ্রুৱ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ধ্রুৱ auto rd = [flp](AstVarScope* vp) { ঘূৰাই নূতন AstVarRef{flp, vp, VAccess::READ}; };
    ধ্রুৱ auto wr = [flp](AstVarScope* vp) { ঘূৰাই নূতন AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ধ্রুৱ loopp = নূতন AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = নূতন AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = নূতন AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ limp = নূতন AstConst{flp, AstConst::Widthed_মান{}, 32, m_n_Vec_শব্দগুলি};
    loopp->addStmtsp(নূতন AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(নূতন AstLoopTest{flp, loopp, নূতন AstLt{flp, rd(nVscp), limp}});

    // Done
    ঘূৰাই funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ধ্রুৱ oDtypep,
                                       AstUnpackArrayDType* ধ্রুৱ iDtypep) ধ্রুৱ {
    AstNetlist* ধ্রুৱ netlistp = v3Global.rootp();
    File_সাৰি* ধ্রুৱ flp = netlistp->শীৰ্ষ_Scopep()->fileline();
    Ast_গ্ৰন্থি_DType* ধ্রুৱ u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* ধ্রুৱ funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(সঁচা);

    // Add arguments
    AstVarScope* ধ্রুৱ oVscp = newArgument(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* ধ্রুৱ iVscp = newArgument(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ধ্রুৱ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ধ্রুৱ auto rd = [flp](AstVarScope* vp) { ঘূৰাই নূতন AstVarRef{flp, vp, VAccess::READ}; };
    ধ্রুৱ auto wr = [flp](AstVarScope* vp) { ঘূৰাই নূতন AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ধ্রুৱ loopp = নূতন AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = নূতন AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ oWordp = নূতন AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ iWordp = নূতন AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = নূতন AstOr{flp, oWordp, iWordp};
    AstConst* ধ্রুৱ outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ধ্রুৱ প্ৰবেশ_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ limp = outputRangeLeftp->অঙ্ক().toSInt() < প্ৰবেশ_Range_Leftp->অঙ্ক().toSInt()
                                  ? outputRangeLeftp->clone_বৃক্ষ_Pure(মিছা)
                                  : প্ৰবেশ_Range_Leftp->clone_বৃক্ষ_Pure(মিছা);
    loopp->addStmtsp(নূতন AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(নূতন AstLoopTest{flp, loopp, নূতন AstLte{flp, rd(nVscp), limp}});

    // Done
    ঘূৰাই funcp;
}

Ast_গ্ৰন্থি_ৰাশিমালা* TriggerKit::newAnySetCall(AstVarScope* ধ্রুৱ vscp) ধ্রুৱ {
    File_সাৰি* ধ্রুৱ flp = v3Global.rootp()->শীৰ্ষ_Scopep()->fileline();
    যদি (!m_n_Vec_শব্দগুলি) ঘূৰাই নূতন AstConst{flp, AstConst::BitFalse{}};

    AstCFunc* funcp = nullptr;
    যদি (vscp->dtypep() == m_trigVecDTypep) {
        যদি (!m_anySetVecp) m_anySetVecp = createAnySetFunc(m_trigVecDTypep);
        funcp = m_anySetVecp;
    } অন্যথা যদি (vscp->dtypep() == m_trigExtDTypep) {
        যদি (!m_anySetExtp) m_anySetExtp = createAnySetFunc(m_trigExtDTypep);
        funcp = m_anySetExtp;
    } অন্যথা {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, funcp};
    callp->addArgsp(নূতন AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetBit();
    ঘূৰাই callp;
}
Ast_গ্ৰন্থি_Stmt* TriggerKit::newClearCall(AstVarScope* ধ্রুৱ vscp) ধ্রুৱ {
    যদি (!m_n_Vec_শব্দগুলি) ঘূৰাই nullptr;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    যদি (!m_clearp) m_clearp = createClearFunc();
    File_সাৰি* ধ্রুৱ flp = v3Global.rootp()->শীৰ্ষ_Scopep()->fileline();
    AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, m_clearp};
    callp->addArgsp(নূতন AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetVoid();
    ঘূৰাই callp->makeStmt();
}
Ast_গ্ৰন্থি_Stmt* TriggerKit::newOrIntoCall(AstVarScope* ধ্রুৱ oVscp, AstVarScope* ধ্রুৱ iVscp) ধ্রুৱ {
    যদি (!m_n_Vec_শব্দগুলি) ঘূৰাই nullptr;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    ধ্রুৱ size_t mask
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[mask];
    যদি (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_সাৰি* ধ্রুৱ flp = v3Global.rootp()->শীৰ্ষ_Scopep()->fileline();
    AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, funcp};
    callp->addArgsp(নূতন AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->addArgsp(নূতন AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtypeSetVoid();
    ঘূৰাই callp->makeStmt();
}

Ast_গ্ৰন্থি_Stmt* TriggerKit::newCompBaseCall() ধ্রুৱ {
    যদি (!m_n_Vec_শব্দগুলি) ঘূৰাই nullptr;
    File_সাৰি* ধ্রুৱ flp = v3Global.rootp()->শীৰ্ষ_Scopep()->fileline();
    AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, m_compVecp};
    callp->dtypeSetVoid();
    ঘূৰাই callp->makeStmt();
}

Ast_গ্ৰন্থি_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ধ্রুৱ {
    যদি (!m_n_Pre_শব্দগুলি) ঘূৰাই nullptr;
    File_সাৰি* ধ্রুৱ flp = v3Global.rootp()->শীৰ্ষ_Scopep()->fileline();
    AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, m_compExtp};
    callp->addArgsp(নূতন AstVarRef{flp, vscp, VAccess::READ});
    callp->dtypeSetVoid();
    ঘূৰাই callp->makeStmt();
}

Ast_গ্ৰন্থি_Stmt* TriggerKit::newDumpCall(AstVarScope* ধ্রুৱ vscp, ধ্রুৱ std::string& tag,
                                     তার্কিক debugOnly) ধ্রুৱ {
    যদি (!m_n_Vec_শব্দগুলি) ঘূৰাই nullptr;
    AstCFunc* funcp = nullptr;
    যদি (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } অন্যথা যদি (vscp->dtypep() == m_trigExtDTypep) {
        যদি (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } অন্যথা {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_সাৰি* ধ্রুৱ flp = v3Global.rootp()->শীৰ্ষ_Scopep()->fileline();
    AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, funcp};
    callp->addArgsp(নূতন AstVarRef{flp, vscp, VAccess::READ});
    callp->addArgsp(নূতন AstConst{flp, AstConst::String{}, tag});
    callp->dtypeSetVoid();
    AstCStmt* ধ্রুৱ cstmtp = নূতন AstCStmt{flp};
    cstmtp->add("#ifdef VL_DEBUG\n");
    যদি (debugOnly) {
        cstmtp->add("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->add(callp->makeStmt());
        cstmtp->add("}\n");
    } অন্যথা {
        cstmtp->add(callp->makeStmt());
    }
    cstmtp->add("#endif");
    ঘূৰাই cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(ধ্রুৱ std::string& name) ধ্রুৱ {
    যদি (!m_n_Vec_শব্দগুলি) ঘূৰাই nullptr;
    ঘূৰাই v3Global.rootp()->শীৰ্ষ_Scopep()->createTemp("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extraTriggerIndex(uint32_t index) ধ্রুৱ {
    UASSERT(index <= m_n_Extra_শব্দগুলি * WORD_SIZE, "Invalid external trigger index");
    ঘূৰাই index + m_n_Sense_শব্দগুলি * WORD_SIZE;
}

std::set<uint32_t> TriggerKit::triggerIndices(ধ্রুৱ Ast_Sen_বৃক্ষ* senTreep, তার্কিক pre) ধ্রুৱ {
    std::set<uint32_t> indices;
    বাবে (ধ্রুৱ AstSenItem* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), SenItem)) {
        uint32_t index = m_senItem2TrigIdx.at(*sip);
        যদি (pre) {
            UASSERT_OBJ(index < m_n_Pre_শব্দগুলি * WORD_SIZE, sip, "Not a 'pre' trigger");
            index += m_n_Vec_শব্দগুলি * WORD_SIZE;
        }
        indices.ঢোকাও(index);
    }
    ঘূৰাই indices;
}

Ast_গ্ৰন্থি_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ধ্রুৱ {
    index = extraTriggerIndex(index);
    ধ্রুৱ uint32_t wordIndex = index / WORD_SIZE;
    ধ্রুৱ uint32_t bitIndex = index % WORD_SIZE;
    File_সাৰি* ধ্রুৱ flp = vscp->fileline();
    AstVarRef* ধ্রুৱ refp = নূতন AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ wordp = নূতন AstArraySel{flp, refp, static_cast<পূর্ণাঙ্ক>(wordIndex)};
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ trigLhsp = নূতন AstSel{flp, wordp, static_cast<পূর্ণাঙ্ক>(bitIndex), 1};
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ trigRhsp = নূতন AstVarRef{flp, vscp, VAccess::READ};
    ঘূৰাই নূতন AstAssign{flp, trigLhsp, trigRhsp};
}

শূন্য TriggerKit::addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ধ্রুৱ {
    File_সাৰি* ধ্রুৱ flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_গ্ৰন্থি* ধ্রুৱ setp = newExtraTriggerAssignment(vscp, index);
    setp->addNext(নূতন AstAssign{flp, নূতন AstVarRef{flp, vscp, VAccess::WRITE},
                                নূতন AstConst{flp, AstConst::BitFalse{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
শূন্য TriggerKit::add_মান_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t index) ধ্রুৱ {
    index += m_n_Sense_শব্দগুলি * WORD_SIZE;
    ধ্রুৱ uint32_t wordIndex = index / WORD_SIZE;
    ধ্রুৱ uint32_t bitIndex = index % WORD_SIZE;

    Ast_শীৰ্ষ_Scope* ধ্রুৱ শীৰ্ষ_Scopep = netlistp->শীৰ্ষ_Scopep();
    File_সাৰি* ধ্রুৱ flp = netlistp->fileline();
    Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    যদি (ধ্রুৱ AstBasicDType* ধ্রুৱ bdtypep = VN_CAST(dtypep, BasicDType)) {
        যদি (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            ঘূৰাই;
        }
    } অন্যথা যদি (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, গ্ৰন্থি_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        ঘূৰাই;
    }

    ধ্রুৱ auto rdInst = [flp](AstVarScope* vp) { ঘূৰাই নূতন AstVarRef{flp, vp, VAccess::READ}; };
    ধ্রুৱ auto wrPrev = [flp](AstVarScope* vp) { ঘূৰাই নূতন AstVarRef{flp, vp, VAccess::WRITE}; };
    ধ্রুৱ auto rdPrev = [flp](AstVarScope* vp) { ঘূৰাই নূতন AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    ধ্রুৱ std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* ধ্রুৱ prevVscp = শীৰ্ষ_Scopep->createTemp(prevName, instVscp->dtypep());

    // Initialize prev = inst
    যদি (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ধ্রুৱ cmhp = নূতন AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        initFuncp->addStmtsp(cmhp->makeStmt());
    } অন্যথা {
        initFuncp->addStmtsp(নূতন AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_গ্ৰন্থি_ৰাশিমালা* neqp;
    যদি (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ধ্রুৱ cmhp
            = নূতন AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtypeSetBit();
        neqp = cmhp;
    } অন্যথা {
        neqp = নূতন AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_গ্ৰন্থি* updp;
    যদি (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ধ্রুৱ cmhp = নূতন AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        updp = cmhp->makeStmt();
    } অন্যথা {
        updp = নূতন AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ধ্রুৱ refp = নূতন AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ wordp = নূতন AstArraySel{flp, refp, static_cast<পূর্ণাঙ্ক>(wordIndex)};
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ trigLhsp = নূতন AstSel{flp, wordp, static_cast<পূর্ণাঙ্ক>(bitIndex), 1};
    Ast_গ্ৰন্থি* ধ্রুৱ setp = নূতন AstAssign{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->addNext(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ধ্রুৱ std::string& name, তার্কিক slow, uint32_t n_Sense_শব্দগুলি,
                       uint32_t n_Extra_শব্দগুলি, uint32_t n_Pre_শব্দগুলি,
                       std::unordered_map<VNRef<ধ্রুৱ AstSenItem>, size_t> senItem2TrigIdx,
                       ধ্রুৱ std::vector<ধ্রুৱ AstSenItem*>& senItemps, তার্কিক useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_শব্দগুলি{n_Sense_শব্দগুলি}
    , m_n_Extra_শব্দগুলি{n_Extra_শব্দগুলি}
    , m_n_Pre_শব্দগুলি{n_Pre_শব্দগুলি}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trigIdx2SenItem{senItemps} {
    // If no triggers, we don't need to generate anything
    যদি (!m_n_Vec_শব্দগুলি) ঘূৰাই;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trigIdx2SenItem.resize((m_n_Vec_শব্দগুলি + m_n_Pre_শব্দগুলি) * WORD_SIZE);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_শব্দগুলি * WORD_SIZE,
              m_trigIdx2SenItem.begin() + m_n_Vec_শব্দগুলি * WORD_SIZE);
    AstNetlist* ধ্রুৱ netlistp = v3Global.rootp();
    AstScope* ধ্রুৱ scopep = netlistp->শীৰ্ষ_Scopep()->scopep();
    File_সাৰি* ধ্রুৱ flp = scopep->fileline();
    // Data type of a single trigger word
    m_wordDTypep = netlistp->findBitDType(WORD_SIZE, WORD_SIZE, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ধ্রুৱ rp = নূতন AstRange{flp, static_cast<পূর্ণাঙ্ক>(m_n_Vec_শব্দগুলি - 1), 0};
    m_trigVecDTypep = নূতন AstUnpackArrayDType{flp, m_wordDTypep, rp};
    netlistp->typeTablep()->addTypesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    যদি (m_n_Pre_শব্দগুলি) {
        AstRange* ধ্রুৱ ep = নূতন AstRange{flp, static_cast<পূর্ণাঙ্ক>(m_n_Vec_শব্দগুলি + m_n_Pre_শব্দগুলি - 1), 0};
        m_trigExtDTypep = নূতন AstUnpackArrayDType{flp, m_wordDTypep, ep};
        netlistp->typeTablep()->addTypesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } অন্যথা {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->শীৰ্ষ_Scopep()->createTemp("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->isInternal(সঁচা);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, সঁচা);
    m_dumpp->isStatic(সঁচা);
    m_dumpp->ifdef("VL_DEBUG");
    যদি (useAcc) {
        m_vscAccp
            = netlistp->শীৰ্ষ_Scopep()->createTemp("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->isInternal(সঁচা);
    }
}

AstAssign* TriggerKit::createSenTrigVecAssignment(AstVarScope* ধ্রুৱ target,
                                                  std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*>& trigps) {
    File_সাৰি* ধ্রুৱ flp = target->fileline();
    AstAssign* trigStmtsp = nullptr;
    // Assign sense triggers vector one word at a time
    বাবে (size_t i = 0; i < trigps.size(); i += WORD_SIZE) {
        // Concatenate all bits in this trigger word using a balanced
        বাবে (uint32_t level = 0; level < WORD_SIZE_LOG2; ++level) {
            ধ্রুৱ uint32_t stride = 1 << level;
            বাবে (uint32_t j = 0; j < WORD_SIZE; j += 2 * stride) {
                trigps[i + j] = নূতন AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = nullptr;
            }
        }

        // Set the whole word in the trigger vector
        ধ্রুৱ পূর্ণাঙ্ক wordIndex = static_cast<পূর্ণাঙ্ক>(i / WORD_SIZE);
        AstArraySel* ধ্রুৱ aselp
            = নূতন AstArraySel{flp, নূতন AstVarRef{flp, target, VAccess::WRITE}, wordIndex};
        trigStmtsp = Ast_গ্ৰন্থি::addNext(trigStmtsp, নূতন AstAssign{flp, aselp, trigps[i]});
    }
    ঘূৰাই trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ধ্রুৱ initFuncp,  //
                              Sen_ৰাশিমালা_Builder& sen_ৰাশিমালা_Builder,  //
                              ধ্রুৱ std::vector<ধ্রুৱ Ast_Sen_বৃক্ষ*>& preTreeps,  //
                              ধ্রুৱ std::vector<ধ্রুৱ Ast_Sen_বৃক্ষ*>& senTreeps,  //
                              ধ্রুৱ string& name,  //
                              ধ্রুৱ ExtraTriggers& extraTriggers,  //
                              তার্কিক slow,  //
                              তার্কিক useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::vector<ধ্রুৱ AstSenItem*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<ধ্রুৱ AstSenItem>, size_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    বাবে (ধ্রুৱ Ast_Sen_বৃক্ষ* ধ্রুৱ senTreep : preTreeps) {
        বাবে (ধ্রুৱ AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ধ্রুৱ auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            যদি (pair.second) senItemps.ঠেলো_back(itemp);
        }
    }
    ধ্রুৱ uint32_t n_Pre_Sen_বস্তুসমূহ = senItemps.size();
    V3Stats::addStat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_বস্তুসমূহ);
    // Number of pre triggers, rounded up to a full word.
    ধ্রুৱ uint32_t nPreTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ধ্রুৱ uint32_t n_Pre_শব্দগুলি = nPreTriggers / WORD_SIZE;

    // Process the rest of the trees
    বাবে (ধ্রুৱ Ast_Sen_বৃক্ষ* ধ্রুৱ senTreep : senTreeps) {
        বাবে (ধ্রুৱ AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ধ্রুৱ auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            যদি (pair.second) senItemps.ঠেলো_back(itemp);
        }
    }
    ধ্রুৱ uint32_t n_Sen_বস্তুসমূহ = senItemps.size() - nPreTriggers;
    V3Stats::addStat("Scheduling, '" + name + "' sense triggers", n_Sen_বস্তুসমূহ + n_Pre_Sen_বস্তুসমূহ);
    // Number of sense triggers, rounded up to a full word
    ধ্রুৱ uint32_t nSenseTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ধ্রুৱ uint32_t n_Sense_শব্দগুলি = nSenseTriggers / WORD_SIZE;

    // Allocate space for the extra triggers
    V3Stats::addStat("Scheduling, '" + name + "' extra triggers", extraTriggers.size());
    // Number of extra triggers, rounded up to a full word.
    ধ্রুৱ uint32_t nExtraTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(extraTriggers.size());
    ধ্রুৱ uint32_t n_Extra_শব্দগুলি = nExtraTriggers / WORD_SIZE;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_শব্দগুলি, n_Extra_শব্দগুলি, n_Pre_শব্দগুলি, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    যদি (!kit.m_n_Vec_শব্দগুলি) ঘূৰাই kit;

    File_সাৰি* ধ্রুৱ flp = netlistp->শীৰ্ষ_Scopep()->fileline();

    // Creates read/write reference
    ধ্রুৱ auto rd = [flp](AstVarScope* vp) { ঘূৰাই নূতন AstVarRef{flp, vp, VAccess::READ}; };
    ধ্রুৱ auto wr = [flp](AstVarScope* vp) { ঘূৰাই নূতন AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* ধ্রুৱ dumpTrgp
        = newArgument(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* ধ্রুৱ dumpTagp
        = newArgument(kit.m_dumpp, netlistp->findStringDType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* ধ্রুৱ ifp = নূতন AstIf{flp, নূতন AstLogNot{flp, kit.newAnySetCall(dumpTrgp)}};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* ধ্রুৱ cstmtp = নূতন AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    ধ্রুৱ auto addDebug = [&](uint32_t index, ধ্রুৱ string& text) {
        ধ্রুৱ পূর্ণাঙ্ক wrdIndex = static_cast<পূর্ণাঙ্ক>(index / WORD_SIZE);
        ধ্রুৱ পূর্ণাঙ্ক bitIndex = static_cast<পূর্ণাঙ্ক>(index % WORD_SIZE);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ aselp = নূতন AstArraySel{flp, rd(dumpTrgp), wrdIndex};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ condp = নূতন AstSel{flp, aselp, bitIndex, 1};
        AstIf* ধ্রুৱ ifp = নূতন AstIf{flp, condp};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* ধ্রুৱ cstmtp = নূতন AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"' region trigger index " + std::to_string(index) + " is active: " + text
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_গ্ৰন্থি_Stmt* initialTrigsp = nullptr;
    বাবে (size_t i = 0; i < senItemps.size(); ++i) {
        ধ্রুৱ AstSenItem* ধ্রুৱ senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        যদি (!senItemp) {
            trigps.emplace_back(নূতন AstConst{flp, AstConst::BitFalse{}});
            অনুৱর্ত;
        }

        // Create the trigger computation expression
        ধ্রুৱ auto& pair = sen_ৰাশিমালা_Builder.build(senItemp);
        trigps.emplace_back(pair.first);

        // Add initialization time trigger
        যদি (pair.second || v3Global.opt.xInitialEdge()) {
            ধ্রুৱ পূর্ণাঙ্ক wrdIndex = static_cast<পূর্ণাঙ্ক>(i / WORD_SIZE);
            ধ্রুৱ পূর্ণাঙ্ক bitIndex = static_cast<পূর্ণাঙ্ক>(i % WORD_SIZE);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ wordp = নূতন AstArraySel{flp, wr(kit.m_vscp), wrdIndex};
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = নূতন AstSel{flp, wordp, bitIndex, 1};
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = নূতন AstConst{flp, AstConst::BitTrue{}};
            যদি (useAcc) {
                initFuncp->addStmtsp(নূতন AstAssign{flp, lhsp, rhsp});
            } অন্যথা {
                initialTrigsp = Ast_গ্ৰন্থি::addNext(initialTrigsp, নূতন AstAssign{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_বৃক্ষ(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        addDebug(i, desc);
    }
    UASSERT(trigps.size() == nSenseTriggers, "Inconsistent number of trigger expressions");

    AstAssign* ধ্রুৱ trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    বাবে (unsigned i = 0; i < extraTriggers.size(); ++i) {
        addDebug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    ধ্রুৱ Sen_ৰাশিমালা_Builder::Results senResults = sen_ৰাশিমালা_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    বাবে (Ast_গ্ৰন্থি_Stmt* ধ্রুৱ nodep : senResults.m_inits) initFuncp->addStmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_শীৰ্ষ_Scope* ধ্রুৱ শীৰ্ষ_Scopep = netlistp->শীৰ্ষ_Scopep();
    {
        AstCFunc* ধ্রুৱ fp = kit.m_compVecp;
        // Trigger computation
        বাবে (Ast_গ্ৰন্থি_Stmt* ধ্রুৱ nodep : senResults.m_preUpdates) fp->addStmtsp(nodep);
        fp->addStmtsp(trigStmtsp);
        বাবে (Ast_গ্ৰন্থি_Stmt* ধ্রুৱ nodep : senResults.m_postUpdates) fp->addStmtsp(nodep);
        // Add the initialization time triggers
        যদি (initialTrigsp) {
            AstVarScope* ধ্রুৱ initVscp = শীৰ্ষ_Scopep->createTemp("__V" + name + "DidInit", 1);
            AstIf* ধ্রুৱ ifp = নূতন AstIf{flp, নূতন AstNot{flp, rd(initVscp)}};
            fp->addStmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->addThensp(util::setVar(initVscp, 1));
            ifp->addThensp(initialTrigsp);
        }
        util::splitCheck(fp);
    };
    // If there are 'pre' triggers, compute them
    যদি (kit.m_n_Pre_শব্দগুলি) {
        AstCFunc* ধ্রুৱ fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ধ্রুৱ latchedp
            = newArgument(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ধ্রুৱ nVscp = শীৰ্ষ_Scopep->createTemp("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(সঁচা);
        // Add a loop to compute the pre words
        AstLoop* ধ্রুৱ loopp = নূতন AstLoop{flp};
        fp->addStmtsp(util::setVar(nVscp, 0));
        fp->addStmtsp(loopp);
        // Loop body
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ offsetp = নূতন AstConst{flp, kit.m_n_Vec_শব্দগুলি};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lIdxp = নূতন AstAdd{flp, rd(nVscp), offsetp};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = নূতন AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ aWordp = নূতন AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ bWordp = নূতন AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = নূতন AstAnd{flp, aWordp, নূতন AstNot{flp, bWordp}};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ limp = নূতন AstConst{flp, AstConst::Widthed_মান{}, 32, n_Pre_শব্দগুলি};
        loopp->addStmtsp(নূতন AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(nVscp));
        loopp->addStmtsp(নূতন AstLoopTest{flp, loopp, নূতন AstLt{flp, rd(nVscp), limp}});
        util::splitCheck(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    যদি (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlinkFrBackWithNext()->delete_বৃক্ষ();
    // Done with the trigger dump function, split as might be large
    util::splitCheck(kit.m_dumpp);

    ঘূৰাই kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
শ্ৰেণী AwaitBeforeTrigVisitor final : সার্ৱজনিক VNVisitor {
    ধ্রুৱ VNUser1InUse m_user1InUse;

    /**
     * AstCAwait::user1()       ->  bool.           True if node has been visited
     * AstSenTree::user1p()     ->  AstCFunc*.      Function that has to be called before awaiting
     *                                              for CAwait pointing to this SenTree
     * AstCFunc::user1p()       ->  AstVarScope*    Function's local temporary extended trigger
     *                                              vector variable scope
     */

    // Netlist - needed for using util::makeSubFunction()
    AstNetlist* ধ্রুৱ m_netlistp;
    // Trigger kit - for accessing trigger vectors and mapping senItems to thier indexes
    ধ্রুৱ TriggerKit& m_trigKit;
    // Expression builder - for building expressions from SenItems
    Sen_ৰাশিমালা_Builder& m_sen_ৰাশিমালা_Builder;
    // Generator of unique names for before-trigger function
    V3UniqueNames m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::vector<std::pair<AstCFunc*, Ast_Sen_বৃক্ষ*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::vector<std::pair<Ast_Sen_বৃক্ষ*, Ast_গ্ৰন্থি_ৰাশিমালা*>> m_sen_বৃক্ষ_To_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_গ্ৰন্থি>, std::vector<AstSenItem*>> m_sen_ৰাশিমালা_To_Sen_Item;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    স্থির Ast_গ্ৰন্থি* get_Sen_Hash_গ্ৰন্থি(ধ্রুৱ AstSenItem* ধ্রুৱ nodep) {
        যদি (AstVarRef* ধ্রুৱ varRefp = VN_CAST(nodep->sensp(), VarRef)) ঘূৰাই varRefp;
        ঘূৰাই nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    শূন্য fill_Sen_ৰাশিমালা_To_Sen_Item() {
        বাবে (auto sen_বৃক্ষ_Sched : m_sen_বৃক্ষ_To_Sched) {
            Ast_Sen_বৃক্ষ* ধ্রুৱ senTreep = sen_বৃক্ষ_Sched.first;

            বাবে (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ধ্রুৱ VEdgeType edge = senItemp->edgeType();
                যদি (edge.anEdge() || edge == VEdgeType::ET_CHANGED
                    || edge == VEdgeType::ET_HYBRID) {
                    m_sen_ৰাশিমালা_To_Sen_Item[*get_Sen_Hash_গ্ৰন্থি(senItemp)].ঠেলো_back(senItemp);
                }
            }
        }

        std::vector<VNRef<Ast_গ্ৰন্থি>> toRemove;
        বাবে (ধ্রুৱ auto& sen_ৰাশিমালা_To_Sen_বৃক্ষ : m_sen_ৰাশিমালা_To_Sen_Item) {
            std::vector<AstSenItem*> senItemps = sen_ৰাশিমালা_To_Sen_বৃক্ষ.second;
            toRemove.ঠেলো_back(sen_ৰাশিমালা_To_Sen_বৃক্ষ.first);
            বাবে (size_t i = 1; i < senItemps.size(); ++i) {
                যদি (senItemps[i]->edgeType() != senItemps[i - 1]->edgeType()) {
                    toRemove.বেৰ_কৰো_back();
                    ভঙ্গ;
                }
            }
        }
        বাবে (ধ্রুৱ VNRef<Ast_গ্ৰন্থি> it : toRemove) m_sen_ৰাশিমালা_To_Sen_Item.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<size_t, std::map<size_t, std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*>>>
    getUsedTriggersToTrees(ধ্রুৱ std::set<size_t>& usedTriggers) {
        std::map<size_t, std::map<size_t, std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*>>> usedTrigsToUsingTrees;
        বাবে (auto sen_বৃক্ষ_Sched : m_sen_বৃক্ষ_To_Sched) {
            ধ্রুৱ Ast_Sen_বৃক্ষ* ধ্রুৱ senTreep = sen_বৃক্ষ_Sched.first;
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ shedp = sen_বৃক্ষ_Sched.second;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            বাবে (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ধ্রুৱ size_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                যদি (usedTriggers.find(idx) != usedTriggers.end()) {
                    usedTrigsToUsingTrees[idx / TriggerKit::WORD_SIZE]
                                         [size_t{1} << (idx % TriggerKit::WORD_SIZE)]
                                             .ঠেলো_back(shedp);
                }
            }
        }
        যদি (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            বাবে (ধ্রুৱ auto& triggersToTrees : usedTrigsToUsingTrees) {
                বাবে (ধ্রুৱ auto& bitsToTrees : triggersToTrees.second) {
                    ধ্রুৱ std::set<ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা*> exprps{bitsToTrees.second.begin(),
                                                              bitsToTrees.second.end()};
                    UASSERT(bitsToTrees.second.size() == exprps.size(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        ঘূৰাই usedTrigsToUsingTrees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_বৃক্ষ* ধ্রুৱ senTreep) {
        File_সাৰি* ধ্রুৱ flp = senTreep->fileline();
        যদি (!senTreep->user1p()) {
            AstCFunc* ধ্রুৱ funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), মিছা);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* ধ্রুৱ tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(সঁচা);

            AstVar* ধ্রুৱ argp = নূতন AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->findBasicDType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(সঁচা);
            argp->direction(VDirection::প্ৰবেশ);
            funcp->addArgsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_back(funcp, senTreep);
        }
        AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtypeSetVoid();
        ঘূৰাই callp;
    }

    শূন্য visit(AstCAwait* ধ্রুৱ nodep) override {
        যদি (nodep->user1SetOnce()) ঘূৰাই;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        যদি (ধ্রুৱ AstCMethodHard* ধ্রুৱ cMethodHardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            যদি (cMethodHardp->method() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ধ্রুৱ beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_গ্ৰন্থি* ধ্রুৱ pinp = cMethodHardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, cMethodHardp, "No event description");
                beforeTrigp->addArgsp(VN_AS(pinp, গ্ৰন্থি_ৰাশিমালা)->clone_বৃক্ষ(মিছা));

                // Call the before-trigger function before the CAwait
                nodep->addHereThisAsNext(beforeTrigp->makeStmt());
                m_sen_বৃক্ষ_To_Sched.emplace_back(nodep->sentreep(), cMethodHardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    শূন্য visit(Ast_গ্ৰন্থি* ধ্রুৱ nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_ৰাশিমালা_Builder& sen_ৰাশিমালা_Builder,
                           ধ্রুৱ TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_ৰাশিমালা_Builder{sen_ৰাশিমালা_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_ৰাশিমালা_To_Sen_Item();

        std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*> trigps;
        std::set<size_t> usedTriggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        বাবে (ধ্রুৱ auto& funcToUsedTriggers : m_generatedFuncs) {
            AstCFunc* ধ্রুৱ funcp = funcToUsedTriggers.first;
            AstVarScope* ধ্রুৱ vscp = VN_AS(funcp->user1p(), VarScope);
            File_সাৰি* ধ্রুৱ flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_বৃক্ষ* ধ্রুৱ senTreep = funcToUsedTriggers.second;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ধ্রুৱ auto emplaceAt
                    = [flp, &trigps, &usedTriggers](Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp, ধ্রুৱ size_t pos) {
                          ধ্রুৱ size_t targetSize
                              = vlstd::roundUpToMultipleOf<TriggerKit::WORD_SIZE>(pos + 1);
                          যদি (trigps.capacity() < targetSize) trigps.reserve(targetSize * 2);
                          যেতিয়ালৈ (trigps.size() < targetSize) {
                              trigps.ঠেলো_back(নূতন AstConst{flp, AstConst::BitFalse{}});
                          }
                          trigps[pos]->delete_বৃক্ষ();
                          trigps[pos] = exprp;
                          usedTriggers.ঢোকাও(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                বাবে (ধ্রুৱ AstSenItem* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), SenItem)) {
                    ধ্রুৱ size_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_ৰাশিমালা_Builder.build(itemp).first, idx);
                    auto iter = m_sen_ৰাশিমালা_To_Sen_Item.find(*get_Sen_Hash_গ্ৰন্থি(itemp));
                    যদি (iter != m_sen_ৰাশিমালা_To_Sen_Item.end()) {
                        বাবে (AstSenItem* ধ্রুৱ additionalItemp : iter->second) {
                            ধ্রুৱ size_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_ৰাশিমালা_Builder.build(additionalItemp).first, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ধ্রুৱ Sen_ৰাশিমালা_Builder::Results results
                    = m_sen_ৰাশিমালা_Builder.getResultsAndClearUpdates();
                বাবে (Ast_গ্ৰন্থি_Stmt* ধ্রুৱ stmtsp : results.m_inits) funcp->addStmtsp(stmtsp);
                বাবে (Ast_গ্ৰন্থি_Stmt* ধ্রুৱ stmtsp : results.m_preUpdates) funcp->addStmtsp(stmtsp);
                funcp->addStmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                বাবে (Ast_গ্ৰন্থি_Stmt* ধ্রুৱ stmtsp : results.m_postUpdates) funcp->addStmtsp(stmtsp);
            }

            ধ্রুৱ std::map<size_t, std::map<size_t, std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*>>>
                usedTrigsToUsingTrees = getUsedTriggersToTrees(usedTriggers);
            usedTriggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            ধ্রুৱ auto getIdx = [flp](AstVarScope* ধ্রুৱ scocep, VAccess access, size_t idx) {
                ঘূৰাই নূতন AstArraySel{flp, নূতন AstVarRef{flp, scocep, access},
                                       নূতন AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* ধ্রুৱ argpVscp = নূতন AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->addVarsp(argpVscp);

            // Mark as ready triggered schedulers
            বাবে (ধ্রুৱ auto& triggersToTrees : usedTrigsToUsingTrees) {
                ধ্রুৱ size_t word = triggersToTrees.first;

                বাবে (ধ্রুৱ auto& bitsToTrees : triggersToTrees.second) {
                    ধ্রুৱ size_t bit = bitsToTrees.first;
                    ধ্রুৱ auto& schedulers = bitsToTrees.second;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* ধ্রুৱ maskConstp = নূতন AstConst{flp, AstConst::Unsized64{}, bit};
                    AstAnd* ধ্রুৱ condp
                        = নূতন AstAnd{flp, getIdx(vscp, VAccess::READ, word), maskConstp};
                    AstIf* ধ্রুৱ ifp = নূতন AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    বাবে (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ schedp : schedulers) {
                        AstCMethodHard* ধ্রুৱ callp = নূতন AstCMethodHard{
                            flp, schedp->clone_বৃক্ষ(মিছা), VCMethod::SCHED_READY};
                        callp->dtypeSetVoid();
                        callp->addPinsp(নূতন AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->addThensp(callp->makeStmt());
                    }
                    funcp->addStmtsp(ifp);
                }
            }

            AstVarScope* ধ্রুৱ vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            বাবে (ধ্রুৱ auto& triggersToTrees : usedTrigsToUsingTrees) {
                ধ্রুৱ size_t word = triggersToTrees.first;
                funcp->addStmtsp(নূতন AstAssign{flp, getIdx(vscAccp, VAccess::WRITE, word),
                                               নূতন AstOr{flp, getIdx(vscAccp, VAccess::READ, word),
                                                         getIdx(vscp, VAccess::READ, word)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = স্ৱভাৱ;
};

শূন্য beforeTrigVisitor(AstNetlist* netlistp, Sen_ৰাশিমালা_Builder& sen_ৰাশিমালা_Builder,
                       ধ্রুৱ TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_ৰাশিমালা_Builder, trigKit};
}

}  // namespace V3Sched
