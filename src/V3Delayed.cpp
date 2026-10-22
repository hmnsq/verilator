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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Delayed.h"

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3ClassGraph.h"
#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3LinkLValue.h"
#അന്തര്ഭാവ "V3SharedTmps.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <deque>
#അന്തര്ഭാവ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert AstAssignDlys (NBAs)

വർഗ്ഗം DelayedVisitor final : പൊതു VNVisitor {
    // TYPES

    // The various NBA conversion schemes, including error cases
    എണ്ണൽ വർഗ്ഗം Scheme : uint8_t {
        Undecided = 0,
        UnsupportedCompoundArrayInLoop,
        ShadowVar,
        ShadowVarMasked,
        FlagShared,
        FlagUnique,
        മൂല്യം_Queue_Whole,
        മൂല്യം_Queue_Partial
    };

    // All info associated with a variable that is the target of an NBA
    വർഗ്ഗം VarScopeInfo final {
    പൊതു:
        // First write reference encountered to the VarScope as the target on an NBA
        സ്ഥിരം AstVarRef* m_firstNbaRefp = nullptr;
        // Active block 'm_firstNbaRefp' is under
        സ്ഥിരം AstActive* m_fistActivep = nullptr;
        തർക്കം m_whole = അസത്യം;  // Used on LHS of NBA directly via VarRef
        തർക്കം m_partial = അസത്യം;  // Used on LHS of NBA under a Sel
        തർക്കം m_inLoop = അസത്യം;  // Used on LHS of NBA in a loop
        തർക്കം m_inSuspOrFork = അസത്യം;  // Used on LHS of NBA in suspendable process or fork
        Scheme m_scheme = Scheme::Undecided;  // Conversion scheme to use for this variable

    സ്വകാര്യ:
        // Combined sensitivities of all NBAs targeting this variable
        Ast_Sen_വൃക്ഷം* m_senTreep = nullptr;

        // Union of stuff needed for the various schemes - use accessors below!
        സംഘം {
            ഘടന {  // Stuff needed for Scheme::ShadowVar
                AstVarScope* vscp;  // The shadow variable
            } m_shadowVariableKit;
            ഘടന {  // Stuff needed for Scheme::ShadowVarMasked
                AstVarScope* vscp;  // The shadow variable
                AstVarScope* maskp;  // The mask variable
            } m_shadowVarMaskedKit;
            ഘടന {  // Stuff needed for Scheme::FlagShared
                AstActive* activep;  // The active block for the Pre/Post logic
                AstAlwaysPost* postp;  // The post block for commiting results
                AstVarScope* commitFlagp;  // The commit flag variable, for reuse
                AstIf* commitIfp;  // The previous if statement for committing, for reuse
            } m_flagSharedKit;
            ഘടന {  // Stuff needed for Scheme::FlagUnique
                AstAlwaysPost* postp;  // The post block for commiting results
            } m_flagUniqueKit;
            ഘടന {  // Stuff needed for Scheme::ValueQueueWhole/Scheme::ValueQueuePartial
                AstVarScope* vscp;  // The commit queue variable
            } m_മൂല്യം_Queue_Kit;
        } m_kitUnion;

    പൊതു:
        VarScopeInfo() = സ്ഥിരസ്ഥിതി;
        ~VarScopeInfo() {
            // Might not be linked if there was an error
            എങ്കിൽ (!m_senTreep->backp()) VL_DO_DANGLING(m_senTreep->delete_വൃക്ഷം(), m_senTreep);
        }
        // Accessors for the above union fields
        auto& shadowVariableKit() {
            UASSERT(m_scheme == Scheme::ShadowVar, "Inconsistent Scheme");
            തിരികെ m_kitUnion.m_shadowVariableKit;
        }
        auto& shadowVarMaskedKit() {
            UASSERT(m_scheme == Scheme::ShadowVarMasked, "Inconsistent Scheme");
            തിരികെ m_kitUnion.m_shadowVarMaskedKit;
        }
        auto& flagSharedKit() {
            UASSERT(m_scheme == Scheme::FlagShared, "Inconsistent Scheme");
            തിരികെ m_kitUnion.m_flagSharedKit;
        }
        auto& flagUniqueKit() {
            UASSERT(m_scheme == Scheme::FlagUnique, "Inconsistent Scheme");
            തിരികെ m_kitUnion.m_flagUniqueKit;
        }
        auto& മൂല്യം_Queue_Kit() {
            UASSERT(m_scheme == Scheme::മൂല്യം_Queue_Partial || m_scheme == Scheme::മൂല്യം_Queue_Whole,
                    "Inconsistent Scheme");
            തിരികെ m_kitUnion.m_മൂല്യം_Queue_Kit;
        }

        // Accessor
        Ast_Sen_വൃക്ഷം* senTreep() സ്ഥിരം { തിരികെ m_senTreep; }

        // Add sensitivities
        ശൂന്യം addSensitivity(AstSenItem* nodep) {
            എങ്കിൽ (!m_senTreep) m_senTreep = പുതിയ Ast_Sen_വൃക്ഷം{nodep->fileline(), nullptr};
            // Add a copy of each term
            m_senTreep->addSensesp(nodep->clone_വൃക്ഷം(സത്യം));
            // Remove duplicates
            V3Const::constifyExpensiveEdit(m_senTreep);
        }
        // cppcheck-suppress constParameterPointer
        ശൂന്യം addSensitivity(Ast_Sen_വൃക്ഷം* nodep) { addSensitivity(nodep->sensesp()); }
    };

    // Data structure to keep track of all writes to
    ഘടന WriteReference final {
        AstVarRef* m_refp = nullptr;  // The reference
        തർക്കം m_isNBA = അസത്യം;  // True if an NBA write
        തർക്കം m_inNonComb = അസത്യം;  // True if reference is known to be in non-combinational logic
        WriteReference() = സ്ഥിരസ്ഥിതി;
        WriteReference(AstVarRef* refp, തർക്കം isNBA, തർക്കം inNonComb)
            : m_refp{refp}
            , m_isNBA{isNBA}
            , m_inNonComb{inNonComb} {}
    };

    // Data required to lower AstAssignDelay later
    ഘടന NBA final {
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
    സ്ഥിരം VNUser1InUse m_user1InUse;
    സ്ഥിരം VNUser2InUse m_user2InUse;
    സ്ഥിരം VNUser3InUse m_user3InUse;

    ഘടന CFuncCache final {
        VInsertionSet<Ast_Sen_വൃക്ഷം*> m_timingDomains;  // What shall be added to m_timingDomains
        std::set<AstCFunc*>
            m_includes;  // CFuncs whose CFuncCache shall be included into this - this is used to
                         // break cycles: A->B->A (instead of visiting A while it is still begin
                         // visited B just marks that it includes A)
        എണ്ണൽ State : uint8_t {
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
    VInsertionSet<Ast_Sen_വൃക്ഷം*> m_timingDomains;  // Timing resume domains
    V3SharedTmps m_dlyTmps{"__Vdly", VVarType::BLOCKTEMP};  // Temporary variables
    // Commit queue data types, by element type and partial flag. Shared by all instances,
    // as m_dlyTmps only shares variables with the same data type.
    std::map<std::pair<സ്ഥിരം Ast_കണ്ണി_DType*, തർക്കം>, AstNBACommitQueueDType*> m_cqDTypeps;

    സ്ഥിരം std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::vector<സ്ഥിരം AstCFunc*> m_call_അട്ടി;  // Current callstack of AstCFuncs

    // STATE - for current visit position (use VL_RESTORER)
    AstActive* m_activep = nullptr;  // Current activate
    സ്ഥിരം AstCFunc* m_cfuncp = nullptr;  // Current public C Function
    Ast_കണ്ണി_Procedure* m_procp = nullptr;  // Current process
    AstScope* m_scopep = nullptr;  // Current scope
    തർക്കം m_inLoop = അസത്യം;  // True in for loops
    തർക്കം m_inSuspendableOrFork = അസത്യം;  // True in suspendable processes and forks
    തർക്കം m_ignoreBlkAndNBlk = അസത്യം;  // Suppress delayed assignment BLKANDNBLK
    തർക്കം m_inNonCombLogic = അസത്യം;  // We are in non-combinational logic
    തർക്കം m_needsInitialTrigger = അസത്യം;  // Whether a NodeProcedure needs a initial trigger
    std::vector<Ast_Sen_വൃക്ഷം*> m_nbaEventSenTreeps;  // Sensitivities of '->>' in the process
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
    VDouble0 m_n_Scheme_മൂല്യം_Queues_Whole;  //  Number of variables using Scheme::ValueQueueWhole
    VDouble0 m_n_Scheme_മൂല്യം_Queues_Partial;  //  Number of variables using Scheme::ValueQueuePartial
    VDouble0 m_nSharedSetFlags;  // "Set" flags actually shared by Scheme::FlagShared variables
    VDouble0 m_nInitialNBA;  // Number of procedural blocks with initial NBA
    VDouble0 m_non_Inlined_CAwaits_With_Sen_വൃക്ഷം;  // Count uses of not inlined co_awaits

    // METHODS

    // Return true iff a variable is assigned by both blocking and nonblocking
    // assignments. Issue BLKANDNBLK error if we can't prove the mixed
    // assignments are to independent bits and the blocking assignment can be
    // in combinational logic, which is something we can't safely implement
    // still.
    തർക്കം checkMixedUsage(സ്ഥിരം AstVarScope* vscp, തർക്കം isIntegralOrPacked) {

        ഘടന Ref final {
            AstVarRef* m_refp;  // The reference
            തർക്കം m_inNonComb;  // True if known to be in non-combinational logic
            പൂർണ്ണസംഖ്യ m_lsb;  // LSB of accessed range
            പൂർണ്ണസംഖ്യ m_msb;  // MSB of accessed range
            Ref(AstVarRef* refp, തർക്കം inNonComb, പൂർണ്ണസംഖ്യ lsb, പൂർണ്ണസംഖ്യ msb)
                : m_refp{refp}
                , m_inNonComb{inNonComb}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };

        std::vector<Ref> blkRefs;  // Blocking writes
        std::vector<Ref> nbaRefs;  // Non-blockign writes

        സ്ഥിരം പൂർണ്ണസംഖ്യ width = isIntegralOrPacked ? vscp->width() : 1;

        വേണ്ടി (സ്ഥിരം auto& writeRef : m_writeRefs(vscp)) {
            പൂർണ്ണസംഖ്യ lsb = 0;
            പൂർണ്ണസംഖ്യ msb = width - 1;
            എങ്കിൽ (സ്ഥിരം AstSel* സ്ഥിരം selp = VN_CAST(writeRef.m_refp->backp(), Sel)) {
                എങ്കിൽ (VN_IS(selp->lsbp(), Const)) {
                    lsb = selp->lsbConst();
                    msb = selp->msbConst();
                }
            }
            എങ്കിൽ (writeRef.m_isNBA) {
                nbaRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            } അല്ലെങ്കിൽ {
                blkRefs.emplace_back(writeRef.m_refp, writeRef.m_inNonComb, lsb, msb);
            }
        }
        // We only run this function on targets of NBAs, so there should be at least one...
        UASSERT_OBJ(!nbaRefs.empty(), vscp, "Did not record NBA write");
        // If no blocking upadte, then we are good
        എങ്കിൽ (blkRefs.empty()) തിരികെ അസത്യം;

        // If the blocking assignment is in non-combinational logic (i.e.:
        // in logic that has an explicit trigger), then we can safely
        // implement it (there is no race between clocked logic and post
        // scheduled logic), so need not error
        blkRefs.erase(std::remove_if(blkRefs.begin(), blkRefs.end(),
                                     [](സ്ഥിരം Ref& ref) { തിരികെ ref.m_inNonComb; }),
                      blkRefs.end());

        // If nothing left, then we need not error
        എങ്കിൽ (blkRefs.empty()) തിരികെ സത്യം;

        // If not a packed variable, warn here as we can't prove independence
        എങ്കിൽ (!isIntegralOrPacked) {
            സ്ഥിരം Ref& blkRef = blkRefs.front();
            സ്ഥിരം Ref& nbaRef = nbaRefs.front();
            vscp->v3warn(
                BLKANDNBLK,
                "Unsupported: Blocking and non-blocking assignments to same non-packed variable: "
                    << vscp->varp()->prettyNameQ() << '\n'
                    << vscp->warnContextPrimary() << '\n'
                    << blkRef.m_refp->warnOther() << "... Location of blocking assignment\n"
                    << blkRef.m_refp->warnContextSecondary() << '\n'
                    << nbaRef.m_refp->warnOther() << "... Location of nonblocking assignment\n"
                    << nbaRef.m_refp->warnContextSecondary());
            തിരികെ സത്യം;
        }

        // We need to error if we can't prove the written bits are independent

        // Sort refs by interval
        സ്ഥിരം auto lessThanRef = [](സ്ഥിരം Ref& a, സ്ഥിരം Ref& b) {
            എങ്കിൽ (a.m_lsb != b.m_lsb) തിരികെ a.m_lsb < b.m_lsb;
            തിരികെ a.m_msb < b.m_msb;
        };
        std::stable_sort(blkRefs.begin(), blkRefs.end(), lessThanRef);
        std::stable_sort(nbaRefs.begin(), nbaRefs.end(), lessThanRef);
        // Iterate both vectors, checking for overlap
        auto bIt = blkRefs.begin();
        auto nIt = nbaRefs.begin();
        ആകുമ്പോൾ (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            എങ്കിൽ (lessThanRef(*bIt, *nIt)) {
                എങ്കിൽ (nIt->m_lsb <= bIt->m_msb) നിർത്തുക;  // Stop on Overlap
                ++bIt;
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (bIt->m_lsb <= nIt->m_msb) നിർത്തുക;  // Stop on Overlap
                ++nIt;
            }
        }

        // If we found an overlapping range that cannot be safely implemented, then wran...
        എങ്കിൽ (bIt != blkRefs.end() && nIt != nbaRefs.end()) {
            സ്ഥിരം Ref& blkRef = *bIt;
            സ്ഥിരം Ref& nbaRef = *nIt;
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

        തിരികെ സത്യം;
    }

    // Choose the NBA scheme used for the given variable.
    Scheme chooseScheme(സ്ഥിരം AstVarScope* vscp, സ്ഥിരം VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::Undecided, vscp, "NBA scheme already decided");

        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = vscp->dtypep()->skipRefp();
        // Unpacked arrays
        എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം uaDTypep = VN_CAST(dtypep, UnpackArrayDType)) {
            // If whole array is target of NBA, use ShadowVar, but not in a suspendable process or
            // fork, which resume outside the 'nba' region, so its 'pre' logic would overwrite the
            // value they write in the shadow variable
            എങ്കിൽ (vscpInfo.m_whole) {
                തിരികെ vscpInfo.m_inSuspOrFork ? Scheme::FlagUnique : Scheme::ShadowVar;
            }
            // Basic underlying type of elements, if any.
            സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = uaDTypep->basicp();
            // If used in a loop, we must have a dynamic commit queue. (Also works in suspendables)
            എങ്കിൽ (vscpInfo.m_inLoop) {
                // Arrays with compound element types are currently not supported in loops
                എങ്കിൽ (!basicp
                    || !(basicp->isIntegralOrPacked()  //
                         || basicp->isDouble()  //
                         || basicp->isString())) {
                    തിരികെ Scheme::UnsupportedCompoundArrayInLoop;
                }
                എങ്കിൽ (vscpInfo.m_partial) തിരികെ Scheme::മൂല്യം_Queue_Partial;
                തിരികെ Scheme::മൂല്യം_Queue_Whole;
            }
            // A suspendable process or fork can resume outside the 'nba' region, so the 'pre'
            // logic of the other schemes would undo its updates. The unique flag scheme has none.
            എങ്കിൽ (vscpInfo.m_inSuspOrFork) തിരികെ Scheme::FlagUnique;
            // Otherwise if an array of packed/basic elements, use the shared flag scheme
            എങ്കിൽ (basicp) തിരികെ Scheme::FlagShared;
            // Finally fall back on the shadow variable scheme, e.g. for
            // arrays of unpacked structs. This will be slow.
            // TODO: generic LHS scheme as discussed in #5092
            തിരികെ Scheme::ShadowVar;
        }

        // Likewise, the 'pre' logic of the shadow variable scheme would undo the updates of a
        // suspendable process or fork, so use the unique flag scheme
        എങ്കിൽ (vscpInfo.m_inSuspOrFork) തിരികെ Scheme::FlagUnique;

        സ്ഥിരം തർക്കം isIntegralOrPacked = dtypep->isIntegralOrPacked();
        // Check for mixed usage (this also warns if not OK)
        എങ്കിൽ (checkMixedUsage(vscp, isIntegralOrPacked)) {
            // If it's a variable updated by both blocking and non-blocking
            // assignments, use the ShadowVarMasked schem if masked update is
            // possible. This can handle blocking and non-blocking updates to
            // inpdendent parts correctly at run-time, and always works, even
            // in loops or other dynamic context.
            എങ്കിൽ (isIntegralOrPacked) തിരികെ Scheme::ShadowVarMasked;
            // If it's inside a loop, use Scheme::ShadowVar, which is safe,
            // but will generate incorrect code if a partial update is used
            എങ്കിൽ (vscpInfo.m_inLoop) തിരികെ Scheme::ShadowVar;
            // Otherwise (for not packed variables), use the FlagUnique scheme,
            // which at least handles partial updates correctly, but might break
            // in loops or other dynamic context
            തിരികെ Scheme::FlagUnique;
        }

        // Otherwise use the simple shadow variable scheme
        തിരികെ Scheme::ShadowVar;
    }

    // Given an expression 'exprp', return a new expression that always evaluates to the
    // value of the given expression at this point in the program. That is:
    // - If given a non-constant expression, create a new temporary AstVarScope under the given
    //   'scopep', with the given 'name', assign the expression to it, and return a read reference
    //   to the new AstVarScope.
    // - If given a constant, just return that constant.
    // New statements are inserted before 'insertp'
    Ast_കണ്ണി_വ്യഞ്ജകം* captureVal(AstScope* സ്ഥിരം scopep, Ast_കണ്ണി_Stmt* സ്ഥിരം insertp,
                            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp, സ്ഥിരം std::string& name) {
        UASSERT_OBJ(!exprp->backp(), exprp, "Should have been unlinked");
        File_വരി* സ്ഥിരം flp = exprp->fileline();
        എങ്കിൽ (VN_IS(exprp, Const)) തിരികെ exprp;
        // TODO: there are some const variables that could be simply referenced here
        AstVarScope* സ്ഥിരം tmpVscp = m_dlyTmps.make(flp, scopep, exprp->dtypep(), name);
        insertp->addHereThisAsNext(
            പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, tmpVscp, VAccess::WRITE}, exprp});
        തിരികെ പുതിയ AstVarRef{flp, tmpVscp, VAccess::READ};
    };

    // Similar to 'captureVal', but captures an LValue expression. That is, the returned
    // expression will reference the same location as the input expression, at this point in the
    // program.
    Ast_കണ്ണി_വ്യഞ്ജകം* captureLhs(AstScope* സ്ഥിരം scopep, Ast_കണ്ണി_Stmt* സ്ഥിരം insertp,
                            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp, സ്ഥിരം std::string& baseName) {
        UASSERT_OBJ(!lhsp->backp(), lhsp, "Should have been unlinked");
        // Running node pointer
        Ast_കണ്ണി_വ്യഞ്ജകം* nodep = lhsp;
        // Capture AstSel indices - there should be only one
        എങ്കിൽ (AstSel* സ്ഥിരം selp = VN_CAST(nodep, Sel)) {
            സ്ഥിരം std::string tmpName{"Lsb" + baseName};
            selp->lsbp(captureVal(scopep, insertp, selp->lsbp()->unlinkFrBack(), tmpName));
            // Continue with target
            nodep = selp->fromp();
        }
        UASSERT_OBJ(!VN_IS(nodep, Sel), lhsp, "Multiple 'AstSel' applied to LHS reference");
        // Capture the indices of selects - might be many, also below struct members. If a handle
        // selects the instance the target is a member of, capture the handle instead, as it is
        // only read, to select the target when the NBA is executed (IEEE 1800-2023 10.4.2).
        size_t nArraySels = 0;
        ആകുമ്പോൾ (സത്യം) {
            എങ്കിൽ (Ast_കണ്ണി_Sel* സ്ഥിരം selp = VN_CAST(nodep, കണ്ണി_Sel)) {
                സ്ഥിരം std::string tmpName{"Dim" + std::to_string(nArraySels++) + baseName};
                selp->bitp(captureVal(scopep, insertp, selp->bitp()->unlinkFrBack(), tmpName));
                nodep = selp->fromp();
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstStructSel* സ്ഥിരം selp = VN_CAST(nodep, StructSel)) {
                nodep = selp->fromp();
            } അല്ലെങ്കിൽ എങ്കിൽ (AstMemberSel* സ്ഥിരം selp = VN_CAST(nodep, MemberSel)) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം handlep = selp->fromp()->unlinkFrBack();
                V3LinkLValue::linkLValueSet(handlep, VAccess::READ);
                AstVarRef* സ്ഥിരം refp
                    = VN_AS(captureVal(scopep, insertp, handlep, "Handle" + baseName), VarRef);
                refp->varp()->setIgnorePostWrite();
                // Like the handle it replaces, mark it written, as is the member it selects
                refp->access(selp->access());
                selp->fromp(refp);
                തിരികെ lhsp;
            } അല്ലെങ്കിൽ {
                നിർത്തുക;
            }
        }
        // What remains must be the variable, or some sort of select, we assume can reuse it
        UASSERT_OBJ(nodep->isPure(), lhsp, "Malformed LHS in NBA");
        // Now have been converted to use the captured values
        തിരികെ lhsp;
    }

    ശൂന്യം add_CFunc_Cached_മൂല്യങ്ങൾ(സ്ഥിരം AstCFunc* സ്ഥിരം cfuncp,
                              std::unordered_set<സ്ഥിരം AstCFunc*>& visited) {
        എങ്കിൽ (!visited.തിരുകുക(cfuncp).second) തിരികെ;
        CFuncCache& മൂല്യം = m_cfuncsCache(cfuncp);
        m_timingDomains.തിരുകുക(മൂല്യം.m_timingDomains.begin(), മൂല്യം.m_timingDomains.end());
        m_non_Inlined_CAwaits_With_Sen_വൃക്ഷം += മൂല്യം.m_timingDomains.size();
        വേണ്ടി (സ്ഥിരം AstCFunc* സ്ഥിരം includedp : മൂല്യം.m_includes) {
            add_CFunc_Cached_മൂല്യങ്ങൾ(includedp, visited);
        }
    }

    // Create a temporary variable in the given 'scopep', with the given 'name', and with 'dtypep'
    // type, with the bits selected by 'sLsbp'/'sWidthp' set to 'valuep', other bits set to zero.
    // Insert new statements before 'insertp'.
    // Returns a read reference to the temporary variable.
    AstVarRef* createWidened(File_വരി* flp, AstScope* scopep, Ast_കണ്ണി_DType* dtypep,
                             Ast_കണ്ണി_വ്യഞ്ജകം* sLsbp, പൂർണ്ണസംഖ്യ sWidth, സ്ഥിരം std::string& name,
                             Ast_കണ്ണി_വ്യഞ്ജകം* valuep, Ast_കണ്ണി* insertp) {
        // Create temporary variable.
        AstVarScope* സ്ഥിരം tp = m_dlyTmps.make(flp, scopep, dtypep, name);
        // Zero it
        AstConst* സ്ഥിരം zerop = പുതിയ AstConst{flp, AstConst::DTyped{}, dtypep};
        zerop->അക്കം().setAllBits0();
        insertp->addHereThisAsNext(
            പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, tp, VAccess::WRITE}, zerop});
        // Set the selected bits to 'valuep'
        AstSel* സ്ഥിരം selp = പുതിയ AstSel{flp, പുതിയ AstVarRef{flp, tp, VAccess::WRITE},
                                        sLsbp->clone_വൃക്ഷം_Pure(സത്യം), sWidth};
        insertp->addHereThisAsNext(പുതിയ AstAssign{flp, selp, valuep});
        // This is the expression to get the value of the temporary
        തിരികെ പുതിയ AstVarRef{flp, tp, VAccess::READ};
    }

    // Scheme::ShadowVar
    ശൂന്യം prepareSchemeShadowVar(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        File_വരി* സ്ഥിരം flp = vscp->fileline();
        AstScope* സ്ഥിരം scopep = vscp->scopep();
        // Create the shadow variable
        സ്ഥിരം std::string name = "_" + vscp->varp()->shortName();
        AstVarScope* സ്ഥിരം shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), name);
        vscpInfo.shadowVariableKit().vscp = shadowVscp;
        // Mark both for V3LifePsot
        vscp->optimizeLifePost(സത്യം);
        shadowVscp->optimizeLifePost(സത്യം);
        // Create the AstActive for the Pre/Post logic
        AstActive* സ്ഥിരം activep = പുതിയ AstActive{flp, "nba-shadow-variable", vscpInfo.senTreep()};
        activep->sen_വൃക്ഷം_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Pre' scheduled 'shadowVariable = originalVariable' assignment
        AstAlwaysPre* സ്ഥിരം prep = പുതിയ AstAlwaysPre{flp};
        activep->addStmtsp(prep);
        prep->addStmtsp(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, shadowVscp, VAccess::WRITE},
                                      പുതിയ AstVarRef{flp, vscp, VAccess::READ}});
        // Add 'Post' scheduled 'originalVariable = shadowVariable' assignment
        AstAlwaysPost* സ്ഥിരം postp = പുതിയ AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        postp->addStmtsp(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, vscp, VAccess::WRITE},
                                       പുതിയ AstVarRef{flp, shadowVscp, VAccess::READ}});
    }
    ശൂന്യം convertSchemeShadowVar(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        AstVarScope* സ്ഥിരം shadowVscp = vscpInfo.shadowVariableKit().vscp;

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* സ്ഥിരം refp) {
            എങ്കിൽ (!refp->access().isWriteOnly()) തിരികെ;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
    }

    // Scheme::ShadowVarMasked
    ശൂന്യം prepareSchemeShadowVarMasked(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        File_വരി* സ്ഥിരം flp = vscp->fileline();
        AstScope* സ്ഥിരം scopep = vscp->scopep();
        // Create the shadow variable
        സ്ഥിരം std::string shadowName = "_" + vscp->varp()->shortName();
        AstVarScope* സ്ഥിരം shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), shadowName);
        vscpInfo.shadowVarMaskedKit().vscp = shadowVscp;
        // Create the makk variable
        സ്ഥിരം std::string maskName = "Mask__" + vscp->varp()->shortName();
        AstVarScope* സ്ഥിരം maskVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), maskName);
        maskVscp->varp()->setIgnorePostWrite();
        vscpInfo.shadowVarMaskedKit().maskp = maskVscp;
        // Create the AstActive for the Post logic
        AstActive* സ്ഥിരം activep
            = പുതിയ AstActive{flp, "nba-shadow-var-masked", vscpInfo.senTreep()};
        activep->sen_വൃക്ഷം_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit and mask clear
        AstAlwaysPost* സ്ഥിരം postp = പുതിയ AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit - vscp = (shadowVscp & maskVscp) | (vscp & ~maskVscp);
        postp->addStmtsp(പുതിയ AstAssign{
            flp, പുതിയ AstVarRef{flp, vscp, VAccess::WRITE},
            പുതിയ AstOr{flp,
                      പുതിയ AstAnd{flp, പുതിയ AstVarRef{flp, shadowVscp, VAccess::READ},
                                 പുതിയ AstVarRef{flp, maskVscp, VAccess::READ}},
                      പുതിയ AstAnd{flp, പുതിയ AstVarRef{flp, vscp, VAccess::READ},
                                 പുതിയ AstNot{flp, പുതിയ AstVarRef{flp, maskVscp, VAccess::READ}}}}});
        vscp->varp()->setIgnorePostRead();
        // Clar the mask - maskVscp = '0;
        postp->addStmtsp(
            പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, maskVscp, VAccess::WRITE},
                          പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, maskVscp->width(), 0}});
    }
    ശൂന്യം convertSchemeShadowVarMasked(AstAssignDly* nodep, AstVarScope* vscp,
                                      VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        AstVarScope* സ്ഥിരം shadowVscp = vscpInfo.shadowVarMaskedKit().vscp;
        AstVarScope* സ്ഥിരം maskVscp = vscpInfo.shadowVarMaskedKit().maskp;

        Ast_കണ്ണി_വ്യഞ്ജകം* lhsClonep = nodep->lhsp()->clone_വൃക്ഷം(അസത്യം);

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* സ്ഥിരം refp) {
            എങ്കിൽ (!refp->access().isWriteOnly()) തിരികെ;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
        // Set the same bits in the mask to 1
        lhsClonep->foreach([&](AstVarRef* സ്ഥിരം refp) {
            എങ്കിൽ (!refp->access().isWriteOnly()) തിരികെ;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(maskVscp);
            refp->varp(maskVscp->varp());
        });
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        AstConst* സ്ഥിരം onesp = പുതിയ AstConst{flp, AstConst::DTyped{}, lhsClonep->dtypep()};
        onesp->അക്കം().setAllBits1();
        nodep->addNextHere(പുതിയ AstAssign{flp, lhsClonep, onesp});
    }

    // Scheme::FlagShared
    ശൂന്യം prepareSchemeFlagShared(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_വരി* സ്ഥിരം flp = vscp->fileline();
        AstScope* സ്ഥിരം scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* സ്ഥിരം activep = പുതിയ AstActive{flp, "nba-flag-shared", vscpInfo.senTreep()};
        activep->sen_വൃക്ഷം_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        vscpInfo.flagSharedKit().activep = activep;
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* സ്ഥിരം postp = പുതിയ AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagSharedKit().postp = postp;
        // Initialize
        vscpInfo.flagSharedKit().commitFlagp = nullptr;
        vscpInfo.flagSharedKit().commitIfp = nullptr;
    }
    ശൂന്യം convertSchemeFlagShared(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_വരി* സ്ഥിരം flp = vscp->fileline();
        AstScope* സ്ഥിരം scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        സ്ഥിരം std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Is this NBA adjacent after the previously processed NBA?
        സ്ഥിരം തർക്കം consecutive = nodep == m_nextDlyp;
        m_nextDlyp = VN_CAST(nodep->nextp(), AssignDly);

        VarScopeInfo* സ്ഥിരം prevVscpInfop = consecutive ? &m_vscpInfo(m_prevVscp) : nullptr;

        // We can reuse the flag of the previous assignment if:
        സ്ഥിരം തർക്കം reuseTheFlag =
            // Consecutive NBAs
            consecutive
            // ... that use the same scheme
            && prevVscpInfop->m_scheme == Scheme::FlagShared
            // ... and are in the same scope as the target variable
            && scopep == vscp->scopep()
            // ... and share the same overall update domain
            && prevVscpInfop->senTreep()->same_വൃക്ഷം(vscpInfo.senTreep());

        എങ്കിൽ (!reuseTheFlag) {
            // Create new flag
            AstVarScope* സ്ഥിരം flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
            // Set the flag at the original NBA
            nodep->addHereThisAsNext(  //
                പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, flagVscp, VAccess::WRITE},
                              പുതിയ AstConst{flp, AstConst::BitTrue{}}});
            // Add the 'Pre' scheduled reset for the flag
            AstAlwaysPre* സ്ഥിരം prep = പുതിയ AstAlwaysPre{flp};
            vscpInfo.flagSharedKit().activep->addStmtsp(prep);
            prep->addStmtsp(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, flagVscp, VAccess::WRITE},
                                          പുതിയ AstConst{flp, AstConst::BitFalse{}}});
            // Add the 'Post' scheduled commit
            AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, പുതിയ AstVarRef{flp, flagVscp, VAccess::READ}};
            vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
            vscpInfo.flagSharedKit().commitFlagp = flagVscp;
            vscpInfo.flagSharedKit().commitIfp = ifp;
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (vscp != m_prevVscp) {
                // Different variable, ensure the commit block exists for this variable,
                // can reuse existing one with the same flag, otherwise create a new one.
                AstVarScope* സ്ഥിരം flagVscp = prevVscpInfop->flagSharedKit().commitFlagp;
                UASSERT_OBJ(flagVscp, nodep, "Commit flag of previous assignment should exist");
                എങ്കിൽ (vscpInfo.flagSharedKit().commitFlagp != flagVscp) {
                    AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, പുതിയ AstVarRef{flp, flagVscp, VAccess::READ}};
                    vscpInfo.flagSharedKit().postp->addStmtsp(ifp);
                    vscpInfo.flagSharedKit().commitFlagp = flagVscp;
                    vscpInfo.flagSharedKit().commitIfp = ifp;
                }
            } അല്ലെങ്കിൽ {
                // Same variable, reuse the commit block of the previous assignment
                vscpInfo.flagSharedKit().commitFlagp = prevVscpInfop->flagSharedKit().commitFlagp;
                vscpInfo.flagSharedKit().commitIfp = prevVscpInfop->flagSharedKit().commitIfp;
            }
            ++m_nSharedSetFlags;
        }
        // Commit the captured value to the captured destination
        vscpInfo.flagSharedKit().commitIfp->addThensp(
            പുതിയ AstAssign{flp, capturedLhsp, capturedRhsp});

        // Remember the variable for the next NBA
        m_prevVscp = vscp;

        // Delete original NBA
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::FlagUnique
    ശൂന്യം prepareSchemeFlagUnique(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_വരി* സ്ഥിരം flp = vscp->fileline();
        AstScope* സ്ഥിരം scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        AstActive* സ്ഥിരം activep = പുതിയ AstActive{flp, "nba-flag-unique", vscpInfo.senTreep()};
        activep->sen_വൃക്ഷം_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* സ്ഥിരം postp = പുതിയ AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        vscpInfo.flagUniqueKit().postp = postp;
    }
    ശൂന്യം convertSchemeFlagUnique(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_വരി* സ്ഥിരം flp = vscp->fileline();
        AstScope* സ്ഥിരം scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        സ്ഥിരം std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // Create new flag
        AstVarScope* സ്ഥിരം flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + baseName);
        flagVscp->varp()->setIgnorePostWrite();
        // Set the flag at the original NBA
        nodep->addHereThisAsNext(  //
            പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, flagVscp, VAccess::WRITE},
                          പുതിയ AstConst{flp, AstConst::BitTrue{}}});
        // Add the 'Post' scheduled commit
        AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, പുതിയ AstVarRef{flp, flagVscp, VAccess::READ}};
        vscpInfo.flagUniqueKit().postp->addStmtsp(ifp);
        // Immediately clear the flag
        ifp->addThensp(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, flagVscp, VAccess::WRITE},
                                     പുതിയ AstConst{flp, AstConst::BitFalse{}}});
        // Commit the value
        ifp->addThensp(പുതിയ AstAssign{flp, capturedLhsp, capturedRhsp});

        // Delete original NBA
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Scheme::ValueQueuePartial/Scheme::ValueQueueWhole
    template <തർക്കം N_Partial>
    ശൂന്യം prepare_Scheme_മൂല്യം_Queue(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(N_Partial ? vscpInfo.m_scheme == Scheme::മൂല്യം_Queue_Partial
                              : vscpInfo.m_scheme == Scheme::മൂല്യം_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_വരി* സ്ഥിരം flp = vscp->fileline();
        AstScope* സ്ഥിരം scopep = vscp->scopep();

        // Create the commit queue variable
        Ast_കണ്ണി_DType* സ്ഥിരം elemDTypep = vscp->dtypep()->skipRefp();
        AstNBACommitQueueDType*& cqDTypep = m_cqDTypeps[{elemDTypep, N_Partial}];
        എങ്കിൽ (!cqDTypep) {
            cqDTypep = പുതിയ AstNBACommitQueueDType{flp, elemDTypep, N_Partial};
            v3Global.rootp()->typeTablep()->addTypesp(cqDTypep);
        }
        സ്ഥിരം std::string name = "CommitQueue__" + vscp->varp()->shortName();
        AstVarScope* സ്ഥിരം queueVscp = m_dlyTmps.make(flp, scopep, cqDTypep, name);
        queueVscp->varp()->noReset(സത്യം);
        queueVscp->varp()->setIgnorePostWrite();
        vscpInfo.മൂല്യം_Queue_Kit().vscp = queueVscp;
        // Create the AstActive for the Post logic
        AstActive* സ്ഥിരം activep
            = പുതിയ AstActive{flp, "nba-value-queue-whole", vscpInfo.senTreep()};
        activep->sen_വൃക്ഷം_Storep(vscpInfo.senTreep());
        scopep->addBlocksp(activep);
        // Add 'Post' scheduled process for the commit
        AstAlwaysPost* സ്ഥിരം postp = പുതിയ AstAlwaysPost{flp};
        activep->addStmtsp(postp);
        // Add the commit
        AstCMethodHard* സ്ഥിരം callp = പുതിയ AstCMethodHard{
            flp, പുതിയ AstVarRef{flp, queueVscp, VAccess::READWRITE}, VCMethod::NBA_COMMIT};
        callp->dtypeSetVoid();
        // TODO: this is a partial update, so must be READWRITE, but that breaks scheduling
        callp->addPinsp(പുതിയ AstVarRef{flp, vscp, VAccess::WRITE});
        postp->addStmtsp(callp->makeStmt());
    }

    ശൂന്യം convert_Scheme_മൂല്യം_Queue(AstAssignDly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo,
                                 തർക്കം partial) {
        UASSERT_OBJ(partial ? vscpInfo.m_scheme == Scheme::മൂല്യം_Queue_Partial
                            : vscpInfo.m_scheme == Scheme::മൂല്യം_Queue_Whole,
                    vscp, "Inconsistent NBA scheme");
        File_വരി* സ്ഥിരം flp = vscp->fileline();
        AstScope* സ്ഥിരം scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        സ്ഥിരം std::string baseName = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlinkFrBack(), "Val" + baseName);

        // Unlink and capture the LHS reference
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlinkFrBack(), baseName);

        // RHS value (can be widened/masked iff Partial)
        Ast_കണ്ണി_വ്യഞ്ജകം* valuep = capturedRhsp;
        // RHS mask (iff Partial)
        Ast_കണ്ണി_വ്യഞ്ജകം* maskp = nullptr;

        // Running node for LHS deconstruction
        Ast_കണ്ണി_വ്യഞ്ജകം* lhsNodep = capturedLhsp;

        // If partial updates are required, construct the mask and the widened value
        എങ്കിൽ (partial) {
            // Type of array element
            Ast_കണ്ണി_DType* സ്ഥിരം eDTypep = [&]() -> Ast_കണ്ണി_DType* {
                Ast_കണ്ണി_DType* dtypep = vscp->dtypep()->skipRefp();
                ആകുമ്പോൾ (AstUnpackArrayDType* സ്ഥിരം ap = VN_CAST(dtypep, UnpackArrayDType)) {
                    dtypep = ap->subDTypep()->skipRefp();
                }
                തിരികെ dtypep;
            }();

            എങ്കിൽ (സ്ഥിരം AstSel* സ്ഥിരം lSelp = VN_CAST(lhsNodep, Sel)) {
                // This is a partial assignment.
                // Need to create a mask and widen the value to element size.
                lhsNodep = lSelp->fromp();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം sLsbp = lSelp->lsbp();
                സ്ഥിരം പൂർണ്ണസംഖ്യ sWidth = lSelp->widthConst();

                // Create mask value
                maskp = [&]() -> Ast_കണ്ണി_വ്യഞ്ജകം* {
                    // Constant mask we can compute here
                    എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം cLsbp = VN_CAST(sLsbp, Const)) {
                        AstConst* സ്ഥിരം cp = പുതിയ AstConst{flp, AstConst::DTyped{}, eDTypep};
                        cp->അക്കം().setMask(sWidth, cLsbp->toSInt());
                        തിരികെ cp;
                    }

                    // A non-constant mask we must compute at run-time.
                    AstConst* സ്ഥിരം onesp = പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, sWidth, 0};
                    onesp->അക്കം().setAllBits1();
                    തിരികെ createWidened(flp, scopep, eDTypep, sLsbp, sWidth, "Mask" + baseName,
                                         onesp, nodep);
                }();

                // Adjust value to element size
                valuep = [&]() -> Ast_കണ്ണി_വ്യഞ്ജകം* {
                    // Constant value with constant select we can compute here
                    എങ്കിൽ (AstConst* സ്ഥിരം cValuep = VN_CAST(valuep, Const)) {
                        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം cLsbp = VN_CAST(sLsbp, Const)) {
                            AstConst* സ്ഥിരം cp = പുതിയ AstConst{flp, AstConst::DTyped{}, eDTypep};
                            cp->അക്കം().setAllBits0();
                            cp->അക്കം().opSelInto(cValuep->അക്കം(), cLsbp->toSInt(), sWidth);
                            VL_DO_DANGLING(valuep->delete_വൃക്ഷം(), valuep);
                            തിരികെ cp;
                        }
                    }

                    // A non-constant value we must adjust.
                    തിരികെ createWidened(flp, scopep, eDTypep, sLsbp, sWidth,  //
                                         "Elem" + baseName, valuep, nodep);
                }();
            } അല്ലെങ്കിൽ {
                // If this assignment is not partial, set mask to ones and we are done
                AstConst* സ്ഥിരം ones = പുതിയ AstConst{flp, AstConst::DTyped{}, eDTypep};
                ones->അക്കം().setAllBits1();
                maskp = ones;
            }
        }

        // Extract array indices
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> idxps;
        {
            UASSERT_OBJ(VN_IS(lhsNodep, ArraySel), lhsNodep, "Unexpected LHS form");
            ആകുമ്പോൾ (AstArraySel* സ്ഥിരം aSelp = VN_CAST(lhsNodep, ArraySel)) {
                idxps.emplace_back(aSelp->bitp()->unlinkFrBack());
                lhsNodep = aSelp->fromp();
            }
            UASSERT_OBJ(VN_IS(lhsNodep, VarRef), lhsNodep, "Unexpected LHS form");
            std::reverse(idxps.begin(), idxps.end());
        }

        // Done with the LHS at this point
        VL_DO_DANGLING(തള്ളുക_Deletep(capturedLhsp), capturedLhsp);

        // Enqueue the update at the site of the original NBA
        AstCMethodHard* സ്ഥിരം callp = പുതിയ AstCMethodHard{
            flp, പുതിയ AstVarRef{flp, vscpInfo.മൂല്യം_Queue_Kit().vscp, VAccess::READWRITE},
            VCMethod::NBA_ENQUEUE};
        callp->dtypeSetVoid();
        callp->addPinsp(valuep);
        എങ്കിൽ (partial) callp->addPinsp(maskp);
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം indexp : idxps) callp->addPinsp(indexp);
        nodep->addHereThisAsNext(callp->makeStmt());

        // Delete original NBA
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // Record where a variable is assigned
    ശൂന്യം recordWriteRef(AstVarRef* nodep, തർക്കം nonBlocking) {
        // Ignore references in certain contexts
        എങ്കിൽ (m_ignoreBlkAndNBlk) തിരികെ;
        // Ignore if it's an array
        // TODO: we do this because it used to be the previous behaviour.
        //       Is it still required, or should we warn for arrays as well?
        //       Scheduling is no different for them...
        //       Clarification: This is OK for arrays of primitive types, but
        //       arrays that use the ShadowVar scheme don't work...
        എങ്കിൽ (VN_IS(nodep->varScopep()->dtypep()->skipRefp(), UnpackArrayDType)) തിരികെ;

        m_writeRefs(nodep->varScopep()).emplace_back(nodep, nonBlocking, m_inNonCombLogic);
    }

    template <typename Procedure_T>
    സ്ഥിതം തർക്കം isProcedureWithSentreep(സ്ഥിരം Ast_കണ്ണി_Procedure* സ്ഥിരം nodep) {
        സ്ഥിരം Procedure_T* സ്ഥിരം procedurep = Ast_കണ്ണി::cast<Procedure_T>(nodep);
        തിരികെ procedurep && procedurep->sentreep();
    }

    // Visit AstCFunc from a AstNodeCCall - this is made into a separate quasi-visitor because
    // AstCFunc that is not called from the code (e.g.: DPI exports) does not need to be visited
    // this way. Also, not visiting such AstCFuncs allows to avoid caching results for them which
    // this function does - which could lead to excessive memory usage
    ശൂന്യം visitCalledCFunc(AstCFunc* സ്ഥിരം nodep) {
        CFuncCache& മൂല്യം = m_cfuncsCache(nodep);
        തിരഞ്ഞെടുപ്പ് (മൂല്യം.m_state) {
        അവസ്ഥ CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_timingDomains);

            // Visit
            മൂല്യം.m_state = CFuncCache::VISITING;
            m_call_അട്ടി.തള്ളുക_back(nodep);
            {
                VL_RESTORER(m_cfuncp);
                m_cfuncp = nodep;
                iterateChildren(nodep);
            }
            m_call_അട്ടി.എടുക്കുക_back();
            മൂല്യം.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_timingDomains, മൂല്യം.m_timingDomains);
        } നിർത്തുക;
        അവസ്ഥ CFuncCache::VISITING: {
            വേണ്ടി (size_t i = m_call_അട്ടി.size() - 1; m_call_അട്ടി.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_അട്ടി[i]).m_includes.തിരുകുക(nodep);
            }
            തിരികെ;  // Break recursion
        }
        അവസ്ഥ CFuncCache::INITIALIZED: നിർത്തുക;
        }
        std::unordered_set<സ്ഥിരം AstCFunc*> visited;
        add_CFunc_Cached_മൂല്യങ്ങൾ(nodep, visited);
    }

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override {
        iterateChildren(nodep);
        // Decide which scheme to use for each variable and do the 'prepare' step
        വേണ്ടി (AstVarScope* സ്ഥിരം vscp : m_vscps) {
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            vscpInfo.m_scheme = chooseScheme(vscp, vscpInfo);
            // Run 'prepare' step
            തിരഞ്ഞെടുപ്പ് (vscpInfo.m_scheme) {
            അവസ്ഥ Scheme::Undecided:  // LCOV_EXCL_START
                UASSERT_OBJ(അസത്യം, vscp, "Failed to choose NBA scheme");
                നിർത്തുക;  // LCOV_EXCL_STOP
            അവസ്ഥ Scheme::UnsupportedCompoundArrayInLoop: {
                // Will report error at the site of the NBA
                നിർത്തുക;
            }
            അവസ്ഥ Scheme::ShadowVar: {
                ++m_nSchemeShadowVar;
                prepareSchemeShadowVar(vscp, vscpInfo);
                നിർത്തുക;
            }
            അവസ്ഥ Scheme::ShadowVarMasked: {
                ++m_nSchemeShadowVarMasked;
                prepareSchemeShadowVarMasked(vscp, vscpInfo);
                നിർത്തുക;
            }
            അവസ്ഥ Scheme::FlagShared: {
                ++m_nSchemeFlagShared;
                prepareSchemeFlagShared(vscp, vscpInfo);
                നിർത്തുക;
            }
            അവസ്ഥ Scheme::FlagUnique: {
                ++m_nSchemeFlagUnique;
                prepareSchemeFlagUnique(vscp, vscpInfo);
                നിർത്തുക;
            }
            അവസ്ഥ Scheme::മൂല്യം_Queue_Whole: {
                ++m_n_Scheme_മൂല്യം_Queues_Whole;
                prepare_Scheme_മൂല്യം_Queue</* Partial: */ അസത്യം>(vscp, vscpInfo);
                നിർത്തുക;
            }
            അവസ്ഥ Scheme::മൂല്യം_Queue_Partial: {
                ++m_n_Scheme_മൂല്യം_Queues_Partial;
                prepare_Scheme_മൂല്യം_Queue</* Partial: */ സത്യം>(vscp, vscpInfo);
                നിർത്തുക;
            }
            }
        }
        // Convert all NBAs
        വേണ്ടി (സ്ഥിരം NBA& nba : m_nbas) {
            AstAssignDly* സ്ഥിരം nbap = nba.nodep;
            AstVarScope* സ്ഥിരം vscp = nba.vscp;
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            // Run 'convert' step
            തിരഞ്ഞെടുപ്പ് (vscpInfo.m_scheme) {
            അവസ്ഥ Scheme::Undecided: {  // LCOV_EXCL_START
                UASSERT_OBJ(അസത്യം, vscp, "Unreachable");
                നിർത്തുക;
            }  // LCOV_EXCL_STOP
            അവസ്ഥ Scheme::UnsupportedCompoundArrayInLoop: {
                // TODO: make this an E_UNSUPPORTED...
                nbap->v3warn(BLKLOOPINIT, "Unsupported: Non-blocking assignment to array with "
                                          "compound element type inside loop");
                നിർത്തുക;
            }
            അവസ്ഥ Scheme::ShadowVar: {
                convertSchemeShadowVar(nbap, vscp, vscpInfo);
                നിർത്തുക;
            }
            അവസ്ഥ Scheme::ShadowVarMasked: {
                convertSchemeShadowVarMasked(nbap, vscp, vscpInfo);
                നിർത്തുക;
            }
            അവസ്ഥ Scheme::FlagShared: {
                convertSchemeFlagShared(nbap, vscp, vscpInfo);
                നിർത്തുക;
            }
            അവസ്ഥ Scheme::FlagUnique: {
                convertSchemeFlagUnique(nbap, vscp, vscpInfo);
                നിർത്തുക;
            }
            അവസ്ഥ Scheme::മൂല്യം_Queue_Whole: {
                convert_Scheme_മൂല്യം_Queue(nbap, vscp, vscpInfo, /* partial: */ അസത്യം);
                നിർത്തുക;
            }
            അവസ്ഥ Scheme::മൂല്യം_Queue_Partial:
                convert_Scheme_മൂല്യം_Queue(nbap, vscp, vscpInfo, /* partial: */ സത്യം);
                നിർത്തുക;
            }
        }
    }
    ശൂന്യം visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstActive* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ignoreBlkAndNBlk);
        VL_RESTORER(m_inNonCombLogic);
        m_activep = nodep;
        സ്ഥിരം Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep = nodep->sentreep();
        m_ignoreBlkAndNBlk = senTreep->hasStatic() || senTreep->hasInitial();
        m_inNonCombLogic = senTreep->hasClocked();
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override {
        VL_RESTORER(m_needsInitialTrigger);
        VL_RESTORER_CLEAR(m_nbaEventSenTreeps);
        സ്ഥിരം size_t firstNBAAddedIndex = m_nbas.size();
        {
            VL_RESTORER(m_inSuspendableOrFork);
            VL_RESTORER(m_procp);
            VL_RESTORER(m_ignoreBlkAndNBlk);
            VL_RESTORER(m_inNonCombLogic);
            // When we are dealing with initial block we need to
            // treat it as suspendable when we meet a NBA
            m_inSuspendableOrFork = nodep->isSuspendable() || VN_IS(nodep, Initial);
            m_procp = nodep;
            എങ്കിൽ (nodep->isSuspendable()) {
                m_ignoreBlkAndNBlk = അസത്യം;
                m_inNonCombLogic = സത്യം;
            }
            iterateChildren(nodep);
        }
        auto containsClocled = [](സ്ഥിരം AstSenItem* itemp) {
            ആകുമ്പോൾ (itemp) {
                എങ്കിൽ (itemp->edgeType().clockedStmt()) തിരികെ സത്യം;
                itemp = VN_AS(itemp->nextp(), SenItem);
            }
            തിരികെ അസത്യം;
        };
        സ്ഥിരം തർക്കം addInitialTrigger = m_needsInitialTrigger
                                       && !(isProcedureWithSentreep<AstAlways>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysObserved>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysReactive>(nodep))
                                       && !containsClocled(m_activep->sentreep()->sensesp());
        എങ്കിൽ (m_timingDomains.empty() && !addInitialTrigger) തിരികെ;

        // There were some timing domains involved in the process. Add all of them as sensitivities
        // of all NBA targets in this process. Note this is a bit of a sledgehammer, we should only
        // need those that directly precede the NBA in control flow, but that is hard to compute,
        // so we will hammer away.

        // First gather all senItems
        AstSenItem* senItemp = nullptr;
        എങ്കിൽ (addInitialTrigger) {
            senItemp = പുതിയ AstSenItem{nodep->fileline(), AstSenItem::InitialNBA{}};
            ++m_nInitialNBA;
        }

        വേണ്ടി (സ്ഥിരം Ast_Sen_വൃക്ഷം* സ്ഥിരം domainp : m_timingDomains) {
            എങ്കിൽ (domainp->sensesp())
                senItemp = Ast_കണ്ണി::addNext(senItemp, domainp->sensesp()->clone_വൃക്ഷം(സത്യം));
        }
        m_timingDomains.clear();
        // Add them to all nba targets we gathered in this process
        വേണ്ടി (size_t i = firstNBAAddedIndex; i < m_nbas.size(); ++i) {
            m_vscpInfo(m_nbas[i].vscp).addSensitivity(senItemp);
        }
        വേണ്ടി (Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep : m_nbaEventSenTreeps)
            senTreep->addSensesp(senItemp->clone_വൃക്ഷം(സത്യം));
        // Done with these
        VL_DO_DANGLING(senItemp->delete_വൃക്ഷം(), senItemp);
    }
    ശൂന്യം visit(AstFork* nodep) override {
        VL_RESTORER(m_inSuspendableOrFork);
        m_inSuspendableOrFork = സത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCAwait* nodep) override {
        എങ്കിൽ (nodep->sentreep()) m_timingDomains.തിരുകുക(nodep->sentreep());
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstFireEvent* nodep) override {
        UASSERT_OBJ(v3Global.hasEvents(), nodep, "Inconsistent");
        File_വരി* സ്ഥിരം flp = nodep->fileline();

        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം eventp = nodep->operandp()->unlinkFrBack();

        // Enqueue for clearing 'triggered' state on next eval
        AstCStmt* സ്ഥിരം cstmtp = പുതിയ AstCStmt{flp};
        cstmtp->add("vlSymsp->fireEvent(");
        cstmtp->add(eventp);
        cstmtp->add(");");

        Ast_കണ്ണി* newp = cstmtp;
        സ്ഥിരം AstVarRef* സ്ഥിരം vrefp = VN_CAST(eventp, VarRef);
        എങ്കിൽ (nodep->isDelayed() && (m_cfuncp || !vrefp)) {
            // V3Timing converts these with --timing
            nodep->v3warn(E_NOTIMING, "Nonblocking event trigger "
                                          << (m_cfuncp ? "in a non-inlined function/task"
                                                       : "of a class or interface member or "
                                                         "array element")
                                          << " requires --timing");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isDelayed()) {
            സ്ഥിരം std::string newvarname = "_" + vrefp->varp()->shortName();
            AstVarScope* സ്ഥിരം dlyvscp
                = m_dlyTmps.make(flp, vrefp->varScopep()->scopep(), 1, newvarname);

            സ്ഥിരം auto dlyRef = [=](VAccess access) {  //
                തിരികെ പുതിയ AstVarRef{flp, dlyvscp, access};
            };

            AstAlwaysPost* സ്ഥിരം postp = പുതിയ AstAlwaysPost{flp};
            AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, dlyRef(VAccess::READ)};
            postp->addStmtsp(ifp);

            UASSERT_OBJ(m_activep, nodep, "No active to handle FireEvent");
            Ast_Sen_വൃക്ഷം* senTreep = m_activep->sentreep();
            AstActive* activep = nullptr;
            എങ്കിൽ (m_inSuspendableOrFork && (senTreep->hasInitial() || senTreep->hasClocked())) {
                // Suspendable code can trigger whenever it resumes, so fire when its NBAs are
                // committed (see visit(AstNodeProcedure*)), clearing the flag like
                // Scheme::FlagUnique
                senTreep = പുതിയ Ast_Sen_വൃക്ഷം{
                    flp, senTreep->hasClocked() ? senTreep->sensesp()->clone_വൃക്ഷം(സത്യം) : nullptr};
                m_nbaEventSenTreeps.തള്ളുക_back(senTreep);
                m_needsInitialTrigger |= m_timingDomains.empty();
                dlyvscp->varp()->setIgnorePostWrite();
                ifp->addThensp(പുതിയ AstAssign{flp, dlyRef(VAccess::WRITE),
                                             പുതിയ AstConst{flp, AstConst::BitFalse{}}});
                activep = പുതിയ AstActive{flp, "nba-event", senTreep};
                activep->sen_വൃക്ഷം_Storep(senTreep);
            } അല്ലെങ്കിൽ {
                activep = പുതിയ AstActive{flp, "nba-event", senTreep};
                AstAlwaysPre* സ്ഥിരം prep = പുതിയ AstAlwaysPre{flp};
                prep->addStmtsp(പുതിയ AstAssign{flp, dlyRef(VAccess::WRITE),
                                              പുതിയ AstConst{flp, AstConst::BitFalse{}}});
                activep->addStmtsp(prep);
            }
            ifp->addThensp(newp);
            m_activep->addNextHere(activep);
            activep->addStmtsp(postp);

            newp = പുതിയ AstAssign{flp, dlyRef(VAccess::WRITE),
                                 പുതിയ AstConst{flp, AstConst::BitTrue{}}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstAssignDly* nodep) override {
        // Prevent double processing due to AstExprStmt being moved before this node
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;

        എങ്കിൽ (m_cfuncp) {
            എങ്കിൽ (!v3Global.rootp()->nbaEventp()) {
                nodep->v3warn(
                    E_NOTIMING,
                    "Delayed assignment in a non-inlined function/task requires --timing");
            }
            തിരികെ;
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
        nodep->lhsp()->foreach([&](Ast_കണ്ണി* currp) {
            // cppcheck-suppress constVariablePointer
            എങ്കിൽ (Ast_വ്യഞ്ജകം_Stmt* സ്ഥിരം exprp = VN_CAST(currp, വ്യഞ്ജകം_Stmt)) {
                // Move statements before the NBA
                nodep->addHereThisAsNext(exprp->stmtsp()->unlinkFrBackWithNext());
                // Replace with result
                currp->replaceWith(exprp->resultp()->unlinkFrBack());
                // Get rid of the AstExprStmt
                VL_DO_DANGLING2(തള്ളുക_Deletep(currp), currp, exprp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(currp, VarRef)) {
                // Ignore reads (e.g.: '_[*here*] <= _')
                എങ്കിൽ (refp->access().isReadOnly()) തിരികെ;
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
        AstVarScope* സ്ഥിരം vscp = m_currNbaLhsRefp->varScopep();
        // Record it on first encounter
        VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
        എങ്കിൽ (!vscpInfo.m_firstNbaRefp) {
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
        എങ്കിൽ (m_activep->sentreep()->hasClocked()) {
            എങ്കിൽ (vscpInfo.m_fistActivep != m_activep) {
                AstVar* സ്ഥിരം varp = vscp->varp();
                എങ്കിൽ (!varp->user1SetOnce()
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
        recordWriteRef(m_currNbaLhsRefp, സത്യം);

        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        // Already checked the NBA LHS ref, ignore here
        എങ്കിൽ (nodep == m_currNbaLhsRefp) തിരികെ;
        // Only care about write refs
        എങ്കിൽ (!nodep->access().isWriteOrRW()) തിരികെ;
        // Record write reference
        recordWriteRef(nodep, അസത്യം);
    }
    ശൂന്യം visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = സത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_CCall* സ്ഥിരം nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        സ്ഥിരം auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        എങ്കിൽ (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } അല്ലെങ്കിൽ {
            വേണ്ടി (AstCFunc* സ്ഥിരം cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    ശൂന്യം visit(AstCFunc* സ്ഥിരം nodep) override {
        സ്ഥിരം auto& മൂല്യം = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        എങ്കിൽ (മൂല്യം.m_state != CFuncCache::UNINITIALIZED) തിരികെ;
        VL_RESTORER(m_cfuncp);
        // Its timing domains are those of the processes calling it, see visitCalledCFunc()
        VL_RESTORER_CLEAR(m_timingDomains);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }

    // Pre/Post logic are created here and their content need no further changes, so ignore.
    ശൂന്യം visit(AstAlwaysPre*) override {}
    ശൂന്യം visit(AstAlwaysPost*) override {}

    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
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
        V3Stats::addStat("NBA, variables using ValueQueueWhole scheme", m_n_Scheme_മൂല്യം_Queues_Whole);
        V3Stats::addStat("NBA, variables using ValueQueuePartial scheme",
                         m_n_Scheme_മൂല്യം_Queues_Partial);
        V3Stats::addStat("Optimizations, NBA flags shared", m_nSharedSetFlags);
        V3Stats::addStat("Procedures needing initial NBA trigger", m_nInitialNBA);
        V3Stats::addStat("Non-inlined co_awaits with SenTree", m_non_Inlined_CAwaits_With_Sen_വൃക്ഷം);
    }
};

//######################################################################
// Delayed class functions

ശൂന്യം V3Delayed::delayedAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DelayedVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("delayed", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
