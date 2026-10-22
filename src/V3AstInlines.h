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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3ASTINLINES_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3ASTINLINES_H_

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3AST_H_
#ਗਲਤੀ "Use V3Ast.h as the include"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#ਜੇ_ਅੰਤ

//######################################################################
// Inline METHODS

ਪੂਰਨ Ast_ਗੰਢ::ਚੌੜਾਈ() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ dtypep() ? dtypep()->ਚੌੜਾਈ() : 0; }
ਪੂਰਨ Ast_ਗੰਢ::ਚੌੜਾਈ_Min() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ dtypep() ? dtypep()->ਚੌੜਾਈ_Min() : 0; }
bool Ast_ਗੰਢ::width1() ਅਚਲ {  // V3Const uses to know it can optimize
    ਵਾਪਸ dtypep() && dtypep()->ਚੌੜਾਈ() == 1;
}
ਪੂਰਨ Ast_ਗੰਢ::ਚੌੜਾਈ_Instrs() ਅਚਲ {
    ਵਾਪਸ (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->ਚੌੜਾਈ_ਸ਼ਬਦ() : 1));
}
bool Ast_ਗੰਢ::isCHandle() ਅਚਲ VL_MT_STABLE {
    ਵਾਪਸ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_ਗੰਢ::isDouble() ਅਚਲ VL_MT_STABLE {
    ਵਾਪਸ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_ਗੰਢ::isString() ਅਚਲ VL_MT_STABLE {
    ਵਾਪਸ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_ਗੰਢ::isEvent() ਅਚਲ VL_MT_STABLE {
    ਵਾਪਸ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_ਗੰਢ::isSigned() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ dtypep() && dtypep()->isSigned(); }

bool Ast_ਗੰਢ::is_Class_Handle_ਮੁੱਲ() ਅਚਲ {
    ਵਾਪਸ (VN_IS(this, Const) && VN_AS(this, Const)->ਅੰਕ().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_ਗੰਢ::isNull() ਅਚਲ { ਵਾਪਸ VN_IS(this, Const) && VN_AS(this, Const)->ਅੰਕ().isNull(); }
bool Ast_ਗੰਢ::isZero() ਅਚਲ {
    ਵਾਪਸ (VN_IS(this, Const) && VN_AS(this, Const)->ਅੰਕ().isEqZero());
}
bool Ast_ਗੰਢ::isNeqZero() ਅਚਲ {
    ਵਾਪਸ (VN_IS(this, Const) && VN_AS(this, Const)->ਅੰਕ().isNeqZero());
}
bool Ast_ਗੰਢ::isOne() ਅਚਲ { ਵਾਪਸ (VN_IS(this, Const) && VN_AS(this, Const)->ਅੰਕ().isEqOne()); }
bool Ast_ਗੰਢ::isAllOnes() ਅਚਲ {
    ਵਾਪਸ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_ਗੰਢ::isAllOnesV() ਅਚਲ {
    ਵਾਪਸ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_ਗੰਢ::same_ਰੁੱਖ(ਅਚਲ Ast_ਗੰਢ* node2p) ਅਚਲ {
    ਵਾਪਸ same_ਰੁੱਖ_Iter(this, node2p, true, false);
}
bool Ast_ਗੰਢ::same_Gate_ਰੁੱਖ(ਅਚਲ Ast_ਗੰਢ* node2p) ਅਚਲ {
    ਵਾਪਸ same_ਰੁੱਖ_Iter(this, node2p, true, true);
}

ਪੂਰਨ Ast_ਗੰਢ_Array_DType::ਖੱਬਾ() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ rangep()->ਖੱਬਾ_Const(); }
ਪੂਰਨ Ast_ਗੰਢ_Array_DType::ਸੱਜਾ() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ rangep()->ਸੱਜਾ_Const(); }
ਪੂਰਨ Ast_ਗੰਢ_Array_DType::hi() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ rangep()->hiConst(); }
ਪੂਰਨ Ast_ਗੰਢ_Array_DType::lo() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ rangep()->loConst(); }
ਪੂਰਨ Ast_ਗੰਢ_Array_DType::elementsConst() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ rangep()->elementsConst(); }
VNumRange Ast_ਗੰਢ_Array_DType::declRange() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ VNumRange{ਖੱਬਾ(), ਸੱਜਾ()}; }

AstFuncRef::AstFuncRef(File_ਸਤਰ* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_ਸਤਰ* fl, ਪੂਰਨ ਖੱਬਾ, ਪੂਰਨ ਸੱਜਾ)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(ਖੱਬਾ)});
    rightp(new AstConst{fl, static_cast<uint32_t>(ਸੱਜਾ)});
}
AstRange::AstRange(File_ਸਤਰ* fl, ਅਚਲ VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.ਖੱਬਾ())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.ਸੱਜਾ())});
}
ਪੂਰਨ AstRange::ਖੱਬਾ_Const() ਅਚਲ VL_MT_STABLE {
    ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(leftp(), Const);
    ਵਾਪਸ (constp ? constp->ਮੰਜ਼ਿਲ_SInt() : 0);
}
ਪੂਰਨ AstRange::ਸੱਜਾ_Const() ਅਚਲ VL_MT_STABLE {
    ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(rightp(), Const);
    ਵਾਪਸ (constp ? constp->ਮੰਜ਼ਿਲ_SInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_ਸਤਰ* fl, VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    ਬੱਚਾ_DTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = subDTypep()->ਚੌੜਾਈ() * rangep->elementsConst();
    ਚੌੜਾਈ_Force(ਚੌੜਾਈ, ਚੌੜਾਈ);
}
AstPackArrayDType::AstPackArrayDType(File_ਸਤਰ* fl, Ast_ਗੰਢ_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = subDTypep()->ਚੌੜਾਈ() * rangep->elementsConst();
    ਚੌੜਾਈ_Force(ਚੌੜਾਈ, ਚੌੜਾਈ);
}

ਪੂਰਨ Ast_ਕਤਾਰ_DType::boundConst() ਅਚਲ VL_MT_STABLE {
    ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(boundp(), Const);
    ਵਾਪਸ (constp ? constp->ਮੰਜ਼ਿਲ_SInt() : 0);
}

AstTaskRef::AstTaskRef(File_ਸਤਰ* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtype_ਸੈੱਟ_Void();
}

ਪੂਰਨ AstBasicDType::hi() ਅਚਲ { ਵਾਪਸ (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
ਪੂਰਨ AstBasicDType::lo() ਅਚਲ { ਵਾਪਸ (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
ਪੂਰਨ AstBasicDType::elements() ਅਚਲ {
    ਵਾਪਸ (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() ਅਚਲ {
    ਵਾਪਸ (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool Ast_ਸਰਗਰਮ::hasClocked() ਅਚਲ { ਵਾਪਸ m_sentreep->hasClocked(); }
bool Ast_ਸਰਗਰਮ::hasCombo() ਅਚਲ { ਵਾਪਸ m_sentreep->hasCombo(); }
bool Ast_ਸਰਗਰਮ::hasInitial() ਅਚਲ { ਵਾਪਸ m_sentreep->hasInitial(); }
bool Ast_ਸਰਗਰਮ::hasStatic() ਅਚਲ { ਵਾਪਸ m_sentreep->hasStatic(); }

bool Ast_ਗੰਢ_Module::isDollarUnit() ਅਚਲ { ਵਾਪਸ this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_ਗੰਢ_Module::isConstPool() ਅਚਲ { ਵਾਪਸ this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(Ast_ਸੌਂਪੋ_W* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ਸੌਂਪੋ} {}

AstElabDisplay::AstElabDisplay(File_ਸਤਰ* fl, VDisplayType dispType, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    ਜੋੜੋ_Fmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ {
    ਅਚਲ AstVar* ਅਚਲ asamep = VN_DBG_AS(samep, Var);
    ਵਾਪਸ m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

Ast_ਮੇਲ_Masked::Ast_ਮੇਲ_Masked(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_ਮੇਲ_Masked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtype_ਸੈੱਟ_UInt32();
}

AstVarRef::AstVarRef(File_ਸਤਰ* fl, AstVar* varp, ਅਚਲ VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    ਜੇ (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } ਨਹੀਂਤਾਂ ਜੇ (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_ਸਤਰ* fl, Ast_ਗੰਢ_Module* pkgp, AstVar* varp, ਅਚਲ VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_ਸਤਰ* fl, AstVarScope* varscp, ਅਚਲ VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ਅਚਲ { ਵਾਪਸ varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_ਗੰਢ(ਅਚਲ AstVarRef* samep) ਅਚਲ {
    ਜੇ (varScopep()) ਵਾਪਸ (varScopep() == samep->varScopep() && access() == samep->access());
    ਵਾਪਸ (self_ਸੂਚਕ() == samep->self_ਸੂਚਕ() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_ਗੰਢ(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ਅਚਲ AstVarRef* samep) ਅਚਲ {
    ਜੇ (varScopep()) ਵਾਪਸ (varScopep() == samep->varScopep());

    ਵਾਪਸ (self_ਸੂਚਕ() == samep->self_ਸੂਚਕ() && classOrPackagep() == samep->classOrPackagep()
            && (!self_ਸੂਚਕ().is_ਖਾਲੀ_ਥਾਵਾਂ() || !samep->self_ਸੂਚਕ().is_ਖਾਲੀ_ਥਾਵਾਂ())
            && varp()->same_ਗੰਢ(samep->varp()));
}

AstVarXRef::AstVarXRef(File_ਸਤਰ* fl, AstVar* varp, ਅਚਲ string& dotted, ਅਚਲ VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_ਵਿਅੰਜਕ* Ast_ਗੰਢ_ਵਿਅੰਜਕ::makeStmt() { ਵਾਪਸ new Ast_Stmt_ਵਿਅੰਜਕ{fileline(), this}; }

Ast_Cons_Array_ਵਸਤੂ* AstConsArray::itemsp() ਅਚਲ VL_MT_STABLE {
    ਵਾਪਸ VN_DBG_AS(op2p(), Cons_Array_ਵਸਤੂ);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
ਸਤਰਅੰਦਰ Ast_ਗੰਢ_Module* ਲੱਭੋ_ਮਾਪਾ_Module(Ast_ਗੰਢ* nodep) {
    ਲਈ (Ast_ਗੰਢ* curp = nodep; curp; curp = curp->backp()) {
        ਜੇ (Ast_ਗੰਢ_Module* ਅਚਲ modp = VN_CAST(curp, ਗੰਢ_Module)) ਵਾਪਸ modp;
    }
    ਵਾਪਸ nullptr;
}

#ਜੇ_ਅੰਤ  // Guard
