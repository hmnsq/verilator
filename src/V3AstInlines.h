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

पूर्णाङ्क Ast_गाँठो::width() अचर VL_MT_STABLE { फर्काऊ dtypep() ? dtypep()->width() : 0; }
पूर्णाङ्क Ast_गाँठो::widthMin() अचर VL_MT_STABLE { फर्काऊ dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_गाँठो::width1() अचर {  // V3Const uses to know it can optimize
    फर्काऊ dtypep() && dtypep()->width() == 1;
}
पूर्णाङ्क Ast_गाँठो::widthInstrs() अचर {
    फर्काऊ (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_शब्दहरू() : 1));
}
bool Ast_गाँठो::isCHandle() अचर VL_MT_STABLE {
    फर्काऊ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_गाँठो::isDouble() अचर VL_MT_STABLE {
    फर्काऊ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_गाँठो::isString() अचर VL_MT_STABLE {
    फर्काऊ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_गाँठो::isEvent() अचर VL_MT_STABLE {
    फर्काऊ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_गाँठो::isSigned() अचर VL_MT_STABLE { फर्काऊ dtypep() && dtypep()->isSigned(); }

bool Ast_गाँठो::is_Class_Handle_मान() अचर {
    फर्काऊ (VN_IS(this, Const) && VN_AS(this, Const)->अङ्क().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_गाँठो::isNull() अचर { फर्काऊ VN_IS(this, Const) && VN_AS(this, Const)->अङ्क().isNull(); }
bool Ast_गाँठो::isZero() अचर {
    फर्काऊ (VN_IS(this, Const) && VN_AS(this, Const)->अङ्क().isEqZero());
}
bool Ast_गाँठो::isNeqZero() अचर {
    फर्काऊ (VN_IS(this, Const) && VN_AS(this, Const)->अङ्क().isNeqZero());
}
bool Ast_गाँठो::isOne() अचर { फर्काऊ (VN_IS(this, Const) && VN_AS(this, Const)->अङ्क().isEqOne()); }
bool Ast_गाँठो::isAllOnes() अचर {
    फर्काऊ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_गाँठो::isAllOnesV() अचर {
    फर्काऊ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_गाँठो::same_रूख(अचर Ast_गाँठो* node2p) अचर {
    फर्काऊ same_रूख_Iter(this, node2p, true, false);
}
bool Ast_गाँठो::same_Gate_रूख(अचर Ast_गाँठो* node2p) अचर {
    फर्काऊ same_रूख_Iter(this, node2p, true, true);
}

पूर्णाङ्क Ast_गाँठो_Array_DType::बायाँ() अचर VL_MT_STABLE { फर्काऊ rangep()->बायाँ_Const(); }
पूर्णाङ्क Ast_गाँठो_Array_DType::दायाँ() अचर VL_MT_STABLE { फर्काऊ rangep()->दायाँ_Const(); }
पूर्णाङ्क Ast_गाँठो_Array_DType::hi() अचर VL_MT_STABLE { फर्काऊ rangep()->hiConst(); }
पूर्णाङ्क Ast_गाँठो_Array_DType::lo() अचर VL_MT_STABLE { फर्काऊ rangep()->loConst(); }
पूर्णाङ्क Ast_गाँठो_Array_DType::elementsConst() अचर VL_MT_STABLE { फर्काऊ rangep()->elementsConst(); }
VNumRange Ast_गाँठो_Array_DType::declRange() अचर VL_MT_STABLE { फर्काऊ VNumRange{बायाँ(), दायाँ()}; }

AstFuncRef::AstFuncRef(File_पङ्क्ति* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_पङ्क्ति* fl, पूर्णाङ्क बायाँ, पूर्णाङ्क दायाँ)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(बायाँ)});
    rightp(new AstConst{fl, static_cast<uint32_t>(दायाँ)});
}
AstRange::AstRange(File_पङ्क्ति* fl, अचर VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.बायाँ())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.दायाँ())});
}
पूर्णाङ्क AstRange::बायाँ_Const() अचर VL_MT_STABLE {
    अचर AstConst* अचर constp = VN_CAST(leftp(), Const);
    फर्काऊ (constp ? constp->toSInt() : 0);
}
पूर्णाङ्क AstRange::दायाँ_Const() अचर VL_MT_STABLE {
    अचर AstConst* अचर constp = VN_CAST(rightp(), Const);
    फर्काऊ (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_पङ्क्ति* fl, VFlagChildDType, Ast_गाँठो_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    अचर पूर्णाङ्क width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_पङ्क्ति* fl, Ast_गाँठो_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    अचर पूर्णाङ्क width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

पूर्णाङ्क AstQueueDType::boundConst() अचर VL_MT_STABLE {
    अचर AstConst* अचर constp = VN_CAST(boundp(), Const);
    फर्काऊ (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(File_पङ्क्ति* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

पूर्णाङ्क AstBasicDType::hi() अचर { फर्काऊ (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
पूर्णाङ्क AstBasicDType::lo() अचर { फर्काऊ (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
पूर्णाङ्क AstBasicDType::elements() अचर {
    फर्काऊ (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() अचर {
    फर्काऊ (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() अचर { फर्काऊ m_sentreep->hasClocked(); }
bool AstActive::hasCombo() अचर { फर्काऊ m_sentreep->hasCombo(); }
bool AstActive::hasInitial() अचर { फर्काऊ m_sentreep->hasInitial(); }
bool AstActive::hasStatic() अचर { फर्काऊ m_sentreep->hasStatic(); }

bool Ast_गाँठो_Module::isDollarUnit() अचर { फर्काऊ this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_गाँठो_Module::isConstPool() अचर { फर्काऊ this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_पङ्क्ति* fl, VDisplayType dispType, Ast_गाँठो_अभिव्यक्ति* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_गाँठो(अचर Ast_गाँठो* samep) अचर {
    अचर AstVar* अचर asamep = VN_DBG_AS(samep, Var);
    फर्काऊ m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_पङ्क्ति* fl, AstVar* varp, अचर VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    यदि (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } अन्यथा यदि (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_पङ्क्ति* fl, Ast_गाँठो_Module* pkgp, AstVar* varp, अचर VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_पङ्क्ति* fl, AstVarScope* varscp, अचर VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() अचर { फर्काऊ varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_गाँठो(अचर AstVarRef* samep) अचर {
    यदि (varScopep()) फर्काऊ (varScopep() == samep->varScopep() && access() == samep->access());
    फर्काऊ (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_गाँठो(samep->varp())));
}
bool AstVarRef::sameNoLvalue(अचर AstVarRef* samep) अचर {
    यदि (varScopep()) फर्काऊ (varScopep() == samep->varScopep());

    फर्काऊ (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_गाँठो(samep->varp()));
}

AstVarXRef::AstVarXRef(File_पङ्क्ति* fl, AstVar* varp, अचर string& dotted, अचर VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_अभिव्यक्ति* Ast_गाँठो_अभिव्यक्ति::makeStmt() { फर्काऊ new Ast_Stmt_अभिव्यक्ति{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() अचर VL_MT_STABLE {
    फर्काऊ VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
अन्तःस्थ Ast_गाँठो_Module* findParentModule(Ast_गाँठो* nodep) {
    लागि (Ast_गाँठो* curp = nodep; curp; curp = curp->backp()) {
        यदि (Ast_गाँठो_Module* अचर modp = VN_CAST(curp, गाँठो_Module)) फर्काऊ modp;
    }
    फर्काऊ nullptr;
}

#endif  // Guard
