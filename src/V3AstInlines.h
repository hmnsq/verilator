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
#ಅನ್ತರ್ಭಾವ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#endif

//######################################################################
// Inline METHODS

ಪೂರ್ಣಾಙ್ಕ Ast_ಗಂಟು::width() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ dtypep() ? dtypep()->width() : 0; }
ಪೂರ್ಣಾಙ್ಕ Ast_ಗಂಟು::widthMin() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_ಗಂಟು::width1() ಧ್ರುವ {  // V3Const uses to know it can optimize
    ಪ್ರತ್ಯಾಗಮ dtypep() && dtypep()->width() == 1;
}
ಪೂರ್ಣಾಙ್ಕ Ast_ಗಂಟು::widthInstrs() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_ಪದಗಳು() : 1));
}
bool Ast_ಗಂಟು::isCHandle() ಧ್ರುವ VL_MT_STABLE {
    ಪ್ರತ್ಯಾಗಮ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_ಗಂಟು::isDouble() ಧ್ರುವ VL_MT_STABLE {
    ಪ್ರತ್ಯಾಗಮ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_ಗಂಟು::isString() ಧ್ರುವ VL_MT_STABLE {
    ಪ್ರತ್ಯಾಗಮ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_ಗಂಟು::isEvent() ಧ್ರುವ VL_MT_STABLE {
    ಪ್ರತ್ಯಾಗಮ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_ಗಂಟು::isSigned() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ dtypep() && dtypep()->isSigned(); }

bool Ast_ಗಂಟು::is_Class_Handle_ಮೌಲ್ಯ() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ (VN_IS(this, Const) && VN_AS(this, Const)->ಅಂಕೆ().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_ಗಂಟು::isNull() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VN_IS(this, Const) && VN_AS(this, Const)->ಅಂಕೆ().isNull(); }
bool Ast_ಗಂಟು::isZero() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ (VN_IS(this, Const) && VN_AS(this, Const)->ಅಂಕೆ().isEqZero());
}
bool Ast_ಗಂಟು::isNeqZero() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ (VN_IS(this, Const) && VN_AS(this, Const)->ಅಂಕೆ().isNeqZero());
}
bool Ast_ಗಂಟು::isOne() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ (VN_IS(this, Const) && VN_AS(this, Const)->ಅಂಕೆ().isEqOne()); }
bool Ast_ಗಂಟು::isAllOnes() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_ಗಂಟು::isAllOnesV() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_ಗಂಟು::same_ವೃಕ್ಷ(ಧ್ರುವ Ast_ಗಂಟು* node2p) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ same_ವೃಕ್ಷ_Iter(this, node2p, true, false);
}
bool Ast_ಗಂಟು::same_Gate_ವೃಕ್ಷ(ಧ್ರುವ Ast_ಗಂಟು* node2p) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ same_ವೃಕ್ಷ_Iter(this, node2p, true, true);
}

ಪೂರ್ಣಾಙ್ಕ Ast_ಗಂಟು_Array_DType::ಎಡ() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ rangep()->ಎಡ_Const(); }
ಪೂರ್ಣಾಙ್ಕ Ast_ಗಂಟು_Array_DType::ಬಲ() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ rangep()->ಬಲ_Const(); }
ಪೂರ್ಣಾಙ್ಕ Ast_ಗಂಟು_Array_DType::hi() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ rangep()->hiConst(); }
ಪೂರ್ಣಾಙ್ಕ Ast_ಗಂಟು_Array_DType::lo() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ rangep()->loConst(); }
ಪೂರ್ಣಾಙ್ಕ Ast_ಗಂಟು_Array_DType::elementsConst() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ rangep()->elementsConst(); }
VNumRange Ast_ಗಂಟು_Array_DType::declRange() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ VNumRange{ಎಡ(), ಬಲ()}; }

