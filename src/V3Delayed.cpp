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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Delayed.h"

#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3ClassGraph.h"
#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3LinkLValue.h"
#અન્તર્ભાવ "V3SharedTmps.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <deque>
#અન્તર્ભાવ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert AstAssignDlys (NBAs)

વર્ગ DelayedVisitor final : જાહેર VNVisitor {
    // TYPES

    // The various NBA conversion schemes, including error cases
    ગણના વર્ગ Scheme : uint8_t {
        Undecided = 0,
        UnsupportedCompoundArrayInLoop,
        ShadowVar,
        ShadowVarMasked,
        FlagShared,
        FlagUnique,
        કિંમત_Queue_Whole,
        કિંમત_Queue_Partial
    };

    // All info associated with a variable that is the target of an NBA
    વર્ગ VarScopeInfo final {
    જાહેર:
        // First write reference encountered to the VarScope as the target on an NBA
        અચલ AstVarRef* m_firstNbaRefp = nullptr;
        // Active block 'm_firstNbaRefp' is under
        અચલ AstActive* m_fistActivep = nullptr;
        તાર્કિક m_whole = ખોટું;  // Used on LHS of NBA directly via VarRef
        તાર્કિક m_partial = ખોટું;  // Used on LHS of NBA under a Sel
        તાર્કિક m_inLoop = ખોટું;  // Used on LHS of NBA in a loop
        તાર્કિક m_inSuspOrFork = ખોટું;  // Used on LHS of NBA in suspendable process or fork
        Scheme m_scheme = Scheme::Undecided;  // Conversion scheme to use for this variable

    ખાનગી:
        // Combined sensitivities of all NBAs targeting this variable
        Ast_Sen_વૃક્ષ* m_senTreep = nullptr;

        // Union of stuff needed for the various schemes - use accessors below!
        સંઘ {
            સંરચના {  // Stuff needed for Scheme::ShadowVar
                AstVarScope* vscp;  // The shadow variable
            } m_shadowVariableKit;
            સંરચના {  // Stuff needed for Scheme::ShadowVarMasked
                AstVarScope* vscp;  // The shadow variable
                AstVarScope* maskp;  // The mask variable
            } m_shadowVarMaskedKit;
            સંરચના {  // Stuff needed for Scheme::FlagShared
                AstActive* activep;  // The active block for the Pre/Post logic
                AstAlwaysPost* postp;  // The post block for commiting results
                AstVarScope* commitFlagp;  // The commit flag variable, for reuse
                AstIf* commitIfp;  // The previous if statement for committing, for reuse
            } m_flagSharedKit;
            સંરચના {  // Stuff needed for Scheme::FlagUnique
                AstAlwaysPost* postp;  // The post block for commiting results
            } m_flagUniqueKit;
            સંરચના {  // Stuff needed for Scheme::ValueQueueWhole/Scheme::ValueQueuePartial
                AstVarScope* vscp;  // The commit queue variable
            } m_કિંમત_Queue_Kit;
        } m_kitUnion;

    જાહેર:
        VarScopeInfo() = મૂળભૂત;
        ~VarScopeInfo() {
            // Might not be linked if there was an error
            જો (!m_senTreep->backp()) VL_DO_DANGLING(m_senTreep->delete_વૃક્ષ(), m_senTreep);
        }
        // Accessors for the above union fields
        auto& shadowVariableKit() {
            UASSERT(m_scheme == Scheme::ShadowVar, "Inconsistent Scheme");
            પાછા m_kitUnion.m_shadowVariableKit;
        }
        auto& shadowVarMaskedKit() {
            UASSERT(m_scheme == Scheme::ShadowVarMasked, "Inconsistent Scheme");
            પાછા m_kitUnion.m_shadowVarMaskedKit;
        }
        auto& flagSharedKit() {
            UASSERT(m_scheme == Scheme::FlagShared, "Inconsistent Scheme");
            પાછા m_kitUnion.m_flagSharedKit;
        }
        auto& flagUniqueKit() {
            UASSERT(m_scheme == Scheme::FlagUnique, "Inconsistent Scheme");
            પાછા m_kitUnion.m_flagUniqueKit;
        }
        auto& કિંમત_Queue_Kit() {
            UASSERT(m_scheme == Scheme::કિંમત_Queue_Partial || m_scheme == Scheme::કિંમત_Queue_Whole,
                    "Inconsistent Scheme");
            પાછા m_kitUnion.m_કિંમત_Queue_Kit;
        }

        // Accessor
        Ast_Sen_વૃક્ષ* senTreep() અચલ { પાછા m_senTreep; }

        // Add sensitivities
        શૂન્ય addSensitivity(AstSenItem* nodep) {
            જો (!m_senTreep) m_senTreep = નવું Ast_Sen_વૃક્ષ{nodep->fileline(), nullptr};
            // Add a copy of each term
            m_senTreep->addSensesp(nodep->clone_વૃક્ષ(સાચું));
            // Remove duplicates
            V3Const::constifyExpensiveEdit(m_senTreep);
        }
        // cppcheck-suppress constParameterPointer
        શૂન્ય addSensitivity(Ast_Sen_વૃક્ષ* nodep) { addSensitivity(nodep->sensesp()); }
    };

    // Data structure to keep track of all writes to
    સંરચના WriteReference final {
        AstVarRef* m_refp = nullptr;  // The reference
        તાર્કિક m_isNBA = ખોટું;  // True if an NBA write
        તાર્કિક m_inNonComb = ખોટું;  // True if reference is known to be in non-combinational logic
        WriteReference() = મૂળભૂત;
        WriteReference(AstVarRef* refp, તાર્કિક isNBA, તાર્કિક inNonComb)
            : m_refp{refp}
            , m_isNBA{isNBA}
            , m_inNonComb{inNonComb} {}
    };

    // Data required to lower AstAssignDelay later
    સંરચના NBA final {
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
    અચલ VNUser1InUse m_user1InUse;
    અચલ VNUser2InUse m_user2InUse;
    અચલ VNUser3InUse m_user3InUse;

    સંરચના CFuncCache final {
        VInsertionSet<Ast_Sen_વૃક્ષ*> m_timingDomains;  // What shall be added to m_timingDomains
        std::set<AstCFunc*>
            m_includes;  // CFuncs whose CFuncCache shall be included into this - this is used to
                         // break cycles: A->B->A (instead of visiting A while it is still begin
                         // visited B just marks that it includes A)
        ગણના State : uint8_t {
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
    VInsertionSet<Ast_Sen_વૃક્ષ*> m_timingDomains;  // Timing resume domains
    V3SharedTmps m_dlyTmps{"__Vdly", VVarType::BLOCKTEMP};  // Temporary variables
    // Commit queue data types, by element type and partial flag. Shared by all instances,
    // as m_dlyTmps only shares variables with the same data type.
    std::map<std::pair<અચલ Ast_ગાંઠ_DType*, તાર્કિક>, AstNBACommitQueueDType*> m_cqDTypeps;

    અચલ std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::vector<અચલ AstCFunc*> m_call_ઢગલો;  // Current callstack of AstCFuncs

    // STATE - for current visit position (use VL_RESTORER)
    AstActive* m_activep = nullptr;  // Current activate
    અચલ AstCFunc* m_cfuncp = nullptr;  // Current public C Function
    Ast_ગાંઠ_Procedure* m_procp = nullptr;  // Current process
    AstScope* m_scopep = nullptr;  // Current scope
    તાર્કિક m_inLoop = ખોટું;  // True in for loops
    તાર્કિક m_inSuspendableOrFork = ખોટું;  // True in suspendable processes and forks
    તાર્કિક m_ignoreBlkAndNBlk = ખોટું;  // Suppress delayed assignment BLKANDNBLK
    તાર્કિક m_inNonCombLogic = ખોટું;  // We are in non-combinational logic
    તાર્કિક m_needsInitialTrigger = ખોટું;  // Whether a NodeProcedure needs a initial trigger
    std::vector<Ast_Sen_વૃક્ષ*> m_nbaEventSenTreeps;  // Sensitivities of '->>' in the process
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
    VDouble0 m_n_Scheme_કિંમત_Queues_Whole;  //  Number of variables using Scheme::ValueQueueWhole
    VDouble0 m_n_Scheme_કિંમત_Queues_Partial;  //  Number of variables using Scheme::ValueQueuePartial
    VDouble0 m_nSharedSetFlags;  // "Set" flags actually shared by Scheme::FlagShared variables
    VDouble0 m_nInitialNBA;  // Number of procedural blocks with initial NBA
    VDouble0 m_non_Inlined_CAwaits_With_Sen_વૃક્ષ;  // Count uses of not inlined co_awaits

    // METHODS

    // Return true iff a variable is assigned by both blocking and nonblocking
    // assignments. Issue BLKANDNBLK error if we can't prove the mixed
    // assignments are to independent bits and the blocking assignment can be
    // in combinational logic, which is something we can't safely implement
    // still.
    તાર્કિક checkMixedUsage(અચલ AstVarScope* vscp, તાર્કિક isIntegralOrPacked) {

        સંરચના Ref final {
            AstVarRef* m_refp;  // The reference
            તાર્કિક m_inNonComb;  // True if known to be in non-combinational logic
            પૂર્ણાંક m_lsb;  // LSB of accessed range
            પૂર્ણાંક m_msb;  // MSB of accessed range
            Ref(AstVarRef* refp, તાર્કિક inNonComb, પૂર્ણાંક lsb, પૂર્ણાંક msb)
                : m_refp{refp}
                , m_inNonComb{inNonComb}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };

        std::vector<Ref> blkRefs;  // Blocking writes
        std::vector<Ref> nbaRefs;  // Non-blockign writes

        અચલ પૂર્ણાંક width = isIntegralOrPacked ? vscp->width() : 1;

        માટે (અચલ auto& writeRef : m_writeRefs(vscp)) {
            પૂર્ણાંક lsb = 0;
            પૂર્ણાંક msb = width - 1;
            જો (અચલ AstSel* અચલ selp = VN_CAST(writeRef.m_refp->backp(), Sel)) {
                જો (VN_IS(selp->lsbp(), Const)) {
                    lsb = selp->lsbConst();
                    msb = selp->msbConst();
                }
            }
            જો (writeRef.m_isNBA) {
                nbaRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            } નહીંતર {
                blkRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            }
        }
        // We only run this function on targets of NBAs, so there should be at least one...
        UASSERT_OBJ(!nbaRefs.empty(), vscp, "Did not record NBA write");
        // If no blocking upadte, then we are good
        જો (blkRefs.empty()) પાછા ખોટું;

        // If the blocking assignment is in non-combinational logic (i.e.:
        // in logic that has an explicit trigger), then we can safely
        // implement it (there is no race between clocked logic and post
        // scheduled logic), so need not error
        blkRefs.erase(std::remove_if(blkRefs.begin(), blkRefs.end(),
                                     [](અચલ Ref& ref) { પાછા ref.m_inNonComb; }),
                      blkRefs.end());

        // If nothing left, then we need not error
        જો (blkRefs.empty()) પાછા સાચું;

        // If not a packed variable, warn here as we can't prove independence
        જો (!isIntegralOrPacked) {
            અચલ Ref& blkRef = blkRefs.front();
            અચલ Ref& nbaRef = nbaRefs.front();
            vscp->v3warn(
                BLKANDNBLK,
                "Unsupported: Blocking and non-blocking assignments to same non-packed variable: "
                    << vscp->varp()->prettyNameQ() << '\n'
                    << vscp->warnContextPrimary() << '\n'
                    << blkRef.m_refp->warnOther() << "... Location of blocking assignment\n"
                    << blkRef.m_refp->warnContextSecondary() << '\n'
                    << nbaRef.m_refp->warnOther() << "... Location of nonblocking assignment\n"
                    << nbaRef.m_refp->warnContextSecondary());
            પાછા સાચું;
        }

        // We need to error if we can't prove the written bits are independent

        // Sort refs by interval
        અચલ auto lessThanRef = [](અચલ Ref& a, અચલ Ref& b) {
            જો (a.m_lsb != b.m_lsb) પાછા a.m_lsb < b.m_lsb;
            પાછા a.m_msb < b.m_msb;
        };
        std::stable_sort(blkRefs.begin(), blkRefs.end(), lessThanRef);
        std::stable_sort(nbaRefs.begin(), nbaRefs.end(), lessThanRef);
        // Iterate both vectors, checking for overlap
        auto bIt = blkRefs.begin();
        auto nIt = nbaRefs.begin();
        જ્યારે (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            જો (lessThanRef(*bIt, *nIt)) {
                જો (nIt->m_lsb <= bIt->m_msb) તોડો;  // Stop on Overlap
                ++bIt;
            } નહીંતર {
                જો (bIt->m_lsb <= nIt->m_msb) તોડો;  // Stop on Overlap
                ++nIt;
            }
        }

        // If we found an overlapping range that cannot be safely implemented, then wran...
        જો (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            અચલ Ref& blkRef = *bIt;
            અચલ Ref& nbaRef = *nIt;
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

        પાછા સાચું;
    }

    // Choose the NBA scheme used for the given variable.
    Scheme chooseScheme(અચલ AstVarScope* vscp, અચલ VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::Undecided, vscp, "NBA scheme already decided");

        અચલ Ast_ગાંઠ_DType* અચલ dtypep = vscp->dtypep()->skipRefp();
        // Unpacked arrays
        જો (અચલ AstUnpackArrayDType* અચલ uaDTypep = VN_CAST(dtypep, UnpackArrayDType)) {
            // If whole array is target of NBA, use ShadowVar, but not in a suspendable process or
            // fork, which resume outside the 'nba' region, so its 'pre' logic would overwrite the
            // value they write in the shadow variable
            જો (vscpInfo.m_whole) {
                પાછા vscpInfo.m_inSuspOrFork ? Scheme::FlagUnique : Scheme::ShadowVar;
            }
            // Basic underlying type of elements, if any.
            અચલ AstBasicDType* અચલ basicp = uaDTypep->basicp();
            // If used in a loop, we must have a dynamic commit queue. (Also works in suspendables)
            જો (vscpInfo.m_inLoop) {
                // Arrays with compound element types are currently not supported in loops
                જો (!basicp
                    || !(basicp->isIntegralOrPacked()  //
                         || basicp->isDouble()  //
                         || basicp->isString())) {
                    પાછા Scheme::UnsupportedCompoundArrayInLoop;
                }
                જો (vscpInfo.m_partial) પાછા Scheme::કિંમત_Queue_Partial;
                પાછા Scheme::કિંમત_Queue_Whole;
            }
            // A suspendable process or fork can resume outside the 'nba' region, so the 'pre'
            // logic of the other schemes would undo its updates. The unique flag scheme has none.
            જો (vscpInfo.m_inSuspOrFork) પાછા Scheme::FlagUnique;
            // Otherwise if an array of packed/basic elements, use the shared flag scheme
            જો (basicp) પાછા Scheme::FlagShared;
            // Finally fall back on the shadow variable scheme, e.g. for
            // arrays of unpacked structs. This will be slow.
            // TODO: generic LHS scheme as discussed in #5092
            પાછા Scheme::ShadowVar;
        }

        // Likewise, the 'pre' logic of the shadow variable scheme would undo the updates of a
        // suspendable process or fork, so use the unique flag scheme
        જો (vscpInfo.m_inSuspOrFork) પાછા Scheme::FlagUnique;

        અચલ તાર્કિક isIntegralOrPacked = dtypep->isIntegralOrPacked();
        // Check for mixed usage (this also warns if not OK)
        જો (checkMixedUsage(vscp, isIntegralOrPacked)) {
            // If it's a variable updated by both blocking and non-blocking
            // assignments, use the ShadowVarMasked schem if masked update is
            // possible. This can handle blocking and non-blocking updates to
            // inpdendent parts correctly at run-time, and always works, even
            // in loops or other dynamic context.
            જો (isIntegralOrPacked) પાછા Scheme::ShadowVarMasked;
            // If it's inside a loop, use Scheme::ShadowVar, which is safe,
            // but will generate incorrect code if a partial update is used
            જો (vscpInfo.m_inLoop) પાછા Scheme::ShadowVar;
            // Otherwise (for not packed variables), use the FlagUnique scheme,
            // which at least handles partial updates correctly, but might break
            // in loops or other dynamic context
            પાછા Scheme::FlagUnique;
        }

        // Otherwise use the simple shadow variable scheme
        પાછા Scheme::ShadowVar;
    }

    // Given an expression 'exprp', return a new expression that always evaluates to the
    // value of the given expression at this point in the program. That is:
    // - If given a non-constant expression, create a new temporary AstVarScope under the given
    //   'scopep', with the given 'name', assign the expression to it, and return a read reference
    //   to the new AstVarScope.
    // - If given a constant, just return that constant.
    // New statements are inserted before 'insertp'
    Ast_ગાંઠ_પદાવલિ* captureVal(AstScope* અચલ scopep, Ast_ગાંઠ_Stmt* અચલ insertp,
                            Ast_ગાંઠ_પદાવલિ* અચલ exprp, અચલ std::string& name) {
        UASSERT_OBJ(!exprp->backp(), exprp, "Should have been unlinked");
        File_લીટી* અચલ flp = exprp->fileline();
        જો (VN_IS(exprp, Const)) પાછા exprp;
        // TODO: there are some const variables that could be simply referenced here
        AstVarScope* અચલ tmpVscp = m_dlyTmps.make(flp, scopep, exprp->dtypep(), name);
        insertp->addHereThisAsNext(
            નવું AstAssign{flp, નવું AstVarRef{flp, tmpVscp, VAccess::WRITE}, exprp});
        પાછા નવું AstVarRef{flp, tmpVscp, VAccess::READ};
    };

    // Similar to 'captureVal', but captures an LValue expression. That is, the returned
    // expression will reference the same location as the input expression, at this point in the
    // program.
    Ast_ગાંઠ_પદાવલિ* captureLhs(AstScope* અચલ scopep, Ast_ગાંઠ_Stmt* અચલ insertp,
                            Ast_ગાંઠ_પદાવલિ* અચલ lhsp, અચલ std::string& baseName) {
        UASSERT_OBJ(!lhsp->backp(), lhsp, "Should have been unlinked");
        // Running node pointer
        Ast_ગાંઠ_પદાવલિ* nodep = lhsp;
        // Capture AstSel indices - there should be only one
        જો (AstSel* અચલ selp = VN_CAST(nodep, Sel)) {
            અચલ std::string tmpName{"Lsb" + baseName};
            selp->lsbp(captureVal(scopep, insertp, selp->lsbp()->unlinkFrBack(), tmpName));
            // Continue with target
            nodep = selp->fromp();
        }
        UASSERT_OBJ(!VN_IS(nodep, Sel), lhsp, "Multiple 'AstSel' applied to LHS reference");
        // Capture the indices of selects - might be many, also below struct members. If a handle
        // selects the instance the target is a member of, capture the handle instead, as it is
        // only read, to select the target when the NBA is executed (IEEE 1800-2023 10.4.2).
        size_t nArraySels = 0;
        જ્યારે (સાચું) {
            જો (Ast_ગાંઠ_Sel* અચલ selp = VN_CAST(nodep, ગાંઠ_Sel)) {
                અચલ std::string tmpName{"Dim" + std::to_string(nArraySels++) + baseName};
                selp->bitp(captureVal(scopep, insertp, selp->bitp()->unlinkFrBack(), tmpName));
                nodep = selp->fromp();
            } નહીંતર જો (અચલ AstStructSel* અચલ selp = VN_CAST(nodep, StructSel)) {
                nodep = selp->fromp();
            } નહીંતર જો (AstMemberSel* અચલ selp = VN_CAST(nodep, MemberSel)) {
                Ast_ગાંઠ_પદાવલિ* અચલ handlep = selp->fromp()->unlinkFrBack();
                V3LinkLValue::linkLValueSet(handlep, VAccess::READ);
                AstVarRef* અચલ refp
                    = VN_AS(captureVal(scopep, insertp, handlep, "Handle" + baseName), VarRef);
                refp->varp()->setIgnorePostWrite();
                // Like the handle it replaces, mark it written, as is the member it selects
                refp->access(selp->access());
                selp->fromp(refp);
                પાછા lhsp;
            } નહીંતર {
                તોડો;
            }
        }
        // What remains must be the variable, or some sort of select, we assume can reuse it
        UASSERT_OBJ(nodep->isPure(), lhsp, "Malformed LHS in NBA");
        // Now have been converted to use the captured values
        પાછા lhsp;
    }

    શૂન્ય add_CFunc_Cached_કિંમતો(અચલ AstCFunc* અચલ cfuncp,
                              std::unordered_set<અચલ AstCFunc*>& visited) {
        જો (!visited.ઉમેરો(cfuncp).second) પાછા;
        CFuncCache& કિંમત = m_cfuncsCache(cfuncp);
        m_timingDomains.ઉમેરો(કિંમત.m_timingDomains.begin(), કિંમત.m_timingDomains.end());
        m_non_Inlined_CAwaits_With_Sen_વૃક્ષ += કિંમત.m_timingDomains.size();
        માટે (અચલ AstCFunc* અચલ includedp : કિંમત.m_includes) {
            add_CFunc_Cached_કિંમતો(includedp, visited);
        }
    }

    // Create a temporary variable in the given 'scopep', with the given 'name', and with 'dtypep'
    // type, with the bits selected by 'sLsbp'/'sWidthp' set to 'valuep', other bits set to zero.
    // Insert new statements before 'insertp'.
    // Returns a read reference to the temporary variable.
    AstVarRef* createWidened(File_લીટી* flp, AstScope* scopep, Ast_ગાંઠ_DType* dtypep,
                             Ast_ગાંઠ_પદાવલિ* sLsbp, પૂર્ણાંક sWidth, અચલ std::string& name,
                             Ast_ગાંઠ_પદાવલિ* valuep, Ast_ગાંઠ* insertp) {
        // Create temporary variable.
        AstVarScope* અચલ tp = m_dlyTmps.make(flp, scopep, dtypep, name);
        // Zero it
        AstConst* અચલ zerop = નવું AstConst{flp, AstConst::DTyped{}, dtypep};
        zerop->અંક().setAllBits0();
        insertp->addHereThisAsNext(
            નવું AstAssign{flp, નવું AstVarRef{flp, tp, VAccess::WRITE}, zerop});
        // Set the selected bits to 'valuep'
        AstSel* અચલ selp = નવું AstSel{flp, નવું AstVarRef{flp, tp, VAccess::WRITE},
                                        sLsbp->clone_વૃક્ષ_Pure(સાચું), sWidth};
        insertp->addHereThisAsNext(નવું AstAssign{flp, selp, valuep});
        // This is the expression to get the value of the temporary
        પાછા નવું AstVarRef{flp, tp, VAccess::READ};
    }

    // Scheme::ShadowVar
    શૂન્ય prepareSchemeShadowVar(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        File_લીટી* અચલ flp = vscp->fileline();
        AstScope* અચલ scopep = vscp->scopep();
        // Create the shadow variable
        અચલ std::string name = "_" + vscp->varp()->shortName();
        AstVarScope* અચલ shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), name);
        vscpInfo.shadowVariableKit().vscp = shadowVscp;
        // Mark both for V3LifePsot
        vscp->optimizeLifePost(સાચું);
        shadowVscp->optimizeLifePost(સાચું);
        // Create the AstActive for the Pre/Post logic
        AstActive* અચલ activep = નવું AstActive{flp, "nba-shadow-variable", vscpInfo.senTreep()};
        activep->sen_વૃક્ષ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Pre' scheduled 'shadowVariable = originalVariable' assignment
        AstAlwaysPre* અચલ prep = નવું AstAlwaysPre{flp};
        activep->addStmtsp(prep);
        prep->addStmtsp(નવું AstAssign{flp, નવું AstVarRef{flp, shadowVscp, VAccess::WRITE},
                                      નવું AstVarRef{flp, vscp, VAccess::READ}});
        // Add 'Post' scheduled 'originalVariable = shadowVariable' assignment
        AstAlwaysPost* અચલ postp = નવું AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        postp->addStmtsp(નવું AstAssign{flp, નવું AstVarRef{flp, vscp, VAccess::WRITE},
                                       નવું AstVarRef{flp, shadowVscp, VAccess::READ}});
    }
    શૂન્ય convertSchemeShadowVar(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        AstVarScope* અચલ shadowVscp = vscpInfo.shadowVariableKit().vscp;

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* અચલ refp) {
            જો (!refp->access().isWriteOnly()) પાછા;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
    }

    // Scheme::ShadowVarMasked
    શૂન્ય prepareSchemeShadowVarMasked(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        File_લીટી* અચલ flp = vscp->fileline();
        AstScope* અચલ scopep = vscp->scopep();
        // Create the shadow variable
        અચલ std::string shadowName = "_" + vscp->varp()->shortName();
        AstVarScope* અચલ shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), shadowName);
        vscpInfo.shadowVarMaskedKit().vscp = shadowVscp;
        // Create the makk variable
        અચલ std::string maskName = "Mask__" + vscp->varp()->shortName();
        AstVarScope* અચલ maskVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), maskName);
        maskVscp->varp()->setIgnorePostWrite();
        vscpInfo.shadowVarMaskedKit().maskp = maskVscp;
        // Create the AstActive for the Post logic
        AstActive* અચલ activep
            = નવું AstActive{flp, "nba-shadow-var-masked", vscpInfo.senTreep()};
        activep->sen_વૃક્ષ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit and mask clear
        AstAlwaysPost* અચલ postp = નવું AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit - vscp = (shadowVscp & maskVscp) | (vscp & ~maskVscp);
        postp->addStmtsp(નવું AstAssign{
            flp, નવું AstVarRef{flp, vscp, VAccess::WRITE},
            નવું AstOr{flp,
                      નવું AstAnd{flp, નવું AstVarRef{flp, shadowVscp, VAccess::READ},
                                 નવું AstVarRef{flp, maskVscp, VAccess::READ}},
                      નવું AstAnd{flp, નવું AstVarRef{flp, vscp, VAccess::READ},
                                 નવું AstNot{flp, નવું AstVarRef{flp, maskVscp, VAccess::READ}}}}});
        vscp->varp()->setIgnorePostRead();
        // Clar the mask - maskVscp = '0;
        postp->addStmtsp(
            નવું AstAssign{flp, નવું AstVarRef{flp, maskVscp, VAccess::WRITE},
                          નવું AstConst{flp, AstConst::Widthed_કિંમત{}, maskVscp->width(), 0}});
    }
    શૂન્ય convertSchemeShadowVarMasked(AstAssignDly* nodep, AstVarScope* vscp,
                                      VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        AstVarScope* અચલ shadowVscp = vscpInfo.shadowVarMaskedKit().vscp;
        AstVarScope* અચલ maskVscp = vscpInfo.shadowVarMaskedKit().maskp;

        Ast_ગાંઠ_પદાવલિ* lhsClonep = nodep->lhsp()->clone_વૃક્ષ(ખોટું);

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* અચલ refp) {
            જો (!refp->access().isWriteOnly()) પાછા;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
        // Set the same bits in the mask to 1
        lhsClonep->foreach([&](AstVarRef* અચલ refp) {
            જો (!refp->access().isWriteOnly()) પાછા;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(maskVscp);
            refp->varp(maskVscp->varp());
        });
        File_લીટી* અચલ flp = nodep->fileline();
        AstConst* અચલ onesp = નવું AstConst{flp, AstConst::DTyped{}, lhsClonep->dtypep()};
        onesp->અંક().setAllBits1();
        nodep->addNextHere(નવું AstAssign{flp, lhsClonep, onesp});
    }

    // Scheme::FlagShared
    શૂન્ય prepareSchemeFlagShared(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_લીટી* અચલ flp = vscp->fileline();
        AstScope* અચલ scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* અચલ activep = નવું AstActive{flp, "nba-flag-shared", vscpInfo.senTreep()};
        activep->sen_વૃક્ષ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        vscpInfo.flagSharedKit().activep = activep;
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* અચલ postp = નવું AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagSharedKit().postp = postp;
        // Initialize
        vscpInfo.flagSharedKit().commitFlagp = nullptr;
        vscpInfo.flagSharedKit().commitIfp = nullptr;
    }
    શૂન્ય convertSchemeFlagShared(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_લીટી* અચલ flp = vscp->fileline();
        AstScope* અચલ scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        અચલ std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_ગાંઠ_પદાવલિ* અચલ capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_ગાંઠ_પદાવલિ* અચલ capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Is this NBA adjacent after the previously processed NBA?
        અચલ તાર્કિક consecutive = nodep == m_nextDlyp;
        m_nextDlyp = VN_CAST(nodep->nextp(), AssignDly);

        VarScopeInfo* અચલ prevVscpInfop = consecutive ? &m_vscpInfo(m_prevVscp) : nullptr;

        // We can reuse the flag of the previous assignment if:
        અચલ તાર્કિક reuseTheFlag =
            // Consecutive NBAs
            consecutive
            // ... that use the same scheme
            && prevVscpInfop->m_scheme == Scheme::FlagShared
            // ... and are in the same scope as the target variable
            && scopep == vscp->scopep()
            // ... and share the same overall update domain
            && prevVscpInfop->senTreep()->same_વૃક્ષ(vscpInfo.senTreep());

        જો (!reuseTheFlag) {
            // Create new flag
            AstVarScope* અચલ flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
            // Set the flag at the original NBA
            nodep->addHereThisAsNext(  //
                નવું AstAssign{flp, નવું AstVarRef{flp, flagVscp, VAccess::WRITE},
                              નવું AstConst{flp, AstConst::BitTrue{}}});
            // Add the 'Pre' scheduled reset for the flag
            AstAlwaysPre* અચલ prep = નવું AstAlwaysPre{flp};
            vscpInfo.flagSharedKit().activep->addStmtsp(prep);
            prep->addStmtsp(નવું AstAssign{flp, નવું AstVarRef{flp, flagVscp, VAccess::WRITE},
                                          નવું AstConst{flp, AstConst::BitFalse{}}});
            // Add the 'Post' scheduled commit
            AstIf* અચલ ifp = નવું AstIf{flp, નવું AstVarRef{flp, flagVscp, VAccess::READ}};
            vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
            vscpInfo.flagSharedKit().commitFlagp = flagVscp;
            vscpInfo.flagSharedKit().commitIfp = ifp;
        } નહીંતર {
            જો (vscp != m_prevVscp) {
                // Different variable, ensure the commit block exists for this variable,
                // can reuse existing one with the same flag, otherwise create a new one.
                AstVarScope* અચલ flagVscp = prevVscpInfop->flagSharedKit().commitFlagp;
                UASSERT_OBJ(flagVscp, nodep, "Commit flag of previous assignment should exist");
                જો (vscpInfo.flagSharedKit().commitFlagp != flagVscp) {
                    AstIf* અચલ ifp = નવું AstIf{flp, નવું AstVarRef{flp, flagVscp, VAccess::READ}};
                    vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
                    vscpInfo.flagSharedKit().commitFlagp = flagVscp;
                    vscpInfo.flagSharedKit().commitIfp = ifp;
                }
            } નહીંતર {
                // Same variable, reuse the commit block of the previous assignment
                vscpInfo.flagSharedKit().commitFlagp = prevVscpInfop->flagSharedKit().commitFlagp;
                vscpInfo.flagSharedKit().commitIfp = prevVscpInfop->flagSharedKit().commitIfp;
            }
            ++m_nSharedSetFlags;
        }
        // Commit the captured value to the captured destination
        vscpInfo.flagSharedKit().commitIfp->addThensp(
            નવું AstAssign{flp, capturedLhsp, capturedRhsp});

        // Remember the variable for the next NBA
        m_prevVscp = vscp;

        // Delete original NBA
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::FlagUnique
    શૂન્ય prepareSchemeFlagUnique(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_લીટી* અચલ flp = vscp->fileline();
        AstScope* અચલ scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* અચલ activep = નવું AstActive{flp, "nba-flag-unique", vscpInfo.senTreep()};
        activep->sen_વૃક્ષ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* અચલ postp = નવું AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagUniqueKit().postp = postp;
    }
    શૂન્ય convertSchemeFlagUnique(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_લીટી* અચલ flp = vscp->fileline();
        AstScope* અચલ scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        અચલ std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_ગાંઠ_પદાવલિ* અચલ capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_ગાંઠ_પદાવલિ* અચલ capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Create new flag
        AstVarScope* અચલ flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
        flagVscp->varp()->setIgnorePostWrite();
        // Set the flag at the original NBA
        nodep->addHereThisAsNext(  //
            નવું AstAssign{flp, નવું AstVarRef{flp, flagVscp, VAccess::WRITE},
                          નવું AstConst{flp, AstConst::BitTrue{}}});
        // Add the 'Post' scheduled commit
        AstIf* અચલ ifp = નવું AstIf{flp, નવું AstVarRef{flp, flagVscp, VAccess::READ}};
        vscpInfo.flagUniqueKit().postp->addStmtsp(ifp);
        // Immediately clear the flag
        ifp->addThensp(નવું AstAssign{flp, નવું AstVarRef{flp, flagVscp, VAccess::WRITE},
                                     નવું AstConst{flp, AstConst::BitFalse{}}});
        // Commit the value
        ifp->addThensp(નવું AstAssign{flp, capturedLhsp, capturedRhsp});

        // Delete original NBA
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::ValueQueuePartial/Scheme::ValueQueueWhole
    template <તાર્કિક N_Partial>
    શૂન્ય prepare_Scheme_કિંમત_Queue(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(N_Partial ? vscpInfo.m_scheme == Scheme::કિંમત_Queue_Partial
                              : vscpInfo.m_scheme == Scheme::કિંમત_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_લીટી* અચલ flp = vscp->fileline();
        AstScope* અચલ scopep = vscp->scopep();

        // Create the commit queue variable
        Ast_ગાંઠ_DType* અચલ elemDTypep = vscp->dtypep()->skipRefp();
        AstNBACommitQueueDType*& cqDTypep = m_cqDTypeps[{elemDTypep, N_Partial}];
        જો (!cqDTypep) {
            cqDTypep = નવું AstNBACommitQueueDType{flp, elemDTypep, N_Partial};
            v3Global.rootp()->typeTablep()->addTypesp(cqDTypep);
        }
        અચલ std::string name = "CommitQueue__" + vscp->varp()->shortName();
        AstVarScope* અચલ queueVscp = m_dlyTmps.make(flp, scopep, cqDTypep, name);
        queueVscp->varp()->noReset(સાચું);
        queueVscp->varp()->setIgnorePostWrite();
        vscpInfo.કિંમત_Queue_Kit().vscp = queueVscp;
        // Create the AstActive for the Post logic
        AstActive* અચલ activep
            = નવું AstActive{flp, "nba-value-queue-whole", vscpInfo.senTreep()};
        activep->sen_વૃક્ષ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit
        AstAlwaysPost* અચલ postp = નવું AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit
        AstCMethodHard* અચલ callp = નવું AstCMethodHard{
            flp, નવું AstVarRef{flp, queueVscp, VAccess::READWRITE}, VCMethod::NBA_COMMIT};
        callp->dtypeSetVoid();
        // TODO: this is a partial update, so must be READWRITE, but that breaks scheduling
        callp->addPinsp(નવું AstVarRef{flp, vscp, VAccess::WRITE});
        postp->addStmtsp(callp->makeStmt());
    }

    શૂન્ય convert_Scheme_કિંમત_Queue(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo,
                                 તાર્કિક partial) {
        UASSERT_OBJ(partial ? vscpInfo.m_scheme == Scheme::કિંમત_Queue_Partial
                            : vscpInfo.m_scheme == Scheme::કિંમત_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_લીટી* અચલ flp = vscp->fileline();
        AstScope* અચલ scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        અચલ std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_ગાંઠ_પદાવલિ* અચલ capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_ગાંઠ_પદાવલિ* અચલ capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // RHS value (can be widened/masked iff Partial)
        Ast_ગાંઠ_પદાવલિ* valuep = capturedRhsp;
        // RHS mask (iff Partial)
        Ast_ગાંઠ_પદાવલિ* maskp = nullptr;

        // Running node for LHS deconstruction
        Ast_ગાંઠ_પદાવલિ* lhsNodep = capturedLhsp;

        // If partial updates are required, construct the mask and the widened value
        જો (partial) {
            // Type of array element
            Ast_ગાંઠ_DType* અચલ eDTypep = [&]() -> Ast_ગાંઠ_DType* {
                Ast_ગાંઠ_DType* dtypep = vscp->dtypep()->skipRefp();
                જ્યારે (AstUnpackArrayDType* અચલ ap = VN_CAST(dtypep, UnpackArrayDType)) {
                    dtypep = ap->subDTypep()->skipRefp();
                }
                પાછા dtypep;
            }();

            જો (અચલ AstSel* અચલ lSelp = VN_CAST(lhsNodep, Sel)) {
                // This is a partial assignment.
                // Need to create a mask and widen the value to element size.
                lhsNodep = lSelp->fromp();
                Ast_ગાંઠ_પદાવલિ* અચલ sLsbp = lSelp->lsbp();
                અચલ પૂર્ણાંક sWidth = lSelp->widthConst();

                // Create mask value
                maskp = [&]() -> Ast_ગાંઠ_પદાવલિ* {
                    // Constant mask we can compute here
                    જો (અચલ AstConst* અચલ cLsbp = VN_CAST(sLsbp, Const)) {
                        AstConst* અચલ cp = નવું AstConst{flp, AstConst::DTyped{}, eDTypep};
                        cp->અંક().setMask(sWidth, cLsbp->toSInt());
                        પાછા cp;
                    }

                    // A non-constant mask we must compute at run-time.
                    AstConst* અચલ onesp = નવું AstConst{flp, AstConst::Widthed_કિંમત{}, sWidth, 0};
                    onesp->અંક().setAllBits1();
                    પાછા createWidened(flp, scopep, eDTypep, sLsbp, sWidth, "Mask" + baseName,
                                         onesp, nodep);
                }();

                // Adjust value to element size
                valuep = [&]() -> Ast_ગાંઠ_પદાવલિ* {
                    // Constant value with constant select we can compute here
                    જો (AstConst* અચલ cValuep = VN_CAST(valuep, Const)) {
                        જો (અચલ AstConst* અચલ cLsbp = VN_CAST(sLsbp, Const)) {
                            AstConst* અચલ cp = નવું AstConst{flp, AstConst::DTyped{}, eDTypep};
                            cp->અંક().setAllBits0();
                            cp->અંક().opSelInto(cValuep->અંક(), cLsbp->toSInt(), sWidth);
                            VL_DO_DANGLING(valuep->delete_વૃક્ષ(), valuep);
                            પાછા cp;
                        }
                    }

                    // A non-constant value we must adjust.
                    પાછા createWidened(flp, scopep, eDTypep, sLsbp, sWidth,  //
                                         "Elem" + baseName, valuep, nodep);
                }();
            } નહીંતર {
                // If this assignment is not partial, set mask to ones and we are done
                AstConst* અચલ ones = નવું AstConst{flp, AstConst::DTyped{}, eDTypep};
                ones->અંક().setAllBits1();
                maskp = ones;
            }
        }

        // Extract array indices
        std::vector<Ast_ગાંઠ_પદાવલિ*> idxps;
        {
            UASSERT_OBJ(VN_IS(lhsNodep, ArraySel), lhsNodep, "Unexpected LHS form");
            જ્યારે (AstArraySel* અચલ aSelp = VN_CAST(lhsNodep, ArraySel)) {
                idxps.emplace_back(aSelp->bitp()->unlinkFrBack());
                lhsNodep = aSelp->fromp();
            }
            UASSERT_OBJ(VN_IS(lhsNodep, VarRef), lhsNodep, "Unexpected LHS form");
            std::reverse(idxps.begin(), idxps.end());
        }

        // Done with the LHS at this point
        VL_DO_DANGLING(ધકેલો_Deletep(capturedLhsp), capturedLhsp);

        // Enqueue the update at the site of the original NBA
        AstCMethodHard* અચલ callp = નવું AstCMethodHard{
            flp, નવું AstVarRef{flp, vscpInfo.કિંમત_Queue_Kit().vscp, VAccess::READWRITE},
            VCMethod::NBA_ENQUEUE};
        callp->dtypeSetVoid();
        callp->addPinsp(valuep);
        જો (partial) callp->addPinsp(maskp);
        માટે (Ast_ગાંઠ_પદાવલિ* અચલ indexp : idxps) callp->addPinsp(indexp);
        nodep->addHereThisAsNext(callp->makeStmt());

        // Delete original NBA
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Record where a variable is assigned
    શૂન્ય recordWriteRef(AstVarRef* nodep, તાર્કિક nonBlocking) {
        // Ignore references in certain contexts
        જો (m_ignoreBlkAndNBlk) પાછા;
        // Ignore if it's an array
        // TODO: we do this because it used to be the previous behaviour.
        //       Is it still required, or should we warn for arrays as well?
        //       Scheduling is no different for them...
        //       Clarification: This is OK for arrays of primitive types, but
        //       arrays that use the ShadowVar scheme don't work...
        જો (VN_IS(nodep->varScopep()->dtypep()->skipRefp(), UnpackArrayDType)) પાછા;

        m_writeRefs(nodep->varScopep()).emplace_back(nodep, nonBlocking, m_inNonCombLogic);
    }

    template <typename Procedure_T>
    સ્થિર તાર્કિક isProcedureWithSentreep(અચલ Ast_ગાંઠ_Procedure* અચલ nodep) {
        અચલ Procedure_T* અચલ procedurep = Ast_ગાંઠ::cast<Procedure_T>(nodep);
        પાછા procedurep && procedurep->sentreep();
    }

    // Visit AstCFunc from a AstNodeCCall - this is made into a separate quasi-visitor because
    // AstCFunc that is not called from the code (e.g.: DPI exports) does not need to be visited
    // this way. Also, not visiting such AstCFuncs allows to avoid caching results for them which
    // this function does - which could lead to excessive memory usage
    શૂન્ય visitCalledCFunc(AstCFunc* અચલ nodep) {
        CFuncCache& કિંમત = m_cfuncsCache(nodep);
        પસંદગી (કિંમત.m_state) {
        સ્થિતિ CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_timingDomains);

            // Visit
            કિંમત.m_state = CFuncCache::VISITING;
            m_call_ઢગલો.ધકેલો_back(nodep);
            {
                VL_RESTORER(m_cfuncp);
                m_cfuncp = nodep;
                iterateChildren(nodep);
            }
            m_call_ઢગલો.કાઢો_back();
            કિંમત.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_timingDomains, કિંમત.m_timingDomains);
        } તોડો;
        સ્થિતિ CFuncCache::VISITING: {
            માટે (size_t i = m_call_ઢગલો.size() - 1; m_call_ઢગલો.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ઢગલો[i]).m_includes.ઉમેરો(nodep);
            }
            પાછા;  // Break recursion
        }
        સ્થિતિ CFuncCache::INITIALIZED: તોડો;
        }
        std::unordered_set<અચલ AstCFunc*> visited;
        add_CFunc_Cached_કિંમતો(nodep, visited);
    }

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override {
        iterateChildren(nodep);
        // Decide which scheme to use for each variable and do the 'prepare' step
        માટે (AstVarScope* અચલ vscp : m_vscps) {
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            vscpInfo.m_scheme = chooseScheme(vscp, vscpInfo);
            // Run 'prepare' step
            પસંદગી (vscpInfo.m_scheme) {
            સ્થિતિ Scheme::Undecided:  // LCOV_EXCL_START
                UASSERT_OBJ(ખોટું, vscp, "Failed to choose NBA scheme");
                તોડો;  // LCOV_EXCL_STOP
            સ્થિતિ Scheme::UnsupportedCompoundArrayInLoop: {
                // Will report error at the site of the NBA
                તોડો;
            }
            સ્થિતિ Scheme::ShadowVar: {
                ++m_nSchemeShadowVar;
                prepareSchemeShadowVar(vscp, vscpInfo);
                તોડો;
            }
            સ્થિતિ Scheme::ShadowVarMasked: {
                ++m_nSchemeShadowVarMasked;
                prepareSchemeShadowVarMasked(vscp, vscpInfo);
                તોડો;
            }
            સ્થિતિ Scheme::FlagShared: {
                ++m_nSchemeFlagShared;
                prepareSchemeFlagShared(vscp, vscpInfo);
                તોડો;
            }
            સ્થિતિ Scheme::FlagUnique: {
                ++m_nSchemeFlagUnique;
                prepareSchemeFlagUnique(vscp, vscpInfo);
                તોડો;
            }
            સ્થિતિ Scheme::કિંમત_Queue_Whole: {
                ++m_n_Scheme_કિંમત_Queues_Whole;
                prepare_Scheme_કિંમત_Queue</* Partial: */ ખોટું>(vscp, vscpInfo);
                તોડો;
            }
            સ્થિતિ Scheme::કિંમત_Queue_Partial: {
                ++m_n_Scheme_કિંમત_Queues_Partial;
                prepare_Scheme_કિંમત_Queue</* Partial: */ સાચું>(vscp, vscpInfo);
                તોડો;
            }
            }
        }
        // Convert all NBAs
        માટે (અચલ NBA& nba : m_nbas) {
            AstAssignDly* અચલ nbap = nba.nodep;
            AstVarScope* અચલ vscp = nba.vscp;
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            // Run 'convert' step
            પસંદગી (vscpInfo.m_scheme) {
            સ્થિતિ Scheme::Undecided: {  // LCOV_EXCL_START
                UASSERT_OBJ(ખોટું, vscp, "Unreachable");
                તોડો;
            }  // LCOV_EXCL_STOP
            સ્થિતિ Scheme::UnsupportedCompoundArrayInLoop: {
                // TODO: make this an E_UNSUPPORTED...
                nbap->v3warn(BLKLOOPINIT, "Unsupported: Non-blocking assignment to array with "
                                          "compound element type inside loop");
                તોડો;
            }
            સ્થિતિ Scheme::ShadowVar: {
                convertSchemeShadowVar(nbap, vscp, vscpInfo);
                તોડો;
            }
            સ્થિતિ Scheme::ShadowVarMasked: {
                convertSchemeShadowVarMasked(nbap, vscp, vscpInfo);
                તોડો;
            }
            સ્થિતિ Scheme::FlagShared: {
                convertSchemeFlagShared(nbap, vscp, vscpInfo);
                તોડો;
            }
            સ્થિતિ Scheme::FlagUnique: {
                convertSchemeFlagUnique(nbap, vscp, vscpInfo);
                તોડો;
            }
            સ્થિતિ Scheme::કિંમત_Queue_Whole: {
                convert_Scheme_કિંમત_Queue(nbap, vscp, vscpInfo, /* partial: */ ખોટું);
                તોડો;
            }
            સ્થિતિ Scheme::કિંમત_Queue_Partial:
                convert_Scheme_કિંમત_Queue(nbap, vscp, vscpInfo, /* partial: */ સાચું);
                તોડો;
            }
        }
    }
    શૂન્ય visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstActive* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ignoreBlkAndNBlk);
        VL_RESTORER(m_inNonCombLogic);
        m_activep = nodep;
        અચલ Ast_Sen_વૃક્ષ* અચલ senTreep = nodep->sentreep();
        m_ignoreBlkAndNBlk = senTreep->hasStatic() || senTreep->hasInitial();
        m_inNonCombLogic = senTreep->hasClocked();
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override {
        VL_RESTORER(m_needsInitialTrigger);
        VL_RESTORER_CLEAR(m_nbaEventSenTreeps);
        અચલ size_t firstNBAAddedIndex = m_nbas.size();
        {
            VL_RESTORER(m_inSuspendableOrFork);
            VL_RESTORER(m_procp);
            VL_RESTORER(m_ignoreBlkAndNBlk);
            VL_RESTORER(m_inNonCombLogic);
            // When we are dealing with initial block we need to
            // treat it as suspendable when we meet a NBA
            m_inSuspendableOrFork = nodep->isSuspendable() || VN_IS(nodep, Initial);
            m_procp = nodep;
            જો (nodep->isSuspendable()) {
                m_ignoreBlkAndNBlk = ખોટું;
                m_inNonCombLogic = સાચું;
            }
            iterateChildren(nodep);
        }
        auto containsClocled = [](અચલ AstSenItem* itemp) {
            જ્યારે (itemp) {
                જો (itemp->edgeType().clockedStmt()) પાછા સાચું;
                itemp = VN_AS(itemp->nextp(), SenItem);
            }
            પાછા ખોટું;
        };
        અચલ તાર્કિક addInitialTrigger = m_needsInitialTrigger
                                       && !(isProcedureWithSentreep<AstAlways>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysObserved>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysReactive>(nodep))
                                       && !containsClocled(m_activep->sentreep()->sensesp());
        જો (m_timingDomains.empty() && !addInitialTrigger) પાછા;

        // There were some timing domains involved in the process. Add all of them as sensitivities
        // of all NBA targets in this process. Note this is a bit of a sledgehammer, we should only
        // need those that directly precede the NBA in control flow, but that is hard to compute,
        // so we will hammer away.

        // First gather all senItems
        AstSenItem* senItemp = nullptr;
        જો (addInitialTrigger) {
            senItemp = નવું AstSenItem{nodep->fileline(), AstSenItem::InitialNBA{}};
            ++m_nInitialNBA;
        }

        માટે (અચલ Ast_Sen_વૃક્ષ* અચલ domainp : m_timingDomains) {
            જો (domainp->sensesp())
                senItemp = Ast_ગાંઠ::addNext(senItemp, domainp->sensesp()->clone_વૃક્ષ(સાચું));
        }
        m_timingDomains.clear();
        // Add them to all nba targets we gathered in this process
        માટે (size_t i = firstNBAAddedIndex; i < m_nbas.size(); ++i) {
            m_vscpInfo(m_nbas[i].vscp).addSensitivity(senItemp);
        }
        માટે (Ast_Sen_વૃક્ષ* અચલ senTreep : m_nbaEventSenTreeps)
            senTreep->addSensesp(senItemp->clone_વૃક્ષ(સાચું));
        // Done with these
        VL_DO_DANGLING(senItemp->delete_વૃક્ષ(), senItemp);
    }
    શૂન્ય visit(AstFork* nodep) override {
        VL_RESTORER(m_inSuspendableOrFork);
        m_inSuspendableOrFork = સાચું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCAwait* nodep) override {
        જો (nodep->sentreep()) m_timingDomains.ઉમેરો(nodep->sentreep());
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstFireEvent* nodep) override {
        UASSERT_OBJ(v3Global.hasEvents(), nodep, "Inconsistent");
        File_લીટી* અચલ flp = nodep->fileline();

        Ast_ગાંઠ_પદાવલિ* અચલ eventp = nodep->operandp()->unlinkFrBack();

        // Enqueue for clearing 'triggered' state on next eval
        AstCStmt* અચલ cstmtp = નવું AstCStmt{flp};
        cstmtp->add("vlSymsp->fireEvent(");
        cstmtp->add(eventp);
        cstmtp->add(");");

        Ast_ગાંઠ* newp = cstmtp;
        અચલ AstVarRef* અચલ vrefp = VN_CAST(eventp, VarRef);
        જો (nodep->isDelayed() && (m_cfuncp || !vrefp)) {
            // V3Timing converts these with --timing
            nodep->v3warn(E_NOTIMING, "Nonblocking event trigger "
                                          << (m_cfuncp ? "in a non-inlined function/task"
                                                       : "of a class or interface member or "
                                                         "array element")
                                          << " requires --timing");
        } નહીંતર જો (nodep->isDelayed()) {
            અચલ std::string newvarname = "_" + vrefp->varp()->shortName();
            AstVarScope* અચલ dlyvscp
                = m_dlyTmps.make(flp, vrefp->varScopep()->scopep(), 1, newvarname);

            અચલ auto dlyRef = [=](VAccess access) {  //
                પાછા નવું AstVarRef{flp, dlyvscp, access};
            };

            AstAlwaysPost* અચલ postp = નવું AstAlwaysPost{flp};
            AstIf* અચલ ifp = નવું AstIf{flp, dlyRef(VAccess::READ)};
            postp->addStmtsp(ifp);

            UASSERT_OBJ(m_activep, nodep, "No active to handle FireEvent");
            Ast_Sen_વૃક્ષ* senTreep = m_activep->sentreep();
            AstActive* activep = nullptr;
            જો (m_inSuspendableOrFork && (senTreep->hasInitial() || senTreep->hasClocked())) {
                // Suspendable code can trigger whenever it resumes, so fire when its NBAs are
                // committed (see visit(AstNodeProcedure*)), clearing the flag like
                // Scheme::FlagUnique
                senTreep = નવું Ast_Sen_વૃક્ષ{
                    flp, senTreep->hasClocked() ? senTreep->sensesp()->clone_વૃક્ષ(સાચું) : nullptr};
                m_nbaEventSenTreeps.ધકેલો_back(senTreep);
                m_needsInitialTrigger |= m_timingDomains.empty();
                dlyvscp->varp()->setIgnorePostWrite();
                ifp->addThensp(નવું AstAssign{flp, dlyRef(VAccess::WRITE),
                                             નવું AstConst{flp, AstConst::BitFalse{}}});
                activep = નવું AstActive{flp, "nba-event", senTreep};
                activep->sen_વૃક્ષ_Storep(senTreep);
            } નહીંતર {
                activep = નવું AstActive{flp, "nba-event", senTreep};
                AstAlwaysPre* અચલ prep = નવું AstAlwaysPre{flp};
                prep->addStmtsp(નવું AstAssign{flp, dlyRef(VAccess::WRITE),
                                              નવું AstConst{flp, AstConst::BitFalse{}}});
                activep->addStmtsp(prep);
            }
            ifp->addThensp(newp);
            m_activep->addNextHere(activep);
            activep->addStmtsp(postp);

            newp = નવું AstAssign{flp, dlyRef(VAccess::WRITE),
                                 નવું AstConst{flp, AstConst::BitTrue{}}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstAssignDly* nodep) override {
        // Prevent double processing due to AstExprStmt being moved before this node
        જો (nodep->user1SetOnce()) પાછા;

        જો (m_cfuncp) {
            જો (!v3Global.rootp()->nbaEventp()) {
                nodep->v3warn(
                    E_NOTIMING,
                    "Delayed assignment in a non-inlined function/task requires --timing");
            }
            પાછા;
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
        nodep->lhsp()->foreach([&](Ast_ગાંઠ* currp) {
            // cppcheck-suppress constVariablePointer
            જો (Ast_પદાવલિ_Stmt* અચલ exprp = VN_CAST(currp, પદાવલિ_Stmt)) {
                // Move statements before the NBA
                nodep->addHereThisAsNext(exprp->stmtsp()->unlinkFrBackWithNext());
                // Replace with result
                currp->replaceWith(exprp->resultp()->unlinkFrBack());
                // Get rid of the AstExprStmt
                VL_DO_DANGLING2(ધકેલો_Deletep(currp), currp, exprp);
            } નહીંતર જો (AstVarRef* અચલ refp = VN_CAST(currp, VarRef)) {
                // Ignore reads (e.g.: '_[*here*] <= _')
                જો (refp->access().isReadOnly()) પાછા;
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
        AstVarScope* અચલ vscp = m_currNbaLhsRefp->varScopep();
        // Record it on first encounter
        VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
        જો (!vscpInfo.m_firstNbaRefp) {
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
        જો (m_activep->sentreep()->hasClocked()) {
            જો (vscpInfo.m_fistActivep != m_activep) {
                AstVar* અચલ varp = vscp->varp();
                જો (!varp->user1SetOnce()
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
        recordWriteRef(m_currNbaLhsRefp, સાચું);

        iterateChildren(nodep);
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        // Already checked the NBA LHS ref, ignore here
        જો (nodep == m_currNbaLhsRefp) પાછા;
        // Only care about write refs
        જો (!nodep->access().isWriteOrRW()) પાછા;
        // Record write reference
        recordWriteRef(nodep, ખોટું);
    }
    શૂન્ય visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = સાચું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_CCall* અચલ nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        અચલ auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        જો (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } નહીંતર {
            માટે (AstCFunc* અચલ cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    શૂન્ય visit(AstCFunc* અચલ nodep) override {
        અચલ auto& કિંમત = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        જો (કિંમત.m_state != CFuncCache::UNINITIALIZED) પાછા;
        VL_RESTORER(m_cfuncp);
        // Its timing domains are those of the processes calling it, see visitCalledCFunc()
        VL_RESTORER_CLEAR(m_timingDomains);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }

    // Pre/Post logic are created here and their content need no further changes, so ignore.
    શૂન્ય visit(AstAlwaysPre*) override {}
    શૂન્ય visit(AstAlwaysPost*) override {}

    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
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
        V3Stats::addStat("NBA, variables using ValueQueueWhole scheme", m_n_Scheme_કિંમત_Queues_Whole);
        V3Stats::addStat("NBA, variables using ValueQueuePartial scheme",
                         m_n_Scheme_કિંમત_Queues_Partial);
        V3Stats::addStat("Optimizations, NBA flags shared", m_nSharedSetFlags);
        V3Stats::addStat("Procedures needing initial NBA trigger", m_nInitialNBA);
        V3Stats::addStat("Non-inlined co_awaits with SenTree", m_non_Inlined_CAwaits_With_Sen_વૃક્ષ);
    }
};

//######################################################################
// Delayed class functions

શૂન્ય V3Delayed::delayedAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DelayedVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("delayed", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
