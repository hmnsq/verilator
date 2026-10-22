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
        AstVarScope* m_rhsVarVscp = nullptr;  // Scope of the var containing RHSID
        AstNodeExpr* m_rhsExprp = nullptr;  // Expression on RHS of this force assignment

        ForceInfo() = افتراضي;
        ForceInfo(صحيح rangeLsb, صحيح rangeMsb, صحيح padLsb, صحيح padMsb, صحيح forceId,
                  منطقي hasArraySel, AstVarScope* rhsVarVscp, AstNodeExpr* rhsExprp)
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
        AstVarScope* m_forceVecVscp = nullptr;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = nullptr;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = nullptr;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = nullptr;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = nullptr;  // Scope of the forced (original) variable
        AstVar* m_varp = nullptr;  // The forced (original) variable
        AstScope* m_scopep = nullptr;  // Scope containing the forced variable
        std::unordered_map<AstAssignForce*, ForceInfo> m_forces;
        std::unordered_map<string, صحيح> m_forcePathToIndex;
        صحيح m_nextForcePathIndex = 1;  // Start at 1 so 0 can be the base path (whole signal)

    خاص:
        ساكن فراغ buildForcePathKeyRecurse(AstNodeExpr* nodep, string& out) {
            𐤀𐤌 (VN_IS(nodep, VarRef)) أرجع;
            𐤀𐤌 (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
                buildForcePathKeyRecurse(selp->fromp(), out);
                out += "|S" + cvtToStr(selp->lsbConst()) + ":" + cvtToStr(selp->widthConst());
                أرجع;
            }
            𐤀𐤌 (AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
                AstNodeExpr* ثابت fromp = selp->fromp();
                buildForcePathKeyRecurse(fromp, out);
                out += "|T" + selp->name();
                أرجع;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        ساكن string forcePathKey(AstNodeExpr* nodep) {
            string out;
            buildForcePathKeyRecurse(nodep, out);
            أرجع out;
        }

    عام:
        صحيح getOrCreateForcePathIndex(AstNodeExpr* nodep) {
            ثابت auto pair
                = m_forcePathToIndex.emplace(forcePathKey(nodep), m_nextForcePathIndex);
            𐤀𐤌 (pair.second) ++m_nextForcePathIndex;
            أرجع pair.first->second;
        }

        صحيح findForcePathIndex(AstNodeExpr* nodep) ثابت {
            ثابت auto it = m_forcePathToIndex.find(forcePathKey(nodep));
            أرجع it != m_forcePathToIndex.end() ? it->second : -1;
        }
    };

    بنية ForceHelperVars final {
        AstVar* m_rdVarp = nullptr;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = nullptr;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = nullptr;  // The __VforceVal (force value) variable
    };

    بنية ArraySelInfo final {
        std::vector<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        منطقي m_hasBitSel = خطأ;  // Bit-select was found along the path too
    };

    بنية ForceRangeInfo final : ForceRange {
        منطقي m_hasArraySel = خطأ;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

خاص:
    using ScopeVarCache = std::unordered_map<ثابت AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    ثابت VNUser1InUse m_user1InUse;
    ثابت VNUser2InUse m_user2InUse;

عام:
    using ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    using ProtectedVarRefSet = std::unordered_set<ثابت AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    using BuiltVarScopeSet = std::unordered_set<ثابت AstVarScope*>;

خاص:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::vector<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, صحيح> m_varToId;
    std::unordered_set<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::vector<ForceInfo*>> m_rhsDepToForces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    منطقي m_doingAssign = خطأ;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    ProtectedVarRefSet& m_permanentlyProtected;
    BuiltVarScopeSet& m_forceRdUpdateBuilt;

عام:
    ForceState(منطقي doingAssign, ForceHelperVarsByVar& forceHelperVarsByVar,
               ProtectedVarRefSet& permanentlyProtected, BuiltVarScopeSet& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doingAssign{doingAssign}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    ساكن AstConst* makeZeroConst(AstNode* nodep, صحيح width) {
        V3Number zero{nodep, width};
        zero.setAllBits0();
        أرجع جديد AstConst{nodep->fileline(), zero};
    }

    ساكن AstConst* makeConst32(FileLine* flp, صحيح value) {
        أرجع جديد AstConst{flp, AstConst::WidthedValue{}, 32, static_cast<uint32_t>(value)};
    }

    ساكن AstConst* makeRangeMaskConst(AstNode* nodep, صحيح width, صحيح lsb, صحيح msb) {
        V3Number mask{nodep, width};
        mask.setAllBits0();
        لكل (صحيح bit = lsb; bit <= msb; ++bit) mask.setBit(bit, 1);
        أرجع جديد AstConst{nodep->fileline(), mask};
    }

    ساكن AstNodeExpr* zeroPadToBaseWidth(AstNodeExpr* exprp, صحيح baseWidth, صحيح padLsb,
                                           صحيح padMsb) {
        𐤀𐤌 (baseWidth <= 0) أرجع exprp;
        ثابت صحيح lowPad = padLsb;
        ثابت صحيح highPad = baseWidth - (padMsb + 1);
        𐤀𐤌 (lowPad > 0) {
            exprp = جديد AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, lowPad)};
        }
        𐤀𐤌 (highPad > 0) {
            exprp = جديد AstConcat{exprp->fileline(), makeZeroConst(exprp, highPad), exprp};
        }
        أرجع exprp;
    }

    ساكن منطقي isUnpackedArrayDType(ثابت AstNodeDType* dtypep) {
        أرجع VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    ساكن منطقي isBitwiseDType(ثابت AstNode* nodep) {
        ثابت AstBasicDType* ثابت basicp = nodep->dtypep()->skipRefp()->basicp();
        أرجع basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    ساكن AstNodeExpr* castToNodeDType(AstNodeExpr* exprp, AstNode* dtypeFromp) {
        ثابت AstNodeDType* ثابت dtypep = dtypeFromp->dtypep()->skipRefp();
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();
        𐤀𐤌 (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            أرجع exprp;
        }
        أرجع جديد AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    ساكن فراغ markNonReplaceable(AstVarRef* ثابت nodep) { nodep->user1SetOnce(); }
    منطقي isNotReplaceable(ثابت AstVarRef* ثابت nodep) ثابت {
        أرجع nodep->user1() || m_permanentlyProtected.count(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    فراغ markPermanentlyProtected(AstVarRef* ثابت nodep) ثابت {
        markNonReplaceable(nodep);
        m_permanentlyProtected.insert(nodep);
    }

    ساكن std::vector<ForceInfo*> forceInfosInIdOrder(VarForceInfo& info) {
        std::vector<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        لكل (auto& it : info.m_forces) forceps.push_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](ثابت ForceInfo* ap, ثابت ForceInfo* bp) {
            أرجع ap->m_forceId < bp->m_forceId;
        });
        أرجع forceps;
    }

    ساكن std::vector<ثابت ForceInfo*> forceInfosInIdOrder(ثابت VarForceInfo& info) {
        std::vector<ثابت ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        لكل (ثابت auto& it : info.m_forces) forceps.push_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](ثابت ForceInfo* ap, ثابت ForceInfo* bp) {
            أرجع ap->m_forceId < bp->m_forceId;
        });
        أرجع forceps;
    }

    ساكن منطقي isOpaquePathSelector(ثابت AstNode* nodep) {
        أرجع VN_IS(nodep, Sel) || VN_IS(nodep, NodeSel) || VN_IS(nodep, StructSel);
    }

    ساكن منطقي isOutermostOpaquePathSelector(ثابت AstNode* nodep) {
        𐤀𐤌 (!isOpaquePathSelector(nodep)) أرجع خطأ;
        ثابت AstNode* ثابت backp = nodep->backp();
        أرجع !backp || !isOpaquePathSelector(backp);
    }

    ساكن AstVarRef* getOneVarRef(AstNodeExpr* forceStmtp) {
        AstNode* ثابت basep = forceStmtp->baseFromp(true);
        𐤀𐤌 (AstSampled* sampledp = VN_CAST(basep, Sampled))
            𐤀𐤌 (AstNodeExpr* exprp = VN_CAST(sampledp->exprp(), NodeExpr))
                أرجع getOneVarRef(exprp);
        AstVarRef* ثابت varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        أرجع varRefp;
    }

    ساكن AstNodeExpr* buildNestedArraySel(FileLine* flp, AstNodeExpr* fromp,
                                            ثابت std::vector<صحيح>& indicies) {
        AstNodeExpr* curp = fromp;
        لكل (ثابت صحيح idx : indicies) curp = جديد AstArraySel{flp, curp, idx};
        أرجع curp;
    }

    template <typename Fn>
    ساكن AstNodeStmt* foreachUnpackedLeaf(ثابت std::vector<AstUnpackArrayDType*>& dims,
                                            Fn buildLeaf) {
        AstNodeStmt* headp = nullptr;
        AstNodeStmt* tailp = nullptr;
        𐤀𐤌 (dims.empty()) أرجع nullptr;
        صحيح total = 1;
        لكل (ثابت AstUnpackArrayDType* ثابت d : dims) total *= d->elementsConst();
        𐤀𐤌 (total <= 0) أرجع nullptr;
        std::vector<صحيح> idx(dims.size(), 0);
        لكل (صحيح flat = 0; flat < total; ++flat) {
            AstNodeStmt* ثابت stmtp = buildLeaf(idx, flat);
            𐤀𐤌 (!headp) {
                headp = stmtp;
            } وإلا {
                tailp->addNext(stmtp);
            }
            tailp = stmtp;
            لكل (صحيح d = static_cast<صحيح>(dims.size()) - 1; d >= 0; --d) {
                𐤀𐤌 (++idx[d] < dims[d]->elementsConst()) اكسر;
                idx[d] = 0;
            }
        }
        أرجع headp;
    }

    ForceRangeInfo getForceRangeInfo(AstNodeExpr* lhsp, AstVar* varp,
                                     منطقي requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.empty();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->width() - 1) : 0;

        𐤀𐤌 (ثابت AstSel* ثابت outerSelp = VN_CAST(lhsp, Sel)) {
            صحيح totalLsb = 0;
            لكل (AstNodeExpr* curp = lhsp; ثابت AstSel* ثابت selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                𐤀𐤌 (requireConstRangeSelect) {
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
        𐤀𐤌 (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::vector<صحيح> indices;
            indices.reserve(info.m_arrayInfo.m_sels.size());
            لكل (AstArraySel* ثابت selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.push_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            info.m_rangeLsb = flattenIndex(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } وإلا 𐤀𐤌 (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        𐤀𐤌 (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            ثابت صحيح index = varInfo.getOrCreateForcePathIndex(lhsp);
            info.m_rangeLsb = index;
            info.m_rangeMsb = index;
        }
        أرجع info;
    }

    AstNodeExpr* addRhsValueReads(ثابت VarForceInfo& varInfo, AstNodeExpr* exprp) ثابت {
        𐤀𐤌 (!doingAssign()) أرجع exprp;

        ثابت std::vector<ثابت ForceInfo*> forceps = forceInfosInIdOrder(varInfo);
        𐤀𐤌 (forceps.empty()) أرجع exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* ثابت cexprp = جديد AstCExpr{exprp->fileline(), AstCExpr::Pure{}};
        cexprp->dtypeFrom(exprp);
        cexprp->add("(");
        لكل (ثابت ForceInfo* ثابت finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* ثابت refp
                = جديد AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->add("(void)(");
            cexprp->add(refp);
            cexprp->add("), ");
        }
        cexprp->add(exprp);
        cexprp->add(")");
        أرجع cexprp;
    }

    AstNodeExpr* createForceReadCall(ثابت VarForceInfo& varInfo, FileLine* flp, VCMethod method,
                                     AstNodeExpr* originalExprp, AstNode* dtypeFromp,
                                     AstNodeExpr* indexExprp) ثابت {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        AstNodeExpr* ثابت origValp
            = addRhsValueReads(varInfo, castToNodeDType(originalExprp, dtypeFromp));

        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, method, origValp};
        𐤀𐤌 (indexExprp) callp->addPinsp(indexExprp);
        callp->dtypeFrom(dtypeFromp);
        أرجع callp;
    }

    AstNodeExpr* createForceRdExpression(ثابت VarForceInfo& varInfo, FileLine* flp) ثابت {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        𐤀𐤌 (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            𐤀𐤌 (doingAssign()) { أرجع جديد AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, true);
            AstVarRef* ثابت baseRefp = جديد AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(baseRefp);
            أرجع createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, baseRefp, varp,
                                       nullptr);
        }
        AstVarRef* ثابت baseRefp = جديد AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(baseRefp);
        AstNodeExpr* ثابت enRefp = جديد AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        AstNodeExpr* ثابت valRefp = جديد AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        𐤀𐤌 (isBitwiseDType(varp)) {
            أرجع جديد AstOr{
                flp, جديد AstAnd{flp, enRefp, valRefp},
                جديد AstAnd{flp, جديد AstNot{flp, enRefp->cloneTreePure(خطأ)}, baseRefp}};
        }
        أرجع جديد AstCond{flp, enRefp, valRefp, baseRefp};
    }

    AstNodeStmt* createForceRdUpdateStmt(ثابت VarForceInfo& varInfo) ثابت {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        𐤀𐤌 (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            أرجع createForceRdUpdateStmtUnpacked(varInfo);
        }
        FileLine* ثابت flp = varInfo.m_varVscp->fileline();
        أرجع جديد AstAssign{flp, جديد AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             createForceRdExpression(varInfo, flp)};
    }

    AstNodeStmt* createForceRdUpdateStmtUnpacked(ثابت VarForceInfo& varInfo) ثابت {
        FileLine* ثابت flp = varInfo.m_varVscp->fileline();
        AstVar* ثابت varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* ثابت arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        ثابت std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        AstNodeStmt* ثابت stmtsp = foreachUnpackedLeaf(
            dims, [&](ثابت std::vector<صحيح>& idx, صحيح /*flat*/) -> AstNodeStmt* {
                AstVarRef* ثابت baseRefp = جديد AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(baseRefp);
                AstNodeExpr* ثابت baseSelp = buildNestedArraySel(flp, baseRefp, idx);
                AstNodeExpr* ثابت enSelp = buildNestedArraySel(
                    flp, جديد AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                AstNodeExpr* ثابت valSelp = buildNestedArraySel(
                    flp, جديد AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                AstNodeExpr* ثابت readExprp = جديد AstOr{
                    flp, جديد AstAnd{flp, enSelp, valSelp},
                    جديد AstAnd{flp, جديد AstNot{flp, enSelp->cloneTreePure(خطأ)}, baseSelp}};
                AstNodeExpr* ثابت rdLhsSelp = buildNestedArraySel(
                    flp, جديد AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                أرجع جديد AstAssign{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        لكل (ثابت ForceInfo* ثابت finfop : forceInfosInIdOrder(varInfo)) {
            𐤀𐤌 (!finfop->m_isExternal) استمر;
            stmtsp->addNext(جديد AstAssign{flp,
                                          جديد AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->cloneTreePure(خطأ)});
        }
        أرجع stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        ثابت auto it = m_varToId.find(vscp);
        𐤀𐤌 (it != m_varToId.end()) أرجع m_varInfos[it->second];

        m_varToId.emplace(vscp, m_varInfos.size());
        m_varInfos.emplace_back();
        VarForceInfo& info = m_varInfos.back();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* ثابت varp = info.m_varp;
        𐤀𐤌 (!varp->isForceable()) أرجع info;

        FileLine* ثابت flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        ثابت منطقي helperVarsBuilt = helperVars.m_rdVarp != nullptr;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != nullptr)
                        && helperVarsBuilt == (helperVars.m_valVarp != nullptr),
                    varp, "Incomplete force helper set");
        𐤀𐤌 (!helperVarsBuilt) {
            ثابت منطقي unpacked = isUnpackedArrayDType(varp->dtypep());
            ثابت VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            AstNodeDType* ثابت enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
            helperVars.m_rdVarp
                = جديد AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sigPublic(true);
            helperVars.m_enVarp
                = جديد AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(true);
            helperVars.m_valVarp
                = جديد AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(true);
            varp->addNextHere(helperVars.m_rdVarp);
            varp->addNextHere(helperVars.m_enVarp);
            varp->addNextHere(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = findScopeVar(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = findScopeVar(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = findScopeVar(info.m_scopep, helperVars.m_valVarp);
        𐤀𐤌 (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } وإلا {
            info.m_forceRdVscp = جديد AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = جديد AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = جديد AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->addVarsp(info.m_forceRdVscp);
            info.m_scopep->addVarsp(info.m_forceEnVscp);
            info.m_scopep->addVarsp(info.m_forceValVscp);
        }
        أرجع info;
    }

    فراغ markClockedWrite(AstVar* varp) { m_clockedWrites.insert(varp); }
    منطقي hasClockedWrite(AstVar* varp) ثابت { أرجع m_clockedWrites.count(varp); }

    منطقي doingAssign() ثابت { أرجع m_doingAssign; }

    ثابت VarForceInfo* getVarInfo(AstVarScope* vscp) ثابت {
        ثابت auto it = m_varToId.find(vscp);
        أرجع it != m_varToId.end() ? &m_varInfos[it->second] : nullptr;
    }

    AstVarScope* findScopeVar(AstScope* scopep, ثابت AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        𐤀𐤌 (cache.empty()) {
            لكل (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        ثابت auto it = cache.find(varp);
        أرجع it != cache.end() ? it->second : nullptr;
    }
    فراغ addForceAssignment(AstVar* varp, AstVarScope* vscp, AstNodeExpr* rhsExprp,
                            AstAssignForce* forceStmtp, صحيح rangeLsb, صحيح rangeMsb, صحيح padLsb,
                            صحيح padMsb, منطقي hasArraySel, منطقي skipRdRefresh = خطأ) {
        v3Global.setUsesForce();
        varp->setForcedByCode();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        ثابت صحيح forceId = info.m_forces.size();
        FileLine* ثابت flp = varp->fileline();
        AstScope* ثابت scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        𐤀𐤌 (!info.m_forceVecVscp) {
            AstCDType* ثابت forceVecDtypep = جديد AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->addTypesp(forceVecDtypep);

            AstVar* ثابت forceVecVarp
                = جديد AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doingAssign ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(خطأ);
            forceVecVarp->isInternal(true);
            varp->addNextHere(forceVecVarp);
            info.m_forceVecVscp = جديد AstVarScope{flp, scopep, forceVecVarp};
            scopep->addVarsp(info.m_forceVecVscp);
        }

        auto pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, nullptr, rhsExprp});
        ForceInfo& finfo = pair.first->second;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        𐤀𐤌 (doingAssign()) {
            std::vector<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* ثابت refp) {
                𐤀𐤌 (!refp->access().isReadOnly()) أرجع;
                AstVar* ثابت depVarp = refp->varp();
                𐤀𐤌 (depVarp
                    && std::find(depVarps.begin(), depVarps.end(), depVarp) == depVarps.end()) {
                    depVarps.push_back(depVarp);
                }
            });
            لكل (AstVar* ثابت depVarp : depVarps) m_rhsDepToForces[depVarp].push_back(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    ساكن فراغ collectArraySelInfo(AstNodeExpr* exprp, ArraySelInfo& info) {
        𐤀𐤌 (ثابت auto* ثابت selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = true;
            collectArraySelInfo(selp->fromp(), info);
        } وإلا 𐤀𐤌 (auto* ثابت selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.push_back(selp);
        } وإلا 𐤀𐤌 (ثابت auto* ثابت memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    ساكن ArraySelInfo getArraySelInfo(AstNodeExpr* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        أرجع info;
    }

    ساكن std::vector<صحيح> arraySelDimSizes(ثابت ArraySelInfo& info) {
        std::vector<صحيح> dims;
        dims.reserve(info.m_sels.size());
        لكل (AstArraySel* ثابت selp : info.m_sels) {
            AstNodeDType* ثابت dtypep = selp->fromp()->dtypep()->skipRefp();
            ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.push_back(arrayp->declRange().elements());
        }
        أرجع dims;
    }

    ساكن صحيح flattenIndex(ثابت std::vector<صحيح>& indices, ثابت std::vector<صحيح>& dimSizes) {
        UASSERT(indices.size() == dimSizes.size(), "Array index and dimension size mismatch");
        صحيح index = 0;
        صحيح stride = 1;
        لكل (صحيح i = static_cast<صحيح>(indices.size()) - 1; i >= 0; --i) {
            index += indices[i] * stride;
            stride *= dimSizes[i];
        }
        أرجع index;
    }

    ساكن AstNodeExpr* buildFlattenIndexExpr(FileLine* flp, ثابت ArraySelInfo& info) {
        ثابت std::vector<صحيح> dimSizes = arraySelDimSizes(info);
        منطقي allConst = true;
        لكل (AstArraySel* ثابت selp : info.m_sels) {
            𐤀𐤌 (!VN_IS(selp->bitp(), Const)) {
                allConst = خطأ;
                اكسر;
            }
        }
        𐤀𐤌 (allConst) {
            std::vector<صحيح> constIndices;
            constIndices.reserve(info.m_sels.size());
            لكل (AstArraySel* ثابت selp : info.m_sels) {
                constIndices.push_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            أرجع makeConst32(flp, flattenIndex(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        AstNodeExpr* resultp = nullptr;
        صحيح stride = 1;
        لكل (صحيح i = static_cast<صحيح>(info.m_sels.size()) - 1; i >= 0; --i) {
            AstNodeExpr* termp = info.m_sels[i]->bitp()->cloneTreePure(خطأ);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            𐤀𐤌 (termp->width() < 32) termp = جديد AstExtend{flp, termp, 32};
            𐤀𐤌 (stride != 1) termp = جديد AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? جديد AstAdd{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        أرجع resultp;
    }

    ساكن AstNodeExpr* buildRhsDataExpr(FileLine* flp, ثابت ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        أرجع جديد AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    فراغ finalizeRhsVars() {
        لكل (VarForceInfo& info : m_varInfos) {
            AstVar* ثابت varp = info.m_varp;
            𐤀𐤌 (info.m_forces.empty()) استمر;

            AstScope* ثابت scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            FileLine* ثابت flp = varp->fileline();
            ثابت std::vector<ForceInfo*> forceps = forceInfosInIdOrder(info);

            لكل (ForceInfo* ثابت finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* ثابت rhsVarp = جديد AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doingAssign() ? "_VassignRHS" : "__VforceRHS")
                        + std::to_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(true);
                rhsVarp->sigPublic(true);
                rhsVarp->setForcedByCode();
                varp->addNextHere(rhsVarp);
                finfo.m_rhsVarVscp = جديد AstVarScope{flp, scopep, rhsVarp};
                scopep->addVarsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                // end
                AstAssign* ثابت rhsAssignp = جديد AstAssign{
                    flp, جديد AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                𐤀𐤌 (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* ثابت touchCallp = جديد AstCMethodHard{
                        flp, جديد AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtypeSetVoid();
                    AstNodeStmt* ثابت touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->addNextHere(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* ثابت alwaysp
                    = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, rhsAssignp};
                AstSenTree* ثابت senTreep
                    = جديد AstSenTree{flp, جديد AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ثابت activep = جديد AstActive{flp, "force-rhs-update", senTreep};
                activep->senTreeStorep(activep->sentreep());
                activep->addStmtsp(alwaysp);
                scopep->addBlocksp(activep);
            }

            𐤀𐤌 (info.m_forceRdVscp) {
                𐤀𐤌 (!m_forceRdUpdateBuilt.insert(info.m_varVscp).second) استمر;
                AstActive* ثابت activeInitp = جديد AstActive{
                    flp, "force-init",
                    جديد AstSenTree{flp, جديد AstSenItem{flp, AstSenItem::Static{}}}};
                activeInitp->senTreeStorep(activeInitp->sentreep());
                AstNodeStmt* initStmtp = nullptr;
                𐤀𐤌 (AstUnpackArrayDType* ثابت arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ثابت std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    ثابت صحيح innerWidth = dims.back()->subDTypep()->skipRefp()->width();
                    initStmtp = foreachUnpackedLeaf(
                        dims, [&](ثابت std::vector<صحيح>& idx, صحيح /*flat*/) -> AstNodeStmt* {
                            AstNodeExpr* ثابت lhsp = buildNestedArraySel(
                                flp, جديد AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            أرجع جديد AstAssign{flp, lhsp, makeZeroConst(varp, innerWidth)};
                        });
                } وإلا {
                    initStmtp = جديد AstAssign{
                        flp, جديد AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->width())};
                }
                initStmtp->addNext(createForceRdUpdateStmt(info));
                activeInitp->addStmtsp(جديد AstInitial{flp, initStmtp});
                scopep->addBlocksp(activeInitp);

                AstSenItem* itemsp = nullptr;
                auto addSenItem = [&](AstVarScope* vscp) {
                    𐤀𐤌 (!vscp) أرجع;
                    AstSenItem* ثابت nextp = جديد AstSenItem{
                        flp, VEdgeType::ET_CHANGED, جديد AstVarRef{flp, vscp, VAccess::READ}};
                    𐤀𐤌 (itemsp) {
                        itemsp->addNext(nextp);
                    } وإلا {
                        itemsp = nextp;
                    }
                };
                addSenItem(info.m_forceEnVscp);
                addSenItem(info.m_forceValVscp);
                AstVarRef* ثابت origSenRefp = جديد AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                AstSenItem* ثابت origItemp
                    = جديد AstSenItem{flp, VEdgeType::ET_CHANGED, origSenRefp};
                𐤀𐤌 (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->addNext(origItemp);
                لكل (ForceInfo* ثابت finfop : forceps) {
                    𐤀𐤌 (!finfop->m_isExternal) addSenItem(finfop->m_rhsVarVscp);
                }

                AstActive* ثابت activep
                    = جديد AstActive{flp, "force-rd-update", جديد AstSenTree{flp, itemsp}};
                activep->senTreeStorep(activep->sentreep());
                activep->addStmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr,
                                                 createForceRdUpdateStmt(info)});
                scopep->addBlocksp(activep);
            }
        }
    }

    AstNode* createRhsUpdatesForWrite(FileLine* flp, AstVar* writtenVarp) ثابت {
        𐤀𐤌 (!doingAssign()) أرجع nullptr;

        ثابت auto it = m_rhsDepToForces.find(writtenVarp);
        𐤀𐤌 (it == m_rhsDepToForces.end()) أرجع nullptr;

        AstNode* headp = nullptr;
        AstNode* tailp = nullptr;
        لكل (ثابت ForceInfo* ثابت finfop : it->second) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            AstAssign* ثابت updatep
                = جديد AstAssign{flp, جديد AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->cloneTreePure(خطأ)};
            𐤀𐤌 (tailp) {
                tailp->addNextHere(updatep);
            } وإلا {
                headp = updatep;
            }
            tailp = updatep;
        }
        أرجع headp;
    }

    ثابت ForceInfo& getForceInfo(AstAssignForce* forceStmtp) ثابت {
        AstVarScope* ثابت vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        ثابت VarForceInfo* ثابت varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        auto it2 = varInfo->m_forces.find(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.end(), "Force statement not found");
        أرجع it2->second;
    }

    ساكن منطقي selOverlapsAnyForce(ثابت VarForceInfo& varInfo, صحيح selLsb, صحيح selMsb) {
        لكل (ثابت auto& pair : varInfo.m_forces) {
            𐤀𐤌 (pair.second.m_rangeLsb <= selMsb && pair.second.m_rangeMsb >= selLsb) أرجع true;
        }
        أرجع خطأ;
    }

    AstNodeExpr* createForceReadExpression(ثابت VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) ثابت {
        FileLine* ثابت flp = originalRefp->fileline();
        AstVarRef* ثابت refp = originalRefp->cloneTreePure(خطأ);
        refp->access(VAccess::READ);
        أرجع createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   nullptr);
    }

    AstNodeExpr* createForceReadIndexExpression(ثابت VarForceInfo& varInfo,
                                                AstNodeExpr* originalExprp,
                                                AstNodeExpr* indexExprp) ثابت {
        FileLine* ثابت flp = originalExprp->fileline();
        AstNodeExpr* ثابت exprp = originalExprp->cloneTreePure(خطأ);
        // Must be an LValue to a static variable
        VN_AS(exprp->getVAccessTargetRecurse(), VarRef)->access(VAccess::READ);
        أرجع createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_INDEX, exprp, originalExprp,
                                   indexExprp);
    }

    ساكن AstNodeExpr* rebuildSelPath(AstNodeExpr* pathp, AstNodeExpr* baseExprp) {
        𐤀𐤌 (ثابت AstSel* ثابت selp = VN_CAST(pathp, Sel)) {
            AstNodeExpr* ثابت fromp = rebuildSelPath(selp->fromp(), baseExprp);
            AstSel* ثابت outp
                = جديد AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->widthConst()};
            outp->dtypeFrom(selp);
            أرجع outp;
        }
        أرجع baseExprp;
    }
};

// Split deassign concat LHS before converting to release internals.
ساكن فراغ splitDeassign(AstDeassign* nodep) {
    AstConcat* ثابت concatp = VN_CAST(nodep->lhsp(), Concat);
    𐤀𐤌 (!concatp) أرجع;

    FileLine* ثابت flp = nodep->fileline();
    AstDeassign* ثابت newLp = جديد AstDeassign{flp, concatp->lhsp()->unlinkFrBack()};
    AstDeassign* ثابت newRp = جديد AstDeassign{flp, concatp->rhsp()->unlinkFrBack()};
    AstNodeExpr* ثابت conp = concatp->unlinkFrBack();
    nodep->replaceWith(newLp);
    newLp->addNextHere(newRp);
    VL_DO_DANGLING(nodep->deleteTree(), nodep);
    VL_DO_DANGLING(conp->deleteTree(), conp);

    splitDeassign(newLp);
    splitDeassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

صنف ForceDiscoveryVisitor final : عام VNVisitorConst {
    ForceState& m_state;
    منطقي m_inClockedActive = خطأ;

    فراغ buildForceableUnpackedArray(AstVarScope* ثابت nodep,
                                     AstUnpackArrayDType* ثابت arrDtypep) {
        AstVar* ثابت varp = nodep->varp();
        ثابت std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.empty(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        ثابت AstNodeDType* ثابت leafDtypep = dims.back()->subDTypep()->skipRefp();
        ثابت AstBasicDType* ثابت innerBasicp = leafDtypep->basicp();
        𐤀𐤌 (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            أرجع;
        }

        FileLine* ثابت flp = varp->fileline();
        ثابت صحيح innerWidth = leafDtypep->width();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* ثابت enVscp = info.m_forceEnVscp;
        AstVarScope* ثابت valVscp = info.m_forceValVscp;

        AstSenItem* ثابت itemsp = جديد AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                  جديد AstVarRef{flp, enVscp, VAccess::READ}};
        AstActive* ثابت activep = جديد AstActive{flp, "force-update", جديد AstSenTree{flp, itemsp}};
        activep->senTreeStorep(activep->sentreep());

        AstNodeStmt* ثابت alwaysBodyHeadp = ForceState::foreachUnpackedLeaf(
            dims, [&](ثابت std::vector<صحيح>& idx, صحيح flat) -> AstNodeStmt* {
                AstVarRef* ثابت origRefp = جديد AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                AstNodeExpr* ثابت origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                AstNodeExpr* ثابت enSelp = ForceState::buildNestedArraySel(
                    flp, جديد AstVarRef{flp, enVscp, VAccess::READ}, idx);
                AstNodeExpr* ثابت valSelp = ForceState::buildNestedArraySel(
                    flp, جديد AstVarRef{flp, valVscp, VAccess::READ}, idx);
                AstNodeExpr* ثابت forceExprp = جديد AstOr{
                    flp, جديد AstAnd{flp, enSelp, valSelp},
                    جديد AstAnd{flp, جديد AstNot{flp, enSelp->cloneTreePure(خطأ)}, origSelp}};
                AstNodeExpr* ثابت lhsSelp = ForceState::buildNestedArraySel(
                    flp, جديد AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                AstAssignForce* ثابت forceAssignp = جديد AstAssignForce{flp, lhsSelp, forceExprp};
                forceAssignp->user2(true);

                AstNodeExpr* ثابت rhsClonep = forceExprp->cloneTreePure(خطأ);
                rhsClonep->foreach([varp](AstVarRef* ثابت r) {
                    𐤀𐤌 (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/innerWidth - 1,
                                           /*hasArraySel=*/true, /*skipRdRefresh=*/true);
                أرجع forceAssignp;
            });
        activep->addStmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, alwaysBodyHeadp});
        nodep->scopep()->addBlocksp(activep);
    }

    فراغ visit(AstAssignForce* nodep) override {
        𐤀𐤌 (nodep->user2()) أرجع;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* ثابت forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, true);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        AstNodeExpr* ثابت rhsExprp = nodep->rhsp()->cloneTreePure(خطأ);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    فراغ visit(AstAssign* nodep) override {
        𐤀𐤌 (m_state.doingAssign() && m_inClockedActive) {
            𐤀𐤌 (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstActive* nodep) override {
        VL_RESTORER(m_inClockedActive);
        m_inClockedActive = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstVarScope* nodep) override {
        𐤀𐤌 (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            𐤀𐤌 (m_state.doingAssign()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                أرجع;
            }

            𐤀𐤌 (AstUnpackArrayDType* ثابت arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                أرجع;
            }

            ثابت AstBasicDType* ثابت bdtypep = nodep->varp()->basicp();
            𐤀𐤌 (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* ثابت varp = nodep->varp();
            FileLine* ثابت flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* ثابت enVscp = info.m_forceEnVscp;
            AstVarScope* ثابت valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            AstSenItem* ثابت itemsp = جديد AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                      جديد AstVarRef{flp, enVscp, VAccess::READ}};
            AstActive* ثابت activep
                = جديد AstActive{flp, "force-update", جديد AstSenTree{flp, itemsp}};
            activep->senTreeStorep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* ثابت refp = جديد AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* ثابت enRefp = جديد AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* ثابت valRefp = جديد AstVarRef{flp, valVscp, VAccess::READ};
            ثابت AstBasicDType* ثابت basicp = varp->dtypep()->skipRefp()->basicp();
            AstNodeExpr* ثابت forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<AstNodeExpr*>(جديد AstOr{
                            flp, جديد AstAnd{flp, enRefp, valRefp},
                            جديد AstAnd{flp, جديد AstNot{flp, enRefp->cloneTreePure(خطأ)}, refp}})
                      : static_cast<AstNodeExpr*>(جديد AstCond{flp, enRefp, valRefp, refp});
            AstAssignForce* ثابت forceAssignp
                = جديد AstAssignForce{flp, جديد AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(true);
            activep->addStmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, forceAssignp});
            nodep->scopep()->addBlocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            AstNodeExpr* ثابت rhsClonep = forceExprp->cloneTreePure(خطأ);
            rhsClonep->foreach([varp](AstVarRef* ثابت refp) {
                𐤀𐤌 (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            ثابت منطقي bitwiseVar = ForceState::isBitwiseDType(varp);
            ثابت صحيح padMsb = bitwiseVar ? (varp->width() - 1) : 0;
            صحيح rangeLsb = 0;
            صحيح rangeMsb = padMsb;
            𐤀𐤌 (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, خطأ);
        }
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

عام:
    explicit ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextConstNull(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

صنف ForceConvertVisitor final : عام VNVisitor {
    ForceState& m_state;

    فراغ visit(AstAssignForce* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        AstNodeExpr* ثابت lhsp = nodep->lhsp();
        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ثابت forcedVarp = lhsVarRefp->varp();

        ثابت ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        ثابت ForceState::VarForceInfo* ثابت varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        FileLine* ثابت flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        AstAssign* ثابت rhsAssignp
            = جديد AstAssign{flp, جديد AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->cloneTreePure(خطأ)};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        ثابت منطقي bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        𐤀𐤌 (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            AstNodeExpr* baseExprp = nodep->rhsp()->cloneTreePure(خطأ);
            baseExprp->foreach(
                [](AstVarRef* ثابت refp) { ForceState::markNonReplaceable(refp); });
            𐤀𐤌 (bitwiseForcedVar) {
                baseExprp = ForceState::zeroPadToBaseWidth(baseExprp, forcedVarp->width(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            𐤀𐤌 (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* ثابت maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                AstNodeExpr* ثابت valReadp
                    = جديد AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                AstNodeExpr* ثابت valWritep
                    = جديد AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                AstNodeExpr* ثابت notMaskp = جديد AstNot{flp, maskConstp};
                AstNodeExpr* ثابت maskedOldp = جديد AstAnd{flp, valReadp, notMaskp};
                AstNodeExpr* ثابت newValp = جديد AstOr{flp, maskedOldp, baseExprp};
                valAssignp = جديد AstAssign{flp, valWritep, newValp};

                AstNodeExpr* ثابت enReadp
                    = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                AstNodeExpr* ثابت enWritep
                    = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                AstNodeExpr* ثابت newEnp
                    = جديد AstOr{flp, enReadp, maskConstp->cloneTreePure(خطأ)};
                enAssignp = جديد AstAssign{flp, enWritep, newEnp};
            } وإلا {
                AstConst* ثابت oneConstp = ForceState::makeRangeMaskConst(nodep, 1, 0, 0);
                AstNodeExpr* ثابت rhsValp = ForceState::castToNodeDType(baseExprp, forcedVarp);
                valAssignp = جديد AstAssign{
                    flp, جديد AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = جديد AstAssign{
                    flp, جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        ثابت AstSel* ثابت selLhsp = VN_CAST(lhsp, Sel);
        ثابت منطقي arrayBitSel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->width();
        AstNodeExpr* ثابت rhsDatap = ForceState::buildRhsDataExpr(flp, info);
        AstCExpr* ثابت rhsAddrp = جديد AstCExpr{flp};
        rhsAddrp->add("&(");
        rhsAddrp->add(rhsDatap);
        rhsAddrp->add(")");
        AstCMethodHard* ثابت addForceCallp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ADD,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        addForceCallp->addPinsp(rhsAddrp);
        addForceCallp->addPinsp(
            ForceState::makeConst32(flp, arrayBitSel ? info.m_padLsb : info.m_rangeLsb));
        𐤀𐤌 (arrayBitSel) {
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padLsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padMsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, selLhsp->fromp()->width()));
        }
        addForceCallp->dtypeSetVoid();
        AstNodeStmt* ثابت stmtp = addForceCallp->makeStmt();

        AstNode* tailp = rhsAssignp;
        𐤀𐤌 (valAssignp) {
            tailp->addNextHere(valAssignp);
            tailp = valAssignp;
        }
        𐤀𐤌 (enAssignp) {
            tailp->addNextHere(enAssignp);
            tailp = enAssignp;
        }
        tailp->addNextHere(stmtp);
        𐤀𐤌 (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    فراغ visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        AstNodeExpr* ثابت lhsp = nodep->lhsp();
        AstVarRef* ثابت lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ثابت releasedVarp = lhsVarRefp->varp();

        ثابت ForceState::VarForceInfo* ثابت varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        𐤀𐤌 (!varInfo) {
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }
        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        FileLine* ثابت flp = nodep->fileline();

        ثابت ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, خطأ);

        ثابت AstSel* ثابت selLhsp = VN_CAST(lhsp, Sel);
        ثابت منطقي arrayBitSel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->width();
        AstCMethodHard* ثابت releaseCallp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        𐤀𐤌 (arrayBitSel) {
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtypeSetVoid();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        AstNodeStmt* ثابت releasep = releaseCallp->makeStmt();

        AstAssign* clearEnp = nullptr;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        𐤀𐤌 (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            AstNodeExpr* ثابت enWritep
                = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            𐤀𐤌 (ForceState::isBitwiseDType(releasedVarp)) {
                ثابت صحيح varWidth = releasedVarp->width();
                𐤀𐤌 (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == varWidth - 1) {
                    clearEnp
                        = جديد AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, varWidth)};
                } وإلا {
                    // forceEn = forceEn & ~mask(range);
                    AstNodeExpr* ثابت enReadp
                        = جديد AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* ثابت maskConst = ForceState::makeRangeMaskConst(
                        nodep, varWidth, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    AstNodeExpr* ثابت newEnp
                        = جديد AstAnd{flp, enReadp, جديد AstNot{flp, maskConst}};
                    clearEnp = جديد AstAssign{flp, enWritep, newEnp};
                }
            } وإلا {
                clearEnp = جديد AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        ثابت AstSel* ثابت selp = VN_CAST(lhsp, Sel);
        AstNodeExpr* ثابت basep = selp ? selp->fromp() : lhsp;

        AstNode* stmtListp = releasep;
        𐤀𐤌 (clearEnp) {
            clearEnp->addNextHere(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        ثابت منطقي fullBitwiseRelease
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->width() - 1;
        𐤀𐤌 (!releasedVarp->isContinuously()
            && !(m_state.doingAssign() && m_state.hasClockedWrite(releasedVarp)
                 && fullBitwiseRelease)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            ثابت منطقي hasOpaquePath = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            AstNodeExpr* forceReadp = nullptr;
            𐤀𐤌 (hasOpaquePath) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } وإلا 𐤀𐤌 (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, basep,
                    ForceState::buildFlattenIndexExpr(flp, rangeInfo.m_arrayInfo));
                𐤀𐤌 (selp) { forceReadp = ForceState::rebuildSelPath(lhsp, forceReadp); }
            } وإلا {
                forceReadp
                    = selp ? ForceState::rebuildSelPath(
                                 lhsp, m_state.createForceReadExpression(*varInfo, lhsVarRefp))
                           : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            }
            AstAssign* ثابت assignp = جديد AstAssign{flp, lhsp->cloneTreePure(خطأ), forceReadp};
            assignp->addNextHere(stmtListp);
            stmtListp = assignp;
        }

        𐤀𐤌 (varInfo->m_forceRdVscp) stmtListp->addNext(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

صنف ForceReplaceVisitor final : عام VNVisitor {
    ثابت ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    AstNodeStmt* m_stmtp = nullptr;
    منطقي m_inLogic = خطأ;

    فراغ iterateLogic(AstNode* nodep) {
        VL_RESTORER(m_inLogic);
        m_inLogic = true;
        iterateChildren(nodep);
    }

    فراغ visit(AstNodeStmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstAssign* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        𐤀𐤌 (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp()->baseFromp(true), VarRef)) {
            𐤀𐤌 (AstNode* ثابت updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->addNextHere(updatep);
            }
        }
    }
    فراغ visit(AstAssignCont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateAndNextNull(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    فراغ visit(AstDeassign*) override {}
    فراغ visit(AstCFunc* nodep) override { iterateLogic(nodep); }
    فراغ visit(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    فراغ visit(AstNodeProcedure* nodep) override { iterateLogic(nodep); }
    فراغ visit(AstAlways* nodep) override {
        𐤀𐤌 (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            أرجع;
        }
        iterateLogic(nodep);
    }
    فراغ visit(AstSenItem* nodep) override { iterateLogic(nodep); }
    فراغ visit(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef);
        𐤀𐤌 (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            visit(static_cast<AstNode*>(nodep));
            أرجع;
        }

        AstVar* ثابت varp = refp->varp();
        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(refp->varScopep());
        𐤀𐤌 (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.empty()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            visit(static_cast<AstNode*>(nodep));
            أرجع;
        }

        𐤀𐤌 (ثابت AstConst* ثابت lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            ثابت صحيح selLsb = lsbConstp->toSInt();
            ثابت صحيح selMsb = selLsb + nodep->width() - 1;
            𐤀𐤌 (!varp->isSigPublic()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                visit(static_cast<AstNode*>(nodep));
                أرجع;
            }
        }

        FileLine* ثابت flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* ثابت refClonep = refp->cloneTreePure(خطأ);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->width())};
        callp->addPinsp(refClonep);
        callp->addPinsp(nodep->lsbp()->cloneTreePure(خطأ));
        callp->addPinsp(ForceState::makeConst32(flp, nodep->width()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstArraySel* nodep) override {
        𐤀𐤌 (ثابت AstArraySel* ثابت backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            𐤀𐤌 (backSelp->fromp() == nodep) {
                iterateChildren(nodep);
                أرجع;
            }
        }

        AstNode* ثابت basep = nodep->baseFromp(true);
        AstVarRef* ثابت baseRefp = VN_CAST(basep, VarRef);
        𐤀𐤌 (!baseRefp) {
            iterateChildren(nodep);
            أرجع;
        }
        AstVar* ثابت varp = baseRefp->varp();
        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(baseRefp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        𐤀𐤌 (m_state.isNotReplaceable(baseRefp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            أرجع;
        }

        𐤀𐤌 (!baseRefp->access().isReadOnly()) {
            iterateChildren(nodep);
            أرجع;
        }
        𐤀𐤌 (m_state.doingAssign() && varInfo->m_forceRdVscp) {
            baseRefp->varp(varInfo->m_forceRdVscp->varp());
            baseRefp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            أرجع;
        }
        ثابت ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        لكل (AstArraySel* ثابت selp : arrayInfo.m_sels) iterateAndNextNull(selp->bitp());
        AstNodeExpr* ثابت indexExprp
            = ForceState::buildFlattenIndexExpr(nodep->fileline(), arrayInfo);
        AstNodeExpr* ثابت readExprp
            = m_state.createForceReadIndexExpression(*varInfo, nodep, indexExprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    فراغ visit(AstVarRef* nodep) override {
        𐤀𐤌 (m_state.isNotReplaceable(nodep)) أرجع;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        𐤀𐤌 (ثابت AstArraySel* ثابت backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            𐤀𐤌 (backSelp->fromp() == nodep) أرجع;
        }

        ثابت ForceState::VarForceInfo* ثابت varInfo = m_state.getVarInfo(nodep->varScopep());
        𐤀𐤌 (!varInfo) أرجع;

        𐤀𐤌 (varInfo->m_forceRdVscp) {
            𐤀𐤌 (nodep->access().isRW()) {
                𐤀𐤌 (m_inLogic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                أرجع;
            }
            𐤀𐤌 (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                AstNodeExpr* ثابت readp
                    = m_state.createForceRdExpression(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                أرجع;
            }
            𐤀𐤌 (m_inLogic && m_stmtp) {
                m_stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
            }
            أرجع;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        𐤀𐤌 (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            أرجع;
        }

        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        𐤀𐤌 (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } وإلا 𐤀𐤌 (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            AstNodeExpr* ثابت readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }
    فراغ visit(AstNode* nodep) override {
        𐤀𐤌 (ForceState::isOutermostOpaquePathSelector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            AstNodeExpr* ثابت exprp = VN_AS(nodep, NodeExpr);
            AstNode* ثابت basep = exprp->baseFromp(true);
            AstVarRef* ثابت baseRefp = VN_CAST(basep, VarRef);
            𐤀𐤌 (baseRefp) {
                AstVar* ثابت varp = baseRefp->varp();
                𐤀𐤌 (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    ثابت ForceState::VarForceInfo* ثابت varInfo
                        = m_state.getVarInfo(baseRefp->varScopep());
                    𐤀𐤌 (!m_state.isNotReplaceable(baseRefp) && varInfo) {
                        ثابت صحيح forcePathIndex = varInfo->findForcePathIndex(exprp);
                        𐤀𐤌 (forcePathIndex >= 0) {
                            𐤀𐤌 (!baseRefp->access().isReadOnly()) أرجع;
                            AstNodeExpr* ثابت readExprp = m_state.createForceReadIndexExpression(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), forcePathIndex));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(pushDeletep(nodep), nodep);
                            أرجع;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

عام:
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

ساكن فراغ forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::ProtectedVarRefSet& permanentlyProtected,
                         ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    𐤀𐤌 (!v3Global.hasForceableSignals()) أرجع;
    ForceState state{خطأ, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dumpCheckGlobalTree("force", 0, dumpTreeEitherLevel() >= 3);
}

ساكن فراغ assignAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::ProtectedVarRefSet& permanentlyProtected,
                          ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    𐤀𐤌 (!v3Global.hasAssignDeassign()) أرجع;

    std::vector<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.push_back(deassignp); });
    لكل (AstDeassign* ثابت deassignp : deassignps) splitDeassign(deassignp);

    std::vector<AstAssignCont*> assignContps;
    deassignps.clear();
    nodep->foreach([&](AstNodeStmt* nodep) {
        𐤀𐤌 (AstAssignCont* ثابت assignContp = VN_CAST(nodep, AssignCont)) {
            assignContps.push_back(assignContp);
        } وإلا 𐤀𐤌 (AstDeassign* ثابت deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.push_back(deassignp);
        }
    });

    لكل (AstAssignCont* ثابت assignp : assignContps) {
        assignp->replaceWith(جديد AstAssignForce{assignp->fileline(),
                                                assignp->lhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
        VL_DO_DANGLING(assignp->deleteTree(), assignp);
    }
    لكل (AstDeassign* ثابت deassignp : deassignps) {
        deassignp->replaceWith(
            جديد AstRelease{deassignp->fileline(), deassignp->lhsp()->cloneTreePure(true)});
        VL_DO_DANGLING(deassignp->deleteTree(), deassignp);
    }
    ForceState state{true, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dumpCheckGlobalTree("assign-deassign", 0, dumpTreeEitherLevel() >= 3);
}

فراغ V3Force::forceAndAssignAll(AstNetlist* nodep) {
    ثابت VNUser3InUse user3InUse;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::ProtectedVarRefSet permanentlyProtected;
    ForceState::BuiltVarScopeSet forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    assignAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
