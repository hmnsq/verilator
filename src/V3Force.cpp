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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Force.h"

#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ଶ୍ରେଣୀ ForceState final {
ସର୍ବସାଧାରଣ:
    ସଂରଚନା ForceRange VL_NOT_FINAL {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_padLsb = 0;  // Bit positions for RHS padding
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_padMsb = 0;  // Bit positions for RHS padding
    };

    ସଂରଚନା ForceInfo final : ForceRange {
        // MEMBERS
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_forceId = 0;  // Unique (per signal) variable of this force assignment
        ତାର୍କିକ m_hasArraySel = ମିଥ୍ୟା;  // If this has an array select on LHS
        ତାର୍କିକ m_isExternal = ମିଥ୍ୟା;  // Synthetic force for the public force controls
        ତାର୍କିକ m_skipRdRefresh = ମିଥ୍ୟା;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = nullptr;  // Scope of the var containing RHSID
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_rhsExprp = nullptr;  // Expression on RHS of this force assignment

        ForceInfo() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        ForceInfo(ପୂର୍ଣ୍ଣସଂଖ୍ୟା rangeLsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା rangeMsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା padLsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା padMsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା forceId,
                  ତାର୍କିକ hasArraySel, AstVarScope* rhsVarVscp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsExprp)
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

    ସଂରଚନା VarForceInfo final {
        AstVarScope* m_forceVecVscp = nullptr;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = nullptr;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = nullptr;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = nullptr;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = nullptr;  // Scope of the forced (original) variable
        AstVar* m_varp = nullptr;  // The forced (original) variable
        AstScope* m_scopep = nullptr;  // Scope containing the forced variable
        std::unordered_map<AstAssignForce*, ForceInfo> m_forces;
        std::unordered_map<string, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> m_forcePathToIndex;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_nextForcePathIndex = 1;  // Start at 1 so 0 can be the base path (whole signal)

    ବ୍ୟକ୍ତିଗତ:
        ସ୍ଥିର ଶୂନ୍ୟ build_Force_Path_ଚାବି_Recurse(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, string& out) {
            ଯଦି (VN_IS(nodep, VarRef)) ଫେରନ୍ତୁ;
            ଯଦି (ଅଚଳ AstSel* ଅଚଳ selp = VN_CAST(nodep, Sel)) {
                build_Force_Path_ଚାବି_Recurse(selp->fromp(), out);
                out += "|S" + cvtToStr(selp->lsbConst()) + ":" + cvtToStr(selp->widthConst());
                ଫେରନ୍ତୁ;
            }
            ଯଦି (AstStructSel* ଅଚଳ selp = VN_CAST(nodep, StructSel)) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = selp->fromp();
                build_Force_Path_ଚାବି_Recurse(fromp, out);
                out += "|T" + selp->name();
                ଫେରନ୍ତୁ;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        ସ୍ଥିର string force_Path_ଚାବି(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
            string out;
            build_Force_Path_ଚାବି_Recurse(nodep, out);
            ଫେରନ୍ତୁ out;
        }

    ସର୍ବସାଧାରଣ:
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା getOrCreateForcePathIndex(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
            ଅଚଳ auto pair
                = m_forcePathToIndex.emplace(force_Path_ଚାବି(nodep), m_nextForcePathIndex);
            ଯଦି (pair.second) ++m_nextForcePathIndex;
            ଫେରନ୍ତୁ pair.first->second;
        }

        ପୂର୍ଣ୍ଣସଂଖ୍ୟା findForcePathIndex(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) ଅଚଳ {
            ଅଚଳ auto it = m_forcePathToIndex.find(force_Path_ଚାବି(nodep));
            ଫେରନ୍ତୁ it != m_forcePathToIndex.end() ? it->second : -1;
        }
    };

    ସଂରଚନା ForceHelperVars final {
        AstVar* m_rdVarp = nullptr;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = nullptr;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = nullptr;  // The __VforceVal (force value) variable
    };

    ସଂରଚନା ArraySelInfo final {
        std::vector<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        ତାର୍କିକ m_hasBitSel = ମିଥ୍ୟା;  // Bit-select was found along the path too
    };

    ସଂରଚନା ForceRangeInfo final : ForceRange {
        ତାର୍କିକ m_hasArraySel = ମିଥ୍ୟା;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

ବ୍ୟକ୍ତିଗତ:
    using ScopeVarCache = std::unordered_map<ଅଚଳ AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    ଅଚଳ VNUser1InUse m_user1InUse;
    ଅଚଳ VNUser2InUse m_user2InUse;

ସର୍ବସାଧାରଣ:
    using ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    using ProtectedVarRefSet = std::unordered_set<ଅଚଳ AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    using BuiltVarScopeSet = std::unordered_set<ଅଚଳ AstVarScope*>;

ବ୍ୟକ୍ତିଗତ:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::vector<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> m_varToId;
    std::unordered_set<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::vector<ForceInfo*>> m_rhsDepToForces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    ତାର୍କିକ m_doingAssign = ମିଥ୍ୟା;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    ProtectedVarRefSet& m_permanentlyProtected;
    BuiltVarScopeSet& m_forceRdUpdateBuilt;

ସର୍ବସାଧାରଣ:
    ForceState(ତାର୍କିକ doingAssign, ForceHelperVarsByVar& forceHelperVarsByVar,
               ProtectedVarRefSet& permanentlyProtected, BuiltVarScopeSet& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doingAssign{doingAssign}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    ସ୍ଥିର AstConst* makeZeroConst(Ast_ଗଣ୍ଠି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) {
        V3_ସଂଖ୍ୟା zero{nodep, width};
        zero.setAllBits0();
        ଫେରନ୍ତୁ ନୂତନ AstConst{nodep->fileline(), zero};
    }

    ସ୍ଥିର AstConst* makeConst32(File_ଧାଡ଼ି* flp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ) {
        ଫେରନ୍ତୁ ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, static_cast<uint32_t>(ମୂଲ୍ୟ)};
    }

    ସ୍ଥିର AstConst* makeRangeMaskConst(Ast_ଗଣ୍ଠି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb) {
        V3_ସଂଖ୍ୟା mask{nodep, width};
        mask.setAllBits0();
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = lsb; bit <= msb; ++bit) mask.setBit(bit, 1);
        ଫେରନ୍ତୁ ନୂତନ AstConst{nodep->fileline(), mask};
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* zeroPadToBaseWidth(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା baseWidth, ପୂର୍ଣ୍ଣସଂଖ୍ୟା padLsb,
                                           ପୂର୍ଣ୍ଣସଂଖ୍ୟା padMsb) {
        ଯଦି (baseWidth <= 0) ଫେରନ୍ତୁ exprp;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ତଳ_Pad = padLsb;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଉପର_Pad = baseWidth - (padMsb + 1);
        ଯଦି (ତଳ_Pad > 0) {
            exprp = ନୂତନ AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, ତଳ_Pad)};
        }
        ଯଦି (ଉପର_Pad > 0) {
            exprp = ନୂତନ AstConcat{exprp->fileline(), makeZeroConst(exprp, ଉପର_Pad), exprp};
        }
        ଫେରନ୍ତୁ exprp;
    }

    ସ୍ଥିର ତାର୍କିକ isUnpackedArrayDType(ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) {
        ଫେରନ୍ତୁ VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    ସ୍ଥିର ତାର୍କିକ isBitwiseDType(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଅଚଳ AstBasicDType* ଅଚଳ basicp = nodep->dtypep()->skipRefp()->basicp();
        ଫେରନ୍ତୁ basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* cast_To_ଗଣ୍ଠି_DType(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି* dtypeFromp) {
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = dtypeFromp->dtypep()->skipRefp();
        ଅଚଳ AstBasicDType* ଅଚଳ basicp = dtypep->basicp();
        ଯଦି (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            ଫେରନ୍ତୁ exprp;
        }
        ଫେରନ୍ତୁ ନୂତନ AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    ସ୍ଥିର ଶୂନ୍ୟ markNonReplaceable(AstVarRef* ଅଚଳ nodep) { nodep->user1SetOnce(); }
    ତାର୍କିକ isNotReplaceable(ଅଚଳ AstVarRef* ଅଚଳ nodep) ଅଚଳ {
        ଫେରନ୍ତୁ nodep->user1() || m_permanentlyProtected.ଗଣନା_2(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    ଶୂନ୍ୟ markPermanentlyProtected(AstVarRef* ଅଚଳ nodep) ଅଚଳ {
        markNonReplaceable(nodep);
        m_permanentlyProtected.ଭର୍ତ୍ତି(nodep);
    }

    ସ୍ଥିର std::vector<ForceInfo*> forceInfosInIdOrder(VarForceInfo& info) {
        std::vector<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        ପାଇଁ (auto& it : info.m_forces) forceps.ଠେଲ_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](ଅଚଳ ForceInfo* ap, ଅଚଳ ForceInfo* bp) {
            ଫେରନ୍ତୁ ap->m_forceId < bp->m_forceId;
        });
        ଫେରନ୍ତୁ forceps;
    }

    ସ୍ଥିର std::vector<ଅଚଳ ForceInfo*> forceInfosInIdOrder(ଅଚଳ VarForceInfo& info) {
        std::vector<ଅଚଳ ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        ପାଇଁ (ଅଚଳ auto& it : info.m_forces) forceps.ଠେଲ_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](ଅଚଳ ForceInfo* ap, ଅଚଳ ForceInfo* bp) {
            ଫେରନ୍ତୁ ap->m_forceId < bp->m_forceId;
        });
        ଫେରନ୍ତୁ forceps;
    }

    ସ୍ଥିର ତାର୍କିକ isOpaquePathSelector(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଫେରନ୍ତୁ VN_IS(nodep, Sel) || VN_IS(nodep, ଗଣ୍ଠି_Sel) || VN_IS(nodep, StructSel);
    }

    ସ୍ଥିର ତାର୍କିକ isOutermostOpaquePathSelector(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (!isOpaquePathSelector(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ backp = nodep->backp();
        ଫେରନ୍ତୁ !backp || !isOpaquePathSelector(backp);
    }

    ସ୍ଥିର AstVarRef* getOneVarRef(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* forceStmtp) {
        Ast_ଗଣ୍ଠି* ଅଚଳ basep = forceStmtp->baseFromp(ସତ୍ୟ);
        ଯଦି (AstSampled* sampledp = VN_CAST(basep, Sampled))
            ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = VN_CAST(sampledp->exprp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି))
                ଫେରନ୍ତୁ getOneVarRef(exprp);
        AstVarRef* ଅଚଳ varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        ଫେରନ୍ତୁ varRefp;
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* buildNestedArraySel(File_ଧାଡ଼ି* flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp,
                                            ଅଚଳ std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>& indicies) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* curp = fromp;
        ପାଇଁ (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା idx : indicies) curp = ନୂତନ AstArraySel{flp, curp, idx};
        ଫେରନ୍ତୁ curp;
    }

    template <typename Fn>
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Stmt* foreachUnpackedLeaf(ଅଚଳ std::vector<AstUnpackArrayDType*>& dims,
                                            Fn buildLeaf) {
        Ast_ଗଣ୍ଠି_Stmt* headp = nullptr;
        Ast_ଗଣ୍ଠି_Stmt* tailp = nullptr;
        ଯଦି (dims.empty()) ଫେରନ୍ତୁ nullptr;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା total = 1;
        ପାଇଁ (ଅଚଳ AstUnpackArrayDType* ଅଚଳ d : dims) total *= d->elementsConst();
        ଯଦି (total <= 0) ଫେରନ୍ତୁ nullptr;
        std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା> idx(dims.size(), 0);
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା flat = 0; flat < total; ++flat) {
            Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtp = buildLeaf(idx, flat);
            ଯଦି (!headp) {
                headp = stmtp;
            } ଅନ୍ୟଥା {
                tailp->addNext(stmtp);
            }
            tailp = stmtp;
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା d = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(dims.size()) - 1; d >= 0; --d) {
                ଯଦି (++idx[d] < dims[d]->elementsConst()) ଭାଙ୍ଗ;
                idx[d] = 0;
            }
        }
        ଫେରନ୍ତୁ headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, AstVar* varp,
                                     ତାର୍କିକ requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.empty();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->width() - 1) : 0;

        ଯଦି (ଅଚଳ AstSel* ଅଚଳ outerSelp = VN_CAST(lhsp, Sel)) {
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା totalLsb = 0;
            ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* curp = lhsp; ଅଚଳ AstSel* ଅଚଳ selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                ଯଦି (requireConstRangeSelect) {
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
        ଯଦି (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା> indices;
            indices.reserve(info.m_arrayInfo.m_sels.size());
            ପାଇଁ (AstArraySel* ଅଚଳ selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.ଠେଲ_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            info.m_rangeLsb = flattenIndex(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } ଅନ୍ୟଥା ଯଦି (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        ଯଦି (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା index = varInfo.getOrCreateForcePathIndex(lhsp);
            info.m_rangeLsb = index;
            info.m_rangeMsb = index;
        }
        ଫେରନ୍ତୁ info;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* add_Rhs_ମୂଲ୍ୟ_Reads(ଅଚଳ VarForceInfo& varInfo, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) ଅଚଳ {
        ଯଦି (!doingAssign()) ଫେରନ୍ତୁ exprp;

        ଅଚଳ std::vector<ଅଚଳ ForceInfo*> forceps = forceInfosInIdOrder(varInfo);
        ଯଦି (forceps.empty()) ଫେରନ୍ତୁ exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* ଅଚଳ cexprp = ନୂତନ AstCExpr{exprp->fileline(), AstCExpr::Pure{}};
        cexprp->dtypeFrom(exprp);
        cexprp->add("(");
        ପାଇଁ (ଅଚଳ ForceInfo* ଅଚଳ finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* ଅଚଳ refp
                = ନୂତନ AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->add("(void)(");
            cexprp->add(refp);
            cexprp->add("), ");
        }
        cexprp->add(exprp);
        cexprp->add(")");
        ଫେରନ୍ତୁ cexprp;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* createForceReadCall(ଅଚଳ VarForceInfo& varInfo, File_ଧାଡ଼ି* flp, VCMethod method,
                                     Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* originalExprp, Ast_ଗଣ୍ଠି* dtypeFromp,
                                     Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* indexExprp) ଅଚଳ {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ origValp
            = add_Rhs_ମୂଲ୍ୟ_Reads(varInfo, cast_To_ଗଣ୍ଠି_DType(originalExprp, dtypeFromp));

        AstCMethodHard* ଅଚଳ callp = ନୂତନ AstCMethodHard{
            flp, ନୂତନ AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, method, origValp};
        ଯଦି (indexExprp) callp->addPinsp(indexExprp);
        callp->dtypeFrom(dtypeFromp);
        ଫେରନ୍ତୁ callp;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* createForceRdExpression(ଅଚଳ VarForceInfo& varInfo, File_ଧାଡ଼ି* flp) ଅଚଳ {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* ଅଚଳ varp = varInfo.m_varVscp->varp();
        ଯଦି (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            ଯଦି (doingAssign()) { ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, ସତ୍ୟ);
            AstVarRef* ଅଚଳ baseRefp = ନୂତନ AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(baseRefp);
            ଫେରନ୍ତୁ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, baseRefp, varp,
                                       nullptr);
        }
        AstVarRef* ଅଚଳ baseRefp = ନୂତନ AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(baseRefp);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enRefp = ନୂତନ AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valRefp = ନୂତନ AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        ଯଦି (isBitwiseDType(varp)) {
            ଫେରନ୍ତୁ ନୂତନ AstOr{
                flp, ନୂତନ AstAnd{flp, enRefp, valRefp},
                ନୂତନ AstAnd{flp, ନୂତନ AstNot{flp, enRefp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)}, baseRefp}};
        }
        ଫେରନ୍ତୁ ନୂତନ AstCond{flp, enRefp, valRefp, baseRefp};
    }

    Ast_ଗଣ୍ଠି_Stmt* createForceRdUpdateStmt(ଅଚଳ VarForceInfo& varInfo) ଅଚଳ {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* ଅଚଳ varp = varInfo.m_varVscp->varp();
        ଯଦି (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            ଫେରନ୍ତୁ createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_ଧାଡ଼ି* ଅଚଳ flp = varInfo.m_varVscp->fileline();
        ଫେରନ୍ତୁ ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             createForceRdExpression(varInfo, flp)};
    }

    Ast_ଗଣ୍ଠି_Stmt* createForceRdUpdateStmtUnpacked(ଅଚଳ VarForceInfo& varInfo) ଅଚଳ {
        File_ଧାଡ଼ି* ଅଚଳ flp = varInfo.m_varVscp->fileline();
        AstVar* ଅଚଳ varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* ଅଚଳ arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        ଅଚଳ std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtsp = foreachUnpackedLeaf(
            dims, [&](ଅଚଳ std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>& idx, ପୂର୍ଣ୍ଣସଂଖ୍ୟା /*flat*/) -> Ast_ଗଣ୍ଠି_Stmt* {
                AstVarRef* ଅଚଳ baseRefp = ନୂତନ AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(baseRefp);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ baseSelp = buildNestedArraySel(flp, baseRefp, idx);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enSelp = buildNestedArraySel(
                    flp, ନୂତନ AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valSelp = buildNestedArraySel(
                    flp, ନୂତନ AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ readExprp = ନୂତନ AstOr{
                    flp, ନୂତନ AstAnd{flp, enSelp, valSelp},
                    ନୂତନ AstAnd{flp, ନୂତନ AstNot{flp, enSelp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)}, baseSelp}};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rdLhsSelp = buildNestedArraySel(
                    flp, ନୂତନ AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                ଫେରନ୍ତୁ ନୂତନ AstAssign{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        ପାଇଁ (ଅଚଳ ForceInfo* ଅଚଳ finfop : forceInfosInIdOrder(varInfo)) {
            ଯଦି (!finfop->m_isExternal) ଜାରି;
            stmtsp->addNext(ନୂତନ AstAssign{flp,
                                          ନୂତନ AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)});
        }
        ଫେରନ୍ତୁ stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        ଅଚଳ auto it = m_varToId.find(vscp);
        ଯଦି (it != m_varToId.end()) ଫେରନ୍ତୁ m_varInfos[it->second];

        m_varToId.emplace(vscp, m_varInfos.size());
        m_varInfos.emplace_back();
        VarForceInfo& info = m_varInfos.back();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* ଅଚଳ varp = info.m_varp;
        ଯଦି (!varp->isForceable()) ଫେରନ୍ତୁ info;

        File_ଧାଡ଼ି* ଅଚଳ flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        ଅଚଳ ତାର୍କିକ helperVarsBuilt = helperVars.m_rdVarp != nullptr;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != nullptr)
                        && helperVarsBuilt == (helperVars.m_valVarp != nullptr),
                    varp, "Incomplete force helper set");
        ଯଦି (!helperVarsBuilt) {
            ଅଚଳ ତାର୍କିକ unpacked = isUnpackedArrayDType(varp->dtypep());
            ଅଚଳ VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
            helperVars.m_rdVarp
                = ନୂତନ AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sigPublic(ସତ୍ୟ);
            helperVars.m_enVarp
                = ନୂତନ AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(ସତ୍ୟ);
            helperVars.m_valVarp
                = ନୂତନ AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(ସତ୍ୟ);
            varp->addNextHere(helperVars.m_rdVarp);
            varp->addNextHere(helperVars.m_enVarp);
            varp->addNextHere(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = findScopeVar(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = findScopeVar(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = findScopeVar(info.m_scopep, helperVars.m_valVarp);
        ଯଦି (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } ଅନ୍ୟଥା {
            info.m_forceRdVscp = ନୂତନ AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = ନୂତନ AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = ନୂତନ AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->addVarsp(info.m_forceRdVscp);
            info.m_scopep->addVarsp(info.m_forceEnVscp);
            info.m_scopep->addVarsp(info.m_forceValVscp);
        }
        ଫେରନ୍ତୁ info;
    }

    ଶୂନ୍ୟ markClockedWrite(AstVar* varp) { m_clockedWrites.ଭର୍ତ୍ତି(varp); }
    ତାର୍କିକ hasClockedWrite(AstVar* varp) ଅଚଳ { ଫେରନ୍ତୁ m_clockedWrites.ଗଣନା_2(varp); }

    ତାର୍କିକ doingAssign() ଅଚଳ { ଫେରନ୍ତୁ m_doingAssign; }

    ଅଚଳ VarForceInfo* getVarInfo(AstVarScope* vscp) ଅଚଳ {
        ଅଚଳ auto it = m_varToId.find(vscp);
        ଫେରନ୍ତୁ it != m_varToId.end() ? &m_varInfos[it->second] : nullptr;
    }

    AstVarScope* findScopeVar(AstScope* scopep, ଅଚଳ AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        ଯଦି (cache.empty()) {
            ପାଇଁ (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        ଅଚଳ auto it = cache.find(varp);
        ଫେରନ୍ତୁ it != cache.end() ? it->second : nullptr;
    }
    ଶୂନ୍ୟ addForceAssignment(AstVar* varp, AstVarScope* vscp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsExprp,
                            AstAssignForce* forceStmtp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା rangeLsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା rangeMsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା padLsb,
                            ପୂର୍ଣ୍ଣସଂଖ୍ୟା padMsb, ତାର୍କିକ hasArraySel, ତାର୍କିକ skipRdRefresh = ମିଥ୍ୟା) {
        v3Global.setUsesForce();
        varp->setForcedByCode();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା forceId = info.m_forces.size();
        File_ଧାଡ଼ି* ଅଚଳ flp = varp->fileline();
        AstScope* ଅଚଳ scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        ଯଦି (!info.m_forceVecVscp) {
            AstCDType* ଅଚଳ forceVecDtypep = ନୂତନ AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->addTypesp(forceVecDtypep);

            AstVar* ଅଚଳ forceVecVarp
                = ନୂତନ AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doingAssign ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(ମିଥ୍ୟା);
            forceVecVarp->isInternal(ସତ୍ୟ);
            varp->addNextHere(forceVecVarp);
            info.m_forceVecVscp = ନୂତନ AstVarScope{flp, scopep, forceVecVarp};
            scopep->addVarsp(info.m_forceVecVscp);
        }

        auto pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, nullptr, rhsExprp});
        ForceInfo& finfo = pair.first->second;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        ଯଦି (doingAssign()) {
            std::vector<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* ଅଚଳ refp) {
                ଯଦି (!refp->access().isReadOnly()) ଫେରନ୍ତୁ;
                AstVar* ଅଚଳ depVarp = refp->varp();
                ଯଦି (depVarp
                    && std::find(depVarps.begin(), depVarps.end(), depVarp) == depVarps.end()) {
                    depVarps.ଠେଲ_back(depVarp);
                }
            });
            ପାଇଁ (AstVar* ଅଚଳ depVarp : depVarps) m_rhsDepToForces[depVarp].ଠେଲ_back(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    ସ୍ଥିର ଶୂନ୍ୟ collectArraySelInfo(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ArraySelInfo& info) {
        ଯଦି (ଅଚଳ auto* ଅଚଳ selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = ସତ୍ୟ;
            collectArraySelInfo(selp->fromp(), info);
        } ଅନ୍ୟଥା ଯଦି (auto* ଅଚଳ selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.ଠେଲ_back(selp);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ auto* ଅଚଳ memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    ସ୍ଥିର ArraySelInfo getArraySelInfo(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        ଫେରନ୍ତୁ info;
    }

    ସ୍ଥିର std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା> arraySelDimSizes(ଅଚଳ ArraySelInfo& info) {
        std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା> dims;
        dims.reserve(info.m_sels.size());
        ପାଇଁ (AstArraySel* ଅଚଳ selp : info.m_sels) {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = selp->fromp()->dtypep()->skipRefp();
            ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.ଠେଲ_back(arrayp->declRange().elements());
        }
        ଫେରନ୍ତୁ dims;
    }

    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା flattenIndex(ଅଚଳ std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>& indices, ଅଚଳ std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>& dimSizes) {
        UASSERT(indices.size() == dimSizes.size(), "Array index and dimension size mismatch");
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା index = 0;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା stride = 1;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(indices.size()) - 1; i >= 0; --i) {
            index += indices[i] * stride;
            stride *= dimSizes[i];
        }
        ଫେରନ୍ତୁ index;
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* build_Flatten_Index_ପଦସମଷ୍ଟି(File_ଧାଡ଼ି* flp, ଅଚଳ ArraySelInfo& info) {
        ଅଚଳ std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା> dimSizes = arraySelDimSizes(info);
        ତାର୍କିକ allConst = ସତ୍ୟ;
        ପାଇଁ (AstArraySel* ଅଚଳ selp : info.m_sels) {
            ଯଦି (!VN_IS(selp->bitp(), Const)) {
                allConst = ମିଥ୍ୟା;
                ଭାଙ୍ଗ;
            }
        }
        ଯଦି (allConst) {
            std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା> constIndices;
            constIndices.reserve(info.m_sels.size());
            ପାଇଁ (AstArraySel* ଅଚଳ selp : info.m_sels) {
                constIndices.ଠେଲ_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            ଫେରନ୍ତୁ makeConst32(flp, flattenIndex(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resultp = nullptr;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା stride = 1;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(info.m_sels.size()) - 1; i >= 0; --i) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* termp = info.m_sels[i]->bitp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            ଯଦି (termp->width() < 32) termp = ନୂତନ AstExtend{flp, termp, 32};
            ଯଦି (stride != 1) termp = ନୂତନ AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? ନୂତନ AstAdd{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        ଫେରନ୍ତୁ resultp;
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* build_Rhs_Data_ପଦସମଷ୍ଟି(File_ଧାଡ଼ି* flp, ଅଚଳ ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    ଶୂନ୍ୟ finalizeRhsVars() {
        ପାଇଁ (VarForceInfo& info : m_varInfos) {
            AstVar* ଅଚଳ varp = info.m_varp;
            ଯଦି (info.m_forces.empty()) ଜାରି;

            AstScope* ଅଚଳ scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_ଧାଡ଼ି* ଅଚଳ flp = varp->fileline();
            ଅଚଳ std::vector<ForceInfo*> forceps = forceInfosInIdOrder(info);

            ପାଇଁ (ForceInfo* ଅଚଳ finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* ଅଚଳ rhsVarp = ନୂତନ AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doingAssign() ? "_VassignRHS" : "__VforceRHS")
                        + std::to_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(ସତ୍ୟ);
                rhsVarp->sigPublic(ସତ୍ୟ);
                rhsVarp->setForcedByCode();
                varp->addNextHere(rhsVarp);
                finfo.m_rhsVarVscp = ନୂତନ AstVarScope{flp, scopep, rhsVarp};
                scopep->addVarsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                // end
                AstAssign* ଅଚଳ rhsAssignp = ନୂତନ AstAssign{
                    flp, ନୂତନ AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                ଯଦି (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* ଅଚଳ touchCallp = ନୂତନ AstCMethodHard{
                        flp, ନୂତନ AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtypeSetVoid();
                    Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->addNextHere(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* ଅଚଳ alwaysp
                    = ନୂତନ AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, rhsAssignp};
                Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep
                    = ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, ନୂତନ AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ଅଚଳ activep = ନୂତନ AstActive{flp, "force-rhs-update", senTreep};
                activep->sen_ବୃକ୍ଷ_Storep(activep->sentreep());
                activep->addStmtsp(alwaysp);
                scopep->addBlocksp(activep);
            }

            ଯଦି (info.m_forceRdVscp) {
                ଯଦି (!m_forceRdUpdateBuilt.ଭର୍ତ୍ତି(info.m_varVscp).second) ଜାରି;
                AstActive* ଅଚଳ activeInitp = ନୂତନ AstActive{
                    flp, "force-init",
                    ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, ନୂତନ AstSenItem{flp, AstSenItem::Static{}}}};
                activeInitp->sen_ବୃକ୍ଷ_Storep(activeInitp->sentreep());
                Ast_ଗଣ୍ଠି_Stmt* initStmtp = nullptr;
                ଯଦି (AstUnpackArrayDType* ଅଚଳ arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ଅଚଳ std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା innerWidth = dims.back()->subDTypep()->skipRefp()->width();
                    initStmtp = foreachUnpackedLeaf(
                        dims, [&](ଅଚଳ std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>& idx, ପୂର୍ଣ୍ଣସଂଖ୍ୟା /*flat*/) -> Ast_ଗଣ୍ଠି_Stmt* {
                            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = buildNestedArraySel(
                                flp, ନୂତନ AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            ଫେରନ୍ତୁ ନୂତନ AstAssign{flp, lhsp, makeZeroConst(varp, innerWidth)};
                        });
                } ଅନ୍ୟଥା {
                    initStmtp = ନୂତନ AstAssign{
                        flp, ନୂତନ AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->width())};
                }
                initStmtp->addNext(createForceRdUpdateStmt(info));
                activeInitp->addStmtsp(ନୂତନ AstInitial{flp, initStmtp});
                scopep->addBlocksp(activeInitp);

                AstSenItem* itemsp = nullptr;
                auto addSenItem = [&](AstVarScope* vscp) {
                    ଯଦି (!vscp) ଫେରନ୍ତୁ;
                    AstSenItem* ଅଚଳ nextp = ନୂତନ AstSenItem{
                        flp, VEdgeType::ET_CHANGED, ନୂତନ AstVarRef{flp, vscp, VAccess::READ}};
                    ଯଦି (itemsp) {
                        itemsp->addNext(nextp);
                    } ଅନ୍ୟଥା {
                        itemsp = nextp;
                    }
                };
                addSenItem(info.m_forceEnVscp);
                addSenItem(info.m_forceValVscp);
                AstVarRef* ଅଚଳ origSenRefp = ନୂତନ AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                AstSenItem* ଅଚଳ origItemp
                    = ନୂତନ AstSenItem{flp, VEdgeType::ET_CHANGED, origSenRefp};
                ଯଦି (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->addNext(origItemp);
                ପାଇଁ (ForceInfo* ଅଚଳ finfop : forceps) {
                    ଯଦି (!finfop->m_isExternal) addSenItem(finfop->m_rhsVarVscp);
                }

                AstActive* ଅଚଳ activep
                    = ନୂତନ AstActive{flp, "force-rd-update", ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, itemsp}};
                activep->sen_ବୃକ୍ଷ_Storep(activep->sentreep());
                activep->addStmtsp(ନୂତନ AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr,
                                                 createForceRdUpdateStmt(info)});
                scopep->addBlocksp(activep);
            }
        }
    }

    Ast_ଗଣ୍ଠି* createRhsUpdatesForWrite(File_ଧାଡ଼ି* flp, AstVar* writtenVarp) ଅଚଳ {
        ଯଦି (!doingAssign()) ଫେରନ୍ତୁ nullptr;

        ଅଚଳ auto it = m_rhsDepToForces.find(writtenVarp);
        ଯଦି (it == m_rhsDepToForces.end()) ଫେରନ୍ତୁ nullptr;

        Ast_ଗଣ୍ଠି* headp = nullptr;
        Ast_ଗଣ୍ଠି* tailp = nullptr;
        ପାଇଁ (ଅଚଳ ForceInfo* ଅଚଳ finfop : it->second) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            AstAssign* ଅଚଳ updatep
                = ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
            ଯଦି (tailp) {
                tailp->addNextHere(updatep);
            } ଅନ୍ୟଥା {
                headp = updatep;
            }
            tailp = updatep;
        }
        ଫେରନ୍ତୁ headp;
    }

    ଅଚଳ ForceInfo& getForceInfo(AstAssignForce* forceStmtp) ଅଚଳ {
        AstVarScope* ଅଚଳ vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        ଅଚଳ VarForceInfo* ଅଚଳ varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        auto it2 = varInfo->m_forces.find(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.end(), "Force statement not found");
        ଫେରନ୍ତୁ it2->second;
    }

    ସ୍ଥିର ତାର୍କିକ selOverlapsAnyForce(ଅଚଳ VarForceInfo& varInfo, ପୂର୍ଣ୍ଣସଂଖ୍ୟା selLsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା selMsb) {
        ପାଇଁ (ଅଚଳ auto& pair : varInfo.m_forces) {
            ଯଦି (pair.second.m_rangeLsb <= selMsb && pair.second.m_rangeMsb >= selLsb) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* createForceReadExpression(ଅଚଳ VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) ଅଚଳ {
        File_ଧାଡ଼ି* ଅଚଳ flp = originalRefp->fileline();
        AstVarRef* ଅଚଳ refp = originalRefp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        refp->access(VAccess::READ);
        ଫେରନ୍ତୁ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   nullptr);
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* createForceReadIndexExpression(ଅଚଳ VarForceInfo& varInfo,
                                                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* originalExprp,
                                                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* indexExprp) ଅଚଳ {
        File_ଧାଡ଼ି* ଅଚଳ flp = originalExprp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = originalExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        // Must be an LValue to a static variable
        VN_AS(exprp->getVAccessTargetRecurse(), VarRef)->access(VAccess::READ);
        ଫେରନ୍ତୁ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_INDEX, exprp, originalExprp,
                                   indexExprp);
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rebuildSelPath(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* pathp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* baseExprp) {
        ଯଦି (ଅଚଳ AstSel* ଅଚଳ selp = VN_CAST(pathp, Sel)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = rebuildSelPath(selp->fromp(), baseExprp);
            AstSel* ଅଚଳ outp
                = ନୂତନ AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->widthConst()};
            outp->dtypeFrom(selp);
            ଫେରନ୍ତୁ outp;
        }
        ଫେରନ୍ତୁ baseExprp;
    }
};

// Split deassign concat LHS before converting to release internals.
ସ୍ଥିର ଶୂନ୍ୟ splitDeassign(AstDeassign* nodep) {
    AstConcat* ଅଚଳ concatp = VN_CAST(nodep->lhsp(), Concat);
    ଯଦି (!concatp) ଫେରନ୍ତୁ;

    File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
    AstDeassign* ଅଚଳ newLp = ନୂତନ AstDeassign{flp, concatp->lhsp()->unlinkFrBack()};
    AstDeassign* ଅଚଳ newRp = ନୂତନ AstDeassign{flp, concatp->rhsp()->unlinkFrBack()};
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ conp = concatp->unlinkFrBack();
    nodep->replaceWith(newLp);
    newLp->addNextHere(newRp);
    VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    VL_DO_DANGLING(conp->delete_ବୃକ୍ଷ(), conp);

    splitDeassign(newLp);
    splitDeassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

ଶ୍ରେଣୀ ForceDiscoveryVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    ForceState& m_state;
    ତାର୍କିକ m_inClockedActive = ମିଥ୍ୟା;

    ଶୂନ୍ୟ buildForceableUnpackedArray(AstVarScope* ଅଚଳ nodep,
                                     AstUnpackArrayDType* ଅଚଳ arrDtypep) {
        AstVar* ଅଚଳ varp = nodep->varp();
        ଅଚଳ std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.empty(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ leafDtypep = dims.back()->subDTypep()->skipRefp();
        ଅଚଳ AstBasicDType* ଅଚଳ innerBasicp = leafDtypep->basicp();
        ଯଦି (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            ଫେରନ୍ତୁ;
        }

        File_ଧାଡ଼ି* ଅଚଳ flp = varp->fileline();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା innerWidth = leafDtypep->width();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* ଅଚଳ enVscp = info.m_forceEnVscp;
        AstVarScope* ଅଚଳ valVscp = info.m_forceValVscp;

        AstSenItem* ଅଚଳ itemsp = ନୂତନ AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                  ନୂତନ AstVarRef{flp, enVscp, VAccess::READ}};
        AstActive* ଅଚଳ activep = ନୂତନ AstActive{flp, "force-update", ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, itemsp}};
        activep->sen_ବୃକ୍ଷ_Storep(activep->sentreep());

        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ alwaysBodyHeadp = ForceState::foreachUnpackedLeaf(
            dims, [&](ଅଚଳ std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>& idx, ପୂର୍ଣ୍ଣସଂଖ୍ୟା flat) -> Ast_ଗଣ୍ଠି_Stmt* {
                AstVarRef* ଅଚଳ origRefp = ନୂତନ AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enSelp = ForceState::buildNestedArraySel(
                    flp, ନୂତନ AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valSelp = ForceState::buildNestedArraySel(
                    flp, ନୂତନ AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ forceExprp = ନୂତନ AstOr{
                    flp, ନୂତନ AstAnd{flp, enSelp, valSelp},
                    ନୂତନ AstAnd{flp, ନୂତନ AstNot{flp, enSelp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)}, origSelp}};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsSelp = ForceState::buildNestedArraySel(
                    flp, ନୂତନ AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                AstAssignForce* ଅଚଳ forceAssignp = ନୂତନ AstAssignForce{flp, lhsSelp, forceExprp};
                forceAssignp->user2(ସତ୍ୟ);

                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsClonep = forceExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                rhsClonep->foreach([varp](AstVarRef* ଅଚଳ r) {
                    ଯଦି (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/innerWidth - 1,
                                           /*hasArraySel=*/ସତ୍ୟ, /*skipRdRefresh=*/ସତ୍ୟ);
                ଫେରନ୍ତୁ forceAssignp;
            });
        activep->addStmtsp(ନୂତନ AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, alwaysBodyHeadp});
        nodep->scopep()->addBlocksp(activep);
    }

    ଶୂନ୍ୟ visit(AstAssignForce* nodep) override {
        ଯଦି (nodep->user2()) ଫେରନ୍ତୁ;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* ଅଚଳ lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* ଅଚଳ forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, ସତ୍ୟ);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsExprp = nodep->rhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    ଶୂନ୍ୟ visit(AstAssign* nodep) override {
        ଯଦି (m_state.doingAssign() && m_inClockedActive) {
            ଯଦି (AstVarRef* ଅଚଳ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(AstActive* nodep) override {
        VL_RESTORER(m_inClockedActive);
        m_inClockedActive = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(AstVarScope* nodep) override {
        ଯଦି (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            ଯଦି (m_state.doingAssign()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                ଫେରନ୍ତୁ;
            }

            ଯଦି (AstUnpackArrayDType* ଅଚଳ arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                ଫେରନ୍ତୁ;
            }

            ଅଚଳ AstBasicDType* ଅଚଳ bdtypep = nodep->varp()->basicp();
            ଯଦି (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* ଅଚଳ varp = nodep->varp();
            File_ଧାଡ଼ି* ଅଚଳ flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* ଅଚଳ enVscp = info.m_forceEnVscp;
            AstVarScope* ଅଚଳ valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            AstSenItem* ଅଚଳ itemsp = ନୂତନ AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                      ନୂତନ AstVarRef{flp, enVscp, VAccess::READ}};
            AstActive* ଅଚଳ activep
                = ନୂତନ AstActive{flp, "force-update", ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, itemsp}};
            activep->sen_ବୃକ୍ଷ_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* ଅଚଳ enRefp = ନୂତନ AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* ଅଚଳ valRefp = ନୂତନ AstVarRef{flp, valVscp, VAccess::READ};
            ଅଚଳ AstBasicDType* ଅଚଳ basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstOr{
                            flp, ନୂତନ AstAnd{flp, enRefp, valRefp},
                            ନୂତନ AstAnd{flp, ନୂତନ AstNot{flp, enRefp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)}, refp}})
                      : static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstCond{flp, enRefp, valRefp, refp});
            AstAssignForce* ଅଚଳ forceAssignp
                = ନୂତନ AstAssignForce{flp, ନୂତନ AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(ସତ୍ୟ);
            activep->addStmtsp(ନୂତନ AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, forceAssignp});
            nodep->scopep()->addBlocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsClonep = forceExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            rhsClonep->foreach([varp](AstVarRef* ଅଚଳ refp) {
                ଯଦି (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            ଅଚଳ ତାର୍କିକ bitwiseVar = ForceState::isBitwiseDType(varp);
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା padMsb = bitwiseVar ? (varp->width() - 1) : 0;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା rangeLsb = 0;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା rangeMsb = padMsb;
            ଯଦି (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, ମିଥ୍ୟା);
        }
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    explicit ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextConstNull(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

ଶ୍ରେଣୀ ForceConvertVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    ForceState& m_state;

    ଶୂନ୍ୟ visit(AstAssignForce* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp();
        AstVarRef* ଅଚଳ lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ଅଚଳ forcedVarp = lhsVarRefp->varp();

        ଅଚଳ ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        ଅଚଳ ForceState::VarForceInfo* ଅଚଳ varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        AstAssign* ଅଚଳ rhsAssignp
            = ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        ଅଚଳ ତାର୍କିକ bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        ଯଦି (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* baseExprp = nodep->rhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            baseExprp->foreach(
                [](AstVarRef* ଅଚଳ refp) { ForceState::markNonReplaceable(refp); });
            ଯଦି (bitwiseForcedVar) {
                baseExprp = ForceState::zeroPadToBaseWidth(baseExprp, forcedVarp->width(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            ଯଦି (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* ଅଚଳ maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valReadp
                    = ନୂତନ AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valWritep
                    = ନୂତନ AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notMaskp = ନୂତନ AstNot{flp, maskConstp};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ maskedOldp = ନୂତନ AstAnd{flp, valReadp, notMaskp};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newValp = ନୂତନ AstOr{flp, maskedOldp, baseExprp};
                valAssignp = ନୂତନ AstAssign{flp, valWritep, newValp};

                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enReadp
                    = ନୂତନ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enWritep
                    = ନୂତନ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newEnp
                    = ନୂତନ AstOr{flp, enReadp, maskConstp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                enAssignp = ନୂତନ AstAssign{flp, enWritep, newEnp};
            } ଅନ୍ୟଥା {
                AstConst* ଅଚଳ oneConstp = ForceState::makeRangeMaskConst(nodep, 1, 0, 0);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsValp = ForceState::cast_To_ଗଣ୍ଠି_DType(baseExprp, forcedVarp);
                valAssignp = ନୂତନ AstAssign{
                    flp, ନୂତନ AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = ନୂତନ AstAssign{
                    flp, ନୂତନ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        ଅଚଳ AstSel* ଅଚଳ selLhsp = VN_CAST(lhsp, Sel);
        ଅଚଳ ତାର୍କିକ arrayBitSel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->width();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsDatap = ForceState::build_Rhs_Data_ପଦସମଷ୍ଟି(flp, info);
        AstCExpr* ଅଚଳ rhsAddrp = ନୂତନ AstCExpr{flp};
        rhsAddrp->add("&(");
        rhsAddrp->add(rhsDatap);
        rhsAddrp->add(")");
        AstCMethodHard* ଅଚଳ addForceCallp = ନୂତନ AstCMethodHard{
            flp, ନୂତନ AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ADD,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        addForceCallp->addPinsp(rhsAddrp);
        addForceCallp->addPinsp(
            ForceState::makeConst32(flp, arrayBitSel ? info.m_padLsb : info.m_rangeLsb));
        ଯଦି (arrayBitSel) {
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padLsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padMsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, selLhsp->fromp()->width()));
        }
        addForceCallp->dtypeSetVoid();
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtp = addForceCallp->makeStmt();

        Ast_ଗଣ୍ଠି* tailp = rhsAssignp;
        ଯଦି (valAssignp) {
            tailp->addNextHere(valAssignp);
            tailp = valAssignp;
        }
        ଯଦି (enAssignp) {
            tailp->addNextHere(enAssignp);
            tailp = enAssignp;
        }
        tailp->addNextHere(stmtp);
        ଯଦି (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp();
        AstVarRef* ଅଚଳ lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ଅଚଳ releasedVarp = lhsVarRefp->varp();

        ଅଚଳ ForceState::VarForceInfo* ଅଚଳ varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        ଯଦି (!varInfo) {
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }
        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();

        ଅଚଳ ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, ମିଥ୍ୟା);

        ଅଚଳ AstSel* ଅଚଳ selLhsp = VN_CAST(lhsp, Sel);
        ଅଚଳ ତାର୍କିକ arrayBitSel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->width();
        AstCMethodHard* ଅଚଳ releaseCallp = ନୂତନ AstCMethodHard{
            flp, ନୂତନ AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        ଯଦି (arrayBitSel) {
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtypeSetVoid();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ releasep = releaseCallp->makeStmt();

        AstAssign* clearEnp = nullptr;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        ଯଦି (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enWritep
                = ନୂତନ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            ଯଦି (ForceState::isBitwiseDType(releasedVarp)) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା varWidth = releasedVarp->width();
                ଯଦି (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == varWidth - 1) {
                    clearEnp
                        = ନୂତନ AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, varWidth)};
                } ଅନ୍ୟଥା {
                    // forceEn = forceEn & ~mask(range);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enReadp
                        = ନୂତନ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* ଅଚଳ maskConst = ForceState::makeRangeMaskConst(
                        nodep, varWidth, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newEnp
                        = ନୂତନ AstAnd{flp, enReadp, ନୂତନ AstNot{flp, maskConst}};
                    clearEnp = ନୂତନ AstAssign{flp, enWritep, newEnp};
                }
            } ଅନ୍ୟଥା {
                clearEnp = ନୂତନ AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        ଅଚଳ AstSel* ଅଚଳ selp = VN_CAST(lhsp, Sel);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ basep = selp ? selp->fromp() : lhsp;

        Ast_ଗଣ୍ଠି* stmtListp = releasep;
        ଯଦି (clearEnp) {
            clearEnp->addNextHere(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        ଅଚଳ ତାର୍କିକ fullBitwiseRelease
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->width() - 1;
        ଯଦି (!releasedVarp->isContinuously()
            && !(m_state.doingAssign() && m_state.hasClockedWrite(releasedVarp)
                 && fullBitwiseRelease)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            ଅଚଳ ତାର୍କିକ hasOpaquePath = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* forceReadp = nullptr;
            ଯଦି (hasOpaquePath) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } ଅନ୍ୟଥା ଯଦି (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, basep,
                    ForceState::build_Flatten_Index_ପଦସମଷ୍ଟି(flp, rangeInfo.m_arrayInfo));
                ଯଦି (selp) { forceReadp = ForceState::rebuildSelPath(lhsp, forceReadp); }
            } ଅନ୍ୟଥା {
                forceReadp
                    = selp ? ForceState::rebuildSelPath(
                                 lhsp, m_state.createForceReadExpression(*varInfo, lhsVarRefp))
                           : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            }
            AstAssign* ଅଚଳ assignp = ନୂତନ AstAssign{flp, lhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), forceReadp};
            assignp->addNextHere(stmtListp);
            stmtListp = assignp;
        }

        ଯଦି (varInfo->m_forceRdVscp) stmtListp->addNext(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

ଶ୍ରେଣୀ ForceReplaceVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    ଅଚଳ ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_ଗଣ୍ଠି_Stmt* m_stmtp = nullptr;
    ତାର୍କିକ m_inLogic = ମିଥ୍ୟା;

    ଶୂନ୍ୟ iterateLogic(Ast_ଗଣ୍ଠି* nodep) {
        VL_RESTORER(m_inLogic);
        m_inLogic = ସତ୍ୟ;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstAssign* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        ଯଦି (AstVarRef* ଅଚଳ lhsp = VN_CAST(nodep->lhsp()->baseFromp(ସତ୍ୟ), VarRef)) {
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->addNextHere(updatep);
            }
        }
    }
    ଶୂନ୍ୟ visit(AstAssignCont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateAndNextNull(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    ଶୂନ୍ୟ visit(AstDeassign*) override {}
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override { iterateLogic(nodep); }
    ଶୂନ୍ୟ visit(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override { iterateLogic(nodep); }
    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        ଯଦି (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }
        iterateLogic(nodep);
    }
    ଶୂନ୍ୟ visit(AstSenItem* nodep) override { iterateLogic(nodep); }
    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* ଅଚଳ refp = VN_CAST(nodep->fromp(), VarRef);
        ଯଦି (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            visit(static_cast<Ast_ଗଣ୍ଠି*>(nodep));
            ଫେରନ୍ତୁ;
        }

        AstVar* ଅଚଳ varp = refp->varp();
        ଅଚଳ ForceState::VarForceInfo* ଅଚଳ varInfo = m_state.getVarInfo(refp->varScopep());
        ଯଦି (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.empty()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            visit(static_cast<Ast_ଗଣ୍ଠି*>(nodep));
            ଫେରନ୍ତୁ;
        }

        ଯଦି (ଅଚଳ AstConst* ଅଚଳ lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା selLsb = lsbConstp->toSInt();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା selMsb = selLsb + nodep->width() - 1;
            ଯଦି (!varp->isSigPublic()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                visit(static_cast<Ast_ଗଣ୍ଠି*>(nodep));
                ଫେରନ୍ତୁ;
            }
        }

        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* ଅଚଳ refClonep = refp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* ଅଚଳ callp = ନୂତନ AstCMethodHard{
            flp, ନୂତନ AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->width())};
        callp->addPinsp(refClonep);
        callp->addPinsp(nodep->lsbp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
        callp->addPinsp(ForceState::makeConst32(flp, nodep->width()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstArraySel* nodep) override {
        ଯଦି (ଅଚଳ AstArraySel* ଅଚଳ backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            ଯଦି (backSelp->fromp() == nodep) {
                iterateChildren(nodep);
                ଫେରନ୍ତୁ;
            }
        }

        Ast_ଗଣ୍ଠି* ଅଚଳ basep = nodep->baseFromp(ସତ୍ୟ);
        AstVarRef* ଅଚଳ baseRefp = VN_CAST(basep, VarRef);
        ଯଦି (!baseRefp) {
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }
        AstVar* ଅଚଳ varp = baseRefp->varp();
        ଅଚଳ ForceState::VarForceInfo* ଅଚଳ varInfo = m_state.getVarInfo(baseRefp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        ଯଦି (m_state.isNotReplaceable(baseRefp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }

        ଯଦି (!baseRefp->access().isReadOnly()) {
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (m_state.doingAssign() && varInfo->m_forceRdVscp) {
            baseRefp->varp(varInfo->m_forceRdVscp->varp());
            baseRefp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }
        ଅଚଳ ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        ପାଇଁ (AstArraySel* ଅଚଳ selp : arrayInfo.m_sels) iterateAndNextNull(selp->bitp());
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ indexExprp
            = ForceState::build_Flatten_Index_ପଦସମଷ୍ଟି(nodep->fileline(), arrayInfo);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ readExprp
            = m_state.createForceReadIndexExpression(*varInfo, nodep, indexExprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଯଦି (m_state.isNotReplaceable(nodep)) ଫେରନ୍ତୁ;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        ଯଦି (ଅଚଳ AstArraySel* ଅଚଳ backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            ଯଦି (backSelp->fromp() == nodep) ଫେରନ୍ତୁ;
        }

        ଅଚଳ ForceState::VarForceInfo* ଅଚଳ varInfo = m_state.getVarInfo(nodep->varScopep());
        ଯଦି (!varInfo) ଫେରନ୍ତୁ;

        ଯଦି (varInfo->m_forceRdVscp) {
            ଯଦି (nodep->access().isRW()) {
                ଯଦି (m_inLogic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                ଫେରନ୍ତୁ;
            }
            ଯଦି (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ readp
                    = m_state.createForceRdExpression(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ;
            }
            ଯଦି (m_inLogic && m_stmtp) {
                m_stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
            }
            ଫେରନ୍ତୁ;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        ଯଦି (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            ଫେରନ୍ତୁ;
        }

        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        ଯଦି (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } ଅନ୍ୟଥା ଯଦି (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (ForceState::isOutermostOpaquePathSelector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = VN_AS(nodep, ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
            Ast_ଗଣ୍ଠି* ଅଚଳ basep = exprp->baseFromp(ସତ୍ୟ);
            AstVarRef* ଅଚଳ baseRefp = VN_CAST(basep, VarRef);
            ଯଦି (baseRefp) {
                AstVar* ଅଚଳ varp = baseRefp->varp();
                ଯଦି (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    ଅଚଳ ForceState::VarForceInfo* ଅଚଳ varInfo
                        = m_state.getVarInfo(baseRefp->varScopep());
                    ଯଦି (!m_state.isNotReplaceable(baseRefp) && varInfo) {
                        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା forcePathIndex = varInfo->findForcePathIndex(exprp);
                        ଯଦି (forcePathIndex >= 0) {
                            ଯଦି (!baseRefp->access().isReadOnly()) ଫେରନ୍ତୁ;
                            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ readExprp = m_state.createForceReadIndexExpression(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), forcePathIndex));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                            ଫେରନ୍ତୁ;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

ସର୍ବସାଧାରଣ:
    explicit ForceReplaceVisitor(AstNetlist* nodep, ଅଚଳ ForceState& state)
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

ସ୍ଥିର ଶୂନ୍ୟ forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::ProtectedVarRefSet& permanentlyProtected,
                         ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    ଯଦି (!v3Global.hasForceableSignals()) ଫେରନ୍ତୁ;
    ForceState state{ମିଥ୍ୟା, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("force", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

ସ୍ଥିର ଶୂନ୍ୟ assignAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::ProtectedVarRefSet& permanentlyProtected,
                          ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    ଯଦି (!v3Global.hasAssignDeassign()) ଫେରନ୍ତୁ;

    std::vector<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.ଠେଲ_back(deassignp); });
    ପାଇଁ (AstDeassign* ଅଚଳ deassignp : deassignps) splitDeassign(deassignp);

    std::vector<AstAssignCont*> assignContps;
    deassignps.clear();
    nodep->foreach([&](Ast_ଗଣ୍ଠି_Stmt* nodep) {
        ଯଦି (AstAssignCont* ଅଚଳ assignContp = VN_CAST(nodep, AssignCont)) {
            assignContps.ଠେଲ_back(assignContp);
        } ଅନ୍ୟଥା ଯଦି (AstDeassign* ଅଚଳ deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.ଠେଲ_back(deassignp);
        }
    });

    ପାଇଁ (AstAssignCont* ଅଚଳ assignp : assignContps) {
        assignp->replaceWith(ନୂତନ AstAssignForce{assignp->fileline(),
                                                assignp->lhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
        VL_DO_DANGLING(assignp->delete_ବୃକ୍ଷ(), assignp);
    }
    ପାଇଁ (AstDeassign* ଅଚଳ deassignp : deassignps) {
        deassignp->replaceWith(
            ନୂତନ AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ)});
        VL_DO_DANGLING(deassignp->delete_ବୃକ୍ଷ(), deassignp);
    }
    ForceState state{ସତ୍ୟ, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("assign-deassign", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

ଶୂନ୍ୟ V3Force::forceAndAssignAll(AstNetlist* nodep) {
    ଅଚଳ VNUser3InUse user3InUse;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::ProtectedVarRefSet permanentlyProtected;
    ForceState::BuiltVarScopeSet forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    assignAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
