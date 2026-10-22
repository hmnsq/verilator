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
#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3HashTable.h"

//============================================================================

class V3ConstPool final {
    friend class V3Global;  // Owns the instance, needs constructor access

    // TYPES
    // Scoping stage of the design, which determines how entries are referenced
    ಗಣನಾ class Stage : uint8_t {
        UNSCOPED,  // Before V3Scope: reference via the package
        SCOPED,  // Between V3Scope and V3Descope: reference via the VarScope
        DESCOPED  // After V3Descope: plain reference
    };
    // Hash and equality of entries in 'm_entries', by value
    ಸಂರಚನಾ EntryHash final {
        size_t operator()(ಧ್ರುವ AstVar* varp) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ (*this)(VN_AS(varp->valuep(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
        }
        size_t operator()(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* initp) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ hash_ಮೌಲ್ಯ(initp).ಮೌಲ್ಯ(); }
    };
    ಸಂರಚನಾ EntryEqual final {
        bool operator()(ಧ್ರುವ AstVar* ap, ಧ್ರುವ AstVar* bp) ಧ್ರುವ {  // LCOV_EXCL_START
            ಪ್ರತ್ಯಾಗಮ (*this)(ap, VN_AS(bp->valuep(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));  // Only on collision while rebuild
        }  // LCOV_EXCL_STOP
        bool operator()(ಧ್ರುವ AstVar* varp, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* initp) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ equal_ಮೌಲ್ಯ(VN_AS(varp->valuep(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), initp);
        }
    };

    // MEMBERS
    Stage m_stage = Stage::UNSCOPED;  // Scoping stage of the design, see setScoped()/setDescoped()
    bool m_cacheValid = false;  // Cache below is up to date
    AstScope* m_scopep = nullptr;  // Scope of the constant pool, between V3Scope and V3Descope
    V3HashSet<AstVar*, EntryHash, EntryEqual> m_entries;  // All entries
    V3HashMap<ಧ್ರುವ AstVar*, AstVarScope*> m_varScopes;  // VarScope of each entry iff m_scopep
    std::array<uint32_t, VNType::ಅಂಕೆ_TYPES()> m_next_ಸಂಖ್ಯೆ{};  // Sequence number for naming

    // METHODS
    ಸ್ಥಿರ V3ConstPool& instance() { ಪ್ರತ್ಯಾಗಮ *v3Global.constPoolp(); }
    // Hash of a value, consistent with 'equalValue'
    ಸ್ಥಿರ V3Hash hash_ಮೌಲ್ಯ(ಧ್ರುವ Ast_ಗಂಟು* nodep);
    // True if the two values are the same, so can use the same entry
    ಸ್ಥಿರ bool equal_ಮೌಲ್ಯ(ಧ್ರುವ Ast_ಗಂಟು* ap, ಧ್ರುವ Ast_ಗಂಟು* bp);
    // Rebuild the lookup cache from the constant pool package
    ಶೂನ್ಯ rebuildCache();

    // CONSTRUCTORS - only V3Global creates and deletes the instance
    V3ConstPool() = ಸ್ವಭಾವ;
    ~V3ConstPool() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(V3ConstPool);
    VL_UNMOVABLE(V3ConstPool);

ಸಾರ್ವಜನಿಕ:
    // STATIC METHODS
    // Find a constant pool variable with the given constant value, or create one if one does
    // not already exist, and return a read reference to it. The reference has the same dtype
    // as initp. The referenced variable *might* have a different, but compatible dtype.
    ಸ್ಥಿರ AstVarRef* find(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* initp);
    // Invalidate the lookup cache. This must be called after deleting any Vars from the pool.
    ಸ್ಥಿರ ಶೂನ್ಯ invalidateCache() { instance().m_cacheValid = false; }
    // Notify that the design has been scoped, called by V3Scope
    ಸ್ಥಿರ ಶೂನ್ಯ setScoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::UNSCOPED, "Constant pool scoped twice");
        self.m_stage = Stage::SCOPED;
        invalidateCache();
    }
    // Notify that the design has been descoped, called by V3Descope
    ಸ್ಥಿರ ಶೂನ್ಯ setDescoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::SCOPED, "Constant pool descoped when not scoped");
        self.m_stage = Stage::DESCOPED;
        invalidateCache();
    }
    // Check the lookup cache refers only to existing nodes, for V3Broken
    ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* broken();
};

#endif  // Guard
