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

#অন্তর্ভাৱ "V3PchAstMT.h"

#অন্তর্ভাৱ "V3ConstPool.h"

#অন্তর্ভাৱ "V3Hasher.h"

//######################################################################
// V3ConstPool

V3Hash V3ConstPool::hash_মান(ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", মিছা, মিছা);
    যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep, Const)) ঘূৰাই hash + constp->অঙ্ক().toHash();
    যদি (ধ্রুৱ AstConsArray* ধ্রুৱ consp = VN_CAST(nodep, ConsArray)) {
        ধ্রুৱ uint64_t size
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        বাবে (uint64_t n = 0; n < size; ++n) hash += hash_মান(consp->valuep(n));
        ঘূৰাই hash;
    }
    ঘূৰাই hash + V3Hasher::uncachedHash(nodep);
}

তার্কিক V3ConstPool::equal_মান(ধ্রুৱ Ast_গ্ৰন্থি* ap, ধ্রুৱ Ast_গ্ৰন্থি* bp) {
    যদি (ap->type() != bp->type()) ঘূৰাই মিছা;
    যদি (ap->dtypep()->cType("", মিছা, মিছা) != bp->dtypep()->cType("", মিছা, মিছা)) {
        ঘূৰাই মিছা;
    }
    // Compare constants by value, which also checks the width
    যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(ap, Const)) {
        ঘূৰাই constp->অঙ্ক().isCaseEq(VN_AS(bp, Const)->অঙ্ক());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    যদি (ধ্রুৱ AstConsArray* ধ্রুৱ aConsp = VN_CAST(ap, ConsArray)) {
        ধ্রুৱ AstConsArray* ধ্রুৱ bConsp = VN_AS(bp, ConsArray);
        ধ্রুৱ uint64_t size
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        বাবে (uint64_t n = 0; n < size; ++n) {
            যদি (!equal_মান(aConsp->valuep(n), bConsp->valuep(n))) ঘূৰাই মিছা;
        }
        ঘূৰাই সঁচা;
    }
    ঘূৰাই ap->same_বৃক্ষ(bp);
}

শূন্য V3ConstPool::rebuildCache() {
    m_cacheValid = সঁচা;
    m_scopep = nullptr;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* ধ্রুৱ pkgp = v3Global.rootp()->constPoolPkgp();
    বাবে (Ast_গ্ৰন্থি* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        যদি (AstScope* ধ্রুৱ scopep = VN_CAST(nodep, Scope)) {
            যদি (m_stage == Stage::SCOPED) m_scopep = scopep;
            অনুৱর্ত;
        }
        // Otherwise must be a variable
        AstVar* ধ্রুৱ varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.ঢোকাও(varp);
    }
    যদি (m_scopep) {
        বাবে (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.ঢোকাও({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(Ast_গ্ৰন্থি_ৰাশিমালা* initp) {
    UASSERT_OBJ(initp->is_Constant_বৃক্ষ(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    যদি (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_সাৰি* ধ্রুৱ flp = initp->fileline();
    Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = initp->dtypep();

    // Look up/create the variable with this value
    ধ্রুৱ auto pair = self.m_entries.ঢোকাও_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_next_সংখ্যা[initp->type()];
        ধ্রুৱ std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* ধ্রুৱ varp = নূতন AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(সঁচা);
        varp->valuep(initp->clone_বৃক্ষ(মিছা));
        v3Global.rootp()->constPoolPkgp()->addStmtsp(varp);
        ঘূৰাই varp;
    });
    AstVar* ধ্রুৱ varp = *pair.first;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = nullptr;
    ৱিকল্প (self.m_stage) {
    পক্ষ Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* ধ্রুৱ pkgp = v3Global.rootp()->constPoolPkgp();
        refp = নূতন AstVarRef{flp, pkgp, varp, VAccess::READ};
        ভঙ্গ;
    }
    পক্ষ Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        ধ্রুৱ auto vscpPair = self.m_varScopes.ঢোকাও_Lazy(varp, [&]() {
            AstVarScope* ধ্রুৱ vscp = নূতন AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->addVarsp(vscp);
            ঘূৰাই std::pair<ধ্রুৱ AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = নূতন AstVarRef{flp, vscpPair.first->second, VAccess::READ};
        ভঙ্গ;
    }
    পক্ষ Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = নূতন AstVarRef{flp, varp, VAccess::READ};
        ভঙ্গ;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    ঘূৰাই refp;
}

ধ্রুৱ ৱর্ণ* V3ConstPool::broken() {
    ধ্রুৱ V3ConstPool& self = instance();
    যদি (!self.m_cacheValid) ঘূৰাই nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    বাবে (ধ্রুৱ AstVar* ধ্রুৱ varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    বাবে (ধ্রুৱ auto& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    ঘূৰাই nullptr;
}
