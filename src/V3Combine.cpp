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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Combine.h"

#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3DupFinder.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <list>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ଶ୍ରେଣୀ CombineVisitor final : VNVisitor {
    // NODE STATE
    // AstNodeModule::user1()   List of AstCFuncs in this module (via m_cfuncs)
    // AstCFunc::user1()        List of AstCCalls to this function (via m_callSites)
    // AstCFunc::user2()        bool: Already replaced (in 'process')
    // AstCFunc::user3()        bool: Marks functions earlier in iteration order (in 'combinePass')
    // *::user4()               Used by V3Hasher
    ଅଚଳ VNUser1InUse m_user1InUse;

    // TYPES
    using funcit_t = std::list<AstCFunc*>::iterator;
    ସଂରଚନା CFuncs final {
        std::list<AstCFunc*> m_fast;  // Functions that are not slow
        std::list<AstCFunc*> m_slow;  // Functions marked for slow files
    };

    // STATE
    AstUser1Allocator<Ast_ଗଣ୍ଠି_Module, CFuncs> m_cfuncs;  // AstCFuncs under module
    AstUser1Allocator<AstCFunc, std::vector<AstCCall*>> m_callSites;  // Call sites of the AstCFunc
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    ଅଚଳ V3Hasher m_hasher;  // For hashing
    VDouble0 m_cfuncsCombined;  // Statistic tracking

    // METHODS

    ଶୂନ୍ୟ removeEmptyFunctions(std::list<AstCFunc*>& funcps) {
        ପାଇଁ (funcit_t it = funcps.begin(), nit; it != funcps.end(); it = nit) {
            AstCFunc* ଅଚଳ funcp = *it;
            nit = it;
            ++nit;

            ଯଦି (funcp->emptyBody()) {
                // Delete call sites
                ପାଇଁ (AstCCall* ଅଚଳ callp : m_callSites(funcp)) {
                    UASSERT_OBJ(VN_IS(callp->backp(), Stmt_ପଦସମଷ୍ଟି), callp,
                                "Deleting non-statement call");
                    VL_DO_DANGLING(callp->backp()->unlinkFrBack()->delete_ବୃକ୍ଷ(), callp);
                }
                m_callSites(funcp).clear();
                // Remove from list
                funcps.erase(it);
                // Delete function
                VL_DO_DANGLING(funcp->unlinkFrBack()->delete_ବୃକ୍ଷ(), funcp);
            }
        }
    }

    // One pass of combining. Returns true if did replacement.
    ତାର୍କିକ combinePass(std::list<AstCFunc*>& funcps, V3DupFinder& dupFinder) {
        ଅଚଳ VNUser3InUse user3InUse;

        ତାର୍କିକ replaced = ମିଥ୍ୟା;

        // Replace all identical functions with the first function in the list
        ପାଇଁ (funcit_t it = funcps.begin(), nit; it != funcps.end(); it = nit) {
            AstCFunc* ଅଚଳ funcp = *it;
            nit = it;
            ++nit;

            // Remove functions already replaced in the previous iteration
            ଯଦି (funcp->user2()) {
                funcps.erase(it);
                VL_DO_DANGLING(funcp->unlinkFrBack()->delete_ବୃକ୍ଷ(), funcp);
                ଜାରି;
            }

            ଯେତେବେଳେ (ସତ୍ୟ) {
                auto dit = dupFinder.findDuplicate(funcp);
                ଯଦି (dit == dupFinder.end()) ଭାଙ୍ଗ;

                AstCFunc* oldp = VN_AS(dit->second, CFunc);
                AstCFunc* newp = funcp;
                UASSERT_OBJ(!oldp->user2(), oldp, "Should have been removed from dupFinder");

                // Swap them, if the duplicate is earlier in the list of functions. This is
                // necessary because replacing a call site in a later function might have made that
                // function equivalent to an earlier function, but we want the first equivalent
                // function in the list to be the canonical one.
                ଯଦି (oldp->user3()) std::swap(oldp, newp);

                // Something is being replaced
                UINFO(9, "Replacing " << oldp);
                UINFO(9, "     with " << newp);
                ++m_cfuncsCombined;
                replaced = ସତ୍ୟ;

                // Mark as replaced
                oldp->user2(ସତ୍ୟ);

                // Redirect the calls
                ପାଇଁ (AstCCall* ଅଚଳ callp : m_callSites(oldp)) {
                    // For sanity check only
                    ଅଚଳ V3Hash oldHash = m_hasher(callp);

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
                ଯଦି (oldp == funcp) ଭାଙ୍ଗ;
            }

            // Mark as function earlier in list of functions.
            funcp->user3(ସତ୍ୟ);
        }

        ଫେରନ୍ତୁ replaced;
    }

    // cppcheck-suppress constParameterPointer
    ଶୂନ୍ୟ process(AstNetlist* netlistp) {
        // First, remove empty functions. We need to do this separately, because removing
        // calls can change the hashes of the callers.
        ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), ଗଣ୍ଠି_Module)) {
            removeEmptyFunctions(m_cfuncs(modulep).m_fast);
            removeEmptyFunctions(m_cfuncs(modulep).m_slow);
        }

        // Combine functions within each module
        ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), ଗଣ୍ଠି_Module)) {
            // Put fast functions first, so they are preferred over slow functions
            auto funcps = std::move(m_cfuncs(modulep).m_fast);
            funcps.splice(funcps.end(), m_cfuncs(modulep).m_slow);

            V3DupFinder dupFinder{m_hasher};

            // First, hash all functions
            ପାଇଁ (AstCFunc* ଅଚଳ funcp : funcps) dupFinder.ଭର୍ତ୍ତି(funcp);

            // Iterate to fixed point
            {
                ଅଚଳ VNUser2InUse user2InUse;
                ଯେତେବେଳେ (combinePass(funcps, dupFinder)) {}
            }
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override {
        // Gather functions and references
        iterateChildrenConst(nodep);
        // Combine functions
        process(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        iterateChildrenConst(nodep);
        ଯଦି (nodep->dontCombine()) ଫେରନ୍ତୁ;
        auto& coll = nodep->slow() ? m_cfuncs(m_modp).m_slow : m_cfuncs(m_modp).m_fast;
        coll.emplace_back(nodep);
    }
    ଶୂନ୍ୟ visit(AstCCall* nodep) override {
        iterateChildrenConst(nodep);
        AstCFunc* ଅଚଳ funcp = nodep->funcp();
        ଯଦି (funcp->dontCombine()) ଫେରନ୍ତୁ;
        m_callSites(funcp).emplace_back(nodep);
    }

    ଶୂନ୍ୟ visit(AstAddrOfCFunc* nodep) override {
        iterateChildrenConst(nodep);
        ଯଦି (nodep->funcp()->dontCombine()) ଫେରନ୍ତୁ;
        // LCOV_EXCL_START
        // We cannot yet handle references via AstAddrOfCFunc, but currently those are
        // only used in tracing functions, which are not combined. Blow up in case this changes.
        nodep->v3fatalSrc(
            "Don't know how to combine functions that are referenced via AstAddrOfCFunc");
        // LCOV_EXCL_STOP
    }

    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CombineVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CombineVisitor() override {
        V3Stats::addStat("Optimizations, Combined CFuncs", m_cfuncsCombined);
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(AstNetlist* netlistp) { CombineVisitor{netlistp}; }
};

//######################################################################
// Combine class functions

ଶୂନ୍ୟ V3Combine::combineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CombineVisitor::apply(nodep);
    V3Global::dump_Check_Global_ବୃକ୍ଷ("combine", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
