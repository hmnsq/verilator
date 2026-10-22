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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3BitBlast.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SharedTmps.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ ਦੋਆਧਾਰੀ_ਅੰਕ_Blast_Visitor final : ਜਨਤਕ VNVisitorConst, ਜਨਤਕ VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    ਬਣਤਰ RdRef final {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp;  // Sel, or the VarRef if whole
        ਪੂਰਨ lsb;  // First bit read
        ਪੂਰਨ msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    ਬਣਤਰ WrRef final {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp;  // The Sel, or the VarRef if whole
        Ast_ਗੰਢ_ਸੌਂਪੋ* assignp;  // The assignment, if it can be expanded
        ਪੂਰਨ lsb;  // The first bit written
        ਪੂਰਨ msb;  // The last bit written
    };

    // A fragment of a split AstVarScope
    ਬਣਤਰ Fragment final {
        ਪੂਰਨ lsb;  // The first bit in the variable
        ਪੂਰਨ msb;  // The last bit in the variable
        AstVarScope* vscp;  // The AstVarScope of the fragment
    };

    // The information about an AstVarScope of an eligible variable
    ਬਣਤਰ VscpInfo final {
        std::ਸਦਿਸ਼<RdRef> rdRefs;  // The reads of the AstVarScope
        std::ਸਦਿਸ਼<WrRef> wrRefs;  // The writes of the AstVarScope
        std::ਸਦਿਸ਼<Fragment> fragments;  // The fragments in LSB order, empty if not split
        ਤਾਰਕਿਕ blocked = ਝੂਠ;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_user2_ਵਿੱਚ_Use;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<ਪੂਰਨ, ਪੂਰਨ>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::ਸਦਿਸ਼<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_ਗੰਢ_ਸੌਂਪੋ* m_assignp = ਖਾਲੀ_ਸੂਚਕ;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_stat_ਵੰਡੋ_Attr;  // AstVarScopes split only due to split_var
    VDouble0 m_stat_ਵੰਡੋ_Auto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ਸਥਿਰ ਖਾਲੀ warn_No_ਵੰਡੋ(AstVar* varp, ਅਚਲ Ast_ਗੰਢ* wherep, ਅਚਲ ਅੱਖਰ* reasonp) {
        // Only warn if user requested splitting
        ਜੇ (!varp->attr_ਵੰਡੋ_Var()) ਵਾਪਸ;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::SPLITVAR, ਸੱਚ);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    ਸਥਿਰ ਤਾਰਕਿਕ isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        ਜੇ (!varp->user1()) {
            ਅਚਲ ਤਾਰਕਿਕ eligible = [&]() {
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                ਜੇ (!dtypep->isIntegralOrPacked()) ਵਾਪਸ ਝੂਠ;
                // Wider than one bit
                ਜੇ (dtypep->ਚੌੜਾਈ() <= 1) ਵਾਪਸ ਝੂਠ;
                // Check properties
                ਅਚਲ ਅੱਖਰ* ਅਚਲ reasonp = varp->cannot_ਵੰਡੋ_ਕਿਸਮ_Reason();
                // Warn that it cannot be split if user explicitly requested splitting
                ਜੇ (reasonp) warn_No_ਵੰਡੋ(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                ਵਾਪਸ !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        ਵਾਪਸ varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscp_Info_ਦਾ(AstVarScope* vscp) {
        ਜੇ (VscpInfo* ਅਚਲ infop = m_vscpInfos.tryGet(vscp)) ਵਾਪਸ *infop;
        // Also record on first encounter
        m_vscps.ਧੱਕੋ_ਪਿੱਠ(vscp);
        ਵਾਪਸ m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    ਸਥਿਰ ਖਾਲੀ ਖੰਡ(AstVar* varp, VscpInfo& info, ਅਚਲ Ast_ਗੰਢ* wherep) {
        warn_No_ਵੰਡੋ(varp, wherep, "it is referenced in an unsupported way");
        ਜੇ (info.blocked) ਵਾਪਸ;
        info.blocked = ਸੱਚ;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    ਖਾਲੀ ਰਿਕਾਰਡ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, AstVarRef* refp, ਪੂਰਨ lsb, ਪੂਰਨ msb) {
        AstVar* ਅਚਲ varp = refp->varp();
        ਜੇ (!isEligible(varp)) ਵਾਪਸ;
        VscpInfo& info = vscp_Info_ਦਾ(refp->varScopep());
        // RW reference blocks splitting
        ਜੇ (refp->access().isRW()) ਖੰਡ(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        ਜੇ (info.blocked) ਵਾਪਸ;
        // Record reference
        ਜੇ (refp->access().isReadOnly()) {
            info.rdRefs.ਧੱਕੋ_ਪਿੱਠ({exprp, lsb, msb});
            ਵਾਪਸ;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        ਅਚਲ ਤਾਰਕਿਕ canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.ਧੱਕੋ_ਪਿੱਠ({exprp, canExpand ? m_assignp : ਖਾਲੀ_ਸੂਚਕ, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    ਖਾਲੀ ਦੇਖੋ_Assignment(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? ਖਾਲੀ_ਸੂਚਕ : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, ਪੂਰਨ lsb, ਪੂਰਨ msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        ਜੇ (fragVarpr) ਵਾਪਸ fragVarpr;
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_ਗੰਢ::encode_ਸੰਖਿਆ(msb);
        ਜੇ (ਚੌੜਾਈ > 1) name += Ast_ਗੰਢ::encodeName(":") + Ast_ਗੰਢ::encode_ਸੰਖਿਆ(lsb);
        name += "__KET__";
        Ast_ਗੰਢ_DType* ਅਚਲ dtypep = varp->dtypep()->isFourstate()
                                         ? varp->ਲੱਭੋ_Logic_DType(ਚੌੜਾਈ, ਚੌੜਾਈ, VSigning::UNSIGNED)
                                         : varp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(ਚੌੜਾਈ, ਚੌੜਾਈ, VSigning::UNSIGNED);
        fragVarpr = ਨਵਾਂ AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagate_ਵੰਡੋ_Attr_From(varp);
        varp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(fragVarpr);
        ਵਾਪਸ fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    ਖਾਲੀ decide(AstVarScope* vscp, VscpInfo& info) {
        ਜੇ (info.blocked) ਵਾਪਸ;
        AstVar* ਅਚਲ varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        ਅਚਲ ਤਾਰਕਿਕ isAuto = [&]() {
            std::ਸਦਿਸ਼<std::pair<ਪੂਰਨ, ਪੂਰਨ>> ranges;  // [lsb, msb] inclusive
            ਲਈ (ਅਚਲ RdRef& ref : info.rdRefs) {
                ਜੇ (VN_IS(ref.exprp, VarRef)) ਵਾਪਸ ਝੂਠ;
                ranges.emplace_ਪਿੱਠ(ref.lsb, ref.msb);
            }
            ਲਈ (ਅਚਲ WrRef& ref : info.wrRefs) {
                ਜੇ (VN_IS(ref.exprp, VarRef)) ਵਾਪਸ ਝੂਠ;
                ranges.emplace_ਪਿੱਠ(ref.lsb, ref.msb);
            }
            std::ਛਾਂਟੋ(ranges.begin(), ranges.ਸਿਰਾ());
            ਲਈ (ਮਾਪ_t i = 0; i + 1 < ranges.ਮਾਪ(); ++i) {
                ਅਚਲ std::pair<ਪੂਰਨ, ਪੂਰਨ>& a = ranges[i];
                ਅਚਲ std::pair<ਪੂਰਨ, ਪੂਰਨ>& b = ranges[i + 1];
                ਜੇ (a != b && a.ਦੂਜਾ >= b.ਪਹਿਲਾ) ਵਾਪਸ ਝੂਠ;
            }
            ਵਾਪਸ ਸੱਚ;
        }();
        // Otherwise only if requested
        ਜੇ (!isAuto && !varp->attr_ਵੰਡੋ_Var()) ਵਾਪਸ;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        ਪੂਰਨ lsb = varp->ਚੌੜਾਈ();
        ਪੂਰਨ msb = -1;
        ਲਈ (ਅਚਲ RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::ਸਦਿਸ਼<ਪੂਰਨ> bounds;
        ਲਈ (ਅਚਲ WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.ਧੱਕੋ_ਪਿੱਠ(ref.lsb);
            bounds.ਧੱਕੋ_ਪਿੱਠ(ref.msb + 1);
        }
        bounds.ਧੱਕੋ_ਪਿੱਠ(lsb);
        bounds.ਧੱਕੋ_ਪਿੱਠ(msb + 1);
        std::ਛਾਂਟੋ(bounds.begin(), bounds.ਸਿਰਾ());
        bounds.erase(std::unique(bounds.begin(), bounds.ਸਿਰਾ()), bounds.ਸਿਰਾ());
        // Nothing to do if the only fragment is the whole variable
        ਜੇ (bounds.ਮਾਪ() == 2 && lsb == 0 && msb == varp->ਚੌੜਾਈ() - 1) ਵਾਪਸ;
        // Writes that cannot be expanded must write one fragment
        ਲਈ (ਅਚਲ WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            ਜੇ (ref.assignp) ਜਾਰੀ;
            // If write is to a whole fragment, it can be replaced
            ਜੇ (*std::ਉੱਤਲਾ_bound(bounds.begin(), bounds.ਸਿਰਾ(), ref.lsb) == ref.msb + 1) {
                ਜਾਰੀ;
            }
            // Otherwise can't split
            warn_No_ਵੰਡੋ(varp, ref.exprp, "it is written in an unsupported way");
            ਵਾਪਸ;
        }

        // Splitting this variable
        ++(isAuto ? m_stat_ਵੰਡੋ_Auto : m_stat_ਵੰਡੋ_Attr);

        // Create the fragment AstVarScopes
        ਲਈ (ਮਾਪ_t i = 0; i + 1 < bounds.ਮਾਪ(); ++i) {
            ਅਚਲ ਪੂਰਨ lsb = bounds[i];
            ਅਚਲ ਪੂਰਨ msb = bounds[i + 1] - 1;
            AstVar* ਅਚਲ fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* ਅਚਲ newp = ਨਵਾਂ AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newp);
            info.fragments.ਧੱਕੋ_ਪਿੱਠ({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    ਸਥਿਰ ਮਾਪ_t fragment_ਸੂਚਕਾਂਕ(ਅਚਲ VscpInfo& info, ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = std::ਉੱਤਲਾ_bound(info.fragments.begin(), info.fragments.ਸਿਰਾ(), ਦੋਆਧਾਰੀ_ਅੰਕ_2,
                                         [](ਪੂਰਨ b, ਅਚਲ Fragment& fragment) {  //
                                             ਵਾਪਸ b <= fragment.msb;
                                         });
        ਵਾਪਸ it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    ਖਾਲੀ rewriteRead(ਅਚਲ RdRef& ref, ਅਚਲ VscpInfo& info) {
        File_ਸਤਰ* ਅਚਲ flp = ref.exprp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* resultp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਮਾਪ_t i = fragment_ਸੂਚਕਾਂਕ(info, ref.lsb); i < info.fragments.ਮਾਪ(); ++i) {
            ਅਚਲ Fragment& fragment = info.fragments[i];
            ਜੇ (fragment.lsb > ref.msb) ਰੋਕੋ;
            ਅਚਲ ਪੂਰਨ partLsb = std::max(ref.lsb, fragment.lsb);
            ਅਚਲ ਪੂਰਨ partMsb = std::min(ref.msb, fragment.msb);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* bitsp = ਨਵਾਂ AstVarRef{flp, fragment.vscp, VAccess::READ};
            ਜੇ (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = ਨਵਾਂ AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? ਨਵਾਂ AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    ਸਥਿਰ ਤਾਰਕਿਕ isSelectable(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (VN_IS(nodep, Const)) ਵਾਪਸ ਸੱਚ;
        ਜੇ (ਅਚਲ AstSel* ਅਚਲ selp = VN_CAST(nodep, Sel)) {
            ਜੇ (!VN_IS(selp->lsbp(), Const)) ਵਾਪਸ ਝੂਠ;
            nodep = selp->fromp();
        }
        ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(nodep, VarRef);
        ਜੇ (!refp) ਵਾਪਸ ਝੂਠ;
        // Not a forced variable (trips several bugs in V3Force)
        ਜੇ (refp->varp()->isForced()) ਵਾਪਸ ਝੂਠ;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        ਵਾਪਸ !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    ਖਾਲੀ rewriteWrite(AstScope* scopep, ਅਚਲ WrRef& ref, ਅਚਲ VscpInfo& info) {
        File_ਸਤਰ* ਅਚਲ flp = ref.exprp->fileline();
        ਅਚਲ ਮਾਪ_t ਪਹਿਲਾ = fragment_ਸੂਚਕਾਂਕ(info, ref.lsb);
        ਅਚਲ ਮਾਪ_t ਆਖਰੀ = fragment_ਸੂਚਕਾਂਕ(info, ref.msb);
        UASSERT_OBJ(info.fragments[ਪਹਿਲਾ].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        ਜੇ (ਪਹਿਲਾ == ਆਖਰੀ) {
            ref.exprp->replaceWith(ਨਵਾਂ AstVarRef{flp, info.fragments[ਪਹਿਲਾ].vscp, VAccess::WRITE});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(ref.exprp), ref.exprp);
            ਵਾਪਸ;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ origp = ref.assignp;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp = origp->rhsp()->unlink_Fr_ਪਿੱਠ();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        ਜੇ (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* ਅਚਲ tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* ਅਚਲ tmpRefp = ਨਵਾਂ AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, tmpRefp, rhsp});
            rhsp = ਨਵਾਂ AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        ਲਈ (ਮਾਪ_t i = ਪਹਿਲਾ; i <= ਆਖਰੀ; ++i) {
            ਅਚਲ Fragment& fragment = info.fragments[i];
            ਅਚਲ ਪੂਰਨ lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = fragment.msb - fragment.lsb + 1;
            AstVarRef* ਅਚਲ lhsp = ਨਵਾਂ AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* ਅਚਲ selp = ਨਵਾਂ AstSel{flp, rhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), lsb, ਚੌੜਾਈ};
            origp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(origp->unlink_Fr_ਪਿੱਠ()), origp);
    }

    // The combinational AstActive of the scope, cached
    ਸਥਿਰ Ast_ਸਰਗਰਮ* combo_ਸਰਗਰਮ(AstScope* scopep) {
        ਜੇ (!scopep->user2p()) scopep->user2p(scopep->comboActivep(ਸੱਚ));
        ਵਾਪਸ VN_AS(scopep->user2p(), ਸਰਗਰਮ);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_DType*) override {}  // No references in data types
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ* nodep) override { ਦੇਖੋ_Assignment(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_W* nodep) override { ਦੇਖੋ_Assignment(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Dly* nodep) override { ਦੇਖੋ_Assignment(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        AstVarRef* ਅਚਲ refp = VN_CAST(nodep->fromp(), VarRef);
        ਜੇ (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            ਵਾਪਸ;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        ਅਚਲ AstConst* ਅਚਲ lsbp = VN_CAST(nodep->lsbp(), Const);
        ਜੇ (!lsbp || lsbp->ਮੰਜ਼ਿਲ_SInt() + nodep->ਚੌੜਾਈ_Const() > refp->varp()->ਚੌੜਾਈ()) {
            ਖੰਡ(refp->varp(), vscp_Info_ਦਾ(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            ਵਾਪਸ;
        }
        // Record partial access
        ਰਿਕਾਰਡ(nodep, refp, lsbp->ਮੰਜ਼ਿਲ_SInt(), lsbp->ਮੰਜ਼ਿਲ_SInt() + nodep->ਚੌੜਾਈ_Const() - 1);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        // Record whole access
        ਰਿਕਾਰਡ(nodep, nodep, 0, nodep->varp()->ਚੌੜਾਈ() - 1);
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        // The AstVarScope is not known, so all of them are blocked, by making it ineligible
        AstVar* ਅਚਲ varp = nodep->varp();
        ਜੇ (!isEligible(varp)) ਵਾਪਸ;
        varp->user1(2);  // Mark not eligible
        warn_No_ਵੰਡੋ(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਦੋਆਧਾਰੀ_ਅੰਕ_Blast_Visitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        ਲਈ (AstVarScope* ਅਚਲ vscp : m_vscps) {
            ਜੇ (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        ਲਈ (AstVarScope* ਅਚਲ vscp : m_vscps) {
            ਅਚਲ VscpInfo& info = m_vscpInfos(vscp);
            ਜੇ (info.fragments.ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
            ਲਈ (ਅਚਲ RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        ਲਈ (AstVarScope* ਅਚਲ vscp : m_vscps) {
            ਅਚਲ VscpInfo& info = m_vscpInfos(vscp);
            ਜੇ (info.fragments.ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
            ਲਈ (ਅਚਲ WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        ਲਈ (AstVarScope* ਅਚਲ vscp : m_vscps) {
            ਅਚਲ VscpInfo& info = m_vscpInfos(vscp);
            ਜੇ (info.fragments.ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
            ਜੇ (!(vscp->varp()->isTrace() && vscp->isTrace())) ਜਾਰੀ;
            Ast_ਸਰਗਰਮ* ਅਚਲ activep = combo_ਸਰਗਰਮ(vscp->scopep());
            File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
            ਲਈ (ਅਚਲ Fragment& fragment : info.fragments) {
                AstVarRef* ਅਚਲ lhsRefp = ਨਵਾਂ AstVarRef{flp, vscp, VAccess::WRITE};
                ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = fragment.msb - fragment.lsb + 1;
                AstSel* ਅਚਲ lhsp = ਨਵਾਂ AstSel{flp, lhsRefp, fragment.lsb, ਚੌੜਾਈ};
                AstVarRef* ਅਚਲ rhsp = ਨਵਾਂ AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{ਨਵਾਂ Ast_ਸੌਂਪੋ_W{flp, lhsp, rhsp}});
            }
        }

        V3Stats::ਜੋੜੋ_Stat("Optimizations, Bitblast, variables split due to attribute",
                         m_stat_ਵੰਡੋ_Attr);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Bitblast, variables split automatically",
                         m_stat_ਵੰਡੋ_Auto);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Bitblast, fragments created", m_statFragments);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Bitblast, assignments through temporary", m_statTmps);
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(AstNetlist* netlistp) { ਦੋਆਧਾਰੀ_ਅੰਕ_Blast_Visitor{netlistp}; }
};

//######################################################################
// V3BitBlast class functions

ਖਾਲੀ V3_ਦੋਆਧਾਰੀ_ਅੰਕ_Blast::ਦੋਆਧਾਰੀ_ਅੰਕ_Blast_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    ਦੋਆਧਾਰੀ_ਅੰਕ_Blast_Visitor::apply(nodep);
    V3Global::dump_Check_Global_ਰੁੱਖ("bitblast", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
