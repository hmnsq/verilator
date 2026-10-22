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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Combine.h"

#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3DupFinder.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <list>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి CombineVisitor final : VNVisitor {
    // NODE STATE
    // AstNodeModule::user1()   List of AstCFuncs in this module (via m_cfuncs)
    // AstCFunc::user1()        List of AstCCalls to this function (via m_callSites)
    // AstCFunc::user2()        bool: Already replaced (in 'process')
    // AstCFunc::user3()        bool: Marks functions earlier in iteration order (in 'combinePass')
    // *::user4()               Used by V3Hasher
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;

    // TYPES
    ఉపయోగించి funcit_t = std::list<AstCFunc*>::iterator;
    నిర్మాణం CFuncs final {
        std::list<AstCFunc*> m_fast;  // Functions that are not slow
        std::list<AstCFunc*> m_slow;  // Functions marked for slow files
    };

    // STATE
    AstUser1Allocator<Ast_కణుపు_Module, CFuncs> m_cfuncs;  // AstCFuncs under module
    AstUser1Allocator<AstCFunc, std::సదిశ<AstCCall*>> m_callSites;  // Call sites of the AstCFunc
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    స్థిరరాశి V3Hasher m_hasher;  // For hashing
    VDouble0 m_cfuncsCombined;  // Statistic tracking

    // METHODS

    శూన్య remove_ఖాళీలు_Functions(std::list<AstCFunc*>& funcps) {
        కొరకు (funcit_t it = funcps.begin(), nit; it != funcps.చివర(); it = nit) {
            AstCFunc* స్థిరరాశి funcp = *it;
            nit = it;
            ++nit;

            ఒకవేళ (funcp->ఖాళీలు_Body()) {
                // Delete call sites
                కొరకు (AstCCall* స్థిరరాశి callp : m_callSites(funcp)) {
                    UASSERT_OBJ(VN_IS(callp->backp(), Stmt_సమాసం), callp,
                                "Deleting non-statement call");
                    VL_DO_DANGLING(callp->backp()->unlink_Fr_వెనుక()->delete_వృక్షం(), callp);
                }
                m_callSites(funcp).clear();
                // Remove from list
                funcps.erase(it);
                // Delete function
                VL_DO_DANGLING(funcp->unlink_Fr_వెనుక()->delete_వృక్షం(), funcp);
            }
        }
    }

    // One pass of combining. Returns true if did replacement.
    తార్కిక combinePass(std::list<AstCFunc*>& funcps, V3DupFinder& dupFinder) {
        స్థిరరాశి VNUser3_లోపల_Use user3_లోపల_Use;

        తార్కిక replaced = అబద్ధం;

        // Replace all identical functions with the first function in the list
        కొరకు (funcit_t it = funcps.begin(), nit; it != funcps.చివర(); it = nit) {
            AstCFunc* స్థిరరాశి funcp = *it;
            nit = it;
            ++nit;

            // Remove functions already replaced in the previous iteration
            ఒకవేళ (funcp->user2()) {
                funcps.erase(it);
                VL_DO_DANGLING(funcp->unlink_Fr_వెనుక()->delete_వృక్షం(), funcp);
                కొనసాగు;
            }

            అయితే (నిజం) {
                స్వయంచాలక dit = dupFinder.కనుగొను_Duplicate(funcp);
                ఒకవేళ (dit == dupFinder.చివర()) ఆపు;

                AstCFunc* oldp = VN_AS(dit->రెండవ, CFunc);
                AstCFunc* newp = funcp;
                UASSERT_OBJ(!oldp->user2(), oldp, "Should have been removed from dupFinder");

                // Swap them, if the duplicate is earlier in the list of functions. This is
                // necessary because replacing a call site in a later function might have made that
                // function equivalent to an earlier function, but we want the first equivalent
                // function in the list to be the canonical one.
                ఒకవేళ (oldp->user3()) std::మార్పిడి(oldp, newp);

                // Something is being replaced
                UINFO(9, "Replacing " << oldp);
                UINFO(9, "     with " << newp);
                ++m_cfuncsCombined;
                replaced = నిజం;

                // Mark as replaced
                oldp->user2(నిజం);

                // Redirect the calls
                కొరకు (AstCCall* స్థిరరాశి callp : m_callSites(oldp)) {
                    // For sanity check only
                    స్థిరరాశి V3Hash oldHash = m_hasher(callp);

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
                ఒకవేళ (oldp == funcp) ఆపు;
            }

            // Mark as function earlier in list of functions.
            funcp->user3(నిజం);
        }

        తిరిగి replaced;
    }

    // cppcheck-suppress constParameterPointer
    శూన్య process(AstNetlist* netlistp) {
        // First, remove empty functions. We need to do this separately, because removing
        // calls can change the hashes of the callers.
        కొరకు (Ast_కణుపు_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), కణుపు_Module)) {
            remove_ఖాళీలు_Functions(m_cfuncs(modulep).m_fast);
            remove_ఖాళీలు_Functions(m_cfuncs(modulep).m_slow);
        }

        // Combine functions within each module
        కొరకు (Ast_కణుపు_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), కణుపు_Module)) {
            // Put fast functions first, so they are preferred over slow functions
            స్వయంచాలక funcps = std::move(m_cfuncs(modulep).m_fast);
            funcps.splice(funcps.చివర(), m_cfuncs(modulep).m_slow);

            V3DupFinder dupFinder{m_hasher};

            // First, hash all functions
            కొరకు (AstCFunc* స్థిరరాశి funcp : funcps) dupFinder.చొప్పించు(funcp);

            // Iterate to fixed point
            {
                స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;
                అయితే (combinePass(funcps, dupFinder)) {}
            }
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override {
        // Gather functions and references
        iterateChildrenConst(nodep);
        // Combine functions
        process(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        iterateChildrenConst(nodep);
        ఒకవేళ (nodep->dontCombine()) తిరిగి;
        స్వయంచాలక& coll = nodep->slow() ? m_cfuncs(m_modp).m_slow : m_cfuncs(m_modp).m_fast;
        coll.emplace_వెనుక(nodep);
    }
    శూన్య సందర్శించు(AstCCall* nodep) override {
        iterateChildrenConst(nodep);
        AstCFunc* స్థిరరాశి funcp = nodep->funcp();
        ఒకవేళ (funcp->dontCombine()) తిరిగి;
        m_callSites(funcp).emplace_వెనుక(nodep);
    }

    శూన్య సందర్శించు(Ast_Addr_యొక్క_CFunc* nodep) override {
        iterateChildrenConst(nodep);
        ఒకవేళ (nodep->funcp()->dontCombine()) తిరిగి;
        // LCOV_EXCL_START
        // We cannot yet handle references via AstAddrOfCFunc, but currently those are
        // only used in tracing functions, which are not combined. Blow up in case this changes.
        nodep->v3fatalSrc(
            "Don't know how to combine functions that are referenced via AstAddrOfCFunc");
        // LCOV_EXCL_STOP
    }

    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    స్పష్టమైన CombineVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CombineVisitor() override {
        V3Stats::కూడు_Stat("Optimizations, Combined CFuncs", m_cfuncsCombined);
    }

బహిరంగ:
    స్థిర శూన్య apply(AstNetlist* netlistp) { CombineVisitor{netlistp}; }
};

//######################################################################
// Combine class functions

శూన్య V3Combine::combineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CombineVisitor::apply(nodep);
    V3Global::dump_Check_Global_వృక్షం("combine", 0, dump_వృక్షం_Either_Level() >= 3);
}
