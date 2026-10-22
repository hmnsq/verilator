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

#include "V3PchAstMT.h"

#include "V3ConstPool.h"

#include "V3Hasher.h"

//######################################################################
// V3ConstPool

V3Hash V3ConstPool::hashValue(const AstNode* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", false, false);
    གལ་སྲིད (const AstConst* const constp = VN_CAST(nodep, Const)) ལོག hash + constp->num().toHash();
    གལ་སྲིད (const AstConsArray* const consp = VN_CAST(nodep, ConsArray)) {
        const uint64_t size
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        དོན་ལུ (uint64_t n = 0; n < size; ++n) hash += hashValue(consp->valuep(n));
        ལོག hash;
    }
    ལོག hash + V3Hasher::uncachedHash(nodep);
}

bool V3ConstPool::equalValue(const AstNode* ap, const AstNode* bp) {
    གལ་སྲིད (ap->type() != bp->type()) ལོག false;
    གལ་སྲིད (ap->dtypep()->cType("", false, false) != bp->dtypep()->cType("", false, false)) {
        ལོག false;
    }
    // Compare constants by value, which also checks the width
    གལ་སྲིད (const AstConst* const constp = VN_CAST(ap, Const)) {
        ལོག constp->num().isCaseEq(VN_AS(bp, Const)->num());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    གལ་སྲིད (const AstConsArray* const aConsp = VN_CAST(ap, ConsArray)) {
        const AstConsArray* const bConsp = VN_AS(bp, ConsArray);
        const uint64_t size
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        དོན་ལུ (uint64_t n = 0; n < size; ++n) {
            གལ་སྲིད (!equalValue(aConsp->valuep(n), bConsp->valuep(n))) ལོག false;
        }
        ལོག true;
    }
    ལོག ap->sameTree(bp);
}

void V3ConstPool::rebuildCache() {
    m_cacheValid = true;
    m_scopep = nullptr;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* const pkgp = v3Global.rootp()->constPoolPkgp();
    དོན་ལུ (AstNode* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        གལ་སྲིད (AstScope* const scopep = VN_CAST(nodep, Scope)) {
            གལ་སྲིད (m_stage == Stage::SCOPED) m_scopep = scopep;
            continue;
        }
        // Otherwise must be a variable
        AstVar* const varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.insert(varp);
    }
    གལ་སྲིད (m_scopep) {
        དོན་ལུ (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.insert({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(AstNodeExpr* initp) {
    UASSERT_OBJ(initp->isConstantTree(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    གལ་སྲིད (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    FileLine* const flp = initp->fileline();
    AstNodeDType* const dtypep = initp->dtypep();

    // Look up/create the variable with this value
    const auto pair = self.m_entries.insertLazy(initp, [&]() {
        uint32_t& numberr = self.m_nextNumber[initp->type()];
        const std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* const varp = new AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(true);
        varp->valuep(initp->cloneTree(false));
        v3Global.rootp()->constPoolPkgp()->addStmtsp(varp);
        ལོག varp;
    });
    AstVar* const varp = *pair.first;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = nullptr;
    switch (self.m_stage) {
    pak_sa Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* const pkgp = v3Global.rootp()->constPoolPkgp();
        refp = new AstVarRef{flp, pkgp, varp, VAccess::READ};
        bha_nga;
    }
    pak_sa Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        const auto vscpPair = self.m_varScopes.insertLazy(varp, [&]() {
            AstVarScope* const vscp = new AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->addVarsp(vscp);
            ལོག std::pair<const AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = new AstVarRef{flp, vscpPair.first->second, VAccess::READ};
        bha_nga;
    }
    pak_sa Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = new AstVarRef{flp, varp, VAccess::READ};
        bha_nga;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    ལོག refp;
}

const var_na* V3ConstPool::broken() {
    const V3ConstPool& self = instance();
    གལ་སྲིད (!self.m_cacheValid) ལོག nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    དོན་ལུ (const AstVar* const varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    དོན་ལུ (const auto& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    ལོག nullptr;
}
