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

ಬಳಸಿ util::new_ವಾದ;

AstVarScope* newLocal(AstCFunc* funcp, Ast_ಗಂಟು_DType* dtypep, ಸ್ಥಿರಾಂಕ std::string& name) {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = funcp->fileline();
    AstScope* ಸ್ಥಿರಾಂಕ scopep = funcp->scopep();
    AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(ನಿಜ);
    varp->noReset(ನಿಜ);
    funcp->ಕೂಡಿಸು_Varsp(varp);
    AstVarScope* ಸ್ಥಿರಾಂಕ vscp = ಹೊಸ AstVarScope{flp, scopep, varp};
    scopep->ಕೂಡಿಸು_Varsp(vscp);
    ಹಿಂತಿರುಗಿ vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() ಸ್ಥಿರಾಂಕ {
    UASSERT(m_n_Pre_ಪದಗಳು, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ಸ್ಥಿರಾಂಕ netlistp = v3Global.rootp();
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = netlistp->ತುದಿ_Scopep()->fileline();
    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ u32DTypep = netlistp->ಕಂಡುಹಿಡಿ_UInt32_DType();
    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ strDtypep = netlistp->ಕಂಡುಹಿಡಿ_String_DType();

    // Dumping function always slow
    ಸ್ಥಿರಾಂಕ std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp = util::makeSubFunction(netlistp, name, ನಿಜ);
    funcp->isStatic(ನಿಜ);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* ಸ್ಥಿರಾಂಕ eVscp = new_ವಾದ(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* ಸ್ಥಿರಾಂಕ tVscp = new_ವಾದ(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ rd = [flp](AstVarScope* vp) { ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, vp, VAccess::READ}; };
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ wr = [flp](AstVarScope* vp) { ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* ಸ್ಥಿರಾಂಕ vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* ಸ್ಥಿರಾಂಕ iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->ಕೂಡಿಸು_Stmtsp(util::ಗಣ_Var(iVscp, 0));
        // Add loop
        AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
        funcp->ಕೂಡಿಸು_Stmtsp(loopp);
        // Loop body
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = ಹೊಸ AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ limp = ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, m_n_Vec_ಪದಗಳು};
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsp, rhsp});
        loopp->ಕೂಡಿಸು_Stmtsp(util::incrementVar(iVscp));
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{flp, loopp, ಹೊಸ AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, m_dumpp};
        callp->dtype_ಗಣ_Void();
        callp->ಕೂಡಿಸು_Argsp(rd(vVscp));
        callp->ಕೂಡಿಸು_Argsp(rd(tVscp));
        funcp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* ಸ್ಥಿರಾಂಕ pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* ಸ್ಥಿರಾಂಕ jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->ಕೂಡಿಸು_Stmtsp(util::ಗಣ_Var(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
            funcp->ಕೂಡಿಸು_Stmtsp(loopp);
            // Loop body
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = ಹೊಸ AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ limp = ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, m_n_Pre_ಪದಗಳು};
            loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsp, rhsp});
            loopp->ಕೂಡಿಸು_Stmtsp(util::incrementVar(jVscp));
            loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{flp, loopp, ಹೊಸ AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
            funcp->ಕೂಡಿಸು_Stmtsp(loopp);
            // Loop body
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = ಹೊಸ AstConst{flp, AstConst::DTyped{}, m_ಶಬ್ದ_DTypep};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ limp = ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, m_n_Vec_ಪದಗಳು};
            loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsp, rhsp});
            loopp->ಕೂಡಿಸು_Stmtsp(util::incrementVar(jVscp));
            loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{flp, loopp, ಹೊಸ AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, m_dumpp};
        callp->dtype_ಗಣ_Void();
        callp->ಕೂಡಿಸು_Argsp(rd(pVscp));
        callp->ಕೂಡಿಸು_Argsp(
            ಹೊಸ AstConcatN{flp, rd(tVscp), ಹೊಸ AstConst{flp, AstConst::String{}, " pre"}});
        funcp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
    }

    ಹಿಂತಿರುಗಿ funcp;
}

