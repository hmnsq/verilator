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
    ರಚನೆ ForceRange VL_NOT_FINAL {
        ಪೂರ್ಣಾಂಕ m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        ಪೂರ್ಣಾಂಕ m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        ಪೂರ್ಣಾಂಕ m_padLsb = 0;  // Bit positions for RHS padding
        ಪೂರ್ಣಾಂಕ m_padMsb = 0;  // Bit positions for RHS padding
    };

    ರಚನೆ ForceInfo final : ForceRange {
        // MEMBERS
        ಪೂರ್ಣಾಂಕ m_forceId = 0;  // Unique (per signal) variable of this force assignment
        ತಾರ್ಕಿಕ m_hasArraySel = ಸುಳ್ಳು;  // If this has an array select on LHS
        ತಾರ್ಕಿಕ m_isExternal = ಸುಳ್ಳು;  // Synthetic force for the public force controls
        ತಾರ್ಕಿಕ m_skipRdRefresh = ಸುಳ್ಳು;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = ಶೂನ್ಯಸೂಚಕ;  // Scope of the var containing RHSID
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_rhsExprp = ಶೂನ್ಯಸೂಚಕ;  // Expression on RHS of this force assignment

        ForceInfo() = ಪೂರ್ವನಿಯೋಜಿತ;
        ForceInfo(ಪೂರ್ಣಾಂಕ rangeLsb, ಪೂರ್ಣಾಂಕ rangeMsb, ಪೂರ್ಣಾಂಕ padLsb, ಪೂರ್ಣಾಂಕ padMsb, ಪೂರ್ಣಾಂಕ forceId,
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

    ರಚನೆ VarForceInfo final {
        AstVarScope* m_forceVecVscp = ಶೂನ್ಯಸೂಚಕ;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = ಶೂನ್ಯಸೂಚಕ;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = ಶೂನ್ಯಸೂಚಕ;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = ಶೂನ್ಯಸೂಚಕ;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = ಶೂನ್ಯಸೂಚಕ;  // Scope of the forced (original) variable
        AstVar* m_varp = ಶೂನ್ಯಸೂಚಕ;  // The forced (original) variable
        AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // Scope containing the forced variable
        std::unordered_map<Ast_ನಿಯೋಜಿಸು_Force*, ForceInfo> m_forces;
        std::unordered_map<string, ಪೂರ್ಣಾಂಕ> m_force_ಮಾರ್ಗ_ಗಮ್ಯ_ಸೂಚ್ಯಂಕ;
        ಪೂರ್ಣಾಂಕ m_ಮುಂದಿನ_Force_ಮಾರ್ಗ_ಸೂಚ್ಯಂಕ = 1;  // Start at 1 so 0 can be the base path (whole signal)

    ಖಾಸಗಿ:
        ಸ್ಥಿರ ಶೂನ್ಯ build_Force_ಮಾರ್ಗ_ಕೀಲಿ_Recurse(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, string& ಹೊರಹೋಗು) {
            ಆದರೆ (VN_IS(nodep, VarRef)) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, Sel)) {
                build_Force_ಮಾರ್ಗ_ಕೀಲಿ_Recurse(selp->fromp(), ಹೊರಹೋಗು);
                ಹೊರಹೋಗು += "|S" + cvt_ಗಮ್ಯ_Str(selp->lsbConst()) + ":" + cvt_ಗಮ್ಯ_Str(selp->ಅಗಲ_Const());
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (AstStructSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, StructSel)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = selp->fromp();
                build_Force_ಮಾರ್ಗ_ಕೀಲಿ_Recurse(fromp, ಹೊರಹೋಗು);
                ಹೊರಹೋಗು += "|T" + selp->name();
                ಹಿಂತಿರುಗಿ;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        ಸ್ಥಿರ string force_ಮಾರ್ಗ_ಕೀಲಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
            string ಹೊರಹೋಗು;
            build_Force_ಮಾರ್ಗ_ಕೀಲಿ_Recurse(nodep, ಹೊರಹೋಗು);
            ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
        }

    ಸಾರ್ವಜನಿಕ:
        ಪೂರ್ಣಾಂಕ get_Or_Create_Force_ಮಾರ್ಗ_ಸೂಚ್ಯಂಕ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair
                = m_force_ಮಾರ್ಗ_ಗಮ್ಯ_ಸೂಚ್ಯಂಕ.emplace(force_ಮಾರ್ಗ_ಕೀಲಿ(nodep), m_ಮುಂದಿನ_Force_ಮಾರ್ಗ_ಸೂಚ್ಯಂಕ);
            ಆದರೆ (pair.ಎರಡನೇ) ++m_ಮುಂದಿನ_Force_ಮಾರ್ಗ_ಸೂಚ್ಯಂಕ;
            ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
        }

        ಪೂರ್ಣಾಂಕ ಕಂಡುಹಿಡಿ_Force_ಮಾರ್ಗ_ಸೂಚ್ಯಂಕ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) ಸ್ಥಿರಾಂಕ {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_force_ಮಾರ್ಗ_ಗಮ್ಯ_ಸೂಚ್ಯಂಕ.ಕಂಡುಹಿಡಿ(force_ಮಾರ್ಗ_ಕೀಲಿ(nodep));
            ಹಿಂತಿರುಗಿ it != m_force_ಮಾರ್ಗ_ಗಮ್ಯ_ಸೂಚ್ಯಂಕ.ಕೊನೆ() ? it->ಎರಡನೇ : -1;
        }
    };

    ರಚನೆ ForceHelperVars final {
        AstVar* m_rdVarp = ಶೂನ್ಯಸೂಚಕ;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = ಶೂನ್ಯಸೂಚಕ;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = ಶೂನ್ಯಸೂಚಕ;  // The __VforceVal (force value) variable
    };

    ರಚನೆ ArraySelInfo final {
        std::ಸದಿಶ<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        ತಾರ್ಕಿಕ m_has_ಬಿಟ್_Sel = ಸುಳ್ಳು;  // Bit-select was found along the path too
    };

    ರಚನೆ ForceRangeInfo final : ForceRange {
        ತಾರ್ಕಿಕ m_hasArraySel = ಸುಳ್ಳು;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

ಖಾಸಗಿ:
    ಬಳಸಿ ScopeVarCache = std::unordered_map<ಸ್ಥಿರಾಂಕ AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_user2_ಒಳ_Use;

ಸಾರ್ವಜನಿಕ:
    ಬಳಸಿ ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    ಬಳಸಿ Protected_Var_Ref_ಗಣ = std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    ಬಳಸಿ Built_Var_Scope_ಗಣ = std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstVarScope*>;

ಖಾಸಗಿ:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::ಸದಿಶ<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, ಪೂರ್ಣಾಂಕ> m_var_ಗಮ್ಯ_Id;
    std::unordered_ಗಣ<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::ಸದಿಶ<ForceInfo*>> m_rhs_Dep_ಗಮ್ಯ_Forces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    ತಾರ್ಕಿಕ m_doing_ನಿಯೋಜಿಸು = ಸುಳ್ಳು;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    Protected_Var_Ref_ಗಣ& m_permanentlyProtected;
    Built_Var_Scope_ಗಣ& m_forceRdUpdateBuilt;

ಸಾರ್ವಜನಿಕ:
    ForceState(ತಾರ್ಕಿಕ doing_ನಿಯೋಜಿಸು, ForceHelperVarsByVar& forceHelperVarsByVar,
               Protected_Var_Ref_ಗಣ& permanentlyProtected, Built_Var_Scope_ಗಣ& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doing_ನಿಯೋಜಿಸು{doing_ನಿಯೋಜಿಸು}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    ಸ್ಥಿರ AstConst* makeZeroConst(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಂಕ ಅಗಲ) {
        V3_ಸಂಖ್ಯೆ zero{nodep, ಅಗಲ};
        zero.ಗಣ_All_Bits0();
        ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{nodep->fileline(), zero};
    }

    ಸ್ಥಿರ AstConst* makeConst32(File_ಸಾಲು* flp, ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ) {
        ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, static_cast<uint32_t>(ಮೌಲ್ಯ)};
    }

    ಸ್ಥಿರ AstConst* make_Range_ಮುಖವಾಡ_Const(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ lsb, ಪೂರ್ಣಾಂಕ msb) {
        V3_ಸಂಖ್ಯೆ ಮುಖವಾಡ{nodep, ಅಗಲ};
        ಮುಖವಾಡ.ಗಣ_All_Bits0();
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = lsb; ಬಿಟ್_2 <= msb; ++ಬಿಟ್_2) ಮುಖವಾಡ.ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 1);
        ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{nodep->fileline(), ಮುಖವಾಡ};
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* zero_Pad_ಗಮ್ಯ_ತಳ_ಅಗಲ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಪೂರ್ಣಾಂಕ ತಳ_ಅಗಲ, ಪೂರ್ಣಾಂಕ padLsb,
                                           ಪೂರ್ಣಾಂಕ padMsb) {
        ಆದರೆ (ತಳ_ಅಗಲ <= 0) ಹಿಂತಿರುಗಿ exprp;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಕೆಳಗಿನ_Pad = padLsb;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಮೇಲಿನ_Pad = ತಳ_ಅಗಲ - (padMsb + 1);
        ಆದರೆ (ಕೆಳಗಿನ_Pad > 0) {
            exprp = ಹೊಸ AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, ಕೆಳಗಿನ_Pad)};
        }
        ಆದರೆ (ಮೇಲಿನ_Pad > 0) {
            exprp = ಹೊಸ AstConcat{exprp->fileline(), makeZeroConst(exprp, ಮೇಲಿನ_Pad), exprp};
        }
        ಹಿಂತಿರುಗಿ exprp;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isUnpackedArrayDType(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) {
        ಹಿಂತಿರುಗಿ VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isBitwiseDType(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = nodep->dtypep()->skipRefp()->basicp();
        ಹಿಂತಿರುಗಿ basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* cast_ಗಮ್ಯ_ಗಂಟು_DType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು* dtypeFromp) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = dtypeFromp->dtypep()->skipRefp();
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = dtypep->basicp();
        ಆದರೆ (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            ಹಿಂತಿರುಗಿ exprp;
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    ಸ್ಥಿರ ಶೂನ್ಯ markNonReplaceable(AstVarRef* ಸ್ಥಿರಾಂಕ nodep) { nodep->user1_ಗಣ_Once(); }
    ತಾರ್ಕಿಕ isNotReplaceable(ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ nodep) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ nodep->user1() || m_permanentlyProtected.ಎಣಿಕೆ_2(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    ಶೂನ್ಯ markPermanentlyProtected(AstVarRef* ಸ್ಥಿರಾಂಕ nodep) ಸ್ಥಿರಾಂಕ {
        markNonReplaceable(nodep);
        m_permanentlyProtected.ಸೇರಿಸು(nodep);
    }

    ಸ್ಥಿರ std::ಸದಿಶ<ForceInfo*> force_Infos_ಒಳ_Id_ಕ್ರಮ(VarForceInfo& info) {
        std::ಸದಿಶ<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.ಗಾತ್ರ_2());
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& it : info.m_forces) forceps.ತಳ್ಳು_ಹಿಂಭಾಗ(&it.ಎರಡನೇ);
        std::ವಿಂಗಡಿಸು(forceps.begin(), forceps.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ ForceInfo* ap, ಸ್ಥಿರಾಂಕ ForceInfo* bp) {
            ಹಿಂತಿರುಗಿ ap->m_forceId < bp->m_forceId;
        });
        ಹಿಂತಿರುಗಿ forceps;
    }

    ಸ್ಥಿರ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ ForceInfo*> force_Infos_ಒಳ_Id_ಕ್ರಮ(ಸ್ಥಿರಾಂಕ VarForceInfo& info) {
        std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ ForceInfo*> forceps;
        forceps.reserve(info.m_forces.ಗಾತ್ರ_2());
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it : info.m_forces) forceps.ತಳ್ಳು_ಹಿಂಭಾಗ(&it.ಎರಡನೇ);
        std::ವಿಂಗಡಿಸು(forceps.begin(), forceps.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ ForceInfo* ap, ಸ್ಥಿರಾಂಕ ForceInfo* bp) {
            ಹಿಂತಿರುಗಿ ap->m_forceId < bp->m_forceId;
        });
        ಹಿಂತಿರುಗಿ forceps;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Opaque_ಮಾರ್ಗ_Selector(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಹಿಂತಿರುಗಿ VN_IS(nodep, Sel) || VN_IS(nodep, ಗಂಟು_Sel) || VN_IS(nodep, StructSel);
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Outermost_Opaque_ಮಾರ್ಗ_Selector(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಆದರೆ (!is_Opaque_ಮಾರ್ಗ_Selector(nodep)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ backp = nodep->backp();
        ಹಿಂತಿರುಗಿ !backp || !is_Opaque_ಮಾರ್ಗ_Selector(backp);
    }

    ಸ್ಥಿರ AstVarRef* getOneVarRef(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* forceStmtp) {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ basep = forceStmtp->ತಳ_Fromp(ನಿಜ);
        ಆದರೆ (AstSampled* sampledp = VN_CAST(basep, Sampled))
            ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = VN_CAST(sampledp->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ))
                ಹಿಂತಿರುಗಿ getOneVarRef(exprp);
        AstVarRef* ಸ್ಥಿರಾಂಕ varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        ಹಿಂತಿರುಗಿ varRefp;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* buildNestedArraySel(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp,
                                            ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ>& indicies) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* curp = fromp;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ idx : indicies) curp = ಹೊಸ AstArraySel{flp, curp, idx};
        ಹಿಂತಿರುಗಿ curp;
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು Fn>
    ಸ್ಥಿರ Ast_ಗಂಟು_Stmt* foreach_Unpacked_ಎಲೆ(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstUnpackArrayDType*>& dims,
                                            Fn build_ಎಲೆ) {
        Ast_ಗಂಟು_Stmt* headp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_Stmt* tailp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (dims.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಪೂರ್ಣಾಂಕ ಒಟ್ಟು = 1;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ d : dims) ಒಟ್ಟು *= d->elementsConst();
        ಆದರೆ (ಒಟ್ಟು <= 0) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ> idx(dims.ಗಾತ್ರ_2(), 0);
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ flat = 0; flat < ಒಟ್ಟು; ++flat) {
            Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtp = build_ಎಲೆ(idx, flat);
            ಆದರೆ (!headp) {
                headp = stmtp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                tailp->ಕೂಡಿಸು_ಮುಂದಿನ(stmtp);
            }
            tailp = stmtp;
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ d = static_cast<ಪೂರ್ಣಾಂಕ>(dims.ಗಾತ್ರ_2()) - 1; d >= 0; --d) {
                ಆದರೆ (++idx[d] < dims[d]->elementsConst()) ನಿಲ್ಲಿಸು;
                idx[d] = 0;
            }
        }
        ಹಿಂತಿರುಗಿ headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, AstVar* varp,
                                     ತಾರ್ಕಿಕ requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.ಖಾಲಿಗಳು();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->ಅಗಲ() - 1) : 0;

        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ outerSelp = VN_CAST(lhsp, Sel)) {
            ಪೂರ್ಣಾಂಕ ಒಟ್ಟು_Lsb = 0;
            ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* curp = lhsp; ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                ಆದರೆ (requireConstRangeSelect) {
                    UASSERT_OBJ(VN_IS(selp->lsbp(), Const), lhsp,
                                "Unsupported: force on non-const range select");
                }
                ಒಟ್ಟು_Lsb += selp->lsbConst();
            }
            info.m_padLsb = ಒಟ್ಟು_Lsb;
            info.m_padMsb = ಒಟ್ಟು_Lsb + outerSelp->ಅಗಲ_Const() - 1;
        }

        info.m_rangeLsb = info.m_padLsb;
        info.m_rangeMsb = info.m_padMsb;
        ಆದರೆ (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ> indices;
            indices.reserve(info.m_arrayInfo.m_sels.ಗಾತ್ರ_2());
            ಗಾಗಿ (AstArraySel* ಸ್ಥಿರಾಂಕ selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.ತಳ್ಳು_ಹಿಂಭಾಗ(VN_AS(selp->bitp(), Const)->ಗಮ್ಯ_SInt());
            }
            info.m_rangeLsb = flatten_ಸೂಚ್ಯಂಕ(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        ಆದರೆ (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಸೂಚ್ಯಂಕ = varInfo.get_Or_Create_Force_ಮಾರ್ಗ_ಸೂಚ್ಯಂಕ(lhsp);
            info.m_rangeLsb = ಸೂಚ್ಯಂಕ;
            info.m_rangeMsb = ಸೂಚ್ಯಂಕ;
        }
        ಹಿಂತಿರುಗಿ info;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಕೂಡಿಸು_Rhs_ಮೌಲ್ಯ_Reads(ಸ್ಥಿರಾಂಕ VarForceInfo& varInfo, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (!doing_ನಿಯೋಜಿಸು()) ಹಿಂತಿರುಗಿ exprp;

        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ ForceInfo*> forceps = force_Infos_ಒಳ_Id_ಕ್ರಮ(varInfo);
        ಆದರೆ (forceps.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* ಸ್ಥಿರಾಂಕ cexprp = ಹೊಸ AstCExpr{exprp->fileline(), AstCExpr::ಶುದ್ಧ{}};
        cexprp->dtypeFrom(exprp);
        cexprp->ಕೂಡಿಸು("(");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ForceInfo* ಸ್ಥಿರಾಂಕ finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* ಸ್ಥಿರಾಂಕ refp
                = ಹೊಸ AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->ಕೂಡಿಸು("(void)(");
            cexprp->ಕೂಡಿಸು(refp);
            cexprp->ಕೂಡಿಸು("), ");
        }
        cexprp->ಕೂಡಿಸು(exprp);
        cexprp->ಕೂಡಿಸು(")");
        ಹಿಂತಿರುಗಿ cexprp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* createForceReadCall(ಸ್ಥಿರಾಂಕ VarForceInfo& varInfo, File_ಸಾಲು* flp, VCMethod ವಿಧಾನ,
                                     Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* originalExprp, Ast_ಗಂಟು* dtypeFromp,
                                     Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸೂಚ್ಯಂಕ_Exprp) ಸ್ಥಿರಾಂಕ {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ origValp
            = ಕೂಡಿಸು_Rhs_ಮೌಲ್ಯ_Reads(varInfo, cast_ಗಮ್ಯ_ಗಂಟು_DType(originalExprp, dtypeFromp));

        AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCMethodHard{
            flp, ಹೊಸ AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, ವಿಧಾನ, origValp};
        ಆದರೆ (ಸೂಚ್ಯಂಕ_Exprp) callp->ಕೂಡಿಸು_Pinsp(ಸೂಚ್ಯಂಕ_Exprp);
        callp->dtypeFrom(dtypeFromp);
        ಹಿಂತಿರುಗಿ callp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* create_Force_Rd_ಅಭಿವ್ಯಕ್ತಿ(ಸ್ಥಿರಾಂಕ VarForceInfo& varInfo, File_ಸಾಲು* flp) ಸ್ಥಿರಾಂಕ {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* ಸ್ಥಿರಾಂಕ varp = varInfo.m_varVscp->varp();
        ಆದರೆ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            ಆದರೆ (doing_ನಿಯೋಜಿಸು()) { ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNOPTFLAT, ನಿಜ);
            AstVarRef* ಸ್ಥಿರಾಂಕ ತಳ_Refp = ಹೊಸ AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(ತಳ_Refp);
            ಹಿಂತಿರುಗಿ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, ತಳ_Refp, varp,
                                       ಶೂನ್ಯಸೂಚಕ);
        }
        AstVarRef* ಸ್ಥಿರಾಂಕ ತಳ_Refp = ಹೊಸ AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(ತಳ_Refp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enRefp = ಹೊಸ AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valRefp = ಹೊಸ AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        ಆದರೆ (isBitwiseDType(varp)) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstOr{
                flp, ಹೊಸ AstAnd{flp, enRefp, valRefp},
                ಹೊಸ AstAnd{flp, ಹೊಸ AstNot{flp, enRefp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)}, ತಳ_Refp}};
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ AstCond{flp, enRefp, valRefp, ತಳ_Refp};
    }

    Ast_ಗಂಟು_Stmt* createForceRdUpdateStmt(ಸ್ಥಿರಾಂಕ VarForceInfo& varInfo) ಸ್ಥಿರಾಂಕ {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* ಸ್ಥಿರಾಂಕ varp = varInfo.m_varVscp->varp();
        ಆದರೆ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            ಹಿಂತಿರುಗಿ createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = varInfo.m_varVscp->fileline();
        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             create_Force_Rd_ಅಭಿವ್ಯಕ್ತಿ(varInfo, flp)};
    }

    Ast_ಗಂಟು_Stmt* createForceRdUpdateStmtUnpacked(ಸ್ಥಿರಾಂಕ VarForceInfo& varInfo) ಸ್ಥಿರಾಂಕ {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = varInfo.m_varVscp->fileline();
        AstVar* ಸ್ಥಿರಾಂಕ varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtsp = foreach_Unpacked_ಎಲೆ(
            dims, [&](ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ>& idx, ಪೂರ್ಣಾಂಕ /*flat*/) -> Ast_ಗಂಟು_Stmt* {
                AstVarRef* ಸ್ಥಿರಾಂಕ ತಳ_Refp = ಹೊಸ AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(ತಳ_Refp);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ತಳ_Selp = buildNestedArraySel(flp, ತಳ_Refp, idx);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enSelp = buildNestedArraySel(
                    flp, ಹೊಸ AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valSelp = buildNestedArraySel(
                    flp, ಹೊಸ AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ readExprp = ಹೊಸ AstOr{
                    flp, ಹೊಸ AstAnd{flp, enSelp, valSelp},
                    ಹೊಸ AstAnd{flp, ಹೊಸ AstNot{flp, enSelp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)}, ತಳ_Selp}};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rdLhsSelp = buildNestedArraySel(
                    flp, ಹೊಸ AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ForceInfo* ಸ್ಥಿರಾಂಕ finfop : force_Infos_ಒಳ_Id_ಕ್ರಮ(varInfo)) {
            ಆದರೆ (!finfop->m_isExternal) ಮುಂದುವರಿ;
            stmtsp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp,
                                          ಹೊಸ AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)});
        }
        ಹಿಂತಿರುಗಿ stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_var_ಗಮ್ಯ_Id.ಕಂಡುಹಿಡಿ(vscp);
        ಆದರೆ (it != m_var_ಗಮ್ಯ_Id.ಕೊನೆ()) ಹಿಂತಿರುಗಿ m_varInfos[it->ಎರಡನೇ];

        m_var_ಗಮ್ಯ_Id.emplace(vscp, m_varInfos.ಗಾತ್ರ_2());
        m_varInfos.emplace_ಹಿಂಭಾಗ();
        VarForceInfo& info = m_varInfos.ಹಿಂಭಾಗ();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* ಸ್ಥಿರಾಂಕ varp = info.m_varp;
        ಆದರೆ (!varp->isForceable()) ಹಿಂತಿರುಗಿ info;

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ helperVarsBuilt = helperVars.m_rdVarp != ಶೂನ್ಯಸೂಚಕ;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != ಶೂನ್ಯಸೂಚಕ)
                        && helperVarsBuilt == (helperVars.m_valVarp != ಶೂನ್ಯಸೂಚಕ),
                    varp, "Incomplete force helper set");
        ಆದರೆ (!helperVarsBuilt) {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ unpacked = isUnpackedArrayDType(varp->dtypep());
            ಸ್ಥಿರಾಂಕ VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType();
            helperVars.m_rdVarp
                = ಹೊಸ AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sig_ಬಹಿರಂಗ(ನಿಜ);
            helperVars.m_enVarp
                = ಹೊಸ AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(ನಿಜ);
            helperVars.m_valVarp
                = ಹೊಸ AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(ನಿಜ);
            varp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(helperVars.m_rdVarp);
            varp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(helperVars.m_enVarp);
            varp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = ಕಂಡುಹಿಡಿ_Scope_Var(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = ಕಂಡುಹಿಡಿ_Scope_Var(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = ಕಂಡುಹಿಡಿ_Scope_Var(info.m_scopep, helperVars.m_valVarp);
        ಆದರೆ (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } ಇಲ್ಲದಿದ್ದರೆ {
            info.m_forceRdVscp = ಹೊಸ AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = ಹೊಸ AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = ಹೊಸ AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->ಕೂಡಿಸು_Varsp(info.m_forceRdVscp);
            info.m_scopep->ಕೂಡಿಸು_Varsp(info.m_forceEnVscp);
            info.m_scopep->ಕೂಡಿಸು_Varsp(info.m_forceValVscp);
        }
        ಹಿಂತಿರುಗಿ info;
    }

    ಶೂನ್ಯ markClockedWrite(AstVar* varp) { m_clockedWrites.ಸೇರಿಸು(varp); }
    ತಾರ್ಕಿಕ hasClockedWrite(AstVar* varp) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_clockedWrites.ಎಣಿಕೆ_2(varp); }

    ತಾರ್ಕಿಕ doing_ನಿಯೋಜಿಸು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_doing_ನಿಯೋಜಿಸು; }

    ಸ್ಥಿರಾಂಕ VarForceInfo* getVarInfo(AstVarScope* vscp) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_var_ಗಮ್ಯ_Id.ಕಂಡುಹಿಡಿ(vscp);
        ಹಿಂತಿರುಗಿ it != m_var_ಗಮ್ಯ_Id.ಕೊನೆ() ? &m_varInfos[it->ಎರಡನೇ] : ಶೂನ್ಯಸೂಚಕ;
    }

    AstVarScope* ಕಂಡುಹಿಡಿ_Scope_Var(AstScope* scopep, ಸ್ಥಿರಾಂಕ AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        ಆದರೆ (cache.ಖಾಲಿಗಳು()) {
            ಗಾಗಿ (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = cache.ಕಂಡುಹಿಡಿ(varp);
        ಹಿಂತಿರುಗಿ it != cache.ಕೊನೆ() ? it->ಎರಡನೇ : ಶೂನ್ಯಸೂಚಕ;
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_Force_Assignment(AstVar* varp, AstVarScope* vscp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsExprp,
                            Ast_ನಿಯೋಜಿಸು_Force* forceStmtp, ಪೂರ್ಣಾಂಕ rangeLsb, ಪೂರ್ಣಾಂಕ rangeMsb, ಪೂರ್ಣಾಂಕ padLsb,
                            ಪೂರ್ಣಾಂಕ padMsb, ತಾರ್ಕಿಕ hasArraySel, ತಾರ್ಕಿಕ skipRdRefresh = ಸುಳ್ಳು) {
        v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Force();
        varp->ಗಣ_Forced_By_ಸಂಕೇತ();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ forceId = info.m_forces.ಗಾತ್ರ_2();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = varp->fileline();
        AstScope* ಸ್ಥಿರಾಂಕ scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        ಆದರೆ (!info.m_forceVecVscp) {
            AstCDType* ಸ್ಥಿರಾಂಕ forceVecDtypep = ಹೊಸ AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(forceVecDtypep);

            AstVar* ಸ್ಥಿರಾಂಕ forceVecVarp
                = ಹೊಸ AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doing_ನಿಯೋಜಿಸು ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(ಸುಳ್ಳು);
            forceVecVarp->is_ಆಂತರಿಕ(ನಿಜ);
            varp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(forceVecVarp);
            info.m_forceVecVscp = ಹೊಸ AstVarScope{flp, scopep, forceVecVarp};
            scopep->ಕೂಡಿಸು_Varsp(info.m_forceVecVscp);
        }

        ಸ್ವಯಂಚಾಲಿತ pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, ಶೂನ್ಯಸೂಚಕ, rhsExprp});
        ForceInfo& finfo = pair.ಮೊದಲ->ಎರಡನೇ;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        ಆದರೆ (doing_ನಿಯೋಜಿಸು()) {
            std::ಸದಿಶ<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* ಸ್ಥಿರಾಂಕ refp) {
                ಆದರೆ (!refp->access().isReadOnly()) ಹಿಂತಿರುಗಿ;
                AstVar* ಸ್ಥಿರಾಂಕ depVarp = refp->varp();
                ಆದರೆ (depVarp
                    && std::ಕಂಡುಹಿಡಿ(depVarps.begin(), depVarps.ಕೊನೆ(), depVarp) == depVarps.ಕೊನೆ()) {
                    depVarps.ತಳ್ಳು_ಹಿಂಭಾಗ(depVarp);
                }
            });
            ಗಾಗಿ (AstVar* ಸ್ಥಿರಾಂಕ depVarp : depVarps) m_rhs_Dep_ಗಮ್ಯ_Forces[depVarp].ತಳ್ಳು_ಹಿಂಭಾಗ(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    ಸ್ಥಿರ ಶೂನ್ಯ collectArraySelInfo(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ArraySelInfo& info) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ selp = VN_CAST(exprp, Sel)) {
            info.m_has_ಬಿಟ್_Sel = ನಿಜ;
            collectArraySelInfo(selp->fromp(), info);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.ತಳ್ಳು_ಹಿಂಭಾಗ(selp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    ಸ್ಥಿರ ArraySelInfo getArraySelInfo(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        ಹಿಂತಿರುಗಿ info;
    }

    ಸ್ಥಿರ std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ> arraySelDimSizes(ಸ್ಥಿರಾಂಕ ArraySelInfo& info) {
        std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ> dims;
        dims.reserve(info.m_sels.ಗಾತ್ರ_2());
        ಗಾಗಿ (AstArraySel* ಸ್ಥಿರಾಂಕ selp : info.m_sels) {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = selp->fromp()->dtypep()->skipRefp();
            ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.ತಳ್ಳು_ಹಿಂಭಾಗ(arrayp->declRange().elements());
        }
        ಹಿಂತಿರುಗಿ dims;
    }

    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ flatten_ಸೂಚ್ಯಂಕ(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ>& indices, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ>& dimSizes) {
        UASSERT(indices.ಗಾತ್ರ_2() == dimSizes.ಗಾತ್ರ_2(), "Array index and dimension size mismatch");
        ಪೂರ್ಣಾಂಕ ಸೂಚ್ಯಂಕ = 0;
        ಪೂರ್ಣಾಂಕ stride = 1;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = static_cast<ಪೂರ್ಣಾಂಕ>(indices.ಗಾತ್ರ_2()) - 1; i >= 0; --i) {
            ಸೂಚ್ಯಂಕ += indices[i] * stride;
            stride *= dimSizes[i];
        }
        ಹಿಂತಿರುಗಿ ಸೂಚ್ಯಂಕ;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* build_Flatten_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ ArraySelInfo& info) {
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ> dimSizes = arraySelDimSizes(info);
        ತಾರ್ಕಿಕ allConst = ನಿಜ;
        ಗಾಗಿ (AstArraySel* ಸ್ಥಿರಾಂಕ selp : info.m_sels) {
            ಆದರೆ (!VN_IS(selp->bitp(), Const)) {
                allConst = ಸುಳ್ಳು;
                ನಿಲ್ಲಿಸು;
            }
        }
        ಆದರೆ (allConst) {
            std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ> constIndices;
            constIndices.reserve(info.m_sels.ಗಾತ್ರ_2());
            ಗಾಗಿ (AstArraySel* ಸ್ಥಿರಾಂಕ selp : info.m_sels) {
                constIndices.ತಳ್ಳು_ಹಿಂಭಾಗ(VN_AS(selp->bitp(), Const)->ಗಮ್ಯ_SInt());
            }
            ಹಿಂತಿರುಗಿ makeConst32(flp, flatten_ಸೂಚ್ಯಂಕ(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = ಶೂನ್ಯಸೂಚಕ;
        ಪೂರ್ಣಾಂಕ stride = 1;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = static_cast<ಪೂರ್ಣಾಂಕ>(info.m_sels.ಗಾತ್ರ_2()) - 1; i >= 0; --i) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* termp = info.m_sels[i]->bitp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            ಆದರೆ (termp->ಅಗಲ() < 32) termp = ಹೊಸ AstExtend{flp, termp, 32};
            ಆದರೆ (stride != 1) termp = ಹೊಸ AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? ಹೊಸ Ast_ಕೂಡಿಸು{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        ಹಿಂತಿರುಗಿ resultp;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* build_Rhs_ದತ್ತಾಂಶ_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    ಶೂನ್ಯ finalizeRhsVars() {
        ಗಾಗಿ (VarForceInfo& info : m_varInfos) {
            AstVar* ಸ್ಥಿರಾಂಕ varp = info.m_varp;
            ಆದರೆ (info.m_forces.ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;

            AstScope* ಸ್ಥಿರಾಂಕ scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = varp->fileline();
            ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ForceInfo*> forceps = force_Infos_ಒಳ_Id_ಕ್ರಮ(info);

            ಗಾಗಿ (ForceInfo* ಸ್ಥಿರಾಂಕ finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* ಸ್ಥಿರಾಂಕ rhsVarp = ಹೊಸ AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doing_ನಿಯೋಜಿಸು() ? "_VassignRHS" : "__VforceRHS")
                        + std::ಗಮ್ಯ_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(ನಿಜ);
                rhsVarp->sig_ಬಹಿರಂಗ(ನಿಜ);
                rhsVarp->ಗಣ_Forced_By_ಸಂಕೇತ();
                varp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(rhsVarp);
                finfo.m_rhsVarVscp = ಹೊಸ AstVarScope{flp, scopep, rhsVarp};
                scopep->ಕೂಡಿಸು_Varsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                //ಕೊನೆ
                Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ rhsAssignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                    flp, ಹೊಸ AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                ಆದರೆ (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* ಸ್ಥಿರಾಂಕ touchCallp = ಹೊಸ AstCMethodHard{
                        flp, ಹೊಸ AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtype_ಗಣ_Void();
                    Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* ಸ್ಥಿರಾಂಕ alwaysp
                    = ಹೊಸ AstAlways{flp, VAlwaysKwd::ALWAYS, ಶೂನ್ಯಸೂಚಕ, rhsAssignp};
                Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep
                    = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, ಹೊಸ Ast_Sen_ಅಂಶ{flp, Ast_Sen_ಅಂಶ::Combo{}}};
                Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "force-rhs-update", senTreep};
                activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());
                activep->ಕೂಡಿಸು_Stmtsp(alwaysp);
                scopep->ಕೂಡಿಸು_Blocksp(activep);
            }

            ಆದರೆ (info.m_forceRdVscp) {
                ಆದರೆ (!m_forceRdUpdateBuilt.ಸೇರಿಸು(info.m_varVscp).ಎರಡನೇ) ಮುಂದುವರಿ;
                Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ ಸಕ್ರಿಯ_Initp = ಹೊಸ Ast_ಸಕ್ರಿಯ{
                    flp, "force-init",
                    ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, ಹೊಸ Ast_Sen_ಅಂಶ{flp, Ast_Sen_ಅಂಶ::Static{}}}};
                ಸಕ್ರಿಯ_Initp->sen_ವೃಕ್ಷ_Storep(ಸಕ್ರಿಯ_Initp->sentreep());
                Ast_ಗಂಟು_Stmt* initStmtp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ inner_ಅಗಲ = dims.ಹಿಂಭಾಗ()->subDTypep()->skipRefp()->ಅಗಲ();
                    initStmtp = foreach_Unpacked_ಎಲೆ(
                        dims, [&](ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ>& idx, ಪೂರ್ಣಾಂಕ /*flat*/) -> Ast_ಗಂಟು_Stmt* {
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = buildNestedArraySel(
                                flp, ಹೊಸ AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsp, makeZeroConst(varp, inner_ಅಗಲ)};
                        });
                } ಇಲ್ಲದಿದ್ದರೆ {
                    initStmtp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                        flp, ಹೊಸ AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->ಅಗಲ())};
                }
                initStmtp->ಕೂಡಿಸು_ಮುಂದಿನ(createForceRdUpdateStmt(info));
                ಸಕ್ರಿಯ_Initp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstInitial{flp, initStmtp});
                scopep->ಕೂಡಿಸು_Blocksp(ಸಕ್ರಿಯ_Initp);

                Ast_Sen_ಅಂಶ* itemsp = ಶೂನ್ಯಸೂಚಕ;
                ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_Sen_ಅಂಶ = [&](AstVarScope* vscp) {
                    ಆದರೆ (!vscp) ಹಿಂತಿರುಗಿ;
                    Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ nextp = ಹೊಸ Ast_Sen_ಅಂಶ{
                        flp, VEdgeType::ET_ಬದಲಾಯಿತು, ಹೊಸ AstVarRef{flp, vscp, VAccess::READ}};
                    ಆದರೆ (itemsp) {
                        itemsp->ಕೂಡಿಸು_ಮುಂದಿನ(nextp);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        itemsp = nextp;
                    }
                };
                ಕೂಡಿಸು_Sen_ಅಂಶ(info.m_forceEnVscp);
                ಕೂಡಿಸು_Sen_ಅಂಶ(info.m_forceValVscp);
                AstVarRef* ಸ್ಥಿರಾಂಕ origSenRefp = ಹೊಸ AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ origItemp
                    = ಹೊಸ Ast_Sen_ಅಂಶ{flp, VEdgeType::ET_ಬದಲಾಯಿತು, origSenRefp};
                ಆದರೆ (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->ಕೂಡಿಸು_ಮುಂದಿನ(origItemp);
                ಗಾಗಿ (ForceInfo* ಸ್ಥಿರಾಂಕ finfop : forceps) {
                    ಆದರೆ (!finfop->m_isExternal) ಕೂಡಿಸು_Sen_ಅಂಶ(finfop->m_rhsVarVscp);
                }

                Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep
                    = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "force-rd-update", ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, itemsp}};
                activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());
                activep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{flp, VAlwaysKwd::ALWAYS, ಶೂನ್ಯಸೂಚಕ,
                                                 createForceRdUpdateStmt(info)});
                scopep->ಕೂಡಿಸು_Blocksp(activep);
            }
        }
    }

    Ast_ಗಂಟು* createRhsUpdatesForWrite(File_ಸಾಲು* flp, AstVar* writtenVarp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (!doing_ನಿಯೋಜಿಸು()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_rhs_Dep_ಗಮ್ಯ_Forces.ಕಂಡುಹಿಡಿ(writtenVarp);
        ಆದರೆ (it == m_rhs_Dep_ಗಮ್ಯ_Forces.ಕೊನೆ()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        Ast_ಗಂಟು* headp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು* tailp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ForceInfo* ಸ್ಥಿರಾಂಕ finfop : it->ಎರಡನೇ) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ updatep
                = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
            ಆದರೆ (tailp) {
                tailp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(updatep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                headp = updatep;
            }
            tailp = updatep;
        }
        ಹಿಂತಿರುಗಿ headp;
    }

    ಸ್ಥಿರಾಂಕ ForceInfo& getForceInfo(Ast_ನಿಯೋಜಿಸು_Force* forceStmtp) ಸ್ಥಿರಾಂಕ {
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        ಸ್ಥಿರಾಂಕ VarForceInfo* ಸ್ಥಿರಾಂಕ varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        ಸ್ವಯಂಚಾಲಿತ it2 = varInfo->m_forces.ಕಂಡುಹಿಡಿ(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.ಕೊನೆ(), "Force statement not found");
        ಹಿಂತಿರುಗಿ it2->ಎರಡನೇ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ selOverlapsAnyForce(ಸ್ಥಿರಾಂಕ VarForceInfo& varInfo, ಪೂರ್ಣಾಂಕ selLsb, ಪೂರ್ಣಾಂಕ selMsb) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : varInfo.m_forces) {
            ಆದರೆ (pair.ಎರಡನೇ.m_rangeLsb <= selMsb && pair.ಎರಡನೇ.m_rangeMsb >= selLsb) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* create_Force_Read_ಅಭಿವ್ಯಕ್ತಿ(ಸ್ಥಿರಾಂಕ VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) ಸ್ಥಿರಾಂಕ {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = originalRefp->fileline();
        AstVarRef* ಸ್ಥಿರಾಂಕ refp = originalRefp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        refp->access(VAccess::READ);
        ಹಿಂತಿರುಗಿ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   ಶೂನ್ಯಸೂಚಕ);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* create_Force_Read_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(ಸ್ಥಿರಾಂಕ VarForceInfo& varInfo,
                                                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* originalExprp,
                                                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸೂಚ್ಯಂಕ_Exprp) ಸ್ಥಿರಾಂಕ {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = originalExprp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = originalExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        // Must be an LValue to a static variable
        VN_AS(exprp->get_VAccess_ಗುರಿ_Recurse(), VarRef)->access(VAccess::READ);
        ಹಿಂತಿರುಗಿ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_ಸೂಚ್ಯಂಕ, exprp, originalExprp,
                                   ಸೂಚ್ಯಂಕ_Exprp);
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rebuild_Sel_ಮಾರ್ಗ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pathp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ತಳ_Exprp) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(pathp, Sel)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = rebuild_Sel_ಮಾರ್ಗ(selp->fromp(), ತಳ_Exprp);
            AstSel* ಸ್ಥಿರಾಂಕ outp
                = ಹೊಸ AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->ಅಗಲ_Const()};
            outp->dtypeFrom(selp);
            ಹಿಂತಿರುಗಿ outp;
        }
        ಹಿಂತಿರುಗಿ ತಳ_Exprp;
    }
};

