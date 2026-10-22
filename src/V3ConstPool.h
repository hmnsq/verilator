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
//
//  V3ConstPool finds or creates entries in the constant pool package
//  (AstNetlist::constPoolPkgp()), which holds read-only static data, i.e.:
//  constant values of any type, shared across the whole design. Entries are
//  ordinary package variables, deduplicated by value.
//
//*************************************************************************

#अगर_अघोषित VERILATOR_V3CONSTPOOL_H_
#घोषणा VERILATOR_V3CONSTPOOL_H_
#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3HashTable.h"

//============================================================================

class V3ConstPool final {
    मित्र class V3Global;  // Owns the instance, needs constructor access

    // TYPES
    // Scoping stage of the design, which determines how entries are referenced
    क्रमागत class Stage : uint8_t {
        UNSCOPED,  // Before V3Scope: reference via the package
        SCOPED,  // Between V3Scope and V3Descope: reference via the VarScope
        DESCOPED  // After V3Descope: plain reference
    };
    // Hash and equality of entries in 'm_entries', by value
    काष्ठा EntryHash final {
        माप_प्रकार चालक()(स्थिर AstVar* varp) स्थिर {
            वापस (*this)(VN_AS(varp->valuep(), गाँठ_व्यंजक));
        }
        माप_प्रकार चालक()(स्थिर Ast_गाँठ_व्यंजक* initp) स्थिर { वापस hash_मूल्य(initp).मूल्य(); }
    };
    काष्ठा EntryEqual final {
        bool चालक()(स्थिर AstVar* ap, स्थिर AstVar* bp) स्थिर {  // LCOV_EXCL_START
            वापस (*this)(ap, VN_AS(bp->valuep(), गाँठ_व्यंजक));  // Only on collision while rebuild
        }  // LCOV_EXCL_STOP
        bool चालक()(स्थिर AstVar* varp, स्थिर Ast_गाँठ_व्यंजक* initp) स्थिर {
            वापस equal_मूल्य(VN_AS(varp->valuep(), गाँठ_व्यंजक), initp);
        }
    };

    // MEMBERS
    Stage m_stage = Stage::UNSCOPED;  // Scoping stage of the design, see setScoped()/setDescoped()
    bool m_cacheValid = false;  // Cache below is up to date
    AstScope* m_scopep = nullptr;  // Scope of the constant pool, between V3Scope and V3Descope
    V3HashSet<AstVar*, EntryHash, EntryEqual> m_entries;  // All entries
    V3HashMap<स्थिर AstVar*, AstVarScope*> m_varScopes;  // VarScope of each entry iff m_scopep
    std::array<uint32_t, VNType::अंक_TYPES()> m_अग्र_संख्या{};  // Sequence number for naming

    // METHODS
    अटल V3ConstPool& instance() { वापस *v3Global.constPoolp(); }
    // Hash of a value, consistent with 'equalValue'
    अटल V3Hash hash_मूल्य(स्थिर Ast_गाँठ* nodep);
    // True if the two values are the same, so can use the same entry
    अटल bool equal_मूल्य(स्थिर Ast_गाँठ* ap, स्थिर Ast_गाँठ* bp);
    // Rebuild the lookup cache from the constant pool package
    व्योम rebuildCache();

    // CONSTRUCTORS - only V3Global creates and deletes the instance
    V3ConstPool() = शेष;
    ~V3ConstPool() = शेष;
    VL_UNCOPYABLE(V3ConstPool);
    VL_UNMOVABLE(V3ConstPool);

खुला:
    // STATIC METHODS
    // Find a constant pool variable with the given constant value, or create one if one does
    // not already exist, and return a read reference to it. The reference has the same dtype
    // as initp. The referenced variable *might* have a different, but compatible dtype.
    अटल AstVarRef* find(Ast_गाँठ_व्यंजक* initp);
    // Invalidate the lookup cache. This must be called after deleting any Vars from the pool.
    अटल व्योम invalidateCache() { instance().m_cacheValid = false; }
    // Notify that the design has been scoped, called by V3Scope
    अटल व्योम setScoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::UNSCOPED, "Constant pool scoped twice");
        self.m_stage = Stage::SCOPED;
        invalidateCache();
    }
    // Notify that the design has been descoped, called by V3Descope
    अटल व्योम setDescoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::SCOPED, "Constant pool descoped when not scoped");
        self.m_stage = Stage::DESCOPED;
        invalidateCache();
    }
    // Check the lookup cache refers only to existing nodes, for V3Broken
    अटल स्थिर अक्षर* broken();
};

#पूर्ण_अगर  // Guard
