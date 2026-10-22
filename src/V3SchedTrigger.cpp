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

उपयुज्य util::new_तर्कः;

AstVarScope* newLocal(AstCFunc* funcp, Ast_ग्रन्थिः_DType* dtypep, ध्रुव std::string& name) {
    File_पङ्क्तिः* ध्रुव flp = funcp->fileline();
    AstScope* ध्रुव scopep = funcp->scopep();
    AstVar* ध्रुव varp = नूतन AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
    varp->funcLocal(सत्य);
    varp->noReset(सत्य);
    funcp->सङ्कलय_Varsp(varp);
    AstVarScope* ध्रुव vscp = नूतन AstVarScope{flp, scopep, varp};
    scopep->सङ्कलय_Varsp(vscp);
    प्रत्यागम vscp;
}

}  // namespace

AstCFunc* TriggerKit::createDumpExtFunc() ध्रुव {
    UASSERT(m_n_Pre_शब्दाः, "Just call the regular dumping function if there are no pre triggers");

    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    File_पङ्क्तिः* ध्रुव flp = netlistp->शिखरम्_Scopep()->fileline();
    Ast_ग्रन्थिः_DType* ध्रुव u32DTypep = netlistp->findUInt32DType();
    Ast_ग्रन्थिः_DType* ध्रुव strDtypep = netlistp->findStringDType();

    // Dumping function always slow
    ध्रुव std::string name = "_dump_triggers__" + m_name + "_ext";
    AstCFunc* ध्रुव funcp = util::makeSubFunction(netlistp, name, सत्य);
    funcp->isStatic(सत्य);
    funcp->ifdef("VL_DEBUG");

    // Add argument
    AstVarScope* ध्रुव eVscp = new_तर्कः(funcp, m_trigExtDTypep, "ext", VDirection::CONSTREF);
    AstVarScope* ध्रुव tVscp = new_तर्कः(funcp, strDtypep, "tag", VDirection::CONSTREF);

    // Creates read/write reference
    ध्रुव स्वचालित rd = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::READ}; };
    ध्रुव स्वचालित wr = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

    // This is a slow function, only for dumping, so we can just copy to locals

    // Copy the vec part, dump it
    {
        AstVarScope* ध्रुव vVscp = newLocal(funcp, m_trigVecDTypep, "vec");
        AstVarScope* ध्रुव iVscp = newLocal(funcp, u32DTypep, "i");
        funcp->सङ्कलय_Stmtsp(util::समुच्चयः_Var(iVscp, 0));
        // Add loop
        AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
        funcp->सङ्कलय_Stmtsp(loopp);
        // Loop body
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstArraySel{flp, wr(vVscp), rd(iVscp)};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstArraySel{flp, rd(eVscp), rd(iVscp)};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, m_n_Vec_शब्दाः};
        loopp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, lhsp, rhsp});
        loopp->सङ्कलय_Stmtsp(util::incrementVar(iVscp));
        loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(iVscp), limp}});
        // Use the vec dumping function
        AstCCall* ध्रुव callp = नूतन AstCCall{flp, m_dumpp};
        callp->dtype_समुच्चयः_Void();
        callp->सङ्कलय_Argsp(rd(vVscp));
        callp->सङ्कलय_Argsp(rd(tVscp));
        funcp->सङ्कलय_Stmtsp(callp->makeStmt());
    }

    // Copy the pre part, zero top bits, dump it
    {
        AstVarScope* ध्रुव pVscp = newLocal(funcp, m_trigVecDTypep, "pre");
        AstVarScope* ध्रुव jVscp = newLocal(funcp, u32DTypep, "j");
        funcp->सङ्कलय_Stmtsp(util::समुच्चयः_Var(jVscp, 0));
        // Copy pre words
        {
            // Add loop
            AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
            funcp->सङ्कलय_Stmtsp(loopp);
            // Loop body
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstArraySel{flp, rd(eVscp), rd(jVscp)};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, m_n_Pre_शब्दाः};
            loopp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, lhsp, rhsp});
            loopp->सङ्कलय_Stmtsp(util::incrementVar(jVscp));
            loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(jVscp), limp}});
        }
        // Zero the rest
        {
            // Add loop - copy
            AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
            funcp->सङ्कलय_Stmtsp(loopp);
            // Loop body
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstArraySel{flp, wr(pVscp), rd(jVscp)};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, m_n_Vec_शब्दाः};
            loopp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, lhsp, rhsp});
            loopp->सङ्कलय_Stmtsp(util::incrementVar(jVscp));
            loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(jVscp), limp}});
        }
        // Use the vec dumping function
        AstCCall* ध्रुव callp = नूतन AstCCall{flp, m_dumpp};
        callp->dtype_समुच्चयः_Void();
        callp->सङ्कलय_Argsp(rd(pVscp));
        callp->सङ्कलय_Argsp(
            नूतन AstConcatN{flp, rd(tVscp), नूतन AstConst{flp, AstConst::String{}, " pre"}});
        funcp->सङ्कलय_Stmtsp(callp->makeStmt());
    }

    प्रत्यागम funcp;
}

