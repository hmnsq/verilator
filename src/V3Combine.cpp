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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Combine.h"

#অন্তর্ভাব "V3AstUserAllocator.h"
#অন্তর্ভাব "V3DupFinder.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <list>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

বর্গ CombineVisitor final : VNVisitor {
    // NODE STATE
    // AstNodeModule::user1()   List of AstCFuncs in this module (via m_cfuncs)
    // AstCFunc::user1()        List of AstCCalls to this function (via m_callSites)
    // AstCFunc::user2()        bool: Already replaced (in 'process')
    // AstCFunc::user3()        bool: Marks functions earlier in iteration order (in 'combinePass')
    // *::user4()               Used by V3Hasher
    ধ্রুব VNUser1InUse m_user1InUse;

    // TYPES
    using funcit_t = std::list<AstCFunc*>::iterator;
    সংরচনা CFuncs final {
        std::list<AstCFunc*> m_fast;  // Functions that are not slow
        std::list<AstCFunc*> m_slow;  // Functions marked for slow files
    };

    // STATE
    AstUser1Allocator<Ast_গ্রন্থি_Module, CFuncs> m_cfuncs;  // AstCFuncs under module
    AstUser1Allocator<AstCFunc, std::vector<AstCCall*>> m_callSites;  // Call sites of the AstCFunc
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    ধ্রুব V3Hasher m_hasher;  // For hashing
    VDouble0 m_cfuncsCombined;  // Statistic tracking

    // METHODS

    শূন্য removeEmptyFunctions(std::list<AstCFunc*>& funcps) {
        প্রতি (funcit_t it = funcps.begin(), nit; it != funcps.end(); it = nit) {
            AstCFunc* ধ্রুব funcp = *it;
            nit = it;
            ++nit;

            যদি (funcp->emptyBody()) {
                // Delete call sites
                প্রতি (AstCCall* ধ্রুব callp : m_callSites(funcp)) {
                    UASSERT_OBJ(VN_IS(callp->backp(), Stmt_রাশিমালা), callp,
                                "Deleting non-statement call");
                    VL_DO_DANGLING(callp->backp()->unlinkFrBack()->delete_বৃক্ষ(), callp);
                }
                m_callSites(funcp).clear();
                // Remove from list
                funcps.erase(it);
                // Delete function
                VL_DO_DANGLING(funcp->unlinkFrBack()->delete_বৃক্ষ(), funcp);
            }
        }
    }

    // One pass of combining. Returns true if did replacement.
    তার্কিক combinePass(std::list<AstCFunc*>& funcps, V3DupFinder& dupFinder) {
        ধ্রুব VNUser3InUse user3InUse;

        তার্কিক replaced = অসত্য;

        // Replace all identical functions with the first function in the list
        প্রতি (funcit_t it = funcps.begin(), nit; it != funcps.end(); it = nit) {
            AstCFunc* ধ্রুব funcp = *it;
            nit = it;
            ++nit;

            // Remove functions already replaced in the previous iteration
            যদি (funcp->user2()) {
                funcps.erase(it);
                VL_DO_DANGLING(funcp->unlinkFrBack()->delete_বৃক্ষ(), funcp);
                অনুবর্ত;
            }

            যাবত্ (সত্য) {
                auto dit = dupFinder.findDuplicate(funcp);
                যদি (dit == dupFinder.end()) ভঙ্গ;

                AstCFunc* oldp = VN_AS(dit->second, CFunc);
                AstCFunc* newp = funcp;
                UASSERT_OBJ(!oldp->user2(), oldp, "Should have been removed from dupFinder");

                // Swap them, if the duplicate is earlier in the list of functions. This is
                // necessary because replacing a call site in a later function might have made that
                // function equivalent to an earlier function, but we want the first equivalent
                // function in the list to be the canonical one.
                যদি (oldp->user3()) std::swap(oldp, newp);

                // Something is being replaced
                UINFO(9, "Replacing " << oldp);
                UINFO(9, "     with " << newp);
                ++m_cfuncsCombined;
                replaced = সত্য;

                // Mark as replaced
                oldp->user2(সত্য);

                // Redirect the calls
                প্রতি (AstCCall* ধ্রুব callp : m_callSites(oldp)) {
                    // For sanity check only
                    ধ্রুব V3Hash oldHash = m_hasher(callp);

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
                যদি (oldp == funcp) ভঙ্গ;
            }

            // Mark as function earlier in list of functions.
            funcp->user3(সত্য);
        }

        প্রত্যাগম replaced;
    }

    // cppcheck-suppress constParameterPointer
    শূন্য process(AstNetlist* netlistp) {
        // First, remove empty functions. We need to do this separately, because removing
        // calls can change the hashes of the callers.
        প্রতি (Ast_গ্রন্থি_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), গ্রন্থি_Module)) {
            removeEmptyFunctions(m_cfuncs(modulep).m_fast);
            removeEmptyFunctions(m_cfuncs(modulep).m_slow);
        }

        // Combine functions within each module
        প্রতি (Ast_গ্রন্থি_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), গ্রন্থি_Module)) {
            // Put fast functions first, so they are preferred over slow functions
            auto funcps = std::move(m_cfuncs(modulep).m_fast);
            funcps.splice(funcps.end(), m_cfuncs(modulep).m_slow);

            V3DupFinder dupFinder{m_hasher};

            // First, hash all functions
            প্রতি (AstCFunc* ধ্রুব funcp : funcps) dupFinder.ঢোকাও(funcp);

            // Iterate to fixed point
            {
                ধ্রুব VNUser2InUse user2InUse;
                যাবত্ (combinePass(funcps, dupFinder)) {}
            }
        }
    }

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override {
        // Gather functions and references
        iterateChildrenConst(nodep);
        // Combine functions
        process(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstCFunc* nodep) override {
        iterateChildrenConst(nodep);
        যদি (nodep->dontCombine()) প্রত্যাগম;
        auto& coll = nodep->slow() ? m_cfuncs(m_modp).m_slow : m_cfuncs(m_modp).m_fast;
        coll.emplace_back(nodep);
    }
    শূন্য visit(AstCCall* nodep) override {
        iterateChildrenConst(nodep);
        AstCFunc* ধ্রুব funcp = nodep->funcp();
        যদি (funcp->dontCombine()) প্রত্যাগম;
        m_callSites(funcp).emplace_back(nodep);
    }

    শূন্য visit(AstAddrOfCFunc* nodep) override {
        iterateChildrenConst(nodep);
        যদি (nodep->funcp()->dontCombine()) প্রত্যাগম;
        // LCOV_EXCL_START
        // We cannot yet handle references via AstAddrOfCFunc, but currently those are
        // only used in tracing functions, which are not combined. Blow up in case this changes.
        nodep->v3fatalSrc(
            "Don't know how to combine functions that are referenced via AstAddrOfCFunc");
        // LCOV_EXCL_STOP
    }

    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CombineVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CombineVisitor() override {
        V3Stats::addStat("Optimizations, Combined CFuncs", m_cfuncsCombined);
    }

সার্বজনিক:
    স্থির শূন্য apply(AstNetlist* netlistp) { CombineVisitor{netlistp}; }
};

//######################################################################
// Combine class functions

শূন্য V3Combine::combineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CombineVisitor::apply(nodep);
    V3Global::dump_Check_Global_বৃক্ষ("combine", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
