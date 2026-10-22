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
        AstVarScope* m_rhsVarVscp = اشاره_تهی;  // Scope of the var containing RHSID
        Ast_گره_عبارت* m_rhsExprp = اشاره_تهی;  // Expression on RHS of this force assignment

        ForceInfo() = افتراضي;
        ForceInfo(صحيح rangeLsb, صحيح rangeMsb, صحيح padLsb, صحيح padMsb, صحيح forceId,
                  منطقي hasArraySel, AstVarScope* rhsVarVscp, Ast_گره_عبارت* rhsExprp)
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
        AstVarScope* m_forceVecVscp = اشاره_تهی;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = اشاره_تهی;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = اشاره_تهی;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = اشاره_تهی;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = اشاره_تهی;  // Scope of the forced (original) variable
        AstVar* m_varp = اشاره_تهی;  // The forced (original) variable
        AstScope* m_scopep = اشاره_تهی;  // Scope containing the forced variable
        std::unordered_map<Ast_تخصیص_بده_Force*, ForceInfo> m_forces;
        std::unordered_map<string, صحيح> m_force_مسیر_مقصد_شاخص;
        صحيح m_بعدی_Force_مسیر_شاخص = 1;  // Start at 1 so 0 can be the base path (whole signal)

    خاص:
        ساكن فراغ build_Force_مسیر_کلید_Recurse(Ast_گره_عبارت* nodep, string& بیرون) {
            إذا (VN_IS(nodep, VarRef)) أرجع;
            إذا (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
                build_Force_مسیر_کلید_Recurse(selp->fromp(), بیرون);
                بیرون += "|S" + cvt_مقصد_Str(selp->lsbConst()) + ":" + cvt_مقصد_Str(selp->عرض_Const());
                أرجع;
            }
            إذا (AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
                Ast_گره_عبارت* ثابت fromp = selp->fromp();
                build_Force_مسیر_کلید_Recurse(fromp, بیرون);
                بیرون += "|T" + selp->name();
                أرجع;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        ساكن string force_مسیر_کلید(Ast_گره_عبارت* nodep) {
            string بیرون;
            build_Force_مسیر_کلید_Recurse(nodep, بیرون);
            أرجع بیرون;
        }

    عام:
        صحيح get_Or_Create_Force_مسیر_شاخص(Ast_گره_عبارت* nodep) {
            ثابت خودکار pair
                = m_force_مسیر_مقصد_شاخص.emplace(force_مسیر_کلید(nodep), m_بعدی_Force_مسیر_شاخص);
            إذا (pair.دومی) ++m_بعدی_Force_مسیر_شاخص;
            أرجع pair.اولی->دومی;
        }

        صحيح پیدا_کن_Force_مسیر_شاخص(Ast_گره_عبارت* nodep) ثابت {
            ثابت خودکار it = m_force_مسیر_مقصد_شاخص.پیدا_کن(force_مسیر_کلید(nodep));
            أرجع it != m_force_مسیر_مقصد_شاخص.انتها() ? it->دومی : -1;
        }
    };

    بنية ForceHelperVars final {
        AstVar* m_rdVarp = اشاره_تهی;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = اشاره_تهی;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = اشاره_تهی;  // The __VforceVal (force value) variable
    };

    بنية ArraySelInfo final {
        std::وکتور<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        منطقي m_has_بیت_Sel = خطأ;  // Bit-select was found along the path too
    };

    بنية ForceRangeInfo final : ForceRange {
        منطقي m_hasArraySel = خطأ;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

خاص:
    استفاده ScopeVarCache = std::unordered_map<ثابت AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    ثابت VNUser1_درون_Use m_user1_درون_Use;
    ثابت VNUser2_درون_Use m_user2_درون_Use;

عام:
    استفاده ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    استفاده Protected_Var_Ref_مجموعه = std::unordered_مجموعه<ثابت AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    استفاده Built_Var_Scope_مجموعه = std::unordered_مجموعه<ثابت AstVarScope*>;

خاص:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::وکتور<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, صحيح> m_var_مقصد_Id;
    std::unordered_مجموعه<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::وکتور<ForceInfo*>> m_rhs_Dep_مقصد_Forces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    منطقي m_doing_تخصیص_بده = خطأ;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    Protected_Var_Ref_مجموعه& m_permanentlyProtected;
    Built_Var_Scope_مجموعه& m_forceRdUpdateBuilt;

عام:
    ForceState(منطقي doing_تخصیص_بده, ForceHelperVarsByVar& forceHelperVarsByVar,
               Protected_Var_Ref_مجموعه& permanentlyProtected, Built_Var_Scope_مجموعه& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doing_تخصیص_بده{doing_تخصیص_بده}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    ساكن AstConst* makeZeroConst(Ast_گره* nodep, صحيح عرض) {
        V3_عدد zero{nodep, عرض};
        zero.مجموعه_All_Bits0();
        أرجع جديد AstConst{nodep->fileline(), zero};
    }

    ساكن AstConst* makeConst32(File_سطر* flp, صحيح مقدار) {
        أرجع جديد AstConst{flp, AstConst::Widthed_مقدار{}, 32, static_cast<uint32_t>(مقدار)};
    }

    ساكن AstConst* make_Range_ماسک_Const(Ast_گره* nodep, صحيح عرض, صحيح lsb, صحيح msb) {
        V3_عدد ماسک{nodep, عرض};
        ماسک.مجموعه_All_Bits0();
        لكل (صحيح بیت_2 = lsb; بیت_2 <= msb; ++بیت_2) ماسک.مجموعه_بیت(بیت_2, 1);
        أرجع جديد AstConst{nodep->fileline(), ماسک};
    }

    ساكن Ast_گره_عبارت* zero_Pad_مقصد_پایه_عرض(Ast_گره_عبارت* exprp, صحيح پایه_عرض, صحيح padLsb,
                                           صحيح padMsb) {
        إذا (پایه_عرض <= 0) أرجع exprp;
        ثابت صحيح کم_Pad = padLsb;
        ثابت صحيح بالا_Pad = پایه_عرض - (padMsb + 1);
        إذا (کم_Pad > 0) {
            exprp = جديد AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, کم_Pad)};
        }
        إذا (بالا_Pad > 0) {
            exprp = جديد AstConcat{exprp->fileline(), makeZeroConst(exprp, بالا_Pad), exprp};
        }
        أرجع exprp;
    }

    ساكن منطقي isUnpackedArrayDType(ثابت Ast_گره_DType* dtypep) {
        أرجع VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    ساكن منطقي isBitwiseDType(ثابت Ast_گره* nodep) {
        ثابت AstBasicDType* ثابت basicp = nodep->dtypep()->skipRefp()->basicp();
        أرجع basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    ساكن Ast_گره_عبارت* cast_مقصد_گره_DType(Ast_گره_عبارت* exprp, Ast_گره* dtypeFromp) {
        ثابت Ast_گره_DType* ثابت dtypep = dtypeFromp->dtypep()->skipRefp();
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();
        إذا (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            أرجع exprp;
        }
        أرجع جديد AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    ساكن فراغ markNonReplaceable(AstVarRef* ثابت nodep) { nodep->user1_مجموعه_Once(); }
    منطقي isNotReplaceable(ثابت AstVarRef* ثابت nodep) ثابت {
        أرجع nodep->user1() || m_permanentlyProtected.شمار(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    فراغ markPermanentlyProtected(AstVarRef* ثابت nodep) ثابت {
        markNonReplaceable(nodep);
        m_permanentlyProtected.درج(nodep);
    }

    ساكن std::وکتور<ForceInfo*> force_Infos_درون_Id_ترتیب(VarForceInfo& info) {
        std::وکتور<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.اندازه());
        لكل (خودکار& it : info.m_forces) forceps.افزودن_پشت(&it.دومی);
        std::مرتب_سازی(forceps.begin(), forceps.انتها(), [](ثابت ForceInfo* ap, ثابت ForceInfo* bp) {
            أرجع ap->m_forceId < bp->m_forceId;
        });
        أرجع forceps;
    }

    ساكن std::وکتور<ثابت ForceInfo*> force_Infos_درون_Id_ترتیب(ثابت VarForceInfo& info) {
        std::وکتور<ثابت ForceInfo*> forceps;
        forceps.reserve(info.m_forces.اندازه());
        لكل (ثابت خودکار& it : info.m_forces) forceps.افزودن_پشت(&it.دومی);
        std::مرتب_سازی(forceps.begin(), forceps.انتها(), [](ثابت ForceInfo* ap, ثابت ForceInfo* bp) {
            أرجع ap->m_forceId < bp->m_forceId;
        });
        أرجع forceps;
    }

    ساكن منطقي is_Opaque_مسیر_Selector(ثابت Ast_گره* nodep) {
        أرجع VN_IS(nodep, Sel) || VN_IS(nodep, گره_Sel) || VN_IS(nodep, StructSel);
    }

    ساكن منطقي is_Outermost_Opaque_مسیر_Selector(ثابت Ast_گره* nodep) {
        إذا (!is_Opaque_مسیر_Selector(nodep)) أرجع خطأ;
        ثابت Ast_گره* ثابت backp = nodep->backp();
        أرجع !backp || !is_Opaque_مسیر_Selector(backp);
    }

    ساكن AstVarRef* getOneVarRef(Ast_گره_عبارت* forceStmtp) {
        Ast_گره* ثابت basep = forceStmtp->پایه_Fromp(درست);
        إذا (AstSampled* sampledp = VN_CAST(basep, Sampled))
            إذا (Ast_گره_عبارت* exprp = VN_CAST(sampledp->exprp(), گره_عبارت))
                أرجع getOneVarRef(exprp);
        AstVarRef* ثابت varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        أرجع varRefp;
    }

    ساكن Ast_گره_عبارت* buildNestedArraySel(File_سطر* flp, Ast_گره_عبارت* fromp,
                                            ثابت std::وکتور<صحيح>& indicies) {
        Ast_گره_عبارت* curp = fromp;
        لكل (ثابت صحيح idx : indicies) curp = جديد AstArraySel{flp, curp, idx};
        أرجع curp;
    }

    قالب <نام_نوع Fn>
    ساكن Ast_گره_Stmt* foreach_Unpacked_برگ(ثابت std::وکتور<AstUnpackArrayDType*>& dims,
                                            Fn build_برگ) {
        Ast_گره_Stmt* headp = اشاره_تهی;
        Ast_گره_Stmt* tailp = اشاره_تهی;
        إذا (dims.خالی()) أرجع اشاره_تهی;
        صحيح مجموع = 1;
        لكل (ثابت AstUnpackArrayDType* ثابت d : dims) مجموع *= d->elementsConst();
        إذا (مجموع <= 0) أرجع اشاره_تهی;
        std::وکتور<صحيح> idx(dims.اندازه(), 0);
        لكل (صحيح flat = 0; flat < مجموع; ++flat) {
            Ast_گره_Stmt* ثابت stmtp = build_برگ(idx, flat);
            إذا (!headp) {
                headp = stmtp;
            } وإلا {
                tailp->جمع_کن_بعدی(stmtp);
            }
            tailp = stmtp;
            لكل (صحيح d = static_cast<صحيح>(dims.اندازه()) - 1; d >= 0; --d) {
                إذا (++idx[d] < dims[d]->elementsConst()) اكسر;
                idx[d] = 0;
            }
        }
        أرجع headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_گره_عبارت* lhsp, AstVar* varp,
                                     منطقي requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.خالی();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->عرض() - 1) : 0;

        إذا (ثابت AstSel* ثابت outerSelp = VN_CAST(lhsp, Sel)) {
            صحيح مجموع_Lsb = 0;
            لكل (Ast_گره_عبارت* curp = lhsp; ثابت AstSel* ثابت selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                إذا (requireConstRangeSelect) {
                    UASSERT_OBJ(VN_IS(selp->lsbp(), Const), lhsp,
                                "Unsupported: force on non-const range select");
                }
                مجموع_Lsb += selp->lsbConst();
            }
            info.m_padLsb = مجموع_Lsb;
            info.m_padMsb = مجموع_Lsb + outerSelp->عرض_Const() - 1;
        }

        info.m_rangeLsb = info.m_padLsb;
        info.m_rangeMsb = info.m_padMsb;
        إذا (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::وکتور<صحيح> indices;
            indices.reserve(info.m_arrayInfo.m_sels.اندازه());
            لكل (AstArraySel* ثابت selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.افزودن_پشت(VN_AS(selp->bitp(), Const)->مقصد_SInt());
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
            ثابت صحيح شاخص = varInfo.get_Or_Create_Force_مسیر_شاخص(lhsp);
            info.m_rangeLsb = شاخص;
            info.m_rangeMsb = شاخص;
        }
        أرجع info;
    }

    Ast_گره_عبارت* جمع_کن_Rhs_مقدار_Reads(ثابت VarForceInfo& varInfo, Ast_گره_عبارت* exprp) ثابت {
        إذا (!doing_تخصیص_بده()) أرجع exprp;

        ثابت std::وکتور<ثابت ForceInfo*> forceps = force_Infos_درون_Id_ترتیب(varInfo);
        إذا (forceps.خالی()) أرجع exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* ثابت cexprp = جديد AstCExpr{exprp->fileline(), AstCExpr::خالص{}};
        cexprp->dtypeFrom(exprp);
        cexprp->جمع_کن("(");
        لكل (ثابت ForceInfo* ثابت finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* ثابت refp
                = جديد AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->جمع_کن("(void)(");
            cexprp->جمع_کن(refp);
            cexprp->جمع_کن("), ");
        }
        cexprp->جمع_کن(exprp);
        cexprp->جمع_کن(")");
        أرجع cexprp;
    }

    Ast_گره_عبارت* createForceReadCall(ثابت VarForceInfo& varInfo, File_سطر* flp, VCMethod روش,
                                     Ast_گره_عبارت* originalExprp, Ast_گره* dtypeFromp,
                                     Ast_گره_عبارت* شاخص_Exprp) ثابت {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_گره_عبارت* ثابت origValp
            = جمع_کن_Rhs_مقدار_Reads(varInfo, cast_مقصد_گره_DType(originalExprp, dtypeFromp));

        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, روش, origValp};
        إذا (شاخص_Exprp) callp->جمع_کن_Pinsp(شاخص_Exprp);
        callp->dtypeFrom(dtypeFromp);
        أرجع callp;
    }

    Ast_گره_عبارت* create_Force_Rd_عبارت(ثابت VarForceInfo& varInfo, File_سطر* flp) ثابت {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        إذا (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            إذا (doing_تخصیص_بده()) { أرجع جديد AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3_Error_کد::UNOPTFLAT, درست);
            AstVarRef* ثابت پایه_Refp = جديد AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(پایه_Refp);
            أرجع createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, پایه_Refp, varp,
                                       اشاره_تهی);
        }
        AstVarRef* ثابت پایه_Refp = جديد AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(پایه_Refp);
        Ast_گره_عبارت* ثابت enRefp = جديد AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_گره_عبارت* ثابت valRefp = جديد AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        إذا (isBitwiseDType(varp)) {
            أرجع جديد AstOr{
                flp, جديد AstAnd{flp, enRefp, valRefp},
                جديد AstAnd{flp, جديد AstNot{flp, enRefp->clone_درخت_خالص(خطأ)}, پایه_Refp}};
        }
        أرجع جديد AstCond{flp, enRefp, valRefp, پایه_Refp};
    }

    Ast_گره_Stmt* createForceRdUpdateStmt(ثابت VarForceInfo& varInfo) ثابت {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        إذا (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            أرجع createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_سطر* ثابت flp = varInfo.m_varVscp->fileline();
        أرجع جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             create_Force_Rd_عبارت(varInfo, flp)};
    }

    Ast_گره_Stmt* createForceRdUpdateStmtUnpacked(ثابت VarForceInfo& varInfo) ثابت {
        File_سطر* ثابت flp = varInfo.m_varVscp->fileline();
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* ثابت arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت std::وکتور<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_گره_Stmt* ثابت stmtsp = foreach_Unpacked_برگ(
            dims, [&](ثابت std::وکتور<صحيح>& idx, صحيح /*flat*/) -> Ast_گره_Stmt* {
                AstVarRef* ثابت پایه_Refp = جديد AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(پایه_Refp);
                Ast_گره_عبارت* ثابت پایه_Selp = buildNestedArraySel(flp, پایه_Refp, idx);
                Ast_گره_عبارت* ثابت enSelp = buildNestedArraySel(
                    flp, جديد AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_گره_عبارت* ثابت valSelp = buildNestedArraySel(
                    flp, جديد AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_گره_عبارت* ثابت readExprp = جديد AstOr{
                    flp, جديد AstAnd{flp, enSelp, valSelp},
                    جديد AstAnd{flp, جديد AstNot{flp, enSelp->clone_درخت_خالص(خطأ)}, پایه_Selp}};
                Ast_گره_عبارت* ثابت rdLhsSelp = buildNestedArraySel(
                    flp, جديد AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                أرجع جديد Ast_تخصیص_بده{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        لكل (ثابت ForceInfo* ثابت finfop : force_Infos_درون_Id_ترتیب(varInfo)) {
            إذا (!finfop->m_isExternal) استمر;
            stmtsp->جمع_کن_بعدی(جديد Ast_تخصیص_بده{flp,
                                          جديد AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_درخت_خالص(خطأ)});
        }
        أرجع stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        ثابت خودکار it = m_var_مقصد_Id.پیدا_کن(vscp);
        إذا (it != m_var_مقصد_Id.انتها()) أرجع m_varInfos[it->دومی];

        m_var_مقصد_Id.emplace(vscp, m_varInfos.اندازه());
        m_varInfos.emplace_پشت();
        VarForceInfo& info = m_varInfos.پشت();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* ثابت varp = info.m_varp;
        إذا (!varp->isForceable()) أرجع info;

        File_سطر* ثابت flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        ثابت منطقي helperVarsBuilt = helperVars.m_rdVarp != اشاره_تهی;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != اشاره_تهی)
                        && helperVarsBuilt == (helperVars.m_valVarp != اشاره_تهی),
                    varp, "Incomplete force helper set");
        إذا (!helperVarsBuilt) {
            ثابت منطقي unpacked = isUnpackedArrayDType(varp->dtypep());
            ثابت VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_گره_DType* ثابت enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->پیدا_کن_بیت_DType();
            helperVars.m_rdVarp
                = جديد AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sig_عمومی(درست);
            helperVars.m_enVarp
                = جديد AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(درست);
            helperVars.m_valVarp
                = جديد AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(درست);
            varp->جمع_کن_بعدی_اینجا(helperVars.m_rdVarp);
            varp->جمع_کن_بعدی_اینجا(helperVars.m_enVarp);
            varp->جمع_کن_بعدی_اینجا(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = پیدا_کن_Scope_Var(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = پیدا_کن_Scope_Var(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = پیدا_کن_Scope_Var(info.m_scopep, helperVars.m_valVarp);
        إذا (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } وإلا {
            info.m_forceRdVscp = جديد AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = جديد AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = جديد AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->جمع_کن_Varsp(info.m_forceRdVscp);
            info.m_scopep->جمع_کن_Varsp(info.m_forceEnVscp);
            info.m_scopep->جمع_کن_Varsp(info.m_forceValVscp);
        }
        أرجع info;
    }

    فراغ markClockedWrite(AstVar* varp) { m_clockedWrites.درج(varp); }
    منطقي hasClockedWrite(AstVar* varp) ثابت { أرجع m_clockedWrites.شمار(varp); }

    منطقي doing_تخصیص_بده() ثابت { أرجع m_doing_تخصیص_بده; }

    ثابت VarForceInfo* getVarInfo(AstVarScope* vscp) ثابت {
        ثابت خودکار it = m_var_مقصد_Id.پیدا_کن(vscp);
        أرجع it != m_var_مقصد_Id.انتها() ? &m_varInfos[it->دومی] : اشاره_تهی;
    }

    AstVarScope* پیدا_کن_Scope_Var(AstScope* scopep, ثابت AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        إذا (cache.خالی()) {
            لكل (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        ثابت خودکار it = cache.پیدا_کن(varp);
        أرجع it != cache.انتها() ? it->دومی : اشاره_تهی;
    }
    فراغ جمع_کن_Force_Assignment(AstVar* varp, AstVarScope* vscp, Ast_گره_عبارت* rhsExprp,
                            Ast_تخصیص_بده_Force* forceStmtp, صحيح rangeLsb, صحيح rangeMsb, صحيح padLsb,
                            صحيح padMsb, منطقي hasArraySel, منطقي skipRdRefresh = خطأ) {
        v3Global.مجموعه_استفاده_میکند_Force();
        varp->مجموعه_Forced_By_کد();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        ثابت صحيح forceId = info.m_forces.اندازه();
        File_سطر* ثابت flp = varp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        إذا (!info.m_forceVecVscp) {
            AstCDType* ثابت forceVecDtypep = جديد AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->جمع_کن_Typesp(forceVecDtypep);

            AstVar* ثابت forceVecVarp
                = جديد AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doing_تخصیص_بده ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(خطأ);
            forceVecVarp->is_داخلی(درست);
            varp->جمع_کن_بعدی_اینجا(forceVecVarp);
            info.m_forceVecVscp = جديد AstVarScope{flp, scopep, forceVecVarp};
            scopep->جمع_کن_Varsp(info.m_forceVecVscp);
        }

        خودکار pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, اشاره_تهی, rhsExprp});
        ForceInfo& finfo = pair.اولی->دومی;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        إذا (doing_تخصیص_بده()) {
            std::وکتور<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* ثابت refp) {
                إذا (!refp->access().isReadOnly()) أرجع;
                AstVar* ثابت depVarp = refp->varp();
                إذا (depVarp
                    && std::پیدا_کن(depVarps.begin(), depVarps.انتها(), depVarp) == depVarps.انتها()) {
                    depVarps.افزودن_پشت(depVarp);
                }
            });
            لكل (AstVar* ثابت depVarp : depVarps) m_rhs_Dep_مقصد_Forces[depVarp].افزودن_پشت(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    ساكن فراغ collectArraySelInfo(Ast_گره_عبارت* exprp, ArraySelInfo& info) {
        إذا (ثابت خودکار* ثابت selp = VN_CAST(exprp, Sel)) {
            info.m_has_بیت_Sel = درست;
            collectArraySelInfo(selp->fromp(), info);
        } وإلا إذا (خودکار* ثابت selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.افزودن_پشت(selp);
        } وإلا إذا (ثابت خودکار* ثابت memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    ساكن ArraySelInfo getArraySelInfo(Ast_گره_عبارت* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        أرجع info;
    }

    ساكن std::وکتور<صحيح> arraySelDimSizes(ثابت ArraySelInfo& info) {
        std::وکتور<صحيح> dims;
        dims.reserve(info.m_sels.اندازه());
        لكل (AstArraySel* ثابت selp : info.m_sels) {
            Ast_گره_DType* ثابت dtypep = selp->fromp()->dtypep()->skipRefp();
            ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.افزودن_پشت(arrayp->declRange().elements());
        }
        أرجع dims;
    }

    ساكن صحيح flatten_شاخص(ثابت std::وکتور<صحيح>& indices, ثابت std::وکتور<صحيح>& dimSizes) {
        UASSERT(indices.اندازه() == dimSizes.اندازه(), "Array index and dimension size mismatch");
        صحيح شاخص = 0;
        صحيح stride = 1;
        لكل (صحيح i = static_cast<صحيح>(indices.اندازه()) - 1; i >= 0; --i) {
            شاخص += indices[i] * stride;
            stride *= dimSizes[i];
        }
        أرجع شاخص;
    }

    ساكن Ast_گره_عبارت* build_Flatten_شاخص_عبارت(File_سطر* flp, ثابت ArraySelInfo& info) {
        ثابت std::وکتور<صحيح> dimSizes = arraySelDimSizes(info);
        منطقي allConst = درست;
        لكل (AstArraySel* ثابت selp : info.m_sels) {
            إذا (!VN_IS(selp->bitp(), Const)) {
                allConst = خطأ;
                اكسر;
            }
        }
        إذا (allConst) {
            std::وکتور<صحيح> constIndices;
            constIndices.reserve(info.m_sels.اندازه());
            لكل (AstArraySel* ثابت selp : info.m_sels) {
                constIndices.افزودن_پشت(VN_AS(selp->bitp(), Const)->مقصد_SInt());
            }
            أرجع makeConst32(flp, flatten_شاخص(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_گره_عبارت* resultp = اشاره_تهی;
        صحيح stride = 1;
        لكل (صحيح i = static_cast<صحيح>(info.m_sels.اندازه()) - 1; i >= 0; --i) {
            Ast_گره_عبارت* termp = info.m_sels[i]->bitp()->clone_درخت_خالص(خطأ);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            إذا (termp->عرض() < 32) termp = جديد AstExtend{flp, termp, 32};
            إذا (stride != 1) termp = جديد AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? جديد Ast_جمع_کن{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        أرجع resultp;
    }

    ساكن Ast_گره_عبارت* build_Rhs_داده_عبارت(File_سطر* flp, ثابت ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        أرجع جديد AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    فراغ finalizeRhsVars() {
        لكل (VarForceInfo& info : m_varInfos) {
            AstVar* ثابت varp = info.m_varp;
            إذا (info.m_forces.خالی()) استمر;

            AstScope* ثابت scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_سطر* ثابت flp = varp->fileline();
            ثابت std::وکتور<ForceInfo*> forceps = force_Infos_درون_Id_ترتیب(info);

            لكل (ForceInfo* ثابت finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* ثابت rhsVarp = جديد AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doing_تخصیص_بده() ? "_VassignRHS" : "__VforceRHS")
                        + std::مقصد_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(درست);
                rhsVarp->sig_عمومی(درست);
                rhsVarp->مجموعه_Forced_By_کد();
                varp->جمع_کن_بعدی_اینجا(rhsVarp);
                finfo.m_rhsVarVscp = جديد AstVarScope{flp, scopep, rhsVarp};
                scopep->جمع_کن_Varsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                //انتها
                Ast_تخصیص_بده* ثابت rhsAssignp = جديد Ast_تخصیص_بده{
                    flp, جديد AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                إذا (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* ثابت touchCallp = جديد AstCMethodHard{
                        flp, جديد AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtype_مجموعه_Void();
                    Ast_گره_Stmt* ثابت touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->جمع_کن_بعدی_اینجا(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* ثابت alwaysp
                    = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, اشاره_تهی, rhsAssignp};
                Ast_Sen_درخت* ثابت senTreep
                    = جديد Ast_Sen_درخت{flp, جديد Ast_Sen_قلم{flp, Ast_Sen_قلم::Combo{}}};
                Ast_فعال* ثابت activep = جديد Ast_فعال{flp, "force-rhs-update", senTreep};
                activep->sen_درخت_Storep(activep->sentreep());
                activep->جمع_کن_Stmtsp(alwaysp);
                scopep->جمع_کن_Blocksp(activep);
            }

            إذا (info.m_forceRdVscp) {
                إذا (!m_forceRdUpdateBuilt.درج(info.m_varVscp).دومی) استمر;
                Ast_فعال* ثابت فعال_Initp = جديد Ast_فعال{
                    flp, "force-init",
                    جديد Ast_Sen_درخت{flp, جديد Ast_Sen_قلم{flp, Ast_Sen_قلم::Static{}}}};
                فعال_Initp->sen_درخت_Storep(فعال_Initp->sentreep());
                Ast_گره_Stmt* initStmtp = اشاره_تهی;
                إذا (AstUnpackArrayDType* ثابت arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ثابت std::وکتور<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    ثابت صحيح inner_عرض = dims.پشت()->subDTypep()->skipRefp()->عرض();
                    initStmtp = foreach_Unpacked_برگ(
                        dims, [&](ثابت std::وکتور<صحيح>& idx, صحيح /*flat*/) -> Ast_گره_Stmt* {
                            Ast_گره_عبارت* ثابت lhsp = buildNestedArraySel(
                                flp, جديد AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            أرجع جديد Ast_تخصیص_بده{flp, lhsp, makeZeroConst(varp, inner_عرض)};
                        });
                } وإلا {
                    initStmtp = جديد Ast_تخصیص_بده{
                        flp, جديد AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->عرض())};
                }
                initStmtp->جمع_کن_بعدی(createForceRdUpdateStmt(info));
                فعال_Initp->جمع_کن_Stmtsp(جديد AstInitial{flp, initStmtp});
                scopep->جمع_کن_Blocksp(فعال_Initp);

                Ast_Sen_قلم* itemsp = اشاره_تهی;
                خودکار جمع_کن_Sen_قلم = [&](AstVarScope* vscp) {
                    إذا (!vscp) أرجع;
                    Ast_Sen_قلم* ثابت nextp = جديد Ast_Sen_قلم{
                        flp, VEdgeType::ET_تغییریافته, جديد AstVarRef{flp, vscp, VAccess::READ}};
                    إذا (itemsp) {
                        itemsp->جمع_کن_بعدی(nextp);
                    } وإلا {
                        itemsp = nextp;
                    }
                };
                جمع_کن_Sen_قلم(info.m_forceEnVscp);
                جمع_کن_Sen_قلم(info.m_forceValVscp);
                AstVarRef* ثابت origSenRefp = جديد AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                Ast_Sen_قلم* ثابت origItemp
                    = جديد Ast_Sen_قلم{flp, VEdgeType::ET_تغییریافته, origSenRefp};
                إذا (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->جمع_کن_بعدی(origItemp);
                لكل (ForceInfo* ثابت finfop : forceps) {
                    إذا (!finfop->m_isExternal) جمع_کن_Sen_قلم(finfop->m_rhsVarVscp);
                }

                Ast_فعال* ثابت activep
                    = جديد Ast_فعال{flp, "force-rd-update", جديد Ast_Sen_درخت{flp, itemsp}};
                activep->sen_درخت_Storep(activep->sentreep());
                activep->جمع_کن_Stmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS, اشاره_تهی,
                                                 createForceRdUpdateStmt(info)});
                scopep->جمع_کن_Blocksp(activep);
            }
        }
    }

    Ast_گره* createRhsUpdatesForWrite(File_سطر* flp, AstVar* writtenVarp) ثابت {
        إذا (!doing_تخصیص_بده()) أرجع اشاره_تهی;

        ثابت خودکار it = m_rhs_Dep_مقصد_Forces.پیدا_کن(writtenVarp);
        إذا (it == m_rhs_Dep_مقصد_Forces.انتها()) أرجع اشاره_تهی;

        Ast_گره* headp = اشاره_تهی;
        Ast_گره* tailp = اشاره_تهی;
        لكل (ثابت ForceInfo* ثابت finfop : it->دومی) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            Ast_تخصیص_بده* ثابت updatep
                = جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_درخت_خالص(خطأ)};
            إذا (tailp) {
                tailp->جمع_کن_بعدی_اینجا(updatep);
            } وإلا {
                headp = updatep;
            }
            tailp = updatep;
        }
        أرجع headp;
    }

    ثابت ForceInfo& getForceInfo(Ast_تخصیص_بده_Force* forceStmtp) ثابت {
        AstVarScope* ثابت vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        ثابت VarForceInfo* ثابت varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        خودکار it2 = varInfo->m_forces.پیدا_کن(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.انتها(), "Force statement not found");
        أرجع it2->دومی;
    }

    ساكن منطقي selOverlapsAnyForce(ثابت VarForceInfo& varInfo, صحيح selLsb, صحيح selMsb) {
        لكل (ثابت خودکار& pair : varInfo.m_forces) {
            إذا (pair.دومی.m_rangeLsb <= selMsb && pair.دومی.m_rangeMsb >= selLsb) أرجع درست;
        }
        أرجع خطأ;
    }

    Ast_گره_عبارت* create_Force_Read_عبارت(ثابت VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) ثابت {
        File_سطر* ثابت flp = originalRefp->fileline();
        AstVarRef* ثابت refp = originalRefp->clone_درخت_خالص(خطأ);
        refp->access(VAccess::READ);
        أرجع createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   اشاره_تهی);
    }

    Ast_گره_عبارت* create_Force_Read_شاخص_عبارت(ثابت VarForceInfo& varInfo,
                                                Ast_گره_عبارت* originalExprp,
                                                Ast_گره_عبارت* شاخص_Exprp) ثابت {
        File_سطر* ثابت flp = originalExprp->fileline();
        Ast_گره_عبارت* ثابت exprp = originalExprp->clone_درخت_خالص(خطأ);
        // Must be an LValue to a static variable
        VN_AS(exprp->get_VAccess_هدف_Recurse(), VarRef)->access(VAccess::READ);
        أرجع createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_شاخص, exprp, originalExprp,
                                   شاخص_Exprp);
    }

    ساكن Ast_گره_عبارت* rebuild_Sel_مسیر(Ast_گره_عبارت* pathp, Ast_گره_عبارت* پایه_Exprp) {
        إذا (ثابت AstSel* ثابت selp = VN_CAST(pathp, Sel)) {
            Ast_گره_عبارت* ثابت fromp = rebuild_Sel_مسیر(selp->fromp(), پایه_Exprp);
            AstSel* ثابت outp
                = جديد AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->عرض_Const()};
            outp->dtypeFrom(selp);
            أرجع outp;
        }
        أرجع پایه_Exprp;
    }
};

// Split deassign concat LHS before converting to release internals.
ساكن فراغ تقسیم_Deassign(AstDeassign* nodep) {
    AstConcat* ثابت concatp = VN_CAST(nodep->lhsp(), Concat);
    إذا (!concatp) أرجع;

    File_سطر* ثابت flp = nodep->fileline();
    AstDeassign* ثابت newLp = جديد AstDeassign{flp, concatp->lhsp()->unlink_Fr_پشت()};
    AstDeassign* ثابت newRp = جديد AstDeassign{flp, concatp->rhsp()->unlink_Fr_پشت()};
    Ast_گره_عبارت* ثابت conp = concatp->unlink_Fr_پشت();
    nodep->replaceWith(newLp);
    newLp->جمع_کن_بعدی_اینجا(newRp);
    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    VL_DO_DANGLING(conp->delete_درخت(), conp);

    تقسیم_Deassign(newLp);
    تقسیم_Deassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

صنف ForceDiscoveryVisitor final : عام VNVisitorConst {
    ForceState& m_state;
    منطقي m_درون_Clocked_فعال = خطأ;

    فراغ buildForceableUnpackedArray(AstVarScope* ثابت nodep,
                                     AstUnpackArrayDType* ثابت arrDtypep) {
        AstVar* ثابت varp = nodep->varp();
        ثابت std::وکتور<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.خالی(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        ثابت Ast_گره_DType* ثابت برگ_Dtypep = dims.پشت()->subDTypep()->skipRefp();
        ثابت AstBasicDType* ثابت innerBasicp = برگ_Dtypep->basicp();
        إذا (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            أرجع;
        }

        File_سطر* ثابت flp = varp->fileline();
        ثابت صحيح inner_عرض = برگ_Dtypep->عرض();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* ثابت enVscp = info.m_forceEnVscp;
        AstVarScope* ثابت valVscp = info.m_forceValVscp;

        Ast_Sen_قلم* ثابت itemsp = جديد Ast_Sen_قلم{flp, VEdgeType::ET_تغییریافته,
                                                  جديد AstVarRef{flp, enVscp, VAccess::READ}};
        Ast_فعال* ثابت activep = جديد Ast_فعال{flp, "force-update", جديد Ast_Sen_درخت{flp, itemsp}};
        activep->sen_درخت_Storep(activep->sentreep());

        Ast_گره_Stmt* ثابت alwaysBodyHeadp = ForceState::foreach_Unpacked_برگ(
            dims, [&](ثابت std::وکتور<صحيح>& idx, صحيح flat) -> Ast_گره_Stmt* {
                AstVarRef* ثابت origRefp = جديد AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_گره_عبارت* ثابت origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_گره_عبارت* ثابت enSelp = ForceState::buildNestedArraySel(
                    flp, جديد AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_گره_عبارت* ثابت valSelp = ForceState::buildNestedArraySel(
                    flp, جديد AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_گره_عبارت* ثابت forceExprp = جديد AstOr{
                    flp, جديد AstAnd{flp, enSelp, valSelp},
                    جديد AstAnd{flp, جديد AstNot{flp, enSelp->clone_درخت_خالص(خطأ)}, origSelp}};
                Ast_گره_عبارت* ثابت lhsSelp = ForceState::buildNestedArraySel(
                    flp, جديد AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                Ast_تخصیص_بده_Force* ثابت forceAssignp = جديد Ast_تخصیص_بده_Force{flp, lhsSelp, forceExprp};
                forceAssignp->user2(درست);

                Ast_گره_عبارت* ثابت rhsClonep = forceExprp->clone_درخت_خالص(خطأ);
                rhsClonep->foreach([varp](AstVarRef* ثابت r) {
                    إذا (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.جمع_کن_Force_Assignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/inner_عرض - 1,
                                           /*hasArraySel=*/درست, /*skipRdRefresh=*/درست);
                أرجع forceAssignp;
            });
        activep->جمع_کن_Stmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS, اشاره_تهی, alwaysBodyHeadp});
        nodep->scopep()->جمع_کن_Blocksp(activep);
    }

    فراغ بازدید(Ast_تخصیص_بده_Force* nodep) override {
        إذا (nodep->user2()) أرجع;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* ثابت forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, درست);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_گره_عبارت* ثابت rhsExprp = nodep->rhsp()->clone_درخت_خالص(خطأ);

        m_state.جمع_کن_Force_Assignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    فراغ بازدید(Ast_تخصیص_بده* nodep) override {
        إذا (m_state.doing_تخصیص_بده() && m_درون_Clocked_فعال) {
            إذا (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    فراغ بازدید(Ast_فعال* nodep) override {
        VL_RESTORER(m_درون_Clocked_فعال);
        m_درون_Clocked_فعال = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    فراغ بازدید(AstVarScope* nodep) override {
        إذا (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            إذا (m_state.doing_تخصیص_بده()) {
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
            File_سطر* ثابت flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* ثابت enVscp = info.m_forceEnVscp;
            AstVarScope* ثابت valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            Ast_Sen_قلم* ثابت itemsp = جديد Ast_Sen_قلم{flp, VEdgeType::ET_تغییریافته,
                                                      جديد AstVarRef{flp, enVscp, VAccess::READ}};
            Ast_فعال* ثابت activep
                = جديد Ast_فعال{flp, "force-update", جديد Ast_Sen_درخت{flp, itemsp}};
            activep->sen_درخت_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* ثابت refp = جديد AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* ثابت enRefp = جديد AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* ثابت valRefp = جديد AstVarRef{flp, valVscp, VAccess::READ};
            ثابت AstBasicDType* ثابت basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_گره_عبارت* ثابت forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_گره_عبارت*>(جديد AstOr{
                            flp, جديد AstAnd{flp, enRefp, valRefp},
                            جديد AstAnd{flp, جديد AstNot{flp, enRefp->clone_درخت_خالص(خطأ)}, refp}})
                      : static_cast<Ast_گره_عبارت*>(جديد AstCond{flp, enRefp, valRefp, refp});
            Ast_تخصیص_بده_Force* ثابت forceAssignp
                = جديد Ast_تخصیص_بده_Force{flp, جديد AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(درست);
            activep->جمع_کن_Stmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS, اشاره_تهی, forceAssignp});
            nodep->scopep()->جمع_کن_Blocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_گره_عبارت* ثابت rhsClonep = forceExprp->clone_درخت_خالص(خطأ);
            rhsClonep->foreach([varp](AstVarRef* ثابت refp) {
                إذا (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            ثابت منطقي bitwiseVar = ForceState::isBitwiseDType(varp);
            ثابت صحيح padMsb = bitwiseVar ? (varp->عرض() - 1) : 0;
            صحيح rangeLsb = 0;
            صحيح rangeMsb = padMsb;
            إذا (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.جمع_کن_Force_Assignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, خطأ);
        }
        iterateChildrenConst(nodep);
    }

    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عام:
    صریح ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_بعدی_Const_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

صنف ForceConvertVisitor final : عام VNVisitor {
    ForceState& m_state;

    فراغ بازدید(Ast_تخصیص_بده_Force* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp();
        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ثابت forcedVarp = lhsVarRefp->varp();

        ثابت ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        ثابت ForceState::VarForceInfo* ثابت varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_سطر* ثابت flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        Ast_تخصیص_بده* ثابت rhsAssignp
            = جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_درخت_خالص(خطأ)};

        Ast_تخصیص_بده* valAssignp = اشاره_تهی;
        Ast_تخصیص_بده* enAssignp = اشاره_تهی;
        ثابت منطقي bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        إذا (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_گره_عبارت* پایه_Exprp = nodep->rhsp()->clone_درخت_خالص(خطأ);
            پایه_Exprp->foreach(
                [](AstVarRef* ثابت refp) { ForceState::markNonReplaceable(refp); });
            إذا (bitwiseForcedVar) {
                پایه_Exprp = ForceState::zero_Pad_مقصد_پایه_عرض(پایه_Exprp, forcedVarp->عرض(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            إذا (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* ثابت ماسک_Constp = ForceState::make_Range_ماسک_Const(
                    nodep, forcedVarp->عرض(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_گره_عبارت* ثابت valReadp
                    = جديد AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_گره_عبارت* ثابت valWritep
                    = جديد AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_گره_عبارت* ثابت notMaskp = جديد AstNot{flp, ماسک_Constp};
                Ast_گره_عبارت* ثابت maskedOldp = جديد AstAnd{flp, valReadp, notMaskp};
                Ast_گره_عبارت* ثابت newValp = جديد AstOr{flp, maskedOldp, پایه_Exprp};
                valAssignp = جديد Ast_تخصیص_بده{flp, valWritep, newValp};

                Ast_گره_عبارت* ثابت enReadp
                    = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_گره_عبارت* ثابت enWritep
                    = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_گره_عبارت* ثابت newEnp
                    = جديد AstOr{flp, enReadp, ماسک_Constp->clone_درخت_خالص(خطأ)};
                enAssignp = جديد Ast_تخصیص_بده{flp, enWritep, newEnp};
            } وإلا {
                AstConst* ثابت oneConstp = ForceState::make_Range_ماسک_Const(nodep, 1, 0, 0);
                Ast_گره_عبارت* ثابت rhsValp = ForceState::cast_مقصد_گره_DType(پایه_Exprp, forcedVarp);
                valAssignp = جديد Ast_تخصیص_بده{
                    flp, جديد AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = جديد Ast_تخصیص_بده{
                    flp, جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        ثابت AstSel* ثابت selLhsp = VN_CAST(lhsp, Sel);
        ثابت منطقي array_بیت_Sel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_has_بیت_Sel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->عرض();
        Ast_گره_عبارت* ثابت rhsDatap = ForceState::build_Rhs_داده_عبارت(flp, info);
        AstCExpr* ثابت rhsAddrp = جديد AstCExpr{flp};
        rhsAddrp->جمع_کن("&(");
        rhsAddrp->جمع_کن(rhsDatap);
        rhsAddrp->جمع_کن(")");
        AstCMethodHard* ثابت جمع_کن_Force_Callp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_جمع_کن,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        جمع_کن_Force_Callp->جمع_کن_Pinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        جمع_کن_Force_Callp->جمع_کن_Pinsp(rhsAddrp);
        جمع_کن_Force_Callp->جمع_کن_Pinsp(
            ForceState::makeConst32(flp, array_بیت_Sel ? info.m_padLsb : info.m_rangeLsb));
        إذا (array_بیت_Sel) {
            جمع_کن_Force_Callp->جمع_کن_Pinsp(ForceState::makeConst32(flp, info.m_padLsb));
            جمع_کن_Force_Callp->جمع_کن_Pinsp(ForceState::makeConst32(flp, info.m_padMsb));
            جمع_کن_Force_Callp->جمع_کن_Pinsp(ForceState::makeConst32(flp, selLhsp->fromp()->عرض()));
        }
        جمع_کن_Force_Callp->dtype_مجموعه_Void();
        Ast_گره_Stmt* ثابت stmtp = جمع_کن_Force_Callp->makeStmt();

        Ast_گره* tailp = rhsAssignp;
        إذا (valAssignp) {
            tailp->جمع_کن_بعدی_اینجا(valAssignp);
            tailp = valAssignp;
        }
        إذا (enAssignp) {
            tailp->جمع_کن_بعدی_اینجا(enAssignp);
            tailp = enAssignp;
        }
        tailp->جمع_کن_بعدی_اینجا(stmtp);
        إذا (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->جمع_کن_بعدی_اینجا(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    فراغ بازدید(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp();
        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ثابت releasedVarp = lhsVarRefp->varp();

        ثابت ForceState::VarForceInfo* ثابت varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        إذا (!varInfo) {
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
            أرجع;
        }
        UASSERT_OBJ(!varInfo->m_forces.خالی(), nodep, "Var info for variable with no forces");

        File_سطر* ثابت flp = nodep->fileline();

        ثابت ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, خطأ);

        ثابت AstSel* ثابت selLhsp = VN_CAST(lhsp, Sel);
        ثابت منطقي array_بیت_Sel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_has_بیت_Sel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->عرض();
        AstCMethodHard* ثابت releaseCallp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->جمع_کن_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        إذا (array_بیت_Sel) {
            releaseCallp->جمع_کن_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->جمع_کن_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtype_مجموعه_Void();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_گره_Stmt* ثابت releasep = releaseCallp->makeStmt();

        Ast_تخصیص_بده* clearEnp = اشاره_تهی;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        إذا (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_گره_عبارت* ثابت enWritep
                = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            إذا (ForceState::isBitwiseDType(releasedVarp)) {
                ثابت صحيح var_عرض = releasedVarp->عرض();
                إذا (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == var_عرض - 1) {
                    clearEnp
                        = جديد Ast_تخصیص_بده{flp, enWritep, ForceState::makeZeroConst(nodep, var_عرض)};
                } وإلا {
                    // forceEn = forceEn & ~mask(range);
                    Ast_گره_عبارت* ثابت enReadp
                        = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* ثابت ماسک_Const = ForceState::make_Range_ماسک_Const(
                        nodep, var_عرض, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_گره_عبارت* ثابت newEnp
                        = جديد AstAnd{flp, enReadp, جديد AstNot{flp, ماسک_Const}};
                    clearEnp = جديد Ast_تخصیص_بده{flp, enWritep, newEnp};
                }
            } وإلا {
                clearEnp = جديد Ast_تخصیص_بده{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        ثابت AstSel* ثابت selp = VN_CAST(lhsp, Sel);
        Ast_گره_عبارت* ثابت basep = selp ? selp->fromp() : lhsp;

        Ast_گره* stmtListp = releasep;
        إذا (clearEnp) {
            clearEnp->جمع_کن_بعدی_اینجا(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        ثابت منطقي پر_Bitwise_Release
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->عرض() - 1;
        إذا (!releasedVarp->isContinuously()
            && !(m_state.doing_تخصیص_بده() && m_state.hasClockedWrite(releasedVarp)
                 && پر_Bitwise_Release)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            ثابت منطقي has_Opaque_مسیر = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_گره_عبارت* forceReadp = اشاره_تهی;
            إذا (has_Opaque_مسیر) {
                forceReadp = m_state.create_Force_Read_شاخص_عبارت(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } وإلا إذا (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.create_Force_Read_شاخص_عبارت(
                    *varInfo, basep,
                    ForceState::build_Flatten_شاخص_عبارت(flp, rangeInfo.m_arrayInfo));
                إذا (selp) { forceReadp = ForceState::rebuild_Sel_مسیر(lhsp, forceReadp); }
            } وإلا {
                forceReadp
                    = selp ? ForceState::rebuild_Sel_مسیر(
                                 lhsp, m_state.create_Force_Read_عبارت(*varInfo, lhsVarRefp))
                           : m_state.create_Force_Read_عبارت(*varInfo, lhsVarRefp);
            }
            Ast_تخصیص_بده* ثابت assignp = جديد Ast_تخصیص_بده{flp, lhsp->clone_درخت_خالص(خطأ), forceReadp};
            assignp->جمع_کن_بعدی_اینجا(stmtListp);
            stmtListp = assignp;
        }

        إذا (varInfo->m_forceRdVscp) stmtListp->جمع_کن_بعدی(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_بعدی_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

صنف ForceReplaceVisitor final : عام VNVisitor {
    ثابت ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_گره_Stmt* m_stmtp = اشاره_تهی;
    منطقي m_درون_Logic = خطأ;

    فراغ iterateLogic(Ast_گره* nodep) {
        VL_RESTORER(m_درون_Logic);
        m_درون_Logic = درست;
        iterateChildren(nodep);
    }

    فراغ بازدید(Ast_گره_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_تخصیص_بده* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        إذا (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp()->پایه_Fromp(درست), VarRef)) {
            إذا (Ast_گره* ثابت updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->جمع_کن_بعدی_اینجا(updatep);
            }
        }
    }
    فراغ بازدید(Ast_تخصیص_بده_Cont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate_And_بعدی_Null(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    فراغ بازدید(AstDeassign*) override {}
    فراغ بازدید(AstCFunc* nodep) override { iterateLogic(nodep); }
    فراغ بازدید(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    فراغ بازدید(Ast_گره_Procedure* nodep) override { iterateLogic(nodep); }
    فراغ بازدید(AstAlways* nodep) override {
        إذا (nodep->keyword() == VAlwaysKwd::CONT_تخصیص_بده) {
            iterateChildren(nodep);
            أرجع;
        }
        iterateLogic(nodep);
    }
    فراغ بازدید(Ast_Sen_قلم* nodep) override { iterateLogic(nodep); }
    فراغ بازدید(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef);
        إذا (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            بازدید(static_cast<Ast_گره*>(nodep));
            أرجع;
        }

        AstVar* ثابت varp = refp->varp();
        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(refp->varScopep());
        إذا (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.خالی()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            بازدید(static_cast<Ast_گره*>(nodep));
            أرجع;
        }

        إذا (ثابت AstConst* ثابت lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            ثابت صحيح selLsb = lsbConstp->مقصد_SInt();
            ثابت صحيح selMsb = selLsb + nodep->عرض() - 1;
            إذا (!varp->is_Sig_عمومی()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                بازدید(static_cast<Ast_گره*>(nodep));
                أرجع;
            }
        }

        File_سطر* ثابت flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* ثابت refClonep = refp->clone_درخت_خالص(خطأ);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->عرض())};
        callp->جمع_کن_Pinsp(refClonep);
        callp->جمع_کن_Pinsp(nodep->lsbp()->clone_درخت_خالص(خطأ));
        callp->جمع_کن_Pinsp(ForceState::makeConst32(flp, nodep->عرض()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    فراغ بازدید(AstArraySel* nodep) override {
        إذا (ثابت AstArraySel* ثابت پشت_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            إذا (پشت_Selp->fromp() == nodep) {
                iterateChildren(nodep);
                أرجع;
            }
        }

        Ast_گره* ثابت basep = nodep->پایه_Fromp(درست);
        AstVarRef* ثابت پایه_Refp = VN_CAST(basep, VarRef);
        إذا (!پایه_Refp) {
            iterateChildren(nodep);
            أرجع;
        }
        AstVar* ثابت varp = پایه_Refp->varp();
        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(پایه_Refp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        إذا (m_state.isNotReplaceable(پایه_Refp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            أرجع;
        }

        إذا (!پایه_Refp->access().isReadOnly()) {
            iterateChildren(nodep);
            أرجع;
        }
        إذا (m_state.doing_تخصیص_بده() && varInfo->m_forceRdVscp) {
            پایه_Refp->varp(varInfo->m_forceRdVscp->varp());
            پایه_Refp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            أرجع;
        }
        ثابت ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        لكل (AstArraySel* ثابت selp : arrayInfo.m_sels) iterate_And_بعدی_Null(selp->bitp());
        Ast_گره_عبارت* ثابت شاخص_Exprp
            = ForceState::build_Flatten_شاخص_عبارت(nodep->fileline(), arrayInfo);
        Ast_گره_عبارت* ثابت readExprp
            = m_state.create_Force_Read_شاخص_عبارت(*varInfo, nodep, شاخص_Exprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    فراغ بازدید(AstVarRef* nodep) override {
        إذا (m_state.isNotReplaceable(nodep)) أرجع;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        إذا (ثابت AstArraySel* ثابت پشت_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            إذا (پشت_Selp->fromp() == nodep) أرجع;
        }

        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(nodep->varScopep());
        إذا (!varInfo) أرجع;

        إذا (varInfo->m_forceRdVscp) {
            إذا (nodep->access().isRW()) {
                إذا (m_درون_Logic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                أرجع;
            }
            إذا (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_گره_عبارت* ثابت readp
                    = m_state.create_Force_Rd_عبارت(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                أرجع;
            }
            إذا (m_درون_Logic && m_stmtp) {
                m_stmtp->جمع_کن_بعدی_اینجا(m_state.createForceRdUpdateStmt(*varInfo));
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

        UASSERT_OBJ(!varInfo->m_forces.خالی(), nodep, "Var info for variable with no forces");

        إذا (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } وإلا إذا (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_گره_عبارت* ثابت readExprp = m_state.create_Force_Read_عبارت(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    فراغ بازدید(Ast_گره* nodep) override {
        إذا (ForceState::is_Outermost_Opaque_مسیر_Selector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_گره_عبارت* ثابت exprp = VN_AS(nodep, گره_عبارت);
            Ast_گره* ثابت basep = exprp->پایه_Fromp(درست);
            AstVarRef* ثابت پایه_Refp = VN_CAST(basep, VarRef);
            إذا (پایه_Refp) {
                AstVar* ثابت varp = پایه_Refp->varp();
                إذا (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    ثابت ForceState::VarForceInfo* ثابت varInfo
                        = m_state.getVarInfo(پایه_Refp->varScopep());
                    إذا (!m_state.isNotReplaceable(پایه_Refp) && varInfo) {
                        ثابت صحيح force_مسیر_شاخص = varInfo->پیدا_کن_Force_مسیر_شاخص(exprp);
                        إذا (force_مسیر_شاخص >= 0) {
                            إذا (!پایه_Refp->access().isReadOnly()) أرجع;
                            Ast_گره_عبارت* ثابت readExprp = m_state.create_Force_Read_شاخص_عبارت(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), force_مسیر_شاخص));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                            أرجع;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

عام:
    صریح ForceReplaceVisitor(AstNetlist* nodep, ثابت ForceState& state)
        : m_state{state} {
        iterate_And_بعدی_Null(nodep->modulesp());
    }
    ~ForceReplaceVisitor() override {
        V3Stats::جمع_کن_Stat("Non-overlapping force sels", m_nonOverlappingForceSels);
    }
};
//######################################################################
//

//######################################################################
// V3Force - Main entry point

ساكن فراغ forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::Protected_Var_Ref_مجموعه& permanentlyProtected,
                         ForceState::Built_Var_Scope_مجموعه& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    إذا (!v3Global.has_Forceable_سیگنالها()) أرجع;
    ForceState state{خطأ, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_درخت("force", 0, dump_درخت_Either_Level() >= 3);
}

ساكن فراغ تخصیص_بده_All_Impl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::Protected_Var_Ref_مجموعه& permanentlyProtected,
                          ForceState::Built_Var_Scope_مجموعه& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    إذا (!v3Global.has_تخصیص_بده_Deassign()) أرجع;

    std::وکتور<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.افزودن_پشت(deassignp); });
    لكل (AstDeassign* ثابت deassignp : deassignps) تقسیم_Deassign(deassignp);

    std::وکتور<Ast_تخصیص_بده_Cont*> تخصیص_بده_Contps;
    deassignps.clear();
    nodep->foreach([&](Ast_گره_Stmt* nodep) {
        إذا (Ast_تخصیص_بده_Cont* ثابت تخصیص_بده_Contp = VN_CAST(nodep, تخصیص_بده_Cont)) {
            تخصیص_بده_Contps.افزودن_پشت(تخصیص_بده_Contp);
        } وإلا إذا (AstDeassign* ثابت deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.افزودن_پشت(deassignp);
        }
    });

    لكل (Ast_تخصیص_بده_Cont* ثابت assignp : تخصیص_بده_Contps) {
        assignp->replaceWith(جديد Ast_تخصیص_بده_Force{assignp->fileline(),
                                                assignp->lhsp()->unlink_Fr_پشت(),
                                                assignp->rhsp()->unlink_Fr_پشت()});
        VL_DO_DANGLING(assignp->delete_درخت(), assignp);
    }
    لكل (AstDeassign* ثابت deassignp : deassignps) {
        deassignp->replaceWith(
            جديد AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_درخت_خالص(درست)});
        VL_DO_DANGLING(deassignp->delete_درخت(), deassignp);
    }
    ForceState state{درست, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_درخت("assign-deassign", 0, dump_درخت_Either_Level() >= 3);
}

فراغ V3Force::force_And_تخصیص_بده_All(AstNetlist* nodep) {
    ثابت VNUser3_درون_Use user3_درون_Use;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::Protected_Var_Ref_مجموعه permanentlyProtected;
    ForceState::Built_Var_Scope_مجموعه forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    تخصیص_بده_All_Impl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
