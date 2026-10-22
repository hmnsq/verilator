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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3ConstPool.h"

#അന്തര്ഭാവ "V3Hasher.h"

//######################################################################
// V3ConstPool

V3Hash V3ConstPool::hash_മൂല്യം(സ്ഥിരം Ast_കണ്ണി* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", അസത്യം, അസത്യം);
    എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const)) തിരികെ hash + constp->അക്കം().toHash();
    എങ്കിൽ (സ്ഥിരം AstConsArray* സ്ഥിരം consp = VN_CAST(nodep, ConsArray)) {
        സ്ഥിരം uint64_t size
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        വേണ്ടി (uint64_t n = 0; n < size; ++n) hash += hash_മൂല്യം(consp->valuep(n));
        തിരികെ hash;
    }
    തിരികെ hash + V3Hasher::uncachedHash(nodep);
}

തർക്കം V3ConstPool::equal_മൂല്യം(സ്ഥിരം Ast_കണ്ണി* ap, സ്ഥിരം Ast_കണ്ണി* bp) {
    എങ്കിൽ (ap->type() != bp->type()) തിരികെ അസത്യം;
    എങ്കിൽ (ap->dtypep()->cType("", അസത്യം, അസത്യം) != bp->dtypep()->cType("", അസത്യം, അസത്യം)) {
        തിരികെ അസത്യം;
    }
    // Compare constants by value, which also checks the width
    എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(ap, Const)) {
        തിരികെ constp->അക്കം().isCaseEq(VN_AS(bp, Const)->അക്കം());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    എങ്കിൽ (സ്ഥിരം AstConsArray* സ്ഥിരം aConsp = VN_CAST(ap, ConsArray)) {
        സ്ഥിരം AstConsArray* സ്ഥിരം bConsp = VN_AS(bp, ConsArray);
        സ്ഥിരം uint64_t size
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        വേണ്ടി (uint64_t n = 0; n < size; ++n) {
            എങ്കിൽ (!equal_മൂല്യം(aConsp->valuep(n), bConsp->valuep(n))) തിരികെ അസത്യം;
        }
        തിരികെ സത്യം;
    }
    തിരികെ ap->same_വൃക്ഷം(bp);
}

ശൂന്യം V3ConstPool::rebuildCache() {
    m_cacheValid = സത്യം;
    m_scopep = nullptr;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* സ്ഥിരം pkgp = v3Global.rootp()->constPoolPkgp();
    വേണ്ടി (Ast_കണ്ണി* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        എങ്കിൽ (AstScope* സ്ഥിരം scopep = VN_CAST(nodep, Scope)) {
            എങ്കിൽ (m_stage == Stage::SCOPED) m_scopep = scopep;
            തുടരുക;
        }
        // Otherwise must be a variable
        AstVar* സ്ഥിരം varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.തിരുകുക(varp);
    }
    എങ്കിൽ (m_scopep) {
        വേണ്ടി (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.തിരുകുക({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(Ast_കണ്ണി_വ്യഞ്ജകം* initp) {
    UASSERT_OBJ(initp->is_Constant_വൃക്ഷം(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    എങ്കിൽ (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_വരി* സ്ഥിരം flp = initp->fileline();
    Ast_കണ്ണി_DType* സ്ഥിരം dtypep = initp->dtypep();

    // Look up/create the variable with this value
    സ്ഥിരം auto pair = self.m_entries.തിരുകുക_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_next_സംഖ്യ[initp->type()];
        സ്ഥിരം std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* സ്ഥിരം varp = പുതിയ AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(സത്യം);
        varp->valuep(initp->clone_വൃക്ഷം(അസത്യം));
        v3Global.rootp()->constPoolPkgp()->addStmtsp(varp);
        തിരികെ varp;
    });
    AstVar* സ്ഥിരം varp = *pair.first;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = nullptr;
    തിരഞ്ഞെടുപ്പ് (self.m_stage) {
    അവസ്ഥ Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* സ്ഥിരം pkgp = v3Global.rootp()->constPoolPkgp();
        refp = പുതിയ AstVarRef{flp, pkgp, varp, VAccess::READ};
        നിർത്തുക;
    }
    അവസ്ഥ Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        സ്ഥിരം auto vscpPair = self.m_varScopes.തിരുകുക_Lazy(varp, [&]() {
            AstVarScope* സ്ഥിരം vscp = പുതിയ AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->addVarsp(vscp);
            തിരികെ std::pair<സ്ഥിരം AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = പുതിയ AstVarRef{flp, vscpPair.first->second, VAccess::READ};
        നിർത്തുക;
    }
    അവസ്ഥ Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = പുതിയ AstVarRef{flp, varp, VAccess::READ};
        നിർത്തുക;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    തിരികെ refp;
}

സ്ഥിരം അക്ഷരം* V3ConstPool::broken() {
    സ്ഥിരം V3ConstPool& self = instance();
    എങ്കിൽ (!self.m_cacheValid) തിരികെ nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    വേണ്ടി (സ്ഥിരം AstVar* സ്ഥിരം varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    വേണ്ടി (സ്ഥിരം auto& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    തിരികെ nullptr;
}
