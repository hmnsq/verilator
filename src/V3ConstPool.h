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
#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3HashTable.h"

//============================================================================

class V3ConstPool final {
    friend class V3Global;  // Owns the instance, needs constructor access

    // TYPES
    // Scoping stage of the design, which determines how entries are referenced
    ଗଣନା class Stage : uint8_t {
        UNSCOPED,  // Before V3Scope: reference via the package
        SCOPED,  // Between V3Scope and V3Descope: reference via the VarScope
        DESCOPED  // After V3Descope: plain reference
    };
    // Hash and equality of entries in 'm_entries', by value
    ସଂରଚନା EntryHash final {
        size_t operator()(ଅଚଳ AstVar* varp) ଅଚଳ {
            ଫେରନ୍ତୁ (*this)(VN_AS(varp->valuep(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି));
        }
        size_t operator()(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* initp) ଅଚଳ { ଫେରନ୍ତୁ hash_ମୂଲ୍ୟ(initp).ମୂଲ୍ୟ(); }
    };
    ସଂରଚନା EntryEqual final {
        bool operator()(ଅଚଳ AstVar* ap, ଅଚଳ AstVar* bp) ଅଚଳ {  // LCOV_EXCL_START
            ଫେରନ୍ତୁ (*this)(ap, VN_AS(bp->valuep(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି));  // Only on collision while rebuild
        }  // LCOV_EXCL_STOP
        bool operator()(ଅଚଳ AstVar* varp, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* initp) ଅଚଳ {
            ଫେରନ୍ତୁ equal_ମୂଲ୍ୟ(VN_AS(varp->valuep(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି), initp);
        }
    };

    // MEMBERS
    Stage m_stage = Stage::UNSCOPED;  // Scoping stage of the design, see setScoped()/setDescoped()
    bool m_cacheValid = false;  // Cache below is up to date
    AstScope* m_scopep = nullptr;  // Scope of the constant pool, between V3Scope and V3Descope
    V3HashSet<AstVar*, EntryHash, EntryEqual> m_entries;  // All entries
    V3HashMap<ଅଚଳ AstVar*, AstVarScope*> m_varScopes;  // VarScope of each entry iff m_scopep
    std::array<uint32_t, VNType::ଅଙ୍କ_TYPES()> m_next_ସଂଖ୍ୟା{};  // Sequence number for naming

    // METHODS
    ସ୍ଥିର V3ConstPool& instance() { ଫେରନ୍ତୁ *v3Global.constPoolp(); }
    // Hash of a value, consistent with 'equalValue'
    ସ୍ଥିର V3Hash hash_ମୂଲ୍ୟ(ଅଚଳ Ast_ଗଣ୍ଠି* nodep);
    // True if the two values are the same, so can use the same entry
    ସ୍ଥିର bool equal_ମୂଲ୍ୟ(ଅଚଳ Ast_ଗଣ୍ଠି* ap, ଅଚଳ Ast_ଗଣ୍ଠି* bp);
    // Rebuild the lookup cache from the constant pool package
    ଶୂନ୍ୟ rebuildCache();

    // CONSTRUCTORS - only V3Global creates and deletes the instance
    V3ConstPool() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~V3ConstPool() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(V3ConstPool);
    VL_UNMOVABLE(V3ConstPool);

ସର୍ବସାଧାରଣ:
    // STATIC METHODS
    // Find a constant pool variable with the given constant value, or create one if one does
    // not already exist, and return a read reference to it. The reference has the same dtype
    // as initp. The referenced variable *might* have a different, but compatible dtype.
    ସ୍ଥିର AstVarRef* find(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* initp);
    // Invalidate the lookup cache. This must be called after deleting any Vars from the pool.
    ସ୍ଥିର ଶୂନ୍ୟ invalidateCache() { instance().m_cacheValid = false; }
    // Notify that the design has been scoped, called by V3Scope
    ସ୍ଥିର ଶୂନ୍ୟ setScoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::UNSCOPED, "Constant pool scoped twice");
        self.m_stage = Stage::SCOPED;
        invalidateCache();
    }
    // Notify that the design has been descoped, called by V3Descope
    ସ୍ଥିର ଶୂନ୍ୟ setDescoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::SCOPED, "Constant pool descoped when not scoped");
        self.m_stage = Stage::DESCOPED;
        invalidateCache();
    }
    // Check the lookup cache refers only to existing nodes, for V3Broken
    ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* broken();
};

#endif  // Guard
