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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3BitBlast.h"

#समावेश "V3AstUserAllocator.h"
#समावेश "V3SharedTmps.h"
#समावेश "V3Stats.h"

#समावेश <algorithm>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

श्रेणी BitBlastVisitor final : खुला VNVisitorConst, खुला VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    काष्ठा RdRef final {
        Ast_गाँठ_व्यंजक* exprp;  // Sel, or the VarRef if whole
        पूर्णांक lsb;  // First bit read
        पूर्णांक msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    काष्ठा WrRef final {
        Ast_गाँठ_व्यंजक* exprp;  // The Sel, or the VarRef if whole
        Ast_गाँठ_Assign* assignp;  // The assignment, if it can be expanded
        पूर्णांक lsb;  // The first bit written
        पूर्णांक msb;  // The last bit written
    };

    // A fragment of a split AstVarScope
    काष्ठा Fragment final {
        पूर्णांक lsb;  // The first bit in the variable
        पूर्णांक msb;  // The last bit in the variable
        AstVarScope* vscp;  // The AstVarScope of the fragment
    };

    // The information about an AstVarScope of an eligible variable
    काष्ठा VscpInfo final {
        std::vector<RdRef> rdRefs;  // The reads of the AstVarScope
        std::vector<WrRef> wrRefs;  // The writes of the AstVarScope
        std::vector<Fragment> fragments;  // The fragments in LSB order, empty if not split
        bool blocked = false;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    स्थिर VNUser1InUse m_user1InUse;
    स्थिर VNUser2InUse m_user2InUse;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<पूर्णांक, पूर्णांक>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::vector<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_गाँठ_Assign* m_assignp = nullptr;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_statSplitAttr;  // AstVarScopes split only due to split_var
    VDouble0 m_statSplitAuto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    अटल व्योम warnNoSplit(AstVar* varp, स्थिर Ast_गाँठ* wherep, स्थिर अक्षर* reasonp) {
        // Only warn if user requested splitting
        अगर (!varp->attrSplitVar()) वापस;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, true);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    अटल bool isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        अगर (!varp->user1()) {
            स्थिर bool eligible = [&]() {
                स्थिर Ast_गाँठ_DType* स्थिर dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                अगर (!dtypep->isIntegralOrPacked()) वापस false;
                // Wider than one bit
                अगर (dtypep->width() <= 1) वापस false;
                // Check properties
                स्थिर अक्षर* स्थिर reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                अगर (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                वापस !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        वापस varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscpInfoOf(AstVarScope* vscp) {
        अगर (VscpInfo* स्थिर infop = m_vscpInfos.tryGet(vscp)) वापस *infop;
        // Also record on first encounter
        m_vscps.धकेलो_back(vscp);
        वापस m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    अटल व्योम block(AstVar* varp, VscpInfo& info, स्थिर Ast_गाँठ* wherep) {
        warnNoSplit(varp, wherep, "it is referenced in an unsupported way");
        अगर (info.blocked) वापस;
        info.blocked = true;
        info.rdRefs.सफा();
        info.wrRefs.सफा();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    व्योम record(Ast_गाँठ_व्यंजक* exprp, AstVarRef* refp, पूर्णांक lsb, पूर्णांक msb) {
        AstVar* स्थिर varp = refp->varp();
        अगर (!isEligible(varp)) वापस;
        VscpInfo& info = vscpInfoOf(refp->varScopep());
        // RW reference blocks splitting
        अगर (refp->access().isRW()) block(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        अगर (info.blocked) वापस;
        // Record reference
        अगर (refp->access().isReadOnly()) {
            info.rdRefs.धकेलो_back({exprp, lsb, msb});
            वापस;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        स्थिर bool canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.धकेलो_back({exprp, canExpand ? m_assignp : nullptr, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    व्योम visitAssignment(Ast_गाँठ_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? nullptr : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, पूर्णांक lsb, पूर्णांक msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        अगर (fragVarpr) वापस fragVarpr;
        स्थिर पूर्णांक width = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_गाँठ::encode_संख्या(msb);
        अगर (width > 1) name += Ast_गाँठ::encodeName(":") + Ast_गाँठ::encode_संख्या(lsb);
        name += "__KET__";
        Ast_गाँठ_DType* स्थिर dtypep = varp->dtypep()->isFourstate()
                                         ? varp->findLogicDType(width, width, VSigning::UNSIGNED)
                                         : varp->findBitDType(width, width, VSigning::UNSIGNED);
        fragVarpr = नया AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagateSplitAttrFrom(varp);
        varp->add_Here_This_As_अग्र(fragVarpr);
        वापस fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    व्योम decide(AstVarScope* vscp, VscpInfo& info) {
        अगर (info.blocked) वापस;
        AstVar* स्थिर varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        स्थिर bool isAuto = [&]() {
            std::vector<std::pair<पूर्णांक, पूर्णांक>> ranges;  // [lsb, msb] inclusive
            क्रम (स्थिर RdRef& ref : info.rdRefs) {
                अगर (VN_IS(ref.exprp, VarRef)) वापस false;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            क्रम (स्थिर WrRef& ref : info.wrRefs) {
                अगर (VN_IS(ref.exprp, VarRef)) वापस false;
                ranges.emplace_back(ref.lsb, ref.msb);
            }
            std::sort(ranges.begin(), ranges.end());
            क्रम (माप_प्रकार i = 0; i + 1 < ranges.आकार(); ++i) {
                स्थिर std::pair<पूर्णांक, पूर्णांक>& a = ranges[i];
                स्थिर std::pair<पूर्णांक, पूर्णांक>& b = ranges[i + 1];
                अगर (a != b && a.second >= b.first) वापस false;
            }
            वापस true;
        }();
        // Otherwise only if requested
        अगर (!isAuto && !varp->attrSplitVar()) वापस;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        पूर्णांक lsb = varp->width();
        पूर्णांक msb = -1;
        क्रम (स्थिर RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::vector<पूर्णांक> bounds;
        क्रम (स्थिर WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.धकेलो_back(ref.lsb);
            bounds.धकेलो_back(ref.msb + 1);
        }
        bounds.धकेलो_back(lsb);
        bounds.धकेलो_back(msb + 1);
        std::sort(bounds.begin(), bounds.end());
        bounds.erase(std::unique(bounds.begin(), bounds.end()), bounds.end());
        // Nothing to do if the only fragment is the whole variable
        अगर (bounds.आकार() == 2 && lsb == 0 && msb == varp->width() - 1) वापस;
        // Writes that cannot be expanded must write one fragment
        क्रम (स्थिर WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            अगर (ref.assignp) जारी;
            // If write is to a whole fragment, it can be replaced
            अगर (*std::upper_bound(bounds.begin(), bounds.end(), ref.lsb) == ref.msb + 1) {
                जारी;
            }
            // Otherwise can't split
            warnNoSplit(varp, ref.exprp, "it is written in an unsupported way");
            वापस;
        }

        // Splitting this variable
        ++(isAuto ? m_statSplitAuto : m_statSplitAttr);

        // Create the fragment AstVarScopes
        क्रम (माप_प्रकार i = 0; i + 1 < bounds.आकार(); ++i) {
            स्थिर पूर्णांक lsb = bounds[i];
            स्थिर पूर्णांक msb = bounds[i + 1] - 1;
            AstVar* स्थिर fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* स्थिर newp = नया AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->add_Here_This_As_अग्र(newp);
            info.fragments.धकेलो_back({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    अटल माप_प्रकार fragment_सूचकांक(स्थिर VscpInfo& info, पूर्णांक bit) {
        स्थिर स्वतः it = std::upper_bound(info.fragments.begin(), info.fragments.end(), bit,
                                         [](पूर्णांक b, स्थिर Fragment& fragment) {  //
                                             वापस b <= fragment.msb;
                                         });
        वापस it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    व्योम rewriteRead(स्थिर RdRef& ref, स्थिर VscpInfo& info) {
        File_पंक्ति* स्थिर flp = ref.exprp->fileline();
        Ast_गाँठ_व्यंजक* resultp = nullptr;
        क्रम (माप_प्रकार i = fragment_सूचकांक(info, ref.lsb); i < info.fragments.आकार(); ++i) {
            स्थिर Fragment& fragment = info.fragments[i];
            अगर (fragment.lsb > ref.msb) अवरोध;
            स्थिर पूर्णांक partLsb = std::max(ref.lsb, fragment.lsb);
            स्थिर पूर्णांक partMsb = std::min(ref.msb, fragment.msb);
            Ast_गाँठ_व्यंजक* bitsp = नया AstVarRef{flp, fragment.vscp, VAccess::READ};
            अगर (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = नया AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? नया AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(धकेलो_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    अटल bool isSelectable(स्थिर Ast_गाँठ_व्यंजक* nodep) {
        अगर (VN_IS(nodep, Const)) वापस true;
        अगर (स्थिर AstSel* स्थिर selp = VN_CAST(nodep, Sel)) {
            अगर (!VN_IS(selp->lsbp(), Const)) वापस false;
            nodep = selp->fromp();
        }
        स्थिर AstVarRef* स्थिर refp = VN_CAST(nodep, VarRef);
        अगर (!refp) वापस false;
        // Not a forced variable (trips several bugs in V3Force)
        अगर (refp->varp()->isForced()) वापस false;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        वापस !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    व्योम rewriteWrite(AstScope* scopep, स्थिर WrRef& ref, स्थिर VscpInfo& info) {
        File_पंक्ति* स्थिर flp = ref.exprp->fileline();
        स्थिर माप_प्रकार first = fragment_सूचकांक(info, ref.lsb);
        स्थिर माप_प्रकार last = fragment_सूचकांक(info, ref.msb);
        UASSERT_OBJ(info.fragments[first].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        अगर (first == last) {
            ref.exprp->replaceWith(नया AstVarRef{flp, info.fragments[first].vscp, VAccess::WRITE});
            VL_DO_DANGLING(धकेलो_Deletep(ref.exprp), ref.exprp);
            वापस;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_गाँठ_Assign* स्थिर origp = ref.assignp;
        Ast_गाँठ_व्यंजक* rhsp = origp->rhsp()->unlinkFrBack();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        अगर (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* स्थिर tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* स्थिर tmpRefp = नया AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->add_Here_This_As_अग्र(नया AstAssign{flp, tmpRefp, rhsp});
            rhsp = नया AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        क्रम (माप_प्रकार i = first; i <= last; ++i) {
            स्थिर Fragment& fragment = info.fragments[i];
            स्थिर पूर्णांक lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            स्थिर पूर्णांक width = fragment.msb - fragment.lsb + 1;
            AstVarRef* स्थिर lhsp = नया AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* स्थिर selp = नया AstSel{flp, rhsp->clone_वृक्ष_Pure(false), lsb, width};
            origp->add_Here_This_As_अग्र(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(धकेलो_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(धकेलो_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    अटल AstActive* comboActive(AstScope* scopep) {
        अगर (!scopep->user2p()) scopep->user2p(scopep->comboActivep(true));
        वापस VN_AS(scopep->user2p(), Active);
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_DType*) override {}  // No references in data types
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }
    व्योम visit(AstAssign* nodep) override { visitAssignment(nodep); }
    व्योम visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    व्योम visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    व्योम visit(AstSel* nodep) override {
        AstVarRef* स्थिर refp = VN_CAST(nodep->fromp(), VarRef);
        अगर (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            वापस;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        स्थिर AstConst* स्थिर lsbp = VN_CAST(nodep->lsbp(), Const);
        अगर (!lsbp || lsbp->toSInt() + nodep->widthConst() > refp->varp()->width()) {
            block(refp->varp(), vscpInfoOf(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            वापस;
        }
        // Record partial access
        record(nodep, refp, lsbp->toSInt(), lsbp->toSInt() + nodep->widthConst() - 1);
    }
    व्योम visit(AstVarRef* nodep) override {
        // Record whole access
        record(nodep, nodep, 0, nodep->varp()->width() - 1);
    }
    व्योम visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        // The AstVarScope is not known, so all of them are blocked, by making it ineligible
        AstVar* स्थिर varp = nodep->varp();
        अगर (!isEligible(varp)) वापस;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    explicit BitBlastVisitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        क्रम (AstVarScope* स्थिर vscp : m_vscps) {
            अगर (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        क्रम (AstVarScope* स्थिर vscp : m_vscps) {
            स्थिर VscpInfo& info = m_vscpInfos(vscp);
            अगर (info.fragments.empty()) जारी;
            क्रम (स्थिर RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        क्रम (AstVarScope* स्थिर vscp : m_vscps) {
            स्थिर VscpInfo& info = m_vscpInfos(vscp);
            अगर (info.fragments.empty()) जारी;
            क्रम (स्थिर WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        क्रम (AstVarScope* स्थिर vscp : m_vscps) {
            स्थिर VscpInfo& info = m_vscpInfos(vscp);
            अगर (info.fragments.empty()) जारी;
            अगर (!(vscp->varp()->isTrace() && vscp->isTrace())) जारी;
            AstActive* स्थिर activep = comboActive(vscp->scopep());
            File_पंक्ति* स्थिर flp = vscp->fileline();
            क्रम (स्थिर Fragment& fragment : info.fragments) {
                AstVarRef* स्थिर lhsRefp = नया AstVarRef{flp, vscp, VAccess::WRITE};
                स्थिर पूर्णांक width = fragment.msb - fragment.lsb + 1;
                AstSel* स्थिर lhsp = नया AstSel{flp, lhsRefp, fragment.lsb, width};
                AstVarRef* स्थिर rhsp = नया AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->addStmtsp(नया AstAlways{नया AstAssignW{flp, lhsp, rhsp}});
            }
        }

        V3Stats::addStat("Optimizations, Bitblast, variables split due to attribute",
                         m_statSplitAttr);
        V3Stats::addStat("Optimizations, Bitblast, variables split automatically",
                         m_statSplitAuto);
        V3Stats::addStat("Optimizations, Bitblast, fragments created", m_statFragments);
        V3Stats::addStat("Optimizations, Bitblast, assignments through temporary", m_statTmps);
    }

खुला:
    अटल व्योम apply(AstNetlist* netlistp) { BitBlastVisitor{netlistp}; }
};

//######################################################################
// V3BitBlast class functions

व्योम V3BitBlast::bitBlastAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    BitBlastVisitor::apply(nodep);
    V3Global::dump_Check_Global_वृक्ष("bitblast", 0, dump_वृक्ष_Either_Level() >= 3);
}
