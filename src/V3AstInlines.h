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
#شامل "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#endif

//######################################################################
// Inline METHODS

صحیح Ast_گره::width() ثابت VL_MT_STABLE { بازگشت dtypep() ? dtypep()->width() : 0; }
صحیح Ast_گره::widthMin() ثابت VL_MT_STABLE { بازگشت dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_گره::width1() ثابت {  // V3Const uses to know it can optimize
    بازگشت dtypep() && dtypep()->width() == 1;
}
صحیح Ast_گره::widthInstrs() ثابت {
    بازگشت (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_واژگان() : 1));
}
bool Ast_گره::isCHandle() ثابت VL_MT_STABLE {
    بازگشت dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_گره::isDouble() ثابت VL_MT_STABLE {
    بازگشت dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_گره::isString() ثابت VL_MT_STABLE {
    بازگشت dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_گره::isEvent() ثابت VL_MT_STABLE {
    بازگشت dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_گره::isSigned() ثابت VL_MT_STABLE { بازگشت dtypep() && dtypep()->isSigned(); }

bool Ast_گره::is_Class_Handle_مقدار() ثابت {
    بازگشت (VN_IS(this, Const) && VN_AS(this, Const)->رقم().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_گره::isNull() ثابت { بازگشت VN_IS(this, Const) && VN_AS(this, Const)->رقم().isNull(); }
bool Ast_گره::isZero() ثابت {
    بازگشت (VN_IS(this, Const) && VN_AS(this, Const)->رقم().isEqZero());
}
bool Ast_گره::isNeqZero() ثابت {
    بازگشت (VN_IS(this, Const) && VN_AS(this, Const)->رقم().isNeqZero());
}
bool Ast_گره::isOne() ثابت { بازگشت (VN_IS(this, Const) && VN_AS(this, Const)->رقم().isEqOne()); }
bool Ast_گره::isAllOnes() ثابت {
    بازگشت (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_گره::isAllOnesV() ثابت {
    بازگشت (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_گره::same_درخت(ثابت Ast_گره* node2p) ثابت {
    بازگشت same_درخت_Iter(this, node2p, true, false);
}
bool Ast_گره::same_Gate_درخت(ثابت Ast_گره* node2p) ثابت {
    بازگشت same_درخت_Iter(this, node2p, true, true);
}

صحیح Ast_گره_Array_DType::چپ() ثابت VL_MT_STABLE { بازگشت rangep()->چپ_Const(); }
صحیح Ast_گره_Array_DType::راست() ثابت VL_MT_STABLE { بازگشت rangep()->راست_Const(); }
صحیح Ast_گره_Array_DType::hi() ثابت VL_MT_STABLE { بازگشت rangep()->hiConst(); }
صحیح Ast_گره_Array_DType::lo() ثابت VL_MT_STABLE { بازگشت rangep()->loConst(); }
صحیح Ast_گره_Array_DType::elementsConst() ثابت VL_MT_STABLE { بازگشت rangep()->elementsConst(); }
VNumRange Ast_گره_Array_DType::declRange() ثابت VL_MT_STABLE { بازگشت VNumRange{چپ(), راست()}; }

AstFuncRef::AstFuncRef(File_سطر* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_سطر* fl, صحیح چپ, صحیح راست)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(چپ)});
    rightp(new AstConst{fl, static_cast<uint32_t>(راست)});
}
AstRange::AstRange(File_سطر* fl, ثابت VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.چپ())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.راست())});
}
صحیح AstRange::چپ_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(leftp(), Const);
    بازگشت (constp ? constp->toSInt() : 0);
}
صحیح AstRange::راست_Const() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(rightp(), Const);
    بازگشت (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_سطر* fl, VFlagChildDType, Ast_گره_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ثابت صحیح width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_سطر* fl, Ast_گره_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ثابت صحیح width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

صحیح AstQueueDType::boundConst() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(boundp(), Const);
    بازگشت (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(File_سطر* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

صحیح AstBasicDType::hi() ثابت { بازگشت (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
صحیح AstBasicDType::lo() ثابت { بازگشت (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
صحیح AstBasicDType::elements() ثابت {
    بازگشت (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() ثابت {
    بازگشت (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() ثابت { بازگشت m_sentreep->hasClocked(); }
bool AstActive::hasCombo() ثابت { بازگشت m_sentreep->hasCombo(); }
bool AstActive::hasInitial() ثابت { بازگشت m_sentreep->hasInitial(); }
bool AstActive::hasStatic() ثابت { بازگشت m_sentreep->hasStatic(); }

bool Ast_گره_Module::isDollarUnit() ثابت { بازگشت this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_گره_Module::isConstPool() ثابت { بازگشت this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_سطر* fl, VDisplayType dispType, Ast_گره_عبارت* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_گره(ثابت Ast_گره* samep) ثابت {
    ثابت AstVar* ثابت asamep = VN_DBG_AS(samep, Var);
    بازگشت m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_سطر* fl, Ast_گره_عبارت* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_سطر* fl, AstVar* varp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    اگر (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } وگرنه اگر (varp) {
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

string AstVarRef::name() ثابت { بازگشت varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_گره(ثابت AstVarRef* samep) ثابت {
    اگر (varScopep()) بازگشت (varScopep() == samep->varScopep() && access() == samep->access());
    بازگشت (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_گره(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ثابت AstVarRef* samep) ثابت {
    اگر (varScopep()) بازگشت (varScopep() == samep->varScopep());

    بازگشت (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_گره(samep->varp()));
}

AstVarXRef::AstVarXRef(File_سطر* fl, AstVar* varp, ثابت string& dotted, ثابت VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_عبارت* Ast_گره_عبارت::makeStmt() { بازگشت new Ast_Stmt_عبارت{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() ثابت VL_MT_STABLE {
    بازگشت VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
درونخطی Ast_گره_Module* findParentModule(Ast_گره* nodep) {
    برای (Ast_گره* curp = nodep; curp; curp = curp->backp()) {
        اگر (Ast_گره_Module* ثابت modp = VN_CAST(curp, گره_Module)) بازگشت modp;
    }
    بازگشت nullptr;
}

#endif  // Guard
