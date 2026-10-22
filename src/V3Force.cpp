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
        AstVarScope* m_rhsVarVscp = خالي_اشارو;  // Scope of the var containing RHSID
        Ast_ڳنڍ_اظهار* m_rhsExprp = خالي_اشارو;  // Expression on RHS of this force assignment

        ForceInfo() = افتراضي;
        ForceInfo(صحيح rangeLsb, صحيح rangeMsb, صحيح padLsb, صحيح padMsb, صحيح forceId,
                  منطقي hasArraySel, AstVarScope* rhsVarVscp, Ast_ڳنڍ_اظهار* rhsExprp)
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
        AstVarScope* m_forceVecVscp = خالي_اشارو;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = خالي_اشارو;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = خالي_اشارو;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = خالي_اشارو;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = خالي_اشارو;  // Scope of the forced (original) variable
        AstVar* m_varp = خالي_اشارو;  // The forced (original) variable
        AstScope* m_scopep = خالي_اشارو;  // Scope containing the forced variable
        std::unordered_map<Ast_سونپيو_Force*, ForceInfo> m_forces;
        std::unordered_map<string, صحيح> m_force_رستو_منزل_اشاريو;
        صحيح m_ايندڙ_Force_رستو_اشاريو = 1;  // Start at 1 so 0 can be the base path (whole signal)

    خاص:
        ساكن فراغ build_Force_رستو_ڪنجي_Recurse(Ast_ڳنڍ_اظهار* nodep, string& ٻاهريون) {
            إذا (VN_IS(nodep, VarRef)) أرجع;
            إذا (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
                build_Force_رستو_ڪنجي_Recurse(selp->fromp(), ٻاهريون);
                ٻاهريون += "|S" + cvt_منزل_Str(selp->lsbConst()) + ":" + cvt_منزل_Str(selp->ويڪر_Const());
                أرجع;
            }
            إذا (AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
                Ast_ڳنڍ_اظهار* ثابت fromp = selp->fromp();
                build_Force_رستو_ڪنجي_Recurse(fromp, ٻاهريون);
                ٻاهريون += "|T" + selp->name();
                أرجع;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        ساكن string force_رستو_ڪنجي(Ast_ڳنڍ_اظهار* nodep) {
            string ٻاهريون;
            build_Force_رستو_ڪنجي_Recurse(nodep, ٻاهريون);
            أرجع ٻاهريون;
        }

    عام:
        صحيح get_Or_Create_Force_رستو_اشاريو(Ast_ڳنڍ_اظهار* nodep) {
            ثابت خودڪار pair
                = m_force_رستو_منزل_اشاريو.emplace(force_رستو_ڪنجي(nodep), m_ايندڙ_Force_رستو_اشاريو);
            إذا (pair.ٻيو) ++m_ايندڙ_Force_رستو_اشاريو;
            أرجع pair.پهريون->ٻيو;
        }

        صحيح ڳوليو_Force_رستو_اشاريو(Ast_ڳنڍ_اظهار* nodep) ثابت {
            ثابت خودڪار it = m_force_رستو_منزل_اشاريو.ڳوليو(force_رستو_ڪنجي(nodep));
            أرجع it != m_force_رستو_منزل_اشاريو.پڇاڙي() ? it->ٻيو : -1;
        }
    };

    بنية ForceHelperVars final {
        AstVar* m_rdVarp = خالي_اشارو;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = خالي_اشارو;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = خالي_اشارو;  // The __VforceVal (force value) variable
    };

    بنية ArraySelInfo final {
        std::ويڪٽر<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        منطقي m_has_ٻٽو_انگ_Sel = خطأ;  // Bit-select was found along the path too
    };

    بنية ForceRangeInfo final : ForceRange {
        منطقي m_hasArraySel = خطأ;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

خاص:
    استعمال ScopeVarCache = std::unordered_map<ثابت AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    ثابت VNUser1_اندريون_Use m_user1_اندريون_Use;
    ثابت VNUser2_اندريون_Use m_user2_اندريون_Use;

عام:
    استعمال ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    استعمال Protected_Var_Ref_سيٽ = std::unordered_سيٽ<ثابت AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    استعمال Built_Var_Scope_سيٽ = std::unordered_سيٽ<ثابت AstVarScope*>;

خاص:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::ويڪٽر<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, صحيح> m_var_منزل_Id;
    std::unordered_سيٽ<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::ويڪٽر<ForceInfo*>> m_rhs_Dep_منزل_Forces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    منطقي m_doing_سونپيو = خطأ;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    Protected_Var_Ref_سيٽ& m_permanentlyProtected;
    Built_Var_Scope_سيٽ& m_forceRdUpdateBuilt;

عام:
    ForceState(منطقي doing_سونپيو, ForceHelperVarsByVar& forceHelperVarsByVar,
               Protected_Var_Ref_سيٽ& permanentlyProtected, Built_Var_Scope_سيٽ& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doing_سونپيو{doing_سونپيو}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    ساكن AstConst* makeZeroConst(Ast_ڳنڍ* nodep, صحيح ويڪر) {
        V3_عدد zero{nodep, ويڪر};
        zero.سيٽ_All_Bits0();
        أرجع جديد AstConst{nodep->fileline(), zero};
    }

    ساكن AstConst* makeConst32(File_سٽ* flp, صحيح قدر) {
        أرجع جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, static_cast<uint32_t>(قدر)};
    }

    ساكن AstConst* make_Range_نقاب_Const(Ast_ڳنڍ* nodep, صحيح ويڪر, صحيح lsb, صحيح msb) {
        V3_عدد نقاب{nodep, ويڪر};
        نقاب.سيٽ_All_Bits0();
        لكل (صحيح ٻٽو_انگ_2 = lsb; ٻٽو_انگ_2 <= msb; ++ٻٽو_انگ_2) نقاب.سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 1);
        أرجع جديد AstConst{nodep->fileline(), نقاب};
    }

    ساكن Ast_ڳنڍ_اظهار* zero_Pad_منزل_بنياد_ويڪر(Ast_ڳنڍ_اظهار* exprp, صحيح بنياد_ويڪر, صحيح padLsb,
                                           صحيح padMsb) {
        إذا (بنياد_ويڪر <= 0) أرجع exprp;
        ثابت صحيح هيٺيون_Pad = padLsb;
        ثابت صحيح مٿاهون_Pad = بنياد_ويڪر - (padMsb + 1);
        إذا (هيٺيون_Pad > 0) {
            exprp = جديد AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, هيٺيون_Pad)};
        }
        إذا (مٿاهون_Pad > 0) {
            exprp = جديد AstConcat{exprp->fileline(), makeZeroConst(exprp, مٿاهون_Pad), exprp};
        }
        أرجع exprp;
    }

    ساكن منطقي isUnpackedArrayDType(ثابت Ast_ڳنڍ_DType* dtypep) {
        أرجع VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    ساكن منطقي isBitwiseDType(ثابت Ast_ڳنڍ* nodep) {
        ثابت AstBasicDType* ثابت basicp = nodep->dtypep()->skipRefp()->basicp();
        أرجع basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    ساكن Ast_ڳنڍ_اظهار* cast_منزل_ڳنڍ_DType(Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ* dtypeFromp) {
        ثابت Ast_ڳنڍ_DType* ثابت dtypep = dtypeFromp->dtypep()->skipRefp();
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();
        إذا (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            أرجع exprp;
        }
        أرجع جديد AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    ساكن فراغ markNonReplaceable(AstVarRef* ثابت nodep) { nodep->user1_سيٽ_Once(); }
    منطقي isNotReplaceable(ثابت AstVarRef* ثابت nodep) ثابت {
        أرجع nodep->user1() || m_permanentlyProtected.ڳڻپ(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    فراغ markPermanentlyProtected(AstVarRef* ثابت nodep) ثابت {
        markNonReplaceable(nodep);
        m_permanentlyProtected.وجهو(nodep);
    }

    ساكن std::ويڪٽر<ForceInfo*> force_Infos_اندريون_Id_ترتيب(VarForceInfo& info) {
        std::ويڪٽر<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.سائيز());
        لكل (خودڪار& it : info.m_forces) forceps.ڌڪو_پٺ(&it.ٻيو);
        std::ترتيب_ڏيو(forceps.begin(), forceps.پڇاڙي(), [](ثابت ForceInfo* ap, ثابت ForceInfo* bp) {
            أرجع ap->m_forceId < bp->m_forceId;
        });
        أرجع forceps;
    }

    ساكن std::ويڪٽر<ثابت ForceInfo*> force_Infos_اندريون_Id_ترتيب(ثابت VarForceInfo& info) {
        std::ويڪٽر<ثابت ForceInfo*> forceps;
        forceps.reserve(info.m_forces.سائيز());
        لكل (ثابت خودڪار& it : info.m_forces) forceps.ڌڪو_پٺ(&it.ٻيو);
        std::ترتيب_ڏيو(forceps.begin(), forceps.پڇاڙي(), [](ثابت ForceInfo* ap, ثابت ForceInfo* bp) {
            أرجع ap->m_forceId < bp->m_forceId;
        });
        أرجع forceps;
    }

    ساكن منطقي is_Opaque_رستو_Selector(ثابت Ast_ڳنڍ* nodep) {
        أرجع VN_IS(nodep, Sel) || VN_IS(nodep, ڳنڍ_Sel) || VN_IS(nodep, StructSel);
    }

    ساكن منطقي is_Outermost_Opaque_رستو_Selector(ثابت Ast_ڳنڍ* nodep) {
        إذا (!is_Opaque_رستو_Selector(nodep)) أرجع خطأ;
        ثابت Ast_ڳنڍ* ثابت backp = nodep->backp();
        أرجع !backp || !is_Opaque_رستو_Selector(backp);
    }

    ساكن AstVarRef* getOneVarRef(Ast_ڳنڍ_اظهار* forceStmtp) {
        Ast_ڳنڍ* ثابت basep = forceStmtp->بنياد_Fromp(سچ);
        إذا (AstSampled* sampledp = VN_CAST(basep, Sampled))
            إذا (Ast_ڳنڍ_اظهار* exprp = VN_CAST(sampledp->exprp(), ڳنڍ_اظهار))
                أرجع getOneVarRef(exprp);
        AstVarRef* ثابت varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        أرجع varRefp;
    }

    ساكن Ast_ڳنڍ_اظهار* buildNestedArraySel(File_سٽ* flp, Ast_ڳنڍ_اظهار* fromp,
                                            ثابت std::ويڪٽر<صحيح>& indicies) {
        Ast_ڳنڍ_اظهار* curp = fromp;
        لكل (ثابت صحيح idx : indicies) curp = جديد AstArraySel{flp, curp, idx};
        أرجع curp;
    }

    سانچو <قسم_نالو Fn>
    ساكن Ast_ڳنڍ_Stmt* foreach_Unpacked_پن(ثابت std::ويڪٽر<AstUnpackArrayDType*>& dims,
                                            Fn build_پن) {
        Ast_ڳنڍ_Stmt* headp = خالي_اشارو;
        Ast_ڳنڍ_Stmt* tailp = خالي_اشارو;
        إذا (dims.خالي_جايون()) أرجع خالي_اشارو;
        صحيح ڪل = 1;
        لكل (ثابت AstUnpackArrayDType* ثابت d : dims) ڪل *= d->elementsConst();
        إذا (ڪل <= 0) أرجع خالي_اشارو;
        std::ويڪٽر<صحيح> idx(dims.سائيز(), 0);
        لكل (صحيح flat = 0; flat < ڪل; ++flat) {
            Ast_ڳنڍ_Stmt* ثابت stmtp = build_پن(idx, flat);
            إذا (!headp) {
                headp = stmtp;
            } وإلا {
                tailp->جوڙيو_ايندڙ(stmtp);
            }
            tailp = stmtp;
            لكل (صحيح d = static_cast<صحيح>(dims.سائيز()) - 1; d >= 0; --d) {
                إذا (++idx[d] < dims[d]->elementsConst()) اكسر;
                idx[d] = 0;
            }
        }
        أرجع headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_ڳنڍ_اظهار* lhsp, AstVar* varp,
                                     منطقي requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.خالي_جايون();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->ويڪر() - 1) : 0;

        إذا (ثابت AstSel* ثابت outerSelp = VN_CAST(lhsp, Sel)) {
            صحيح ڪل_Lsb = 0;
            لكل (Ast_ڳنڍ_اظهار* curp = lhsp; ثابت AstSel* ثابت selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                إذا (requireConstRangeSelect) {
                    UASSERT_OBJ(VN_IS(selp->lsbp(), Const), lhsp,
                                "Unsupported: force on non-const range select");
                }
                ڪل_Lsb += selp->lsbConst();
            }
            info.m_padLsb = ڪل_Lsb;
            info.m_padMsb = ڪل_Lsb + outerSelp->ويڪر_Const() - 1;
        }

        info.m_rangeLsb = info.m_padLsb;
        info.m_rangeMsb = info.m_padMsb;
        إذا (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::ويڪٽر<صحيح> indices;
            indices.reserve(info.m_arrayInfo.m_sels.سائيز());
            لكل (AstArraySel* ثابت selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.ڌڪو_پٺ(VN_AS(selp->bitp(), Const)->منزل_SInt());
            }
            info.m_rangeLsb = flatten_اشاريو(indices, arraySelDimSizes(info.m_arrayInfo));

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
            ثابت صحيح اشاريو = varInfo.get_Or_Create_Force_رستو_اشاريو(lhsp);
            info.m_rangeLsb = اشاريو;
            info.m_rangeMsb = اشاريو;
        }
        أرجع info;
    }

    Ast_ڳنڍ_اظهار* جوڙيو_Rhs_قدر_Reads(ثابت VarForceInfo& varInfo, Ast_ڳنڍ_اظهار* exprp) ثابت {
        إذا (!doing_سونپيو()) أرجع exprp;

        ثابت std::ويڪٽر<ثابت ForceInfo*> forceps = force_Infos_اندريون_Id_ترتيب(varInfo);
        إذا (forceps.خالي_جايون()) أرجع exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* ثابت cexprp = جديد AstCExpr{exprp->fileline(), AstCExpr::خالص{}};
        cexprp->dtypeFrom(exprp);
        cexprp->جوڙيو("(");
        لكل (ثابت ForceInfo* ثابت finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* ثابت refp
                = جديد AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->جوڙيو("(void)(");
            cexprp->جوڙيو(refp);
            cexprp->جوڙيو("), ");
        }
        cexprp->جوڙيو(exprp);
        cexprp->جوڙيو(")");
        أرجع cexprp;
    }

    Ast_ڳنڍ_اظهار* createForceReadCall(ثابت VarForceInfo& varInfo, File_سٽ* flp, VCMethod طريقو,
                                     Ast_ڳنڍ_اظهار* originalExprp, Ast_ڳنڍ* dtypeFromp,
                                     Ast_ڳنڍ_اظهار* اشاريو_Exprp) ثابت {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_ڳنڍ_اظهار* ثابت origValp
            = جوڙيو_Rhs_قدر_Reads(varInfo, cast_منزل_ڳنڍ_DType(originalExprp, dtypeFromp));

        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, طريقو, origValp};
        إذا (اشاريو_Exprp) callp->جوڙيو_Pinsp(اشاريو_Exprp);
        callp->dtypeFrom(dtypeFromp);
        أرجع callp;
    }

    Ast_ڳنڍ_اظهار* create_Force_Rd_اظهار(ثابت VarForceInfo& varInfo, File_سٽ* flp) ثابت {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        إذا (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            إذا (doing_سونپيو()) { أرجع جديد AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3_Error_ڪوڊ::UNOPTFLAT, سچ);
            AstVarRef* ثابت بنياد_Refp = جديد AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(بنياد_Refp);
            أرجع createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, بنياد_Refp, varp,
                                       خالي_اشارو);
        }
        AstVarRef* ثابت بنياد_Refp = جديد AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(بنياد_Refp);
        Ast_ڳنڍ_اظهار* ثابت enRefp = جديد AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_ڳنڍ_اظهار* ثابت valRefp = جديد AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        إذا (isBitwiseDType(varp)) {
            أرجع جديد AstOr{
                flp, جديد AstAnd{flp, enRefp, valRefp},
                جديد AstAnd{flp, جديد AstNot{flp, enRefp->clone_وڻ_خالص(خطأ)}, بنياد_Refp}};
        }
        أرجع جديد AstCond{flp, enRefp, valRefp, بنياد_Refp};
    }

    Ast_ڳنڍ_Stmt* createForceRdUpdateStmt(ثابت VarForceInfo& varInfo) ثابت {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        إذا (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            أرجع createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_سٽ* ثابت flp = varInfo.m_varVscp->fileline();
        أرجع جديد Ast_سونپيو{flp, جديد AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             create_Force_Rd_اظهار(varInfo, flp)};
    }

    Ast_ڳنڍ_Stmt* createForceRdUpdateStmtUnpacked(ثابت VarForceInfo& varInfo) ثابت {
        File_سٽ* ثابت flp = varInfo.m_varVscp->fileline();
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* ثابت arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت std::ويڪٽر<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_ڳنڍ_Stmt* ثابت stmtsp = foreach_Unpacked_پن(
            dims, [&](ثابت std::ويڪٽر<صحيح>& idx, صحيح /*flat*/) -> Ast_ڳنڍ_Stmt* {
                AstVarRef* ثابت بنياد_Refp = جديد AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(بنياد_Refp);
                Ast_ڳنڍ_اظهار* ثابت بنياد_Selp = buildNestedArraySel(flp, بنياد_Refp, idx);
                Ast_ڳنڍ_اظهار* ثابت enSelp = buildNestedArraySel(
                    flp, جديد AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_ڳنڍ_اظهار* ثابت valSelp = buildNestedArraySel(
                    flp, جديد AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_ڳنڍ_اظهار* ثابت readExprp = جديد AstOr{
                    flp, جديد AstAnd{flp, enSelp, valSelp},
                    جديد AstAnd{flp, جديد AstNot{flp, enSelp->clone_وڻ_خالص(خطأ)}, بنياد_Selp}};
                Ast_ڳنڍ_اظهار* ثابت rdLhsSelp = buildNestedArraySel(
                    flp, جديد AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                أرجع جديد Ast_سونپيو{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        لكل (ثابت ForceInfo* ثابت finfop : force_Infos_اندريون_Id_ترتيب(varInfo)) {
            إذا (!finfop->m_isExternal) استمر;
            stmtsp->جوڙيو_ايندڙ(جديد Ast_سونپيو{flp,
                                          جديد AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_وڻ_خالص(خطأ)});
        }
        أرجع stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        ثابت خودڪار it = m_var_منزل_Id.ڳوليو(vscp);
        إذا (it != m_var_منزل_Id.پڇاڙي()) أرجع m_varInfos[it->ٻيو];

        m_var_منزل_Id.emplace(vscp, m_varInfos.سائيز());
        m_varInfos.emplace_پٺ();
        VarForceInfo& info = m_varInfos.پٺ();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* ثابت varp = info.m_varp;
        إذا (!varp->isForceable()) أرجع info;

        File_سٽ* ثابت flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        ثابت منطقي helperVarsBuilt = helperVars.m_rdVarp != خالي_اشارو;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != خالي_اشارو)
                        && helperVarsBuilt == (helperVars.m_valVarp != خالي_اشارو),
                    varp, "Incomplete force helper set");
        إذا (!helperVarsBuilt) {
            ثابت منطقي unpacked = isUnpackedArrayDType(varp->dtypep());
            ثابت VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_ڳنڍ_DType* ثابت enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->ڳوليو_ٻٽو_انگ_DType();
            helperVars.m_rdVarp
                = جديد AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sig_عوامي(سچ);
            helperVars.m_enVarp
                = جديد AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(سچ);
            helperVars.m_valVarp
                = جديد AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(سچ);
            varp->جوڙيو_ايندڙ_هتي(helperVars.m_rdVarp);
            varp->جوڙيو_ايندڙ_هتي(helperVars.m_enVarp);
            varp->جوڙيو_ايندڙ_هتي(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = ڳوليو_Scope_Var(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = ڳوليو_Scope_Var(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = ڳوليو_Scope_Var(info.m_scopep, helperVars.m_valVarp);
        إذا (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } وإلا {
            info.m_forceRdVscp = جديد AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = جديد AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = جديد AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->جوڙيو_Varsp(info.m_forceRdVscp);
            info.m_scopep->جوڙيو_Varsp(info.m_forceEnVscp);
            info.m_scopep->جوڙيو_Varsp(info.m_forceValVscp);
        }
        أرجع info;
    }

    فراغ markClockedWrite(AstVar* varp) { m_clockedWrites.وجهو(varp); }
    منطقي hasClockedWrite(AstVar* varp) ثابت { أرجع m_clockedWrites.ڳڻپ(varp); }

    منطقي doing_سونپيو() ثابت { أرجع m_doing_سونپيو; }

    ثابت VarForceInfo* getVarInfo(AstVarScope* vscp) ثابت {
        ثابت خودڪار it = m_var_منزل_Id.ڳوليو(vscp);
        أرجع it != m_var_منزل_Id.پڇاڙي() ? &m_varInfos[it->ٻيو] : خالي_اشارو;
    }

    AstVarScope* ڳوليو_Scope_Var(AstScope* scopep, ثابت AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        إذا (cache.خالي_جايون()) {
            لكل (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        ثابت خودڪار it = cache.ڳوليو(varp);
        أرجع it != cache.پڇاڙي() ? it->ٻيو : خالي_اشارو;
    }
    فراغ جوڙيو_Force_Assignment(AstVar* varp, AstVarScope* vscp, Ast_ڳنڍ_اظهار* rhsExprp,
                            Ast_سونپيو_Force* forceStmtp, صحيح rangeLsb, صحيح rangeMsb, صحيح padLsb,
                            صحيح padMsb, منطقي hasArraySel, منطقي skipRdRefresh = خطأ) {
        v3Global.سيٽ_استعمال_ڪري_Force();
        varp->سيٽ_Forced_By_ڪوڊ();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        ثابت صحيح forceId = info.m_forces.سائيز();
        File_سٽ* ثابت flp = varp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        إذا (!info.m_forceVecVscp) {
            AstCDType* ثابت forceVecDtypep = جديد AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->جوڙيو_Typesp(forceVecDtypep);

            AstVar* ثابت forceVecVarp
                = جديد AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doing_سونپيو ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(خطأ);
            forceVecVarp->is_اندروني(سچ);
            varp->جوڙيو_ايندڙ_هتي(forceVecVarp);
            info.m_forceVecVscp = جديد AstVarScope{flp, scopep, forceVecVarp};
            scopep->جوڙيو_Varsp(info.m_forceVecVscp);
        }

        خودڪار pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, خالي_اشارو, rhsExprp});
        ForceInfo& finfo = pair.پهريون->ٻيو;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        إذا (doing_سونپيو()) {
            std::ويڪٽر<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* ثابت refp) {
                إذا (!refp->access().isReadOnly()) أرجع;
                AstVar* ثابت depVarp = refp->varp();
                إذا (depVarp
                    && std::ڳوليو(depVarps.begin(), depVarps.پڇاڙي(), depVarp) == depVarps.پڇاڙي()) {
                    depVarps.ڌڪو_پٺ(depVarp);
                }
            });
            لكل (AstVar* ثابت depVarp : depVarps) m_rhs_Dep_منزل_Forces[depVarp].ڌڪو_پٺ(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    ساكن فراغ collectArraySelInfo(Ast_ڳنڍ_اظهار* exprp, ArraySelInfo& info) {
        إذا (ثابت خودڪار* ثابت selp = VN_CAST(exprp, Sel)) {
            info.m_has_ٻٽو_انگ_Sel = سچ;
            collectArraySelInfo(selp->fromp(), info);
        } وإلا إذا (خودڪار* ثابت selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.ڌڪو_پٺ(selp);
        } وإلا إذا (ثابت خودڪار* ثابت memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    ساكن ArraySelInfo getArraySelInfo(Ast_ڳنڍ_اظهار* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        أرجع info;
    }

    ساكن std::ويڪٽر<صحيح> arraySelDimSizes(ثابت ArraySelInfo& info) {
        std::ويڪٽر<صحيح> dims;
        dims.reserve(info.m_sels.سائيز());
        لكل (AstArraySel* ثابت selp : info.m_sels) {
            Ast_ڳنڍ_DType* ثابت dtypep = selp->fromp()->dtypep()->skipRefp();
            ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.ڌڪو_پٺ(arrayp->declRange().elements());
        }
        أرجع dims;
    }

    ساكن صحيح flatten_اشاريو(ثابت std::ويڪٽر<صحيح>& indices, ثابت std::ويڪٽر<صحيح>& dimSizes) {
        UASSERT(indices.سائيز() == dimSizes.سائيز(), "Array index and dimension size mismatch");
        صحيح اشاريو = 0;
        صحيح stride = 1;
        لكل (صحيح i = static_cast<صحيح>(indices.سائيز()) - 1; i >= 0; --i) {
            اشاريو += indices[i] * stride;
            stride *= dimSizes[i];
        }
        أرجع اشاريو;
    }

    ساكن Ast_ڳنڍ_اظهار* build_Flatten_اشاريو_اظهار(File_سٽ* flp, ثابت ArraySelInfo& info) {
        ثابت std::ويڪٽر<صحيح> dimSizes = arraySelDimSizes(info);
        منطقي allConst = سچ;
        لكل (AstArraySel* ثابت selp : info.m_sels) {
            إذا (!VN_IS(selp->bitp(), Const)) {
                allConst = خطأ;
                اكسر;
            }
        }
        إذا (allConst) {
            std::ويڪٽر<صحيح> constIndices;
            constIndices.reserve(info.m_sels.سائيز());
            لكل (AstArraySel* ثابت selp : info.m_sels) {
                constIndices.ڌڪو_پٺ(VN_AS(selp->bitp(), Const)->منزل_SInt());
            }
            أرجع makeConst32(flp, flatten_اشاريو(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_ڳنڍ_اظهار* resultp = خالي_اشارو;
        صحيح stride = 1;
        لكل (صحيح i = static_cast<صحيح>(info.m_sels.سائيز()) - 1; i >= 0; --i) {
            Ast_ڳنڍ_اظهار* termp = info.m_sels[i]->bitp()->clone_وڻ_خالص(خطأ);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            إذا (termp->ويڪر() < 32) termp = جديد AstExtend{flp, termp, 32};
            إذا (stride != 1) termp = جديد AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? جديد Ast_جوڙيو{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        أرجع resultp;
    }

    ساكن Ast_ڳنڍ_اظهار* build_Rhs_معلومات_اظهار(File_سٽ* flp, ثابت ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        أرجع جديد AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    فراغ finalizeRhsVars() {
        لكل (VarForceInfo& info : m_varInfos) {
            AstVar* ثابت varp = info.m_varp;
            إذا (info.m_forces.خالي_جايون()) استمر;

            AstScope* ثابت scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_سٽ* ثابت flp = varp->fileline();
            ثابت std::ويڪٽر<ForceInfo*> forceps = force_Infos_اندريون_Id_ترتيب(info);

            لكل (ForceInfo* ثابت finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* ثابت rhsVarp = جديد AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doing_سونپيو() ? "_VassignRHS" : "__VforceRHS")
                        + std::منزل_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(سچ);
                rhsVarp->sig_عوامي(سچ);
                rhsVarp->سيٽ_Forced_By_ڪوڊ();
                varp->جوڙيو_ايندڙ_هتي(rhsVarp);
                finfo.m_rhsVarVscp = جديد AstVarScope{flp, scopep, rhsVarp};
                scopep->جوڙيو_Varsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                //پڇاڙي
                Ast_سونپيو* ثابت rhsAssignp = جديد Ast_سونپيو{
                    flp, جديد AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                إذا (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* ثابت touchCallp = جديد AstCMethodHard{
                        flp, جديد AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtype_سيٽ_Void();
                    Ast_ڳنڍ_Stmt* ثابت touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->جوڙيو_ايندڙ_هتي(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* ثابت alwaysp
                    = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, خالي_اشارو, rhsAssignp};
                Ast_Sen_وڻ* ثابت senTreep
                    = جديد Ast_Sen_وڻ{flp, جديد Ast_Sen_شي{flp, Ast_Sen_شي::Combo{}}};
                Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{flp, "force-rhs-update", senTreep};
                activep->sen_وڻ_Storep(activep->sentreep());
                activep->جوڙيو_Stmtsp(alwaysp);
                scopep->جوڙيو_Blocksp(activep);
            }

            إذا (info.m_forceRdVscp) {
                إذا (!m_forceRdUpdateBuilt.وجهو(info.m_varVscp).ٻيو) استمر;
                Ast_سرگرم* ثابت سرگرم_Initp = جديد Ast_سرگرم{
                    flp, "force-init",
                    جديد Ast_Sen_وڻ{flp, جديد Ast_Sen_شي{flp, Ast_Sen_شي::Static{}}}};
                سرگرم_Initp->sen_وڻ_Storep(سرگرم_Initp->sentreep());
                Ast_ڳنڍ_Stmt* initStmtp = خالي_اشارو;
                إذا (AstUnpackArrayDType* ثابت arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ثابت std::ويڪٽر<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    ثابت صحيح inner_ويڪر = dims.پٺ()->subDTypep()->skipRefp()->ويڪر();
                    initStmtp = foreach_Unpacked_پن(
                        dims, [&](ثابت std::ويڪٽر<صحيح>& idx, صحيح /*flat*/) -> Ast_ڳنڍ_Stmt* {
                            Ast_ڳنڍ_اظهار* ثابت lhsp = buildNestedArraySel(
                                flp, جديد AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            أرجع جديد Ast_سونپيو{flp, lhsp, makeZeroConst(varp, inner_ويڪر)};
                        });
                } وإلا {
                    initStmtp = جديد Ast_سونپيو{
                        flp, جديد AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->ويڪر())};
                }
                initStmtp->جوڙيو_ايندڙ(createForceRdUpdateStmt(info));
                سرگرم_Initp->جوڙيو_Stmtsp(جديد AstInitial{flp, initStmtp});
                scopep->جوڙيو_Blocksp(سرگرم_Initp);

                Ast_Sen_شي* itemsp = خالي_اشارو;
                خودڪار جوڙيو_Sen_شي = [&](AstVarScope* vscp) {
                    إذا (!vscp) أرجع;
                    Ast_Sen_شي* ثابت nextp = جديد Ast_Sen_شي{
                        flp, VEdgeType::ET_بدليو, جديد AstVarRef{flp, vscp, VAccess::READ}};
                    إذا (itemsp) {
                        itemsp->جوڙيو_ايندڙ(nextp);
                    } وإلا {
                        itemsp = nextp;
                    }
                };
                جوڙيو_Sen_شي(info.m_forceEnVscp);
                جوڙيو_Sen_شي(info.m_forceValVscp);
                AstVarRef* ثابت origSenRefp = جديد AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                Ast_Sen_شي* ثابت origItemp
                    = جديد Ast_Sen_شي{flp, VEdgeType::ET_بدليو, origSenRefp};
                إذا (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->جوڙيو_ايندڙ(origItemp);
                لكل (ForceInfo* ثابت finfop : forceps) {
                    إذا (!finfop->m_isExternal) جوڙيو_Sen_شي(finfop->m_rhsVarVscp);
                }

                Ast_سرگرم* ثابت activep
                    = جديد Ast_سرگرم{flp, "force-rd-update", جديد Ast_Sen_وڻ{flp, itemsp}};
                activep->sen_وڻ_Storep(activep->sentreep());
                activep->جوڙيو_Stmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS, خالي_اشارو,
                                                 createForceRdUpdateStmt(info)});
                scopep->جوڙيو_Blocksp(activep);
            }
        }
    }

    Ast_ڳنڍ* createRhsUpdatesForWrite(File_سٽ* flp, AstVar* writtenVarp) ثابت {
        إذا (!doing_سونپيو()) أرجع خالي_اشارو;

        ثابت خودڪار it = m_rhs_Dep_منزل_Forces.ڳوليو(writtenVarp);
        إذا (it == m_rhs_Dep_منزل_Forces.پڇاڙي()) أرجع خالي_اشارو;

        Ast_ڳنڍ* headp = خالي_اشارو;
        Ast_ڳنڍ* tailp = خالي_اشارو;
        لكل (ثابت ForceInfo* ثابت finfop : it->ٻيو) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            Ast_سونپيو* ثابت updatep
                = جديد Ast_سونپيو{flp, جديد AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_وڻ_خالص(خطأ)};
            إذا (tailp) {
                tailp->جوڙيو_ايندڙ_هتي(updatep);
            } وإلا {
                headp = updatep;
            }
            tailp = updatep;
        }
        أرجع headp;
    }

    ثابت ForceInfo& getForceInfo(Ast_سونپيو_Force* forceStmtp) ثابت {
        AstVarScope* ثابت vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        ثابت VarForceInfo* ثابت varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        خودڪار it2 = varInfo->m_forces.ڳوليو(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.پڇاڙي(), "Force statement not found");
        أرجع it2->ٻيو;
    }

    ساكن منطقي selOverlapsAnyForce(ثابت VarForceInfo& varInfo, صحيح selLsb, صحيح selMsb) {
        لكل (ثابت خودڪار& pair : varInfo.m_forces) {
            إذا (pair.ٻيو.m_rangeLsb <= selMsb && pair.ٻيو.m_rangeMsb >= selLsb) أرجع سچ;
        }
        أرجع خطأ;
    }

    Ast_ڳنڍ_اظهار* create_Force_Read_اظهار(ثابت VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) ثابت {
        File_سٽ* ثابت flp = originalRefp->fileline();
        AstVarRef* ثابت refp = originalRefp->clone_وڻ_خالص(خطأ);
        refp->access(VAccess::READ);
        أرجع createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   خالي_اشارو);
    }

    Ast_ڳنڍ_اظهار* create_Force_Read_اشاريو_اظهار(ثابت VarForceInfo& varInfo,
                                                Ast_ڳنڍ_اظهار* originalExprp,
                                                Ast_ڳنڍ_اظهار* اشاريو_Exprp) ثابت {
        File_سٽ* ثابت flp = originalExprp->fileline();
        Ast_ڳنڍ_اظهار* ثابت exprp = originalExprp->clone_وڻ_خالص(خطأ);
        // Must be an LValue to a static variable
        VN_AS(exprp->get_VAccess_هدف_Recurse(), VarRef)->access(VAccess::READ);
        أرجع createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_اشاريو, exprp, originalExprp,
                                   اشاريو_Exprp);
    }

    ساكن Ast_ڳنڍ_اظهار* rebuild_Sel_رستو(Ast_ڳنڍ_اظهار* pathp, Ast_ڳنڍ_اظهار* بنياد_Exprp) {
        إذا (ثابت AstSel* ثابت selp = VN_CAST(pathp, Sel)) {
            Ast_ڳنڍ_اظهار* ثابت fromp = rebuild_Sel_رستو(selp->fromp(), بنياد_Exprp);
            AstSel* ثابت outp
                = جديد AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->ويڪر_Const()};
            outp->dtypeFrom(selp);
            أرجع outp;
        }
        أرجع بنياد_Exprp;
    }
};

// Split deassign concat LHS before converting to release internals.
ساكن فراغ ورهايو_Deassign(AstDeassign* nodep) {
    AstConcat* ثابت concatp = VN_CAST(nodep->lhsp(), Concat);
    إذا (!concatp) أرجع;

    File_سٽ* ثابت flp = nodep->fileline();
    AstDeassign* ثابت newLp = جديد AstDeassign{flp, concatp->lhsp()->unlink_Fr_پٺ()};
    AstDeassign* ثابت newRp = جديد AstDeassign{flp, concatp->rhsp()->unlink_Fr_پٺ()};
    Ast_ڳنڍ_اظهار* ثابت conp = concatp->unlink_Fr_پٺ();
    nodep->replaceWith(newLp);
    newLp->جوڙيو_ايندڙ_هتي(newRp);
    VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    VL_DO_DANGLING(conp->delete_وڻ(), conp);

    ورهايو_Deassign(newLp);
    ورهايو_Deassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

صنف ForceDiscoveryVisitor final : عام VNVisitorConst {
    ForceState& m_state;
    منطقي m_اندريون_Clocked_سرگرم = خطأ;

    فراغ buildForceableUnpackedArray(AstVarScope* ثابت nodep,
                                     AstUnpackArrayDType* ثابت arrDtypep) {
        AstVar* ثابت varp = nodep->varp();
        ثابت std::ويڪٽر<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.خالي_جايون(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        ثابت Ast_ڳنڍ_DType* ثابت پن_Dtypep = dims.پٺ()->subDTypep()->skipRefp();
        ثابت AstBasicDType* ثابت innerBasicp = پن_Dtypep->basicp();
        إذا (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            أرجع;
        }

        File_سٽ* ثابت flp = varp->fileline();
        ثابت صحيح inner_ويڪر = پن_Dtypep->ويڪر();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* ثابت enVscp = info.m_forceEnVscp;
        AstVarScope* ثابت valVscp = info.m_forceValVscp;

        Ast_Sen_شي* ثابت itemsp = جديد Ast_Sen_شي{flp, VEdgeType::ET_بدليو,
                                                  جديد AstVarRef{flp, enVscp, VAccess::READ}};
        Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{flp, "force-update", جديد Ast_Sen_وڻ{flp, itemsp}};
        activep->sen_وڻ_Storep(activep->sentreep());

        Ast_ڳنڍ_Stmt* ثابت alwaysBodyHeadp = ForceState::foreach_Unpacked_پن(
            dims, [&](ثابت std::ويڪٽر<صحيح>& idx, صحيح flat) -> Ast_ڳنڍ_Stmt* {
                AstVarRef* ثابت origRefp = جديد AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_ڳنڍ_اظهار* ثابت origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_ڳنڍ_اظهار* ثابت enSelp = ForceState::buildNestedArraySel(
                    flp, جديد AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_ڳنڍ_اظهار* ثابت valSelp = ForceState::buildNestedArraySel(
                    flp, جديد AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_ڳنڍ_اظهار* ثابت forceExprp = جديد AstOr{
                    flp, جديد AstAnd{flp, enSelp, valSelp},
                    جديد AstAnd{flp, جديد AstNot{flp, enSelp->clone_وڻ_خالص(خطأ)}, origSelp}};
                Ast_ڳنڍ_اظهار* ثابت lhsSelp = ForceState::buildNestedArraySel(
                    flp, جديد AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                Ast_سونپيو_Force* ثابت forceAssignp = جديد Ast_سونپيو_Force{flp, lhsSelp, forceExprp};
                forceAssignp->user2(سچ);

                Ast_ڳنڍ_اظهار* ثابت rhsClonep = forceExprp->clone_وڻ_خالص(خطأ);
                rhsClonep->foreach([varp](AstVarRef* ثابت r) {
                    إذا (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.جوڙيو_Force_Assignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/inner_ويڪر - 1,
                                           /*hasArraySel=*/سچ, /*skipRdRefresh=*/سچ);
                أرجع forceAssignp;
            });
        activep->جوڙيو_Stmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS, خالي_اشارو, alwaysBodyHeadp});
        nodep->scopep()->جوڙيو_Blocksp(activep);
    }

    فراغ گهمو(Ast_سونپيو_Force* nodep) override {
        إذا (nodep->user2()) أرجع;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* ثابت forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, سچ);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_ڳنڍ_اظهار* ثابت rhsExprp = nodep->rhsp()->clone_وڻ_خالص(خطأ);

        m_state.جوڙيو_Force_Assignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    فراغ گهمو(Ast_سونپيو* nodep) override {
        إذا (m_state.doing_سونپيو() && m_اندريون_Clocked_سرگرم) {
            إذا (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    فراغ گهمو(Ast_سرگرم* nodep) override {
        VL_RESTORER(m_اندريون_Clocked_سرگرم);
        m_اندريون_Clocked_سرگرم = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    فراغ گهمو(AstVarScope* nodep) override {
        إذا (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            إذا (m_state.doing_سونپيو()) {
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
            File_سٽ* ثابت flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* ثابت enVscp = info.m_forceEnVscp;
            AstVarScope* ثابت valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            Ast_Sen_شي* ثابت itemsp = جديد Ast_Sen_شي{flp, VEdgeType::ET_بدليو,
                                                      جديد AstVarRef{flp, enVscp, VAccess::READ}};
            Ast_سرگرم* ثابت activep
                = جديد Ast_سرگرم{flp, "force-update", جديد Ast_Sen_وڻ{flp, itemsp}};
            activep->sen_وڻ_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* ثابت refp = جديد AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* ثابت enRefp = جديد AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* ثابت valRefp = جديد AstVarRef{flp, valVscp, VAccess::READ};
            ثابت AstBasicDType* ثابت basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_ڳنڍ_اظهار* ثابت forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstOr{
                            flp, جديد AstAnd{flp, enRefp, valRefp},
                            جديد AstAnd{flp, جديد AstNot{flp, enRefp->clone_وڻ_خالص(خطأ)}, refp}})
                      : static_cast<Ast_ڳنڍ_اظهار*>(جديد AstCond{flp, enRefp, valRefp, refp});
            Ast_سونپيو_Force* ثابت forceAssignp
                = جديد Ast_سونپيو_Force{flp, جديد AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(سچ);
            activep->جوڙيو_Stmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS, خالي_اشارو, forceAssignp});
            nodep->scopep()->جوڙيو_Blocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_ڳنڍ_اظهار* ثابت rhsClonep = forceExprp->clone_وڻ_خالص(خطأ);
            rhsClonep->foreach([varp](AstVarRef* ثابت refp) {
                إذا (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            ثابت منطقي bitwiseVar = ForceState::isBitwiseDType(varp);
            ثابت صحيح padMsb = bitwiseVar ? (varp->ويڪر() - 1) : 0;
            صحيح rangeLsb = 0;
            صحيح rangeMsb = padMsb;
            إذا (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.جوڙيو_Force_Assignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, خطأ);
        }
        iterateChildrenConst(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildrenConst(nodep); }

عام:
    واضح ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_ايندڙ_Const_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

صنف ForceConvertVisitor final : عام VNVisitor {
    ForceState& m_state;

    فراغ گهمو(Ast_سونپيو_Force* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp();
        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ثابت forcedVarp = lhsVarRefp->varp();

        ثابت ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        ثابت ForceState::VarForceInfo* ثابت varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_سٽ* ثابت flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        Ast_سونپيو* ثابت rhsAssignp
            = جديد Ast_سونپيو{flp, جديد AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_وڻ_خالص(خطأ)};

        Ast_سونپيو* valAssignp = خالي_اشارو;
        Ast_سونپيو* enAssignp = خالي_اشارو;
        ثابت منطقي bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        إذا (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_ڳنڍ_اظهار* بنياد_Exprp = nodep->rhsp()->clone_وڻ_خالص(خطأ);
            بنياد_Exprp->foreach(
                [](AstVarRef* ثابت refp) { ForceState::markNonReplaceable(refp); });
            إذا (bitwiseForcedVar) {
                بنياد_Exprp = ForceState::zero_Pad_منزل_بنياد_ويڪر(بنياد_Exprp, forcedVarp->ويڪر(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            إذا (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* ثابت نقاب_Constp = ForceState::make_Range_نقاب_Const(
                    nodep, forcedVarp->ويڪر(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_ڳنڍ_اظهار* ثابت valReadp
                    = جديد AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_ڳنڍ_اظهار* ثابت valWritep
                    = جديد AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_ڳنڍ_اظهار* ثابت notMaskp = جديد AstNot{flp, نقاب_Constp};
                Ast_ڳنڍ_اظهار* ثابت maskedOldp = جديد AstAnd{flp, valReadp, notMaskp};
                Ast_ڳنڍ_اظهار* ثابت newValp = جديد AstOr{flp, maskedOldp, بنياد_Exprp};
                valAssignp = جديد Ast_سونپيو{flp, valWritep, newValp};

                Ast_ڳنڍ_اظهار* ثابت enReadp
                    = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_ڳنڍ_اظهار* ثابت enWritep
                    = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_ڳنڍ_اظهار* ثابت newEnp
                    = جديد AstOr{flp, enReadp, نقاب_Constp->clone_وڻ_خالص(خطأ)};
                enAssignp = جديد Ast_سونپيو{flp, enWritep, newEnp};
            } وإلا {
                AstConst* ثابت oneConstp = ForceState::make_Range_نقاب_Const(nodep, 1, 0, 0);
                Ast_ڳنڍ_اظهار* ثابت rhsValp = ForceState::cast_منزل_ڳنڍ_DType(بنياد_Exprp, forcedVarp);
                valAssignp = جديد Ast_سونپيو{
                    flp, جديد AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = جديد Ast_سونپيو{
                    flp, جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        ثابت AstSel* ثابت selLhsp = VN_CAST(lhsp, Sel);
        ثابت منطقي array_ٻٽو_انگ_Sel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_has_ٻٽو_انگ_Sel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->ويڪر();
        Ast_ڳنڍ_اظهار* ثابت rhsDatap = ForceState::build_Rhs_معلومات_اظهار(flp, info);
        AstCExpr* ثابت rhsAddrp = جديد AstCExpr{flp};
        rhsAddrp->جوڙيو("&(");
        rhsAddrp->جوڙيو(rhsDatap);
        rhsAddrp->جوڙيو(")");
        AstCMethodHard* ثابت جوڙيو_Force_Callp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_جوڙيو,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        جوڙيو_Force_Callp->جوڙيو_Pinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        جوڙيو_Force_Callp->جوڙيو_Pinsp(rhsAddrp);
        جوڙيو_Force_Callp->جوڙيو_Pinsp(
            ForceState::makeConst32(flp, array_ٻٽو_انگ_Sel ? info.m_padLsb : info.m_rangeLsb));
        إذا (array_ٻٽو_انگ_Sel) {
            جوڙيو_Force_Callp->جوڙيو_Pinsp(ForceState::makeConst32(flp, info.m_padLsb));
            جوڙيو_Force_Callp->جوڙيو_Pinsp(ForceState::makeConst32(flp, info.m_padMsb));
            جوڙيو_Force_Callp->جوڙيو_Pinsp(ForceState::makeConst32(flp, selLhsp->fromp()->ويڪر()));
        }
        جوڙيو_Force_Callp->dtype_سيٽ_Void();
        Ast_ڳنڍ_Stmt* ثابت stmtp = جوڙيو_Force_Callp->makeStmt();

        Ast_ڳنڍ* tailp = rhsAssignp;
        إذا (valAssignp) {
            tailp->جوڙيو_ايندڙ_هتي(valAssignp);
            tailp = valAssignp;
        }
        إذا (enAssignp) {
            tailp->جوڙيو_ايندڙ_هتي(enAssignp);
            tailp = enAssignp;
        }
        tailp->جوڙيو_ايندڙ_هتي(stmtp);
        إذا (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->جوڙيو_ايندڙ_هتي(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    فراغ گهمو(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp();
        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ثابت releasedVarp = lhsVarRefp->varp();

        ثابت ForceState::VarForceInfo* ثابت varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        إذا (!varInfo) {
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
            أرجع;
        }
        UASSERT_OBJ(!varInfo->m_forces.خالي_جايون(), nodep, "Var info for variable with no forces");

        File_سٽ* ثابت flp = nodep->fileline();

        ثابت ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, خطأ);

        ثابت AstSel* ثابت selLhsp = VN_CAST(lhsp, Sel);
        ثابت منطقي array_ٻٽو_انگ_Sel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_has_ٻٽو_انگ_Sel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->ويڪر();
        AstCMethodHard* ثابت releaseCallp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->جوڙيو_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        إذا (array_ٻٽو_انگ_Sel) {
            releaseCallp->جوڙيو_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->جوڙيو_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtype_سيٽ_Void();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_ڳنڍ_Stmt* ثابت releasep = releaseCallp->makeStmt();

        Ast_سونپيو* clearEnp = خالي_اشارو;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        إذا (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_ڳنڍ_اظهار* ثابت enWritep
                = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            إذا (ForceState::isBitwiseDType(releasedVarp)) {
                ثابت صحيح var_ويڪر = releasedVarp->ويڪر();
                إذا (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == var_ويڪر - 1) {
                    clearEnp
                        = جديد Ast_سونپيو{flp, enWritep, ForceState::makeZeroConst(nodep, var_ويڪر)};
                } وإلا {
                    // forceEn = forceEn & ~mask(range);
                    Ast_ڳنڍ_اظهار* ثابت enReadp
                        = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* ثابت نقاب_Const = ForceState::make_Range_نقاب_Const(
                        nodep, var_ويڪر, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_ڳنڍ_اظهار* ثابت newEnp
                        = جديد AstAnd{flp, enReadp, جديد AstNot{flp, نقاب_Const}};
                    clearEnp = جديد Ast_سونپيو{flp, enWritep, newEnp};
                }
            } وإلا {
                clearEnp = جديد Ast_سونپيو{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        ثابت AstSel* ثابت selp = VN_CAST(lhsp, Sel);
        Ast_ڳنڍ_اظهار* ثابت basep = selp ? selp->fromp() : lhsp;

        Ast_ڳنڍ* stmtListp = releasep;
        إذا (clearEnp) {
            clearEnp->جوڙيو_ايندڙ_هتي(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        ثابت منطقي ڀريل_Bitwise_Release
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->ويڪر() - 1;
        إذا (!releasedVarp->isContinuously()
            && !(m_state.doing_سونپيو() && m_state.hasClockedWrite(releasedVarp)
                 && ڀريل_Bitwise_Release)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            ثابت منطقي has_Opaque_رستو = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_ڳنڍ_اظهار* forceReadp = خالي_اشارو;
            إذا (has_Opaque_رستو) {
                forceReadp = m_state.create_Force_Read_اشاريو_اظهار(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } وإلا إذا (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.create_Force_Read_اشاريو_اظهار(
                    *varInfo, basep,
                    ForceState::build_Flatten_اشاريو_اظهار(flp, rangeInfo.m_arrayInfo));
                إذا (selp) { forceReadp = ForceState::rebuild_Sel_رستو(lhsp, forceReadp); }
            } وإلا {
                forceReadp
                    = selp ? ForceState::rebuild_Sel_رستو(
                                 lhsp, m_state.create_Force_Read_اظهار(*varInfo, lhsVarRefp))
                           : m_state.create_Force_Read_اظهار(*varInfo, lhsVarRefp);
            }
            Ast_سونپيو* ثابت assignp = جديد Ast_سونپيو{flp, lhsp->clone_وڻ_خالص(خطأ), forceReadp};
            assignp->جوڙيو_ايندڙ_هتي(stmtListp);
            stmtListp = assignp;
        }

        إذا (varInfo->m_forceRdVscp) stmtListp->جوڙيو_ايندڙ(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_ايندڙ_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

صنف ForceReplaceVisitor final : عام VNVisitor {
    ثابت ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_ڳنڍ_Stmt* m_stmtp = خالي_اشارو;
    منطقي m_اندريون_Logic = خطأ;

    فراغ iterateLogic(Ast_ڳنڍ* nodep) {
        VL_RESTORER(m_اندريون_Logic);
        m_اندريون_Logic = سچ;
        iterateChildren(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_سونپيو* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        إذا (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp()->بنياد_Fromp(سچ), VarRef)) {
            إذا (Ast_ڳنڍ* ثابت updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->جوڙيو_ايندڙ_هتي(updatep);
            }
        }
    }
    فراغ گهمو(Ast_سونپيو_Cont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate_And_ايندڙ_Null(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    فراغ گهمو(AstDeassign*) override {}
    فراغ گهمو(AstCFunc* nodep) override { iterateLogic(nodep); }
    فراغ گهمو(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    فراغ گهمو(Ast_ڳنڍ_Procedure* nodep) override { iterateLogic(nodep); }
    فراغ گهمو(AstAlways* nodep) override {
        إذا (nodep->keyword() == VAlwaysKwd::CONT_سونپيو) {
            iterateChildren(nodep);
            أرجع;
        }
        iterateLogic(nodep);
    }
    فراغ گهمو(Ast_Sen_شي* nodep) override { iterateLogic(nodep); }
    فراغ گهمو(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef);
        إذا (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            گهمو(static_cast<Ast_ڳنڍ*>(nodep));
            أرجع;
        }

        AstVar* ثابت varp = refp->varp();
        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(refp->varScopep());
        إذا (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.خالي_جايون()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            گهمو(static_cast<Ast_ڳنڍ*>(nodep));
            أرجع;
        }

        إذا (ثابت AstConst* ثابت lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            ثابت صحيح selLsb = lsbConstp->منزل_SInt();
            ثابت صحيح selMsb = selLsb + nodep->ويڪر() - 1;
            إذا (!varp->is_Sig_عوامي()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                گهمو(static_cast<Ast_ڳنڍ*>(nodep));
                أرجع;
            }
        }

        File_سٽ* ثابت flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* ثابت refClonep = refp->clone_وڻ_خالص(خطأ);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->ويڪر())};
        callp->جوڙيو_Pinsp(refClonep);
        callp->جوڙيو_Pinsp(nodep->lsbp()->clone_وڻ_خالص(خطأ));
        callp->جوڙيو_Pinsp(ForceState::makeConst32(flp, nodep->ويڪر()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstArraySel* nodep) override {
        إذا (ثابت AstArraySel* ثابت پٺ_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            إذا (پٺ_Selp->fromp() == nodep) {
                iterateChildren(nodep);
                أرجع;
            }
        }

        Ast_ڳنڍ* ثابت basep = nodep->بنياد_Fromp(سچ);
        AstVarRef* ثابت بنياد_Refp = VN_CAST(basep, VarRef);
        إذا (!بنياد_Refp) {
            iterateChildren(nodep);
            أرجع;
        }
        AstVar* ثابت varp = بنياد_Refp->varp();
        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(بنياد_Refp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        إذا (m_state.isNotReplaceable(بنياد_Refp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            أرجع;
        }

        إذا (!بنياد_Refp->access().isReadOnly()) {
            iterateChildren(nodep);
            أرجع;
        }
        إذا (m_state.doing_سونپيو() && varInfo->m_forceRdVscp) {
            بنياد_Refp->varp(varInfo->m_forceRdVscp->varp());
            بنياد_Refp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            أرجع;
        }
        ثابت ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        لكل (AstArraySel* ثابت selp : arrayInfo.m_sels) iterate_And_ايندڙ_Null(selp->bitp());
        Ast_ڳنڍ_اظهار* ثابت اشاريو_Exprp
            = ForceState::build_Flatten_اشاريو_اظهار(nodep->fileline(), arrayInfo);
        Ast_ڳنڍ_اظهار* ثابت readExprp
            = m_state.create_Force_Read_اشاريو_اظهار(*varInfo, nodep, اشاريو_Exprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    فراغ گهمو(AstVarRef* nodep) override {
        إذا (m_state.isNotReplaceable(nodep)) أرجع;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        إذا (ثابت AstArraySel* ثابت پٺ_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            إذا (پٺ_Selp->fromp() == nodep) أرجع;
        }

        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(nodep->varScopep());
        إذا (!varInfo) أرجع;

        إذا (varInfo->m_forceRdVscp) {
            إذا (nodep->access().isRW()) {
                إذا (m_اندريون_Logic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                أرجع;
            }
            إذا (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_ڳنڍ_اظهار* ثابت readp
                    = m_state.create_Force_Rd_اظهار(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                أرجع;
            }
            إذا (m_اندريون_Logic && m_stmtp) {
                m_stmtp->جوڙيو_ايندڙ_هتي(m_state.createForceRdUpdateStmt(*varInfo));
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

        UASSERT_OBJ(!varInfo->m_forces.خالي_جايون(), nodep, "Var info for variable with no forces");

        إذا (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } وإلا إذا (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_ڳنڍ_اظهار* ثابت readExprp = m_state.create_Force_Read_اظهار(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        }
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        إذا (ForceState::is_Outermost_Opaque_رستو_Selector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_ڳنڍ_اظهار* ثابت exprp = VN_AS(nodep, ڳنڍ_اظهار);
            Ast_ڳنڍ* ثابت basep = exprp->بنياد_Fromp(سچ);
            AstVarRef* ثابت بنياد_Refp = VN_CAST(basep, VarRef);
            إذا (بنياد_Refp) {
                AstVar* ثابت varp = بنياد_Refp->varp();
                إذا (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    ثابت ForceState::VarForceInfo* ثابت varInfo
                        = m_state.getVarInfo(بنياد_Refp->varScopep());
                    إذا (!m_state.isNotReplaceable(بنياد_Refp) && varInfo) {
                        ثابت صحيح force_رستو_اشاريو = varInfo->ڳوليو_Force_رستو_اشاريو(exprp);
                        إذا (force_رستو_اشاريو >= 0) {
                            إذا (!بنياد_Refp->access().isReadOnly()) أرجع;
                            Ast_ڳنڍ_اظهار* ثابت readExprp = m_state.create_Force_Read_اشاريو_اظهار(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), force_رستو_اشاريو));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                            أرجع;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

عام:
    واضح ForceReplaceVisitor(AstNetlist* nodep, ثابت ForceState& state)
        : m_state{state} {
        iterate_And_ايندڙ_Null(nodep->modulesp());
    }
    ~ForceReplaceVisitor() override {
        V3Stats::جوڙيو_Stat("Non-overlapping force sels", m_nonOverlappingForceSels);
    }
};
//######################################################################
//

//######################################################################
// V3Force - Main entry point

ساكن فراغ forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::Protected_Var_Ref_سيٽ& permanentlyProtected,
                         ForceState::Built_Var_Scope_سيٽ& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    إذا (!v3Global.has_Forceable_سگنل()) أرجع;
    ForceState state{خطأ, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_وڻ("force", 0, dump_وڻ_Either_Level() >= 3);
}

ساكن فراغ سونپيو_All_Impl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::Protected_Var_Ref_سيٽ& permanentlyProtected,
                          ForceState::Built_Var_Scope_سيٽ& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    إذا (!v3Global.has_سونپيو_Deassign()) أرجع;

    std::ويڪٽر<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.ڌڪو_پٺ(deassignp); });
    لكل (AstDeassign* ثابت deassignp : deassignps) ورهايو_Deassign(deassignp);

    std::ويڪٽر<Ast_سونپيو_Cont*> سونپيو_Contps;
    deassignps.clear();
    nodep->foreach([&](Ast_ڳنڍ_Stmt* nodep) {
        إذا (Ast_سونپيو_Cont* ثابت سونپيو_Contp = VN_CAST(nodep, سونپيو_Cont)) {
            سونپيو_Contps.ڌڪو_پٺ(سونپيو_Contp);
        } وإلا إذا (AstDeassign* ثابت deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.ڌڪو_پٺ(deassignp);
        }
    });

    لكل (Ast_سونپيو_Cont* ثابت assignp : سونپيو_Contps) {
        assignp->replaceWith(جديد Ast_سونپيو_Force{assignp->fileline(),
                                                assignp->lhsp()->unlink_Fr_پٺ(),
                                                assignp->rhsp()->unlink_Fr_پٺ()});
        VL_DO_DANGLING(assignp->delete_وڻ(), assignp);
    }
    لكل (AstDeassign* ثابت deassignp : deassignps) {
        deassignp->replaceWith(
            جديد AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_وڻ_خالص(سچ)});
        VL_DO_DANGLING(deassignp->delete_وڻ(), deassignp);
    }
    ForceState state{سچ, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_وڻ("assign-deassign", 0, dump_وڻ_Either_Level() >= 3);
}

فراغ V3Force::force_And_سونپيو_All(AstNetlist* nodep) {
    ثابت VNUser3_اندريون_Use user3_اندريون_Use;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::Protected_Var_Ref_سيٽ permanentlyProtected;
    ForceState::Built_Var_Scope_سيٽ forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    سونپيو_All_Impl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
