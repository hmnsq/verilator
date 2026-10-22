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

#شامل "V3PchAstMT.h"

#شامل "V3ConstPool.h"

#شامل "V3Hasher.h"

//######################################################################
// V3ConstPool

V3Hash V3ConstPool::hash_قدر(ثابت Ast_گرہ* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", جھوٹ, جھوٹ);
    اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) واپس hash + constp->عدد().مقصد_Hash();
    اگر (ثابت AstConsArray* ثابت consp = VN_CAST(nodep, ConsArray)) {
        ثابت uint64_t سائز
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        برائے (uint64_t n = 0; n < سائز; ++n) hash += hash_قدر(consp->valuep(n));
        واپس hash;
    }
    واپس hash + V3Hasher::uncachedHash(nodep);
}

منطقی V3ConstPool::equal_قدر(ثابت Ast_گرہ* ap, ثابت Ast_گرہ* bp) {
    اگر (ap->type() != bp->type()) واپس جھوٹ;
    اگر (ap->dtypep()->cType("", جھوٹ, جھوٹ) != bp->dtypep()->cType("", جھوٹ, جھوٹ)) {
        واپس جھوٹ;
    }
    // Compare constants by value, which also checks the width
    اگر (ثابت AstConst* ثابت constp = VN_CAST(ap, Const)) {
        واپس constp->عدد().isCaseEq(VN_AS(bp, Const)->عدد());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    اگر (ثابت AstConsArray* ثابت aConsp = VN_CAST(ap, ConsArray)) {
        ثابت AstConsArray* ثابت bConsp = VN_AS(bp, ConsArray);
        ثابت uint64_t سائز
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        برائے (uint64_t n = 0; n < سائز; ++n) {
            اگر (!equal_قدر(aConsp->valuep(n), bConsp->valuep(n))) واپس جھوٹ;
        }
        واپس سچ;
    }
    واپس ap->same_درخت(bp);
}

خالی V3ConstPool::rebuildCache() {
    m_cacheValid = سچ;
    m_scopep = خالی_اشاریہ;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* ثابت pkgp = v3Global.rootp()->constPoolPkgp();
    برائے (Ast_گرہ* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        اگر (AstScope* ثابت scopep = VN_CAST(nodep, Scope)) {
            اگر (m_stage == Stage::SCOPED) m_scopep = scopep;
            جاری;
        }
        // Otherwise must be a variable
        AstVar* ثابت varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->const_Pool_اندراج(), varp, "Unmarked variable in constant pool");
        m_entries.داخل_کرو(varp);
    }
    اگر (m_scopep) {
        برائے (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.داخل_کرو({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::ڈھونڈو(Ast_گرہ_اظہاریہ* initp) {
    UASSERT_OBJ(initp->is_Constant_درخت(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    اگر (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_سطر* ثابت flp = initp->fileline();
    Ast_گرہ_DType* ثابت dtypep = initp->dtypep();

    // Look up/create the variable with this value
    ثابت خودکار pair = self.m_entries.داخل_کرو_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_اگلا_نمبر[initp->type()];
        ثابت std::string name = std::string{initp->typeName()} + "_" + std::مقصد_string(numberr++);
        AstVar* ثابت varp = نیا AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->سیٹ_Const_Pool_اندراج();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(سچ);
        varp->valuep(initp->clone_درخت(جھوٹ));
        v3Global.rootp()->constPoolPkgp()->جوڑو_Stmtsp(varp);
        واپس varp;
    });
    AstVar* ثابت varp = *pair.پہلا;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = خالی_اشاریہ;
    انتخاب (self.m_stage) {
    حالت Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* ثابت pkgp = v3Global.rootp()->constPoolPkgp();
        refp = نیا AstVarRef{flp, pkgp, varp, VAccess::READ};
        توڑو;
    }
    حالت Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        ثابت خودکار vscpPair = self.m_varScopes.داخل_کرو_Lazy(varp, [&]() {
            AstVarScope* ثابت vscp = نیا AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->جوڑو_Varsp(vscp);
            واپس std::pair<ثابت AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = نیا AstVarRef{flp, vscpPair.پہلا->دوسرا, VAccess::READ};
        توڑو;
    }
    حالت Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = نیا AstVarRef{flp, varp, VAccess::READ};
        توڑو;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    واپس refp;
}

ثابت حرف* V3ConstPool::broken() {
    ثابت V3ConstPool& self = instance();
    اگر (!self.m_cacheValid) واپس خالی_اشاریہ;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    برائے (ثابت AstVar* ثابت varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    برائے (ثابت خودکار& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.پہلا->brokeExists());
        BROKEN_RTN(!pair.دوسرا->brokeExists());
    }
    واپس خالی_اشاریہ;
}
