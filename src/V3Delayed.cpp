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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Delayed.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3ClassGraph.h"
#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3LinkLValue.h"
#ಅನ್ತರ್ಭಾವ "V3SharedTmps.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <deque>
#ಅನ್ತರ್ಭಾವ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert AstAssignDlys (NBAs)

ವರ್ಗ DelayedVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // TYPES

    // The various NBA conversion schemes, including error cases
    ಗಣನಾ ವರ್ಗ Scheme : uint8_t {
        Undecided = 0,
        UnsupportedCompoundArrayInLoop,
        ShadowVar,
        ShadowVarMasked,
        FlagShared,
        FlagUnique,
        ಮೌಲ್ಯ_Queue_Whole,
        ಮೌಲ್ಯ_Queue_Partial
    };

    // All info associated with a variable that is the target of an NBA
    ವರ್ಗ VarScopeInfo final {
    ಸಾರ್ವಜನಿಕ:
        // First write reference encountered to the VarScope as the target on an NBA
        ಧ್ರುವ AstVarRef* m_firstNbaRefp = nullptr;
        // Active block 'm_firstNbaRefp' is under
        ಧ್ರುವ AstActive* m_fistActivep = nullptr;
        ತಾರ್ಕಿಕ m_whole = ಅಸತ್ಯ;  // Used on LHS of NBA directly via VarRef
        ತಾರ್ಕಿಕ m_partial = ಅಸತ್ಯ;  // Used on LHS of NBA under a Sel
        ತಾರ್ಕಿಕ m_inLoop = ಅಸತ್ಯ;  // Used on LHS of NBA in a loop
        ತಾರ್ಕಿಕ m_inSuspOrFork = ಅಸತ್ಯ;  // Used on LHS of NBA in suspendable process or fork
        Scheme m_scheme = Scheme::Undecided;  // Conversion scheme to use for this variable

    ಗುಪ್ತ:
        // Combined sensitivities of all NBAs targeting this variable
        Ast_Sen_ವೃಕ್ಷ* m_senTreep = nullptr;

        // Union of stuff needed for the various schemes - use accessors below!
        ಸಙ್ಘ {
            ಸಂರಚನಾ {  // Stuff needed for Scheme::ShadowVar
                AstVarScope* vscp;  // The shadow variable
            } m_shadowVariableKit;
            ಸಂರಚನಾ {  // Stuff needed for Scheme::ShadowVarMasked
                AstVarScope* vscp;  // The shadow variable
                AstVarScope* maskp;  // The mask variable
            } m_shadowVarMaskedKit;
            ಸಂರಚನಾ {  // Stuff needed for Scheme::FlagShared
                AstActive* activep;  // The active block for the Pre/Post logic
                AstAlwaysPost* postp;  // The post block for commiting results
                AstVarScope* commitFlagp;  // The commit flag variable, for reuse
                AstIf* commitIfp;  // The previous if statement for committing, for reuse
            } m_flagSharedKit;
            ಸಂರಚನಾ {  // Stuff needed for Scheme::FlagUnique
                AstAlwaysPost* postp;  // The post block for commiting results
            } m_flagUniqueKit;
            ಸಂರಚನಾ {  // Stuff needed for Scheme::ValueQueueWhole/Scheme::ValueQueuePartial
                AstVarScope* vscp;  // The commit queue variable
            } m_ಮೌಲ್ಯ_Queue_Kit;
        } m_kitUnion;

    ಸಾರ್ವಜನಿಕ:
        VarScopeInfo() = ಸ್ವಭಾವ;
        ~VarScopeInfo() {
            // Might not be linked if there was an error
            ಯದಿ (!m_senTreep->backp()) VL_DO_DANGLING(m_senTreep->delete_ವೃಕ್ಷ(), m_senTreep);
        }
        // Accessors for the above union fields
        auto& shadowVariableKit() {
            UASSERT(m_scheme == Scheme::ShadowVar, "Inconsistent Scheme");
            ಪ್ರತ್ಯಾಗಮ m_kitUnion.m_shadowVariableKit;
        }
        auto& shadowVarMaskedKit() {
            UASSERT(m_scheme == Scheme::ShadowVarMasked, "Inconsistent Scheme");
            ಪ್ರತ್ಯಾಗಮ m_kitUnion.m_shadowVarMaskedKit;
        }
        auto& flagSharedKit() {
            UASSERT(m_scheme == Scheme::FlagShared, "Inconsistent Scheme");
            ಪ್ರತ್ಯಾಗಮ m_kitUnion.m_flagSharedKit;
        }
        auto& flagUniqueKit() {
            UASSERT(m_scheme == Scheme::FlagUnique, "Inconsistent Scheme");
            ಪ್ರತ್ಯಾಗಮ m_kitUnion.m_flagUniqueKit;
        }
        auto& ಮೌಲ್ಯ_Queue_Kit() {
            UASSERT(m_scheme == Scheme::ಮೌಲ್ಯ_Queue_Partial || m_scheme == Scheme::ಮೌಲ್ಯ_Queue_Whole,
                    "Inconsistent Scheme");
            ಪ್ರತ್ಯಾಗಮ m_kitUnion.m_ಮೌಲ್ಯ_Queue_Kit;
        }

        // Accessor
        Ast_Sen_ವೃಕ್ಷ* senTreep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_senTreep; }

        // Add sensitivities
        ಶೂನ್ಯ addSensitivity(AstSenItem* nodep) {
            ಯದಿ (!m_senTreep) m_senTreep = ನೂತನ Ast_Sen_ವೃಕ್ಷ{nodep->fileline(), nullptr};
            // Add a copy of each term
            m_senTreep->addSensesp(nodep->clone_ವೃಕ್ಷ(ಸತ್ಯ));
            // Remove duplicates
            V3Const::constifyExpensiveEdit(m_senTreep);
        }
        // cppcheck-suppress constParameterPointer
        ಶೂನ್ಯ addSensitivity(Ast_Sen_ವೃಕ್ಷ* nodep) { addSensitivity(nodep->sensesp()); }
    };

    // Data structure to keep track of all writes to
    ಸಂರಚನಾ WriteReference final {
        AstVarRef* m_refp = nullptr;  // The reference
        ತಾರ್ಕಿಕ m_isNBA = ಅಸತ್ಯ;  // True if an NBA write
        ತಾರ್ಕಿಕ m_inNonComb = ಅಸತ್ಯ;  // True if reference is known to be in non-combinational logic
        WriteReference() = ಸ್ವಭಾವ;
        WriteReference(AstVarRef* refp, ತಾರ್ಕಿಕ isNBA, ತಾರ್ಕಿಕ inNonComb)
            : m_refp{refp}
            , m_isNBA{isNBA}
            , m_inNonComb{inNonComb} {}
    };

    // Data required to lower AstAssignDelay later
    ಸಂರಚನಾ NBA final {
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
    ಧ್ರುವ VNUser1InUse m_user1InUse;
    ಧ್ರುವ VNUser2InUse m_user2InUse;
    ಧ್ರುವ VNUser3InUse m_user3InUse;

    ಸಂರಚನಾ CFuncCache final {
        VInsertionSet<Ast_Sen_ವೃಕ್ಷ*> m_timingDomains;  // What shall be added to m_timingDomains
        std::set<AstCFunc*>
            m_includes;  // CFuncs whose CFuncCache shall be included into this - this is used to
                         // break cycles: A->B->A (instead of visiting A while it is still begin
                         // visited B just marks that it includes A)
        ಗಣನಾ State : uint8_t {
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
    VInsertionSet<Ast_Sen_ವೃಕ್ಷ*> m_timingDomains;  // Timing resume domains
    V3SharedTmps m_dlyTmps{"__Vdly", VVarType::BLOCKTEMP};  // Temporary variables
    // Commit queue data types, by element type and partial flag. Shared by all instances,
    // as m_dlyTmps only shares variables with the same data type.
    std::map<std::pair<ಧ್ರುವ Ast_ಗಂಟು_DType*, ತಾರ್ಕಿಕ>, AstNBACommitQueueDType*> m_cqDTypeps;

    ಧ್ರುವ std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::vector<ಧ್ರುವ AstCFunc*> m_call_ರಾಶಿ;  // Current callstack of AstCFuncs

    // STATE - for current visit position (use VL_RESTORER)
    AstActive* m_activep = nullptr;  // Current activate
    ಧ್ರುವ AstCFunc* m_cfuncp = nullptr;  // Current public C Function
    Ast_ಗಂಟು_Procedure* m_procp = nullptr;  // Current process
    AstScope* m_scopep = nullptr;  // Current scope
    ತಾರ್ಕಿಕ m_inLoop = ಅಸತ್ಯ;  // True in for loops
    ತಾರ್ಕಿಕ m_inSuspendableOrFork = ಅಸತ್ಯ;  // True in suspendable processes and forks
    ತಾರ್ಕಿಕ m_ignoreBlkAndNBlk = ಅಸತ್ಯ;  // Suppress delayed assignment BLKANDNBLK
    ತಾರ್ಕಿಕ m_inNonCombLogic = ಅಸತ್ಯ;  // We are in non-combinational logic
    ತಾರ್ಕಿಕ m_needsInitialTrigger = ಅಸತ್ಯ;  // Whether a NodeProcedure needs a initial trigger
    std::vector<Ast_Sen_ವೃಕ್ಷ*> m_nbaEventSenTreeps;  // Sensitivities of '->>' in the process
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
    VDouble0 m_n_Scheme_ಮೌಲ್ಯ_Queues_Whole;  //  Number of variables using Scheme::ValueQueueWhole
    VDouble0 m_n_Scheme_ಮೌಲ್ಯ_Queues_Partial;  //  Number of variables using Scheme::ValueQueuePartial
    VDouble0 m_nSharedSetFlags;  // "Set" flags actually shared by Scheme::FlagShared variables
    VDouble0 m_nInitialNBA;  // Number of procedural blocks with initial NBA
    VDouble0 m_non_Inlined_CAwaits_With_Sen_ವೃಕ್ಷ;  // Count uses of not inlined co_awaits

    // METHODS

    // Return true iff a variable is assigned by both blocking and nonblocking
    // assignments. Issue BLKANDNBLK error if we can't prove the mixed
    // assignments are to independent bits and the blocking assignment can be
    // in combinational logic, which is something we can't safely implement
    // still.
    ತಾರ್ಕಿಕ checkMixedUsage(ಧ್ರುವ AstVarScope* vscp, ತಾರ್ಕಿಕ isIntegralOrPacked) {

        ಸಂರಚನಾ Ref final {
            AstVarRef* m_refp;  // The reference
            ತಾರ್ಕಿಕ m_inNonComb;  // True if known to be in non-combinational logic
            ಪೂರ್ಣಾಙ್ಕ m_lsb;  // LSB of accessed range
            ಪೂರ್ಣಾಙ್ಕ m_msb;  // MSB of accessed range
            Ref(AstVarRef* refp, ತಾರ್ಕಿಕ inNonComb, ಪೂರ್ಣಾಙ್ಕ lsb, ಪೂರ್ಣಾಙ್ಕ msb)
                : m_refp{refp}
                , m_inNonComb{inNonComb}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };

        std::vector<Ref> blkRefs;  // Blocking writes
        std::vector<Ref> nbaRefs;  // Non-blockign writes

        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = isIntegralOrPacked ? vscp->width() : 1;

        ಪ್ರತಿ (ಧ್ರುವ auto& writeRef : m_writeRefs(vscp)) {
            ಪೂರ್ಣಾಙ್ಕ lsb = 0;
            ಪೂರ್ಣಾಙ್ಕ msb = width - 1;
            ಯದಿ (ಧ್ರುವ AstSel* ಧ್ರುವ selp = VN_CAST(writeRef.m_refp->backp(), Sel)) {
                ಯದಿ (VN_IS(selp->lsbp(), Const)) {
                    lsb = selp->lsbConst();
                    msb = selp->msbConst();
                }
            }
            ಯದಿ (writeRef.m_isNBA) {
                nbaRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            } ಅನ್ಯಥಾ {
                blkRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            }
        }
        // We only run this function on targets of NBAs, so there should be at least one...
        UASSERT_OBJ(!nbaRefs.empty(), vscp, "Did not record NBA write");
        // If no blocking upadte, then we are good
        ಯದಿ (blkRefs.empty()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // If the blocking assignment is in non-combinational logic (i.e.:
        // in logic that has an explicit trigger), then we can safely
        // implement it (there is no race between clocked logic and post
        // scheduled logic), so need not error
        blkRefs.erase(std::remove_if(blkRefs.begin(), blkRefs.end(),
                                     [](ಧ್ರುವ Ref& ref) { ಪ್ರತ್ಯಾಗಮ ref.m_inNonComb; }),
                      blkRefs.end());

        // If nothing left, then we need not error
        ಯದಿ (blkRefs.empty()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;

        // If not a packed variable, warn here as we can't prove independence
        ಯದಿ (!isIntegralOrPacked) {
            ಧ್ರುವ Ref& blkRef = blkRefs.front();
            ಧ್ರುವ Ref& nbaRef = nbaRefs.front();
            vscp->v3warn(
                BLKANDNBLK,
                "Unsupported: Blocking and non-blocking assignments to same non-packed variable: "
                    << vscp->varp()->prettyNameQ() << '\n'
                    << vscp->warnContextPrimary() << '\n'
                    << blkRef.m_refp->warnOther() << "... Location of blocking assignment\n"
                    << blkRef.m_refp->warnContextSecondary() << '\n'
                    << nbaRef.m_refp->warnOther() << "... Location of nonblocking assignment\n"
                    << nbaRef.m_refp->warnContextSecondary());
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }

        // We need to error if we can't prove the written bits are independent

        // Sort refs by interval
        ಧ್ರುವ auto lessThanRef = [](ಧ್ರುವ Ref& a, ಧ್ರುವ Ref& b) {
            ಯದಿ (a.m_lsb != b.m_lsb) ಪ್ರತ್ಯಾಗಮ a.m_lsb < b.m_lsb;
            ಪ್ರತ್ಯಾಗಮ a.m_msb < b.m_msb;
        };
        std::stable_sort(blkRefs.begin(), blkRefs.end(), lessThanRef);
        std::stable_sort(nbaRefs.begin(), nbaRefs.end(), lessThanRef);
        // Iterate both vectors, checking for overlap
        auto bIt = blkRefs.begin();
        auto nIt = nbaRefs.begin();
        ಯಾವತ್ (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            ಯದಿ (lessThanRef(*bIt, *nIt)) {
                ಯದಿ (nIt->m_lsb <= bIt->m_msb) ಭಙ್ಗ;  // Stop on Overlap
                ++bIt;
            } ಅನ್ಯಥಾ {
                ಯದಿ (bIt->m_lsb <= nIt->m_msb) ಭಙ್ಗ;  // Stop on Overlap
                ++nIt;
            }
        }

        // If we found an overlapping range that cannot be safely implemented, then wran...
        ಯದಿ (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            ಧ್ರುವ Ref& blkRef = *bIt;
            ಧ್ರುವ Ref& nbaRef = *nIt;
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

        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Choose the NBA scheme used for the given variable.
    Scheme chooseScheme(ಧ್ರುವ AstVarScope* vscp, ಧ್ರುವ VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::Undecided, vscp, "NBA scheme already decided");

        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = vscp->dtypep()->skipRefp();
        // Unpacked arrays
        ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ uaDTypep = VN_CAST(dtypep, UnpackArrayDType)) {
            // If whole array is target of NBA, use ShadowVar, but not in a suspendable process or
            // fork, which resume outside the 'nba' region, so its 'pre' logic would overwrite the
            // value they write in the shadow variable
            ಯದಿ (vscpInfo.m_whole) {
                ಪ್ರತ್ಯಾಗಮ vscpInfo.m_inSuspOrFork ? Scheme::FlagUnique : Scheme::ShadowVar;
            }
            // Basic underlying type of elements, if any.
            ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = uaDTypep->basicp();
            // If used in a loop, we must have a dynamic commit queue. (Also works in suspendables)
            ಯದಿ (vscpInfo.m_inLoop) {
                // Arrays with compound element types are currently not supported in loops
                ಯದಿ (!basicp
                    || !(basicp->isIntegralOrPacked()  //
                         || basicp->isDouble()  //
                         || basicp->isString())) {
                    ಪ್ರತ್ಯಾಗಮ Scheme::UnsupportedCompoundArrayInLoop;
                }
                ಯದಿ (vscpInfo.m_partial) ಪ್ರತ್ಯಾಗಮ Scheme::ಮೌಲ್ಯ_Queue_Partial;
                ಪ್ರತ್ಯಾಗಮ Scheme::ಮೌಲ್ಯ_Queue_Whole;
            }
            // A suspendable process or fork can resume outside the 'nba' region, so the 'pre'
            // logic of the other schemes would undo its updates. The unique flag scheme has none.
            ಯದಿ (vscpInfo.m_inSuspOrFork) ಪ್ರತ್ಯಾಗಮ Scheme::FlagUnique;
            // Otherwise if an array of packed/basic elements, use the shared flag scheme
            ಯದಿ (basicp) ಪ್ರತ್ಯಾಗಮ Scheme::FlagShared;
            // Finally fall back on the shadow variable scheme, e.g. for
            // arrays of unpacked structs. This will be slow.
            // TODO: generic LHS scheme as discussed in #5092
            ಪ್ರತ್ಯಾಗಮ Scheme::ShadowVar;
        }

        // Likewise, the 'pre' logic of the shadow variable scheme would undo the updates of a
        // suspendable process or fork, so use the unique flag scheme
        ಯದಿ (vscpInfo.m_inSuspOrFork) ಪ್ರತ್ಯಾಗಮ Scheme::FlagUnique;

        ಧ್ರುವ ತಾರ್ಕಿಕ isIntegralOrPacked = dtypep->isIntegralOrPacked();
        // Check for mixed usage (this also warns if not OK)
        ಯದಿ (checkMixedUsage(vscp, isIntegralOrPacked)) {
            // If it's a variable updated by both blocking and non-blocking
            // assignments, use the ShadowVarMasked schem if masked update is
            // possible. This can handle blocking and non-blocking updates to
            // inpdendent parts correctly at run-time, and always works, even
            // in loops or other dynamic context.
            ಯದಿ (isIntegralOrPacked) ಪ್ರತ್ಯಾಗಮ Scheme::ShadowVarMasked;
            // If it's inside a loop, use Scheme::ShadowVar, which is safe,
            // but will generate incorrect code if a partial update is used
            ಯದಿ (vscpInfo.m_inLoop) ಪ್ರತ್ಯಾಗಮ Scheme::ShadowVar;
            // Otherwise (for not packed variables), use the FlagUnique scheme,
            // which at least handles partial updates correctly, but might break
            // in loops or other dynamic context
            ಪ್ರತ್ಯಾಗಮ Scheme::FlagUnique;
        }

        // Otherwise use the simple shadow variable scheme
        ಪ್ರತ್ಯಾಗಮ Scheme::ShadowVar;
    }

    // Given an expression 'exprp', return a new expression that always evaluates to the
    // value of the given expression at this point in the program. That is:
    // - If given a non-constant expression, create a new temporary AstVarScope under the given
    //   'scopep', with the given 'name', assign the expression to it, and return a read reference
    //   to the new AstVarScope.
    // - If given a constant, just return that constant.
    // New statements are inserted before 'insertp'
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* captureVal(AstScope* ಧ್ರುವ scopep, Ast_ಗಂಟು_Stmt* ಧ್ರುವ insertp,
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp, ಧ್ರುವ std::string& name) {
        UASSERT_OBJ(!exprp->backp(), exprp, "Should have been unlinked");
        File_ಸಾಲು* ಧ್ರುವ flp = exprp->fileline();
        ಯದಿ (VN_IS(exprp, Const)) ಪ್ರತ್ಯಾಗಮ exprp;
        // TODO: there are some const variables that could be simply referenced here
        AstVarScope* ಧ್ರುವ tmpVscp = m_dlyTmps.make(flp, scopep, exprp->dtypep(), name);
        insertp->addHereThisAsNext(
            ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, tmpVscp, VAccess::WRITE}, exprp});
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, tmpVscp, VAccess::READ};
    };

    // Similar to 'captureVal', but captures an LValue expression. That is, the returned
    // expression will reference the same location as the input expression, at this point in the
    // program.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* captureLhs(AstScope* ಧ್ರುವ scopep, Ast_ಗಂಟು_Stmt* ಧ್ರುವ insertp,
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp, ಧ್ರುವ std::string& baseName) {
        UASSERT_OBJ(!lhsp->backp(), lhsp, "Should have been unlinked");
        // Running node pointer
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep = lhsp;
        // Capture AstSel indices - there should be only one
        ಯದಿ (AstSel* ಧ್ರುವ selp = VN_CAST(nodep, Sel)) {
            ಧ್ರುವ std::string tmpName{"Lsb" + baseName};
            selp->lsbp(captureVal(scopep, insertp, selp->lsbp()->unlinkFrBack(), tmpName));
            // Continue with target
            nodep = selp->fromp();
        }
        UASSERT_OBJ(!VN_IS(nodep, Sel), lhsp, "Multiple 'AstSel' applied to LHS reference");
        // Capture the indices of selects - might be many, also below struct members. If a handle
        // selects the instance the target is a member of, capture the handle instead, as it is
        // only read, to select the target when the NBA is executed (IEEE 1800-2023 10.4.2).
        size_t nArraySels = 0;
        ಯಾವತ್ (ಸತ್ಯ) {
            ಯದಿ (Ast_ಗಂಟು_Sel* ಧ್ರುವ selp = VN_CAST(nodep, ಗಂಟು_Sel)) {
                ಧ್ರುವ std::string tmpName{"Dim" + std::to_string(nArraySels++) + baseName};
                selp->bitp(captureVal(scopep, insertp, selp->bitp()->unlinkFrBack(), tmpName));
                nodep = selp->fromp();
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstStructSel* ಧ್ರುವ selp = VN_CAST(nodep, StructSel)) {
                nodep = selp->fromp();
            } ಅನ್ಯಥಾ ಯದಿ (AstMemberSel* ಧ್ರುವ selp = VN_CAST(nodep, MemberSel)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ handlep = selp->fromp()->unlinkFrBack();
                V3LinkLValue::linkLValueSet(handlep, VAccess::READ);
                AstVarRef* ಧ್ರುವ refp
                    = VN_AS(captureVal(scopep, insertp, handlep, "Handle" + baseName), VarRef);
                refp->varp()->setIgnorePostWrite();
                // Like the handle it replaces, mark it written, as is the member it selects
                refp->access(selp->access());
                selp->fromp(refp);
                ಪ್ರತ್ಯಾಗಮ lhsp;
            } ಅನ್ಯಥಾ {
                ಭಙ್ಗ;
            }
        }
        // What remains must be the variable, or some sort of select, we assume can reuse it
        UASSERT_OBJ(nodep->isPure(), lhsp, "Malformed LHS in NBA");
        // Now have been converted to use the captured values
        ಪ್ರತ್ಯಾಗಮ lhsp;
    }

    ಶೂನ್ಯ add_CFunc_Cached_ಮೌಲ್ಯಗಳು(ಧ್ರುವ AstCFunc* ಧ್ರುವ cfuncp,
                              std::unordered_set<ಧ್ರುವ AstCFunc*>& visited) {
        ಯದಿ (!visited.ಸೇರಿಸು(cfuncp).second) ಪ್ರತ್ಯಾಗಮ;
        CFuncCache& ಮೌಲ್ಯ = m_cfuncsCache(cfuncp);
        m_timingDomains.ಸೇರಿಸು(ಮೌಲ್ಯ.m_timingDomains.begin(), ಮೌಲ್ಯ.m_timingDomains.end());
        m_non_Inlined_CAwaits_With_Sen_ವೃಕ್ಷ += ಮೌಲ್ಯ.m_timingDomains.size();
        ಪ್ರತಿ (ಧ್ರುವ AstCFunc* ಧ್ರುವ includedp : ಮೌಲ್ಯ.m_includes) {
            add_CFunc_Cached_ಮೌಲ್ಯಗಳು(includedp, visited);
        }
    }

    // Create a temporary variable in the given 'scopep', with the given 'name', and with 'dtypep'
    // type, with the bits selected by 'sLsbp'/'sWidthp' set to 'valuep', other bits set to zero.
    // Insert new statements before 'insertp'.
    // Returns a read reference to the temporary variable.
    AstVarRef* createWidened(File_ಸಾಲು* flp, AstScope* scopep, Ast_ಗಂಟು_DType* dtypep,
                             Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sLsbp, ಪೂರ್ಣಾಙ್ಕ sWidth, ಧ್ರುವ std::string& name,
                             Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, Ast_ಗಂಟು* insertp) {
        // Create temporary variable.
        AstVarScope* ಧ್ರುವ tp = m_dlyTmps.make(flp, scopep, dtypep, name);
        // Zero it
        AstConst* ಧ್ರುವ zerop = ನೂತನ AstConst{flp, AstConst::DTyped{}, dtypep};
        zerop->ಅಂಕೆ().setAllBits0();
        insertp->addHereThisAsNext(
            ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, tp, VAccess::WRITE}, zerop});
        // Set the selected bits to 'valuep'
        AstSel* ಧ್ರುವ selp = ನೂತನ AstSel{flp, ನೂತನ AstVarRef{flp, tp, VAccess::WRITE},
                                        sLsbp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ), sWidth};
        insertp->addHereThisAsNext(ನೂತನ AstAssign{flp, selp, valuep});
        // This is the expression to get the value of the temporary
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, tp, VAccess::READ};
    }

    // Scheme::ShadowVar
    ಶೂನ್ಯ prepareSchemeShadowVar(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
        AstScope* ಧ್ರುವ scopep = vscp->scopep();
        // Create the shadow variable
        ಧ್ರುವ std::string name = "_" + vscp->varp()->shortName();
        AstVarScope* ಧ್ರುವ shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), name);
        vscpInfo.shadowVariableKit().vscp = shadowVscp;
        // Mark both for V3LifePsot
        vscp->optimizeLifePost(ಸತ್ಯ);
        shadowVscp->optimizeLifePost(ಸತ್ಯ);
        // Create the AstActive for the Pre/Post logic
        AstActive* ಧ್ರುವ activep = ನೂತನ AstActive{flp, "nba-shadow-variable", vscpInfo.senTreep()};
        activep->sen_ವೃಕ್ಷ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Pre' scheduled 'shadowVariable = originalVariable' assignment
        AstAlwaysPre* ಧ್ರುವ prep = ನೂತನ AstAlwaysPre{flp};
        activep->addStmtsp(prep);
        prep->addStmtsp(ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, shadowVscp, VAccess::WRITE},
                                      ನೂತನ AstVarRef{flp, vscp, VAccess::READ}});
        // Add 'Post' scheduled 'originalVariable = shadowVariable' assignment
        AstAlwaysPost* ಧ್ರುವ postp = ನೂತನ AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        postp->addStmtsp(ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, vscp, VAccess::WRITE},
                                       ನೂತನ AstVarRef{flp, shadowVscp, VAccess::READ}});
    }
    ಶೂನ್ಯ convertSchemeShadowVar(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        AstVarScope* ಧ್ರುವ shadowVscp = vscpInfo.shadowVariableKit().vscp;

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ಧ್ರುವ refp) {
            ಯದಿ (!refp->access().isWriteOnly()) ಪ್ರತ್ಯಾಗಮ;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
    }

    // Scheme::ShadowVarMasked
    ಶೂನ್ಯ prepareSchemeShadowVarMasked(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
        AstScope* ಧ್ರುವ scopep = vscp->scopep();
        // Create the shadow variable
        ಧ್ರುವ std::string shadowName = "_" + vscp->varp()->shortName();
        AstVarScope* ಧ್ರುವ shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), shadowName);
        vscpInfo.shadowVarMaskedKit().vscp = shadowVscp;
        // Create the makk variable
        ಧ್ರುವ std::string maskName = "Mask__" + vscp->varp()->shortName();
        AstVarScope* ಧ್ರುವ maskVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), maskName);
        maskVscp->varp()->setIgnorePostWrite();
        vscpInfo.shadowVarMaskedKit().maskp = maskVscp;
        // Create the AstActive for the Post logic
        AstActive* ಧ್ರುವ activep
            = ನೂತನ AstActive{flp, "nba-shadow-var-masked", vscpInfo.senTreep()};
        activep->sen_ವೃಕ್ಷ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit and mask clear
        AstAlwaysPost* ಧ್ರುವ postp = ನೂತನ AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit - vscp = (shadowVscp & maskVscp) | (vscp & ~maskVscp);
        postp->addStmtsp(ನೂತನ AstAssign{
            flp, ನೂತನ AstVarRef{flp, vscp, VAccess::WRITE},
            ನೂತನ AstOr{flp,
                      ನೂತನ AstAnd{flp, ನೂತನ AstVarRef{flp, shadowVscp, VAccess::READ},
                                 ನೂತನ AstVarRef{flp, maskVscp, VAccess::READ}},
                      ನೂತನ AstAnd{flp, ನೂತನ AstVarRef{flp, vscp, VAccess::READ},
                                 ನೂತನ AstNot{flp, ನೂತನ AstVarRef{flp, maskVscp, VAccess::READ}}}}});
        vscp->varp()->setIgnorePostRead();
        // Clar the mask - maskVscp = '0;
        postp->addStmtsp(
            ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, maskVscp, VAccess::WRITE},
                          ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, maskVscp->width(), 0}});
    }
    ಶೂನ್ಯ convertSchemeShadowVarMasked(AstAssignDly* nodep, AstVarScope* vscp,
                                      VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        AstVarScope* ಧ್ರುವ shadowVscp = vscpInfo.shadowVarMaskedKit().vscp;
        AstVarScope* ಧ್ರುವ maskVscp = vscpInfo.shadowVarMaskedKit().maskp;

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsClonep = nodep->lhsp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ಧ್ರುವ refp) {
            ಯದಿ (!refp->access().isWriteOnly()) ಪ್ರತ್ಯಾಗಮ;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
        // Set the same bits in the mask to 1
        lhsClonep->foreach([&](AstVarRef* ಧ್ರುವ refp) {
            ಯದಿ (!refp->access().isWriteOnly()) ಪ್ರತ್ಯಾಗಮ;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(maskVscp);
            refp->varp(maskVscp->varp());
        });
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        AstConst* ಧ್ರುವ onesp = ನೂತನ AstConst{flp, AstConst::DTyped{}, lhsClonep->dtypep()};
        onesp->ಅಂಕೆ().setAllBits1();
        nodep->addNextHere(ನೂತನ AstAssign{flp, lhsClonep, onesp});
    }

    // Scheme::FlagShared
    ಶೂನ್ಯ prepareSchemeFlagShared(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
        AstScope* ಧ್ರುವ scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* ಧ್ರುವ activep = ನೂತನ AstActive{flp, "nba-flag-shared", vscpInfo.senTreep()};
        activep->sen_ವೃಕ್ಷ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        vscpInfo.flagSharedKit().activep = activep;
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ಧ್ರುವ postp = ನೂತನ AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagSharedKit().postp = postp;
        // Initialize
        vscpInfo.flagSharedKit().commitFlagp = nullptr;
        vscpInfo.flagSharedKit().commitIfp = nullptr;
    }
    ಶೂನ್ಯ convertSchemeFlagShared(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
        AstScope* ಧ್ರುವ scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ಧ್ರುವ std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Is this NBA adjacent after the previously processed NBA?
        ಧ್ರುವ ತಾರ್ಕಿಕ consecutive = nodep == m_nextDlyp;
        m_nextDlyp = VN_CAST(nodep->nextp(), AssignDly);

        VarScopeInfo* ಧ್ರುವ prevVscpInfop = consecutive ? &m_vscpInfo(m_prevVscp) : nullptr;

        // We can reuse the flag of the previous assignment if:
        ಧ್ರುವ ತಾರ್ಕಿಕ reuseTheFlag =
            // Consecutive NBAs
            consecutive
            // ... that use the same scheme
            && prevVscpInfop->m_scheme == Scheme::FlagShared
            // ... and are in the same scope as the target variable
            && scopep == vscp->scopep()
            // ... and share the same overall update domain
            && prevVscpInfop->senTreep()->same_ವೃಕ್ಷ(vscpInfo.senTreep());

        ಯದಿ (!reuseTheFlag) {
            // Create new flag
            AstVarScope* ಧ್ರುವ flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
            // Set the flag at the original NBA
            nodep->addHereThisAsNext(  //
                ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, flagVscp, VAccess::WRITE},
                              ನೂತನ AstConst{flp, AstConst::BitTrue{}}});
            // Add the 'Pre' scheduled reset for the flag
            AstAlwaysPre* ಧ್ರುವ prep = ನೂತನ AstAlwaysPre{flp};
            vscpInfo.flagSharedKit().activep->addStmtsp(prep);
            prep->addStmtsp(ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, flagVscp, VAccess::WRITE},
                                          ನೂತನ AstConst{flp, AstConst::BitFalse{}}});
            // Add the 'Post' scheduled commit
            AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, ನೂತನ AstVarRef{flp, flagVscp, VAccess::READ}};
            vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
            vscpInfo.flagSharedKit().commitFlagp = flagVscp;
            vscpInfo.flagSharedKit().commitIfp = ifp;
        } ಅನ್ಯಥಾ {
            ಯದಿ (vscp != m_prevVscp) {
                // Different variable, ensure the commit block exists for this variable,
                // can reuse existing one with the same flag, otherwise create a new one.
                AstVarScope* ಧ್ರುವ flagVscp = prevVscpInfop->flagSharedKit().commitFlagp;
                UASSERT_OBJ(flagVscp, nodep, "Commit flag of previous assignment should exist");
                ಯದಿ (vscpInfo.flagSharedKit().commitFlagp != flagVscp) {
                    AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, ನೂತನ AstVarRef{flp, flagVscp, VAccess::READ}};
                    vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
                    vscpInfo.flagSharedKit().commitFlagp = flagVscp;
                    vscpInfo.flagSharedKit().commitIfp = ifp;
                }
            } ಅನ್ಯಥಾ {
                // Same variable, reuse the commit block of the previous assignment
                vscpInfo.flagSharedKit().commitFlagp = prevVscpInfop->flagSharedKit().commitFlagp;
                vscpInfo.flagSharedKit().commitIfp = prevVscpInfop->flagSharedKit().commitIfp;
            }
            ++m_nSharedSetFlags;
        }
        // Commit the captured value to the captured destination
        vscpInfo.flagSharedKit().commitIfp->addThensp(
            ನೂತನ AstAssign{flp, capturedLhsp, capturedRhsp});

        // Remember the variable for the next NBA
        m_prevVscp = vscp;

        // Delete original NBA
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::FlagUnique
    ಶೂನ್ಯ prepareSchemeFlagUnique(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
        AstScope* ಧ್ರುವ scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* ಧ್ರುವ activep = ನೂತನ AstActive{flp, "nba-flag-unique", vscpInfo.senTreep()};
        activep->sen_ವೃಕ್ಷ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ಧ್ರುವ postp = ನೂತನ AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagUniqueKit().postp = postp;
    }
    ಶೂನ್ಯ convertSchemeFlagUnique(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
        AstScope* ಧ್ರುವ scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ಧ್ರುವ std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Create new flag
        AstVarScope* ಧ್ರುವ flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
        flagVscp->varp()->setIgnorePostWrite();
        // Set the flag at the original NBA
        nodep->addHereThisAsNext(  //
            ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, flagVscp, VAccess::WRITE},
                          ನೂತನ AstConst{flp, AstConst::BitTrue{}}});
        // Add the 'Post' scheduled commit
        AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, ನೂತನ AstVarRef{flp, flagVscp, VAccess::READ}};
        vscpInfo.flagUniqueKit().postp->addStmtsp(ifp);
        // Immediately clear the flag
        ifp->addThensp(ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, flagVscp, VAccess::WRITE},
                                     ನೂತನ AstConst{flp, AstConst::BitFalse{}}});
        // Commit the value
        ifp->addThensp(ನೂತನ AstAssign{flp, capturedLhsp, capturedRhsp});

        // Delete original NBA
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::ValueQueuePartial/Scheme::ValueQueueWhole
    template <ತಾರ್ಕಿಕ N_Partial>
    ಶೂನ್ಯ prepare_Scheme_ಮೌಲ್ಯ_Queue(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(N_Partial ? vscpInfo.m_scheme == Scheme::ಮೌಲ್ಯ_Queue_Partial
                              : vscpInfo.m_scheme == Scheme::ಮೌಲ್ಯ_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
        AstScope* ಧ್ರುವ scopep = vscp->scopep();

        // Create the commit queue variable
        Ast_ಗಂಟು_DType* ಧ್ರುವ elemDTypep = vscp->dtypep()->skipRefp();
        AstNBACommitQueueDType*& cqDTypep = m_cqDTypeps[{elemDTypep, N_Partial}];
        ಯದಿ (!cqDTypep) {
            cqDTypep = ನೂತನ AstNBACommitQueueDType{flp, elemDTypep, N_Partial};
            v3Global.rootp()->typeTablep()->addTypesp(cqDTypep);
        }
        ಧ್ರುವ std::string name = "CommitQueue__" + vscp->varp()->shortName();
        AstVarScope* ಧ್ರುವ queueVscp = m_dlyTmps.make(flp, scopep, cqDTypep, name);
        queueVscp->varp()->noReset(ಸತ್ಯ);
        queueVscp->varp()->setIgnorePostWrite();
        vscpInfo.ಮೌಲ್ಯ_Queue_Kit().vscp = queueVscp;
        // Create the AstActive for the Post logic
        AstActive* ಧ್ರುವ activep
            = ನೂತನ AstActive{flp, "nba-value-queue-whole", vscpInfo.senTreep()};
        activep->sen_ವೃಕ್ಷ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit
        AstAlwaysPost* ಧ್ರುವ postp = ನೂತನ AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit
        AstCMethodHard* ಧ್ರುವ callp = ನೂತನ AstCMethodHard{
            flp, ನೂತನ AstVarRef{flp, queueVscp, VAccess::READWRITE}, VCMethod::NBA_COMMIT};
        callp->dtypeSetVoid();
        // TODO: this is a partial update, so must be READWRITE, but that breaks scheduling
        callp->addPinsp(ನೂತನ AstVarRef{flp, vscp, VAccess::WRITE});
        postp->addStmtsp(callp->makeStmt());
    }

    ಶೂನ್ಯ convert_Scheme_ಮೌಲ್ಯ_Queue(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo,
                                 ತಾರ್ಕಿಕ partial) {
        UASSERT_OBJ(partial ? vscpInfo.m_scheme == Scheme::ಮೌಲ್ಯ_Queue_Partial
                            : vscpInfo.m_scheme == Scheme::ಮೌಲ್ಯ_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
        AstScope* ಧ್ರುವ scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ಧ್ರುವ std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // RHS value (can be widened/masked iff Partial)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = capturedRhsp;
        // RHS mask (iff Partial)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* maskp = nullptr;

        // Running node for LHS deconstruction
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsNodep = capturedLhsp;

        // If partial updates are required, construct the mask and the widened value
        ಯದಿ (partial) {
            // Type of array element
            Ast_ಗಂಟು_DType* ಧ್ರುವ eDTypep = [&]() -> Ast_ಗಂಟು_DType* {
                Ast_ಗಂಟು_DType* dtypep = vscp->dtypep()->skipRefp();
                ಯಾವತ್ (AstUnpackArrayDType* ಧ್ರುವ ap = VN_CAST(dtypep, UnpackArrayDType)) {
                    dtypep = ap->subDTypep()->skipRefp();
                }
                ಪ್ರತ್ಯಾಗಮ dtypep;
            }();

            ಯದಿ (ಧ್ರುವ AstSel* ಧ್ರುವ lSelp = VN_CAST(lhsNodep, Sel)) {
                // This is a partial assignment.
                // Need to create a mask and widen the value to element size.
                lhsNodep = lSelp->fromp();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sLsbp = lSelp->lsbp();
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ sWidth = lSelp->widthConst();

                // Create mask value
                maskp = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                    // Constant mask we can compute here
                    ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ cLsbp = VN_CAST(sLsbp, Const)) {
                        AstConst* ಧ್ರುವ cp = ನೂತನ AstConst{flp, AstConst::DTyped{}, eDTypep};
                        cp->ಅಂಕೆ().setMask(sWidth, cLsbp->toSInt());
                        ಪ್ರತ್ಯಾಗಮ cp;
                    }

                    // A non-constant mask we must compute at run-time.
                    AstConst* ಧ್ರುವ onesp = ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, sWidth, 0};
                    onesp->ಅಂಕೆ().setAllBits1();
                    ಪ್ರತ್ಯಾಗಮ createWidened(flp, scopep, eDTypep, sLsbp, sWidth, "Mask" + baseName,
                                         onesp, nodep);
                }();

                // Adjust value to element size
                valuep = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                    // Constant value with constant select we can compute here
                    ಯದಿ (AstConst* ಧ್ರುವ cValuep = VN_CAST(valuep, Const)) {
                        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ cLsbp = VN_CAST(sLsbp, Const)) {
                            AstConst* ಧ್ರುವ cp = ನೂತನ AstConst{flp, AstConst::DTyped{}, eDTypep};
                            cp->ಅಂಕೆ().setAllBits0();
                            cp->ಅಂಕೆ().opSelInto(cValuep->ಅಂಕೆ(), cLsbp->toSInt(), sWidth);
                            VL_DO_DANGLING(valuep->delete_ವೃಕ್ಷ(), valuep);
                            ಪ್ರತ್ಯಾಗಮ cp;
                        }
                    }

                    // A non-constant value we must adjust.
                    ಪ್ರತ್ಯಾಗಮ createWidened(flp, scopep, eDTypep, sLsbp, sWidth,  //
                                         "Elem" + baseName, valuep, nodep);
                }();
            } ಅನ್ಯಥಾ {
                // If this assignment is not partial, set mask to ones and we are done
                AstConst* ಧ್ರುವ ones = ನೂತನ AstConst{flp, AstConst::DTyped{}, eDTypep};
                ones->ಅಂಕೆ().setAllBits1();
                maskp = ones;
            }
        }

        // Extract array indices
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> idxps;
        {
            UASSERT_OBJ(VN_IS(lhsNodep, ArraySel), lhsNodep, "Unexpected LHS form");
            ಯಾವತ್ (AstArraySel* ಧ್ರುವ aSelp = VN_CAST(lhsNodep, ArraySel)) {
                idxps.emplace_back(aSelp->bitp()->unlinkFrBack());
                lhsNodep = aSelp->fromp();
            }
            UASSERT_OBJ(VN_IS(lhsNodep, VarRef), lhsNodep, "Unexpected LHS form");
            std::reverse(idxps.begin(), idxps.end());
        }

        // Done with the LHS at this point
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(capturedLhsp), capturedLhsp);

        // Enqueue the update at the site of the original NBA
        AstCMethodHard* ಧ್ರುವ callp = ನೂತನ AstCMethodHard{
            flp, ನೂತನ AstVarRef{flp, vscpInfo.ಮೌಲ್ಯ_Queue_Kit().vscp, VAccess::READWRITE},
            VCMethod::NBA_ENQUEUE};
        callp->dtypeSetVoid();
        callp->addPinsp(valuep);
        ಯದಿ (partial) callp->addPinsp(maskp);
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ indexp : idxps) callp->addPinsp(indexp);
        nodep->addHereThisAsNext(callp->makeStmt());

        // Delete original NBA
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Record where a variable is assigned
    ಶೂನ್ಯ recordWriteRef(AstVarRef* nodep, ತಾರ್ಕಿಕ nonBlocking) {
        // Ignore references in certain contexts
        ಯದಿ (m_ignoreBlkAndNBlk) ಪ್ರತ್ಯಾಗಮ;
        // Ignore if it's an array
        // TODO: we do this because it used to be the previous behaviour.
        //       Is it still required, or should we warn for arrays as well?
        //       Scheduling is no different for them...
        //       Clarification: This is OK for arrays of primitive types, but
        //       arrays that use the ShadowVar scheme don't work...
        ಯದಿ (VN_IS(nodep->varScopep()->dtypep()->skipRefp(), UnpackArrayDType)) ಪ್ರತ್ಯಾಗಮ;

        m_writeRefs(nodep->varScopep()).emplace_back(nodep, nonBlocking, m_inNonCombLogic);
    }

    template <typename Procedure_T>
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isProcedureWithSentreep(ಧ್ರುವ Ast_ಗಂಟು_Procedure* ಧ್ರುವ nodep) {
        ಧ್ರುವ Procedure_T* ಧ್ರುವ procedurep = Ast_ಗಂಟು::cast<Procedure_T>(nodep);
        ಪ್ರತ್ಯಾಗಮ procedurep && procedurep->sentreep();
    }

    // Visit AstCFunc from a AstNodeCCall - this is made into a separate quasi-visitor because
    // AstCFunc that is not called from the code (e.g.: DPI exports) does not need to be visited
    // this way. Also, not visiting such AstCFuncs allows to avoid caching results for them which
    // this function does - which could lead to excessive memory usage
    ಶೂನ್ಯ visitCalledCFunc(AstCFunc* ಧ್ರುವ nodep) {
        CFuncCache& ಮೌಲ್ಯ = m_cfuncsCache(nodep);
        ವಿಕಲ್ಪ (ಮೌಲ್ಯ.m_state) {
        ಪಕ್ಷ CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_timingDomains);

            // Visit
            ಮೌಲ್ಯ.m_state = CFuncCache::VISITING;
            m_call_ರಾಶಿ.ತಳ್ಳು_back(nodep);
            {
                VL_RESTORER(m_cfuncp);
                m_cfuncp = nodep;
                iterateChildren(nodep);
            }
            m_call_ರಾಶಿ.ತೆಗೆ_back();
            ಮೌಲ್ಯ.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_timingDomains, ಮೌಲ್ಯ.m_timingDomains);
        } ಭಙ್ಗ;
        ಪಕ್ಷ CFuncCache::VISITING: {
            ಪ್ರತಿ (size_t i = m_call_ರಾಶಿ.size() - 1; m_call_ರಾಶಿ.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ರಾಶಿ[i]).m_includes.ಸೇರಿಸು(nodep);
            }
            ಪ್ರತ್ಯಾಗಮ;  // Break recursion
        }
        ಪಕ್ಷ CFuncCache::INITIALIZED: ಭಙ್ಗ;
        }
        std::unordered_set<ಧ್ರುವ AstCFunc*> visited;
        add_CFunc_Cached_ಮೌಲ್ಯಗಳು(nodep, visited);
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstNetlist* nodep) override {
        iterateChildren(nodep);
        // Decide which scheme to use for each variable and do the 'prepare' step
        ಪ್ರತಿ (AstVarScope* ಧ್ರುವ vscp : m_vscps) {
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            vscpInfo.m_scheme = chooseScheme(vscp, vscpInfo);
            // Run 'prepare' step
            ವಿಕಲ್ಪ (vscpInfo.m_scheme) {
            ಪಕ್ಷ Scheme::Undecided:  // LCOV_EXCL_START
                UASSERT_OBJ(ಅಸತ್ಯ, vscp, "Failed to choose NBA scheme");
                ಭಙ್ಗ;  // LCOV_EXCL_STOP
            ಪಕ್ಷ Scheme::UnsupportedCompoundArrayInLoop: {
                // Will report error at the site of the NBA
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ Scheme::ShadowVar: {
                ++m_nSchemeShadowVar;
                prepareSchemeShadowVar(vscp, vscpInfo);
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ Scheme::ShadowVarMasked: {
                ++m_nSchemeShadowVarMasked;
                prepareSchemeShadowVarMasked(vscp, vscpInfo);
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ Scheme::FlagShared: {
                ++m_nSchemeFlagShared;
                prepareSchemeFlagShared(vscp, vscpInfo);
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ Scheme::FlagUnique: {
                ++m_nSchemeFlagUnique;
                prepareSchemeFlagUnique(vscp, vscpInfo);
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ Scheme::ಮೌಲ್ಯ_Queue_Whole: {
                ++m_n_Scheme_ಮೌಲ್ಯ_Queues_Whole;
                prepare_Scheme_ಮೌಲ್ಯ_Queue</* Partial: */ ಅಸತ್ಯ>(vscp, vscpInfo);
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ Scheme::ಮೌಲ್ಯ_Queue_Partial: {
                ++m_n_Scheme_ಮೌಲ್ಯ_Queues_Partial;
                prepare_Scheme_ಮೌಲ್ಯ_Queue</* Partial: */ ಸತ್ಯ>(vscp, vscpInfo);
                ಭಙ್ಗ;
            }
            }
        }
        // Convert all NBAs
        ಪ್ರತಿ (ಧ್ರುವ NBA& nba : m_nbas) {
            AstAssignDly* ಧ್ರುವ nbap = nba.nodep;
            AstVarScope* ಧ್ರುವ vscp = nba.vscp;
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            // Run 'convert' step
            ವಿಕಲ್ಪ (vscpInfo.m_scheme) {
            ಪಕ್ಷ Scheme::Undecided: {  // LCOV_EXCL_START
                UASSERT_OBJ(ಅಸತ್ಯ, vscp, "Unreachable");
                ಭಙ್ಗ;
            }  // LCOV_EXCL_STOP
            ಪಕ್ಷ Scheme::UnsupportedCompoundArrayInLoop: {
                // TODO: make this an E_UNSUPPORTED...
                nbap->v3warn(BLKLOOPINIT, "Unsupported: Non-blocking assignment to array with "
                                          "compound element type inside loop");
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ Scheme::ShadowVar: {
                convertSchemeShadowVar(nbap, vscp, vscpInfo);
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ Scheme::ShadowVarMasked: {
                convertSchemeShadowVarMasked(nbap, vscp, vscpInfo);
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ Scheme::FlagShared: {
                convertSchemeFlagShared(nbap, vscp, vscpInfo);
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ Scheme::FlagUnique: {
                convertSchemeFlagUnique(nbap, vscp, vscpInfo);
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ Scheme::ಮೌಲ್ಯ_Queue_Whole: {
                convert_Scheme_ಮೌಲ್ಯ_Queue(nbap, vscp, vscpInfo, /* partial: */ ಅಸತ್ಯ);
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ Scheme::ಮೌಲ್ಯ_Queue_Partial:
                convert_Scheme_ಮೌಲ್ಯ_Queue(nbap, vscp, vscpInfo, /* partial: */ ಸತ್ಯ);
                ಭಙ್ಗ;
            }
        }
    }
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstActive* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ignoreBlkAndNBlk);
        VL_RESTORER(m_inNonCombLogic);
        m_activep = nodep;
        ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = nodep->sentreep();
        m_ignoreBlkAndNBlk = senTreep->hasStatic() || senTreep->hasInitial();
        m_inNonCombLogic = senTreep->hasClocked();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override {
        VL_RESTORER(m_needsInitialTrigger);
        VL_RESTORER_CLEAR(m_nbaEventSenTreeps);
        ಧ್ರುವ size_t firstNBAAddedIndex = m_nbas.size();
        {
            VL_RESTORER(m_inSuspendableOrFork);
            VL_RESTORER(m_procp);
            VL_RESTORER(m_ignoreBlkAndNBlk);
            VL_RESTORER(m_inNonCombLogic);
            // When we are dealing with initial block we need to
            // treat it as suspendable when we meet a NBA
            m_inSuspendableOrFork = nodep->isSuspendable() || VN_IS(nodep, Initial);
            m_procp = nodep;
            ಯದಿ (nodep->isSuspendable()) {
                m_ignoreBlkAndNBlk = ಅಸತ್ಯ;
                m_inNonCombLogic = ಸತ್ಯ;
            }
            iterateChildren(nodep);
        }
        auto containsClocled = [](ಧ್ರುವ AstSenItem* itemp) {
            ಯಾವತ್ (itemp) {
                ಯದಿ (itemp->edgeType().clockedStmt()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                itemp = VN_AS(itemp->nextp(), SenItem);
            }
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        };
        ಧ್ರುವ ತಾರ್ಕಿಕ addInitialTrigger = m_needsInitialTrigger
                                       && !(isProcedureWithSentreep<AstAlways>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysObserved>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysReactive>(nodep))
                                       && !containsClocled(m_activep->sentreep()->sensesp());
        ಯದಿ (m_timingDomains.empty() && !addInitialTrigger) ಪ್ರತ್ಯಾಗಮ;

        // There were some timing domains involved in the process. Add all of them as sensitivities
        // of all NBA targets in this process. Note this is a bit of a sledgehammer, we should only
        // need those that directly precede the NBA in control flow, but that is hard to compute,
        // so we will hammer away.

        // First gather all senItems
        AstSenItem* senItemp = nullptr;
        ಯದಿ (addInitialTrigger) {
            senItemp = ನೂತನ AstSenItem{nodep->fileline(), AstSenItem::InitialNBA{}};
            ++m_nInitialNBA;
        }

        ಪ್ರತಿ (ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ domainp : m_timingDomains) {
            ಯದಿ (domainp->sensesp())
                senItemp = Ast_ಗಂಟು::addNext(senItemp, domainp->sensesp()->clone_ವೃಕ್ಷ(ಸತ್ಯ));
        }
        m_timingDomains.clear();
        // Add them to all nba targets we gathered in this process
        ಪ್ರತಿ (size_t i = firstNBAAddedIndex; i < m_nbas.size(); ++i) {
            m_vscpInfo(m_nbas[i].vscp).addSensitivity(senItemp);
        }
        ಪ್ರತಿ (Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep : m_nbaEventSenTreeps)
            senTreep->addSensesp(senItemp->clone_ವೃಕ್ಷ(ಸತ್ಯ));
        // Done with these
        VL_DO_DANGLING(senItemp->delete_ವೃಕ್ಷ(), senItemp);
    }
    ಶೂನ್ಯ visit(AstFork* nodep) override {
        VL_RESTORER(m_inSuspendableOrFork);
        m_inSuspendableOrFork = ಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCAwait* nodep) override {
        ಯದಿ (nodep->sentreep()) m_timingDomains.ಸೇರಿಸು(nodep->sentreep());
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstFireEvent* nodep) override {
        UASSERT_OBJ(v3Global.hasEvents(), nodep, "Inconsistent");
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ eventp = nodep->operandp()->unlinkFrBack();

        // Enqueue for clearing 'triggered' state on next eval
        AstCStmt* ಧ್ರುವ cstmtp = ನೂತನ AstCStmt{flp};
        cstmtp->add("vlSymsp->fireEvent(");
        cstmtp->add(eventp);
        cstmtp->add(");");

        Ast_ಗಂಟು* newp = cstmtp;
        ಧ್ರುವ AstVarRef* ಧ್ರುವ vrefp = VN_CAST(eventp, VarRef);
        ಯದಿ (nodep->isDelayed() && (m_cfuncp || !vrefp)) {
            // V3Timing converts these with --timing
            nodep->v3warn(E_NOTIMING, "Nonblocking event trigger "
                                          << (m_cfuncp ? "in a non-inlined function/task"
                                                       : "of a class or interface member or "
                                                         "array element")
                                          << " requires --timing");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isDelayed()) {
            ಧ್ರುವ std::string newvarname = "_" + vrefp->varp()->shortName();
            AstVarScope* ಧ್ರುವ dlyvscp
                = m_dlyTmps.make(flp, vrefp->varScopep()->scopep(), 1, newvarname);

            ಧ್ರುವ auto dlyRef = [=](VAccess access) {  //
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, dlyvscp, access};
            };

            AstAlwaysPost* ಧ್ರುವ postp = ನೂತನ AstAlwaysPost{flp};
            AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, dlyRef(VAccess::READ)};
            postp->addStmtsp(ifp);

            UASSERT_OBJ(m_activep, nodep, "No active to handle FireEvent");
            Ast_Sen_ವೃಕ್ಷ* senTreep = m_activep->sentreep();
            AstActive* activep = nullptr;
            ಯದಿ (m_inSuspendableOrFork && (senTreep->hasInitial() || senTreep->hasClocked())) {
                // Suspendable code can trigger whenever it resumes, so fire when its NBAs are
                // committed (see visit(AstNodeProcedure*)), clearing the flag like
                // Scheme::FlagUnique
                senTreep = ನೂತನ Ast_Sen_ವೃಕ್ಷ{
                    flp, senTreep->hasClocked() ? senTreep->sensesp()->clone_ವೃಕ್ಷ(ಸತ್ಯ) : nullptr};
                m_nbaEventSenTreeps.ತಳ್ಳು_back(senTreep);
                m_needsInitialTrigger |= m_timingDomains.empty();
                dlyvscp->varp()->setIgnorePostWrite();
                ifp->addThensp(ನೂತನ AstAssign{flp, dlyRef(VAccess::WRITE),
                                             ನೂತನ AstConst{flp, AstConst::BitFalse{}}});
                activep = ನೂತನ AstActive{flp, "nba-event", senTreep};
                activep->sen_ವೃಕ್ಷ_Storep(senTreep);
            } ಅನ್ಯಥಾ {
                activep = ನೂತನ AstActive{flp, "nba-event", senTreep};
                AstAlwaysPre* ಧ್ರುವ prep = ನೂತನ AstAlwaysPre{flp};
                prep->addStmtsp(ನೂತನ AstAssign{flp, dlyRef(VAccess::WRITE),
                                              ನೂತನ AstConst{flp, AstConst::BitFalse{}}});
                activep->addStmtsp(prep);
            }
            ifp->addThensp(newp);
            m_activep->addNextHere(activep);
            activep->addStmtsp(postp);

            newp = ನೂತನ AstAssign{flp, dlyRef(VAccess::WRITE),
                                 ನೂತನ AstConst{flp, AstConst::BitTrue{}}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstAssignDly* nodep) override {
        // Prevent double processing due to AstExprStmt being moved before this node
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (m_cfuncp) {
            ಯದಿ (!v3Global.rootp()->nbaEventp()) {
                nodep->v3warn(
                    E_NOTIMING,
                    "Delayed assignment in a non-inlined function/task requires --timing");
            }
            ಪ್ರತ್ಯಾಗಮ;
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
        nodep->lhsp()->foreach([&](Ast_ಗಂಟು* currp) {
            // cppcheck-suppress constVariablePointer
            ಯದಿ (Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಧ್ರುವ exprp = VN_CAST(currp, ಅಭಿವ್ಯಕ್ತಿ_Stmt)) {
                // Move statements before the NBA
                nodep->addHereThisAsNext(exprp->stmtsp()->unlinkFrBackWithNext());
                // Replace with result
                currp->replaceWith(exprp->resultp()->unlinkFrBack());
                // Get rid of the AstExprStmt
                VL_DO_DANGLING2(ತಳ್ಳು_Deletep(currp), currp, exprp);
            } ಅನ್ಯಥಾ ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(currp, VarRef)) {
                // Ignore reads (e.g.: '_[*here*] <= _')
                ಯದಿ (refp->access().isReadOnly()) ಪ್ರತ್ಯಾಗಮ;
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
        AstVarScope* ಧ್ರುವ vscp = m_currNbaLhsRefp->varScopep();
        // Record it on first encounter
        VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
        ಯದಿ (!vscpInfo.m_firstNbaRefp) {
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
        ಯದಿ (m_activep->sentreep()->hasClocked()) {
            ಯದಿ (vscpInfo.m_fistActivep != m_activep) {
                AstVar* ಧ್ರುವ varp = vscp->varp();
                ಯದಿ (!varp->user1SetOnce()
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
        recordWriteRef(m_currNbaLhsRefp, ಸತ್ಯ);

        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        // Already checked the NBA LHS ref, ignore here
        ಯದಿ (nodep == m_currNbaLhsRefp) ಪ್ರತ್ಯಾಗಮ;
        // Only care about write refs
        ಯದಿ (!nodep->access().isWriteOrRW()) ಪ್ರತ್ಯಾಗಮ;
        // Record write reference
        recordWriteRef(nodep, ಅಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = ಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_CCall* ಧ್ರುವ nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ಧ್ರುವ auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        ಯದಿ (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } ಅನ್ಯಥಾ {
            ಪ್ರತಿ (AstCFunc* ಧ್ರುವ cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    ಶೂನ್ಯ visit(AstCFunc* ಧ್ರುವ nodep) override {
        ಧ್ರುವ auto& ಮೌಲ್ಯ = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        ಯದಿ (ಮೌಲ್ಯ.m_state != CFuncCache::UNINITIALIZED) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_cfuncp);
        // Its timing domains are those of the processes calling it, see visitCalledCFunc()
        VL_RESTORER_CLEAR(m_timingDomains);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }

    // Pre/Post logic are created here and their content need no further changes, so ignore.
    ಶೂನ್ಯ visit(AstAlwaysPre*) override {}
    ಶೂನ್ಯ visit(AstAlwaysPost*) override {}

    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
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
        V3Stats::addStat("NBA, variables using ValueQueueWhole scheme", m_n_Scheme_ಮೌಲ್ಯ_Queues_Whole);
        V3Stats::addStat("NBA, variables using ValueQueuePartial scheme",
                         m_n_Scheme_ಮೌಲ್ಯ_Queues_Partial);
        V3Stats::addStat("Optimizations, NBA flags shared", m_nSharedSetFlags);
        V3Stats::addStat("Procedures needing initial NBA trigger", m_nInitialNBA);
        V3Stats::addStat("Non-inlined co_awaits with SenTree", m_non_Inlined_CAwaits_With_Sen_ವೃಕ್ಷ);
    }
};

//######################################################################
// Delayed class functions

ಶೂನ್ಯ V3Delayed::delayedAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DelayedVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("delayed", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
