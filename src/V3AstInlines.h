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

पूर्णांक Ast_गाठ::width() स्थिरांक VL_MT_STABLE { परत dtypep() ? dtypep()->width() : 0; }
पूर्णांक Ast_गाठ::widthMin() स्थिरांक VL_MT_STABLE { परत dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_गाठ::width1() स्थिरांक {  // V3Const uses to know it can optimize
    परत dtypep() && dtypep()->width() == 1;
}
पूर्णांक Ast_गाठ::widthInstrs() स्थिरांक {
    परत (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_शब्द() : 1));
}
bool Ast_गाठ::isCHandle() स्थिरांक VL_MT_STABLE {
    परत dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_गाठ::isDouble() स्थिरांक VL_MT_STABLE {
    परत dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_गाठ::isString() स्थिरांक VL_MT_STABLE {
    परत dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_गाठ::isEvent() स्थिरांक VL_MT_STABLE {
    परत dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_गाठ::isSigned() स्थिरांक VL_MT_STABLE { परत dtypep() && dtypep()->isSigned(); }

bool Ast_गाठ::is_Class_Handle_मूल्य() स्थिरांक {
    परत (VN_IS(this, Const) && VN_AS(this, Const)->अंक().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_गाठ::isNull() स्थिरांक { परत VN_IS(this, Const) && VN_AS(this, Const)->अंक().isNull(); }
bool Ast_गाठ::isZero() स्थिरांक {
    परत (VN_IS(this, Const) && VN_AS(this, Const)->अंक().isEqZero());
}
bool Ast_गाठ::isNeqZero() स्थिरांक {
    परत (VN_IS(this, Const) && VN_AS(this, Const)->अंक().isNeqZero());
}
bool Ast_गाठ::isOne() स्थिरांक { परत (VN_IS(this, Const) && VN_AS(this, Const)->अंक().isEqOne()); }
bool Ast_गाठ::isAllOnes() स्थिरांक {
    परत (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_गाठ::isAllOnesV() स्थिरांक {
    परत (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_गाठ::same_वृक्ष(स्थिरांक Ast_गाठ* node2p) स्थिरांक {
    परत same_वृक्ष_Iter(this, node2p, true, false);
}
bool Ast_गाठ::same_Gate_वृक्ष(स्थिरांक Ast_गाठ* node2p) स्थिरांक {
    परत same_वृक्ष_Iter(this, node2p, true, true);
}

पूर्णांक Ast_गाठ_Array_DType::डावा() स्थिरांक VL_MT_STABLE { परत rangep()->डावा_Const(); }
पूर्णांक Ast_गाठ_Array_DType::उजवा() स्थिरांक VL_MT_STABLE { परत rangep()->उजवा_Const(); }
पूर्णांक Ast_गाठ_Array_DType::hi() स्थिरांक VL_MT_STABLE { परत rangep()->hiConst(); }
पूर्णांक Ast_गाठ_Array_DType::lo() स्थिरांक VL_MT_STABLE { परत rangep()->loConst(); }
पूर्णांक Ast_गाठ_Array_DType::elementsConst() स्थिरांक VL_MT_STABLE { परत rangep()->elementsConst(); }
VNumRange Ast_गाठ_Array_DType::declRange() स्थिरांक VL_MT_STABLE { परत VNumRange{डावा(), उजवा()}; }

AstFuncRef::AstFuncRef(File_ओळ* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_ओळ* fl, पूर्णांक डावा, पूर्णांक उजवा)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(डावा)});
    rightp(new AstConst{fl, static_cast<uint32_t>(उजवा)});
}
AstRange::AstRange(File_ओळ* fl, स्थिरांक VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.डावा())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.उजवा())});
}
पूर्णांक AstRange::डावा_Const() स्थिरांक VL_MT_STABLE {
    स्थिरांक AstConst* स्थिरांक constp = VN_CAST(leftp(), Const);
    परत (constp ? constp->toSInt() : 0);
}
पूर्णांक AstRange::उजवा_Const() स्थिरांक VL_MT_STABLE {
    स्थिरांक AstConst* स्थिरांक constp = VN_CAST(rightp(), Const);
    परत (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_ओळ* fl, VFlagChildDType, Ast_गाठ_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    स्थिरांक पूर्णांक width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_ओळ* fl, Ast_गाठ_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    स्थिरांक पूर्णांक width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

पूर्णांक AstQueueDType::boundConst() स्थिरांक VL_MT_STABLE {
    स्थिरांक AstConst* स्थिरांक constp = VN_CAST(boundp(), Const);
    परत (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(File_ओळ* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

पूर्णांक AstBasicDType::hi() स्थिरांक { परत (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
पूर्णांक AstBasicDType::lo() स्थिरांक { परत (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
पूर्णांक AstBasicDType::elements() स्थिरांक {
    परत (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() स्थिरांक {
    परत (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() स्थिरांक { परत m_sentreep->hasClocked(); }
bool AstActive::hasCombo() स्थिरांक { परत m_sentreep->hasCombo(); }
bool AstActive::hasInitial() स्थिरांक { परत m_sentreep->hasInitial(); }
bool AstActive::hasStatic() स्थिरांक { परत m_sentreep->hasStatic(); }

bool Ast_गाठ_Module::isDollarUnit() स्थिरांक { परत this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_गाठ_Module::isConstPool() स्थिरांक { परत this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_ओळ* fl, VDisplayType dispType, Ast_गाठ_अभिव्यक्ती* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_गाठ(स्थिरांक Ast_गाठ* samep) स्थिरांक {
    स्थिरांक AstVar* स्थिरांक asamep = VN_DBG_AS(samep, Var);
    परत m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_ओळ* fl, Ast_गाठ_अभिव्यक्ती* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_ओळ* fl, AstVar* varp, स्थिरांक VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    जर (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } अन्यथा जर (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_ओळ* fl, Ast_गाठ_Module* pkgp, AstVar* varp, स्थिरांक VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_ओळ* fl, AstVarScope* varscp, स्थिरांक VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() स्थिरांक { परत varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_गाठ(स्थिरांक AstVarRef* samep) स्थिरांक {
    जर (varScopep()) परत (varScopep() == samep->varScopep() && access() == samep->access());
    परत (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_गाठ(samep->varp())));
}
bool AstVarRef::sameNoLvalue(स्थिरांक AstVarRef* samep) स्थिरांक {
    जर (varScopep()) परत (varScopep() == samep->varScopep());

    परत (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_गाठ(samep->varp()));
}

AstVarXRef::AstVarXRef(File_ओळ* fl, AstVar* varp, स्थिरांक string& dotted, स्थिरांक VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_अभिव्यक्ती* Ast_गाठ_अभिव्यक्ती::makeStmt() { परत new Ast_Stmt_अभिव्यक्ती{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() स्थिरांक VL_MT_STABLE {
    परत VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
ओळीत Ast_गाठ_Module* findParentModule(Ast_गाठ* nodep) {
    साठी (Ast_गाठ* curp = nodep; curp; curp = curp->backp()) {
        जर (Ast_गाठ_Module* स्थिरांक modp = VN_CAST(curp, गाठ_Module)) परत modp;
    }
    परत nullptr;
}

#endif  // Guard
