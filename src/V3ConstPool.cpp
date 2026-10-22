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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ConstPool.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hasher.h"

//######################################################################
// V3ConstPool

V3Hash V3ConstPool::hash_ਮੁੱਲ(ਅਚਲ Ast_ਗੰਢ* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", ਝੂਠ, ਝੂਠ);
    ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep, Const)) ਵਾਪਸ hash + constp->ਅੰਕ().ਮੰਜ਼ਿਲ_Hash();
    ਜੇ (ਅਚਲ AstConsArray* ਅਚਲ consp = VN_CAST(nodep, ConsArray)) {
        ਅਚਲ uint64_t ਮਾਪ
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        ਲਈ (uint64_t n = 0; n < ਮਾਪ; ++n) hash += hash_ਮੁੱਲ(consp->valuep(n));
        ਵਾਪਸ hash;
    }
    ਵਾਪਸ hash + V3Hasher::uncachedHash(nodep);
}

ਤਾਰਕਿਕ V3ConstPool::equal_ਮੁੱਲ(ਅਚਲ Ast_ਗੰਢ* ap, ਅਚਲ Ast_ਗੰਢ* bp) {
    ਜੇ (ap->type() != bp->type()) ਵਾਪਸ ਝੂਠ;
    ਜੇ (ap->dtypep()->cType("", ਝੂਠ, ਝੂਠ) != bp->dtypep()->cType("", ਝੂਠ, ਝੂਠ)) {
        ਵਾਪਸ ਝੂਠ;
    }
    // Compare constants by value, which also checks the width
    ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(ap, Const)) {
        ਵਾਪਸ constp->ਅੰਕ().isCaseEq(VN_AS(bp, Const)->ਅੰਕ());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    ਜੇ (ਅਚਲ AstConsArray* ਅਚਲ aConsp = VN_CAST(ap, ConsArray)) {
        ਅਚਲ AstConsArray* ਅਚਲ bConsp = VN_AS(bp, ConsArray);
        ਅਚਲ uint64_t ਮਾਪ
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        ਲਈ (uint64_t n = 0; n < ਮਾਪ; ++n) {
            ਜੇ (!equal_ਮੁੱਲ(aConsp->valuep(n), bConsp->valuep(n))) ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ ਸੱਚ;
    }
    ਵਾਪਸ ap->same_ਰੁੱਖ(bp);
}

ਖਾਲੀ V3ConstPool::rebuildCache() {
    m_cacheValid = ਸੱਚ;
    m_scopep = ਖਾਲੀ_ਸੂਚਕ;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* ਅਚਲ pkgp = v3Global.rootp()->constPoolPkgp();
    ਲਈ (Ast_ਗੰਢ* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        ਜੇ (AstScope* ਅਚਲ scopep = VN_CAST(nodep, Scope)) {
            ਜੇ (m_stage == Stage::SCOPED) m_scopep = scopep;
            ਜਾਰੀ;
        }
        // Otherwise must be a variable
        AstVar* ਅਚਲ varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->const_Pool_ਇੰਦਰਾਜ(), varp, "Unmarked variable in constant pool");
        m_entries.ਪਾਓ(varp);
    }
    ਜੇ (m_scopep) {
        ਲਈ (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.ਪਾਓ({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::ਲੱਭੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* initp) {
    UASSERT_OBJ(initp->is_Constant_ਰੁੱਖ(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    ਜੇ (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_ਸਤਰ* ਅਚਲ flp = initp->fileline();
    Ast_ਗੰਢ_DType* ਅਚਲ dtypep = initp->dtypep();

    // Look up/create the variable with this value
    ਅਚਲ ਸਵੈਚਾਲਿਤ pair = self.m_entries.ਪਾਓ_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_ਅਗਲਾ_ਸੰਖਿਆ[initp->type()];
        ਅਚਲ std::string name = std::string{initp->typeName()} + "_" + std::ਮੰਜ਼ਿਲ_string(numberr++);
        AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->ਸੈੱਟ_Const_Pool_ਇੰਦਰਾਜ();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(ਸੱਚ);
        varp->valuep(initp->clone_ਰੁੱਖ(ਝੂਠ));
        v3Global.rootp()->constPoolPkgp()->ਜੋੜੋ_Stmtsp(varp);
        ਵਾਪਸ varp;
    });
    AstVar* ਅਚਲ varp = *pair.ਪਹਿਲਾ;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = ਖਾਲੀ_ਸੂਚਕ;
    ਚੋਣ (self.m_stage) {
    ਹਾਲਤ Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* ਅਚਲ pkgp = v3Global.rootp()->constPoolPkgp();
        refp = ਨਵਾਂ AstVarRef{flp, pkgp, varp, VAccess::READ};
        ਰੋਕੋ;
    }
    ਹਾਲਤ Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        ਅਚਲ ਸਵੈਚਾਲਿਤ vscpPair = self.m_varScopes.ਪਾਓ_Lazy(varp, [&]() {
            AstVarScope* ਅਚਲ vscp = ਨਵਾਂ AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->ਜੋੜੋ_Varsp(vscp);
            ਵਾਪਸ std::pair<ਅਚਲ AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = ਨਵਾਂ AstVarRef{flp, vscpPair.ਪਹਿਲਾ->ਦੂਜਾ, VAccess::READ};
        ਰੋਕੋ;
    }
    ਹਾਲਤ Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = ਨਵਾਂ AstVarRef{flp, varp, VAccess::READ};
        ਰੋਕੋ;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    ਵਾਪਸ refp;
}

ਅਚਲ ਅੱਖਰ* V3ConstPool::broken() {
    ਅਚਲ V3ConstPool& self = instance();
    ਜੇ (!self.m_cacheValid) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    ਲਈ (ਅਚਲ AstVar* ਅਚਲ varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.ਪਹਿਲਾ->brokeExists());
        BROKEN_RTN(!pair.ਦੂਜਾ->brokeExists());
    }
    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
}
