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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3BitBlast.h"

#অন্তর্ভাব "V3AstUserAllocator.h"
#অন্তর্ভাব "V3SharedTmps.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <algorithm>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

শ্রেণী BitBlastVisitor final : সর্বজনীন VNVisitorConst, সর্বজনীন VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    কাঠামো RdRef final {
        Ast_গ্রন্থি_রাশিমালা* exprp;  // Sel, or the VarRef if whole
        পূর্ণসংখ্যা lsb;  // First bit read
        পূর্ণসংখ্যা msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    কাঠামো WrRef final {
        Ast_গ্রন্থি_রাশিমালা* exprp;  // The Sel, or the VarRef if whole
        Ast_গ্রন্থি_Assign* assignp;  // The assignment, if it can be expanded
        পূর্ণসংখ্যা lsb;  // The first bit written
        পূর্ণসংখ্যা msb;  // The last bit written
    };

    // A fragment of a split AstVarScope
    কাঠামো Fragment final {
        পূর্ণসংখ্যা lsb;  // The first bit in the variable
        পূর্ণসংখ্যা msb;  // The last bit in the variable
        AstVarScope* vscp;  // The AstVarScope of the fragment
    };

    // The information about an AstVarScope of an eligible variable
    কাঠামো VscpInfo final {
        std::vector<RdRef> rdRefs;  // The reads of the AstVarScope
        std::vector<WrRef> wrRefs;  // The writes of the AstVarScope
        std::vector<Fragment> fragments;  // The fragments in LSB order, empty if not split
        যৌক্তিক blocked = মিথ্যা;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    ধ্রুবক VNUser1InUse m_user1InUse;
    ধ্রুবক VNUser2InUse m_user2InUse;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::vector<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_গ্রন্থি_Assign* m_assignp = nullptr;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_statSplitAttr;  // AstVarScopes split only due to split_var
    VDouble0 m_statSplitAuto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    স্থির শূন্য warnNoSplit(AstVar* varp, ধ্রুবক Ast_গ্রন্থি* wherep, ধ্রুবক অক্ষর* reasonp) {
        // Only warn if user requested splitting
        যদি (!varp->attrSplitVar()) ফেরত;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, সত্য);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    স্থির যৌক্তিক isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        যদি (!varp->user1()) {
            ধ্রুবক যৌক্তিক eligible = [&]() {
                ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                যদি (!dtypep->isIntegralOrPacked()) ফেরত মিথ্যা;
                // Wider than one bit
                যদি (dtypep->width() <= 1) ফেরত মিথ্যা;
                // Check properties
                ধ্রুবক অক্ষর* ধ্রুবক reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                যদি (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                ফেরত !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        ফেরত varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscpInfoOf(AstVarScope* vscp) {
        যদি (VscpInfo* ধ্রুবক infop = m_vscpInfos.tryGet(vscp)) ফেরত *infop;
        // Also record on first encounter
        m_vscps.ঠেলো_back(vscp);
        ফেরত m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    স্থির শূন্য block(AstVar* varp, VscpInfo& info, ধ্রুবক Ast_গ্রন্থি* wherep) {
        warnNoSplit(varp, wherep, "it is referenced in an unsupported way");
        যদি (info.blocked) ফেরত;
        info.blocked = সত্য;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    শূন্য record(Ast_গ্রন্থি_রাশিমালা* exprp, AstVarRef* refp, পূর্ণসংখ্যা lsb, পূর্ণসংখ্যা msb) {
        AstVar* ধ্রুবক varp = refp->varp();
        যদি (!isEligible(varp)) ফেরত;
        VscpInfo& info = vscpInfoOf(refp->varScopep());
        // RW reference blocks splitting
        যদি (refp->access().isRW()) block(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        যদি (info.blocked) ফেরত;
        // Record reference
        যদি (refp->access().isReadOnly()) {
            info.rdRefs.ঠেলো_back({exprp, lsb, msb});
            ফেরত;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        ধ্রুবক যৌক্তিক canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.ঠেলো_back({exprp, canExpand ? m_assignp : nullptr, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    শূন্য visitAssignment(Ast_গ্রন্থি_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? nullptr : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, পূর্ণসংখ্যা lsb, পূর্ণসংখ্যা msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        যদি (fragVarpr) ফেরত fragVarpr;
        ধ্রুবক পূর্ণসংখ্যা width = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_গ্রন্থি::encode_সংখ্যা(msb);
        যদি (width > 1) name += Ast_গ্রন্থি::encodeName(":") + Ast_গ্রন্থি::encode_সংখ্যা(lsb);
        name += "__KET__";
        Ast_গ্রন্থি_DType* ধ্রুবক dtypep = varp->dtypep()->isFourstate()
                                         ? varp->findLogicDType(width, width, VSigning::UNSIGNED)
                                         : varp->findBitDType(width, width, VSigning::UNSIGNED);
        fragVarpr = নতুন AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagateSplitAttrFrom(varp);
        varp->add_Here_This_As_পরবর্তী(fragVarpr);
        ফেরত fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    শূন্য decide(AstVarScope* vscp, VscpInfo& info) {
        যদি (info.blocked) ফেরত;
        AstVar* ধ্রুবক varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        ধ্রুবক যৌক্তিক isAuto = [&]() {
            std::vector<std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা>> ranges;  // [lsb, msb] inclusive
            জন্য (ধ্রুবক RdRef& ref : info.rdRefs) {
                যদি (VN_IS(ref.exprp, VarRef)) ফেরত মিথ্যা;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            জন্য (ধ্রুবক WrRef& ref : info.wrRefs) {
                যদি (VN_IS(ref.exprp, VarRef)) ফেরত মিথ্যা;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            std::sort(ranges.begin(), ranges.end());
            জন্য (আকার_t i = 0; i + 1 < ranges.আকার_2(); ++i) {
                ধ্রুবক std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা>& a = ranges[i];
                ধ্রুবক std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা>& b = ranges[i + 1];
                যদি (a != b && a.second >= b.first) ফেরত মিথ্যা;
            }
            ফেরত সত্য;
        }();
        // Otherwise only if requested
        যদি (!isAuto && !varp->attrSplitVar()) ফেরত;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        পূর্ণসংখ্যা lsb = varp->width();
        পূর্ণসংখ্যা msb = -1;
        জন্য (ধ্রুবক RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::vector<পূর্ণসংখ্যা> bounds;
        জন্য (ধ্রুবক WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.ঠেলো_back(ref.lsb);
            bounds.ঠেলো_back(ref.msb + 1);
        }
        bounds.ঠেলো_back(lsb);
        bounds.ঠেলো_back(msb + 1);
        std::sort(bounds.begin(), bounds.end());
        bounds.erase(std::unique(bounds.begin(), bounds.end()), bounds.end());
        // Nothing to do if the only fragment is the whole variable
        যদি (bounds.আকার_2() == 2 && lsb == 0 && msb == varp->width() - 1) ফেরত;
        // Writes that cannot be expanded must write one fragment
        জন্য (ধ্রুবক WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            যদি (ref.assignp) চালু;
            // If write is to a whole fragment, it can be replaced
            যদি (*std::upper_bound(bounds.begin(), bounds.end(), ref.lsb) == ref.msb + 1) {
                চালু;
            }
            // Otherwise can't split
            warnNoSplit(varp, ref.exprp, "it is written in an unsupported way");
            ফেরত;
        }

        // Splitting this variable
        ++(isAuto ? m_statSplitAuto : m_statSplitAttr);

        // Create the fragment AstVarScopes
        জন্য (আকার_t i = 0; i + 1 < bounds.আকার_2(); ++i) {
            ধ্রুবক পূর্ণসংখ্যা lsb = bounds[i];
            ধ্রুবক পূর্ণসংখ্যা msb = bounds[i + 1] - 1;
            AstVar* ধ্রুবক fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* ধ্রুবক newp = নতুন AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->add_Here_This_As_পরবর্তী(newp);
            info.fragments.ঠেলো_back({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    স্থির আকার_t fragment_সূচক(ধ্রুবক VscpInfo& info, পূর্ণসংখ্যা bit) {
        ধ্রুবক auto it = std::upper_bound(info.fragments.begin(), info.fragments.end(), bit,
                                         [](পূর্ণসংখ্যা b, ধ্রুবক Fragment& fragment) {  //
                                             ফেরত b <= fragment.msb;
                                         });
        ফেরত it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    শূন্য rewriteRead(ধ্রুবক RdRef& ref, ধ্রুবক VscpInfo& info) {
        File_সারি* ধ্রুবক flp = ref.exprp->fileline();
        Ast_গ্রন্থি_রাশিমালা* resultp = nullptr;
        জন্য (আকার_t i = fragment_সূচক(info, ref.lsb); i < info.fragments.আকার_2(); ++i) {
            ধ্রুবক Fragment& fragment = info.fragments[i];
            যদি (fragment.lsb > ref.msb) বিরতি;
            ধ্রুবক পূর্ণসংখ্যা partLsb = std::max(ref.lsb, fragment.lsb);
            ধ্রুবক পূর্ণসংখ্যা partMsb = std::min(ref.msb, fragment.msb);
            Ast_গ্রন্থি_রাশিমালা* bitsp = নতুন AstVarRef{flp, fragment.vscp, VAccess::READ};
            যদি (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = নতুন AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? নতুন AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(ঠেলো_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    স্থির যৌক্তিক isSelectable(ধ্রুবক Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (VN_IS(nodep, Const)) ফেরত সত্য;
        যদি (ধ্রুবক AstSel* ধ্রুবক selp = VN_CAST(nodep, Sel)) {
            যদি (!VN_IS(selp->lsbp(), Const)) ফেরত মিথ্যা;
            nodep = selp->fromp();
        }
        ধ্রুবক AstVarRef* ধ্রুবক refp = VN_CAST(nodep, VarRef);
        যদি (!refp) ফেরত মিথ্যা;
        // Not a forced variable (trips several bugs in V3Force)
        যদি (refp->varp()->isForced()) ফেরত মিথ্যা;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        ফেরত !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    শূন্য rewriteWrite(AstScope* scopep, ধ্রুবক WrRef& ref, ধ্রুবক VscpInfo& info) {
        File_সারি* ধ্রুবক flp = ref.exprp->fileline();
        ধ্রুবক আকার_t first = fragment_সূচক(info, ref.lsb);
        ধ্রুবক আকার_t last = fragment_সূচক(info, ref.msb);
        UASSERT_OBJ(info.fragments[first].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        যদি (first == last) {
            ref.exprp->replaceWith(নতুন AstVarRef{flp, info.fragments[first].vscp, VAccess::WRITE});
            VL_DO_DANGLING(ঠেলো_Deletep(ref.exprp), ref.exprp);
            ফেরত;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_গ্রন্থি_Assign* ধ্রুবক origp = ref.assignp;
        Ast_গ্রন্থি_রাশিমালা* rhsp = origp->rhsp()->unlinkFrBack();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        যদি (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* ধ্রুবক tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* ধ্রুবক tmpRefp = নতুন AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->add_Here_This_As_পরবর্তী(নতুন AstAssign{flp, tmpRefp, rhsp});
            rhsp = নতুন AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        জন্য (আকার_t i = first; i <= last; ++i) {
            ধ্রুবক Fragment& fragment = info.fragments[i];
            ধ্রুবক পূর্ণসংখ্যা lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            ধ্রুবক পূর্ণসংখ্যা width = fragment.msb - fragment.lsb + 1;
            AstVarRef* ধ্রুবক lhsp = নতুন AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* ধ্রুবক selp = নতুন AstSel{flp, rhsp->clone_বৃক্ষ_Pure(মিথ্যা), lsb, width};
            origp->add_Here_This_As_পরবর্তী(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(ঠেলো_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ঠেলো_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    স্থির AstActive* comboActive(AstScope* scopep) {
        যদি (!scopep->user2p()) scopep->user2p(scopep->comboActivep(সত্য));
        ফেরত VN_AS(scopep->user2p(), Active);
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_DType*) override {}  // No references in data types
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }
    শূন্য visit(AstAssign* nodep) override { visitAssignment(nodep); }
    শূন্য visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    শূন্য visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    শূন্য visit(AstSel* nodep) override {
        AstVarRef* ধ্রুবক refp = VN_CAST(nodep->fromp(), VarRef);
        যদি (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            ফেরত;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        ধ্রুবক AstConst* ধ্রুবক lsbp = VN_CAST(nodep->lsbp(), Const);
        যদি (!lsbp || lsbp->toSInt() + nodep->widthConst() > refp->varp()->width()) {
            block(refp->varp(), vscpInfoOf(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            ফেরত;
        }
        // Record partial access
        record(nodep, refp, lsbp->toSInt(), lsbp->toSInt() + nodep->widthConst() - 1);
    }
    শূন্য visit(AstVarRef* nodep) override {
        // Record whole access
        record(nodep, nodep, 0, nodep->varp()->width() - 1);
    }
    শূন্য visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        // The AstVarScope is not known, so all of them are blocked, by making it ineligible
        AstVar* ধ্রুবক varp = nodep->varp();
        যদি (!isEligible(varp)) ফেরত;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    explicit BitBlastVisitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        জন্য (AstVarScope* ধ্রুবক vscp : m_vscps) {
            যদি (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        জন্য (AstVarScope* ধ্রুবক vscp : m_vscps) {
            ধ্রুবক VscpInfo& info = m_vscpInfos(vscp);
            যদি (info.fragments.empty()) চালু;
            জন্য (ধ্রুবক RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        জন্য (AstVarScope* ধ্রুবক vscp : m_vscps) {
            ধ্রুবক VscpInfo& info = m_vscpInfos(vscp);
            যদি (info.fragments.empty()) চালু;
            জন্য (ধ্রুবক WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        জন্য (AstVarScope* ধ্রুবক vscp : m_vscps) {
            ধ্রুবক VscpInfo& info = m_vscpInfos(vscp);
            যদি (info.fragments.empty()) চালু;
            যদি (!(vscp->varp()->isTrace() && vscp->isTrace())) চালু;
            AstActive* ধ্রুবক activep = comboActive(vscp->scopep());
            File_সারি* ধ্রুবক flp = vscp->fileline();
            জন্য (ধ্রুবক Fragment& fragment : info.fragments) {
                AstVarRef* ধ্রুবক lhsRefp = নতুন AstVarRef{flp, vscp, VAccess::WRITE};
                ধ্রুবক পূর্ণসংখ্যা width = fragment.msb - fragment.lsb + 1;
                AstSel* ধ্রুবক lhsp = নতুন AstSel{flp, lhsRefp, fragment.lsb, width};
                AstVarRef* ধ্রুবক rhsp = নতুন AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->addStmtsp(নতুন AstAlways{নতুন AstAssignW{flp, lhsp, rhsp}});
            }
        }

        V3Stats::addStat("Optimizations, Bitblast, variables split due to attribute",
                         m_statSplitAttr);
        V3Stats::addStat("Optimizations, Bitblast, variables split automatically",
                         m_statSplitAuto);
        V3Stats::addStat("Optimizations, Bitblast, fragments created", m_statFragments);
        V3Stats::addStat("Optimizations, Bitblast, assignments through temporary", m_statTmps);
    }

সর্বজনীন:
    স্থির শূন্য apply(AstNetlist* netlistp) { BitBlastVisitor{netlistp}; }
};

//######################################################################
// V3BitBlast class functions

শূন্য V3BitBlast::bitBlastAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    BitBlastVisitor::apply(nodep);
    V3Global::dump_Check_Global_বৃক্ষ("bitblast", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
