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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3ConstPool.h"

#అన్తర్భావ "V3Hasher.h"

//######################################################################
// V3ConstPool

V3Hash V3ConstPool::hash_విలువ(స్థిరరాశి Ast_కణుపు* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", అబద్ధం, అబద్ధం);
    ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const)) తిరిగి hash + constp->అంకె().గమ్యం_Hash();
    ఒకవేళ (స్థిరరాశి AstConsArray* స్థిరరాశి consp = VN_CAST(nodep, ConsArray)) {
        స్థిరరాశి uint64_t కొలత
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        కొరకు (uint64_t n = 0; n < కొలత; ++n) hash += hash_విలువ(consp->valuep(n));
        తిరిగి hash;
    }
    తిరిగి hash + V3Hasher::uncachedHash(nodep);
}

తార్కిక V3ConstPool::equal_విలువ(స్థిరరాశి Ast_కణుపు* ap, స్థిరరాశి Ast_కణుపు* bp) {
    ఒకవేళ (ap->type() != bp->type()) తిరిగి అబద్ధం;
    ఒకవేళ (ap->dtypep()->cType("", అబద్ధం, అబద్ధం) != bp->dtypep()->cType("", అబద్ధం, అబద్ధం)) {
        తిరిగి అబద్ధం;
    }
    // Compare constants by value, which also checks the width
    ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(ap, Const)) {
        తిరిగి constp->అంకె().isCaseEq(VN_AS(bp, Const)->అంకె());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    ఒకవేళ (స్థిరరాశి AstConsArray* స్థిరరాశి aConsp = VN_CAST(ap, ConsArray)) {
        స్థిరరాశి AstConsArray* స్థిరరాశి bConsp = VN_AS(bp, ConsArray);
        స్థిరరాశి uint64_t కొలత
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        కొరకు (uint64_t n = 0; n < కొలత; ++n) {
            ఒకవేళ (!equal_విలువ(aConsp->valuep(n), bConsp->valuep(n))) తిరిగి అబద్ధం;
        }
        తిరిగి నిజం;
    }
    తిరిగి ap->same_వృక్షం(bp);
}

శూన్య V3ConstPool::rebuildCache() {
    m_cacheValid = నిజం;
    m_scopep = శూన్యసూచిక;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* స్థిరరాశి pkgp = v3Global.rootp()->constPoolPkgp();
    కొరకు (Ast_కణుపు* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        ఒకవేళ (AstScope* స్థిరరాశి scopep = VN_CAST(nodep, Scope)) {
            ఒకవేళ (m_stage == Stage::SCOPED) m_scopep = scopep;
            కొనసాగు;
        }
        // Otherwise must be a variable
        AstVar* స్థిరరాశి varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->const_Pool_నమోదు(), varp, "Unmarked variable in constant pool");
        m_entries.చొప్పించు(varp);
    }
    ఒకవేళ (m_scopep) {
        కొరకు (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.చొప్పించు({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::కనుగొను(Ast_కణుపు_సమాసం* initp) {
    UASSERT_OBJ(initp->is_Constant_వృక్షం(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    ఒకవేళ (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_పంక్తి* స్థిరరాశి flp = initp->fileline();
    Ast_కణుపు_DType* స్థిరరాశి dtypep = initp->dtypep();

    // Look up/create the variable with this value
    స్థిరరాశి స్వయంచాలక pair = self.m_entries.చొప్పించు_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_తదుపరి_సంఖ్య[initp->type()];
        స్థిరరాశి std::string name = std::string{initp->typeName()} + "_" + std::గమ్యం_string(numberr++);
        AstVar* స్థిరరాశి varp = కొత్త AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->సమితి_Const_Pool_నమోదు();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(నిజం);
        varp->valuep(initp->clone_వృక్షం(అబద్ధం));
        v3Global.rootp()->constPoolPkgp()->కూడు_Stmtsp(varp);
        తిరిగి varp;
    });
    AstVar* స్థిరరాశి varp = *pair.మొదటి;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = శూన్యసూచిక;
    ఎంపిక (self.m_stage) {
    స్థితి Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* స్థిరరాశి pkgp = v3Global.rootp()->constPoolPkgp();
        refp = కొత్త AstVarRef{flp, pkgp, varp, VAccess::READ};
        ఆపు;
    }
    స్థితి Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        స్థిరరాశి స్వయంచాలక vscpPair = self.m_varScopes.చొప్పించు_Lazy(varp, [&]() {
            AstVarScope* స్థిరరాశి vscp = కొత్త AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->కూడు_Varsp(vscp);
            తిరిగి std::pair<స్థిరరాశి AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = కొత్త AstVarRef{flp, vscpPair.మొదటి->రెండవ, VAccess::READ};
        ఆపు;
    }
    స్థితి Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = కొత్త AstVarRef{flp, varp, VAccess::READ};
        ఆపు;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    తిరిగి refp;
}

స్థిరరాశి అక్షరం* V3ConstPool::broken() {
    స్థిరరాశి V3ConstPool& self = instance();
    ఒకవేళ (!self.m_cacheValid) తిరిగి శూన్యసూచిక;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    కొరకు (స్థిరరాశి AstVar* స్థిరరాశి varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    కొరకు (స్థిరరాశి స్వయంచాలక& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.మొదటి->brokeExists());
        BROKEN_RTN(!pair.రెండవ->brokeExists());
    }
    తిరిగి శూన్యసూచిక;
}
