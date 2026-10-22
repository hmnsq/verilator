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
#अन्तर्भाव "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#endif

//######################################################################
// Inline METHODS

पूर्णाङ्क Ast_ग्रन्थिः::width() ध्रुव VL_MT_STABLE { पटिनिवत्त dtypep() ? dtypep()->width() : 0; }
पूर्णाङ्क Ast_ग्रन्थिः::widthMin() ध्रुव VL_MT_STABLE { पटिनिवत्त dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_ग्रन्थिः::width1() ध्रुव {  // V3Const uses to know it can optimize
    पटिनिवत्त dtypep() && dtypep()->width() == 1;
}
पूर्णाङ्क Ast_ग्रन्थिः::widthInstrs() ध्रुव {
    पटिनिवत्त (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_शब्दाः() : 1));
}
bool Ast_ग्रन्थिः::isCHandle() ध्रुव VL_MT_STABLE {
    पटिनिवत्त dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_ग्रन्थिः::isDouble() ध्रुव VL_MT_STABLE {
    पटिनिवत्त dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_ग्रन्थिः::isString() ध्रुव VL_MT_STABLE {
    पटिनिवत्त dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_ग्रन्थिः::isEvent() ध्रुव VL_MT_STABLE {
    पटिनिवत्त dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_ग्रन्थिः::isSigned() ध्रुव VL_MT_STABLE { पटिनिवत्त dtypep() && dtypep()->isSigned(); }

bool Ast_ग्रन्थिः::is_Class_Handle_मानम्() ध्रुव {
    पटिनिवत्त (VN_IS(this, Const) && VN_AS(this, Const)->अङ्कः().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_ग्रन्थिः::isNull() ध्रुव { पटिनिवत्त VN_IS(this, Const) && VN_AS(this, Const)->अङ्कः().isNull(); }
bool Ast_ग्रन्थिः::isZero() ध्रुव {
    पटिनिवत्त (VN_IS(this, Const) && VN_AS(this, Const)->अङ्कः().isEqZero());
}
bool Ast_ग्रन्थिः::isNeqZero() ध्रुव {
    पटिनिवत्त (VN_IS(this, Const) && VN_AS(this, Const)->अङ्कः().isNeqZero());
}
bool Ast_ग्रन्थिः::isOne() ध्रुव { पटिनिवत्त (VN_IS(this, Const) && VN_AS(this, Const)->अङ्कः().isEqOne()); }
bool Ast_ग्रन्थिः::isAllOnes() ध्रुव {
    पटिनिवत्त (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_ग्रन्थिः::isAllOnesV() ध्रुव {
    पटिनिवत्त (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_ग्रन्थिः::same_वृक्षः(ध्रुव Ast_ग्रन्थिः* node2p) ध्रुव {
    पटिनिवत्त same_वृक्षः_Iter(this, node2p, true, false);
}
bool Ast_ग्रन्थिः::same_Gate_वृक्षः(ध्रुव Ast_ग्रन्थिः* node2p) ध्रुव {
    पटिनिवत्त same_वृक्षः_Iter(this, node2p, true, true);
}

पूर्णाङ्क Ast_ग्रन्थिः_Array_DType::वामः() ध्रुव VL_MT_STABLE { पटिनिवत्त rangep()->वामः_Const(); }
पूर्णाङ्क Ast_ग्रन्थिः_Array_DType::दक्षिणः() ध्रुव VL_MT_STABLE { पटिनिवत्त rangep()->दक्षिणः_Const(); }
पूर्णाङ्क Ast_ग्रन्थिः_Array_DType::hi() ध्रुव VL_MT_STABLE { पटिनिवत्त rangep()->hiConst(); }
पूर्णाङ्क Ast_ग्रन्थिः_Array_DType::lo() ध्रुव VL_MT_STABLE { पटिनिवत्त rangep()->loConst(); }
पूर्णाङ्क Ast_ग्रन्थिः_Array_DType::elementsConst() ध्रुव VL_MT_STABLE { पटिनिवत्त rangep()->elementsConst(); }
VNumRange Ast_ग्रन्थिः_Array_DType::declRange() ध्रुव VL_MT_STABLE { पटिनिवत्त VNumRange{वामः(), दक्षिणः()}; }

AstFuncRef::AstFuncRef(File_पङ्क्तिः* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_पङ्क्तिः* fl, पूर्णाङ्क वामः, पूर्णाङ्क दक्षिणः)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(वामः)});
    rightp(new AstConst{fl, static_cast<uint32_t>(दक्षिणः)});
}
AstRange::AstRange(File_पङ्क्तिः* fl, ध्रुव VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.वामः())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.दक्षिणः())});
}
पूर्णाङ्क AstRange::वामः_Const() ध्रुव VL_MT_STABLE {
    ध्रुव AstConst* ध्रुव constp = VN_CAST(leftp(), Const);
    पटिनिवत्त (constp ? constp->toSInt() : 0);
}
पूर्णाङ्क AstRange::दक्षिणः_Const() ध्रुव VL_MT_STABLE {
    ध्रुव AstConst* ध्रुव constp = VN_CAST(rightp(), Const);
    पटिनिवत्त (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_पङ्क्तिः* fl, VFlagChildDType, Ast_ग्रन्थिः_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ध्रुव पूर्णाङ्क width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ध्रुव पूर्णाङ्क width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

पूर्णाङ्क AstQueueDType::boundConst() ध्रुव VL_MT_STABLE {
    ध्रुव AstConst* ध्रुव constp = VN_CAST(boundp(), Const);
    पटिनिवत्त (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(File_पङ्क्तिः* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

पूर्णाङ्क AstBasicDType::hi() ध्रुव { पटिनिवत्त (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
पूर्णाङ्क AstBasicDType::lo() ध्रुव { पटिनिवत्त (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
पूर्णाङ्क AstBasicDType::elements() ध्रुव {
    पटिनिवत्त (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() ध्रुव {
    पटिनिवत्त (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() ध्रुव { पटिनिवत्त m_sentreep->hasClocked(); }
bool AstActive::hasCombo() ध्रुव { पटिनिवत्त m_sentreep->hasCombo(); }
bool AstActive::hasInitial() ध्रुव { पटिनिवत्त m_sentreep->hasInitial(); }
bool AstActive::hasStatic() ध्रुव { पटिनिवत्त m_sentreep->hasStatic(); }

bool Ast_ग्रन्थिः_Module::isDollarUnit() ध्रुव { पटिनिवत्त this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_ग्रन्थिः_Module::isConstPool() ध्रुव { पटिनिवत्त this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_पङ्क्तिः* fl, VDisplayType dispType, Ast_ग्रन्थिः_व्यञ्जकम्* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव {
    ध्रुव AstVar* ध्रुव asamep = VN_DBG_AS(samep, Var);
    पटिनिवत्त m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_पङ्क्तिः* fl, AstVar* varp, ध्रुव VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    सचे (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } अञ्ञथा सचे (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_Module* pkgp, AstVar* varp, ध्रुव VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_पङ्क्तिः* fl, AstVarScope* varscp, ध्रुव VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ध्रुव { पटिनिवत्त varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_ग्रन्थिः(ध्रुव AstVarRef* samep) ध्रुव {
    सचे (varScopep()) पटिनिवत्त (varScopep() == samep->varScopep() && access() == samep->access());
    पटिनिवत्त (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_ग्रन्थिः(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ध्रुव AstVarRef* samep) ध्रुव {
    सचे (varScopep()) पटिनिवत्त (varScopep() == samep->varScopep());

    पटिनिवत्त (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_ग्रन्थिः(samep->varp()));
}

AstVarXRef::AstVarXRef(File_पङ्क्तिः* fl, AstVar* varp, ध्रुव string& dotted, ध्रुव VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_व्यञ्जकम्* Ast_ग्रन्थिः_व्यञ्जकम्::makeStmt() { पटिनिवत्त new Ast_Stmt_व्यञ्जकम्{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() ध्रुव VL_MT_STABLE {
    पटिनिवत्त VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
अन्तःस्थ Ast_ग्रन्थिः_Module* findParentModule(Ast_ग्रन्थिः* nodep) {
    अत्थाय (Ast_ग्रन्थिः* curp = nodep; curp; curp = curp->backp()) {
        सचे (Ast_ग्रन्थिः_Module* ध्रुव modp = VN_CAST(curp, ग्रन्थिः_Module)) पटिनिवत्त modp;
    }
    पटिनिवत्त nullptr;
}

#endif  // Guard
