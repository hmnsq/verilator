// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Split packed variables into bit ranges
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
// V3BitBlast replaces a packed variable with one variable per bit range,
// called a fragment. This avoids false combinational loops (UNOPTFLAT) through
// the bits of a variable, and lets downstream passes optimize each fragment
// separately.
//
// A variable is split if all its references are either:
//  - constant bit selects,
//  - whole,
// and either:
//   - it is marked with split_var
//   - all its references are constant bit selects, which are either disjoint
//     or identical (i.e.: non-overlapping)
// The fragments are the ranges between the boundaries of the bit ranges
// written, and the edges of the range of bits referenced by any read or write,
// so unused bits at the edges are dropped. A write then assigns whole
// fragments, and a read selects from, or concatenates the fragments it
// overlaps.
//
// A write to multiple fragments must be an assignment LHS, which is expanded
// into assignments to each fragment. This only happens for split_var
// variables, e.g.:
//
//   logic [7:0] x /* verilator split_var */;
//   always_comb begin
//     x[3:0] = a;
//     x[7:4] = b;
//     x = {x[3:0], x[7:4]};
//   end
//   assign out = x[5:2];
//
// Becomes, with fragments 'x_7_4' and 'x_3_0' for 'x[7:4]' and 'x[3:0]':
//
//   always_comb begin
//     x_3_0 = a;
//     x_7_4 = b;
//     tmp = {x_3_0, x_7_4};
//     x_3_0 = tmp[3:0];
//     x_7_4 = tmp[7:4];
//   end
//   assign out = {x_7_4[1:0], x_3_0[3:2]};
//
// The RHS of an assignment to multiple fragments is assigned to a temporary
// first, so it is evaluated once, and reads the fragments before any of them is
// written. A constant, a variable, or a constant select of a variable is
// selected from directly instead.
//
// The pass records all references in a single traversal, decides which
// variables to split, then replaces the references. The decision is made for
// each AstVarScope separately. AstVarScopes of the same variable with the same
// fragment share the AstVar of the fragment.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3BitBlast.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3SharedTmps.h"
#ضمن "V3Stats.h"

