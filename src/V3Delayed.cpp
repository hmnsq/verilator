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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Delayed.h"

#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3ClassGraph.h"
#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3LinkLValue.h"
#అన్తర్భావ "V3SharedTmps.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <deque>
#అన్తర్భావ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert AstAssignDlys (NBAs)

తరగతి DelayedVisitor final : బహిరంగ VNVisitor {
    // TYPES

    // The various NBA conversion schemes, including error cases
    గణన తరగతి Scheme : uint8_t {
        Undecided = 0,
        Unsupported_Compound_Array_లోపల_Loop,
        ShadowVar,
        ShadowVarMasked,
        FlagShared,
        FlagUnique,
        విలువ_వరుస_సంపూర్ణ,
        విలువ_వరుస_Partial
    };

    // All info associated with a variable that is the target of an NBA
    తరగతి VarScopeInfo final {
    బహిరంగ:
        // First write reference encountered to the VarScope as the target on an NBA
        స్థిరరాశి AstVarRef* m_మొదటి_Nba_Refp = శూన్యసూచిక;
        // Active block 'm_firstNbaRefp' is under
        స్థిరరాశి Ast_చురుకైన* m_fistActivep = శూన్యసూచిక;
        తార్కిక m_సంపూర్ణ = అబద్ధం;  // Used on LHS of NBA directly via VarRef
        తార్కిక m_partial = అబద్ధం;  // Used on LHS of NBA under a Sel
        తార్కిక m_లోపల_Loop = అబద్ధం;  // Used on LHS of NBA in a loop
        తార్కిక m_లోపల_Susp_Or_Fork = అబద్ధం;  // Used on LHS of NBA in suspendable process or fork
        Scheme m_scheme = Scheme::Undecided;  // Conversion scheme to use for this variable

    అంతరంగ:
        // Combined sensitivities of all NBAs targeting this variable
        Ast_Sen_వృక్షం* m_senTreep = శూన్యసూచిక;

        // Union of stuff needed for the various schemes - use accessors below!
        సంఘం {
            నిర్మాణం {  // Stuff needed for Scheme::ShadowVar
                AstVarScope* vscp;  // The shadow variable
            } m_shadowVariableKit;
            నిర్మాణం {  // Stuff needed for Scheme::ShadowVarMasked
                AstVarScope* vscp;  // The shadow variable
                AstVarScope* maskp;  // The mask variable
            } m_shadowVarMaskedKit;
            నిర్మాణం {  // Stuff needed for Scheme::FlagShared
                Ast_చురుకైన* activep;  // The active block for the Pre/Post logic
                AstAlwaysPost* postp;  // The post block for commiting results
                AstVarScope* commitFlagp;  // The commit flag variable, for reuse
                AstIf* commitIfp;  // The previous if statement for committing, for reuse
            } m_flagSharedKit;
            నిర్మాణం {  // Stuff needed for Scheme::FlagUnique
                AstAlwaysPost* postp;  // The post block for commiting results
            } m_flagUniqueKit;
            నిర్మాణం {  // Stuff needed for Scheme::ValueQueueWhole/Scheme::ValueQueuePartial
                AstVarScope* vscp;  // The commit queue variable
            } m_విలువ_వరుస_Kit;
        } m_kitUnion;

    బహిరంగ:
        VarScopeInfo() = అప్రమేయ;
        ~VarScopeInfo() {
            // Might not be linked if there was an error
            ఒకవేళ (!m_senTreep->backp()) VL_DO_DANGLING(m_senTreep->delete_వృక్షం(), m_senTreep);
        }
        // Accessors for the above union fields
        స్వయంచాలక& shadowVariableKit() {
            UASSERT(m_scheme == Scheme::ShadowVar, "Inconsistent Scheme");
            తిరిగి m_kitUnion.m_shadowVariableKit;
        }
        స్వయంచాలక& shadowVarMaskedKit() {
            UASSERT(m_scheme == Scheme::ShadowVarMasked, "Inconsistent Scheme");
            తిరిగి m_kitUnion.m_shadowVarMaskedKit;
        }
        స్వయంచాలక& flagSharedKit() {
            UASSERT(m_scheme == Scheme::FlagShared, "Inconsistent Scheme");
            తిరిగి m_kitUnion.m_flagSharedKit;
        }
        స్వయంచాలక& flagUniqueKit() {
            UASSERT(m_scheme == Scheme::FlagUnique, "Inconsistent Scheme");
            తిరిగి m_kitUnion.m_flagUniqueKit;
        }
        స్వయంచాలక& విలువ_వరుస_Kit() {
            UASSERT(m_scheme == Scheme::విలువ_వరుస_Partial || m_scheme == Scheme::విలువ_వరుస_సంపూర్ణ,
                    "Inconsistent Scheme");
            తిరిగి m_kitUnion.m_విలువ_వరుస_Kit;
        }

        // Accessor
        Ast_Sen_వృక్షం* senTreep() స్థిరరాశి { తిరిగి m_senTreep; }

        // Add sensitivities
        శూన్య కూడు_సున్నితత్వం(Ast_Sen_అంశం* nodep) {
            ఒకవేళ (!m_senTreep) m_senTreep = కొత్త Ast_Sen_వృక్షం{nodep->fileline(), శూన్యసూచిక};
            // Add a copy of each term
            m_senTreep->కూడు_Sensesp(nodep->clone_వృక్షం(నిజం));
            // Remove duplicates
            V3Const::constifyExpensiveEdit(m_senTreep);
        }
        // cppcheck-suppress constParameterPointer
        శూన్య కూడు_సున్నితత్వం(Ast_Sen_వృక్షం* nodep) { కూడు_సున్నితత్వం(nodep->sensesp()); }
    };

    // Data structure to keep track of all writes to
    నిర్మాణం WriteReference final {
        AstVarRef* m_refp = శూన్యసూచిక;  // The reference
        తార్కిక m_isNBA = అబద్ధం;  // True if an NBA write
        తార్కిక m_లోపల_Non_Comb = అబద్ధం;  // True if reference is known to be in non-combinational logic
        WriteReference() = అప్రమేయ;
        WriteReference(AstVarRef* refp, తార్కిక isNBA, తార్కిక లోపల_Non_Comb)
            : m_refp{refp}
            , m_isNBA{isNBA}
            , m_లోపల_Non_Comb{లోపల_Non_Comb} {}
    };

    // Data required to lower AstAssignDelay later
    నిర్మాణం NBA final {
        Ast_కేటాయించు_Dly* nodep = శూన్యసూచిక;  // The NBA this record refers to
        AstVarScope* vscp = శూన్యసూచిక;  // The target variable the NBA is updating
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
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;
    స్థిరరాశి VNUser2_లోపల_Use m_user2_లోపల_Use;
    స్థిరరాశి VNUser3_లోపల_Use m_user3_లోపల_Use;

    నిర్మాణం CFuncCache final {
        VInsertion_సమితి<Ast_Sen_వృక్షం*> m_timingDomains;  // What shall be added to m_timingDomains
        std::సమితి<AstCFunc*>
            m_includes;  // CFuncs whose CFuncCache shall be included into this - this is used to
                         // break cycles: A->B->A (instead of visiting A while it is still begin
                         // visited B just marks that it includes A)
        గణన State : uint8_t {
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
    AstUser3Allocator<AstVarScope, std::సదిశ<WriteReference>> m_writeRefs;

    // STATE - across all visitors
    VInsertion_సమితి<Ast_Sen_వృక్షం*> m_timingDomains;  // Timing resume domains
    V3SharedTmps m_dlyTmps{"__Vdly", VVarType::BLOCKTEMP};  // Temporary variables
    // Commit queue data types, by element type and partial flag. Shared by all instances,
    // as m_dlyTmps only shares variables with the same data type.
    std::map<std::pair<స్థిరరాశి Ast_కణుపు_DType*, తార్కిక>, Ast_NBACommit_వరుస_DType*> m_cqDTypeps;

    స్థిరరాశి std::unique_ptr<V3_Class_లేఖాచిత్రం>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::సదిశ<స్థిరరాశి AstCFunc*> m_call_దొంతర;  // Current callstack of AstCFuncs

    // STATE - for current visit position (use VL_RESTORER)
    Ast_చురుకైన* m_activep = శూన్యసూచిక;  // Current activate
    స్థిరరాశి AstCFunc* m_cfuncp = శూన్యసూచిక;  // Current public C Function
    Ast_కణుపు_Procedure* m_procp = శూన్యసూచిక;  // Current process
    AstScope* m_scopep = శూన్యసూచిక;  // Current scope
    తార్కిక m_లోపల_Loop = అబద్ధం;  // True in for loops
    తార్కిక m_లోపల_Suspendable_Or_Fork = అబద్ధం;  // True in suspendable processes and forks
    తార్కిక m_ignoreBlkAndNBlk = అబద్ధం;  // Suppress delayed assignment BLKANDNBLK
    తార్కిక m_లోపల_Non_Comb_Logic = అబద్ధం;  // We are in non-combinational logic
    తార్కిక m_needsInitialTrigger = అబద్ధం;  // Whether a NodeProcedure needs a initial trigger
    std::సదిశ<Ast_Sen_వృక్షం*> m_nbaEventSenTreeps;  // Sensitivities of '->>' in the process
    AstVarRef* m_currNbaLhsRefp = శూన్యసూచిక;  // Current NBA LHS variable reference

    // STATE - during NBA conversion (after visit)
    std::సదిశ<NBA> m_nbas;  // AstAssignDly instances to lower at the end
    std::సదిశ<AstVarScope*> m_vscps;  // Target variables on LHSs of NBAs
    Ast_కేటాయించు_Dly* m_తదుపరి_Dlyp = శూన్యసూచిక;  // The nextp of the previous AstAssignDly
    AstVarScope* m_prevVscp = శూన్యసూచిక;  // The target of the previous AstAssignDly

    // STATE - Statistic tracking
    VDouble0 m_nSchemeShadowVar;  // Number of variables using Scheme::ShadowVar
    VDouble0 m_nSchemeShadowVarMasked;  // Number of variabels using Scheme::ShadowVarMasked
    VDouble0 m_nSchemeFlagShared;  // Number of variables using Scheme::FlagShared
    VDouble0 m_nSchemeFlagUnique;  // Number of variables using Scheme::FlagUnique
    VDouble0 m_n_Scheme_విలువ_Queues_సంపూర్ణ;  //  Number of variables using Scheme::ValueQueueWhole
    VDouble0 m_n_Scheme_విలువ_Queues_Partial;  //  Number of variables using Scheme::ValueQueuePartial
    VDouble0 m_n_Shared_సమితి_Flags;  // "Set" flags actually shared by Scheme::FlagShared variables
    VDouble0 m_nInitialNBA;  // Number of procedural blocks with initial NBA
    VDouble0 m_non_Inlined_CAwaits_With_Sen_వృక్షం;  // Count uses of not inlined co_awaits

    // METHODS

    // Return true iff a variable is assigned by both blocking and nonblocking
    // assignments. Issue BLKANDNBLK error if we can't prove the mixed
    // assignments are to independent bits and the blocking assignment can be
    // in combinational logic, which is something we can't safely implement
    // still.
    తార్కిక checkMixedUsage(స్థిరరాశి AstVarScope* vscp, తార్కిక isIntegralOrPacked) {

        నిర్మాణం Ref final {
            AstVarRef* m_refp;  // The reference
            తార్కిక m_లోపల_Non_Comb;  // True if known to be in non-combinational logic
            పూర్ణాంకం m_lsb;  // LSB of accessed range
            పూర్ణాంకం m_msb;  // MSB of accessed range
            Ref(AstVarRef* refp, తార్కిక లోపల_Non_Comb, పూర్ణాంకం lsb, పూర్ణాంకం msb)
                : m_refp{refp}
                , m_లోపల_Non_Comb{లోపల_Non_Comb}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };

        std::సదిశ<Ref> blkRefs;  // Blocking writes
        std::సదిశ<Ref> nbaRefs;  // Non-blockign writes

        స్థిరరాశి పూర్ణాంకం వెడల్పు = isIntegralOrPacked ? vscp->వెడల్పు() : 1;

        కొరకు (స్థిరరాశి స్వయంచాలక& writeRef : m_writeRefs(vscp)) {
            పూర్ణాంకం lsb = 0;
            పూర్ణాంకం msb = వెడల్పు - 1;
            ఒకవేళ (స్థిరరాశి AstSel* స్థిరరాశి selp = VN_CAST(writeRef.m_refp->backp(), Sel)) {
                ఒకవేళ (VN_IS(selp->lsbp(), Const)) {
                    lsb = selp->lsbConst();
                    msb = selp->msbConst();
                }
            }
            ఒకవేళ (writeRef.m_isNBA) {
                nbaRefs.emplace_వెనుక(writeRef.m_refp, writeRef.m_లోపల_Non_Comb, lsb, msb);
            } లేకపోతే {
                blkRefs.emplace_వెనుక(writeRef.m_refp, writeRef.m_లోపల_Non_Comb, lsb, msb);
            }
        }
        // We only run this function on targets of NBAs, so there should be at least one...
        UASSERT_OBJ(!nbaRefs.ఖాళీలు(), vscp, "Did not record NBA write");
        // If no blocking upadte, then we are good
        ఒకవేళ (blkRefs.ఖాళీలు()) తిరిగి అబద్ధం;

        // If the blocking assignment is in non-combinational logic (i.e.:
        // in logic that has an explicit trigger), then we can safely
        // implement it (there is no race between clocked logic and post
        // scheduled logic), so need not error
        blkRefs.erase(std::remove_if(blkRefs.begin(), blkRefs.చివర(),
                                     [](స్థిరరాశి Ref& ref) { తిరిగి ref.m_లోపల_Non_Comb; }),
                      blkRefs.చివర());

        // If nothing left, then we need not error
        ఒకవేళ (blkRefs.ఖాళీలు()) తిరిగి నిజం;

        // If not a packed variable, warn here as we can't prove independence
        ఒకవేళ (!isIntegralOrPacked) {
            స్థిరరాశి Ref& blkRef = blkRefs.front();
            స్థిరరాశి Ref& nbaRef = nbaRefs.front();
            vscp->v3warn(
                BLKANDNBLK,
                "Unsupported: Blocking and non-blocking assignments to same non-packed variable: "
                    << vscp->varp()->prettyNameQ() << '\n'
                    << vscp->warnContextPrimary() << '\n'
                    << blkRef.m_refp->warnOther() << "... Location of blocking assignment\n"
                    << blkRef.m_refp->warnContextSecondary() << '\n'
                    << nbaRef.m_refp->warnOther() << "... Location of nonblocking assignment\n"
                    << nbaRef.m_refp->warnContextSecondary());
            తిరిగి నిజం;
        }

        // We need to error if we can't prove the written bits are independent

        // Sort refs by interval
        స్థిరరాశి స్వయంచాలక lessThanRef = [](స్థిరరాశి Ref& a, స్థిరరాశి Ref& b) {
            ఒకవేళ (a.m_lsb != b.m_lsb) తిరిగి a.m_lsb < b.m_lsb;
            తిరిగి a.m_msb < b.m_msb;
        };
        std::stable_క్రమీకరించు(blkRefs.begin(), blkRefs.చివర(), lessThanRef);
        std::stable_క్రమీకరించు(nbaRefs.begin(), nbaRefs.చివర(), lessThanRef);
        // Iterate both vectors, checking for overlap
        స్వయంచాలక bIt = blkRefs.begin();
        స్వయంచాలక nIt = nbaRefs.begin();
        అయితే (bIt != blkRefs.చివర() && nIt != nbaRefs.చివర()) {
            ఒకవేళ (lessThanRef(*bIt, *nIt)) {
                ఒకవేళ (nIt->m_lsb <= bIt->m_msb) ఆపు;  // Stop on Overlap
                ++bIt;
            } లేకపోతే {
                ఒకవేళ (bIt->m_lsb <= nIt->m_msb) ఆపు;  // Stop on Overlap
                ++nIt;
            }
        }

        // If we found an overlapping range that cannot be safely implemented, then wran...
        ఒకవేళ (bIt != blkRefs.చివర() && nIt != nbaRefs.చివర()) {
            స్థిరరాశి Ref& blkRef = *bIt;
            స్థిరరాశి Ref& nbaRef = *nIt;
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

        తిరిగి నిజం;
    }

    // Choose the NBA scheme used for the given variable.
    Scheme ఎంచుకో_Scheme(స్థిరరాశి AstVarScope* vscp, స్థిరరాశి VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::Undecided, vscp, "NBA scheme already decided");

        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = vscp->dtypep()->skipRefp();
        // Unpacked arrays
        ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి uaDTypep = VN_CAST(dtypep, UnpackArrayDType)) {
            // If whole array is target of NBA, use ShadowVar, but not in a suspendable process or
            // fork, which resume outside the 'nba' region, so its 'pre' logic would overwrite the
            // value they write in the shadow variable
            ఒకవేళ (vscpInfo.m_సంపూర్ణ) {
                తిరిగి vscpInfo.m_లోపల_Susp_Or_Fork ? Scheme::FlagUnique : Scheme::ShadowVar;
            }
            // Basic underlying type of elements, if any.
            స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = uaDTypep->basicp();
            // If used in a loop, we must have a dynamic commit queue. (Also works in suspendables)
            ఒకవేళ (vscpInfo.m_లోపల_Loop) {
                // Arrays with compound element types are currently not supported in loops
                ఒకవేళ (!basicp
                    || !(basicp->isIntegralOrPacked()  //
                         || basicp->isDouble()  //
                         || basicp->isString())) {
                    తిరిగి Scheme::Unsupported_Compound_Array_లోపల_Loop;
                }
                ఒకవేళ (vscpInfo.m_partial) తిరిగి Scheme::విలువ_వరుస_Partial;
                తిరిగి Scheme::విలువ_వరుస_సంపూర్ణ;
            }
            // A suspendable process or fork can resume outside the 'nba' region, so the 'pre'
            // logic of the other schemes would undo its updates. The unique flag scheme has none.
            ఒకవేళ (vscpInfo.m_లోపల_Susp_Or_Fork) తిరిగి Scheme::FlagUnique;
            // Otherwise if an array of packed/basic elements, use the shared flag scheme
            ఒకవేళ (basicp) తిరిగి Scheme::FlagShared;
            // Finally fall back on the shadow variable scheme, e.g. for
            // arrays of unpacked structs. This will be slow.
            // TODO: generic LHS scheme as discussed in #5092
            తిరిగి Scheme::ShadowVar;
        }

        // Likewise, the 'pre' logic of the shadow variable scheme would undo the updates of a
        // suspendable process or fork, so use the unique flag scheme
        ఒకవేళ (vscpInfo.m_లోపల_Susp_Or_Fork) తిరిగి Scheme::FlagUnique;

        స్థిరరాశి తార్కిక isIntegralOrPacked = dtypep->isIntegralOrPacked();
        // Check for mixed usage (this also warns if not OK)
        ఒకవేళ (checkMixedUsage(vscp, isIntegralOrPacked)) {
            // If it's a variable updated by both blocking and non-blocking
            // assignments, use the ShadowVarMasked schem if masked update is
            // possible. This can handle blocking and non-blocking updates to
            // inpdendent parts correctly at run-time, and always works, even
            // in loops or other dynamic context.
            ఒకవేళ (isIntegralOrPacked) తిరిగి Scheme::ShadowVarMasked;
            // If it's inside a loop, use Scheme::ShadowVar, which is safe,
            // but will generate incorrect code if a partial update is used
            ఒకవేళ (vscpInfo.m_లోపల_Loop) తిరిగి Scheme::ShadowVar;
            // Otherwise (for not packed variables), use the FlagUnique scheme,
            // which at least handles partial updates correctly, but might break
            // in loops or other dynamic context
            తిరిగి Scheme::FlagUnique;
        }

        // Otherwise use the simple shadow variable scheme
        తిరిగి Scheme::ShadowVar;
    }

    // Given an expression 'exprp', return a new expression that always evaluates to the
    // value of the given expression at this point in the program. That is:
    // - If given a non-constant expression, create a new temporary AstVarScope under the given
    //   'scopep', with the given 'name', assign the expression to it, and return a read reference
    //   to the new AstVarScope.
    // - If given a constant, just return that constant.
    // New statements are inserted before 'insertp'
    Ast_కణుపు_సమాసం* captureVal(AstScope* స్థిరరాశి scopep, Ast_కణుపు_Stmt* స్థిరరాశి insertp,
                            Ast_కణుపు_సమాసం* స్థిరరాశి exprp, స్థిరరాశి std::string& name) {
        UASSERT_OBJ(!exprp->backp(), exprp, "Should have been unlinked");
        File_పంక్తి* స్థిరరాశి flp = exprp->fileline();
        ఒకవేళ (VN_IS(exprp, Const)) తిరిగి exprp;
        // TODO: there are some const variables that could be simply referenced here
        AstVarScope* స్థిరరాశి tmpVscp = m_dlyTmps.make(flp, scopep, exprp->dtypep(), name);
        insertp->కూడు_ఇక్కడ_This_As_తదుపరి(
            కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, tmpVscp, VAccess::WRITE}, exprp});
        తిరిగి కొత్త AstVarRef{flp, tmpVscp, VAccess::READ};
    };

    // Similar to 'captureVal', but captures an LValue expression. That is, the returned
    // expression will reference the same location as the input expression, at this point in the
    // program.
    Ast_కణుపు_సమాసం* captureLhs(AstScope* స్థిరరాశి scopep, Ast_కణుపు_Stmt* స్థిరరాశి insertp,
                            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp, స్థిరరాశి std::string& పునాది_Name) {
        UASSERT_OBJ(!lhsp->backp(), lhsp, "Should have been unlinked");
        // Running node pointer
        Ast_కణుపు_సమాసం* nodep = lhsp;
        // Capture AstSel indices - there should be only one
        ఒకవేళ (AstSel* స్థిరరాశి selp = VN_CAST(nodep, Sel)) {
            స్థిరరాశి std::string tmpName{"Lsb" + పునాది_Name};
            selp->lsbp(captureVal(scopep, insertp, selp->lsbp()->unlink_Fr_వెనుక(), tmpName));
            // Continue with target
            nodep = selp->fromp();
        }
        UASSERT_OBJ(!VN_IS(nodep, Sel), lhsp, "Multiple 'AstSel' applied to LHS reference");
        // Capture the indices of selects - might be many, also below struct members. If a handle
        // selects the instance the target is a member of, capture the handle instead, as it is
        // only read, to select the target when the NBA is executed (IEEE 1800-2023 10.4.2).
        కొలత_t nArraySels = 0;
        అయితే (నిజం) {
            ఒకవేళ (Ast_కణుపు_Sel* స్థిరరాశి selp = VN_CAST(nodep, కణుపు_Sel)) {
                స్థిరరాశి std::string tmpName{"Dim" + std::గమ్యం_string(nArraySels++) + పునాది_Name};
                selp->bitp(captureVal(scopep, insertp, selp->bitp()->unlink_Fr_వెనుక(), tmpName));
                nodep = selp->fromp();
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstStructSel* స్థిరరాశి selp = VN_CAST(nodep, StructSel)) {
                nodep = selp->fromp();
            } లేకపోతే ఒకవేళ (AstMemberSel* స్థిరరాశి selp = VN_CAST(nodep, MemberSel)) {
                Ast_కణుపు_సమాసం* స్థిరరాశి handlep = selp->fromp()->unlink_Fr_వెనుక();
                V3LinkLValue::link_LValue_సమితి(handlep, VAccess::READ);
                AstVarRef* స్థిరరాశి refp
                    = VN_AS(captureVal(scopep, insertp, handlep, "Handle" + పునాది_Name), VarRef);
                refp->varp()->సమితి_Ignore_Post_Write();
                // Like the handle it replaces, mark it written, as is the member it selects
                refp->access(selp->access());
                selp->fromp(refp);
                తిరిగి lhsp;
            } లేకపోతే {
                ఆపు;
            }
        }
        // What remains must be the variable, or some sort of select, we assume can reuse it
        UASSERT_OBJ(nodep->is_శుద్ధ(), lhsp, "Malformed LHS in NBA");
        // Now have been converted to use the captured values
        తిరిగి lhsp;
    }

    శూన్య కూడు_CFunc_Cached_విలువలు(స్థిరరాశి AstCFunc* స్థిరరాశి cfuncp,
                              std::unordered_సమితి<స్థిరరాశి AstCFunc*>& సందర్శించిన) {
        ఒకవేళ (!సందర్శించిన.చొప్పించు(cfuncp).రెండవ) తిరిగి;
        CFuncCache& విలువ = m_cfuncsCache(cfuncp);
        m_timingDomains.చొప్పించు(విలువ.m_timingDomains.begin(), విలువ.m_timingDomains.చివర());
        m_non_Inlined_CAwaits_With_Sen_వృక్షం += విలువ.m_timingDomains.కొలత();
        కొరకు (స్థిరరాశి AstCFunc* స్థిరరాశి includedp : విలువ.m_includes) {
            కూడు_CFunc_Cached_విలువలు(includedp, సందర్శించిన);
        }
    }

    // Create a temporary variable in the given 'scopep', with the given 'name', and with 'dtypep'
    // type, with the bits selected by 'sLsbp'/'sWidthp' set to 'valuep', other bits set to zero.
    // Insert new statements before 'insertp'.
    // Returns a read reference to the temporary variable.
    AstVarRef* createWidened(File_పంక్తి* flp, AstScope* scopep, Ast_కణుపు_DType* dtypep,
                             Ast_కణుపు_సమాసం* sLsbp, పూర్ణాంకం s_వెడల్పు, స్థిరరాశి std::string& name,
                             Ast_కణుపు_సమాసం* valuep, Ast_కణుపు* insertp) {
        // Create temporary variable.
        AstVarScope* స్థిరరాశి tp = m_dlyTmps.make(flp, scopep, dtypep, name);
        // Zero it
        AstConst* స్థిరరాశి zerop = కొత్త AstConst{flp, AstConst::DTyped{}, dtypep};
        zerop->అంకె().సమితి_All_Bits0();
        insertp->కూడు_ఇక్కడ_This_As_తదుపరి(
            కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, tp, VAccess::WRITE}, zerop});
        // Set the selected bits to 'valuep'
        AstSel* స్థిరరాశి selp = కొత్త AstSel{flp, కొత్త AstVarRef{flp, tp, VAccess::WRITE},
                                        sLsbp->clone_వృక్షం_శుద్ధ(నిజం), s_వెడల్పు};
        insertp->కూడు_ఇక్కడ_This_As_తదుపరి(కొత్త Ast_కేటాయించు{flp, selp, valuep});
        // This is the expression to get the value of the temporary
        తిరిగి కొత్త AstVarRef{flp, tp, VAccess::READ};
    }

    // Scheme::ShadowVar
    శూన్య prepareSchemeShadowVar(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
        AstScope* స్థిరరాశి scopep = vscp->scopep();
        // Create the shadow variable
        స్థిరరాశి std::string name = "_" + vscp->varp()->shortName();
        AstVarScope* స్థిరరాశి shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), name);
        vscpInfo.shadowVariableKit().vscp = shadowVscp;
        // Mark both for V3LifePsot
        vscp->optimizeLifePost(నిజం);
        shadowVscp->optimizeLifePost(నిజం);
        // Create the AstActive for the Pre/Post logic
        Ast_చురుకైన* స్థిరరాశి activep = కొత్త Ast_చురుకైన{flp, "nba-shadow-variable", vscpInfo.senTreep()};
        activep->sen_వృక్షం_Storep(vscpInfo.senTreep());
        scopep->కూడు_Blocksp(activep);
        // Add 'Pre' scheduled 'shadowVariable = originalVariable' assignment
        AstAlwaysPre* స్థిరరాశి prep = కొత్త AstAlwaysPre{flp};
        activep->కూడు_Stmtsp(prep);
        prep->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, shadowVscp, VAccess::WRITE},
                                      కొత్త AstVarRef{flp, vscp, VAccess::READ}});
        // Add 'Post' scheduled 'originalVariable = shadowVariable' assignment
        AstAlwaysPost* స్థిరరాశి postp = కొత్త AstAlwaysPost{flp};
        activep->కూడు_Stmtsp(postp);
        postp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, vscp, VAccess::WRITE},
                                       కొత్త AstVarRef{flp, shadowVscp, VAccess::READ}});
    }
    శూన్య convertSchemeShadowVar(Ast_కేటాయించు_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        AstVarScope* స్థిరరాశి shadowVscp = vscpInfo.shadowVariableKit().vscp;

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* స్థిరరాశి refp) {
            ఒకవేళ (!refp->access().isWriteOnly()) తిరిగి;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
    }

    // Scheme::ShadowVarMasked
    శూన్య prepareSchemeShadowVarMasked(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
        AstScope* స్థిరరాశి scopep = vscp->scopep();
        // Create the shadow variable
        స్థిరరాశి std::string shadowName = "_" + vscp->varp()->shortName();
        AstVarScope* స్థిరరాశి shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), shadowName);
        vscpInfo.shadowVarMaskedKit().vscp = shadowVscp;
        // Create the makk variable
        స్థిరరాశి std::string ముసుగు_Name = "Mask__" + vscp->varp()->shortName();
        AstVarScope* స్థిరరాశి ముసుగు_Vscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), ముసుగు_Name);
        ముసుగు_Vscp->varp()->సమితి_Ignore_Post_Write();
        vscpInfo.shadowVarMaskedKit().maskp = ముసుగు_Vscp;
        // Create the AstActive for the Post logic
        Ast_చురుకైన* స్థిరరాశి activep
            = కొత్త Ast_చురుకైన{flp, "nba-shadow-var-masked", vscpInfo.senTreep()};
        activep->sen_వృక్షం_Storep(vscpInfo.senTreep());
        scopep->కూడు_Blocksp(activep);
        // Add 'Post' scheduled process for the commit and mask clear
        AstAlwaysPost* స్థిరరాశి postp = కొత్త AstAlwaysPost{flp};
        activep->కూడు_Stmtsp(postp);
        // Add the commit - vscp = (shadowVscp & maskVscp) | (vscp & ~maskVscp);
        postp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{
            flp, కొత్త AstVarRef{flp, vscp, VAccess::WRITE},
            కొత్త AstOr{flp,
                      కొత్త AstAnd{flp, కొత్త AstVarRef{flp, shadowVscp, VAccess::READ},
                                 కొత్త AstVarRef{flp, ముసుగు_Vscp, VAccess::READ}},
                      కొత్త AstAnd{flp, కొత్త AstVarRef{flp, vscp, VAccess::READ},
                                 కొత్త AstNot{flp, కొత్త AstVarRef{flp, ముసుగు_Vscp, VAccess::READ}}}}});
        vscp->varp()->సమితి_Ignore_Post_Read();
        // Clar the mask - maskVscp = '0;
        postp->కూడు_Stmtsp(
            కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, ముసుగు_Vscp, VAccess::WRITE},
                          కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, ముసుగు_Vscp->వెడల్పు(), 0}});
    }
    శూన్య convertSchemeShadowVarMasked(Ast_కేటాయించు_Dly* nodep, AstVarScope* vscp,
                                      VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        AstVarScope* స్థిరరాశి shadowVscp = vscpInfo.shadowVarMaskedKit().vscp;
        AstVarScope* స్థిరరాశి ముసుగు_Vscp = vscpInfo.shadowVarMaskedKit().maskp;

        Ast_కణుపు_సమాసం* lhsClonep = nodep->lhsp()->clone_వృక్షం(అబద్ధం);

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* స్థిరరాశి refp) {
            ఒకవేళ (!refp->access().isWriteOnly()) తిరిగి;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
        // Set the same bits in the mask to 1
        lhsClonep->foreach([&](AstVarRef* స్థిరరాశి refp) {
            ఒకవేళ (!refp->access().isWriteOnly()) తిరిగి;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(ముసుగు_Vscp);
            refp->varp(ముసుగు_Vscp->varp());
        });
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        AstConst* స్థిరరాశి onesp = కొత్త AstConst{flp, AstConst::DTyped{}, lhsClonep->dtypep()};
        onesp->అంకె().సమితి_All_Bits1();
        nodep->కూడు_తదుపరి_ఇక్కడ(కొత్త Ast_కేటాయించు{flp, lhsClonep, onesp});
    }

    // Scheme::FlagShared
    శూన్య prepareSchemeFlagShared(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
        AstScope* స్థిరరాశి scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        Ast_చురుకైన* స్థిరరాశి activep = కొత్త Ast_చురుకైన{flp, "nba-flag-shared", vscpInfo.senTreep()};
        activep->sen_వృక్షం_Storep(vscpInfo.senTreep());
        scopep->కూడు_Blocksp(activep);
        vscpInfo.flagSharedKit().activep = activep;
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* స్థిరరాశి postp = కొత్త AstAlwaysPost{flp};
        activep->కూడు_Stmtsp(postp);
        vscpInfo.flagSharedKit().postp = postp;
        // Initialize
        vscpInfo.flagSharedKit().commitFlagp = శూన్యసూచిక;
        vscpInfo.flagSharedKit().commitIfp = శూన్యసూచిక;
    }
    శూన్య convertSchemeFlagShared(Ast_కేటాయించు_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
        AstScope* స్థిరరాశి scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        స్థిరరాశి std::string పునాది_Name = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_కణుపు_సమాసం* స్థిరరాశి capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlink_Fr_వెనుక(), "Val" + పునాది_Name);

        // Unlink and capture the LHS reference
        Ast_కణుపు_సమాసం* స్థిరరాశి capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlink_Fr_వెనుక(), పునాది_Name);

        // Is this NBA adjacent after the previously processed NBA?
        స్థిరరాశి తార్కిక consecutive = nodep == m_తదుపరి_Dlyp;
        m_తదుపరి_Dlyp = VN_CAST(nodep->nextp(), కేటాయించు_Dly);

        VarScopeInfo* స్థిరరాశి prevVscpInfop = consecutive ? &m_vscpInfo(m_prevVscp) : శూన్యసూచిక;

        // We can reuse the flag of the previous assignment if:
        స్థిరరాశి తార్కిక reuseTheFlag =
            // Consecutive NBAs
            consecutive
            // ... that use the same scheme
            && prevVscpInfop->m_scheme == Scheme::FlagShared
            // ... and are in the same scope as the target variable
            && scopep == vscp->scopep()
            // ... and share the same overall update domain
            && prevVscpInfop->senTreep()->same_వృక్షం(vscpInfo.senTreep());

        ఒకవేళ (!reuseTheFlag) {
            // Create new flag
            AstVarScope* స్థిరరాశి flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + పునాది_Name);
            // Set the flag at the original NBA
            nodep->కూడు_ఇక్కడ_This_As_తదుపరి(  //
                కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, flagVscp, VAccess::WRITE},
                              కొత్త AstConst{flp, AstConst::బిట్టు_True{}}});
            // Add the 'Pre' scheduled reset for the flag
            AstAlwaysPre* స్థిరరాశి prep = కొత్త AstAlwaysPre{flp};
            vscpInfo.flagSharedKit().activep->కూడు_Stmtsp(prep);
            prep->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, flagVscp, VAccess::WRITE},
                                          కొత్త AstConst{flp, AstConst::బిట్టు_False{}}});
            // Add the 'Post' scheduled commit
            AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, కొత్త AstVarRef{flp, flagVscp, VAccess::READ}};
            vscpInfo.flagSharedKit().postp->కూడు_Stmtsp(ifp);
            vscpInfo.flagSharedKit().commitFlagp = flagVscp;
            vscpInfo.flagSharedKit().commitIfp = ifp;
        } లేకపోతే {
            ఒకవేళ (vscp != m_prevVscp) {
                // Different variable, ensure the commit block exists for this variable,
                // can reuse existing one with the same flag, otherwise create a new one.
                AstVarScope* స్థిరరాశి flagVscp = prevVscpInfop->flagSharedKit().commitFlagp;
                UASSERT_OBJ(flagVscp, nodep, "Commit flag of previous assignment should exist");
                ఒకవేళ (vscpInfo.flagSharedKit().commitFlagp != flagVscp) {
                    AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, కొత్త AstVarRef{flp, flagVscp, VAccess::READ}};
                    vscpInfo.flagSharedKit().postp->కూడు_Stmtsp(ifp);
                    vscpInfo.flagSharedKit().commitFlagp = flagVscp;
                    vscpInfo.flagSharedKit().commitIfp = ifp;
                }
            } లేకపోతే {
                // Same variable, reuse the commit block of the previous assignment
                vscpInfo.flagSharedKit().commitFlagp = prevVscpInfop->flagSharedKit().commitFlagp;
                vscpInfo.flagSharedKit().commitIfp = prevVscpInfop->flagSharedKit().commitIfp;
            }
            ++m_n_Shared_సమితి_Flags;
        }
        // Commit the captured value to the captured destination
        vscpInfo.flagSharedKit().commitIfp->కూడు_Thensp(
            కొత్త Ast_కేటాయించు{flp, capturedLhsp, capturedRhsp});

        // Remember the variable for the next NBA
        m_prevVscp = vscp;

        // Delete original NBA
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }

    // Scheme::FlagUnique
    శూన్య prepareSchemeFlagUnique(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
        AstScope* స్థిరరాశి scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        Ast_చురుకైన* స్థిరరాశి activep = కొత్త Ast_చురుకైన{flp, "nba-flag-unique", vscpInfo.senTreep()};
        activep->sen_వృక్షం_Storep(vscpInfo.senTreep());
        scopep->కూడు_Blocksp(activep);
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* స్థిరరాశి postp = కొత్త AstAlwaysPost{flp};
        activep->కూడు_Stmtsp(postp);
        vscpInfo.flagUniqueKit().postp = postp;
    }
    శూన్య convertSchemeFlagUnique(Ast_కేటాయించు_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
        AstScope* స్థిరరాశి scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        స్థిరరాశి std::string పునాది_Name = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_కణుపు_సమాసం* స్థిరరాశి capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlink_Fr_వెనుక(), "Val" + పునాది_Name);

        // Unlink and capture the LHS reference
        Ast_కణుపు_సమాసం* స్థిరరాశి capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlink_Fr_వెనుక(), పునాది_Name);

        // Create new flag
        AstVarScope* స్థిరరాశి flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + పునాది_Name);
        flagVscp->varp()->సమితి_Ignore_Post_Write();
        // Set the flag at the original NBA
        nodep->కూడు_ఇక్కడ_This_As_తదుపరి(  //
            కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, flagVscp, VAccess::WRITE},
                          కొత్త AstConst{flp, AstConst::బిట్టు_True{}}});
        // Add the 'Post' scheduled commit
        AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, కొత్త AstVarRef{flp, flagVscp, VAccess::READ}};
        vscpInfo.flagUniqueKit().postp->కూడు_Stmtsp(ifp);
        // Immediately clear the flag
        ifp->కూడు_Thensp(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, flagVscp, VAccess::WRITE},
                                     కొత్త AstConst{flp, AstConst::బిట్టు_False{}}});
        // Commit the value
        ifp->కూడు_Thensp(కొత్త Ast_కేటాయించు{flp, capturedLhsp, capturedRhsp});

        // Delete original NBA
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }

    // Scheme::ValueQueuePartial/Scheme::ValueQueueWhole
    మూస <తార్కిక N_Partial>
    శూన్య prepare_Scheme_విలువ_వరుస(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(N_Partial ? vscpInfo.m_scheme == Scheme::విలువ_వరుస_Partial
                              : vscpInfo.m_scheme == Scheme::విలువ_వరుస_సంపూర్ణ,
                    vscp, "Inconsistent NBA scheme");
        File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
        AstScope* స్థిరరాశి scopep = vscp->scopep();

        // Create the commit queue variable
        Ast_కణుపు_DType* స్థిరరాశి elemDTypep = vscp->dtypep()->skipRefp();
        Ast_NBACommit_వరుస_DType*& cqDTypep = m_cqDTypeps[{elemDTypep, N_Partial}];
        ఒకవేళ (!cqDTypep) {
            cqDTypep = కొత్త Ast_NBACommit_వరుస_DType{flp, elemDTypep, N_Partial};
            v3Global.rootp()->typeTablep()->కూడు_Typesp(cqDTypep);
        }
        స్థిరరాశి std::string name = "CommitQueue__" + vscp->varp()->shortName();
        AstVarScope* స్థిరరాశి వరుస_Vscp = m_dlyTmps.make(flp, scopep, cqDTypep, name);
        వరుస_Vscp->varp()->noReset(నిజం);
        వరుస_Vscp->varp()->సమితి_Ignore_Post_Write();
        vscpInfo.విలువ_వరుస_Kit().vscp = వరుస_Vscp;
        // Create the AstActive for the Post logic
        Ast_చురుకైన* స్థిరరాశి activep
            = కొత్త Ast_చురుకైన{flp, "nba-value-queue-whole", vscpInfo.senTreep()};
        activep->sen_వృక్షం_Storep(vscpInfo.senTreep());
        scopep->కూడు_Blocksp(activep);
        // Add 'Post' scheduled process for the commit
        AstAlwaysPost* స్థిరరాశి postp = కొత్త AstAlwaysPost{flp};
        activep->కూడు_Stmtsp(postp);
        // Add the commit
        AstCMethodHard* స్థిరరాశి callp = కొత్త AstCMethodHard{
            flp, కొత్త AstVarRef{flp, వరుస_Vscp, VAccess::READWRITE}, VCMethod::NBA_COMMIT};
        callp->dtype_సమితి_Void();
        // TODO: this is a partial update, so must be READWRITE, but that breaks scheduling
        callp->కూడు_Pinsp(కొత్త AstVarRef{flp, vscp, VAccess::WRITE});
        postp->కూడు_Stmtsp(callp->makeStmt());
    }

    శూన్య convert_Scheme_విలువ_వరుస(Ast_కేటాయించు_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo,
                                 తార్కిక partial) {
        UASSERT_OBJ(partial ? vscpInfo.m_scheme == Scheme::విలువ_వరుస_Partial
                            : vscpInfo.m_scheme == Scheme::విలువ_వరుస_సంపూర్ణ,
                    vscp, "Inconsistent NBA scheme");
        File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
        AstScope* స్థిరరాశి scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        స్థిరరాశి std::string పునాది_Name = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_కణుపు_సమాసం* స్థిరరాశి capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlink_Fr_వెనుక(), "Val" + పునాది_Name);

        // Unlink and capture the LHS reference
        Ast_కణుపు_సమాసం* స్థిరరాశి capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlink_Fr_వెనుక(), పునాది_Name);

        // RHS value (can be widened/masked iff Partial)
        Ast_కణుపు_సమాసం* valuep = capturedRhsp;
        // RHS mask (iff Partial)
        Ast_కణుపు_సమాసం* maskp = శూన్యసూచిక;

        // Running node for LHS deconstruction
        Ast_కణుపు_సమాసం* lhsNodep = capturedLhsp;

        // If partial updates are required, construct the mask and the widened value
        ఒకవేళ (partial) {
            // Type of array element
            Ast_కణుపు_DType* స్థిరరాశి eDTypep = [&]() -> Ast_కణుపు_DType* {
                Ast_కణుపు_DType* dtypep = vscp->dtypep()->skipRefp();
                అయితే (AstUnpackArrayDType* స్థిరరాశి ap = VN_CAST(dtypep, UnpackArrayDType)) {
                    dtypep = ap->subDTypep()->skipRefp();
                }
                తిరిగి dtypep;
            }();

            ఒకవేళ (స్థిరరాశి AstSel* స్థిరరాశి lSelp = VN_CAST(lhsNodep, Sel)) {
                // This is a partial assignment.
                // Need to create a mask and widen the value to element size.
                lhsNodep = lSelp->fromp();
                Ast_కణుపు_సమాసం* స్థిరరాశి sLsbp = lSelp->lsbp();
                స్థిరరాశి పూర్ణాంకం s_వెడల్పు = lSelp->వెడల్పు_Const();

                // Create mask value
                maskp = [&]() -> Ast_కణుపు_సమాసం* {
                    // Constant mask we can compute here
                    ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి cLsbp = VN_CAST(sLsbp, Const)) {
                        AstConst* స్థిరరాశి cp = కొత్త AstConst{flp, AstConst::DTyped{}, eDTypep};
                        cp->అంకె().సమితి_ముసుగు(s_వెడల్పు, cLsbp->గమ్యం_SInt());
                        తిరిగి cp;
                    }

                    // A non-constant mask we must compute at run-time.
                    AstConst* స్థిరరాశి onesp = కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, s_వెడల్పు, 0};
                    onesp->అంకె().సమితి_All_Bits1();
                    తిరిగి createWidened(flp, scopep, eDTypep, sLsbp, s_వెడల్పు, "Mask" + పునాది_Name,
                                         onesp, nodep);
                }();

                // Adjust value to element size
                valuep = [&]() -> Ast_కణుపు_సమాసం* {
                    // Constant value with constant select we can compute here
                    ఒకవేళ (AstConst* స్థిరరాశి cValuep = VN_CAST(valuep, Const)) {
                        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి cLsbp = VN_CAST(sLsbp, Const)) {
                            AstConst* స్థిరరాశి cp = కొత్త AstConst{flp, AstConst::DTyped{}, eDTypep};
                            cp->అంకె().సమితి_All_Bits0();
                            cp->అంకె().opSelInto(cValuep->అంకె(), cLsbp->గమ్యం_SInt(), s_వెడల్పు);
                            VL_DO_DANGLING(valuep->delete_వృక్షం(), valuep);
                            తిరిగి cp;
                        }
                    }

                    // A non-constant value we must adjust.
                    తిరిగి createWidened(flp, scopep, eDTypep, sLsbp, s_వెడల్పు,  //
                                         "Elem" + పునాది_Name, valuep, nodep);
                }();
            } లేకపోతే {
                // If this assignment is not partial, set mask to ones and we are done
                AstConst* స్థిరరాశి ones = కొత్త AstConst{flp, AstConst::DTyped{}, eDTypep};
                ones->అంకె().సమితి_All_Bits1();
                maskp = ones;
            }
        }

        // Extract array indices
        std::సదిశ<Ast_కణుపు_సమాసం*> idxps;
        {
            UASSERT_OBJ(VN_IS(lhsNodep, ArraySel), lhsNodep, "Unexpected LHS form");
            అయితే (AstArraySel* స్థిరరాశి aSelp = VN_CAST(lhsNodep, ArraySel)) {
                idxps.emplace_వెనుక(aSelp->bitp()->unlink_Fr_వెనుక());
                lhsNodep = aSelp->fromp();
            }
            UASSERT_OBJ(VN_IS(lhsNodep, VarRef), lhsNodep, "Unexpected LHS form");
            std::తిరగేయి(idxps.begin(), idxps.చివర());
        }

        // Done with the LHS at this point
        VL_DO_DANGLING(నెట్టు_Deletep(capturedLhsp), capturedLhsp);

        // Enqueue the update at the site of the original NBA
        AstCMethodHard* స్థిరరాశి callp = కొత్త AstCMethodHard{
            flp, కొత్త AstVarRef{flp, vscpInfo.విలువ_వరుస_Kit().vscp, VAccess::READWRITE},
            VCMethod::NBA_వరుసలో_చేర్చు};
        callp->dtype_సమితి_Void();
        callp->కూడు_Pinsp(valuep);
        ఒకవేళ (partial) callp->కూడు_Pinsp(maskp);
        కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి indexp : idxps) callp->కూడు_Pinsp(indexp);
        nodep->కూడు_ఇక్కడ_This_As_తదుపరి(callp->makeStmt());

        // Delete original NBA
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }

    // Record where a variable is assigned
    శూన్య రికార్డు_Write_Ref(AstVarRef* nodep, తార్కిక nonBlocking) {
        // Ignore references in certain contexts
        ఒకవేళ (m_ignoreBlkAndNBlk) తిరిగి;
        // Ignore if it's an array
        // TODO: we do this because it used to be the previous behaviour.
        //       Is it still required, or should we warn for arrays as well?
        //       Scheduling is no different for them...
        //       Clarification: This is OK for arrays of primitive types, but
        //       arrays that use the ShadowVar scheme don't work...
        ఒకవేళ (VN_IS(nodep->varScopep()->dtypep()->skipRefp(), UnpackArrayDType)) తిరిగి;

        m_writeRefs(nodep->varScopep()).emplace_వెనుక(nodep, nonBlocking, m_లోపల_Non_Comb_Logic);
    }

    మూస <రకం_పేరు Procedure_T>
    స్థిర తార్కిక isProcedureWithSentreep(స్థిరరాశి Ast_కణుపు_Procedure* స్థిరరాశి nodep) {
        స్థిరరాశి Procedure_T* స్థిరరాశి procedurep = Ast_కణుపు::cast<Procedure_T>(nodep);
        తిరిగి procedurep && procedurep->sentreep();
    }

    // Visit AstCFunc from a AstNodeCCall - this is made into a separate quasi-visitor because
    // AstCFunc that is not called from the code (e.g.: DPI exports) does not need to be visited
    // this way. Also, not visiting such AstCFuncs allows to avoid caching results for them which
    // this function does - which could lead to excessive memory usage
    శూన్య సందర్శించు_Called_CFunc(AstCFunc* స్థిరరాశి nodep) {
        CFuncCache& విలువ = m_cfuncsCache(nodep);
        ఎంపిక (విలువ.m_state) {
        స్థితి CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_timingDomains);

            // Visit
            విలువ.m_state = CFuncCache::VISITING;
            m_call_దొంతర.నెట్టు_వెనుక(nodep);
            {
                VL_RESTORER(m_cfuncp);
                m_cfuncp = nodep;
                iterateChildren(nodep);
            }
            m_call_దొంతర.తీయి_వెనుక();
            విలువ.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::మార్పిడి(m_timingDomains, విలువ.m_timingDomains);
        } ఆపు;
        స్థితి CFuncCache::VISITING: {
            కొరకు (కొలత_t i = m_call_దొంతర.కొలత() - 1; m_call_దొంతర.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_దొంతర[i]).m_includes.చొప్పించు(nodep);
            }
            తిరిగి;  // Break recursion
        }
        స్థితి CFuncCache::INITIALIZED: ఆపు;
        }
        std::unordered_సమితి<స్థిరరాశి AstCFunc*> సందర్శించిన;
        కూడు_CFunc_Cached_విలువలు(nodep, సందర్శించిన);
    }

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override {
        iterateChildren(nodep);
        // Decide which scheme to use for each variable and do the 'prepare' step
        కొరకు (AstVarScope* స్థిరరాశి vscp : m_vscps) {
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            vscpInfo.m_scheme = ఎంచుకో_Scheme(vscp, vscpInfo);
            // Run 'prepare' step
            ఎంపిక (vscpInfo.m_scheme) {
            స్థితి Scheme::Undecided:  // LCOV_EXCL_START
                UASSERT_OBJ(అబద్ధం, vscp, "Failed to choose NBA scheme");
                ఆపు;  // LCOV_EXCL_STOP
            స్థితి Scheme::Unsupported_Compound_Array_లోపల_Loop: {
                // Will report error at the site of the NBA
                ఆపు;
            }
            స్థితి Scheme::ShadowVar: {
                ++m_nSchemeShadowVar;
                prepareSchemeShadowVar(vscp, vscpInfo);
                ఆపు;
            }
            స్థితి Scheme::ShadowVarMasked: {
                ++m_nSchemeShadowVarMasked;
                prepareSchemeShadowVarMasked(vscp, vscpInfo);
                ఆపు;
            }
            స్థితి Scheme::FlagShared: {
                ++m_nSchemeFlagShared;
                prepareSchemeFlagShared(vscp, vscpInfo);
                ఆపు;
            }
            స్థితి Scheme::FlagUnique: {
                ++m_nSchemeFlagUnique;
                prepareSchemeFlagUnique(vscp, vscpInfo);
                ఆపు;
            }
            స్థితి Scheme::విలువ_వరుస_సంపూర్ణ: {
                ++m_n_Scheme_విలువ_Queues_సంపూర్ణ;
                prepare_Scheme_విలువ_వరుస</* Partial: */ అబద్ధం>(vscp, vscpInfo);
                ఆపు;
            }
            స్థితి Scheme::విలువ_వరుస_Partial: {
                ++m_n_Scheme_విలువ_Queues_Partial;
                prepare_Scheme_విలువ_వరుస</* Partial: */ నిజం>(vscp, vscpInfo);
                ఆపు;
            }
            }
        }
        // Convert all NBAs
        కొరకు (స్థిరరాశి NBA& nba : m_nbas) {
            Ast_కేటాయించు_Dly* స్థిరరాశి nbap = nba.nodep;
            AstVarScope* స్థిరరాశి vscp = nba.vscp;
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            // Run 'convert' step
            ఎంపిక (vscpInfo.m_scheme) {
            స్థితి Scheme::Undecided: {  // LCOV_EXCL_START
                UASSERT_OBJ(అబద్ధం, vscp, "Unreachable");
                ఆపు;
            }  // LCOV_EXCL_STOP
            స్థితి Scheme::Unsupported_Compound_Array_లోపల_Loop: {
                // TODO: make this an E_UNSUPPORTED...
                nbap->v3warn(BLKLOOPINIT, "Unsupported: Non-blocking assignment to array with "
                                          "compound element type inside loop");
                ఆపు;
            }
            స్థితి Scheme::ShadowVar: {
                convertSchemeShadowVar(nbap, vscp, vscpInfo);
                ఆపు;
            }
            స్థితి Scheme::ShadowVarMasked: {
                convertSchemeShadowVarMasked(nbap, vscp, vscpInfo);
                ఆపు;
            }
            స్థితి Scheme::FlagShared: {
                convertSchemeFlagShared(nbap, vscp, vscpInfo);
                ఆపు;
            }
            స్థితి Scheme::FlagUnique: {
                convertSchemeFlagUnique(nbap, vscp, vscpInfo);
                ఆపు;
            }
            స్థితి Scheme::విలువ_వరుస_సంపూర్ణ: {
                convert_Scheme_విలువ_వరుస(nbap, vscp, vscpInfo, /* partial: */ అబద్ధం);
                ఆపు;
            }
            స్థితి Scheme::విలువ_వరుస_Partial:
                convert_Scheme_విలువ_వరుస(nbap, vscp, vscpInfo, /* partial: */ నిజం);
                ఆపు;
            }
        }
    }
    శూన్య సందర్శించు(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_చురుకైన* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ignoreBlkAndNBlk);
        VL_RESTORER(m_లోపల_Non_Comb_Logic);
        m_activep = nodep;
        స్థిరరాశి Ast_Sen_వృక్షం* స్థిరరాశి senTreep = nodep->sentreep();
        m_ignoreBlkAndNBlk = senTreep->hasStatic() || senTreep->hasInitial();
        m_లోపల_Non_Comb_Logic = senTreep->hasClocked();
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override {
        VL_RESTORER(m_needsInitialTrigger);
        VL_RESTORER_CLEAR(m_nbaEventSenTreeps);
        స్థిరరాశి కొలత_t మొదటి_NBAAdded_సూచీ = m_nbas.కొలత();
        {
            VL_RESTORER(m_లోపల_Suspendable_Or_Fork);
            VL_RESTORER(m_procp);
            VL_RESTORER(m_ignoreBlkAndNBlk);
            VL_RESTORER(m_లోపల_Non_Comb_Logic);
            // When we are dealing with initial block we need to
            // treat it as suspendable when we meet a NBA
            m_లోపల_Suspendable_Or_Fork = nodep->isSuspendable() || VN_IS(nodep, Initial);
            m_procp = nodep;
            ఒకవేళ (nodep->isSuspendable()) {
                m_ignoreBlkAndNBlk = అబద్ధం;
                m_లోపల_Non_Comb_Logic = నిజం;
            }
            iterateChildren(nodep);
        }
        స్వయంచాలక కలిగియుంది_Clocled = [](స్థిరరాశి Ast_Sen_అంశం* itemp) {
            అయితే (itemp) {
                ఒకవేళ (itemp->అంచు_Type().clockedStmt()) తిరిగి నిజం;
                itemp = VN_AS(itemp->nextp(), Sen_అంశం);
            }
            తిరిగి అబద్ధం;
        };
        స్థిరరాశి తార్కిక కూడు_Initial_Trigger = m_needsInitialTrigger
                                       && !(isProcedureWithSentreep<AstAlways>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysObserved>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysReactive>(nodep))
                                       && !కలిగియుంది_Clocled(m_activep->sentreep()->sensesp());
        ఒకవేళ (m_timingDomains.ఖాళీలు() && !కూడు_Initial_Trigger) తిరిగి;

        // There were some timing domains involved in the process. Add all of them as sensitivities
        // of all NBA targets in this process. Note this is a bit of a sledgehammer, we should only
        // need those that directly precede the NBA in control flow, but that is hard to compute,
        // so we will hammer away.

        // First gather all senItems
        Ast_Sen_అంశం* senItemp = శూన్యసూచిక;
        ఒకవేళ (కూడు_Initial_Trigger) {
            senItemp = కొత్త Ast_Sen_అంశం{nodep->fileline(), Ast_Sen_అంశం::InitialNBA{}};
            ++m_nInitialNBA;
        }

        కొరకు (స్థిరరాశి Ast_Sen_వృక్షం* స్థిరరాశి domainp : m_timingDomains) {
            ఒకవేళ (domainp->sensesp())
                senItemp = Ast_కణుపు::కూడు_తదుపరి(senItemp, domainp->sensesp()->clone_వృక్షం(నిజం));
        }
        m_timingDomains.clear();
        // Add them to all nba targets we gathered in this process
        కొరకు (కొలత_t i = మొదటి_NBAAdded_సూచీ; i < m_nbas.కొలత(); ++i) {
            m_vscpInfo(m_nbas[i].vscp).కూడు_సున్నితత్వం(senItemp);
        }
        కొరకు (Ast_Sen_వృక్షం* స్థిరరాశి senTreep : m_nbaEventSenTreeps)
            senTreep->కూడు_Sensesp(senItemp->clone_వృక్షం(నిజం));
        // Done with these
        VL_DO_DANGLING(senItemp->delete_వృక్షం(), senItemp);
    }
    శూన్య సందర్శించు(AstFork* nodep) override {
        VL_RESTORER(m_లోపల_Suspendable_Or_Fork);
        m_లోపల_Suspendable_Or_Fork = నిజం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCAwait* nodep) override {
        ఒకవేళ (nodep->sentreep()) m_timingDomains.చొప్పించు(nodep->sentreep());
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstFireEvent* nodep) override {
        UASSERT_OBJ(v3Global.hasEvents(), nodep, "Inconsistent");
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();

        Ast_కణుపు_సమాసం* స్థిరరాశి eventp = nodep->operandp()->unlink_Fr_వెనుక();

        // Enqueue for clearing 'triggered' state on next eval
        AstCStmt* స్థిరరాశి cstmtp = కొత్త AstCStmt{flp};
        cstmtp->కూడు("vlSymsp->fireEvent(");
        cstmtp->కూడు(eventp);
        cstmtp->కూడు(");");

        Ast_కణుపు* newp = cstmtp;
        స్థిరరాశి AstVarRef* స్థిరరాశి vrefp = VN_CAST(eventp, VarRef);
        ఒకవేళ (nodep->isDelayed() && (m_cfuncp || !vrefp)) {
            // V3Timing converts these with --timing
            nodep->v3warn(E_NOTIMING, "Nonblocking event trigger "
                                          << (m_cfuncp ? "in a non-inlined function/task"
                                                       : "of a class or interface member or "
                                                         "array element")
                                          << " requires --timing");
        } లేకపోతే ఒకవేళ (nodep->isDelayed()) {
            స్థిరరాశి std::string newvarname = "_" + vrefp->varp()->shortName();
            AstVarScope* స్థిరరాశి dlyvscp
                = m_dlyTmps.make(flp, vrefp->varScopep()->scopep(), 1, newvarname);

            స్థిరరాశి స్వయంచాలక dlyRef = [=](VAccess access) {  //
                తిరిగి కొత్త AstVarRef{flp, dlyvscp, access};
            };

            AstAlwaysPost* స్థిరరాశి postp = కొత్త AstAlwaysPost{flp};
            AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, dlyRef(VAccess::READ)};
            postp->కూడు_Stmtsp(ifp);

            UASSERT_OBJ(m_activep, nodep, "No active to handle FireEvent");
            Ast_Sen_వృక్షం* senTreep = m_activep->sentreep();
            Ast_చురుకైన* activep = శూన్యసూచిక;
            ఒకవేళ (m_లోపల_Suspendable_Or_Fork && (senTreep->hasInitial() || senTreep->hasClocked())) {
                // Suspendable code can trigger whenever it resumes, so fire when its NBAs are
                // committed (see visit(AstNodeProcedure*)), clearing the flag like
                // Scheme::FlagUnique
                senTreep = కొత్త Ast_Sen_వృక్షం{
                    flp, senTreep->hasClocked() ? senTreep->sensesp()->clone_వృక్షం(నిజం) : శూన్యసూచిక};
                m_nbaEventSenTreeps.నెట్టు_వెనుక(senTreep);
                m_needsInitialTrigger |= m_timingDomains.ఖాళీలు();
                dlyvscp->varp()->సమితి_Ignore_Post_Write();
                ifp->కూడు_Thensp(కొత్త Ast_కేటాయించు{flp, dlyRef(VAccess::WRITE),
                                             కొత్త AstConst{flp, AstConst::బిట్టు_False{}}});
                activep = కొత్త Ast_చురుకైన{flp, "nba-event", senTreep};
                activep->sen_వృక్షం_Storep(senTreep);
            } లేకపోతే {
                activep = కొత్త Ast_చురుకైన{flp, "nba-event", senTreep};
                AstAlwaysPre* స్థిరరాశి prep = కొత్త AstAlwaysPre{flp};
                prep->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, dlyRef(VAccess::WRITE),
                                              కొత్త AstConst{flp, AstConst::బిట్టు_False{}}});
                activep->కూడు_Stmtsp(prep);
            }
            ifp->కూడు_Thensp(newp);
            m_activep->కూడు_తదుపరి_ఇక్కడ(activep);
            activep->కూడు_Stmtsp(postp);

            newp = కొత్త Ast_కేటాయించు{flp, dlyRef(VAccess::WRITE),
                                 కొత్త AstConst{flp, AstConst::బిట్టు_True{}}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_Dly* nodep) override {
        // Prevent double processing due to AstExprStmt being moved before this node
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;

        ఒకవేళ (m_cfuncp) {
            ఒకవేళ (!v3Global.rootp()->nbaEventp()) {
                nodep->v3warn(
                    E_NOTIMING,
                    "Delayed assignment in a non-inlined function/task requires --timing");
            }
            తిరిగి;
        }
        UASSERT_OBJ(m_procp, nodep, "Delayed assignment not under process");
        UASSERT_OBJ(m_activep, nodep, "<= not under sensitivity block");
        UASSERT_OBJ(m_scopep, nodep, "<= not under scope");
        UASSERT_OBJ(m_లోపల_Suspendable_Or_Fork || m_activep->hasClocked(), nodep,
                    "<= assignment in non-clocked block, should have been converted in V3Active");

        m_needsInitialTrigger |= m_timingDomains.ఖాళీలు();

        // Record scope of this NBA
        nodep->user2p(m_scopep);

        // Grab the reference to the target of the NBA, also lift ExprStmt statements on the LHS
        VL_RESTORER(m_currNbaLhsRefp);
        UASSERT_OBJ(!m_currNbaLhsRefp, nodep, "NBAs should not nest");
        nodep->lhsp()->foreach([&](Ast_కణుపు* currp) {
            // cppcheck-suppress constVariablePointer
            ఒకవేళ (Ast_సమాసం_Stmt* స్థిరరాశి exprp = VN_CAST(currp, సమాసం_Stmt)) {
                // Move statements before the NBA
                nodep->కూడు_ఇక్కడ_This_As_తదుపరి(exprp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
                // Replace with result
                currp->replaceWith(exprp->resultp()->unlink_Fr_వెనుక());
                // Get rid of the AstExprStmt
                VL_DO_DANGLING2(నెట్టు_Deletep(currp), currp, exprp);
            } లేకపోతే ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(currp, VarRef)) {
                // Ignore reads (e.g.: '_[*here*] <= _')
                ఒకవేళ (refp->access().isReadOnly()) తిరిగి;
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
        AstVarScope* స్థిరరాశి vscp = m_currNbaLhsRefp->varScopep();
        // Record it on first encounter
        VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
        ఒకవేళ (!vscpInfo.m_మొదటి_Nba_Refp) {
            vscpInfo.m_మొదటి_Nba_Refp = m_currNbaLhsRefp;
            vscpInfo.m_fistActivep = m_activep;
            m_vscps.emplace_వెనుక(vscp);
        }
        // Note usage context
        vscpInfo.m_సంపూర్ణ |= VN_IS(nodep->lhsp(), VarRef);
        vscpInfo.m_partial |= VN_IS(nodep->lhsp(), Sel);
        vscpInfo.m_లోపల_Loop |= m_లోపల_Loop;
        vscpInfo.m_లోపల_Susp_Or_Fork |= m_లోపల_Suspendable_Or_Fork;
        // Sensitivity might be non-clocked, in a suspendable process, which are handled elsewhere
        ఒకవేళ (m_activep->sentreep()->hasClocked()) {
            ఒకవేళ (vscpInfo.m_fistActivep != m_activep) {
                AstVar* స్థిరరాశి varp = vscp->varp();
                ఒకవేళ (!varp->user1_సమితి_Once()
                    && !varp->fileline()->warnIsOff(V3_Error_సంకేతం::MULTIDRIVEN)) {
                    varp->v3warn(MULTIDRIVEN,
                                 "Signal has multiple driving blocks with different clocking: "
                                     << varp->prettyNameQ() << '\n'
                                     << vscpInfo.m_మొదటి_Nba_Refp->warnOther()
                                     << "... Location of first driving block\n"
                                     << vscpInfo.m_మొదటి_Nba_Refp->warnContextSecondary()
                                     << m_currNbaLhsRefp->warnOther()
                                     << "... Location of other driving block\n"
                                     << m_currNbaLhsRefp->warnContextPrimary() << '\n');
                }
            }
            // Add this sensitivity to the variable
            vscpInfo.కూడు_సున్నితత్వం(m_activep->sentreep());
        }

        // Record the NBA for later processing
        m_nbas.emplace_వెనుక();
        NBA& nba = m_nbas.వెనుక();
        nba.nodep = nodep;
        nba.vscp = vscp;

        // Record write reference
        రికార్డు_Write_Ref(m_currNbaLhsRefp, నిజం);

        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        // Already checked the NBA LHS ref, ignore here
        ఒకవేళ (nodep == m_currNbaLhsRefp) తిరిగి;
        // Only care about write refs
        ఒకవేళ (!nodep->access().isWriteOrRW()) తిరిగి;
        // Record write reference
        రికార్డు_Write_Ref(nodep, అబద్ధం);
    }
    శూన్య సందర్శించు(AstLoop* nodep) override {
        VL_RESTORER(m_లోపల_Loop);
        m_లోపల_Loop = నిజం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_CCall* స్థిరరాశి nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        స్థిరరాశి స్వయంచాలక& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        ఒకవేళ (cfuncps.ఖాళీలు()) {
            సందర్శించు_Called_CFunc(nodep->funcp());
        } లేకపోతే {
            కొరకు (AstCFunc* స్థిరరాశి cfuncp : cfuncps) సందర్శించు_Called_CFunc(cfuncp);
        }
    }
    శూన్య సందర్శించు(AstCFunc* స్థిరరాశి nodep) override {
        స్థిరరాశి స్వయంచాలక& విలువ = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        ఒకవేళ (విలువ.m_state != CFuncCache::UNINITIALIZED) తిరిగి;
        VL_RESTORER(m_cfuncp);
        // Its timing domains are those of the processes calling it, see visitCalledCFunc()
        VL_RESTORER_CLEAR(m_timingDomains);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }

    // Pre/Post logic are created here and their content need no further changes, so ignore.
    శూన్య సందర్శించు(AstAlwaysPre*) override {}
    శూన్య సందర్శించు(AstAlwaysPost*) override {}

    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన DelayedVisitor(AstNetlist* nodep)
        : m_classGraphp{V3_Class_లేఖాచిత్రం::build(nodep)} {
        iterate(nodep);
    }
    ~DelayedVisitor() override {
        V3Stats::కూడు_Stat("NBA, variables using ShadowVar scheme", m_nSchemeShadowVar);
        V3Stats::కూడు_Stat("NBA, variables using ShadowVarMasked scheme", m_nSchemeShadowVarMasked);
        V3Stats::కూడు_Stat("NBA, variables using FlagShared scheme", m_nSchemeFlagShared);
        V3Stats::కూడు_Stat("NBA, variables using FlagUnique scheme", m_nSchemeFlagUnique);
        V3Stats::కూడు_Stat("NBA, variables using ValueQueueWhole scheme", m_n_Scheme_విలువ_Queues_సంపూర్ణ);
        V3Stats::కూడు_Stat("NBA, variables using ValueQueuePartial scheme",
                         m_n_Scheme_విలువ_Queues_Partial);
        V3Stats::కూడు_Stat("Optimizations, NBA flags shared", m_n_Shared_సమితి_Flags);
        V3Stats::కూడు_Stat("Procedures needing initial NBA trigger", m_nInitialNBA);
        V3Stats::కూడు_Stat("Non-inlined co_awaits with SenTree", m_non_Inlined_CAwaits_With_Sen_వృక్షం);
    }
};

//######################################################################
// Delayed class functions

శూన్య V3Delayed::delayedAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DelayedVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("delayed", 0, dump_వృక్షం_Either_Level() >= 3);
}
