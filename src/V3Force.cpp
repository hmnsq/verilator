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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Force.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ ForceState final {
ਜਨਤਕ:
    ਬਣਤਰ ForceRange VL_NOT_FINAL {
        ਪੂਰਨ m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        ਪੂਰਨ m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        ਪੂਰਨ m_padLsb = 0;  // Bit positions for RHS padding
        ਪੂਰਨ m_padMsb = 0;  // Bit positions for RHS padding
    };

    ਬਣਤਰ ForceInfo final : ForceRange {
        // MEMBERS
        ਪੂਰਨ m_forceId = 0;  // Unique (per signal) variable of this force assignment
        ਤਾਰਕਿਕ m_hasArraySel = ਝੂਠ;  // If this has an array select on LHS
        ਤਾਰਕਿਕ m_isExternal = ਝੂਠ;  // Synthetic force for the public force controls
        ਤਾਰਕਿਕ m_skipRdRefresh = ਝੂਠ;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = ਖਾਲੀ_ਸੂਚਕ;  // Scope of the var containing RHSID
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_rhsExprp = ਖਾਲੀ_ਸੂਚਕ;  // Expression on RHS of this force assignment

        ForceInfo() = ਮੂਲ;
        ForceInfo(ਪੂਰਨ rangeLsb, ਪੂਰਨ rangeMsb, ਪੂਰਨ padLsb, ਪੂਰਨ padMsb, ਪੂਰਨ forceId,
                  ਤਾਰਕਿਕ hasArraySel, AstVarScope* rhsVarVscp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsExprp)
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

    ਬਣਤਰ VarForceInfo final {
        AstVarScope* m_forceVecVscp = ਖਾਲੀ_ਸੂਚਕ;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = ਖਾਲੀ_ਸੂਚਕ;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = ਖਾਲੀ_ਸੂਚਕ;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = ਖਾਲੀ_ਸੂਚਕ;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = ਖਾਲੀ_ਸੂਚਕ;  // Scope of the forced (original) variable
        AstVar* m_varp = ਖਾਲੀ_ਸੂਚਕ;  // The forced (original) variable
        AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Scope containing the forced variable
        std::unordered_map<Ast_ਸੌਂਪੋ_Force*, ForceInfo> m_forces;
        std::unordered_map<string, ਪੂਰਨ> m_force_ਰਸਤਾ_ਮੰਜ਼ਿਲ_ਸੂਚਕਾਂਕ;
        ਪੂਰਨ m_ਅਗਲਾ_Force_ਰਸਤਾ_ਸੂਚਕਾਂਕ = 1;  // Start at 1 so 0 can be the base path (whole signal)

    ਨਿਜੀ:
        ਸਥਿਰ ਖਾਲੀ build_Force_ਰਸਤਾ_ਕੁੰਜੀ_Recurse(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, string& ਬਾਹਰਲਾ) {
            ਜੇ (VN_IS(nodep, VarRef)) ਵਾਪਸ;
            ਜੇ (ਅਚਲ AstSel* ਅਚਲ selp = VN_CAST(nodep, Sel)) {
                build_Force_ਰਸਤਾ_ਕੁੰਜੀ_Recurse(selp->fromp(), ਬਾਹਰਲਾ);
                ਬਾਹਰਲਾ += "|S" + cvt_ਮੰਜ਼ਿਲ_Str(selp->lsbConst()) + ":" + cvt_ਮੰਜ਼ਿਲ_Str(selp->ਚੌੜਾਈ_Const());
                ਵਾਪਸ;
            }
            ਜੇ (AstStructSel* ਅਚਲ selp = VN_CAST(nodep, StructSel)) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = selp->fromp();
                build_Force_ਰਸਤਾ_ਕੁੰਜੀ_Recurse(fromp, ਬਾਹਰਲਾ);
                ਬਾਹਰਲਾ += "|T" + selp->name();
                ਵਾਪਸ;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        ਸਥਿਰ string force_ਰਸਤਾ_ਕੁੰਜੀ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
            string ਬਾਹਰਲਾ;
            build_Force_ਰਸਤਾ_ਕੁੰਜੀ_Recurse(nodep, ਬਾਹਰਲਾ);
            ਵਾਪਸ ਬਾਹਰਲਾ;
        }

    ਜਨਤਕ:
        ਪੂਰਨ get_Or_Create_Force_ਰਸਤਾ_ਸੂਚਕਾਂਕ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ pair
                = m_force_ਰਸਤਾ_ਮੰਜ਼ਿਲ_ਸੂਚਕਾਂਕ.emplace(force_ਰਸਤਾ_ਕੁੰਜੀ(nodep), m_ਅਗਲਾ_Force_ਰਸਤਾ_ਸੂਚਕਾਂਕ);
            ਜੇ (pair.ਦੂਜਾ) ++m_ਅਗਲਾ_Force_ਰਸਤਾ_ਸੂਚਕਾਂਕ;
            ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
        }

        ਪੂਰਨ ਲੱਭੋ_Force_ਰਸਤਾ_ਸੂਚਕਾਂਕ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) ਅਚਲ {
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_force_ਰਸਤਾ_ਮੰਜ਼ਿਲ_ਸੂਚਕਾਂਕ.ਲੱਭੋ(force_ਰਸਤਾ_ਕੁੰਜੀ(nodep));
            ਵਾਪਸ it != m_force_ਰਸਤਾ_ਮੰਜ਼ਿਲ_ਸੂਚਕਾਂਕ.ਸਿਰਾ() ? it->ਦੂਜਾ : -1;
        }
    };

    ਬਣਤਰ ForceHelperVars final {
        AstVar* m_rdVarp = ਖਾਲੀ_ਸੂਚਕ;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = ਖਾਲੀ_ਸੂਚਕ;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = ਖਾਲੀ_ਸੂਚਕ;  // The __VforceVal (force value) variable
    };

    ਬਣਤਰ ArraySelInfo final {
        std::ਸਦਿਸ਼<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        ਤਾਰਕਿਕ m_has_ਦੋਆਧਾਰੀ_ਅੰਕ_Sel = ਝੂਠ;  // Bit-select was found along the path too
    };

    ਬਣਤਰ ForceRangeInfo final : ForceRange {
        ਤਾਰਕਿਕ m_hasArraySel = ਝੂਠ;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

ਨਿਜੀ:
    ਵਰਤੋਂ ScopeVarCache = std::unordered_map<ਅਚਲ AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_user2_ਵਿੱਚ_Use;

ਜਨਤਕ:
    ਵਰਤੋਂ ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    ਵਰਤੋਂ Protected_Var_Ref_ਸੈੱਟ = std::unordered_ਸੈੱਟ<ਅਚਲ AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    ਵਰਤੋਂ Built_Var_Scope_ਸੈੱਟ = std::unordered_ਸੈੱਟ<ਅਚਲ AstVarScope*>;

ਨਿਜੀ:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::ਸਦਿਸ਼<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, ਪੂਰਨ> m_var_ਮੰਜ਼ਿਲ_Id;
    std::unordered_ਸੈੱਟ<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::ਸਦਿਸ਼<ForceInfo*>> m_rhs_Dep_ਮੰਜ਼ਿਲ_Forces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    ਤਾਰਕਿਕ m_doing_ਸੌਂਪੋ = ਝੂਠ;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    Protected_Var_Ref_ਸੈੱਟ& m_permanentlyProtected;
    Built_Var_Scope_ਸੈੱਟ& m_forceRdUpdateBuilt;

ਜਨਤਕ:
    ForceState(ਤਾਰਕਿਕ doing_ਸੌਂਪੋ, ForceHelperVarsByVar& forceHelperVarsByVar,
               Protected_Var_Ref_ਸੈੱਟ& permanentlyProtected, Built_Var_Scope_ਸੈੱਟ& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doing_ਸੌਂਪੋ{doing_ਸੌਂਪੋ}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    ਸਥਿਰ AstConst* makeZeroConst(Ast_ਗੰਢ* nodep, ਪੂਰਨ ਚੌੜਾਈ) {
        V3_ਸੰਖਿਆ zero{nodep, ਚੌੜਾਈ};
        zero.ਸੈੱਟ_All_Bits0();
        ਵਾਪਸ ਨਵਾਂ AstConst{nodep->fileline(), zero};
    }

    ਸਥਿਰ AstConst* makeConst32(File_ਸਤਰ* flp, ਪੂਰਨ ਮੁੱਲ) {
        ਵਾਪਸ ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, 32, static_cast<uint32_t>(ਮੁੱਲ)};
    }

    ਸਥਿਰ AstConst* make_Range_ਨਕਾਬ_Const(Ast_ਗੰਢ* nodep, ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ lsb, ਪੂਰਨ msb) {
        V3_ਸੰਖਿਆ ਨਕਾਬ{nodep, ਚੌੜਾਈ};
        ਨਕਾਬ.ਸੈੱਟ_All_Bits0();
        ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = lsb; ਦੋਆਧਾਰੀ_ਅੰਕ_2 <= msb; ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) ਨਕਾਬ.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 1);
        ਵਾਪਸ ਨਵਾਂ AstConst{nodep->fileline(), ਨਕਾਬ};
    }

    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* zero_Pad_ਮੰਜ਼ਿਲ_ਅਧਾਰ_ਚੌੜਾਈ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਪੂਰਨ ਅਧਾਰ_ਚੌੜਾਈ, ਪੂਰਨ padLsb,
                                           ਪੂਰਨ padMsb) {
        ਜੇ (ਅਧਾਰ_ਚੌੜਾਈ <= 0) ਵਾਪਸ exprp;
        ਅਚਲ ਪੂਰਨ ਨੀਵਾਂ_Pad = padLsb;
        ਅਚਲ ਪੂਰਨ ਉੱਚਾ_Pad = ਅਧਾਰ_ਚੌੜਾਈ - (padMsb + 1);
        ਜੇ (ਨੀਵਾਂ_Pad > 0) {
            exprp = ਨਵਾਂ AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, ਨੀਵਾਂ_Pad)};
        }
        ਜੇ (ਉੱਚਾ_Pad > 0) {
            exprp = ਨਵਾਂ AstConcat{exprp->fileline(), makeZeroConst(exprp, ਉੱਚਾ_Pad), exprp};
        }
        ਵਾਪਸ exprp;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ isUnpackedArrayDType(ਅਚਲ Ast_ਗੰਢ_DType* dtypep) {
        ਵਾਪਸ VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    ਸਥਿਰ ਤਾਰਕਿਕ isBitwiseDType(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਅਚਲ AstBasicDType* ਅਚਲ basicp = nodep->dtypep()->skipRefp()->basicp();
        ਵਾਪਸ basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* cast_ਮੰਜ਼ਿਲ_ਗੰਢ_DType(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ* dtypeFromp) {
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = dtypeFromp->dtypep()->skipRefp();
        ਅਚਲ AstBasicDType* ਅਚਲ basicp = dtypep->basicp();
        ਜੇ (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            ਵਾਪਸ exprp;
        }
        ਵਾਪਸ ਨਵਾਂ AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    ਸਥਿਰ ਖਾਲੀ markNonReplaceable(AstVarRef* ਅਚਲ nodep) { nodep->user1_ਸੈੱਟ_Once(); }
    ਤਾਰਕਿਕ isNotReplaceable(ਅਚਲ AstVarRef* ਅਚਲ nodep) ਅਚਲ {
        ਵਾਪਸ nodep->user1() || m_permanentlyProtected.ਗਿਣਤੀ_2(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    ਖਾਲੀ markPermanentlyProtected(AstVarRef* ਅਚਲ nodep) ਅਚਲ {
        markNonReplaceable(nodep);
        m_permanentlyProtected.ਪਾਓ(nodep);
    }

    ਸਥਿਰ std::ਸਦਿਸ਼<ForceInfo*> force_Infos_ਵਿੱਚ_Id_ਕ੍ਰਮ(VarForceInfo& info) {
        std::ਸਦਿਸ਼<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.ਮਾਪ());
        ਲਈ (ਸਵੈਚਾਲਿਤ& it : info.m_forces) forceps.ਧੱਕੋ_ਪਿੱਠ(&it.ਦੂਜਾ);
        std::ਛਾਂਟੋ(forceps.begin(), forceps.ਸਿਰਾ(), [](ਅਚਲ ForceInfo* ap, ਅਚਲ ForceInfo* bp) {
            ਵਾਪਸ ap->m_forceId < bp->m_forceId;
        });
        ਵਾਪਸ forceps;
    }

    ਸਥਿਰ std::ਸਦਿਸ਼<ਅਚਲ ForceInfo*> force_Infos_ਵਿੱਚ_Id_ਕ੍ਰਮ(ਅਚਲ VarForceInfo& info) {
        std::ਸਦਿਸ਼<ਅਚਲ ForceInfo*> forceps;
        forceps.reserve(info.m_forces.ਮਾਪ());
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& it : info.m_forces) forceps.ਧੱਕੋ_ਪਿੱਠ(&it.ਦੂਜਾ);
        std::ਛਾਂਟੋ(forceps.begin(), forceps.ਸਿਰਾ(), [](ਅਚਲ ForceInfo* ap, ਅਚਲ ForceInfo* bp) {
            ਵਾਪਸ ap->m_forceId < bp->m_forceId;
        });
        ਵਾਪਸ forceps;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ is_Opaque_ਰਸਤਾ_Selector(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਵਾਪਸ VN_IS(nodep, Sel) || VN_IS(nodep, ਗੰਢ_Sel) || VN_IS(nodep, StructSel);
    }

    ਸਥਿਰ ਤਾਰਕਿਕ is_Outermost_Opaque_ਰਸਤਾ_Selector(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਜੇ (!is_Opaque_ਰਸਤਾ_Selector(nodep)) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_ਗੰਢ* ਅਚਲ backp = nodep->backp();
        ਵਾਪਸ !backp || !is_Opaque_ਰਸਤਾ_Selector(backp);
    }

    ਸਥਿਰ AstVarRef* getOneVarRef(Ast_ਗੰਢ_ਵਿਅੰਜਕ* forceStmtp) {
        Ast_ਗੰਢ* ਅਚਲ basep = forceStmtp->ਅਧਾਰ_Fromp(ਸੱਚ);
        ਜੇ (AstSampled* sampledp = VN_CAST(basep, Sampled))
            ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = VN_CAST(sampledp->exprp(), ਗੰਢ_ਵਿਅੰਜਕ))
                ਵਾਪਸ getOneVarRef(exprp);
        AstVarRef* ਅਚਲ varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        ਵਾਪਸ varRefp;
    }

    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* buildNestedArraySel(File_ਸਤਰ* flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp,
                                            ਅਚਲ std::ਸਦਿਸ਼<ਪੂਰਨ>& indicies) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* curp = fromp;
        ਲਈ (ਅਚਲ ਪੂਰਨ idx : indicies) curp = ਨਵਾਂ AstArraySel{flp, curp, idx};
        ਵਾਪਸ curp;
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ Fn>
    ਸਥਿਰ Ast_ਗੰਢ_Stmt* foreach_Unpacked_ਪੱਤੀ(ਅਚਲ std::ਸਦਿਸ਼<AstUnpackArrayDType*>& dims,
                                            Fn build_ਪੱਤੀ) {
        Ast_ਗੰਢ_Stmt* headp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_Stmt* tailp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (dims.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਪੂਰਨ ਕੁੱਲ = 1;
        ਲਈ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ d : dims) ਕੁੱਲ *= d->elementsConst();
        ਜੇ (ਕੁੱਲ <= 0) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        std::ਸਦਿਸ਼<ਪੂਰਨ> idx(dims.ਮਾਪ(), 0);
        ਲਈ (ਪੂਰਨ flat = 0; flat < ਕੁੱਲ; ++flat) {
            Ast_ਗੰਢ_Stmt* ਅਚਲ stmtp = build_ਪੱਤੀ(idx, flat);
            ਜੇ (!headp) {
                headp = stmtp;
            } ਨਹੀਂਤਾਂ {
                tailp->ਜੋੜੋ_ਅਗਲਾ(stmtp);
            }
            tailp = stmtp;
            ਲਈ (ਪੂਰਨ d = static_cast<ਪੂਰਨ>(dims.ਮਾਪ()) - 1; d >= 0; --d) {
                ਜੇ (++idx[d] < dims[d]->elementsConst()) ਰੋਕੋ;
                idx[d] = 0;
            }
        }
        ਵਾਪਸ headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, AstVar* varp,
                                     ਤਾਰਕਿਕ requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.ਖਾਲੀ_ਥਾਵਾਂ();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->ਚੌੜਾਈ() - 1) : 0;

        ਜੇ (ਅਚਲ AstSel* ਅਚਲ outerSelp = VN_CAST(lhsp, Sel)) {
            ਪੂਰਨ ਕੁੱਲ_Lsb = 0;
            ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* curp = lhsp; ਅਚਲ AstSel* ਅਚਲ selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                ਜੇ (requireConstRangeSelect) {
                    UASSERT_OBJ(VN_IS(selp->lsbp(), Const), lhsp,
                                "Unsupported: force on non-const range select");
                }
                ਕੁੱਲ_Lsb += selp->lsbConst();
            }
            info.m_padLsb = ਕੁੱਲ_Lsb;
            info.m_padMsb = ਕੁੱਲ_Lsb + outerSelp->ਚੌੜਾਈ_Const() - 1;
        }

        info.m_rangeLsb = info.m_padLsb;
        info.m_rangeMsb = info.m_padMsb;
        ਜੇ (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::ਸਦਿਸ਼<ਪੂਰਨ> indices;
            indices.reserve(info.m_arrayInfo.m_sels.ਮਾਪ());
            ਲਈ (AstArraySel* ਅਚਲ selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.ਧੱਕੋ_ਪਿੱਠ(VN_AS(selp->bitp(), Const)->ਮੰਜ਼ਿਲ_SInt());
            }
            info.m_rangeLsb = flatten_ਸੂਚਕਾਂਕ(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } ਨਹੀਂਤਾਂ ਜੇ (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        ਜੇ (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            ਅਚਲ ਪੂਰਨ ਸੂਚਕਾਂਕ = varInfo.get_Or_Create_Force_ਰਸਤਾ_ਸੂਚਕਾਂਕ(lhsp);
            info.m_rangeLsb = ਸੂਚਕਾਂਕ;
            info.m_rangeMsb = ਸੂਚਕਾਂਕ;
        }
        ਵਾਪਸ info;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਜੋੜੋ_Rhs_ਮੁੱਲ_Reads(ਅਚਲ VarForceInfo& varInfo, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) ਅਚਲ {
        ਜੇ (!doing_ਸੌਂਪੋ()) ਵਾਪਸ exprp;

        ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ ForceInfo*> forceps = force_Infos_ਵਿੱਚ_Id_ਕ੍ਰਮ(varInfo);
        ਜੇ (forceps.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* ਅਚਲ cexprp = ਨਵਾਂ AstCExpr{exprp->fileline(), AstCExpr::ਸ਼ੁੱਧ{}};
        cexprp->dtypeFrom(exprp);
        cexprp->ਜੋੜੋ("(");
        ਲਈ (ਅਚਲ ForceInfo* ਅਚਲ finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* ਅਚਲ refp
                = ਨਵਾਂ AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->ਜੋੜੋ("(void)(");
            cexprp->ਜੋੜੋ(refp);
            cexprp->ਜੋੜੋ("), ");
        }
        cexprp->ਜੋੜੋ(exprp);
        cexprp->ਜੋੜੋ(")");
        ਵਾਪਸ cexprp;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* createForceReadCall(ਅਚਲ VarForceInfo& varInfo, File_ਸਤਰ* flp, VCMethod ਵਿਧੀ,
                                     Ast_ਗੰਢ_ਵਿਅੰਜਕ* originalExprp, Ast_ਗੰਢ* dtypeFromp,
                                     Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਸੂਚਕਾਂਕ_Exprp) ਅਚਲ {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ origValp
            = ਜੋੜੋ_Rhs_ਮੁੱਲ_Reads(varInfo, cast_ਮੰਜ਼ਿਲ_ਗੰਢ_DType(originalExprp, dtypeFromp));

        AstCMethodHard* ਅਚਲ callp = ਨਵਾਂ AstCMethodHard{
            flp, ਨਵਾਂ AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, ਵਿਧੀ, origValp};
        ਜੇ (ਸੂਚਕਾਂਕ_Exprp) callp->ਜੋੜੋ_Pinsp(ਸੂਚਕਾਂਕ_Exprp);
        callp->dtypeFrom(dtypeFromp);
        ਵਾਪਸ callp;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* create_Force_Rd_ਵਿਅੰਜਕ(ਅਚਲ VarForceInfo& varInfo, File_ਸਤਰ* flp) ਅਚਲ {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* ਅਚਲ varp = varInfo.m_varVscp->varp();
        ਜੇ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            ਜੇ (doing_ਸੌਂਪੋ()) { ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::UNOPTFLAT, ਸੱਚ);
            AstVarRef* ਅਚਲ ਅਧਾਰ_Refp = ਨਵਾਂ AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(ਅਧਾਰ_Refp);
            ਵਾਪਸ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, ਅਧਾਰ_Refp, varp,
                                       ਖਾਲੀ_ਸੂਚਕ);
        }
        AstVarRef* ਅਚਲ ਅਧਾਰ_Refp = ਨਵਾਂ AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(ਅਧਾਰ_Refp);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enRefp = ਨਵਾਂ AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valRefp = ਨਵਾਂ AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        ਜੇ (isBitwiseDType(varp)) {
            ਵਾਪਸ ਨਵਾਂ AstOr{
                flp, ਨਵਾਂ AstAnd{flp, enRefp, valRefp},
                ਨਵਾਂ AstAnd{flp, ਨਵਾਂ AstNot{flp, enRefp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)}, ਅਧਾਰ_Refp}};
        }
        ਵਾਪਸ ਨਵਾਂ AstCond{flp, enRefp, valRefp, ਅਧਾਰ_Refp};
    }

    Ast_ਗੰਢ_Stmt* createForceRdUpdateStmt(ਅਚਲ VarForceInfo& varInfo) ਅਚਲ {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* ਅਚਲ varp = varInfo.m_varVscp->varp();
        ਜੇ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            ਵਾਪਸ createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_ਸਤਰ* ਅਚਲ flp = varInfo.m_varVscp->fileline();
        ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             create_Force_Rd_ਵਿਅੰਜਕ(varInfo, flp)};
    }

    Ast_ਗੰਢ_Stmt* createForceRdUpdateStmtUnpacked(ਅਚਲ VarForceInfo& varInfo) ਅਚਲ {
        File_ਸਤਰ* ਅਚਲ flp = varInfo.m_varVscp->fileline();
        AstVar* ਅਚਲ varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* ਅਚਲ arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        ਅਚਲ std::ਸਦਿਸ਼<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_ਗੰਢ_Stmt* ਅਚਲ stmtsp = foreach_Unpacked_ਪੱਤੀ(
            dims, [&](ਅਚਲ std::ਸਦਿਸ਼<ਪੂਰਨ>& idx, ਪੂਰਨ /*flat*/) -> Ast_ਗੰਢ_Stmt* {
                AstVarRef* ਅਚਲ ਅਧਾਰ_Refp = ਨਵਾਂ AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(ਅਧਾਰ_Refp);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਅਧਾਰ_Selp = buildNestedArraySel(flp, ਅਧਾਰ_Refp, idx);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enSelp = buildNestedArraySel(
                    flp, ਨਵਾਂ AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valSelp = buildNestedArraySel(
                    flp, ਨਵਾਂ AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ readExprp = ਨਵਾਂ AstOr{
                    flp, ਨਵਾਂ AstAnd{flp, enSelp, valSelp},
                    ਨਵਾਂ AstAnd{flp, ਨਵਾਂ AstNot{flp, enSelp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)}, ਅਧਾਰ_Selp}};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rdLhsSelp = buildNestedArraySel(
                    flp, ਨਵਾਂ AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        ਲਈ (ਅਚਲ ForceInfo* ਅਚਲ finfop : force_Infos_ਵਿੱਚ_Id_ਕ੍ਰਮ(varInfo)) {
            ਜੇ (!finfop->m_isExternal) ਜਾਰੀ;
            stmtsp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp,
                                          ਨਵਾਂ AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)});
        }
        ਵਾਪਸ stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_var_ਮੰਜ਼ਿਲ_Id.ਲੱਭੋ(vscp);
        ਜੇ (it != m_var_ਮੰਜ਼ਿਲ_Id.ਸਿਰਾ()) ਵਾਪਸ m_varInfos[it->ਦੂਜਾ];

        m_var_ਮੰਜ਼ਿਲ_Id.emplace(vscp, m_varInfos.ਮਾਪ());
        m_varInfos.emplace_ਪਿੱਠ();
        VarForceInfo& info = m_varInfos.ਪਿੱਠ();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* ਅਚਲ varp = info.m_varp;
        ਜੇ (!varp->isForceable()) ਵਾਪਸ info;

        File_ਸਤਰ* ਅਚਲ flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        ਅਚਲ ਤਾਰਕਿਕ helperVarsBuilt = helperVars.m_rdVarp != ਖਾਲੀ_ਸੂਚਕ;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != ਖਾਲੀ_ਸੂਚਕ)
                        && helperVarsBuilt == (helperVars.m_valVarp != ਖਾਲੀ_ਸੂਚਕ),
                    varp, "Incomplete force helper set");
        ਜੇ (!helperVarsBuilt) {
            ਅਚਲ ਤਾਰਕਿਕ unpacked = isUnpackedArrayDType(varp->dtypep());
            ਅਚਲ VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_ਗੰਢ_DType* ਅਚਲ enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType();
            helperVars.m_rdVarp
                = ਨਵਾਂ AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sig_ਜਨਤਕ(ਸੱਚ);
            helperVars.m_enVarp
                = ਨਵਾਂ AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(ਸੱਚ);
            helperVars.m_valVarp
                = ਨਵਾਂ AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(ਸੱਚ);
            varp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(helperVars.m_rdVarp);
            varp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(helperVars.m_enVarp);
            varp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = ਲੱਭੋ_Scope_Var(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = ਲੱਭੋ_Scope_Var(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = ਲੱਭੋ_Scope_Var(info.m_scopep, helperVars.m_valVarp);
        ਜੇ (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } ਨਹੀਂਤਾਂ {
            info.m_forceRdVscp = ਨਵਾਂ AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = ਨਵਾਂ AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = ਨਵਾਂ AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->ਜੋੜੋ_Varsp(info.m_forceRdVscp);
            info.m_scopep->ਜੋੜੋ_Varsp(info.m_forceEnVscp);
            info.m_scopep->ਜੋੜੋ_Varsp(info.m_forceValVscp);
        }
        ਵਾਪਸ info;
    }

    ਖਾਲੀ markClockedWrite(AstVar* varp) { m_clockedWrites.ਪਾਓ(varp); }
    ਤਾਰਕਿਕ hasClockedWrite(AstVar* varp) ਅਚਲ { ਵਾਪਸ m_clockedWrites.ਗਿਣਤੀ_2(varp); }

    ਤਾਰਕਿਕ doing_ਸੌਂਪੋ() ਅਚਲ { ਵਾਪਸ m_doing_ਸੌਂਪੋ; }

    ਅਚਲ VarForceInfo* getVarInfo(AstVarScope* vscp) ਅਚਲ {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_var_ਮੰਜ਼ਿਲ_Id.ਲੱਭੋ(vscp);
        ਵਾਪਸ it != m_var_ਮੰਜ਼ਿਲ_Id.ਸਿਰਾ() ? &m_varInfos[it->ਦੂਜਾ] : ਖਾਲੀ_ਸੂਚਕ;
    }

    AstVarScope* ਲੱਭੋ_Scope_Var(AstScope* scopep, ਅਚਲ AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        ਜੇ (cache.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਲਈ (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = cache.ਲੱਭੋ(varp);
        ਵਾਪਸ it != cache.ਸਿਰਾ() ? it->ਦੂਜਾ : ਖਾਲੀ_ਸੂਚਕ;
    }
    ਖਾਲੀ ਜੋੜੋ_Force_Assignment(AstVar* varp, AstVarScope* vscp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsExprp,
                            Ast_ਸੌਂਪੋ_Force* forceStmtp, ਪੂਰਨ rangeLsb, ਪੂਰਨ rangeMsb, ਪੂਰਨ padLsb,
                            ਪੂਰਨ padMsb, ਤਾਰਕਿਕ hasArraySel, ਤਾਰਕਿਕ skipRdRefresh = ਝੂਠ) {
        v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Force();
        varp->ਸੈੱਟ_Forced_By_ਕੋਡ();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        ਅਚਲ ਪੂਰਨ forceId = info.m_forces.ਮਾਪ();
        File_ਸਤਰ* ਅਚਲ flp = varp->fileline();
        AstScope* ਅਚਲ scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        ਜੇ (!info.m_forceVecVscp) {
            AstCDType* ਅਚਲ forceVecDtypep = ਨਵਾਂ AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(forceVecDtypep);

            AstVar* ਅਚਲ forceVecVarp
                = ਨਵਾਂ AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doing_ਸੌਂਪੋ ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(ਝੂਠ);
            forceVecVarp->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
            varp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(forceVecVarp);
            info.m_forceVecVscp = ਨਵਾਂ AstVarScope{flp, scopep, forceVecVarp};
            scopep->ਜੋੜੋ_Varsp(info.m_forceVecVscp);
        }

        ਸਵੈਚਾਲਿਤ pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, ਖਾਲੀ_ਸੂਚਕ, rhsExprp});
        ForceInfo& finfo = pair.ਪਹਿਲਾ->ਦੂਜਾ;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        ਜੇ (doing_ਸੌਂਪੋ()) {
            std::ਸਦਿਸ਼<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* ਅਚਲ refp) {
                ਜੇ (!refp->access().isReadOnly()) ਵਾਪਸ;
                AstVar* ਅਚਲ depVarp = refp->varp();
                ਜੇ (depVarp
                    && std::ਲੱਭੋ(depVarps.begin(), depVarps.ਸਿਰਾ(), depVarp) == depVarps.ਸਿਰਾ()) {
                    depVarps.ਧੱਕੋ_ਪਿੱਠ(depVarp);
                }
            });
            ਲਈ (AstVar* ਅਚਲ depVarp : depVarps) m_rhs_Dep_ਮੰਜ਼ਿਲ_Forces[depVarp].ਧੱਕੋ_ਪਿੱਠ(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    ਸਥਿਰ ਖਾਲੀ collectArraySelInfo(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ArraySelInfo& info) {
        ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ* ਅਚਲ selp = VN_CAST(exprp, Sel)) {
            info.m_has_ਦੋਆਧਾਰੀ_ਅੰਕ_Sel = ਸੱਚ;
            collectArraySelInfo(selp->fromp(), info);
        } ਨਹੀਂਤਾਂ ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.ਧੱਕੋ_ਪਿੱਠ(selp);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ* ਅਚਲ memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    ਸਥਿਰ ArraySelInfo getArraySelInfo(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        ਵਾਪਸ info;
    }

    ਸਥਿਰ std::ਸਦਿਸ਼<ਪੂਰਨ> arraySelDimSizes(ਅਚਲ ArraySelInfo& info) {
        std::ਸਦਿਸ਼<ਪੂਰਨ> dims;
        dims.reserve(info.m_sels.ਮਾਪ());
        ਲਈ (AstArraySel* ਅਚਲ selp : info.m_sels) {
            Ast_ਗੰਢ_DType* ਅਚਲ dtypep = selp->fromp()->dtypep()->skipRefp();
            ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.ਧੱਕੋ_ਪਿੱਠ(arrayp->declRange().elements());
        }
        ਵਾਪਸ dims;
    }

    ਸਥਿਰ ਪੂਰਨ flatten_ਸੂਚਕਾਂਕ(ਅਚਲ std::ਸਦਿਸ਼<ਪੂਰਨ>& indices, ਅਚਲ std::ਸਦਿਸ਼<ਪੂਰਨ>& dimSizes) {
        UASSERT(indices.ਮਾਪ() == dimSizes.ਮਾਪ(), "Array index and dimension size mismatch");
        ਪੂਰਨ ਸੂਚਕਾਂਕ = 0;
        ਪੂਰਨ stride = 1;
        ਲਈ (ਪੂਰਨ i = static_cast<ਪੂਰਨ>(indices.ਮਾਪ()) - 1; i >= 0; --i) {
            ਸੂਚਕਾਂਕ += indices[i] * stride;
            stride *= dimSizes[i];
        }
        ਵਾਪਸ ਸੂਚਕਾਂਕ;
    }

    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* build_Flatten_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(File_ਸਤਰ* flp, ਅਚਲ ArraySelInfo& info) {
        ਅਚਲ std::ਸਦਿਸ਼<ਪੂਰਨ> dimSizes = arraySelDimSizes(info);
        ਤਾਰਕਿਕ allConst = ਸੱਚ;
        ਲਈ (AstArraySel* ਅਚਲ selp : info.m_sels) {
            ਜੇ (!VN_IS(selp->bitp(), Const)) {
                allConst = ਝੂਠ;
                ਰੋਕੋ;
            }
        }
        ਜੇ (allConst) {
            std::ਸਦਿਸ਼<ਪੂਰਨ> constIndices;
            constIndices.reserve(info.m_sels.ਮਾਪ());
            ਲਈ (AstArraySel* ਅਚਲ selp : info.m_sels) {
                constIndices.ਧੱਕੋ_ਪਿੱਠ(VN_AS(selp->bitp(), Const)->ਮੰਜ਼ਿਲ_SInt());
            }
            ਵਾਪਸ makeConst32(flp, flatten_ਸੂਚਕਾਂਕ(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* resultp = ਖਾਲੀ_ਸੂਚਕ;
        ਪੂਰਨ stride = 1;
        ਲਈ (ਪੂਰਨ i = static_cast<ਪੂਰਨ>(info.m_sels.ਮਾਪ()) - 1; i >= 0; --i) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* termp = info.m_sels[i]->bitp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            ਜੇ (termp->ਚੌੜਾਈ() < 32) termp = ਨਵਾਂ AstExtend{flp, termp, 32};
            ਜੇ (stride != 1) termp = ਨਵਾਂ AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? ਨਵਾਂ Ast_ਜੋੜੋ{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        ਵਾਪਸ resultp;
    }

    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* build_Rhs_ਅੰਕੜੇ_ਵਿਅੰਜਕ(File_ਸਤਰ* flp, ਅਚਲ ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    ਖਾਲੀ finalizeRhsVars() {
        ਲਈ (VarForceInfo& info : m_varInfos) {
            AstVar* ਅਚਲ varp = info.m_varp;
            ਜੇ (info.m_forces.ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;

            AstScope* ਅਚਲ scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_ਸਤਰ* ਅਚਲ flp = varp->fileline();
            ਅਚਲ std::ਸਦਿਸ਼<ForceInfo*> forceps = force_Infos_ਵਿੱਚ_Id_ਕ੍ਰਮ(info);

            ਲਈ (ForceInfo* ਅਚਲ finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* ਅਚਲ rhsVarp = ਨਵਾਂ AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doing_ਸੌਂਪੋ() ? "_VassignRHS" : "__VforceRHS")
                        + std::ਮੰਜ਼ਿਲ_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(ਸੱਚ);
                rhsVarp->sig_ਜਨਤਕ(ਸੱਚ);
                rhsVarp->ਸੈੱਟ_Forced_By_ਕੋਡ();
                varp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(rhsVarp);
                finfo.m_rhsVarVscp = ਨਵਾਂ AstVarScope{flp, scopep, rhsVarp};
                scopep->ਜੋੜੋ_Varsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                //ਸਿਰਾ
                Ast_ਸੌਂਪੋ* ਅਚਲ rhsAssignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                    flp, ਨਵਾਂ AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                ਜੇ (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* ਅਚਲ touchCallp = ਨਵਾਂ AstCMethodHard{
                        flp, ਨਵਾਂ AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtype_ਸੈੱਟ_Void();
                    Ast_ਗੰਢ_Stmt* ਅਚਲ touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* ਅਚਲ alwaysp
                    = ਨਵਾਂ AstAlways{flp, VAlwaysKwd::ALWAYS, ਖਾਲੀ_ਸੂਚਕ, rhsAssignp};
                Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep
                    = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, Ast_Sen_ਵਸਤੂ::Combo{}}};
                Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "force-rhs-update", senTreep};
                activep->sen_ਰੁੱਖ_Storep(activep->sentreep());
                activep->ਜੋੜੋ_Stmtsp(alwaysp);
                scopep->ਜੋੜੋ_Blocksp(activep);
            }

            ਜੇ (info.m_forceRdVscp) {
                ਜੇ (!m_forceRdUpdateBuilt.ਪਾਓ(info.m_varVscp).ਦੂਜਾ) ਜਾਰੀ;
                Ast_ਸਰਗਰਮ* ਅਚਲ ਸਰਗਰਮ_Initp = ਨਵਾਂ Ast_ਸਰਗਰਮ{
                    flp, "force-init",
                    ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, Ast_Sen_ਵਸਤੂ::Static{}}}};
                ਸਰਗਰਮ_Initp->sen_ਰੁੱਖ_Storep(ਸਰਗਰਮ_Initp->sentreep());
                Ast_ਗੰਢ_Stmt* initStmtp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (AstUnpackArrayDType* ਅਚਲ arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ਅਚਲ std::ਸਦਿਸ਼<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    ਅਚਲ ਪੂਰਨ inner_ਚੌੜਾਈ = dims.ਪਿੱਠ()->subDTypep()->skipRefp()->ਚੌੜਾਈ();
                    initStmtp = foreach_Unpacked_ਪੱਤੀ(
                        dims, [&](ਅਚਲ std::ਸਦਿਸ਼<ਪੂਰਨ>& idx, ਪੂਰਨ /*flat*/) -> Ast_ਗੰਢ_Stmt* {
                            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = buildNestedArraySel(
                                flp, ਨਵਾਂ AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsp, makeZeroConst(varp, inner_ਚੌੜਾਈ)};
                        });
                } ਨਹੀਂਤਾਂ {
                    initStmtp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                        flp, ਨਵਾਂ AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->ਚੌੜਾਈ())};
                }
                initStmtp->ਜੋੜੋ_ਅਗਲਾ(createForceRdUpdateStmt(info));
                ਸਰਗਰਮ_Initp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstInitial{flp, initStmtp});
                scopep->ਜੋੜੋ_Blocksp(ਸਰਗਰਮ_Initp);

                Ast_Sen_ਵਸਤੂ* itemsp = ਖਾਲੀ_ਸੂਚਕ;
                ਸਵੈਚਾਲਿਤ ਜੋੜੋ_Sen_ਵਸਤੂ = [&](AstVarScope* vscp) {
                    ਜੇ (!vscp) ਵਾਪਸ;
                    Ast_Sen_ਵਸਤੂ* ਅਚਲ nextp = ਨਵਾਂ Ast_Sen_ਵਸਤੂ{
                        flp, VEdgeType::ET_ਬਦਲਿਆ, ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ}};
                    ਜੇ (itemsp) {
                        itemsp->ਜੋੜੋ_ਅਗਲਾ(nextp);
                    } ਨਹੀਂਤਾਂ {
                        itemsp = nextp;
                    }
                };
                ਜੋੜੋ_Sen_ਵਸਤੂ(info.m_forceEnVscp);
                ਜੋੜੋ_Sen_ਵਸਤੂ(info.m_forceValVscp);
                AstVarRef* ਅਚਲ origSenRefp = ਨਵਾਂ AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                Ast_Sen_ਵਸਤੂ* ਅਚਲ origItemp
                    = ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, VEdgeType::ET_ਬਦਲਿਆ, origSenRefp};
                ਜੇ (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->ਜੋੜੋ_ਅਗਲਾ(origItemp);
                ਲਈ (ForceInfo* ਅਚਲ finfop : forceps) {
                    ਜੇ (!finfop->m_isExternal) ਜੋੜੋ_Sen_ਵਸਤੂ(finfop->m_rhsVarVscp);
                }

                Ast_ਸਰਗਰਮ* ਅਚਲ activep
                    = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "force-rd-update", ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, itemsp}};
                activep->sen_ਰੁੱਖ_Storep(activep->sentreep());
                activep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{flp, VAlwaysKwd::ALWAYS, ਖਾਲੀ_ਸੂਚਕ,
                                                 createForceRdUpdateStmt(info)});
                scopep->ਜੋੜੋ_Blocksp(activep);
            }
        }
    }

    Ast_ਗੰਢ* createRhsUpdatesForWrite(File_ਸਤਰ* flp, AstVar* writtenVarp) ਅਚਲ {
        ਜੇ (!doing_ਸੌਂਪੋ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_rhs_Dep_ਮੰਜ਼ਿਲ_Forces.ਲੱਭੋ(writtenVarp);
        ਜੇ (it == m_rhs_Dep_ਮੰਜ਼ਿਲ_Forces.ਸਿਰਾ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        Ast_ਗੰਢ* headp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ* tailp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਅਚਲ ForceInfo* ਅਚਲ finfop : it->ਦੂਜਾ) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            Ast_ਸੌਂਪੋ* ਅਚਲ updatep
                = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
            ਜੇ (tailp) {
                tailp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(updatep);
            } ਨਹੀਂਤਾਂ {
                headp = updatep;
            }
            tailp = updatep;
        }
        ਵਾਪਸ headp;
    }

    ਅਚਲ ForceInfo& getForceInfo(Ast_ਸੌਂਪੋ_Force* forceStmtp) ਅਚਲ {
        AstVarScope* ਅਚਲ vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        ਅਚਲ VarForceInfo* ਅਚਲ varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        ਸਵੈਚਾਲਿਤ it2 = varInfo->m_forces.ਲੱਭੋ(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.ਸਿਰਾ(), "Force statement not found");
        ਵਾਪਸ it2->ਦੂਜਾ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ selOverlapsAnyForce(ਅਚਲ VarForceInfo& varInfo, ਪੂਰਨ selLsb, ਪੂਰਨ selMsb) {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : varInfo.m_forces) {
            ਜੇ (pair.ਦੂਜਾ.m_rangeLsb <= selMsb && pair.ਦੂਜਾ.m_rangeMsb >= selLsb) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* create_Force_Read_ਵਿਅੰਜਕ(ਅਚਲ VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) ਅਚਲ {
        File_ਸਤਰ* ਅਚਲ flp = originalRefp->fileline();
        AstVarRef* ਅਚਲ refp = originalRefp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        refp->access(VAccess::READ);
        ਵਾਪਸ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   ਖਾਲੀ_ਸੂਚਕ);
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* create_Force_Read_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(ਅਚਲ VarForceInfo& varInfo,
                                                Ast_ਗੰਢ_ਵਿਅੰਜਕ* originalExprp,
                                                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਸੂਚਕਾਂਕ_Exprp) ਅਚਲ {
        File_ਸਤਰ* ਅਚਲ flp = originalExprp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = originalExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        // Must be an LValue to a static variable
        VN_AS(exprp->get_VAccess_ਨਿਸ਼ਾਨਾ_Recurse(), VarRef)->access(VAccess::READ);
        ਵਾਪਸ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_ਸੂਚਕਾਂਕ, exprp, originalExprp,
                                   ਸੂਚਕਾਂਕ_Exprp);
    }

    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* rebuild_Sel_ਰਸਤਾ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* pathp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਧਾਰ_Exprp) {
        ਜੇ (ਅਚਲ AstSel* ਅਚਲ selp = VN_CAST(pathp, Sel)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = rebuild_Sel_ਰਸਤਾ(selp->fromp(), ਅਧਾਰ_Exprp);
            AstSel* ਅਚਲ outp
                = ਨਵਾਂ AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->ਚੌੜਾਈ_Const()};
            outp->dtypeFrom(selp);
            ਵਾਪਸ outp;
        }
        ਵਾਪਸ ਅਧਾਰ_Exprp;
    }
};

