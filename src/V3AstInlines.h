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

#که_نه_تعریف_شوی VERILATOR_V3ASTINLINES_H_
#تعریف_کړه VERILATOR_V3ASTINLINES_H_

#که_نه_تعریف_شوی VERILATOR_V3AST_H_
#تېروتنه "Use V3Ast.h as the include"
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#د_که_پای

//######################################################################
// Inline METHODS

صحيح Ast_غوټه::پلنوالی() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep()->پلنوالی() : 0; }
صحيح Ast_غوټه::پلنوالی_Min() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep()->پلنوالی_Min() : 0; }
bool Ast_غوټه::width1() ثابت {  // V3Const uses to know it can optimize
    أرجع dtypep() && dtypep()->پلنوالی() == 1;
}
صحيح Ast_غوټه::پلنوالی_Instrs() ثابت {
    أرجع (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->پلنوالی_ټکي() : 1));
}
bool Ast_غوټه::isCHandle() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_غوټه::isDouble() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_غوټه::isString() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_غوټه::isEvent() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_غوټه::isSigned() ثابت VL_MT_STABLE { أرجع dtypep() && dtypep()->isSigned(); }

bool Ast_غوټه::is_Class_Handle_ارزښت() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->عدد().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_غوټه::isNull() ثابت { أرجع VN_IS(this, Const) && VN_AS(this, Const)->عدد().isNull(); }
bool Ast_غوټه::isZero() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->عدد().isEqZero());
}
bool Ast_غوټه::isNeqZero() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->عدد().isNeqZero());
}
bool Ast_غوټه::isOne() ثابت { أرجع (VN_IS(this, Const) && VN_AS(this, Const)->عدد().isEqOne()); }
bool Ast_غوټه::isAllOnes() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_غوټه::isAllOnesV() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_غوټه::same_ونه(ثابت Ast_غوټه* node2p) ثابت {
    أرجع same_ونه_Iter(this, node2p, true, false);
}
bool Ast_غوټه::same_Gate_ونه(ثابت Ast_غوټه* node2p) ثابت {
    أرجع same_ونه_Iter(this, node2p, true, true);
}

صحيح Ast_غوټه_Array_DType::کیڼ_2() ثابت VL_MT_STABLE { أرجع rangep()->کیڼ_Const(); }
صحيح Ast_غوټه_Array_DType::ښی_2() ثابت VL_MT_STABLE { أرجع rangep()->ښی_Const(); }
صحيح Ast_غوټه_Array_DType::hi() ثابت VL_MT_STABLE { أرجع rangep()->hiConst(); }
صحيح Ast_غوټه_Array_DType::lo() ثابت VL_MT_STABLE { أرجع rangep()->loConst(); }
صحيح Ast_غوټه_Array_DType::elementsConst() ثابت VL_MT_STABLE { أرجع rangep()->elementsConst(); }
VNumRange Ast_غوټه_Array_DType::declRange() ثابت VL_MT_STABLE { أرجع VNumRange{کیڼ_2(), ښی_2()}; }

AstFuncRef::AstFuncRef(File_کرښه* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_کرښه* fl, صحيح کیڼ_2, صحيح ښی_2)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(کیڼ_2)});
    rightp(new AstConst{fl, static_cast<uint32_t>(ښی_2)});
}
AstRange::AstRange(File_کرښه* fl, ثابت VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.کیڼ_2())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.ښی_2())});
}
صحيح AstRange::کیڼ_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(leftp(), Const);
    أرجع (constp ? constp->منزل_SInt() : 0);
}
صحيح AstRange::ښی_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(rightp(), Const);
    أرجع (constp ? constp->منزل_SInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_کرښه* fl, VFlag_ماشوم_DType, Ast_غوټه_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    ماشوم_DTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ثابت صحيح پلنوالی = subDTypep()->پلنوالی() * rangep->elementsConst();
    پلنوالی_Force(پلنوالی, پلنوالی);
}
AstPackArrayDType::AstPackArrayDType(File_کرښه* fl, Ast_غوټه_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ثابت صحيح پلنوالی = subDTypep()->پلنوالی() * rangep->elementsConst();
    پلنوالی_Force(پلنوالی, پلنوالی);
}

صحيح Ast_کتار_DType::boundConst() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(boundp(), Const);
    أرجع (constp ? constp->منزل_SInt() : 0);
}

AstTaskRef::AstTaskRef(File_کرښه* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtype_سېټ_Void();
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

bool Ast_غوټه_Module::isDollarUnit() ثابت { أرجع this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_غوټه_Module::isConstPool() ثابت { أرجع this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(Ast_ټاکل_W* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ټاکل} {}

AstElabDisplay::AstElabDisplay(File_کرښه* fl, VDisplayType dispType, Ast_غوټه_بیان* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    جمع_کړه_Fmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_غوټه(ثابت Ast_غوټه* samep) ثابت {
    ثابت AstVar* ثابت asamep = VN_DBG_AS(samep, Var);
    أرجع m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

Ast_سمون_Masked::Ast_سمون_Masked(File_کرښه* fl, Ast_غوټه_بیان* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_سمون_Masked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtype_سېټ_UInt32();
}

AstVarRef::AstVarRef(File_کرښه* fl, AstVar* varp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    إذا (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } وإلا إذا (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_کرښه* fl, Ast_غوټه_Module* pkgp, AstVar* varp, ثابت VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_کرښه* fl, AstVarScope* varscp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ثابت { أرجع varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_غوټه(ثابت AstVarRef* samep) ثابت {
    إذا (varScopep()) أرجع (varScopep() == samep->varScopep() && access() == samep->access());
    أرجع (self_اشاره_ګر() == samep->self_اشاره_ګر() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_غوټه(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ثابت AstVarRef* samep) ثابت {
    إذا (varScopep()) أرجع (varScopep() == samep->varScopep());

    أرجع (self_اشاره_ګر() == samep->self_اشاره_ګر() && classOrPackagep() == samep->classOrPackagep()
            && (!self_اشاره_ګر().is_تشه() || !samep->self_اشاره_ګر().is_تشه())
            && varp()->same_غوټه(samep->varp()));
}

AstVarXRef::AstVarXRef(File_کرښه* fl, AstVar* varp, ثابت string& dotted, ثابت VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_بیان* Ast_غوټه_بیان::makeStmt() { أرجع new Ast_Stmt_بیان{fileline(), this}; }

Ast_Cons_Array_توکی* AstConsArray::itemsp() ثابت VL_MT_STABLE {
    أرجع VN_DBG_AS(op2p(), Cons_Array_توکی);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
مضمن Ast_غوټه_Module* پیدا_کړه_پلار_Module(Ast_غوټه* nodep) {
    لكل (Ast_غوټه* curp = nodep; curp; curp = curp->backp()) {
        إذا (Ast_غوټه_Module* ثابت modp = VN_CAST(curp, غوټه_Module)) أرجع modp;
    }
    أرجع nullptr;
}

#د_که_پای  // Guard
