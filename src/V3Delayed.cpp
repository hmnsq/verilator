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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Delayed.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3ClassGraph.h"
#شامل "V3Const.h"
#شامل "V3LinkLValue.h"
#شامل "V3SharedTmps.h"
#شامل "V3Stats.h"

#شامل <deque>
#شامل <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert AstAssignDlys (NBAs)

رده DelayedVisitor final : عمومی VNVisitor {
    // TYPES

    // The various NBA conversion schemes, including error cases
    شمارش رده Scheme : uint8_t {
        Undecided = 0,
        UnsupportedCompoundArrayInLoop,
        ShadowVar,
        ShadowVarMasked,
        FlagShared,
        FlagUnique,
        مقدار_Queue_Whole,
        مقدار_Queue_Partial
    };

    // All info associated with a variable that is the target of an NBA
    رده VarScopeInfo final {
    عمومی:
        // First write reference encountered to the VarScope as the target on an NBA
        ثابت AstVarRef* m_firstNbaRefp = nullptr;
        // Active block 'm_firstNbaRefp' is under
        ثابت AstActive* m_fistActivep = nullptr;
        منطقی m_whole = نادرست;  // Used on LHS of NBA directly via VarRef
        منطقی m_partial = نادرست;  // Used on LHS of NBA under a Sel
        منطقی m_inLoop = نادرست;  // Used on LHS of NBA in a loop
        منطقی m_inSuspOrFork = نادرست;  // Used on LHS of NBA in suspendable process or fork
        Scheme m_scheme = Scheme::Undecided;  // Conversion scheme to use for this variable

    خصوصی:
        // Combined sensitivities of all NBAs targeting this variable
        Ast_Sen_درخت* m_senTreep = nullptr;

        // Union of stuff needed for the various schemes - use accessors below!
        اجتماع {
            ساختار {  // Stuff needed for Scheme::ShadowVar
                AstVarScope* vscp;  // The shadow variable
            } m_shadowVariableKit;
            ساختار {  // Stuff needed for Scheme::ShadowVarMasked
                AstVarScope* vscp;  // The shadow variable
                AstVarScope* maskp;  // The mask variable
            } m_shadowVarMaskedKit;
            ساختار {  // Stuff needed for Scheme::FlagShared
                AstActive* activep;  // The active block for the Pre/Post logic
                AstAlwaysPost* postp;  // The post block for commiting results
                AstVarScope* commitFlagp;  // The commit flag variable, for reuse
                AstIf* commitIfp;  // The previous if statement for committing, for reuse
            } m_flagSharedKit;
            ساختار {  // Stuff needed for Scheme::FlagUnique
                AstAlwaysPost* postp;  // The post block for commiting results
            } m_flagUniqueKit;
            ساختار {  // Stuff needed for Scheme::ValueQueueWhole/Scheme::ValueQueuePartial
                AstVarScope* vscp;  // The commit queue variable
            } m_مقدار_Queue_Kit;
        } m_kitUnion;

    عمومی:
        VarScopeInfo() = پیشفرض;
        ~VarScopeInfo() {
            // Might not be linked if there was an error
            اگر (!m_senTreep->backp()) VL_DO_DANGLING(m_senTreep->delete_درخت(), m_senTreep);
        }
        // Accessors for the above union fields
        auto& shadowVariableKit() {
            UASSERT(m_scheme == Scheme::ShadowVar, "Inconsistent Scheme");
            بازگشت m_kitUnion.m_shadowVariableKit;
        }
        auto& shadowVarMaskedKit() {
            UASSERT(m_scheme == Scheme::ShadowVarMasked, "Inconsistent Scheme");
            بازگشت m_kitUnion.m_shadowVarMaskedKit;
        }
        auto& flagSharedKit() {
            UASSERT(m_scheme == Scheme::FlagShared, "Inconsistent Scheme");
            بازگشت m_kitUnion.m_flagSharedKit;
        }
        auto& flagUniqueKit() {
            UASSERT(m_scheme == Scheme::FlagUnique, "Inconsistent Scheme");
            بازگشت m_kitUnion.m_flagUniqueKit;
        }
        auto& مقدار_Queue_Kit() {
            UASSERT(m_scheme == Scheme::مقدار_Queue_Partial || m_scheme == Scheme::مقدار_Queue_Whole,
                    "Inconsistent Scheme");
            بازگشت m_kitUnion.m_مقدار_Queue_Kit;
        }

        // Accessor
        Ast_Sen_درخت* senTreep() ثابت { بازگشت m_senTreep; }

        // Add sensitivities
        تهی addSensitivity(AstSenItem* nodep) {
            اگر (!m_senTreep) m_senTreep = جدید Ast_Sen_درخت{nodep->fileline(), nullptr};
            // Add a copy of each term
            m_senTreep->addSensesp(nodep->clone_درخت(درست));
            // Remove duplicates
            V3Const::constifyExpensiveEdit(m_senTreep);
        }
        // cppcheck-suppress constParameterPointer
        تهی addSensitivity(Ast_Sen_درخت* nodep) { addSensitivity(nodep->sensesp()); }
    };

    // Data structure to keep track of all writes to
    ساختار WriteReference final {
        AstVarRef* m_refp = nullptr;  // The reference
        منطقی m_isNBA = نادرست;  // True if an NBA write
        منطقی m_inNonComb = نادرست;  // True if reference is known to be in non-combinational logic
        WriteReference() = پیشفرض;
        WriteReference(AstVarRef* refp, منطقی isNBA, منطقی inNonComb)
            : m_refp{refp}
            , m_isNBA{isNBA}
            , m_inNonComb{inNonComb} {}
    };

    // Data required to lower AstAssignDelay later
    ساختار NBA final {
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
    ثابت VNUser1InUse m_user1InUse;
    ثابت VNUser2InUse m_user2InUse;
    ثابت VNUser3InUse m_user3InUse;

    ساختار CFuncCache final {
        VInsertionSet<Ast_Sen_درخت*> m_timingDomains;  // What shall be added to m_timingDomains
        std::set<AstCFunc*>
            m_includes;  // CFuncs whose CFuncCache shall be included into this - this is used to
                         // break cycles: A->B->A (instead of visiting A while it is still begin
                         // visited B just marks that it includes A)
        شمارش State : uint8_t {
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
    VInsertionSet<Ast_Sen_درخت*> m_timingDomains;  // Timing resume domains
    V3SharedTmps m_dlyTmps{"__Vdly", VVarType::BLOCKTEMP};  // Temporary variables
    // Commit queue data types, by element type and partial flag. Shared by all instances,
    // as m_dlyTmps only shares variables with the same data type.
    std::map<std::pair<ثابت Ast_گره_DType*, منطقی>, AstNBACommitQueueDType*> m_cqDTypeps;

    ثابت std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::vector<ثابت AstCFunc*> m_call_پشته;  // Current callstack of AstCFuncs

    // STATE - for current visit position (use VL_RESTORER)
    AstActive* m_activep = nullptr;  // Current activate
    ثابت AstCFunc* m_cfuncp = nullptr;  // Current public C Function
    Ast_گره_Procedure* m_procp = nullptr;  // Current process
    AstScope* m_scopep = nullptr;  // Current scope
    منطقی m_inLoop = نادرست;  // True in for loops
    منطقی m_inSuspendableOrFork = نادرست;  // True in suspendable processes and forks
    منطقی m_ignoreBlkAndNBlk = نادرست;  // Suppress delayed assignment BLKANDNBLK
    منطقی m_inNonCombLogic = نادرست;  // We are in non-combinational logic
    منطقی m_needsInitialTrigger = نادرست;  // Whether a NodeProcedure needs a initial trigger
    std::vector<Ast_Sen_درخت*> m_nbaEventSenTreeps;  // Sensitivities of '->>' in the process
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
    VDouble0 m_n_Scheme_مقدار_Queues_Whole;  //  Number of variables using Scheme::ValueQueueWhole
    VDouble0 m_n_Scheme_مقدار_Queues_Partial;  //  Number of variables using Scheme::ValueQueuePartial
    VDouble0 m_nSharedSetFlags;  // "Set" flags actually shared by Scheme::FlagShared variables
    VDouble0 m_nInitialNBA;  // Number of procedural blocks with initial NBA
    VDouble0 m_non_Inlined_CAwaits_With_Sen_درخت;  // Count uses of not inlined co_awaits

    // METHODS

    // Return true iff a variable is assigned by both blocking and nonblocking
    // assignments. Issue BLKANDNBLK error if we can't prove the mixed
    // assignments are to independent bits and the blocking assignment can be
    // in combinational logic, which is something we can't safely implement
    // still.
    منطقی checkMixedUsage(ثابت AstVarScope* vscp, منطقی isIntegralOrPacked) {

        ساختار Ref final {
            AstVarRef* m_refp;  // The reference
            منطقی m_inNonComb;  // True if known to be in non-combinational logic
            صحیح m_lsb;  // LSB of accessed range
            صحیح m_msb;  // MSB of accessed range
            Ref(AstVarRef* refp, منطقی inNonComb, صحیح lsb, صحیح msb)
                : m_refp{refp}
                , m_inNonComb{inNonComb}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };

        std::vector<Ref> blkRefs;  // Blocking writes
        std::vector<Ref> nbaRefs;  // Non-blockign writes

        ثابت صحیح width = isIntegralOrPacked ? vscp->width() : 1;

        برای (ثابت auto& writeRef : m_writeRefs(vscp)) {
            صحیح lsb = 0;
            صحیح msb = width - 1;
            اگر (ثابت AstSel* ثابت selp = VN_CAST(writeRef.m_refp->backp(), Sel)) {
                اگر (VN_IS(selp->lsbp(), Const)) {
                    lsb = selp->lsbConst();
                    msb = selp->msbConst();
                }
            }
            اگر (writeRef.m_isNBA) {
                nbaRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            } وگرنه {
                blkRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            }
        }
        // We only run this function on targets of NBAs, so there should be at least one...
        UASSERT_OBJ(!nbaRefs.empty(), vscp, "Did not record NBA write");
        // If no blocking upadte, then we are good
        اگر (blkRefs.empty()) بازگشت نادرست;

        // If the blocking assignment is in non-combinational logic (i.e.:
        // in logic that has an explicit trigger), then we can safely
        // implement it (there is no race between clocked logic and post
        // scheduled logic), so need not error
        blkRefs.erase(std::remove_if(blkRefs.begin(), blkRefs.end(),
                                     [](ثابت Ref& ref) { بازگشت ref.m_inNonComb; }),
                      blkRefs.end());

        // If nothing left, then we need not error
        اگر (blkRefs.empty()) بازگشت درست;

        // If not a packed variable, warn here as we can't prove independence
        اگر (!isIntegralOrPacked) {
            ثابت Ref& blkRef = blkRefs.front();
            ثابت Ref& nbaRef = nbaRefs.front();
            vscp->v3warn(
                BLKANDNBLK,
                "Unsupported: Blocking and non-blocking assignments to same non-packed variable: "
                    << vscp->varp()->prettyNameQ() << '\n'
                    << vscp->warnContextPrimary() << '\n'
                    << blkRef.m_refp->warnOther() << "... Location of blocking assignment\n"
                    << blkRef.m_refp->warnContextSecondary() << '\n'
                    << nbaRef.m_refp->warnOther() << "... Location of nonblocking assignment\n"
                    << nbaRef.m_refp->warnContextSecondary());
            بازگشت درست;
        }

        // We need to error if we can't prove the written bits are independent

        // Sort refs by interval
        ثابت auto lessThanRef = [](ثابت Ref& a, ثابت Ref& b) {
            اگر (a.m_lsb != b.m_lsb) بازگشت a.m_lsb < b.m_lsb;
            بازگشت a.m_msb < b.m_msb;
        };
        std::stable_sort(blkRefs.begin(), blkRefs.end(), lessThanRef);
        std::stable_sort(nbaRefs.begin(), nbaRefs.end(), lessThanRef);
        // Iterate both vectors, checking for overlap
        auto bIt = blkRefs.begin();
        auto nIt = nbaRefs.begin();
        وقتی (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            اگر (lessThanRef(*bIt, *nIt)) {
                اگر (nIt->m_lsb <= bIt->m_msb) بشکن;  // Stop on Overlap
                ++bIt;
            } وگرنه {
                اگر (bIt->m_lsb <= nIt->m_msb) بشکن;  // Stop on Overlap
                ++nIt;
            }
        }

        // If we found an overlapping range that cannot be safely implemented, then wran...
        اگر (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            ثابت Ref& blkRef = *bIt;
            ثابت Ref& nbaRef = *nIt;
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

        بازگشت درست;
    }

    // Choose the NBA scheme used for the given variable.
    Scheme chooseScheme(ثابت AstVarScope* vscp, ثابت VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::Undecided, vscp, "NBA scheme already decided");

        ثابت Ast_گره_DType* ثابت dtypep = vscp->dtypep()->skipRefp();
        // Unpacked arrays
        اگر (ثابت AstUnpackArrayDType* ثابت uaDTypep = VN_CAST(dtypep, UnpackArrayDType)) {
            // If whole array is target of NBA, use ShadowVar, but not in a suspendable process or
            // fork, which resume outside the 'nba' region, so its 'pre' logic would overwrite the
            // value they write in the shadow variable
            اگر (vscpInfo.m_whole) {
                بازگشت vscpInfo.m_inSuspOrFork ? Scheme::FlagUnique : Scheme::ShadowVar;
            }
            // Basic underlying type of elements, if any.
            ثابت AstBasicDType* ثابت basicp = uaDTypep->basicp();
            // If used in a loop, we must have a dynamic commit queue. (Also works in suspendables)
            اگر (vscpInfo.m_inLoop) {
                // Arrays with compound element types are currently not supported in loops
                اگر (!basicp
                    || !(basicp->isIntegralOrPacked()  //
                         || basicp->isDouble()  //
                         || basicp->isString())) {
                    بازگشت Scheme::UnsupportedCompoundArrayInLoop;
                }
                اگر (vscpInfo.m_partial) بازگشت Scheme::مقدار_Queue_Partial;
                بازگشت Scheme::مقدار_Queue_Whole;
            }
            // A suspendable process or fork can resume outside the 'nba' region, so the 'pre'
            // logic of the other schemes would undo its updates. The unique flag scheme has none.
            اگر (vscpInfo.m_inSuspOrFork) بازگشت Scheme::FlagUnique;
            // Otherwise if an array of packed/basic elements, use the shared flag scheme
            اگر (basicp) بازگشت Scheme::FlagShared;
            // Finally fall back on the shadow variable scheme, e.g. for
            // arrays of unpacked structs. This will be slow.
            // TODO: generic LHS scheme as discussed in #5092
            بازگشت Scheme::ShadowVar;
        }

        // Likewise, the 'pre' logic of the shadow variable scheme would undo the updates of a
        // suspendable process or fork, so use the unique flag scheme
        اگر (vscpInfo.m_inSuspOrFork) بازگشت Scheme::FlagUnique;

        ثابت منطقی isIntegralOrPacked = dtypep->isIntegralOrPacked();
        // Check for mixed usage (this also warns if not OK)
        اگر (checkMixedUsage(vscp, isIntegralOrPacked)) {
            // If it's a variable updated by both blocking and non-blocking
            // assignments, use the ShadowVarMasked schem if masked update is
            // possible. This can handle blocking and non-blocking updates to
            // inpdendent parts correctly at run-time, and always works, even
            // in loops or other dynamic context.
            اگر (isIntegralOrPacked) بازگشت Scheme::ShadowVarMasked;
            // If it's inside a loop, use Scheme::ShadowVar, which is safe,
            // but will generate incorrect code if a partial update is used
            اگر (vscpInfo.m_inLoop) بازگشت Scheme::ShadowVar;
            // Otherwise (for not packed variables), use the FlagUnique scheme,
            // which at least handles partial updates correctly, but might break
            // in loops or other dynamic context
            بازگشت Scheme::FlagUnique;
        }

        // Otherwise use the simple shadow variable scheme
        بازگشت Scheme::ShadowVar;
    }

    // Given an expression 'exprp', return a new expression that always evaluates to the
    // value of the given expression at this point in the program. That is:
    // - If given a non-constant expression, create a new temporary AstVarScope under the given
    //   'scopep', with the given 'name', assign the expression to it, and return a read reference
    //   to the new AstVarScope.
    // - If given a constant, just return that constant.
    // New statements are inserted before 'insertp'
    Ast_گره_عبارت* captureVal(AstScope* ثابت scopep, Ast_گره_Stmt* ثابت insertp,
                            Ast_گره_عبارت* ثابت exprp, ثابت std::string& name) {
        UASSERT_OBJ(!exprp->backp(), exprp, "Should have been unlinked");
        File_سطر* ثابت flp = exprp->fileline();
        اگر (VN_IS(exprp, Const)) بازگشت exprp;
        // TODO: there are some const variables that could be simply referenced here
        AstVarScope* ثابت tmpVscp = m_dlyTmps.make(flp, scopep, exprp->dtypep(), name);
        insertp->addHereThisAsNext(
            جدید AstAssign{flp, جدید AstVarRef{flp, tmpVscp, VAccess::WRITE}, exprp});
        بازگشت جدید AstVarRef{flp, tmpVscp, VAccess::READ};
    };

    // Similar to 'captureVal', but captures an LValue expression. That is, the returned
    // expression will reference the same location as the input expression, at this point in the
    // program.
    Ast_گره_عبارت* captureLhs(AstScope* ثابت scopep, Ast_گره_Stmt* ثابت insertp,
                            Ast_گره_عبارت* ثابت lhsp, ثابت std::string& baseName) {
        UASSERT_OBJ(!lhsp->backp(), lhsp, "Should have been unlinked");
        // Running node pointer
        Ast_گره_عبارت* nodep = lhsp;
        // Capture AstSel indices - there should be only one
        اگر (AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            ثابت std::string tmpName{"Lsb" + baseName};
            selp->lsbp(captureVal(scopep, insertp, selp->lsbp()->unlinkFrBack(), tmpName));
            // Continue with target
            nodep = selp->fromp();
        }
        UASSERT_OBJ(!VN_IS(nodep, Sel), lhsp, "Multiple 'AstSel' applied to LHS reference");
        // Capture the indices of selects - might be many, also below struct members. If a handle
        // selects the instance the target is a member of, capture the handle instead, as it is
        // only read, to select the target when the NBA is executed (IEEE 1800-2023 10.4.2).
        size_t nArraySels = 0;
        وقتی (درست) {
            اگر (Ast_گره_Sel* ثابت selp = VN_CAST(nodep, گره_Sel)) {
                ثابت std::string tmpName{"Dim" + std::to_string(nArraySels++) + baseName};
                selp->bitp(captureVal(scopep, insertp, selp->bitp()->unlinkFrBack(), tmpName));
                nodep = selp->fromp();
            } وگرنه اگر (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
                nodep = selp->fromp();
            } وگرنه اگر (AstMemberSel* ثابت selp = VN_CAST(nodep, MemberSel)) {
                Ast_گره_عبارت* ثابت handlep = selp->fromp()->unlinkFrBack();
                V3LinkLValue::linkLValueSet(handlep, VAccess::READ);
                AstVarRef* ثابت refp
                    = VN_AS(captureVal(scopep, insertp, handlep, "Handle" + baseName), VarRef);
                refp->varp()->setIgnorePostWrite();
                // Like the handle it replaces, mark it written, as is the member it selects
                refp->access(selp->access());
                selp->fromp(refp);
                بازگشت lhsp;
            } وگرنه {
                بشکن;
            }
        }
        // What remains must be the variable, or some sort of select, we assume can reuse it
        UASSERT_OBJ(nodep->isPure(), lhsp, "Malformed LHS in NBA");
        // Now have been converted to use the captured values
        بازگشت lhsp;
    }

    تهی add_CFunc_Cached_مقادیر(ثابت AstCFunc* ثابت cfuncp,
                              std::unordered_set<ثابت AstCFunc*>& visited) {
        اگر (!visited.درج(cfuncp).second) بازگشت;
        CFuncCache& مقدار = m_cfuncsCache(cfuncp);
        m_timingDomains.درج(مقدار.m_timingDomains.begin(), مقدار.m_timingDomains.end());
        m_non_Inlined_CAwaits_With_Sen_درخت += مقدار.m_timingDomains.size();
        برای (ثابت AstCFunc* ثابت includedp : مقدار.m_includes) {
            add_CFunc_Cached_مقادیر(includedp, visited);
        }
    }

    // Create a temporary variable in the given 'scopep', with the given 'name', and with 'dtypep'
    // type, with the bits selected by 'sLsbp'/'sWidthp' set to 'valuep', other bits set to zero.
    // Insert new statements before 'insertp'.
    // Returns a read reference to the temporary variable.
    AstVarRef* createWidened(File_سطر* flp, AstScope* scopep, Ast_گره_DType* dtypep,
                             Ast_گره_عبارت* sLsbp, صحیح sWidth, ثابت std::string& name,
                             Ast_گره_عبارت* valuep, Ast_گره* insertp) {
        // Create temporary variable.
        AstVarScope* ثابت tp = m_dlyTmps.make(flp, scopep, dtypep, name);
        // Zero it
        AstConst* ثابت zerop = جدید AstConst{flp, AstConst::DTyped{}, dtypep};
        zerop->رقم().setAllBits0();
        insertp->addHereThisAsNext(
            جدید AstAssign{flp, جدید AstVarRef{flp, tp, VAccess::WRITE}, zerop});
        // Set the selected bits to 'valuep'
        AstSel* ثابت selp = جدید AstSel{flp, جدید AstVarRef{flp, tp, VAccess::WRITE},
                                        sLsbp->clone_درخت_Pure(درست), sWidth};
        insertp->addHereThisAsNext(جدید AstAssign{flp, selp, valuep});
        // This is the expression to get the value of the temporary
        بازگشت جدید AstVarRef{flp, tp, VAccess::READ};
    }

    // Scheme::ShadowVar
    تهی prepareSchemeShadowVar(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Create the shadow variable
        ثابت std::string name = "_" + vscp->varp()->shortName();
        AstVarScope* ثابت shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), name);
        vscpInfo.shadowVariableKit().vscp = shadowVscp;
        // Mark both for V3LifePsot
        vscp->optimizeLifePost(درست);
        shadowVscp->optimizeLifePost(درست);
        // Create the AstActive for the Pre/Post logic
        AstActive* ثابت activep = جدید AstActive{flp, "nba-shadow-variable", vscpInfo.senTreep()};
        activep->sen_درخت_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Pre' scheduled 'shadowVariable = originalVariable' assignment
        AstAlwaysPre* ثابت prep = جدید AstAlwaysPre{flp};
        activep->addStmtsp(prep);
        prep->addStmtsp(جدید AstAssign{flp, جدید AstVarRef{flp, shadowVscp, VAccess::WRITE},
                                      جدید AstVarRef{flp, vscp, VAccess::READ}});
        // Add 'Post' scheduled 'originalVariable = shadowVariable' assignment
        AstAlwaysPost* ثابت postp = جدید AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        postp->addStmtsp(جدید AstAssign{flp, جدید AstVarRef{flp, vscp, VAccess::WRITE},
                                       جدید AstVarRef{flp, shadowVscp, VAccess::READ}});
    }
    تهی convertSchemeShadowVar(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        AstVarScope* ثابت shadowVscp = vscpInfo.shadowVariableKit().vscp;

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ثابت refp) {
            اگر (!refp->access().isWriteOnly()) بازگشت;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
    }

    // Scheme::ShadowVarMasked
    تهی prepareSchemeShadowVarMasked(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Create the shadow variable
        ثابت std::string shadowName = "_" + vscp->varp()->shortName();
        AstVarScope* ثابت shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), shadowName);
        vscpInfo.shadowVarMaskedKit().vscp = shadowVscp;
        // Create the makk variable
        ثابت std::string maskName = "Mask__" + vscp->varp()->shortName();
        AstVarScope* ثابت maskVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), maskName);
        maskVscp->varp()->setIgnorePostWrite();
        vscpInfo.shadowVarMaskedKit().maskp = maskVscp;
        // Create the AstActive for the Post logic
        AstActive* ثابت activep
            = جدید AstActive{flp, "nba-shadow-var-masked", vscpInfo.senTreep()};
        activep->sen_درخت_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit and mask clear
        AstAlwaysPost* ثابت postp = جدید AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit - vscp = (shadowVscp & maskVscp) | (vscp & ~maskVscp);
        postp->addStmtsp(جدید AstAssign{
            flp, جدید AstVarRef{flp, vscp, VAccess::WRITE},
            جدید AstOr{flp,
                      جدید AstAnd{flp, جدید AstVarRef{flp, shadowVscp, VAccess::READ},
                                 جدید AstVarRef{flp, maskVscp, VAccess::READ}},
                      جدید AstAnd{flp, جدید AstVarRef{flp, vscp, VAccess::READ},
                                 جدید AstNot{flp, جدید AstVarRef{flp, maskVscp, VAccess::READ}}}}});
        vscp->varp()->setIgnorePostRead();
        // Clar the mask - maskVscp = '0;
        postp->addStmtsp(
            جدید AstAssign{flp, جدید AstVarRef{flp, maskVscp, VAccess::WRITE},
                          جدید AstConst{flp, AstConst::Widthed_مقدار{}, maskVscp->width(), 0}});
    }
    تهی convertSchemeShadowVarMasked(AstAssignDly* nodep, AstVarScope* vscp,
                                      VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        AstVarScope* ثابت shadowVscp = vscpInfo.shadowVarMaskedKit().vscp;
        AstVarScope* ثابت maskVscp = vscpInfo.shadowVarMaskedKit().maskp;

        Ast_گره_عبارت* lhsClonep = nodep->lhsp()->clone_درخت(نادرست);

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ثابت refp) {
            اگر (!refp->access().isWriteOnly()) بازگشت;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
        // Set the same bits in the mask to 1
        lhsClonep->foreach([&](AstVarRef* ثابت refp) {
            اگر (!refp->access().isWriteOnly()) بازگشت;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(maskVscp);
            refp->varp(maskVscp->varp());
        });
        File_سطر* ثابت flp = nodep->fileline();
        AstConst* ثابت onesp = جدید AstConst{flp, AstConst::DTyped{}, lhsClonep->dtypep()};
        onesp->رقم().setAllBits1();
        nodep->addNextHere(جدید AstAssign{flp, lhsClonep, onesp});
    }

    // Scheme::FlagShared
    تهی prepareSchemeFlagShared(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* ثابت activep = جدید AstActive{flp, "nba-flag-shared", vscpInfo.senTreep()};
        activep->sen_درخت_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        vscpInfo.flagSharedKit().activep = activep;
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ثابت postp = جدید AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagSharedKit().postp = postp;
        // Initialize
        vscpInfo.flagSharedKit().commitFlagp = nullptr;
        vscpInfo.flagSharedKit().commitIfp = nullptr;
    }
    تهی convertSchemeFlagShared(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ثابت std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_گره_عبارت* ثابت capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_گره_عبارت* ثابت capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Is this NBA adjacent after the previously processed NBA?
        ثابت منطقی consecutive = nodep == m_nextDlyp;
        m_nextDlyp = VN_CAST(nodep->nextp(), AssignDly);

        VarScopeInfo* ثابت prevVscpInfop = consecutive ? &m_vscpInfo(m_prevVscp) : nullptr;

        // We can reuse the flag of the previous assignment if:
        ثابت منطقی reuseTheFlag =
            // Consecutive NBAs
            consecutive
            // ... that use the same scheme
            && prevVscpInfop->m_scheme == Scheme::FlagShared
            // ... and are in the same scope as the target variable
            && scopep == vscp->scopep()
            // ... and share the same overall update domain
            && prevVscpInfop->senTreep()->same_درخت(vscpInfo.senTreep());

        اگر (!reuseTheFlag) {
            // Create new flag
            AstVarScope* ثابت flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
            // Set the flag at the original NBA
            nodep->addHereThisAsNext(  //
                جدید AstAssign{flp, جدید AstVarRef{flp, flagVscp, VAccess::WRITE},
                              جدید AstConst{flp, AstConst::BitTrue{}}});
            // Add the 'Pre' scheduled reset for the flag
            AstAlwaysPre* ثابت prep = جدید AstAlwaysPre{flp};
            vscpInfo.flagSharedKit().activep->addStmtsp(prep);
            prep->addStmtsp(جدید AstAssign{flp, جدید AstVarRef{flp, flagVscp, VAccess::WRITE},
                                          جدید AstConst{flp, AstConst::BitFalse{}}});
            // Add the 'Post' scheduled commit
            AstIf* ثابت ifp = جدید AstIf{flp, جدید AstVarRef{flp, flagVscp, VAccess::READ}};
            vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
            vscpInfo.flagSharedKit().commitFlagp = flagVscp;
            vscpInfo.flagSharedKit().commitIfp = ifp;
        } وگرنه {
            اگر (vscp != m_prevVscp) {
                // Different variable, ensure the commit block exists for this variable,
                // can reuse existing one with the same flag, otherwise create a new one.
                AstVarScope* ثابت flagVscp = prevVscpInfop->flagSharedKit().commitFlagp;
                UASSERT_OBJ(flagVscp, nodep, "Commit flag of previous assignment should exist");
                اگر (vscpInfo.flagSharedKit().commitFlagp != flagVscp) {
                    AstIf* ثابت ifp = جدید AstIf{flp, جدید AstVarRef{flp, flagVscp, VAccess::READ}};
                    vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
                    vscpInfo.flagSharedKit().commitFlagp = flagVscp;
                    vscpInfo.flagSharedKit().commitIfp = ifp;
                }
            } وگرنه {
                // Same variable, reuse the commit block of the previous assignment
                vscpInfo.flagSharedKit().commitFlagp = prevVscpInfop->flagSharedKit().commitFlagp;
                vscpInfo.flagSharedKit().commitIfp = prevVscpInfop->flagSharedKit().commitIfp;
            }
            ++m_nSharedSetFlags;
        }
        // Commit the captured value to the captured destination
        vscpInfo.flagSharedKit().commitIfp->addThensp(
            جدید AstAssign{flp, capturedLhsp, capturedRhsp});

        // Remember the variable for the next NBA
        m_prevVscp = vscp;

        // Delete original NBA
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::FlagUnique
    تهی prepareSchemeFlagUnique(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* ثابت activep = جدید AstActive{flp, "nba-flag-unique", vscpInfo.senTreep()};
        activep->sen_درخت_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ثابت postp = جدید AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagUniqueKit().postp = postp;
    }
    تهی convertSchemeFlagUnique(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ثابت std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_گره_عبارت* ثابت capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_گره_عبارت* ثابت capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Create new flag
        AstVarScope* ثابت flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
        flagVscp->varp()->setIgnorePostWrite();
        // Set the flag at the original NBA
        nodep->addHereThisAsNext(  //
            جدید AstAssign{flp, جدید AstVarRef{flp, flagVscp, VAccess::WRITE},
                          جدید AstConst{flp, AstConst::BitTrue{}}});
        // Add the 'Post' scheduled commit
        AstIf* ثابت ifp = جدید AstIf{flp, جدید AstVarRef{flp, flagVscp, VAccess::READ}};
        vscpInfo.flagUniqueKit().postp->addStmtsp(ifp);
        // Immediately clear the flag
        ifp->addThensp(جدید AstAssign{flp, جدید AstVarRef{flp, flagVscp, VAccess::WRITE},
                                     جدید AstConst{flp, AstConst::BitFalse{}}});
        // Commit the value
        ifp->addThensp(جدید AstAssign{flp, capturedLhsp, capturedRhsp});

        // Delete original NBA
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::ValueQueuePartial/Scheme::ValueQueueWhole
    template <منطقی N_Partial>
    تهی prepare_Scheme_مقدار_Queue(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(N_Partial ? vscpInfo.m_scheme == Scheme::مقدار_Queue_Partial
                              : vscpInfo.m_scheme == Scheme::مقدار_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = vscp->scopep();

        // Create the commit queue variable
        Ast_گره_DType* ثابت elemDTypep = vscp->dtypep()->skipRefp();
        AstNBACommitQueueDType*& cqDTypep = m_cqDTypeps[{elemDTypep, N_Partial}];
        اگر (!cqDTypep) {
            cqDTypep = جدید AstNBACommitQueueDType{flp, elemDTypep, N_Partial};
            v3Global.rootp()->typeTablep()->addTypesp(cqDTypep);
        }
        ثابت std::string name = "CommitQueue__" + vscp->varp()->shortName();
        AstVarScope* ثابت queueVscp = m_dlyTmps.make(flp, scopep, cqDTypep, name);
        queueVscp->varp()->noReset(درست);
        queueVscp->varp()->setIgnorePostWrite();
        vscpInfo.مقدار_Queue_Kit().vscp = queueVscp;
        // Create the AstActive for the Post logic
        AstActive* ثابت activep
            = جدید AstActive{flp, "nba-value-queue-whole", vscpInfo.senTreep()};
        activep->sen_درخت_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit
        AstAlwaysPost* ثابت postp = جدید AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit
        AstCMethodHard* ثابت callp = جدید AstCMethodHard{
            flp, جدید AstVarRef{flp, queueVscp, VAccess::READWRITE}, VCMethod::NBA_COMMIT};
        callp->dtypeSetVoid();
        // TODO: this is a partial update, so must be READWRITE, but that breaks scheduling
        callp->addPinsp(جدید AstVarRef{flp, vscp, VAccess::WRITE});
        postp->addStmtsp(callp->makeStmt());
    }

    تهی convert_Scheme_مقدار_Queue(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo,
                                 منطقی partial) {
        UASSERT_OBJ(partial ? vscpInfo.m_scheme == Scheme::مقدار_Queue_Partial
                            : vscpInfo.m_scheme == Scheme::مقدار_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ثابت std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_گره_عبارت* ثابت capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_گره_عبارت* ثابت capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // RHS value (can be widened/masked iff Partial)
        Ast_گره_عبارت* valuep = capturedRhsp;
        // RHS mask (iff Partial)
        Ast_گره_عبارت* maskp = nullptr;

        // Running node for LHS deconstruction
        Ast_گره_عبارت* lhsNodep = capturedLhsp;

        // If partial updates are required, construct the mask and the widened value
        اگر (partial) {
            // Type of array element
            Ast_گره_DType* ثابت eDTypep = [&]() -> Ast_گره_DType* {
                Ast_گره_DType* dtypep = vscp->dtypep()->skipRefp();
                وقتی (AstUnpackArrayDType* ثابت ap = VN_CAST(dtypep, UnpackArrayDType)) {
                    dtypep = ap->subDTypep()->skipRefp();
                }
                بازگشت dtypep;
            }();

            اگر (ثابت AstSel* ثابت lSelp = VN_CAST(lhsNodep, Sel)) {
                // This is a partial assignment.
                // Need to create a mask and widen the value to element size.
                lhsNodep = lSelp->fromp();
                Ast_گره_عبارت* ثابت sLsbp = lSelp->lsbp();
                ثابت صحیح sWidth = lSelp->widthConst();

                // Create mask value
                maskp = [&]() -> Ast_گره_عبارت* {
                    // Constant mask we can compute here
                    اگر (ثابت AstConst* ثابت cLsbp = VN_CAST(sLsbp, Const)) {
                        AstConst* ثابت cp = جدید AstConst{flp, AstConst::DTyped{}, eDTypep};
                        cp->رقم().setMask(sWidth, cLsbp->toSInt());
                        بازگشت cp;
                    }

                    // A non-constant mask we must compute at run-time.
                    AstConst* ثابت onesp = جدید AstConst{flp, AstConst::Widthed_مقدار{}, sWidth, 0};
                    onesp->رقم().setAllBits1();
                    بازگشت createWidened(flp, scopep, eDTypep, sLsbp, sWidth, "Mask" + baseName,
                                         onesp, nodep);
                }();

                // Adjust value to element size
                valuep = [&]() -> Ast_گره_عبارت* {
                    // Constant value with constant select we can compute here
                    اگر (AstConst* ثابت cValuep = VN_CAST(valuep, Const)) {
                        اگر (ثابت AstConst* ثابت cLsbp = VN_CAST(sLsbp, Const)) {
                            AstConst* ثابت cp = جدید AstConst{flp, AstConst::DTyped{}, eDTypep};
                            cp->رقم().setAllBits0();
                            cp->رقم().opSelInto(cValuep->رقم(), cLsbp->toSInt(), sWidth);
                            VL_DO_DANGLING(valuep->delete_درخت(), valuep);
                            بازگشت cp;
                        }
                    }

                    // A non-constant value we must adjust.
                    بازگشت createWidened(flp, scopep, eDTypep, sLsbp, sWidth,  //
                                         "Elem" + baseName, valuep, nodep);
                }();
            } وگرنه {
                // If this assignment is not partial, set mask to ones and we are done
                AstConst* ثابت ones = جدید AstConst{flp, AstConst::DTyped{}, eDTypep};
                ones->رقم().setAllBits1();
                maskp = ones;
            }
        }

        // Extract array indices
        std::vector<Ast_گره_عبارت*> idxps;
        {
            UASSERT_OBJ(VN_IS(lhsNodep, ArraySel), lhsNodep, "Unexpected LHS form");
            وقتی (AstArraySel* ثابت aSelp = VN_CAST(lhsNodep, ArraySel)) {
                idxps.emplace_back(aSelp->bitp()->unlinkFrBack());
                lhsNodep = aSelp->fromp();
            }
            UASSERT_OBJ(VN_IS(lhsNodep, VarRef), lhsNodep, "Unexpected LHS form");
            std::reverse(idxps.begin(), idxps.end());
        }

        // Done with the LHS at this point
        VL_DO_DANGLING(افزودن_Deletep(capturedLhsp), capturedLhsp);

        // Enqueue the update at the site of the original NBA
        AstCMethodHard* ثابت callp = جدید AstCMethodHard{
            flp, جدید AstVarRef{flp, vscpInfo.مقدار_Queue_Kit().vscp, VAccess::READWRITE},
            VCMethod::NBA_ENQUEUE};
        callp->dtypeSetVoid();
        callp->addPinsp(valuep);
        اگر (partial) callp->addPinsp(maskp);
        برای (Ast_گره_عبارت* ثابت indexp : idxps) callp->addPinsp(indexp);
        nodep->addHereThisAsNext(callp->makeStmt());

        // Delete original NBA
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Record where a variable is assigned
    تهی recordWriteRef(AstVarRef* nodep, منطقی nonBlocking) {
        // Ignore references in certain contexts
        اگر (m_ignoreBlkAndNBlk) بازگشت;
        // Ignore if it's an array
        // TODO: we do this because it used to be the previous behaviour.
        //       Is it still required, or should we warn for arrays as well?
        //       Scheduling is no different for them...
        //       Clarification: This is OK for arrays of primitive types, but
        //       arrays that use the ShadowVar scheme don't work...
        اگر (VN_IS(nodep->varScopep()->dtypep()->skipRefp(), UnpackArrayDType)) بازگشت;

        m_writeRefs(nodep->varScopep()).emplace_back(nodep, nonBlocking, m_inNonCombLogic);
    }

    template <typename Procedure_T>
    ایستا منطقی isProcedureWithSentreep(ثابت Ast_گره_Procedure* ثابت nodep) {
        ثابت Procedure_T* ثابت procedurep = Ast_گره::cast<Procedure_T>(nodep);
        بازگشت procedurep && procedurep->sentreep();
    }

    // Visit AstCFunc from a AstNodeCCall - this is made into a separate quasi-visitor because
    // AstCFunc that is not called from the code (e.g.: DPI exports) does not need to be visited
    // this way. Also, not visiting such AstCFuncs allows to avoid caching results for them which
    // this function does - which could lead to excessive memory usage
    تهی visitCalledCFunc(AstCFunc* ثابت nodep) {
        CFuncCache& مقدار = m_cfuncsCache(nodep);
        انتخاب (مقدار.m_state) {
        حالت CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_timingDomains);

            // Visit
            مقدار.m_state = CFuncCache::VISITING;
            m_call_پشته.افزودن_back(nodep);
            {
                VL_RESTORER(m_cfuncp);
                m_cfuncp = nodep;
                iterateChildren(nodep);
            }
            m_call_پشته.برداشت_back();
            مقدار.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_timingDomains, مقدار.m_timingDomains);
        } بشکن;
        حالت CFuncCache::VISITING: {
            برای (size_t i = m_call_پشته.size() - 1; m_call_پشته.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_پشته[i]).m_includes.درج(nodep);
            }
            بازگشت;  // Break recursion
        }
        حالت CFuncCache::INITIALIZED: بشکن;
        }
        std::unordered_set<ثابت AstCFunc*> visited;
        add_CFunc_Cached_مقادیر(nodep, visited);
    }

    // VISITORS
    تهی visit(AstNetlist* nodep) override {
        iterateChildren(nodep);
        // Decide which scheme to use for each variable and do the 'prepare' step
        برای (AstVarScope* ثابت vscp : m_vscps) {
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            vscpInfo.m_scheme = chooseScheme(vscp, vscpInfo);
            // Run 'prepare' step
            انتخاب (vscpInfo.m_scheme) {
            حالت Scheme::Undecided:  // LCOV_EXCL_START
                UASSERT_OBJ(نادرست, vscp, "Failed to choose NBA scheme");
                بشکن;  // LCOV_EXCL_STOP
            حالت Scheme::UnsupportedCompoundArrayInLoop: {
                // Will report error at the site of the NBA
                بشکن;
            }
            حالت Scheme::ShadowVar: {
                ++m_nSchemeShadowVar;
                prepareSchemeShadowVar(vscp, vscpInfo);
                بشکن;
            }
            حالت Scheme::ShadowVarMasked: {
                ++m_nSchemeShadowVarMasked;
                prepareSchemeShadowVarMasked(vscp, vscpInfo);
                بشکن;
            }
            حالت Scheme::FlagShared: {
                ++m_nSchemeFlagShared;
                prepareSchemeFlagShared(vscp, vscpInfo);
                بشکن;
            }
            حالت Scheme::FlagUnique: {
                ++m_nSchemeFlagUnique;
                prepareSchemeFlagUnique(vscp, vscpInfo);
                بشکن;
            }
            حالت Scheme::مقدار_Queue_Whole: {
                ++m_n_Scheme_مقدار_Queues_Whole;
                prepare_Scheme_مقدار_Queue</* Partial: */ نادرست>(vscp, vscpInfo);
                بشکن;
            }
            حالت Scheme::مقدار_Queue_Partial: {
                ++m_n_Scheme_مقدار_Queues_Partial;
                prepare_Scheme_مقدار_Queue</* Partial: */ درست>(vscp, vscpInfo);
                بشکن;
            }
            }
        }
        // Convert all NBAs
        برای (ثابت NBA& nba : m_nbas) {
            AstAssignDly* ثابت nbap = nba.nodep;
            AstVarScope* ثابت vscp = nba.vscp;
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            // Run 'convert' step
            انتخاب (vscpInfo.m_scheme) {
            حالت Scheme::Undecided: {  // LCOV_EXCL_START
                UASSERT_OBJ(نادرست, vscp, "Unreachable");
                بشکن;
            }  // LCOV_EXCL_STOP
            حالت Scheme::UnsupportedCompoundArrayInLoop: {
                // TODO: make this an E_UNSUPPORTED...
                nbap->v3warn(BLKLOOPINIT, "Unsupported: Non-blocking assignment to array with "
                                          "compound element type inside loop");
                بشکن;
            }
            حالت Scheme::ShadowVar: {
                convertSchemeShadowVar(nbap, vscp, vscpInfo);
                بشکن;
            }
            حالت Scheme::ShadowVarMasked: {
                convertSchemeShadowVarMasked(nbap, vscp, vscpInfo);
                بشکن;
            }
            حالت Scheme::FlagShared: {
                convertSchemeFlagShared(nbap, vscp, vscpInfo);
                بشکن;
            }
            حالت Scheme::FlagUnique: {
                convertSchemeFlagUnique(nbap, vscp, vscpInfo);
                بشکن;
            }
            حالت Scheme::مقدار_Queue_Whole: {
                convert_Scheme_مقدار_Queue(nbap, vscp, vscpInfo, /* partial: */ نادرست);
                بشکن;
            }
            حالت Scheme::مقدار_Queue_Partial:
                convert_Scheme_مقدار_Queue(nbap, vscp, vscpInfo, /* partial: */ درست);
                بشکن;
            }
        }
    }
    تهی visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstActive* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ignoreBlkAndNBlk);
        VL_RESTORER(m_inNonCombLogic);
        m_activep = nodep;
        ثابت Ast_Sen_درخت* ثابت senTreep = nodep->sentreep();
        m_ignoreBlkAndNBlk = senTreep->hasStatic() || senTreep->hasInitial();
        m_inNonCombLogic = senTreep->hasClocked();
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Procedure* nodep) override {
        VL_RESTORER(m_needsInitialTrigger);
        VL_RESTORER_CLEAR(m_nbaEventSenTreeps);
        ثابت size_t firstNBAAddedIndex = m_nbas.size();
        {
            VL_RESTORER(m_inSuspendableOrFork);
            VL_RESTORER(m_procp);
            VL_RESTORER(m_ignoreBlkAndNBlk);
            VL_RESTORER(m_inNonCombLogic);
            // When we are dealing with initial block we need to
            // treat it as suspendable when we meet a NBA
            m_inSuspendableOrFork = nodep->isSuspendable() || VN_IS(nodep, Initial);
            m_procp = nodep;
            اگر (nodep->isSuspendable()) {
                m_ignoreBlkAndNBlk = نادرست;
                m_inNonCombLogic = درست;
            }
            iterateChildren(nodep);
        }
        auto containsClocled = [](ثابت AstSenItem* itemp) {
            وقتی (itemp) {
                اگر (itemp->edgeType().clockedStmt()) بازگشت درست;
                itemp = VN_AS(itemp->nextp(), SenItem);
            }
            بازگشت نادرست;
        };
        ثابت منطقی addInitialTrigger = m_needsInitialTrigger
                                       && !(isProcedureWithSentreep<AstAlways>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysObserved>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysReactive>(nodep))
                                       && !containsClocled(m_activep->sentreep()->sensesp());
        اگر (m_timingDomains.empty() && !addInitialTrigger) بازگشت;

        // There were some timing domains involved in the process. Add all of them as sensitivities
        // of all NBA targets in this process. Note this is a bit of a sledgehammer, we should only
        // need those that directly precede the NBA in control flow, but that is hard to compute,
        // so we will hammer away.

        // First gather all senItems
        AstSenItem* senItemp = nullptr;
        اگر (addInitialTrigger) {
            senItemp = جدید AstSenItem{nodep->fileline(), AstSenItem::InitialNBA{}};
            ++m_nInitialNBA;
        }

        برای (ثابت Ast_Sen_درخت* ثابت domainp : m_timingDomains) {
            اگر (domainp->sensesp())
                senItemp = Ast_گره::addNext(senItemp, domainp->sensesp()->clone_درخت(درست));
        }
        m_timingDomains.clear();
        // Add them to all nba targets we gathered in this process
        برای (size_t i = firstNBAAddedIndex; i < m_nbas.size(); ++i) {
            m_vscpInfo(m_nbas[i].vscp).addSensitivity(senItemp);
        }
        برای (Ast_Sen_درخت* ثابت senTreep : m_nbaEventSenTreeps)
            senTreep->addSensesp(senItemp->clone_درخت(درست));
        // Done with these
        VL_DO_DANGLING(senItemp->delete_درخت(), senItemp);
    }
    تهی visit(AstFork* nodep) override {
        VL_RESTORER(m_inSuspendableOrFork);
        m_inSuspendableOrFork = درست;
        iterateChildren(nodep);
    }
    تهی visit(AstCAwait* nodep) override {
        اگر (nodep->sentreep()) m_timingDomains.درج(nodep->sentreep());
        iterateChildren(nodep);
    }
    تهی visit(AstFireEvent* nodep) override {
        UASSERT_OBJ(v3Global.hasEvents(), nodep, "Inconsistent");
        File_سطر* ثابت flp = nodep->fileline();

        Ast_گره_عبارت* ثابت eventp = nodep->operandp()->unlinkFrBack();

        // Enqueue for clearing 'triggered' state on next eval
        AstCStmt* ثابت cstmtp = جدید AstCStmt{flp};
        cstmtp->add("vlSymsp->fireEvent(");
        cstmtp->add(eventp);
        cstmtp->add(");");

        Ast_گره* newp = cstmtp;
        ثابت AstVarRef* ثابت vrefp = VN_CAST(eventp, VarRef);
        اگر (nodep->isDelayed() && (m_cfuncp || !vrefp)) {
            // V3Timing converts these with --timing
            nodep->v3warn(E_NOTIMING, "Nonblocking event trigger "
                                          << (m_cfuncp ? "in a non-inlined function/task"
                                                       : "of a class or interface member or "
                                                         "array element")
                                          << " requires --timing");
        } وگرنه اگر (nodep->isDelayed()) {
            ثابت std::string newvarname = "_" + vrefp->varp()->shortName();
            AstVarScope* ثابت dlyvscp
                = m_dlyTmps.make(flp, vrefp->varScopep()->scopep(), 1, newvarname);

            ثابت auto dlyRef = [=](VAccess access) {  //
                بازگشت جدید AstVarRef{flp, dlyvscp, access};
            };

            AstAlwaysPost* ثابت postp = جدید AstAlwaysPost{flp};
            AstIf* ثابت ifp = جدید AstIf{flp, dlyRef(VAccess::READ)};
            postp->addStmtsp(ifp);

            UASSERT_OBJ(m_activep, nodep, "No active to handle FireEvent");
            Ast_Sen_درخت* senTreep = m_activep->sentreep();
            AstActive* activep = nullptr;
            اگر (m_inSuspendableOrFork && (senTreep->hasInitial() || senTreep->hasClocked())) {
                // Suspendable code can trigger whenever it resumes, so fire when its NBAs are
                // committed (see visit(AstNodeProcedure*)), clearing the flag like
                // Scheme::FlagUnique
                senTreep = جدید Ast_Sen_درخت{
                    flp, senTreep->hasClocked() ? senTreep->sensesp()->clone_درخت(درست) : nullptr};
                m_nbaEventSenTreeps.افزودن_back(senTreep);
                m_needsInitialTrigger |= m_timingDomains.empty();
                dlyvscp->varp()->setIgnorePostWrite();
                ifp->addThensp(جدید AstAssign{flp, dlyRef(VAccess::WRITE),
                                             جدید AstConst{flp, AstConst::BitFalse{}}});
                activep = جدید AstActive{flp, "nba-event", senTreep};
                activep->sen_درخت_Storep(senTreep);
            } وگرنه {
                activep = جدید AstActive{flp, "nba-event", senTreep};
                AstAlwaysPre* ثابت prep = جدید AstAlwaysPre{flp};
                prep->addStmtsp(جدید AstAssign{flp, dlyRef(VAccess::WRITE),
                                              جدید AstConst{flp, AstConst::BitFalse{}}});
                activep->addStmtsp(prep);
            }
            ifp->addThensp(newp);
            m_activep->addNextHere(activep);
            activep->addStmtsp(postp);

            newp = جدید AstAssign{flp, dlyRef(VAccess::WRITE),
                                 جدید AstConst{flp, AstConst::BitTrue{}}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(AstAssignDly* nodep) override {
        // Prevent double processing due to AstExprStmt being moved before this node
        اگر (nodep->user1SetOnce()) بازگشت;

        اگر (m_cfuncp) {
            اگر (!v3Global.rootp()->nbaEventp()) {
                nodep->v3warn(
                    E_NOTIMING,
                    "Delayed assignment in a non-inlined function/task requires --timing");
            }
            بازگشت;
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
        nodep->lhsp()->foreach([&](Ast_گره* currp) {
            // cppcheck-suppress constVariablePointer
            اگر (Ast_عبارت_Stmt* ثابت exprp = VN_CAST(currp, عبارت_Stmt)) {
                // Move statements before the NBA
                nodep->addHereThisAsNext(exprp->stmtsp()->unlinkFrBackWithNext());
                // Replace with result
                currp->replaceWith(exprp->resultp()->unlinkFrBack());
                // Get rid of the AstExprStmt
                VL_DO_DANGLING2(افزودن_Deletep(currp), currp, exprp);
            } وگرنه اگر (AstVarRef* ثابت refp = VN_CAST(currp, VarRef)) {
                // Ignore reads (e.g.: '_[*here*] <= _')
                اگر (refp->access().isReadOnly()) بازگشت;
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
        AstVarScope* ثابت vscp = m_currNbaLhsRefp->varScopep();
        // Record it on first encounter
        VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
        اگر (!vscpInfo.m_firstNbaRefp) {
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
        اگر (m_activep->sentreep()->hasClocked()) {
            اگر (vscpInfo.m_fistActivep != m_activep) {
                AstVar* ثابت varp = vscp->varp();
                اگر (!varp->user1SetOnce()
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
        recordWriteRef(m_currNbaLhsRefp, درست);

        iterateChildren(nodep);
    }
    تهی visit(AstVarRef* nodep) override {
        // Already checked the NBA LHS ref, ignore here
        اگر (nodep == m_currNbaLhsRefp) بازگشت;
        // Only care about write refs
        اگر (!nodep->access().isWriteOrRW()) بازگشت;
        // Record write reference
        recordWriteRef(nodep, نادرست);
    }
    تهی visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = درست;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_CCall* ثابت nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ثابت auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        اگر (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } وگرنه {
            برای (AstCFunc* ثابت cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    تهی visit(AstCFunc* ثابت nodep) override {
        ثابت auto& مقدار = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        اگر (مقدار.m_state != CFuncCache::UNINITIALIZED) بازگشت;
        VL_RESTORER(m_cfuncp);
        // Its timing domains are those of the processes calling it, see visitCalledCFunc()
        VL_RESTORER_CLEAR(m_timingDomains);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }

    // Pre/Post logic are created here and their content need no further changes, so ignore.
    تهی visit(AstAlwaysPre*) override {}
    تهی visit(AstAlwaysPost*) override {}

    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
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
        V3Stats::addStat("NBA, variables using ValueQueueWhole scheme", m_n_Scheme_مقدار_Queues_Whole);
        V3Stats::addStat("NBA, variables using ValueQueuePartial scheme",
                         m_n_Scheme_مقدار_Queues_Partial);
        V3Stats::addStat("Optimizations, NBA flags shared", m_nSharedSetFlags);
        V3Stats::addStat("Procedures needing initial NBA trigger", m_nInitialNBA);
        V3Stats::addStat("Non-inlined co_awaits with SenTree", m_non_Inlined_CAwaits_With_Sen_درخت);
    }
};

//######################################################################
// Delayed class functions

تهی V3Delayed::delayedAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DelayedVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("delayed", 0, dump_درخت_Either_Level() >= 3);
}
