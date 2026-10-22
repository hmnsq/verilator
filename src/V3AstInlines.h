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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3ASTINLINES_H_
#وصف_ڪريو VERILATOR_V3ASTINLINES_H_

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3AST_H_
#غلطي "Use V3Ast.h as the include"
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#جيڪڏهن_پورو

//######################################################################
// Inline METHODS

صحيح Ast_ڳنڍ::ويڪر() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep()->ويڪر() : 0; }
صحيح Ast_ڳنڍ::ويڪر_Min() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep()->ويڪر_Min() : 0; }
bool Ast_ڳنڍ::width1() ثابت {  // V3Const uses to know it can optimize
    أرجع dtypep() && dtypep()->ويڪر() == 1;
}
صحيح Ast_ڳنڍ::ويڪر_Instrs() ثابت {
    أرجع (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->ويڪر_لفظ() : 1));
}
bool Ast_ڳنڍ::isCHandle() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_ڳنڍ::isDouble() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_ڳنڍ::isString() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_ڳنڍ::isEvent() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_ڳنڍ::isSigned() ثابت VL_MT_STABLE { أرجع dtypep() && dtypep()->isSigned(); }

bool Ast_ڳنڍ::is_Class_Handle_قدر() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->انگ().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_ڳنڍ::isNull() ثابت { أرجع VN_IS(this, Const) && VN_AS(this, Const)->انگ().isNull(); }
bool Ast_ڳنڍ::isZero() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->انگ().isEqZero());
}
bool Ast_ڳنڍ::isNeqZero() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->انگ().isNeqZero());
}
bool Ast_ڳنڍ::isOne() ثابت { أرجع (VN_IS(this, Const) && VN_AS(this, Const)->انگ().isEqOne()); }
bool Ast_ڳنڍ::isAllOnes() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_ڳنڍ::isAllOnesV() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_ڳنڍ::same_وڻ(ثابت Ast_ڳنڍ* node2p) ثابت {
    أرجع same_وڻ_Iter(this, node2p, true, false);
}
bool Ast_ڳنڍ::same_Gate_وڻ(ثابت Ast_ڳنڍ* node2p) ثابت {
    أرجع same_وڻ_Iter(this, node2p, true, true);
}

صحيح Ast_ڳنڍ_Array_DType::کاٻو() ثابت VL_MT_STABLE { أرجع rangep()->کاٻو_Const(); }
صحيح Ast_ڳنڍ_Array_DType::ساڄو() ثابت VL_MT_STABLE { أرجع rangep()->ساڄو_Const(); }
صحيح Ast_ڳنڍ_Array_DType::hi() ثابت VL_MT_STABLE { أرجع rangep()->hiConst(); }
صحيح Ast_ڳنڍ_Array_DType::lo() ثابت VL_MT_STABLE { أرجع rangep()->loConst(); }
صحيح Ast_ڳنڍ_Array_DType::elementsConst() ثابت VL_MT_STABLE { أرجع rangep()->elementsConst(); }
VNumRange Ast_ڳنڍ_Array_DType::declRange() ثابت VL_MT_STABLE { أرجع VNumRange{کاٻو(), ساڄو()}; }

AstFuncRef::AstFuncRef(File_سٽ* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_سٽ* fl, صحيح کاٻو, صحيح ساڄو)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(کاٻو)});
    rightp(new AstConst{fl, static_cast<uint32_t>(ساڄو)});
}
AstRange::AstRange(File_سٽ* fl, ثابت VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.کاٻو())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.ساڄو())});
}
صحيح AstRange::کاٻو_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(leftp(), Const);
    أرجع (constp ? constp->منزل_SInt() : 0);
}
صحيح AstRange::ساڄو_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(rightp(), Const);
    أرجع (constp ? constp->منزل_SInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_سٽ* fl, VFlag_ٻار_DType, Ast_ڳنڍ_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    ٻار_DTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ثابت صحيح ويڪر = subDTypep()->ويڪر() * rangep->elementsConst();
    ويڪر_Force(ويڪر, ويڪر);
}
AstPackArrayDType::AstPackArrayDType(File_سٽ* fl, Ast_ڳنڍ_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ثابت صحيح ويڪر = subDTypep()->ويڪر() * rangep->elementsConst();
    ويڪر_Force(ويڪر, ويڪر);
}

