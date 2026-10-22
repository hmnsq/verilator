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

صحيح Ast_كلي::width() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep()->width() : 0; }
صحيح Ast_كلي::widthMin() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_كلي::width1() ثابت {  // V3Const uses to know it can optimize
    أرجع dtypep() && dtypep()->width() == 1;
}
صحيح Ast_كلي::widthInstrs() ثابت {
    أرجع (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_كلموم() : 1));
}
bool Ast_كلي::isCHandle() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_كلي::isDouble() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_كلي::isString() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_كلي::isEvent() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_كلي::isSigned() ثابت VL_MT_STABLE { أرجع dtypep() && dtypep()->isSigned(); }

bool Ast_كلي::is_Class_Handle_درج() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->لمب().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_كلي::isNull() ثابت { أرجع VN_IS(this, Const) && VN_AS(this, Const)->لمب().isNull(); }
bool Ast_كلي::isZero() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->لمب().isEqZero());
}
bool Ast_كلي::isNeqZero() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->لمب().isNeqZero());
}
bool Ast_كلي::isOne() ثابت { أرجع (VN_IS(this, Const) && VN_AS(this, Const)->لمب().isEqOne()); }
bool Ast_كلي::isAllOnes() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_كلي::isAllOnesV() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_كلي::same_بشيا(ثابت Ast_كلي* node2p) ثابت {
    أرجع same_بشيا_Iter(this, node2p, true, false);
}
bool Ast_كلي::same_Gate_بشيا(ثابت Ast_كلي* node2p) ثابت {
    أرجع same_بشيا_Iter(this, node2p, true, true);
}

صحيح Ast_كلي_Array_DType::هغو() ثابت VL_MT_STABLE { أرجع rangep()->هغو_Const(); }
صحيح Ast_كلي_Array_DType::دام() ثابت VL_MT_STABLE { أرجع rangep()->دام_Const(); }
صحيح Ast_كلي_Array_DType::hi() ثابت VL_MT_STABLE { أرجع rangep()->hiConst(); }
صحيح Ast_كلي_Array_DType::lo() ثابت VL_MT_STABLE { أرجع rangep()->loConst(); }
صحيح Ast_كلي_Array_DType::elementsConst() ثابت VL_MT_STABLE { أرجع rangep()->elementsConst(); }
VNumRange Ast_كلي_Array_DType::declRange() ثابت VL_MT_STABLE { أرجع VNumRange{هغو(), دام()}; }

AstFuncRef::AstFuncRef(File_لي* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_لي* fl, صحيح هغو, صحيح دام)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(هغو)});
    rightp(new AstConst{fl, static_cast<uint32_t>(دام)});
}
AstRange::AstRange(File_لي* fl, ثابت VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.هغو())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.دام())});
}
صحيح AstRange::هغو_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(leftp(), Const);
    أرجع (constp ? constp->toSInt() : 0);
}
صحيح AstRange::دام_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(rightp(), Const);
    أرجع (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_لي* fl, VFlagChildDType, Ast_كلي_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ثابت صحيح width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_لي* fl, Ast_كلي_DType* dtp, AstRange* rangep)
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

AstTaskRef::AstTaskRef(File_لي* fl, AstTask* taskp, AstArg* argsp)
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

bool Ast_كلي_Module::isDollarUnit() ثابت { أرجع this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_كلي_Module::isConstPool() ثابت { أرجع this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_لي* fl, VDisplayType dispType, Ast_كلي_فرچ* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_كلي(ثابت Ast_كلي* samep) ثابت {
    ثابت AstVar* ثابت asamep = VN_DBG_AS(samep, Var);
    أرجع m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_لي* fl, Ast_كلي_فرچ* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_لي* fl, AstVar* varp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    إذا (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } وإلا إذا (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_لي* fl, Ast_كلي_Module* pkgp, AstVar* varp, ثابت VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_لي* fl, AstVarScope* varscp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ثابت { أرجع varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_كلي(ثابت AstVarRef* samep) ثابت {
    إذا (varScopep()) أرجع (varScopep() == samep->varScopep() && access() == samep->access());
    أرجع (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_كلي(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ثابت AstVarRef* samep) ثابت {
    إذا (varScopep()) أرجع (varScopep() == samep->varScopep());

    أرجع (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_كلي(samep->varp()));
}

AstVarXRef::AstVarXRef(File_لي* fl, AstVar* varp, ثابت string& dotted, ثابت VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_فرچ* Ast_كلي_فرچ::makeStmt() { أرجع new Ast_Stmt_فرچ{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() ثابت VL_MT_STABLE {
    أرجع VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
مضمن Ast_كلي_Module* findParentModule(Ast_كلي* nodep) {
    لكل (Ast_كلي* curp = nodep; curp; curp = curp->backp()) {
        إذا (Ast_كلي_Module* ثابت modp = VN_CAST(curp, كلي_Module)) أرجع modp;
    }
    أرجع nullptr;
}

#endif  // Guard
