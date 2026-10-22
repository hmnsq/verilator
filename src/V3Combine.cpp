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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Combine.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3DupFinder.h"
#ضمن "V3Stats.h"

#ضمن <list>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

صنف CombineVisitor final : VNVisitor {
    // NODE STATE
    // AstNodeModule::user1()   List of AstCFuncs in this module (via m_cfuncs)
    // AstCFunc::user1()        List of AstCCalls to this function (via m_callSites)
    // AstCFunc::user2()        bool: Already replaced (in 'process')
    // AstCFunc::user3()        bool: Marks functions earlier in iteration order (in 'combinePass')
    // *::user4()               Used by V3Hasher
    ثابت VNUser1_اندريون_Use m_user1_اندريون_Use;

    // TYPES
    استعمال funcit_t = std::list<AstCFunc*>::iterator;
    بنية CFuncs final {
        std::list<AstCFunc*> m_fast;  // Functions that are not slow
        std::list<AstCFunc*> m_slow;  // Functions marked for slow files
    };

    // STATE
    AstUser1Allocator<Ast_ڳنڍ_Module, CFuncs> m_cfuncs;  // AstCFuncs under module
    AstUser1Allocator<AstCFunc, std::ويڪٽر<AstCCall*>> m_callSites;  // Call sites of the AstCFunc
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    ثابت V3Hasher m_hasher;  // For hashing
    VDouble0 m_cfuncsCombined;  // Statistic tracking

    // METHODS

    فراغ remove_خالي_جايون_Functions(std::list<AstCFunc*>& funcps) {
        لكل (funcit_t it = funcps.begin(), nit; it != funcps.پڇاڙي(); it = nit) {
            AstCFunc* ثابت funcp = *it;
            nit = it;
            ++nit;

            إذا (funcp->خالي_جايون_Body()) {
                // Delete call sites
                لكل (AstCCall* ثابت callp : m_callSites(funcp)) {
                    UASSERT_OBJ(VN_IS(callp->backp(), Stmt_اظهار), callp,
                                "Deleting non-statement call");
                    VL_DO_DANGLING(callp->backp()->unlink_Fr_پٺ()->delete_وڻ(), callp);
                }
                m_callSites(funcp).clear();
                // Remove from list
                funcps.erase(it);
                // Delete function
                VL_DO_DANGLING(funcp->unlink_Fr_پٺ()->delete_وڻ(), funcp);
            }
        }
    }

    // One pass of combining. Returns true if did replacement.
    منطقي combinePass(std::list<AstCFunc*>& funcps, V3DupFinder& dupFinder) {
        ثابت VNUser3_اندريون_Use user3_اندريون_Use;

        منطقي replaced = خطأ;

        // Replace all identical functions with the first function in the list
        لكل (funcit_t it = funcps.begin(), nit; it != funcps.پڇاڙي(); it = nit) {
            AstCFunc* ثابت funcp = *it;
            nit = it;
            ++nit;

            // Remove functions already replaced in the previous iteration
            إذا (funcp->user2()) {
                funcps.erase(it);
                VL_DO_DANGLING(funcp->unlink_Fr_پٺ()->delete_وڻ(), funcp);
                استمر;
            }

            طالما (سچ) {
                خودڪار dit = dupFinder.ڳوليو_Duplicate(funcp);
                إذا (dit == dupFinder.پڇاڙي()) اكسر;

                AstCFunc* oldp = VN_AS(dit->ٻيو, CFunc);
                AstCFunc* newp = funcp;
                UASSERT_OBJ(!oldp->user2(), oldp, "Should have been removed from dupFinder");

                // Swap them, if the duplicate is earlier in the list of functions. This is
                // necessary because replacing a call site in a later function might have made that
                // function equivalent to an earlier function, but we want the first equivalent
                // function in the list to be the canonical one.
                إذا (oldp->user3()) std::مٽا_سٽا(oldp, newp);

                // Something is being replaced
                UINFO(9, "Replacing " << oldp);
                UINFO(9, "     with " << newp);
                ++m_cfuncsCombined;
                replaced = سچ;

                // Mark as replaced
                oldp->user2(سچ);

                // Redirect the calls
                لكل (AstCCall* ثابت callp : m_callSites(oldp)) {
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
                إذا (oldp == funcp) اكسر;
            }

            // Mark as function earlier in list of functions.
            funcp->user3(سچ);
        }

        أرجع replaced;
    }

    // cppcheck-suppress constParameterPointer
    فراغ process(AstNetlist* netlistp) {
        // First, remove empty functions. We need to do this separately, because removing
        // calls can change the hashes of the callers.
        لكل (Ast_ڳنڍ_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), ڳنڍ_Module)) {
            remove_خالي_جايون_Functions(m_cfuncs(modulep).m_fast);
            remove_خالي_جايون_Functions(m_cfuncs(modulep).m_slow);
        }

        // Combine functions within each module
        لكل (Ast_ڳنڍ_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), ڳنڍ_Module)) {
            // Put fast functions first, so they are preferred over slow functions
            خودڪار funcps = std::move(m_cfuncs(modulep).m_fast);
            funcps.splice(funcps.پڇاڙي(), m_cfuncs(modulep).m_slow);

            V3DupFinder dupFinder{m_hasher};

            // First, hash all functions
            لكل (AstCFunc* ثابت funcp : funcps) dupFinder.وجهو(funcp);

            // Iterate to fixed point
            {
                ثابت VNUser2_اندريون_Use user2_اندريون_Use;
                طالما (combinePass(funcps, dupFinder)) {}
            }
        }
    }

    // VISITORS
    فراغ گهمو(AstNetlist* nodep) override {
        // Gather functions and references
        iterateChildrenConst(nodep);
        // Combine functions
        process(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstCFunc* nodep) override {
        iterateChildrenConst(nodep);
        إذا (nodep->dontCombine()) أرجع;
        خودڪار& coll = nodep->slow() ? m_cfuncs(m_modp).m_slow : m_cfuncs(m_modp).m_fast;
        coll.emplace_پٺ(nodep);
    }
    فراغ گهمو(AstCCall* nodep) override {
        iterateChildrenConst(nodep);
        AstCFunc* ثابت funcp = nodep->funcp();
        إذا (funcp->dontCombine()) أرجع;
        m_callSites(funcp).emplace_پٺ(nodep);
    }

    فراغ گهمو(Ast_Addr_جو_CFunc* nodep) override {
        iterateChildrenConst(nodep);
        إذا (nodep->funcp()->dontCombine()) أرجع;
        // LCOV_EXCL_START
        // We cannot yet handle references via AstAddrOfCFunc, but currently those are
        // only used in tracing functions, which are not combined. Blow up in case this changes.
        nodep->v3fatalSrc(
            "Don't know how to combine functions that are referenced via AstAddrOfCFunc");
        // LCOV_EXCL_STOP
    }

    //--------------------
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    واضح CombineVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CombineVisitor() override {
        V3Stats::جوڙيو_Stat("Optimizations, Combined CFuncs", m_cfuncsCombined);
    }

عام:
    ساكن فراغ apply(AstNetlist* netlistp) { CombineVisitor{netlistp}; }
};

//######################################################################
// Combine class functions

فراغ V3Combine::combineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    CombineVisitor::apply(nodep);
    V3Global::dump_Check_Global_وڻ("combine", 0, dump_وڻ_Either_Level() >= 3);
}
