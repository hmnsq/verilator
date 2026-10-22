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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Force.h"

#অন্তর্ভাব "V3AstUserAllocator.h"
#অন্তর্ভাব "V3Stats.h"
#অন্তর্ভাব "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

বর্গ ForceState final {
সার্বজনিক:
    সংরচনা ForceRange VL_NOT_FINAL {
        পূর্ণাঙ্ক m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        পূর্ণাঙ্ক m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        পূর্ণাঙ্ক m_padLsb = 0;  // Bit positions for RHS padding
        পূর্ণাঙ্ক m_padMsb = 0;  // Bit positions for RHS padding
    };

    সংরচনা ForceInfo final : ForceRange {
        // MEMBERS
        পূর্ণাঙ্ক m_forceId = 0;  // Unique (per signal) variable of this force assignment
        তার্কিক m_hasArraySel = অসত্য;  // If this has an array select on LHS
        তার্কিক m_isExternal = অসত্য;  // Synthetic force for the public force controls
        তার্কিক m_skipRdRefresh = অসত্য;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = nullptr;  // Scope of the var containing RHSID
        Ast_গ্রন্থি_রাশিমালা* m_rhsExprp = nullptr;  // Expression on RHS of this force assignment

        ForceInfo() = স্বভাব;
        ForceInfo(পূর্ণাঙ্ক rangeLsb, পূর্ণাঙ্ক rangeMsb, পূর্ণাঙ্ক padLsb, পূর্ণাঙ্ক padMsb, পূর্ণাঙ্ক forceId,
                  তার্কিক hasArraySel, AstVarScope* rhsVarVscp, Ast_গ্রন্থি_রাশিমালা* rhsExprp)
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

    সংরচনা VarForceInfo final {
        AstVarScope* m_forceVecVscp = nullptr;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = nullptr;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = nullptr;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = nullptr;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = nullptr;  // Scope of the forced (original) variable
        AstVar* m_varp = nullptr;  // The forced (original) variable
        AstScope* m_scopep = nullptr;  // Scope containing the forced variable
        std::unordered_map<AstAssignForce*, ForceInfo> m_forces;
        std::unordered_map<string, পূর্ণাঙ্ক> m_forcePathToIndex;
        পূর্ণাঙ্ক m_nextForcePathIndex = 1;  // Start at 1 so 0 can be the base path (whole signal)

    গুপ্ত:
        স্থির শূন্য build_Force_Path_চাবি_Recurse(Ast_গ্রন্থি_রাশিমালা* nodep, string& out) {
            যদি (VN_IS(nodep, VarRef)) প্রত্যাগম;
            যদি (ধ্রুব AstSel* ধ্রুব selp = VN_CAST(nodep, Sel)) {
                build_Force_Path_চাবি_Recurse(selp->fromp(), out);
                out += "|S" + cvtToStr(selp->lsbConst()) + ":" + cvtToStr(selp->widthConst());
                প্রত্যাগম;
            }
            যদি (AstStructSel* ধ্রুব selp = VN_CAST(nodep, StructSel)) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব fromp = selp->fromp();
                build_Force_Path_চাবি_Recurse(fromp, out);
                out += "|T" + selp->name();
                প্রত্যাগম;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        স্থির string force_Path_চাবি(Ast_গ্রন্থি_রাশিমালা* nodep) {
            string out;
            build_Force_Path_চাবি_Recurse(nodep, out);
            প্রত্যাগম out;
        }

    সার্বজনিক:
        পূর্ণাঙ্ক getOrCreateForcePathIndex(Ast_গ্রন্থি_রাশিমালা* nodep) {
            ধ্রুব auto pair
                = m_forcePathToIndex.emplace(force_Path_চাবি(nodep), m_nextForcePathIndex);
            যদি (pair.second) ++m_nextForcePathIndex;
            প্রত্যাগম pair.first->second;
        }

        পূর্ণাঙ্ক findForcePathIndex(Ast_গ্রন্থি_রাশিমালা* nodep) ধ্রুব {
            ধ্রুব auto it = m_forcePathToIndex.find(force_Path_চাবি(nodep));
            প্রত্যাগম it != m_forcePathToIndex.end() ? it->second : -1;
        }
    };

    সংরচনা ForceHelperVars final {
        AstVar* m_rdVarp = nullptr;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = nullptr;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = nullptr;  // The __VforceVal (force value) variable
    };

    সংরচনা ArraySelInfo final {
        std::vector<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        তার্কিক m_hasBitSel = অসত্য;  // Bit-select was found along the path too
    };

    সংরচনা ForceRangeInfo final : ForceRange {
        তার্কিক m_hasArraySel = অসত্য;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

গুপ্ত:
    using ScopeVarCache = std::unordered_map<ধ্রুব AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    ধ্রুব VNUser1InUse m_user1InUse;
    ধ্রুব VNUser2InUse m_user2InUse;

সার্বজনিক:
    using ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    using ProtectedVarRefSet = std::unordered_set<ধ্রুব AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    using BuiltVarScopeSet = std::unordered_set<ধ্রুব AstVarScope*>;

গুপ্ত:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::vector<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, পূর্ণাঙ্ক> m_varToId;
    std::unordered_set<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::vector<ForceInfo*>> m_rhsDepToForces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    তার্কিক m_doingAssign = অসত্য;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    ProtectedVarRefSet& m_permanentlyProtected;
    BuiltVarScopeSet& m_forceRdUpdateBuilt;

সার্বজনিক:
    ForceState(তার্কিক doingAssign, ForceHelperVarsByVar& forceHelperVarsByVar,
               ProtectedVarRefSet& permanentlyProtected, BuiltVarScopeSet& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doingAssign{doingAssign}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    স্থির AstConst* makeZeroConst(Ast_গ্রন্থি* nodep, পূর্ণাঙ্ক width) {
        V3_সংখ্যা zero{nodep, width};
        zero.setAllBits0();
        প্রত্যাগম নূতন AstConst{nodep->fileline(), zero};
    }

    স্থির AstConst* makeConst32(File_সারি* flp, পূর্ণাঙ্ক মান) {
        প্রত্যাগম নূতন AstConst{flp, AstConst::Widthed_মান{}, 32, static_cast<uint32_t>(মান)};
    }

    স্থির AstConst* makeRangeMaskConst(Ast_গ্রন্থি* nodep, পূর্ণাঙ্ক width, পূর্ণাঙ্ক lsb, পূর্ণাঙ্ক msb) {
        V3_সংখ্যা mask{nodep, width};
        mask.setAllBits0();
        প্রতি (পূর্ণাঙ্ক bit = lsb; bit <= msb; ++bit) mask.setBit(bit, 1);
        প্রত্যাগম নূতন AstConst{nodep->fileline(), mask};
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* zeroPadToBaseWidth(Ast_গ্রন্থি_রাশিমালা* exprp, পূর্ণাঙ্ক baseWidth, পূর্ণাঙ্ক padLsb,
                                           পূর্ণাঙ্ক padMsb) {
        যদি (baseWidth <= 0) প্রত্যাগম exprp;
        ধ্রুব পূর্ণাঙ্ক নিচের_Pad = padLsb;
        ধ্রুব পূর্ণাঙ্ক উপরের_Pad = baseWidth - (padMsb + 1);
        যদি (নিচের_Pad > 0) {
            exprp = নূতন AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, নিচের_Pad)};
        }
        যদি (উপরের_Pad > 0) {
            exprp = নূতন AstConcat{exprp->fileline(), makeZeroConst(exprp, উপরের_Pad), exprp};
        }
        প্রত্যাগম exprp;
    }

    স্থির তার্কিক isUnpackedArrayDType(ধ্রুব Ast_গ্রন্থি_DType* dtypep) {
        প্রত্যাগম VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    স্থির তার্কিক isBitwiseDType(ধ্রুব Ast_গ্রন্থি* nodep) {
        ধ্রুব AstBasicDType* ধ্রুব basicp = nodep->dtypep()->skipRefp()->basicp();
        প্রত্যাগম basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* cast_To_গ্রন্থি_DType(Ast_গ্রন্থি_রাশিমালা* exprp, Ast_গ্রন্থি* dtypeFromp) {
        ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtypep = dtypeFromp->dtypep()->skipRefp();
        ধ্রুব AstBasicDType* ধ্রুব basicp = dtypep->basicp();
        যদি (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            প্রত্যাগম exprp;
        }
        প্রত্যাগম নূতন AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    স্থির শূন্য markNonReplaceable(AstVarRef* ধ্রুব nodep) { nodep->user1SetOnce(); }
    তার্কিক isNotReplaceable(ধ্রুব AstVarRef* ধ্রুব nodep) ধ্রুব {
        প্রত্যাগম nodep->user1() || m_permanentlyProtected.গুনতি(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    শূন্য markPermanentlyProtected(AstVarRef* ধ্রুব nodep) ধ্রুব {
        markNonReplaceable(nodep);
        m_permanentlyProtected.ঢোকাও(nodep);
    }

    স্থির std::vector<ForceInfo*> forceInfosInIdOrder(VarForceInfo& info) {
        std::vector<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        প্রতি (auto& it : info.m_forces) forceps.ঠেলো_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](ধ্রুব ForceInfo* ap, ধ্রুব ForceInfo* bp) {
            প্রত্যাগম ap->m_forceId < bp->m_forceId;
        });
        প্রত্যাগম forceps;
    }

    স্থির std::vector<ধ্রুব ForceInfo*> forceInfosInIdOrder(ধ্রুব VarForceInfo& info) {
        std::vector<ধ্রুব ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        প্রতি (ধ্রুব auto& it : info.m_forces) forceps.ঠেলো_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](ধ্রুব ForceInfo* ap, ধ্রুব ForceInfo* bp) {
            প্রত্যাগম ap->m_forceId < bp->m_forceId;
        });
        প্রত্যাগম forceps;
    }

    স্থির তার্কিক isOpaquePathSelector(ধ্রুব Ast_গ্রন্থি* nodep) {
        প্রত্যাগম VN_IS(nodep, Sel) || VN_IS(nodep, গ্রন্থি_Sel) || VN_IS(nodep, StructSel);
    }

    স্থির তার্কিক isOutermostOpaquePathSelector(ধ্রুব Ast_গ্রন্থি* nodep) {
        যদি (!isOpaquePathSelector(nodep)) প্রত্যাগম অসত্য;
        ধ্রুব Ast_গ্রন্থি* ধ্রুব backp = nodep->backp();
        প্রত্যাগম !backp || !isOpaquePathSelector(backp);
    }

    স্থির AstVarRef* getOneVarRef(Ast_গ্রন্থি_রাশিমালা* forceStmtp) {
        Ast_গ্রন্থি* ধ্রুব basep = forceStmtp->baseFromp(সত্য);
        যদি (AstSampled* sampledp = VN_CAST(basep, Sampled))
            যদি (Ast_গ্রন্থি_রাশিমালা* exprp = VN_CAST(sampledp->exprp(), গ্রন্থি_রাশিমালা))
                প্রত্যাগম getOneVarRef(exprp);
        AstVarRef* ধ্রুব varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        প্রত্যাগম varRefp;
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* buildNestedArraySel(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* fromp,
                                            ধ্রুব std::vector<পূর্ণাঙ্ক>& indicies) {
        Ast_গ্রন্থি_রাশিমালা* curp = fromp;
        প্রতি (ধ্রুব পূর্ণাঙ্ক idx : indicies) curp = নূতন AstArraySel{flp, curp, idx};
        প্রত্যাগম curp;
    }

    template <typename Fn>
    স্থির Ast_গ্রন্থি_Stmt* foreachUnpackedLeaf(ধ্রুব std::vector<AstUnpackArrayDType*>& dims,
                                            Fn buildLeaf) {
        Ast_গ্রন্থি_Stmt* headp = nullptr;
        Ast_গ্রন্থি_Stmt* tailp = nullptr;
        যদি (dims.empty()) প্রত্যাগম nullptr;
        পূর্ণাঙ্ক total = 1;
        প্রতি (ধ্রুব AstUnpackArrayDType* ধ্রুব d : dims) total *= d->elementsConst();
        যদি (total <= 0) প্রত্যাগম nullptr;
        std::vector<পূর্ণাঙ্ক> idx(dims.size(), 0);
        প্রতি (পূর্ণাঙ্ক flat = 0; flat < total; ++flat) {
            Ast_গ্রন্থি_Stmt* ধ্রুব stmtp = buildLeaf(idx, flat);
            যদি (!headp) {
                headp = stmtp;
            } অন্যথা {
                tailp->addNext(stmtp);
            }
            tailp = stmtp;
            প্রতি (পূর্ণাঙ্ক d = static_cast<পূর্ণাঙ্ক>(dims.size()) - 1; d >= 0; --d) {
                যদি (++idx[d] < dims[d]->elementsConst()) ভঙ্গ;
                idx[d] = 0;
            }
        }
        প্রত্যাগম headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_গ্রন্থি_রাশিমালা* lhsp, AstVar* varp,
                                     তার্কিক requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.empty();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->width() - 1) : 0;

        যদি (ধ্রুব AstSel* ধ্রুব outerSelp = VN_CAST(lhsp, Sel)) {
            পূর্ণাঙ্ক totalLsb = 0;
            প্রতি (Ast_গ্রন্থি_রাশিমালা* curp = lhsp; ধ্রুব AstSel* ধ্রুব selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                যদি (requireConstRangeSelect) {
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
        যদি (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::vector<পূর্ণাঙ্ক> indices;
            indices.reserve(info.m_arrayInfo.m_sels.size());
            প্রতি (AstArraySel* ধ্রুব selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.ঠেলো_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            info.m_rangeLsb = flattenIndex(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } অন্যথা যদি (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        যদি (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            ধ্রুব পূর্ণাঙ্ক index = varInfo.getOrCreateForcePathIndex(lhsp);
            info.m_rangeLsb = index;
            info.m_rangeMsb = index;
        }
        প্রত্যাগম info;
    }

    Ast_গ্রন্থি_রাশিমালা* add_Rhs_মান_Reads(ধ্রুব VarForceInfo& varInfo, Ast_গ্রন্থি_রাশিমালা* exprp) ধ্রুব {
        যদি (!doingAssign()) প্রত্যাগম exprp;

        ধ্রুব std::vector<ধ্রুব ForceInfo*> forceps = forceInfosInIdOrder(varInfo);
        যদি (forceps.empty()) প্রত্যাগম exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* ধ্রুব cexprp = নূতন AstCExpr{exprp->fileline(), AstCExpr::Pure{}};
        cexprp->dtypeFrom(exprp);
        cexprp->add("(");
        প্রতি (ধ্রুব ForceInfo* ধ্রুব finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* ধ্রুব refp
                = নূতন AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->add("(void)(");
            cexprp->add(refp);
            cexprp->add("), ");
        }
        cexprp->add(exprp);
        cexprp->add(")");
        প্রত্যাগম cexprp;
    }

    Ast_গ্রন্থি_রাশিমালা* createForceReadCall(ধ্রুব VarForceInfo& varInfo, File_সারি* flp, VCMethod method,
                                     Ast_গ্রন্থি_রাশিমালা* originalExprp, Ast_গ্রন্থি* dtypeFromp,
                                     Ast_গ্রন্থি_রাশিমালা* indexExprp) ধ্রুব {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব origValp
            = add_Rhs_মান_Reads(varInfo, cast_To_গ্রন্থি_DType(originalExprp, dtypeFromp));

        AstCMethodHard* ধ্রুব callp = নূতন AstCMethodHard{
            flp, নূতন AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, method, origValp};
        যদি (indexExprp) callp->addPinsp(indexExprp);
        callp->dtypeFrom(dtypeFromp);
        প্রত্যাগম callp;
    }

    Ast_গ্রন্থি_রাশিমালা* createForceRdExpression(ধ্রুব VarForceInfo& varInfo, File_সারি* flp) ধ্রুব {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* ধ্রুব varp = varInfo.m_varVscp->varp();
        যদি (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            যদি (doingAssign()) { প্রত্যাগম নূতন AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, সত্য);
            AstVarRef* ধ্রুব baseRefp = নূতন AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(baseRefp);
            প্রত্যাগম createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, baseRefp, varp,
                                       nullptr);
        }
        AstVarRef* ধ্রুব baseRefp = নূতন AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(baseRefp);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব enRefp = নূতন AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব valRefp = নূতন AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        যদি (isBitwiseDType(varp)) {
            প্রত্যাগম নূতন AstOr{
                flp, নূতন AstAnd{flp, enRefp, valRefp},
                নূতন AstAnd{flp, নূতন AstNot{flp, enRefp->clone_বৃক্ষ_Pure(অসত্য)}, baseRefp}};
        }
        প্রত্যাগম নূতন AstCond{flp, enRefp, valRefp, baseRefp};
    }

    Ast_গ্রন্থি_Stmt* createForceRdUpdateStmt(ধ্রুব VarForceInfo& varInfo) ধ্রুব {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* ধ্রুব varp = varInfo.m_varVscp->varp();
        যদি (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            প্রত্যাগম createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_সারি* ধ্রুব flp = varInfo.m_varVscp->fileline();
        প্রত্যাগম নূতন AstAssign{flp, নূতন AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             createForceRdExpression(varInfo, flp)};
    }

    Ast_গ্রন্থি_Stmt* createForceRdUpdateStmtUnpacked(ধ্রুব VarForceInfo& varInfo) ধ্রুব {
        File_সারি* ধ্রুব flp = varInfo.m_varVscp->fileline();
        AstVar* ধ্রুব varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* ধ্রুব arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        ধ্রুব std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_গ্রন্থি_Stmt* ধ্রুব stmtsp = foreachUnpackedLeaf(
            dims, [&](ধ্রুব std::vector<পূর্ণাঙ্ক>& idx, পূর্ণাঙ্ক /*flat*/) -> Ast_গ্রন্থি_Stmt* {
                AstVarRef* ধ্রুব baseRefp = নূতন AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(baseRefp);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব baseSelp = buildNestedArraySel(flp, baseRefp, idx);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব enSelp = buildNestedArraySel(
                    flp, নূতন AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব valSelp = buildNestedArraySel(
                    flp, নূতন AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব readExprp = নূতন AstOr{
                    flp, নূতন AstAnd{flp, enSelp, valSelp},
                    নূতন AstAnd{flp, নূতন AstNot{flp, enSelp->clone_বৃক্ষ_Pure(অসত্য)}, baseSelp}};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব rdLhsSelp = buildNestedArraySel(
                    flp, নূতন AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                প্রত্যাগম নূতন AstAssign{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        প্রতি (ধ্রুব ForceInfo* ধ্রুব finfop : forceInfosInIdOrder(varInfo)) {
            যদি (!finfop->m_isExternal) অনুবর্ত;
            stmtsp->addNext(নূতন AstAssign{flp,
                                          নূতন AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_বৃক্ষ_Pure(অসত্য)});
        }
        প্রত্যাগম stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        ধ্রুব auto it = m_varToId.find(vscp);
        যদি (it != m_varToId.end()) প্রত্যাগম m_varInfos[it->second];

        m_varToId.emplace(vscp, m_varInfos.size());
        m_varInfos.emplace_back();
        VarForceInfo& info = m_varInfos.back();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* ধ্রুব varp = info.m_varp;
        যদি (!varp->isForceable()) প্রত্যাগম info;

        File_সারি* ধ্রুব flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        ধ্রুব তার্কিক helperVarsBuilt = helperVars.m_rdVarp != nullptr;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != nullptr)
                        && helperVarsBuilt == (helperVars.m_valVarp != nullptr),
                    varp, "Incomplete force helper set");
        যদি (!helperVarsBuilt) {
            ধ্রুব তার্কিক unpacked = isUnpackedArrayDType(varp->dtypep());
            ধ্রুব VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_গ্রন্থি_DType* ধ্রুব enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
            helperVars.m_rdVarp
                = নূতন AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sigPublic(সত্য);
            helperVars.m_enVarp
                = নূতন AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(সত্য);
            helperVars.m_valVarp
                = নূতন AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(সত্য);
            varp->addNextHere(helperVars.m_rdVarp);
            varp->addNextHere(helperVars.m_enVarp);
            varp->addNextHere(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = findScopeVar(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = findScopeVar(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = findScopeVar(info.m_scopep, helperVars.m_valVarp);
        যদি (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } অন্যথা {
            info.m_forceRdVscp = নূতন AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = নূতন AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = নূতন AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->addVarsp(info.m_forceRdVscp);
            info.m_scopep->addVarsp(info.m_forceEnVscp);
            info.m_scopep->addVarsp(info.m_forceValVscp);
        }
        প্রত্যাগম info;
    }

    শূন্য markClockedWrite(AstVar* varp) { m_clockedWrites.ঢোকাও(varp); }
    তার্কিক hasClockedWrite(AstVar* varp) ধ্রুব { প্রত্যাগম m_clockedWrites.গুনতি(varp); }

    তার্কিক doingAssign() ধ্রুব { প্রত্যাগম m_doingAssign; }

    ধ্রুব VarForceInfo* getVarInfo(AstVarScope* vscp) ধ্রুব {
        ধ্রুব auto it = m_varToId.find(vscp);
        প্রত্যাগম it != m_varToId.end() ? &m_varInfos[it->second] : nullptr;
    }

    AstVarScope* findScopeVar(AstScope* scopep, ধ্রুব AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        যদি (cache.empty()) {
            প্রতি (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        ধ্রুব auto it = cache.find(varp);
        প্রত্যাগম it != cache.end() ? it->second : nullptr;
    }
    শূন্য addForceAssignment(AstVar* varp, AstVarScope* vscp, Ast_গ্রন্থি_রাশিমালা* rhsExprp,
                            AstAssignForce* forceStmtp, পূর্ণাঙ্ক rangeLsb, পূর্ণাঙ্ক rangeMsb, পূর্ণাঙ্ক padLsb,
                            পূর্ণাঙ্ক padMsb, তার্কিক hasArraySel, তার্কিক skipRdRefresh = অসত্য) {
        v3Global.setUsesForce();
        varp->setForcedByCode();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        ধ্রুব পূর্ণাঙ্ক forceId = info.m_forces.size();
        File_সারি* ধ্রুব flp = varp->fileline();
        AstScope* ধ্রুব scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        যদি (!info.m_forceVecVscp) {
            AstCDType* ধ্রুব forceVecDtypep = নূতন AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->addTypesp(forceVecDtypep);

            AstVar* ধ্রুব forceVecVarp
                = নূতন AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doingAssign ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(অসত্য);
            forceVecVarp->isInternal(সত্য);
            varp->addNextHere(forceVecVarp);
            info.m_forceVecVscp = নূতন AstVarScope{flp, scopep, forceVecVarp};
            scopep->addVarsp(info.m_forceVecVscp);
        }

        auto pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, nullptr, rhsExprp});
        ForceInfo& finfo = pair.first->second;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        যদি (doingAssign()) {
            std::vector<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* ধ্রুব refp) {
                যদি (!refp->access().isReadOnly()) প্রত্যাগম;
                AstVar* ধ্রুব depVarp = refp->varp();
                যদি (depVarp
                    && std::find(depVarps.begin(), depVarps.end(), depVarp) == depVarps.end()) {
                    depVarps.ঠেলো_back(depVarp);
                }
            });
            প্রতি (AstVar* ধ্রুব depVarp : depVarps) m_rhsDepToForces[depVarp].ঠেলো_back(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    স্থির শূন্য collectArraySelInfo(Ast_গ্রন্থি_রাশিমালা* exprp, ArraySelInfo& info) {
        যদি (ধ্রুব auto* ধ্রুব selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = সত্য;
            collectArraySelInfo(selp->fromp(), info);
        } অন্যথা যদি (auto* ধ্রুব selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.ঠেলো_back(selp);
        } অন্যথা যদি (ধ্রুব auto* ধ্রুব memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    স্থির ArraySelInfo getArraySelInfo(Ast_গ্রন্থি_রাশিমালা* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        প্রত্যাগম info;
    }

    স্থির std::vector<পূর্ণাঙ্ক> arraySelDimSizes(ধ্রুব ArraySelInfo& info) {
        std::vector<পূর্ণাঙ্ক> dims;
        dims.reserve(info.m_sels.size());
        প্রতি (AstArraySel* ধ্রুব selp : info.m_sels) {
            Ast_গ্রন্থি_DType* ধ্রুব dtypep = selp->fromp()->dtypep()->skipRefp();
            ধ্রুব AstUnpackArrayDType* ধ্রুব arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.ঠেলো_back(arrayp->declRange().elements());
        }
        প্রত্যাগম dims;
    }

    স্থির পূর্ণাঙ্ক flattenIndex(ধ্রুব std::vector<পূর্ণাঙ্ক>& indices, ধ্রুব std::vector<পূর্ণাঙ্ক>& dimSizes) {
        UASSERT(indices.size() == dimSizes.size(), "Array index and dimension size mismatch");
        পূর্ণাঙ্ক index = 0;
        পূর্ণাঙ্ক stride = 1;
        প্রতি (পূর্ণাঙ্ক i = static_cast<পূর্ণাঙ্ক>(indices.size()) - 1; i >= 0; --i) {
            index += indices[i] * stride;
            stride *= dimSizes[i];
        }
        প্রত্যাগম index;
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* build_Flatten_Index_রাশিমালা(File_সারি* flp, ধ্রুব ArraySelInfo& info) {
        ধ্রুব std::vector<পূর্ণাঙ্ক> dimSizes = arraySelDimSizes(info);
        তার্কিক allConst = সত্য;
        প্রতি (AstArraySel* ধ্রুব selp : info.m_sels) {
            যদি (!VN_IS(selp->bitp(), Const)) {
                allConst = অসত্য;
                ভঙ্গ;
            }
        }
        যদি (allConst) {
            std::vector<পূর্ণাঙ্ক> constIndices;
            constIndices.reserve(info.m_sels.size());
            প্রতি (AstArraySel* ধ্রুব selp : info.m_sels) {
                constIndices.ঠেলো_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            প্রত্যাগম makeConst32(flp, flattenIndex(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_গ্রন্থি_রাশিমালা* resultp = nullptr;
        পূর্ণাঙ্ক stride = 1;
        প্রতি (পূর্ণাঙ্ক i = static_cast<পূর্ণাঙ্ক>(info.m_sels.size()) - 1; i >= 0; --i) {
            Ast_গ্রন্থি_রাশিমালা* termp = info.m_sels[i]->bitp()->clone_বৃক্ষ_Pure(অসত্য);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            যদি (termp->width() < 32) termp = নূতন AstExtend{flp, termp, 32};
            যদি (stride != 1) termp = নূতন AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? নূতন AstAdd{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        প্রত্যাগম resultp;
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* build_Rhs_Data_রাশিমালা(File_সারি* flp, ধ্রুব ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        প্রত্যাগম নূতন AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    শূন্য finalizeRhsVars() {
        প্রতি (VarForceInfo& info : m_varInfos) {
            AstVar* ধ্রুব varp = info.m_varp;
            যদি (info.m_forces.empty()) অনুবর্ত;

            AstScope* ধ্রুব scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_সারি* ধ্রুব flp = varp->fileline();
            ধ্রুব std::vector<ForceInfo*> forceps = forceInfosInIdOrder(info);

            প্রতি (ForceInfo* ধ্রুব finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* ধ্রুব rhsVarp = নূতন AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doingAssign() ? "_VassignRHS" : "__VforceRHS")
                        + std::to_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(সত্য);
                rhsVarp->sigPublic(সত্য);
                rhsVarp->setForcedByCode();
                varp->addNextHere(rhsVarp);
                finfo.m_rhsVarVscp = নূতন AstVarScope{flp, scopep, rhsVarp};
                scopep->addVarsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                // end
                AstAssign* ধ্রুব rhsAssignp = নূতন AstAssign{
                    flp, নূতন AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                যদি (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* ধ্রুব touchCallp = নূতন AstCMethodHard{
                        flp, নূতন AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtypeSetVoid();
                    Ast_গ্রন্থি_Stmt* ধ্রুব touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->addNextHere(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* ধ্রুব alwaysp
                    = নূতন AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, rhsAssignp};
                Ast_Sen_বৃক্ষ* ধ্রুব senTreep
                    = নূতন Ast_Sen_বৃক্ষ{flp, নূতন AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ধ্রুব activep = নূতন AstActive{flp, "force-rhs-update", senTreep};
                activep->sen_বৃক্ষ_Storep(activep->sentreep());
                activep->addStmtsp(alwaysp);
                scopep->addBlocksp(activep);
            }

            যদি (info.m_forceRdVscp) {
                যদি (!m_forceRdUpdateBuilt.ঢোকাও(info.m_varVscp).second) অনুবর্ত;
                AstActive* ধ্রুব activeInitp = নূতন AstActive{
                    flp, "force-init",
                    নূতন Ast_Sen_বৃক্ষ{flp, নূতন AstSenItem{flp, AstSenItem::Static{}}}};
                activeInitp->sen_বৃক্ষ_Storep(activeInitp->sentreep());
                Ast_গ্রন্থি_Stmt* initStmtp = nullptr;
                যদি (AstUnpackArrayDType* ধ্রুব arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ধ্রুব std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    ধ্রুব পূর্ণাঙ্ক innerWidth = dims.back()->subDTypep()->skipRefp()->width();
                    initStmtp = foreachUnpackedLeaf(
                        dims, [&](ধ্রুব std::vector<পূর্ণাঙ্ক>& idx, পূর্ণাঙ্ক /*flat*/) -> Ast_গ্রন্থি_Stmt* {
                            Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp = buildNestedArraySel(
                                flp, নূতন AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            প্রত্যাগম নূতন AstAssign{flp, lhsp, makeZeroConst(varp, innerWidth)};
                        });
                } অন্যথা {
                    initStmtp = নূতন AstAssign{
                        flp, নূতন AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->width())};
                }
                initStmtp->addNext(createForceRdUpdateStmt(info));
                activeInitp->addStmtsp(নূতন AstInitial{flp, initStmtp});
                scopep->addBlocksp(activeInitp);

                AstSenItem* itemsp = nullptr;
                auto addSenItem = [&](AstVarScope* vscp) {
                    যদি (!vscp) প্রত্যাগম;
                    AstSenItem* ধ্রুব nextp = নূতন AstSenItem{
                        flp, VEdgeType::ET_CHANGED, নূতন AstVarRef{flp, vscp, VAccess::READ}};
                    যদি (itemsp) {
                        itemsp->addNext(nextp);
                    } অন্যথা {
                        itemsp = nextp;
                    }
                };
                addSenItem(info.m_forceEnVscp);
                addSenItem(info.m_forceValVscp);
                AstVarRef* ধ্রুব origSenRefp = নূতন AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                AstSenItem* ধ্রুব origItemp
                    = নূতন AstSenItem{flp, VEdgeType::ET_CHANGED, origSenRefp};
                যদি (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->addNext(origItemp);
                প্রতি (ForceInfo* ধ্রুব finfop : forceps) {
                    যদি (!finfop->m_isExternal) addSenItem(finfop->m_rhsVarVscp);
                }

                AstActive* ধ্রুব activep
                    = নূতন AstActive{flp, "force-rd-update", নূতন Ast_Sen_বৃক্ষ{flp, itemsp}};
                activep->sen_বৃক্ষ_Storep(activep->sentreep());
                activep->addStmtsp(নূতন AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr,
                                                 createForceRdUpdateStmt(info)});
                scopep->addBlocksp(activep);
            }
        }
    }

    Ast_গ্রন্থি* createRhsUpdatesForWrite(File_সারি* flp, AstVar* writtenVarp) ধ্রুব {
        যদি (!doingAssign()) প্রত্যাগম nullptr;

        ধ্রুব auto it = m_rhsDepToForces.find(writtenVarp);
        যদি (it == m_rhsDepToForces.end()) প্রত্যাগম nullptr;

        Ast_গ্রন্থি* headp = nullptr;
        Ast_গ্রন্থি* tailp = nullptr;
        প্রতি (ধ্রুব ForceInfo* ধ্রুব finfop : it->second) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            AstAssign* ধ্রুব updatep
                = নূতন AstAssign{flp, নূতন AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_বৃক্ষ_Pure(অসত্য)};
            যদি (tailp) {
                tailp->addNextHere(updatep);
            } অন্যথা {
                headp = updatep;
            }
            tailp = updatep;
        }
        প্রত্যাগম headp;
    }

    ধ্রুব ForceInfo& getForceInfo(AstAssignForce* forceStmtp) ধ্রুব {
        AstVarScope* ধ্রুব vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        ধ্রুব VarForceInfo* ধ্রুব varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        auto it2 = varInfo->m_forces.find(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.end(), "Force statement not found");
        প্রত্যাগম it2->second;
    }

    স্থির তার্কিক selOverlapsAnyForce(ধ্রুব VarForceInfo& varInfo, পূর্ণাঙ্ক selLsb, পূর্ণাঙ্ক selMsb) {
        প্রতি (ধ্রুব auto& pair : varInfo.m_forces) {
            যদি (pair.second.m_rangeLsb <= selMsb && pair.second.m_rangeMsb >= selLsb) প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }

    Ast_গ্রন্থি_রাশিমালা* createForceReadExpression(ধ্রুব VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) ধ্রুব {
        File_সারি* ধ্রুব flp = originalRefp->fileline();
        AstVarRef* ধ্রুব refp = originalRefp->clone_বৃক্ষ_Pure(অসত্য);
        refp->access(VAccess::READ);
        প্রত্যাগম createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   nullptr);
    }

    Ast_গ্রন্থি_রাশিমালা* createForceReadIndexExpression(ধ্রুব VarForceInfo& varInfo,
                                                Ast_গ্রন্থি_রাশিমালা* originalExprp,
                                                Ast_গ্রন্থি_রাশিমালা* indexExprp) ধ্রুব {
        File_সারি* ধ্রুব flp = originalExprp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = originalExprp->clone_বৃক্ষ_Pure(অসত্য);
        // Must be an LValue to a static variable
        VN_AS(exprp->getVAccessTargetRecurse(), VarRef)->access(VAccess::READ);
        প্রত্যাগম createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_INDEX, exprp, originalExprp,
                                   indexExprp);
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* rebuildSelPath(Ast_গ্রন্থি_রাশিমালা* pathp, Ast_গ্রন্থি_রাশিমালা* baseExprp) {
        যদি (ধ্রুব AstSel* ধ্রুব selp = VN_CAST(pathp, Sel)) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব fromp = rebuildSelPath(selp->fromp(), baseExprp);
            AstSel* ধ্রুব outp
                = নূতন AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->widthConst()};
            outp->dtypeFrom(selp);
            প্রত্যাগম outp;
        }
        প্রত্যাগম baseExprp;
    }
};

// Split deassign concat LHS before converting to release internals.
স্থির শূন্য splitDeassign(AstDeassign* nodep) {
    AstConcat* ধ্রুব concatp = VN_CAST(nodep->lhsp(), Concat);
    যদি (!concatp) প্রত্যাগম;

    File_সারি* ধ্রুব flp = nodep->fileline();
    AstDeassign* ধ্রুব newLp = নূতন AstDeassign{flp, concatp->lhsp()->unlinkFrBack()};
    AstDeassign* ধ্রুব newRp = নূতন AstDeassign{flp, concatp->rhsp()->unlinkFrBack()};
    Ast_গ্রন্থি_রাশিমালা* ধ্রুব conp = concatp->unlinkFrBack();
    nodep->replaceWith(newLp);
    newLp->addNextHere(newRp);
    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    VL_DO_DANGLING(conp->delete_বৃক্ষ(), conp);

    splitDeassign(newLp);
    splitDeassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

বর্গ ForceDiscoveryVisitor final : সার্বজনিক VNVisitorConst {
    ForceState& m_state;
    তার্কিক m_inClockedActive = অসত্য;

    শূন্য buildForceableUnpackedArray(AstVarScope* ধ্রুব nodep,
                                     AstUnpackArrayDType* ধ্রুব arrDtypep) {
        AstVar* ধ্রুব varp = nodep->varp();
        ধ্রুব std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.empty(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব leafDtypep = dims.back()->subDTypep()->skipRefp();
        ধ্রুব AstBasicDType* ধ্রুব innerBasicp = leafDtypep->basicp();
        যদি (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            প্রত্যাগম;
        }

        File_সারি* ধ্রুব flp = varp->fileline();
        ধ্রুব পূর্ণাঙ্ক innerWidth = leafDtypep->width();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* ধ্রুব enVscp = info.m_forceEnVscp;
        AstVarScope* ধ্রুব valVscp = info.m_forceValVscp;

        AstSenItem* ধ্রুব itemsp = নূতন AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                  নূতন AstVarRef{flp, enVscp, VAccess::READ}};
        AstActive* ধ্রুব activep = নূতন AstActive{flp, "force-update", নূতন Ast_Sen_বৃক্ষ{flp, itemsp}};
        activep->sen_বৃক্ষ_Storep(activep->sentreep());

        Ast_গ্রন্থি_Stmt* ধ্রুব alwaysBodyHeadp = ForceState::foreachUnpackedLeaf(
            dims, [&](ধ্রুব std::vector<পূর্ণাঙ্ক>& idx, পূর্ণাঙ্ক flat) -> Ast_গ্রন্থি_Stmt* {
                AstVarRef* ধ্রুব origRefp = নূতন AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব enSelp = ForceState::buildNestedArraySel(
                    flp, নূতন AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব valSelp = ForceState::buildNestedArraySel(
                    flp, নূতন AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব forceExprp = নূতন AstOr{
                    flp, নূতন AstAnd{flp, enSelp, valSelp},
                    নূতন AstAnd{flp, নূতন AstNot{flp, enSelp->clone_বৃক্ষ_Pure(অসত্য)}, origSelp}};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsSelp = ForceState::buildNestedArraySel(
                    flp, নূতন AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                AstAssignForce* ধ্রুব forceAssignp = নূতন AstAssignForce{flp, lhsSelp, forceExprp};
                forceAssignp->user2(সত্য);

                Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsClonep = forceExprp->clone_বৃক্ষ_Pure(অসত্য);
                rhsClonep->foreach([varp](AstVarRef* ধ্রুব r) {
                    যদি (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/innerWidth - 1,
                                           /*hasArraySel=*/সত্য, /*skipRdRefresh=*/সত্য);
                প্রত্যাগম forceAssignp;
            });
        activep->addStmtsp(নূতন AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, alwaysBodyHeadp});
        nodep->scopep()->addBlocksp(activep);
    }

    শূন্য visit(AstAssignForce* nodep) override {
        যদি (nodep->user2()) প্রত্যাগম;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* ধ্রুব lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* ধ্রুব forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, সত্য);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsExprp = nodep->rhsp()->clone_বৃক্ষ_Pure(অসত্য);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    শূন্য visit(AstAssign* nodep) override {
        যদি (m_state.doingAssign() && m_inClockedActive) {
            যদি (AstVarRef* ধ্রুব lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstActive* nodep) override {
        VL_RESTORER(m_inClockedActive);
        m_inClockedActive = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstVarScope* nodep) override {
        যদি (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            যদি (m_state.doingAssign()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                প্রত্যাগম;
            }

            যদি (AstUnpackArrayDType* ধ্রুব arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                প্রত্যাগম;
            }

            ধ্রুব AstBasicDType* ধ্রুব bdtypep = nodep->varp()->basicp();
            যদি (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* ধ্রুব varp = nodep->varp();
            File_সারি* ধ্রুব flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* ধ্রুব enVscp = info.m_forceEnVscp;
            AstVarScope* ধ্রুব valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            AstSenItem* ধ্রুব itemsp = নূতন AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                      নূতন AstVarRef{flp, enVscp, VAccess::READ}};
            AstActive* ধ্রুব activep
                = নূতন AstActive{flp, "force-update", নূতন Ast_Sen_বৃক্ষ{flp, itemsp}};
            activep->sen_বৃক্ষ_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* ধ্রুব refp = নূতন AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* ধ্রুব enRefp = নূতন AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* ধ্রুব valRefp = নূতন AstVarRef{flp, valVscp, VAccess::READ};
            ধ্রুব AstBasicDType* ধ্রুব basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstOr{
                            flp, নূতন AstAnd{flp, enRefp, valRefp},
                            নূতন AstAnd{flp, নূতন AstNot{flp, enRefp->clone_বৃক্ষ_Pure(অসত্য)}, refp}})
                      : static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstCond{flp, enRefp, valRefp, refp});
            AstAssignForce* ধ্রুব forceAssignp
                = নূতন AstAssignForce{flp, নূতন AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(সত্য);
            activep->addStmtsp(নূতন AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, forceAssignp});
            nodep->scopep()->addBlocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsClonep = forceExprp->clone_বৃক্ষ_Pure(অসত্য);
            rhsClonep->foreach([varp](AstVarRef* ধ্রুব refp) {
                যদি (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            ধ্রুব তার্কিক bitwiseVar = ForceState::isBitwiseDType(varp);
            ধ্রুব পূর্ণাঙ্ক padMsb = bitwiseVar ? (varp->width() - 1) : 0;
            পূর্ণাঙ্ক rangeLsb = 0;
            পূর্ণাঙ্ক rangeMsb = padMsb;
            যদি (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, অসত্য);
        }
        iterateChildrenConst(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সার্বজনিক:
    explicit ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextConstNull(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

বর্গ ForceConvertVisitor final : সার্বজনিক VNVisitor {
    ForceState& m_state;

    শূন্য visit(AstAssignForce* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp = nodep->lhsp();
        AstVarRef* ধ্রুব lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ধ্রুব forcedVarp = lhsVarRefp->varp();

        ধ্রুব ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        ধ্রুব ForceState::VarForceInfo* ধ্রুব varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_সারি* ধ্রুব flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        AstAssign* ধ্রুব rhsAssignp
            = নূতন AstAssign{flp, নূতন AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_বৃক্ষ_Pure(অসত্য)};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        ধ্রুব তার্কিক bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        যদি (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_গ্রন্থি_রাশিমালা* baseExprp = nodep->rhsp()->clone_বৃক্ষ_Pure(অসত্য);
            baseExprp->foreach(
                [](AstVarRef* ধ্রুব refp) { ForceState::markNonReplaceable(refp); });
            যদি (bitwiseForcedVar) {
                baseExprp = ForceState::zeroPadToBaseWidth(baseExprp, forcedVarp->width(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            যদি (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* ধ্রুব maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব valReadp
                    = নূতন AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব valWritep
                    = নূতন AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব notMaskp = নূতন AstNot{flp, maskConstp};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব maskedOldp = নূতন AstAnd{flp, valReadp, notMaskp};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব newValp = নূতন AstOr{flp, maskedOldp, baseExprp};
                valAssignp = নূতন AstAssign{flp, valWritep, newValp};

                Ast_গ্রন্থি_রাশিমালা* ধ্রুব enReadp
                    = নূতন AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব enWritep
                    = নূতন AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব newEnp
                    = নূতন AstOr{flp, enReadp, maskConstp->clone_বৃক্ষ_Pure(অসত্য)};
                enAssignp = নূতন AstAssign{flp, enWritep, newEnp};
            } অন্যথা {
                AstConst* ধ্রুব oneConstp = ForceState::makeRangeMaskConst(nodep, 1, 0, 0);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsValp = ForceState::cast_To_গ্রন্থি_DType(baseExprp, forcedVarp);
                valAssignp = নূতন AstAssign{
                    flp, নূতন AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = নূতন AstAssign{
                    flp, নূতন AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        ধ্রুব AstSel* ধ্রুব selLhsp = VN_CAST(lhsp, Sel);
        ধ্রুব তার্কিক arrayBitSel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->width();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsDatap = ForceState::build_Rhs_Data_রাশিমালা(flp, info);
        AstCExpr* ধ্রুব rhsAddrp = নূতন AstCExpr{flp};
        rhsAddrp->add("&(");
        rhsAddrp->add(rhsDatap);
        rhsAddrp->add(")");
        AstCMethodHard* ধ্রুব addForceCallp = নূতন AstCMethodHard{
            flp, নূতন AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ADD,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        addForceCallp->addPinsp(rhsAddrp);
        addForceCallp->addPinsp(
            ForceState::makeConst32(flp, arrayBitSel ? info.m_padLsb : info.m_rangeLsb));
        যদি (arrayBitSel) {
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padLsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padMsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, selLhsp->fromp()->width()));
        }
        addForceCallp->dtypeSetVoid();
        Ast_গ্রন্থি_Stmt* ধ্রুব stmtp = addForceCallp->makeStmt();

        Ast_গ্রন্থি* tailp = rhsAssignp;
        যদি (valAssignp) {
            tailp->addNextHere(valAssignp);
            tailp = valAssignp;
        }
        যদি (enAssignp) {
            tailp->addNextHere(enAssignp);
            tailp = enAssignp;
        }
        tailp->addNextHere(stmtp);
        যদি (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp = nodep->lhsp();
        AstVarRef* ধ্রুব lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ধ্রুব releasedVarp = lhsVarRefp->varp();

        ধ্রুব ForceState::VarForceInfo* ধ্রুব varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        যদি (!varInfo) {
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            প্রত্যাগম;
        }
        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        File_সারি* ধ্রুব flp = nodep->fileline();

        ধ্রুব ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, অসত্য);

        ধ্রুব AstSel* ধ্রুব selLhsp = VN_CAST(lhsp, Sel);
        ধ্রুব তার্কিক arrayBitSel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->width();
        AstCMethodHard* ধ্রুব releaseCallp = নূতন AstCMethodHard{
            flp, নূতন AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        যদি (arrayBitSel) {
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtypeSetVoid();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_গ্রন্থি_Stmt* ধ্রুব releasep = releaseCallp->makeStmt();

        AstAssign* clearEnp = nullptr;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        যদি (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব enWritep
                = নূতন AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            যদি (ForceState::isBitwiseDType(releasedVarp)) {
                ধ্রুব পূর্ণাঙ্ক varWidth = releasedVarp->width();
                যদি (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == varWidth - 1) {
                    clearEnp
                        = নূতন AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, varWidth)};
                } অন্যথা {
                    // forceEn = forceEn & ~mask(range);
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব enReadp
                        = নূতন AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* ধ্রুব maskConst = ForceState::makeRangeMaskConst(
                        nodep, varWidth, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব newEnp
                        = নূতন AstAnd{flp, enReadp, নূতন AstNot{flp, maskConst}};
                    clearEnp = নূতন AstAssign{flp, enWritep, newEnp};
                }
            } অন্যথা {
                clearEnp = নূতন AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        ধ্রুব AstSel* ধ্রুব selp = VN_CAST(lhsp, Sel);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব basep = selp ? selp->fromp() : lhsp;

        Ast_গ্রন্থি* stmtListp = releasep;
        যদি (clearEnp) {
            clearEnp->addNextHere(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        ধ্রুব তার্কিক fullBitwiseRelease
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->width() - 1;
        যদি (!releasedVarp->isContinuously()
            && !(m_state.doingAssign() && m_state.hasClockedWrite(releasedVarp)
                 && fullBitwiseRelease)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            ধ্রুব তার্কিক hasOpaquePath = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_গ্রন্থি_রাশিমালা* forceReadp = nullptr;
            যদি (hasOpaquePath) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } অন্যথা যদি (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, basep,
                    ForceState::build_Flatten_Index_রাশিমালা(flp, rangeInfo.m_arrayInfo));
                যদি (selp) { forceReadp = ForceState::rebuildSelPath(lhsp, forceReadp); }
            } অন্যথা {
                forceReadp
                    = selp ? ForceState::rebuildSelPath(
                                 lhsp, m_state.createForceReadExpression(*varInfo, lhsVarRefp))
                           : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            }
            AstAssign* ধ্রুব assignp = নূতন AstAssign{flp, lhsp->clone_বৃক্ষ_Pure(অসত্য), forceReadp};
            assignp->addNextHere(stmtListp);
            stmtListp = assignp;
        }

        যদি (varInfo->m_forceRdVscp) stmtListp->addNext(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

বর্গ ForceReplaceVisitor final : সার্বজনিক VNVisitor {
    ধ্রুব ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_গ্রন্থি_Stmt* m_stmtp = nullptr;
    তার্কিক m_inLogic = অসত্য;

    শূন্য iterateLogic(Ast_গ্রন্থি* nodep) {
        VL_RESTORER(m_inLogic);
        m_inLogic = সত্য;
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstAssign* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        যদি (AstVarRef* ধ্রুব lhsp = VN_CAST(nodep->lhsp()->baseFromp(সত্য), VarRef)) {
            যদি (Ast_গ্রন্থি* ধ্রুব updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->addNextHere(updatep);
            }
        }
    }
    শূন্য visit(AstAssignCont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateAndNextNull(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    শূন্য visit(AstDeassign*) override {}
    শূন্য visit(AstCFunc* nodep) override { iterateLogic(nodep); }
    শূন্য visit(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override { iterateLogic(nodep); }
    শূন্য visit(AstAlways* nodep) override {
        যদি (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            প্রত্যাগম;
        }
        iterateLogic(nodep);
    }
    শূন্য visit(AstSenItem* nodep) override { iterateLogic(nodep); }
    শূন্য visit(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* ধ্রুব refp = VN_CAST(nodep->fromp(), VarRef);
        যদি (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            visit(static_cast<Ast_গ্রন্থি*>(nodep));
            প্রত্যাগম;
        }

        AstVar* ধ্রুব varp = refp->varp();
        ধ্রুব ForceState::VarForceInfo* ধ্রুব varInfo = m_state.getVarInfo(refp->varScopep());
        যদি (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.empty()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            visit(static_cast<Ast_গ্রন্থি*>(nodep));
            প্রত্যাগম;
        }

        যদি (ধ্রুব AstConst* ধ্রুব lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            ধ্রুব পূর্ণাঙ্ক selLsb = lsbConstp->toSInt();
            ধ্রুব পূর্ণাঙ্ক selMsb = selLsb + nodep->width() - 1;
            যদি (!varp->isSigPublic()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                visit(static_cast<Ast_গ্রন্থি*>(nodep));
                প্রত্যাগম;
            }
        }

        File_সারি* ধ্রুব flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* ধ্রুব refClonep = refp->clone_বৃক্ষ_Pure(অসত্য);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* ধ্রুব callp = নূতন AstCMethodHard{
            flp, নূতন AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->width())};
        callp->addPinsp(refClonep);
        callp->addPinsp(nodep->lsbp()->clone_বৃক্ষ_Pure(অসত্য));
        callp->addPinsp(ForceState::makeConst32(flp, nodep->width()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstArraySel* nodep) override {
        যদি (ধ্রুব AstArraySel* ধ্রুব backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            যদি (backSelp->fromp() == nodep) {
                iterateChildren(nodep);
                প্রত্যাগম;
            }
        }

        Ast_গ্রন্থি* ধ্রুব basep = nodep->baseFromp(সত্য);
        AstVarRef* ধ্রুব baseRefp = VN_CAST(basep, VarRef);
        যদি (!baseRefp) {
            iterateChildren(nodep);
            প্রত্যাগম;
        }
        AstVar* ধ্রুব varp = baseRefp->varp();
        ধ্রুব ForceState::VarForceInfo* ধ্রুব varInfo = m_state.getVarInfo(baseRefp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        যদি (m_state.isNotReplaceable(baseRefp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            প্রত্যাগম;
        }

        যদি (!baseRefp->access().isReadOnly()) {
            iterateChildren(nodep);
            প্রত্যাগম;
        }
        যদি (m_state.doingAssign() && varInfo->m_forceRdVscp) {
            baseRefp->varp(varInfo->m_forceRdVscp->varp());
            baseRefp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            প্রত্যাগম;
        }
        ধ্রুব ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        প্রতি (AstArraySel* ধ্রুব selp : arrayInfo.m_sels) iterateAndNextNull(selp->bitp());
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব indexExprp
            = ForceState::build_Flatten_Index_রাশিমালা(nodep->fileline(), arrayInfo);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব readExprp
            = m_state.createForceReadIndexExpression(*varInfo, nodep, indexExprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(AstVarRef* nodep) override {
        যদি (m_state.isNotReplaceable(nodep)) প্রত্যাগম;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        যদি (ধ্রুব AstArraySel* ধ্রুব backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            যদি (backSelp->fromp() == nodep) প্রত্যাগম;
        }

        ধ্রুব ForceState::VarForceInfo* ধ্রুব varInfo = m_state.getVarInfo(nodep->varScopep());
        যদি (!varInfo) প্রত্যাগম;

        যদি (varInfo->m_forceRdVscp) {
            যদি (nodep->access().isRW()) {
                যদি (m_inLogic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                প্রত্যাগম;
            }
            যদি (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব readp
                    = m_state.createForceRdExpression(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                প্রত্যাগম;
            }
            যদি (m_inLogic && m_stmtp) {
                m_stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
            }
            প্রত্যাগম;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        যদি (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            প্রত্যাগম;
        }

        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        যদি (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } অন্যথা যদি (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (ForceState::isOutermostOpaquePathSelector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = VN_AS(nodep, গ্রন্থি_রাশিমালা);
            Ast_গ্রন্থি* ধ্রুব basep = exprp->baseFromp(সত্য);
            AstVarRef* ধ্রুব baseRefp = VN_CAST(basep, VarRef);
            যদি (baseRefp) {
                AstVar* ধ্রুব varp = baseRefp->varp();
                যদি (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    ধ্রুব ForceState::VarForceInfo* ধ্রুব varInfo
                        = m_state.getVarInfo(baseRefp->varScopep());
                    যদি (!m_state.isNotReplaceable(baseRefp) && varInfo) {
                        ধ্রুব পূর্ণাঙ্ক forcePathIndex = varInfo->findForcePathIndex(exprp);
                        যদি (forcePathIndex >= 0) {
                            যদি (!baseRefp->access().isReadOnly()) প্রত্যাগম;
                            Ast_গ্রন্থি_রাশিমালা* ধ্রুব readExprp = m_state.createForceReadIndexExpression(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), forcePathIndex));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                            প্রত্যাগম;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

সার্বজনিক:
    explicit ForceReplaceVisitor(AstNetlist* nodep, ধ্রুব ForceState& state)
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

স্থির শূন্য forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::ProtectedVarRefSet& permanentlyProtected,
                         ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    যদি (!v3Global.hasForceableSignals()) প্রত্যাগম;
    ForceState state{অসত্য, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_বৃক্ষ("force", 0, dump_বৃক্ষ_Either_Level() >= 3);
}

স্থির শূন্য assignAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::ProtectedVarRefSet& permanentlyProtected,
                          ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    যদি (!v3Global.hasAssignDeassign()) প্রত্যাগম;

    std::vector<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.ঠেলো_back(deassignp); });
    প্রতি (AstDeassign* ধ্রুব deassignp : deassignps) splitDeassign(deassignp);

    std::vector<AstAssignCont*> assignContps;
    deassignps.clear();
    nodep->foreach([&](Ast_গ্রন্থি_Stmt* nodep) {
        যদি (AstAssignCont* ধ্রুব assignContp = VN_CAST(nodep, AssignCont)) {
            assignContps.ঠেলো_back(assignContp);
        } অন্যথা যদি (AstDeassign* ধ্রুব deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.ঠেলো_back(deassignp);
        }
    });

    প্রতি (AstAssignCont* ধ্রুব assignp : assignContps) {
        assignp->replaceWith(নূতন AstAssignForce{assignp->fileline(),
                                                assignp->lhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
        VL_DO_DANGLING(assignp->delete_বৃক্ষ(), assignp);
    }
    প্রতি (AstDeassign* ধ্রুব deassignp : deassignps) {
        deassignp->replaceWith(
            নূতন AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_বৃক্ষ_Pure(সত্য)});
        VL_DO_DANGLING(deassignp->delete_বৃক্ষ(), deassignp);
    }
    ForceState state{সত্য, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_বৃক্ষ("assign-deassign", 0, dump_বৃক্ষ_Either_Level() >= 3);
}

শূন্য V3Force::forceAndAssignAll(AstNetlist* nodep) {
    ধ্রুব VNUser3InUse user3InUse;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::ProtectedVarRefSet permanentlyProtected;
    ForceState::BuiltVarScopeSet forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    assignAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
