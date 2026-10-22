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

#समावेश "V3PchAstMT.h"

#समावेश "V3ConstPool.h"

#समावेश "V3Hasher.h"

//######################################################################
// V3ConstPool

V3Hash V3ConstPool::hash_मूल्य(स्थिर Ast_गाँठ* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", false, false);
    अगर (स्थिर AstConst* स्थिर constp = VN_CAST(nodep, Const)) वापस hash + constp->अंक().toHash();
    अगर (स्थिर AstConsArray* स्थिर consp = VN_CAST(nodep, ConsArray)) {
        स्थिर uint64_t आकार
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        क्रम (uint64_t n = 0; n < आकार; ++n) hash += hash_मूल्य(consp->valuep(n));
        वापस hash;
    }
    वापस hash + V3Hasher::uncachedHash(nodep);
}

bool V3ConstPool::equal_मूल्य(स्थिर Ast_गाँठ* ap, स्थिर Ast_गाँठ* bp) {
    अगर (ap->type() != bp->type()) वापस false;
    अगर (ap->dtypep()->cType("", false, false) != bp->dtypep()->cType("", false, false)) {
        वापस false;
    }
    // Compare constants by value, which also checks the width
    अगर (स्थिर AstConst* स्थिर constp = VN_CAST(ap, Const)) {
        वापस constp->अंक().isCaseEq(VN_AS(bp, Const)->अंक());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    अगर (स्थिर AstConsArray* स्थिर aConsp = VN_CAST(ap, ConsArray)) {
        स्थिर AstConsArray* स्थिर bConsp = VN_AS(bp, ConsArray);
        स्थिर uint64_t आकार
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        क्रम (uint64_t n = 0; n < आकार; ++n) {
            अगर (!equal_मूल्य(aConsp->valuep(n), bConsp->valuep(n))) वापस false;
        }
        वापस true;
    }
    वापस ap->same_वृक्ष(bp);
}

व्योम V3ConstPool::rebuildCache() {
    m_cacheValid = true;
    m_scopep = nullptr;
    m_entries.सफा();
    m_varScopes.सफा();
    AstPackage* स्थिर pkgp = v3Global.rootp()->constPoolPkgp();
    क्रम (Ast_गाँठ* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        अगर (AstScope* स्थिर scopep = VN_CAST(nodep, Scope)) {
            अगर (m_stage == Stage::SCOPED) m_scopep = scopep;
            जारी;
        }
        // Otherwise must be a variable
        AstVar* स्थिर varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.जोड़ो(varp);
    }
    अगर (m_scopep) {
        क्रम (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.जोड़ो({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(Ast_गाँठ_व्यंजक* initp) {
    UASSERT_OBJ(initp->is_Constant_वृक्ष(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    अगर (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_पंक्ति* स्थिर flp = initp->fileline();
    Ast_गाँठ_DType* स्थिर dtypep = initp->dtypep();

    // Look up/create the variable with this value
    स्थिर स्वतः pair = self.m_entries.जोड़ो_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_अग्र_संख्या[initp->type()];
        स्थिर std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* स्थिर varp = नया AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(true);
        varp->valuep(initp->clone_वृक्ष(false));
        v3Global.rootp()->constPoolPkgp()->addStmtsp(varp);
        वापस varp;
    });
    AstVar* स्थिर varp = *pair.first;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = nullptr;
    चयन (self.m_stage) {
    हाल Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* स्थिर pkgp = v3Global.rootp()->constPoolPkgp();
        refp = नया AstVarRef{flp, pkgp, varp, VAccess::READ};
        अवरोध;
    }
    हाल Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        स्थिर स्वतः vscpPair = self.m_varScopes.जोड़ो_Lazy(varp, [&]() {
            AstVarScope* स्थिर vscp = नया AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->addVarsp(vscp);
            वापस std::pair<स्थिर AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = नया AstVarRef{flp, vscpPair.first->second, VAccess::READ};
        अवरोध;
    }
    हाल Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = नया AstVarRef{flp, varp, VAccess::READ};
        अवरोध;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    वापस refp;
}

स्थिर अक्षर* V3ConstPool::broken() {
    स्थिर V3ConstPool& self = instance();
    अगर (!self.m_cacheValid) वापस nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    क्रम (स्थिर AstVar* स्थिर varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    क्रम (स्थिर स्वतः& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    वापस nullptr;
}
