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

वग्ग BitBlastVisitor final : सार्वजनिक VNVisitorConst, सार्वजनिक VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    संरचना RdRef final {
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp;  // Sel, or the VarRef if whole
        पूर्णाङ्क lsb;  // First bit read
        पूर्णाङ्क msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    संरचना WrRef final {
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp;  // The Sel, or the VarRef if whole
        Ast_ग्रन्थिः_Assign* assignp;  // The assignment, if it can be expanded
        पूर्णाङ्क lsb;  // The first bit written
        पूर्णाङ्क msb;  // The last bit written
    };

    // A fragment of a split AstVarScope
    संरचना Fragment final {
        पूर्णाङ्क lsb;  // The first bit in the variable
        पूर्णाङ्क msb;  // The last bit in the variable
        AstVarScope* vscp;  // The AstVarScope of the fragment
    };

    // The information about an AstVarScope of an eligible variable
    संरचना VscpInfo final {
        std::vector<RdRef> rdRefs;  // The reads of the AstVarScope
        std::vector<WrRef> wrRefs;  // The writes of the AstVarScope
        std::vector<Fragment> fragments;  // The fragments in LSB order, empty if not split
        तार्किक blocked = मिच्छा;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    ध्रुव VNUser1InUse m_user1InUse;
    ध्रुव VNUser2InUse m_user2InUse;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<पूर्णाङ्क, पूर्णाङ्क>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::vector<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_ग्रन्थिः_Assign* m_assignp = nullptr;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_statSplitAttr;  // AstVarScopes split only due to split_var
    VDouble0 m_statSplitAuto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    स्थिर शून्य warnNoSplit(AstVar* varp, ध्रुव Ast_ग्रन्थिः* wherep, ध्रुव वर्ण* reasonp) {
        // Only warn if user requested splitting
        सचे (!varp->attrSplitVar()) पटिनिवत्त;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, सच्च);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    स्थिर तार्किक isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        सचे (!varp->user1()) {
            ध्रुव तार्किक eligible = [&]() {
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                सचे (!dtypep->isIntegralOrPacked()) पटिनिवत्त मिच्छा;
                // Wider than one bit
                सचे (dtypep->width() <= 1) पटिनिवत्त मिच्छा;
                // Check properties
                ध्रुव वर्ण* ध्रुव reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                सचे (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                पटिनिवत्त !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        पटिनिवत्त varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscpInfoOf(AstVarScope* vscp) {
        सचे (VscpInfo* ध्रुव infop = m_vscpInfos.tryGet(vscp)) पटिनिवत्त *infop;
        // Also record on first encounter
        m_vscps.प्रक्षिप_back(vscp);
        पटिनिवत्त m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    स्थिर शून्य block(AstVar* varp, VscpInfo& info, ध्रुव Ast_ग्रन्थिः* wherep) {
        warnNoSplit(varp, wherep, "it is referenced in an unsupported way");
        सचे (info.blocked) पटिनिवत्त;
        info.blocked = सच्च;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    शून्य record(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstVarRef* refp, पूर्णाङ्क lsb, पूर्णाङ्क msb) {
        AstVar* ध्रुव varp = refp->varp();
        सचे (!isEligible(varp)) पटिनिवत्त;
        VscpInfo& info = vscpInfoOf(refp->varScopep());
        // RW reference blocks splitting
        सचे (refp->access().isRW()) block(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        सचे (info.blocked) पटिनिवत्त;
        // Record reference
        सचे (refp->access().isReadOnly()) {
            info.rdRefs.प्रक्षिप_back({exprp, lsb, msb});
            पटिनिवत्त;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        ध्रुव तार्किक canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.प्रक्षिप_back({exprp, canExpand ? m_assignp : nullptr, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    शून्य visitAssignment(Ast_ग्रन्थिः_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? nullptr : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, पूर्णाङ्क lsb, पूर्णाङ्क msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        सचे (fragVarpr) पटिनिवत्त fragVarpr;
        ध्रुव पूर्णाङ्क width = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_ग्रन्थिः::encode_सङ्ख्या(msb);
        सचे (width > 1) name += Ast_ग्रन्थिः::encodeName(":") + Ast_ग्रन्थिः::encode_सङ्ख्या(lsb);
        name += "__KET__";
        Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep()->isFourstate()
                                         ? varp->findLogicDType(width, width, VSigning::UNSIGNED)
                                         : varp->findBitDType(width, width, VSigning::UNSIGNED);
        fragVarpr = नूतन AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagateSplitAttrFrom(varp);
        varp->addHereThisAsNext(fragVarpr);
        पटिनिवत्त fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    शून्य decide(AstVarScope* vscp, VscpInfo& info) {
        सचे (info.blocked) पटिनिवत्त;
        AstVar* ध्रुव varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        ध्रुव तार्किक isAuto = [&]() {
            std::vector<std::pair<पूर्णाङ्क, पूर्णाङ्क>> ranges;  // [lsb, msb] inclusive
            अत्थाय (ध्रुव RdRef& ref : info.rdRefs) {
                सचे (VN_IS(ref.exprp, VarRef)) पटिनिवत्त मिच्छा;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            अत्थाय (ध्रुव WrRef& ref : info.wrRefs) {
                सचे (VN_IS(ref.exprp, VarRef)) पटिनिवत्त मिच्छा;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            std::sort(ranges.begin(), ranges.end());
            अत्थाय (size_t i = 0; i + 1 < ranges.size(); ++i) {
                ध्रुव std::pair<पूर्णाङ्क, पूर्णाङ्क>& a = ranges[i];
                ध्रुव std::pair<पूर्णाङ्क, पूर्णाङ्क>& b = ranges[i + 1];
                सचे (a != b && a.second >= b.first) पटिनिवत्त मिच्छा;
            }
            पटिनिवत्त सच्च;
        }();
        // Otherwise only if requested
        सचे (!isAuto && !varp->attrSplitVar()) पटिनिवत्त;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        पूर्णाङ्क lsb = varp->width();
        पूर्णाङ्क msb = -1;
        अत्थाय (ध्रुव RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::vector<पूर्णाङ्क> bounds;
        अत्थाय (ध्रुव WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.प्रक्षिप_back(ref.lsb);
            bounds.प्रक्षिप_back(ref.msb + 1);
        }
        bounds.प्रक्षिप_back(lsb);
        bounds.प्रक्षिप_back(msb + 1);
        std::sort(bounds.begin(), bounds.end());
        bounds.erase(std::unique(bounds.begin(), bounds.end()), bounds.end());
        // Nothing to do if the only fragment is the whole variable
        सचे (bounds.size() == 2 && lsb == 0 && msb == varp->width() - 1) पटिनिवत्त;
        // Writes that cannot be expanded must write one fragment
        अत्थाय (ध्रुव WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            सचे (ref.assignp) अनुवर्त;
            // If write is to a whole fragment, it can be replaced
            सचे (*std::upper_bound(bounds.begin(), bounds.end(), ref.lsb) == ref.msb + 1) {
                अनुवर्त;
            }
            // Otherwise can't split
            warnNoSplit(varp, ref.exprp, "it is written in an unsupported way");
            पटिनिवत्त;
        }

        // Splitting this variable
        ++(isAuto ? m_statSplitAuto : m_statSplitAttr);

        // Create the fragment AstVarScopes
        अत्थाय (size_t i = 0; i + 1 < bounds.size(); ++i) {
            ध्रुव पूर्णाङ्क lsb = bounds[i];
            ध्रुव पूर्णाङ्क msb = bounds[i + 1] - 1;
            AstVar* ध्रुव fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* ध्रुव newp = नूतन AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->addHereThisAsNext(newp);
            info.fragments.प्रक्षिप_back({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    स्थिर size_t fragmentIndex(ध्रुव VscpInfo& info, पूर्णाङ्क bit) {
        ध्रुव auto it = std::upper_bound(info.fragments.begin(), info.fragments.end(), bit,
                                         [](पूर्णाङ्क b, ध्रुव Fragment& fragment) {  //
                                             पटिनिवत्त b <= fragment.msb;
                                         });
        पटिनिवत्त it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    शून्य rewriteRead(ध्रुव RdRef& ref, ध्रुव VscpInfo& info) {
        File_पङ्क्तिः* ध्रुव flp = ref.exprp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* resultp = nullptr;
        अत्थाय (size_t i = fragmentIndex(info, ref.lsb); i < info.fragments.size(); ++i) {
            ध्रुव Fragment& fragment = info.fragments[i];
            सचे (fragment.lsb > ref.msb) भङ्ग;
            ध्रुव पूर्णाङ्क partLsb = std::max(ref.lsb, fragment.lsb);
            ध्रुव पूर्णाङ्क partMsb = std::min(ref.msb, fragment.msb);
            Ast_ग्रन्थिः_व्यञ्जकम्* bitsp = नूतन AstVarRef{flp, fragment.vscp, VAccess::READ};
            सचे (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = नूतन AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? नूतन AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    स्थिर तार्किक isSelectable(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        सचे (VN_IS(nodep, Const)) पटिनिवत्त सच्च;
        सचे (ध्रुव AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) {
            सचे (!VN_IS(selp->lsbp(), Const)) पटिनिवत्त मिच्छा;
            nodep = selp->fromp();
        }
        ध्रुव AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef);
        सचे (!refp) पटिनिवत्त मिच्छा;
        // Not a forced variable (trips several bugs in V3Force)
        सचे (refp->varp()->isForced()) पटिनिवत्त मिच्छा;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        पटिनिवत्त !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    शून्य rewriteWrite(AstScope* scopep, ध्रुव WrRef& ref, ध्रुव VscpInfo& info) {
        File_पङ्क्तिः* ध्रुव flp = ref.exprp->fileline();
        ध्रुव size_t first = fragmentIndex(info, ref.lsb);
        ध्रुव size_t last = fragmentIndex(info, ref.msb);
        UASSERT_OBJ(info.fragments[first].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        सचे (first == last) {
            ref.exprp->replaceWith(नूतन AstVarRef{flp, info.fragments[first].vscp, VAccess::WRITE});
            VL_DO_DANGLING(प्रक्षिप_Deletep(ref.exprp), ref.exprp);
            पटिनिवत्त;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_ग्रन्थिः_Assign* ध्रुव origp = ref.assignp;
        Ast_ग्रन्थिः_व्यञ्जकम्* rhsp = origp->rhsp()->unlinkFrBack();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        सचे (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* ध्रुव tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* ध्रुव tmpRefp = नूतन AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->addHereThisAsNext(नूतन AstAssign{flp, tmpRefp, rhsp});
            rhsp = नूतन AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        अत्थाय (size_t i = first; i <= last; ++i) {
            ध्रुव Fragment& fragment = info.fragments[i];
            ध्रुव पूर्णाङ्क lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            ध्रुव पूर्णाङ्क width = fragment.msb - fragment.lsb + 1;
            AstVarRef* ध्रुव lhsp = नूतन AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* ध्रुव selp = नूतन AstSel{flp, rhsp->clone_वृक्षः_Pure(मिच्छा), lsb, width};
            origp->addHereThisAsNext(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    स्थिर AstActive* comboActive(AstScope* scopep) {
        सचे (!scopep->user2p()) scopep->user2p(scopep->comboActivep(सच्च));
        पटिनिवत्त VN_AS(scopep->user2p(), Active);
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_DType*) override {}  // No references in data types
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }
    शून्य visit(AstAssign* nodep) override { visitAssignment(nodep); }
    शून्य visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    शून्य visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    शून्य visit(AstSel* nodep) override {
        AstVarRef* ध्रुव refp = VN_CAST(nodep->fromp(), VarRef);
        सचे (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            पटिनिवत्त;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        ध्रुव AstConst* ध्रुव lsbp = VN_CAST(nodep->lsbp(), Const);
        सचे (!lsbp || lsbp->toSInt() + nodep->widthConst() > refp->varp()->width()) {
            block(refp->varp(), vscpInfoOf(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            पटिनिवत्त;
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
        AstVar* ध्रुव varp = nodep->varp();
        सचे (!isEligible(varp)) पटिनिवत्त;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    explicit BitBlastVisitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        अत्थाय (AstVarScope* ध्रुव vscp : m_vscps) {
            सचे (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        अत्थाय (AstVarScope* ध्रुव vscp : m_vscps) {
            ध्रुव VscpInfo& info = m_vscpInfos(vscp);
            सचे (info.fragments.empty()) अनुवर्त;
            अत्थाय (ध्रुव RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        अत्थाय (AstVarScope* ध्रुव vscp : m_vscps) {
            ध्रुव VscpInfo& info = m_vscpInfos(vscp);
            सचे (info.fragments.empty()) अनुवर्त;
            अत्थाय (ध्रुव WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        अत्थाय (AstVarScope* ध्रुव vscp : m_vscps) {
            ध्रुव VscpInfo& info = m_vscpInfos(vscp);
            सचे (info.fragments.empty()) अनुवर्त;
            सचे (!(vscp->varp()->isTrace() && vscp->isTrace())) अनुवर्त;
            AstActive* ध्रुव activep = comboActive(vscp->scopep());
            File_पङ्क्तिः* ध्रुव flp = vscp->fileline();
            अत्थाय (ध्रुव Fragment& fragment : info.fragments) {
                AstVarRef* ध्रुव lhsRefp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
                ध्रुव पूर्णाङ्क width = fragment.msb - fragment.lsb + 1;
                AstSel* ध्रुव lhsp = नूतन AstSel{flp, lhsRefp, fragment.lsb, width};
                AstVarRef* ध्रुव rhsp = नूतन AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->addStmtsp(नूतन AstAlways{नूतन AstAssignW{flp, lhsp, rhsp}});
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
    V3Global::dump_Check_Global_वृक्षः("bitblast", 0, dump_वृक्षः_Either_Level() >= 3);
}
