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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Combine.h"

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3DupFinder.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <list>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

വർഗ്ഗം CombineVisitor final : VNVisitor {
    // NODE STATE
    // AstNodeModule::user1()   List of AstCFuncs in this module (via m_cfuncs)
    // AstCFunc::user1()        List of AstCCalls to this function (via m_callSites)
    // AstCFunc::user2()        bool: Already replaced (in 'process')
    // AstCFunc::user3()        bool: Marks functions earlier in iteration order (in 'combinePass')
    // *::user4()               Used by V3Hasher
    സ്ഥിരം VNUser1InUse m_user1InUse;

    // TYPES
    using funcit_t = std::list<AstCFunc*>::iterator;
    ഘടന CFuncs final {
        std::list<AstCFunc*> m_fast;  // Functions that are not slow
        std::list<AstCFunc*> m_slow;  // Functions marked for slow files
    };

    // STATE
    AstUser1Allocator<Ast_കണ്ണി_Module, CFuncs> m_cfuncs;  // AstCFuncs under module
    AstUser1Allocator<AstCFunc, std::vector<AstCCall*>> m_callSites;  // Call sites of the AstCFunc
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    സ്ഥിരം V3Hasher m_hasher;  // For hashing
    VDouble0 m_cfuncsCombined;  // Statistic tracking

    // METHODS

    ശൂന്യം removeEmptyFunctions(std::list<AstCFunc*>& funcps) {
        വേണ്ടി (funcit_t it = funcps.begin(), nit; it != funcps.end(); it = nit) {
            AstCFunc* സ്ഥിരം funcp = *it;
            nit = it;
            ++nit;

            എങ്കിൽ (funcp->emptyBody()) {
                // Delete call sites
                വേണ്ടി (AstCCall* സ്ഥിരം callp : m_callSites(funcp)) {
                    UASSERT_OBJ(VN_IS(callp->backp(), Stmt_വ്യഞ്ജകം), callp,
                                "Deleting non-statement call");
                    VL_DO_DANGLING(callp->backp()->unlinkFrBack()->delete_വൃക്ഷം(), callp);
                }
                m_callSites(funcp).clear();
                // Remove from list
                funcps.erase(it);
                // Delete function
                VL_DO_DANGLING(funcp->unlinkFrBack()->delete_വൃക്ഷം(), funcp);
            }
        }
    }

    // One pass of combining. Returns true if did replacement.
    തർക്കം combinePass(std::list<AstCFunc*>& funcps, V3DupFinder& dupFinder) {
        സ്ഥിരം VNUser3InUse user3InUse;

        തർക്കം replaced = അസത്യം;

        // Replace all identical functions with the first function in the list
        വേണ്ടി (funcit_t it = funcps.begin(), nit; it != funcps.end(); it = nit) {
            AstCFunc* സ്ഥിരം funcp = *it;
            nit = it;
            ++nit;

            // Remove functions already replaced in the previous iteration
            എങ്കിൽ (funcp->user2()) {
                funcps.erase(it);
                VL_DO_DANGLING(funcp->unlinkFrBack()->delete_വൃക്ഷം(), funcp);
                തുടരുക;
            }

            ആകുമ്പോൾ (സത്യം) {
                auto dit = dupFinder.findDuplicate(funcp);
                എങ്കിൽ (dit == dupFinder.end()) നിർത്തുക;

                AstCFunc* oldp = VN_AS(dit->second, CFunc);
                AstCFunc* newp = funcp;
                UASSERT_OBJ(!oldp->user2(), oldp, "Should have been removed from dupFinder");

                // Swap them, if the duplicate is earlier in the list of functions. This is
                // necessary because replacing a call site in a later function might have made that
                // function equivalent to an earlier function, but we want the first equivalent
                // function in the list to be the canonical one.
                എങ്കിൽ (oldp->user3()) std::swap(oldp, newp);

                // Something is being replaced
                UINFO(9, "Replacing " << oldp);
                UINFO(9, "     with " << newp);
                ++m_cfuncsCombined;
                replaced = സത്യം;

                // Mark as replaced
                oldp->user2(സത്യം);

                // Redirect the calls
                വേണ്ടി (AstCCall* സ്ഥിരം callp : m_callSites(oldp)) {
                    // For sanity check only
                    സ്ഥിരം V3Hash oldHash = m_hasher(callp);

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
                എങ്കിൽ (oldp == funcp) നിർത്തുക;
            }

            // Mark as function earlier in list of functions.
            funcp->user3(സത്യം);
        }

        തിരികെ replaced;
    }

    // cppcheck-suppress constParameterPointer
    ശൂന്യം process(AstNetlist* netlistp) {
        // First, remove empty functions. We need to do this separately, because removing
        // calls can change the hashes of the callers.
        വേണ്ടി (Ast_കണ്ണി_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), കണ്ണി_Module)) {
            removeEmptyFunctions(m_cfuncs(modulep).m_fast);
            removeEmptyFunctions(m_cfuncs(modulep).m_slow);
        }

        // Combine functions within each module
        വേണ്ടി (Ast_കണ്ണി_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), കണ്ണി_Module)) {
            // Put fast functions first, so they are preferred over slow functions
            auto funcps = std::move(m_cfuncs(modulep).m_fast);
            funcps.splice(funcps.end(), m_cfuncs(modulep).m_slow);

            V3DupFinder dupFinder{m_hasher};

            // First, hash all functions
            വേണ്ടി (AstCFunc* സ്ഥിരം funcp : funcps) dupFinder.തിരുകുക(funcp);

            // Iterate to fixed point
            {
                സ്ഥിരം VNUser2InUse user2InUse;
                ആകുമ്പോൾ (combinePass(funcps, dupFinder)) {}
            }
        }
    }

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override {
        // Gather functions and references
        iterateChildrenConst(nodep);
        // Combine functions
        process(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        iterateChildrenConst(nodep);
        എങ്കിൽ (nodep->dontCombine()) തിരികെ;
        auto& coll = nodep->slow() ? m_cfuncs(m_modp).m_slow : m_cfuncs(m_modp).m_fast;
        coll.emplace_back(nodep);
    }
    ശൂന്യം visit(AstCCall* nodep) override {
        iterateChildrenConst(nodep);
        AstCFunc* സ്ഥിരം funcp = nodep->funcp();
        എങ്കിൽ (funcp->dontCombine()) തിരികെ;
        m_callSites(funcp).emplace_back(nodep);
    }

    ശൂന്യം visit(AstAddrOfCFunc* nodep) override {
        iterateChildrenConst(nodep);
        എങ്കിൽ (nodep->funcp()->dontCombine()) തിരികെ;
        // LCOV_EXCL_START
        // We cannot yet handle references via AstAddrOfCFunc, but currently those are
        // only used in tracing functions, which are not combined. Blow up in case this changes.
        nodep->v3fatalSrc(
            "Don't know how to combine functions that are referenced via AstAddrOfCFunc");
        // LCOV_EXCL_STOP
    }

    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CombineVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CombineVisitor() override {
        V3Stats::addStat("Optimizations, Combined CFuncs", m_cfuncsCombined);
    }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(AstNetlist* netlistp) { CombineVisitor{netlistp}; }
};

//######################################################################
// Combine class functions

ശൂന്യം V3Combine::combineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CombineVisitor::apply(nodep);
    V3Global::dump_Check_Global_വൃക്ഷം("combine", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
