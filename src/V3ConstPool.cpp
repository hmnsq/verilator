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

V3Hash V3ConstPool::hash_मान(अचर Ast_गाँठो* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", असत्य, असत्य);
    यदि (अचर AstConst* अचर constp = VN_CAST(nodep, Const)) फर्काऊ hash + constp->अङ्क().toHash();
    यदि (अचर AstConsArray* अचर consp = VN_CAST(nodep, ConsArray)) {
        अचर uint64_t size
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        लागि (uint64_t n = 0; n < size; ++n) hash += hash_मान(consp->valuep(n));
        फर्काऊ hash;
    }
    फर्काऊ hash + V3Hasher::uncachedHash(nodep);
}

तार्किक V3ConstPool::equal_मान(अचर Ast_गाँठो* ap, अचर Ast_गाँठो* bp) {
    यदि (ap->type() != bp->type()) फर्काऊ असत्य;
    यदि (ap->dtypep()->cType("", असत्य, असत्य) != bp->dtypep()->cType("", असत्य, असत्य)) {
        फर्काऊ असत्य;
    }
    // Compare constants by value, which also checks the width
    यदि (अचर AstConst* अचर constp = VN_CAST(ap, Const)) {
        फर्काऊ constp->अङ्क().isCaseEq(VN_AS(bp, Const)->अङ्क());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    यदि (अचर AstConsArray* अचर aConsp = VN_CAST(ap, ConsArray)) {
        अचर AstConsArray* अचर bConsp = VN_AS(bp, ConsArray);
        अचर uint64_t size
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        लागि (uint64_t n = 0; n < size; ++n) {
            यदि (!equal_मान(aConsp->valuep(n), bConsp->valuep(n))) फर्काऊ असत्य;
        }
        फर्काऊ सत्य;
    }
    फर्काऊ ap->same_रूख(bp);
}

शून्य V3ConstPool::rebuildCache() {
    m_cacheValid = सत्य;
    m_scopep = nullptr;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* अचर pkgp = v3Global.rootp()->constPoolPkgp();
    लागि (Ast_गाँठो* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        यदि (AstScope* अचर scopep = VN_CAST(nodep, Scope)) {
            यदि (m_stage == Stage::SCOPED) m_scopep = scopep;
            जारी;
        }
        // Otherwise must be a variable
        AstVar* अचर varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.घुसाउ(varp);
    }
    यदि (m_scopep) {
        लागि (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.घुसाउ({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(Ast_गाँठो_अभिव्यक्ति* initp) {
    UASSERT_OBJ(initp->is_Constant_रूख(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    यदि (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_पङ्क्ति* अचर flp = initp->fileline();
    Ast_गाँठो_DType* अचर dtypep = initp->dtypep();

    // Look up/create the variable with this value
    अचर स्वतः pair = self.m_entries.घुसाउ_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_next_सङ्ख्या[initp->type()];
        अचर std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* अचर varp = नूतन AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(सत्य);
        varp->valuep(initp->clone_रूख(असत्य));
        v3Global.rootp()->constPoolPkgp()->addStmtsp(varp);
        फर्काऊ varp;
    });
    AstVar* अचर varp = *pair.first;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = nullptr;
    विकल्प (self.m_stage) {
    पक्ष Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* अचर pkgp = v3Global.rootp()->constPoolPkgp();
        refp = नूतन AstVarRef{flp, pkgp, varp, VAccess::READ};
        रोक;
    }
    पक्ष Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        अचर स्वतः vscpPair = self.m_varScopes.घुसाउ_Lazy(varp, [&]() {
            AstVarScope* अचर vscp = नूतन AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->addVarsp(vscp);
            फर्काऊ std::pair<अचर AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = नूतन AstVarRef{flp, vscpPair.first->second, VAccess::READ};
        रोक;
    }
    पक्ष Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = नूतन AstVarRef{flp, varp, VAccess::READ};
        रोक;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    फर्काऊ refp;
}

अचर वर्ण* V3ConstPool::broken() {
    अचर V3ConstPool& self = instance();
    यदि (!self.m_cacheValid) फर्काऊ nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    लागि (अचर AstVar* अचर varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    लागि (अचर स्वतः& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    फर्काऊ nullptr;
}