AstCFunc* TriggerKit::create_Any_समुच्चयः_Func(AstUnpackArrayDType* ध्रुव dtypep) ध्रुव {
    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    File_पङ्क्तिः* ध्रुव flp = netlistp->शिखरम्_Scopep()->fileline();
    Ast_ग्रन्थिः_DType* ध्रुव u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_anySet__" + m_name;
    name += dtypep == m_trigVecDTypep ? "" : "_ext";
    AstCFunc* ध्रुव funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(सत्य);
    funcp->rtnType("bool");

    // Add argument
    AstVarScope* ध्रुव iVscp = new_तर्कः(funcp, dtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ध्रुव nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read reference
    ध्रुव स्वचालित rd = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::READ}; };

    // Function body
    AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
    funcp->सङ्कलय_Stmtsp(util::समुच्चयः_Var(nVscp, 0));
    funcp->सङ्कलय_Stmtsp(loopp);
    funcp->सङ्कलय_Stmtsp(नूतन AstCReturn{flp, नूतन AstConst{flp, AstConst::BitFalse{}}});

    // Loop body
    ध्रुव uint32_t n_शब्दाः = dtypep->elementsConst();
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = नूतन AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ग्रन्थिः_Stmt* ध्रुव thenp = नूतन AstCReturn{flp, नूतन AstConst{flp, AstConst::BitTrue{}}};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, n_शब्दाः};
    loopp->सङ्कलय_Stmtsp(नूतन AstIf{flp, condp, thenp});
    loopp->सङ्कलय_Stmtsp(util::incrementVar(nVscp));
    loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(nVscp), limp}});

    //समाप्तम्
    प्रत्यागम funcp;
}
AstCFunc* TriggerKit::createClearFunc() ध्रुव {
    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    File_पङ्क्तिः* ध्रुव flp = netlistp->शिखरम्_Scopep()->fileline();
    Ast_ग्रन्थिः_DType* ध्रुव u32DTypep = netlistp->findUInt32DType();

    // Create function
    AstCFunc* ध्रुव funcp = util::makeSubFunction(netlistp, "_trigger_clear__" + m_name, m_slow);
    funcp->isStatic(सत्य);

    // Add arguments
    AstVarScope* ध्रुव oVscp = new_तर्कः(funcp, m_trigVecDTypep, "out", VDirection::OUTPUT);

    // Add loop counter variable
    AstVarScope* ध्रुव nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ध्रुव स्वचालित rd = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::READ}; };
    ध्रुव स्वचालित wr = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
    funcp->सङ्कलय_Stmtsp(util::समुच्चयः_Var(nVscp, 0));
    funcp->सङ्कलय_Stmtsp(loopp);

    // Loop body
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstConst{flp, AstConst::DTyped{}, m_wordDTypep};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, m_n_Vec_शब्दाः};
    loopp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, lhsp, rhsp});
    loopp->सङ्कलय_Stmtsp(util::incrementVar(nVscp));
    loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(nVscp), limp}});

    //समाप्तम्
    प्रत्यागम funcp;
}
AstCFunc* TriggerKit::createOrIntoFunc(AstUnpackArrayDType* ध्रुव oDtypep,
                                       AstUnpackArrayDType* ध्रुव iDtypep) ध्रुव {
    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    File_पङ्क्तिः* ध्रुव flp = netlistp->शिखरम्_Scopep()->fileline();
    Ast_ग्रन्थिः_DType* ध्रुव u32DTypep = netlistp->findUInt32DType();

    // Create function
    std::string name = "_trigger_orInto__" + m_name;
    name += iDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    name += oDtypep == m_trigVecDTypep ? "_vec" : "_ext";
    AstCFunc* ध्रुव funcp = util::makeSubFunction(netlistp, name, m_slow);
    funcp->isStatic(सत्य);

    // Add arguments
    AstVarScope* ध्रुव oVscp = new_तर्कः(funcp, oDtypep, "out", VDirection::INOUT);
    AstVarScope* ध्रुव iVscp = new_तर्कः(funcp, iDtypep, "in", VDirection::CONSTREF);

    // Add loop counter variable
    AstVarScope* ध्रुव nVscp = newLocal(funcp, u32DTypep, "n");

    // Creates read/write reference
    ध्रुव स्वचालित rd = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::READ}; };
    ध्रुव स्वचालित wr = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

    // Function body
    AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
    funcp->सङ्कलय_Stmtsp(util::समुच्चयः_Var(nVscp, 0));
    funcp->सङ्कलय_Stmtsp(loopp);

    // Loop body
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstArraySel{flp, wr(oVscp), rd(nVscp)};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव oWordp = नूतन AstArraySel{flp, rd(oVscp), rd(nVscp)};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव iWordp = नूतन AstArraySel{flp, rd(iVscp), rd(nVscp)};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstOr{flp, oWordp, iWordp};
    AstConst* ध्रुव outputRangeLeftp = VN_AS(oDtypep->rangep()->leftp(), Const);
    AstConst* ध्रुव निवेशः_Range_Leftp = VN_AS(iDtypep->rangep()->leftp(), Const);
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव limp = outputRangeLeftp->अङ्कः().गन्तव्यम्_SInt() < निवेशः_Range_Leftp->अङ्कः().गन्तव्यम्_SInt()
                                  ? outputRangeLeftp->clone_वृक्षः_विशुद्धम्(असत्य)
                                  : निवेशः_Range_Leftp->clone_वृक्षः_विशुद्धम्(असत्य);
    loopp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, lhsp, rhsp});
    loopp->सङ्कलय_Stmtsp(util::incrementVar(nVscp));
    loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLte{flp, rd(nVscp), limp}});

    //समाप्तम्
    प्रत्यागम funcp;
}

Ast_ग्रन्थिः_व्यञ्जकम्* TriggerKit::new_Any_समुच्चयः_Call(AstVarScope* ध्रुव vscp) ध्रुव {
    File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->शिखरम्_Scopep()->fileline();
    यदि (!m_n_Vec_शब्दाः) प्रत्यागम नूतन AstConst{flp, AstConst::BitFalse{}};

    AstCFunc* funcp = शून्यसूचकः;
    यदि (vscp->dtypep() == m_trigVecDTypep) {
        यदि (!m_any_समुच्चयः_Vecp) m_any_समुच्चयः_Vecp = create_Any_समुच्चयः_Func(m_trigVecDTypep);
        funcp = m_any_समुच्चयः_Vecp;
    } अन्यथा यदि (vscp->dtypep() == m_trigExtDTypep) {
        यदि (!m_any_समुच्चयः_Extp) m_any_समुच्चयः_Extp = create_Any_समुच्चयः_Func(m_trigExtDTypep);
        funcp = m_any_समुच्चयः_Extp;
    } अन्यथा {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, funcp};
    callp->सङ्कलय_Argsp(नूतन AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_समुच्चयः_Bit();
    प्रत्यागम callp;
}
Ast_ग्रन्थिः_Stmt* TriggerKit::newClearCall(AstVarScope* ध्रुव vscp) ध्रुव {
    यदि (!m_n_Vec_शब्दाः) प्रत्यागम शून्यसूचकः;
    UASSERT_OBJ(vscp->dtypep() == m_trigVecDTypep, vscp, "Bad trigger vector type");
    यदि (!m_clearp) m_clearp = createClearFunc();
    File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->शिखरम्_Scopep()->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, m_clearp};
    callp->सङ्कलय_Argsp(नूतन AstVarRef{flp, vscp, VAccess::WRITE});
    callp->dtype_समुच्चयः_Void();
    प्रत्यागम callp->makeStmt();
}
Ast_ग्रन्थिः_Stmt* TriggerKit::newOrIntoCall(AstVarScope* ध्रुव oVscp, AstVarScope* ध्रुव iVscp) ध्रुव {
    यदि (!m_n_Vec_शब्दाः) प्रत्यागम शून्यसूचकः;
    UASSERT_OBJ(iVscp->dtypep() == m_trigVecDTypep || iVscp->dtypep() == m_trigExtDTypep, iVscp,
                "Bad input trigger vector type");
    UASSERT_OBJ(oVscp->dtypep() == m_trigVecDTypep || oVscp->dtypep() == m_trigExtDTypep, oVscp,
                "Bad output trigger vector type");
    ध्रुव आकार_t आवरणम्
        = ((oVscp->dtypep() == m_trigExtDTypep) << 1) | (iVscp->dtypep() == m_trigExtDTypep);
    AstCFunc*& funcp = m_orIntoVecps[आवरणम्];
    यदि (!funcp) {
        funcp = createOrIntoFunc(VN_AS(oVscp->dtypep(), UnpackArrayDType),
                                 VN_AS(iVscp->dtypep(), UnpackArrayDType));
    }
    File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->शिखरम्_Scopep()->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, funcp};
    callp->सङ्कलय_Argsp(नूतन AstVarRef{flp, oVscp, VAccess::WRITE});
    callp->सङ्कलय_Argsp(नूतन AstVarRef{flp, iVscp, VAccess::READ});
    callp->dtype_समुच्चयः_Void();
    प्रत्यागम callp->makeStmt();
}

Ast_ग्रन्थिः_Stmt* TriggerKit::new_Comp_आधारः_Call() ध्रुव {
    यदि (!m_n_Vec_शब्दाः) प्रत्यागम शून्यसूचकः;
    File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->शिखरम्_Scopep()->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, m_compVecp};
    callp->dtype_समुच्चयः_Void();
    प्रत्यागम callp->makeStmt();
}

Ast_ग्रन्थिः_Stmt* TriggerKit::newCompExtCall(AstVarScope* vscp) ध्रुव {
    यदि (!m_n_Pre_शब्दाः) प्रत्यागम शून्यसूचकः;
    File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->शिखरम्_Scopep()->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, m_compExtp};
    callp->सङ्कलय_Argsp(नूतन AstVarRef{flp, vscp, VAccess::READ});
    callp->dtype_समुच्चयः_Void();
    प्रत्यागम callp->makeStmt();
}

