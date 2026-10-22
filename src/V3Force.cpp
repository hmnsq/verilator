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

رده ForceState final {
عمومی:
    ساختار ForceRange VL_NOT_FINAL {
        صحیح m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        صحیح m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        صحیح m_padLsb = 0;  // Bit positions for RHS padding
        صحیح m_padMsb = 0;  // Bit positions for RHS padding
    };

    ساختار ForceInfo final : ForceRange {
        // MEMBERS
        صحیح m_forceId = 0;  // Unique (per signal) variable of this force assignment
        منطقی m_hasArraySel = نادرست;  // If this has an array select on LHS
        منطقی m_isExternal = نادرست;  // Synthetic force for the public force controls
        منطقی m_skipRdRefresh = نادرست;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = nullptr;  // Scope of the var containing RHSID
        Ast_گره_عبارت* m_rhsExprp = nullptr;  // Expression on RHS of this force assignment

        ForceInfo() = پیشفرض;
        ForceInfo(صحیح rangeLsb, صحیح rangeMsb, صحیح padLsb, صحیح padMsb, صحیح forceId,
                  منطقی hasArraySel, AstVarScope* rhsVarVscp, Ast_گره_عبارت* rhsExprp)
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

    ساختار VarForceInfo final {
        AstVarScope* m_forceVecVscp = nullptr;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = nullptr;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = nullptr;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = nullptr;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = nullptr;  // Scope of the forced (original) variable
        AstVar* m_varp = nullptr;  // The forced (original) variable
        AstScope* m_scopep = nullptr;  // Scope containing the forced variable
        std::unordered_map<AstAssignForce*, ForceInfo> m_forces;
        std::unordered_map<string, صحیح> m_forcePathToIndex;
        صحیح m_nextForcePathIndex = 1;  // Start at 1 so 0 can be the base path (whole signal)

    خصوصی:
        ایستا تهی build_Force_Path_کلید_Recurse(Ast_گره_عبارت* nodep, string& out) {
            اگر (VN_IS(nodep, VarRef)) بازگشت;
            اگر (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
                build_Force_Path_کلید_Recurse(selp->fromp(), out);
                out += "|S" + cvtToStr(selp->lsbConst()) + ":" + cvtToStr(selp->widthConst());
                بازگشت;
            }
            اگر (AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
                Ast_گره_عبارت* ثابت fromp = selp->fromp();
                build_Force_Path_کلید_Recurse(fromp, out);
                out += "|T" + selp->name();
                بازگشت;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        ایستا string force_Path_کلید(Ast_گره_عبارت* nodep) {
            string out;
            build_Force_Path_کلید_Recurse(nodep, out);
            بازگشت out;
        }

    عمومی:
        صحیح getOrCreateForcePathIndex(Ast_گره_عبارت* nodep) {
            ثابت auto pair
                = m_forcePathToIndex.emplace(force_Path_کلید(nodep), m_nextForcePathIndex);
            اگر (pair.second) ++m_nextForcePathIndex;
            بازگشت pair.first->second;
        }

        صحیح findForcePathIndex(Ast_گره_عبارت* nodep) ثابت {
            ثابت auto it = m_forcePathToIndex.find(force_Path_کلید(nodep));
            بازگشت it != m_forcePathToIndex.end() ? it->second : -1;
        }
    };

    ساختار ForceHelperVars final {
        AstVar* m_rdVarp = nullptr;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = nullptr;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = nullptr;  // The __VforceVal (force value) variable
    };

    ساختار ArraySelInfo final {
        std::vector<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        منطقی m_hasBitSel = نادرست;  // Bit-select was found along the path too
    };

    ساختار ForceRangeInfo final : ForceRange {
        منطقی m_hasArraySel = نادرست;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

خصوصی:
    using ScopeVarCache = std::unordered_map<ثابت AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    ثابت VNUser1InUse m_user1InUse;
    ثابت VNUser2InUse m_user2InUse;

عمومی:
    using ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    using ProtectedVarRefSet = std::unordered_set<ثابت AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    using BuiltVarScopeSet = std::unordered_set<ثابت AstVarScope*>;

خصوصی:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::vector<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, صحیح> m_varToId;
    std::unordered_set<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::vector<ForceInfo*>> m_rhsDepToForces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    منطقی m_doingAssign = نادرست;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    ProtectedVarRefSet& m_permanentlyProtected;
    BuiltVarScopeSet& m_forceRdUpdateBuilt;

عمومی:
    ForceState(منطقی doingAssign, ForceHelperVarsByVar& forceHelperVarsByVar,
               ProtectedVarRefSet& permanentlyProtected, BuiltVarScopeSet& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doingAssign{doingAssign}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    ایستا AstConst* makeZeroConst(Ast_گره* nodep, صحیح width) {
        V3_عدد zero{nodep, width};
        zero.setAllBits0();
        بازگشت جدید AstConst{nodep->fileline(), zero};
    }

    ایستا AstConst* makeConst32(File_سطر* flp, صحیح مقدار) {
        بازگشت جدید AstConst{flp, AstConst::Widthed_مقدار{}, 32, static_cast<uint32_t>(مقدار)};
    }

    ایستا AstConst* makeRangeMaskConst(Ast_گره* nodep, صحیح width, صحیح lsb, صحیح msb) {
        V3_عدد mask{nodep, width};
        mask.setAllBits0();
        برای (صحیح bit = lsb; bit <= msb; ++bit) mask.setBit(bit, 1);
        بازگشت جدید AstConst{nodep->fileline(), mask};
    }

    ایستا Ast_گره_عبارت* zeroPadToBaseWidth(Ast_گره_عبارت* exprp, صحیح baseWidth, صحیح padLsb,
                                           صحیح padMsb) {
        اگر (baseWidth <= 0) بازگشت exprp;
        ثابت صحیح کم_Pad = padLsb;
        ثابت صحیح بالا_Pad = baseWidth - (padMsb + 1);
        اگر (کم_Pad > 0) {
            exprp = جدید AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, کم_Pad)};
        }
        اگر (بالا_Pad > 0) {
            exprp = جدید AstConcat{exprp->fileline(), makeZeroConst(exprp, بالا_Pad), exprp};
        }
        بازگشت exprp;
    }

    ایستا منطقی isUnpackedArrayDType(ثابت Ast_گره_DType* dtypep) {
        بازگشت VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    ایستا منطقی isBitwiseDType(ثابت Ast_گره* nodep) {
        ثابت AstBasicDType* ثابت basicp = nodep->dtypep()->skipRefp()->basicp();
        بازگشت basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    ایستا Ast_گره_عبارت* cast_To_گره_DType(Ast_گره_عبارت* exprp, Ast_گره* dtypeFromp) {
        ثابت Ast_گره_DType* ثابت dtypep = dtypeFromp->dtypep()->skipRefp();
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();
        اگر (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            بازگشت exprp;
        }
        بازگشت جدید AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    ایستا تهی markNonReplaceable(AstVarRef* ثابت nodep) { nodep->user1SetOnce(); }
    منطقی isNotReplaceable(ثابت AstVarRef* ثابت nodep) ثابت {
        بازگشت nodep->user1() || m_permanentlyProtected.شمار(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    تهی markPermanentlyProtected(AstVarRef* ثابت nodep) ثابت {
        markNonReplaceable(nodep);
        m_permanentlyProtected.درج(nodep);
    }

    ایستا std::vector<ForceInfo*> forceInfosInIdOrder(VarForceInfo& info) {
        std::vector<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        برای (auto& it : info.m_forces) forceps.افزودن_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](ثابت ForceInfo* ap, ثابت ForceInfo* bp) {
            بازگشت ap->m_forceId < bp->m_forceId;
        });
        بازگشت forceps;
    }

    ایستا std::vector<ثابت ForceInfo*> forceInfosInIdOrder(ثابت VarForceInfo& info) {
        std::vector<ثابت ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        برای (ثابت auto& it : info.m_forces) forceps.افزودن_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](ثابت ForceInfo* ap, ثابت ForceInfo* bp) {
            بازگشت ap->m_forceId < bp->m_forceId;
        });
        بازگشت forceps;
    }

    ایستا منطقی isOpaquePathSelector(ثابت Ast_گره* nodep) {
        بازگشت VN_IS(nodep, Sel) || VN_IS(nodep, گره_Sel) || VN_IS(nodep, StructSel);
    }

    ایستا منطقی isOutermostOpaquePathSelector(ثابت Ast_گره* nodep) {
        اگر (!isOpaquePathSelector(nodep)) بازگشت نادرست;
        ثابت Ast_گره* ثابت backp = nodep->backp();
        بازگشت !backp || !isOpaquePathSelector(backp);
    }

    ایستا AstVarRef* getOneVarRef(Ast_گره_عبارت* forceStmtp) {
        Ast_گره* ثابت basep = forceStmtp->baseFromp(درست);
        اگر (AstSampled* sampledp = VN_CAST(basep, Sampled))
            اگر (Ast_گره_عبارت* exprp = VN_CAST(sampledp->exprp(), گره_عبارت))
                بازگشت getOneVarRef(exprp);
        AstVarRef* ثابت varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        بازگشت varRefp;
    }

    ایستا Ast_گره_عبارت* buildNestedArraySel(File_سطر* flp, Ast_گره_عبارت* fromp,
                                            ثابت std::vector<صحیح>& indicies) {
        Ast_گره_عبارت* curp = fromp;
        برای (ثابت صحیح idx : indicies) curp = جدید AstArraySel{flp, curp, idx};
        بازگشت curp;
    }

    template <typename Fn>
    ایستا Ast_گره_Stmt* foreachUnpackedLeaf(ثابت std::vector<AstUnpackArrayDType*>& dims,
                                            Fn buildLeaf) {
        Ast_گره_Stmt* headp = nullptr;
        Ast_گره_Stmt* tailp = nullptr;
        اگر (dims.empty()) بازگشت nullptr;
        صحیح total = 1;
        برای (ثابت AstUnpackArrayDType* ثابت d : dims) total *= d->elementsConst();
        اگر (total <= 0) بازگشت nullptr;
        std::vector<صحیح> idx(dims.size(), 0);
        برای (صحیح flat = 0; flat < total; ++flat) {
            Ast_گره_Stmt* ثابت stmtp = buildLeaf(idx, flat);
            اگر (!headp) {
                headp = stmtp;
            } وگرنه {
                tailp->addNext(stmtp);
            }
            tailp = stmtp;
            برای (صحیح d = static_cast<صحیح>(dims.size()) - 1; d >= 0; --d) {
                اگر (++idx[d] < dims[d]->elementsConst()) بشکن;
                idx[d] = 0;
            }
        }
        بازگشت headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_گره_عبارت* lhsp, AstVar* varp,
                                     منطقی requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.empty();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->width() - 1) : 0;

        اگر (ثابت AstSel* ثابت outerSelp = VN_CAST(lhsp, Sel)) {
            صحیح totalLsb = 0;
            برای (Ast_گره_عبارت* curp = lhsp; ثابت AstSel* ثابت selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                اگر (requireConstRangeSelect) {
                    UASSERT_OBJ(VN_IS(selp->lsbp(), Const), lhsp,
                                "Unsupported: force on non-const range select");
                }
                totalLsb += selp->lsbConst();
            }
            info.m_padLsb = totalLsb;
            info.m_padMsb = totalLsb + outerSelp->widthConst() - 1;
        }

        info.m_rangeLsb = info.m_padLsb;
        info.m_rangeMsb = info.m_padMsb;
        اگر (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::vector<صحیح> indices;
            indices.reserve(info.m_arrayInfo.m_sels.size());
            برای (AstArraySel* ثابت selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.افزودن_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            info.m_rangeLsb = flattenIndex(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } وگرنه اگر (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        اگر (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            ثابت صحیح index = varInfo.getOrCreateForcePathIndex(lhsp);
            info.m_rangeLsb = index;
            info.m_rangeMsb = index;
        }
        بازگشت info;
    }

    Ast_گره_عبارت* add_Rhs_مقدار_Reads(ثابت VarForceInfo& varInfo, Ast_گره_عبارت* exprp) ثابت {
        اگر (!doingAssign()) بازگشت exprp;

        ثابت std::vector<ثابت ForceInfo*> forceps = forceInfosInIdOrder(varInfo);
        اگر (forceps.empty()) بازگشت exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* ثابت cexprp = جدید AstCExpr{exprp->fileline(), AstCExpr::Pure{}};
        cexprp->dtypeFrom(exprp);
        cexprp->add("(");
        برای (ثابت ForceInfo* ثابت finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* ثابت refp
                = جدید AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->add("(void)(");
            cexprp->add(refp);
            cexprp->add("), ");
        }
        cexprp->add(exprp);
        cexprp->add(")");
        بازگشت cexprp;
    }

    Ast_گره_عبارت* createForceReadCall(ثابت VarForceInfo& varInfo, File_سطر* flp, VCMethod method,
                                     Ast_گره_عبارت* originalExprp, Ast_گره* dtypeFromp,
                                     Ast_گره_عبارت* indexExprp) ثابت {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_گره_عبارت* ثابت origValp
            = add_Rhs_مقدار_Reads(varInfo, cast_To_گره_DType(originalExprp, dtypeFromp));

        AstCMethodHard* ثابت callp = جدید AstCMethodHard{
            flp, جدید AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, method, origValp};
        اگر (indexExprp) callp->addPinsp(indexExprp);
        callp->dtypeFrom(dtypeFromp);
        بازگشت callp;
    }

    Ast_گره_عبارت* createForceRdExpression(ثابت VarForceInfo& varInfo, File_سطر* flp) ثابت {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        اگر (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            اگر (doingAssign()) { بازگشت جدید AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, درست);
            AstVarRef* ثابت baseRefp = جدید AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(baseRefp);
            بازگشت createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, baseRefp, varp,
                                       nullptr);
        }
        AstVarRef* ثابت baseRefp = جدید AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(baseRefp);
        Ast_گره_عبارت* ثابت enRefp = جدید AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_گره_عبارت* ثابت valRefp = جدید AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        اگر (isBitwiseDType(varp)) {
            بازگشت جدید AstOr{
                flp, جدید AstAnd{flp, enRefp, valRefp},
                جدید AstAnd{flp, جدید AstNot{flp, enRefp->clone_درخت_Pure(نادرست)}, baseRefp}};
        }
        بازگشت جدید AstCond{flp, enRefp, valRefp, baseRefp};
    }

    Ast_گره_Stmt* createForceRdUpdateStmt(ثابت VarForceInfo& varInfo) ثابت {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        اگر (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            بازگشت createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_سطر* ثابت flp = varInfo.m_varVscp->fileline();
        بازگشت جدید AstAssign{flp, جدید AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             createForceRdExpression(varInfo, flp)};
    }

    Ast_گره_Stmt* createForceRdUpdateStmtUnpacked(ثابت VarForceInfo& varInfo) ثابت {
        File_سطر* ثابت flp = varInfo.m_varVscp->fileline();
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* ثابت arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_گره_Stmt* ثابت stmtsp = foreachUnpackedLeaf(
            dims, [&](ثابت std::vector<صحیح>& idx, صحیح /*flat*/) -> Ast_گره_Stmt* {
                AstVarRef* ثابت baseRefp = جدید AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(baseRefp);
                Ast_گره_عبارت* ثابت baseSelp = buildNestedArraySel(flp, baseRefp, idx);
                Ast_گره_عبارت* ثابت enSelp = buildNestedArraySel(
                    flp, جدید AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_گره_عبارت* ثابت valSelp = buildNestedArraySel(
                    flp, جدید AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_گره_عبارت* ثابت readExprp = جدید AstOr{
                    flp, جدید AstAnd{flp, enSelp, valSelp},
                    جدید AstAnd{flp, جدید AstNot{flp, enSelp->clone_درخت_Pure(نادرست)}, baseSelp}};
                Ast_گره_عبارت* ثابت rdLhsSelp = buildNestedArraySel(
                    flp, جدید AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                بازگشت جدید AstAssign{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        برای (ثابت ForceInfo* ثابت finfop : forceInfosInIdOrder(varInfo)) {
            اگر (!finfop->m_isExternal) ادامه;
            stmtsp->addNext(جدید AstAssign{flp,
                                          جدید AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_درخت_Pure(نادرست)});
        }
        بازگشت stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        ثابت auto it = m_varToId.find(vscp);
        اگر (it != m_varToId.end()) بازگشت m_varInfos[it->second];

        m_varToId.emplace(vscp, m_varInfos.size());
        m_varInfos.emplace_back();
        VarForceInfo& info = m_varInfos.back();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* ثابت varp = info.m_varp;
        اگر (!varp->isForceable()) بازگشت info;

        File_سطر* ثابت flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        ثابت منطقی helperVarsBuilt = helperVars.m_rdVarp != nullptr;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != nullptr)
                        && helperVarsBuilt == (helperVars.m_valVarp != nullptr),
                    varp, "Incomplete force helper set");
        اگر (!helperVarsBuilt) {
            ثابت منطقی unpacked = isUnpackedArrayDType(varp->dtypep());
            ثابت VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_گره_DType* ثابت enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
            helperVars.m_rdVarp
                = جدید AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sigPublic(درست);
            helperVars.m_enVarp
                = جدید AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(درست);
            helperVars.m_valVarp
                = جدید AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(درست);
            varp->addNextHere(helperVars.m_rdVarp);
            varp->addNextHere(helperVars.m_enVarp);
            varp->addNextHere(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = findScopeVar(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = findScopeVar(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = findScopeVar(info.m_scopep, helperVars.m_valVarp);
        اگر (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } وگرنه {
            info.m_forceRdVscp = جدید AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = جدید AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = جدید AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->addVarsp(info.m_forceRdVscp);
            info.m_scopep->addVarsp(info.m_forceEnVscp);
            info.m_scopep->addVarsp(info.m_forceValVscp);
        }
        بازگشت info;
    }

    تهی markClockedWrite(AstVar* varp) { m_clockedWrites.درج(varp); }
    منطقی hasClockedWrite(AstVar* varp) ثابت { بازگشت m_clockedWrites.شمار(varp); }

    منطقی doingAssign() ثابت { بازگشت m_doingAssign; }

    ثابت VarForceInfo* getVarInfo(AstVarScope* vscp) ثابت {
        ثابت auto it = m_varToId.find(vscp);
        بازگشت it != m_varToId.end() ? &m_varInfos[it->second] : nullptr;
    }

    AstVarScope* findScopeVar(AstScope* scopep, ثابت AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        اگر (cache.empty()) {
            برای (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        ثابت auto it = cache.find(varp);
        بازگشت it != cache.end() ? it->second : nullptr;
    }
    تهی addForceAssignment(AstVar* varp, AstVarScope* vscp, Ast_گره_عبارت* rhsExprp,
                            AstAssignForce* forceStmtp, صحیح rangeLsb, صحیح rangeMsb, صحیح padLsb,
                            صحیح padMsb, منطقی hasArraySel, منطقی skipRdRefresh = نادرست) {
        v3Global.setUsesForce();
        varp->setForcedByCode();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        ثابت صحیح forceId = info.m_forces.size();
        File_سطر* ثابت flp = varp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        اگر (!info.m_forceVecVscp) {
            AstCDType* ثابت forceVecDtypep = جدید AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->addTypesp(forceVecDtypep);

            AstVar* ثابت forceVecVarp
                = جدید AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doingAssign ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(نادرست);
            forceVecVarp->isInternal(درست);
            varp->addNextHere(forceVecVarp);
            info.m_forceVecVscp = جدید AstVarScope{flp, scopep, forceVecVarp};
            scopep->addVarsp(info.m_forceVecVscp);
        }

        auto pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, nullptr, rhsExprp});
        ForceInfo& finfo = pair.first->second;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        اگر (doingAssign()) {
            std::vector<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* ثابت refp) {
                اگر (!refp->access().isReadOnly()) بازگشت;
                AstVar* ثابت depVarp = refp->varp();
                اگر (depVarp
                    && std::find(depVarps.begin(), depVarps.end(), depVarp) == depVarps.end()) {
                    depVarps.افزودن_back(depVarp);
                }
            });
            برای (AstVar* ثابت depVarp : depVarps) m_rhsDepToForces[depVarp].افزودن_back(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    ایستا تهی collectArraySelInfo(Ast_گره_عبارت* exprp, ArraySelInfo& info) {
        اگر (ثابت auto* ثابت selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = درست;
            collectArraySelInfo(selp->fromp(), info);
        } وگرنه اگر (auto* ثابت selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.افزودن_back(selp);
        } وگرنه اگر (ثابت auto* ثابت memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    ایستا ArraySelInfo getArraySelInfo(Ast_گره_عبارت* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        بازگشت info;
    }

    ایستا std::vector<صحیح> arraySelDimSizes(ثابت ArraySelInfo& info) {
        std::vector<صحیح> dims;
        dims.reserve(info.m_sels.size());
        برای (AstArraySel* ثابت selp : info.m_sels) {
            Ast_گره_DType* ثابت dtypep = selp->fromp()->dtypep()->skipRefp();
            ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.افزودن_back(arrayp->declRange().elements());
        }
        بازگشت dims;
    }

    ایستا صحیح flattenIndex(ثابت std::vector<صحیح>& indices, ثابت std::vector<صحیح>& dimSizes) {
        UASSERT(indices.size() == dimSizes.size(), "Array index and dimension size mismatch");
        صحیح index = 0;
        صحیح stride = 1;
        برای (صحیح i = static_cast<صحیح>(indices.size()) - 1; i >= 0; --i) {
            index += indices[i] * stride;
            stride *= dimSizes[i];
        }
        بازگشت index;
    }

    ایستا Ast_گره_عبارت* build_Flatten_Index_عبارت(File_سطر* flp, ثابت ArraySelInfo& info) {
        ثابت std::vector<صحیح> dimSizes = arraySelDimSizes(info);
        منطقی allConst = درست;
        برای (AstArraySel* ثابت selp : info.m_sels) {
            اگر (!VN_IS(selp->bitp(), Const)) {
                allConst = نادرست;
                بشکن;
            }
        }
        اگر (allConst) {
            std::vector<صحیح> constIndices;
            constIndices.reserve(info.m_sels.size());
            برای (AstArraySel* ثابت selp : info.m_sels) {
                constIndices.افزودن_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            بازگشت makeConst32(flp, flattenIndex(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_گره_عبارت* resultp = nullptr;
        صحیح stride = 1;
        برای (صحیح i = static_cast<صحیح>(info.m_sels.size()) - 1; i >= 0; --i) {
            Ast_گره_عبارت* termp = info.m_sels[i]->bitp()->clone_درخت_Pure(نادرست);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            اگر (termp->width() < 32) termp = جدید AstExtend{flp, termp, 32};
            اگر (stride != 1) termp = جدید AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? جدید AstAdd{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        بازگشت resultp;
    }

    ایستا Ast_گره_عبارت* build_Rhs_Data_عبارت(File_سطر* flp, ثابت ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        بازگشت جدید AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    تهی finalizeRhsVars() {
        برای (VarForceInfo& info : m_varInfos) {
            AstVar* ثابت varp = info.m_varp;
            اگر (info.m_forces.empty()) ادامه;

            AstScope* ثابت scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_سطر* ثابت flp = varp->fileline();
            ثابت std::vector<ForceInfo*> forceps = forceInfosInIdOrder(info);

            برای (ForceInfo* ثابت finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* ثابت rhsVarp = جدید AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doingAssign() ? "_VassignRHS" : "__VforceRHS")
                        + std::to_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(درست);
                rhsVarp->sigPublic(درست);
                rhsVarp->setForcedByCode();
                varp->addNextHere(rhsVarp);
                finfo.m_rhsVarVscp = جدید AstVarScope{flp, scopep, rhsVarp};
                scopep->addVarsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                // end
                AstAssign* ثابت rhsAssignp = جدید AstAssign{
                    flp, جدید AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                اگر (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* ثابت touchCallp = جدید AstCMethodHard{
                        flp, جدید AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtypeSetVoid();
                    Ast_گره_Stmt* ثابت touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->addNextHere(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* ثابت alwaysp
                    = جدید AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, rhsAssignp};
                Ast_Sen_درخت* ثابت senTreep
                    = جدید Ast_Sen_درخت{flp, جدید AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ثابت activep = جدید AstActive{flp, "force-rhs-update", senTreep};
                activep->sen_درخت_Storep(activep->sentreep());
                activep->addStmtsp(alwaysp);
                scopep->addBlocksp(activep);
            }

            اگر (info.m_forceRdVscp) {
                اگر (!m_forceRdUpdateBuilt.درج(info.m_varVscp).second) ادامه;
                AstActive* ثابت activeInitp = جدید AstActive{
                    flp, "force-init",
                    جدید Ast_Sen_درخت{flp, جدید AstSenItem{flp, AstSenItem::Static{}}}};
                activeInitp->sen_درخت_Storep(activeInitp->sentreep());
                Ast_گره_Stmt* initStmtp = nullptr;
                اگر (AstUnpackArrayDType* ثابت arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ثابت std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    ثابت صحیح innerWidth = dims.back()->subDTypep()->skipRefp()->width();
                    initStmtp = foreachUnpackedLeaf(
                        dims, [&](ثابت std::vector<صحیح>& idx, صحیح /*flat*/) -> Ast_گره_Stmt* {
                            Ast_گره_عبارت* ثابت lhsp = buildNestedArraySel(
                                flp, جدید AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            بازگشت جدید AstAssign{flp, lhsp, makeZeroConst(varp, innerWidth)};
                        });
                } وگرنه {
                    initStmtp = جدید AstAssign{
                        flp, جدید AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->width())};
                }
                initStmtp->addNext(createForceRdUpdateStmt(info));
                activeInitp->addStmtsp(جدید AstInitial{flp, initStmtp});
                scopep->addBlocksp(activeInitp);

                AstSenItem* itemsp = nullptr;
                auto addSenItem = [&](AstVarScope* vscp) {
                    اگر (!vscp) بازگشت;
                    AstSenItem* ثابت nextp = جدید AstSenItem{
                        flp, VEdgeType::ET_CHANGED, جدید AstVarRef{flp, vscp, VAccess::READ}};
                    اگر (itemsp) {
                        itemsp->addNext(nextp);
                    } وگرنه {
                        itemsp = nextp;
                    }
                };
                addSenItem(info.m_forceEnVscp);
                addSenItem(info.m_forceValVscp);
                AstVarRef* ثابت origSenRefp = جدید AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                AstSenItem* ثابت origItemp
                    = جدید AstSenItem{flp, VEdgeType::ET_CHANGED, origSenRefp};
                اگر (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->addNext(origItemp);
                برای (ForceInfo* ثابت finfop : forceps) {
                    اگر (!finfop->m_isExternal) addSenItem(finfop->m_rhsVarVscp);
                }

                AstActive* ثابت activep
                    = جدید AstActive{flp, "force-rd-update", جدید Ast_Sen_درخت{flp, itemsp}};
                activep->sen_درخت_Storep(activep->sentreep());
                activep->addStmtsp(جدید AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr,
                                                 createForceRdUpdateStmt(info)});
                scopep->addBlocksp(activep);
            }
        }
    }

    Ast_گره* createRhsUpdatesForWrite(File_سطر* flp, AstVar* writtenVarp) ثابت {
        اگر (!doingAssign()) بازگشت nullptr;

        ثابت auto it = m_rhsDepToForces.find(writtenVarp);
        اگر (it == m_rhsDepToForces.end()) بازگشت nullptr;

        Ast_گره* headp = nullptr;
        Ast_گره* tailp = nullptr;
        برای (ثابت ForceInfo* ثابت finfop : it->second) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            AstAssign* ثابت updatep
                = جدید AstAssign{flp, جدید AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_درخت_Pure(نادرست)};
            اگر (tailp) {
                tailp->addNextHere(updatep);
            } وگرنه {
                headp = updatep;
            }
            tailp = updatep;
        }
        بازگشت headp;
    }

    ثابت ForceInfo& getForceInfo(AstAssignForce* forceStmtp) ثابت {
        AstVarScope* ثابت vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        ثابت VarForceInfo* ثابت varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        auto it2 = varInfo->m_forces.find(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.end(), "Force statement not found");
        بازگشت it2->second;
    }

    ایستا منطقی selOverlapsAnyForce(ثابت VarForceInfo& varInfo, صحیح selLsb, صحیح selMsb) {
        برای (ثابت auto& pair : varInfo.m_forces) {
            اگر (pair.second.m_rangeLsb <= selMsb && pair.second.m_rangeMsb >= selLsb) بازگشت درست;
        }
        بازگشت نادرست;
    }

    Ast_گره_عبارت* createForceReadExpression(ثابت VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) ثابت {
        File_سطر* ثابت flp = originalRefp->fileline();
        AstVarRef* ثابت refp = originalRefp->clone_درخت_Pure(نادرست);
        refp->access(VAccess::READ);
        بازگشت createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   nullptr);
    }

    Ast_گره_عبارت* createForceReadIndexExpression(ثابت VarForceInfo& varInfo,
                                                Ast_گره_عبارت* originalExprp,
                                                Ast_گره_عبارت* indexExprp) ثابت {
        File_سطر* ثابت flp = originalExprp->fileline();
        Ast_گره_عبارت* ثابت exprp = originalExprp->clone_درخت_Pure(نادرست);
        // Must be an LValue to a static variable
        VN_AS(exprp->getVAccessTargetRecurse(), VarRef)->access(VAccess::READ);
        بازگشت createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_INDEX, exprp, originalExprp,
                                   indexExprp);
    }

    ایستا Ast_گره_عبارت* rebuildSelPath(Ast_گره_عبارت* pathp, Ast_گره_عبارت* baseExprp) {
        اگر (ثابت AstSel* ثابت selp = VN_CAST(pathp, Sel)) {
            Ast_گره_عبارت* ثابت fromp = rebuildSelPath(selp->fromp(), baseExprp);
            AstSel* ثابت outp
                = جدید AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->widthConst()};
            outp->dtypeFrom(selp);
            بازگشت outp;
        }
        بازگشت baseExprp;
    }
};

// Split deassign concat LHS before converting to release internals.
ایستا تهی splitDeassign(AstDeassign* nodep) {
    AstConcat* ثابت concatp = VN_CAST(nodep->lhsp(), Concat);
    اگر (!concatp) بازگشت;

    File_سطر* ثابت flp = nodep->fileline();
    AstDeassign* ثابت newLp = جدید AstDeassign{flp, concatp->lhsp()->unlinkFrBack()};
    AstDeassign* ثابت newRp = جدید AstDeassign{flp, concatp->rhsp()->unlinkFrBack()};
    Ast_گره_عبارت* ثابت conp = concatp->unlinkFrBack();
    nodep->replaceWith(newLp);
    newLp->addNextHere(newRp);
    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    VL_DO_DANGLING(conp->delete_درخت(), conp);

    splitDeassign(newLp);
    splitDeassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

رده ForceDiscoveryVisitor final : عمومی VNVisitorConst {
    ForceState& m_state;
    منطقی m_inClockedActive = نادرست;

    تهی buildForceableUnpackedArray(AstVarScope* ثابت nodep,
                                     AstUnpackArrayDType* ثابت arrDtypep) {
        AstVar* ثابت varp = nodep->varp();
        ثابت std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.empty(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        ثابت Ast_گره_DType* ثابت leafDtypep = dims.back()->subDTypep()->skipRefp();
        ثابت AstBasicDType* ثابت innerBasicp = leafDtypep->basicp();
        اگر (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            بازگشت;
        }

        File_سطر* ثابت flp = varp->fileline();
        ثابت صحیح innerWidth = leafDtypep->width();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* ثابت enVscp = info.m_forceEnVscp;
        AstVarScope* ثابت valVscp = info.m_forceValVscp;

        AstSenItem* ثابت itemsp = جدید AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                  جدید AstVarRef{flp, enVscp, VAccess::READ}};
        AstActive* ثابت activep = جدید AstActive{flp, "force-update", جدید Ast_Sen_درخت{flp, itemsp}};
        activep->sen_درخت_Storep(activep->sentreep());

        Ast_گره_Stmt* ثابت alwaysBodyHeadp = ForceState::foreachUnpackedLeaf(
            dims, [&](ثابت std::vector<صحیح>& idx, صحیح flat) -> Ast_گره_Stmt* {
                AstVarRef* ثابت origRefp = جدید AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_گره_عبارت* ثابت origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_گره_عبارت* ثابت enSelp = ForceState::buildNestedArraySel(
                    flp, جدید AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_گره_عبارت* ثابت valSelp = ForceState::buildNestedArraySel(
                    flp, جدید AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_گره_عبارت* ثابت forceExprp = جدید AstOr{
                    flp, جدید AstAnd{flp, enSelp, valSelp},
                    جدید AstAnd{flp, جدید AstNot{flp, enSelp->clone_درخت_Pure(نادرست)}, origSelp}};
                Ast_گره_عبارت* ثابت lhsSelp = ForceState::buildNestedArraySel(
                    flp, جدید AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                AstAssignForce* ثابت forceAssignp = جدید AstAssignForce{flp, lhsSelp, forceExprp};
                forceAssignp->user2(درست);

                Ast_گره_عبارت* ثابت rhsClonep = forceExprp->clone_درخت_Pure(نادرست);
                rhsClonep->foreach([varp](AstVarRef* ثابت r) {
                    اگر (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/innerWidth - 1,
                                           /*hasArraySel=*/درست, /*skipRdRefresh=*/درست);
                بازگشت forceAssignp;
            });
        activep->addStmtsp(جدید AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, alwaysBodyHeadp});
        nodep->scopep()->addBlocksp(activep);
    }

    تهی visit(AstAssignForce* nodep) override {
        اگر (nodep->user2()) بازگشت;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* ثابت forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, درست);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_گره_عبارت* ثابت rhsExprp = nodep->rhsp()->clone_درخت_Pure(نادرست);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    تهی visit(AstAssign* nodep) override {
        اگر (m_state.doingAssign() && m_inClockedActive) {
            اگر (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    تهی visit(AstActive* nodep) override {
        VL_RESTORER(m_inClockedActive);
        m_inClockedActive = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    تهی visit(AstVarScope* nodep) override {
        اگر (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            اگر (m_state.doingAssign()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                بازگشت;
            }

            اگر (AstUnpackArrayDType* ثابت arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                بازگشت;
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
            AstSenItem* ثابت itemsp = جدید AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                      جدید AstVarRef{flp, enVscp, VAccess::READ}};
            AstActive* ثابت activep
                = جدید AstActive{flp, "force-update", جدید Ast_Sen_درخت{flp, itemsp}};
            activep->sen_درخت_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* ثابت refp = جدید AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* ثابت enRefp = جدید AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* ثابت valRefp = جدید AstVarRef{flp, valVscp, VAccess::READ};
            ثابت AstBasicDType* ثابت basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_گره_عبارت* ثابت forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_گره_عبارت*>(جدید AstOr{
                            flp, جدید AstAnd{flp, enRefp, valRefp},
                            جدید AstAnd{flp, جدید AstNot{flp, enRefp->clone_درخت_Pure(نادرست)}, refp}})
                      : static_cast<Ast_گره_عبارت*>(جدید AstCond{flp, enRefp, valRefp, refp});
            AstAssignForce* ثابت forceAssignp
                = جدید AstAssignForce{flp, جدید AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(درست);
            activep->addStmtsp(جدید AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, forceAssignp});
            nodep->scopep()->addBlocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_گره_عبارت* ثابت rhsClonep = forceExprp->clone_درخت_Pure(نادرست);
            rhsClonep->foreach([varp](AstVarRef* ثابت refp) {
                اگر (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            ثابت منطقی bitwiseVar = ForceState::isBitwiseDType(varp);
            ثابت صحیح padMsb = bitwiseVar ? (varp->width() - 1) : 0;
            صحیح rangeLsb = 0;
            صحیح rangeMsb = padMsb;
            اگر (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, نادرست);
        }
        iterateChildrenConst(nodep);
    }

    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عمومی:
    explicit ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextConstNull(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

رده ForceConvertVisitor final : عمومی VNVisitor {
    ForceState& m_state;

    تهی visit(AstAssignForce* nodep) override {
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
        AstAssign* ثابت rhsAssignp
            = جدید AstAssign{flp, جدید AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_درخت_Pure(نادرست)};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        ثابت منطقی bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        اگر (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_گره_عبارت* baseExprp = nodep->rhsp()->clone_درخت_Pure(نادرست);
            baseExprp->foreach(
                [](AstVarRef* ثابت refp) { ForceState::markNonReplaceable(refp); });
            اگر (bitwiseForcedVar) {
                baseExprp = ForceState::zeroPadToBaseWidth(baseExprp, forcedVarp->width(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            اگر (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* ثابت maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_گره_عبارت* ثابت valReadp
                    = جدید AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_گره_عبارت* ثابت valWritep
                    = جدید AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_گره_عبارت* ثابت notMaskp = جدید AstNot{flp, maskConstp};
                Ast_گره_عبارت* ثابت maskedOldp = جدید AstAnd{flp, valReadp, notMaskp};
                Ast_گره_عبارت* ثابت newValp = جدید AstOr{flp, maskedOldp, baseExprp};
                valAssignp = جدید AstAssign{flp, valWritep, newValp};

                Ast_گره_عبارت* ثابت enReadp
                    = جدید AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_گره_عبارت* ثابت enWritep
                    = جدید AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_گره_عبارت* ثابت newEnp
                    = جدید AstOr{flp, enReadp, maskConstp->clone_درخت_Pure(نادرست)};
                enAssignp = جدید AstAssign{flp, enWritep, newEnp};
            } وگرنه {
                AstConst* ثابت oneConstp = ForceState::makeRangeMaskConst(nodep, 1, 0, 0);
                Ast_گره_عبارت* ثابت rhsValp = ForceState::cast_To_گره_DType(baseExprp, forcedVarp);
                valAssignp = جدید AstAssign{
                    flp, جدید AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = جدید AstAssign{
                    flp, جدید AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        ثابت AstSel* ثابت selLhsp = VN_CAST(lhsp, Sel);
        ثابت منطقی arrayBitSel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->width();
        Ast_گره_عبارت* ثابت rhsDatap = ForceState::build_Rhs_Data_عبارت(flp, info);
        AstCExpr* ثابت rhsAddrp = جدید AstCExpr{flp};
        rhsAddrp->add("&(");
        rhsAddrp->add(rhsDatap);
        rhsAddrp->add(")");
        AstCMethodHard* ثابت addForceCallp = جدید AstCMethodHard{
            flp, جدید AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ADD,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        addForceCallp->addPinsp(rhsAddrp);
        addForceCallp->addPinsp(
            ForceState::makeConst32(flp, arrayBitSel ? info.m_padLsb : info.m_rangeLsb));
        اگر (arrayBitSel) {
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padLsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padMsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, selLhsp->fromp()->width()));
        }
        addForceCallp->dtypeSetVoid();
        Ast_گره_Stmt* ثابت stmtp = addForceCallp->makeStmt();

        Ast_گره* tailp = rhsAssignp;
        اگر (valAssignp) {
            tailp->addNextHere(valAssignp);
            tailp = valAssignp;
        }
        اگر (enAssignp) {
            tailp->addNextHere(enAssignp);
            tailp = enAssignp;
        }
        tailp->addNextHere(stmtp);
        اگر (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp();
        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ثابت releasedVarp = lhsVarRefp->varp();

        ثابت ForceState::VarForceInfo* ثابت varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        اگر (!varInfo) {
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }
        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        File_سطر* ثابت flp = nodep->fileline();

        ثابت ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, نادرست);

        ثابت AstSel* ثابت selLhsp = VN_CAST(lhsp, Sel);
        ثابت منطقی arrayBitSel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->width();
        AstCMethodHard* ثابت releaseCallp = جدید AstCMethodHard{
            flp, جدید AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        اگر (arrayBitSel) {
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtypeSetVoid();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_گره_Stmt* ثابت releasep = releaseCallp->makeStmt();

        AstAssign* clearEnp = nullptr;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        اگر (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_گره_عبارت* ثابت enWritep
                = جدید AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            اگر (ForceState::isBitwiseDType(releasedVarp)) {
                ثابت صحیح varWidth = releasedVarp->width();
                اگر (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == varWidth - 1) {
                    clearEnp
                        = جدید AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, varWidth)};
                } وگرنه {
                    // forceEn = forceEn & ~mask(range);
                    Ast_گره_عبارت* ثابت enReadp
                        = جدید AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* ثابت maskConst = ForceState::makeRangeMaskConst(
                        nodep, varWidth, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_گره_عبارت* ثابت newEnp
                        = جدید AstAnd{flp, enReadp, جدید AstNot{flp, maskConst}};
                    clearEnp = جدید AstAssign{flp, enWritep, newEnp};
                }
            } وگرنه {
                clearEnp = جدید AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        ثابت AstSel* ثابت selp = VN_CAST(lhsp, Sel);
        Ast_گره_عبارت* ثابت basep = selp ? selp->fromp() : lhsp;

        Ast_گره* stmtListp = releasep;
        اگر (clearEnp) {
            clearEnp->addNextHere(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        ثابت منطقی fullBitwiseRelease
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->width() - 1;
        اگر (!releasedVarp->isContinuously()
            && !(m_state.doingAssign() && m_state.hasClockedWrite(releasedVarp)
                 && fullBitwiseRelease)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            ثابت منطقی hasOpaquePath = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_گره_عبارت* forceReadp = nullptr;
            اگر (hasOpaquePath) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } وگرنه اگر (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, basep,
                    ForceState::build_Flatten_Index_عبارت(flp, rangeInfo.m_arrayInfo));
                اگر (selp) { forceReadp = ForceState::rebuildSelPath(lhsp, forceReadp); }
            } وگرنه {
                forceReadp
                    = selp ? ForceState::rebuildSelPath(
                                 lhsp, m_state.createForceReadExpression(*varInfo, lhsVarRefp))
                           : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            }
            AstAssign* ثابت assignp = جدید AstAssign{flp, lhsp->clone_درخت_Pure(نادرست), forceReadp};
            assignp->addNextHere(stmtListp);
            stmtListp = assignp;
        }

        اگر (varInfo->m_forceRdVscp) stmtListp->addNext(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

رده ForceReplaceVisitor final : عمومی VNVisitor {
    ثابت ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_گره_Stmt* m_stmtp = nullptr;
    منطقی m_inLogic = نادرست;

    تهی iterateLogic(Ast_گره* nodep) {
        VL_RESTORER(m_inLogic);
        m_inLogic = درست;
        iterateChildren(nodep);
    }

    تهی visit(Ast_گره_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstAssign* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        اگر (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp()->baseFromp(درست), VarRef)) {
            اگر (Ast_گره* ثابت updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->addNextHere(updatep);
            }
        }
    }
    تهی visit(AstAssignCont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateAndNextNull(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    تهی visit(AstDeassign*) override {}
    تهی visit(AstCFunc* nodep) override { iterateLogic(nodep); }
    تهی visit(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    تهی visit(Ast_گره_Procedure* nodep) override { iterateLogic(nodep); }
    تهی visit(AstAlways* nodep) override {
        اگر (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            بازگشت;
        }
        iterateLogic(nodep);
    }
    تهی visit(AstSenItem* nodep) override { iterateLogic(nodep); }
    تهی visit(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef);
        اگر (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            visit(static_cast<Ast_گره*>(nodep));
            بازگشت;
        }

        AstVar* ثابت varp = refp->varp();
        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(refp->varScopep());
        اگر (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.empty()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            visit(static_cast<Ast_گره*>(nodep));
            بازگشت;
        }

        اگر (ثابت AstConst* ثابت lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            ثابت صحیح selLsb = lsbConstp->toSInt();
            ثابت صحیح selMsb = selLsb + nodep->width() - 1;
            اگر (!varp->isSigPublic()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                visit(static_cast<Ast_گره*>(nodep));
                بازگشت;
            }
        }

        File_سطر* ثابت flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* ثابت refClonep = refp->clone_درخت_Pure(نادرست);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* ثابت callp = جدید AstCMethodHard{
            flp, جدید AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->width())};
        callp->addPinsp(refClonep);
        callp->addPinsp(nodep->lsbp()->clone_درخت_Pure(نادرست));
        callp->addPinsp(ForceState::makeConst32(flp, nodep->width()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstArraySel* nodep) override {
        اگر (ثابت AstArraySel* ثابت backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            اگر (backSelp->fromp() == nodep) {
                iterateChildren(nodep);
                بازگشت;
            }
        }

        Ast_گره* ثابت basep = nodep->baseFromp(درست);
        AstVarRef* ثابت baseRefp = VN_CAST(basep, VarRef);
        اگر (!baseRefp) {
            iterateChildren(nodep);
            بازگشت;
        }
        AstVar* ثابت varp = baseRefp->varp();
        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(baseRefp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        اگر (m_state.isNotReplaceable(baseRefp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            بازگشت;
        }

        اگر (!baseRefp->access().isReadOnly()) {
            iterateChildren(nodep);
            بازگشت;
        }
        اگر (m_state.doingAssign() && varInfo->m_forceRdVscp) {
            baseRefp->varp(varInfo->m_forceRdVscp->varp());
            baseRefp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            بازگشت;
        }
        ثابت ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        برای (AstArraySel* ثابت selp : arrayInfo.m_sels) iterateAndNextNull(selp->bitp());
        Ast_گره_عبارت* ثابت indexExprp
            = ForceState::build_Flatten_Index_عبارت(nodep->fileline(), arrayInfo);
        Ast_گره_عبارت* ثابت readExprp
            = m_state.createForceReadIndexExpression(*varInfo, nodep, indexExprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی visit(AstVarRef* nodep) override {
        اگر (m_state.isNotReplaceable(nodep)) بازگشت;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        اگر (ثابت AstArraySel* ثابت backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            اگر (backSelp->fromp() == nodep) بازگشت;
        }

        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(nodep->varScopep());
        اگر (!varInfo) بازگشت;

        اگر (varInfo->m_forceRdVscp) {
            اگر (nodep->access().isRW()) {
                اگر (m_inLogic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                بازگشت;
            }
            اگر (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_گره_عبارت* ثابت readp
                    = m_state.createForceRdExpression(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت;
            }
            اگر (m_inLogic && m_stmtp) {
                m_stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
            }
            بازگشت;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        اگر (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            بازگشت;
        }

        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        اگر (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } وگرنه اگر (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_گره_عبارت* ثابت readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    تهی visit(Ast_گره* nodep) override {
        اگر (ForceState::isOutermostOpaquePathSelector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_گره_عبارت* ثابت exprp = VN_AS(nodep, گره_عبارت);
            Ast_گره* ثابت basep = exprp->baseFromp(درست);
            AstVarRef* ثابت baseRefp = VN_CAST(basep, VarRef);
            اگر (baseRefp) {
                AstVar* ثابت varp = baseRefp->varp();
                اگر (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    ثابت ForceState::VarForceInfo* ثابت varInfo
                        = m_state.getVarInfo(baseRefp->varScopep());
                    اگر (!m_state.isNotReplaceable(baseRefp) && varInfo) {
                        ثابت صحیح forcePathIndex = varInfo->findForcePathIndex(exprp);
                        اگر (forcePathIndex >= 0) {
                            اگر (!baseRefp->access().isReadOnly()) بازگشت;
                            Ast_گره_عبارت* ثابت readExprp = m_state.createForceReadIndexExpression(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), forcePathIndex));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                            بازگشت;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

عمومی:
    explicit ForceReplaceVisitor(AstNetlist* nodep, ثابت ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
    ~ForceReplaceVisitor() override {
        V3Stats::addStat("Non-overlapping force sels", m_nonOverlappingForceSels);
    }
};
//######################################################################
//

//######################################################################
// V3Force - Main entry point

ایستا تهی forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::ProtectedVarRefSet& permanentlyProtected,
                         ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    اگر (!v3Global.hasForceableSignals()) بازگشت;
    ForceState state{نادرست, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_درخت("force", 0, dump_درخت_Either_Level() >= 3);
}

ایستا تهی assignAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::ProtectedVarRefSet& permanentlyProtected,
                          ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    اگر (!v3Global.hasAssignDeassign()) بازگشت;

    std::vector<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.افزودن_back(deassignp); });
    برای (AstDeassign* ثابت deassignp : deassignps) splitDeassign(deassignp);

    std::vector<AstAssignCont*> assignContps;
    deassignps.clear();
    nodep->foreach([&](Ast_گره_Stmt* nodep) {
        اگر (AstAssignCont* ثابت assignContp = VN_CAST(nodep, AssignCont)) {
            assignContps.افزودن_back(assignContp);
        } وگرنه اگر (AstDeassign* ثابت deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.افزودن_back(deassignp);
        }
    });

    برای (AstAssignCont* ثابت assignp : assignContps) {
        assignp->replaceWith(جدید AstAssignForce{assignp->fileline(),
                                                assignp->lhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
        VL_DO_DANGLING(assignp->delete_درخت(), assignp);
    }
    برای (AstDeassign* ثابت deassignp : deassignps) {
        deassignp->replaceWith(
            جدید AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_درخت_Pure(درست)});
        VL_DO_DANGLING(deassignp->delete_درخت(), deassignp);
    }
    ForceState state{درست, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_درخت("assign-deassign", 0, dump_درخت_Either_Level() >= 3);
}

تهی V3Force::forceAndAssignAll(AstNetlist* nodep) {
    ثابت VNUser3InUse user3InUse;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::ProtectedVarRefSet permanentlyProtected;
    ForceState::BuiltVarScopeSet forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    assignAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
