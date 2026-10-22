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
#অন্তর্ভাৱ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#endif

//######################################################################
// Inline METHODS

পূর্ণাঙ্ক Ast_গ্ৰন্থি::width() ধ্রুৱ VL_MT_STABLE { ঘূৰাই dtypep() ? dtypep()->width() : 0; }
পূর্ণাঙ্ক Ast_গ্ৰন্থি::widthMin() ধ্রুৱ VL_MT_STABLE { ঘূৰাই dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_গ্ৰন্থি::width1() ধ্রুৱ {  // V3Const uses to know it can optimize
    ঘূৰাই dtypep() && dtypep()->width() == 1;
}
পূর্ণাঙ্ক Ast_গ্ৰন্থি::widthInstrs() ধ্রুৱ {
    ঘূৰাই (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_শব্দগুলি() : 1));
}
bool Ast_গ্ৰন্থি::isCHandle() ধ্রুৱ VL_MT_STABLE {
    ঘূৰাই dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_গ্ৰন্থি::isDouble() ধ্রুৱ VL_MT_STABLE {
    ঘূৰাই dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_গ্ৰন্থি::isString() ধ্রুৱ VL_MT_STABLE {
    ঘূৰাই dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_গ্ৰন্থি::isEvent() ধ্রুৱ VL_MT_STABLE {
    ঘূৰাই dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_গ্ৰন্থি::isSigned() ধ্রুৱ VL_MT_STABLE { ঘূৰাই dtypep() && dtypep()->isSigned(); }

bool Ast_গ্ৰন্থি::is_Class_Handle_মান() ধ্রুৱ {
    ঘূৰাই (VN_IS(this, Const) && VN_AS(this, Const)->অঙ্ক().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_গ্ৰন্থি::isNull() ধ্রুৱ { ঘূৰাই VN_IS(this, Const) && VN_AS(this, Const)->অঙ্ক().isNull(); }
bool Ast_গ্ৰন্থি::isZero() ধ্রুৱ {
    ঘূৰাই (VN_IS(this, Const) && VN_AS(this, Const)->অঙ্ক().isEqZero());
}
bool Ast_গ্ৰন্থি::isNeqZero() ধ্রুৱ {
    ঘূৰাই (VN_IS(this, Const) && VN_AS(this, Const)->অঙ্ক().isNeqZero());
}
bool Ast_গ্ৰন্থি::isOne() ধ্রুৱ { ঘূৰাই (VN_IS(this, Const) && VN_AS(this, Const)->অঙ্ক().isEqOne()); }
bool Ast_গ্ৰন্থি::isAllOnes() ধ্রুৱ {
    ঘূৰাই (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_গ্ৰন্থি::isAllOnesV() ধ্রুৱ {
    ঘূৰাই (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_গ্ৰন্থি::same_বৃক্ষ(ধ্রুৱ Ast_গ্ৰন্থি* node2p) ধ্রুৱ {
    ঘূৰাই same_বৃক্ষ_Iter(this, node2p, true, false);
}
bool Ast_গ্ৰন্থি::same_Gate_বৃক্ষ(ধ্রুৱ Ast_গ্ৰন্থি* node2p) ধ্রুৱ {
    ঘূৰাই same_বৃক্ষ_Iter(this, node2p, true, true);
}

পূর্ণাঙ্ক Ast_গ্ৰন্থি_Array_DType::বাম() ধ্রুৱ VL_MT_STABLE { ঘূৰাই rangep()->বাম_Const(); }
পূর্ণাঙ্ক Ast_গ্ৰন্থি_Array_DType::ডান() ধ্রুৱ VL_MT_STABLE { ঘূৰাই rangep()->ডান_Const(); }
পূর্ণাঙ্ক Ast_গ্ৰন্থি_Array_DType::hi() ধ্রুৱ VL_MT_STABLE { ঘূৰাই rangep()->hiConst(); }
পূর্ণাঙ্ক Ast_গ্ৰন্থি_Array_DType::lo() ধ্রুৱ VL_MT_STABLE { ঘূৰাই rangep()->loConst(); }
পূর্ণাঙ্ক Ast_গ্ৰন্থি_Array_DType::elementsConst() ধ্রুৱ VL_MT_STABLE { ঘূৰাই rangep()->elementsConst(); }
VNumRange Ast_গ্ৰন্থি_Array_DType::declRange() ধ্রুৱ VL_MT_STABLE { ঘূৰাই VNumRange{বাম(), ডান()}; }

AstFuncRef::AstFuncRef(File_সাৰি* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_সাৰি* fl, পূর্ণাঙ্ক বাম, পূর্ণাঙ্ক ডান)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(বাম)});
    rightp(new AstConst{fl, static_cast<uint32_t>(ডান)});
}
AstRange::AstRange(File_সাৰি* fl, ধ্রুৱ VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.বাম())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.ডান())});
}
পূর্ণাঙ্ক AstRange::বাম_Const() ধ্রুৱ VL_MT_STABLE {
    ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(leftp(), Const);
    ঘূৰাই (constp ? constp->toSInt() : 0);
}
পূর্ণাঙ্ক AstRange::ডান_Const() ধ্রুৱ VL_MT_STABLE {
    ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(rightp(), Const);
    ঘূৰাই (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_সাৰি* fl, VFlagChildDType, Ast_গ্ৰন্থি_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ধ্রুৱ পূর্ণাঙ্ক width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_সাৰি* fl, Ast_গ্ৰন্থি_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ধ্রুৱ পূর্ণাঙ্ক width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

পূর্ণাঙ্ক AstQueueDType::boundConst() ধ্রুৱ VL_MT_STABLE {
    ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(boundp(), Const);
    ঘূৰাই (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(File_সাৰি* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

পূর্ণাঙ্ক AstBasicDType::hi() ধ্রুৱ { ঘূৰাই (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
পূর্ণাঙ্ক AstBasicDType::lo() ধ্রুৱ { ঘূৰাই (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
পূর্ণাঙ্ক AstBasicDType::elements() ধ্রুৱ {
    ঘূৰাই (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() ধ্রুৱ {
    ঘূৰাই (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() ধ্রুৱ { ঘূৰাই m_sentreep->hasClocked(); }
bool AstActive::hasCombo() ধ্রুৱ { ঘূৰাই m_sentreep->hasCombo(); }
bool AstActive::hasInitial() ধ্রুৱ { ঘূৰাই m_sentreep->hasInitial(); }
bool AstActive::hasStatic() ধ্রুৱ { ঘূৰাই m_sentreep->hasStatic(); }

bool Ast_গ্ৰন্থি_Module::isDollarUnit() ধ্রুৱ { ঘূৰাই this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_গ্ৰন্থি_Module::isConstPool() ধ্রুৱ { ঘূৰাই this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_সাৰি* fl, VDisplayType dispType, Ast_গ্ৰন্থি_ৰাশিমালা* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ {
    ধ্রুৱ AstVar* ধ্রুৱ asamep = VN_DBG_AS(samep, Var);
    ঘূৰাই m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_সাৰি* fl, Ast_গ্ৰন্থি_ৰাশিমালা* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_সাৰি* fl, AstVar* varp, ধ্রুৱ VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    যদি (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } অন্যথা যদি (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_সাৰি* fl, Ast_গ্ৰন্থি_Module* pkgp, AstVar* varp, ধ্রুৱ VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_সাৰি* fl, AstVarScope* varscp, ধ্রুৱ VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ধ্রুৱ { ঘূৰাই varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_গ্ৰন্থি(ধ্রুৱ AstVarRef* samep) ধ্রুৱ {
    যদি (varScopep()) ঘূৰাই (varScopep() == samep->varScopep() && access() == samep->access());
    ঘূৰাই (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_গ্ৰন্থি(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ধ্রুৱ AstVarRef* samep) ধ্রুৱ {
    যদি (varScopep()) ঘূৰাই (varScopep() == samep->varScopep());

    ঘূৰাই (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_গ্ৰন্থি(samep->varp()));
}

AstVarXRef::AstVarXRef(File_সাৰি* fl, AstVar* varp, ধ্রুৱ string& dotted, ধ্রুৱ VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_ৰাশিমালা* Ast_গ্ৰন্থি_ৰাশিমালা::makeStmt() { ঘূৰাই new Ast_Stmt_ৰাশিমালা{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() ধ্রুৱ VL_MT_STABLE {
    ঘূৰাই VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
অন্তঃস্থ Ast_গ্ৰন্থি_Module* findParentModule(Ast_গ্ৰন্থি* nodep) {
    বাবে (Ast_গ্ৰন্থি* curp = nodep; curp; curp = curp->backp()) {
        যদি (Ast_গ্ৰন্থি_Module* ধ্রুৱ modp = VN_CAST(curp, গ্ৰন্থি_Module)) ঘূৰাই modp;
    }
    ঘূৰাই nullptr;
}

#endif  // Guard
