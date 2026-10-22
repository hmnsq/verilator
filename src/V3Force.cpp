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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Force.h"

#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு ForceState final {
பொது:
    கட்டமைப்பு ForceRange VL_NOT_FINAL {
        முழு m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        முழு m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        முழு m_padLsb = 0;  // Bit positions for RHS padding
        முழு m_padMsb = 0;  // Bit positions for RHS padding
    };

    கட்டமைப்பு ForceInfo final : ForceRange {
        // MEMBERS
        முழு m_forceId = 0;  // Unique (per signal) variable of this force assignment
        தருக்க m_hasArraySel = பொய்;  // If this has an array select on LHS
        தருக்க m_isExternal = பொய்;  // Synthetic force for the public force controls
        தருக்க m_skipRdRefresh = பொய்;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = nullptr;  // Scope of the var containing RHSID
        Ast_முனை_கோவை* m_rhsExprp = nullptr;  // Expression on RHS of this force assignment

        ForceInfo() = இயல்பு;
        ForceInfo(முழு rangeLsb, முழு rangeMsb, முழு padLsb, முழு padMsb, முழு forceId,
                  தருக்க hasArraySel, AstVarScope* rhsVarVscp, Ast_முனை_கோவை* rhsExprp)
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

    கட்டமைப்பு VarForceInfo final {
        AstVarScope* m_forceVecVscp = nullptr;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = nullptr;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = nullptr;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = nullptr;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = nullptr;  // Scope of the forced (original) variable
        AstVar* m_varp = nullptr;  // The forced (original) variable
        AstScope* m_scopep = nullptr;  // Scope containing the forced variable
        std::unordered_map<AstAssignForce*, ForceInfo> m_forces;
        std::unordered_map<string, முழு> m_forcePathToIndex;
        முழு m_nextForcePathIndex = 1;  // Start at 1 so 0 can be the base path (whole signal)

    தனிப்பட்ட:
        நிலையான வெறுமை build_Force_Path_சாவி_Recurse(Ast_முனை_கோவை* nodep, string& out) {
            எனில் (VN_IS(nodep, VarRef)) திரும்பு;
            எனில் (மாறிலி AstSel* மாறிலி selp = VN_CAST(nodep, Sel)) {
                build_Force_Path_சாவி_Recurse(selp->fromp(), out);
                out += "|S" + cvtToStr(selp->lsbConst()) + ":" + cvtToStr(selp->widthConst());
                திரும்பு;
            }
            எனில் (AstStructSel* மாறிலி selp = VN_CAST(nodep, StructSel)) {
                Ast_முனை_கோவை* மாறிலி fromp = selp->fromp();
                build_Force_Path_சாவி_Recurse(fromp, out);
                out += "|T" + selp->name();
                திரும்பு;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        நிலையான string force_Path_சாவி(Ast_முனை_கோவை* nodep) {
            string out;
            build_Force_Path_சாவி_Recurse(nodep, out);
            திரும்பு out;
        }

    பொது:
        முழு getOrCreateForcePathIndex(Ast_முனை_கோவை* nodep) {
            மாறிலி auto pair
                = m_forcePathToIndex.emplace(force_Path_சாவி(nodep), m_nextForcePathIndex);
            எனில் (pair.second) ++m_nextForcePathIndex;
            திரும்பு pair.first->second;
        }

        முழு findForcePathIndex(Ast_முனை_கோவை* nodep) மாறிலி {
            மாறிலி auto it = m_forcePathToIndex.find(force_Path_சாவி(nodep));
            திரும்பு it != m_forcePathToIndex.end() ? it->second : -1;
        }
    };

    கட்டமைப்பு ForceHelperVars final {
        AstVar* m_rdVarp = nullptr;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = nullptr;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = nullptr;  // The __VforceVal (force value) variable
    };

    கட்டமைப்பு ArraySelInfo final {
        std::vector<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        தருக்க m_hasBitSel = பொய்;  // Bit-select was found along the path too
    };

    கட்டமைப்பு ForceRangeInfo final : ForceRange {
        தருக்க m_hasArraySel = பொய்;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

தனிப்பட்ட:
    using ScopeVarCache = std::unordered_map<மாறிலி AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    மாறிலி VNUser1InUse m_user1InUse;
    மாறிலி VNUser2InUse m_user2InUse;

பொது:
    using ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    using ProtectedVarRefSet = std::unordered_set<மாறிலி AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    using BuiltVarScopeSet = std::unordered_set<மாறிலி AstVarScope*>;

தனிப்பட்ட:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::vector<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, முழு> m_varToId;
    std::unordered_set<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::vector<ForceInfo*>> m_rhsDepToForces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    தருக்க m_doingAssign = பொய்;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    ProtectedVarRefSet& m_permanentlyProtected;
    BuiltVarScopeSet& m_forceRdUpdateBuilt;

பொது:
    ForceState(தருக்க doingAssign, ForceHelperVarsByVar& forceHelperVarsByVar,
               ProtectedVarRefSet& permanentlyProtected, BuiltVarScopeSet& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doingAssign{doingAssign}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    நிலையான AstConst* makeZeroConst(Ast_முனை* nodep, முழு width) {
        V3_எண் zero{nodep, width};
        zero.setAllBits0();
        திரும்பு புதிய AstConst{nodep->fileline(), zero};
    }

    நிலையான AstConst* makeConst32(File_வரி* flp, முழு மதிப்பு) {
        திரும்பு புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{}, 32, static_cast<uint32_t>(மதிப்பு)};
    }

    நிலையான AstConst* makeRangeMaskConst(Ast_முனை* nodep, முழு width, முழு lsb, முழு msb) {
        V3_எண் mask{nodep, width};
        mask.setAllBits0();
        க்கு (முழு bit = lsb; bit <= msb; ++bit) mask.setBit(bit, 1);
        திரும்பு புதிய AstConst{nodep->fileline(), mask};
    }

    நிலையான Ast_முனை_கோவை* zeroPadToBaseWidth(Ast_முனை_கோவை* exprp, முழு baseWidth, முழு padLsb,
                                           முழு padMsb) {
        எனில் (baseWidth <= 0) திரும்பு exprp;
        மாறிலி முழு கீழ்_Pad = padLsb;
        மாறிலி முழு உயர்வு_Pad = baseWidth - (padMsb + 1);
        எனில் (கீழ்_Pad > 0) {
            exprp = புதிய AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, கீழ்_Pad)};
        }
        எனில் (உயர்வு_Pad > 0) {
            exprp = புதிய AstConcat{exprp->fileline(), makeZeroConst(exprp, உயர்வு_Pad), exprp};
        }
        திரும்பு exprp;
    }

    நிலையான தருக்க isUnpackedArrayDType(மாறிலி Ast_முனை_DType* dtypep) {
        திரும்பு VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    நிலையான தருக்க isBitwiseDType(மாறிலி Ast_முனை* nodep) {
        மாறிலி AstBasicDType* மாறிலி basicp = nodep->dtypep()->skipRefp()->basicp();
        திரும்பு basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    நிலையான Ast_முனை_கோவை* cast_To_முனை_DType(Ast_முனை_கோவை* exprp, Ast_முனை* dtypeFromp) {
        மாறிலி Ast_முனை_DType* மாறிலி dtypep = dtypeFromp->dtypep()->skipRefp();
        மாறிலி AstBasicDType* மாறிலி basicp = dtypep->basicp();
        எனில் (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            திரும்பு exprp;
        }
        திரும்பு புதிய AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    நிலையான வெறுமை markNonReplaceable(AstVarRef* மாறிலி nodep) { nodep->user1SetOnce(); }
    தருக்க isNotReplaceable(மாறிலி AstVarRef* மாறிலி nodep) மாறிலி {
        திரும்பு nodep->user1() || m_permanentlyProtected.மொத்தம்(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    வெறுமை markPermanentlyProtected(AstVarRef* மாறிலி nodep) மாறிலி {
        markNonReplaceable(nodep);
        m_permanentlyProtected.செருகு(nodep);
    }

    நிலையான std::vector<ForceInfo*> forceInfosInIdOrder(VarForceInfo& info) {
        std::vector<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        க்கு (auto& it : info.m_forces) forceps.தள்ளு_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](மாறிலி ForceInfo* ap, மாறிலி ForceInfo* bp) {
            திரும்பு ap->m_forceId < bp->m_forceId;
        });
        திரும்பு forceps;
    }

    நிலையான std::vector<மாறிலி ForceInfo*> forceInfosInIdOrder(மாறிலி VarForceInfo& info) {
        std::vector<மாறிலி ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        க்கு (மாறிலி auto& it : info.m_forces) forceps.தள்ளு_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](மாறிலி ForceInfo* ap, மாறிலி ForceInfo* bp) {
            திரும்பு ap->m_forceId < bp->m_forceId;
        });
        திரும்பு forceps;
    }

    நிலையான தருக்க isOpaquePathSelector(மாறிலி Ast_முனை* nodep) {
        திரும்பு VN_IS(nodep, Sel) || VN_IS(nodep, முனை_Sel) || VN_IS(nodep, StructSel);
    }

    நிலையான தருக்க isOutermostOpaquePathSelector(மாறிலி Ast_முனை* nodep) {
        எனில் (!isOpaquePathSelector(nodep)) திரும்பு பொய்;
        மாறிலி Ast_முனை* மாறிலி backp = nodep->backp();
        திரும்பு !backp || !isOpaquePathSelector(backp);
    }

    நிலையான AstVarRef* getOneVarRef(Ast_முனை_கோவை* forceStmtp) {
        Ast_முனை* மாறிலி basep = forceStmtp->baseFromp(உண்மை);
        எனில் (AstSampled* sampledp = VN_CAST(basep, Sampled))
            எனில் (Ast_முனை_கோவை* exprp = VN_CAST(sampledp->exprp(), முனை_கோவை))
                திரும்பு getOneVarRef(exprp);
        AstVarRef* மாறிலி varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        திரும்பு varRefp;
    }

    நிலையான Ast_முனை_கோவை* buildNestedArraySel(File_வரி* flp, Ast_முனை_கோவை* fromp,
                                            மாறிலி std::vector<முழு>& indicies) {
        Ast_முனை_கோவை* curp = fromp;
        க்கு (மாறிலி முழு idx : indicies) curp = புதிய AstArraySel{flp, curp, idx};
        திரும்பு curp;
    }

    template <typename Fn>
    நிலையான Ast_முனை_Stmt* foreachUnpackedLeaf(மாறிலி std::vector<AstUnpackArrayDType*>& dims,
                                            Fn buildLeaf) {
        Ast_முனை_Stmt* headp = nullptr;
        Ast_முனை_Stmt* tailp = nullptr;
        எனில் (dims.empty()) திரும்பு nullptr;
        முழு total = 1;
        க்கு (மாறிலி AstUnpackArrayDType* மாறிலி d : dims) total *= d->elementsConst();
        எனில் (total <= 0) திரும்பு nullptr;
        std::vector<முழு> idx(dims.size(), 0);
        க்கு (முழு flat = 0; flat < total; ++flat) {
            Ast_முனை_Stmt* மாறிலி stmtp = buildLeaf(idx, flat);
            எனில் (!headp) {
                headp = stmtp;
            } இல்லையெனில் {
                tailp->addNext(stmtp);
            }
            tailp = stmtp;
            க்கு (முழு d = static_cast<முழு>(dims.size()) - 1; d >= 0; --d) {
                எனில் (++idx[d] < dims[d]->elementsConst()) நிறுத்து;
                idx[d] = 0;
            }
        }
        திரும்பு headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_முனை_கோவை* lhsp, AstVar* varp,
                                     தருக்க requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.empty();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->width() - 1) : 0;

        எனில் (மாறிலி AstSel* மாறிலி outerSelp = VN_CAST(lhsp, Sel)) {
            முழு totalLsb = 0;
            க்கு (Ast_முனை_கோவை* curp = lhsp; மாறிலி AstSel* மாறிலி selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                எனில் (requireConstRangeSelect) {
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
        எனில் (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::vector<முழு> indices;
            indices.reserve(info.m_arrayInfo.m_sels.size());
            க்கு (AstArraySel* மாறிலி selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.தள்ளு_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            info.m_rangeLsb = flattenIndex(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } இல்லையெனில் எனில் (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        எனில் (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            மாறிலி முழு index = varInfo.getOrCreateForcePathIndex(lhsp);
            info.m_rangeLsb = index;
            info.m_rangeMsb = index;
        }
        திரும்பு info;
    }

    Ast_முனை_கோவை* add_Rhs_மதிப்பு_Reads(மாறிலி VarForceInfo& varInfo, Ast_முனை_கோவை* exprp) மாறிலி {
        எனில் (!doingAssign()) திரும்பு exprp;

        மாறிலி std::vector<மாறிலி ForceInfo*> forceps = forceInfosInIdOrder(varInfo);
        எனில் (forceps.empty()) திரும்பு exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* மாறிலி cexprp = புதிய AstCExpr{exprp->fileline(), AstCExpr::Pure{}};
        cexprp->dtypeFrom(exprp);
        cexprp->add("(");
        க்கு (மாறிலி ForceInfo* மாறிலி finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* மாறிலி refp
                = புதிய AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->add("(void)(");
            cexprp->add(refp);
            cexprp->add("), ");
        }
        cexprp->add(exprp);
        cexprp->add(")");
        திரும்பு cexprp;
    }

    Ast_முனை_கோவை* createForceReadCall(மாறிலி VarForceInfo& varInfo, File_வரி* flp, VCMethod method,
                                     Ast_முனை_கோவை* originalExprp, Ast_முனை* dtypeFromp,
                                     Ast_முனை_கோவை* indexExprp) மாறிலி {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_முனை_கோவை* மாறிலி origValp
            = add_Rhs_மதிப்பு_Reads(varInfo, cast_To_முனை_DType(originalExprp, dtypeFromp));

        AstCMethodHard* மாறிலி callp = புதிய AstCMethodHard{
            flp, புதிய AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, method, origValp};
        எனில் (indexExprp) callp->addPinsp(indexExprp);
        callp->dtypeFrom(dtypeFromp);
        திரும்பு callp;
    }

    Ast_முனை_கோவை* createForceRdExpression(மாறிலி VarForceInfo& varInfo, File_வரி* flp) மாறிலி {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* மாறிலி varp = varInfo.m_varVscp->varp();
        எனில் (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            எனில் (doingAssign()) { திரும்பு புதிய AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, உண்மை);
            AstVarRef* மாறிலி baseRefp = புதிய AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(baseRefp);
            திரும்பு createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, baseRefp, varp,
                                       nullptr);
        }
        AstVarRef* மாறிலி baseRefp = புதிய AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(baseRefp);
        Ast_முனை_கோவை* மாறிலி enRefp = புதிய AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_முனை_கோவை* மாறிலி valRefp = புதிய AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        எனில் (isBitwiseDType(varp)) {
            திரும்பு புதிய AstOr{
                flp, புதிய AstAnd{flp, enRefp, valRefp},
                புதிய AstAnd{flp, புதிய AstNot{flp, enRefp->clone_மரம்_Pure(பொய்)}, baseRefp}};
        }
        திரும்பு புதிய AstCond{flp, enRefp, valRefp, baseRefp};
    }

    Ast_முனை_Stmt* createForceRdUpdateStmt(மாறிலி VarForceInfo& varInfo) மாறிலி {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* மாறிலி varp = varInfo.m_varVscp->varp();
        எனில் (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            திரும்பு createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_வரி* மாறிலி flp = varInfo.m_varVscp->fileline();
        திரும்பு புதிய AstAssign{flp, புதிய AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             createForceRdExpression(varInfo, flp)};
    }

    Ast_முனை_Stmt* createForceRdUpdateStmtUnpacked(மாறிலி VarForceInfo& varInfo) மாறிலி {
        File_வரி* மாறிலி flp = varInfo.m_varVscp->fileline();
        AstVar* மாறிலி varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* மாறிலி arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        மாறிலி std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_முனை_Stmt* மாறிலி stmtsp = foreachUnpackedLeaf(
            dims, [&](மாறிலி std::vector<முழு>& idx, முழு /*flat*/) -> Ast_முனை_Stmt* {
                AstVarRef* மாறிலி baseRefp = புதிய AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(baseRefp);
                Ast_முனை_கோவை* மாறிலி baseSelp = buildNestedArraySel(flp, baseRefp, idx);
                Ast_முனை_கோவை* மாறிலி enSelp = buildNestedArraySel(
                    flp, புதிய AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_முனை_கோவை* மாறிலி valSelp = buildNestedArraySel(
                    flp, புதிய AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_முனை_கோவை* மாறிலி readExprp = புதிய AstOr{
                    flp, புதிய AstAnd{flp, enSelp, valSelp},
                    புதிய AstAnd{flp, புதிய AstNot{flp, enSelp->clone_மரம்_Pure(பொய்)}, baseSelp}};
                Ast_முனை_கோவை* மாறிலி rdLhsSelp = buildNestedArraySel(
                    flp, புதிய AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                திரும்பு புதிய AstAssign{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        க்கு (மாறிலி ForceInfo* மாறிலி finfop : forceInfosInIdOrder(varInfo)) {
            எனில் (!finfop->m_isExternal) தொடர்;
            stmtsp->addNext(புதிய AstAssign{flp,
                                          புதிய AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_மரம்_Pure(பொய்)});
        }
        திரும்பு stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        மாறிலி auto it = m_varToId.find(vscp);
        எனில் (it != m_varToId.end()) திரும்பு m_varInfos[it->second];

        m_varToId.emplace(vscp, m_varInfos.size());
        m_varInfos.emplace_back();
        VarForceInfo& info = m_varInfos.back();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* மாறிலி varp = info.m_varp;
        எனில் (!varp->isForceable()) திரும்பு info;

        File_வரி* மாறிலி flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        மாறிலி தருக்க helperVarsBuilt = helperVars.m_rdVarp != nullptr;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != nullptr)
                        && helperVarsBuilt == (helperVars.m_valVarp != nullptr),
                    varp, "Incomplete force helper set");
        எனில் (!helperVarsBuilt) {
            மாறிலி தருக்க unpacked = isUnpackedArrayDType(varp->dtypep());
            மாறிலி VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_முனை_DType* மாறிலி enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
            helperVars.m_rdVarp
                = புதிய AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sigPublic(உண்மை);
            helperVars.m_enVarp
                = புதிய AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(உண்மை);
            helperVars.m_valVarp
                = புதிய AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(உண்மை);
            varp->addNextHere(helperVars.m_rdVarp);
            varp->addNextHere(helperVars.m_enVarp);
            varp->addNextHere(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = findScopeVar(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = findScopeVar(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = findScopeVar(info.m_scopep, helperVars.m_valVarp);
        எனில் (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } இல்லையெனில் {
            info.m_forceRdVscp = புதிய AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = புதிய AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = புதிய AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->addVarsp(info.m_forceRdVscp);
            info.m_scopep->addVarsp(info.m_forceEnVscp);
            info.m_scopep->addVarsp(info.m_forceValVscp);
        }
        திரும்பு info;
    }

    வெறுமை markClockedWrite(AstVar* varp) { m_clockedWrites.செருகு(varp); }
    தருக்க hasClockedWrite(AstVar* varp) மாறிலி { திரும்பு m_clockedWrites.மொத்தம்(varp); }

    தருக்க doingAssign() மாறிலி { திரும்பு m_doingAssign; }

    மாறிலி VarForceInfo* getVarInfo(AstVarScope* vscp) மாறிலி {
        மாறிலி auto it = m_varToId.find(vscp);
        திரும்பு it != m_varToId.end() ? &m_varInfos[it->second] : nullptr;
    }

    AstVarScope* findScopeVar(AstScope* scopep, மாறிலி AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        எனில் (cache.empty()) {
            க்கு (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        மாறிலி auto it = cache.find(varp);
        திரும்பு it != cache.end() ? it->second : nullptr;
    }
    வெறுமை addForceAssignment(AstVar* varp, AstVarScope* vscp, Ast_முனை_கோவை* rhsExprp,
                            AstAssignForce* forceStmtp, முழு rangeLsb, முழு rangeMsb, முழு padLsb,
                            முழு padMsb, தருக்க hasArraySel, தருக்க skipRdRefresh = பொய்) {
        v3Global.setUsesForce();
        varp->setForcedByCode();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        மாறிலி முழு forceId = info.m_forces.size();
        File_வரி* மாறிலி flp = varp->fileline();
        AstScope* மாறிலி scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        எனில் (!info.m_forceVecVscp) {
            AstCDType* மாறிலி forceVecDtypep = புதிய AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->addTypesp(forceVecDtypep);

            AstVar* மாறிலி forceVecVarp
                = புதிய AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doingAssign ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(பொய்);
            forceVecVarp->isInternal(உண்மை);
            varp->addNextHere(forceVecVarp);
            info.m_forceVecVscp = புதிய AstVarScope{flp, scopep, forceVecVarp};
            scopep->addVarsp(info.m_forceVecVscp);
        }

        auto pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, nullptr, rhsExprp});
        ForceInfo& finfo = pair.first->second;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        எனில் (doingAssign()) {
            std::vector<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* மாறிலி refp) {
                எனில் (!refp->access().isReadOnly()) திரும்பு;
                AstVar* மாறிலி depVarp = refp->varp();
                எனில் (depVarp
                    && std::find(depVarps.begin(), depVarps.end(), depVarp) == depVarps.end()) {
                    depVarps.தள்ளு_back(depVarp);
                }
            });
            க்கு (AstVar* மாறிலி depVarp : depVarps) m_rhsDepToForces[depVarp].தள்ளு_back(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    நிலையான வெறுமை collectArraySelInfo(Ast_முனை_கோவை* exprp, ArraySelInfo& info) {
        எனில் (மாறிலி auto* மாறிலி selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = உண்மை;
            collectArraySelInfo(selp->fromp(), info);
        } இல்லையெனில் எனில் (auto* மாறிலி selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.தள்ளு_back(selp);
        } இல்லையெனில் எனில் (மாறிலி auto* மாறிலி memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    நிலையான ArraySelInfo getArraySelInfo(Ast_முனை_கோவை* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        திரும்பு info;
    }

    நிலையான std::vector<முழு> arraySelDimSizes(மாறிலி ArraySelInfo& info) {
        std::vector<முழு> dims;
        dims.reserve(info.m_sels.size());
        க்கு (AstArraySel* மாறிலி selp : info.m_sels) {
            Ast_முனை_DType* மாறிலி dtypep = selp->fromp()->dtypep()->skipRefp();
            மாறிலி AstUnpackArrayDType* மாறிலி arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.தள்ளு_back(arrayp->declRange().elements());
        }
        திரும்பு dims;
    }

    நிலையான முழு flattenIndex(மாறிலி std::vector<முழு>& indices, மாறிலி std::vector<முழு>& dimSizes) {
        UASSERT(indices.size() == dimSizes.size(), "Array index and dimension size mismatch");
        முழு index = 0;
        முழு stride = 1;
        க்கு (முழு i = static_cast<முழு>(indices.size()) - 1; i >= 0; --i) {
            index += indices[i] * stride;
            stride *= dimSizes[i];
        }
        திரும்பு index;
    }

    நிலையான Ast_முனை_கோவை* build_Flatten_Index_கோவை(File_வரி* flp, மாறிலி ArraySelInfo& info) {
        மாறிலி std::vector<முழு> dimSizes = arraySelDimSizes(info);
        தருக்க allConst = உண்மை;
        க்கு (AstArraySel* மாறிலி selp : info.m_sels) {
            எனில் (!VN_IS(selp->bitp(), Const)) {
                allConst = பொய்;
                நிறுத்து;
            }
        }
        எனில் (allConst) {
            std::vector<முழு> constIndices;
            constIndices.reserve(info.m_sels.size());
            க்கு (AstArraySel* மாறிலி selp : info.m_sels) {
                constIndices.தள்ளு_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            திரும்பு makeConst32(flp, flattenIndex(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_முனை_கோவை* resultp = nullptr;
        முழு stride = 1;
        க்கு (முழு i = static_cast<முழு>(info.m_sels.size()) - 1; i >= 0; --i) {
            Ast_முனை_கோவை* termp = info.m_sels[i]->bitp()->clone_மரம்_Pure(பொய்);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            எனில் (termp->width() < 32) termp = புதிய AstExtend{flp, termp, 32};
            எனில் (stride != 1) termp = புதிய AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? புதிய AstAdd{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        திரும்பு resultp;
    }

    நிலையான Ast_முனை_கோவை* build_Rhs_Data_கோவை(File_வரி* flp, மாறிலி ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        திரும்பு புதிய AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    வெறுமை finalizeRhsVars() {
        க்கு (VarForceInfo& info : m_varInfos) {
            AstVar* மாறிலி varp = info.m_varp;
            எனில் (info.m_forces.empty()) தொடர்;

            AstScope* மாறிலி scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_வரி* மாறிலி flp = varp->fileline();
            மாறிலி std::vector<ForceInfo*> forceps = forceInfosInIdOrder(info);

            க்கு (ForceInfo* மாறிலி finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* மாறிலி rhsVarp = புதிய AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doingAssign() ? "_VassignRHS" : "__VforceRHS")
                        + std::to_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(உண்மை);
                rhsVarp->sigPublic(உண்மை);
                rhsVarp->setForcedByCode();
                varp->addNextHere(rhsVarp);
                finfo.m_rhsVarVscp = புதிய AstVarScope{flp, scopep, rhsVarp};
                scopep->addVarsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                // end
                AstAssign* மாறிலி rhsAssignp = புதிய AstAssign{
                    flp, புதிய AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                எனில் (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* மாறிலி touchCallp = புதிய AstCMethodHard{
                        flp, புதிய AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtypeSetVoid();
                    Ast_முனை_Stmt* மாறிலி touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->addNextHere(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* மாறிலி alwaysp
                    = புதிய AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, rhsAssignp};
                Ast_Sen_மரம்* மாறிலி senTreep
                    = புதிய Ast_Sen_மரம்{flp, புதிய AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* மாறிலி activep = புதிய AstActive{flp, "force-rhs-update", senTreep};
                activep->sen_மரம்_Storep(activep->sentreep());
                activep->addStmtsp(alwaysp);
                scopep->addBlocksp(activep);
            }

            எனில் (info.m_forceRdVscp) {
                எனில் (!m_forceRdUpdateBuilt.செருகு(info.m_varVscp).second) தொடர்;
                AstActive* மாறிலி activeInitp = புதிய AstActive{
                    flp, "force-init",
                    புதிய Ast_Sen_மரம்{flp, புதிய AstSenItem{flp, AstSenItem::Static{}}}};
                activeInitp->sen_மரம்_Storep(activeInitp->sentreep());
                Ast_முனை_Stmt* initStmtp = nullptr;
                எனில் (AstUnpackArrayDType* மாறிலி arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    மாறிலி std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    மாறிலி முழு innerWidth = dims.back()->subDTypep()->skipRefp()->width();
                    initStmtp = foreachUnpackedLeaf(
                        dims, [&](மாறிலி std::vector<முழு>& idx, முழு /*flat*/) -> Ast_முனை_Stmt* {
                            Ast_முனை_கோவை* மாறிலி lhsp = buildNestedArraySel(
                                flp, புதிய AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            திரும்பு புதிய AstAssign{flp, lhsp, makeZeroConst(varp, innerWidth)};
                        });
                } இல்லையெனில் {
                    initStmtp = புதிய AstAssign{
                        flp, புதிய AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->width())};
                }
                initStmtp->addNext(createForceRdUpdateStmt(info));
                activeInitp->addStmtsp(புதிய AstInitial{flp, initStmtp});
                scopep->addBlocksp(activeInitp);

                AstSenItem* itemsp = nullptr;
                auto addSenItem = [&](AstVarScope* vscp) {
                    எனில் (!vscp) திரும்பு;
                    AstSenItem* மாறிலி nextp = புதிய AstSenItem{
                        flp, VEdgeType::ET_CHANGED, புதிய AstVarRef{flp, vscp, VAccess::READ}};
                    எனில் (itemsp) {
                        itemsp->addNext(nextp);
                    } இல்லையெனில் {
                        itemsp = nextp;
                    }
                };
                addSenItem(info.m_forceEnVscp);
                addSenItem(info.m_forceValVscp);
                AstVarRef* மாறிலி origSenRefp = புதிய AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                AstSenItem* மாறிலி origItemp
                    = புதிய AstSenItem{flp, VEdgeType::ET_CHANGED, origSenRefp};
                எனில் (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->addNext(origItemp);
                க்கு (ForceInfo* மாறிலி finfop : forceps) {
                    எனில் (!finfop->m_isExternal) addSenItem(finfop->m_rhsVarVscp);
                }

                AstActive* மாறிலி activep
                    = புதிய AstActive{flp, "force-rd-update", புதிய Ast_Sen_மரம்{flp, itemsp}};
                activep->sen_மரம்_Storep(activep->sentreep());
                activep->addStmtsp(புதிய AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr,
                                                 createForceRdUpdateStmt(info)});
                scopep->addBlocksp(activep);
            }
        }
    }

    Ast_முனை* createRhsUpdatesForWrite(File_வரி* flp, AstVar* writtenVarp) மாறிலி {
        எனில் (!doingAssign()) திரும்பு nullptr;

        மாறிலி auto it = m_rhsDepToForces.find(writtenVarp);
        எனில் (it == m_rhsDepToForces.end()) திரும்பு nullptr;

        Ast_முனை* headp = nullptr;
        Ast_முனை* tailp = nullptr;
        க்கு (மாறிலி ForceInfo* மாறிலி finfop : it->second) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            AstAssign* மாறிலி updatep
                = புதிய AstAssign{flp, புதிய AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_மரம்_Pure(பொய்)};
            எனில் (tailp) {
                tailp->addNextHere(updatep);
            } இல்லையெனில் {
                headp = updatep;
            }
            tailp = updatep;
        }
        திரும்பு headp;
    }

    மாறிலி ForceInfo& getForceInfo(AstAssignForce* forceStmtp) மாறிலி {
        AstVarScope* மாறிலி vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        மாறிலி VarForceInfo* மாறிலி varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        auto it2 = varInfo->m_forces.find(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.end(), "Force statement not found");
        திரும்பு it2->second;
    }

    நிலையான தருக்க selOverlapsAnyForce(மாறிலி VarForceInfo& varInfo, முழு selLsb, முழு selMsb) {
        க்கு (மாறிலி auto& pair : varInfo.m_forces) {
            எனில் (pair.second.m_rangeLsb <= selMsb && pair.second.m_rangeMsb >= selLsb) திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }

    Ast_முனை_கோவை* createForceReadExpression(மாறிலி VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) மாறிலி {
        File_வரி* மாறிலி flp = originalRefp->fileline();
        AstVarRef* மாறிலி refp = originalRefp->clone_மரம்_Pure(பொய்);
        refp->access(VAccess::READ);
        திரும்பு createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   nullptr);
    }

    Ast_முனை_கோவை* createForceReadIndexExpression(மாறிலி VarForceInfo& varInfo,
                                                Ast_முனை_கோவை* originalExprp,
                                                Ast_முனை_கோவை* indexExprp) மாறிலி {
        File_வரி* மாறிலி flp = originalExprp->fileline();
        Ast_முனை_கோவை* மாறிலி exprp = originalExprp->clone_மரம்_Pure(பொய்);
        // Must be an LValue to a static variable
        VN_AS(exprp->getVAccessTargetRecurse(), VarRef)->access(VAccess::READ);
        திரும்பு createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_INDEX, exprp, originalExprp,
                                   indexExprp);
    }

    நிலையான Ast_முனை_கோவை* rebuildSelPath(Ast_முனை_கோவை* pathp, Ast_முனை_கோவை* baseExprp) {
        எனில் (மாறிலி AstSel* மாறிலி selp = VN_CAST(pathp, Sel)) {
            Ast_முனை_கோவை* மாறிலி fromp = rebuildSelPath(selp->fromp(), baseExprp);
            AstSel* மாறிலி outp
                = புதிய AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->widthConst()};
            outp->dtypeFrom(selp);
            திரும்பு outp;
        }
        திரும்பு baseExprp;
    }
};

// Split deassign concat LHS before converting to release internals.
நிலையான வெறுமை splitDeassign(AstDeassign* nodep) {
    AstConcat* மாறிலி concatp = VN_CAST(nodep->lhsp(), Concat);
    எனில் (!concatp) திரும்பு;

    File_வரி* மாறிலி flp = nodep->fileline();
    AstDeassign* மாறிலி newLp = புதிய AstDeassign{flp, concatp->lhsp()->unlinkFrBack()};
    AstDeassign* மாறிலி newRp = புதிய AstDeassign{flp, concatp->rhsp()->unlinkFrBack()};
    Ast_முனை_கோவை* மாறிலி conp = concatp->unlinkFrBack();
    nodep->replaceWith(newLp);
    newLp->addNextHere(newRp);
    VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    VL_DO_DANGLING(conp->delete_மரம்(), conp);

    splitDeassign(newLp);
    splitDeassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

வகுப்பு ForceDiscoveryVisitor final : பொது VNVisitorConst {
    ForceState& m_state;
    தருக்க m_inClockedActive = பொய்;

    வெறுமை buildForceableUnpackedArray(AstVarScope* மாறிலி nodep,
                                     AstUnpackArrayDType* மாறிலி arrDtypep) {
        AstVar* மாறிலி varp = nodep->varp();
        மாறிலி std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.empty(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        மாறிலி Ast_முனை_DType* மாறிலி leafDtypep = dims.back()->subDTypep()->skipRefp();
        மாறிலி AstBasicDType* மாறிலி innerBasicp = leafDtypep->basicp();
        எனில் (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            திரும்பு;
        }

        File_வரி* மாறிலி flp = varp->fileline();
        மாறிலி முழு innerWidth = leafDtypep->width();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* மாறிலி enVscp = info.m_forceEnVscp;
        AstVarScope* மாறிலி valVscp = info.m_forceValVscp;

        AstSenItem* மாறிலி itemsp = புதிய AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                  புதிய AstVarRef{flp, enVscp, VAccess::READ}};
        AstActive* மாறிலி activep = புதிய AstActive{flp, "force-update", புதிய Ast_Sen_மரம்{flp, itemsp}};
        activep->sen_மரம்_Storep(activep->sentreep());

        Ast_முனை_Stmt* மாறிலி alwaysBodyHeadp = ForceState::foreachUnpackedLeaf(
            dims, [&](மாறிலி std::vector<முழு>& idx, முழு flat) -> Ast_முனை_Stmt* {
                AstVarRef* மாறிலி origRefp = புதிய AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_முனை_கோவை* மாறிலி origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_முனை_கோவை* மாறிலி enSelp = ForceState::buildNestedArraySel(
                    flp, புதிய AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_முனை_கோவை* மாறிலி valSelp = ForceState::buildNestedArraySel(
                    flp, புதிய AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_முனை_கோவை* மாறிலி forceExprp = புதிய AstOr{
                    flp, புதிய AstAnd{flp, enSelp, valSelp},
                    புதிய AstAnd{flp, புதிய AstNot{flp, enSelp->clone_மரம்_Pure(பொய்)}, origSelp}};
                Ast_முனை_கோவை* மாறிலி lhsSelp = ForceState::buildNestedArraySel(
                    flp, புதிய AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                AstAssignForce* மாறிலி forceAssignp = புதிய AstAssignForce{flp, lhsSelp, forceExprp};
                forceAssignp->user2(உண்மை);

                Ast_முனை_கோவை* மாறிலி rhsClonep = forceExprp->clone_மரம்_Pure(பொய்);
                rhsClonep->foreach([varp](AstVarRef* மாறிலி r) {
                    எனில் (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/innerWidth - 1,
                                           /*hasArraySel=*/உண்மை, /*skipRdRefresh=*/உண்மை);
                திரும்பு forceAssignp;
            });
        activep->addStmtsp(புதிய AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, alwaysBodyHeadp});
        nodep->scopep()->addBlocksp(activep);
    }

    வெறுமை visit(AstAssignForce* nodep) override {
        எனில் (nodep->user2()) திரும்பு;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* மாறிலி lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* மாறிலி forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, உண்மை);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_முனை_கோவை* மாறிலி rhsExprp = nodep->rhsp()->clone_மரம்_Pure(பொய்);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    வெறுமை visit(AstAssign* nodep) override {
        எனில் (m_state.doingAssign() && m_inClockedActive) {
            எனில் (AstVarRef* மாறிலி lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(AstActive* nodep) override {
        VL_RESTORER(m_inClockedActive);
        m_inClockedActive = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(AstVarScope* nodep) override {
        எனில் (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            எனில் (m_state.doingAssign()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                திரும்பு;
            }

            எனில் (AstUnpackArrayDType* மாறிலி arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                திரும்பு;
            }

            மாறிலி AstBasicDType* மாறிலி bdtypep = nodep->varp()->basicp();
            எனில் (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* மாறிலி varp = nodep->varp();
            File_வரி* மாறிலி flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* மாறிலி enVscp = info.m_forceEnVscp;
            AstVarScope* மாறிலி valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            AstSenItem* மாறிலி itemsp = புதிய AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                      புதிய AstVarRef{flp, enVscp, VAccess::READ}};
            AstActive* மாறிலி activep
                = புதிய AstActive{flp, "force-update", புதிய Ast_Sen_மரம்{flp, itemsp}};
            activep->sen_மரம்_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* மாறிலி refp = புதிய AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* மாறிலி enRefp = புதிய AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* மாறிலி valRefp = புதிய AstVarRef{flp, valVscp, VAccess::READ};
            மாறிலி AstBasicDType* மாறிலி basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_முனை_கோவை* மாறிலி forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_முனை_கோவை*>(புதிய AstOr{
                            flp, புதிய AstAnd{flp, enRefp, valRefp},
                            புதிய AstAnd{flp, புதிய AstNot{flp, enRefp->clone_மரம்_Pure(பொய்)}, refp}})
                      : static_cast<Ast_முனை_கோவை*>(புதிய AstCond{flp, enRefp, valRefp, refp});
            AstAssignForce* மாறிலி forceAssignp
                = புதிய AstAssignForce{flp, புதிய AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(உண்மை);
            activep->addStmtsp(புதிய AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, forceAssignp});
            nodep->scopep()->addBlocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_முனை_கோவை* மாறிலி rhsClonep = forceExprp->clone_மரம்_Pure(பொய்);
            rhsClonep->foreach([varp](AstVarRef* மாறிலி refp) {
                எனில் (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            மாறிலி தருக்க bitwiseVar = ForceState::isBitwiseDType(varp);
            மாறிலி முழு padMsb = bitwiseVar ? (varp->width() - 1) : 0;
            முழு rangeLsb = 0;
            முழு rangeMsb = padMsb;
            எனில் (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, பொய்);
        }
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    explicit ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextConstNull(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

வகுப்பு ForceConvertVisitor final : பொது VNVisitor {
    ForceState& m_state;

    வெறுமை visit(AstAssignForce* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp();
        AstVarRef* மாறிலி lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* மாறிலி forcedVarp = lhsVarRefp->varp();

        மாறிலி ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        மாறிலி ForceState::VarForceInfo* மாறிலி varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_வரி* மாறிலி flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        AstAssign* மாறிலி rhsAssignp
            = புதிய AstAssign{flp, புதிய AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_மரம்_Pure(பொய்)};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        மாறிலி தருக்க bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        எனில் (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_முனை_கோவை* baseExprp = nodep->rhsp()->clone_மரம்_Pure(பொய்);
            baseExprp->foreach(
                [](AstVarRef* மாறிலி refp) { ForceState::markNonReplaceable(refp); });
            எனில் (bitwiseForcedVar) {
                baseExprp = ForceState::zeroPadToBaseWidth(baseExprp, forcedVarp->width(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            எனில் (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* மாறிலி maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_முனை_கோவை* மாறிலி valReadp
                    = புதிய AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_முனை_கோவை* மாறிலி valWritep
                    = புதிய AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_முனை_கோவை* மாறிலி notMaskp = புதிய AstNot{flp, maskConstp};
                Ast_முனை_கோவை* மாறிலி maskedOldp = புதிய AstAnd{flp, valReadp, notMaskp};
                Ast_முனை_கோவை* மாறிலி newValp = புதிய AstOr{flp, maskedOldp, baseExprp};
                valAssignp = புதிய AstAssign{flp, valWritep, newValp};

                Ast_முனை_கோவை* மாறிலி enReadp
                    = புதிய AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_முனை_கோவை* மாறிலி enWritep
                    = புதிய AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_முனை_கோவை* மாறிலி newEnp
                    = புதிய AstOr{flp, enReadp, maskConstp->clone_மரம்_Pure(பொய்)};
                enAssignp = புதிய AstAssign{flp, enWritep, newEnp};
            } இல்லையெனில் {
                AstConst* மாறிலி oneConstp = ForceState::makeRangeMaskConst(nodep, 1, 0, 0);
                Ast_முனை_கோவை* மாறிலி rhsValp = ForceState::cast_To_முனை_DType(baseExprp, forcedVarp);
                valAssignp = புதிய AstAssign{
                    flp, புதிய AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = புதிய AstAssign{
                    flp, புதிய AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        மாறிலி AstSel* மாறிலி selLhsp = VN_CAST(lhsp, Sel);
        மாறிலி தருக்க arrayBitSel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->width();
        Ast_முனை_கோவை* மாறிலி rhsDatap = ForceState::build_Rhs_Data_கோவை(flp, info);
        AstCExpr* மாறிலி rhsAddrp = புதிய AstCExpr{flp};
        rhsAddrp->add("&(");
        rhsAddrp->add(rhsDatap);
        rhsAddrp->add(")");
        AstCMethodHard* மாறிலி addForceCallp = புதிய AstCMethodHard{
            flp, புதிய AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ADD,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        addForceCallp->addPinsp(rhsAddrp);
        addForceCallp->addPinsp(
            ForceState::makeConst32(flp, arrayBitSel ? info.m_padLsb : info.m_rangeLsb));
        எனில் (arrayBitSel) {
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padLsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padMsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, selLhsp->fromp()->width()));
        }
        addForceCallp->dtypeSetVoid();
        Ast_முனை_Stmt* மாறிலி stmtp = addForceCallp->makeStmt();

        Ast_முனை* tailp = rhsAssignp;
        எனில் (valAssignp) {
            tailp->addNextHere(valAssignp);
            tailp = valAssignp;
        }
        எனில் (enAssignp) {
            tailp->addNextHere(enAssignp);
            tailp = enAssignp;
        }
        tailp->addNextHere(stmtp);
        எனில் (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp();
        AstVarRef* மாறிலி lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* மாறிலி releasedVarp = lhsVarRefp->varp();

        மாறிலி ForceState::VarForceInfo* மாறிலி varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        எனில் (!varInfo) {
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }
        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        File_வரி* மாறிலி flp = nodep->fileline();

        மாறிலி ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, பொய்);

        மாறிலி AstSel* மாறிலி selLhsp = VN_CAST(lhsp, Sel);
        மாறிலி தருக்க arrayBitSel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->width();
        AstCMethodHard* மாறிலி releaseCallp = புதிய AstCMethodHard{
            flp, புதிய AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        எனில் (arrayBitSel) {
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtypeSetVoid();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_முனை_Stmt* மாறிலி releasep = releaseCallp->makeStmt();

        AstAssign* clearEnp = nullptr;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        எனில் (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_முனை_கோவை* மாறிலி enWritep
                = புதிய AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            எனில் (ForceState::isBitwiseDType(releasedVarp)) {
                மாறிலி முழு varWidth = releasedVarp->width();
                எனில் (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == varWidth - 1) {
                    clearEnp
                        = புதிய AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, varWidth)};
                } இல்லையெனில் {
                    // forceEn = forceEn & ~mask(range);
                    Ast_முனை_கோவை* மாறிலி enReadp
                        = புதிய AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* மாறிலி maskConst = ForceState::makeRangeMaskConst(
                        nodep, varWidth, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_முனை_கோவை* மாறிலி newEnp
                        = புதிய AstAnd{flp, enReadp, புதிய AstNot{flp, maskConst}};
                    clearEnp = புதிய AstAssign{flp, enWritep, newEnp};
                }
            } இல்லையெனில் {
                clearEnp = புதிய AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        மாறிலி AstSel* மாறிலி selp = VN_CAST(lhsp, Sel);
        Ast_முனை_கோவை* மாறிலி basep = selp ? selp->fromp() : lhsp;

        Ast_முனை* stmtListp = releasep;
        எனில் (clearEnp) {
            clearEnp->addNextHere(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        மாறிலி தருக்க fullBitwiseRelease
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->width() - 1;
        எனில் (!releasedVarp->isContinuously()
            && !(m_state.doingAssign() && m_state.hasClockedWrite(releasedVarp)
                 && fullBitwiseRelease)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            மாறிலி தருக்க hasOpaquePath = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_முனை_கோவை* forceReadp = nullptr;
            எனில் (hasOpaquePath) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } இல்லையெனில் எனில் (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, basep,
                    ForceState::build_Flatten_Index_கோவை(flp, rangeInfo.m_arrayInfo));
                எனில் (selp) { forceReadp = ForceState::rebuildSelPath(lhsp, forceReadp); }
            } இல்லையெனில் {
                forceReadp
                    = selp ? ForceState::rebuildSelPath(
                                 lhsp, m_state.createForceReadExpression(*varInfo, lhsVarRefp))
                           : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            }
            AstAssign* மாறிலி assignp = புதிய AstAssign{flp, lhsp->clone_மரம்_Pure(பொய்), forceReadp};
            assignp->addNextHere(stmtListp);
            stmtListp = assignp;
        }

        எனில் (varInfo->m_forceRdVscp) stmtListp->addNext(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

வகுப்பு ForceReplaceVisitor final : பொது VNVisitor {
    மாறிலி ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_முனை_Stmt* m_stmtp = nullptr;
    தருக்க m_inLogic = பொய்;

    வெறுமை iterateLogic(Ast_முனை* nodep) {
        VL_RESTORER(m_inLogic);
        m_inLogic = உண்மை;
        iterateChildren(nodep);
    }

    வெறுமை visit(Ast_முனை_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstAssign* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        எனில் (AstVarRef* மாறிலி lhsp = VN_CAST(nodep->lhsp()->baseFromp(உண்மை), VarRef)) {
            எனில் (Ast_முனை* மாறிலி updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->addNextHere(updatep);
            }
        }
    }
    வெறுமை visit(AstAssignCont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateAndNextNull(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    வெறுமை visit(AstDeassign*) override {}
    வெறுமை visit(AstCFunc* nodep) override { iterateLogic(nodep); }
    வெறுமை visit(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    வெறுமை visit(Ast_முனை_Procedure* nodep) override { iterateLogic(nodep); }
    வெறுமை visit(AstAlways* nodep) override {
        எனில் (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            திரும்பு;
        }
        iterateLogic(nodep);
    }
    வெறுமை visit(AstSenItem* nodep) override { iterateLogic(nodep); }
    வெறுமை visit(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* மாறிலி refp = VN_CAST(nodep->fromp(), VarRef);
        எனில் (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            visit(static_cast<Ast_முனை*>(nodep));
            திரும்பு;
        }

        AstVar* மாறிலி varp = refp->varp();
        மாறிலி ForceState::VarForceInfo* மாறிலி varInfo = m_state.getVarInfo(refp->varScopep());
        எனில் (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.empty()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            visit(static_cast<Ast_முனை*>(nodep));
            திரும்பு;
        }

        எனில் (மாறிலி AstConst* மாறிலி lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            மாறிலி முழு selLsb = lsbConstp->toSInt();
            மாறிலி முழு selMsb = selLsb + nodep->width() - 1;
            எனில் (!varp->isSigPublic()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                visit(static_cast<Ast_முனை*>(nodep));
                திரும்பு;
            }
        }

        File_வரி* மாறிலி flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* மாறிலி refClonep = refp->clone_மரம்_Pure(பொய்);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* மாறிலி callp = புதிய AstCMethodHard{
            flp, புதிய AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->width())};
        callp->addPinsp(refClonep);
        callp->addPinsp(nodep->lsbp()->clone_மரம்_Pure(பொய்));
        callp->addPinsp(ForceState::makeConst32(flp, nodep->width()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstArraySel* nodep) override {
        எனில் (மாறிலி AstArraySel* மாறிலி backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            எனில் (backSelp->fromp() == nodep) {
                iterateChildren(nodep);
                திரும்பு;
            }
        }

        Ast_முனை* மாறிலி basep = nodep->baseFromp(உண்மை);
        AstVarRef* மாறிலி baseRefp = VN_CAST(basep, VarRef);
        எனில் (!baseRefp) {
            iterateChildren(nodep);
            திரும்பு;
        }
        AstVar* மாறிலி varp = baseRefp->varp();
        மாறிலி ForceState::VarForceInfo* மாறிலி varInfo = m_state.getVarInfo(baseRefp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        எனில் (m_state.isNotReplaceable(baseRefp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            திரும்பு;
        }

        எனில் (!baseRefp->access().isReadOnly()) {
            iterateChildren(nodep);
            திரும்பு;
        }
        எனில் (m_state.doingAssign() && varInfo->m_forceRdVscp) {
            baseRefp->varp(varInfo->m_forceRdVscp->varp());
            baseRefp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            திரும்பு;
        }
        மாறிலி ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        க்கு (AstArraySel* மாறிலி selp : arrayInfo.m_sels) iterateAndNextNull(selp->bitp());
        Ast_முனை_கோவை* மாறிலி indexExprp
            = ForceState::build_Flatten_Index_கோவை(nodep->fileline(), arrayInfo);
        Ast_முனை_கோவை* மாறிலி readExprp
            = m_state.createForceReadIndexExpression(*varInfo, nodep, indexExprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை visit(AstVarRef* nodep) override {
        எனில் (m_state.isNotReplaceable(nodep)) திரும்பு;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        எனில் (மாறிலி AstArraySel* மாறிலி backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            எனில் (backSelp->fromp() == nodep) திரும்பு;
        }

        மாறிலி ForceState::VarForceInfo* மாறிலி varInfo = m_state.getVarInfo(nodep->varScopep());
        எனில் (!varInfo) திரும்பு;

        எனில் (varInfo->m_forceRdVscp) {
            எனில் (nodep->access().isRW()) {
                எனில் (m_inLogic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                திரும்பு;
            }
            எனில் (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_முனை_கோவை* மாறிலி readp
                    = m_state.createForceRdExpression(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு;
            }
            எனில் (m_inLogic && m_stmtp) {
                m_stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
            }
            திரும்பு;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        எனில் (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            திரும்பு;
        }

        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        எனில் (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } இல்லையெனில் எனில் (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_முனை_கோவை* மாறிலி readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (ForceState::isOutermostOpaquePathSelector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_முனை_கோவை* மாறிலி exprp = VN_AS(nodep, முனை_கோவை);
            Ast_முனை* மாறிலி basep = exprp->baseFromp(உண்மை);
            AstVarRef* மாறிலி baseRefp = VN_CAST(basep, VarRef);
            எனில் (baseRefp) {
                AstVar* மாறிலி varp = baseRefp->varp();
                எனில் (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    மாறிலி ForceState::VarForceInfo* மாறிலி varInfo
                        = m_state.getVarInfo(baseRefp->varScopep());
                    எனில் (!m_state.isNotReplaceable(baseRefp) && varInfo) {
                        மாறிலி முழு forcePathIndex = varInfo->findForcePathIndex(exprp);
                        எனில் (forcePathIndex >= 0) {
                            எனில் (!baseRefp->access().isReadOnly()) திரும்பு;
                            Ast_முனை_கோவை* மாறிலி readExprp = m_state.createForceReadIndexExpression(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), forcePathIndex));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                            திரும்பு;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

பொது:
    explicit ForceReplaceVisitor(AstNetlist* nodep, மாறிலி ForceState& state)
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

நிலையான வெறுமை forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::ProtectedVarRefSet& permanentlyProtected,
                         ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    எனில் (!v3Global.hasForceableSignals()) திரும்பு;
    ForceState state{பொய், helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_மரம்("force", 0, dump_மரம்_Either_Level() >= 3);
}

நிலையான வெறுமை assignAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::ProtectedVarRefSet& permanentlyProtected,
                          ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    எனில் (!v3Global.hasAssignDeassign()) திரும்பு;

    std::vector<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.தள்ளு_back(deassignp); });
    க்கு (AstDeassign* மாறிலி deassignp : deassignps) splitDeassign(deassignp);

    std::vector<AstAssignCont*> assignContps;
    deassignps.clear();
    nodep->foreach([&](Ast_முனை_Stmt* nodep) {
        எனில் (AstAssignCont* மாறிலி assignContp = VN_CAST(nodep, AssignCont)) {
            assignContps.தள்ளு_back(assignContp);
        } இல்லையெனில் எனில் (AstDeassign* மாறிலி deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.தள்ளு_back(deassignp);
        }
    });

    க்கு (AstAssignCont* மாறிலி assignp : assignContps) {
        assignp->replaceWith(புதிய AstAssignForce{assignp->fileline(),
                                                assignp->lhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
        VL_DO_DANGLING(assignp->delete_மரம்(), assignp);
    }
    க்கு (AstDeassign* மாறிலி deassignp : deassignps) {
        deassignp->replaceWith(
            புதிய AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_மரம்_Pure(உண்மை)});
        VL_DO_DANGLING(deassignp->delete_மரம்(), deassignp);
    }
    ForceState state{உண்மை, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_மரம்("assign-deassign", 0, dump_மரம்_Either_Level() >= 3);
}

வெறுமை V3Force::forceAndAssignAll(AstNetlist* nodep) {
    மாறிலி VNUser3InUse user3InUse;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::ProtectedVarRefSet permanentlyProtected;
    ForceState::BuiltVarScopeSet forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    assignAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
