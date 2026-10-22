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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3ASTINLINES_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3ASTINLINES_H_

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3AST_H_
#ದೋಷ "Use V3Ast.h as the include"
#ಅನ್ತರ್ಭಾವ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#ಆದರೆ_ಅಂತ್ಯ

//######################################################################
// Inline METHODS

ಪೂರ್ಣಾಂಕ Ast_ಗಂಟು::ಅಗಲ() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ dtypep() ? dtypep()->ಅಗಲ() : 0; }
ಪೂರ್ಣಾಂಕ Ast_ಗಂಟು::ಅಗಲ_Min() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ dtypep() ? dtypep()->ಅಗಲ_Min() : 0; }
bool Ast_ಗಂಟು::width1() ಸ್ಥಿರಾಂಕ {  // V3Const uses to know it can optimize
    ಹಿಂತಿರುಗಿ dtypep() && dtypep()->ಅಗಲ() == 1;
}
ಪೂರ್ಣಾಂಕ Ast_ಗಂಟು::ಅಗಲ_Instrs() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->ಅಗಲ_ಪದಗಳು() : 1));
}
bool Ast_ಗಂಟು::isCHandle() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    ಹಿಂತಿರುಗಿ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_ಗಂಟು::isDouble() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    ಹಿಂತಿರುಗಿ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_ಗಂಟು::isString() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    ಹಿಂತಿರುಗಿ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_ಗಂಟು::isEvent() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    ಹಿಂತಿರುಗಿ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_ಗಂಟು::isSigned() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ dtypep() && dtypep()->isSigned(); }

bool Ast_ಗಂಟು::is_Class_Handle_ಮೌಲ್ಯ() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ (VN_IS(this, Const) && VN_AS(this, Const)->ಅಂಕೆ().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_ಗಂಟು::isNull() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VN_IS(this, Const) && VN_AS(this, Const)->ಅಂಕೆ().isNull(); }
bool Ast_ಗಂಟು::isZero() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ (VN_IS(this, Const) && VN_AS(this, Const)->ಅಂಕೆ().isEqZero());
}
bool Ast_ಗಂಟು::isNeqZero() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ (VN_IS(this, Const) && VN_AS(this, Const)->ಅಂಕೆ().isNeqZero());
}
bool Ast_ಗಂಟು::isOne() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ (VN_IS(this, Const) && VN_AS(this, Const)->ಅಂಕೆ().isEqOne()); }
bool Ast_ಗಂಟು::isAllOnes() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_ಗಂಟು::isAllOnesV() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_ಗಂಟು::same_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* node2p) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ same_ವೃಕ್ಷ_Iter(this, node2p, true, false);
}
bool Ast_ಗಂಟು::same_Gate_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* node2p) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ same_ವೃಕ್ಷ_Iter(this, node2p, true, true);
}

ಪೂರ್ಣಾಂಕ Ast_ಗಂಟು_Array_DType::ಎಡ_2() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ rangep()->ಎಡ_Const(); }
ಪೂರ್ಣಾಂಕ Ast_ಗಂಟು_Array_DType::ಬಲ_2() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ rangep()->ಬಲ_Const(); }
ಪೂರ್ಣಾಂಕ Ast_ಗಂಟು_Array_DType::hi() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ rangep()->hiConst(); }
ಪೂರ್ಣಾಂಕ Ast_ಗಂಟು_Array_DType::lo() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ rangep()->loConst(); }
ಪೂರ್ಣಾಂಕ Ast_ಗಂಟು_Array_DType::elementsConst() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ rangep()->elementsConst(); }
VNumRange Ast_ಗಂಟು_Array_DType::declRange() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ VNumRange{ಎಡ_2(), ಬಲ_2()}; }

AstFuncRef::AstFuncRef(File_ಸಾಲು* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_ಸಾಲು* fl, ಪೂರ್ಣಾಂಕ ಎಡ_2, ಪೂರ್ಣಾಂಕ ಬಲ_2)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(ಎಡ_2)});
    rightp(new AstConst{fl, static_cast<uint32_t>(ಬಲ_2)});
}
AstRange::AstRange(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.ಎಡ_2())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.ಬಲ_2())});
}
ಪೂರ್ಣಾಂಕ AstRange::ಎಡ_Const() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(leftp(), Const);
    ಹಿಂತಿರುಗಿ (constp ? constp->ಗಮ್ಯ_SInt() : 0);
}
ಪೂರ್ಣಾಂಕ AstRange::ಬಲ_Const() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(rightp(), Const);
    ಹಿಂತಿರುಗಿ (constp ? constp->ಗಮ್ಯ_SInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_ಸಾಲು* fl, VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    ಮಗು_DTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = subDTypep()->ಅಗಲ() * rangep->elementsConst();
    ಅಗಲ_Force(ಅಗಲ, ಅಗಲ);
}
AstPackArrayDType::AstPackArrayDType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = subDTypep()->ಅಗಲ() * rangep->elementsConst();
    ಅಗಲ_Force(ಅಗಲ, ಅಗಲ);
}

