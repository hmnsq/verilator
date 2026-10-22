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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Delayed.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ClassGraph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkLValue.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SharedTmps.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <deque>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert AstAssignDlys (NBAs)

ਵਰਗ DelayedVisitor final : ਜਨਤਕ VNVisitor {
    // TYPES

    // The various NBA conversion schemes, including error cases
    ਗਿਣਤੀ ਵਰਗ Scheme : uint8_t {
        Undecided = 0,
        Unsupported_Compound_Array_ਵਿੱਚ_Loop,
        ShadowVar,
        ShadowVarMasked,
        FlagShared,
        FlagUnique,
        ਮੁੱਲ_ਕਤਾਰ_ਸਾਰਾ,
        ਮੁੱਲ_ਕਤਾਰ_Partial
    };

    // All info associated with a variable that is the target of an NBA
    ਵਰਗ VarScopeInfo final {
    ਜਨਤਕ:
        // First write reference encountered to the VarScope as the target on an NBA
        ਅਚਲ AstVarRef* m_ਪਹਿਲਾ_Nba_Refp = ਖਾਲੀ_ਸੂਚਕ;
        // Active block 'm_firstNbaRefp' is under
        ਅਚਲ Ast_ਸਰਗਰਮ* m_fistActivep = ਖਾਲੀ_ਸੂਚਕ;
        ਤਾਰਕਿਕ m_ਸਾਰਾ = ਝੂਠ;  // Used on LHS of NBA directly via VarRef
        ਤਾਰਕਿਕ m_partial = ਝੂਠ;  // Used on LHS of NBA under a Sel
        ਤਾਰਕਿਕ m_ਵਿੱਚ_Loop = ਝੂਠ;  // Used on LHS of NBA in a loop
        ਤਾਰਕਿਕ m_ਵਿੱਚ_Susp_Or_Fork = ਝੂਠ;  // Used on LHS of NBA in suspendable process or fork
        Scheme m_scheme = Scheme::Undecided;  // Conversion scheme to use for this variable

    ਨਿਜੀ:
        // Combined sensitivities of all NBAs targeting this variable
        Ast_Sen_ਰੁੱਖ* m_senTreep = ਖਾਲੀ_ਸੂਚਕ;

        // Union of stuff needed for the various schemes - use accessors below!
        ਸੰਘ {
            ਬਣਤਰ {  // Stuff needed for Scheme::ShadowVar
                AstVarScope* vscp;  // The shadow variable
            } m_shadowVariableKit;
            ਬਣਤਰ {  // Stuff needed for Scheme::ShadowVarMasked
                AstVarScope* vscp;  // The shadow variable
                AstVarScope* maskp;  // The mask variable
            } m_shadowVarMaskedKit;
            ਬਣਤਰ {  // Stuff needed for Scheme::FlagShared
                Ast_ਸਰਗਰਮ* activep;  // The active block for the Pre/Post logic
                AstAlwaysPost* postp;  // The post block for commiting results
                AstVarScope* commitFlagp;  // The commit flag variable, for reuse
                AstIf* commitIfp;  // The previous if statement for committing, for reuse
            } m_flagSharedKit;
            ਬਣਤਰ {  // Stuff needed for Scheme::FlagUnique
                AstAlwaysPost* postp;  // The post block for commiting results
            } m_flagUniqueKit;
            ਬਣਤਰ {  // Stuff needed for Scheme::ValueQueueWhole/Scheme::ValueQueuePartial
                AstVarScope* vscp;  // The commit queue variable
            } m_ਮੁੱਲ_ਕਤਾਰ_Kit;
        } m_kitUnion;

    ਜਨਤਕ:
        VarScopeInfo() = ਮੂਲ;
        ~VarScopeInfo() {
            // Might not be linked if there was an error
            ਜੇ (!m_senTreep->backp()) VL_DO_DANGLING(m_senTreep->delete_ਰੁੱਖ(), m_senTreep);
        }
        // Accessors for the above union fields
        ਸਵੈਚਾਲਿਤ& shadowVariableKit() {
            UASSERT(m_scheme == Scheme::ShadowVar, "Inconsistent Scheme");
            ਵਾਪਸ m_kitUnion.m_shadowVariableKit;
        }
        ਸਵੈਚਾਲਿਤ& shadowVarMaskedKit() {
            UASSERT(m_scheme == Scheme::ShadowVarMasked, "Inconsistent Scheme");
            ਵਾਪਸ m_kitUnion.m_shadowVarMaskedKit;
        }
        ਸਵੈਚਾਲਿਤ& flagSharedKit() {
            UASSERT(m_scheme == Scheme::FlagShared, "Inconsistent Scheme");
            ਵਾਪਸ m_kitUnion.m_flagSharedKit;
        }
        ਸਵੈਚਾਲਿਤ& flagUniqueKit() {
            UASSERT(m_scheme == Scheme::FlagUnique, "Inconsistent Scheme");
            ਵਾਪਸ m_kitUnion.m_flagUniqueKit;
        }
        ਸਵੈਚਾਲਿਤ& ਮੁੱਲ_ਕਤਾਰ_Kit() {
            UASSERT(m_scheme == Scheme::ਮੁੱਲ_ਕਤਾਰ_Partial || m_scheme == Scheme::ਮੁੱਲ_ਕਤਾਰ_ਸਾਰਾ,
                    "Inconsistent Scheme");
            ਵਾਪਸ m_kitUnion.m_ਮੁੱਲ_ਕਤਾਰ_Kit;
        }

        // Accessor
        Ast_Sen_ਰੁੱਖ* senTreep() ਅਚਲ { ਵਾਪਸ m_senTreep; }

        // Add sensitivities
        ਖਾਲੀ ਜੋੜੋ_ਸੰਵੇਦਨਸ਼ੀਲਤਾ(Ast_Sen_ਵਸਤੂ* nodep) {
            ਜੇ (!m_senTreep) m_senTreep = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{nodep->fileline(), ਖਾਲੀ_ਸੂਚਕ};
            // Add a copy of each term
            m_senTreep->ਜੋੜੋ_Sensesp(nodep->clone_ਰੁੱਖ(ਸੱਚ));
            // Remove duplicates
            V3Const::constifyExpensiveEdit(m_senTreep);
        }
        // cppcheck-suppress constParameterPointer
        ਖਾਲੀ ਜੋੜੋ_ਸੰਵੇਦਨਸ਼ੀਲਤਾ(Ast_Sen_ਰੁੱਖ* nodep) { ਜੋੜੋ_ਸੰਵੇਦਨਸ਼ੀਲਤਾ(nodep->sensesp()); }
    };

    // Data structure to keep track of all writes to
    ਬਣਤਰ WriteReference final {
        AstVarRef* m_refp = ਖਾਲੀ_ਸੂਚਕ;  // The reference
        ਤਾਰਕਿਕ m_isNBA = ਝੂਠ;  // True if an NBA write
        ਤਾਰਕਿਕ m_ਵਿੱਚ_Non_Comb = ਝੂਠ;  // True if reference is known to be in non-combinational logic
        WriteReference() = ਮੂਲ;
        WriteReference(AstVarRef* refp, ਤਾਰਕਿਕ isNBA, ਤਾਰਕਿਕ ਵਿੱਚ_Non_Comb)
            : m_refp{refp}
            , m_isNBA{isNBA}
            , m_ਵਿੱਚ_Non_Comb{ਵਿੱਚ_Non_Comb} {}
    };

    // Data required to lower AstAssignDelay later
    ਬਣਤਰ NBA final {
        Ast_ਸੌਂਪੋ_Dly* nodep = ਖਾਲੀ_ਸੂਚਕ;  // The NBA this record refers to
        AstVarScope* vscp = ਖਾਲੀ_ਸੂਚਕ;  // The target variable the NBA is updating
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
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_user2_ਵਿੱਚ_Use;
    ਅਚਲ VNUser3_ਵਿੱਚ_Use m_user3_ਵਿੱਚ_Use;

    ਬਣਤਰ CFuncCache final {
        VInsertion_ਸੈੱਟ<Ast_Sen_ਰੁੱਖ*> m_timingDomains;  // What shall be added to m_timingDomains
        std::ਸੈੱਟ<AstCFunc*>
            m_includes;  // CFuncs whose CFuncCache shall be included into this - this is used to
                         // break cycles: A->B->A (instead of visiting A while it is still begin
                         // visited B just marks that it includes A)
        ਗਿਣਤੀ State : uint8_t {
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
    AstUser3Allocator<AstVarScope, std::ਸਦਿਸ਼<WriteReference>> m_writeRefs;

    // STATE - across all visitors
    VInsertion_ਸੈੱਟ<Ast_Sen_ਰੁੱਖ*> m_timingDomains;  // Timing resume domains
    V3SharedTmps m_dlyTmps{"__Vdly", VVarType::BLOCKTEMP};  // Temporary variables
    // Commit queue data types, by element type and partial flag. Shared by all instances,
    // as m_dlyTmps only shares variables with the same data type.
    std::map<std::pair<ਅਚਲ Ast_ਗੰਢ_DType*, ਤਾਰਕਿਕ>, Ast_NBACommit_ਕਤਾਰ_DType*> m_cqDTypeps;

    ਅਚਲ std::unique_ptr<V3_Class_ਗ੍ਰਾਫ>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::ਸਦਿਸ਼<ਅਚਲ AstCFunc*> m_call_ਢੇਰ;  // Current callstack of AstCFuncs

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ਸਰਗਰਮ* m_activep = ਖਾਲੀ_ਸੂਚਕ;  // Current activate
    ਅਚਲ AstCFunc* m_cfuncp = ਖਾਲੀ_ਸੂਚਕ;  // Current public C Function
    Ast_ਗੰਢ_Procedure* m_procp = ਖਾਲੀ_ਸੂਚਕ;  // Current process
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Current scope
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Loop = ਝੂਠ;  // True in for loops
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Suspendable_Or_Fork = ਝੂਠ;  // True in suspendable processes and forks
    ਤਾਰਕਿਕ m_ignoreBlkAndNBlk = ਝੂਠ;  // Suppress delayed assignment BLKANDNBLK
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Non_Comb_Logic = ਝੂਠ;  // We are in non-combinational logic
    ਤਾਰਕਿਕ m_needsInitialTrigger = ਝੂਠ;  // Whether a NodeProcedure needs a initial trigger
    std::ਸਦਿਸ਼<Ast_Sen_ਰੁੱਖ*> m_nbaEventSenTreeps;  // Sensitivities of '->>' in the process
    AstVarRef* m_currNbaLhsRefp = ਖਾਲੀ_ਸੂਚਕ;  // Current NBA LHS variable reference

    // STATE - during NBA conversion (after visit)
    std::ਸਦਿਸ਼<NBA> m_nbas;  // AstAssignDly instances to lower at the end
    std::ਸਦਿਸ਼<AstVarScope*> m_vscps;  // Target variables on LHSs of NBAs
    Ast_ਸੌਂਪੋ_Dly* m_ਅਗਲਾ_Dlyp = ਖਾਲੀ_ਸੂਚਕ;  // The nextp of the previous AstAssignDly
    AstVarScope* m_prevVscp = ਖਾਲੀ_ਸੂਚਕ;  // The target of the previous AstAssignDly

    // STATE - Statistic tracking
    VDouble0 m_nSchemeShadowVar;  // Number of variables using Scheme::ShadowVar
    VDouble0 m_nSchemeShadowVarMasked;  // Number of variabels using Scheme::ShadowVarMasked
    VDouble0 m_nSchemeFlagShared;  // Number of variables using Scheme::FlagShared
    VDouble0 m_nSchemeFlagUnique;  // Number of variables using Scheme::FlagUnique
    VDouble0 m_n_Scheme_ਮੁੱਲ_Queues_ਸਾਰਾ;  //  Number of variables using Scheme::ValueQueueWhole
    VDouble0 m_n_Scheme_ਮੁੱਲ_Queues_Partial;  //  Number of variables using Scheme::ValueQueuePartial
    VDouble0 m_n_Shared_ਸੈੱਟ_Flags;  // "Set" flags actually shared by Scheme::FlagShared variables
    VDouble0 m_nInitialNBA;  // Number of procedural blocks with initial NBA
    VDouble0 m_non_Inlined_CAwaits_With_Sen_ਰੁੱਖ;  // Count uses of not inlined co_awaits

    // METHODS

    // Return true iff a variable is assigned by both blocking and nonblocking
    // assignments. Issue BLKANDNBLK error if we can't prove the mixed
    // assignments are to independent bits and the blocking assignment can be
    // in combinational logic, which is something we can't safely implement
    // still.
    ਤਾਰਕਿਕ checkMixedUsage(ਅਚਲ AstVarScope* vscp, ਤਾਰਕਿਕ isIntegralOrPacked) {

        ਬਣਤਰ Ref final {
            AstVarRef* m_refp;  // The reference
            ਤਾਰਕਿਕ m_ਵਿੱਚ_Non_Comb;  // True if known to be in non-combinational logic
            ਪੂਰਨ m_lsb;  // LSB of accessed range
            ਪੂਰਨ m_msb;  // MSB of accessed range
            Ref(AstVarRef* refp, ਤਾਰਕਿਕ ਵਿੱਚ_Non_Comb, ਪੂਰਨ lsb, ਪੂਰਨ msb)
                : m_refp{refp}
                , m_ਵਿੱਚ_Non_Comb{ਵਿੱਚ_Non_Comb}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };

        std::ਸਦਿਸ਼<Ref> blkRefs;  // Blocking writes
        std::ਸਦਿਸ਼<Ref> nbaRefs;  // Non-blockign writes

        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = isIntegralOrPacked ? vscp->ਚੌੜਾਈ() : 1;

        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& writeRef : m_writeRefs(vscp)) {
            ਪੂਰਨ lsb = 0;
            ਪੂਰਨ msb = ਚੌੜਾਈ - 1;
            ਜੇ (ਅਚਲ AstSel* ਅਚਲ selp = VN_CAST(writeRef.m_refp->backp(), Sel)) {
                ਜੇ (VN_IS(selp->lsbp(), Const)) {
                    lsb = selp->lsbConst();
                    msb = selp->msbConst();
                }
            }
            ਜੇ (writeRef.m_isNBA) {
                nbaRefs.emplace_ਪਿੱਠ(writeRef.m_refp, writeRef.m_ਵਿੱਚ_Non_Comb, lsb, msb);
            } ਨਹੀਂਤਾਂ {
                blkRefs.emplace_ਪਿੱਠ(writeRef.m_refp, writeRef.m_ਵਿੱਚ_Non_Comb, lsb, msb);
            }
        }
        // We only run this function on targets of NBAs, so there should be at least one...
        UASSERT_OBJ(!nbaRefs.ਖਾਲੀ_ਥਾਵਾਂ(), vscp, "Did not record NBA write");
        // If no blocking upadte, then we are good
        ਜੇ (blkRefs.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਝੂਠ;

        // If the blocking assignment is in non-combinational logic (i.e.:
        // in logic that has an explicit trigger), then we can safely
        // implement it (there is no race between clocked logic and post
        // scheduled logic), so need not error
        blkRefs.erase(std::remove_if(blkRefs.begin(), blkRefs.ਸਿਰਾ(),
                                     [](ਅਚਲ Ref& ref) { ਵਾਪਸ ref.m_ਵਿੱਚ_Non_Comb; }),
                      blkRefs.ਸਿਰਾ());

        // If nothing left, then we need not error
        ਜੇ (blkRefs.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਸੱਚ;

        // If not a packed variable, warn here as we can't prove independence
        ਜੇ (!isIntegralOrPacked) {
            ਅਚਲ Ref& blkRef = blkRefs.front();
            ਅਚਲ Ref& nbaRef = nbaRefs.front();
            vscp->v3warn(
                BLKANDNBLK,
                "Unsupported: Blocking and non-blocking assignments to same non-packed variable: "
                    << vscp->varp()->prettyNameQ() << '\n'
                    << vscp->warnContextPrimary() << '\n'
                    << blkRef.m_refp->warnOther() << "... Location of blocking assignment\n"
                    << blkRef.m_refp->warnContextSecondary() << '\n'
                    << nbaRef.m_refp->warnOther() << "... Location of nonblocking assignment\n"
                    << nbaRef.m_refp->warnContextSecondary());
            ਵਾਪਸ ਸੱਚ;
        }

        // We need to error if we can't prove the written bits are independent

        // Sort refs by interval
        ਅਚਲ ਸਵੈਚਾਲਿਤ lessThanRef = [](ਅਚਲ Ref& a, ਅਚਲ Ref& b) {
            ਜੇ (a.m_lsb != b.m_lsb) ਵਾਪਸ a.m_lsb < b.m_lsb;
            ਵਾਪਸ a.m_msb < b.m_msb;
        };
        std::stable_ਛਾਂਟੋ(blkRefs.begin(), blkRefs.ਸਿਰਾ(), lessThanRef);
        std::stable_ਛਾਂਟੋ(nbaRefs.begin(), nbaRefs.ਸਿਰਾ(), lessThanRef);
        // Iterate both vectors, checking for overlap
        ਸਵੈਚਾਲਿਤ bIt = blkRefs.begin();
        ਸਵੈਚਾਲਿਤ nIt = nbaRefs.begin();
        ਜਦੋਂ (bIt != blkRefs.ਸਿਰਾ() && nIt != nbaRefs.ਸਿਰਾ()) {
            ਜੇ (lessThanRef(*bIt, *nIt)) {
                ਜੇ (nIt->m_lsb <= bIt->m_msb) ਰੋਕੋ;  // Stop on Overlap
                ++bIt;
            } ਨਹੀਂਤਾਂ {
                ਜੇ (bIt->m_lsb <= nIt->m_msb) ਰੋਕੋ;  // Stop on Overlap
                ++nIt;
            }
        }

        // If we found an overlapping range that cannot be safely implemented, then wran...
        ਜੇ (bIt != blkRefs.ਸਿਰਾ() && nIt != nbaRefs.ਸਿਰਾ()) {
            ਅਚਲ Ref& blkRef = *bIt;
            ਅਚਲ Ref& nbaRef = *nIt;
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

        ਵਾਪਸ ਸੱਚ;
    }

    // Choose the NBA scheme used for the given variable.
    Scheme ਚੁਣੋ_Scheme(ਅਚਲ AstVarScope* vscp, ਅਚਲ VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::Undecided, vscp, "NBA scheme already decided");

        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = vscp->dtypep()->skipRefp();
        // Unpacked arrays
        ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ uaDTypep = VN_CAST(dtypep, UnpackArrayDType)) {
            // If whole array is target of NBA, use ShadowVar, but not in a suspendable process or
            // fork, which resume outside the 'nba' region, so its 'pre' logic would overwrite the
            // value they write in the shadow variable
            ਜੇ (vscpInfo.m_ਸਾਰਾ) {
                ਵਾਪਸ vscpInfo.m_ਵਿੱਚ_Susp_Or_Fork ? Scheme::FlagUnique : Scheme::ShadowVar;
            }
            // Basic underlying type of elements, if any.
            ਅਚਲ AstBasicDType* ਅਚਲ basicp = uaDTypep->basicp();
            // If used in a loop, we must have a dynamic commit queue. (Also works in suspendables)
            ਜੇ (vscpInfo.m_ਵਿੱਚ_Loop) {
                // Arrays with compound element types are currently not supported in loops
                ਜੇ (!basicp
                    || !(basicp->isIntegralOrPacked()  //
                         || basicp->isDouble()  //
                         || basicp->isString())) {
                    ਵਾਪਸ Scheme::Unsupported_Compound_Array_ਵਿੱਚ_Loop;
                }
                ਜੇ (vscpInfo.m_partial) ਵਾਪਸ Scheme::ਮੁੱਲ_ਕਤਾਰ_Partial;
                ਵਾਪਸ Scheme::ਮੁੱਲ_ਕਤਾਰ_ਸਾਰਾ;
            }
            // A suspendable process or fork can resume outside the 'nba' region, so the 'pre'
            // logic of the other schemes would undo its updates. The unique flag scheme has none.
            ਜੇ (vscpInfo.m_ਵਿੱਚ_Susp_Or_Fork) ਵਾਪਸ Scheme::FlagUnique;
            // Otherwise if an array of packed/basic elements, use the shared flag scheme
            ਜੇ (basicp) ਵਾਪਸ Scheme::FlagShared;
            // Finally fall back on the shadow variable scheme, e.g. for
            // arrays of unpacked structs. This will be slow.
            // TODO: generic LHS scheme as discussed in #5092
            ਵਾਪਸ Scheme::ShadowVar;
        }

        // Likewise, the 'pre' logic of the shadow variable scheme would undo the updates of a
        // suspendable process or fork, so use the unique flag scheme
        ਜੇ (vscpInfo.m_ਵਿੱਚ_Susp_Or_Fork) ਵਾਪਸ Scheme::FlagUnique;

        ਅਚਲ ਤਾਰਕਿਕ isIntegralOrPacked = dtypep->isIntegralOrPacked();
        // Check for mixed usage (this also warns if not OK)
        ਜੇ (checkMixedUsage(vscp, isIntegralOrPacked)) {
            // If it's a variable updated by both blocking and non-blocking
            // assignments, use the ShadowVarMasked schem if masked update is
            // possible. This can handle blocking and non-blocking updates to
            // inpdendent parts correctly at run-time, and always works, even
            // in loops or other dynamic context.
            ਜੇ (isIntegralOrPacked) ਵਾਪਸ Scheme::ShadowVarMasked;
            // If it's inside a loop, use Scheme::ShadowVar, which is safe,
            // but will generate incorrect code if a partial update is used
            ਜੇ (vscpInfo.m_ਵਿੱਚ_Loop) ਵਾਪਸ Scheme::ShadowVar;
            // Otherwise (for not packed variables), use the FlagUnique scheme,
            // which at least handles partial updates correctly, but might break
            // in loops or other dynamic context
            ਵਾਪਸ Scheme::FlagUnique;
        }

        // Otherwise use the simple shadow variable scheme
        ਵਾਪਸ Scheme::ShadowVar;
    }

    // Given an expression 'exprp', return a new expression that always evaluates to the
    // value of the given expression at this point in the program. That is:
    // - If given a non-constant expression, create a new temporary AstVarScope under the given
    //   'scopep', with the given 'name', assign the expression to it, and return a read reference
    //   to the new AstVarScope.
    // - If given a constant, just return that constant.
    // New statements are inserted before 'insertp'
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* captureVal(AstScope* ਅਚਲ scopep, Ast_ਗੰਢ_Stmt* ਅਚਲ insertp,
                            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp, ਅਚਲ std::string& name) {
        UASSERT_OBJ(!exprp->backp(), exprp, "Should have been unlinked");
        File_ਸਤਰ* ਅਚਲ flp = exprp->fileline();
        ਜੇ (VN_IS(exprp, Const)) ਵਾਪਸ exprp;
        // TODO: there are some const variables that could be simply referenced here
        AstVarScope* ਅਚਲ tmpVscp = m_dlyTmps.make(flp, scopep, exprp->dtypep(), name);
        insertp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(
            ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, tmpVscp, VAccess::WRITE}, exprp});
        ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, tmpVscp, VAccess::READ};
    };

    // Similar to 'captureVal', but captures an LValue expression. That is, the returned
    // expression will reference the same location as the input expression, at this point in the
    // program.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* captureLhs(AstScope* ਅਚਲ scopep, Ast_ਗੰਢ_Stmt* ਅਚਲ insertp,
                            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp, ਅਚਲ std::string& ਅਧਾਰ_Name) {
        UASSERT_OBJ(!lhsp->backp(), lhsp, "Should have been unlinked");
        // Running node pointer
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep = lhsp;
        // Capture AstSel indices - there should be only one
        ਜੇ (AstSel* ਅਚਲ selp = VN_CAST(nodep, Sel)) {
            ਅਚਲ std::string tmpName{"Lsb" + ਅਧਾਰ_Name};
            selp->lsbp(captureVal(scopep, insertp, selp->lsbp()->unlink_Fr_ਪਿੱਠ(), tmpName));
            // Continue with target
            nodep = selp->fromp();
        }
        UASSERT_OBJ(!VN_IS(nodep, Sel), lhsp, "Multiple 'AstSel' applied to LHS reference");
        // Capture the indices of selects - might be many, also below struct members. If a handle
        // selects the instance the target is a member of, capture the handle instead, as it is
        // only read, to select the target when the NBA is executed (IEEE 1800-2023 10.4.2).
        ਮਾਪ_t nArraySels = 0;
        ਜਦੋਂ (ਸੱਚ) {
            ਜੇ (Ast_ਗੰਢ_Sel* ਅਚਲ selp = VN_CAST(nodep, ਗੰਢ_Sel)) {
                ਅਚਲ std::string tmpName{"Dim" + std::ਮੰਜ਼ਿਲ_string(nArraySels++) + ਅਧਾਰ_Name};
                selp->bitp(captureVal(scopep, insertp, selp->bitp()->unlink_Fr_ਪਿੱਠ(), tmpName));
                nodep = selp->fromp();
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstStructSel* ਅਚਲ selp = VN_CAST(nodep, StructSel)) {
                nodep = selp->fromp();
            } ਨਹੀਂਤਾਂ ਜੇ (AstMemberSel* ਅਚਲ selp = VN_CAST(nodep, MemberSel)) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ handlep = selp->fromp()->unlink_Fr_ਪਿੱਠ();
                V3LinkLValue::link_LValue_ਸੈੱਟ(handlep, VAccess::READ);
                AstVarRef* ਅਚਲ refp
                    = VN_AS(captureVal(scopep, insertp, handlep, "Handle" + ਅਧਾਰ_Name), VarRef);
                refp->varp()->ਸੈੱਟ_Ignore_Post_Write();
                // Like the handle it replaces, mark it written, as is the member it selects
                refp->access(selp->access());
                selp->fromp(refp);
                ਵਾਪਸ lhsp;
            } ਨਹੀਂਤਾਂ {
                ਰੋਕੋ;
            }
        }
        // What remains must be the variable, or some sort of select, we assume can reuse it
        UASSERT_OBJ(nodep->is_ਸ਼ੁੱਧ(), lhsp, "Malformed LHS in NBA");
        // Now have been converted to use the captured values
        ਵਾਪਸ lhsp;
    }

    ਖਾਲੀ ਜੋੜੋ_CFunc_Cached_ਮੁੱਲਾਂ(ਅਚਲ AstCFunc* ਅਚਲ cfuncp,
                              std::unordered_ਸੈੱਟ<ਅਚਲ AstCFunc*>& ਦੇਖਿਆ) {
        ਜੇ (!ਦੇਖਿਆ.ਪਾਓ(cfuncp).ਦੂਜਾ) ਵਾਪਸ;
        CFuncCache& ਮੁੱਲ = m_cfuncsCache(cfuncp);
        m_timingDomains.ਪਾਓ(ਮੁੱਲ.m_timingDomains.begin(), ਮੁੱਲ.m_timingDomains.ਸਿਰਾ());
        m_non_Inlined_CAwaits_With_Sen_ਰੁੱਖ += ਮੁੱਲ.m_timingDomains.ਮਾਪ();
        ਲਈ (ਅਚਲ AstCFunc* ਅਚਲ includedp : ਮੁੱਲ.m_includes) {
            ਜੋੜੋ_CFunc_Cached_ਮੁੱਲਾਂ(includedp, ਦੇਖਿਆ);
        }
    }

    // Create a temporary variable in the given 'scopep', with the given 'name', and with 'dtypep'
    // type, with the bits selected by 'sLsbp'/'sWidthp' set to 'valuep', other bits set to zero.
    // Insert new statements before 'insertp'.
    // Returns a read reference to the temporary variable.
    AstVarRef* createWidened(File_ਸਤਰ* flp, AstScope* scopep, Ast_ਗੰਢ_DType* dtypep,
                             Ast_ਗੰਢ_ਵਿਅੰਜਕ* sLsbp, ਪੂਰਨ s_ਚੌੜਾਈ, ਅਚਲ std::string& name,
                             Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep, Ast_ਗੰਢ* insertp) {
        // Create temporary variable.
        AstVarScope* ਅਚਲ tp = m_dlyTmps.make(flp, scopep, dtypep, name);
        // Zero it
        AstConst* ਅਚਲ zerop = ਨਵਾਂ AstConst{flp, AstConst::DTyped{}, dtypep};
        zerop->ਅੰਕ().ਸੈੱਟ_All_Bits0();
        insertp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(
            ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, tp, VAccess::WRITE}, zerop});
        // Set the selected bits to 'valuep'
        AstSel* ਅਚਲ selp = ਨਵਾਂ AstSel{flp, ਨਵਾਂ AstVarRef{flp, tp, VAccess::WRITE},
                                        sLsbp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ), s_ਚੌੜਾਈ};
        insertp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, selp, valuep});
        // This is the expression to get the value of the temporary
        ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, tp, VAccess::READ};
    }

    // Scheme::ShadowVar
    ਖਾਲੀ prepareSchemeShadowVar(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
        AstScope* ਅਚਲ scopep = vscp->scopep();
        // Create the shadow variable
        ਅਚਲ std::string name = "_" + vscp->varp()->shortName();
        AstVarScope* ਅਚਲ shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), name);
        vscpInfo.shadowVariableKit().vscp = shadowVscp;
        // Mark both for V3LifePsot
        vscp->optimizeLifePost(ਸੱਚ);
        shadowVscp->optimizeLifePost(ਸੱਚ);
        // Create the AstActive for the Pre/Post logic
        Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "nba-shadow-variable", vscpInfo.senTreep()};
        activep->sen_ਰੁੱਖ_Storep(vscpInfo.senTreep());
        scopep->ਜੋੜੋ_Blocksp(activep);
        // Add 'Pre' scheduled 'shadowVariable = originalVariable' assignment
        AstAlwaysPre* ਅਚਲ prep = ਨਵਾਂ AstAlwaysPre{flp};
        activep->ਜੋੜੋ_Stmtsp(prep);
        prep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, shadowVscp, VAccess::WRITE},
                                      ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ}});
        // Add 'Post' scheduled 'originalVariable = shadowVariable' assignment
        AstAlwaysPost* ਅਚਲ postp = ਨਵਾਂ AstAlwaysPost{flp};
        activep->ਜੋੜੋ_Stmtsp(postp);
        postp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, vscp, VAccess::WRITE},
                                       ਨਵਾਂ AstVarRef{flp, shadowVscp, VAccess::READ}});
    }
    ਖਾਲੀ convertSchemeShadowVar(Ast_ਸੌਂਪੋ_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        AstVarScope* ਅਚਲ shadowVscp = vscpInfo.shadowVariableKit().vscp;

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ਅਚਲ refp) {
            ਜੇ (!refp->access().isWriteOnly()) ਵਾਪਸ;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
    }

    // Scheme::ShadowVarMasked
    ਖਾਲੀ prepareSchemeShadowVarMasked(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
        AstScope* ਅਚਲ scopep = vscp->scopep();
        // Create the shadow variable
        ਅਚਲ std::string shadowName = "_" + vscp->varp()->shortName();
        AstVarScope* ਅਚਲ shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), shadowName);
        vscpInfo.shadowVarMaskedKit().vscp = shadowVscp;
        // Create the makk variable
        ਅਚਲ std::string ਨਕਾਬ_Name = "Mask__" + vscp->varp()->shortName();
        AstVarScope* ਅਚਲ ਨਕਾਬ_Vscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), ਨਕਾਬ_Name);
        ਨਕਾਬ_Vscp->varp()->ਸੈੱਟ_Ignore_Post_Write();
        vscpInfo.shadowVarMaskedKit().maskp = ਨਕਾਬ_Vscp;
        // Create the AstActive for the Post logic
        Ast_ਸਰਗਰਮ* ਅਚਲ activep
            = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "nba-shadow-var-masked", vscpInfo.senTreep()};
        activep->sen_ਰੁੱਖ_Storep(vscpInfo.senTreep());
        scopep->ਜੋੜੋ_Blocksp(activep);
        // Add 'Post' scheduled process for the commit and mask clear
        AstAlwaysPost* ਅਚਲ postp = ਨਵਾਂ AstAlwaysPost{flp};
        activep->ਜੋੜੋ_Stmtsp(postp);
        // Add the commit - vscp = (shadowVscp & maskVscp) | (vscp & ~maskVscp);
        postp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
            flp, ਨਵਾਂ AstVarRef{flp, vscp, VAccess::WRITE},
            ਨਵਾਂ AstOr{flp,
                      ਨਵਾਂ AstAnd{flp, ਨਵਾਂ AstVarRef{flp, shadowVscp, VAccess::READ},
                                 ਨਵਾਂ AstVarRef{flp, ਨਕਾਬ_Vscp, VAccess::READ}},
                      ਨਵਾਂ AstAnd{flp, ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ},
                                 ਨਵਾਂ AstNot{flp, ਨਵਾਂ AstVarRef{flp, ਨਕਾਬ_Vscp, VAccess::READ}}}}});
        vscp->varp()->ਸੈੱਟ_Ignore_Post_Read();
        // Clar the mask - maskVscp = '0;
        postp->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, ਨਕਾਬ_Vscp, VAccess::WRITE},
                          ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, ਨਕਾਬ_Vscp->ਚੌੜਾਈ(), 0}});
    }
    ਖਾਲੀ convertSchemeShadowVarMasked(Ast_ਸੌਂਪੋ_Dly* nodep, AstVarScope* vscp,
                                      VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        AstVarScope* ਅਚਲ shadowVscp = vscpInfo.shadowVarMaskedKit().vscp;
        AstVarScope* ਅਚਲ ਨਕਾਬ_Vscp = vscpInfo.shadowVarMaskedKit().maskp;

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsClonep = nodep->lhsp()->clone_ਰੁੱਖ(ਝੂਠ);

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ਅਚਲ refp) {
            ਜੇ (!refp->access().isWriteOnly()) ਵਾਪਸ;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
        // Set the same bits in the mask to 1
        lhsClonep->foreach([&](AstVarRef* ਅਚਲ refp) {
            ਜੇ (!refp->access().isWriteOnly()) ਵਾਪਸ;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(ਨਕਾਬ_Vscp);
            refp->varp(ਨਕਾਬ_Vscp->varp());
        });
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        AstConst* ਅਚਲ onesp = ਨਵਾਂ AstConst{flp, AstConst::DTyped{}, lhsClonep->dtypep()};
        onesp->ਅੰਕ().ਸੈੱਟ_All_Bits1();
        nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsClonep, onesp});
    }

    // Scheme::FlagShared
    ਖਾਲੀ prepareSchemeFlagShared(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
        AstScope* ਅਚਲ scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "nba-flag-shared", vscpInfo.senTreep()};
        activep->sen_ਰੁੱਖ_Storep(vscpInfo.senTreep());
        scopep->ਜੋੜੋ_Blocksp(activep);
        vscpInfo.flagSharedKit().activep = activep;
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ਅਚਲ postp = ਨਵਾਂ AstAlwaysPost{flp};
        activep->ਜੋੜੋ_Stmtsp(postp);
        vscpInfo.flagSharedKit().postp = postp;
        // Initialize
        vscpInfo.flagSharedKit().commitFlagp = ਖਾਲੀ_ਸੂਚਕ;
        vscpInfo.flagSharedKit().commitIfp = ਖਾਲੀ_ਸੂਚਕ;
    }
    ਖਾਲੀ convertSchemeFlagShared(Ast_ਸੌਂਪੋ_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
        AstScope* ਅਚਲ scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ਅਚਲ std::string ਅਧਾਰ_Name = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlink_Fr_ਪਿੱਠ(), "Val" + ਅਧਾਰ_Name);

        // Unlink and capture the LHS reference
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlink_Fr_ਪਿੱਠ(), ਅਧਾਰ_Name);

        // Is this NBA adjacent after the previously processed NBA?
        ਅਚਲ ਤਾਰਕਿਕ consecutive = nodep == m_ਅਗਲਾ_Dlyp;
        m_ਅਗਲਾ_Dlyp = VN_CAST(nodep->nextp(), ਸੌਂਪੋ_Dly);

        VarScopeInfo* ਅਚਲ prevVscpInfop = consecutive ? &m_vscpInfo(m_prevVscp) : ਖਾਲੀ_ਸੂਚਕ;

        // We can reuse the flag of the previous assignment if:
        ਅਚਲ ਤਾਰਕਿਕ reuseTheFlag =
            // Consecutive NBAs
            consecutive
            // ... that use the same scheme
            && prevVscpInfop->m_scheme == Scheme::FlagShared
            // ... and are in the same scope as the target variable
            && scopep == vscp->scopep()
            // ... and share the same overall update domain
            && prevVscpInfop->senTreep()->same_ਰੁੱਖ(vscpInfo.senTreep());

        ਜੇ (!reuseTheFlag) {
            // Create new flag
            AstVarScope* ਅਚਲ flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + ਅਧਾਰ_Name);
            // Set the flag at the original NBA
            nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(  //
                ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, flagVscp, VAccess::WRITE},
                              ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}});
            // Add the 'Pre' scheduled reset for the flag
            AstAlwaysPre* ਅਚਲ prep = ਨਵਾਂ AstAlwaysPre{flp};
            vscpInfo.flagSharedKit().activep->ਜੋੜੋ_Stmtsp(prep);
            prep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, flagVscp, VAccess::WRITE},
                                          ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}});
            // Add the 'Post' scheduled commit
            AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstVarRef{flp, flagVscp, VAccess::READ}};
            vscpInfo.flagSharedKit().postp->ਜੋੜੋ_Stmtsp(ifp);
            vscpInfo.flagSharedKit().commitFlagp = flagVscp;
            vscpInfo.flagSharedKit().commitIfp = ifp;
        } ਨਹੀਂਤਾਂ {
            ਜੇ (vscp != m_prevVscp) {
                // Different variable, ensure the commit block exists for this variable,
                // can reuse existing one with the same flag, otherwise create a new one.
                AstVarScope* ਅਚਲ flagVscp = prevVscpInfop->flagSharedKit().commitFlagp;
                UASSERT_OBJ(flagVscp, nodep, "Commit flag of previous assignment should exist");
                ਜੇ (vscpInfo.flagSharedKit().commitFlagp != flagVscp) {
                    AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstVarRef{flp, flagVscp, VAccess::READ}};
                    vscpInfo.flagSharedKit().postp->ਜੋੜੋ_Stmtsp(ifp);
                    vscpInfo.flagSharedKit().commitFlagp = flagVscp;
                    vscpInfo.flagSharedKit().commitIfp = ifp;
                }
            } ਨਹੀਂਤਾਂ {
                // Same variable, reuse the commit block of the previous assignment
                vscpInfo.flagSharedKit().commitFlagp = prevVscpInfop->flagSharedKit().commitFlagp;
                vscpInfo.flagSharedKit().commitIfp = prevVscpInfop->flagSharedKit().commitIfp;
            }
            ++m_n_Shared_ਸੈੱਟ_Flags;
        }
        // Commit the captured value to the captured destination
        vscpInfo.flagSharedKit().commitIfp->ਜੋੜੋ_Thensp(
            ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, capturedLhsp, capturedRhsp});

        // Remember the variable for the next NBA
        m_prevVscp = vscp;

        // Delete original NBA
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }

    // Scheme::FlagUnique
    ਖਾਲੀ prepareSchemeFlagUnique(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
        AstScope* ਅਚਲ scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "nba-flag-unique", vscpInfo.senTreep()};
        activep->sen_ਰੁੱਖ_Storep(vscpInfo.senTreep());
        scopep->ਜੋੜੋ_Blocksp(activep);
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ਅਚਲ postp = ਨਵਾਂ AstAlwaysPost{flp};
        activep->ਜੋੜੋ_Stmtsp(postp);
        vscpInfo.flagUniqueKit().postp = postp;
    }
    ਖਾਲੀ convertSchemeFlagUnique(Ast_ਸੌਂਪੋ_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
        AstScope* ਅਚਲ scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ਅਚਲ std::string ਅਧਾਰ_Name = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlink_Fr_ਪਿੱਠ(), "Val" + ਅਧਾਰ_Name);

        // Unlink and capture the LHS reference
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlink_Fr_ਪਿੱਠ(), ਅਧਾਰ_Name);

        // Create new flag
        AstVarScope* ਅਚਲ flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + ਅਧਾਰ_Name);
        flagVscp->varp()->ਸੈੱਟ_Ignore_Post_Write();
        // Set the flag at the original NBA
        nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(  //
            ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, flagVscp, VAccess::WRITE},
                          ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}});
        // Add the 'Post' scheduled commit
        AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstVarRef{flp, flagVscp, VAccess::READ}};
        vscpInfo.flagUniqueKit().postp->ਜੋੜੋ_Stmtsp(ifp);
        // Immediately clear the flag
        ifp->ਜੋੜੋ_Thensp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, flagVscp, VAccess::WRITE},
                                     ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}});
        // Commit the value
        ifp->ਜੋੜੋ_Thensp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, capturedLhsp, capturedRhsp});

        // Delete original NBA
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }

    // Scheme::ValueQueuePartial/Scheme::ValueQueueWhole
    ਸਾਂਚਾ <ਤਾਰਕਿਕ N_Partial>
    ਖਾਲੀ prepare_Scheme_ਮੁੱਲ_ਕਤਾਰ(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(N_Partial ? vscpInfo.m_scheme == Scheme::ਮੁੱਲ_ਕਤਾਰ_Partial
                              : vscpInfo.m_scheme == Scheme::ਮੁੱਲ_ਕਤਾਰ_ਸਾਰਾ,
                    vscp, "Inconsistent NBA scheme");
        File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
        AstScope* ਅਚਲ scopep = vscp->scopep();

        // Create the commit queue variable
        Ast_ਗੰਢ_DType* ਅਚਲ elemDTypep = vscp->dtypep()->skipRefp();
        Ast_NBACommit_ਕਤਾਰ_DType*& cqDTypep = m_cqDTypeps[{elemDTypep, N_Partial}];
        ਜੇ (!cqDTypep) {
            cqDTypep = ਨਵਾਂ Ast_NBACommit_ਕਤਾਰ_DType{flp, elemDTypep, N_Partial};
            v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(cqDTypep);
        }
        ਅਚਲ std::string name = "CommitQueue__" + vscp->varp()->shortName();
        AstVarScope* ਅਚਲ ਕਤਾਰ_Vscp = m_dlyTmps.make(flp, scopep, cqDTypep, name);
        ਕਤਾਰ_Vscp->varp()->noReset(ਸੱਚ);
        ਕਤਾਰ_Vscp->varp()->ਸੈੱਟ_Ignore_Post_Write();
        vscpInfo.ਮੁੱਲ_ਕਤਾਰ_Kit().vscp = ਕਤਾਰ_Vscp;
        // Create the AstActive for the Post logic
        Ast_ਸਰਗਰਮ* ਅਚਲ activep
            = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "nba-value-queue-whole", vscpInfo.senTreep()};
        activep->sen_ਰੁੱਖ_Storep(vscpInfo.senTreep());
        scopep->ਜੋੜੋ_Blocksp(activep);
        // Add 'Post' scheduled process for the commit
        AstAlwaysPost* ਅਚਲ postp = ਨਵਾਂ AstAlwaysPost{flp};
        activep->ਜੋੜੋ_Stmtsp(postp);
        // Add the commit
        AstCMethodHard* ਅਚਲ callp = ਨਵਾਂ AstCMethodHard{
            flp, ਨਵਾਂ AstVarRef{flp, ਕਤਾਰ_Vscp, VAccess::READWRITE}, VCMethod::NBA_COMMIT};
        callp->dtype_ਸੈੱਟ_Void();
        // TODO: this is a partial update, so must be READWRITE, but that breaks scheduling
        callp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstVarRef{flp, vscp, VAccess::WRITE});
        postp->ਜੋੜੋ_Stmtsp(callp->makeStmt());
    }

    ਖਾਲੀ convert_Scheme_ਮੁੱਲ_ਕਤਾਰ(Ast_ਸੌਂਪੋ_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo,
                                 ਤਾਰਕਿਕ partial) {
        UASSERT_OBJ(partial ? vscpInfo.m_scheme == Scheme::ਮੁੱਲ_ਕਤਾਰ_Partial
                            : vscpInfo.m_scheme == Scheme::ਮੁੱਲ_ਕਤਾਰ_ਸਾਰਾ,
                    vscp, "Inconsistent NBA scheme");
        File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
        AstScope* ਅਚਲ scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ਅਚਲ std::string ਅਧਾਰ_Name = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlink_Fr_ਪਿੱਠ(), "Val" + ਅਧਾਰ_Name);

        // Unlink and capture the LHS reference
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlink_Fr_ਪਿੱਠ(), ਅਧਾਰ_Name);

        // RHS value (can be widened/masked iff Partial)
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = capturedRhsp;
        // RHS mask (iff Partial)
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* maskp = ਖਾਲੀ_ਸੂਚਕ;

        // Running node for LHS deconstruction
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsNodep = capturedLhsp;

        // If partial updates are required, construct the mask and the widened value
        ਜੇ (partial) {
            // Type of array element
            Ast_ਗੰਢ_DType* ਅਚਲ eDTypep = [&]() -> Ast_ਗੰਢ_DType* {
                Ast_ਗੰਢ_DType* dtypep = vscp->dtypep()->skipRefp();
                ਜਦੋਂ (AstUnpackArrayDType* ਅਚਲ ap = VN_CAST(dtypep, UnpackArrayDType)) {
                    dtypep = ap->subDTypep()->skipRefp();
                }
                ਵਾਪਸ dtypep;
            }();

            ਜੇ (ਅਚਲ AstSel* ਅਚਲ lSelp = VN_CAST(lhsNodep, Sel)) {
                // This is a partial assignment.
                // Need to create a mask and widen the value to element size.
                lhsNodep = lSelp->fromp();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ sLsbp = lSelp->lsbp();
                ਅਚਲ ਪੂਰਨ s_ਚੌੜਾਈ = lSelp->ਚੌੜਾਈ_Const();

                // Create mask value
                maskp = [&]() -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
                    // Constant mask we can compute here
                    ਜੇ (ਅਚਲ AstConst* ਅਚਲ cLsbp = VN_CAST(sLsbp, Const)) {
                        AstConst* ਅਚਲ cp = ਨਵਾਂ AstConst{flp, AstConst::DTyped{}, eDTypep};
                        cp->ਅੰਕ().ਸੈੱਟ_ਨਕਾਬ(s_ਚੌੜਾਈ, cLsbp->ਮੰਜ਼ਿਲ_SInt());
                        ਵਾਪਸ cp;
                    }

                    // A non-constant mask we must compute at run-time.
                    AstConst* ਅਚਲ onesp = ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, s_ਚੌੜਾਈ, 0};
                    onesp->ਅੰਕ().ਸੈੱਟ_All_Bits1();
                    ਵਾਪਸ createWidened(flp, scopep, eDTypep, sLsbp, s_ਚੌੜਾਈ, "Mask" + ਅਧਾਰ_Name,
                                         onesp, nodep);
                }();

                // Adjust value to element size
                valuep = [&]() -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
                    // Constant value with constant select we can compute here
                    ਜੇ (AstConst* ਅਚਲ cValuep = VN_CAST(valuep, Const)) {
                        ਜੇ (ਅਚਲ AstConst* ਅਚਲ cLsbp = VN_CAST(sLsbp, Const)) {
                            AstConst* ਅਚਲ cp = ਨਵਾਂ AstConst{flp, AstConst::DTyped{}, eDTypep};
                            cp->ਅੰਕ().ਸੈੱਟ_All_Bits0();
                            cp->ਅੰਕ().opSelInto(cValuep->ਅੰਕ(), cLsbp->ਮੰਜ਼ਿਲ_SInt(), s_ਚੌੜਾਈ);
                            VL_DO_DANGLING(valuep->delete_ਰੁੱਖ(), valuep);
                            ਵਾਪਸ cp;
                        }
                    }

                    // A non-constant value we must adjust.
                    ਵਾਪਸ createWidened(flp, scopep, eDTypep, sLsbp, s_ਚੌੜਾਈ,  //
                                         "Elem" + ਅਧਾਰ_Name, valuep, nodep);
                }();
            } ਨਹੀਂਤਾਂ {
                // If this assignment is not partial, set mask to ones and we are done
                AstConst* ਅਚਲ ones = ਨਵਾਂ AstConst{flp, AstConst::DTyped{}, eDTypep};
                ones->ਅੰਕ().ਸੈੱਟ_All_Bits1();
                maskp = ones;
            }
        }

        // Extract array indices
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> idxps;
        {
            UASSERT_OBJ(VN_IS(lhsNodep, ArraySel), lhsNodep, "Unexpected LHS form");
            ਜਦੋਂ (AstArraySel* ਅਚਲ aSelp = VN_CAST(lhsNodep, ArraySel)) {
                idxps.emplace_ਪਿੱਠ(aSelp->bitp()->unlink_Fr_ਪਿੱਠ());
                lhsNodep = aSelp->fromp();
            }
            UASSERT_OBJ(VN_IS(lhsNodep, VarRef), lhsNodep, "Unexpected LHS form");
            std::ਉਲਟ(idxps.begin(), idxps.ਸਿਰਾ());
        }

        // Done with the LHS at this point
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(capturedLhsp), capturedLhsp);

        // Enqueue the update at the site of the original NBA
        AstCMethodHard* ਅਚਲ callp = ਨਵਾਂ AstCMethodHard{
            flp, ਨਵਾਂ AstVarRef{flp, vscpInfo.ਮੁੱਲ_ਕਤਾਰ_Kit().vscp, VAccess::READWRITE},
            VCMethod::NBA_ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ};
        callp->dtype_ਸੈੱਟ_Void();
        callp->ਜੋੜੋ_Pinsp(valuep);
        ਜੇ (partial) callp->ਜੋੜੋ_Pinsp(maskp);
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ indexp : idxps) callp->ਜੋੜੋ_Pinsp(indexp);
        nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(callp->makeStmt());

        // Delete original NBA
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }

    // Record where a variable is assigned
    ਖਾਲੀ ਰਿਕਾਰਡ_Write_Ref(AstVarRef* nodep, ਤਾਰਕਿਕ nonBlocking) {
        // Ignore references in certain contexts
        ਜੇ (m_ignoreBlkAndNBlk) ਵਾਪਸ;
        // Ignore if it's an array
        // TODO: we do this because it used to be the previous behaviour.
        //       Is it still required, or should we warn for arrays as well?
        //       Scheduling is no different for them...
        //       Clarification: This is OK for arrays of primitive types, but
        //       arrays that use the ShadowVar scheme don't work...
        ਜੇ (VN_IS(nodep->varScopep()->dtypep()->skipRefp(), UnpackArrayDType)) ਵਾਪਸ;

        m_writeRefs(nodep->varScopep()).emplace_ਪਿੱਠ(nodep, nonBlocking, m_ਵਿੱਚ_Non_Comb_Logic);
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ Procedure_T>
    ਸਥਿਰ ਤਾਰਕਿਕ isProcedureWithSentreep(ਅਚਲ Ast_ਗੰਢ_Procedure* ਅਚਲ nodep) {
        ਅਚਲ Procedure_T* ਅਚਲ procedurep = Ast_ਗੰਢ::cast<Procedure_T>(nodep);
        ਵਾਪਸ procedurep && procedurep->sentreep();
    }

    // Visit AstCFunc from a AstNodeCCall - this is made into a separate quasi-visitor because
    // AstCFunc that is not called from the code (e.g.: DPI exports) does not need to be visited
    // this way. Also, not visiting such AstCFuncs allows to avoid caching results for them which
    // this function does - which could lead to excessive memory usage
    ਖਾਲੀ ਦੇਖੋ_Called_CFunc(AstCFunc* ਅਚਲ nodep) {
        CFuncCache& ਮੁੱਲ = m_cfuncsCache(nodep);
        ਚੋਣ (ਮੁੱਲ.m_state) {
        ਹਾਲਤ CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_timingDomains);

            // Visit
            ਮੁੱਲ.m_state = CFuncCache::VISITING;
            m_call_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(nodep);
            {
                VL_RESTORER(m_cfuncp);
                m_cfuncp = nodep;
                iterateChildren(nodep);
            }
            m_call_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
            ਮੁੱਲ.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::ਅਦਲਾ_ਬਦਲੀ(m_timingDomains, ਮੁੱਲ.m_timingDomains);
        } ਰੋਕੋ;
        ਹਾਲਤ CFuncCache::VISITING: {
            ਲਈ (ਮਾਪ_t i = m_call_ਢੇਰ.ਮਾਪ() - 1; m_call_ਢੇਰ.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ਢੇਰ[i]).m_includes.ਪਾਓ(nodep);
            }
            ਵਾਪਸ;  // Break recursion
        }
        ਹਾਲਤ CFuncCache::INITIALIZED: ਰੋਕੋ;
        }
        std::unordered_ਸੈੱਟ<ਅਚਲ AstCFunc*> ਦੇਖਿਆ;
        ਜੋੜੋ_CFunc_Cached_ਮੁੱਲਾਂ(nodep, ਦੇਖਿਆ);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override {
        iterateChildren(nodep);
        // Decide which scheme to use for each variable and do the 'prepare' step
        ਲਈ (AstVarScope* ਅਚਲ vscp : m_vscps) {
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            vscpInfo.m_scheme = ਚੁਣੋ_Scheme(vscp, vscpInfo);
            // Run 'prepare' step
            ਚੋਣ (vscpInfo.m_scheme) {
            ਹਾਲਤ Scheme::Undecided:  // LCOV_EXCL_START
                UASSERT_OBJ(ਝੂਠ, vscp, "Failed to choose NBA scheme");
                ਰੋਕੋ;  // LCOV_EXCL_STOP
            ਹਾਲਤ Scheme::Unsupported_Compound_Array_ਵਿੱਚ_Loop: {
                // Will report error at the site of the NBA
                ਰੋਕੋ;
            }
            ਹਾਲਤ Scheme::ShadowVar: {
                ++m_nSchemeShadowVar;
                prepareSchemeShadowVar(vscp, vscpInfo);
                ਰੋਕੋ;
            }
            ਹਾਲਤ Scheme::ShadowVarMasked: {
                ++m_nSchemeShadowVarMasked;
                prepareSchemeShadowVarMasked(vscp, vscpInfo);
                ਰੋਕੋ;
            }
            ਹਾਲਤ Scheme::FlagShared: {
                ++m_nSchemeFlagShared;
                prepareSchemeFlagShared(vscp, vscpInfo);
                ਰੋਕੋ;
            }
            ਹਾਲਤ Scheme::FlagUnique: {
                ++m_nSchemeFlagUnique;
                prepareSchemeFlagUnique(vscp, vscpInfo);
                ਰੋਕੋ;
            }
            ਹਾਲਤ Scheme::ਮੁੱਲ_ਕਤਾਰ_ਸਾਰਾ: {
                ++m_n_Scheme_ਮੁੱਲ_Queues_ਸਾਰਾ;
                prepare_Scheme_ਮੁੱਲ_ਕਤਾਰ</* Partial: */ ਝੂਠ>(vscp, vscpInfo);
                ਰੋਕੋ;
            }
            ਹਾਲਤ Scheme::ਮੁੱਲ_ਕਤਾਰ_Partial: {
                ++m_n_Scheme_ਮੁੱਲ_Queues_Partial;
                prepare_Scheme_ਮੁੱਲ_ਕਤਾਰ</* Partial: */ ਸੱਚ>(vscp, vscpInfo);
                ਰੋਕੋ;
            }
            }
        }
        // Convert all NBAs
        ਲਈ (ਅਚਲ NBA& nba : m_nbas) {
            Ast_ਸੌਂਪੋ_Dly* ਅਚਲ nbap = nba.nodep;
            AstVarScope* ਅਚਲ vscp = nba.vscp;
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            // Run 'convert' step
            ਚੋਣ (vscpInfo.m_scheme) {
            ਹਾਲਤ Scheme::Undecided: {  // LCOV_EXCL_START
                UASSERT_OBJ(ਝੂਠ, vscp, "Unreachable");
                ਰੋਕੋ;
            }  // LCOV_EXCL_STOP
            ਹਾਲਤ Scheme::Unsupported_Compound_Array_ਵਿੱਚ_Loop: {
                // TODO: make this an E_UNSUPPORTED...
                nbap->v3warn(BLKLOOPINIT, "Unsupported: Non-blocking assignment to array with "
                                          "compound element type inside loop");
                ਰੋਕੋ;
            }
            ਹਾਲਤ Scheme::ShadowVar: {
                convertSchemeShadowVar(nbap, vscp, vscpInfo);
                ਰੋਕੋ;
            }
            ਹਾਲਤ Scheme::ShadowVarMasked: {
                convertSchemeShadowVarMasked(nbap, vscp, vscpInfo);
                ਰੋਕੋ;
            }
            ਹਾਲਤ Scheme::FlagShared: {
                convertSchemeFlagShared(nbap, vscp, vscpInfo);
                ਰੋਕੋ;
            }
            ਹਾਲਤ Scheme::FlagUnique: {
                convertSchemeFlagUnique(nbap, vscp, vscpInfo);
                ਰੋਕੋ;
            }
            ਹਾਲਤ Scheme::ਮੁੱਲ_ਕਤਾਰ_ਸਾਰਾ: {
                convert_Scheme_ਮੁੱਲ_ਕਤਾਰ(nbap, vscp, vscpInfo, /* partial: */ ਝੂਠ);
                ਰੋਕੋ;
            }
            ਹਾਲਤ Scheme::ਮੁੱਲ_ਕਤਾਰ_Partial:
                convert_Scheme_ਮੁੱਲ_ਕਤਾਰ(nbap, vscp, vscpInfo, /* partial: */ ਸੱਚ);
                ਰੋਕੋ;
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਰਗਰਮ* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ignoreBlkAndNBlk);
        VL_RESTORER(m_ਵਿੱਚ_Non_Comb_Logic);
        m_activep = nodep;
        ਅਚਲ Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = nodep->sentreep();
        m_ignoreBlkAndNBlk = senTreep->hasStatic() || senTreep->hasInitial();
        m_ਵਿੱਚ_Non_Comb_Logic = senTreep->hasClocked();
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override {
        VL_RESTORER(m_needsInitialTrigger);
        VL_RESTORER_CLEAR(m_nbaEventSenTreeps);
        ਅਚਲ ਮਾਪ_t ਪਹਿਲਾ_NBAAdded_ਸੂਚਕਾਂਕ = m_nbas.ਮਾਪ();
        {
            VL_RESTORER(m_ਵਿੱਚ_Suspendable_Or_Fork);
            VL_RESTORER(m_procp);
            VL_RESTORER(m_ignoreBlkAndNBlk);
            VL_RESTORER(m_ਵਿੱਚ_Non_Comb_Logic);
            // When we are dealing with initial block we need to
            // treat it as suspendable when we meet a NBA
            m_ਵਿੱਚ_Suspendable_Or_Fork = nodep->isSuspendable() || VN_IS(nodep, Initial);
            m_procp = nodep;
            ਜੇ (nodep->isSuspendable()) {
                m_ignoreBlkAndNBlk = ਝੂਠ;
                m_ਵਿੱਚ_Non_Comb_Logic = ਸੱਚ;
            }
            iterateChildren(nodep);
        }
        ਸਵੈਚਾਲਿਤ ਸ਼ਾਮਲ_ਹੈ_Clocled = [](ਅਚਲ Ast_Sen_ਵਸਤੂ* itemp) {
            ਜਦੋਂ (itemp) {
                ਜੇ (itemp->ਕਿਨਾਰਾ_Type().clockedStmt()) ਵਾਪਸ ਸੱਚ;
                itemp = VN_AS(itemp->nextp(), Sen_ਵਸਤੂ);
            }
            ਵਾਪਸ ਝੂਠ;
        };
        ਅਚਲ ਤਾਰਕਿਕ ਜੋੜੋ_Initial_Trigger = m_needsInitialTrigger
                                       && !(isProcedureWithSentreep<AstAlways>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysObserved>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysReactive>(nodep))
                                       && !ਸ਼ਾਮਲ_ਹੈ_Clocled(m_activep->sentreep()->sensesp());
        ਜੇ (m_timingDomains.ਖਾਲੀ_ਥਾਵਾਂ() && !ਜੋੜੋ_Initial_Trigger) ਵਾਪਸ;

        // There were some timing domains involved in the process. Add all of them as sensitivities
        // of all NBA targets in this process. Note this is a bit of a sledgehammer, we should only
        // need those that directly precede the NBA in control flow, but that is hard to compute,
        // so we will hammer away.

        // First gather all senItems
        Ast_Sen_ਵਸਤੂ* senItemp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (ਜੋੜੋ_Initial_Trigger) {
            senItemp = ਨਵਾਂ Ast_Sen_ਵਸਤੂ{nodep->fileline(), Ast_Sen_ਵਸਤੂ::InitialNBA{}};
            ++m_nInitialNBA;
        }

        ਲਈ (ਅਚਲ Ast_Sen_ਰੁੱਖ* ਅਚਲ domainp : m_timingDomains) {
            ਜੇ (domainp->sensesp())
                senItemp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(senItemp, domainp->sensesp()->clone_ਰੁੱਖ(ਸੱਚ));
        }
        m_timingDomains.clear();
        // Add them to all nba targets we gathered in this process
        ਲਈ (ਮਾਪ_t i = ਪਹਿਲਾ_NBAAdded_ਸੂਚਕਾਂਕ; i < m_nbas.ਮਾਪ(); ++i) {
            m_vscpInfo(m_nbas[i].vscp).ਜੋੜੋ_ਸੰਵੇਦਨਸ਼ੀਲਤਾ(senItemp);
        }
        ਲਈ (Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep : m_nbaEventSenTreeps)
            senTreep->ਜੋੜੋ_Sensesp(senItemp->clone_ਰੁੱਖ(ਸੱਚ));
        // Done with these
        VL_DO_DANGLING(senItemp->delete_ਰੁੱਖ(), senItemp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Suspendable_Or_Fork);
        m_ਵਿੱਚ_Suspendable_Or_Fork = ਸੱਚ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCAwait* nodep) override {
        ਜੇ (nodep->sentreep()) m_timingDomains.ਪਾਓ(nodep->sentreep());
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFireEvent* nodep) override {
        UASSERT_OBJ(v3Global.hasEvents(), nodep, "Inconsistent");
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ eventp = nodep->operandp()->unlink_Fr_ਪਿੱਠ();

        // Enqueue for clearing 'triggered' state on next eval
        AstCStmt* ਅਚਲ cstmtp = ਨਵਾਂ AstCStmt{flp};
        cstmtp->ਜੋੜੋ("vlSymsp->fireEvent(");
        cstmtp->ਜੋੜੋ(eventp);
        cstmtp->ਜੋੜੋ(");");

        Ast_ਗੰਢ* newp = cstmtp;
        ਅਚਲ AstVarRef* ਅਚਲ vrefp = VN_CAST(eventp, VarRef);
        ਜੇ (nodep->isDelayed() && (m_cfuncp || !vrefp)) {
            // V3Timing converts these with --timing
            nodep->v3warn(E_NOTIMING, "Nonblocking event trigger "
                                          << (m_cfuncp ? "in a non-inlined function/task"
                                                       : "of a class or interface member or "
                                                         "array element")
                                          << " requires --timing");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isDelayed()) {
            ਅਚਲ std::string newvarname = "_" + vrefp->varp()->shortName();
            AstVarScope* ਅਚਲ dlyvscp
                = m_dlyTmps.make(flp, vrefp->varScopep()->scopep(), 1, newvarname);

            ਅਚਲ ਸਵੈਚਾਲਿਤ dlyRef = [=](VAccess access) {  //
                ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, dlyvscp, access};
            };

            AstAlwaysPost* ਅਚਲ postp = ਨਵਾਂ AstAlwaysPost{flp};
            AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, dlyRef(VAccess::READ)};
            postp->ਜੋੜੋ_Stmtsp(ifp);

            UASSERT_OBJ(m_activep, nodep, "No active to handle FireEvent");
            Ast_Sen_ਰੁੱਖ* senTreep = m_activep->sentreep();
            Ast_ਸਰਗਰਮ* activep = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (m_ਵਿੱਚ_Suspendable_Or_Fork && (senTreep->hasInitial() || senTreep->hasClocked())) {
                // Suspendable code can trigger whenever it resumes, so fire when its NBAs are
                // committed (see visit(AstNodeProcedure*)), clearing the flag like
                // Scheme::FlagUnique
                senTreep = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{
                    flp, senTreep->hasClocked() ? senTreep->sensesp()->clone_ਰੁੱਖ(ਸੱਚ) : ਖਾਲੀ_ਸੂਚਕ};
                m_nbaEventSenTreeps.ਧੱਕੋ_ਪਿੱਠ(senTreep);
                m_needsInitialTrigger |= m_timingDomains.ਖਾਲੀ_ਥਾਵਾਂ();
                dlyvscp->varp()->ਸੈੱਟ_Ignore_Post_Write();
                ifp->ਜੋੜੋ_Thensp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, dlyRef(VAccess::WRITE),
                                             ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}});
                activep = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "nba-event", senTreep};
                activep->sen_ਰੁੱਖ_Storep(senTreep);
            } ਨਹੀਂਤਾਂ {
                activep = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "nba-event", senTreep};
                AstAlwaysPre* ਅਚਲ prep = ਨਵਾਂ AstAlwaysPre{flp};
                prep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, dlyRef(VAccess::WRITE),
                                              ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}});
                activep->ਜੋੜੋ_Stmtsp(prep);
            }
            ifp->ਜੋੜੋ_Thensp(newp);
            m_activep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(activep);
            activep->ਜੋੜੋ_Stmtsp(postp);

            newp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, dlyRef(VAccess::WRITE),
                                 ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Dly* nodep) override {
        // Prevent double processing due to AstExprStmt being moved before this node
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;

        ਜੇ (m_cfuncp) {
            ਜੇ (!v3Global.rootp()->nbaEventp()) {
                nodep->v3warn(
                    E_NOTIMING,
                    "Delayed assignment in a non-inlined function/task requires --timing");
            }
            ਵਾਪਸ;
        }
        UASSERT_OBJ(m_procp, nodep, "Delayed assignment not under process");
        UASSERT_OBJ(m_activep, nodep, "<= not under sensitivity block");
        UASSERT_OBJ(m_scopep, nodep, "<= not under scope");
        UASSERT_OBJ(m_ਵਿੱਚ_Suspendable_Or_Fork || m_activep->hasClocked(), nodep,
                    "<= assignment in non-clocked block, should have been converted in V3Active");

        m_needsInitialTrigger |= m_timingDomains.ਖਾਲੀ_ਥਾਵਾਂ();

        // Record scope of this NBA
        nodep->user2p(m_scopep);

        // Grab the reference to the target of the NBA, also lift ExprStmt statements on the LHS
        VL_RESTORER(m_currNbaLhsRefp);
        UASSERT_OBJ(!m_currNbaLhsRefp, nodep, "NBAs should not nest");
        nodep->lhsp()->foreach([&](Ast_ਗੰਢ* currp) {
            // cppcheck-suppress constVariablePointer
            ਜੇ (Ast_ਵਿਅੰਜਕ_Stmt* ਅਚਲ exprp = VN_CAST(currp, ਵਿਅੰਜਕ_Stmt)) {
                // Move statements before the NBA
                nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(exprp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                // Replace with result
                currp->replaceWith(exprp->resultp()->unlink_Fr_ਪਿੱਠ());
                // Get rid of the AstExprStmt
                VL_DO_DANGLING2(ਧੱਕੋ_Deletep(currp), currp, exprp);
            } ਨਹੀਂਤਾਂ ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(currp, VarRef)) {
                // Ignore reads (e.g.: '_[*here*] <= _')
                ਜੇ (refp->access().isReadOnly()) ਵਾਪਸ;
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
        AstVarScope* ਅਚਲ vscp = m_currNbaLhsRefp->varScopep();
        // Record it on first encounter
        VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
        ਜੇ (!vscpInfo.m_ਪਹਿਲਾ_Nba_Refp) {
            vscpInfo.m_ਪਹਿਲਾ_Nba_Refp = m_currNbaLhsRefp;
            vscpInfo.m_fistActivep = m_activep;
            m_vscps.emplace_ਪਿੱਠ(vscp);
        }
        // Note usage context
        vscpInfo.m_ਸਾਰਾ |= VN_IS(nodep->lhsp(), VarRef);
        vscpInfo.m_partial |= VN_IS(nodep->lhsp(), Sel);
        vscpInfo.m_ਵਿੱਚ_Loop |= m_ਵਿੱਚ_Loop;
        vscpInfo.m_ਵਿੱਚ_Susp_Or_Fork |= m_ਵਿੱਚ_Suspendable_Or_Fork;
        // Sensitivity might be non-clocked, in a suspendable process, which are handled elsewhere
        ਜੇ (m_activep->sentreep()->hasClocked()) {
            ਜੇ (vscpInfo.m_fistActivep != m_activep) {
                AstVar* ਅਚਲ varp = vscp->varp();
                ਜੇ (!varp->user1_ਸੈੱਟ_Once()
                    && !varp->fileline()->warnIsOff(V3_Error_ਕੋਡ::MULTIDRIVEN)) {
                    varp->v3warn(MULTIDRIVEN,
                                 "Signal has multiple driving blocks with different clocking: "
                                     << varp->prettyNameQ() << '\n'
                                     << vscpInfo.m_ਪਹਿਲਾ_Nba_Refp->warnOther()
                                     << "... Location of first driving block\n"
                                     << vscpInfo.m_ਪਹਿਲਾ_Nba_Refp->warnContextSecondary()
                                     << m_currNbaLhsRefp->warnOther()
                                     << "... Location of other driving block\n"
                                     << m_currNbaLhsRefp->warnContextPrimary() << '\n');
                }
            }
            // Add this sensitivity to the variable
            vscpInfo.ਜੋੜੋ_ਸੰਵੇਦਨਸ਼ੀਲਤਾ(m_activep->sentreep());
        }

        // Record the NBA for later processing
        m_nbas.emplace_ਪਿੱਠ();
        NBA& nba = m_nbas.ਪਿੱਠ();
        nba.nodep = nodep;
        nba.vscp = vscp;

        // Record write reference
        ਰਿਕਾਰਡ_Write_Ref(m_currNbaLhsRefp, ਸੱਚ);

        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        // Already checked the NBA LHS ref, ignore here
        ਜੇ (nodep == m_currNbaLhsRefp) ਵਾਪਸ;
        // Only care about write refs
        ਜੇ (!nodep->access().isWriteOrRW()) ਵਾਪਸ;
        // Record write reference
        ਰਿਕਾਰਡ_Write_Ref(nodep, ਝੂਠ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Loop);
        m_ਵਿੱਚ_Loop = ਸੱਚ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* ਅਚਲ nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ਅਚਲ ਸਵੈਚਾਲਿਤ& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        ਜੇ (cfuncps.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਦੇਖੋ_Called_CFunc(nodep->funcp());
        } ਨਹੀਂਤਾਂ {
            ਲਈ (AstCFunc* ਅਚਲ cfuncp : cfuncps) ਦੇਖੋ_Called_CFunc(cfuncp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* ਅਚਲ nodep) override {
        ਅਚਲ ਸਵੈਚਾਲਿਤ& ਮੁੱਲ = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        ਜੇ (ਮੁੱਲ.m_state != CFuncCache::UNINITIALIZED) ਵਾਪਸ;
        VL_RESTORER(m_cfuncp);
        // Its timing domains are those of the processes calling it, see visitCalledCFunc()
        VL_RESTORER_CLEAR(m_timingDomains);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }

    // Pre/Post logic are created here and their content need no further changes, so ignore.
    ਖਾਲੀ ਦੇਖੋ(AstAlwaysPre*) override {}
    ਖਾਲੀ ਦੇਖੋ(AstAlwaysPost*) override {}

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ DelayedVisitor(AstNetlist* nodep)
        : m_classGraphp{V3_Class_ਗ੍ਰਾਫ::build(nodep)} {
        iterate(nodep);
    }
    ~DelayedVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("NBA, variables using ShadowVar scheme", m_nSchemeShadowVar);
        V3Stats::ਜੋੜੋ_Stat("NBA, variables using ShadowVarMasked scheme", m_nSchemeShadowVarMasked);
        V3Stats::ਜੋੜੋ_Stat("NBA, variables using FlagShared scheme", m_nSchemeFlagShared);
        V3Stats::ਜੋੜੋ_Stat("NBA, variables using FlagUnique scheme", m_nSchemeFlagUnique);
        V3Stats::ਜੋੜੋ_Stat("NBA, variables using ValueQueueWhole scheme", m_n_Scheme_ਮੁੱਲ_Queues_ਸਾਰਾ);
        V3Stats::ਜੋੜੋ_Stat("NBA, variables using ValueQueuePartial scheme",
                         m_n_Scheme_ਮੁੱਲ_Queues_Partial);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, NBA flags shared", m_n_Shared_ਸੈੱਟ_Flags);
        V3Stats::ਜੋੜੋ_Stat("Procedures needing initial NBA trigger", m_nInitialNBA);
        V3Stats::ਜੋੜੋ_Stat("Non-inlined co_awaits with SenTree", m_non_Inlined_CAwaits_With_Sen_ਰੁੱਖ);
    }
};

//######################################################################
// Delayed class functions

ਖਾਲੀ V3Delayed::delayedAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DelayedVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("delayed", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
