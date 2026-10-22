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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Force.h"

#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

વર્ગ ForceState final {
જાહેર:
    સંરચના ForceRange VL_NOT_FINAL {
        પૂર્ણાંક m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        પૂર્ણાંક m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        પૂર્ણાંક m_padLsb = 0;  // Bit positions for RHS padding
        પૂર્ણાંક m_padMsb = 0;  // Bit positions for RHS padding
    };

    સંરચના ForceInfo final : ForceRange {
        // MEMBERS
        પૂર્ણાંક m_forceId = 0;  // Unique (per signal) variable of this force assignment
        તાર્કિક m_hasArraySel = ખોટું;  // If this has an array select on LHS
        તાર્કિક m_isExternal = ખોટું;  // Synthetic force for the public force controls
        તાર્કિક m_skipRdRefresh = ખોટું;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = nullptr;  // Scope of the var containing RHSID
        Ast_ગાંઠ_પદાવલિ* m_rhsExprp = nullptr;  // Expression on RHS of this force assignment

        ForceInfo() = મૂળભૂત;
        ForceInfo(પૂર્ણાંક rangeLsb, પૂર્ણાંક rangeMsb, પૂર્ણાંક padLsb, પૂર્ણાંક padMsb, પૂર્ણાંક forceId,
                  તાર્કિક hasArraySel, AstVarScope* rhsVarVscp, Ast_ગાંઠ_પદાવલિ* rhsExprp)
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

    સંરચના VarForceInfo final {
        AstVarScope* m_forceVecVscp = nullptr;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = nullptr;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = nullptr;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = nullptr;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = nullptr;  // Scope of the forced (original) variable
        AstVar* m_varp = nullptr;  // The forced (original) variable
        AstScope* m_scopep = nullptr;  // Scope containing the forced variable
        std::unordered_map<AstAssignForce*, ForceInfo> m_forces;
        std::unordered_map<string, પૂર્ણાંક> m_forcePathToIndex;
        પૂર્ણાંક m_nextForcePathIndex = 1;  // Start at 1 so 0 can be the base path (whole signal)

    ખાનગી:
        સ્થિર શૂન્ય build_Force_Path_ચાવી_Recurse(Ast_ગાંઠ_પદાવલિ* nodep, string& out) {
            જો (VN_IS(nodep, VarRef)) પાછા;
            જો (અચલ AstSel* અચલ selp = VN_CAST(nodep, Sel)) {
                build_Force_Path_ચાવી_Recurse(selp->fromp(), out);
                out += "|S" + cvtToStr(selp->lsbConst()) + ":" + cvtToStr(selp->widthConst());
                પાછા;
            }
            જો (AstStructSel* અચલ selp = VN_CAST(nodep, StructSel)) {
                Ast_ગાંઠ_પદાવલિ* અચલ fromp = selp->fromp();
                build_Force_Path_ચાવી_Recurse(fromp, out);
                out += "|T" + selp->name();
                પાછા;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        સ્થિર string force_Path_ચાવી(Ast_ગાંઠ_પદાવલિ* nodep) {
            string out;
            build_Force_Path_ચાવી_Recurse(nodep, out);
            પાછા out;
        }

    જાહેર:
        પૂર્ણાંક getOrCreateForcePathIndex(Ast_ગાંઠ_પદાવલિ* nodep) {
            અચલ auto pair
                = m_forcePathToIndex.emplace(force_Path_ચાવી(nodep), m_nextForcePathIndex);
            જો (pair.second) ++m_nextForcePathIndex;
            પાછા pair.first->second;
        }

        પૂર્ણાંક findForcePathIndex(Ast_ગાંઠ_પદાવલિ* nodep) અચલ {
            અચલ auto it = m_forcePathToIndex.find(force_Path_ચાવી(nodep));
            પાછા it != m_forcePathToIndex.end() ? it->second : -1;
        }
    };

    સંરચના ForceHelperVars final {
        AstVar* m_rdVarp = nullptr;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = nullptr;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = nullptr;  // The __VforceVal (force value) variable
    };

    સંરચના ArraySelInfo final {
        std::vector<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        તાર્કિક m_hasBitSel = ખોટું;  // Bit-select was found along the path too
    };

    સંરચના ForceRangeInfo final : ForceRange {
        તાર્કિક m_hasArraySel = ખોટું;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

ખાનગી:
    using ScopeVarCache = std::unordered_map<અચલ AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    અચલ VNUser1InUse m_user1InUse;
    અચલ VNUser2InUse m_user2InUse;

જાહેર:
    using ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    using ProtectedVarRefSet = std::unordered_set<અચલ AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    using BuiltVarScopeSet = std::unordered_set<અચલ AstVarScope*>;

ખાનગી:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::vector<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, પૂર્ણાંક> m_varToId;
    std::unordered_set<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::vector<ForceInfo*>> m_rhsDepToForces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    તાર્કિક m_doingAssign = ખોટું;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    ProtectedVarRefSet& m_permanentlyProtected;
    BuiltVarScopeSet& m_forceRdUpdateBuilt;

જાહેર:
    ForceState(તાર્કિક doingAssign, ForceHelperVarsByVar& forceHelperVarsByVar,
               ProtectedVarRefSet& permanentlyProtected, BuiltVarScopeSet& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doingAssign{doingAssign}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    સ્થિર AstConst* makeZeroConst(Ast_ગાંઠ* nodep, પૂર્ણાંક width) {
        V3_સંખ્યા zero{nodep, width};
        zero.setAllBits0();
        પાછા નવું AstConst{nodep->fileline(), zero};
    }

    સ્થિર AstConst* makeConst32(File_લીટી* flp, પૂર્ણાંક કિંમત) {
        પાછા નવું AstConst{flp, AstConst::Widthed_કિંમત{}, 32, static_cast<uint32_t>(કિંમત)};
    }

    સ્થિર AstConst* makeRangeMaskConst(Ast_ગાંઠ* nodep, પૂર્ણાંક width, પૂર્ણાંક lsb, પૂર્ણાંક msb) {
        V3_સંખ્યા mask{nodep, width};
        mask.setAllBits0();
        માટે (પૂર્ણાંક bit = lsb; bit <= msb; ++bit) mask.setBit(bit, 1);
        પાછા નવું AstConst{nodep->fileline(), mask};
    }

    સ્થિર Ast_ગાંઠ_પદાવલિ* zeroPadToBaseWidth(Ast_ગાંઠ_પદાવલિ* exprp, પૂર્ણાંક baseWidth, પૂર્ણાંક padLsb,
                                           પૂર્ણાંક padMsb) {
        જો (baseWidth <= 0) પાછા exprp;
        અચલ પૂર્ણાંક નીચલું_Pad = padLsb;
        અચલ પૂર્ણાંક ઉપલું_Pad = baseWidth - (padMsb + 1);
        જો (નીચલું_Pad > 0) {
            exprp = નવું AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, નીચલું_Pad)};
        }
        જો (ઉપલું_Pad > 0) {
            exprp = નવું AstConcat{exprp->fileline(), makeZeroConst(exprp, ઉપલું_Pad), exprp};
        }
        પાછા exprp;
    }

    સ્થિર તાર્કિક isUnpackedArrayDType(અચલ Ast_ગાંઠ_DType* dtypep) {
        પાછા VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    સ્થિર તાર્કિક isBitwiseDType(અચલ Ast_ગાંઠ* nodep) {
        અચલ AstBasicDType* અચલ basicp = nodep->dtypep()->skipRefp()->basicp();
        પાછા basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    સ્થિર Ast_ગાંઠ_પદાવલિ* cast_To_ગાંઠ_DType(Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ* dtypeFromp) {
        અચલ Ast_ગાંઠ_DType* અચલ dtypep = dtypeFromp->dtypep()->skipRefp();
        અચલ AstBasicDType* અચલ basicp = dtypep->basicp();
        જો (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            પાછા exprp;
        }
        પાછા નવું AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    સ્થિર શૂન્ય markNonReplaceable(AstVarRef* અચલ nodep) { nodep->user1SetOnce(); }
    તાર્કિક isNotReplaceable(અચલ AstVarRef* અચલ nodep) અચલ {
        પાછા nodep->user1() || m_permanentlyProtected.ગણતરી(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    શૂન્ય markPermanentlyProtected(AstVarRef* અચલ nodep) અચલ {
        markNonReplaceable(nodep);
        m_permanentlyProtected.ઉમેરો(nodep);
    }

    સ્થિર std::vector<ForceInfo*> forceInfosInIdOrder(VarForceInfo& info) {
        std::vector<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        માટે (auto& it : info.m_forces) forceps.ધકેલો_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](અચલ ForceInfo* ap, અચલ ForceInfo* bp) {
            પાછા ap->m_forceId < bp->m_forceId;
        });
        પાછા forceps;
    }

    સ્થિર std::vector<અચલ ForceInfo*> forceInfosInIdOrder(અચલ VarForceInfo& info) {
        std::vector<અચલ ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        માટે (અચલ auto& it : info.m_forces) forceps.ધકેલો_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](અચલ ForceInfo* ap, અચલ ForceInfo* bp) {
            પાછા ap->m_forceId < bp->m_forceId;
        });
        પાછા forceps;
    }

    સ્થિર તાર્કિક isOpaquePathSelector(અચલ Ast_ગાંઠ* nodep) {
        પાછા VN_IS(nodep, Sel) || VN_IS(nodep, ગાંઠ_Sel) || VN_IS(nodep, StructSel);
    }

    સ્થિર તાર્કિક isOutermostOpaquePathSelector(અચલ Ast_ગાંઠ* nodep) {
        જો (!isOpaquePathSelector(nodep)) પાછા ખોટું;
        અચલ Ast_ગાંઠ* અચલ backp = nodep->backp();
        પાછા !backp || !isOpaquePathSelector(backp);
    }

    સ્થિર AstVarRef* getOneVarRef(Ast_ગાંઠ_પદાવલિ* forceStmtp) {
        Ast_ગાંઠ* અચલ basep = forceStmtp->baseFromp(સાચું);
        જો (AstSampled* sampledp = VN_CAST(basep, Sampled))
            જો (Ast_ગાંઠ_પદાવલિ* exprp = VN_CAST(sampledp->exprp(), ગાંઠ_પદાવલિ))
                પાછા getOneVarRef(exprp);
        AstVarRef* અચલ varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        પાછા varRefp;
    }

    સ્થિર Ast_ગાંઠ_પદાવલિ* buildNestedArraySel(File_લીટી* flp, Ast_ગાંઠ_પદાવલિ* fromp,
                                            અચલ std::vector<પૂર્ણાંક>& indicies) {
        Ast_ગાંઠ_પદાવલિ* curp = fromp;
        માટે (અચલ પૂર્ણાંક idx : indicies) curp = નવું AstArraySel{flp, curp, idx};
        પાછા curp;
    }

    template <typename Fn>
    સ્થિર Ast_ગાંઠ_Stmt* foreachUnpackedLeaf(અચલ std::vector<AstUnpackArrayDType*>& dims,
                                            Fn buildLeaf) {
        Ast_ગાંઠ_Stmt* headp = nullptr;
        Ast_ગાંઠ_Stmt* tailp = nullptr;
        જો (dims.empty()) પાછા nullptr;
        પૂર્ણાંક total = 1;
        માટે (અચલ AstUnpackArrayDType* અચલ d : dims) total *= d->elementsConst();
        જો (total <= 0) પાછા nullptr;
        std::vector<પૂર્ણાંક> idx(dims.size(), 0);
        માટે (પૂર્ણાંક flat = 0; flat < total; ++flat) {
            Ast_ગાંઠ_Stmt* અચલ stmtp = buildLeaf(idx, flat);
            જો (!headp) {
                headp = stmtp;
            } નહીંતર {
                tailp->addNext(stmtp);
            }
            tailp = stmtp;
            માટે (પૂર્ણાંક d = static_cast<પૂર્ણાંક>(dims.size()) - 1; d >= 0; --d) {
                જો (++idx[d] < dims[d]->elementsConst()) તોડો;
                idx[d] = 0;
            }
        }
        પાછા headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_ગાંઠ_પદાવલિ* lhsp, AstVar* varp,
                                     તાર્કિક requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.empty();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->width() - 1) : 0;

        જો (અચલ AstSel* અચલ outerSelp = VN_CAST(lhsp, Sel)) {
            પૂર્ણાંક totalLsb = 0;
            માટે (Ast_ગાંઠ_પદાવલિ* curp = lhsp; અચલ AstSel* અચલ selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                જો (requireConstRangeSelect) {
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
        જો (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::vector<પૂર્ણાંક> indices;
            indices.reserve(info.m_arrayInfo.m_sels.size());
            માટે (AstArraySel* અચલ selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.ધકેલો_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            info.m_rangeLsb = flattenIndex(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } નહીંતર જો (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        જો (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            અચલ પૂર્ણાંક index = varInfo.getOrCreateForcePathIndex(lhsp);
            info.m_rangeLsb = index;
            info.m_rangeMsb = index;
        }
        પાછા info;
    }

    Ast_ગાંઠ_પદાવલિ* add_Rhs_કિંમત_Reads(અચલ VarForceInfo& varInfo, Ast_ગાંઠ_પદાવલિ* exprp) અચલ {
        જો (!doingAssign()) પાછા exprp;

        અચલ std::vector<અચલ ForceInfo*> forceps = forceInfosInIdOrder(varInfo);
        જો (forceps.empty()) પાછા exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* અચલ cexprp = નવું AstCExpr{exprp->fileline(), AstCExpr::Pure{}};
        cexprp->dtypeFrom(exprp);
        cexprp->add("(");
        માટે (અચલ ForceInfo* અચલ finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* અચલ refp
                = નવું AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->add("(void)(");
            cexprp->add(refp);
            cexprp->add("), ");
        }
        cexprp->add(exprp);
        cexprp->add(")");
        પાછા cexprp;
    }

    Ast_ગાંઠ_પદાવલિ* createForceReadCall(અચલ VarForceInfo& varInfo, File_લીટી* flp, VCMethod method,
                                     Ast_ગાંઠ_પદાવલિ* originalExprp, Ast_ગાંઠ* dtypeFromp,
                                     Ast_ગાંઠ_પદાવલિ* indexExprp) અચલ {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_ગાંઠ_પદાવલિ* અચલ origValp
            = add_Rhs_કિંમત_Reads(varInfo, cast_To_ગાંઠ_DType(originalExprp, dtypeFromp));

        AstCMethodHard* અચલ callp = નવું AstCMethodHard{
            flp, નવું AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, method, origValp};
        જો (indexExprp) callp->addPinsp(indexExprp);
        callp->dtypeFrom(dtypeFromp);
        પાછા callp;
    }

    Ast_ગાંઠ_પદાવલિ* createForceRdExpression(અચલ VarForceInfo& varInfo, File_લીટી* flp) અચલ {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* અચલ varp = varInfo.m_varVscp->varp();
        જો (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            જો (doingAssign()) { પાછા નવું AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, સાચું);
            AstVarRef* અચલ baseRefp = નવું AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(baseRefp);
            પાછા createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, baseRefp, varp,
                                       nullptr);
        }
        AstVarRef* અચલ baseRefp = નવું AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(baseRefp);
        Ast_ગાંઠ_પદાવલિ* અચલ enRefp = નવું AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_ગાંઠ_પદાવલિ* અચલ valRefp = નવું AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        જો (isBitwiseDType(varp)) {
            પાછા નવું AstOr{
                flp, નવું AstAnd{flp, enRefp, valRefp},
                નવું AstAnd{flp, નવું AstNot{flp, enRefp->clone_વૃક્ષ_Pure(ખોટું)}, baseRefp}};
        }
        પાછા નવું AstCond{flp, enRefp, valRefp, baseRefp};
    }

    Ast_ગાંઠ_Stmt* createForceRdUpdateStmt(અચલ VarForceInfo& varInfo) અચલ {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* અચલ varp = varInfo.m_varVscp->varp();
        જો (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            પાછા createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_લીટી* અચલ flp = varInfo.m_varVscp->fileline();
        પાછા નવું AstAssign{flp, નવું AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             createForceRdExpression(varInfo, flp)};
    }

    Ast_ગાંઠ_Stmt* createForceRdUpdateStmtUnpacked(અચલ VarForceInfo& varInfo) અચલ {
        File_લીટી* અચલ flp = varInfo.m_varVscp->fileline();
        AstVar* અચલ varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* અચલ arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        અચલ std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_ગાંઠ_Stmt* અચલ stmtsp = foreachUnpackedLeaf(
            dims, [&](અચલ std::vector<પૂર્ણાંક>& idx, પૂર્ણાંક /*flat*/) -> Ast_ગાંઠ_Stmt* {
                AstVarRef* અચલ baseRefp = નવું AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(baseRefp);
                Ast_ગાંઠ_પદાવલિ* અચલ baseSelp = buildNestedArraySel(flp, baseRefp, idx);
                Ast_ગાંઠ_પદાવલિ* અચલ enSelp = buildNestedArraySel(
                    flp, નવું AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_ગાંઠ_પદાવલિ* અચલ valSelp = buildNestedArraySel(
                    flp, નવું AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_ગાંઠ_પદાવલિ* અચલ readExprp = નવું AstOr{
                    flp, નવું AstAnd{flp, enSelp, valSelp},
                    નવું AstAnd{flp, નવું AstNot{flp, enSelp->clone_વૃક્ષ_Pure(ખોટું)}, baseSelp}};
                Ast_ગાંઠ_પદાવલિ* અચલ rdLhsSelp = buildNestedArraySel(
                    flp, નવું AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                પાછા નવું AstAssign{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        માટે (અચલ ForceInfo* અચલ finfop : forceInfosInIdOrder(varInfo)) {
            જો (!finfop->m_isExternal) ચાલુ;
            stmtsp->addNext(નવું AstAssign{flp,
                                          નવું AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_વૃક્ષ_Pure(ખોટું)});
        }
        પાછા stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        અચલ auto it = m_varToId.find(vscp);
        જો (it != m_varToId.end()) પાછા m_varInfos[it->second];

        m_varToId.emplace(vscp, m_varInfos.size());
        m_varInfos.emplace_back();
        VarForceInfo& info = m_varInfos.back();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* અચલ varp = info.m_varp;
        જો (!varp->isForceable()) પાછા info;

        File_લીટી* અચલ flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        અચલ તાર્કિક helperVarsBuilt = helperVars.m_rdVarp != nullptr;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != nullptr)
                        && helperVarsBuilt == (helperVars.m_valVarp != nullptr),
                    varp, "Incomplete force helper set");
        જો (!helperVarsBuilt) {
            અચલ તાર્કિક unpacked = isUnpackedArrayDType(varp->dtypep());
            અચલ VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_ગાંઠ_DType* અચલ enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
            helperVars.m_rdVarp
                = નવું AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sigPublic(સાચું);
            helperVars.m_enVarp
                = નવું AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(સાચું);
            helperVars.m_valVarp
                = નવું AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(સાચું);
            varp->addNextHere(helperVars.m_rdVarp);
            varp->addNextHere(helperVars.m_enVarp);
            varp->addNextHere(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = findScopeVar(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = findScopeVar(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = findScopeVar(info.m_scopep, helperVars.m_valVarp);
        જો (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } નહીંતર {
            info.m_forceRdVscp = નવું AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = નવું AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = નવું AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->addVarsp(info.m_forceRdVscp);
            info.m_scopep->addVarsp(info.m_forceEnVscp);
            info.m_scopep->addVarsp(info.m_forceValVscp);
        }
        પાછા info;
    }

    શૂન્ય markClockedWrite(AstVar* varp) { m_clockedWrites.ઉમેરો(varp); }
    તાર્કિક hasClockedWrite(AstVar* varp) અચલ { પાછા m_clockedWrites.ગણતરી(varp); }

    તાર્કિક doingAssign() અચલ { પાછા m_doingAssign; }

    અચલ VarForceInfo* getVarInfo(AstVarScope* vscp) અચલ {
        અચલ auto it = m_varToId.find(vscp);
        પાછા it != m_varToId.end() ? &m_varInfos[it->second] : nullptr;
    }

    AstVarScope* findScopeVar(AstScope* scopep, અચલ AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        જો (cache.empty()) {
            માટે (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        અચલ auto it = cache.find(varp);
        પાછા it != cache.end() ? it->second : nullptr;
    }
    શૂન્ય addForceAssignment(AstVar* varp, AstVarScope* vscp, Ast_ગાંઠ_પદાવલિ* rhsExprp,
                            AstAssignForce* forceStmtp, પૂર્ણાંક rangeLsb, પૂર્ણાંક rangeMsb, પૂર્ણાંક padLsb,
                            પૂર્ણાંક padMsb, તાર્કિક hasArraySel, તાર્કિક skipRdRefresh = ખોટું) {
        v3Global.setUsesForce();
        varp->setForcedByCode();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        અચલ પૂર્ણાંક forceId = info.m_forces.size();
        File_લીટી* અચલ flp = varp->fileline();
        AstScope* અચલ scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        જો (!info.m_forceVecVscp) {
            AstCDType* અચલ forceVecDtypep = નવું AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->addTypesp(forceVecDtypep);

            AstVar* અચલ forceVecVarp
                = નવું AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doingAssign ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(ખોટું);
            forceVecVarp->isInternal(સાચું);
            varp->addNextHere(forceVecVarp);
            info.m_forceVecVscp = નવું AstVarScope{flp, scopep, forceVecVarp};
            scopep->addVarsp(info.m_forceVecVscp);
        }

        auto pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, nullptr, rhsExprp});
        ForceInfo& finfo = pair.first->second;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        જો (doingAssign()) {
            std::vector<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* અચલ refp) {
                જો (!refp->access().isReadOnly()) પાછા;
                AstVar* અચલ depVarp = refp->varp();
                જો (depVarp
                    && std::find(depVarps.begin(), depVarps.end(), depVarp) == depVarps.end()) {
                    depVarps.ધકેલો_back(depVarp);
                }
            });
            માટે (AstVar* અચલ depVarp : depVarps) m_rhsDepToForces[depVarp].ધકેલો_back(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    સ્થિર શૂન્ય collectArraySelInfo(Ast_ગાંઠ_પદાવલિ* exprp, ArraySelInfo& info) {
        જો (અચલ auto* અચલ selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = સાચું;
            collectArraySelInfo(selp->fromp(), info);
        } નહીંતર જો (auto* અચલ selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.ધકેલો_back(selp);
        } નહીંતર જો (અચલ auto* અચલ memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    સ્થિર ArraySelInfo getArraySelInfo(Ast_ગાંઠ_પદાવલિ* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        પાછા info;
    }

    સ્થિર std::vector<પૂર્ણાંક> arraySelDimSizes(અચલ ArraySelInfo& info) {
        std::vector<પૂર્ણાંક> dims;
        dims.reserve(info.m_sels.size());
        માટે (AstArraySel* અચલ selp : info.m_sels) {
            Ast_ગાંઠ_DType* અચલ dtypep = selp->fromp()->dtypep()->skipRefp();
            અચલ AstUnpackArrayDType* અચલ arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.ધકેલો_back(arrayp->declRange().elements());
        }
        પાછા dims;
    }

    સ્થિર પૂર્ણાંક flattenIndex(અચલ std::vector<પૂર્ણાંક>& indices, અચલ std::vector<પૂર્ણાંક>& dimSizes) {
        UASSERT(indices.size() == dimSizes.size(), "Array index and dimension size mismatch");
        પૂર્ણાંક index = 0;
        પૂર્ણાંક stride = 1;
        માટે (પૂર્ણાંક i = static_cast<પૂર્ણાંક>(indices.size()) - 1; i >= 0; --i) {
            index += indices[i] * stride;
            stride *= dimSizes[i];
        }
        પાછા index;
    }

    સ્થિર Ast_ગાંઠ_પદાવલિ* build_Flatten_Index_પદાવલિ(File_લીટી* flp, અચલ ArraySelInfo& info) {
        અચલ std::vector<પૂર્ણાંક> dimSizes = arraySelDimSizes(info);
        તાર્કિક allConst = સાચું;
        માટે (AstArraySel* અચલ selp : info.m_sels) {
            જો (!VN_IS(selp->bitp(), Const)) {
                allConst = ખોટું;
                તોડો;
            }
        }
        જો (allConst) {
            std::vector<પૂર્ણાંક> constIndices;
            constIndices.reserve(info.m_sels.size());
            માટે (AstArraySel* અચલ selp : info.m_sels) {
                constIndices.ધકેલો_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            પાછા makeConst32(flp, flattenIndex(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_ગાંઠ_પદાવલિ* resultp = nullptr;
        પૂર્ણાંક stride = 1;
        માટે (પૂર્ણાંક i = static_cast<પૂર્ણાંક>(info.m_sels.size()) - 1; i >= 0; --i) {
            Ast_ગાંઠ_પદાવલિ* termp = info.m_sels[i]->bitp()->clone_વૃક્ષ_Pure(ખોટું);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            જો (termp->width() < 32) termp = નવું AstExtend{flp, termp, 32};
            જો (stride != 1) termp = નવું AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? નવું AstAdd{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        પાછા resultp;
    }

    સ્થિર Ast_ગાંઠ_પદાવલિ* build_Rhs_Data_પદાવલિ(File_લીટી* flp, અચલ ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        પાછા નવું AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    શૂન્ય finalizeRhsVars() {
        માટે (VarForceInfo& info : m_varInfos) {
            AstVar* અચલ varp = info.m_varp;
            જો (info.m_forces.empty()) ચાલુ;

            AstScope* અચલ scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_લીટી* અચલ flp = varp->fileline();
            અચલ std::vector<ForceInfo*> forceps = forceInfosInIdOrder(info);

            માટે (ForceInfo* અચલ finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* અચલ rhsVarp = નવું AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doingAssign() ? "_VassignRHS" : "__VforceRHS")
                        + std::to_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(સાચું);
                rhsVarp->sigPublic(સાચું);
                rhsVarp->setForcedByCode();
                varp->addNextHere(rhsVarp);
                finfo.m_rhsVarVscp = નવું AstVarScope{flp, scopep, rhsVarp};
                scopep->addVarsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                // end
                AstAssign* અચલ rhsAssignp = નવું AstAssign{
                    flp, નવું AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                જો (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* અચલ touchCallp = નવું AstCMethodHard{
                        flp, નવું AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtypeSetVoid();
                    Ast_ગાંઠ_Stmt* અચલ touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->addNextHere(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* અચલ alwaysp
                    = નવું AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, rhsAssignp};
                Ast_Sen_વૃક્ષ* અચલ senTreep
                    = નવું Ast_Sen_વૃક્ષ{flp, નવું AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* અચલ activep = નવું AstActive{flp, "force-rhs-update", senTreep};
                activep->sen_વૃક્ષ_Storep(activep->sentreep());
                activep->addStmtsp(alwaysp);
                scopep->addBlocksp(activep);
            }

            જો (info.m_forceRdVscp) {
                જો (!m_forceRdUpdateBuilt.ઉમેરો(info.m_varVscp).second) ચાલુ;
                AstActive* અચલ activeInitp = નવું AstActive{
                    flp, "force-init",
                    નવું Ast_Sen_વૃક્ષ{flp, નવું AstSenItem{flp, AstSenItem::Static{}}}};
                activeInitp->sen_વૃક્ષ_Storep(activeInitp->sentreep());
                Ast_ગાંઠ_Stmt* initStmtp = nullptr;
                જો (AstUnpackArrayDType* અચલ arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    અચલ std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    અચલ પૂર્ણાંક innerWidth = dims.back()->subDTypep()->skipRefp()->width();
                    initStmtp = foreachUnpackedLeaf(
                        dims, [&](અચલ std::vector<પૂર્ણાંક>& idx, પૂર્ણાંક /*flat*/) -> Ast_ગાંઠ_Stmt* {
                            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = buildNestedArraySel(
                                flp, નવું AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            પાછા નવું AstAssign{flp, lhsp, makeZeroConst(varp, innerWidth)};
                        });
                } નહીંતર {
                    initStmtp = નવું AstAssign{
                        flp, નવું AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->width())};
                }
                initStmtp->addNext(createForceRdUpdateStmt(info));
                activeInitp->addStmtsp(નવું AstInitial{flp, initStmtp});
                scopep->addBlocksp(activeInitp);

                AstSenItem* itemsp = nullptr;
                auto addSenItem = [&](AstVarScope* vscp) {
                    જો (!vscp) પાછા;
                    AstSenItem* અચલ nextp = નવું AstSenItem{
                        flp, VEdgeType::ET_CHANGED, નવું AstVarRef{flp, vscp, VAccess::READ}};
                    જો (itemsp) {
                        itemsp->addNext(nextp);
                    } નહીંતર {
                        itemsp = nextp;
                    }
                };
                addSenItem(info.m_forceEnVscp);
                addSenItem(info.m_forceValVscp);
                AstVarRef* અચલ origSenRefp = નવું AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                AstSenItem* અચલ origItemp
                    = નવું AstSenItem{flp, VEdgeType::ET_CHANGED, origSenRefp};
                જો (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->addNext(origItemp);
                માટે (ForceInfo* અચલ finfop : forceps) {
                    જો (!finfop->m_isExternal) addSenItem(finfop->m_rhsVarVscp);
                }

                AstActive* અચલ activep
                    = નવું AstActive{flp, "force-rd-update", નવું Ast_Sen_વૃક્ષ{flp, itemsp}};
                activep->sen_વૃક્ષ_Storep(activep->sentreep());
                activep->addStmtsp(નવું AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr,
                                                 createForceRdUpdateStmt(info)});
                scopep->addBlocksp(activep);
            }
        }
    }

    Ast_ગાંઠ* createRhsUpdatesForWrite(File_લીટી* flp, AstVar* writtenVarp) અચલ {
        જો (!doingAssign()) પાછા nullptr;

        અચલ auto it = m_rhsDepToForces.find(writtenVarp);
        જો (it == m_rhsDepToForces.end()) પાછા nullptr;

        Ast_ગાંઠ* headp = nullptr;
        Ast_ગાંઠ* tailp = nullptr;
        માટે (અચલ ForceInfo* અચલ finfop : it->second) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            AstAssign* અચલ updatep
                = નવું AstAssign{flp, નવું AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_વૃક્ષ_Pure(ખોટું)};
            જો (tailp) {
                tailp->addNextHere(updatep);
            } નહીંતર {
                headp = updatep;
            }
            tailp = updatep;
        }
        પાછા headp;
    }

    અચલ ForceInfo& getForceInfo(AstAssignForce* forceStmtp) અચલ {
        AstVarScope* અચલ vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        અચલ VarForceInfo* અચલ varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        auto it2 = varInfo->m_forces.find(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.end(), "Force statement not found");
        પાછા it2->second;
    }

    સ્થિર તાર્કિક selOverlapsAnyForce(અચલ VarForceInfo& varInfo, પૂર્ણાંક selLsb, પૂર્ણાંક selMsb) {
        માટે (અચલ auto& pair : varInfo.m_forces) {
            જો (pair.second.m_rangeLsb <= selMsb && pair.second.m_rangeMsb >= selLsb) પાછા સાચું;
        }
        પાછા ખોટું;
    }

    Ast_ગાંઠ_પદાવલિ* createForceReadExpression(અચલ VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) અચલ {
        File_લીટી* અચલ flp = originalRefp->fileline();
        AstVarRef* અચલ refp = originalRefp->clone_વૃક્ષ_Pure(ખોટું);
        refp->access(VAccess::READ);
        પાછા createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   nullptr);
    }

    Ast_ગાંઠ_પદાવલિ* createForceReadIndexExpression(અચલ VarForceInfo& varInfo,
                                                Ast_ગાંઠ_પદાવલિ* originalExprp,
                                                Ast_ગાંઠ_પદાવલિ* indexExprp) અચલ {
        File_લીટી* અચલ flp = originalExprp->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = originalExprp->clone_વૃક્ષ_Pure(ખોટું);
        // Must be an LValue to a static variable
        VN_AS(exprp->getVAccessTargetRecurse(), VarRef)->access(VAccess::READ);
        પાછા createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_INDEX, exprp, originalExprp,
                                   indexExprp);
    }

    સ્થિર Ast_ગાંઠ_પદાવલિ* rebuildSelPath(Ast_ગાંઠ_પદાવલિ* pathp, Ast_ગાંઠ_પદાવલિ* baseExprp) {
        જો (અચલ AstSel* અચલ selp = VN_CAST(pathp, Sel)) {
            Ast_ગાંઠ_પદાવલિ* અચલ fromp = rebuildSelPath(selp->fromp(), baseExprp);
            AstSel* અચલ outp
                = નવું AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->widthConst()};
            outp->dtypeFrom(selp);
            પાછા outp;
        }
        પાછા baseExprp;
    }
};

// Split deassign concat LHS before converting to release internals.
સ્થિર શૂન્ય splitDeassign(AstDeassign* nodep) {
    AstConcat* અચલ concatp = VN_CAST(nodep->lhsp(), Concat);
    જો (!concatp) પાછા;

    File_લીટી* અચલ flp = nodep->fileline();
    AstDeassign* અચલ newLp = નવું AstDeassign{flp, concatp->lhsp()->unlinkFrBack()};
    AstDeassign* અચલ newRp = નવું AstDeassign{flp, concatp->rhsp()->unlinkFrBack()};
    Ast_ગાંઠ_પદાવલિ* અચલ conp = concatp->unlinkFrBack();
    nodep->replaceWith(newLp);
    newLp->addNextHere(newRp);
    VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    VL_DO_DANGLING(conp->delete_વૃક્ષ(), conp);

    splitDeassign(newLp);
    splitDeassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

વર્ગ ForceDiscoveryVisitor final : જાહેર VNVisitorConst {
    ForceState& m_state;
    તાર્કિક m_inClockedActive = ખોટું;

    શૂન્ય buildForceableUnpackedArray(AstVarScope* અચલ nodep,
                                     AstUnpackArrayDType* અચલ arrDtypep) {
        AstVar* અચલ varp = nodep->varp();
        અચલ std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.empty(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        અચલ Ast_ગાંઠ_DType* અચલ leafDtypep = dims.back()->subDTypep()->skipRefp();
        અચલ AstBasicDType* અચલ innerBasicp = leafDtypep->basicp();
        જો (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            પાછા;
        }

        File_લીટી* અચલ flp = varp->fileline();
        અચલ પૂર્ણાંક innerWidth = leafDtypep->width();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* અચલ enVscp = info.m_forceEnVscp;
        AstVarScope* અચલ valVscp = info.m_forceValVscp;

        AstSenItem* અચલ itemsp = નવું AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                  નવું AstVarRef{flp, enVscp, VAccess::READ}};
        AstActive* અચલ activep = નવું AstActive{flp, "force-update", નવું Ast_Sen_વૃક્ષ{flp, itemsp}};
        activep->sen_વૃક્ષ_Storep(activep->sentreep());

        Ast_ગાંઠ_Stmt* અચલ alwaysBodyHeadp = ForceState::foreachUnpackedLeaf(
            dims, [&](અચલ std::vector<પૂર્ણાંક>& idx, પૂર્ણાંક flat) -> Ast_ગાંઠ_Stmt* {
                AstVarRef* અચલ origRefp = નવું AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_ગાંઠ_પદાવલિ* અચલ origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_ગાંઠ_પદાવલિ* અચલ enSelp = ForceState::buildNestedArraySel(
                    flp, નવું AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_ગાંઠ_પદાવલિ* અચલ valSelp = ForceState::buildNestedArraySel(
                    flp, નવું AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_ગાંઠ_પદાવલિ* અચલ forceExprp = નવું AstOr{
                    flp, નવું AstAnd{flp, enSelp, valSelp},
                    નવું AstAnd{flp, નવું AstNot{flp, enSelp->clone_વૃક્ષ_Pure(ખોટું)}, origSelp}};
                Ast_ગાંઠ_પદાવલિ* અચલ lhsSelp = ForceState::buildNestedArraySel(
                    flp, નવું AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                AstAssignForce* અચલ forceAssignp = નવું AstAssignForce{flp, lhsSelp, forceExprp};
                forceAssignp->user2(સાચું);

                Ast_ગાંઠ_પદાવલિ* અચલ rhsClonep = forceExprp->clone_વૃક્ષ_Pure(ખોટું);
                rhsClonep->foreach([varp](AstVarRef* અચલ r) {
                    જો (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/innerWidth - 1,
                                           /*hasArraySel=*/સાચું, /*skipRdRefresh=*/સાચું);
                પાછા forceAssignp;
            });
        activep->addStmtsp(નવું AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, alwaysBodyHeadp});
        nodep->scopep()->addBlocksp(activep);
    }

    શૂન્ય visit(AstAssignForce* nodep) override {
        જો (nodep->user2()) પાછા;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* અચલ lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* અચલ forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, સાચું);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_ગાંઠ_પદાવલિ* અચલ rhsExprp = nodep->rhsp()->clone_વૃક્ષ_Pure(ખોટું);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    શૂન્ય visit(AstAssign* nodep) override {
        જો (m_state.doingAssign() && m_inClockedActive) {
            જો (AstVarRef* અચલ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(AstActive* nodep) override {
        VL_RESTORER(m_inClockedActive);
        m_inClockedActive = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(AstVarScope* nodep) override {
        જો (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            જો (m_state.doingAssign()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                પાછા;
            }

            જો (AstUnpackArrayDType* અચલ arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                પાછા;
            }

            અચલ AstBasicDType* અચલ bdtypep = nodep->varp()->basicp();
            જો (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* અચલ varp = nodep->varp();
            File_લીટી* અચલ flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* અચલ enVscp = info.m_forceEnVscp;
            AstVarScope* અચલ valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            AstSenItem* અચલ itemsp = નવું AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                      નવું AstVarRef{flp, enVscp, VAccess::READ}};
            AstActive* અચલ activep
                = નવું AstActive{flp, "force-update", નવું Ast_Sen_વૃક્ષ{flp, itemsp}};
            activep->sen_વૃક્ષ_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* અચલ refp = નવું AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* અચલ enRefp = નવું AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* અચલ valRefp = નવું AstVarRef{flp, valVscp, VAccess::READ};
            અચલ AstBasicDType* અચલ basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_ગાંઠ_પદાવલિ* અચલ forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstOr{
                            flp, નવું AstAnd{flp, enRefp, valRefp},
                            નવું AstAnd{flp, નવું AstNot{flp, enRefp->clone_વૃક્ષ_Pure(ખોટું)}, refp}})
                      : static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstCond{flp, enRefp, valRefp, refp});
            AstAssignForce* અચલ forceAssignp
                = નવું AstAssignForce{flp, નવું AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(સાચું);
            activep->addStmtsp(નવું AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, forceAssignp});
            nodep->scopep()->addBlocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_ગાંઠ_પદાવલિ* અચલ rhsClonep = forceExprp->clone_વૃક્ષ_Pure(ખોટું);
            rhsClonep->foreach([varp](AstVarRef* અચલ refp) {
                જો (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            અચલ તાર્કિક bitwiseVar = ForceState::isBitwiseDType(varp);
            અચલ પૂર્ણાંક padMsb = bitwiseVar ? (varp->width() - 1) : 0;
            પૂર્ણાંક rangeLsb = 0;
            પૂર્ણાંક rangeMsb = padMsb;
            જો (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, ખોટું);
        }
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    explicit ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextConstNull(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

વર્ગ ForceConvertVisitor final : જાહેર VNVisitor {
    ForceState& m_state;

    શૂન્ય visit(AstAssignForce* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp();
        AstVarRef* અચલ lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* અચલ forcedVarp = lhsVarRefp->varp();

        અચલ ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        અચલ ForceState::VarForceInfo* અચલ varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_લીટી* અચલ flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        AstAssign* અચલ rhsAssignp
            = નવું AstAssign{flp, નવું AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_વૃક્ષ_Pure(ખોટું)};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        અચલ તાર્કિક bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        જો (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_ગાંઠ_પદાવલિ* baseExprp = nodep->rhsp()->clone_વૃક્ષ_Pure(ખોટું);
            baseExprp->foreach(
                [](AstVarRef* અચલ refp) { ForceState::markNonReplaceable(refp); });
            જો (bitwiseForcedVar) {
                baseExprp = ForceState::zeroPadToBaseWidth(baseExprp, forcedVarp->width(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            જો (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* અચલ maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_ગાંઠ_પદાવલિ* અચલ valReadp
                    = નવું AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_ગાંઠ_પદાવલિ* અચલ valWritep
                    = નવું AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_ગાંઠ_પદાવલિ* અચલ notMaskp = નવું AstNot{flp, maskConstp};
                Ast_ગાંઠ_પદાવલિ* અચલ maskedOldp = નવું AstAnd{flp, valReadp, notMaskp};
                Ast_ગાંઠ_પદાવલિ* અચલ newValp = નવું AstOr{flp, maskedOldp, baseExprp};
                valAssignp = નવું AstAssign{flp, valWritep, newValp};

                Ast_ગાંઠ_પદાવલિ* અચલ enReadp
                    = નવું AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_ગાંઠ_પદાવલિ* અચલ enWritep
                    = નવું AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_ગાંઠ_પદાવલિ* અચલ newEnp
                    = નવું AstOr{flp, enReadp, maskConstp->clone_વૃક્ષ_Pure(ખોટું)};
                enAssignp = નવું AstAssign{flp, enWritep, newEnp};
            } નહીંતર {
                AstConst* અચલ oneConstp = ForceState::makeRangeMaskConst(nodep, 1, 0, 0);
                Ast_ગાંઠ_પદાવલિ* અચલ rhsValp = ForceState::cast_To_ગાંઠ_DType(baseExprp, forcedVarp);
                valAssignp = નવું AstAssign{
                    flp, નવું AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = નવું AstAssign{
                    flp, નવું AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        અચલ AstSel* અચલ selLhsp = VN_CAST(lhsp, Sel);
        અચલ તાર્કિક arrayBitSel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->width();
        Ast_ગાંઠ_પદાવલિ* અચલ rhsDatap = ForceState::build_Rhs_Data_પદાવલિ(flp, info);
        AstCExpr* અચલ rhsAddrp = નવું AstCExpr{flp};
        rhsAddrp->add("&(");
        rhsAddrp->add(rhsDatap);
        rhsAddrp->add(")");
        AstCMethodHard* અચલ addForceCallp = નવું AstCMethodHard{
            flp, નવું AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ADD,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        addForceCallp->addPinsp(rhsAddrp);
        addForceCallp->addPinsp(
            ForceState::makeConst32(flp, arrayBitSel ? info.m_padLsb : info.m_rangeLsb));
        જો (arrayBitSel) {
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padLsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padMsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, selLhsp->fromp()->width()));
        }
        addForceCallp->dtypeSetVoid();
        Ast_ગાંઠ_Stmt* અચલ stmtp = addForceCallp->makeStmt();

        Ast_ગાંઠ* tailp = rhsAssignp;
        જો (valAssignp) {
            tailp->addNextHere(valAssignp);
            tailp = valAssignp;
        }
        જો (enAssignp) {
            tailp->addNextHere(enAssignp);
            tailp = enAssignp;
        }
        tailp->addNextHere(stmtp);
        જો (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp();
        AstVarRef* અચલ lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* અચલ releasedVarp = lhsVarRefp->varp();

        અચલ ForceState::VarForceInfo* અચલ varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        જો (!varInfo) {
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }
        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        File_લીટી* અચલ flp = nodep->fileline();

        અચલ ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, ખોટું);

        અચલ AstSel* અચલ selLhsp = VN_CAST(lhsp, Sel);
        અચલ તાર્કિક arrayBitSel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->width();
        AstCMethodHard* અચલ releaseCallp = નવું AstCMethodHard{
            flp, નવું AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        જો (arrayBitSel) {
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtypeSetVoid();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_ગાંઠ_Stmt* અચલ releasep = releaseCallp->makeStmt();

        AstAssign* clearEnp = nullptr;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        જો (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_ગાંઠ_પદાવલિ* અચલ enWritep
                = નવું AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            જો (ForceState::isBitwiseDType(releasedVarp)) {
                અચલ પૂર્ણાંક varWidth = releasedVarp->width();
                જો (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == varWidth - 1) {
                    clearEnp
                        = નવું AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, varWidth)};
                } નહીંતર {
                    // forceEn = forceEn & ~mask(range);
                    Ast_ગાંઠ_પદાવલિ* અચલ enReadp
                        = નવું AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* અચલ maskConst = ForceState::makeRangeMaskConst(
                        nodep, varWidth, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_ગાંઠ_પદાવલિ* અચલ newEnp
                        = નવું AstAnd{flp, enReadp, નવું AstNot{flp, maskConst}};
                    clearEnp = નવું AstAssign{flp, enWritep, newEnp};
                }
            } નહીંતર {
                clearEnp = નવું AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        અચલ AstSel* અચલ selp = VN_CAST(lhsp, Sel);
        Ast_ગાંઠ_પદાવલિ* અચલ basep = selp ? selp->fromp() : lhsp;

        Ast_ગાંઠ* stmtListp = releasep;
        જો (clearEnp) {
            clearEnp->addNextHere(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        અચલ તાર્કિક fullBitwiseRelease
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->width() - 1;
        જો (!releasedVarp->isContinuously()
            && !(m_state.doingAssign() && m_state.hasClockedWrite(releasedVarp)
                 && fullBitwiseRelease)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            અચલ તાર્કિક hasOpaquePath = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_ગાંઠ_પદાવલિ* forceReadp = nullptr;
            જો (hasOpaquePath) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } નહીંતર જો (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, basep,
                    ForceState::build_Flatten_Index_પદાવલિ(flp, rangeInfo.m_arrayInfo));
                જો (selp) { forceReadp = ForceState::rebuildSelPath(lhsp, forceReadp); }
            } નહીંતર {
                forceReadp
                    = selp ? ForceState::rebuildSelPath(
                                 lhsp, m_state.createForceReadExpression(*varInfo, lhsVarRefp))
                           : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            }
            AstAssign* અચલ assignp = નવું AstAssign{flp, lhsp->clone_વૃક્ષ_Pure(ખોટું), forceReadp};
            assignp->addNextHere(stmtListp);
            stmtListp = assignp;
        }

        જો (varInfo->m_forceRdVscp) stmtListp->addNext(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

વર્ગ ForceReplaceVisitor final : જાહેર VNVisitor {
    અચલ ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_ગાંઠ_Stmt* m_stmtp = nullptr;
    તાર્કિક m_inLogic = ખોટું;

    શૂન્ય iterateLogic(Ast_ગાંઠ* nodep) {
        VL_RESTORER(m_inLogic);
        m_inLogic = સાચું;
        iterateChildren(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstAssign* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        જો (AstVarRef* અચલ lhsp = VN_CAST(nodep->lhsp()->baseFromp(સાચું), VarRef)) {
            જો (Ast_ગાંઠ* અચલ updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->addNextHere(updatep);
            }
        }
    }
    શૂન્ય visit(AstAssignCont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateAndNextNull(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    શૂન્ય visit(AstDeassign*) override {}
    શૂન્ય visit(AstCFunc* nodep) override { iterateLogic(nodep); }
    શૂન્ય visit(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override { iterateLogic(nodep); }
    શૂન્ય visit(AstAlways* nodep) override {
        જો (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            પાછા;
        }
        iterateLogic(nodep);
    }
    શૂન્ય visit(AstSenItem* nodep) override { iterateLogic(nodep); }
    શૂન્ય visit(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* અચલ refp = VN_CAST(nodep->fromp(), VarRef);
        જો (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            visit(static_cast<Ast_ગાંઠ*>(nodep));
            પાછા;
        }

        AstVar* અચલ varp = refp->varp();
        અચલ ForceState::VarForceInfo* અચલ varInfo = m_state.getVarInfo(refp->varScopep());
        જો (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.empty()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            visit(static_cast<Ast_ગાંઠ*>(nodep));
            પાછા;
        }

        જો (અચલ AstConst* અચલ lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            અચલ પૂર્ણાંક selLsb = lsbConstp->toSInt();
            અચલ પૂર્ણાંક selMsb = selLsb + nodep->width() - 1;
            જો (!varp->isSigPublic()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                visit(static_cast<Ast_ગાંઠ*>(nodep));
                પાછા;
            }
        }

        File_લીટી* અચલ flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* અચલ refClonep = refp->clone_વૃક્ષ_Pure(ખોટું);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* અચલ callp = નવું AstCMethodHard{
            flp, નવું AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->width())};
        callp->addPinsp(refClonep);
        callp->addPinsp(nodep->lsbp()->clone_વૃક્ષ_Pure(ખોટું));
        callp->addPinsp(ForceState::makeConst32(flp, nodep->width()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstArraySel* nodep) override {
        જો (અચલ AstArraySel* અચલ backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            જો (backSelp->fromp() == nodep) {
                iterateChildren(nodep);
                પાછા;
            }
        }

        Ast_ગાંઠ* અચલ basep = nodep->baseFromp(સાચું);
        AstVarRef* અચલ baseRefp = VN_CAST(basep, VarRef);
        જો (!baseRefp) {
            iterateChildren(nodep);
            પાછા;
        }
        AstVar* અચલ varp = baseRefp->varp();
        અચલ ForceState::VarForceInfo* અચલ varInfo = m_state.getVarInfo(baseRefp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        જો (m_state.isNotReplaceable(baseRefp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            પાછા;
        }

        જો (!baseRefp->access().isReadOnly()) {
            iterateChildren(nodep);
            પાછા;
        }
        જો (m_state.doingAssign() && varInfo->m_forceRdVscp) {
            baseRefp->varp(varInfo->m_forceRdVscp->varp());
            baseRefp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            પાછા;
        }
        અચલ ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        માટે (AstArraySel* અચલ selp : arrayInfo.m_sels) iterateAndNextNull(selp->bitp());
        Ast_ગાંઠ_પદાવલિ* અચલ indexExprp
            = ForceState::build_Flatten_Index_પદાવલિ(nodep->fileline(), arrayInfo);
        Ast_ગાંઠ_પદાવલિ* અચલ readExprp
            = m_state.createForceReadIndexExpression(*varInfo, nodep, indexExprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય visit(AstVarRef* nodep) override {
        જો (m_state.isNotReplaceable(nodep)) પાછા;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        જો (અચલ AstArraySel* અચલ backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            જો (backSelp->fromp() == nodep) પાછા;
        }

        અચલ ForceState::VarForceInfo* અચલ varInfo = m_state.getVarInfo(nodep->varScopep());
        જો (!varInfo) પાછા;

        જો (varInfo->m_forceRdVscp) {
            જો (nodep->access().isRW()) {
                જો (m_inLogic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                પાછા;
            }
            જો (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_ગાંઠ_પદાવલિ* અચલ readp
                    = m_state.createForceRdExpression(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા;
            }
            જો (m_inLogic && m_stmtp) {
                m_stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
            }
            પાછા;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        જો (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            પાછા;
        }

        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        જો (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } નહીંતર જો (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_ગાંઠ_પદાવલિ* અચલ readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (ForceState::isOutermostOpaquePathSelector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_ગાંઠ_પદાવલિ* અચલ exprp = VN_AS(nodep, ગાંઠ_પદાવલિ);
            Ast_ગાંઠ* અચલ basep = exprp->baseFromp(સાચું);
            AstVarRef* અચલ baseRefp = VN_CAST(basep, VarRef);
            જો (baseRefp) {
                AstVar* અચલ varp = baseRefp->varp();
                જો (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    અચલ ForceState::VarForceInfo* અચલ varInfo
                        = m_state.getVarInfo(baseRefp->varScopep());
                    જો (!m_state.isNotReplaceable(baseRefp) && varInfo) {
                        અચલ પૂર્ણાંક forcePathIndex = varInfo->findForcePathIndex(exprp);
                        જો (forcePathIndex >= 0) {
                            જો (!baseRefp->access().isReadOnly()) પાછા;
                            Ast_ગાંઠ_પદાવલિ* અચલ readExprp = m_state.createForceReadIndexExpression(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), forcePathIndex));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                            પાછા;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

જાહેર:
    explicit ForceReplaceVisitor(AstNetlist* nodep, અચલ ForceState& state)
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

સ્થિર શૂન્ય forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::ProtectedVarRefSet& permanentlyProtected,
                         ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    જો (!v3Global.hasForceableSignals()) પાછા;
    ForceState state{ખોટું, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_વૃક્ષ("force", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

સ્થિર શૂન્ય assignAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::ProtectedVarRefSet& permanentlyProtected,
                          ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    જો (!v3Global.hasAssignDeassign()) પાછા;

    std::vector<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.ધકેલો_back(deassignp); });
    માટે (AstDeassign* અચલ deassignp : deassignps) splitDeassign(deassignp);

    std::vector<AstAssignCont*> assignContps;
    deassignps.clear();
    nodep->foreach([&](Ast_ગાંઠ_Stmt* nodep) {
        જો (AstAssignCont* અચલ assignContp = VN_CAST(nodep, AssignCont)) {
            assignContps.ધકેલો_back(assignContp);
        } નહીંતર જો (AstDeassign* અચલ deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.ધકેલો_back(deassignp);
        }
    });

    માટે (AstAssignCont* અચલ assignp : assignContps) {
        assignp->replaceWith(નવું AstAssignForce{assignp->fileline(),
                                                assignp->lhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
        VL_DO_DANGLING(assignp->delete_વૃક્ષ(), assignp);
    }
    માટે (AstDeassign* અચલ deassignp : deassignps) {
        deassignp->replaceWith(
            નવું AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_વૃક્ષ_Pure(સાચું)});
        VL_DO_DANGLING(deassignp->delete_વૃક્ષ(), deassignp);
    }
    ForceState state{સાચું, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_વૃક્ષ("assign-deassign", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

શૂન્ય V3Force::forceAndAssignAll(AstNetlist* nodep) {
    અચલ VNUser3InUse user3InUse;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::ProtectedVarRefSet permanentlyProtected;
    ForceState::BuiltVarScopeSet forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    assignAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
