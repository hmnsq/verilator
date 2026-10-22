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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3BitBlast.h"

#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3SharedTmps.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <algorithm>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

વર્ગ BitBlastVisitor final : જાહેર VNVisitorConst, જાહેર VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    સંરચના RdRef final {
        Ast_ગાંઠ_પદાવલિ* exprp;  // Sel, or the VarRef if whole
        પૂર્ણાંક lsb;  // First bit read
        પૂર્ણાંક msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    સંરચના WrRef final {
        Ast_ગાંઠ_પદાવલિ* exprp;  // The Sel, or the VarRef if whole
        Ast_ગાંઠ_Assign* assignp;  // The assignment, if it can be expanded
        પૂર્ણાંક lsb;  // The first bit written
        પૂર્ણાંક msb;  // The last bit written
    };

    // A fragment of a split AstVarScope
    સંરચના Fragment final {
        પૂર્ણાંક lsb;  // The first bit in the variable
        પૂર્ણાંક msb;  // The last bit in the variable
        AstVarScope* vscp;  // The AstVarScope of the fragment
    };

    // The information about an AstVarScope of an eligible variable
    સંરચના VscpInfo final {
        std::vector<RdRef> rdRefs;  // The reads of the AstVarScope
        std::vector<WrRef> wrRefs;  // The writes of the AstVarScope
        std::vector<Fragment> fragments;  // The fragments in LSB order, empty if not split
        તાર્કિક blocked = ખોટું;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    અચલ VNUser1InUse m_user1InUse;
    અચલ VNUser2InUse m_user2InUse;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<પૂર્ણાંક, પૂર્ણાંક>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::vector<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_ગાંઠ_Assign* m_assignp = nullptr;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_statSplitAttr;  // AstVarScopes split only due to split_var
    VDouble0 m_statSplitAuto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    સ્થિર શૂન્ય warnNoSplit(AstVar* varp, અચલ Ast_ગાંઠ* wherep, અચલ અક્ષર* reasonp) {
        // Only warn if user requested splitting
        જો (!varp->attrSplitVar()) પાછા;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, સાચું);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    સ્થિર તાર્કિક isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        જો (!varp->user1()) {
            અચલ તાર્કિક eligible = [&]() {
                અચલ Ast_ગાંઠ_DType* અચલ dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                જો (!dtypep->isIntegralOrPacked()) પાછા ખોટું;
                // Wider than one bit
                જો (dtypep->width() <= 1) પાછા ખોટું;
                // Check properties
                અચલ અક્ષર* અચલ reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                જો (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                પાછા !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        પાછા varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscpInfoOf(AstVarScope* vscp) {
        જો (VscpInfo* અચલ infop = m_vscpInfos.tryGet(vscp)) પાછા *infop;
        // Also record on first encounter
        m_vscps.ધકેલો_back(vscp);
        પાછા m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    સ્થિર શૂન્ય block(AstVar* varp, VscpInfo& info, અચલ Ast_ગાંઠ* wherep) {
        warnNoSplit(varp, wherep, "it is referenced in an unsupported way");
        જો (info.blocked) પાછા;
        info.blocked = સાચું;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    શૂન્ય record(Ast_ગાંઠ_પદાવલિ* exprp, AstVarRef* refp, પૂર્ણાંક lsb, પૂર્ણાંક msb) {
        AstVar* અચલ varp = refp->varp();
        જો (!isEligible(varp)) પાછા;
        VscpInfo& info = vscpInfoOf(refp->varScopep());
        // RW reference blocks splitting
        જો (refp->access().isRW()) block(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        જો (info.blocked) પાછા;
        // Record reference
        જો (refp->access().isReadOnly()) {
            info.rdRefs.ધકેલો_back({exprp, lsb, msb});
            પાછા;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        અચલ તાર્કિક canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.ધકેલો_back({exprp, canExpand ? m_assignp : nullptr, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    શૂન્ય visitAssignment(Ast_ગાંઠ_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? nullptr : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, પૂર્ણાંક lsb, પૂર્ણાંક msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        જો (fragVarpr) પાછા fragVarpr;
        અચલ પૂર્ણાંક width = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_ગાંઠ::encode_સંખ્યા(msb);
        જો (width > 1) name += Ast_ગાંઠ::encodeName(":") + Ast_ગાંઠ::encode_સંખ્યા(lsb);
        name += "__KET__";
        Ast_ગાંઠ_DType* અચલ dtypep = varp->dtypep()->isFourstate()
                                         ? varp->findLogicDType(width, width, VSigning::UNSIGNED)
                                         : varp->findBitDType(width, width, VSigning::UNSIGNED);
        fragVarpr = નવું AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagateSplitAttrFrom(varp);
        varp->addHereThisAsNext(fragVarpr);
        પાછા fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    શૂન્ય decide(AstVarScope* vscp, VscpInfo& info) {
        જો (info.blocked) પાછા;
        AstVar* અચલ varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        અચલ તાર્કિક isAuto = [&]() {
            std::vector<std::pair<પૂર્ણાંક, પૂર્ણાંક>> ranges;  // [lsb, msb] inclusive
            માટે (અચલ RdRef& ref : info.rdRefs) {
                જો (VN_IS(ref.exprp, VarRef)) પાછા ખોટું;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            માટે (અચલ WrRef& ref : info.wrRefs) {
                જો (VN_IS(ref.exprp, VarRef)) પાછા ખોટું;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            std::sort(ranges.begin(), ranges.end());
            માટે (size_t i = 0; i + 1 < ranges.size(); ++i) {
                અચલ std::pair<પૂર્ણાંક, પૂર્ણાંક>& a = ranges[i];
                અચલ std::pair<પૂર્ણાંક, પૂર્ણાંક>& b = ranges[i + 1];
                જો (a != b && a.second >= b.first) પાછા ખોટું;
            }
            પાછા સાચું;
        }();
        // Otherwise only if requested
        જો (!isAuto && !varp->attrSplitVar()) પાછા;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        પૂર્ણાંક lsb = varp->width();
        પૂર્ણાંક msb = -1;
        માટે (અચલ RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::vector<પૂર્ણાંક> bounds;
        માટે (અચલ WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.ધકેલો_back(ref.lsb);
            bounds.ધકેલો_back(ref.msb + 1);
        }
        bounds.ધકેલો_back(lsb);
        bounds.ધકેલો_back(msb + 1);
        std::sort(bounds.begin(), bounds.end());
        bounds.erase(std::unique(bounds.begin(), bounds.end()), bounds.end());
        // Nothing to do if the only fragment is the whole variable
        જો (bounds.size() == 2 && lsb == 0 && msb == varp->width() - 1) પાછા;
        // Writes that cannot be expanded must write one fragment
        માટે (અચલ WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            જો (ref.assignp) ચાલુ;
            // If write is to a whole fragment, it can be replaced
            જો (*std::upper_bound(bounds.begin(), bounds.end(), ref.lsb) == ref.msb + 1) {
                ચાલુ;
            }
            // Otherwise can't split
            warnNoSplit(varp, ref.exprp, "it is written in an unsupported way");
            પાછા;
        }

        // Splitting this variable
        ++(isAuto ? m_statSplitAuto : m_statSplitAttr);

        // Create the fragment AstVarScopes
        માટે (size_t i = 0; i + 1 < bounds.size(); ++i) {
            અચલ પૂર્ણાંક lsb = bounds[i];
            અચલ પૂર્ણાંક msb = bounds[i + 1] - 1;
            AstVar* અચલ fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* અચલ newp = નવું AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->addHereThisAsNext(newp);
            info.fragments.ધકેલો_back({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    સ્થિર size_t fragmentIndex(અચલ VscpInfo& info, પૂર્ણાંક bit) {
        અચલ auto it = std::upper_bound(info.fragments.begin(), info.fragments.end(), bit,
                                         [](પૂર્ણાંક b, અચલ Fragment& fragment) {  //
                                             પાછા b <= fragment.msb;
                                         });
        પાછા it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    શૂન્ય rewriteRead(અચલ RdRef& ref, અચલ VscpInfo& info) {
        File_લીટી* અચલ flp = ref.exprp->fileline();
        Ast_ગાંઠ_પદાવલિ* resultp = nullptr;
        માટે (size_t i = fragmentIndex(info, ref.lsb); i < info.fragments.size(); ++i) {
            અચલ Fragment& fragment = info.fragments[i];
            જો (fragment.lsb > ref.msb) તોડો;
            અચલ પૂર્ણાંક partLsb = std::max(ref.lsb, fragment.lsb);
            અચલ પૂર્ણાંક partMsb = std::min(ref.msb, fragment.msb);
            Ast_ગાંઠ_પદાવલિ* bitsp = નવું AstVarRef{flp, fragment.vscp, VAccess::READ};
            જો (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = નવું AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? નવું AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(ધકેલો_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    સ્થિર તાર્કિક isSelectable(અચલ Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (VN_IS(nodep, Const)) પાછા સાચું;
        જો (અચલ AstSel* અચલ selp = VN_CAST(nodep, Sel)) {
            જો (!VN_IS(selp->lsbp(), Const)) પાછા ખોટું;
            nodep = selp->fromp();
        }
        અચલ AstVarRef* અચલ refp = VN_CAST(nodep, VarRef);
        જો (!refp) પાછા ખોટું;
        // Not a forced variable (trips several bugs in V3Force)
        જો (refp->varp()->isForced()) પાછા ખોટું;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        પાછા !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    શૂન્ય rewriteWrite(AstScope* scopep, અચલ WrRef& ref, અચલ VscpInfo& info) {
        File_લીટી* અચલ flp = ref.exprp->fileline();
        અચલ size_t first = fragmentIndex(info, ref.lsb);
        અચલ size_t last = fragmentIndex(info, ref.msb);
        UASSERT_OBJ(info.fragments[first].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        જો (first == last) {
            ref.exprp->replaceWith(નવું AstVarRef{flp, info.fragments[first].vscp, VAccess::WRITE});
            VL_DO_DANGLING(ધકેલો_Deletep(ref.exprp), ref.exprp);
            પાછા;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_ગાંઠ_Assign* અચલ origp = ref.assignp;
        Ast_ગાંઠ_પદાવલિ* rhsp = origp->rhsp()->unlinkFrBack();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        જો (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* અચલ tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* અચલ tmpRefp = નવું AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->addHereThisAsNext(નવું AstAssign{flp, tmpRefp, rhsp});
            rhsp = નવું AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        માટે (size_t i = first; i <= last; ++i) {
            અચલ Fragment& fragment = info.fragments[i];
            અચલ પૂર્ણાંક lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            અચલ પૂર્ણાંક width = fragment.msb - fragment.lsb + 1;
            AstVarRef* અચલ lhsp = નવું AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* અચલ selp = નવું AstSel{flp, rhsp->clone_વૃક્ષ_Pure(ખોટું), lsb, width};
            origp->addHereThisAsNext(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(ધકેલો_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ધકેલો_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    સ્થિર AstActive* comboActive(AstScope* scopep) {
        જો (!scopep->user2p()) scopep->user2p(scopep->comboActivep(સાચું));
        પાછા VN_AS(scopep->user2p(), Active);
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_DType*) override {}  // No references in data types
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }
    શૂન્ય visit(AstAssign* nodep) override { visitAssignment(nodep); }
    શૂન્ય visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    શૂન્ય visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    શૂન્ય visit(AstSel* nodep) override {
        AstVarRef* અચલ refp = VN_CAST(nodep->fromp(), VarRef);
        જો (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            પાછા;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        અચલ AstConst* અચલ lsbp = VN_CAST(nodep->lsbp(), Const);
        જો (!lsbp || lsbp->toSInt() + nodep->widthConst() > refp->varp()->width()) {
            block(refp->varp(), vscpInfoOf(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            પાછા;
        }
        // Record partial access
        record(nodep, refp, lsbp->toSInt(), lsbp->toSInt() + nodep->widthConst() - 1);
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        // Record whole access
        record(nodep, nodep, 0, nodep->varp()->width() - 1);
    }
    શૂન્ય visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        // The AstVarScope is not known, so all of them are blocked, by making it ineligible
        AstVar* અચલ varp = nodep->varp();
        જો (!isEligible(varp)) પાછા;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    explicit BitBlastVisitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        માટે (AstVarScope* અચલ vscp : m_vscps) {
            જો (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        માટે (AstVarScope* અચલ vscp : m_vscps) {
            અચલ VscpInfo& info = m_vscpInfos(vscp);
            જો (info.fragments.empty()) ચાલુ;
            માટે (અચલ RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        માટે (AstVarScope* અચલ vscp : m_vscps) {
            અચલ VscpInfo& info = m_vscpInfos(vscp);
            જો (info.fragments.empty()) ચાલુ;
            માટે (અચલ WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        માટે (AstVarScope* અચલ vscp : m_vscps) {
            અચલ VscpInfo& info = m_vscpInfos(vscp);
            જો (info.fragments.empty()) ચાલુ;
            જો (!(vscp->varp()->isTrace() && vscp->isTrace())) ચાલુ;
            AstActive* અચલ activep = comboActive(vscp->scopep());
            File_લીટી* અચલ flp = vscp->fileline();
            માટે (અચલ Fragment& fragment : info.fragments) {
                AstVarRef* અચલ lhsRefp = નવું AstVarRef{flp, vscp, VAccess::WRITE};
                અચલ પૂર્ણાંક width = fragment.msb - fragment.lsb + 1;
                AstSel* અચલ lhsp = નવું AstSel{flp, lhsRefp, fragment.lsb, width};
                AstVarRef* અચલ rhsp = નવું AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->addStmtsp(નવું AstAlways{નવું AstAssignW{flp, lhsp, rhsp}});
            }
        }

        V3Stats::addStat("Optimizations, Bitblast, variables split due to attribute",
                         m_statSplitAttr);
        V3Stats::addStat("Optimizations, Bitblast, variables split automatically",
                         m_statSplitAuto);
        V3Stats::addStat("Optimizations, Bitblast, fragments created", m_statFragments);
        V3Stats::addStat("Optimizations, Bitblast, assignments through temporary", m_statTmps);
    }

જાહેર:
    સ્થિર શૂન્ય apply(AstNetlist* netlistp) { BitBlastVisitor{netlistp}; }
};

//######################################################################
// V3BitBlast class functions

શૂન્ય V3BitBlast::bitBlastAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    BitBlastVisitor::apply(nodep);
    V3Global::dump_Check_Global_વૃક્ષ("bitblast", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
