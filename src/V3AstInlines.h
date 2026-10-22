// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Ast node inline functions
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

#నిర్వచితం_కాకపోతే VERILATOR_V3ASTINLINES_H_
#నిర్వచించు VERILATOR_V3ASTINLINES_H_

#నిర్వచితం_కాకపోతే VERILATOR_V3AST_H_
#దోషం "Use V3Ast.h as the include"
#అన్తర్భావ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#అయితే_ముగింపు

//######################################################################
// Inline METHODS

పూర్ణాంకం Ast_కణుపు::వెడల్పు() స్థిరరాశి VL_MT_STABLE { తిరిగి dtypep() ? dtypep()->వెడల్పు() : 0; }
పూర్ణాంకం Ast_కణుపు::వెడల్పు_Min() స్థిరరాశి VL_MT_STABLE { తిరిగి dtypep() ? dtypep()->వెడల్పు_Min() : 0; }
bool Ast_కణుపు::width1() స్థిరరాశి {  // V3Const uses to know it can optimize
    తిరిగి dtypep() && dtypep()->వెడల్పు() == 1;
}
పూర్ణాంకం Ast_కణుపు::వెడల్పు_Instrs() స్థిరరాశి {
    తిరిగి (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->వెడల్పు_పదాలు() : 1));
}
bool Ast_కణుపు::isCHandle() స్థిరరాశి VL_MT_STABLE {
    తిరిగి dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_కణుపు::isDouble() స్థిరరాశి VL_MT_STABLE {
    తిరిగి dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_కణుపు::isString() స్థిరరాశి VL_MT_STABLE {
    తిరిగి dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_కణుపు::isEvent() స్థిరరాశి VL_MT_STABLE {
    తిరిగి dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_కణుపు::isSigned() స్థిరరాశి VL_MT_STABLE { తిరిగి dtypep() && dtypep()->isSigned(); }

bool Ast_కణుపు::is_Class_Handle_విలువ() స్థిరరాశి {
    తిరిగి (VN_IS(this, Const) && VN_AS(this, Const)->అంకె().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_కణుపు::isNull() స్థిరరాశి { తిరిగి VN_IS(this, Const) && VN_AS(this, Const)->అంకె().isNull(); }
bool Ast_కణుపు::isZero() స్థిరరాశి {
    తిరిగి (VN_IS(this, Const) && VN_AS(this, Const)->అంకె().isEqZero());
}
bool Ast_కణుపు::isNeqZero() స్థిరరాశి {
    తిరిగి (VN_IS(this, Const) && VN_AS(this, Const)->అంకె().isNeqZero());
}
bool Ast_కణుపు::isOne() స్థిరరాశి { తిరిగి (VN_IS(this, Const) && VN_AS(this, Const)->అంకె().isEqOne()); }
bool Ast_కణుపు::isAllOnes() స్థిరరాశి {
    తిరిగి (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_కణుపు::isAllOnesV() స్థిరరాశి {
    తిరిగి (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_కణుపు::same_వృక్షం(స్థిరరాశి Ast_కణుపు* node2p) స్థిరరాశి {
    తిరిగి same_వృక్షం_Iter(this, node2p, true, false);
}
bool Ast_కణుపు::same_Gate_వృక్షం(స్థిరరాశి Ast_కణుపు* node2p) స్థిరరాశి {
    తిరిగి same_వృక్షం_Iter(this, node2p, true, true);
}

పూర్ణాంకం Ast_కణుపు_Array_DType::ఎడమ_2() స్థిరరాశి VL_MT_STABLE { తిరిగి rangep()->ఎడమ_Const(); }
పూర్ణాంకం Ast_కణుపు_Array_DType::కుడి_2() స్థిరరాశి VL_MT_STABLE { తిరిగి rangep()->కుడి_Const(); }
పూర్ణాంకం Ast_కణుపు_Array_DType::hi() స్థిరరాశి VL_MT_STABLE { తిరిగి rangep()->hiConst(); }
పూర్ణాంకం Ast_కణుపు_Array_DType::lo() స్థిరరాశి VL_MT_STABLE { తిరిగి rangep()->loConst(); }
పూర్ణాంకం Ast_కణుపు_Array_DType::elementsConst() స్థిరరాశి VL_MT_STABLE { తిరిగి rangep()->elementsConst(); }
VNumRange Ast_కణుపు_Array_DType::declRange() స్థిరరాశి VL_MT_STABLE { తిరిగి VNumRange{ఎడమ_2(), కుడి_2()}; }

AstFuncRef::AstFuncRef(File_పంక్తి* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_పంక్తి* fl, పూర్ణాంకం ఎడమ_2, పూర్ణాంకం కుడి_2)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(ఎడమ_2)});
    rightp(new AstConst{fl, static_cast<uint32_t>(కుడి_2)});
}
AstRange::AstRange(File_పంక్తి* fl, స్థిరరాశి VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.ఎడమ_2())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.కుడి_2())});
}
పూర్ణాంకం AstRange::ఎడమ_Const() స్థిరరాశి VL_MT_STABLE {
    స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(leftp(), Const);
    తిరిగి (constp ? constp->గమ్యం_SInt() : 0);
}
పూర్ణాంకం AstRange::కుడి_Const() స్థిరరాశి VL_MT_STABLE {
    స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(rightp(), Const);
    తిరిగి (constp ? constp->గమ్యం_SInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_పంక్తి* fl, VFlag_బిడ్డ_DType, Ast_కణుపు_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    బిడ్డ_DTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    స్థిరరాశి పూర్ణాంకం వెడల్పు = subDTypep()->వెడల్పు() * rangep->elementsConst();
    వెడల్పు_Force(వెడల్పు, వెడల్పు);
}
AstPackArrayDType::AstPackArrayDType(File_పంక్తి* fl, Ast_కణుపు_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    స్థిరరాశి పూర్ణాంకం వెడల్పు = subDTypep()->వెడల్పు() * rangep->elementsConst();
    వెడల్పు_Force(వెడల్పు, వెడల్పు);
}

పూర్ణాంకం Ast_వరుస_DType::boundConst() స్థిరరాశి VL_MT_STABLE {
    స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(boundp(), Const);
    తిరిగి (constp ? constp->గమ్యం_SInt() : 0);
}

AstTaskRef::AstTaskRef(File_పంక్తి* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtype_సమితి_Void();
}

పూర్ణాంకం AstBasicDType::hi() స్థిరరాశి { తిరిగి (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
పూర్ణాంకం AstBasicDType::lo() స్థిరరాశి { తిరిగి (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
పూర్ణాంకం AstBasicDType::elements() స్థిరరాశి {
    తిరిగి (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() స్థిరరాశి {
    తిరిగి (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool Ast_చురుకైన::hasClocked() స్థిరరాశి { తిరిగి m_sentreep->hasClocked(); }
bool Ast_చురుకైన::hasCombo() స్థిరరాశి { తిరిగి m_sentreep->hasCombo(); }
bool Ast_చురుకైన::hasInitial() స్థిరరాశి { తిరిగి m_sentreep->hasInitial(); }
bool Ast_చురుకైన::hasStatic() స్థిరరాశి { తిరిగి m_sentreep->hasStatic(); }

bool Ast_కణుపు_Module::isDollarUnit() స్థిరరాశి { తిరిగి this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_కణుపు_Module::isConstPool() స్థిరరాశి { తిరిగి this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(Ast_కేటాయించు_W* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_కేటాయించు} {}

AstElabDisplay::AstElabDisplay(File_పంక్తి* fl, VDisplayType dispType, Ast_కణుపు_సమాసం* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    కూడు_Fmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి {
    స్థిరరాశి AstVar* స్థిరరాశి asamep = VN_DBG_AS(samep, Var);
    తిరిగి m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

Ast_సరిపోలిక_Masked::Ast_సరిపోలిక_Masked(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_సరిపోలిక_Masked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtype_సమితి_UInt32();
}

AstVarRef::AstVarRef(File_పంక్తి* fl, AstVar* varp, స్థిరరాశి VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    ఒకవేళ (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } లేకపోతే ఒకవేళ (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_పంక్తి* fl, Ast_కణుపు_Module* pkgp, AstVar* varp, స్థిరరాశి VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_పంక్తి* fl, AstVarScope* varscp, స్థిరరాశి VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() స్థిరరాశి { తిరిగి varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_కణుపు(స్థిరరాశి AstVarRef* samep) స్థిరరాశి {
    ఒకవేళ (varScopep()) తిరిగి (varScopep() == samep->varScopep() && access() == samep->access());
    తిరిగి (self_సూచిక() == samep->self_సూచిక() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_కణుపు(samep->varp())));
}
bool AstVarRef::sameNoLvalue(స్థిరరాశి AstVarRef* samep) స్థిరరాశి {
    ఒకవేళ (varScopep()) తిరిగి (varScopep() == samep->varScopep());

    తిరిగి (self_సూచిక() == samep->self_సూచిక() && classOrPackagep() == samep->classOrPackagep()
            && (!self_సూచిక().is_ఖాళీలు() || !samep->self_సూచిక().is_ఖాళీలు())
            && varp()->same_కణుపు(samep->varp()));
}

AstVarXRef::AstVarXRef(File_పంక్తి* fl, AstVar* varp, స్థిరరాశి string& dotted, స్థిరరాశి VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_సమాసం* Ast_కణుపు_సమాసం::makeStmt() { తిరిగి new Ast_Stmt_సమాసం{fileline(), this}; }

Ast_Cons_Array_అంశం* AstConsArray::itemsp() స్థిరరాశి VL_MT_STABLE {
    తిరిగి VN_DBG_AS(op2p(), Cons_Array_అంశం);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
అంతఃవరుస Ast_కణుపు_Module* కనుగొను_మాతృ_Module(Ast_కణుపు* nodep) {
    కొరకు (Ast_కణుపు* curp = nodep; curp; curp = curp->backp()) {
        ఒకవేళ (Ast_కణుపు_Module* స్థిరరాశి modp = VN_CAST(curp, కణుపు_Module)) తిరిగి modp;
    }
    తిరిగి nullptr;
}

#అయితే_ముగింపు  // Guard
