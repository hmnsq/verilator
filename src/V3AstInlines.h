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

#اگر_غیر_تعریف_شدہ VERILATOR_V3ASTINLINES_H_
#تعریف_کرو VERILATOR_V3ASTINLINES_H_

#اگر_غیر_تعریف_شدہ VERILATOR_V3AST_H_
#غلطی "Use V3Ast.h as the include"
#شامل "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#اگر_ختم

//######################################################################
// Inline METHODS

صحیح Ast_گرہ::چوڑائی() ثابت VL_MT_STABLE { واپس dtypep() ? dtypep()->چوڑائی() : 0; }
صحیح Ast_گرہ::چوڑائی_Min() ثابت VL_MT_STABLE { واپس dtypep() ? dtypep()->چوڑائی_Min() : 0; }
bool Ast_گرہ::width1() ثابت {  // V3Const uses to know it can optimize
    واپس dtypep() && dtypep()->چوڑائی() == 1;
}
صحیح Ast_گرہ::چوڑائی_Instrs() ثابت {
    واپس (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->چوڑائی_الفاظ() : 1));
}
bool Ast_گرہ::isCHandle() ثابت VL_MT_STABLE {
    واپس dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_گرہ::isDouble() ثابت VL_MT_STABLE {
    واپس dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_گرہ::isString() ثابت VL_MT_STABLE {
    واپس dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_گرہ::isEvent() ثابت VL_MT_STABLE {
    واپس dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_گرہ::isSigned() ثابت VL_MT_STABLE { واپس dtypep() && dtypep()->isSigned(); }

bool Ast_گرہ::is_Class_Handle_قدر() ثابت {
    واپس (VN_IS(this, Const) && VN_AS(this, Const)->عدد().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_گرہ::isNull() ثابت { واپس VN_IS(this, Const) && VN_AS(this, Const)->عدد().isNull(); }
bool Ast_گرہ::isZero() ثابت {
    واپس (VN_IS(this, Const) && VN_AS(this, Const)->عدد().isEqZero());
}
bool Ast_گرہ::isNeqZero() ثابت {
    واپس (VN_IS(this, Const) && VN_AS(this, Const)->عدد().isNeqZero());
}
bool Ast_گرہ::isOne() ثابت { واپس (VN_IS(this, Const) && VN_AS(this, Const)->عدد().isEqOne()); }
bool Ast_گرہ::isAllOnes() ثابت {
    واپس (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_گرہ::isAllOnesV() ثابت {
    واپس (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_گرہ::same_درخت(ثابت Ast_گرہ* node2p) ثابت {
    واپس same_درخت_Iter(this, node2p, true, false);
}
bool Ast_گرہ::same_Gate_درخت(ثابت Ast_گرہ* node2p) ثابت {
    واپس same_درخت_Iter(this, node2p, true, true);
}

صحیح Ast_گرہ_Array_DType::بایاں() ثابت VL_MT_STABLE { واپس rangep()->بایاں_Const(); }
صحیح Ast_گرہ_Array_DType::دایاں() ثابت VL_MT_STABLE { واپس rangep()->دایاں_Const(); }
صحیح Ast_گرہ_Array_DType::hi() ثابت VL_MT_STABLE { واپس rangep()->hiConst(); }
صحیح Ast_گرہ_Array_DType::lo() ثابت VL_MT_STABLE { واپس rangep()->loConst(); }
صحیح Ast_گرہ_Array_DType::elementsConst() ثابت VL_MT_STABLE { واپس rangep()->elementsConst(); }
VNumRange Ast_گرہ_Array_DType::declRange() ثابت VL_MT_STABLE { واپس VNumRange{بایاں(), دایاں()}; }

AstFuncRef::AstFuncRef(File_سطر* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_سطر* fl, صحیح بایاں, صحیح دایاں)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(بایاں)});
    rightp(new AstConst{fl, static_cast<uint32_t>(دایاں)});
}
AstRange::AstRange(File_سطر* fl, ثابت VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.بایاں())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.دایاں())});
}
صحیح AstRange::بایاں_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(leftp(), Const);
    واپس (constp ? constp->مقصد_SInt() : 0);
}
صحیح AstRange::دایاں_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(rightp(), Const);
    واپس (constp ? constp->مقصد_SInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_سطر* fl, VFlag_بچہ_DType, Ast_گرہ_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    بچہ_DTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ثابت صحیح چوڑائی = subDTypep()->چوڑائی() * rangep->elementsConst();
    چوڑائی_Force(چوڑائی, چوڑائی);
}
AstPackArrayDType::AstPackArrayDType(File_سطر* fl, Ast_گرہ_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ثابت صحیح چوڑائی = subDTypep()->چوڑائی() * rangep->elementsConst();
    چوڑائی_Force(چوڑائی, چوڑائی);
}

صحیح Ast_قطار_DType::boundConst() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(boundp(), Const);
    واپس (constp ? constp->مقصد_SInt() : 0);
}

AstTaskRef::AstTaskRef(File_سطر* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtype_سیٹ_Void();
}

صحیح AstBasicDType::hi() ثابت { واپس (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
صحیح AstBasicDType::lo() ثابت { واپس (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
صحیح AstBasicDType::elements() ثابت {
    واپس (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() ثابت {
    واپس (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool Ast_فعال::hasClocked() ثابت { واپس m_sentreep->hasClocked(); }
bool Ast_فعال::hasCombo() ثابت { واپس m_sentreep->hasCombo(); }
bool Ast_فعال::hasInitial() ثابت { واپس m_sentreep->hasInitial(); }
bool Ast_فعال::hasStatic() ثابت { واپس m_sentreep->hasStatic(); }

bool Ast_گرہ_Module::isDollarUnit() ثابت { واپس this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_گرہ_Module::isConstPool() ثابت { واپس this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(Ast_تفویض_W* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_تفویض} {}

AstElabDisplay::AstElabDisplay(File_سطر* fl, VDisplayType dispType, Ast_گرہ_اظہاریہ* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    جوڑو_Fmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_گرہ(ثابت Ast_گرہ* samep) ثابت {
    ثابت AstVar* ثابت asamep = VN_DBG_AS(samep, Var);
    واپس m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

Ast_مطابقت_Masked::Ast_مطابقت_Masked(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_مطابقت_Masked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtype_سیٹ_UInt32();
}

AstVarRef::AstVarRef(File_سطر* fl, AstVar* varp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    اگر (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } ورنہ اگر (varp) {
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

string AstVarRef::name() ثابت { واپس varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_گرہ(ثابت AstVarRef* samep) ثابت {
    اگر (varScopep()) واپس (varScopep() == samep->varScopep() && access() == samep->access());
    واپس (self_اشارہ_گر() == samep->self_اشارہ_گر() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_گرہ(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ثابت AstVarRef* samep) ثابت {
    اگر (varScopep()) واپس (varScopep() == samep->varScopep());

    واپس (self_اشارہ_گر() == samep->self_اشارہ_گر() && classOrPackagep() == samep->classOrPackagep()
            && (!self_اشارہ_گر().is_خالی_خانے() || !samep->self_اشارہ_گر().is_خالی_خانے())
            && varp()->same_گرہ(samep->varp()));
}

AstVarXRef::AstVarXRef(File_سطر* fl, AstVar* varp, ثابت string& dotted, ثابت VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_اظہاریہ* Ast_گرہ_اظہاریہ::makeStmt() { واپس new Ast_Stmt_اظہاریہ{fileline(), this}; }

Ast_Cons_Array_شے* AstConsArray::itemsp() ثابت VL_MT_STABLE {
    واپس VN_DBG_AS(op2p(), Cons_Array_شے);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
اندرونی Ast_گرہ_Module* ڈھونڈو_والد_Module(Ast_گرہ* nodep) {
    برائے (Ast_گرہ* curp = nodep; curp; curp = curp->backp()) {
        اگر (Ast_گرہ_Module* ثابت modp = VN_CAST(curp, گرہ_Module)) واپس modp;
    }
    واپس nullptr;
}

#اگر_ختم  // Guard
