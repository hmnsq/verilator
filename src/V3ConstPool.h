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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3CONSTPOOL_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3CONSTPOOL_H_
#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3HashTable.h"

//============================================================================

class V3ConstPool final {
    ਮਿੱਤਰ class V3Global;  // Owns the instance, needs constructor access

    // TYPES
    // Scoping stage of the design, which determines how entries are referenced
    ਗਿਣਤੀ class Stage : uint8_t {
        UNSCOPED,  // Before V3Scope: reference via the package
        SCOPED,  // Between V3Scope and V3Descope: reference via the VarScope
        DESCOPED  // After V3Descope: plain reference
    };
    // Hash and equality of entries in 'm_entries', by value
    ਬਣਤਰ ਇੰਦਰਾਜ_Hash final {
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ AstVar* varp) ਅਚਲ {
            ਵਾਪਸ (*this)(VN_AS(varp->valuep(), ਗੰਢ_ਵਿਅੰਜਕ));
        }
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* initp) ਅਚਲ { ਵਾਪਸ hash_ਮੁੱਲ(initp).ਮੁੱਲ(); }
    };
    ਬਣਤਰ ਇੰਦਰਾਜ_Equal final {
        bool ਸੰਚਾਲਕ()(ਅਚਲ AstVar* ap, ਅਚਲ AstVar* bp) ਅਚਲ {  // LCOV_EXCL_START
            ਵਾਪਸ (*this)(ap, VN_AS(bp->valuep(), ਗੰਢ_ਵਿਅੰਜਕ));  // Only on collision while rebuild
        }  // LCOV_EXCL_STOP
        bool ਸੰਚਾਲਕ()(ਅਚਲ AstVar* varp, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* initp) ਅਚਲ {
            ਵਾਪਸ equal_ਮੁੱਲ(VN_AS(varp->valuep(), ਗੰਢ_ਵਿਅੰਜਕ), initp);
        }
    };

    // MEMBERS
    Stage m_stage = Stage::UNSCOPED;  // Scoping stage of the design, see setScoped()/setDescoped()
    bool m_cacheValid = false;  // Cache below is up to date
    AstScope* m_scopep = nullptr;  // Scope of the constant pool, between V3Scope and V3Descope
    V3_Hash_ਸੈੱਟ<AstVar*, ਇੰਦਰਾਜ_Hash, ਇੰਦਰਾਜ_Equal> m_entries;  // All entries
    V3HashMap<ਅਚਲ AstVar*, AstVarScope*> m_varScopes;  // VarScope of each entry iff m_scopep
    std::array<uint32_t, VNType::ਅੰਕ_TYPES()> m_ਅਗਲਾ_ਸੰਖਿਆ{};  // Sequence number for naming

    // METHODS
    ਸਥਿਰ V3ConstPool& instance() { ਵਾਪਸ *v3Global.constPoolp(); }
    // Hash of a value, consistent with 'equalValue'
    ਸਥਿਰ V3Hash hash_ਮੁੱਲ(ਅਚਲ Ast_ਗੰਢ* nodep);
    // True if the two values are the same, so can use the same entry
    ਸਥਿਰ bool equal_ਮੁੱਲ(ਅਚਲ Ast_ਗੰਢ* ap, ਅਚਲ Ast_ਗੰਢ* bp);
    // Rebuild the lookup cache from the constant pool package
    ਖਾਲੀ rebuildCache();

    // CONSTRUCTORS - only V3Global creates and deletes the instance
    V3ConstPool() = ਮੂਲ;
    ~V3ConstPool() = ਮੂਲ;
    VL_UNCOPYABLE(V3ConstPool);
    VL_UNMOVABLE(V3ConstPool);

ਜਨਤਕ:
    // STATIC METHODS
    // Find a constant pool variable with the given constant value, or create one if one does
    // not already exist, and return a read reference to it. The reference has the same dtype
    // as initp. The referenced variable *might* have a different, but compatible dtype.
    ਸਥਿਰ AstVarRef* ਲੱਭੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* initp);
    // Invalidate the lookup cache. This must be called after deleting any Vars from the pool.
    ਸਥਿਰ ਖਾਲੀ invalidateCache() { instance().m_cacheValid = false; }
    // Notify that the design has been scoped, called by V3Scope
    ਸਥਿਰ ਖਾਲੀ ਸੈੱਟ_Scoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::UNSCOPED, "Constant pool scoped twice");
        self.m_stage = Stage::SCOPED;
        invalidateCache();
    }
    // Notify that the design has been descoped, called by V3Descope
    ਸਥਿਰ ਖਾਲੀ ਸੈੱਟ_Descoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::SCOPED, "Constant pool descoped when not scoped");
        self.m_stage = Stage::DESCOPED;
        invalidateCache();
    }
    // Check the lookup cache refers only to existing nodes, for V3Broken
    ਸਥਿਰ ਅਚਲ ਅੱਖਰ* broken();
};

#ਜੇ_ਅੰਤ  // Guard