ಪೂರ್ಣಾಂಕ Ast_ಸರತಿ_DType::boundConst() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(boundp(), Const);
    ಹಿಂತಿರುಗಿ (constp ? constp->ಗಮ್ಯ_SInt() : 0);
}

AstTaskRef::AstTaskRef(File_ಸಾಲು* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtype_ಗಣ_Void();
}

ಪೂರ್ಣಾಂಕ AstBasicDType::hi() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
ಪೂರ್ಣಾಂಕ AstBasicDType::lo() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
ಪೂರ್ಣಾಂಕ AstBasicDType::elements() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool Ast_ಸಕ್ರಿಯ::hasClocked() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_sentreep->hasClocked(); }
bool Ast_ಸಕ್ರಿಯ::hasCombo() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_sentreep->hasCombo(); }
bool Ast_ಸಕ್ರಿಯ::hasInitial() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_sentreep->hasInitial(); }
bool Ast_ಸಕ್ರಿಯ::hasStatic() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_sentreep->hasStatic(); }

bool Ast_ಗಂಟು_Module::isDollarUnit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_ಗಂಟು_Module::isConstPool() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(Ast_ನಿಯೋಜಿಸು_W* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ನಿಯೋಜಿಸು} {}

AstElabDisplay::AstElabDisplay(File_ಸಾಲು* fl, VDisplayType dispType, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    ಕೂಡಿಸು_Fmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ {
    ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, Var);
    ಹಿಂತಿರುಗಿ m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

Ast_ಹೊಂದಿಕೆ_Masked::Ast_ಹೊಂದಿಕೆ_Masked(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_ಹೊಂದಿಕೆ_Masked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtype_ಗಣ_UInt32();
}

AstVarRef::AstVarRef(File_ಸಾಲು* fl, AstVar* varp, ಸ್ಥಿರಾಂಕ VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    ಆದರೆ (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_ಸಾಲು* fl, Ast_ಗಂಟು_Module* pkgp, AstVar* varp, ಸ್ಥಿರಾಂಕ VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_ಸಾಲು* fl, AstVarScope* varscp, ಸ್ಥಿರಾಂಕ VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_ಗಂಟು(ಸ್ಥಿರಾಂಕ AstVarRef* samep) ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (varScopep()) ಹಿಂತಿರುಗಿ (varScopep() == samep->varScopep() && access() == samep->access());
    ಹಿಂತಿರುಗಿ (self_ಸೂಚಕ() == samep->self_ಸೂಚಕ() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_ಗಂಟು(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ಸ್ಥಿರಾಂಕ AstVarRef* samep) ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (varScopep()) ಹಿಂತಿರುಗಿ (varScopep() == samep->varScopep());

    ಹಿಂತಿರುಗಿ (self_ಸೂಚಕ() == samep->self_ಸೂಚಕ() && classOrPackagep() == samep->classOrPackagep()
            && (!self_ಸೂಚಕ().is_ಖಾಲಿಗಳು() || !samep->self_ಸೂಚಕ().is_ಖಾಲಿಗಳು())
            && varp()->same_ಗಂಟು(samep->varp()));
}

AstVarXRef::AstVarXRef(File_ಸಾಲು* fl, AstVar* varp, ಸ್ಥಿರಾಂಕ string& dotted, ಸ್ಥಿರಾಂಕ VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::makeStmt() { ಹಿಂತಿರುಗಿ new Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{fileline(), this}; }

Ast_Cons_Array_ಅಂಶ* AstConsArray::itemsp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    ಹಿಂತಿರುಗಿ VN_DBG_AS(op2p(), Cons_Array_ಅಂಶ);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
ಸಾಲೊಳಗೆ Ast_ಗಂಟು_Module* ಕಂಡುಹಿಡಿ_ಪೋಷಕ_Module(Ast_ಗಂಟು* nodep) {
    ಗಾಗಿ (Ast_ಗಂಟು* curp = nodep; curp; curp = curp->backp()) {
        ಆದರೆ (Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = VN_CAST(curp, ಗಂಟು_Module)) ಹಿಂತಿರುಗಿ modp;
    }
    ಹಿಂತಿರುಗಿ nullptr;
}

#ಆದರೆ_ಅಂತ್ಯ  // Guard
