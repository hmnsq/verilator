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

جماعت DelayedVisitor final : عوامی VNVisitor {
    // TYPES

    // The various NBA conversion schemes, including error cases
    شمار جماعت Scheme : uint8_t {
        Undecided = 0,
        Unsupported_Compound_Array_اندر_Loop,
        ShadowVar,
        ShadowVarMasked,
        FlagShared,
        FlagUnique,
        قدر_قطار_پورا,
        قدر_قطار_Partial
    };

    // All info associated with a variable that is the target of an NBA
    جماعت VarScopeInfo final {
    عوامی:
        // First write reference encountered to the VarScope as the target on an NBA
        ثابت AstVarRef* m_پہلا_Nba_Refp = خالی_اشاریہ;
        // Active block 'm_firstNbaRefp' is under
        ثابت Ast_فعال* m_fistActivep = خالی_اشاریہ;
        منطقی m_پورا = جھوٹ;  // Used on LHS of NBA directly via VarRef
        منطقی m_partial = جھوٹ;  // Used on LHS of NBA under a Sel
        منطقی m_اندر_Loop = جھوٹ;  // Used on LHS of NBA in a loop
        منطقی m_اندر_Susp_Or_Fork = جھوٹ;  // Used on LHS of NBA in suspendable process or fork
        Scheme m_scheme = Scheme::Undecided;  // Conversion scheme to use for this variable

    نجی:
        // Combined sensitivities of all NBAs targeting this variable
        Ast_Sen_درخت* m_senTreep = خالی_اشاریہ;

        // Union of stuff needed for the various schemes - use accessors below!
        اتحاد {
            ساخت {  // Stuff needed for Scheme::ShadowVar
                AstVarScope* vscp;  // The shadow variable
            } m_shadowVariableKit;
            ساخت {  // Stuff needed for Scheme::ShadowVarMasked
                AstVarScope* vscp;  // The shadow variable
                AstVarScope* maskp;  // The mask variable
            } m_shadowVarMaskedKit;
            ساخت {  // Stuff needed for Scheme::FlagShared
                Ast_فعال* activep;  // The active block for the Pre/Post logic
                AstAlwaysPost* postp;  // The post block for commiting results
                AstVarScope* commitFlagp;  // The commit flag variable, for reuse
                AstIf* commitIfp;  // The previous if statement for committing, for reuse
            } m_flagSharedKit;
            ساخت {  // Stuff needed for Scheme::FlagUnique
                AstAlwaysPost* postp;  // The post block for commiting results
            } m_flagUniqueKit;
            ساخت {  // Stuff needed for Scheme::ValueQueueWhole/Scheme::ValueQueuePartial
                AstVarScope* vscp;  // The commit queue variable
            } m_قدر_قطار_Kit;
        } m_kitUnion;

    عوامی:
        VarScopeInfo() = طےشدہ;
        ~VarScopeInfo() {
            // Might not be linked if there was an error
            اگر (!m_senTreep->backp()) VL_DO_DANGLING(m_senTreep->delete_درخت(), m_senTreep);
        }
        // Accessors for the above union fields
        خودکار& shadowVariableKit() {
            UASSERT(m_scheme == Scheme::ShadowVar, "Inconsistent Scheme");
            واپس m_kitUnion.m_shadowVariableKit;
        }
        خودکار& shadowVarMaskedKit() {
            UASSERT(m_scheme == Scheme::ShadowVarMasked, "Inconsistent Scheme");
            واپس m_kitUnion.m_shadowVarMaskedKit;
        }
        خودکار& flagSharedKit() {
            UASSERT(m_scheme == Scheme::FlagShared, "Inconsistent Scheme");
            واپس m_kitUnion.m_flagSharedKit;
        }
        خودکار& flagUniqueKit() {
            UASSERT(m_scheme == Scheme::FlagUnique, "Inconsistent Scheme");
            واپس m_kitUnion.m_flagUniqueKit;
        }
        خودکار& قدر_قطار_Kit() {
            UASSERT(m_scheme == Scheme::قدر_قطار_Partial || m_scheme == Scheme::قدر_قطار_پورا,
                    "Inconsistent Scheme");
            واپس m_kitUnion.m_قدر_قطار_Kit;
        }

        // Accessor
        Ast_Sen_درخت* senTreep() ثابت { واپس m_senTreep; }

        // Add sensitivities
        خالی جوڑو_حساسیت(Ast_Sen_شے* nodep) {
            اگر (!m_senTreep) m_senTreep = نیا Ast_Sen_درخت{nodep->fileline(), خالی_اشاریہ};
            // Add a copy of each term
            m_senTreep->جوڑو_Sensesp(nodep->clone_درخت(سچ));
            // Remove duplicates
            V3Const::constifyExpensiveEdit(m_senTreep);
        }
        // cppcheck-suppress constParameterPointer
        خالی جوڑو_حساسیت(Ast_Sen_درخت* nodep) { جوڑو_حساسیت(nodep->sensesp()); }
    };

    // Data structure to keep track of all writes to
    ساخت WriteReference final {
        AstVarRef* m_refp = خالی_اشاریہ;  // The reference
        منطقی m_isNBA = جھوٹ;  // True if an NBA write
        منطقی m_اندر_Non_Comb = جھوٹ;  // True if reference is known to be in non-combinational logic
        WriteReference() = طےشدہ;
        WriteReference(AstVarRef* refp, منطقی isNBA, منطقی اندر_Non_Comb)
            : m_refp{refp}
            , m_isNBA{isNBA}
            , m_اندر_Non_Comb{اندر_Non_Comb} {}
    };

    // Data required to lower AstAssignDelay later
    ساخت NBA final {
        Ast_تفویض_Dly* nodep = خالی_اشاریہ;  // The NBA this record refers to
        AstVarScope* vscp = خالی_اشاریہ;  // The target variable the NBA is updating
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
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;
    ثابت VNUser2_اندر_Use m_user2_اندر_Use;
    ثابت VNUser3_اندر_Use m_user3_اندر_Use;

    ساخت CFuncCache final {
        VInsertion_سیٹ<Ast_Sen_درخت*> m_timingDomains;  // What shall be added to m_timingDomains
        std::سیٹ<AstCFunc*>
            m_includes;  // CFuncs whose CFuncCache shall be included into this - this is used to
                         // break cycles: A->B->A (instead of visiting A while it is still begin
                         // visited B just marks that it includes A)
        شمار State : uint8_t {
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
    AstUser3Allocator<AstVarScope, std::سمتیہ<WriteReference>> m_writeRefs;

    // STATE - across all visitors
    VInsertion_سیٹ<Ast_Sen_درخت*> m_timingDomains;  // Timing resume domains
    V3SharedTmps m_dlyTmps{"__Vdly", VVarType::BLOCKTEMP};  // Temporary variables
    // Commit queue data types, by element type and partial flag. Shared by all instances,
    // as m_dlyTmps only shares variables with the same data type.
    std::map<std::pair<ثابت Ast_گرہ_DType*, منطقی>, Ast_NBACommit_قطار_DType*> m_cqDTypeps;

    ثابت std::unique_ptr<V3_Class_گراف>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::سمتیہ<ثابت AstCFunc*> m_call_ڈھیر;  // Current callstack of AstCFuncs

    // STATE - for current visit position (use VL_RESTORER)
    Ast_فعال* m_activep = خالی_اشاریہ;  // Current activate
    ثابت AstCFunc* m_cfuncp = خالی_اشاریہ;  // Current public C Function
    Ast_گرہ_Procedure* m_procp = خالی_اشاریہ;  // Current process
    AstScope* m_scopep = خالی_اشاریہ;  // Current scope
    منطقی m_اندر_Loop = جھوٹ;  // True in for loops
    منطقی m_اندر_Suspendable_Or_Fork = جھوٹ;  // True in suspendable processes and forks
    منطقی m_ignoreBlkAndNBlk = جھوٹ;  // Suppress delayed assignment BLKANDNBLK
    منطقی m_اندر_Non_Comb_Logic = جھوٹ;  // We are in non-combinational logic
    منطقی m_needsInitialTrigger = جھوٹ;  // Whether a NodeProcedure needs a initial trigger
    std::سمتیہ<Ast_Sen_درخت*> m_nbaEventSenTreeps;  // Sensitivities of '->>' in the process
    AstVarRef* m_currNbaLhsRefp = خالی_اشاریہ;  // Current NBA LHS variable reference

    // STATE - during NBA conversion (after visit)
    std::سمتیہ<NBA> m_nbas;  // AstAssignDly instances to lower at the end
    std::سمتیہ<AstVarScope*> m_vscps;  // Target variables on LHSs of NBAs
    Ast_تفویض_Dly* m_اگلا_Dlyp = خالی_اشاریہ;  // The nextp of the previous AstAssignDly
    AstVarScope* m_پچھلا_Vscp = خالی_اشاریہ;  // The target of the previous AstAssignDly

    // STATE - Statistic tracking
    VDouble0 m_nSchemeShadowVar;  // Number of variables using Scheme::ShadowVar
    VDouble0 m_nSchemeShadowVarMasked;  // Number of variabels using Scheme::ShadowVarMasked
    VDouble0 m_nSchemeFlagShared;  // Number of variables using Scheme::FlagShared
    VDouble0 m_nSchemeFlagUnique;  // Number of variables using Scheme::FlagUnique
    VDouble0 m_n_Scheme_قدر_Queues_پورا;  //  Number of variables using Scheme::ValueQueueWhole
    VDouble0 m_n_Scheme_قدر_Queues_Partial;  //  Number of variables using Scheme::ValueQueuePartial
    VDouble0 m_n_Shared_سیٹ_Flags;  // "Set" flags actually shared by Scheme::FlagShared variables
    VDouble0 m_nInitialNBA;  // Number of procedural blocks with initial NBA
    VDouble0 m_non_Inlined_CAwaits_With_Sen_درخت;  // Count uses of not inlined co_awaits

    // METHODS

    // Return true iff a variable is assigned by both blocking and nonblocking
    // assignments. Issue BLKANDNBLK error if we can't prove the mixed
    // assignments are to independent bits and the blocking assignment can be
    // in combinational logic, which is something we can't safely implement
    // still.
    منطقی checkMixedUsage(ثابت AstVarScope* vscp, منطقی isIntegralOrPacked) {

        ساخت Ref final {
            AstVarRef* m_refp;  // The reference
            منطقی m_اندر_Non_Comb;  // True if known to be in non-combinational logic
            صحیح m_lsb;  // LSB of accessed range
            صحیح m_msb;  // MSB of accessed range
            Ref(AstVarRef* refp, منطقی اندر_Non_Comb, صحیح lsb, صحیح msb)
                : m_refp{refp}
                , m_اندر_Non_Comb{اندر_Non_Comb}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };

        std::سمتیہ<Ref> blkRefs;  // Blocking writes
        std::سمتیہ<Ref> nbaRefs;  // Non-blockign writes

        ثابت صحیح چوڑائی = isIntegralOrPacked ? vscp->چوڑائی() : 1;

        برائے (ثابت خودکار& writeRef : m_writeRefs(vscp)) {
            صحیح lsb = 0;
            صحیح msb = چوڑائی - 1;
            اگر (ثابت AstSel* ثابت selp = VN_CAST(writeRef.m_refp->backp(), Sel)) {
                اگر (VN_IS(selp->lsbp(), Const)) {
                    lsb = selp->lsbConst();
                    msb = selp->msbConst();
                }
            }
            اگر (writeRef.m_isNBA) {
                nbaRefs.emplace_عقب(writeRef.m_refp, writeRef.m_اندر_Non_Comb, lsb, msb);
            } ورنہ {
                blkRefs.emplace_عقب(writeRef.m_refp, writeRef.m_اندر_Non_Comb, lsb, msb);
            }
        }
        // We only run this function on targets of NBAs, so there should be at least one...
        UASSERT_OBJ(!nbaRefs.خالی_خانے(), vscp, "Did not record NBA write");
        // If no blocking upadte, then we are good
        اگر (blkRefs.خالی_خانے()) واپس جھوٹ;

        // If the blocking assignment is in non-combinational logic (i.e.:
        // in logic that has an explicit trigger), then we can safely
        // implement it (there is no race between clocked logic and post
        // scheduled logic), so need not error
        blkRefs.erase(std::remove_if(blkRefs.begin(), blkRefs.انتہا(),
                                     [](ثابت Ref& ref) { واپس ref.m_اندر_Non_Comb; }),
                      blkRefs.انتہا());

        // If nothing left, then we need not error
        اگر (blkRefs.خالی_خانے()) واپس سچ;

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
            واپس سچ;
        }

        // We need to error if we can't prove the written bits are independent

        // Sort refs by interval
        ثابت خودکار lessThanRef = [](ثابت Ref& a, ثابت Ref& b) {
            اگر (a.m_lsb != b.m_lsb) واپس a.m_lsb < b.m_lsb;
            واپس a.m_msb < b.m_msb;
        };
        std::stable_ترتیب_دو(blkRefs.begin(), blkRefs.انتہا(), lessThanRef);
        std::stable_ترتیب_دو(nbaRefs.begin(), nbaRefs.انتہا(), lessThanRef);
        // Iterate both vectors, checking for overlap
        خودکار bIt = blkRefs.begin();
        خودکار nIt = nbaRefs.begin();
        جب (bIt != blkRefs.انتہا() && nIt != nbaRefs.انتہا()) {
            اگر (lessThanRef(*bIt, *nIt)) {
                اگر (nIt->m_lsb <= bIt->m_msb) توڑو;  // Stop on Overlap
                ++bIt;
            } ورنہ {
                اگر (bIt->m_lsb <= nIt->m_msb) توڑو;  // Stop on Overlap
                ++nIt;
            }
        }

        // If we found an overlapping range that cannot be safely implemented, then wran...
        اگر (bIt != blkRefs.انتہا() && nIt != nbaRefs.انتہا()) {
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

        واپس سچ;
    }

    // Choose the NBA scheme used for the given variable.
    Scheme انتخاب_Scheme(ثابت AstVarScope* vscp, ثابت VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::Undecided, vscp, "NBA scheme already decided");

        ثابت Ast_گرہ_DType* ثابت dtypep = vscp->dtypep()->skipRefp();
        // Unpacked arrays
        اگر (ثابت AstUnpackArrayDType* ثابت uaDTypep = VN_CAST(dtypep, UnpackArrayDType)) {
            // If whole array is target of NBA, use ShadowVar, but not in a suspendable process or
            // fork, which resume outside the 'nba' region, so its 'pre' logic would overwrite the
            // value they write in the shadow variable
            اگر (vscpInfo.m_پورا) {
                واپس vscpInfo.m_اندر_Susp_Or_Fork ? Scheme::FlagUnique : Scheme::ShadowVar;
            }
            // Basic underlying type of elements, if any.
            ثابت AstBasicDType* ثابت basicp = uaDTypep->basicp();
            // If used in a loop, we must have a dynamic commit queue. (Also works in suspendables)
            اگر (vscpInfo.m_اندر_Loop) {
                // Arrays with compound element types are currently not supported in loops
                اگر (!basicp
                    || !(basicp->isIntegralOrPacked()  //
                         || basicp->isDouble()  //
                         || basicp->isString())) {
                    واپس Scheme::Unsupported_Compound_Array_اندر_Loop;
                }
                اگر (vscpInfo.m_partial) واپس Scheme::قدر_قطار_Partial;
                واپس Scheme::قدر_قطار_پورا;
            }
            // A suspendable process or fork can resume outside the 'nba' region, so the 'pre'
            // logic of the other schemes would undo its updates. The unique flag scheme has none.
            اگر (vscpInfo.m_اندر_Susp_Or_Fork) واپس Scheme::FlagUnique;
            // Otherwise if an array of packed/basic elements, use the shared flag scheme
            اگر (basicp) واپس Scheme::FlagShared;
            // Finally fall back on the shadow variable scheme, e.g. for
            // arrays of unpacked structs. This will be slow.
            // TODO: generic LHS scheme as discussed in #5092
            واپس Scheme::ShadowVar;
        }

        // Likewise, the 'pre' logic of the shadow variable scheme would undo the updates of a
        // suspendable process or fork, so use the unique flag scheme
        اگر (vscpInfo.m_اندر_Susp_Or_Fork) واپس Scheme::FlagUnique;

        ثابت منطقی isIntegralOrPacked = dtypep->isIntegralOrPacked();
        // Check for mixed usage (this also warns if not OK)
        اگر (checkMixedUsage(vscp, isIntegralOrPacked)) {
            // If it's a variable updated by both blocking and non-blocking
            // assignments, use the ShadowVarMasked schem if masked update is
            // possible. This can handle blocking and non-blocking updates to
            // inpdendent parts correctly at run-time, and always works, even
            // in loops or other dynamic context.
            اگر (isIntegralOrPacked) واپس Scheme::ShadowVarMasked;
            // If it's inside a loop, use Scheme::ShadowVar, which is safe,
            // but will generate incorrect code if a partial update is used
            اگر (vscpInfo.m_اندر_Loop) واپس Scheme::ShadowVar;
            // Otherwise (for not packed variables), use the FlagUnique scheme,
            // which at least handles partial updates correctly, but might break
            // in loops or other dynamic context
            واپس Scheme::FlagUnique;
        }

        // Otherwise use the simple shadow variable scheme
        واپس Scheme::ShadowVar;
    }

    // Given an expression 'exprp', return a new expression that always evaluates to the
    // value of the given expression at this point in the program. That is:
    // - If given a non-constant expression, create a new temporary AstVarScope under the given
    //   'scopep', with the given 'name', assign the expression to it, and return a read reference
    //   to the new AstVarScope.
    // - If given a constant, just return that constant.
    // New statements are inserted before 'insertp'
    Ast_گرہ_اظہاریہ* captureVal(AstScope* ثابت scopep, Ast_گرہ_Stmt* ثابت insertp,
                            Ast_گرہ_اظہاریہ* ثابت exprp, ثابت std::string& name) {
        UASSERT_OBJ(!exprp->backp(), exprp, "Should have been unlinked");
        File_سطر* ثابت flp = exprp->fileline();
        اگر (VN_IS(exprp, Const)) واپس exprp;
        // TODO: there are some const variables that could be simply referenced here
        AstVarScope* ثابت tmpVscp = m_dlyTmps.make(flp, scopep, exprp->dtypep(), name);
        insertp->جوڑو_یہاں_This_As_اگلا(
            نیا Ast_تفویض{flp, نیا AstVarRef{flp, tmpVscp, VAccess::WRITE}, exprp});
        واپس نیا AstVarRef{flp, tmpVscp, VAccess::READ};
    };

    // Similar to 'captureVal', but captures an LValue expression. That is, the returned
    // expression will reference the same location as the input expression, at this point in the
    // program.
    Ast_گرہ_اظہاریہ* captureLhs(AstScope* ثابت scopep, Ast_گرہ_Stmt* ثابت insertp,
                            Ast_گرہ_اظہاریہ* ثابت lhsp, ثابت std::string& بنیاد_Name) {
        UASSERT_OBJ(!lhsp->backp(), lhsp, "Should have been unlinked");
        // Running node pointer
        Ast_گرہ_اظہاریہ* nodep = lhsp;
        // Capture AstSel indices - there should be only one
        اگر (AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            ثابت std::string tmpName{"Lsb" + بنیاد_Name};
            selp->lsbp(captureVal(scopep, insertp, selp->lsbp()->unlink_Fr_عقب(), tmpName));
            // Continue with target
            nodep = selp->fromp();
        }
        UASSERT_OBJ(!VN_IS(nodep, Sel), lhsp, "Multiple 'AstSel' applied to LHS reference");
        // Capture the indices of selects - might be many, also below struct members. If a handle
        // selects the instance the target is a member of, capture the handle instead, as it is
        // only read, to select the target when the NBA is executed (IEEE 1800-2023 10.4.2).
        سائز_t nArraySels = 0;
        جب (سچ) {
            اگر (Ast_گرہ_Sel* ثابت selp = VN_CAST(nodep, گرہ_Sel)) {
                ثابت std::string tmpName{"Dim" + std::مقصد_string(nArraySels++) + بنیاد_Name};
                selp->bitp(captureVal(scopep, insertp, selp->bitp()->unlink_Fr_عقب(), tmpName));
                nodep = selp->fromp();
            } ورنہ اگر (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
                nodep = selp->fromp();
            } ورنہ اگر (AstMemberSel* ثابت selp = VN_CAST(nodep, MemberSel)) {
                Ast_گرہ_اظہاریہ* ثابت handlep = selp->fromp()->unlink_Fr_عقب();
                V3LinkLValue::link_LValue_سیٹ(handlep, VAccess::READ);
                AstVarRef* ثابت refp
                    = VN_AS(captureVal(scopep, insertp, handlep, "Handle" + بنیاد_Name), VarRef);
                refp->varp()->سیٹ_Ignore_Post_Write();
                // Like the handle it replaces, mark it written, as is the member it selects
                refp->access(selp->access());
                selp->fromp(refp);
                واپس lhsp;
            } ورنہ {
                توڑو;
            }
        }
        // What remains must be the variable, or some sort of select, we assume can reuse it
        UASSERT_OBJ(nodep->is_خالص(), lhsp, "Malformed LHS in NBA");
        // Now have been converted to use the captured values
        واپس lhsp;
    }

    خالی جوڑو_CFunc_Cached_اقدار(ثابت AstCFunc* ثابت cfuncp,
                              std::unordered_سیٹ<ثابت AstCFunc*>& دیکھا) {
        اگر (!دیکھا.داخل_کرو(cfuncp).دوسرا) واپس;
        CFuncCache& قدر = m_cfuncsCache(cfuncp);
        m_timingDomains.داخل_کرو(قدر.m_timingDomains.begin(), قدر.m_timingDomains.انتہا());
        m_non_Inlined_CAwaits_With_Sen_درخت += قدر.m_timingDomains.سائز();
        برائے (ثابت AstCFunc* ثابت includedp : قدر.m_includes) {
            جوڑو_CFunc_Cached_اقدار(includedp, دیکھا);
        }
    }

    // Create a temporary variable in the given 'scopep', with the given 'name', and with 'dtypep'
    // type, with the bits selected by 'sLsbp'/'sWidthp' set to 'valuep', other bits set to zero.
    // Insert new statements before 'insertp'.
    // Returns a read reference to the temporary variable.
    AstVarRef* createWidened(File_سطر* flp, AstScope* scopep, Ast_گرہ_DType* dtypep,
                             Ast_گرہ_اظہاریہ* sLsbp, صحیح s_چوڑائی, ثابت std::string& name,
                             Ast_گرہ_اظہاریہ* valuep, Ast_گرہ* insertp) {
        // Create temporary variable.
        AstVarScope* ثابت tp = m_dlyTmps.make(flp, scopep, dtypep, name);
        // Zero it
        AstConst* ثابت zerop = نیا AstConst{flp, AstConst::DTyped{}, dtypep};
        zerop->عدد().سیٹ_All_Bits0();
        insertp->جوڑو_یہاں_This_As_اگلا(
            نیا Ast_تفویض{flp, نیا AstVarRef{flp, tp, VAccess::WRITE}, zerop});
        // Set the selected bits to 'valuep'
        AstSel* ثابت selp = نیا AstSel{flp, نیا AstVarRef{flp, tp, VAccess::WRITE},
                                        sLsbp->clone_درخت_خالص(سچ), s_چوڑائی};
        insertp->جوڑو_یہاں_This_As_اگلا(نیا Ast_تفویض{flp, selp, valuep});
        // This is the expression to get the value of the temporary
        واپس نیا AstVarRef{flp, tp, VAccess::READ};
    }

    // Scheme::ShadowVar
    خالی prepareSchemeShadowVar(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Create the shadow variable
        ثابت std::string name = "_" + vscp->varp()->shortName();
        AstVarScope* ثابت shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), name);
        vscpInfo.shadowVariableKit().vscp = shadowVscp;
        // Mark both for V3LifePsot
        vscp->optimizeLifePost(سچ);
        shadowVscp->optimizeLifePost(سچ);
        // Create the AstActive for the Pre/Post logic
        Ast_فعال* ثابت activep = نیا Ast_فعال{flp, "nba-shadow-variable", vscpInfo.senTreep()};
        activep->sen_درخت_Storep(vscpInfo.senTreep());
        scopep->جوڑو_Blocksp(activep);
        // Add 'Pre' scheduled 'shadowVariable = originalVariable' assignment
        AstAlwaysPre* ثابت prep = نیا AstAlwaysPre{flp};
        activep->جوڑو_Stmtsp(prep);
        prep->جوڑو_Stmtsp(نیا Ast_تفویض{flp, نیا AstVarRef{flp, shadowVscp, VAccess::WRITE},
                                      نیا AstVarRef{flp, vscp, VAccess::READ}});
        // Add 'Post' scheduled 'originalVariable = shadowVariable' assignment
        AstAlwaysPost* ثابت postp = نیا AstAlwaysPost{flp};
        activep->جوڑو_Stmtsp(postp);
        postp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, نیا AstVarRef{flp, vscp, VAccess::WRITE},
                                       نیا AstVarRef{flp, shadowVscp, VAccess::READ}});
    }
    خالی convertSchemeShadowVar(Ast_تفویض_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVar, vscp, "Inconsistent NBA scheme");
        AstVarScope* ثابت shadowVscp = vscpInfo.shadowVariableKit().vscp;

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ثابت refp) {
            اگر (!refp->access().isWriteOnly()) واپس;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
    }

    // Scheme::ShadowVarMasked
    خالی prepareSchemeShadowVarMasked(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Create the shadow variable
        ثابت std::string shadowName = "_" + vscp->varp()->shortName();
        AstVarScope* ثابت shadowVscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), shadowName);
        vscpInfo.shadowVarMaskedKit().vscp = shadowVscp;
        // Create the makk variable
        ثابت std::string نقاب_Name = "Mask__" + vscp->varp()->shortName();
        AstVarScope* ثابت نقاب_Vscp = m_dlyTmps.make(flp, scopep, vscp->dtypep(), نقاب_Name);
        نقاب_Vscp->varp()->سیٹ_Ignore_Post_Write();
        vscpInfo.shadowVarMaskedKit().maskp = نقاب_Vscp;
        // Create the AstActive for the Post logic
        Ast_فعال* ثابت activep
            = نیا Ast_فعال{flp, "nba-shadow-var-masked", vscpInfo.senTreep()};
        activep->sen_درخت_Storep(vscpInfo.senTreep());
        scopep->جوڑو_Blocksp(activep);
        // Add 'Post' scheduled process for the commit and mask clear
        AstAlwaysPost* ثابت postp = نیا AstAlwaysPost{flp};
        activep->جوڑو_Stmtsp(postp);
        // Add the commit - vscp = (shadowVscp & maskVscp) | (vscp & ~maskVscp);
        postp->جوڑو_Stmtsp(نیا Ast_تفویض{
            flp, نیا AstVarRef{flp, vscp, VAccess::WRITE},
            نیا AstOr{flp,
                      نیا AstAnd{flp, نیا AstVarRef{flp, shadowVscp, VAccess::READ},
                                 نیا AstVarRef{flp, نقاب_Vscp, VAccess::READ}},
                      نیا AstAnd{flp, نیا AstVarRef{flp, vscp, VAccess::READ},
                                 نیا AstNot{flp, نیا AstVarRef{flp, نقاب_Vscp, VAccess::READ}}}}});
        vscp->varp()->سیٹ_Ignore_Post_Read();
        // Clar the mask - maskVscp = '0;
        postp->جوڑو_Stmtsp(
            نیا Ast_تفویض{flp, نیا AstVarRef{flp, نقاب_Vscp, VAccess::WRITE},
                          نیا AstConst{flp, AstConst::Widthed_قدر{}, نقاب_Vscp->چوڑائی(), 0}});
    }
    خالی convertSchemeShadowVarMasked(Ast_تفویض_Dly* nodep, AstVarScope* vscp,
                                      VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::ShadowVarMasked, vscp, "Inconsistent NBA scheme");
        AstVarScope* ثابت shadowVscp = vscpInfo.shadowVarMaskedKit().vscp;
        AstVarScope* ثابت نقاب_Vscp = vscpInfo.shadowVarMaskedKit().maskp;

        Ast_گرہ_اظہاریہ* lhsClonep = nodep->lhsp()->clone_درخت(جھوٹ);

        // Replace the write ref on the LHS with the shadow variable
        nodep->lhsp()->foreach([&](AstVarRef* ثابت refp) {
            اگر (!refp->access().isWriteOnly()) واپس;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(shadowVscp);
            refp->varp(shadowVscp->varp());
        });
        // Set the same bits in the mask to 1
        lhsClonep->foreach([&](AstVarRef* ثابت refp) {
            اگر (!refp->access().isWriteOnly()) واپس;
            UASSERT_OBJ(refp->varScopep() == vscp, nodep, "NBA not setting expected variable");
            refp->varScopep(نقاب_Vscp);
            refp->varp(نقاب_Vscp->varp());
        });
        File_سطر* ثابت flp = nodep->fileline();
        AstConst* ثابت onesp = نیا AstConst{flp, AstConst::DTyped{}, lhsClonep->dtypep()};
        onesp->عدد().سیٹ_All_Bits1();
        nodep->جوڑو_اگلا_یہاں(نیا Ast_تفویض{flp, lhsClonep, onesp});
    }

    // Scheme::FlagShared
    خالی prepareSchemeFlagShared(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        Ast_فعال* ثابت activep = نیا Ast_فعال{flp, "nba-flag-shared", vscpInfo.senTreep()};
        activep->sen_درخت_Storep(vscpInfo.senTreep());
        scopep->جوڑو_Blocksp(activep);
        vscpInfo.flagSharedKit().activep = activep;
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ثابت postp = نیا AstAlwaysPost{flp};
        activep->جوڑو_Stmtsp(postp);
        vscpInfo.flagSharedKit().postp = postp;
        // Initialize
        vscpInfo.flagSharedKit().commitFlagp = خالی_اشاریہ;
        vscpInfo.flagSharedKit().commitIfp = خالی_اشاریہ;
    }
    خالی convertSchemeFlagShared(Ast_تفویض_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagShared, vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ثابت std::string بنیاد_Name = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_گرہ_اظہاریہ* ثابت capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlink_Fr_عقب(), "Val" + بنیاد_Name);

        // Unlink and capture the LHS reference
        Ast_گرہ_اظہاریہ* ثابت capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlink_Fr_عقب(), بنیاد_Name);

        // Is this NBA adjacent after the previously processed NBA?
        ثابت منطقی consecutive = nodep == m_اگلا_Dlyp;
        m_اگلا_Dlyp = VN_CAST(nodep->nextp(), تفویض_Dly);

        VarScopeInfo* ثابت پچھلا_Vscp_Infop = consecutive ? &m_vscpInfo(m_پچھلا_Vscp) : خالی_اشاریہ;

        // We can reuse the flag of the previous assignment if:
        ثابت منطقی reuseTheFlag =
            // Consecutive NBAs
            consecutive
            // ... that use the same scheme
            && پچھلا_Vscp_Infop->m_scheme == Scheme::FlagShared
            // ... and are in the same scope as the target variable
            && scopep == vscp->scopep()
            // ... and share the same overall update domain
            && پچھلا_Vscp_Infop->senTreep()->same_درخت(vscpInfo.senTreep());

        اگر (!reuseTheFlag) {
            // Create new flag
            AstVarScope* ثابت flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + بنیاد_Name);
            // Set the flag at the original NBA
            nodep->جوڑو_یہاں_This_As_اگلا(  //
                نیا Ast_تفویض{flp, نیا AstVarRef{flp, flagVscp, VAccess::WRITE},
                              نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}});
            // Add the 'Pre' scheduled reset for the flag
            AstAlwaysPre* ثابت prep = نیا AstAlwaysPre{flp};
            vscpInfo.flagSharedKit().activep->جوڑو_Stmtsp(prep);
            prep->جوڑو_Stmtsp(نیا Ast_تفویض{flp, نیا AstVarRef{flp, flagVscp, VAccess::WRITE},
                                          نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}});
            // Add the 'Post' scheduled commit
            AstIf* ثابت ifp = نیا AstIf{flp, نیا AstVarRef{flp, flagVscp, VAccess::READ}};
            vscpInfo.flagSharedKit().postp->جوڑو_Stmtsp(ifp);
            vscpInfo.flagSharedKit().commitFlagp = flagVscp;
            vscpInfo.flagSharedKit().commitIfp = ifp;
        } ورنہ {
            اگر (vscp != m_پچھلا_Vscp) {
                // Different variable, ensure the commit block exists for this variable,
                // can reuse existing one with the same flag, otherwise create a new one.
                AstVarScope* ثابت flagVscp = پچھلا_Vscp_Infop->flagSharedKit().commitFlagp;
                UASSERT_OBJ(flagVscp, nodep, "Commit flag of previous assignment should exist");
                اگر (vscpInfo.flagSharedKit().commitFlagp != flagVscp) {
                    AstIf* ثابت ifp = نیا AstIf{flp, نیا AstVarRef{flp, flagVscp, VAccess::READ}};
                    vscpInfo.flagSharedKit().postp->جوڑو_Stmtsp(ifp);
                    vscpInfo.flagSharedKit().commitFlagp = flagVscp;
                    vscpInfo.flagSharedKit().commitIfp = ifp;
                }
            } ورنہ {
                // Same variable, reuse the commit block of the previous assignment
                vscpInfo.flagSharedKit().commitFlagp = پچھلا_Vscp_Infop->flagSharedKit().commitFlagp;
                vscpInfo.flagSharedKit().commitIfp = پچھلا_Vscp_Infop->flagSharedKit().commitIfp;
            }
            ++m_n_Shared_سیٹ_Flags;
        }
        // Commit the captured value to the captured destination
        vscpInfo.flagSharedKit().commitIfp->جوڑو_Thensp(
            نیا Ast_تفویض{flp, capturedLhsp, capturedRhsp});

        // Remember the variable for the next NBA
        m_پچھلا_Vscp = vscp;

        // Delete original NBA
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }

    // Scheme::FlagUnique
    خالی prepareSchemeFlagUnique(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Create the AstActive for the Pre/Post logic
        Ast_فعال* ثابت activep = نیا Ast_فعال{flp, "nba-flag-unique", vscpInfo.senTreep()};
        activep->sen_درخت_Storep(vscpInfo.senTreep());
        scopep->جوڑو_Blocksp(activep);
        // Add 'Post' scheduled process to be populated later
        AstAlwaysPost* ثابت postp = نیا AstAlwaysPost{flp};
        activep->جوڑو_Stmtsp(postp);
        vscpInfo.flagUniqueKit().postp = postp;
    }
    خالی convertSchemeFlagUnique(Ast_تفویض_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(vscpInfo.m_scheme == Scheme::FlagUnique, vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ثابت std::string بنیاد_Name = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_گرہ_اظہاریہ* ثابت capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlink_Fr_عقب(), "Val" + بنیاد_Name);

        // Unlink and capture the LHS reference
        Ast_گرہ_اظہاریہ* ثابت capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlink_Fr_عقب(), بنیاد_Name);

        // Create new flag
        AstVarScope* ثابت flagVscp = m_dlyTmps.make(flp, scopep, 1, "Set" + بنیاد_Name);
        flagVscp->varp()->سیٹ_Ignore_Post_Write();
        // Set the flag at the original NBA
        nodep->جوڑو_یہاں_This_As_اگلا(  //
            نیا Ast_تفویض{flp, نیا AstVarRef{flp, flagVscp, VAccess::WRITE},
                          نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}});
        // Add the 'Post' scheduled commit
        AstIf* ثابت ifp = نیا AstIf{flp, نیا AstVarRef{flp, flagVscp, VAccess::READ}};
        vscpInfo.flagUniqueKit().postp->جوڑو_Stmtsp(ifp);
        // Immediately clear the flag
        ifp->جوڑو_Thensp(نیا Ast_تفویض{flp, نیا AstVarRef{flp, flagVscp, VAccess::WRITE},
                                     نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}});
        // Commit the value
        ifp->جوڑو_Thensp(نیا Ast_تفویض{flp, capturedLhsp, capturedRhsp});

        // Delete original NBA
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }

    // Scheme::ValueQueuePartial/Scheme::ValueQueueWhole
    سانچہ <منطقی N_Partial>
    خالی prepare_Scheme_قدر_قطار(AstVarScope* vscp, VarScopeInfo& vscpInfo) {
        UASSERT_OBJ(N_Partial ? vscpInfo.m_scheme == Scheme::قدر_قطار_Partial
                              : vscpInfo.m_scheme == Scheme::قدر_قطار_پورا,
                    vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = vscp->scopep();

        // Create the commit queue variable
        Ast_گرہ_DType* ثابت elemDTypep = vscp->dtypep()->skipRefp();
        Ast_NBACommit_قطار_DType*& cqDTypep = m_cqDTypeps[{elemDTypep, N_Partial}];
        اگر (!cqDTypep) {
            cqDTypep = نیا Ast_NBACommit_قطار_DType{flp, elemDTypep, N_Partial};
            v3Global.rootp()->typeTablep()->جوڑو_Typesp(cqDTypep);
        }
        ثابت std::string name = "CommitQueue__" + vscp->varp()->shortName();
        AstVarScope* ثابت قطار_Vscp = m_dlyTmps.make(flp, scopep, cqDTypep, name);
        قطار_Vscp->varp()->noReset(سچ);
        قطار_Vscp->varp()->سیٹ_Ignore_Post_Write();
        vscpInfo.قدر_قطار_Kit().vscp = قطار_Vscp;
        // Create the AstActive for the Post logic
        Ast_فعال* ثابت activep
            = نیا Ast_فعال{flp, "nba-value-queue-whole", vscpInfo.senTreep()};
        activep->sen_درخت_Storep(vscpInfo.senTreep());
        scopep->جوڑو_Blocksp(activep);
        // Add 'Post' scheduled process for the commit
        AstAlwaysPost* ثابت postp = نیا AstAlwaysPost{flp};
        activep->جوڑو_Stmtsp(postp);
        // Add the commit
        AstCMethodHard* ثابت callp = نیا AstCMethodHard{
            flp, نیا AstVarRef{flp, قطار_Vscp, VAccess::READWRITE}, VCMethod::NBA_COMMIT};
        callp->dtype_سیٹ_Void();
        // TODO: this is a partial update, so must be READWRITE, but that breaks scheduling
        callp->جوڑو_Pinsp(نیا AstVarRef{flp, vscp, VAccess::WRITE});
        postp->جوڑو_Stmtsp(callp->makeStmt());
    }

    خالی convert_Scheme_قدر_قطار(Ast_تفویض_Dly* nodep, AstVarScope* vscp, VarScopeInfo& vscpInfo,
                                 منطقی partial) {
        UASSERT_OBJ(partial ? vscpInfo.m_scheme == Scheme::قدر_قطار_Partial
                            : vscpInfo.m_scheme == Scheme::قدر_قطار_پورا,
                    vscp, "Inconsistent NBA scheme");
        File_سطر* ثابت flp = vscp->fileline();
        AstScope* ثابت scopep = VN_AS(nodep->user2p(), Scope);

        // Base name suffix for signals constructed below
        ثابت std::string بنیاد_Name = "__" + vscp->varp()->shortName();

        // Unlink and capture the RHS value
        Ast_گرہ_اظہاریہ* ثابت capturedRhsp
            = captureVal(scopep, nodep, nodep->rhsp()->unlink_Fr_عقب(), "Val" + بنیاد_Name);

        // Unlink and capture the LHS reference
        Ast_گرہ_اظہاریہ* ثابت capturedLhsp
            = captureLhs(scopep, nodep, nodep->lhsp()->unlink_Fr_عقب(), بنیاد_Name);

        // RHS value (can be widened/masked iff Partial)
        Ast_گرہ_اظہاریہ* valuep = capturedRhsp;
        // RHS mask (iff Partial)
        Ast_گرہ_اظہاریہ* maskp = خالی_اشاریہ;

        // Running node for LHS deconstruction
        Ast_گرہ_اظہاریہ* lhsNodep = capturedLhsp;

        // If partial updates are required, construct the mask and the widened value
        اگر (partial) {
            // Type of array element
            Ast_گرہ_DType* ثابت eDTypep = [&]() -> Ast_گرہ_DType* {
                Ast_گرہ_DType* dtypep = vscp->dtypep()->skipRefp();
                جب (AstUnpackArrayDType* ثابت ap = VN_CAST(dtypep, UnpackArrayDType)) {
                    dtypep = ap->subDTypep()->skipRefp();
                }
                واپس dtypep;
            }();

            اگر (ثابت AstSel* ثابت lSelp = VN_CAST(lhsNodep, Sel)) {
                // This is a partial assignment.
                // Need to create a mask and widen the value to element size.
                lhsNodep = lSelp->fromp();
                Ast_گرہ_اظہاریہ* ثابت sLsbp = lSelp->lsbp();
                ثابت صحیح s_چوڑائی = lSelp->چوڑائی_Const();

                // Create mask value
                maskp = [&]() -> Ast_گرہ_اظہاریہ* {
                    // Constant mask we can compute here
                    اگر (ثابت AstConst* ثابت cLsbp = VN_CAST(sLsbp, Const)) {
                        AstConst* ثابت cp = نیا AstConst{flp, AstConst::DTyped{}, eDTypep};
                        cp->عدد().سیٹ_نقاب(s_چوڑائی, cLsbp->مقصد_SInt());
                        واپس cp;
                    }

                    // A non-constant mask we must compute at run-time.
                    AstConst* ثابت onesp = نیا AstConst{flp, AstConst::Widthed_قدر{}, s_چوڑائی, 0};
                    onesp->عدد().سیٹ_All_Bits1();
                    واپس createWidened(flp, scopep, eDTypep, sLsbp, s_چوڑائی, "Mask" + بنیاد_Name,
                                         onesp, nodep);
                }();

                // Adjust value to element size
                valuep = [&]() -> Ast_گرہ_اظہاریہ* {
                    // Constant value with constant select we can compute here
                    اگر (AstConst* ثابت cValuep = VN_CAST(valuep, Const)) {
                        اگر (ثابت AstConst* ثابت cLsbp = VN_CAST(sLsbp, Const)) {
                            AstConst* ثابت cp = نیا AstConst{flp, AstConst::DTyped{}, eDTypep};
                            cp->عدد().سیٹ_All_Bits0();
                            cp->عدد().opSelInto(cValuep->عدد(), cLsbp->مقصد_SInt(), s_چوڑائی);
                            VL_DO_DANGLING(valuep->delete_درخت(), valuep);
                            واپس cp;
                        }
                    }

                    // A non-constant value we must adjust.
                    واپس createWidened(flp, scopep, eDTypep, sLsbp, s_چوڑائی,  //
                                         "Elem" + بنیاد_Name, valuep, nodep);
                }();
            } ورنہ {
                // If this assignment is not partial, set mask to ones and we are done
                AstConst* ثابت ones = نیا AstConst{flp, AstConst::DTyped{}, eDTypep};
                ones->عدد().سیٹ_All_Bits1();
                maskp = ones;
            }
        }

        // Extract array indices
        std::سمتیہ<Ast_گرہ_اظہاریہ*> idxps;
        {
            UASSERT_OBJ(VN_IS(lhsNodep, ArraySel), lhsNodep, "Unexpected LHS form");
            جب (AstArraySel* ثابت aSelp = VN_CAST(lhsNodep, ArraySel)) {
                idxps.emplace_عقب(aSelp->bitp()->unlink_Fr_عقب());
                lhsNodep = aSelp->fromp();
            }
            UASSERT_OBJ(VN_IS(lhsNodep, VarRef), lhsNodep, "Unexpected LHS form");
            std::الٹ(idxps.begin(), idxps.انتہا());
        }

        // Done with the LHS at this point
        VL_DO_DANGLING(ڈالو_Deletep(capturedLhsp), capturedLhsp);

        // Enqueue the update at the site of the original NBA
        AstCMethodHard* ثابت callp = نیا AstCMethodHard{
            flp, نیا AstVarRef{flp, vscpInfo.قدر_قطار_Kit().vscp, VAccess::READWRITE},
            VCMethod::NBA_قطار_میں_ڈالو};
        callp->dtype_سیٹ_Void();
        callp->جوڑو_Pinsp(valuep);
        اگر (partial) callp->جوڑو_Pinsp(maskp);
        برائے (Ast_گرہ_اظہاریہ* ثابت indexp : idxps) callp->جوڑو_Pinsp(indexp);
        nodep->جوڑو_یہاں_This_As_اگلا(callp->makeStmt());

        // Delete original NBA
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }

    // Record where a variable is assigned
    خالی ریکارڈ_Write_Ref(AstVarRef* nodep, منطقی nonBlocking) {
        // Ignore references in certain contexts
        اگر (m_ignoreBlkAndNBlk) واپس;
        // Ignore if it's an array
        // TODO: we do this because it used to be the previous behaviour.
        //       Is it still required, or should we warn for arrays as well?
        //       Scheduling is no different for them...
        //       Clarification: This is OK for arrays of primitive types, but
        //       arrays that use the ShadowVar scheme don't work...
        اگر (VN_IS(nodep->varScopep()->dtypep()->skipRefp(), UnpackArrayDType)) واپس;

        m_writeRefs(nodep->varScopep()).emplace_عقب(nodep, nonBlocking, m_اندر_Non_Comb_Logic);
    }

    سانچہ <قسم_نام Procedure_T>
    ساکن منطقی isProcedureWithSentreep(ثابت Ast_گرہ_Procedure* ثابت nodep) {
        ثابت Procedure_T* ثابت procedurep = Ast_گرہ::cast<Procedure_T>(nodep);
        واپس procedurep && procedurep->sentreep();
    }

    // Visit AstCFunc from a AstNodeCCall - this is made into a separate quasi-visitor because
    // AstCFunc that is not called from the code (e.g.: DPI exports) does not need to be visited
    // this way. Also, not visiting such AstCFuncs allows to avoid caching results for them which
    // this function does - which could lead to excessive memory usage
    خالی دورہ_Called_CFunc(AstCFunc* ثابت nodep) {
        CFuncCache& قدر = m_cfuncsCache(nodep);
        انتخاب (قدر.m_state) {
        حالت CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_timingDomains);

            // Visit
            قدر.m_state = CFuncCache::VISITING;
            m_call_ڈھیر.ڈالو_عقب(nodep);
            {
                VL_RESTORER(m_cfuncp);
                m_cfuncp = nodep;
                iterateChildren(nodep);
            }
            m_call_ڈھیر.نکالو_عقب();
            قدر.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::تبادلہ(m_timingDomains, قدر.m_timingDomains);
        } توڑو;
        حالت CFuncCache::VISITING: {
            برائے (سائز_t i = m_call_ڈھیر.سائز() - 1; m_call_ڈھیر.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ڈھیر[i]).m_includes.داخل_کرو(nodep);
            }
            واپس;  // Break recursion
        }
        حالت CFuncCache::INITIALIZED: توڑو;
        }
        std::unordered_سیٹ<ثابت AstCFunc*> دیکھا;
        جوڑو_CFunc_Cached_اقدار(nodep, دیکھا);
    }

    // VISITORS
    خالی دورہ(AstNetlist* nodep) override {
        iterateChildren(nodep);
        // Decide which scheme to use for each variable and do the 'prepare' step
        برائے (AstVarScope* ثابت vscp : m_vscps) {
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            vscpInfo.m_scheme = انتخاب_Scheme(vscp, vscpInfo);
            // Run 'prepare' step
            انتخاب (vscpInfo.m_scheme) {
            حالت Scheme::Undecided:  // LCOV_EXCL_START
                UASSERT_OBJ(جھوٹ, vscp, "Failed to choose NBA scheme");
                توڑو;  // LCOV_EXCL_STOP
            حالت Scheme::Unsupported_Compound_Array_اندر_Loop: {
                // Will report error at the site of the NBA
                توڑو;
            }
            حالت Scheme::ShadowVar: {
                ++m_nSchemeShadowVar;
                prepareSchemeShadowVar(vscp, vscpInfo);
                توڑو;
            }
            حالت Scheme::ShadowVarMasked: {
                ++m_nSchemeShadowVarMasked;
                prepareSchemeShadowVarMasked(vscp, vscpInfo);
                توڑو;
            }
            حالت Scheme::FlagShared: {
                ++m_nSchemeFlagShared;
                prepareSchemeFlagShared(vscp, vscpInfo);
                توڑو;
            }
            حالت Scheme::FlagUnique: {
                ++m_nSchemeFlagUnique;
                prepareSchemeFlagUnique(vscp, vscpInfo);
                توڑو;
            }
            حالت Scheme::قدر_قطار_پورا: {
                ++m_n_Scheme_قدر_Queues_پورا;
                prepare_Scheme_قدر_قطار</* Partial: */ جھوٹ>(vscp, vscpInfo);
                توڑو;
            }
            حالت Scheme::قدر_قطار_Partial: {
                ++m_n_Scheme_قدر_Queues_Partial;
                prepare_Scheme_قدر_قطار</* Partial: */ سچ>(vscp, vscpInfo);
                توڑو;
            }
            }
        }
        // Convert all NBAs
        برائے (ثابت NBA& nba : m_nbas) {
            Ast_تفویض_Dly* ثابت nbap = nba.nodep;
            AstVarScope* ثابت vscp = nba.vscp;
            VarScopeInfo& vscpInfo = m_vscpInfo(vscp);
            // Run 'convert' step
            انتخاب (vscpInfo.m_scheme) {
            حالت Scheme::Undecided: {  // LCOV_EXCL_START
                UASSERT_OBJ(جھوٹ, vscp, "Unreachable");
                توڑو;
            }  // LCOV_EXCL_STOP
            حالت Scheme::Unsupported_Compound_Array_اندر_Loop: {
                // TODO: make this an E_UNSUPPORTED...
                nbap->v3warn(BLKLOOPINIT, "Unsupported: Non-blocking assignment to array with "
                                          "compound element type inside loop");
                توڑو;
            }
            حالت Scheme::ShadowVar: {
                convertSchemeShadowVar(nbap, vscp, vscpInfo);
                توڑو;
            }
            حالت Scheme::ShadowVarMasked: {
                convertSchemeShadowVarMasked(nbap, vscp, vscpInfo);
                توڑو;
            }
            حالت Scheme::FlagShared: {
                convertSchemeFlagShared(nbap, vscp, vscpInfo);
                توڑو;
            }
            حالت Scheme::FlagUnique: {
                convertSchemeFlagUnique(nbap, vscp, vscpInfo);
                توڑو;
            }
            حالت Scheme::قدر_قطار_پورا: {
                convert_Scheme_قدر_قطار(nbap, vscp, vscpInfo, /* partial: */ جھوٹ);
                توڑو;
            }
            حالت Scheme::قدر_قطار_Partial:
                convert_Scheme_قدر_قطار(nbap, vscp, vscpInfo, /* partial: */ سچ);
                توڑو;
            }
        }
    }
    خالی دورہ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_فعال* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ignoreBlkAndNBlk);
        VL_RESTORER(m_اندر_Non_Comb_Logic);
        m_activep = nodep;
        ثابت Ast_Sen_درخت* ثابت senTreep = nodep->sentreep();
        m_ignoreBlkAndNBlk = senTreep->hasStatic() || senTreep->hasInitial();
        m_اندر_Non_Comb_Logic = senTreep->hasClocked();
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_Procedure* nodep) override {
        VL_RESTORER(m_needsInitialTrigger);
        VL_RESTORER_CLEAR(m_nbaEventSenTreeps);
        ثابت سائز_t پہلا_NBAAdded_اشاریہ = m_nbas.سائز();
        {
            VL_RESTORER(m_اندر_Suspendable_Or_Fork);
            VL_RESTORER(m_procp);
            VL_RESTORER(m_ignoreBlkAndNBlk);
            VL_RESTORER(m_اندر_Non_Comb_Logic);
            // When we are dealing with initial block we need to
            // treat it as suspendable when we meet a NBA
            m_اندر_Suspendable_Or_Fork = nodep->isSuspendable() || VN_IS(nodep, Initial);
            m_procp = nodep;
            اگر (nodep->isSuspendable()) {
                m_ignoreBlkAndNBlk = جھوٹ;
                m_اندر_Non_Comb_Logic = سچ;
            }
            iterateChildren(nodep);
        }
        خودکار شامل_ہے_Clocled = [](ثابت Ast_Sen_شے* itemp) {
            جب (itemp) {
                اگر (itemp->کنارہ_Type().clockedStmt()) واپس سچ;
                itemp = VN_AS(itemp->nextp(), Sen_شے);
            }
            واپس جھوٹ;
        };
        ثابت منطقی جوڑو_Initial_Trigger = m_needsInitialTrigger
                                       && !(isProcedureWithSentreep<AstAlways>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysObserved>(nodep)
                                            || isProcedureWithSentreep<AstAlwaysReactive>(nodep))
                                       && !شامل_ہے_Clocled(m_activep->sentreep()->sensesp());
        اگر (m_timingDomains.خالی_خانے() && !جوڑو_Initial_Trigger) واپس;

        // There were some timing domains involved in the process. Add all of them as sensitivities
        // of all NBA targets in this process. Note this is a bit of a sledgehammer, we should only
        // need those that directly precede the NBA in control flow, but that is hard to compute,
        // so we will hammer away.

        // First gather all senItems
        Ast_Sen_شے* senItemp = خالی_اشاریہ;
        اگر (جوڑو_Initial_Trigger) {
            senItemp = نیا Ast_Sen_شے{nodep->fileline(), Ast_Sen_شے::InitialNBA{}};
            ++m_nInitialNBA;
        }

        برائے (ثابت Ast_Sen_درخت* ثابت domainp : m_timingDomains) {
            اگر (domainp->sensesp())
                senItemp = Ast_گرہ::جوڑو_اگلا(senItemp, domainp->sensesp()->clone_درخت(سچ));
        }
        m_timingDomains.clear();
        // Add them to all nba targets we gathered in this process
        برائے (سائز_t i = پہلا_NBAAdded_اشاریہ; i < m_nbas.سائز(); ++i) {
            m_vscpInfo(m_nbas[i].vscp).جوڑو_حساسیت(senItemp);
        }
        برائے (Ast_Sen_درخت* ثابت senTreep : m_nbaEventSenTreeps)
            senTreep->جوڑو_Sensesp(senItemp->clone_درخت(سچ));
        // Done with these
        VL_DO_DANGLING(senItemp->delete_درخت(), senItemp);
    }
    خالی دورہ(AstFork* nodep) override {
        VL_RESTORER(m_اندر_Suspendable_Or_Fork);
        m_اندر_Suspendable_Or_Fork = سچ;
        iterateChildren(nodep);
    }
    خالی دورہ(AstCAwait* nodep) override {
        اگر (nodep->sentreep()) m_timingDomains.داخل_کرو(nodep->sentreep());
        iterateChildren(nodep);
    }
    خالی دورہ(AstFireEvent* nodep) override {
        UASSERT_OBJ(v3Global.hasEvents(), nodep, "Inconsistent");
        File_سطر* ثابت flp = nodep->fileline();

        Ast_گرہ_اظہاریہ* ثابت eventp = nodep->operandp()->unlink_Fr_عقب();

        // Enqueue for clearing 'triggered' state on next eval
        AstCStmt* ثابت cstmtp = نیا AstCStmt{flp};
        cstmtp->جوڑو("vlSymsp->fireEvent(");
        cstmtp->جوڑو(eventp);
        cstmtp->جوڑو(");");

        Ast_گرہ* newp = cstmtp;
        ثابت AstVarRef* ثابت vrefp = VN_CAST(eventp, VarRef);
        اگر (nodep->isDelayed() && (m_cfuncp || !vrefp)) {
            // V3Timing converts these with --timing
            nodep->v3warn(E_NOTIMING, "Nonblocking event trigger "
                                          << (m_cfuncp ? "in a non-inlined function/task"
                                                       : "of a class or interface member or "
                                                         "array element")
                                          << " requires --timing");
        } ورنہ اگر (nodep->isDelayed()) {
            ثابت std::string newvarname = "_" + vrefp->varp()->shortName();
            AstVarScope* ثابت dlyvscp
                = m_dlyTmps.make(flp, vrefp->varScopep()->scopep(), 1, newvarname);

            ثابت خودکار dlyRef = [=](VAccess access) {  //
                واپس نیا AstVarRef{flp, dlyvscp, access};
            };

            AstAlwaysPost* ثابت postp = نیا AstAlwaysPost{flp};
            AstIf* ثابت ifp = نیا AstIf{flp, dlyRef(VAccess::READ)};
            postp->جوڑو_Stmtsp(ifp);

            UASSERT_OBJ(m_activep, nodep, "No active to handle FireEvent");
            Ast_Sen_درخت* senTreep = m_activep->sentreep();
            Ast_فعال* activep = خالی_اشاریہ;
            اگر (m_اندر_Suspendable_Or_Fork && (senTreep->hasInitial() || senTreep->hasClocked())) {
                // Suspendable code can trigger whenever it resumes, so fire when its NBAs are
                // committed (see visit(AstNodeProcedure*)), clearing the flag like
                // Scheme::FlagUnique
                senTreep = نیا Ast_Sen_درخت{
                    flp, senTreep->hasClocked() ? senTreep->sensesp()->clone_درخت(سچ) : خالی_اشاریہ};
                m_nbaEventSenTreeps.ڈالو_عقب(senTreep);
                m_needsInitialTrigger |= m_timingDomains.خالی_خانے();
                dlyvscp->varp()->سیٹ_Ignore_Post_Write();
                ifp->جوڑو_Thensp(نیا Ast_تفویض{flp, dlyRef(VAccess::WRITE),
                                             نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}});
                activep = نیا Ast_فعال{flp, "nba-event", senTreep};
                activep->sen_درخت_Storep(senTreep);
            } ورنہ {
                activep = نیا Ast_فعال{flp, "nba-event", senTreep};
                AstAlwaysPre* ثابت prep = نیا AstAlwaysPre{flp};
                prep->جوڑو_Stmtsp(نیا Ast_تفویض{flp, dlyRef(VAccess::WRITE),
                                              نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}});
                activep->جوڑو_Stmtsp(prep);
            }
            ifp->جوڑو_Thensp(newp);
            m_activep->جوڑو_اگلا_یہاں(activep);
            activep->جوڑو_Stmtsp(postp);

            newp = نیا Ast_تفویض{flp, dlyRef(VAccess::WRITE),
                                 نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    خالی دورہ(Ast_تفویض_Dly* nodep) override {
        // Prevent double processing due to AstExprStmt being moved before this node
        اگر (nodep->user1_سیٹ_Once()) واپس;

        اگر (m_cfuncp) {
            اگر (!v3Global.rootp()->nbaEventp()) {
                nodep->v3warn(
                    E_NOTIMING,
                    "Delayed assignment in a non-inlined function/task requires --timing");
            }
            واپس;
        }
        UASSERT_OBJ(m_procp, nodep, "Delayed assignment not under process");
        UASSERT_OBJ(m_activep, nodep, "<= not under sensitivity block");
        UASSERT_OBJ(m_scopep, nodep, "<= not under scope");
        UASSERT_OBJ(m_اندر_Suspendable_Or_Fork || m_activep->hasClocked(), nodep,
                    "<= assignment in non-clocked block, should have been converted in V3Active");

        m_needsInitialTrigger |= m_timingDomains.خالی_خانے();

        // Record scope of this NBA
        nodep->user2p(m_scopep);

        // Grab the reference to the target of the NBA, also lift ExprStmt statements on the LHS
        VL_RESTORER(m_currNbaLhsRefp);
        UASSERT_OBJ(!m_currNbaLhsRefp, nodep, "NBAs should not nest");
        nodep->lhsp()->foreach([&](Ast_گرہ* currp) {
            // cppcheck-suppress constVariablePointer
            اگر (Ast_اظہاریہ_Stmt* ثابت exprp = VN_CAST(currp, اظہاریہ_Stmt)) {
                // Move statements before the NBA
                nodep->جوڑو_یہاں_This_As_اگلا(exprp->stmtsp()->unlink_Fr_عقب_With_اگلا());
                // Replace with result
                currp->replaceWith(exprp->resultp()->unlink_Fr_عقب());
                // Get rid of the AstExprStmt
                VL_DO_DANGLING2(ڈالو_Deletep(currp), currp, exprp);
            } ورنہ اگر (AstVarRef* ثابت refp = VN_CAST(currp, VarRef)) {
                // Ignore reads (e.g.: '_[*here*] <= _')
                اگر (refp->access().isReadOnly()) واپس;
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
        اگر (!vscpInfo.m_پہلا_Nba_Refp) {
            vscpInfo.m_پہلا_Nba_Refp = m_currNbaLhsRefp;
            vscpInfo.m_fistActivep = m_activep;
            m_vscps.emplace_عقب(vscp);
        }
        // Note usage context
        vscpInfo.m_پورا |= VN_IS(nodep->lhsp(), VarRef);
        vscpInfo.m_partial |= VN_IS(nodep->lhsp(), Sel);
        vscpInfo.m_اندر_Loop |= m_اندر_Loop;
        vscpInfo.m_اندر_Susp_Or_Fork |= m_اندر_Suspendable_Or_Fork;
        // Sensitivity might be non-clocked, in a suspendable process, which are handled elsewhere
        اگر (m_activep->sentreep()->hasClocked()) {
            اگر (vscpInfo.m_fistActivep != m_activep) {
                AstVar* ثابت varp = vscp->varp();
                اگر (!varp->user1_سیٹ_Once()
                    && !varp->fileline()->warnIsOff(V3_Error_رمز::MULTIDRIVEN)) {
                    varp->v3warn(MULTIDRIVEN,
                                 "Signal has multiple driving blocks with different clocking: "
                                     << varp->prettyNameQ() << '\n'
                                     << vscpInfo.m_پہلا_Nba_Refp->warnOther()
                                     << "... Location of first driving block\n"
                                     << vscpInfo.m_پہلا_Nba_Refp->warnContextSecondary()
                                     << m_currNbaLhsRefp->warnOther()
                                     << "... Location of other driving block\n"
                                     << m_currNbaLhsRefp->warnContextPrimary() << '\n');
                }
            }
            // Add this sensitivity to the variable
            vscpInfo.جوڑو_حساسیت(m_activep->sentreep());
        }

        // Record the NBA for later processing
        m_nbas.emplace_عقب();
        NBA& nba = m_nbas.عقب();
        nba.nodep = nodep;
        nba.vscp = vscp;

        // Record write reference
        ریکارڈ_Write_Ref(m_currNbaLhsRefp, سچ);

        iterateChildren(nodep);
    }
    خالی دورہ(AstVarRef* nodep) override {
        // Already checked the NBA LHS ref, ignore here
        اگر (nodep == m_currNbaLhsRefp) واپس;
        // Only care about write refs
        اگر (!nodep->access().isWriteOrRW()) واپس;
        // Record write reference
        ریکارڈ_Write_Ref(nodep, جھوٹ);
    }
    خالی دورہ(AstLoop* nodep) override {
        VL_RESTORER(m_اندر_Loop);
        m_اندر_Loop = سچ;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_CCall* ثابت nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ثابت خودکار& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        اگر (cfuncps.خالی_خانے()) {
            دورہ_Called_CFunc(nodep->funcp());
        } ورنہ {
            برائے (AstCFunc* ثابت cfuncp : cfuncps) دورہ_Called_CFunc(cfuncp);
        }
    }
    خالی دورہ(AstCFunc* ثابت nodep) override {
        ثابت خودکار& قدر = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        اگر (قدر.m_state != CFuncCache::UNINITIALIZED) واپس;
        VL_RESTORER(m_cfuncp);
        // Its timing domains are those of the processes calling it, see visitCalledCFunc()
        VL_RESTORER_CLEAR(m_timingDomains);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }

    // Pre/Post logic are created here and their content need no further changes, so ignore.
    خالی دورہ(AstAlwaysPre*) override {}
    خالی دورہ(AstAlwaysPost*) override {}

    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح DelayedVisitor(AstNetlist* nodep)
        : m_classGraphp{V3_Class_گراف::build(nodep)} {
        iterate(nodep);
    }
    ~DelayedVisitor() override {
        V3Stats::جوڑو_Stat("NBA, variables using ShadowVar scheme", m_nSchemeShadowVar);
        V3Stats::جوڑو_Stat("NBA, variables using ShadowVarMasked scheme", m_nSchemeShadowVarMasked);
        V3Stats::جوڑو_Stat("NBA, variables using FlagShared scheme", m_nSchemeFlagShared);
        V3Stats::جوڑو_Stat("NBA, variables using FlagUnique scheme", m_nSchemeFlagUnique);
        V3Stats::جوڑو_Stat("NBA, variables using ValueQueueWhole scheme", m_n_Scheme_قدر_Queues_پورا);
        V3Stats::جوڑو_Stat("NBA, variables using ValueQueuePartial scheme",
                         m_n_Scheme_قدر_Queues_Partial);
        V3Stats::جوڑو_Stat("Optimizations, NBA flags shared", m_n_Shared_سیٹ_Flags);
        V3Stats::جوڑو_Stat("Procedures needing initial NBA trigger", m_nInitialNBA);
        V3Stats::جوڑو_Stat("Non-inlined co_awaits with SenTree", m_non_Inlined_CAwaits_With_Sen_درخت);
    }
};

//######################################################################
// Delayed class functions

خالی V3Delayed::delayedAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DelayedVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("delayed", 0, dump_درخت_Either_Level() >= 3);
}
