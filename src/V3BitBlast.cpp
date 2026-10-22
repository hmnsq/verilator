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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3BitBlast.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3SharedTmps.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <algorithm>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ BitBlastVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst, ಸಾರ್ವಜನಿಕ VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    ಸಂರಚನಾ RdRef final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp;  // Sel, or the VarRef if whole
        ಪೂರ್ಣಾಙ್ಕ lsb;  // First bit read
        ಪೂರ್ಣಾಙ್ಕ msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    ಸಂರಚನಾ WrRef final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp;  // The Sel, or the VarRef if whole
        Ast_ಗಂಟು_Assign* assignp;  // The assignment, if it can be expanded
        ಪೂರ್ಣಾಙ್ಕ lsb;  // The first bit written
        ಪೂರ್ಣಾಙ್ಕ msb;  // The last bit written
    };

    // A fragment of a split AstVarScope
    ಸಂರಚನಾ Fragment final {
        ಪೂರ್ಣಾಙ್ಕ lsb;  // The first bit in the variable
        ಪೂರ್ಣಾಙ್ಕ msb;  // The last bit in the variable
        AstVarScope* vscp;  // The AstVarScope of the fragment
    };

    // The information about an AstVarScope of an eligible variable
    ಸಂರಚನಾ VscpInfo final {
        std::vector<RdRef> rdRefs;  // The reads of the AstVarScope
        std::vector<WrRef> wrRefs;  // The writes of the AstVarScope
        std::vector<Fragment> fragments;  // The fragments in LSB order, empty if not split
        ತಾರ್ಕಿಕ blocked = ಅಸತ್ಯ;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    ಧ್ರುವ VNUser1InUse m_user1InUse;
    ಧ್ರುವ VNUser2InUse m_user2InUse;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::vector<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_ಗಂಟು_Assign* m_assignp = nullptr;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_statSplitAttr;  // AstVarScopes split only due to split_var
    VDouble0 m_statSplitAuto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ಸ್ಥಿರ ಶೂನ್ಯ warnNoSplit(AstVar* varp, ಧ್ರುವ Ast_ಗಂಟು* wherep, ಧ್ರುವ ವರ್ಣ* reasonp) {
        // Only warn if user requested splitting
        ಯದಿ (!varp->attrSplitVar()) ಪ್ರತ್ಯಾಗಮ;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, ಸತ್ಯ);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        ಯದಿ (!varp->user1()) {
            ಧ್ರುವ ತಾರ್ಕಿಕ eligible = [&]() {
                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                ಯದಿ (!dtypep->isIntegralOrPacked()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                // Wider than one bit
                ಯದಿ (dtypep->width() <= 1) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                // Check properties
                ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                ಯದಿ (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                ಪ್ರತ್ಯಾಗಮ !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        ಪ್ರತ್ಯಾಗಮ varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscpInfoOf(AstVarScope* vscp) {
        ಯದಿ (VscpInfo* ಧ್ರುವ infop = m_vscpInfos.tryGet(vscp)) ಪ್ರತ್ಯಾಗಮ *infop;
        // Also record on first encounter
        m_vscps.ತಳ್ಳು_back(vscp);
        ಪ್ರತ್ಯಾಗಮ m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    ಸ್ಥಿರ ಶೂನ್ಯ block(AstVar* varp, VscpInfo& info, ಧ್ರುವ Ast_ಗಂಟು* wherep) {
        warnNoSplit(varp, wherep, "it is referenced in an unsupported way");
        ಯದಿ (info.blocked) ಪ್ರತ್ಯಾಗಮ;
        info.blocked = ಸತ್ಯ;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    ಶೂನ್ಯ record(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstVarRef* refp, ಪೂರ್ಣಾಙ್ಕ lsb, ಪೂರ್ಣಾಙ್ಕ msb) {
        AstVar* ಧ್ರುವ varp = refp->varp();
        ಯದಿ (!isEligible(varp)) ಪ್ರತ್ಯಾಗಮ;
        VscpInfo& info = vscpInfoOf(refp->varScopep());
        // RW reference blocks splitting
        ಯದಿ (refp->access().isRW()) block(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        ಯದಿ (info.blocked) ಪ್ರತ್ಯಾಗಮ;
        // Record reference
        ಯದಿ (refp->access().isReadOnly()) {
            info.rdRefs.ತಳ್ಳು_back({exprp, lsb, msb});
            ಪ್ರತ್ಯಾಗಮ;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        ಧ್ರುವ ತಾರ್ಕಿಕ canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.ತಳ್ಳು_back({exprp, canExpand ? m_assignp : nullptr, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    ಶೂನ್ಯ visitAssignment(Ast_ಗಂಟು_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? nullptr : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, ಪೂರ್ಣಾಙ್ಕ lsb, ಪೂರ್ಣಾಙ್ಕ msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        ಯದಿ (fragVarpr) ಪ್ರತ್ಯಾಗಮ fragVarpr;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(msb);
        ಯದಿ (width > 1) name += Ast_ಗಂಟು::encodeName(":") + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(lsb);
        name += "__KET__";
        Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = varp->dtypep()->isFourstate()
                                         ? varp->findLogicDType(width, width, VSigning::UNSIGNED)
                                         : varp->findBitDType(width, width, VSigning::UNSIGNED);
        fragVarpr = ನೂತನ AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagateSplitAttrFrom(varp);
        varp->addHereThisAsNext(fragVarpr);
        ಪ್ರತ್ಯಾಗಮ fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    ಶೂನ್ಯ decide(AstVarScope* vscp, VscpInfo& info) {
        ಯದಿ (info.blocked) ಪ್ರತ್ಯಾಗಮ;
        AstVar* ಧ್ರುವ varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        ಧ್ರುವ ತಾರ್ಕಿಕ isAuto = [&]() {
            std::vector<std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>> ranges;  // [lsb, msb] inclusive
            ಪ್ರತಿ (ಧ್ರುವ RdRef& ref : info.rdRefs) {
                ಯದಿ (VN_IS(ref.exprp, VarRef)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            ಪ್ರತಿ (ಧ್ರುವ WrRef& ref : info.wrRefs) {
                ಯದಿ (VN_IS(ref.exprp, VarRef)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            std::sort(ranges.begin(), ranges.end());
            ಪ್ರತಿ (size_t i = 0; i + 1 < ranges.size(); ++i) {
                ಧ್ರುವ std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>& a = ranges[i];
                ಧ್ರುವ std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>& b = ranges[i + 1];
                ಯದಿ (a != b && a.second >= b.first) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }();
        // Otherwise only if requested
        ಯದಿ (!isAuto && !varp->attrSplitVar()) ಪ್ರತ್ಯಾಗಮ;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        ಪೂರ್ಣಾಙ್ಕ lsb = varp->width();
        ಪೂರ್ಣಾಙ್ಕ msb = -1;
        ಪ್ರತಿ (ಧ್ರುವ RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::vector<ಪೂರ್ಣಾಙ್ಕ> bounds;
        ಪ್ರತಿ (ಧ್ರುವ WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.ತಳ್ಳು_back(ref.lsb);
            bounds.ತಳ್ಳು_back(ref.msb + 1);
        }
        bounds.ತಳ್ಳು_back(lsb);
        bounds.ತಳ್ಳು_back(msb + 1);
        std::sort(bounds.begin(), bounds.end());
        bounds.erase(std::unique(bounds.begin(), bounds.end()), bounds.end());
        // Nothing to do if the only fragment is the whole variable
        ಯದಿ (bounds.size() == 2 && lsb == 0 && msb == varp->width() - 1) ಪ್ರತ್ಯಾಗಮ;
        // Writes that cannot be expanded must write one fragment
        ಪ್ರತಿ (ಧ್ರುವ WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            ಯದಿ (ref.assignp) ಅನುವರ್ತ;
            // If write is to a whole fragment, it can be replaced
            ಯದಿ (*std::upper_bound(bounds.begin(), bounds.end(), ref.lsb) == ref.msb + 1) {
                ಅನುವರ್ತ;
            }
            // Otherwise can't split
            warnNoSplit(varp, ref.exprp, "it is written in an unsupported way");
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Splitting this variable
        ++(isAuto ? m_statSplitAuto : m_statSplitAttr);

        // Create the fragment AstVarScopes
        ಪ್ರತಿ (size_t i = 0; i + 1 < bounds.size(); ++i) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb = bounds[i];
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ msb = bounds[i + 1] - 1;
            AstVar* ಧ್ರುವ fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* ಧ್ರುವ newp = ನೂತನ AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->addHereThisAsNext(newp);
            info.fragments.ತಳ್ಳು_back({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    ಸ್ಥಿರ size_t fragmentIndex(ಧ್ರುವ VscpInfo& info, ಪೂರ್ಣಾಙ್ಕ bit) {
        ಧ್ರುವ auto it = std::upper_bound(info.fragments.begin(), info.fragments.end(), bit,
                                         [](ಪೂರ್ಣಾಙ್ಕ b, ಧ್ರುವ Fragment& fragment) {  //
                                             ಪ್ರತ್ಯಾಗಮ b <= fragment.msb;
                                         });
        ಪ್ರತ್ಯಾಗಮ it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    ಶೂನ್ಯ rewriteRead(ಧ್ರುವ RdRef& ref, ಧ್ರುವ VscpInfo& info) {
        File_ಸಾಲು* ಧ್ರುವ flp = ref.exprp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = nullptr;
        ಪ್ರತಿ (size_t i = fragmentIndex(info, ref.lsb); i < info.fragments.size(); ++i) {
            ಧ್ರುವ Fragment& fragment = info.fragments[i];
            ಯದಿ (fragment.lsb > ref.msb) ಭಙ್ಗ;
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ partLsb = std::max(ref.lsb, fragment.lsb);
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ partMsb = std::min(ref.msb, fragment.msb);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitsp = ನೂತನ AstVarRef{flp, fragment.vscp, VAccess::READ};
            ಯದಿ (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = ನೂತನ AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? ನೂತನ AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSelectable(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (VN_IS(nodep, Const)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (ಧ್ರುವ AstSel* ಧ್ರುವ selp = VN_CAST(nodep, Sel)) {
            ಯದಿ (!VN_IS(selp->lsbp(), Const)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            nodep = selp->fromp();
        }
        ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep, VarRef);
        ಯದಿ (!refp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Not a forced variable (trips several bugs in V3Force)
        ಯದಿ (refp->varp()->isForced()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        ಪ್ರತ್ಯಾಗಮ !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    ಶೂನ್ಯ rewriteWrite(AstScope* scopep, ಧ್ರುವ WrRef& ref, ಧ್ರುವ VscpInfo& info) {
        File_ಸಾಲು* ಧ್ರುವ flp = ref.exprp->fileline();
        ಧ್ರುವ size_t first = fragmentIndex(info, ref.lsb);
        ಧ್ರುವ size_t last = fragmentIndex(info, ref.msb);
        UASSERT_OBJ(info.fragments[first].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        ಯದಿ (first == last) {
            ref.exprp->replaceWith(ನೂತನ AstVarRef{flp, info.fragments[first].vscp, VAccess::WRITE});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(ref.exprp), ref.exprp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_ಗಂಟು_Assign* ಧ್ರುವ origp = ref.assignp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp = origp->rhsp()->unlinkFrBack();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        ಯದಿ (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* ಧ್ರುವ tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* ಧ್ರುವ tmpRefp = ನೂತನ AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->addHereThisAsNext(ನೂತನ AstAssign{flp, tmpRefp, rhsp});
            rhsp = ನೂತನ AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        ಪ್ರತಿ (size_t i = first; i <= last; ++i) {
            ಧ್ರುವ Fragment& fragment = info.fragments[i];
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = fragment.msb - fragment.lsb + 1;
            AstVarRef* ಧ್ರುವ lhsp = ನೂತನ AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* ಧ್ರುವ selp = ನೂತನ AstSel{flp, rhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), lsb, width};
            origp->addHereThisAsNext(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    ಸ್ಥಿರ AstActive* comboActive(AstScope* scopep) {
        ಯದಿ (!scopep->user2p()) scopep->user2p(scopep->comboActivep(ಸತ್ಯ));
        ಪ್ರತ್ಯಾಗಮ VN_AS(scopep->user2p(), Active);
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_DType*) override {}  // No references in data types
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }
    ಶೂನ್ಯ visit(AstAssign* nodep) override { visitAssignment(nodep); }
    ಶೂನ್ಯ visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    ಶೂನ್ಯ visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    ಶೂನ್ಯ visit(AstSel* nodep) override {
        AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep->fromp(), VarRef);
        ಯದಿ (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        ಧ್ರುವ AstConst* ಧ್ರುವ lsbp = VN_CAST(nodep->lsbp(), Const);
        ಯದಿ (!lsbp || lsbp->toSInt() + nodep->widthConst() > refp->varp()->width()) {
            block(refp->varp(), vscpInfoOf(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Record partial access
        record(nodep, refp, lsbp->toSInt(), lsbp->toSInt() + nodep->widthConst() - 1);
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        // Record whole access
        record(nodep, nodep, 0, nodep->varp()->width() - 1);
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        // The AstVarScope is not known, so all of them are blocked, by making it ineligible
        AstVar* ಧ್ರುವ varp = nodep->varp();
        ಯದಿ (!isEligible(varp)) ಪ್ರತ್ಯಾಗಮ;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    explicit BitBlastVisitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        ಪ್ರತಿ (AstVarScope* ಧ್ರುವ vscp : m_vscps) {
            ಯದಿ (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        ಪ್ರತಿ (AstVarScope* ಧ್ರುವ vscp : m_vscps) {
            ಧ್ರುವ VscpInfo& info = m_vscpInfos(vscp);
            ಯದಿ (info.fragments.empty()) ಅನುವರ್ತ;
            ಪ್ರತಿ (ಧ್ರುವ RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        ಪ್ರತಿ (AstVarScope* ಧ್ರುವ vscp : m_vscps) {
            ಧ್ರುವ VscpInfo& info = m_vscpInfos(vscp);
            ಯದಿ (info.fragments.empty()) ಅನುವರ್ತ;
            ಪ್ರತಿ (ಧ್ರುವ WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        ಪ್ರತಿ (AstVarScope* ಧ್ರುವ vscp : m_vscps) {
            ಧ್ರುವ VscpInfo& info = m_vscpInfos(vscp);
            ಯದಿ (info.fragments.empty()) ಅನುವರ್ತ;
            ಯದಿ (!(vscp->varp()->isTrace() && vscp->isTrace())) ಅನುವರ್ತ;
            AstActive* ಧ್ರುವ activep = comboActive(vscp->scopep());
            File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
            ಪ್ರತಿ (ಧ್ರುವ Fragment& fragment : info.fragments) {
                AstVarRef* ಧ್ರುವ lhsRefp = ನೂತನ AstVarRef{flp, vscp, VAccess::WRITE};
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = fragment.msb - fragment.lsb + 1;
                AstSel* ಧ್ರುವ lhsp = ನೂತನ AstSel{flp, lhsRefp, fragment.lsb, width};
                AstVarRef* ಧ್ರುವ rhsp = ನೂತನ AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->addStmtsp(ನೂತನ AstAlways{ನೂತನ AstAssignW{flp, lhsp, rhsp}});
            }
        }

        V3Stats::addStat("Optimizations, Bitblast, variables split due to attribute",
                         m_statSplitAttr);
        V3Stats::addStat("Optimizations, Bitblast, variables split automatically",
                         m_statSplitAuto);
        V3Stats::addStat("Optimizations, Bitblast, fragments created", m_statFragments);
        V3Stats::addStat("Optimizations, Bitblast, assignments through temporary", m_statTmps);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(AstNetlist* netlistp) { BitBlastVisitor{netlistp}; }
};

//######################################################################
// V3BitBlast class functions

ಶೂನ್ಯ V3BitBlast::bitBlastAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    BitBlastVisitor::apply(nodep);
    V3Global::dump_Check_Global_ವೃಕ್ಷ("bitblast", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
