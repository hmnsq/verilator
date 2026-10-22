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
    स्थिरांक VNUser1InUse m_user1InUse;

    // TYPES
    using funcit_t = std::list<AstCFunc*>::iterator;
    संरचना CFuncs final {
        std::list<AstCFunc*> m_fast;  // Functions that are not slow
        std::list<AstCFunc*> m_slow;  // Functions marked for slow files
    };

    // STATE
    AstUser1Allocator<Ast_गाठ_Module, CFuncs> m_cfuncs;  // AstCFuncs under module
    AstUser1Allocator<AstCFunc, std::vector<AstCCall*>> m_callSites;  // Call sites of the AstCFunc
    Ast_गाठ_Module* m_modp = nullptr;  // Current module
    स्थिरांक V3Hasher m_hasher;  // For hashing
    VDouble0 m_cfuncsCombined;  // Statistic tracking

    // METHODS

    शून्य removeEmptyFunctions(std::list<AstCFunc*>& funcps) {
        साठी (funcit_t it = funcps.begin(), nit; it != funcps.end(); it = nit) {
            AstCFunc* स्थिरांक funcp = *it;
            nit = it;
            ++nit;

            जर (funcp->emptyBody()) {
                // Delete call sites
                साठी (AstCCall* स्थिरांक callp : m_callSites(funcp)) {
                    UASSERT_OBJ(VN_IS(callp->backp(), Stmt_अभिव्यक्ती), callp,
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
        स्थिरांक VNUser3InUse user3InUse;

        तार्किक replaced = असत्य;

        // Replace all identical functions with the first function in the list
        साठी (funcit_t it = funcps.begin(), nit; it != funcps.end(); it = nit) {
            AstCFunc* स्थिरांक funcp = *it;
            nit = it;
            ++nit;

            // Remove functions already replaced in the previous iteration
            जर (funcp->user2()) {
                funcps.erase(it);
                VL_DO_DANGLING(funcp->unlinkFrBack()->delete_वृक्ष(), funcp);
                सुरूठेवा;
            }

            पर्यंत (सत्य) {
                स्वतः dit = dupFinder.findDuplicate(funcp);
                जर (dit == dupFinder.end()) थांबा;

                AstCFunc* oldp = VN_AS(dit->second, CFunc);
                AstCFunc* newp = funcp;
                UASSERT_OBJ(!oldp->user2(), oldp, "Should have been removed from dupFinder");

                // Swap them, if the duplicate is earlier in the list of functions. This is
                // necessary because replacing a call site in a later function might have made that
                // function equivalent to an earlier function, but we want the first equivalent
                // function in the list to be the canonical one.
                जर (oldp->user3()) std::swap(oldp, newp);

                // Something is being replaced
                UINFO(9, "Replacing " << oldp);
                UINFO(9, "     with " << newp);
                ++m_cfuncsCombined;
                replaced = सत्य;

                // Mark as replaced
                oldp->user2(सत्य);

                // Redirect the calls
                साठी (AstCCall* स्थिरांक callp : m_callSites(oldp)) {
                    // For sanity check only
                    स्थिरांक V3Hash oldHash = m_hasher(callp);

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
                जर (oldp == funcp) थांबा;
            }

            // Mark as function earlier in list of functions.
            funcp->user3(सत्य);
        }

        परत replaced;
    }

    // cppcheck-suppress constParameterPointer
    शून्य process(AstNetlist* netlistp) {
        // First, remove empty functions. We need to do this separately, because removing
        // calls can change the hashes of the callers.
        साठी (Ast_गाठ_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), गाठ_Module)) {
            removeEmptyFunctions(m_cfuncs(modulep).m_fast);
            removeEmptyFunctions(m_cfuncs(modulep).m_slow);
        }

        // Combine functions within each module
        साठी (Ast_गाठ_Module* modulep = netlistp->modulesp(); modulep;
             modulep = VN_AS(modulep->nextp(), गाठ_Module)) {
            // Put fast functions first, so they are preferred over slow functions
            स्वतः funcps = std::move(m_cfuncs(modulep).m_fast);
            funcps.splice(funcps.end(), m_cfuncs(modulep).m_slow);

            V3DupFinder dupFinder{m_hasher};

            // First, hash all functions
            साठी (AstCFunc* स्थिरांक funcp : funcps) dupFinder.घाला(funcp);

            // Iterate to fixed point
            {
                स्थिरांक VNUser2InUse user2InUse;
                पर्यंत (combinePass(funcps, dupFinder)) {}
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
    शून्य visit(Ast_गाठ_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCFunc* nodep) override {
        iterateChildrenConst(nodep);
        जर (nodep->dontCombine()) परत;
        स्वतः& coll = nodep->slow() ? m_cfuncs(m_modp).m_slow : m_cfuncs(m_modp).m_fast;
        coll.emplace_back(nodep);
    }
    शून्य visit(AstCCall* nodep) override {
        iterateChildrenConst(nodep);
        AstCFunc* स्थिरांक funcp = nodep->funcp();
        जर (funcp->dontCombine()) परत;
        m_callSites(funcp).emplace_back(nodep);
    }

    शून्य visit(AstAddrOfCFunc* nodep) override {
        iterateChildrenConst(nodep);
        जर (nodep->funcp()->dontCombine()) परत;
        // LCOV_EXCL_START
        // We cannot yet handle references via AstAddrOfCFunc, but currently those are
        // only used in tracing functions, which are not combined. Blow up in case this changes.
        nodep->v3fatalSrc(
            "Don't know how to combine functions that are referenced via AstAddrOfCFunc");
        // LCOV_EXCL_STOP
    }

    //--------------------
    शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }

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
