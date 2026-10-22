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
#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3HashTable.h"

//============================================================================

class V3ConstPool final {
    friend class V3Global;  // Owns the instance, needs constructor access

    // TYPES
    // Scoping stage of the design, which determines how entries are referenced
    شمارش class Stage : uint8_t {
        UNSCOPED,  // Before V3Scope: reference via the package
        SCOPED,  // Between V3Scope and V3Descope: reference via the VarScope
        DESCOPED  // After V3Descope: plain reference
    };
    // Hash and equality of entries in 'm_entries', by value
    ساختار EntryHash final {
        size_t operator()(ثابت AstVar* varp) ثابت {
            بازگشت (*this)(VN_AS(varp->valuep(), گره_عبارت));
        }
        size_t operator()(ثابت Ast_گره_عبارت* initp) ثابت { بازگشت hash_مقدار(initp).مقدار(); }
    };
    ساختار EntryEqual final {
        bool operator()(ثابت AstVar* ap, ثابت AstVar* bp) ثابت {  // LCOV_EXCL_START
            بازگشت (*this)(ap, VN_AS(bp->valuep(), گره_عبارت));  // Only on collision while rebuild
        }  // LCOV_EXCL_STOP
        bool operator()(ثابت AstVar* varp, ثابت Ast_گره_عبارت* initp) ثابت {
            بازگشت equal_مقدار(VN_AS(varp->valuep(), گره_عبارت), initp);
        }
    };

    // MEMBERS
    Stage m_stage = Stage::UNSCOPED;  // Scoping stage of the design, see setScoped()/setDescoped()
    bool m_cacheValid = false;  // Cache below is up to date
    AstScope* m_scopep = nullptr;  // Scope of the constant pool, between V3Scope and V3Descope
    V3HashSet<AstVar*, EntryHash, EntryEqual> m_entries;  // All entries
    V3HashMap<ثابت AstVar*, AstVarScope*> m_varScopes;  // VarScope of each entry iff m_scopep
    std::array<uint32_t, VNType::رقم_TYPES()> m_next_عدد{};  // Sequence number for naming

    // METHODS
    ایستا V3ConstPool& instance() { بازگشت *v3Global.constPoolp(); }
    // Hash of a value, consistent with 'equalValue'
    ایستا V3Hash hash_مقدار(ثابت Ast_گره* nodep);
    // True if the two values are the same, so can use the same entry
    ایستا bool equal_مقدار(ثابت Ast_گره* ap, ثابت Ast_گره* bp);
    // Rebuild the lookup cache from the constant pool package
    تهی rebuildCache();

    // CONSTRUCTORS - only V3Global creates and deletes the instance
    V3ConstPool() = پیشفرض;
    ~V3ConstPool() = پیشفرض;
    VL_UNCOPYABLE(V3ConstPool);
    VL_UNMOVABLE(V3ConstPool);

عمومی:
    // STATIC METHODS
    // Find a constant pool variable with the given constant value, or create one if one does
    // not already exist, and return a read reference to it. The reference has the same dtype
    // as initp. The referenced variable *might* have a different, but compatible dtype.
    ایستا AstVarRef* find(Ast_گره_عبارت* initp);
    // Invalidate the lookup cache. This must be called after deleting any Vars from the pool.
    ایستا تهی invalidateCache() { instance().m_cacheValid = false; }
    // Notify that the design has been scoped, called by V3Scope
    ایستا تهی setScoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::UNSCOPED, "Constant pool scoped twice");
        self.m_stage = Stage::SCOPED;
        invalidateCache();
    }
    // Notify that the design has been descoped, called by V3Descope
    ایستا تهی setDescoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::SCOPED, "Constant pool descoped when not scoped");
        self.m_stage = Stage::DESCOPED;
        invalidateCache();
    }
    // Check the lookup cache refers only to existing nodes, for V3Broken
    ایستا ثابت نویسه* broken();
};

#endif  // Guard