#ضمن <algorithm>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف ٻٽو_انگ_Blast_Visitor final : عام VNVisitorConst, عام VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    بنية RdRef final {
        Ast_ڳنڍ_اظهار* exprp;  // Sel, or the VarRef if whole
        صحيح lsb;  // First bit read
        صحيح msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    بنية WrRef final {
        Ast_ڳنڍ_اظهار* exprp;  // The Sel, or the VarRef if whole
        Ast_ڳنڍ_سونپيو* assignp;  // The assignment, if it can be expanded
        صحيح lsb;  // The first bit written
        صحيح msb;  // The last bit written
    };

    // A fragment of a split AstVarScope
    بنية Fragment final {
        صحيح lsb;  // The first bit in the variable
        صحيح msb;  // The last bit in the variable
        AstVarScope* vscp;  // The AstVarScope of the fragment
    };

    // The information about an AstVarScope of an eligible variable
    بنية VscpInfo final {
        std::ويڪٽر<RdRef> rdRefs;  // The reads of the AstVarScope
        std::ويڪٽر<WrRef> wrRefs;  // The writes of the AstVarScope
        std::ويڪٽر<Fragment> fragments;  // The fragments in LSB order, empty if not split
        منطقي blocked = خطأ;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    ثابت VNUser1_اندريون_Use m_user1_اندريون_Use;
    ثابت VNUser2_اندريون_Use m_user2_اندريون_Use;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<صحيح, صحيح>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::ويڪٽر<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_ڳنڍ_سونپيو* m_assignp = خالي_اشارو;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_stat_ورهايو_Attr;  // AstVarScopes split only due to split_var
    VDouble0 m_stat_ورهايو_Auto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ساكن فراغ warn_No_ورهايو(AstVar* varp, ثابت Ast_ڳنڍ* wherep, ثابت حرف* reasonp) {
        // Only warn if user requested splitting
        إذا (!varp->attr_ورهايو_Var()) أرجع;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3_Error_ڪوڊ::SPLITVAR, سچ);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    ساكن منطقي isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        إذا (!varp->user1()) {
            ثابت منطقي eligible = [&]() {
                ثابت Ast_ڳنڍ_DType* ثابت dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                إذا (!dtypep->isIntegralOrPacked()) أرجع خطأ;
                // Wider than one bit
                إذا (dtypep->ويڪر() <= 1) أرجع خطأ;
                // Check properties
                ثابت حرف* ثابت reasonp = varp->cannot_ورهايو_قسم_Reason();
                // Warn that it cannot be split if user explicitly requested splitting
                إذا (reasonp) warn_No_ورهايو(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                أرجع !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        أرجع varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscp_Info_جو(AstVarScope* vscp) {
        إذا (VscpInfo* ثابت infop = m_vscpInfos.tryGet(vscp)) أرجع *infop;
        // Also record on first encounter
        m_vscps.ڌڪو_پٺ(vscp);
        أرجع m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    ساكن فراغ بلاڪ(AstVar* varp, VscpInfo& info, ثابت Ast_ڳنڍ* wherep) {
        warn_No_ورهايو(varp, wherep, "it is referenced in an unsupported way");
        إذا (info.blocked) أرجع;
        info.blocked = سچ;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    فراغ رڪارڊ(Ast_ڳنڍ_اظهار* exprp, AstVarRef* refp, صحيح lsb, صحيح msb) {
        AstVar* ثابت varp = refp->varp();
        إذا (!isEligible(varp)) أرجع;
        VscpInfo& info = vscp_Info_جو(refp->varScopep());
        // RW reference blocks splitting
        إذا (refp->access().isRW()) بلاڪ(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        إذا (info.blocked) أرجع;
        // Record reference
        إذا (refp->access().isReadOnly()) {
            info.rdRefs.ڌڪو_پٺ({exprp, lsb, msb});
            أرجع;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        ثابت منطقي canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.ڌڪو_پٺ({exprp, canExpand ? m_assignp : خالي_اشارو, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    فراغ گهمو_Assignment(Ast_ڳنڍ_سونپيو* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? خالي_اشارو : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, صحيح lsb, صحيح msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        إذا (fragVarpr) أرجع fragVarpr;
        ثابت صحيح ويڪر = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_ڳنڍ::encode_عدد(msb);
        إذا (ويڪر > 1) name += Ast_ڳنڍ::encodeName(":") + Ast_ڳنڍ::encode_عدد(lsb);
        name += "__KET__";
        Ast_ڳنڍ_DType* ثابت dtypep = varp->dtypep()->isFourstate()
                                         ? varp->ڳوليو_Logic_DType(ويڪر, ويڪر, VSigning::UNSIGNED)
                                         : varp->ڳوليو_ٻٽو_انگ_DType(ويڪر, ويڪر, VSigning::UNSIGNED);
        fragVarpr = جديد AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagate_ورهايو_Attr_From(varp);
        varp->جوڙيو_هتي_This_As_ايندڙ(fragVarpr);
        أرجع fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    فراغ decide(AstVarScope* vscp, VscpInfo& info) {
        إذا (info.blocked) أرجع;
        AstVar* ثابت varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        ثابت منطقي isAuto = [&]() {
            std::ويڪٽر<std::pair<صحيح, صحيح>> ranges;  // [lsb, msb] inclusive
            لكل (ثابت RdRef& ref : info.rdRefs) {
                إذا (VN_IS(ref.exprp, VarRef)) أرجع خطأ;
                ranges.emplace_پٺ(ref.lsb, ref.msb);
            }
            لكل (ثابت WrRef& ref : info.wrRefs) {
                إذا (VN_IS(ref.exprp, VarRef)) أرجع خطأ;
                ranges.emplace_پٺ(ref.lsb, ref.msb);
            }
            std::ترتيب_ڏيو(ranges.begin(), ranges.پڇاڙي());
            لكل (سائيز_t i = 0; i + 1 < ranges.سائيز(); ++i) {
                ثابت std::pair<صحيح, صحيح>& a = ranges[i];
                ثابت std::pair<صحيح, صحيح>& b = ranges[i + 1];
                إذا (a != b && a.ٻيو >= b.پهريون) أرجع خطأ;
            }
            أرجع سچ;
        }();
        // Otherwise only if requested
        إذا (!isAuto && !varp->attr_ورهايو_Var()) أرجع;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        صحيح lsb = varp->ويڪر();
        صحيح msb = -1;
        لكل (ثابت RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::ويڪٽر<صحيح> bounds;
        لكل (ثابت WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.ڌڪو_پٺ(ref.lsb);
            bounds.ڌڪو_پٺ(ref.msb + 1);
        }
        bounds.ڌڪو_پٺ(lsb);
        bounds.ڌڪو_پٺ(msb + 1);
        std::ترتيب_ڏيو(bounds.begin(), bounds.پڇاڙي());
        bounds.erase(std::unique(bounds.begin(), bounds.پڇاڙي()), bounds.پڇاڙي());
        // Nothing to do if the only fragment is the whole variable
        إذا (bounds.سائيز() == 2 && lsb == 0 && msb == varp->ويڪر() - 1) أرجع;
        // Writes that cannot be expanded must write one fragment
        لكل (ثابت WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            إذا (ref.assignp) استمر;
            // If write is to a whole fragment, it can be replaced
            إذا (*std::مٿانهون_bound(bounds.begin(), bounds.پڇاڙي(), ref.lsb) == ref.msb + 1) {
                استمر;
            }
            // Otherwise can't split
            warn_No_ورهايو(varp, ref.exprp, "it is written in an unsupported way");
            أرجع;
        }

        // Splitting this variable
        ++(isAuto ? m_stat_ورهايو_Auto : m_stat_ورهايو_Attr);

        // Create the fragment AstVarScopes
        لكل (سائيز_t i = 0; i + 1 < bounds.سائيز(); ++i) {
            ثابت صحيح lsb = bounds[i];
            ثابت صحيح msb = bounds[i + 1] - 1;
            AstVar* ثابت fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* ثابت newp = جديد AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->جوڙيو_هتي_This_As_ايندڙ(newp);
            info.fragments.ڌڪو_پٺ({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    ساكن سائيز_t fragment_اشاريو(ثابت VscpInfo& info, صحيح ٻٽو_انگ_2) {
        ثابت خودڪار it = std::مٿانهون_bound(info.fragments.begin(), info.fragments.پڇاڙي(), ٻٽو_انگ_2,
                                         [](صحيح b, ثابت Fragment& fragment) {  //
                                             أرجع b <= fragment.msb;
                                         });
        أرجع it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    فراغ rewriteRead(ثابت RdRef& ref, ثابت VscpInfo& info) {
        File_سٽ* ثابت flp = ref.exprp->fileline();
        Ast_ڳنڍ_اظهار* resultp = خالي_اشارو;
        لكل (سائيز_t i = fragment_اشاريو(info, ref.lsb); i < info.fragments.سائيز(); ++i) {
            ثابت Fragment& fragment = info.fragments[i];
            إذا (fragment.lsb > ref.msb) اكسر;
            ثابت صحيح partLsb = std::max(ref.lsb, fragment.lsb);
            ثابت صحيح partMsb = std::min(ref.msb, fragment.msb);
            Ast_ڳنڍ_اظهار* bitsp = جديد AstVarRef{flp, fragment.vscp, VAccess::READ};
            إذا (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = جديد AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? جديد AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(ڌڪو_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    ساكن منطقي isSelectable(ثابت Ast_ڳنڍ_اظهار* nodep) {
        إذا (VN_IS(nodep, Const)) أرجع سچ;
        إذا (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            إذا (!VN_IS(selp->lsbp(), Const)) أرجع خطأ;
            nodep = selp->fromp();
        }
        ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef);
        إذا (!refp) أرجع خطأ;
        // Not a forced variable (trips several bugs in V3Force)
        إذا (refp->varp()->isForced()) أرجع خطأ;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        أرجع !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    فراغ rewriteWrite(AstScope* scopep, ثابت WrRef& ref, ثابت VscpInfo& info) {
        File_سٽ* ثابت flp = ref.exprp->fileline();
        ثابت سائيز_t پهريون = fragment_اشاريو(info, ref.lsb);
        ثابت سائيز_t آخري = fragment_اشاريو(info, ref.msb);
        UASSERT_OBJ(info.fragments[پهريون].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        إذا (پهريون == آخري) {
            ref.exprp->replaceWith(جديد AstVarRef{flp, info.fragments[پهريون].vscp, VAccess::WRITE});
            VL_DO_DANGLING(ڌڪو_Deletep(ref.exprp), ref.exprp);
            أرجع;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_ڳنڍ_سونپيو* ثابت origp = ref.assignp;
        Ast_ڳنڍ_اظهار* rhsp = origp->rhsp()->unlink_Fr_پٺ();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        إذا (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* ثابت tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* ثابت tmpRefp = جديد AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->جوڙيو_هتي_This_As_ايندڙ(جديد Ast_سونپيو{flp, tmpRefp, rhsp});
            rhsp = جديد AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        لكل (سائيز_t i = پهريون; i <= آخري; ++i) {
            ثابت Fragment& fragment = info.fragments[i];
            ثابت صحيح lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            ثابت صحيح ويڪر = fragment.msb - fragment.lsb + 1;
            AstVarRef* ثابت lhsp = جديد AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* ثابت selp = جديد AstSel{flp, rhsp->clone_وڻ_خالص(خطأ), lsb, ويڪر};
            origp->جوڙيو_هتي_This_As_ايندڙ(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(ڌڪو_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ڌڪو_Deletep(origp->unlink_Fr_پٺ()), origp);
    }

    // The combinational AstActive of the scope, cached
    ساكن Ast_سرگرم* combo_سرگرم(AstScope* scopep) {
        إذا (!scopep->user2p()) scopep->user2p(scopep->comboActivep(سچ));
        أرجع VN_AS(scopep->user2p(), سرگرم);
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_DType*) override {}  // No references in data types
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildrenConst(nodep); }
    فراغ گهمو(Ast_سونپيو* nodep) override { گهمو_Assignment(nodep); }
    فراغ گهمو(Ast_سونپيو_W* nodep) override { گهمو_Assignment(nodep); }
    فراغ گهمو(Ast_سونپيو_Dly* nodep) override { گهمو_Assignment(nodep); }
    فراغ گهمو(AstSel* nodep) override {
        AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef);
        إذا (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            أرجع;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        ثابت AstConst* ثابت lsbp = VN_CAST(nodep->lsbp(), Const);
        إذا (!lsbp || lsbp->منزل_SInt() + nodep->ويڪر_Const() > refp->varp()->ويڪر()) {
            بلاڪ(refp->varp(), vscp_Info_جو(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            أرجع;
        }
        // Record partial access
        رڪارڊ(nodep, refp, lsbp->منزل_SInt(), lsbp->منزل_SInt() + nodep->ويڪر_Const() - 1);
    }
    فراغ گهمو(AstVarRef* nodep) override {
        // Record whole access
        رڪارڊ(nodep, nodep, 0, nodep->varp()->ويڪر() - 1);
    }
    فراغ گهمو(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        // The AstVarScope is not known, so all of them are blocked, by making it ineligible
        AstVar* ثابت varp = nodep->varp();
        إذا (!isEligible(varp)) أرجع;
        varp->user1(2);  // Mark not eligible
        warn_No_ورهايو(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    واضح ٻٽو_انگ_Blast_Visitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        لكل (AstVarScope* ثابت vscp : m_vscps) {
            إذا (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        لكل (AstVarScope* ثابت vscp : m_vscps) {
            ثابت VscpInfo& info = m_vscpInfos(vscp);
            إذا (info.fragments.خالي_جايون()) استمر;
            لكل (ثابت RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        لكل (AstVarScope* ثابت vscp : m_vscps) {
            ثابت VscpInfo& info = m_vscpInfos(vscp);
            إذا (info.fragments.خالي_جايون()) استمر;
            لكل (ثابت WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        لكل (AstVarScope* ثابت vscp : m_vscps) {
            ثابت VscpInfo& info = m_vscpInfos(vscp);
            إذا (info.fragments.خالي_جايون()) استمر;
            إذا (!(vscp->varp()->isTrace() && vscp->isTrace())) استمر;
            Ast_سرگرم* ثابت activep = combo_سرگرم(vscp->scopep());
            File_سٽ* ثابت flp = vscp->fileline();
            لكل (ثابت Fragment& fragment : info.fragments) {
                AstVarRef* ثابت lhsRefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
                ثابت صحيح ويڪر = fragment.msb - fragment.lsb + 1;
                AstSel* ثابت lhsp = جديد AstSel{flp, lhsRefp, fragment.lsb, ويڪر};
                AstVarRef* ثابت rhsp = جديد AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->جوڙيو_Stmtsp(جديد AstAlways{جديد Ast_سونپيو_W{flp, lhsp, rhsp}});
            }
        }

        V3Stats::جوڙيو_Stat("Optimizations, Bitblast, variables split due to attribute",
                         m_stat_ورهايو_Attr);
        V3Stats::جوڙيو_Stat("Optimizations, Bitblast, variables split automatically",
                         m_stat_ورهايو_Auto);
        V3Stats::جوڙيو_Stat("Optimizations, Bitblast, fragments created", m_statFragments);
        V3Stats::جوڙيو_Stat("Optimizations, Bitblast, assignments through temporary", m_statTmps);
    }

عام:
    ساكن فراغ apply(AstNetlist* netlistp) { ٻٽو_انگ_Blast_Visitor{netlistp}; }
};

//######################################################################
// V3BitBlast class functions

فراغ V3_ٻٽو_انگ_Blast::ٻٽو_انگ_Blast_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    ٻٽو_انگ_Blast_Visitor::apply(nodep);
    V3Global::dump_Check_Global_وڻ("bitblast", 0, dump_وڻ_Either_Level() >= 3);
}