صحيح Ast_قطار_DType::boundConst() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(boundp(), Const);
    أرجع (constp ? constp->منزل_SInt() : 0);
}

AstTaskRef::AstTaskRef(File_سٽ* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtype_سيٽ_Void();
}

صحيح AstBasicDType::hi() ثابت { أرجع (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
صحيح AstBasicDType::lo() ثابت { أرجع (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
صحيح AstBasicDType::elements() ثابت {
    أرجع (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() ثابت {
    أرجع (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool Ast_سرگرم::hasClocked() ثابت { أرجع m_sentreep->hasClocked(); }
bool Ast_سرگرم::hasCombo() ثابت { أرجع m_sentreep->hasCombo(); }
bool Ast_سرگرم::hasInitial() ثابت { أرجع m_sentreep->hasInitial(); }
bool Ast_سرگرم::hasStatic() ثابت { أرجع m_sentreep->hasStatic(); }

bool Ast_ڳنڍ_Module::isDollarUnit() ثابت { أرجع this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_ڳنڍ_Module::isConstPool() ثابت { أرجع this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(Ast_سونپيو_W* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_سونپيو} {}

AstElabDisplay::AstElabDisplay(File_سٽ* fl, VDisplayType dispType, Ast_ڳنڍ_اظهار* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    جوڙيو_Fmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت {
    ثابت AstVar* ثابت asamep = VN_DBG_AS(samep, Var);
    أرجع m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

Ast_ميلاپ_Masked::Ast_ميلاپ_Masked(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_ميلاپ_Masked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtype_سيٽ_UInt32();
}

AstVarRef::AstVarRef(File_سٽ* fl, AstVar* varp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    إذا (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } وإلا إذا (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_سٽ* fl, Ast_ڳنڍ_Module* pkgp, AstVar* varp, ثابت VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_سٽ* fl, AstVarScope* varscp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ثابت { أرجع varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_ڳنڍ(ثابت AstVarRef* samep) ثابت {
    إذا (varScopep()) أرجع (varScopep() == samep->varScopep() && access() == samep->access());
    أرجع (self_اشارو() == samep->self_اشارو() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_ڳنڍ(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ثابت AstVarRef* samep) ثابت {
    إذا (varScopep()) أرجع (varScopep() == samep->varScopep());

    أرجع (self_اشارو() == samep->self_اشارو() && classOrPackagep() == samep->classOrPackagep()
            && (!self_اشارو().is_خالي_جايون() || !samep->self_اشارو().is_خالي_جايون())
            && varp()->same_ڳنڍ(samep->varp()));
}

AstVarXRef::AstVarXRef(File_سٽ* fl, AstVar* varp, ثابت string& dotted, ثابت VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_اظهار* Ast_ڳنڍ_اظهار::makeStmt() { أرجع new Ast_Stmt_اظهار{fileline(), this}; }

Ast_Cons_Array_شي* AstConsArray::itemsp() ثابت VL_MT_STABLE {
    أرجع VN_DBG_AS(op2p(), Cons_Array_شي);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
مضمن Ast_ڳنڍ_Module* ڳوليو_والد_Module(Ast_ڳنڍ* nodep) {
    لكل (Ast_ڳنڍ* curp = nodep; curp; curp = curp->backp()) {
        إذا (Ast_ڳنڍ_Module* ثابت modp = VN_CAST(curp, ڳنڍ_Module)) أرجع modp;
    }
    أرجع nullptr;
}

#جيڪڏهن_پورو  // Guard
