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

#నిర్వచితం_కాకపోతే VERILATOR_V3CONSTPOOL_H_
#నిర్వచించు VERILATOR_V3CONSTPOOL_H_
#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3HashTable.h"

//============================================================================

class V3ConstPool final {
    మిత్ర class V3Global;  // Owns the instance, needs constructor access

    // TYPES
    // Scoping stage of the design, which determines how entries are referenced
    గణన class Stage : uint8_t {
        UNSCOPED,  // Before V3Scope: reference via the package
        SCOPED,  // Between V3Scope and V3Descope: reference via the VarScope
        DESCOPED  // After V3Descope: plain reference
    };
    // Hash and equality of entries in 'm_entries', by value
    నిర్మాణం నమోదు_Hash final {
        కొలత_t క్రియాకారకం()(స్థిరరాశి AstVar* varp) స్థిరరాశి {
            తిరిగి (*this)(VN_AS(varp->valuep(), కణుపు_సమాసం));
        }
        కొలత_t క్రియాకారకం()(స్థిరరాశి Ast_కణుపు_సమాసం* initp) స్థిరరాశి { తిరిగి hash_విలువ(initp).విలువ(); }
    };
    నిర్మాణం నమోదు_Equal final {
        bool క్రియాకారకం()(స్థిరరాశి AstVar* ap, స్థిరరాశి AstVar* bp) స్థిరరాశి {  // LCOV_EXCL_START
            తిరిగి (*this)(ap, VN_AS(bp->valuep(), కణుపు_సమాసం));  // Only on collision while rebuild
        }  // LCOV_EXCL_STOP
        bool క్రియాకారకం()(స్థిరరాశి AstVar* varp, స్థిరరాశి Ast_కణుపు_సమాసం* initp) స్థిరరాశి {
            తిరిగి equal_విలువ(VN_AS(varp->valuep(), కణుపు_సమాసం), initp);
        }
    };

    // MEMBERS
    Stage m_stage = Stage::UNSCOPED;  // Scoping stage of the design, see setScoped()/setDescoped()
    bool m_cacheValid = false;  // Cache below is up to date
    AstScope* m_scopep = nullptr;  // Scope of the constant pool, between V3Scope and V3Descope
    V3_Hash_సమితి<AstVar*, నమోదు_Hash, నమోదు_Equal> m_entries;  // All entries
    V3HashMap<స్థిరరాశి AstVar*, AstVarScope*> m_varScopes;  // VarScope of each entry iff m_scopep
    std::array<uint32_t, VNType::అంకె_TYPES()> m_తదుపరి_సంఖ్య{};  // Sequence number for naming

    // METHODS
    స్థిర V3ConstPool& instance() { తిరిగి *v3Global.constPoolp(); }
    // Hash of a value, consistent with 'equalValue'
    స్థిర V3Hash hash_విలువ(స్థిరరాశి Ast_కణుపు* nodep);
    // True if the two values are the same, so can use the same entry
    స్థిర bool equal_విలువ(స్థిరరాశి Ast_కణుపు* ap, స్థిరరాశి Ast_కణుపు* bp);
    // Rebuild the lookup cache from the constant pool package
    శూన్య rebuildCache();

    // CONSTRUCTORS - only V3Global creates and deletes the instance
    V3ConstPool() = అప్రమేయ;
    ~V3ConstPool() = అప్రమేయ;
    VL_UNCOPYABLE(V3ConstPool);
    VL_UNMOVABLE(V3ConstPool);

బహిరంగ:
    // STATIC METHODS
    // Find a constant pool variable with the given constant value, or create one if one does
    // not already exist, and return a read reference to it. The reference has the same dtype
    // as initp. The referenced variable *might* have a different, but compatible dtype.
    స్థిర AstVarRef* కనుగొను(Ast_కణుపు_సమాసం* initp);
    // Invalidate the lookup cache. This must be called after deleting any Vars from the pool.
    స్థిర శూన్య invalidateCache() { instance().m_cacheValid = false; }
    // Notify that the design has been scoped, called by V3Scope
    స్థిర శూన్య సమితి_Scoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::UNSCOPED, "Constant pool scoped twice");
        self.m_stage = Stage::SCOPED;
        invalidateCache();
    }
    // Notify that the design has been descoped, called by V3Descope
    స్థిర శూన్య సమితి_Descoped() {
        V3ConstPool& self = instance();
        UASSERT(self.m_stage == Stage::SCOPED, "Constant pool descoped when not scoped");
        self.m_stage = Stage::DESCOPED;
        invalidateCache();
    }
    // Check the lookup cache refers only to existing nodes, for V3Broken
    స్థిర స్థిరరాశి అక్షరం* broken();
};

#అయితే_ముగింపు  // Guard
