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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Delayed.h"

#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3ClassGraph.h"
#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3LinkLValue.h"
#அந்தர்பாவ "V3SharedTmps.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <deque>
#அந்தர்பாவ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert AstAssignDlys (NBAs)

வகுப்பு DelayedVisitor final : பொது VNVisitor {
    // TYPES

    // The various NBA conversion schemes, including error cases
    எண்ணிக்கை வகுப்பு Scheme : uint8_t {
        Undecided = 0,
        UnsupportedCompoundArrayInLoop,
        ShadowVar,
        ShadowVarMasked,
        FlagShared,
        FlagUnique,
        மதிப்பு_Queue_Whole,
        மதிப்பு_Queue_Partial
    };

    // All info associated with a variable that is the target of an NBA
    வகுப்பு VarScopeInfo final {
    பொது:
        // First write reference encountered to the VarScope as the target on an NBA
        மாறிலி AstVarRef* m_firstNbaRefp = nullptr;
        // Active block 'm_firstNbaRefp' is under
        மாறிலி AstActive* m_fistActivep = nullptr;
        தருக்க m_whole = பொய்;  // Used on LHS of NBA directly via VarRef
        தருக்க m_partial = பொய்;  // Used on LHS of NBA under a Sel
        தருக்க m_inLoop = பொய்;  // Used on LHS of NBA in a loop
        தருக்க m_inSuspOrFork = பொய்;  // Used on LHS of NBA in suspendable process or fork
        Scheme m_scheme = Scheme::Undecided;  // Conversion scheme to use for this variable

    தனிப்பட்ட:
        // Combined sensitivities of all NBAs targeting this variable
        Ast_Sen_மரம்* m_senTreep = nullptr;

        // Union of stuff needed for the various schemes - use accessors below!
        ஒன்றிப்பு {
            கட்டமைப்பு {  // Stuff needed for Scheme::ShadowVar
                AstVarScope* vscp;  // The shadow variable
            } m_shadowVariableKit;
            கட்டமைப்பு {  // Stuff needed for Scheme::ShadowVarMasked
                AstVarScope* vscp;  // The shadow variable
                AstVarScope* maskp;  // The mask variable
            } m_shadowVarMaskedKit;
            கட்டமைப்பு {  // Stuff needed for Scheme::FlagShared
                AstActive* activep;  // The active block for the Pre/Post logic
                AstAlwaysPost* postp;  // The post block for commiting results
                AstVarScope* commitFlagp;  // The commit flag variable, for reuse
                AstIf* commitIfp;  // The previous if statement for committing, for reuse
            } m_flagSharedKit;
            கட்டமைப்பு {  // Stuff needed for Scheme::FlagUnique
                AstAlwaysPost* postp;  // The post block for commiting results
            } m_flagUniqueKit;
            கட்டமைப்பு {  // Stuff needed for Scheme::ValueQueueWhole/Scheme::ValueQueuePartial
                AstVarScope* vscp;  // The commit queue variable
            } m_மதிப்பு_Queue_Kit;
        } m_kitUnion;

    பொது:
        VarScopeInfo() = இயல்பு;
        ~VarScopeInfo() {
            // Might not be linked if there was an error
            எனில் (!m_senTreep->backp()) VL_DO_DANGLING(m_senTreep->delete_மரம்(), m_senTreep);
        }
        // Accessors for the above union fields
        auto& shadowVariableKit() {
            UASSERT(m_scheme == Scheme::ShadowVar, "Inconsistent Scheme");
            திரும்பு m_kitUnion.m_shadowVariableKit;
        }
        auto& shadowVarMaskedKit() {
            UASSERT(m_scheme == Scheme::ShadowVarMasked, "Inconsistent Scheme");
            திரும்பு m_kitUnion.m_shadowVarMaskedKit;
        }
        auto& flagSharedKit() {
            UASSERT(m_scheme == Scheme::FlagShared, "Inconsistent Scheme");
            திரும்பு m_kitUnion.m_flagSharedKit;
        }
        auto& flagUniqueKit() {
            UASSERT(m_scheme == Scheme::FlagUnique, "Inconsistent Scheme");
            திரும்பு m_kitUnion.m_flagUniqueKit;
        }
        auto& மதிப்பு_Queue_Kit() {
            UASSERT(m_scheme == Scheme::மதிப்பு_Queue_Partial || m_scheme == Scheme::மதிப்பு_Queue_Whole,
                    "Inconsistent Scheme");
            திரும்பு m_kitUnion.m_மதிப்பு_Queue_Kit;
        }

        // Accessor
        Ast_Sen_மரம்* senTreep() மாறிலி { திரும்பு m_senTreep; }

        // Add sensitivities
        வெறுமை addSensitivity(AstSenItem* nodep) {
            எனில் (!m_senTreep) m_senTreep = புதிய Ast_Sen_மரம்{nodep->fileline(), nullptr};
            // Add a copy of each term
            m_senTreep->addSensesp(nodep->clone_மரம்(உண்மை));
            // Remove duplicates
            V3Const::constifyExpensiveEdit(m_senTreep);
        }
        // cppcheck-suppress constParameterPointer
        வெறுமை addSensitivity(Ast_Sen_மரம்* nodep) { addSensitivity(nodep->sensesp()); }
    };

    // Data structure to keep track of all writes to
    கட்டமைப்பு WriteReference final {
        AstVarRef* m_refp = nullptr;  // The reference
        தருக்க m_isNBA = பொய்;  // True if an NBA write
        தருக்க m_inNonComb = பொய்;  // True if reference is known to be in non-combinational logic
        WriteReference() = இயல்பு;
        WriteReference(AstVarRef* refp, தருக்க isNBA, தருக்க inNonComb)
            : m_refp{refp}
            , m_isNBA{isNBA}
            , m_inNonComb{inNonComb} {}
    };

    // Data required to lower AstAssignDelay later
    கட்டமைப்பு NBA final {
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
    மாறிலி VNUser1InUse m_user1InUse;
    மாறிலி VNUser2InUse m_user2InUse;
    மாறிலி VNUser3InUse m_user3InUse;

    கட்டமைப்பு CFuncCache final {
        VInsertionSet<Ast_Sen_மரம்*> m_timingDomains;  // What shall be added to m_timingDomains
        std::set<AstCFunc*>
            m_includes;  // CFuncs whose CFuncCache shall be included into this - this is used to
                         // break cycles: A->B->A (instead of visiting A while it is still begin
                         // visited B just marks that it includes A)
        எண்ணிக்கை State : uint8_t {
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
    VInsertionSet<Ast_Sen_மரம்*> m_timingDomains;  // Timing resume domains
    V3SharedTmps m_dlyTmps{"__Vdly", VVarType::BLOCKTEMP};  // Temporary variables
    // Commit queue data types, by element type and partial flag. Shared by all instances,
    // as m_dlyTmps only shares variables with the same data type.
    std::map<std::pair<மாறிலி Ast_முனை_DType*, தருக்க>, AstNBACommitQueueDType*> m_cqDTypeps;

    மாறிலி std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::vector<மாறிலி AstCFunc*> m_call_அடுக்கு;  // Current callstack of AstCFuncs

    // STATE - for current visit position (use VL_RESTORER)
    AstActive* m_activep = nullptr;  // Current activate
    மாறிலி AstCFunc* m_cfuncp = nullptr;  // Current public C Function
    Ast_முனை_Procedure* m_procp = nullptr;  // Current process
    AstScope* m_scopep = nullptr;  // Current scope
    தருக்க m_inLoop = பொய்;  // True in for loops
    தருக்க m_inSuspendableOrFork = பொய்;  // True in suspendable processes and forks
    தருக்க m_ignoreBlkAndNBlk = பொய்;  // Suppress delayed assignment BLKANDNBLK
    தருக்க m_inNonCombLogic = பொய்;  // We are in non-combinational logic
    தருக்க m_needsInitialTrigger = பொய்;  // Whether a NodeProcedure needs a initial trigger
    std::vector<Ast_Sen_மரம்*> m_nbaEventSenTreeps;  // Sensitivities of '->>' in the process
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
    VDouble0 m_n_Scheme_மதிப்பு_Queues_Whole;  //  Number of variables using Scheme::ValueQueueWhole
    VDouble0 m_n_Scheme_மதிப்பு_Queues_Partial;  //  Number of variables using Scheme::ValueQueuePartial
    VDouble0 m_nSharedSetFlags;  // "Set" flags actually shared by Scheme::FlagShared variables
    VDouble0 m_nInitialNBA;  // Number of procedural blocks with initial NBA
    VDouble0 m_non_Inlined_CAwaits_With_Sen_மரம்;  // Count uses of not inlined co_awaits

    // METHODS

    // Return true iff a variable is assigned by both blocking and nonblocking
    // assignments. Issue BLKANDNBLK error if we can't prove the mixed
    // assignments are to independent bits and the blocking assignment can be
    // in combinational logic, which is something we can't safely implement
    // still.
    தருக்க checkMixedUsage(மாறிலி AstVarScope* vscp, தருக்க isIntegralOrPacked) {

        கட்டமைப்பு Ref final {
            AstVarRef* m_refp;  // The reference
            தருக்க m_inNonComb;  // True if known to be in non-combinational logic
            முழு m_lsb;  // LSB of accessed range
            முழு m_msb;  // MSB of accessed range
            Ref(AstVarRef* refp, தருக்க inNonComb, முழு lsb, முழு msb)
                : m_refp{refp}
                , m_inNonComb{inNonComb}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };

        std::vector<Ref> blkRefs;  // Blocking writes
        std::vector<Ref> nbaRefs;  // Non-blockign writes

        மாறிலி முழு width = isIntegralOrPacked ? vscp->width() : 1;

        க்கு (மாறிலி auto& writeRef : m_writeRefs(vscp)) {
            முழு lsb = 0;
            முழு msb = width - 1;
            எனில் (மாறிலி AstSel* மாறிலி selp = VN_CAST(writeRef.m_refp->backp(), Sel)) {
                எனில் (VN_IS(selp->lsbp(), Const)) {
                    lsb = selp->lsbConst();
                    msb = selp->msbConst();
                }
            }
            எனில் (writeRef.m_isNBA) {
                nbaRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            } இல்லையெனில் {
                blkRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            }
        }
        // We only run this function on targets of NBAs, so there should be at least one...
        UASSERT_OBJ(!nbaRefs.empty(), vscp, "Did not record NBA write");
        // If no blocking upadte, then we are good
        எனில் (blkRefs.empty()) திரும்பு பொய்;

        // If the blocking assignment is in non-combinational logic (i.e.:
        // in logic that has an explicit trigger), then we can safely
        // implement it (there is no race between clocked logic and post
        // scheduled logic), so need not error
        blkRefs.erase(std::remove_if(blkRefs.begin(), blkRefs.end(),
                                     [](மாறிலி Ref& ref) { திரும்பு ref.m_inNonComb; }),
                      blkRefs.end());

        // If nothing left, then we need not error
        எனில் (blkRefs.empty()) திரும்பு உண்மை;

        // If not a packed variable, warn here as we can't prove independence
        எனில் (!isIntegralOrPacked) {
            மாறிலி Ref& blkRef = blkRefs.front();
            மாறிலி Ref& nbaRef = nbaRefs.front();
            vscp->v3warn(
                BLKANDNBLK,
                "Unsupported: Blocking and non-blocking assignments to same non-packed variable: "
                    << vscp->varp()->prettyNameQ() << '\n'
                    << vscp->warnContextPrimary() << '\n'
                    << blkRef.m_refp->warnOther() << "... Location of blocking assignment\n"
                    << blkRef.m_refp->warnContextSecondary() << '\n'
                    << nbaRef.m_refp->warnOther() << "... Location of nonblocking assignment\n"
                    << nbaRef.m_refp->warnContextSecondary());
            திரும்பு உண்மை;
        }

        // We need to error if we can't prove the written bits are independent

        // Sort refs by interval
        மாறிலி auto lessThanRef = [](மாறிலி Ref& a, மாறிலி Ref& b) {
            எனில் (a.m_lsb != b.m_lsb) திரும்பு a.m_lsb < b.m_lsb;
            திரும்பு a.m_msb < b.m_msb;
        };
        std::stable_sort(blkRefs.begin(), blkRefs.end(), lessThanRef);
        std::stable_sort(nbaRefs.begin(), nbaRefs.end(), lessThanRef);
        // Iterate both vectors, checking for overlap
        auto bIt = blkRefs.begin();
        auto nIt = nbaRefs.begin();
        போது (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            எனில் (lessThanRef(*bIt, *nIt)) {
                எனில் (nIt->m_lsb <= bIt->m_msb) நிறுத்து;  // Stop on Overlap
                ++bIt;
            } இல்லையெனில் {
                எனில் (bIt->m_lsb <= nIt->m_msb) நிறுத்து;  // Stop on Overlap
                ++nIt;
            }
        }

        // If we found an overlapping range that cannot be safely implemented, then wran...
        எனில் (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            மாறிலி Ref& blkRef = *bIt;
            மாறிலி Ref& nbaRef = *nIt;
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

        திரும்பு உண்மை;
    }

    // Choose the NBA scheme used for the given variable.
    Scheme chooseScheme(மாறிலி AstVarScope* vscp, மாறிலி VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::Undecided, vscp, "NBA scheme already decided");

        மாறிலி Ast_முனை_DType* மாறிலி dtypep = vscp->dtypep()->skipRefp();
        // Unpacked arrays
        எனில் (மாறிலி AstUnpackArrayDType* மாறிலி uaDTypep = VN_CAST(dtypep, UnpackArrayDType)) {
            // If whole array is target of NBA, use ShadowVar, but not in a suspendable process or
            // fork, which resume outside the 'nba' region, so its 'pre' logic would overwrite the
            // value they write in the shadow variable
            எனில் (vscpInfo.m_whole) {
                திரும்பு vscpInfo.m_inSuspOrFork ? Scheme::FlagUnique : Scheme::ShadowVar;
            }
            // Basic underlying type of elements, if any.
            மாறிலி AstBasicDType* மாறிலி basicp = uaDTypep->basicp();
            // If used in a loop, we must have a dynamic commit queue. (Also works in suspendables)
            எனில் (vscpInfo.m_inLoop) {
                // Arrays with compound element types are currently not supported in loops
                எனில் (!basicp
                    || !(basicp->isIntegralOrPacked()  //
                         || basicp->isDouble()  //
                         || basicp->isString())) {
                    திரும்பு Scheme::UnsupportedCompoundArrayInLoop;
                }
                எனில் (vscpInfo.m_partial) திரும்பு Scheme::மதிப்பு_Queue_Partial;
                திரும்பு Scheme::மதிப்பு_Queue_Whole;
            }
            // A suspendable process or fork can resume outside the 'nba' region, so the 'pre'
            // logic of the other schemes would undo its updates. The unique flag scheme has none.
            எனில் (vscpInfo.m_inSuspOrFork) திரும்பு Scheme::FlagUnique;
            // Otherwise if an array of packed/basic elements, use the shared flag scheme
            எனில் (basicp) திரும்பு Scheme::FlagShared;
            // Finally fall back on the shadow variable scheme, e.g. for
            // arrays of unpacked structs. This will be slow.
            // TODO: generic LHS scheme as discussed in #5092
            திரும்பு Scheme::ShadowVar;
        }

        // Likewise, the 'pre' logic of the shadow variable scheme would undo the updates of a
        // suspendable process or fork, so use the unique flag scheme
        எனில் (vscpInfo.m_inSuspOrFork) திரும்பு Scheme::FlagUnique;

        மாறிலி தருக்க isIntegralOrPacked = dtypep->isIntegralOrPacked();
        // Check for mixed usage (this also warns if not OK)
        எனில் (checkMixedUsage(vscp, isIntegralOrPacked)) {
            // If it's a variable updated by both blocking and non-blocking
            // assignments, use the ShadowVarMasked schem if masked update is
            // possible. This can handle blocking and non-blocking updates to
            // inpdendent parts correctly at run-time, and always works, even
            // in loops or other dynamic context.
            எனில் (isIntegralOrPacked) திரும்பு Scheme::ShadowVarMasked;
            // If it's inside a loop, use Scheme::ShadowVar, which is safe,
            // but will generate incorrect code if a partial update is used
            எனில் (vscpInfo.m_inLoop) திரும்பு Scheme::ShadowVar;
            // Otherwise (for not packed variables), use the FlagUnique scheme,
            // which at least handles partial updates correctly, but might break
            // in loops or other dynamic context
            திரும்பு Scheme::FlagUnique;
        }

        // Otherwise use the simple shadow variable scheme
        திரும்பு Scheme::ShadowVar;
    }

    // Given an expression 'exprp', return a new expression that always evaluates to the
    // value of the given expression at this point in the program. That is:
    // - If given a non-constant expression, create a new temporary AstVarScope under the given
    //   'scopep', with the given 'name', assign the expression to it, and return a read reference
    //   to the new AstVarScope.
    // - If given a constant, just return that constant.
    // New statements are inserted before 'insertp'
    Ast_முனை_கோவை* captureVal(AstScope* மாறிலி scopep, Ast_முனை_Stmt* மாறிலி insertp,
                            Ast_முனை_கோவை* மாறிலி exprp, மாறிலி std::string& name) {
        UASSERT_OBJ(!exprp->backp(), exprp, "Should have been unlinked");
        File_வரி* மாறிலி flp = exprp->fileline();
        எனில் (VN_IS(exprp, Const)) திரும்பு exprp;
        // TODO: there are some const variables that could be simply referenced here
        AstVarScope* மாறிலி tmpVscp = m_dlyTmps.make(flp, scopep, exprp->dtypep(), name);
        insertp->addHereThisAsNext(
            புதிய AstAssign{flp, புதிய AstVarRef{flp, tmpVscp, VAccess::WRITE}, exprp});
        திரும்பு புதிய AstVarRef{flp, tmpVscp, VAccess::READ};
    };

    // Similar to 'captureVal', but captures an LValue expression. That is, the returned
    // expression will reference the same location as the input expression, at this point in the
    // program.
    Ast_முனை_கோவை* captureLhs(AstScope* மாறிலி scopep, Ast_முனை_Stmt* மாறிலி insertp,
                            Ast_முனை_கோவை* மாறிலி lhsp, மாறிலி std::string& baseName) {
        UASSERT_OBJ(!lhsp->backp(), lhsp, "Should have been unlinked");
        // Running node pointer
        Ast_முனை_கோவை* nodep = lhsp;
        // Capture AstSel indices - there should be only one
        எனில் (AstSel* மாறிலி selp = VN_CAST(nodep, Sel)) {
            மாறிலி std::string tmpName{"Lsb" + baseName};
            selp->lsbp(captureVal(scopep, insertp, selp->lsbp()->unlinkFrBack(), tmpName));
            // Continue with target
            nodep = selp->fromp();
        }
        UASSERT_OBJ(!VN_IS(nodep, Sel), lhsp, "Multiple 'AstSel' applied to LHS reference");
        // Capture the indices of selects - might be many, also below struct members. If a handle
        // selects the instance the target is a member of, capture the handle instead, as it is
        // only read, to select the target when the NBA is executed (IEEE 1800-2023 10.4.2).
        size_t nArraySels = 0;
        போது (உண்மை) {
            எனில் (Ast_முனை_Sel* மாறிலி selp = VN_CAST(nodep, முனை_Sel)) {
                மாறிலி std::string tmpName{"Dim" + std::to_string(nArraySels++) + baseName};
                selp->bitp(captureVal(scopep, insertp, selp->bitp()->unlinkFrBack(), tmpName));
                nodep = selp->fromp();
            } இல்லையெனில் எனில் (மாறிலி AstStructSel* மாறிலி selp = VN_CAST(nodep, StructSel)) {
                nodep = selp->fromp();
            } இல்லையெனில் எனில் (AstMemberSel* மாறிலி selp = VN_CAST(nodep, MemberSel)) {
                Ast_முனை_கோவை* மாறிலி handlep = selp->fromp()->unlinkFrBack();
                V3LinkLValue::linkLValueSet(handlep, VAccess::READ);
                AstVarRef* மாறிலி refp
                    = VN_AS(captureVal(scopep, insertp, handlep, "Handle" + baseName), VarRef);
                refp->varp()->setIgnorePostWrite();
                // Like the handle it replaces, mark it written, as is the member it selects
                refp->access(selp->access());
                selp->fromp(refp);
                திரும்பு lhsp;
            } இல்லையெனில் {
                நிறுத்து;
            }
        }
        // What remains must be the variable, or some sort of select, we assume can reuse it
        UASSERT_OBJ(nodep->isPure(), lhsp, "Malformed LHS in NBA");
        // Now have been converted to use the captured values
        திரும்பு lhsp;
    }

    வெறுமை add_CFunc_Cached_மதிப்புகள்(மாறிலி AstCFunc* மாறிலி cfuncp,
                              std::unordered_set<மாறிலி AstCFunc*>& visited) {
        எனில் (!visited.செருகு(cfuncp).second) திரும்பு;
        CFuncCache& மதிப்பு = m_cfuncsCache(cfuncp);
        m_timingDomains.செருகு(மதிப்பு.m_timingDomains.begin(), மதிப்பு.m_timingDomains.end());
        m_non_Inlined_CAwaits_With_Sen_மரம் += மதிப்பு.m_timingDomains.size();
        க்கு (மாறிலி AstCFunc* மாறிலி includedp : மதிப்பு.m_includes) {
            add_CFunc_Cached_மதிப்புகள்(includedp, visited);
        }
    }

    // Create a temporary variable in the given 'scopep', with the given 'name', and with 'dtypep'
    // type, with the bits selected by 'sLsbp'/'sWidthp' set to 'valuep', other bits set to zero.
    // Insert new statements before 'insertp'.
    // Returns a read reference to the temporary variable.
    AstVarRef* createWidened(File_வரி* flp, AstScope* scopep, Ast_முனை_DType* dtypep,
                             Ast_முனை_கோவை* sLsbp, முழு sWidth, மாறிலி std::string& name,
                             Ast_முனை_கோவை* valuep, Ast_முனை* insertp) {
        // Create temporary variable.
        AstVarScope* மாறிலி tp = m_dlyTmps.make(flp, scopep, dtypep, name);
        // Zero it
        AstConst* மாறிலி zerop = புதிய AstConst{flp, AstConst::DTyped{}, dtypep};
        zerop->இலக்கம்().setAllBits0();
        insertp->addHereThisAsNext(
            புதிய AstAssign{flp, புதிய AstVarRef{flp, tp, VAccess::WRITE}, zerop});
        // Set the selected bits to 'valuep'
        AstSel* மாறிலி selp = புதிய AstSel{flp, புதிய AstVarRef{flp, tp, VAccess::WRITE},
                                        sLsbp->clone_மரம்_Pure(உண்மை), sWidth};
        insertp->addHereThisAsNext(புதிய AstAssign{flp, selp, valuep});
        // This is the expression to get the value of the temporary
        திரும்பு புதிய AstVarRef{flp, tp, VAccess::READ};
    }

    // Scheme::ShadowVar
    வெறுமை prepareSchemeShadowVar(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        File_வரி* மாறிலி flp = vscp->fileline();
        AstScope* மாறிலி scopep = vscp->scopep();
        // Create the shadow variable
        மாறிலி std::string name = "_" + vscp->varp()->shortName();
        AstVarScope* மாறிலி shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), name);
        vscpInfo.shadowVariableKit().vscp = shadowVscp;
        // Mark both for V3LifePsot
        vscp->optimizeLifePost(உண்மை);
        shadowVscp->optimizeLifePost(உண்மை);
        // Create the AstActive for the Pre/Post logic
        AstActive* மாறிலி activep = புதிய AstActive{flp, "nba-shadow-variable", vscpInfo.senTreep()};
        activep->sen_மரம்_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Pre' scheduled 'shadowVariable = originalVariable' assignment
        AstAlwaysPre* மாறிலி prep = புதிய AstAlwaysPre{flp};
        activep->addStmtsp(prep);
        prep->addStmtsp(புதிய AstAssign{flp, புதிய AstVarRef{flp, shadowVscp, VAccess::WRITE},
                                      புதிய AstVarRef{flp, vscp, VAccess::READ}});
        // Add 'Post' scheduled 'originalVariable = shadowVariable' assignment
        AstAlwaysPost* மாறிலி postp = புதிய AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        postp->addStmtsp(புதிய AstAssign{flp, புதிய AstVarRef{flp, vscp, VAccess::WRITE},
                                       புதிய AstVarRef{flp, shadowVscp, VAccess::READ}});
    }
    வெறுமை convertSchemeShadowVar(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        AstVarScope* மாறிலி shadowVscp = vscpInfo.shadowVariableKit().vscp;

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* மாறிலி refp) {
            எனில் (!refp->access().isWriteOnly()) திரும்பு;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
    }

    // Scheme::ShadowVarMasked
    வெறுமை prepareSchemeShadowVarMasked(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        File_வரி* மாறிலி flp = vscp->fileline();
        AstScope* மாறிலி scopep = vscp->scopep();
        // Create the shadow variable
        மாறிலி std::string shadowName = "_" + vscp->varp()->shortName();
        AstVarScope* மாறிலி shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), shadowName);
        vscpInfo.shadowVarMaskedKit().vscp = shadowVscp;
        // Create the makk variable
        மாறிலி std::string maskName = "Mask__" + vscp->varp()->shortName();
        AstVarScope* மாறிலி maskVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), maskName);
        maskVscp->varp()->setIgnorePostWrite();
        vscpInfo.shadowVarMaskedKit().maskp = maskVscp;
        // Create the AstActive for the Post logic
        AstActive* மாறிலி activep
            = புதிய AstActive{flp, "nba-shadow-var-masked", vscpInfo.senTreep()};
        activep->sen_மரம்_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit and mask clear
        AstAlwaysPost* மாறிலி postp = புதிய AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit - vscp = (shadowVscp & maskVscp) | (vscp & ~maskVscp);
        postp->addStmtsp(புதிய AstAssign{
            flp, புதிய AstVarRef{flp, vscp, VAccess::WRITE},
            புதிய AstOr{flp,
                      புதிய AstAnd{flp, புதிய AstVarRef{flp, shadowVscp, VAccess::READ},
                                 புதிய AstVarRef{flp, maskVscp, VAccess::READ}},
                      புதிய AstAnd{flp, புதிய AstVarRef{flp, vscp, VAccess::READ},
                                 புதிய AstNot{flp, புதிய AstVarRef{flp, maskVscp, VAccess::READ}}}}});
        vscp->varp()->setIgnorePostRead();
        // Clar the mask - maskVscp = '0;
        postp->addStmtsp(
            புதிய AstAssign{flp, புதிய AstVarRef{flp, maskVscp, VAccess::WRITE},
                          புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{}, maskVscp->width(), 0}});
    }
    வெறுமை convertSchemeShadowVarMasked(AstAssignDly* nodep, AstVarScope* vscp,
                                      VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        AstVarScope* மாறிலி shadowVscp = vscpInfo.shadowVarMaskedKit().vscp;
        AstVarScope* மாறிலி maskVscp = vscpInfo.shadowVarMaskedKit().maskp;

        Ast_முனை_கோவை* lhsClonep = nodep->lhsp()->clone_மரம்(பொய்);

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* மாறிலி refp) {
            எனில் (!refp->access().isWriteOnly()) திரும்பு;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
        // Set the same bits in the mask to 1
        lhsClonep->foreach([&](AstVarRef* மாறிலி refp) {
            எனில் (!refp->access().isWriteOnly()) திரும்பு;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(maskVscp);
            refp->varp(maskVscp->varp());
        });
        File_வரி* மாறிலி flp = nodep->fileline();
        AstConst* மாறிலி onesp = புதிய AstConst{flp, AstConst::DTyped{}, lhsClonep->dtypep()};
        onesp->இலக்கம்().setAllBits1();
        nodep->addNextHere(புதிய AstAssign{flp, lhsClonep, onesp});
    }

    // Scheme::FlagShared
    வெறுமை prepareSchemeFlagShared(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_வரி* மாறிலி flp = vscp->fileline();
        AstScope* மாறிலி scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* மாறிலி activep = புதிய AstActive{flp, "nba-flag-shared", vscpInfo.senTreep()};
        activep->sen_மரம்_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        vscpInfo.flagSharedKit().activep = activep;
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* மாறிலி postp = புதிய AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagSharedKit().postp = postp;
        // Initialize
        vscpInfo.flagSharedKit().commitFlagp = nullptr;
        vscpInfo.flagSharedKit().commitIfp = nullptr;
    }
    வெறுமை convertSchemeFlagShared(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_வரி* மாறிலி flp = vscp->fileline();
        AstScope* மாறிலி scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        மாறிலி std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_முனை_கோவை* மாறிலி capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_முனை_கோவை* மாறிலி capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Is this NBA adjacent after the previously processed NBA?
        மாறிலி தருக்க consecutive = nodep == m_nextDlyp;
        m_nextDlyp = VN_CAST(nodep->nextp(), AssignDly);

        VarScopeInfo* மாறிலி prevVscpInfop = consecutive ? &m_vscpInfo(m_prevVscp) : nullptr;

        // We can reuse the flag of the previous assignment if:
        மாறிலி தருக்க reuseTheFlag =
            // Consecutive NBAs
            consecutive
            // ... that use the same scheme
            && prevVscpInfop->m_scheme == Scheme::FlagShared
            // ... and are in the same scope as the target variable
            && scopep == vscp->scopep()
            // ... and share the same overall update domain
            && prevVscpInfop->senTreep()->same_மரம்(vscpInfo.senTreep());

        எனில் (!reuseTheFlag) {
            // Create new flag
            AstVarScope* மாறிலி flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
            // Set the flag at the original NBA
            nodep->addHereThisAsNext(  //
                புதிய AstAssign{flp, புதிய AstVarRef{flp, flagVscp, VAccess::WRITE},
                              புதிய AstConst{flp, AstConst::BitTrue{}}});
            // Add the 'Pre' scheduled reset for the flag
            AstAlwaysPre* மாறிலி prep = புதிய AstAlwaysPre{flp};
            vscpInfo.flagSharedKit().activep->addStmtsp(prep);
            prep->addStmtsp(புதிய AstAssign{flp, புதிய AstVarRef{flp, flagVscp, VAccess::WRITE},
                                          புதிய AstConst{flp, AstConst::BitFalse{}}});
            // Add the 'Post' scheduled commit
            AstIf* மாறிலி ifp = புதிய AstIf{flp, புதிய AstVarRef{flp, flagVscp, VAccess::READ}};
            vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
            vscpInfo.flagSharedKit().commitFlagp = flagVscp;
            vscpInfo.flagSharedKit().commitIfp = ifp;
        } இல்லையெனில் {
            எனில் (vscp != m_prevVscp) {
                // Different variable, ensure the commit block exists for this variable,
                // can reuse existing one with the same flag, otherwise create a new one.
                AstVarScope* மாறிலி flagVscp = prevVscpInfop->flagSharedKit().commitFlagp;
                UASSERT_OBJ(flagVscp, nodep, "Commit flag of previous assignment should exist");
                எனில் (vscpInfo.flagSharedKit().commitFlagp != flagVscp) {
                    AstIf* மாறிலி ifp = புதிய AstIf{flp, புதிய AstVarRef{flp, flagVscp, VAccess::READ}};
                    vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
                    vscpInfo.flagSharedKit().commitFlagp = flagVscp;
                    vscpInfo.flagSharedKit().commitIfp = ifp;
                }
            } இல்லையெனில் {
                // Same variable, reuse the commit block of the previous assignment
                vscpInfo.flagSharedKit().commitFlagp = prevVscpInfop->flagSharedKit().commitFlagp;
                vscpInfo.flagSharedKit().commitIfp = prevVscpInfop->flagSharedKit().commitIfp;
            }
            ++m_nSharedSetFlags;
        }
        // Commit the captured value to the captured destination
        vscpInfo.flagSharedKit().commitIfp->addThensp(
            புதிய AstAssign{flp, capturedLhsp, capturedRhsp});

        // Remember the variable for the next NBA
        m_prevVscp = vscp;

        // Delete original NBA
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::FlagUnique
    வெறுமை prepareSchemeFlagUnique(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_வரி* மாறிலி flp = vscp->fileline();
        AstScope* மாறிலி scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* மாறிலி activep = புதிய AstActive{flp, "nba-flag-unique", vscpInfo.senTreep()};
        activep->sen_மரம்_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* மாறிலி postp = புதிய AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagUniqueKit().postp = postp;
    }
    வெறுமை convertSchemeFlagUnique(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_வரி* மாறிலி flp = vscp->fileline();
        AstScope* மாறிலி scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        மாறிலி std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_முனை_கோவை* மாறிலி capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_முனை_கோவை* மாறிலி capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Create new flag
        AstVarScope* மாறிலி flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
        flagVscp->varp()->setIgnorePostWrite();
        // Set the flag at the original NBA
        nodep->addHereThisAsNext(  //
            புதிய AstAssign{flp, புதிய AstVarRef{flp, flagVscp, VAccess::WRITE},
                          புதிய AstConst{flp, AstConst::BitTrue{}}});
        // Add the 'Post' scheduled commit
        AstIf* மாறிலி ifp = புதிய AstIf{flp, புதிய AstVarRef{flp, flagVscp, VAccess::READ}};
        vscpInfo.flagUniqueKit().postp->addStmtsp(ifp);
        // Immediately clear the flag
        ifp->addThensp(புதிய AstAssign{flp, புதிய AstVarRef{flp, flagVscp, VAccess::WRITE},
                                     புதிய AstConst{flp, AstConst::BitFalse{}}});
        // Commit the value
        ifp->addThensp(புதிய AstAssign{flp, capturedLhsp, capturedRhsp});

        // Delete original NBA
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::ValueQueuePartial/Scheme::ValueQueueWhole
    template <தருக்க N_Partial>
    வெறுமை prepare_Scheme_மதிப்பு_Queue(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(N_Partial ? vscpInfo.m_scheme == Scheme::மதிப்பு_Queue_Partial
                              : vscpInfo.m_scheme == Scheme::மதிப்பு_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_வரி* மாறிலி flp = vscp->fileline();
        AstScope* மாறிலி scopep = vscp->scopep();

        // Create the commit queue variable
        Ast_முனை_DType* மாறிலி elemDTypep = vscp->dtypep()->skipRefp();
        AstNBACommitQueueDType*& cqDTypep = m_cqDTypeps[{elemDTypep, N_Partial}];
        எனில் (!cqDTypep) {
            cqDTypep = புதிய AstNBACommitQueueDType{flp, elemDTypep, N_Partial};
            v3Global.rootp()->typeTablep()->addTypesp(cqDTypep);
        }
        மாறிலி std::string name = "CommitQueue__" + vscp->varp()->shortName();
        AstVarScope* மாறிலி queueVscp = m_dlyTmps.make(flp, scopep, cqDTypep, name);
        queueVscp->varp()->noReset(உண்மை);
        queueVscp->varp()->setIgnorePostWrite();
        vscpInfo.மதிப்பு_Queue_Kit().vscp = queueVscp;
        // Create the AstActive for the Post logic
        AstActive* மாறிலி activep
            = புதிய AstActive{flp, "nba-value-queue-whole", vscpInfo.senTreep()};
        activep->sen_மரம்_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit
        AstAlwaysPost* மாறிலி postp = புதிய AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit
        AstCMethodHard* மாறிலி callp = புதிய AstCMethodHard{
            flp, புதிய AstVarRef{flp, queueVscp, VAccess::READWRITE}, VCMethod::NBA_COMMIT};
        callp->dtypeSetVoid();
        // TODO: this is a partial update, so must be READWRITE, but that breaks scheduling
        callp->addPinsp(புதிய AstVarRef{flp, vscp, VAccess::WRITE});
        postp->addStmtsp(callp->makeStmt());
    }

    வெறுமை convert_Scheme_மதிப்பு_Queue(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo,
                                 தருக்க partial) {
        UASSERT_OBJ(partial ? vscpInfo.m_scheme == Scheme::மதிப்பு_Queue_Partial
                            : vscpInfo.m_scheme == Scheme::மதிப்பு_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_வரி* மாறிலி flp = vscp->fileline();
        AstScope* மாறிலி scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        மாறிலி std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_முனை_கோவை* மாறிலி capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_முனை_கோவை* மாறிலி capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // RHS value (can be widened/masked iff Partial)
        Ast_முனை_கோவை* valuep = capturedRhsp;
        // RHS mask (iff Partial)
        Ast_முனை_கோவை* maskp = nullptr;

        // Running node for LHS deconstruction
        Ast_முனை_கோவை* lhsNodep = capturedLhsp;

        // If partial updates are required, construct the mask and the widened value
        எனில் (partial) {
            // Type of array element
            Ast_முனை_DType* மாறிலி eDTypep = [&]() -> Ast_முனை_DType* {
                Ast_முனை_DType* dtypep = vscp->dtypep()->skipRefp();
                போது (AstUnpackArrayDType* மாறிலி ap = VN_CAST(dtypep, UnpackArrayDType)) {
                    dtypep = ap->subDTypep()->skipRefp();
                }
                திரும்பு dtypep;
            }();

            எனில் (மாறிலி AstSel* மாறிலி lSelp = VN_CAST(lhsNodep, Sel)) {
                // This is a partial assignment.
                // Need to create a mask and widen the value to element size.
                lhsNodep = lSelp->fromp();
                Ast_முனை_கோவை* மாறிலி sLsbp = lSelp->lsbp();
                மாறிலி முழு sWidth = lSelp->widthConst();

                // Create mask value
                maskp = [&]() -> Ast_முனை_கோவை* {
                    // Constant mask we can compute here
                    எனில் (மாறிலி AstConst* மாறிலி cLsbp = VN_CAST(sLsbp, Const)) {
                        AstConst* மாறிலி cp = புதிய AstConst{flp, AstConst::DTyped{}, eDTypep};
                        cp->இலக்கம்().setMask(sWidth, cLsbp->toSInt());
                        திரும்பு cp;
                    }

                    // A non-constant mask we must compute at run-time.
                    AstConst* மாறிலி onesp = புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{}, sWidth, 0};
                    onesp->இலக்கம்().setAllBits1();
                    திரும்பு createWidened(flp, scopep, eDTypep, sLsbp, sWidth, "Mask" + baseName,
                                         onesp, nodep);
                }();

                // Adjust value to element size
                valuep = [&]() -> Ast_முனை_கோவை* {
                    // Constant value with constant select we can compute here
                    எனில் (AstConst* மாறிலி cValuep = VN_CAST(valuep, Const)) {
                        எனில் (மாறிலி AstConst* மாறிலி cLsbp = VN_CAST(sLsbp, Const)) {
                            AstConst* மாறிலி cp = புதிய AstConst{flp, AstConst::DTyped{}, eDTypep};
                            cp->இலக்கம்().setAllBits0();
                            cp->இலக்கம்().opSelInto(cValuep->இலக்கம்(), cLsbp->toSInt(), sWidth);
                            VL_DO_DANGLING(valuep->delete_மரம்(), valuep);
                            திரும்பு cp;
                        }
                    }

                    // A non-constant value we must adjust.
                    திரும்பு createWidened(flp, scopep, eDTypep, sLsbp, sWidth,  //
                                         "Elem" + baseName, valuep, nodep);
                }();
            } இல்லையெனில் {
                // If this assignment is not partial, set mask to ones and we are done
                AstConst* மாறிலி ones = புதிய AstConst{flp, AstConst::DTyped{}, eDTypep};
                ones->இலக்கம்().setAllBits1();
                maskp = ones;
            }
        }

        // Extract array indices
        std::vector<Ast_முனை_கோவை*> idxps;
        {
            UASSERT_OBJ(VN_IS(lhsNodep, ArraySel), lhsNodep, "Unexpected LHS form");
            போது (AstArraySel* மாறிலி aSelp = VN_CAST(lhsNodep, ArraySel)) {
                idxps.emplace_back(aSelp->bitp()->unlinkFrBack());
                lhsNodep = aSelp->fromp();
            }
            UASSERT_OBJ(VN_IS(lhsNodep, VarRef), lhsNodep, "Unexpected LHS form");
            std::reverse(idxps.begin(), idxps.end());
        }

        // Done with the LHS at this point
        VL_DO_DANGLING(தள்ளு_Deletep(capturedLhsp), capturedLhsp);

        // Enqueue the update at the site of the original NBA
        AstCMethodHard* மாறிலி callp = புதிய AstCMethodHard{
            flp, புதிய AstVarRef{flp, vscpInfo.மதிப்பு_Queue_Kit().vscp, VAccess::READWRITE},
            VCMethod::NBA_ENQUEUE};
        callp->dtypeSetVoid();
        callp->addPinsp(valuep);
        எனில் (partial) callp->addPinsp(maskp);
        க்கு (Ast_முனை_கோவை* மாறிலி indexp : idxps) callp->addPinsp(indexp);
        nodep->addHereThisAsNext(callp->makeStmt());

        // Delete original NBA
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Record where a variable is assigned
    வெறுமை recordWriteRef(AstVarRef* nodep, தருக்க nonBlocking) {
        // Ignore references in certain contexts
        எனில் (m_ignoreBlkAndNBlk) திரும்பு;
        // Ignore if it's an array
        // TODO: we do this because it used to be the previous behaviour.
        //       Is it still required, or should we warn for arrays as well?
        //       Scheduling is no different for them...
        //       Clarification: This is OK for arrays of primitive types, but
        //       arrays that use the ShadowVar scheme don't work...
        எனில் (VN_IS(nodep->varScopep()->dtypep()->skipRefp(), UnpackArrayDType)) திரும்பு;

        m_writeRefs(nodep->varScopep()).emplace_back(nodep, nonBlocking, m_inNonCombLogic);
    }

    template <typename Procedure_T>
    நிலையான தருக்க isProcedureWithSentreep(மாறிலி Ast_முனை_Procedure* மாறிலி nodep) {
        மாறிலி Procedure_T* மாறிலி procedurep = Ast_முனை::cast<Procedure_T>(nodep);
        திரும்பு procedurep && procedurep->sentreep();
    }

    // Visit AstCFunc from a AstNodeCCall - this is made into a separate quasi-visitor because
    // AstCFunc that is not called from the code (e.g.: DPI exports) does not need to be visited
    // this way. Also, not visiting such AstCFuncs allows to avoid caching results for them which
    // this function does - which could lead to excessive memory usage
    வெறுமை visitCalledCFunc(AstCFunc* மாறிலி nodep) {
        CFuncCache& மதிப்பு = m_cfuncsCache(nodep);
        தேர்வு (மதிப்பு.m_state) {
        நிலை CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_timingDomains);

            // Visit
            மதிப்பு.m_state = CFuncCache::VISITING;
            m_call_அடுக்கு.தள்ளு_back(nodep);
            {
                VL_RESTORER(m_cfuncp);
                m_cfuncp = nodep;
                iterateChildren(nodep);
            }
            m_call_அடுக்கு.எடு_back();
            மதிப்பு.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_timingDomains, மதிப்பு.m_timingDomains);
        } நிறுத்து;
        நிலை CFuncCache::VISITING: {
            க்கு (size_t i = m_call_அடுக்கு.size() - 1; m_call_அடுக்கு.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_அடுக்கு[i]).m_includes.செருகு(nodep);
            }
            திரும்பு;  // Break recursion
        }
        நிலை CFuncCache::INITIALIZED: நிறுத்து;
        }
        std::unordered_set<மாறிலி AstCFunc*> visited;
        add_CFunc_Cached_மதிப்புகள்(nodep, visited);
    }

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override {
        iterateChildren(nodep);
        // Decide which scheme to use for each variable and do the 'prepare' step
        க்கு (AstVarScope* மாறிலி vscp : m_vscps) {
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            vscpInfo.m_scheme = chooseScheme(vscp, vscpInfo);
            // Run 'prepare' step
            தேர்வு (vscpInfo.m_scheme) {
            நிலை Scheme::Undecided:  // LCOV_EXCL_START
                UASSERT_OBJ(பொய், vscp, "Failed to choose NBA scheme");
                நிறுத்து;  // LCOV_EXCL_STOP
            நிலை Scheme::UnsupportedCompoundArrayInLoop: {
                // Will report error at the site of the NBA
                நிறுத்து;
            }
            நிலை Scheme::ShadowVar: {
                ++m_nSchemeShadowVar;
                prepareSchemeShadowVar(vscp, vscpInfo);
                நிறுத்து;
            }
            நிலை Scheme::ShadowVarMasked: {
                ++m_nSchemeShadowVarMasked;
                prepareSchemeShadowVarMasked(vscp, vscpInfo);
                நிறுத்து;
            }
            நிலை Scheme::FlagShared: {
                ++m_nSchemeFlagShared;
                prepareSchemeFlagShared(vscp, vscpInfo);
                நிறுத்து;
            }
            நிலை Scheme::FlagUnique: {
                ++m_nSchemeFlagUnique;
                prepareSchemeFlagUnique(vscp, vscpInfo);
                நிறுத்து;
            }
            நிலை Scheme::மதிப்பு_Queue_Whole: {
                ++m_n_Scheme_மதிப்பு_Queues_Whole;
                prepare_Scheme_மதிப்பு_Queue</* Partial: */ பொய்>(vscp, vscpInfo);
                நிறுத்து;
            }
            நிலை Scheme::மதிப்பு_Queue_Partial: {
                ++m_n_Scheme_மதிப்பு_Queues_Partial;
                prepare_Scheme_மதிப்பு_Queue</* Partial: */ உண்மை>(vscp, vscpInfo);
                நிறுத்து;
            }
            }
        }
        // Convert all NBAs
        க்கு (மாறிலி NBA& nba : m_nbas) {
            AstAssignDly* மாறிலி nbap = nba.nodep;
            AstVarScope* மாறிலி vscp = nba.vscp;
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            // Run 'convert' step
            தேர்வு (vscpInfo.m_scheme) {
            நிலை Scheme::Undecided: {  // LCOV_EXCL_START
                UASSERT_OBJ(பொய், vscp, "Unreachable");
                நிறுத்து;
            }  // LCOV_EXCL_STOP
            நிலை Scheme::UnsupportedCompoundArrayInLoop: {
                // TODO: make this an E_UNSUPPORTED...
                nbap->v3warn(BLKLOOPINIT, "Unsupported: Non-blocking assignment to array with "
                                          "compound element type inside loop");
                நிறுத்து;
            }
            நிலை Scheme::ShadowVar: {
                convertSchemeShadowVar(nbap, vscp, vscpInfo);
                நிறுத்து;
            }
            நிலை Scheme::ShadowVarMasked: {
                convertSchemeShadowVarMasked(nbap, vscp, vscpInfo);
                நிறுத்து;
            }
            நிலை Scheme::FlagShared: {
                convertSchemeFlagShared(nbap, vscp, vscpInfo);
                நிறுத்து;
            }
            நிலை Scheme::FlagUnique: {
                convertSchemeFlagUnique(nbap, vscp, vscpInfo);
                நிறுத்து;
            }
            நிலை Scheme::மதிப்பு_Queue_Whole: {
                convert_Scheme_மதிப்பு_Queue(nbap, vscp, vscpInfo, /* partial: */ பொய்);
                நிறுத்து;
            }
            நிலை Scheme::மதிப்பு_Queue_Partial:
                convert_Scheme_மதிப்பு_Queue(nbap, vscp, vscpInfo, /* partial: */ உண்மை);
                நிறுத்து;
            }
        }
    }
    வெறுமை visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstActive* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ignoreBlkAndNBlk);
        VL_RESTORER(m_inNonCombLogic);
        m_activep = nodep;
        மாறிலி Ast_Sen_மரம்* மாறிலி senTreep = nodep->sentreep();
        m_ignoreBlkAndNBlk = senTreep->hasStatic() || senTreep->hasInitial();
        m_inNonCombLogic = senTreep->hasClocked();
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Procedure* nodep) override {
        VL_RESTORER(m_needsInitialTrigger);
        VL_RESTORER_CLEAR(m_nbaEventSenTreeps);
        மாறிலி size_t firstNBAAddedIndex = m_nbas.size();
        {
            VL_RESTORER(m_inSuspendableOrFork);
            VL_RESTORER(m_procp);
            VL_RESTORER(m_ignoreBlkAndNBlk);
            VL_RESTORER(m_inNonCombLogic);
            // When we are dealing with initial block we need to
            // treat it as suspendable when we meet a NBA
            m_inSuspendableOrFork = nodep->isSuspendable() || VN_IS(nodep, Initial);
            m_procp = nodep;
            எனில் (nodep->isSuspendable()) {
                m_ignoreBlkAndNBlk = பொய்;
                m_inNonCombLogic = உண்மை;
            }
            iterateChildren(nodep);
        }
        auto containsClocled = [](மாறிலி AstSenItem* itemp) {
            போது (itemp) {
                எனில் (itemp->edgeType().clockedStmt()) திரும்பு உண்மை;
                itemp = VN_AS(itemp->nextp(), SenItem);
            }
            திரும்பு பொய்;
        };
        மாறிலி தருக்க addInitialTrigger = m_needsInitialTrigger
                                       && !(isProcedureWithSentreep<AstAlways>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysObserved>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysReactive>(nodep))
                                       && !containsClocled(m_activep->sentreep()->sensesp());
        எனில் (m_timingDomains.empty() && !addInitialTrigger) திரும்பு;

        // There were some timing domains involved in the process. Add all of them as sensitivities
        // of all NBA targets in this process. Note this is a bit of a sledgehammer, we should only
        // need those that directly precede the NBA in control flow, but that is hard to compute,
        // so we will hammer away.

        // First gather all senItems
        AstSenItem* senItemp = nullptr;
        எனில் (addInitialTrigger) {
            senItemp = புதிய AstSenItem{nodep->fileline(), AstSenItem::InitialNBA{}};
            ++m_nInitialNBA;
        }

        க்கு (மாறிலி Ast_Sen_மரம்* மாறிலி domainp : m_timingDomains) {
            எனில் (domainp->sensesp())
                senItemp = Ast_முனை::addNext(senItemp, domainp->sensesp()->clone_மரம்(உண்மை));
        }
        m_timingDomains.clear();
        // Add them to all nba targets we gathered in this process
        க்கு (size_t i = firstNBAAddedIndex; i < m_nbas.size(); ++i) {
            m_vscpInfo(m_nbas[i].vscp).addSensitivity(senItemp);
        }
        க்கு (Ast_Sen_மரம்* மாறிலி senTreep : m_nbaEventSenTreeps)
            senTreep->addSensesp(senItemp->clone_மரம்(உண்மை));
        // Done with these
        VL_DO_DANGLING(senItemp->delete_மரம்(), senItemp);
    }
    வெறுமை visit(AstFork* nodep) override {
        VL_RESTORER(m_inSuspendableOrFork);
        m_inSuspendableOrFork = உண்மை;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCAwait* nodep) override {
        எனில் (nodep->sentreep()) m_timingDomains.செருகு(nodep->sentreep());
        iterateChildren(nodep);
    }
    வெறுமை visit(AstFireEvent* nodep) override {
        UASSERT_OBJ(v3Global.hasEvents(), nodep, "Inconsistent");
        File_வரி* மாறிலி flp = nodep->fileline();

        Ast_முனை_கோவை* மாறிலி eventp = nodep->operandp()->unlinkFrBack();

        // Enqueue for clearing 'triggered' state on next eval
        AstCStmt* மாறிலி cstmtp = புதிய AstCStmt{flp};
        cstmtp->add("vlSymsp->fireEvent(");
        cstmtp->add(eventp);
        cstmtp->add(");");

        Ast_முனை* newp = cstmtp;
        மாறிலி AstVarRef* மாறிலி vrefp = VN_CAST(eventp, VarRef);
        எனில் (nodep->isDelayed() && (m_cfuncp || !vrefp)) {
            // V3Timing converts these with --timing
            nodep->v3warn(E_NOTIMING, "Nonblocking event trigger "
                                          << (m_cfuncp ? "in a non-inlined function/task"
                                                       : "of a class or interface member or "
                                                         "array element")
                                          << " requires --timing");
        } இல்லையெனில் எனில் (nodep->isDelayed()) {
            மாறிலி std::string newvarname = "_" + vrefp->varp()->shortName();
            AstVarScope* மாறிலி dlyvscp
                = m_dlyTmps.make(flp, vrefp->varScopep()->scopep(), 1, newvarname);

            மாறிலி auto dlyRef = [=](VAccess access) {  //
                திரும்பு புதிய AstVarRef{flp, dlyvscp, access};
            };

            AstAlwaysPost* மாறிலி postp = புதிய AstAlwaysPost{flp};
            AstIf* மாறிலி ifp = புதிய AstIf{flp, dlyRef(VAccess::READ)};
            postp->addStmtsp(ifp);

            UASSERT_OBJ(m_activep, nodep, "No active to handle FireEvent");
            Ast_Sen_மரம்* senTreep = m_activep->sentreep();
            AstActive* activep = nullptr;
            எனில் (m_inSuspendableOrFork && (senTreep->hasInitial() || senTreep->hasClocked())) {
                // Suspendable code can trigger whenever it resumes, so fire when its NBAs are
                // committed (see visit(AstNodeProcedure*)), clearing the flag like
                // Scheme::FlagUnique
                senTreep = புதிய Ast_Sen_மரம்{
                    flp, senTreep->hasClocked() ? senTreep->sensesp()->clone_மரம்(உண்மை) : nullptr};
                m_nbaEventSenTreeps.தள்ளு_back(senTreep);
                m_needsInitialTrigger |= m_timingDomains.empty();
                dlyvscp->varp()->setIgnorePostWrite();
                ifp->addThensp(புதிய AstAssign{flp, dlyRef(VAccess::WRITE),
                                             புதிய AstConst{flp, AstConst::BitFalse{}}});
                activep = புதிய AstActive{flp, "nba-event", senTreep};
                activep->sen_மரம்_Storep(senTreep);
            } இல்லையெனில் {
                activep = புதிய AstActive{flp, "nba-event", senTreep};
                AstAlwaysPre* மாறிலி prep = புதிய AstAlwaysPre{flp};
                prep->addStmtsp(புதிய AstAssign{flp, dlyRef(VAccess::WRITE),
                                              புதிய AstConst{flp, AstConst::BitFalse{}}});
                activep->addStmtsp(prep);
            }
            ifp->addThensp(newp);
            m_activep->addNextHere(activep);
            activep->addStmtsp(postp);

            newp = புதிய AstAssign{flp, dlyRef(VAccess::WRITE),
                                 புதிய AstConst{flp, AstConst::BitTrue{}}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstAssignDly* nodep) override {
        // Prevent double processing due to AstExprStmt being moved before this node
        எனில் (nodep->user1SetOnce()) திரும்பு;

        எனில் (m_cfuncp) {
            எனில் (!v3Global.rootp()->nbaEventp()) {
                nodep->v3warn(
                    E_NOTIMING,
                    "Delayed assignment in a non-inlined function/task requires --timing");
            }
            திரும்பு;
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
        nodep->lhsp()->foreach([&](Ast_முனை* currp) {
            // cppcheck-suppress constVariablePointer
            எனில் (Ast_கோவை_Stmt* மாறிலி exprp = VN_CAST(currp, கோவை_Stmt)) {
                // Move statements before the NBA
                nodep->addHereThisAsNext(exprp->stmtsp()->unlinkFrBackWithNext());
                // Replace with result
                currp->replaceWith(exprp->resultp()->unlinkFrBack());
                // Get rid of the AstExprStmt
                VL_DO_DANGLING2(தள்ளு_Deletep(currp), currp, exprp);
            } இல்லையெனில் எனில் (AstVarRef* மாறிலி refp = VN_CAST(currp, VarRef)) {
                // Ignore reads (e.g.: '_[*here*] <= _')
                எனில் (refp->access().isReadOnly()) திரும்பு;
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
        AstVarScope* மாறிலி vscp = m_currNbaLhsRefp->varScopep();
        // Record it on first encounter
        VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
        எனில் (!vscpInfo.m_firstNbaRefp) {
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
        எனில் (m_activep->sentreep()->hasClocked()) {
            எனில் (vscpInfo.m_fistActivep != m_activep) {
                AstVar* மாறிலி varp = vscp->varp();
                எனில் (!varp->user1SetOnce()
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
        recordWriteRef(m_currNbaLhsRefp, உண்மை);

        iterateChildren(nodep);
    }
    வெறுமை visit(AstVarRef* nodep) override {
        // Already checked the NBA LHS ref, ignore here
        எனில் (nodep == m_currNbaLhsRefp) திரும்பு;
        // Only care about write refs
        எனில் (!nodep->access().isWriteOrRW()) திரும்பு;
        // Record write reference
        recordWriteRef(nodep, பொய்);
    }
    வெறுமை visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = உண்மை;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_CCall* மாறிலி nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        மாறிலி auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        எனில் (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } இல்லையெனில் {
            க்கு (AstCFunc* மாறிலி cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    வெறுமை visit(AstCFunc* மாறிலி nodep) override {
        மாறிலி auto& மதிப்பு = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        எனில் (மதிப்பு.m_state != CFuncCache::UNINITIALIZED) திரும்பு;
        VL_RESTORER(m_cfuncp);
        // Its timing domains are those of the processes calling it, see visitCalledCFunc()
        VL_RESTORER_CLEAR(m_timingDomains);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }

    // Pre/Post logic are created here and their content need no further changes, so ignore.
    வெறுமை visit(AstAlwaysPre*) override {}
    வெறுமை visit(AstAlwaysPost*) override {}

    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
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
        V3Stats::addStat("NBA, variables using ValueQueueWhole scheme", m_n_Scheme_மதிப்பு_Queues_Whole);
        V3Stats::addStat("NBA, variables using ValueQueuePartial scheme",
                         m_n_Scheme_மதிப்பு_Queues_Partial);
        V3Stats::addStat("Optimizations, NBA flags shared", m_nSharedSetFlags);
        V3Stats::addStat("Procedures needing initial NBA trigger", m_nInitialNBA);
        V3Stats::addStat("Non-inlined co_awaits with SenTree", m_non_Inlined_CAwaits_With_Sen_மரம்);
    }
};

//######################################################################
// Delayed class functions

வெறுமை V3Delayed::delayedAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DelayedVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("delayed", 0, dump_மரம்_Either_Level() >= 3);
}
