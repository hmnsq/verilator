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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Delayed.h"

#समावेश "V3AstUserAllocator.h"
#समावेश "V3ClassGraph.h"
#समावेश "V3Const.h"
#समावेश "V3LinkLValue.h"
#समावेश "V3SharedTmps.h"
#समावेश "V3Stats.h"

#समावेश <deque>
#समावेश <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert AstAssignDlys (NBAs)

श्रेणी DelayedVisitor final : खुला VNVisitor {
    // TYPES

    // The various NBA conversion schemes, including error cases
    क्रमागत श्रेणी Scheme : uint8_t {
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
    श्रेणी VarScopeInfo final {
    खुला:
        // First write reference encountered to the VarScope as the target on an NBA
        स्थिर AstVarRef* m_firstNbaRefp = nullptr;
        // Active block 'm_firstNbaRefp' is under
        स्थिर AstActive* m_fistActivep = nullptr;
        bool m_whole = false;  // Used on LHS of NBA directly via VarRef
        bool m_partial = false;  // Used on LHS of NBA under a Sel
        bool m_inLoop = false;  // Used on LHS of NBA in a loop
        bool m_inSuspOrFork = false;  // Used on LHS of NBA in suspendable process or fork
        Scheme m_scheme = Scheme::Undecided;  // Conversion scheme to use for this variable

    निजी:
        // Combined sensitivities of all NBAs targeting this variable
        Ast_Sen_वृक्ष* m_senTreep = nullptr;

        // Union of stuff needed for the various schemes - use accessors below!
        जोड़ {
            काष्ठा {  // Stuff needed for Scheme::ShadowVar
                AstVarScope* vscp;  // The shadow variable
            } m_shadowVariableKit;
            काष्ठा {  // Stuff needed for Scheme::ShadowVarMasked
                AstVarScope* vscp;  // The shadow variable
                AstVarScope* maskp;  // The mask variable
            } m_shadowVarMaskedKit;
            काष्ठा {  // Stuff needed for Scheme::FlagShared
                AstActive* activep;  // The active block for the Pre/Post logic
                AstAlwaysPost* postp;  // The post block for commiting results
                AstVarScope* commitFlagp;  // The commit flag variable, for reuse
                AstIf* commitIfp;  // The previous if statement for committing, for reuse
            } m_flagSharedKit;
            काष्ठा {  // Stuff needed for Scheme::FlagUnique
                AstAlwaysPost* postp;  // The post block for commiting results
            } m_flagUniqueKit;
            काष्ठा {  // Stuff needed for Scheme::ValueQueueWhole/Scheme::ValueQueuePartial
                AstVarScope* vscp;  // The commit queue variable
            } m_मूल्य_Queue_Kit;
        } m_kitUnion;

    खुला:
        VarScopeInfo() = शेष;
        ~VarScopeInfo() {
            // Might not be linked if there was an error
            अगर (!m_senTreep->backp()) VL_DO_DANGLING(m_senTreep->delete_वृक्ष(), m_senTreep);
        }
        // Accessors for the above union fields
        स्वतः& shadowVariableKit() {
            UASSERT(m_scheme == Scheme::ShadowVar, "Inconsistent Scheme");
            वापस m_kitUnion.m_shadowVariableKit;
        }
        स्वतः& shadowVarMaskedKit() {
            UASSERT(m_scheme == Scheme::ShadowVarMasked, "Inconsistent Scheme");
            वापस m_kitUnion.m_shadowVarMaskedKit;
        }
        स्वतः& flagSharedKit() {
            UASSERT(m_scheme == Scheme::FlagShared, "Inconsistent Scheme");
            वापस m_kitUnion.m_flagSharedKit;
        }
        स्वतः& flagUniqueKit() {
            UASSERT(m_scheme == Scheme::FlagUnique, "Inconsistent Scheme");
            वापस m_kitUnion.m_flagUniqueKit;
        }
        स्वतः& मूल्य_Queue_Kit() {
            UASSERT(m_scheme == Scheme::मूल्य_Queue_Partial || m_scheme == Scheme::मूल्य_Queue_Whole,
                    "Inconsistent Scheme");
            वापस m_kitUnion.m_मूल्य_Queue_Kit;
        }

        // Accessor
        Ast_Sen_वृक्ष* senTreep() स्थिर { वापस m_senTreep; }

        // Add sensitivities
        व्योम addSensitivity(AstSenItem* nodep) {
            अगर (!m_senTreep) m_senTreep = नया Ast_Sen_वृक्ष{nodep->fileline(), nullptr};
            // Add a copy of each term
            m_senTreep->addSensesp(nodep->clone_वृक्ष(true));
            // Remove duplicates
            V3Const::constifyExpensiveEdit(m_senTreep);
        }
        // cppcheck-suppress constParameterPointer
        व्योम addSensitivity(Ast_Sen_वृक्ष* nodep) { addSensitivity(nodep->sensesp()); }
    };

    // Data structure to keep track of all writes to
    काष्ठा WriteReference final {
        AstVarRef* m_refp = nullptr;  // The reference
        bool m_isNBA = false;  // True if an NBA write
        bool m_inNonComb = false;  // True if reference is known to be in non-combinational logic
        WriteReference() = शेष;
        WriteReference(AstVarRef* refp, bool isNBA, bool inNonComb)
            : m_refp{refp}
            , m_isNBA{isNBA}
            , m_inNonComb{inNonComb} {}
    };

    // Data required to lower AstAssignDelay later
    काष्ठा NBA final {
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
    स्थिर VNUser1InUse m_user1InUse;
    स्थिर VNUser2InUse m_user2InUse;
    स्थिर VNUser3InUse m_user3InUse;

    काष्ठा CFuncCache final {
        VInsertionSet<Ast_Sen_वृक्ष*> m_timingDomains;  // What shall be added to m_timingDomains
        std::set<AstCFunc*>
            m_includes;  // CFuncs whose CFuncCache shall be included into this - this is used to
                         // break cycles: A->B->A (instead of visiting A while it is still begin
                         // visited B just marks that it includes A)
        क्रमागत State : uint8_t {
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
    std::map<std::pair<स्थिर Ast_गाँठ_DType*, bool>, AstNBACommitQueueDType*> m_cqDTypeps;

    स्थिर std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::vector<स्थिर AstCFunc*> m_call_ढेर;  // Current callstack of AstCFuncs

    // STATE - for current visit position (use VL_RESTORER)
    AstActive* m_activep = nullptr;  // Current activate
    स्थिर AstCFunc* m_cfuncp = nullptr;  // Current public C Function
    Ast_गाँठ_Procedure* m_procp = nullptr;  // Current process
    AstScope* m_scopep = nullptr;  // Current scope
    bool m_inLoop = false;  // True in for loops
    bool m_inSuspendableOrFork = false;  // True in suspendable processes and forks
    bool m_ignoreBlkAndNBlk = false;  // Suppress delayed assignment BLKANDNBLK
    bool m_inNonCombLogic = false;  // We are in non-combinational logic
    bool m_needsInitialTrigger = false;  // Whether a NodeProcedure needs a initial trigger
    std::vector<Ast_Sen_वृक्ष*> m_nbaEventSenTreeps;  // Sensitivities of '->>' in the process
    AstVarRef* m_currNbaLhsRefp = nullptr;  // Current NBA LHS variable reference

    // STATE - during NBA conversion (after visit)
    std::vector<NBA> m_nbas;  // AstAssignDly instances to lower at the end
    std::vector<AstVarScope*> m_vscps;  // Target variables on LHSs of NBAs
    AstAssignDly* m_अग्र_Dlyp = nullptr;  // The nextp of the previous AstAssignDly
    AstVarScope* m_पूर्व_Vscp = nullptr;  // The target of the previous AstAssignDly

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
    bool checkMixedUsage(स्थिर AstVarScope* vscp, bool isIntegralOrPacked) {

        काष्ठा Ref final {
            AstVarRef* m_refp;  // The reference
            bool m_inNonComb;  // True if known to be in non-combinational logic
            पूर्णांक m_lsb;  // LSB of accessed range
            पूर्णांक m_msb;  // MSB of accessed range
            Ref(AstVarRef* refp, bool inNonComb, पूर्णांक lsb, पूर्णांक msb)
                : m_refp{refp}
                , m_inNonComb{inNonComb}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };

        std::vector<Ref> blkRefs;  // Blocking writes
        std::vector<Ref> nbaRefs;  // Non-blockign writes

        स्थिर पूर्णांक width = isIntegralOrPacked ? vscp->width() : 1;

        क्रम (स्थिर स्वतः& writeRef : m_writeRefs(vscp)) {
            पूर्णांक lsb = 0;
            पूर्णांक msb = width - 1;
            अगर (स्थिर AstSel* स्थिर selp = VN_CAST(writeRef.m_refp->backp(), Sel)) {
                अगर (VN_IS(selp->lsbp(), Const)) {
                    lsb = selp->lsbConst();
                    msb = selp->msbConst();
                }
            }
            अगर (writeRef.m_isNBA) {
                nbaRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            } अन्यथा {
                blkRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            }
        }
        // We only run this function on targets of NBAs, so there should be at least one...
        UASSERT_OBJ(!nbaRefs.empty(), vscp, "Did not record NBA write");
        // If no blocking upadte, then we are good
        अगर (blkRefs.empty()) वापस false;

        // If the blocking assignment is in non-combinational logic (i.e.:
        // in logic that has an explicit trigger), then we can safely
        // implement it (there is no race between clocked logic and post
        // scheduled logic), so need not error
        blkRefs.erase(std::remove_if(blkRefs.begin(), blkRefs.end(),
                                     [](स्थिर Ref& ref) { वापस ref.m_inNonComb; }),
                      blkRefs.end());

        // If nothing left, then we need not error
        अगर (blkRefs.empty()) वापस true;

        // If not a packed variable, warn here as we can't prove independence
        अगर (!isIntegralOrPacked) {
            स्थिर Ref& blkRef = blkRefs.front();
            स्थिर Ref& nbaRef = nbaRefs.front();
            vscp->v3warn(
                BLKANDNBLK,
                "Unsupported: Blocking and non-blocking assignments to same non-packed variable: "
                    << vscp->varp()->prettyNameQ() << '\n'
                    << vscp->warnContextPrimary() << '\n'
                    << blkRef.m_refp->warnOther() << "... Location of blocking assignment\n"
                    << blkRef.m_refp->warnContextSecondary() << '\n'
                    << nbaRef.m_refp->warnOther() << "... Location of nonblocking assignment\n"
                    << nbaRef.m_refp->warnContextSecondary());
            वापस true;
        }

        // We need to error if we can't prove the written bits are independent

        // Sort refs by interval
        स्थिर स्वतः lessThanRef = [](स्थिर Ref& a, स्थिर Ref& b) {
            अगर (a.m_lsb != b.m_lsb) वापस a.m_lsb < b.m_lsb;
            वापस a.m_msb < b.m_msb;
        };
        std::stable_sort(blkRefs.begin(), blkRefs.end(), lessThanRef);
        std::stable_sort(nbaRefs.begin(), nbaRefs.end(), lessThanRef);
        // Iterate both vectors, checking for overlap
        स्वतः bIt = blkRefs.begin();
        स्वतः nIt = nbaRefs.begin();
        जबतक (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            अगर (lessThanRef(*bIt, *nIt)) {
                अगर (nIt->m_lsb <= bIt->m_msb) अवरोध;  // Stop on Overlap
                ++bIt;
            } अन्यथा {
                अगर (bIt->m_lsb <= nIt->m_msb) अवरोध;  // Stop on Overlap
                ++nIt;
            }
        }

        // If we found an overlapping range that cannot be safely implemented, then wran...
        अगर (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            स्थिर Ref& blkRef = *bIt;
            स्थिर Ref& nbaRef = *nIt;
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

        वापस true;
    }

    // Choose the NBA scheme used for the given variable.
    Scheme chooseScheme(स्थिर AstVarScope* vscp, स्थिर VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::Undecided, vscp, "NBA scheme already decided");

        स्थिर Ast_गाँठ_DType* स्थिर dtypep = vscp->dtypep()->skipRefp();
        // Unpacked arrays
        अगर (स्थिर AstUnpackArrayDType* स्थिर uaDTypep = VN_CAST(dtypep, UnpackArrayDType)) {
            // If whole array is target of NBA, use ShadowVar, but not in a suspendable process or
            // fork, which resume outside the 'nba' region, so its 'pre' logic would overwrite the
            // value they write in the shadow variable
            अगर (vscpInfo.m_whole) {
                वापस vscpInfo.m_inSuspOrFork ? Scheme::FlagUnique : Scheme::ShadowVar;
            }
            // Basic underlying type of elements, if any.
            स्थिर AstBasicDType* स्थिर basicp = uaDTypep->basicp();
            // If used in a loop, we must have a dynamic commit queue. (Also works in suspendables)
            अगर (vscpInfo.m_inLoop) {
                // Arrays with compound element types are currently not supported in loops
                अगर (!basicp
                    || !(basicp->isIntegralOrPacked()  //
                         || basicp->isDouble()  //
                         || basicp->isString())) {
                    वापस Scheme::UnsupportedCompoundArrayInLoop;
                }
                अगर (vscpInfo.m_partial) वापस Scheme::मूल्य_Queue_Partial;
                वापस Scheme::मूल्य_Queue_Whole;
            }
            // A suspendable process or fork can resume outside the 'nba' region, so the 'pre'
            // logic of the other schemes would undo its updates. The unique flag scheme has none.
            अगर (vscpInfo.m_inSuspOrFork) वापस Scheme::FlagUnique;
            // Otherwise if an array of packed/basic elements, use the shared flag scheme
            अगर (basicp) वापस Scheme::FlagShared;
            // Finally fall back on the shadow variable scheme, e.g. for
            // arrays of unpacked structs. This will be slow.
            // TODO: generic LHS scheme as discussed in #5092
            वापस Scheme::ShadowVar;
        }

        // Likewise, the 'pre' logic of the shadow variable scheme would undo the updates of a
        // suspendable process or fork, so use the unique flag scheme
        अगर (vscpInfo.m_inSuspOrFork) वापस Scheme::FlagUnique;

        स्थिर bool isIntegralOrPacked = dtypep->isIntegralOrPacked();
        // Check for mixed usage (this also warns if not OK)
        अगर (checkMixedUsage(vscp, isIntegralOrPacked)) {
            // If it's a variable updated by both blocking and non-blocking
            // assignments, use the ShadowVarMasked schem if masked update is
            // possible. This can handle blocking and non-blocking updates to
            // inpdendent parts correctly at run-time, and always works, even
            // in loops or other dynamic context.
            अगर (isIntegralOrPacked) वापस Scheme::ShadowVarMasked;
            // If it's inside a loop, use Scheme::ShadowVar, which is safe,
            // but will generate incorrect code if a partial update is used
            अगर (vscpInfo.m_inLoop) वापस Scheme::ShadowVar;
            // Otherwise (for not packed variables), use the FlagUnique scheme,
            // which at least handles partial updates correctly, but might break
            // in loops or other dynamic context
            वापस Scheme::FlagUnique;
        }

        // Otherwise use the simple shadow variable scheme
        वापस Scheme::ShadowVar;
    }

    // Given an expression 'exprp', return a new expression that always evaluates to the
    // value of the given expression at this point in the program. That is:
    // - If given a non-constant expression, create a new temporary AstVarScope under the given
    //   'scopep', with the given 'name', assign the expression to it, and return a read reference
    //   to the new AstVarScope.
    // - If given a constant, just return that constant.
    // New statements are inserted before 'insertp'
    Ast_गाँठ_व्यंजक* captureVal(AstScope* स्थिर scopep, Ast_गाँठ_Stmt* स्थिर insertp,
                            Ast_गाँठ_व्यंजक* स्थिर exprp, स्थिर std::string& name) {
        UASSERT_OBJ(!exprp->backp(), exprp, "Should have been unlinked");
        File_पंक्ति* स्थिर flp = exprp->fileline();
        अगर (VN_IS(exprp, Const)) वापस exprp;
        // TODO: there are some const variables that could be simply referenced here
        AstVarScope* स्थिर tmpVscp = m_dlyTmps.make(flp, scopep, exprp->dtypep(), name);
        insertp->add_Here_This_As_अग्र(
            नया AstAssign{flp, नया AstVarRef{flp, tmpVscp, VAccess::WRITE}, exprp});
        वापस नया AstVarRef{flp, tmpVscp, VAccess::READ};
    };

    // Similar to 'captureVal', but captures an LValue expression. That is, the returned
    // expression will reference the same location as the input expression, at this point in the
    // program.
    Ast_गाँठ_व्यंजक* captureLhs(AstScope* स्थिर scopep, Ast_गाँठ_Stmt* स्थिर insertp,
                            Ast_गाँठ_व्यंजक* स्थिर lhsp, स्थिर std::string& baseName) {
        UASSERT_OBJ(!lhsp->backp(), lhsp, "Should have been unlinked");
        // Running node pointer
        Ast_गाँठ_व्यंजक* nodep = lhsp;
        // Capture AstSel indices - there should be only one
        अगर (AstSel* स्थिर selp = VN_CAST(nodep, Sel)) {
            स्थिर std::string tmpName{"Lsb" + baseName};
            selp->lsbp(captureVal(scopep, insertp, selp->lsbp()->unlinkFrBack(), tmpName));
            // Continue with target
            nodep = selp->fromp();
        }
        UASSERT_OBJ(!VN_IS(nodep, Sel), lhsp, "Multiple 'AstSel' applied to LHS reference");
        // Capture the indices of selects - might be many, also below struct members. If a handle
        // selects the instance the target is a member of, capture the handle instead, as it is
        // only read, to select the target when the NBA is executed (IEEE 1800-2023 10.4.2).
        माप_प्रकार nArraySels = 0;
        जबतक (true) {
            अगर (Ast_गाँठ_Sel* स्थिर selp = VN_CAST(nodep, गाँठ_Sel)) {
                स्थिर std::string tmpName{"Dim" + std::to_string(nArraySels++) + baseName};
                selp->bitp(captureVal(scopep, insertp, selp->bitp()->unlinkFrBack(), tmpName));
                nodep = selp->fromp();
            } अन्यथा अगर (स्थिर AstStructSel* स्थिर selp = VN_CAST(nodep, StructSel)) {
                nodep = selp->fromp();
            } अन्यथा अगर (AstMemberSel* स्थिर selp = VN_CAST(nodep, MemberSel)) {
                Ast_गाँठ_व्यंजक* स्थिर handlep = selp->fromp()->unlinkFrBack();
                V3LinkLValue::linkLValueSet(handlep, VAccess::READ);
                AstVarRef* स्थिर refp
                    = VN_AS(captureVal(scopep, insertp, handlep, "Handle" + baseName), VarRef);
                refp->varp()->setIgnorePostWrite();
                // Like the handle it replaces, mark it written, as is the member it selects
                refp->access(selp->access());
                selp->fromp(refp);
                वापस lhsp;
            } अन्यथा {
                अवरोध;
            }
        }
        // What remains must be the variable, or some sort of select, we assume can reuse it
        UASSERT_OBJ(nodep->isPure(), lhsp, "Malformed LHS in NBA");
        // Now have been converted to use the captured values
        वापस lhsp;
    }

    व्योम add_CFunc_Cached_मूल्यावली(स्थिर AstCFunc* स्थिर cfuncp,
                              std::unordered_set<स्थिर AstCFunc*>& visited) {
        अगर (!visited.जोड़ो(cfuncp).second) वापस;
        CFuncCache& मूल्य = m_cfuncsCache(cfuncp);
        m_timingDomains.जोड़ो(मूल्य.m_timingDomains.begin(), मूल्य.m_timingDomains.end());
        m_non_Inlined_CAwaits_With_Sen_वृक्ष += मूल्य.m_timingDomains.आकार();
        क्रम (स्थिर AstCFunc* स्थिर includedp : मूल्य.m_includes) {
            add_CFunc_Cached_मूल्यावली(includedp, visited);
        }
    }

    // Create a temporary variable in the given 'scopep', with the given 'name', and with 'dtypep'
    // type, with the bits selected by 'sLsbp'/'sWidthp' set to 'valuep', other bits set to zero.
    // Insert new statements before 'insertp'.
    // Returns a read reference to the temporary variable.
    AstVarRef* createWidened(File_पंक्ति* flp, AstScope* scopep, Ast_गाँठ_DType* dtypep,
                             Ast_गाँठ_व्यंजक* sLsbp, पूर्णांक sWidth, स्थिर std::string& name,
                             Ast_गाँठ_व्यंजक* valuep, Ast_गाँठ* insertp) {
        // Create temporary variable.
        AstVarScope* स्थिर tp = m_dlyTmps.make(flp, scopep, dtypep, name);
        // Zero it
        AstConst* स्थिर zerop = नया AstConst{flp, AstConst::DTyped{}, dtypep};
        zerop->अंक().setAllBits0();
        insertp->add_Here_This_As_अग्र(
            नया AstAssign{flp, नया AstVarRef{flp, tp, VAccess::WRITE}, zerop});
        // Set the selected bits to 'valuep'
        AstSel* स्थिर selp = नया AstSel{flp, नया AstVarRef{flp, tp, VAccess::WRITE},
                                        sLsbp->clone_वृक्ष_Pure(true), sWidth};
        insertp->add_Here_This_As_अग्र(नया AstAssign{flp, selp, valuep});
        // This is the expression to get the value of the temporary
        वापस नया AstVarRef{flp, tp, VAccess::READ};
    }

    // Scheme::ShadowVar
    व्योम prepareSchemeShadowVar(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        File_पंक्ति* स्थिर flp = vscp->fileline();
        AstScope* स्थिर scopep = vscp->scopep();
        // Create the shadow variable
        स्थिर std::string name = "_" + vscp->varp()->shortName();
        AstVarScope* स्थिर shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), name);
        vscpInfo.shadowVariableKit().vscp = shadowVscp;
        // Mark both for V3LifePsot
        vscp->optimizeLifePost(true);
        shadowVscp->optimizeLifePost(true);
        // Create the AstActive for the Pre/Post logic
        AstActive* स्थिर activep = नया AstActive{flp, "nba-shadow-variable", vscpInfo.senTreep()};
        activep->sen_वृक्ष_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Pre' scheduled 'shadowVariable = originalVariable' assignment
        AstAlwaysPre* स्थिर prep = नया AstAlwaysPre{flp};
        activep->addStmtsp(prep);
        prep->addStmtsp(नया AstAssign{flp, नया AstVarRef{flp, shadowVscp, VAccess::WRITE},
                                      नया AstVarRef{flp, vscp, VAccess::READ}});
        // Add 'Post' scheduled 'originalVariable = shadowVariable' assignment
        AstAlwaysPost* स्थिर postp = नया AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        postp->addStmtsp(नया AstAssign{flp, नया AstVarRef{flp, vscp, VAccess::WRITE},
                                       नया AstVarRef{flp, shadowVscp, VAccess::READ}});
    }
    व्योम convertSchemeShadowVar(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        AstVarScope* स्थिर shadowVscp = vscpInfo.shadowVariableKit().vscp;

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* स्थिर refp) {
            अगर (!refp->access().isWriteOnly()) वापस;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
    }

    // Scheme::ShadowVarMasked
    व्योम prepareSchemeShadowVarMasked(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        File_पंक्ति* स्थिर flp = vscp->fileline();
        AstScope* स्थिर scopep = vscp->scopep();
        // Create the shadow variable
        स्थिर std::string shadowName = "_" + vscp->varp()->shortName();
        AstVarScope* स्थिर shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), shadowName);
        vscpInfo.shadowVarMaskedKit().vscp = shadowVscp;
        // Create the makk variable
        स्थिर std::string maskName = "Mask__" + vscp->varp()->shortName();
        AstVarScope* स्थिर maskVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), maskName);
        maskVscp->varp()->setIgnorePostWrite();
        vscpInfo.shadowVarMaskedKit().maskp = maskVscp;
        // Create the AstActive for the Post logic
        AstActive* स्थिर activep
            = नया AstActive{flp, "nba-shadow-var-masked", vscpInfo.senTreep()};
        activep->sen_वृक्ष_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit and mask clear
        AstAlwaysPost* स्थिर postp = नया AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit - vscp = (shadowVscp & maskVscp) | (vscp & ~maskVscp);
        postp->addStmtsp(नया AstAssign{
            flp, नया AstVarRef{flp, vscp, VAccess::WRITE},
            नया AstOr{flp,
                      नया AstAnd{flp, नया AstVarRef{flp, shadowVscp, VAccess::READ},
                                 नया AstVarRef{flp, maskVscp, VAccess::READ}},
                      नया AstAnd{flp, नया AstVarRef{flp, vscp, VAccess::READ},
                                 नया AstNot{flp, नया AstVarRef{flp, maskVscp, VAccess::READ}}}}});
        vscp->varp()->setIgnorePostRead();
        // Clar the mask - maskVscp = '0;
        postp->addStmtsp(
            नया AstAssign{flp, नया AstVarRef{flp, maskVscp, VAccess::WRITE},
                          नया AstConst{flp, AstConst::Widthed_मूल्य{}, maskVscp->width(), 0}});
    }
    व्योम convertSchemeShadowVarMasked(AstAssignDly* nodep, AstVarScope* vscp,
                                      VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        AstVarScope* स्थिर shadowVscp = vscpInfo.shadowVarMaskedKit().vscp;
        AstVarScope* स्थिर maskVscp = vscpInfo.shadowVarMaskedKit().maskp;

        Ast_गाँठ_व्यंजक* lhsClonep = nodep->lhsp()->clone_वृक्ष(false);

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* स्थिर refp) {
            अगर (!refp->access().isWriteOnly()) वापस;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
        // Set the same bits in the mask to 1
        lhsClonep->foreach([&](AstVarRef* स्थिर refp) {
            अगर (!refp->access().isWriteOnly()) वापस;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(maskVscp);
            refp->varp(maskVscp->varp());
        });
        File_पंक्ति* स्थिर flp = nodep->fileline();
        AstConst* स्थिर onesp = नया AstConst{flp, AstConst::DTyped{}, lhsClonep->dtypep()};
        onesp->अंक().setAllBits1();
        nodep->add_अग्र_Here(नया AstAssign{flp, lhsClonep, onesp});
    }

    // Scheme::FlagShared
    व्योम prepareSchemeFlagShared(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_पंक्ति* स्थिर flp = vscp->fileline();
        AstScope* स्थिर scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* स्थिर activep = नया AstActive{flp, "nba-flag-shared", vscpInfo.senTreep()};
        activep->sen_वृक्ष_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        vscpInfo.flagSharedKit().activep = activep;
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* स्थिर postp = नया AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagSharedKit().postp = postp;
        // Initialize
        vscpInfo.flagSharedKit().commitFlagp = nullptr;
        vscpInfo.flagSharedKit().commitIfp = nullptr;
    }
    व्योम convertSchemeFlagShared(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_पंक्ति* स्थिर flp = vscp->fileline();
        AstScope* स्थिर scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        स्थिर std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_गाँठ_व्यंजक* स्थिर capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_गाँठ_व्यंजक* स्थिर capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Is this NBA adjacent after the previously processed NBA?
        स्थिर bool consecutive = nodep == m_अग्र_Dlyp;
        m_अग्र_Dlyp = VN_CAST(nodep->nextp(), AssignDly);

        VarScopeInfo* स्थिर पूर्व_Vscp_Infop = consecutive ? &m_vscpInfo(m_पूर्व_Vscp) : nullptr;

        // We can reuse the flag of the previous assignment if:
        स्थिर bool reuseTheFlag =
            // Consecutive NBAs
            consecutive
            // ... that use the same scheme
            && पूर्व_Vscp_Infop->m_scheme == Scheme::FlagShared
            // ... and are in the same scope as the target variable
            && scopep == vscp->scopep()
            // ... and share the same overall update domain
            && पूर्व_Vscp_Infop->senTreep()->same_वृक्ष(vscpInfo.senTreep());

        अगर (!reuseTheFlag) {
            // Create new flag
            AstVarScope* स्थिर flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
            // Set the flag at the original NBA
            nodep->add_Here_This_As_अग्र(  //
                नया AstAssign{flp, नया AstVarRef{flp, flagVscp, VAccess::WRITE},
                              नया AstConst{flp, AstConst::BitTrue{}}});
            // Add the 'Pre' scheduled reset for the flag
            AstAlwaysPre* स्थिर prep = नया AstAlwaysPre{flp};
            vscpInfo.flagSharedKit().activep->addStmtsp(prep);
            prep->addStmtsp(नया AstAssign{flp, नया AstVarRef{flp, flagVscp, VAccess::WRITE},
                                          नया AstConst{flp, AstConst::BitFalse{}}});
            // Add the 'Post' scheduled commit
            AstIf* स्थिर ifp = नया AstIf{flp, नया AstVarRef{flp, flagVscp, VAccess::READ}};
            vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
            vscpInfo.flagSharedKit().commitFlagp = flagVscp;
            vscpInfo.flagSharedKit().commitIfp = ifp;
        } अन्यथा {
            अगर (vscp != m_पूर्व_Vscp) {
                // Different variable, ensure the commit block exists for this variable,
                // can reuse existing one with the same flag, otherwise create a new one.
                AstVarScope* स्थिर flagVscp = पूर्व_Vscp_Infop->flagSharedKit().commitFlagp;
                UASSERT_OBJ(flagVscp, nodep, "Commit flag of previous assignment should exist");
                अगर (vscpInfo.flagSharedKit().commitFlagp != flagVscp) {
                    AstIf* स्थिर ifp = नया AstIf{flp, नया AstVarRef{flp, flagVscp, VAccess::READ}};
                    vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
                    vscpInfo.flagSharedKit().commitFlagp = flagVscp;
                    vscpInfo.flagSharedKit().commitIfp = ifp;
                }
            } अन्यथा {
                // Same variable, reuse the commit block of the previous assignment
                vscpInfo.flagSharedKit().commitFlagp = पूर्व_Vscp_Infop->flagSharedKit().commitFlagp;
                vscpInfo.flagSharedKit().commitIfp = पूर्व_Vscp_Infop->flagSharedKit().commitIfp;
            }
            ++m_nSharedSetFlags;
        }
        // Commit the captured value to the captured destination
        vscpInfo.flagSharedKit().commitIfp->addThensp(
            नया AstAssign{flp, capturedLhsp, capturedRhsp});

        // Remember the variable for the next NBA
        m_पूर्व_Vscp = vscp;

        // Delete original NBA
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::FlagUnique
    व्योम prepareSchemeFlagUnique(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_पंक्ति* स्थिर flp = vscp->fileline();
        AstScope* स्थिर scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* स्थिर activep = नया AstActive{flp, "nba-flag-unique", vscpInfo.senTreep()};
        activep->sen_वृक्ष_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* स्थिर postp = नया AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagUniqueKit().postp = postp;
    }
    व्योम convertSchemeFlagUnique(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_पंक्ति* स्थिर flp = vscp->fileline();
        AstScope* स्थिर scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        स्थिर std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_गाँठ_व्यंजक* स्थिर capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_गाँठ_व्यंजक* स्थिर capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Create new flag
        AstVarScope* स्थिर flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
        flagVscp->varp()->setIgnorePostWrite();
        // Set the flag at the original NBA
        nodep->add_Here_This_As_अग्र(  //
            नया AstAssign{flp, नया AstVarRef{flp, flagVscp, VAccess::WRITE},
                          नया AstConst{flp, AstConst::BitTrue{}}});
        // Add the 'Post' scheduled commit
        AstIf* स्थिर ifp = नया AstIf{flp, नया AstVarRef{flp, flagVscp, VAccess::READ}};
        vscpInfo.flagUniqueKit().postp->addStmtsp(ifp);
        // Immediately clear the flag
        ifp->addThensp(नया AstAssign{flp, नया AstVarRef{flp, flagVscp, VAccess::WRITE},
                                     नया AstConst{flp, AstConst::BitFalse{}}});
        // Commit the value
        ifp->addThensp(नया AstAssign{flp, capturedLhsp, capturedRhsp});

        // Delete original NBA
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::ValueQueuePartial/Scheme::ValueQueueWhole
    ढाँचा <bool N_Partial>
    व्योम prepare_Scheme_मूल्य_Queue(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(N_Partial ? vscpInfo.m_scheme == Scheme::मूल्य_Queue_Partial
                              : vscpInfo.m_scheme == Scheme::मूल्य_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_पंक्ति* स्थिर flp = vscp->fileline();
        AstScope* स्थिर scopep = vscp->scopep();

        // Create the commit queue variable
        Ast_गाँठ_DType* स्थिर elemDTypep = vscp->dtypep()->skipRefp();
        AstNBACommitQueueDType*& cqDTypep = m_cqDTypeps[{elemDTypep, N_Partial}];
        अगर (!cqDTypep) {
            cqDTypep = नया AstNBACommitQueueDType{flp, elemDTypep, N_Partial};
            v3Global.rootp()->typeTablep()->addTypesp(cqDTypep);
        }
        स्थिर std::string name = "CommitQueue__" + vscp->varp()->shortName();
        AstVarScope* स्थिर queueVscp = m_dlyTmps.make(flp, scopep, cqDTypep, name);
        queueVscp->varp()->noReset(true);
        queueVscp->varp()->setIgnorePostWrite();
        vscpInfo.मूल्य_Queue_Kit().vscp = queueVscp;
        // Create the AstActive for the Post logic
        AstActive* स्थिर activep
            = नया AstActive{flp, "nba-value-queue-whole", vscpInfo.senTreep()};
        activep->sen_वृक्ष_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit
        AstAlwaysPost* स्थिर postp = नया AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit
        AstCMethodHard* स्थिर callp = नया AstCMethodHard{
            flp, नया AstVarRef{flp, queueVscp, VAccess::READWRITE}, VCMethod::NBA_COMMIT};
        callp->dtypeSetVoid();
        // TODO: this is a partial update, so must be READWRITE, but that breaks scheduling
        callp->addPinsp(नया AstVarRef{flp, vscp, VAccess::WRITE});
        postp->addStmtsp(callp->makeStmt());
    }

    व्योम convert_Scheme_मूल्य_Queue(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo,
                                 bool partial) {
        UASSERT_OBJ(partial ? vscpInfo.m_scheme == Scheme::मूल्य_Queue_Partial
                            : vscpInfo.m_scheme == Scheme::मूल्य_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_पंक्ति* स्थिर flp = vscp->fileline();
        AstScope* स्थिर scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        स्थिर std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_गाँठ_व्यंजक* स्थिर capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_गाँठ_व्यंजक* स्थिर capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // RHS value (can be widened/masked iff Partial)
        Ast_गाँठ_व्यंजक* valuep = capturedRhsp;
        // RHS mask (iff Partial)
        Ast_गाँठ_व्यंजक* maskp = nullptr;

        // Running node for LHS deconstruction
        Ast_गाँठ_व्यंजक* lhsNodep = capturedLhsp;

        // If partial updates are required, construct the mask and the widened value
        अगर (partial) {
            // Type of array element
            Ast_गाँठ_DType* स्थिर eDTypep = [&]() -> Ast_गाँठ_DType* {
                Ast_गाँठ_DType* dtypep = vscp->dtypep()->skipRefp();
                जबतक (AstUnpackArrayDType* स्थिर ap = VN_CAST(dtypep, UnpackArrayDType)) {
                    dtypep = ap->subDTypep()->skipRefp();
                }
                वापस dtypep;
            }();

            अगर (स्थिर AstSel* स्थिर lSelp = VN_CAST(lhsNodep, Sel)) {
                // This is a partial assignment.
                // Need to create a mask and widen the value to element size.
                lhsNodep = lSelp->fromp();
                Ast_गाँठ_व्यंजक* स्थिर sLsbp = lSelp->lsbp();
                स्थिर पूर्णांक sWidth = lSelp->widthConst();

                // Create mask value
                maskp = [&]() -> Ast_गाँठ_व्यंजक* {
                    // Constant mask we can compute here
                    अगर (स्थिर AstConst* स्थिर cLsbp = VN_CAST(sLsbp, Const)) {
                        AstConst* स्थिर cp = नया AstConst{flp, AstConst::DTyped{}, eDTypep};
                        cp->अंक().setMask(sWidth, cLsbp->toSInt());
                        वापस cp;
                    }

                    // A non-constant mask we must compute at run-time.
                    AstConst* स्थिर onesp = नया AstConst{flp, AstConst::Widthed_मूल्य{}, sWidth, 0};
                    onesp->अंक().setAllBits1();
                    वापस createWidened(flp, scopep, eDTypep, sLsbp, sWidth, "Mask" + baseName,
                                         onesp, nodep);
                }();

                // Adjust value to element size
                valuep = [&]() -> Ast_गाँठ_व्यंजक* {
                    // Constant value with constant select we can compute here
                    अगर (AstConst* स्थिर cValuep = VN_CAST(valuep, Const)) {
                        अगर (स्थिर AstConst* स्थिर cLsbp = VN_CAST(sLsbp, Const)) {
                            AstConst* स्थिर cp = नया AstConst{flp, AstConst::DTyped{}, eDTypep};
                            cp->अंक().setAllBits0();
                            cp->अंक().opSelInto(cValuep->अंक(), cLsbp->toSInt(), sWidth);
                            VL_DO_DANGLING(valuep->delete_वृक्ष(), valuep);
                            वापस cp;
                        }
                    }

                    // A non-constant value we must adjust.
                    वापस createWidened(flp, scopep, eDTypep, sLsbp, sWidth,  //
                                         "Elem" + baseName, valuep, nodep);
                }();
            } अन्यथा {
                // If this assignment is not partial, set mask to ones and we are done
                AstConst* स्थिर ones = नया AstConst{flp, AstConst::DTyped{}, eDTypep};
                ones->अंक().setAllBits1();
                maskp = ones;
            }
        }

        // Extract array indices
        std::vector<Ast_गाँठ_व्यंजक*> idxps;
        {
            UASSERT_OBJ(VN_IS(lhsNodep, ArraySel), lhsNodep, "Unexpected LHS form");
            जबतक (AstArraySel* स्थिर aSelp = VN_CAST(lhsNodep, ArraySel)) {
                idxps.emplace_back(aSelp->bitp()->unlinkFrBack());
                lhsNodep = aSelp->fromp();
            }
            UASSERT_OBJ(VN_IS(lhsNodep, VarRef), lhsNodep, "Unexpected LHS form");
            std::reverse(idxps.begin(), idxps.end());
        }

        // Done with the LHS at this point
        VL_DO_DANGLING(धकेलो_Deletep(capturedLhsp), capturedLhsp);

        // Enqueue the update at the site of the original NBA
        AstCMethodHard* स्थिर callp = नया AstCMethodHard{
            flp, नया AstVarRef{flp, vscpInfo.मूल्य_Queue_Kit().vscp, VAccess::READWRITE},
            VCMethod::NBA_ENQUEUE};
        callp->dtypeSetVoid();
        callp->addPinsp(valuep);
        अगर (partial) callp->addPinsp(maskp);
        क्रम (Ast_गाँठ_व्यंजक* स्थिर indexp : idxps) callp->addPinsp(indexp);
        nodep->add_Here_This_As_अग्र(callp->makeStmt());

        // Delete original NBA
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Record where a variable is assigned
    व्योम recordWriteRef(AstVarRef* nodep, bool nonBlocking) {
        // Ignore references in certain contexts
        अगर (m_ignoreBlkAndNBlk) वापस;
        // Ignore if it's an array
        // TODO: we do this because it used to be the previous behaviour.
        //       Is it still required, or should we warn for arrays as well?
        //       Scheduling is no different for them...
        //       Clarification: This is OK for arrays of primitive types, but
        //       arrays that use the ShadowVar scheme don't work...
        अगर (VN_IS(nodep->varScopep()->dtypep()->skipRefp(), UnpackArrayDType)) वापस;

        m_writeRefs(nodep->varScopep()).emplace_back(nodep, nonBlocking, m_inNonCombLogic);
    }

    ढाँचा <typename Procedure_T>
    अटल bool isProcedureWithSentreep(स्थिर Ast_गाँठ_Procedure* स्थिर nodep) {
        स्थिर Procedure_T* स्थिर procedurep = Ast_गाँठ::cast<Procedure_T>(nodep);
        वापस procedurep && procedurep->sentreep();
    }

    // Visit AstCFunc from a AstNodeCCall - this is made into a separate quasi-visitor because
    // AstCFunc that is not called from the code (e.g.: DPI exports) does not need to be visited
    // this way. Also, not visiting such AstCFuncs allows to avoid caching results for them which
    // this function does - which could lead to excessive memory usage
    व्योम visitCalledCFunc(AstCFunc* स्थिर nodep) {
        CFuncCache& मूल्य = m_cfuncsCache(nodep);
        चयन (मूल्य.m_state) {
        हाल CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_timingDomains);

            // Visit
            मूल्य.m_state = CFuncCache::VISITING;
            m_call_ढेर.धकेलो_back(nodep);
            {
                VL_RESTORER(m_cfuncp);
                m_cfuncp = nodep;
                iterateChildren(nodep);
            }
            m_call_ढेर.निकालो_back();
            मूल्य.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_timingDomains, मूल्य.m_timingDomains);
        } अवरोध;
        हाल CFuncCache::VISITING: {
            क्रम (माप_प्रकार i = m_call_ढेर.आकार() - 1; m_call_ढेर.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ढेर[i]).m_includes.जोड़ो(nodep);
            }
            वापस;  // Break recursion
        }
        हाल CFuncCache::INITIALIZED: अवरोध;
        }
        std::unordered_set<स्थिर AstCFunc*> visited;
        add_CFunc_Cached_मूल्यावली(nodep, visited);
    }

    // VISITORS
    व्योम visit(AstNetlist* nodep) override {
        iterateChildren(nodep);
        // Decide which scheme to use for each variable and do the 'prepare' step
        क्रम (AstVarScope* स्थिर vscp : m_vscps) {
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            vscpInfo.m_scheme = chooseScheme(vscp, vscpInfo);
            // Run 'prepare' step
            चयन (vscpInfo.m_scheme) {
            हाल Scheme::Undecided:  // LCOV_EXCL_START
                UASSERT_OBJ(false, vscp, "Failed to choose NBA scheme");
                अवरोध;  // LCOV_EXCL_STOP
            हाल Scheme::UnsupportedCompoundArrayInLoop: {
                // Will report error at the site of the NBA
                अवरोध;
            }
            हाल Scheme::ShadowVar: {
                ++m_nSchemeShadowVar;
                prepareSchemeShadowVar(vscp, vscpInfo);
                अवरोध;
            }
            हाल Scheme::ShadowVarMasked: {
                ++m_nSchemeShadowVarMasked;
                prepareSchemeShadowVarMasked(vscp, vscpInfo);
                अवरोध;
            }
            हाल Scheme::FlagShared: {
                ++m_nSchemeFlagShared;
                prepareSchemeFlagShared(vscp, vscpInfo);
                अवरोध;
            }
            हाल Scheme::FlagUnique: {
                ++m_nSchemeFlagUnique;
                prepareSchemeFlagUnique(vscp, vscpInfo);
                अवरोध;
            }
            हाल Scheme::मूल्य_Queue_Whole: {
                ++m_n_Scheme_मूल्य_Queues_Whole;
                prepare_Scheme_मूल्य_Queue</* Partial: */ false>(vscp, vscpInfo);
                अवरोध;
            }
            हाल Scheme::मूल्य_Queue_Partial: {
                ++m_n_Scheme_मूल्य_Queues_Partial;
                prepare_Scheme_मूल्य_Queue</* Partial: */ true>(vscp, vscpInfo);
                अवरोध;
            }
            }
        }
        // Convert all NBAs
        क्रम (स्थिर NBA& nba : m_nbas) {
            AstAssignDly* स्थिर nbap = nba.nodep;
            AstVarScope* स्थिर vscp = nba.vscp;
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            // Run 'convert' step
            चयन (vscpInfo.m_scheme) {
            हाल Scheme::Undecided: {  // LCOV_EXCL_START
                UASSERT_OBJ(false, vscp, "Unreachable");
                अवरोध;
            }  // LCOV_EXCL_STOP
            हाल Scheme::UnsupportedCompoundArrayInLoop: {
                // TODO: make this an E_UNSUPPORTED...
                nbap->v3warn(BLKLOOPINIT, "Unsupported: Non-blocking assignment to array with "
                                          "compound element type inside loop");
                अवरोध;
            }
            हाल Scheme::ShadowVar: {
                convertSchemeShadowVar(nbap, vscp, vscpInfo);
                अवरोध;
            }
            हाल Scheme::ShadowVarMasked: {
                convertSchemeShadowVarMasked(nbap, vscp, vscpInfo);
                अवरोध;
            }
            हाल Scheme::FlagShared: {
                convertSchemeFlagShared(nbap, vscp, vscpInfo);
                अवरोध;
            }
            हाल Scheme::FlagUnique: {
                convertSchemeFlagUnique(nbap, vscp, vscpInfo);
                अवरोध;
            }
            हाल Scheme::मूल्य_Queue_Whole: {
                convert_Scheme_मूल्य_Queue(nbap, vscp, vscpInfo, /* partial: */ false);
                अवरोध;
            }
            हाल Scheme::मूल्य_Queue_Partial:
                convert_Scheme_मूल्य_Queue(nbap, vscp, vscpInfo, /* partial: */ true);
                अवरोध;
            }
        }
    }
    व्योम visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstActive* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ignoreBlkAndNBlk);
        VL_RESTORER(m_inNonCombLogic);
        m_activep = nodep;
        स्थिर Ast_Sen_वृक्ष* स्थिर senTreep = nodep->sentreep();
        m_ignoreBlkAndNBlk = senTreep->hasStatic() || senTreep->hasInitial();
        m_inNonCombLogic = senTreep->hasClocked();
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_Procedure* nodep) override {
        VL_RESTORER(m_needsInitialTrigger);
        VL_RESTORER_CLEAR(m_nbaEventSenTreeps);
        स्थिर माप_प्रकार first_NBAAdded_सूचकांक = m_nbas.आकार();
        {
            VL_RESTORER(m_inSuspendableOrFork);
            VL_RESTORER(m_procp);
            VL_RESTORER(m_ignoreBlkAndNBlk);
            VL_RESTORER(m_inNonCombLogic);
            // When we are dealing with initial block we need to
            // treat it as suspendable when we meet a NBA
            m_inSuspendableOrFork = nodep->isSuspendable() || VN_IS(nodep, Initial);
            m_procp = nodep;
            अगर (nodep->isSuspendable()) {
                m_ignoreBlkAndNBlk = false;
                m_inNonCombLogic = true;
            }
            iterateChildren(nodep);
        }
        स्वतः containsClocled = [](स्थिर AstSenItem* itemp) {
            जबतक (itemp) {
                अगर (itemp->edgeType().clockedStmt()) वापस true;
                itemp = VN_AS(itemp->nextp(), SenItem);
            }
            वापस false;
        };
        स्थिर bool addInitialTrigger = m_needsInitialTrigger
                                       && !(isProcedureWithSentreep<AstAlways>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysObserved>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysReactive>(nodep))
                                       && !containsClocled(m_activep->sentreep()->sensesp());
        अगर (m_timingDomains.empty() && !addInitialTrigger) वापस;

        // There were some timing domains involved in the process. Add all of them as sensitivities
        // of all NBA targets in this process. Note this is a bit of a sledgehammer, we should only
        // need those that directly precede the NBA in control flow, but that is hard to compute,
        // so we will hammer away.

        // First gather all senItems
        AstSenItem* senItemp = nullptr;
        अगर (addInitialTrigger) {
            senItemp = नया AstSenItem{nodep->fileline(), AstSenItem::InitialNBA{}};
            ++m_nInitialNBA;
        }

        क्रम (स्थिर Ast_Sen_वृक्ष* स्थिर domainp : m_timingDomains) {
            अगर (domainp->sensesp())
                senItemp = Ast_गाँठ::add_अग्र(senItemp, domainp->sensesp()->clone_वृक्ष(true));
        }
        m_timingDomains.सफा();
        // Add them to all nba targets we gathered in this process
        क्रम (माप_प्रकार i = first_NBAAdded_सूचकांक; i < m_nbas.आकार(); ++i) {
            m_vscpInfo(m_nbas[i].vscp).addSensitivity(senItemp);
        }
        क्रम (Ast_Sen_वृक्ष* स्थिर senTreep : m_nbaEventSenTreeps)
            senTreep->addSensesp(senItemp->clone_वृक्ष(true));
        // Done with these
        VL_DO_DANGLING(senItemp->delete_वृक्ष(), senItemp);
    }
    व्योम visit(AstFork* nodep) override {
        VL_RESTORER(m_inSuspendableOrFork);
        m_inSuspendableOrFork = true;
        iterateChildren(nodep);
    }
    व्योम visit(AstCAwait* nodep) override {
        अगर (nodep->sentreep()) m_timingDomains.जोड़ो(nodep->sentreep());
        iterateChildren(nodep);
    }
    व्योम visit(AstFireEvent* nodep) override {
        UASSERT_OBJ(v3Global.hasEvents(), nodep, "Inconsistent");
        File_पंक्ति* स्थिर flp = nodep->fileline();

        Ast_गाँठ_व्यंजक* स्थिर eventp = nodep->operandp()->unlinkFrBack();

        // Enqueue for clearing 'triggered' state on next eval
        AstCStmt* स्थिर cstmtp = नया AstCStmt{flp};
        cstmtp->add("vlSymsp->fireEvent(");
        cstmtp->add(eventp);
        cstmtp->add(");");

        Ast_गाँठ* newp = cstmtp;
        स्थिर AstVarRef* स्थिर vrefp = VN_CAST(eventp, VarRef);
        अगर (nodep->isDelayed() && (m_cfuncp || !vrefp)) {
            // V3Timing converts these with --timing
            nodep->v3warn(E_NOTIMING, "Nonblocking event trigger "
                                          << (m_cfuncp ? "in a non-inlined function/task"
                                                       : "of a class or interface member or "
                                                         "array element")
                                          << " requires --timing");
        } अन्यथा अगर (nodep->isDelayed()) {
            स्थिर std::string newvarname = "_" + vrefp->varp()->shortName();
            AstVarScope* स्थिर dlyvscp
                = m_dlyTmps.make(flp, vrefp->varScopep()->scopep(), 1, newvarname);

            स्थिर स्वतः dlyRef = [=](VAccess access) {  //
                वापस नया AstVarRef{flp, dlyvscp, access};
            };

            AstAlwaysPost* स्थिर postp = नया AstAlwaysPost{flp};
            AstIf* स्थिर ifp = नया AstIf{flp, dlyRef(VAccess::READ)};
            postp->addStmtsp(ifp);

            UASSERT_OBJ(m_activep, nodep, "No active to handle FireEvent");
            Ast_Sen_वृक्ष* senTreep = m_activep->sentreep();
            AstActive* activep = nullptr;
            अगर (m_inSuspendableOrFork && (senTreep->hasInitial() || senTreep->hasClocked())) {
                // Suspendable code can trigger whenever it resumes, so fire when its NBAs are
                // committed (see visit(AstNodeProcedure*)), clearing the flag like
                // Scheme::FlagUnique
                senTreep = नया Ast_Sen_वृक्ष{
                    flp, senTreep->hasClocked() ? senTreep->sensesp()->clone_वृक्ष(true) : nullptr};
                m_nbaEventSenTreeps.धकेलो_back(senTreep);
                m_needsInitialTrigger |= m_timingDomains.empty();
                dlyvscp->varp()->setIgnorePostWrite();
                ifp->addThensp(नया AstAssign{flp, dlyRef(VAccess::WRITE),
                                             नया AstConst{flp, AstConst::BitFalse{}}});
                activep = नया AstActive{flp, "nba-event", senTreep};
                activep->sen_वृक्ष_Storep(senTreep);
            } अन्यथा {
                activep = नया AstActive{flp, "nba-event", senTreep};
                AstAlwaysPre* स्थिर prep = नया AstAlwaysPre{flp};
                prep->addStmtsp(नया AstAssign{flp, dlyRef(VAccess::WRITE),
                                              नया AstConst{flp, AstConst::BitFalse{}}});
                activep->addStmtsp(prep);
            }
            ifp->addThensp(newp);
            m_activep->add_अग्र_Here(activep);
            activep->addStmtsp(postp);

            newp = नया AstAssign{flp, dlyRef(VAccess::WRITE),
                                 नया AstConst{flp, AstConst::BitTrue{}}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    व्योम visit(AstAssignDly* nodep) override {
        // Prevent double processing due to AstExprStmt being moved before this node
        अगर (nodep->user1SetOnce()) वापस;

        अगर (m_cfuncp) {
            अगर (!v3Global.rootp()->nbaEventp()) {
                nodep->v3warn(
                    E_NOTIMING,
                    "Delayed assignment in a non-inlined function/task requires --timing");
            }
            वापस;
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
        nodep->lhsp()->foreach([&](Ast_गाँठ* currp) {
            // cppcheck-suppress constVariablePointer
            अगर (Ast_व्यंजक_Stmt* स्थिर exprp = VN_CAST(currp, व्यंजक_Stmt)) {
                // Move statements before the NBA
                nodep->add_Here_This_As_अग्र(exprp->stmtsp()->unlink_Fr_Back_With_अग्र());
                // Replace with result
                currp->replaceWith(exprp->resultp()->unlinkFrBack());
                // Get rid of the AstExprStmt
                VL_DO_DANGLING2(धकेलो_Deletep(currp), currp, exprp);
            } अन्यथा अगर (AstVarRef* स्थिर refp = VN_CAST(currp, VarRef)) {
                // Ignore reads (e.g.: '_[*here*] <= _')
                अगर (refp->access().isReadOnly()) वापस;
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
        AstVarScope* स्थिर vscp = m_currNbaLhsRefp->varScopep();
        // Record it on first encounter
        VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
        अगर (!vscpInfo.m_firstNbaRefp) {
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
        अगर (m_activep->sentreep()->hasClocked()) {
            अगर (vscpInfo.m_fistActivep != m_activep) {
                AstVar* स्थिर varp = vscp->varp();
                अगर (!varp->user1SetOnce()
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
        recordWriteRef(m_currNbaLhsRefp, true);

        iterateChildren(nodep);
    }
    व्योम visit(AstVarRef* nodep) override {
        // Already checked the NBA LHS ref, ignore here
        अगर (nodep == m_currNbaLhsRefp) वापस;
        // Only care about write refs
        अगर (!nodep->access().isWriteOrRW()) वापस;
        // Record write reference
        recordWriteRef(nodep, false);
    }
    व्योम visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = true;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_CCall* स्थिर nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        स्थिर स्वतः& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        अगर (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } अन्यथा {
            क्रम (AstCFunc* स्थिर cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    व्योम visit(AstCFunc* स्थिर nodep) override {
        स्थिर स्वतः& मूल्य = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        अगर (मूल्य.m_state != CFuncCache::UNINITIALIZED) वापस;
        VL_RESTORER(m_cfuncp);
        // Its timing domains are those of the processes calling it, see visitCalledCFunc()
        VL_RESTORER_CLEAR(m_timingDomains);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }

    // Pre/Post logic are created here and their content need no further changes, so ignore.
    व्योम visit(AstAlwaysPre*) override {}
    व्योम visit(AstAlwaysPost*) override {}

    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
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

व्योम V3Delayed::delayedAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DelayedVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("delayed", 0, dump_वृक्ष_Either_Level() >= 3);
}
