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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3BitBlast.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3SharedTmps.h"
#شامل "V3Stats.h"

#شامل <algorithm>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

جماعت ثنائی_ہندسہ_Blast_Visitor final : عوامی VNVisitorConst, عوامی VNDeleter {
    // TYPES

    // A read of a variable: a constant bit select or a whole VarRef
    ساخت RdRef final {
        Ast_گرہ_اظہاریہ* exprp;  // Sel, or the VarRef if whole
        صحیح lsb;  // First bit read
        صحیح msb;  // Last bit read
    };

    // A write of a variable: a constant bit select or a whole VarRef
    ساخت WrRef final {
        Ast_گرہ_اظہاریہ* exprp;  // The Sel, or the VarRef if whole
        Ast_گرہ_تفویض* assignp;  // The assignment, if it can be expanded
        صحیح lsb;  // The first bit written
        صحیح msb;  // The last bit written
    };

    // A fragment of a split AstVarScope
    ساخت Fragment final {
        صحیح lsb;  // The first bit in the variable
        صحیح msb;  // The last bit in the variable
        AstVarScope* vscp;  // The AstVarScope of the fragment
    };

    // The information about an AstVarScope of an eligible variable
    ساخت VscpInfo final {
        std::سمتیہ<RdRef> rdRefs;  // The reads of the AstVarScope
        std::سمتیہ<WrRef> wrRefs;  // The writes of the AstVarScope
        std::سمتیہ<Fragment> fragments;  // The fragments in LSB order, empty if not split
        منطقی blocked = جھوٹ;  // Referenced other than by an RdRef or WrRef
    };

    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstVarScope::user1()        -> VscpInfo, via m_vscpInfos
    //  AstVar::user2()             -> Fragment AstVars by LSB and MSB, via m_fragmentVarps
    //  AstScope::user2p()          -> AstActive*: combinational active, see comboActive
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;
    ثابت VNUser2_اندر_Use m_user2_اندر_Use;

    // STATE
    // The fragment AstVars created so far, by LSB and MSB. AstVarScopes with the same
    // fragment share its AstVar.
    AstUser2Allocator<AstVar, std::map<std::pair<صحیح, صحیح>, AstVar*>> m_fragmentVarps;
    AstUser1Allocator<AstVarScope, VscpInfo> m_vscpInfos;
    std::سمتیہ<AstVarScope*> m_vscps;  // AstVarScopes of eligible variables, in encounter order
    Ast_گرہ_تفویض* m_assignp = خالی_اشاریہ;  // The enclosing assignment, if it can be expanded
    V3SharedTmps m_tmps{"__VbitblastTmp", VVarType::MODULETEMP};  // Temporaries of RHSs
    VDouble0 m_stat_بانٹو_Attr;  // AstVarScopes split only due to split_var
    VDouble0 m_stat_بانٹو_Auto;  // AstVarScopes split automatically
    VDouble0 m_statFragments;  // Fragment AstVarScopes created
    VDouble0 m_statTmps;  // Assignments to multiple fragments through a temporary

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ساکن خالی warn_No_بانٹو(AstVar* varp, ثابت Ast_گرہ* wherep, ثابت حرف* reasonp) {
        // Only warn if user requested splitting
        اگر (!varp->attr_بانٹو_Var()) واپس;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp);
        wherep->fileline()->modifyWarnOff(V3_Error_رمز::SPLITVAR, سچ);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties. Note this might be
    // overridden by a visit to an unsupported construct, so it is not completely determined
    // until the whole netlist is visited.
    ساکن منطقی isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        اگر (!varp->user1()) {
            ثابت منطقی eligible = [&]() {
                ثابت Ast_گرہ_DType* ثابت dtypep = varp->dtypep()->skipRefp();
                // Packed types only
                اگر (!dtypep->isIntegralOrPacked()) واپس جھوٹ;
                // Wider than one bit
                اگر (dtypep->چوڑائی() <= 1) واپس جھوٹ;
                // Check properties
                ثابت حرف* ثابت reasonp = varp->cannot_بانٹو_قسم_Reason();
                // Warn that it cannot be split if user explicitly requested splitting
                اگر (reasonp) warn_No_بانٹو(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                واپس !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        واپس varp->user1() & 1;
    }

    // The VscpInfo of the AstVarScope of an eligible variable
    VscpInfo& vscp_Info_کا(AstVarScope* vscp) {
        اگر (VscpInfo* ثابت infop = m_vscpInfos.tryGet(vscp)) واپس *infop;
        // Also record on first encounter
        m_vscps.ڈالو_عقب(vscp);
        واپس m_vscpInfos(vscp);
    }

    // Record step methods

    // Block splitting of the AstVarScope, due to the construct in 'wherep'
    ساکن خالی بلاک(AstVar* varp, VscpInfo& info, ثابت Ast_گرہ* wherep) {
        warn_No_بانٹو(varp, wherep, "it is referenced in an unsupported way");
        اگر (info.blocked) واپس;
        info.blocked = سچ;
        info.rdRefs.clear();
        info.wrRefs.clear();
    }

    // Record the reference 'exprp' of bits 'lsb' to 'msb' through 'refp'
    خالی ریکارڈ(Ast_گرہ_اظہاریہ* exprp, AstVarRef* refp, صحیح lsb, صحیح msb) {
        AstVar* ثابت varp = refp->varp();
        اگر (!isEligible(varp)) واپس;
        VscpInfo& info = vscp_Info_کا(refp->varScopep());
        // RW reference blocks splitting
        اگر (refp->access().isRW()) بلاک(varp, info, exprp);
        // Don't recod if blocked (due to RW above, or any other reason from elsewhere)
        اگر (info.blocked) واپس;
        // Record reference
        اگر (refp->access().isReadOnly()) {
            info.rdRefs.ڈالو_عقب({exprp, lsb, msb});
            واپس;
        }
        // A write as an assignment LHS can be expanded into assignments to multiple fragments
        ثابت منطقی canExpand = m_assignp && exprp == m_assignp->lhsp();
        info.wrRefs.ڈالو_عقب({exprp, canExpand ? m_assignp : خالی_اشاریہ, lsb, msb});
    }

    // Assignment visitor, shared by the assignment types that can be expanded
    خالی دورہ_Assignment(Ast_گرہ_تفویض* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, but always needs visiting
        m_assignp = nodep->timingControlp() ? خالی_اشاریہ : nodep;
        iterateChildrenConst(nodep);
    }

    // Decision step methods

    // The AstVar of the fragment of 'varp' of bits 'lsb' to 'msb', created on first use
    AstVar* fragmentVarp(AstVar* varp, صحیح lsb, صحیح msb) {
        AstVar*& fragVarpr = m_fragmentVarps(varp)[{lsb, msb}];
        اگر (fragVarpr) واپس fragVarpr;
        ثابت صحیح چوڑائی = msb - lsb + 1;
        std::string name = varp->name() + "__BRA__" + Ast_گرہ::encode_نمبر(msb);
        اگر (چوڑائی > 1) name += Ast_گرہ::encodeName(":") + Ast_گرہ::encode_نمبر(lsb);
        name += "__KET__";
        Ast_گرہ_DType* ثابت dtypep = varp->dtypep()->isFourstate()
                                         ? varp->ڈھونڈو_Logic_DType(چوڑائی, چوڑائی, VSigning::UNSIGNED)
                                         : varp->ڈھونڈو_ثنائی_ہندسہ_DType(چوڑائی, چوڑائی, VSigning::UNSIGNED);
        fragVarpr = نیا AstVar{varp->fileline(), varp->varType(), name, dtypep};
        fragVarpr->propagate_بانٹو_Attr_From(varp);
        varp->جوڑو_یہاں_This_As_اگلا(fragVarpr);
        واپس fragVarpr;
    }

    // Decide whether to split the AstVarScope, and if so, create its fragments
    خالی decide(AstVarScope* vscp, VscpInfo& info) {
        اگر (info.blocked) واپس;
        AstVar* ثابت varp = vscp->varp();
        // Split automatically if all references are disjoint or identical bit selects
        ثابت منطقی isAuto = [&]() {
            std::سمتیہ<std::pair<صحیح, صحیح>> ranges;  // [lsb, msb] inclusive
            برائے (ثابت RdRef& ref : info.rdRefs) {
                اگر (VN_IS(ref.exprp, VarRef)) واپس جھوٹ;
                ranges.emplace_عقب(ref.lsb, ref.msb);
            }
            برائے (ثابت WrRef& ref : info.wrRefs) {
                اگر (VN_IS(ref.exprp, VarRef)) واپس جھوٹ;
                ranges.emplace_عقب(ref.lsb, ref.msb);
            }
            std::ترتیب_دو(ranges.begin(), ranges.انتہا());
            برائے (سائز_t i = 0; i + 1 < ranges.سائز(); ++i) {
                ثابت std::pair<صحیح, صحیح>& a = ranges[i];
                ثابت std::pair<صحیح, صحیح>& b = ranges[i + 1];
                اگر (a != b && a.دوسرا >= b.پہلا) واپس جھوٹ;
            }
            واپس سچ;
        }();
        // Otherwise only if requested
        اگر (!isAuto && !varp->attr_بانٹو_Var()) واپس;
        // The fragment boundaries are the boundaries of the ranges written,
        // and of the range referenced, as bits outside of it are not used
        صحیح lsb = varp->چوڑائی();
        صحیح msb = -1;
        برائے (ثابت RdRef& ref : info.rdRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
        }
        std::سمتیہ<صحیح> bounds;
        برائے (ثابت WrRef& ref : info.wrRefs) {
            lsb = std::min(lsb, ref.lsb);
            msb = std::max(msb, ref.msb);
            bounds.ڈالو_عقب(ref.lsb);
            bounds.ڈالو_عقب(ref.msb + 1);
        }
        bounds.ڈالو_عقب(lsb);
        bounds.ڈالو_عقب(msb + 1);
        std::ترتیب_دو(bounds.begin(), bounds.انتہا());
        bounds.erase(std::unique(bounds.begin(), bounds.انتہا()), bounds.انتہا());
        // Nothing to do if the only fragment is the whole variable
        اگر (bounds.سائز() == 2 && lsb == 0 && msb == varp->چوڑائی() - 1) واپس;
        // Writes that cannot be expanded must write one fragment
        برائے (ثابت WrRef& ref : info.wrRefs) {
            // On the LHS of assignment it can be expanded
            اگر (ref.assignp) جاری;
            // If write is to a whole fragment, it can be replaced
            اگر (*std::بالائی_bound(bounds.begin(), bounds.انتہا(), ref.lsb) == ref.msb + 1) {
                جاری;
            }
            // Otherwise can't split
            warn_No_بانٹو(varp, ref.exprp, "it is written in an unsupported way");
            واپس;
        }

        // Splitting this variable
        ++(isAuto ? m_stat_بانٹو_Auto : m_stat_بانٹو_Attr);

        // Create the fragment AstVarScopes
        برائے (سائز_t i = 0; i + 1 < bounds.سائز(); ++i) {
            ثابت صحیح lsb = bounds[i];
            ثابت صحیح msb = bounds[i + 1] - 1;
            AstVar* ثابت fragVarp = fragmentVarp(varp, lsb, msb);
            AstVarScope* ثابت newp = نیا AstVarScope{vscp->fileline(), vscp->scopep(), fragVarp};
            vscp->جوڑو_یہاں_This_As_اگلا(newp);
            info.fragments.ڈالو_عقب({lsb, msb, newp});
            ++m_statFragments;
        }
    }

    // Rewrite step methods

    // Index of the fragment containing bit 'bit'
    ساکن سائز_t fragment_اشاریہ(ثابت VscpInfo& info, صحیح ثنائی_ہندسہ_2) {
        ثابت خودکار it = std::بالائی_bound(info.fragments.begin(), info.fragments.انتہا(), ثنائی_ہندسہ_2,
                                         [](صحیح b, ثابت Fragment& fragment) {  //
                                             واپس b <= fragment.msb;
                                         });
        واپس it - info.fragments.begin();
    }

    // Replace the read 'ref' with the bits of the fragments it overlaps
    خالی rewriteRead(ثابت RdRef& ref, ثابت VscpInfo& info) {
        File_سطر* ثابت flp = ref.exprp->fileline();
        Ast_گرہ_اظہاریہ* resultp = خالی_اشاریہ;
        برائے (سائز_t i = fragment_اشاریہ(info, ref.lsb); i < info.fragments.سائز(); ++i) {
            ثابت Fragment& fragment = info.fragments[i];
            اگر (fragment.lsb > ref.msb) توڑو;
            ثابت صحیح partLsb = std::max(ref.lsb, fragment.lsb);
            ثابت صحیح partMsb = std::min(ref.msb, fragment.msb);
            Ast_گرہ_اظہاریہ* bitsp = نیا AstVarRef{flp, fragment.vscp, VAccess::READ};
            اگر (partLsb != fragment.lsb || partMsb != fragment.msb) {
                bitsp = نیا AstSel{flp, bitsp, partLsb - fragment.lsb, partMsb - partLsb + 1};
            }
            // Higher bits go to the left
            resultp = resultp ? نیا AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ref.exprp->replaceWith(resultp);
        VL_DO_DANGLING(ڈالو_Deletep(ref.exprp), ref.exprp);
    }

    // Can the bits of the RHS of an assignment to multiple fragments be selected directly for
    // each fragment. It must be pure and cheap, and must not read a fragment being written, which
    // it cannot, as it is wider than one fragment, and its reads were rewritten into fragments.
    ساکن منطقی isSelectable(ثابت Ast_گرہ_اظہاریہ* nodep) {
        اگر (VN_IS(nodep, Const)) واپس سچ;
        اگر (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            اگر (!VN_IS(selp->lsbp(), Const)) واپس جھوٹ;
            nodep = selp->fromp();
        }
        ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef);
        اگر (!refp) واپس جھوٹ;
        // Not a forced variable (trips several bugs in V3Force)
        اگر (refp->varp()->isForced()) واپس جھوٹ;
        // Not a SystemC variable, which can only be accessed whole, not selected from.
        واپس !refp->varp()->isSc();
    }

    // Replace the write 'ref' with the fragments it covers
    خالی rewriteWrite(AstScope* scopep, ثابت WrRef& ref, ثابت VscpInfo& info) {
        File_سطر* ثابت flp = ref.exprp->fileline();
        ثابت سائز_t پہلا = fragment_اشاریہ(info, ref.lsb);
        ثابت سائز_t آخری = fragment_اشاریہ(info, ref.msb);
        UASSERT_OBJ(info.fragments[پہلا].lsb == ref.lsb, ref.exprp,
                    "Write not at fragment boundary");
        // If one fragment, write it instead
        اگر (پہلا == آخری) {
            ref.exprp->replaceWith(نیا AstVarRef{flp, info.fragments[پہلا].vscp, VAccess::WRITE});
            VL_DO_DANGLING(ڈالو_Deletep(ref.exprp), ref.exprp);
            واپس;
        }
        // Otherwise it must be an assignment LHS: expand the assignment into
        // an assignment to each fragment
        Ast_گرہ_تفویض* ثابت origp = ref.assignp;
        Ast_گرہ_اظہاریہ* rhsp = origp->rhsp()->unlink_Fr_عقب();
        // Unless its bits can be selected directly, assign the RHS to a temporary first, so it
        // is evaluated once, and reads the fragments before any of them is written
        اگر (!isSelectable(rhsp)) {
            ++m_statTmps;
            AstVarScope* ثابت tmpp = m_tmps.make(flp, scopep, rhsp->dtypep());
            AstVarRef* ثابت tmpRefp = نیا AstVarRef{flp, tmpp, VAccess::WRITE};
            origp->جوڑو_یہاں_This_As_اگلا(نیا Ast_تفویض{flp, tmpRefp, rhsp});
            rhsp = نیا AstVarRef{flp, tmpp, VAccess::READ};
        }
        // Assign each fragment its bits of the RHS
        برائے (سائز_t i = پہلا; i <= آخری; ++i) {
            ثابت Fragment& fragment = info.fragments[i];
            ثابت صحیح lsb = fragment.lsb - ref.lsb;  // LSB of the fragment's bits in the RHS
            ثابت صحیح چوڑائی = fragment.msb - fragment.lsb + 1;
            AstVarRef* ثابت lhsp = نیا AstVarRef{flp, fragment.vscp, VAccess::WRITE};
            AstSel* ثابت selp = نیا AstSel{flp, rhsp->clone_درخت_خالص(جھوٹ), lsb, چوڑائی};
            origp->جوڑو_یہاں_This_As_اگلا(origp->cloneType(lhsp, selp));
        }
        VL_DO_DANGLING(ڈالو_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ڈالو_Deletep(origp->unlink_Fr_عقب()), origp);
    }

    // The combinational AstActive of the scope, cached
    ساکن Ast_فعال* combo_فعال(AstScope* scopep) {
        اگر (!scopep->user2p()) scopep->user2p(scopep->comboActivep(سچ));
        واپس VN_AS(scopep->user2p(), فعال);
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_DType*) override {}  // No references in data types
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }
    خالی دورہ(Ast_تفویض* nodep) override { دورہ_Assignment(nodep); }
    خالی دورہ(Ast_تفویض_W* nodep) override { دورہ_Assignment(nodep); }
    خالی دورہ(Ast_تفویض_Dly* nodep) override { دورہ_Assignment(nodep); }
    خالی دورہ(AstSel* nodep) override {
        AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef);
        اگر (!refp || !isEligible(refp->varp())) {
            iterateChildrenConst(nodep);
            واپس;
        }
        // Must be constant and in range. V3Unknown only guards the LSB, so a select with
        // the LSB in range, but the MSB beyond the variable can reach here.
        ثابت AstConst* ثابت lsbp = VN_CAST(nodep->lsbp(), Const);
        اگر (!lsbp || lsbp->مقصد_SInt() + nodep->چوڑائی_Const() > refp->varp()->چوڑائی()) {
            بلاک(refp->varp(), vscp_Info_کا(refp->varScopep()), refp);
            iterateConst(nodep->lsbp());
            واپس;
        }
        // Record partial access
        ریکارڈ(nodep, refp, lsbp->مقصد_SInt(), lsbp->مقصد_SInt() + nodep->چوڑائی_Const() - 1);
    }
    خالی دورہ(AstVarRef* nodep) override {
        // Record whole access
        ریکارڈ(nodep, nodep, 0, nodep->varp()->چوڑائی() - 1);
    }
    خالی دورہ(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        // The AstVarScope is not known, so all of them are blocked, by making it ineligible
        AstVar* ثابت varp = nodep->varp();
        اگر (!isEligible(varp)) واپس;
        varp->user1(2);  // Mark not eligible
        warn_No_بانٹو(varp, nodep, "it is referenced in an unsupported way");
    }

    // CONSTRUCTORS
    صریح ثنائی_ہندسہ_Blast_Visitor(AstNetlist* netlistp) {
        // Record the references
        iterateConst(netlistp);

        // Decide which AstVarScopes are split, unless the variable became ineligible
        برائے (AstVarScope* ثابت vscp : m_vscps) {
            اگر (isEligible(vscp->varp())) decide(vscp, m_vscpInfos(vscp));
        }

        // Rewrite the reads, then the writes, which might move the reads in their RHS
        برائے (AstVarScope* ثابت vscp : m_vscps) {
            ثابت VscpInfo& info = m_vscpInfos(vscp);
            اگر (info.fragments.خالی_خانے()) جاری;
            برائے (ثابت RdRef& ref : info.rdRefs) rewriteRead(ref, info);
        }
        برائے (AstVarScope* ثابت vscp : m_vscps) {
            ثابت VscpInfo& info = m_vscpInfos(vscp);
            اگر (info.fragments.خالی_خانے()) جاری;
            برائے (ثابت WrRef& ref : info.wrRefs) rewriteWrite(vscp->scopep(), ref, info);
        }

        // Drive variables that must be kept from their fragments
        برائے (AstVarScope* ثابت vscp : m_vscps) {
            ثابت VscpInfo& info = m_vscpInfos(vscp);
            اگر (info.fragments.خالی_خانے()) جاری;
            اگر (!(vscp->varp()->isTrace() && vscp->isTrace())) جاری;
            Ast_فعال* ثابت activep = combo_فعال(vscp->scopep());
            File_سطر* ثابت flp = vscp->fileline();
            برائے (ثابت Fragment& fragment : info.fragments) {
                AstVarRef* ثابت lhsRefp = نیا AstVarRef{flp, vscp, VAccess::WRITE};
                ثابت صحیح چوڑائی = fragment.msb - fragment.lsb + 1;
                AstSel* ثابت lhsp = نیا AstSel{flp, lhsRefp, fragment.lsb, چوڑائی};
                AstVarRef* ثابت rhsp = نیا AstVarRef{flp, fragment.vscp, VAccess::READ};
                activep->جوڑو_Stmtsp(نیا AstAlways{نیا Ast_تفویض_W{flp, lhsp, rhsp}});
            }
        }

        V3Stats::جوڑو_Stat("Optimizations, Bitblast, variables split due to attribute",
                         m_stat_بانٹو_Attr);
        V3Stats::جوڑو_Stat("Optimizations, Bitblast, variables split automatically",
                         m_stat_بانٹو_Auto);
        V3Stats::جوڑو_Stat("Optimizations, Bitblast, fragments created", m_statFragments);
        V3Stats::جوڑو_Stat("Optimizations, Bitblast, assignments through temporary", m_statTmps);
    }

عوامی:
    ساکن خالی apply(AstNetlist* netlistp) { ثنائی_ہندسہ_Blast_Visitor{netlistp}; }
};

//######################################################################
// V3BitBlast class functions

خالی V3_ثنائی_ہندسہ_Blast::ثنائی_ہندسہ_Blast_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    ثنائی_ہندسہ_Blast_Visitor::apply(nodep);
    V3Global::dump_Check_Global_درخت("bitblast", 0, dump_درخت_Either_Level() >= 3);
}
