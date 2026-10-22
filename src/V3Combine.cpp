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

رده CombineVisitor final : VNVisitor {
    // NODE STATE
    // AstNodeModule::user1()   List of AstCFuncs in this module (via m_cfuncs)
    // AstCFunc::user1()        List of AstCCalls to this function (via m_callSites)
    // AstCFunc::user2()        bool: Already replaced (in 'process')
    // AstCFunc::user3()        bool: Marks functions earlier in iteration order (in 'combinePass')
    // *::user4()               Used by V3Hasher
    ثابت VNUser1InUse m_user1InUse;

    // TYPES
    using funcit_t = std::list<AstCFunc*>::iterator;
    ساختار CFuncs final {
        std::list<AstCFunc*> m_fast;  // Functions that are not slow
        std::list<AstCFunc*> m_slow;  // Functions marked for slow files
    };

    // STATE
    AstUser1Allocator<Ast_گره_Module, CFuncs> m_cfuncs;  // AstCFuncs under module
    AstUser1Allocator<AstCFunc, std::vector<AstCCall*>> m_callSites;  // Call sites of the AstCFunc
    Ast_گره_Module* m_modp = nullptr;  // Current module
    ثابت V3Hasher m_hasher;  // For hashing
    VDouble0 m_cfuncsCombined;  // Statistic tracking

    // METHODS

    تهی removeEmptyFunctions(std::list<AstCFunc*>& funcps) {
        برای (funcit_t it = funcps.begin(), nit; it != funcps.end(); it = nit) {
            AstCFunc* ثابت funcp = *it;
            nit = it;
            ++nit;

            اگر (funcp->emptyBody()) {
                // Delete call sites
                برای (AstCCall* ثابت callp : m_callSites(funcp)) {
                    UASSERT_OBJ(VN_IS(callp->backp(), Stmt_عبارت), callp,
                                "Deleting non-statement call");
                    VL_DO_DANGLING(callp->backp()->unlinkFrBack()->delete_درخت(), callp);
                }
                m_callSites(funcp).clear();
                // Remove from list
                funcps.erase(it);
                // Delete function
                VL_DO_DANGLING(funcp->unlinkFrBack()->delete_درخت(), funcp);
            }
        }
    }

    // One pass of combining. Returns true if did replacement.
    منطقی combinePass(std::list<AstCFunc*>& funcps, V3DupFinder& dupFinder) {
        ثابت VNUser3InUse user3InUse;

        منطقی replaced = نادرست;

        // Replace all identical functions with the first function in the list
        برای (funcit_t it = funcps.begin(), nit; it != funcps.end(); it = nit) {
            AstCFunc* ثابت funcp = *it;
            nit = it;
            ++nit;

            // Remove functions already replaced in the previous iteration
            اگر (funcp->user2()) {
                funcps.erase(it);
                VL_DO_DANGLING(funcp->unlinkFrBack()->delete_درخت(), funcp);
                ادامه;
            }

            وقتی (درست) {
                auto dit = dupFinder.findDuplicate(funcp);
                اگر (dit == dupFinder.end()) بشکن;

                AstCFunc* oldp = VN_AS(dit->second, CFunc);
                AstCFunc* newp = funcp;
                UASSERT_OBJ(!oldp->user2(), oldp, "Should have been removed from dupFinder");

                // Swap them, if the duplicate is earlier in the list of functions. This is
                // necessary because replacing a call site in a later function might have made that
                // function equivalent to an earlier function, but we want the first equivalent
                // function in the list to be the canonical one.
                اگر (oldp->user3()) std::swap(oldp, newp);

                // Something is being replaced
                UINFO(9, "Replacing " << oldp);
                UINFO(9, "     with " << newp);
                ++m_cfuncsCombined;
                replaced = درست;

                // Mark as replaced
                oldp->user2(درست);

                // Redirect the calls
                برای (AstCCall* ثابت callp : m_callSites(oldp)) {
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
                اگر (oldp == funcp) بشکن;
            }

            // Mark as function earlier in list of functions.
            funcp->user3(درست);
        }

        بازگشت replaced;
    }

    // cppcheck-suppress constParameterPointer
    تهی process(AstNetlist* netlistp) {
        // First, remove empty functions. We need to do this separately, because removing
        // calls can change the hashes of the callers.
        برای (Ast_گره_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), گره_Module)) {
            removeEmptyFunctions(m_cfuncs(modulep).m_fast);
            removeEmptyFunctions(m_cfuncs(modulep).m_slow);
        }

        // Combine functions within each module
        برای (Ast_گره_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), گره_Module)) {
            // Put fast functions first, so they are preferred over slow functions
            auto funcps = std::move(m_cfuncs(modulep).m_fast);
            funcps.splice(funcps.end(), m_cfuncs(modulep).m_slow);

            V3DupFinder dupFinder{m_hasher};

            // First, hash all functions
            برای (AstCFunc* ثابت funcp : funcps) dupFinder.درج(funcp);

            // Iterate to fixed point
            {
                ثابت VNUser2InUse user2InUse;
                وقتی (combinePass(funcps, dupFinder)) {}
            }
        }
    }

    // VISITORS
    تهی visit(AstNetlist* nodep) override {
        // Gather functions and references
        iterateChildrenConst(nodep);
        // Combine functions
        process(nodep);
    }
    تهی visit(Ast_گره_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    تهی visit(AstCFunc* nodep) override {
        iterateChildrenConst(nodep);
        اگر (nodep->dontCombine()) بازگشت;
        auto& coll = nodep->slow() ? m_cfuncs(m_modp).m_slow : m_cfuncs(m_modp).m_fast;
        coll.emplace_back(nodep);
    }
    تهی visit(AstCCall* nodep) override {
        iterateChildrenConst(nodep);
        AstCFunc* ثابت funcp = nodep->funcp();
        اگر (funcp->dontCombine()) بازگشت;
        m_callSites(funcp).emplace_back(nodep);
    }

    تهی visit(AstAddrOfCFunc* nodep) override {
        iterateChildrenConst(nodep);
        اگر (nodep->funcp()->dontCombine()) بازگشت;
        // LCOV_EXCL_START
        // We cannot yet handle references via AstAddrOfCFunc, but currently those are
        // only used in tracing functions, which are not combined. Blow up in case this changes.
        nodep->v3fatalSrc(
            "Don't know how to combine functions that are referenced via AstAddrOfCFunc");
        // LCOV_EXCL_STOP
    }

    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CombineVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CombineVisitor() override {
        V3Stats::addStat("Optimizations, Combined CFuncs", m_cfuncsCombined);
    }

عمومی:
    ایستا تهی apply(AstNetlist* netlistp) { CombineVisitor{netlistp}; }
};

//######################################################################
// Combine class functions

تهی V3Combine::combineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CombineVisitor::apply(nodep);
    V3Global::dump_Check_Global_درخت("combine", 0, dump_درخت_Either_Level() >= 3);
}
