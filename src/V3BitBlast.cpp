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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3BitBlast.h"

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3SharedTmps.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <algorithm>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वर्ग BitBlastVisitor final : सार्वजनिक VNVisitorConst, सार्वजनिक VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    संरचना RdRef final {
        Ast_गाठ_अभिव्यक्ती* exprp;  // Sel, or the VarRef if whole
        पूर्णांक lsb;  // First bit read
        पूर्णांक msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    संरचना WrRef final {
        Ast_गाठ_अभिव्यक्ती* exprp;  // The Sel, or the VarRef if whole
        Ast_गाठ_Assign* assignp;  // The assignment, if it can be expanded
        पूर्णांक lsb;  // The first bit written
        पूर्णांक msb;  // The last bit written
    };

    // A fragment of a split AstVarScope
    संरचना Fragment final {
        पूर्णांक lsb;  // The first bit in the variable
        पूर्णांक msb;  // The last bit in the variable
        AstVarScope* vscp;  // The AstVarScope of the fragment
    };

    // The information about an AstVarScope of an eligible variable
    संरचना VscpInfo final {
        std::vector<RdRef> rdRefs;  // The reads of the AstVarScope
        std::vector<WrRef> wrRefs;  // The writes of the AstVarScope
        std::vector<Fragment> fragments;  // The fragments in LSB order, empty if not split
        तार्किक blocked = असत्य;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    स्थिरांक VNUser1InUse m_user1InUse;
    स्थिरांक VNUser2InUse m_user2InUse;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<पूर्णांक, पूर्णांक>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::vector<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_गाठ_Assign* m_assignp = nullptr;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_statSplitAttr;  // AstVarScopes split only due to split_var
    VDouble0 m_statSplitAuto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    स्थिर शून्य warnNoSplit(AstVar* varp, स्थिरांक Ast_गाठ* wherep, स्थिरांक अक्षर* reasonp) {
        // Only warn if user requested splitting
        जर (!varp->attrSplitVar()) परत;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, सत्य);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    स्थिर तार्किक isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        जर (!varp->user1()) {
            स्थिरांक तार्किक eligible = [&]() {
                स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                जर (!dtypep->isIntegralOrPacked()) परत असत्य;
                // Wider than one bit
                जर (dtypep->width() <= 1) परत असत्य;
                // Check properties
                स्थिरांक अक्षर* स्थिरांक reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                जर (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                परत !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        परत varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscpInfoOf(AstVarScope* vscp) {
        जर (VscpInfo* स्थिरांक infop = m_vscpInfos.tryGet(vscp)) परत *infop;
        // Also record on first encounter
        m_vscps.ढकला_back(vscp);
        परत m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    स्थिर शून्य block(AstVar* varp, VscpInfo& info, स्थिरांक Ast_गाठ* wherep) {
        warnNoSplit(varp, wherep, "it is referenced in an unsupported way");
        जर (info.blocked) परत;
        info.blocked = सत्य;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    शून्य record(Ast_गाठ_अभिव्यक्ती* exprp, AstVarRef* refp, पूर्णांक lsb, पूर्णांक msb) {
        AstVar* स्थिरांक varp = refp->varp();
        जर (!isEligible(varp)) परत;
        VscpInfo& info = vscpInfoOf(refp->varScopep());
        // RW reference blocks splitting
        जर (refp->access().isRW()) block(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        जर (info.blocked) परत;
        // Record reference
        जर (refp->access().isReadOnly()) {
            info.rdRefs.ढकला_back({exprp, lsb, msb});
            परत;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        स्थिरांक तार्किक canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.ढकला_back({exprp, canExpand ? m_assignp : nullptr, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    शून्य visitAssignment(Ast_गाठ_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? nullptr : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, पूर्णांक lsb, पूर्णांक msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        जर (fragVarpr) परत fragVarpr;
        स्थिरांक पूर्णांक width = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_गाठ::encode_संख्या(msb);
        जर (width > 1) name += Ast_गाठ::encodeName(":") + Ast_गाठ::encode_संख्या(lsb);
        name += "__KET__";
        Ast_गाठ_DType* स्थिरांक dtypep = varp->dtypep()->isFourstate()
                                         ? varp->findLogicDType(width, width, VSigning::UNSIGNED)
                                         : varp->findBitDType(width, width, VSigning::UNSIGNED);
        fragVarpr = नवीन AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagateSplitAttrFrom(varp);
        varp->addHereThisAsNext(fragVarpr);
        परत fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    शून्य decide(AstVarScope* vscp, VscpInfo& info) {
        जर (info.blocked) परत;
        AstVar* स्थिरांक varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        स्थिरांक तार्किक isAuto = [&]() {
            std::vector<std::pair<पूर्णांक, पूर्णांक>> ranges;  // [lsb, msb] inclusive
            साठी (स्थिरांक RdRef& ref : info.rdRefs) {
                जर (VN_IS(ref.exprp, VarRef)) परत असत्य;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            साठी (स्थिरांक WrRef& ref : info.wrRefs) {
                जर (VN_IS(ref.exprp, VarRef)) परत असत्य;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            std::sort(ranges.begin(), ranges.end());
            साठी (माप_प्रकार i = 0; i + 1 < ranges.size(); ++i) {
                स्थिरांक std::pair<पूर्णांक, पूर्णांक>& a = ranges[i];
                स्थिरांक std::pair<पूर्णांक, पूर्णांक>& b = ranges[i + 1];
                जर (a != b && a.second >= b.first) परत असत्य;
            }
            परत सत्य;
        }();
        // Otherwise only if requested
        जर (!isAuto && !varp->attrSplitVar()) परत;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        पूर्णांक lsb = varp->width();
        पूर्णांक msb = -1;
        साठी (स्थिरांक RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::vector<पूर्णांक> bounds;
        साठी (स्थिरांक WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.ढकला_back(ref.lsb);
            bounds.ढकला_back(ref.msb + 1);
        }
        bounds.ढकला_back(lsb);
        bounds.ढकला_back(msb + 1);
        std::sort(bounds.begin(), bounds.end());
        bounds.erase(std::unique(bounds.begin(), bounds.end()), bounds.end());
        // Nothing to do if the only fragment is the whole variable
        जर (bounds.size() == 2 && lsb == 0 && msb == varp->width() - 1) परत;
        // Writes that cannot be expanded must write one fragment
        साठी (स्थिरांक WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            जर (ref.assignp) सुरूठेवा;
            // If write is to a whole fragment, it can be replaced
            जर (*std::upper_bound(bounds.begin(), bounds.end(), ref.lsb) == ref.msb + 1) {
                सुरूठेवा;
            }
            // Otherwise can't split
            warnNoSplit(varp, ref.exprp, "it is written in an unsupported way");
            परत;
        }

        // Splitting this variable
        ++(isAuto ? m_statSplitAuto : m_statSplitAttr);

        // Create the fragment AstVarScopes
        साठी (माप_प्रकार i = 0; i + 1 < bounds.size(); ++i) {
            स्थिरांक पूर्णांक lsb = bounds[i];
            स्थिरांक पूर्णांक msb = bounds[i + 1] - 1;
            AstVar* स्थिरांक fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* स्थिरांक newp = नवीन AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->addHereThisAsNext(newp);
            info.fragments.ढकला_back({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    स्थिर माप_प्रकार fragmentIndex(स्थिरांक VscpInfo& info, पूर्णांक bit) {
        स्थिरांक स्वतः it = std::upper_bound(info.fragments.begin(), info.fragments.end(), bit,
                                         [](पूर्णांक b, स्थिरांक Fragment& fragment) {  //
                                             परत b <= fragment.msb;
                                         });
        परत it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    शून्य rewriteRead(स्थिरांक RdRef& ref, स्थिरांक VscpInfo& info) {
        File_ओळ* स्थिरांक flp = ref.exprp->fileline();
        Ast_गाठ_अभिव्यक्ती* resultp = nullptr;
        साठी (माप_प्रकार i = fragmentIndex(info, ref.lsb); i < info.fragments.size(); ++i) {
            स्थिरांक Fragment& fragment = info.fragments[i];
            जर (fragment.lsb > ref.msb) थांबा;
            स्थिरांक पूर्णांक partLsb = std::max(ref.lsb, fragment.lsb);
            स्थिरांक पूर्णांक partMsb = std::min(ref.msb, fragment.msb);
            Ast_गाठ_अभिव्यक्ती* bitsp = नवीन AstVarRef{flp, fragment.vscp, VAccess::READ};
            जर (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = नवीन AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? नवीन AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(ढकला_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    स्थिर तार्किक isSelectable(स्थिरांक Ast_गाठ_अभिव्यक्ती* nodep) {
        जर (VN_IS(nodep, Const)) परत सत्य;
        जर (स्थिरांक AstSel* स्थिरांक selp = VN_CAST(nodep, Sel)) {
            जर (!VN_IS(selp->lsbp(), Const)) परत असत्य;
            nodep = selp->fromp();
        }
        स्थिरांक AstVarRef* स्थिरांक refp = VN_CAST(nodep, VarRef);
        जर (!refp) परत असत्य;
        // Not a forced variable (trips several bugs in V3Force)
        जर (refp->varp()->isForced()) परत असत्य;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        परत !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    शून्य rewriteWrite(AstScope* scopep, स्थिरांक WrRef& ref, स्थिरांक VscpInfo& info) {
        File_ओळ* स्थिरांक flp = ref.exprp->fileline();
        स्थिरांक माप_प्रकार first = fragmentIndex(info, ref.lsb);
        स्थिरांक माप_प्रकार last = fragmentIndex(info, ref.msb);
        UASSERT_OBJ(info.fragments[first].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        जर (first == last) {
            ref.exprp->replaceWith(नवीन AstVarRef{flp, info.fragments[first].vscp, VAccess::WRITE});
            VL_DO_DANGLING(ढकला_Deletep(ref.exprp), ref.exprp);
            परत;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_गाठ_Assign* स्थिरांक origp = ref.assignp;
        Ast_गाठ_अभिव्यक्ती* rhsp = origp->rhsp()->unlinkFrBack();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        जर (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* स्थिरांक tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* स्थिरांक tmpRefp = नवीन AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->addHereThisAsNext(नवीन AstAssign{flp, tmpRefp, rhsp});
            rhsp = नवीन AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        साठी (माप_प्रकार i = first; i <= last; ++i) {
            स्थिरांक Fragment& fragment = info.fragments[i];
            स्थिरांक पूर्णांक lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            स्थिरांक पूर्णांक width = fragment.msb - fragment.lsb + 1;
            AstVarRef* स्थिरांक lhsp = नवीन AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* स्थिरांक selp = नवीन AstSel{flp, rhsp->clone_वृक्ष_Pure(असत्य), lsb, width};
            origp->addHereThisAsNext(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(ढकला_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ढकला_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    स्थिर AstActive* comboActive(AstScope* scopep) {
        जर (!scopep->user2p()) scopep->user2p(scopep->comboActivep(सत्य));
        परत VN_AS(scopep->user2p(), Active);
    }

    // VISITORS
    शून्य visit(Ast_गाठ_DType*) override {}  // No references in data types
    शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }
    शून्य visit(AstAssign* nodep) override { visitAssignment(nodep); }
    शून्य visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    शून्य visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    शून्य visit(AstSel* nodep) override {
        AstVarRef* स्थिरांक refp = VN_CAST(nodep->fromp(), VarRef);
        जर (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            परत;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        स्थिरांक AstConst* स्थिरांक lsbp = VN_CAST(nodep->lsbp(), Const);
        जर (!lsbp || lsbp->toSInt() + nodep->widthConst() > refp->varp()->width()) {
            block(refp->varp(), vscpInfoOf(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            परत;
        }
        // Record partial access
        record(nodep, refp, lsbp->toSInt(), lsbp->toSInt() + nodep->widthConst() - 1);
    }
    शून्य visit(AstVarRef* nodep) override {
        // Record whole access
        record(nodep, nodep, 0, nodep->varp()->width() - 1);
    }
    शून्य visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        // The AstVarScope is not known, so all of them are blocked, by making it ineligible
        AstVar* स्थिरांक varp = nodep->varp();
        जर (!isEligible(varp)) परत;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    explicit BitBlastVisitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        साठी (AstVarScope* स्थिरांक vscp : m_vscps) {
            जर (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        साठी (AstVarScope* स्थिरांक vscp : m_vscps) {
            स्थिरांक VscpInfo& info = m_vscpInfos(vscp);
            जर (info.fragments.empty()) सुरूठेवा;
            साठी (स्थिरांक RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        साठी (AstVarScope* स्थिरांक vscp : m_vscps) {
            स्थिरांक VscpInfo& info = m_vscpInfos(vscp);
            जर (info.fragments.empty()) सुरूठेवा;
            साठी (स्थिरांक WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        साठी (AstVarScope* स्थिरांक vscp : m_vscps) {
            स्थिरांक VscpInfo& info = m_vscpInfos(vscp);
            जर (info.fragments.empty()) सुरूठेवा;
            जर (!(vscp->varp()->isTrace() && vscp->isTrace())) सुरूठेवा;
            AstActive* स्थिरांक activep = comboActive(vscp->scopep());
            File_ओळ* स्थिरांक flp = vscp->fileline();
            साठी (स्थिरांक Fragment& fragment : info.fragments) {
                AstVarRef* स्थिरांक lhsRefp = नवीन AstVarRef{flp, vscp, VAccess::WRITE};
                स्थिरांक पूर्णांक width = fragment.msb - fragment.lsb + 1;
                AstSel* स्थिरांक lhsp = नवीन AstSel{flp, lhsRefp, fragment.lsb, width};
                AstVarRef* स्थिरांक rhsp = नवीन AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->addStmtsp(नवीन AstAlways{नवीन AstAssignW{flp, lhsp, rhsp}});
            }
        }

        V3Stats::addStat("Optimizations, Bitblast, variables split due to attribute",
                         m_statSplitAttr);
        V3Stats::addStat("Optimizations, Bitblast, variables split automatically",
                         m_statSplitAuto);
        V3Stats::addStat("Optimizations, Bitblast, fragments created", m_statFragments);
        V3Stats::addStat("Optimizations, Bitblast, assignments through temporary", m_statTmps);
    }

सार्वजनिक:
    स्थिर शून्य apply(AstNetlist* netlistp) { BitBlastVisitor{netlistp}; }
};

//######################################################################
// V3BitBlast class functions

शून्य V3BitBlast::bitBlastAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    BitBlastVisitor::apply(nodep);
    V3Global::dump_Check_Global_वृक्ष("bitblast", 0, dump_वृक्ष_Either_Level() >= 3);
}