Ast_ग्रन्थिः_Stmt* TriggerKit::newDumpCall(AstVarScope* ध्रुव vscp, ध्रुव std::string& चिह्नकम्,
                                     तार्किक debugOnly) ध्रुव {
    यदि (!m_n_Vec_शब्दाः) प्रत्यागम शून्यसूचकः;
    AstCFunc* funcp = शून्यसूचकः;
    यदि (vscp->dtypep() == m_trigVecDTypep) {
        funcp = m_dumpp;
    } अन्यथा यदि (vscp->dtypep() == m_trigExtDTypep) {
        यदि (!m_dumpExtp) m_dumpExtp = createDumpExtFunc();
        funcp = m_dumpExtp;
    } अन्यथा {
        vscp->v3fatalSrc("Bad trigger vector type");
    }
    File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->शिखरम्_Scopep()->fileline();
    AstCCall* ध्रुव callp = नूतन AstCCall{flp, funcp};
    callp->सङ्कलय_Argsp(नूतन AstVarRef{flp, vscp, VAccess::READ});
    callp->सङ्कलय_Argsp(नूतन AstConst{flp, AstConst::String{}, चिह्नकम्});
    callp->dtype_समुच्चयः_Void();
    AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{flp};
    cstmtp->सङ्कलय("#ifdef VL_DEBUG\n");
    यदि (debugOnly) {
        cstmtp->सङ्कलय("if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {\n");
        cstmtp->सङ्कलय(callp->makeStmt());
        cstmtp->सङ्कलय("}\n");
    } अन्यथा {
        cstmtp->सङ्कलय(callp->makeStmt());
    }
    cstmtp->सङ्कलय("#endif");
    प्रत्यागम cstmtp;
}

AstVarScope* TriggerKit::newTrigVec(ध्रुव std::string& name) ध्रुव {
    यदि (!m_n_Vec_शब्दाः) प्रत्यागम शून्यसूचकः;
    प्रत्यागम v3Global.rootp()->शिखरम्_Scopep()->create_अस्थायी("__V" + name + "Triggered", m_trigVecDTypep);
}

uint32_t TriggerKit::extra_Trigger_सूचकांक(uint32_t सूचकांक) ध्रुव {
    UASSERT(सूचकांक <= m_n_Extra_शब्दाः * WORD_आकार, "Invalid external trigger index");
    प्रत्यागम सूचकांक + m_n_Sense_शब्दाः * WORD_आकार;
}

std::समुच्चयः<uint32_t> TriggerKit::triggerIndices(ध्रुव Ast_Sen_वृक्षः* senTreep, तार्किक pre) ध्रुव {
    std::समुच्चयः<uint32_t> indices;
    प्रति (ध्रुव AstSenItem* sip = senTreep->sensesp(); sip; sip = VN_AS(sip->nextp(), SenItem)) {
        uint32_t सूचकांक = m_senItem2TrigIdx.at(*sip);
        यदि (pre) {
            UASSERT_OBJ(सूचकांक < m_n_Pre_शब्दाः * WORD_आकार, sip, "Not a 'pre' trigger");
            सूचकांक += m_n_Vec_शब्दाः * WORD_आकार;
        }
        indices.निवेशय(सूचकांक);
    }
    प्रत्यागम indices;
}

Ast_ग्रन्थिः_Stmt* TriggerKit::newExtraTriggerAssignment(AstVarScope* vscp, uint32_t सूचकांक) ध्रुव {
    सूचकांक = extra_Trigger_सूचकांक(सूचकांक);
    ध्रुव uint32_t word_सूचकांक = सूचकांक / WORD_आकार;
    ध्रुव uint32_t bit_सूचकांक = सूचकांक % WORD_आकार;
    File_पङ्क्तिः* ध्रुव flp = vscp->fileline();
    AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव wordp = नूतन AstArraySel{flp, refp, static_cast<पूर्णाङ्क>(word_सूचकांक)};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव trigLhsp = नूतन AstSel{flp, wordp, static_cast<पूर्णाङ्क>(bit_सूचकांक), 1};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव trigRhsp = नूतन AstVarRef{flp, vscp, VAccess::READ};
    प्रत्यागम नूतन Ast_नियोजय{flp, trigLhsp, trigRhsp};
}

शून्य TriggerKit::सङ्कलय_Extra_Trigger_Assignment(AstVarScope* vscp, uint32_t सूचकांक) ध्रुव {
    File_पङ्क्तिः* ध्रुव flp = vscp->fileline();
    // Set the trigger bit, then clear the input variable
    Ast_ग्रन्थिः* ध्रुव setp = newExtraTriggerAssignment(vscp, सूचकांक);
    setp->सङ्कलय_अग्र(नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, vscp, VAccess::WRITE},
                                नूतन AstConst{flp, AstConst::BitFalse{}}});
    m_compVecp->prependStmtsp(setp);
}

// Value-change detection for a single interface member VarScope written through a VIF.
// Creates: trigger[bit] = (vscp != prev); prev = vscp;
शून्य TriggerKit::सङ्कलय_मानम्_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                                 AstVarScope* instVscp, uint32_t सूचकांक) ध्रुव {
    सूचकांक += m_n_Sense_शब्दाः * WORD_आकार;
    ध्रुव uint32_t word_सूचकांक = सूचकांक / WORD_आकार;
    ध्रुव uint32_t bit_सूचकांक = सूचकांक % WORD_आकार;

    Ast_शिखरम्_Scope* ध्रुव शिखरम्_Scopep = netlistp->शिखरम्_Scopep();
    File_पङ्क्तिः* ध्रुव flp = netlistp->fileline();
    Ast_ग्रन्थिः_DType* ध्रुव dtypep = instVscp->dtypep()->skipRefp();

    // Reject types that do not support value-change comparison
    यदि (ध्रुव AstBasicDType* ध्रुव bdtypep = VN_CAST(dtypep, BasicDType)) {
        यदि (bdtypep->isEvent() || bdtypep->isString()
            || bdtypep->keyword() == VBasicDTypeKwd::CHANDLE) {
            instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                                << dtypep->prettyDTypeNameQ() << " type");
            प्रत्यागम;
        }
    } अन्यथा यदि (!VN_IS(dtypep, PackArrayDType) && !VN_IS(dtypep, UnpackArrayDType)
               && !VN_IS(dtypep, ग्रन्थिः_UOr_Struct_DType) && !VN_IS(dtypep, ClassRefDType)) {
        instVscp->v3warn(E_UNSUPPORTED, "Unsupported: virtual interface trigger on "
                                            << dtypep->prettyDTypeNameQ() << " type");
        प्रत्यागम;
    }

    ध्रुव स्वचालित rdInst = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::READ}; };
    ध्रुव स्वचालित wr_पूर्व = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::WRITE}; };
    ध्रुव स्वचालित rd_पूर्व = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::READ}; };

    // Create prev variable
    ध्रुव std::string पूर्व_Name = "__Vtrigprevvif_" + m_name + "_"
                                 + instVscp->scopep()->nameDotless() + "__"
                                 + instVscp->varp()->name();
    AstVarScope* ध्रुव पूर्व_Vscp = शिखरम्_Scopep->create_अस्थायी(पूर्व_Name, instVscp->dtypep());

    // Initialize prev = inst
    यदि (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ध्रुव cmhp = नूतन AstCMethodHard{
            flp, wr_पूर्व(पूर्व_Vscp), VCMethod::UNPACKED_नियोजय, rdInst(instVscp)};
        cmhp->dtype_समुच्चयः_Void();
        initFuncp->सङ्कलय_Stmtsp(cmhp->makeStmt());
    } अन्यथा {
        initFuncp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, wr_पूर्व(पूर्व_Vscp), rdInst(instVscp)});
    }

    // Build comparison: inst != prev
    Ast_ग्रन्थिः_व्यञ्जकम्* neqp;
    यदि (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ध्रुव cmhp
            = नूतन AstCMethodHard{flp, rd_पूर्व(पूर्व_Vscp), VCMethod::UNPACKED_NEQ, rdInst(instVscp)};
        cmhp->dtype_समुच्चयः_Bit();
        neqp = cmhp;
    } अन्यथा {
        neqp = नूतन AstNeq{flp, rdInst(instVscp), rd_पूर्व(पूर्व_Vscp)};
    }

    // Build post-update: prev = inst
    Ast_ग्रन्थिः* updp;
    यदि (VN_IS(dtypep, UnpackArrayDType)) {
        AstCMethodHard* ध्रुव cmhp = नूतन AstCMethodHard{
            flp, wr_पूर्व(पूर्व_Vscp), VCMethod::UNPACKED_नियोजय, rdInst(instVscp)};
        cmhp->dtype_समुच्चयः_Void();
        updp = cmhp->makeStmt();
    } अन्यथा {
        updp = नूतन Ast_नियोजय{flp, wr_पूर्व(पूर्व_Vscp), rdInst(instVscp)};
    }

    // Set trigger bit
    AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, m_vscp, VAccess::WRITE};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव wordp = नूतन AstArraySel{flp, refp, static_cast<पूर्णाङ्क>(word_सूचकांक)};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव trigLhsp = नूतन AstSel{flp, wordp, static_cast<पूर्णाङ्क>(bit_सूचकांक), 1};
    Ast_ग्रन्थिः* ध्रुव setp = नूतन Ast_नियोजय{flp, trigLhsp, neqp};

    // Chain: set trigger bit -> update prev
    setp->सङ्कलय_अग्र(updp);

    m_compVecp->prependStmtsp(setp);
}

