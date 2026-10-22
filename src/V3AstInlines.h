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

#اگر_تعریف_نشده VERILATOR_V3ASTINLINES_H_
#تعریف_کن VERILATOR_V3ASTINLINES_H_

#اگر_تعریف_نشده VERILATOR_V3AST_H_
#خطا "Use V3Ast.h as the include"
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#پایان_اگر

//######################################################################
// Inline METHODS

صحيح Ast_گره::عرض() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep()->عرض() : 0; }
صحيح Ast_گره::عرض_Min() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep()->عرض_Min() : 0; }
bool Ast_گره::width1() ثابت {  // V3Const uses to know it can optimize
    أرجع dtypep() && dtypep()->عرض() == 1;
}
صحيح Ast_گره::عرض_Instrs() ثابت {
    أرجع (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->عرض_واژگان() : 1));
}
bool Ast_گره::isCHandle() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_گره::isDouble() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_گره::isString() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_گره::isEvent() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_گره::isSigned() ثابت VL_MT_STABLE { أرجع dtypep() && dtypep()->isSigned(); }

bool Ast_گره::is_Class_Handle_مقدار() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->رقم().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_گره::isNull() ثابت { أرجع VN_IS(this, Const) && VN_AS(this, Const)->رقم().isNull(); }
bool Ast_گره::isZero() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->رقم().isEqZero());
}
bool Ast_گره::isNeqZero() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->رقم().isNeqZero());
}
bool Ast_گره::isOne() ثابت { أرجع (VN_IS(this, Const) && VN_AS(this, Const)->رقم().isEqOne()); }
bool Ast_گره::isAllOnes() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_گره::isAllOnesV() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_گره::same_درخت(ثابت Ast_گره* node2p) ثابت {
    أرجع same_درخت_Iter(this, node2p, true, false);
}
bool Ast_گره::same_Gate_درخت(ثابت Ast_گره* node2p) ثابت {
    أرجع same_درخت_Iter(this, node2p, true, true);
}

صحيح Ast_گره_Array_DType::چپ_2() ثابت VL_MT_STABLE { أرجع rangep()->چپ_Const(); }
صحيح Ast_گره_Array_DType::راست_2() ثابت VL_MT_STABLE { أرجع rangep()->راست_Const(); }
صحيح Ast_گره_Array_DType::hi() ثابت VL_MT_STABLE { أرجع rangep()->hiConst(); }
صحيح Ast_گره_Array_DType::lo() ثابت VL_MT_STABLE { أرجع rangep()->loConst(); }
صحيح Ast_گره_Array_DType::elementsConst() ثابت VL_MT_STABLE { أرجع rangep()->elementsConst(); }
VNumRange Ast_گره_Array_DType::declRange() ثابت VL_MT_STABLE { أرجع VNumRange{چپ_2(), راست_2()}; }

AstFuncRef::AstFuncRef(File_سطر* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_سطر* fl, صحيح چپ_2, صحيح راست_2)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(چپ_2)});
    rightp(new AstConst{fl, static_cast<uint32_t>(راست_2)});
}
AstRange::AstRange(File_سطر* fl, ثابت VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.چپ_2())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.راست_2())});
}
صحيح AstRange::چپ_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(leftp(), Const);
    أرجع (constp ? constp->مقصد_SInt() : 0);
}
صحيح AstRange::راست_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(rightp(), Const);
    أرجع (constp ? constp->مقصد_SInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_سطر* fl, VFlag_فرزند_DType, Ast_گره_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    فرزند_DTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ثابت صحيح عرض = subDTypep()->عرض() * rangep->elementsConst();
    عرض_Force(عرض, عرض);
}
AstPackArrayDType::AstPackArrayDType(File_سطر* fl, Ast_گره_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ثابت صحيح عرض = subDTypep()->عرض() * rangep->elementsConst();
    عرض_Force(عرض, عرض);
}

صحيح Ast_صف_DType::boundConst() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(boundp(), Const);
    أرجع (constp ? constp->مقصد_SInt() : 0);
}

AstTaskRef::AstTaskRef(File_سطر* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtype_مجموعه_Void();
}

صحيح AstBasicDType::hi() ثابت { أرجع (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
صحيح AstBasicDType::lo() ثابت { أرجع (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
صحيح AstBasicDType::elements() ثابت {
    أرجع (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() ثابت {
    أرجع (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool Ast_فعال::hasClocked() ثابت { أرجع m_sentreep->hasClocked(); }
bool Ast_فعال::hasCombo() ثابت { أرجع m_sentreep->hasCombo(); }
bool Ast_فعال::hasInitial() ثابت { أرجع m_sentreep->hasInitial(); }
bool Ast_فعال::hasStatic() ثابت { أرجع m_sentreep->hasStatic(); }

bool Ast_گره_Module::isDollarUnit() ثابت { أرجع this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_گره_Module::isConstPool() ثابت { أرجع this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(Ast_تخصیص_بده_W* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_تخصیص_بده} {}

AstElabDisplay::AstElabDisplay(File_سطر* fl, VDisplayType dispType, Ast_گره_عبارت* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    جمع_کن_Fmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_گره(ثابت Ast_گره* samep) ثابت {
    ثابت AstVar* ثابت asamep = VN_DBG_AS(samep, Var);
    أرجع m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

Ast_تطابق_Masked::Ast_تطابق_Masked(File_سطر* fl, Ast_گره_عبارت* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_تطابق_Masked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtype_مجموعه_UInt32();
}

AstVarRef::AstVarRef(File_سطر* fl, AstVar* varp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    إذا (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } وإلا إذا (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_سطر* fl, Ast_گره_Module* pkgp, AstVar* varp, ثابت VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_سطر* fl, AstVarScope* varscp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ثابت { أرجع varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_گره(ثابت AstVarRef* samep) ثابت {
    إذا (varScopep()) أرجع (varScopep() == samep->varScopep() && access() == samep->access());
    أرجع (self_اشاره_گر() == samep->self_اشاره_گر() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_گره(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ثابت AstVarRef* samep) ثابت {
    إذا (varScopep()) أرجع (varScopep() == samep->varScopep());

    أرجع (self_اشاره_گر() == samep->self_اشاره_گر() && classOrPackagep() == samep->classOrPackagep()
            && (!self_اشاره_گر().is_خالی() || !samep->self_اشاره_گر().is_خالی())
            && varp()->same_گره(samep->varp()));
}

AstVarXRef::AstVarXRef(File_سطر* fl, AstVar* varp, ثابت string& dotted, ثابت VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_عبارت* Ast_گره_عبارت::makeStmt() { أرجع new Ast_Stmt_عبارت{fileline(), this}; }

Ast_Cons_Array_قلم* AstConsArray::itemsp() ثابت VL_MT_STABLE {
    أرجع VN_DBG_AS(op2p(), Cons_Array_قلم);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
مضمن Ast_گره_Module* پیدا_کن_والد_Module(Ast_گره* nodep) {
    لكل (Ast_گره* curp = nodep; curp; curp = curp->backp()) {
        إذا (Ast_گره_Module* ثابت modp = VN_CAST(curp, گره_Module)) أرجع modp;
    }
    أرجع nullptr;
}

#پایان_اگر  // Guard
