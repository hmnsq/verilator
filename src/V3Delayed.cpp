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
    ಎಣಿಕೆ ವರ್ಗ Scheme : uint8_t {
        Undecided = 0,
        Unsupported_Compound_Array_ಒಳ_Loop,
        ShadowVar,
        ShadowVarMasked,
        FlagShared,
        FlagUnique,
        ಮೌಲ್ಯ_ಸರತಿ_ಸಂಪೂರ್ಣ,
        ಮೌಲ್ಯ_ಸರತಿ_Partial
    };

    // All info associated with a variable that is the target of an NBA
    ವರ್ಗ VarScopeInfo final {
    ಸಾರ್ವಜನಿಕ:
        // First write reference encountered to the VarScope as the target on an NBA
        ಸ್ಥಿರಾಂಕ AstVarRef* m_ಮೊದಲ_Nba_Refp = ಶೂನ್ಯಸೂಚಕ;
        // Active block 'm_firstNbaRefp' is under
        ಸ್ಥಿರಾಂಕ Ast_ಸಕ್ರಿಯ* m_fistActivep = ಶೂನ್ಯಸೂಚಕ;
        ತಾರ್ಕಿಕ m_ಸಂಪೂರ್ಣ = ಸುಳ್ಳು;  // Used on LHS of NBA directly via VarRef
        ತಾರ್ಕಿಕ m_partial = ಸುಳ್ಳು;  // Used on LHS of NBA under a Sel
        ತಾರ್ಕಿಕ m_ಒಳ_Loop = ಸುಳ್ಳು;  // Used on LHS of NBA in a loop
        ತಾರ್ಕಿಕ m_ಒಳ_Susp_Or_Fork = ಸುಳ್ಳು;  // Used on LHS of NBA in suspendable process or fork
        Scheme m_scheme = Scheme::Undecided;  // Conversion scheme to use for this variable

    ಖಾಸಗಿ:
        // Combined sensitivities of all NBAs targeting this variable
        Ast_Sen_ವೃಕ್ಷ* m_senTreep = ಶೂನ್ಯಸೂಚಕ;

        // Union of stuff needed for the various schemes - use accessors below!
        ಒಕ್ಕೂಟ {
            ರಚನೆ {  // Stuff needed for Scheme::ShadowVar
                AstVarScope* vscp;  // The shadow variable
            } m_shadowVariableKit;
            ರಚನೆ {  // Stuff needed for Scheme::ShadowVarMasked
                AstVarScope* vscp;  // The shadow variable
                AstVarScope* maskp;  // The mask variable
            } m_shadowVarMaskedKit;
            ರಚನೆ {  // Stuff needed for Scheme::FlagShared
                Ast_ಸಕ್ರಿಯ* activep;  // The active block for the Pre/Post logic
                AstAlwaysPost* postp;  // The post block for commiting results
                AstVarScope* commitFlagp;  // The commit flag variable, for reuse
                AstIf* commitIfp;  // The previous if statement for committing, for reuse
            } m_flagSharedKit;
            ರಚನೆ {  // Stuff needed for Scheme::FlagUnique
                AstAlwaysPost* postp;  // The post block for commiting results
            } m_flagUniqueKit;
            ರಚನೆ {  // Stuff needed for Scheme::ValueQueueWhole/Scheme::ValueQueuePartial
                AstVarScope* vscp;  // The commit queue variable
            } m_ಮೌಲ್ಯ_ಸರತಿ_Kit;
        } m_kitUnion;

    ಸಾರ್ವಜನಿಕ:
        VarScopeInfo() = ಪೂರ್ವನಿಯೋಜಿತ;
        ~VarScopeInfo() {
            // Might not be linked if there was an error
            ಆದರೆ (!m_senTreep->backp()) VL_DO_DANGLING(m_senTreep->delete_ವೃಕ್ಷ(), m_senTreep);
        }
        // Accessors for the above union fields
        ಸ್ವಯಂಚಾಲಿತ& shadowVariableKit() {
            UASSERT(m_scheme == Scheme::ShadowVar, "Inconsistent Scheme");
            ಹಿಂತಿರುಗಿ m_kitUnion.m_shadowVariableKit;
        }
        ಸ್ವಯಂಚಾಲಿತ& shadowVarMaskedKit() {
            UASSERT(m_scheme == Scheme::ShadowVarMasked, "Inconsistent Scheme");
            ಹಿಂತಿರುಗಿ m_kitUnion.m_shadowVarMaskedKit;
        }
        ಸ್ವಯಂಚಾಲಿತ& flagSharedKit() {
            UASSERT(m_scheme == Scheme::FlagShared, "Inconsistent Scheme");
            ಹಿಂತಿರುಗಿ m_kitUnion.m_flagSharedKit;
        }
        ಸ್ವಯಂಚಾಲಿತ& flagUniqueKit() {
            UASSERT(m_scheme == Scheme::FlagUnique, "Inconsistent Scheme");
            ಹಿಂತಿರುಗಿ m_kitUnion.m_flagUniqueKit;
        }
        ಸ್ವಯಂಚಾಲಿತ& ಮೌಲ್ಯ_ಸರತಿ_Kit() {
            UASSERT(m_scheme == Scheme::ಮೌಲ್ಯ_ಸರತಿ_Partial || m_scheme == Scheme::ಮೌಲ್ಯ_ಸರತಿ_ಸಂಪೂರ್ಣ,
                    "Inconsistent Scheme");
            ಹಿಂತಿರುಗಿ m_kitUnion.m_ಮೌಲ್ಯ_ಸರತಿ_Kit;
        }

        // Accessor
        Ast_Sen_ವೃಕ್ಷ* senTreep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_senTreep; }

        // Add sensitivities
        ಶೂನ್ಯ ಕೂಡಿಸು_ಸಂವೇದನಾಶೀಲತೆ(Ast_Sen_ಅಂಶ* nodep) {
            ಆದರೆ (!m_senTreep) m_senTreep = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{nodep->fileline(), ಶೂನ್ಯಸೂಚಕ};
            // Add a copy of each term
            m_senTreep->ಕೂಡಿಸು_Sensesp(nodep->clone_ವೃಕ್ಷ(ನಿಜ));
            // Remove duplicates
            V3Const::constifyExpensiveEdit(m_senTreep);
        }
        // cppcheck-suppress constParameterPointer
        ಶೂನ್ಯ ಕೂಡಿಸು_ಸಂವೇದನಾಶೀಲತೆ(Ast_Sen_ವೃಕ್ಷ* nodep) { ಕೂಡಿಸು_ಸಂವೇದನಾಶೀಲತೆ(nodep->sensesp()); }
    };

    // Data structure to keep track of all writes to
    ರಚನೆ WriteReference final {
        AstVarRef* m_refp = ಶೂನ್ಯಸೂಚಕ;  // The reference
        ತಾರ್ಕಿಕ m_isNBA = ಸುಳ್ಳು;  // True if an NBA write
        ತಾರ್ಕಿಕ m_ಒಳ_Non_Comb = ಸುಳ್ಳು;  // True if reference is known to be in non-combinational logic
        WriteReference() = ಪೂರ್ವನಿಯೋಜಿತ;
        WriteReference(AstVarRef* refp, ತಾರ್ಕಿಕ isNBA, ತಾರ್ಕಿಕ ಒಳ_Non_Comb)
            : m_refp{refp}
            , m_isNBA{isNBA}
            , m_ಒಳ_Non_Comb{ಒಳ_Non_Comb} {}
    };

    // Data required to lower AstAssignDelay later
    ರಚನೆ NBA final {
        Ast_ನಿಯೋಜಿಸು_Dly* nodep = ಶೂನ್ಯಸೂಚಕ;  // The NBA this record refers to
        AstVarScope* vscp = ಶೂನ್ಯಸೂಚಕ;  // The target variable the NBA is updating
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
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_user2_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use m_user3_ಒಳ_Use;

    ರಚನೆ CFuncCache final {
        VInsertion_ಗಣ<Ast_Sen_ವೃಕ್ಷ*> m_timingDomains;  // What shall be added to m_timingDomains
        std::ಗಣ<AstCFunc*>
            m_includes;  // CFuncs whose CFuncCache shall be included into this - this is used to
                         // break cycles: A->B->A (instead of visiting A while it is still begin
                         // visited B just marks that it includes A)
        ಎಣಿಕೆ State : uint8_t {
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
    AstUser3Allocator<AstVarScope, std::ಸದಿಶ<WriteReference>> m_writeRefs;

    // STATE - across all visitors
    VInsertion_ಗಣ<Ast_Sen_ವೃಕ್ಷ*> m_timingDomains;  // Timing resume domains
    V3SharedTmps m_dlyTmps{"__Vdly", VVarType::BLOCKTEMP};  // Temporary variables
    // Commit queue data types, by element type and partial flag. Shared by all instances,
    // as m_dlyTmps only shares variables with the same data type.
    std::map<std::pair<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType*, ತಾರ್ಕಿಕ>, Ast_NBACommit_ಸರತಿ_DType*> m_cqDTypeps;

    ಸ್ಥಿರಾಂಕ std::unique_ptr<V3_Class_ನಕ್ಷೆ>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ AstCFunc*> m_call_ರಾಶಿ;  // Current callstack of AstCFuncs

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ಸಕ್ರಿಯ* m_activep = ಶೂನ್ಯಸೂಚಕ;  // Current activate
    ಸ್ಥಿರಾಂಕ AstCFunc* m_cfuncp = ಶೂನ್ಯಸೂಚಕ;  // Current public C Function
    Ast_ಗಂಟು_Procedure* m_procp = ಶೂನ್ಯಸೂಚಕ;  // Current process
    AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // Current scope
    ತಾರ್ಕಿಕ m_ಒಳ_Loop = ಸುಳ್ಳು;  // True in for loops
    ತಾರ್ಕಿಕ m_ಒಳ_Suspendable_Or_Fork = ಸುಳ್ಳು;  // True in suspendable processes and forks
    ತಾರ್ಕಿಕ m_ignoreBlkAndNBlk = ಸುಳ್ಳು;  // Suppress delayed assignment BLKANDNBLK
    ತಾರ್ಕಿಕ m_ಒಳ_Non_Comb_Logic = ಸುಳ್ಳು;  // We are in non-combinational logic
    ತಾರ್ಕಿಕ m_needsInitialTrigger = ಸುಳ್ಳು;  // Whether a NodeProcedure needs a initial trigger
    std::ಸದಿಶ<Ast_Sen_ವೃಕ್ಷ*> m_nbaEventSenTreeps;  // Sensitivities of '->>' in the process
    AstVarRef* m_currNbaLhsRefp = ಶೂನ್ಯಸೂಚಕ;  // Current NBA LHS variable reference

    // STATE - during NBA conversion (after visit)
    std::ಸದಿಶ<NBA> m_nbas;  // AstAssignDly instances to lower at the end
    std::ಸದಿಶ<AstVarScope*> m_vscps;  // Target variables on LHSs of NBAs
    Ast_ನಿಯೋಜಿಸು_Dly* m_ಮುಂದಿನ_Dlyp = ಶೂನ್ಯಸೂಚಕ;  // The nextp of the previous AstAssignDly
    AstVarScope* m_prevVscp = ಶೂನ್ಯಸೂಚಕ;  // The target of the previous AstAssignDly

    // STATE - Statistic tracking
    VDouble0 m_nSchemeShadowVar;  // Number of variables using Scheme::ShadowVar
    VDouble0 m_nSchemeShadowVarMasked;  // Number of variabels using Scheme::ShadowVarMasked
    VDouble0 m_nSchemeFlagShared;  // Number of variables using Scheme::FlagShared
    VDouble0 m_nSchemeFlagUnique;  // Number of variables using Scheme::FlagUnique
    VDouble0 m_n_Scheme_ಮೌಲ್ಯ_Queues_ಸಂಪೂರ್ಣ;  //  Number of variables using Scheme::ValueQueueWhole
    VDouble0 m_n_Scheme_ಮೌಲ್ಯ_Queues_Partial;  //  Number of variables using Scheme::ValueQueuePartial
    VDouble0 m_n_Shared_ಗಣ_Flags;  // "Set" flags actually shared by Scheme::FlagShared variables
    VDouble0 m_nInitialNBA;  // Number of procedural blocks with initial NBA
    VDouble0 m_non_Inlined_CAwaits_With_Sen_ವೃಕ್ಷ;  // Count uses of not inlined co_awaits

    // METHODS

    // Return true iff a variable is assigned by both blocking and nonblocking
    // assignments. Issue BLKANDNBLK error if we can't prove the mixed
    // assignments are to independent bits and the blocking assignment can be
    // in combinational logic, which is something we can't safely implement
    // still.
    ತಾರ್ಕಿಕ checkMixedUsage(ಸ್ಥಿರಾಂಕ AstVarScope* vscp, ತಾರ್ಕಿಕ isIntegralOrPacked) {

        ರಚನೆ Ref final {
            AstVarRef* m_refp;  // The reference
            ತಾರ್ಕಿಕ m_ಒಳ_Non_Comb;  // True if known to be in non-combinational logic
            ಪೂರ್ಣಾಂಕ m_lsb;  // LSB of accessed range
            ಪೂರ್ಣಾಂಕ m_msb;  // MSB of accessed range
            Ref(AstVarRef* refp, ತಾರ್ಕಿಕ ಒಳ_Non_Comb, ಪೂರ್ಣಾಂಕ lsb, ಪೂರ್ಣಾಂಕ msb)
                : m_refp{refp}
                , m_ಒಳ_Non_Comb{ಒಳ_Non_Comb}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };

        std::ಸದಿಶ<Ref> blkRefs;  // Blocking writes
        std::ಸದಿಶ<Ref> nbaRefs;  // Non-blockign writes

        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = isIntegralOrPacked ? vscp->ಅಗಲ() : 1;

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& writeRef : m_writeRefs(vscp)) {
            ಪೂರ್ಣಾಂಕ lsb = 0;
            ಪೂರ್ಣಾಂಕ msb = ಅಗಲ - 1;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(writeRef.m_refp->backp(), Sel)) {
                ಆದರೆ (VN_IS(selp->lsbp(), Const)) {
                    lsb = selp->lsbConst();
                    msb = selp->msbConst();
                }
            }
            ಆದರೆ (writeRef.m_isNBA) {
                nbaRefs.emplace_ಹಿಂಭಾಗ(writeRef.m_refp, writeRef.m_ಒಳ_Non_Comb, lsb, msb);
            } ಇಲ್ಲದಿದ್ದರೆ {
                blkRefs.emplace_ಹಿಂಭಾಗ(writeRef.m_refp, writeRef.m_ಒಳ_Non_Comb, lsb, msb);
            }
        }
        // We only run this function on targets of NBAs, so there should be at least one...
        UASSERT_OBJ(!nbaRefs.ಖಾಲಿಗಳು(), vscp, "Did not record NBA write");
        // If no blocking upadte, then we are good
        ಆದರೆ (blkRefs.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // If the blocking assignment is in non-combinational logic (i.e.:
        // in logic that has an explicit trigger), then we can safely
        // implement it (there is no race between clocked logic and post
        // scheduled logic), so need not error
        blkRefs.erase(std::remove_if(blkRefs.begin(), blkRefs.ಕೊನೆ(),
                                     [](ಸ್ಥಿರಾಂಕ Ref& ref) { ಹಿಂತಿರುಗಿ ref.m_ಒಳ_Non_Comb; }),
                      blkRefs.ಕೊನೆ());

        // If nothing left, then we need not error
        ಆದರೆ (blkRefs.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ನಿಜ;

        // If not a packed variable, warn here as we can't prove independence
        ಆದರೆ (!isIntegralOrPacked) {
            ಸ್ಥಿರಾಂಕ Ref& blkRef = blkRefs.front();
            ಸ್ಥಿರಾಂಕ Ref& nbaRef = nbaRefs.front();
            vscp->v3warn(
                BLKANDNBLK,
                "Unsupported: Blocking and non-blocking assignments to same non-packed variable: "
                    << vscp->varp()->prettyNameQ() << '\n'
                    << vscp->warnContextPrimary() << '\n'
                    << blkRef.m_refp->warnOther() << "... Location of blocking assignment\n"
                    << blkRef.m_refp->warnContextSecondary() << '\n'
                    << nbaRef.m_refp->warnOther() << "... Location of nonblocking assignment\n"
                    << nbaRef.m_refp->warnContextSecondary());
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }

        // We need to error if we can't prove the written bits are independent

        // Sort refs by interval
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ lessThanRef = [](ಸ್ಥಿರಾಂಕ Ref& a, ಸ್ಥಿರಾಂಕ Ref& b) {
            ಆದರೆ (a.m_lsb != b.m_lsb) ಹಿಂತಿರುಗಿ a.m_lsb < b.m_lsb;
            ಹಿಂತಿರುಗಿ a.m_msb < b.m_msb;
        };
        std::stable_ವಿಂಗಡಿಸು(blkRefs.begin(), blkRefs.ಕೊನೆ(), lessThanRef);
        std::stable_ವಿಂಗಡಿಸು(nbaRefs.begin(), nbaRefs.ಕೊನೆ(), lessThanRef);
        // Iterate both vectors, checking for overlap
        ಸ್ವಯಂಚಾಲಿತ bIt = blkRefs.begin();
        ಸ್ವಯಂಚಾಲಿತ nIt = nbaRefs.begin();
        ಆಗ (bIt != blkRefs.ಕೊನೆ() && nIt != nbaRefs.ಕೊನೆ()) {
            ಆದರೆ (lessThanRef(*bIt, *nIt)) {
                ಆದರೆ (nIt->m_lsb <= bIt->m_msb) ನಿಲ್ಲಿಸು;  // Stop on Overlap
                ++bIt;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (bIt->m_lsb <= nIt->m_msb) ನಿಲ್ಲಿಸು;  // Stop on Overlap
                ++nIt;
            }
        }

        // If we found an overlapping range that cannot be safely implemented, then wran...
        ಆದರೆ (bIt != blkRefs.ಕೊನೆ() && nIt != nbaRefs.ಕೊನೆ()) {
            ಸ್ಥಿರಾಂಕ Ref& blkRef = *bIt;
            ಸ್ಥಿರಾಂಕ Ref& nbaRef = *nIt;
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

        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Choose the NBA scheme used for the given variable.
    Scheme ಆರಿಸು_Scheme(ಸ್ಥಿರಾಂಕ AstVarScope* vscp, ಸ್ಥಿರಾಂಕ VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::Undecided, vscp, "NBA scheme already decided");

        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = vscp->dtypep()->skipRefp();
        // Unpacked arrays
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ uaDTypep = VN_CAST(dtypep, UnpackArrayDType)) {
            // If whole array is target of NBA, use ShadowVar, but not in a suspendable process or
            // fork, which resume outside the 'nba' region, so its 'pre' logic would overwrite the
            // value they write in the shadow variable
            ಆದರೆ (vscpInfo.m_ಸಂಪೂರ್ಣ) {
                ಹಿಂತಿರುಗಿ vscpInfo.m_ಒಳ_Susp_Or_Fork ? Scheme::FlagUnique : Scheme::ShadowVar;
            }
            // Basic underlying type of elements, if any.
            ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = uaDTypep->basicp();
            // If used in a loop, we must have a dynamic commit queue. (Also works in suspendables)
            ಆದರೆ (vscpInfo.m_ಒಳ_Loop) {
                // Arrays with compound element types are currently not supported in loops
                ಆದರೆ (!basicp
                    || !(basicp->isIntegralOrPacked()  //
                         || basicp->isDouble()  //
                         || basicp->isString())) {
                    ಹಿಂತಿರುಗಿ Scheme::Unsupported_Compound_Array_ಒಳ_Loop;
                }
                ಆದರೆ (vscpInfo.m_partial) ಹಿಂತಿರುಗಿ Scheme::ಮೌಲ್ಯ_ಸರತಿ_Partial;
                ಹಿಂತಿರುಗಿ Scheme::ಮೌಲ್ಯ_ಸರತಿ_ಸಂಪೂರ್ಣ;
            }
            // A suspendable process or fork can resume outside the 'nba' region, so the 'pre'
            // logic of the other schemes would undo its updates. The unique flag scheme has none.
            ಆದರೆ (vscpInfo.m_ಒಳ_Susp_Or_Fork) ಹಿಂತಿರುಗಿ Scheme::FlagUnique;
            // Otherwise if an array of packed/basic elements, use the shared flag scheme
            ಆದರೆ (basicp) ಹಿಂತಿರುಗಿ Scheme::FlagShared;
            // Finally fall back on the shadow variable scheme, e.g. for
            // arrays of unpacked structs. This will be slow.
            // TODO: generic LHS scheme as discussed in #5092
            ಹಿಂತಿರುಗಿ Scheme::ShadowVar;
        }

        // Likewise, the 'pre' logic of the shadow variable scheme would undo the updates of a
        // suspendable process or fork, so use the unique flag scheme
        ಆದರೆ (vscpInfo.m_ಒಳ_Susp_Or_Fork) ಹಿಂತಿರುಗಿ Scheme::FlagUnique;

        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isIntegralOrPacked = dtypep->isIntegralOrPacked();
        // Check for mixed usage (this also warns if not OK)
        ಆದರೆ (checkMixedUsage(vscp, isIntegralOrPacked)) {
            // If it's a variable updated by both blocking and non-blocking
            // assignments, use the ShadowVarMasked schem if masked update is
            // possible. This can handle blocking and non-blocking updates to
            // inpdendent parts correctly at run-time, and always works, even
            // in loops or other dynamic context.
            ಆದರೆ (isIntegralOrPacked) ಹಿಂತಿರುಗಿ Scheme::ShadowVarMasked;
            // If it's inside a loop, use Scheme::ShadowVar, which is safe,
            // but will generate incorrect code if a partial update is used
            ಆದರೆ (vscpInfo.m_ಒಳ_Loop) ಹಿಂತಿರುಗಿ Scheme::ShadowVar;
            // Otherwise (for not packed variables), use the FlagUnique scheme,
            // which at least handles partial updates correctly, but might break
            // in loops or other dynamic context
            ಹಿಂತಿರುಗಿ Scheme::FlagUnique;
        }

        // Otherwise use the simple shadow variable scheme
        ಹಿಂತಿರುಗಿ Scheme::ShadowVar;
    }

    // Given an expression 'exprp', return a new expression that always evaluates to the
    // value of the given expression at this point in the program. That is:
    // - If given a non-constant expression, create a new temporary AstVarScope under the given
    //   'scopep', with the given 'name', assign the expression to it, and return a read reference
    //   to the new AstVarScope.
    // - If given a constant, just return that constant.
    // New statements are inserted before 'insertp'
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* captureVal(AstScope* ಸ್ಥಿರಾಂಕ scopep, Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ insertp,
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp, ಸ್ಥಿರಾಂಕ std::string& name) {
        UASSERT_OBJ(!exprp->backp(), exprp, "Should have been unlinked");
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = exprp->fileline();
        ಆದರೆ (VN_IS(exprp, Const)) ಹಿಂತಿರುಗಿ exprp;
        // TODO: there are some const variables that could be simply referenced here
        AstVarScope* ಸ್ಥಿರಾಂಕ tmpVscp = m_dlyTmps.make(flp, scopep, exprp->dtypep(), name);
        insertp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, tmpVscp, VAccess::WRITE}, exprp});
        ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, tmpVscp, VAccess::READ};
    };

    // Similar to 'captureVal', but captures an LValue expression. That is, the returned
    // expression will reference the same location as the input expression, at this point in the
    // program.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* captureLhs(AstScope* ಸ್ಥಿರಾಂಕ scopep, Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ insertp,
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp, ಸ್ಥಿರಾಂಕ std::string& ತಳ_Name) {
        UASSERT_OBJ(!lhsp->backp(), lhsp, "Should have been unlinked");
        // Running node pointer
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep = lhsp;
        // Capture AstSel indices - there should be only one
        ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, Sel)) {
            ಸ್ಥಿರಾಂಕ std::string tmpName{"Lsb" + ತಳ_Name};
            selp->lsbp(captureVal(scopep, insertp, selp->lsbp()->unlink_Fr_ಹಿಂಭಾಗ(), tmpName));
            // Continue with target
            nodep = selp->fromp();
        }
        UASSERT_OBJ(!VN_IS(nodep, Sel), lhsp, "Multiple 'AstSel' applied to LHS reference");
        // Capture the indices of selects - might be many, also below struct members. If a handle
        // selects the instance the target is a member of, capture the handle instead, as it is
        // only read, to select the target when the NBA is executed (IEEE 1800-2023 10.4.2).
        ಗಾತ್ರ_t nArraySels = 0;
        ಆಗ (ನಿಜ) {
            ಆದರೆ (Ast_ಗಂಟು_Sel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, ಗಂಟು_Sel)) {
                ಸ್ಥಿರಾಂಕ std::string tmpName{"Dim" + std::ಗಮ್ಯ_string(nArraySels++) + ತಳ_Name};
                selp->bitp(captureVal(scopep, insertp, selp->bitp()->unlink_Fr_ಹಿಂಭಾಗ(), tmpName));
                nodep = selp->fromp();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstStructSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, StructSel)) {
                nodep = selp->fromp();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstMemberSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, MemberSel)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ handlep = selp->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
                V3LinkLValue::link_LValue_ಗಣ(handlep, VAccess::READ);
                AstVarRef* ಸ್ಥಿರಾಂಕ refp
                    = VN_AS(captureVal(scopep, insertp, handlep, "Handle" + ತಳ_Name), VarRef);
                refp->varp()->ಗಣ_Ignore_Post_Write();
                // Like the handle it replaces, mark it written, as is the member it selects
                refp->access(selp->access());
                selp->fromp(refp);
                ಹಿಂತಿರುಗಿ lhsp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ನಿಲ್ಲಿಸು;
            }
        }
        // What remains must be the variable, or some sort of select, we assume can reuse it
        UASSERT_OBJ(nodep->is_ಶುದ್ಧ(), lhsp, "Malformed LHS in NBA");
        // Now have been converted to use the captured values
        ಹಿಂತಿರುಗಿ lhsp;
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_CFunc_Cached_ಮೌಲ್ಯಗಳು(ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ cfuncp,
                              std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstCFunc*>& ಭೇಟಿಯಾದ) {
        ಆದರೆ (!ಭೇಟಿಯಾದ.ಸೇರಿಸು(cfuncp).ಎರಡನೇ) ಹಿಂತಿರುಗಿ;
        CFuncCache& ಮೌಲ್ಯ = m_cfuncsCache(cfuncp);
        m_timingDomains.ಸೇರಿಸು(ಮೌಲ್ಯ.m_timingDomains.begin(), ಮೌಲ್ಯ.m_timingDomains.ಕೊನೆ());
        m_non_Inlined_CAwaits_With_Sen_ವೃಕ್ಷ += ಮೌಲ್ಯ.m_timingDomains.ಗಾತ್ರ_2();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ includedp : ಮೌಲ್ಯ.m_includes) {
            ಕೂಡಿಸು_CFunc_Cached_ಮೌಲ್ಯಗಳು(includedp, ಭೇಟಿಯಾದ);
        }
    }

    // Create a temporary variable in the given 'scopep', with the given 'name', and with 'dtypep'
    // type, with the bits selected by 'sLsbp'/'sWidthp' set to 'valuep', other bits set to zero.
    // Insert new statements before 'insertp'.
    // Returns a read reference to the temporary variable.
    AstVarRef* createWidened(File_ಸಾಲು* flp, AstScope* scopep, Ast_ಗಂಟು_DType* dtypep,
                             Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sLsbp, ಪೂರ್ಣಾಂಕ s_ಅಗಲ, ಸ್ಥಿರಾಂಕ std::string& name,
                             Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, Ast_ಗಂಟು* insertp) {
        // Create temporary variable.
        AstVarScope* ಸ್ಥಿರಾಂಕ tp = m_dlyTmps.make(flp, scopep, dtypep, name);
        // Zero it
        AstConst* ಸ್ಥಿರಾಂಕ zerop = ಹೊಸ AstConst{flp, AstConst::DTyped{}, dtypep};
        zerop->ಅಂಕೆ().ಗಣ_All_Bits0();
        insertp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, tp, VAccess::WRITE}, zerop});
        // Set the selected bits to 'valuep'
        AstSel* ಸ್ಥಿರಾಂಕ selp = ಹೊಸ AstSel{flp, ಹೊಸ AstVarRef{flp, tp, VAccess::WRITE},
                                        sLsbp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ), s_ಅಗಲ};
        insertp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, selp, valuep});
        // This is the expression to get the value of the temporary
        ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, tp, VAccess::READ};
    }

    // Scheme::ShadowVar
    ಶೂನ್ಯ prepareSchemeShadowVar(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
        AstScope* ಸ್ಥಿರಾಂಕ scopep = vscp->scopep();
        // Create the shadow variable
        ಸ್ಥಿರಾಂಕ std::string name = "_" + vscp->varp()->shortName();
        AstVarScope* ಸ್ಥಿರಾಂಕ shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), name);
        vscpInfo.shadowVariableKit().vscp = shadowVscp;
        // Mark both for V3LifePsot
        vscp->optimizeLifePost(ನಿಜ);
        shadowVscp->optimizeLifePost(ನಿಜ);
        // Create the AstActive for the Pre/Post logic
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "nba-shadow-variable", vscpInfo.senTreep()};
        activep->sen_ವೃಕ್ಷ_Storep(vscpInfo.senTreep());
        scopep->ಕೂಡಿಸು_Blocksp(activep);
        // Add 'Pre' scheduled 'shadowVariable = originalVariable' assignment
        AstAlwaysPre* ಸ್ಥಿರಾಂಕ prep = ಹೊಸ AstAlwaysPre{flp};
        activep->ಕೂಡಿಸು_Stmtsp(prep);
        prep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, shadowVscp, VAccess::WRITE},
                                      ಹೊಸ AstVarRef{flp, vscp, VAccess::READ}});
        // Add 'Post' scheduled 'originalVariable = shadowVariable' assignment
        AstAlwaysPost* ಸ್ಥಿರಾಂಕ postp = ಹೊಸ AstAlwaysPost{flp};
        activep->ಕೂಡಿಸು_Stmtsp(postp);
        postp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, vscp, VAccess::WRITE},
                                       ಹೊಸ AstVarRef{flp, shadowVscp, VAccess::READ}});
    }
    ಶೂನ್ಯ convertSchemeShadowVar(Ast_ನಿಯೋಜಿಸು_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        AstVarScope* ಸ್ಥಿರಾಂಕ shadowVscp = vscpInfo.shadowVariableKit().vscp;

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ಸ್ಥಿರಾಂಕ refp) {
            ಆದರೆ (!refp->access().isWriteOnly()) ಹಿಂತಿರುಗಿ;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
    }

    // Scheme::ShadowVarMasked
    ಶೂನ್ಯ prepareSchemeShadowVarMasked(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
        AstScope* ಸ್ಥಿರಾಂಕ scopep = vscp->scopep();
        // Create the shadow variable
        ಸ್ಥಿರಾಂಕ std::string shadowName = "_" + vscp->varp()->shortName();
        AstVarScope* ಸ್ಥಿರಾಂಕ shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), shadowName);
        vscpInfo.shadowVarMaskedKit().vscp = shadowVscp;
        // Create the makk variable
        ಸ್ಥಿರಾಂಕ std::string ಮುಖವಾಡ_Name = "Mask__" + vscp->varp()->shortName();
        AstVarScope* ಸ್ಥಿರಾಂಕ ಮುಖವಾಡ_Vscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), ಮುಖವಾಡ_Name);
        ಮುಖವಾಡ_Vscp->varp()->ಗಣ_Ignore_Post_Write();
        vscpInfo.shadowVarMaskedKit().maskp = ಮುಖವಾಡ_Vscp;
        // Create the AstActive for the Post logic
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep
            = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "nba-shadow-var-masked", vscpInfo.senTreep()};
        activep->sen_ವೃಕ್ಷ_Storep(vscpInfo.senTreep());
        scopep->ಕೂಡಿಸು_Blocksp(activep);
        // Add 'Post' scheduled process for the commit and mask clear
        AstAlwaysPost* ಸ್ಥಿರಾಂಕ postp = ಹೊಸ AstAlwaysPost{flp};
        activep->ಕೂಡಿಸು_Stmtsp(postp);
        // Add the commit - vscp = (shadowVscp & maskVscp) | (vscp & ~maskVscp);
        postp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            flp, ಹೊಸ AstVarRef{flp, vscp, VAccess::WRITE},
            ಹೊಸ AstOr{flp,
                      ಹೊಸ AstAnd{flp, ಹೊಸ AstVarRef{flp, shadowVscp, VAccess::READ},
                                 ಹೊಸ AstVarRef{flp, ಮುಖವಾಡ_Vscp, VAccess::READ}},
                      ಹೊಸ AstAnd{flp, ಹೊಸ AstVarRef{flp, vscp, VAccess::READ},
                                 ಹೊಸ AstNot{flp, ಹೊಸ AstVarRef{flp, ಮುಖವಾಡ_Vscp, VAccess::READ}}}}});
        vscp->varp()->ಗಣ_Ignore_Post_Read();
        // Clar the mask - maskVscp = '0;
        postp->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, ಮುಖವಾಡ_Vscp, VAccess::WRITE},
                          ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, ಮುಖವಾಡ_Vscp->ಅಗಲ(), 0}});
    }
    ಶೂನ್ಯ convertSchemeShadowVarMasked(Ast_ನಿಯೋಜಿಸು_Dly* nodep, AstVarScope* vscp,
                                      VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        AstVarScope* ಸ್ಥಿರಾಂಕ shadowVscp = vscpInfo.shadowVarMaskedKit().vscp;
        AstVarScope* ಸ್ಥಿರಾಂಕ ಮುಖವಾಡ_Vscp = vscpInfo.shadowVarMaskedKit().maskp;

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsClonep = nodep->lhsp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ಸ್ಥಿರಾಂಕ refp) {
            ಆದರೆ (!refp->access().isWriteOnly()) ಹಿಂತಿರುಗಿ;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
        // Set the same bits in the mask to 1
        lhsClonep->foreach([&](AstVarRef* ಸ್ಥಿರಾಂಕ refp) {
            ಆದರೆ (!refp->access().isWriteOnly()) ಹಿಂತಿರುಗಿ;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(ಮುಖವಾಡ_Vscp);
            refp->varp(ಮುಖವಾಡ_Vscp->varp());
        });
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        AstConst* ಸ್ಥಿರಾಂಕ onesp = ಹೊಸ AstConst{flp, AstConst::DTyped{}, lhsClonep->dtypep()};
        onesp->ಅಂಕೆ().ಗಣ_All_Bits1();
        nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsClonep, onesp});
    }

    // Scheme::FlagShared
    ಶೂನ್ಯ prepareSchemeFlagShared(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
        AstScope* ಸ್ಥಿರಾಂಕ scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "nba-flag-shared", vscpInfo.senTreep()};
        activep->sen_ವೃಕ್ಷ_Storep(vscpInfo.senTreep());
        scopep->ಕೂಡಿಸು_Blocksp(activep);
        vscpInfo.flagSharedKit().activep = activep;
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ಸ್ಥಿರಾಂಕ postp = ಹೊಸ AstAlwaysPost{flp};
        activep->ಕೂಡಿಸು_Stmtsp(postp);
        vscpInfo.flagSharedKit().postp = postp;
        // Initialize
        vscpInfo.flagSharedKit().commitFlagp = ಶೂನ್ಯಸೂಚಕ;
        vscpInfo.flagSharedKit().commitIfp = ಶೂನ್ಯಸೂಚಕ;
    }
    ಶೂನ್ಯ convertSchemeFlagShared(Ast_ನಿಯೋಜಿಸು_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
        AstScope* ಸ್ಥಿರಾಂಕ scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ಸ್ಥಿರಾಂಕ std::string ತಳ_Name = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ(), "Val" + ತಳ_Name);

        // Unlink and capture the LHS reference
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), ತಳ_Name);

        // Is this NBA adjacent after the previously processed NBA?
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ consecutive = nodep == m_ಮುಂದಿನ_Dlyp;
        m_ಮುಂದಿನ_Dlyp = VN_CAST(nodep->nextp(), ನಿಯೋಜಿಸು_Dly);

        VarScopeInfo* ಸ್ಥಿರಾಂಕ prevVscpInfop = consecutive ? &m_vscpInfo(m_prevVscp) : ಶೂನ್ಯಸೂಚಕ;

        // We can reuse the flag of the previous assignment if:
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ reuseTheFlag =
            // Consecutive NBAs
            consecutive
            // ... that use the same scheme
            && prevVscpInfop->m_scheme == Scheme::FlagShared
            // ... and are in the same scope as the target variable
            && scopep == vscp->scopep()
            // ... and share the same overall update domain
            && prevVscpInfop->senTreep()->same_ವೃಕ್ಷ(vscpInfo.senTreep());

        ಆದರೆ (!reuseTheFlag) {
            // Create new flag
            AstVarScope* ಸ್ಥಿರಾಂಕ flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + ತಳ_Name);
            // Set the flag at the original NBA
            nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(  //
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, flagVscp, VAccess::WRITE},
                              ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}}});
            // Add the 'Pre' scheduled reset for the flag
            AstAlwaysPre* ಸ್ಥಿರಾಂಕ prep = ಹೊಸ AstAlwaysPre{flp};
            vscpInfo.flagSharedKit().activep->ಕೂಡಿಸು_Stmtsp(prep);
            prep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, flagVscp, VAccess::WRITE},
                                          ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}}});
            // Add the 'Post' scheduled commit
            AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, ಹೊಸ AstVarRef{flp, flagVscp, VAccess::READ}};
            vscpInfo.flagSharedKit().postp->ಕೂಡಿಸು_Stmtsp(ifp);
            vscpInfo.flagSharedKit().commitFlagp = flagVscp;
            vscpInfo.flagSharedKit().commitIfp = ifp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (vscp != m_prevVscp) {
                // Different variable, ensure the commit block exists for this variable,
                // can reuse existing one with the same flag, otherwise create a new one.
                AstVarScope* ಸ್ಥಿರಾಂಕ flagVscp = prevVscpInfop->flagSharedKit().commitFlagp;
                UASSERT_OBJ(flagVscp, nodep, "Commit flag of previous assignment should exist");
                ಆದರೆ (vscpInfo.flagSharedKit().commitFlagp != flagVscp) {
                    AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, ಹೊಸ AstVarRef{flp, flagVscp, VAccess::READ}};
                    vscpInfo.flagSharedKit().postp->ಕೂಡಿಸು_Stmtsp(ifp);
                    vscpInfo.flagSharedKit().commitFlagp = flagVscp;
                    vscpInfo.flagSharedKit().commitIfp = ifp;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Same variable, reuse the commit block of the previous assignment
                vscpInfo.flagSharedKit().commitFlagp = prevVscpInfop->flagSharedKit().commitFlagp;
                vscpInfo.flagSharedKit().commitIfp = prevVscpInfop->flagSharedKit().commitIfp;
            }
            ++m_n_Shared_ಗಣ_Flags;
        }
        // Commit the captured value to the captured destination
        vscpInfo.flagSharedKit().commitIfp->ಕೂಡಿಸು_Thensp(
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, capturedLhsp, capturedRhsp});

        // Remember the variable for the next NBA
        m_prevVscp = vscp;

        // Delete original NBA
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }

    // Scheme::FlagUnique
    ಶೂನ್ಯ prepareSchemeFlagUnique(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
        AstScope* ಸ್ಥಿರಾಂಕ scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "nba-flag-unique", vscpInfo.senTreep()};
        activep->sen_ವೃಕ್ಷ_Storep(vscpInfo.senTreep());
        scopep->ಕೂಡಿಸು_Blocksp(activep);
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ಸ್ಥಿರಾಂಕ postp = ಹೊಸ AstAlwaysPost{flp};
        activep->ಕೂಡಿಸು_Stmtsp(postp);
        vscpInfo.flagUniqueKit().postp = postp;
    }
    ಶೂನ್ಯ convertSchemeFlagUnique(Ast_ನಿಯೋಜಿಸು_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
        AstScope* ಸ್ಥಿರಾಂಕ scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ಸ್ಥಿರಾಂಕ std::string ತಳ_Name = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ(), "Val" + ತಳ_Name);

        // Unlink and capture the LHS reference
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), ತಳ_Name);

        // Create new flag
        AstVarScope* ಸ್ಥಿರಾಂಕ flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + ತಳ_Name);
        flagVscp->varp()->ಗಣ_Ignore_Post_Write();
        // Set the flag at the original NBA
        nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(  //
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, flagVscp, VAccess::WRITE},
                          ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}}});
        // Add the 'Post' scheduled commit
        AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, ಹೊಸ AstVarRef{flp, flagVscp, VAccess::READ}};
        vscpInfo.flagUniqueKit().postp->ಕೂಡಿಸು_Stmtsp(ifp);
        // Immediately clear the flag
        ifp->ಕೂಡಿಸು_Thensp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, flagVscp, VAccess::WRITE},
                                     ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}}});
        // Commit the value
        ifp->ಕೂಡಿಸು_Thensp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, capturedLhsp, capturedRhsp});

        // Delete original NBA
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }

    // Scheme::ValueQueuePartial/Scheme::ValueQueueWhole
    ಮಾದರಿ <ತಾರ್ಕಿಕ N_Partial>
    ಶೂನ್ಯ prepare_Scheme_ಮೌಲ್ಯ_ಸರತಿ(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(N_Partial ? vscpInfo.m_scheme == Scheme::ಮೌಲ್ಯ_ಸರತಿ_Partial
                              : vscpInfo.m_scheme == Scheme::ಮೌಲ್ಯ_ಸರತಿ_ಸಂಪೂರ್ಣ,
                    vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
        AstScope* ಸ್ಥಿರಾಂಕ scopep = vscp->scopep();

        // Create the commit queue variable
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDTypep = vscp->dtypep()->skipRefp();
        Ast_NBACommit_ಸರತಿ_DType*& cqDTypep = m_cqDTypeps[{elemDTypep, N_Partial}];
        ಆದರೆ (!cqDTypep) {
            cqDTypep = ಹೊಸ Ast_NBACommit_ಸರತಿ_DType{flp, elemDTypep, N_Partial};
            v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(cqDTypep);
        }
        ಸ್ಥಿರಾಂಕ std::string name = "CommitQueue__" + vscp->varp()->shortName();
        AstVarScope* ಸ್ಥಿರಾಂಕ ಸರತಿ_Vscp = m_dlyTmps.make(flp, scopep, cqDTypep, name);
        ಸರತಿ_Vscp->varp()->noReset(ನಿಜ);
        ಸರತಿ_Vscp->varp()->ಗಣ_Ignore_Post_Write();
        vscpInfo.ಮೌಲ್ಯ_ಸರತಿ_Kit().vscp = ಸರತಿ_Vscp;
        // Create the AstActive for the Post logic
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep
            = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "nba-value-queue-whole", vscpInfo.senTreep()};
        activep->sen_ವೃಕ್ಷ_Storep(vscpInfo.senTreep());
        scopep->ಕೂಡಿಸು_Blocksp(activep);
        // Add 'Post' scheduled process for the commit
        AstAlwaysPost* ಸ್ಥಿರಾಂಕ postp = ಹೊಸ AstAlwaysPost{flp};
        activep->ಕೂಡಿಸು_Stmtsp(postp);
        // Add the commit
        AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCMethodHard{
            flp, ಹೊಸ AstVarRef{flp, ಸರತಿ_Vscp, VAccess::READWRITE}, VCMethod::NBA_COMMIT};
        callp->dtype_ಗಣ_Void();
        // TODO: this is a partial update, so must be READWRITE, but that breaks scheduling
        callp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstVarRef{flp, vscp, VAccess::WRITE});
        postp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
    }

    ಶೂನ್ಯ convert_Scheme_ಮೌಲ್ಯ_ಸರತಿ(Ast_ನಿಯೋಜಿಸು_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo,
                                 ತಾರ್ಕಿಕ partial) {
        UASSERT_OBJ(partial ? vscpInfo.m_scheme == Scheme::ಮೌಲ್ಯ_ಸರತಿ_Partial
                            : vscpInfo.m_scheme == Scheme::ಮೌಲ್ಯ_ಸರತಿ_ಸಂಪೂರ್ಣ,
                    vscp, "Inconsistent NBA scheme");
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
        AstScope* ಸ್ಥಿರಾಂಕ scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ಸ್ಥಿರಾಂಕ std::string ತಳ_Name = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ(), "Val" + ತಳ_Name);

        // Unlink and capture the LHS reference
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), ತಳ_Name);

        // RHS value (can be widened/masked iff Partial)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = capturedRhsp;
        // RHS mask (iff Partial)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* maskp = ಶೂನ್ಯಸೂಚಕ;

        // Running node for LHS deconstruction
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsNodep = capturedLhsp;

        // If partial updates are required, construct the mask and the widened value
        ಆದರೆ (partial) {
            // Type of array element
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ eDTypep = [&]() -> Ast_ಗಂಟು_DType* {
                Ast_ಗಂಟು_DType* dtypep = vscp->dtypep()->skipRefp();
                ಆಗ (AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ ap = VN_CAST(dtypep, UnpackArrayDType)) {
                    dtypep = ap->subDTypep()->skipRefp();
                }
                ಹಿಂತಿರುಗಿ dtypep;
            }();

            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ lSelp = VN_CAST(lhsNodep, Sel)) {
                // This is a partial assignment.
                // Need to create a mask and widen the value to element size.
                lhsNodep = lSelp->fromp();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sLsbp = lSelp->lsbp();
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ s_ಅಗಲ = lSelp->ಅಗಲ_Const();

                // Create mask value
                maskp = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                    // Constant mask we can compute here
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ cLsbp = VN_CAST(sLsbp, Const)) {
                        AstConst* ಸ್ಥಿರಾಂಕ cp = ಹೊಸ AstConst{flp, AstConst::DTyped{}, eDTypep};
                        cp->ಅಂಕೆ().ಗಣ_ಮುಖವಾಡ(s_ಅಗಲ, cLsbp->ಗಮ್ಯ_SInt());
                        ಹಿಂತಿರುಗಿ cp;
                    }

                    // A non-constant mask we must compute at run-time.
                    AstConst* ಸ್ಥಿರಾಂಕ onesp = ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, s_ಅಗಲ, 0};
                    onesp->ಅಂಕೆ().ಗಣ_All_Bits1();
                    ಹಿಂತಿರುಗಿ createWidened(flp, scopep, eDTypep, sLsbp, s_ಅಗಲ, "Mask" + ತಳ_Name,
                                         onesp, nodep);
                }();

                // Adjust value to element size
                valuep = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                    // Constant value with constant select we can compute here
                    ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ cValuep = VN_CAST(valuep, Const)) {
                        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ cLsbp = VN_CAST(sLsbp, Const)) {
                            AstConst* ಸ್ಥಿರಾಂಕ cp = ಹೊಸ AstConst{flp, AstConst::DTyped{}, eDTypep};
                            cp->ಅಂಕೆ().ಗಣ_All_Bits0();
                            cp->ಅಂಕೆ().opSelInto(cValuep->ಅಂಕೆ(), cLsbp->ಗಮ್ಯ_SInt(), s_ಅಗಲ);
                            VL_DO_DANGLING(valuep->delete_ವೃಕ್ಷ(), valuep);
                            ಹಿಂತಿರುಗಿ cp;
                        }
                    }

                    // A non-constant value we must adjust.
                    ಹಿಂತಿರುಗಿ createWidened(flp, scopep, eDTypep, sLsbp, s_ಅಗಲ,  //
                                         "Elem" + ತಳ_Name, valuep, nodep);
                }();
            } ಇಲ್ಲದಿದ್ದರೆ {
                // If this assignment is not partial, set mask to ones and we are done
                AstConst* ಸ್ಥಿರಾಂಕ ones = ಹೊಸ AstConst{flp, AstConst::DTyped{}, eDTypep};
                ones->ಅಂಕೆ().ಗಣ_All_Bits1();
                maskp = ones;
            }
        }

        // Extract array indices
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> idxps;
        {
            UASSERT_OBJ(VN_IS(lhsNodep, ArraySel), lhsNodep, "Unexpected LHS form");
            ಆಗ (AstArraySel* ಸ್ಥಿರಾಂಕ aSelp = VN_CAST(lhsNodep, ArraySel)) {
                idxps.emplace_ಹಿಂಭಾಗ(aSelp->bitp()->unlink_Fr_ಹಿಂಭಾಗ());
                lhsNodep = aSelp->fromp();
            }
            UASSERT_OBJ(VN_IS(lhsNodep, VarRef), lhsNodep, "Unexpected LHS form");
            std::ಹಿಮ್ಮುಖ(idxps.begin(), idxps.ಕೊನೆ());
        }

        // Done with the LHS at this point
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(capturedLhsp), capturedLhsp);

        // Enqueue the update at the site of the original NBA
        AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCMethodHard{
            flp, ಹೊಸ AstVarRef{flp, vscpInfo.ಮೌಲ್ಯ_ಸರತಿ_Kit().vscp, VAccess::READWRITE},
            VCMethod::NBA_ಸರತಿಗೆ_ಸೇರಿಸು};
        callp->dtype_ಗಣ_Void();
        callp->ಕೂಡಿಸು_Pinsp(valuep);
        ಆದರೆ (partial) callp->ಕೂಡಿಸು_Pinsp(maskp);
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ indexp : idxps) callp->ಕೂಡಿಸು_Pinsp(indexp);
        nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(callp->makeStmt());

        // Delete original NBA
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }

    // Record where a variable is assigned
    ಶೂನ್ಯ ದಾಖಲೆ_Write_Ref(AstVarRef* nodep, ತಾರ್ಕಿಕ nonBlocking) {
        // Ignore references in certain contexts
        ಆದರೆ (m_ignoreBlkAndNBlk) ಹಿಂತಿರುಗಿ;
        // Ignore if it's an array
        // TODO: we do this because it used to be the previous behaviour.
        //       Is it still required, or should we warn for arrays as well?
        //       Scheduling is no different for them...
        //       Clarification: This is OK for arrays of primitive types, but
        //       arrays that use the ShadowVar scheme don't work...
        ಆದರೆ (VN_IS(nodep->varScopep()->dtypep()->skipRefp(), UnpackArrayDType)) ಹಿಂತಿರುಗಿ;

        m_writeRefs(nodep->varScopep()).emplace_ಹಿಂಭಾಗ(nodep, nonBlocking, m_ಒಳ_Non_Comb_Logic);
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು Procedure_T>
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isProcedureWithSentreep(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Procedure* ಸ್ಥಿರಾಂಕ nodep) {
        ಸ್ಥಿರಾಂಕ Procedure_T* ಸ್ಥಿರಾಂಕ procedurep = Ast_ಗಂಟು::cast<Procedure_T>(nodep);
        ಹಿಂತಿರುಗಿ procedurep && procedurep->sentreep();
    }

    // Visit AstCFunc from a AstNodeCCall - this is made into a separate quasi-visitor because
    // AstCFunc that is not called from the code (e.g.: DPI exports) does not need to be visited
    // this way. Also, not visiting such AstCFuncs allows to avoid caching results for them which
    // this function does - which could lead to excessive memory usage
    ಶೂನ್ಯ ಭೇಟಿ_Called_CFunc(AstCFunc* ಸ್ಥಿರಾಂಕ nodep) {
        CFuncCache& ಮೌಲ್ಯ = m_cfuncsCache(nodep);
        ಆಯ್ಕೆ (ಮೌಲ್ಯ.m_state) {
        ಸ್ಥಿತಿ CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_timingDomains);

            // Visit
            ಮೌಲ್ಯ.m_state = CFuncCache::VISITING;
            m_call_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
            {
                VL_RESTORER(m_cfuncp);
                m_cfuncp = nodep;
                iterateChildren(nodep);
            }
            m_call_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
            ಮೌಲ್ಯ.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::ಅದಲುಬದಲು(m_timingDomains, ಮೌಲ್ಯ.m_timingDomains);
        } ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ CFuncCache::VISITING: {
            ಗಾಗಿ (ಗಾತ್ರ_t i = m_call_ರಾಶಿ.ಗಾತ್ರ_2() - 1; m_call_ರಾಶಿ.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ರಾಶಿ[i]).m_includes.ಸೇರಿಸು(nodep);
            }
            ಹಿಂತಿರುಗಿ;  // Break recursion
        }
        ಸ್ಥಿತಿ CFuncCache::INITIALIZED: ನಿಲ್ಲಿಸು;
        }
        std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstCFunc*> ಭೇಟಿಯಾದ;
        ಕೂಡಿಸು_CFunc_Cached_ಮೌಲ್ಯಗಳು(nodep, ಭೇಟಿಯಾದ);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override {
        iterateChildren(nodep);
        // Decide which scheme to use for each variable and do the 'prepare' step
        ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp : m_vscps) {
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            vscpInfo.m_scheme = ಆರಿಸು_Scheme(vscp, vscpInfo);
            // Run 'prepare' step
            ಆಯ್ಕೆ (vscpInfo.m_scheme) {
            ಸ್ಥಿತಿ Scheme::Undecided:  // LCOV_EXCL_START
                UASSERT_OBJ(ಸುಳ್ಳು, vscp, "Failed to choose NBA scheme");
                ನಿಲ್ಲಿಸು;  // LCOV_EXCL_STOP
            ಸ್ಥಿತಿ Scheme::Unsupported_Compound_Array_ಒಳ_Loop: {
                // Will report error at the site of the NBA
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ Scheme::ShadowVar: {
                ++m_nSchemeShadowVar;
                prepareSchemeShadowVar(vscp, vscpInfo);
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ Scheme::ShadowVarMasked: {
                ++m_nSchemeShadowVarMasked;
                prepareSchemeShadowVarMasked(vscp, vscpInfo);
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ Scheme::FlagShared: {
                ++m_nSchemeFlagShared;
                prepareSchemeFlagShared(vscp, vscpInfo);
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ Scheme::FlagUnique: {
                ++m_nSchemeFlagUnique;
                prepareSchemeFlagUnique(vscp, vscpInfo);
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ Scheme::ಮೌಲ್ಯ_ಸರತಿ_ಸಂಪೂರ್ಣ: {
                ++m_n_Scheme_ಮೌಲ್ಯ_Queues_ಸಂಪೂರ್ಣ;
                prepare_Scheme_ಮೌಲ್ಯ_ಸರತಿ</* Partial: */ ಸುಳ್ಳು>(vscp, vscpInfo);
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ Scheme::ಮೌಲ್ಯ_ಸರತಿ_Partial: {
                ++m_n_Scheme_ಮೌಲ್ಯ_Queues_Partial;
                prepare_Scheme_ಮೌಲ್ಯ_ಸರತಿ</* Partial: */ ನಿಜ>(vscp, vscpInfo);
                ನಿಲ್ಲಿಸು;
            }
            }
        }
        // Convert all NBAs
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ NBA& nba : m_nbas) {
            Ast_ನಿಯೋಜಿಸು_Dly* ಸ್ಥಿರಾಂಕ nbap = nba.nodep;
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = nba.vscp;
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            // Run 'convert' step
            ಆಯ್ಕೆ (vscpInfo.m_scheme) {
            ಸ್ಥಿತಿ Scheme::Undecided: {  // LCOV_EXCL_START
                UASSERT_OBJ(ಸುಳ್ಳು, vscp, "Unreachable");
                ನಿಲ್ಲಿಸು;
            }  // LCOV_EXCL_STOP
            ಸ್ಥಿತಿ Scheme::Unsupported_Compound_Array_ಒಳ_Loop: {
                // TODO: make this an E_UNSUPPORTED...
                nbap->v3warn(BLKLOOPINIT, "Unsupported: Non-blocking assignment to array with "
                                          "compound element type inside loop");
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ Scheme::ShadowVar: {
                convertSchemeShadowVar(nbap, vscp, vscpInfo);
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ Scheme::ShadowVarMasked: {
                convertSchemeShadowVarMasked(nbap, vscp, vscpInfo);
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ Scheme::FlagShared: {
                convertSchemeFlagShared(nbap, vscp, vscpInfo);
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ Scheme::FlagUnique: {
                convertSchemeFlagUnique(nbap, vscp, vscpInfo);
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ Scheme::ಮೌಲ್ಯ_ಸರತಿ_ಸಂಪೂರ್ಣ: {
                convert_Scheme_ಮೌಲ್ಯ_ಸರತಿ(nbap, vscp, vscpInfo, /* partial: */ ಸುಳ್ಳು);
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ Scheme::ಮೌಲ್ಯ_ಸರತಿ_Partial:
                convert_Scheme_ಮೌಲ್ಯ_ಸರತಿ(nbap, vscp, vscpInfo, /* partial: */ ನಿಜ);
                ನಿಲ್ಲಿಸು;
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಕ್ರಿಯ* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ignoreBlkAndNBlk);
        VL_RESTORER(m_ಒಳ_Non_Comb_Logic);
        m_activep = nodep;
        ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = nodep->sentreep();
        m_ignoreBlkAndNBlk = senTreep->hasStatic() || senTreep->hasInitial();
        m_ಒಳ_Non_Comb_Logic = senTreep->hasClocked();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override {
        VL_RESTORER(m_needsInitialTrigger);
        VL_RESTORER_CLEAR(m_nbaEventSenTreeps);
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಮೊದಲ_NBAAdded_ಸೂಚ್ಯಂಕ = m_nbas.ಗಾತ್ರ_2();
        {
            VL_RESTORER(m_ಒಳ_Suspendable_Or_Fork);
            VL_RESTORER(m_procp);
            VL_RESTORER(m_ignoreBlkAndNBlk);
            VL_RESTORER(m_ಒಳ_Non_Comb_Logic);
            // When we are dealing with initial block we need to
            // treat it as suspendable when we meet a NBA
            m_ಒಳ_Suspendable_Or_Fork = nodep->isSuspendable() || VN_IS(nodep, Initial);
            m_procp = nodep;
            ಆದರೆ (nodep->isSuspendable()) {
                m_ignoreBlkAndNBlk = ಸುಳ್ಳು;
                m_ಒಳ_Non_Comb_Logic = ನಿಜ;
            }
            iterateChildren(nodep);
        }
        ಸ್ವಯಂಚಾಲಿತ ಒಳಗೊಂಡಿದೆ_Clocled = [](ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* itemp) {
            ಆಗ (itemp) {
                ಆದರೆ (itemp->ಅಂಚು_Type().clockedStmt()) ಹಿಂತಿರುಗಿ ನಿಜ;
                itemp = VN_AS(itemp->nextp(), Sen_ಅಂಶ);
            }
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        };
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಕೂಡಿಸು_Initial_Trigger = m_needsInitialTrigger
                                       && !(isProcedureWithSentreep<AstAlways>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysObserved>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysReactive>(nodep))
                                       && !ಒಳಗೊಂಡಿದೆ_Clocled(m_activep->sentreep()->sensesp());
        ಆದರೆ (m_timingDomains.ಖಾಲಿಗಳು() && !ಕೂಡಿಸು_Initial_Trigger) ಹಿಂತಿರುಗಿ;

        // There were some timing domains involved in the process. Add all of them as sensitivities
        // of all NBA targets in this process. Note this is a bit of a sledgehammer, we should only
        // need those that directly precede the NBA in control flow, but that is hard to compute,
        // so we will hammer away.

        // First gather all senItems
        Ast_Sen_ಅಂಶ* senItemp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (ಕೂಡಿಸು_Initial_Trigger) {
            senItemp = ಹೊಸ Ast_Sen_ಅಂಶ{nodep->fileline(), Ast_Sen_ಅಂಶ::InitialNBA{}};
            ++m_nInitialNBA;
        }

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ domainp : m_timingDomains) {
            ಆದರೆ (domainp->sensesp())
                senItemp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(senItemp, domainp->sensesp()->clone_ವೃಕ್ಷ(ನಿಜ));
        }
        m_timingDomains.clear();
        // Add them to all nba targets we gathered in this process
        ಗಾಗಿ (ಗಾತ್ರ_t i = ಮೊದಲ_NBAAdded_ಸೂಚ್ಯಂಕ; i < m_nbas.ಗಾತ್ರ_2(); ++i) {
            m_vscpInfo(m_nbas[i].vscp).ಕೂಡಿಸು_ಸಂವೇದನಾಶೀಲತೆ(senItemp);
        }
        ಗಾಗಿ (Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep : m_nbaEventSenTreeps)
            senTreep->ಕೂಡಿಸು_Sensesp(senItemp->clone_ವೃಕ್ಷ(ನಿಜ));
        // Done with these
        VL_DO_DANGLING(senItemp->delete_ವೃಕ್ಷ(), senItemp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override {
        VL_RESTORER(m_ಒಳ_Suspendable_Or_Fork);
        m_ಒಳ_Suspendable_Or_Fork = ನಿಜ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCAwait* nodep) override {
        ಆದರೆ (nodep->sentreep()) m_timingDomains.ಸೇರಿಸು(nodep->sentreep());
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFireEvent* nodep) override {
        UASSERT_OBJ(v3Global.hasEvents(), nodep, "Inconsistent");
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ eventp = nodep->operandp()->unlink_Fr_ಹಿಂಭಾಗ();

        // Enqueue for clearing 'triggered' state on next eval
        AstCStmt* ಸ್ಥಿರಾಂಕ cstmtp = ಹೊಸ AstCStmt{flp};
        cstmtp->ಕೂಡಿಸು("vlSymsp->fireEvent(");
        cstmtp->ಕೂಡಿಸು(eventp);
        cstmtp->ಕೂಡಿಸು(");");

        Ast_ಗಂಟು* newp = cstmtp;
        ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ vrefp = VN_CAST(eventp, VarRef);
        ಆದರೆ (nodep->isDelayed() && (m_cfuncp || !vrefp)) {
            // V3Timing converts these with --timing
            nodep->v3warn(E_NOTIMING, "Nonblocking event trigger "
                                          << (m_cfuncp ? "in a non-inlined function/task"
                                                       : "of a class or interface member or "
                                                         "array element")
                                          << " requires --timing");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isDelayed()) {
            ಸ್ಥಿರಾಂಕ std::string newvarname = "_" + vrefp->varp()->shortName();
            AstVarScope* ಸ್ಥಿರಾಂಕ dlyvscp
                = m_dlyTmps.make(flp, vrefp->varScopep()->scopep(), 1, newvarname);

            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ dlyRef = [=](VAccess access) {  //
                ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, dlyvscp, access};
            };

            AstAlwaysPost* ಸ್ಥಿರಾಂಕ postp = ಹೊಸ AstAlwaysPost{flp};
            AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, dlyRef(VAccess::READ)};
            postp->ಕೂಡಿಸು_Stmtsp(ifp);

            UASSERT_OBJ(m_activep, nodep, "No active to handle FireEvent");
            Ast_Sen_ವೃಕ್ಷ* senTreep = m_activep->sentreep();
            Ast_ಸಕ್ರಿಯ* activep = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (m_ಒಳ_Suspendable_Or_Fork && (senTreep->hasInitial() || senTreep->hasClocked())) {
                // Suspendable code can trigger whenever it resumes, so fire when its NBAs are
                // committed (see visit(AstNodeProcedure*)), clearing the flag like
                // Scheme::FlagUnique
                senTreep = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{
                    flp, senTreep->hasClocked() ? senTreep->sensesp()->clone_ವೃಕ್ಷ(ನಿಜ) : ಶೂನ್ಯಸೂಚಕ};
                m_nbaEventSenTreeps.ತಳ್ಳು_ಹಿಂಭಾಗ(senTreep);
                m_needsInitialTrigger |= m_timingDomains.ಖಾಲಿಗಳು();
                dlyvscp->varp()->ಗಣ_Ignore_Post_Write();
                ifp->ಕೂಡಿಸು_Thensp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, dlyRef(VAccess::WRITE),
                                             ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}}});
                activep = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "nba-event", senTreep};
                activep->sen_ವೃಕ್ಷ_Storep(senTreep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                activep = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "nba-event", senTreep};
                AstAlwaysPre* ಸ್ಥಿರಾಂಕ prep = ಹೊಸ AstAlwaysPre{flp};
                prep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, dlyRef(VAccess::WRITE),
                                              ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}}});
                activep->ಕೂಡಿಸು_Stmtsp(prep);
            }
            ifp->ಕೂಡಿಸು_Thensp(newp);
            m_activep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(activep);
            activep->ಕೂಡಿಸು_Stmtsp(postp);

            newp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, dlyRef(VAccess::WRITE),
                                 ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Dly* nodep) override {
        // Prevent double processing due to AstExprStmt being moved before this node
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (m_cfuncp) {
            ಆದರೆ (!v3Global.rootp()->nbaEventp()) {
                nodep->v3warn(
                    E_NOTIMING,
                    "Delayed assignment in a non-inlined function/task requires --timing");
            }
            ಹಿಂತಿರುಗಿ;
        }
        UASSERT_OBJ(m_procp, nodep, "Delayed assignment not under process");
        UASSERT_OBJ(m_activep, nodep, "<= not under sensitivity block");
        UASSERT_OBJ(m_scopep, nodep, "<= not under scope");
        UASSERT_OBJ(m_ಒಳ_Suspendable_Or_Fork || m_activep->hasClocked(), nodep,
                    "<= assignment in non-clocked block, should have been converted in V3Active");

        m_needsInitialTrigger |= m_timingDomains.ಖಾಲಿಗಳು();

        // Record scope of this NBA
        nodep->user2p(m_scopep);

        // Grab the reference to the target of the NBA, also lift ExprStmt statements on the LHS
        VL_RESTORER(m_currNbaLhsRefp);
        UASSERT_OBJ(!m_currNbaLhsRefp, nodep, "NBAs should not nest");
        nodep->lhsp()->foreach([&](Ast_ಗಂಟು* currp) {
            // cppcheck-suppress constVariablePointer
            ಆದರೆ (Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಸ್ಥಿರಾಂಕ exprp = VN_CAST(currp, ಅಭಿವ್ಯಕ್ತಿ_Stmt)) {
                // Move statements before the NBA
                nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(exprp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                // Replace with result
                currp->replaceWith(exprp->resultp()->unlink_Fr_ಹಿಂಭಾಗ());
                // Get rid of the AstExprStmt
                VL_DO_DANGLING2(ತಳ್ಳು_Deletep(currp), currp, exprp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(currp, VarRef)) {
                // Ignore reads (e.g.: '_[*here*] <= _')
                ಆದರೆ (refp->access().isReadOnly()) ಹಿಂತಿರುಗಿ;
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
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = m_currNbaLhsRefp->varScopep();
        // Record it on first encounter
        VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
        ಆದರೆ (!vscpInfo.m_ಮೊದಲ_Nba_Refp) {
            vscpInfo.m_ಮೊದಲ_Nba_Refp = m_currNbaLhsRefp;
            vscpInfo.m_fistActivep = m_activep;
            m_vscps.emplace_ಹಿಂಭಾಗ(vscp);
        }
        // Note usage context
        vscpInfo.m_ಸಂಪೂರ್ಣ |= VN_IS(nodep->lhsp(), VarRef);
        vscpInfo.m_partial |= VN_IS(nodep->lhsp(), Sel);
        vscpInfo.m_ಒಳ_Loop |= m_ಒಳ_Loop;
        vscpInfo.m_ಒಳ_Susp_Or_Fork |= m_ಒಳ_Suspendable_Or_Fork;
        // Sensitivity might be non-clocked, in a suspendable process, which are handled elsewhere
        ಆದರೆ (m_activep->sentreep()->hasClocked()) {
            ಆದರೆ (vscpInfo.m_fistActivep != m_activep) {
                AstVar* ಸ್ಥಿರಾಂಕ varp = vscp->varp();
                ಆದರೆ (!varp->user1_ಗಣ_Once()
                    && !varp->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::MULTIDRIVEN)) {
                    varp->v3warn(MULTIDRIVEN,
                                 "Signal has multiple driving blocks with different clocking: "
                                     << varp->prettyNameQ() << '\n'
                                     << vscpInfo.m_ಮೊದಲ_Nba_Refp->warnOther()
                                     << "... Location of first driving block\n"
                                     << vscpInfo.m_ಮೊದಲ_Nba_Refp->warnContextSecondary()
                                     << m_currNbaLhsRefp->warnOther()
                                     << "... Location of other driving block\n"
                                     << m_currNbaLhsRefp->warnContextPrimary() << '\n');
                }
            }
            // Add this sensitivity to the variable
            vscpInfo.ಕೂಡಿಸು_ಸಂವೇದನಾಶೀಲತೆ(m_activep->sentreep());
        }

        // Record the NBA for later processing
        m_nbas.emplace_ಹಿಂಭಾಗ();
        NBA& nba = m_nbas.ಹಿಂಭಾಗ();
        nba.nodep = nodep;
        nba.vscp = vscp;

        // Record write reference
        ದಾಖಲೆ_Write_Ref(m_currNbaLhsRefp, ನಿಜ);

        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        // Already checked the NBA LHS ref, ignore here
        ಆದರೆ (nodep == m_currNbaLhsRefp) ಹಿಂತಿರುಗಿ;
        // Only care about write refs
        ಆದರೆ (!nodep->access().isWriteOrRW()) ಹಿಂತಿರುಗಿ;
        // Record write reference
        ದಾಖಲೆ_Write_Ref(nodep, ಸುಳ್ಳು);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override {
        VL_RESTORER(m_ಒಳ_Loop);
        m_ಒಳ_Loop = ನಿಜ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* ಸ್ಥಿರಾಂಕ nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        ಆದರೆ (cfuncps.ಖಾಲಿಗಳು()) {
            ಭೇಟಿ_Called_CFunc(nodep->funcp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಾಗಿ (AstCFunc* ಸ್ಥಿರಾಂಕ cfuncp : cfuncps) ಭೇಟಿ_Called_CFunc(cfuncp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* ಸ್ಥಿರಾಂಕ nodep) override {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಮೌಲ್ಯ = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        ಆದರೆ (ಮೌಲ್ಯ.m_state != CFuncCache::UNINITIALIZED) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_cfuncp);
        // Its timing domains are those of the processes calling it, see visitCalledCFunc()
        VL_RESTORER_CLEAR(m_timingDomains);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }

    // Pre/Post logic are created here and their content need no further changes, so ignore.
    ಶೂನ್ಯ ಭೇಟಿ(AstAlwaysPre*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstAlwaysPost*) override {}

    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ DelayedVisitor(AstNetlist* nodep)
        : m_classGraphp{V3_Class_ನಕ್ಷೆ::build(nodep)} {
        iterate(nodep);
    }
    ~DelayedVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("NBA, variables using ShadowVar scheme", m_nSchemeShadowVar);
        V3Stats::ಕೂಡಿಸು_Stat("NBA, variables using ShadowVarMasked scheme", m_nSchemeShadowVarMasked);
        V3Stats::ಕೂಡಿಸು_Stat("NBA, variables using FlagShared scheme", m_nSchemeFlagShared);
        V3Stats::ಕೂಡಿಸು_Stat("NBA, variables using FlagUnique scheme", m_nSchemeFlagUnique);
        V3Stats::ಕೂಡಿಸು_Stat("NBA, variables using ValueQueueWhole scheme", m_n_Scheme_ಮೌಲ್ಯ_Queues_ಸಂಪೂರ್ಣ);
        V3Stats::ಕೂಡಿಸು_Stat("NBA, variables using ValueQueuePartial scheme",
                         m_n_Scheme_ಮೌಲ್ಯ_Queues_Partial);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, NBA flags shared", m_n_Shared_ಗಣ_Flags);
        V3Stats::ಕೂಡಿಸು_Stat("Procedures needing initial NBA trigger", m_nInitialNBA);
        V3Stats::ಕೂಡಿಸು_Stat("Non-inlined co_awaits with SenTree", m_non_Inlined_CAwaits_With_Sen_ವೃಕ್ಷ);
    }
};

//######################################################################
// Delayed class functions

ಶೂನ್ಯ V3Delayed::delayedAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DelayedVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("delayed", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