TriggerKit::TriggerKit(ध्रुव std::string& name, तार्किक slow, uint32_t n_Sense_शब्दाः,
                       uint32_t n_Extra_शब्दाः, uint32_t n_Pre_शब्दाः,
                       std::unordered_map<VNRef<ध्रुव AstSenItem>, आकार_t> senItem2TrigIdx,
                       ध्रुव std::सदिशम्<ध्रुव AstSenItem*>& senItemps, तार्किक useAcc)
    : m_name{name}
    , m_slow{slow}
    , m_n_Sense_शब्दाः{n_Sense_शब्दाः}
    , m_n_Extra_शब्दाः{n_Extra_शब्दाः}
    , m_n_Pre_शब्दाः{n_Pre_शब्दाः}
    , m_senItem2TrigIdx{std::move(senItem2TrigIdx)}
    , m_trigIdx2SenItem{senItemps} {
    // If no triggers, we don't need to generate anything
    यदि (!m_n_Vec_शब्दाः) प्रत्यागम;
    // Otherwise construct the parts of the kit
    // Map from bit index to SenItem, including the 'pre' copies in the extended vector
    m_trigIdx2SenItem.resize((m_n_Vec_शब्दाः + m_n_Pre_शब्दाः) * WORD_आकार);
    std::copy(senItemps.begin(), senItemps.begin() + m_n_Pre_शब्दाः * WORD_आकार,
              m_trigIdx2SenItem.begin() + m_n_Vec_शब्दाः * WORD_आकार);
    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    AstScope* ध्रुव scopep = netlistp->शिखरम्_Scopep()->scopep();
    File_पङ्क्तिः* ध्रुव flp = scopep->fileline();
    // Data type of a single trigger word
    m_wordDTypep = netlistp->findBitDType(WORD_आकार, WORD_आकार, VSigning::UNSIGNED);
    // Data type of trigger vector
    AstRange* ध्रुव rp = नूतन AstRange{flp, static_cast<पूर्णाङ्क>(m_n_Vec_शब्दाः - 1), 0};
    m_trigVecDTypep = नूतन AstUnpackArrayDType{flp, m_wordDTypep, rp};
    netlistp->typeTablep()->सङ्कलय_Typesp(m_trigVecDTypep);
    // Data type of extended trigger vector, which only differs if there are pre triggers
    यदि (m_n_Pre_शब्दाः) {
        AstRange* ध्रुव ep = नूतन AstRange{flp, static_cast<पूर्णाङ्क>(m_n_Vec_शब्दाः + m_n_Pre_शब्दाः - 1), 0};
        m_trigExtDTypep = नूतन AstUnpackArrayDType{flp, m_wordDTypep, ep};
        netlistp->typeTablep()->सङ्कलय_Typesp(m_trigExtDTypep);
        m_compExtp = util::makeSubFunction(netlistp, "_eval_triggers_ext__" + m_name, m_slow);
    } अन्यथा {
        m_trigExtDTypep = m_trigVecDTypep;
    }
    // The AstVarScope representing the extended trigger vector
    m_vscp = netlistp->शिखरम्_Scopep()->create_अस्थायी("__V" + m_name + "Triggered", m_trigExtDTypep);
    m_vscp->varp()->is_आन्तरिकम्(सत्य);
    // The trigger computation function
    m_compVecp = util::makeSubFunction(netlistp, "_eval_triggers_vec__" + m_name, m_slow);
    // The debug dump function, always 'slow'
    m_dumpp = util::makeSubFunction(netlistp, "_dump_triggers__" + m_name, सत्य);
    m_dumpp->isStatic(सत्य);
    m_dumpp->ifdef("VL_DEBUG");
    यदि (useAcc) {
        m_vscAccp
            = netlistp->शिखरम्_Scopep()->create_अस्थायी("__V" + m_name + "TriggeredAcc", m_trigVecDTypep);
        m_vscAccp->varp()->is_आन्तरिकम्(सत्य);
    }
}

Ast_नियोजय* TriggerKit::createSenTrigVecAssignment(AstVarScope* ध्रुव target,
                                                  std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>& trigps) {
    File_पङ्क्तिः* ध्रुव flp = target->fileline();
    Ast_नियोजय* trigStmtsp = शून्यसूचकः;
    // Assign sense triggers vector one word at a time
    प्रति (आकार_t i = 0; i < trigps.आकार(); i += WORD_आकार) {
        // Concatenate all bits in this trigger word using a balanced
        प्रति (uint32_t level = 0; level < WORD_आकार_LOG2; ++level) {
            ध्रुव uint32_t stride = 1 << level;
            प्रति (uint32_t j = 0; j < WORD_आकार; j += 2 * stride) {
                trigps[i + j] = नूतन AstConcat{trigps[i + j]->fileline(), trigps[i + j + stride],
                                              trigps[i + j]};
                trigps[i + j + stride] = शून्यसूचकः;
            }
        }

        // Set the whole word in the trigger vector
        ध्रुव पूर्णाङ्क word_सूचकांक = static_cast<पूर्णाङ्क>(i / WORD_आकार);
        AstArraySel* ध्रुव aselp
            = नूतन AstArraySel{flp, नूतन AstVarRef{flp, target, VAccess::WRITE}, word_सूचकांक};
        trigStmtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(trigStmtsp, नूतन Ast_नियोजय{flp, aselp, trigps[i]});
    }
    प्रत्यागम trigStmtsp;
}

