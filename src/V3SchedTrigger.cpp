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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"
#ଅନ୍ତର୍ଭାଵ "V3Order.h"
#ଅନ୍ତର୍ଭାଵ "V3Sched.h"
#ଅନ୍ତର୍ଭାଵ "V3SenExprBuilder.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ V3Sched {

ନାମାକାଶ {

using util::newArgument;

AstVarScope* newLocal(AstCFunc* funcp, Ast_ଗଣ୍ଠି_DType* dtypep, ଅଚଳ std::string& name) {
    File_ଧାଡ଼ି* ଅଚଳ flp = funcp->fileline();
    AstScope* ଅଚଳ scopep = funcp->scopep();
    AstVar* ଅଚଳ varp = ନୂତନ AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(ସତ୍ୟ);
    varp->noReset(ସତ୍ୟ);
    funcp->addVarsp(varp);
    AstVarScope* ଅଚଳ vscp = ନୂତନ AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    ଫେରନ୍ତୁ vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() ଅଚଳ {
    UASSERT(m_n_Pre_ଶବ୍ଦଗୁଡ଼ିକ, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ଅଚଳ netlistp = v3Global.rootp();
    File_ଧାଡ଼ି* ଅଚଳ flp = netlistp->ଶୀର୍ଷ_Scopep()->fileline();
    Ast_ଗଣ୍ଠି_DType* ଅଚଳ u32DTypep = netlistp->findUInt32DType();
    Ast_ଗଣ୍ଠି_DType* ଅଚଳ strDtypep = netlistp->findStringDType();

    // Dumping function always slow
    ଅଚଳ std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* ଅଚଳ funcp = util::makeSubFunction(netlistp, name, ସତ୍ୟ);
    funcp->isStatic(ସତ୍ୟ);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* ଅଚଳ eVscp = newArgument(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* ଅଚଳ tVscp = newArgument(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    ଅଚଳ auto rd = [flp](AstVarScope* vp) { ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, vp, VAccess::READ}; };
    ଅଚଳ auto wr = [flp](AstVarScope* vp) { ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* ଅଚଳ vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* ଅଚଳ iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->addStmtsp(util::setVar(iVscp, 0));
        // Add loop
        AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
        funcp->addStmtsp(loopp);
        // Loop body
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = ନୂତନ AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = ନୂତନ AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ limp = ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ};
        loopp->addStmtsp(ନୂତନ AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(iVscp));
        loopp->addStmtsp(ନୂତନ AstLoopTest{flp, loopp, ନୂତନ AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(vVscp));
        callp->addArgsp(rd(tVscp));
        funcp->addStmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* ଅଚଳ pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* ଅଚଳ jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->addStmtsp(util::setVar(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = ନୂତନ AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = ନୂତନ AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ limp = ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, m_n_Pre_ଶବ୍ଦଗୁଡ଼ିକ};
            loopp->addStmtsp(ନୂତନ AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(ନୂତନ AstLoopTest{flp, loopp, ନୂତନ AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = ନୂତନ AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = ନୂତନ AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ limp = ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ};
            loopp->addStmtsp(ନୂତନ AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(ନୂତନ AstLoopTest{flp, loopp, ନୂତନ AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(pVscp));
        callp->addArgsp(
            ନୂତନ AstConcatN{flp, rd(tVscp), ନୂତନ AstConst{flp, AstConst::String{}, " pre"}});
        funcp->addStmtsp(callp->makeStmt());
    }

    ଫେରନ୍ତୁ funcp;
}

AstCFunc* TriggerKit::createAnySetFunc(AstUnpackArrayDType* ଅଚଳ dtypep) ଅଚଳ {
    AstNetlist* ଅଚଳ netlistp = v3Global.rootp();
    File_ଧାଡ଼ି* ଅଚଳ flp = netlistp->ଶୀର୍ଷ_Scopep()->fileline();
    Ast_ଗଣ୍ଠି_DType* ଅଚଳ u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* ଅଚଳ funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(ସତ୍ୟ);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* ଅଚଳ iVscp = newArgument(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ଅଚଳ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    ଅଚଳ auto rd = [flp](AstVarScope* vp) { ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);
    funcp->addStmtsp(ନୂତନ AstCReturn{flp, ନୂତନ AstConst{flp, AstConst::BitFalse{}}});

    // Loop body
    ଅଚଳ uint32_t n_ଶବ୍ଦଗୁଡ଼ିକ = dtypep->elementsConst();
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = ନୂତନ AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ thenp = ନୂତନ AstCReturn{flp, ନୂତନ AstConst{flp, AstConst::BitTrue{}}};
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ limp = ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, n_ଶବ୍ଦଗୁଡ଼ିକ};
    loopp->addStmtsp(ନୂତନ AstIf{flp, condp, thenp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(ନୂତନ AstLoopTest{flp, loopp, ନୂତନ AstLt{flp, rd(nVscp), limp}});

    // Done
    ଫେରନ୍ତୁ funcp;
}
AstCFunc* TriggerKit::createClearFunc() ଅଚଳ {
    AstNetlist* ଅଚଳ netlistp = v3Global.rootp();
    File_ଧାଡ଼ି* ଅଚଳ flp = netlistp->ଶୀର୍ଷ_Scopep()->fileline();
    Ast_ଗଣ୍ଠି_DType* ଅଚଳ u32DTypep = netlistp->findUInt32DType();

    // Create function
    AstCFunc* ଅଚଳ funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(ସତ୍ୟ);

    // Add arguments
    AstVarScope* ଅଚଳ oVscp = newArgument(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* ଅଚଳ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ଅଚଳ auto rd = [flp](AstVarScope* vp) { ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, vp, VAccess::READ}; };
    ଅଚଳ auto wr = [flp](AstVarScope* vp) { ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = ନୂତନ AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = ନୂତନ AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ limp = ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ};
    loopp->addStmtsp(ନୂତନ AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(ନୂତନ AstLoopTest{flp, loopp, ନୂତନ AstLt{flp, rd(nVscp), limp}});

    // Done
    ଫେରନ୍ତୁ funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ଅଚଳ oDtypep,
                                       AstUnpackArrayDType* ଅଚଳ iDtypep) ଅଚଳ {
    AstNetlist* ଅଚଳ netlistp = v3Global.rootp();
    File_ଧାଡ଼ି* ଅଚଳ flp = netlistp->ଶୀର୍ଷ_Scopep()->fileline();
    Ast_ଗଣ୍ଠି_DType* ଅଚଳ u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* ଅଚଳ funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(ସତ୍ୟ);

    // Add arguments
    AstVarScope* ଅଚଳ oVscp = newArgument(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* ଅଚଳ iVscp = newArgument(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ଅଚଳ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ଅଚଳ auto rd = [flp](AstVarScope* vp) { ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, vp, VAccess::READ}; };
    ଅଚଳ auto wr = [flp](AstVarScope* vp) { ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = ନୂତନ AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ oWordp = ନୂତନ AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ iWordp = ନୂତନ AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = ନୂତନ AstOr{flp, oWordp, iWordp};
    AstConst* ଅଚଳ outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ଅଚଳ ନିବେଶ_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ limp = outputRangeLeftp->ଅଙ୍କ().toSInt() < ନିବେଶ_Range_Leftp->ଅଙ୍କ().toSInt()
                                  ? outputRangeLeftp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)
                                  : ନିବେଶ_Range_Leftp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
    loopp->addStmtsp(ନୂତନ AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(ନୂତନ AstLoopTest{flp, loopp, ନୂତନ AstLte{flp, rd(nVscp), limp}});

    // Done
    ଫେରନ୍ତୁ funcp;
}

Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* TriggerKit::newAnySetCall(AstVarScope* ଅଚଳ vscp) ଅଚଳ {
    File_ଧାଡ଼ି* ଅଚଳ flp = v3Global.rootp()->ଶୀର୍ଷ_Scopep()->fileline();
    ଯଦି (!m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ) ଫେରନ୍ତୁ ନୂତନ AstConst{flp, AstConst::BitFalse{}};

    AstCFunc* funcp = nullptr;
    ଯଦି (vscp->dtypep() == m_trigVecDTypep) {
        ଯଦି (!m_anySetVecp) m_anySetVecp = createAnySetFunc(m_trigVecDTypep);
        funcp = m_anySetVecp;
    } ଅନ୍ୟଥା ଯଦି (vscp->dtypep() == m_trigExtDTypep) {
        ଯଦି (!m_anySetExtp) m_anySetExtp = createAnySetFunc(m_trigExtDTypep);
        funcp = m_anySetExtp;
    } ଅନ୍ୟଥା {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, funcp};
    callp->addArgsp(ନୂତନ AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetBit();
    ଫେରନ୍ତୁ callp;
}
Ast_ଗଣ୍ଠି_Stmt* TriggerKit::newClearCall(AstVarScope* ଅଚଳ vscp) ଅଚଳ {
    ଯଦି (!m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ) ଫେରନ୍ତୁ nullptr;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    ଯଦି (!m_clearp) m_clearp = createClearFunc();
    File_ଧାଡ଼ି* ଅଚଳ flp = v3Global.rootp()->ଶୀର୍ଷ_Scopep()->fileline();
    AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, m_clearp};
    callp->addArgsp(ନୂତନ AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetVoid();
    ଫେରନ୍ତୁ callp->makeStmt();
}
Ast_ଗଣ୍ଠି_Stmt* TriggerKit::newOrIntoCall(AstVarScope* ଅଚଳ oVscp, AstVarScope* ଅଚଳ iVscp) ଅଚଳ {
    ଯଦି (!m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ) ଫେରନ୍ତୁ nullptr;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    ଅଚଳ size_t mask
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[mask];
    ଯଦି (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_ଧାଡ଼ି* ଅଚଳ flp = v3Global.rootp()->ଶୀର୍ଷ_Scopep()->fileline();
    AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, funcp};
    callp->addArgsp(ନୂତନ AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->addArgsp(ନୂତନ AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtypeSetVoid();
    ଫେରନ୍ତୁ callp->makeStmt();
}

Ast_ଗଣ୍ଠି_Stmt* TriggerKit::newCompBaseCall() ଅଚଳ {
    ଯଦି (!m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ) ଫେରନ୍ତୁ nullptr;
    File_ଧାଡ଼ି* ଅଚଳ flp = v3Global.rootp()->ଶୀର୍ଷ_Scopep()->fileline();
    AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, m_compVecp};
    callp->dtypeSetVoid();
    ଫେରନ୍ତୁ callp->makeStmt();
}

Ast_ଗଣ୍ଠି_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ଅଚଳ {
    ଯଦି (!m_n_Pre_ଶବ୍ଦଗୁଡ଼ିକ) ଫେରନ୍ତୁ nullptr;
    File_ଧାଡ଼ି* ଅଚଳ flp = v3Global.rootp()->ଶୀର୍ଷ_Scopep()->fileline();
    AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, m_compExtp};
    callp->addArgsp(ନୂତନ AstVarRef{flp, vscp, VAccess::READ});
    callp->dtypeSetVoid();
    ଫେରନ୍ତୁ callp->makeStmt();
}

Ast_ଗଣ୍ଠି_Stmt* TriggerKit::newDumpCall(AstVarScope* ଅଚଳ vscp, ଅଚଳ std::string& tag,
                                     ତାର୍କିକ debugOnly) ଅଚଳ {
    ଯଦି (!m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ) ଫେରନ୍ତୁ nullptr;
    AstCFunc* funcp = nullptr;
    ଯଦି (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } ଅନ୍ୟଥା ଯଦି (vscp->dtypep() == m_trigExtDTypep) {
        ଯଦି (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } ଅନ୍ୟଥା {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_ଧାଡ଼ି* ଅଚଳ flp = v3Global.rootp()->ଶୀର୍ଷ_Scopep()->fileline();
    AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, funcp};
    callp->addArgsp(ନୂତନ AstVarRef{flp, vscp, VAccess::READ});
    callp->addArgsp(ନୂତନ AstConst{flp, AstConst::String{}, tag});
    callp->dtypeSetVoid();
    AstCStmt* ଅଚଳ cstmtp = ନୂତନ AstCStmt{flp};
    cstmtp->add("#ifdef VL_DEBUG\n");
    ଯଦି (debugOnly) {
        cstmtp->add("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->add(callp->makeStmt());
        cstmtp->add("}\n");
    } ଅନ୍ୟଥା {
        cstmtp->add(callp->makeStmt());
    }
    cstmtp->add("#endif");
    ଫେରନ୍ତୁ cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(ଅଚଳ std::string& name) ଅଚଳ {
    ଯଦି (!m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ) ଫେରନ୍ତୁ nullptr;
    ଫେରନ୍ତୁ v3Global.rootp()->ଶୀର୍ଷ_Scopep()->createTemp("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extraTriggerIndex(uint32_t index) ଅଚଳ {
    UASSERT(index <= m_n_Extra_ଶବ୍ଦଗୁଡ଼ିକ * WORD_SIZE, "Invalid external trigger index");
    ଫେରନ୍ତୁ index + m_n_Sense_ଶବ୍ଦଗୁଡ଼ିକ * WORD_SIZE;
}

std::set<uint32_t> TriggerKit::triggerIndices(ଅଚଳ Ast_Sen_ବୃକ୍ଷ* senTreep, ତାର୍କିକ pre) ଅଚଳ {
    std::set<uint32_t> indices;
    ପାଇଁ (ଅଚଳ AstSenItem* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), SenItem)) {
        uint32_t index = m_senItem2TrigIdx.at(*sip);
        ଯଦି (pre) {
            UASSERT_OBJ(index < m_n_Pre_ଶବ୍ଦଗୁଡ଼ିକ * WORD_SIZE, sip, "Not a 'pre' trigger");
            index += m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ * WORD_SIZE;
        }
        indices.ଭର୍ତ୍ତି(index);
    }
    ଫେରନ୍ତୁ indices;
}

Ast_ଗଣ୍ଠି_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ଅଚଳ {
    index = extraTriggerIndex(index);
    ଅଚଳ uint32_t wordIndex = index / WORD_SIZE;
    ଅଚଳ uint32_t bitIndex = index % WORD_SIZE;
    File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
    AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ wordp = ନୂତନ AstArraySel{flp, refp, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(wordIndex)};
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ trigLhsp = ନୂତନ AstSel{flp, wordp, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(bitIndex), 1};
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ trigRhsp = ନୂତନ AstVarRef{flp, vscp, VAccess::READ};
    ଫେରନ୍ତୁ ନୂତନ AstAssign{flp, trigLhsp, trigRhsp};
}

ଶୂନ୍ୟ TriggerKit::addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ଅଚଳ {
    File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_ଗଣ୍ଠି* ଅଚଳ setp = newExtraTriggerAssignment(vscp, index);
    setp->addNext(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, vscp, VAccess::WRITE},
                                ନୂତନ AstConst{flp, AstConst::BitFalse{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
ଶୂନ୍ୟ TriggerKit::add_ମୂଲ୍ୟ_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t index) ଅଚଳ {
    index += m_n_Sense_ଶବ୍ଦଗୁଡ଼ିକ * WORD_SIZE;
    ଅଚଳ uint32_t wordIndex = index / WORD_SIZE;
    ଅଚଳ uint32_t bitIndex = index % WORD_SIZE;

    Ast_ଶୀର୍ଷ_Scope* ଅଚଳ ଶୀର୍ଷ_Scopep = netlistp->ଶୀର୍ଷ_Scopep();
    File_ଧାଡ଼ି* ଅଚଳ flp = netlistp->fileline();
    Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ bdtypep = VN_CAST(dtypep, BasicDType)) {
        ଯଦି (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            ଫେରନ୍ତୁ;
        }
    } ଅନ୍ୟଥା ଯଦି (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, ଗଣ୍ଠି_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        ଫେରନ୍ତୁ;
    }

    ଅଚଳ auto rdInst = [flp](AstVarScope* vp) { ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, vp, VAccess::READ}; };
    ଅଚଳ auto wrPrev = [flp](AstVarScope* vp) { ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, vp, VAccess::WRITE}; };
    ଅଚଳ auto rdPrev = [flp](AstVarScope* vp) { ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    ଅଚଳ std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* ଅଚଳ prevVscp = ଶୀର୍ଷ_Scopep->createTemp(prevName, instVscp->dtypep());

    // Initialize prev = inst
    ଯଦି (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ଅଚଳ cmhp = ନୂତନ AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        initFuncp->addStmtsp(cmhp->makeStmt());
    } ଅନ୍ୟଥା {
        initFuncp->addStmtsp(ନୂତନ AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* neqp;
    ଯଦି (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ଅଚଳ cmhp
            = ନୂତନ AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtypeSetBit();
        neqp = cmhp;
    } ଅନ୍ୟଥା {
        neqp = ନୂତନ AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_ଗଣ୍ଠି* updp;
    ଯଦି (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ଅଚଳ cmhp = ନୂତନ AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        updp = cmhp->makeStmt();
    } ଅନ୍ୟଥା {
        updp = ନୂତନ AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ wordp = ନୂତନ AstArraySel{flp, refp, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(wordIndex)};
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ trigLhsp = ନୂତନ AstSel{flp, wordp, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(bitIndex), 1};
    Ast_ଗଣ୍ଠି* ଅଚଳ setp = ନୂତନ AstAssign{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->addNext(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ଅଚଳ std::string& name, ତାର୍କିକ slow, uint32_t n_Sense_ଶବ୍ଦଗୁଡ଼ିକ,
                       uint32_t n_Extra_ଶବ୍ଦଗୁଡ଼ିକ, uint32_t n_Pre_ଶବ୍ଦଗୁଡ଼ିକ,
                       std::unordered_map<VNRef<ଅଚଳ AstSenItem>, size_t> senItem2TrigIdx,
                       ଅଚଳ std::vector<ଅଚଳ AstSenItem*>& senItemps, ତାର୍କିକ useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_ଶବ୍ଦଗୁଡ଼ିକ{n_Sense_ଶବ୍ଦଗୁଡ଼ିକ}
    , m_n_Extra_ଶବ୍ଦଗୁଡ଼ିକ{n_Extra_ଶବ୍ଦଗୁଡ଼ିକ}
    , m_n_Pre_ଶବ୍ଦଗୁଡ଼ିକ{n_Pre_ଶବ୍ଦଗୁଡ଼ିକ}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trigIdx2SenItem{senItemps} {
    // If no triggers, we don't need to generate anything
    ଯଦି (!m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ) ଫେରନ୍ତୁ;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trigIdx2SenItem.resize((m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ + m_n_Pre_ଶବ୍ଦଗୁଡ଼ିକ) * WORD_SIZE);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_ଶବ୍ଦଗୁଡ଼ିକ * WORD_SIZE,
              m_trigIdx2SenItem.begin() + m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ * WORD_SIZE);
    AstNetlist* ଅଚଳ netlistp = v3Global.rootp();
    AstScope* ଅଚଳ scopep = netlistp->ଶୀର୍ଷ_Scopep()->scopep();
    File_ଧାଡ଼ି* ଅଚଳ flp = scopep->fileline();
    // Data type of a single trigger word
    m_wordDTypep = netlistp->findBitDType(WORD_SIZE, WORD_SIZE, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ଅଚଳ rp = ନୂତନ AstRange{flp, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ - 1), 0};
    m_trigVecDTypep = ନୂତନ AstUnpackArrayDType{flp, m_wordDTypep, rp};
    netlistp->typeTablep()->addTypesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    ଯଦି (m_n_Pre_ଶବ୍ଦଗୁଡ଼ିକ) {
        AstRange* ଅଚଳ ep = ନୂତନ AstRange{flp, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ + m_n_Pre_ଶବ୍ଦଗୁଡ଼ିକ - 1), 0};
        m_trigExtDTypep = ନୂତନ AstUnpackArrayDType{flp, m_wordDTypep, ep};
        netlistp->typeTablep()->addTypesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } ଅନ୍ୟଥା {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->ଶୀର୍ଷ_Scopep()->createTemp("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->isInternal(ସତ୍ୟ);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, ସତ୍ୟ);
    m_dumpp->isStatic(ସତ୍ୟ);
    m_dumpp->ifdef("VL_DEBUG");
    ଯଦି (useAcc) {
        m_vscAccp
            = netlistp->ଶୀର୍ଷ_Scopep()->createTemp("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->isInternal(ସତ୍ୟ);
    }
}

AstAssign* TriggerKit::createSenTrigVecAssignment(AstVarScope* ଅଚଳ target,
                                                  std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>& trigps) {
    File_ଧାଡ଼ି* ଅଚଳ flp = target->fileline();
    AstAssign* trigStmtsp = nullptr;
    // Assign sense triggers vector one word at a time
    ପାଇଁ (size_t i = 0; i < trigps.size(); i += WORD_SIZE) {
        // Concatenate all bits in this trigger word using a balanced
        ପାଇଁ (uint32_t level = 0; level < WORD_SIZE_LOG2; ++level) {
            ଅଚଳ uint32_t stride = 1 << level;
            ପାଇଁ (uint32_t j = 0; j < WORD_SIZE; j += 2 * stride) {
                trigps[i + j] = ନୂତନ AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = nullptr;
            }
        }

        // Set the whole word in the trigger vector
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା wordIndex = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(i / WORD_SIZE);
        AstArraySel* ଅଚଳ aselp
            = ନୂତନ AstArraySel{flp, ନୂତନ AstVarRef{flp, target, VAccess::WRITE}, wordIndex};
        trigStmtsp = Ast_ଗଣ୍ଠି::addNext(trigStmtsp, ନୂତନ AstAssign{flp, aselp, trigps[i]});
    }
    ଫେରନ୍ତୁ trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ଅଚଳ initFuncp,  //
                              Sen_ପଦସମଷ୍ଟି_Builder& sen_ପଦସମଷ୍ଟି_Builder,  //
                              ଅଚଳ std::vector<ଅଚଳ Ast_Sen_ବୃକ୍ଷ*>& preTreeps,  //
                              ଅଚଳ std::vector<ଅଚଳ Ast_Sen_ବୃକ୍ଷ*>& senTreeps,  //
                              ଅଚଳ string& name,  //
                              ଅଚଳ ExtraTriggers& extraTriggers,  //
                              ତାର୍କିକ slow,  //
                              ତାର୍କିକ useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::vector<ଅଚଳ AstSenItem*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<ଅଚଳ AstSenItem>, size_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    ପାଇଁ (ଅଚଳ Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep : preTreeps) {
        ପାଇଁ (ଅଚଳ AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ଅଚଳ auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            ଯଦି (pair.second) senItemps.ଠେଲ_back(itemp);
        }
    }
    ଅଚଳ uint32_t n_Pre_Sen_ବସ୍ତୁଗୁଡ଼ିକ = senItemps.size();
    V3Stats::addStat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_ବସ୍ତୁଗୁଡ଼ିକ);
    // Number of pre triggers, rounded up to a full word.
    ଅଚଳ uint32_t nPreTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ଅଚଳ uint32_t n_Pre_ଶବ୍ଦଗୁଡ଼ିକ = nPreTriggers / WORD_SIZE;

    // Process the rest of the trees
    ପାଇଁ (ଅଚଳ Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep : senTreeps) {
        ପାଇଁ (ଅଚଳ AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ଅଚଳ auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            ଯଦି (pair.second) senItemps.ଠେଲ_back(itemp);
        }
    }
    ଅଚଳ uint32_t n_Sen_ବସ୍ତୁଗୁଡ଼ିକ = senItemps.size() - nPreTriggers;
    V3Stats::addStat("Scheduling, '" + name + "' sense triggers", n_Sen_ବସ୍ତୁଗୁଡ଼ିକ + n_Pre_Sen_ବସ୍ତୁଗୁଡ଼ିକ);
    // Number of sense triggers, rounded up to a full word
    ଅଚଳ uint32_t nSenseTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ଅଚଳ uint32_t n_Sense_ଶବ୍ଦଗୁଡ଼ିକ = nSenseTriggers / WORD_SIZE;

    // Allocate space for the extra triggers
    V3Stats::addStat("Scheduling, '" + name + "' extra triggers", extraTriggers.size());
    // Number of extra triggers, rounded up to a full word.
    ଅଚଳ uint32_t nExtraTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(extraTriggers.size());
    ଅଚଳ uint32_t n_Extra_ଶବ୍ଦଗୁଡ଼ିକ = nExtraTriggers / WORD_SIZE;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_ଶବ୍ଦଗୁଡ଼ିକ, n_Extra_ଶବ୍ଦଗୁଡ଼ିକ, n_Pre_ଶବ୍ଦଗୁଡ଼ିକ, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    ଯଦି (!kit.m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ) ଫେରନ୍ତୁ kit;

    File_ଧାଡ଼ି* ଅଚଳ flp = netlistp->ଶୀର୍ଷ_Scopep()->fileline();

    // Creates read/write reference
    ଅଚଳ auto rd = [flp](AstVarScope* vp) { ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, vp, VAccess::READ}; };
    ଅଚଳ auto wr = [flp](AstVarScope* vp) { ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* ଅଚଳ dumpTrgp
        = newArgument(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* ଅଚଳ dumpTagp
        = newArgument(kit.m_dumpp, netlistp->findStringDType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, ନୂତନ AstLogNot{flp, kit.newAnySetCall(dumpTrgp)}};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* ଅଚଳ cstmtp = ନୂତନ AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    ଅଚଳ auto addDebug = [&](uint32_t index, ଅଚଳ string& text) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା wrdIndex = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(index / WORD_SIZE);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା bitIndex = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(index % WORD_SIZE);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ aselp = ନୂତନ AstArraySel{flp, rd(dumpTrgp), wrdIndex};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = ନୂତନ AstSel{flp, aselp, bitIndex, 1};
        AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, condp};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* ଅଚଳ cstmtp = ନୂତନ AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"' region trigger index " + std::to_string(index) + " is active: " + text
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_ଗଣ୍ଠି_Stmt* initialTrigsp = nullptr;
    ପାଇଁ (size_t i = 0; i < senItemps.size(); ++i) {
        ଅଚଳ AstSenItem* ଅଚଳ senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        ଯଦି (!senItemp) {
            trigps.emplace_back(ନୂତନ AstConst{flp, AstConst::BitFalse{}});
            ଜାରି;
        }

        // Create the trigger computation expression
        ଅଚଳ auto& pair = sen_ପଦସମଷ୍ଟି_Builder.build(senItemp);
        trigps.emplace_back(pair.first);

        // Add initialization time trigger
        ଯଦି (pair.second || v3Global.opt.xInitialEdge()) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା wrdIndex = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(i / WORD_SIZE);
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା bitIndex = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(i % WORD_SIZE);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ wordp = ନୂତନ AstArraySel{flp, wr(kit.m_vscp), wrdIndex};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = ନୂତନ AstSel{flp, wordp, bitIndex, 1};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = ନୂତନ AstConst{flp, AstConst::BitTrue{}};
            ଯଦି (useAcc) {
                initFuncp->addStmtsp(ନୂତନ AstAssign{flp, lhsp, rhsp});
            } ଅନ୍ୟଥା {
                initialTrigsp = Ast_ଗଣ୍ଠି::addNext(initialTrigsp, ନୂତନ AstAssign{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_ବୃକ୍ଷ(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        addDebug(i, desc);
    }
    UASSERT(trigps.size() == nSenseTriggers, "Inconsistent number of trigger expressions");

    AstAssign* ଅଚଳ trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    ପାଇଁ (unsigned i = 0; i < extraTriggers.size(); ++i) {
        addDebug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    ଅଚଳ Sen_ପଦସମଷ୍ଟି_Builder::Results senResults = sen_ପଦସମଷ୍ଟି_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ nodep : senResults.m_inits) initFuncp->addStmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_ଶୀର୍ଷ_Scope* ଅଚଳ ଶୀର୍ଷ_Scopep = netlistp->ଶୀର୍ଷ_Scopep();
    {
        AstCFunc* ଅଚଳ fp = kit.m_compVecp;
        // Trigger computation
        ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ nodep : senResults.m_preUpdates) fp->addStmtsp(nodep);
        fp->addStmtsp(trigStmtsp);
        ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ nodep : senResults.m_postUpdates) fp->addStmtsp(nodep);
        // Add the initialization time triggers
        ଯଦି (initialTrigsp) {
            AstVarScope* ଅଚଳ initVscp = ଶୀର୍ଷ_Scopep->createTemp("__V" + name + "DidInit", 1);
            AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, ନୂତନ AstNot{flp, rd(initVscp)}};
            fp->addStmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->addThensp(util::setVar(initVscp, 1));
            ifp->addThensp(initialTrigsp);
        }
        util::splitCheck(fp);
    };
    // If there are 'pre' triggers, compute them
    ଯଦି (kit.m_n_Pre_ଶବ୍ଦଗୁଡ଼ିକ) {
        AstCFunc* ଅଚଳ fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ଅଚଳ latchedp
            = newArgument(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ଅଚଳ nVscp = ଶୀର୍ଷ_Scopep->createTemp("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(ସତ୍ୟ);
        // Add a loop to compute the pre words
        AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
        fp->addStmtsp(util::setVar(nVscp, 0));
        fp->addStmtsp(loopp);
        // Loop body
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ offsetp = ନୂତନ AstConst{flp, kit.m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lIdxp = ନୂତନ AstAdd{flp, rd(nVscp), offsetp};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = ନୂତନ AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ aWordp = ନୂତନ AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bWordp = ନୂତନ AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = ନୂତନ AstAnd{flp, aWordp, ନୂତନ AstNot{flp, bWordp}};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ limp = ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, n_Pre_ଶବ୍ଦଗୁଡ଼ିକ};
        loopp->addStmtsp(ନୂତନ AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(nVscp));
        loopp->addStmtsp(ନୂତନ AstLoopTest{flp, loopp, ନୂତନ AstLt{flp, rd(nVscp), limp}});
        util::splitCheck(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    ଯଦି (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlinkFrBackWithNext()->delete_ବୃକ୍ଷ();
    // Done with the trigger dump function, split as might be large
    util::splitCheck(kit.m_dumpp);

    ଫେରନ୍ତୁ kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
ଶ୍ରେଣୀ AwaitBeforeTrigVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    ଅଚଳ VNUser1InUse m_user1InUse;

    /**
     * AstCAwait::user1()       ->  bool.           True if node has been visited
     * AstSenTree::user1p()     ->  AstCFunc*.      Function that has to be called before awaiting
     *                                              for CAwait pointing to this SenTree
     * AstCFunc::user1p()       ->  AstVarScope*    Function's local temporary extended trigger
     *                                              vector variable scope
     */

    // Netlist - needed for using util::makeSubFunction()
    AstNetlist* ଅଚଳ m_netlistp;
    // Trigger kit - for accessing trigger vectors and mapping senItems to thier indexes
    ଅଚଳ TriggerKit& m_trigKit;
    // Expression builder - for building expressions from SenItems
    Sen_ପଦସମଷ୍ଟି_Builder& m_sen_ପଦସମଷ୍ଟି_Builder;
    // Generator of unique names for before-trigger function
    V3UniqueNames m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::vector<std::pair<AstCFunc*, Ast_Sen_ବୃକ୍ଷ*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::vector<std::pair<Ast_Sen_ବୃକ୍ଷ*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>> m_sen_ବୃକ୍ଷ_To_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_ଗଣ୍ଠି>, std::vector<AstSenItem*>> m_sen_ପଦସମଷ୍ଟି_To_Sen_Item;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    ସ୍ଥିର Ast_ଗଣ୍ଠି* get_Sen_Hash_ଗଣ୍ଠି(ଅଚଳ AstSenItem* ଅଚଳ nodep) {
        ଯଦି (AstVarRef* ଅଚଳ varRefp = VN_CAST(nodep->sensp(), VarRef)) ଫେରନ୍ତୁ varRefp;
        ଫେରନ୍ତୁ nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    ଶୂନ୍ୟ fill_Sen_ପଦସମଷ୍ଟି_To_Sen_Item() {
        ପାଇଁ (auto sen_ବୃକ୍ଷ_Sched : m_sen_ବୃକ୍ଷ_To_Sched) {
            Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep = sen_ବୃକ୍ଷ_Sched.first;

            ପାଇଁ (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ଅଚଳ VEdgeType edge = senItemp->edgeType();
                ଯଦି (edge.anEdge() || edge == VEdgeType::ET_CHANGED
                    || edge == VEdgeType::ET_HYBRID) {
                    m_sen_ପଦସମଷ୍ଟି_To_Sen_Item[*get_Sen_Hash_ଗଣ୍ଠି(senItemp)].ଠେଲ_back(senItemp);
                }
            }
        }

        std::vector<VNRef<Ast_ଗଣ୍ଠି>> toRemove;
        ପାଇଁ (ଅଚଳ auto& sen_ପଦସମଷ୍ଟି_To_Sen_ବୃକ୍ଷ : m_sen_ପଦସମଷ୍ଟି_To_Sen_Item) {
            std::vector<AstSenItem*> senItemps = sen_ପଦସମଷ୍ଟି_To_Sen_ବୃକ୍ଷ.second;
            toRemove.ଠେଲ_back(sen_ପଦସମଷ୍ଟି_To_Sen_ବୃକ୍ଷ.first);
            ପାଇଁ (size_t i = 1; i < senItemps.size(); ++i) {
                ଯଦି (senItemps[i]->edgeType() != senItemps[i - 1]->edgeType()) {
                    toRemove.କାଢ଼_back();
                    ଭାଙ୍ଗ;
                }
            }
        }
        ପାଇଁ (ଅଚଳ VNRef<Ast_ଗଣ୍ଠି> it : toRemove) m_sen_ପଦସମଷ୍ଟି_To_Sen_Item.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<size_t, std::map<size_t, std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>>>
    getUsedTriggersToTrees(ଅଚଳ std::set<size_t>& usedTriggers) {
        std::map<size_t, std::map<size_t, std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>>> usedTrigsToUsingTrees;
        ପାଇଁ (auto sen_ବୃକ୍ଷ_Sched : m_sen_ବୃକ୍ଷ_To_Sched) {
            ଅଚଳ Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep = sen_ବୃକ୍ଷ_Sched.first;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ shedp = sen_ବୃକ୍ଷ_Sched.second;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            ପାଇଁ (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ଅଚଳ size_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                ଯଦି (usedTriggers.find(idx) != usedTriggers.end()) {
                    usedTrigsToUsingTrees[idx / TriggerKit::WORD_SIZE]
                                         [size_t{1} << (idx % TriggerKit::WORD_SIZE)]
                                             .ଠେଲ_back(shedp);
                }
            }
        }
        ଯଦି (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            ପାଇଁ (ଅଚଳ auto& triggersToTrees : usedTrigsToUsingTrees) {
                ପାଇଁ (ଅଚଳ auto& bitsToTrees : triggersToTrees.second) {
                    ଅଚଳ std::set<ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> exprps{bitsToTrees.second.begin(),
                                                              bitsToTrees.second.end()};
                    UASSERT(bitsToTrees.second.size() == exprps.size(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        ଫେରନ୍ତୁ usedTrigsToUsingTrees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep) {
        File_ଧାଡ଼ି* ଅଚଳ flp = senTreep->fileline();
        ଯଦି (!senTreep->user1p()) {
            AstCFunc* ଅଚଳ funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), ମିଥ୍ୟା);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* ଅଚଳ tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(ସତ୍ୟ);

            AstVar* ଅଚଳ argp = ନୂତନ AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->findBasicDType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(ସତ୍ୟ);
            argp->direction(VDirection::ନିବେଶ);
            funcp->addArgsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_back(funcp, senTreep);
        }
        AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtypeSetVoid();
        ଫେରନ୍ତୁ callp;
    }

    ଶୂନ୍ୟ visit(AstCAwait* ଅଚଳ nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        ଯଦି (ଅଚଳ AstCMethodHard* ଅଚଳ cMethodHardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            ଯଦି (cMethodHardp->method() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ଅଚଳ beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_ଗଣ୍ଠି* ଅଚଳ pinp = cMethodHardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, cMethodHardp, "No event description");
                beforeTrigp->addArgsp(VN_AS(pinp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି)->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));

                // Call the before-trigger function before the CAwait
                nodep->addHereThisAsNext(beforeTrigp->makeStmt());
                m_sen_ବୃକ୍ଷ_To_Sched.emplace_back(nodep->sentreep(), cMethodHardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* ଅଚଳ nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_ପଦସମଷ୍ଟି_Builder& sen_ପଦସମଷ୍ଟି_Builder,
                           ଅଚଳ TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_ପଦସମଷ୍ଟି_Builder{sen_ପଦସମଷ୍ଟି_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_ପଦସମଷ୍ଟି_To_Sen_Item();

        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> trigps;
        std::set<size_t> usedTriggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        ପାଇଁ (ଅଚଳ auto& funcToUsedTriggers : m_generatedFuncs) {
            AstCFunc* ଅଚଳ funcp = funcToUsedTriggers.first;
            AstVarScope* ଅଚଳ vscp = VN_AS(funcp->user1p(), VarScope);
            File_ଧାଡ଼ି* ଅଚଳ flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep = funcToUsedTriggers.second;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ଅଚଳ auto emplaceAt
                    = [flp, &trigps, &usedTriggers](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp, ଅଚଳ size_t pos) {
                          ଅଚଳ size_t targetSize
                              = vlstd::roundUpToMultipleOf<TriggerKit::WORD_SIZE>(pos + 1);
                          ଯଦି (trigps.capacity() < targetSize) trigps.reserve(targetSize * 2);
                          ଯେତେବେଳେ (trigps.size() < targetSize) {
                              trigps.ଠେଲ_back(ନୂତନ AstConst{flp, AstConst::BitFalse{}});
                          }
                          trigps[pos]->delete_ବୃକ୍ଷ();
                          trigps[pos] = exprp;
                          usedTriggers.ଭର୍ତ୍ତି(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                ପାଇଁ (ଅଚଳ AstSenItem* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), SenItem)) {
                    ଅଚଳ size_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_ପଦସମଷ୍ଟି_Builder.build(itemp).first, idx);
                    auto iter = m_sen_ପଦସମଷ୍ଟି_To_Sen_Item.find(*get_Sen_Hash_ଗଣ୍ଠି(itemp));
                    ଯଦି (iter != m_sen_ପଦସମଷ୍ଟି_To_Sen_Item.end()) {
                        ପାଇଁ (AstSenItem* ଅଚଳ additionalItemp : iter->second) {
                            ଅଚଳ size_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_ପଦସମଷ୍ଟି_Builder.build(additionalItemp).first, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ଅଚଳ Sen_ପଦସମଷ୍ଟି_Builder::Results results
                    = m_sen_ପଦସମଷ୍ଟି_Builder.getResultsAndClearUpdates();
                ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtsp : results.m_inits) funcp->addStmtsp(stmtsp);
                ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtsp : results.m_preUpdates) funcp->addStmtsp(stmtsp);
                funcp->addStmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtsp : results.m_postUpdates) funcp->addStmtsp(stmtsp);
            }

            ଅଚଳ std::map<size_t, std::map<size_t, std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>>>
                usedTrigsToUsingTrees = getUsedTriggersToTrees(usedTriggers);
            usedTriggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            ଅଚଳ auto getIdx = [flp](AstVarScope* ଅଚଳ scocep, VAccess access, size_t idx) {
                ଫେରନ୍ତୁ ନୂତନ AstArraySel{flp, ନୂତନ AstVarRef{flp, scocep, access},
                                       ନୂତନ AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* ଅଚଳ argpVscp = ନୂତନ AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->addVarsp(argpVscp);

            // Mark as ready triggered schedulers
            ପାଇଁ (ଅଚଳ auto& triggersToTrees : usedTrigsToUsingTrees) {
                ଅଚଳ size_t word = triggersToTrees.first;

                ପାଇଁ (ଅଚଳ auto& bitsToTrees : triggersToTrees.second) {
                    ଅଚଳ size_t bit = bitsToTrees.first;
                    ଅଚଳ auto& schedulers = bitsToTrees.second;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* ଅଚଳ maskConstp = ନୂତନ AstConst{flp, AstConst::Unsized64{}, bit};
                    AstAnd* ଅଚଳ condp
                        = ନୂତନ AstAnd{flp, getIdx(vscp, VAccess::READ, word), maskConstp};
                    AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ schedp : schedulers) {
                        AstCMethodHard* ଅଚଳ callp = ନୂତନ AstCMethodHard{
                            flp, schedp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), VCMethod::SCHED_READY};
                        callp->dtypeSetVoid();
                        callp->addPinsp(ନୂତନ AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->addThensp(callp->makeStmt());
                    }
                    funcp->addStmtsp(ifp);
                }
            }

            AstVarScope* ଅଚଳ vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            ପାଇଁ (ଅଚଳ auto& triggersToTrees : usedTrigsToUsingTrees) {
                ଅଚଳ size_t word = triggersToTrees.first;
                funcp->addStmtsp(ନୂତନ AstAssign{flp, getIdx(vscAccp, VAccess::WRITE, word),
                                               ନୂତନ AstOr{flp, getIdx(vscAccp, VAccess::READ, word),
                                                         getIdx(vscp, VAccess::READ, word)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

ଶୂନ୍ୟ beforeTrigVisitor(AstNetlist* netlistp, Sen_ପଦସମଷ୍ଟି_Builder& sen_ପଦସମଷ୍ଟି_Builder,
                       ଅଚଳ TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_ପଦସମଷ୍ଟି_Builder, trigKit};
}

}  // namespace V3Sched
