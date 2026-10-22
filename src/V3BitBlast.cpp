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

বর্গ BitBlastVisitor final : সার্বজনিক VNVisitorConst, সার্বজনিক VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    সংরচনা RdRef final {
        Ast_গ্রন্থি_রাশিমালা* exprp;  // Sel, or the VarRef if whole
        পূর্ণাঙ্ক lsb;  // First bit read
        পূর্ণাঙ্ক msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    সংরচনা WrRef final {
        Ast_গ্রন্থি_রাশিমালা* exprp;  // The Sel, or the VarRef if whole
        Ast_গ্রন্থি_Assign* assignp;  // The assignment, if it can be expanded
        পূর্ণাঙ্ক lsb;  // The first bit written
        পূর্ণাঙ্ক msb;  // The last bit written
    };

    // A fragment of a split AstVarScope
    সংরচনা Fragment final {
        পূর্ণাঙ্ক lsb;  // The first bit in the variable
        পূর্ণাঙ্ক msb;  // The last bit in the variable
        AstVarScope* vscp;  // The AstVarScope of the fragment
    };

    // The information about an AstVarScope of an eligible variable
    সংরচনা VscpInfo final {
        std::vector<RdRef> rdRefs;  // The reads of the AstVarScope
        std::vector<WrRef> wrRefs;  // The writes of the AstVarScope
        std::vector<Fragment> fragments;  // The fragments in LSB order, empty if not split
        তার্কিক blocked = অসত্য;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    ধ্রুব VNUser1InUse m_user1InUse;
    ধ্রুব VNUser2InUse m_user2InUse;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<পূর্ণাঙ্ক, পূর্ণাঙ্ক>, AstVar*>> m_fragmentVarps;
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
    স্থির শূন্য warnNoSplit(AstVar* varp, ধ্রুব Ast_গ্রন্থি* wherep, ধ্রুব বর্ণ* reasonp) {
        // Only warn if user requested splitting
        যদি (!varp->attrSplitVar()) প্রত্যাগম;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, সত্য);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    স্থির তার্কিক isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        যদি (!varp->user1()) {
            ধ্রুব তার্কিক eligible = [&]() {
                ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                যদি (!dtypep->isIntegralOrPacked()) প্রত্যাগম অসত্য;
                // Wider than one bit
                যদি (dtypep->width() <= 1) প্রত্যাগম অসত্য;
                // Check properties
                ধ্রুব বর্ণ* ধ্রুব reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                যদি (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                প্রত্যাগম !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        প্রত্যাগম varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscpInfoOf(AstVarScope* vscp) {
        যদি (VscpInfo* ধ্রুব infop = m_vscpInfos.tryGet(vscp)) প্রত্যাগম *infop;
        // Also record on first encounter
        m_vscps.ঠেলো_back(vscp);
        প্রত্যাগম m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    স্থির শূন্য block(AstVar* varp, VscpInfo& info, ধ্রুব Ast_গ্রন্থি* wherep) {
        warnNoSplit(varp, wherep, "it is referenced in an unsupported way");
        যদি (info.blocked) প্রত্যাগম;
        info.blocked = সত্য;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    শূন্য record(Ast_গ্রন্থি_রাশিমালা* exprp, AstVarRef* refp, পূর্ণাঙ্ক lsb, পূর্ণাঙ্ক msb) {
        AstVar* ধ্রুব varp = refp->varp();
        যদি (!isEligible(varp)) প্রত্যাগম;
        VscpInfo& info = vscpInfoOf(refp->varScopep());
        // RW reference blocks splitting
        যদি (refp->access().isRW()) block(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        যদি (info.blocked) প্রত্যাগম;
        // Record reference
        যদি (refp->access().isReadOnly()) {
            info.rdRefs.ঠেলো_back({exprp, lsb, msb});
            প্রত্যাগম;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        ধ্রুব তার্কিক canExpand = m_assignp && exprp == m_assignp->lhsp();
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
    AstVar* fragmentVarp(AstVar* varp, পূর্ণাঙ্ক lsb, পূর্ণাঙ্ক msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        যদি (fragVarpr) প্রত্যাগম fragVarpr;
        ধ্রুব পূর্ণাঙ্ক width = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_গ্রন্থি::encode_সংখ্যা(msb);
        যদি (width > 1) name += Ast_গ্রন্থি::encodeName(":") + Ast_গ্রন্থি::encode_সংখ্যা(lsb);
        name += "__KET__";
        Ast_গ্রন্থি_DType* ধ্রুব dtypep = varp->dtypep()->isFourstate()
                                         ? varp->findLogicDType(width, width, VSigning::UNSIGNED)
                                         : varp->findBitDType(width, width, VSigning::UNSIGNED);
        fragVarpr = নূতন AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagateSplitAttrFrom(varp);
        varp->addHereThisAsNext(fragVarpr);
        প্রত্যাগম fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    শূন্য decide(AstVarScope* vscp, VscpInfo& info) {
        যদি (info.blocked) প্রত্যাগম;
        AstVar* ধ্রুব varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        ধ্রুব তার্কিক isAuto = [&]() {
            std::vector<std::pair<পূর্ণাঙ্ক, পূর্ণাঙ্ক>> ranges;  // [lsb, msb] inclusive
            প্রতি (ধ্রুব RdRef& ref : info.rdRefs) {
                যদি (VN_IS(ref.exprp, VarRef)) প্রত্যাগম অসত্য;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            প্রতি (ধ্রুব WrRef& ref : info.wrRefs) {
                যদি (VN_IS(ref.exprp, VarRef)) প্রত্যাগম অসত্য;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            std::sort(ranges.begin(), ranges.end());
            প্রতি (size_t i = 0; i + 1 < ranges.size(); ++i) {
                ধ্রুব std::pair<পূর্ণাঙ্ক, পূর্ণাঙ্ক>& a = ranges[i];
                ধ্রুব std::pair<পূর্ণাঙ্ক, পূর্ণাঙ্ক>& b = ranges[i + 1];
                যদি (a != b && a.second >= b.first) প্রত্যাগম অসত্য;
            }
            প্রত্যাগম সত্য;
        }();
        // Otherwise only if requested
        যদি (!isAuto && !varp->attrSplitVar()) প্রত্যাগম;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        পূর্ণাঙ্ক lsb = varp->width();
        পূর্ণাঙ্ক msb = -1;
        প্রতি (ধ্রুব RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::vector<পূর্ণাঙ্ক> bounds;
        প্রতি (ধ্রুব WrRef& ref : info.wrRefs) {
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
        যদি (bounds.size() == 2 && lsb == 0 && msb == varp->width() - 1) প্রত্যাগম;
        // Writes that cannot be expanded must write one fragment
        প্রতি (ধ্রুব WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            যদি (ref.assignp) অনুবর্ত;
            // If write is to a whole fragment, it can be replaced
            যদি (*std::upper_bound(bounds.begin(), bounds.end(), ref.lsb) == ref.msb + 1) {
                অনুবর্ত;
            }
            // Otherwise can't split
            warnNoSplit(varp, ref.exprp, "it is written in an unsupported way");
            প্রত্যাগম;
        }

        // Splitting this variable
        ++(isAuto ? m_statSplitAuto : m_statSplitAttr);

        // Create the fragment AstVarScopes
        প্রতি (size_t i = 0; i + 1 < bounds.size(); ++i) {
            ধ্রুব পূর্ণাঙ্ক lsb = bounds[i];
            ধ্রুব পূর্ণাঙ্ক msb = bounds[i + 1] - 1;
            AstVar* ধ্রুব fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* ধ্রুব newp = নূতন AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->addHereThisAsNext(newp);
            info.fragments.ঠেলো_back({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    স্থির size_t fragmentIndex(ধ্রুব VscpInfo& info, পূর্ণাঙ্ক bit) {
        ধ্রুব auto it = std::upper_bound(info.fragments.begin(), info.fragments.end(), bit,
                                         [](পূর্ণাঙ্ক b, ধ্রুব Fragment& fragment) {  //
                                             প্রত্যাগম b <= fragment.msb;
                                         });
        প্রত্যাগম it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    শূন্য rewriteRead(ধ্রুব RdRef& ref, ধ্রুব VscpInfo& info) {
        File_সারি* ধ্রুব flp = ref.exprp->fileline();
        Ast_গ্রন্থি_রাশিমালা* resultp = nullptr;
        প্রতি (size_t i = fragmentIndex(info, ref.lsb); i < info.fragments.size(); ++i) {
            ধ্রুব Fragment& fragment = info.fragments[i];
            যদি (fragment.lsb > ref.msb) ভঙ্গ;
            ধ্রুব পূর্ণাঙ্ক partLsb = std::max(ref.lsb, fragment.lsb);
            ধ্রুব পূর্ণাঙ্ক partMsb = std::min(ref.msb, fragment.msb);
            Ast_গ্রন্থি_রাশিমালা* bitsp = নূতন AstVarRef{flp, fragment.vscp, VAccess::READ};
            যদি (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = নূতন AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? নূতন AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(ঠেলো_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    স্থির তার্কিক isSelectable(ধ্রুব Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (VN_IS(nodep, Const)) প্রত্যাগম সত্য;
        যদি (ধ্রুব AstSel* ধ্রুব selp = VN_CAST(nodep, Sel)) {
            যদি (!VN_IS(selp->lsbp(), Const)) প্রত্যাগম অসত্য;
            nodep = selp->fromp();
        }
        ধ্রুব AstVarRef* ধ্রুব refp = VN_CAST(nodep, VarRef);
        যদি (!refp) প্রত্যাগম অসত্য;
        // Not a forced variable (trips several bugs in V3Force)
        যদি (refp->varp()->isForced()) প্রত্যাগম অসত্য;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        প্রত্যাগম !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    শূন্য rewriteWrite(AstScope* scopep, ধ্রুব WrRef& ref, ধ্রুব VscpInfo& info) {
        File_সারি* ধ্রুব flp = ref.exprp->fileline();
        ধ্রুব size_t first = fragmentIndex(info, ref.lsb);
        ধ্রুব size_t last = fragmentIndex(info, ref.msb);
        UASSERT_OBJ(info.fragments[first].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        যদি (first == last) {
            ref.exprp->replaceWith(নূতন AstVarRef{flp, info.fragments[first].vscp, VAccess::WRITE});
            VL_DO_DANGLING(ঠেলো_Deletep(ref.exprp), ref.exprp);
            প্রত্যাগম;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_গ্রন্থি_Assign* ধ্রুব origp = ref.assignp;
        Ast_গ্রন্থি_রাশিমালা* rhsp = origp->rhsp()->unlinkFrBack();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        যদি (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* ধ্রুব tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* ধ্রুব tmpRefp = নূতন AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->addHereThisAsNext(নূতন AstAssign{flp, tmpRefp, rhsp});
            rhsp = নূতন AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        প্রতি (size_t i = first; i <= last; ++i) {
            ধ্রুব Fragment& fragment = info.fragments[i];
            ধ্রুব পূর্ণাঙ্ক lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            ধ্রুব পূর্ণাঙ্ক width = fragment.msb - fragment.lsb + 1;
            AstVarRef* ধ্রুব lhsp = নূতন AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* ধ্রুব selp = নূতন AstSel{flp, rhsp->clone_বৃক্ষ_Pure(অসত্য), lsb, width};
            origp->addHereThisAsNext(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(ঠেলো_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ঠেলো_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    স্থির AstActive* comboActive(AstScope* scopep) {
        যদি (!scopep->user2p()) scopep->user2p(scopep->comboActivep(সত্য));
        প্রত্যাগম VN_AS(scopep->user2p(), Active);
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_DType*) override {}  // No references in data types
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }
    শূন্য visit(AstAssign* nodep) override { visitAssignment(nodep); }
    শূন্য visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    শূন্য visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    শূন্য visit(AstSel* nodep) override {
        AstVarRef* ধ্রুব refp = VN_CAST(nodep->fromp(), VarRef);
        যদি (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            প্রত্যাগম;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        ধ্রুব AstConst* ধ্রুব lsbp = VN_CAST(nodep->lsbp(), Const);
        যদি (!lsbp || lsbp->toSInt() + nodep->widthConst() > refp->varp()->width()) {
            block(refp->varp(), vscpInfoOf(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            প্রত্যাগম;
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
        AstVar* ধ্রুব varp = nodep->varp();
        যদি (!isEligible(varp)) প্রত্যাগম;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    explicit BitBlastVisitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        প্রতি (AstVarScope* ধ্রুব vscp : m_vscps) {
            যদি (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        প্রতি (AstVarScope* ধ্রুব vscp : m_vscps) {
            ধ্রুব VscpInfo& info = m_vscpInfos(vscp);
            যদি (info.fragments.empty()) অনুবর্ত;
            প্রতি (ধ্রুব RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        প্রতি (AstVarScope* ধ্রুব vscp : m_vscps) {
            ধ্রুব VscpInfo& info = m_vscpInfos(vscp);
            যদি (info.fragments.empty()) অনুবর্ত;
            প্রতি (ধ্রুব WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        প্রতি (AstVarScope* ধ্রুব vscp : m_vscps) {
            ধ্রুব VscpInfo& info = m_vscpInfos(vscp);
            যদি (info.fragments.empty()) অনুবর্ত;
            যদি (!(vscp->varp()->isTrace() && vscp->isTrace())) অনুবর্ত;
            AstActive* ধ্রুব activep = comboActive(vscp->scopep());
            File_সারি* ধ্রুব flp = vscp->fileline();
            প্রতি (ধ্রুব Fragment& fragment : info.fragments) {
                AstVarRef* ধ্রুব lhsRefp = নূতন AstVarRef{flp, vscp, VAccess::WRITE};
                ধ্রুব পূর্ণাঙ্ক width = fragment.msb - fragment.lsb + 1;
                AstSel* ধ্রুব lhsp = নূতন AstSel{flp, lhsRefp, fragment.lsb, width};
                AstVarRef* ধ্রুব rhsp = নূতন AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->addStmtsp(নূতন AstAlways{নূতন AstAssignW{flp, lhsp, rhsp}});
            }
        }

        V3Stats::addStat("Optimizations, Bitblast, variables split due to attribute",
                         m_statSplitAttr);
        V3Stats::addStat("Optimizations, Bitblast, variables split automatically",
                         m_statSplitAuto);
        V3Stats::addStat("Optimizations, Bitblast, fragments created", m_statFragments);
        V3Stats::addStat("Optimizations, Bitblast, assignments through temporary", m_statTmps);
    }

সার্বজনিক:
    স্থির শূন্য apply(AstNetlist* netlistp) { BitBlastVisitor{netlistp}; }
};

//######################################################################
// V3BitBlast class functions

শূন্য V3BitBlast::bitBlastAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    BitBlastVisitor::apply(nodep);
    V3Global::dump_Check_Global_বৃক্ষ("bitblast", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
