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
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#endif

//######################################################################
// Inline METHODS

صحيح AstNode::width() ثابت VL_MT_STABLE { ܦܢܐ dtypep() ? dtypep()->width() : 0; }
صحيح AstNode::widthMin() ثابت VL_MT_STABLE { ܦܢܐ dtypep() ? dtypep()->widthMin() : 0; }
bool AstNode::width1() ثابت {  // V3Const uses to know it can optimize
    ܦܢܐ dtypep() && dtypep()->width() == 1;
}
صحيح AstNode::widthInstrs() ثابت {
    ܦܢܐ (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->widthWords() : 1));
}
bool AstNode::isCHandle() ثابت VL_MT_STABLE {
    ܦܢܐ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool AstNode::isDouble() ثابت VL_MT_STABLE {
    ܦܢܐ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool AstNode::isString() ثابت VL_MT_STABLE {
    ܦܢܐ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool AstNode::isEvent() ثابت VL_MT_STABLE {
    ܦܢܐ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool AstNode::isSigned() ثابت VL_MT_STABLE { ܦܢܐ dtypep() && dtypep()->isSigned(); }

bool AstNode::isClassHandleValue() ثابت {
    ܦܢܐ (VN_IS(this, Const) && VN_AS(this, Const)->num().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool AstNode::isNull() ثابت { ܦܢܐ VN_IS(this, Const) && VN_AS(this, Const)->num().isNull(); }
bool AstNode::isZero() ثابت {
    ܦܢܐ (VN_IS(this, Const) && VN_AS(this, Const)->num().isEqZero());
}
bool AstNode::isNeqZero() ثابت {
    ܦܢܐ (VN_IS(this, Const) && VN_AS(this, Const)->num().isNeqZero());
}
bool AstNode::isOne() ثابت { ܦܢܐ (VN_IS(this, Const) && VN_AS(this, Const)->num().isEqOne()); }
bool AstNode::isAllOnes() ثابت {
    ܦܢܐ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool AstNode::isAllOnesV() ثابت {
    ܦܢܐ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool AstNode::sameTree(ثابت AstNode* node2p) ثابت {
    ܦܢܐ sameTreeIter(this, node2p, true, false);
}
bool AstNode::sameGateTree(ثابت AstNode* node2p) ثابت {
    ܦܢܐ sameTreeIter(this, node2p, true, true);
}

صحيح AstNodeArrayDType::left() ثابت VL_MT_STABLE { ܦܢܐ rangep()->leftConst(); }
صحيح AstNodeArrayDType::right() ثابت VL_MT_STABLE { ܦܢܐ rangep()->rightConst(); }
صحيح AstNodeArrayDType::hi() ثابت VL_MT_STABLE { ܦܢܐ rangep()->hiConst(); }
صحيح AstNodeArrayDType::lo() ثابت VL_MT_STABLE { ܦܢܐ rangep()->loConst(); }
صحيح AstNodeArrayDType::elementsConst() ثابت VL_MT_STABLE { ܦܢܐ rangep()->elementsConst(); }
VNumRange AstNodeArrayDType::declRange() ثابت VL_MT_STABLE { ܦܢܐ VNumRange{left(), right()}; }

AstFuncRef::AstFuncRef(FileLine* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(FileLine* fl, صحيح left, صحيح right)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(left)});
    rightp(new AstConst{fl, static_cast<uint32_t>(right)});
}
AstRange::AstRange(FileLine* fl, ثابت VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.left())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.right())});
}
صحيح AstRange::leftConst() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(leftp(), Const);
    ܦܢܐ (constp ? constp->toSInt() : 0);
}
صحيح AstRange::rightConst() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(rightp(), Const);
    ܦܢܐ (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(FileLine* fl, VFlagChildDType, AstNodeDType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    ثابت صحيح width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(FileLine* fl, AstNodeDType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    ثابت صحيح width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

صحيح AstQueueDType::boundConst() ثابت VL_MT_STABLE {
    ثابت AstConst* ثابت constp = VN_CAST(boundp(), Const);
    ܦܢܐ (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(FileLine* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

صحيح AstBasicDType::hi() ثابت { ܦܢܐ (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
صحيح AstBasicDType::lo() ثابت { ܦܢܐ (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
صحيح AstBasicDType::elements() ثابت {
    ܦܢܐ (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() ثابت {
    ܦܢܐ (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() ثابت { ܦܢܐ m_sentreep->hasClocked(); }
bool AstActive::hasCombo() ثابت { ܦܢܐ m_sentreep->hasCombo(); }
bool AstActive::hasInitial() ثابت { ܦܢܐ m_sentreep->hasInitial(); }
bool AstActive::hasStatic() ثابت { ܦܢܐ m_sentreep->hasStatic(); }

bool AstNodeModule::isDollarUnit() ثابت { ܦܢܐ this == v3Global.rootp()->dollarUnitPkgp(); }
bool AstNodeModule::isConstPool() ثابت { ܦܢܐ this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(FileLine* fl, VDisplayType dispType, AstNodeExpr* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::sameNode(ثابت AstNode* samep) ثابت {
    ثابت AstVar* ثابت asamep = VN_DBG_AS(samep, Var);
    ܦܢܐ m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(FileLine* fl, AstNodeExpr* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(FileLine* fl, AstVar* varp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    ܐܢ (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } ܐܚܪܢܐ ܐܢ (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(FileLine* fl, AstNodeModule* pkgp, AstVar* varp, ثابت VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(FileLine* fl, AstVarScope* varscp, ثابت VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() ثابت { ܦܢܐ varp() ? varp()->name() : nameThis(); }

bool AstVarRef::sameNode(ثابت AstVarRef* samep) ثابت {
    ܐܢ (varScopep()) ܦܢܐ (varScopep() == samep->varScopep() && access() == samep->access());
    ܦܢܐ (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->sameNode(samep->varp())));
}
bool AstVarRef::sameNoLvalue(ثابت AstVarRef* samep) ثابت {
    ܐܢ (varScopep()) ܦܢܐ (varScopep() == samep->varScopep());

    ܦܢܐ (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->sameNode(samep->varp()));
}

AstVarXRef::AstVarXRef(FileLine* fl, AstVar* varp, ثابت string& dotted, ثابت VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

AstStmtExpr* AstNodeExpr::makeStmt() { ܦܢܐ new AstStmtExpr{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() ثابت VL_MT_STABLE {
    ܦܢܐ VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
مضمن AstNodeModule* findParentModule(AstNode* nodep) {
    لكل (AstNode* curp = nodep; curp; curp = curp->backp()) {
        ܐܢ (AstNodeModule* ثابت modp = VN_CAST(curp, NodeModule)) ܦܢܐ modp;
    }
    ܦܢܐ nullptr;
}

#endif  // Guard
