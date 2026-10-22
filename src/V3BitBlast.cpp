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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3BitBlast.h"

#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3SharedTmps.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <algorithm>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

తరగతి బిట్టు_Blast_Visitor final : బహిరంగ VNVisitorConst, బహిరంగ VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    నిర్మాణం RdRef final {
        Ast_కణుపు_సమాసం* exprp;  // Sel, or the VarRef if whole
        పూర్ణాంకం lsb;  // First bit read
        పూర్ణాంకం msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    నిర్మాణం WrRef final {
        Ast_కణుపు_సమాసం* exprp;  // The Sel, or the VarRef if whole
        Ast_కణుపు_కేటాయించు* assignp;  // The assignment, if it can be expanded
        పూర్ణాంకం lsb;  // The first bit written
        పూర్ణాంకం msb;  // The last bit written
    };

    // A fragment of a split AstVarScope
    నిర్మాణం Fragment final {
        పూర్ణాంకం lsb;  // The first bit in the variable
        పూర్ణాంకం msb;  // The last bit in the variable
        AstVarScope* vscp;  // The AstVarScope of the fragment
    };

    // The information about an AstVarScope of an eligible variable
    నిర్మాణం VscpInfo final {
        std::సదిశ<RdRef> rdRefs;  // The reads of the AstVarScope
        std::సదిశ<WrRef> wrRefs;  // The writes of the AstVarScope
        std::సదిశ<Fragment> fragments;  // The fragments in LSB order, empty if not split
        తార్కిక blocked = అబద్ధం;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;
    స్థిరరాశి VNUser2_లోపల_Use m_user2_లోపల_Use;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<పూర్ణాంకం, పూర్ణాంకం>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::సదిశ<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_కణుపు_కేటాయించు* m_assignp = శూన్యసూచిక;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_stat_విభజించు_Attr;  // AstVarScopes split only due to split_var
    VDouble0 m_stat_విభజించు_Auto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    స్థిర శూన్య warn_No_విభజించు(AstVar* varp, స్థిరరాశి Ast_కణుపు* wherep, స్థిరరాశి అక్షరం* reasonp) {
        // Only warn if user requested splitting
        ఒకవేళ (!varp->attr_విభజించు_Var()) తిరిగి;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3_Error_సంకేతం::SPLITVAR, నిజం);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    స్థిర తార్కిక isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        ఒకవేళ (!varp->user1()) {
            స్థిరరాశి తార్కిక eligible = [&]() {
                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                ఒకవేళ (!dtypep->isIntegralOrPacked()) తిరిగి అబద్ధం;
                // Wider than one bit
                ఒకవేళ (dtypep->వెడల్పు() <= 1) తిరిగి అబద్ధం;
                // Check properties
                స్థిరరాశి అక్షరం* స్థిరరాశి reasonp = varp->cannot_విభజించు_రకం_Reason();
                // Warn that it cannot be split if user explicitly requested splitting
                ఒకవేళ (reasonp) warn_No_విభజించు(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                తిరిగి !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        తిరిగి varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscp_Info_యొక్క(AstVarScope* vscp) {
        ఒకవేళ (VscpInfo* స్థిరరాశి infop = m_vscpInfos.tryGet(vscp)) తిరిగి *infop;
        // Also record on first encounter
        m_vscps.నెట్టు_వెనుక(vscp);
        తిరిగి m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    స్థిర శూన్య ఖండం(AstVar* varp, VscpInfo& info, స్థిరరాశి Ast_కణుపు* wherep) {
        warn_No_విభజించు(varp, wherep, "it is referenced in an unsupported way");
        ఒకవేళ (info.blocked) తిరిగి;
        info.blocked = నిజం;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    శూన్య రికార్డు(Ast_కణుపు_సమాసం* exprp, AstVarRef* refp, పూర్ణాంకం lsb, పూర్ణాంకం msb) {
        AstVar* స్థిరరాశి varp = refp->varp();
        ఒకవేళ (!isEligible(varp)) తిరిగి;
        VscpInfo& info = vscp_Info_యొక్క(refp->varScopep());
        // RW reference blocks splitting
        ఒకవేళ (refp->access().isRW()) ఖండం(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        ఒకవేళ (info.blocked) తిరిగి;
        // Record reference
        ఒకవేళ (refp->access().isReadOnly()) {
            info.rdRefs.నెట్టు_వెనుక({exprp, lsb, msb});
            తిరిగి;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        స్థిరరాశి తార్కిక canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.నెట్టు_వెనుక({exprp, canExpand ? m_assignp : శూన్యసూచిక, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    శూన్య సందర్శించు_Assignment(Ast_కణుపు_కేటాయించు* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? శూన్యసూచిక : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, పూర్ణాంకం lsb, పూర్ణాంకం msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        ఒకవేళ (fragVarpr) తిరిగి fragVarpr;
        స్థిరరాశి పూర్ణాంకం వెడల్పు = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_కణుపు::encode_సంఖ్య(msb);
        ఒకవేళ (వెడల్పు > 1) name += Ast_కణుపు::encodeName(":") + Ast_కణుపు::encode_సంఖ్య(lsb);
        name += "__KET__";
        Ast_కణుపు_DType* స్థిరరాశి dtypep = varp->dtypep()->isFourstate()
                                         ? varp->కనుగొను_Logic_DType(వెడల్పు, వెడల్పు, VSigning::UNSIGNED)
                                         : varp->కనుగొను_బిట్టు_DType(వెడల్పు, వెడల్పు, VSigning::UNSIGNED);
        fragVarpr = కొత్త AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagate_విభజించు_Attr_From(varp);
        varp->కూడు_ఇక్కడ_This_As_తదుపరి(fragVarpr);
        తిరిగి fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    శూన్య decide(AstVarScope* vscp, VscpInfo& info) {
        ఒకవేళ (info.blocked) తిరిగి;
        AstVar* స్థిరరాశి varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        స్థిరరాశి తార్కిక isAuto = [&]() {
            std::సదిశ<std::pair<పూర్ణాంకం, పూర్ణాంకం>> ranges;  // [lsb, msb] inclusive
            కొరకు (స్థిరరాశి RdRef& ref : info.rdRefs) {
                ఒకవేళ (VN_IS(ref.exprp, VarRef)) తిరిగి అబద్ధం;
                ranges.emplace_వెనుక(ref.lsb, ref.msb);
            }
            కొరకు (స్థిరరాశి WrRef& ref : info.wrRefs) {
                ఒకవేళ (VN_IS(ref.exprp, VarRef)) తిరిగి అబద్ధం;
                ranges.emplace_వెనుక(ref.lsb, ref.msb);
            }
            std::క్రమీకరించు(ranges.begin(), ranges.చివర());
            కొరకు (కొలత_t i = 0; i + 1 < ranges.కొలత(); ++i) {
                స్థిరరాశి std::pair<పూర్ణాంకం, పూర్ణాంకం>& a = ranges[i];
                స్థిరరాశి std::pair<పూర్ణాంకం, పూర్ణాంకం>& b = ranges[i + 1];
                ఒకవేళ (a != b && a.రెండవ >= b.మొదటి) తిరిగి అబద్ధం;
            }
            తిరిగి నిజం;
        }();
        // Otherwise only if requested
        ఒకవేళ (!isAuto && !varp->attr_విభజించు_Var()) తిరిగి;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        పూర్ణాంకం lsb = varp->వెడల్పు();
        పూర్ణాంకం msb = -1;
        కొరకు (స్థిరరాశి RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::సదిశ<పూర్ణాంకం> bounds;
        కొరకు (స్థిరరాశి WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.నెట్టు_వెనుక(ref.lsb);
            bounds.నెట్టు_వెనుక(ref.msb + 1);
        }
        bounds.నెట్టు_వెనుక(lsb);
        bounds.నెట్టు_వెనుక(msb + 1);
        std::క్రమీకరించు(bounds.begin(), bounds.చివర());
        bounds.erase(std::unique(bounds.begin(), bounds.చివర()), bounds.చివర());
        // Nothing to do if the only fragment is the whole variable
        ఒకవేళ (bounds.కొలత() == 2 && lsb == 0 && msb == varp->వెడల్పు() - 1) తిరిగి;
        // Writes that cannot be expanded must write one fragment
        కొరకు (స్థిరరాశి WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            ఒకవేళ (ref.assignp) కొనసాగు;
            // If write is to a whole fragment, it can be replaced
            ఒకవేళ (*std::పై_bound(bounds.begin(), bounds.చివర(), ref.lsb) == ref.msb + 1) {
                కొనసాగు;
            }
            // Otherwise can't split
            warn_No_విభజించు(varp, ref.exprp, "it is written in an unsupported way");
            తిరిగి;
        }

        // Splitting this variable
        ++(isAuto ? m_stat_విభజించు_Auto : m_stat_విభజించు_Attr);

        // Create the fragment AstVarScopes
        కొరకు (కొలత_t i = 0; i + 1 < bounds.కొలత(); ++i) {
            స్థిరరాశి పూర్ణాంకం lsb = bounds[i];
            స్థిరరాశి పూర్ణాంకం msb = bounds[i + 1] - 1;
            AstVar* స్థిరరాశి fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* స్థిరరాశి newp = కొత్త AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->కూడు_ఇక్కడ_This_As_తదుపరి(newp);
            info.fragments.నెట్టు_వెనుక({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    స్థిర కొలత_t fragment_సూచీ(స్థిరరాశి VscpInfo& info, పూర్ణాంకం బిట్టు_2) {
        స్థిరరాశి స్వయంచాలక it = std::పై_bound(info.fragments.begin(), info.fragments.చివర(), బిట్టు_2,
                                         [](పూర్ణాంకం b, స్థిరరాశి Fragment& fragment) {  //
                                             తిరిగి b <= fragment.msb;
                                         });
        తిరిగి it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    శూన్య rewriteRead(స్థిరరాశి RdRef& ref, స్థిరరాశి VscpInfo& info) {
        File_పంక్తి* స్థిరరాశి flp = ref.exprp->fileline();
        Ast_కణుపు_సమాసం* resultp = శూన్యసూచిక;
        కొరకు (కొలత_t i = fragment_సూచీ(info, ref.lsb); i < info.fragments.కొలత(); ++i) {
            స్థిరరాశి Fragment& fragment = info.fragments[i];
            ఒకవేళ (fragment.lsb > ref.msb) ఆపు;
            స్థిరరాశి పూర్ణాంకం partLsb = std::max(ref.lsb, fragment.lsb);
            స్థిరరాశి పూర్ణాంకం partMsb = std::min(ref.msb, fragment.msb);
            Ast_కణుపు_సమాసం* bitsp = కొత్త AstVarRef{flp, fragment.vscp, VAccess::READ};
            ఒకవేళ (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = కొత్త AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? కొత్త AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(నెట్టు_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    స్థిర తార్కిక isSelectable(స్థిరరాశి Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (VN_IS(nodep, Const)) తిరిగి నిజం;
        ఒకవేళ (స్థిరరాశి AstSel* స్థిరరాశి selp = VN_CAST(nodep, Sel)) {
            ఒకవేళ (!VN_IS(selp->lsbp(), Const)) తిరిగి అబద్ధం;
            nodep = selp->fromp();
        }
        స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(nodep, VarRef);
        ఒకవేళ (!refp) తిరిగి అబద్ధం;
        // Not a forced variable (trips several bugs in V3Force)
        ఒకవేళ (refp->varp()->isForced()) తిరిగి అబద్ధం;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        తిరిగి !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    శూన్య rewriteWrite(AstScope* scopep, స్థిరరాశి WrRef& ref, స్థిరరాశి VscpInfo& info) {
        File_పంక్తి* స్థిరరాశి flp = ref.exprp->fileline();
        స్థిరరాశి కొలత_t మొదటి = fragment_సూచీ(info, ref.lsb);
        స్థిరరాశి కొలత_t చివరి = fragment_సూచీ(info, ref.msb);
        UASSERT_OBJ(info.fragments[మొదటి].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        ఒకవేళ (మొదటి == చివరి) {
            ref.exprp->replaceWith(కొత్త AstVarRef{flp, info.fragments[మొదటి].vscp, VAccess::WRITE});
            VL_DO_DANGLING(నెట్టు_Deletep(ref.exprp), ref.exprp);
            తిరిగి;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_కణుపు_కేటాయించు* స్థిరరాశి origp = ref.assignp;
        Ast_కణుపు_సమాసం* rhsp = origp->rhsp()->unlink_Fr_వెనుక();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        ఒకవేళ (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* స్థిరరాశి tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* స్థిరరాశి tmpRefp = కొత్త AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->కూడు_ఇక్కడ_This_As_తదుపరి(కొత్త Ast_కేటాయించు{flp, tmpRefp, rhsp});
            rhsp = కొత్త AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        కొరకు (కొలత_t i = మొదటి; i <= చివరి; ++i) {
            స్థిరరాశి Fragment& fragment = info.fragments[i];
            స్థిరరాశి పూర్ణాంకం lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            స్థిరరాశి పూర్ణాంకం వెడల్పు = fragment.msb - fragment.lsb + 1;
            AstVarRef* స్థిరరాశి lhsp = కొత్త AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* స్థిరరాశి selp = కొత్త AstSel{flp, rhsp->clone_వృక్షం_శుద్ధ(అబద్ధం), lsb, వెడల్పు};
            origp->కూడు_ఇక్కడ_This_As_తదుపరి(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(నెట్టు_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(నెట్టు_Deletep(origp->unlink_Fr_వెనుక()), origp);
    }

    // The combinational AstActive of the scope, cached
    స్థిర Ast_చురుకైన* combo_చురుకైన(AstScope* scopep) {
        ఒకవేళ (!scopep->user2p()) scopep->user2p(scopep->comboActivep(నిజం));
        తిరిగి VN_AS(scopep->user2p(), చురుకైన);
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_DType*) override {}  // No references in data types
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }
    శూన్య సందర్శించు(Ast_కేటాయించు* nodep) override { సందర్శించు_Assignment(nodep); }
    శూన్య సందర్శించు(Ast_కేటాయించు_W* nodep) override { సందర్శించు_Assignment(nodep); }
    శూన్య సందర్శించు(Ast_కేటాయించు_Dly* nodep) override { సందర్శించు_Assignment(nodep); }
    శూన్య సందర్శించు(AstSel* nodep) override {
        AstVarRef* స్థిరరాశి refp = VN_CAST(nodep->fromp(), VarRef);
        ఒకవేళ (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            తిరిగి;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        స్థిరరాశి AstConst* స్థిరరాశి lsbp = VN_CAST(nodep->lsbp(), Const);
        ఒకవేళ (!lsbp || lsbp->గమ్యం_SInt() + nodep->వెడల్పు_Const() > refp->varp()->వెడల్పు()) {
            ఖండం(refp->varp(), vscp_Info_యొక్క(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            తిరిగి;
        }
        // Record partial access
        రికార్డు(nodep, refp, lsbp->గమ్యం_SInt(), lsbp->గమ్యం_SInt() + nodep->వెడల్పు_Const() - 1);
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        // Record whole access
        రికార్డు(nodep, nodep, 0, nodep->varp()->వెడల్పు() - 1);
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        // The AstVarScope is not known, so all of them are blocked, by making it ineligible
        AstVar* స్థిరరాశి varp = nodep->varp();
        ఒకవేళ (!isEligible(varp)) తిరిగి;
        varp->user1(2);  // Mark not eligible
        warn_No_విభజించు(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    స్పష్టమైన బిట్టు_Blast_Visitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        కొరకు (AstVarScope* స్థిరరాశి vscp : m_vscps) {
            ఒకవేళ (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        కొరకు (AstVarScope* స్థిరరాశి vscp : m_vscps) {
            స్థిరరాశి VscpInfo& info = m_vscpInfos(vscp);
            ఒకవేళ (info.fragments.ఖాళీలు()) కొనసాగు;
            కొరకు (స్థిరరాశి RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        కొరకు (AstVarScope* స్థిరరాశి vscp : m_vscps) {
            స్థిరరాశి VscpInfo& info = m_vscpInfos(vscp);
            ఒకవేళ (info.fragments.ఖాళీలు()) కొనసాగు;
            కొరకు (స్థిరరాశి WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        కొరకు (AstVarScope* స్థిరరాశి vscp : m_vscps) {
            స్థిరరాశి VscpInfo& info = m_vscpInfos(vscp);
            ఒకవేళ (info.fragments.ఖాళీలు()) కొనసాగు;
            ఒకవేళ (!(vscp->varp()->isTrace() && vscp->isTrace())) కొనసాగు;
            Ast_చురుకైన* స్థిరరాశి activep = combo_చురుకైన(vscp->scopep());
            File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
            కొరకు (స్థిరరాశి Fragment& fragment : info.fragments) {
                AstVarRef* స్థిరరాశి lhsRefp = కొత్త AstVarRef{flp, vscp, VAccess::WRITE};
                స్థిరరాశి పూర్ణాంకం వెడల్పు = fragment.msb - fragment.lsb + 1;
                AstSel* స్థిరరాశి lhsp = కొత్త AstSel{flp, lhsRefp, fragment.lsb, వెడల్పు};
                AstVarRef* స్థిరరాశి rhsp = కొత్త AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->కూడు_Stmtsp(కొత్త AstAlways{కొత్త Ast_కేటాయించు_W{flp, lhsp, rhsp}});
            }
        }

        V3Stats::కూడు_Stat("Optimizations, Bitblast, variables split due to attribute",
                         m_stat_విభజించు_Attr);
        V3Stats::కూడు_Stat("Optimizations, Bitblast, variables split automatically",
                         m_stat_విభజించు_Auto);
        V3Stats::కూడు_Stat("Optimizations, Bitblast, fragments created", m_statFragments);
        V3Stats::కూడు_Stat("Optimizations, Bitblast, assignments through temporary", m_statTmps);
    }

బహిరంగ:
    స్థిర శూన్య apply(AstNetlist* netlistp) { బిట్టు_Blast_Visitor{netlistp}; }
};

//######################################################################
// V3BitBlast class functions

శూన్య V3_బిట్టు_Blast::బిట్టు_Blast_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    బిట్టు_Blast_Visitor::apply(nodep);
    V3Global::dump_Check_Global_వృక్షం("bitblast", 0, dump_వృక్షం_Either_Level() >= 3);
}
