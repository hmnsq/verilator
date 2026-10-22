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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Delayed.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3ClassGraph.h"
#ضمن "V3Const.h"
#ضمن "V3LinkLValue.h"
#ضمن "V3SharedTmps.h"
#ضمن "V3Stats.h"

#ضمن <deque>
#ضمن <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert AstAssignDlys (NBAs)

صنف DelayedVisitor final : عام VNVisitor {
    // TYPES

    // The various NBA conversion schemes, including error cases
    تعداد صنف Scheme : uint8_t {
        Undecided = 0,
        Unsupported_Compound_Array_دننه_Loop,
        ShadowVar,
        ShadowVarMasked,
        FlagShared,
        FlagUnique,
        ارزښت_کتار_ټولیز,
        ارزښت_کتار_Partial
    };

    // All info associated with a variable that is the target of an NBA
    صنف VarScopeInfo final {
    عام:
        // First write reference encountered to the VarScope as the target on an NBA
        ثابت AstVarRef* m_لومړی_Nba_Refp = تش_اشاره;
        // Active block 'm_firstNbaRefp' is under
        ثابت Ast_فعال* m_fistActivep = تش_اشاره;
        منطقي m_ټولیز = خطأ;  // Used on LHS of NBA directly via VarRef
        منطقي m_partial = خطأ;  // Used on LHS of NBA under a Sel
        منطقي m_دننه_Loop = خطأ;  // Used on LHS of NBA in a loop
        منطقي m_دننه_Susp_Or_Fork = خطأ;  // Used on LHS of NBA in suspendable process or fork
        Scheme m_scheme = Scheme::Undecided;  // Conversion scheme to use for this variable

    خاص:
        // Combined sensitivities of all NBAs targeting this variable
        Ast_Sen_ونه* m_senTreep = تش_اشاره;

        // Union of stuff needed for the various schemes - use accessors below!
        اتحاد {
            بنية {  // Stuff needed for Scheme::ShadowVar
                AstVarScope* vscp;  // The shadow variable
            } m_shadowVariableKit;
            بنية {  // Stuff needed for Scheme::ShadowVarMasked
                AstVarScope* vscp;  // The shadow variable
                AstVarScope* maskp;  // The mask variable
            } m_shadowVarMaskedKit;
            بنية {  // Stuff needed for Scheme::FlagShared
                Ast_فعال* activep;  // The active block for the Pre/Post logic
                AstAlwaysPost* postp;  // The post block for commiting results
                AstVarScope* commitFlagp;  // The commit flag variable, for reuse
                AstIf* commitIfp;  // The previous if statement for committing, for reuse
            } m_flagSharedKit;
            بنية {  // Stuff needed for Scheme::FlagUnique
                AstAlwaysPost* postp;  // The post block for commiting results
            } m_flagUniqueKit;
            بنية {  // Stuff needed for Scheme::ValueQueueWhole/Scheme::ValueQueuePartial
                AstVarScope* vscp;  // The commit queue variable
            } m_ارزښت_کتار_Kit;
        } m_kitUnion;

    عام:
        VarScopeInfo() = افتراضي;
        ~VarScopeInfo() {
            // Might not be linked if there was an error
            إذا (!m_senTreep->backp()) VL_DO_DANGLING(m_senTreep->delete_ونه(), m_senTreep);
        }
        // Accessors for the above union fields
        خپلکاری& shadowVariableKit() {
            UASSERT(m_scheme == Scheme::ShadowVar, "Inconsistent Scheme");
            أرجع m_kitUnion.m_shadowVariableKit;
        }
        خپلکاری& shadowVarMaskedKit() {
            UASSERT(m_scheme == Scheme::ShadowVarMasked, "Inconsistent Scheme");
            أرجع m_kitUnion.m_shadowVarMaskedKit;
        }
        خپلکاری& flagSharedKit() {
            UASSERT(m_scheme == Scheme::FlagShared, "Inconsistent Scheme");
            أرجع m_kitUnion.m_flagSharedKit;
        }
        خپلکاری& flagUniqueKit() {
            UASSERT(m_scheme == Scheme::FlagUnique, "Inconsistent Scheme");
            أرجع m_kitUnion.m_flagUniqueKit;
        }
        خپلکاری& ارزښت_کتار_Kit() {
            UASSERT(m_scheme == Scheme::ارزښت_کتار_Partial || m_scheme == Scheme::ارزښت_کتار_ټولیز,
                    "Inconsistent Scheme");
            أرجع m_kitUnion.m_ارزښت_کتار_Kit;
        }

        // Accessor
        Ast_Sen_ونه* senTreep() ثابت { أرجع m_senTreep; }

        // Add sensitivities
        فراغ جمع_کړه_حساسیت(Ast_Sen_توکی* nodep) {
            إذا (!m_senTreep) m_senTreep = جديد Ast_Sen_ونه{nodep->fileline(), تش_اشاره};
            // Add a copy of each term
            m_senTreep->جمع_کړه_Sensesp(nodep->clone_ونه(ریښتیا));
            // Remove duplicates
            V3Const::constifyExpensiveEdit(m_senTreep);
        }
        // cppcheck-suppress constParameterPointer
        فراغ جمع_کړه_حساسیت(Ast_Sen_ونه* nodep) { جمع_کړه_حساسیت(nodep->sensesp()); }
    };

    // Data structure to keep track of all writes to
    بنية WriteReference final {
        AstVarRef* m_refp = تش_اشاره;  // The reference
        منطقي m_isNBA = خطأ;  // True if an NBA write
        منطقي m_دننه_Non_Comb = خطأ;  // True if reference is known to be in non-combinational logic
        WriteReference() = افتراضي;
        WriteReference(AstVarRef* refp, منطقي isNBA, منطقي دننه_Non_Comb)
            : m_refp{refp}
            , m_isNBA{isNBA}
            , m_دننه_Non_Comb{دننه_Non_Comb} {}
    };

    // Data required to lower AstAssignDelay later
    بنية NBA final {
        Ast_ټاکل_Dly* nodep = تش_اشاره;  // The NBA this record refers to
        AstVarScope* vscp = تش_اشاره;  // The target variable the NBA is updating
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
    ثابت VNUser1_دننه_Use m_user1_دننه_Use;
    ثابت VNUser2_دننه_Use m_user2_دننه_Use;
    ثابت VNUser3_دننه_Use m_user3_دننه_Use;

    بنية CFuncCache final {
        VInsertion_سېټ<Ast_Sen_ونه*> m_timingDomains;  // What shall be added to m_timingDomains
        std::سېټ<AstCFunc*>
            m_includes;  // CFuncs whose CFuncCache shall be included into this - this is used to
                         // break cycles: A->B->A (instead of visiting A while it is still begin
                         // visited B just marks that it includes A)
        تعداد State : uint8_t {
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
    AstUser3Allocator<AstVarScope, std::ویکټور<WriteReference>> m_writeRefs;

    // STATE - across all visitors
    VInsertion_سېټ<Ast_Sen_ونه*> m_timingDomains;  // Timing resume domains
    V3SharedTmps m_dlyTmps{"__Vdly", VVarType::BLOCKTEMP};  // Temporary variables
    // Commit queue data types, by element type and partial flag. Shared by all instances,
    // as m_dlyTmps only shares variables with the same data type.
    std::map<std::pair<ثابت Ast_غوټه_DType*, منطقي>, Ast_NBACommit_کتار_DType*> m_cqDTypeps;

    ثابت std::unique_ptr<V3_Class_ګراف>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::ویکټور<ثابت AstCFunc*> m_call_ډېری;  // Current callstack of AstCFuncs

    // STATE - for current visit position (use VL_RESTORER)
    Ast_فعال* m_activep = تش_اشاره;  // Current activate
    ثابت AstCFunc* m_cfuncp = تش_اشاره;  // Current public C Function
    Ast_غوټه_Procedure* m_procp = تش_اشاره;  // Current process
    AstScope* m_scopep = تش_اشاره;  // Current scope
    منطقي m_دننه_Loop = خطأ;  // True in for loops
    منطقي m_دننه_Suspendable_Or_Fork = خطأ;  // True in suspendable processes and forks
    منطقي m_ignoreBlkAndNBlk = خطأ;  // Suppress delayed assignment BLKANDNBLK
    منطقي m_دننه_Non_Comb_Logic = خطأ;  // We are in non-combinational logic
    منطقي m_needsInitialTrigger = خطأ;  // Whether a NodeProcedure needs a initial trigger
    std::ویکټور<Ast_Sen_ونه*> m_nbaEventSenTreeps;  // Sensitivities of '->>' in the process
    AstVarRef* m_currNbaLhsRefp = تش_اشاره;  // Current NBA LHS variable reference

    // STATE - during NBA conversion (after visit)
    std::ویکټور<NBA> m_nbas;  // AstAssignDly instances to lower at the end
    std::ویکټور<AstVarScope*> m_vscps;  // Target variables on LHSs of NBAs
    Ast_ټاکل_Dly* m_بل_Dlyp = تش_اشاره;  // The nextp of the previous AstAssignDly
    AstVarScope* m_prevVscp = تش_اشاره;  // The target of the previous AstAssignDly

    // STATE - Statistic tracking
    VDouble0 m_nSchemeShadowVar;  // Number of variables using Scheme::ShadowVar
    VDouble0 m_nSchemeShadowVarMasked;  // Number of variabels using Scheme::ShadowVarMasked
    VDouble0 m_nSchemeFlagShared;  // Number of variables using Scheme::FlagShared
    VDouble0 m_nSchemeFlagUnique;  // Number of variables using Scheme::FlagUnique
    VDouble0 m_n_Scheme_ارزښت_Queues_ټولیز;  //  Number of variables using Scheme::ValueQueueWhole
    VDouble0 m_n_Scheme_ارزښت_Queues_Partial;  //  Number of variables using Scheme::ValueQueuePartial
    VDouble0 m_n_Shared_سېټ_Flags;  // "Set" flags actually shared by Scheme::FlagShared variables
    VDouble0 m_nInitialNBA;  // Number of procedural blocks with initial NBA
    VDouble0 m_non_Inlined_CAwaits_With_Sen_ونه;  // Count uses of not inlined co_awaits

    // METHODS

    // Return true iff a variable is assigned by both blocking and nonblocking
    // assignments. Issue BLKANDNBLK error if we can't prove the mixed
    // assignments are to independent bits and the blocking assignment can be
    // in combinational logic, which is something we can't safely implement
    // still.
    منطقي checkMixedUsage(ثابت AstVarScope* vscp, منطقي isIntegralOrPacked) {

        بنية Ref final {
            AstVarRef* m_refp;  // The reference
            منطقي m_دننه_Non_Comb;  // True if known to be in non-combinational logic
            صحيح m_lsb;  // LSB of accessed range
            صحيح m_msb;  // MSB of accessed range
            Ref(AstVarRef* refp, منطقي دننه_Non_Comb, صحيح lsb, صحيح msb)
                : m_refp{refp}
                , m_دننه_Non_Comb{دننه_Non_Comb}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };

        std::ویکټور<Ref> blkRefs;  // Blocking writes
        std::ویکټور<Ref> nbaRefs;  // Non-blockign writes

        ثابت صحيح پلنوالی = isIntegralOrPacked ? vscp->پلنوالی() : 1;

        لكل (ثابت خپلکاری& writeRef : m_writeRefs(vscp)) {
            صحيح lsb = 0;
            صحيح msb = پلنوالی - 1;
            إذا (ثابت AstSel* ثابت selp = VN_CAST(writeRef.m_refp->backp(), Sel)) {
                إذا (VN_IS(selp->lsbp(), Const)) {
                    lsb = selp->lsbConst();
                    msb = selp->msbConst();
                }
            }
            إذا (writeRef.m_isNBA) {
                nbaRefs.emplace_شا(writeRef.m_refp, writeRef.m_دننه_Non_Comb, lsb, msb);
            } وإلا {
                blkRefs.emplace_شا(writeRef.m_refp, writeRef.m_دننه_Non_Comb, lsb, msb);
            }
        }
        // We only run this function on targets of NBAs, so there should be at least one...
        UASSERT_OBJ(!nbaRefs.تشه(), vscp, "Did not record NBA write");
        // If no blocking upadte, then we are good
        إذا (blkRefs.تشه()) أرجع خطأ;

        // If the blocking assignment is in non-combinational logic (i.e.:
        // in logic that has an explicit trigger), then we can safely
        // implement it (there is no race between clocked logic and post
        // scheduled logic), so need not error
        blkRefs.erase(std::remove_if(blkRefs.begin(), blkRefs.پای_ټکی(),
                                     [](ثابت Ref& ref) { أرجع ref.m_دننه_Non_Comb; }),
                      blkRefs.پای_ټکی());

        // If nothing left, then we need not error
        إذا (blkRefs.تشه()) أرجع ریښتیا;

        // If not a packed variable, warn here as we can't prove independence
        إذا (!isIntegralOrPacked) {
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
            أرجع ریښتیا;
        }

        // We need to error if we can't prove the written bits are independent

        // Sort refs by interval
        ثابت خپلکاری lessThanRef = [](ثابت Ref& a, ثابت Ref& b) {
            إذا (a.m_lsb != b.m_lsb) أرجع a.m_lsb < b.m_lsb;
            أرجع a.m_msb < b.m_msb;
        };
        std::stable_ترتیبول(blkRefs.begin(), blkRefs.پای_ټکی(), lessThanRef);
        std::stable_ترتیبول(nbaRefs.begin(), nbaRefs.پای_ټکی(), lessThanRef);
        // Iterate both vectors, checking for overlap
        خپلکاری bIt = blkRefs.begin();
        خپلکاری nIt = nbaRefs.begin();
        طالما (bIt != blkRefs.پای_ټکی() && nIt != nbaRefs.پای_ټکی()) {
            إذا (lessThanRef(*bIt, *nIt)) {
                إذا (nIt->m_lsb <= bIt->m_msb) اكسر;  // Stop on Overlap
                ++bIt;
            } وإلا {
                إذا (bIt->m_lsb <= nIt->m_msb) اكسر;  // Stop on Overlap
                ++nIt;
            }
        }

        // If we found an overlapping range that cannot be safely implemented, then wran...
        إذا (bIt != blkRefs.پای_ټکی() && nIt != nbaRefs.پای_ټکی()) {
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

        أرجع ریښتیا;
    }

    // Choose the NBA scheme used for the given variable.
    Scheme ټاکنه_Scheme(ثابت AstVarScope* vscp, ثابت VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::Undecided, vscp, "NBA scheme already decided");

        ثابت Ast_غوټه_DType* ثابت dtypep = vscp->dtypep()->skipRefp();
        // Unpacked arrays
        إذا (ثابت AstUnpackArrayDType* ثابت uaDTypep = VN_CAST(dtypep, UnpackArrayDType)) {
            // If whole array is target of NBA, use ShadowVar, but not in a suspendable process or
            // fork, which resume outside the 'nba' region, so its 'pre' logic would overwrite the
            // value they write in the shadow variable
            إذا (vscpInfo.m_ټولیز) {
                أرجع vscpInfo.m_دننه_Susp_Or_Fork ? Scheme::FlagUnique : Scheme::ShadowVar;
            }
            // Basic underlying type of elements, if any.
            ثابت AstBasicDType* ثابت basicp = uaDTypep->basicp();
            // If used in a loop, we must have a dynamic commit queue. (Also works in suspendables)
            إذا (vscpInfo.m_دننه_Loop) {
                // Arrays with compound element types are currently not supported in loops
                إذا (!basicp
                    || !(basicp->isIntegralOrPacked()  //
                         || basicp->isDouble()  //
                         || basicp->isString())) {
                    أرجع Scheme::Unsupported_Compound_Array_دننه_Loop;
                }
                إذا (vscpInfo.m_partial) أرجع Scheme::ارزښت_کتار_Partial;
                أرجع Scheme::ارزښت_کتار_ټولیز;
            }
            // A suspendable process or fork can resume outside the 'nba' region, so the 'pre'
            // logic of the other schemes would undo its updates. The unique flag scheme has none.
            إذا (vscpInfo.m_دننه_Susp_Or_Fork) أرجع Scheme::FlagUnique;
            // Otherwise if an array of packed/basic elements, use the shared flag scheme
            إذا (basicp) أرجع Scheme::FlagShared;
            // Finally fall back on the shadow variable scheme, e.g. for
            // arrays of unpacked structs. This will be slow.
            // TODO: generic LHS scheme as discussed in #5092
            أرجع Scheme::ShadowVar;
        }

        // Likewise, the 'pre' logic of the shadow variable scheme would undo the updates of a
        // suspendable process or fork, so use the unique flag scheme
        إذا (vscpInfo.m_دننه_Susp_Or_Fork) أرجع Scheme::FlagUnique;

        ثابت منطقي isIntegralOrPacked = dtypep->isIntegralOrPacked();
        // Check for mixed usage (this also warns if not OK)
        إذا (checkMixedUsage(vscp, isIntegralOrPacked)) {
            // If it's a variable updated by both blocking and non-blocking
            // assignments, use the ShadowVarMasked schem if masked update is
            // possible. This can handle blocking and non-blocking updates to
            // inpdendent parts correctly at run-time, and always works, even
            // in loops or other dynamic context.
            إذا (isIntegralOrPacked) أرجع Scheme::ShadowVarMasked;
            // If it's inside a loop, use Scheme::ShadowVar, which is safe,
            // but will generate incorrect code if a partial update is used
            إذا (vscpInfo.m_دننه_Loop) أرجع Scheme::ShadowVar;
            // Otherwise (for not packed variables), use the FlagUnique scheme,
            // which at least handles partial updates correctly, but might break
            // in loops or other dynamic context
            أرجع Scheme::FlagUnique;
        }

        // Otherwise use the simple shadow variable scheme
        أرجع Scheme::ShadowVar;
    }

    // Given an expression 'exprp', return a new expression that always evaluates to the
    // value of the given expression at this point in the program. That is:
    // - If given a non-constant expression, create a new temporary AstVarScope under the given
    //   'scopep', with the given 'name', assign the expression to it, and return a read reference
    //   to the new AstVarScope.
    // - If given a constant, just return that constant.
    // New statements are inserted before 'insertp'
    Ast_غوټه_بیان* captureVal(AstScope* ثابت scopep, Ast_غوټه_Stmt* ثابت insertp,
                            Ast_غوټه_بیان* ثابت exprp, ثابت std::string& name) {
        UASSERT_OBJ(!exprp->backp(), exprp, "Should have been unlinked");
        File_کرښه* ثابت flp = exprp->fileline();
        إذا (VN_IS(exprp, Const)) أرجع exprp;
        // TODO: there are some const variables that could be simply referenced here
        AstVarScope* ثابت tmpVscp = m_dlyTmps.make(flp, scopep, exprp->dtypep(), name);
        insertp->جمع_کړه_دلته_This_As_بل(
            جديد Ast_ټاکل{flp, جديد AstVarRef{flp, tmpVscp, VAccess::WRITE}, exprp});
        أرجع جديد AstVarRef{flp, tmpVscp, VAccess::READ};
    };

    // Similar to 'captureVal', but captures an LValue expression. That is, the returned
    // expression will reference the same location as the input expression, at this point in the
    // program.
    Ast_غوټه_بیان* captureLhs(AstScope* ثابت scopep, Ast_غوټه_Stmt* ثابت insertp,
                            Ast_غوټه_بیان* ثابت lhsp, ثابت std::string& بنسټ_Name) {
        UASSERT_OBJ(!lhsp->backp(), lhsp, "Should have been unlinked");
        // Running node pointer
        Ast_غوټه_بیان* nodep = lhsp;
        // Capture AstSel indices - there should be only one
        إذا (AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            ثابت std::string tmpName{"Lsb" + بنسټ_Name};
            selp->lsbp(captureVal(scopep, insertp, selp->lsbp()->unlink_Fr_شا(), tmpName));
            // Continue with target
            nodep = selp->fromp();
        }
        UASSERT_OBJ(!VN_IS(nodep, Sel), lhsp, "Multiple 'AstSel' applied to LHS reference");
        // Capture the indices of selects - might be many, also below struct members. If a handle
        // selects the instance the target is a member of, capture the handle instead, as it is
        // only read, to select the target when the NBA is executed (IEEE 1800-2023 10.4.2).
        اندازه_t nArraySels = 0;
        طالما (ریښتیا) {
            إذا (Ast_غوټه_Sel* ثابت selp = VN_CAST(nodep, غوټه_Sel)) {
                ثابت std::string tmpName{"Dim" + std::منزل_string(nArraySels++) + بنسټ_Name};
                selp->bitp(captureVal(scopep, insertp, selp->bitp()->unlink_Fr_شا(), tmpName));
                nodep = selp->fromp();
            } وإلا إذا (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
                nodep = selp->fromp();
            } وإلا إذا (AstMemberSel* ثابت selp = VN_CAST(nodep, MemberSel)) {
                Ast_غوټه_بیان* ثابت handlep = selp->fromp()->unlink_Fr_شا();
                V3LinkLValue::link_LValue_سېټ(handlep, VAccess::READ);
                AstVarRef* ثابت refp
                    = VN_AS(captureVal(scopep, insertp, handlep, "Handle" + بنسټ_Name), VarRef);
                refp->varp()->سېټ_Ignore_Post_Write();
                // Like the handle it replaces, mark it written, as is the member it selects
                refp->access(selp->access());
                selp->fromp(refp);
                أرجع lhsp;
            } وإلا {
                اكسر;
            }
        }
        // What remains must be the variable, or some sort of select, we assume can reuse it
        UASSERT_OBJ(nodep->is_خالص(), lhsp, "Malformed LHS in NBA");
        // Now have been converted to use the captured values
        أرجع lhsp;
    }

    فراغ جمع_کړه_CFunc_Cached_ارزښتونه(ثابت AstCFunc* ثابت cfuncp,
                              std::unordered_سېټ<ثابت AstCFunc*>& لیدل_شوی) {
        إذا (!لیدل_شوی.ورننباسه(cfuncp).دوهم) أرجع;
        CFuncCache& ارزښت = m_cfuncsCache(cfuncp);
        m_timingDomains.ورننباسه(ارزښت.m_timingDomains.begin(), ارزښت.m_timingDomains.پای_ټکی());
        m_non_Inlined_CAwaits_With_Sen_ونه += ارزښت.m_timingDomains.اندازه();
        لكل (ثابت AstCFunc* ثابت includedp : ارزښت.m_includes) {
            جمع_کړه_CFunc_Cached_ارزښتونه(includedp, لیدل_شوی);
        }
    }

    // Create a temporary variable in the given 'scopep', with the given 'name', and with 'dtypep'
    // type, with the bits selected by 'sLsbp'/'sWidthp' set to 'valuep', other bits set to zero.
    // Insert new statements before 'insertp'.
    // Returns a read reference to the temporary variable.
    AstVarRef* createWidened(File_کرښه* flp, AstScope* scopep, Ast_غوټه_DType* dtypep,
                             Ast_غوټه_بیان* sLsbp, صحيح s_پلنوالی, ثابت std::string& name,
                             Ast_غوټه_بیان* valuep, Ast_غوټه* insertp) {
        // Create temporary variable.
        AstVarScope* ثابت tp = m_dlyTmps.make(flp, scopep, dtypep, name);
        // Zero it
        AstConst* ثابت zerop = جديد AstConst{flp, AstConst::DTyped{}, dtypep};
        zerop->عدد().سېټ_All_Bits0();
        insertp->جمع_کړه_دلته_This_As_بل(
            جديد Ast_ټاکل{flp, جديد AstVarRef{flp, tp, VAccess::WRITE}, zerop});
        // Set the selected bits to 'valuep'
        AstSel* ثابت selp = جديد AstSel{flp, جديد AstVarRef{flp, tp, VAccess::WRITE},
                                        sLsbp->clone_ونه_خالص(ریښتیا), s_پلنوالی};
        insertp->جمع_کړه_دلته_This_As_بل(جديد Ast_ټاکل{flp, selp, valuep});
        // This is the expression to get the value of the temporary
        أرجع جديد AstVarRef{flp, tp, VAccess::READ};
    }

    // Scheme::ShadowVar
    فراغ prepareSchemeShadowVar(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        File_کرښه* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Create the shadow variable
        ثابت std::string name = "_" + vscp->varp()->shortName();
        AstVarScope* ثابت shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), name);
        vscpInfo.shadowVariableKit().vscp = shadowVscp;
        // Mark both for V3LifePsot
        vscp->optimizeLifePost(ریښتیا);
        shadowVscp->optimizeLifePost(ریښتیا);
        // Create the AstActive for the Pre/Post logic
        Ast_فعال* ثابت activep = جديد Ast_فعال{flp, "nba-shadow-variable", vscpInfo.senTreep()};
        activep->sen_ونه_Storep(vscpInfo.senTreep());
        scopep->جمع_کړه_Blocksp(activep);
        // Add 'Pre' scheduled 'shadowVariable = originalVariable' assignment
        AstAlwaysPre* ثابت prep = جديد AstAlwaysPre{flp};
        activep->جمع_کړه_Stmtsp(prep);
        prep->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, جديد AstVarRef{flp, shadowVscp, VAccess::WRITE},
                                      جديد AstVarRef{flp, vscp, VAccess::READ}});
        // Add 'Post' scheduled 'originalVariable = shadowVariable' assignment
        AstAlwaysPost* ثابت postp = جديد AstAlwaysPost{flp};
        activep->جمع_کړه_Stmtsp(postp);
        postp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, جديد AstVarRef{flp, vscp, VAccess::WRITE},
                                       جديد AstVarRef{flp, shadowVscp, VAccess::READ}});
    }
    فراغ convertSchemeShadowVar(Ast_ټاکل_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        AstVarScope* ثابت shadowVscp = vscpInfo.shadowVariableKit().vscp;

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ثابت refp) {
            إذا (!refp->access().isWriteOnly()) أرجع;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
    }

    // Scheme::ShadowVarMasked
    فراغ prepareSchemeShadowVarMasked(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        File_کرښه* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Create the shadow variable
        ثابت std::string shadowName = "_" + vscp->varp()->shortName();
        AstVarScope* ثابت shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), shadowName);
        vscpInfo.shadowVarMaskedKit().vscp = shadowVscp;
        // Create the makk variable
        ثابت std::string ماسک_Name = "Mask__" + vscp->varp()->shortName();
        AstVarScope* ثابت ماسک_Vscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), ماسک_Name);
        ماسک_Vscp->varp()->سېټ_Ignore_Post_Write();
        vscpInfo.shadowVarMaskedKit().maskp = ماسک_Vscp;
        // Create the AstActive for the Post logic
        Ast_فعال* ثابت activep
            = جديد Ast_فعال{flp, "nba-shadow-var-masked", vscpInfo.senTreep()};
        activep->sen_ونه_Storep(vscpInfo.senTreep());
        scopep->جمع_کړه_Blocksp(activep);
        // Add 'Post' scheduled process for the commit and mask clear
        AstAlwaysPost* ثابت postp = جديد AstAlwaysPost{flp};
        activep->جمع_کړه_Stmtsp(postp);
        // Add the commit - vscp = (shadowVscp & maskVscp) | (vscp & ~maskVscp);
        postp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{
            flp, جديد AstVarRef{flp, vscp, VAccess::WRITE},
            جديد AstOr{flp,
                      جديد AstAnd{flp, جديد AstVarRef{flp, shadowVscp, VAccess::READ},
                                 جديد AstVarRef{flp, ماسک_Vscp, VAccess::READ}},
                      جديد AstAnd{flp, جديد AstVarRef{flp, vscp, VAccess::READ},
                                 جديد AstNot{flp, جديد AstVarRef{flp, ماسک_Vscp, VAccess::READ}}}}});
        vscp->varp()->سېټ_Ignore_Post_Read();
        // Clar the mask - maskVscp = '0;
        postp->جمع_کړه_Stmtsp(
            جديد Ast_ټاکل{flp, جديد AstVarRef{flp, ماسک_Vscp, VAccess::WRITE},
                          جديد AstConst{flp, AstConst::Widthed_ارزښت{}, ماسک_Vscp->پلنوالی(), 0}});
    }
    فراغ convertSchemeShadowVarMasked(Ast_ټاکل_Dly* nodep, AstVarScope* vscp,
                                      VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        AstVarScope* ثابت shadowVscp = vscpInfo.shadowVarMaskedKit().vscp;
        AstVarScope* ثابت ماسک_Vscp = vscpInfo.shadowVarMaskedKit().maskp;

        Ast_غوټه_بیان* lhsClonep = nodep->lhsp()->clone_ونه(خطأ);

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ثابت refp) {
            إذا (!refp->access().isWriteOnly()) أرجع;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
        // Set the same bits in the mask to 1
        lhsClonep->foreach([&](AstVarRef* ثابت refp) {
            إذا (!refp->access().isWriteOnly()) أرجع;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(ماسک_Vscp);
            refp->varp(ماسک_Vscp->varp());
        });
        File_کرښه* ثابت flp = nodep->fileline();
        AstConst* ثابت onesp = جديد AstConst{flp, AstConst::DTyped{}, lhsClonep->dtypep()};
        onesp->عدد().سېټ_All_Bits1();
        nodep->جمع_کړه_بل_دلته(جديد Ast_ټاکل{flp, lhsClonep, onesp});
    }

    // Scheme::FlagShared
    فراغ prepareSchemeFlagShared(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_کرښه* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        Ast_فعال* ثابت activep = جديد Ast_فعال{flp, "nba-flag-shared", vscpInfo.senTreep()};
        activep->sen_ونه_Storep(vscpInfo.senTreep());
        scopep->جمع_کړه_Blocksp(activep);
        vscpInfo.flagSharedKit().activep = activep;
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ثابت postp = جديد AstAlwaysPost{flp};
        activep->جمع_کړه_Stmtsp(postp);
        vscpInfo.flagSharedKit().postp = postp;
        // Initialize
        vscpInfo.flagSharedKit().commitFlagp = تش_اشاره;
        vscpInfo.flagSharedKit().commitIfp = تش_اشاره;
    }
    فراغ convertSchemeFlagShared(Ast_ټاکل_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_کرښه* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ثابت std::string بنسټ_Name = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_غوټه_بیان* ثابت capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlink_Fr_شا(), "Val" + بنسټ_Name);

        // Unlink and capture the LHS reference
        Ast_غوټه_بیان* ثابت capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlink_Fr_شا(), بنسټ_Name);

        // Is this NBA adjacent after the previously processed NBA?
        ثابت منطقي consecutive = nodep == m_بل_Dlyp;
        m_بل_Dlyp = VN_CAST(nodep->nextp(), ټاکل_Dly);

        VarScopeInfo* ثابت prevVscpInfop = consecutive ? &m_vscpInfo(m_prevVscp) : تش_اشاره;

        // We can reuse the flag of the previous assignment if:
        ثابت منطقي reuseTheFlag =
            // Consecutive NBAs
            consecutive
            // ... that use the same scheme
            && prevVscpInfop->m_scheme == Scheme::FlagShared
            // ... and are in the same scope as the target variable
            && scopep == vscp->scopep()
            // ... and share the same overall update domain
            && prevVscpInfop->senTreep()->same_ونه(vscpInfo.senTreep());

        إذا (!reuseTheFlag) {
            // Create new flag
            AstVarScope* ثابت flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + بنسټ_Name);
            // Set the flag at the original NBA
            nodep->جمع_کړه_دلته_This_As_بل(  //
                جديد Ast_ټاکل{flp, جديد AstVarRef{flp, flagVscp, VAccess::WRITE},
                              جديد AstConst{flp, AstConst::بټ_True{}}});
            // Add the 'Pre' scheduled reset for the flag
            AstAlwaysPre* ثابت prep = جديد AstAlwaysPre{flp};
            vscpInfo.flagSharedKit().activep->جمع_کړه_Stmtsp(prep);
            prep->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, جديد AstVarRef{flp, flagVscp, VAccess::WRITE},
                                          جديد AstConst{flp, AstConst::بټ_False{}}});
            // Add the 'Post' scheduled commit
            AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, flagVscp, VAccess::READ}};
            vscpInfo.flagSharedKit().postp->جمع_کړه_Stmtsp(ifp);
            vscpInfo.flagSharedKit().commitFlagp = flagVscp;
            vscpInfo.flagSharedKit().commitIfp = ifp;
        } وإلا {
            إذا (vscp != m_prevVscp) {
                // Different variable, ensure the commit block exists for this variable,
                // can reuse existing one with the same flag, otherwise create a new one.
                AstVarScope* ثابت flagVscp = prevVscpInfop->flagSharedKit().commitFlagp;
                UASSERT_OBJ(flagVscp, nodep, "Commit flag of previous assignment should exist");
                إذا (vscpInfo.flagSharedKit().commitFlagp != flagVscp) {
                    AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, flagVscp, VAccess::READ}};
                    vscpInfo.flagSharedKit().postp->جمع_کړه_Stmtsp(ifp);
                    vscpInfo.flagSharedKit().commitFlagp = flagVscp;
                    vscpInfo.flagSharedKit().commitIfp = ifp;
                }
            } وإلا {
                // Same variable, reuse the commit block of the previous assignment
                vscpInfo.flagSharedKit().commitFlagp = prevVscpInfop->flagSharedKit().commitFlagp;
                vscpInfo.flagSharedKit().commitIfp = prevVscpInfop->flagSharedKit().commitIfp;
            }
            ++m_n_Shared_سېټ_Flags;
        }
        // Commit the captured value to the captured destination
        vscpInfo.flagSharedKit().commitIfp->جمع_کړه_Thensp(
            جديد Ast_ټاکل{flp, capturedLhsp, capturedRhsp});

        // Remember the variable for the next NBA
        m_prevVscp = vscp;

        // Delete original NBA
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }

    // Scheme::FlagUnique
    فراغ prepareSchemeFlagUnique(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_کرښه* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        Ast_فعال* ثابت activep = جديد Ast_فعال{flp, "nba-flag-unique", vscpInfo.senTreep()};
        activep->sen_ونه_Storep(vscpInfo.senTreep());
        scopep->جمع_کړه_Blocksp(activep);
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ثابت postp = جديد AstAlwaysPost{flp};
        activep->جمع_کړه_Stmtsp(postp);
        vscpInfo.flagUniqueKit().postp = postp;
    }
    فراغ convertSchemeFlagUnique(Ast_ټاکل_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_کرښه* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ثابت std::string بنسټ_Name = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_غوټه_بیان* ثابت capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlink_Fr_شا(), "Val" + بنسټ_Name);

        // Unlink and capture the LHS reference
        Ast_غوټه_بیان* ثابت capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlink_Fr_شا(), بنسټ_Name);

        // Create new flag
        AstVarScope* ثابت flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + بنسټ_Name);
        flagVscp->varp()->سېټ_Ignore_Post_Write();
        // Set the flag at the original NBA
        nodep->جمع_کړه_دلته_This_As_بل(  //
            جديد Ast_ټاکل{flp, جديد AstVarRef{flp, flagVscp, VAccess::WRITE},
                          جديد AstConst{flp, AstConst::بټ_True{}}});
        // Add the 'Post' scheduled commit
        AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, flagVscp, VAccess::READ}};
        vscpInfo.flagUniqueKit().postp->جمع_کړه_Stmtsp(ifp);
        // Immediately clear the flag
        ifp->جمع_کړه_Thensp(جديد Ast_ټاکل{flp, جديد AstVarRef{flp, flagVscp, VAccess::WRITE},
                                     جديد AstConst{flp, AstConst::بټ_False{}}});
        // Commit the value
        ifp->جمع_کړه_Thensp(جديد Ast_ټاکل{flp, capturedLhsp, capturedRhsp});

        // Delete original NBA
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }

    // Scheme::ValueQueuePartial/Scheme::ValueQueueWhole
    کينډۍ <منطقي N_Partial>
    فراغ prepare_Scheme_ارزښت_کتار(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(N_Partial ? vscpInfo.m_scheme == Scheme::ارزښت_کتار_Partial
                              : vscpInfo.m_scheme == Scheme::ارزښت_کتار_ټولیز,
                    vscp, "Inconsistent NBA scheme");
        File_کرښه* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = vscp->scopep();

        // Create the commit queue variable
        Ast_غوټه_DType* ثابت elemDTypep = vscp->dtypep()->skipRefp();
        Ast_NBACommit_کتار_DType*& cqDTypep = m_cqDTypeps[{elemDTypep, N_Partial}];
        إذا (!cqDTypep) {
            cqDTypep = جديد Ast_NBACommit_کتار_DType{flp, elemDTypep, N_Partial};
            v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(cqDTypep);
        }
        ثابت std::string name = "CommitQueue__" + vscp->varp()->shortName();
        AstVarScope* ثابت کتار_Vscp = m_dlyTmps.make(flp, scopep, cqDTypep, name);
        کتار_Vscp->varp()->noReset(ریښتیا);
        کتار_Vscp->varp()->سېټ_Ignore_Post_Write();
        vscpInfo.ارزښت_کتار_Kit().vscp = کتار_Vscp;
        // Create the AstActive for the Post logic
        Ast_فعال* ثابت activep
            = جديد Ast_فعال{flp, "nba-value-queue-whole", vscpInfo.senTreep()};
        activep->sen_ونه_Storep(vscpInfo.senTreep());
        scopep->جمع_کړه_Blocksp(activep);
        // Add 'Post' scheduled process for the commit
        AstAlwaysPost* ثابت postp = جديد AstAlwaysPost{flp};
        activep->جمع_کړه_Stmtsp(postp);
        // Add the commit
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, کتار_Vscp, VAccess::READWRITE}, VCMethod::NBA_COMMIT};
        callp->dtype_سېټ_Void();
        // TODO: this is a partial update, so must be READWRITE, but that breaks scheduling
        callp->جمع_کړه_Pinsp(جديد AstVarRef{flp, vscp, VAccess::WRITE});
        postp->جمع_کړه_Stmtsp(callp->makeStmt());
    }

    فراغ convert_Scheme_ارزښت_کتار(Ast_ټاکل_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo,
                                 منطقي partial) {
        UASSERT_OBJ(partial ? vscpInfo.m_scheme == Scheme::ارزښت_کتار_Partial
                            : vscpInfo.m_scheme == Scheme::ارزښت_کتار_ټولیز,
                    vscp, "Inconsistent NBA scheme");
        File_کرښه* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ثابت std::string بنسټ_Name = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_غوټه_بیان* ثابت capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlink_Fr_شا(), "Val" + بنسټ_Name);

        // Unlink and capture the LHS reference
        Ast_غوټه_بیان* ثابت capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlink_Fr_شا(), بنسټ_Name);

        // RHS value (can be widened/masked iff Partial)
        Ast_غوټه_بیان* valuep = capturedRhsp;
        // RHS mask (iff Partial)
        Ast_غوټه_بیان* maskp = تش_اشاره;

        // Running node for LHS deconstruction
        Ast_غوټه_بیان* lhsNodep = capturedLhsp;

        // If partial updates are required, construct the mask and the widened value
        إذا (partial) {
            // Type of array element
            Ast_غوټه_DType* ثابت eDTypep = [&]() -> Ast_غوټه_DType* {
                Ast_غوټه_DType* dtypep = vscp->dtypep()->skipRefp();
                طالما (AstUnpackArrayDType* ثابت ap = VN_CAST(dtypep, UnpackArrayDType)) {
                    dtypep = ap->subDTypep()->skipRefp();
                }
                أرجع dtypep;
            }();

            إذا (ثابت AstSel* ثابت lSelp = VN_CAST(lhsNodep, Sel)) {
                // This is a partial assignment.
                // Need to create a mask and widen the value to element size.
                lhsNodep = lSelp->fromp();
                Ast_غوټه_بیان* ثابت sLsbp = lSelp->lsbp();
                ثابت صحيح s_پلنوالی = lSelp->پلنوالی_Const();

                // Create mask value
                maskp = [&]() -> Ast_غوټه_بیان* {
                    // Constant mask we can compute here
                    إذا (ثابت AstConst* ثابت cLsbp = VN_CAST(sLsbp, Const)) {
                        AstConst* ثابت cp = جديد AstConst{flp, AstConst::DTyped{}, eDTypep};
                        cp->عدد().سېټ_ماسک(s_پلنوالی, cLsbp->منزل_SInt());
                        أرجع cp;
                    }

                    // A non-constant mask we must compute at run-time.
                    AstConst* ثابت onesp = جديد AstConst{flp, AstConst::Widthed_ارزښت{}, s_پلنوالی, 0};
                    onesp->عدد().سېټ_All_Bits1();
                    أرجع createWidened(flp, scopep, eDTypep, sLsbp, s_پلنوالی, "Mask" + بنسټ_Name,
                                         onesp, nodep);
                }();

                // Adjust value to element size
                valuep = [&]() -> Ast_غوټه_بیان* {
                    // Constant value with constant select we can compute here
                    إذا (AstConst* ثابت cValuep = VN_CAST(valuep, Const)) {
                        إذا (ثابت AstConst* ثابت cLsbp = VN_CAST(sLsbp, Const)) {
                            AstConst* ثابت cp = جديد AstConst{flp, AstConst::DTyped{}, eDTypep};
                            cp->عدد().سېټ_All_Bits0();
                            cp->عدد().opSelInto(cValuep->عدد(), cLsbp->منزل_SInt(), s_پلنوالی);
                            VL_DO_DANGLING(valuep->delete_ونه(), valuep);
                            أرجع cp;
                        }
                    }

                    // A non-constant value we must adjust.
                    أرجع createWidened(flp, scopep, eDTypep, sLsbp, s_پلنوالی,  //
                                         "Elem" + بنسټ_Name, valuep, nodep);
                }();
            } وإلا {
                // If this assignment is not partial, set mask to ones and we are done
                AstConst* ثابت ones = جديد AstConst{flp, AstConst::DTyped{}, eDTypep};
                ones->عدد().سېټ_All_Bits1();
                maskp = ones;
            }
        }

        // Extract array indices
        std::ویکټور<Ast_غوټه_بیان*> idxps;
        {
            UASSERT_OBJ(VN_IS(lhsNodep, ArraySel), lhsNodep, "Unexpected LHS form");
            طالما (AstArraySel* ثابت aSelp = VN_CAST(lhsNodep, ArraySel)) {
                idxps.emplace_شا(aSelp->bitp()->unlink_Fr_شا());
                lhsNodep = aSelp->fromp();
            }
            UASSERT_OBJ(VN_IS(lhsNodep, VarRef), lhsNodep, "Unexpected LHS form");
            std::سرچپه(idxps.begin(), idxps.پای_ټکی());
        }

        // Done with the LHS at this point
        VL_DO_DANGLING(ټېل_Deletep(capturedLhsp), capturedLhsp);

        // Enqueue the update at the site of the original NBA
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, vscpInfo.ارزښت_کتار_Kit().vscp, VAccess::READWRITE},
            VCMethod::NBA_کتار_ته_ورزیاتول};
        callp->dtype_سېټ_Void();
        callp->جمع_کړه_Pinsp(valuep);
        إذا (partial) callp->جمع_کړه_Pinsp(maskp);
        لكل (Ast_غوټه_بیان* ثابت indexp : idxps) callp->جمع_کړه_Pinsp(indexp);
        nodep->جمع_کړه_دلته_This_As_بل(callp->makeStmt());

        // Delete original NBA
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }

    // Record where a variable is assigned
    فراغ ریکارډ_Write_Ref(AstVarRef* nodep, منطقي nonBlocking) {
        // Ignore references in certain contexts
        إذا (m_ignoreBlkAndNBlk) أرجع;
        // Ignore if it's an array
        // TODO: we do this because it used to be the previous behaviour.
        //       Is it still required, or should we warn for arrays as well?
        //       Scheduling is no different for them...
        //       Clarification: This is OK for arrays of primitive types, but
        //       arrays that use the ShadowVar scheme don't work...
        إذا (VN_IS(nodep->varScopep()->dtypep()->skipRefp(), UnpackArrayDType)) أرجع;

        m_writeRefs(nodep->varScopep()).emplace_شا(nodep, nonBlocking, m_دننه_Non_Comb_Logic);
    }

    کينډۍ <ډول_نوم Procedure_T>
    ساكن منطقي isProcedureWithSentreep(ثابت Ast_غوټه_Procedure* ثابت nodep) {
        ثابت Procedure_T* ثابت procedurep = Ast_غوټه::cast<Procedure_T>(nodep);
        أرجع procedurep && procedurep->sentreep();
    }

    // Visit AstCFunc from a AstNodeCCall - this is made into a separate quasi-visitor because
    // AstCFunc that is not called from the code (e.g.: DPI exports) does not need to be visited
    // this way. Also, not visiting such AstCFuncs allows to avoid caching results for them which
    // this function does - which could lead to excessive memory usage
    فراغ لیدنه_Called_CFunc(AstCFunc* ثابت nodep) {
        CFuncCache& ارزښت = m_cfuncsCache(nodep);
        بدل (ارزښت.m_state) {
        حالة CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_timingDomains);

            // Visit
            ارزښت.m_state = CFuncCache::VISITING;
            m_call_ډېری.ټېل_شا(nodep);
            {
                VL_RESTORER(m_cfuncp);
                m_cfuncp = nodep;
                iterateChildren(nodep);
            }
            m_call_ډېری.راوباسه_شا();
            ارزښت.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::بدلون(m_timingDomains, ارزښت.m_timingDomains);
        } اكسر;
        حالة CFuncCache::VISITING: {
            لكل (اندازه_t i = m_call_ډېری.اندازه() - 1; m_call_ډېری.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ډېری[i]).m_includes.ورننباسه(nodep);
            }
            أرجع;  // Break recursion
        }
        حالة CFuncCache::INITIALIZED: اكسر;
        }
        std::unordered_سېټ<ثابت AstCFunc*> لیدل_شوی;
        جمع_کړه_CFunc_Cached_ارزښتونه(nodep, لیدل_شوی);
    }

    // VISITORS
    فراغ لیدنه(AstNetlist* nodep) override {
        iterateChildren(nodep);
        // Decide which scheme to use for each variable and do the 'prepare' step
        لكل (AstVarScope* ثابت vscp : m_vscps) {
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            vscpInfo.m_scheme = ټاکنه_Scheme(vscp, vscpInfo);
            // Run 'prepare' step
            بدل (vscpInfo.m_scheme) {
            حالة Scheme::Undecided:  // LCOV_EXCL_START
                UASSERT_OBJ(خطأ, vscp, "Failed to choose NBA scheme");
                اكسر;  // LCOV_EXCL_STOP
            حالة Scheme::Unsupported_Compound_Array_دننه_Loop: {
                // Will report error at the site of the NBA
                اكسر;
            }
            حالة Scheme::ShadowVar: {
                ++m_nSchemeShadowVar;
                prepareSchemeShadowVar(vscp, vscpInfo);
                اكسر;
            }
            حالة Scheme::ShadowVarMasked: {
                ++m_nSchemeShadowVarMasked;
                prepareSchemeShadowVarMasked(vscp, vscpInfo);
                اكسر;
            }
            حالة Scheme::FlagShared: {
                ++m_nSchemeFlagShared;
                prepareSchemeFlagShared(vscp, vscpInfo);
                اكسر;
            }
            حالة Scheme::FlagUnique: {
                ++m_nSchemeFlagUnique;
                prepareSchemeFlagUnique(vscp, vscpInfo);
                اكسر;
            }
            حالة Scheme::ارزښت_کتار_ټولیز: {
                ++m_n_Scheme_ارزښت_Queues_ټولیز;
                prepare_Scheme_ارزښت_کتار</* Partial: */ خطأ>(vscp, vscpInfo);
                اكسر;
            }
            حالة Scheme::ارزښت_کتار_Partial: {
                ++m_n_Scheme_ارزښت_Queues_Partial;
                prepare_Scheme_ارزښت_کتار</* Partial: */ ریښتیا>(vscp, vscpInfo);
                اكسر;
            }
            }
        }
        // Convert all NBAs
        لكل (ثابت NBA& nba : m_nbas) {
            Ast_ټاکل_Dly* ثابت nbap = nba.nodep;
            AstVarScope* ثابت vscp = nba.vscp;
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            // Run 'convert' step
            بدل (vscpInfo.m_scheme) {
            حالة Scheme::Undecided: {  // LCOV_EXCL_START
                UASSERT_OBJ(خطأ, vscp, "Unreachable");
                اكسر;
            }  // LCOV_EXCL_STOP
            حالة Scheme::Unsupported_Compound_Array_دننه_Loop: {
                // TODO: make this an E_UNSUPPORTED...
                nbap->v3warn(BLKLOOPINIT, "Unsupported: Non-blocking assignment to array with "
                                          "compound element type inside loop");
                اكسر;
            }
            حالة Scheme::ShadowVar: {
                convertSchemeShadowVar(nbap, vscp, vscpInfo);
                اكسر;
            }
            حالة Scheme::ShadowVarMasked: {
                convertSchemeShadowVarMasked(nbap, vscp, vscpInfo);
                اكسر;
            }
            حالة Scheme::FlagShared: {
                convertSchemeFlagShared(nbap, vscp, vscpInfo);
                اكسر;
            }
            حالة Scheme::FlagUnique: {
                convertSchemeFlagUnique(nbap, vscp, vscpInfo);
                اكسر;
            }
            حالة Scheme::ارزښت_کتار_ټولیز: {
                convert_Scheme_ارزښت_کتار(nbap, vscp, vscpInfo, /* partial: */ خطأ);
                اكسر;
            }
            حالة Scheme::ارزښت_کتار_Partial:
                convert_Scheme_ارزښت_کتار(nbap, vscp, vscpInfo, /* partial: */ ریښتیا);
                اكسر;
            }
        }
    }
    فراغ لیدنه(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_فعال* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ignoreBlkAndNBlk);
        VL_RESTORER(m_دننه_Non_Comb_Logic);
        m_activep = nodep;
        ثابت Ast_Sen_ونه* ثابت senTreep = nodep->sentreep();
        m_ignoreBlkAndNBlk = senTreep->hasStatic() || senTreep->hasInitial();
        m_دننه_Non_Comb_Logic = senTreep->hasClocked();
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override {
        VL_RESTORER(m_needsInitialTrigger);
        VL_RESTORER_CLEAR(m_nbaEventSenTreeps);
        ثابت اندازه_t لومړی_NBAAdded_شاخص = m_nbas.اندازه();
        {
            VL_RESTORER(m_دننه_Suspendable_Or_Fork);
            VL_RESTORER(m_procp);
            VL_RESTORER(m_ignoreBlkAndNBlk);
            VL_RESTORER(m_دننه_Non_Comb_Logic);
            // When we are dealing with initial block we need to
            // treat it as suspendable when we meet a NBA
            m_دننه_Suspendable_Or_Fork = nodep->isSuspendable() || VN_IS(nodep, Initial);
            m_procp = nodep;
            إذا (nodep->isSuspendable()) {
                m_ignoreBlkAndNBlk = خطأ;
                m_دننه_Non_Comb_Logic = ریښتیا;
            }
            iterateChildren(nodep);
        }
        خپلکاری لري_Clocled = [](ثابت Ast_Sen_توکی* itemp) {
            طالما (itemp) {
                إذا (itemp->څنډه_Type().clockedStmt()) أرجع ریښتیا;
                itemp = VN_AS(itemp->nextp(), Sen_توکی);
            }
            أرجع خطأ;
        };
        ثابت منطقي جمع_کړه_Initial_Trigger = m_needsInitialTrigger
                                       && !(isProcedureWithSentreep<AstAlways>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysObserved>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysReactive>(nodep))
                                       && !لري_Clocled(m_activep->sentreep()->sensesp());
        إذا (m_timingDomains.تشه() && !جمع_کړه_Initial_Trigger) أرجع;

        // There were some timing domains involved in the process. Add all of them as sensitivities
        // of all NBA targets in this process. Note this is a bit of a sledgehammer, we should only
        // need those that directly precede the NBA in control flow, but that is hard to compute,
        // so we will hammer away.

        // First gather all senItems
        Ast_Sen_توکی* senItemp = تش_اشاره;
        إذا (جمع_کړه_Initial_Trigger) {
            senItemp = جديد Ast_Sen_توکی{nodep->fileline(), Ast_Sen_توکی::InitialNBA{}};
            ++m_nInitialNBA;
        }

        لكل (ثابت Ast_Sen_ونه* ثابت domainp : m_timingDomains) {
            إذا (domainp->sensesp())
                senItemp = Ast_غوټه::جمع_کړه_بل(senItemp, domainp->sensesp()->clone_ونه(ریښتیا));
        }
        m_timingDomains.clear();
        // Add them to all nba targets we gathered in this process
        لكل (اندازه_t i = لومړی_NBAAdded_شاخص; i < m_nbas.اندازه(); ++i) {
            m_vscpInfo(m_nbas[i].vscp).جمع_کړه_حساسیت(senItemp);
        }
        لكل (Ast_Sen_ونه* ثابت senTreep : m_nbaEventSenTreeps)
            senTreep->جمع_کړه_Sensesp(senItemp->clone_ونه(ریښتیا));
        // Done with these
        VL_DO_DANGLING(senItemp->delete_ونه(), senItemp);
    }
    فراغ لیدنه(AstFork* nodep) override {
        VL_RESTORER(m_دننه_Suspendable_Or_Fork);
        m_دننه_Suspendable_Or_Fork = ریښتیا;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCAwait* nodep) override {
        إذا (nodep->sentreep()) m_timingDomains.ورننباسه(nodep->sentreep());
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstFireEvent* nodep) override {
        UASSERT_OBJ(v3Global.hasEvents(), nodep, "Inconsistent");
        File_کرښه* ثابت flp = nodep->fileline();

        Ast_غوټه_بیان* ثابت eventp = nodep->operandp()->unlink_Fr_شا();

        // Enqueue for clearing 'triggered' state on next eval
        AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
        cstmtp->جمع_کړه("vlSymsp->fireEvent(");
        cstmtp->جمع_کړه(eventp);
        cstmtp->جمع_کړه(");");

        Ast_غوټه* newp = cstmtp;
        ثابت AstVarRef* ثابت vrefp = VN_CAST(eventp, VarRef);
        إذا (nodep->isDelayed() && (m_cfuncp || !vrefp)) {
            // V3Timing converts these with --timing
            nodep->v3warn(E_NOTIMING, "Nonblocking event trigger "
                                          << (m_cfuncp ? "in a non-inlined function/task"
                                                       : "of a class or interface member or "
                                                         "array element")
                                          << " requires --timing");
        } وإلا إذا (nodep->isDelayed()) {
            ثابت std::string newvarname = "_" + vrefp->varp()->shortName();
            AstVarScope* ثابت dlyvscp
                = m_dlyTmps.make(flp, vrefp->varScopep()->scopep(), 1, newvarname);

            ثابت خپلکاری dlyRef = [=](VAccess access) {  //
                أرجع جديد AstVarRef{flp, dlyvscp, access};
            };

            AstAlwaysPost* ثابت postp = جديد AstAlwaysPost{flp};
            AstIf* ثابت ifp = جديد AstIf{flp, dlyRef(VAccess::READ)};
            postp->جمع_کړه_Stmtsp(ifp);

            UASSERT_OBJ(m_activep, nodep, "No active to handle FireEvent");
            Ast_Sen_ونه* senTreep = m_activep->sentreep();
            Ast_فعال* activep = تش_اشاره;
            إذا (m_دننه_Suspendable_Or_Fork && (senTreep->hasInitial() || senTreep->hasClocked())) {
                // Suspendable code can trigger whenever it resumes, so fire when its NBAs are
                // committed (see visit(AstNodeProcedure*)), clearing the flag like
                // Scheme::FlagUnique
                senTreep = جديد Ast_Sen_ونه{
                    flp, senTreep->hasClocked() ? senTreep->sensesp()->clone_ونه(ریښتیا) : تش_اشاره};
                m_nbaEventSenTreeps.ټېل_شا(senTreep);
                m_needsInitialTrigger |= m_timingDomains.تشه();
                dlyvscp->varp()->سېټ_Ignore_Post_Write();
                ifp->جمع_کړه_Thensp(جديد Ast_ټاکل{flp, dlyRef(VAccess::WRITE),
                                             جديد AstConst{flp, AstConst::بټ_False{}}});
                activep = جديد Ast_فعال{flp, "nba-event", senTreep};
                activep->sen_ونه_Storep(senTreep);
            } وإلا {
                activep = جديد Ast_فعال{flp, "nba-event", senTreep};
                AstAlwaysPre* ثابت prep = جديد AstAlwaysPre{flp};
                prep->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, dlyRef(VAccess::WRITE),
                                              جديد AstConst{flp, AstConst::بټ_False{}}});
                activep->جمع_کړه_Stmtsp(prep);
            }
            ifp->جمع_کړه_Thensp(newp);
            m_activep->جمع_کړه_بل_دلته(activep);
            activep->جمع_کړه_Stmtsp(postp);

            newp = جديد Ast_ټاکل{flp, dlyRef(VAccess::WRITE),
                                 جديد AstConst{flp, AstConst::بټ_True{}}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(Ast_ټاکل_Dly* nodep) override {
        // Prevent double processing due to AstExprStmt being moved before this node
        إذا (nodep->user1_سېټ_Once()) أرجع;

        إذا (m_cfuncp) {
            إذا (!v3Global.rootp()->nbaEventp()) {
                nodep->v3warn(
                    E_NOTIMING,
                    "Delayed assignment in a non-inlined function/task requires --timing");
            }
            أرجع;
        }
        UASSERT_OBJ(m_procp, nodep, "Delayed assignment not under process");
        UASSERT_OBJ(m_activep, nodep, "<= not under sensitivity block");
        UASSERT_OBJ(m_scopep, nodep, "<= not under scope");
        UASSERT_OBJ(m_دننه_Suspendable_Or_Fork || m_activep->hasClocked(), nodep,
                    "<= assignment in non-clocked block, should have been converted in V3Active");

        m_needsInitialTrigger |= m_timingDomains.تشه();

        // Record scope of this NBA
        nodep->user2p(m_scopep);

        // Grab the reference to the target of the NBA, also lift ExprStmt statements on the LHS
        VL_RESTORER(m_currNbaLhsRefp);
        UASSERT_OBJ(!m_currNbaLhsRefp, nodep, "NBAs should not nest");
        nodep->lhsp()->foreach([&](Ast_غوټه* currp) {
            // cppcheck-suppress constVariablePointer
            إذا (Ast_بیان_Stmt* ثابت exprp = VN_CAST(currp, بیان_Stmt)) {
                // Move statements before the NBA
                nodep->جمع_کړه_دلته_This_As_بل(exprp->stmtsp()->unlink_Fr_شا_With_بل());
                // Replace with result
                currp->replaceWith(exprp->resultp()->unlink_Fr_شا());
                // Get rid of the AstExprStmt
                VL_DO_DANGLING2(ټېل_Deletep(currp), currp, exprp);
            } وإلا إذا (AstVarRef* ثابت refp = VN_CAST(currp, VarRef)) {
                // Ignore reads (e.g.: '_[*here*] <= _')
                إذا (refp->access().isReadOnly()) أرجع;
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
        إذا (!vscpInfo.m_لومړی_Nba_Refp) {
            vscpInfo.m_لومړی_Nba_Refp = m_currNbaLhsRefp;
            vscpInfo.m_fistActivep = m_activep;
            m_vscps.emplace_شا(vscp);
        }
        // Note usage context
        vscpInfo.m_ټولیز |= VN_IS(nodep->lhsp(), VarRef);
        vscpInfo.m_partial |= VN_IS(nodep->lhsp(), Sel);
        vscpInfo.m_دننه_Loop |= m_دننه_Loop;
        vscpInfo.m_دننه_Susp_Or_Fork |= m_دننه_Suspendable_Or_Fork;
        // Sensitivity might be non-clocked, in a suspendable process, which are handled elsewhere
        إذا (m_activep->sentreep()->hasClocked()) {
            إذا (vscpInfo.m_fistActivep != m_activep) {
                AstVar* ثابت varp = vscp->varp();
                إذا (!varp->user1_سېټ_Once()
                    && !varp->fileline()->warnIsOff(V3_Error_کوډ::MULTIDRIVEN)) {
                    varp->v3warn(MULTIDRIVEN,
                                 "Signal has multiple driving blocks with different clocking: "
                                     << varp->prettyNameQ() << '\n'
                                     << vscpInfo.m_لومړی_Nba_Refp->warnOther()
                                     << "... Location of first driving block\n"
                                     << vscpInfo.m_لومړی_Nba_Refp->warnContextSecondary()
                                     << m_currNbaLhsRefp->warnOther()
                                     << "... Location of other driving block\n"
                                     << m_currNbaLhsRefp->warnContextPrimary() << '\n');
                }
            }
            // Add this sensitivity to the variable
            vscpInfo.جمع_کړه_حساسیت(m_activep->sentreep());
        }

        // Record the NBA for later processing
        m_nbas.emplace_شا();
        NBA& nba = m_nbas.شا();
        nba.nodep = nodep;
        nba.vscp = vscp;

        // Record write reference
        ریکارډ_Write_Ref(m_currNbaLhsRefp, ریښتیا);

        iterateChildren(nodep);
    }
    فراغ لیدنه(AstVarRef* nodep) override {
        // Already checked the NBA LHS ref, ignore here
        إذا (nodep == m_currNbaLhsRefp) أرجع;
        // Only care about write refs
        إذا (!nodep->access().isWriteOrRW()) أرجع;
        // Record write reference
        ریکارډ_Write_Ref(nodep, خطأ);
    }
    فراغ لیدنه(AstLoop* nodep) override {
        VL_RESTORER(m_دننه_Loop);
        m_دننه_Loop = ریښتیا;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_CCall* ثابت nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ثابت خپلکاری& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        إذا (cfuncps.تشه()) {
            لیدنه_Called_CFunc(nodep->funcp());
        } وإلا {
            لكل (AstCFunc* ثابت cfuncp : cfuncps) لیدنه_Called_CFunc(cfuncp);
        }
    }
    فراغ لیدنه(AstCFunc* ثابت nodep) override {
        ثابت خپلکاری& ارزښت = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        إذا (ارزښت.m_state != CFuncCache::UNINITIALIZED) أرجع;
        VL_RESTORER(m_cfuncp);
        // Its timing domains are those of the processes calling it, see visitCalledCFunc()
        VL_RESTORER_CLEAR(m_timingDomains);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }

    // Pre/Post logic are created here and their content need no further changes, so ignore.
    فراغ لیدنه(AstAlwaysPre*) override {}
    فراغ لیدنه(AstAlwaysPost*) override {}

    //--------------------
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند DelayedVisitor(AstNetlist* nodep)
        : m_classGraphp{V3_Class_ګراف::build(nodep)} {
        iterate(nodep);
    }
    ~DelayedVisitor() override {
        V3Stats::جمع_کړه_Stat("NBA, variables using ShadowVar scheme", m_nSchemeShadowVar);
        V3Stats::جمع_کړه_Stat("NBA, variables using ShadowVarMasked scheme", m_nSchemeShadowVarMasked);
        V3Stats::جمع_کړه_Stat("NBA, variables using FlagShared scheme", m_nSchemeFlagShared);
        V3Stats::جمع_کړه_Stat("NBA, variables using FlagUnique scheme", m_nSchemeFlagUnique);
        V3Stats::جمع_کړه_Stat("NBA, variables using ValueQueueWhole scheme", m_n_Scheme_ارزښت_Queues_ټولیز);
        V3Stats::جمع_کړه_Stat("NBA, variables using ValueQueuePartial scheme",
                         m_n_Scheme_ارزښت_Queues_Partial);
        V3Stats::جمع_کړه_Stat("Optimizations, NBA flags shared", m_n_Shared_سېټ_Flags);
        V3Stats::جمع_کړه_Stat("Procedures needing initial NBA trigger", m_nInitialNBA);
        V3Stats::جمع_کړه_Stat("Non-inlined co_awaits with SenTree", m_non_Inlined_CAwaits_With_Sen_ونه);
    }
};

//######################################################################
// Delayed class functions

فراغ V3Delayed::delayedAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DelayedVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("delayed", 0, dump_ونه_Either_Level() >= 3);
}
