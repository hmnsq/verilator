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
#അന്തര്ഭാവ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#endif

//######################################################################
// Inline METHODS

പൂർണ്ണസംഖ്യ Ast_കണ്ണി::width() സ്ഥിരം VL_MT_STABLE { തിരികെ dtypep() ? dtypep()->width() : 0; }
പൂർണ്ണസംഖ്യ Ast_കണ്ണി::widthMin() സ്ഥിരം VL_MT_STABLE { തിരികെ dtypep() ? dtypep()->widthMin() : 0; }
bool Ast_കണ്ണി::width1() സ്ഥിരം {  // V3Const uses to know it can optimize
    തിരികെ dtypep() && dtypep()->width() == 1;
}
പൂർണ്ണസംഖ്യ Ast_കണ്ണി::widthInstrs() സ്ഥിരം {
    തിരികെ (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->width_വാക്കുകൾ() : 1));
}
bool Ast_കണ്ണി::isCHandle() സ്ഥിരം VL_MT_STABLE {
    തിരികെ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isCHandle();
}
bool Ast_കണ്ണി::isDouble() സ്ഥിരം VL_MT_STABLE {
    തിരികെ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isDouble();
}
bool Ast_കണ്ണി::isString() സ്ഥിരം VL_MT_STABLE {
    തിരികെ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool Ast_കണ്ണി::isEvent() സ്ഥിരം VL_MT_STABLE {
    തിരികെ dtypep() && dtypep()->basicp() && dtypep()->basicp()->isEvent();
}

bool Ast_കണ്ണി::isSigned() സ്ഥിരം VL_MT_STABLE { തിരികെ dtypep() && dtypep()->isSigned(); }

bool Ast_കണ്ണി::is_Class_Handle_മൂല്യം() സ്ഥിരം {
    തിരികെ (VN_IS(this, Const) && VN_AS(this, Const)->അക്കം().isNull())
           || (dtypep() && VN_IS(dtypep()->skipRefp(), ClassRefDType));
}
bool Ast_കണ്ണി::isNull() സ്ഥിരം { തിരികെ VN_IS(this, Const) && VN_AS(this, Const)->അക്കം().isNull(); }
bool Ast_കണ്ണി::isZero() സ്ഥിരം {
    തിരികെ (VN_IS(this, Const) && VN_AS(this, Const)->അക്കം().isEqZero());
}
bool Ast_കണ്ണി::isNeqZero() സ്ഥിരം {
    തിരികെ (VN_IS(this, Const) && VN_AS(this, Const)->അക്കം().isNeqZero());
}
bool Ast_കണ്ണി::isOne() സ്ഥിരം { തിരികെ (VN_IS(this, Const) && VN_AS(this, Const)->അക്കം().isEqOne()); }
bool Ast_കണ്ണി::isAllOnes() സ്ഥിരം {
    തിരികെ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool Ast_കണ്ണി::isAllOnesV() സ്ഥിരം {
    തിരികെ (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool Ast_കണ്ണി::same_വൃക്ഷം(സ്ഥിരം Ast_കണ്ണി* node2p) സ്ഥിരം {
    തിരികെ same_വൃക്ഷം_Iter(this, node2p, true, false);
}
bool Ast_കണ്ണി::same_Gate_വൃക്ഷം(സ്ഥിരം Ast_കണ്ണി* node2p) സ്ഥിരം {
    തിരികെ same_വൃക്ഷം_Iter(this, node2p, true, true);
}

പൂർണ്ണസംഖ്യ Ast_കണ്ണി_Array_DType::ഇടത്() സ്ഥിരം VL_MT_STABLE { തിരികെ rangep()->ഇടത്_Const(); }
പൂർണ്ണസംഖ്യ Ast_കണ്ണി_Array_DType::വലത്() സ്ഥിരം VL_MT_STABLE { തിരികെ rangep()->വലത്_Const(); }
പൂർണ്ണസംഖ്യ Ast_കണ്ണി_Array_DType::hi() സ്ഥിരം VL_MT_STABLE { തിരികെ rangep()->hiConst(); }
പൂർണ്ണസംഖ്യ Ast_കണ്ണി_Array_DType::lo() സ്ഥിരം VL_MT_STABLE { തിരികെ rangep()->loConst(); }
പൂർണ്ണസംഖ്യ Ast_കണ്ണി_Array_DType::elementsConst() സ്ഥിരം VL_MT_STABLE { തിരികെ rangep()->elementsConst(); }
VNumRange Ast_കണ്ണി_Array_DType::declRange() സ്ഥിരം VL_MT_STABLE { തിരികെ VNumRange{ഇടത്(), വലത്()}; }

AstFuncRef::AstFuncRef(File_വരി* fl, AstFunc* taskp, AstArg* argsp)
    : ASTGEN_SUPER_FuncRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeFrom(taskp);
}

AstRange::AstRange(File_വരി* fl, പൂർണ്ണസംഖ്യ ഇടത്, പൂർണ്ണസംഖ്യ വലത്)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(ഇടത്)});
    rightp(new AstConst{fl, static_cast<uint32_t>(വലത്)});
}
AstRange::AstRange(File_വരി* fl, സ്ഥിരം VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.ഇടത്())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.വലത്())});
}
പൂർണ്ണസംഖ്യ AstRange::ഇടത്_Const() സ്ഥിരം VL_MT_STABLE {
    സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(leftp(), Const);
    തിരികെ (constp ? constp->toSInt() : 0);
}
പൂർണ്ണസംഖ്യ AstRange::വലത്_Const() സ്ഥിരം VL_MT_STABLE {
    സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(rightp(), Const);
    തിരികെ (constp ? constp->toSInt() : 0);
}

