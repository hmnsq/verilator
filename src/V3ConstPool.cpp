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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3ConstPool.h"

#અન્તર્ભાવ "V3Hasher.h"

//######################################################################
// V3ConstPool

V3Hash V3ConstPool::hash_કિંમત(અચલ Ast_ગાંઠ* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", ખોટું, ખોટું);
    જો (અચલ AstConst* અચલ constp = VN_CAST(nodep, Const)) પાછા hash + constp->અંક().toHash();
    જો (અચલ AstConsArray* અચલ consp = VN_CAST(nodep, ConsArray)) {
        અચલ uint64_t size
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        માટે (uint64_t n = 0; n < size; ++n) hash += hash_કિંમત(consp->valuep(n));
        પાછા hash;
    }
    પાછા hash + V3Hasher::uncachedHash(nodep);
}

તાર્કિક V3ConstPool::equal_કિંમત(અચલ Ast_ગાંઠ* ap, અચલ Ast_ગાંઠ* bp) {
    જો (ap->type() != bp->type()) પાછા ખોટું;
    જો (ap->dtypep()->cType("", ખોટું, ખોટું) != bp->dtypep()->cType("", ખોટું, ખોટું)) {
        પાછા ખોટું;
    }
    // Compare constants by value, which also checks the width
    જો (અચલ AstConst* અચલ constp = VN_CAST(ap, Const)) {
        પાછા constp->અંક().isCaseEq(VN_AS(bp, Const)->અંક());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    જો (અચલ AstConsArray* અચલ aConsp = VN_CAST(ap, ConsArray)) {
        અચલ AstConsArray* અચલ bConsp = VN_AS(bp, ConsArray);
        અચલ uint64_t size
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        માટે (uint64_t n = 0; n < size; ++n) {
            જો (!equal_કિંમત(aConsp->valuep(n), bConsp->valuep(n))) પાછા ખોટું;
        }
        પાછા સાચું;
    }
    પાછા ap->same_વૃક્ષ(bp);
}

શૂન્ય V3ConstPool::rebuildCache() {
    m_cacheValid = સાચું;
    m_scopep = nullptr;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* અચલ pkgp = v3Global.rootp()->constPoolPkgp();
    માટે (Ast_ગાંઠ* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        જો (AstScope* અચલ scopep = VN_CAST(nodep, Scope)) {
            જો (m_stage == Stage::SCOPED) m_scopep = scopep;
            ચાલુ;
        }
        // Otherwise must be a variable
        AstVar* અચલ varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.ઉમેરો(varp);
    }
    જો (m_scopep) {
        માટે (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.ઉમેરો({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(Ast_ગાંઠ_પદાવલિ* initp) {
    UASSERT_OBJ(initp->is_Constant_વૃક્ષ(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    જો (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_લીટી* અચલ flp = initp->fileline();
    Ast_ગાંઠ_DType* અચલ dtypep = initp->dtypep();

    // Look up/create the variable with this value
    અચલ auto pair = self.m_entries.ઉમેરો_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_next_સંખ્યા[initp->type()];
        અચલ std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* અચલ varp = નવું AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(સાચું);
        varp->valuep(initp->clone_વૃક્ષ(ખોટું));
        v3Global.rootp()->constPoolPkgp()->addStmtsp(varp);
        પાછા varp;
    });
    AstVar* અચલ varp = *pair.first;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = nullptr;
    પસંદગી (self.m_stage) {
    સ્થિતિ Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* અચલ pkgp = v3Global.rootp()->constPoolPkgp();
        refp = નવું AstVarRef{flp, pkgp, varp, VAccess::READ};
        તોડો;
    }
    સ્થિતિ Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        અચલ auto vscpPair = self.m_varScopes.ઉમેરો_Lazy(varp, [&]() {
            AstVarScope* અચલ vscp = નવું AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->addVarsp(vscp);
            પાછા std::pair<અચલ AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = નવું AstVarRef{flp, vscpPair.first->second, VAccess::READ};
        તોડો;
    }
    સ્થિતિ Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = નવું AstVarRef{flp, varp, VAccess::READ};
        તોડો;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    પાછા refp;
}

અચલ અક્ષર* V3ConstPool::broken() {
    અચલ V3ConstPool& self = instance();
    જો (!self.m_cacheValid) પાછા nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    માટે (અચલ AstVar* અચલ varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    માટે (અચલ auto& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    પાછા nullptr;
}
