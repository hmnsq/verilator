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

V3Hash V3ConstPool::hash_مقدار(ثابت Ast_گره* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", نادرست, نادرست);
    اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) بازگشت hash + constp->رقم().toHash();
    اگر (ثابت AstConsArray* ثابت consp = VN_CAST(nodep, ConsArray)) {
        ثابت uint64_t size
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        برای (uint64_t n = 0; n < size; ++n) hash += hash_مقدار(consp->valuep(n));
        بازگشت hash;
    }
    بازگشت hash + V3Hasher::uncachedHash(nodep);
}

منطقی V3ConstPool::equal_مقدار(ثابت Ast_گره* ap, ثابت Ast_گره* bp) {
    اگر (ap->type() != bp->type()) بازگشت نادرست;
    اگر (ap->dtypep()->cType("", نادرست, نادرست) != bp->dtypep()->cType("", نادرست, نادرست)) {
        بازگشت نادرست;
    }
    // Compare constants by value, which also checks the width
    اگر (ثابت AstConst* ثابت constp = VN_CAST(ap, Const)) {
        بازگشت constp->رقم().isCaseEq(VN_AS(bp, Const)->رقم());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    اگر (ثابت AstConsArray* ثابت aConsp = VN_CAST(ap, ConsArray)) {
        ثابت AstConsArray* ثابت bConsp = VN_AS(bp, ConsArray);
        ثابت uint64_t size
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        برای (uint64_t n = 0; n < size; ++n) {
            اگر (!equal_مقدار(aConsp->valuep(n), bConsp->valuep(n))) بازگشت نادرست;
        }
        بازگشت درست;
    }
    بازگشت ap->same_درخت(bp);
}

تهی V3ConstPool::rebuildCache() {
    m_cacheValid = درست;
    m_scopep = nullptr;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* ثابت pkgp = v3Global.rootp()->constPoolPkgp();
    برای (Ast_گره* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        اگر (AstScope* ثابت scopep = VN_CAST(nodep, Scope)) {
            اگر (m_stage == Stage::SCOPED) m_scopep = scopep;
            ادامه;
        }
        // Otherwise must be a variable
        AstVar* ثابت varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.درج(varp);
    }
    اگر (m_scopep) {
        برای (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.درج({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(Ast_گره_عبارت* initp) {
    UASSERT_OBJ(initp->is_Constant_درخت(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    اگر (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_سطر* ثابت flp = initp->fileline();
    Ast_گره_DType* ثابت dtypep = initp->dtypep();

    // Look up/create the variable with this value
    ثابت auto pair = self.m_entries.درج_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_next_عدد[initp->type()];
        ثابت std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* ثابت varp = جدید AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(درست);
        varp->valuep(initp->clone_درخت(نادرست));
        v3Global.rootp()->constPoolPkgp()->addStmtsp(varp);
        بازگشت varp;
    });
    AstVar* ثابت varp = *pair.first;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = nullptr;
    انتخاب (self.m_stage) {
    حالت Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* ثابت pkgp = v3Global.rootp()->constPoolPkgp();
        refp = جدید AstVarRef{flp, pkgp, varp, VAccess::READ};
        بشکن;
    }
    حالت Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        ثابت auto vscpPair = self.m_varScopes.درج_Lazy(varp, [&]() {
            AstVarScope* ثابت vscp = جدید AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->addVarsp(vscp);
            بازگشت std::pair<ثابت AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = جدید AstVarRef{flp, vscpPair.first->second, VAccess::READ};
        بشکن;
    }
    حالت Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = جدید AstVarRef{flp, varp, VAccess::READ};
        بشکن;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    بازگشت refp;
}

ثابت نویسه* V3ConstPool::broken() {
    ثابت V3ConstPool& self = instance();
    اگر (!self.m_cacheValid) بازگشت nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    برای (ثابت AstVar* ثابت varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    برای (ثابت auto& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    بازگشت nullptr;
}
