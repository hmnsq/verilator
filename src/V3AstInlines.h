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
#include "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#endif

//######################################################################
// Inline METHODS

පූර්ණ_සංඛ්‍යා AstNode::width() const VL_MT_STABLE { ආපසු dtypep() ? dtypep()->width() : 0; }
පූර්ණ_සංඛ්‍යා AstNode::widthMin() const VL_MT_STABLE { ආපසු dtypep() ? dtypep()->widthMin() : 0; }
bool AstNode::width1() const {  // V3Const uses to know it can optimize
    ආපසු dtypep() && dtypep()->width() == 1;
}
පූර්ණ_සංඛ්‍යා AstNode::widthInstrs() const {
    ආපසු (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->widthWords() : 1));
}
bool AstNode::isCHandle() const VL_MT_STABLE {
    ආපසු dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool AstNode::isDouble() const VL_MT_STABLE {
    ආපසු dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool AstNode::isString() const VL_MT_STABLE {
    ආපසු dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool AstNode::isEvent() const VL_MT_STABLE {
    ආපසු dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool AstNode::isSigned() const VL_MT_STABLE { ආපසු dtypep() && dtypep()->isSigned(); }

bool AstNode::isClassHandleValue() const {
    ආපසු (VN_IS(this, Const) && VN_AS(this, Const)->num().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool AstNode::isNull() const { ආපසු VN_IS(this, Const) && VN_AS(this, Const)->num().isNull(); }
bool AstNode::isZero() const {
    ආපසු (VN_IS(this, Const) && VN_AS(this, Const)->num().isEqZero());
}
bool AstNode::isNeqZero() const {
    ආපසු (VN_IS(this, Const) && VN_AS(this, Const)->num().isNeqZero());
}
bool AstNode::isOne() const { ආපසු (VN_IS(this, Const) && VN_AS(this, Const)->num().isEqOne()); }
bool AstNode::isAllOnes() const {
    ආපසු (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool AstNode::isAllOnesV() const {
    ආපසු (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool AstNode::sameTree(const AstNode* node2p) const {
    ආපසු sameTreeIter(this, node2p, true, false);
}
bool AstNode::sameGateTree(const AstNode* node2p) const {
    ආපසු sameTreeIter(this, node2p, true, true);
}

පූර්ණ_සංඛ්‍යා AstNodeArrayDType::left() const VL_MT_STABLE { ආපසු rangep()->leftConst(); }
පූර්ණ_සංඛ්‍යා AstNodeArrayDType::right() const VL_MT_STABLE { ආපසු rangep()->rightConst(); }
පූර්ණ_සංඛ්‍යා AstNodeArrayDType::hi() const VL_MT_STABLE { ආපසු rangep()->hiConst(); }
පූර්ණ_සංඛ්‍යා AstNodeArrayDType::lo() const VL_MT_STABLE { ආපසු rangep()->loConst(); }
පූර්ණ_සංඛ්‍යා AstNodeArrayDType::elementsConst() const VL_MT_STABLE { ආපසු rangep()->elementsConst(); }
VNumRange AstNodeArrayDType::declRange() const VL_MT_STABLE { ආපසු VNumRange{left(), right()}; }

AstFuncRef::AstFuncRef(FileLine* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(FileLine* fl, පූර්ණ_සංඛ්‍යා left, පූර්ණ_සංඛ්‍යා right)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(left)});
    rightp(new AstConst{fl, static_cast<uint32_t>(right)});
}
AstRange::AstRange(FileLine* fl, const VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.left())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.right())});
}
පූර්ණ_සංඛ්‍යා AstRange::leftConst() const VL_MT_STABLE {
    const AstConst* const constp = VN_CAST(leftp(), Const);
    ආපසු (constp ? constp->toSInt() : 0);
}
පූර්ණ_සංඛ්‍යා AstRange::rightConst() const VL_MT_STABLE {
    const AstConst* const constp = VN_CAST(rightp(), Const);
    ආපසු (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(FileLine* fl, VFlagChildDType, AstNodeDType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    const පූර්ණ_සංඛ්‍යා width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(FileLine* fl, AstNodeDType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    const පූර්ණ_සංඛ්‍යා width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

පූර්ණ_සංඛ්‍යා AstQueueDType::boundConst() const VL_MT_STABLE {
    const AstConst* const constp = VN_CAST(boundp(), Const);
    ආපසු (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(FileLine* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

පූර්ණ_සංඛ්‍යා AstBasicDType::hi() const { ආපසු (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
පූර්ණ_සංඛ්‍යා AstBasicDType::lo() const { ආපසු (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
පූර්ණ_සංඛ්‍යා AstBasicDType::elements() const {
    ආපසු (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() const {
    ආපසු (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() const { ආපසු m_sentreep->hasClocked(); }
bool AstActive::hasCombo() const { ආපසු m_sentreep->hasCombo(); }
bool AstActive::hasInitial() const { ආපසු m_sentreep->hasInitial(); }
bool AstActive::hasStatic() const { ආපසු m_sentreep->hasStatic(); }

bool AstNodeModule::isDollarUnit() const { ආපසු this == v3Global.rootp()->dollarUnitPkgp(); }
bool AstNodeModule::isConstPool() const { ආපසු this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(FileLine* fl, VDisplayType dispType, AstNodeExpr* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::sameNode(const AstNode* samep) const {
    const AstVar* const asamep = VN_DBG_AS(samep, Var);
    ආපසු m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(FileLine* fl, AstNodeExpr* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(FileLine* fl, AstVar* varp, const VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    නම් (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } නැතහොත් නම් (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(FileLine* fl, AstNodeModule* pkgp, AstVar* varp, const VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(FileLine* fl, AstVarScope* varscp, const VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() const { ආපසු varp() ? varp()->name() : nameThis(); }

bool AstVarRef::sameNode(const AstVarRef* samep) const {
    නම් (varScopep()) ආපසු (varScopep() == samep->varScopep() && access() == samep->access());
    ආපසු (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->sameNode(samep->varp())));
}
bool AstVarRef::sameNoLvalue(const AstVarRef* samep) const {
    නම් (varScopep()) ආපසු (varScopep() == samep->varScopep());

    ආපසු (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->sameNode(samep->varp()));
}

AstVarXRef::AstVarXRef(FileLine* fl, AstVar* varp, const string& dotted, const VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

AstStmtExpr* AstNodeExpr::makeStmt() { ආපසු new AstStmtExpr{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() const VL_MT_STABLE {
    ආපසු VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
anta_hstha AstNodeModule* findParentModule(AstNode* nodep) {
    සඳහා (AstNode* curp = nodep; curp; curp = curp->backp()) {
        නම් (AstNodeModule* const modp = VN_CAST(curp, NodeModule)) ආපසු modp;
    }
    ආපසු nullptr;
}

#endif  // Guard