AstCFunc* TriggerKit::create_Any_ಗಣ_Func(AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ dtypep) ಸ್ಥಿರಾಂಕ {
    AstNetlist* ಸ್ಥಿರಾಂಕ netlistp = v3Global.rootp();
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = netlistp->ತುದಿ_Scopep()->fileline();
    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ u32DTypep = netlistp->ಕಂಡುಹಿಡಿ_UInt32_DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(ನಿಜ);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* ಸ್ಥಿರಾಂಕ iVscp = new_ವಾದ(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ಸ್ಥಿರಾಂಕ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ rd = [flp](AstVarScope* vp) { ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
    funcp->ಕೂಡಿಸು_Stmtsp(util::ಗಣ_Var(nVscp, 0));
    funcp->ಕೂಡಿಸು_Stmtsp(loopp);
    funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCReturn{flp, ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}}});

    // Loop body
    ಸ್ಥಿರಾಂಕ uint32_t n_ಪದಗಳು = dtypep->elementsConst();
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = ಹೊಸ AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ thenp = ಹೊಸ AstCReturn{flp, ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}}};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ limp = ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, n_ಪದಗಳು};
    loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{flp, condp, thenp});
    loopp->ಕೂಡಿಸು_Stmtsp(util::incrementVar(nVscp));
    loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{flp, loopp, ಹೊಸ AstLt{flp, rd(nVscp), limp}});

    //ಮುಗಿದ
    ಹಿಂತಿರುಗಿ funcp;
}
AstCFunc* TriggerKit::createClearFunc() ಸ್ಥಿರಾಂಕ {
    AstNetlist* ಸ್ಥಿರಾಂಕ netlistp = v3Global.rootp();
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = netlistp->ತುದಿ_Scopep()->fileline();
    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ u32DTypep = netlistp->ಕಂಡುಹಿಡಿ_UInt32_DType();

    // Create function
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(ನಿಜ);

    // Add arguments
    AstVarScope* ಸ್ಥಿರಾಂಕ oVscp = new_ವಾದ(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* ಸ್ಥಿರಾಂಕ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ rd = [flp](AstVarScope* vp) { ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, vp, VAccess::READ}; };
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ wr = [flp](AstVarScope* vp) { ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
    funcp->ಕೂಡಿಸು_Stmtsp(util::ಗಣ_Var(nVscp, 0));
    funcp->ಕೂಡಿಸು_Stmtsp(loopp);

    // Loop body
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = ಹೊಸ AstConst{flp, AstConst::DTyped{}, m_ಶಬ್ದ_DTypep};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ limp = ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, m_n_Vec_ಪದಗಳು};
    loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsp, rhsp});
    loopp->ಕೂಡಿಸು_Stmtsp(util::incrementVar(nVscp));
    loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{flp, loopp, ಹೊಸ AstLt{flp, rd(nVscp), limp}});

    //ಮುಗಿದ
    ಹಿಂತಿರುಗಿ funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ oDtypep,
                                       AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ iDtypep) ಸ್ಥಿರಾಂಕ {
    AstNetlist* ಸ್ಥಿರಾಂಕ netlistp = v3Global.rootp();
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = netlistp->ತುದಿ_Scopep()->fileline();
    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ u32DTypep = netlistp->ಕಂಡುಹಿಡಿ_UInt32_DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(ನಿಜ);

    // Add arguments
    AstVarScope* ಸ್ಥಿರಾಂಕ oVscp = new_ವಾದ(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* ಸ್ಥಿರಾಂಕ iVscp = new_ವಾದ(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ಸ್ಥಿರಾಂಕ nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ rd = [flp](AstVarScope* vp) { ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, vp, VAccess::READ}; };
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ wr = [flp](AstVarScope* vp) { ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
    funcp->ಕೂಡಿಸು_Stmtsp(util::ಗಣ_Var(nVscp, 0));
    funcp->ಕೂಡಿಸು_Stmtsp(loopp);

    // Loop body
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ oWordp = ಹೊಸ AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ iWordp = ಹೊಸ AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = ಹೊಸ AstOr{flp, oWordp, iWordp};
    AstConst* ಸ್ಥಿರಾಂಕ outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ಸ್ಥಿರಾಂಕ ಒಳಹರಿವು_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ limp = outputRangeLeftp->ಅಂಕೆ().ಗಮ್ಯ_SInt() < ಒಳಹರಿವು_Range_Leftp->ಅಂಕೆ().ಗಮ್ಯ_SInt()
                                  ? outputRangeLeftp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)
                                  : ಒಳಹರಿವು_Range_Leftp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
    loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsp, rhsp});
    loopp->ಕೂಡಿಸು_Stmtsp(util::incrementVar(nVscp));
    loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{flp, loopp, ಹೊಸ AstLte{flp, rd(nVscp), limp}});

    //ಮುಗಿದ
    ಹಿಂತಿರುಗಿ funcp;
}

Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* TriggerKit::new_Any_ಗಣ_Call(AstVarScope* ಸ್ಥಿರಾಂಕ vscp) ಸ್ಥಿರಾಂಕ {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = v3Global.rootp()->ತುದಿ_Scopep()->fileline();
    ಆದರೆ (!m_n_Vec_ಪದಗಳು) ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}};

    AstCFunc* funcp = ಶೂನ್ಯಸೂಚಕ;
    ಆದರೆ (vscp->dtypep() == m_trigVecDTypep) {
        ಆದರೆ (!m_any_ಗಣ_Vecp) m_any_ಗಣ_Vecp = create_Any_ಗಣ_Func(m_trigVecDTypep);
        funcp = m_any_ಗಣ_Vecp;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (vscp->dtypep() == m_trigExtDTypep) {
        ಆದರೆ (!m_any_ಗಣ_Extp) m_any_ಗಣ_Extp = create_Any_ಗಣ_Func(m_trigExtDTypep);
        funcp = m_any_ಗಣ_Extp;
    } ಇಲ್ಲದಿದ್ದರೆ {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, funcp};
    callp->ಕೂಡಿಸು_Argsp(ಹೊಸ AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_ಗಣ_ಬಿಟ್();
    ಹಿಂತಿರುಗಿ callp;
}
Ast_ಗಂಟು_Stmt* TriggerKit::newClearCall(AstVarScope* ಸ್ಥಿರಾಂಕ vscp) ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (!m_n_Vec_ಪದಗಳು) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    ಆದರೆ (!m_clearp) m_clearp = createClearFunc();
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = v3Global.rootp()->ತುದಿ_Scopep()->fileline();
    AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, m_clearp};
    callp->ಕೂಡಿಸು_Argsp(ಹೊಸ AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_ಗಣ_Void();
    ಹಿಂತಿರುಗಿ callp->makeStmt();
}
Ast_ಗಂಟು_Stmt* TriggerKit::newOrIntoCall(AstVarScope* ಸ್ಥಿರಾಂಕ oVscp, AstVarScope* ಸ್ಥಿರಾಂಕ iVscp) ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (!m_n_Vec_ಪದಗಳು) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಮುಖವಾಡ
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[ಮುಖವಾಡ];
    ಆದರೆ (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = v3Global.rootp()->ತುದಿ_Scopep()->fileline();
    AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, funcp};
    callp->ಕೂಡಿಸು_Argsp(ಹೊಸ AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->ಕೂಡಿಸು_Argsp(ಹೊಸ AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtype_ಗಣ_Void();
    ಹಿಂತಿರುಗಿ callp->makeStmt();
}

Ast_ಗಂಟು_Stmt* TriggerKit::new_Comp_ತಳ_Call() ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (!m_n_Vec_ಪದಗಳು) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = v3Global.rootp()->ತುದಿ_Scopep()->fileline();
    AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, m_compVecp};
    callp->dtype_ಗಣ_Void();
    ಹಿಂತಿರುಗಿ callp->makeStmt();
}

Ast_ಗಂಟು_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (!m_n_Pre_ಪದಗಳು) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = v3Global.rootp()->ತುದಿ_Scopep()->fileline();
    AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, m_compExtp};
    callp->ಕೂಡಿಸು_Argsp(ಹೊಸ AstVarRef{flp, vscp, VAccess::READ});
    callp->dtype_ಗಣ_Void();
    ಹಿಂತಿರುಗಿ callp->makeStmt();
}

