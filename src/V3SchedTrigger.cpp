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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3Order.h"
#ಅನ್ತರ್ಭಾವ "V3Sched.h"
#ಅನ್ತರ್ಭಾವ "V3SenExprBuilder.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ V3Sched {

ನಾಮಾಕಾಶ {

using util::newArgument;

AstVarScope* newLocal(AstCFunc* funcp, Ast_ಗಂಟು_DType* dtypep, ಧ್ರುವ std::string& name) {
    File_ಸಾಲು* ಧ್ರುವ flp = funcp->fileline();
    AstScope* ಧ್ರುವ scopep = funcp->scopep();
    AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(ಸತ್ಯ);
    varp->noReset(ಸತ್ಯ);
    funcp->addVarsp(varp);
    AstVarScope* ಧ್ರುವ vscp = ನೂತನ AstVarScope{flp, scopep, varp};
    scopep->addVarsp(vscp);
    ಪ್ರತ್ಯಾಗಮ vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() ಧ್ರುವ {
    UASSERT(m_n_Pre_ಪದಗಳು, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ಧ್ರುವ netlistp = v3Global.rootp();
    File_ಸಾಲು* ಧ್ರುವ flp = netlistp->ತುದಿ_Scopep()->fileline();
    Ast_ಗಂಟು_DType* ಧ್ರುವ u32DTypep = netlistp->findUInt32DType();
    Ast_ಗಂಟು_DType* ಧ್ರುವ strDtypep = netlistp->findStringDType();

    // Dumping function always slow
    ಧ್ರುವ std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* ಧ್ರುವ funcp = util::makeSubFunction(netlistp, name, ಸತ್ಯ);
    funcp->isStatic(ಸತ್ಯ);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* ಧ್ರುವ eVscp = newArgument(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* ಧ್ರುವ tVscp = newArgument(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    ಧ್ರುವ auto rd = [flp](AstVarScope* vp) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, vp, VAccess::READ}; };
    ಧ್ರುವ auto wr = [flp](AstVarScope* vp) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* ಧ್ರುವ vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* ಧ್ರುವ iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->addStmtsp(util::setVar(iVscp, 0));
        // Add loop
        AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
        funcp->addStmtsp(loopp);
        // Loop body
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = ನೂತನ AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = ನೂತನ AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ limp = ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, m_n_Vec_ಪದಗಳು};
        loopp->addStmtsp(ನೂತನ AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(iVscp));
        loopp->addStmtsp(ನೂತನ AstLoopTest{flp, loopp, ನೂತನ AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(vVscp));
        callp->addArgsp(rd(tVscp));
        funcp->addStmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* ಧ್ರುವ pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* ಧ್ರುವ jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->addStmtsp(util::setVar(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = ನೂತನ AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = ನೂತನ AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ limp = ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, m_n_Pre_ಪದಗಳು};
            loopp->addStmtsp(ನೂತನ AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(ನೂತನ AstLoopTest{flp, loopp, ನೂತನ AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
            funcp->addStmtsp(loopp);
            // Loop body
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = ನೂತನ AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = ನೂತನ AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ limp = ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, m_n_Vec_ಪದಗಳು};
            loopp->addStmtsp(ನೂತನ AstAssign{flp, lhsp, rhsp});
            loopp->addStmtsp(util::incrementVar(jVscp));
            loopp->addStmtsp(ನೂತನ AstLoopTest{flp, loopp, ನೂತನ AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, m_dumpp};
        callp->dtypeSetVoid();
        callp->addArgsp(rd(pVscp));
        callp->addArgsp(
            ನೂತನ AstConcatN{flp, rd(tVscp), ನೂತನ AstConst{flp, AstConst::String{}, " pre"}});
        funcp->addStmtsp(callp->makeStmt());
    }

    ಪ್ರತ್ಯಾಗಮ funcp;
}

AstCFunc* TriggerKit::createAnySetFunc(AstUnpackArrayDType* ಧ್ರುವ dtypep) ಧ್ರುವ {
    AstNetlist* ಧ್ರುವ netlistp = v3Global.rootp();
    File_ಸಾಲು* ಧ್ರುವ flp = netlistp->ತುದಿ_Scopep()->fileline();
    Ast_ಗಂಟು_DType* ಧ್ರುವ u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* ಧ್ರುವ funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(ಸತ್ಯ);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* ಧ್ರುವ iVscp = newArgument(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ಧ್ರುವ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    ಧ್ರುವ auto rd = [flp](AstVarScope* vp) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);
    funcp->addStmtsp(ನೂತನ AstCReturn{flp, ನೂತನ AstConst{flp, AstConst::BitFalse{}}});

    // Loop body
    ಧ್ರುವ uint32_t n_ಪದಗಳು = dtypep->elementsConst();
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = ನೂತನ AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ಗಂಟು_Stmt* ಧ್ರುವ thenp = ನೂತನ AstCReturn{flp, ನೂತನ AstConst{flp, AstConst::BitTrue{}}};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ limp = ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, n_ಪದಗಳು};
    loopp->addStmtsp(ನೂತನ AstIf{flp, condp, thenp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(ನೂತನ AstLoopTest{flp, loopp, ನೂತನ AstLt{flp, rd(nVscp), limp}});

    // Done
    ಪ್ರತ್ಯಾಗಮ funcp;
}
AstCFunc* TriggerKit::createClearFunc() ಧ್ರುವ {
    AstNetlist* ಧ್ರುವ netlistp = v3Global.rootp();
    File_ಸಾಲು* ಧ್ರುವ flp = netlistp->ತುದಿ_Scopep()->fileline();
    Ast_ಗಂಟು_DType* ಧ್ರುವ u32DTypep = netlistp->findUInt32DType();

    // Create function
    AstCFunc* ಧ್ರುವ funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(ಸತ್ಯ);

    // Add arguments
    AstVarScope* ಧ್ರುವ oVscp = newArgument(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* ಧ್ರುವ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ಧ್ರುವ auto rd = [flp](AstVarScope* vp) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, vp, VAccess::READ}; };
    ಧ್ರುವ auto wr = [flp](AstVarScope* vp) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = ನೂತನ AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = ನೂತನ AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ limp = ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, m_n_Vec_ಪದಗಳು};
    loopp->addStmtsp(ನೂತನ AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(ನೂತನ AstLoopTest{flp, loopp, ನೂತನ AstLt{flp, rd(nVscp), limp}});

    // Done
    ಪ್ರತ್ಯಾಗಮ funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ಧ್ರುವ oDtypep,
                                       AstUnpackArrayDType* ಧ್ರುವ iDtypep) ಧ್ರುವ {
    AstNetlist* ಧ್ರುವ netlistp = v3Global.rootp();
    File_ಸಾಲು* ಧ್ರುವ flp = netlistp->ತುದಿ_Scopep()->fileline();
    Ast_ಗಂಟು_DType* ಧ್ರುವ u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* ಧ್ರುವ funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(ಸತ್ಯ);

    // Add arguments
    AstVarScope* ಧ್ರುವ oVscp = newArgument(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* ಧ್ರುವ iVscp = newArgument(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ಧ್ರುವ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ಧ್ರುವ auto rd = [flp](AstVarScope* vp) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, vp, VAccess::READ}; };
    ಧ್ರುವ auto wr = [flp](AstVarScope* vp) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
    funcp->addStmtsp(util::setVar(nVscp, 0));
    funcp->addStmtsp(loopp);

    // Loop body
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = ನೂತನ AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ oWordp = ನೂತನ AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ iWordp = ನೂತನ AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = ನೂತನ AstOr{flp, oWordp, iWordp};
    AstConst* ಧ್ರುವ outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ಧ್ರುವ ಒಳಹರಿವು_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ limp = outputRangeLeftp->ಅಂಕೆ().toSInt() < ಒಳಹರಿವು_Range_Leftp->ಅಂಕೆ().toSInt()
                                  ? outputRangeLeftp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)
                                  : ಒಳಹರಿವು_Range_Leftp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
    loopp->addStmtsp(ನೂತನ AstAssign{flp, lhsp, rhsp});
    loopp->addStmtsp(util::incrementVar(nVscp));
    loopp->addStmtsp(ನೂತನ AstLoopTest{flp, loopp, ನೂತನ AstLte{flp, rd(nVscp), limp}});

    // Done
    ಪ್ರತ್ಯಾಗಮ funcp;
}

Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* TriggerKit::newAnySetCall(AstVarScope* ಧ್ರುವ vscp) ಧ್ರುವ {
    File_ಸಾಲು* ಧ್ರುವ flp = v3Global.rootp()->ತುದಿ_Scopep()->fileline();
    ಯದಿ (!m_n_Vec_ಪದಗಳು) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{flp, AstConst::BitFalse{}};

    AstCFunc* funcp = nullptr;
    ಯದಿ (vscp->dtypep() == m_trigVecDTypep) {
        ಯದಿ (!m_anySetVecp) m_anySetVecp = createAnySetFunc(m_trigVecDTypep);
        funcp = m_anySetVecp;
    } ಅನ್ಯಥಾ ಯದಿ (vscp->dtypep() == m_trigExtDTypep) {
        ಯದಿ (!m_anySetExtp) m_anySetExtp = createAnySetFunc(m_trigExtDTypep);
        funcp = m_anySetExtp;
    } ಅನ್ಯಥಾ {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, funcp};
    callp->addArgsp(ನೂತನ AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetBit();
    ಪ್ರತ್ಯಾಗಮ callp;
}
Ast_ಗಂಟು_Stmt* TriggerKit::newClearCall(AstVarScope* ಧ್ರುವ vscp) ಧ್ರುವ {
    ಯದಿ (!m_n_Vec_ಪದಗಳು) ಪ್ರತ್ಯಾಗಮ nullptr;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    ಯದಿ (!m_clearp) m_clearp = createClearFunc();
    File_ಸಾಲು* ಧ್ರುವ flp = v3Global.rootp()->ತುದಿ_Scopep()->fileline();
    AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, m_clearp};
    callp->addArgsp(ನೂತನ AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtypeSetVoid();
    ಪ್ರತ್ಯಾಗಮ callp->makeStmt();
}
Ast_ಗಂಟು_Stmt* TriggerKit::newOrIntoCall(AstVarScope* ಧ್ರುವ oVscp, AstVarScope* ಧ್ರುವ iVscp) ಧ್ರುವ {
    ಯದಿ (!m_n_Vec_ಪದಗಳು) ಪ್ರತ್ಯಾಗಮ nullptr;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    ಧ್ರುವ size_t mask
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[mask];
    ಯದಿ (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_ಸಾಲು* ಧ್ರುವ flp = v3Global.rootp()->ತುದಿ_Scopep()->fileline();
    AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, funcp};
    callp->addArgsp(ನೂತನ AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->addArgsp(ನೂತನ AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtypeSetVoid();
    ಪ್ರತ್ಯಾಗಮ callp->makeStmt();
}

Ast_ಗಂಟು_Stmt* TriggerKit::newCompBaseCall() ಧ್ರುವ {
    ಯದಿ (!m_n_Vec_ಪದಗಳು) ಪ್ರತ್ಯಾಗಮ nullptr;
    File_ಸಾಲು* ಧ್ರುವ flp = v3Global.rootp()->ತುದಿ_Scopep()->fileline();
    AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, m_compVecp};
    callp->dtypeSetVoid();
    ಪ್ರತ್ಯಾಗಮ callp->makeStmt();
}

Ast_ಗಂಟು_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ಧ್ರುವ {
    ಯದಿ (!m_n_Pre_ಪದಗಳು) ಪ್ರತ್ಯಾಗಮ nullptr;
    File_ಸಾಲು* ಧ್ರುವ flp = v3Global.rootp()->ತುದಿ_Scopep()->fileline();
    AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, m_compExtp};
    callp->addArgsp(ನೂತನ AstVarRef{flp, vscp, VAccess::READ});
    callp->dtypeSetVoid();
    ಪ್ರತ್ಯಾಗಮ callp->makeStmt();
}

Ast_ಗಂಟು_Stmt* TriggerKit::newDumpCall(AstVarScope* ಧ್ರುವ vscp, ಧ್ರುವ std::string& tag,
                                     ತಾರ್ಕಿಕ debugOnly) ಧ್ರುವ {
    ಯದಿ (!m_n_Vec_ಪದಗಳು) ಪ್ರತ್ಯಾಗಮ nullptr;
    AstCFunc* funcp = nullptr;
    ಯದಿ (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } ಅನ್ಯಥಾ ಯದಿ (vscp->dtypep() == m_trigExtDTypep) {
        ಯದಿ (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } ಅನ್ಯಥಾ {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_ಸಾಲು* ಧ್ರುವ flp = v3Global.rootp()->ತುದಿ_Scopep()->fileline();
    AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, funcp};
    callp->addArgsp(ನೂತನ AstVarRef{flp, vscp, VAccess::READ});
    callp->addArgsp(ನೂತನ AstConst{flp, AstConst::String{}, tag});
    callp->dtypeSetVoid();
    AstCStmt* ಧ್ರುವ cstmtp = ನೂತನ AstCStmt{flp};
    cstmtp->add("#ifdef VL_DEBUG\n");
    ಯದಿ (debugOnly) {
        cstmtp->add("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->add(callp->makeStmt());
        cstmtp->add("}\n");
    } ಅನ್ಯಥಾ {
        cstmtp->add(callp->makeStmt());
    }
    cstmtp->add("#endif");
    ಪ್ರತ್ಯಾಗಮ cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(ಧ್ರುವ std::string& name) ಧ್ರುವ {
    ಯದಿ (!m_n_Vec_ಪದಗಳು) ಪ್ರತ್ಯಾಗಮ nullptr;
    ಪ್ರತ್ಯಾಗಮ v3Global.rootp()->ತುದಿ_Scopep()->createTemp("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extraTriggerIndex(uint32_t index) ಧ್ರುವ {
    UASSERT(index <= m_n_Extra_ಪದಗಳು * WORD_SIZE, "Invalid external trigger index");
    ಪ್ರತ್ಯಾಗಮ index + m_n_Sense_ಪದಗಳು * WORD_SIZE;
}

std::set<uint32_t> TriggerKit::triggerIndices(ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ* senTreep, ತಾರ್ಕಿಕ pre) ಧ್ರುವ {
    std::set<uint32_t> indices;
    ಪ್ರತಿ (ಧ್ರುವ AstSenItem* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), SenItem)) {
        uint32_t index = m_senItem2TrigIdx.at(*sip);
        ಯದಿ (pre) {
            UASSERT_OBJ(index < m_n_Pre_ಪದಗಳು * WORD_SIZE, sip, "Not a 'pre' trigger");
            index += m_n_Vec_ಪದಗಳು * WORD_SIZE;
        }
        indices.ಸೇರಿಸು(index);
    }
    ಪ್ರತ್ಯಾಗಮ indices;
}

Ast_ಗಂಟು_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ಧ್ರುವ {
    index = extraTriggerIndex(index);
    ಧ್ರುವ uint32_t wordIndex = index / WORD_SIZE;
    ಧ್ರುವ uint32_t bitIndex = index % WORD_SIZE;
    File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
    AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ wordp = ನೂತನ AstArraySel{flp, refp, static_cast<ಪೂರ್ಣಾಙ್ಕ>(wordIndex)};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ trigLhsp = ನೂತನ AstSel{flp, wordp, static_cast<ಪೂರ್ಣಾಙ್ಕ>(bitIndex), 1};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ trigRhsp = ನೂತನ AstVarRef{flp, vscp, VAccess::READ};
    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{flp, trigLhsp, trigRhsp};
}

ಶೂನ್ಯ TriggerKit::addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ಧ್ರುವ {
    File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_ಗಂಟು* ಧ್ರುವ setp = newExtraTriggerAssignment(vscp, index);
    setp->addNext(ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, vscp, VAccess::WRITE},
                                ನೂತನ AstConst{flp, AstConst::BitFalse{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
ಶೂನ್ಯ TriggerKit::add_ಮೌಲ್ಯ_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t index) ಧ್ರುವ {
    index += m_n_Sense_ಪದಗಳು * WORD_SIZE;
    ಧ್ರುವ uint32_t wordIndex = index / WORD_SIZE;
    ಧ್ರುವ uint32_t bitIndex = index % WORD_SIZE;

    Ast_ತುದಿ_Scope* ಧ್ರುವ ತುದಿ_Scopep = netlistp->ತುದಿ_Scopep();
    File_ಸಾಲು* ಧ್ರುವ flp = netlistp->fileline();
    Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ bdtypep = VN_CAST(dtypep, BasicDType)) {
        ಯದಿ (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            ಪ್ರತ್ಯಾಗಮ;
        }
    } ಅನ್ಯಥಾ ಯದಿ (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, ಗಂಟು_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಧ್ರುವ auto rdInst = [flp](AstVarScope* vp) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, vp, VAccess::READ}; };
    ಧ್ರುವ auto wrPrev = [flp](AstVarScope* vp) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, vp, VAccess::WRITE}; };
    ಧ್ರುವ auto rdPrev = [flp](AstVarScope* vp) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    ಧ್ರುವ std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* ಧ್ರುವ prevVscp = ತುದಿ_Scopep->createTemp(prevName, instVscp->dtypep());

    // Initialize prev = inst
    ಯದಿ (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ಧ್ರುವ cmhp = ನೂತನ AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        initFuncp->addStmtsp(cmhp->makeStmt());
    } ಅನ್ಯಥಾ {
        initFuncp->addStmtsp(ನೂತನ AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* neqp;
    ಯದಿ (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ಧ್ರುವ cmhp
            = ನೂತನ AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtypeSetBit();
        neqp = cmhp;
    } ಅನ್ಯಥಾ {
        neqp = ನೂತನ AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_ಗಂಟು* updp;
    ಯದಿ (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ಧ್ರುವ cmhp = ನೂತನ AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ASSIGN, rdInst(instVscp)};
        cmhp->dtypeSetVoid();
        updp = cmhp->makeStmt();
    } ಅನ್ಯಥಾ {
        updp = ನೂತನ AstAssign{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ wordp = ನೂತನ AstArraySel{flp, refp, static_cast<ಪೂರ್ಣಾಙ್ಕ>(wordIndex)};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ trigLhsp = ನೂತನ AstSel{flp, wordp, static_cast<ಪೂರ್ಣಾಙ್ಕ>(bitIndex), 1};
    Ast_ಗಂಟು* ಧ್ರುವ setp = ನೂತನ AstAssign{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->addNext(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ಧ್ರುವ std::string& name, ತಾರ್ಕಿಕ slow, uint32_t n_Sense_ಪದಗಳು,
                       uint32_t n_Extra_ಪದಗಳು, uint32_t n_Pre_ಪದಗಳು,
                       std::unordered_map<VNRef<ಧ್ರುವ AstSenItem>, size_t> senItem2TrigIdx,
                       ಧ್ರುವ std::vector<ಧ್ರುವ AstSenItem*>& senItemps, ತಾರ್ಕಿಕ useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_ಪದಗಳು{n_Sense_ಪದಗಳು}
    , m_n_Extra_ಪದಗಳು{n_Extra_ಪದಗಳು}
    , m_n_Pre_ಪದಗಳು{n_Pre_ಪದಗಳು}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trigIdx2SenItem{senItemps} {
    // If no triggers, we don't need to generate anything
    ಯದಿ (!m_n_Vec_ಪದಗಳು) ಪ್ರತ್ಯಾಗಮ;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trigIdx2SenItem.resize((m_n_Vec_ಪದಗಳು + m_n_Pre_ಪದಗಳು) * WORD_SIZE);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_ಪದಗಳು * WORD_SIZE,
              m_trigIdx2SenItem.begin() + m_n_Vec_ಪದಗಳು * WORD_SIZE);
    AstNetlist* ಧ್ರುವ netlistp = v3Global.rootp();
    AstScope* ಧ್ರುವ scopep = netlistp->ತುದಿ_Scopep()->scopep();
    File_ಸಾಲು* ಧ್ರುವ flp = scopep->fileline();
    // Data type of a single trigger word
    m_wordDTypep = netlistp->findBitDType(WORD_SIZE, WORD_SIZE, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ಧ್ರುವ rp = ನೂತನ AstRange{flp, static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_n_Vec_ಪದಗಳು - 1), 0};
    m_trigVecDTypep = ನೂತನ AstUnpackArrayDType{flp, m_wordDTypep, rp};
    netlistp->typeTablep()->addTypesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    ಯದಿ (m_n_Pre_ಪದಗಳು) {
        AstRange* ಧ್ರುವ ep = ನೂತನ AstRange{flp, static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_n_Vec_ಪದಗಳು + m_n_Pre_ಪದಗಳು - 1), 0};
        m_trigExtDTypep = ನೂತನ AstUnpackArrayDType{flp, m_wordDTypep, ep};
        netlistp->typeTablep()->addTypesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } ಅನ್ಯಥಾ {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->ತುದಿ_Scopep()->createTemp("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->isInternal(ಸತ್ಯ);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, ಸತ್ಯ);
    m_dumpp->isStatic(ಸತ್ಯ);
    m_dumpp->ifdef("VL_DEBUG");
    ಯದಿ (useAcc) {
        m_vscAccp
            = netlistp->ತುದಿ_Scopep()->createTemp("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->isInternal(ಸತ್ಯ);
    }
}

AstAssign* TriggerKit::createSenTrigVecAssignment(AstVarScope* ಧ್ರುವ target,
                                                  std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& trigps) {
    File_ಸಾಲು* ಧ್ರುವ flp = target->fileline();
    AstAssign* trigStmtsp = nullptr;
    // Assign sense triggers vector one word at a time
    ಪ್ರತಿ (size_t i = 0; i < trigps.size(); i += WORD_SIZE) {
        // Concatenate all bits in this trigger word using a balanced
        ಪ್ರತಿ (uint32_t level = 0; level < WORD_SIZE_LOG2; ++level) {
            ಧ್ರುವ uint32_t stride = 1 << level;
            ಪ್ರತಿ (uint32_t j = 0; j < WORD_SIZE; j += 2 * stride) {
                trigps[i + j] = ನೂತನ AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = nullptr;
            }
        }

        // Set the whole word in the trigger vector
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ wordIndex = static_cast<ಪೂರ್ಣಾಙ್ಕ>(i / WORD_SIZE);
        AstArraySel* ಧ್ರುವ aselp
            = ನೂತನ AstArraySel{flp, ನೂತನ AstVarRef{flp, target, VAccess::WRITE}, wordIndex};
        trigStmtsp = Ast_ಗಂಟು::addNext(trigStmtsp, ನೂತನ AstAssign{flp, aselp, trigps[i]});
    }
    ಪ್ರತ್ಯಾಗಮ trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ಧ್ರುವ initFuncp,  //
                              Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& sen_ಅಭಿವ್ಯಕ್ತಿ_Builder,  //
                              ಧ್ರುವ std::vector<ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ*>& preTreeps,  //
                              ಧ್ರುವ std::vector<ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ*>& senTreeps,  //
                              ಧ್ರುವ string& name,  //
                              ಧ್ರುವ ExtraTriggers& extraTriggers,  //
                              ತಾರ್ಕಿಕ slow,  //
                              ತಾರ್ಕಿಕ useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::vector<ಧ್ರುವ AstSenItem*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<ಧ್ರುವ AstSenItem>, size_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    ಪ್ರತಿ (ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep : preTreeps) {
        ಪ್ರತಿ (ಧ್ರುವ AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ಧ್ರುವ auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            ಯದಿ (pair.second) senItemps.ತಳ್ಳು_back(itemp);
        }
    }
    ಧ್ರುವ uint32_t n_Pre_Sen_ವಸ್ತುಗಳು = senItemps.size();
    V3Stats::addStat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_ವಸ್ತುಗಳು);
    // Number of pre triggers, rounded up to a full word.
    ಧ್ರುವ uint32_t nPreTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ಧ್ರುವ uint32_t n_Pre_ಪದಗಳು = nPreTriggers / WORD_SIZE;

    // Process the rest of the trees
    ಪ್ರತಿ (ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep : senTreeps) {
        ಪ್ರತಿ (ಧ್ರುವ AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ಧ್ರುವ auto pair = senItem2TrigIdx.emplace(*itemp, senItemps.size());
            ಯದಿ (pair.second) senItemps.ತಳ್ಳು_back(itemp);
        }
    }
    ಧ್ರುವ uint32_t n_Sen_ವಸ್ತುಗಳು = senItemps.size() - nPreTriggers;
    V3Stats::addStat("Scheduling, '" + name + "' sense triggers", n_Sen_ವಸ್ತುಗಳು + n_Pre_Sen_ವಸ್ತುಗಳು);
    // Number of sense triggers, rounded up to a full word
    ಧ್ರುವ uint32_t nSenseTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(senItemps.size());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ಧ್ರುವ uint32_t n_Sense_ಪದಗಳು = nSenseTriggers / WORD_SIZE;

    // Allocate space for the extra triggers
    V3Stats::addStat("Scheduling, '" + name + "' extra triggers", extraTriggers.size());
    // Number of extra triggers, rounded up to a full word.
    ಧ್ರುವ uint32_t nExtraTriggers = vlstd::roundUpToMultipleOf<WORD_SIZE>(extraTriggers.size());
    ಧ್ರುವ uint32_t n_Extra_ಪದಗಳು = nExtraTriggers / WORD_SIZE;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_ಪದಗಳು, n_Extra_ಪದಗಳು, n_Pre_ಪದಗಳು, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    ಯದಿ (!kit.m_n_Vec_ಪದಗಳು) ಪ್ರತ್ಯಾಗಮ kit;

    File_ಸಾಲು* ಧ್ರುವ flp = netlistp->ತುದಿ_Scopep()->fileline();

    // Creates read/write reference
    ಧ್ರುವ auto rd = [flp](AstVarScope* vp) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, vp, VAccess::READ}; };
    ಧ್ರುವ auto wr = [flp](AstVarScope* vp) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* ಧ್ರುವ dumpTrgp
        = newArgument(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* ಧ್ರುವ dumpTagp
        = newArgument(kit.m_dumpp, netlistp->findStringDType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, ನೂತನ AstLogNot{flp, kit.newAnySetCall(dumpTrgp)}};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* ಧ್ರುವ cstmtp = ನೂತನ AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    ಧ್ರುವ auto addDebug = [&](uint32_t index, ಧ್ರುವ string& text) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ wrdIndex = static_cast<ಪೂರ್ಣಾಙ್ಕ>(index / WORD_SIZE);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ bitIndex = static_cast<ಪೂರ್ಣಾಙ್ಕ>(index % WORD_SIZE);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ aselp = ನೂತನ AstArraySel{flp, rd(dumpTrgp), wrdIndex};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = ನೂತನ AstSel{flp, aselp, bitIndex, 1};
        AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, condp};
        kit.m_dumpp->addStmtsp(ifp);
        AstCStmt* ಧ್ರುವ cstmtp = ನೂತನ AstCStmt{flp};
        ifp->addThensp(cstmtp);
        cstmtp->add("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->add(rd(dumpTagp));
        cstmtp->add(" + \"' region trigger index " + std::to_string(index) + " is active: " + text
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_ಗಂಟು_Stmt* initialTrigsp = nullptr;
    ಪ್ರತಿ (size_t i = 0; i < senItemps.size(); ++i) {
        ಧ್ರುವ AstSenItem* ಧ್ರುವ senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        ಯದಿ (!senItemp) {
            trigps.emplace_back(ನೂತನ AstConst{flp, AstConst::BitFalse{}});
            ಅನುವರ್ತ;
        }

        // Create the trigger computation expression
        ಧ್ರುವ auto& pair = sen_ಅಭಿವ್ಯಕ್ತಿ_Builder.build(senItemp);
        trigps.emplace_back(pair.first);

        // Add initialization time trigger
        ಯದಿ (pair.second || v3Global.opt.xInitialEdge()) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ wrdIndex = static_cast<ಪೂರ್ಣಾಙ್ಕ>(i / WORD_SIZE);
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ bitIndex = static_cast<ಪೂರ್ಣಾಙ್ಕ>(i % WORD_SIZE);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ wordp = ನೂತನ AstArraySel{flp, wr(kit.m_vscp), wrdIndex};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = ನೂತನ AstSel{flp, wordp, bitIndex, 1};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = ನೂತನ AstConst{flp, AstConst::BitTrue{}};
            ಯದಿ (useAcc) {
                initFuncp->addStmtsp(ನೂತನ AstAssign{flp, lhsp, rhsp});
            } ಅನ್ಯಥಾ {
                initialTrigsp = Ast_ಗಂಟು::addNext(initialTrigsp, ನೂತನ AstAssign{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_ವೃಕ್ಷ(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        addDebug(i, desc);
    }
    UASSERT(trigps.size() == nSenseTriggers, "Inconsistent number of trigger expressions");

    AstAssign* ಧ್ರುವ trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    ಪ್ರತಿ (unsigned i = 0; i < extraTriggers.size(); ++i) {
        addDebug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    ಧ್ರುವ Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder::Results senResults = sen_ಅಭಿವ್ಯಕ್ತಿ_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    ಪ್ರತಿ (Ast_ಗಂಟು_Stmt* ಧ್ರುವ nodep : senResults.m_inits) initFuncp->addStmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_ತುದಿ_Scope* ಧ್ರುವ ತುದಿ_Scopep = netlistp->ತುದಿ_Scopep();
    {
        AstCFunc* ಧ್ರುವ fp = kit.m_compVecp;
        // Trigger computation
        ಪ್ರತಿ (Ast_ಗಂಟು_Stmt* ಧ್ರುವ nodep : senResults.m_preUpdates) fp->addStmtsp(nodep);
        fp->addStmtsp(trigStmtsp);
        ಪ್ರತಿ (Ast_ಗಂಟು_Stmt* ಧ್ರುವ nodep : senResults.m_postUpdates) fp->addStmtsp(nodep);
        // Add the initialization time triggers
        ಯದಿ (initialTrigsp) {
            AstVarScope* ಧ್ರುವ initVscp = ತುದಿ_Scopep->createTemp("__V" + name + "DidInit", 1);
            AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, ನೂತನ AstNot{flp, rd(initVscp)}};
            fp->addStmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->addThensp(util::setVar(initVscp, 1));
            ifp->addThensp(initialTrigsp);
        }
        util::splitCheck(fp);
    };
    // If there are 'pre' triggers, compute them
    ಯದಿ (kit.m_n_Pre_ಪದಗಳು) {
        AstCFunc* ಧ್ರುವ fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ಧ್ರುವ latchedp
            = newArgument(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ಧ್ರುವ nVscp = ತುದಿ_Scopep->createTemp("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(ಸತ್ಯ);
        // Add a loop to compute the pre words
        AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
        fp->addStmtsp(util::setVar(nVscp, 0));
        fp->addStmtsp(loopp);
        // Loop body
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ offsetp = ನೂತನ AstConst{flp, kit.m_n_Vec_ಪದಗಳು};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lIdxp = ನೂತನ AstAdd{flp, rd(nVscp), offsetp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = ನೂತನ AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ aWordp = ನೂತನ AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bWordp = ನೂತನ AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = ನೂತನ AstAnd{flp, aWordp, ನೂತನ AstNot{flp, bWordp}};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ limp = ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, n_Pre_ಪದಗಳು};
        loopp->addStmtsp(ನೂತನ AstAssign{flp, lhsp, rhsp});
        loopp->addStmtsp(util::incrementVar(nVscp));
        loopp->addStmtsp(ನೂತನ AstLoopTest{flp, loopp, ನೂತನ AstLt{flp, rd(nVscp), limp}});
        util::splitCheck(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    ಯದಿ (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlinkFrBackWithNext()->delete_ವೃಕ್ಷ();
    // Done with the trigger dump function, split as might be large
    util::splitCheck(kit.m_dumpp);

    ಪ್ರತ್ಯಾಗಮ kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
ವರ್ಗ AwaitBeforeTrigVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    ಧ್ರುವ VNUser1InUse m_user1InUse;

    /**
     * AstCAwait::user1()       ->  bool.           True if node has been visited
     * AstSenTree::user1p()     ->  AstCFunc*.      Function that has to be called before awaiting
     *                                              for CAwait pointing to this SenTree
     * AstCFunc::user1p()       ->  AstVarScope*    Function's local temporary extended trigger
     *                                              vector variable scope
     */

    // Netlist - needed for using util::makeSubFunction()
    AstNetlist* ಧ್ರುವ m_netlistp;
    // Trigger kit - for accessing trigger vectors and mapping senItems to thier indexes
    ಧ್ರುವ TriggerKit& m_trigKit;
    // Expression builder - for building expressions from SenItems
    Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builder;
    // Generator of unique names for before-trigger function
    V3UniqueNames m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::vector<std::pair<AstCFunc*, Ast_Sen_ವೃಕ್ಷ*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::vector<std::pair<Ast_Sen_ವೃಕ್ಷ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> m_sen_ವೃಕ್ಷ_To_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_ಗಂಟು>, std::vector<AstSenItem*>> m_sen_ಅಭಿವ್ಯಕ್ತಿ_To_Sen_Item;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    ಸ್ಥಿರ Ast_ಗಂಟು* get_Sen_Hash_ಗಂಟು(ಧ್ರುವ AstSenItem* ಧ್ರುವ nodep) {
        ಯದಿ (AstVarRef* ಧ್ರುವ varRefp = VN_CAST(nodep->sensp(), VarRef)) ಪ್ರತ್ಯಾಗಮ varRefp;
        ಪ್ರತ್ಯಾಗಮ nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    ಶೂನ್ಯ fill_Sen_ಅಭಿವ್ಯಕ್ತಿ_To_Sen_Item() {
        ಪ್ರತಿ (auto sen_ವೃಕ್ಷ_Sched : m_sen_ವೃಕ್ಷ_To_Sched) {
            Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = sen_ವೃಕ್ಷ_Sched.first;

            ಪ್ರತಿ (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ಧ್ರುವ VEdgeType edge = senItemp->edgeType();
                ಯದಿ (edge.anEdge() || edge == VEdgeType::ET_CHANGED
                    || edge == VEdgeType::ET_HYBRID) {
                    m_sen_ಅಭಿವ್ಯಕ್ತಿ_To_Sen_Item[*get_Sen_Hash_ಗಂಟು(senItemp)].ತಳ್ಳು_back(senItemp);
                }
            }
        }

        std::vector<VNRef<Ast_ಗಂಟು>> toRemove;
        ಪ್ರತಿ (ಧ್ರುವ auto& sen_ಅಭಿವ್ಯಕ್ತಿ_To_Sen_ವೃಕ್ಷ : m_sen_ಅಭಿವ್ಯಕ್ತಿ_To_Sen_Item) {
            std::vector<AstSenItem*> senItemps = sen_ಅಭಿವ್ಯಕ್ತಿ_To_Sen_ವೃಕ್ಷ.second;
            toRemove.ತಳ್ಳು_back(sen_ಅಭಿವ್ಯಕ್ತಿ_To_Sen_ವೃಕ್ಷ.first);
            ಪ್ರತಿ (size_t i = 1; i < senItemps.size(); ++i) {
                ಯದಿ (senItemps[i]->edgeType() != senItemps[i - 1]->edgeType()) {
                    toRemove.ತೆಗೆ_back();
                    ಭಙ್ಗ;
                }
            }
        }
        ಪ್ರತಿ (ಧ್ರುವ VNRef<Ast_ಗಂಟು> it : toRemove) m_sen_ಅಭಿವ್ಯಕ್ತಿ_To_Sen_Item.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<size_t, std::map<size_t, std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>>>
    getUsedTriggersToTrees(ಧ್ರುವ std::set<size_t>& usedTriggers) {
        std::map<size_t, std::map<size_t, std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>>> usedTrigsToUsingTrees;
        ಪ್ರತಿ (auto sen_ವೃಕ್ಷ_Sched : m_sen_ವೃಕ್ಷ_To_Sched) {
            ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = sen_ವೃಕ್ಷ_Sched.first;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ shedp = sen_ವೃಕ್ಷ_Sched.second;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            ಪ್ರತಿ (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ಧ್ರುವ size_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                ಯದಿ (usedTriggers.find(idx) != usedTriggers.end()) {
                    usedTrigsToUsingTrees[idx / TriggerKit::WORD_SIZE]
                                         [size_t{1} << (idx % TriggerKit::WORD_SIZE)]
                                             .ತಳ್ಳು_back(shedp);
                }
            }
        }
        ಯದಿ (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            ಪ್ರತಿ (ಧ್ರುವ auto& triggersToTrees : usedTrigsToUsingTrees) {
                ಪ್ರತಿ (ಧ್ರುವ auto& bitsToTrees : triggersToTrees.second) {
                    ಧ್ರುವ std::set<ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> exprps{bitsToTrees.second.begin(),
                                                              bitsToTrees.second.end()};
                    UASSERT(bitsToTrees.second.size() == exprps.size(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        ಪ್ರತ್ಯಾಗಮ usedTrigsToUsingTrees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep) {
        File_ಸಾಲು* ಧ್ರುವ flp = senTreep->fileline();
        ಯದಿ (!senTreep->user1p()) {
            AstCFunc* ಧ್ರುವ funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), ಅಸತ್ಯ);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* ಧ್ರುವ tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(ಸತ್ಯ);

            AstVar* ಧ್ರುವ argp = ನೂತನ AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->findBasicDType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(ಸತ್ಯ);
            argp->direction(VDirection::ಒಳಹರಿವು);
            funcp->addArgsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_back(funcp, senTreep);
        }
        AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtypeSetVoid();
        ಪ್ರತ್ಯಾಗಮ callp;
    }

    ಶೂನ್ಯ visit(AstCAwait* ಧ್ರುವ nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        ಯದಿ (ಧ್ರುವ AstCMethodHard* ಧ್ರುವ cMethodHardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            ಯದಿ (cMethodHardp->method() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ಧ್ರುವ beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_ಗಂಟು* ಧ್ರುವ pinp = cMethodHardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, cMethodHardp, "No event description");
                beforeTrigp->addArgsp(VN_AS(pinp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));

                // Call the before-trigger function before the CAwait
                nodep->addHereThisAsNext(beforeTrigp->makeStmt());
                m_sen_ವೃಕ್ಷ_To_Sched.emplace_back(nodep->sentreep(), cMethodHardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* ಧ್ರುವ nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& sen_ಅಭಿವ್ಯಕ್ತಿ_Builder,
                           ಧ್ರುವ TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builder{sen_ಅಭಿವ್ಯಕ್ತಿ_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_ಅಭಿವ್ಯಕ್ತಿ_To_Sen_Item();

        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> trigps;
        std::set<size_t> usedTriggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        ಪ್ರತಿ (ಧ್ರುವ auto& funcToUsedTriggers : m_generatedFuncs) {
            AstCFunc* ಧ್ರುವ funcp = funcToUsedTriggers.first;
            AstVarScope* ಧ್ರುವ vscp = VN_AS(funcp->user1p(), VarScope);
            File_ಸಾಲು* ಧ್ರುವ flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = funcToUsedTriggers.second;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ಧ್ರುವ auto emplaceAt
                    = [flp, &trigps, &usedTriggers](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp, ಧ್ರುವ size_t pos) {
                          ಧ್ರುವ size_t targetSize
                              = vlstd::roundUpToMultipleOf<TriggerKit::WORD_SIZE>(pos + 1);
                          ಯದಿ (trigps.capacity() < targetSize) trigps.reserve(targetSize * 2);
                          ಯಾವತ್ (trigps.size() < targetSize) {
                              trigps.ತಳ್ಳು_back(ನೂತನ AstConst{flp, AstConst::BitFalse{}});
                          }
                          trigps[pos]->delete_ವೃಕ್ಷ();
                          trigps[pos] = exprp;
                          usedTriggers.ಸೇರಿಸು(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                ಪ್ರತಿ (ಧ್ರುವ AstSenItem* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), SenItem)) {
                    ಧ್ರುವ size_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builder.build(itemp).first, idx);
                    auto iter = m_sen_ಅಭಿವ್ಯಕ್ತಿ_To_Sen_Item.find(*get_Sen_Hash_ಗಂಟು(itemp));
                    ಯದಿ (iter != m_sen_ಅಭಿವ್ಯಕ್ತಿ_To_Sen_Item.end()) {
                        ಪ್ರತಿ (AstSenItem* ಧ್ರುವ additionalItemp : iter->second) {
                            ಧ್ರುವ size_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builder.build(additionalItemp).first, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ಧ್ರುವ Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder::Results results
                    = m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builder.getResultsAndClearUpdates();
                ಪ್ರತಿ (Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtsp : results.m_inits) funcp->addStmtsp(stmtsp);
                ಪ್ರತಿ (Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtsp : results.m_preUpdates) funcp->addStmtsp(stmtsp);
                funcp->addStmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                ಪ್ರತಿ (Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtsp : results.m_postUpdates) funcp->addStmtsp(stmtsp);
            }

            ಧ್ರುವ std::map<size_t, std::map<size_t, std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>>>
                usedTrigsToUsingTrees = getUsedTriggersToTrees(usedTriggers);
            usedTriggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            ಧ್ರುವ auto getIdx = [flp](AstVarScope* ಧ್ರುವ scocep, VAccess access, size_t idx) {
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstArraySel{flp, ನೂತನ AstVarRef{flp, scocep, access},
                                       ನೂತನ AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* ಧ್ರುವ argpVscp = ನೂತನ AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->addVarsp(argpVscp);

            // Mark as ready triggered schedulers
            ಪ್ರತಿ (ಧ್ರುವ auto& triggersToTrees : usedTrigsToUsingTrees) {
                ಧ್ರುವ size_t word = triggersToTrees.first;

                ಪ್ರತಿ (ಧ್ರುವ auto& bitsToTrees : triggersToTrees.second) {
                    ಧ್ರುವ size_t bit = bitsToTrees.first;
                    ಧ್ರುವ auto& schedulers = bitsToTrees.second;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* ಧ್ರುವ maskConstp = ನೂತನ AstConst{flp, AstConst::Unsized64{}, bit};
                    AstAnd* ಧ್ರುವ condp
                        = ನೂತನ AstAnd{flp, getIdx(vscp, VAccess::READ, word), maskConstp};
                    AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ schedp : schedulers) {
                        AstCMethodHard* ಧ್ರುವ callp = ನೂತನ AstCMethodHard{
                            flp, schedp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), VCMethod::SCHED_READY};
                        callp->dtypeSetVoid();
                        callp->addPinsp(ನೂತನ AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->addThensp(callp->makeStmt());
                    }
                    funcp->addStmtsp(ifp);
                }
            }

            AstVarScope* ಧ್ರುವ vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            ಪ್ರತಿ (ಧ್ರುವ auto& triggersToTrees : usedTrigsToUsingTrees) {
                ಧ್ರುವ size_t word = triggersToTrees.first;
                funcp->addStmtsp(ನೂತನ AstAssign{flp, getIdx(vscAccp, VAccess::WRITE, word),
                                               ನೂತನ AstOr{flp, getIdx(vscAccp, VAccess::READ, word),
                                                         getIdx(vscp, VAccess::READ, word)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = ಸ್ವಭಾವ;
};

ಶೂನ್ಯ beforeTrigVisitor(AstNetlist* netlistp, Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& sen_ಅಭಿವ್ಯಕ್ತಿ_Builder,
                       ಧ್ರುವ TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_ಅಭಿವ್ಯಕ್ತಿ_Builder, trigKit};
}

}  // namespace V3Sched
