// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Convert BLOCKTEMPs to local variables
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// LOCALIZE TRANSFORMATIONS:
//      All modules:
//          VARSCOPE(BLOCKTEMP...
//             if only referenced in one CFUNC, make it local
//          VARSCOPE
//             if non-public, always written before used, make it local
//
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Localize.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LocalizeVisitor

ਵਰਗ LocalizeVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  AstVarScope::user1()    ->  bool.  VarScope is not optimizable
    //  AstCFunc::user1()       ->  bool.  CFunc is not a leaf function
    //  AstVarScope::user2()    ->  bool.  VarScope was fully assigned in current function
    //  AstVarScope::user3p()   ->  Set of CFuncs referencing this VarScope. (via m_accessors)
    //  AstCFunc::user4p()      ->  Multimap of 'VarScope -> VarRefs that reference that VarScope'
    //                              in this function. (via m_references)
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;
    ਅਚਲ VNUser3_ਵਿੱਚ_Use m_user3_ਵਿੱਚ_Use;
    ਅਚਲ VNUser4_ਵਿੱਚ_Use m_user4_ਵਿੱਚ_Use;

    AstUser3Allocator<AstVarScope, std::unordered_ਸੈੱਟ<AstCFunc*>> m_accessors;
    AstUser4Allocator<AstCFunc, std::unordered_multimap<ਅਚਲ AstVarScope*, AstVarRef*>>
        m_references;

    // STATE - across all visitors
    std::ਸਦਿਸ਼<AstVarScope*> m_varScopeps;  // List of variables to consider for localization
    VDouble0 m_statLocVars;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = ਖਾਲੀ_ਸੂਚਕ;  // Current active function
    uint32_t m_ਗੰਢ_ਡੂੰਘਾਈ = 0;  // Node depth under m_cfuncp

    // METHODS
    ਤਾਰਕਿਕ isOptimizable(AstVarScope* nodep) {
        // Don't want to malloc/free the backing store all the time
        ਜੇ (VN_IS(nodep->dtypep(), NBACommit_ਕਤਾਰ_DType)) ਵਾਪਸ ਝੂਠ;
        // Do not localize strings. They result in unnecessary initialization
        // and bloated code size due to destructor calls when unused.
        // TODO: Local variables should be pushed into the narrowest scope rather
        // than emitted at the top of the function. See discussion in #6969.
        ਜੇ (nodep->dtypep()->skipRefp()->isString()) ਵਾਪਸ ਝੂਠ;
        // Variables used in super constructor call can't be localized, because
        // in C++ there is no way to declare them before base class constructor call
        ਵਾਪਸ ((!nodep->user1()  // Not marked as not optimizable, or ...
                                  // .. a block temp used in a single CFunc
                 || (nodep->varp()->varType() == VVarType::BLOCKTEMP
                     && m_accessors(nodep).ਮਾਪ() == 1))
                // and under size limit
                && nodep->varp()->dtypep()->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() <= v3Global.opt.localize_Max_ਮਾਪ());
    }

    ਸਥਿਰ ਤਾਰਕਿਕ exists_Non_ਪੱਤੀ(ਅਚਲ std::unordered_ਸੈੱਟ<AstCFunc*>& funcps) {
        ਲਈ (ਅਚਲ AstCFunc* ਅਚਲ funcp : funcps) {
            ਜੇ (funcp->user1()) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    ਖਾਲੀ moveVarScopes() {
        ਲਈ (AstVarScope* ਅਚਲ nodep : m_varScopeps) {
            ਜੇ (!isOptimizable(nodep)) ਜਾਰੀ;  // Not optimizable

            ਅਚਲ std::unordered_ਸੈੱਟ<AstCFunc*>& funcps = m_accessors(nodep);
            ਜੇ (funcps.ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;  // No referencing functions at all

            // If more than one referencing function, but not all are leaf
            // functions, then don't localize, as one of the referencing
            // functions might be calling another, which the current analysis
            // cannot cope with. This should be rare (introduced by V3Depth).
            ਜੇ (funcps.ਮਾਪ() > 1 && exists_Non_ਪੱਤੀ(funcps)) ਜਾਰੀ;

            UINFO(4, "Localizing " << nodep);
            ++m_statLocVars;

            // Yank the VarScope from it's parent and schedule them for deletion. Leave the Var
            // for now, as not all VarScopes referencing this Var might be localized.
            ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ());  // TODO nodep used past here

            // In each referencing function, create a replacement local variable
            AstVar* ਅਚਲ oldVarp = nodep->varp();
            ਲਈ (AstCFunc* ਅਚਲ funcp : funcps) {
                // Create the new local variable.
                ਅਚਲ string newName
                    = nodep->scopep() == funcp->scopep()
                          ? oldVarp->name()
                          : nodep->scopep()->nameDotless() + "__DOT__" + oldVarp->name();
                AstVar* ਅਚਲ newVarp
                    = ਨਵਾਂ AstVar{oldVarp->fileline(), oldVarp->varType(), newName, oldVarp};
                newVarp->funcLocal(ਸੱਚ);
                newVarp->noReset(oldVarp->noReset());
                newVarp->noSubst(oldVarp->noSubst());
                funcp->ਜੋੜੋ_Varsp(newVarp);

                // Fix up all the references within this function
                ਅਚਲ ਸਵੈਚਾਲਿਤ er = m_references(funcp).equal_range(nodep);
                ਲਈ (ਸਵੈਚਾਲਿਤ it = er.ਪਹਿਲਾ; it != er.ਦੂਜਾ; ++it) {
                    AstVarRef* ਅਚਲ refp = it->ਦੂਜਾ;
                    refp->varScopep(ਖਾਲੀ_ਸੂਚਕ);
                    refp->varp(newVarp);
                }
            }
        }
        m_varScopeps.clear();
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override {
        iterateChildrenConst(nodep);
        moveVarScopes();
    }

    ਖਾਲੀ ਦੇਖੋ(AstCAwait* nodep) override {
        m_cfuncp->user1(ਸੱਚ);  // Mark caller as not a leaf function
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        UINFO(4, "  CFUNC " << nodep);
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_ਗੰਢ_ਡੂੰਘਾਈ);
        m_cfuncp = nodep;
        m_ਗੰਢ_ਡੂੰਘਾਈ = 0;
        ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCNew* nodep) override {
        m_cfuncp->user1(ਸੱਚ);  // Mark caller as not a leaf function
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCCall* nodep) override {
        m_cfuncp->user1(ਸੱਚ);  // Mark caller as not a leaf function
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        // Analyze RHS first so "a = a + 1" is detected as a read before write
        iterate(nodep->rhsp());
        // For now we only consider an assignment that is directly under the function, (in
        // particular: not under an AstIf, or other kind of branch). This could be improved with
        // proper data flow analysis.
        ਜੇ (m_ਗੰਢ_ਡੂੰਘਾਈ == 0) {
            // Check if simple "VARREF = ..." assignment, i.e.: this assignment sets the whole
            // variable (and in particular, it is not assigned only in part).
            ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(nodep->lhsp(), VarRef)) {
                // Mark this VarScope as assigned in this function
                refp->varScopep()->user2(1);
            }
        }
        // Analyze LHS (in case it's not the above simple case
        iterate(nodep->lhsp());
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarScope* nodep) override {
        ਜੇ (!nodep->varp()->isPrimaryIO()  // Not an IO the user wants to interact with
            && !nodep->varp()->is_Sig_ਜਨਤਕ()  // Not something the user wants to interact with
            && !nodep->varp()->isFuncLocal()  // Not already a function local (e.g.: argument)
            && !nodep->varp()->isClassMember()  // Statically exists in design hierarchy
            && !nodep->varp()->sensIfacep()  // Not sensitive to an interface
            && !nodep->varp()->isVirtIface()  // Not interface pointer
            && !nodep->varp()->const_Pool_ਇੰਦਰਾਜ()  // Not a const pool entry
            && !nodep->varp()->valuep()  // Does not have an initializer
        ) {
            UINFO(4, "Consider for localization: " << nodep);
            m_varScopeps.ਧੱਕੋ_ਪਿੱਠ(nodep);
        }
        // No iterate; Don't want varrefs under it (e.g.: in child dtype?)
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        UASSERT_OBJ(m_cfuncp, nodep, "AstVarRef not under function");

        AstVarScope* ਅਚਲ varScopep = nodep->varScopep();
        // Remember this function accesses this VarScope (we always need this as we might optimize
        // this VarScope into a local, even if it's not assigned. See 'isOptimizable')
        m_accessors(varScopep).ਪਾਓ(m_cfuncp);  // emplace performs a temporary malloc
        // Remember the reference so we can fix it up later (we always need this as well)
        m_references(m_cfuncp).emplace(varScopep, nodep);

        ਜੇ (!varScopep->user1()) {  // Check if already marked as not optimizable
            // Note: we only check read variables, as it's ok to localize (and in fact discard)
            // any variables that are only written but never read.
            ਜੇ (nodep->access().isReadOrRW() && !varScopep->user2()) {
                // Variable is read, but is not known to have been assigned in this function. Mark
                // as not optimizable.
                UINFO(4, "Not optimizable (not written): " << nodep);
                varScopep->user1(1);
            }
        }
        // No iterate; Don't want varrefs under it  (e.g.: in child dtype?)
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        VL_RESTORER(m_ਗੰਢ_ਡੂੰਘਾਈ);
        ++m_ਗੰਢ_ਡੂੰਘਾਈ;
        iterateChildrenConst(nodep);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ LocalizeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LocalizeVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Vars localized", m_statLocVars);
    }
};

//######################################################################
// Localize class functions

ਖਾਲੀ V3Localize::localizeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LocalizeVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("localize", 0, dump_ਰੁੱਖ_Either_Level() >= 6);
}
