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

صنف ثنائی_ہندسہ_Blast_Visitor final : عام VNVisitorConst, عام VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    بنية RdRef final {
        Ast_گرہ_اظہاریہ* exprp;  // Sel, or the VarRef if whole
        صحيح lsb;  // First bit read
        صحيح msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    بنية WrRef final {
        Ast_گرہ_اظہاریہ* exprp;  // The Sel, or the VarRef if whole
        Ast_گرہ_سونپو* assignp;  // The assignment, if it can be expanded
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
        std::سمتیہ<RdRef> rdRefs;  // The reads of the AstVarScope
        std::سمتیہ<WrRef> wrRefs;  // The writes of the AstVarScope
        std::سمتیہ<Fragment> fragments;  // The fragments in LSB order, empty if not split
        منطقي blocked = خطأ;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    ثابت VNUser1_وچ_Use m_user1_وچ_Use;
    ثابت VNUser2_وچ_Use m_user2_وچ_Use;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<صحيح, صحيح>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::سمتیہ<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_گرہ_سونپو* m_assignp = خالی_سوچک;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_stat_ونڈو_Attr;  // AstVarScopes split only due to split_var
    VDouble0 m_stat_ونڈو_Auto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ساكن فراغ warn_No_ونڈو(AstVar* varp, ثابت Ast_گرہ* wherep, ثابت حرف* reasonp) {
        // Only warn if user requested splitting
        إذا (!varp->attr_ونڈو_Var()) أرجع;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3_Error_کوڈ::SPLITVAR, سچ);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    ساكن منطقي isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        إذا (!varp->user1()) {
            ثابت منطقي eligible = [&]() {
                ثابت Ast_گرہ_DType* ثابت dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                إذا (!dtypep->isIntegralOrPacked()) أرجع خطأ;
                // Wider than one bit
                إذا (dtypep->چوڑائی() <= 1) أرجع خطأ;
                // Check properties
                ثابت حرف* ثابت reasonp = varp->cannot_ونڈو_قسم_Reason();
                // Warn that it cannot be split if user explicitly requested splitting
                إذا (reasonp) warn_No_ونڈو(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                أرجع !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        أرجع varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscp_Info_دا(AstVarScope* vscp) {
        إذا (VscpInfo* ثابت infop = m_vscpInfos.tryGet(vscp)) أرجع *infop;
        // Also record on first encounter
        m_vscps.ڈالو_پٹھ(vscp);
        أرجع m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    ساكن فراغ بلاک(AstVar* varp, VscpInfo& info, ثابت Ast_گرہ* wherep) {
        warn_No_ونڈو(varp, wherep, "it is referenced in an unsupported way");
        إذا (info.blocked) أرجع;
        info.blocked = سچ;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    فراغ ریکارڈ(Ast_گرہ_اظہاریہ* exprp, AstVarRef* refp, صحيح lsb, صحيح msb) {
        AstVar* ثابت varp = refp->varp();
        إذا (!isEligible(varp)) أرجع;
        VscpInfo& info = vscp_Info_دا(refp->varScopep());
        // RW reference blocks splitting
        إذا (refp->access().isRW()) بلاک(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        إذا (info.blocked) أرجع;
        // Record reference
        إذا (refp->access().isReadOnly()) {
            info.rdRefs.ڈالو_پٹھ({exprp, lsb, msb});
            أرجع;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        ثابت منطقي canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.ڈالو_پٹھ({exprp, canExpand ? m_assignp : خالی_سوچک, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    فراغ ویکھو_Assignment(Ast_گرہ_سونپو* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? خالی_سوچک : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, صحيح lsb, صحيح msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        إذا (fragVarpr) أرجع fragVarpr;
        ثابت صحيح چوڑائی = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_گرہ::encode_نمبر(msb);
        إذا (چوڑائی > 1) name += Ast_گرہ::encodeName(":") + Ast_گرہ::encode_نمبر(lsb);
        name += "__KET__";
        Ast_گرہ_DType* ثابت dtypep = varp->dtypep()->isFourstate()
                                         ? varp->لبھو_Logic_DType(چوڑائی, چوڑائی, VSigning::UNSIGNED)
                                         : varp->لبھو_ثنائی_ہندسہ_DType(چوڑائی, چوڑائی, VSigning::UNSIGNED);
        fragVarpr = جديد AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagate_ونڈو_Attr_From(varp);
        varp->جوڑو_ایتھے_This_As_اگلا(fragVarpr);
        أرجع fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    فراغ decide(AstVarScope* vscp, VscpInfo& info) {
        إذا (info.blocked) أرجع;
        AstVar* ثابت varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        ثابت منطقي isAuto = [&]() {
            std::سمتیہ<std::pair<صحيح, صحيح>> ranges;  // [lsb, msb] inclusive
            لكل (ثابت RdRef& ref : info.rdRefs) {
                إذا (VN_IS(ref.exprp, VarRef)) أرجع خطأ;
                ranges.emplace_پٹھ(ref.lsb, ref.msb);
            }
            لكل (ثابت WrRef& ref : info.wrRefs) {
                إذا (VN_IS(ref.exprp, VarRef)) أرجع خطأ;
                ranges.emplace_پٹھ(ref.lsb, ref.msb);
            }
            std::ترتیب_دیو(ranges.begin(), ranges.سرا());
            لكل (ناپ_t i = 0; i + 1 < ranges.ناپ(); ++i) {
                ثابت std::pair<صحيح, صحيح>& a = ranges[i];
                ثابت std::pair<صحيح, صحيح>& b = ranges[i + 1];
                إذا (a != b && a.دوجا >= b.پہلا) أرجع خطأ;
            }
            أرجع سچ;
        }();
        // Otherwise only if requested
        إذا (!isAuto && !varp->attr_ونڈو_Var()) أرجع;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        صحيح lsb = varp->چوڑائی();
        صحيح msb = -1;
        لكل (ثابت RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::سمتیہ<صحيح> bounds;
        لكل (ثابت WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.ڈالو_پٹھ(ref.lsb);
            bounds.ڈالو_پٹھ(ref.msb + 1);
        }
        bounds.ڈالو_پٹھ(lsb);
        bounds.ڈالو_پٹھ(msb + 1);
        std::ترتیب_دیو(bounds.begin(), bounds.سرا());
        bounds.erase(std::unique(bounds.begin(), bounds.سرا()), bounds.سرا());
        // Nothing to do if the only fragment is the whole variable
        إذا (bounds.ناپ() == 2 && lsb == 0 && msb == varp->چوڑائی() - 1) أرجع;
        // Writes that cannot be expanded must write one fragment
        لكل (ثابت WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            إذا (ref.assignp) استمر;
            // If write is to a whole fragment, it can be replaced
            إذا (*std::اتلا_bound(bounds.begin(), bounds.سرا(), ref.lsb) == ref.msb + 1) {
                استمر;
            }
            // Otherwise can't split
            warn_No_ونڈو(varp, ref.exprp, "it is written in an unsupported way");
            أرجع;
        }

        // Splitting this variable
        ++(isAuto ? m_stat_ونڈو_Auto : m_stat_ونڈو_Attr);

        // Create the fragment AstVarScopes
        لكل (ناپ_t i = 0; i + 1 < bounds.ناپ(); ++i) {
            ثابت صحيح lsb = bounds[i];
            ثابت صحيح msb = bounds[i + 1] - 1;
            AstVar* ثابت fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* ثابت newp = جديد AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->جوڑو_ایتھے_This_As_اگلا(newp);
            info.fragments.ڈالو_پٹھ({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    ساكن ناپ_t fragment_اشاریہ(ثابت VscpInfo& info, صحيح ثنائی_ہندسہ_2) {
        ثابت خودکار it = std::اتلا_bound(info.fragments.begin(), info.fragments.سرا(), ثنائی_ہندسہ_2,
                                         [](صحيح b, ثابت Fragment& fragment) {  //
                                             أرجع b <= fragment.msb;
                                         });
        أرجع it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    فراغ rewriteRead(ثابت RdRef& ref, ثابت VscpInfo& info) {
        File_سطر* ثابت flp = ref.exprp->fileline();
        Ast_گرہ_اظہاریہ* resultp = خالی_سوچک;
        لكل (ناپ_t i = fragment_اشاریہ(info, ref.lsb); i < info.fragments.ناپ(); ++i) {
            ثابت Fragment& fragment = info.fragments[i];
            إذا (fragment.lsb > ref.msb) اكسر;
            ثابت صحيح partLsb = std::max(ref.lsb, fragment.lsb);
            ثابت صحيح partMsb = std::min(ref.msb, fragment.msb);
            Ast_گرہ_اظہاریہ* bitsp = جديد AstVarRef{flp, fragment.vscp, VAccess::READ};
            إذا (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = جديد AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? جديد AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(ڈالو_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    ساكن منطقي isSelectable(ثابت Ast_گرہ_اظہاریہ* nodep) {
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
        File_سطر* ثابت flp = ref.exprp->fileline();
        ثابت ناپ_t پہلا = fragment_اشاریہ(info, ref.lsb);
        ثابت ناپ_t آخری = fragment_اشاریہ(info, ref.msb);
        UASSERT_OBJ(info.fragments[پہلا].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        إذا (پہلا == آخری) {
            ref.exprp->replaceWith(جديد AstVarRef{flp, info.fragments[پہلا].vscp, VAccess::WRITE});
            VL_DO_DANGLING(ڈالو_Deletep(ref.exprp), ref.exprp);
            أرجع;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_گرہ_سونپو* ثابت origp = ref.assignp;
        Ast_گرہ_اظہاریہ* rhsp = origp->rhsp()->unlink_Fr_پٹھ();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        إذا (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* ثابت tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* ثابت tmpRefp = جديد AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->جوڑو_ایتھے_This_As_اگلا(جديد Ast_سونپو{flp, tmpRefp, rhsp});
            rhsp = جديد AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        لكل (ناپ_t i = پہلا; i <= آخری; ++i) {
            ثابت Fragment& fragment = info.fragments[i];
            ثابت صحيح lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            ثابت صحيح چوڑائی = fragment.msb - fragment.lsb + 1;
            AstVarRef* ثابت lhsp = جديد AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* ثابت selp = جديد AstSel{flp, rhsp->clone_درخت_خالص(خطأ), lsb, چوڑائی};
            origp->جوڑو_ایتھے_This_As_اگلا(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(ڈالو_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ڈالو_Deletep(origp->unlink_Fr_پٹھ()), origp);
    }

    // The combinational AstActive of the scope, cached
    ساكن Ast_سرگرم* combo_سرگرم(AstScope* scopep) {
        إذا (!scopep->user2p()) scopep->user2p(scopep->comboActivep(سچ));
        أرجع VN_AS(scopep->user2p(), سرگرم);
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_DType*) override {}  // No references in data types
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }
    فراغ ویکھو(Ast_سونپو* nodep) override { ویکھو_Assignment(nodep); }
    فراغ ویکھو(Ast_سونپو_W* nodep) override { ویکھو_Assignment(nodep); }
    فراغ ویکھو(Ast_سونپو_Dly* nodep) override { ویکھو_Assignment(nodep); }
    فراغ ویکھو(AstSel* nodep) override {
        AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef);
        إذا (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            أرجع;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        ثابت AstConst* ثابت lsbp = VN_CAST(nodep->lsbp(), Const);
        إذا (!lsbp || lsbp->منزل_SInt() + nodep->چوڑائی_Const() > refp->varp()->چوڑائی()) {
            بلاک(refp->varp(), vscp_Info_دا(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            أرجع;
        }
        // Record partial access
        ریکارڈ(nodep, refp, lsbp->منزل_SInt(), lsbp->منزل_SInt() + nodep->چوڑائی_Const() - 1);
    }
    فراغ ویکھو(AstVarRef* nodep) override {
        // Record whole access
        ریکارڈ(nodep, nodep, 0, nodep->varp()->چوڑائی() - 1);
    }
    فراغ ویکھو(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        // The AstVarScope is not known, so all of them are blocked, by making it ineligible
        AstVar* ثابت varp = nodep->varp();
        إذا (!isEligible(varp)) أرجع;
        varp->user1(2);  // Mark not eligible
        warn_No_ونڈو(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    واضح ثنائی_ہندسہ_Blast_Visitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        لكل (AstVarScope* ثابت vscp : m_vscps) {
            إذا (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        لكل (AstVarScope* ثابت vscp : m_vscps) {
            ثابت VscpInfo& info = m_vscpInfos(vscp);
            إذا (info.fragments.خالی_تھاواں()) استمر;
            لكل (ثابت RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        لكل (AstVarScope* ثابت vscp : m_vscps) {
            ثابت VscpInfo& info = m_vscpInfos(vscp);
            إذا (info.fragments.خالی_تھاواں()) استمر;
            لكل (ثابت WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        لكل (AstVarScope* ثابت vscp : m_vscps) {
            ثابت VscpInfo& info = m_vscpInfos(vscp);
            إذا (info.fragments.خالی_تھاواں()) استمر;
            إذا (!(vscp->varp()->isTrace() && vscp->isTrace())) استمر;
            Ast_سرگرم* ثابت activep = combo_سرگرم(vscp->scopep());
            File_سطر* ثابت flp = vscp->fileline();
            لكل (ثابت Fragment& fragment : info.fragments) {
                AstVarRef* ثابت lhsRefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
                ثابت صحيح چوڑائی = fragment.msb - fragment.lsb + 1;
                AstSel* ثابت lhsp = جديد AstSel{flp, lhsRefp, fragment.lsb, چوڑائی};
                AstVarRef* ثابت rhsp = جديد AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->جوڑو_Stmtsp(جديد AstAlways{جديد Ast_سونپو_W{flp, lhsp, rhsp}});
            }
        }

        V3Stats::جوڑو_Stat("Optimizations, Bitblast, variables split due to attribute",
                         m_stat_ونڈو_Attr);
        V3Stats::جوڑو_Stat("Optimizations, Bitblast, variables split automatically",
                         m_stat_ونڈو_Auto);
        V3Stats::جوڑو_Stat("Optimizations, Bitblast, fragments created", m_statFragments);
        V3Stats::جوڑو_Stat("Optimizations, Bitblast, assignments through temporary", m_statTmps);
    }

عام:
    ساكن فراغ apply(AstNetlist* netlistp) { ثنائی_ہندسہ_Blast_Visitor{netlistp}; }
};

//######################################################################
// V3BitBlast class functions

فراغ V3_ثنائی_ہندسہ_Blast::ثنائی_ہندسہ_Blast_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    ثنائی_ہندسہ_Blast_Visitor::apply(nodep);
    V3Global::dump_Check_Global_درخت("bitblast", 0, dump_درخت_Either_Level() >= 3);
}
