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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Combine.h"

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3DupFinder.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <list>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

वर्ग CombineVisitor final : VNVisitor {
    // NODE STATE
    // AstNodeModule::user1()   List of AstCFuncs in this module (via m_cfuncs)
    // AstCFunc::user1()        List of AstCCalls to this function (via m_callSites)
    // AstCFunc::user2()        bool: Already replaced (in 'process')
    // AstCFunc::user3()        bool: Marks functions earlier in iteration order (in 'combinePass')
    // *::user4()               Used by V3Hasher
    ध्रुव VNUser1InUse m_user1InUse;

    // TYPES
    using funcit_t = std::list<AstCFunc*>::iterator;
    संरचना CFuncs final {
        std::list<AstCFunc*> m_fast;  // Functions that are not slow
        std::list<AstCFunc*> m_slow;  // Functions marked for slow files
    };

    // STATE
    AstUser1Allocator<Ast_गाँठ_Module, CFuncs> m_cfuncs;  // AstCFuncs under module
    AstUser1Allocator<AstCFunc, std::vector<AstCCall*>> m_callSites;  // Call sites of the AstCFunc
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    ध्रुव V3Hasher m_hasher;  // For hashing
    VDouble0 m_cfuncsCombined;  // Statistic tracking

    // METHODS

    शून्य removeEmptyFunctions(std::list<AstCFunc*>& funcps) {
        प्रति (funcit_t it = funcps.begin(), nit; it != funcps.end(); it = nit) {
            AstCFunc* ध्रुव funcp = *it;
            nit = it;
            ++nit;

            यदि (funcp->emptyBody()) {
                // Delete call sites
                प्रति (AstCCall* ध्रुव callp : m_callSites(funcp)) {
                    UASSERT_OBJ(VN_IS(callp->backp(), Stmt_व्यंजक), callp,
                                "Deleting non-statement call");
                    VL_DO_DANGLING(callp->backp()->unlinkFrBack()->delete_वृक्ष(), callp);
                }
                m_callSites(funcp).clear();
                // Remove from list
                funcps.erase(it);
                // Delete function
                VL_DO_DANGLING(funcp->unlinkFrBack()->delete_वृक्ष(), funcp);
            }
        }
    }

    // One pass of combining. Returns true if did replacement.
    तार्किक combinePass(std::list<AstCFunc*>& funcps, V3DupFinder& dupFinder) {
        ध्रुव VNUser3InUse user3InUse;

        तार्किक replaced = असत्य;

        // Replace all identical functions with the first function in the list
        प्रति (funcit_t it = funcps.begin(), nit; it != funcps.end(); it = nit) {
            AstCFunc* ध्रुव funcp = *it;
            nit = it;
            ++nit;

            // Remove functions already replaced in the previous iteration
            यदि (funcp->user2()) {
                funcps.erase(it);
                VL_DO_DANGLING(funcp->unlinkFrBack()->delete_वृक्ष(), funcp);
                अनुवर्त;
            }

            यावत् (सत्य) {
                स्वतः dit = dupFinder.findDuplicate(funcp);
                यदि (dit == dupFinder.end()) भङ्ग;

                AstCFunc* oldp = VN_AS(dit->second, CFunc);
                AstCFunc* newp = funcp;
                UASSERT_OBJ(!oldp->user2(), oldp, "Should have been removed from dupFinder");

                // Swap them, if the duplicate is earlier in the list of functions. This is
                // necessary because replacing a call site in a later function might have made that
                // function equivalent to an earlier function, but we want the first equivalent
                // function in the list to be the canonical one.
                यदि (oldp->user3()) std::swap(oldp, newp);

                // Something is being replaced
                UINFO(9, "Replacing " << oldp);
                UINFO(9, "     with " << newp);
                ++m_cfuncsCombined;
                replaced = सत्य;

                // Mark as replaced
                oldp->user2(सत्य);

                // Redirect the calls
                प्रति (AstCCall* ध्रुव callp : m_callSites(oldp)) {
                    // For sanity check only
                    ध्रुव V3Hash oldHash = m_hasher(callp);

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
                यदि (oldp == funcp) भङ्ग;
            }

            // Mark as function earlier in list of functions.
            funcp->user3(सत्य);
        }

        प्रत्यागम replaced;
    }

    // cppcheck-suppress constParameterPointer
    शून्य process(AstNetlist* netlistp) {
        // First, remove empty functions. We need to do this separately, because removing
        // calls can change the hashes of the callers.
        प्रति (Ast_गाँठ_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), गाँठ_Module)) {
            removeEmptyFunctions(m_cfuncs(modulep).m_fast);
            removeEmptyFunctions(m_cfuncs(modulep).m_slow);
        }

        // Combine functions within each module
        प्रति (Ast_गाँठ_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), गाँठ_Module)) {
            // Put fast functions first, so they are preferred over slow functions
            स्वतः funcps = std::move(m_cfuncs(modulep).m_fast);
            funcps.splice(funcps.end(), m_cfuncs(modulep).m_slow);

            V3DupFinder dupFinder{m_hasher};

            // First, hash all functions
            प्रति (AstCFunc* ध्रुव funcp : funcps) dupFinder.जोड़ो(funcp);

            // Iterate to fixed point
            {
                ध्रुव VNUser2InUse user2InUse;
                यावत् (combinePass(funcps, dupFinder)) {}
            }
        }
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {
        // Gather functions and references
        iterateChildrenConst(nodep);
        // Combine functions
        process(nodep);
    }
    शून्य visit(Ast_गाँठ_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCFunc* nodep) override {
        iterateChildrenConst(nodep);
        यदि (nodep->dontCombine()) प्रत्यागम;
        स्वतः& coll = nodep->slow() ? m_cfuncs(m_modp).m_slow : m_cfuncs(m_modp).m_fast;
        coll.emplace_back(nodep);
    }
    शून्य visit(AstCCall* nodep) override {
        iterateChildrenConst(nodep);
        AstCFunc* ध्रुव funcp = nodep->funcp();
        यदि (funcp->dontCombine()) प्रत्यागम;
        m_callSites(funcp).emplace_back(nodep);
    }

    शून्य visit(AstAddrOfCFunc* nodep) override {
        iterateChildrenConst(nodep);
        यदि (nodep->funcp()->dontCombine()) प्रत्यागम;
        // LCOV_EXCL_START
        // We cannot yet handle references via AstAddrOfCFunc, but currently those are
        // only used in tracing functions, which are not combined. Blow up in case this changes.
        nodep->v3fatalSrc(
            "Don't know how to combine functions that are referenced via AstAddrOfCFunc");
        // LCOV_EXCL_STOP
    }

    //--------------------
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CombineVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CombineVisitor() override {
        V3Stats::addStat("Optimizations, Combined CFuncs", m_cfuncsCombined);
    }

सार्वजनिक:
    स्थिर शून्य apply(AstNetlist* netlistp) { CombineVisitor{netlistp}; }
};

//######################################################################
// Combine class functions

शून्य V3Combine::combineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CombineVisitor::apply(nodep);
    V3Global::dump_Check_Global_वृक्ष("combine", 0, dump_वृक्ष_Either_Level() >= 3);
}