// Split deassign concat LHS before converting to release internals.
ਸਥਿਰ ਖਾਲੀ ਵੰਡੋ_Deassign(AstDeassign* nodep) {
    AstConcat* ਅਚਲ concatp = VN_CAST(nodep->lhsp(), Concat);
    ਜੇ (!concatp) ਵਾਪਸ;

    File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
    AstDeassign* ਅਚਲ newLp = ਨਵਾਂ AstDeassign{flp, concatp->lhsp()->unlink_Fr_ਪਿੱਠ()};
    AstDeassign* ਅਚਲ newRp = ਨਵਾਂ AstDeassign{flp, concatp->rhsp()->unlink_Fr_ਪਿੱਠ()};
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ conp = concatp->unlink_Fr_ਪਿੱਠ();
    nodep->replaceWith(newLp);
    newLp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newRp);
    VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    VL_DO_DANGLING(conp->delete_ਰੁੱਖ(), conp);

    ਵੰਡੋ_Deassign(newLp);
    ਵੰਡੋ_Deassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

ਵਰਗ ForceDiscoveryVisitor final : ਜਨਤਕ VNVisitorConst {
    ForceState& m_state;
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Clocked_ਸਰਗਰਮ = ਝੂਠ;

    ਖਾਲੀ buildForceableUnpackedArray(AstVarScope* ਅਚਲ nodep,
                                     AstUnpackArrayDType* ਅਚਲ arrDtypep) {
        AstVar* ਅਚਲ varp = nodep->varp();
        ਅਚਲ std::ਸਦਿਸ਼<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.ਖਾਲੀ_ਥਾਵਾਂ(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ ਪੱਤੀ_Dtypep = dims.ਪਿੱਠ()->subDTypep()->skipRefp();
        ਅਚਲ AstBasicDType* ਅਚਲ innerBasicp = ਪੱਤੀ_Dtypep->basicp();
        ਜੇ (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            ਵਾਪਸ;
        }

        File_ਸਤਰ* ਅਚਲ flp = varp->fileline();
        ਅਚਲ ਪੂਰਨ inner_ਚੌੜਾਈ = ਪੱਤੀ_Dtypep->ਚੌੜਾਈ();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* ਅਚਲ enVscp = info.m_forceEnVscp;
        AstVarScope* ਅਚਲ valVscp = info.m_forceValVscp;

        Ast_Sen_ਵਸਤੂ* ਅਚਲ itemsp = ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, VEdgeType::ET_ਬਦਲਿਆ,
                                                  ਨਵਾਂ AstVarRef{flp, enVscp, VAccess::READ}};
        Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "force-update", ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, itemsp}};
        activep->sen_ਰੁੱਖ_Storep(activep->sentreep());

        Ast_ਗੰਢ_Stmt* ਅਚਲ alwaysBodyHeadp = ForceState::foreach_Unpacked_ਪੱਤੀ(
            dims, [&](ਅਚਲ std::ਸਦਿਸ਼<ਪੂਰਨ>& idx, ਪੂਰਨ flat) -> Ast_ਗੰਢ_Stmt* {
                AstVarRef* ਅਚਲ origRefp = ਨਵਾਂ AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enSelp = ForceState::buildNestedArraySel(
                    flp, ਨਵਾਂ AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valSelp = ForceState::buildNestedArraySel(
                    flp, ਨਵਾਂ AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ forceExprp = ਨਵਾਂ AstOr{
                    flp, ਨਵਾਂ AstAnd{flp, enSelp, valSelp},
                    ਨਵਾਂ AstAnd{flp, ਨਵਾਂ AstNot{flp, enSelp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)}, origSelp}};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsSelp = ForceState::buildNestedArraySel(
                    flp, ਨਵਾਂ AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                Ast_ਸੌਂਪੋ_Force* ਅਚਲ forceAssignp = ਨਵਾਂ Ast_ਸੌਂਪੋ_Force{flp, lhsSelp, forceExprp};
                forceAssignp->user2(ਸੱਚ);

                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsClonep = forceExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                rhsClonep->foreach([varp](AstVarRef* ਅਚਲ r) {
                    ਜੇ (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.ਜੋੜੋ_Force_Assignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/inner_ਚੌੜਾਈ - 1,
                                           /*hasArraySel=*/ਸੱਚ, /*skipRdRefresh=*/ਸੱਚ);
                ਵਾਪਸ forceAssignp;
            });
        activep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{flp, VAlwaysKwd::ALWAYS, ਖਾਲੀ_ਸੂਚਕ, alwaysBodyHeadp});
        nodep->scopep()->ਜੋੜੋ_Blocksp(activep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Force* nodep) override {
        ਜੇ (nodep->user2()) ਵਾਪਸ;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* ਅਚਲ lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* ਅਚਲ forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, ਸੱਚ);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsExprp = nodep->rhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);

        m_state.ਜੋੜੋ_Force_Assignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ* nodep) override {
        ਜੇ (m_state.doing_ਸੌਂਪੋ() && m_ਵਿੱਚ_Clocked_ਸਰਗਰਮ) {
            ਜੇ (AstVarRef* ਅਚਲ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਰਗਰਮ* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Clocked_ਸਰਗਰਮ);
        m_ਵਿੱਚ_Clocked_ਸਰਗਰਮ = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarScope* nodep) override {
        ਜੇ (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            ਜੇ (m_state.doing_ਸੌਂਪੋ()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                ਵਾਪਸ;
            }

            ਜੇ (AstUnpackArrayDType* ਅਚਲ arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                ਵਾਪਸ;
            }

            ਅਚਲ AstBasicDType* ਅਚਲ bdtypep = nodep->varp()->basicp();
            ਜੇ (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* ਅਚਲ varp = nodep->varp();
            File_ਸਤਰ* ਅਚਲ flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* ਅਚਲ enVscp = info.m_forceEnVscp;
            AstVarScope* ਅਚਲ valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            Ast_Sen_ਵਸਤੂ* ਅਚਲ itemsp = ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, VEdgeType::ET_ਬਦਲਿਆ,
                                                      ਨਵਾਂ AstVarRef{flp, enVscp, VAccess::READ}};
            Ast_ਸਰਗਰਮ* ਅਚਲ activep
                = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "force-update", ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, itemsp}};
            activep->sen_ਰੁੱਖ_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* ਅਚਲ enRefp = ਨਵਾਂ AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* ਅਚਲ valRefp = ਨਵਾਂ AstVarRef{flp, valVscp, VAccess::READ};
            ਅਚਲ AstBasicDType* ਅਚਲ basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstOr{
                            flp, ਨਵਾਂ AstAnd{flp, enRefp, valRefp},
                            ਨਵਾਂ AstAnd{flp, ਨਵਾਂ AstNot{flp, enRefp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)}, refp}})
                      : static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstCond{flp, enRefp, valRefp, refp});
            Ast_ਸੌਂਪੋ_Force* ਅਚਲ forceAssignp
                = ਨਵਾਂ Ast_ਸੌਂਪੋ_Force{flp, ਨਵਾਂ AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(ਸੱਚ);
            activep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{flp, VAlwaysKwd::ALWAYS, ਖਾਲੀ_ਸੂਚਕ, forceAssignp});
            nodep->scopep()->ਜੋੜੋ_Blocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsClonep = forceExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            rhsClonep->foreach([varp](AstVarRef* ਅਚਲ refp) {
                ਜੇ (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            ਅਚਲ ਤਾਰਕਿਕ bitwiseVar = ForceState::isBitwiseDType(varp);
            ਅਚਲ ਪੂਰਨ padMsb = bitwiseVar ? (varp->ਚੌੜਾਈ() - 1) : 0;
            ਪੂਰਨ rangeLsb = 0;
            ਪੂਰਨ rangeMsb = padMsb;
            ਜੇ (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.ਜੋੜੋ_Force_Assignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, ਝੂਠ);
        }
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    ਸਪਸ਼ਟ ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_ਅਗਲਾ_Const_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

ਵਰਗ ForceConvertVisitor final : ਜਨਤਕ VNVisitor {
    ForceState& m_state;

    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Force* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp();
        AstVarRef* ਅਚਲ lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ਅਚਲ forcedVarp = lhsVarRefp->varp();

        ਅਚਲ ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        ਅਚਲ ForceState::VarForceInfo* ਅਚਲ varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        Ast_ਸੌਂਪੋ* ਅਚਲ rhsAssignp
            = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};

        Ast_ਸੌਂਪੋ* valAssignp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਸੌਂਪੋ* enAssignp = ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ ਤਾਰਕਿਕ bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        ਜੇ (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਧਾਰ_Exprp = nodep->rhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            ਅਧਾਰ_Exprp->foreach(
                [](AstVarRef* ਅਚਲ refp) { ForceState::markNonReplaceable(refp); });
            ਜੇ (bitwiseForcedVar) {
                ਅਧਾਰ_Exprp = ForceState::zero_Pad_ਮੰਜ਼ਿਲ_ਅਧਾਰ_ਚੌੜਾਈ(ਅਧਾਰ_Exprp, forcedVarp->ਚੌੜਾਈ(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            ਜੇ (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* ਅਚਲ ਨਕਾਬ_Constp = ForceState::make_Range_ਨਕਾਬ_Const(
                    nodep, forcedVarp->ਚੌੜਾਈ(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valReadp
                    = ਨਵਾਂ AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valWritep
                    = ਨਵਾਂ AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notMaskp = ਨਵਾਂ AstNot{flp, ਨਕਾਬ_Constp};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ maskedOldp = ਨਵਾਂ AstAnd{flp, valReadp, notMaskp};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newValp = ਨਵਾਂ AstOr{flp, maskedOldp, ਅਧਾਰ_Exprp};
                valAssignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, valWritep, newValp};

                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enReadp
                    = ਨਵਾਂ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enWritep
                    = ਨਵਾਂ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newEnp
                    = ਨਵਾਂ AstOr{flp, enReadp, ਨਕਾਬ_Constp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                enAssignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, enWritep, newEnp};
            } ਨਹੀਂਤਾਂ {
                AstConst* ਅਚਲ oneConstp = ForceState::make_Range_ਨਕਾਬ_Const(nodep, 1, 0, 0);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsValp = ForceState::cast_ਮੰਜ਼ਿਲ_ਗੰਢ_DType(ਅਧਾਰ_Exprp, forcedVarp);
                valAssignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                    flp, ਨਵਾਂ AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                    flp, ਨਵਾਂ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        ਅਚਲ AstSel* ਅਚਲ selLhsp = VN_CAST(lhsp, Sel);
        ਅਚਲ ਤਾਰਕਿਕ array_ਦੋਆਧਾਰੀ_ਅੰਕ_Sel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_has_ਦੋਆਧਾਰੀ_ਅੰਕ_Sel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->ਚੌੜਾਈ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsDatap = ForceState::build_Rhs_ਅੰਕੜੇ_ਵਿਅੰਜਕ(flp, info);
        AstCExpr* ਅਚਲ rhsAddrp = ਨਵਾਂ AstCExpr{flp};
        rhsAddrp->ਜੋੜੋ("&(");
        rhsAddrp->ਜੋੜੋ(rhsDatap);
        rhsAddrp->ਜੋੜੋ(")");
        AstCMethodHard* ਅਚਲ ਜੋੜੋ_Force_Callp = ਨਵਾਂ AstCMethodHard{
            flp, ਨਵਾਂ AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ਜੋੜੋ,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        ਜੋੜੋ_Force_Callp->ਜੋੜੋ_Pinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        ਜੋੜੋ_Force_Callp->ਜੋੜੋ_Pinsp(rhsAddrp);
        ਜੋੜੋ_Force_Callp->ਜੋੜੋ_Pinsp(
            ForceState::makeConst32(flp, array_ਦੋਆਧਾਰੀ_ਅੰਕ_Sel ? info.m_padLsb : info.m_rangeLsb));
        ਜੇ (array_ਦੋਆਧਾਰੀ_ਅੰਕ_Sel) {
            ਜੋੜੋ_Force_Callp->ਜੋੜੋ_Pinsp(ForceState::makeConst32(flp, info.m_padLsb));
            ਜੋੜੋ_Force_Callp->ਜੋੜੋ_Pinsp(ForceState::makeConst32(flp, info.m_padMsb));
            ਜੋੜੋ_Force_Callp->ਜੋੜੋ_Pinsp(ForceState::makeConst32(flp, selLhsp->fromp()->ਚੌੜਾਈ()));
        }
        ਜੋੜੋ_Force_Callp->dtype_ਸੈੱਟ_Void();
        Ast_ਗੰਢ_Stmt* ਅਚਲ stmtp = ਜੋੜੋ_Force_Callp->makeStmt();

        Ast_ਗੰਢ* tailp = rhsAssignp;
        ਜੇ (valAssignp) {
            tailp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(valAssignp);
            tailp = valAssignp;
        }
        ਜੇ (enAssignp) {
            tailp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(enAssignp);
            tailp = enAssignp;
        }
        tailp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(stmtp);
        ਜੇ (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp();
        AstVarRef* ਅਚਲ lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ਅਚਲ releasedVarp = lhsVarRefp->varp();

        ਅਚਲ ForceState::VarForceInfo* ਅਚਲ varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        ਜੇ (!varInfo) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }
        UASSERT_OBJ(!varInfo->m_forces.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Var info for variable with no forces");

        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();

        ਅਚਲ ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, ਝੂਠ);

        ਅਚਲ AstSel* ਅਚਲ selLhsp = VN_CAST(lhsp, Sel);
        ਅਚਲ ਤਾਰਕਿਕ array_ਦੋਆਧਾਰੀ_ਅੰਕ_Sel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_has_ਦੋਆਧਾਰੀ_ਅੰਕ_Sel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->ਚੌੜਾਈ();
        AstCMethodHard* ਅਚਲ releaseCallp = ਨਵਾਂ AstCMethodHard{
            flp, ਨਵਾਂ AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->ਜੋੜੋ_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        ਜੇ (array_ਦੋਆਧਾਰੀ_ਅੰਕ_Sel) {
            releaseCallp->ਜੋੜੋ_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->ਜੋੜੋ_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtype_ਸੈੱਟ_Void();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_ਗੰਢ_Stmt* ਅਚਲ releasep = releaseCallp->makeStmt();

        Ast_ਸੌਂਪੋ* clearEnp = ਖਾਲੀ_ਸੂਚਕ;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        ਜੇ (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enWritep
                = ਨਵਾਂ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            ਜੇ (ForceState::isBitwiseDType(releasedVarp)) {
                ਅਚਲ ਪੂਰਨ var_ਚੌੜਾਈ = releasedVarp->ਚੌੜਾਈ();
                ਜੇ (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == var_ਚੌੜਾਈ - 1) {
                    clearEnp
                        = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, enWritep, ForceState::makeZeroConst(nodep, var_ਚੌੜਾਈ)};
                } ਨਹੀਂਤਾਂ {
                    // forceEn = forceEn & ~mask(range);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enReadp
                        = ਨਵਾਂ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* ਅਚਲ ਨਕਾਬ_Const = ForceState::make_Range_ਨਕਾਬ_Const(
                        nodep, var_ਚੌੜਾਈ, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newEnp
                        = ਨਵਾਂ AstAnd{flp, enReadp, ਨਵਾਂ AstNot{flp, ਨਕਾਬ_Const}};
                    clearEnp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, enWritep, newEnp};
                }
            } ਨਹੀਂਤਾਂ {
                clearEnp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        ਅਚਲ AstSel* ਅਚਲ selp = VN_CAST(lhsp, Sel);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ basep = selp ? selp->fromp() : lhsp;

        Ast_ਗੰਢ* stmtListp = releasep;
        ਜੇ (clearEnp) {
            clearEnp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        ਅਚਲ ਤਾਰਕਿਕ ਭਰਿਆ_Bitwise_Release
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->ਚੌੜਾਈ() - 1;
        ਜੇ (!releasedVarp->isContinuously()
            && !(m_state.doing_ਸੌਂਪੋ() && m_state.hasClockedWrite(releasedVarp)
                 && ਭਰਿਆ_Bitwise_Release)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            ਅਚਲ ਤਾਰਕਿਕ has_Opaque_ਰਸਤਾ = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* forceReadp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (has_Opaque_ਰਸਤਾ) {
                forceReadp = m_state.create_Force_Read_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } ਨਹੀਂਤਾਂ ਜੇ (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.create_Force_Read_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(
                    *varInfo, basep,
                    ForceState::build_Flatten_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(flp, rangeInfo.m_arrayInfo));
                ਜੇ (selp) { forceReadp = ForceState::rebuild_Sel_ਰਸਤਾ(lhsp, forceReadp); }
            } ਨਹੀਂਤਾਂ {
                forceReadp
                    = selp ? ForceState::rebuild_Sel_ਰਸਤਾ(
                                 lhsp, m_state.create_Force_Read_ਵਿਅੰਜਕ(*varInfo, lhsVarRefp))
                           : m_state.create_Force_Read_ਵਿਅੰਜਕ(*varInfo, lhsVarRefp);
            }
            Ast_ਸੌਂਪੋ* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), forceReadp};
            assignp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(stmtListp);
            stmtListp = assignp;
        }

        ਜੇ (varInfo->m_forceRdVscp) stmtListp->ਜੋੜੋ_ਅਗਲਾ(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_ਅਗਲਾ_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

ਵਰਗ ForceReplaceVisitor final : ਜਨਤਕ VNVisitor {
    ਅਚਲ ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_ਗੰਢ_Stmt* m_stmtp = ਖਾਲੀ_ਸੂਚਕ;
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Logic = ਝੂਠ;

    ਖਾਲੀ iterateLogic(Ast_ਗੰਢ* nodep) {
        VL_RESTORER(m_ਵਿੱਚ_Logic);
        m_ਵਿੱਚ_Logic = ਸੱਚ;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        ਜੇ (AstVarRef* ਅਚਲ lhsp = VN_CAST(nodep->lhsp()->ਅਧਾਰ_Fromp(ਸੱਚ), VarRef)) {
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(updatep);
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Cont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate_And_ਅਗਲਾ_Null(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstDeassign*) override {}
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override { iterateLogic(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override { iterateLogic(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        ਜੇ (nodep->keyword() == VAlwaysKwd::CONT_ਸੌਂਪੋ) {
            iterateChildren(nodep);
            ਵਾਪਸ;
        }
        iterateLogic(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਵਸਤੂ* nodep) override { iterateLogic(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* ਅਚਲ refp = VN_CAST(nodep->fromp(), VarRef);
        ਜੇ (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            ਦੇਖੋ(static_cast<Ast_ਗੰਢ*>(nodep));
            ਵਾਪਸ;
        }

        AstVar* ਅਚਲ varp = refp->varp();
        ਅਚਲ ForceState::VarForceInfo* ਅਚਲ varInfo = m_state.getVarInfo(refp->varScopep());
        ਜੇ (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.ਖਾਲੀ_ਥਾਵਾਂ()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            ਦੇਖੋ(static_cast<Ast_ਗੰਢ*>(nodep));
            ਵਾਪਸ;
        }

        ਜੇ (ਅਚਲ AstConst* ਅਚਲ lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            ਅਚਲ ਪੂਰਨ selLsb = lsbConstp->ਮੰਜ਼ਿਲ_SInt();
            ਅਚਲ ਪੂਰਨ selMsb = selLsb + nodep->ਚੌੜਾਈ() - 1;
            ਜੇ (!varp->is_Sig_ਜਨਤਕ()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                ਦੇਖੋ(static_cast<Ast_ਗੰਢ*>(nodep));
                ਵਾਪਸ;
            }
        }

        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* ਅਚਲ refClonep = refp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* ਅਚਲ callp = ਨਵਾਂ AstCMethodHard{
            flp, ਨਵਾਂ AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->ਚੌੜਾਈ())};
        callp->ਜੋੜੋ_Pinsp(refClonep);
        callp->ਜੋੜੋ_Pinsp(nodep->lsbp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
        callp->ਜੋੜੋ_Pinsp(ForceState::makeConst32(flp, nodep->ਚੌੜਾਈ()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstArraySel* nodep) override {
        ਜੇ (ਅਚਲ AstArraySel* ਅਚਲ ਪਿੱਠ_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            ਜੇ (ਪਿੱਠ_Selp->fromp() == nodep) {
                iterateChildren(nodep);
                ਵਾਪਸ;
            }
        }

        Ast_ਗੰਢ* ਅਚਲ basep = nodep->ਅਧਾਰ_Fromp(ਸੱਚ);
        AstVarRef* ਅਚਲ ਅਧਾਰ_Refp = VN_CAST(basep, VarRef);
        ਜੇ (!ਅਧਾਰ_Refp) {
            iterateChildren(nodep);
            ਵਾਪਸ;
        }
        AstVar* ਅਚਲ varp = ਅਧਾਰ_Refp->varp();
        ਅਚਲ ForceState::VarForceInfo* ਅਚਲ varInfo = m_state.getVarInfo(ਅਧਾਰ_Refp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        ਜੇ (m_state.isNotReplaceable(ਅਧਾਰ_Refp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            ਵਾਪਸ;
        }

        ਜੇ (!ਅਧਾਰ_Refp->access().isReadOnly()) {
            iterateChildren(nodep);
            ਵਾਪਸ;
        }
        ਜੇ (m_state.doing_ਸੌਂਪੋ() && varInfo->m_forceRdVscp) {
            ਅਧਾਰ_Refp->varp(varInfo->m_forceRdVscp->varp());
            ਅਧਾਰ_Refp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            ਵਾਪਸ;
        }
        ਅਚਲ ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        ਲਈ (AstArraySel* ਅਚਲ selp : arrayInfo.m_sels) iterate_And_ਅਗਲਾ_Null(selp->bitp());
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸੂਚਕਾਂਕ_Exprp
            = ForceState::build_Flatten_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(nodep->fileline(), arrayInfo);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ readExprp
            = m_state.create_Force_Read_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(*varInfo, nodep, ਸੂਚਕਾਂਕ_Exprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਜੇ (m_state.isNotReplaceable(nodep)) ਵਾਪਸ;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        ਜੇ (ਅਚਲ AstArraySel* ਅਚਲ ਪਿੱਠ_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            ਜੇ (ਪਿੱਠ_Selp->fromp() == nodep) ਵਾਪਸ;
        }

        ਅਚਲ ForceState::VarForceInfo* ਅਚਲ varInfo = m_state.getVarInfo(nodep->varScopep());
        ਜੇ (!varInfo) ਵਾਪਸ;

        ਜੇ (varInfo->m_forceRdVscp) {
            ਜੇ (nodep->access().isRW()) {
                ਜੇ (m_ਵਿੱਚ_Logic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                ਵਾਪਸ;
            }
            ਜੇ (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ readp
                    = m_state.create_Force_Rd_ਵਿਅੰਜਕ(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ;
            }
            ਜੇ (m_ਵਿੱਚ_Logic && m_stmtp) {
                m_stmtp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(m_state.createForceRdUpdateStmt(*varInfo));
            }
            ਵਾਪਸ;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        ਜੇ (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            ਵਾਪਸ;
        }

        UASSERT_OBJ(!varInfo->m_forces.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Var info for variable with no forces");

        ਜੇ (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ readExprp = m_state.create_Force_Read_ਵਿਅੰਜਕ(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (ForceState::is_Outermost_Opaque_ਰਸਤਾ_Selector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = VN_AS(nodep, ਗੰਢ_ਵਿਅੰਜਕ);
            Ast_ਗੰਢ* ਅਚਲ basep = exprp->ਅਧਾਰ_Fromp(ਸੱਚ);
            AstVarRef* ਅਚਲ ਅਧਾਰ_Refp = VN_CAST(basep, VarRef);
            ਜੇ (ਅਧਾਰ_Refp) {
                AstVar* ਅਚਲ varp = ਅਧਾਰ_Refp->varp();
                ਜੇ (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    ਅਚਲ ForceState::VarForceInfo* ਅਚਲ varInfo
                        = m_state.getVarInfo(ਅਧਾਰ_Refp->varScopep());
                    ਜੇ (!m_state.isNotReplaceable(ਅਧਾਰ_Refp) && varInfo) {
                        ਅਚਲ ਪੂਰਨ force_ਰਸਤਾ_ਸੂਚਕਾਂਕ = varInfo->ਲੱਭੋ_Force_ਰਸਤਾ_ਸੂਚਕਾਂਕ(exprp);
                        ਜੇ (force_ਰਸਤਾ_ਸੂਚਕਾਂਕ >= 0) {
                            ਜੇ (!ਅਧਾਰ_Refp->access().isReadOnly()) ਵਾਪਸ;
                            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ readExprp = m_state.create_Force_Read_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), force_ਰਸਤਾ_ਸੂਚਕਾਂਕ));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                            ਵਾਪਸ;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

ਜਨਤਕ:
    ਸਪਸ਼ਟ ForceReplaceVisitor(AstNetlist* nodep, ਅਚਲ ForceState& state)
        : m_state{state} {
        iterate_And_ਅਗਲਾ_Null(nodep->modulesp());
    }
    ~ForceReplaceVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Non-overlapping force sels", m_nonOverlappingForceSels);
    }
};
//######################################################################
//

//######################################################################
// V3Force - Main entry point

ਸਥਿਰ ਖਾਲੀ forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::Protected_Var_Ref_ਸੈੱਟ& permanentlyProtected,
                         ForceState::Built_Var_Scope_ਸੈੱਟ& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    ਜੇ (!v3Global.has_Forceable_ਸੰਕੇਤ()) ਵਾਪਸ;
    ForceState state{ਝੂਠ, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_ਰੁੱਖ("force", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

ਸਥਿਰ ਖਾਲੀ ਸੌਂਪੋ_All_Impl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::Protected_Var_Ref_ਸੈੱਟ& permanentlyProtected,
                          ForceState::Built_Var_Scope_ਸੈੱਟ& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    ਜੇ (!v3Global.has_ਸੌਂਪੋ_Deassign()) ਵਾਪਸ;

    std::ਸਦਿਸ਼<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.ਧੱਕੋ_ਪਿੱਠ(deassignp); });
    ਲਈ (AstDeassign* ਅਚਲ deassignp : deassignps) ਵੰਡੋ_Deassign(deassignp);

    std::ਸਦਿਸ਼<Ast_ਸੌਂਪੋ_Cont*> ਸੌਂਪੋ_Contps;
    deassignps.clear();
    nodep->foreach([&](Ast_ਗੰਢ_Stmt* nodep) {
        ਜੇ (Ast_ਸੌਂਪੋ_Cont* ਅਚਲ ਸੌਂਪੋ_Contp = VN_CAST(nodep, ਸੌਂਪੋ_Cont)) {
            ਸੌਂਪੋ_Contps.ਧੱਕੋ_ਪਿੱਠ(ਸੌਂਪੋ_Contp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstDeassign* ਅਚਲ deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.ਧੱਕੋ_ਪਿੱਠ(deassignp);
        }
    });

    ਲਈ (Ast_ਸੌਂਪੋ_Cont* ਅਚਲ assignp : ਸੌਂਪੋ_Contps) {
        assignp->replaceWith(ਨਵਾਂ Ast_ਸੌਂਪੋ_Force{assignp->fileline(),
                                                assignp->lhsp()->unlink_Fr_ਪਿੱਠ(),
                                                assignp->rhsp()->unlink_Fr_ਪਿੱਠ()});
        VL_DO_DANGLING(assignp->delete_ਰੁੱਖ(), assignp);
    }
    ਲਈ (AstDeassign* ਅਚਲ deassignp : deassignps) {
        deassignp->replaceWith(
            ਨਵਾਂ AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ)});
        VL_DO_DANGLING(deassignp->delete_ਰੁੱਖ(), deassignp);
    }
    ForceState state{ਸੱਚ, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_ਰੁੱਖ("assign-deassign", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

ਖਾਲੀ V3Force::force_And_ਸੌਂਪੋ_All(AstNetlist* nodep) {
    ਅਚਲ VNUser3_ਵਿੱਚ_Use user3_ਵਿੱਚ_Use;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::Protected_Var_Ref_ਸੈੱਟ permanentlyProtected;
    ForceState::Built_Var_Scope_ਸੈੱਟ forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    ਸੌਂਪੋ_All_Impl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
