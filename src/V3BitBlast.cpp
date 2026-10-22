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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3BitBlast.h"

#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3SharedTmps.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <algorithm>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

வகுப்பு BitBlastVisitor final : பொது VNVisitorConst, பொது VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    கட்டமைப்பு RdRef final {
        Ast_முனை_கோவை* exprp;  // Sel, or the VarRef if whole
        முழு lsb;  // First bit read
        முழு msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    கட்டமைப்பு WrRef final {
        Ast_முனை_கோவை* exprp;  // The Sel, or the VarRef if whole
        Ast_முனை_Assign* assignp;  // The assignment, if it can be expanded
        முழு lsb;  // The first bit written
        முழு msb;  // The last bit written
    };

    // A fragment of a split AstVarScope
    கட்டமைப்பு Fragment final {
        முழு lsb;  // The first bit in the variable
        முழு msb;  // The last bit in the variable
        AstVarScope* vscp;  // The AstVarScope of the fragment
    };

    // The information about an AstVarScope of an eligible variable
    கட்டமைப்பு VscpInfo final {
        std::vector<RdRef> rdRefs;  // The reads of the AstVarScope
        std::vector<WrRef> wrRefs;  // The writes of the AstVarScope
        std::vector<Fragment> fragments;  // The fragments in LSB order, empty if not split
        தருக்க blocked = பொய்;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    மாறிலி VNUser1InUse m_user1InUse;
    மாறிலி VNUser2InUse m_user2InUse;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<முழு, முழு>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::vector<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_முனை_Assign* m_assignp = nullptr;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_statSplitAttr;  // AstVarScopes split only due to split_var
    VDouble0 m_statSplitAuto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    நிலையான வெறுமை warnNoSplit(AstVar* varp, மாறிலி Ast_முனை* wherep, மாறிலி எழுத்து* reasonp) {
        // Only warn if user requested splitting
        எனில் (!varp->attrSplitVar()) திரும்பு;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, உண்மை);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    நிலையான தருக்க isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        எனில் (!varp->user1()) {
            மாறிலி தருக்க eligible = [&]() {
                மாறிலி Ast_முனை_DType* மாறிலி dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                எனில் (!dtypep->isIntegralOrPacked()) திரும்பு பொய்;
                // Wider than one bit
                எனில் (dtypep->width() <= 1) திரும்பு பொய்;
                // Check properties
                மாறிலி எழுத்து* மாறிலி reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                எனில் (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                திரும்பு !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        திரும்பு varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscpInfoOf(AstVarScope* vscp) {
        எனில் (VscpInfo* மாறிலி infop = m_vscpInfos.tryGet(vscp)) திரும்பு *infop;
        // Also record on first encounter
        m_vscps.தள்ளு_back(vscp);
        திரும்பு m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    நிலையான வெறுமை block(AstVar* varp, VscpInfo& info, மாறிலி Ast_முனை* wherep) {
        warnNoSplit(varp, wherep, "it is referenced in an unsupported way");
        எனில் (info.blocked) திரும்பு;
        info.blocked = உண்மை;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    வெறுமை record(Ast_முனை_கோவை* exprp, AstVarRef* refp, முழு lsb, முழு msb) {
        AstVar* மாறிலி varp = refp->varp();
        எனில் (!isEligible(varp)) திரும்பு;
        VscpInfo& info = vscpInfoOf(refp->varScopep());
        // RW reference blocks splitting
        எனில் (refp->access().isRW()) block(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        எனில் (info.blocked) திரும்பு;
        // Record reference
        எனில் (refp->access().isReadOnly()) {
            info.rdRefs.தள்ளு_back({exprp, lsb, msb});
            திரும்பு;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        மாறிலி தருக்க canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.தள்ளு_back({exprp, canExpand ? m_assignp : nullptr, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    வெறுமை visitAssignment(Ast_முனை_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? nullptr : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, முழு lsb, முழு msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        எனில் (fragVarpr) திரும்பு fragVarpr;
        மாறிலி முழு width = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_முனை::encode_எண்(msb);
        எனில் (width > 1) name += Ast_முனை::encodeName(":") + Ast_முனை::encode_எண்(lsb);
        name += "__KET__";
        Ast_முனை_DType* மாறிலி dtypep = varp->dtypep()->isFourstate()
                                         ? varp->findLogicDType(width, width, VSigning::UNSIGNED)
                                         : varp->findBitDType(width, width, VSigning::UNSIGNED);
        fragVarpr = புதிய AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagateSplitAttrFrom(varp);
        varp->addHereThisAsNext(fragVarpr);
        திரும்பு fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    வெறுமை decide(AstVarScope* vscp, VscpInfo& info) {
        எனில் (info.blocked) திரும்பு;
        AstVar* மாறிலி varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        மாறிலி தருக்க isAuto = [&]() {
            std::vector<std::pair<முழு, முழு>> ranges;  // [lsb, msb] inclusive
            க்கு (மாறிலி RdRef& ref : info.rdRefs) {
                எனில் (VN_IS(ref.exprp, VarRef)) திரும்பு பொய்;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            க்கு (மாறிலி WrRef& ref : info.wrRefs) {
                எனில் (VN_IS(ref.exprp, VarRef)) திரும்பு பொய்;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            std::sort(ranges.begin(), ranges.end());
            க்கு (size_t i = 0; i + 1 < ranges.size(); ++i) {
                மாறிலி std::pair<முழு, முழு>& a = ranges[i];
                மாறிலி std::pair<முழு, முழு>& b = ranges[i + 1];
                எனில் (a != b && a.second >= b.first) திரும்பு பொய்;
            }
            திரும்பு உண்மை;
        }();
        // Otherwise only if requested
        எனில் (!isAuto && !varp->attrSplitVar()) திரும்பு;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        முழு lsb = varp->width();
        முழு msb = -1;
        க்கு (மாறிலி RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::vector<முழு> bounds;
        க்கு (மாறிலி WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.தள்ளு_back(ref.lsb);
            bounds.தள்ளு_back(ref.msb + 1);
        }
        bounds.தள்ளு_back(lsb);
        bounds.தள்ளு_back(msb + 1);
        std::sort(bounds.begin(), bounds.end());
        bounds.erase(std::unique(bounds.begin(), bounds.end()), bounds.end());
        // Nothing to do if the only fragment is the whole variable
        எனில் (bounds.size() == 2 && lsb == 0 && msb == varp->width() - 1) திரும்பு;
        // Writes that cannot be expanded must write one fragment
        க்கு (மாறிலி WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            எனில் (ref.assignp) தொடர்;
            // If write is to a whole fragment, it can be replaced
            எனில் (*std::upper_bound(bounds.begin(), bounds.end(), ref.lsb) == ref.msb + 1) {
                தொடர்;
            }
            // Otherwise can't split
            warnNoSplit(varp, ref.exprp, "it is written in an unsupported way");
            திரும்பு;
        }

        // Splitting this variable
        ++(isAuto ? m_statSplitAuto : m_statSplitAttr);

        // Create the fragment AstVarScopes
        க்கு (size_t i = 0; i + 1 < bounds.size(); ++i) {
            மாறிலி முழு lsb = bounds[i];
            மாறிலி முழு msb = bounds[i + 1] - 1;
            AstVar* மாறிலி fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* மாறிலி newp = புதிய AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->addHereThisAsNext(newp);
            info.fragments.தள்ளு_back({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    நிலையான size_t fragmentIndex(மாறிலி VscpInfo& info, முழு bit) {
        மாறிலி auto it = std::upper_bound(info.fragments.begin(), info.fragments.end(), bit,
                                         [](முழு b, மாறிலி Fragment& fragment) {  //
                                             திரும்பு b <= fragment.msb;
                                         });
        திரும்பு it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    வெறுமை rewriteRead(மாறிலி RdRef& ref, மாறிலி VscpInfo& info) {
        File_வரி* மாறிலி flp = ref.exprp->fileline();
        Ast_முனை_கோவை* resultp = nullptr;
        க்கு (size_t i = fragmentIndex(info, ref.lsb); i < info.fragments.size(); ++i) {
            மாறிலி Fragment& fragment = info.fragments[i];
            எனில் (fragment.lsb > ref.msb) நிறுத்து;
            மாறிலி முழு partLsb = std::max(ref.lsb, fragment.lsb);
            மாறிலி முழு partMsb = std::min(ref.msb, fragment.msb);
            Ast_முனை_கோவை* bitsp = புதிய AstVarRef{flp, fragment.vscp, VAccess::READ};
            எனில் (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = புதிய AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? புதிய AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(தள்ளு_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    நிலையான தருக்க isSelectable(மாறிலி Ast_முனை_கோவை* nodep) {
        எனில் (VN_IS(nodep, Const)) திரும்பு உண்மை;
        எனில் (மாறிலி AstSel* மாறிலி selp = VN_CAST(nodep, Sel)) {
            எனில் (!VN_IS(selp->lsbp(), Const)) திரும்பு பொய்;
            nodep = selp->fromp();
        }
        மாறிலி AstVarRef* மாறிலி refp = VN_CAST(nodep, VarRef);
        எனில் (!refp) திரும்பு பொய்;
        // Not a forced variable (trips several bugs in V3Force)
        எனில் (refp->varp()->isForced()) திரும்பு பொய்;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        திரும்பு !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    வெறுமை rewriteWrite(AstScope* scopep, மாறிலி WrRef& ref, மாறிலி VscpInfo& info) {
        File_வரி* மாறிலி flp = ref.exprp->fileline();
        மாறிலி size_t first = fragmentIndex(info, ref.lsb);
        மாறிலி size_t last = fragmentIndex(info, ref.msb);
        UASSERT_OBJ(info.fragments[first].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        எனில் (first == last) {
            ref.exprp->replaceWith(புதிய AstVarRef{flp, info.fragments[first].vscp, VAccess::WRITE});
            VL_DO_DANGLING(தள்ளு_Deletep(ref.exprp), ref.exprp);
            திரும்பு;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_முனை_Assign* மாறிலி origp = ref.assignp;
        Ast_முனை_கோவை* rhsp = origp->rhsp()->unlinkFrBack();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        எனில் (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* மாறிலி tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* மாறிலி tmpRefp = புதிய AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->addHereThisAsNext(புதிய AstAssign{flp, tmpRefp, rhsp});
            rhsp = புதிய AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        க்கு (size_t i = first; i <= last; ++i) {
            மாறிலி Fragment& fragment = info.fragments[i];
            மாறிலி முழு lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            மாறிலி முழு width = fragment.msb - fragment.lsb + 1;
            AstVarRef* மாறிலி lhsp = புதிய AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* மாறிலி selp = புதிய AstSel{flp, rhsp->clone_மரம்_Pure(பொய்), lsb, width};
            origp->addHereThisAsNext(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(தள்ளு_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(தள்ளு_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    நிலையான AstActive* comboActive(AstScope* scopep) {
        எனில் (!scopep->user2p()) scopep->user2p(scopep->comboActivep(உண்மை));
        திரும்பு VN_AS(scopep->user2p(), Active);
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_DType*) override {}  // No references in data types
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }
    வெறுமை visit(AstAssign* nodep) override { visitAssignment(nodep); }
    வெறுமை visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    வெறுமை visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    வெறுமை visit(AstSel* nodep) override {
        AstVarRef* மாறிலி refp = VN_CAST(nodep->fromp(), VarRef);
        எனில் (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            திரும்பு;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        மாறிலி AstConst* மாறிலி lsbp = VN_CAST(nodep->lsbp(), Const);
        எனில் (!lsbp || lsbp->toSInt() + nodep->widthConst() > refp->varp()->width()) {
            block(refp->varp(), vscpInfoOf(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            திரும்பு;
        }
        // Record partial access
        record(nodep, refp, lsbp->toSInt(), lsbp->toSInt() + nodep->widthConst() - 1);
    }
    வெறுமை visit(AstVarRef* nodep) override {
        // Record whole access
        record(nodep, nodep, 0, nodep->varp()->width() - 1);
    }
    வெறுமை visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        // The AstVarScope is not known, so all of them are blocked, by making it ineligible
        AstVar* மாறிலி varp = nodep->varp();
        எனில் (!isEligible(varp)) திரும்பு;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    explicit BitBlastVisitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        க்கு (AstVarScope* மாறிலி vscp : m_vscps) {
            எனில் (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        க்கு (AstVarScope* மாறிலி vscp : m_vscps) {
            மாறிலி VscpInfo& info = m_vscpInfos(vscp);
            எனில் (info.fragments.empty()) தொடர்;
            க்கு (மாறிலி RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        க்கு (AstVarScope* மாறிலி vscp : m_vscps) {
            மாறிலி VscpInfo& info = m_vscpInfos(vscp);
            எனில் (info.fragments.empty()) தொடர்;
            க்கு (மாறிலி WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        க்கு (AstVarScope* மாறிலி vscp : m_vscps) {
            மாறிலி VscpInfo& info = m_vscpInfos(vscp);
            எனில் (info.fragments.empty()) தொடர்;
            எனில் (!(vscp->varp()->isTrace() && vscp->isTrace())) தொடர்;
            AstActive* மாறிலி activep = comboActive(vscp->scopep());
            File_வரி* மாறிலி flp = vscp->fileline();
            க்கு (மாறிலி Fragment& fragment : info.fragments) {
                AstVarRef* மாறிலி lhsRefp = புதிய AstVarRef{flp, vscp, VAccess::WRITE};
                மாறிலி முழு width = fragment.msb - fragment.lsb + 1;
                AstSel* மாறிலி lhsp = புதிய AstSel{flp, lhsRefp, fragment.lsb, width};
                AstVarRef* மாறிலி rhsp = புதிய AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->addStmtsp(புதிய AstAlways{புதிய AstAssignW{flp, lhsp, rhsp}});
            }
        }

        V3Stats::addStat("Optimizations, Bitblast, variables split due to attribute",
                         m_statSplitAttr);
        V3Stats::addStat("Optimizations, Bitblast, variables split automatically",
                         m_statSplitAuto);
        V3Stats::addStat("Optimizations, Bitblast, fragments created", m_statFragments);
        V3Stats::addStat("Optimizations, Bitblast, assignments through temporary", m_statTmps);
    }

பொது:
    நிலையான வெறுமை apply(AstNetlist* netlistp) { BitBlastVisitor{netlistp}; }
};

//######################################################################
// V3BitBlast class functions

வெறுமை V3BitBlast::bitBlastAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    BitBlastVisitor::apply(nodep);
    V3Global::dump_Check_Global_மரம்("bitblast", 0, dump_மரம்_Either_Level() >= 3);
}
