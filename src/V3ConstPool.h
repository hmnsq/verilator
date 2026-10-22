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
#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3HashTable.h"

//============================================================================

class V3ConstPool final {
    friend class V3Global;  // Owns the instance, needs constructor access

    // TYPES
    // Scoping stage of the design, which determines how entries are referenced
    எண்ணிக்கை class Stage : uint8_t {
        UNSCOPED,  // Before V3Scope: reference via the package
        SCOPED,  // Between V3Scope and V3Descope: reference via the VarScope
        DESCOPED  // After V3Descope: plain reference
    };
    // Hash and equality of entries in 'm_entries', by value
    கட்டமைப்பு EntryHash final {
        size_t operator()(மாறிலி AstVar* varp) மாறிலி {
            திரும்பு (*this)(VN_AS(varp->valuep(), முனை_கோவை));
        }
        size_t operator()(மாறிலி Ast_முனை_கோவை* initp) மாறிலி { திரும்பு hash_மதிப்பு(initp).மதிப்பு(); }
    };
    கட்டமைப்பு EntryEqual final {
        bool operator()(மாறிலி AstVar* ap, மாறிலி AstVar* bp) மாறிலி {  // LCOV_EXCL_START
            திரும்பு (*this)(ap, VN_AS(bp->valuep(), முனை_கோவை));  // Only on collision while rebuild
        }  // LCOV_EXCL_STOP
        bool operator()(மாறிலி AstVar* varp, மாறிலி Ast_முனை_கோவை* initp) மாறிலி {
            திரும்பு equal_மதிப்பு(VN_AS(varp->valuep(), முனை_கோவை), initp);
        }
    };

    // MEMBERS
    Stage m_stage = Stage::UNSCOPED;  // Scoping stage of the design, see setScoped()/setDescoped()
    bool m_cacheValid = false;  // Cache below is up to date
    AstScope* m_scopep = nullptr;  // Scope of the constant pool, between V3Scope and V3Descope
    V3HashSet<AstVar*, EntryHash, EntryEqual> m_entries;  // All entries
    V3HashMap<மாறிலி AstVar*, AstVarScope*> m_varScopes;  // VarScope of each entry iff m_scopep
    std::array<uint32_t, VNType::இலக்கம்_TYPES()> m_next_எண்{};  // Sequence number for naming

    // METHODS
    நிலையான V3ConstPool& instance() { திரும்பு *v3Global.constPoolp(); }
    // Hash of a value, consistent with 'equalValue'
    நிலையான V3Hash hash_மதிப்பு(மாறிலி Ast_முனை* nodep);
    // True if the two values are the same, so can use the same entry
    நிலையான bool equal_மதிப்பு(மாறிலி Ast_முனை* ap, மாறிலி Ast_முனை* bp);
    // Rebuild the lookup cache from the constant pool package
    வெறுமை rebuildCache();

    // CONSTRUCTORS - only V3Global creates and deletes the instance
    V3ConstPool() = இயல்பு;
    ~V3ConstPool() = இயல்பு;
    VL_UNCOPYABLE(V3ConstPool);
    VL_UNMOVABLE(V3ConstPool);

பொது:
    // STATIC METHODS
    // Find a constant pool variable with the given constant value, or create one if one does
    // not already exist, and return a read reference to it. The reference has the same dtype
    // as initp. The referenced variable *might* have a different, but compatible dtype.
    நிலையான AstVarRef* find(Ast_முனை_கோவை* initp);
    // Invalidate the lookup cache. This must be called after deleting any Vars from the pool.
    நிலையான வெறுமை invalidateCache() { instance().m_cacheValid = false; }
    // Notify that the design has been scoped, called by V3Scope
    நிலையான வெறுமை setScoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::UNSCOPED, "Constant pool scoped twice");
        self.m_stage = Stage::SCOPED;
        invalidateCache();
    }
    // Notify that the design has been descoped, called by V3Descope
    நிலையான வெறுமை setDescoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::SCOPED, "Constant pool descoped when not scoped");
        self.m_stage = Stage::DESCOPED;
        invalidateCache();
    }
    // Check the lookup cache refers only to existing nodes, for V3Broken
    நிலையான மாறிலி எழுத்து* broken();
};

#endif  // Guard
