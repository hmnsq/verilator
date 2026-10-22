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

V3Hash V3ConstPool::hash_بەها(ثابت Ast_گرێ* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", خطأ, خطأ);
    إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) أرجع hash + constp->ژمارە().toHash();
    إذا (ثابت AstConsArray* ثابت consp = VN_CAST(nodep, ConsArray)) {
        ثابت uint64_t size
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        لكل (uint64_t n = 0; n < size; ++n) hash += hash_بەها(consp->valuep(n));
        أرجع hash;
    }
    أرجع hash + V3Hasher::uncachedHash(nodep);
}

منطقي V3ConstPool::equal_بەها(ثابت Ast_گرێ* ap, ثابت Ast_گرێ* bp) {
    إذا (ap->type() != bp->type()) أرجع خطأ;
    إذا (ap->dtypep()->cType("", خطأ, خطأ) != bp->dtypep()->cType("", خطأ, خطأ)) {
        أرجع خطأ;
    }
    // Compare constants by value, which also checks the width
    إذا (ثابت AstConst* ثابت constp = VN_CAST(ap, Const)) {
        أرجع constp->ژمارە().isCaseEq(VN_AS(bp, Const)->ژمارە());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    إذا (ثابت AstConsArray* ثابت aConsp = VN_CAST(ap, ConsArray)) {
        ثابت AstConsArray* ثابت bConsp = VN_AS(bp, ConsArray);
        ثابت uint64_t size
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        لكل (uint64_t n = 0; n < size; ++n) {
            إذا (!equal_بەها(aConsp->valuep(n), bConsp->valuep(n))) أرجع خطأ;
        }
        أرجع true;
    }
    أرجع ap->same_درەخت(bp);
}

فراغ V3ConstPool::rebuildCache() {
    m_cacheValid = true;
    m_scopep = nullptr;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* ثابت pkgp = v3Global.rootp()->constPoolPkgp();
    لكل (Ast_گرێ* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        إذا (AstScope* ثابت scopep = VN_CAST(nodep, Scope)) {
            إذا (m_stage == Stage::SCOPED) m_scopep = scopep;
            استمر;
        }
        // Otherwise must be a variable
        AstVar* ثابت varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.زیادکردن(varp);
    }
    إذا (m_scopep) {
        لكل (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.زیادکردن({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(Ast_گرێ_دەربڕین* initp) {
    UASSERT_OBJ(initp->is_Constant_درەخت(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    إذا (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_هێڵ* ثابت flp = initp->fileline();
    Ast_گرێ_DType* ثابت dtypep = initp->dtypep();

    // Look up/create the variable with this value
    ثابت auto pair = self.m_entries.زیادکردن_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_next_ژمار[initp->type()];
        ثابت std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(true);
        varp->valuep(initp->clone_درەخت(خطأ));
        v3Global.rootp()->constPoolPkgp()->addStmtsp(varp);
        أرجع varp;
    });
    AstVar* ثابت varp = *pair.first;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = nullptr;
    بدل (self.m_stage) {
    حالة Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* ثابت pkgp = v3Global.rootp()->constPoolPkgp();
        refp = جديد AstVarRef{flp, pkgp, varp, VAccess::READ};
        اكسر;
    }
    حالة Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        ثابت auto vscpPair = self.m_varScopes.زیادکردن_Lazy(varp, [&]() {
            AstVarScope* ثابت vscp = جديد AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->addVarsp(vscp);
            أرجع std::pair<ثابت AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = جديد AstVarRef{flp, vscpPair.first->second, VAccess::READ};
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
    إذا (!self.m_cacheValid) أرجع nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    لكل (ثابت AstVar* ثابت varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    لكل (ثابت auto& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    أرجع nullptr;
}
