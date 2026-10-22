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

শ্রেণী ForceState final {
সর্বজনীন:
    কাঠামো ForceRange VL_NOT_FINAL {
        পূর্ণসংখ্যা m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        পূর্ণসংখ্যা m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        পূর্ণসংখ্যা m_padLsb = 0;  // Bit positions for RHS padding
        পূর্ণসংখ্যা m_padMsb = 0;  // Bit positions for RHS padding
    };

    কাঠামো ForceInfo final : ForceRange {
        // MEMBERS
        পূর্ণসংখ্যা m_forceId = 0;  // Unique (per signal) variable of this force assignment
        যৌক্তিক m_hasArraySel = মিথ্যা;  // If this has an array select on LHS
        যৌক্তিক m_isExternal = মিথ্যা;  // Synthetic force for the public force controls
        যৌক্তিক m_skipRdRefresh = মিথ্যা;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = nullptr;  // Scope of the var containing RHSID
        Ast_গ্রন্থি_রাশিমালা* m_rhsExprp = nullptr;  // Expression on RHS of this force assignment

        ForceInfo() = পূর্বনির্ধারিত;
        ForceInfo(পূর্ণসংখ্যা rangeLsb, পূর্ণসংখ্যা rangeMsb, পূর্ণসংখ্যা padLsb, পূর্ণসংখ্যা padMsb, পূর্ণসংখ্যা forceId,
                  যৌক্তিক hasArraySel, AstVarScope* rhsVarVscp, Ast_গ্রন্থি_রাশিমালা* rhsExprp)
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

    কাঠামো VarForceInfo final {
        AstVarScope* m_forceVecVscp = nullptr;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = nullptr;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = nullptr;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = nullptr;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = nullptr;  // Scope of the forced (original) variable
        AstVar* m_varp = nullptr;  // The forced (original) variable
        AstScope* m_scopep = nullptr;  // Scope containing the forced variable
        std::unordered_map<AstAssignForce*, ForceInfo> m_forces;
        std::unordered_map<string, পূর্ণসংখ্যা> m_force_Path_To_সূচক;
        পূর্ণসংখ্যা m_পরবর্তী_Force_Path_সূচক = 1;  // Start at 1 so 0 can be the base path (whole signal)

    ব্যক্তিগত:
        স্থির শূন্য build_Force_Path_চাবি_Recurse(Ast_গ্রন্থি_রাশিমালা* nodep, string& out) {
            যদি (VN_IS(nodep, VarRef)) ফেরত;
            যদি (ধ্রুবক AstSel* ধ্রুবক selp = VN_CAST(nodep, Sel)) {
                build_Force_Path_চাবি_Recurse(selp->fromp(), out);
                out += "|S" + cvtToStr(selp->lsbConst()) + ":" + cvtToStr(selp->widthConst());
                ফেরত;
            }
            যদি (AstStructSel* ধ্রুবক selp = VN_CAST(nodep, StructSel)) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = selp->fromp();
                build_Force_Path_চাবি_Recurse(fromp, out);
                out += "|T" + selp->name();
                ফেরত;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        স্থির string force_Path_চাবি(Ast_গ্রন্থি_রাশিমালা* nodep) {
            string out;
            build_Force_Path_চাবি_Recurse(nodep, out);
            ফেরত out;
        }

    সর্বজনীন:
        পূর্ণসংখ্যা get_Or_Create_Force_Path_সূচক(Ast_গ্রন্থি_রাশিমালা* nodep) {
            ধ্রুবক auto pair
                = m_force_Path_To_সূচক.emplace(force_Path_চাবি(nodep), m_পরবর্তী_Force_Path_সূচক);
            যদি (pair.second) ++m_পরবর্তী_Force_Path_সূচক;
            ফেরত pair.first->second;
        }

        পূর্ণসংখ্যা find_Force_Path_সূচক(Ast_গ্রন্থি_রাশিমালা* nodep) ধ্রুবক {
            ধ্রুবক auto it = m_force_Path_To_সূচক.find(force_Path_চাবি(nodep));
            ফেরত it != m_force_Path_To_সূচক.end() ? it->second : -1;
        }
    };

    কাঠামো ForceHelperVars final {
        AstVar* m_rdVarp = nullptr;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = nullptr;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = nullptr;  // The __VforceVal (force value) variable
    };

    কাঠামো ArraySelInfo final {
        std::vector<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        যৌক্তিক m_hasBitSel = মিথ্যা;  // Bit-select was found along the path too
    };

    কাঠামো ForceRangeInfo final : ForceRange {
        যৌক্তিক m_hasArraySel = মিথ্যা;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

ব্যক্তিগত:
    using ScopeVarCache = std::unordered_map<ধ্রুবক AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    ধ্রুবক VNUser1InUse m_user1InUse;
    ধ্রুবক VNUser2InUse m_user2InUse;

সর্বজনীন:
    using ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    using ProtectedVarRefSet = std::unordered_set<ধ্রুবক AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    using BuiltVarScopeSet = std::unordered_set<ধ্রুবক AstVarScope*>;

ব্যক্তিগত:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::vector<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, পূর্ণসংখ্যা> m_varToId;
    std::unordered_set<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::vector<ForceInfo*>> m_rhsDepToForces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    যৌক্তিক m_doingAssign = মিথ্যা;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    ProtectedVarRefSet& m_permanentlyProtected;
    BuiltVarScopeSet& m_forceRdUpdateBuilt;

সর্বজনীন:
    ForceState(যৌক্তিক doingAssign, ForceHelperVarsByVar& forceHelperVarsByVar,
               ProtectedVarRefSet& permanentlyProtected, BuiltVarScopeSet& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doingAssign{doingAssign}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    স্থির AstConst* makeZeroConst(Ast_গ্রন্থি* nodep, পূর্ণসংখ্যা width) {
        V3_সংখ্যা zero{nodep, width};
        zero.setAllBits0();
        ফেরত নতুন AstConst{nodep->fileline(), zero};
    }

    স্থির AstConst* makeConst32(File_সারি* flp, পূর্ণসংখ্যা মান) {
        ফেরত নতুন AstConst{flp, AstConst::Widthed_মান{}, 32, static_cast<uint32_t>(মান)};
    }

    স্থির AstConst* makeRangeMaskConst(Ast_গ্রন্থি* nodep, পূর্ণসংখ্যা width, পূর্ণসংখ্যা lsb, পূর্ণসংখ্যা msb) {
        V3_সংখ্যা mask{nodep, width};
        mask.setAllBits0();
        জন্য (পূর্ণসংখ্যা bit = lsb; bit <= msb; ++bit) mask.setBit(bit, 1);
        ফেরত নতুন AstConst{nodep->fileline(), mask};
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* zeroPadToBaseWidth(Ast_গ্রন্থি_রাশিমালা* exprp, পূর্ণসংখ্যা baseWidth, পূর্ণসংখ্যা padLsb,
                                           পূর্ণসংখ্যা padMsb) {
        যদি (baseWidth <= 0) ফেরত exprp;
        ধ্রুবক পূর্ণসংখ্যা নিচের_Pad = padLsb;
        ধ্রুবক পূর্ণসংখ্যা উপরের_Pad = baseWidth - (padMsb + 1);
        যদি (নিচের_Pad > 0) {
            exprp = নতুন AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, নিচের_Pad)};
        }
        যদি (উপরের_Pad > 0) {
            exprp = নতুন AstConcat{exprp->fileline(), makeZeroConst(exprp, উপরের_Pad), exprp};
        }
        ফেরত exprp;
    }

    স্থির যৌক্তিক isUnpackedArrayDType(ধ্রুবক Ast_গ্রন্থি_DType* dtypep) {
        ফেরত VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    স্থির যৌক্তিক isBitwiseDType(ধ্রুবক Ast_গ্রন্থি* nodep) {
        ধ্রুবক AstBasicDType* ধ্রুবক basicp = nodep->dtypep()->skipRefp()->basicp();
        ফেরত basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* cast_To_গ্রন্থি_DType(Ast_গ্রন্থি_রাশিমালা* exprp, Ast_গ্রন্থি* dtypeFromp) {
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = dtypeFromp->dtypep()->skipRefp();
        ধ্রুবক AstBasicDType* ধ্রুবক basicp = dtypep->basicp();
        যদি (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            ফেরত exprp;
        }
        ফেরত নতুন AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    স্থির শূন্য markNonReplaceable(AstVarRef* ধ্রুবক nodep) { nodep->user1SetOnce(); }
    যৌক্তিক isNotReplaceable(ধ্রুবক AstVarRef* ধ্রুবক nodep) ধ্রুবক {
        ফেরত nodep->user1() || m_permanentlyProtected.গুনতি(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    শূন্য markPermanentlyProtected(AstVarRef* ধ্রুবক nodep) ধ্রুবক {
        markNonReplaceable(nodep);
        m_permanentlyProtected.ঢোকাও(nodep);
    }

    স্থির std::vector<ForceInfo*> forceInfosInIdOrder(VarForceInfo& info) {
        std::vector<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.আকার_2());
        জন্য (auto& it : info.m_forces) forceps.ঠেলো_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](ধ্রুবক ForceInfo* ap, ধ্রুবক ForceInfo* bp) {
            ফেরত ap->m_forceId < bp->m_forceId;
        });
        ফেরত forceps;
    }

    স্থির std::vector<ধ্রুবক ForceInfo*> forceInfosInIdOrder(ধ্রুবক VarForceInfo& info) {
        std::vector<ধ্রুবক ForceInfo*> forceps;
        forceps.reserve(info.m_forces.আকার_2());
        জন্য (ধ্রুবক auto& it : info.m_forces) forceps.ঠেলো_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](ধ্রুবক ForceInfo* ap, ধ্রুবক ForceInfo* bp) {
            ফেরত ap->m_forceId < bp->m_forceId;
        });
        ফেরত forceps;
    }

    স্থির যৌক্তিক isOpaquePathSelector(ধ্রুবক Ast_গ্রন্থি* nodep) {
        ফেরত VN_IS(nodep, Sel) || VN_IS(nodep, গ্রন্থি_Sel) || VN_IS(nodep, StructSel);
    }

    স্থির যৌক্তিক isOutermostOpaquePathSelector(ধ্রুবক Ast_গ্রন্থি* nodep) {
        যদি (!isOpaquePathSelector(nodep)) ফেরত মিথ্যা;
        ধ্রুবক Ast_গ্রন্থি* ধ্রুবক backp = nodep->backp();
        ফেরত !backp || !isOpaquePathSelector(backp);
    }

    স্থির AstVarRef* getOneVarRef(Ast_গ্রন্থি_রাশিমালা* forceStmtp) {
        Ast_গ্রন্থি* ধ্রুবক basep = forceStmtp->baseFromp(সত্য);
        যদি (AstSampled* sampledp = VN_CAST(basep, Sampled))
            যদি (Ast_গ্রন্থি_রাশিমালা* exprp = VN_CAST(sampledp->exprp(), গ্রন্থি_রাশিমালা))
                ফেরত getOneVarRef(exprp);
        AstVarRef* ধ্রুবক varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        ফেরত varRefp;
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* buildNestedArraySel(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* fromp,
                                            ধ্রুবক std::vector<পূর্ণসংখ্যা>& indicies) {
        Ast_গ্রন্থি_রাশিমালা* curp = fromp;
        জন্য (ধ্রুবক পূর্ণসংখ্যা idx : indicies) curp = নতুন AstArraySel{flp, curp, idx};
        ফেরত curp;
    }

    template <typename Fn>
    স্থির Ast_গ্রন্থি_Stmt* foreachUnpackedLeaf(ধ্রুবক std::vector<AstUnpackArrayDType*>& dims,
                                            Fn buildLeaf) {
        Ast_গ্রন্থি_Stmt* headp = nullptr;
        Ast_গ্রন্থি_Stmt* tailp = nullptr;
        যদি (dims.empty()) ফেরত nullptr;
        পূর্ণসংখ্যা যোগ = 1;
        জন্য (ধ্রুবক AstUnpackArrayDType* ধ্রুবক d : dims) যোগ *= d->elementsConst();
        যদি (যোগ <= 0) ফেরত nullptr;
        std::vector<পূর্ণসংখ্যা> idx(dims.আকার_2(), 0);
        জন্য (পূর্ণসংখ্যা flat = 0; flat < যোগ; ++flat) {
            Ast_গ্রন্থি_Stmt* ধ্রুবক stmtp = buildLeaf(idx, flat);
            যদি (!headp) {
                headp = stmtp;
            } অন্যথা {
                tailp->add_পরবর্তী(stmtp);
            }
            tailp = stmtp;
            জন্য (পূর্ণসংখ্যা d = static_cast<পূর্ণসংখ্যা>(dims.আকার_2()) - 1; d >= 0; --d) {
                যদি (++idx[d] < dims[d]->elementsConst()) বিরতি;
                idx[d] = 0;
            }
        }
        ফেরত headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_গ্রন্থি_রাশিমালা* lhsp, AstVar* varp,
                                     যৌক্তিক requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.empty();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->width() - 1) : 0;

        যদি (ধ্রুবক AstSel* ধ্রুবক outerSelp = VN_CAST(lhsp, Sel)) {
            পূর্ণসংখ্যা যোগ_Lsb = 0;
            জন্য (Ast_গ্রন্থি_রাশিমালা* curp = lhsp; ধ্রুবক AstSel* ধ্রুবক selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                যদি (requireConstRangeSelect) {
                    UASSERT_OBJ(VN_IS(selp->lsbp(), Const), lhsp,
                                "Unsupported: force on non-const range select");
                }
                যোগ_Lsb += selp->lsbConst();
            }
            info.m_padLsb = যোগ_Lsb;
            info.m_padMsb = যোগ_Lsb + outerSelp->widthConst() - 1;
        }

        info.m_rangeLsb = info.m_padLsb;
        info.m_rangeMsb = info.m_padMsb;
        যদি (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::vector<পূর্ণসংখ্যা> indices;
            indices.reserve(info.m_arrayInfo.m_sels.আকার_2());
            জন্য (AstArraySel* ধ্রুবক selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.ঠেলো_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            info.m_rangeLsb = flatten_সূচক(indices, arraySelDimSizes(info.m_arrayInfo));

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
            ধ্রুবক পূর্ণসংখ্যা সূচক = varInfo.get_Or_Create_Force_Path_সূচক(lhsp);
            info.m_rangeLsb = সূচক;
            info.m_rangeMsb = সূচক;
        }
        ফেরত info;
    }

    Ast_গ্রন্থি_রাশিমালা* add_Rhs_মান_Reads(ধ্রুবক VarForceInfo& varInfo, Ast_গ্রন্থি_রাশিমালা* exprp) ধ্রুবক {
        যদি (!doingAssign()) ফেরত exprp;

        ধ্রুবক std::vector<ধ্রুবক ForceInfo*> forceps = forceInfosInIdOrder(varInfo);
        যদি (forceps.empty()) ফেরত exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* ধ্রুবক cexprp = নতুন AstCExpr{exprp->fileline(), AstCExpr::Pure{}};
        cexprp->dtypeFrom(exprp);
        cexprp->add("(");
        জন্য (ধ্রুবক ForceInfo* ধ্রুবক finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* ধ্রুবক refp
                = নতুন AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->add("(void)(");
            cexprp->add(refp);
            cexprp->add("), ");
        }
        cexprp->add(exprp);
        cexprp->add(")");
        ফেরত cexprp;
    }

    Ast_গ্রন্থি_রাশিমালা* createForceReadCall(ধ্রুবক VarForceInfo& varInfo, File_সারি* flp, VCMethod method,
                                     Ast_গ্রন্থি_রাশিমালা* originalExprp, Ast_গ্রন্থি* dtypeFromp,
                                     Ast_গ্রন্থি_রাশিমালা* সূচক_Exprp) ধ্রুবক {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক origValp
            = add_Rhs_মান_Reads(varInfo, cast_To_গ্রন্থি_DType(originalExprp, dtypeFromp));

        AstCMethodHard* ধ্রুবক callp = নতুন AstCMethodHard{
            flp, নতুন AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, method, origValp};
        যদি (সূচক_Exprp) callp->addPinsp(সূচক_Exprp);
        callp->dtypeFrom(dtypeFromp);
        ফেরত callp;
    }

    Ast_গ্রন্থি_রাশিমালা* createForceRdExpression(ধ্রুবক VarForceInfo& varInfo, File_সারি* flp) ধ্রুবক {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* ধ্রুবক varp = varInfo.m_varVscp->varp();
        যদি (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            যদি (doingAssign()) { ফেরত নতুন AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, সত্য);
            AstVarRef* ধ্রুবক baseRefp = নতুন AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(baseRefp);
            ফেরত createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, baseRefp, varp,
                                       nullptr);
        }
        AstVarRef* ধ্রুবক baseRefp = নতুন AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(baseRefp);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক enRefp = নতুন AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valRefp = নতুন AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        যদি (isBitwiseDType(varp)) {
            ফেরত নতুন AstOr{
                flp, নতুন AstAnd{flp, enRefp, valRefp},
                নতুন AstAnd{flp, নতুন AstNot{flp, enRefp->clone_বৃক্ষ_Pure(মিথ্যা)}, baseRefp}};
        }
        ফেরত নতুন AstCond{flp, enRefp, valRefp, baseRefp};
    }

    Ast_গ্রন্থি_Stmt* createForceRdUpdateStmt(ধ্রুবক VarForceInfo& varInfo) ধ্রুবক {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* ধ্রুবক varp = varInfo.m_varVscp->varp();
        যদি (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            ফেরত createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_সারি* ধ্রুবক flp = varInfo.m_varVscp->fileline();
        ফেরত নতুন AstAssign{flp, নতুন AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             createForceRdExpression(varInfo, flp)};
    }

    Ast_গ্রন্থি_Stmt* createForceRdUpdateStmtUnpacked(ধ্রুবক VarForceInfo& varInfo) ধ্রুবক {
        File_সারি* ধ্রুবক flp = varInfo.m_varVscp->fileline();
        AstVar* ধ্রুবক varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* ধ্রুবক arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        ধ্রুবক std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_গ্রন্থি_Stmt* ধ্রুবক stmtsp = foreachUnpackedLeaf(
            dims, [&](ধ্রুবক std::vector<পূর্ণসংখ্যা>& idx, পূর্ণসংখ্যা /*flat*/) -> Ast_গ্রন্থি_Stmt* {
                AstVarRef* ধ্রুবক baseRefp = নতুন AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(baseRefp);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক baseSelp = buildNestedArraySel(flp, baseRefp, idx);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক enSelp = buildNestedArraySel(
                    flp, নতুন AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valSelp = buildNestedArraySel(
                    flp, নতুন AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক readExprp = নতুন AstOr{
                    flp, নতুন AstAnd{flp, enSelp, valSelp},
                    নতুন AstAnd{flp, নতুন AstNot{flp, enSelp->clone_বৃক্ষ_Pure(মিথ্যা)}, baseSelp}};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rdLhsSelp = buildNestedArraySel(
                    flp, নতুন AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                ফেরত নতুন AstAssign{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        জন্য (ধ্রুবক ForceInfo* ধ্রুবক finfop : forceInfosInIdOrder(varInfo)) {
            যদি (!finfop->m_isExternal) চালু;
            stmtsp->add_পরবর্তী(নতুন AstAssign{flp,
                                          নতুন AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_বৃক্ষ_Pure(মিথ্যা)});
        }
        ফেরত stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        ধ্রুবক auto it = m_varToId.find(vscp);
        যদি (it != m_varToId.end()) ফেরত m_varInfos[it->second];

        m_varToId.emplace(vscp, m_varInfos.আকার_2());
        m_varInfos.emplace_back();
        VarForceInfo& info = m_varInfos.back();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* ধ্রুবক varp = info.m_varp;
        যদি (!varp->isForceable()) ফেরত info;

        File_সারি* ধ্রুবক flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        ধ্রুবক যৌক্তিক helperVarsBuilt = helperVars.m_rdVarp != nullptr;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != nullptr)
                        && helperVarsBuilt == (helperVars.m_valVarp != nullptr),
                    varp, "Incomplete force helper set");
        যদি (!helperVarsBuilt) {
            ধ্রুবক যৌক্তিক unpacked = isUnpackedArrayDType(varp->dtypep());
            ধ্রুবক VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_গ্রন্থি_DType* ধ্রুবক enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
            helperVars.m_rdVarp
                = নতুন AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sigPublic(সত্য);
            helperVars.m_enVarp
                = নতুন AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(সত্য);
            helperVars.m_valVarp
                = নতুন AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(সত্য);
            varp->add_পরবর্তী_Here(helperVars.m_rdVarp);
            varp->add_পরবর্তী_Here(helperVars.m_enVarp);
            varp->add_পরবর্তী_Here(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = findScopeVar(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = findScopeVar(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = findScopeVar(info.m_scopep, helperVars.m_valVarp);
        যদি (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } অন্যথা {
            info.m_forceRdVscp = নতুন AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = নতুন AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = নতুন AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->addVarsp(info.m_forceRdVscp);
            info.m_scopep->addVarsp(info.m_forceEnVscp);
            info.m_scopep->addVarsp(info.m_forceValVscp);
        }
        ফেরত info;
    }

    শূন্য markClockedWrite(AstVar* varp) { m_clockedWrites.ঢোকাও(varp); }
    যৌক্তিক hasClockedWrite(AstVar* varp) ধ্রুবক { ফেরত m_clockedWrites.গুনতি(varp); }

    যৌক্তিক doingAssign() ধ্রুবক { ফেরত m_doingAssign; }

    ধ্রুবক VarForceInfo* getVarInfo(AstVarScope* vscp) ধ্রুবক {
        ধ্রুবক auto it = m_varToId.find(vscp);
        ফেরত it != m_varToId.end() ? &m_varInfos[it->second] : nullptr;
    }

    AstVarScope* findScopeVar(AstScope* scopep, ধ্রুবক AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        যদি (cache.empty()) {
            জন্য (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        ধ্রুবক auto it = cache.find(varp);
        ফেরত it != cache.end() ? it->second : nullptr;
    }
    শূন্য addForceAssignment(AstVar* varp, AstVarScope* vscp, Ast_গ্রন্থি_রাশিমালা* rhsExprp,
                            AstAssignForce* forceStmtp, পূর্ণসংখ্যা rangeLsb, পূর্ণসংখ্যা rangeMsb, পূর্ণসংখ্যা padLsb,
                            পূর্ণসংখ্যা padMsb, যৌক্তিক hasArraySel, যৌক্তিক skipRdRefresh = মিথ্যা) {
        v3Global.setUsesForce();
        varp->setForcedByCode();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        ধ্রুবক পূর্ণসংখ্যা forceId = info.m_forces.আকার_2();
        File_সারি* ধ্রুবক flp = varp->fileline();
        AstScope* ধ্রুবক scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        যদি (!info.m_forceVecVscp) {
            AstCDType* ধ্রুবক forceVecDtypep = নতুন AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->addTypesp(forceVecDtypep);

            AstVar* ধ্রুবক forceVecVarp
                = নতুন AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doingAssign ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(মিথ্যা);
            forceVecVarp->isInternal(সত্য);
            varp->add_পরবর্তী_Here(forceVecVarp);
            info.m_forceVecVscp = নতুন AstVarScope{flp, scopep, forceVecVarp};
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
            finfo.m_rhsExprp->foreach([&](AstVarRef* ধ্রুবক refp) {
                যদি (!refp->access().isReadOnly()) ফেরত;
                AstVar* ধ্রুবক depVarp = refp->varp();
                যদি (depVarp
                    && std::find(depVarps.begin(), depVarps.end(), depVarp) == depVarps.end()) {
                    depVarps.ঠেলো_back(depVarp);
                }
            });
            জন্য (AstVar* ধ্রুবক depVarp : depVarps) m_rhsDepToForces[depVarp].ঠেলো_back(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    স্থির শূন্য collectArraySelInfo(Ast_গ্রন্থি_রাশিমালা* exprp, ArraySelInfo& info) {
        যদি (ধ্রুবক auto* ধ্রুবক selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = সত্য;
            collectArraySelInfo(selp->fromp(), info);
        } অন্যথা যদি (auto* ধ্রুবক selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.ঠেলো_back(selp);
        } অন্যথা যদি (ধ্রুবক auto* ধ্রুবক memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    স্থির ArraySelInfo getArraySelInfo(Ast_গ্রন্থি_রাশিমালা* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        ফেরত info;
    }

    স্থির std::vector<পূর্ণসংখ্যা> arraySelDimSizes(ধ্রুবক ArraySelInfo& info) {
        std::vector<পূর্ণসংখ্যা> dims;
        dims.reserve(info.m_sels.আকার_2());
        জন্য (AstArraySel* ধ্রুবক selp : info.m_sels) {
            Ast_গ্রন্থি_DType* ধ্রুবক dtypep = selp->fromp()->dtypep()->skipRefp();
            ধ্রুবক AstUnpackArrayDType* ধ্রুবক arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.ঠেলো_back(arrayp->declRange().elements());
        }
        ফেরত dims;
    }

    স্থির পূর্ণসংখ্যা flatten_সূচক(ধ্রুবক std::vector<পূর্ণসংখ্যা>& indices, ধ্রুবক std::vector<পূর্ণসংখ্যা>& dimSizes) {
        UASSERT(indices.আকার_2() == dimSizes.আকার_2(), "Array index and dimension size mismatch");
        পূর্ণসংখ্যা সূচক = 0;
        পূর্ণসংখ্যা stride = 1;
        জন্য (পূর্ণসংখ্যা i = static_cast<পূর্ণসংখ্যা>(indices.আকার_2()) - 1; i >= 0; --i) {
            সূচক += indices[i] * stride;
            stride *= dimSizes[i];
        }
        ফেরত সূচক;
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* build_Flatten_সূচক_রাশিমালা(File_সারি* flp, ধ্রুবক ArraySelInfo& info) {
        ধ্রুবক std::vector<পূর্ণসংখ্যা> dimSizes = arraySelDimSizes(info);
        যৌক্তিক allConst = সত্য;
        জন্য (AstArraySel* ধ্রুবক selp : info.m_sels) {
            যদি (!VN_IS(selp->bitp(), Const)) {
                allConst = মিথ্যা;
                বিরতি;
            }
        }
        যদি (allConst) {
            std::vector<পূর্ণসংখ্যা> constIndices;
            constIndices.reserve(info.m_sels.আকার_2());
            জন্য (AstArraySel* ধ্রুবক selp : info.m_sels) {
                constIndices.ঠেলো_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            ফেরত makeConst32(flp, flatten_সূচক(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_গ্রন্থি_রাশিমালা* resultp = nullptr;
        পূর্ণসংখ্যা stride = 1;
        জন্য (পূর্ণসংখ্যা i = static_cast<পূর্ণসংখ্যা>(info.m_sels.আকার_2()) - 1; i >= 0; --i) {
            Ast_গ্রন্থি_রাশিমালা* termp = info.m_sels[i]->bitp()->clone_বৃক্ষ_Pure(মিথ্যা);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            যদি (termp->width() < 32) termp = নতুন AstExtend{flp, termp, 32};
            যদি (stride != 1) termp = নতুন AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? নতুন AstAdd{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        ফেরত resultp;
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* build_Rhs_তথ্য_রাশিমালা(File_সারি* flp, ধ্রুবক ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        ফেরত নতুন AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    শূন্য finalizeRhsVars() {
        জন্য (VarForceInfo& info : m_varInfos) {
            AstVar* ধ্রুবক varp = info.m_varp;
            যদি (info.m_forces.empty()) চালু;

            AstScope* ধ্রুবক scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_সারি* ধ্রুবক flp = varp->fileline();
            ধ্রুবক std::vector<ForceInfo*> forceps = forceInfosInIdOrder(info);

            জন্য (ForceInfo* ধ্রুবক finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* ধ্রুবক rhsVarp = নতুন AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doingAssign() ? "_VassignRHS" : "__VforceRHS")
                        + std::to_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(সত্য);
                rhsVarp->sigPublic(সত্য);
                rhsVarp->setForcedByCode();
                varp->add_পরবর্তী_Here(rhsVarp);
                finfo.m_rhsVarVscp = নতুন AstVarScope{flp, scopep, rhsVarp};
                scopep->addVarsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                // end
                AstAssign* ধ্রুবক rhsAssignp = নতুন AstAssign{
                    flp, নতুন AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                যদি (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* ধ্রুবক touchCallp = নতুন AstCMethodHard{
                        flp, নতুন AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtypeSetVoid();
                    Ast_গ্রন্থি_Stmt* ধ্রুবক touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->add_পরবর্তী_Here(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* ধ্রুবক alwaysp
                    = নতুন AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, rhsAssignp};
                Ast_Sen_বৃক্ষ* ধ্রুবক senTreep
                    = নতুন Ast_Sen_বৃক্ষ{flp, নতুন AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ধ্রুবক activep = নতুন AstActive{flp, "force-rhs-update", senTreep};
                activep->sen_বৃক্ষ_Storep(activep->sentreep());
                activep->addStmtsp(alwaysp);
                scopep->addBlocksp(activep);
            }

            যদি (info.m_forceRdVscp) {
                যদি (!m_forceRdUpdateBuilt.ঢোকাও(info.m_varVscp).second) চালু;
                AstActive* ধ্রুবক activeInitp = নতুন AstActive{
                    flp, "force-init",
                    নতুন Ast_Sen_বৃক্ষ{flp, নতুন AstSenItem{flp, AstSenItem::Static{}}}};
                activeInitp->sen_বৃক্ষ_Storep(activeInitp->sentreep());
                Ast_গ্রন্থি_Stmt* initStmtp = nullptr;
                যদি (AstUnpackArrayDType* ধ্রুবক arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ধ্রুবক std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    ধ্রুবক পূর্ণসংখ্যা innerWidth = dims.back()->subDTypep()->skipRefp()->width();
                    initStmtp = foreachUnpackedLeaf(
                        dims, [&](ধ্রুবক std::vector<পূর্ণসংখ্যা>& idx, পূর্ণসংখ্যা /*flat*/) -> Ast_গ্রন্থি_Stmt* {
                            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = buildNestedArraySel(
                                flp, নতুন AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            ফেরত নতুন AstAssign{flp, lhsp, makeZeroConst(varp, innerWidth)};
                        });
                } অন্যথা {
                    initStmtp = নতুন AstAssign{
                        flp, নতুন AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->width())};
                }
                initStmtp->add_পরবর্তী(createForceRdUpdateStmt(info));
                activeInitp->addStmtsp(নতুন AstInitial{flp, initStmtp});
                scopep->addBlocksp(activeInitp);

                AstSenItem* itemsp = nullptr;
                auto addSenItem = [&](AstVarScope* vscp) {
                    যদি (!vscp) ফেরত;
                    AstSenItem* ধ্রুবক nextp = নতুন AstSenItem{
                        flp, VEdgeType::ET_CHANGED, নতুন AstVarRef{flp, vscp, VAccess::READ}};
                    যদি (itemsp) {
                        itemsp->add_পরবর্তী(nextp);
                    } অন্যথা {
                        itemsp = nextp;
                    }
                };
                addSenItem(info.m_forceEnVscp);
                addSenItem(info.m_forceValVscp);
                AstVarRef* ধ্রুবক origSenRefp = নতুন AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                AstSenItem* ধ্রুবক origItemp
                    = নতুন AstSenItem{flp, VEdgeType::ET_CHANGED, origSenRefp};
                যদি (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->add_পরবর্তী(origItemp);
                জন্য (ForceInfo* ধ্রুবক finfop : forceps) {
                    যদি (!finfop->m_isExternal) addSenItem(finfop->m_rhsVarVscp);
                }

                AstActive* ধ্রুবক activep
                    = নতুন AstActive{flp, "force-rd-update", নতুন Ast_Sen_বৃক্ষ{flp, itemsp}};
                activep->sen_বৃক্ষ_Storep(activep->sentreep());
                activep->addStmtsp(নতুন AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr,
                                                 createForceRdUpdateStmt(info)});
                scopep->addBlocksp(activep);
            }
        }
    }

    Ast_গ্রন্থি* createRhsUpdatesForWrite(File_সারি* flp, AstVar* writtenVarp) ধ্রুবক {
        যদি (!doingAssign()) ফেরত nullptr;

        ধ্রুবক auto it = m_rhsDepToForces.find(writtenVarp);
        যদি (it == m_rhsDepToForces.end()) ফেরত nullptr;

        Ast_গ্রন্থি* headp = nullptr;
        Ast_গ্রন্থি* tailp = nullptr;
        জন্য (ধ্রুবক ForceInfo* ধ্রুবক finfop : it->second) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            AstAssign* ধ্রুবক updatep
                = নতুন AstAssign{flp, নতুন AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_বৃক্ষ_Pure(মিথ্যা)};
            যদি (tailp) {
                tailp->add_পরবর্তী_Here(updatep);
            } অন্যথা {
                headp = updatep;
            }
            tailp = updatep;
        }
        ফেরত headp;
    }

    ধ্রুবক ForceInfo& getForceInfo(AstAssignForce* forceStmtp) ধ্রুবক {
        AstVarScope* ধ্রুবক vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        ধ্রুবক VarForceInfo* ধ্রুবক varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        auto it2 = varInfo->m_forces.find(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.end(), "Force statement not found");
        ফেরত it2->second;
    }

    স্থির যৌক্তিক selOverlapsAnyForce(ধ্রুবক VarForceInfo& varInfo, পূর্ণসংখ্যা selLsb, পূর্ণসংখ্যা selMsb) {
        জন্য (ধ্রুবক auto& pair : varInfo.m_forces) {
            যদি (pair.second.m_rangeLsb <= selMsb && pair.second.m_rangeMsb >= selLsb) ফেরত সত্য;
        }
        ফেরত মিথ্যা;
    }

    Ast_গ্রন্থি_রাশিমালা* createForceReadExpression(ধ্রুবক VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) ধ্রুবক {
        File_সারি* ধ্রুবক flp = originalRefp->fileline();
        AstVarRef* ধ্রুবক refp = originalRefp->clone_বৃক্ষ_Pure(মিথ্যা);
        refp->access(VAccess::READ);
        ফেরত createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   nullptr);
    }

    Ast_গ্রন্থি_রাশিমালা* create_Force_Read_সূচক_Expression(ধ্রুবক VarForceInfo& varInfo,
                                                Ast_গ্রন্থি_রাশিমালা* originalExprp,
                                                Ast_গ্রন্থি_রাশিমালা* সূচক_Exprp) ধ্রুবক {
        File_সারি* ধ্রুবক flp = originalExprp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = originalExprp->clone_বৃক্ষ_Pure(মিথ্যা);
        // Must be an LValue to a static variable
        VN_AS(exprp->getVAccessTargetRecurse(), VarRef)->access(VAccess::READ);
        ফেরত createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_সূচক, exprp, originalExprp,
                                   সূচক_Exprp);
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* rebuildSelPath(Ast_গ্রন্থি_রাশিমালা* pathp, Ast_গ্রন্থি_রাশিমালা* baseExprp) {
        যদি (ধ্রুবক AstSel* ধ্রুবক selp = VN_CAST(pathp, Sel)) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = rebuildSelPath(selp->fromp(), baseExprp);
            AstSel* ধ্রুবক outp
                = নতুন AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->widthConst()};
            outp->dtypeFrom(selp);
            ফেরত outp;
        }
        ফেরত baseExprp;
    }
};

// Split deassign concat LHS before converting to release internals.
স্থির শূন্য splitDeassign(AstDeassign* nodep) {
    AstConcat* ধ্রুবক concatp = VN_CAST(nodep->lhsp(), Concat);
    যদি (!concatp) ফেরত;

    File_সারি* ধ্রুবক flp = nodep->fileline();
    AstDeassign* ধ্রুবক newLp = নতুন AstDeassign{flp, concatp->lhsp()->unlinkFrBack()};
    AstDeassign* ধ্রুবক newRp = নতুন AstDeassign{flp, concatp->rhsp()->unlinkFrBack()};
    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক conp = concatp->unlinkFrBack();
    nodep->replaceWith(newLp);
    newLp->add_পরবর্তী_Here(newRp);
    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    VL_DO_DANGLING(conp->delete_বৃক্ষ(), conp);

    splitDeassign(newLp);
    splitDeassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

শ্রেণী ForceDiscoveryVisitor final : সর্বজনীন VNVisitorConst {
    ForceState& m_state;
    যৌক্তিক m_inClockedActive = মিথ্যা;

    শূন্য buildForceableUnpackedArray(AstVarScope* ধ্রুবক nodep,
                                     AstUnpackArrayDType* ধ্রুবক arrDtypep) {
        AstVar* ধ্রুবক varp = nodep->varp();
        ধ্রুবক std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.empty(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক leafDtypep = dims.back()->subDTypep()->skipRefp();
        ধ্রুবক AstBasicDType* ধ্রুবক innerBasicp = leafDtypep->basicp();
        যদি (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            ফেরত;
        }

        File_সারি* ধ্রুবক flp = varp->fileline();
        ধ্রুবক পূর্ণসংখ্যা innerWidth = leafDtypep->width();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* ধ্রুবক enVscp = info.m_forceEnVscp;
        AstVarScope* ধ্রুবক valVscp = info.m_forceValVscp;

        AstSenItem* ধ্রুবক itemsp = নতুন AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                  নতুন AstVarRef{flp, enVscp, VAccess::READ}};
        AstActive* ধ্রুবক activep = নতুন AstActive{flp, "force-update", নতুন Ast_Sen_বৃক্ষ{flp, itemsp}};
        activep->sen_বৃক্ষ_Storep(activep->sentreep());

        Ast_গ্রন্থি_Stmt* ধ্রুবক alwaysBodyHeadp = ForceState::foreachUnpackedLeaf(
            dims, [&](ধ্রুবক std::vector<পূর্ণসংখ্যা>& idx, পূর্ণসংখ্যা flat) -> Ast_গ্রন্থি_Stmt* {
                AstVarRef* ধ্রুবক origRefp = নতুন AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক enSelp = ForceState::buildNestedArraySel(
                    flp, নতুন AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valSelp = ForceState::buildNestedArraySel(
                    flp, নতুন AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক forceExprp = নতুন AstOr{
                    flp, নতুন AstAnd{flp, enSelp, valSelp},
                    নতুন AstAnd{flp, নতুন AstNot{flp, enSelp->clone_বৃক্ষ_Pure(মিথ্যা)}, origSelp}};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsSelp = ForceState::buildNestedArraySel(
                    flp, নতুন AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                AstAssignForce* ধ্রুবক forceAssignp = নতুন AstAssignForce{flp, lhsSelp, forceExprp};
                forceAssignp->user2(সত্য);

                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsClonep = forceExprp->clone_বৃক্ষ_Pure(মিথ্যা);
                rhsClonep->foreach([varp](AstVarRef* ধ্রুবক r) {
                    যদি (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/innerWidth - 1,
                                           /*hasArraySel=*/সত্য, /*skipRdRefresh=*/সত্য);
                ফেরত forceAssignp;
            });
        activep->addStmtsp(নতুন AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, alwaysBodyHeadp});
        nodep->scopep()->addBlocksp(activep);
    }

    শূন্য visit(AstAssignForce* nodep) override {
        যদি (nodep->user2()) ফেরত;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* ধ্রুবক lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* ধ্রুবক forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, সত্য);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsExprp = nodep->rhsp()->clone_বৃক্ষ_Pure(মিথ্যা);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    শূন্য visit(AstAssign* nodep) override {
        যদি (m_state.doingAssign() && m_inClockedActive) {
            যদি (AstVarRef* ধ্রুবক lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
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
                ফেরত;
            }

            যদি (AstUnpackArrayDType* ধ্রুবক arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                ফেরত;
            }

            ধ্রুবক AstBasicDType* ধ্রুবক bdtypep = nodep->varp()->basicp();
            যদি (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* ধ্রুবক varp = nodep->varp();
            File_সারি* ধ্রুবক flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* ধ্রুবক enVscp = info.m_forceEnVscp;
            AstVarScope* ধ্রুবক valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            AstSenItem* ধ্রুবক itemsp = নতুন AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                      নতুন AstVarRef{flp, enVscp, VAccess::READ}};
            AstActive* ধ্রুবক activep
                = নতুন AstActive{flp, "force-update", নতুন Ast_Sen_বৃক্ষ{flp, itemsp}};
            activep->sen_বৃক্ষ_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* ধ্রুবক refp = নতুন AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* ধ্রুবক enRefp = নতুন AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* ধ্রুবক valRefp = নতুন AstVarRef{flp, valVscp, VAccess::READ};
            ধ্রুবক AstBasicDType* ধ্রুবক basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নতুন AstOr{
                            flp, নতুন AstAnd{flp, enRefp, valRefp},
                            নতুন AstAnd{flp, নতুন AstNot{flp, enRefp->clone_বৃক্ষ_Pure(মিথ্যা)}, refp}})
                      : static_cast<Ast_গ্রন্থি_রাশিমালা*>(নতুন AstCond{flp, enRefp, valRefp, refp});
            AstAssignForce* ধ্রুবক forceAssignp
                = নতুন AstAssignForce{flp, নতুন AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(সত্য);
            activep->addStmtsp(নতুন AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, forceAssignp});
            nodep->scopep()->addBlocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsClonep = forceExprp->clone_বৃক্ষ_Pure(মিথ্যা);
            rhsClonep->foreach([varp](AstVarRef* ধ্রুবক refp) {
                যদি (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            ধ্রুবক যৌক্তিক bitwiseVar = ForceState::isBitwiseDType(varp);
            ধ্রুবক পূর্ণসংখ্যা padMsb = bitwiseVar ? (varp->width() - 1) : 0;
            পূর্ণসংখ্যা rangeLsb = 0;
            পূর্ণসংখ্যা rangeMsb = padMsb;
            যদি (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, মিথ্যা);
        }
        iterateChildrenConst(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সর্বজনীন:
    explicit ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_পরবর্তী_Const_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

শ্রেণী ForceConvertVisitor final : সর্বজনীন VNVisitor {
    ForceState& m_state;

    শূন্য visit(AstAssignForce* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = nodep->lhsp();
        AstVarRef* ধ্রুবক lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ধ্রুবক forcedVarp = lhsVarRefp->varp();

        ধ্রুবক ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        ধ্রুবক ForceState::VarForceInfo* ধ্রুবক varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_সারি* ধ্রুবক flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        AstAssign* ধ্রুবক rhsAssignp
            = নতুন AstAssign{flp, নতুন AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_বৃক্ষ_Pure(মিথ্যা)};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        ধ্রুবক যৌক্তিক bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        যদি (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_গ্রন্থি_রাশিমালা* baseExprp = nodep->rhsp()->clone_বৃক্ষ_Pure(মিথ্যা);
            baseExprp->foreach(
                [](AstVarRef* ধ্রুবক refp) { ForceState::markNonReplaceable(refp); });
            যদি (bitwiseForcedVar) {
                baseExprp = ForceState::zeroPadToBaseWidth(baseExprp, forcedVarp->width(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            যদি (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* ধ্রুবক maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valReadp
                    = নতুন AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valWritep
                    = নতুন AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notMaskp = নতুন AstNot{flp, maskConstp};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক maskedOldp = নতুন AstAnd{flp, valReadp, notMaskp};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newValp = নতুন AstOr{flp, maskedOldp, baseExprp};
                valAssignp = নতুন AstAssign{flp, valWritep, newValp};

                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক enReadp
                    = নতুন AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক enWritep
                    = নতুন AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newEnp
                    = নতুন AstOr{flp, enReadp, maskConstp->clone_বৃক্ষ_Pure(মিথ্যা)};
                enAssignp = নতুন AstAssign{flp, enWritep, newEnp};
            } অন্যথা {
                AstConst* ধ্রুবক oneConstp = ForceState::makeRangeMaskConst(nodep, 1, 0, 0);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsValp = ForceState::cast_To_গ্রন্থি_DType(baseExprp, forcedVarp);
                valAssignp = নতুন AstAssign{
                    flp, নতুন AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = নতুন AstAssign{
                    flp, নতুন AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        ধ্রুবক AstSel* ধ্রুবক selLhsp = VN_CAST(lhsp, Sel);
        ধ্রুবক যৌক্তিক arrayBitSel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->width();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsDatap = ForceState::build_Rhs_তথ্য_রাশিমালা(flp, info);
        AstCExpr* ধ্রুবক rhsAddrp = নতুন AstCExpr{flp};
        rhsAddrp->add("&(");
        rhsAddrp->add(rhsDatap);
        rhsAddrp->add(")");
        AstCMethodHard* ধ্রুবক addForceCallp = নতুন AstCMethodHard{
            flp, নতুন AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ADD,
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
        Ast_গ্রন্থি_Stmt* ধ্রুবক stmtp = addForceCallp->makeStmt();

        Ast_গ্রন্থি* tailp = rhsAssignp;
        যদি (valAssignp) {
            tailp->add_পরবর্তী_Here(valAssignp);
            tailp = valAssignp;
        }
        যদি (enAssignp) {
            tailp->add_পরবর্তী_Here(enAssignp);
            tailp = enAssignp;
        }
        tailp->add_পরবর্তী_Here(stmtp);
        যদি (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->add_পরবর্তী_Here(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = nodep->lhsp();
        AstVarRef* ধ্রুবক lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ধ্রুবক releasedVarp = lhsVarRefp->varp();

        ধ্রুবক ForceState::VarForceInfo* ধ্রুবক varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        যদি (!varInfo) {
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ফেরত;
        }
        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        File_সারি* ধ্রুবক flp = nodep->fileline();

        ধ্রুবক ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, মিথ্যা);

        ধ্রুবক AstSel* ধ্রুবক selLhsp = VN_CAST(lhsp, Sel);
        ধ্রুবক যৌক্তিক arrayBitSel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->width();
        AstCMethodHard* ধ্রুবক releaseCallp = নতুন AstCMethodHard{
            flp, নতুন AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        যদি (arrayBitSel) {
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtypeSetVoid();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_গ্রন্থি_Stmt* ধ্রুবক releasep = releaseCallp->makeStmt();

        AstAssign* clearEnp = nullptr;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        যদি (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক enWritep
                = নতুন AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            যদি (ForceState::isBitwiseDType(releasedVarp)) {
                ধ্রুবক পূর্ণসংখ্যা varWidth = releasedVarp->width();
                যদি (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == varWidth - 1) {
                    clearEnp
                        = নতুন AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, varWidth)};
                } অন্যথা {
                    // forceEn = forceEn & ~mask(range);
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক enReadp
                        = নতুন AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* ধ্রুবক maskConst = ForceState::makeRangeMaskConst(
                        nodep, varWidth, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newEnp
                        = নতুন AstAnd{flp, enReadp, নতুন AstNot{flp, maskConst}};
                    clearEnp = নতুন AstAssign{flp, enWritep, newEnp};
                }
            } অন্যথা {
                clearEnp = নতুন AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        ধ্রুবক AstSel* ধ্রুবক selp = VN_CAST(lhsp, Sel);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক basep = selp ? selp->fromp() : lhsp;

        Ast_গ্রন্থি* stmtListp = releasep;
        যদি (clearEnp) {
            clearEnp->add_পরবর্তী_Here(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        ধ্রুবক যৌক্তিক fullBitwiseRelease
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->width() - 1;
        যদি (!releasedVarp->isContinuously()
            && !(m_state.doingAssign() && m_state.hasClockedWrite(releasedVarp)
                 && fullBitwiseRelease)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            ধ্রুবক যৌক্তিক hasOpaquePath = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_গ্রন্থি_রাশিমালা* forceReadp = nullptr;
            যদি (hasOpaquePath) {
                forceReadp = m_state.create_Force_Read_সূচক_Expression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } অন্যথা যদি (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.create_Force_Read_সূচক_Expression(
                    *varInfo, basep,
                    ForceState::build_Flatten_সূচক_রাশিমালা(flp, rangeInfo.m_arrayInfo));
                যদি (selp) { forceReadp = ForceState::rebuildSelPath(lhsp, forceReadp); }
            } অন্যথা {
                forceReadp
                    = selp ? ForceState::rebuildSelPath(
                                 lhsp, m_state.createForceReadExpression(*varInfo, lhsVarRefp))
                           : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            }
            AstAssign* ধ্রুবক assignp = নতুন AstAssign{flp, lhsp->clone_বৃক্ষ_Pure(মিথ্যা), forceReadp};
            assignp->add_পরবর্তী_Here(stmtListp);
            stmtListp = assignp;
        }

        যদি (varInfo->m_forceRdVscp) stmtListp->add_পরবর্তী(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_পরবর্তী_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

শ্রেণী ForceReplaceVisitor final : সর্বজনীন VNVisitor {
    ধ্রুবক ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_গ্রন্থি_Stmt* m_stmtp = nullptr;
    যৌক্তিক m_inLogic = মিথ্যা;

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
        যদি (AstVarRef* ধ্রুবক lhsp = VN_CAST(nodep->lhsp()->baseFromp(সত্য), VarRef)) {
            যদি (Ast_গ্রন্থি* ধ্রুবক updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->add_পরবর্তী_Here(updatep);
            }
        }
    }
    শূন্য visit(AstAssignCont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate_And_পরবর্তী_Null(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    শূন্য visit(AstDeassign*) override {}
    শূন্য visit(AstCFunc* nodep) override { iterateLogic(nodep); }
    শূন্য visit(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override { iterateLogic(nodep); }
    শূন্য visit(AstAlways* nodep) override {
        যদি (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            ফেরত;
        }
        iterateLogic(nodep);
    }
    শূন্য visit(AstSenItem* nodep) override { iterateLogic(nodep); }
    শূন্য visit(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* ধ্রুবক refp = VN_CAST(nodep->fromp(), VarRef);
        যদি (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            visit(static_cast<Ast_গ্রন্থি*>(nodep));
            ফেরত;
        }

        AstVar* ধ্রুবক varp = refp->varp();
        ধ্রুবক ForceState::VarForceInfo* ধ্রুবক varInfo = m_state.getVarInfo(refp->varScopep());
        যদি (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.empty()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            visit(static_cast<Ast_গ্রন্থি*>(nodep));
            ফেরত;
        }

        যদি (ধ্রুবক AstConst* ধ্রুবক lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            ধ্রুবক পূর্ণসংখ্যা selLsb = lsbConstp->toSInt();
            ধ্রুবক পূর্ণসংখ্যা selMsb = selLsb + nodep->width() - 1;
            যদি (!varp->isSigPublic()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                visit(static_cast<Ast_গ্রন্থি*>(nodep));
                ফেরত;
            }
        }

        File_সারি* ধ্রুবক flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* ধ্রুবক refClonep = refp->clone_বৃক্ষ_Pure(মিথ্যা);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* ধ্রুবক callp = নতুন AstCMethodHard{
            flp, নতুন AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->width())};
        callp->addPinsp(refClonep);
        callp->addPinsp(nodep->lsbp()->clone_বৃক্ষ_Pure(মিথ্যা));
        callp->addPinsp(ForceState::makeConst32(flp, nodep->width()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstArraySel* nodep) override {
        যদি (ধ্রুবক AstArraySel* ধ্রুবক backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            যদি (backSelp->fromp() == nodep) {
                iterateChildren(nodep);
                ফেরত;
            }
        }

        Ast_গ্রন্থি* ধ্রুবক basep = nodep->baseFromp(সত্য);
        AstVarRef* ধ্রুবক baseRefp = VN_CAST(basep, VarRef);
        যদি (!baseRefp) {
            iterateChildren(nodep);
            ফেরত;
        }
        AstVar* ধ্রুবক varp = baseRefp->varp();
        ধ্রুবক ForceState::VarForceInfo* ধ্রুবক varInfo = m_state.getVarInfo(baseRefp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        যদি (m_state.isNotReplaceable(baseRefp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            ফেরত;
        }

        যদি (!baseRefp->access().isReadOnly()) {
            iterateChildren(nodep);
            ফেরত;
        }
        যদি (m_state.doingAssign() && varInfo->m_forceRdVscp) {
            baseRefp->varp(varInfo->m_forceRdVscp->varp());
            baseRefp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            ফেরত;
        }
        ধ্রুবক ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        জন্য (AstArraySel* ধ্রুবক selp : arrayInfo.m_sels) iterate_And_পরবর্তী_Null(selp->bitp());
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক সূচক_Exprp
            = ForceState::build_Flatten_সূচক_রাশিমালা(nodep->fileline(), arrayInfo);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক readExprp
            = m_state.create_Force_Read_সূচক_Expression(*varInfo, nodep, সূচক_Exprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(AstVarRef* nodep) override {
        যদি (m_state.isNotReplaceable(nodep)) ফেরত;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        যদি (ধ্রুবক AstArraySel* ধ্রুবক backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            যদি (backSelp->fromp() == nodep) ফেরত;
        }

        ধ্রুবক ForceState::VarForceInfo* ধ্রুবক varInfo = m_state.getVarInfo(nodep->varScopep());
        যদি (!varInfo) ফেরত;

        যদি (varInfo->m_forceRdVscp) {
            যদি (nodep->access().isRW()) {
                যদি (m_inLogic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                ফেরত;
            }
            যদি (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক readp
                    = m_state.createForceRdExpression(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ফেরত;
            }
            যদি (m_inLogic && m_stmtp) {
                m_stmtp->add_পরবর্তী_Here(m_state.createForceRdUpdateStmt(*varInfo));
            }
            ফেরত;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        যদি (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            ফেরত;
        }

        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        যদি (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } অন্যথা যদি (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (ForceState::isOutermostOpaquePathSelector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = VN_AS(nodep, গ্রন্থি_রাশিমালা);
            Ast_গ্রন্থি* ধ্রুবক basep = exprp->baseFromp(সত্য);
            AstVarRef* ধ্রুবক baseRefp = VN_CAST(basep, VarRef);
            যদি (baseRefp) {
                AstVar* ধ্রুবক varp = baseRefp->varp();
                যদি (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    ধ্রুবক ForceState::VarForceInfo* ধ্রুবক varInfo
                        = m_state.getVarInfo(baseRefp->varScopep());
                    যদি (!m_state.isNotReplaceable(baseRefp) && varInfo) {
                        ধ্রুবক পূর্ণসংখ্যা force_Path_সূচক = varInfo->find_Force_Path_সূচক(exprp);
                        যদি (force_Path_সূচক >= 0) {
                            যদি (!baseRefp->access().isReadOnly()) ফেরত;
                            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক readExprp = m_state.create_Force_Read_সূচক_Expression(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), force_Path_সূচক));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                            ফেরত;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

সর্বজনীন:
    explicit ForceReplaceVisitor(AstNetlist* nodep, ধ্রুবক ForceState& state)
        : m_state{state} {
        iterate_And_পরবর্তী_Null(nodep->modulesp());
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
    যদি (!v3Global.hasForceableSignals()) ফেরত;
    ForceState state{মিথ্যা, helperVars, permanentlyProtected, forceRdUpdateBuilt};
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
    যদি (!v3Global.hasAssignDeassign()) ফেরত;

    std::vector<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.ঠেলো_back(deassignp); });
    জন্য (AstDeassign* ধ্রুবক deassignp : deassignps) splitDeassign(deassignp);

    std::vector<AstAssignCont*> assignContps;
    deassignps.clear();
    nodep->foreach([&](Ast_গ্রন্থি_Stmt* nodep) {
        যদি (AstAssignCont* ধ্রুবক assignContp = VN_CAST(nodep, AssignCont)) {
            assignContps.ঠেলো_back(assignContp);
        } অন্যথা যদি (AstDeassign* ধ্রুবক deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.ঠেলো_back(deassignp);
        }
    });

    জন্য (AstAssignCont* ধ্রুবক assignp : assignContps) {
        assignp->replaceWith(নতুন AstAssignForce{assignp->fileline(),
                                                assignp->lhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
        VL_DO_DANGLING(assignp->delete_বৃক্ষ(), assignp);
    }
    জন্য (AstDeassign* ধ্রুবক deassignp : deassignps) {
        deassignp->replaceWith(
            নতুন AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_বৃক্ষ_Pure(সত্য)});
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
    ধ্রুবক VNUser3InUse user3InUse;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::ProtectedVarRefSet permanentlyProtected;
    ForceState::BuiltVarScopeSet forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    assignAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
