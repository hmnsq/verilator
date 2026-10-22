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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Combine.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3DupFinder.h"
#شامل "V3Stats.h"

#شامل <list>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

جماعت CombineVisitor final : VNVisitor {
    // NODE STATE
    // AstNodeModule::user1()   List of AstCFuncs in this module (via m_cfuncs)
    // AstCFunc::user1()        List of AstCCalls to this function (via m_callSites)
    // AstCFunc::user2()        bool: Already replaced (in 'process')
    // AstCFunc::user3()        bool: Marks functions earlier in iteration order (in 'combinePass')
    // *::user4()               Used by V3Hasher
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;

    // TYPES
    استعمال funcit_t = std::list<AstCFunc*>::iterator;
    ساخت CFuncs final {
        std::list<AstCFunc*> m_fast;  // Functions that are not slow
        std::list<AstCFunc*> m_slow;  // Functions marked for slow files
    };

    // STATE
    AstUser1Allocator<Ast_گرہ_Module, CFuncs> m_cfuncs;  // AstCFuncs under module
    AstUser1Allocator<AstCFunc, std::سمتیہ<AstCCall*>> m_callSites;  // Call sites of the AstCFunc
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    ثابت V3Hasher m_hasher;  // For hashing
    VDouble0 m_cfuncsCombined;  // Statistic tracking

    // METHODS

    خالی remove_خالی_خانے_Functions(std::list<AstCFunc*>& funcps) {
        برائے (funcit_t it = funcps.begin(), nit; it != funcps.انتہا(); it = nit) {
            AstCFunc* ثابت funcp = *it;
            nit = it;
            ++nit;

            اگر (funcp->خالی_خانے_Body()) {
                // Delete call sites
                برائے (AstCCall* ثابت callp : m_callSites(funcp)) {
                    UASSERT_OBJ(VN_IS(callp->backp(), Stmt_اظہاریہ), callp,
                                "Deleting non-statement call");
                    VL_DO_DANGLING(callp->backp()->unlink_Fr_عقب()->delete_درخت(), callp);
                }
                m_callSites(funcp).clear();
                // Remove from list
                funcps.erase(it);
                // Delete function
                VL_DO_DANGLING(funcp->unlink_Fr_عقب()->delete_درخت(), funcp);
            }
        }
    }

    // One pass of combining. Returns true if did replacement.
    منطقی combinePass(std::list<AstCFunc*>& funcps, V3DupFinder& dupFinder) {
        ثابت VNUser3_اندر_Use user3_اندر_Use;

        منطقی replaced = جھوٹ;

        // Replace all identical functions with the first function in the list
        برائے (funcit_t it = funcps.begin(), nit; it != funcps.انتہا(); it = nit) {
            AstCFunc* ثابت funcp = *it;
            nit = it;
            ++nit;

            // Remove functions already replaced in the previous iteration
            اگر (funcp->user2()) {
                funcps.erase(it);
                VL_DO_DANGLING(funcp->unlink_Fr_عقب()->delete_درخت(), funcp);
                جاری;
            }

            جب (سچ) {
                خودکار dit = dupFinder.ڈھونڈو_Duplicate(funcp);
                اگر (dit == dupFinder.انتہا()) توڑو;

                AstCFunc* oldp = VN_AS(dit->دوسرا, CFunc);
                AstCFunc* newp = funcp;
                UASSERT_OBJ(!oldp->user2(), oldp, "Should have been removed from dupFinder");

                // Swap them, if the duplicate is earlier in the list of functions. This is
                // necessary because replacing a call site in a later function might have made that
                // function equivalent to an earlier function, but we want the first equivalent
                // function in the list to be the canonical one.
                اگر (oldp->user3()) std::تبادلہ(oldp, newp);

                // Something is being replaced
                UINFO(9, "Replacing " << oldp);
                UINFO(9, "     with " << newp);
                ++m_cfuncsCombined;
                replaced = سچ;

                // Mark as replaced
                oldp->user2(سچ);

                // Redirect the calls
                برائے (AstCCall* ثابت callp : m_callSites(oldp)) {
                    // For sanity check only
                    ثابت V3Hash oldHash = m_hasher(callp);

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
                اگر (oldp == funcp) توڑو;
            }

            // Mark as function earlier in list of functions.
            funcp->user3(سچ);
        }

        واپس replaced;
    }

    // cppcheck-suppress constParameterPointer
    خالی process(AstNetlist* netlistp) {
        // First, remove empty functions. We need to do this separately, because removing
        // calls can change the hashes of the callers.
        برائے (Ast_گرہ_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), گرہ_Module)) {
            remove_خالی_خانے_Functions(m_cfuncs(modulep).m_fast);
            remove_خالی_خانے_Functions(m_cfuncs(modulep).m_slow);
        }

        // Combine functions within each module
        برائے (Ast_گرہ_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), گرہ_Module)) {
            // Put fast functions first, so they are preferred over slow functions
            خودکار funcps = std::move(m_cfuncs(modulep).m_fast);
            funcps.splice(funcps.انتہا(), m_cfuncs(modulep).m_slow);

            V3DupFinder dupFinder{m_hasher};

            // First, hash all functions
            برائے (AstCFunc* ثابت funcp : funcps) dupFinder.داخل_کرو(funcp);

            // Iterate to fixed point
            {
                ثابت VNUser2_اندر_Use user2_اندر_Use;
                جب (combinePass(funcps, dupFinder)) {}
            }
        }
    }

    // VISITORS
    خالی دورہ(AstNetlist* nodep) override {
        // Gather functions and references
        iterateChildrenConst(nodep);
        // Combine functions
        process(nodep);
    }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstCFunc* nodep) override {
        iterateChildrenConst(nodep);
        اگر (nodep->dontCombine()) واپس;
        خودکار& coll = nodep->slow() ? m_cfuncs(m_modp).m_slow : m_cfuncs(m_modp).m_fast;
        coll.emplace_عقب(nodep);
    }
    خالی دورہ(AstCCall* nodep) override {
        iterateChildrenConst(nodep);
        AstCFunc* ثابت funcp = nodep->funcp();
        اگر (funcp->dontCombine()) واپس;
        m_callSites(funcp).emplace_عقب(nodep);
    }

    خالی دورہ(Ast_Addr_کا_CFunc* nodep) override {
        iterateChildrenConst(nodep);
        اگر (nodep->funcp()->dontCombine()) واپس;
        // LCOV_EXCL_START
        // We cannot yet handle references via AstAddrOfCFunc, but currently those are
        // only used in tracing functions, which are not combined. Blow up in case this changes.
        nodep->v3fatalSrc(
            "Don't know how to combine functions that are referenced via AstAddrOfCFunc");
        // LCOV_EXCL_STOP
    }

    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    صریح CombineVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CombineVisitor() override {
        V3Stats::جوڑو_Stat("Optimizations, Combined CFuncs", m_cfuncsCombined);
    }

عوامی:
    ساکن خالی apply(AstNetlist* netlistp) { CombineVisitor{netlistp}; }
};

//######################################################################
// Combine class functions

خالی V3Combine::combineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CombineVisitor::apply(nodep);
    V3Global::dump_Check_Global_درخت("combine", 0, dump_درخت_Either_Level() >= 3);
}