AstPackArrayDType::AstPackArrayDType(File_വരി* fl, VFlagChildDType, Ast_കണ്ണി_DType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    സ്ഥിരം പൂർണ്ണസംഖ്യ width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(File_വരി* fl, Ast_കണ്ണി_DType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    സ്ഥിരം പൂർണ്ണസംഖ്യ width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

പൂർണ്ണസംഖ്യ AstQueueDType::boundConst() സ്ഥിരം VL_MT_STABLE {
    സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(boundp(), Const);
    തിരികെ (constp ? constp->toSInt() : 0);
}

AstTaskRef::AstTaskRef(File_വരി* fl, AstTask* taskp, AstArg* argsp)
    : ASTGEN_SUPER_TaskRef(fl, taskp->name(), argsp) {
    this->taskp(taskp);
    dtypeSetVoid();
}

പൂർണ്ണസംഖ്യ AstBasicDType::hi() സ്ഥിരം { തിരികെ (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
പൂർണ്ണസംഖ്യ AstBasicDType::lo() സ്ഥിരം { തിരികെ (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
പൂർണ്ണസംഖ്യ AstBasicDType::elements() സ്ഥിരം {
    തിരികെ (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() സ്ഥിരം {
    തിരികെ (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() സ്ഥിരം { തിരികെ m_sentreep->hasClocked(); }
bool AstActive::hasCombo() സ്ഥിരം { തിരികെ m_sentreep->hasCombo(); }
bool AstActive::hasInitial() സ്ഥിരം { തിരികെ m_sentreep->hasInitial(); }
bool AstActive::hasStatic() സ്ഥിരം { തിരികെ m_sentreep->hasStatic(); }

bool Ast_കണ്ണി_Module::isDollarUnit() സ്ഥിരം { തിരികെ this == v3Global.rootp()->dollarUnitPkgp(); }
bool Ast_കണ്ണി_Module::isConstPool() സ്ഥിരം { തിരികെ this == v3Global.rootp()->constPoolPkgp(); }

AstAlways::AstAlways(AstAssignW* assignp)
    : ASTGEN_SUPER_Always(assignp->fileline(), assignp)
    , m_keyword{VAlwaysKwd::CONT_ASSIGN} {}

AstElabDisplay::AstElabDisplay(File_വരി* fl, VDisplayType dispType, Ast_കണ്ണി_വ്യഞ്ജകം* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, "", true, exprsp});
    m_displayType = dispType;
}

bool AstVar::same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം {
    സ്ഥിരം AstVar* സ്ഥിരം asamep = VN_DBG_AS(samep, Var);
    തിരികെ m_name == asamep->m_name && varType() == asamep->varType()
           && covergroupRefMember() == asamep->covergroupRefMember()
           && libInoutId() == asamep->libInoutId() && libInoutEnable() == asamep->libInoutEnable();
}

AstMatchMasked::AstMatchMasked(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, AstVarRef* matchp)
    : ASTGEN_SUPER_MatchMasked(fl) {
    this->lhsp(lhsp);
    this->matchp(matchp);
    dtypeSetUInt32();
}

AstVarRef::AstVarRef(File_വരി* fl, AstVar* varp, സ്ഥിരം VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp, access) {
    എങ്കിൽ (v3Global.assertDTypesResolved()) {
        UASSERT_OBJ(varp, this, "Require non-null varp post resolution");
    } അല്ലെങ്കിൽ എങ്കിൽ (varp) {
        m_name = varp->name();
    }
}
AstVarRef::AstVarRef(File_വരി* fl, Ast_കണ്ണി_Module* pkgp, AstVar* varp, സ്ഥിരം VAccess& access)
    : AstVarRef{fl, varp, access} {
    classOrPackagep(pkgp);
}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(File_വരി* fl, AstVarScope* varscp, സ്ഥിരം VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp(), access) {
    varScopep(varscp);
}

string AstVarRef::name() സ്ഥിരം { തിരികെ varp() ? varp()->name() : nameThis(); }

bool AstVarRef::same_കണ്ണി(സ്ഥിരം AstVarRef* samep) സ്ഥിരം {
    എങ്കിൽ (varScopep()) തിരികെ (varScopep() == samep->varScopep() && access() == samep->access());
    തിരികെ (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && access() == samep->access()
            && (varp() && samep->varp() && varp()->same_കണ്ണി(samep->varp())));
}
bool AstVarRef::sameNoLvalue(സ്ഥിരം AstVarRef* samep) സ്ഥിരം {
    എങ്കിൽ (varScopep()) തിരികെ (varScopep() == samep->varScopep());

    തിരികെ (selfPointer() == samep->selfPointer() && classOrPackagep() == samep->classOrPackagep()
            && (!selfPointer().isEmpty() || !samep->selfPointer().isEmpty())
            && varp()->same_കണ്ണി(samep->varp()));
}

AstVarXRef::AstVarXRef(File_വരി* fl, AstVar* varp, സ്ഥിരം string& dotted, സ്ഥിരം VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp, access)
    , m_name{varp->name()}
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

Ast_Stmt_വ്യഞ്ജകം* Ast_കണ്ണി_വ്യഞ്ജകം::makeStmt() { തിരികെ new Ast_Stmt_വ്യഞ്ജകം{fileline(), this}; }

AstConsArrayItem* AstConsArray::itemsp() സ്ഥിരം VL_MT_STABLE {
    തിരികെ VN_DBG_AS(op2p(), ConsArrayItem);
}

// Walk up the AST via backp() to find the containing AstNodeModule.
// Returns nullptr if not found.
വരിയിൽ Ast_കണ്ണി_Module* findParentModule(Ast_കണ്ണി* nodep) {
    വേണ്ടി (Ast_കണ്ണി* curp = nodep; curp; curp = curp->backp()) {
        എങ്കിൽ (Ast_കണ്ണി_Module* സ്ഥിരം modp = VN_CAST(curp, കണ്ണി_Module)) തിരികെ modp;
    }
    തിരികെ nullptr;
}

#endif  // Guard
