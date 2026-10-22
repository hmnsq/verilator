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

ವರ್ಗ ಬಿಟ್_Blast_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst, ಸಾರ್ವಜನಿಕ VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    ರಚನೆ RdRef final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp;  // Sel, or the VarRef if whole
        ಪೂರ್ಣಾಂಕ lsb;  // First bit read
        ಪೂರ್ಣಾಂಕ msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    ರಚನೆ WrRef final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp;  // The Sel, or the VarRef if whole
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* assignp;  // The assignment, if it can be expanded
        ಪೂರ್ಣಾಂಕ lsb;  // The first bit written
        ಪೂರ್ಣಾಂಕ msb;  // The last bit written
    };

    // A fragment of a split AstVarScope
    ರಚನೆ Fragment final {
        ಪೂರ್ಣಾಂಕ lsb;  // The first bit in the variable
        ಪೂರ್ಣಾಂಕ msb;  // The last bit in the variable
        AstVarScope* vscp;  // The AstVarScope of the fragment
    };

    // The information about an AstVarScope of an eligible variable
    ರಚನೆ VscpInfo final {
        std::ಸದಿಶ<RdRef> rdRefs;  // The reads of the AstVarScope
        std::ಸದಿಶ<WrRef> wrRefs;  // The writes of the AstVarScope
        std::ಸದಿಶ<Fragment> fragments;  // The fragments in LSB order, empty if not split
        ತಾರ್ಕಿಕ blocked = ಸುಳ್ಳು;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_user2_ಒಳ_Use;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::ಸದಿಶ<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* m_assignp = ಶೂನ್ಯಸೂಚಕ;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_stat_ವಿಭಜಿಸು_Attr;  // AstVarScopes split only due to split_var
    VDouble0 m_stat_ವಿಭಜಿಸು_Auto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ಸ್ಥಿರ ಶೂನ್ಯ warn_No_ವಿಭಜಿಸು(AstVar* varp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* wherep, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* reasonp) {
        // Only warn if user requested splitting
        ಆದರೆ (!varp->attr_ವಿಭಜಿಸು_Var()) ಹಿಂತಿರುಗಿ;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::SPLITVAR, ನಿಜ);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        ಆದರೆ (!varp->user1()) {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ eligible = [&]() {
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                ಆದರೆ (!dtypep->isIntegralOrPacked()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                // Wider than one bit
                ಆದರೆ (dtypep->ಅಗಲ() <= 1) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                // Check properties
                ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ reasonp = varp->cannot_ವಿಭಜಿಸು_ಬಗೆ_Reason();
                // Warn that it cannot be split if user explicitly requested splitting
                ಆದರೆ (reasonp) warn_No_ವಿಭಜಿಸು(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                ಹಿಂತಿರುಗಿ !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        ಹಿಂತಿರುಗಿ varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscp_Info_ದ(AstVarScope* vscp) {
        ಆದರೆ (VscpInfo* ಸ್ಥಿರಾಂಕ infop = m_vscpInfos.tryGet(vscp)) ಹಿಂತಿರುಗಿ *infop;
        // Also record on first encounter
        m_vscps.ತಳ್ಳು_ಹಿಂಭಾಗ(vscp);
        ಹಿಂತಿರುಗಿ m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    ಸ್ಥಿರ ಶೂನ್ಯ ಖಂಡ(AstVar* varp, VscpInfo& info, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* wherep) {
        warn_No_ವಿಭಜಿಸು(varp, wherep, "it is referenced in an unsupported way");
        ಆದರೆ (info.blocked) ಹಿಂತಿರುಗಿ;
        info.blocked = ನಿಜ;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    ಶೂನ್ಯ ದಾಖಲೆ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstVarRef* refp, ಪೂರ್ಣಾಂಕ lsb, ಪೂರ್ಣಾಂಕ msb) {
        AstVar* ಸ್ಥಿರಾಂಕ varp = refp->varp();
        ಆದರೆ (!isEligible(varp)) ಹಿಂತಿರುಗಿ;
        VscpInfo& info = vscp_Info_ದ(refp->varScopep());
        // RW reference blocks splitting
        ಆದರೆ (refp->access().isRW()) ಖಂಡ(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        ಆದರೆ (info.blocked) ಹಿಂತಿರುಗಿ;
        // Record reference
        ಆದರೆ (refp->access().isReadOnly()) {
            info.rdRefs.ತಳ್ಳು_ಹಿಂಭಾಗ({exprp, lsb, msb});
            ಹಿಂತಿರುಗಿ;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.ತಳ್ಳು_ಹಿಂಭಾಗ({exprp, canExpand ? m_assignp : ಶೂನ್ಯಸೂಚಕ, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    ಶೂನ್ಯ ಭೇಟಿ_Assignment(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? ಶೂನ್ಯಸೂಚಕ : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, ಪೂರ್ಣಾಂಕ lsb, ಪೂರ್ಣಾಂಕ msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        ಆದರೆ (fragVarpr) ಹಿಂತಿರುಗಿ fragVarpr;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(msb);
        ಆದರೆ (ಅಗಲ > 1) name += Ast_ಗಂಟು::encodeName(":") + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(lsb);
        name += "__KET__";
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = varp->dtypep()->isFourstate()
                                         ? varp->ಕಂಡುಹಿಡಿ_Logic_DType(ಅಗಲ, ಅಗಲ, VSigning::UNSIGNED)
                                         : varp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(ಅಗಲ, ಅಗಲ, VSigning::UNSIGNED);
        fragVarpr = ಹೊಸ AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagate_ವಿಭಜಿಸು_Attr_From(varp);
        varp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(fragVarpr);
        ಹಿಂತಿರುಗಿ fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    ಶೂನ್ಯ decide(AstVarScope* vscp, VscpInfo& info) {
        ಆದರೆ (info.blocked) ಹಿಂತಿರುಗಿ;
        AstVar* ಸ್ಥಿರಾಂಕ varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isAuto = [&]() {
            std::ಸದಿಶ<std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>> ranges;  // [lsb, msb] inclusive
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ RdRef& ref : info.rdRefs) {
                ಆದರೆ (VN_IS(ref.exprp, VarRef)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                ranges.emplace_ಹಿಂಭಾಗ(ref.lsb, ref.msb);
            }
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ WrRef& ref : info.wrRefs) {
                ಆದರೆ (VN_IS(ref.exprp, VarRef)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                ranges.emplace_ಹಿಂಭಾಗ(ref.lsb, ref.msb);
            }
            std::ವಿಂಗಡಿಸು(ranges.begin(), ranges.ಕೊನೆ());
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i + 1 < ranges.ಗಾತ್ರ_2(); ++i) {
                ಸ್ಥಿರಾಂಕ std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>& a = ranges[i];
                ಸ್ಥಿರಾಂಕ std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>& b = ranges[i + 1];
                ಆದರೆ (a != b && a.ಎರಡನೇ >= b.ಮೊದಲ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }();
        // Otherwise only if requested
        ಆದರೆ (!isAuto && !varp->attr_ವಿಭಜಿಸು_Var()) ಹಿಂತಿರುಗಿ;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        ಪೂರ್ಣಾಂಕ lsb = varp->ಅಗಲ();
        ಪೂರ್ಣಾಂಕ msb = -1;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ> bounds;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.ತಳ್ಳು_ಹಿಂಭಾಗ(ref.lsb);
            bounds.ತಳ್ಳು_ಹಿಂಭಾಗ(ref.msb + 1);
        }
        bounds.ತಳ್ಳು_ಹಿಂಭಾಗ(lsb);
        bounds.ತಳ್ಳು_ಹಿಂಭಾಗ(msb + 1);
        std::ವಿಂಗಡಿಸು(bounds.begin(), bounds.ಕೊನೆ());
        bounds.erase(std::unique(bounds.begin(), bounds.ಕೊನೆ()), bounds.ಕೊನೆ());
        // Nothing to do if the only fragment is the whole variable
        ಆದರೆ (bounds.ಗಾತ್ರ_2() == 2 && lsb == 0 && msb == varp->ಅಗಲ() - 1) ಹಿಂತಿರುಗಿ;
        // Writes that cannot be expanded must write one fragment
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            ಆದರೆ (ref.assignp) ಮುಂದುವರಿ;
            // If write is to a whole fragment, it can be replaced
            ಆದರೆ (*std::ಊರ್ಧ್ವ_bound(bounds.begin(), bounds.ಕೊನೆ(), ref.lsb) == ref.msb + 1) {
                ಮುಂದುವರಿ;
            }
            // Otherwise can't split
            warn_No_ವಿಭಜಿಸು(varp, ref.exprp, "it is written in an unsupported way");
            ಹಿಂತಿರುಗಿ;
        }

        // Splitting this variable
        ++(isAuto ? m_stat_ವಿಭಜಿಸು_Auto : m_stat_ವಿಭಜಿಸು_Attr);

        // Create the fragment AstVarScopes
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i + 1 < bounds.ಗಾತ್ರ_2(); ++i) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb = bounds[i];
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ msb = bounds[i + 1] - 1;
            AstVar* ಸ್ಥಿರಾಂಕ fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(newp);
            info.fragments.ತಳ್ಳು_ಹಿಂಭಾಗ({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    ಸ್ಥಿರ ಗಾತ್ರ_t fragment_ಸೂಚ್ಯಂಕ(ಸ್ಥಿರಾಂಕ VscpInfo& info, ಪೂರ್ಣಾಂಕ ಬಿಟ್_2) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = std::ಊರ್ಧ್ವ_bound(info.fragments.begin(), info.fragments.ಕೊನೆ(), ಬಿಟ್_2,
                                         [](ಪೂರ್ಣಾಂಕ b, ಸ್ಥಿರಾಂಕ Fragment& fragment) {  //
                                             ಹಿಂತಿರುಗಿ b <= fragment.msb;
                                         });
        ಹಿಂತಿರುಗಿ it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    ಶೂನ್ಯ rewriteRead(ಸ್ಥಿರಾಂಕ RdRef& ref, ಸ್ಥಿರಾಂಕ VscpInfo& info) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = ref.exprp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಗಾತ್ರ_t i = fragment_ಸೂಚ್ಯಂಕ(info, ref.lsb); i < info.fragments.ಗಾತ್ರ_2(); ++i) {
            ಸ್ಥಿರಾಂಕ Fragment& fragment = info.fragments[i];
            ಆದರೆ (fragment.lsb > ref.msb) ನಿಲ್ಲಿಸು;
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ partLsb = std::max(ref.lsb, fragment.lsb);
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ partMsb = std::min(ref.msb, fragment.msb);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitsp = ಹೊಸ AstVarRef{flp, fragment.vscp, VAccess::READ};
            ಆದರೆ (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = ಹೊಸ AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? ಹೊಸ AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSelectable(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (VN_IS(nodep, Const)) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, Sel)) {
            ಆದರೆ (!VN_IS(selp->lsbp(), Const)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            nodep = selp->fromp();
        }
        ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, VarRef);
        ಆದರೆ (!refp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Not a forced variable (trips several bugs in V3Force)
        ಆದರೆ (refp->varp()->isForced()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        ಹಿಂತಿರುಗಿ !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    ಶೂನ್ಯ rewriteWrite(AstScope* scopep, ಸ್ಥಿರಾಂಕ WrRef& ref, ಸ್ಥಿರಾಂಕ VscpInfo& info) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = ref.exprp->fileline();
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಮೊದಲ = fragment_ಸೂಚ್ಯಂಕ(info, ref.lsb);
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಕೊನೆಯ = fragment_ಸೂಚ್ಯಂಕ(info, ref.msb);
        UASSERT_OBJ(info.fragments[ಮೊದಲ].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        ಆದರೆ (ಮೊದಲ == ಕೊನೆಯ) {
            ref.exprp->replaceWith(ಹೊಸ AstVarRef{flp, info.fragments[ಮೊದಲ].vscp, VAccess::WRITE});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(ref.exprp), ref.exprp);
            ಹಿಂತಿರುಗಿ;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ origp = ref.assignp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp = origp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        ಆದರೆ (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* ಸ್ಥಿರಾಂಕ tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* ಸ್ಥಿರಾಂಕ tmpRefp = ಹೊಸ AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, tmpRefp, rhsp});
            rhsp = ಹೊಸ AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        ಗಾಗಿ (ಗಾತ್ರ_t i = ಮೊದಲ; i <= ಕೊನೆಯ; ++i) {
            ಸ್ಥಿರಾಂಕ Fragment& fragment = info.fragments[i];
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = fragment.msb - fragment.lsb + 1;
            AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* ಸ್ಥಿರಾಂಕ selp = ಹೊಸ AstSel{flp, rhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), lsb, ಅಗಲ};
            origp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(origp->unlink_Fr_ಹಿಂಭಾಗ()), origp);
    }

    // The combinational AstActive of the scope, cached
    ಸ್ಥಿರ Ast_ಸಕ್ರಿಯ* combo_ಸಕ್ರಿಯ(AstScope* scopep) {
        ಆದರೆ (!scopep->user2p()) scopep->user2p(scopep->comboActivep(ನಿಜ));
        ಹಿಂತಿರುಗಿ VN_AS(scopep->user2p(), ಸಕ್ರಿಯ);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_DType*) override {}  // No references in data types
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು* nodep) override { ಭೇಟಿ_Assignment(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_W* nodep) override { ಭೇಟಿ_Assignment(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Dly* nodep) override { ಭೇಟಿ_Assignment(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep->fromp(), VarRef);
        ಆದರೆ (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ lsbp = VN_CAST(nodep->lsbp(), Const);
        ಆದರೆ (!lsbp || lsbp->ಗಮ್ಯ_SInt() + nodep->ಅಗಲ_Const() > refp->varp()->ಅಗಲ()) {
            ಖಂಡ(refp->varp(), vscp_Info_ದ(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            ಹಿಂತಿರುಗಿ;
        }
        // Record partial access
        ದಾಖಲೆ(nodep, refp, lsbp->ಗಮ್ಯ_SInt(), lsbp->ಗಮ್ಯ_SInt() + nodep->ಅಗಲ_Const() - 1);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        // Record whole access
        ದಾಖಲೆ(nodep, nodep, 0, nodep->varp()->ಅಗಲ() - 1);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        // The AstVarScope is not known, so all of them are blocked, by making it ineligible
        AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        ಆದರೆ (!isEligible(varp)) ಹಿಂತಿರುಗಿ;
        varp->user1(2);  // Mark not eligible
        warn_No_ವಿಭಜಿಸು(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ಬಿಟ್_Blast_Visitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp : m_vscps) {
            ಆದರೆ (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp : m_vscps) {
            ಸ್ಥಿರಾಂಕ VscpInfo& info = m_vscpInfos(vscp);
            ಆದರೆ (info.fragments.ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp : m_vscps) {
            ಸ್ಥಿರಾಂಕ VscpInfo& info = m_vscpInfos(vscp);
            ಆದರೆ (info.fragments.ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp : m_vscps) {
            ಸ್ಥಿರಾಂಕ VscpInfo& info = m_vscpInfos(vscp);
            ಆದರೆ (info.fragments.ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
            ಆದರೆ (!(vscp->varp()->isTrace() && vscp->isTrace())) ಮುಂದುವರಿ;
            Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = combo_ಸಕ್ರಿಯ(vscp->scopep());
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Fragment& fragment : info.fragments) {
                AstVarRef* ಸ್ಥಿರಾಂಕ lhsRefp = ಹೊಸ AstVarRef{flp, vscp, VAccess::WRITE};
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = fragment.msb - fragment.lsb + 1;
                AstSel* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstSel{flp, lhsRefp, fragment.lsb, ಅಗಲ};
                AstVarRef* ಸ್ಥಿರಾಂಕ rhsp = ಹೊಸ AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{flp, lhsp, rhsp}});
            }
        }

        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Bitblast, variables split due to attribute",
                         m_stat_ವಿಭಜಿಸು_Attr);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Bitblast, variables split automatically",
                         m_stat_ವಿಭಜಿಸು_Auto);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Bitblast, fragments created", m_statFragments);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Bitblast, assignments through temporary", m_statTmps);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(AstNetlist* netlistp) { ಬಿಟ್_Blast_Visitor{netlistp}; }
};

//######################################################################
// V3BitBlast class functions

ಶೂನ್ಯ V3_ಬಿಟ್_Blast::ಬಿಟ್_Blast_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    ಬಿಟ್_Blast_Visitor::apply(nodep);
    V3Global::dump_Check_Global_ವೃಕ್ಷ("bitblast", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
