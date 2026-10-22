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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3ConstPool.h"

#अन्तर्भाव "V3Hasher.h"

//######################################################################
// V3ConstPool

V3Hash V3ConstPool::hash_मानम्(ध्रुव Ast_ग्रन्थिः* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", मिच्छ, मिच्छ);
    जइ (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) प्रत्यागम hash + constp->अङ्कः().toHash();
    जइ (ध्रुव AstConsArray* ध्रुव consp = VN_CAST(nodep, ConsArray)) {
        ध्रुव uint64_t size
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        प्रति (uint64_t n = 0; n < size; ++n) hash += hash_मानम्(consp->valuep(n));
        प्रत्यागम hash;
    }
    प्रत्यागम hash + V3Hasher::uncachedHash(nodep);
}

तार्किक V3ConstPool::equal_मानम्(ध्रुव Ast_ग्रन्थिः* ap, ध्रुव Ast_ग्रन्थिः* bp) {
    जइ (ap->type() != bp->type()) प्रत्यागम मिच्छ;
    जइ (ap->dtypep()->cType("", मिच्छ, मिच्छ) != bp->dtypep()->cType("", मिच्छ, मिच्छ)) {
        प्रत्यागम मिच्छ;
    }
    // Compare constants by value, which also checks the width
    जइ (ध्रुव AstConst* ध्रुव constp = VN_CAST(ap, Const)) {
        प्रत्यागम constp->अङ्कः().isCaseEq(VN_AS(bp, Const)->अङ्कः());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    जइ (ध्रुव AstConsArray* ध्रुव aConsp = VN_CAST(ap, ConsArray)) {
        ध्रुव AstConsArray* ध्रुव bConsp = VN_AS(bp, ConsArray);
        ध्रुव uint64_t size
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        प्रति (uint64_t n = 0; n < size; ++n) {
            जइ (!equal_मानम्(aConsp->valuep(n), bConsp->valuep(n))) प्रत्यागम मिच्छ;
        }
        प्रत्यागम सच्च;
    }
    प्रत्यागम ap->same_वृक्षः(bp);
}

शून्य V3ConstPool::rebuildCache() {
    m_cacheValid = सच्च;
    m_scopep = nullptr;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* ध्रुव pkgp = v3Global.rootp()->constPoolPkgp();
    प्रति (Ast_ग्रन्थिः* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        जइ (AstScope* ध्रुव scopep = VN_CAST(nodep, Scope)) {
            जइ (m_stage == Stage::SCOPED) m_scopep = scopep;
            अनुवर्त;
        }
        // Otherwise must be a variable
        AstVar* ध्रुव varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.निवेशय(varp);
    }
    जइ (m_scopep) {
        प्रति (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.निवेशय({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(Ast_ग्रन्थिः_व्यञ्जकम्* initp) {
    UASSERT_OBJ(initp->is_Constant_वृक्षः(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    जइ (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_पङ्क्तिः* ध्रुव flp = initp->fileline();
    Ast_ग्रन्थिः_DType* ध्रुव dtypep = initp->dtypep();

    // Look up/create the variable with this value
    ध्रुव auto pair = self.m_entries.निवेशय_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_next_सङ्ख्या[initp->type()];
        ध्रुव std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* ध्रुव varp = नूतन AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(सच्च);
        varp->valuep(initp->clone_वृक्षः(मिच्छ));
        v3Global.rootp()->constPoolPkgp()->addStmtsp(varp);
        प्रत्यागम varp;
    });
    AstVar* ध्रुव varp = *pair.first;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = nullptr;
    विकल्प (self.m_stage) {
    पक्ष Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* ध्रुव pkgp = v3Global.rootp()->constPoolPkgp();
        refp = नूतन AstVarRef{flp, pkgp, varp, VAccess::READ};
        भङ्ग;
    }
    पक्ष Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        ध्रुव auto vscpPair = self.m_varScopes.निवेशय_Lazy(varp, [&]() {
            AstVarScope* ध्रुव vscp = नूतन AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->addVarsp(vscp);
            प्रत्यागम std::pair<ध्रुव AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = नूतन AstVarRef{flp, vscpPair.first->second, VAccess::READ};
        भङ्ग;
    }
    पक्ष Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = नूतन AstVarRef{flp, varp, VAccess::READ};
        भङ्ग;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    प्रत्यागम refp;
}

ध्रुव वर्ण* V3ConstPool::broken() {
    ध्रुव V3ConstPool& self = instance();
    जइ (!self.m_cacheValid) प्रत्यागम nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    प्रति (ध्रुव AstVar* ध्रुव varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    प्रति (ध्रुव auto& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    प्रत्यागम nullptr;
}
