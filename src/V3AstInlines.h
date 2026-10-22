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

#ifndef VERILATOR_V3ASTINLINES_H_
#define VERILATOR_V3ASTINLINES_H_

#ifndef VERILATOR_V3AST_H_
#error "Use V3Ast.h as the include"
#અન્તર્ભાવ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#endif

//######################################################################
// Inline METHODS

પૂર્ણાંક Ast_ગાંઠ::width() અચલ VL_MT_STABLE { પાછા dtypep() ? dtypep()->width() : 0; }
પૂર્ણાંક Ast_ગાંઠ::widthMin() અચલ VL_MT_STABLE { પાછા dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_ગાંઠ::width1() અચલ {  // V3Const uses to know it can optimize
    પાછા dtypep() && dtypep()->width() == 1;
}
પૂર્ણાંક Ast_ગાંઠ::widthInstrs() અચલ {
    પાછા (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_શબ્દો() : 1));
}
bool Ast_ગાંઠ::isCHandle() અચલ VL_MT_STABLE {
    પાછા dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_ગાંઠ::isDouble() અચલ VL_MT_STABLE {
    પાછા dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_ગાંઠ::isString() અચલ VL_MT_STABLE {
    પાછા dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_ગાંઠ::isEvent() અચલ VL_MT_STABLE {
    પાછા dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_ગાંઠ::isSigned() અચલ VL_MT_STABLE { પાછા dtypep() && dtypep()->isSigned(); }

bool Ast_ગાંઠ::is_Class_Handle_કિંમત() અચલ {
    પાછા (VN_IS(this, Const) && VN_AS(this, Const)->અંક().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_ગાંઠ::isNull() અચલ { પાછા VN_IS(this, Const) && VN_AS(this, Const)->અંક().isNull(); }
bool Ast_ગાંઠ::isZero() અચલ {
    પાછા (VN_IS(this, Const) && VN_AS(this, Const)->અંક().isEqZero());
}
bool Ast_ગાંઠ::isNeqZero() અચલ {
    પાછા (VN_IS(this, Const) && VN_AS(this, Const)->અંક().isNeqZero());
}
bool Ast_ગાંઠ::isOne() અચલ { પાછા (VN_IS(this, Const) && VN_AS(this, Const)->અંક().isEqOne()); }
bool Ast_ગાંઠ::isAllOnes() અચલ {
    પાછા (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_ગાંઠ::isAllOnesV() અચલ {
    પાછા (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_ગાંઠ::same_વૃક્ષ(અચલ Ast_ગાંઠ* node2p) અચલ {
    પાછા same_વૃક્ષ_Iter(this, node2p, true, false);
}
bool Ast_ગાંઠ::same_Gate_વૃક્ષ(અચલ Ast_ગાંઠ* node2p) અચલ {
    પાછા same_વૃક્ષ_Iter(this, node2p, true, true);
}

પૂર્ણાંક Ast_ગાંઠ_Array_DType::ડાબું() અચલ VL_MT_STABLE { પાછા rangep()->ડાબું_Const(); }
પૂર્ણાંક Ast_ગાંઠ_Array_DType::જમણું() અચલ VL_MT_STABLE { પાછા rangep()->જમણું_Const(); }
પૂર્ણાંક Ast_ગાંઠ_Array_DType::hi() અચલ VL_MT_STABLE { પાછા rangep()->hiConst(); }
પૂર્ણાંક Ast_ગાંઠ_Array_DType::lo() અચલ VL_MT_STABLE { પાછા rangep()->loConst(); }
પૂર્ણાંક Ast_ગાંઠ_Array_DType::elementsConst() અચલ VL_MT_STABLE { પાછા rangep()->elementsConst(); }
VNumRange Ast_ગાંઠ_Array_DType::declRange() અચલ VL_MT_STABLE { પાછા VNumRange{ડાબું(), જમણું()}; }

AstFuncRef::AstFuncRef(File_લીટી* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_લીટી* fl, પૂર્ણાંક ડાબું, પૂર્ણાંક જમણું)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(ડાબું)});
    rightp(new AstConst{fl, static_cast<uint32_t>(જમણું)});
}
AstRange::AstRange(File_લીટી* fl, અચલ VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.ડાબું())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.જમણું())});
}
પૂર્ણાંક AstRange::ડાબું_Const() અચલ VL_MT_STABLE {
    અચલ AstConst* અચલ constp = VN_CAST(leftp(), Const);
    પાછા (constp ? constp->toSInt() : 0);
}
પૂર્ણાંક AstRange::જમણું_Const() અચલ VL_MT_STABLE {
    અચલ AstConst* અચલ constp = VN_CAST(rightp(), Const);
    પાછા (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_લીટી* fl, VFlagChildDType, Ast_ગાંઠ_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    અચલ પૂર્ણાંક width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_લીટી* fl, Ast_ગાંઠ_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    અચલ પૂર્ણાંક width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

પૂર્ણાંક AstQueueDType::boundConst() અચલ VL_MT_STABLE {
    અચલ AstConst* અચલ constp = VN_CAST(boundp(), Const);
    પાછા (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(File_લીટી* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

પૂર્ણાંક AstBasicDType::hi() અચલ { પાછા (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
પૂર્ણાંક AstBasicDType::lo() અચલ { પાછા (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
પૂર્ણાંક AstBasicDType::elements() અચલ {
    પાછા (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() અચલ {
    પાછા (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() અચલ { પાછા m_sentreep->hasClocked(); }
bool AstActive::hasCombo() અચલ { પાછા m_sentreep->hasCombo(); }
bool AstActive::hasInitial() અચલ { પાછા m_sentreep->hasInitial(); }
bool AstActive::hasStatic() અચલ { પાછા m_sentreep->hasStatic(); }

bool Ast_ગાંઠ_Module::isDollarUnit() અચલ { પાછા this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_ગાંઠ_Module::isConstPool() અચલ { પાછા this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_લીટી* fl, VDisplayType dispType, Ast_ગાંઠ_પદાવલિ* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ {
    અચલ AstVar* અચલ asamep = VN_DBG_AS(samep, Var);
    પાછા m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_લીટી* fl, AstVar* varp, અચલ VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    જો (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } નહીંતર જો (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_લીટી* fl, Ast_ગાંઠ_Module* pkgp, AstVar* varp, અચલ VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_લીટી* fl, AstVarScope* varscp, અચલ VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() અચલ { પાછા varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_ગાંઠ(અચલ AstVarRef* samep) અચલ {
    જો (varScopep()) પાછા (varScopep() == samep->varScopep() && access() == samep->access());
    પાછા (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_ગાંઠ(samep->varp())));
}
bool AstVarRef::sameNoLvalue(અચલ AstVarRef* samep) અચલ {
    જો (varScopep()) પાછા (varScopep() == samep->varScopep());

    પાછા (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_ગાંઠ(samep->varp()));
}

AstVarXRef::AstVarXRef(File_લીટી* fl, AstVar* varp, અચલ string& dotted, અચલ VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_પદાવલિ* Ast_ગાંઠ_પદાવલિ::makeStmt() { પાછા new Ast_Stmt_પદાવલિ{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() અચલ VL_MT_STABLE {
    પાછા VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
અંતઃરેખ Ast_ગાંઠ_Module* findParentModule(Ast_ગાંઠ* nodep) {
    માટે (Ast_ગાંઠ* curp = nodep; curp; curp = curp->backp()) {
        જો (Ast_ગાંઠ_Module* અચલ modp = VN_CAST(curp, ગાંઠ_Module)) પાછા modp;
    }
    પાછા nullptr;
}

#endif  // Guard
