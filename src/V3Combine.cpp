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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Combine.h"

#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3DupFinder.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <list>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

વર્ગ CombineVisitor final : VNVisitor {
    // NODE STATE
    // AstNodeModule::user1()   List of AstCFuncs in this module (via m_cfuncs)
    // AstCFunc::user1()        List of AstCCalls to this function (via m_callSites)
    // AstCFunc::user2()        bool: Already replaced (in 'process')
    // AstCFunc::user3()        bool: Marks functions earlier in iteration order (in 'combinePass')
    // *::user4()               Used by V3Hasher
    અચલ VNUser1InUse m_user1InUse;

    // TYPES
    using funcit_t = std::list<AstCFunc*>::iterator;
    સંરચના CFuncs final {
        std::list<AstCFunc*> m_fast;  // Functions that are not slow
        std::list<AstCFunc*> m_slow;  // Functions marked for slow files
    };

    // STATE
    AstUser1Allocator<Ast_ગાંઠ_Module, CFuncs> m_cfuncs;  // AstCFuncs under module
    AstUser1Allocator<AstCFunc, std::vector<AstCCall*>> m_callSites;  // Call sites of the AstCFunc
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    અચલ V3Hasher m_hasher;  // For hashing
    VDouble0 m_cfuncsCombined;  // Statistic tracking

    // METHODS

    શૂન્ય removeEmptyFunctions(std::list<AstCFunc*>& funcps) {
        માટે (funcit_t it = funcps.begin(), nit; it != funcps.end(); it = nit) {
            AstCFunc* અચલ funcp = *it;
            nit = it;
            ++nit;

            જો (funcp->emptyBody()) {
                // Delete call sites
                માટે (AstCCall* અચલ callp : m_callSites(funcp)) {
                    UASSERT_OBJ(VN_IS(callp->backp(), Stmt_પદાવલિ), callp,
                                "Deleting non-statement call");
                    VL_DO_DANGLING(callp->backp()->unlinkFrBack()->delete_વૃક્ષ(), callp);
                }
                m_callSites(funcp).clear();
                // Remove from list
                funcps.erase(it);
                // Delete function
                VL_DO_DANGLING(funcp->unlinkFrBack()->delete_વૃક્ષ(), funcp);
            }
        }
    }

    // One pass of combining. Returns true if did replacement.
    તાર્કિક combinePass(std::list<AstCFunc*>& funcps, V3DupFinder& dupFinder) {
        અચલ VNUser3InUse user3InUse;

        તાર્કિક replaced = ખોટું;

        // Replace all identical functions with the first function in the list
        માટે (funcit_t it = funcps.begin(), nit; it != funcps.end(); it = nit) {
            AstCFunc* અચલ funcp = *it;
            nit = it;
            ++nit;

            // Remove functions already replaced in the previous iteration
            જો (funcp->user2()) {
                funcps.erase(it);
                VL_DO_DANGLING(funcp->unlinkFrBack()->delete_વૃક્ષ(), funcp);
                ચાલુ;
            }

            જ્યારે (સાચું) {
                auto dit = dupFinder.findDuplicate(funcp);
                જો (dit == dupFinder.end()) તોડો;

                AstCFunc* oldp = VN_AS(dit->second, CFunc);
                AstCFunc* newp = funcp;
                UASSERT_OBJ(!oldp->user2(), oldp, "Should have been removed from dupFinder");

                // Swap them, if the duplicate is earlier in the list of functions. This is
                // necessary because replacing a call site in a later function might have made that
                // function equivalent to an earlier function, but we want the first equivalent
                // function in the list to be the canonical one.
                જો (oldp->user3()) std::swap(oldp, newp);

                // Something is being replaced
                UINFO(9, "Replacing " << oldp);
                UINFO(9, "     with " << newp);
                ++m_cfuncsCombined;
                replaced = સાચું;

                // Mark as replaced
                oldp->user2(સાચું);

                // Redirect the calls
                માટે (AstCCall* અચલ callp : m_callSites(oldp)) {
                    // For sanity check only
                    અચલ V3Hash oldHash = m_hasher(callp);

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
                જો (oldp == funcp) તોડો;
            }

            // Mark as function earlier in list of functions.
            funcp->user3(સાચું);
        }

        પાછા replaced;
    }

    // cppcheck-suppress constParameterPointer
    શૂન્ય process(AstNetlist* netlistp) {
        // First, remove empty functions. We need to do this separately, because removing
        // calls can change the hashes of the callers.
        માટે (Ast_ગાંઠ_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), ગાંઠ_Module)) {
            removeEmptyFunctions(m_cfuncs(modulep).m_fast);
            removeEmptyFunctions(m_cfuncs(modulep).m_slow);
        }

        // Combine functions within each module
        માટે (Ast_ગાંઠ_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), ગાંઠ_Module)) {
            // Put fast functions first, so they are preferred over slow functions
            auto funcps = std::move(m_cfuncs(modulep).m_fast);
            funcps.splice(funcps.end(), m_cfuncs(modulep).m_slow);

            V3DupFinder dupFinder{m_hasher};

            // First, hash all functions
            માટે (AstCFunc* અચલ funcp : funcps) dupFinder.ઉમેરો(funcp);

            // Iterate to fixed point
            {
                અચલ VNUser2InUse user2InUse;
                જ્યારે (combinePass(funcps, dupFinder)) {}
            }
        }
    }

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override {
        // Gather functions and references
        iterateChildrenConst(nodep);
        // Combine functions
        process(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        iterateChildrenConst(nodep);
        જો (nodep->dontCombine()) પાછા;
        auto& coll = nodep->slow() ? m_cfuncs(m_modp).m_slow : m_cfuncs(m_modp).m_fast;
        coll.emplace_back(nodep);
    }
    શૂન્ય visit(AstCCall* nodep) override {
        iterateChildrenConst(nodep);
        AstCFunc* અચલ funcp = nodep->funcp();
        જો (funcp->dontCombine()) પાછા;
        m_callSites(funcp).emplace_back(nodep);
    }

    શૂન્ય visit(AstAddrOfCFunc* nodep) override {
        iterateChildrenConst(nodep);
        જો (nodep->funcp()->dontCombine()) પાછા;
        // LCOV_EXCL_START
        // We cannot yet handle references via AstAddrOfCFunc, but currently those are
        // only used in tracing functions, which are not combined. Blow up in case this changes.
        nodep->v3fatalSrc(
            "Don't know how to combine functions that are referenced via AstAddrOfCFunc");
        // LCOV_EXCL_STOP
    }

    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CombineVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CombineVisitor() override {
        V3Stats::addStat("Optimizations, Combined CFuncs", m_cfuncsCombined);
    }

જાહેર:
    સ્થિર શૂન્ય apply(AstNetlist* netlistp) { CombineVisitor{netlistp}; }
};

//######################################################################
// Combine class functions

શૂન્ય V3Combine::combineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CombineVisitor::apply(nodep);
    V3Global::dump_Check_Global_વૃક્ષ("combine", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
