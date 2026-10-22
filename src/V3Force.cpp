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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Force.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3Stats.h"
#شامل "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

جماعت ForceState final {
عوامی:
    ساخت ForceRange VL_NOT_FINAL {
        صحیح m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        صحیح m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        صحیح m_padLsb = 0;  // Bit positions for RHS padding
        صحیح m_padMsb = 0;  // Bit positions for RHS padding
    };

    ساخت ForceInfo final : ForceRange {
        // MEMBERS
        صحیح m_forceId = 0;  // Unique (per signal) variable of this force assignment
        منطقی m_hasArraySel = جھوٹ;  // If this has an array select on LHS
        منطقی m_isExternal = جھوٹ;  // Synthetic force for the public force controls
        منطقی m_skipRdRefresh = جھوٹ;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = خالی_اشاریہ;  // Scope of the var containing RHSID
        Ast_گرہ_اظہاریہ* m_rhsExprp = خالی_اشاریہ;  // Expression on RHS of this force assignment

        ForceInfo() = طےشدہ;
        ForceInfo(صحیح rangeLsb, صحیح rangeMsb, صحیح padLsb, صحیح padMsb, صحیح forceId,
                  منطقی hasArraySel, AstVarScope* rhsVarVscp, Ast_گرہ_اظہاریہ* rhsExprp)
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

    ساخت VarForceInfo final {
        AstVarScope* m_forceVecVscp = خالی_اشاریہ;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = خالی_اشاریہ;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = خالی_اشاریہ;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = خالی_اشاریہ;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = خالی_اشاریہ;  // Scope of the forced (original) variable
        AstVar* m_varp = خالی_اشاریہ;  // The forced (original) variable
        AstScope* m_scopep = خالی_اشاریہ;  // Scope containing the forced variable
        std::unordered_map<Ast_تفویض_Force*, ForceInfo> m_forces;
        std::unordered_map<string, صحیح> m_force_راستہ_مقصد_اشاریہ;
        صحیح m_اگلا_Force_راستہ_اشاریہ = 1;  // Start at 1 so 0 can be the base path (whole signal)

    نجی:
        ساکن خالی build_Force_راستہ_کلید_Recurse(Ast_گرہ_اظہاریہ* nodep, string& برآمد) {
            اگر (VN_IS(nodep, VarRef)) واپس;
            اگر (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
                build_Force_راستہ_کلید_Recurse(selp->fromp(), برآمد);
                برآمد += "|S" + cvt_مقصد_Str(selp->lsbConst()) + ":" + cvt_مقصد_Str(selp->چوڑائی_Const());
                واپس;
            }
            اگر (AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
                Ast_گرہ_اظہاریہ* ثابت fromp = selp->fromp();
                build_Force_راستہ_کلید_Recurse(fromp, برآمد);
                برآمد += "|T" + selp->name();
                واپس;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        ساکن string force_راستہ_کلید(Ast_گرہ_اظہاریہ* nodep) {
            string برآمد;
            build_Force_راستہ_کلید_Recurse(nodep, برآمد);
            واپس برآمد;
        }

    عوامی:
        صحیح get_Or_Create_Force_راستہ_اشاریہ(Ast_گرہ_اظہاریہ* nodep) {
            ثابت خودکار pair
                = m_force_راستہ_مقصد_اشاریہ.emplace(force_راستہ_کلید(nodep), m_اگلا_Force_راستہ_اشاریہ);
            اگر (pair.دوسرا) ++m_اگلا_Force_راستہ_اشاریہ;
            واپس pair.پہلا->دوسرا;
        }

        صحیح ڈھونڈو_Force_راستہ_اشاریہ(Ast_گرہ_اظہاریہ* nodep) ثابت {
            ثابت خودکار it = m_force_راستہ_مقصد_اشاریہ.ڈھونڈو(force_راستہ_کلید(nodep));
            واپس it != m_force_راستہ_مقصد_اشاریہ.انتہا() ? it->دوسرا : -1;
        }
    };

    ساخت ForceHelperVars final {
        AstVar* m_rdVarp = خالی_اشاریہ;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = خالی_اشاریہ;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = خالی_اشاریہ;  // The __VforceVal (force value) variable
    };

    ساخت ArraySelInfo final {
        std::سمتیہ<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        منطقی m_has_ثنائی_ہندسہ_Sel = جھوٹ;  // Bit-select was found along the path too
    };

    ساخت ForceRangeInfo final : ForceRange {
        منطقی m_hasArraySel = جھوٹ;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

نجی:
    استعمال ScopeVarCache = std::unordered_map<ثابت AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;
    ثابت VNUser2_اندر_Use m_user2_اندر_Use;

عوامی:
    استعمال ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    استعمال Protected_Var_Ref_سیٹ = std::unordered_سیٹ<ثابت AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    استعمال Built_Var_Scope_سیٹ = std::unordered_سیٹ<ثابت AstVarScope*>;

نجی:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::سمتیہ<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, صحیح> m_var_مقصد_Id;
    std::unordered_سیٹ<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::سمتیہ<ForceInfo*>> m_rhs_Dep_مقصد_Forces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    منطقی m_doing_تفویض = جھوٹ;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    Protected_Var_Ref_سیٹ& m_permanentlyProtected;
    Built_Var_Scope_سیٹ& m_forceRdUpdateBuilt;

عوامی:
    ForceState(منطقی doing_تفویض, ForceHelperVarsByVar& forceHelperVarsByVar,
               Protected_Var_Ref_سیٹ& permanentlyProtected, Built_Var_Scope_سیٹ& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doing_تفویض{doing_تفویض}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    ساکن AstConst* makeZeroConst(Ast_گرہ* nodep, صحیح چوڑائی) {
        V3_نمبر zero{nodep, چوڑائی};
        zero.سیٹ_All_Bits0();
        واپس نیا AstConst{nodep->fileline(), zero};
    }

    ساکن AstConst* makeConst32(File_سطر* flp, صحیح قدر) {
        واپس نیا AstConst{flp, AstConst::Widthed_قدر{}, 32, static_cast<uint32_t>(قدر)};
    }

    ساکن AstConst* make_Range_نقاب_Const(Ast_گرہ* nodep, صحیح چوڑائی, صحیح lsb, صحیح msb) {
        V3_نمبر نقاب{nodep, چوڑائی};
        نقاب.سیٹ_All_Bits0();
        برائے (صحیح ثنائی_ہندسہ_2 = lsb; ثنائی_ہندسہ_2 <= msb; ++ثنائی_ہندسہ_2) نقاب.سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
        واپس نیا AstConst{nodep->fileline(), نقاب};
    }

    ساکن Ast_گرہ_اظہاریہ* zero_Pad_مقصد_بنیاد_چوڑائی(Ast_گرہ_اظہاریہ* exprp, صحیح بنیاد_چوڑائی, صحیح padLsb,
                                           صحیح padMsb) {
        اگر (بنیاد_چوڑائی <= 0) واپس exprp;
        ثابت صحیح کم_Pad = padLsb;
        ثابت صحیح بلند_Pad = بنیاد_چوڑائی - (padMsb + 1);
        اگر (کم_Pad > 0) {
            exprp = نیا AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, کم_Pad)};
        }
        اگر (بلند_Pad > 0) {
            exprp = نیا AstConcat{exprp->fileline(), makeZeroConst(exprp, بلند_Pad), exprp};
        }
        واپس exprp;
    }

    ساکن منطقی isUnpackedArrayDType(ثابت Ast_گرہ_DType* dtypep) {
        واپس VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    ساکن منطقی isBitwiseDType(ثابت Ast_گرہ* nodep) {
        ثابت AstBasicDType* ثابت basicp = nodep->dtypep()->skipRefp()->basicp();
        واپس basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    ساکن Ast_گرہ_اظہاریہ* cast_مقصد_گرہ_DType(Ast_گرہ_اظہاریہ* exprp, Ast_گرہ* dtypeFromp) {
        ثابت Ast_گرہ_DType* ثابت dtypep = dtypeFromp->dtypep()->skipRefp();
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();
        اگر (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            واپس exprp;
        }
        واپس نیا AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    ساکن خالی markNonReplaceable(AstVarRef* ثابت nodep) { nodep->user1_سیٹ_Once(); }
    منطقی isNotReplaceable(ثابت AstVarRef* ثابت nodep) ثابت {
        واپس nodep->user1() || m_permanentlyProtected.گنتی(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    خالی markPermanentlyProtected(AstVarRef* ثابت nodep) ثابت {
        markNonReplaceable(nodep);
        m_permanentlyProtected.داخل_کرو(nodep);
    }

    ساکن std::سمتیہ<ForceInfo*> force_Infos_اندر_Id_ترتیب(VarForceInfo& info) {
        std::سمتیہ<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.سائز());
        برائے (خودکار& it : info.m_forces) forceps.ڈالو_عقب(&it.دوسرا);
        std::ترتیب_دو(forceps.begin(), forceps.انتہا(), [](ثابت ForceInfo* ap, ثابت ForceInfo* bp) {
            واپس ap->m_forceId < bp->m_forceId;
        });
        واپس forceps;
    }

    ساکن std::سمتیہ<ثابت ForceInfo*> force_Infos_اندر_Id_ترتیب(ثابت VarForceInfo& info) {
        std::سمتیہ<ثابت ForceInfo*> forceps;
        forceps.reserve(info.m_forces.سائز());
        برائے (ثابت خودکار& it : info.m_forces) forceps.ڈالو_عقب(&it.دوسرا);
        std::ترتیب_دو(forceps.begin(), forceps.انتہا(), [](ثابت ForceInfo* ap, ثابت ForceInfo* bp) {
            واپس ap->m_forceId < bp->m_forceId;
        });
        واپس forceps;
    }

    ساکن منطقی is_Opaque_راستہ_Selector(ثابت Ast_گرہ* nodep) {
        واپس VN_IS(nodep, Sel) || VN_IS(nodep, گرہ_Sel) || VN_IS(nodep, StructSel);
    }

    ساکن منطقی is_Outermost_Opaque_راستہ_Selector(ثابت Ast_گرہ* nodep) {
        اگر (!is_Opaque_راستہ_Selector(nodep)) واپس جھوٹ;
        ثابت Ast_گرہ* ثابت backp = nodep->backp();
        واپس !backp || !is_Opaque_راستہ_Selector(backp);
    }

    ساکن AstVarRef* getOneVarRef(Ast_گرہ_اظہاریہ* forceStmtp) {
        Ast_گرہ* ثابت basep = forceStmtp->بنیاد_Fromp(سچ);
        اگر (AstSampled* sampledp = VN_CAST(basep, Sampled))
            اگر (Ast_گرہ_اظہاریہ* exprp = VN_CAST(sampledp->exprp(), گرہ_اظہاریہ))
                واپس getOneVarRef(exprp);
        AstVarRef* ثابت varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        واپس varRefp;
    }

    ساکن Ast_گرہ_اظہاریہ* buildNestedArraySel(File_سطر* flp, Ast_گرہ_اظہاریہ* fromp,
                                            ثابت std::سمتیہ<صحیح>& indicies) {
        Ast_گرہ_اظہاریہ* curp = fromp;
        برائے (ثابت صحیح idx : indicies) curp = نیا AstArraySel{flp, curp, idx};
        واپس curp;
    }

    سانچہ <قسم_نام Fn>
    ساکن Ast_گرہ_Stmt* foreach_Unpacked_برگ(ثابت std::سمتیہ<AstUnpackArrayDType*>& dims,
                                            Fn build_برگ) {
        Ast_گرہ_Stmt* headp = خالی_اشاریہ;
        Ast_گرہ_Stmt* tailp = خالی_اشاریہ;
        اگر (dims.خالی_خانے()) واپس خالی_اشاریہ;
        صحیح کل = 1;
        برائے (ثابت AstUnpackArrayDType* ثابت d : dims) کل *= d->elementsConst();
        اگر (کل <= 0) واپس خالی_اشاریہ;
        std::سمتیہ<صحیح> idx(dims.سائز(), 0);
        برائے (صحیح flat = 0; flat < کل; ++flat) {
            Ast_گرہ_Stmt* ثابت stmtp = build_برگ(idx, flat);
            اگر (!headp) {
                headp = stmtp;
            } ورنہ {
                tailp->جوڑو_اگلا(stmtp);
            }
            tailp = stmtp;
            برائے (صحیح d = static_cast<صحیح>(dims.سائز()) - 1; d >= 0; --d) {
                اگر (++idx[d] < dims[d]->elementsConst()) توڑو;
                idx[d] = 0;
            }
        }
        واپس headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_گرہ_اظہاریہ* lhsp, AstVar* varp,
                                     منطقی requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.خالی_خانے();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->چوڑائی() - 1) : 0;

        اگر (ثابت AstSel* ثابت outerSelp = VN_CAST(lhsp, Sel)) {
            صحیح کل_Lsb = 0;
            برائے (Ast_گرہ_اظہاریہ* curp = lhsp; ثابت AstSel* ثابت selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                اگر (requireConstRangeSelect) {
                    UASSERT_OBJ(VN_IS(selp->lsbp(), Const), lhsp,
                                "Unsupported: force on non-const range select");
                }
                کل_Lsb += selp->lsbConst();
            }
            info.m_padLsb = کل_Lsb;
            info.m_padMsb = کل_Lsb + outerSelp->چوڑائی_Const() - 1;
        }

        info.m_rangeLsb = info.m_padLsb;
        info.m_rangeMsb = info.m_padMsb;
        اگر (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::سمتیہ<صحیح> indices;
            indices.reserve(info.m_arrayInfo.m_sels.سائز());
            برائے (AstArraySel* ثابت selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.ڈالو_عقب(VN_AS(selp->bitp(), Const)->مقصد_SInt());
            }
            info.m_rangeLsb = flatten_اشاریہ(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } ورنہ اگر (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        اگر (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            ثابت صحیح اشاریہ = varInfo.get_Or_Create_Force_راستہ_اشاریہ(lhsp);
            info.m_rangeLsb = اشاریہ;
            info.m_rangeMsb = اشاریہ;
        }
        واپس info;
    }

    Ast_گرہ_اظہاریہ* جوڑو_Rhs_قدر_Reads(ثابت VarForceInfo& varInfo, Ast_گرہ_اظہاریہ* exprp) ثابت {
        اگر (!doing_تفویض()) واپس exprp;

        ثابت std::سمتیہ<ثابت ForceInfo*> forceps = force_Infos_اندر_Id_ترتیب(varInfo);
        اگر (forceps.خالی_خانے()) واپس exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* ثابت cexprp = نیا AstCExpr{exprp->fileline(), AstCExpr::خالص{}};
        cexprp->dtypeFrom(exprp);
        cexprp->جوڑو("(");
        برائے (ثابت ForceInfo* ثابت finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* ثابت refp
                = نیا AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->جوڑو("(void)(");
            cexprp->جوڑو(refp);
            cexprp->جوڑو("), ");
        }
        cexprp->جوڑو(exprp);
        cexprp->جوڑو(")");
        واپس cexprp;
    }

    Ast_گرہ_اظہاریہ* createForceReadCall(ثابت VarForceInfo& varInfo, File_سطر* flp, VCMethod طریقہ,
                                     Ast_گرہ_اظہاریہ* originalExprp, Ast_گرہ* dtypeFromp,
                                     Ast_گرہ_اظہاریہ* اشاریہ_Exprp) ثابت {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_گرہ_اظہاریہ* ثابت origValp
            = جوڑو_Rhs_قدر_Reads(varInfo, cast_مقصد_گرہ_DType(originalExprp, dtypeFromp));

        AstCMethodHard* ثابت callp = نیا AstCMethodHard{
            flp, نیا AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, طریقہ, origValp};
        اگر (اشاریہ_Exprp) callp->جوڑو_Pinsp(اشاریہ_Exprp);
        callp->dtypeFrom(dtypeFromp);
        واپس callp;
    }

    Ast_گرہ_اظہاریہ* create_Force_Rd_عبارت(ثابت VarForceInfo& varInfo, File_سطر* flp) ثابت {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        اگر (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            اگر (doing_تفویض()) { واپس نیا AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3_Error_رمز::UNOPTFLAT, سچ);
            AstVarRef* ثابت بنیاد_Refp = نیا AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(بنیاد_Refp);
            واپس createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, بنیاد_Refp, varp,
                                       خالی_اشاریہ);
        }
        AstVarRef* ثابت بنیاد_Refp = نیا AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(بنیاد_Refp);
        Ast_گرہ_اظہاریہ* ثابت enRefp = نیا AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_گرہ_اظہاریہ* ثابت valRefp = نیا AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        اگر (isBitwiseDType(varp)) {
            واپس نیا AstOr{
                flp, نیا AstAnd{flp, enRefp, valRefp},
                نیا AstAnd{flp, نیا AstNot{flp, enRefp->clone_درخت_خالص(جھوٹ)}, بنیاد_Refp}};
        }
        واپس نیا AstCond{flp, enRefp, valRefp, بنیاد_Refp};
    }

    Ast_گرہ_Stmt* createForceRdUpdateStmt(ثابت VarForceInfo& varInfo) ثابت {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        اگر (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            واپس createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_سطر* ثابت flp = varInfo.m_varVscp->fileline();
        واپس نیا Ast_تفویض{flp, نیا AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             create_Force_Rd_عبارت(varInfo, flp)};
    }

    Ast_گرہ_Stmt* createForceRdUpdateStmtUnpacked(ثابت VarForceInfo& varInfo) ثابت {
        File_سطر* ثابت flp = varInfo.m_varVscp->fileline();
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* ثابت arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت std::سمتیہ<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_گرہ_Stmt* ثابت stmtsp = foreach_Unpacked_برگ(
            dims, [&](ثابت std::سمتیہ<صحیح>& idx, صحیح /*flat*/) -> Ast_گرہ_Stmt* {
                AstVarRef* ثابت بنیاد_Refp = نیا AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(بنیاد_Refp);
                Ast_گرہ_اظہاریہ* ثابت بنیاد_Selp = buildNestedArraySel(flp, بنیاد_Refp, idx);
                Ast_گرہ_اظہاریہ* ثابت enSelp = buildNestedArraySel(
                    flp, نیا AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_گرہ_اظہاریہ* ثابت valSelp = buildNestedArraySel(
                    flp, نیا AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_گرہ_اظہاریہ* ثابت readExprp = نیا AstOr{
                    flp, نیا AstAnd{flp, enSelp, valSelp},
                    نیا AstAnd{flp, نیا AstNot{flp, enSelp->clone_درخت_خالص(جھوٹ)}, بنیاد_Selp}};
                Ast_گرہ_اظہاریہ* ثابت rdLhsSelp = buildNestedArraySel(
                    flp, نیا AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                واپس نیا Ast_تفویض{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        برائے (ثابت ForceInfo* ثابت finfop : force_Infos_اندر_Id_ترتیب(varInfo)) {
            اگر (!finfop->m_isExternal) جاری;
            stmtsp->جوڑو_اگلا(نیا Ast_تفویض{flp,
                                          نیا AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_درخت_خالص(جھوٹ)});
        }
        واپس stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        ثابت خودکار it = m_var_مقصد_Id.ڈھونڈو(vscp);
        اگر (it != m_var_مقصد_Id.انتہا()) واپس m_varInfos[it->دوسرا];

        m_var_مقصد_Id.emplace(vscp, m_varInfos.سائز());
        m_varInfos.emplace_عقب();
        VarForceInfo& info = m_varInfos.عقب();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* ثابت varp = info.m_varp;
        اگر (!varp->isForceable()) واپس info;

        File_سطر* ثابت flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        ثابت منطقی helperVarsBuilt = helperVars.m_rdVarp != خالی_اشاریہ;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != خالی_اشاریہ)
                        && helperVarsBuilt == (helperVars.m_valVarp != خالی_اشاریہ),
                    varp, "Incomplete force helper set");
        اگر (!helperVarsBuilt) {
            ثابت منطقی unpacked = isUnpackedArrayDType(varp->dtypep());
            ثابت VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_گرہ_DType* ثابت enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->ڈھونڈو_ثنائی_ہندسہ_DType();
            helperVars.m_rdVarp
                = نیا AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sig_عوامی(سچ);
            helperVars.m_enVarp
                = نیا AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(سچ);
            helperVars.m_valVarp
                = نیا AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(سچ);
            varp->جوڑو_اگلا_یہاں(helperVars.m_rdVarp);
            varp->جوڑو_اگلا_یہاں(helperVars.m_enVarp);
            varp->جوڑو_اگلا_یہاں(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = ڈھونڈو_Scope_Var(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = ڈھونڈو_Scope_Var(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = ڈھونڈو_Scope_Var(info.m_scopep, helperVars.m_valVarp);
        اگر (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } ورنہ {
            info.m_forceRdVscp = نیا AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = نیا AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = نیا AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->جوڑو_Varsp(info.m_forceRdVscp);
            info.m_scopep->جوڑو_Varsp(info.m_forceEnVscp);
            info.m_scopep->جوڑو_Varsp(info.m_forceValVscp);
        }
        واپس info;
    }

    خالی markClockedWrite(AstVar* varp) { m_clockedWrites.داخل_کرو(varp); }
    منطقی hasClockedWrite(AstVar* varp) ثابت { واپس m_clockedWrites.گنتی(varp); }

    منطقی doing_تفویض() ثابت { واپس m_doing_تفویض; }

    ثابت VarForceInfo* getVarInfo(AstVarScope* vscp) ثابت {
        ثابت خودکار it = m_var_مقصد_Id.ڈھونڈو(vscp);
        واپس it != m_var_مقصد_Id.انتہا() ? &m_varInfos[it->دوسرا] : خالی_اشاریہ;
    }

    AstVarScope* ڈھونڈو_Scope_Var(AstScope* scopep, ثابت AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        اگر (cache.خالی_خانے()) {
            برائے (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        ثابت خودکار it = cache.ڈھونڈو(varp);
        واپس it != cache.انتہا() ? it->دوسرا : خالی_اشاریہ;
    }
    خالی جوڑو_Force_Assignment(AstVar* varp, AstVarScope* vscp, Ast_گرہ_اظہاریہ* rhsExprp,
                            Ast_تفویض_Force* forceStmtp, صحیح rangeLsb, صحیح rangeMsb, صحیح padLsb,
                            صحیح padMsb, منطقی hasArraySel, منطقی skipRdRefresh = جھوٹ) {
        v3Global.سیٹ_استعمال_کرتا_Force();
        varp->سیٹ_Forced_By_رمز();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        ثابت صحیح forceId = info.m_forces.سائز();
        File_سطر* ثابت flp = varp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        اگر (!info.m_forceVecVscp) {
            AstCDType* ثابت forceVecDtypep = نیا AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->جوڑو_Typesp(forceVecDtypep);

            AstVar* ثابت forceVecVarp
                = نیا AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doing_تفویض ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(جھوٹ);
            forceVecVarp->is_اندرونی(سچ);
            varp->جوڑو_اگلا_یہاں(forceVecVarp);
            info.m_forceVecVscp = نیا AstVarScope{flp, scopep, forceVecVarp};
            scopep->جوڑو_Varsp(info.m_forceVecVscp);
        }

        خودکار pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, خالی_اشاریہ, rhsExprp});
        ForceInfo& finfo = pair.پہلا->دوسرا;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        اگر (doing_تفویض()) {
            std::سمتیہ<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* ثابت refp) {
                اگر (!refp->access().isReadOnly()) واپس;
                AstVar* ثابت depVarp = refp->varp();
                اگر (depVarp
                    && std::ڈھونڈو(depVarps.begin(), depVarps.انتہا(), depVarp) == depVarps.انتہا()) {
                    depVarps.ڈالو_عقب(depVarp);
                }
            });
            برائے (AstVar* ثابت depVarp : depVarps) m_rhs_Dep_مقصد_Forces[depVarp].ڈالو_عقب(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    ساکن خالی collectArraySelInfo(Ast_گرہ_اظہاریہ* exprp, ArraySelInfo& info) {
        اگر (ثابت خودکار* ثابت selp = VN_CAST(exprp, Sel)) {
            info.m_has_ثنائی_ہندسہ_Sel = سچ;
            collectArraySelInfo(selp->fromp(), info);
        } ورنہ اگر (خودکار* ثابت selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.ڈالو_عقب(selp);
        } ورنہ اگر (ثابت خودکار* ثابت memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    ساکن ArraySelInfo getArraySelInfo(Ast_گرہ_اظہاریہ* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        واپس info;
    }

    ساکن std::سمتیہ<صحیح> arraySelDimSizes(ثابت ArraySelInfo& info) {
        std::سمتیہ<صحیح> dims;
        dims.reserve(info.m_sels.سائز());
        برائے (AstArraySel* ثابت selp : info.m_sels) {
            Ast_گرہ_DType* ثابت dtypep = selp->fromp()->dtypep()->skipRefp();
            ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.ڈالو_عقب(arrayp->declRange().elements());
        }
        واپس dims;
    }

    ساکن صحیح flatten_اشاریہ(ثابت std::سمتیہ<صحیح>& indices, ثابت std::سمتیہ<صحیح>& dimSizes) {
        UASSERT(indices.سائز() == dimSizes.سائز(), "Array index and dimension size mismatch");
        صحیح اشاریہ = 0;
        صحیح stride = 1;
        برائے (صحیح i = static_cast<صحیح>(indices.سائز()) - 1; i >= 0; --i) {
            اشاریہ += indices[i] * stride;
            stride *= dimSizes[i];
        }
        واپس اشاریہ;
    }

    ساکن Ast_گرہ_اظہاریہ* build_Flatten_اشاریہ_اظہاریہ(File_سطر* flp, ثابت ArraySelInfo& info) {
        ثابت std::سمتیہ<صحیح> dimSizes = arraySelDimSizes(info);
        منطقی allConst = سچ;
        برائے (AstArraySel* ثابت selp : info.m_sels) {
            اگر (!VN_IS(selp->bitp(), Const)) {
                allConst = جھوٹ;
                توڑو;
            }
        }
        اگر (allConst) {
            std::سمتیہ<صحیح> constIndices;
            constIndices.reserve(info.m_sels.سائز());
            برائے (AstArraySel* ثابت selp : info.m_sels) {
                constIndices.ڈالو_عقب(VN_AS(selp->bitp(), Const)->مقصد_SInt());
            }
            واپس makeConst32(flp, flatten_اشاریہ(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_گرہ_اظہاریہ* resultp = خالی_اشاریہ;
        صحیح stride = 1;
        برائے (صحیح i = static_cast<صحیح>(info.m_sels.سائز()) - 1; i >= 0; --i) {
            Ast_گرہ_اظہاریہ* termp = info.m_sels[i]->bitp()->clone_درخت_خالص(جھوٹ);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            اگر (termp->چوڑائی() < 32) termp = نیا AstExtend{flp, termp, 32};
            اگر (stride != 1) termp = نیا AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? نیا Ast_جوڑو{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        واپس resultp;
    }

    ساکن Ast_گرہ_اظہاریہ* build_Rhs_ڈیٹا_اظہاریہ(File_سطر* flp, ثابت ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        واپس نیا AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    خالی finalizeRhsVars() {
        برائے (VarForceInfo& info : m_varInfos) {
            AstVar* ثابت varp = info.m_varp;
            اگر (info.m_forces.خالی_خانے()) جاری;

            AstScope* ثابت scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_سطر* ثابت flp = varp->fileline();
            ثابت std::سمتیہ<ForceInfo*> forceps = force_Infos_اندر_Id_ترتیب(info);

            برائے (ForceInfo* ثابت finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* ثابت rhsVarp = نیا AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doing_تفویض() ? "_VassignRHS" : "__VforceRHS")
                        + std::مقصد_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(سچ);
                rhsVarp->sig_عوامی(سچ);
                rhsVarp->سیٹ_Forced_By_رمز();
                varp->جوڑو_اگلا_یہاں(rhsVarp);
                finfo.m_rhsVarVscp = نیا AstVarScope{flp, scopep, rhsVarp};
                scopep->جوڑو_Varsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                //انتہا
                Ast_تفویض* ثابت rhsAssignp = نیا Ast_تفویض{
                    flp, نیا AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                اگر (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* ثابت touchCallp = نیا AstCMethodHard{
                        flp, نیا AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtype_سیٹ_Void();
                    Ast_گرہ_Stmt* ثابت touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->جوڑو_اگلا_یہاں(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* ثابت alwaysp
                    = نیا AstAlways{flp, VAlwaysKwd::ALWAYS, خالی_اشاریہ, rhsAssignp};
                Ast_Sen_درخت* ثابت senTreep
                    = نیا Ast_Sen_درخت{flp, نیا Ast_Sen_شے{flp, Ast_Sen_شے::Combo{}}};
                Ast_فعال* ثابت activep = نیا Ast_فعال{flp, "force-rhs-update", senTreep};
                activep->sen_درخت_Storep(activep->sentreep());
                activep->جوڑو_Stmtsp(alwaysp);
                scopep->جوڑو_Blocksp(activep);
            }

            اگر (info.m_forceRdVscp) {
                اگر (!m_forceRdUpdateBuilt.داخل_کرو(info.m_varVscp).دوسرا) جاری;
                Ast_فعال* ثابت فعال_Initp = نیا Ast_فعال{
                    flp, "force-init",
                    نیا Ast_Sen_درخت{flp, نیا Ast_Sen_شے{flp, Ast_Sen_شے::Static{}}}};
                فعال_Initp->sen_درخت_Storep(فعال_Initp->sentreep());
                Ast_گرہ_Stmt* initStmtp = خالی_اشاریہ;
                اگر (AstUnpackArrayDType* ثابت arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ثابت std::سمتیہ<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    ثابت صحیح inner_چوڑائی = dims.عقب()->subDTypep()->skipRefp()->چوڑائی();
                    initStmtp = foreach_Unpacked_برگ(
                        dims, [&](ثابت std::سمتیہ<صحیح>& idx, صحیح /*flat*/) -> Ast_گرہ_Stmt* {
                            Ast_گرہ_اظہاریہ* ثابت lhsp = buildNestedArraySel(
                                flp, نیا AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            واپس نیا Ast_تفویض{flp, lhsp, makeZeroConst(varp, inner_چوڑائی)};
                        });
                } ورنہ {
                    initStmtp = نیا Ast_تفویض{
                        flp, نیا AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->چوڑائی())};
                }
                initStmtp->جوڑو_اگلا(createForceRdUpdateStmt(info));
                فعال_Initp->جوڑو_Stmtsp(نیا AstInitial{flp, initStmtp});
                scopep->جوڑو_Blocksp(فعال_Initp);

                Ast_Sen_شے* itemsp = خالی_اشاریہ;
                خودکار جوڑو_Sen_شے = [&](AstVarScope* vscp) {
                    اگر (!vscp) واپس;
                    Ast_Sen_شے* ثابت nextp = نیا Ast_Sen_شے{
                        flp, VEdgeType::ET_بدلا, نیا AstVarRef{flp, vscp, VAccess::READ}};
                    اگر (itemsp) {
                        itemsp->جوڑو_اگلا(nextp);
                    } ورنہ {
                        itemsp = nextp;
                    }
                };
                جوڑو_Sen_شے(info.m_forceEnVscp);
                جوڑو_Sen_شے(info.m_forceValVscp);
                AstVarRef* ثابت origSenRefp = نیا AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                Ast_Sen_شے* ثابت origItemp
                    = نیا Ast_Sen_شے{flp, VEdgeType::ET_بدلا, origSenRefp};
                اگر (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->جوڑو_اگلا(origItemp);
                برائے (ForceInfo* ثابت finfop : forceps) {
                    اگر (!finfop->m_isExternal) جوڑو_Sen_شے(finfop->m_rhsVarVscp);
                }

                Ast_فعال* ثابت activep
                    = نیا Ast_فعال{flp, "force-rd-update", نیا Ast_Sen_درخت{flp, itemsp}};
                activep->sen_درخت_Storep(activep->sentreep());
                activep->جوڑو_Stmtsp(نیا AstAlways{flp, VAlwaysKwd::ALWAYS, خالی_اشاریہ,
                                                 createForceRdUpdateStmt(info)});
                scopep->جوڑو_Blocksp(activep);
            }
        }
    }

    Ast_گرہ* createRhsUpdatesForWrite(File_سطر* flp, AstVar* writtenVarp) ثابت {
        اگر (!doing_تفویض()) واپس خالی_اشاریہ;

        ثابت خودکار it = m_rhs_Dep_مقصد_Forces.ڈھونڈو(writtenVarp);
        اگر (it == m_rhs_Dep_مقصد_Forces.انتہا()) واپس خالی_اشاریہ;

        Ast_گرہ* headp = خالی_اشاریہ;
        Ast_گرہ* tailp = خالی_اشاریہ;
        برائے (ثابت ForceInfo* ثابت finfop : it->دوسرا) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            Ast_تفویض* ثابت updatep
                = نیا Ast_تفویض{flp, نیا AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_درخت_خالص(جھوٹ)};
            اگر (tailp) {
                tailp->جوڑو_اگلا_یہاں(updatep);
            } ورنہ {
                headp = updatep;
            }
            tailp = updatep;
        }
        واپس headp;
    }

    ثابت ForceInfo& getForceInfo(Ast_تفویض_Force* forceStmtp) ثابت {
        AstVarScope* ثابت vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        ثابت VarForceInfo* ثابت varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        خودکار it2 = varInfo->m_forces.ڈھونڈو(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.انتہا(), "Force statement not found");
        واپس it2->دوسرا;
    }

    ساکن منطقی selOverlapsAnyForce(ثابت VarForceInfo& varInfo, صحیح selLsb, صحیح selMsb) {
        برائے (ثابت خودکار& pair : varInfo.m_forces) {
            اگر (pair.دوسرا.m_rangeLsb <= selMsb && pair.دوسرا.m_rangeMsb >= selLsb) واپس سچ;
        }
        واپس جھوٹ;
    }

    Ast_گرہ_اظہاریہ* create_Force_Read_عبارت(ثابت VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) ثابت {
        File_سطر* ثابت flp = originalRefp->fileline();
        AstVarRef* ثابت refp = originalRefp->clone_درخت_خالص(جھوٹ);
        refp->access(VAccess::READ);
        واپس createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   خالی_اشاریہ);
    }

    Ast_گرہ_اظہاریہ* create_Force_Read_اشاریہ_عبارت(ثابت VarForceInfo& varInfo,
                                                Ast_گرہ_اظہاریہ* originalExprp,
                                                Ast_گرہ_اظہاریہ* اشاریہ_Exprp) ثابت {
        File_سطر* ثابت flp = originalExprp->fileline();
        Ast_گرہ_اظہاریہ* ثابت exprp = originalExprp->clone_درخت_خالص(جھوٹ);
        // Must be an LValue to a static variable
        VN_AS(exprp->get_VAccess_ہدف_Recurse(), VarRef)->access(VAccess::READ);
        واپس createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_اشاریہ, exprp, originalExprp,
                                   اشاریہ_Exprp);
    }

    ساکن Ast_گرہ_اظہاریہ* rebuild_Sel_راستہ(Ast_گرہ_اظہاریہ* pathp, Ast_گرہ_اظہاریہ* بنیاد_Exprp) {
        اگر (ثابت AstSel* ثابت selp = VN_CAST(pathp, Sel)) {
            Ast_گرہ_اظہاریہ* ثابت fromp = rebuild_Sel_راستہ(selp->fromp(), بنیاد_Exprp);
            AstSel* ثابت outp
                = نیا AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->چوڑائی_Const()};
            outp->dtypeFrom(selp);
            واپس outp;
        }
        واپس بنیاد_Exprp;
    }
};

// Split deassign concat LHS before converting to release internals.
ساکن خالی بانٹو_Deassign(AstDeassign* nodep) {
    AstConcat* ثابت concatp = VN_CAST(nodep->lhsp(), Concat);
    اگر (!concatp) واپس;

    File_سطر* ثابت flp = nodep->fileline();
    AstDeassign* ثابت newLp = نیا AstDeassign{flp, concatp->lhsp()->unlink_Fr_عقب()};
    AstDeassign* ثابت newRp = نیا AstDeassign{flp, concatp->rhsp()->unlink_Fr_عقب()};
    Ast_گرہ_اظہاریہ* ثابت conp = concatp->unlink_Fr_عقب();
    nodep->replaceWith(newLp);
    newLp->جوڑو_اگلا_یہاں(newRp);
    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    VL_DO_DANGLING(conp->delete_درخت(), conp);

    بانٹو_Deassign(newLp);
    بانٹو_Deassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

جماعت ForceDiscoveryVisitor final : عوامی VNVisitorConst {
    ForceState& m_state;
    منطقی m_اندر_Clocked_فعال = جھوٹ;

    خالی buildForceableUnpackedArray(AstVarScope* ثابت nodep,
                                     AstUnpackArrayDType* ثابت arrDtypep) {
        AstVar* ثابت varp = nodep->varp();
        ثابت std::سمتیہ<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.خالی_خانے(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        ثابت Ast_گرہ_DType* ثابت برگ_Dtypep = dims.عقب()->subDTypep()->skipRefp();
        ثابت AstBasicDType* ثابت innerBasicp = برگ_Dtypep->basicp();
        اگر (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            واپس;
        }

        File_سطر* ثابت flp = varp->fileline();
        ثابت صحیح inner_چوڑائی = برگ_Dtypep->چوڑائی();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* ثابت enVscp = info.m_forceEnVscp;
        AstVarScope* ثابت valVscp = info.m_forceValVscp;

        Ast_Sen_شے* ثابت itemsp = نیا Ast_Sen_شے{flp, VEdgeType::ET_بدلا,
                                                  نیا AstVarRef{flp, enVscp, VAccess::READ}};
        Ast_فعال* ثابت activep = نیا Ast_فعال{flp, "force-update", نیا Ast_Sen_درخت{flp, itemsp}};
        activep->sen_درخت_Storep(activep->sentreep());

        Ast_گرہ_Stmt* ثابت alwaysBodyHeadp = ForceState::foreach_Unpacked_برگ(
            dims, [&](ثابت std::سمتیہ<صحیح>& idx, صحیح flat) -> Ast_گرہ_Stmt* {
                AstVarRef* ثابت origRefp = نیا AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_گرہ_اظہاریہ* ثابت origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_گرہ_اظہاریہ* ثابت enSelp = ForceState::buildNestedArraySel(
                    flp, نیا AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_گرہ_اظہاریہ* ثابت valSelp = ForceState::buildNestedArraySel(
                    flp, نیا AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_گرہ_اظہاریہ* ثابت forceExprp = نیا AstOr{
                    flp, نیا AstAnd{flp, enSelp, valSelp},
                    نیا AstAnd{flp, نیا AstNot{flp, enSelp->clone_درخت_خالص(جھوٹ)}, origSelp}};
                Ast_گرہ_اظہاریہ* ثابت lhsSelp = ForceState::buildNestedArraySel(
                    flp, نیا AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                Ast_تفویض_Force* ثابت forceAssignp = نیا Ast_تفویض_Force{flp, lhsSelp, forceExprp};
                forceAssignp->user2(سچ);

                Ast_گرہ_اظہاریہ* ثابت rhsClonep = forceExprp->clone_درخت_خالص(جھوٹ);
                rhsClonep->foreach([varp](AstVarRef* ثابت r) {
                    اگر (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.جوڑو_Force_Assignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/inner_چوڑائی - 1,
                                           /*hasArraySel=*/سچ, /*skipRdRefresh=*/سچ);
                واپس forceAssignp;
            });
        activep->جوڑو_Stmtsp(نیا AstAlways{flp, VAlwaysKwd::ALWAYS, خالی_اشاریہ, alwaysBodyHeadp});
        nodep->scopep()->جوڑو_Blocksp(activep);
    }

    خالی دورہ(Ast_تفویض_Force* nodep) override {
        اگر (nodep->user2()) واپس;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* ثابت forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, سچ);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_گرہ_اظہاریہ* ثابت rhsExprp = nodep->rhsp()->clone_درخت_خالص(جھوٹ);

        m_state.جوڑو_Force_Assignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    خالی دورہ(Ast_تفویض* nodep) override {
        اگر (m_state.doing_تفویض() && m_اندر_Clocked_فعال) {
            اگر (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    خالی دورہ(Ast_فعال* nodep) override {
        VL_RESTORER(m_اندر_Clocked_فعال);
        m_اندر_Clocked_فعال = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    خالی دورہ(AstVarScope* nodep) override {
        اگر (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            اگر (m_state.doing_تفویض()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                واپس;
            }

            اگر (AstUnpackArrayDType* ثابت arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                واپس;
            }

            ثابت AstBasicDType* ثابت bdtypep = nodep->varp()->basicp();
            اگر (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
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
            Ast_Sen_شے* ثابت itemsp = نیا Ast_Sen_شے{flp, VEdgeType::ET_بدلا,
                                                      نیا AstVarRef{flp, enVscp, VAccess::READ}};
            Ast_فعال* ثابت activep
                = نیا Ast_فعال{flp, "force-update", نیا Ast_Sen_درخت{flp, itemsp}};
            activep->sen_درخت_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* ثابت refp = نیا AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* ثابت enRefp = نیا AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* ثابت valRefp = نیا AstVarRef{flp, valVscp, VAccess::READ};
            ثابت AstBasicDType* ثابت basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_گرہ_اظہاریہ* ثابت forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_گرہ_اظہاریہ*>(نیا AstOr{
                            flp, نیا AstAnd{flp, enRefp, valRefp},
                            نیا AstAnd{flp, نیا AstNot{flp, enRefp->clone_درخت_خالص(جھوٹ)}, refp}})
                      : static_cast<Ast_گرہ_اظہاریہ*>(نیا AstCond{flp, enRefp, valRefp, refp});
            Ast_تفویض_Force* ثابت forceAssignp
                = نیا Ast_تفویض_Force{flp, نیا AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(سچ);
            activep->جوڑو_Stmtsp(نیا AstAlways{flp, VAlwaysKwd::ALWAYS, خالی_اشاریہ, forceAssignp});
            nodep->scopep()->جوڑو_Blocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_گرہ_اظہاریہ* ثابت rhsClonep = forceExprp->clone_درخت_خالص(جھوٹ);
            rhsClonep->foreach([varp](AstVarRef* ثابت refp) {
                اگر (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            ثابت منطقی bitwiseVar = ForceState::isBitwiseDType(varp);
            ثابت صحیح padMsb = bitwiseVar ? (varp->چوڑائی() - 1) : 0;
            صحیح rangeLsb = 0;
            صحیح rangeMsb = padMsb;
            اگر (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.جوڑو_Force_Assignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, جھوٹ);
        }
        iterateChildrenConst(nodep);
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عوامی:
    صریح ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_اگلا_Const_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

جماعت ForceConvertVisitor final : عوامی VNVisitor {
    ForceState& m_state;

    خالی دورہ(Ast_تفویض_Force* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp();
        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ثابت forcedVarp = lhsVarRefp->varp();

        ثابت ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        ثابت ForceState::VarForceInfo* ثابت varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_سطر* ثابت flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        Ast_تفویض* ثابت rhsAssignp
            = نیا Ast_تفویض{flp, نیا AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_درخت_خالص(جھوٹ)};

        Ast_تفویض* valAssignp = خالی_اشاریہ;
        Ast_تفویض* enAssignp = خالی_اشاریہ;
        ثابت منطقی bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        اگر (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_گرہ_اظہاریہ* بنیاد_Exprp = nodep->rhsp()->clone_درخت_خالص(جھوٹ);
            بنیاد_Exprp->foreach(
                [](AstVarRef* ثابت refp) { ForceState::markNonReplaceable(refp); });
            اگر (bitwiseForcedVar) {
                بنیاد_Exprp = ForceState::zero_Pad_مقصد_بنیاد_چوڑائی(بنیاد_Exprp, forcedVarp->چوڑائی(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            اگر (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* ثابت نقاب_Constp = ForceState::make_Range_نقاب_Const(
                    nodep, forcedVarp->چوڑائی(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_گرہ_اظہاریہ* ثابت valReadp
                    = نیا AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_گرہ_اظہاریہ* ثابت valWritep
                    = نیا AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_گرہ_اظہاریہ* ثابت notMaskp = نیا AstNot{flp, نقاب_Constp};
                Ast_گرہ_اظہاریہ* ثابت maskedOldp = نیا AstAnd{flp, valReadp, notMaskp};
                Ast_گرہ_اظہاریہ* ثابت newValp = نیا AstOr{flp, maskedOldp, بنیاد_Exprp};
                valAssignp = نیا Ast_تفویض{flp, valWritep, newValp};

                Ast_گرہ_اظہاریہ* ثابت enReadp
                    = نیا AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_گرہ_اظہاریہ* ثابت enWritep
                    = نیا AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_گرہ_اظہاریہ* ثابت newEnp
                    = نیا AstOr{flp, enReadp, نقاب_Constp->clone_درخت_خالص(جھوٹ)};
                enAssignp = نیا Ast_تفویض{flp, enWritep, newEnp};
            } ورنہ {
                AstConst* ثابت oneConstp = ForceState::make_Range_نقاب_Const(nodep, 1, 0, 0);
                Ast_گرہ_اظہاریہ* ثابت rhsValp = ForceState::cast_مقصد_گرہ_DType(بنیاد_Exprp, forcedVarp);
                valAssignp = نیا Ast_تفویض{
                    flp, نیا AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = نیا Ast_تفویض{
                    flp, نیا AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        ثابت AstSel* ثابت selLhsp = VN_CAST(lhsp, Sel);
        ثابت منطقی array_ثنائی_ہندسہ_Sel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_has_ثنائی_ہندسہ_Sel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->چوڑائی();
        Ast_گرہ_اظہاریہ* ثابت rhsDatap = ForceState::build_Rhs_ڈیٹا_اظہاریہ(flp, info);
        AstCExpr* ثابت rhsAddrp = نیا AstCExpr{flp};
        rhsAddrp->جوڑو("&(");
        rhsAddrp->جوڑو(rhsDatap);
        rhsAddrp->جوڑو(")");
        AstCMethodHard* ثابت جوڑو_Force_Callp = نیا AstCMethodHard{
            flp, نیا AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_جوڑو,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        جوڑو_Force_Callp->جوڑو_Pinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        جوڑو_Force_Callp->جوڑو_Pinsp(rhsAddrp);
        جوڑو_Force_Callp->جوڑو_Pinsp(
            ForceState::makeConst32(flp, array_ثنائی_ہندسہ_Sel ? info.m_padLsb : info.m_rangeLsb));
        اگر (array_ثنائی_ہندسہ_Sel) {
            جوڑو_Force_Callp->جوڑو_Pinsp(ForceState::makeConst32(flp, info.m_padLsb));
            جوڑو_Force_Callp->جوڑو_Pinsp(ForceState::makeConst32(flp, info.m_padMsb));
            جوڑو_Force_Callp->جوڑو_Pinsp(ForceState::makeConst32(flp, selLhsp->fromp()->چوڑائی()));
        }
        جوڑو_Force_Callp->dtype_سیٹ_Void();
        Ast_گرہ_Stmt* ثابت stmtp = جوڑو_Force_Callp->makeStmt();

        Ast_گرہ* tailp = rhsAssignp;
        اگر (valAssignp) {
            tailp->جوڑو_اگلا_یہاں(valAssignp);
            tailp = valAssignp;
        }
        اگر (enAssignp) {
            tailp->جوڑو_اگلا_یہاں(enAssignp);
            tailp = enAssignp;
        }
        tailp->جوڑو_اگلا_یہاں(stmtp);
        اگر (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->جوڑو_اگلا_یہاں(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    خالی دورہ(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp();
        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ثابت releasedVarp = lhsVarRefp->varp();

        ثابت ForceState::VarForceInfo* ثابت varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        اگر (!varInfo) {
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            واپس;
        }
        UASSERT_OBJ(!varInfo->m_forces.خالی_خانے(), nodep, "Var info for variable with no forces");

        File_سطر* ثابت flp = nodep->fileline();

        ثابت ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, جھوٹ);

        ثابت AstSel* ثابت selLhsp = VN_CAST(lhsp, Sel);
        ثابت منطقی array_ثنائی_ہندسہ_Sel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_has_ثنائی_ہندسہ_Sel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->چوڑائی();
        AstCMethodHard* ثابت releaseCallp = نیا AstCMethodHard{
            flp, نیا AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->جوڑو_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        اگر (array_ثنائی_ہندسہ_Sel) {
            releaseCallp->جوڑو_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->جوڑو_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtype_سیٹ_Void();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_گرہ_Stmt* ثابت releasep = releaseCallp->makeStmt();

        Ast_تفویض* clearEnp = خالی_اشاریہ;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        اگر (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_گرہ_اظہاریہ* ثابت enWritep
                = نیا AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            اگر (ForceState::isBitwiseDType(releasedVarp)) {
                ثابت صحیح var_چوڑائی = releasedVarp->چوڑائی();
                اگر (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == var_چوڑائی - 1) {
                    clearEnp
                        = نیا Ast_تفویض{flp, enWritep, ForceState::makeZeroConst(nodep, var_چوڑائی)};
                } ورنہ {
                    // forceEn = forceEn & ~mask(range);
                    Ast_گرہ_اظہاریہ* ثابت enReadp
                        = نیا AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* ثابت نقاب_Const = ForceState::make_Range_نقاب_Const(
                        nodep, var_چوڑائی, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_گرہ_اظہاریہ* ثابت newEnp
                        = نیا AstAnd{flp, enReadp, نیا AstNot{flp, نقاب_Const}};
                    clearEnp = نیا Ast_تفویض{flp, enWritep, newEnp};
                }
            } ورنہ {
                clearEnp = نیا Ast_تفویض{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        ثابت AstSel* ثابت selp = VN_CAST(lhsp, Sel);
        Ast_گرہ_اظہاریہ* ثابت basep = selp ? selp->fromp() : lhsp;

        Ast_گرہ* stmtListp = releasep;
        اگر (clearEnp) {
            clearEnp->جوڑو_اگلا_یہاں(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        ثابت منطقی بھرا_Bitwise_Release
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->چوڑائی() - 1;
        اگر (!releasedVarp->isContinuously()
            && !(m_state.doing_تفویض() && m_state.hasClockedWrite(releasedVarp)
                 && بھرا_Bitwise_Release)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            ثابت منطقی has_Opaque_راستہ = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_گرہ_اظہاریہ* forceReadp = خالی_اشاریہ;
            اگر (has_Opaque_راستہ) {
                forceReadp = m_state.create_Force_Read_اشاریہ_عبارت(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } ورنہ اگر (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.create_Force_Read_اشاریہ_عبارت(
                    *varInfo, basep,
                    ForceState::build_Flatten_اشاریہ_اظہاریہ(flp, rangeInfo.m_arrayInfo));
                اگر (selp) { forceReadp = ForceState::rebuild_Sel_راستہ(lhsp, forceReadp); }
            } ورنہ {
                forceReadp
                    = selp ? ForceState::rebuild_Sel_راستہ(
                                 lhsp, m_state.create_Force_Read_عبارت(*varInfo, lhsVarRefp))
                           : m_state.create_Force_Read_عبارت(*varInfo, lhsVarRefp);
            }
            Ast_تفویض* ثابت assignp = نیا Ast_تفویض{flp, lhsp->clone_درخت_خالص(جھوٹ), forceReadp};
            assignp->جوڑو_اگلا_یہاں(stmtListp);
            stmtListp = assignp;
        }

        اگر (varInfo->m_forceRdVscp) stmtListp->جوڑو_اگلا(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_اگلا_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

جماعت ForceReplaceVisitor final : عوامی VNVisitor {
    ثابت ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_گرہ_Stmt* m_stmtp = خالی_اشاریہ;
    منطقی m_اندر_Logic = جھوٹ;

    خالی iterateLogic(Ast_گرہ* nodep) {
        VL_RESTORER(m_اندر_Logic);
        m_اندر_Logic = سچ;
        iterateChildren(nodep);
    }

    خالی دورہ(Ast_گرہ_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_تفویض* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        اگر (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp()->بنیاد_Fromp(سچ), VarRef)) {
            اگر (Ast_گرہ* ثابت updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->جوڑو_اگلا_یہاں(updatep);
            }
        }
    }
    خالی دورہ(Ast_تفویض_Cont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate_And_اگلا_Null(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    خالی دورہ(AstDeassign*) override {}
    خالی دورہ(AstCFunc* nodep) override { iterateLogic(nodep); }
    خالی دورہ(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    خالی دورہ(Ast_گرہ_Procedure* nodep) override { iterateLogic(nodep); }
    خالی دورہ(AstAlways* nodep) override {
        اگر (nodep->keyword() == VAlwaysKwd::CONT_تفویض) {
            iterateChildren(nodep);
            واپس;
        }
        iterateLogic(nodep);
    }
    خالی دورہ(Ast_Sen_شے* nodep) override { iterateLogic(nodep); }
    خالی دورہ(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef);
        اگر (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            دورہ(static_cast<Ast_گرہ*>(nodep));
            واپس;
        }

        AstVar* ثابت varp = refp->varp();
        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(refp->varScopep());
        اگر (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.خالی_خانے()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            دورہ(static_cast<Ast_گرہ*>(nodep));
            واپس;
        }

        اگر (ثابت AstConst* ثابت lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            ثابت صحیح selLsb = lsbConstp->مقصد_SInt();
            ثابت صحیح selMsb = selLsb + nodep->چوڑائی() - 1;
            اگر (!varp->is_Sig_عوامی()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                دورہ(static_cast<Ast_گرہ*>(nodep));
                واپس;
            }
        }

        File_سطر* ثابت flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* ثابت refClonep = refp->clone_درخت_خالص(جھوٹ);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* ثابت callp = نیا AstCMethodHard{
            flp, نیا AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->چوڑائی())};
        callp->جوڑو_Pinsp(refClonep);
        callp->جوڑو_Pinsp(nodep->lsbp()->clone_درخت_خالص(جھوٹ));
        callp->جوڑو_Pinsp(ForceState::makeConst32(flp, nodep->چوڑائی()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstArraySel* nodep) override {
        اگر (ثابت AstArraySel* ثابت عقب_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            اگر (عقب_Selp->fromp() == nodep) {
                iterateChildren(nodep);
                واپس;
            }
        }

        Ast_گرہ* ثابت basep = nodep->بنیاد_Fromp(سچ);
        AstVarRef* ثابت بنیاد_Refp = VN_CAST(basep, VarRef);
        اگر (!بنیاد_Refp) {
            iterateChildren(nodep);
            واپس;
        }
        AstVar* ثابت varp = بنیاد_Refp->varp();
        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(بنیاد_Refp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        اگر (m_state.isNotReplaceable(بنیاد_Refp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            واپس;
        }

        اگر (!بنیاد_Refp->access().isReadOnly()) {
            iterateChildren(nodep);
            واپس;
        }
        اگر (m_state.doing_تفویض() && varInfo->m_forceRdVscp) {
            بنیاد_Refp->varp(varInfo->m_forceRdVscp->varp());
            بنیاد_Refp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            واپس;
        }
        ثابت ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        برائے (AstArraySel* ثابت selp : arrayInfo.m_sels) iterate_And_اگلا_Null(selp->bitp());
        Ast_گرہ_اظہاریہ* ثابت اشاریہ_Exprp
            = ForceState::build_Flatten_اشاریہ_اظہاریہ(nodep->fileline(), arrayInfo);
        Ast_گرہ_اظہاریہ* ثابت readExprp
            = m_state.create_Force_Read_اشاریہ_عبارت(*varInfo, nodep, اشاریہ_Exprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    خالی دورہ(AstVarRef* nodep) override {
        اگر (m_state.isNotReplaceable(nodep)) واپس;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        اگر (ثابت AstArraySel* ثابت عقب_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            اگر (عقب_Selp->fromp() == nodep) واپس;
        }

        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(nodep->varScopep());
        اگر (!varInfo) واپس;

        اگر (varInfo->m_forceRdVscp) {
            اگر (nodep->access().isRW()) {
                اگر (m_اندر_Logic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                واپس;
            }
            اگر (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_گرہ_اظہاریہ* ثابت readp
                    = m_state.create_Force_Rd_عبارت(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                واپس;
            }
            اگر (m_اندر_Logic && m_stmtp) {
                m_stmtp->جوڑو_اگلا_یہاں(m_state.createForceRdUpdateStmt(*varInfo));
            }
            واپس;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        اگر (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            واپس;
        }

        UASSERT_OBJ(!varInfo->m_forces.خالی_خانے(), nodep, "Var info for variable with no forces");

        اگر (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } ورنہ اگر (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_گرہ_اظہاریہ* ثابت readExprp = m_state.create_Force_Read_عبارت(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }
    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (ForceState::is_Outermost_Opaque_راستہ_Selector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_گرہ_اظہاریہ* ثابت exprp = VN_AS(nodep, گرہ_اظہاریہ);
            Ast_گرہ* ثابت basep = exprp->بنیاد_Fromp(سچ);
            AstVarRef* ثابت بنیاد_Refp = VN_CAST(basep, VarRef);
            اگر (بنیاد_Refp) {
                AstVar* ثابت varp = بنیاد_Refp->varp();
                اگر (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    ثابت ForceState::VarForceInfo* ثابت varInfo
                        = m_state.getVarInfo(بنیاد_Refp->varScopep());
                    اگر (!m_state.isNotReplaceable(بنیاد_Refp) && varInfo) {
                        ثابت صحیح force_راستہ_اشاریہ = varInfo->ڈھونڈو_Force_راستہ_اشاریہ(exprp);
                        اگر (force_راستہ_اشاریہ >= 0) {
                            اگر (!بنیاد_Refp->access().isReadOnly()) واپس;
                            Ast_گرہ_اظہاریہ* ثابت readExprp = m_state.create_Force_Read_اشاریہ_عبارت(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), force_راستہ_اشاریہ));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                            واپس;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

عوامی:
    صریح ForceReplaceVisitor(AstNetlist* nodep, ثابت ForceState& state)
        : m_state{state} {
        iterate_And_اگلا_Null(nodep->modulesp());
    }
    ~ForceReplaceVisitor() override {
        V3Stats::جوڑو_Stat("Non-overlapping force sels", m_nonOverlappingForceSels);
    }
};
//######################################################################
//

//######################################################################
// V3Force - Main entry point

ساکن خالی forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::Protected_Var_Ref_سیٹ& permanentlyProtected,
                         ForceState::Built_Var_Scope_سیٹ& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    اگر (!v3Global.has_Forceable_اشارات()) واپس;
    ForceState state{جھوٹ, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_درخت("force", 0, dump_درخت_Either_Level() >= 3);
}

ساکن خالی تفویض_All_Impl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::Protected_Var_Ref_سیٹ& permanentlyProtected,
                          ForceState::Built_Var_Scope_سیٹ& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    اگر (!v3Global.has_تفویض_Deassign()) واپس;

    std::سمتیہ<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.ڈالو_عقب(deassignp); });
    برائے (AstDeassign* ثابت deassignp : deassignps) بانٹو_Deassign(deassignp);

    std::سمتیہ<Ast_تفویض_Cont*> تفویض_Contps;
    deassignps.clear();
    nodep->foreach([&](Ast_گرہ_Stmt* nodep) {
        اگر (Ast_تفویض_Cont* ثابت تفویض_Contp = VN_CAST(nodep, تفویض_Cont)) {
            تفویض_Contps.ڈالو_عقب(تفویض_Contp);
        } ورنہ اگر (AstDeassign* ثابت deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.ڈالو_عقب(deassignp);
        }
    });

    برائے (Ast_تفویض_Cont* ثابت assignp : تفویض_Contps) {
        assignp->replaceWith(نیا Ast_تفویض_Force{assignp->fileline(),
                                                assignp->lhsp()->unlink_Fr_عقب(),
                                                assignp->rhsp()->unlink_Fr_عقب()});
        VL_DO_DANGLING(assignp->delete_درخت(), assignp);
    }
    برائے (AstDeassign* ثابت deassignp : deassignps) {
        deassignp->replaceWith(
            نیا AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_درخت_خالص(سچ)});
        VL_DO_DANGLING(deassignp->delete_درخت(), deassignp);
    }
    ForceState state{سچ, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_درخت("assign-deassign", 0, dump_درخت_Either_Level() >= 3);
}

خالی V3Force::force_And_تفویض_All(AstNetlist* nodep) {
    ثابت VNUser3_اندر_Use user3_اندر_Use;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::Protected_Var_Ref_سیٹ permanentlyProtected;
    ForceState::Built_Var_Scope_سیٹ forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    تفویض_All_Impl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
