// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Convert forceable signals, process force/release
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
//  V3Force's Transformations:
//
//  For each forceable var/net "<name>":
//    - Create <name>__VforceVec (VlForceVec) to track active force ranges
//    - Create <name>__VforceRHS<ID> vars to hold RHS shadow values
//    - Add continuous assignments: <name>__VforceRHS<ID> = RHS
//
//  For each `force <name>[range] = <RHS>` with ID:
//    - <name>__VforceVec.addForce(lsb, msb, &__VforceRHS, rhsLsb)
//
//  For each `release <name>[range]`:
//    - If not continuously driven: <name> = VlForceVec::read(<name>, __VforceVec)
//    - <name>__VforceVec.release(lsb, msb)
//
//  For each read of <name>:
//    - Replace with: VlForceVec::read(<name>, __VforceVec)
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Force.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3Stats.h"
#ضمن "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

صنف ForceState final {
عام:
    بنية ForceRange VL_NOT_FINAL {
        صحيح m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        صحيح m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        صحيح m_padLsb = 0;  // Bit positions for RHS padding
        صحيح m_padMsb = 0;  // Bit positions for RHS padding
    };

    بنية ForceInfo final : ForceRange {
        // MEMBERS
        صحيح m_forceId = 0;  // Unique (per signal) variable of this force assignment
        منطقي m_hasArraySel = خطأ;  // If this has an array select on LHS
        منطقي m_isExternal = خطأ;  // Synthetic force for the public force controls
        منطقي m_skipRdRefresh = خطأ;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = تش_اشاره;  // Scope of the var containing RHSID
        Ast_غوټه_بیان* m_rhsExprp = تش_اشاره;  // Expression on RHS of this force assignment

        ForceInfo() = افتراضي;
        ForceInfo(صحيح rangeLsb, صحيح rangeMsb, صحيح padLsb, صحيح padMsb, صحيح forceId,
                  منطقي hasArraySel, AstVarScope* rhsVarVscp, Ast_غوټه_بیان* rhsExprp)
            : m_forceId{forceId}
            , m_hasArraySel{hasArraySel}
            , m_rhsVarVscp{rhsVarVscp}
            , m_rhsExprp{rhsExprp} {
            m_rangeLsb = rangeLsb;
            m_rangeMsb = rangeMsb;
            m_padLsb = padLsb;
            m_padMsb = padMsb;
        }
    };

    بنية VarForceInfo final {
        AstVarScope* m_forceVecVscp = تش_اشاره;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = تش_اشاره;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = تش_اشاره;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = تش_اشاره;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = تش_اشاره;  // Scope of the forced (original) variable
        AstVar* m_varp = تش_اشاره;  // The forced (original) variable
        AstScope* m_scopep = تش_اشاره;  // Scope containing the forced variable
        std::unordered_map<Ast_ټاکل_Force*, ForceInfo> m_forces;
        std::unordered_map<string, صحيح> m_force_لار_منزل_شاخص;
        صحيح m_بل_Force_لار_شاخص = 1;  // Start at 1 so 0 can be the base path (whole signal)

    خاص:
        ساكن فراغ build_Force_لار_کلی_Recurse(Ast_غوټه_بیان* nodep, string& بهر) {
            إذا (VN_IS(nodep, VarRef)) أرجع;
            إذا (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
                build_Force_لار_کلی_Recurse(selp->fromp(), بهر);
                بهر += "|S" + cvt_منزل_Str(selp->lsbConst()) + ":" + cvt_منزل_Str(selp->پلنوالی_Const());
                أرجع;
            }
            إذا (AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
                Ast_غوټه_بیان* ثابت fromp = selp->fromp();
                build_Force_لار_کلی_Recurse(fromp, بهر);
                بهر += "|T" + selp->name();
                أرجع;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        ساكن string force_لار_کلی(Ast_غوټه_بیان* nodep) {
            string بهر;
            build_Force_لار_کلی_Recurse(nodep, بهر);
            أرجع بهر;
        }

    عام:
        صحيح get_Or_Create_Force_لار_شاخص(Ast_غوټه_بیان* nodep) {
            ثابت خپلکاری pair
                = m_force_لار_منزل_شاخص.emplace(force_لار_کلی(nodep), m_بل_Force_لار_شاخص);
            إذا (pair.دوهم) ++m_بل_Force_لار_شاخص;
            أرجع pair.لومړی->دوهم;
        }

        صحيح پیدا_کړه_Force_لار_شاخص(Ast_غوټه_بیان* nodep) ثابت {
            ثابت خپلکاری it = m_force_لار_منزل_شاخص.پیدا_کړه(force_لار_کلی(nodep));
            أرجع it != m_force_لار_منزل_شاخص.پای_ټکی() ? it->دوهم : -1;
        }
    };

    بنية ForceHelperVars final {
        AstVar* m_rdVarp = تش_اشاره;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = تش_اشاره;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = تش_اشاره;  // The __VforceVal (force value) variable
    };

    بنية ArraySelInfo final {
        std::ویکټور<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        منطقي m_has_بټ_Sel = خطأ;  // Bit-select was found along the path too
    };

    بنية ForceRangeInfo final : ForceRange {
        منطقي m_hasArraySel = خطأ;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

خاص:
    کارول ScopeVarCache = std::unordered_map<ثابت AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    ثابت VNUser1_دننه_Use m_user1_دننه_Use;
    ثابت VNUser2_دننه_Use m_user2_دننه_Use;

عام:
    کارول ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    کارول Protected_Var_Ref_سېټ = std::unordered_سېټ<ثابت AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    کارول Built_Var_Scope_سېټ = std::unordered_سېټ<ثابت AstVarScope*>;

خاص:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::ویکټور<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, صحيح> m_var_منزل_Id;
    std::unordered_سېټ<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::ویکټور<ForceInfo*>> m_rhs_Dep_منزل_Forces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    منطقي m_doing_ټاکل = خطأ;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    Protected_Var_Ref_سېټ& m_permanentlyProtected;
    Built_Var_Scope_سېټ& m_forceRdUpdateBuilt;

عام:
    ForceState(منطقي doing_ټاکل, ForceHelperVarsByVar& forceHelperVarsByVar,
               Protected_Var_Ref_سېټ& permanentlyProtected, Built_Var_Scope_سېټ& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doing_ټاکل{doing_ټاکل}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    ساكن AstConst* makeZeroConst(Ast_غوټه* nodep, صحيح پلنوالی) {
        V3_شمېره zero{nodep, پلنوالی};
        zero.سېټ_All_Bits0();
        أرجع جديد AstConst{nodep->fileline(), zero};
    }

    ساكن AstConst* makeConst32(File_کرښه* flp, صحيح ارزښت) {
        أرجع جديد AstConst{flp, AstConst::Widthed_ارزښت{}, 32, static_cast<uint32_t>(ارزښت)};
    }

    ساكن AstConst* make_Range_ماسک_Const(Ast_غوټه* nodep, صحيح پلنوالی, صحيح lsb, صحيح msb) {
        V3_شمېره ماسک{nodep, پلنوالی};
        ماسک.سېټ_All_Bits0();
        لكل (صحيح بټ_2 = lsb; بټ_2 <= msb; ++بټ_2) ماسک.سېټ_بټ(بټ_2, 1);
        أرجع جديد AstConst{nodep->fileline(), ماسک};
    }

    ساكن Ast_غوټه_بیان* zero_Pad_منزل_بنسټ_پلنوالی(Ast_غوټه_بیان* exprp, صحيح بنسټ_پلنوالی, صحيح padLsb,
                                           صحيح padMsb) {
        إذا (بنسټ_پلنوالی <= 0) أرجع exprp;
        ثابت صحيح ټیټ_Pad = padLsb;
        ثابت صحيح لوړ_Pad = بنسټ_پلنوالی - (padMsb + 1);
        إذا (ټیټ_Pad > 0) {
            exprp = جديد AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, ټیټ_Pad)};
        }
        إذا (لوړ_Pad > 0) {
            exprp = جديد AstConcat{exprp->fileline(), makeZeroConst(exprp, لوړ_Pad), exprp};
        }
        أرجع exprp;
    }

    ساكن منطقي isUnpackedArrayDType(ثابت Ast_غوټه_DType* dtypep) {
        أرجع VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    ساكن منطقي isBitwiseDType(ثابت Ast_غوټه* nodep) {
        ثابت AstBasicDType* ثابت basicp = nodep->dtypep()->skipRefp()->basicp();
        أرجع basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    ساكن Ast_غوټه_بیان* cast_منزل_غوټه_DType(Ast_غوټه_بیان* exprp, Ast_غوټه* dtypeFromp) {
        ثابت Ast_غوټه_DType* ثابت dtypep = dtypeFromp->dtypep()->skipRefp();
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();
        إذا (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            أرجع exprp;
        }
        أرجع جديد AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    ساكن فراغ markNonReplaceable(AstVarRef* ثابت nodep) { nodep->user1_سېټ_Once(); }
    منطقي isNotReplaceable(ثابت AstVarRef* ثابت nodep) ثابت {
        أرجع nodep->user1() || m_permanentlyProtected.شمېر(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    فراغ markPermanentlyProtected(AstVarRef* ثابت nodep) ثابت {
        markNonReplaceable(nodep);
        m_permanentlyProtected.ورننباسه(nodep);
    }

    ساكن std::ویکټور<ForceInfo*> force_Infos_دننه_Id_ترتیب(VarForceInfo& info) {
        std::ویکټور<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.اندازه());
        لكل (خپلکاری& it : info.m_forces) forceps.ټېل_شا(&it.دوهم);
        std::ترتیبول(forceps.begin(), forceps.پای_ټکی(), [](ثابت ForceInfo* ap, ثابت ForceInfo* bp) {
            أرجع ap->m_forceId < bp->m_forceId;
        });
        أرجع forceps;
    }

    ساكن std::ویکټور<ثابت ForceInfo*> force_Infos_دننه_Id_ترتیب(ثابت VarForceInfo& info) {
        std::ویکټور<ثابت ForceInfo*> forceps;
        forceps.reserve(info.m_forces.اندازه());
        لكل (ثابت خپلکاری& it : info.m_forces) forceps.ټېل_شا(&it.دوهم);
        std::ترتیبول(forceps.begin(), forceps.پای_ټکی(), [](ثابت ForceInfo* ap, ثابت ForceInfo* bp) {
            أرجع ap->m_forceId < bp->m_forceId;
        });
        أرجع forceps;
    }

    ساكن منطقي is_Opaque_لار_Selector(ثابت Ast_غوټه* nodep) {
        أرجع VN_IS(nodep, Sel) || VN_IS(nodep, غوټه_Sel) || VN_IS(nodep, StructSel);
    }

    ساكن منطقي is_Outermost_Opaque_لار_Selector(ثابت Ast_غوټه* nodep) {
        إذا (!is_Opaque_لار_Selector(nodep)) أرجع خطأ;
        ثابت Ast_غوټه* ثابت backp = nodep->backp();
        أرجع !backp || !is_Opaque_لار_Selector(backp);
    }

    ساكن AstVarRef* getOneVarRef(Ast_غوټه_بیان* forceStmtp) {
        Ast_غوټه* ثابت basep = forceStmtp->بنسټ_Fromp(ریښتیا);
        إذا (AstSampled* sampledp = VN_CAST(basep, Sampled))
            إذا (Ast_غوټه_بیان* exprp = VN_CAST(sampledp->exprp(), غوټه_بیان))
                أرجع getOneVarRef(exprp);
        AstVarRef* ثابت varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        أرجع varRefp;
    }

    ساكن Ast_غوټه_بیان* buildNestedArraySel(File_کرښه* flp, Ast_غوټه_بیان* fromp,
                                            ثابت std::ویکټور<صحيح>& indicies) {
        Ast_غوټه_بیان* curp = fromp;
        لكل (ثابت صحيح idx : indicies) curp = جديد AstArraySel{flp, curp, idx};
        أرجع curp;
    }

    کينډۍ <ډول_نوم Fn>
    ساكن Ast_غوټه_Stmt* foreach_Unpacked_ونې_پاڼه(ثابت std::ویکټور<AstUnpackArrayDType*>& dims,
                                            Fn build_ونې_پاڼه) {
        Ast_غوټه_Stmt* headp = تش_اشاره;
        Ast_غوټه_Stmt* tailp = تش_اشاره;
        إذا (dims.تشه()) أرجع تش_اشاره;
        صحيح ټول = 1;
        لكل (ثابت AstUnpackArrayDType* ثابت d : dims) ټول *= d->elementsConst();
        إذا (ټول <= 0) أرجع تش_اشاره;
        std::ویکټور<صحيح> idx(dims.اندازه(), 0);
        لكل (صحيح flat = 0; flat < ټول; ++flat) {
            Ast_غوټه_Stmt* ثابت stmtp = build_ونې_پاڼه(idx, flat);
            إذا (!headp) {
                headp = stmtp;
            } وإلا {
                tailp->جمع_کړه_بل(stmtp);
            }
            tailp = stmtp;
            لكل (صحيح d = static_cast<صحيح>(dims.اندازه()) - 1; d >= 0; --d) {
                إذا (++idx[d] < dims[d]->elementsConst()) اكسر;
                idx[d] = 0;
            }
        }
        أرجع headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_غوټه_بیان* lhsp, AstVar* varp,
                                     منطقي requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.تشه();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->پلنوالی() - 1) : 0;

        إذا (ثابت AstSel* ثابت outerSelp = VN_CAST(lhsp, Sel)) {
            صحيح ټول_Lsb = 0;
            لكل (Ast_غوټه_بیان* curp = lhsp; ثابت AstSel* ثابت selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                إذا (requireConstRangeSelect) {
                    UASSERT_OBJ(VN_IS(selp->lsbp(), Const), lhsp,
                                "Unsupported: force on non-const range select");
                }
                ټول_Lsb += selp->lsbConst();
            }
            info.m_padLsb = ټول_Lsb;
            info.m_padMsb = ټول_Lsb + outerSelp->پلنوالی_Const() - 1;
        }

        info.m_rangeLsb = info.m_padLsb;
        info.m_rangeMsb = info.m_padMsb;
        إذا (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::ویکټور<صحيح> indices;
            indices.reserve(info.m_arrayInfo.m_sels.اندازه());
            لكل (AstArraySel* ثابت selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.ټېل_شا(VN_AS(selp->bitp(), Const)->منزل_SInt());
            }
            info.m_rangeLsb = flatten_شاخص(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } وإلا إذا (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        إذا (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            ثابت صحيح شاخص = varInfo.get_Or_Create_Force_لار_شاخص(lhsp);
            info.m_rangeLsb = شاخص;
            info.m_rangeMsb = شاخص;
        }
        أرجع info;
    }

    Ast_غوټه_بیان* جمع_کړه_Rhs_ارزښت_Reads(ثابت VarForceInfo& varInfo, Ast_غوټه_بیان* exprp) ثابت {
        إذا (!doing_ټاکل()) أرجع exprp;

        ثابت std::ویکټور<ثابت ForceInfo*> forceps = force_Infos_دننه_Id_ترتیب(varInfo);
        إذا (forceps.تشه()) أرجع exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* ثابت cexprp = جديد AstCExpr{exprp->fileline(), AstCExpr::خالص{}};
        cexprp->dtypeFrom(exprp);
        cexprp->جمع_کړه("(");
        لكل (ثابت ForceInfo* ثابت finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* ثابت refp
                = جديد AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->جمع_کړه("(void)(");
            cexprp->جمع_کړه(refp);
            cexprp->جمع_کړه("), ");
        }
        cexprp->جمع_کړه(exprp);
        cexprp->جمع_کړه(")");
        أرجع cexprp;
    }

    Ast_غوټه_بیان* createForceReadCall(ثابت VarForceInfo& varInfo, File_کرښه* flp, VCMethod لاره,
                                     Ast_غوټه_بیان* originalExprp, Ast_غوټه* dtypeFromp,
                                     Ast_غوټه_بیان* شاخص_Exprp) ثابت {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_غوټه_بیان* ثابت origValp
            = جمع_کړه_Rhs_ارزښت_Reads(varInfo, cast_منزل_غوټه_DType(originalExprp, dtypeFromp));

        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, لاره, origValp};
        إذا (شاخص_Exprp) callp->جمع_کړه_Pinsp(شاخص_Exprp);
        callp->dtypeFrom(dtypeFromp);
        أرجع callp;
    }

    Ast_غوټه_بیان* create_Force_Rd_افاده(ثابت VarForceInfo& varInfo, File_کرښه* flp) ثابت {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        إذا (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            إذا (doing_ټاکل()) { أرجع جديد AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3_Error_کوډ::UNOPTFLAT, ریښتیا);
            AstVarRef* ثابت بنسټ_Refp = جديد AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(بنسټ_Refp);
            أرجع createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, بنسټ_Refp, varp,
                                       تش_اشاره);
        }
        AstVarRef* ثابت بنسټ_Refp = جديد AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(بنسټ_Refp);
        Ast_غوټه_بیان* ثابت enRefp = جديد AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_غوټه_بیان* ثابت valRefp = جديد AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        إذا (isBitwiseDType(varp)) {
            أرجع جديد AstOr{
                flp, جديد AstAnd{flp, enRefp, valRefp},
                جديد AstAnd{flp, جديد AstNot{flp, enRefp->clone_ونه_خالص(خطأ)}, بنسټ_Refp}};
        }
        أرجع جديد AstCond{flp, enRefp, valRefp, بنسټ_Refp};
    }

    Ast_غوټه_Stmt* createForceRdUpdateStmt(ثابت VarForceInfo& varInfo) ثابت {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        إذا (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            أرجع createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_کرښه* ثابت flp = varInfo.m_varVscp->fileline();
        أرجع جديد Ast_ټاکل{flp, جديد AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             create_Force_Rd_افاده(varInfo, flp)};
    }

    Ast_غوټه_Stmt* createForceRdUpdateStmtUnpacked(ثابت VarForceInfo& varInfo) ثابت {
        File_کرښه* ثابت flp = varInfo.m_varVscp->fileline();
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* ثابت arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت std::ویکټور<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_غوټه_Stmt* ثابت stmtsp = foreach_Unpacked_ونې_پاڼه(
            dims, [&](ثابت std::ویکټور<صحيح>& idx, صحيح /*flat*/) -> Ast_غوټه_Stmt* {
                AstVarRef* ثابت بنسټ_Refp = جديد AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(بنسټ_Refp);
                Ast_غوټه_بیان* ثابت بنسټ_Selp = buildNestedArraySel(flp, بنسټ_Refp, idx);
                Ast_غوټه_بیان* ثابت enSelp = buildNestedArraySel(
                    flp, جديد AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_غوټه_بیان* ثابت valSelp = buildNestedArraySel(
                    flp, جديد AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_غوټه_بیان* ثابت readExprp = جديد AstOr{
                    flp, جديد AstAnd{flp, enSelp, valSelp},
                    جديد AstAnd{flp, جديد AstNot{flp, enSelp->clone_ونه_خالص(خطأ)}, بنسټ_Selp}};
                Ast_غوټه_بیان* ثابت rdLhsSelp = buildNestedArraySel(
                    flp, جديد AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                أرجع جديد Ast_ټاکل{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        لكل (ثابت ForceInfo* ثابت finfop : force_Infos_دننه_Id_ترتیب(varInfo)) {
            إذا (!finfop->m_isExternal) استمر;
            stmtsp->جمع_کړه_بل(جديد Ast_ټاکل{flp,
                                          جديد AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_ونه_خالص(خطأ)});
        }
        أرجع stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        ثابت خپلکاری it = m_var_منزل_Id.پیدا_کړه(vscp);
        إذا (it != m_var_منزل_Id.پای_ټکی()) أرجع m_varInfos[it->دوهم];

        m_var_منزل_Id.emplace(vscp, m_varInfos.اندازه());
        m_varInfos.emplace_شا();
        VarForceInfo& info = m_varInfos.شا();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* ثابت varp = info.m_varp;
        إذا (!varp->isForceable()) أرجع info;

        File_کرښه* ثابت flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        ثابت منطقي helperVarsBuilt = helperVars.m_rdVarp != تش_اشاره;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != تش_اشاره)
                        && helperVarsBuilt == (helperVars.m_valVarp != تش_اشاره),
                    varp, "Incomplete force helper set");
        إذا (!helperVarsBuilt) {
            ثابت منطقي unpacked = isUnpackedArrayDType(varp->dtypep());
            ثابت VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_غوټه_DType* ثابت enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->پیدا_کړه_بټ_DType();
            helperVars.m_rdVarp
                = جديد AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sig_عامه(ریښتیا);
            helperVars.m_enVarp
                = جديد AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(ریښتیا);
            helperVars.m_valVarp
                = جديد AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(ریښتیا);
            varp->جمع_کړه_بل_دلته(helperVars.m_rdVarp);
            varp->جمع_کړه_بل_دلته(helperVars.m_enVarp);
            varp->جمع_کړه_بل_دلته(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = پیدا_کړه_Scope_Var(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = پیدا_کړه_Scope_Var(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = پیدا_کړه_Scope_Var(info.m_scopep, helperVars.m_valVarp);
        إذا (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } وإلا {
            info.m_forceRdVscp = جديد AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = جديد AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = جديد AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->جمع_کړه_Varsp(info.m_forceRdVscp);
            info.m_scopep->جمع_کړه_Varsp(info.m_forceEnVscp);
            info.m_scopep->جمع_کړه_Varsp(info.m_forceValVscp);
        }
        أرجع info;
    }

    فراغ markClockedWrite(AstVar* varp) { m_clockedWrites.ورننباسه(varp); }
    منطقي hasClockedWrite(AstVar* varp) ثابت { أرجع m_clockedWrites.شمېر(varp); }

    منطقي doing_ټاکل() ثابت { أرجع m_doing_ټاکل; }

    ثابت VarForceInfo* getVarInfo(AstVarScope* vscp) ثابت {
        ثابت خپلکاری it = m_var_منزل_Id.پیدا_کړه(vscp);
        أرجع it != m_var_منزل_Id.پای_ټکی() ? &m_varInfos[it->دوهم] : تش_اشاره;
    }

    AstVarScope* پیدا_کړه_Scope_Var(AstScope* scopep, ثابت AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        إذا (cache.تشه()) {
            لكل (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        ثابت خپلکاری it = cache.پیدا_کړه(varp);
        أرجع it != cache.پای_ټکی() ? it->دوهم : تش_اشاره;
    }
    فراغ جمع_کړه_Force_Assignment(AstVar* varp, AstVarScope* vscp, Ast_غوټه_بیان* rhsExprp,
                            Ast_ټاکل_Force* forceStmtp, صحيح rangeLsb, صحيح rangeMsb, صحيح padLsb,
                            صحيح padMsb, منطقي hasArraySel, منطقي skipRdRefresh = خطأ) {
        v3Global.سېټ_کاروي_Force();
        varp->سېټ_Forced_By_کوډ();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        ثابت صحيح forceId = info.m_forces.اندازه();
        File_کرښه* ثابت flp = varp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        إذا (!info.m_forceVecVscp) {
            AstCDType* ثابت forceVecDtypep = جديد AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(forceVecDtypep);

            AstVar* ثابت forceVecVarp
                = جديد AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doing_ټاکل ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(خطأ);
            forceVecVarp->is_دننی(ریښتیا);
            varp->جمع_کړه_بل_دلته(forceVecVarp);
            info.m_forceVecVscp = جديد AstVarScope{flp, scopep, forceVecVarp};
            scopep->جمع_کړه_Varsp(info.m_forceVecVscp);
        }

        خپلکاری pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, تش_اشاره, rhsExprp});
        ForceInfo& finfo = pair.لومړی->دوهم;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        إذا (doing_ټاکل()) {
            std::ویکټور<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* ثابت refp) {
                إذا (!refp->access().isReadOnly()) أرجع;
                AstVar* ثابت depVarp = refp->varp();
                إذا (depVarp
                    && std::پیدا_کړه(depVarps.begin(), depVarps.پای_ټکی(), depVarp) == depVarps.پای_ټکی()) {
                    depVarps.ټېل_شا(depVarp);
                }
            });
            لكل (AstVar* ثابت depVarp : depVarps) m_rhs_Dep_منزل_Forces[depVarp].ټېل_شا(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    ساكن فراغ collectArraySelInfo(Ast_غوټه_بیان* exprp, ArraySelInfo& info) {
        إذا (ثابت خپلکاری* ثابت selp = VN_CAST(exprp, Sel)) {
            info.m_has_بټ_Sel = ریښتیا;
            collectArraySelInfo(selp->fromp(), info);
        } وإلا إذا (خپلکاری* ثابت selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.ټېل_شا(selp);
        } وإلا إذا (ثابت خپلکاری* ثابت memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    ساكن ArraySelInfo getArraySelInfo(Ast_غوټه_بیان* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        أرجع info;
    }

    ساكن std::ویکټور<صحيح> arraySelDimSizes(ثابت ArraySelInfo& info) {
        std::ویکټور<صحيح> dims;
        dims.reserve(info.m_sels.اندازه());
        لكل (AstArraySel* ثابت selp : info.m_sels) {
            Ast_غوټه_DType* ثابت dtypep = selp->fromp()->dtypep()->skipRefp();
            ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.ټېل_شا(arrayp->declRange().elements());
        }
        أرجع dims;
    }

    ساكن صحيح flatten_شاخص(ثابت std::ویکټور<صحيح>& indices, ثابت std::ویکټور<صحيح>& dimSizes) {
        UASSERT(indices.اندازه() == dimSizes.اندازه(), "Array index and dimension size mismatch");
        صحيح شاخص = 0;
        صحيح stride = 1;
        لكل (صحيح i = static_cast<صحيح>(indices.اندازه()) - 1; i >= 0; --i) {
            شاخص += indices[i] * stride;
            stride *= dimSizes[i];
        }
        أرجع شاخص;
    }

    ساكن Ast_غوټه_بیان* build_Flatten_شاخص_بیان(File_کرښه* flp, ثابت ArraySelInfo& info) {
        ثابت std::ویکټور<صحيح> dimSizes = arraySelDimSizes(info);
        منطقي allConst = ریښتیا;
        لكل (AstArraySel* ثابت selp : info.m_sels) {
            إذا (!VN_IS(selp->bitp(), Const)) {
                allConst = خطأ;
                اكسر;
            }
        }
        إذا (allConst) {
            std::ویکټور<صحيح> constIndices;
            constIndices.reserve(info.m_sels.اندازه());
            لكل (AstArraySel* ثابت selp : info.m_sels) {
                constIndices.ټېل_شا(VN_AS(selp->bitp(), Const)->منزل_SInt());
            }
            أرجع makeConst32(flp, flatten_شاخص(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_غوټه_بیان* resultp = تش_اشاره;
        صحيح stride = 1;
        لكل (صحيح i = static_cast<صحيح>(info.m_sels.اندازه()) - 1; i >= 0; --i) {
            Ast_غوټه_بیان* termp = info.m_sels[i]->bitp()->clone_ونه_خالص(خطأ);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            إذا (termp->پلنوالی() < 32) termp = جديد AstExtend{flp, termp, 32};
            إذا (stride != 1) termp = جديد AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? جديد Ast_جمع_کړه{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        أرجع resultp;
    }

    ساكن Ast_غوټه_بیان* build_Rhs_معلومات_بیان(File_کرښه* flp, ثابت ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        أرجع جديد AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    فراغ finalizeRhsVars() {
        لكل (VarForceInfo& info : m_varInfos) {
            AstVar* ثابت varp = info.m_varp;
            إذا (info.m_forces.تشه()) استمر;

            AstScope* ثابت scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_کرښه* ثابت flp = varp->fileline();
            ثابت std::ویکټور<ForceInfo*> forceps = force_Infos_دننه_Id_ترتیب(info);

            لكل (ForceInfo* ثابت finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* ثابت rhsVarp = جديد AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doing_ټاکل() ? "_VassignRHS" : "__VforceRHS")
                        + std::منزل_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(ریښتیا);
                rhsVarp->sig_عامه(ریښتیا);
                rhsVarp->سېټ_Forced_By_کوډ();
                varp->جمع_کړه_بل_دلته(rhsVarp);
                finfo.m_rhsVarVscp = جديد AstVarScope{flp, scopep, rhsVarp};
                scopep->جمع_کړه_Varsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                //پای_ټکی
                Ast_ټاکل* ثابت rhsAssignp = جديد Ast_ټاکل{
                    flp, جديد AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                إذا (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* ثابت touchCallp = جديد AstCMethodHard{
                        flp, جديد AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtype_سېټ_Void();
                    Ast_غوټه_Stmt* ثابت touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->جمع_کړه_بل_دلته(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* ثابت alwaysp
                    = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, تش_اشاره, rhsAssignp};
                Ast_Sen_ونه* ثابت senTreep
                    = جديد Ast_Sen_ونه{flp, جديد Ast_Sen_توکی{flp, Ast_Sen_توکی::Combo{}}};
                Ast_فعال* ثابت activep = جديد Ast_فعال{flp, "force-rhs-update", senTreep};
                activep->sen_ونه_Storep(activep->sentreep());
                activep->جمع_کړه_Stmtsp(alwaysp);
                scopep->جمع_کړه_Blocksp(activep);
            }

            إذا (info.m_forceRdVscp) {
                إذا (!m_forceRdUpdateBuilt.ورننباسه(info.m_varVscp).دوهم) استمر;
                Ast_فعال* ثابت فعال_Initp = جديد Ast_فعال{
                    flp, "force-init",
                    جديد Ast_Sen_ونه{flp, جديد Ast_Sen_توکی{flp, Ast_Sen_توکی::Static{}}}};
                فعال_Initp->sen_ونه_Storep(فعال_Initp->sentreep());
                Ast_غوټه_Stmt* initStmtp = تش_اشاره;
                إذا (AstUnpackArrayDType* ثابت arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ثابت std::ویکټور<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    ثابت صحيح inner_پلنوالی = dims.شا()->subDTypep()->skipRefp()->پلنوالی();
                    initStmtp = foreach_Unpacked_ونې_پاڼه(
                        dims, [&](ثابت std::ویکټور<صحيح>& idx, صحيح /*flat*/) -> Ast_غوټه_Stmt* {
                            Ast_غوټه_بیان* ثابت lhsp = buildNestedArraySel(
                                flp, جديد AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            أرجع جديد Ast_ټاکل{flp, lhsp, makeZeroConst(varp, inner_پلنوالی)};
                        });
                } وإلا {
                    initStmtp = جديد Ast_ټاکل{
                        flp, جديد AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->پلنوالی())};
                }
                initStmtp->جمع_کړه_بل(createForceRdUpdateStmt(info));
                فعال_Initp->جمع_کړه_Stmtsp(جديد AstInitial{flp, initStmtp});
                scopep->جمع_کړه_Blocksp(فعال_Initp);

                Ast_Sen_توکی* itemsp = تش_اشاره;
                خپلکاری جمع_کړه_Sen_توکی = [&](AstVarScope* vscp) {
                    إذا (!vscp) أرجع;
                    Ast_Sen_توکی* ثابت nextp = جديد Ast_Sen_توکی{
                        flp, VEdgeType::ET_بدل_شوی, جديد AstVarRef{flp, vscp, VAccess::READ}};
                    إذا (itemsp) {
                        itemsp->جمع_کړه_بل(nextp);
                    } وإلا {
                        itemsp = nextp;
                    }
                };
                جمع_کړه_Sen_توکی(info.m_forceEnVscp);
                جمع_کړه_Sen_توکی(info.m_forceValVscp);
                AstVarRef* ثابت origSenRefp = جديد AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                Ast_Sen_توکی* ثابت origItemp
                    = جديد Ast_Sen_توکی{flp, VEdgeType::ET_بدل_شوی, origSenRefp};
                إذا (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->جمع_کړه_بل(origItemp);
                لكل (ForceInfo* ثابت finfop : forceps) {
                    إذا (!finfop->m_isExternal) جمع_کړه_Sen_توکی(finfop->m_rhsVarVscp);
                }

                Ast_فعال* ثابت activep
                    = جديد Ast_فعال{flp, "force-rd-update", جديد Ast_Sen_ونه{flp, itemsp}};
                activep->sen_ونه_Storep(activep->sentreep());
                activep->جمع_کړه_Stmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS, تش_اشاره,
                                                 createForceRdUpdateStmt(info)});
                scopep->جمع_کړه_Blocksp(activep);
            }
        }
    }

    Ast_غوټه* createRhsUpdatesForWrite(File_کرښه* flp, AstVar* writtenVarp) ثابت {
        إذا (!doing_ټاکل()) أرجع تش_اشاره;

        ثابت خپلکاری it = m_rhs_Dep_منزل_Forces.پیدا_کړه(writtenVarp);
        إذا (it == m_rhs_Dep_منزل_Forces.پای_ټکی()) أرجع تش_اشاره;

        Ast_غوټه* headp = تش_اشاره;
        Ast_غوټه* tailp = تش_اشاره;
        لكل (ثابت ForceInfo* ثابت finfop : it->دوهم) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            Ast_ټاکل* ثابت updatep
                = جديد Ast_ټاکل{flp, جديد AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_ونه_خالص(خطأ)};
            إذا (tailp) {
                tailp->جمع_کړه_بل_دلته(updatep);
            } وإلا {
                headp = updatep;
            }
            tailp = updatep;
        }
        أرجع headp;
    }

    ثابت ForceInfo& getForceInfo(Ast_ټاکل_Force* forceStmtp) ثابت {
        AstVarScope* ثابت vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        ثابت VarForceInfo* ثابت varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        خپلکاری it2 = varInfo->m_forces.پیدا_کړه(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.پای_ټکی(), "Force statement not found");
        أرجع it2->دوهم;
    }

    ساكن منطقي selOverlapsAnyForce(ثابت VarForceInfo& varInfo, صحيح selLsb, صحيح selMsb) {
        لكل (ثابت خپلکاری& pair : varInfo.m_forces) {
            إذا (pair.دوهم.m_rangeLsb <= selMsb && pair.دوهم.m_rangeMsb >= selLsb) أرجع ریښتیا;
        }
        أرجع خطأ;
    }

    Ast_غوټه_بیان* create_Force_Read_افاده(ثابت VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) ثابت {
        File_کرښه* ثابت flp = originalRefp->fileline();
        AstVarRef* ثابت refp = originalRefp->clone_ونه_خالص(خطأ);
        refp->access(VAccess::READ);
        أرجع createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   تش_اشاره);
    }

    Ast_غوټه_بیان* create_Force_Read_شاخص_افاده(ثابت VarForceInfo& varInfo,
                                                Ast_غوټه_بیان* originalExprp,
                                                Ast_غوټه_بیان* شاخص_Exprp) ثابت {
        File_کرښه* ثابت flp = originalExprp->fileline();
        Ast_غوټه_بیان* ثابت exprp = originalExprp->clone_ونه_خالص(خطأ);
        // Must be an LValue to a static variable
        VN_AS(exprp->get_VAccess_موخه_Recurse(), VarRef)->access(VAccess::READ);
        أرجع createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_شاخص, exprp, originalExprp,
                                   شاخص_Exprp);
    }

    ساكن Ast_غوټه_بیان* rebuild_Sel_لار(Ast_غوټه_بیان* pathp, Ast_غوټه_بیان* بنسټ_Exprp) {
        إذا (ثابت AstSel* ثابت selp = VN_CAST(pathp, Sel)) {
            Ast_غوټه_بیان* ثابت fromp = rebuild_Sel_لار(selp->fromp(), بنسټ_Exprp);
            AstSel* ثابت outp
                = جديد AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->پلنوالی_Const()};
            outp->dtypeFrom(selp);
            أرجع outp;
        }
        أرجع بنسټ_Exprp;
    }
};

// Split deassign concat LHS before converting to release internals.
ساكن فراغ وېشل_Deassign(AstDeassign* nodep) {
    AstConcat* ثابت concatp = VN_CAST(nodep->lhsp(), Concat);
    إذا (!concatp) أرجع;

    File_کرښه* ثابت flp = nodep->fileline();
    AstDeassign* ثابت newLp = جديد AstDeassign{flp, concatp->lhsp()->unlink_Fr_شا()};
    AstDeassign* ثابت newRp = جديد AstDeassign{flp, concatp->rhsp()->unlink_Fr_شا()};
    Ast_غوټه_بیان* ثابت conp = concatp->unlink_Fr_شا();
    nodep->replaceWith(newLp);
    newLp->جمع_کړه_بل_دلته(newRp);
    VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    VL_DO_DANGLING(conp->delete_ونه(), conp);

    وېشل_Deassign(newLp);
    وېشل_Deassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

صنف ForceDiscoveryVisitor final : عام VNVisitorConst {
    ForceState& m_state;
    منطقي m_دننه_Clocked_فعال = خطأ;

    فراغ buildForceableUnpackedArray(AstVarScope* ثابت nodep,
                                     AstUnpackArrayDType* ثابت arrDtypep) {
        AstVar* ثابت varp = nodep->varp();
        ثابت std::ویکټور<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.تشه(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        ثابت Ast_غوټه_DType* ثابت ونې_پاڼه_Dtypep = dims.شا()->subDTypep()->skipRefp();
        ثابت AstBasicDType* ثابت innerBasicp = ونې_پاڼه_Dtypep->basicp();
        إذا (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            أرجع;
        }

        File_کرښه* ثابت flp = varp->fileline();
        ثابت صحيح inner_پلنوالی = ونې_پاڼه_Dtypep->پلنوالی();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* ثابت enVscp = info.m_forceEnVscp;
        AstVarScope* ثابت valVscp = info.m_forceValVscp;

        Ast_Sen_توکی* ثابت itemsp = جديد Ast_Sen_توکی{flp, VEdgeType::ET_بدل_شوی,
                                                  جديد AstVarRef{flp, enVscp, VAccess::READ}};
        Ast_فعال* ثابت activep = جديد Ast_فعال{flp, "force-update", جديد Ast_Sen_ونه{flp, itemsp}};
        activep->sen_ونه_Storep(activep->sentreep());

        Ast_غوټه_Stmt* ثابت alwaysBodyHeadp = ForceState::foreach_Unpacked_ونې_پاڼه(
            dims, [&](ثابت std::ویکټور<صحيح>& idx, صحيح flat) -> Ast_غوټه_Stmt* {
                AstVarRef* ثابت origRefp = جديد AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_غوټه_بیان* ثابت origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_غوټه_بیان* ثابت enSelp = ForceState::buildNestedArraySel(
                    flp, جديد AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_غوټه_بیان* ثابت valSelp = ForceState::buildNestedArraySel(
                    flp, جديد AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_غوټه_بیان* ثابت forceExprp = جديد AstOr{
                    flp, جديد AstAnd{flp, enSelp, valSelp},
                    جديد AstAnd{flp, جديد AstNot{flp, enSelp->clone_ونه_خالص(خطأ)}, origSelp}};
                Ast_غوټه_بیان* ثابت lhsSelp = ForceState::buildNestedArraySel(
                    flp, جديد AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                Ast_ټاکل_Force* ثابت forceAssignp = جديد Ast_ټاکل_Force{flp, lhsSelp, forceExprp};
                forceAssignp->user2(ریښتیا);

                Ast_غوټه_بیان* ثابت rhsClonep = forceExprp->clone_ونه_خالص(خطأ);
                rhsClonep->foreach([varp](AstVarRef* ثابت r) {
                    إذا (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.جمع_کړه_Force_Assignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/inner_پلنوالی - 1,
                                           /*hasArraySel=*/ریښتیا, /*skipRdRefresh=*/ریښتیا);
                أرجع forceAssignp;
            });
        activep->جمع_کړه_Stmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS, تش_اشاره, alwaysBodyHeadp});
        nodep->scopep()->جمع_کړه_Blocksp(activep);
    }

    فراغ لیدنه(Ast_ټاکل_Force* nodep) override {
        إذا (nodep->user2()) أرجع;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* ثابت forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, ریښتیا);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_غوټه_بیان* ثابت rhsExprp = nodep->rhsp()->clone_ونه_خالص(خطأ);

        m_state.جمع_کړه_Force_Assignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    فراغ لیدنه(Ast_ټاکل* nodep) override {
        إذا (m_state.doing_ټاکل() && m_دننه_Clocked_فعال) {
            إذا (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    فراغ لیدنه(Ast_فعال* nodep) override {
        VL_RESTORER(m_دننه_Clocked_فعال);
        m_دننه_Clocked_فعال = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    فراغ لیدنه(AstVarScope* nodep) override {
        إذا (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            إذا (m_state.doing_ټاکل()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                أرجع;
            }

            إذا (AstUnpackArrayDType* ثابت arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                أرجع;
            }

            ثابت AstBasicDType* ثابت bdtypep = nodep->varp()->basicp();
            إذا (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* ثابت varp = nodep->varp();
            File_کرښه* ثابت flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* ثابت enVscp = info.m_forceEnVscp;
            AstVarScope* ثابت valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            Ast_Sen_توکی* ثابت itemsp = جديد Ast_Sen_توکی{flp, VEdgeType::ET_بدل_شوی,
                                                      جديد AstVarRef{flp, enVscp, VAccess::READ}};
            Ast_فعال* ثابت activep
                = جديد Ast_فعال{flp, "force-update", جديد Ast_Sen_ونه{flp, itemsp}};
            activep->sen_ونه_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* ثابت refp = جديد AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* ثابت enRefp = جديد AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* ثابت valRefp = جديد AstVarRef{flp, valVscp, VAccess::READ};
            ثابت AstBasicDType* ثابت basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_غوټه_بیان* ثابت forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_غوټه_بیان*>(جديد AstOr{
                            flp, جديد AstAnd{flp, enRefp, valRefp},
                            جديد AstAnd{flp, جديد AstNot{flp, enRefp->clone_ونه_خالص(خطأ)}, refp}})
                      : static_cast<Ast_غوټه_بیان*>(جديد AstCond{flp, enRefp, valRefp, refp});
            Ast_ټاکل_Force* ثابت forceAssignp
                = جديد Ast_ټاکل_Force{flp, جديد AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(ریښتیا);
            activep->جمع_کړه_Stmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS, تش_اشاره, forceAssignp});
            nodep->scopep()->جمع_کړه_Blocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_غوټه_بیان* ثابت rhsClonep = forceExprp->clone_ونه_خالص(خطأ);
            rhsClonep->foreach([varp](AstVarRef* ثابت refp) {
                إذا (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            ثابت منطقي bitwiseVar = ForceState::isBitwiseDType(varp);
            ثابت صحيح padMsb = bitwiseVar ? (varp->پلنوالی() - 1) : 0;
            صحيح rangeLsb = 0;
            صحيح rangeMsb = padMsb;
            إذا (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.جمع_کړه_Force_Assignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, خطأ);
        }
        iterateChildrenConst(nodep);
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildrenConst(nodep); }

عام:
    څرګند ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_بل_Const_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

صنف ForceConvertVisitor final : عام VNVisitor {
    ForceState& m_state;

    فراغ لیدنه(Ast_ټاکل_Force* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp();
        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ثابت forcedVarp = lhsVarRefp->varp();

        ثابت ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        ثابت ForceState::VarForceInfo* ثابت varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_کرښه* ثابت flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        Ast_ټاکل* ثابت rhsAssignp
            = جديد Ast_ټاکل{flp, جديد AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_ونه_خالص(خطأ)};

        Ast_ټاکل* valAssignp = تش_اشاره;
        Ast_ټاکل* enAssignp = تش_اشاره;
        ثابت منطقي bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        إذا (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_غوټه_بیان* بنسټ_Exprp = nodep->rhsp()->clone_ونه_خالص(خطأ);
            بنسټ_Exprp->foreach(
                [](AstVarRef* ثابت refp) { ForceState::markNonReplaceable(refp); });
            إذا (bitwiseForcedVar) {
                بنسټ_Exprp = ForceState::zero_Pad_منزل_بنسټ_پلنوالی(بنسټ_Exprp, forcedVarp->پلنوالی(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            إذا (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* ثابت ماسک_Constp = ForceState::make_Range_ماسک_Const(
                    nodep, forcedVarp->پلنوالی(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_غوټه_بیان* ثابت valReadp
                    = جديد AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_غوټه_بیان* ثابت valWritep
                    = جديد AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_غوټه_بیان* ثابت notMaskp = جديد AstNot{flp, ماسک_Constp};
                Ast_غوټه_بیان* ثابت maskedOldp = جديد AstAnd{flp, valReadp, notMaskp};
                Ast_غوټه_بیان* ثابت newValp = جديد AstOr{flp, maskedOldp, بنسټ_Exprp};
                valAssignp = جديد Ast_ټاکل{flp, valWritep, newValp};

                Ast_غوټه_بیان* ثابت enReadp
                    = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_غوټه_بیان* ثابت enWritep
                    = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_غوټه_بیان* ثابت newEnp
                    = جديد AstOr{flp, enReadp, ماسک_Constp->clone_ونه_خالص(خطأ)};
                enAssignp = جديد Ast_ټاکل{flp, enWritep, newEnp};
            } وإلا {
                AstConst* ثابت oneConstp = ForceState::make_Range_ماسک_Const(nodep, 1, 0, 0);
                Ast_غوټه_بیان* ثابت rhsValp = ForceState::cast_منزل_غوټه_DType(بنسټ_Exprp, forcedVarp);
                valAssignp = جديد Ast_ټاکل{
                    flp, جديد AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = جديد Ast_ټاکل{
                    flp, جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        ثابت AstSel* ثابت selLhsp = VN_CAST(lhsp, Sel);
        ثابت منطقي array_بټ_Sel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_has_بټ_Sel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->پلنوالی();
        Ast_غوټه_بیان* ثابت rhsDatap = ForceState::build_Rhs_معلومات_بیان(flp, info);
        AstCExpr* ثابت rhsAddrp = جديد AstCExpr{flp};
        rhsAddrp->جمع_کړه("&(");
        rhsAddrp->جمع_کړه(rhsDatap);
        rhsAddrp->جمع_کړه(")");
        AstCMethodHard* ثابت جمع_کړه_Force_Callp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_جمع_کړه,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        جمع_کړه_Force_Callp->جمع_کړه_Pinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        جمع_کړه_Force_Callp->جمع_کړه_Pinsp(rhsAddrp);
        جمع_کړه_Force_Callp->جمع_کړه_Pinsp(
            ForceState::makeConst32(flp, array_بټ_Sel ? info.m_padLsb : info.m_rangeLsb));
        إذا (array_بټ_Sel) {
            جمع_کړه_Force_Callp->جمع_کړه_Pinsp(ForceState::makeConst32(flp, info.m_padLsb));
            جمع_کړه_Force_Callp->جمع_کړه_Pinsp(ForceState::makeConst32(flp, info.m_padMsb));
            جمع_کړه_Force_Callp->جمع_کړه_Pinsp(ForceState::makeConst32(flp, selLhsp->fromp()->پلنوالی()));
        }
        جمع_کړه_Force_Callp->dtype_سېټ_Void();
        Ast_غوټه_Stmt* ثابت stmtp = جمع_کړه_Force_Callp->makeStmt();

        Ast_غوټه* tailp = rhsAssignp;
        إذا (valAssignp) {
            tailp->جمع_کړه_بل_دلته(valAssignp);
            tailp = valAssignp;
        }
        إذا (enAssignp) {
            tailp->جمع_کړه_بل_دلته(enAssignp);
            tailp = enAssignp;
        }
        tailp->جمع_کړه_بل_دلته(stmtp);
        إذا (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->جمع_کړه_بل_دلته(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ لیدنه(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp();
        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ثابت releasedVarp = lhsVarRefp->varp();

        ثابت ForceState::VarForceInfo* ثابت varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        إذا (!varInfo) {
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }
        UASSERT_OBJ(!varInfo->m_forces.تشه(), nodep, "Var info for variable with no forces");

        File_کرښه* ثابت flp = nodep->fileline();

        ثابت ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, خطأ);

        ثابت AstSel* ثابت selLhsp = VN_CAST(lhsp, Sel);
        ثابت منطقي array_بټ_Sel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_has_بټ_Sel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->پلنوالی();
        AstCMethodHard* ثابت releaseCallp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->جمع_کړه_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        إذا (array_بټ_Sel) {
            releaseCallp->جمع_کړه_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->جمع_کړه_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtype_سېټ_Void();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_غوټه_Stmt* ثابت releasep = releaseCallp->makeStmt();

        Ast_ټاکل* clearEnp = تش_اشاره;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        إذا (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_غوټه_بیان* ثابت enWritep
                = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            إذا (ForceState::isBitwiseDType(releasedVarp)) {
                ثابت صحيح var_پلنوالی = releasedVarp->پلنوالی();
                إذا (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == var_پلنوالی - 1) {
                    clearEnp
                        = جديد Ast_ټاکل{flp, enWritep, ForceState::makeZeroConst(nodep, var_پلنوالی)};
                } وإلا {
                    // forceEn = forceEn & ~mask(range);
                    Ast_غوټه_بیان* ثابت enReadp
                        = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* ثابت ماسک_Const = ForceState::make_Range_ماسک_Const(
                        nodep, var_پلنوالی, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_غوټه_بیان* ثابت newEnp
                        = جديد AstAnd{flp, enReadp, جديد AstNot{flp, ماسک_Const}};
                    clearEnp = جديد Ast_ټاکل{flp, enWritep, newEnp};
                }
            } وإلا {
                clearEnp = جديد Ast_ټاکل{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        ثابت AstSel* ثابت selp = VN_CAST(lhsp, Sel);
        Ast_غوټه_بیان* ثابت basep = selp ? selp->fromp() : lhsp;

        Ast_غوټه* stmtListp = releasep;
        إذا (clearEnp) {
            clearEnp->جمع_کړه_بل_دلته(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        ثابت منطقي ډک_Bitwise_Release
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->پلنوالی() - 1;
        إذا (!releasedVarp->isContinuously()
            && !(m_state.doing_ټاکل() && m_state.hasClockedWrite(releasedVarp)
                 && ډک_Bitwise_Release)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            ثابت منطقي has_Opaque_لار = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_غوټه_بیان* forceReadp = تش_اشاره;
            إذا (has_Opaque_لار) {
                forceReadp = m_state.create_Force_Read_شاخص_افاده(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } وإلا إذا (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.create_Force_Read_شاخص_افاده(
                    *varInfo, basep,
                    ForceState::build_Flatten_شاخص_بیان(flp, rangeInfo.m_arrayInfo));
                إذا (selp) { forceReadp = ForceState::rebuild_Sel_لار(lhsp, forceReadp); }
            } وإلا {
                forceReadp
                    = selp ? ForceState::rebuild_Sel_لار(
                                 lhsp, m_state.create_Force_Read_افاده(*varInfo, lhsVarRefp))
                           : m_state.create_Force_Read_افاده(*varInfo, lhsVarRefp);
            }
            Ast_ټاکل* ثابت assignp = جديد Ast_ټاکل{flp, lhsp->clone_ونه_خالص(خطأ), forceReadp};
            assignp->جمع_کړه_بل_دلته(stmtListp);
            stmtListp = assignp;
        }

        إذا (varInfo->m_forceRdVscp) stmtListp->جمع_کړه_بل(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_بل_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

صنف ForceReplaceVisitor final : عام VNVisitor {
    ثابت ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_غوټه_Stmt* m_stmtp = تش_اشاره;
    منطقي m_دننه_Logic = خطأ;

    فراغ iterateLogic(Ast_غوټه* nodep) {
        VL_RESTORER(m_دننه_Logic);
        m_دننه_Logic = ریښتیا;
        iterateChildren(nodep);
    }

    فراغ لیدنه(Ast_غوټه_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_ټاکل* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        إذا (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp()->بنسټ_Fromp(ریښتیا), VarRef)) {
            إذا (Ast_غوټه* ثابت updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->جمع_کړه_بل_دلته(updatep);
            }
        }
    }
    فراغ لیدنه(Ast_ټاکل_Cont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate_And_بل_Null(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    فراغ لیدنه(AstDeassign*) override {}
    فراغ لیدنه(AstCFunc* nodep) override { iterateLogic(nodep); }
    فراغ لیدنه(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override { iterateLogic(nodep); }
    فراغ لیدنه(AstAlways* nodep) override {
        إذا (nodep->keyword() == VAlwaysKwd::CONT_ټاکل) {
            iterateChildren(nodep);
            أرجع;
        }
        iterateLogic(nodep);
    }
    فراغ لیدنه(Ast_Sen_توکی* nodep) override { iterateLogic(nodep); }
    فراغ لیدنه(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef);
        إذا (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            لیدنه(static_cast<Ast_غوټه*>(nodep));
            أرجع;
        }

        AstVar* ثابت varp = refp->varp();
        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(refp->varScopep());
        إذا (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.تشه()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            لیدنه(static_cast<Ast_غوټه*>(nodep));
            أرجع;
        }

        إذا (ثابت AstConst* ثابت lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            ثابت صحيح selLsb = lsbConstp->منزل_SInt();
            ثابت صحيح selMsb = selLsb + nodep->پلنوالی() - 1;
            إذا (!varp->is_Sig_عامه()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                لیدنه(static_cast<Ast_غوټه*>(nodep));
                أرجع;
            }
        }

        File_کرښه* ثابت flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* ثابت refClonep = refp->clone_ونه_خالص(خطأ);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->پلنوالی())};
        callp->جمع_کړه_Pinsp(refClonep);
        callp->جمع_کړه_Pinsp(nodep->lsbp()->clone_ونه_خالص(خطأ));
        callp->جمع_کړه_Pinsp(ForceState::makeConst32(flp, nodep->پلنوالی()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstArraySel* nodep) override {
        إذا (ثابت AstArraySel* ثابت شا_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            إذا (شا_Selp->fromp() == nodep) {
                iterateChildren(nodep);
                أرجع;
            }
        }

        Ast_غوټه* ثابت basep = nodep->بنسټ_Fromp(ریښتیا);
        AstVarRef* ثابت بنسټ_Refp = VN_CAST(basep, VarRef);
        إذا (!بنسټ_Refp) {
            iterateChildren(nodep);
            أرجع;
        }
        AstVar* ثابت varp = بنسټ_Refp->varp();
        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(بنسټ_Refp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        إذا (m_state.isNotReplaceable(بنسټ_Refp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            أرجع;
        }

        إذا (!بنسټ_Refp->access().isReadOnly()) {
            iterateChildren(nodep);
            أرجع;
        }
        إذا (m_state.doing_ټاکل() && varInfo->m_forceRdVscp) {
            بنسټ_Refp->varp(varInfo->m_forceRdVscp->varp());
            بنسټ_Refp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            أرجع;
        }
        ثابت ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        لكل (AstArraySel* ثابت selp : arrayInfo.m_sels) iterate_And_بل_Null(selp->bitp());
        Ast_غوټه_بیان* ثابت شاخص_Exprp
            = ForceState::build_Flatten_شاخص_بیان(nodep->fileline(), arrayInfo);
        Ast_غوټه_بیان* ثابت readExprp
            = m_state.create_Force_Read_شاخص_افاده(*varInfo, nodep, شاخص_Exprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ لیدنه(AstVarRef* nodep) override {
        إذا (m_state.isNotReplaceable(nodep)) أرجع;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        إذا (ثابت AstArraySel* ثابت شا_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            إذا (شا_Selp->fromp() == nodep) أرجع;
        }

        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(nodep->varScopep());
        إذا (!varInfo) أرجع;

        إذا (varInfo->m_forceRdVscp) {
            إذا (nodep->access().isRW()) {
                إذا (m_دننه_Logic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                أرجع;
            }
            إذا (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_غوټه_بیان* ثابت readp
                    = m_state.create_Force_Rd_افاده(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع;
            }
            إذا (m_دننه_Logic && m_stmtp) {
                m_stmtp->جمع_کړه_بل_دلته(m_state.createForceRdUpdateStmt(*varInfo));
            }
            أرجع;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        إذا (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            أرجع;
        }

        UASSERT_OBJ(!varInfo->m_forces.تشه(), nodep, "Var info for variable with no forces");

        إذا (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } وإلا إذا (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_غوټه_بیان* ثابت readExprp = m_state.create_Force_Read_افاده(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
    }
    فراغ لیدنه(Ast_غوټه* nodep) override {
        إذا (ForceState::is_Outermost_Opaque_لار_Selector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_غوټه_بیان* ثابت exprp = VN_AS(nodep, غوټه_بیان);
            Ast_غوټه* ثابت basep = exprp->بنسټ_Fromp(ریښتیا);
            AstVarRef* ثابت بنسټ_Refp = VN_CAST(basep, VarRef);
            إذا (بنسټ_Refp) {
                AstVar* ثابت varp = بنسټ_Refp->varp();
                إذا (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    ثابت ForceState::VarForceInfo* ثابت varInfo
                        = m_state.getVarInfo(بنسټ_Refp->varScopep());
                    إذا (!m_state.isNotReplaceable(بنسټ_Refp) && varInfo) {
                        ثابت صحيح force_لار_شاخص = varInfo->پیدا_کړه_Force_لار_شاخص(exprp);
                        إذا (force_لار_شاخص >= 0) {
                            إذا (!بنسټ_Refp->access().isReadOnly()) أرجع;
                            Ast_غوټه_بیان* ثابت readExprp = m_state.create_Force_Read_شاخص_افاده(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), force_لار_شاخص));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                            أرجع;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

عام:
    څرګند ForceReplaceVisitor(AstNetlist* nodep, ثابت ForceState& state)
        : m_state{state} {
        iterate_And_بل_Null(nodep->modulesp());
    }
    ~ForceReplaceVisitor() override {
        V3Stats::جمع_کړه_Stat("Non-overlapping force sels", m_nonOverlappingForceSels);
    }
};
//######################################################################
//

//######################################################################
// V3Force - Main entry point

ساكن فراغ forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::Protected_Var_Ref_سېټ& permanentlyProtected,
                         ForceState::Built_Var_Scope_سېټ& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    إذا (!v3Global.has_Forceable_سیګنالونه()) أرجع;
    ForceState state{خطأ, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_ونه("force", 0, dump_ونه_Either_Level() >= 3);
}

ساكن فراغ ټاکل_All_Impl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::Protected_Var_Ref_سېټ& permanentlyProtected,
                          ForceState::Built_Var_Scope_سېټ& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    إذا (!v3Global.has_ټاکل_Deassign()) أرجع;

    std::ویکټور<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.ټېل_شا(deassignp); });
    لكل (AstDeassign* ثابت deassignp : deassignps) وېشل_Deassign(deassignp);

    std::ویکټور<Ast_ټاکل_Cont*> ټاکل_Contps;
    deassignps.clear();
    nodep->foreach([&](Ast_غوټه_Stmt* nodep) {
        إذا (Ast_ټاکل_Cont* ثابت ټاکل_Contp = VN_CAST(nodep, ټاکل_Cont)) {
            ټاکل_Contps.ټېل_شا(ټاکل_Contp);
        } وإلا إذا (AstDeassign* ثابت deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.ټېل_شا(deassignp);
        }
    });

    لكل (Ast_ټاکل_Cont* ثابت assignp : ټاکل_Contps) {
        assignp->replaceWith(جديد Ast_ټاکل_Force{assignp->fileline(),
                                                assignp->lhsp()->unlink_Fr_شا(),
                                                assignp->rhsp()->unlink_Fr_شا()});
        VL_DO_DANGLING(assignp->delete_ونه(), assignp);
    }
    لكل (AstDeassign* ثابت deassignp : deassignps) {
        deassignp->replaceWith(
            جديد AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_ونه_خالص(ریښتیا)});
        VL_DO_DANGLING(deassignp->delete_ونه(), deassignp);
    }
    ForceState state{ریښتیا, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_ونه("assign-deassign", 0, dump_ونه_Either_Level() >= 3);
}

فراغ V3Force::force_And_ټاکل_All(AstNetlist* nodep) {
    ثابت VNUser3_دننه_Use user3_دننه_Use;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::Protected_Var_Ref_سېټ permanentlyProtected;
    ForceState::Built_Var_Scope_سېټ forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    ټاکل_All_Impl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
