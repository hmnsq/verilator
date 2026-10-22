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

ចំនួនគត់ AstNode::width() const VL_MT_STABLE { ត្រឡប់ dtypep() ? dtypep()->width() : 0; }
ចំនួនគត់ AstNode::widthMin() const VL_MT_STABLE { ត្រឡប់ dtypep() ? dtypep()->widthMin() : 0; }
bool AstNode::width1() const {  // V3Const uses to know it can optimize
    ត្រឡប់ dtypep() && dtypep()->width() == 1;
}
ចំនួនគត់ AstNode::widthInstrs() const {
    ត្រឡប់ (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->widthWords() : 1));
}
bool AstNode::isCHandle() const VL_MT_STABLE {
    ត្រឡប់ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool AstNode::isDouble() const VL_MT_STABLE {
    ត្រឡប់ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool AstNode::isString() const VL_MT_STABLE {
    ត្រឡប់ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool AstNode::isEvent() const VL_MT_STABLE {
    ត្រឡប់ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool AstNode::isSigned() const VL_MT_STABLE { ត្រឡប់ dtypep() && dtypep()->isSigned(); }

bool AstNode::isClassHandleValue() const {
    ត្រឡប់ (VN_IS(this, Const) && VN_AS(this, Const)->num().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool AstNode::isNull() const { ត្រឡប់ VN_IS(this, Const) && VN_AS(this, Const)->num().isNull(); }
bool AstNode::isZero() const {
    ត្រឡប់ (VN_IS(this, Const) && VN_AS(this, Const)->num().isEqZero());
}
bool AstNode::isNeqZero() const {
    ត្រឡប់ (VN_IS(this, Const) && VN_AS(this, Const)->num().isNeqZero());
}
bool AstNode::isOne() const { ត្រឡប់ (VN_IS(this, Const) && VN_AS(this, Const)->num().isEqOne()); }
bool AstNode::isAllOnes() const {
    ត្រឡប់ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool AstNode::isAllOnesV() const {
    ត្រឡប់ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool AstNode::sameTree(const AstNode* node2p) const {
    ត្រឡប់ sameTreeIter(this, node2p, true, false);
}
bool AstNode::sameGateTree(const AstNode* node2p) const {
    ត្រឡប់ sameTreeIter(this, node2p, true, true);
}

ចំនួនគត់ AstNodeArrayDType::left() const VL_MT_STABLE { ត្រឡប់ rangep()->leftConst(); }
ចំនួនគត់ AstNodeArrayDType::right() const VL_MT_STABLE { ត្រឡប់ rangep()->rightConst(); }
ចំនួនគត់ AstNodeArrayDType::hi() const VL_MT_STABLE { ត្រឡប់ rangep()->hiConst(); }
ចំនួនគត់ AstNodeArrayDType::lo() const VL_MT_STABLE { ត្រឡប់ rangep()->loConst(); }
ចំនួនគត់ AstNodeArrayDType::elementsConst() const VL_MT_STABLE { ត្រឡប់ rangep()->elementsConst(); }
VNumRange AstNodeArrayDType::declRange() const VL_MT_STABLE { ត្រឡប់ VNumRange{left(), right()}; }

AstFuncRef::AstFuncRef(FileLine* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(FileLine* fl, ចំនួនគត់ left, ចំនួនគត់ right)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(left)});
    rightp(new AstConst{fl, static_cast<uint32_t>(right)});
}
AstRange::AstRange(FileLine* fl, const VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.left())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.right())});
}
ចំនួនគត់ AstRange::leftConst() const VL_MT_STABLE {
    const AstConst* const constp = VN_CAST(leftp(), Const);
    ត្រឡប់ (constp ? constp->toSInt() : 0);
}
ចំនួនគត់ AstRange::rightConst() const VL_MT_STABLE {
    const AstConst* const constp = VN_CAST(rightp(), Const);
    ត្រឡប់ (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(FileLine* fl, VFlagChildDType, AstNodeDType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    const ចំនួនគត់ width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(FileLine* fl, AstNodeDType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    const ចំនួនគត់ width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

ចំនួនគត់ AstQueueDType::boundConst() const VL_MT_STABLE {
    const AstConst* const constp = VN_CAST(boundp(), Const);
    ត្រឡប់ (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(FileLine* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

ចំនួនគត់ AstBasicDType::hi() const { ត្រឡប់ (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
ចំនួនគត់ AstBasicDType::lo() const { ត្រឡប់ (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
ចំនួនគត់ AstBasicDType::elements() const {
    ត្រឡប់ (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() const {
    ត្រឡប់ (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() const { ត្រឡប់ m_sentreep->hasClocked(); }
bool AstActive::hasCombo() const { ត្រឡប់ m_sentreep->hasCombo(); }
bool AstActive::hasInitial() const { ត្រឡប់ m_sentreep->hasInitial(); }
bool AstActive::hasStatic() const { ត្រឡប់ m_sentreep->hasStatic(); }

bool AstNodeModule::isDollarUnit() const { ត្រឡប់ this == v3Global.rootp()->dollarUnitPkgp(); }
bool AstNodeModule::isConstPool() const { ត្រឡប់ this == v3Global.rootp()->constPoolPkgp(); }

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
    ត្រឡប់ m_name == asamep->m_name && varType() == asamep->varType()
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
    ប្រសិនបើ (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } បើមិនដូច្នេះ ប្រសិនបើ (varp) {
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

string AstVarRef::name() const { ត្រឡប់ varp() ? varp()->name() : nameThis(); }

bool AstVarRef::sameNode(const AstVarRef* samep) const {
    ប្រសិនបើ (varScopep()) ត្រឡប់ (varScopep() == samep->varScopep() && access() == samep->access());
    ត្រឡប់ (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->sameNode(samep->varp())));
}
bool AstVarRef::sameNoLvalue(const AstVarRef* samep) const {
    ប្រសិនបើ (varScopep()) ត្រឡប់ (varScopep() == samep->varScopep());

    ត្រឡប់ (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->sameNode(samep->varp()));
}

AstVarXRef::AstVarXRef(FileLine* fl, AstVar* varp, const string& dotted, const VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

AstStmtExpr* AstNodeExpr::makeStmt() { ត្រឡប់ new AstStmtExpr{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() const VL_MT_STABLE {
    ត្រឡប់ VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
anta_hstha AstNodeModule* findParentModule(AstNode* nodep) {
    សម្រាប់ (AstNode* curp = nodep; curp; curp = curp->backp()) {
        ប្រសិនបើ (AstNodeModule* const modp = VN_CAST(curp, NodeModule)) ត្រឡប់ modp;
    }
    ត្រឡប់ nullptr;
}

#endif  // Guard