TriggerKit TriggerKit::create(AstNetlist* netlistp,  //
                              AstCFunc* ध्रुव initFuncp,  //
                              Sen_व्यञ्जकम्_Builder& sen_व्यञ्जकम्_Builder,  //
                              ध्रुव std::सदिशम्<ध्रुव Ast_Sen_वृक्षः*>& preTreeps,  //
                              ध्रुव std::सदिशम्<ध्रुव Ast_Sen_वृक्षः*>& senTreeps,  //
                              ध्रुव string& name,  //
                              ध्रुव ExtraTriggers& extraTriggers,  //
                              तार्किक slow,  //
                              तार्किक useAcc) {
    // Need to gather all the unique SenItems under the given SenTrees

    // List of unique SenItems used by all 'senTreeps'
    std::सदिशम्<ध्रुव AstSenItem*> senItemps;
    // Map from SenItem to trigger bit standing for that SenItem. There might
    // be duplicate SenItems, we map all of them to the same index.
    std::unordered_map<VNRef<ध्रुव AstSenItem>, आकार_t> senItem2TrigIdx;

    // Process the 'pre' trees first, so they are at the begining of the vector
    प्रति (ध्रुव Ast_Sen_वृक्षः* ध्रुव senTreep : preTreeps) {
        प्रति (ध्रुव AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ध्रुव स्वचालित pair = senItem2TrigIdx.emplace(*itemp, senItemps.आकार());
            यदि (pair.second) senItemps.प्रक्षिप_पृष्ठभागः(itemp);
        }
    }
    ध्रुव uint32_t n_Pre_Sen_वस्तूनि = senItemps.आकार();
    V3Stats::सङ्कलय_Stat("Scheduling, '" + name + "' pre triggers", n_Pre_Sen_वस्तूनि);
    // Number of pre triggers, rounded up to a full word.
    ध्रुव uint32_t nPreTriggers = vlstd::आवर्तनम्_ऊर्ध्वम्_गन्तव्यम्_गुणजः_सम्बन्धि<WORD_आकार>(senItemps.आकार());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nPreTriggers);
    // Number of words for pre triggers
    ध्रुव uint32_t n_Pre_शब्दाः = nPreTriggers / WORD_आकार;

    // Process the rest of the trees
    प्रति (ध्रुव Ast_Sen_वृक्षः* ध्रुव senTreep : senTreeps) {
        प्रति (ध्रुव AstSenItem *itemp = senTreep->sensesp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), SenItem);
            UASSERT_OBJ(itemp->isClocked() || itemp->isHybrid(), itemp,
                        "Cannot create trigger expression for non-clocked sensitivity");
            ध्रुव स्वचालित pair = senItem2TrigIdx.emplace(*itemp, senItemps.आकार());
            यदि (pair.second) senItemps.प्रक्षिप_पृष्ठभागः(itemp);
        }
    }
    ध्रुव uint32_t n_Sen_वस्तूनि = senItemps.आकार() - nPreTriggers;
    V3Stats::सङ्कलय_Stat("Scheduling, '" + name + "' sense triggers", n_Sen_वस्तूनि + n_Pre_Sen_वस्तूनि);
    // Number of sense triggers, rounded up to a full word
    ध्रुव uint32_t nSenseTriggers = vlstd::आवर्तनम्_ऊर्ध्वम्_गन्तव्यम्_गुणजः_सम्बन्धि<WORD_आकार>(senItemps.आकार());
    // Pad 'senItemps' to nSenseTriggers with nullptr
    senItemps.resize(nSenseTriggers);
    // Number of words sense triggers (inclued pre)
    ध्रुव uint32_t n_Sense_शब्दाः = nSenseTriggers / WORD_आकार;

    // Allocate space for the extra triggers
    V3Stats::सङ्कलय_Stat("Scheduling, '" + name + "' extra triggers", extraTriggers.आकार());
    // Number of extra triggers, rounded up to a full word.
    ध्रुव uint32_t nExtraTriggers = vlstd::आवर्तनम्_ऊर्ध्वम्_गन्तव्यम्_गुणजः_सम्बन्धि<WORD_आकार>(extraTriggers.आकार());
    ध्रुव uint32_t n_Extra_शब्दाः = nExtraTriggers / WORD_आकार;

    // We can now construct the trigger kit - this constructs all items that will be kept
    TriggerKit kit{name,      slow,  n_Sense_शब्दाः, n_Extra_शब्दाः, n_Pre_शब्दाः, senItem2TrigIdx,
                   senItemps, useAcc};

    // If there are no triggers we are done
    यदि (!kit.m_n_Vec_शब्दाः) प्रत्यागम kit;

    File_पङ्क्तिः* ध्रुव flp = netlistp->शिखरम्_Scopep()->fileline();

    // Creates read/write reference
    ध्रुव स्वचालित rd = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::READ}; };
    ध्रुव स्वचालित wr = [flp](AstVarScope* vp) { प्रत्यागम नूतन AstVarRef{flp, vp, VAccess::WRITE}; };

    // Construct the comp and dump functions

    // Add arguments to the dump function. The trigger vector is passed into
    // the dumping function via reference so one dump function can dump all
    // different copies of the trigger vector. To do so, it also needs the tag
    // string at runtime, which is the second argument.
    AstVarScope* ध्रुव dumpTrgp
        = new_तर्कः(kit.m_dumpp, kit.m_trigVecDTypep, "triggers", VDirection::CONSTREF);
    AstVarScope* ध्रुव dumpTagp
        = new_तर्कः(kit.m_dumpp, netlistp->findStringDType(), "tag", VDirection::CONSTREF);

    // Add a print to the dumping function if there are no triggers pending
    {
        AstIf* ध्रुव ifp = नूतन AstIf{flp, नूतन AstLogNot{flp, kit.new_Any_समुच्चयः_Call(dumpTrgp)}};
        kit.m_dumpp->सङ्कलय_Stmtsp(ifp);
        AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{flp};
        ifp->सङ्कलय_Thensp(cstmtp);
        cstmtp->सङ्कलय("VL_DBG_MSGS(\"         No '\" + ");
        cstmtp->सङ्कलय(rd(dumpTagp));
        cstmtp->सङ्कलय(" + \"\' region triggers active\\n\");");
    }

    // Adds a debug dumping statement for this trigger
    ध्रुव स्वचालित सङ्कलय_Debug = [&](uint32_t सूचकांक, ध्रुव string& text) {
        ध्रुव पूर्णाङ्क wrd_सूचकांक = static_cast<पूर्णाङ्क>(सूचकांक / WORD_आकार);
        ध्रुव पूर्णाङ्क bit_सूचकांक = static_cast<पूर्णाङ्क>(सूचकांक % WORD_आकार);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव aselp = नूतन AstArraySel{flp, rd(dumpTrgp), wrd_सूचकांक};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = नूतन AstSel{flp, aselp, bit_सूचकांक, 1};
        AstIf* ध्रुव ifp = नूतन AstIf{flp, condp};
        kit.m_dumpp->सङ्कलय_Stmtsp(ifp);
        AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{flp};
        ifp->सङ्कलय_Thensp(cstmtp);
        cstmtp->सङ्कलय("VL_DBG_MSGS(\"         '\" + ");
        cstmtp->सङ्कलय(rd(dumpTagp));
        cstmtp->सङ्कलय(" + \"' region trigger index " + std::गन्तव्यम्_string(सूचकांक) + " is active: " + text
                    + "\\n\");");
    };

    // Add sense trigger computation
    // List of trigger computation expressions
    std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> trigps;
    trigps.reserve(nSenseTriggers);
    // Statements to exectue at initialization time to fire initial triggers
    Ast_ग्रन्थिः_Stmt* initialTrigsp = शून्यसूचकः;
    प्रति (आकार_t i = 0; i < senItemps.आकार(); ++i) {
        ध्रुव AstSenItem* ध्रुव senItemp = senItemps[i];

        // If this is just paddign, use constant zero
        यदि (!senItemp) {
            trigps.emplace_पृष्ठभागः(नूतन AstConst{flp, AstConst::BitFalse{}});
            अनुवर्त;
        }

        // Create the trigger computation expression
        ध्रुव स्वचालित& pair = sen_व्यञ्जकम्_Builder.build(senItemp);
        trigps.emplace_पृष्ठभागः(pair.first);

        // Add initialization time trigger
        यदि (pair.second || v3Global.opt.xInitialEdge()) {
            ध्रुव पूर्णाङ्क wrd_सूचकांक = static_cast<पूर्णाङ्क>(i / WORD_आकार);
            ध्रुव पूर्णाङ्क bit_सूचकांक = static_cast<पूर्णाङ्क>(i % WORD_आकार);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव wordp = नूतन AstArraySel{flp, wr(kit.m_vscp), wrd_सूचकांक};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstSel{flp, wordp, bit_सूचकांक, 1};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstConst{flp, AstConst::BitTrue{}};
            यदि (useAcc) {
                initFuncp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, lhsp, rhsp});
            } अन्यथा {
                initialTrigsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(initialTrigsp, नूतन Ast_नियोजय{flp, lhsp, rhsp});
            }
        }

        // Add a debug statement for this trigger
        std::stringstream ss;
        ss << "@(";
        V3EmitV::verilog_For_वृक्षः(senItemp, ss);
        ss << ")";
        // Names were hashed by V3Name, show the user the original text
        std::string desc = VString::quoteBackslash(VName::dehash(ss.str()));
        desc = VString::replaceSubstr(desc, "\n", "\\n");
        सङ्कलय_Debug(i, desc);
    }
    UASSERT(trigps.आकार() == nSenseTriggers, "Inconsistent number of trigger expressions");

    Ast_नियोजय* ध्रुव trigStmtsp = createSenTrigVecAssignment(kit.m_vscp, trigps);

    // Add a print for each of the extra triggers
    प्रति (अचिह्नित i = 0; i < extraTriggers.आकार(); ++i) {
        सङ्कलय_Debug(nSenseTriggers + i,
                 "Internal '" + name + "' trigger - " + extraTriggers.m_descriptions.at(i));
    }

    // Get the SenExprBuilder results
    ध्रुव Sen_व्यञ्जकम्_Builder::Results senResults = sen_व्यञ्जकम्_Builder.getResultsAndClearUpdates();

    // Add the SenExprBuilder init statements to the static initialization functino
    प्रति (Ast_ग्रन्थिः_Stmt* ध्रुव nodep : senResults.m_inits) initFuncp->सङ्कलय_Stmtsp(nodep);

    // Assemble the base trigger computation function
    Ast_शिखरम्_Scope* ध्रुव शिखरम्_Scopep = netlistp->शिखरम्_Scopep();
    {
        AstCFunc* ध्रुव fp = kit.m_compVecp;
        // Trigger computation
        प्रति (Ast_ग्रन्थिः_Stmt* ध्रुव nodep : senResults.m_preUpdates) fp->सङ्कलय_Stmtsp(nodep);
        fp->सङ्कलय_Stmtsp(trigStmtsp);
        प्रति (Ast_ग्रन्थिः_Stmt* ध्रुव nodep : senResults.m_postUpdates) fp->सङ्कलय_Stmtsp(nodep);
        // Add the initialization time triggers
        यदि (initialTrigsp) {
            AstVarScope* ध्रुव initVscp = शिखरम्_Scopep->create_अस्थायी("__V" + name + "DidInit", 1);
            AstIf* ध्रुव ifp = नूतन AstIf{flp, नूतन AstNot{flp, rd(initVscp)}};
            fp->सङ्कलय_Stmtsp(ifp);
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            ifp->सङ्कलय_Thensp(util::समुच्चयः_Var(initVscp, 1));
            ifp->सङ्कलय_Thensp(initialTrigsp);
        }
        util::विभज_Check(fp);
    };
    // If there are 'pre' triggers, compute them
    यदि (kit.m_n_Pre_शब्दाः) {
        AstCFunc* ध्रुव fp = kit.m_compExtp;
        // Add an argument to the function that takes the latched values
        AstVarScope* ध्रुव latchedp
            = new_तर्कः(fp, kit.m_trigVecDTypep, "latched", VDirection::CONSTREF);
        // Add loop counter variable - this can't be local because we call util::splitCheck
        AstVarScope* ध्रुव nVscp = शिखरम्_Scopep->create_अस्थायी("__V" + name + "TrigPreLoopCounter", 32);
        nVscp->varp()->noReset(सत्य);
        // Add a loop to compute the pre words
        AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
        fp->सङ्कलय_Stmtsp(util::समुच्चयः_Var(nVscp, 0));
        fp->सङ्कलय_Stmtsp(loopp);
        // Loop body
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव offsetp = नूतन AstConst{flp, kit.m_n_Vec_शब्दाः};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lIdxp = नूतन Ast_सङ्कलय{flp, rd(nVscp), offsetp};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstArraySel{flp, wr(kit.m_vscp), lIdxp};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव aWordp = नूतन AstArraySel{flp, rd(kit.m_vscp), rd(nVscp)};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bWordp = नूतन AstArraySel{flp, rd(latchedp), rd(nVscp)};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstAnd{flp, aWordp, नूतन AstNot{flp, bWordp}};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव limp = नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, n_Pre_शब्दाः};
        loopp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, lhsp, rhsp});
        loopp->सङ्कलय_Stmtsp(util::incrementVar(nVscp));
        loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLt{flp, rd(nVscp), limp}});
        util::विभज_Check(fp);
    }
    // Done with the trigger computation function, split as might be large

    // The debug code might leak signal names, so simply delete it when using --protect-ids
    यदि (v3Global.opt.protectIds()) kit.m_dumpp->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र()->delete_वृक्षः();
    // Done with the trigger dump function, split as might be large
    util::विभज_Check(kit.m_dumpp);

    प्रत्यागम kit;
}

