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

#جے_بنا_تعریف VERILATOR_V3ASTINLINES_H_
#تعریف_کرو VERILATOR_V3ASTINLINES_H_

#جے_بنا_تعریف VERILATOR_V3AST_H_
#غلطی "Use V3Ast.h as the include"
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#جے_انت

//######################################################################
// Inline METHODS

صحيح Ast_گرہ::چوڑائی() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep()->چوڑائی() : 0; }
صحيح Ast_گرہ::چوڑائی_Min() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep()->چوڑائی_Min() : 0; }
bool Ast_گرہ::width1() ثابت {  // V3Const uses to know it can optimize
    أرجع dtypep() && dtypep()->چوڑائی() == 1;
}
صحيح Ast_گرہ::چوڑائی_Instrs() ثابت {
    أرجع (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->چوڑائی_الفاظ() : 1));
}
bool Ast_گرہ::isCHandle() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_گرہ::isDouble() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_گرہ::isString() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_گرہ::isEvent() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_گرہ::isSigned() ثابت VL_MT_STABLE { أرجع dtypep() && dtypep()->isSigned(); }

bool Ast_گرہ::is_Class_Handle_قدر() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->عدد().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_گرہ::isNull() ثابت { أرجع VN_IS(this, Const) && VN_AS(this, Const)->عدد().isNull(); }
bool Ast_گرہ::isZero() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->عدد().isEqZero());
}
bool Ast_گرہ::isNeqZero() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->عدد().isNeqZero());
}
bool Ast_گرہ::isOne() ثابت { أرجع (VN_IS(this, Const) && VN_AS(this, Const)->عدد().isEqOne()); }
bool Ast_گرہ::isAllOnes() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_گرہ::isAllOnesV() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_گرہ::same_درخت(ثابت Ast_گرہ* node2p) ثابت {
    أرجع same_درخت_Iter(this, node2p, true, false);
}
bool Ast_گرہ::same_Gate_درخت(ثابت Ast_گرہ* node2p) ثابت {
    أرجع same_درخت_Iter(this, node2p, true, true);
}

صحيح Ast_گرہ_Array_DType::بایاں() ثابت VL_MT_STABLE { أرجع rangep()->بایاں_Const(); }
صحيح Ast_گرہ_Array_DType::دایاں() ثابت VL_MT_STABLE { أرجع rangep()->دایاں_Const(); }
صحيح Ast_گرہ_Array_DType::hi() ثابت VL_MT_STABLE { أرجع rangep()->hiConst(); }
صحيح Ast_گرہ_Array_DType::lo() ثابت VL_MT_STABLE { أرجع rangep()->loConst(); }
صحيح Ast_گرہ_Array_DType::elementsConst() ثابت VL_MT_STABLE { أرجع rangep()->elementsConst(); }
VNumRange Ast_گرہ_Array_DType::declRange() ثابت VL_MT_STABLE { أرجع VNumRange{بایاں(), دایاں()}; }

AstFuncRef::AstFuncRef(File_سطر* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_سطر* fl, صحيح بایاں, صحيح دایاں)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(بایاں)});
    rightp(new AstConst{fl, static_cast<uint32_t>(دایاں)});
}
AstRange::AstRange(File_سطر* fl, ثابت VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.بایاں())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.دایاں())});
}
صحيح AstRange::بایاں_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(leftp(), Const);
    أرجع (constp ? constp->منزل_SInt() : 0);
}
صحيح AstRange::دایاں_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(rightp(), Const);
    أرجع (constp ? constp->منزل_SInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_سطر* fl, VFlag_بچہ_DType, Ast_گرہ_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    بچہ_DTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ثابت صحيح چوڑائی = subDTypep()->چوڑائی() * rangep->elementsConst();
    چوڑائی_Force(چوڑائی, چوڑائی);
}
AstPackArrayDType::AstPackArrayDType(File_سطر* fl, Ast_گرہ_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ثابت صحيح چوڑائی = subDTypep()->چوڑائی() * rangep->elementsConst();
    چوڑائی_Force(چوڑائی, چوڑائی);
}

صحيح Ast_قطار_DType::boundConst() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(boundp(), Const);
    أرجع (constp ? constp->منزل_SInt() : 0);
}

AstTaskRef::AstTaskRef(File_سطر* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtype_سیٹ_Void();
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

bool Ast_گرہ_Module::isDollarUnit() ثابت { أرجع this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_گرہ_Module::isConstPool() ثابت { أرجع this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(Ast_سونپو_W* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_سونپو} {}

AstElabDisplay::AstElabDisplay(File_سطر* fl, VDisplayType dispType, Ast_گرہ_اظہاریہ* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    جوڑو_Fmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_گرہ(ثابت Ast_گرہ* samep) ثابت {
    ثابت AstVar* ثابت asamep = VN_DBG_AS(samep, Var);
    أرجع m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

Ast_میل_Masked::Ast_میل_Masked(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_میل_Masked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtype_سیٹ_UInt32();
}

AstVarRef::AstVarRef(File_سطر* fl, AstVar* varp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    إذا (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } وإلا إذا (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_سطر* fl, Ast_گرہ_Module* pkgp, AstVar* varp, ثابت VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_سطر* fl, AstVarScope* varscp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ثابت { أرجع varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_گرہ(ثابت AstVarRef* samep) ثابت {
    إذا (varScopep()) أرجع (varScopep() == samep->varScopep() && access() == samep->access());
    أرجع (self_اشارہ_گر() == samep->self_اشارہ_گر() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_گرہ(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ثابت AstVarRef* samep) ثابت {
    إذا (varScopep()) أرجع (varScopep() == samep->varScopep());

    أرجع (self_اشارہ_گر() == samep->self_اشارہ_گر() && classOrPackagep() == samep->classOrPackagep()
            && (!self_اشارہ_گر().is_خالی_تھاواں() || !samep->self_اشارہ_گر().is_خالی_تھاواں())
            && varp()->same_گرہ(samep->varp()));
}

AstVarXRef::AstVarXRef(File_سطر* fl, AstVar* varp, ثابت string& dotted, ثابت VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_اظہاریہ* Ast_گرہ_اظہاریہ::makeStmt() { أرجع new Ast_Stmt_اظہاریہ{fileline(), this}; }

Ast_Cons_Array_شے* AstConsArray::itemsp() ثابت VL_MT_STABLE {
    أرجع VN_DBG_AS(op2p(), Cons_Array_شے);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
مضمن Ast_گرہ_Module* لبھو_ماپا_Module(Ast_گرہ* nodep) {
    لكل (Ast_گرہ* curp = nodep; curp; curp = curp->backp()) {
        إذا (Ast_گرہ_Module* ثابت modp = VN_CAST(curp, گرہ_Module)) أرجع modp;
    }
    أرجع nullptr;
}

#جے_انت  // Guard
