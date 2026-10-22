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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3ConstPool.h"

#அந்தர்பாவ "V3Hasher.h"

//######################################################################
// V3ConstPool

V3Hash V3ConstPool::hash_மதிப்பு(மாறிலி Ast_முனை* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", பொய், பொய்);
    எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep, Const)) திரும்பு hash + constp->இலக்கம்().toHash();
    எனில் (மாறிலி AstConsArray* மாறிலி consp = VN_CAST(nodep, ConsArray)) {
        மாறிலி uint64_t size
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        க்கு (uint64_t n = 0; n < size; ++n) hash += hash_மதிப்பு(consp->valuep(n));
        திரும்பு hash;
    }
    திரும்பு hash + V3Hasher::uncachedHash(nodep);
}

தருக்க V3ConstPool::equal_மதிப்பு(மாறிலி Ast_முனை* ap, மாறிலி Ast_முனை* bp) {
    எனில் (ap->type() != bp->type()) திரும்பு பொய்;
    எனில் (ap->dtypep()->cType("", பொய், பொய்) != bp->dtypep()->cType("", பொய், பொய்)) {
        திரும்பு பொய்;
    }
    // Compare constants by value, which also checks the width
    எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(ap, Const)) {
        திரும்பு constp->இலக்கம்().isCaseEq(VN_AS(bp, Const)->இலக்கம்());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    எனில் (மாறிலி AstConsArray* மாறிலி aConsp = VN_CAST(ap, ConsArray)) {
        மாறிலி AstConsArray* மாறிலி bConsp = VN_AS(bp, ConsArray);
        மாறிலி uint64_t size
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        க்கு (uint64_t n = 0; n < size; ++n) {
            எனில் (!equal_மதிப்பு(aConsp->valuep(n), bConsp->valuep(n))) திரும்பு பொய்;
        }
        திரும்பு உண்மை;
    }
    திரும்பு ap->same_மரம்(bp);
}

வெறுமை V3ConstPool::rebuildCache() {
    m_cacheValid = உண்மை;
    m_scopep = nullptr;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* மாறிலி pkgp = v3Global.rootp()->constPoolPkgp();
    க்கு (Ast_முனை* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        எனில் (AstScope* மாறிலி scopep = VN_CAST(nodep, Scope)) {
            எனில் (m_stage == Stage::SCOPED) m_scopep = scopep;
            தொடர்;
        }
        // Otherwise must be a variable
        AstVar* மாறிலி varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.செருகு(varp);
    }
    எனில் (m_scopep) {
        க்கு (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.செருகு({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(Ast_முனை_கோவை* initp) {
    UASSERT_OBJ(initp->is_Constant_மரம்(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    எனில் (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_வரி* மாறிலி flp = initp->fileline();
    Ast_முனை_DType* மாறிலி dtypep = initp->dtypep();

    // Look up/create the variable with this value
    மாறிலி auto pair = self.m_entries.செருகு_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_next_எண்[initp->type()];
        மாறிலி std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* மாறிலி varp = புதிய AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(உண்மை);
        varp->valuep(initp->clone_மரம்(பொய்));
        v3Global.rootp()->constPoolPkgp()->addStmtsp(varp);
        திரும்பு varp;
    });
    AstVar* மாறிலி varp = *pair.first;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = nullptr;
    தேர்வு (self.m_stage) {
    நிலை Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* மாறிலி pkgp = v3Global.rootp()->constPoolPkgp();
        refp = புதிய AstVarRef{flp, pkgp, varp, VAccess::READ};
        நிறுத்து;
    }
    நிலை Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        மாறிலி auto vscpPair = self.m_varScopes.செருகு_Lazy(varp, [&]() {
            AstVarScope* மாறிலி vscp = புதிய AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->addVarsp(vscp);
            திரும்பு std::pair<மாறிலி AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = புதிய AstVarRef{flp, vscpPair.first->second, VAccess::READ};
        நிறுத்து;
    }
    நிலை Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = புதிய AstVarRef{flp, varp, VAccess::READ};
        நிறுத்து;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    திரும்பு refp;
}

மாறிலி எழுத்து* V3ConstPool::broken() {
    மாறிலி V3ConstPool& self = instance();
    எனில் (!self.m_cacheValid) திரும்பு nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    க்கு (மாறிலி AstVar* மாறிலி varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    க்கு (மாறிலி auto& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    திரும்பு nullptr;
}
