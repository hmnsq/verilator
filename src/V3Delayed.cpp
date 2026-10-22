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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Delayed.h"

#অন্তর্ভাব "V3AstUserAllocator.h"
#অন্তর্ভাব "V3ClassGraph.h"
#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3LinkLValue.h"
#অন্তর্ভাব "V3SharedTmps.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <deque>
#অন্তর্ভাব <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert AstAssignDlys (NBAs)

বর্গ DelayedVisitor final : সার্বজনিক VNVisitor {
    // TYPES

    // The various NBA conversion schemes, including error cases
    গণনা বর্গ Scheme : uint8_t {
        Undecided = 0,
        UnsupportedCompoundArrayInLoop,
        ShadowVar,
        ShadowVarMasked,
        FlagShared,
        FlagUnique,
        মান_Queue_Whole,
        মান_Queue_Partial
    };

    // All info associated with a variable that is the target of an NBA
    বর্গ VarScopeInfo final {
    সার্বজনিক:
        // First write reference encountered to the VarScope as the target on an NBA
        ধ্রুব AstVarRef* m_firstNbaRefp = nullptr;
        // Active block 'm_firstNbaRefp' is under
        ধ্রুব AstActive* m_fistActivep = nullptr;
        তার্কিক m_whole = অসত্য;  // Used on LHS of NBA directly via VarRef
        তার্কিক m_partial = অসত্য;  // Used on LHS of NBA under a Sel
        তার্কিক m_inLoop = অসত্য;  // Used on LHS of NBA in a loop
        তার্কিক m_inSuspOrFork = অসত্য;  // Used on LHS of NBA in suspendable process or fork
        Scheme m_scheme = Scheme::Undecided;  // Conversion scheme to use for this variable

    গুপ্ত:
        // Combined sensitivities of all NBAs targeting this variable
        Ast_Sen_বৃক্ষ* m_senTreep = nullptr;

        // Union of stuff needed for the various schemes - use accessors below!
        সঙ্ঘ {
            সংরচনা {  // Stuff needed for Scheme::ShadowVar
                AstVarScope* vscp;  // The shadow variable
            } m_shadowVariableKit;
            সংরচনা {  // Stuff needed for Scheme::ShadowVarMasked
                AstVarScope* vscp;  // The shadow variable
                AstVarScope* maskp;  // The mask variable
            } m_shadowVarMaskedKit;
            সংরচনা {  // Stuff needed for Scheme::FlagShared
                AstActive* activep;  // The active block for the Pre/Post logic
                AstAlwaysPost* postp;  // The post block for commiting results
                AstVarScope* commitFlagp;  // The commit flag variable, for reuse
                AstIf* commitIfp;  // The previous if statement for committing, for reuse
            } m_flagSharedKit;
            সংরচনা {  // Stuff needed for Scheme::FlagUnique
                AstAlwaysPost* postp;  // The post block for commiting results
            } m_flagUniqueKit;
            সংরচনা {  // Stuff needed for Scheme::ValueQueueWhole/Scheme::ValueQueuePartial
                AstVarScope* vscp;  // The commit queue variable
            } m_মান_Queue_Kit;
        } m_kitUnion;

    সার্বজনিক:
        VarScopeInfo() = স্বভাব;
        ~VarScopeInfo() {
            // Might not be linked if there was an error
            যদি (!m_senTreep->backp()) VL_DO_DANGLING(m_senTreep->delete_বৃক্ষ(), m_senTreep);
        }
        // Accessors for the above union fields
        auto& shadowVariableKit() {
            UASSERT(m_scheme == Scheme::ShadowVar, "Inconsistent Scheme");
            প্রত্যাগম m_kitUnion.m_shadowVariableKit;
        }
        auto& shadowVarMaskedKit() {
            UASSERT(m_scheme == Scheme::ShadowVarMasked, "Inconsistent Scheme");
            প্রত্যাগম m_kitUnion.m_shadowVarMaskedKit;
        }
        auto& flagSharedKit() {
            UASSERT(m_scheme == Scheme::FlagShared, "Inconsistent Scheme");
            প্রত্যাগম m_kitUnion.m_flagSharedKit;
        }
        auto& flagUniqueKit() {
            UASSERT(m_scheme == Scheme::FlagUnique, "Inconsistent Scheme");
            প্রত্যাগম m_kitUnion.m_flagUniqueKit;
        }
        auto& মান_Queue_Kit() {
            UASSERT(m_scheme == Scheme::মান_Queue_Partial || m_scheme == Scheme::মান_Queue_Whole,
                    "Inconsistent Scheme");
            প্রত্যাগম m_kitUnion.m_মান_Queue_Kit;
        }

        // Accessor
        Ast_Sen_বৃক্ষ* senTreep() ধ্রুব { প্রত্যাগম m_senTreep; }

        // Add sensitivities
        শূন্য addSensitivity(AstSenItem* nodep) {
            যদি (!m_senTreep) m_senTreep = নূতন Ast_Sen_বৃক্ষ{nodep->fileline(), nullptr};
            // Add a copy of each term
            m_senTreep->addSensesp(nodep->clone_বৃক্ষ(সত্য));
            // Remove duplicates
            V3Const::constifyExpensiveEdit(m_senTreep);
        }
        // cppcheck-suppress constParameterPointer
        শূন্য addSensitivity(Ast_Sen_বৃক্ষ* nodep) { addSensitivity(nodep->sensesp()); }
    };

    // Data structure to keep track of all writes to
    সংরচনা WriteReference final {
        AstVarRef* m_refp = nullptr;  // The reference
        তার্কিক m_isNBA = অসত্য;  // True if an NBA write
        তার্কিক m_inNonComb = অসত্য;  // True if reference is known to be in non-combinational logic
        WriteReference() = স্বভাব;
        WriteReference(AstVarRef* refp, তার্কিক isNBA, তার্কিক inNonComb)
            : m_refp{refp}
            , m_isNBA{isNBA}
            , m_inNonComb{inNonComb} {}
    };

    // Data required to lower AstAssignDelay later
    সংরচনা NBA final {
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
    ধ্রুব VNUser1InUse m_user1InUse;
    ধ্রুব VNUser2InUse m_user2InUse;
    ধ্রুব VNUser3InUse m_user3InUse;

    সংরচনা CFuncCache final {
        VInsertionSet<Ast_Sen_বৃক্ষ*> m_timingDomains;  // What shall be added to m_timingDomains
        std::set<AstCFunc*>
            m_includes;  // CFuncs whose CFuncCache shall be included into this - this is used to
                         // break cycles: A->B->A (instead of visiting A while it is still begin
                         // visited B just marks that it includes A)
        গণনা State : uint8_t {
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
    VInsertionSet<Ast_Sen_বৃক্ষ*> m_timingDomains;  // Timing resume domains
    V3SharedTmps m_dlyTmps{"__Vdly", VVarType::BLOCKTEMP};  // Temporary variables
    // Commit queue data types, by element type and partial flag. Shared by all instances,
    // as m_dlyTmps only shares variables with the same data type.
    std::map<std::pair<ধ্রুব Ast_গ্রন্থি_DType*, তার্কিক>, AstNBACommitQueueDType*> m_cqDTypeps;

    ধ্রুব std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::vector<ধ্রুব AstCFunc*> m_call_স্তূপ;  // Current callstack of AstCFuncs

    // STATE - for current visit position (use VL_RESTORER)
    AstActive* m_activep = nullptr;  // Current activate
    ধ্রুব AstCFunc* m_cfuncp = nullptr;  // Current public C Function
    Ast_গ্রন্থি_Procedure* m_procp = nullptr;  // Current process
    AstScope* m_scopep = nullptr;  // Current scope
    তার্কিক m_inLoop = অসত্য;  // True in for loops
    তার্কিক m_inSuspendableOrFork = অসত্য;  // True in suspendable processes and forks
    তার্কিক m_ignoreBlkAndNBlk = অসত্য;  // Suppress delayed assignment BLKANDNBLK
    তার্কিক m_inNonCombLogic = অসত্য;  // We are in non-combinational logic
    তার্কিক m_needsInitialTrigger = অসত্য;  // Whether a NodeProcedure needs a initial trigger
    std::vector<Ast_Sen_বৃক্ষ*> m_nbaEventSenTreeps;  // Sensitivities of '->>' in the process
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
    VDouble0 m_n_Scheme_মান_Queues_Whole;  //  Number of variables using Scheme::ValueQueueWhole
    VDouble0 m_n_Scheme_মান_Queues_Partial;  //  Number of variables using Scheme::ValueQueuePartial
    VDouble0 m_nSharedSetFlags;  // "Set" flags actually shared by Scheme::FlagShared variables
    VDouble0 m_nInitialNBA;  // Number of procedural blocks with initial NBA
    VDouble0 m_non_Inlined_CAwaits_With_Sen_বৃক্ষ;  // Count uses of not inlined co_awaits

    // METHODS

    // Return true iff a variable is assigned by both blocking and nonblocking
    // assignments. Issue BLKANDNBLK error if we can't prove the mixed
    // assignments are to independent bits and the blocking assignment can be
    // in combinational logic, which is something we can't safely implement
    // still.
    তার্কিক checkMixedUsage(ধ্রুব AstVarScope* vscp, তার্কিক isIntegralOrPacked) {

        সংরচনা Ref final {
            AstVarRef* m_refp;  // The reference
            তার্কিক m_inNonComb;  // True if known to be in non-combinational logic
            পূর্ণাঙ্ক m_lsb;  // LSB of accessed range
            পূর্ণাঙ্ক m_msb;  // MSB of accessed range
            Ref(AstVarRef* refp, তার্কিক inNonComb, পূর্ণাঙ্ক lsb, পূর্ণাঙ্ক msb)
                : m_refp{refp}
                , m_inNonComb{inNonComb}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };

        std::vector<Ref> blkRefs;  // Blocking writes
        std::vector<Ref> nbaRefs;  // Non-blockign writes

        ধ্রুব পূর্ণাঙ্ক width = isIntegralOrPacked ? vscp->width() : 1;

        প্রতি (ধ্রুব auto& writeRef : m_writeRefs(vscp)) {
            পূর্ণাঙ্ক lsb = 0;
            পূর্ণাঙ্ক msb = width - 1;
            যদি (ধ্রুব AstSel* ধ্রুব selp = VN_CAST(writeRef.m_refp->backp(), Sel)) {
                যদি (VN_IS(selp->lsbp(), Const)) {
                    lsb = selp->lsbConst();
                    msb = selp->msbConst();
                }
            }
            যদি (writeRef.m_isNBA) {
                nbaRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            } অন্যথা {
                blkRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            }
        }
        // We only run this function on targets of NBAs, so there should be at least one...
        UASSERT_OBJ(!nbaRefs.empty(), vscp, "Did not record NBA write");
        // If no blocking upadte, then we are good
        যদি (blkRefs.empty()) প্রত্যাগম অসত্য;

        // If the blocking assignment is in non-combinational logic (i.e.:
        // in logic that has an explicit trigger), then we can safely
        // implement it (there is no race between clocked logic and post
        // scheduled logic), so need not error
        blkRefs.erase(std::remove_if(blkRefs.begin(), blkRefs.end(),
                                     [](ধ্রুব Ref& ref) { প্রত্যাগম ref.m_inNonComb; }),
                      blkRefs.end());

        // If nothing left, then we need not error
        যদি (blkRefs.empty()) প্রত্যাগম সত্য;

        // If not a packed variable, warn here as we can't prove independence
        যদি (!isIntegralOrPacked) {
            ধ্রুব Ref& blkRef = blkRefs.front();
            ধ্রুব Ref& nbaRef = nbaRefs.front();
            vscp->v3warn(
                BLKANDNBLK,
                "Unsupported: Blocking and non-blocking assignments to same non-packed variable: "
                    << vscp->varp()->prettyNameQ() << '\n'
                    << vscp->warnContextPrimary() << '\n'
                    << blkRef.m_refp->warnOther() << "... Location of blocking assignment\n"
                    << blkRef.m_refp->warnContextSecondary() << '\n'
                    << nbaRef.m_refp->warnOther() << "... Location of nonblocking assignment\n"
                    << nbaRef.m_refp->warnContextSecondary());
            প্রত্যাগম সত্য;
        }

        // We need to error if we can't prove the written bits are independent

        // Sort refs by interval
        ধ্রুব auto lessThanRef = [](ধ্রুব Ref& a, ধ্রুব Ref& b) {
            যদি (a.m_lsb != b.m_lsb) প্রত্যাগম a.m_lsb < b.m_lsb;
            প্রত্যাগম a.m_msb < b.m_msb;
        };
        std::stable_sort(blkRefs.begin(), blkRefs.end(), lessThanRef);
        std::stable_sort(nbaRefs.begin(), nbaRefs.end(), lessThanRef);
        // Iterate both vectors, checking for overlap
        auto bIt = blkRefs.begin();
        auto nIt = nbaRefs.begin();
        যাবত্ (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            যদি (lessThanRef(*bIt, *nIt)) {
                যদি (nIt->m_lsb <= bIt->m_msb) ভঙ্গ;  // Stop on Overlap
                ++bIt;
            } অন্যথা {
                যদি (bIt->m_lsb <= nIt->m_msb) ভঙ্গ;  // Stop on Overlap
                ++nIt;
            }
        }

        // If we found an overlapping range that cannot be safely implemented, then wran...
        যদি (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            ধ্রুব Ref& blkRef = *bIt;
            ধ্রুব Ref& nbaRef = *nIt;
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

        প্রত্যাগম সত্য;
    }

    // Choose the NBA scheme used for the given variable.
    Scheme chooseScheme(ধ্রুব AstVarScope* vscp, ধ্রুব VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::Undecided, vscp, "NBA scheme already decided");

        ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtypep = vscp->dtypep()->skipRefp();
        // Unpacked arrays
        যদি (ধ্রুব AstUnpackArrayDType* ধ্রুব uaDTypep = VN_CAST(dtypep, UnpackArrayDType)) {
            // If whole array is target of NBA, use ShadowVar, but not in a suspendable process or
            // fork, which resume outside the 'nba' region, so its 'pre' logic would overwrite the
            // value they write in the shadow variable
            যদি (vscpInfo.m_whole) {
                প্রত্যাগম vscpInfo.m_inSuspOrFork ? Scheme::FlagUnique : Scheme::ShadowVar;
            }
            // Basic underlying type of elements, if any.
            ধ্রুব AstBasicDType* ধ্রুব basicp = uaDTypep->basicp();
            // If used in a loop, we must have a dynamic commit queue. (Also works in suspendables)
            যদি (vscpInfo.m_inLoop) {
                // Arrays with compound element types are currently not supported in loops
                যদি (!basicp
                    || !(basicp->isIntegralOrPacked()  //
                         || basicp->isDouble()  //
                         || basicp->isString())) {
                    প্রত্যাগম Scheme::UnsupportedCompoundArrayInLoop;
                }
                যদি (vscpInfo.m_partial) প্রত্যাগম Scheme::মান_Queue_Partial;
                প্রত্যাগম Scheme::মান_Queue_Whole;
            }
            // A suspendable process or fork can resume outside the 'nba' region, so the 'pre'
            // logic of the other schemes would undo its updates. The unique flag scheme has none.
            যদি (vscpInfo.m_inSuspOrFork) প্রত্যাগম Scheme::FlagUnique;
            // Otherwise if an array of packed/basic elements, use the shared flag scheme
            যদি (basicp) প্রত্যাগম Scheme::FlagShared;
            // Finally fall back on the shadow variable scheme, e.g. for
            // arrays of unpacked structs. This will be slow.
            // TODO: generic LHS scheme as discussed in #5092
            প্রত্যাগম Scheme::ShadowVar;
        }

        // Likewise, the 'pre' logic of the shadow variable scheme would undo the updates of a
        // suspendable process or fork, so use the unique flag scheme
        যদি (vscpInfo.m_inSuspOrFork) প্রত্যাগম Scheme::FlagUnique;

        ধ্রুব তার্কিক isIntegralOrPacked = dtypep->isIntegralOrPacked();
        // Check for mixed usage (this also warns if not OK)
        যদি (checkMixedUsage(vscp, isIntegralOrPacked)) {
            // If it's a variable updated by both blocking and non-blocking
            // assignments, use the ShadowVarMasked schem if masked update is
            // possible. This can handle blocking and non-blocking updates to
            // inpdendent parts correctly at run-time, and always works, even
            // in loops or other dynamic context.
            যদি (isIntegralOrPacked) প্রত্যাগম Scheme::ShadowVarMasked;
            // If it's inside a loop, use Scheme::ShadowVar, which is safe,
            // but will generate incorrect code if a partial update is used
            যদি (vscpInfo.m_inLoop) প্রত্যাগম Scheme::ShadowVar;
            // Otherwise (for not packed variables), use the FlagUnique scheme,
            // which at least handles partial updates correctly, but might break
            // in loops or other dynamic context
            প্রত্যাগম Scheme::FlagUnique;
        }

        // Otherwise use the simple shadow variable scheme
        প্রত্যাগম Scheme::ShadowVar;
    }

    // Given an expression 'exprp', return a new expression that always evaluates to the
    // value of the given expression at this point in the program. That is:
    // - If given a non-constant expression, create a new temporary AstVarScope under the given
    //   'scopep', with the given 'name', assign the expression to it, and return a read reference
    //   to the new AstVarScope.
    // - If given a constant, just return that constant.
    // New statements are inserted before 'insertp'
    Ast_গ্রন্থি_রাশিমালা* captureVal(AstScope* ধ্রুব scopep, Ast_গ্রন্থি_Stmt* ধ্রুব insertp,
                            Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp, ধ্রুব std::string& name) {
        UASSERT_OBJ(!exprp->backp(), exprp, "Should have been unlinked");
        File_সারি* ধ্রুব flp = exprp->fileline();
        যদি (VN_IS(exprp, Const)) প্রত্যাগম exprp;
        // TODO: there are some const variables that could be simply referenced here
        AstVarScope* ধ্রুব tmpVscp = m_dlyTmps.make(flp, scopep, exprp->dtypep(), name);
        insertp->addHereThisAsNext(
            নূতন AstAssign{flp, নূতন AstVarRef{flp, tmpVscp, VAccess::WRITE}, exprp});
        প্রত্যাগম নূতন AstVarRef{flp, tmpVscp, VAccess::READ};
    };

    // Similar to 'captureVal', but captures an LValue expression. That is, the returned
    // expression will reference the same location as the input expression, at this point in the
    // program.
    Ast_গ্রন্থি_রাশিমালা* captureLhs(AstScope* ধ্রুব scopep, Ast_গ্রন্থি_Stmt* ধ্রুব insertp,
                            Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp, ধ্রুব std::string& baseName) {
        UASSERT_OBJ(!lhsp->backp(), lhsp, "Should have been unlinked");
        // Running node pointer
        Ast_গ্রন্থি_রাশিমালা* nodep = lhsp;
        // Capture AstSel indices - there should be only one
        যদি (AstSel* ধ্রুব selp = VN_CAST(nodep, Sel)) {
            ধ্রুব std::string tmpName{"Lsb" + baseName};
            selp->lsbp(captureVal(scopep, insertp, selp->lsbp()->unlinkFrBack(), tmpName));
            // Continue with target
            nodep = selp->fromp();
        }
        UASSERT_OBJ(!VN_IS(nodep, Sel), lhsp, "Multiple 'AstSel' applied to LHS reference");
        // Capture the indices of selects - might be many, also below struct members. If a handle
        // selects the instance the target is a member of, capture the handle instead, as it is
        // only read, to select the target when the NBA is executed (IEEE 1800-2023 10.4.2).
        size_t nArraySels = 0;
        যাবত্ (সত্য) {
            যদি (Ast_গ্রন্থি_Sel* ধ্রুব selp = VN_CAST(nodep, গ্রন্থি_Sel)) {
                ধ্রুব std::string tmpName{"Dim" + std::to_string(nArraySels++) + baseName};
                selp->bitp(captureVal(scopep, insertp, selp->bitp()->unlinkFrBack(), tmpName));
                nodep = selp->fromp();
            } অন্যথা যদি (ধ্রুব AstStructSel* ধ্রুব selp = VN_CAST(nodep, StructSel)) {
                nodep = selp->fromp();
            } অন্যথা যদি (AstMemberSel* ধ্রুব selp = VN_CAST(nodep, MemberSel)) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব handlep = selp->fromp()->unlinkFrBack();
                V3LinkLValue::linkLValueSet(handlep, VAccess::READ);
                AstVarRef* ধ্রুব refp
                    = VN_AS(captureVal(scopep, insertp, handlep, "Handle" + baseName), VarRef);
                refp->varp()->setIgnorePostWrite();
                // Like the handle it replaces, mark it written, as is the member it selects
                refp->access(selp->access());
                selp->fromp(refp);
                প্রত্যাগম lhsp;
            } অন্যথা {
                ভঙ্গ;
            }
        }
        // What remains must be the variable, or some sort of select, we assume can reuse it
        UASSERT_OBJ(nodep->isPure(), lhsp, "Malformed LHS in NBA");
        // Now have been converted to use the captured values
        প্রত্যাগম lhsp;
    }

    শূন্য add_CFunc_Cached_মানসমূহ(ধ্রুব AstCFunc* ধ্রুব cfuncp,
                              std::unordered_set<ধ্রুব AstCFunc*>& visited) {
        যদি (!visited.ঢোকাও(cfuncp).second) প্রত্যাগম;
        CFuncCache& মান = m_cfuncsCache(cfuncp);
        m_timingDomains.ঢোকাও(মান.m_timingDomains.begin(), মান.m_timingDomains.end());
        m_non_Inlined_CAwaits_With_Sen_বৃক্ষ += মান.m_timingDomains.size();
        প্রতি (ধ্রুব AstCFunc* ধ্রুব includedp : মান.m_includes) {
            add_CFunc_Cached_মানসমূহ(includedp, visited);
        }
    }

    // Create a temporary variable in the given 'scopep', with the given 'name', and with 'dtypep'
    // type, with the bits selected by 'sLsbp'/'sWidthp' set to 'valuep', other bits set to zero.
    // Insert new statements before 'insertp'.
    // Returns a read reference to the temporary variable.
    AstVarRef* createWidened(File_সারি* flp, AstScope* scopep, Ast_গ্রন্থি_DType* dtypep,
                             Ast_গ্রন্থি_রাশিমালা* sLsbp, পূর্ণাঙ্ক sWidth, ধ্রুব std::string& name,
                             Ast_গ্রন্থি_রাশিমালা* valuep, Ast_গ্রন্থি* insertp) {
        // Create temporary variable.
        AstVarScope* ধ্রুব tp = m_dlyTmps.make(flp, scopep, dtypep, name);
        // Zero it
        AstConst* ধ্রুব zerop = নূতন AstConst{flp, AstConst::DTyped{}, dtypep};
        zerop->অঙ্ক().setAllBits0();
        insertp->addHereThisAsNext(
            নূতন AstAssign{flp, নূতন AstVarRef{flp, tp, VAccess::WRITE}, zerop});
        // Set the selected bits to 'valuep'
        AstSel* ধ্রুব selp = নূতন AstSel{flp, নূতন AstVarRef{flp, tp, VAccess::WRITE},
                                        sLsbp->clone_বৃক্ষ_Pure(সত্য), sWidth};
        insertp->addHereThisAsNext(নূতন AstAssign{flp, selp, valuep});
        // This is the expression to get the value of the temporary
        প্রত্যাগম নূতন AstVarRef{flp, tp, VAccess::READ};
    }

    // Scheme::ShadowVar
    শূন্য prepareSchemeShadowVar(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        File_সারি* ধ্রুব flp = vscp->fileline();
        AstScope* ধ্রুব scopep = vscp->scopep();
        // Create the shadow variable
        ধ্রুব std::string name = "_" + vscp->varp()->shortName();
        AstVarScope* ধ্রুব shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), name);
        vscpInfo.shadowVariableKit().vscp = shadowVscp;
        // Mark both for V3LifePsot
        vscp->optimizeLifePost(সত্য);
        shadowVscp->optimizeLifePost(সত্য);
        // Create the AstActive for the Pre/Post logic
        AstActive* ধ্রুব activep = নূতন AstActive{flp, "nba-shadow-variable", vscpInfo.senTreep()};
        activep->sen_বৃক্ষ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Pre' scheduled 'shadowVariable = originalVariable' assignment
        AstAlwaysPre* ধ্রুব prep = নূতন AstAlwaysPre{flp};
        activep->addStmtsp(prep);
        prep->addStmtsp(নূতন AstAssign{flp, নূতন AstVarRef{flp, shadowVscp, VAccess::WRITE},
                                      নূতন AstVarRef{flp, vscp, VAccess::READ}});
        // Add 'Post' scheduled 'originalVariable = shadowVariable' assignment
        AstAlwaysPost* ধ্রুব postp = নূতন AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        postp->addStmtsp(নূতন AstAssign{flp, নূতন AstVarRef{flp, vscp, VAccess::WRITE},
                                       নূতন AstVarRef{flp, shadowVscp, VAccess::READ}});
    }
    শূন্য convertSchemeShadowVar(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        AstVarScope* ধ্রুব shadowVscp = vscpInfo.shadowVariableKit().vscp;

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ধ্রুব refp) {
            যদি (!refp->access().isWriteOnly()) প্রত্যাগম;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
    }

    // Scheme::ShadowVarMasked
    শূন্য prepareSchemeShadowVarMasked(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        File_সারি* ধ্রুব flp = vscp->fileline();
        AstScope* ধ্রুব scopep = vscp->scopep();
        // Create the shadow variable
        ধ্রুব std::string shadowName = "_" + vscp->varp()->shortName();
        AstVarScope* ধ্রুব shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), shadowName);
        vscpInfo.shadowVarMaskedKit().vscp = shadowVscp;
        // Create the makk variable
        ধ্রুব std::string maskName = "Mask__" + vscp->varp()->shortName();
        AstVarScope* ধ্রুব maskVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), maskName);
        maskVscp->varp()->setIgnorePostWrite();
        vscpInfo.shadowVarMaskedKit().maskp = maskVscp;
        // Create the AstActive for the Post logic
        AstActive* ধ্রুব activep
            = নূতন AstActive{flp, "nba-shadow-var-masked", vscpInfo.senTreep()};
        activep->sen_বৃক্ষ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit and mask clear
        AstAlwaysPost* ধ্রুব postp = নূতন AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit - vscp = (shadowVscp & maskVscp) | (vscp & ~maskVscp);
        postp->addStmtsp(নূতন AstAssign{
            flp, নূতন AstVarRef{flp, vscp, VAccess::WRITE},
            নূতন AstOr{flp,
                      নূতন AstAnd{flp, নূতন AstVarRef{flp, shadowVscp, VAccess::READ},
                                 নূতন AstVarRef{flp, maskVscp, VAccess::READ}},
                      নূতন AstAnd{flp, নূতন AstVarRef{flp, vscp, VAccess::READ},
                                 নূতন AstNot{flp, নূতন AstVarRef{flp, maskVscp, VAccess::READ}}}}});
        vscp->varp()->setIgnorePostRead();
        // Clar the mask - maskVscp = '0;
        postp->addStmtsp(
            নূতন AstAssign{flp, নূতন AstVarRef{flp, maskVscp, VAccess::WRITE},
                          নূতন AstConst{flp, AstConst::Widthed_মান{}, maskVscp->width(), 0}});
    }
    শূন্য convertSchemeShadowVarMasked(AstAssignDly* nodep, AstVarScope* vscp,
                                      VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        AstVarScope* ধ্রুব shadowVscp = vscpInfo.shadowVarMaskedKit().vscp;
        AstVarScope* ধ্রুব maskVscp = vscpInfo.shadowVarMaskedKit().maskp;

        Ast_গ্রন্থি_রাশিমালা* lhsClonep = nodep->lhsp()->clone_বৃক্ষ(অসত্য);

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ধ্রুব refp) {
            যদি (!refp->access().isWriteOnly()) প্রত্যাগম;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
        // Set the same bits in the mask to 1
        lhsClonep->foreach([&](AstVarRef* ধ্রুব refp) {
            যদি (!refp->access().isWriteOnly()) প্রত্যাগম;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(maskVscp);
            refp->varp(maskVscp->varp());
        });
        File_সারি* ধ্রুব flp = nodep->fileline();
        AstConst* ধ্রুব onesp = নূতন AstConst{flp, AstConst::DTyped{}, lhsClonep->dtypep()};
        onesp->অঙ্ক().setAllBits1();
        nodep->addNextHere(নূতন AstAssign{flp, lhsClonep, onesp});
    }

    // Scheme::FlagShared
    শূন্য prepareSchemeFlagShared(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_সারি* ধ্রুব flp = vscp->fileline();
        AstScope* ধ্রুব scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* ধ্রুব activep = নূতন AstActive{flp, "nba-flag-shared", vscpInfo.senTreep()};
        activep->sen_বৃক্ষ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        vscpInfo.flagSharedKit().activep = activep;
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ধ্রুব postp = নূতন AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagSharedKit().postp = postp;
        // Initialize
        vscpInfo.flagSharedKit().commitFlagp = nullptr;
        vscpInfo.flagSharedKit().commitIfp = nullptr;
    }
    শূন্য convertSchemeFlagShared(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_সারি* ধ্রুব flp = vscp->fileline();
        AstScope* ধ্রুব scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ধ্রুব std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Is this NBA adjacent after the previously processed NBA?
        ধ্রুব তার্কিক consecutive = nodep == m_nextDlyp;
        m_nextDlyp = VN_CAST(nodep->nextp(), AssignDly);

        VarScopeInfo* ধ্রুব prevVscpInfop = consecutive ? &m_vscpInfo(m_prevVscp) : nullptr;

        // We can reuse the flag of the previous assignment if:
        ধ্রুব তার্কিক reuseTheFlag =
            // Consecutive NBAs
            consecutive
            // ... that use the same scheme
            && prevVscpInfop->m_scheme == Scheme::FlagShared
            // ... and are in the same scope as the target variable
            && scopep == vscp->scopep()
            // ... and share the same overall update domain
            && prevVscpInfop->senTreep()->same_বৃক্ষ(vscpInfo.senTreep());

        যদি (!reuseTheFlag) {
            // Create new flag
            AstVarScope* ধ্রুব flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
            // Set the flag at the original NBA
            nodep->addHereThisAsNext(  //
                নূতন AstAssign{flp, নূতন AstVarRef{flp, flagVscp, VAccess::WRITE},
                              নূতন AstConst{flp, AstConst::BitTrue{}}});
            // Add the 'Pre' scheduled reset for the flag
            AstAlwaysPre* ধ্রুব prep = নূতন AstAlwaysPre{flp};
            vscpInfo.flagSharedKit().activep->addStmtsp(prep);
            prep->addStmtsp(নূতন AstAssign{flp, নূতন AstVarRef{flp, flagVscp, VAccess::WRITE},
                                          নূতন AstConst{flp, AstConst::BitFalse{}}});
            // Add the 'Post' scheduled commit
            AstIf* ধ্রুব ifp = নূতন AstIf{flp, নূতন AstVarRef{flp, flagVscp, VAccess::READ}};
            vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
            vscpInfo.flagSharedKit().commitFlagp = flagVscp;
            vscpInfo.flagSharedKit().commitIfp = ifp;
        } অন্যথা {
            যদি (vscp != m_prevVscp) {
                // Different variable, ensure the commit block exists for this variable,
                // can reuse existing one with the same flag, otherwise create a new one.
                AstVarScope* ধ্রুব flagVscp = prevVscpInfop->flagSharedKit().commitFlagp;
                UASSERT_OBJ(flagVscp, nodep, "Commit flag of previous assignment should exist");
                যদি (vscpInfo.flagSharedKit().commitFlagp != flagVscp) {
                    AstIf* ধ্রুব ifp = নূতন AstIf{flp, নূতন AstVarRef{flp, flagVscp, VAccess::READ}};
                    vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
                    vscpInfo.flagSharedKit().commitFlagp = flagVscp;
                    vscpInfo.flagSharedKit().commitIfp = ifp;
                }
            } অন্যথা {
                // Same variable, reuse the commit block of the previous assignment
                vscpInfo.flagSharedKit().commitFlagp = prevVscpInfop->flagSharedKit().commitFlagp;
                vscpInfo.flagSharedKit().commitIfp = prevVscpInfop->flagSharedKit().commitIfp;
            }
            ++m_nSharedSetFlags;
        }
        // Commit the captured value to the captured destination
        vscpInfo.flagSharedKit().commitIfp->addThensp(
            নূতন AstAssign{flp, capturedLhsp, capturedRhsp});

        // Remember the variable for the next NBA
        m_prevVscp = vscp;

        // Delete original NBA
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::FlagUnique
    শূন্য prepareSchemeFlagUnique(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_সারি* ধ্রুব flp = vscp->fileline();
        AstScope* ধ্রুব scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* ধ্রুব activep = নূতন AstActive{flp, "nba-flag-unique", vscpInfo.senTreep()};
        activep->sen_বৃক্ষ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ধ্রুব postp = নূতন AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagUniqueKit().postp = postp;
    }
    শূন্য convertSchemeFlagUnique(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_সারি* ধ্রুব flp = vscp->fileline();
        AstScope* ধ্রুব scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ধ্রুব std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Create new flag
        AstVarScope* ধ্রুব flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
        flagVscp->varp()->setIgnorePostWrite();
        // Set the flag at the original NBA
        nodep->addHereThisAsNext(  //
            নূতন AstAssign{flp, নূতন AstVarRef{flp, flagVscp, VAccess::WRITE},
                          নূতন AstConst{flp, AstConst::BitTrue{}}});
        // Add the 'Post' scheduled commit
        AstIf* ধ্রুব ifp = নূতন AstIf{flp, নূতন AstVarRef{flp, flagVscp, VAccess::READ}};
        vscpInfo.flagUniqueKit().postp->addStmtsp(ifp);
        // Immediately clear the flag
        ifp->addThensp(নূতন AstAssign{flp, নূতন AstVarRef{flp, flagVscp, VAccess::WRITE},
                                     নূতন AstConst{flp, AstConst::BitFalse{}}});
        // Commit the value
        ifp->addThensp(নূতন AstAssign{flp, capturedLhsp, capturedRhsp});

        // Delete original NBA
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::ValueQueuePartial/Scheme::ValueQueueWhole
    template <তার্কিক N_Partial>
    শূন্য prepare_Scheme_মান_Queue(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(N_Partial ? vscpInfo.m_scheme == Scheme::মান_Queue_Partial
                              : vscpInfo.m_scheme == Scheme::মান_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_সারি* ধ্রুব flp = vscp->fileline();
        AstScope* ধ্রুব scopep = vscp->scopep();

        // Create the commit queue variable
        Ast_গ্রন্থি_DType* ধ্রুব elemDTypep = vscp->dtypep()->skipRefp();
        AstNBACommitQueueDType*& cqDTypep = m_cqDTypeps[{elemDTypep, N_Partial}];
        যদি (!cqDTypep) {
            cqDTypep = নূতন AstNBACommitQueueDType{flp, elemDTypep, N_Partial};
            v3Global.rootp()->typeTablep()->addTypesp(cqDTypep);
        }
        ধ্রুব std::string name = "CommitQueue__" + vscp->varp()->shortName();
        AstVarScope* ধ্রুব queueVscp = m_dlyTmps.make(flp, scopep, cqDTypep, name);
        queueVscp->varp()->noReset(সত্য);
        queueVscp->varp()->setIgnorePostWrite();
        vscpInfo.মান_Queue_Kit().vscp = queueVscp;
        // Create the AstActive for the Post logic
        AstActive* ধ্রুব activep
            = নূতন AstActive{flp, "nba-value-queue-whole", vscpInfo.senTreep()};
        activep->sen_বৃক্ষ_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit
        AstAlwaysPost* ধ্রুব postp = নূতন AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit
        AstCMethodHard* ধ্রুব callp = নূতন AstCMethodHard{
            flp, নূতন AstVarRef{flp, queueVscp, VAccess::READWRITE}, VCMethod::NBA_COMMIT};
        callp->dtypeSetVoid();
        // TODO: this is a partial update, so must be READWRITE, but that breaks scheduling
        callp->addPinsp(নূতন AstVarRef{flp, vscp, VAccess::WRITE});
        postp->addStmtsp(callp->makeStmt());
    }

    শূন্য convert_Scheme_মান_Queue(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo,
                                 তার্কিক partial) {
        UASSERT_OBJ(partial ? vscpInfo.m_scheme == Scheme::মান_Queue_Partial
                            : vscpInfo.m_scheme == Scheme::মান_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_সারি* ধ্রুব flp = vscp->fileline();
        AstScope* ধ্রুব scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ধ্রুব std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // RHS value (can be widened/masked iff Partial)
        Ast_গ্রন্থি_রাশিমালা* valuep = capturedRhsp;
        // RHS mask (iff Partial)
        Ast_গ্রন্থি_রাশিমালা* maskp = nullptr;

        // Running node for LHS deconstruction
        Ast_গ্রন্থি_রাশিমালা* lhsNodep = capturedLhsp;

        // If partial updates are required, construct the mask and the widened value
        যদি (partial) {
            // Type of array element
            Ast_গ্রন্থি_DType* ধ্রুব eDTypep = [&]() -> Ast_গ্রন্থি_DType* {
                Ast_গ্রন্থি_DType* dtypep = vscp->dtypep()->skipRefp();
                যাবত্ (AstUnpackArrayDType* ধ্রুব ap = VN_CAST(dtypep, UnpackArrayDType)) {
                    dtypep = ap->subDTypep()->skipRefp();
                }
                প্রত্যাগম dtypep;
            }();

            যদি (ধ্রুব AstSel* ধ্রুব lSelp = VN_CAST(lhsNodep, Sel)) {
                // This is a partial assignment.
                // Need to create a mask and widen the value to element size.
                lhsNodep = lSelp->fromp();
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব sLsbp = lSelp->lsbp();
                ধ্রুব পূর্ণাঙ্ক sWidth = lSelp->widthConst();

                // Create mask value
                maskp = [&]() -> Ast_গ্রন্থি_রাশিমালা* {
                    // Constant mask we can compute here
                    যদি (ধ্রুব AstConst* ধ্রুব cLsbp = VN_CAST(sLsbp, Const)) {
                        AstConst* ধ্রুব cp = নূতন AstConst{flp, AstConst::DTyped{}, eDTypep};
                        cp->অঙ্ক().setMask(sWidth, cLsbp->toSInt());
                        প্রত্যাগম cp;
                    }

                    // A non-constant mask we must compute at run-time.
                    AstConst* ধ্রুব onesp = নূতন AstConst{flp, AstConst::Widthed_মান{}, sWidth, 0};
                    onesp->অঙ্ক().setAllBits1();
                    প্রত্যাগম createWidened(flp, scopep, eDTypep, sLsbp, sWidth, "Mask" + baseName,
                                         onesp, nodep);
                }();

                // Adjust value to element size
                valuep = [&]() -> Ast_গ্রন্থি_রাশিমালা* {
                    // Constant value with constant select we can compute here
                    যদি (AstConst* ধ্রুব cValuep = VN_CAST(valuep, Const)) {
                        যদি (ধ্রুব AstConst* ধ্রুব cLsbp = VN_CAST(sLsbp, Const)) {
                            AstConst* ধ্রুব cp = নূতন AstConst{flp, AstConst::DTyped{}, eDTypep};
                            cp->অঙ্ক().setAllBits0();
                            cp->অঙ্ক().opSelInto(cValuep->অঙ্ক(), cLsbp->toSInt(), sWidth);
                            VL_DO_DANGLING(valuep->delete_বৃক্ষ(), valuep);
                            প্রত্যাগম cp;
                        }
                    }

                    // A non-constant value we must adjust.
                    প্রত্যাগম createWidened(flp, scopep, eDTypep, sLsbp, sWidth,  //
                                         "Elem" + baseName, valuep, nodep);
                }();
            } অন্যথা {
                // If this assignment is not partial, set mask to ones and we are done
                AstConst* ধ্রুব ones = নূতন AstConst{flp, AstConst::DTyped{}, eDTypep};
                ones->অঙ্ক().setAllBits1();
                maskp = ones;
            }
        }

        // Extract array indices
        std::vector<Ast_গ্রন্থি_রাশিমালা*> idxps;
        {
            UASSERT_OBJ(VN_IS(lhsNodep, ArraySel), lhsNodep, "Unexpected LHS form");
            যাবত্ (AstArraySel* ধ্রুব aSelp = VN_CAST(lhsNodep, ArraySel)) {
                idxps.emplace_back(aSelp->bitp()->unlinkFrBack());
                lhsNodep = aSelp->fromp();
            }
            UASSERT_OBJ(VN_IS(lhsNodep, VarRef), lhsNodep, "Unexpected LHS form");
            std::reverse(idxps.begin(), idxps.end());
        }

        // Done with the LHS at this point
        VL_DO_DANGLING(ঠেলো_Deletep(capturedLhsp), capturedLhsp);

        // Enqueue the update at the site of the original NBA
        AstCMethodHard* ধ্রুব callp = নূতন AstCMethodHard{
            flp, নূতন AstVarRef{flp, vscpInfo.মান_Queue_Kit().vscp, VAccess::READWRITE},
            VCMethod::NBA_ENQUEUE};
        callp->dtypeSetVoid();
        callp->addPinsp(valuep);
        যদি (partial) callp->addPinsp(maskp);
        প্রতি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব indexp : idxps) callp->addPinsp(indexp);
        nodep->addHereThisAsNext(callp->makeStmt());

        // Delete original NBA
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Record where a variable is assigned
    শূন্য recordWriteRef(AstVarRef* nodep, তার্কিক nonBlocking) {
        // Ignore references in certain contexts
        যদি (m_ignoreBlkAndNBlk) প্রত্যাগম;
        // Ignore if it's an array
        // TODO: we do this because it used to be the previous behaviour.
        //       Is it still required, or should we warn for arrays as well?
        //       Scheduling is no different for them...
        //       Clarification: This is OK for arrays of primitive types, but
        //       arrays that use the ShadowVar scheme don't work...
        যদি (VN_IS(nodep->varScopep()->dtypep()->skipRefp(), UnpackArrayDType)) প্রত্যাগম;

        m_writeRefs(nodep->varScopep()).emplace_back(nodep, nonBlocking, m_inNonCombLogic);
    }

    template <typename Procedure_T>
    স্থির তার্কিক isProcedureWithSentreep(ধ্রুব Ast_গ্রন্থি_Procedure* ধ্রুব nodep) {
        ধ্রুব Procedure_T* ধ্রুব procedurep = Ast_গ্রন্থি::cast<Procedure_T>(nodep);
        প্রত্যাগম procedurep && procedurep->sentreep();
    }

    // Visit AstCFunc from a AstNodeCCall - this is made into a separate quasi-visitor because
    // AstCFunc that is not called from the code (e.g.: DPI exports) does not need to be visited
    // this way. Also, not visiting such AstCFuncs allows to avoid caching results for them which
    // this function does - which could lead to excessive memory usage
    শূন্য visitCalledCFunc(AstCFunc* ধ্রুব nodep) {
        CFuncCache& মান = m_cfuncsCache(nodep);
        বিকল্প (মান.m_state) {
        পক্ষ CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_timingDomains);

            // Visit
            মান.m_state = CFuncCache::VISITING;
            m_call_স্তূপ.ঠেলো_back(nodep);
            {
                VL_RESTORER(m_cfuncp);
                m_cfuncp = nodep;
                iterateChildren(nodep);
            }
            m_call_স্তূপ.বের_করো_back();
            মান.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_timingDomains, মান.m_timingDomains);
        } ভঙ্গ;
        পক্ষ CFuncCache::VISITING: {
            প্রতি (size_t i = m_call_স্তূপ.size() - 1; m_call_স্তূপ.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_স্তূপ[i]).m_includes.ঢোকাও(nodep);
            }
            প্রত্যাগম;  // Break recursion
        }
        পক্ষ CFuncCache::INITIALIZED: ভঙ্গ;
        }
        std::unordered_set<ধ্রুব AstCFunc*> visited;
        add_CFunc_Cached_মানসমূহ(nodep, visited);
    }

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override {
        iterateChildren(nodep);
        // Decide which scheme to use for each variable and do the 'prepare' step
        প্রতি (AstVarScope* ধ্রুব vscp : m_vscps) {
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            vscpInfo.m_scheme = chooseScheme(vscp, vscpInfo);
            // Run 'prepare' step
            বিকল্প (vscpInfo.m_scheme) {
            পক্ষ Scheme::Undecided:  // LCOV_EXCL_START
                UASSERT_OBJ(অসত্য, vscp, "Failed to choose NBA scheme");
                ভঙ্গ;  // LCOV_EXCL_STOP
            পক্ষ Scheme::UnsupportedCompoundArrayInLoop: {
                // Will report error at the site of the NBA
                ভঙ্গ;
            }
            পক্ষ Scheme::ShadowVar: {
                ++m_nSchemeShadowVar;
                prepareSchemeShadowVar(vscp, vscpInfo);
                ভঙ্গ;
            }
            পক্ষ Scheme::ShadowVarMasked: {
                ++m_nSchemeShadowVarMasked;
                prepareSchemeShadowVarMasked(vscp, vscpInfo);
                ভঙ্গ;
            }
            পক্ষ Scheme::FlagShared: {
                ++m_nSchemeFlagShared;
                prepareSchemeFlagShared(vscp, vscpInfo);
                ভঙ্গ;
            }
            পক্ষ Scheme::FlagUnique: {
                ++m_nSchemeFlagUnique;
                prepareSchemeFlagUnique(vscp, vscpInfo);
                ভঙ্গ;
            }
            পক্ষ Scheme::মান_Queue_Whole: {
                ++m_n_Scheme_মান_Queues_Whole;
                prepare_Scheme_মান_Queue</* Partial: */ অসত্য>(vscp, vscpInfo);
                ভঙ্গ;
            }
            পক্ষ Scheme::মান_Queue_Partial: {
                ++m_n_Scheme_মান_Queues_Partial;
                prepare_Scheme_মান_Queue</* Partial: */ সত্য>(vscp, vscpInfo);
                ভঙ্গ;
            }
            }
        }
        // Convert all NBAs
        প্রতি (ধ্রুব NBA& nba : m_nbas) {
            AstAssignDly* ধ্রুব nbap = nba.nodep;
            AstVarScope* ধ্রুব vscp = nba.vscp;
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            // Run 'convert' step
            বিকল্প (vscpInfo.m_scheme) {
            পক্ষ Scheme::Undecided: {  // LCOV_EXCL_START
                UASSERT_OBJ(অসত্য, vscp, "Unreachable");
                ভঙ্গ;
            }  // LCOV_EXCL_STOP
            পক্ষ Scheme::UnsupportedCompoundArrayInLoop: {
                // TODO: make this an E_UNSUPPORTED...
                nbap->v3warn(BLKLOOPINIT, "Unsupported: Non-blocking assignment to array with "
                                          "compound element type inside loop");
                ভঙ্গ;
            }
            পক্ষ Scheme::ShadowVar: {
                convertSchemeShadowVar(nbap, vscp, vscpInfo);
                ভঙ্গ;
            }
            পক্ষ Scheme::ShadowVarMasked: {
                convertSchemeShadowVarMasked(nbap, vscp, vscpInfo);
                ভঙ্গ;
            }
            পক্ষ Scheme::FlagShared: {
                convertSchemeFlagShared(nbap, vscp, vscpInfo);
                ভঙ্গ;
            }
            পক্ষ Scheme::FlagUnique: {
                convertSchemeFlagUnique(nbap, vscp, vscpInfo);
                ভঙ্গ;
            }
            পক্ষ Scheme::মান_Queue_Whole: {
                convert_Scheme_মান_Queue(nbap, vscp, vscpInfo, /* partial: */ অসত্য);
                ভঙ্গ;
            }
            পক্ষ Scheme::মান_Queue_Partial:
                convert_Scheme_মান_Queue(nbap, vscp, vscpInfo, /* partial: */ সত্য);
                ভঙ্গ;
            }
        }
    }
    শূন্য visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstActive* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ignoreBlkAndNBlk);
        VL_RESTORER(m_inNonCombLogic);
        m_activep = nodep;
        ধ্রুব Ast_Sen_বৃক্ষ* ধ্রুব senTreep = nodep->sentreep();
        m_ignoreBlkAndNBlk = senTreep->hasStatic() || senTreep->hasInitial();
        m_inNonCombLogic = senTreep->hasClocked();
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override {
        VL_RESTORER(m_needsInitialTrigger);
        VL_RESTORER_CLEAR(m_nbaEventSenTreeps);
        ধ্রুব size_t firstNBAAddedIndex = m_nbas.size();
        {
            VL_RESTORER(m_inSuspendableOrFork);
            VL_RESTORER(m_procp);
            VL_RESTORER(m_ignoreBlkAndNBlk);
            VL_RESTORER(m_inNonCombLogic);
            // When we are dealing with initial block we need to
            // treat it as suspendable when we meet a NBA
            m_inSuspendableOrFork = nodep->isSuspendable() || VN_IS(nodep, Initial);
            m_procp = nodep;
            যদি (nodep->isSuspendable()) {
                m_ignoreBlkAndNBlk = অসত্য;
                m_inNonCombLogic = সত্য;
            }
            iterateChildren(nodep);
        }
        auto containsClocled = [](ধ্রুব AstSenItem* itemp) {
            যাবত্ (itemp) {
                যদি (itemp->edgeType().clockedStmt()) প্রত্যাগম সত্য;
                itemp = VN_AS(itemp->nextp(), SenItem);
            }
            প্রত্যাগম অসত্য;
        };
        ধ্রুব তার্কিক addInitialTrigger = m_needsInitialTrigger
                                       && !(isProcedureWithSentreep<AstAlways>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysObserved>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysReactive>(nodep))
                                       && !containsClocled(m_activep->sentreep()->sensesp());
        যদি (m_timingDomains.empty() && !addInitialTrigger) প্রত্যাগম;

        // There were some timing domains involved in the process. Add all of them as sensitivities
        // of all NBA targets in this process. Note this is a bit of a sledgehammer, we should only
        // need those that directly precede the NBA in control flow, but that is hard to compute,
        // so we will hammer away.

        // First gather all senItems
        AstSenItem* senItemp = nullptr;
        যদি (addInitialTrigger) {
            senItemp = নূতন AstSenItem{nodep->fileline(), AstSenItem::InitialNBA{}};
            ++m_nInitialNBA;
        }

        প্রতি (ধ্রুব Ast_Sen_বৃক্ষ* ধ্রুব domainp : m_timingDomains) {
            যদি (domainp->sensesp())
                senItemp = Ast_গ্রন্থি::addNext(senItemp, domainp->sensesp()->clone_বৃক্ষ(সত্য));
        }
        m_timingDomains.clear();
        // Add them to all nba targets we gathered in this process
        প্রতি (size_t i = firstNBAAddedIndex; i < m_nbas.size(); ++i) {
            m_vscpInfo(m_nbas[i].vscp).addSensitivity(senItemp);
        }
        প্রতি (Ast_Sen_বৃক্ষ* ধ্রুব senTreep : m_nbaEventSenTreeps)
            senTreep->addSensesp(senItemp->clone_বৃক্ষ(সত্য));
        // Done with these
        VL_DO_DANGLING(senItemp->delete_বৃক্ষ(), senItemp);
    }
    শূন্য visit(AstFork* nodep) override {
        VL_RESTORER(m_inSuspendableOrFork);
        m_inSuspendableOrFork = সত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(AstCAwait* nodep) override {
        যদি (nodep->sentreep()) m_timingDomains.ঢোকাও(nodep->sentreep());
        iterateChildren(nodep);
    }
    শূন্য visit(AstFireEvent* nodep) override {
        UASSERT_OBJ(v3Global.hasEvents(), nodep, "Inconsistent");
        File_সারি* ধ্রুব flp = nodep->fileline();

        Ast_গ্রন্থি_রাশিমালা* ধ্রুব eventp = nodep->operandp()->unlinkFrBack();

        // Enqueue for clearing 'triggered' state on next eval
        AstCStmt* ধ্রুব cstmtp = নূতন AstCStmt{flp};
        cstmtp->add("vlSymsp->fireEvent(");
        cstmtp->add(eventp);
        cstmtp->add(");");

        Ast_গ্রন্থি* newp = cstmtp;
        ধ্রুব AstVarRef* ধ্রুব vrefp = VN_CAST(eventp, VarRef);
        যদি (nodep->isDelayed() && (m_cfuncp || !vrefp)) {
            // V3Timing converts these with --timing
            nodep->v3warn(E_NOTIMING, "Nonblocking event trigger "
                                          << (m_cfuncp ? "in a non-inlined function/task"
                                                       : "of a class or interface member or "
                                                         "array element")
                                          << " requires --timing");
        } অন্যথা যদি (nodep->isDelayed()) {
            ধ্রুব std::string newvarname = "_" + vrefp->varp()->shortName();
            AstVarScope* ধ্রুব dlyvscp
                = m_dlyTmps.make(flp, vrefp->varScopep()->scopep(), 1, newvarname);

            ধ্রুব auto dlyRef = [=](VAccess access) {  //
                প্রত্যাগম নূতন AstVarRef{flp, dlyvscp, access};
            };

            AstAlwaysPost* ধ্রুব postp = নূতন AstAlwaysPost{flp};
            AstIf* ধ্রুব ifp = নূতন AstIf{flp, dlyRef(VAccess::READ)};
            postp->addStmtsp(ifp);

            UASSERT_OBJ(m_activep, nodep, "No active to handle FireEvent");
            Ast_Sen_বৃক্ষ* senTreep = m_activep->sentreep();
            AstActive* activep = nullptr;
            যদি (m_inSuspendableOrFork && (senTreep->hasInitial() || senTreep->hasClocked())) {
                // Suspendable code can trigger whenever it resumes, so fire when its NBAs are
                // committed (see visit(AstNodeProcedure*)), clearing the flag like
                // Scheme::FlagUnique
                senTreep = নূতন Ast_Sen_বৃক্ষ{
                    flp, senTreep->hasClocked() ? senTreep->sensesp()->clone_বৃক্ষ(সত্য) : nullptr};
                m_nbaEventSenTreeps.ঠেলো_back(senTreep);
                m_needsInitialTrigger |= m_timingDomains.empty();
                dlyvscp->varp()->setIgnorePostWrite();
                ifp->addThensp(নূতন AstAssign{flp, dlyRef(VAccess::WRITE),
                                             নূতন AstConst{flp, AstConst::BitFalse{}}});
                activep = নূতন AstActive{flp, "nba-event", senTreep};
                activep->sen_বৃক্ষ_Storep(senTreep);
            } অন্যথা {
                activep = নূতন AstActive{flp, "nba-event", senTreep};
                AstAlwaysPre* ধ্রুব prep = নূতন AstAlwaysPre{flp};
                prep->addStmtsp(নূতন AstAssign{flp, dlyRef(VAccess::WRITE),
                                              নূতন AstConst{flp, AstConst::BitFalse{}}});
                activep->addStmtsp(prep);
            }
            ifp->addThensp(newp);
            m_activep->addNextHere(activep);
            activep->addStmtsp(postp);

            newp = নূতন AstAssign{flp, dlyRef(VAccess::WRITE),
                                 নূতন AstConst{flp, AstConst::BitTrue{}}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstAssignDly* nodep) override {
        // Prevent double processing due to AstExprStmt being moved before this node
        যদি (nodep->user1SetOnce()) প্রত্যাগম;

        যদি (m_cfuncp) {
            যদি (!v3Global.rootp()->nbaEventp()) {
                nodep->v3warn(
                    E_NOTIMING,
                    "Delayed assignment in a non-inlined function/task requires --timing");
            }
            প্রত্যাগম;
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
        nodep->lhsp()->foreach([&](Ast_গ্রন্থি* currp) {
            // cppcheck-suppress constVariablePointer
            যদি (Ast_রাশিমালা_Stmt* ধ্রুব exprp = VN_CAST(currp, রাশিমালা_Stmt)) {
                // Move statements before the NBA
                nodep->addHereThisAsNext(exprp->stmtsp()->unlinkFrBackWithNext());
                // Replace with result
                currp->replaceWith(exprp->resultp()->unlinkFrBack());
                // Get rid of the AstExprStmt
                VL_DO_DANGLING2(ঠেলো_Deletep(currp), currp, exprp);
            } অন্যথা যদি (AstVarRef* ধ্রুব refp = VN_CAST(currp, VarRef)) {
                // Ignore reads (e.g.: '_[*here*] <= _')
                যদি (refp->access().isReadOnly()) প্রত্যাগম;
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
        AstVarScope* ধ্রুব vscp = m_currNbaLhsRefp->varScopep();
        // Record it on first encounter
        VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
        যদি (!vscpInfo.m_firstNbaRefp) {
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
        যদি (m_activep->sentreep()->hasClocked()) {
            যদি (vscpInfo.m_fistActivep != m_activep) {
                AstVar* ধ্রুব varp = vscp->varp();
                যদি (!varp->user1SetOnce()
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
        recordWriteRef(m_currNbaLhsRefp, সত্য);

        iterateChildren(nodep);
    }
    শূন্য visit(AstVarRef* nodep) override {
        // Already checked the NBA LHS ref, ignore here
        যদি (nodep == m_currNbaLhsRefp) প্রত্যাগম;
        // Only care about write refs
        যদি (!nodep->access().isWriteOrRW()) প্রত্যাগম;
        // Record write reference
        recordWriteRef(nodep, অসত্য);
    }
    শূন্য visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = সত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_CCall* ধ্রুব nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ধ্রুব auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        যদি (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } অন্যথা {
            প্রতি (AstCFunc* ধ্রুব cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    শূন্য visit(AstCFunc* ধ্রুব nodep) override {
        ধ্রুব auto& মান = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        যদি (মান.m_state != CFuncCache::UNINITIALIZED) প্রত্যাগম;
        VL_RESTORER(m_cfuncp);
        // Its timing domains are those of the processes calling it, see visitCalledCFunc()
        VL_RESTORER_CLEAR(m_timingDomains);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }

    // Pre/Post logic are created here and their content need no further changes, so ignore.
    শূন্য visit(AstAlwaysPre*) override {}
    শূন্য visit(AstAlwaysPost*) override {}

    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
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
        V3Stats::addStat("NBA, variables using ValueQueueWhole scheme", m_n_Scheme_মান_Queues_Whole);
        V3Stats::addStat("NBA, variables using ValueQueuePartial scheme",
                         m_n_Scheme_মান_Queues_Partial);
        V3Stats::addStat("Optimizations, NBA flags shared", m_nSharedSetFlags);
        V3Stats::addStat("Procedures needing initial NBA trigger", m_nInitialNBA);
        V3Stats::addStat("Non-inlined co_awaits with SenTree", m_non_Inlined_CAwaits_With_Sen_বৃক্ষ);
    }
};

//######################################################################
// Delayed class functions

শূন্য V3Delayed::delayedAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DelayedVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("delayed", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