//  Find all CAwaits, clear SenTrees inside them, generate before-trigger functions (functions that
//  shall be called before awaiting for a VCMethod::SCHED_TRIGGER) and add thier calls before
//  proper CAwaits
वर्ग AwaitBeforeTrigVisitor final : सार्वजनिक VNVisitor {
    ध्रुव VNUser1_अन्तर्_Use m_user1_अन्तर्_Use;

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
    Sen_व्यञ्जकम्_Builder& m_sen_व्यञ्जकम्_Builder;
    // Generator of unique names for before-trigger function
    V3_Unique_नामानि m_beforeTriggerFuncUniqueName;

    // Vector containing every generated CFuncs and related SenTree
    std::सदिशम्<std::pair<AstCFunc*, Ast_Sen_वृक्षः*>> m_generatedFuncs;
    // Vector containing SenTrees and coresponding scheduler
    std::सदिशम्<std::pair<Ast_Sen_वृक्षः*, Ast_ग्रन्थिः_व्यञ्जकम्*>> m_sen_वृक्षः_गन्तव्यम्_Sched;
    // Map containing vectors of SenItems that share the same prevValue variable
    std::unordered_map<VNRef<Ast_ग्रन्थिः>, std::सदिशम्<AstSenItem*>> m_sen_व्यञ्जकम्_गन्तव्यम्_Sen_Item;

    // Returns node which is used for grouping SenItems in `m_senExprToSenItem`
    स्थिर Ast_ग्रन्थिः* get_Sen_Hash_ग्रन्थिः(ध्रुव AstSenItem* ध्रुव nodep) {
        यदि (AstVarRef* ध्रुव varRefp = VN_CAST(nodep->sensp(), VarRef)) प्रत्यागम varRefp;
        प्रत्यागम nodep->sensp();
    }

    // Populates `m_senExprToSenItem` with every group of SenItems that share the same prevValue
    // variable. Groups that contain only one type of an edge are omitted.
    शून्य fill_Sen_व्यञ्जकम्_गन्तव्यम्_Sen_Item() {
        प्रति (स्वचालित sen_वृक्षः_Sched : m_sen_वृक्षः_गन्तव्यम्_Sched) {
            Ast_Sen_वृक्षः* ध्रुव senTreep = sen_वृक्षः_Sched.first;

            प्रति (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ध्रुव VEdgeType edge = senItemp->edgeType();
                यदि (edge.anEdge() || edge == VEdgeType::ET_परिवर्तितम्
                    || edge == VEdgeType::ET_HYBRID) {
                    m_sen_व्यञ्जकम्_गन्तव्यम्_Sen_Item[*get_Sen_Hash_ग्रन्थिः(senItemp)].प्रक्षिप_पृष्ठभागः(senItemp);
                }
            }
        }

        std::सदिशम्<VNRef<Ast_ग्रन्थिः>> गन्तव्यम्_Remove;
        प्रति (ध्रुव स्वचालित& sen_व्यञ्जकम्_गन्तव्यम्_Sen_वृक्षः : m_sen_व्यञ्जकम्_गन्तव्यम्_Sen_Item) {
            std::सदिशम्<AstSenItem*> senItemps = sen_व्यञ्जकम्_गन्तव्यम्_Sen_वृक्षः.second;
            गन्तव्यम्_Remove.प्रक्षिप_पृष्ठभागः(sen_व्यञ्जकम्_गन्तव्यम्_Sen_वृक्षः.first);
            प्रति (आकार_t i = 1; i < senItemps.आकार(); ++i) {
                यदि (senItemps[i]->edgeType() != senItemps[i - 1]->edgeType()) {
                    गन्तव्यम्_Remove.निष्कासय_पृष्ठभागः();
                    भङ्ग;
                }
            }
        }
        प्रति (ध्रुव VNRef<Ast_ग्रन्थिः> it : गन्तव्यम्_Remove) m_sen_व्यञ्जकम्_गन्तव्यम्_Sen_Item.erase(it);
    }

    // For set of bits indexes (of sensitivity vector) return map from those indexes to set
    // of schedulers sensitive to these indexes. Indices are split into word index and bit
    // masking this index within given word
    std::map<आकार_t, std::map<आकार_t, std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>>>
    get_प्रयुक्तम्_Triggers_गन्तव्यम्_Trees(ध्रुव std::समुच्चयः<आकार_t>& प्रयुक्तम्_Triggers) {
        std::map<आकार_t, std::map<आकार_t, std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>>> प्रयुक्तम्_Trigs_गन्तव्यम्_Using_Trees;
        प्रति (स्वचालित sen_वृक्षः_Sched : m_sen_वृक्षः_गन्तव्यम्_Sched) {
            ध्रुव Ast_Sen_वृक्षः* ध्रुव senTreep = sen_वृक्षः_Sched.first;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव shedp = sen_वृक्षः_Sched.second;

            // Find all common SenItem indexes for `senTreep` and `usedTriggers`
            प्रति (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
                 senItemp = VN_AS(senItemp->nextp(), SenItem)) {
                ध्रुव आकार_t idx = m_trigKit.senItem2TrigIdx(senItemp);
                यदि (प्रयुक्तम्_Triggers.find(idx) != प्रयुक्तम्_Triggers.अन्तः_2()) {
                    प्रयुक्तम्_Trigs_गन्तव्यम्_Using_Trees[idx / TriggerKit::WORD_आकार]
                                         [आकार_t{1} << (idx % TriggerKit::WORD_आकार)]
                                             .प्रक्षिप_पृष्ठभागः(shedp);
                }
            }
        }
        यदि (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            प्रति (ध्रुव स्वचालित& triggers_गन्तव्यम्_Trees : प्रयुक्तम्_Trigs_गन्तव्यम्_Using_Trees) {
                प्रति (ध्रुव स्वचालित& द्वयङ्काः_गन्तव्यम्_Trees : triggers_गन्तव्यम्_Trees.second) {
                    ध्रुव std::समुच्चयः<ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्*> exprps{द्वयङ्काः_गन्तव्यम्_Trees.second.begin(),
                                                              द्वयङ्काः_गन्तव्यम्_Trees.second.अन्तः_2()};
                    UASSERT(द्वयङ्काः_गन्तव्यम्_Trees.second.आकार() == exprps.आकार(),
                            "There is a SenTree with two SenItems indicating to the same bit");
                }
            }
        }
        प्रत्यागम प्रयुक्तम्_Trigs_गन्तव्यम्_Using_Trees;
    }

    // Returns a CCall to a before-trigger function for a given SenTree,
    // Constructs such a function if it doesn't exist yet
    AstCCall* getBeforeTriggerStmt(Ast_Sen_वृक्षः* ध्रुव senTreep) {
        File_पङ्क्तिः* ध्रुव flp = senTreep->fileline();
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
            argp->direction(VDirection::निवेशः);
            funcp->सङ्कलय_Argsp(argp);
            // Scope is created in the constructor after iterate finishes

            m_generatedFuncs.emplace_पृष्ठभागः(funcp, senTreep);
        }
        AstCCall* ध्रुव callp = नूतन AstCCall{flp, VN_AS(senTreep->user1p(), CFunc)};
        callp->dtype_समुच्चयः_Void();
        प्रत्यागम callp;
    }

    शून्य दर्शय(AstCAwait* ध्रुव nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;

        // Check whether it is a CAwait for a VCMethod::SCHED_TRIGGER
        यदि (ध्रुव AstCMethodHard* ध्रुव c_विधानम्_Hardp = VN_CAST(nodep->exprp(), CMethodHard)) {
            यदि (c_विधानम्_Hardp->विधानम्() == VCMethod::SCHED_TRIGGER) {
                AstCCall* ध्रुव beforeTrigp = getBeforeTriggerStmt(nodep->sentreep());

                // Add eventDescription argument value to a CCall - it is used for --runtime-debug
                Ast_ग्रन्थिः* ध्रुव pinp = c_विधानम्_Hardp->pinsp()->nextp()->nextp();
                UASSERT_OBJ(pinp, c_विधानम्_Hardp, "No event description");
                beforeTrigp->सङ्कलय_Argsp(VN_AS(pinp, ग्रन्थिः_व्यञ्जकम्)->clone_वृक्षः(असत्य));

                // Call the before-trigger function before the CAwait
                nodep->सङ्कलय_अत्र_This_As_अग्र(beforeTrigp->makeStmt());
                m_sen_वृक्षः_गन्तव्यम्_Sched.emplace_पृष्ठभागः(nodep->sentreep(), c_विधानम्_Hardp->fromp());
            }
        }
        nodep->clearSentreep();  // Clear as these sentrees will get deleted later
        iterate(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः* ध्रुव nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    AwaitBeforeTrigVisitor(AstNetlist* netlistp, Sen_व्यञ्जकम्_Builder& sen_व्यञ्जकम्_Builder,
                           ध्रुव TriggerKit& trigKit)
        : m_netlistp{netlistp}
        , m_trigKit{trigKit}
        , m_sen_व्यञ्जकम्_Builder{sen_व्यञ्जकम्_Builder}
        , m_beforeTriggerFuncUniqueName{"__VbeforeTrig"} {
        iterate(netlistp);

        fill_Sen_व्यञ्जकम्_गन्तव्यम्_Sen_Item();

        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> trigps;
        std::समुच्चयः<आकार_t> प्रयुक्तम्_Triggers;
        // In each of before-trigger functions check if anything was triggered and mark as ready
        // triggered schedulers
        प्रति (ध्रुव स्वचालित& func_गन्तव्यम्_प्रयुक्तम्_Triggers : m_generatedFuncs) {
            AstCFunc* ध्रुव funcp = func_गन्तव्यम्_प्रयुक्तम्_Triggers.first;
            AstVarScope* ध्रुव vscp = VN_AS(funcp->user1p(), VarScope);
            File_पङ्क्तिः* ध्रुव flp = funcp->fileline();

            // Generate trigger evaluation
            {
                Ast_Sen_वृक्षः* ध्रुव senTreep = func_गन्तव्यम्_प्रयुक्तम्_Triggers.second;
                // Puts `exprp` at `pos` and makes sure that trigps.size() is multiple of
                // TriggerKit::WORD_SIZE
                ध्रुव स्वचालित emplaceAt
                    = [flp, &trigps, &प्रयुक्तम्_Triggers](Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp, ध्रुव आकार_t pos) {
                          ध्रुव आकार_t target_आकार
                              = vlstd::आवर्तनम्_ऊर्ध्वम्_गन्तव्यम्_गुणजः_सम्बन्धि<TriggerKit::WORD_आकार>(pos + 1);
                          यदि (trigps.क्षमता() < target_आकार) trigps.reserve(target_आकार * 2);
                          यावत् (trigps.आकार() < target_आकार) {
                              trigps.प्रक्षिप_पृष्ठभागः(नूतन AstConst{flp, AstConst::BitFalse{}});
                          }
                          trigps[pos]->delete_वृक्षः();
                          trigps[pos] = exprp;
                          प्रयुक्तम्_Triggers.निवेशय(pos);
                      };

                // Find all trigger indexes of SenItems inside `senTreep`
                // and add them to `trigps` and `usedTriggers`
                प्रति (ध्रुव AstSenItem* itemp = senTreep->sensesp(); itemp;
                     itemp = VN_AS(itemp->nextp(), SenItem)) {
                    ध्रुव आकार_t idx = m_trigKit.senItem2TrigIdx(itemp);
                    emplaceAt(m_sen_व्यञ्जकम्_Builder.build(itemp).first, idx);
                    स्वचालित iter = m_sen_व्यञ्जकम्_गन्तव्यम्_Sen_Item.find(*get_Sen_Hash_ग्रन्थिः(itemp));
                    यदि (iter != m_sen_व्यञ्जकम्_गन्तव्यम्_Sen_Item.अन्तः_2()) {
                        प्रति (AstSenItem* ध्रुव additionalItemp : iter->second) {
                            ध्रुव आकार_t idx = m_trigKit.senItem2TrigIdx(additionalItemp);
                            emplaceAt(m_sen_व्यञ्जकम्_Builder.build(additionalItemp).first, idx);
                        }
                    }
                }

                // Fill the function with neccessary statements
                ध्रुव Sen_व्यञ्जकम्_Builder::Results results
                    = m_sen_व्यञ्जकम्_Builder.getResultsAndClearUpdates();
                प्रति (Ast_ग्रन्थिः_Stmt* ध्रुव stmtsp : results.m_inits) funcp->सङ्कलय_Stmtsp(stmtsp);
                प्रति (Ast_ग्रन्थिः_Stmt* ध्रुव stmtsp : results.m_preUpdates) funcp->सङ्कलय_Stmtsp(stmtsp);
                funcp->सङ्कलय_Stmtsp(TriggerKit::createSenTrigVecAssignment(vscp, trigps));
                trigps.clear();
                प्रति (Ast_ग्रन्थिः_Stmt* ध्रुव stmtsp : results.m_postUpdates) funcp->सङ्कलय_Stmtsp(stmtsp);
            }

            ध्रुव std::map<आकार_t, std::map<आकार_t, std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>>>
                प्रयुक्तम्_Trigs_गन्तव्यम्_Using_Trees = get_प्रयुक्तम्_Triggers_गन्तव्यम्_Trees(प्रयुक्तम्_Triggers);
            प्रयुक्तम्_Triggers.clear();

            // Helper returning expression getting array index `idx` from `scocep` with access
            // `access`
            ध्रुव स्वचालित getIdx = [flp](AstVarScope* ध्रुव scocep, VAccess access, आकार_t idx) {
                प्रत्यागम नूतन AstArraySel{flp, नूतन AstVarRef{flp, scocep, access},
                                       नूतन AstConst{flp, AstConst::Unsized64{}, idx}};
            };

            // Get eventDescription argument
            AstVarScope* ध्रुव argpVscp = नूतन AstVarScope{flp, funcp->scopep(), funcp->argsp()};
            funcp->scopep()->सङ्कलय_Varsp(argpVscp);

            // Mark as ready triggered schedulers
            प्रति (ध्रुव स्वचालित& triggers_गन्तव्यम्_Trees : प्रयुक्तम्_Trigs_गन्तव्यम्_Using_Trees) {
                ध्रुव आकार_t word = triggers_गन्तव्यम्_Trees.first;

                प्रति (ध्रुव स्वचालित& द्वयङ्काः_गन्तव्यम्_Trees : triggers_गन्तव्यम्_Trees.second) {
                    ध्रुव आकार_t bit = द्वयङ्काः_गन्तव्यम्_Trees.first;
                    ध्रुव स्वचालित& schedulers = द्वयङ्काः_गन्तव्यम्_Trees.second;

                    // Check if given bit is fired - single bits are checked since
                    // usually there is only a few of them (only one most of the times as we await
                    // only for one event)
                    AstConst* ध्रुव आवरणम्_Constp = नूतन AstConst{flp, AstConst::Unsized64{}, bit};
                    AstAnd* ध्रुव condp
                        = नूतन AstAnd{flp, getIdx(vscp, VAccess::READ, word), आवरणम्_Constp};
                    AstIf* ध्रुव ifp = नूतन AstIf{flp, condp};

                    // Call ready() on each scheduler sensitive to `condp`
                    प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव schedp : schedulers) {
                        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
                            flp, schedp->clone_वृक्षः(असत्य), VCMethod::SCHED_READY};
                        callp->dtype_समुच्चयः_Void();
                        callp->सङ्कलय_Pinsp(नूतन AstVarRef{flp, argpVscp, VAccess::READ});
                        ifp->सङ्कलय_Thensp(callp->makeStmt());
                    }
                    funcp->सङ्कलय_Stmtsp(ifp);
                }
            }

            AstVarScope* ध्रुव vscAccp = m_trigKit.vscAccp();
            // Add touched values to accumulator
            प्रति (ध्रुव स्वचालित& triggers_गन्तव्यम्_Trees : प्रयुक्तम्_Trigs_गन्तव्यम्_Using_Trees) {
                ध्रुव आकार_t word = triggers_गन्तव्यम्_Trees.first;
                funcp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, getIdx(vscAccp, VAccess::WRITE, word),
                                               नूतन AstOr{flp, getIdx(vscAccp, VAccess::READ, word),
                                                         getIdx(vscp, VAccess::READ, word)}});
            }
        }
    }
    ~AwaitBeforeTrigVisitor() override = स्वभाव;
};

शून्य beforeTrigVisitor(AstNetlist* netlistp, Sen_व्यञ्जकम्_Builder& sen_व्यञ्जकम्_Builder,
                       ध्रुव TriggerKit& trigKit) {
    AwaitBeforeTrigVisitor{netlistp, sen_व्यञ्जकम्_Builder, trigKit};
}

}  // namespace V3Sched