// Split deassign concat LHS before converting to release internals.
ಸ್ಥಿರ ಶೂನ್ಯ ವಿಭಜಿಸು_Deassign(AstDeassign* nodep) {
    AstConcat* ಸ್ಥಿರಾಂಕ concatp = VN_CAST(nodep->lhsp(), Concat);
    ಆದರೆ (!concatp) ಹಿಂತಿರುಗಿ;

    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
    AstDeassign* ಸ್ಥಿರಾಂಕ newLp = ಹೊಸ AstDeassign{flp, concatp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ()};
    AstDeassign* ಸ್ಥಿರಾಂಕ newRp = ಹೊಸ AstDeassign{flp, concatp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ()};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ conp = concatp->unlink_Fr_ಹಿಂಭಾಗ();
    nodep->replaceWith(newLp);
    newLp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newRp);
    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    VL_DO_DANGLING(conp->delete_ವೃಕ್ಷ(), conp);

    ವಿಭಜಿಸು_Deassign(newLp);
    ವಿಭಜಿಸು_Deassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

ವರ್ಗ ForceDiscoveryVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    ForceState& m_state;
    ತಾರ್ಕಿಕ m_ಒಳ_Clocked_ಸಕ್ರಿಯ = ಸುಳ್ಳು;

    ಶೂನ್ಯ buildForceableUnpackedArray(AstVarScope* ಸ್ಥಿರಾಂಕ nodep,
                                     AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrDtypep) {
        AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.ಖಾಲಿಗಳು(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಎಲೆ_Dtypep = dims.ಹಿಂಭಾಗ()->subDTypep()->skipRefp();
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ innerBasicp = ಎಲೆ_Dtypep->basicp();
        ಆದರೆ (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            ಹಿಂತಿರುಗಿ;
        }

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = varp->fileline();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ inner_ಅಗಲ = ಎಲೆ_Dtypep->ಅಗಲ();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* ಸ್ಥಿರಾಂಕ enVscp = info.m_forceEnVscp;
        AstVarScope* ಸ್ಥಿರಾಂಕ valVscp = info.m_forceValVscp;

        Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ itemsp = ಹೊಸ Ast_Sen_ಅಂಶ{flp, VEdgeType::ET_ಬದಲಾಯಿತು,
                                                  ಹೊಸ AstVarRef{flp, enVscp, VAccess::READ}};
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "force-update", ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, itemsp}};
        activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());

        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ alwaysBodyHeadp = ForceState::foreach_Unpacked_ಎಲೆ(
            dims, [&](ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ>& idx, ಪೂರ್ಣಾಂಕ flat) -> Ast_ಗಂಟು_Stmt* {
                AstVarRef* ಸ್ಥಿರಾಂಕ origRefp = ಹೊಸ AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enSelp = ForceState::buildNestedArraySel(
                    flp, ಹೊಸ AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valSelp = ForceState::buildNestedArraySel(
                    flp, ಹೊಸ AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ forceExprp = ಹೊಸ AstOr{
                    flp, ಹೊಸ AstAnd{flp, enSelp, valSelp},
                    ಹೊಸ AstAnd{flp, ಹೊಸ AstNot{flp, enSelp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)}, origSelp}};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsSelp = ForceState::buildNestedArraySel(
                    flp, ಹೊಸ AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                Ast_ನಿಯೋಜಿಸು_Force* ಸ್ಥಿರಾಂಕ forceAssignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_Force{flp, lhsSelp, forceExprp};
                forceAssignp->user2(ನಿಜ);

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsClonep = forceExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                rhsClonep->foreach([varp](AstVarRef* ಸ್ಥಿರಾಂಕ r) {
                    ಆದರೆ (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.ಕೂಡಿಸು_Force_Assignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/inner_ಅಗಲ - 1,
                                           /*hasArraySel=*/ನಿಜ, /*skipRdRefresh=*/ನಿಜ);
                ಹಿಂತಿರುಗಿ forceAssignp;
            });
        activep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{flp, VAlwaysKwd::ALWAYS, ಶೂನ್ಯಸೂಚಕ, alwaysBodyHeadp});
        nodep->scopep()->ಕೂಡಿಸು_Blocksp(activep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Force* nodep) override {
        ಆದರೆ (nodep->user2()) ಹಿಂತಿರುಗಿ;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* ಸ್ಥಿರಾಂಕ lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* ಸ್ಥಿರಾಂಕ forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, ನಿಜ);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsExprp = nodep->rhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);

        m_state.ಕೂಡಿಸು_Force_Assignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (m_state.doing_ನಿಯೋಜಿಸು() && m_ಒಳ_Clocked_ಸಕ್ರಿಯ) {
            ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಕ್ರಿಯ* nodep) override {
        VL_RESTORER(m_ಒಳ_Clocked_ಸಕ್ರಿಯ);
        m_ಒಳ_Clocked_ಸಕ್ರಿಯ = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope* nodep) override {
        ಆದರೆ (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            ಆದರೆ (m_state.doing_ನಿಯೋಜಿಸು()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                ಹಿಂತಿರುಗಿ;
            }

            ಆದರೆ (AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                ಹಿಂತಿರುಗಿ;
            }

            ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ bdtypep = nodep->varp()->basicp();
            ಆದರೆ (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* ಸ್ಥಿರಾಂಕ enVscp = info.m_forceEnVscp;
            AstVarScope* ಸ್ಥಿರಾಂಕ valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ itemsp = ಹೊಸ Ast_Sen_ಅಂಶ{flp, VEdgeType::ET_ಬದಲಾಯಿತು,
                                                      ಹೊಸ AstVarRef{flp, enVscp, VAccess::READ}};
            Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep
                = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "force-update", ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, itemsp}};
            activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* ಸ್ಥಿರಾಂಕ enRefp = ಹೊಸ AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* ಸ್ಥಿರಾಂಕ valRefp = ಹೊಸ AstVarRef{flp, valVscp, VAccess::READ};
            ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstOr{
                            flp, ಹೊಸ AstAnd{flp, enRefp, valRefp},
                            ಹೊಸ AstAnd{flp, ಹೊಸ AstNot{flp, enRefp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)}, refp}})
                      : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstCond{flp, enRefp, valRefp, refp});
            Ast_ನಿಯೋಜಿಸು_Force* ಸ್ಥಿರಾಂಕ forceAssignp
                = ಹೊಸ Ast_ನಿಯೋಜಿಸು_Force{flp, ಹೊಸ AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(ನಿಜ);
            activep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{flp, VAlwaysKwd::ALWAYS, ಶೂನ್ಯಸೂಚಕ, forceAssignp});
            nodep->scopep()->ಕೂಡಿಸು_Blocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsClonep = forceExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            rhsClonep->foreach([varp](AstVarRef* ಸ್ಥಿರಾಂಕ refp) {
                ಆದರೆ (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ bitwiseVar = ForceState::isBitwiseDType(varp);
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ padMsb = bitwiseVar ? (varp->ಅಗಲ() - 1) : 0;
            ಪೂರ್ಣಾಂಕ rangeLsb = 0;
            ಪೂರ್ಣಾಂಕ rangeMsb = padMsb;
            ಆದರೆ (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.ಕೂಡಿಸು_Force_Assignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, ಸುಳ್ಳು);
        }
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

ವರ್ಗ ForceConvertVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    ForceState& m_state;

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Force* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp();
        AstVarRef* ಸ್ಥಿರಾಂಕ lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ಸ್ಥಿರಾಂಕ forcedVarp = lhsVarRefp->varp();

        ಸ್ಥಿರಾಂಕ ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        ಸ್ಥಿರಾಂಕ ForceState::VarForceInfo* ಸ್ಥಿರಾಂಕ varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ rhsAssignp
            = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};

        Ast_ನಿಯೋಜಿಸು* valAssignp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ನಿಯೋಜಿಸು* enAssignp = ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        ಆದರೆ (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ತಳ_Exprp = nodep->rhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            ತಳ_Exprp->foreach(
                [](AstVarRef* ಸ್ಥಿರಾಂಕ refp) { ForceState::markNonReplaceable(refp); });
            ಆದರೆ (bitwiseForcedVar) {
                ತಳ_Exprp = ForceState::zero_Pad_ಗಮ್ಯ_ತಳ_ಅಗಲ(ತಳ_Exprp, forcedVarp->ಅಗಲ(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            ಆದರೆ (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* ಸ್ಥಿರಾಂಕ ಮುಖವಾಡ_Constp = ForceState::make_Range_ಮುಖವಾಡ_Const(
                    nodep, forcedVarp->ಅಗಲ(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valReadp
                    = ಹೊಸ AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valWritep
                    = ಹೊಸ AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notMaskp = ಹೊಸ AstNot{flp, ಮುಖವಾಡ_Constp};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ maskedOldp = ಹೊಸ AstAnd{flp, valReadp, notMaskp};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newValp = ಹೊಸ AstOr{flp, maskedOldp, ತಳ_Exprp};
                valAssignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, valWritep, newValp};

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enReadp
                    = ಹೊಸ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enWritep
                    = ಹೊಸ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newEnp
                    = ಹೊಸ AstOr{flp, enReadp, ಮುಖವಾಡ_Constp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                enAssignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, enWritep, newEnp};
            } ಇಲ್ಲದಿದ್ದರೆ {
                AstConst* ಸ್ಥಿರಾಂಕ oneConstp = ForceState::make_Range_ಮುಖವಾಡ_Const(nodep, 1, 0, 0);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsValp = ForceState::cast_ಗಮ್ಯ_ಗಂಟು_DType(ತಳ_Exprp, forcedVarp);
                valAssignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                    flp, ಹೊಸ AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                    flp, ಹೊಸ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selLhsp = VN_CAST(lhsp, Sel);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ array_ಬಿಟ್_Sel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_has_ಬಿಟ್_Sel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->ಅಗಲ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsDatap = ForceState::build_Rhs_ದತ್ತಾಂಶ_ಅಭಿವ್ಯಕ್ತಿ(flp, info);
        AstCExpr* ಸ್ಥಿರಾಂಕ rhsAddrp = ಹೊಸ AstCExpr{flp};
        rhsAddrp->ಕೂಡಿಸು("&(");
        rhsAddrp->ಕೂಡಿಸು(rhsDatap);
        rhsAddrp->ಕೂಡಿಸು(")");
        AstCMethodHard* ಸ್ಥಿರಾಂಕ ಕೂಡಿಸು_Force_Callp = ಹೊಸ AstCMethodHard{
            flp, ಹೊಸ AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ಕೂಡಿಸು,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        ಕೂಡಿಸು_Force_Callp->ಕೂಡಿಸು_Pinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        ಕೂಡಿಸು_Force_Callp->ಕೂಡಿಸು_Pinsp(rhsAddrp);
        ಕೂಡಿಸು_Force_Callp->ಕೂಡಿಸು_Pinsp(
            ForceState::makeConst32(flp, array_ಬಿಟ್_Sel ? info.m_padLsb : info.m_rangeLsb));
        ಆದರೆ (array_ಬಿಟ್_Sel) {
            ಕೂಡಿಸು_Force_Callp->ಕೂಡಿಸು_Pinsp(ForceState::makeConst32(flp, info.m_padLsb));
            ಕೂಡಿಸು_Force_Callp->ಕೂಡಿಸು_Pinsp(ForceState::makeConst32(flp, info.m_padMsb));
            ಕೂಡಿಸು_Force_Callp->ಕೂಡಿಸು_Pinsp(ForceState::makeConst32(flp, selLhsp->fromp()->ಅಗಲ()));
        }
        ಕೂಡಿಸು_Force_Callp->dtype_ಗಣ_Void();
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtp = ಕೂಡಿಸು_Force_Callp->makeStmt();

        Ast_ಗಂಟು* tailp = rhsAssignp;
        ಆದರೆ (valAssignp) {
            tailp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(valAssignp);
            tailp = valAssignp;
        }
        ಆದರೆ (enAssignp) {
            tailp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(enAssignp);
            tailp = enAssignp;
        }
        tailp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(stmtp);
        ಆದರೆ (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp();
        AstVarRef* ಸ್ಥಿರಾಂಕ lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ಸ್ಥಿರಾಂಕ releasedVarp = lhsVarRefp->varp();

        ಸ್ಥಿರಾಂಕ ForceState::VarForceInfo* ಸ್ಥಿರಾಂಕ varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        ಆದರೆ (!varInfo) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        UASSERT_OBJ(!varInfo->m_forces.ಖಾಲಿಗಳು(), nodep, "Var info for variable with no forces");

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();

        ಸ್ಥಿರಾಂಕ ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, ಸುಳ್ಳು);

        ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selLhsp = VN_CAST(lhsp, Sel);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ array_ಬಿಟ್_Sel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_has_ಬಿಟ್_Sel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->ಅಗಲ();
        AstCMethodHard* ಸ್ಥಿರಾಂಕ releaseCallp = ಹೊಸ AstCMethodHard{
            flp, ಹೊಸ AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->ಕೂಡಿಸು_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        ಆದರೆ (array_ಬಿಟ್_Sel) {
            releaseCallp->ಕೂಡಿಸು_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->ಕೂಡಿಸು_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtype_ಗಣ_Void();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ releasep = releaseCallp->makeStmt();

        Ast_ನಿಯೋಜಿಸು* clearEnp = ಶೂನ್ಯಸೂಚಕ;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        ಆದರೆ (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enWritep
                = ಹೊಸ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            ಆದರೆ (ForceState::isBitwiseDType(releasedVarp)) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ var_ಅಗಲ = releasedVarp->ಅಗಲ();
                ಆದರೆ (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == var_ಅಗಲ - 1) {
                    clearEnp
                        = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, enWritep, ForceState::makeZeroConst(nodep, var_ಅಗಲ)};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // forceEn = forceEn & ~mask(range);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enReadp
                        = ಹೊಸ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* ಸ್ಥಿರಾಂಕ ಮುಖವಾಡ_Const = ForceState::make_Range_ಮುಖವಾಡ_Const(
                        nodep, var_ಅಗಲ, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newEnp
                        = ಹೊಸ AstAnd{flp, enReadp, ಹೊಸ AstNot{flp, ಮುಖವಾಡ_Const}};
                    clearEnp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, enWritep, newEnp};
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                clearEnp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(lhsp, Sel);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ basep = selp ? selp->fromp() : lhsp;

        Ast_ಗಂಟು* stmtListp = releasep;
        ಆದರೆ (clearEnp) {
            clearEnp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ತುಂಬಿದ_Bitwise_Release
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->ಅಗಲ() - 1;
        ಆದರೆ (!releasedVarp->isContinuously()
            && !(m_state.doing_ನಿಯೋಜಿಸು() && m_state.hasClockedWrite(releasedVarp)
                 && ತುಂಬಿದ_Bitwise_Release)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ has_Opaque_ಮಾರ್ಗ = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* forceReadp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (has_Opaque_ಮಾರ್ಗ) {
                forceReadp = m_state.create_Force_Read_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.create_Force_Read_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(
                    *varInfo, basep,
                    ForceState::build_Flatten_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(flp, rangeInfo.m_arrayInfo));
                ಆದರೆ (selp) { forceReadp = ForceState::rebuild_Sel_ಮಾರ್ಗ(lhsp, forceReadp); }
            } ಇಲ್ಲದಿದ್ದರೆ {
                forceReadp
                    = selp ? ForceState::rebuild_Sel_ಮಾರ್ಗ(
                                 lhsp, m_state.create_Force_Read_ಅಭಿವ್ಯಕ್ತಿ(*varInfo, lhsVarRefp))
                           : m_state.create_Force_Read_ಅಭಿವ್ಯಕ್ತಿ(*varInfo, lhsVarRefp);
            }
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), forceReadp};
            assignp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(stmtListp);
            stmtListp = assignp;
        }

        ಆದರೆ (varInfo->m_forceRdVscp) stmtListp->ಕೂಡಿಸು_ಮುಂದಿನ(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_ಮುಂದಿನ_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

ವರ್ಗ ForceReplaceVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    ಸ್ಥಿರಾಂಕ ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_ಗಂಟು_Stmt* m_stmtp = ಶೂನ್ಯಸೂಚಕ;
    ತಾರ್ಕಿಕ m_ಒಳ_Logic = ಸುಳ್ಳು;

    ಶೂನ್ಯ iterateLogic(Ast_ಗಂಟು* nodep) {
        VL_RESTORER(m_ಒಳ_Logic);
        m_ಒಳ_Logic = ನಿಜ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(nodep->lhsp()->ತಳ_Fromp(ನಿಜ), VarRef)) {
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(updatep);
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Cont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate_And_ಮುಂದಿನ_Null(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDeassign*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override { iterateLogic(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override { iterateLogic(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        ಆದರೆ (nodep->keyword() == VAlwaysKwd::CONT_ನಿಯೋಜಿಸು) {
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        iterateLogic(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ಅಂಶ* nodep) override { iterateLogic(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep->fromp(), VarRef);
        ಆದರೆ (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            ಭೇಟಿ(static_cast<Ast_ಗಂಟು*>(nodep));
            ಹಿಂತಿರುಗಿ;
        }

        AstVar* ಸ್ಥಿರಾಂಕ varp = refp->varp();
        ಸ್ಥಿರಾಂಕ ForceState::VarForceInfo* ಸ್ಥಿರಾಂಕ varInfo = m_state.getVarInfo(refp->varScopep());
        ಆದರೆ (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.ಖಾಲಿಗಳು()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            ಭೇಟಿ(static_cast<Ast_ಗಂಟು*>(nodep));
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ selLsb = lsbConstp->ಗಮ್ಯ_SInt();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ selMsb = selLsb + nodep->ಅಗಲ() - 1;
            ಆದರೆ (!varp->is_Sig_ಬಹಿರಂಗ()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                ಭೇಟಿ(static_cast<Ast_ಗಂಟು*>(nodep));
                ಹಿಂತಿರುಗಿ;
            }
        }

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* ಸ್ಥಿರಾಂಕ refClonep = refp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCMethodHard{
            flp, ಹೊಸ AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->ಅಗಲ())};
        callp->ಕೂಡಿಸು_Pinsp(refClonep);
        callp->ಕೂಡಿಸು_Pinsp(nodep->lsbp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
        callp->ಕೂಡಿಸು_Pinsp(ForceState::makeConst32(flp, nodep->ಅಗಲ()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstArraySel* nodep) override {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstArraySel* ಸ್ಥಿರಾಂಕ ಹಿಂಭಾಗ_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            ಆದರೆ (ಹಿಂಭಾಗ_Selp->fromp() == nodep) {
                iterateChildren(nodep);
                ಹಿಂತಿರುಗಿ;
            }
        }

        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ basep = nodep->ತಳ_Fromp(ನಿಜ);
        AstVarRef* ಸ್ಥಿರಾಂಕ ತಳ_Refp = VN_CAST(basep, VarRef);
        ಆದರೆ (!ತಳ_Refp) {
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        AstVar* ಸ್ಥಿರಾಂಕ varp = ತಳ_Refp->varp();
        ಸ್ಥಿರಾಂಕ ForceState::VarForceInfo* ಸ್ಥಿರಾಂಕ varInfo = m_state.getVarInfo(ತಳ_Refp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        ಆದರೆ (m_state.isNotReplaceable(ತಳ_Refp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (!ತಳ_Refp->access().isReadOnly()) {
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (m_state.doing_ನಿಯೋಜಿಸು() && varInfo->m_forceRdVscp) {
            ತಳ_Refp->varp(varInfo->m_forceRdVscp->varp());
            ತಳ_Refp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಸ್ಥಿರಾಂಕ ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        ಗಾಗಿ (AstArraySel* ಸ್ಥಿರಾಂಕ selp : arrayInfo.m_sels) iterate_And_ಮುಂದಿನ_Null(selp->bitp());
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಸೂಚ್ಯಂಕ_Exprp
            = ForceState::build_Flatten_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(nodep->fileline(), arrayInfo);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ readExprp
            = m_state.create_Force_Read_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(*varInfo, nodep, ಸೂಚ್ಯಂಕ_Exprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಆದರೆ (m_state.isNotReplaceable(nodep)) ಹಿಂತಿರುಗಿ;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstArraySel* ಸ್ಥಿರಾಂಕ ಹಿಂಭಾಗ_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            ಆದರೆ (ಹಿಂಭಾಗ_Selp->fromp() == nodep) ಹಿಂತಿರುಗಿ;
        }

        ಸ್ಥಿರಾಂಕ ForceState::VarForceInfo* ಸ್ಥಿರಾಂಕ varInfo = m_state.getVarInfo(nodep->varScopep());
        ಆದರೆ (!varInfo) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (varInfo->m_forceRdVscp) {
            ಆದರೆ (nodep->access().isRW()) {
                ಆದರೆ (m_ಒಳ_Logic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ readp
                    = m_state.create_Force_Rd_ಅಭಿವ್ಯಕ್ತಿ(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (m_ಒಳ_Logic && m_stmtp) {
                m_stmtp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(m_state.createForceRdUpdateStmt(*varInfo));
            }
            ಹಿಂತಿರುಗಿ;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        ಆದರೆ (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            ಹಿಂತಿರುಗಿ;
        }

        UASSERT_OBJ(!varInfo->m_forces.ಖಾಲಿಗಳು(), nodep, "Var info for variable with no forces");

        ಆದರೆ (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ readExprp = m_state.create_Force_Read_ಅಭಿವ್ಯಕ್ತಿ(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (ForceState::is_Outermost_Opaque_ಮಾರ್ಗ_Selector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = VN_AS(nodep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ basep = exprp->ತಳ_Fromp(ನಿಜ);
            AstVarRef* ಸ್ಥಿರಾಂಕ ತಳ_Refp = VN_CAST(basep, VarRef);
            ಆದರೆ (ತಳ_Refp) {
                AstVar* ಸ್ಥಿರಾಂಕ varp = ತಳ_Refp->varp();
                ಆದರೆ (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    ಸ್ಥಿರಾಂಕ ForceState::VarForceInfo* ಸ್ಥಿರಾಂಕ varInfo
                        = m_state.getVarInfo(ತಳ_Refp->varScopep());
                    ಆದರೆ (!m_state.isNotReplaceable(ತಳ_Refp) && varInfo) {
                        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ force_ಮಾರ್ಗ_ಸೂಚ್ಯಂಕ = varInfo->ಕಂಡುಹಿಡಿ_Force_ಮಾರ್ಗ_ಸೂಚ್ಯಂಕ(exprp);
                        ಆದರೆ (force_ಮಾರ್ಗ_ಸೂಚ್ಯಂಕ >= 0) {
                            ಆದರೆ (!ತಳ_Refp->access().isReadOnly()) ಹಿಂತಿರುಗಿ;
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ readExprp = m_state.create_Force_Read_ಸೂಚ್ಯಂಕ_ಅಭಿವ್ಯಕ್ತಿ(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), force_ಮಾರ್ಗ_ಸೂಚ್ಯಂಕ));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                            ಹಿಂತಿರುಗಿ;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ ForceReplaceVisitor(AstNetlist* nodep, ಸ್ಥಿರಾಂಕ ForceState& state)
        : m_state{state} {
        iterate_And_ಮುಂದಿನ_Null(nodep->modulesp());
    }
    ~ForceReplaceVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Non-overlapping force sels", m_nonOverlappingForceSels);
    }
};
//######################################################################
//

//######################################################################
// V3Force - Main entry point

ಸ್ಥಿರ ಶೂನ್ಯ forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::Protected_Var_Ref_ಗಣ& permanentlyProtected,
                         ForceState::Built_Var_Scope_ಗಣ& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    ಆದರೆ (!v3Global.has_Forceable_ಸಂಕೇತಗಳು()) ಹಿಂತಿರುಗಿ;
    ForceState state{ಸುಳ್ಳು, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("force", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಸ್ಥಿರ ಶೂನ್ಯ ನಿಯೋಜಿಸು_All_Impl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::Protected_Var_Ref_ಗಣ& permanentlyProtected,
                          ForceState::Built_Var_Scope_ಗಣ& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    ಆದರೆ (!v3Global.has_ನಿಯೋಜಿಸು_Deassign()) ಹಿಂತಿರುಗಿ;

    std::ಸದಿಶ<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.ತಳ್ಳು_ಹಿಂಭಾಗ(deassignp); });
    ಗಾಗಿ (AstDeassign* ಸ್ಥಿರಾಂಕ deassignp : deassignps) ವಿಭಜಿಸು_Deassign(deassignp);

    std::ಸದಿಶ<Ast_ನಿಯೋಜಿಸು_Cont*> ನಿಯೋಜಿಸು_Contps;
    deassignps.clear();
    nodep->foreach([&](Ast_ಗಂಟು_Stmt* nodep) {
        ಆದರೆ (Ast_ನಿಯೋಜಿಸು_Cont* ಸ್ಥಿರಾಂಕ ನಿಯೋಜಿಸು_Contp = VN_CAST(nodep, ನಿಯೋಜಿಸು_Cont)) {
            ನಿಯೋಜಿಸು_Contps.ತಳ್ಳು_ಹಿಂಭಾಗ(ನಿಯೋಜಿಸು_Contp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstDeassign* ಸ್ಥಿರಾಂಕ deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.ತಳ್ಳು_ಹಿಂಭಾಗ(deassignp);
        }
    });

    ಗಾಗಿ (Ast_ನಿಯೋಜಿಸು_Cont* ಸ್ಥಿರಾಂಕ assignp : ನಿಯೋಜಿಸು_Contps) {
        assignp->replaceWith(ಹೊಸ Ast_ನಿಯೋಜಿಸು_Force{assignp->fileline(),
                                                assignp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                                assignp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ()});
        VL_DO_DANGLING(assignp->delete_ವೃಕ್ಷ(), assignp);
    }
    ಗಾಗಿ (AstDeassign* ಸ್ಥಿರಾಂಕ deassignp : deassignps) {
        deassignp->replaceWith(
            ಹೊಸ AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ)});
        VL_DO_DANGLING(deassignp->delete_ವೃಕ್ಷ(), deassignp);
    }
    ForceState state{ನಿಜ, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("assign-deassign", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3Force::force_And_ನಿಯೋಜಿಸು_All(AstNetlist* nodep) {
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use user3_ಒಳ_Use;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::Protected_Var_Ref_ಗಣ permanentlyProtected;
    ForceState::Built_Var_Scope_ಗಣ forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    ನಿಯೋಜಿಸು_All_Impl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
