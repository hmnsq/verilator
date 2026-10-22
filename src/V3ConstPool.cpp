// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Constant pool
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ضمن "V3PchAstMT.h"

#ضمن "V3ConstPool.h"

#ضمن "V3Hasher.h"

//######################################################################
// V3ConstPool

V3Hash V3ConstPool::hash_قدر(ثابت Ast_گرہ* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", خطأ, خطأ);
    إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) أرجع hash + constp->عدد().منزل_Hash();
    إذا (ثابت AstConsArray* ثابت consp = VN_CAST(nodep, ConsArray)) {
        ثابت uint64_t ناپ
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        لكل (uint64_t n = 0; n < ناپ; ++n) hash += hash_قدر(consp->valuep(n));
        أرجع hash;
    }
    أرجع hash + V3Hasher::uncachedHash(nodep);
}

منطقي V3ConstPool::equal_قدر(ثابت Ast_گرہ* ap, ثابت Ast_گرہ* bp) {
    إذا (ap->type() != bp->type()) أرجع خطأ;
    إذا (ap->dtypep()->cType("", خطأ, خطأ) != bp->dtypep()->cType("", خطأ, خطأ)) {
        أرجع خطأ;
    }
    // Compare constants by value, which also checks the width
    إذا (ثابت AstConst* ثابت constp = VN_CAST(ap, Const)) {
        أرجع constp->عدد().isCaseEq(VN_AS(bp, Const)->عدد());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    إذا (ثابت AstConsArray* ثابت aConsp = VN_CAST(ap, ConsArray)) {
        ثابت AstConsArray* ثابت bConsp = VN_AS(bp, ConsArray);
        ثابت uint64_t ناپ
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        لكل (uint64_t n = 0; n < ناپ; ++n) {
            إذا (!equal_قدر(aConsp->valuep(n), bConsp->valuep(n))) أرجع خطأ;
        }
        أرجع سچ;
    }
    أرجع ap->same_درخت(bp);
}

فراغ V3ConstPool::rebuildCache() {
    m_cacheValid = سچ;
    m_scopep = خالی_سوچک;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* ثابت pkgp = v3Global.rootp()->constPoolPkgp();
    لكل (Ast_گرہ* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        إذا (AstScope* ثابت scopep = VN_CAST(nodep, Scope)) {
            إذا (m_stage == Stage::SCOPED) m_scopep = scopep;
            استمر;
        }
        // Otherwise must be a variable
        AstVar* ثابت varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->const_Pool_اندراج(), varp, "Unmarked variable in constant pool");
        m_entries.داخل_کرو(varp);
    }
    إذا (m_scopep) {
        لكل (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.داخل_کرو({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::لبھو(Ast_گرہ_اظہاریہ* initp) {
    UASSERT_OBJ(initp->is_Constant_درخت(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    إذا (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_سطر* ثابت flp = initp->fileline();
    Ast_گرہ_DType* ثابت dtypep = initp->dtypep();

    // Look up/create the variable with this value
    ثابت خودکار pair = self.m_entries.داخل_کرو_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_اگلا_نمبر[initp->type()];
        ثابت std::string name = std::string{initp->typeName()} + "_" + std::منزل_string(numberr++);
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->سیٹ_Const_Pool_اندراج();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(سچ);
        varp->valuep(initp->clone_درخت(خطأ));
        v3Global.rootp()->constPoolPkgp()->جوڑو_Stmtsp(varp);
        أرجع varp;
    });
    AstVar* ثابت varp = *pair.پہلا;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = خالی_سوچک;
    بدل (self.m_stage) {
    حالة Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* ثابت pkgp = v3Global.rootp()->constPoolPkgp();
        refp = جديد AstVarRef{flp, pkgp, varp, VAccess::READ};
        اكسر;
    }
    حالة Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        ثابت خودکار vscpPair = self.m_varScopes.داخل_کرو_Lazy(varp, [&]() {
            AstVarScope* ثابت vscp = جديد AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->جوڑو_Varsp(vscp);
            أرجع std::pair<ثابت AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = جديد AstVarRef{flp, vscpPair.پہلا->دوجا, VAccess::READ};
        اكسر;
    }
    حالة Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = جديد AstVarRef{flp, varp, VAccess::READ};
        اكسر;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    أرجع refp;
}

ثابت حرف* V3ConstPool::broken() {
    ثابت V3ConstPool& self = instance();
    إذا (!self.m_cacheValid) أرجع خالی_سوچک;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    لكل (ثابت AstVar* ثابت varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    لكل (ثابت خودکار& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.پہلا->brokeExists());
        BROKEN_RTN(!pair.دوجا->brokeExists());
    }
    أرجع خالی_سوچک;
}
