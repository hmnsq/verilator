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
#অন্তর্ভাব "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#endif

//######################################################################
// Inline METHODS

পূর্ণাঙ্ক Ast_গ্রন্থি::width() ধ্রুব VL_MT_STABLE { প্রত্যাগম dtypep() ? dtypep()->width() : 0; }
পূর্ণাঙ্ক Ast_গ্রন্থি::widthMin() ধ্রুব VL_MT_STABLE { প্রত্যাগম dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_গ্রন্থি::width1() ধ্রুব {  // V3Const uses to know it can optimize
    প্রত্যাগম dtypep() && dtypep()->width() == 1;
}
পূর্ণাঙ্ক Ast_গ্রন্থি::widthInstrs() ধ্রুব {
    প্রত্যাগম (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_শব্দগুলি() : 1));
}
bool Ast_গ্রন্থি::isCHandle() ধ্রুব VL_MT_STABLE {
    প্রত্যাগম dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_গ্রন্থি::isDouble() ধ্রুব VL_MT_STABLE {
    প্রত্যাগম dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_গ্রন্থি::isString() ধ্রুব VL_MT_STABLE {
    প্রত্যাগম dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_গ্রন্থি::isEvent() ধ্রুব VL_MT_STABLE {
    প্রত্যাগম dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_গ্রন্থি::isSigned() ধ্রুব VL_MT_STABLE { প্রত্যাগম dtypep() && dtypep()->isSigned(); }

bool Ast_গ্রন্থি::is_Class_Handle_মান() ধ্রুব {
    প্রত্যাগম (VN_IS(this, Const) && VN_AS(this, Const)->অঙ্ক().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_গ্রন্থি::isNull() ধ্রুব { প্রত্যাগম VN_IS(this, Const) && VN_AS(this, Const)->অঙ্ক().isNull(); }
bool Ast_গ্রন্থি::isZero() ধ্রুব {
    প্রত্যাগম (VN_IS(this, Const) && VN_AS(this, Const)->অঙ্ক().isEqZero());
}
bool Ast_গ্রন্থি::isNeqZero() ধ্রুব {
    প্রত্যাগম (VN_IS(this, Const) && VN_AS(this, Const)->অঙ্ক().isNeqZero());
}
bool Ast_গ্রন্থি::isOne() ধ্রুব { প্রত্যাগম (VN_IS(this, Const) && VN_AS(this, Const)->অঙ্ক().isEqOne()); }
bool Ast_গ্রন্থি::isAllOnes() ধ্রুব {
    প্রত্যাগম (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_গ্রন্থি::isAllOnesV() ধ্রুব {
    প্রত্যাগম (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_গ্রন্থি::same_বৃক্ষ(ধ্রুব Ast_গ্রন্থি* node2p) ধ্রুব {
    প্রত্যাগম same_বৃক্ষ_Iter(this, node2p, true, false);
}
bool Ast_গ্রন্থি::same_Gate_বৃক্ষ(ধ্রুব Ast_গ্রন্থি* node2p) ধ্রুব {
    প্রত্যাগম same_বৃক্ষ_Iter(this, node2p, true, true);
}

পূর্ণাঙ্ক Ast_গ্রন্থি_Array_DType::বাম() ধ্রুব VL_MT_STABLE { প্রত্যাগম rangep()->বাম_Const(); }
পূর্ণাঙ্ক Ast_গ্রন্থি_Array_DType::ডান() ধ্রুব VL_MT_STABLE { প্রত্যাগম rangep()->ডান_Const(); }
পূর্ণাঙ্ক Ast_গ্রন্থি_Array_DType::hi() ধ্রুব VL_MT_STABLE { প্রত্যাগম rangep()->hiConst(); }
পূর্ণাঙ্ক Ast_গ্রন্থি_Array_DType::lo() ধ্রুব VL_MT_STABLE { প্রত্যাগম rangep()->loConst(); }
পূর্ণাঙ্ক Ast_গ্রন্থি_Array_DType::elementsConst() ধ্রুব VL_MT_STABLE { প্রত্যাগম rangep()->elementsConst(); }
VNumRange Ast_গ্রন্থি_Array_DType::declRange() ধ্রুব VL_MT_STABLE { প্রত্যাগম VNumRange{বাম(), ডান()}; }

AstFuncRef::AstFuncRef(File_সারি* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_সারি* fl, পূর্ণাঙ্ক বাম, পূর্ণাঙ্ক ডান)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(বাম)});
    rightp(new AstConst{fl, static_cast<uint32_t>(ডান)});
}
AstRange::AstRange(File_সারি* fl, ধ্রুব VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.বাম())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.ডান())});
}
পূর্ণাঙ্ক AstRange::বাম_Const() ধ্রুব VL_MT_STABLE {
    ধ্রুব AstConst* ধ্রুব constp = VN_CAST(leftp(), Const);
    প্রত্যাগম (constp ? constp->toSInt() : 0);
}
পূর্ণাঙ্ক AstRange::ডান_Const() ধ্রুব VL_MT_STABLE {
    ধ্রুব AstConst* ধ্রুব constp = VN_CAST(rightp(), Const);
    প্রত্যাগম (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_সারি* fl, VFlagChildDType, Ast_গ্রন্থি_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ধ্রুব পূর্ণাঙ্ক width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_সারি* fl, Ast_গ্রন্থি_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ধ্রুব পূর্ণাঙ্ক width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

পূর্ণাঙ্ক AstQueueDType::boundConst() ধ্রুব VL_MT_STABLE {
    ধ্রুব AstConst* ধ্রুব constp = VN_CAST(boundp(), Const);
    প্রত্যাগম (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(File_সারি* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

পূর্ণাঙ্ক AstBasicDType::hi() ধ্রুব { প্রত্যাগম (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
পূর্ণাঙ্ক AstBasicDType::lo() ধ্রুব { প্রত্যাগম (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
পূর্ণাঙ্ক AstBasicDType::elements() ধ্রুব {
    প্রত্যাগম (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() ধ্রুব {
    প্রত্যাগম (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() ধ্রুব { প্রত্যাগম m_sentreep->hasClocked(); }
bool AstActive::hasCombo() ধ্রুব { প্রত্যাগম m_sentreep->hasCombo(); }
bool AstActive::hasInitial() ধ্রুব { প্রত্যাগম m_sentreep->hasInitial(); }
bool AstActive::hasStatic() ধ্রুব { প্রত্যাগম m_sentreep->hasStatic(); }

bool Ast_গ্রন্থি_Module::isDollarUnit() ধ্রুব { প্রত্যাগম this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_গ্রন্থি_Module::isConstPool() ধ্রুব { প্রত্যাগম this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_সারি* fl, VDisplayType dispType, Ast_গ্রন্থি_রাশিমালা* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব {
    ধ্রুব AstVar* ধ্রুব asamep = VN_DBG_AS(samep, Var);
    প্রত্যাগম m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_সারি* fl, AstVar* varp, ধ্রুব VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    যদি (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } অন্যথা যদি (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_সারি* fl, Ast_গ্রন্থি_Module* pkgp, AstVar* varp, ধ্রুব VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_সারি* fl, AstVarScope* varscp, ধ্রুব VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ধ্রুব { প্রত্যাগম varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_গ্রন্থি(ধ্রুব AstVarRef* samep) ধ্রুব {
    যদি (varScopep()) প্রত্যাগম (varScopep() == samep->varScopep() && access() == samep->access());
    প্রত্যাগম (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_গ্রন্থি(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ধ্রুব AstVarRef* samep) ধ্রুব {
    যদি (varScopep()) প্রত্যাগম (varScopep() == samep->varScopep());

    প্রত্যাগম (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_গ্রন্থি(samep->varp()));
}

AstVarXRef::AstVarXRef(File_সারি* fl, AstVar* varp, ধ্রুব string& dotted, ধ্রুব VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_রাশিমালা* Ast_গ্রন্থি_রাশিমালা::makeStmt() { প্রত্যাগম new Ast_Stmt_রাশিমালা{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() ধ্রুব VL_MT_STABLE {
    প্রত্যাগম VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
অন্তঃস্থ Ast_গ্রন্থি_Module* findParentModule(Ast_গ্রন্থি* nodep) {
    প্রতি (Ast_গ্রন্থি* curp = nodep; curp; curp = curp->backp()) {
        যদি (Ast_গ্রন্থি_Module* ধ্রুব modp = VN_CAST(curp, গ্রন্থি_Module)) প্রত্যাগম modp;
    }
    প্রত্যাগম nullptr;
}

#endif  // Guard
