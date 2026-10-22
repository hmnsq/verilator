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
#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3HashTable.h"

//============================================================================

class V3ConstPool final {
    friend class V3Global;  // Owns the instance, needs constructor access

    // TYPES
    // Scoping stage of the design, which determines how entries are referenced
    ગણના class Stage : uint8_t {
        UNSCOPED,  // Before V3Scope: reference via the package
        SCOPED,  // Between V3Scope and V3Descope: reference via the VarScope
        DESCOPED  // After V3Descope: plain reference
    };
    // Hash and equality of entries in 'm_entries', by value
    સંરચના EntryHash final {
        size_t operator()(અચલ AstVar* varp) અચલ {
            પાછા (*this)(VN_AS(varp->valuep(), ગાંઠ_પદાવલિ));
        }
        size_t operator()(અચલ Ast_ગાંઠ_પદાવલિ* initp) અચલ { પાછા hash_કિંમત(initp).કિંમત(); }
    };
    સંરચના EntryEqual final {
        bool operator()(અચલ AstVar* ap, અચલ AstVar* bp) અચલ {  // LCOV_EXCL_START
            પાછા (*this)(ap, VN_AS(bp->valuep(), ગાંઠ_પદાવલિ));  // Only on collision while rebuild
        }  // LCOV_EXCL_STOP
        bool operator()(અચલ AstVar* varp, અચલ Ast_ગાંઠ_પદાવલિ* initp) અચલ {
            પાછા equal_કિંમત(VN_AS(varp->valuep(), ગાંઠ_પદાવલિ), initp);
        }
    };

    // MEMBERS
    Stage m_stage = Stage::UNSCOPED;  // Scoping stage of the design, see setScoped()/setDescoped()
    bool m_cacheValid = false;  // Cache below is up to date
    AstScope* m_scopep = nullptr;  // Scope of the constant pool, between V3Scope and V3Descope
    V3HashSet<AstVar*, EntryHash, EntryEqual> m_entries;  // All entries
    V3HashMap<અચલ AstVar*, AstVarScope*> m_varScopes;  // VarScope of each entry iff m_scopep
    std::array<uint32_t, VNType::અંક_TYPES()> m_next_સંખ્યા{};  // Sequence number for naming

    // METHODS
    સ્થિર V3ConstPool& instance() { પાછા *v3Global.constPoolp(); }
    // Hash of a value, consistent with 'equalValue'
    સ્થિર V3Hash hash_કિંમત(અચલ Ast_ગાંઠ* nodep);
    // True if the two values are the same, so can use the same entry
    સ્થિર bool equal_કિંમત(અચલ Ast_ગાંઠ* ap, અચલ Ast_ગાંઠ* bp);
    // Rebuild the lookup cache from the constant pool package
    શૂન્ય rebuildCache();

    // CONSTRUCTORS - only V3Global creates and deletes the instance
    V3ConstPool() = મૂળભૂત;
    ~V3ConstPool() = મૂળભૂત;
    VL_UNCOPYABLE(V3ConstPool);
    VL_UNMOVABLE(V3ConstPool);

જાહેર:
    // STATIC METHODS
    // Find a constant pool variable with the given constant value, or create one if one does
    // not already exist, and return a read reference to it. The reference has the same dtype
    // as initp. The referenced variable *might* have a different, but compatible dtype.
    સ્થિર AstVarRef* find(Ast_ગાંઠ_પદાવલિ* initp);
    // Invalidate the lookup cache. This must be called after deleting any Vars from the pool.
    સ્થિર શૂન્ય invalidateCache() { instance().m_cacheValid = false; }
    // Notify that the design has been scoped, called by V3Scope
    સ્થિર શૂન્ય setScoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::UNSCOPED, "Constant pool scoped twice");
        self.m_stage = Stage::SCOPED;
        invalidateCache();
    }
    // Notify that the design has been descoped, called by V3Descope
    સ્થિર શૂન્ય setDescoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::SCOPED, "Constant pool descoped when not scoped");
        self.m_stage = Stage::DESCOPED;
        invalidateCache();
    }
    // Check the lookup cache refers only to existing nodes, for V3Broken
    સ્થિર અચલ અક્ષર* broken();
};

#endif  // Guard
