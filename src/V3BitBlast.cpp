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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3BitBlast.h"

#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3SharedTmps.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ଶ୍ରେଣୀ BitBlastVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst, ସର୍ବସାଧାରଣ VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    ସଂରଚନା RdRef final {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp;  // Sel, or the VarRef if whole
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb;  // First bit read
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    ସଂରଚନା WrRef final {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp;  // The Sel, or the VarRef if whole
        Ast_ଗଣ୍ଠି_Assign* assignp;  // The assignment, if it can be expanded
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb;  // The first bit written
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb;  // The last bit written
    };

    // A fragment of a split AstVarScope
    ସଂରଚନା Fragment final {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb;  // The first bit in the variable
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb;  // The last bit in the variable
        AstVarScope* vscp;  // The AstVarScope of the fragment
    };

    // The information about an AstVarScope of an eligible variable
    ସଂରଚନା VscpInfo final {
        std::vector<RdRef> rdRefs;  // The reads of the AstVarScope
        std::vector<WrRef> wrRefs;  // The writes of the AstVarScope
        std::vector<Fragment> fragments;  // The fragments in LSB order, empty if not split
        ତାର୍କିକ blocked = ମିଥ୍ୟା;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    ଅଚଳ VNUser1InUse m_user1InUse;
    ଅଚଳ VNUser2InUse m_user2InUse;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::vector<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_ଗଣ୍ଠି_Assign* m_assignp = nullptr;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_statSplitAttr;  // AstVarScopes split only due to split_var
    VDouble0 m_statSplitAuto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ସ୍ଥିର ଶୂନ୍ୟ warnNoSplit(AstVar* varp, ଅଚଳ Ast_ଗଣ୍ଠି* wherep, ଅଚଳ ଅକ୍ଷର* reasonp) {
        // Only warn if user requested splitting
        ଯଦି (!varp->attrSplitVar()) ଫେରନ୍ତୁ;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, ସତ୍ୟ);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    ସ୍ଥିର ତାର୍କିକ isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        ଯଦି (!varp->user1()) {
            ଅଚଳ ତାର୍କିକ eligible = [&]() {
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                ଯଦି (!dtypep->isIntegralOrPacked()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                // Wider than one bit
                ଯଦି (dtypep->width() <= 1) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                // Check properties
                ଅଚଳ ଅକ୍ଷର* ଅଚଳ reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                ଯଦି (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                ଫେରନ୍ତୁ !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        ଫେରନ୍ତୁ varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscpInfoOf(AstVarScope* vscp) {
        ଯଦି (VscpInfo* ଅଚଳ infop = m_vscpInfos.tryGet(vscp)) ଫେରନ୍ତୁ *infop;
        // Also record on first encounter
        m_vscps.ଠେଲ_back(vscp);
        ଫେରନ୍ତୁ m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    ସ୍ଥିର ଶୂନ୍ୟ block(AstVar* varp, VscpInfo& info, ଅଚଳ Ast_ଗଣ୍ଠି* wherep) {
        warnNoSplit(varp, wherep, "it is referenced in an unsupported way");
        ଯଦି (info.blocked) ଫେରନ୍ତୁ;
        info.blocked = ସତ୍ୟ;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    ଶୂନ୍ୟ record(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, AstVarRef* refp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb) {
        AstVar* ଅଚଳ varp = refp->varp();
        ଯଦି (!isEligible(varp)) ଫେରନ୍ତୁ;
        VscpInfo& info = vscpInfoOf(refp->varScopep());
        // RW reference blocks splitting
        ଯଦି (refp->access().isRW()) block(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        ଯଦି (info.blocked) ଫେରନ୍ତୁ;
        // Record reference
        ଯଦି (refp->access().isReadOnly()) {
            info.rdRefs.ଠେଲ_back({exprp, lsb, msb});
            ଫେରନ୍ତୁ;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        ଅଚଳ ତାର୍କିକ canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.ଠେଲ_back({exprp, canExpand ? m_assignp : nullptr, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    ଶୂନ୍ୟ visitAssignment(Ast_ଗଣ୍ଠି_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? nullptr : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        ଯଦି (fragVarpr) ଫେରନ୍ତୁ fragVarpr;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_ଗଣ୍ଠି::encode_ସଂଖ୍ୟା(msb);
        ଯଦି (width > 1) name += Ast_ଗଣ୍ଠି::encodeName(":") + Ast_ଗଣ୍ଠି::encode_ସଂଖ୍ୟା(lsb);
        name += "__KET__";
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = varp->dtypep()->isFourstate()
                                         ? varp->findLogicDType(width, width, VSigning::UNSIGNED)
                                         : varp->findBitDType(width, width, VSigning::UNSIGNED);
        fragVarpr = ନୂତନ AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagateSplitAttrFrom(varp);
        varp->addHereThisAsNext(fragVarpr);
        ଫେରନ୍ତୁ fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    ଶୂନ୍ୟ decide(AstVarScope* vscp, VscpInfo& info) {
        ଯଦି (info.blocked) ଫେରନ୍ତୁ;
        AstVar* ଅଚଳ varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        ଅଚଳ ତାର୍କିକ isAuto = [&]() {
            std::vector<std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>> ranges;  // [lsb, msb] inclusive
            ପାଇଁ (ଅଚଳ RdRef& ref : info.rdRefs) {
                ଯଦି (VN_IS(ref.exprp, VarRef)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            ପାଇଁ (ଅଚଳ WrRef& ref : info.wrRefs) {
                ଯଦି (VN_IS(ref.exprp, VarRef)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            std::sort(ranges.begin(), ranges.end());
            ପାଇଁ (size_t i = 0; i + 1 < ranges.size(); ++i) {
                ଅଚଳ std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>& a = ranges[i];
                ଅଚଳ std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>& b = ranges[i + 1];
                ଯଦି (a != b && a.second >= b.first) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }();
        // Otherwise only if requested
        ଯଦି (!isAuto && !varp->attrSplitVar()) ଫେରନ୍ତୁ;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = varp->width();
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = -1;
        ପାଇଁ (ଅଚଳ RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା> bounds;
        ପାଇଁ (ଅଚଳ WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.ଠେଲ_back(ref.lsb);
            bounds.ଠେଲ_back(ref.msb + 1);
        }
        bounds.ଠେଲ_back(lsb);
        bounds.ଠେଲ_back(msb + 1);
        std::sort(bounds.begin(), bounds.end());
        bounds.erase(std::unique(bounds.begin(), bounds.end()), bounds.end());
        // Nothing to do if the only fragment is the whole variable
        ଯଦି (bounds.size() == 2 && lsb == 0 && msb == varp->width() - 1) ଫେରନ୍ତୁ;
        // Writes that cannot be expanded must write one fragment
        ପାଇଁ (ଅଚଳ WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            ଯଦି (ref.assignp) ଜାରି;
            // If write is to a whole fragment, it can be replaced
            ଯଦି (*std::upper_bound(bounds.begin(), bounds.end(), ref.lsb) == ref.msb + 1) {
                ଜାରି;
            }
            // Otherwise can't split
            warnNoSplit(varp, ref.exprp, "it is written in an unsupported way");
            ଫେରନ୍ତୁ;
        }

        // Splitting this variable
        ++(isAuto ? m_statSplitAuto : m_statSplitAttr);

        // Create the fragment AstVarScopes
        ପାଇଁ (size_t i = 0; i + 1 < bounds.size(); ++i) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = bounds[i];
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = bounds[i + 1] - 1;
            AstVar* ଅଚଳ fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* ଅଚଳ newp = ନୂତନ AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->addHereThisAsNext(newp);
            info.fragments.ଠେଲ_back({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    ସ୍ଥିର size_t fragmentIndex(ଅଚଳ VscpInfo& info, ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit) {
        ଅଚଳ auto it = std::upper_bound(info.fragments.begin(), info.fragments.end(), bit,
                                         [](ପୂର୍ଣ୍ଣସଂଖ୍ୟା b, ଅଚଳ Fragment& fragment) {  //
                                             ଫେରନ୍ତୁ b <= fragment.msb;
                                         });
        ଫେରନ୍ତୁ it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    ଶୂନ୍ୟ rewriteRead(ଅଚଳ RdRef& ref, ଅଚଳ VscpInfo& info) {
        File_ଧାଡ଼ି* ଅଚଳ flp = ref.exprp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resultp = nullptr;
        ପାଇଁ (size_t i = fragmentIndex(info, ref.lsb); i < info.fragments.size(); ++i) {
            ଅଚଳ Fragment& fragment = info.fragments[i];
            ଯଦି (fragment.lsb > ref.msb) ଭାଙ୍ଗ;
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା partLsb = std::max(ref.lsb, fragment.lsb);
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା partMsb = std::min(ref.msb, fragment.msb);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bitsp = ନୂତନ AstVarRef{flp, fragment.vscp, VAccess::READ};
            ଯଦି (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = ନୂତନ AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? ନୂତନ AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(ଠେଲ_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    ସ୍ଥିର ତାର୍କିକ isSelectable(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (VN_IS(nodep, Const)) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (ଅଚଳ AstSel* ଅଚଳ selp = VN_CAST(nodep, Sel)) {
            ଯଦି (!VN_IS(selp->lsbp(), Const)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            nodep = selp->fromp();
        }
        ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(nodep, VarRef);
        ଯଦି (!refp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Not a forced variable (trips several bugs in V3Force)
        ଯଦି (refp->varp()->isForced()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        ଫେରନ୍ତୁ !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    ଶୂନ୍ୟ rewriteWrite(AstScope* scopep, ଅଚଳ WrRef& ref, ଅଚଳ VscpInfo& info) {
        File_ଧାଡ଼ି* ଅଚଳ flp = ref.exprp->fileline();
        ଅଚଳ size_t first = fragmentIndex(info, ref.lsb);
        ଅଚଳ size_t last = fragmentIndex(info, ref.msb);
        UASSERT_OBJ(info.fragments[first].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        ଯଦି (first == last) {
            ref.exprp->replaceWith(ନୂତନ AstVarRef{flp, info.fragments[first].vscp, VAccess::WRITE});
            VL_DO_DANGLING(ଠେଲ_Deletep(ref.exprp), ref.exprp);
            ଫେରନ୍ତୁ;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_ଗଣ୍ଠି_Assign* ଅଚଳ origp = ref.assignp;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp = origp->rhsp()->unlinkFrBack();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        ଯଦି (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* ଅଚଳ tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* ଅଚଳ tmpRefp = ନୂତନ AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->addHereThisAsNext(ନୂତନ AstAssign{flp, tmpRefp, rhsp});
            rhsp = ନୂତନ AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        ପାଇଁ (size_t i = first; i <= last; ++i) {
            ଅଚଳ Fragment& fragment = info.fragments[i];
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = fragment.msb - fragment.lsb + 1;
            AstVarRef* ଅଚଳ lhsp = ନୂତନ AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* ଅଚଳ selp = ନୂତନ AstSel{flp, rhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), lsb, width};
            origp->addHereThisAsNext(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ଠେଲ_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    ସ୍ଥିର AstActive* comboActive(AstScope* scopep) {
        ଯଦି (!scopep->user2p()) scopep->user2p(scopep->comboActivep(ସତ୍ୟ));
        ଫେରନ୍ତୁ VN_AS(scopep->user2p(), Active);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_DType*) override {}  // No references in data types
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }
    ଶୂନ୍ୟ visit(AstAssign* nodep) override { visitAssignment(nodep); }
    ଶୂନ୍ୟ visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    ଶୂନ୍ୟ visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        AstVarRef* ଅଚଳ refp = VN_CAST(nodep->fromp(), VarRef);
        ଯଦି (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            ଫେରନ୍ତୁ;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        ଅଚଳ AstConst* ଅଚଳ lsbp = VN_CAST(nodep->lsbp(), Const);
        ଯଦି (!lsbp || lsbp->toSInt() + nodep->widthConst() > refp->varp()->width()) {
            block(refp->varp(), vscpInfoOf(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            ଫେରନ୍ତୁ;
        }
        // Record partial access
        record(nodep, refp, lsbp->toSInt(), lsbp->toSInt() + nodep->widthConst() - 1);
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        // Record whole access
        record(nodep, nodep, 0, nodep->varp()->width() - 1);
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        // The AstVarScope is not known, so all of them are blocked, by making it ineligible
        AstVar* ଅଚଳ varp = nodep->varp();
        ଯଦି (!isEligible(varp)) ଫେରନ୍ତୁ;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    explicit BitBlastVisitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        ପାଇଁ (AstVarScope* ଅଚଳ vscp : m_vscps) {
            ଯଦି (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        ପାଇଁ (AstVarScope* ଅଚଳ vscp : m_vscps) {
            ଅଚଳ VscpInfo& info = m_vscpInfos(vscp);
            ଯଦି (info.fragments.empty()) ଜାରି;
            ପାଇଁ (ଅଚଳ RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        ପାଇଁ (AstVarScope* ଅଚଳ vscp : m_vscps) {
            ଅଚଳ VscpInfo& info = m_vscpInfos(vscp);
            ଯଦି (info.fragments.empty()) ଜାରି;
            ପାଇଁ (ଅଚଳ WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        ପାଇଁ (AstVarScope* ଅଚଳ vscp : m_vscps) {
            ଅଚଳ VscpInfo& info = m_vscpInfos(vscp);
            ଯଦି (info.fragments.empty()) ଜାରି;
            ଯଦି (!(vscp->varp()->isTrace() && vscp->isTrace())) ଜାରି;
            AstActive* ଅଚଳ activep = comboActive(vscp->scopep());
            File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
            ପାଇଁ (ଅଚଳ Fragment& fragment : info.fragments) {
                AstVarRef* ଅଚଳ lhsRefp = ନୂତନ AstVarRef{flp, vscp, VAccess::WRITE};
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = fragment.msb - fragment.lsb + 1;
                AstSel* ଅଚଳ lhsp = ନୂତନ AstSel{flp, lhsRefp, fragment.lsb, width};
                AstVarRef* ଅଚଳ rhsp = ନୂତନ AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->addStmtsp(ନୂତନ AstAlways{ନୂତନ AstAssignW{flp, lhsp, rhsp}});
            }
        }

        V3Stats::addStat("Optimizations, Bitblast, variables split due to attribute",
                         m_statSplitAttr);
        V3Stats::addStat("Optimizations, Bitblast, variables split automatically",
                         m_statSplitAuto);
        V3Stats::addStat("Optimizations, Bitblast, fragments created", m_statFragments);
        V3Stats::addStat("Optimizations, Bitblast, assignments through temporary", m_statTmps);
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(AstNetlist* netlistp) { BitBlastVisitor{netlistp}; }
};

//######################################################################
// V3BitBlast class functions

ଶୂନ୍ୟ V3BitBlast::bitBlastAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    BitBlastVisitor::apply(nodep);
    V3Global::dump_Check_Global_ବୃକ୍ଷ("bitblast", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
