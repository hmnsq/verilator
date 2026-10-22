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
#அந்தர்பாவ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#endif

//######################################################################
// Inline METHODS

முழு Ast_முனை::width() மாறிலி VL_MT_STABLE { திரும்பு dtypep() ? dtypep()->width() : 0; }
முழு Ast_முனை::widthMin() மாறிலி VL_MT_STABLE { திரும்பு dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_முனை::width1() மாறிலி {  // V3Const uses to know it can optimize
    திரும்பு dtypep() && dtypep()->width() == 1;
}
முழு Ast_முனை::widthInstrs() மாறிலி {
    திரும்பு (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_சொற்கள்() : 1));
}
bool Ast_முனை::isCHandle() மாறிலி VL_MT_STABLE {
    திரும்பு dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_முனை::isDouble() மாறிலி VL_MT_STABLE {
    திரும்பு dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_முனை::isString() மாறிலி VL_MT_STABLE {
    திரும்பு dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_முனை::isEvent() மாறிலி VL_MT_STABLE {
    திரும்பு dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_முனை::isSigned() மாறிலி VL_MT_STABLE { திரும்பு dtypep() && dtypep()->isSigned(); }

bool Ast_முனை::is_Class_Handle_மதிப்பு() மாறிலி {
    திரும்பு (VN_IS(this, Const) && VN_AS(this, Const)->இலக்கம்().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_முனை::isNull() மாறிலி { திரும்பு VN_IS(this, Const) && VN_AS(this, Const)->இலக்கம்().isNull(); }
bool Ast_முனை::isZero() மாறிலி {
    திரும்பு (VN_IS(this, Const) && VN_AS(this, Const)->இலக்கம்().isEqZero());
}
bool Ast_முனை::isNeqZero() மாறிலி {
    திரும்பு (VN_IS(this, Const) && VN_AS(this, Const)->இலக்கம்().isNeqZero());
}
bool Ast_முனை::isOne() மாறிலி { திரும்பு (VN_IS(this, Const) && VN_AS(this, Const)->இலக்கம்().isEqOne()); }
bool Ast_முனை::isAllOnes() மாறிலி {
    திரும்பு (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_முனை::isAllOnesV() மாறிலி {
    திரும்பு (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_முனை::same_மரம்(மாறிலி Ast_முனை* node2p) மாறிலி {
    திரும்பு same_மரம்_Iter(this, node2p, true, false);
}
bool Ast_முனை::same_Gate_மரம்(மாறிலி Ast_முனை* node2p) மாறிலி {
    திரும்பு same_மரம்_Iter(this, node2p, true, true);
}

முழு Ast_முனை_Array_DType::இடது() மாறிலி VL_MT_STABLE { திரும்பு rangep()->இடது_Const(); }
முழு Ast_முனை_Array_DType::வலது() மாறிலி VL_MT_STABLE { திரும்பு rangep()->வலது_Const(); }
முழு Ast_முனை_Array_DType::hi() மாறிலி VL_MT_STABLE { திரும்பு rangep()->hiConst(); }
முழு Ast_முனை_Array_DType::lo() மாறிலி VL_MT_STABLE { திரும்பு rangep()->loConst(); }
முழு Ast_முனை_Array_DType::elementsConst() மாறிலி VL_MT_STABLE { திரும்பு rangep()->elementsConst(); }
VNumRange Ast_முனை_Array_DType::declRange() மாறிலி VL_MT_STABLE { திரும்பு VNumRange{இடது(), வலது()}; }

AstFuncRef::AstFuncRef(File_வரி* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_வரி* fl, முழு இடது, முழு வலது)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(இடது)});
    rightp(new AstConst{fl, static_cast<uint32_t>(வலது)});
}
AstRange::AstRange(File_வரி* fl, மாறிலி VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.இடது())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.வலது())});
}
முழு AstRange::இடது_Const() மாறிலி VL_MT_STABLE {
    மாறிலி AstConst* மாறிலி constp = VN_CAST(leftp(), Const);
    திரும்பு (constp ? constp->toSInt() : 0);
}
முழு AstRange::வலது_Const() மாறிலி VL_MT_STABLE {
    மாறிலி AstConst* மாறிலி constp = VN_CAST(rightp(), Const);
    திரும்பு (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_வரி* fl, VFlagChildDType, Ast_முனை_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    மாறிலி முழு width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_வரி* fl, Ast_முனை_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    மாறிலி முழு width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

முழு AstQueueDType::boundConst() மாறிலி VL_MT_STABLE {
    மாறிலி AstConst* மாறிலி constp = VN_CAST(boundp(), Const);
    திரும்பு (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(File_வரி* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

முழு AstBasicDType::hi() மாறிலி { திரும்பு (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
முழு AstBasicDType::lo() மாறிலி { திரும்பு (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
முழு AstBasicDType::elements() மாறிலி {
    திரும்பு (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() மாறிலி {
    திரும்பு (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() மாறிலி { திரும்பு m_sentreep->hasClocked(); }
bool AstActive::hasCombo() மாறிலி { திரும்பு m_sentreep->hasCombo(); }
bool AstActive::hasInitial() மாறிலி { திரும்பு m_sentreep->hasInitial(); }
bool AstActive::hasStatic() மாறிலி { திரும்பு m_sentreep->hasStatic(); }

bool Ast_முனை_Module::isDollarUnit() மாறிலி { திரும்பு this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_முனை_Module::isConstPool() மாறிலி { திரும்பு this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_வரி* fl, VDisplayType dispType, Ast_முனை_கோவை* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_முனை(மாறிலி Ast_முனை* samep) மாறிலி {
    மாறிலி AstVar* மாறிலி asamep = VN_DBG_AS(samep, Var);
    திரும்பு m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_வரி* fl, Ast_முனை_கோவை* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_வரி* fl, AstVar* varp, மாறிலி VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    எனில் (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } இல்லையெனில் எனில் (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_வரி* fl, Ast_முனை_Module* pkgp, AstVar* varp, மாறிலி VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_வரி* fl, AstVarScope* varscp, மாறிலி VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() மாறிலி { திரும்பு varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_முனை(மாறிலி AstVarRef* samep) மாறிலி {
    எனில் (varScopep()) திரும்பு (varScopep() == samep->varScopep() && access() == samep->access());
    திரும்பு (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_முனை(samep->varp())));
}
bool AstVarRef::sameNoLvalue(மாறிலி AstVarRef* samep) மாறிலி {
    எனில் (varScopep()) திரும்பு (varScopep() == samep->varScopep());

    திரும்பு (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_முனை(samep->varp()));
}

AstVarXRef::AstVarXRef(File_வரி* fl, AstVar* varp, மாறிலி string& dotted, மாறிலி VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_கோவை* Ast_முனை_கோவை::makeStmt() { திரும்பு new Ast_Stmt_கோவை{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() மாறிலி VL_MT_STABLE {
    திரும்பு VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
உள்வரி Ast_முனை_Module* findParentModule(Ast_முனை* nodep) {
    க்கு (Ast_முனை* curp = nodep; curp; curp = curp->backp()) {
        எனில் (Ast_முனை_Module* மாறிலி modp = VN_CAST(curp, முனை_Module)) திரும்பு modp;
    }
    திரும்பு nullptr;
}

#endif  // Guard
