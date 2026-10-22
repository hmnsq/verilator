// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Add temporaries, such as for delayed nodes
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
// V3Delayed's Transformations:
//
// Convert AstAssignDly into temporaries an specially scheduled blocks.
// For the Pre/Post scheduling semantics, see V3OrderGraph.
//
// There are several "Schemes" we can choose from for implementing a
// non-blocking assignment (NBA), represented by an AstAssignDly.
//
// It is assumed and required in this pass that each NBA updates at
// most one variable. Earlier passes should have ensured this.
//
// Each variable is associated with a single NBA scheme, that is, all
// NBAs targeting the same variable will use the scheme assigned to
// that variable. This necessitates a global analysis of all NBAs
// before any decision can be made on how to handle them.
//
// The algorithm proceeds in 3 steps.
// 1. Gather all AstAssignDly non-blocking assignments (NBAs) in the
//    whole design. Note usage context of variables updated by these NBAs.
//    This is implemented in the 'visit' methods
// 2. For each variable that is the target of an NBA, decide which of
//    the possible conversion schemes to use, based on info gathered in
//    step 1.
//    This is implemented in the 'chooseScheme' method
// 3. For each NBA gathered in step 1, convert it based on the scheme
//    selected in step 2.
//    This is implemented in the 'prepare*'/'convert*' methods. The
//    'prepare*' methods do the parts common for all NBAs updating
//    the given variable. The 'convert*' methods then convert each
//    AstAssignDly separately
//
// These are the possible NBA Schemes:
// "Shadow variable" scheme. Used for non-unpackedarray target
// variables in synthesizeable code. E.g.:
//   LHS <= RHS;
// is converted to:
//  - Add new "Pre-scheduled" logic:
//      __Vdly__LHS = LHS;
//  - In the original logic, replace the target variable 'LHS' with '__Vdly__LHS' variables:
//      __Vdly__LHS = RHS;
//  - Add new "Post-scheduled" logic:
//      LHS = __Vdly__LHS;
//
// "Shared flag" scheme. Used for unpacked array target variables
// in synthesizeable code. E.g.:
//   LHS[idxa][idxb] <= RHS
// is converted to:
//  - Add new "Pre-scheduled" logic:
//      __Vdly_Set__LHS = 0;
//  - In the original logic, replace the AstAssignDelay with:
//      __Vdly_Set__LHS = 1;
//      __Vdly_Dim0__LHS = idxa;
//      __Vdly_Dim1__LHS = idxb;
//      __Vdly_Val__LHS = RHS;
//  - Add new "Post-scheduled" logic:
//      if (__Vdly_Set__LHS) a[__Vdly_Dim0__LHS][__Vdly_Dim1__LHS] = __Vdly_Val__LHS;
// Multiple consecutive NBAs of compatible form can share the same  __Vdly_Set* flag
//
// "Shadow variable masked" scheme. Used for packed target variables that
// have both blocking and non-blocking updates. E.g.:
//   LHS[Index] <= RHS;
//   When there is also LHS[SomeNonOverlappingIndex] = RHS2;
// is converted to:
//  - In the original logic, replace the AstAssignDelay with:
//      __Vdly__LHS[Index] = RHS;
//      __Vdly_Mask__LHS[Index] = '1;
//  - Add new "Post-scheduled" logic:
//      LHS = (__Vdly__LHS & __Vdly_Mask__LHS) | (LHS & ~__Vdly_Mask__LHS);
//      __Vdly_Mask__LHS = '0;
//
// "Unique flag" scheme. Used for all variables updated by NBAs
// in suspendable processees or forks. E.g.:
//   #1 LHS <= RHS;
// is converted to:
//  - In the original logic, replace the AstAssignDelay with:
//      __Vdly_SetUnique__LHS = 1;
//      __Vdly_Val__LHS = RHS;
//  - Add new "Post-scheduled" logic:
//      if (__Vdly_SetUnique__LHS) {
//         __Vdly_SetUnique__LHS = 0;
//         LHS = __Vdly_Val__LHS;
//      }
//
// The "Value Queue Whole/Partial" schemes are used for cases where the
// target of an assignment cannot be statically determined, for example,
// with an array LHS in a loop:
//   LHS[idxa][idxb] <= RHS
// is converted to:
//  - In the original logic, replace the AstAssignDelay with:
//      __Vdly_Dim0__LHS = idxa;
//      __Vdly_Dim1__LHS = idxb;
//      __Vdly_Val__LHS = RHS;
//      __Vdly_CommitQueue__LHS.enqueue(__Vdly_Val__LHS, __Vdly_Dim0__LHS, __Vdly_Dim1__LHS);
//  - Add new "Post-scheduled" logic:
//      __Vdly_CommitQueue__LHS.commit(LHS);
//
// TODO: generic LHS scheme as discussed in #5092
//
//*************************************************************************

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Delayed.h"

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3ClassGraph.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3LinkLValue.h"
#अन्तर्भाव "V3SharedTmps.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <deque>
#अन्तर्भाव <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert AstAssignDlys (NBAs)

