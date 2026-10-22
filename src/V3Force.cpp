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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Force.h"

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

വർഗ്ഗം ForceState final {
പൊതു:
    ഘടന ForceRange VL_NOT_FINAL {
        പൂർണ്ണസംഖ്യ m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        പൂർണ്ണസംഖ്യ m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        പൂർണ്ണസംഖ്യ m_padLsb = 0;  // Bit positions for RHS padding
        പൂർണ്ണസംഖ്യ m_padMsb = 0;  // Bit positions for RHS padding
    };

    ഘടന ForceInfo final : ForceRange {
        // MEMBERS
        പൂർണ്ണസംഖ്യ m_forceId = 0;  // Unique (per signal) variable of this force assignment
        തർക്കം m_hasArraySel = അസത്യം;  // If this has an array select on LHS
        തർക്കം m_isExternal = അസത്യം;  // Synthetic force for the public force controls
        തർക്കം m_skipRdRefresh = അസത്യം;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = nullptr;  // Scope of the var containing RHSID
        Ast_കണ്ണി_വ്യഞ്ജകം* m_rhsExprp = nullptr;  // Expression on RHS of this force assignment

        ForceInfo() = സ്ഥിരസ്ഥിതി;
        ForceInfo(പൂർണ്ണസംഖ്യ rangeLsb, പൂർണ്ണസംഖ്യ rangeMsb, പൂർണ്ണസംഖ്യ padLsb, പൂർണ്ണസംഖ്യ padMsb, പൂർണ്ണസംഖ്യ forceId,
                  തർക്കം hasArraySel, AstVarScope* rhsVarVscp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsExprp)
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

    ഘടന VarForceInfo final {
        AstVarScope* m_forceVecVscp = nullptr;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = nullptr;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = nullptr;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = nullptr;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = nullptr;  // Scope of the forced (original) variable
        AstVar* m_varp = nullptr;  // The forced (original) variable
        AstScope* m_scopep = nullptr;  // Scope containing the forced variable
        std::unordered_map<AstAssignForce*, ForceInfo> m_forces;
        std::unordered_map<string, പൂർണ്ണസംഖ്യ> m_forcePathToIndex;
        പൂർണ്ണസംഖ്യ m_nextForcePathIndex = 1;  // Start at 1 so 0 can be the base path (whole signal)

    സ്വകാര്യ:
        സ്ഥിതം ശൂന്യം build_Force_Path_താക്കോൽ_Recurse(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, string& out) {
            എങ്കിൽ (VN_IS(nodep, VarRef)) തിരികെ;
            എങ്കിൽ (സ്ഥിരം AstSel* സ്ഥിരം selp = VN_CAST(nodep, Sel)) {
                build_Force_Path_താക്കോൽ_Recurse(selp->fromp(), out);
                out += "|S" + cvtToStr(selp->lsbConst()) + ":" + cvtToStr(selp->widthConst());
                തിരികെ;
            }
            എങ്കിൽ (AstStructSel* സ്ഥിരം selp = VN_CAST(nodep, StructSel)) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = selp->fromp();
                build_Force_Path_താക്കോൽ_Recurse(fromp, out);
                out += "|T" + selp->name();
                തിരികെ;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        സ്ഥിതം string force_Path_താക്കോൽ(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
            string out;
            build_Force_Path_താക്കോൽ_Recurse(nodep, out);
            തിരികെ out;
        }

    പൊതു:
        പൂർണ്ണസംഖ്യ getOrCreateForcePathIndex(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
            സ്ഥിരം auto pair
                = m_forcePathToIndex.emplace(force_Path_താക്കോൽ(nodep), m_nextForcePathIndex);
            എങ്കിൽ (pair.second) ++m_nextForcePathIndex;
            തിരികെ pair.first->second;
        }

        പൂർണ്ണസംഖ്യ findForcePathIndex(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) സ്ഥിരം {
            സ്ഥിരം auto it = m_forcePathToIndex.find(force_Path_താക്കോൽ(nodep));
            തിരികെ it != m_forcePathToIndex.end() ? it->second : -1;
        }
    };

    ഘടന ForceHelperVars final {
        AstVar* m_rdVarp = nullptr;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = nullptr;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = nullptr;  // The __VforceVal (force value) variable
    };

    ഘടന ArraySelInfo final {
        std::vector<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        തർക്കം m_hasBitSel = അസത്യം;  // Bit-select was found along the path too
    };

    ഘടന ForceRangeInfo final : ForceRange {
        തർക്കം m_hasArraySel = അസത്യം;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

സ്വകാര്യ:
    using ScopeVarCache = std::unordered_map<സ്ഥിരം AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    സ്ഥിരം VNUser1InUse m_user1InUse;
    സ്ഥിരം VNUser2InUse m_user2InUse;

പൊതു:
    using ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    using ProtectedVarRefSet = std::unordered_set<സ്ഥിരം AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    using BuiltVarScopeSet = std::unordered_set<സ്ഥിരം AstVarScope*>;

സ്വകാര്യ:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::vector<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, പൂർണ്ണസംഖ്യ> m_varToId;
    std::unordered_set<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::vector<ForceInfo*>> m_rhsDepToForces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    തർക്കം m_doingAssign = അസത്യം;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    ProtectedVarRefSet& m_permanentlyProtected;
    BuiltVarScopeSet& m_forceRdUpdateBuilt;

പൊതു:
    ForceState(തർക്കം doingAssign, ForceHelperVarsByVar& forceHelperVarsByVar,
               ProtectedVarRefSet& permanentlyProtected, BuiltVarScopeSet& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doingAssign{doingAssign}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    സ്ഥിതം AstConst* makeZeroConst(Ast_കണ്ണി* nodep, പൂർണ്ണസംഖ്യ width) {
        V3_സംഖ്യ zero{nodep, width};
        zero.setAllBits0();
        തിരികെ പുതിയ AstConst{nodep->fileline(), zero};
    }

    സ്ഥിതം AstConst* makeConst32(File_വരി* flp, പൂർണ്ണസംഖ്യ മൂല്യം) {
        തിരികെ പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, 32, static_cast<uint32_t>(മൂല്യം)};
    }

    സ്ഥിതം AstConst* makeRangeMaskConst(Ast_കണ്ണി* nodep, പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ lsb, പൂർണ്ണസംഖ്യ msb) {
        V3_സംഖ്യ mask{nodep, width};
        mask.setAllBits0();
        വേണ്ടി (പൂർണ്ണസംഖ്യ bit = lsb; bit <= msb; ++bit) mask.setBit(bit, 1);
        തിരികെ പുതിയ AstConst{nodep->fileline(), mask};
    }

    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* zeroPadToBaseWidth(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, പൂർണ്ണസംഖ്യ baseWidth, പൂർണ്ണസംഖ്യ padLsb,
                                           പൂർണ്ണസംഖ്യ padMsb) {
        എങ്കിൽ (baseWidth <= 0) തിരികെ exprp;
        സ്ഥിരം പൂർണ്ണസംഖ്യ താഴെയുള്ള_Pad = padLsb;
        സ്ഥിരം പൂർണ്ണസംഖ്യ മുകളിലെ_Pad = baseWidth - (padMsb + 1);
        എങ്കിൽ (താഴെയുള്ള_Pad > 0) {
            exprp = പുതിയ AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, താഴെയുള്ള_Pad)};
        }
        എങ്കിൽ (മുകളിലെ_Pad > 0) {
            exprp = പുതിയ AstConcat{exprp->fileline(), makeZeroConst(exprp, മുകളിലെ_Pad), exprp};
        }
        തിരികെ exprp;
    }

    സ്ഥിതം തർക്കം isUnpackedArrayDType(സ്ഥിരം Ast_കണ്ണി_DType* dtypep) {
        തിരികെ VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    സ്ഥിതം തർക്കം isBitwiseDType(സ്ഥിരം Ast_കണ്ണി* nodep) {
        സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = nodep->dtypep()->skipRefp()->basicp();
        തിരികെ basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* cast_To_കണ്ണി_DType(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി* dtypeFromp) {
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = dtypeFromp->dtypep()->skipRefp();
        സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = dtypep->basicp();
        എങ്കിൽ (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            തിരികെ exprp;
        }
        തിരികെ പുതിയ AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    സ്ഥിതം ശൂന്യം markNonReplaceable(AstVarRef* സ്ഥിരം nodep) { nodep->user1SetOnce(); }
    തർക്കം isNotReplaceable(സ്ഥിരം AstVarRef* സ്ഥിരം nodep) സ്ഥിരം {
        തിരികെ nodep->user1() || m_permanentlyProtected.എണ്ണം(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    ശൂന്യം markPermanentlyProtected(AstVarRef* സ്ഥിരം nodep) സ്ഥിരം {
        markNonReplaceable(nodep);
        m_permanentlyProtected.തിരുകുക(nodep);
    }

    സ്ഥിതം std::vector<ForceInfo*> forceInfosInIdOrder(VarForceInfo& info) {
        std::vector<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        വേണ്ടി (auto& it : info.m_forces) forceps.തള്ളുക_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](സ്ഥിരം ForceInfo* ap, സ്ഥിരം ForceInfo* bp) {
            തിരികെ ap->m_forceId < bp->m_forceId;
        });
        തിരികെ forceps;
    }

    സ്ഥിതം std::vector<സ്ഥിരം ForceInfo*> forceInfosInIdOrder(സ്ഥിരം VarForceInfo& info) {
        std::vector<സ്ഥിരം ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        വേണ്ടി (സ്ഥിരം auto& it : info.m_forces) forceps.തള്ളുക_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](സ്ഥിരം ForceInfo* ap, സ്ഥിരം ForceInfo* bp) {
            തിരികെ ap->m_forceId < bp->m_forceId;
        });
        തിരികെ forceps;
    }

    സ്ഥിതം തർക്കം isOpaquePathSelector(സ്ഥിരം Ast_കണ്ണി* nodep) {
        തിരികെ VN_IS(nodep, Sel) || VN_IS(nodep, കണ്ണി_Sel) || VN_IS(nodep, StructSel);
    }

    സ്ഥിതം തർക്കം isOutermostOpaquePathSelector(സ്ഥിരം Ast_കണ്ണി* nodep) {
        എങ്കിൽ (!isOpaquePathSelector(nodep)) തിരികെ അസത്യം;
        സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം backp = nodep->backp();
        തിരികെ !backp || !isOpaquePathSelector(backp);
    }

    സ്ഥിതം AstVarRef* getOneVarRef(Ast_കണ്ണി_വ്യഞ്ജകം* forceStmtp) {
        Ast_കണ്ണി* സ്ഥിരം basep = forceStmtp->baseFromp(സത്യം);
        എങ്കിൽ (AstSampled* sampledp = VN_CAST(basep, Sampled))
            എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* exprp = VN_CAST(sampledp->exprp(), കണ്ണി_വ്യഞ്ജകം))
                തിരികെ getOneVarRef(exprp);
        AstVarRef* സ്ഥിരം varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        തിരികെ varRefp;
    }

    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* buildNestedArraySel(File_വരി* flp, Ast_കണ്ണി_വ്യഞ്ജകം* fromp,
                                            സ്ഥിരം std::vector<പൂർണ്ണസംഖ്യ>& indicies) {
        Ast_കണ്ണി_വ്യഞ്ജകം* curp = fromp;
        വേണ്ടി (സ്ഥിരം പൂർണ്ണസംഖ്യ idx : indicies) curp = പുതിയ AstArraySel{flp, curp, idx};
        തിരികെ curp;
    }

    template <typename Fn>
    സ്ഥിതം Ast_കണ്ണി_Stmt* foreachUnpackedLeaf(സ്ഥിരം std::vector<AstUnpackArrayDType*>& dims,
                                            Fn buildLeaf) {
        Ast_കണ്ണി_Stmt* headp = nullptr;
        Ast_കണ്ണി_Stmt* tailp = nullptr;
        എങ്കിൽ (dims.empty()) തിരികെ nullptr;
        പൂർണ്ണസംഖ്യ total = 1;
        വേണ്ടി (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം d : dims) total *= d->elementsConst();
        എങ്കിൽ (total <= 0) തിരികെ nullptr;
        std::vector<പൂർണ്ണസംഖ്യ> idx(dims.size(), 0);
        വേണ്ടി (പൂർണ്ണസംഖ്യ flat = 0; flat < total; ++flat) {
            Ast_കണ്ണി_Stmt* സ്ഥിരം stmtp = buildLeaf(idx, flat);
            എങ്കിൽ (!headp) {
                headp = stmtp;
            } അല്ലെങ്കിൽ {
                tailp->addNext(stmtp);
            }
            tailp = stmtp;
            വേണ്ടി (പൂർണ്ണസംഖ്യ d = static_cast<പൂർണ്ണസംഖ്യ>(dims.size()) - 1; d >= 0; --d) {
                എങ്കിൽ (++idx[d] < dims[d]->elementsConst()) നിർത്തുക;
                idx[d] = 0;
            }
        }
        തിരികെ headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, AstVar* varp,
                                     തർക്കം requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.empty();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->width() - 1) : 0;

        എങ്കിൽ (സ്ഥിരം AstSel* സ്ഥിരം outerSelp = VN_CAST(lhsp, Sel)) {
            പൂർണ്ണസംഖ്യ totalLsb = 0;
            വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* curp = lhsp; സ്ഥിരം AstSel* സ്ഥിരം selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                എങ്കിൽ (requireConstRangeSelect) {
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
        എങ്കിൽ (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::vector<പൂർണ്ണസംഖ്യ> indices;
            indices.reserve(info.m_arrayInfo.m_sels.size());
            വേണ്ടി (AstArraySel* സ്ഥിരം selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.തള്ളുക_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            info.m_rangeLsb = flattenIndex(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } അല്ലെങ്കിൽ എങ്കിൽ (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        എങ്കിൽ (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            സ്ഥിരം പൂർണ്ണസംഖ്യ index = varInfo.getOrCreateForcePathIndex(lhsp);
            info.m_rangeLsb = index;
            info.m_rangeMsb = index;
        }
        തിരികെ info;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* add_Rhs_മൂല്യം_Reads(സ്ഥിരം VarForceInfo& varInfo, Ast_കണ്ണി_വ്യഞ്ജകം* exprp) സ്ഥിരം {
        എങ്കിൽ (!doingAssign()) തിരികെ exprp;

        സ്ഥിരം std::vector<സ്ഥിരം ForceInfo*> forceps = forceInfosInIdOrder(varInfo);
        എങ്കിൽ (forceps.empty()) തിരികെ exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* സ്ഥിരം cexprp = പുതിയ AstCExpr{exprp->fileline(), AstCExpr::Pure{}};
        cexprp->dtypeFrom(exprp);
        cexprp->add("(");
        വേണ്ടി (സ്ഥിരം ForceInfo* സ്ഥിരം finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* സ്ഥിരം refp
                = പുതിയ AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->add("(void)(");
            cexprp->add(refp);
            cexprp->add("), ");
        }
        cexprp->add(exprp);
        cexprp->add(")");
        തിരികെ cexprp;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* createForceReadCall(സ്ഥിരം VarForceInfo& varInfo, File_വരി* flp, VCMethod method,
                                     Ast_കണ്ണി_വ്യഞ്ജകം* originalExprp, Ast_കണ്ണി* dtypeFromp,
                                     Ast_കണ്ണി_വ്യഞ്ജകം* indexExprp) സ്ഥിരം {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം origValp
            = add_Rhs_മൂല്യം_Reads(varInfo, cast_To_കണ്ണി_DType(originalExprp, dtypeFromp));

        AstCMethodHard* സ്ഥിരം callp = പുതിയ AstCMethodHard{
            flp, പുതിയ AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, method, origValp};
        എങ്കിൽ (indexExprp) callp->addPinsp(indexExprp);
        callp->dtypeFrom(dtypeFromp);
        തിരികെ callp;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* createForceRdExpression(സ്ഥിരം VarForceInfo& varInfo, File_വരി* flp) സ്ഥിരം {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* സ്ഥിരം varp = varInfo.m_varVscp->varp();
        എങ്കിൽ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            എങ്കിൽ (doingAssign()) { തിരികെ പുതിയ AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, സത്യം);
            AstVarRef* സ്ഥിരം baseRefp = പുതിയ AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(baseRefp);
            തിരികെ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, baseRefp, varp,
                                       nullptr);
        }
        AstVarRef* സ്ഥിരം baseRefp = പുതിയ AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(baseRefp);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enRefp = പുതിയ AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valRefp = പുതിയ AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        എങ്കിൽ (isBitwiseDType(varp)) {
            തിരികെ പുതിയ AstOr{
                flp, പുതിയ AstAnd{flp, enRefp, valRefp},
                പുതിയ AstAnd{flp, പുതിയ AstNot{flp, enRefp->clone_വൃക്ഷം_Pure(അസത്യം)}, baseRefp}};
        }
        തിരികെ പുതിയ AstCond{flp, enRefp, valRefp, baseRefp};
    }

    Ast_കണ്ണി_Stmt* createForceRdUpdateStmt(സ്ഥിരം VarForceInfo& varInfo) സ്ഥിരം {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* സ്ഥിരം varp = varInfo.m_varVscp->varp();
        എങ്കിൽ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            തിരികെ createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_വരി* സ്ഥിരം flp = varInfo.m_varVscp->fileline();
        തിരികെ പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             createForceRdExpression(varInfo, flp)};
    }

    Ast_കണ്ണി_Stmt* createForceRdUpdateStmtUnpacked(സ്ഥിരം VarForceInfo& varInfo) സ്ഥിരം {
        File_വരി* സ്ഥിരം flp = varInfo.m_varVscp->fileline();
        AstVar* സ്ഥിരം varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* സ്ഥിരം arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        സ്ഥിരം std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_കണ്ണി_Stmt* സ്ഥിരം stmtsp = foreachUnpackedLeaf(
            dims, [&](സ്ഥിരം std::vector<പൂർണ്ണസംഖ്യ>& idx, പൂർണ്ണസംഖ്യ /*flat*/) -> Ast_കണ്ണി_Stmt* {
                AstVarRef* സ്ഥിരം baseRefp = പുതിയ AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(baseRefp);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം baseSelp = buildNestedArraySel(flp, baseRefp, idx);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enSelp = buildNestedArraySel(
                    flp, പുതിയ AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valSelp = buildNestedArraySel(
                    flp, പുതിയ AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം readExprp = പുതിയ AstOr{
                    flp, പുതിയ AstAnd{flp, enSelp, valSelp},
                    പുതിയ AstAnd{flp, പുതിയ AstNot{flp, enSelp->clone_വൃക്ഷം_Pure(അസത്യം)}, baseSelp}};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rdLhsSelp = buildNestedArraySel(
                    flp, പുതിയ AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                തിരികെ പുതിയ AstAssign{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        വേണ്ടി (സ്ഥിരം ForceInfo* സ്ഥിരം finfop : forceInfosInIdOrder(varInfo)) {
            എങ്കിൽ (!finfop->m_isExternal) തുടരുക;
            stmtsp->addNext(പുതിയ AstAssign{flp,
                                          പുതിയ AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_വൃക്ഷം_Pure(അസത്യം)});
        }
        തിരികെ stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        സ്ഥിരം auto it = m_varToId.find(vscp);
        എങ്കിൽ (it != m_varToId.end()) തിരികെ m_varInfos[it->second];

        m_varToId.emplace(vscp, m_varInfos.size());
        m_varInfos.emplace_back();
        VarForceInfo& info = m_varInfos.back();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* സ്ഥിരം varp = info.m_varp;
        എങ്കിൽ (!varp->isForceable()) തിരികെ info;

        File_വരി* സ്ഥിരം flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        സ്ഥിരം തർക്കം helperVarsBuilt = helperVars.m_rdVarp != nullptr;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != nullptr)
                        && helperVarsBuilt == (helperVars.m_valVarp != nullptr),
                    varp, "Incomplete force helper set");
        എങ്കിൽ (!helperVarsBuilt) {
            സ്ഥിരം തർക്കം unpacked = isUnpackedArrayDType(varp->dtypep());
            സ്ഥിരം VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_കണ്ണി_DType* സ്ഥിരം enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
            helperVars.m_rdVarp
                = പുതിയ AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sigPublic(സത്യം);
            helperVars.m_enVarp
                = പുതിയ AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(സത്യം);
            helperVars.m_valVarp
                = പുതിയ AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(സത്യം);
            varp->addNextHere(helperVars.m_rdVarp);
            varp->addNextHere(helperVars.m_enVarp);
            varp->addNextHere(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = findScopeVar(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = findScopeVar(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = findScopeVar(info.m_scopep, helperVars.m_valVarp);
        എങ്കിൽ (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } അല്ലെങ്കിൽ {
            info.m_forceRdVscp = പുതിയ AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = പുതിയ AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = പുതിയ AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->addVarsp(info.m_forceRdVscp);
            info.m_scopep->addVarsp(info.m_forceEnVscp);
            info.m_scopep->addVarsp(info.m_forceValVscp);
        }
        തിരികെ info;
    }

    ശൂന്യം markClockedWrite(AstVar* varp) { m_clockedWrites.തിരുകുക(varp); }
    തർക്കം hasClockedWrite(AstVar* varp) സ്ഥിരം { തിരികെ m_clockedWrites.എണ്ണം(varp); }

    തർക്കം doingAssign() സ്ഥിരം { തിരികെ m_doingAssign; }

    സ്ഥിരം VarForceInfo* getVarInfo(AstVarScope* vscp) സ്ഥിരം {
        സ്ഥിരം auto it = m_varToId.find(vscp);
        തിരികെ it != m_varToId.end() ? &m_varInfos[it->second] : nullptr;
    }

    AstVarScope* findScopeVar(AstScope* scopep, സ്ഥിരം AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        എങ്കിൽ (cache.empty()) {
            വേണ്ടി (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        സ്ഥിരം auto it = cache.find(varp);
        തിരികെ it != cache.end() ? it->second : nullptr;
    }
    ശൂന്യം addForceAssignment(AstVar* varp, AstVarScope* vscp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsExprp,
                            AstAssignForce* forceStmtp, പൂർണ്ണസംഖ്യ rangeLsb, പൂർണ്ണസംഖ്യ rangeMsb, പൂർണ്ണസംഖ്യ padLsb,
                            പൂർണ്ണസംഖ്യ padMsb, തർക്കം hasArraySel, തർക്കം skipRdRefresh = അസത്യം) {
        v3Global.setUsesForce();
        varp->setForcedByCode();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        സ്ഥിരം പൂർണ്ണസംഖ്യ forceId = info.m_forces.size();
        File_വരി* സ്ഥിരം flp = varp->fileline();
        AstScope* സ്ഥിരം scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        എങ്കിൽ (!info.m_forceVecVscp) {
            AstCDType* സ്ഥിരം forceVecDtypep = പുതിയ AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->addTypesp(forceVecDtypep);

            AstVar* സ്ഥിരം forceVecVarp
                = പുതിയ AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doingAssign ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(അസത്യം);
            forceVecVarp->isInternal(സത്യം);
            varp->addNextHere(forceVecVarp);
            info.m_forceVecVscp = പുതിയ AstVarScope{flp, scopep, forceVecVarp};
            scopep->addVarsp(info.m_forceVecVscp);
        }

        auto pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, nullptr, rhsExprp});
        ForceInfo& finfo = pair.first->second;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        എങ്കിൽ (doingAssign()) {
            std::vector<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* സ്ഥിരം refp) {
                എങ്കിൽ (!refp->access().isReadOnly()) തിരികെ;
                AstVar* സ്ഥിരം depVarp = refp->varp();
                എങ്കിൽ (depVarp
                    && std::find(depVarps.begin(), depVarps.end(), depVarp) == depVarps.end()) {
                    depVarps.തള്ളുക_back(depVarp);
                }
            });
            വേണ്ടി (AstVar* സ്ഥിരം depVarp : depVarps) m_rhsDepToForces[depVarp].തള്ളുക_back(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    സ്ഥിതം ശൂന്യം collectArraySelInfo(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, ArraySelInfo& info) {
        എങ്കിൽ (സ്ഥിരം auto* സ്ഥിരം selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = സത്യം;
            collectArraySelInfo(selp->fromp(), info);
        } അല്ലെങ്കിൽ എങ്കിൽ (auto* സ്ഥിരം selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.തള്ളുക_back(selp);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം auto* സ്ഥിരം memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    സ്ഥിതം ArraySelInfo getArraySelInfo(Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        തിരികെ info;
    }

    സ്ഥിതം std::vector<പൂർണ്ണസംഖ്യ> arraySelDimSizes(സ്ഥിരം ArraySelInfo& info) {
        std::vector<പൂർണ്ണസംഖ്യ> dims;
        dims.reserve(info.m_sels.size());
        വേണ്ടി (AstArraySel* സ്ഥിരം selp : info.m_sels) {
            Ast_കണ്ണി_DType* സ്ഥിരം dtypep = selp->fromp()->dtypep()->skipRefp();
            സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.തള്ളുക_back(arrayp->declRange().elements());
        }
        തിരികെ dims;
    }

    സ്ഥിതം പൂർണ്ണസംഖ്യ flattenIndex(സ്ഥിരം std::vector<പൂർണ്ണസംഖ്യ>& indices, സ്ഥിരം std::vector<പൂർണ്ണസംഖ്യ>& dimSizes) {
        UASSERT(indices.size() == dimSizes.size(), "Array index and dimension size mismatch");
        പൂർണ്ണസംഖ്യ index = 0;
        പൂർണ്ണസംഖ്യ stride = 1;
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = static_cast<പൂർണ്ണസംഖ്യ>(indices.size()) - 1; i >= 0; --i) {
            index += indices[i] * stride;
            stride *= dimSizes[i];
        }
        തിരികെ index;
    }

    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* build_Flatten_Index_വ്യഞ്ജകം(File_വരി* flp, സ്ഥിരം ArraySelInfo& info) {
        സ്ഥിരം std::vector<പൂർണ്ണസംഖ്യ> dimSizes = arraySelDimSizes(info);
        തർക്കം allConst = സത്യം;
        വേണ്ടി (AstArraySel* സ്ഥിരം selp : info.m_sels) {
            എങ്കിൽ (!VN_IS(selp->bitp(), Const)) {
                allConst = അസത്യം;
                നിർത്തുക;
            }
        }
        എങ്കിൽ (allConst) {
            std::vector<പൂർണ്ണസംഖ്യ> constIndices;
            constIndices.reserve(info.m_sels.size());
            വേണ്ടി (AstArraySel* സ്ഥിരം selp : info.m_sels) {
                constIndices.തള്ളുക_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            തിരികെ makeConst32(flp, flattenIndex(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_കണ്ണി_വ്യഞ്ജകം* resultp = nullptr;
        പൂർണ്ണസംഖ്യ stride = 1;
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = static_cast<പൂർണ്ണസംഖ്യ>(info.m_sels.size()) - 1; i >= 0; --i) {
            Ast_കണ്ണി_വ്യഞ്ജകം* termp = info.m_sels[i]->bitp()->clone_വൃക്ഷം_Pure(അസത്യം);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            എങ്കിൽ (termp->width() < 32) termp = പുതിയ AstExtend{flp, termp, 32};
            എങ്കിൽ (stride != 1) termp = പുതിയ AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? പുതിയ AstAdd{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        തിരികെ resultp;
    }

    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* build_Rhs_Data_വ്യഞ്ജകം(File_വരി* flp, സ്ഥിരം ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        തിരികെ പുതിയ AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    ശൂന്യം finalizeRhsVars() {
        വേണ്ടി (VarForceInfo& info : m_varInfos) {
            AstVar* സ്ഥിരം varp = info.m_varp;
            എങ്കിൽ (info.m_forces.empty()) തുടരുക;

            AstScope* സ്ഥിരം scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_വരി* സ്ഥിരം flp = varp->fileline();
            സ്ഥിരം std::vector<ForceInfo*> forceps = forceInfosInIdOrder(info);

            വേണ്ടി (ForceInfo* സ്ഥിരം finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* സ്ഥിരം rhsVarp = പുതിയ AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doingAssign() ? "_VassignRHS" : "__VforceRHS")
                        + std::to_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(സത്യം);
                rhsVarp->sigPublic(സത്യം);
                rhsVarp->setForcedByCode();
                varp->addNextHere(rhsVarp);
                finfo.m_rhsVarVscp = പുതിയ AstVarScope{flp, scopep, rhsVarp};
                scopep->addVarsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                // end
                AstAssign* സ്ഥിരം rhsAssignp = പുതിയ AstAssign{
                    flp, പുതിയ AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                എങ്കിൽ (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* സ്ഥിരം touchCallp = പുതിയ AstCMethodHard{
                        flp, പുതിയ AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtypeSetVoid();
                    Ast_കണ്ണി_Stmt* സ്ഥിരം touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->addNextHere(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* സ്ഥിരം alwaysp
                    = പുതിയ AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, rhsAssignp};
                Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep
                    = പുതിയ Ast_Sen_വൃക്ഷം{flp, പുതിയ AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* സ്ഥിരം activep = പുതിയ AstActive{flp, "force-rhs-update", senTreep};
                activep->sen_വൃക്ഷം_Storep(activep->sentreep());
                activep->addStmtsp(alwaysp);
                scopep->addBlocksp(activep);
            }

            എങ്കിൽ (info.m_forceRdVscp) {
                എങ്കിൽ (!m_forceRdUpdateBuilt.തിരുകുക(info.m_varVscp).second) തുടരുക;
                AstActive* സ്ഥിരം activeInitp = പുതിയ AstActive{
                    flp, "force-init",
                    പുതിയ Ast_Sen_വൃക്ഷം{flp, പുതിയ AstSenItem{flp, AstSenItem::Static{}}}};
                activeInitp->sen_വൃക്ഷം_Storep(activeInitp->sentreep());
                Ast_കണ്ണി_Stmt* initStmtp = nullptr;
                എങ്കിൽ (AstUnpackArrayDType* സ്ഥിരം arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    സ്ഥിരം std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    സ്ഥിരം പൂർണ്ണസംഖ്യ innerWidth = dims.back()->subDTypep()->skipRefp()->width();
                    initStmtp = foreachUnpackedLeaf(
                        dims, [&](സ്ഥിരം std::vector<പൂർണ്ണസംഖ്യ>& idx, പൂർണ്ണസംഖ്യ /*flat*/) -> Ast_കണ്ണി_Stmt* {
                            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = buildNestedArraySel(
                                flp, പുതിയ AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            തിരികെ പുതിയ AstAssign{flp, lhsp, makeZeroConst(varp, innerWidth)};
                        });
                } അല്ലെങ്കിൽ {
                    initStmtp = പുതിയ AstAssign{
                        flp, പുതിയ AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->width())};
                }
                initStmtp->addNext(createForceRdUpdateStmt(info));
                activeInitp->addStmtsp(പുതിയ AstInitial{flp, initStmtp});
                scopep->addBlocksp(activeInitp);

                AstSenItem* itemsp = nullptr;
                auto addSenItem = [&](AstVarScope* vscp) {
                    എങ്കിൽ (!vscp) തിരികെ;
                    AstSenItem* സ്ഥിരം nextp = പുതിയ AstSenItem{
                        flp, VEdgeType::ET_CHANGED, പുതിയ AstVarRef{flp, vscp, VAccess::READ}};
                    എങ്കിൽ (itemsp) {
                        itemsp->addNext(nextp);
                    } അല്ലെങ്കിൽ {
                        itemsp = nextp;
                    }
                };
                addSenItem(info.m_forceEnVscp);
                addSenItem(info.m_forceValVscp);
                AstVarRef* സ്ഥിരം origSenRefp = പുതിയ AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                AstSenItem* സ്ഥിരം origItemp
                    = പുതിയ AstSenItem{flp, VEdgeType::ET_CHANGED, origSenRefp};
                എങ്കിൽ (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->addNext(origItemp);
                വേണ്ടി (ForceInfo* സ്ഥിരം finfop : forceps) {
                    എങ്കിൽ (!finfop->m_isExternal) addSenItem(finfop->m_rhsVarVscp);
                }

                AstActive* സ്ഥിരം activep
                    = പുതിയ AstActive{flp, "force-rd-update", പുതിയ Ast_Sen_വൃക്ഷം{flp, itemsp}};
                activep->sen_വൃക്ഷം_Storep(activep->sentreep());
                activep->addStmtsp(പുതിയ AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr,
                                                 createForceRdUpdateStmt(info)});
                scopep->addBlocksp(activep);
            }
        }
    }

    Ast_കണ്ണി* createRhsUpdatesForWrite(File_വരി* flp, AstVar* writtenVarp) സ്ഥിരം {
        എങ്കിൽ (!doingAssign()) തിരികെ nullptr;

        സ്ഥിരം auto it = m_rhsDepToForces.find(writtenVarp);
        എങ്കിൽ (it == m_rhsDepToForces.end()) തിരികെ nullptr;

        Ast_കണ്ണി* headp = nullptr;
        Ast_കണ്ണി* tailp = nullptr;
        വേണ്ടി (സ്ഥിരം ForceInfo* സ്ഥിരം finfop : it->second) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            AstAssign* സ്ഥിരം updatep
                = പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_വൃക്ഷം_Pure(അസത്യം)};
            എങ്കിൽ (tailp) {
                tailp->addNextHere(updatep);
            } അല്ലെങ്കിൽ {
                headp = updatep;
            }
            tailp = updatep;
        }
        തിരികെ headp;
    }

    സ്ഥിരം ForceInfo& getForceInfo(AstAssignForce* forceStmtp) സ്ഥിരം {
        AstVarScope* സ്ഥിരം vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        സ്ഥിരം VarForceInfo* സ്ഥിരം varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        auto it2 = varInfo->m_forces.find(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.end(), "Force statement not found");
        തിരികെ it2->second;
    }

    സ്ഥിതം തർക്കം selOverlapsAnyForce(സ്ഥിരം VarForceInfo& varInfo, പൂർണ്ണസംഖ്യ selLsb, പൂർണ്ണസംഖ്യ selMsb) {
        വേണ്ടി (സ്ഥിരം auto& pair : varInfo.m_forces) {
            എങ്കിൽ (pair.second.m_rangeLsb <= selMsb && pair.second.m_rangeMsb >= selLsb) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* createForceReadExpression(സ്ഥിരം VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) സ്ഥിരം {
        File_വരി* സ്ഥിരം flp = originalRefp->fileline();
        AstVarRef* സ്ഥിരം refp = originalRefp->clone_വൃക്ഷം_Pure(അസത്യം);
        refp->access(VAccess::READ);
        തിരികെ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   nullptr);
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* createForceReadIndexExpression(സ്ഥിരം VarForceInfo& varInfo,
                                                Ast_കണ്ണി_വ്യഞ്ജകം* originalExprp,
                                                Ast_കണ്ണി_വ്യഞ്ജകം* indexExprp) സ്ഥിരം {
        File_വരി* സ്ഥിരം flp = originalExprp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = originalExprp->clone_വൃക്ഷം_Pure(അസത്യം);
        // Must be an LValue to a static variable
        VN_AS(exprp->getVAccessTargetRecurse(), VarRef)->access(VAccess::READ);
        തിരികെ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_INDEX, exprp, originalExprp,
                                   indexExprp);
    }

    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* rebuildSelPath(Ast_കണ്ണി_വ്യഞ്ജകം* pathp, Ast_കണ്ണി_വ്യഞ്ജകം* baseExprp) {
        എങ്കിൽ (സ്ഥിരം AstSel* സ്ഥിരം selp = VN_CAST(pathp, Sel)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = rebuildSelPath(selp->fromp(), baseExprp);
            AstSel* സ്ഥിരം outp
                = പുതിയ AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->widthConst()};
            outp->dtypeFrom(selp);
            തിരികെ outp;
        }
        തിരികെ baseExprp;
    }
};

// Split deassign concat LHS before converting to release internals.
സ്ഥിതം ശൂന്യം splitDeassign(AstDeassign* nodep) {
    AstConcat* സ്ഥിരം concatp = VN_CAST(nodep->lhsp(), Concat);
    എങ്കിൽ (!concatp) തിരികെ;

    File_വരി* സ്ഥിരം flp = nodep->fileline();
    AstDeassign* സ്ഥിരം newLp = പുതിയ AstDeassign{flp, concatp->lhsp()->unlinkFrBack()};
    AstDeassign* സ്ഥിരം newRp = പുതിയ AstDeassign{flp, concatp->rhsp()->unlinkFrBack()};
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം conp = concatp->unlinkFrBack();
    nodep->replaceWith(newLp);
    newLp->addNextHere(newRp);
    VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    VL_DO_DANGLING(conp->delete_വൃക്ഷം(), conp);

    splitDeassign(newLp);
    splitDeassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

വർഗ്ഗം ForceDiscoveryVisitor final : പൊതു VNVisitorConst {
    ForceState& m_state;
    തർക്കം m_inClockedActive = അസത്യം;

    ശൂന്യം buildForceableUnpackedArray(AstVarScope* സ്ഥിരം nodep,
                                     AstUnpackArrayDType* സ്ഥിരം arrDtypep) {
        AstVar* സ്ഥിരം varp = nodep->varp();
        സ്ഥിരം std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.empty(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം leafDtypep = dims.back()->subDTypep()->skipRefp();
        സ്ഥിരം AstBasicDType* സ്ഥിരം innerBasicp = leafDtypep->basicp();
        എങ്കിൽ (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            തിരികെ;
        }

        File_വരി* സ്ഥിരം flp = varp->fileline();
        സ്ഥിരം പൂർണ്ണസംഖ്യ innerWidth = leafDtypep->width();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* സ്ഥിരം enVscp = info.m_forceEnVscp;
        AstVarScope* സ്ഥിരം valVscp = info.m_forceValVscp;

        AstSenItem* സ്ഥിരം itemsp = പുതിയ AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                  പുതിയ AstVarRef{flp, enVscp, VAccess::READ}};
        AstActive* സ്ഥിരം activep = പുതിയ AstActive{flp, "force-update", പുതിയ Ast_Sen_വൃക്ഷം{flp, itemsp}};
        activep->sen_വൃക്ഷം_Storep(activep->sentreep());

        Ast_കണ്ണി_Stmt* സ്ഥിരം alwaysBodyHeadp = ForceState::foreachUnpackedLeaf(
            dims, [&](സ്ഥിരം std::vector<പൂർണ്ണസംഖ്യ>& idx, പൂർണ്ണസംഖ്യ flat) -> Ast_കണ്ണി_Stmt* {
                AstVarRef* സ്ഥിരം origRefp = പുതിയ AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enSelp = ForceState::buildNestedArraySel(
                    flp, പുതിയ AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valSelp = ForceState::buildNestedArraySel(
                    flp, പുതിയ AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം forceExprp = പുതിയ AstOr{
                    flp, പുതിയ AstAnd{flp, enSelp, valSelp},
                    പുതിയ AstAnd{flp, പുതിയ AstNot{flp, enSelp->clone_വൃക്ഷം_Pure(അസത്യം)}, origSelp}};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsSelp = ForceState::buildNestedArraySel(
                    flp, പുതിയ AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                AstAssignForce* സ്ഥിരം forceAssignp = പുതിയ AstAssignForce{flp, lhsSelp, forceExprp};
                forceAssignp->user2(സത്യം);

                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsClonep = forceExprp->clone_വൃക്ഷം_Pure(അസത്യം);
                rhsClonep->foreach([varp](AstVarRef* സ്ഥിരം r) {
                    എങ്കിൽ (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/innerWidth - 1,
                                           /*hasArraySel=*/സത്യം, /*skipRdRefresh=*/സത്യം);
                തിരികെ forceAssignp;
            });
        activep->addStmtsp(പുതിയ AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, alwaysBodyHeadp});
        nodep->scopep()->addBlocksp(activep);
    }

    ശൂന്യം visit(AstAssignForce* nodep) override {
        എങ്കിൽ (nodep->user2()) തിരികെ;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* സ്ഥിരം lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* സ്ഥിരം forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, സത്യം);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsExprp = nodep->rhsp()->clone_വൃക്ഷം_Pure(അസത്യം);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    ശൂന്യം visit(AstAssign* nodep) override {
        എങ്കിൽ (m_state.doingAssign() && m_inClockedActive) {
            എങ്കിൽ (AstVarRef* സ്ഥിരം lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(AstActive* nodep) override {
        VL_RESTORER(m_inClockedActive);
        m_inClockedActive = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(AstVarScope* nodep) override {
        എങ്കിൽ (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            എങ്കിൽ (m_state.doingAssign()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                തിരികെ;
            }

            എങ്കിൽ (AstUnpackArrayDType* സ്ഥിരം arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                തിരികെ;
            }

            സ്ഥിരം AstBasicDType* സ്ഥിരം bdtypep = nodep->varp()->basicp();
            എങ്കിൽ (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* സ്ഥിരം varp = nodep->varp();
            File_വരി* സ്ഥിരം flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* സ്ഥിരം enVscp = info.m_forceEnVscp;
            AstVarScope* സ്ഥിരം valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            AstSenItem* സ്ഥിരം itemsp = പുതിയ AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                      പുതിയ AstVarRef{flp, enVscp, VAccess::READ}};
            AstActive* സ്ഥിരം activep
                = പുതിയ AstActive{flp, "force-update", പുതിയ Ast_Sen_വൃക്ഷം{flp, itemsp}};
            activep->sen_വൃക്ഷം_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* സ്ഥിരം enRefp = പുതിയ AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* സ്ഥിരം valRefp = പുതിയ AstVarRef{flp, valVscp, VAccess::READ};
            സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstOr{
                            flp, പുതിയ AstAnd{flp, enRefp, valRefp},
                            പുതിയ AstAnd{flp, പുതിയ AstNot{flp, enRefp->clone_വൃക്ഷം_Pure(അസത്യം)}, refp}})
                      : static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstCond{flp, enRefp, valRefp, refp});
            AstAssignForce* സ്ഥിരം forceAssignp
                = പുതിയ AstAssignForce{flp, പുതിയ AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(സത്യം);
            activep->addStmtsp(പുതിയ AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, forceAssignp});
            nodep->scopep()->addBlocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsClonep = forceExprp->clone_വൃക്ഷം_Pure(അസത്യം);
            rhsClonep->foreach([varp](AstVarRef* സ്ഥിരം refp) {
                എങ്കിൽ (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            സ്ഥിരം തർക്കം bitwiseVar = ForceState::isBitwiseDType(varp);
            സ്ഥിരം പൂർണ്ണസംഖ്യ padMsb = bitwiseVar ? (varp->width() - 1) : 0;
            പൂർണ്ണസംഖ്യ rangeLsb = 0;
            പൂർണ്ണസംഖ്യ rangeMsb = padMsb;
            എങ്കിൽ (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, അസത്യം);
        }
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    explicit ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextConstNull(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

വർഗ്ഗം ForceConvertVisitor final : പൊതു VNVisitor {
    ForceState& m_state;

    ശൂന്യം visit(AstAssignForce* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp();
        AstVarRef* സ്ഥിരം lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* സ്ഥിരം forcedVarp = lhsVarRefp->varp();

        സ്ഥിരം ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        സ്ഥിരം ForceState::VarForceInfo* സ്ഥിരം varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_വരി* സ്ഥിരം flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        AstAssign* സ്ഥിരം rhsAssignp
            = പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_വൃക്ഷം_Pure(അസത്യം)};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        സ്ഥിരം തർക്കം bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        എങ്കിൽ (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_കണ്ണി_വ്യഞ്ജകം* baseExprp = nodep->rhsp()->clone_വൃക്ഷം_Pure(അസത്യം);
            baseExprp->foreach(
                [](AstVarRef* സ്ഥിരം refp) { ForceState::markNonReplaceable(refp); });
            എങ്കിൽ (bitwiseForcedVar) {
                baseExprp = ForceState::zeroPadToBaseWidth(baseExprp, forcedVarp->width(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            എങ്കിൽ (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* സ്ഥിരം maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valReadp
                    = പുതിയ AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valWritep
                    = പുതിയ AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notMaskp = പുതിയ AstNot{flp, maskConstp};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം maskedOldp = പുതിയ AstAnd{flp, valReadp, notMaskp};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newValp = പുതിയ AstOr{flp, maskedOldp, baseExprp};
                valAssignp = പുതിയ AstAssign{flp, valWritep, newValp};

                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enReadp
                    = പുതിയ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enWritep
                    = പുതിയ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newEnp
                    = പുതിയ AstOr{flp, enReadp, maskConstp->clone_വൃക്ഷം_Pure(അസത്യം)};
                enAssignp = പുതിയ AstAssign{flp, enWritep, newEnp};
            } അല്ലെങ്കിൽ {
                AstConst* സ്ഥിരം oneConstp = ForceState::makeRangeMaskConst(nodep, 1, 0, 0);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsValp = ForceState::cast_To_കണ്ണി_DType(baseExprp, forcedVarp);
                valAssignp = പുതിയ AstAssign{
                    flp, പുതിയ AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = പുതിയ AstAssign{
                    flp, പുതിയ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        സ്ഥിരം AstSel* സ്ഥിരം selLhsp = VN_CAST(lhsp, Sel);
        സ്ഥിരം തർക്കം arrayBitSel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->width();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsDatap = ForceState::build_Rhs_Data_വ്യഞ്ജകം(flp, info);
        AstCExpr* സ്ഥിരം rhsAddrp = പുതിയ AstCExpr{flp};
        rhsAddrp->add("&(");
        rhsAddrp->add(rhsDatap);
        rhsAddrp->add(")");
        AstCMethodHard* സ്ഥിരം addForceCallp = പുതിയ AstCMethodHard{
            flp, പുതിയ AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ADD,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        addForceCallp->addPinsp(rhsAddrp);
        addForceCallp->addPinsp(
            ForceState::makeConst32(flp, arrayBitSel ? info.m_padLsb : info.m_rangeLsb));
        എങ്കിൽ (arrayBitSel) {
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padLsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padMsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, selLhsp->fromp()->width()));
        }
        addForceCallp->dtypeSetVoid();
        Ast_കണ്ണി_Stmt* സ്ഥിരം stmtp = addForceCallp->makeStmt();

        Ast_കണ്ണി* tailp = rhsAssignp;
        എങ്കിൽ (valAssignp) {
            tailp->addNextHere(valAssignp);
            tailp = valAssignp;
        }
        എങ്കിൽ (enAssignp) {
            tailp->addNextHere(enAssignp);
            tailp = enAssignp;
        }
        tailp->addNextHere(stmtp);
        എങ്കിൽ (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp();
        AstVarRef* സ്ഥിരം lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* സ്ഥിരം releasedVarp = lhsVarRefp->varp();

        സ്ഥിരം ForceState::VarForceInfo* സ്ഥിരം varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        എങ്കിൽ (!varInfo) {
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }
        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        File_വരി* സ്ഥിരം flp = nodep->fileline();

        സ്ഥിരം ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, അസത്യം);

        സ്ഥിരം AstSel* സ്ഥിരം selLhsp = VN_CAST(lhsp, Sel);
        സ്ഥിരം തർക്കം arrayBitSel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->width();
        AstCMethodHard* സ്ഥിരം releaseCallp = പുതിയ AstCMethodHard{
            flp, പുതിയ AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        എങ്കിൽ (arrayBitSel) {
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtypeSetVoid();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_കണ്ണി_Stmt* സ്ഥിരം releasep = releaseCallp->makeStmt();

        AstAssign* clearEnp = nullptr;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        എങ്കിൽ (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enWritep
                = പുതിയ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            എങ്കിൽ (ForceState::isBitwiseDType(releasedVarp)) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ varWidth = releasedVarp->width();
                എങ്കിൽ (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == varWidth - 1) {
                    clearEnp
                        = പുതിയ AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, varWidth)};
                } അല്ലെങ്കിൽ {
                    // forceEn = forceEn & ~mask(range);
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enReadp
                        = പുതിയ AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* സ്ഥിരം maskConst = ForceState::makeRangeMaskConst(
                        nodep, varWidth, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newEnp
                        = പുതിയ AstAnd{flp, enReadp, പുതിയ AstNot{flp, maskConst}};
                    clearEnp = പുതിയ AstAssign{flp, enWritep, newEnp};
                }
            } അല്ലെങ്കിൽ {
                clearEnp = പുതിയ AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        സ്ഥിരം AstSel* സ്ഥിരം selp = VN_CAST(lhsp, Sel);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം basep = selp ? selp->fromp() : lhsp;

        Ast_കണ്ണി* stmtListp = releasep;
        എങ്കിൽ (clearEnp) {
            clearEnp->addNextHere(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        സ്ഥിരം തർക്കം fullBitwiseRelease
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->width() - 1;
        എങ്കിൽ (!releasedVarp->isContinuously()
            && !(m_state.doingAssign() && m_state.hasClockedWrite(releasedVarp)
                 && fullBitwiseRelease)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            സ്ഥിരം തർക്കം hasOpaquePath = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_കണ്ണി_വ്യഞ്ജകം* forceReadp = nullptr;
            എങ്കിൽ (hasOpaquePath) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } അല്ലെങ്കിൽ എങ്കിൽ (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, basep,
                    ForceState::build_Flatten_Index_വ്യഞ്ജകം(flp, rangeInfo.m_arrayInfo));
                എങ്കിൽ (selp) { forceReadp = ForceState::rebuildSelPath(lhsp, forceReadp); }
            } അല്ലെങ്കിൽ {
                forceReadp
                    = selp ? ForceState::rebuildSelPath(
                                 lhsp, m_state.createForceReadExpression(*varInfo, lhsVarRefp))
                           : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            }
            AstAssign* സ്ഥിരം assignp = പുതിയ AstAssign{flp, lhsp->clone_വൃക്ഷം_Pure(അസത്യം), forceReadp};
            assignp->addNextHere(stmtListp);
            stmtListp = assignp;
        }

        എങ്കിൽ (varInfo->m_forceRdVscp) stmtListp->addNext(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

വർഗ്ഗം ForceReplaceVisitor final : പൊതു VNVisitor {
    സ്ഥിരം ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_കണ്ണി_Stmt* m_stmtp = nullptr;
    തർക്കം m_inLogic = അസത്യം;

    ശൂന്യം iterateLogic(Ast_കണ്ണി* nodep) {
        VL_RESTORER(m_inLogic);
        m_inLogic = സത്യം;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstAssign* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        എങ്കിൽ (AstVarRef* സ്ഥിരം lhsp = VN_CAST(nodep->lhsp()->baseFromp(സത്യം), VarRef)) {
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->addNextHere(updatep);
            }
        }
    }
    ശൂന്യം visit(AstAssignCont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateAndNextNull(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    ശൂന്യം visit(AstDeassign*) override {}
    ശൂന്യം visit(AstCFunc* nodep) override { iterateLogic(nodep); }
    ശൂന്യം visit(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override { iterateLogic(nodep); }
    ശൂന്യം visit(AstAlways* nodep) override {
        എങ്കിൽ (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            തിരികെ;
        }
        iterateLogic(nodep);
    }
    ശൂന്യം visit(AstSenItem* nodep) override { iterateLogic(nodep); }
    ശൂന്യം visit(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* സ്ഥിരം refp = VN_CAST(nodep->fromp(), VarRef);
        എങ്കിൽ (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            visit(static_cast<Ast_കണ്ണി*>(nodep));
            തിരികെ;
        }

        AstVar* സ്ഥിരം varp = refp->varp();
        സ്ഥിരം ForceState::VarForceInfo* സ്ഥിരം varInfo = m_state.getVarInfo(refp->varScopep());
        എങ്കിൽ (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.empty()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            visit(static_cast<Ast_കണ്ണി*>(nodep));
            തിരികെ;
        }

        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ selLsb = lsbConstp->toSInt();
            സ്ഥിരം പൂർണ്ണസംഖ്യ selMsb = selLsb + nodep->width() - 1;
            എങ്കിൽ (!varp->isSigPublic()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                visit(static_cast<Ast_കണ്ണി*>(nodep));
                തിരികെ;
            }
        }

        File_വരി* സ്ഥിരം flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* സ്ഥിരം refClonep = refp->clone_വൃക്ഷം_Pure(അസത്യം);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* സ്ഥിരം callp = പുതിയ AstCMethodHard{
            flp, പുതിയ AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->width())};
        callp->addPinsp(refClonep);
        callp->addPinsp(nodep->lsbp()->clone_വൃക്ഷം_Pure(അസത്യം));
        callp->addPinsp(ForceState::makeConst32(flp, nodep->width()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstArraySel* nodep) override {
        എങ്കിൽ (സ്ഥിരം AstArraySel* സ്ഥിരം backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            എങ്കിൽ (backSelp->fromp() == nodep) {
                iterateChildren(nodep);
                തിരികെ;
            }
        }

        Ast_കണ്ണി* സ്ഥിരം basep = nodep->baseFromp(സത്യം);
        AstVarRef* സ്ഥിരം baseRefp = VN_CAST(basep, VarRef);
        എങ്കിൽ (!baseRefp) {
            iterateChildren(nodep);
            തിരികെ;
        }
        AstVar* സ്ഥിരം varp = baseRefp->varp();
        സ്ഥിരം ForceState::VarForceInfo* സ്ഥിരം varInfo = m_state.getVarInfo(baseRefp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        എങ്കിൽ (m_state.isNotReplaceable(baseRefp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            തിരികെ;
        }

        എങ്കിൽ (!baseRefp->access().isReadOnly()) {
            iterateChildren(nodep);
            തിരികെ;
        }
        എങ്കിൽ (m_state.doingAssign() && varInfo->m_forceRdVscp) {
            baseRefp->varp(varInfo->m_forceRdVscp->varp());
            baseRefp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            തിരികെ;
        }
        സ്ഥിരം ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        വേണ്ടി (AstArraySel* സ്ഥിരം selp : arrayInfo.m_sels) iterateAndNextNull(selp->bitp());
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം indexExprp
            = ForceState::build_Flatten_Index_വ്യഞ്ജകം(nodep->fileline(), arrayInfo);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം readExprp
            = m_state.createForceReadIndexExpression(*varInfo, nodep, indexExprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം visit(AstVarRef* nodep) override {
        എങ്കിൽ (m_state.isNotReplaceable(nodep)) തിരികെ;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        എങ്കിൽ (സ്ഥിരം AstArraySel* സ്ഥിരം backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            എങ്കിൽ (backSelp->fromp() == nodep) തിരികെ;
        }

        സ്ഥിരം ForceState::VarForceInfo* സ്ഥിരം varInfo = m_state.getVarInfo(nodep->varScopep());
        എങ്കിൽ (!varInfo) തിരികെ;

        എങ്കിൽ (varInfo->m_forceRdVscp) {
            എങ്കിൽ (nodep->access().isRW()) {
                എങ്കിൽ (m_inLogic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                തിരികെ;
            }
            എങ്കിൽ (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം readp
                    = m_state.createForceRdExpression(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ;
            }
            എങ്കിൽ (m_inLogic && m_stmtp) {
                m_stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
            }
            തിരികെ;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        എങ്കിൽ (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            തിരികെ;
        }

        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        എങ്കിൽ (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (ForceState::isOutermostOpaquePathSelector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = VN_AS(nodep, കണ്ണി_വ്യഞ്ജകം);
            Ast_കണ്ണി* സ്ഥിരം basep = exprp->baseFromp(സത്യം);
            AstVarRef* സ്ഥിരം baseRefp = VN_CAST(basep, VarRef);
            എങ്കിൽ (baseRefp) {
                AstVar* സ്ഥിരം varp = baseRefp->varp();
                എങ്കിൽ (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    സ്ഥിരം ForceState::VarForceInfo* സ്ഥിരം varInfo
                        = m_state.getVarInfo(baseRefp->varScopep());
                    എങ്കിൽ (!m_state.isNotReplaceable(baseRefp) && varInfo) {
                        സ്ഥിരം പൂർണ്ണസംഖ്യ forcePathIndex = varInfo->findForcePathIndex(exprp);
                        എങ്കിൽ (forcePathIndex >= 0) {
                            എങ്കിൽ (!baseRefp->access().isReadOnly()) തിരികെ;
                            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം readExprp = m_state.createForceReadIndexExpression(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), forcePathIndex));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                            തിരികെ;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

പൊതു:
    explicit ForceReplaceVisitor(AstNetlist* nodep, സ്ഥിരം ForceState& state)
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

സ്ഥിതം ശൂന്യം forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::ProtectedVarRefSet& permanentlyProtected,
                         ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    എങ്കിൽ (!v3Global.hasForceableSignals()) തിരികെ;
    ForceState state{അസത്യം, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_വൃക്ഷം("force", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

സ്ഥിതം ശൂന്യം assignAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::ProtectedVarRefSet& permanentlyProtected,
                          ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    എങ്കിൽ (!v3Global.hasAssignDeassign()) തിരികെ;

    std::vector<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.തള്ളുക_back(deassignp); });
    വേണ്ടി (AstDeassign* സ്ഥിരം deassignp : deassignps) splitDeassign(deassignp);

    std::vector<AstAssignCont*> assignContps;
    deassignps.clear();
    nodep->foreach([&](Ast_കണ്ണി_Stmt* nodep) {
        എങ്കിൽ (AstAssignCont* സ്ഥിരം assignContp = VN_CAST(nodep, AssignCont)) {
            assignContps.തള്ളുക_back(assignContp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstDeassign* സ്ഥിരം deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.തള്ളുക_back(deassignp);
        }
    });

    വേണ്ടി (AstAssignCont* സ്ഥിരം assignp : assignContps) {
        assignp->replaceWith(പുതിയ AstAssignForce{assignp->fileline(),
                                                assignp->lhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
        VL_DO_DANGLING(assignp->delete_വൃക്ഷം(), assignp);
    }
    വേണ്ടി (AstDeassign* സ്ഥിരം deassignp : deassignps) {
        deassignp->replaceWith(
            പുതിയ AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_വൃക്ഷം_Pure(സത്യം)});
        VL_DO_DANGLING(deassignp->delete_വൃക്ഷം(), deassignp);
    }
    ForceState state{സത്യം, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_വൃക്ഷം("assign-deassign", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

ശൂന്യം V3Force::forceAndAssignAll(AstNetlist* nodep) {
    സ്ഥിരം VNUser3InUse user3InUse;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::ProtectedVarRefSet permanentlyProtected;
    ForceState::BuiltVarScopeSet forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    assignAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
