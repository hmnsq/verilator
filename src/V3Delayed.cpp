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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Delayed.h"

#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3ClassGraph.h"
#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3LinkLValue.h"
#ଅନ୍ତର୍ଭାଵ "V3SharedTmps.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <deque>
#ଅନ୍ତର୍ଭାଵ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert AstAssignDlys (NBAs)

ଶ୍ରେଣୀ DelayedVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // TYPES

    // The various NBA conversion schemes, including error cases
    ଗଣନା ଶ୍ରେଣୀ Scheme : uint8_t {
        Undecided = 0,
        UnsupportedCompoundArrayInLoop,
        ShadowVar,
        ShadowVarMasked,
        FlagShared,
        FlagUnique,
        ମୂଲ୍ୟ_Queue_Whole,
        ମୂଲ୍ୟ_Queue_Partial
    };

    // All info associated with a variable that is the target of an NBA
    ଶ୍ରେଣୀ VarScopeInfo final {
    ସର୍ବସାଧାରଣ:
        // First write reference encountered to the VarScope as the target on an NBA
        ଅଚଳ AstVarRef* m_firstNbaRefp = nullptr;
        // Active block 'm_firstNbaRefp' is under
        ଅଚଳ AstActive* m_fistActivep = nullptr;
        ତାର୍କିକ m_whole = ମିଥ୍ୟା;  // Used on LHS of NBA directly via VarRef
        ତାର୍କିକ m_partial = ମିଥ୍ୟା;  // Used on LHS of NBA under a Sel
        ତାର୍କିକ m_inLoop = ମିଥ୍ୟା;  // Used on LHS of NBA in a loop
        ତାର୍କିକ m_inSuspOrFork = ମିଥ୍ୟା;  // Used on LHS of NBA in suspendable process or fork
        Scheme m_scheme = Scheme::Undecided;  // Conversion scheme to use for this variable

    ବ୍ୟକ୍ତିଗତ:
        // Combined sensitivities of all NBAs targeting this variable
        Ast_Sen_ବୃକ୍ଷ* m_senTreep = nullptr;

        // Union of stuff needed for the various schemes - use accessors below!
        ସଂଘ {
            ସଂରଚନା {  // Stuff needed for Scheme::ShadowVar
                AstVarScope* vscp;  // The shadow variable
            } m_shadowVariableKit;
            ସଂରଚନା {  // Stuff needed for Scheme::ShadowVarMasked
                AstVarScope* vscp;  // The shadow variable
                AstVarScope* maskp;  // The mask variable
            } m_shadowVarMaskedKit;
            ସଂରଚନା {  // Stuff needed for Scheme::FlagShared
                AstActive* activep;  // The active block for the Pre/Post logic
                AstAlwaysPost* postp;  // The post block for commiting results
                AstVarScope* commitFlagp;  // The commit flag variable, for reuse
                AstIf* commitIfp;  // The previous if statement for committing, for reuse
            } m_flagSharedKit;
            ସଂରଚନା {  // Stuff needed for Scheme::FlagUnique
                AstAlwaysPost* postp;  // The post block for commiting results
            } m_flagUniqueKit;
            ସଂରଚନା {  // Stuff needed for Scheme::ValueQueueWhole/Scheme::ValueQueuePartial
                AstVarScope* vscp;  // The commit queue variable
            } m_ମୂଲ୍ୟ_Queue_Kit;
        } m_kitUnion;

    ସର୍ବସାଧାରଣ:
        VarScopeInfo() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        ~VarScopeInfo() {
            // Might not be linked if there was an error
            ଯଦି (!m_senTreep->backp()) VL_DO_DANGLING(m_senTreep->delete_ବୃକ୍ଷ(), m_senTreep);
        }
        // Accessors for the above union fields
        auto& shadowVariableKit() {
            UASSERT(m_scheme == Scheme::ShadowVar, "Inconsistent Scheme");
            ଫେରନ୍ତୁ m_kitUnion.m_shadowVariableKit;
        }
        auto& shadowVarMaskedKit() {
            UASSERT(m_scheme == Scheme::ShadowVarMasked, "Inconsistent Scheme");
            ଫେରନ୍ତୁ m_kitUnion.m_shadowVarMaskedKit;
        }
        auto& flagSharedKit() {
            UASSERT(m_scheme == Scheme::FlagShared, "Inconsistent Scheme");
            ଫେରନ୍ତୁ m_kitUnion.m_flagSharedKit;
        }
        auto& flagUniqueKit() {
            UASSERT(m_scheme == Scheme::FlagUnique, "Inconsistent Scheme");
            ଫେରନ୍ତୁ m_kitUnion.m_flagUniqueKit;
        }
        auto& ମୂଲ୍ୟ_Queue_Kit() {
            UASSERT(m_scheme == Scheme::ମୂଲ୍ୟ_Queue_Partial || m_scheme == Scheme::ମୂଲ୍ୟ_Queue_Whole,
                    "Inconsistent Scheme");
            ଫେରନ୍ତୁ m_kitUnion.m_ମୂଲ୍ୟ_Queue_Kit;
        }

        // Accessor
        Ast_Sen_ବୃକ୍ଷ* senTreep() ଅଚଳ { ଫେରନ୍ତୁ m_senTreep; }

        // Add sensitivities
        ଶୂନ୍ୟ addSensitivity(AstSenItem* nodep) {
            ଯଦି (!m_senTreep) m_senTreep = ନୂତନ Ast_Sen_ବୃକ୍ଷ{nodep->fileline(), nullptr};
            // Add a copy of each term
            m_senTreep->addSensesp(nodep->clone_ବୃକ୍ଷ(ସତ୍ୟ));
            // Remove duplicates
            V3Const::constifyExpensiveEdit(m_senTreep);
        }
        // cppcheck-suppress constParameterPointer
        ଶୂନ୍ୟ addSensitivity(Ast_Sen_ବୃକ୍ଷ* nodep) { addSensitivity(nodep->sensesp()); }
    };

    // Data structure to keep track of all writes to
    ସଂରଚନା WriteReference final {
        AstVarRef* m_refp = nullptr;  // The reference
        ତାର୍କିକ m_isNBA = ମିଥ୍ୟା;  // True if an NBA write
        ତାର୍କିକ m_inNonComb = ମିଥ୍ୟା;  // True if reference is known to be in non-combinational logic
        WriteReference() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        WriteReference(AstVarRef* refp, ତାର୍କିକ isNBA, ତାର୍କିକ inNonComb)
            : m_refp{refp}
            , m_isNBA{isNBA}
            , m_inNonComb{inNonComb} {}
    };

    // Data required to lower AstAssignDelay later
    ସଂରଚନା NBA final {
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
    ଅଚଳ VNUser1InUse m_user1InUse;
    ଅଚଳ VNUser2InUse m_user2InUse;
    ଅଚଳ VNUser3InUse m_user3InUse;

    ସଂରଚନା CFuncCache final {
        VInsertionSet<Ast_Sen_ବୃକ୍ଷ*> m_timingDomains;  // What shall be added to m_timingDomains
        std::set<AstCFunc*>
            m_includes;  // CFuncs whose CFuncCache shall be included into this - this is used to
                         // break cycles: A->B->A (instead of visiting A while it is still begin
                         // visited B just marks that it includes A)
        ଗଣନା State : uint8_t {
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
    VInsertionSet<Ast_Sen_ବୃକ୍ଷ*> m_timingDomains;  // Timing resume domains
    V3SharedTmps m_dlyTmps{"__Vdly", VVarType::BLOCKTEMP};  // Temporary variables
    // Commit queue data types, by element type and partial flag. Shared by all instances,
    // as m_dlyTmps only shares variables with the same data type.
    std::map<std::pair<ଅଚଳ Ast_ଗଣ୍ଠି_DType*, ତାର୍କିକ>, AstNBACommitQueueDType*> m_cqDTypeps;

    ଅଚଳ std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::vector<ଅଚଳ AstCFunc*> m_call_ଗଦା;  // Current callstack of AstCFuncs

    // STATE - for current visit position (use VL_RESTORER)
    AstActive* m_activep = nullptr;  // Current activate
    ଅଚଳ AstCFunc* m_cfuncp = nullptr;  // Current public C Function
    Ast_ଗଣ୍ଠି_Procedure* m_procp = nullptr;  // Current process
    AstScope* m_scopep = nullptr;  // Current scope
    ତାର୍କିକ m_inLoop = ମିଥ୍ୟା;  // True in for loops
    ତାର୍କିକ m_inSuspendableOrFork = ମିଥ୍ୟା;  // True in suspendable processes and forks
    ତାର୍କିକ m_ignoreBlkAndNBlk = ମିଥ୍ୟା;  // Suppress delayed assignment BLKANDNBLK
    ତାର୍କିକ m_inNonCombLogic = ମିଥ୍ୟା;  // We are in non-combinational logic
    ତାର୍କିକ m_needsInitialTrigger = ମିଥ୍ୟା;  // Whether a NodeProcedure needs a initial trigger
    std::vector<Ast_Sen_ବୃକ୍ଷ*> m_nbaEventSenTreeps;  // Sensitivities of '->>' in the process
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
    VDouble0 m_n_Scheme_ମୂଲ୍ୟ_Queues_Whole;  //  Number of variables using Scheme::ValueQueueWhole
    VDouble0 m_n_Scheme_ମୂଲ୍ୟ_Queues_Partial;  //  Number of variables using Scheme::ValueQueuePartial
    VDouble0 m_nSharedSetFlags;  // "Set" flags actually shared by Scheme::FlagShared variables
    VDouble0 m_nInitialNBA;  // Number of procedural blocks with initial NBA
    VDouble0 m_non_Inlined_CAwaits_With_Sen_ବୃକ୍ଷ;  // Count uses of not inlined co_awaits

    // METHODS

    // Return true iff a variable is assigned by both blocking and nonblocking
    // assignments. Issue BLKANDNBLK error if we can't prove the mixed
    // assignments are to independent bits and the blocking assignment can be
    // in combinational logic, which is something we can't safely implement
    // still.
    ତାର୍କିକ checkMixedUsage(ଅଚଳ AstVarScope* vscp, ତାର୍କିକ isIntegralOrPacked) {

        ସଂରଚନା Ref final {
            AstVarRef* m_refp;  // The reference
            ତାର୍କିକ m_inNonComb;  // True if known to be in non-combinational logic
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_lsb;  // LSB of accessed range
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_msb;  // MSB of accessed range
            Ref(AstVarRef* refp, ତାର୍କିକ inNonComb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb)
                : m_refp{refp}
                , m_inNonComb{inNonComb}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };

        std::vector<Ref> blkRefs;  // Blocking writes
        std::vector<Ref> nbaRefs;  // Non-blockign writes

        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = isIntegralOrPacked ? vscp->width() : 1;

        ପାଇଁ (ଅଚଳ auto& writeRef : m_writeRefs(vscp)) {
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = 0;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = width - 1;
            ଯଦି (ଅଚଳ AstSel* ଅଚଳ selp = VN_CAST(writeRef.m_refp->backp(), Sel)) {
                ଯଦି (VN_IS(selp->lsbp(), Const)) {
                    lsb = selp->lsbConst();
                    msb = selp->msbConst();
                }
            }
            ଯଦି (writeRef.m_isNBA) {
                nbaRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            } ଅନ୍ୟଥା {
                blkRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            }
        }
        // We only run this function on targets of NBAs, so there should be at least one...
        UASSERT_OBJ(!nbaRefs.empty(), vscp, "Did not record NBA write");
        // If no blocking upadte, then we are good
        ଯଦି (blkRefs.empty()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // If the blocking assignment is in non-combinational logic (i.e.:
        // in logic that has an explicit trigger), then we can safely
        // implement it (there is no race between clocked logic and post
        // scheduled logic), so need not error
        blkRefs.erase(std::remove_if(blkRefs.begin(), blkRefs.end(),
                                     [](ଅଚଳ Ref& ref) { ଫେରନ୍ତୁ ref.m_inNonComb; }),
                      blkRefs.end());

        // If nothing left, then we need not error
        ଯଦି (blkRefs.empty()) ଫେରନ୍ତୁ ସତ୍ୟ;

        // If not a packed variable, warn here as we can't prove independence
        ଯଦି (!isIntegralOrPacked) {
            ଅଚଳ Ref& blkRef = blkRefs.front();
            ଅଚଳ Ref& nbaRef = nbaRefs.front();
            vscp->v3warn(
                BLKANDNBLK,
                "Unsupported: Blocking and non-blocking assignments to same non-packed variable: "
                    << vscp->varp()->prettyNameQ() << '\n'
                    << vscp->warnContextPrimary() << '\n'
                    << blkRef.m_refp->warnOther() << "... Location of blocking assignment\n"
                    << blkRef.m_refp->warnContextSecondary() << '\n'
                    << nbaRef.m_refp->warnOther() << "... Location of nonblocking assignment\n"
                    << nbaRef.m_refp->warnContextSecondary());
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }

        // We need to error if we can't prove the written bits are independent

        // Sort refs by interval
        ଅଚଳ auto lessThanRef = [](ଅଚଳ Ref& a, ଅଚଳ Ref& b) {
            ଯଦି (a.m_lsb != b.m_lsb) ଫେରନ୍ତୁ a.m_lsb < b.m_lsb;
            ଫେରନ୍ତୁ a.m_msb < b.m_msb;
        };
        std::stable_sort(blkRefs.begin(), blkRefs.end(), lessThanRef);
        std::stable_sort(nbaRefs.begin(), nbaRefs.end(), lessThanRef);
        // Iterate both vectors, checking for overlap
        auto bIt = blkRefs.begin();
        auto nIt = nbaRefs.begin();
        ଯେତେବେଳେ (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            ଯଦି (lessThanRef(*bIt, *nIt)) {
                ଯଦି (nIt->m_lsb <= bIt->m_msb) ଭାଙ୍ଗ;  // Stop on Overlap
                ++bIt;
            } ଅନ୍ୟଥା {
                ଯଦି (bIt->m_lsb <= nIt->m_msb) ଭାଙ୍ଗ;  // Stop on Overlap
                ++nIt;
            }
        }

        // If we found an overlapping range that cannot be safely implemented, then wran...
        ଯଦି (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            ଅଚଳ Ref& blkRef = *bIt;
            ଅଚଳ Ref& nbaRef = *nIt;
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

        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Choose the NBA scheme used for the given variable.
    Scheme chooseScheme(ଅଚଳ AstVarScope* vscp, ଅଚଳ VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::Undecided, vscp, "NBA scheme already decided");

        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = vscp->dtypep()->skipRefp();
        // Unpacked arrays
        ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ uaDTypep = VN_CAST(dtypep, UnpackArrayDType)) {
            // If whole array is target of NBA, use ShadowVar, but not in a suspendable process or
            // fork, which resume outside the 'nba' region, so its 'pre' logic would overwrite the
            // value they write in the shadow variable
            ଯଦି (vscpInfo.m_whole) {
                ଫେରନ୍ତୁ vscpInfo.m_inSuspOrFork ? Scheme::FlagUnique : Scheme::ShadowVar;
            }
            // Basic underlying type of elements, if any.
            ଅଚଳ AstBasicDType* ଅଚଳ basicp = uaDTypep->basicp();
            // If used in a loop, we must have a dynamic commit queue. (Also works in suspendables)
            ଯଦି (vscpInfo.m_inLoop) {
                // Arrays with compound element types are currently not supported in loops
                ଯଦି (!basicp
                    || !(basicp->isIntegralOrPacked()  //
                         || basicp->isDouble()  //
                         || basicp->isString())) {
                    ଫେରନ୍ତୁ Scheme::UnsupportedCompoundArrayInLoop;
                }
                ଯଦି (vscpInfo.m_partial) ଫେରନ୍ତୁ Scheme::ମୂଲ୍ୟ_Queue_Partial;
                ଫେରନ୍ତୁ Scheme::ମୂଲ୍ୟ_Queue_Whole;
            }
            // A suspendable process or fork can resume outside the 'nba' region, so the 'pre'
            // logic of the other schemes would undo its updates. The unique flag scheme has none.
            ଯଦି (vscpInfo.m_inSuspOrFork) ଫେରନ୍ତୁ Scheme::FlagUnique;
            // Otherwise if an array of packed/basic elements, use the shared flag scheme
            ଯଦି (basicp) ଫେରନ୍ତୁ Scheme::FlagShared;
            // Finally fall back on the shadow variable scheme, e.g. for
            // arrays of unpacked structs. This will be slow.
            // TODO: generic LHS scheme as discussed in #5092
            ଫେରନ୍ତୁ Scheme::ShadowVar;
        }

        // Likewise, the 'pre' logic of the shadow variable scheme would undo the updates of a
        // suspendable process or fork, so use the unique flag scheme
        ଯଦି (vscpInfo.m_inSuspOrFork) ଫେରନ୍ତୁ Scheme::FlagUnique;

        ଅଚଳ ତାର୍କିକ isIntegralOrPacked = dtypep->isIntegralOrPacked();
        // Check for mixed usage (this also warns if not OK)
        ଯଦି (checkMixedUsage(vscp, isIntegralOrPacked)) {
            // If it's a variable updated by both blocking and non-blocking
            // assignments, use the ShadowVarMasked schem if masked update is
            // possible. This can handle blocking and non-blocking updates to
            // inpdendent parts correctly at run-time, and always works, even
            // in loops or other dynamic context.
            ଯଦି (isIntegralOrPacked) ଫେରନ୍ତୁ Scheme::ShadowVarMasked;
            // If it's inside a loop, use Scheme::ShadowVar, which is safe,
            // but will generate incorrect code if a partial update is used
            ଯଦି (vscpInfo.m_inLoop) ଫେରନ୍ତୁ Scheme::ShadowVar;
            // Otherwise (for not packed variables), use the FlagUnique scheme,
            // which at least handles partial updates correctly, but might break
            // in loops or other dynamic context
            ଫେରନ୍ତୁ Scheme::FlagUnique;
        }

        // Otherwise use the simple shadow variable scheme
        ଫେରନ୍ତୁ Scheme::ShadowVar;
    }

    // Given an expression 'exprp', return a new expression that always evaluates to the
    // value of the given expression at this point in the program. That is:
    // - If given a non-constant expression, create a new temporary AstVarScope under the given
    //   'scopep', with the given 'name', assign the expression to it, and return a read reference
    //   to the new AstVarScope.
    // - If given a constant, just return that constant.
    // New statements are inserted before 'insertp'
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* captureVal(AstScope* ଅଚଳ scopep, Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ insertp,
                            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp, ଅଚଳ std::string& name) {
        UASSERT_OBJ(!exprp->backp(), exprp, "Should have been unlinked");
        File_ଧାଡ଼ି* ଅଚଳ flp = exprp->fileline();
        ଯଦି (VN_IS(exprp, Const)) ଫେରନ୍ତୁ exprp;
        // TODO: there are some const variables that could be simply referenced here
        AstVarScope* ଅଚଳ tmpVscp = m_dlyTmps.make(flp, scopep, exprp->dtypep(), name);
        insertp->addHereThisAsNext(
            ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, tmpVscp, VAccess::WRITE}, exprp});
        ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, tmpVscp, VAccess::READ};
    };

    // Similar to 'captureVal', but captures an LValue expression. That is, the returned
    // expression will reference the same location as the input expression, at this point in the
    // program.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* captureLhs(AstScope* ଅଚଳ scopep, Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ insertp,
                            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp, ଅଚଳ std::string& baseName) {
        UASSERT_OBJ(!lhsp->backp(), lhsp, "Should have been unlinked");
        // Running node pointer
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep = lhsp;
        // Capture AstSel indices - there should be only one
        ଯଦି (AstSel* ଅଚଳ selp = VN_CAST(nodep, Sel)) {
            ଅଚଳ std::string tmpName{"Lsb" + baseName};
            selp->lsbp(captureVal(scopep, insertp, selp->lsbp()->unlinkFrBack(), tmpName));
            // Continue with target
            nodep = selp->fromp();
        }
        UASSERT_OBJ(!VN_IS(nodep, Sel), lhsp, "Multiple 'AstSel' applied to LHS reference");
        // Capture the indices of selects - might be many, also below struct members. If a handle
        // selects the instance the target is a member of, capture the handle instead, as it is
        // only read, to select the target when the NBA is executed (IEEE 1800-2023 10.4.2).
        size_t nArraySels = 0;
        ଯେତେବେଳେ (ସତ୍ୟ) {
            ଯଦି (Ast_ଗଣ୍ଠି_Sel* ଅଚଳ selp = VN_CAST(nodep, ଗଣ୍ଠି_Sel)) {
                ଅଚଳ std::string tmpName{"Dim" + std::to_string(nArraySels++) + baseName};
                selp->bitp(captureVal(scopep, insertp, selp->bitp()->unlinkFrBack(), tmpName));
                nodep = selp->fromp();
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstStructSel* ଅଚଳ selp = VN_CAST(nodep, StructSel)) {
                nodep = selp->fromp();
            } ଅନ୍ୟଥା ଯଦି (AstMemberSel* ଅଚଳ selp = VN_CAST(nodep, MemberSel)) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ handlep = selp->fromp()->unlinkFrBack();
                V3LinkLValue::linkLValueSet(handlep, VAccess::READ);
                AstVarRef* ଅଚଳ refp
                    = VN_AS(captureVal(scopep, insertp, handlep, "Handle" + baseName), VarRef);
                refp->varp()->setIgnorePostWrite();
                // Like the handle it replaces, mark it written, as is the member it selects
                refp->access(selp->access());
                selp->fromp(refp);
                ଫେରନ୍ତୁ lhsp;
            } ଅନ୍ୟଥା {
                ଭାଙ୍ଗ;
            }
        }
        // What remains must be the variable, or some sort of select, we assume can reuse it
        UASSERT_OBJ(nodep->isPure(), lhsp, "Malformed LHS in NBA");
        // Now have been converted to use the captured values
        ଫେରନ୍ତୁ lhsp;
    }

    ଶୂନ୍ୟ add_CFunc_Cached_ମୂଲ୍ୟଗୁଡ଼ିକ(ଅଚଳ AstCFunc* ଅଚଳ cfuncp,
                              std::unordered_set<ଅଚଳ AstCFunc*>& visited) {
        ଯଦି (!visited.ଭର୍ତ୍ତି(cfuncp).second) ଫେରନ୍ତୁ;
        CFuncCache& ମୂଲ୍ୟ = m_cfuncsCache(cfuncp);
        m_timingDomains.ଭର୍ତ୍ତି(ମୂଲ୍ୟ.m_timingDomains.begin(), ମୂଲ୍ୟ.m_timingDomains.end());
        m_non_Inlined_CAwaits_With_Sen_ବୃକ୍ଷ += ମୂଲ୍ୟ.m_timingDomains.size();
        ପାଇଁ (ଅଚଳ AstCFunc* ଅଚଳ includedp : ମୂଲ୍ୟ.m_includes) {
            add_CFunc_Cached_ମୂଲ୍ୟଗୁଡ଼ିକ(includedp, visited);
        }
    }

    // Create a temporary variable in the given 'scopep', with the given 'name', and with 'dtypep'
    // type, with the bits selected by 'sLsbp'/'sWidthp' set to 'valuep', other bits set to zero.
    // Insert new statements before 'insertp'.
    // Returns a read reference to the temporary variable.
    AstVarRef* createWidened(File_ଧାଡ଼ି* flp, AstScope* scopep, Ast_ଗଣ୍ଠି_DType* dtypep,
                             Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* sLsbp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା sWidth, ଅଚଳ std::string& name,
                             Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep, Ast_ଗଣ୍ଠି* insertp) {
        // Create temporary variable.
        AstVarScope* ଅଚଳ tp = m_dlyTmps.make(flp, scopep, dtypep, name);
        // Zero it
        AstConst* ଅଚଳ zerop = ନୂତନ AstConst{flp, AstConst::DTyped{}, dtypep};
        zerop->ଅଙ୍କ().setAllBits0();
        insertp->addHereThisAsNext(
            ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, tp, VAccess::WRITE}, zerop});
        // Set the selected bits to 'valuep'
        AstSel* ଅଚଳ selp = ନୂତନ AstSel{flp, ନୂତନ AstVarRef{flp, tp, VAccess::WRITE},
                                        sLsbp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ), sWidth};
        insertp->addHereThisAsNext(ନୂତନ AstAssign{flp, selp, valuep});
        // This is the expression to get the value of the temporary
        ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, tp, VAccess::READ};
    }

    // Scheme::ShadowVar
    ଶୂନ୍ୟ prepareSchemeShadowVar(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
        AstScope* ଅଚଳ scopep = vscp->scopep();
        // Create the shadow variable
        ଅଚଳ std::string name = "_" + vscp->varp()->shortName();
        AstVarScope* ଅଚଳ shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), name);
        vscpInfo.shadowVariableKit().vscp = shadowVscp;
        // Mark both for V3LifePsot
        vscp->optimizeLifePost(ସତ୍ୟ);
        shadowVscp->optimizeLifePost(ସତ୍ୟ);
        // Create the AstActive for the Pre/Post logic
        AstActive* ଅଚଳ activep = ନୂତନ AstActive{flp, "nba-shadow-variable", vscpInfo.senTreep()};
        activep->sen_ବୃକ୍ଷ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Pre' scheduled 'shadowVariable = originalVariable' assignment
        AstAlwaysPre* ଅଚଳ prep = ନୂତନ AstAlwaysPre{flp};
        activep->addStmtsp(prep);
        prep->addStmtsp(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, shadowVscp, VAccess::WRITE},
                                      ନୂତନ AstVarRef{flp, vscp, VAccess::READ}});
        // Add 'Post' scheduled 'originalVariable = shadowVariable' assignment
        AstAlwaysPost* ଅଚଳ postp = ନୂତନ AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        postp->addStmtsp(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, vscp, VAccess::WRITE},
                                       ନୂତନ AstVarRef{flp, shadowVscp, VAccess::READ}});
    }
    ଶୂନ୍ୟ convertSchemeShadowVar(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        AstVarScope* ଅଚଳ shadowVscp = vscpInfo.shadowVariableKit().vscp;

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ଅଚଳ refp) {
            ଯଦି (!refp->access().isWriteOnly()) ଫେରନ୍ତୁ;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
    }

    // Scheme::ShadowVarMasked
    ଶୂନ୍ୟ prepareSchemeShadowVarMasked(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
        AstScope* ଅଚଳ scopep = vscp->scopep();
        // Create the shadow variable
        ଅଚଳ std::string shadowName = "_" + vscp->varp()->shortName();
        AstVarScope* ଅଚଳ shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), shadowName);
        vscpInfo.shadowVarMaskedKit().vscp = shadowVscp;
        // Create the makk variable
        ଅଚଳ std::string maskName = "Mask__" + vscp->varp()->shortName();
        AstVarScope* ଅଚଳ maskVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), maskName);
        maskVscp->varp()->setIgnorePostWrite();
        vscpInfo.shadowVarMaskedKit().maskp = maskVscp;
        // Create the AstActive for the Post logic
        AstActive* ଅଚଳ activep
            = ନୂତନ AstActive{flp, "nba-shadow-var-masked", vscpInfo.senTreep()};
        activep->sen_ବୃକ୍ଷ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit and mask clear
        AstAlwaysPost* ଅଚଳ postp = ନୂତନ AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit - vscp = (shadowVscp & maskVscp) | (vscp & ~maskVscp);
        postp->addStmtsp(ନୂତନ AstAssign{
            flp, ନୂତନ AstVarRef{flp, vscp, VAccess::WRITE},
            ନୂତନ AstOr{flp,
                      ନୂତନ AstAnd{flp, ନୂତନ AstVarRef{flp, shadowVscp, VAccess::READ},
                                 ନୂତନ AstVarRef{flp, maskVscp, VAccess::READ}},
                      ନୂତନ AstAnd{flp, ନୂତନ AstVarRef{flp, vscp, VAccess::READ},
                                 ନୂତନ AstNot{flp, ନୂତନ AstVarRef{flp, maskVscp, VAccess::READ}}}}});
        vscp->varp()->setIgnorePostRead();
        // Clar the mask - maskVscp = '0;
        postp->addStmtsp(
            ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, maskVscp, VAccess::WRITE},
                          ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, maskVscp->width(), 0}});
    }
    ଶୂନ୍ୟ convertSchemeShadowVarMasked(AstAssignDly* nodep, AstVarScope* vscp,
                                      VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        AstVarScope* ଅଚଳ shadowVscp = vscpInfo.shadowVarMaskedKit().vscp;
        AstVarScope* ଅଚଳ maskVscp = vscpInfo.shadowVarMaskedKit().maskp;

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsClonep = nodep->lhsp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ଅଚଳ refp) {
            ଯଦି (!refp->access().isWriteOnly()) ଫେରନ୍ତୁ;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
        // Set the same bits in the mask to 1
        lhsClonep->foreach([&](AstVarRef* ଅଚଳ refp) {
            ଯଦି (!refp->access().isWriteOnly()) ଫେରନ୍ତୁ;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(maskVscp);
            refp->varp(maskVscp->varp());
        });
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        AstConst* ଅଚଳ onesp = ନୂତନ AstConst{flp, AstConst::DTyped{}, lhsClonep->dtypep()};
        onesp->ଅଙ୍କ().setAllBits1();
        nodep->addNextHere(ନୂତନ AstAssign{flp, lhsClonep, onesp});
    }

    // Scheme::FlagShared
    ଶୂନ୍ୟ prepareSchemeFlagShared(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
        AstScope* ଅଚଳ scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* ଅଚଳ activep = ନୂତନ AstActive{flp, "nba-flag-shared", vscpInfo.senTreep()};
        activep->sen_ବୃକ୍ଷ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        vscpInfo.flagSharedKit().activep = activep;
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ଅଚଳ postp = ନୂତନ AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagSharedKit().postp = postp;
        // Initialize
        vscpInfo.flagSharedKit().commitFlagp = nullptr;
        vscpInfo.flagSharedKit().commitIfp = nullptr;
    }
    ଶୂନ୍ୟ convertSchemeFlagShared(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
        AstScope* ଅଚଳ scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ଅଚଳ std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Is this NBA adjacent after the previously processed NBA?
        ଅଚଳ ତାର୍କିକ consecutive = nodep == m_nextDlyp;
        m_nextDlyp = VN_CAST(nodep->nextp(), AssignDly);

        VarScopeInfo* ଅଚଳ prevVscpInfop = consecutive ? &m_vscpInfo(m_prevVscp) : nullptr;

        // We can reuse the flag of the previous assignment if:
        ଅଚଳ ତାର୍କିକ reuseTheFlag =
            // Consecutive NBAs
            consecutive
            // ... that use the same scheme
            && prevVscpInfop->m_scheme == Scheme::FlagShared
            // ... and are in the same scope as the target variable
            && scopep == vscp->scopep()
            // ... and share the same overall update domain
            && prevVscpInfop->senTreep()->same_ବୃକ୍ଷ(vscpInfo.senTreep());

        ଯଦି (!reuseTheFlag) {
            // Create new flag
            AstVarScope* ଅଚଳ flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
            // Set the flag at the original NBA
            nodep->addHereThisAsNext(  //
                ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, flagVscp, VAccess::WRITE},
                              ନୂତନ AstConst{flp, AstConst::BitTrue{}}});
            // Add the 'Pre' scheduled reset for the flag
            AstAlwaysPre* ଅଚଳ prep = ନୂତନ AstAlwaysPre{flp};
            vscpInfo.flagSharedKit().activep->addStmtsp(prep);
            prep->addStmtsp(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, flagVscp, VAccess::WRITE},
                                          ନୂତନ AstConst{flp, AstConst::BitFalse{}}});
            // Add the 'Post' scheduled commit
            AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, ନୂତନ AstVarRef{flp, flagVscp, VAccess::READ}};
            vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
            vscpInfo.flagSharedKit().commitFlagp = flagVscp;
            vscpInfo.flagSharedKit().commitIfp = ifp;
        } ଅନ୍ୟଥା {
            ଯଦି (vscp != m_prevVscp) {
                // Different variable, ensure the commit block exists for this variable,
                // can reuse existing one with the same flag, otherwise create a new one.
                AstVarScope* ଅଚଳ flagVscp = prevVscpInfop->flagSharedKit().commitFlagp;
                UASSERT_OBJ(flagVscp, nodep, "Commit flag of previous assignment should exist");
                ଯଦି (vscpInfo.flagSharedKit().commitFlagp != flagVscp) {
                    AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, ନୂତନ AstVarRef{flp, flagVscp, VAccess::READ}};
                    vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
                    vscpInfo.flagSharedKit().commitFlagp = flagVscp;
                    vscpInfo.flagSharedKit().commitIfp = ifp;
                }
            } ଅନ୍ୟଥା {
                // Same variable, reuse the commit block of the previous assignment
                vscpInfo.flagSharedKit().commitFlagp = prevVscpInfop->flagSharedKit().commitFlagp;
                vscpInfo.flagSharedKit().commitIfp = prevVscpInfop->flagSharedKit().commitIfp;
            }
            ++m_nSharedSetFlags;
        }
        // Commit the captured value to the captured destination
        vscpInfo.flagSharedKit().commitIfp->addThensp(
            ନୂତନ AstAssign{flp, capturedLhsp, capturedRhsp});

        // Remember the variable for the next NBA
        m_prevVscp = vscp;

        // Delete original NBA
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::FlagUnique
    ଶୂନ୍ୟ prepareSchemeFlagUnique(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
        AstScope* ଅଚଳ scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* ଅଚଳ activep = ନୂତନ AstActive{flp, "nba-flag-unique", vscpInfo.senTreep()};
        activep->sen_ବୃକ୍ଷ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ଅଚଳ postp = ନୂତନ AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagUniqueKit().postp = postp;
    }
    ଶୂନ୍ୟ convertSchemeFlagUnique(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
        AstScope* ଅଚଳ scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ଅଚଳ std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Create new flag
        AstVarScope* ଅଚଳ flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
        flagVscp->varp()->setIgnorePostWrite();
        // Set the flag at the original NBA
        nodep->addHereThisAsNext(  //
            ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, flagVscp, VAccess::WRITE},
                          ନୂତନ AstConst{flp, AstConst::BitTrue{}}});
        // Add the 'Post' scheduled commit
        AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, ନୂତନ AstVarRef{flp, flagVscp, VAccess::READ}};
        vscpInfo.flagUniqueKit().postp->addStmtsp(ifp);
        // Immediately clear the flag
        ifp->addThensp(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, flagVscp, VAccess::WRITE},
                                     ନୂତନ AstConst{flp, AstConst::BitFalse{}}});
        // Commit the value
        ifp->addThensp(ନୂତନ AstAssign{flp, capturedLhsp, capturedRhsp});

        // Delete original NBA
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::ValueQueuePartial/Scheme::ValueQueueWhole
    template <ତାର୍କିକ N_Partial>
    ଶୂନ୍ୟ prepare_Scheme_ମୂଲ୍ୟ_Queue(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(N_Partial ? vscpInfo.m_scheme == Scheme::ମୂଲ୍ୟ_Queue_Partial
                              : vscpInfo.m_scheme == Scheme::ମୂଲ୍ୟ_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
        AstScope* ଅଚଳ scopep = vscp->scopep();

        // Create the commit queue variable
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDTypep = vscp->dtypep()->skipRefp();
        AstNBACommitQueueDType*& cqDTypep = m_cqDTypeps[{elemDTypep, N_Partial}];
        ଯଦି (!cqDTypep) {
            cqDTypep = ନୂତନ AstNBACommitQueueDType{flp, elemDTypep, N_Partial};
            v3Global.rootp()->typeTablep()->addTypesp(cqDTypep);
        }
        ଅଚଳ std::string name = "CommitQueue__" + vscp->varp()->shortName();
        AstVarScope* ଅଚଳ queueVscp = m_dlyTmps.make(flp, scopep, cqDTypep, name);
        queueVscp->varp()->noReset(ସତ୍ୟ);
        queueVscp->varp()->setIgnorePostWrite();
        vscpInfo.ମୂଲ୍ୟ_Queue_Kit().vscp = queueVscp;
        // Create the AstActive for the Post logic
        AstActive* ଅଚଳ activep
            = ନୂତନ AstActive{flp, "nba-value-queue-whole", vscpInfo.senTreep()};
        activep->sen_ବୃକ୍ଷ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit
        AstAlwaysPost* ଅଚଳ postp = ନୂତନ AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit
        AstCMethodHard* ଅଚଳ callp = ନୂତନ AstCMethodHard{
            flp, ନୂତନ AstVarRef{flp, queueVscp, VAccess::READWRITE}, VCMethod::NBA_COMMIT};
        callp->dtypeSetVoid();
        // TODO: this is a partial update, so must be READWRITE, but that breaks scheduling
        callp->addPinsp(ନୂତନ AstVarRef{flp, vscp, VAccess::WRITE});
        postp->addStmtsp(callp->makeStmt());
    }

    ଶୂନ୍ୟ convert_Scheme_ମୂଲ୍ୟ_Queue(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo,
                                 ତାର୍କିକ partial) {
        UASSERT_OBJ(partial ? vscpInfo.m_scheme == Scheme::ମୂଲ୍ୟ_Queue_Partial
                            : vscpInfo.m_scheme == Scheme::ମୂଲ୍ୟ_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
        AstScope* ଅଚଳ scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ଅଚଳ std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // RHS value (can be widened/masked iff Partial)
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = capturedRhsp;
        // RHS mask (iff Partial)
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* maskp = nullptr;

        // Running node for LHS deconstruction
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsNodep = capturedLhsp;

        // If partial updates are required, construct the mask and the widened value
        ଯଦି (partial) {
            // Type of array element
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ eDTypep = [&]() -> Ast_ଗଣ୍ଠି_DType* {
                Ast_ଗଣ୍ଠି_DType* dtypep = vscp->dtypep()->skipRefp();
                ଯେତେବେଳେ (AstUnpackArrayDType* ଅଚଳ ap = VN_CAST(dtypep, UnpackArrayDType)) {
                    dtypep = ap->subDTypep()->skipRefp();
                }
                ଫେରନ୍ତୁ dtypep;
            }();

            ଯଦି (ଅଚଳ AstSel* ଅଚଳ lSelp = VN_CAST(lhsNodep, Sel)) {
                // This is a partial assignment.
                // Need to create a mask and widen the value to element size.
                lhsNodep = lSelp->fromp();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ sLsbp = lSelp->lsbp();
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା sWidth = lSelp->widthConst();

                // Create mask value
                maskp = [&]() -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
                    // Constant mask we can compute here
                    ଯଦି (ଅଚଳ AstConst* ଅଚଳ cLsbp = VN_CAST(sLsbp, Const)) {
                        AstConst* ଅଚଳ cp = ନୂତନ AstConst{flp, AstConst::DTyped{}, eDTypep};
                        cp->ଅଙ୍କ().setMask(sWidth, cLsbp->toSInt());
                        ଫେରନ୍ତୁ cp;
                    }

                    // A non-constant mask we must compute at run-time.
                    AstConst* ଅଚଳ onesp = ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, sWidth, 0};
                    onesp->ଅଙ୍କ().setAllBits1();
                    ଫେରନ୍ତୁ createWidened(flp, scopep, eDTypep, sLsbp, sWidth, "Mask" + baseName,
                                         onesp, nodep);
                }();

                // Adjust value to element size
                valuep = [&]() -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
                    // Constant value with constant select we can compute here
                    ଯଦି (AstConst* ଅଚଳ cValuep = VN_CAST(valuep, Const)) {
                        ଯଦି (ଅଚଳ AstConst* ଅଚଳ cLsbp = VN_CAST(sLsbp, Const)) {
                            AstConst* ଅଚଳ cp = ନୂତନ AstConst{flp, AstConst::DTyped{}, eDTypep};
                            cp->ଅଙ୍କ().setAllBits0();
                            cp->ଅଙ୍କ().opSelInto(cValuep->ଅଙ୍କ(), cLsbp->toSInt(), sWidth);
                            VL_DO_DANGLING(valuep->delete_ବୃକ୍ଷ(), valuep);
                            ଫେରନ୍ତୁ cp;
                        }
                    }

                    // A non-constant value we must adjust.
                    ଫେରନ୍ତୁ createWidened(flp, scopep, eDTypep, sLsbp, sWidth,  //
                                         "Elem" + baseName, valuep, nodep);
                }();
            } ଅନ୍ୟଥା {
                // If this assignment is not partial, set mask to ones and we are done
                AstConst* ଅଚଳ ones = ନୂତନ AstConst{flp, AstConst::DTyped{}, eDTypep};
                ones->ଅଙ୍କ().setAllBits1();
                maskp = ones;
            }
        }

        // Extract array indices
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> idxps;
        {
            UASSERT_OBJ(VN_IS(lhsNodep, ArraySel), lhsNodep, "Unexpected LHS form");
            ଯେତେବେଳେ (AstArraySel* ଅଚଳ aSelp = VN_CAST(lhsNodep, ArraySel)) {
                idxps.emplace_back(aSelp->bitp()->unlinkFrBack());
                lhsNodep = aSelp->fromp();
            }
            UASSERT_OBJ(VN_IS(lhsNodep, VarRef), lhsNodep, "Unexpected LHS form");
            std::reverse(idxps.begin(), idxps.end());
        }

        // Done with the LHS at this point
        VL_DO_DANGLING(ଠେଲ_Deletep(capturedLhsp), capturedLhsp);

        // Enqueue the update at the site of the original NBA
        AstCMethodHard* ଅଚଳ callp = ନୂତନ AstCMethodHard{
            flp, ନୂତନ AstVarRef{flp, vscpInfo.ମୂଲ୍ୟ_Queue_Kit().vscp, VAccess::READWRITE},
            VCMethod::NBA_ENQUEUE};
        callp->dtypeSetVoid();
        callp->addPinsp(valuep);
        ଯଦି (partial) callp->addPinsp(maskp);
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ indexp : idxps) callp->addPinsp(indexp);
        nodep->addHereThisAsNext(callp->makeStmt());

        // Delete original NBA
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Record where a variable is assigned
    ଶୂନ୍ୟ recordWriteRef(AstVarRef* nodep, ତାର୍କିକ nonBlocking) {
        // Ignore references in certain contexts
        ଯଦି (m_ignoreBlkAndNBlk) ଫେରନ୍ତୁ;
        // Ignore if it's an array
        // TODO: we do this because it used to be the previous behaviour.
        //       Is it still required, or should we warn for arrays as well?
        //       Scheduling is no different for them...
        //       Clarification: This is OK for arrays of primitive types, but
        //       arrays that use the ShadowVar scheme don't work...
        ଯଦି (VN_IS(nodep->varScopep()->dtypep()->skipRefp(), UnpackArrayDType)) ଫେରନ୍ତୁ;

        m_writeRefs(nodep->varScopep()).emplace_back(nodep, nonBlocking, m_inNonCombLogic);
    }

    template <typename Procedure_T>
    ସ୍ଥିର ତାର୍କିକ isProcedureWithSentreep(ଅଚଳ Ast_ଗଣ୍ଠି_Procedure* ଅଚଳ nodep) {
        ଅଚଳ Procedure_T* ଅଚଳ procedurep = Ast_ଗଣ୍ଠି::cast<Procedure_T>(nodep);
        ଫେରନ୍ତୁ procedurep && procedurep->sentreep();
    }

    // Visit AstCFunc from a AstNodeCCall - this is made into a separate quasi-visitor because
    // AstCFunc that is not called from the code (e.g.: DPI exports) does not need to be visited
    // this way. Also, not visiting such AstCFuncs allows to avoid caching results for them which
    // this function does - which could lead to excessive memory usage
    ଶୂନ୍ୟ visitCalledCFunc(AstCFunc* ଅଚଳ nodep) {
        CFuncCache& ମୂଲ୍ୟ = m_cfuncsCache(nodep);
        ବାଛ (ମୂଲ୍ୟ.m_state) {
        ସ୍ଥିତି CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_timingDomains);

            // Visit
            ମୂଲ୍ୟ.m_state = CFuncCache::VISITING;
            m_call_ଗଦା.ଠେଲ_back(nodep);
            {
                VL_RESTORER(m_cfuncp);
                m_cfuncp = nodep;
                iterateChildren(nodep);
            }
            m_call_ଗଦା.କାଢ଼_back();
            ମୂଲ୍ୟ.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_timingDomains, ମୂଲ୍ୟ.m_timingDomains);
        } ଭାଙ୍ଗ;
        ସ୍ଥିତି CFuncCache::VISITING: {
            ପାଇଁ (size_t i = m_call_ଗଦା.size() - 1; m_call_ଗଦା.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ଗଦା[i]).m_includes.ଭର୍ତ୍ତି(nodep);
            }
            ଫେରନ୍ତୁ;  // Break recursion
        }
        ସ୍ଥିତି CFuncCache::INITIALIZED: ଭାଙ୍ଗ;
        }
        std::unordered_set<ଅଚଳ AstCFunc*> visited;
        add_CFunc_Cached_ମୂଲ୍ୟଗୁଡ଼ିକ(nodep, visited);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override {
        iterateChildren(nodep);
        // Decide which scheme to use for each variable and do the 'prepare' step
        ପାଇଁ (AstVarScope* ଅଚଳ vscp : m_vscps) {
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            vscpInfo.m_scheme = chooseScheme(vscp, vscpInfo);
            // Run 'prepare' step
            ବାଛ (vscpInfo.m_scheme) {
            ସ୍ଥିତି Scheme::Undecided:  // LCOV_EXCL_START
                UASSERT_OBJ(ମିଥ୍ୟା, vscp, "Failed to choose NBA scheme");
                ଭାଙ୍ଗ;  // LCOV_EXCL_STOP
            ସ୍ଥିତି Scheme::UnsupportedCompoundArrayInLoop: {
                // Will report error at the site of the NBA
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି Scheme::ShadowVar: {
                ++m_nSchemeShadowVar;
                prepareSchemeShadowVar(vscp, vscpInfo);
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି Scheme::ShadowVarMasked: {
                ++m_nSchemeShadowVarMasked;
                prepareSchemeShadowVarMasked(vscp, vscpInfo);
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି Scheme::FlagShared: {
                ++m_nSchemeFlagShared;
                prepareSchemeFlagShared(vscp, vscpInfo);
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି Scheme::FlagUnique: {
                ++m_nSchemeFlagUnique;
                prepareSchemeFlagUnique(vscp, vscpInfo);
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି Scheme::ମୂଲ୍ୟ_Queue_Whole: {
                ++m_n_Scheme_ମୂଲ୍ୟ_Queues_Whole;
                prepare_Scheme_ମୂଲ୍ୟ_Queue</* Partial: */ ମିଥ୍ୟା>(vscp, vscpInfo);
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି Scheme::ମୂଲ୍ୟ_Queue_Partial: {
                ++m_n_Scheme_ମୂଲ୍ୟ_Queues_Partial;
                prepare_Scheme_ମୂଲ୍ୟ_Queue</* Partial: */ ସତ୍ୟ>(vscp, vscpInfo);
                ଭାଙ୍ଗ;
            }
            }
        }
        // Convert all NBAs
        ପାଇଁ (ଅଚଳ NBA& nba : m_nbas) {
            AstAssignDly* ଅଚଳ nbap = nba.nodep;
            AstVarScope* ଅଚଳ vscp = nba.vscp;
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            // Run 'convert' step
            ବାଛ (vscpInfo.m_scheme) {
            ସ୍ଥିତି Scheme::Undecided: {  // LCOV_EXCL_START
                UASSERT_OBJ(ମିଥ୍ୟା, vscp, "Unreachable");
                ଭାଙ୍ଗ;
            }  // LCOV_EXCL_STOP
            ସ୍ଥିତି Scheme::UnsupportedCompoundArrayInLoop: {
                // TODO: make this an E_UNSUPPORTED...
                nbap->v3warn(BLKLOOPINIT, "Unsupported: Non-blocking assignment to array with "
                                          "compound element type inside loop");
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି Scheme::ShadowVar: {
                convertSchemeShadowVar(nbap, vscp, vscpInfo);
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି Scheme::ShadowVarMasked: {
                convertSchemeShadowVarMasked(nbap, vscp, vscpInfo);
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି Scheme::FlagShared: {
                convertSchemeFlagShared(nbap, vscp, vscpInfo);
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି Scheme::FlagUnique: {
                convertSchemeFlagUnique(nbap, vscp, vscpInfo);
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି Scheme::ମୂଲ୍ୟ_Queue_Whole: {
                convert_Scheme_ମୂଲ୍ୟ_Queue(nbap, vscp, vscpInfo, /* partial: */ ମିଥ୍ୟା);
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି Scheme::ମୂଲ୍ୟ_Queue_Partial:
                convert_Scheme_ମୂଲ୍ୟ_Queue(nbap, vscp, vscpInfo, /* partial: */ ସତ୍ୟ);
                ଭାଙ୍ଗ;
            }
        }
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstActive* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ignoreBlkAndNBlk);
        VL_RESTORER(m_inNonCombLogic);
        m_activep = nodep;
        ଅଚଳ Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep = nodep->sentreep();
        m_ignoreBlkAndNBlk = senTreep->hasStatic() || senTreep->hasInitial();
        m_inNonCombLogic = senTreep->hasClocked();
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override {
        VL_RESTORER(m_needsInitialTrigger);
        VL_RESTORER_CLEAR(m_nbaEventSenTreeps);
        ଅଚଳ size_t firstNBAAddedIndex = m_nbas.size();
        {
            VL_RESTORER(m_inSuspendableOrFork);
            VL_RESTORER(m_procp);
            VL_RESTORER(m_ignoreBlkAndNBlk);
            VL_RESTORER(m_inNonCombLogic);
            // When we are dealing with initial block we need to
            // treat it as suspendable when we meet a NBA
            m_inSuspendableOrFork = nodep->isSuspendable() || VN_IS(nodep, Initial);
            m_procp = nodep;
            ଯଦି (nodep->isSuspendable()) {
                m_ignoreBlkAndNBlk = ମିଥ୍ୟା;
                m_inNonCombLogic = ସତ୍ୟ;
            }
            iterateChildren(nodep);
        }
        auto containsClocled = [](ଅଚଳ AstSenItem* itemp) {
            ଯେତେବେଳେ (itemp) {
                ଯଦି (itemp->edgeType().clockedStmt()) ଫେରନ୍ତୁ ସତ୍ୟ;
                itemp = VN_AS(itemp->nextp(), SenItem);
            }
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        };
        ଅଚଳ ତାର୍କିକ addInitialTrigger = m_needsInitialTrigger
                                       && !(isProcedureWithSentreep<AstAlways>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysObserved>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysReactive>(nodep))
                                       && !containsClocled(m_activep->sentreep()->sensesp());
        ଯଦି (m_timingDomains.empty() && !addInitialTrigger) ଫେରନ୍ତୁ;

        // There were some timing domains involved in the process. Add all of them as sensitivities
        // of all NBA targets in this process. Note this is a bit of a sledgehammer, we should only
        // need those that directly precede the NBA in control flow, but that is hard to compute,
        // so we will hammer away.

        // First gather all senItems
        AstSenItem* senItemp = nullptr;
        ଯଦି (addInitialTrigger) {
            senItemp = ନୂତନ AstSenItem{nodep->fileline(), AstSenItem::InitialNBA{}};
            ++m_nInitialNBA;
        }

        ପାଇଁ (ଅଚଳ Ast_Sen_ବୃକ୍ଷ* ଅଚଳ domainp : m_timingDomains) {
            ଯଦି (domainp->sensesp())
                senItemp = Ast_ଗଣ୍ଠି::addNext(senItemp, domainp->sensesp()->clone_ବୃକ୍ଷ(ସତ୍ୟ));
        }
        m_timingDomains.clear();
        // Add them to all nba targets we gathered in this process
        ପାଇଁ (size_t i = firstNBAAddedIndex; i < m_nbas.size(); ++i) {
            m_vscpInfo(m_nbas[i].vscp).addSensitivity(senItemp);
        }
        ପାଇଁ (Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep : m_nbaEventSenTreeps)
            senTreep->addSensesp(senItemp->clone_ବୃକ୍ଷ(ସତ୍ୟ));
        // Done with these
        VL_DO_DANGLING(senItemp->delete_ବୃକ୍ଷ(), senItemp);
    }
    ଶୂନ୍ୟ visit(AstFork* nodep) override {
        VL_RESTORER(m_inSuspendableOrFork);
        m_inSuspendableOrFork = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCAwait* nodep) override {
        ଯଦି (nodep->sentreep()) m_timingDomains.ଭର୍ତ୍ତି(nodep->sentreep());
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstFireEvent* nodep) override {
        UASSERT_OBJ(v3Global.hasEvents(), nodep, "Inconsistent");
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ eventp = nodep->operandp()->unlinkFrBack();

        // Enqueue for clearing 'triggered' state on next eval
        AstCStmt* ଅଚଳ cstmtp = ନୂତନ AstCStmt{flp};
        cstmtp->add("vlSymsp->fireEvent(");
        cstmtp->add(eventp);
        cstmtp->add(");");

        Ast_ଗଣ୍ଠି* newp = cstmtp;
        ଅଚଳ AstVarRef* ଅଚଳ vrefp = VN_CAST(eventp, VarRef);
        ଯଦି (nodep->isDelayed() && (m_cfuncp || !vrefp)) {
            // V3Timing converts these with --timing
            nodep->v3warn(E_NOTIMING, "Nonblocking event trigger "
                                          << (m_cfuncp ? "in a non-inlined function/task"
                                                       : "of a class or interface member or "
                                                         "array element")
                                          << " requires --timing");
        } ଅନ୍ୟଥା ଯଦି (nodep->isDelayed()) {
            ଅଚଳ std::string newvarname = "_" + vrefp->varp()->shortName();
            AstVarScope* ଅଚଳ dlyvscp
                = m_dlyTmps.make(flp, vrefp->varScopep()->scopep(), 1, newvarname);

            ଅଚଳ auto dlyRef = [=](VAccess access) {  //
                ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, dlyvscp, access};
            };

            AstAlwaysPost* ଅଚଳ postp = ନୂତନ AstAlwaysPost{flp};
            AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, dlyRef(VAccess::READ)};
            postp->addStmtsp(ifp);

            UASSERT_OBJ(m_activep, nodep, "No active to handle FireEvent");
            Ast_Sen_ବୃକ୍ଷ* senTreep = m_activep->sentreep();
            AstActive* activep = nullptr;
            ଯଦି (m_inSuspendableOrFork && (senTreep->hasInitial() || senTreep->hasClocked())) {
                // Suspendable code can trigger whenever it resumes, so fire when its NBAs are
                // committed (see visit(AstNodeProcedure*)), clearing the flag like
                // Scheme::FlagUnique
                senTreep = ନୂତନ Ast_Sen_ବୃକ୍ଷ{
                    flp, senTreep->hasClocked() ? senTreep->sensesp()->clone_ବୃକ୍ଷ(ସତ୍ୟ) : nullptr};
                m_nbaEventSenTreeps.ଠେଲ_back(senTreep);
                m_needsInitialTrigger |= m_timingDomains.empty();
                dlyvscp->varp()->setIgnorePostWrite();
                ifp->addThensp(ନୂତନ AstAssign{flp, dlyRef(VAccess::WRITE),
                                             ନୂତନ AstConst{flp, AstConst::BitFalse{}}});
                activep = ନୂତନ AstActive{flp, "nba-event", senTreep};
                activep->sen_ବୃକ୍ଷ_Storep(senTreep);
            } ଅନ୍ୟଥା {
                activep = ନୂତନ AstActive{flp, "nba-event", senTreep};
                AstAlwaysPre* ଅଚଳ prep = ନୂତନ AstAlwaysPre{flp};
                prep->addStmtsp(ନୂତନ AstAssign{flp, dlyRef(VAccess::WRITE),
                                              ନୂତନ AstConst{flp, AstConst::BitFalse{}}});
                activep->addStmtsp(prep);
            }
            ifp->addThensp(newp);
            m_activep->addNextHere(activep);
            activep->addStmtsp(postp);

            newp = ନୂତନ AstAssign{flp, dlyRef(VAccess::WRITE),
                                 ନୂତନ AstConst{flp, AstConst::BitTrue{}}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstAssignDly* nodep) override {
        // Prevent double processing due to AstExprStmt being moved before this node
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;

        ଯଦି (m_cfuncp) {
            ଯଦି (!v3Global.rootp()->nbaEventp()) {
                nodep->v3warn(
                    E_NOTIMING,
                    "Delayed assignment in a non-inlined function/task requires --timing");
            }
            ଫେରନ୍ତୁ;
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
        nodep->lhsp()->foreach([&](Ast_ଗଣ୍ଠି* currp) {
            // cppcheck-suppress constVariablePointer
            ଯଦି (Ast_ପଦସମଷ୍ଟି_Stmt* ଅଚଳ exprp = VN_CAST(currp, ପଦସମଷ୍ଟି_Stmt)) {
                // Move statements before the NBA
                nodep->addHereThisAsNext(exprp->stmtsp()->unlinkFrBackWithNext());
                // Replace with result
                currp->replaceWith(exprp->resultp()->unlinkFrBack());
                // Get rid of the AstExprStmt
                VL_DO_DANGLING2(ଠେଲ_Deletep(currp), currp, exprp);
            } ଅନ୍ୟଥା ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(currp, VarRef)) {
                // Ignore reads (e.g.: '_[*here*] <= _')
                ଯଦି (refp->access().isReadOnly()) ଫେରନ୍ତୁ;
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
        AstVarScope* ଅଚଳ vscp = m_currNbaLhsRefp->varScopep();
        // Record it on first encounter
        VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
        ଯଦି (!vscpInfo.m_firstNbaRefp) {
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
        ଯଦି (m_activep->sentreep()->hasClocked()) {
            ଯଦି (vscpInfo.m_fistActivep != m_activep) {
                AstVar* ଅଚଳ varp = vscp->varp();
                ଯଦି (!varp->user1SetOnce()
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
        recordWriteRef(m_currNbaLhsRefp, ସତ୍ୟ);

        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        // Already checked the NBA LHS ref, ignore here
        ଯଦି (nodep == m_currNbaLhsRefp) ଫେରନ୍ତୁ;
        // Only care about write refs
        ଯଦି (!nodep->access().isWriteOrRW()) ଫେରନ୍ତୁ;
        // Record write reference
        recordWriteRef(nodep, ମିଥ୍ୟା);
    }
    ଶୂନ୍ୟ visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* ଅଚଳ nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ଅଚଳ auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        ଯଦି (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } ଅନ୍ୟଥା {
            ପାଇଁ (AstCFunc* ଅଚଳ cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    ଶୂନ୍ୟ visit(AstCFunc* ଅଚଳ nodep) override {
        ଅଚଳ auto& ମୂଲ୍ୟ = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        ଯଦି (ମୂଲ୍ୟ.m_state != CFuncCache::UNINITIALIZED) ଫେରନ୍ତୁ;
        VL_RESTORER(m_cfuncp);
        // Its timing domains are those of the processes calling it, see visitCalledCFunc()
        VL_RESTORER_CLEAR(m_timingDomains);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }

    // Pre/Post logic are created here and their content need no further changes, so ignore.
    ଶୂନ୍ୟ visit(AstAlwaysPre*) override {}
    ଶୂନ୍ୟ visit(AstAlwaysPost*) override {}

    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
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
        V3Stats::addStat("NBA, variables using ValueQueueWhole scheme", m_n_Scheme_ମୂଲ୍ୟ_Queues_Whole);
        V3Stats::addStat("NBA, variables using ValueQueuePartial scheme",
                         m_n_Scheme_ମୂଲ୍ୟ_Queues_Partial);
        V3Stats::addStat("Optimizations, NBA flags shared", m_nSharedSetFlags);
        V3Stats::addStat("Procedures needing initial NBA trigger", m_nInitialNBA);
        V3Stats::addStat("Non-inlined co_awaits with SenTree", m_non_Inlined_CAwaits_With_Sen_ବୃକ୍ଷ);
    }
};

//######################################################################
// Delayed class functions

ଶୂନ୍ୟ V3Delayed::delayedAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DelayedVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("delayed", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
