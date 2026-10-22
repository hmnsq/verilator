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

صحيح Ast_نود::width() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep()->width() : 0; }
صحيح Ast_نود::widthMin() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_نود::width1() ثابت {  // V3Const uses to know it can optimize
    أرجع dtypep() && dtypep()->width() == 1;
}
صحيح Ast_نود::widthInstrs() ثابت {
    أرجع (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_ڤرکاتاءن() : 1));
}
bool Ast_نود::isCHandle() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_نود::isDouble() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_نود::isString() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_نود::isEvent() ثابت VL_MT_STABLE {
    أرجع dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_نود::isSigned() ثابت VL_MT_STABLE { أرجع dtypep() && dtypep()->isSigned(); }

bool Ast_نود::is_Class_Handle_نيلاي() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->اڠک().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_نود::isNull() ثابت { أرجع VN_IS(this, Const) && VN_AS(this, Const)->اڠک().isNull(); }
bool Ast_نود::isZero() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->اڠک().isEqZero());
}
bool Ast_نود::isNeqZero() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->اڠک().isNeqZero());
}
bool Ast_نود::isOne() ثابت { أرجع (VN_IS(this, Const) && VN_AS(this, Const)->اڠک().isEqOne()); }
bool Ast_نود::isAllOnes() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_نود::isAllOnesV() ثابت {
    أرجع (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_نود::same_ڤوکوق(ثابت Ast_نود* node2p) ثابت {
    أرجع same_ڤوکوق_Iter(this, node2p, true, false);
}
bool Ast_نود::same_Gate_ڤوکوق(ثابت Ast_نود* node2p) ثابت {
    أرجع same_ڤوکوق_Iter(this, node2p, true, true);
}

صحيح Ast_نود_Array_DType::کيري() ثابت VL_MT_STABLE { أرجع rangep()->کيري_Const(); }
صحيح Ast_نود_Array_DType::کانن() ثابت VL_MT_STABLE { أرجع rangep()->کانن_Const(); }
صحيح Ast_نود_Array_DType::hi() ثابت VL_MT_STABLE { أرجع rangep()->hiConst(); }
صحيح Ast_نود_Array_DType::lo() ثابت VL_MT_STABLE { أرجع rangep()->loConst(); }
صحيح Ast_نود_Array_DType::elementsConst() ثابت VL_MT_STABLE { أرجع rangep()->elementsConst(); }
VNumRange Ast_نود_Array_DType::declRange() ثابت VL_MT_STABLE { أرجع VNumRange{کيري(), کانن()}; }

AstFuncRef::AstFuncRef(File_باريس* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_باريس* fl, صحيح کيري, صحيح کانن)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(کيري)});
    rightp(new AstConst{fl, static_cast<uint32_t>(کانن)});
}
AstRange::AstRange(File_باريس* fl, ثابت VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.کيري())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.کانن())});
}
صحيح AstRange::کيري_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(leftp(), Const);
    أرجع (constp ? constp->toSInt() : 0);
}
صحيح AstRange::کانن_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(rightp(), Const);
    أرجع (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_باريس* fl, VFlagChildDType, Ast_نود_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ثابت صحيح width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_باريس* fl, Ast_نود_DType* dtp, AstRange* rangep)
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

AstTaskRef::AstTaskRef(File_باريس* fl, AstTask* taskp, AstArg* argsp)
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

bool Ast_نود_Module::isDollarUnit() ثابت { أرجع this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_نود_Module::isConstPool() ثابت { أرجع this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_باريس* fl, VDisplayType dispType, Ast_نود_اوڠکاڤن* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_نود(ثابت Ast_نود* samep) ثابت {
    ثابت AstVar* ثابت asamep = VN_DBG_AS(samep, Var);
    أرجع m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_باريس* fl, Ast_نود_اوڠکاڤن* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_باريس* fl, AstVar* varp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    إذا (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } وإلا إذا (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_باريس* fl, Ast_نود_Module* pkgp, AstVar* varp, ثابت VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_باريس* fl, AstVarScope* varscp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ثابت { أرجع varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_نود(ثابت AstVarRef* samep) ثابت {
    إذا (varScopep()) أرجع (varScopep() == samep->varScopep() && access() == samep->access());
    أرجع (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_نود(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ثابت AstVarRef* samep) ثابت {
    إذا (varScopep()) أرجع (varScopep() == samep->varScopep());

    أرجع (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_نود(samep->varp()));
}

AstVarXRef::AstVarXRef(File_باريس* fl, AstVar* varp, ثابت string& dotted, ثابت VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_اوڠکاڤن* Ast_نود_اوڠکاڤن::makeStmt() { أرجع new Ast_Stmt_اوڠکاڤن{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() ثابت VL_MT_STABLE {
    أرجع VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
مضمن Ast_نود_Module* findParentModule(Ast_نود* nodep) {
    لكل (Ast_نود* curp = nodep; curp; curp = curp->backp()) {
        إذا (Ast_نود_Module* ثابت modp = VN_CAST(curp, نود_Module)) أرجع modp;
    }
    أرجع nullptr;
}

#endif  // Guard
