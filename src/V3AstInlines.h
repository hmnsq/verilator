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
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#endif

//######################################################################
// Inline METHODS

صحيح Ast_گرێ::width() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep()->width() : 0; }
صحيح Ast_گرێ::widthMin() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_گرێ::width1() ثابت {  // V3Const uses to know it can optimize
    أرجع dtypep() && dtypep()->width() == 1;
}
صحيح Ast_گرێ::widthInstrs() ثابت {
    أرجع (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_وشەکان() : 1));
}
bool Ast_گرێ::isCHandle() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_گرێ::isDouble() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_گرێ::isString() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_گرێ::isEvent() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_گرێ::isSigned() ثابت VL_MT_STABLE { أرجع dtypep() && dtypep()->isSigned(); }

bool Ast_گرێ::is_Class_Handle_بەها() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->ژمارە().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_گرێ::isNull() ثابت { أرجع VN_IS(this, Const) && VN_AS(this, Const)->ژمارە().isNull(); }
bool Ast_گرێ::isZero() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->ژمارە().isEqZero());
}
bool Ast_گرێ::isNeqZero() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->ژمارە().isNeqZero());
}
bool Ast_گرێ::isOne() ثابت { أرجع (VN_IS(this, Const) && VN_AS(this, Const)->ژمارە().isEqOne()); }
bool Ast_گرێ::isAllOnes() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_گرێ::isAllOnesV() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_گرێ::same_درەخت(ثابت Ast_گرێ* node2p) ثابت {
    أرجع same_درەخت_Iter(this, node2p, true, false);
}
bool Ast_گرێ::same_Gate_درەخت(ثابت Ast_گرێ* node2p) ثابت {
    أرجع same_درەخت_Iter(this, node2p, true, true);
}

صحيح Ast_گرێ_Array_DType::چەپ() ثابت VL_MT_STABLE { أرجع rangep()->چەپ_Const(); }
صحيح Ast_گرێ_Array_DType::ڕاست() ثابت VL_MT_STABLE { أرجع rangep()->ڕاست_Const(); }
صحيح Ast_گرێ_Array_DType::hi() ثابت VL_MT_STABLE { أرجع rangep()->hiConst(); }
صحيح Ast_گرێ_Array_DType::lo() ثابت VL_MT_STABLE { أرجع rangep()->loConst(); }
صحيح Ast_گرێ_Array_DType::elementsConst() ثابت VL_MT_STABLE { أرجع rangep()->elementsConst(); }
VNumRange Ast_گرێ_Array_DType::declRange() ثابت VL_MT_STABLE { أرجع VNumRange{چەپ(), ڕاست()}; }

AstFuncRef::AstFuncRef(File_هێڵ* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_هێڵ* fl, صحيح چەپ, صحيح ڕاست)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(چەپ)});
    rightp(new AstConst{fl, static_cast<uint32_t>(ڕاست)});
}
AstRange::AstRange(File_هێڵ* fl, ثابت VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.چەپ())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.ڕاست())});
}
صحيح AstRange::چەپ_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(leftp(), Const);
    أرجع (constp ? constp->toSInt() : 0);
}
صحيح AstRange::ڕاست_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(rightp(), Const);
    أرجع (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_هێڵ* fl, VFlagChildDType, Ast_گرێ_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ثابت صحيح width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_هێڵ* fl, Ast_گرێ_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ثابت صحيح width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

صحيح AstQueueDType::boundConst() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(boundp(), Const);
    أرجع (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(File_هێڵ* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

صحيح AstBasicDType::hi() ثابت { أرجع (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
صحيح AstBasicDType::lo() ثابت { أرجع (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
صحيح AstBasicDType::elements() ثابت {
    أرجع (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() ثابت {
    أرجع (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() ثابت { أرجع m_sentreep->hasClocked(); }
bool AstActive::hasCombo() ثابت { أرجع m_sentreep->hasCombo(); }
bool AstActive::hasInitial() ثابت { أرجع m_sentreep->hasInitial(); }
bool AstActive::hasStatic() ثابت { أرجع m_sentreep->hasStatic(); }

bool Ast_گرێ_Module::isDollarUnit() ثابت { أرجع this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_گرێ_Module::isConstPool() ثابت { أرجع this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_هێڵ* fl, VDisplayType dispType, Ast_گرێ_دەربڕین* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_گرێ(ثابت Ast_گرێ* samep) ثابت {
    ثابت AstVar* ثابت asamep = VN_DBG_AS(samep, Var);
    أرجع m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_هێڵ* fl, Ast_گرێ_دەربڕین* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_هێڵ* fl, AstVar* varp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    إذا (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } وإلا إذا (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_هێڵ* fl, Ast_گرێ_Module* pkgp, AstVar* varp, ثابت VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_هێڵ* fl, AstVarScope* varscp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ثابت { أرجع varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_گرێ(ثابت AstVarRef* samep) ثابت {
    إذا (varScopep()) أرجع (varScopep() == samep->varScopep() && access() == samep->access());
    أرجع (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_گرێ(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ثابت AstVarRef* samep) ثابت {
    إذا (varScopep()) أرجع (varScopep() == samep->varScopep());

    أرجع (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_گرێ(samep->varp()));
}

AstVarXRef::AstVarXRef(File_هێڵ* fl, AstVar* varp, ثابت string& dotted, ثابت VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_دەربڕین* Ast_گرێ_دەربڕین::makeStmt() { أرجع new Ast_Stmt_دەربڕین{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() ثابت VL_MT_STABLE {
    أرجع VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
مضمن Ast_گرێ_Module* findParentModule(Ast_گرێ* nodep) {
    لكل (Ast_گرێ* curp = nodep; curp; curp = curp->backp()) {
        إذا (Ast_گرێ_Module* ثابت modp = VN_CAST(curp, گرێ_Module)) أرجع modp;
    }
    أرجع nullptr;
}

#endif  // Guard
