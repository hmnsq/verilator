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
        ध्रुव AstVarRef* m_firstNbaRefp = nullptr;
        // Active block 'm_firstNbaRefp' is under
        ध्रुव AstActive* m_fistActivep = nullptr;
        तार्किक m_whole = असत्य;  // Used on LHS of NBA directly via VarRef
        तार्किक m_partial = असत्य;  // Used on LHS of NBA under a Sel
        तार्किक m_inLoop = असत्य;  // Used on LHS of NBA in a loop
        तार्किक m_inSuspOrFork = असत्य;  // Used on LHS of NBA in suspendable process or fork
        Scheme m_scheme = Scheme::Undecided;  // Conversion scheme to use for this variable

    गुप्त:
        // Combined sensitivities of all NBAs targeting this variable
        Ast_Sen_वृक्ष* m_senTreep = nullptr;

        // Union of stuff needed for the various schemes - use accessors below!
        सङ्घ {
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
        VarScopeInfo() = स्वभाव;
        ~VarScopeInfo() {
            // Might not be linked if there was an error
            यदि (!m_senTreep->backp()) VL_DO_DANGLING(m_senTreep->delete_वृक्ष(), m_senTreep);
        }
        // Accessors for the above union fields
        auto& shadowVariableKit() {
            UASSERT(m_scheme == Scheme::ShadowVar, "Inconsistent Scheme");
            प्रत्यागम m_kitUnion.m_shadowVariableKit;
        }
        auto& shadowVarMaskedKit() {
            UASSERT(m_scheme == Scheme::ShadowVarMasked, "Inconsistent Scheme");
            प्रत्यागम m_kitUnion.m_shadowVarMaskedKit;
        }
        auto& flagSharedKit() {
            UASSERT(m_scheme == Scheme::FlagShared, "Inconsistent Scheme");
            प्रत्यागम m_kitUnion.m_flagSharedKit;
        }
        auto& flagUniqueKit() {
            UASSERT(m_scheme == Scheme::FlagUnique, "Inconsistent Scheme");
            प्रत्यागम m_kitUnion.m_flagUniqueKit;
        }
        auto& मूल्य_Queue_Kit() {
            UASSERT(m_scheme == Scheme::मूल्य_Queue_Partial || m_scheme == Scheme::मूल्य_Queue_Whole,
                    "Inconsistent Scheme");
            प्रत्यागम m_kitUnion.m_मूल्य_Queue_Kit;
        }

        // Accessor
        Ast_Sen_वृक्ष* senTreep() ध्रुव { प्रत्यागम m_senTreep; }

        // Add sensitivities
        शून्य addSensitivity(AstSenItem* nodep) {
            यदि (!m_senTreep) m_senTreep = नूतन Ast_Sen_वृक्ष{nodep->fileline(), nullptr};
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
        WriteReference() = स्वभाव;
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
    ध्रुव VNUser1InUse m_user1InUse;
    ध्रुव VNUser2InUse m_user2InUse;
    ध्रुव VNUser3InUse m_user3InUse;

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
    std::map<std::pair<ध्रुव Ast_गाठ_DType*, तार्किक>, AstNBACommitQueueDType*> m_cqDTypeps;

    ध्रुव std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::vector<ध्रुव AstCFunc*> m_call_चळत;  // Current callstack of AstCFuncs

    // STATE - for current visit position (use VL_RESTORER)
    AstActive* m_activep = nullptr;  // Current activate
    ध्रुव AstCFunc* m_cfuncp = nullptr;  // Current public C Function
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
    तार्किक checkMixedUsage(ध्रुव AstVarScope* vscp, तार्किक isIntegralOrPacked) {

        संरचना Ref final {
            AstVarRef* m_refp;  // The reference
            तार्किक m_inNonComb;  // True if known to be in non-combinational logic
            पूर्णाङ्क m_lsb;  // LSB of accessed range
            पूर्णाङ्क m_msb;  // MSB of accessed range
            Ref(AstVarRef* refp, तार्किक inNonComb, पूर्णाङ्क lsb, पूर्णाङ्क msb)
                : m_refp{refp}
                , m_inNonComb{inNonComb}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };

        std::vector<Ref> blkRefs;  // Blocking writes
        std::vector<Ref> nbaRefs;  // Non-blockign writes

        ध्रुव पूर्णाङ्क width = isIntegralOrPacked ? vscp->width() : 1;

        प्रति (ध्रुव auto& writeRef : m_writeRefs(vscp)) {
            पूर्णाङ्क lsb = 0;
            पूर्णाङ्क msb = width - 1;
            यदि (ध्रुव AstSel* ध्रुव selp = VN_CAST(writeRef.m_refp->backp(), Sel)) {
                यदि (VN_IS(selp->lsbp(), Const)) {
                    lsb = selp->lsbConst();
                    msb = selp->msbConst();
                }
            }
            यदि (writeRef.m_isNBA) {
                nbaRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            } अन्यथा {
                blkRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            }
        }
        // We only run this function on targets of NBAs, so there should be at least one...
        UASSERT_OBJ(!nbaRefs.empty(), vscp, "Did not record NBA write");
        // If no blocking upadte, then we are good
        यदि (blkRefs.empty()) प्रत्यागम असत्य;

        // If the blocking assignment is in non-combinational logic (i.e.:
        // in logic that has an explicit trigger), then we can safely
        // implement it (there is no race between clocked logic and post
        // scheduled logic), so need not error
        blkRefs.erase(std::remove_if(blkRefs.begin(), blkRefs.end(),
                                     [](ध्रुव Ref& ref) { प्रत्यागम ref.m_inNonComb; }),
                      blkRefs.end());

        // If nothing left, then we need not error
        यदि (blkRefs.empty()) प्रत्यागम सत्य;

        // If not a packed variable, warn here as we can't prove independence
        यदि (!isIntegralOrPacked) {
            ध्रुव Ref& blkRef = blkRefs.front();
            ध्रुव Ref& nbaRef = nbaRefs.front();
            vscp->v3warn(
                BLKANDNBLK,
                "Unsupported: Blocking and non-blocking assignments to same non-packed variable: "
                    << vscp->varp()->prettyNameQ() << '\n'
                    << vscp->warnContextPrimary() << '\n'
                    << blkRef.m_refp->warnOther() << "... Location of blocking assignment\n"
                    << blkRef.m_refp->warnContextSecondary() << '\n'
                    << nbaRef.m_refp->warnOther() << "... Location of nonblocking assignment\n"
                    << nbaRef.m_refp->warnContextSecondary());
            प्रत्यागम सत्य;
        }

        // We need to error if we can't prove the written bits are independent

        // Sort refs by interval
        ध्रुव auto lessThanRef = [](ध्रुव Ref& a, ध्रुव Ref& b) {
            यदि (a.m_lsb != b.m_lsb) प्रत्यागम a.m_lsb < b.m_lsb;
            प्रत्यागम a.m_msb < b.m_msb;
        };
        std::stable_sort(blkRefs.begin(), blkRefs.end(), lessThanRef);
        std::stable_sort(nbaRefs.begin(), nbaRefs.end(), lessThanRef);
        // Iterate both vectors, checking for overlap
        auto bIt = blkRefs.begin();
        auto nIt = nbaRefs.begin();
        यावत् (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            यदि (lessThanRef(*bIt, *nIt)) {
                यदि (nIt->m_lsb <= bIt->m_msb) भङ्ग;  // Stop on Overlap
                ++bIt;
            } अन्यथा {
                यदि (bIt->m_lsb <= nIt->m_msb) भङ्ग;  // Stop on Overlap
                ++nIt;
            }
        }

        // If we found an overlapping range that cannot be safely implemented, then wran...
        यदि (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            ध्रुव Ref& blkRef = *bIt;
            ध्रुव Ref& nbaRef = *nIt;
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

        प्रत्यागम सत्य;
    }

    // Choose the NBA scheme used for the given variable.
    Scheme chooseScheme(ध्रुव AstVarScope* vscp, ध्रुव VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::Undecided, vscp, "NBA scheme already decided");

        ध्रुव Ast_गाठ_DType* ध्रुव dtypep = vscp->dtypep()->skipRefp();
        // Unpacked arrays
        यदि (ध्रुव AstUnpackArrayDType* ध्रुव uaDTypep = VN_CAST(dtypep, UnpackArrayDType)) {
            // If whole array is target of NBA, use ShadowVar, but not in a suspendable process or
            // fork, which resume outside the 'nba' region, so its 'pre' logic would overwrite the
            // value they write in the shadow variable
            यदि (vscpInfo.m_whole) {
                प्रत्यागम vscpInfo.m_inSuspOrFork ? Scheme::FlagUnique : Scheme::ShadowVar;
            }
            // Basic underlying type of elements, if any.
            ध्रुव AstBasicDType* ध्रुव basicp = uaDTypep->basicp();
            // If used in a loop, we must have a dynamic commit queue. (Also works in suspendables)
            यदि (vscpInfo.m_inLoop) {
                // Arrays with compound element types are currently not supported in loops
                यदि (!basicp
                    || !(basicp->isIntegralOrPacked()  //
                         || basicp->isDouble()  //
                         || basicp->isString())) {
                    प्रत्यागम Scheme::UnsupportedCompoundArrayInLoop;
                }
                यदि (vscpInfo.m_partial) प्रत्यागम Scheme::मूल्य_Queue_Partial;
                प्रत्यागम Scheme::मूल्य_Queue_Whole;
            }
            // A suspendable process or fork can resume outside the 'nba' region, so the 'pre'
            // logic of the other schemes would undo its updates. The unique flag scheme has none.
            यदि (vscpInfo.m_inSuspOrFork) प्रत्यागम Scheme::FlagUnique;
            // Otherwise if an array of packed/basic elements, use the shared flag scheme
            यदि (basicp) प्रत्यागम Scheme::FlagShared;
            // Finally fall back on the shadow variable scheme, e.g. for
            // arrays of unpacked structs. This will be slow.
            // TODO: generic LHS scheme as discussed in #5092
            प्रत्यागम Scheme::ShadowVar;
        }

        // Likewise, the 'pre' logic of the shadow variable scheme would undo the updates of a
        // suspendable process or fork, so use the unique flag scheme
        यदि (vscpInfo.m_inSuspOrFork) प्रत्यागम Scheme::FlagUnique;

        ध्रुव तार्किक isIntegralOrPacked = dtypep->isIntegralOrPacked();
        // Check for mixed usage (this also warns if not OK)
        यदि (checkMixedUsage(vscp, isIntegralOrPacked)) {
            // If it's a variable updated by both blocking and non-blocking
            // assignments, use the ShadowVarMasked schem if masked update is
            // possible. This can handle blocking and non-blocking updates to
            // inpdendent parts correctly at run-time, and always works, even
            // in loops or other dynamic context.
            यदि (isIntegralOrPacked) प्रत्यागम Scheme::ShadowVarMasked;
            // If it's inside a loop, use Scheme::ShadowVar, which is safe,
            // but will generate incorrect code if a partial update is used
            यदि (vscpInfo.m_inLoop) प्रत्यागम Scheme::ShadowVar;
            // Otherwise (for not packed variables), use the FlagUnique scheme,
            // which at least handles partial updates correctly, but might break
            // in loops or other dynamic context
            प्रत्यागम Scheme::FlagUnique;
        }

        // Otherwise use the simple shadow variable scheme
        प्रत्यागम Scheme::ShadowVar;
    }

    // Given an expression 'exprp', return a new expression that always evaluates to the
    // value of the given expression at this point in the program. That is:
    // - If given a non-constant expression, create a new temporary AstVarScope under the given
    //   'scopep', with the given 'name', assign the expression to it, and return a read reference
    //   to the new AstVarScope.
    // - If given a constant, just return that constant.
    // New statements are inserted before 'insertp'
    Ast_गाठ_अभिव्यक्ती* captureVal(AstScope* ध्रुव scopep, Ast_गाठ_Stmt* ध्रुव insertp,
                            Ast_गाठ_अभिव्यक्ती* ध्रुव exprp, ध्रुव std::string& name) {
        UASSERT_OBJ(!exprp->backp(), exprp, "Should have been unlinked");
        File_ओळ* ध्रुव flp = exprp->fileline();
        यदि (VN_IS(exprp, Const)) प्रत्यागम exprp;
        // TODO: there are some const variables that could be simply referenced here
        AstVarScope* ध्रुव tmpVscp = m_dlyTmps.make(flp, scopep, exprp->dtypep(), name);
        insertp->addHereThisAsNext(
            नूतन AstAssign{flp, नूतन AstVarRef{flp, tmpVscp, VAccess::WRITE}, exprp});
        प्रत्यागम नूतन AstVarRef{flp, tmpVscp, VAccess::READ};
    };

    // Similar to 'captureVal', but captures an LValue expression. That is, the returned
    // expression will reference the same location as the input expression, at this point in the
    // program.
    Ast_गाठ_अभिव्यक्ती* captureLhs(AstScope* ध्रुव scopep, Ast_गाठ_Stmt* ध्रुव insertp,
                            Ast_गाठ_अभिव्यक्ती* ध्रुव lhsp, ध्रुव std::string& baseName) {
        UASSERT_OBJ(!lhsp->backp(), lhsp, "Should have been unlinked");
        // Running node pointer
        Ast_गाठ_अभिव्यक्ती* nodep = lhsp;
        // Capture AstSel indices - there should be only one
        यदि (AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) {
            ध्रुव std::string tmpName{"Lsb" + baseName};
            selp->lsbp(captureVal(scopep, insertp, selp->lsbp()->unlinkFrBack(), tmpName));
            // Continue with target
            nodep = selp->fromp();
        }
        UASSERT_OBJ(!VN_IS(nodep, Sel), lhsp, "Multiple 'AstSel' applied to LHS reference");
        // Capture the indices of selects - might be many, also below struct members. If a handle
        // selects the instance the target is a member of, capture the handle instead, as it is
        // only read, to select the target when the NBA is executed (IEEE 1800-2023 10.4.2).
        size_t nArraySels = 0;
        यावत् (सत्य) {
            यदि (Ast_गाठ_Sel* ध्रुव selp = VN_CAST(nodep, गाठ_Sel)) {
                ध्रुव std::string tmpName{"Dim" + std::to_string(nArraySels++) + baseName};
                selp->bitp(captureVal(scopep, insertp, selp->bitp()->unlinkFrBack(), tmpName));
                nodep = selp->fromp();
            } अन्यथा यदि (ध्रुव AstStructSel* ध्रुव selp = VN_CAST(nodep, StructSel)) {
                nodep = selp->fromp();
            } अन्यथा यदि (AstMemberSel* ध्रुव selp = VN_CAST(nodep, MemberSel)) {
                Ast_गाठ_अभिव्यक्ती* ध्रुव handlep = selp->fromp()->unlinkFrBack();
                V3LinkLValue::linkLValueSet(handlep, VAccess::READ);
                AstVarRef* ध्रुव refp
                    = VN_AS(captureVal(scopep, insertp, handlep, "Handle" + baseName), VarRef);
                refp->varp()->setIgnorePostWrite();
                // Like the handle it replaces, mark it written, as is the member it selects
                refp->access(selp->access());
                selp->fromp(refp);
                प्रत्यागम lhsp;
            } अन्यथा {
                भङ्ग;
            }
        }
        // What remains must be the variable, or some sort of select, we assume can reuse it
        UASSERT_OBJ(nodep->isPure(), lhsp, "Malformed LHS in NBA");
        // Now have been converted to use the captured values
        प्रत्यागम lhsp;
    }

    शून्य add_CFunc_Cached_मूल्ये(ध्रुव AstCFunc* ध्रुव cfuncp,
                              std::unordered_set<ध्रुव AstCFunc*>& visited) {
        यदि (!visited.घाला(cfuncp).second) प्रत्यागम;
        CFuncCache& मूल्य = m_cfuncsCache(cfuncp);
        m_timingDomains.घाला(मूल्य.m_timingDomains.begin(), मूल्य.m_timingDomains.end());
        m_non_Inlined_CAwaits_With_Sen_वृक्ष += मूल्य.m_timingDomains.size();
        प्रति (ध्रुव AstCFunc* ध्रुव includedp : मूल्य.m_includes) {
            add_CFunc_Cached_मूल्ये(includedp, visited);
        }
    }

    // Create a temporary variable in the given 'scopep', with the given 'name', and with 'dtypep'
    // type, with the bits selected by 'sLsbp'/'sWidthp' set to 'valuep', other bits set to zero.
    // Insert new statements before 'insertp'.
    // Returns a read reference to the temporary variable.
    AstVarRef* createWidened(File_ओळ* flp, AstScope* scopep, Ast_गाठ_DType* dtypep,
                             Ast_गाठ_अभिव्यक्ती* sLsbp, पूर्णाङ्क sWidth, ध्रुव std::string& name,
                             Ast_गाठ_अभिव्यक्ती* valuep, Ast_गाठ* insertp) {
        // Create temporary variable.
        AstVarScope* ध्रुव tp = m_dlyTmps.make(flp, scopep, dtypep, name);
        // Zero it
        AstConst* ध्रुव zerop = नूतन AstConst{flp, AstConst::DTyped{}, dtypep};
        zerop->अंक().setAllBits0();
        insertp->addHereThisAsNext(
            नूतन AstAssign{flp, नूतन AstVarRef{flp, tp, VAccess::WRITE}, zerop});
        // Set the selected bits to 'valuep'
        AstSel* ध्रुव selp = नूतन AstSel{flp, नूतन AstVarRef{flp, tp, VAccess::WRITE},
                                        sLsbp->clone_वृक्ष_Pure(सत्य), sWidth};
        insertp->addHereThisAsNext(नूतन AstAssign{flp, selp, valuep});
        // This is the expression to get the value of the temporary
        प्रत्यागम नूतन AstVarRef{flp, tp, VAccess::READ};
    }

    // Scheme::ShadowVar
    शून्य prepareSchemeShadowVar(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        File_ओळ* ध्रुव flp = vscp->fileline();
        AstScope* ध्रुव scopep = vscp->scopep();
        // Create the shadow variable
        ध्रुव std::string name = "_" + vscp->varp()->shortName();
        AstVarScope* ध्रुव shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), name);
        vscpInfo.shadowVariableKit().vscp = shadowVscp;
        // Mark both for V3LifePsot
        vscp->optimizeLifePost(सत्य);
        shadowVscp->optimizeLifePost(सत्य);
        // Create the AstActive for the Pre/Post logic
        AstActive* ध्रुव activep = नूतन AstActive{flp, "nba-shadow-variable", vscpInfo.senTreep()};
        activep->sen_वृक्ष_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Pre' scheduled 'shadowVariable = originalVariable' assignment
        AstAlwaysPre* ध्रुव prep = नूतन AstAlwaysPre{flp};
        activep->addStmtsp(prep);
        prep->addStmtsp(नूतन AstAssign{flp, नूतन AstVarRef{flp, shadowVscp, VAccess::WRITE},
                                      नूतन AstVarRef{flp, vscp, VAccess::READ}});
        // Add 'Post' scheduled 'originalVariable = shadowVariable' assignment
        AstAlwaysPost* ध्रुव postp = नूतन AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        postp->addStmtsp(नूतन AstAssign{flp, नूतन AstVarRef{flp, vscp, VAccess::WRITE},
                                       नूतन AstVarRef{flp, shadowVscp, VAccess::READ}});
    }
    शून्य convertSchemeShadowVar(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        AstVarScope* ध्रुव shadowVscp = vscpInfo.shadowVariableKit().vscp;

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ध्रुव refp) {
            यदि (!refp->access().isWriteOnly()) प्रत्यागम;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
    }

    // Scheme::ShadowVarMasked
    शून्य prepareSchemeShadowVarMasked(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        File_ओळ* ध्रुव flp = vscp->fileline();
        AstScope* ध्रुव scopep = vscp->scopep();
        // Create the shadow variable
        ध्रुव std::string shadowName = "_" + vscp->varp()->shortName();
        AstVarScope* ध्रुव shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), shadowName);
        vscpInfo.shadowVarMaskedKit().vscp = shadowVscp;
        // Create the makk variable
        ध्रुव std::string maskName = "Mask__" + vscp->varp()->shortName();
        AstVarScope* ध्रुव maskVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), maskName);
        maskVscp->varp()->setIgnorePostWrite();
        vscpInfo.shadowVarMaskedKit().maskp = maskVscp;
        // Create the AstActive for the Post logic
        AstActive* ध्रुव activep
            = नूतन AstActive{flp, "nba-shadow-var-masked", vscpInfo.senTreep()};
        activep->sen_वृक्ष_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit and mask clear
        AstAlwaysPost* ध्रुव postp = नूतन AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit - vscp = (shadowVscp & maskVscp) | (vscp & ~maskVscp);
        postp->addStmtsp(नूतन AstAssign{
            flp, नूतन AstVarRef{flp, vscp, VAccess::WRITE},
            नूतन AstOr{flp,
                      नूतन AstAnd{flp, नूतन AstVarRef{flp, shadowVscp, VAccess::READ},
                                 नूतन AstVarRef{flp, maskVscp, VAccess::READ}},
                      नूतन AstAnd{flp, नूतन AstVarRef{flp, vscp, VAccess::READ},
                                 नूतन AstNot{flp, नूतन AstVarRef{flp, maskVscp, VAccess::READ}}}}});
        vscp->varp()->setIgnorePostRead();
        // Clar the mask - maskVscp = '0;
        postp->addStmtsp(
            नूतन AstAssign{flp, नूतन AstVarRef{flp, maskVscp, VAccess::WRITE},
                          नूतन AstConst{flp, AstConst::Widthed_मूल्य{}, maskVscp->width(), 0}});
    }
    शून्य convertSchemeShadowVarMasked(AstAssignDly* nodep, AstVarScope* vscp,
                                      VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        AstVarScope* ध्रुव shadowVscp = vscpInfo.shadowVarMaskedKit().vscp;
        AstVarScope* ध्रुव maskVscp = vscpInfo.shadowVarMaskedKit().maskp;

        Ast_गाठ_अभिव्यक्ती* lhsClonep = nodep->lhsp()->clone_वृक्ष(असत्य);

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ध्रुव refp) {
            यदि (!refp->access().isWriteOnly()) प्रत्यागम;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
        // Set the same bits in the mask to 1
        lhsClonep->foreach([&](AstVarRef* ध्रुव refp) {
            यदि (!refp->access().isWriteOnly()) प्रत्यागम;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(maskVscp);
            refp->varp(maskVscp->varp());
        });
        File_ओळ* ध्रुव flp = nodep->fileline();
        AstConst* ध्रुव onesp = नूतन AstConst{flp, AstConst::DTyped{}, lhsClonep->dtypep()};
        onesp->अंक().setAllBits1();
        nodep->addNextHere(नूतन AstAssign{flp, lhsClonep, onesp});
    }

    // Scheme::FlagShared
    शून्य prepareSchemeFlagShared(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_ओळ* ध्रुव flp = vscp->fileline();
        AstScope* ध्रुव scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* ध्रुव activep = नूतन AstActive{flp, "nba-flag-shared", vscpInfo.senTreep()};
        activep->sen_वृक्ष_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        vscpInfo.flagSharedKit().activep = activep;
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ध्रुव postp = नूतन AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagSharedKit().postp = postp;
        // Initialize
        vscpInfo.flagSharedKit().commitFlagp = nullptr;
        vscpInfo.flagSharedKit().commitIfp = nullptr;
    }
    शून्य convertSchemeFlagShared(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_ओळ* ध्रुव flp = vscp->fileline();
        AstScope* ध्रुव scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ध्रुव std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_गाठ_अभिव्यक्ती* ध्रुव capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_गाठ_अभिव्यक्ती* ध्रुव capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Is this NBA adjacent after the previously processed NBA?
        ध्रुव तार्किक consecutive = nodep == m_nextDlyp;
        m_nextDlyp = VN_CAST(nodep->nextp(), AssignDly);

        VarScopeInfo* ध्रुव prevVscpInfop = consecutive ? &m_vscpInfo(m_prevVscp) : nullptr;

        // We can reuse the flag of the previous assignment if:
        ध्रुव तार्किक reuseTheFlag =
            // Consecutive NBAs
            consecutive
            // ... that use the same scheme
            && prevVscpInfop->m_scheme == Scheme::FlagShared
            // ... and are in the same scope as the target variable
            && scopep == vscp->scopep()
            // ... and share the same overall update domain
            && prevVscpInfop->senTreep()->same_वृक्ष(vscpInfo.senTreep());

        यदि (!reuseTheFlag) {
            // Create new flag
            AstVarScope* ध्रुव flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
            // Set the flag at the original NBA
            nodep->addHereThisAsNext(  //
                नूतन AstAssign{flp, नूतन AstVarRef{flp, flagVscp, VAccess::WRITE},
                              नूतन AstConst{flp, AstConst::BitTrue{}}});
            // Add the 'Pre' scheduled reset for the flag
            AstAlwaysPre* ध्रुव prep = नूतन AstAlwaysPre{flp};
            vscpInfo.flagSharedKit().activep->addStmtsp(prep);
            prep->addStmtsp(नूतन AstAssign{flp, नूतन AstVarRef{flp, flagVscp, VAccess::WRITE},
                                          नूतन AstConst{flp, AstConst::BitFalse{}}});
            // Add the 'Post' scheduled commit
            AstIf* ध्रुव ifp = नूतन AstIf{flp, नूतन AstVarRef{flp, flagVscp, VAccess::READ}};
            vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
            vscpInfo.flagSharedKit().commitFlagp = flagVscp;
            vscpInfo.flagSharedKit().commitIfp = ifp;
        } अन्यथा {
            यदि (vscp != m_prevVscp) {
                // Different variable, ensure the commit block exists for this variable,
                // can reuse existing one with the same flag, otherwise create a new one.
                AstVarScope* ध्रुव flagVscp = prevVscpInfop->flagSharedKit().commitFlagp;
                UASSERT_OBJ(flagVscp, nodep, "Commit flag of previous assignment should exist");
                यदि (vscpInfo.flagSharedKit().commitFlagp != flagVscp) {
                    AstIf* ध्रुव ifp = नूतन AstIf{flp, नूतन AstVarRef{flp, flagVscp, VAccess::READ}};
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
            नूतन AstAssign{flp, capturedLhsp, capturedRhsp});

        // Remember the variable for the next NBA
        m_prevVscp = vscp;

        // Delete original NBA
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::FlagUnique
    शून्य prepareSchemeFlagUnique(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_ओळ* ध्रुव flp = vscp->fileline();
        AstScope* ध्रुव scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* ध्रुव activep = नूतन AstActive{flp, "nba-flag-unique", vscpInfo.senTreep()};
        activep->sen_वृक्ष_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ध्रुव postp = नूतन AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagUniqueKit().postp = postp;
    }
    शून्य convertSchemeFlagUnique(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_ओळ* ध्रुव flp = vscp->fileline();
        AstScope* ध्रुव scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ध्रुव std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_गाठ_अभिव्यक्ती* ध्रुव capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_गाठ_अभिव्यक्ती* ध्रुव capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Create new flag
        AstVarScope* ध्रुव flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
        flagVscp->varp()->setIgnorePostWrite();
        // Set the flag at the original NBA
        nodep->addHereThisAsNext(  //
            नूतन AstAssign{flp, नूतन AstVarRef{flp, flagVscp, VAccess::WRITE},
                          नूतन AstConst{flp, AstConst::BitTrue{}}});
        // Add the 'Post' scheduled commit
        AstIf* ध्रुव ifp = नूतन AstIf{flp, नूतन AstVarRef{flp, flagVscp, VAccess::READ}};
        vscpInfo.flagUniqueKit().postp->addStmtsp(ifp);
        // Immediately clear the flag
        ifp->addThensp(नूतन AstAssign{flp, नूतन AstVarRef{flp, flagVscp, VAccess::WRITE},
                                     नूतन AstConst{flp, AstConst::BitFalse{}}});
        // Commit the value
        ifp->addThensp(नूतन AstAssign{flp, capturedLhsp, capturedRhsp});

        // Delete original NBA
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::ValueQueuePartial/Scheme::ValueQueueWhole
    template <तार्किक N_Partial>
    शून्य prepare_Scheme_मूल्य_Queue(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(N_Partial ? vscpInfo.m_scheme == Scheme::मूल्य_Queue_Partial
                              : vscpInfo.m_scheme == Scheme::मूल्य_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_ओळ* ध्रुव flp = vscp->fileline();
        AstScope* ध्रुव scopep = vscp->scopep();

        // Create the commit queue variable
        Ast_गाठ_DType* ध्रुव elemDTypep = vscp->dtypep()->skipRefp();
        AstNBACommitQueueDType*& cqDTypep = m_cqDTypeps[{elemDTypep, N_Partial}];
        यदि (!cqDTypep) {
            cqDTypep = नूतन AstNBACommitQueueDType{flp, elemDTypep, N_Partial};
            v3Global.rootp()->typeTablep()->addTypesp(cqDTypep);
        }
        ध्रुव std::string name = "CommitQueue__" + vscp->varp()->shortName();
        AstVarScope* ध्रुव queueVscp = m_dlyTmps.make(flp, scopep, cqDTypep, name);
        queueVscp->varp()->noReset(सत्य);
        queueVscp->varp()->setIgnorePostWrite();
        vscpInfo.मूल्य_Queue_Kit().vscp = queueVscp;
        // Create the AstActive for the Post logic
        AstActive* ध्रुव activep
            = नूतन AstActive{flp, "nba-value-queue-whole", vscpInfo.senTreep()};
        activep->sen_वृक्ष_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit
        AstAlwaysPost* ध्रुव postp = नूतन AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit
        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, queueVscp, VAccess::READWRITE}, VCMethod::NBA_COMMIT};
        callp->dtypeSetVoid();
        // TODO: this is a partial update, so must be READWRITE, but that breaks scheduling
        callp->addPinsp(नूतन AstVarRef{flp, vscp, VAccess::WRITE});
        postp->addStmtsp(callp->makeStmt());
    }

    शून्य convert_Scheme_मूल्य_Queue(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo,
                                 तार्किक partial) {
        UASSERT_OBJ(partial ? vscpInfo.m_scheme == Scheme::मूल्य_Queue_Partial
                            : vscpInfo.m_scheme == Scheme::मूल्य_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_ओळ* ध्रुव flp = vscp->fileline();
        AstScope* ध्रुव scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ध्रुव std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_गाठ_अभिव्यक्ती* ध्रुव capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_गाठ_अभिव्यक्ती* ध्रुव capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // RHS value (can be widened/masked iff Partial)
        Ast_गाठ_अभिव्यक्ती* valuep = capturedRhsp;
        // RHS mask (iff Partial)
        Ast_गाठ_अभिव्यक्ती* maskp = nullptr;

        // Running node for LHS deconstruction
        Ast_गाठ_अभिव्यक्ती* lhsNodep = capturedLhsp;

        // If partial updates are required, construct the mask and the widened value
        यदि (partial) {
            // Type of array element
            Ast_गाठ_DType* ध्रुव eDTypep = [&]() -> Ast_गाठ_DType* {
                Ast_गाठ_DType* dtypep = vscp->dtypep()->skipRefp();
                यावत् (AstUnpackArrayDType* ध्रुव ap = VN_CAST(dtypep, UnpackArrayDType)) {
                    dtypep = ap->subDTypep()->skipRefp();
                }
                प्रत्यागम dtypep;
            }();

            यदि (ध्रुव AstSel* ध्रुव lSelp = VN_CAST(lhsNodep, Sel)) {
                // This is a partial assignment.
                // Need to create a mask and widen the value to element size.
                lhsNodep = lSelp->fromp();
                Ast_गाठ_अभिव्यक्ती* ध्रुव sLsbp = lSelp->lsbp();
                ध्रुव पूर्णाङ्क sWidth = lSelp->widthConst();

                // Create mask value
                maskp = [&]() -> Ast_गाठ_अभिव्यक्ती* {
                    // Constant mask we can compute here
                    यदि (ध्रुव AstConst* ध्रुव cLsbp = VN_CAST(sLsbp, Const)) {
                        AstConst* ध्रुव cp = नूतन AstConst{flp, AstConst::DTyped{}, eDTypep};
                        cp->अंक().setMask(sWidth, cLsbp->toSInt());
                        प्रत्यागम cp;
                    }

                    // A non-constant mask we must compute at run-time.
                    AstConst* ध्रुव onesp = नूतन AstConst{flp, AstConst::Widthed_मूल्य{}, sWidth, 0};
                    onesp->अंक().setAllBits1();
                    प्रत्यागम createWidened(flp, scopep, eDTypep, sLsbp, sWidth, "Mask" + baseName,
                                         onesp, nodep);
                }();

                // Adjust value to element size
                valuep = [&]() -> Ast_गाठ_अभिव्यक्ती* {
                    // Constant value with constant select we can compute here
                    यदि (AstConst* ध्रुव cValuep = VN_CAST(valuep, Const)) {
                        यदि (ध्रुव AstConst* ध्रुव cLsbp = VN_CAST(sLsbp, Const)) {
                            AstConst* ध्रुव cp = नूतन AstConst{flp, AstConst::DTyped{}, eDTypep};
                            cp->अंक().setAllBits0();
                            cp->अंक().opSelInto(cValuep->अंक(), cLsbp->toSInt(), sWidth);
                            VL_DO_DANGLING(valuep->delete_वृक्ष(), valuep);
                            प्रत्यागम cp;
                        }
                    }

                    // A non-constant value we must adjust.
                    प्रत्यागम createWidened(flp, scopep, eDTypep, sLsbp, sWidth,  //
                                         "Elem" + baseName, valuep, nodep);
                }();
            } अन्यथा {
                // If this assignment is not partial, set mask to ones and we are done
                AstConst* ध्रुव ones = नूतन AstConst{flp, AstConst::DTyped{}, eDTypep};
                ones->अंक().setAllBits1();
                maskp = ones;
            }
        }

        // Extract array indices
        std::vector<Ast_गाठ_अभिव्यक्ती*> idxps;
        {
            UASSERT_OBJ(VN_IS(lhsNodep, ArraySel), lhsNodep, "Unexpected LHS form");
            यावत् (AstArraySel* ध्रुव aSelp = VN_CAST(lhsNodep, ArraySel)) {
                idxps.emplace_back(aSelp->bitp()->unlinkFrBack());
                lhsNodep = aSelp->fromp();
            }
            UASSERT_OBJ(VN_IS(lhsNodep, VarRef), lhsNodep, "Unexpected LHS form");
            std::reverse(idxps.begin(), idxps.end());
        }

        // Done with the LHS at this point
        VL_DO_DANGLING(ढकला_Deletep(capturedLhsp), capturedLhsp);

        // Enqueue the update at the site of the original NBA
        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, vscpInfo.मूल्य_Queue_Kit().vscp, VAccess::READWRITE},
            VCMethod::NBA_ENQUEUE};
        callp->dtypeSetVoid();
        callp->addPinsp(valuep);
        यदि (partial) callp->addPinsp(maskp);
        प्रति (Ast_गाठ_अभिव्यक्ती* ध्रुव indexp : idxps) callp->addPinsp(indexp);
        nodep->addHereThisAsNext(callp->makeStmt());

        // Delete original NBA
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Record where a variable is assigned
    शून्य recordWriteRef(AstVarRef* nodep, तार्किक nonBlocking) {
        // Ignore references in certain contexts
        यदि (m_ignoreBlkAndNBlk) प्रत्यागम;
        // Ignore if it's an array
        // TODO: we do this because it used to be the previous behaviour.
        //       Is it still required, or should we warn for arrays as well?
        //       Scheduling is no different for them...
        //       Clarification: This is OK for arrays of primitive types, but
        //       arrays that use the ShadowVar scheme don't work...
        यदि (VN_IS(nodep->varScopep()->dtypep()->skipRefp(), UnpackArrayDType)) प्रत्यागम;

        m_writeRefs(nodep->varScopep()).emplace_back(nodep, nonBlocking, m_inNonCombLogic);
    }

    template <typename Procedure_T>
    स्थिर तार्किक isProcedureWithSentreep(ध्रुव Ast_गाठ_Procedure* ध्रुव nodep) {
        ध्रुव Procedure_T* ध्रुव procedurep = Ast_गाठ::cast<Procedure_T>(nodep);
        प्रत्यागम procedurep && procedurep->sentreep();
    }

    // Visit AstCFunc from a AstNodeCCall - this is made into a separate quasi-visitor because
    // AstCFunc that is not called from the code (e.g.: DPI exports) does not need to be visited
    // this way. Also, not visiting such AstCFuncs allows to avoid caching results for them which
    // this function does - which could lead to excessive memory usage
    शून्य visitCalledCFunc(AstCFunc* ध्रुव nodep) {
        CFuncCache& मूल्य = m_cfuncsCache(nodep);
        विकल्प (मूल्य.m_state) {
        पक्ष CFuncCache::UNINITIALIZED: {
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
        } भङ्ग;
        पक्ष CFuncCache::VISITING: {
            प्रति (size_t i = m_call_चळत.size() - 1; m_call_चळत.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_चळत[i]).m_includes.घाला(nodep);
            }
            प्रत्यागम;  // Break recursion
        }
        पक्ष CFuncCache::INITIALIZED: भङ्ग;
        }
        std::unordered_set<ध्रुव AstCFunc*> visited;
        add_CFunc_Cached_मूल्ये(nodep, visited);
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {
        iterateChildren(nodep);
        // Decide which scheme to use for each variable and do the 'prepare' step
        प्रति (AstVarScope* ध्रुव vscp : m_vscps) {
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            vscpInfo.m_scheme = chooseScheme(vscp, vscpInfo);
            // Run 'prepare' step
            विकल्प (vscpInfo.m_scheme) {
            पक्ष Scheme::Undecided:  // LCOV_EXCL_START
                UASSERT_OBJ(असत्य, vscp, "Failed to choose NBA scheme");
                भङ्ग;  // LCOV_EXCL_STOP
            पक्ष Scheme::UnsupportedCompoundArrayInLoop: {
                // Will report error at the site of the NBA
                भङ्ग;
            }
            पक्ष Scheme::ShadowVar: {
                ++m_nSchemeShadowVar;
                prepareSchemeShadowVar(vscp, vscpInfo);
                भङ्ग;
            }
            पक्ष Scheme::ShadowVarMasked: {
                ++m_nSchemeShadowVarMasked;
                prepareSchemeShadowVarMasked(vscp, vscpInfo);
                भङ्ग;
            }
            पक्ष Scheme::FlagShared: {
                ++m_nSchemeFlagShared;
                prepareSchemeFlagShared(vscp, vscpInfo);
                भङ्ग;
            }
            पक्ष Scheme::FlagUnique: {
                ++m_nSchemeFlagUnique;
                prepareSchemeFlagUnique(vscp, vscpInfo);
                भङ्ग;
            }
            पक्ष Scheme::मूल्य_Queue_Whole: {
                ++m_n_Scheme_मूल्य_Queues_Whole;
                prepare_Scheme_मूल्य_Queue</* Partial: */ असत्य>(vscp, vscpInfo);
                भङ्ग;
            }
            पक्ष Scheme::मूल्य_Queue_Partial: {
                ++m_n_Scheme_मूल्य_Queues_Partial;
                prepare_Scheme_मूल्य_Queue</* Partial: */ सत्य>(vscp, vscpInfo);
                भङ्ग;
            }
            }
        }
        // Convert all NBAs
        प्रति (ध्रुव NBA& nba : m_nbas) {
            AstAssignDly* ध्रुव nbap = nba.nodep;
            AstVarScope* ध्रुव vscp = nba.vscp;
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            // Run 'convert' step
            विकल्प (vscpInfo.m_scheme) {
            पक्ष Scheme::Undecided: {  // LCOV_EXCL_START
                UASSERT_OBJ(असत्य, vscp, "Unreachable");
                भङ्ग;
            }  // LCOV_EXCL_STOP
            पक्ष Scheme::UnsupportedCompoundArrayInLoop: {
                // TODO: make this an E_UNSUPPORTED...
                nbap->v3warn(BLKLOOPINIT, "Unsupported: Non-blocking assignment to array with "
                                          "compound element type inside loop");
                भङ्ग;
            }
            पक्ष Scheme::ShadowVar: {
                convertSchemeShadowVar(nbap, vscp, vscpInfo);
                भङ्ग;
            }
            पक्ष Scheme::ShadowVarMasked: {
                convertSchemeShadowVarMasked(nbap, vscp, vscpInfo);
                भङ्ग;
            }
            पक्ष Scheme::FlagShared: {
                convertSchemeFlagShared(nbap, vscp, vscpInfo);
                भङ्ग;
            }
            पक्ष Scheme::FlagUnique: {
                convertSchemeFlagUnique(nbap, vscp, vscpInfo);
                भङ्ग;
            }
            पक्ष Scheme::मूल्य_Queue_Whole: {
                convert_Scheme_मूल्य_Queue(nbap, vscp, vscpInfo, /* partial: */ असत्य);
                भङ्ग;
            }
            पक्ष Scheme::मूल्य_Queue_Partial:
                convert_Scheme_मूल्य_Queue(nbap, vscp, vscpInfo, /* partial: */ सत्य);
                भङ्ग;
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
        ध्रुव Ast_Sen_वृक्ष* ध्रुव senTreep = nodep->sentreep();
        m_ignoreBlkAndNBlk = senTreep->hasStatic() || senTreep->hasInitial();
        m_inNonCombLogic = senTreep->hasClocked();
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_Procedure* nodep) override {
        VL_RESTORER(m_needsInitialTrigger);
        VL_RESTORER_CLEAR(m_nbaEventSenTreeps);
        ध्रुव size_t firstNBAAddedIndex = m_nbas.size();
        {
            VL_RESTORER(m_inSuspendableOrFork);
            VL_RESTORER(m_procp);
            VL_RESTORER(m_ignoreBlkAndNBlk);
            VL_RESTORER(m_inNonCombLogic);
            // When we are dealing with initial block we need to
            // treat it as suspendable when we meet a NBA
            m_inSuspendableOrFork = nodep->isSuspendable() || VN_IS(nodep, Initial);
            m_procp = nodep;
            यदि (nodep->isSuspendable()) {
                m_ignoreBlkAndNBlk = असत्य;
                m_inNonCombLogic = सत्य;
            }
            iterateChildren(nodep);
        }
        auto containsClocled = [](ध्रुव AstSenItem* itemp) {
            यावत् (itemp) {
                यदि (itemp->edgeType().clockedStmt()) प्रत्यागम सत्य;
                itemp = VN_AS(itemp->nextp(), SenItem);
            }
            प्रत्यागम असत्य;
        };
        ध्रुव तार्किक addInitialTrigger = m_needsInitialTrigger
                                       && !(isProcedureWithSentreep<AstAlways>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysObserved>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysReactive>(nodep))
                                       && !containsClocled(m_activep->sentreep()->sensesp());
        यदि (m_timingDomains.empty() && !addInitialTrigger) प्रत्यागम;

        // There were some timing domains involved in the process. Add all of them as sensitivities
        // of all NBA targets in this process. Note this is a bit of a sledgehammer, we should only
        // need those that directly precede the NBA in control flow, but that is hard to compute,
        // so we will hammer away.

        // First gather all senItems
        AstSenItem* senItemp = nullptr;
        यदि (addInitialTrigger) {
            senItemp = नूतन AstSenItem{nodep->fileline(), AstSenItem::InitialNBA{}};
            ++m_nInitialNBA;
        }

        प्रति (ध्रुव Ast_Sen_वृक्ष* ध्रुव domainp : m_timingDomains) {
            यदि (domainp->sensesp())
                senItemp = Ast_गाठ::addNext(senItemp, domainp->sensesp()->clone_वृक्ष(सत्य));
        }
        m_timingDomains.clear();
        // Add them to all nba targets we gathered in this process
        प्रति (size_t i = firstNBAAddedIndex; i < m_nbas.size(); ++i) {
            m_vscpInfo(m_nbas[i].vscp).addSensitivity(senItemp);
        }
        प्रति (Ast_Sen_वृक्ष* ध्रुव senTreep : m_nbaEventSenTreeps)
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
        यदि (nodep->sentreep()) m_timingDomains.घाला(nodep->sentreep());
        iterateChildren(nodep);
    }
    शून्य visit(AstFireEvent* nodep) override {
        UASSERT_OBJ(v3Global.hasEvents(), nodep, "Inconsistent");
        File_ओळ* ध्रुव flp = nodep->fileline();

        Ast_गाठ_अभिव्यक्ती* ध्रुव eventp = nodep->operandp()->unlinkFrBack();

        // Enqueue for clearing 'triggered' state on next eval
        AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{flp};
        cstmtp->add("vlSymsp->fireEvent(");
        cstmtp->add(eventp);
        cstmtp->add(");");

        Ast_गाठ* newp = cstmtp;
        ध्रुव AstVarRef* ध्रुव vrefp = VN_CAST(eventp, VarRef);
        यदि (nodep->isDelayed() && (m_cfuncp || !vrefp)) {
            // V3Timing converts these with --timing
            nodep->v3warn(E_NOTIMING, "Nonblocking event trigger "
                                          << (m_cfuncp ? "in a non-inlined function/task"
                                                       : "of a class or interface member or "
                                                         "array element")
                                          << " requires --timing");
        } अन्यथा यदि (nodep->isDelayed()) {
            ध्रुव std::string newvarname = "_" + vrefp->varp()->shortName();
            AstVarScope* ध्रुव dlyvscp
                = m_dlyTmps.make(flp, vrefp->varScopep()->scopep(), 1, newvarname);

            ध्रुव auto dlyRef = [=](VAccess access) {  //
                प्रत्यागम नूतन AstVarRef{flp, dlyvscp, access};
            };

            AstAlwaysPost* ध्रुव postp = नूतन AstAlwaysPost{flp};
            AstIf* ध्रुव ifp = नूतन AstIf{flp, dlyRef(VAccess::READ)};
            postp->addStmtsp(ifp);

            UASSERT_OBJ(m_activep, nodep, "No active to handle FireEvent");
            Ast_Sen_वृक्ष* senTreep = m_activep->sentreep();
            AstActive* activep = nullptr;
            यदि (m_inSuspendableOrFork && (senTreep->hasInitial() || senTreep->hasClocked())) {
                // Suspendable code can trigger whenever it resumes, so fire when its NBAs are
                // committed (see visit(AstNodeProcedure*)), clearing the flag like
                // Scheme::FlagUnique
                senTreep = नूतन Ast_Sen_वृक्ष{
                    flp, senTreep->hasClocked() ? senTreep->sensesp()->clone_वृक्ष(सत्य) : nullptr};
                m_nbaEventSenTreeps.ढकला_back(senTreep);
                m_needsInitialTrigger |= m_timingDomains.empty();
                dlyvscp->varp()->setIgnorePostWrite();
                ifp->addThensp(नूतन AstAssign{flp, dlyRef(VAccess::WRITE),
                                             नूतन AstConst{flp, AstConst::BitFalse{}}});
                activep = नूतन AstActive{flp, "nba-event", senTreep};
                activep->sen_वृक्ष_Storep(senTreep);
            } अन्यथा {
                activep = नूतन AstActive{flp, "nba-event", senTreep};
                AstAlwaysPre* ध्रुव prep = नूतन AstAlwaysPre{flp};
                prep->addStmtsp(नूतन AstAssign{flp, dlyRef(VAccess::WRITE),
                                              नूतन AstConst{flp, AstConst::BitFalse{}}});
                activep->addStmtsp(prep);
            }
            ifp->addThensp(newp);
            m_activep->addNextHere(activep);
            activep->addStmtsp(postp);

            newp = नूतन AstAssign{flp, dlyRef(VAccess::WRITE),
                                 नूतन AstConst{flp, AstConst::BitTrue{}}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstAssignDly* nodep) override {
        // Prevent double processing due to AstExprStmt being moved before this node
        यदि (nodep->user1SetOnce()) प्रत्यागम;

        यदि (m_cfuncp) {
            यदि (!v3Global.rootp()->nbaEventp()) {
                nodep->v3warn(
                    E_NOTIMING,
                    "Delayed assignment in a non-inlined function/task requires --timing");
            }
            प्रत्यागम;
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
            यदि (Ast_अभिव्यक्ती_Stmt* ध्रुव exprp = VN_CAST(currp, अभिव्यक्ती_Stmt)) {
                // Move statements before the NBA
                nodep->addHereThisAsNext(exprp->stmtsp()->unlinkFrBackWithNext());
                // Replace with result
                currp->replaceWith(exprp->resultp()->unlinkFrBack());
                // Get rid of the AstExprStmt
                VL_DO_DANGLING2(ढकला_Deletep(currp), currp, exprp);
            } अन्यथा यदि (AstVarRef* ध्रुव refp = VN_CAST(currp, VarRef)) {
                // Ignore reads (e.g.: '_[*here*] <= _')
                यदि (refp->access().isReadOnly()) प्रत्यागम;
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
        AstVarScope* ध्रुव vscp = m_currNbaLhsRefp->varScopep();
        // Record it on first encounter
        VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
        यदि (!vscpInfo.m_firstNbaRefp) {
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
        यदि (m_activep->sentreep()->hasClocked()) {
            यदि (vscpInfo.m_fistActivep != m_activep) {
                AstVar* ध्रुव varp = vscp->varp();
                यदि (!varp->user1SetOnce()
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
        यदि (nodep == m_currNbaLhsRefp) प्रत्यागम;
        // Only care about write refs
        यदि (!nodep->access().isWriteOrRW()) प्रत्यागम;
        // Record write reference
        recordWriteRef(nodep, असत्य);
    }
    शून्य visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_CCall* ध्रुव nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ध्रुव auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        यदि (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } अन्यथा {
            प्रति (AstCFunc* ध्रुव cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    शून्य visit(AstCFunc* ध्रुव nodep) override {
        ध्रुव auto& मूल्य = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        यदि (मूल्य.m_state != CFuncCache::UNINITIALIZED) प्रत्यागम;
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
