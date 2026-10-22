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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Force.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ ForceState final {
ಸಾರ್ವಜನಿಕ:
    ಸಂರಚನಾ ForceRange VL_NOT_FINAL {
        ಪೂರ್ಣಾಙ್ಕ m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        ಪೂರ್ಣಾಙ್ಕ m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        ಪೂರ್ಣಾಙ್ಕ m_padLsb = 0;  // Bit positions for RHS padding
        ಪೂರ್ಣಾಙ್ಕ m_padMsb = 0;  // Bit positions for RHS padding
    };

    ಸಂರಚನಾ ForceInfo final : ForceRange {
        // MEMBERS
        ಪೂರ್ಣಾಙ್ಕ m_forceId = 0;  // Unique (per signal) variable of this force assignment
        ತಾರ್ಕಿಕ m_hasArraySel = ಅಸತ್ಯ;  // If this has an array select on LHS
        ತಾರ್ಕಿಕ m_isExternal = ಅಸತ್ಯ;  // Synthetic force for the public force controls
        ತಾರ್ಕಿಕ m_skipRdRefresh = ಅಸತ್ಯ;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = nullptr;  // Scope of the var containing RHSID
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_rhsExprp = nullptr;  // Expression on RHS of this force assignment

        ForceInfo() = ಸ್ವಭಾವ;
        ForceInfo(ಪೂರ್ಣಾಙ್ಕ rangeLsb, ಪೂರ್ಣಾಙ್ಕ rangeMsb, ಪೂರ್ಣಾಙ್ಕ padLsb, ಪೂರ್ಣಾಙ್ಕ padMsb, ಪೂರ್ಣಾಙ್ಕ forceId,
                  ತಾರ್ಕಿಕ hasArraySel, AstVarScope* rhsVarVscp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsExprp)
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

    ಸಂರಚನಾ VarForceInfo final {
        AstVarScope* m_forceVecVscp = nullptr;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = nullptr;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = nullptr;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = nullptr;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = nullptr;  // Scope of the forced (original) variable
        AstVar* m_varp = nullptr;  // The forced (original) variable
        AstScope* m_scopep = nullptr;  // Scope containing the forced variable
        std::unordered_map<AstAssignForce*, ForceInfo> m_forces;
        std::unordered_map<string, ಪೂರ್ಣಾಙ್ಕ> m_forcePathToIndex;
        ಪೂರ್ಣಾಙ್ಕ m_nextForcePathIndex = 1;  // Start at 1 so 0 can be the base path (whole signal)

    ಗುಪ್ತ:
        ಸ್ಥಿರ ಶೂನ್ಯ build_Force_Path_ಕೀಲಿ_Recurse(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, string& out) {
            ಯದಿ (VN_IS(nodep, VarRef)) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (ಧ್ರುವ AstSel* ಧ್ರುವ selp = VN_CAST(nodep, Sel)) {
                build_Force_Path_ಕೀಲಿ_Recurse(selp->fromp(), out);
                out += "|S" + cvtToStr(selp->lsbConst()) + ":" + cvtToStr(selp->widthConst());
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (AstStructSel* ಧ್ರುವ selp = VN_CAST(nodep, StructSel)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = selp->fromp();
                build_Force_Path_ಕೀಲಿ_Recurse(fromp, out);
                out += "|T" + selp->name();
                ಪ್ರತ್ಯಾಗಮ;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        ಸ್ಥಿರ string force_Path_ಕೀಲಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
            string out;
            build_Force_Path_ಕೀಲಿ_Recurse(nodep, out);
            ಪ್ರತ್ಯಾಗಮ out;
        }

    ಸಾರ್ವಜನಿಕ:
        ಪೂರ್ಣಾಙ್ಕ getOrCreateForcePathIndex(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
            ಧ್ರುವ auto pair
                = m_forcePathToIndex.emplace(force_Path_ಕೀಲಿ(nodep), m_nextForcePathIndex);
            ಯದಿ (pair.second) ++m_nextForcePathIndex;
            ಪ್ರತ್ಯಾಗಮ pair.first->second;
        }

        ಪೂರ್ಣಾಙ್ಕ findForcePathIndex(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) ಧ್ರುವ {
            ಧ್ರುವ auto it = m_forcePathToIndex.find(force_Path_ಕೀಲಿ(nodep));
            ಪ್ರತ್ಯಾಗಮ it != m_forcePathToIndex.end() ? it->second : -1;
        }
    };

    ಸಂರಚನಾ ForceHelperVars final {
        AstVar* m_rdVarp = nullptr;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = nullptr;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = nullptr;  // The __VforceVal (force value) variable
    };

    ಸಂರಚನಾ ArraySelInfo final {
        std::vector<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        ತಾರ್ಕಿಕ m_hasBitSel = ಅಸತ್ಯ;  // Bit-select was found along the path too
    };

    ಸಂರಚನಾ ForceRangeInfo final : ForceRange {
        ತಾರ್ಕಿಕ m_hasArraySel = ಅಸತ್ಯ;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

ಗುಪ್ತ:
    using ScopeVarCache = std::unordered_map<ಧ್ರುವ AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    ಧ್ರುವ VNUser1InUse m_user1InUse;
    ಧ್ರುವ VNUser2InUse m_user2InUse;

ಸಾರ್ವಜನಿಕ:
    using ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    using ProtectedVarRefSet = std::unordered_set<ಧ್ರುವ AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    using BuiltVarScopeSet = std::unordered_set<ಧ್ರುವ AstVarScope*>;

ಗುಪ್ತ:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::vector<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, ಪೂರ್ಣಾಙ್ಕ> m_varToId;
    std::unordered_set<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::vector<ForceInfo*>> m_rhsDepToForces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    ತಾರ್ಕಿಕ m_doingAssign = ಅಸತ್ಯ;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    ProtectedVarRefSet& m_permanentlyProtected;
    BuiltVarScopeSet& m_forceRdUpdateBuilt;

ಸಾರ್ವಜನಿಕ:
    ForceState(ತಾರ್ಕಿಕ doingAssign, ForceHelperVarsByVar& forceHelperVarsByVar,
               ProtectedVarRefSet& permanentlyProtected, BuiltVarScopeSet& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doingAssign{doingAssign}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    ಸ್ಥಿರ AstConst* makeZeroConst(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಙ್ಕ width) {
        V3_ಸಂಖ್ಯೆ zero{nodep, width};
        zero.setAllBits0();
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{nodep->fileline(), zero};
    }

    ಸ್ಥಿರ AstConst* makeConst32(File_ಸಾಲು* flp, ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ) {
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, static_cast<uint32_t>(ಮೌಲ್ಯ)};
    }

    ಸ್ಥಿರ AstConst* makeRangeMaskConst(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಙ್ಕ width, ಪೂರ್ಣಾಙ್ಕ lsb, ಪೂರ್ಣಾಙ್ಕ msb) {
        V3_ಸಂಖ್ಯೆ mask{nodep, width};
        mask.setAllBits0();
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = lsb; bit <= msb; ++bit) mask.setBit(bit, 1);
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{nodep->fileline(), mask};
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* zeroPadToBaseWidth(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಪೂರ್ಣಾಙ್ಕ baseWidth, ಪೂರ್ಣಾಙ್ಕ padLsb,
                                           ಪೂರ್ಣಾಙ್ಕ padMsb) {
        ಯದಿ (baseWidth <= 0) ಪ್ರತ್ಯಾಗಮ exprp;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಕೆಳಗಿನ_Pad = padLsb;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಮೇಲಿನ_Pad = baseWidth - (padMsb + 1);
        ಯದಿ (ಕೆಳಗಿನ_Pad > 0) {
            exprp = ನೂತನ AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, ಕೆಳಗಿನ_Pad)};
        }
        ಯದಿ (ಮೇಲಿನ_Pad > 0) {
            exprp = ನೂತನ AstConcat{exprp->fileline(), makeZeroConst(exprp, ಮೇಲಿನ_Pad), exprp};
        }
        ಪ್ರತ್ಯಾಗಮ exprp;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isUnpackedArrayDType(ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) {
        ಪ್ರತ್ಯಾಗಮ VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isBitwiseDType(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = nodep->dtypep()->skipRefp()->basicp();
        ಪ್ರತ್ಯಾಗಮ basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* cast_To_ಗಂಟು_DType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು* dtypeFromp) {
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = dtypeFromp->dtypep()->skipRefp();
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = dtypep->basicp();
        ಯದಿ (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            ಪ್ರತ್ಯಾಗಮ exprp;
        }
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    ಸ್ಥಿರ ಶೂನ್ಯ markNonReplaceable(AstVarRef* ಧ್ರುವ nodep) { nodep->user1SetOnce(); }
    ತಾರ್ಕಿಕ isNotReplaceable(ಧ್ರುವ AstVarRef* ಧ್ರುವ nodep) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ nodep->user1() || m_permanentlyProtected.ಎಣಿಕೆ(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    ಶೂನ್ಯ markPermanentlyProtected(AstVarRef* ಧ್ರುವ nodep) ಧ್ರುವ {
        markNonReplaceable(nodep);
        m_permanentlyProtected.ಸೇರಿಸು(nodep);
    }

    ಸ್ಥಿರ std::vector<ForceInfo*> forceInfosInIdOrder(VarForceInfo& info) {
        std::vector<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        ಪ್ರತಿ (auto& it : info.m_forces) forceps.ತಳ್ಳು_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](ಧ್ರುವ ForceInfo* ap, ಧ್ರುವ ForceInfo* bp) {
            ಪ್ರತ್ಯಾಗಮ ap->m_forceId < bp->m_forceId;
        });
        ಪ್ರತ್ಯಾಗಮ forceps;
    }

    ಸ್ಥಿರ std::vector<ಧ್ರುವ ForceInfo*> forceInfosInIdOrder(ಧ್ರುವ VarForceInfo& info) {
        std::vector<ಧ್ರುವ ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        ಪ್ರತಿ (ಧ್ರುವ auto& it : info.m_forces) forceps.ತಳ್ಳು_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](ಧ್ರುವ ForceInfo* ap, ಧ್ರುವ ForceInfo* bp) {
            ಪ್ರತ್ಯಾಗಮ ap->m_forceId < bp->m_forceId;
        });
        ಪ್ರತ್ಯಾಗಮ forceps;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isOpaquePathSelector(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, Sel) || VN_IS(nodep, ಗಂಟು_Sel) || VN_IS(nodep, StructSel);
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isOutermostOpaquePathSelector(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಯದಿ (!isOpaquePathSelector(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ backp = nodep->backp();
        ಪ್ರತ್ಯಾಗಮ !backp || !isOpaquePathSelector(backp);
    }

    ಸ್ಥಿರ AstVarRef* getOneVarRef(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* forceStmtp) {
        Ast_ಗಂಟು* ಧ್ರುವ basep = forceStmtp->baseFromp(ಸತ್ಯ);
        ಯದಿ (AstSampled* sampledp = VN_CAST(basep, Sampled))
            ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = VN_CAST(sampledp->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ))
                ಪ್ರತ್ಯಾಗಮ getOneVarRef(exprp);
        AstVarRef* ಧ್ರುವ varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        ಪ್ರತ್ಯಾಗಮ varRefp;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* buildNestedArraySel(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp,
                                            ಧ್ರುವ std::vector<ಪೂರ್ಣಾಙ್ಕ>& indicies) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* curp = fromp;
        ಪ್ರತಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ idx : indicies) curp = ನೂತನ AstArraySel{flp, curp, idx};
        ಪ್ರತ್ಯಾಗಮ curp;
    }

    template <typename Fn>
    ಸ್ಥಿರ Ast_ಗಂಟು_Stmt* foreachUnpackedLeaf(ಧ್ರುವ std::vector<AstUnpackArrayDType*>& dims,
                                            Fn buildLeaf) {
        Ast_ಗಂಟು_Stmt* headp = nullptr;
        Ast_ಗಂಟು_Stmt* tailp = nullptr;
        ಯದಿ (dims.empty()) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪೂರ್ಣಾಙ್ಕ total = 1;
        ಪ್ರತಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ d : dims) total *= d->elementsConst();
        ಯದಿ (total <= 0) ಪ್ರತ್ಯಾಗಮ nullptr;
        std::vector<ಪೂರ್ಣಾಙ್ಕ> idx(dims.size(), 0);
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ flat = 0; flat < total; ++flat) {
            Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtp = buildLeaf(idx, flat);
            ಯದಿ (!headp) {
                headp = stmtp;
            } ಅನ್ಯಥಾ {
                tailp->addNext(stmtp);
            }
            tailp = stmtp;
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ d = static_cast<ಪೂರ್ಣಾಙ್ಕ>(dims.size()) - 1; d >= 0; --d) {
                ಯದಿ (++idx[d] < dims[d]->elementsConst()) ಭಙ್ಗ;
                idx[d] = 0;
            }
        }
        ಪ್ರತ್ಯಾಗಮ headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, AstVar* varp,
                                     ತಾರ್ಕಿಕ requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.empty();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->width() - 1) : 0;

        ಯದಿ (ಧ್ರುವ AstSel* ಧ್ರುವ outerSelp = VN_CAST(lhsp, Sel)) {
            ಪೂರ್ಣಾಙ್ಕ totalLsb = 0;
            ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* curp = lhsp; ಧ್ರುವ AstSel* ಧ್ರುವ selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                ಯದಿ (requireConstRangeSelect) {
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
        ಯದಿ (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::vector<ಪೂರ್ಣಾಙ್ಕ> indices;
            indices.reserve(info.m_arrayInfo.m_sels.size());
            ಪ್ರತಿ (AstArraySel* ಧ್ರುವ selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.ತಳ್ಳು_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            info.m_rangeLsb = flattenIndex(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } ಅನ್ಯಥಾ ಯದಿ (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        ಯದಿ (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ index = varInfo.getOrCreateForcePathIndex(lhsp);
            info.m_rangeLsb = index;
            info.m_rangeMsb = index;
        }
        ಪ್ರತ್ಯಾಗಮ info;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* add_Rhs_ಮೌಲ್ಯ_Reads(ಧ್ರುವ VarForceInfo& varInfo, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) ಧ್ರುವ {
        ಯದಿ (!doingAssign()) ಪ್ರತ್ಯಾಗಮ exprp;

        ಧ್ರುವ std::vector<ಧ್ರುವ ForceInfo*> forceps = forceInfosInIdOrder(varInfo);
        ಯದಿ (forceps.empty()) ಪ್ರತ್ಯಾಗಮ exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* ಧ್ರುವ cexprp = ನೂತನ AstCExpr{exprp->fileline(), AstCExpr::Pure{}};
        cexprp->dtypeFrom(exprp);
        cexprp->add("(");
        ಪ್ರತಿ (ಧ್ರುವ ForceInfo* ಧ್ರುವ finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* ಧ್ರುವ refp
                = ನೂತನ AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->add("(void)(");
            cexprp->add(refp);
            cexprp->add("), ");
        }
        cexprp->add(exprp);
        cexprp->add(")");
        ಪ್ರತ್ಯಾಗಮ cexprp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* createForceReadCall(ಧ್ರುವ VarForceInfo& varInfo, File_ಸಾಲು* flp, VCMethod method,
                                     Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* originalExprp, Ast_ಗಂಟು* dtypeFromp,
                                     Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* indexExprp) ಧ್ರುವ {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ origValp
            = add_Rhs_ಮೌಲ್ಯ_Reads(varInfo, cast_To_ಗಂಟು_DType(originalExprp, dtypeFromp));

        AstCMethodHard* ಧ್ರುವ callp = ನೂತನ AstCMethodHard{
            flp, ನೂತನ AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, method, origValp};
        ಯದಿ (indexExprp) callp->addPinsp(indexExprp);
        callp->dtypeFrom(dtypeFromp);
        ಪ್ರತ್ಯಾಗಮ callp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* createForceRdExpression(ಧ್ರುವ VarForceInfo& varInfo, File_ಸಾಲು* flp) ಧ್ರುವ {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* ಧ್ರುವ varp = varInfo.m_varVscp->varp();
        ಯದಿ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            ಯದಿ (doingAssign()) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, ಸತ್ಯ);
            AstVarRef* ಧ್ರುವ baseRefp = ನೂತನ AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(baseRefp);
            ಪ್ರತ್ಯಾಗಮ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, baseRefp, varp,
                                       nullptr);
        }
        AstVarRef* ಧ್ರುವ baseRefp = ನೂತನ AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(baseRefp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enRefp = ನೂತನ AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valRefp = ನೂತನ AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        ಯದಿ (isBitwiseDType(varp)) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstOr{
                flp, ನೂತನ AstAnd{flp, enRefp, valRefp},
                ನೂತನ AstAnd{flp, ನೂತನ AstNot{flp, enRefp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)}, baseRefp}};
        }
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCond{flp, enRefp, valRefp, baseRefp};
    }

    Ast_ಗಂಟು_Stmt* createForceRdUpdateStmt(ಧ್ರುವ VarForceInfo& varInfo) ಧ್ರುವ {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* ಧ್ರುವ varp = varInfo.m_varVscp->varp();
        ಯದಿ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            ಪ್ರತ್ಯಾಗಮ createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_ಸಾಲು* ಧ್ರುವ flp = varInfo.m_varVscp->fileline();
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             createForceRdExpression(varInfo, flp)};
    }

    Ast_ಗಂಟು_Stmt* createForceRdUpdateStmtUnpacked(ಧ್ರುವ VarForceInfo& varInfo) ಧ್ರುವ {
        File_ಸಾಲು* ಧ್ರುವ flp = varInfo.m_varVscp->fileline();
        AstVar* ಧ್ರುವ varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* ಧ್ರುವ arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        ಧ್ರುವ std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtsp = foreachUnpackedLeaf(
            dims, [&](ಧ್ರುವ std::vector<ಪೂರ್ಣಾಙ್ಕ>& idx, ಪೂರ್ಣಾಙ್ಕ /*flat*/) -> Ast_ಗಂಟು_Stmt* {
                AstVarRef* ಧ್ರುವ baseRefp = ನೂತನ AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(baseRefp);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ baseSelp = buildNestedArraySel(flp, baseRefp, idx);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enSelp = buildNestedArraySel(
                    flp, ನೂತನ AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valSelp = buildNestedArraySel(
                    flp, ನೂತನ AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ readExprp = ನೂತನ AstOr{
                    flp, ನೂತನ AstAnd{flp, enSelp, valSelp},
                    ನೂತನ AstAnd{flp, ನೂತನ AstNot{flp, enSelp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)}, baseSelp}};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rdLhsSelp = buildNestedArraySel(
                    flp, ನೂತನ AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        ಪ್ರತಿ (ಧ್ರುವ ForceInfo* ಧ್ರುವ finfop : forceInfosInIdOrder(varInfo)) {
            ಯದಿ (!finfop->m_isExternal) ಅನುವರ್ತ;
            stmtsp->addNext(ನೂತನ AstAssign{flp,
                                          ನೂತನ AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)});
        }
        ಪ್ರತ್ಯಾಗಮ stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        ಧ್ರುವ auto it = m_varToId.find(vscp);
        ಯದಿ (it != m_varToId.end()) ಪ್ರತ್ಯಾಗಮ m_varInfos[it->second];

        m_varToId.emplace(vscp, m_varInfos.size());
        m_varInfos.emplace_back();
        VarForceInfo& info = m_varInfos.back();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* ಧ್ರುವ varp = info.m_varp;
        ಯದಿ (!varp->isForceable()) ಪ್ರತ್ಯಾಗಮ info;

        File_ಸಾಲು* ಧ್ರುವ flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        ಧ್ರುವ ತಾರ್ಕಿಕ helperVarsBuilt = helperVars.m_rdVarp != nullptr;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != nullptr)
                        && helperVarsBuilt == (helperVars.m_valVarp != nullptr),
                    varp, "Incomplete force helper set");
        ಯದಿ (!helperVarsBuilt) {
            ಧ್ರುವ ತಾರ್ಕಿಕ unpacked = isUnpackedArrayDType(varp->dtypep());
            ಧ್ರುವ VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_ಗಂಟು_DType* ಧ್ರುವ enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
            helperVars.m_rdVarp
                = ನೂತನ AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sigPublic(ಸತ್ಯ);
            helperVars.m_enVarp
                = ನೂತನ AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(ಸತ್ಯ);
            helperVars.m_valVarp
                = ನೂತನ AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(ಸತ್ಯ);
            varp->addNextHere(helperVars.m_rdVarp);
            varp->addNextHere(helperVars.m_enVarp);
            varp->addNextHere(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = findScopeVar(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = findScopeVar(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = findScopeVar(info.m_scopep, helperVars.m_valVarp);
        ಯದಿ (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } ಅನ್ಯಥಾ {
            info.m_forceRdVscp = ನೂತನ AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = ನೂತನ AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = ನೂತನ AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->addVarsp(info.m_forceRdVscp);
            info.m_scopep->addVarsp(info.m_forceEnVscp);
            info.m_scopep->addVarsp(info.m_forceValVscp);
        }
        ಪ್ರತ್ಯಾಗಮ info;
    }

    ಶೂನ್ಯ markClockedWrite(AstVar* varp) { m_clockedWrites.ಸೇರಿಸು(varp); }
    ತಾರ್ಕಿಕ hasClockedWrite(AstVar* varp) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_clockedWrites.ಎಣಿಕೆ(varp); }

    ತಾರ್ಕಿಕ doingAssign() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_doingAssign; }

    ಧ್ರುವ VarForceInfo* getVarInfo(AstVarScope* vscp) ಧ್ರುವ {
        ಧ್ರುವ auto it = m_varToId.find(vscp);
        ಪ್ರತ್ಯಾಗಮ it != m_varToId.end() ? &m_varInfos[it->second] : nullptr;
    }

    AstVarScope* findScopeVar(AstScope* scopep, ಧ್ರುವ AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        ಯದಿ (cache.empty()) {
            ಪ್ರತಿ (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        ಧ್ರುವ auto it = cache.find(varp);
        ಪ್ರತ್ಯಾಗಮ it != cache.end() ? it->second : nullptr;
    }
    ಶೂನ್ಯ addForceAssignment(AstVar* varp, AstVarScope* vscp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsExprp,
                            AstAssignForce* forceStmtp, ಪೂರ್ಣಾಙ್ಕ rangeLsb, ಪೂರ್ಣಾಙ್ಕ rangeMsb, ಪೂರ್ಣಾಙ್ಕ padLsb,
                            ಪೂರ್ಣಾಙ್ಕ padMsb, ತಾರ್ಕಿಕ hasArraySel, ತಾರ್ಕಿಕ skipRdRefresh = ಅಸತ್ಯ) {
        v3Global.setUsesForce();
        varp->setForcedByCode();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ forceId = info.m_forces.size();
        File_ಸಾಲು* ಧ್ರುವ flp = varp->fileline();
        AstScope* ಧ್ರುವ scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        ಯದಿ (!info.m_forceVecVscp) {
            AstCDType* ಧ್ರುವ forceVecDtypep = ನೂತನ AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->addTypesp(forceVecDtypep);

            AstVar* ಧ್ರುವ forceVecVarp
                = ನೂತನ AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doingAssign ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(ಅಸತ್ಯ);
            forceVecVarp->isInternal(ಸತ್ಯ);
            varp->addNextHere(forceVecVarp);
            info.m_forceVecVscp = ನೂತನ AstVarScope{flp, scopep, forceVecVarp};
            scopep->addVarsp(info.m_forceVecVscp);
        }

        auto pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, nullptr, rhsExprp});
        ForceInfo& finfo = pair.first->second;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        ಯದಿ (doingAssign()) {
            std::vector<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* ಧ್ರುವ refp) {
                ಯದಿ (!refp->access().isReadOnly()) ಪ್ರತ್ಯಾಗಮ;
                AstVar* ಧ್ರುವ depVarp = refp->varp();
                ಯದಿ (depVarp
                    && std::find(depVarps.begin(), depVarps.end(), depVarp) == depVarps.end()) {
                    depVarps.ತಳ್ಳು_back(depVarp);
                }
            });
            ಪ್ರತಿ (AstVar* ಧ್ರುವ depVarp : depVarps) m_rhsDepToForces[depVarp].ತಳ್ಳು_back(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    ಸ್ಥಿರ ಶೂನ್ಯ collectArraySelInfo(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ArraySelInfo& info) {
        ಯದಿ (ಧ್ರುವ auto* ಧ್ರುವ selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = ಸತ್ಯ;
            collectArraySelInfo(selp->fromp(), info);
        } ಅನ್ಯಥಾ ಯದಿ (auto* ಧ್ರುವ selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.ತಳ್ಳು_back(selp);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ auto* ಧ್ರುವ memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    ಸ್ಥಿರ ArraySelInfo getArraySelInfo(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        ಪ್ರತ್ಯಾಗಮ info;
    }

    ಸ್ಥಿರ std::vector<ಪೂರ್ಣಾಙ್ಕ> arraySelDimSizes(ಧ್ರುವ ArraySelInfo& info) {
        std::vector<ಪೂರ್ಣಾಙ್ಕ> dims;
        dims.reserve(info.m_sels.size());
        ಪ್ರತಿ (AstArraySel* ಧ್ರುವ selp : info.m_sels) {
            Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = selp->fromp()->dtypep()->skipRefp();
            ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.ತಳ್ಳು_back(arrayp->declRange().elements());
        }
        ಪ್ರತ್ಯಾಗಮ dims;
    }

    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ flattenIndex(ಧ್ರುವ std::vector<ಪೂರ್ಣಾಙ್ಕ>& indices, ಧ್ರುವ std::vector<ಪೂರ್ಣಾಙ್ಕ>& dimSizes) {
        UASSERT(indices.size() == dimSizes.size(), "Array index and dimension size mismatch");
        ಪೂರ್ಣಾಙ್ಕ index = 0;
        ಪೂರ್ಣಾಙ್ಕ stride = 1;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = static_cast<ಪೂರ್ಣಾಙ್ಕ>(indices.size()) - 1; i >= 0; --i) {
            index += indices[i] * stride;
            stride *= dimSizes[i];
        }
        ಪ್ರತ್ಯಾಗಮ index;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* build_Flatten_Index_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* flp, ಧ್ರುವ ArraySelInfo& info) {
        ಧ್ರುವ std::vector<ಪೂರ್ಣಾಙ್ಕ> dimSizes = arraySelDimSizes(info);
        ತಾರ್ಕಿಕ allConst = ಸತ್ಯ;
        ಪ್ರತಿ (AstArraySel* ಧ್ರುವ selp : info.m_sels) {
            ಯದಿ (!VN_IS(selp->bitp(), Const)) {
                allConst = ಅಸತ್ಯ;
                ಭಙ್ಗ;
            }
        }
        ಯದಿ (allConst) {
            std::vector<ಪೂರ್ಣಾಙ್ಕ> constIndices;
            constIndices.reserve(info.m_sels.size());
            ಪ್ರತಿ (AstArraySel* ಧ್ರುವ selp : info.m_sels) {
                constIndices.ತಳ್ಳು_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            ಪ್ರತ್ಯಾಗಮ makeConst32(flp, flattenIndex(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = nullptr;
        ಪೂರ್ಣಾಙ್ಕ stride = 1;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = static_cast<ಪೂರ್ಣಾಙ್ಕ>(info.m_sels.size()) - 1; i >= 0; --i) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* termp = info.m_sels[i]->bitp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            ಯದಿ (termp->width() < 32) termp = ನೂತನ AstExtend{flp, termp, 32};
            ಯದಿ (stride != 1) termp = ನೂತನ AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? ನೂತನ AstAdd{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        ಪ್ರತ್ಯಾಗಮ resultp;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* build_Rhs_Data_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* flp, ಧ್ರುವ ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    ಶೂನ್ಯ finalizeRhsVars() {
        ಪ್ರತಿ (VarForceInfo& info : m_varInfos) {
            AstVar* ಧ್ರುವ varp = info.m_varp;
            ಯದಿ (info.m_forces.empty()) ಅನುವರ್ತ;

            AstScope* ಧ್ರುವ scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_ಸಾಲು* ಧ್ರುವ flp = varp->fileline();
            ಧ್ರುವ std::vector<ForceInfo*> forceps = forceInfosInIdOrder(info);

            ಪ್ರತಿ (ForceInfo* ಧ್ರುವ finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* ಧ್ರುವ rhsVarp = ನೂತನ AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doingAssign() ? "_VassignRHS" : "__VforceRHS")
                        + std::to_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(ಸತ್ಯ);
                rhsVarp->sigPublic(ಸತ್ಯ);
                rhsVarp->setForcedByCode();
                varp->addNextHere(rhsVarp);
                finfo.m_rhsVarVscp = ನೂತನ AstVarScope{flp, scopep, rhsVarp};
                scopep->addVarsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                // end
                AstAssign* ಧ್ರುವ rhsAssignp = ನೂತನ AstAssign{
                    flp, ನೂತನ AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                ಯದಿ (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* ಧ್ರುವ touchCallp = ನೂತನ AstCMethodHard{
                        flp, ನೂತನ AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtypeSetVoid();
                    Ast_ಗಂಟು_Stmt* ಧ್ರುವ touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->addNextHere(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* ಧ್ರುವ alwaysp
                    = ನೂತನ AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, rhsAssignp};
                Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep
                    = ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, ನೂತನ AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ಧ್ರುವ activep = ನೂತನ AstActive{flp, "force-rhs-update", senTreep};
                activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());
                activep->addStmtsp(alwaysp);
                scopep->addBlocksp(activep);
            }

            ಯದಿ (info.m_forceRdVscp) {
                ಯದಿ (!m_forceRdUpdateBuilt.ಸೇರಿಸು(info.m_varVscp).second) ಅನುವರ್ತ;
                AstActive* ಧ್ರುವ activeInitp = ನೂತನ AstActive{
                    flp, "force-init",
                    ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, ನೂತನ AstSenItem{flp, AstSenItem::Static{}}}};
                activeInitp->sen_ವೃಕ್ಷ_Storep(activeInitp->sentreep());
                Ast_ಗಂಟು_Stmt* initStmtp = nullptr;
                ಯದಿ (AstUnpackArrayDType* ಧ್ರುವ arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ಧ್ರುವ std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ innerWidth = dims.back()->subDTypep()->skipRefp()->width();
                    initStmtp = foreachUnpackedLeaf(
                        dims, [&](ಧ್ರುವ std::vector<ಪೂರ್ಣಾಙ್ಕ>& idx, ಪೂರ್ಣಾಙ್ಕ /*flat*/) -> Ast_ಗಂಟು_Stmt* {
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = buildNestedArraySel(
                                flp, ನೂತನ AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{flp, lhsp, makeZeroConst(varp, innerWidth)};
                        });
                } ಅನ್ಯಥಾ {
                    initStmtp = ನೂತನ AstAssign{
                        flp, ನೂತನ AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->width())};
                }
                initStmtp->addNext(createForceRdUpdateStmt(info));
                activeInitp->addStmtsp(ನೂತನ AstInitial{flp, initStmtp});
                scopep->addBlocksp(activeInitp);

                AstSenItem* itemsp = nullptr;
                auto addSenItem = [&](AstVarScope* vscp) {
                    ಯದಿ (!vscp) ಪ್ರತ್ಯಾಗಮ;
                    AstSenItem* ಧ್ರುವ nextp = ನೂತನ AstSenItem{
                        flp, VEdgeType::ET_CHANGED, ನೂತನ AstVarRef{flp, vscp, VAccess::READ}};
                    ಯದಿ (itemsp) {
                        itemsp->addNext(nextp);
                    } ಅನ್ಯಥಾ {
                        itemsp = nextp;
                    }
                };
                addSenItem(info.m_forceEnVscp);
                addSenItem(info.m_forceValVscp);
                AstVarRef* ಧ್ರುವ origSenRefp = ನೂತನ AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                AstSenItem* ಧ್ರುವ origItemp
                    = ನೂತನ AstSenItem{flp, VEdgeType::ET_CHANGED, origSenRefp};
                ಯದಿ (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->addNext(origItemp);
                ಪ್ರತಿ (ForceInfo* ಧ್ರುವ finfop : forceps) {
                    ಯದಿ (!finfop->m_isExternal) addSenItem(finfop->m_rhsVarVscp);
                }

                AstActive* ಧ್ರುವ activep
                    = ನೂತನ AstActive{flp, "force-rd-update", ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, itemsp}};
                activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());
                activep->addStmtsp(ನೂತನ AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr,
                                                 createForceRdUpdateStmt(info)});
                scopep->addBlocksp(activep);
            }
        }
    }

    Ast_ಗಂಟು* createRhsUpdatesForWrite(File_ಸಾಲು* flp, AstVar* writtenVarp) ಧ್ರುವ {
        ಯದಿ (!doingAssign()) ಪ್ರತ್ಯಾಗಮ nullptr;

        ಧ್ರುವ auto it = m_rhsDepToForces.find(writtenVarp);
        ಯದಿ (it == m_rhsDepToForces.end()) ಪ್ರತ್ಯಾಗಮ nullptr;

        Ast_ಗಂಟು* headp = nullptr;
        Ast_ಗಂಟು* tailp = nullptr;
        ಪ್ರತಿ (ಧ್ರುವ ForceInfo* ಧ್ರುವ finfop : it->second) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            AstAssign* ಧ್ರುವ updatep
                = ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
            ಯದಿ (tailp) {
                tailp->addNextHere(updatep);
            } ಅನ್ಯಥಾ {
                headp = updatep;
            }
            tailp = updatep;
        }
        ಪ್ರತ್ಯಾಗಮ headp;
    }

    ಧ್ರುವ ForceInfo& getForceInfo(AstAssignForce* forceStmtp) ಧ್ರುವ {
        AstVarScope* ಧ್ರುವ vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        ಧ್ರುವ VarForceInfo* ಧ್ರುವ varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        auto it2 = varInfo->m_forces.find(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.end(), "Force statement not found");
        ಪ್ರತ್ಯಾಗಮ it2->second;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ selOverlapsAnyForce(ಧ್ರುವ VarForceInfo& varInfo, ಪೂರ್ಣಾಙ್ಕ selLsb, ಪೂರ್ಣಾಙ್ಕ selMsb) {
        ಪ್ರತಿ (ಧ್ರುವ auto& pair : varInfo.m_forces) {
            ಯದಿ (pair.second.m_rangeLsb <= selMsb && pair.second.m_rangeMsb >= selLsb) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* createForceReadExpression(ಧ್ರುವ VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) ಧ್ರುವ {
        File_ಸಾಲು* ಧ್ರುವ flp = originalRefp->fileline();
        AstVarRef* ಧ್ರುವ refp = originalRefp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        refp->access(VAccess::READ);
        ಪ್ರತ್ಯಾಗಮ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   nullptr);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* createForceReadIndexExpression(ಧ್ರುವ VarForceInfo& varInfo,
                                                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* originalExprp,
                                                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* indexExprp) ಧ್ರುವ {
        File_ಸಾಲು* ಧ್ರುವ flp = originalExprp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = originalExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        // Must be an LValue to a static variable
        VN_AS(exprp->getVAccessTargetRecurse(), VarRef)->access(VAccess::READ);
        ಪ್ರತ್ಯಾಗಮ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_INDEX, exprp, originalExprp,
                                   indexExprp);
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rebuildSelPath(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pathp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* baseExprp) {
        ಯದಿ (ಧ್ರುವ AstSel* ಧ್ರುವ selp = VN_CAST(pathp, Sel)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = rebuildSelPath(selp->fromp(), baseExprp);
            AstSel* ಧ್ರುವ outp
                = ನೂತನ AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->widthConst()};
            outp->dtypeFrom(selp);
            ಪ್ರತ್ಯಾಗಮ outp;
        }
        ಪ್ರತ್ಯಾಗಮ baseExprp;
    }
};

// Split deassign concat LHS before converting to release internals.
ಸ್ಥಿರ ಶೂನ್ಯ splitDeassign(AstDeassign* nodep) {
    AstConcat* ಧ್ರುವ concatp = VN_CAST(nodep->lhsp(), Concat);
    ಯದಿ (!concatp) ಪ್ರತ್ಯಾಗಮ;

    File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
    AstDeassign* ಧ್ರುವ newLp = ನೂತನ AstDeassign{flp, concatp->lhsp()->unlinkFrBack()};
    AstDeassign* ಧ್ರುವ newRp = ನೂತನ AstDeassign{flp, concatp->rhsp()->unlinkFrBack()};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ conp = concatp->unlinkFrBack();
    nodep->replaceWith(newLp);
    newLp->addNextHere(newRp);
    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    VL_DO_DANGLING(conp->delete_ವೃಕ್ಷ(), conp);

    splitDeassign(newLp);
    splitDeassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

ವರ್ಗ ForceDiscoveryVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    ForceState& m_state;
    ತಾರ್ಕಿಕ m_inClockedActive = ಅಸತ್ಯ;

    ಶೂನ್ಯ buildForceableUnpackedArray(AstVarScope* ಧ್ರುವ nodep,
                                     AstUnpackArrayDType* ಧ್ರುವ arrDtypep) {
        AstVar* ಧ್ರುವ varp = nodep->varp();
        ಧ್ರುವ std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.empty(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ leafDtypep = dims.back()->subDTypep()->skipRefp();
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ innerBasicp = leafDtypep->basicp();
        ಯದಿ (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            ಪ್ರತ್ಯಾಗಮ;
        }

        File_ಸಾಲು* ಧ್ರುವ flp = varp->fileline();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ innerWidth = leafDtypep->width();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* ಧ್ರುವ enVscp = info.m_forceEnVscp;
        AstVarScope* ಧ್ರುವ valVscp = info.m_forceValVscp;

        AstSenItem* ಧ್ರುವ itemsp = ನೂತನ AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                  ನೂತನ AstVarRef{flp, enVscp, VAccess::READ}};
        AstActive* ಧ್ರುವ activep = ನೂತನ AstActive{flp, "force-update", ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, itemsp}};
        activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());

        Ast_ಗಂಟು_Stmt* ಧ್ರುವ alwaysBodyHeadp = ForceState::foreachUnpackedLeaf(
            dims, [&](ಧ್ರುವ std::vector<ಪೂರ್ಣಾಙ್ಕ>& idx, ಪೂರ್ಣಾಙ್ಕ flat) -> Ast_ಗಂಟು_Stmt* {
                AstVarRef* ಧ್ರುವ origRefp = ನೂತನ AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enSelp = ForceState::buildNestedArraySel(
                    flp, ನೂತನ AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valSelp = ForceState::buildNestedArraySel(
                    flp, ನೂತನ AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ forceExprp = ನೂತನ AstOr{
                    flp, ನೂತನ AstAnd{flp, enSelp, valSelp},
                    ನೂತನ AstAnd{flp, ನೂತನ AstNot{flp, enSelp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)}, origSelp}};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsSelp = ForceState::buildNestedArraySel(
                    flp, ನೂತನ AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                AstAssignForce* ಧ್ರುವ forceAssignp = ನೂತನ AstAssignForce{flp, lhsSelp, forceExprp};
                forceAssignp->user2(ಸತ್ಯ);

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsClonep = forceExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                rhsClonep->foreach([varp](AstVarRef* ಧ್ರುವ r) {
                    ಯದಿ (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/innerWidth - 1,
                                           /*hasArraySel=*/ಸತ್ಯ, /*skipRdRefresh=*/ಸತ್ಯ);
                ಪ್ರತ್ಯಾಗಮ forceAssignp;
            });
        activep->addStmtsp(ನೂತನ AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, alwaysBodyHeadp});
        nodep->scopep()->addBlocksp(activep);
    }

    ಶೂನ್ಯ visit(AstAssignForce* nodep) override {
        ಯದಿ (nodep->user2()) ಪ್ರತ್ಯಾಗಮ;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* ಧ್ರುವ lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* ಧ್ರುವ forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, ಸತ್ಯ);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsExprp = nodep->rhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    ಶೂನ್ಯ visit(AstAssign* nodep) override {
        ಯದಿ (m_state.doingAssign() && m_inClockedActive) {
            ಯದಿ (AstVarRef* ಧ್ರುವ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(AstActive* nodep) override {
        VL_RESTORER(m_inClockedActive);
        m_inClockedActive = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(AstVarScope* nodep) override {
        ಯದಿ (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            ಯದಿ (m_state.doingAssign()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }

            ಯದಿ (AstUnpackArrayDType* ಧ್ರುವ arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }

            ಧ್ರುವ AstBasicDType* ಧ್ರುವ bdtypep = nodep->varp()->basicp();
            ಯದಿ (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* ಧ್ರುವ varp = nodep->varp();
            File_ಸಾಲು* ಧ್ರುವ flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* ಧ್ರುವ enVscp = info.m_forceEnVscp;
            AstVarScope* ಧ್ರುವ valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            AstSenItem* ಧ್ರುವ itemsp = ನೂತನ AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                      ನೂತನ AstVarRef{flp, enVscp, VAccess::READ}};
            AstActive* ಧ್ರುವ activep
                = ನೂತನ AstActive{flp, "force-update", ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, itemsp}};
            activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* ಧ್ರುವ enRefp = ನೂತನ AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* ಧ್ರುವ valRefp = ನೂತನ AstVarRef{flp, valVscp, VAccess::READ};
            ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstOr{
                            flp, ನೂತನ AstAnd{flp, enRefp, valRefp},
                            ನೂತನ AstAnd{flp, ನೂತನ AstNot{flp, enRefp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)}, refp}})
                      : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstCond{flp, enRefp, valRefp, refp});
            AstAssignForce* ಧ್ರುವ forceAssignp
                = ನೂತನ AstAssignForce{flp, ನೂತನ AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(ಸತ್ಯ);
            activep->addStmtsp(ನೂತನ AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, forceAssignp});
            nodep->scopep()->addBlocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsClonep = forceExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            rhsClonep->foreach([varp](AstVarRef* ಧ್ರುವ refp) {
                ಯದಿ (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            ಧ್ರುವ ತಾರ್ಕಿಕ bitwiseVar = ForceState::isBitwiseDType(varp);
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ padMsb = bitwiseVar ? (varp->width() - 1) : 0;
            ಪೂರ್ಣಾಙ್ಕ rangeLsb = 0;
            ಪೂರ್ಣಾಙ್ಕ rangeMsb = padMsb;
            ಯದಿ (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, ಅಸತ್ಯ);
        }
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    explicit ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextConstNull(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

ವರ್ಗ ForceConvertVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    ForceState& m_state;

    ಶೂನ್ಯ visit(AstAssignForce* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp();
        AstVarRef* ಧ್ರುವ lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ಧ್ರುವ forcedVarp = lhsVarRefp->varp();

        ಧ್ರುವ ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        ಧ್ರುವ ForceState::VarForceInfo* ಧ್ರುವ varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        AstAssign* ಧ್ರುವ rhsAssignp
            = ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        ಧ್ರುವ ತಾರ್ಕಿಕ bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        ಯದಿ (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* baseExprp = nodep->rhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            baseExprp->foreach(
                [](AstVarRef* ಧ್ರುವ refp) { ForceState::markNonReplaceable(refp); });
            ಯದಿ (bitwiseForcedVar) {
                baseExprp = ForceState::zeroPadToBaseWidth(baseExprp, forcedVarp->width(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            ಯದಿ (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* ಧ್ರುವ maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valReadp
                    = ನೂತನ AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valWritep
                    = ನೂತನ AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notMaskp = ನೂತನ AstNot{flp, maskConstp};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ maskedOldp = ನೂತನ AstAnd{flp, valReadp, notMaskp};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newValp = ನೂತನ AstOr{flp, maskedOldp, baseExprp};
                valAssignp = ನೂತನ AstAssign{flp, valWritep, newValp};

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enReadp
                    = ನೂತನ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enWritep
                    = ನೂತನ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newEnp
                    = ನೂತನ AstOr{flp, enReadp, maskConstp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                enAssignp = ನೂತನ AstAssign{flp, enWritep, newEnp};
            } ಅನ್ಯಥಾ {
                AstConst* ಧ್ರುವ oneConstp = ForceState::makeRangeMaskConst(nodep, 1, 0, 0);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsValp = ForceState::cast_To_ಗಂಟು_DType(baseExprp, forcedVarp);
                valAssignp = ನೂತನ AstAssign{
                    flp, ನೂತನ AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = ನೂತನ AstAssign{
                    flp, ನೂತನ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        ಧ್ರುವ AstSel* ಧ್ರುವ selLhsp = VN_CAST(lhsp, Sel);
        ಧ್ರುವ ತಾರ್ಕಿಕ arrayBitSel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->width();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsDatap = ForceState::build_Rhs_Data_ಅಭಿವ್ಯಕ್ತಿ(flp, info);
        AstCExpr* ಧ್ರುವ rhsAddrp = ನೂತನ AstCExpr{flp};
        rhsAddrp->add("&(");
        rhsAddrp->add(rhsDatap);
        rhsAddrp->add(")");
        AstCMethodHard* ಧ್ರುವ addForceCallp = ನೂತನ AstCMethodHard{
            flp, ನೂತನ AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ADD,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        addForceCallp->addPinsp(rhsAddrp);
        addForceCallp->addPinsp(
            ForceState::makeConst32(flp, arrayBitSel ? info.m_padLsb : info.m_rangeLsb));
        ಯದಿ (arrayBitSel) {
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padLsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padMsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, selLhsp->fromp()->width()));
        }
        addForceCallp->dtypeSetVoid();
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtp = addForceCallp->makeStmt();

        Ast_ಗಂಟು* tailp = rhsAssignp;
        ಯದಿ (valAssignp) {
            tailp->addNextHere(valAssignp);
            tailp = valAssignp;
        }
        ಯದಿ (enAssignp) {
            tailp->addNextHere(enAssignp);
            tailp = enAssignp;
        }
        tailp->addNextHere(stmtp);
        ಯದಿ (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp();
        AstVarRef* ಧ್ರುವ lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ಧ್ರುವ releasedVarp = lhsVarRefp->varp();

        ಧ್ರುವ ForceState::VarForceInfo* ಧ್ರುವ varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        ಯದಿ (!varInfo) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();

        ಧ್ರುವ ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, ಅಸತ್ಯ);

        ಧ್ರುವ AstSel* ಧ್ರುವ selLhsp = VN_CAST(lhsp, Sel);
        ಧ್ರುವ ತಾರ್ಕಿಕ arrayBitSel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->width();
        AstCMethodHard* ಧ್ರುವ releaseCallp = ನೂತನ AstCMethodHard{
            flp, ನೂತನ AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        ಯದಿ (arrayBitSel) {
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtypeSetVoid();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ releasep = releaseCallp->makeStmt();

        AstAssign* clearEnp = nullptr;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        ಯದಿ (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enWritep
                = ನೂತನ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            ಯದಿ (ForceState::isBitwiseDType(releasedVarp)) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ varWidth = releasedVarp->width();
                ಯದಿ (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == varWidth - 1) {
                    clearEnp
                        = ನೂತನ AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, varWidth)};
                } ಅನ್ಯಥಾ {
                    // forceEn = forceEn & ~mask(range);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enReadp
                        = ನೂತನ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* ಧ್ರುವ maskConst = ForceState::makeRangeMaskConst(
                        nodep, varWidth, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newEnp
                        = ನೂತನ AstAnd{flp, enReadp, ನೂತನ AstNot{flp, maskConst}};
                    clearEnp = ನೂತನ AstAssign{flp, enWritep, newEnp};
                }
            } ಅನ್ಯಥಾ {
                clearEnp = ನೂತನ AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        ಧ್ರುವ AstSel* ಧ್ರುವ selp = VN_CAST(lhsp, Sel);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ basep = selp ? selp->fromp() : lhsp;

        Ast_ಗಂಟು* stmtListp = releasep;
        ಯದಿ (clearEnp) {
            clearEnp->addNextHere(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        ಧ್ರುವ ತಾರ್ಕಿಕ fullBitwiseRelease
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->width() - 1;
        ಯದಿ (!releasedVarp->isContinuously()
            && !(m_state.doingAssign() && m_state.hasClockedWrite(releasedVarp)
                 && fullBitwiseRelease)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            ಧ್ರುವ ತಾರ್ಕಿಕ hasOpaquePath = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* forceReadp = nullptr;
            ಯದಿ (hasOpaquePath) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } ಅನ್ಯಥಾ ಯದಿ (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, basep,
                    ForceState::build_Flatten_Index_ಅಭಿವ್ಯಕ್ತಿ(flp, rangeInfo.m_arrayInfo));
                ಯದಿ (selp) { forceReadp = ForceState::rebuildSelPath(lhsp, forceReadp); }
            } ಅನ್ಯಥಾ {
                forceReadp
                    = selp ? ForceState::rebuildSelPath(
                                 lhsp, m_state.createForceReadExpression(*varInfo, lhsVarRefp))
                           : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            }
            AstAssign* ಧ್ರುವ assignp = ನೂತನ AstAssign{flp, lhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), forceReadp};
            assignp->addNextHere(stmtListp);
            stmtListp = assignp;
        }

        ಯದಿ (varInfo->m_forceRdVscp) stmtListp->addNext(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

ವರ್ಗ ForceReplaceVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    ಧ್ರುವ ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_ಗಂಟು_Stmt* m_stmtp = nullptr;
    ತಾರ್ಕಿಕ m_inLogic = ಅಸತ್ಯ;

    ಶೂನ್ಯ iterateLogic(Ast_ಗಂಟು* nodep) {
        VL_RESTORER(m_inLogic);
        m_inLogic = ಸತ್ಯ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstAssign* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        ಯದಿ (AstVarRef* ಧ್ರುವ lhsp = VN_CAST(nodep->lhsp()->baseFromp(ಸತ್ಯ), VarRef)) {
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->addNextHere(updatep);
            }
        }
    }
    ಶೂನ್ಯ visit(AstAssignCont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateAndNextNull(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    ಶೂನ್ಯ visit(AstDeassign*) override {}
    ಶೂನ್ಯ visit(AstCFunc* nodep) override { iterateLogic(nodep); }
    ಶೂನ್ಯ visit(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override { iterateLogic(nodep); }
    ಶೂನ್ಯ visit(AstAlways* nodep) override {
        ಯದಿ (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        iterateLogic(nodep);
    }
    ಶೂನ್ಯ visit(AstSenItem* nodep) override { iterateLogic(nodep); }
    ಶೂನ್ಯ visit(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep->fromp(), VarRef);
        ಯದಿ (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            visit(static_cast<Ast_ಗಂಟು*>(nodep));
            ಪ್ರತ್ಯಾಗಮ;
        }

        AstVar* ಧ್ರುವ varp = refp->varp();
        ಧ್ರುವ ForceState::VarForceInfo* ಧ್ರುವ varInfo = m_state.getVarInfo(refp->varScopep());
        ಯದಿ (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.empty()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            visit(static_cast<Ast_ಗಂಟು*>(nodep));
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ selLsb = lsbConstp->toSInt();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ selMsb = selLsb + nodep->width() - 1;
            ಯದಿ (!varp->isSigPublic()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                visit(static_cast<Ast_ಗಂಟು*>(nodep));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* ಧ್ರುವ refClonep = refp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* ಧ್ರುವ callp = ನೂತನ AstCMethodHard{
            flp, ನೂತನ AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->width())};
        callp->addPinsp(refClonep);
        callp->addPinsp(nodep->lsbp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
        callp->addPinsp(ForceState::makeConst32(flp, nodep->width()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstArraySel* nodep) override {
        ಯದಿ (ಧ್ರುವ AstArraySel* ಧ್ರುವ backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            ಯದಿ (backSelp->fromp() == nodep) {
                iterateChildren(nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        Ast_ಗಂಟು* ಧ್ರುವ basep = nodep->baseFromp(ಸತ್ಯ);
        AstVarRef* ಧ್ರುವ baseRefp = VN_CAST(basep, VarRef);
        ಯದಿ (!baseRefp) {
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        AstVar* ಧ್ರುವ varp = baseRefp->varp();
        ಧ್ರುವ ForceState::VarForceInfo* ಧ್ರುವ varInfo = m_state.getVarInfo(baseRefp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        ಯದಿ (m_state.isNotReplaceable(baseRefp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (!baseRefp->access().isReadOnly()) {
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (m_state.doingAssign() && varInfo->m_forceRdVscp) {
            baseRefp->varp(varInfo->m_forceRdVscp->varp());
            baseRefp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಧ್ರುವ ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        ಪ್ರತಿ (AstArraySel* ಧ್ರುವ selp : arrayInfo.m_sels) iterateAndNextNull(selp->bitp());
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ indexExprp
            = ForceState::build_Flatten_Index_ಅಭಿವ್ಯಕ್ತಿ(nodep->fileline(), arrayInfo);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ readExprp
            = m_state.createForceReadIndexExpression(*varInfo, nodep, indexExprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಯದಿ (m_state.isNotReplaceable(nodep)) ಪ್ರತ್ಯಾಗಮ;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        ಯದಿ (ಧ್ರುವ AstArraySel* ಧ್ರುವ backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            ಯದಿ (backSelp->fromp() == nodep) ಪ್ರತ್ಯಾಗಮ;
        }

        ಧ್ರುವ ForceState::VarForceInfo* ಧ್ರುವ varInfo = m_state.getVarInfo(nodep->varScopep());
        ಯದಿ (!varInfo) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (varInfo->m_forceRdVscp) {
            ಯದಿ (nodep->access().isRW()) {
                ಯದಿ (m_inLogic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ readp
                    = m_state.createForceRdExpression(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (m_inLogic && m_stmtp) {
                m_stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        ಯದಿ (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            ಪ್ರತ್ಯಾಗಮ;
        }

        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        ಯದಿ (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (ForceState::isOutermostOpaquePathSelector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = VN_AS(nodep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            Ast_ಗಂಟು* ಧ್ರುವ basep = exprp->baseFromp(ಸತ್ಯ);
            AstVarRef* ಧ್ರುವ baseRefp = VN_CAST(basep, VarRef);
            ಯದಿ (baseRefp) {
                AstVar* ಧ್ರುವ varp = baseRefp->varp();
                ಯದಿ (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    ಧ್ರುವ ForceState::VarForceInfo* ಧ್ರುವ varInfo
                        = m_state.getVarInfo(baseRefp->varScopep());
                    ಯದಿ (!m_state.isNotReplaceable(baseRefp) && varInfo) {
                        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ forcePathIndex = varInfo->findForcePathIndex(exprp);
                        ಯದಿ (forcePathIndex >= 0) {
                            ಯದಿ (!baseRefp->access().isReadOnly()) ಪ್ರತ್ಯಾಗಮ;
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ readExprp = m_state.createForceReadIndexExpression(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), forcePathIndex));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                            ಪ್ರತ್ಯಾಗಮ;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    explicit ForceReplaceVisitor(AstNetlist* nodep, ಧ್ರುವ ForceState& state)
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

ಸ್ಥಿರ ಶೂನ್ಯ forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::ProtectedVarRefSet& permanentlyProtected,
                         ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    ಯದಿ (!v3Global.hasForceableSignals()) ಪ್ರತ್ಯಾಗಮ;
    ForceState state{ಅಸತ್ಯ, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("force", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಸ್ಥಿರ ಶೂನ್ಯ assignAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::ProtectedVarRefSet& permanentlyProtected,
                          ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    ಯದಿ (!v3Global.hasAssignDeassign()) ಪ್ರತ್ಯಾಗಮ;

    std::vector<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.ತಳ್ಳು_back(deassignp); });
    ಪ್ರತಿ (AstDeassign* ಧ್ರುವ deassignp : deassignps) splitDeassign(deassignp);

    std::vector<AstAssignCont*> assignContps;
    deassignps.clear();
    nodep->foreach([&](Ast_ಗಂಟು_Stmt* nodep) {
        ಯದಿ (AstAssignCont* ಧ್ರುವ assignContp = VN_CAST(nodep, AssignCont)) {
            assignContps.ತಳ್ಳು_back(assignContp);
        } ಅನ್ಯಥಾ ಯದಿ (AstDeassign* ಧ್ರುವ deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.ತಳ್ಳು_back(deassignp);
        }
    });

    ಪ್ರತಿ (AstAssignCont* ಧ್ರುವ assignp : assignContps) {
        assignp->replaceWith(ನೂತನ AstAssignForce{assignp->fileline(),
                                                assignp->lhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
        VL_DO_DANGLING(assignp->delete_ವೃಕ್ಷ(), assignp);
    }
    ಪ್ರತಿ (AstDeassign* ಧ್ರುವ deassignp : deassignps) {
        deassignp->replaceWith(
            ನೂತನ AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ)});
        VL_DO_DANGLING(deassignp->delete_ವೃಕ್ಷ(), deassignp);
    }
    ForceState state{ಸತ್ಯ, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("assign-deassign", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3Force::forceAndAssignAll(AstNetlist* nodep) {
    ಧ್ರುವ VNUser3InUse user3InUse;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::ProtectedVarRefSet permanentlyProtected;
    ForceState::BuiltVarScopeSet forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    assignAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
