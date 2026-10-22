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

#অন্তর্ভাব "V3PchAstMT.h"

#অন্তর্ভাব "V3ConstPool.h"

#অন্তর্ভাব "V3Hasher.h"

//######################################################################
// V3ConstPool

V3Hash V3ConstPool::hash_মান(ধ্রুব Ast_গ্রন্থি* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", অসত্য, অসত্য);
    যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(nodep, Const)) প্রত্যাগম hash + constp->অঙ্ক().toHash();
    যদি (ধ্রুব AstConsArray* ধ্রুব consp = VN_CAST(nodep, ConsArray)) {
        ধ্রুব uint64_t size
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        প্রতি (uint64_t n = 0; n < size; ++n) hash += hash_মান(consp->valuep(n));
        প্রত্যাগম hash;
    }
    প্রত্যাগম hash + V3Hasher::uncachedHash(nodep);
}

তার্কিক V3ConstPool::equal_মান(ধ্রুব Ast_গ্রন্থি* ap, ধ্রুব Ast_গ্রন্থি* bp) {
    যদি (ap->type() != bp->type()) প্রত্যাগম অসত্য;
    যদি (ap->dtypep()->cType("", অসত্য, অসত্য) != bp->dtypep()->cType("", অসত্য, অসত্য)) {
        প্রত্যাগম অসত্য;
    }
    // Compare constants by value, which also checks the width
    যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(ap, Const)) {
        প্রত্যাগম constp->অঙ্ক().isCaseEq(VN_AS(bp, Const)->অঙ্ক());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    যদি (ধ্রুব AstConsArray* ধ্রুব aConsp = VN_CAST(ap, ConsArray)) {
        ধ্রুব AstConsArray* ধ্রুব bConsp = VN_AS(bp, ConsArray);
        ধ্রুব uint64_t size
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        প্রতি (uint64_t n = 0; n < size; ++n) {
            যদি (!equal_মান(aConsp->valuep(n), bConsp->valuep(n))) প্রত্যাগম অসত্য;
        }
        প্রত্যাগম সত্য;
    }
    প্রত্যাগম ap->same_বৃক্ষ(bp);
}

শূন্য V3ConstPool::rebuildCache() {
    m_cacheValid = সত্য;
    m_scopep = nullptr;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* ধ্রুব pkgp = v3Global.rootp()->constPoolPkgp();
    প্রতি (Ast_গ্রন্থি* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        যদি (AstScope* ধ্রুব scopep = VN_CAST(nodep, Scope)) {
            যদি (m_stage == Stage::SCOPED) m_scopep = scopep;
            অনুবর্ত;
        }
        // Otherwise must be a variable
        AstVar* ধ্রুব varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.ঢোকাও(varp);
    }
    যদি (m_scopep) {
        প্রতি (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.ঢোকাও({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(Ast_গ্রন্থি_রাশিমালা* initp) {
    UASSERT_OBJ(initp->is_Constant_বৃক্ষ(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    যদি (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_সারি* ধ্রুব flp = initp->fileline();
    Ast_গ্রন্থি_DType* ধ্রুব dtypep = initp->dtypep();

    // Look up/create the variable with this value
    ধ্রুব auto pair = self.m_entries.ঢোকাও_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_next_সংখ্যা[initp->type()];
        ধ্রুব std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* ধ্রুব varp = নূতন AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(সত্য);
        varp->valuep(initp->clone_বৃক্ষ(অসত্য));
        v3Global.rootp()->constPoolPkgp()->addStmtsp(varp);
        প্রত্যাগম varp;
    });
    AstVar* ধ্রুব varp = *pair.first;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = nullptr;
    বিকল্প (self.m_stage) {
    পক্ষ Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* ধ্রুব pkgp = v3Global.rootp()->constPoolPkgp();
        refp = নূতন AstVarRef{flp, pkgp, varp, VAccess::READ};
        ভঙ্গ;
    }
    পক্ষ Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        ধ্রুব auto vscpPair = self.m_varScopes.ঢোকাও_Lazy(varp, [&]() {
            AstVarScope* ধ্রুব vscp = নূতন AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->addVarsp(vscp);
            প্রত্যাগম std::pair<ধ্রুব AstVar*, AstVarScope*>{varp, vscp};
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
    প্রত্যাগম refp;
}

ধ্রুব বর্ণ* V3ConstPool::broken() {
    ধ্রুব V3ConstPool& self = instance();
    যদি (!self.m_cacheValid) প্রত্যাগম nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    প্রতি (ধ্রুব AstVar* ধ্রুব varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    প্রতি (ধ্রুব auto& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    প্রত্যাগম nullptr;
}
