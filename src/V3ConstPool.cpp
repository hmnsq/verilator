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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3ConstPool.h"

#अन्तर्भाव "V3Hasher.h"

//######################################################################
// V3ConstPool

V3Hash V3ConstPool::hash_मूल्य(स्थिरांक Ast_गाठ* nodep) {
    // Hash what 'equalValue' compares
    V3Hash hash{nodep->type().hash()};
    hash += nodep->dtypep()->cType("", असत्य, असत्य);
    जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep, Const)) परत hash + constp->अंक().toHash();
    जर (स्थिरांक AstConsArray* स्थिरांक consp = VN_CAST(nodep, ConsArray)) {
        स्थिरांक uint64_t size
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        साठी (uint64_t n = 0; n < size; ++n) hash += hash_मूल्य(consp->valuep(n));
        परत hash;
    }
    परत hash + V3Hasher::uncachedHash(nodep);
}

तार्किक V3ConstPool::equal_मूल्य(स्थिरांक Ast_गाठ* ap, स्थिरांक Ast_गाठ* bp) {
    जर (ap->type() != bp->type()) परत असत्य;
    जर (ap->dtypep()->cType("", असत्य, असत्य) != bp->dtypep()->cType("", असत्य, असत्य)) {
        परत असत्य;
    }
    // Compare constants by value, which also checks the width
    जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(ap, Const)) {
        परत constp->अंक().isCaseEq(VN_AS(bp, Const)->अंक());
    }
    // Compare unpacked arrays by element values, as equal arrays might differ in their range,
    // and in which elements are given or defaulted
    जर (स्थिरांक AstConsArray* स्थिरांक aConsp = VN_CAST(ap, ConsArray)) {
        स्थिरांक AstConsArray* स्थिरांक bConsp = VN_AS(bp, ConsArray);
        स्थिरांक uint64_t size
            = VN_AS(aConsp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
        साठी (uint64_t n = 0; n < size; ++n) {
            जर (!equal_मूल्य(aConsp->valuep(n), bConsp->valuep(n))) परत असत्य;
        }
        परत सत्य;
    }
    परत ap->same_वृक्ष(bp);
}

शून्य V3ConstPool::rebuildCache() {
    m_cacheValid = सत्य;
    m_scopep = nullptr;
    m_entries.clear();
    m_varScopes.clear();
    AstPackage* स्थिरांक pkgp = v3Global.rootp()->constPoolPkgp();
    साठी (Ast_गाठ* nodep = pkgp->stmtsp(); nodep; nodep = nodep->nextp()) {
        जर (AstScope* स्थिरांक scopep = VN_CAST(nodep, Scope)) {
            जर (m_stage == Stage::SCOPED) m_scopep = scopep;
            सुरूठेवा;
        }
        // Otherwise must be a variable
        AstVar* स्थिरांक varp = VN_AS(nodep, Var);
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Unmarked variable in constant pool");
        m_entries.घाला(varp);
    }
    जर (m_scopep) {
        साठी (AstVarScope* vscp = m_scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            m_varScopes.घाला({vscp->varp(), vscp});
        }
    }
}

AstVarRef* V3ConstPool::find(Ast_गाठ_अभिव्यक्ती* initp) {
    UASSERT_OBJ(initp->is_Constant_वृक्ष(), initp, "Const pool value is not constant");
    V3ConstPool& self = instance();
    जर (VL_UNLIKELY(!self.m_cacheValid)) self.rebuildCache();

    File_ओळ* स्थिरांक flp = initp->fileline();
    Ast_गाठ_DType* स्थिरांक dtypep = initp->dtypep();

    // Look up/create the variable with this value
    स्थिरांक स्वतः pair = self.m_entries.घाला_Lazy(initp, [&]() {
        uint32_t& numberr = self.m_next_संख्या[initp->type()];
        स्थिरांक std::string name = std::string{initp->typeName()} + "_" + std::to_string(numberr++);
        AstVar* स्थिरांक varp = नवीन AstVar{flp, VVarType::MODULETEMP, name, dtypep};
        varp->setConstPoolEntry();
        varp->lifetime(VLifetime::STATIC_EXPLICIT);
        varp->isConst(सत्य);
        varp->valuep(initp->clone_वृक्ष(असत्य));
        v3Global.rootp()->constPoolPkgp()->addStmtsp(varp);
        परत varp;
    });
    AstVar* स्थिरांक varp = *pair.first;

    // Create a read reference to the entry, of the right form for the current stage
    AstVarRef* refp = nullptr;
    निवड (self.m_stage) {
    स्थिती Stage::UNSCOPED: {
        // Before V3Scope, reference via the package, as any other package variable
        AstPackage* स्थिरांक pkgp = v3Global.rootp()->constPoolPkgp();
        refp = नवीन AstVarRef{flp, pkgp, varp, VAccess::READ};
        थांबा;
    }
    स्थिती Stage::SCOPED: {
        // While scoped, reference the VarScope, creating it for a new entry, or if removed
        स्थिरांक स्वतः vscpPair = self.m_varScopes.घाला_Lazy(varp, [&]() {
            AstVarScope* स्थिरांक vscp = नवीन AstVarScope{varp->fileline(), self.m_scopep, varp};
            self.m_scopep->addVarsp(vscp);
            परत std::pair<स्थिरांक AstVar*, AstVarScope*>{varp, vscp};
        });
        refp = नवीन AstVarRef{flp, vscpPair.first->second, VAccess::READ};
        थांबा;
    }
    स्थिती Stage::DESCOPED: {
        // After V3Descope, no VarScope is needed
        refp = नवीन AstVarRef{flp, varp, VAccess::READ};
        थांबा;
    }
    }

    // The entry might have been created with a different, but compatible dtype
    refp->dtypep(dtypep);
    परत refp;
}

स्थिरांक अक्षर* V3ConstPool::broken() {
    स्थिरांक V3ConstPool& self = instance();
    जर (!self.m_cacheValid) परत nullptr;
    BROKEN_RTN(self.m_scopep && !self.m_scopep->brokeExists());
    साठी (स्थिरांक AstVar* स्थिरांक varp : self.m_entries) BROKEN_RTN(!varp->brokeExists());
    साठी (स्थिरांक स्वतः& pair : self.m_varScopes) {
        BROKEN_RTN(!pair.first->brokeExists());
        BROKEN_RTN(!pair.second->brokeExists());
    }
    परत nullptr;
}
