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
        Ast_गाँठो_अभिव्यक्ति* exprp;  // Sel, or the VarRef if whole
        पूर्णाङ्क lsb;  // First bit read
        पूर्णाङ्क msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    संरचना WrRef final {
        Ast_गाँठो_अभिव्यक्ति* exprp;  // The Sel, or the VarRef if whole
        Ast_गाँठो_Assign* assignp;  // The assignment, if it can be expanded
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
        तार्किक blocked = असत्य;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    अचर VNUser1InUse m_user1InUse;
    अचर VNUser2InUse m_user2InUse;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<पूर्णाङ्क, पूर्णाङ्क>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::vector<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_गाँठो_Assign* m_assignp = nullptr;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_statSplitAttr;  // AstVarScopes split only due to split_var
    VDouble0 m_statSplitAuto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    स्थिर शून्य warnNoSplit(AstVar* varp, अचर Ast_गाँठो* wherep, अचर वर्ण* reasonp) {
        // Only warn if user requested splitting
        यदि (!varp->attrSplitVar()) फर्काऊ;
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
        यदि (!varp->user1()) {
            अचर तार्किक eligible = [&]() {
                अचर Ast_गाँठो_DType* अचर dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                यदि (!dtypep->isIntegralOrPacked()) फर्काऊ असत्य;
                // Wider than one bit
                यदि (dtypep->width() <= 1) फर्काऊ असत्य;
                // Check properties
                अचर वर्ण* अचर reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                यदि (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                फर्काऊ !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        फर्काऊ varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscpInfoOf(AstVarScope* vscp) {
        यदि (VscpInfo* अचर infop = m_vscpInfos.tryGet(vscp)) फर्काऊ *infop;
        // Also record on first encounter
        m_vscps.थप_back(vscp);
        फर्काऊ m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    स्थिर शून्य block(AstVar* varp, VscpInfo& info, अचर Ast_गाँठो* wherep) {
        warnNoSplit(varp, wherep, "it is referenced in an unsupported way");
        यदि (info.blocked) फर्काऊ;
        info.blocked = सत्य;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    शून्य record(Ast_गाँठो_अभिव्यक्ति* exprp, AstVarRef* refp, पूर्णाङ्क lsb, पूर्णाङ्क msb) {
        AstVar* अचर varp = refp->varp();
        यदि (!isEligible(varp)) फर्काऊ;
        VscpInfo& info = vscpInfoOf(refp->varScopep());
        // RW reference blocks splitting
        यदि (refp->access().isRW()) block(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        यदि (info.blocked) फर्काऊ;
        // Record reference
        यदि (refp->access().isReadOnly()) {
            info.rdRefs.थप_back({exprp, lsb, msb});
            फर्काऊ;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        अचर तार्किक canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.थप_back({exprp, canExpand ? m_assignp : nullptr, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    शून्य visitAssignment(Ast_गाँठो_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? nullptr : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, पूर्णाङ्क lsb, पूर्णाङ्क msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        यदि (fragVarpr) फर्काऊ fragVarpr;
        अचर पूर्णाङ्क width = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_गाँठो::encode_सङ्ख्या(msb);
        यदि (width > 1) name += Ast_गाँठो::encodeName(":") + Ast_गाँठो::encode_सङ्ख्या(lsb);
        name += "__KET__";
        Ast_गाँठो_DType* अचर dtypep = varp->dtypep()->isFourstate()
                                         ? varp->findLogicDType(width, width, VSigning::UNSIGNED)
                                         : varp->findBitDType(width, width, VSigning::UNSIGNED);
        fragVarpr = नूतन AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagateSplitAttrFrom(varp);
        varp->addHereThisAsNext(fragVarpr);
        फर्काऊ fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    शून्य decide(AstVarScope* vscp, VscpInfo& info) {
        यदि (info.blocked) फर्काऊ;
        AstVar* अचर varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        अचर तार्किक isAuto = [&]() {
            std::vector<std::pair<पूर्णाङ्क, पूर्णाङ्क>> ranges;  // [lsb, msb] inclusive
            लागि (अचर RdRef& ref : info.rdRefs) {
                यदि (VN_IS(ref.exprp, VarRef)) फर्काऊ असत्य;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            लागि (अचर WrRef& ref : info.wrRefs) {
                यदि (VN_IS(ref.exprp, VarRef)) फर्काऊ असत्य;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            std::sort(ranges.begin(), ranges.end());
            लागि (माप_प्रकार i = 0; i + 1 < ranges.size(); ++i) {
                अचर std::pair<पूर्णाङ्क, पूर्णाङ्क>& a = ranges[i];
                अचर std::pair<पूर्णाङ्क, पूर्णाङ्क>& b = ranges[i + 1];
                यदि (a != b && a.second >= b.first) फर्काऊ असत्य;
            }
            फर्काऊ सत्य;
        }();
        // Otherwise only if requested
        यदि (!isAuto && !varp->attrSplitVar()) फर्काऊ;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        पूर्णाङ्क lsb = varp->width();
        पूर्णाङ्क msb = -1;
        लागि (अचर RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::vector<पूर्णाङ्क> bounds;
        लागि (अचर WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.थप_back(ref.lsb);
            bounds.थप_back(ref.msb + 1);
        }
        bounds.थप_back(lsb);
        bounds.थप_back(msb + 1);
        std::sort(bounds.begin(), bounds.end());
        bounds.erase(std::unique(bounds.begin(), bounds.end()), bounds.end());
        // Nothing to do if the only fragment is the whole variable
        यदि (bounds.size() == 2 && lsb == 0 && msb == varp->width() - 1) फर्काऊ;
        // Writes that cannot be expanded must write one fragment
        लागि (अचर WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            यदि (ref.assignp) जारी;
            // If write is to a whole fragment, it can be replaced
            यदि (*std::upper_bound(bounds.begin(), bounds.end(), ref.lsb) == ref.msb + 1) {
                जारी;
            }
            // Otherwise can't split
            warnNoSplit(varp, ref.exprp, "it is written in an unsupported way");
            फर्काऊ;
        }

        // Splitting this variable
        ++(isAuto ? m_statSplitAuto : m_statSplitAttr);

        // Create the fragment AstVarScopes
        लागि (माप_प्रकार i = 0; i + 1 < bounds.size(); ++i) {
            अचर पूर्णाङ्क lsb = bounds[i];
            अचर पूर्णाङ्क msb = bounds[i + 1] - 1;
            AstVar* अचर fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* अचर newp = नूतन AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->addHereThisAsNext(newp);
            info.fragments.थप_back({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    स्थिर माप_प्रकार fragmentIndex(अचर VscpInfo& info, पूर्णाङ्क bit) {
        अचर स्वतः it = std::upper_bound(info.fragments.begin(), info.fragments.end(), bit,
                                         [](पूर्णाङ्क b, अचर Fragment& fragment) {  //
                                             फर्काऊ b <= fragment.msb;
                                         });
        फर्काऊ it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    शून्य rewriteRead(अचर RdRef& ref, अचर VscpInfo& info) {
        File_पङ्क्ति* अचर flp = ref.exprp->fileline();
        Ast_गाँठो_अभिव्यक्ति* resultp = nullptr;
        लागि (माप_प्रकार i = fragmentIndex(info, ref.lsb); i < info.fragments.size(); ++i) {
            अचर Fragment& fragment = info.fragments[i];
            यदि (fragment.lsb > ref.msb) रोक;
            अचर पूर्णाङ्क partLsb = std::max(ref.lsb, fragment.lsb);
            अचर पूर्णाङ्क partMsb = std::min(ref.msb, fragment.msb);
            Ast_गाँठो_अभिव्यक्ति* bitsp = नूतन AstVarRef{flp, fragment.vscp, VAccess::READ};
            यदि (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = नूतन AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? नूतन AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(थप_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    स्थिर तार्किक isSelectable(अचर Ast_गाँठो_अभिव्यक्ति* nodep) {
        यदि (VN_IS(nodep, Const)) फर्काऊ सत्य;
        यदि (अचर AstSel* अचर selp = VN_CAST(nodep, Sel)) {
            यदि (!VN_IS(selp->lsbp(), Const)) फर्काऊ असत्य;
            nodep = selp->fromp();
        }
        अचर AstVarRef* अचर refp = VN_CAST(nodep, VarRef);
        यदि (!refp) फर्काऊ असत्य;
        // Not a forced variable (trips several bugs in V3Force)
        यदि (refp->varp()->isForced()) फर्काऊ असत्य;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        फर्काऊ !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    शून्य rewriteWrite(AstScope* scopep, अचर WrRef& ref, अचर VscpInfo& info) {
        File_पङ्क्ति* अचर flp = ref.exprp->fileline();
        अचर माप_प्रकार first = fragmentIndex(info, ref.lsb);
        अचर माप_प्रकार last = fragmentIndex(info, ref.msb);
        UASSERT_OBJ(info.fragments[first].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        यदि (first == last) {
            ref.exprp->replaceWith(नूतन AstVarRef{flp, info.fragments[first].vscp, VAccess::WRITE});
            VL_DO_DANGLING(थप_Deletep(ref.exprp), ref.exprp);
            फर्काऊ;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_गाँठो_Assign* अचर origp = ref.assignp;
        Ast_गाँठो_अभिव्यक्ति* rhsp = origp->rhsp()->unlinkFrBack();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        यदि (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* अचर tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* अचर tmpRefp = नूतन AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->addHereThisAsNext(नूतन AstAssign{flp, tmpRefp, rhsp});
            rhsp = नूतन AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        लागि (माप_प्रकार i = first; i <= last; ++i) {
            अचर Fragment& fragment = info.fragments[i];
            अचर पूर्णाङ्क lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            अचर पूर्णाङ्क width = fragment.msb - fragment.lsb + 1;
            AstVarRef* अचर lhsp = नूतन AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* अचर selp = नूतन AstSel{flp, rhsp->clone_रूख_Pure(असत्य), lsb, width};
            origp->addHereThisAsNext(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(थप_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(थप_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    स्थिर AstActive* comboActive(AstScope* scopep) {
        यदि (!scopep->user2p()) scopep->user2p(scopep->comboActivep(सत्य));
        फर्काऊ VN_AS(scopep->user2p(), Active);
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_DType*) override {}  // No references in data types
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }
    शून्य visit(AstAssign* nodep) override { visitAssignment(nodep); }
    शून्य visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    शून्य visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    शून्य visit(AstSel* nodep) override {
        AstVarRef* अचर refp = VN_CAST(nodep->fromp(), VarRef);
        यदि (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            फर्काऊ;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        अचर AstConst* अचर lsbp = VN_CAST(nodep->lsbp(), Const);
        यदि (!lsbp || lsbp->toSInt() + nodep->widthConst() > refp->varp()->width()) {
            block(refp->varp(), vscpInfoOf(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            फर्काऊ;
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
        AstVar* अचर varp = nodep->varp();
        यदि (!isEligible(varp)) फर्काऊ;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    explicit BitBlastVisitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        लागि (AstVarScope* अचर vscp : m_vscps) {
            यदि (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        लागि (AstVarScope* अचर vscp : m_vscps) {
            अचर VscpInfo& info = m_vscpInfos(vscp);
            यदि (info.fragments.empty()) जारी;
            लागि (अचर RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        लागि (AstVarScope* अचर vscp : m_vscps) {
            अचर VscpInfo& info = m_vscpInfos(vscp);
            यदि (info.fragments.empty()) जारी;
            लागि (अचर WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        लागि (AstVarScope* अचर vscp : m_vscps) {
            अचर VscpInfo& info = m_vscpInfos(vscp);
            यदि (info.fragments.empty()) जारी;
            यदि (!(vscp->varp()->isTrace() && vscp->isTrace())) जारी;
            AstActive* अचर activep = comboActive(vscp->scopep());
            File_पङ्क्ति* अचर flp = vscp->fileline();
            लागि (अचर Fragment& fragment : info.fragments) {
                AstVarRef* अचर lhsRefp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
                अचर पूर्णाङ्क width = fragment.msb - fragment.lsb + 1;
                AstSel* अचर lhsp = नूतन AstSel{flp, lhsRefp, fragment.lsb, width};
                AstVarRef* अचर rhsp = नूतन AstVarRef{flp, fragment.vscp, VAccess::READ};
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
    V3Global::dump_Check_Global_रूख("bitblast", 0, dump_रूख_Either_Level() >= 3);
}
