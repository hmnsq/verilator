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
#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3HashTable.h"

//============================================================================

class V3ConstPool final {
    friend class V3Global;  // Owns the instance, needs constructor access

    // TYPES
    // Scoping stage of the design, which determines how entries are referenced
    গণনা class Stage : uint8_t {
        UNSCOPED,  // Before V3Scope: reference via the package
        SCOPED,  // Between V3Scope and V3Descope: reference via the VarScope
        DESCOPED  // After V3Descope: plain reference
    };
    // Hash and equality of entries in 'm_entries', by value
    সংরচনা EntryHash final {
        size_t operator()(ধ্রুব AstVar* varp) ধ্রুব {
            প্রত্যাগম (*this)(VN_AS(varp->valuep(), গ্রন্থি_রাশিমালা));
        }
        size_t operator()(ধ্রুব Ast_গ্রন্থি_রাশিমালা* initp) ধ্রুব { প্রত্যাগম hash_মান(initp).মান(); }
    };
    সংরচনা EntryEqual final {
        bool operator()(ধ্রুব AstVar* ap, ধ্রুব AstVar* bp) ধ্রুব {  // LCOV_EXCL_START
            প্রত্যাগম (*this)(ap, VN_AS(bp->valuep(), গ্রন্থি_রাশিমালা));  // Only on collision while rebuild
        }  // LCOV_EXCL_STOP
        bool operator()(ধ্রুব AstVar* varp, ধ্রুব Ast_গ্রন্থি_রাশিমালা* initp) ধ্রুব {
            প্রত্যাগম equal_মান(VN_AS(varp->valuep(), গ্রন্থি_রাশিমালা), initp);
        }
    };

    // MEMBERS
    Stage m_stage = Stage::UNSCOPED;  // Scoping stage of the design, see setScoped()/setDescoped()
    bool m_cacheValid = false;  // Cache below is up to date
    AstScope* m_scopep = nullptr;  // Scope of the constant pool, between V3Scope and V3Descope
    V3HashSet<AstVar*, EntryHash, EntryEqual> m_entries;  // All entries
    V3HashMap<ধ্রুব AstVar*, AstVarScope*> m_varScopes;  // VarScope of each entry iff m_scopep
    std::array<uint32_t, VNType::অঙ্ক_TYPES()> m_next_সংখ্যা{};  // Sequence number for naming

    // METHODS
    স্থির V3ConstPool& instance() { প্রত্যাগম *v3Global.constPoolp(); }
    // Hash of a value, consistent with 'equalValue'
    স্থির V3Hash hash_মান(ধ্রুব Ast_গ্রন্থি* nodep);
    // True if the two values are the same, so can use the same entry
    স্থির bool equal_মান(ধ্রুব Ast_গ্রন্থি* ap, ধ্রুব Ast_গ্রন্থি* bp);
    // Rebuild the lookup cache from the constant pool package
    শূন্য rebuildCache();

    // CONSTRUCTORS - only V3Global creates and deletes the instance
    V3ConstPool() = স্বভাব;
    ~V3ConstPool() = স্বভাব;
    VL_UNCOPYABLE(V3ConstPool);
    VL_UNMOVABLE(V3ConstPool);

সার্বজনিক:
    // STATIC METHODS
    // Find a constant pool variable with the given constant value, or create one if one does
    // not already exist, and return a read reference to it. The reference has the same dtype
    // as initp. The referenced variable *might* have a different, but compatible dtype.
    স্থির AstVarRef* find(Ast_গ্রন্থি_রাশিমালা* initp);
    // Invalidate the lookup cache. This must be called after deleting any Vars from the pool.
    স্থির শূন্য invalidateCache() { instance().m_cacheValid = false; }
    // Notify that the design has been scoped, called by V3Scope
    স্থির শূন্য setScoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::UNSCOPED, "Constant pool scoped twice");
        self.m_stage = Stage::SCOPED;
        invalidateCache();
    }
    // Notify that the design has been descoped, called by V3Descope
    স্থির শূন্য setDescoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::SCOPED, "Constant pool descoped when not scoped");
        self.m_stage = Stage::DESCOPED;
        invalidateCache();
    }
    // Check the lookup cache refers only to existing nodes, for V3Broken
    স্থির ধ্রুব বর্ণ* broken();
};

#endif  // Guard