Ast_ಗಂಟು_Stmt* TriggerKit::newDumpCall(AstVarScope* ಸ್ಥಿರಾಂಕ vscp, ಸ್ಥಿರಾಂಕ std::string& ಟ್ಯಾಗ್,
                                     ತಾರ್ಕಿಕ debugOnly) ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (!m_n_Vec_ಪದಗಳು) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    AstCFunc* funcp = ಶೂನ್ಯಸೂಚಕ;
    ಆದರೆ (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (vscp->dtypep() == m_trigExtDTypep) {
        ಆದರೆ (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } ಇಲ್ಲದಿದ್ದರೆ {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = v3Global.rootp()->ತುದಿ_Scopep()->fileline();
    AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, funcp};
    callp->ಕೂಡಿಸು_Argsp(ಹೊಸ AstVarRef{flp, vscp, VAccess::READ});
    callp->ಕೂಡಿಸು_Argsp(ಹೊಸ AstConst{flp, AstConst::String{}, ಟ್ಯಾಗ್});
    callp->dtype_ಗಣ_Void();
    AstCStmt* ಸ್ಥಿರಾಂಕ cstmtp = ಹೊಸ AstCStmt{flp};
    cstmtp->ಕೂಡಿಸು("#ifdef VL_DEBUG\n");
    ಆದರೆ (debugOnly) {
        cstmtp->ಕೂಡಿಸು("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->ಕೂಡಿಸು(callp->makeStmt());
        cstmtp->ಕೂಡಿಸು("}\n");
    } ಇಲ್ಲದಿದ್ದರೆ {
        cstmtp->ಕೂಡಿಸು(callp->makeStmt());
    }
    cstmtp->ಕೂಡಿಸು("#endif");
    ಹಿಂತಿರುಗಿ cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(ಸ್ಥಿರಾಂಕ std::string& name) ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (!m_n_Vec_ಪದಗಳು) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    ಹಿಂತಿರುಗಿ v3Global.rootp()->ತುದಿ_Scopep()->create_ತಾತ್ಕಾಲಿಕ("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extra_Trigger_ಸೂಚ್ಯಂಕ(uint32_t ಸೂಚ್ಯಂಕ) ಸ್ಥಿರಾಂಕ {
    UASSERT(ಸೂಚ್ಯಂಕ <= m_n_Extra_ಪದಗಳು * ಶಬ್ದ_ಗಾತ್ರ, "Invalid external trigger index");
    ಹಿಂತಿರುಗಿ ಸೂಚ್ಯಂಕ + m_n_Sense_ಪದಗಳು * ಶಬ್ದ_ಗಾತ್ರ;
}

std::ಗಣ<uint32_t> TriggerKit::triggerIndices(ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ* senTreep, ತಾರ್ಕಿಕ pre) ಸ್ಥಿರಾಂಕ {
    std::ಗಣ<uint32_t> indices;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), Sen_ಅಂಶ)) {
        uint32_t ಸೂಚ್ಯಂಕ = m_senItem2TrigIdx.at(*sip);
        ಆದರೆ (pre) {
            UASSERT_OBJ(ಸೂಚ್ಯಂಕ < m_n_Pre_ಪದಗಳು * ಶಬ್ದ_ಗಾತ್ರ, sip, "Not a 'pre' trigger");
            ಸೂಚ್ಯಂಕ += m_n_Vec_ಪದಗಳು * ಶಬ್ದ_ಗಾತ್ರ;
        }
        indices.ಸೇರಿಸು(ಸೂಚ್ಯಂಕ);
    }
    ಹಿಂತಿರುಗಿ indices;
}

Ast_ಗಂಟು_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t ಸೂಚ್ಯಂಕ) ಸ್ಥಿರಾಂಕ {
    ಸೂಚ್ಯಂಕ = extra_Trigger_ಸೂಚ್ಯಂಕ(ಸೂಚ್ಯಂಕ);
    ಸ್ಥಿರಾಂಕ uint32_t ಶಬ್ದ_ಸೂಚ್ಯಂಕ = ಸೂಚ್ಯಂಕ / ಶಬ್ದ_ಗಾತ್ರ;
    ಸ್ಥಿರಾಂಕ uint32_t ಬಿಟ್_ಸೂಚ್ಯಂಕ = ಸೂಚ್ಯಂಕ % ಶಬ್ದ_ಗಾತ್ರ;
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
    AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ wordp = ಹೊಸ AstArraySel{flp, refp, static_cast<ಪೂರ್ಣಾಂಕ>(ಶಬ್ದ_ಸೂಚ್ಯಂಕ)};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ trigLhsp = ಹೊಸ AstSel{flp, wordp, static_cast<ಪೂರ್ಣಾಂಕ>(ಬಿಟ್_ಸೂಚ್ಯಂಕ), 1};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ trigRhsp = ಹೊಸ AstVarRef{flp, vscp, VAccess::READ};
    ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, trigLhsp, trigRhsp};
}

ಶೂನ್ಯ TriggerKit::ಕೂಡಿಸು_Extra_Trigger_Assignment(AstVarScope* vscp, uint32_t ಸೂಚ್ಯಂಕ) ಸ್ಥಿರಾಂಕ {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ setp = newExtraTriggerAssignment(vscp, ಸೂಚ್ಯಂಕ);
    setp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, vscp, VAccess::WRITE},
                                ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
