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
#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3HashTable.h"

//============================================================================

class V3ConstPool final {
    friend class V3Global;  // Owns the instance, needs constructor access

    // TYPES
    // Scoping stage of the design, which determines how entries are referenced
    എണ്ണൽ class Stage : uint8_t {
        UNSCOPED,  // Before V3Scope: reference via the package
        SCOPED,  // Between V3Scope and V3Descope: reference via the VarScope
        DESCOPED  // After V3Descope: plain reference
    };
    // Hash and equality of entries in 'm_entries', by value
    ഘടന EntryHash final {
        size_t operator()(സ്ഥിരം AstVar* varp) സ്ഥിരം {
            തിരികെ (*this)(VN_AS(varp->valuep(), കണ്ണി_വ്യഞ്ജകം));
        }
        size_t operator()(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* initp) സ്ഥിരം { തിരികെ hash_മൂല്യം(initp).മൂല്യം(); }
    };
    ഘടന EntryEqual final {
        bool operator()(സ്ഥിരം AstVar* ap, സ്ഥിരം AstVar* bp) സ്ഥിരം {  // LCOV_EXCL_START
            തിരികെ (*this)(ap, VN_AS(bp->valuep(), കണ്ണി_വ്യഞ്ജകം));  // Only on collision while rebuild
        }  // LCOV_EXCL_STOP
        bool operator()(സ്ഥിരം AstVar* varp, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* initp) സ്ഥിരം {
            തിരികെ equal_മൂല്യം(VN_AS(varp->valuep(), കണ്ണി_വ്യഞ്ജകം), initp);
        }
    };

    // MEMBERS
    Stage m_stage = Stage::UNSCOPED;  // Scoping stage of the design, see setScoped()/setDescoped()
    bool m_cacheValid = false;  // Cache below is up to date
    AstScope* m_scopep = nullptr;  // Scope of the constant pool, between V3Scope and V3Descope
    V3HashSet<AstVar*, EntryHash, EntryEqual> m_entries;  // All entries
    V3HashMap<സ്ഥിരം AstVar*, AstVarScope*> m_varScopes;  // VarScope of each entry iff m_scopep
    std::array<uint32_t, VNType::അക്കം_TYPES()> m_next_സംഖ്യ{};  // Sequence number for naming

    // METHODS
    സ്ഥിതം V3ConstPool& instance() { തിരികെ *v3Global.constPoolp(); }
    // Hash of a value, consistent with 'equalValue'
    സ്ഥിതം V3Hash hash_മൂല്യം(സ്ഥിരം Ast_കണ്ണി* nodep);
    // True if the two values are the same, so can use the same entry
    സ്ഥിതം bool equal_മൂല്യം(സ്ഥിരം Ast_കണ്ണി* ap, സ്ഥിരം Ast_കണ്ണി* bp);
    // Rebuild the lookup cache from the constant pool package
    ശൂന്യം rebuildCache();

    // CONSTRUCTORS - only V3Global creates and deletes the instance
    V3ConstPool() = സ്ഥിരസ്ഥിതി;
    ~V3ConstPool() = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(V3ConstPool);
    VL_UNMOVABLE(V3ConstPool);

പൊതു:
    // STATIC METHODS
    // Find a constant pool variable with the given constant value, or create one if one does
    // not already exist, and return a read reference to it. The reference has the same dtype
    // as initp. The referenced variable *might* have a different, but compatible dtype.
    സ്ഥിതം AstVarRef* find(Ast_കണ്ണി_വ്യഞ്ജകം* initp);
    // Invalidate the lookup cache. This must be called after deleting any Vars from the pool.
    സ്ഥിതം ശൂന്യം invalidateCache() { instance().m_cacheValid = false; }
    // Notify that the design has been scoped, called by V3Scope
    സ്ഥിതം ശൂന്യം setScoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::UNSCOPED, "Constant pool scoped twice");
        self.m_stage = Stage::SCOPED;
        invalidateCache();
    }
    // Notify that the design has been descoped, called by V3Descope
    സ്ഥിതം ശൂന്യം setDescoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::SCOPED, "Constant pool descoped when not scoped");
        self.m_stage = Stage::DESCOPED;
        invalidateCache();
    }
    // Check the lookup cache refers only to existing nodes, for V3Broken
    സ്ഥിതം സ്ഥിരം അക്ഷരം* broken();
};

#endif  // Guard