AstFuncRef::AstFuncRef(File_ಸಾಲು* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_ಸಾಲು* fl, ಪೂರ್ಣಾಙ್ಕ ಎಡ, ಪೂರ್ಣಾಙ್ಕ ಬಲ)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(ಎಡ)});
    rightp(new AstConst{fl, static_cast<uint32_t>(ಬಲ)});
}
AstRange::AstRange(File_ಸಾಲು* fl, ಧ್ರುವ VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.ಎಡ())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.ಬಲ())});
}
ಪೂರ್ಣಾಙ್ಕ AstRange::ಎಡ_Const() ಧ್ರುವ VL_MT_STABLE {
    ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(leftp(), Const);
    ಪ್ರತ್ಯಾಗಮ (constp ? constp->toSInt() : 0);
}
ಪೂರ್ಣಾಙ್ಕ AstRange::ಬಲ_Const() ಧ್ರುವ VL_MT_STABLE {
    ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(rightp(), Const);
    ಪ್ರತ್ಯಾಗಮ (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_ಸಾಲು* fl, VFlagChildDType, Ast_ಗಂಟು_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

ಪೂರ್ಣಾಙ್ಕ AstQueueDType::boundConst() ಧ್ರುವ VL_MT_STABLE {
    ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(boundp(), Const);
    ಪ್ರತ್ಯಾಗಮ (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(File_ಸಾಲು* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

ಪೂರ್ಣಾಙ್ಕ AstBasicDType::hi() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
ಪೂರ್ಣಾಙ್ಕ AstBasicDType::lo() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
ಪೂರ್ಣಾಙ್ಕ AstBasicDType::elements() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_sentreep->hasClocked(); }
bool AstActive::hasCombo() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_sentreep->hasCombo(); }
bool AstActive::hasInitial() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_sentreep->hasInitial(); }
bool AstActive::hasStatic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_sentreep->hasStatic(); }

bool Ast_ಗಂಟು_Module::isDollarUnit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_ಗಂಟು_Module::isConstPool() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_ಸಾಲು* fl, VDisplayType dispType, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ {
    ಧ್ರುವ AstVar* ಧ್ರುವ asamep = VN_DBG_AS(samep, Var);
    ಪ್ರತ್ಯಾಗಮ m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_ಸಾಲು* fl, AstVar* varp, ಧ್ರುವ VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    ಯದಿ (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } ಅನ್ಯಥಾ ಯದಿ (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_ಸಾಲು* fl, Ast_ಗಂಟು_Module* pkgp, AstVar* varp, ಧ್ರುವ VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_ಸಾಲು* fl, AstVarScope* varscp, ಧ್ರುವ VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_ಗಂಟು(ಧ್ರುವ AstVarRef* samep) ಧ್ರುವ {
    ಯದಿ (varScopep()) ಪ್ರತ್ಯಾಗಮ (varScopep() == samep->varScopep() && access() == samep->access());
    ಪ್ರತ್ಯಾಗಮ (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_ಗಂಟು(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ಧ್ರುವ AstVarRef* samep) ಧ್ರುವ {
    ಯದಿ (varScopep()) ಪ್ರತ್ಯಾಗಮ (varScopep() == samep->varScopep());

    ಪ್ರತ್ಯಾಗಮ (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_ಗಂಟು(samep->varp()));
}

AstVarXRef::AstVarXRef(File_ಸಾಲು* fl, AstVar* varp, ಧ್ರುವ string& dotted, ಧ್ರುವ VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::makeStmt() { ಪ್ರತ್ಯಾಗಮ new Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() ಧ್ರುವ VL_MT_STABLE {
    ಪ್ರತ್ಯಾಗಮ VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
ಅನ್ತಃಸ್ಥ Ast_ಗಂಟು_Module* findParentModule(Ast_ಗಂಟು* nodep) {
    ಪ್ರತಿ (Ast_ಗಂಟು* curp = nodep; curp; curp = curp->backp()) {
        ಯದಿ (Ast_ಗಂಟು_Module* ಧ್ರುವ modp = VN_CAST(curp, ಗಂಟು_Module)) ಪ್ರತ್ಯಾಗಮ modp;
    }
    ಪ್ರತ್ಯಾಗಮ nullptr;
}

#endif  // Guard