ಶೂನ್ಯ TriggerKit::ಕೂಡಿಸು_ಮೌಲ್ಯ_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t ಸೂಚ್ಯಂಕ) ಸ್ಥಿರಾಂಕ {
    ಸೂಚ್ಯಂಕ += m_n_Sense_ಪದಗಳು * ಶಬ್ದ_ಗಾತ್ರ;
    ಸ್ಥಿರಾಂಕ uint32_t ಶಬ್ದ_ಸೂಚ್ಯಂಕ = ಸೂಚ್ಯಂಕ / ಶಬ್ದ_ಗಾತ್ರ;
    ಸ್ಥಿರಾಂಕ uint32_t ಬಿಟ್_ಸೂಚ್ಯಂಕ = ಸೂಚ್ಯಂಕ % ಶಬ್ದ_ಗಾತ್ರ;

    Ast_ತುದಿ_Scope* ಸ್ಥಿರಾಂಕ ತುದಿ_Scopep = netlistp->ತುದಿ_Scopep();
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = netlistp->fileline();
    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ bdtypep = VN_CAST(dtypep, BasicDType)) {
        ಆದರೆ (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            ಹಿಂತಿರುಗಿ;
        }
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, ಗಂಟು_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ rdInst = [flp](AstVarScope* vp) { ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, vp, VAccess::READ}; };
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ wrPrev = [flp](AstVarScope* vp) { ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, vp, VAccess::WRITE}; };
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ rdPrev = [flp](AstVarScope* vp) { ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    ಸ್ಥಿರಾಂಕ std::string prevName = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* ಸ್ಥಿರಾಂಕ prevVscp = ತುದಿ_Scopep->create_ತಾತ್ಕಾಲಿಕ(prevName, instVscp->dtypep());

    // Initialize prev = inst
    ಆದರೆ (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ಸ್ಥಿರಾಂಕ cmhp = ಹೊಸ AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ನಿಯೋಜಿಸು, rdInst(instVscp)};
        cmhp->dtype_ಗಣ_Void();
        initFuncp->ಕೂಡಿಸು_Stmtsp(cmhp->makeStmt());
    } ಇಲ್ಲದಿದ್ದರೆ {
        initFuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, wrPrev(prevVscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* neqp;
    ಆದರೆ (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ಸ್ಥಿರಾಂಕ cmhp
            = ಹೊಸ AstCMethodHard{flp, rdPrev(prevVscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtype_ಗಣ_ಬಿಟ್();
        neqp = cmhp;
    } ಇಲ್ಲದಿದ್ದರೆ {
        neqp = ಹೊಸ AstNeq{flp, rdInst(instVscp), rdPrev(prevVscp)};
    }

    // Build post-update: prev = inst
    Ast_ಗಂಟು* updp;
    ಆದರೆ (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ಸ್ಥಿರಾಂಕ cmhp = ಹೊಸ AstCMethodHard{
            flp, wrPrev(prevVscp), VCMethod::UNPACKED_ನಿಯೋಜಿಸು, rdInst(instVscp)};
        cmhp->dtype_ಗಣ_Void();
        updp = cmhp->makeStmt();
    } ಇಲ್ಲದಿದ್ದರೆ {
        updp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, wrPrev(prevVscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ wordp = ಹೊಸ AstArraySel{flp, refp, static_cast<ಪೂರ್ಣಾಂಕ>(ಶಬ್ದ_ಸೂಚ್ಯಂಕ)};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ trigLhsp = ಹೊಸ AstSel{flp, wordp, static_cast<ಪೂರ್ಣಾಂಕ>(ಬಿಟ್_ಸೂಚ್ಯಂಕ), 1};
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ setp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->ಕೂಡಿಸು_ಮುಂದಿನ(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ಸ್ಥಿರಾಂಕ std::string& name, ತಾರ್ಕಿಕ slow, uint32_t n_Sense_ಪದಗಳು,
                       uint32_t n_Extra_ಪದಗಳು, uint32_t n_Pre_ಪದಗಳು,
                       std::unordered_map<VNRef<ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ>, ಗಾತ್ರ_t> senItem2TrigIdx,
                       ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ*>& senItemps, ತಾರ್ಕಿಕ useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_ಪದಗಳು{n_Sense_ಪದಗಳು}
    , m_n_Extra_ಪದಗಳು{n_Extra_ಪದಗಳು}
    , m_n_Pre_ಪದಗಳು{n_Pre_ಪದಗಳು}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trig_Idx2_Sen_ಅಂಶ{senItemps} {
    // If no triggers, we don't need to generate anything
    ಆದರೆ (!m_n_Vec_ಪದಗಳು) ಹಿಂತಿರುಗಿ;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trig_Idx2_Sen_ಅಂಶ.resize((m_n_Vec_ಪದಗಳು + m_n_Pre_ಪದಗಳು) * ಶಬ್ದ_ಗಾತ್ರ);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_ಪದಗಳು * ಶಬ್ದ_ಗಾತ್ರ,
              m_trig_Idx2_Sen_ಅಂಶ.begin() + m_n_Vec_ಪದಗಳು * ಶಬ್ದ_ಗಾತ್ರ);
    AstNetlist* ಸ್ಥಿರಾಂಕ netlistp = v3Global.rootp();
    AstScope* ಸ್ಥಿರಾಂಕ scopep = netlistp->ತುದಿ_Scopep()->scopep();
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = scopep->fileline();
    // Data type of a single trigger word
    m_ಶಬ್ದ_DTypep = netlistp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(ಶಬ್ದ_ಗಾತ್ರ, ಶಬ್ದ_ಗಾತ್ರ, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ಸ್ಥಿರಾಂಕ rp = ಹೊಸ AstRange{flp, static_cast<ಪೂರ್ಣಾಂಕ>(m_n_Vec_ಪದಗಳು - 1), 0};
    m_trigVecDTypep = ಹೊಸ AstUnpackArrayDType{flp, m_ಶಬ್ದ_DTypep, rp};
    netlistp->typeTablep()->ಕೂಡಿಸು_Typesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    ಆದರೆ (m_n_Pre_ಪದಗಳು) {
        AstRange* ಸ್ಥಿರಾಂಕ ep = ಹೊಸ AstRange{flp, static_cast<ಪೂರ್ಣಾಂಕ>(m_n_Vec_ಪದಗಳು + m_n_Pre_ಪದಗಳು - 1), 0};
        m_trigExtDTypep = ಹೊಸ AstUnpackArrayDType{flp, m_ಶಬ್ದ_DTypep, ep};
        netlistp->typeTablep()->ಕೂಡಿಸು_Typesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } ಇಲ್ಲದಿದ್ದರೆ {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->ತುದಿ_Scopep()->create_ತಾತ್ಕಾಲಿಕ("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->is_ಆಂತರಿಕ(ನಿಜ);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, ನಿಜ);
    m_dumpp->isStatic(ನಿಜ);
    m_dumpp->ifdef("VL_DEBUG");
    ಆದರೆ (useAcc) {
        m_vscAccp
            = netlistp->ತುದಿ_Scopep()->create_ತಾತ್ಕಾಲಿಕ("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->is_ಆಂತರಿಕ(ನಿಜ);
    }
}

Ast_ನಿಯೋಜಿಸು* TriggerKit::createSenTrigVecAssignment(AstVarScope* ಸ್ಥಿರಾಂಕ ಗುರಿ,
                                                  std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& trigps) {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = ಗುರಿ->fileline();
    Ast_ನಿಯೋಜಿಸು* trigStmtsp = ಶೂನ್ಯಸೂಚಕ;
    // Assign sense triggers vector one word at a time
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < trigps.ಗಾತ್ರ_2(); i += ಶಬ್ದ_ಗಾತ್ರ) {
        // Concatenate all bits in this trigger word using a balanced
        ಗಾಗಿ (uint32_t level = 0; level < ಶಬ್ದ_ಗಾತ್ರ_LOG2; ++level) {
            ಸ್ಥಿರಾಂಕ uint32_t stride = 1 << level;
            ಗಾಗಿ (uint32_t j = 0; j < ಶಬ್ದ_ಗಾತ್ರ; j += 2 * stride) {
                trigps[i + j] = ಹೊಸ AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = ಶೂನ್ಯಸೂಚಕ;
            }
        }

        // Set the whole word in the trigger vector
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಶಬ್ದ_ಸೂಚ್ಯಂಕ = static_cast<ಪೂರ್ಣಾಂಕ>(i / ಶಬ್ದ_ಗಾತ್ರ);
        AstArraySel* ಸ್ಥಿರಾಂಕ aselp
            = ಹೊಸ AstArraySel{flp, ಹೊಸ AstVarRef{flp, ಗುರಿ, VAccess::WRITE}, ಶಬ್ದ_ಸೂಚ್ಯಂಕ};
        trigStmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(trigStmtsp, ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, aselp, trigps[i]});
    }
    ಹಿಂತಿರುಗಿ trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ಸ್ಥಿರಾಂಕ initFuncp,  //
                              Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& sen_ಅಭಿವ್ಯಕ್ತಿ_Builder,  //
                              ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ*>& preTreeps,  //
                              ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ*>& senTreeps,  //
                              ಸ್ಥಿರಾಂಕ string& name,  //
                              ಸ್ಥಿರಾಂಕ ExtraTriggers& extraTriggers,  //
                              ತಾರ್ಕಿಕ slow,  //
                              ತಾರ್ಕಿಕ useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ>, ಗಾತ್ರ_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep : preTreeps) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_ಅಂಶ);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = senItem2TrigIdx.emplace(*itemp, senItemps.ಗಾತ್ರ_2());
            ಆದರೆ (pair.ಎರಡನೇ) senItemps.ತಳ್ಳು_ಹಿಂಭಾಗ(itemp);
        }
    }
    ಸ್ಥಿರಾಂಕ uint32_t n_Pre_Sen_ವಸ್ತುಗಳು = senItemps.ಗಾತ್ರ_2();
    V3Stats::ಕೂಡಿಸು_Stat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_ವಸ್ತುಗಳು);
    // Number of pre triggers, rounded up to a full word.
    ಸ್ಥಿರಾಂಕ uint32_t nPreTriggers = vlstd::ಸುತ್ತು_ಮೇಲೆ_ಗಮ್ಯ_ಅಪವರ್ತ್ಯ_ದ<ಶಬ್ದ_ಗಾತ್ರ>(senItemps.ಗಾತ್ರ_2());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ಸ್ಥಿರಾಂಕ uint32_t n_Pre_ಪದಗಳು = nPreTriggers / ಶಬ್ದ_ಗಾತ್ರ;

    // Process the rest of the trees
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep : senTreeps) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Sen_ಅಂಶ);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = senItem2TrigIdx.emplace(*itemp, senItemps.ಗಾತ್ರ_2());
            ಆದರೆ (pair.ಎರಡನೇ) senItemps.ತಳ್ಳು_ಹಿಂಭಾಗ(itemp);
        }
    }
    ಸ್ಥಿರಾಂಕ uint32_t n_Sen_ವಸ್ತುಗಳು = senItemps.ಗಾತ್ರ_2() - nPreTriggers;
    V3Stats::ಕೂಡಿಸು_Stat("Scheduling, '" + name + "' sense triggers", n_Sen_ವಸ್ತುಗಳು + n_Pre_Sen_ವಸ್ತುಗಳು);
    // Number of sense triggers, rounded up to a full word
    ಸ್ಥಿರಾಂಕ uint32_t nSenseTriggers = vlstd::ಸುತ್ತು_ಮೇಲೆ_ಗಮ್ಯ_ಅಪವರ್ತ್ಯ_ದ<ಶಬ್ದ_ಗಾತ್ರ>(senItemps.ಗಾತ್ರ_2());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ಸ್ಥಿರಾಂಕ uint32_t n_Sense_ಪದಗಳು = nSenseTriggers / ಶಬ್ದ_ಗಾತ್ರ;

    // Allocate space for the extra triggers
    V3Stats::ಕೂಡಿಸು_Stat("Scheduling, '" + name + "' extra triggers", extraTriggers.ಗಾತ್ರ_2());
    // Number of extra triggers, rounded up to a full word.
    ಸ್ಥಿರಾಂಕ uint32_t nExtraTriggers = vlstd::ಸುತ್ತು_ಮೇಲೆ_ಗಮ್ಯ_ಅಪವರ್ತ್ಯ_ದ<ಶಬ್ದ_ಗಾತ್ರ>(extraTriggers.ಗಾತ್ರ_2());
    ಸ್ಥಿರಾಂಕ uint32_t n_Extra_ಪದಗಳು = nExtraTriggers / ಶಬ್ದ_ಗಾತ್ರ;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_ಪದಗಳು, n_Extra_ಪದಗಳು, n_Pre_ಪದಗಳು, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    ಆದರೆ (!kit.m_n_Vec_ಪದಗಳು) ಹಿಂತಿರುಗಿ kit;

    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = netlistp->ತುದಿ_Scopep()->fileline();

    // Creates read/write reference
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ rd = [flp](AstVarScope* vp) { ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, vp, VAccess::READ}; };
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ wr = [flp](AstVarScope* vp) { ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* ಸ್ಥಿರಾಂಕ dumpTrgp
        = new_ವಾದ(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* ಸ್ಥಿರಾಂಕ dumpTagp
        = new_ವಾದ(kit.m_dumpp, netlistp->ಕಂಡುಹಿಡಿ_String_DType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, ಹೊಸ AstLogNot{flp, kit.new_Any_ಗಣ_Call(dumpTrgp)}};
        kit.m_dumpp->ಕೂಡಿಸು_Stmtsp(ifp);
        AstCStmt* ಸ್ಥಿರಾಂಕ cstmtp = ಹೊಸ AstCStmt{flp};
        ifp->ಕೂಡಿಸು_Thensp(cstmtp);
        cstmtp->ಕೂಡಿಸು("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->ಕೂಡಿಸು(rd(dumpTagp));
        cstmtp->ಕೂಡಿಸು(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_Debug = [&](uint32_t ಸೂಚ್ಯಂಕ, ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ wrd_ಸೂಚ್ಯಂಕ = static_cast<ಪೂರ್ಣಾಂಕ>(ಸೂಚ್ಯಂಕ / ಶಬ್ದ_ಗಾತ್ರ);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಬಿಟ್_ಸೂಚ್ಯಂಕ = static_cast<ಪೂರ್ಣಾಂಕ>(ಸೂಚ್ಯಂಕ % ಶಬ್ದ_ಗಾತ್ರ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ aselp = ಹೊಸ AstArraySel{flp, rd(dumpTrgp), wrd_ಸೂಚ್ಯಂಕ};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = ಹೊಸ AstSel{flp, aselp, ಬಿಟ್_ಸೂಚ್ಯಂಕ, 1};
        AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, condp};
        kit.m_dumpp->ಕೂಡಿಸು_Stmtsp(ifp);
        AstCStmt* ಸ್ಥಿರಾಂಕ cstmtp = ಹೊಸ AstCStmt{flp};
        ifp->ಕೂಡಿಸು_Thensp(cstmtp);
        cstmtp->ಕೂಡಿಸು("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->ಕೂಡಿಸು(rd(dumpTagp));
        cstmtp->ಕೂಡಿಸು(" + \"' region trigger index " + std::ಗಮ್ಯ_string(ಸೂಚ್ಯಂಕ) + " is active: " + ಪಠ್ಯ
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_ಗಂಟು_Stmt* initialTrigsp = ಶೂನ್ಯಸೂಚಕ;
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < senItemps.ಗಾತ್ರ_2(); ++i) {
        ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        ಆದರೆ (!senItemp) {
            trigps.emplace_ಹಿಂಭಾಗ(ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}});
            ಮುಂದುವರಿ;
        }

        // Create the trigger computation expression
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair = sen_ಅಭಿವ್ಯಕ್ತಿ_Builder.build(senItemp);
        trigps.emplace_ಹಿಂಭಾಗ(pair.ಮೊದಲ);

        // Add initialization time trigger
        ಆದರೆ (pair.ಎರಡನೇ || v3Global.opt.x_Initial_ಅಂಚು()) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ wrd_ಸೂಚ್ಯಂಕ = static_cast<ಪೂರ್ಣಾಂಕ>(i / ಶಬ್ದ_ಗಾತ್ರ);
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಬಿಟ್_ಸೂಚ್ಯಂಕ = static_cast<ಪೂರ್ಣಾಂಕ>(i % ಶಬ್ದ_ಗಾತ್ರ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ wordp = ಹೊಸ AstArraySel{flp, wr(kit.m_vscp), wrd_ಸೂಚ್ಯಂಕ};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstSel{flp, wordp, ಬಿಟ್_ಸೂಚ್ಯಂಕ, 1};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}};
            ಆದರೆ (useAcc) {
                initFuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsp, rhsp});
            } ಇಲ್ಲದಿದ್ದರೆ {
                initialTrigsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(initialTrigsp, ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsp, rhsp});
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
        ಕೂಡಿಸು_Debug(i, desc);
    }
    UASSERT(trigps.ಗಾತ್ರ_2() == nSenseTriggers, "Inconsistent number of trigger expressions");

    Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ i = 0; i < extraTriggers.ಗಾತ್ರ_2(); ++i) {
        ಕೂಡಿಸು_Debug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    ಸ್ಥಿರಾಂಕ Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder::Results senResults = sen_ಅಭಿವ್ಯಕ್ತಿ_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    ಗಾಗಿ (Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ nodep : senResults.m_inits) initFuncp->ಕೂಡಿಸು_Stmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_ತುದಿ_Scope* ಸ್ಥಿರಾಂಕ ತುದಿ_Scopep = netlistp->ತುದಿ_Scopep();
    {
        AstCFunc* ಸ್ಥಿರಾಂಕ fp = kit.m_compVecp;
        // Trigger computation
        ಗಾಗಿ (Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ nodep : senResults.m_preUpdates) fp->ಕೂಡಿಸು_Stmtsp(nodep);
        fp->ಕೂಡಿಸು_Stmtsp(trigStmtsp);
        ಗಾಗಿ (Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ nodep : senResults.m_postUpdates) fp->ಕೂಡಿಸು_Stmtsp(nodep);
        // Add the initialization time triggers
        ಆದರೆ (initialTrigsp) {
            AstVarScope* ಸ್ಥಿರಾಂಕ initVscp = ತುದಿ_Scopep->create_ತಾತ್ಕಾಲಿಕ("__V" + name + "DidInit", 1);
            AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, ಹೊಸ AstNot{flp, rd(initVscp)}};
            fp->ಕೂಡಿಸು_Stmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->ಕೂಡಿಸು_Thensp(util::ಗಣ_Var(initVscp, 1));
            ifp->ಕೂಡಿಸು_Thensp(initialTrigsp);
        }
        util::ವಿಭಜಿಸು_Check(fp);
    };
    // If there are 'pre' triggers, compute them
    ಆದರೆ (kit.m_n_Pre_ಪದಗಳು) {
        AstCFunc* ಸ್ಥಿರಾಂಕ fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ಸ್ಥಿರಾಂಕ latchedp
            = new_ವಾದ(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ಸ್ಥಿರಾಂಕ nVscp = ತುದಿ_Scopep->create_ತಾತ್ಕಾಲಿಕ("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(ನಿಜ);
        // Add a loop to compute the pre words
        AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
        fp->ಕೂಡಿಸು_Stmtsp(util::ಗಣ_Var(nVscp, 0));
        fp->ಕೂಡಿಸು_Stmtsp(loopp);
        // Loop body
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ offsetp = ಹೊಸ AstConst{flp, kit.m_n_Vec_ಪದಗಳು};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lIdxp = ಹೊಸ Ast_ಕೂಡಿಸು{flp, rd(nVscp), offsetp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ aWordp = ಹೊಸ AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bWordp = ಹೊಸ AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = ಹೊಸ AstAnd{flp, aWordp, ಹೊಸ AstNot{flp, bWordp}};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ limp = ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, n_Pre_ಪದಗಳು};
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsp, rhsp});
        loopp->ಕೂಡಿಸು_Stmtsp(util::incrementVar(nVscp));
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{flp, loopp, ಹೊಸ AstLt{flp, rd(nVscp), limp}});
        util::ವಿಭಜಿಸು_Check(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    ಆದರೆ (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()->delete_ವೃಕ್ಷ();
    // Done with the trigger dump function, split as might be large
    util::ವಿಭಜಿಸು_Check(kit.m_dumpp);

    ಹಿಂತಿರುಗಿ kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
ವರ್ಗ AwaitBeforeTrigVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;

    /**
     * AstCAwait::user1()       ->  bool.           True if node has been visited
     * AstSenTree::user1p()     ->  AstCFunc*.      Function that has to be called before awaiting
     *                                              for CAwait pointing to this SenTree
     * AstCFunc::user1p()       ->  AstVarScope*    Function's local temporary extended trigger
     *                                              vector variable scope
     */

    // Netlist - needed for using util::makeSubFunction()
    AstNetlist* ಸ್ಥಿರಾಂಕ m_netlistp;
    // Trigger kit - for accessing trigger vectors and mapping senItems to thier indexes
    ಸ್ಥಿರಾಂಕ TriggerKit& m_trigKit;
    // Expression builder - for building expressions from SenItems
    Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builder;
    // Generator of unique names for before-trigger function
    V3_Unique_ಹೆಸರುಗಳು m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::ಸದಿಶ<std::pair<AstCFunc*, Ast_Sen_ವೃಕ್ಷ*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::ಸದಿಶ<std::pair<Ast_Sen_ವೃಕ್ಷ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> m_sen_ವೃಕ್ಷ_ಗಮ್ಯ_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_ಗಂಟು>, std::ಸದಿಶ<Ast_Sen_ಅಂಶ*>> m_sen_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_Sen_ಅಂಶ;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    ಸ್ಥಿರ Ast_ಗಂಟು* get_Sen_Hash_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ nodep) {
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ varRefp = VN_CAST(nodep->sensp(), VarRef)) ಹಿಂತಿರುಗಿ varRefp;
        ಹಿಂತಿರುಗಿ nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    ಶೂನ್ಯ fill_Sen_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_Sen_ಅಂಶ() {
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ sen_ವೃಕ್ಷ_Sched : m_sen_ವೃಕ್ಷ_ಗಮ್ಯ_Sched) {
            Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = sen_ವೃಕ್ಷ_Sched.ಮೊದಲ;

            ಗಾಗಿ (Ast_Sen_ಅಂಶ* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_ಅಂಶ)) {
                ಸ್ಥಿರಾಂಕ VEdgeType ಅಂಚು = senItemp->ಅಂಚು_Type();
                ಆದರೆ (ಅಂಚು.an_ಅಂಚು() || ಅಂಚು == VEdgeType::ET_ಬದಲಾಯಿತು
                    || ಅಂಚು == VEdgeType::ET_HYBRID) {
                    m_sen_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_Sen_ಅಂಶ[*get_Sen_Hash_ಗಂಟು(senItemp)].ತಳ್ಳು_ಹಿಂಭಾಗ(senItemp);
                }
            }
        }

        std::ಸದಿಶ<VNRef<Ast_ಗಂಟು>> ಗಮ್ಯ_Remove;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& sen_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_Sen_ವೃಕ್ಷ : m_sen_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_Sen_ಅಂಶ) {
            std::ಸದಿಶ<Ast_Sen_ಅಂಶ*> senItemps = sen_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_Sen_ವೃಕ್ಷ.ಎರಡನೇ;
            ಗಮ್ಯ_Remove.ತಳ್ಳು_ಹಿಂಭಾಗ(sen_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_Sen_ವೃಕ್ಷ.ಮೊದಲ);
            ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < senItemps.ಗಾತ್ರ_2(); ++i) {
                ಆದರೆ (senItemps[i]->ಅಂಚು_Type() != senItemps[i - 1]->ಅಂಚು_Type()) {
                    ಗಮ್ಯ_Remove.ತೆಗೆ_ಹಿಂಭಾಗ();
                    ನಿಲ್ಲಿಸು;
                }
            }
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ VNRef<Ast_ಗಂಟು> it : ಗಮ್ಯ_Remove) m_sen_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_Sen_ಅಂಶ.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<ಗಾತ್ರ_t, std::map<ಗಾತ್ರ_t, std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>>>
    get_ಬಳಸಿದ_Triggers_ಗಮ್ಯ_Trees(ಸ್ಥಿರಾಂಕ std::ಗಣ<ಗಾತ್ರ_t>& ಬಳಸಿದ_Triggers) {
        std::map<ಗಾತ್ರ_t, std::map<ಗಾತ್ರ_t, std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>>> ಬಳಸಿದ_Trigs_ಗಮ್ಯ_Using_Trees;
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ sen_ವೃಕ್ಷ_Sched : m_sen_ವೃಕ್ಷ_ಗಮ್ಯ_Sched) {
            ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = sen_ವೃಕ್ಷ_Sched.ಮೊದಲ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ shedp = sen_ವೃಕ್ಷ_Sched.ಎರಡನೇ;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            ಗಾಗಿ (Ast_Sen_ಅಂಶ* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), Sen_ಅಂಶ)) {
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                ಆದರೆ (ಬಳಸಿದ_Triggers.ಕಂಡುಹಿಡಿ(idx) != ಬಳಸಿದ_Triggers.ಕೊನೆ()) {
                    ಬಳಸಿದ_Trigs_ಗಮ್ಯ_Using_Trees[idx / TriggerKit::ಶಬ್ದ_ಗಾತ್ರ]
                                         [ಗಾತ್ರ_t{1} << (idx % TriggerKit::ಶಬ್ದ_ಗಾತ್ರ)]
                                             .ತಳ್ಳು_ಹಿಂಭಾಗ(shedp);
                }
            }
        }
        ಆದರೆ (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& triggers_ಗಮ್ಯ_Trees : ಬಳಸಿದ_Trigs_ಗಮ್ಯ_Using_Trees) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Trees : triggers_ಗಮ್ಯ_Trees.ಎರಡನೇ) {
                    ಸ್ಥಿರಾಂಕ std::ಗಣ<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> exprps{ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Trees.ಎರಡನೇ.begin(),
                                                              ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Trees.ಎರಡನೇ.ಕೊನೆ()};
                    UASSERT(ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Trees.ಎರಡನೇ.ಗಾತ್ರ_2() == exprps.ಗಾತ್ರ_2(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        ಹಿಂತಿರುಗಿ ಬಳಸಿದ_Trigs_ಗಮ್ಯ_Using_Trees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = senTreep->fileline();
        ಆದರೆ (!senTreep->user1p()) {
            AstCFunc* ಸ್ಥಿರಾಂಕ funcp = util::makeSubFunction(
                m_netlistp, m_beforeTriggerFuncUniqueName.get(senTreep), ಸುಳ್ಳು);
            senTreep->user1p(funcp);

            // Create a local temporary extended vector
            AstVarScope* ಸ್ಥಿರಾಂಕ tmpp = newLocal(funcp, m_trigKit.vscAccp()->dtypep(), "__VTmp");
            funcp->user1p(tmpp);
            // This function can be called multiple times, and accesses model state, which
            // violates the assumption made in V3Life that there is no such function.
            funcp->noLife(ನಿಜ);

            AstVar* ಸ್ಥಿರಾಂಕ argp = ಹೊಸ AstVar{flp, VVarType::BLOCKTEMP, "__VeventDescription",
                                            senTreep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::CHARPTR)};
            argp->funcLocal(ನಿಜ);
            argp->direction(VDirection::ಒಳಹರಿವು);
            funcp->ಕೂಡಿಸು_Argsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_ಹಿಂಭಾಗ(funcp, senTreep);
        }
        AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtype_ಗಣ_Void();
        ಹಿಂತಿರುಗಿ callp;
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCAwait* ಸ್ಥಿರಾಂಕ nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCMethodHard* ಸ್ಥಿರಾಂಕ c_ವಿಧಾನ_Hardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            ಆದರೆ (c_ವಿಧಾನ_Hardp->ವಿಧಾನ() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ಸ್ಥಿರಾಂಕ beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ pinp = c_ವಿಧಾನ_Hardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, c_ವಿಧಾನ_Hardp, "No event description");
                beforeTrigp->ಕೂಡಿಸು_Argsp(VN_AS(pinp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));

                // Call the before-trigger function before the CAwait
                nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(beforeTrigp->makeStmt());
                m_sen_ವೃಕ್ಷ_ಗಮ್ಯ_Sched.emplace_ಹಿಂಭಾಗ(nodep->sentreep(), c_ವಿಧಾನ_Hardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& sen_ಅಭಿವ್ಯಕ್ತಿ_Builder,
                           ಸ್ಥಿರಾಂಕ TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builder{sen_ಅಭಿವ್ಯಕ್ತಿ_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_Sen_ಅಂಶ();

        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> trigps;
        std::ಗಣ<ಗಾತ್ರ_t> ಬಳಸಿದ_Triggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& func_ಗಮ್ಯ_ಬಳಸಿದ_Triggers : m_generatedFuncs) {
            AstCFunc* ಸ್ಥಿರಾಂಕ funcp = func_ಗಮ್ಯ_ಬಳಸಿದ_Triggers.ಮೊದಲ;
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = VN_AS(funcp->user1p(), VarScope);
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = func_ಗಮ್ಯ_ಬಳಸಿದ_Triggers.ಎರಡನೇ;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ emplaceAt
                    = [flp, &trigps, &ಬಳಸಿದ_Triggers](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp, ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t pos) {
                          ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಗುರಿ_ಗಾತ್ರ
                              = vlstd::ಸುತ್ತು_ಮೇಲೆ_ಗಮ್ಯ_ಅಪವರ್ತ್ಯ_ದ<TriggerKit::ಶಬ್ದ_ಗಾತ್ರ>(pos + 1);
                          ಆದರೆ (trigps.ಸಾಮರ್ಥ್ಯ() < ಗುರಿ_ಗಾತ್ರ) trigps.reserve(ಗುರಿ_ಗಾತ್ರ * 2);
                          ಆಗ (trigps.ಗಾತ್ರ_2() < ಗುರಿ_ಗಾತ್ರ) {
                              trigps.ತಳ್ಳು_ಹಿಂಭಾಗ(ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}});
                          }
                          trigps[pos]->delete_ವೃಕ್ಷ();
                          trigps[pos] = exprp;
                          ಬಳಸಿದ_Triggers.ಸೇರಿಸು(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Sen_ಅಂಶ)) {
                    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builder.build(itemp).ಮೊದಲ, idx);
                    ಸ್ವಯಂಚಾಲಿತ iter = m_sen_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_Sen_ಅಂಶ.ಕಂಡುಹಿಡಿ(*get_Sen_Hash_ಗಂಟು(itemp));
                    ಆದರೆ (iter != m_sen_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_Sen_ಅಂಶ.ಕೊನೆ()) {
                        ಗಾಗಿ (Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ additionalItemp : iter->ಎರಡನೇ) {
                            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builder.build(additionalItemp).ಮೊದಲ, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ಸ್ಥಿರಾಂಕ Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder::Results results
                    = m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builder.getResultsAndClearUpdates();
                ಗಾಗಿ (Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtsp : results.m_inits) funcp->ಕೂಡಿಸು_Stmtsp(stmtsp);
                ಗಾಗಿ (Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtsp : results.m_preUpdates) funcp->ಕೂಡಿಸು_Stmtsp(stmtsp);
                funcp->ಕೂಡಿಸು_Stmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                ಗಾಗಿ (Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtsp : results.m_postUpdates) funcp->ಕೂಡಿಸು_Stmtsp(stmtsp);
            }

            ಸ್ಥಿರಾಂಕ std::map<ಗಾತ್ರ_t, std::map<ಗಾತ್ರ_t, std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>>>
                ಬಳಸಿದ_Trigs_ಗಮ್ಯ_Using_Trees = get_ಬಳಸಿದ_Triggers_ಗಮ್ಯ_Trees(ಬಳಸಿದ_Triggers);
            ಬಳಸಿದ_Triggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ getIdx = [flp](AstVarScope* ಸ್ಥಿರಾಂಕ scocep, VAccess access, ಗಾತ್ರ_t idx) {
                ಹಿಂತಿರುಗಿ ಹೊಸ AstArraySel{flp, ಹೊಸ AstVarRef{flp, scocep, access},
                                       ಹೊಸ AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* ಸ್ಥಿರಾಂಕ argpVscp = ಹೊಸ AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->ಕೂಡಿಸು_Varsp(argpVscp);

            // Mark as ready triggered schedulers
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& triggers_ಗಮ್ಯ_Trees : ಬಳಸಿದ_Trigs_ಗಮ್ಯ_Using_Trees) {
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಶಬ್ದ = triggers_ಗಮ್ಯ_Trees.ಮೊದಲ;

                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Trees : triggers_ಗಮ್ಯ_Trees.ಎರಡನೇ) {
                    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಬಿಟ್_2 = ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Trees.ಮೊದಲ;
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& schedulers = ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Trees.ಎರಡನೇ;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* ಸ್ಥಿರಾಂಕ ಮುಖವಾಡ_Constp = ಹೊಸ AstConst{flp, AstConst::Unsized64{}, ಬಿಟ್_2};
                    AstAnd* ಸ್ಥಿರಾಂಕ condp
                        = ಹೊಸ AstAnd{flp, getIdx(vscp, VAccess::READ, ಶಬ್ದ), ಮುಖವಾಡ_Constp};
                    AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ schedp : schedulers) {
                        AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCMethodHard{
                            flp, schedp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), VCMethod::SCHED_READY};
                        callp->dtype_ಗಣ_Void();
                        callp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->ಕೂಡಿಸು_Thensp(callp->makeStmt());
                    }
                    funcp->ಕೂಡಿಸು_Stmtsp(ifp);
                }
            }

            AstVarScope* ಸ್ಥಿರಾಂಕ vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& triggers_ಗಮ್ಯ_Trees : ಬಳಸಿದ_Trigs_ಗಮ್ಯ_Using_Trees) {
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಶಬ್ದ = triggers_ಗಮ್ಯ_Trees.ಮೊದಲ;
                funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, getIdx(vscAccp, VAccess::WRITE, ಶಬ್ದ),
                                               ಹೊಸ AstOr{flp, getIdx(vscAccp, VAccess::READ, ಶಬ್ದ),
                                                         getIdx(vscp, VAccess::READ, ಶಬ್ದ)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

ಶೂನ್ಯ beforeTrigVisitor(AstNetlist* netlistp, Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& sen_ಅಭಿವ್ಯಕ್ತಿ_Builder,
                       ಸ್ಥಿರಾಂಕ TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_ಅಭಿವ್ಯಕ್ತಿ_Builder, trigKit};
}

}  // namespace V3Sched
