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

V3Hash V3ConstPool::hash_মান(ধ্রুবক Ast_গ্রন্থি* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", মিথ্যা, মিথ্যা);
    যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep, Const)) ফেরত hash + constp->অঙ্ক().toHash();
    যদি (ধ্রুবক AstConsArray* ধ্রুবক consp = VN_CAST(nodep, ConsArray)) {
        ধ্রুবক uint64_t আকার_2
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        জন্য (uint64_t n = 0; n < আকার_2; ++n) hash += hash_মান(consp->valuep(n));
        ফেরত hash;
    }
    ফেরত hash + V3Hasher::uncachedHash(nodep);
}

যৌক্তিক V3ConstPool::equal_মান(ধ্রুবক Ast_গ্রন্থি* ap, ধ্রুবক Ast_গ্রন্থি* bp) {
    যদি (ap->type() != bp->type()) ফেরত মিথ্যা;
    যদি (ap->dtypep()->cType("", মিথ্যা, মিথ্যা) != bp->dtypep()->cType("", মিথ্যা, মিথ্যা)) {
        ফেরত মিথ্যা;
    }
    // Compare constants by value, which also checks the width
    যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(ap, Const)) {
        ফেরত constp->অঙ্ক().isCaseEq(VN_AS(bp, Const)->অঙ্ক());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    যদি (ধ্রুবক AstConsArray* ধ্রুবক aConsp = VN_CAST(ap, ConsArray)) {
        ধ্রুবক AstConsArray* ধ্রুবক bConsp = VN_AS(bp, ConsArray);
        ধ্রুবক uint64_t আকার_2
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        জন্য (uint64_t n = 0; n < আকার_2; ++n) {
            যদি (!equal_মান(aConsp->valuep(n), bConsp->valuep(n))) ফেরত মিথ্যা;
        }
        ফেরত সত্য;
    }
    ফেরত ap->same_বৃক্ষ(bp);
}

শূন্য V3ConstPool::rebuildCache() {
    m_cacheValid = সত্য;
    m_scopep = nullptr;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* ধ্রুবক pkgp = v3Global.rootp()->constPoolPkgp();
    জন্য (Ast_গ্রন্থি* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        যদি (AstScope* ধ্রুবক scopep = VN_CAST(nodep, Scope)) {
            যদি (m_stage == Stage::SCOPED) m_scopep = scopep;
            চালু;
        }
        // Otherwise must be a variable
        AstVar* ধ্রুবক varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.ঢোকাও(varp);
    }
    যদি (m_scopep) {
        জন্য (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.ঢোকাও({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(Ast_গ্রন্থি_রাশিমালা* initp) {
    UASSERT_OBJ(initp->is_Constant_বৃক্ষ(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    যদি (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_সারি* ধ্রুবক flp = initp->fileline();
    Ast_গ্রন্থি_DType* ধ্রুবক dtypep = initp->dtypep();

    // Look up/create the variable with this value
    ধ্রুবক auto pair = self.m_entries.ঢোকাও_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_পরবর্তী_সংখ্যা[initp->type()];
        ধ্রুবক std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* ধ্রুবক varp = নতুন AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(সত্য);
        varp->valuep(initp->clone_বৃক্ষ(মিথ্যা));
        v3Global.rootp()->constPoolPkgp()->addStmtsp(varp);
        ফেরত varp;
    });
    AstVar* ধ্রুবক varp = *pair.first;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = nullptr;
    নির্বাচন (self.m_stage) {
    ক্ষেত্র Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* ধ্রুবক pkgp = v3Global.rootp()->constPoolPkgp();
        refp = নতুন AstVarRef{flp, pkgp, varp, VAccess::READ};
        বিরতি;
    }
    ক্ষেত্র Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        ধ্রুবক auto vscpPair = self.m_varScopes.ঢোকাও_Lazy(varp, [&]() {
            AstVarScope* ধ্রুবক vscp = নতুন AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->addVarsp(vscp);
            ফেরত std::pair<ধ্রুবক AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = নতুন AstVarRef{flp, vscpPair.first->second, VAccess::READ};
        বিরতি;
    }
    ক্ষেত্র Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = নতুন AstVarRef{flp, varp, VAccess::READ};
        বিরতি;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    ফেরত refp;
}

ধ্রুবক অক্ষর* V3ConstPool::broken() {
    ধ্রুবক V3ConstPool& self = instance();
    যদি (!self.m_cacheValid) ফেরত nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    জন্য (ধ্রুবক AstVar* ধ্রুবক varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    জন্য (ধ্রুবক auto& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    ফেরত nullptr;
}
