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

V3Hash V3ConstPool::hash_मूल्य(ध्रुव Ast_गाँठ* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", असत्य, असत्य);
    यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) प्रत्यागम hash + constp->अंक().toHash();
    यदि (ध्रुव AstConsArray* ध्रुव consp = VN_CAST(nodep, ConsArray)) {
        ध्रुव uint64_t size
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        प्रति (uint64_t n = 0; n < size; ++n) hash += hash_मूल्य(consp->valuep(n));
        प्रत्यागम hash;
    }
    प्रत्यागम hash + V3Hasher::uncachedHash(nodep);
}

तार्किक V3ConstPool::equal_मूल्य(ध्रुव Ast_गाँठ* ap, ध्रुव Ast_गाँठ* bp) {
    यदि (ap->type() != bp->type()) प्रत्यागम असत्य;
    यदि (ap->dtypep()->cType("", असत्य, असत्य) != bp->dtypep()->cType("", असत्य, असत्य)) {
        प्रत्यागम असत्य;
    }
    // Compare constants by value, which also checks the width
    यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(ap, Const)) {
        प्रत्यागम constp->अंक().isCaseEq(VN_AS(bp, Const)->अंक());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    यदि (ध्रुव AstConsArray* ध्रुव aConsp = VN_CAST(ap, ConsArray)) {
        ध्रुव AstConsArray* ध्रुव bConsp = VN_AS(bp, ConsArray);
        ध्रुव uint64_t size
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        प्रति (uint64_t n = 0; n < size; ++n) {
            यदि (!equal_मूल्य(aConsp->valuep(n), bConsp->valuep(n))) प्रत्यागम असत्य;
        }
        प्रत्यागम सत्य;
    }
    प्रत्यागम ap->same_वृक्ष(bp);
}

शून्य V3ConstPool::rebuildCache() {
    m_cacheValid = सत्य;
    m_scopep = nullptr;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* ध्रुव pkgp = v3Global.rootp()->constPoolPkgp();
    प्रति (Ast_गाँठ* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        यदि (AstScope* ध्रुव scopep = VN_CAST(nodep, Scope)) {
            यदि (m_stage == Stage::SCOPED) m_scopep = scopep;
            अनुवर्त;
        }
        // Otherwise must be a variable
        AstVar* ध्रुव varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.जोड़ो(varp);
    }
    यदि (m_scopep) {
        प्रति (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.जोड़ो({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(Ast_गाँठ_व्यंजक* initp) {
    UASSERT_OBJ(initp->is_Constant_वृक्ष(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    यदि (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_पंक्ति* ध्रुव flp = initp->fileline();
    Ast_गाँठ_DType* ध्रुव dtypep = initp->dtypep();

    // Look up/create the variable with this value
    ध्रुव स्वतः pair = self.m_entries.जोड़ो_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_next_संख्या[initp->type()];
        ध्रुव std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* ध्रुव varp = नूतन AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(सत्य);
        varp->valuep(initp->clone_वृक्ष(असत्य));
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
        ध्रुव स्वतः vscpPair = self.m_varScopes.जोड़ो_Lazy(varp, [&]() {
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
    यदि (!self.m_cacheValid) प्रत्यागम nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    प्रति (ध्रुव AstVar* ध्रुव varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    प्रति (ध्रुव स्वतः& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    प्रत्यागम nullptr;
}
