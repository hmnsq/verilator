// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Combine common code into functions
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
// V3Combine's Transformations:
//
//      Combine identical CFuncs by retaining only a single copy
//      Also drop empty CFuncs
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Combine.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DupFinder.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <list>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ CombineVisitor final : VNVisitor {
    // NODE STATE
    // AstNodeModule::user1()   List of AstCFuncs in this module (via m_cfuncs)
    // AstCFunc::user1()        List of AstCCalls to this function (via m_callSites)
    // AstCFunc::user2()        bool: Already replaced (in 'process')
    // AstCFunc::user3()        bool: Marks functions earlier in iteration order (in 'combinePass')
    // *::user4()               Used by V3Hasher
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;

    // TYPES
    ਵਰਤੋਂ funcit_t = std::list<AstCFunc*>::iterator;
    ਬਣਤਰ CFuncs final {
        std::list<AstCFunc*> m_fast;  // Functions that are not slow
        std::list<AstCFunc*> m_slow;  // Functions marked for slow files
    };

    // STATE
    AstUser1Allocator<Ast_ਗੰਢ_Module, CFuncs> m_cfuncs;  // AstCFuncs under module
    AstUser1Allocator<AstCFunc, std::ਸਦਿਸ਼<AstCCall*>> m_callSites;  // Call sites of the AstCFunc
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    ਅਚਲ V3Hasher m_hasher;  // For hashing
    VDouble0 m_cfuncsCombined;  // Statistic tracking

    // METHODS

    ਖਾਲੀ remove_ਖਾਲੀ_ਥਾਵਾਂ_Functions(std::list<AstCFunc*>& funcps) {
        ਲਈ (funcit_t it = funcps.begin(), nit; it != funcps.ਸਿਰਾ(); it = nit) {
            AstCFunc* ਅਚਲ funcp = *it;
            nit = it;
            ++nit;

            ਜੇ (funcp->ਖਾਲੀ_ਥਾਵਾਂ_Body()) {
                // Delete call sites
                ਲਈ (AstCCall* ਅਚਲ callp : m_callSites(funcp)) {
                    UASSERT_OBJ(VN_IS(callp->backp(), Stmt_ਵਿਅੰਜਕ), callp,
                                "Deleting non-statement call");
                    VL_DO_DANGLING(callp->backp()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), callp);
                }
                m_callSites(funcp).clear();
                // Remove from list
                funcps.erase(it);
                // Delete function
                VL_DO_DANGLING(funcp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), funcp);
            }
        }
    }

    // One pass of combining. Returns true if did replacement.
    ਤਾਰਕਿਕ combinePass(std::list<AstCFunc*>& funcps, V3DupFinder& dupFinder) {
        ਅਚਲ VNUser3_ਵਿੱਚ_Use user3_ਵਿੱਚ_Use;

        ਤਾਰਕਿਕ replaced = ਝੂਠ;

        // Replace all identical functions with the first function in the list
        ਲਈ (funcit_t it = funcps.begin(), nit; it != funcps.ਸਿਰਾ(); it = nit) {
            AstCFunc* ਅਚਲ funcp = *it;
            nit = it;
            ++nit;

            // Remove functions already replaced in the previous iteration
            ਜੇ (funcp->user2()) {
                funcps.erase(it);
                VL_DO_DANGLING(funcp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), funcp);
                ਜਾਰੀ;
            }

            ਜਦੋਂ (ਸੱਚ) {
                ਸਵੈਚਾਲਿਤ dit = dupFinder.ਲੱਭੋ_Duplicate(funcp);
                ਜੇ (dit == dupFinder.ਸਿਰਾ()) ਰੋਕੋ;

                AstCFunc* oldp = VN_AS(dit->ਦੂਜਾ, CFunc);
                AstCFunc* newp = funcp;
                UASSERT_OBJ(!oldp->user2(), oldp, "Should have been removed from dupFinder");

                // Swap them, if the duplicate is earlier in the list of functions. This is
                // necessary because replacing a call site in a later function might have made that
                // function equivalent to an earlier function, but we want the first equivalent
                // function in the list to be the canonical one.
                ਜੇ (oldp->user3()) std::ਅਦਲਾ_ਬਦਲੀ(oldp, newp);

                // Something is being replaced
                UINFO(9, "Replacing " << oldp);
                UINFO(9, "     with " << newp);
                ++m_cfuncsCombined;
                replaced = ਸੱਚ;

                // Mark as replaced
                oldp->user2(ਸੱਚ);

                // Redirect the calls
                ਲਈ (AstCCall* ਅਚਲ callp : m_callSites(oldp)) {
                    // For sanity check only
                    ਅਚਲ V3Hash oldHash = m_hasher(callp);

                    // Redirect the call
                    callp->funcp(newp);

                    // When redirecting a call to an equivalent function, we do not need to re-hash
                    // the caller, because the hash of the two calls must be the same, and hence
                    // the hash of the caller should not change.
                    UASSERT_OBJ(oldHash == m_hasher.rehash(callp), callp, "Hash changed");
                }

                // Erase the replaced duplicate
                UASSERT_OBJ(dupFinder.erase(oldp) == 1, oldp, "Replaced node not in dupFinder");

                // If we just replaced the function we are iterating (because there was an
                // equivalent earlier in the list), then move on, as this is on longer a candidate
                ਜੇ (oldp == funcp) ਰੋਕੋ;
            }

            // Mark as function earlier in list of functions.
            funcp->user3(ਸੱਚ);
        }

        ਵਾਪਸ replaced;
    }

    // cppcheck-suppress constParameterPointer
    ਖਾਲੀ process(AstNetlist* netlistp) {
        // First, remove empty functions. We need to do this separately, because removing
        // calls can change the hashes of the callers.
        ਲਈ (Ast_ਗੰਢ_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), ਗੰਢ_Module)) {
            remove_ਖਾਲੀ_ਥਾਵਾਂ_Functions(m_cfuncs(modulep).m_fast);
            remove_ਖਾਲੀ_ਥਾਵਾਂ_Functions(m_cfuncs(modulep).m_slow);
        }

        // Combine functions within each module
        ਲਈ (Ast_ਗੰਢ_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), ਗੰਢ_Module)) {
            // Put fast functions first, so they are preferred over slow functions
            ਸਵੈਚਾਲਿਤ funcps = std::move(m_cfuncs(modulep).m_fast);
            funcps.splice(funcps.ਸਿਰਾ(), m_cfuncs(modulep).m_slow);

            V3DupFinder dupFinder{m_hasher};

            // First, hash all functions
            ਲਈ (AstCFunc* ਅਚਲ funcp : funcps) dupFinder.ਪਾਓ(funcp);

            // Iterate to fixed point
            {
                ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;
                ਜਦੋਂ (combinePass(funcps, dupFinder)) {}
            }
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override {
        // Gather functions and references
        iterateChildrenConst(nodep);
        // Combine functions
        process(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        iterateChildrenConst(nodep);
        ਜੇ (nodep->dontCombine()) ਵਾਪਸ;
        ਸਵੈਚਾਲਿਤ& coll = nodep->slow() ? m_cfuncs(m_modp).m_slow : m_cfuncs(m_modp).m_fast;
        coll.emplace_ਪਿੱਠ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCCall* nodep) override {
        iterateChildrenConst(nodep);
        AstCFunc* ਅਚਲ funcp = nodep->funcp();
        ਜੇ (funcp->dontCombine()) ਵਾਪਸ;
        m_callSites(funcp).emplace_ਪਿੱਠ(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Addr_ਦਾ_CFunc* nodep) override {
        iterateChildrenConst(nodep);
        ਜੇ (nodep->funcp()->dontCombine()) ਵਾਪਸ;
        // LCOV_EXCL_START
        // We cannot yet handle references via AstAddrOfCFunc, but currently those are
        // only used in tracing functions, which are not combined. Blow up in case this changes.
        nodep->v3fatalSrc(
            "Don't know how to combine functions that are referenced via AstAddrOfCFunc");
        // LCOV_EXCL_STOP
    }

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ CombineVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CombineVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Combined CFuncs", m_cfuncsCombined);
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(AstNetlist* netlistp) { CombineVisitor{netlistp}; }
};

//######################################################################
// Combine class functions

ਖਾਲੀ V3Combine::combineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CombineVisitor::apply(nodep);
    V3Global::dump_Check_Global_ਰੁੱਖ("combine", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
