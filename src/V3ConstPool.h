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

#که_نه_تعریف_شوی VERILATOR_V3CONSTPOOL_H_
#تعریف_کړه VERILATOR_V3CONSTPOOL_H_
#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3HashTable.h"

//============================================================================

class V3ConstPool final {
    ملګری class V3Global;  // Owns the instance, needs constructor access

    // TYPES
    // Scoping stage of the design, which determines how entries are referenced
    تعداد class Stage : uint8_t {
        UNSCOPED,  // Before V3Scope: reference via the package
        SCOPED,  // Between V3Scope and V3Descope: reference via the VarScope
        DESCOPED  // After V3Descope: plain reference
    };
    // Hash and equality of entries in 'm_entries', by value
    بنية ثبت_Hash final {
        اندازه_t چلوونکی()(ثابت AstVar* varp) ثابت {
            أرجع (*this)(VN_AS(varp->valuep(), غوټه_بیان));
        }
        اندازه_t چلوونکی()(ثابت Ast_غوټه_بیان* initp) ثابت { أرجع hash_ارزښت(initp).ارزښت(); }
    };
    بنية ثبت_Equal final {
        bool چلوونکی()(ثابت AstVar* ap, ثابت AstVar* bp) ثابت {  // LCOV_EXCL_START
            أرجع (*this)(ap, VN_AS(bp->valuep(), غوټه_بیان));  // Only on collision while rebuild
        }  // LCOV_EXCL_STOP
        bool چلوونکی()(ثابت AstVar* varp, ثابت Ast_غوټه_بیان* initp) ثابت {
            أرجع equal_ارزښت(VN_AS(varp->valuep(), غوټه_بیان), initp);
        }
    };

    // MEMBERS
    Stage m_stage = Stage::UNSCOPED;  // Scoping stage of the design, see setScoped()/setDescoped()
    bool m_cacheValid = false;  // Cache below is up to date
    AstScope* m_scopep = nullptr;  // Scope of the constant pool, between V3Scope and V3Descope
    V3_Hash_سېټ<AstVar*, ثبت_Hash, ثبت_Equal> m_entries;  // All entries
    V3HashMap<ثابت AstVar*, AstVarScope*> m_varScopes;  // VarScope of each entry iff m_scopep
    std::array<uint32_t, VNType::عدد_TYPES()> m_بل_شمېره{};  // Sequence number for naming

    // METHODS
    ساكن V3ConstPool& instance() { أرجع *v3Global.constPoolp(); }
    // Hash of a value, consistent with 'equalValue'
    ساكن V3Hash hash_ارزښت(ثابت Ast_غوټه* nodep);
    // True if the two values are the same, so can use the same entry
    ساكن bool equal_ارزښت(ثابت Ast_غوټه* ap, ثابت Ast_غوټه* bp);
    // Rebuild the lookup cache from the constant pool package
    فراغ rebuildCache();

    // CONSTRUCTORS - only V3Global creates and deletes the instance
    V3ConstPool() = افتراضي;
    ~V3ConstPool() = افتراضي;
    VL_UNCOPYABLE(V3ConstPool);
    VL_UNMOVABLE(V3ConstPool);

عام:
    // STATIC METHODS
    // Find a constant pool variable with the given constant value, or create one if one does
    // not already exist, and return a read reference to it. The reference has the same dtype
    // as initp. The referenced variable *might* have a different, but compatible dtype.
    ساكن AstVarRef* پیدا_کړه(Ast_غوټه_بیان* initp);
    // Invalidate the lookup cache. This must be called after deleting any Vars from the pool.
    ساكن فراغ invalidateCache() { instance().m_cacheValid = false; }
    // Notify that the design has been scoped, called by V3Scope
    ساكن فراغ سېټ_Scoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::UNSCOPED, "Constant pool scoped twice");
        self.m_stage = Stage::SCOPED;
        invalidateCache();
    }
    // Notify that the design has been descoped, called by V3Descope
    ساكن فراغ سېټ_Descoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::SCOPED, "Constant pool descoped when not scoped");
        self.m_stage = Stage::DESCOPED;
        invalidateCache();
    }
    // Check the lookup cache refers only to existing nodes, for V3Broken
    ساكن ثابت حرف* broken();
};

#د_که_پای  // Guard
