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

#ifndef VERILATOR_V3CONSTPOOL_H_
#define VERILATOR_V3CONSTPOOL_H_
#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3HashTable.h"

//============================================================================

class V3ConstPool final {
    friend class V3Global;  // Owns the instance, needs constructor access

    // TYPES
    // Scoping stage of the design, which determines how entries are referenced
    गणना class Stage : uint8_t {
        UNSCOPED,  // Before V3Scope: reference via the package
        SCOPED,  // Between V3Scope and V3Descope: reference via the VarScope
        DESCOPED  // After V3Descope: plain reference
    };
    // Hash and equality of entries in 'm_entries', by value
    संरचना EntryHash final {
        size_t operator()(ध्रुव AstVar* varp) ध्रुव {
            प्रत्यागम (*this)(VN_AS(varp->valuep(), गाँठ_व्यंजक));
        }
        size_t operator()(ध्रुव Ast_गाँठ_व्यंजक* initp) ध्रुव { प्रत्यागम hash_मूल्य(initp).मूल्य(); }
    };
    संरचना EntryEqual final {
        bool operator()(ध्रुव AstVar* ap, ध्रुव AstVar* bp) ध्रुव {  // LCOV_EXCL_START
            प्रत्यागम (*this)(ap, VN_AS(bp->valuep(), गाँठ_व्यंजक));  // Only on collision while rebuild
        }  // LCOV_EXCL_STOP
        bool operator()(ध्रुव AstVar* varp, ध्रुव Ast_गाँठ_व्यंजक* initp) ध्रुव {
            प्रत्यागम equal_मूल्य(VN_AS(varp->valuep(), गाँठ_व्यंजक), initp);
        }
    };

    // MEMBERS
    Stage m_stage = Stage::UNSCOPED;  // Scoping stage of the design, see setScoped()/setDescoped()
    bool m_cacheValid = false;  // Cache below is up to date
    AstScope* m_scopep = nullptr;  // Scope of the constant pool, between V3Scope and V3Descope
    V3HashSet<AstVar*, EntryHash, EntryEqual> m_entries;  // All entries
    V3HashMap<ध्रुव AstVar*, AstVarScope*> m_varScopes;  // VarScope of each entry iff m_scopep
    std::array<uint32_t, VNType::अंक_TYPES()> m_next_संख्या{};  // Sequence number for naming

    // METHODS
    स्थिर V3ConstPool& instance() { प्रत्यागम *v3Global.constPoolp(); }
    // Hash of a value, consistent with 'equalValue'
    स्थिर V3Hash hash_मूल्य(ध्रुव Ast_गाँठ* nodep);
    // True if the two values are the same, so can use the same entry
    स्थिर bool equal_मूल्य(ध्रुव Ast_गाँठ* ap, ध्रुव Ast_गाँठ* bp);
    // Rebuild the lookup cache from the constant pool package
    शून्य rebuildCache();

    // CONSTRUCTORS - only V3Global creates and deletes the instance
    V3ConstPool() = स्वभाव;
    ~V3ConstPool() = स्वभाव;
    VL_UNCOPYABLE(V3ConstPool);
    VL_UNMOVABLE(V3ConstPool);

सार्वजनिक:
    // STATIC METHODS
    // Find a constant pool variable with the given constant value, or create one if one does
    // not already exist, and return a read reference to it. The reference has the same dtype
    // as initp. The referenced variable *might* have a different, but compatible dtype.
    स्थिर AstVarRef* find(Ast_गाँठ_व्यंजक* initp);
    // Invalidate the lookup cache. This must be called after deleting any Vars from the pool.
    स्थिर शून्य invalidateCache() { instance().m_cacheValid = false; }
    // Notify that the design has been scoped, called by V3Scope
    स्थिर शून्य setScoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::UNSCOPED, "Constant pool scoped twice");
        self.m_stage = Stage::SCOPED;
        invalidateCache();
    }
    // Notify that the design has been descoped, called by V3Descope
    स्थिर शून्य setDescoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::SCOPED, "Constant pool descoped when not scoped");
        self.m_stage = Stage::DESCOPED;
        invalidateCache();
    }
    // Check the lookup cache refers only to existing nodes, for V3Broken
    स्थिर ध्रुव वर्ण* broken();
};

#endif  // Guard
