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
#ଅନ୍ତର୍ଭାଵ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#endif

//######################################################################
// Inline METHODS

ପୂର୍ଣ୍ଣସଂଖ୍ୟା Ast_ଗଣ୍ଠି::width() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ dtypep() ? dtypep()->width() : 0; }
ପୂର୍ଣ୍ଣସଂଖ୍ୟା Ast_ଗଣ୍ଠି::widthMin() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_ଗଣ୍ଠି::width1() ଅଚଳ {  // V3Const uses to know it can optimize
    ଫେରନ୍ତୁ dtypep() && dtypep()->width() == 1;
}
ପୂର୍ଣ୍ଣସଂଖ୍ୟା Ast_ଗଣ୍ଠି::widthInstrs() ଅଚଳ {
    ଫେରନ୍ତୁ (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_ଶବ୍ଦଗୁଡ଼ିକ() : 1));
}
bool Ast_ଗଣ୍ଠି::isCHandle() ଅଚଳ VL_MT_STABLE {
    ଫେରନ୍ତୁ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_ଗଣ୍ଠି::isDouble() ଅଚଳ VL_MT_STABLE {
    ଫେରନ୍ତୁ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_ଗଣ୍ଠି::isString() ଅଚଳ VL_MT_STABLE {
    ଫେରନ୍ତୁ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_ଗଣ୍ଠି::isEvent() ଅଚଳ VL_MT_STABLE {
    ଫେରନ୍ତୁ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_ଗଣ୍ଠି::isSigned() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ dtypep() && dtypep()->isSigned(); }

bool Ast_ଗଣ୍ଠି::is_Class_Handle_ମୂଲ୍ୟ() ଅଚଳ {
    ଫେରନ୍ତୁ (VN_IS(this, Const) && VN_AS(this, Const)->ଅଙ୍କ().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_ଗଣ୍ଠି::isNull() ଅଚଳ { ଫେରନ୍ତୁ VN_IS(this, Const) && VN_AS(this, Const)->ଅଙ୍କ().isNull(); }
bool Ast_ଗଣ୍ଠି::isZero() ଅଚଳ {
    ଫେରନ୍ତୁ (VN_IS(this, Const) && VN_AS(this, Const)->ଅଙ୍କ().isEqZero());
}
bool Ast_ଗଣ୍ଠି::isNeqZero() ଅଚଳ {
    ଫେରନ୍ତୁ (VN_IS(this, Const) && VN_AS(this, Const)->ଅଙ୍କ().isNeqZero());
}
bool Ast_ଗଣ୍ଠି::isOne() ଅଚଳ { ଫେରନ୍ତୁ (VN_IS(this, Const) && VN_AS(this, Const)->ଅଙ୍କ().isEqOne()); }
bool Ast_ଗଣ୍ଠି::isAllOnes() ଅଚଳ {
    ଫେରନ୍ତୁ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_ଗଣ୍ଠି::isAllOnesV() ଅଚଳ {
    ଫେରନ୍ତୁ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_ଗଣ୍ଠି::same_ବୃକ୍ଷ(ଅଚଳ Ast_ଗଣ୍ଠି* node2p) ଅଚଳ {
    ଫେରନ୍ତୁ same_ବୃକ୍ଷ_Iter(this, node2p, true, false);
}
bool Ast_ଗଣ୍ଠି::same_Gate_ବୃକ୍ଷ(ଅଚଳ Ast_ଗଣ୍ଠି* node2p) ଅଚଳ {
    ଫେରନ୍ତୁ same_ବୃକ୍ଷ_Iter(this, node2p, true, true);
}

ପୂର୍ଣ୍ଣସଂଖ୍ୟା Ast_ଗଣ୍ଠି_Array_DType::ବାମ() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ rangep()->ବାମ_Const(); }
ପୂର୍ଣ୍ଣସଂଖ୍ୟା Ast_ଗଣ୍ଠି_Array_DType::ଡାହାଣ() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ rangep()->ଡାହାଣ_Const(); }
ପୂର୍ଣ୍ଣସଂଖ୍ୟା Ast_ଗଣ୍ଠି_Array_DType::hi() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ rangep()->hiConst(); }
ପୂର୍ଣ୍ଣସଂଖ୍ୟା Ast_ଗଣ୍ଠି_Array_DType::lo() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ rangep()->loConst(); }
ପୂର୍ଣ୍ଣସଂଖ୍ୟା Ast_ଗଣ୍ଠି_Array_DType::elementsConst() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ rangep()->elementsConst(); }
VNumRange Ast_ଗଣ୍ଠି_Array_DType::declRange() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ VNumRange{ବାମ(), ଡାହାଣ()}; }

AstFuncRef::AstFuncRef(File_ଧାଡ଼ି* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_ଧାଡ଼ି* fl, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ବାମ, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଡାହାଣ)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(ବାମ)});
    rightp(new AstConst{fl, static_cast<uint32_t>(ଡାହାଣ)});
}
AstRange::AstRange(File_ଧାଡ଼ି* fl, ଅଚଳ VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.ବାମ())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.ଡାହାଣ())});
}
ପୂର୍ଣ୍ଣସଂଖ୍ୟା AstRange::ବାମ_Const() ଅଚଳ VL_MT_STABLE {
    ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(leftp(), Const);
    ଫେରନ୍ତୁ (constp ? constp->toSInt() : 0);
}
ପୂର୍ଣ୍ଣସଂଖ୍ୟା AstRange::ଡାହାଣ_Const() ଅଚଳ VL_MT_STABLE {
    ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(rightp(), Const);
    ଫେରନ୍ତୁ (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_ଧାଡ଼ି* fl, VFlagChildDType, Ast_ଗଣ୍ଠି_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

ପୂର୍ଣ୍ଣସଂଖ୍ୟା AstQueueDType::boundConst() ଅଚଳ VL_MT_STABLE {
    ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(boundp(), Const);
    ଫେରନ୍ତୁ (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(File_ଧାଡ଼ି* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

ପୂର୍ଣ୍ଣସଂଖ୍ୟା AstBasicDType::hi() ଅଚଳ { ଫେରନ୍ତୁ (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
ପୂର୍ଣ୍ଣସଂଖ୍ୟା AstBasicDType::lo() ଅଚଳ { ଫେରନ୍ତୁ (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
ପୂର୍ଣ୍ଣସଂଖ୍ୟା AstBasicDType::elements() ଅଚଳ {
    ଫେରନ୍ତୁ (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() ଅଚଳ {
    ଫେରନ୍ତୁ (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() ଅଚଳ { ଫେରନ୍ତୁ m_sentreep->hasClocked(); }
bool AstActive::hasCombo() ଅଚଳ { ଫେରନ୍ତୁ m_sentreep->hasCombo(); }
bool AstActive::hasInitial() ଅଚଳ { ଫେରନ୍ତୁ m_sentreep->hasInitial(); }
bool AstActive::hasStatic() ଅଚଳ { ଫେରନ୍ତୁ m_sentreep->hasStatic(); }

bool Ast_ଗଣ୍ଠି_Module::isDollarUnit() ଅଚଳ { ଫେରନ୍ତୁ this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_ଗଣ୍ଠି_Module::isConstPool() ଅଚଳ { ଫେରନ୍ତୁ this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_ଧାଡ଼ି* fl, VDisplayType dispType, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ {
    ଅଚଳ AstVar* ଅଚଳ asamep = VN_DBG_AS(samep, Var);
    ଫେରନ୍ତୁ m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_ଧାଡ଼ି* fl, AstVar* varp, ଅଚଳ VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    ଯଦି (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } ଅନ୍ୟଥା ଯଦି (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_Module* pkgp, AstVar* varp, ଅଚଳ VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_ଧାଡ଼ି* fl, AstVarScope* varscp, ଅଚଳ VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ଅଚଳ { ଫେରନ୍ତୁ varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_ଗଣ୍ଠି(ଅଚଳ AstVarRef* samep) ଅଚଳ {
    ଯଦି (varScopep()) ଫେରନ୍ତୁ (varScopep() == samep->varScopep() && access() == samep->access());
    ଫେରନ୍ତୁ (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_ଗଣ୍ଠି(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ଅଚଳ AstVarRef* samep) ଅଚଳ {
    ଯଦି (varScopep()) ଫେରନ୍ତୁ (varScopep() == samep->varScopep());

    ଫେରନ୍ତୁ (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_ଗଣ୍ଠି(samep->varp()));
}

AstVarXRef::AstVarXRef(File_ଧାଡ଼ି* fl, AstVar* varp, ଅଚଳ string& dotted, ଅଚଳ VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_ପଦସମଷ୍ଟି* Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି::makeStmt() { ଫେରନ୍ତୁ new Ast_Stmt_ପଦସମଷ୍ଟି{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() ଅଚଳ VL_MT_STABLE {
    ଫେରନ୍ତୁ VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
ଧାଡ଼ିଭିତରେ Ast_ଗଣ୍ଠି_Module* findParentModule(Ast_ଗଣ୍ଠି* nodep) {
    ପାଇଁ (Ast_ଗଣ୍ଠି* curp = nodep; curp; curp = curp->backp()) {
        ଯଦି (Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = VN_CAST(curp, ଗଣ୍ଠି_Module)) ଫେରନ୍ତୁ modp;
    }
    ଫେରନ୍ତୁ nullptr;
}

#endif  // Guard
