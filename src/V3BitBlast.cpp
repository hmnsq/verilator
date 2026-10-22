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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3BitBlast.h"

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3SharedTmps.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <algorithm>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

വർഗ്ഗം BitBlastVisitor final : പൊതു VNVisitorConst, പൊതു VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    ഘടന RdRef final {
        Ast_കണ്ണി_വ്യഞ്ജകം* exprp;  // Sel, or the VarRef if whole
        പൂർണ്ണസംഖ്യ lsb;  // First bit read
        പൂർണ്ണസംഖ്യ msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    ഘടന WrRef final {
        Ast_കണ്ണി_വ്യഞ്ജകം* exprp;  // The Sel, or the VarRef if whole
        Ast_കണ്ണി_Assign* assignp;  // The assignment, if it can be expanded
        പൂർണ്ണസംഖ്യ lsb;  // The first bit written
        പൂർണ്ണസംഖ്യ msb;  // The last bit written
    };

    // A fragment of a split AstVarScope
    ഘടന Fragment final {
        പൂർണ്ണസംഖ്യ lsb;  // The first bit in the variable
        പൂർണ്ണസംഖ്യ msb;  // The last bit in the variable
        AstVarScope* vscp;  // The AstVarScope of the fragment
    };

    // The information about an AstVarScope of an eligible variable
    ഘടന VscpInfo final {
        std::vector<RdRef> rdRefs;  // The reads of the AstVarScope
        std::vector<WrRef> wrRefs;  // The writes of the AstVarScope
        std::vector<Fragment> fragments;  // The fragments in LSB order, empty if not split
        തർക്കം blocked = അസത്യം;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    സ്ഥിരം VNUser1InUse m_user1InUse;
    സ്ഥിരം VNUser2InUse m_user2InUse;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::vector<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_കണ്ണി_Assign* m_assignp = nullptr;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_statSplitAttr;  // AstVarScopes split only due to split_var
    VDouble0 m_statSplitAuto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    സ്ഥിതം ശൂന്യം warnNoSplit(AstVar* varp, സ്ഥിരം Ast_കണ്ണി* wherep, സ്ഥിരം അക്ഷരം* reasonp) {
        // Only warn if user requested splitting
        എങ്കിൽ (!varp->attrSplitVar()) തിരികെ;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, സത്യം);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    സ്ഥിതം തർക്കം isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        എങ്കിൽ (!varp->user1()) {
            സ്ഥിരം തർക്കം eligible = [&]() {
                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                എങ്കിൽ (!dtypep->isIntegralOrPacked()) തിരികെ അസത്യം;
                // Wider than one bit
                എങ്കിൽ (dtypep->width() <= 1) തിരികെ അസത്യം;
                // Check properties
                സ്ഥിരം അക്ഷരം* സ്ഥിരം reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                എങ്കിൽ (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                തിരികെ !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        തിരികെ varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscpInfoOf(AstVarScope* vscp) {
        എങ്കിൽ (VscpInfo* സ്ഥിരം infop = m_vscpInfos.tryGet(vscp)) തിരികെ *infop;
        // Also record on first encounter
        m_vscps.തള്ളുക_back(vscp);
        തിരികെ m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    സ്ഥിതം ശൂന്യം block(AstVar* varp, VscpInfo& info, സ്ഥിരം Ast_കണ്ണി* wherep) {
        warnNoSplit(varp, wherep, "it is referenced in an unsupported way");
        എങ്കിൽ (info.blocked) തിരികെ;
        info.blocked = സത്യം;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    ശൂന്യം record(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, AstVarRef* refp, പൂർണ്ണസംഖ്യ lsb, പൂർണ്ണസംഖ്യ msb) {
        AstVar* സ്ഥിരം varp = refp->varp();
        എങ്കിൽ (!isEligible(varp)) തിരികെ;
        VscpInfo& info = vscpInfoOf(refp->varScopep());
        // RW reference blocks splitting
        എങ്കിൽ (refp->access().isRW()) block(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        എങ്കിൽ (info.blocked) തിരികെ;
        // Record reference
        എങ്കിൽ (refp->access().isReadOnly()) {
            info.rdRefs.തള്ളുക_back({exprp, lsb, msb});
            തിരികെ;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        സ്ഥിരം തർക്കം canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.തള്ളുക_back({exprp, canExpand ? m_assignp : nullptr, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    ശൂന്യം visitAssignment(Ast_കണ്ണി_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? nullptr : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, പൂർണ്ണസംഖ്യ lsb, പൂർണ്ണസംഖ്യ msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        എങ്കിൽ (fragVarpr) തിരികെ fragVarpr;
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_കണ്ണി::encode_സംഖ്യ(msb);
        എങ്കിൽ (width > 1) name += Ast_കണ്ണി::encodeName(":") + Ast_കണ്ണി::encode_സംഖ്യ(lsb);
        name += "__KET__";
        Ast_കണ്ണി_DType* സ്ഥിരം dtypep = varp->dtypep()->isFourstate()
                                         ? varp->findLogicDType(width, width, VSigning::UNSIGNED)
                                         : varp->findBitDType(width, width, VSigning::UNSIGNED);
        fragVarpr = പുതിയ AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagateSplitAttrFrom(varp);
        varp->addHereThisAsNext(fragVarpr);
        തിരികെ fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    ശൂന്യം decide(AstVarScope* vscp, VscpInfo& info) {
        എങ്കിൽ (info.blocked) തിരികെ;
        AstVar* സ്ഥിരം varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        സ്ഥിരം തർക്കം isAuto = [&]() {
            std::vector<std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>> ranges;  // [lsb, msb] inclusive
            വേണ്ടി (സ്ഥിരം RdRef& ref : info.rdRefs) {
                എങ്കിൽ (VN_IS(ref.exprp, VarRef)) തിരികെ അസത്യം;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            വേണ്ടി (സ്ഥിരം WrRef& ref : info.wrRefs) {
                എങ്കിൽ (VN_IS(ref.exprp, VarRef)) തിരികെ അസത്യം;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            std::sort(ranges.begin(), ranges.end());
            വേണ്ടി (size_t i = 0; i + 1 < ranges.size(); ++i) {
                സ്ഥിരം std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>& a = ranges[i];
                സ്ഥിരം std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>& b = ranges[i + 1];
                എങ്കിൽ (a != b && a.second >= b.first) തിരികെ അസത്യം;
            }
            തിരികെ സത്യം;
        }();
        // Otherwise only if requested
        എങ്കിൽ (!isAuto && !varp->attrSplitVar()) തിരികെ;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        പൂർണ്ണസംഖ്യ lsb = varp->width();
        പൂർണ്ണസംഖ്യ msb = -1;
        വേണ്ടി (സ്ഥിരം RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::vector<പൂർണ്ണസംഖ്യ> bounds;
        വേണ്ടി (സ്ഥിരം WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.തള്ളുക_back(ref.lsb);
            bounds.തള്ളുക_back(ref.msb + 1);
        }
        bounds.തള്ളുക_back(lsb);
        bounds.തള്ളുക_back(msb + 1);
        std::sort(bounds.begin(), bounds.end());
        bounds.erase(std::unique(bounds.begin(), bounds.end()), bounds.end());
        // Nothing to do if the only fragment is the whole variable
        എങ്കിൽ (bounds.size() == 2 && lsb == 0 && msb == varp->width() - 1) തിരികെ;
        // Writes that cannot be expanded must write one fragment
        വേണ്ടി (സ്ഥിരം WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            എങ്കിൽ (ref.assignp) തുടരുക;
            // If write is to a whole fragment, it can be replaced
            എങ്കിൽ (*std::upper_bound(bounds.begin(), bounds.end(), ref.lsb) == ref.msb + 1) {
                തുടരുക;
            }
            // Otherwise can't split
            warnNoSplit(varp, ref.exprp, "it is written in an unsupported way");
            തിരികെ;
        }

        // Splitting this variable
        ++(isAuto ? m_statSplitAuto : m_statSplitAttr);

        // Create the fragment AstVarScopes
        വേണ്ടി (size_t i = 0; i + 1 < bounds.size(); ++i) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = bounds[i];
            സ്ഥിരം പൂർണ്ണസംഖ്യ msb = bounds[i + 1] - 1;
            AstVar* സ്ഥിരം fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* സ്ഥിരം newp = പുതിയ AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->addHereThisAsNext(newp);
            info.fragments.തള്ളുക_back({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    സ്ഥിതം size_t fragmentIndex(സ്ഥിരം VscpInfo& info, പൂർണ്ണസംഖ്യ bit) {
        സ്ഥിരം auto it = std::upper_bound(info.fragments.begin(), info.fragments.end(), bit,
                                         [](പൂർണ്ണസംഖ്യ b, സ്ഥിരം Fragment& fragment) {  //
                                             തിരികെ b <= fragment.msb;
                                         });
        തിരികെ it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    ശൂന്യം rewriteRead(സ്ഥിരം RdRef& ref, സ്ഥിരം VscpInfo& info) {
        File_വരി* സ്ഥിരം flp = ref.exprp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* resultp = nullptr;
        വേണ്ടി (size_t i = fragmentIndex(info, ref.lsb); i < info.fragments.size(); ++i) {
            സ്ഥിരം Fragment& fragment = info.fragments[i];
            എങ്കിൽ (fragment.lsb > ref.msb) നിർത്തുക;
            സ്ഥിരം പൂർണ്ണസംഖ്യ partLsb = std::max(ref.lsb, fragment.lsb);
            സ്ഥിരം പൂർണ്ണസംഖ്യ partMsb = std::min(ref.msb, fragment.msb);
            Ast_കണ്ണി_വ്യഞ്ജകം* bitsp = പുതിയ AstVarRef{flp, fragment.vscp, VAccess::READ};
            എങ്കിൽ (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = പുതിയ AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? പുതിയ AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(തള്ളുക_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    സ്ഥിതം തർക്കം isSelectable(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (VN_IS(nodep, Const)) തിരികെ സത്യം;
        എങ്കിൽ (സ്ഥിരം AstSel* സ്ഥിരം selp = VN_CAST(nodep, Sel)) {
            എങ്കിൽ (!VN_IS(selp->lsbp(), Const)) തിരികെ അസത്യം;
            nodep = selp->fromp();
        }
        സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(nodep, VarRef);
        എങ്കിൽ (!refp) തിരികെ അസത്യം;
        // Not a forced variable (trips several bugs in V3Force)
        എങ്കിൽ (refp->varp()->isForced()) തിരികെ അസത്യം;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        തിരികെ !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    ശൂന്യം rewriteWrite(AstScope* scopep, സ്ഥിരം WrRef& ref, സ്ഥിരം VscpInfo& info) {
        File_വരി* സ്ഥിരം flp = ref.exprp->fileline();
        സ്ഥിരം size_t first = fragmentIndex(info, ref.lsb);
        സ്ഥിരം size_t last = fragmentIndex(info, ref.msb);
        UASSERT_OBJ(info.fragments[first].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        എങ്കിൽ (first == last) {
            ref.exprp->replaceWith(പുതിയ AstVarRef{flp, info.fragments[first].vscp, VAccess::WRITE});
            VL_DO_DANGLING(തള്ളുക_Deletep(ref.exprp), ref.exprp);
            തിരികെ;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_കണ്ണി_Assign* സ്ഥിരം origp = ref.assignp;
        Ast_കണ്ണി_വ്യഞ്ജകം* rhsp = origp->rhsp()->unlinkFrBack();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        എങ്കിൽ (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* സ്ഥിരം tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* സ്ഥിരം tmpRefp = പുതിയ AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->addHereThisAsNext(പുതിയ AstAssign{flp, tmpRefp, rhsp});
            rhsp = പുതിയ AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        വേണ്ടി (size_t i = first; i <= last; ++i) {
            സ്ഥിരം Fragment& fragment = info.fragments[i];
            സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            സ്ഥിരം പൂർണ്ണസംഖ്യ width = fragment.msb - fragment.lsb + 1;
            AstVarRef* സ്ഥിരം lhsp = പുതിയ AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* സ്ഥിരം selp = പുതിയ AstSel{flp, rhsp->clone_വൃക്ഷം_Pure(അസത്യം), lsb, width};
            origp->addHereThisAsNext(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(തള്ളുക_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    സ്ഥിതം AstActive* comboActive(AstScope* scopep) {
        എങ്കിൽ (!scopep->user2p()) scopep->user2p(scopep->comboActivep(സത്യം));
        തിരികെ VN_AS(scopep->user2p(), Active);
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_DType*) override {}  // No references in data types
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }
    ശൂന്യം visit(AstAssign* nodep) override { visitAssignment(nodep); }
    ശൂന്യം visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    ശൂന്യം visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    ശൂന്യം visit(AstSel* nodep) override {
        AstVarRef* സ്ഥിരം refp = VN_CAST(nodep->fromp(), VarRef);
        എങ്കിൽ (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            തിരികെ;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        സ്ഥിരം AstConst* സ്ഥിരം lsbp = VN_CAST(nodep->lsbp(), Const);
        എങ്കിൽ (!lsbp || lsbp->toSInt() + nodep->widthConst() > refp->varp()->width()) {
            block(refp->varp(), vscpInfoOf(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            തിരികെ;
        }
        // Record partial access
        record(nodep, refp, lsbp->toSInt(), lsbp->toSInt() + nodep->widthConst() - 1);
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        // Record whole access
        record(nodep, nodep, 0, nodep->varp()->width() - 1);
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        // The AstVarScope is not known, so all of them are blocked, by making it ineligible
        AstVar* സ്ഥിരം varp = nodep->varp();
        എങ്കിൽ (!isEligible(varp)) തിരികെ;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    explicit BitBlastVisitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        വേണ്ടി (AstVarScope* സ്ഥിരം vscp : m_vscps) {
            എങ്കിൽ (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        വേണ്ടി (AstVarScope* സ്ഥിരം vscp : m_vscps) {
            സ്ഥിരം VscpInfo& info = m_vscpInfos(vscp);
            എങ്കിൽ (info.fragments.empty()) തുടരുക;
            വേണ്ടി (സ്ഥിരം RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        വേണ്ടി (AstVarScope* സ്ഥിരം vscp : m_vscps) {
            സ്ഥിരം VscpInfo& info = m_vscpInfos(vscp);
            എങ്കിൽ (info.fragments.empty()) തുടരുക;
            വേണ്ടി (സ്ഥിരം WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        വേണ്ടി (AstVarScope* സ്ഥിരം vscp : m_vscps) {
            സ്ഥിരം VscpInfo& info = m_vscpInfos(vscp);
            എങ്കിൽ (info.fragments.empty()) തുടരുക;
            എങ്കിൽ (!(vscp->varp()->isTrace() && vscp->isTrace())) തുടരുക;
            AstActive* സ്ഥിരം activep = comboActive(vscp->scopep());
            File_വരി* സ്ഥിരം flp = vscp->fileline();
            വേണ്ടി (സ്ഥിരം Fragment& fragment : info.fragments) {
                AstVarRef* സ്ഥിരം lhsRefp = പുതിയ AstVarRef{flp, vscp, VAccess::WRITE};
                സ്ഥിരം പൂർണ്ണസംഖ്യ width = fragment.msb - fragment.lsb + 1;
                AstSel* സ്ഥിരം lhsp = പുതിയ AstSel{flp, lhsRefp, fragment.lsb, width};
                AstVarRef* സ്ഥിരം rhsp = പുതിയ AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->addStmtsp(പുതിയ AstAlways{പുതിയ AstAssignW{flp, lhsp, rhsp}});
            }
        }

        V3Stats::addStat("Optimizations, Bitblast, variables split due to attribute",
                         m_statSplitAttr);
        V3Stats::addStat("Optimizations, Bitblast, variables split automatically",
                         m_statSplitAuto);
        V3Stats::addStat("Optimizations, Bitblast, fragments created", m_statFragments);
        V3Stats::addStat("Optimizations, Bitblast, assignments through temporary", m_statTmps);
    }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(AstNetlist* netlistp) { BitBlastVisitor{netlistp}; }
};

//######################################################################
// V3BitBlast class functions

ശൂന്യം V3BitBlast::bitBlastAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    BitBlastVisitor::apply(nodep);
    V3Global::dump_Check_Global_വൃക്ഷം("bitblast", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
