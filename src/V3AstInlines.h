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

पूर्णाङ्क Ast_गाठ::width() ध्रुव VL_MT_STABLE { प्रत्यागम dtypep() ? dtypep()->width() : 0; }
पूर्णाङ्क Ast_गाठ::widthMin() ध्रुव VL_MT_STABLE { प्रत्यागम dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_गाठ::width1() ध्रुव {  // V3Const uses to know it can optimize
    प्रत्यागम dtypep() && dtypep()->width() == 1;
}
पूर्णाङ्क Ast_गाठ::widthInstrs() ध्रुव {
    प्रत्यागम (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_शब्द() : 1));
}
bool Ast_गाठ::isCHandle() ध्रुव VL_MT_STABLE {
    प्रत्यागम dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_गाठ::isDouble() ध्रुव VL_MT_STABLE {
    प्रत्यागम dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_गाठ::isString() ध्रुव VL_MT_STABLE {
    प्रत्यागम dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_गाठ::isEvent() ध्रुव VL_MT_STABLE {
    प्रत्यागम dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_गाठ::isSigned() ध्रुव VL_MT_STABLE { प्रत्यागम dtypep() && dtypep()->isSigned(); }

bool Ast_गाठ::is_Class_Handle_मूल्य() ध्रुव {
    प्रत्यागम (VN_IS(this, Const) && VN_AS(this, Const)->अंक().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_गाठ::isNull() ध्रुव { प्रत्यागम VN_IS(this, Const) && VN_AS(this, Const)->अंक().isNull(); }
bool Ast_गाठ::isZero() ध्रुव {
    प्रत्यागम (VN_IS(this, Const) && VN_AS(this, Const)->अंक().isEqZero());
}
bool Ast_गाठ::isNeqZero() ध्रुव {
    प्रत्यागम (VN_IS(this, Const) && VN_AS(this, Const)->अंक().isNeqZero());
}
bool Ast_गाठ::isOne() ध्रुव { प्रत्यागम (VN_IS(this, Const) && VN_AS(this, Const)->अंक().isEqOne()); }
bool Ast_गाठ::isAllOnes() ध्रुव {
    प्रत्यागम (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_गाठ::isAllOnesV() ध्रुव {
    प्रत्यागम (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_गाठ::same_वृक्ष(ध्रुव Ast_गाठ* node2p) ध्रुव {
    प्रत्यागम same_वृक्ष_Iter(this, node2p, true, false);
}
bool Ast_गाठ::same_Gate_वृक्ष(ध्रुव Ast_गाठ* node2p) ध्रुव {
    प्रत्यागम same_वृक्ष_Iter(this, node2p, true, true);
}

पूर्णाङ्क Ast_गाठ_Array_DType::डावा() ध्रुव VL_MT_STABLE { प्रत्यागम rangep()->डावा_Const(); }
पूर्णाङ्क Ast_गाठ_Array_DType::उजवा() ध्रुव VL_MT_STABLE { प्रत्यागम rangep()->उजवा_Const(); }
पूर्णाङ्क Ast_गाठ_Array_DType::hi() ध्रुव VL_MT_STABLE { प्रत्यागम rangep()->hiConst(); }
पूर्णाङ्क Ast_गाठ_Array_DType::lo() ध्रुव VL_MT_STABLE { प्रत्यागम rangep()->loConst(); }
पूर्णाङ्क Ast_गाठ_Array_DType::elementsConst() ध्रुव VL_MT_STABLE { प्रत्यागम rangep()->elementsConst(); }
VNumRange Ast_गाठ_Array_DType::declRange() ध्रुव VL_MT_STABLE { प्रत्यागम VNumRange{डावा(), उजवा()}; }

AstFuncRef::AstFuncRef(File_ओळ* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_ओळ* fl, पूर्णाङ्क डावा, पूर्णाङ्क उजवा)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(डावा)});
    rightp(new AstConst{fl, static_cast<uint32_t>(उजवा)});
}
AstRange::AstRange(File_ओळ* fl, ध्रुव VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.डावा())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.उजवा())});
}
पूर्णाङ्क AstRange::डावा_Const() ध्रुव VL_MT_STABLE {
    ध्रुव AstConst* ध्रुव constp = VN_CAST(leftp(), Const);
    प्रत्यागम (constp ? constp->toSInt() : 0);
}
पूर्णाङ्क AstRange::उजवा_Const() ध्रुव VL_MT_STABLE {
    ध्रुव AstConst* ध्रुव constp = VN_CAST(rightp(), Const);
    प्रत्यागम (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_ओळ* fl, VFlagChildDType, Ast_गाठ_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ध्रुव पूर्णाङ्क width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_ओळ* fl, Ast_गाठ_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ध्रुव पूर्णाङ्क width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

पूर्णाङ्क AstQueueDType::boundConst() ध्रुव VL_MT_STABLE {
    ध्रुव AstConst* ध्रुव constp = VN_CAST(boundp(), Const);
    प्रत्यागम (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(File_ओळ* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

पूर्णाङ्क AstBasicDType::hi() ध्रुव { प्रत्यागम (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
पूर्णाङ्क AstBasicDType::lo() ध्रुव { प्रत्यागम (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
पूर्णाङ्क AstBasicDType::elements() ध्रुव {
    प्रत्यागम (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() ध्रुव {
    प्रत्यागम (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() ध्रुव { प्रत्यागम m_sentreep->hasClocked(); }
bool AstActive::hasCombo() ध्रुव { प्रत्यागम m_sentreep->hasCombo(); }
bool AstActive::hasInitial() ध्रुव { प्रत्यागम m_sentreep->hasInitial(); }
bool AstActive::hasStatic() ध्रुव { प्रत्यागम m_sentreep->hasStatic(); }

bool Ast_गाठ_Module::isDollarUnit() ध्रुव { प्रत्यागम this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_गाठ_Module::isConstPool() ध्रुव { प्रत्यागम this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_ओळ* fl, VDisplayType dispType, Ast_गाठ_अभिव्यक्ती* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_गाठ(ध्रुव Ast_गाठ* samep) ध्रुव {
    ध्रुव AstVar* ध्रुव asamep = VN_DBG_AS(samep, Var);
    प्रत्यागम m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_ओळ* fl, Ast_गाठ_अभिव्यक्ती* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_ओळ* fl, AstVar* varp, ध्रुव VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    यदि (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } अन्यथा यदि (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_ओळ* fl, Ast_गाठ_Module* pkgp, AstVar* varp, ध्रुव VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_ओळ* fl, AstVarScope* varscp, ध्रुव VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ध्रुव { प्रत्यागम varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_गाठ(ध्रुव AstVarRef* samep) ध्रुव {
    यदि (varScopep()) प्रत्यागम (varScopep() == samep->varScopep() && access() == samep->access());
    प्रत्यागम (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_गाठ(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ध्रुव AstVarRef* samep) ध्रुव {
    यदि (varScopep()) प्रत्यागम (varScopep() == samep->varScopep());

    प्रत्यागम (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_गाठ(samep->varp()));
}

AstVarXRef::AstVarXRef(File_ओळ* fl, AstVar* varp, ध्रुव string& dotted, ध्रुव VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_अभिव्यक्ती* Ast_गाठ_अभिव्यक्ती::makeStmt() { प्रत्यागम new Ast_Stmt_अभिव्यक्ती{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() ध्रुव VL_MT_STABLE {
    प्रत्यागम VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
अन्तःस्थ Ast_गाठ_Module* findParentModule(Ast_गाठ* nodep) {
    प्रति (Ast_गाठ* curp = nodep; curp; curp = curp->backp()) {
        यदि (Ast_गाठ_Module* ध्रुव modp = VN_CAST(curp, गाठ_Module)) प्रत्यागम modp;
    }
    प्रत्यागम nullptr;
}

#endif  // Guard
