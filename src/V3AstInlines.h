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

#अगर_अघोषित VERILATOR_V3ASTINLINES_H_
#घोषणा VERILATOR_V3ASTINLINES_H_

#अगर_अघोषित VERILATOR_V3AST_H_
#त्रुटि "Use V3Ast.h as the include"
#समावेश "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#पूर्ण_अगर

//######################################################################
// Inline METHODS

पूर्णांक Ast_गाँठ::width() स्थिर VL_MT_STABLE { वापस dtypep() ? dtypep()->width() : 0; }
पूर्णांक Ast_गाँठ::widthMin() स्थिर VL_MT_STABLE { वापस dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_गाँठ::width1() स्थिर {  // V3Const uses to know it can optimize
    वापस dtypep() && dtypep()->width() == 1;
}
पूर्णांक Ast_गाँठ::widthInstrs() स्थिर {
    वापस (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_शब्दगण() : 1));
}
bool Ast_गाँठ::isCHandle() स्थिर VL_MT_STABLE {
    वापस dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_गाँठ::isDouble() स्थिर VL_MT_STABLE {
    वापस dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_गाँठ::isString() स्थिर VL_MT_STABLE {
    वापस dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_गाँठ::isEvent() स्थिर VL_MT_STABLE {
    वापस dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_गाँठ::isSigned() स्थिर VL_MT_STABLE { वापस dtypep() && dtypep()->isSigned(); }

bool Ast_गाँठ::is_Class_Handle_मूल्य() स्थिर {
    वापस (VN_IS(this, Const) && VN_AS(this, Const)->अंक().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_गाँठ::isNull() स्थिर { वापस VN_IS(this, Const) && VN_AS(this, Const)->अंक().isNull(); }
bool Ast_गाँठ::isZero() स्थिर {
    वापस (VN_IS(this, Const) && VN_AS(this, Const)->अंक().isEqZero());
}
bool Ast_गाँठ::isNeqZero() स्थिर {
    वापस (VN_IS(this, Const) && VN_AS(this, Const)->अंक().isNeqZero());
}
bool Ast_गाँठ::isOne() स्थिर { वापस (VN_IS(this, Const) && VN_AS(this, Const)->अंक().isEqOne()); }
bool Ast_गाँठ::isAllOnes() स्थिर {
    वापस (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_गाँठ::isAllOnesV() स्थिर {
    वापस (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_गाँठ::same_वृक्ष(स्थिर Ast_गाँठ* node2p) स्थिर {
    वापस same_वृक्ष_Iter(this, node2p, true, false);
}
bool Ast_गाँठ::same_Gate_वृक्ष(स्थिर Ast_गाँठ* node2p) स्थिर {
    वापस same_वृक्ष_Iter(this, node2p, true, true);
}

पूर्णांक Ast_गाँठ_Array_DType::बायाँ() स्थिर VL_MT_STABLE { वापस rangep()->बायाँ_Const(); }
पूर्णांक Ast_गाँठ_Array_DType::दायाँ() स्थिर VL_MT_STABLE { वापस rangep()->दायाँ_Const(); }
पूर्णांक Ast_गाँठ_Array_DType::hi() स्थिर VL_MT_STABLE { वापस rangep()->hiConst(); }
पूर्णांक Ast_गाँठ_Array_DType::lo() स्थिर VL_MT_STABLE { वापस rangep()->loConst(); }
पूर्णांक Ast_गाँठ_Array_DType::elementsConst() स्थिर VL_MT_STABLE { वापस rangep()->elementsConst(); }
VNumRange Ast_गाँठ_Array_DType::declRange() स्थिर VL_MT_STABLE { वापस VNumRange{बायाँ(), दायाँ()}; }

AstFuncRef::AstFuncRef(File_पंक्ति* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_पंक्ति* fl, पूर्णांक बायाँ, पूर्णांक दायाँ)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(बायाँ)});
    rightp(new AstConst{fl, static_cast<uint32_t>(दायाँ)});
}
AstRange::AstRange(File_पंक्ति* fl, स्थिर VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.बायाँ())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.दायाँ())});
}
पूर्णांक AstRange::बायाँ_Const() स्थिर VL_MT_STABLE {
    स्थिर AstConst* स्थिर constp = VN_CAST(leftp(), Const);
    वापस (constp ? constp->toSInt() : 0);
}
पूर्णांक AstRange::दायाँ_Const() स्थिर VL_MT_STABLE {
    स्थिर AstConst* स्थिर constp = VN_CAST(rightp(), Const);
    वापस (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_पंक्ति* fl, VFlagChildDType, Ast_गाँठ_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    स्थिर पूर्णांक width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_पंक्ति* fl, Ast_गाँठ_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    स्थिर पूर्णांक width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

पूर्णांक AstQueueDType::boundConst() स्थिर VL_MT_STABLE {
    स्थिर AstConst* स्थिर constp = VN_CAST(boundp(), Const);
    वापस (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(File_पंक्ति* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

पूर्णांक AstBasicDType::hi() स्थिर { वापस (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
पूर्णांक AstBasicDType::lo() स्थिर { वापस (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
पूर्णांक AstBasicDType::elements() स्थिर {
    वापस (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() स्थिर {
    वापस (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() स्थिर { वापस m_sentreep->hasClocked(); }
bool AstActive::hasCombo() स्थिर { वापस m_sentreep->hasCombo(); }
bool AstActive::hasInitial() स्थिर { वापस m_sentreep->hasInitial(); }
bool AstActive::hasStatic() स्थिर { वापस m_sentreep->hasStatic(); }

bool Ast_गाँठ_Module::isDollarUnit() स्थिर { वापस this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_गाँठ_Module::isConstPool() स्थिर { वापस this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_पंक्ति* fl, VDisplayType dispType, Ast_गाँठ_व्यंजक* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर {
    स्थिर AstVar* स्थिर asamep = VN_DBG_AS(samep, Var);
    वापस m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_पंक्ति* fl, AstVar* varp, स्थिर VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    अगर (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } अन्यथा अगर (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_पंक्ति* fl, Ast_गाँठ_Module* pkgp, AstVar* varp, स्थिर VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_पंक्ति* fl, AstVarScope* varscp, स्थिर VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() स्थिर { वापस varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_गाँठ(स्थिर AstVarRef* samep) स्थिर {
    अगर (varScopep()) वापस (varScopep() == samep->varScopep() && access() == samep->access());
    वापस (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_गाँठ(samep->varp())));
}
bool AstVarRef::sameNoLvalue(स्थिर AstVarRef* samep) स्थिर {
    अगर (varScopep()) वापस (varScopep() == samep->varScopep());

    वापस (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_गाँठ(samep->varp()));
}

AstVarXRef::AstVarXRef(File_पंक्ति* fl, AstVar* varp, स्थिर string& dotted, स्थिर VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_व्यंजक* Ast_गाँठ_व्यंजक::makeStmt() { वापस new Ast_Stmt_व्यंजक{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() स्थिर VL_MT_STABLE {
    वापस VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
अंतरभूत Ast_गाँठ_Module* findParentModule(Ast_गाँठ* nodep) {
    क्रम (Ast_गाँठ* curp = nodep; curp; curp = curp->backp()) {
        अगर (Ast_गाँठ_Module* स्थिर modp = VN_CAST(curp, गाँठ_Module)) वापस modp;
    }
    वापस nullptr;
}

#पूर्ण_अगर  // Guard
