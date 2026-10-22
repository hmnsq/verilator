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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"

#ಅನ್ತರ್ಭಾವ "V3Hasher.h"

//######################################################################
// V3ConstPool

V3Hash V3ConstPool::hash_ಮೌಲ್ಯ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", ಸುಳ್ಳು, ಸುಳ್ಳು);
    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const)) ಹಿಂತಿರುಗಿ hash + constp->ಅಂಕೆ().ಗಮ್ಯ_Hash();
    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConsArray* ಸ್ಥಿರಾಂಕ consp = VN_CAST(nodep, ConsArray)) {
        ಸ್ಥಿರಾಂಕ uint64_t ಗಾತ್ರ_2
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        ಗಾಗಿ (uint64_t n = 0; n < ಗಾತ್ರ_2; ++n) hash += hash_ಮೌಲ್ಯ(consp->valuep(n));
        ಹಿಂತಿರುಗಿ hash;
    }
    ಹಿಂತಿರುಗಿ hash + V3Hasher::uncachedHash(nodep);
}

ತಾರ್ಕಿಕ V3ConstPool::equal_ಮೌಲ್ಯ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ap, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* bp) {
    ಆದರೆ (ap->type() != bp->type()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (ap->dtypep()->cType("", ಸುಳ್ಳು, ಸುಳ್ಳು) != bp->dtypep()->cType("", ಸುಳ್ಳು, ಸುಳ್ಳು)) {
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    // Compare constants by value, which also checks the width
    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(ap, Const)) {
        ಹಿಂತಿರುಗಿ constp->ಅಂಕೆ().isCaseEq(VN_AS(bp, Const)->ಅಂಕೆ());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConsArray* ಸ್ಥಿರಾಂಕ aConsp = VN_CAST(ap, ConsArray)) {
        ಸ್ಥಿರಾಂಕ AstConsArray* ಸ್ಥಿರಾಂಕ bConsp = VN_AS(bp, ConsArray);
        ಸ್ಥಿರಾಂಕ uint64_t ಗಾತ್ರ_2
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        ಗಾಗಿ (uint64_t n = 0; n < ಗಾತ್ರ_2; ++n) {
            ಆದರೆ (!equal_ಮೌಲ್ಯ(aConsp->valuep(n), bConsp->valuep(n))) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಹಿಂತಿರುಗಿ ap->same_ವೃಕ್ಷ(bp);
}

ಶೂನ್ಯ V3ConstPool::rebuildCache() {
    m_cacheValid = ನಿಜ;
    m_scopep = ಶೂನ್ಯಸೂಚಕ;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* ಸ್ಥಿರಾಂಕ pkgp = v3Global.rootp()->constPoolPkgp();
    ಗಾಗಿ (Ast_ಗಂಟು* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        ಆದರೆ (AstScope* ಸ್ಥಿರಾಂಕ scopep = VN_CAST(nodep, Scope)) {
            ಆದರೆ (m_stage == Stage::SCOPED) m_scopep = scopep;
            ಮುಂದುವರಿ;
        }
        // Otherwise must be a variable
        AstVar* ಸ್ಥಿರಾಂಕ varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->const_Pool_ನಮೂದು(), varp, "Unmarked variable in constant pool");
        m_entries.ಸೇರಿಸು(varp);
    }
    ಆದರೆ (m_scopep) {
        ಗಾಗಿ (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.ಸೇರಿಸು({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::ಕಂಡುಹಿಡಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* initp) {
    UASSERT_OBJ(initp->is_Constant_ವೃಕ್ಷ(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    ಆದರೆ (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = initp->fileline();
    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = initp->dtypep();

    // Look up/create the variable with this value
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = self.m_entries.ಸೇರಿಸು_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_ಮುಂದಿನ_ಸಂಖ್ಯೆ[initp->type()];
        ಸ್ಥಿರಾಂಕ std::string name = std::string{initp->typeName()} + "_" + std::ಗಮ್ಯ_string(numberr++);
        AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->ಗಣ_Const_Pool_ನಮೂದು();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(ನಿಜ);
        varp->valuep(initp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
        v3Global.rootp()->constPoolPkgp()->ಕೂಡಿಸು_Stmtsp(varp);
        ಹಿಂತಿರುಗಿ varp;
    });
    AstVar* ಸ್ಥಿರಾಂಕ varp = *pair.ಮೊದಲ;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = ಶೂನ್ಯಸೂಚಕ;
    ಆಯ್ಕೆ (self.m_stage) {
    ಸ್ಥಿತಿ Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* ಸ್ಥಿರಾಂಕ pkgp = v3Global.rootp()->constPoolPkgp();
        refp = ಹೊಸ AstVarRef{flp, pkgp, varp, VAccess::READ};
        ನಿಲ್ಲಿಸು;
    }
    ಸ್ಥಿತಿ Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ vscpPair = self.m_varScopes.ಸೇರಿಸು_Lazy(varp, [&]() {
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = ಹೊಸ AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->ಕೂಡಿಸು_Varsp(vscp);
            ಹಿಂತಿರುಗಿ std::pair<ಸ್ಥಿರಾಂಕ AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = ಹೊಸ AstVarRef{flp, vscpPair.ಮೊದಲ->ಎರಡನೇ, VAccess::READ};
        ನಿಲ್ಲಿಸು;
    }
    ಸ್ಥಿತಿ Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = ಹೊಸ AstVarRef{flp, varp, VAccess::READ};
        ನಿಲ್ಲಿಸು;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    ಹಿಂತಿರುಗಿ refp;
}

ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* V3ConstPool::broken() {
    ಸ್ಥಿರಾಂಕ V3ConstPool& self = instance();
    ಆದರೆ (!self.m_cacheValid) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.ಮೊದಲ->brokeExists());
        BROKEN_RTN(!pair.ಎರಡನೇ->brokeExists());
    }
    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
}