वर्ग DelayedVisitor final : सार्वजनिक VNVisitor {
    // TYPES

    // The various NBA conversion schemes, including error cases
    गणना वर्ग Scheme : uint8_t {
        Undecided = 0,
        UnsupportedCompoundArrayInLoop,
        ShadowVar,
        ShadowVarMasked,
        FlagShared,
        FlagUnique,
        मूल्य_Queue_Whole,
        मूल्य_Queue_Partial
    };

    // All info associated with a variable that is the target of an NBA
    वर्ग VarScopeInfo final {
    सार्वजनिक:
        // First write reference encountered to the VarScope as the target on an NBA
        स्थिरांक AstVarRef* m_firstNbaRefp = nullptr;
        // Active block 'm_firstNbaRefp' is under
        स्थिरांक AstActive* m_fistActivep = nullptr;
        तार्किक m_whole = असत्य;  // Used on LHS of NBA directly via VarRef
        तार्किक m_partial = असत्य;  // Used on LHS of NBA under a Sel
        तार्किक m_inLoop = असत्य;  // Used on LHS of NBA in a loop
        तार्किक m_inSuspOrFork = असत्य;  // Used on LHS of NBA in suspendable process or fork
        Scheme m_scheme = Scheme::Undecided;  // Conversion scheme to use for this variable

    खाजगी:
        // Combined sensitivities of all NBAs targeting this variable
        Ast_Sen_वृक्ष* m_senTreep = nullptr;

        // Union of stuff needed for the various schemes - use accessors below!
        संघ {
            संरचना {  // Stuff needed for Scheme::ShadowVar
                AstVarScope* vscp;  // The shadow variable
            } m_shadowVariableKit;
            संरचना {  // Stuff needed for Scheme::ShadowVarMasked
                AstVarScope* vscp;  // The shadow variable
                AstVarScope* maskp;  // The mask variable
            } m_shadowVarMaskedKit;
            संरचना {  // Stuff needed for Scheme::FlagShared
                AstActive* activep;  // The active block for the Pre/Post logic
                AstAlwaysPost* postp;  // The post block for commiting results
                AstVarScope* commitFlagp;  // The commit flag variable, for reuse
                AstIf* commitIfp;  // The previous if statement for committing, for reuse
            } m_flagSharedKit;
            संरचना {  // Stuff needed for Scheme::FlagUnique
                AstAlwaysPost* postp;  // The post block for commiting results
            } m_flagUniqueKit;
            संरचना {  // Stuff needed for Scheme::ValueQueueWhole/Scheme::ValueQueuePartial
                AstVarScope* vscp;  // The commit queue variable
            } m_मूल्य_Queue_Kit;
        } m_kitUnion;

    सार्वजनिक:
        VarScopeInfo() = पूर्वनिर्धारित;
        ~VarScopeInfo() {
            // Might not be linked if there was an error
            जर (!m_senTreep->backp()) VL_DO_DANGLING(m_senTreep->delete_वृक्ष(), m_senTreep);
        }
        // Accessors for the above union fields
        स्वतः& shadowVariableKit() {
            UASSERT(m_scheme == Scheme::ShadowVar, "Inconsistent Scheme");
            परत m_kitUnion.m_shadowVariableKit;
        }
        स्वतः& shadowVarMaskedKit() {
            UASSERT(m_scheme == Scheme::ShadowVarMasked, "Inconsistent Scheme");
            परत m_kitUnion.m_shadowVarMaskedKit;
        }
        स्वतः& flagSharedKit() {
            UASSERT(m_scheme == Scheme::FlagShared, "Inconsistent Scheme");
            परत m_kitUnion.m_flagSharedKit;
        }
        स्वतः& flagUniqueKit() {
            UASSERT(m_scheme == Scheme::FlagUnique, "Inconsistent Scheme");
            परत m_kitUnion.m_flagUniqueKit;
        }
        स्वतः& मूल्य_Queue_Kit() {
            UASSERT(m_scheme == Scheme::मूल्य_Queue_Partial || m_scheme == Scheme::मूल्य_Queue_Whole,
                    "Inconsistent Scheme");
            परत m_kitUnion.m_मूल्य_Queue_Kit;
        }

        // Accessor
        Ast_Sen_वृक्ष* senTreep() स्थिरांक { परत m_senTreep; }

        // Add sensitivities
        शून्य addSensitivity(AstSenItem* nodep) {
            जर (!m_senTreep) m_senTreep = नवीन Ast_Sen_वृक्ष{nodep->fileline(), nullptr};
            // Add a copy of each term
            m_senTreep->addSensesp(nodep->clone_वृक्ष(सत्य));
            // Remove duplicates
            V3Const::constifyExpensiveEdit(m_senTreep);
        }
        // cppcheck-suppress constParameterPointer
        शून्य addSensitivity(Ast_Sen_वृक्ष* nodep) { addSensitivity(nodep->sensesp()); }
    };

    // Data structure to keep track of all writes to
    संरचना WriteReference final {
        AstVarRef* m_refp = nullptr;  // The reference
        तार्किक m_isNBA = असत्य;  // True if an NBA write
        तार्किक m_inNonComb = असत्य;  // True if reference is known to be in non-combinational logic
        WriteReference() = पूर्वनिर्धारित;
        WriteReference(AstVarRef* refp, तार्किक isNBA, तार्किक inNonComb)
            : m_refp{refp}
            , m_isNBA{isNBA}
            , m_inNonComb{inNonComb} {}
    };

    // Data required to lower AstAssignDelay later
    संरचना NBA final {
        AstAssignDly* nodep = nullptr;  // The NBA this record refers to
        AstVarScope* vscp = nullptr;  // The target variable the NBA is updating
    };

    // NODE STATE
    //  AstVar::user1()         -> bool.  Set true if already issued MULTIDRIVEN warning
    //  AstVarRef::user1()      -> bool.  Set true if target of NBA
    //  AstAssignDly::user1()   -> bool.  Set true if already visited
    //  AstCFunc::user1()       -> AstUser1Allocator.  See `m_cfuncsCache` below
    //  AstAssignDly::user2p()  -> AstVarScope*: Scope this AstAssignDelay is under
    //  AstVarScope::user1p()   -> VarScopeInfo via m_vscpInfo
    //  AstVarScope::user2p()   -> AstVarRef*: First write reference to the Variable
    //  AstVarScope::user3p()   -> std::vector<WriteReference> via m_writeRefs;
    स्थिरांक VNUser1InUse m_user1InUse;
    स्थिरांक VNUser2InUse m_user2InUse;
    स्थिरांक VNUser3InUse m_user3InUse;

    संरचना CFuncCache final {
        VInsertionSet<Ast_Sen_वृक्ष*> m_timingDomains;  // What shall be added to m_timingDomains
        std::set<AstCFunc*>
            m_includes;  // CFuncs whose CFuncCache shall be included into this - this is used to
                         // break cycles: A->B->A (instead of visiting A while it is still begin
                         // visited B just marks that it includes A)
        गणना State : uint8_t {
            UNINITIALIZED = 0,  // Not initialized members are empty
            VISITING,  // Visiting - needed for breaking recursion
            INITIALIZED,  // Members contains correct values
        } m_state  // Current state of Cache
        = UNINITIALIZED;
    };

    // Caches what should be added to m_timingDomains because of calls to the AstCFunc (with
    // recursive check of other AstCFuncs called from inside)
    AstUser1Allocator<AstCFunc, CFuncCache> m_cfuncsCache;
    AstUser1Allocator<AstVarScope, VarScopeInfo> m_vscpInfo;
    AstUser3Allocator<AstVarScope, std::vector<WriteReference>> m_writeRefs;

    // STATE - across all visitors
    VInsertionSet<Ast_Sen_वृक्ष*> m_timingDomains;  // Timing resume domains
    V3SharedTmps m_dlyTmps{"__Vdly", VVarType::BLOCKTEMP};  // Temporary variables
    // Commit queue data types, by element type and partial flag. Shared by all instances,
    // as m_dlyTmps only shares variables with the same data type.
    std::map<std::pair<स्थिरांक Ast_गाठ_DType*, तार्किक>, AstNBACommitQueueDType*> m_cqDTypeps;

    स्थिरांक std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::vector<स्थिरांक AstCFunc*> m_call_चळत;  // Current callstack of AstCFuncs

    // STATE - for current visit position (use VL_RESTORER)
    AstActive* m_activep = nullptr;  // Current activate
    स्थिरांक AstCFunc* m_cfuncp = nullptr;  // Current public C Function
    Ast_गाठ_Procedure* m_procp = nullptr;  // Current process
    AstScope* m_scopep = nullptr;  // Current scope
    तार्किक m_inLoop = असत्य;  // True in for loops
    तार्किक m_inSuspendableOrFork = असत्य;  // True in suspendable processes and forks
    तार्किक m_ignoreBlkAndNBlk = असत्य;  // Suppress delayed assignment BLKANDNBLK
    तार्किक m_inNonCombLogic = असत्य;  // We are in non-combinational logic
    तार्किक m_needsInitialTrigger = असत्य;  // Whether a NodeProcedure needs a initial trigger
    std::vector<Ast_Sen_वृक्ष*> m_nbaEventSenTreeps;  // Sensitivities of '->>' in the process
    AstVarRef* m_currNbaLhsRefp = nullptr;  // Current NBA LHS variable reference

    // STATE - during NBA conversion (after visit)
    std::vector<NBA> m_nbas;  // AstAssignDly instances to lower at the end
    std::vector<AstVarScope*> m_vscps;  // Target variables on LHSs of NBAs
    AstAssignDly* m_nextDlyp = nullptr;  // The nextp of the previous AstAssignDly
    AstVarScope* m_prevVscp = nullptr;  // The target of the previous AstAssignDly

    // STATE - Statistic tracking
    VDouble0 m_nSchemeShadowVar;  // Number of variables using Scheme::ShadowVar
    VDouble0 m_nSchemeShadowVarMasked;  // Number of variabels using Scheme::ShadowVarMasked
    VDouble0 m_nSchemeFlagShared;  // Number of variables using Scheme::FlagShared
    VDouble0 m_nSchemeFlagUnique;  // Number of variables using Scheme::FlagUnique
    VDouble0 m_n_Scheme_मूल्य_Queues_Whole;  //  Number of variables using Scheme::ValueQueueWhole
    VDouble0 m_n_Scheme_मूल्य_Queues_Partial;  //  Number of variables using Scheme::ValueQueuePartial
    VDouble0 m_nSharedSetFlags;  // "Set" flags actually shared by Scheme::FlagShared variables
    VDouble0 m_nInitialNBA;  // Number of procedural blocks with initial NBA
    VDouble0 m_non_Inlined_CAwaits_With_Sen_वृक्ष;  // Count uses of not inlined co_awaits

    // METHODS

    // Return true iff a variable is assigned by both blocking and nonblocking
    // assignments. Issue BLKANDNBLK error if we can't prove the mixed
    // assignments are to independent bits and the blocking assignment can be
    // in combinational logic, which is something we can't safely implement
    // still.
    तार्किक checkMixedUsage(स्थिरांक AstVarScope* vscp, तार्किक isIntegralOrPacked) {

        संरचना Ref final {
            AstVarRef* m_refp;  // The reference
            तार्किक m_inNonComb;  // True if known to be in non-combinational logic
            पूर्णांक m_lsb;  // LSB of accessed range
            पूर्णांक m_msb;  // MSB of accessed range
            Ref(AstVarRef* refp, तार्किक inNonComb, पूर्णांक lsb, पूर्णांक msb)
                : m_refp{refp}
                , m_inNonComb{inNonComb}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };

        std::vector<Ref> blkRefs;  // Blocking writes
        std::vector<Ref> nbaRefs;  // Non-blockign writes

        स्थिरांक पूर्णांक width = isIntegralOrPacked ? vscp->width() : 1;

        साठी (स्थिरांक स्वतः& writeRef : m_writeRefs(vscp)) {
            पूर्णांक lsb = 0;
            पूर्णांक msb = width - 1;
            जर (स्थिरांक AstSel* स्थिरांक selp = VN_CAST(writeRef.m_refp->backp(), Sel)) {
                जर (VN_IS(selp->lsbp(), Const)) {
                    lsb = selp->lsbConst();
                    msb = selp->msbConst();
                }
            }
            जर (writeRef.m_isNBA) {
                nbaRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            } अन्यथा {
                blkRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            }
        }
        // We only run this function on targets of NBAs, so there should be at least one...
        UASSERT_OBJ(!nbaRefs.empty(), vscp, "Did not record NBA write");
        // If no blocking upadte, then we are good
        जर (blkRefs.empty()) परत असत्य;

        // If the blocking assignment is in non-combinational logic (i.e.:
        // in logic that has an explicit trigger), then we can safely
        // implement it (there is no race between clocked logic and post
        // scheduled logic), so need not error
        blkRefs.erase(std::remove_if(blkRefs.begin(), blkRefs.end(),
                                     [](स्थिरांक Ref& ref) { परत ref.m_inNonComb; }),
                      blkRefs.end());

        // If nothing left, then we need not error
        जर (blkRefs.empty()) परत सत्य;

        // If not a packed variable, warn here as we can't prove independence
        जर (!isIntegralOrPacked) {
            स्थिरांक Ref& blkRef = blkRefs.front();
            स्थिरांक Ref& nbaRef = nbaRefs.front();
            vscp->v3warn(
                BLKANDNBLK,
                "Unsupported: Blocking and non-blocking assignments to same non-packed variable: "
                    << vscp->varp()->prettyNameQ() << '\n'
                    << vscp->warnContextPrimary() << '\n'
                    << blkRef.m_refp->warnOther() << "... Location of blocking assignment\n"
                    << blkRef.m_refp->warnContextSecondary() << '\n'
                    << nbaRef.m_refp->warnOther() << "... Location of nonblocking assignment\n"
                    << nbaRef.m_refp->warnContextSecondary());
            परत सत्य;
        }

        // We need to error if we can't prove the written bits are independent

        // Sort refs by interval
        स्थिरांक स्वतः lessThanRef = [](स्थिरांक Ref& a, स्थिरांक Ref& b) {
            जर (a.m_lsb != b.m_lsb) परत a.m_lsb < b.m_lsb;
            परत a.m_msb < b.m_msb;
        };
        std::stable_sort(blkRefs.begin(), blkRefs.end(), lessThanRef);
        std::stable_sort(nbaRefs.begin(), nbaRefs.end(), lessThanRef);
        // Iterate both vectors, checking for overlap
        स्वतः bIt = blkRefs.begin();
        स्वतः nIt = nbaRefs.begin();
        पर्यंत (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            जर (lessThanRef(*bIt, *nIt)) {
                जर (nIt->m_lsb <= bIt->m_msb) थांबा;  // Stop on Overlap
                ++bIt;
            } अन्यथा {
                जर (bIt->m_lsb <= nIt->m_msb) थांबा;  // Stop on Overlap
                ++nIt;
            }
        }

        // If we found an overlapping range that cannot be safely implemented, then wran...
        जर (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            स्थिरांक Ref& blkRef = *bIt;
            स्थिरांक Ref& nbaRef = *nIt;
            vscp->v3warn(BLKANDNBLK, "Unsupported: Blocking and non-blocking assignments to "
                                     "potentially overlapping bits of same packed variable: "
                                         << vscp->varp()->prettyNameQ() << '\n'
                                         << vscp->warnContextPrimary() << '\n'
                                         << blkRef.m_refp->warnOther()
                                         << "... Location of blocking assignment" << " (bits ["
                                         << blkRef.m_msb << ":" << blkRef.m_lsb << "])\n"
                                         << blkRef.m_refp->warnContextSecondary() << '\n'
                                         << nbaRef.m_refp->warnOther()
                                         << "... Location of nonblocking assignment" << " (bits ["
                                         << nbaRef.m_msb << ":" << nbaRef.m_lsb << "])\n"
                                         << nbaRef.m_refp->warnContextSecondary());
        }

        परत सत्य;
    }

    // Choose the NBA scheme used for the given variable.
    Scheme chooseScheme(स्थिरांक AstVarScope* vscp, स्थिरांक VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::Undecided, vscp, "NBA scheme already decided");

        स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep = vscp->dtypep()->skipRefp();
        // Unpacked arrays
        जर (स्थिरांक AstUnpackArrayDType* स्थिरांक uaDTypep = VN_CAST(dtypep, UnpackArrayDType)) {
            // If whole array is target of NBA, use ShadowVar, but not in a suspendable process or
            // fork, which resume outside the 'nba' region, so its 'pre' logic would overwrite the
            // value they write in the shadow variable
            जर (vscpInfo.m_whole) {
                परत vscpInfo.m_inSuspOrFork ? Scheme::FlagUnique : Scheme::ShadowVar;
            }
            // Basic underlying type of elements, if any.
            स्थिरांक AstBasicDType* स्थिरांक basicp = uaDTypep->basicp();
            // If used in a loop, we must have a dynamic commit queue. (Also works in suspendables)
            जर (vscpInfo.m_inLoop) {
                // Arrays with compound element types are currently not supported in loops
                जर (!basicp
                    || !(basicp->isIntegralOrPacked()  //
                         || basicp->isDouble()  //
                         || basicp->isString())) {
                    परत Scheme::UnsupportedCompoundArrayInLoop;
                }
                जर (vscpInfo.m_partial) परत Scheme::मूल्य_Queue_Partial;
                परत Scheme::मूल्य_Queue_Whole;
            }
            // A suspendable process or fork can resume outside the 'nba' region, so the 'pre'
            // logic of the other schemes would undo its updates. The unique flag scheme has none.
            जर (vscpInfo.m_inSuspOrFork) परत Scheme::FlagUnique;
            // Otherwise if an array of packed/basic elements, use the shared flag scheme
            जर (basicp) परत Scheme::FlagShared;
            // Finally fall back on the shadow variable scheme, e.g. for
            // arrays of unpacked structs. This will be slow.
            // TODO: generic LHS scheme as discussed in #5092
            परत Scheme::ShadowVar;
        }

        // Likewise, the 'pre' logic of the shadow variable scheme would undo the updates of a
        // suspendable process or fork, so use the unique flag scheme
        जर (vscpInfo.m_inSuspOrFork) परत Scheme::FlagUnique;

        स्थिरांक तार्किक isIntegralOrPacked = dtypep->isIntegralOrPacked();
        // Check for mixed usage (this also warns if not OK)
        जर (checkMixedUsage(vscp, isIntegralOrPacked)) {
            // If it's a variable updated by both blocking and non-blocking
            // assignments, use the ShadowVarMasked schem if masked update is
            // possible. This can handle blocking and non-blocking updates to
            // inpdendent parts correctly at run-time, and always works, even
            // in loops or other dynamic context.
            जर (isIntegralOrPacked) परत Scheme::ShadowVarMasked;
            // If it's inside a loop, use Scheme::ShadowVar, which is safe,
            // but will generate incorrect code if a partial update is used
            जर (vscpInfo.m_inLoop) परत Scheme::ShadowVar;
            // Otherwise (for not packed variables), use the FlagUnique scheme,
            // which at least handles partial updates correctly, but might break
            // in loops or other dynamic context
            परत Scheme::FlagUnique;
        }

        // Otherwise use the simple shadow variable scheme
        परत Scheme::ShadowVar;
    }

    // Given an expression 'exprp', return a new expression that always evaluates to the
    // value of the given expression at this point in the program. That is:
    // - If given a non-constant expression, create a new temporary AstVarScope under the given
    //   'scopep', with the given 'name', assign the expression to it, and return a read reference
    //   to the new AstVarScope.
    // - If given a constant, just return that constant.
    // New statements are inserted before 'insertp'
    Ast_गाठ_अभिव्यक्ती* captureVal(AstScope* स्थिरांक scopep, Ast_गाठ_Stmt* स्थिरांक insertp,
                            Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp, स्थिरांक std::string& name) {
        UASSERT_OBJ(!exprp->backp(), exprp, "Should have been unlinked");
        File_ओळ* स्थिरांक flp = exprp->fileline();
        जर (VN_IS(exprp, Const)) परत exprp;
        // TODO: there are some const variables that could be simply referenced here
        AstVarScope* स्थिरांक tmpVscp = m_dlyTmps.make(flp, scopep, exprp->dtypep(), name);
        insertp->addHereThisAsNext(
            नवीन AstAssign{flp, नवीन AstVarRef{flp, tmpVscp, VAccess::WRITE}, exprp});
        परत नवीन AstVarRef{flp, tmpVscp, VAccess::READ};
    };

    // Similar to 'captureVal', but captures an LValue expression. That is, the returned
    // expression will reference the same location as the input expression, at this point in the
    // program.
    Ast_गाठ_अभिव्यक्ती* captureLhs(AstScope* स्थिरांक scopep, Ast_गाठ_Stmt* स्थिरांक insertp,
                            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp, स्थिरांक std::string& baseName) {
        UASSERT_OBJ(!lhsp->backp(), lhsp, "Should have been unlinked");
        // Running node pointer
        Ast_गाठ_अभिव्यक्ती* nodep = lhsp;
        // Capture AstSel indices - there should be only one
        जर (AstSel* स्थिरांक selp = VN_CAST(nodep, Sel)) {
            स्थिरांक std::string tmpName{"Lsb" + baseName};
            selp->lsbp(captureVal(scopep, insertp, selp->lsbp()->unlinkFrBack(), tmpName));
            // Continue with target
            nodep = selp->fromp();
        }
        UASSERT_OBJ(!VN_IS(nodep, Sel), lhsp, "Multiple 'AstSel' applied to LHS reference");
        // Capture the indices of selects - might be many, also below struct members. If a handle
        // selects the instance the target is a member of, capture the handle instead, as it is
        // only read, to select the target when the NBA is executed (IEEE 1800-2023 10.4.2).
        माप_प्रकार nArraySels = 0;
        पर्यंत (सत्य) {
            जर (Ast_गाठ_Sel* स्थिरांक selp = VN_CAST(nodep, गाठ_Sel)) {
                स्थिरांक std::string tmpName{"Dim" + std::to_string(nArraySels++) + baseName};
                selp->bitp(captureVal(scopep, insertp, selp->bitp()->unlinkFrBack(), tmpName));
                nodep = selp->fromp();
            } अन्यथा जर (स्थिरांक AstStructSel* स्थिरांक selp = VN_CAST(nodep, StructSel)) {
                nodep = selp->fromp();
            } अन्यथा जर (AstMemberSel* स्थिरांक selp = VN_CAST(nodep, MemberSel)) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक handlep = selp->fromp()->unlinkFrBack();
                V3LinkLValue::linkLValueSet(handlep, VAccess::READ);
                AstVarRef* स्थिरांक refp
                    = VN_AS(captureVal(scopep, insertp, handlep, "Handle" + baseName), VarRef);
                refp->varp()->setIgnorePostWrite();
                // Like the handle it replaces, mark it written, as is the member it selects
                refp->access(selp->access());
                selp->fromp(refp);
                परत lhsp;
            } अन्यथा {
                थांबा;
            }
        }
        // What remains must be the variable, or some sort of select, we assume can reuse it
        UASSERT_OBJ(nodep->isPure(), lhsp, "Malformed LHS in NBA");
        // Now have been converted to use the captured values
        परत lhsp;
    }

    शून्य add_CFunc_Cached_मूल्ये(स्थिरांक AstCFunc* स्थिरांक cfuncp,
                              std::unordered_set<स्थिरांक AstCFunc*>& visited) {
        जर (!visited.घाला(cfuncp).second) परत;
        CFuncCache& मूल्य = m_cfuncsCache(cfuncp);
        m_timingDomains.घाला(मूल्य.m_timingDomains.begin(), मूल्य.m_timingDomains.end());
        m_non_Inlined_CAwaits_With_Sen_वृक्ष += मूल्य.m_timingDomains.size();
        साठी (स्थिरांक AstCFunc* स्थिरांक includedp : मूल्य.m_includes) {
            add_CFunc_Cached_मूल्ये(includedp, visited);
        }
    }

    // Create a temporary variable in the given 'scopep', with the given 'name', and with 'dtypep'
    // type, with the bits selected by 'sLsbp'/'sWidthp' set to 'valuep', other bits set to zero.
    // Insert new statements before 'insertp'.
    // Returns a read reference to the temporary variable.
    AstVarRef* createWidened(File_ओळ* flp, AstScope* scopep, Ast_गाठ_DType* dtypep,
                             Ast_गाठ_अभिव्यक्ती* sLsbp, पूर्णांक sWidth, स्थिरांक std::string& name,
                             Ast_गाठ_अभिव्यक्ती* valuep, Ast_गाठ* insertp) {
        // Create temporary variable.
        AstVarScope* स्थिरांक tp = m_dlyTmps.make(flp, scopep, dtypep, name);
        // Zero it
        AstConst* स्थिरांक zerop = नवीन AstConst{flp, AstConst::DTyped{}, dtypep};
        zerop->अंक().setAllBits0();
        insertp->addHereThisAsNext(
            नवीन AstAssign{flp, नवीन AstVarRef{flp, tp, VAccess::WRITE}, zerop});
        // Set the selected bits to 'valuep'
        AstSel* स्थिरांक selp = नवीन AstSel{flp, नवीन AstVarRef{flp, tp, VAccess::WRITE},
                                        sLsbp->clone_वृक्ष_Pure(सत्य), sWidth};
        insertp->addHereThisAsNext(नवीन AstAssign{flp, selp, valuep});
        // This is the expression to get the value of the temporary
        परत नवीन AstVarRef{flp, tp, VAccess::READ};
    }

    // Scheme::ShadowVar
    शून्य prepareSchemeShadowVar(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        File_ओळ* स्थिरांक flp = vscp->fileline();
        AstScope* स्थिरांक scopep = vscp->scopep();
        // Create the shadow variable
        स्थिरांक std::string name = "_" + vscp->varp()->shortName();
        AstVarScope* स्थिरांक shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), name);
        vscpInfo.shadowVariableKit().vscp = shadowVscp;
        // Mark both for V3LifePsot
        vscp->optimizeLifePost(सत्य);
        shadowVscp->optimizeLifePost(सत्य);
        // Create the AstActive for the Pre/Post logic
        AstActive* स्थिरांक activep = नवीन AstActive{flp, "nba-shadow-variable", vscpInfo.senTreep()};
        activep->sen_वृक्ष_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Pre' scheduled 'shadowVariable = originalVariable' assignment
        AstAlwaysPre* स्थिरांक prep = नवीन AstAlwaysPre{flp};
        activep->addStmtsp(prep);
        prep->addStmtsp(नवीन AstAssign{flp, नवीन AstVarRef{flp, shadowVscp, VAccess::WRITE},
                                      नवीन AstVarRef{flp, vscp, VAccess::READ}});
        // Add 'Post' scheduled 'originalVariable = shadowVariable' assignment
        AstAlwaysPost* स्थिरांक postp = नवीन AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        postp->addStmtsp(नवीन AstAssign{flp, नवीन AstVarRef{flp, vscp, VAccess::WRITE},
                                       नवीन AstVarRef{flp, shadowVscp, VAccess::READ}});
    }
    शून्य convertSchemeShadowVar(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        AstVarScope* स्थिरांक shadowVscp = vscpInfo.shadowVariableKit().vscp;

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* स्थिरांक refp) {
            जर (!refp->access().isWriteOnly()) परत;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
    }

    // Scheme::ShadowVarMasked
    शून्य prepareSchemeShadowVarMasked(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        File_ओळ* स्थिरांक flp = vscp->fileline();
        AstScope* स्थिरांक scopep = vscp->scopep();
        // Create the shadow variable
        स्थिरांक std::string shadowName = "_" + vscp->varp()->shortName();
        AstVarScope* स्थिरांक shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), shadowName);
        vscpInfo.shadowVarMaskedKit().vscp = shadowVscp;
        // Create the makk variable
        स्थिरांक std::string maskName = "Mask__" + vscp->varp()->shortName();
        AstVarScope* स्थिरांक maskVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), maskName);
        maskVscp->varp()->setIgnorePostWrite();
        vscpInfo.shadowVarMaskedKit().maskp = maskVscp;
        // Create the AstActive for the Post logic
        AstActive* स्थिरांक activep
            = नवीन AstActive{flp, "nba-shadow-var-masked", vscpInfo.senTreep()};
        activep->sen_वृक्ष_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit and mask clear
        AstAlwaysPost* स्थिरांक postp = नवीन AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit - vscp = (shadowVscp & maskVscp) | (vscp & ~maskVscp);
        postp->addStmtsp(नवीन AstAssign{
            flp, नवीन AstVarRef{flp, vscp, VAccess::WRITE},
            नवीन AstOr{flp,
                      नवीन AstAnd{flp, नवीन AstVarRef{flp, shadowVscp, VAccess::READ},
                                 नवीन AstVarRef{flp, maskVscp, VAccess::READ}},
                      नवीन AstAnd{flp, नवीन AstVarRef{flp, vscp, VAccess::READ},
                                 नवीन AstNot{flp, नवीन AstVarRef{flp, maskVscp, VAccess::READ}}}}});
        vscp->varp()->setIgnorePostRead();
        // Clar the mask - maskVscp = '0;
        postp->addStmtsp(
            नवीन AstAssign{flp, नवीन AstVarRef{flp, maskVscp, VAccess::WRITE},
                          नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, maskVscp->width(), 0}});
    }
    शून्य convertSchemeShadowVarMasked(AstAssignDly* nodep, AstVarScope* vscp,
                                      VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        AstVarScope* स्थिरांक shadowVscp = vscpInfo.shadowVarMaskedKit().vscp;
        AstVarScope* स्थिरांक maskVscp = vscpInfo.shadowVarMaskedKit().maskp;

        Ast_गाठ_अभिव्यक्ती* lhsClonep = nodep->lhsp()->clone_वृक्ष(असत्य);

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* स्थिरांक refp) {
            जर (!refp->access().isWriteOnly()) परत;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
        // Set the same bits in the mask to 1
        lhsClonep->foreach([&](AstVarRef* स्थिरांक refp) {
            जर (!refp->access().isWriteOnly()) परत;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(maskVscp);
            refp->varp(maskVscp->varp());
        });
        File_ओळ* स्थिरांक flp = nodep->fileline();
        AstConst* स्थिरांक onesp = नवीन AstConst{flp, AstConst::DTyped{}, lhsClonep->dtypep()};
        onesp->अंक().setAllBits1();
        nodep->addNextHere(नवीन AstAssign{flp, lhsClonep, onesp});
    }

    // Scheme::FlagShared
    शून्य prepareSchemeFlagShared(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_ओळ* स्थिरांक flp = vscp->fileline();
        AstScope* स्थिरांक scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* स्थिरांक activep = नवीन AstActive{flp, "nba-flag-shared", vscpInfo.senTreep()};
        activep->sen_वृक्ष_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        vscpInfo.flagSharedKit().activep = activep;
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* स्थिरांक postp = नवीन AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagSharedKit().postp = postp;
        // Initialize
        vscpInfo.flagSharedKit().commitFlagp = nullptr;
        vscpInfo.flagSharedKit().commitIfp = nullptr;
    }
    शून्य convertSchemeFlagShared(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_ओळ* स्थिरांक flp = vscp->fileline();
        AstScope* स्थिरांक scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        स्थिरांक std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_गाठ_अभिव्यक्ती* स्थिरांक capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_गाठ_अभिव्यक्ती* स्थिरांक capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Is this NBA adjacent after the previously processed NBA?
        स्थिरांक तार्किक consecutive = nodep == m_nextDlyp;
        m_nextDlyp = VN_CAST(nodep->nextp(), AssignDly);

        VarScopeInfo* स्थिरांक prevVscpInfop = consecutive ? &m_vscpInfo(m_prevVscp) : nullptr;

        // We can reuse the flag of the previous assignment if:
        स्थिरांक तार्किक reuseTheFlag =
            // Consecutive NBAs
            consecutive
            // ... that use the same scheme
            && prevVscpInfop->m_scheme == Scheme::FlagShared
            // ... and are in the same scope as the target variable
            && scopep == vscp->scopep()
            // ... and share the same overall update domain
            && prevVscpInfop->senTreep()->same_वृक्ष(vscpInfo.senTreep());

        जर (!reuseTheFlag) {
            // Create new flag
            AstVarScope* स्थिरांक flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
            // Set the flag at the original NBA
            nodep->addHereThisAsNext(  //
                नवीन AstAssign{flp, नवीन AstVarRef{flp, flagVscp, VAccess::WRITE},
                              नवीन AstConst{flp, AstConst::BitTrue{}}});
            // Add the 'Pre' scheduled reset for the flag
            AstAlwaysPre* स्थिरांक prep = नवीन AstAlwaysPre{flp};
            vscpInfo.flagSharedKit().activep->addStmtsp(prep);
            prep->addStmtsp(नवीन AstAssign{flp, नवीन AstVarRef{flp, flagVscp, VAccess::WRITE},
                                          नवीन AstConst{flp, AstConst::BitFalse{}}});
            // Add the 'Post' scheduled commit
            AstIf* स्थिरांक ifp = नवीन AstIf{flp, नवीन AstVarRef{flp, flagVscp, VAccess::READ}};
            vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
            vscpInfo.flagSharedKit().commitFlagp = flagVscp;
            vscpInfo.flagSharedKit().commitIfp = ifp;
        } अन्यथा {
            जर (vscp != m_prevVscp) {
                // Different variable, ensure the commit block exists for this variable,
                // can reuse existing one with the same flag, otherwise create a new one.
                AstVarScope* स्थिरांक flagVscp = prevVscpInfop->flagSharedKit().commitFlagp;
                UASSERT_OBJ(flagVscp, nodep, "Commit flag of previous assignment should exist");
                जर (vscpInfo.flagSharedKit().commitFlagp != flagVscp) {
                    AstIf* स्थिरांक ifp = नवीन AstIf{flp, नवीन AstVarRef{flp, flagVscp, VAccess::READ}};
                    vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
                    vscpInfo.flagSharedKit().commitFlagp = flagVscp;
                    vscpInfo.flagSharedKit().commitIfp = ifp;
                }
            } अन्यथा {
                // Same variable, reuse the commit block of the previous assignment
                vscpInfo.flagSharedKit().commitFlagp = prevVscpInfop->flagSharedKit().commitFlagp;
                vscpInfo.flagSharedKit().commitIfp = prevVscpInfop->flagSharedKit().commitIfp;
            }
            ++m_nSharedSetFlags;
        }
        // Commit the captured value to the captured destination
        vscpInfo.flagSharedKit().commitIfp->addThensp(
            नवीन AstAssign{flp, capturedLhsp, capturedRhsp});

        // Remember the variable for the next NBA
        m_prevVscp = vscp;

        // Delete original NBA
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::FlagUnique
    शून्य prepareSchemeFlagUnique(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_ओळ* स्थिरांक flp = vscp->fileline();
        AstScope* स्थिरांक scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* स्थिरांक activep = नवीन AstActive{flp, "nba-flag-unique", vscpInfo.senTreep()};
        activep->sen_वृक्ष_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* स्थिरांक postp = नवीन AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagUniqueKit().postp = postp;
    }
    शून्य convertSchemeFlagUnique(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_ओळ* स्थिरांक flp = vscp->fileline();
        AstScope* स्थिरांक scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        स्थिरांक std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_गाठ_अभिव्यक्ती* स्थिरांक capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_गाठ_अभिव्यक्ती* स्थिरांक capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Create new flag
        AstVarScope* स्थिरांक flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
        flagVscp->varp()->setIgnorePostWrite();
        // Set the flag at the original NBA
        nodep->addHereThisAsNext(  //
            नवीन AstAssign{flp, नवीन AstVarRef{flp, flagVscp, VAccess::WRITE},
                          नवीन AstConst{flp, AstConst::BitTrue{}}});
        // Add the 'Post' scheduled commit
        AstIf* स्थिरांक ifp = नवीन AstIf{flp, नवीन AstVarRef{flp, flagVscp, VAccess::READ}};
        vscpInfo.flagUniqueKit().postp->addStmtsp(ifp);
        // Immediately clear the flag
        ifp->addThensp(नवीन AstAssign{flp, नवीन AstVarRef{flp, flagVscp, VAccess::WRITE},
                                     नवीन AstConst{flp, AstConst::BitFalse{}}});
        // Commit the value
        ifp->addThensp(नवीन AstAssign{flp, capturedLhsp, capturedRhsp});

        // Delete original NBA
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::ValueQueuePartial/Scheme::ValueQueueWhole
    ढाँचा <तार्किक N_Partial>
    शून्य prepare_Scheme_मूल्य_Queue(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(N_Partial ? vscpInfo.m_scheme == Scheme::मूल्य_Queue_Partial
                              : vscpInfo.m_scheme == Scheme::मूल्य_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_ओळ* स्थिरांक flp = vscp->fileline();
        AstScope* स्थिरांक scopep = vscp->scopep();

        // Create the commit queue variable
        Ast_गाठ_DType* स्थिरांक elemDTypep = vscp->dtypep()->skipRefp();
        AstNBACommitQueueDType*& cqDTypep = m_cqDTypeps[{elemDTypep, N_Partial}];
        जर (!cqDTypep) {
            cqDTypep = नवीन AstNBACommitQueueDType{flp, elemDTypep, N_Partial};
            v3Global.rootp()->typeTablep()->addTypesp(cqDTypep);
        }
        स्थिरांक std::string name = "CommitQueue__" + vscp->varp()->shortName();
        AstVarScope* स्थिरांक queueVscp = m_dlyTmps.make(flp, scopep, cqDTypep, name);
        queueVscp->varp()->noReset(सत्य);
        queueVscp->varp()->setIgnorePostWrite();
        vscpInfo.मूल्य_Queue_Kit().vscp = queueVscp;
        // Create the AstActive for the Post logic
        AstActive* स्थिरांक activep
            = नवीन AstActive{flp, "nba-value-queue-whole", vscpInfo.senTreep()};
        activep->sen_वृक्ष_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit
        AstAlwaysPost* स्थिरांक postp = नवीन AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit
        AstCMethodHard* स्थिरांक callp = नवीन AstCMethodHard{
            flp, नवीन AstVarRef{flp, queueVscp, VAccess::READWRITE}, VCMethod::NBA_COMMIT};
        callp->dtypeSetVoid();
        // TODO: this is a partial update, so must be READWRITE, but that breaks scheduling
        callp->addPinsp(नवीन AstVarRef{flp, vscp, VAccess::WRITE});
        postp->addStmtsp(callp->makeStmt());
    }

    शून्य convert_Scheme_मूल्य_Queue(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo,
                                 तार्किक partial) {
        UASSERT_OBJ(partial ? vscpInfo.m_scheme == Scheme::मूल्य_Queue_Partial
                            : vscpInfo.m_scheme == Scheme::मूल्य_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_ओळ* स्थिरांक flp = vscp->fileline();
        AstScope* स्थिरांक scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        स्थिरांक std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_गाठ_अभिव्यक्ती* स्थिरांक capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_गाठ_अभिव्यक्ती* स्थिरांक capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // RHS value (can be widened/masked iff Partial)
        Ast_गाठ_अभिव्यक्ती* valuep = capturedRhsp;
        // RHS mask (iff Partial)
        Ast_गाठ_अभिव्यक्ती* maskp = nullptr;

        // Running node for LHS deconstruction
        Ast_गाठ_अभिव्यक्ती* lhsNodep = capturedLhsp;

        // If partial updates are required, construct the mask and the widened value
        जर (partial) {
            // Type of array element
            Ast_गाठ_DType* स्थिरांक eDTypep = [&]() -> Ast_गाठ_DType* {
                Ast_गाठ_DType* dtypep = vscp->dtypep()->skipRefp();
                पर्यंत (AstUnpackArrayDType* स्थिरांक ap = VN_CAST(dtypep, UnpackArrayDType)) {
                    dtypep = ap->subDTypep()->skipRefp();
                }
                परत dtypep;
            }();

            जर (स्थिरांक AstSel* स्थिरांक lSelp = VN_CAST(lhsNodep, Sel)) {
                // This is a partial assignment.
                // Need to create a mask and widen the value to element size.
                lhsNodep = lSelp->fromp();
                Ast_गाठ_अभिव्यक्ती* स्थिरांक sLsbp = lSelp->lsbp();
                स्थिरांक पूर्णांक sWidth = lSelp->widthConst();

                // Create mask value
                maskp = [&]() -> Ast_गाठ_अभिव्यक्ती* {
                    // Constant mask we can compute here
                    जर (स्थिरांक AstConst* स्थिरांक cLsbp = VN_CAST(sLsbp, Const)) {
                        AstConst* स्थिरांक cp = नवीन AstConst{flp, AstConst::DTyped{}, eDTypep};
                        cp->अंक().setMask(sWidth, cLsbp->toSInt());
                        परत cp;
                    }

                    // A non-constant mask we must compute at run-time.
                    AstConst* स्थिरांक onesp = नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, sWidth, 0};
                    onesp->अंक().setAllBits1();
                    परत createWidened(flp, scopep, eDTypep, sLsbp, sWidth, "Mask" + baseName,
                                         onesp, nodep);
                }();

                // Adjust value to element size
                valuep = [&]() -> Ast_गाठ_अभिव्यक्ती* {
                    // Constant value with constant select we can compute here
                    जर (AstConst* स्थिरांक cValuep = VN_CAST(valuep, Const)) {
                        जर (स्थिरांक AstConst* स्थिरांक cLsbp = VN_CAST(sLsbp, Const)) {
                            AstConst* स्थिरांक cp = नवीन AstConst{flp, AstConst::DTyped{}, eDTypep};
                            cp->अंक().setAllBits0();
                            cp->अंक().opSelInto(cValuep->अंक(), cLsbp->toSInt(), sWidth);
                            VL_DO_DANGLING(valuep->delete_वृक्ष(), valuep);
                            परत cp;
                        }
                    }

                    // A non-constant value we must adjust.
                    परत createWidened(flp, scopep, eDTypep, sLsbp, sWidth,  //
                                         "Elem" + baseName, valuep, nodep);
                }();
            } अन्यथा {
                // If this assignment is not partial, set mask to ones and we are done
                AstConst* स्थिरांक ones = नवीन AstConst{flp, AstConst::DTyped{}, eDTypep};
                ones->अंक().setAllBits1();
                maskp = ones;
            }
        }

        // Extract array indices
        std::vector<Ast_गाठ_अभिव्यक्ती*> idxps;
        {
            UASSERT_OBJ(VN_IS(lhsNodep, ArraySel), lhsNodep, "Unexpected LHS form");
            पर्यंत (AstArraySel* स्थिरांक aSelp = VN_CAST(lhsNodep, ArraySel)) {
                idxps.emplace_back(aSelp->bitp()->unlinkFrBack());
                lhsNodep = aSelp->fromp();
            }
            UASSERT_OBJ(VN_IS(lhsNodep, VarRef), lhsNodep, "Unexpected LHS form");
            std::reverse(idxps.begin(), idxps.end());
        }

        // Done with the LHS at this point
        VL_DO_DANGLING(ढकला_Deletep(capturedLhsp), capturedLhsp);

        // Enqueue the update at the site of the original NBA
        AstCMethodHard* स्थिरांक callp = नवीन AstCMethodHard{
            flp, नवीन AstVarRef{flp, vscpInfo.मूल्य_Queue_Kit().vscp, VAccess::READWRITE},
            VCMethod::NBA_ENQUEUE};
        callp->dtypeSetVoid();
        callp->addPinsp(valuep);
        जर (partial) callp->addPinsp(maskp);
        साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक indexp : idxps) callp->addPinsp(indexp);
        nodep->addHereThisAsNext(callp->makeStmt());

        // Delete original NBA
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Record where a variable is assigned
    शून्य recordWriteRef(AstVarRef* nodep, तार्किक nonBlocking) {
        // Ignore references in certain contexts
        जर (m_ignoreBlkAndNBlk) परत;
        // Ignore if it's an array
        // TODO: we do this because it used to be the previous behaviour.
        //       Is it still required, or should we warn for arrays as well?
        //       Scheduling is no different for them...
        //       Clarification: This is OK for arrays of primitive types, but
        //       arrays that use the ShadowVar scheme don't work...
        जर (VN_IS(nodep->varScopep()->dtypep()->skipRefp(), UnpackArrayDType)) परत;

        m_writeRefs(nodep->varScopep()).emplace_back(nodep, nonBlocking, m_inNonCombLogic);
    }

    ढाँचा <typename Procedure_T>
    स्थिर तार्किक isProcedureWithSentreep(स्थिरांक Ast_गाठ_Procedure* स्थिरांक nodep) {
        स्थिरांक Procedure_T* स्थिरांक procedurep = Ast_गाठ::cast<Procedure_T>(nodep);
        परत procedurep && procedurep->sentreep();
    }

    // Visit AstCFunc from a AstNodeCCall - this is made into a separate quasi-visitor because
    // AstCFunc that is not called from the code (e.g.: DPI exports) does not need to be visited
    // this way. Also, not visiting such AstCFuncs allows to avoid caching results for them which
    // this function does - which could lead to excessive memory usage
    शून्य visitCalledCFunc(AstCFunc* स्थिरांक nodep) {
        CFuncCache& मूल्य = m_cfuncsCache(nodep);
        निवड (मूल्य.m_state) {
        स्थिती CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_timingDomains);

            // Visit
            मूल्य.m_state = CFuncCache::VISITING;
            m_call_चळत.ढकला_back(nodep);
            {
                VL_RESTORER(m_cfuncp);
                m_cfuncp = nodep;
                iterateChildren(nodep);
            }
            m_call_चळत.काढा_back();
            मूल्य.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_timingDomains, मूल्य.m_timingDomains);
        } थांबा;
        स्थिती CFuncCache::VISITING: {
            साठी (माप_प्रकार i = m_call_चळत.size() - 1; m_call_चळत.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_चळत[i]).m_includes.घाला(nodep);
            }
            परत;  // Break recursion
        }
        स्थिती CFuncCache::INITIALIZED: थांबा;
        }
        std::unordered_set<स्थिरांक AstCFunc*> visited;
        add_CFunc_Cached_मूल्ये(nodep, visited);
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {
        iterateChildren(nodep);
        // Decide which scheme to use for each variable and do the 'prepare' step
        साठी (AstVarScope* स्थिरांक vscp : m_vscps) {
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            vscpInfo.m_scheme = chooseScheme(vscp, vscpInfo);
            // Run 'prepare' step
            निवड (vscpInfo.m_scheme) {
            स्थिती Scheme::Undecided:  // LCOV_EXCL_START
                UASSERT_OBJ(असत्य, vscp, "Failed to choose NBA scheme");
                थांबा;  // LCOV_EXCL_STOP
            स्थिती Scheme::UnsupportedCompoundArrayInLoop: {
                // Will report error at the site of the NBA
                थांबा;
            }
            स्थिती Scheme::ShadowVar: {
                ++m_nSchemeShadowVar;
                prepareSchemeShadowVar(vscp, vscpInfo);
                थांबा;
            }
            स्थिती Scheme::ShadowVarMasked: {
                ++m_nSchemeShadowVarMasked;
                prepareSchemeShadowVarMasked(vscp, vscpInfo);
                थांबा;
            }
            स्थिती Scheme::FlagShared: {
                ++m_nSchemeFlagShared;
                prepareSchemeFlagShared(vscp, vscpInfo);
                थांबा;
            }
            स्थिती Scheme::FlagUnique: {
                ++m_nSchemeFlagUnique;
                prepareSchemeFlagUnique(vscp, vscpInfo);
                थांबा;
            }
            स्थिती Scheme::मूल्य_Queue_Whole: {
                ++m_n_Scheme_मूल्य_Queues_Whole;
                prepare_Scheme_मूल्य_Queue</* Partial: */ असत्य>(vscp, vscpInfo);
                थांबा;
            }
            स्थिती Scheme::मूल्य_Queue_Partial: {
                ++m_n_Scheme_मूल्य_Queues_Partial;
                prepare_Scheme_मूल्य_Queue</* Partial: */ सत्य>(vscp, vscpInfo);
                थांबा;
            }
            }
        }
        // Convert all NBAs
        साठी (स्थिरांक NBA& nba : m_nbas) {
            AstAssignDly* स्थिरांक nbap = nba.nodep;
            AstVarScope* स्थिरांक vscp = nba.vscp;
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            // Run 'convert' step
            निवड (vscpInfo.m_scheme) {
            स्थिती Scheme::Undecided: {  // LCOV_EXCL_START
                UASSERT_OBJ(असत्य, vscp, "Unreachable");
                थांबा;
            }  // LCOV_EXCL_STOP
            स्थिती Scheme::UnsupportedCompoundArrayInLoop: {
                // TODO: make this an E_UNSUPPORTED...
                nbap->v3warn(BLKLOOPINIT, "Unsupported: Non-blocking assignment to array with "
                                          "compound element type inside loop");
                थांबा;
            }
            स्थिती Scheme::ShadowVar: {
                convertSchemeShadowVar(nbap, vscp, vscpInfo);
                थांबा;
            }
            स्थिती Scheme::ShadowVarMasked: {
                convertSchemeShadowVarMasked(nbap, vscp, vscpInfo);
                थांबा;
            }
            स्थिती Scheme::FlagShared: {
                convertSchemeFlagShared(nbap, vscp, vscpInfo);
                थांबा;
            }
            स्थिती Scheme::FlagUnique: {
                convertSchemeFlagUnique(nbap, vscp, vscpInfo);
                थांबा;
            }
            स्थिती Scheme::मूल्य_Queue_Whole: {
                convert_Scheme_मूल्य_Queue(nbap, vscp, vscpInfo, /* partial: */ असत्य);
                थांबा;
            }
            स्थिती Scheme::मूल्य_Queue_Partial:
                convert_Scheme_मूल्य_Queue(nbap, vscp, vscpInfo, /* partial: */ सत्य);
                थांबा;
            }
        }
    }
    शून्य visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstActive* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ignoreBlkAndNBlk);
        VL_RESTORER(m_inNonCombLogic);
        m_activep = nodep;
        स्थिरांक Ast_Sen_वृक्ष* स्थिरांक senTreep = nodep->sentreep();
        m_ignoreBlkAndNBlk = senTreep->hasStatic() || senTreep->hasInitial();
        m_inNonCombLogic = senTreep->hasClocked();
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_Procedure* nodep) override {
        VL_RESTORER(m_needsInitialTrigger);
        VL_RESTORER_CLEAR(m_nbaEventSenTreeps);
        स्थिरांक माप_प्रकार firstNBAAddedIndex = m_nbas.size();
        {
            VL_RESTORER(m_inSuspendableOrFork);
            VL_RESTORER(m_procp);
            VL_RESTORER(m_ignoreBlkAndNBlk);
            VL_RESTORER(m_inNonCombLogic);
            // When we are dealing with initial block we need to
            // treat it as suspendable when we meet a NBA
            m_inSuspendableOrFork = nodep->isSuspendable() || VN_IS(nodep, Initial);
            m_procp = nodep;
            जर (nodep->isSuspendable()) {
                m_ignoreBlkAndNBlk = असत्य;
                m_inNonCombLogic = सत्य;
            }
            iterateChildren(nodep);
        }
        स्वतः containsClocled = [](स्थिरांक AstSenItem* itemp) {
            पर्यंत (itemp) {
                जर (itemp->edgeType().clockedStmt()) परत सत्य;
                itemp = VN_AS(itemp->nextp(), SenItem);
            }
            परत असत्य;
        };
        स्थिरांक तार्किक addInitialTrigger = m_needsInitialTrigger
                                       && !(isProcedureWithSentreep<AstAlways>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysObserved>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysReactive>(nodep))
                                       && !containsClocled(m_activep->sentreep()->sensesp());
        जर (m_timingDomains.empty() && !addInitialTrigger) परत;

        // There were some timing domains involved in the process. Add all of them as sensitivities
        // of all NBA targets in this process. Note this is a bit of a sledgehammer, we should only
        // need those that directly precede the NBA in control flow, but that is hard to compute,
        // so we will hammer away.

        // First gather all senItems
        AstSenItem* senItemp = nullptr;
        जर (addInitialTrigger) {
            senItemp = नवीन AstSenItem{nodep->fileline(), AstSenItem::InitialNBA{}};
            ++m_nInitialNBA;
        }

        साठी (स्थिरांक Ast_Sen_वृक्ष* स्थिरांक domainp : m_timingDomains) {
            जर (domainp->sensesp())
                senItemp = Ast_गाठ::addNext(senItemp, domainp->sensesp()->clone_वृक्ष(सत्य));
        }
        m_timingDomains.clear();
        // Add them to all nba targets we gathered in this process
        साठी (माप_प्रकार i = firstNBAAddedIndex; i < m_nbas.size(); ++i) {
            m_vscpInfo(m_nbas[i].vscp).addSensitivity(senItemp);
        }
        साठी (Ast_Sen_वृक्ष* स्थिरांक senTreep : m_nbaEventSenTreeps)
            senTreep->addSensesp(senItemp->clone_वृक्ष(सत्य));
        // Done with these
        VL_DO_DANGLING(senItemp->delete_वृक्ष(), senItemp);
    }
    शून्य visit(AstFork* nodep) override {
        VL_RESTORER(m_inSuspendableOrFork);
        m_inSuspendableOrFork = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(AstCAwait* nodep) override {
        जर (nodep->sentreep()) m_timingDomains.घाला(nodep->sentreep());
        iterateChildren(nodep);
    }
    शून्य visit(AstFireEvent* nodep) override {
        UASSERT_OBJ(v3Global.hasEvents(), nodep, "Inconsistent");
        File_ओळ* स्थिरांक flp = nodep->fileline();

        Ast_गाठ_अभिव्यक्ती* स्थिरांक eventp = nodep->operandp()->unlinkFrBack();

        // Enqueue for clearing 'triggered' state on next eval
        AstCStmt* स्थिरांक cstmtp = नवीन AstCStmt{flp};
        cstmtp->add("vlSymsp->fireEvent(");
        cstmtp->add(eventp);
        cstmtp->add(");");

        Ast_गाठ* newp = cstmtp;
        स्थिरांक AstVarRef* स्थिरांक vrefp = VN_CAST(eventp, VarRef);
        जर (nodep->isDelayed() && (m_cfuncp || !vrefp)) {
            // V3Timing converts these with --timing
            nodep->v3warn(E_NOTIMING, "Nonblocking event trigger "
                                          << (m_cfuncp ? "in a non-inlined function/task"
                                                       : "of a class or interface member or "
                                                         "array element")
                                          << " requires --timing");
        } अन्यथा जर (nodep->isDelayed()) {
            स्थिरांक std::string newvarname = "_" + vrefp->varp()->shortName();
            AstVarScope* स्थिरांक dlyvscp
                = m_dlyTmps.make(flp, vrefp->varScopep()->scopep(), 1, newvarname);

            स्थिरांक स्वतः dlyRef = [=](VAccess access) {  //
                परत नवीन AstVarRef{flp, dlyvscp, access};
            };

            AstAlwaysPost* स्थिरांक postp = नवीन AstAlwaysPost{flp};
            AstIf* स्थिरांक ifp = नवीन AstIf{flp, dlyRef(VAccess::READ)};
            postp->addStmtsp(ifp);

            UASSERT_OBJ(m_activep, nodep, "No active to handle FireEvent");
            Ast_Sen_वृक्ष* senTreep = m_activep->sentreep();
            AstActive* activep = nullptr;
            जर (m_inSuspendableOrFork && (senTreep->hasInitial() || senTreep->hasClocked())) {
                // Suspendable code can trigger whenever it resumes, so fire when its NBAs are
                // committed (see visit(AstNodeProcedure*)), clearing the flag like
                // Scheme::FlagUnique
                senTreep = नवीन Ast_Sen_वृक्ष{
                    flp, senTreep->hasClocked() ? senTreep->sensesp()->clone_वृक्ष(सत्य) : nullptr};
                m_nbaEventSenTreeps.ढकला_back(senTreep);
                m_needsInitialTrigger |= m_timingDomains.empty();
                dlyvscp->varp()->setIgnorePostWrite();
                ifp->addThensp(नवीन AstAssign{flp, dlyRef(VAccess::WRITE),
                                             नवीन AstConst{flp, AstConst::BitFalse{}}});
                activep = नवीन AstActive{flp, "nba-event", senTreep};
                activep->sen_वृक्ष_Storep(senTreep);
            } अन्यथा {
                activep = नवीन AstActive{flp, "nba-event", senTreep};
                AstAlwaysPre* स्थिरांक prep = नवीन AstAlwaysPre{flp};
                prep->addStmtsp(नवीन AstAssign{flp, dlyRef(VAccess::WRITE),
                                              नवीन AstConst{flp, AstConst::BitFalse{}}});
                activep->addStmtsp(prep);
            }
            ifp->addThensp(newp);
            m_activep->addNextHere(activep);
            activep->addStmtsp(postp);

            newp = नवीन AstAssign{flp, dlyRef(VAccess::WRITE),
                                 नवीन AstConst{flp, AstConst::BitTrue{}}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstAssignDly* nodep) override {
        // Prevent double processing due to AstExprStmt being moved before this node
        जर (nodep->user1SetOnce()) परत;

        जर (m_cfuncp) {
            जर (!v3Global.rootp()->nbaEventp()) {
                nodep->v3warn(
                    E_NOTIMING,
                    "Delayed assignment in a non-inlined function/task requires --timing");
            }
            परत;
        }
        UASSERT_OBJ(m_procp, nodep, "Delayed assignment not under process");
        UASSERT_OBJ(m_activep, nodep, "<= not under sensitivity block");
        UASSERT_OBJ(m_scopep, nodep, "<= not under scope");
        UASSERT_OBJ(m_inSuspendableOrFork || m_activep->hasClocked(), nodep,
                    "<= assignment in non-clocked block, should have been converted in V3Active");

        m_needsInitialTrigger |= m_timingDomains.empty();

        // Record scope of this NBA
        nodep->user2p(m_scopep);

        // Grab the reference to the target of the NBA, also lift ExprStmt statements on the LHS
        VL_RESTORER(m_currNbaLhsRefp);
        UASSERT_OBJ(!m_currNbaLhsRefp, nodep, "NBAs should not nest");
        nodep->lhsp()->foreach([&](Ast_गाठ* currp) {
            // cppcheck-suppress constVariablePointer
            जर (Ast_अभिव्यक्ती_Stmt* स्थिरांक exprp = VN_CAST(currp, अभिव्यक्ती_Stmt)) {
                // Move statements before the NBA
                nodep->addHereThisAsNext(exprp->stmtsp()->unlinkFrBackWithNext());
                // Replace with result
                currp->replaceWith(exprp->resultp()->unlinkFrBack());
                // Get rid of the AstExprStmt
                VL_DO_DANGLING2(ढकला_Deletep(currp), currp, exprp);
            } अन्यथा जर (AstVarRef* स्थिरांक refp = VN_CAST(currp, VarRef)) {
                // Ignore reads (e.g.: '_[*here*] <= _')
                जर (refp->access().isReadOnly()) परत;
                // A RW ref on the LHS (e.g.: '_[preInc(*here*)] <= _') is asking for trouble at
                // this point. These should be lowered in an earlier pass into sequenced
                // temporaries.
                UASSERT_OBJ(!refp->access().isRW(), refp, "RW ref on LHS of NBA");
                // Multiple target variables
                // (e.g.: '{*here*, *and here*} <= _',or '*here*[*and here* = _] <= _').
                // These should be lowered in an earlier pass into sequenced statements.
                UASSERT_OBJ(!m_currNbaLhsRefp, refp, "Multiple Write refs on LHS of NBA");
                // Hold on to it
                m_currNbaLhsRefp = refp;
            }
        });
        // The target variable of the NBA (there can only be one per NBA at this point)
        AstVarScope* स्थिरांक vscp = m_currNbaLhsRefp->varScopep();
        // Record it on first encounter
        VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
        जर (!vscpInfo.m_firstNbaRefp) {
            vscpInfo.m_firstNbaRefp = m_currNbaLhsRefp;
            vscpInfo.m_fistActivep = m_activep;
            m_vscps.emplace_back(vscp);
        }
        // Note usage context
        vscpInfo.m_whole |= VN_IS(nodep->lhsp(), VarRef);
        vscpInfo.m_partial |= VN_IS(nodep->lhsp(), Sel);
        vscpInfo.m_inLoop |= m_inLoop;
        vscpInfo.m_inSuspOrFork |= m_inSuspendableOrFork;
        // Sensitivity might be non-clocked, in a suspendable process, which are handled elsewhere
        जर (m_activep->sentreep()->hasClocked()) {
            जर (vscpInfo.m_fistActivep != m_activep) {
                AstVar* स्थिरांक varp = vscp->varp();
                जर (!varp->user1SetOnce()
                    && !varp->fileline()->warnIsOff(V3ErrorCode::MULTIDRIVEN)) {
                    varp->v3warn(MULTIDRIVEN,
                                 "Signal has multiple driving blocks with different clocking: "
                                     << varp->prettyNameQ() << '\n'
                                     << vscpInfo.m_firstNbaRefp->warnOther()
                                     << "... Location of first driving block\n"
                                     << vscpInfo.m_firstNbaRefp->warnContextSecondary()
                                     << m_currNbaLhsRefp->warnOther()
                                     << "... Location of other driving block\n"
                                     << m_currNbaLhsRefp->warnContextPrimary() << '\n');
                }
            }
            // Add this sensitivity to the variable
            vscpInfo.addSensitivity(m_activep->sentreep());
        }

        // Record the NBA for later processing
        m_nbas.emplace_back();
        NBA& nba = m_nbas.back();
        nba.nodep = nodep;
        nba.vscp = vscp;

        // Record write reference
        recordWriteRef(m_currNbaLhsRefp, सत्य);

        iterateChildren(nodep);
    }
    शून्य visit(AstVarRef* nodep) override {
        // Already checked the NBA LHS ref, ignore here
        जर (nodep == m_currNbaLhsRefp) परत;
        // Only care about write refs
        जर (!nodep->access().isWriteOrRW()) परत;
        // Record write reference
        recordWriteRef(nodep, असत्य);
    }
    शून्य visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_CCall* स्थिरांक nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        स्थिरांक स्वतः& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        जर (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } अन्यथा {
            साठी (AstCFunc* स्थिरांक cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    शून्य visit(AstCFunc* स्थिरांक nodep) override {
        स्थिरांक स्वतः& मूल्य = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        जर (मूल्य.m_state != CFuncCache::UNINITIALIZED) परत;
        VL_RESTORER(m_cfuncp);
        // Its timing domains are those of the processes calling it, see visitCalledCFunc()
        VL_RESTORER_CLEAR(m_timingDomains);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }

    // Pre/Post logic are created here and their content need no further changes, so ignore.
    शून्य visit(AstAlwaysPre*) override {}
    शून्य visit(AstAlwaysPost*) override {}

    //--------------------
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit DelayedVisitor(AstNetlist* nodep)
        : m_classGraphp{V3ClassGraph::build(nodep)} {
        iterate(nodep);
    }
    ~DelayedVisitor() override {
        V3Stats::addStat("NBA, variables using ShadowVar scheme", m_nSchemeShadowVar);
        V3Stats::addStat("NBA, variables using ShadowVarMasked scheme", m_nSchemeShadowVarMasked);
        V3Stats::addStat("NBA, variables using FlagShared scheme", m_nSchemeFlagShared);
        V3Stats::addStat("NBA, variables using FlagUnique scheme", m_nSchemeFlagUnique);
        V3Stats::addStat("NBA, variables using ValueQueueWhole scheme", m_n_Scheme_मूल्य_Queues_Whole);
        V3Stats::addStat("NBA, variables using ValueQueuePartial scheme",
                         m_n_Scheme_मूल्य_Queues_Partial);
        V3Stats::addStat("Optimizations, NBA flags shared", m_nSharedSetFlags);
        V3Stats::addStat("Procedures needing initial NBA trigger", m_nInitialNBA);
        V3Stats::addStat("Non-inlined co_awaits with SenTree", m_non_Inlined_CAwaits_With_Sen_वृक्ष);
    }
};

//######################################################################
// Delayed class functions

शून्य V3Delayed::delayedAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DelayedVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("delayed", 0, dump_वृक्ष_Either_Level() >= 3);
}
