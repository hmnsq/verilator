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

V3Hash V3ConstPool::hash_ಮೌಲ್ಯ(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", ಅಸತ್ಯ, ಅಸತ್ಯ);
    ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const)) ಪ್ರತ್ಯಾಗಮ hash + constp->ಅಂಕೆ().toHash();
    ಯದಿ (ಧ್ರುವ AstConsArray* ಧ್ರುವ consp = VN_CAST(nodep, ConsArray)) {
        ಧ್ರುವ uint64_t size
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        ಪ್ರತಿ (uint64_t n = 0; n < size; ++n) hash += hash_ಮೌಲ್ಯ(consp->valuep(n));
        ಪ್ರತ್ಯಾಗಮ hash;
    }
    ಪ್ರತ್ಯಾಗಮ hash + V3Hasher::uncachedHash(nodep);
}

ತಾರ್ಕಿಕ V3ConstPool::equal_ಮೌಲ್ಯ(ಧ್ರುವ Ast_ಗಂಟು* ap, ಧ್ರುವ Ast_ಗಂಟು* bp) {
    ಯದಿ (ap->type() != bp->type()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (ap->dtypep()->cType("", ಅಸತ್ಯ, ಅಸತ್ಯ) != bp->dtypep()->cType("", ಅಸತ್ಯ, ಅಸತ್ಯ)) {
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    // Compare constants by value, which also checks the width
    ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(ap, Const)) {
        ಪ್ರತ್ಯಾಗಮ constp->ಅಂಕೆ().isCaseEq(VN_AS(bp, Const)->ಅಂಕೆ());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    ಯದಿ (ಧ್ರುವ AstConsArray* ಧ್ರುವ aConsp = VN_CAST(ap, ConsArray)) {
        ಧ್ರುವ AstConsArray* ಧ್ರುವ bConsp = VN_AS(bp, ConsArray);
        ಧ್ರುವ uint64_t size
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        ಪ್ರತಿ (uint64_t n = 0; n < size; ++n) {
            ಯದಿ (!equal_ಮೌಲ್ಯ(aConsp->valuep(n), bConsp->valuep(n))) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ap->same_ವೃಕ್ಷ(bp);
}

ಶೂನ್ಯ V3ConstPool::rebuildCache() {
    m_cacheValid = ಸತ್ಯ;
    m_scopep = nullptr;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* ಧ್ರುವ pkgp = v3Global.rootp()->constPoolPkgp();
    ಪ್ರತಿ (Ast_ಗಂಟು* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        ಯದಿ (AstScope* ಧ್ರುವ scopep = VN_CAST(nodep, Scope)) {
            ಯದಿ (m_stage == Stage::SCOPED) m_scopep = scopep;
            ಅನುವರ್ತ;
        }
        // Otherwise must be a variable
        AstVar* ಧ್ರುವ varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.ಸೇರಿಸು(varp);
    }
    ಯದಿ (m_scopep) {
        ಪ್ರತಿ (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.ಸೇರಿಸು({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* initp) {
    UASSERT_OBJ(initp->is_Constant_ವೃಕ್ಷ(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    ಯದಿ (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_ಸಾಲು* ಧ್ರುವ flp = initp->fileline();
    Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = initp->dtypep();

    // Look up/create the variable with this value
    ಧ್ರುವ auto pair = self.m_entries.ಸೇರಿಸು_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_next_ಸಂಖ್ಯೆ[initp->type()];
        ಧ್ರುವ std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(ಸತ್ಯ);
        varp->valuep(initp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
        v3Global.rootp()->constPoolPkgp()->addStmtsp(varp);
        ಪ್ರತ್ಯಾಗಮ varp;
    });
    AstVar* ಧ್ರುವ varp = *pair.first;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = nullptr;
    ವಿಕಲ್ಪ (self.m_stage) {
    ಪಕ್ಷ Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* ಧ್ರುವ pkgp = v3Global.rootp()->constPoolPkgp();
        refp = ನೂತನ AstVarRef{flp, pkgp, varp, VAccess::READ};
        ಭಙ್ಗ;
    }
    ಪಕ್ಷ Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        ಧ್ರುವ auto vscpPair = self.m_varScopes.ಸೇರಿಸು_Lazy(varp, [&]() {
            AstVarScope* ಧ್ರುವ vscp = ನೂತನ AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->addVarsp(vscp);
            ಪ್ರತ್ಯಾಗಮ std::pair<ಧ್ರುವ AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = ನೂತನ AstVarRef{flp, vscpPair.first->second, VAccess::READ};
        ಭಙ್ಗ;
    }
    ಪಕ್ಷ Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = ನೂತನ AstVarRef{flp, varp, VAccess::READ};
        ಭಙ್ಗ;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    ಪ್ರತ್ಯಾಗಮ refp;
}

ಧ್ರುವ ವರ್ಣ* V3ConstPool::broken() {
    ಧ್ರುವ V3ConstPool& self = instance();
    ಯದಿ (!self.m_cacheValid) ಪ್ರತ್ಯಾಗಮ nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    ಪ್ರತಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    ಪ್ರತಿ (ಧ್ರುವ auto& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    ಪ್ರತ್ಯಾಗಮ nullptr;
}
