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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3ConstPool.h"

#ଅନ୍ତର୍ଭାଵ "V3Hasher.h"

//######################################################################
// V3ConstPool

V3Hash V3ConstPool::hash_ମୂଲ୍ୟ(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", ମିଥ୍ୟା, ମିଥ୍ୟା);
    ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep, Const)) ଫେରନ୍ତୁ hash + constp->ଅଙ୍କ().toHash();
    ଯଦି (ଅଚଳ AstConsArray* ଅଚଳ consp = VN_CAST(nodep, ConsArray)) {
        ଅଚଳ uint64_t size
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        ପାଇଁ (uint64_t n = 0; n < size; ++n) hash += hash_ମୂଲ୍ୟ(consp->valuep(n));
        ଫେରନ୍ତୁ hash;
    }
    ଫେରନ୍ତୁ hash + V3Hasher::uncachedHash(nodep);
}

ତାର୍କିକ V3ConstPool::equal_ମୂଲ୍ୟ(ଅଚଳ Ast_ଗଣ୍ଠି* ap, ଅଚଳ Ast_ଗଣ୍ଠି* bp) {
    ଯଦି (ap->type() != bp->type()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (ap->dtypep()->cType("", ମିଥ୍ୟା, ମିଥ୍ୟା) != bp->dtypep()->cType("", ମିଥ୍ୟା, ମିଥ୍ୟା)) {
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    // Compare constants by value, which also checks the width
    ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(ap, Const)) {
        ଫେରନ୍ତୁ constp->ଅଙ୍କ().isCaseEq(VN_AS(bp, Const)->ଅଙ୍କ());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    ଯଦି (ଅଚଳ AstConsArray* ଅଚଳ aConsp = VN_CAST(ap, ConsArray)) {
        ଅଚଳ AstConsArray* ଅଚଳ bConsp = VN_AS(bp, ConsArray);
        ଅଚଳ uint64_t size
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        ପାଇଁ (uint64_t n = 0; n < size; ++n) {
            ଯଦି (!equal_ମୂଲ୍ୟ(aConsp->valuep(n), bConsp->valuep(n))) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଫେରନ୍ତୁ ap->same_ବୃକ୍ଷ(bp);
}

ଶୂନ୍ୟ V3ConstPool::rebuildCache() {
    m_cacheValid = ସତ୍ୟ;
    m_scopep = nullptr;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* ଅଚଳ pkgp = v3Global.rootp()->constPoolPkgp();
    ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        ଯଦି (AstScope* ଅଚଳ scopep = VN_CAST(nodep, Scope)) {
            ଯଦି (m_stage == Stage::SCOPED) m_scopep = scopep;
            ଜାରି;
        }
        // Otherwise must be a variable
        AstVar* ଅଚଳ varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.ଭର୍ତ୍ତି(varp);
    }
    ଯଦି (m_scopep) {
        ପାଇଁ (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.ଭର୍ତ୍ତି({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* initp) {
    UASSERT_OBJ(initp->is_Constant_ବୃକ୍ଷ(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    ଯଦି (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_ଧାଡ଼ି* ଅଚଳ flp = initp->fileline();
    Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = initp->dtypep();

    // Look up/create the variable with this value
    ଅଚଳ auto pair = self.m_entries.ଭର୍ତ୍ତି_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_next_ସଂଖ୍ୟା[initp->type()];
        ଅଚଳ std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* ଅଚଳ varp = ନୂତନ AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(ସତ୍ୟ);
        varp->valuep(initp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
        v3Global.rootp()->constPoolPkgp()->addStmtsp(varp);
        ଫେରନ୍ତୁ varp;
    });
    AstVar* ଅଚଳ varp = *pair.first;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = nullptr;
    ବାଛ (self.m_stage) {
    ସ୍ଥିତି Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* ଅଚଳ pkgp = v3Global.rootp()->constPoolPkgp();
        refp = ନୂତନ AstVarRef{flp, pkgp, varp, VAccess::READ};
        ଭାଙ୍ଗ;
    }
    ସ୍ଥିତି Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        ଅଚଳ auto vscpPair = self.m_varScopes.ଭର୍ତ୍ତି_Lazy(varp, [&]() {
            AstVarScope* ଅଚଳ vscp = ନୂତନ AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->addVarsp(vscp);
            ଫେରନ୍ତୁ std::pair<ଅଚଳ AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = ନୂତନ AstVarRef{flp, vscpPair.first->second, VAccess::READ};
        ଭାଙ୍ଗ;
    }
    ସ୍ଥିତି Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = ନୂତନ AstVarRef{flp, varp, VAccess::READ};
        ଭାଙ୍ଗ;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    ଫେରନ୍ତୁ refp;
}

ଅଚଳ ଅକ୍ଷର* V3ConstPool::broken() {
    ଅଚଳ V3ConstPool& self = instance();
    ଯଦି (!self.m_cacheValid) ଫେରନ୍ତୁ nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    ପାଇଁ (ଅଚଳ AstVar* ଅଚଳ varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    ପାଇଁ (ଅଚଳ auto& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    ଫେରନ୍ତୁ nullptr;
}
